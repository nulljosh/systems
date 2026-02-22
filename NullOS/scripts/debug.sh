#!/bin/bash
# NullOS Debug Launcher
# Runs QEMU with GDB stub. Connect with:
#   gdb build/kernel/kernel.elf -ex "target remote :1234"

set -e

IMAGE="build/nullos.img"
KERNEL_ELF="build/kernel/kernel.elf"

if [ ! -f "$IMAGE" ]; then
    echo "Error: $IMAGE not found. Run 'make' first."
    exit 1
fi

echo "Launching NullOS in QEMU (debug mode)..."
echo "QEMU is paused, waiting for GDB connection on :1234"
echo ""
echo "In another terminal, run:"
echo "  gdb $KERNEL_ELF -ex 'target remote :1234'"
echo ""
echo "Useful GDB commands:"
echo "  break kmain       - Break at kernel entry"
echo "  continue          - Resume execution"
echo "  info registers    - Show CPU registers"
echo "  x/16x 0x7c00     - Examine boot sector memory"
echo "  layout asm        - Show disassembly view"
echo "---"

qemu-system-i386 \
    -fda "$IMAGE" \
    -boot a \
    -serial stdio \
    -m 32M \
    -s -S
