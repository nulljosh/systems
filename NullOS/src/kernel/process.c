/* NullOS Process Management
 *
 * Process creation, destruction, and the Process Control Block (PCB).
 * Each process has its own page directory and kernel stack.
 *
 * See: docs/PHASES.md Phase 4.1
 */

/* TODO: Implement process management
 *
 * Process Control Block:
 *   typedef struct process {
 *       uint32_t pid;
 *       uint32_t esp, ebp, eip;
 *       uint32_t *page_directory;
 *       process_state_t state;
 *       uint32_t kernel_stack;
 *       struct process *next;
 *   } process_t;
 *
 * Functions:
 *   process_create(void (*entry)(void)) - Create new process
 *   process_exit()                      - Terminate current process
 *   process_get_current()               - Return current PCB
 */
