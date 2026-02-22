/* NullOS PIT Timer Driver
 *
 * Programmable Interval Timer on IRQ 0 (interrupt 32).
 * Configured for the requested frequency (default 100 Hz = 10ms ticks).
 */

#include <kernel/timer.h>
#include <kernel/irq.h>
#include <system.h>

/* PIT ports */
#define PIT_CHANNEL0  0x40
#define PIT_CMD       0x43
/* Command: channel 0, lobyte/hibyte, rate generator mode */
#define PIT_CMD_RATE  0x36

static volatile uint32_t tick_count = 0;
static uint32_t ticks_per_ms = 0;

static void timer_irq(void) {
    tick_count++;
}

void timer_init(uint32_t frequency) {
    /* PIT base frequency: 1,193,180 Hz */
    uint32_t divisor = 1193180 / frequency;

    /* Program PIT channel 0, rate generator */
    outb(PIT_CMD, PIT_CMD_RATE);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)(divisor >> 8));

    /* ticks_per_ms = frequency / 1000; at 100 Hz -> 0.1, so we work in ticks */
    ticks_per_ms = frequency / 1000;
    if (ticks_per_ms == 0) ticks_per_ms = 1;

    irq_install_handler(0, timer_irq);
}

uint32_t timer_get_ticks(void) {
    return tick_count;
}

/* Busy-wait for ms milliseconds.
 * At 100 Hz, 1 tick = 10 ms, so ticks_needed = ms / 10.
 * Minimum resolution is 1 tick. */
void timer_sleep(uint32_t ms) {
    uint32_t ticks_needed = ms / 10;
    if (ticks_needed == 0) ticks_needed = 1;
    uint32_t start = tick_count;
    while ((tick_count - start) < ticks_needed) {
        asm volatile("hlt");
    }
}
