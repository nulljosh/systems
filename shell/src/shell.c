/*
 * shell -- Unix shell from scratch
 * C99, single file, zero dependencies beyond POSIX
 *
 * Features: REPL, quoting, $VAR expansion, pipes, redirects,
 *           builtins (cd/exit/export/history/fg/bg), job control, signals
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LINE   4096
#define MAX_TOKENS 256
#define MAX_HIST   128
#define MAX_JOBS   64

/* ── Token types ────────────────────────────────────────────────── */

typedef enum {
    TOK_WORD,
    TOK_PIPE,
    TOK_REDIR_OUT,    /* >  */
    TOK_REDIR_APPEND, /* >> */
    TOK_REDIR_IN,     /* <  */
    TOK_BG,           /* &  */
} token_type_t;

typedef struct {
    token_type_t type;
    char        *value;
} token_t;

/* ── Command / pipeline ─────────────────────────────────────────── */

typedef struct {
    char *argv[MAX_TOKENS];
    int   argc;
    char *redir_in;
    char *redir_out;
    int   append;
} command_t;

typedef struct {
    command_t cmds[MAX_TOKENS];
    int       ncmds;
    int       bg;
} pipeline_t;

/* ── Job tracking ───────────────────────────────────────────────── */

typedef struct {
    pid_t pid;
    int   id;
    char  cmd[MAX_LINE];
    int   active;
} job_t;

static job_t  jobs[MAX_JOBS];
static int    next_job_id = 1;

/* ── History ────────────────────────────────────────────────────── */

static char *history[MAX_HIST];
static int   hist_count = 0;

static void hist_add(const char *line) {
    if (hist_count < MAX_HIST) {
        history[hist_count++] = strdup(line);
    } else {
        free(history[0]);
        memmove(history, history + 1, (MAX_HIST - 1) * sizeof(char *));
        history[MAX_HIST - 1] = strdup(line);
    }
}

/* ── Signal handling ────────────────────────────────────────────── */

static volatile sig_atomic_t got_sigchld = 0;

static void sigchld_handler(int sig) {
    (void)sig;
    got_sigchld = 1;
}

static void setup_signals(void) {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);

    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);
}

static void reap_bg(void) {
    int status;
    pid_t p;
    while ((p = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < MAX_JOBS; i++) {
            if (jobs[i].active && jobs[i].pid == p) {
                fprintf(stderr, "[%d] done  %s\n", jobs[i].id, jobs[i].cmd);
                jobs[i].active = 0;
                break;
            }
        }
    }
}

/* ── Tokenizer ──────────────────────────────────────────────────── */

static int tokenize(const char *line, token_t *tokens, int max) {
    int n = 0;
    const char *p = line;

    while (*p && n < max - 1) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p || *p == '\n') break;

        if (*p == '|') {
            tokens[n++] = (token_t){TOK_PIPE, NULL};
            p++;
        } else if (*p == '&') {
            tokens[n++] = (token_t){TOK_BG, NULL};
            p++;
        } else if (*p == '>' && *(p + 1) == '>') {
            tokens[n++] = (token_t){TOK_REDIR_APPEND, NULL};
            p += 2;
        } else if (*p == '>') {
            tokens[n++] = (token_t){TOK_REDIR_OUT, NULL};
            p++;
        } else if (*p == '<') {
            tokens[n++] = (token_t){TOK_REDIR_IN, NULL};
            p++;
        } else {
            /* word: plain, single-quoted, or double-quoted */
            char buf[MAX_LINE];
            int  len = 0;
            while (*p && *p != ' ' && *p != '\t' && *p != '\n' &&
                   *p != '|' && *p != '&' && *p != '>' && *p != '<') {
                if (*p == '\'') {
                    p++;
                    while (*p && *p != '\'') {
                        if (len < MAX_LINE - 1) buf[len++] = *p;
                        p++;
                    }
                    if (*p == '\'') p++;
                    else {
                        fprintf(stderr, "parse error: unclosed quote\n");
                        return -1;
                    }
                } else if (*p == '"') {
                    p++;
                    while (*p && *p != '"') {
                        if (*p == '$') {
                            /* expand $VAR inside double quotes */
                            p++;
                            char var[256];
                            int vl = 0;
                            while (*p && (isalnum((unsigned char)*p) || *p == '_')) {
                                if (vl >= 255) break;
                                var[vl++] = *p++;
                            }
                            var[vl] = '\0';
                            const char *val = getenv(var);
                            if (val) {
                                int slen = strlen(val);
                                for (int i = 0; i < slen && len < MAX_LINE - 1; i++)
                                    buf[len++] = val[i];
                            }
                        } else {
                            if (len < MAX_LINE - 1) buf[len++] = *p;
                            p++;
                        }
                    }
                    if (*p == '"') p++;
                    else {
                        fprintf(stderr, "parse error: unclosed quote\n");
                        return -1;
                    }
                } else if (*p == '$') {
                    /* unquoted $VAR expansion */
                    p++;
                    char var[256];
                    int vl = 0;
                    while (*p && (isalnum((unsigned char)*p) || *p == '_')) {
                        if (vl >= 255) break;
                        var[vl++] = *p++;
                    }
                    var[vl] = '\0';
                    const char *val = getenv(var);
                    if (val) {
                        int slen = strlen(val);
                        for (int i = 0; i < slen && len < MAX_LINE - 1; i++)
                            buf[len++] = val[i];
                    }
                } else {
                    if (len < MAX_LINE - 1) buf[len++] = *p;
                    p++;
                }
            }
            buf[len] = '\0';
            tokens[n++] = (token_t){TOK_WORD, strdup(buf)};
        }
    }
    return n;
}

