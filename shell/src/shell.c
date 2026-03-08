#define _POSIX_C_SOURCE 200809L

/*
 * shell -- Unix shell from scratch
 * C99, single file, POSIX-only
 *
 * Features:
 * - REPL with termios raw-mode line editor
 * - quoting + $VAR expansion
 * - pipes + redirects
 * - builtins: cd/exit/export/history/fg/bg/alias
 * - job control + signal handling
 * - up/down history navigation + Ctrl+R reverse search
 * - tab completion (PATH commands + cwd files)
 * - git branch in prompt (reads .git/HEAD)
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_LINE        4096
#define MAX_TOKENS       256
#define MAX_HIST         256
#define MAX_JOBS          64
#define MAX_ALIASES      128
#define MAX_ALIAS_DEPTH   16
#define MAX_CANDIDATES  1024

/* -------------------------------------------------------------------------- */
/* Utilities                                                                  */
/* -------------------------------------------------------------------------- */

static int is_blank_line(const char *s) {
    while (*s == ' ' || *s == '\t') s++;
    return *s == '\0';
}

static size_t strlcpy_local(char *dst, const char *src, size_t dsz) {
    size_t sl = strlen(src);
    if (dsz > 0) {
        size_t n = (sl >= dsz) ? dsz - 1 : sl;
        memcpy(dst, src, n);
        dst[n] = '\0';
    }
    return sl;
}

static int is_executable_file(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    if (!S_ISREG(st.st_mode)) return 0;
    return access(path, X_OK) == 0;
}

/* -------------------------------------------------------------------------- */
/* Token model                                                                */
/* -------------------------------------------------------------------------- */

typedef enum {
    TOK_WORD,
    TOK_PIPE,
    TOK_REDIR_OUT,
    TOK_REDIR_APPEND,
    TOK_REDIR_IN,
    TOK_BG,
    TOK_AND,
    TOK_OR,
} token_type_t;

typedef struct {
    token_type_t type;
    char *value;
} token_t;

/* -------------------------------------------------------------------------- */
/* Command / pipeline                                                         */
/* -------------------------------------------------------------------------- */

typedef struct {
    char *argv[MAX_TOKENS];
    int argc;
    char *redir_in;
    char *redir_out;
    int append;
    int owned_argv[MAX_TOKENS];
    int owned_redir_in;
    int owned_redir_out;
} command_t;

typedef struct {
    command_t cmds[MAX_TOKENS];
    int ncmds;
    int bg;
} pipeline_t;

/* -------------------------------------------------------------------------- */
/* Job tracking                                                               */
/* -------------------------------------------------------------------------- */

typedef struct {
    pid_t pid;
    int id;
    char cmd[MAX_LINE];
    int active;
} job_t;

static job_t jobs[MAX_JOBS];
static int next_job_id = 1;
static int last_exit_status = 0;

/* -------------------------------------------------------------------------- */
/* History                                                                    */
/* -------------------------------------------------------------------------- */

static char *history[MAX_HIST];
static int hist_count = 0;

static void hist_add(const char *line) {
    if (!line || !*line) return;
    if (hist_count < MAX_HIST) {
        history[hist_count++] = strdup(line);
    } else {
        free(history[0]);
        memmove(history, history + 1, (MAX_HIST - 1) * sizeof(char *));
        history[MAX_HIST - 1] = strdup(line);
    }
}

static void hist_free_all(void) {
    for (int i = 0; i < hist_count; i++) {
        free(history[i]);
    }
    hist_count = 0;
}

/* -------------------------------------------------------------------------- */
/* Alias table                                                                */
/* -------------------------------------------------------------------------- */

typedef struct {
    char *name;
    char *value;
    int used;
} alias_t;

static alias_t aliases[MAX_ALIASES];

static alias_t *alias_find(const char *name) {
    for (int i = 0; i < MAX_ALIASES; i++) {
        if (aliases[i].used && strcmp(aliases[i].name, name) == 0) return &aliases[i];
    }
    return NULL;
}

static int alias_set(const char *name, const char *value) {
    if (!name || !*name || !value) return -1;
    alias_t *a = alias_find(name);
    if (a) {
        char *nv = strdup(value);
        if (!nv) return -1;
        free(a->value);
        a->value = nv;
        return 0;
    }
    for (int i = 0; i < MAX_ALIASES; i++) {
        if (!aliases[i].used) {
            aliases[i].name = strdup(name);
            aliases[i].value = strdup(value);
            if (!aliases[i].name || !aliases[i].value) {
                free(aliases[i].name);
                free(aliases[i].value);
                aliases[i].name = aliases[i].value = NULL;
                aliases[i].used = 0;
                return -1;
            }
            aliases[i].used = 1;
            return 0;
        }
    }
    return -1;
}

static void alias_print_one(const alias_t *a) {
    printf("alias %s='%s'\n", a->name, a->value);
}

static void alias_list_all(void) {
    for (int i = 0; i < MAX_ALIASES; i++) {
        if (aliases[i].used) alias_print_one(&aliases[i]);
    }
}

static void alias_free_all(void) {
    for (int i = 0; i < MAX_ALIASES; i++) {
        if (aliases[i].used) {
            free(aliases[i].name);
            free(aliases[i].value);
            aliases[i].name = NULL;
            aliases[i].value = NULL;
            aliases[i].used = 0;
        }
    }
}

/* -------------------------------------------------------------------------- */
/* Signal handling                                                            */
/* -------------------------------------------------------------------------- */

static volatile sig_atomic_t got_sigchld = 0;

static void sigchld_handler(int sig) {
    (void)sig;
    got_sigchld = 1;
}

static void setup_signals(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

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
    got_sigchld = 0;
}

/* -------------------------------------------------------------------------- */
/* Terminal raw mode + line editor                                            */
/* -------------------------------------------------------------------------- */

static struct termios g_orig_termios;
static int g_raw_enabled = 0;

static void disable_raw_mode(void) {
    if (g_raw_enabled) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
        g_raw_enabled = 0;
    }
}

static void enable_raw_mode(void) {
    if (!isatty(STDIN_FILENO) || g_raw_enabled) return;
    if (tcgetattr(STDIN_FILENO, &g_orig_termios) < 0) return;

    struct termios raw = g_orig_termios;
    raw.c_iflag &= (tcflag_t)~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= (tcflag_t)~(OPOST);
    raw.c_cflag |= (tcflag_t)(CS8);
    raw.c_lflag &= (tcflag_t)~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0) {
        g_raw_enabled = 1;
        atexit(disable_raw_mode);
    }
}

static void editor_write(const char *s) {
    if (!s) return;
    write(STDOUT_FILENO, s, strlen(s));
}

