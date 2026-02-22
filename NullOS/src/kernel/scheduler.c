/* NullOS Scheduler
 *
 * Preemptive round-robin scheduler. Triggered by the PIT timer
 * interrupt. Time quantum: 20ms (2 ticks at 100 Hz).
 *
 * See: docs/PHASES.md Phase 4.3
 */

/* TODO: Implement scheduler
 *
 * Data:
 *   static process_t *current_process;
 *   static process_t *ready_queue;
 *
 * Functions:
 *   scheduler_init()  - Initialize scheduler, create idle process
 *   schedule()        - Pick next process, context switch
 *
 * Context switch (assembly):
 *   1. Save current process ESP/EBP/EIP
 *   2. Switch to next process's stack
 *   3. Switch page directory (CR3) if needed
 *   4. Restore next process registers
 *   5. Return (resumes in new process)
 */
