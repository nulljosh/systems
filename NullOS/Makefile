# NullOS Makefile
# Builds the kernel ELF and (optionally) a bootable floppy image.
# Primary test path: make run  (uses QEMU multiboot -kernel, no floppy needed)
# Floppy path:       make image (stage1 + stage2 + kernel.bin)

# --- Toolchain ---
ASM       = nasm
CC        = i686-elf-gcc
LD        = i686-elf-ld
OBJCOPY   = i686-elf-objcopy
QEMU      = qemu-system-i386

# --- Directories ---
SRC_BOOT  = src/boot
SRC_KERN  = src/kernel
SRC_DRV   = src/drivers
SRC_LIB   = src/lib
INC       = include
BUILD     = build
BUILD_BOOT = $(BUILD)/boot
BUILD_KERN = $(BUILD)/kernel
BUILD_DRV  = $(BUILD)/drivers
BUILD_LIB  = $(BUILD)/lib

# --- Flags ---
ASMFLAGS     = -f bin
ASMFLAGS_ELF = -f elf32
CFLAGS       = -m32 -ffreestanding -fno-builtin -fno-exceptions -fno-stack-protector \
               -nostdlib -nostdinc -Wall -Wextra -Werror -O2 -I$(INC)
LDFLAGS      = -T linker.ld -nostdlib

# --- Output ---
IMAGE      = $(BUILD)/nullos.img
BOOT_BIN   = $(BUILD_BOOT)/boot.bin
STAGE2_BIN = $(BUILD_BOOT)/stage2.bin
KERNEL_BIN = $(BUILD_KERN)/kernel.bin
KERNEL_ELF = $(BUILD_KERN)/kernel.elf

# --- Source files ---

# Stage 1 bootloader (flat binary, 512 bytes)
BOOT_SRCS = $(SRC_BOOT)/stage1.asm

# Stage 2 bootloader (flat binary, loaded to 0x7E00 by stage1)
STAGE2_SRC = $(SRC_BOOT)/stage2.asm

# Kernel entry ASM (ELF object, multiboot header + _start)
KERN_ENTRY_SRC = $(SRC_KERN)/entry.asm
KERN_ENTRY_OBJ = $(BUILD_KERN)/entry.o

