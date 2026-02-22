/* NullOS Timer Header */

#ifndef _KERNEL_TIMER_H
#define _KERNEL_TIMER_H

#include <lib/stdint.h>

void     timer_init(uint32_t frequency);
uint32_t timer_get_ticks(void);
void     timer_sleep(uint32_t ms);

#endif /* _KERNEL_TIMER_H */
