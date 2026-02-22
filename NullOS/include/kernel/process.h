/* NullOS Process Management Header */

#ifndef _KERNEL_PROCESS_H
#define _KERNEL_PROCESS_H

#include <lib/stdint.h>

typedef enum {
    PROCESS_RUNNING,
    PROCESS_READY,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} process_state_t;

typedef struct process {
    uint32_t pid;
    uint32_t esp;
    uint32_t ebp;
    uint32_t eip;
    uint32_t *page_directory;
    process_state_t state;
    uint32_t kernel_stack;
    struct process *next;
} process_t;

process_t *process_create(void (*entry)(void));
void       process_exit(void);
process_t *process_get_current(void);

#endif /* _KERNEL_PROCESS_H */
