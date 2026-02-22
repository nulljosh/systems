/* NullOS System Call Interface
 *
 * Software interrupt 0x80 for user-mode to kernel communication.
 * Arguments passed in registers (Linux convention).
 *
 * See: docs/PHASES.md Phase 4.4
 */

/* TODO: Implement syscalls
 *
 * Convention:
 *   EAX = syscall number
 *   EBX, ECX, EDX, ESI, EDI = arguments
 *   Return value in EAX
 *
 * Syscall table:
 *   0: sys_exit(int status)
 *   1: sys_write(int fd, const char *buf, uint32_t len)
 *   2: sys_read(int fd, char *buf, uint32_t len)
 *   3: sys_fork()
 *   4: sys_getpid()
 *   5: sys_sleep(uint32_t ms)
 *
 * Functions:
 *   syscall_init()                  - Register int 0x80 handler
 *   syscall_handler(registers_t *r) - Dispatch based on EAX
 */
