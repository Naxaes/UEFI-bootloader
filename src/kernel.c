#include <stdarg.h>

#include "preamble.h"
#include "bootloader.h"


#include "memory.c"
#include "stdio.c"
#include "renderer.c"
#include "maths.c"
#include "string.c"
//#include "allocator.c"
#define PAGE_SIZE 4096

#define IN
#define OUT
#define OPTIONAL


// ---- Bit hacks ----
// https://www.youtube.com/watch?v=ZRNO-ewsNcQ

typedef struct Cursor
{
    int row;
    int col;
} Cursor;


typedef struct Window
{
    int width;
    int height;
} Window;

typedef struct Font
{
    int  size;
    u8*  glyphs;
} Font;


void debug_break()
{
    // Change to 0 in debugger to continue.
    int volatile wait = 1;
    while (wait) {
        __asm__ __volatile__("pause");
    }
}

Graphics*  g_graphics   = NULL;
Cursor*    g_cursor     = NULL;
PSF1_Font* g_font       = NULL;
Pixel      g_text_color = { 0xFF, 0xFF, 0xFF, 0xFF };


/* ---- RENDERER ---- */
inline void draw(Pixel pixel, int row, int col)
{
    g_graphics->base[g_graphics->pixels_per_scanline * row + col] = pixel;
}

void fill(Pixel pixel)
{
    for (u64 i = 0; i < g_graphics->size / sizeof(Pixel); ++i)
        g_graphics->base[i] = pixel;
}

void newline()
{
    int row_step = g_font->scale * g_font->header.font_height;

    int height = (int) g_graphics->height;
    int width  = (int) g_graphics->width;

    g_cursor->col  = 0;
    g_cursor->row += row_step;
    if (g_cursor->row + row_step >= height)
    {
        int row = 0;
        for (; row < height-1; row+=row_step)
        {
            for (int y = 0; y < row_step; ++y)
            {
                int row_above_index   = (row+y)          * width;
                int row_current_index = (row+y+row_step) * width;
                for (int col = 0; col < width; ++col)
                {
                    g_graphics->base[row_above_index+col] = g_graphics->base[row_current_index+col];
                }
            }
        }

        for (int y = 0; y < height-row; ++y)
        {
            int row_current_index = (row+y+row_step) * width;
            for (int col = 0; col < width; ++col)
            {
                g_graphics->base[row_current_index+col] = BLACK;
            }
        }

        g_cursor->row -= row_step;
    }
}


void advance_cursor(int step)
{
    // TODO(ted): FIX! Width is hardcoded.
    // TODO(ted): Should `step` continue after a newline?
    int col_step = g_font->scale * (g_font->header.font_height / 2);
    g_cursor->col += col_step * step;
    if (g_cursor->col + col_step >= (int) g_graphics->width)
        newline();
}


void print_char(char character)
{
    u8* glyph = g_font->glyphs + (character * g_font->header.font_height);
    for (int row = 0; row < 16; ++row)
    {
        for (int col = 0; col < 8; ++col)
        {
            if (*glyph & (0x80 /* 0b10000000 */ >> col))
                for (int i = 0; i < g_font->scale; ++i)
                    for (int j = 0; j < g_font->scale; ++j)
                        draw(g_text_color, g_cursor->row + row*g_font->scale + i, g_cursor->col + col*g_font->scale + j);
        }
        glyph++;
    }

    advance_cursor(1);
}