static void editor_writef(const char *fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    editor_write(buf);
}

static void editor_refresh(const char *prompt, const char *buf, int len, int cur) {
    int plen = (int)strlen(prompt);
    editor_write("\r\x1b[2K");
    write(STDOUT_FILENO, prompt, (size_t)plen);
    if (len > 0) write(STDOUT_FILENO, buf, (size_t)len);
    if (cur < len) {
        editor_write("\r");
        editor_writef("\x1b[%dC", plen + cur);
    }
    fflush(stdout);
}

typedef enum {
    KEY_NULL = 0,
    KEY_CTRL_A = 1,
    KEY_CTRL_B = 2,
    KEY_CTRL_C = 3,
    KEY_CTRL_D = 4,
    KEY_CTRL_E = 5,
    KEY_CTRL_F = 6,
    KEY_CTRL_G = 7,
    KEY_BACKSPACE = 127,
    KEY_ENTER = 13,
    KEY_ESC = 27,
    KEY_TAB = 9,
    KEY_CTRL_R = 18,
    KEY_UP = 1001,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_HOME,
    KEY_END,
    KEY_DELETE,
} editor_key_t;

static editor_key_t editor_read_key(void) {
    unsigned char c;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n <= 0) return KEY_NULL;

    if (c == KEY_ESC) {
        unsigned char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return KEY_ESC;
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return KEY_ESC;

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return KEY_ESC;
                if (seq[2] == '~') {
                    switch (seq[1]) {
                    case '1': return KEY_HOME;
                    case '3': return KEY_DELETE;
                    case '4': return KEY_END;
                    case '7': return KEY_HOME;
                    case '8': return KEY_END;
                    default: return KEY_ESC;
                    }
                }
            } else {
                switch (seq[1]) {
                case 'A': return KEY_UP;
                case 'B': return KEY_DOWN;
                case 'C': return KEY_RIGHT;
                case 'D': return KEY_LEFT;
                case 'H': return KEY_HOME;
                case 'F': return KEY_END;
                default: return KEY_ESC;
                }
            }
        }
        return KEY_ESC;
    }

    if (c == '\r' || c == '\n') return KEY_ENTER;
    if (c == 8) return KEY_BACKSPACE;
    return (editor_key_t)c;
}

static int command_pos_prefix(const char *buf, int cursor, char *out, size_t outsz, int *is_first_word) {
    int start = cursor;
    while (start > 0 && buf[start - 1] != ' ' && buf[start - 1] != '\t') start--;

    int j = start;
    while (j > 0 && (buf[j - 1] == ' ' || buf[j - 1] == '\t')) j--;
    int first = 0;
    if (j == 0) {
        first = 1;
    } else {
        char prev = buf[j - 1];
        if (prev == '|' || prev == '&') first = 1;
    }

    int plen = cursor - start;
    if (plen < 0) plen = 0;
    if ((size_t)plen >= outsz) plen = (int)outsz - 1;
    memcpy(out, buf + start, (size_t)plen);
    out[plen] = '\0';
    if (is_first_word) *is_first_word = first;
    return start;
}

static int add_candidate(char **cands, int *count, const char *name) {
    for (int i = 0; i < *count; i++) {
        if (strcmp(cands[i], name) == 0) return 0;
    }
    if (*count >= MAX_CANDIDATES) return -1;
    cands[*count] = strdup(name);
    if (!cands[*count]) return -1;
    (*count)++;
    return 0;
}

static void free_candidates(char **cands, int count) {
    for (int i = 0; i < count; i++) free(cands[i]);
}

static int gather_completion_candidates(const char *prefix, int first_word, char **cands) {
    int count = 0;
    size_t plen = strlen(prefix);

    DIR *d = opendir(".");
    if (d) {
        struct dirent *de;
        while ((de = readdir(d)) != NULL) {
            const char *name = de->d_name;
            if (name[0] == '.' && plen == 0) continue;
            if (strncmp(name, prefix, plen) == 0) {
                add_candidate(cands, &count, name);
            }
        }
        closedir(d);
    }

    if (first_word) {
        const char *path = getenv("PATH");
        if (!path) path = "/bin:/usr/bin";
        char *dup = strdup(path);
        if (!dup) return count;

        char *save = NULL;
        for (char *dir = strtok_r(dup, ":", &save); dir; dir = strtok_r(NULL, ":", &save)) {
            DIR *pd = opendir(*dir ? dir : ".");
            if (!pd) continue;
            struct dirent *de;
            while ((de = readdir(pd)) != NULL) {
                if (strncmp(de->d_name, prefix, plen) != 0) continue;
                char full[PATH_MAX];
                snprintf(full, sizeof(full), "%s/%s", *dir ? dir : ".", de->d_name);
                if (is_executable_file(full)) {
                    add_candidate(cands, &count, de->d_name);
                }
            }
            closedir(pd);
        }
        free(dup);
    }

    return count;
}

static int common_prefix_len(char **cands, int count) {
    if (count <= 0) return 0;
    int l = (int)strlen(cands[0]);
    for (int i = 1; i < count; i++) {
        int j = 0;
        while (j < l && cands[i][j] && cands[0][j] == cands[i][j]) j++;
        l = j;
    }
    return l;
}

static void insert_text(char *buf, int *len, int *cur, int max, const char *text) {
    int add = (int)strlen(text);
    if (add <= 0) return;
    if (*len + add >= max) add = max - *len - 1;
    if (add <= 0) return;

    memmove(buf + *cur + add, buf + *cur, (size_t)(*len - *cur + 1));
    memcpy(buf + *cur, text, (size_t)add);
    *len += add;
    *cur += add;
}

static void replace_range(char *buf, int *len, int *cur, int max, int start, int end, const char *text) {
    if (start < 0) start = 0;
    if (end < start) end = start;
    if (end > *len) end = *len;

    int old = end - start;
    int add = (int)strlen(text);
    int delta = add - old;

    if (*len + delta >= max) {
        add = max - (*len - old) - 1;
        if (add < 0) add = 0;
        delta = add - old;
    }

    if (delta != 0) {
        memmove(buf + end + delta, buf + end, (size_t)(*len - end + 1));
    }
    if (add > 0) memcpy(buf + start, text, (size_t)add);
    *len += delta;
    *cur = start + add;
}

static const char *history_search_reverse(const char *needle, int from) {
    if (!needle || !*needle) return NULL;
    if (from >= hist_count) from = hist_count - 1;
    for (int i = from; i >= 0; i--) {
        if (strstr(history[i], needle)) return history[i];
    }
    return NULL;
}

