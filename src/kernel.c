#include <stdarg.h>

#include "bootloader.h"
#include "types.h"

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


// ---- Bit stuff ----
// https://www.youtube.com/watch?v=ZRNO-ewsNcQ
#define BIT_SET(a, b)   ((a) |=  (1ULL << (b)))
#define BIT_CLEAR(a, b) ((a) &= ~(1ULL << (b)))
#define BIT_FLIP(a, b)  ((a) ^=  (1ULL << (b)))
#define BIT_CHECK(a, b) (!!((a) & (1ULL << (b))))

#define BITMASK_SET(x, mask)        ((x) |= (mask))
#define BITMASK_CLEAR(x, mask)      ((x) &= (~(mask)))
#define BITMASK_FLIP(x, mask)       ((x) ^= (mask))
#define BITMASK_CHECK_ALL(x, mask)  (!(~(x) & (mask)))
#define BITMASK_CHECK_ANY(x, mask)  ((x) & (mask))






void debug_halt();
void printf(const char* format, ...);
#define PANIC(...)                                                             \
    do {                                                                       \
        printf(                                                                \
            "[Panic]:\n"                                                       \
            "\tFile:     %s\n"                                                 \
            "\tLine:     %d\n"                                                 \
            "\tMessage:  ",                                                    \
            __FILE__, __LINE__                                                 \
        );                                                                     \
        printf(__VA_ARGS__);                                                   \
        debug_halt();                                                          \
    } while (0)

#define ASSERT(statement, ...)                                                 \
    do {                                                                       \
        if (!(statement))                                                      \
        {                                                                      \
            printf(                                                            \
                "[Assert] (%s):\n"                                             \
                "\tFile:     %s\n"                                             \
                "\tLine:     %d\n"                                             \
                "\tMessage:  ",                                                \
                #statement, __FILE__, __LINE__                                 \
            );                                                                 \
            printf(__VA_ARGS__);                                               \
            debug_halt();                                                      \
        }                                                                      \
    } while (0)



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

