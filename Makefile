# https://www.gnu.org/software/make/manual/html_node/Automatic-Variables.html#Automatic-Variables
# https://www.systutorials.com/docs/linux/man/1-x86_64-w64-mingw32-gcc/
QEMU :=qemu-system-x86_64
QEMU_FLAGS :=-drive format=raw,file=drive/drive.hdd -bios qemu/bios64.bin -m 256M -vga std -name TedOS -machine q35 -serial stdio # -d cpu_reset -d int -d guest_errors    -monitor stdio -no-reboot -no-shutdown

BUILD_DIR  :=build
SOURCE_DIR :=src


# ---- BOOTLOADER ----
EFI_CC :=x86_64-w64-mingw32-gcc
EFI_LD :=x86_64-w64-mingw32-ld

# Using --subsystem,10 for UEFI application.
# More info about -nostdlib and -lgcc: https://cs107e.github.io/guides/gcc/.
# I got "undefined reference to `___chkstk_ms'". It linked with libgcc, so pass -lgcc to solve.
# -finstrument-functions
EFI_FLAGS_WARNINGS  := -Wall -Wextra -Wvla -Wfatal-errors -Werror -Wdouble-promotion -Wformat-signedness -Wshadow -Wformat=2 -Wformat-truncation -Wundef -Wconversion -Wno-unused-parameter # -Wpadded
EFI_FLAGS_CODE_GEN  := -m64 -fno-omit-frame-pointer -march=x86-64 -ffreestanding -Og -g3 -ggdb -ffunction-sections -fdata-sections -fbounds-check -ftrapv -fno-common -fverbose-asm
EFI_FLAGS_INTERRUPT := -mgeneral-regs-only -mno-red-zone -mgeneral-regs-only
EFI_FLAGS_MONITOR   := -fstack-usage --std=gnu99 -nostdlib -lgcc -shared
EFI_FLAGS_DEFINES   := -D USE_WIDE_CHARACTER=1
EFI_CFLAGS          := $(EFI_FLAGS_WARNINGS) $(EFI_FLAGS_CODE_GEN) $(EFI_FLAGS_INTERRUPT) $(EFI_FLAGS_MONITOR) $(EFI_FLAGS_DEFINES)
EFI_LFLAGS          := -Wl,--gc-sections -Wl,-dll -Wl,--subsystem,10 -e EfiMain  # -Wl,--print-gc-sections

EFI_IMAGE := BOOTX64.EFI   # This file name is what UEFI looks for in EFI/BOOT folder.

EFI_SOURCE_DIR :=src/bootloader
EFI_SOURCES    :=$(EFI_SOURCE_DIR)/efi_main.c $(EFI_SOURCE_DIR)/efi.c $(EFI_SOURCE_DIR)/efi_lib.c $(EFI_SOURCE_DIR)/efi_error.c $(SOURCE_DIR)/page_allocator.c


# ---- KERNEL ----
KERNEL_CC :=x86_64-elf-gcc
KERNEL_LD :=x86_64-elf-ld
NASM      :=nasm

# https://gcc.gnu.org/onlinedocs/gcc/x86-Function-Attributes.html#x86-Function-Attributes
# Since GCC doesn’t preserve SSE, MMX nor x87 states, the GCC option -mgeneral-regs-only should be used to compile interrupt and exception handlers.
# Any interruptible-without-stack-switch code must be compiled with -mno-red-zone since interrupt handlers can and will, because of the hardware design, touch the red zone.
# Using 'main' as entry point will make gcc complain about not having `int argv`
# and `char* argv[]` as arguments, so use something else (like `_start`) instead.
# -v  - Verbose output. The commands the linker runs during compilation.
# -mcmodel=kernel  - Generate code for the kernel code model.
# -fno-asynchronous-unwind-tables - Disable generation of DWARF-based unwinding.
# https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
KERNEL_WARNINGS :=-Wall -Wextra -Wvla -Wfatal-errors -Werror -Wdouble-promotion -Wformat-signedness -Wshadow -Wformat=2 -Wformat-truncation -Wundef -fno-common  # -Wconversion -Wpadded
KERNEL_CFLAGS   :=-fno-asynchronous-unwind-tables -fstack-usage -mgeneral-regs-only -mno-red-zone -march=x86-64 -m64 --freestanding -ffunction-sections -fdata-sections --std=gnu99 -m64 -Og -g3 -ggdb $(KERNEL_WARNINGS)
KERNEL_LFLAGS   :=-nostdlib -e _start -Wl,--gc-sections -Wl,--print-gc-sections
KERNEL_NASM_FLAGS :=-f elf64 -g -F dwarf

