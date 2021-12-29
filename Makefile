# https://www.systutorials.com/docs/linux/man/1-x86_64-w64-mingw32-gcc/
QEMU       :=qemu-system-x86_64
QEMU_FLAGS :=-drive format=raw,file=drive/drive.hdd -bios qemu/bios64.bin -m 256M -vga std -name TedOS -machine q35

EFI_CC      :=x86_64-w64-mingw32-gcc
EFI_CFLAGS  :=-ffreestanding -fshort-wchar -Og -ggdb -std=c99 -Wall -Werror -Wno-error=unused-parameter -Wno-error=unused-variable
EFI_LFLAGS  :=-nostdlib -lgcc -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main
EFI_OBJCOPY :=x86_64-w64-mingw32-objcopy

# This file name is what UEFI looks for in EFI/BOOT folder.
EFI_TARGET		:= BOOTX64.EFI
EFI_IMAGE 		:= release.BOOTX64.EFI
EFI_DEBUG_IMAGE := debug.BOOTX64.EFI

KERNEL_CC     :=x86_64-elf-gcc
KERNEL_CFLAGS :=-fno-unwind-tables -fno-exceptions -march=x86-64 -m64 -Og -ggdb --freestanding -Wall -Werror
KERNEL_LFLAGS :=-nostdlib -lgcc -e _start  # -T kernel.lds -static

BUILD_DIR :=build

SECTIONS :=.text .rdata .pdata .xdata .edata .idata .sdata .data .dynamic .dynsym .rel .rela .reloc
DEBUG_SECTIONS :=.debug_info .debug_abbrev .debug_loc .debug_aranges .debug_line .debug_macinfo .debug_str


# Using --subsystem,10 for UEFI application.
# More info about -nostdlib and -lgcc: https://cs107e.github.io/guides/gcc/.
# I got "undefined reference to `___chkstk_ms'". It linked with libgcc, so pass -lgcc to solve.
all: $(EFI_IMAGE) $(EFI_DEBUG_IMAGE) run


$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# We can compile the EFI_IMAGE in debug and strip debug symbols so the stripped
# version can be used to boot and the non-stripped can be a symbol file to gdb.
$(EFI_IMAGE): $(EFI_TARGET) $(BUILD_DIR)
	$(EFI_OBJCOPY) $(foreach sec,$(SECTIONS),-j $(sec)) --target=efi-app-x86_64 $(BUILD_DIR)/$< $(BUILD_DIR)/$@


$(EFI_DEBUG_IMAGE): $(EFI_TARGET) $(BUILD_DIR)
	cp $(BUILD_DIR)/$< $(BUILD_DIR)/$@


$(EFI_TARGET): src/bootloader/efi_main.c $(BUILD_DIR)
	$(EFI_CC) $(EFI_CFLAGS) -c $< -o $(BUILD_DIR)/$@
	$(EFI_CC) $< $(EFI_LFLAGS) -o $(BUILD_DIR)/$@


# TODO(ted): Create a target for generating drive/drive.hdd.
deploy: $(BUILD_DIR) $(EFI_IMAGE) $(EFI_DEBUG_IMAGE) kernel
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


# Using 'main' as entry point will make gcc complain about not having `int argv`
# and `char* argv[]` as arguments, so use something else (like `start`) instead.
# -v  - Verbose output. The commands the linker runs during compilation.
# -mcmodel=kernel  - Generate code for the kernel code model.
# -fno-asynchronous-unwind-tables - Disable generation of DWARF-based unwinding.
kernel: src/kernel.cpp $(BUILD_DIR)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -c $< -o $(BUILD_DIR)/$@.o
	$(KERNEL_CC) $(KERNEL_LFLAGS) $(BUILD_DIR)/$@.o -o  $(BUILD_DIR)/$@


clean:
	@echo "Cleaning files...."
	rm -fr $(BUILD_DIR)
	@echo "Done."
