cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(UEFI-bootloader)

set(CMAKE_C_STANDARD 99)


# These are only here to provide intellisense for CLion.
add_executable(not_actually_a_target1 ../src/kernel.c)
add_executable(not_actually_a_target2 ../src/bootloader/efi_main.c)



add_executable(format format.c)
add_executable(elf elf.c ../src/elf.c)
add_executable(page_allocator page_allocator.c ../src/page_allocator.c)