#!/usr/bin/env bash


# Create the hdd file.
# hdiutil create -fs fat32 -ov -size 48m -volname NEWOS -format UDTO -srcfolder diskImage uefi.cdr


# Attach our drive as a virtual disk image.
# We'll get information of multiple disks images back, for example the
# path for "GUID_partition_scheme" and "Microsoft Basic Data".
# We want the path to "Microsoft Basic Data".
disk=$( \
  hdiutil attach -imagekey diskimage-class=CRawDiskImage -nomount drive/drive.hdd \
  | grep "Microsoft Basic Data" \
  | cut -d' ' -f1 \
)



# Mount the disk to a temporary location.
mkdir -p /tmp/mnt
mount -t msdos "${disk}" /tmp/mnt &> /dev/null

cp build/release.BOOTX64.EFI /tmp/mnt/EFI/Boot/BOOTX64.EFI
cp drive/text.txt /tmp/mnt/text.txt
cp drive/default-font.psf /tmp/mnt/default-font.psf
cp build/kernel /tmp/mnt/kernel


# Unmount and detach the disk.
umount /tmp/mnt
hdiutil detach $disk

# Qemu needs the bios64.bin file, but it's necessary for real hardware.
# bios64.bin on real hardware is just the motherboard firmware.
#qemu-system-x86_64 -drive format=raw,file=drive/drive.hdd -bios qemu/bios64.bin -m 256M -vga std -name TedOS -machine q35