static int editor_ctrl_r_search(const char *prompt, char *buf, int *len, int *cur, int max) {
    char query[MAX_LINE];
    int qlen = 0;
    query[0] = '\0';
    int search_from = hist_count - 1;
    const char *match = NULL;

    while (1) {
        editor_write("\r\x1b[2K");
        if (qlen > 0) {
            match = history_search_reverse(query, search_from);
            editor_writef("(reverse-i-search)`%s`: %s", query, match ? match : "");
        } else {
            editor_write("(reverse-i-search)``: ");
        }

        editor_key_t k = editor_read_key();
        if (k == KEY_ENTER) {
            if (match) {
                strlcpy_local(buf, match, (size_t)max);
                *len = (int)strlen(buf);
                *cur = *len;
            }
            editor_refresh(prompt, buf, *len, *cur);
            return 0;
        }
        if (k == KEY_ESC || k == KEY_CTRL_G) {
            editor_refresh(prompt, buf, *len, *cur);
            return 0;
        }
        if (k == KEY_CTRL_R) {
            if (qlen > 0 && match) {
                for (int i = hist_count - 1; i >= 0; i--) {
                    if (history[i] == match) {
                        search_from = i - 1;
                        if (search_from < 0) search_from = hist_count - 1;
                        break;
                    }
                }
            }
            continue;
        }
        if (k == KEY_BACKSPACE || k == KEY_DELETE) {
            if (qlen > 0) {
                qlen--;
                query[qlen] = '\0';
                search_from = hist_count - 1;
            }
            continue;
        }
        if ((int)k >= 32 && (int)k < 127) {
            if (qlen < max - 1) {
                query[qlen++] = (char)k;
                query[qlen] = '\0';
                search_from = hist_count - 1;
            }
            continue;
        }
    }
}

static void editor_tab_complete(const char *prompt, char *buf, int *len, int *cur, int max) {
    char prefix[MAX_LINE];
    int first_word = 0;
    int start = command_pos_prefix(buf, *cur, prefix, sizeof(prefix), &first_word);

    char *cands[MAX_CANDIDATES];
    int n = gather_completion_candidates(prefix, first_word, cands);
    if (n <= 0) {
        editor_write("\a");
        return;
    }

    if (n == 1) {
        char insert[MAX_LINE];
        snprintf(insert, sizeof(insert), "%s ", cands[0]);
        replace_range(buf, len, cur, max, start, *cur, insert);
        editor_refresh(prompt, buf, *len, *cur);
        free_candidates(cands, n);
        return;
    }

    int cpl = common_prefix_len(cands, n);
    if (cpl > (int)strlen(prefix)) {
        char insert[MAX_LINE];
        snprintf(insert, sizeof(insert), "%.*s", cpl, cands[0]);
        replace_range(buf, len, cur, max, start, *cur, insert);
        editor_refresh(prompt, buf, *len, *cur);
    } else {
        editor_write("\r\n");
        for (int i = 0; i < n; i++) {
            editor_write(cands[i]);
            editor_write("  ");
            if ((i + 1) % 6 == 0) editor_write("\r\n");
        }
        editor_write("\r\n");
        editor_refresh(prompt, buf, *len, *cur);
    }

    free_candidates(cands, n);
}

static int read_line_editor(const char *prompt, char *out, int outsz) {
    if (!isatty(STDIN_FILENO) || !g_raw_enabled) {
        if (!fgets(out, outsz, stdin)) return -1;
        size_t l = strlen(out);
        if (l > 0 && out[l - 1] == '\n') out[l - 1] = '\0';
        return 0;
    }

    char buf[MAX_LINE];
    int len = 0;
    int cur = 0;
    int hist_index = hist_count;
    buf[0] = '\0';

    editor_write(prompt);

    while (1) {
        editor_key_t k = editor_read_key();
        if (k == KEY_NULL) continue;

        if (k == KEY_ENTER) {
            editor_write("\r\n");
            buf[len] = '\0';
            strlcpy_local(out, buf, (size_t)outsz);
            return 0;
        }

        if (k == KEY_CTRL_C) {
            len = cur = 0;
            buf[0] = '\0';
            editor_write("^C\r\n");
            editor_write(prompt);
            hist_index = hist_count;
            continue;
        }

        if (k == KEY_CTRL_D) {
            if (len == 0) {
                editor_write("\r\n");
                return -1;
            }
            if (cur < len) {
                memmove(buf + cur, buf + cur + 1, (size_t)(len - cur));
                len--;
                editor_refresh(prompt, buf, len, cur);
            }
            continue;
        }

        if (k == KEY_CTRL_R) {
            editor_ctrl_r_search(prompt, buf, &len, &cur, MAX_LINE);
            hist_index = hist_count;
            continue;
        }

        if (k == KEY_TAB) {
            editor_tab_complete(prompt, buf, &len, &cur, MAX_LINE);
            continue;
        }

        if (k == KEY_BACKSPACE) {
            if (cur > 0) {
                memmove(buf + cur - 1, buf + cur, (size_t)(len - cur + 1));
                cur--;
                len--;
                editor_refresh(prompt, buf, len, cur);
            }
            continue;
        }

        if (k == KEY_DELETE) {
            if (cur < len) {
                memmove(buf + cur, buf + cur + 1, (size_t)(len - cur));
                len--;
                editor_refresh(prompt, buf, len, cur);
            }
            continue;
        }

        if (k == KEY_LEFT || k == KEY_CTRL_B) {
            if (cur > 0) {
                cur--;
                editor_refresh(prompt, buf, len, cur);
            }
            continue;
        }

        if (k == KEY_RIGHT || k == KEY_CTRL_F) {
            if (cur < len) {
                cur++;
                editor_refresh(prompt, buf, len, cur);
            }
            continue;
        }

        if (k == KEY_HOME || k == KEY_CTRL_A) {
            cur = 0;
            editor_refresh(prompt, buf, len, cur);
            continue;
        }

        if (k == KEY_END || k == KEY_CTRL_E) {
            cur = len;
            editor_refresh(prompt, buf, len, cur);
            continue;
        }

        if (k == KEY_UP) {
            if (hist_count > 0 && hist_index > 0) {
                hist_index--;
                strlcpy_local(buf, history[hist_index], sizeof(buf));
                len = (int)strlen(buf);
                cur = len;
                editor_refresh(prompt, buf, len, cur);
            }
            continue;
        }

        if (k == KEY_DOWN) {
            if (hist_count == 0) continue;
            if (hist_index < hist_count - 1) {
                hist_index++;
                strlcpy_local(buf, history[hist_index], sizeof(buf));
                len = (int)strlen(buf);
                cur = len;
            } else {
                hist_index = hist_count;
                len = cur = 0;
                buf[0] = '\0';
            }
            editor_refresh(prompt, buf, len, cur);
            continue;
        }

        if ((int)k >= 32 && (int)k < 127) {
            char ch[2];
            ch[0] = (char)k;
            ch[1] = '\0';
            insert_text(buf, &len, &cur, MAX_LINE, ch);
            editor_refresh(prompt, buf, len, cur);
        }
    }
}

