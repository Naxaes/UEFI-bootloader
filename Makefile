OBJCOPY :=x86_64-w64-mingw32-objcopy
CC      :=x86_64-w64-mingw32-gcc

CFLAGS  :=-Wall -Werror -m64 -ffreestanding -Og -ggdb
LFLAGS  :=-nostdlib -lgcc -shared -Wl,-dll -Wl,--subsystem,10 -e efi_main -o

# This file name is what UEFI looks for in EFI/BOOT folder.
TARGET			:= BOOTX64.EFI
EFI_IMAGE 		:= release.BOOTX64.EFI
EFI_DEBUG_IMAGE := debug.BOOTX64.EFI

BUILD_DIR :=build

SECTIONS :=.text .rdata .pdata .xdata .edata .idata .sdata .data .dynamic .dynsym .rel .rela .reloc
DEBUG_SECTIONS :=.debug_info .debug_abbrev .debug_loc .debug_aranges .debug_line .debug_macinfo .debug_str


# Using --subsystem,10 for UEFI application.
# More info about -nostdlib and -lgcc: https://cs107e.github.io/guides/gcc/.
# I got "undefined reference to `___chkstk_ms'". It linked with libgcc, so pass -lgcc to solve.
all: $(EFI_IMAGE) $(EFI_DEBUG_IMAGE) run


$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


$(EFI_IMAGE): $(TARGET) $(BUILD_DIR)
	$(OBJCOPY) $(foreach sec,$(SECTIONS),-j $(sec)) --target=efi-app-x86_64 $(BUILD_DIR)/$< $(BUILD_DIR)/$@


$(EFI_DEBUG_IMAGE): $(TARGET) $(BUILD_DIR)
	$(OBJCOPY) $(foreach sec,$(SECTIONS) $(DEBUG_SECTIONS),-j $(sec)) --target=efi-app-x86_64 $(BUILD_DIR)/$< $(BUILD_DIR)/$@


$(TARGET): src/efi_main.c $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $(BUILD_DIR)/$@
	$(CC) $< $(LFLAGS) $(BUILD_DIR)/$@


# TODO(ted): Create a target for generating drive/drive.hdd.
deploy: $(EFI_IMAGE) $(BUILD_DIR)
	./deploy.sh


run: drive/drive.hdd deploy
	# Qemu needs the bios64.bin file, but it's necessary for real hardware.
	# bios64.bin on real hardware is just the motherboard firmware.
	qemu-system-x86_64 -drive format=raw,file=drive/drive.hdd -bios qemu/bios64.bin -m 256M -vga std -name TedOS -machine q35


# https://wiki.osdev.org/Debugging_UEFI_applications_with_GDB
# https://sourceforge.net/p/ast-phoenix/code/ci/master/tree/kernel/boot/Makefile#l43
#qemu-system-x86_64 -s -bios qemu/bios64.bin -net none -debugcon file:debug.log -global isa-debugcon.iobase=0x402
#x86_64-w64-mingw32-objcopy $(foreach sec,$(SECTIONS),-j $(sec)) --target=efi-app-x86_64	$(target) a_$(target)
#qemu-system-x86_64 -S -s -drive format=raw,file=drive/drive.hdd -bios qemu/bios64.bin -m 256M -vga std -name TedOS -machine q35 &
#x86_64-elf-gdb

# Unfortunately, running x86_64-elf-gdb -ex "target remote localhost:1234"
# will make qemu terminate on ctrl-C instead of pausing the program.
debug: drive/drive.hdd deploy
	qemu-system-x86_64 -S -s -drive format=raw,file=drive/drive.hdd -bios qemu/bios64.bin -m 256M -vga std -name TedOS -machine q35 &


clean:
	@echo "Cleaning files...."
	rm -fr $(BUILD_DIR)
	@echo "Done."