# Kernel C files
KERN_C_SRCS = $(wildcard $(SRC_KERN)/*.c)
KERN_C_OBJS = $(patsubst $(SRC_KERN)/%.c,$(BUILD_KERN)/%.o,$(KERN_C_SRCS))

# Driver C files
DRV_C_SRCS  = $(wildcard $(SRC_DRV)/*.c)
DRV_C_OBJS  = $(patsubst $(SRC_DRV)/%.c,$(BUILD_DRV)/%.o,$(DRV_C_SRCS))

# Lib C files
LIB_C_SRCS  = $(wildcard $(SRC_LIB)/*.c)
LIB_C_OBJS  = $(patsubst $(SRC_LIB)/%.c,$(BUILD_LIB)/%.o,$(LIB_C_SRCS))

# All kernel objects (entry first so multiboot header is within first 8KB)
ALL_OBJS = $(KERN_ENTRY_OBJ) $(KERN_C_OBJS) $(DRV_C_OBJS) $(LIB_C_OBJS)

# --- QEMU flags ---
# Primary: multiboot via -kernel (no floppy needed, fast iteration)
QEMU_FLAGS       = -kernel $(KERNEL_ELF) -serial stdio -m 32M -display none
QEMU_FLAGS_VGA   = -kernel $(KERNEL_ELF) -serial stdio -m 32M
QEMU_DEBUG       = -kernel $(KERNEL_ELF) -serial stdio -m 32M -s -S -display none
# Floppy boot (requires working stage2 with kernel loader)
QEMU_FLOPPY      = -fda $(IMAGE) -boot a -serial stdio -m 32M

# ===========================================================================
# Targets
# ===========================================================================

.PHONY: all run run-vga debug image clean boot kernel dirs

all: $(KERNEL_ELF)

# --- Create build directories ---
dirs:
	@mkdir -p $(BUILD_BOOT) $(BUILD_KERN) $(BUILD_DRV) $(BUILD_LIB)

# --- Kernel ELF (link all objects) ---
$(KERNEL_ELF): $(ALL_OBJS) linker.ld | dirs
	@echo "  LD    $@"
	@$(LD) $(LDFLAGS) -o $@ $(ALL_OBJS)
	@echo ""
	@echo "  Kernel ELF: $@"
	@echo "  Run with: make run"

# --- Kernel entry ASM -> ELF object ---
$(KERN_ENTRY_OBJ): $(KERN_ENTRY_SRC) | dirs
	@echo "  ASM   $<"
	@$(ASM) $(ASMFLAGS_ELF) $< -o $@

# --- Compile kernel C files ---
$(BUILD_KERN)/%.o: $(SRC_KERN)/%.c | dirs
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# --- Compile driver C files ---
$(BUILD_DRV)/%.o: $(SRC_DRV)/%.c | dirs
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# --- Compile lib C files ---
$(BUILD_LIB)/%.o: $(SRC_LIB)/%.c | dirs
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# --- Run in QEMU (no display, serial output to terminal) ---
run: $(KERNEL_ELF)
	@echo "  QEMU  -kernel $(KERNEL_ELF)"
	@$(QEMU) $(QEMU_FLAGS)

# --- Run in QEMU with VGA display ---
run-vga: $(KERNEL_ELF)
	@echo "  QEMU  -kernel $(KERNEL_ELF) (VGA display)"
	@$(QEMU) $(QEMU_FLAGS_VGA)

# --- Run with GDB stub ---
debug: $(KERNEL_ELF)
	@echo "  QEMU  waiting for GDB on :1234"
	@echo "  Connect: gdb $(KERNEL_ELF) -ex 'target remote :1234'"
	@$(QEMU) $(QEMU_DEBUG)

# ---------------------------------------------------------------------------
# Floppy image (for custom bootloader -- requires stage2 kernel loader)
# ---------------------------------------------------------------------------

# Stage 1 (flat binary, 512 bytes)
$(BOOT_BIN): $(BOOT_SRCS) | dirs
	@echo "  ASM   $<"
	@$(ASM) $(ASMFLAGS) $< -o $@

# Stage 2 (flat binary, loaded at 0x7E00)
$(STAGE2_BIN): $(STAGE2_SRC) | dirs
	@echo "  ASM   $<"
	@$(ASM) $(ASMFLAGS) $< -o $@

# Kernel flat binary
$(KERNEL_BIN): $(KERNEL_ELF) | dirs
	@echo "  BIN   $@"
	@$(OBJCOPY) -O binary $< $@

# Full floppy image: stage1 | stage2 (sectors 1-15) | kernel (sector 16+)
image: dirs $(BOOT_BIN) $(STAGE2_BIN) $(KERNEL_BIN)
	@echo "  IMG   $(IMAGE)"
	@dd if=/dev/zero of=$(IMAGE) bs=512 count=2880 status=none
	@dd if=$(BOOT_BIN) of=$(IMAGE) bs=512 count=1 conv=notrunc status=none
	@dd if=$(STAGE2_BIN) of=$(IMAGE) bs=512 seek=1 conv=notrunc status=none
	@dd if=$(KERNEL_BIN) of=$(IMAGE) bs=512 seek=16 conv=notrunc status=none
	@echo "  NullOS floppy image: $(IMAGE)"

# Boot from floppy image in QEMU
run-floppy: image
	@$(QEMU) $(QEMU_FLOPPY)

# Build bootloader only
boot: dirs $(BOOT_BIN) $(STAGE2_BIN)
	@echo "  Boot sector: $(BOOT_BIN)"
	@echo "  Stage 2:     $(STAGE2_BIN)"

# Build kernel only
kernel: $(KERNEL_ELF)

# --- Clean ---
clean:
	@echo "  CLEAN build/"
	@rm -rf $(BUILD)