/* -------------------------------------------------------------------------- */
/* Variable expansion helper                                                  */
/* -------------------------------------------------------------------------- */

static void expand_cmd_sub(const char **pp, char *buf, int *len, int max_len) {
    const char *p = *pp;
    p += 2; /* skip '$(' */

    /* find matching close paren, respecting nesting */
    int depth = 1;
    const char *start = p;
    while (*p && depth > 0) {
        if (*p == '(' && *(p - 1) == '$') depth++;
        else if (*p == '(') depth++;
        else if (*p == ')') depth--;
        if (depth > 0) p++;
    }

    char cmd[MAX_LINE];
    int cmdlen = (int)(p - start);
    if (cmdlen >= MAX_LINE) cmdlen = MAX_LINE - 1;
    memcpy(cmd, start, (size_t)cmdlen);
    cmd[cmdlen] = '\0';

    if (*p == ')') p++;
    *pp = p;

    int pipefd[2];
    if (pipe(pipefd) < 0) return;

    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }

    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
        _exit(127);
    }

    close(pipefd[1]);
    char rbuf[MAX_LINE];
    ssize_t total = 0;
    ssize_t n;
    while ((n = read(pipefd[0], rbuf + total, (size_t)(MAX_LINE - 1 - total))) > 0) {
        total += n;
        if (total >= MAX_LINE - 1) break;
    }
    close(pipefd[0]);
    rbuf[total] = '\0';

    /* strip trailing newlines */
    while (total > 0 && (rbuf[total - 1] == '\n' || rbuf[total - 1] == '\r'))
        rbuf[--total] = '\0';

    int status;
    waitpid(pid, &status, 0);

    for (int i = 0; i < total && *len < max_len - 1; i++)
        buf[(*len)++] = rbuf[i];
}

static void expand_var(const char **pp, char *buf, int *len, int max_len) {
    const char *p = *pp;

    /* command substitution $(...) */
    if (*(p + 1) == '(') {
        expand_cmd_sub(pp, buf, len, max_len);
        return;
    }

    p++; /* skip '$' */

    char var[256];
    int vl = 0;
    if (*p == '{') {
        p++;
        while (*p && *p != '}') {
            if (vl < (int)sizeof(var) - 1) var[vl++] = *p;
            p++;
        }
        if (*p == '}') p++;
    } else {
        if (*p == '?') {
            if (vl < (int)sizeof(var) - 1) var[vl++] = *p;
            p++;
        } else {
            while (*p && (isalnum((unsigned char)*p) || *p == '_')) {
                if (vl < (int)sizeof(var) - 1) var[vl++] = *p;
                p++;
            }
        }
    }
    var[vl] = '\0';

    if (vl == 0) {
        if (*len < max_len - 1) buf[(*len)++] = '$';
        *pp = p;
        return;
    }

    if (strcmp(var, "?") == 0) {
        char status_buf[32];
        snprintf(status_buf, sizeof(status_buf), "%d", last_exit_status);
        for (const char *s = status_buf; *s && *len < max_len - 1; s++) {
            buf[(*len)++] = *s;
        }
        *pp = p;
        return;
    }

    const char *val = getenv(var);
    if (val) {
        for (const char *s = val; *s && *len < max_len - 1; s++) {
            buf[(*len)++] = *s;
        }
    }

    *pp = p;
}

/* -------------------------------------------------------------------------- */
/* Tokenizer                                                                  */
/* -------------------------------------------------------------------------- */

static int tokenize(const char *line, token_t *tokens, int max) {
    int n = 0;
    const char *p = line;

    while (*p && n < max - 1) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p || *p == '\n') break;

        if (*p == '|' && *(p + 1) == '|') {
            tokens[n++] = (token_t){TOK_OR, NULL};
            p += 2;
            continue;
        }
        if (*p == '&' && *(p + 1) == '&') {
            tokens[n++] = (token_t){TOK_AND, NULL};
            p += 2;
            continue;
        }
        if (*p == '|') {
            tokens[n++] = (token_t){TOK_PIPE, NULL};
            p++;
            continue;
        }
        if (*p == '&') {
            tokens[n++] = (token_t){TOK_BG, NULL};
            p++;
            continue;
        }
        if (*p == '>' && *(p + 1) == '>') {
            tokens[n++] = (token_t){TOK_REDIR_APPEND, NULL};
            p += 2;
            continue;
        }
        if (*p == '>') {
            tokens[n++] = (token_t){TOK_REDIR_OUT, NULL};
            p++;
            continue;
        }
        if (*p == '<') {
            tokens[n++] = (token_t){TOK_REDIR_IN, NULL};
            p++;
            continue;
        }

        char buf[MAX_LINE];
        int len = 0;

        while (*p && *p != ' ' && *p != '\t' && *p != '\n' &&
               *p != '|' && *p != '&' && *p != '>' && *p != '<') {
            if (*p == '\'') {
                p++;
                while (*p && *p != '\'') {
                    if (len < MAX_LINE - 1) buf[len++] = *p;
                    p++;
                }
                if (*p == '\'') {
                    p++;
                } else {
                    fprintf(stderr, "parse error: unclosed quote\n");
                    return -1;
                }
            } else if (*p == '"') {
                p++;
                while (*p && *p != '"') {
                    if (*p == '$') {
                        expand_var(&p, buf, &len, MAX_LINE);
                    } else {
                        if (len < MAX_LINE - 1) buf[len++] = *p;
                        p++;
                    }
                }
                if (*p == '"') {
                    p++;
                } else {
                    fprintf(stderr, "parse error: unclosed quote\n");
                    return -1;
                }
            } else if (*p == '$') {
                expand_var(&p, buf, &len, MAX_LINE);
            } else {
                if (len < MAX_LINE - 1) buf[len++] = *p;
                p++;
            }
        }

        buf[len] = '\0';

        /* tilde expansion: ~/... -> $HOME/... */
        if (buf[0] == '~' && (buf[1] == '/' || buf[1] == '\0')) {
            const char *home = getenv("HOME");
            if (home) {
                char expanded[MAX_LINE];
                snprintf(expanded, sizeof(expanded), "%s%s", home, buf + 1);
                tokens[n].type = TOK_WORD;
                tokens[n].value = strdup(expanded);
            } else {
                tokens[n].type = TOK_WORD;
                tokens[n].value = strdup(buf);
            }
        } else {
            tokens[n].type = TOK_WORD;
            tokens[n].value = strdup(buf);
        }
        if (!tokens[n].value) {
            fprintf(stderr, "shell: out of memory\n");
            return -1;
        }
        n++;
    }

    return n;
}

