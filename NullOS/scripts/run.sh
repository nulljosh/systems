#!/bin/bash
# NullOS QEMU Launcher
# Runs the OS image in QEMU with serial output to terminal.

set -e

IMAGE="build/nullos.img"

if [ ! -f "$IMAGE" ]; then
    echo "Error: $IMAGE not found. Run 'make' first."
    exit 1
fi

echo "Launching NullOS in QEMU..."
echo "Serial output below. Press Ctrl+A, X to exit QEMU."
echo "---"

qemu-system-i386 \
    -fda "$IMAGE" \
    -boot a \
    -serial stdio \
    -m 32M \
    -display sdl