/* ── Pipeline builder ───────────────────────────────────────────── */

static int build_pipeline(token_t *tokens, int ntok, pipeline_t *pl) {
    memset(pl, 0, sizeof(*pl));
    pl->ncmds = 1;
    command_t *cmd = &pl->cmds[0];
    cmd->argc = 0;
    cmd->redir_in = cmd->redir_out = NULL;
    cmd->append = 0;

    for (int i = 0; i < ntok; i++) {
        switch (tokens[i].type) {
        case TOK_PIPE:
            if (cmd->argc == 0) {
                fprintf(stderr, "parse error: empty pipe segment\n");
                return -1;
            }
            cmd->argv[cmd->argc] = NULL;
            pl->ncmds++;
            cmd = &pl->cmds[pl->ncmds - 1];
            cmd->argc = 0;
            cmd->redir_in = cmd->redir_out = NULL;
            cmd->append = 0;
            break;
        case TOK_REDIR_OUT:
        case TOK_REDIR_APPEND:
            if (i + 1 >= ntok || tokens[i + 1].type != TOK_WORD) {
                fprintf(stderr, "parse error: redirect missing filename\n");
                return -1;
            }
            cmd->redir_out = tokens[++i].value;
            cmd->append = (tokens[i - 1].type == TOK_REDIR_APPEND);
            break;
        case TOK_REDIR_IN:
            if (i + 1 >= ntok || tokens[i + 1].type != TOK_WORD) {
                fprintf(stderr, "parse error: redirect missing filename\n");
                return -1;
            }
            cmd->redir_in = tokens[++i].value;
            break;
        case TOK_BG:
            pl->bg = 1;
            break;
        case TOK_WORD:
            cmd->argv[cmd->argc++] = tokens[i].value;
            break;
        }
    }
    if (cmd->argc == 0 && pl->ncmds > 1) {
        fprintf(stderr, "parse error: empty pipe segment\n");
        return -1;
    }
    cmd->argv[cmd->argc] = NULL;
    return 0;
}

/* ── Builtins ───────────────────────────────────────────────────── */

static int builtin_cd(command_t *cmd) {
    const char *path = cmd->argc > 1 ? cmd->argv[1] : getenv("HOME");
    if (!path) path = "/";
    if (*path == '~') {
        char expanded[MAX_LINE];
        const char *home = getenv("HOME");
        if (home) {
            snprintf(expanded, sizeof(expanded), "%s%s", home, path + 1);
            path = expanded;
        }
    }
    if (chdir(path) != 0)
        fprintf(stderr, "cd: no such directory: %s\n", path);
    return 1;
}

static int builtin_exit(command_t *cmd) {
    int code = cmd->argc > 1 ? atoi(cmd->argv[1]) : 0;
    exit(code);
}

static int builtin_export(command_t *cmd) {
    for (int i = 1; i < cmd->argc; i++) {
        char *eq = strchr(cmd->argv[i], '=');
        if (eq) {
            *eq = '\0';
            setenv(cmd->argv[i], eq + 1, 1);
            *eq = '=';
        }
    }
    return 1;
}

static int builtin_history(command_t *cmd) {
    (void)cmd;
    for (int i = 0; i < hist_count; i++)
        printf("%4d  %s\n", i + 1, history[i]);
    return 1;
}

static int builtin_fg(command_t *cmd) {
    int target = -1;
    if (cmd->argc > 1) target = atoi(cmd->argv[1]);

    for (int i = MAX_JOBS - 1; i >= 0; i--) {
        if (!jobs[i].active) continue;
        if (target > 0 && jobs[i].id != target) continue;
        fprintf(stderr, "[%d] fg  %s\n", jobs[i].id, jobs[i].cmd);
        int status;
        waitpid(jobs[i].pid, &status, 0);
        jobs[i].active = 0;
        return 1;
    }
    fprintf(stderr, "fg: no such job\n");
    return 1;
}