static int has_glob_meta(const char *s) {
    for (; *s; s++) {
        if (*s == '*' || *s == '?' || *s == '[') return 1;
    }
    return 0;
}

static int expand_globs(token_t *tokens, int *ntok, int max) {
    int n = *ntok;

    for (int i = 0; i < n; i++) {
        if (tokens[i].type != TOK_WORD || !tokens[i].value) continue;
        if (!has_glob_meta(tokens[i].value)) continue;

        glob_t g;
        memset(&g, 0, sizeof(g));
        int gr = glob(tokens[i].value, GLOB_NOCHECK | GLOB_TILDE, NULL, &g);
        if (gr == GLOB_NOMATCH) {
            globfree(&g);
            continue;
        }
        if (gr != 0) {
            fprintf(stderr, "shell: glob failed: %s\n", tokens[i].value);
            globfree(&g);
            return -1;
        }

        if (g.gl_pathc == 0 ||
            (g.gl_pathc == 1 && strcmp(g.gl_pathv[0], tokens[i].value) == 0)) {
            globfree(&g);
            continue;
        }

        if (n - 1 + (int)g.gl_pathc > max) {
            fprintf(stderr, "parse error: too many tokens\n");
            globfree(&g);
            return -1;
        }

        char *expanded[MAX_TOKENS];
        for (size_t j = 0; j < g.gl_pathc; j++) {
            expanded[j] = strdup(g.gl_pathv[j]);
            if (!expanded[j]) {
                for (size_t k = 0; k < j; k++) free(expanded[k]);
                fprintf(stderr, "shell: out of memory\n");
                globfree(&g);
                return -1;
            }
        }

        int add = (int)g.gl_pathc - 1;
        if (add > 0) {
            memmove(&tokens[i + 1 + add], &tokens[i + 1],
                    (size_t)(n - (i + 1)) * sizeof(token_t));
        }

        free(tokens[i].value);
        for (size_t j = 0; j < g.gl_pathc; j++) {
            tokens[i + (int)j].type = TOK_WORD;
            tokens[i + (int)j].value = expanded[j];
        }

        n += add;
        i += add;
        globfree(&g);
    }

    *ntok = n;
    return 0;
}

static void free_tokens(token_t *tokens, int ntok) {
    for (int i = 0; i < ntok; i++) {
        if (tokens[i].type == TOK_WORD) {
            free(tokens[i].value);
            tokens[i].value = NULL;
        }
    }
}

/* -------------------------------------------------------------------------- */
/* Pipeline builder                                                           */
/* -------------------------------------------------------------------------- */

static void clear_command(command_t *cmd) {
    memset(cmd, 0, sizeof(*cmd));
}

