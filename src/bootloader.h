#pragma once
// Things that are shared between the bootloader and the kernel.

#include "efi_main.h"
#include "types.h"


typedef struct Pixel {
    u8 blue;
    u8 green;
    u8 red;
    u8 alpha;
} Pixel;

typedef struct Graphics {
    volatile Pixel* base;
    u64     size;
    u32     width;
    u32     height;
    u32     pixels_per_scanline;
} Graphics;

typedef struct Memory {
    EFI_MEMORY_DESCRIPTOR* MemoryMap;
    UINTN                  MemoryMapSize;
    UINTN                  DescriptorSize;
} Memory;


typedef struct PSF1_Header {
    u8 magic[2];
    u8 file_mode;
    u8 font_height;
} PSF1_Header;

typedef struct PSF1_Font {
    PSF1_Header header;
    int         scale;
    u8*         glyphs;
} PSF1_Font;

typedef struct Context
{
    EFI_RUNTIME_SERVICES* services;
    Memory    memory;
    Graphics  graphics;
    PSF1_Font font;
} Context;
