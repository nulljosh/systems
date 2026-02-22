/* NullOS System Call Header */

#ifndef _KERNEL_SYSCALL_H
#define _KERNEL_SYSCALL_H

#include <lib/stdint.h>

/* Syscall numbers */
#define SYS_EXIT    0
#define SYS_WRITE   1
#define SYS_READ    2
#define SYS_FORK    3
#define SYS_GETPID  4
#define SYS_SLEEP   5

void syscall_init(void);

#endif /* _KERNEL_SYSCALL_H */