static int build_pipeline(token_t *tokens, int ntok, pipeline_t *pl) {
    memset(pl, 0, sizeof(*pl));

    pl->ncmds = 1;
    command_t *cmd = &pl->cmds[0];
    clear_command(cmd);

    for (int i = 0; i < ntok; i++) {
        switch (tokens[i].type) {
        case TOK_PIPE:
            if (cmd->argc == 0) {
                fprintf(stderr, "parse error: empty pipe segment\n");
                return -1;
            }
            cmd->argv[cmd->argc] = NULL;
            if (pl->ncmds >= MAX_TOKENS) {
                fprintf(stderr, "parse error: too many pipeline segments\n");
                return -1;
            }
            pl->ncmds++;
            cmd = &pl->cmds[pl->ncmds - 1];
            clear_command(cmd);
            break;

        case TOK_REDIR_OUT:
        case TOK_REDIR_APPEND:
            if (i + 1 >= ntok || tokens[i + 1].type != TOK_WORD) {
                fprintf(stderr, "parse error: redirect missing filename\n");
                return -1;
            }
            cmd->redir_out = tokens[++i].value;
            cmd->append = (tokens[i - 1].type == TOK_REDIR_APPEND);
            cmd->owned_redir_out = 0;
            break;

        case TOK_REDIR_IN:
            if (i + 1 >= ntok || tokens[i + 1].type != TOK_WORD) {
                fprintf(stderr, "parse error: redirect missing filename\n");
                return -1;
            }
            cmd->redir_in = tokens[++i].value;
            cmd->owned_redir_in = 0;
            break;

        case TOK_BG:
            pl->bg = 1;
            break;

        case TOK_AND:
        case TOK_OR:
            fprintf(stderr, "parse error: unexpected logical operator\n");
            return -1;

        case TOK_WORD:
            if (cmd->argc >= MAX_TOKENS - 1) {
                fprintf(stderr, "parse error: too many argv tokens\n");
                return -1;
            }
            cmd->argv[cmd->argc] = tokens[i].value;
            cmd->owned_argv[cmd->argc] = 0;
            cmd->argc++;
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

static void free_pipeline_owned(pipeline_t *pl) {
    for (int i = 0; i < pl->ncmds; i++) {
        command_t *c = &pl->cmds[i];
        for (int j = 0; j < c->argc; j++) {
            if (c->owned_argv[j]) {
                free(c->argv[j]);
                c->argv[j] = NULL;
                c->owned_argv[j] = 0;
            }
        }
        if (c->owned_redir_in) {
            free(c->redir_in);
            c->redir_in = NULL;
            c->owned_redir_in = 0;
        }
        if (c->owned_redir_out) {
            free(c->redir_out);
            c->redir_out = NULL;
            c->owned_redir_out = 0;
        }
    }
}

/* -------------------------------------------------------------------------- */
/* Alias expansion                                                            */
/* -------------------------------------------------------------------------- */

static int command_replace_argv(command_t *cmd, char **newv, int newc) {
    if (newc >= MAX_TOKENS) return -1;

    for (int i = 0; i < cmd->argc; i++) {
        if (cmd->owned_argv[i]) {
            free(cmd->argv[i]);
            cmd->owned_argv[i] = 0;
        }
    }

    for (int i = 0; i < newc; i++) {
        cmd->argv[i] = newv[i];
        cmd->owned_argv[i] = 1;
    }

    cmd->argc = newc;
    cmd->argv[newc] = NULL;
    return 0;
}

static int alias_expand_command(command_t *cmd) {
    if (cmd->argc <= 0 || !cmd->argv[0]) return 0;

    char *seen[MAX_ALIAS_DEPTH];
    int seen_n = 0;

    while (seen_n < MAX_ALIAS_DEPTH) {
        alias_t *a = alias_find(cmd->argv[0]);
        if (!a) return 0;

        int loop = 0;
        for (int i = 0; i < seen_n; i++) {
            if (strcmp(seen[i], a->name) == 0) {
                loop = 1;
                break;
            }
        }
        if (loop) {
            fprintf(stderr, "alias: recursive alias detected: %s\n", a->name);
            return -1;
        }
        seen[seen_n++] = a->name;

        token_t atoks[MAX_TOKENS];
        int an = tokenize(a->value, atoks, MAX_TOKENS);
        if (an < 0) return -1;

        char *newv[MAX_TOKENS];
        int newc = 0;
        int ok = 1;

        for (int i = 0; i < an; i++) {
            if (atoks[i].type != TOK_WORD) {
                fprintf(stderr, "alias: metacharacters not supported in alias value: %s\n", a->name);
                ok = 0;
                break;
            }
            if (newc >= MAX_TOKENS - 1) {
                ok = 0;
                break;
            }
            newv[newc] = strdup(atoks[i].value);
            if (!newv[newc]) {
                ok = 0;
                break;
            }
            newc++;
        }

        if (ok) {
            for (int i = 1; i < cmd->argc; i++) {
                if (newc >= MAX_TOKENS - 1) {
                    ok = 0;
                    break;
                }
                newv[newc] = strdup(cmd->argv[i]);
                if (!newv[newc]) {
                    ok = 0;
                    break;
                }
                newc++;
            }
        }

        free_tokens(atoks, an);

        if (!ok || newc <= 0) {
            for (int i = 0; i < newc; i++) free(newv[i]);
            fprintf(stderr, "alias: expansion failed for %s\n", a->name);
            return -1;
        }

        if (command_replace_argv(cmd, newv, newc) < 0) {
            for (int i = 0; i < newc; i++) free(newv[i]);
            return -1;
        }
    }

    fprintf(stderr, "alias: expansion depth exceeded for %s\n", cmd->argv[0]);
    return -1;
}

static int alias_expand_pipeline(pipeline_t *pl) {
    for (int i = 0; i < pl->ncmds; i++) {
        if (alias_expand_command(&pl->cmds[i]) < 0) return -1;
    }
    return 0;
}

/* -------------------------------------------------------------------------- */
/* Builtins                                                                   */
/* -------------------------------------------------------------------------- */

static int builtin_cd(command_t *cmd) {
    const char *path = cmd->argc > 1 ? cmd->argv[1] : getenv("HOME");
    if (!path) path = "/";

    if (path[0] == '~') {
        const char *home = getenv("HOME");
        if (home) {
            char expanded[PATH_MAX];
            snprintf(expanded, sizeof(expanded), "%s%s", home, path + 1);
            if (chdir(expanded) != 0) {
                fprintf(stderr, "cd: %s: %s\n", expanded, strerror(errno));
            }
            return 1;
        }
    }

    if (chdir(path) != 0) {
        fprintf(stderr, "cd: %s: %s\n", path, strerror(errno));
    }
    return 1;
}

static int builtin_exit(command_t *cmd) {
    int code = (cmd->argc > 1) ? atoi(cmd->argv[1]) : 0;
    disable_raw_mode();
    hist_free_all();
    alias_free_all();
    exit(code);
}

static int builtin_export(command_t *cmd) {
    for (int i = 1; i < cmd->argc; i++) {
        char *eq = strchr(cmd->argv[i], '=');
        if (!eq) continue;
        *eq = '\0';
        setenv(cmd->argv[i], eq + 1, 1);
        *eq = '=';
    }
    return 1;
}

static int builtin_history(command_t *cmd) {
    (void)cmd;
    for (int i = 0; i < hist_count; i++) {
        printf("%4d  %s\n", i + 1, history[i]);
    }
    return 1;
}

static int builtin_fg(command_t *cmd) {
    int target = -1;
    if (cmd->argc > 1) target = atoi(cmd->argv[1]);

    for (int i = MAX_JOBS - 1; i >= 0; i--) {
        if (!jobs[i].active) continue;
        if (target > 0 && jobs[i].id != target) continue;

        fprintf(stderr, "[%d] fg  %s\n", jobs[i].id, jobs[i].cmd);
        kill(jobs[i].pid, SIGCONT);

        int status;
        if (waitpid(jobs[i].pid, &status, 0) < 0) {
            perror("fg waitpid");
        }
        jobs[i].active = 0;
        return 1;
    }

    fprintf(stderr, "fg: no such job\n");
    return 1;
}

static int builtin_bg(command_t *cmd) {
    int target = -1;
    if (cmd->argc > 1) target = atoi(cmd->argv[1]);

    int resumed = 0;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].active) continue;
        if (target > 0 && jobs[i].id != target) continue;
        kill(jobs[i].pid, SIGCONT);
        fprintf(stderr, "[%d] running  %s\n", jobs[i].id, jobs[i].cmd);
        resumed = 1;
        if (target > 0) break;
    }

    if (!resumed && target > 0) fprintf(stderr, "bg: no such job\n");
    return 1;
}

static int valid_alias_name(const char *s) {
    if (!s || !*s) return 0;
    if (!(isalpha((unsigned char)s[0]) || s[0] == '_')) return 0;
    for (const char *p = s + 1; *p; p++) {
        if (!(isalnum((unsigned char)*p) || *p == '_')) return 0;
    }
    return 1;
}

static int builtin_alias(command_t *cmd) {
    if (cmd->argc == 1) {
        alias_list_all();
        return 1;
    }

    for (int i = 1; i < cmd->argc; i++) {
        char *arg = cmd->argv[i];
        char *eq = strchr(arg, '=');

        if (!eq) {
            alias_t *a = alias_find(arg);
            if (a) {
                alias_print_one(a);
            } else {
                fprintf(stderr, "alias: %s: not found\n", arg);
            }
            continue;
        }

        *eq = '\0';
        const char *name = arg;
        const char *value = eq + 1;

        if (!valid_alias_name(name)) {
            fprintf(stderr, "alias: invalid name: %s\n", name);
            *eq = '=';
            continue;
        }

        if (alias_set(name, value) != 0) {
            fprintf(stderr, "alias: failed to set %s\n", name);
        }
        *eq = '=';
    }

    return 1;
}

static int builtin_source(command_t *cmd);