KERNEL_SOURCES := $(SOURCE_DIR)/kernel.c


OBJCOPY  :=x86_64-w64-mingw32-objcopy
SECTIONS :=.text .rdata .pdata .xdata .edata .idata .sdata .data .dynamic .dynsym .rel .rela .reloc
DEBUG_SECTIONS :=.debug_info .debug_abbrev .debug_loc .debug_aranges .debug_line .debug_macinfo .debug_str



all: bootloader run


$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


# An EFI binary with additionally debug sections will be unable to launch by the EFI firmware.
bootloader: $(EFI_SOURCES) $(BUILD_DIR)
	$(NASM) src/x86_64/x86_64.asm $(KERNEL_NASM_FLAGS) -o $(BUILD_DIR)/x86_64.o
	$(EFI_CC) $(BUILD_DIR)/x86_64.o $(EFI_SOURCES) $(EFI_CFLAGS) $(EFI_LFLAGS) -o $(BUILD_DIR)/symbol-file-$(EFI_IMAGE)
	$(OBJCOPY) $(foreach sec,$(SECTIONS),-j $(sec)) --target=efi-app-x86_64 $(BUILD_DIR)/symbol-file-$(EFI_IMAGE) $(BUILD_DIR)/$(EFI_IMAGE)



# TODO(ted): Create a target for generating drive/drive.hdd.
deploy: $(BUILD_DIR) bootloader kernel
	./deploy.sh


# Qemu needs the bios64.bin file, but it's necessary for real hardware.
# bios64.bin on real hardware is just the motherboard firmware.
run: $(BUILD_DIR) kernel drive/drive.hdd deploy
	$(QEMU) $(QEMU_FLAGS)


# https://wiki.osdev.org/Debugging_UEFI_applications_with_GDB
# https://sourceforge.net/p/ast-phoenix/code/ci/master/tree/kernel/boot/Makefile#l43
#qemu-system-x86_64 -s -bios qemu/bios64.bin -net none -debugcon file:debug.log -global isa-debugcon.iobase=0x402
#x86_64-w64-mingw32-objcopy $(foreach sec,$(SECTIONS),-j $(sec)) --target=efi-app-x86_64	$(target) a_$(target)
#qemu-system-x86_64 -S -s -drive format=raw,file=drive/drive.hdd -bios qemu/bios64.bin -m 256M -vga std -name TedOS -machine q35 &
#x86_64-elf-gdb

# Unfortunately, running x86_64-elf-gdb -ex "target remote localhost:1234"
# will make qemu terminate on ctrl-C instead of pausing the program.
debug: kernel drive/drive.hdd deploy $(BUILD_DIR)
	$(QEMU) -S -s $(QEMU_FLAGS) &


kernel: $(KERNEL_SOURCES) $(SOURCE_DIR)/x86_64/x86_64.asm $(BUILD_DIR)
	$(NASM) src/x86_64/x86_64.asm $(KERNEL_NASM_FLAGS) -o $(BUILD_DIR)/x86_64.o
	$(KERNEL_CC) $(BUILD_DIR)/setup.o $(BUILD_DIR)/x86_64.o $(KERNEL_CFLAGS) $(KERNEL_LFLAGS) -o $(BUILD_DIR)/$@.o

clean:
	@echo "Cleaning files..."
	rm -fr $(BUILD_DIR)
	@echo "Done."