void printf(const char* format, ...)
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
                        PANIC("Not implemented '%s'\n", character);
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
                        printf(usize_to_string(buffer, u, 10));
                        break;
                    }
                    case L'o':  // int - octal
                    {
                        break;
                    }
                    case L's':  // null-terminated string
                    {
                        char* s = va_arg(arg, char*);
                        printf(s);  // TODO(ted): Safe? This allow non-static `format.
                        break;
                    }
                    case L'x':  // int - hexadecimal
                    {
                        print_char('0');
                        print_char('x');
                        usize x = va_arg(arg, usize);
                        char buffer[20];
                        printf(usize_to_string(buffer, x, 16));
                        break;
                    }
                    case L'z':  // size_t or ssize_t
                    {
                        if (*(character+1) == L'u')
                        {
                            ++character;
                            usize zu = va_arg(arg, usize);
                            char buffer[20];
                            printf(usize_to_string(buffer, zu, 10));
                            break;
                        }
                        else
                        {
                            i64 z = va_arg(arg, i64);
                            if (z < 0) print_char('-');
                            u64 zu = abs64(z);
                            char buffer[20];
                            printf(usize_to_string(buffer, zu, 10));
                            break;
                        }
                    }
                    default:
                    {
                        ASSERT(0, "Unknown format option %s\n", character);
                        return;
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



void debug_halt()
{
    // Change to 0 in debugger to continue.
    int volatile wait = 1;
    while (wait) {
        __asm__ __volatile__("pause");
    }
}



void print_memory_map(const Memory* memory)
{
    intn entries = memory->MemoryMapSize / memory->DescriptorSize;
    intn base    = (u64) memory->MemoryMap;

    printf("Memory mappings:\n");
    for (intn i = 0; i < entries; ++i)
    {
        EFI_MEMORY_DESCRIPTOR* descriptor = (EFI_MEMORY_DESCRIPTOR*)(base + memory->DescriptorSize * i);

        printf(
            "%s: %x KiB (%x -> %x)\n",
            EFI_MEMORY_TYPE_STRINGS_CHAR[descriptor->Type],
            descriptor->NumberOfPages * 4096 / 1024,
            descriptor->PhysicalStart,
            descriptor->VirtualStart
        );
    }
}


typedef struct Bitmap {
    u8*   base;
    usize size;
} Bitmap;

intn is_set(const Bitmap* bitmap, usize index)
{
    usize j = index / 8;
    usize i = index % 8;

    intn result = BIT_CHECK(bitmap->base[j], i);
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


typedef struct PageAllocator {
    Bitmap header;
    u8*    base;
    usize  size;
} PageAllocator;


usize g_total_memory    = 0;
usize g_used_memory     = 0;
usize g_free_memory     = 0;
usize g_reserved_memory = 0;


/// Make page marked as used for conventional usage.
void lock_page(PageAllocator* allocator, void* address)
{
    usize index = (usize) (address) / PAGE_SIZE;
    ASSERT(is_set(&allocator->header, index), "Page already locked!");
    set(&allocator->header, index);
    g_free_memory -= PAGE_SIZE;
    g_used_memory += PAGE_SIZE;
}
void lock_pages(PageAllocator* allocator, void* address, usize count)
{
    for (usize i = 0; i < count; ++i)
        lock_page(allocator, (void *) ((usize) (address) + count * PAGE_SIZE));
}

/// Make page available from conventional usage.
void free_page(PageAllocator* allocator, void* address)
{
    usize index = (usize) (address) / PAGE_SIZE;
    ASSERT(!is_set(&allocator->header, index), "Page already free!");
    unset(&allocator->header, index);
    g_free_memory += PAGE_SIZE;
    g_used_memory -= PAGE_SIZE;
}
void free_pages(PageAllocator* allocator, void* address, usize count)
{
    for (usize i = 0; i < count; ++i)
        free_page(allocator, (void *) ((usize) (address) + count * PAGE_SIZE));
}

/// Make page marked as used for internal/reserved usage.
void reserve_page(PageAllocator* allocator, void* address)
{
    usize index = (usize) (address) / PAGE_SIZE;
    ASSERT(is_set(&allocator->header, index), "Page already reserved!");
    set(&allocator->header, index);
    g_free_memory     -= PAGE_SIZE;
    g_reserved_memory += PAGE_SIZE;
}
void reserve_pages(PageAllocator* allocator, void* address, usize count)
{
    for (usize i = 0; i < count; ++i)
        reserve_page(allocator, (void *) ((usize) (address) + count * PAGE_SIZE));
}

/// Make page available from internal/reserved usage.
void release_page(PageAllocator* allocator, void* address)
{
    usize index = (usize) (address) / PAGE_SIZE;
    ASSERT(!is_set(&allocator->header, index), "Page already released!");
    unset(&allocator->header, index);
    g_free_memory     += PAGE_SIZE;
    g_reserved_memory -= PAGE_SIZE;
}
void release_pages(PageAllocator* allocator, void* address, usize count)
{
    for (usize i = 0; i < count; ++i)
        release_page(allocator, (void *) ((usize) (address) + count * PAGE_SIZE));
}

PageAllocator memory_map(const Memory* memory)
{
    //
    intn entries = memory->MemoryMapSize / memory->DescriptorSize;
    intn base    = (u64) memory->MemoryMap;

    u8*   largest_memory_segment = NULL;
    usize largest_memory_size    = 0;

    usize total_memory_size = 0;
    usize total_free_memory_size = 0;
    usize total_reserved_memory_size = 0;

    for (intn i = 0; i < entries; ++i)
    {
        EFI_MEMORY_DESCRIPTOR* descriptor = (EFI_MEMORY_DESCRIPTOR *)(base + memory->DescriptorSize * i);
        usize memory_size = descriptor->NumberOfPages * PAGE_SIZE;

        printf("%s at %x with %d pages\n", EFI_MEMORY_TYPE_STRINGS_CHAR[descriptor->Type], descriptor->PhysicalStart, descriptor->NumberOfPages);

        total_memory_size += memory_size;
        if (descriptor->Type == EfiConventionalMemory)
        {
            total_free_memory_size += memory_size;
            if (descriptor->NumberOfPages * PAGE_SIZE > largest_memory_size)
            {
                largest_memory_segment = (u8 *) descriptor->PhysicalStart;
                largest_memory_size    = memory_size;
            }
        }
        else
        {
            total_reserved_memory_size += memory_size;
        }
    }

    usize bitmap_size = total_memory_size / PAGE_SIZE / 8 + 1;

    // TODO(ted): Align.
    Bitmap bitmap = { largest_memory_segment, bitmap_size };
    PageAllocator allocator = { bitmap, largest_memory_segment + bitmap_size, largest_memory_size - bitmap_size };

    g_total_memory    = total_memory_size;
    g_free_memory     = total_free_memory_size;
    g_reserved_memory = total_reserved_memory_size;
    g_used_memory     = bitmap_size;

    printf(
        "Total memory:    %d Kib\n"
        "Free memory:     %d Kib\n"
        "Reserved memory: %d Kib\n"
        "Used memory:     %d Kib\n",
        g_total_memory / 1024, g_free_memory / 1024, g_reserved_memory / 1024, g_used_memory / 1024
    );

    return allocator;
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
    PageAllocator allocator = memory_map(&context->memory);
    printf("Allocator: { base=%x, size=%x }\n", allocator.base, allocator.size);


    print_memory_map(&context->memory);

    printf(
        "Testing:\n\t" ANSI_COLOR_CODE_RED "c: %c" ANSI_COLOR_CODE_NORMAL "\n\td: %d\n\td: %d\n\ts: %s\n\tx: %x\n\tz: %z\n\tz: %z\n\tzu: %zu\n",
        'X', 10, -10, "Hello", 0xdead, 4321, -4321, 4321
    );

//    debug_halt();

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


//    debug_halt();
    for (usize i = 0; i < context->graphics.size / sizeof(Pixel); ++i)
    {
        context->graphics.base[i] = (Pixel) { 0xFF, 0, 0xFF, 0xFF };
    }


    int value = (u64) context;
    return LETTERS[value][2];
}