static int builtin_uptime(command_t *cmd) {
    (void)cmd;
    FILE *fp = popen("uptime", "r");
    if (!fp) { perror("uptime"); return 1; }
    char buf[256];
    while (fgets(buf, sizeof(buf), fp)) fputs(buf, stdout);
    pclose(fp);
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
    if (strcmp(cmd->argv[0], "alias") == 0) return builtin_alias(cmd);
    if (strcmp(cmd->argv[0], "uptime") == 0) return builtin_uptime(cmd);
    if (strcmp(cmd->argv[0], "source") == 0 || strcmp(cmd->argv[0], ".") == 0) return builtin_source(cmd);
    return 0;
}

/* -------------------------------------------------------------------------- */
/* Redirect setup (in child)                                                  */
/* -------------------------------------------------------------------------- */

static void setup_redirects(command_t *cmd) {
    if (cmd->redir_in) {
        int fd = open(cmd->redir_in, O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "%s: %s\n", cmd->redir_in, strerror(errno));
            _exit(1);
        }
        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("dup2");
            close(fd);
            _exit(1);
        }
        close(fd);
    }

    if (cmd->redir_out) {
        int flags = O_WRONLY | O_CREAT | (cmd->append ? O_APPEND : O_TRUNC);
        int fd = open(cmd->redir_out, flags, 0644);
        if (fd < 0) {
            fprintf(stderr, "%s: %s\n", cmd->redir_out, strerror(errno));
            _exit(1);
        }
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2");
            close(fd);
            _exit(1);
        }
        close(fd);
    }
}

/* -------------------------------------------------------------------------- */
/* Executor                                                                   */
/* -------------------------------------------------------------------------- */

static void add_job(pid_t pid, const char *line) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].active) {
            jobs[i].pid = pid;
            jobs[i].id = next_job_id++;
            strlcpy_local(jobs[i].cmd, line, sizeof(jobs[i].cmd));
            jobs[i].active = 1;
            fprintf(stderr, "[%d] %d\n", jobs[i].id, (int)pid);
            return;
        }
    }
    fprintf(stderr, "shell: too many background jobs\n");
}

static int wait_pipeline_children(pid_t *pids, int n) {
    if (n <= 0) return 0;
    int status;
    for (int i = 0; i < n; i++) {
        if (pids[i] > 0) {
            while (waitpid(pids[i], &status, 0) < 0) {
                if (errno == EINTR) continue;
                return 1;
            }
            if (i == n - 1) {
                if (WIFEXITED(status)) return WEXITSTATUS(status);
                return 1;
            }
        }
    }
    return 0;
}

static void exec_pipeline(pipeline_t *pl, const char *line) {
    int n = pl->ncmds;

    if (n == 1 && !pl->bg && try_builtin(&pl->cmds[0])) {
        last_exit_status = 0;
        return;
    }

    int prev_fd = -1;
    pid_t last_pid = -1;
    pid_t pids[MAX_TOKENS];
    int pcount = 0;

    for (int i = 0; i < n; i++) {
        int pipefd[2] = {-1, -1};
        if (i < n - 1) {
            if (pipe(pipefd) < 0) {
                perror("pipe");
                if (prev_fd >= 0) close(prev_fd);
                return;
            }
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            if (prev_fd >= 0) close(prev_fd);
            if (pipefd[0] >= 0) close(pipefd[0]);
            if (pipefd[1] >= 0) close(pipefd[1]);
            return;
        }

        if (pid == 0) {
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);

            if (prev_fd >= 0) {
                if (dup2(prev_fd, STDIN_FILENO) < 0) _exit(1);
            }
            if (pipefd[1] >= 0) {
                if (dup2(pipefd[1], STDOUT_FILENO) < 0) _exit(1);
            }

            if (prev_fd >= 0) close(prev_fd);
            if (pipefd[0] >= 0) close(pipefd[0]);
            if (pipefd[1] >= 0) close(pipefd[1]);

            setup_redirects(&pl->cmds[i]);

            execvp(pl->cmds[i].argv[0], pl->cmds[i].argv);
            fprintf(stderr, "%s: command not found\n", pl->cmds[i].argv[0]);
            _exit(127);
        }

        if (prev_fd >= 0) close(prev_fd);
        if (pipefd[1] >= 0) close(pipefd[1]);
        prev_fd = pipefd[0];

        last_pid = pid;
        pids[pcount++] = pid;
    }

    if (prev_fd >= 0) close(prev_fd);

    if (pl->bg) {
        add_job(last_pid, line);
    } else {
        last_exit_status = wait_pipeline_children(pids, pcount);
    }
}