int printf(const char* format, ...)
{
    va_list arg;
    va_start(arg, format);

    const char* character = format;
    while (*character != '\0')
    {
        switch (*character)
        {
            case '\t': advance_cursor(4); break;
            case '\n': newline(); break;
            case '\\':
                if (*(character+1) == 'e')
                {
                    if (strcmp(character, ANSI_COLOR_CODE_NORMAL) == 0)
                    {
                        g_text_color = WHITE;
                        character += sizeof(ANSI_COLOR_CODE_NORMAL)-1;
                        continue;
                    }
                    else if (strcmp(character, ANSI_COLOR_CODE_WHITE) == 0)
                    {
                        g_text_color = WHITE;
                        character += sizeof(ANSI_COLOR_CODE_WHITE)-1;
                        continue;
                    }
                    else if (strcmp(character, ANSI_COLOR_CODE_BLACK) == 0)
                    {
                        g_text_color = BLACK;
                        character += sizeof(ANSI_COLOR_CODE_BLACK)-1;
                        continue;
                    }
                    else if (strcmp(character, ANSI_COLOR_CODE_RED) == 0)
                    {
                        g_text_color = RED;
                        character += sizeof(ANSI_COLOR_CODE_RED)-1;
                        continue;
                    }
                    else
                    {
                        PANICF("Not implemented '%s'\n", character);
                    }
                }
                break;
            case '%':
                switch (*(++character))
                {
                    case L'c':  // char
                    {
                        char c = va_arg(arg, int);
                        print_char(c);
                        break;
                    }
                    case L'd':  // int - decimal
                    {
                        i32 d = va_arg(arg, int);
                        if (d < 0) print_char('-');
                        u32 u = abs32(d);
                        char buffer[20];
                        printf("%s", usize_to_string(buffer, u, 10));
                        break;
                    }
                    case L'o':  // int - octal
                    {
                        break;
                    }
                    case L's':  // null-terminated string
                    {
                        char* s = va_arg(arg, char*);
                        while (*s != '\0')
                            print_char(*s++);
                        break;
                    }
                    case L'x':  // int - hexadecimal
                    {
                        print_char('0');
                        print_char('x');
                        usize x = va_arg(arg, usize);
                        char buffer[20];
                        printf("%s", usize_to_string(buffer, x, 16));
                        break;
                    }
                    case L'z':  // size_t or ssize_t
                    {
                        if (*(character+1) == L'u')
                        {
                            ++character;
                            usize zu = va_arg(arg, usize);
                            char buffer[20];
                            printf("%s", usize_to_string(buffer, zu, 10));
                            break;
                        }
                        else if (*(character+1) == L'x')
                        {
                            print_char('0');
                            print_char('x');
                            usize x = va_arg(arg, usize);
                            char buffer[20];
                            printf("%s", usize_to_string(buffer, x, 16));
                            break;
                        }
                        else
                        {
                            i64 z = va_arg(arg, i64);
                            if (z < 0) print_char('-');
                            u64 zu = abs64(z);
                            char buffer[20];
                            printf("%s", usize_to_string(buffer, zu, 10));
                            break;
                        }
                    }
                    default:
                    {
                        LOGF("Unknown format option %s\n", character);
                        return -1;
                    }
                }
                break;
            default:
                print_char(*character);
                break;
        }

        ++character;
    }

    va_end(arg);
    return 0;
}
#define dprintf(format, ...) printf(ANSI_COLOR_CODE_RED format ANSI_COLOR_CODE_NORMAL, __VA_ARGS__)




//static const u16 VGA_CONTROL_REGISTER = 0x3d4;
//static const u16 VGA_DATA_REGISTER    = 0x3d5;


inline u8 read_port(u16 port)
{
    u8 result = 0;
    __asm__ __volatile__("in %%dx, %%al" : "=a" (result) : "d" (port));
    return result;
}

inline void write_port(u16 port, u8 data)
{
    __asm__ __volatile__("out %%al, %%dx" : : "a" (data), "d" (port));
}


void delay()
{
    u32 volatile x = 1;
    while (x++ < 10000000);
}


typedef struct Bitmap {
    u8*   base;
    usize size;
} Bitmap;

usize is_set(const Bitmap* bitmap, usize index)
{
    usize j = index / 8;
    usize i = index % 8;

    usize result = BIT_CHECK(bitmap->base[j], i);
    return result;
}

void set(Bitmap* bitmap, usize index)
{
    usize j = index / 8;
    usize i = index % 8;

    BIT_SET(bitmap->base[j], i);
}
void unset(Bitmap* bitmap, usize index)
{
    usize j = index / 8;
    usize i = index % 8;

    BIT_CLEAR(bitmap->base[j], i);
}


/*
During boot services time the processor is in the following execution mode:
• Uniprocessor, as described in chapter 8.4 of:
— Intel 64 and IA-32 Architectures Software Developer's Manual, Volume 3, System Programming Guide, Part 1, Order Number: 253668-033US, December 2009
— See “Links to UEFI-Related Documents” (http://uefi.org/uefi) under the heading “Intel Processor Manuals”.
• Long mode, in 64-bit mode
• Paging mode is enabled and any memory space defined by the UEFI memory map is identity mapped (virtual address equals physical address), although the attributes of certain regions may not have all read, write, and execute attributes or be unmarked for purposes of platform protection. The mappings to other regions, such as those for unaccepted memory, are undefined and may vary from implementation to implementation.
• Selectors are set to be flat and are otherwise not used.
• Interrupts are enabled–though no interrupt services are supported other than the UEFI boot services timer functions (All loaded device drivers are serviced synchronously by “polling.”)
• Direction flag in EFLAGs is clear
• Other general purpose flag registers are undefined
• 128 KiB, or more, of available stack space
• The stack must be 16-byte aligned. Stack may be marked as non-executable in identity mapped page tables.
• Floating-point control word must be initialized to 0x037F (all exceptions masked, double- extended-precision, round-to-nearest)
• Multimedia-extensions control word (if supported) must be initialized to 0x1F80 (all exceptions masked, round-to-nearest, flush to zero for masked underflow).
• CR0.EM must be zero
• CR0.TS must be zero
 */
