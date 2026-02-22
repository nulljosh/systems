/* NullOS Keyboard Header */

#ifndef _KERNEL_KEYBOARD_H
#define _KERNEL_KEYBOARD_H

#include <lib/stdint.h>

void keyboard_init(void);
char keyboard_getchar(void);
int  keyboard_haskey(void);

#endif /* _KERNEL_KEYBOARD_H */
