## Info

UEFI uses a subset of the PE32+ image format with a modified header signature (COFF?), which is Microsoft ABI compliant ([UEFI-spec, p.15](./UEFI-Spec-2-9_2021-03-18.pdf)). This project is one of the three supported UEFI images, namely a UEFI application (subsystem 10).

## Setup

### To download

Compiling with GCC compiler on Windows systems (UEFI is a Windows system).

    brew install mingw-w64

Running in a virtual environment (using qemu).

    brew install qemu

You need to download and configure qemu to use OVMF.d in order to run in UEFI

    https://unix.stackexchange.com/a/228053
    https://github.com/BlankOn/ovmf-blobs

You might need to change the deploy script's permissions to be executable.

    chmod +x deploy.sh


### Already in the project
* qemu/bios64.bin - Required by qemu to run.
* drive/drive.hdd - A default disk image with FAT32 partition on it with the folder EFI/BOOT.



# Things to know:
* Freestanding mode might require the existence of the symbols memset, memcpy, memmove and memcmp. If your program crashes due to *"undefined reference to 'memcpy'"* or similar, then you must provide them.
* If you get *"undefined reference to '___chkstk_ms'"*, it's because gcc linked with libgcc, so pass -lgcc to solve it.
* The text isn't drawn from the edge while the `GraphicsOutput` is. This is expected, not a bug.


# Debugging
To debug A binary file, run qemu with the options `-S` (prevents the CPU from starting) and `-s` (listen for gdb service on port 1234).

    x86_64-elf-gdb
    qemu-system-x86_64 -s -S <output-name>

Qemu should now wait for a continue command from a gdb service at localhost:1234. Start up `gdb` and run

    x86_64-elf-gdb
    target remote localhost:1234

This will connect the debugger to qemu. Then, set a breakpoint at the location your program starts at, as there are many other lines of code that runs before the bootloader. The bootloader presumably starts at 0x7C00.

    break *0x7C00

Then run `continue`. Now, you might want to switch to assembly mode in gdb. Add the two following layouts, by typing the command in gdb:

    layout asm
    layout regs

You can also set the syntax to intel by running (which can be put in '~/.gdbinit' to run at startup):

    set disassembly-flavor intel

Here's the documentation for navigating the layouts https://sourceware.org/gdb/current/onlinedocs/gdb/TUI-Commands.html.

Then start stepping with `si`, or by setting breakpoints and running with `continue`.


## Debugging the kernel with graphical debugger

The kernel can be compiled with debug information, the bootloader only cares about the loadable segments. Once done, configure CLion to set up a GDB Remote Debug on localhost:1234 and select the kernel executable as Symbol File. For convenience, create a build configuration for 'make debug' and select it as a 'Before launch' step in the GDB Remote Debug configuration. You should now be able to set a breakpoint and automatically attach the graphical debugger.



# BIOS, UEFI, PI, EDK II

## Resources

* https://tianocore-training.github.io/Lesson-0/
* https://tianocore-training.github.io/Lesson-1/


## Platform initialization

Pipeline:
* SEC - Security
    Ensure firmware integrity is intact.
* PEI - Pre EFI Initialization
    Set up basic environment.
* DXE - Driver Execution Environment
    The bulk of the booting process.
* BDS - Boot Dev Select
    Determine how and where to boot the OS.
* TSL - Transient System Load
    Building up the OS.
* RT  - Run Time
    Running the OS.
* AL  - After Life
    OS shuts down or reboots.


## X64 startup - [UEFI p.27](./UEFI-Spec-2-9_2021-03-18.pdf)

All calls is with the C language calling convention.

During boot services time the processor is in the following execution mode:

* Uniprocessor
* Long mode, in 64-bit mode
* Paging mode enabled (although restrictions in attributes),and memory space defined by the UEFI memory map is identity mapped.
* Selectors are set to be flat and are otherwise not used.
* Interrupts enabled (no interrupt services are supported other than the UEFI boot services timer functions)
* 128 KiB+ of stack space, 16-byte aligned.
* ... and more ...

For an operating system to use any UEFI runtime services, it must:

* Preserve all memory in the memory map marked as runtime code and runtime data.

And it must during calls to runtime services have the following:

* Long mode, in 64-bit mode. 
* Paging enabled.
* 4 KiB+ of stack space, 16-byte aligned.
* ... and more ...



## File formats

### [Binary](https://software-dl.ti.com/ccs/esd/documents/sdto_cgt_an_introduction_to_binary_files.html)

When using `objdump -O binary` it'll put the text section in a flat binary. However, if you define functions before main, then they'll be put before main, meaning that the "entry point" is no longer at the start of the binary file. To fix this, compile with `-ffunction-sections` to put each function in a different section and then use a linker script that puts the section containing main at the top of the file.




# Problems

### _"cp: drive/default-font.psf: could not copy extended attributes to /tmp/mnt/default-font.psf: Operation not supported_"

On MacOS, if a file has extended attributes (denoted by @ after the permissions when doing `ls -l`) the `cp` might fail. Do `xattr -c <file>` on the file to remove the extended attributes. 