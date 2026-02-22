/* NullOS Scheduler Header */

#ifndef _KERNEL_SCHEDULER_H
#define _KERNEL_SCHEDULER_H

#include <kernel/process.h>

#define TIME_QUANTUM_MS 20  /* 20ms = 2 ticks at 100 Hz */

void scheduler_init(void);
void schedule(void);

#endif /* _KERNEL_SCHEDULER_H */