#include "idt.c"
extern void x86_64_interrupt(int);


int _start(Context* context)
{
    // ---- INITIALIZATION START ---
    Cursor cursor = { 0, 0 };

    g_cursor   = &cursor;
    g_graphics = &context->graphics;
    g_font     = &context->font;

    load_gdt(get_descriptor());
    idt_install();

    x86_64_interrupt(3);

    fill(BLACK);
//    PageAllocator allocator = memory_map(&context->memory);
//    printf("Allocator: { base=%zx, size=%zx }\n", (usize) allocator.base, (usize) allocator.size);

    printf("%s\n", test);

    printf(
        "Testing:\n\t" ANSI_COLOR_CODE_RED "c: %c" ANSI_COLOR_CODE_NORMAL "\n\td: %d\n\td: %d\n\ts: %s\n\tx: %x\n\tz: %d\n\tz: %d\n\tzu: %d\n",
        'X', 10, -10, "Hello", 0xdead, 4321, -4321, 4321
    );

//    debug_break();

    printf(
        "\n"
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Cras ex diam, pharetra lacinia consectetur ac, facilisis ac justo. Nullam sem urna, viverra eu ante ut, dapibus egestas eros. Ut semper ligula ut elit interdum, sit amet facilisis sapien malesuada. Morbi fermentum augue eget leo scelerisque ornare. Mauris vestibulum non ex ut pharetra. In hac habitasse platea dictumst. Curabitur fermentum velit eros, id auctor tellus laoreet quis. Sed nec maximus sapien. Vestibulum a mattis ex.\n"
        "\n"
        "Aliquam tortor tellus, vulputate nec tincidunt in, elementum ac lectus. In nec lacus id massa luctus tempus ut in magna. Sed augue ipsum, porttitor a justo et, tincidunt cursus ipsum. Nullam pellentesque eros sed pellentesque convallis. Quisque euismod ex justo, quis tincidunt sem consectetur eget. Morbi hendrerit ultrices bibendum. Vestibulum et nulla quis urna posuere aliquam in vitae tellus. Nulla sit amet accumsan arcu. Nulla sagittis egestas magna at lacinia. Duis posuere vel ex id mollis. Nunc blandit elit eget elit interdum vehicula.\n"
        "\n"
        "Nunc fringilla odio a dignissim tristique. Donec mattis, arcu et hendrerit dapibus, justo elit tristique ligula, ac venenatis magna ipsum scelerisque est. In quis neque ac leo blandit volutpat nec eu sapien. Sed lacinia risus vitae aliquam facilisis. Aliquam in libero iaculis, consequat lectus non, pharetra nulla. Pellentesque fermentum eget urna at fermentum. Etiam finibus tincidunt est vel imperdiet. Vivamus efficitur cursus nisi luctus tristique. Quisque sed pretium orci, nec suscipit odio. Vivamus molestie tortor sit amet porttitor finibus.\n"
        "\n"
        "Phasellus sed sodales magna, eu rutrum neque. Sed tortor mauris, mollis ut aliquam eu, viverra sed felis. Nunc a nulla lacus. Phasellus commodo nunc purus, a scelerisque nisl sollicitudin eget. Maecenas vehicula mi id elit sollicitudin sollicitudin. Sed vel varius nulla. Integer sed placerat nulla. Integer vitae tellus dapibus, faucibus enim sed, eleifend justo. Nam suscipit tellus vitae mattis aliquet. Aliquam a libero blandit, commodo purus et, volutpat sapien.\n"
        "\n"
        "Aliquam hendrerit felis vitae lacus egestas sodales. Aliquam mauris lorem, aliquet at ultricies in, vulputate hendrerit justo. Praesent et accumsan ex. Fusce ac tempus ipsum, id iaculis eros. Integer id orci mattis, suscipit augue quis, luctus justo. Donec congue, magna quis mollis imperdiet, magna odio semper magna, sit amet dignissim tortor orci quis erat. Suspendisse fermentum est eget semper aliquet. Praesent gravida dui a metus iaculis consequat. "
    );


//    debug_break();
    for (usize i = 0; i < context->graphics.size / sizeof(Pixel); ++i)
    {
        context->graphics.base[i] = (Pixel) { 0xFF, 0, 0xFF, 0xFF };
    }


    int value = (u64) context;
    return LETTERS[value][2];
}