static int builtin_bg(command_t *cmd) {
    (void)cmd;
    /* bg just reports active jobs -- actual resume would need SIGCONT */
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active)
            fprintf(stderr, "[%d] running  %s\n", jobs[i].id, jobs[i].cmd);
    }
    return 1;
}

static int try_builtin(command_t *cmd) {
    if (!cmd->argv[0]) return 0;
    if (strcmp(cmd->argv[0], "cd") == 0) return builtin_cd(cmd);
    if (strcmp(cmd->argv[0], "exit") == 0) return builtin_exit(cmd);
    if (strcmp(cmd->argv[0], "export") == 0) return builtin_export(cmd);
    if (strcmp(cmd->argv[0], "history") == 0) return builtin_history(cmd);
    if (strcmp(cmd->argv[0], "fg") == 0) return builtin_fg(cmd);
    if (strcmp(cmd->argv[0], "bg") == 0) return builtin_bg(cmd);
    return 0;
}

/* ── Redirect setup (called in child) ──────────────────────────── */

static void setup_redirects(command_t *cmd) {
    if (cmd->redir_in) {
        int fd = open(cmd->redir_in, O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "%s: %s\n", cmd->redir_in, strerror(errno));
            _exit(1);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    if (cmd->redir_out) {
        int flags = O_WRONLY | O_CREAT | (cmd->append ? O_APPEND : O_TRUNC);
        int fd = open(cmd->redir_out, flags, 0644);
        if (fd < 0) {
            fprintf(stderr, "%s: %s\n", cmd->redir_out, strerror(errno));
            _exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
}

/* ── Executor ───────────────────────────────────────────────────── */

static void add_job(pid_t pid, const char *line) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].active) {
            jobs[i].pid = pid;
            jobs[i].id = next_job_id++;
            strncpy(jobs[i].cmd, line, MAX_LINE - 1);
            jobs[i].cmd[MAX_LINE - 1] = '\0';
            jobs[i].active = 1;
            fprintf(stderr, "[%d] %d\n", jobs[i].id, pid);
            return;
        }
    }
    fprintf(stderr, "shell: too many background jobs\n");
}

static void exec_pipeline(pipeline_t *pl, const char *line) {
    int n = pl->ncmds;

    /* single builtin -- no fork */
    if (n == 1 && !pl->bg && try_builtin(&pl->cmds[0]))
        return;

    int prev_fd = -1;
    pid_t last_pid = -1;

    for (int i = 0; i < n; i++) {
        int pipefd[2] = {-1, -1};
        if (i < n - 1) {
            if (pipe(pipefd) < 0) {
                perror("pipe");
                return;
            }
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return;
        }

        if (pid == 0) {
            /* child: restore signals */
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);

            /* wire up pipe input */
            if (prev_fd >= 0) {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }
            /* wire up pipe output */
            if (pipefd[1] >= 0) {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);
            }

            setup_redirects(&pl->cmds[i]);
            execvp(pl->cmds[i].argv[0], pl->cmds[i].argv);
            fprintf(stderr, "%s: command not found\n", pl->cmds[i].argv[0]);
            _exit(127);
        }

        /* parent */
        if (prev_fd >= 0) close(prev_fd);
        if (pipefd[1] >= 0) close(pipefd[1]);
        prev_fd = pipefd[0];
        last_pid = pid;
    }

    if (pl->bg) {
        add_job(last_pid, line);
    } else {
        int status;
        while (waitpid(-1, &status, 0) > 0)
            ;
    }
}

/* ── Prompt ─────────────────────────────────────────────────────── */

static void print_prompt(void) {
    char cwd[MAX_LINE];
    if (!getcwd(cwd, sizeof(cwd))) strcpy(cwd, "?");

    const char *home = getenv("HOME");
    if (home && strncmp(cwd, home, strlen(home)) == 0) {
        printf("~%s $ ", cwd + strlen(home));
    } else {
        printf("%s $ ", cwd);
    }
    fflush(stdout);
}

/* ── Main loop ──────────────────────────────────────────────────── */

int main(void) {
    setup_signals();

    char line[MAX_LINE];
    while (1) {
        reap_bg();
        print_prompt();

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        /* strip trailing newline */
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';

        /* skip blank lines */
        const char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) continue;

        hist_add(line);

        token_t tokens[MAX_TOKENS];
        int ntok = tokenize(line, tokens, MAX_TOKENS);
        if (ntok < 0) continue;
        if (ntok == 0) continue;

        pipeline_t pl;
        if (build_pipeline(tokens, ntok, &pl) < 0) continue;
        if (pl.cmds[0].argc == 0) continue;

        exec_pipeline(&pl, line);

        /* free token values */
        for (int i = 0; i < ntok; i++)
            free(tokens[i].value);
    }

    /* free history */
    for (int i = 0; i < hist_count; i++)
        free(history[i]);

    return 0;
}