static int exec_command_list(token_t *tokens, int ntok, const char *line) {
    if (tokens[0].type == TOK_AND || tokens[0].type == TOK_OR ||
        tokens[ntok - 1].type == TOK_AND || tokens[ntok - 1].type == TOK_OR) {
        fprintf(stderr, "parse error: unexpected logical operator\n");
        return -1;
    }

    for (int i = 1; i < ntok; i++) {
        int cur_sep = (tokens[i].type == TOK_AND || tokens[i].type == TOK_OR);
        int prev_sep = (tokens[i - 1].type == TOK_AND || tokens[i - 1].type == TOK_OR);
        if (cur_sep && prev_sep) {
            fprintf(stderr, "parse error: unexpected logical operator\n");
            return -1;
        }
    }

    int seg_start = 0;
    token_type_t prev_sep = TOK_WORD;

    for (int i = 0; i <= ntok; i++) {
        int is_sep = (i < ntok) && (tokens[i].type == TOK_AND || tokens[i].type == TOK_OR);
        if (!is_sep && i != ntok) continue;

        int should_exec = 1;
        if (seg_start > 0) {
            if (prev_sep == TOK_AND) {
                should_exec = (last_exit_status == 0);
            } else if (prev_sep == TOK_OR) {
                should_exec = (last_exit_status != 0);
            }
        }

        if (should_exec) {
            pipeline_t pl;
            int seg_ntok = i - seg_start;
            if (build_pipeline(tokens + seg_start, seg_ntok, &pl) < 0) {
                return -1;
            }

            if (pl.cmds[0].argc > 0 && alias_expand_pipeline(&pl) == 0) {
                exec_pipeline(&pl, line);
            }
            free_pipeline_owned(&pl);
        }

        if (i == ntok) {
            break;
        }

        prev_sep = tokens[i].type;
        seg_start = i + 1;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
/* Prompt + git branch                                                        */
/* -------------------------------------------------------------------------- */

static int path_join(char *out, size_t outsz, const char *a, const char *b) {
    if (!a || !b || !out || outsz == 0) return -1;
    if (snprintf(out, outsz, "%s/%s", a, b) >= (int)outsz) return -1;
    return 0;
}

static int read_first_line(const char *path, char *out, size_t outsz) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    if (!fgets(out, (int)outsz, f)) {
        fclose(f);
        return -1;
    }
    fclose(f);
    size_t l = strlen(out);
    while (l > 0 && (out[l - 1] == '\n' || out[l - 1] == '\r')) out[--l] = '\0';
    return 0;
}

static int file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

static int dir_exists(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISDIR(st.st_mode);
}

static int resolve_gitdir_from_file(const char *base_dir, const char *gitfile, char *gitdir, size_t gsz) {
    char line[PATH_MAX];
    if (read_first_line(gitfile, line, sizeof(line)) != 0) return -1;

    const char *prefix = "gitdir:";
    size_t pl = strlen(prefix);
    if (strncmp(line, prefix, pl) != 0) return -1;

    const char *p = line + pl;
    while (*p == ' ' || *p == '\t') p++;
    if (!*p) return -1;

    if (*p == '/') {
        if (snprintf(gitdir, gsz, "%s", p) >= (int)gsz) return -1;
    } else {
        if (snprintf(gitdir, gsz, "%s/%s", base_dir, p) >= (int)gsz) return -1;
    }

    return dir_exists(gitdir) ? 0 : -1;
}

static int find_git_dir(char *gitdir, size_t gsz) {
    char cur[PATH_MAX];
    if (!getcwd(cur, sizeof(cur))) return -1;

    while (1) {
        char dotgit[PATH_MAX];
        if (path_join(dotgit, sizeof(dotgit), cur, ".git") == 0) {
            if (dir_exists(dotgit)) {
                strlcpy_local(gitdir, dotgit, gsz);
                return 0;
            }
            if (file_exists(dotgit)) {
                if (resolve_gitdir_from_file(cur, dotgit, gitdir, gsz) == 0) return 0;
            }
        }

        if (strcmp(cur, "/") == 0) break;
        char *slash = strrchr(cur, '/');
        if (!slash) break;
        if (slash == cur) {
            cur[1] = '\0';
        } else {
            *slash = '\0';
        }
    }

    return -1;
}

static int get_git_branch(char *branch, size_t bsz) {
    char gitdir[PATH_MAX];
    if (find_git_dir(gitdir, sizeof(gitdir)) != 0) return -1;

    char headpath[PATH_MAX];
    if (path_join(headpath, sizeof(headpath), gitdir, "HEAD") != 0) return -1;

    char head[PATH_MAX];
    if (read_first_line(headpath, head, sizeof(head)) != 0) return -1;

    const char *ref_prefix = "ref: ";
    if (strncmp(head, ref_prefix, strlen(ref_prefix)) == 0) {
        const char *ref = head + strlen(ref_prefix);
        const char *last = strrchr(ref, '/');
        const char *name = last ? last + 1 : ref;
        strlcpy_local(branch, name, bsz);
        return 0;
    }

    if (strlen(head) >= 7) {
        char short_hash[8];
        memcpy(short_hash, head, 7);
        short_hash[7] = '\0';
        snprintf(branch, bsz, "detached:%s", short_hash);
    } else {
        strlcpy_local(branch, "detached", bsz);
    }

    return 0;
}

static void build_prompt(char *out, size_t outsz) {
    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd))) strlcpy_local(cwd, "?", sizeof(cwd));

    char shown[PATH_MAX];
    const char *home = getenv("HOME");
    if (home) {
        size_t hl = strlen(home);
        if (strncmp(cwd, home, hl) == 0) {
            snprintf(shown, sizeof(shown), "~%s", cwd + hl);
        } else {
            strlcpy_local(shown, cwd, sizeof(shown));
        }
    } else {
        strlcpy_local(shown, cwd, sizeof(shown));
    }

    char branch[256];
    if (get_git_branch(branch, sizeof(branch)) == 0) {
        snprintf(out, outsz, "%s (%s) $ ", shown, branch);
    } else {
        snprintf(out, outsz, "%s $ ", shown);
    }
}

/* -------------------------------------------------------------------------- */
/* Eval + source                                                              */
/* -------------------------------------------------------------------------- */

static void eval_line(const char *line) {
    if (is_blank_line(line)) return;

    char buf[MAX_LINE];
    strlcpy_local(buf, line, sizeof(buf));

    /* strip comments */
    int in_sq = 0, in_dq = 0;
    for (char *c = buf; *c; c++) {
        if (*c == '\'' && !in_dq) in_sq = !in_sq;
        else if (*c == '"' && !in_sq) in_dq = !in_dq;
        else if (*c == '#' && !in_sq && !in_dq) { *c = '\0'; break; }
    }

    if (is_blank_line(buf)) return;

    token_t tokens[MAX_TOKENS];
    int ntok = tokenize(buf, tokens, MAX_TOKENS);
    if (ntok < 0) {
        free_tokens(tokens, ntok > 0 ? ntok : 0);
        return;
    }
    if (ntok == 0) return;

    if (expand_globs(tokens, &ntok, MAX_TOKENS) < 0) {
        free_tokens(tokens, ntok);
        return;
    }

    exec_command_list(tokens, ntok, buf);
    free_tokens(tokens, ntok);
}

static int source_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "source: %s: %s\n", path, strerror(errno));
        return -1;
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        size_t l = strlen(line);
        if (l > 0 && line[l - 1] == '\n') line[l - 1] = '\0';
        eval_line(line);
    }

    fclose(f);
    return 0;
}

static int builtin_source(command_t *cmd) {
    if (cmd->argc < 2) {
        fprintf(stderr, "source: filename argument required\n");
        return 1;
    }
    source_file(cmd->argv[1]);
    return 1;
}

/* -------------------------------------------------------------------------- */
/* Register source builtin                                                    */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/* Main loop                                                                  */
/* -------------------------------------------------------------------------- */

static void load_shellrc(void) {
    const char *home = getenv("HOME");
    if (!home) return;
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/.shellrc", home);
    struct stat st;
    if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
        source_file(path);
    }
}

int main(int argc, char **argv) {
    setup_signals();

    /* script mode: shell script.sh */
    if (argc > 1) {
        source_file(argv[1]);
        alias_free_all();
        hist_free_all();
        return last_exit_status;
    }

    enable_raw_mode();
    load_shellrc();

    while (1) {
        if (got_sigchld) reap_bg();
        else reap_bg();

        char prompt[MAX_LINE];
        build_prompt(prompt, sizeof(prompt));

        char line[MAX_LINE];
        if (read_line_editor(prompt, line, sizeof(line)) < 0) {
            break;
        }

        if (is_blank_line(line)) {
            continue;
        }

        hist_add(line);
        eval_line(line);
    }

    disable_raw_mode();
    alias_free_all();
    hist_free_all();
    return 0;
}
