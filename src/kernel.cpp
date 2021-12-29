#include <stdarg.h>

#include "bootloader.h"
#include "types.h"

#include "memory.c"

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




#define ANSI_COLOR_CODE_NORMAL   "\\e[0m"
#define ANSI_COLOR_CODE_WHITE    "\\e[37m"
#define ANSI_COLOR_CODE_BLACK    "\\e[30m"
#define ANSI_COLOR_CODE_BLUE     "\\e[34m"
#define ANSI_COLOR_CODE_GREEN    "\\e[32m"
#define ANSI_COLOR_CODE_RED      "\\e[31m"
#define ANSI_COLOR_CODE_GREY     "\\e[30;1m"
#define ANSI_COLOR_CODE_YELLOW   "\\e[33m"
#define ANSI_COLOR_CODE_CYAN     "\\e[36m"
#define ANSI_COLOR_CODE_MAGENTA  "\\e[35m"



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

const Pixel BLACK = { 0x00, 0x00, 0x00, 0x00 };
const Pixel WHITE = { 0xFF, 0xFF, 0xFF, 0xFF };
const Pixel BLUE  = { 0xFF, 0x00, 0x00, 0xFF };
const Pixel GREEN = { 0x00, 0xFF, 0x00, 0xFF };
const Pixel RED   = { 0x00, 0x00, 0xFF, 0xFF };


Graphics*  g_graphics   = NULL;
Cursor*    g_cursor     = NULL;
PSF1_Font* g_font       = NULL;
Pixel      g_text_color = { 0xFF, 0xFF, 0xFF, 0xFF };



#define FONT_LETTER_COUNT  95
#define FONT_LETTER_WIDTH  8
#define FONT_LETTER_HEIGHT 13

u8 LETTERS[FONT_LETTER_COUNT][FONT_LETTER_HEIGHT] = {
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // space :32
        {0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18}, // ! :33
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x36, 0x36, 0x36},
        {0x00, 0x00, 0x00, 0x66, 0x66, 0xff, 0x66, 0x66, 0xff, 0x66, 0x66, 0x00, 0x00},
        {0x00, 0x00, 0x18, 0x7e, 0xff, 0x1b, 0x1f, 0x7e, 0xf8, 0xd8, 0xff, 0x7e, 0x18},
        {0x00, 0x00, 0x0e, 0x1b, 0xdb, 0x6e, 0x30, 0x18, 0x0c, 0x76, 0xdb, 0xd8, 0x70},
        {0x00, 0x00, 0x7f, 0xc6, 0xcf, 0xd8, 0x70, 0x70, 0xd8, 0xcc, 0xcc, 0x6c, 0x38},
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x1c, 0x0c, 0x0e},
        {0x00, 0x00, 0x0c, 0x18, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x18, 0x0c},
        {0x00, 0x00, 0x30, 0x18, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x18, 0x30},
        {0x00, 0x00, 0x00, 0x00, 0x99, 0x5a, 0x3c, 0xff, 0x3c, 0x5a, 0x99, 0x00, 0x00},
        {0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0xff, 0xff, 0x18, 0x18, 0x18, 0x00, 0x00},
        {0x00, 0x00, 0x30, 0x18, 0x1c, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0x00, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x60, 0x60, 0x30, 0x30, 0x18, 0x18, 0x0c, 0x0c, 0x06, 0x06, 0x03, 0x03},
        {0x00, 0x00, 0x3c, 0x66, 0xc3, 0xe3, 0xf3, 0xdb, 0xcf, 0xc7, 0xc3, 0x66, 0x3c},
        {0x00, 0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x78, 0x38, 0x18},
        {0x00, 0x00, 0xff, 0xc0, 0xc0, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x03, 0xe7, 0x7e},
        {0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x07, 0x7e, 0x07, 0x03, 0x03, 0xe7, 0x7e},
        {0x00, 0x00, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0xff, 0xcc, 0x6c, 0x3c, 0x1c, 0x0c},
        {0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x07, 0xfe, 0xc0, 0xc0, 0xc0, 0xc0, 0xff},
        {0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xc7, 0xfe, 0xc0, 0xc0, 0xc0, 0xe7, 0x7e},
        {0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x18, 0x0c, 0x06, 0x03, 0x03, 0x03, 0xff},
        {0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xe7, 0x7e, 0xe7, 0xc3, 0xc3, 0xe7, 0x7e},
        {0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x03, 0x7f, 0xe7, 0xc3, 0xc3, 0xe7, 0x7e},
        {0x00, 0x00, 0x00, 0x38, 0x38, 0x00, 0x00, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0x30, 0x18, 0x1c, 0x1c, 0x00, 0x00, 0x1c, 0x1c, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0x06, 0x0c, 0x18, 0x30, 0x60, 0xc0, 0x60, 0x30, 0x18, 0x0c, 0x06},
        {0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60},
        {0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x18, 0x0c, 0x06, 0x03, 0xc3, 0xc3, 0x7e},
        {0x00, 0x00, 0x3f, 0x60, 0xcf, 0xdb, 0xd3, 0xdd, 0xc3, 0x7e, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xff, 0xc3, 0xc3, 0xc3, 0x66, 0x3c, 0x18},
        {0x00, 0x00, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe},
        {0x00, 0x00, 0x7e, 0xe7, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xe7, 0x7e},
        {0x00, 0x00, 0xfc, 0xce, 0xc7, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc7, 0xce, 0xfc},
        {0x00, 0x00, 0xff, 0xc0, 0xc0, 0xc0, 0xc0, 0xfc, 0xc0, 0xc0, 0xc0, 0xc0, 0xff},
        {0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xfc, 0xc0, 0xc0, 0xc0, 0xff},
        {0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xcf, 0xc0, 0xc0, 0xc0, 0xc0, 0xe7, 0x7e},
        {0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xff, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3},
        {0x00, 0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7e},
        {0x00, 0x00, 0x7c, 0xee, 0xc6, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06},
        {0x00, 0x00, 0xc3, 0xc6, 0xcc, 0xd8, 0xf0, 0xe0, 0xf0, 0xd8, 0xcc, 0xc6, 0xc3},
        {0x00, 0x00, 0xff, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0},
        {0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xdb, 0xff, 0xff, 0xe7, 0xc3},
        {0x00, 0x00, 0xc7, 0xc7, 0xcf, 0xcf, 0xdf, 0xdb, 0xfb, 0xf3, 0xf3, 0xe3, 0xe3},
        {0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xe7, 0x7e},
        {0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe},
        {0x00, 0x00, 0x3f, 0x6e, 0xdf, 0xdb, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x66, 0x3c},
        {0x00, 0x00, 0xc3, 0xc6, 0xcc, 0xd8, 0xf0, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe},
        {0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x07, 0x7e, 0xe0, 0xc0, 0xc0, 0xe7, 0x7e},
        {0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xff},
        {0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3},
        {0x00, 0x00, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3},
        {0x00, 0x00, 0xc3, 0xe7, 0xff, 0xff, 0xdb, 0xdb, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3},
        {0x00, 0x00, 0xc3, 0x66, 0x66, 0x3c, 0x3c, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3},
        {0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3},
        {0x00, 0x00, 0xff, 0xc0, 0xc0, 0x60, 0x30, 0x7e, 0x0c, 0x06, 0x03, 0x03, 0xff},
        {0x00, 0x00, 0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c},
        {0x00, 0x03, 0x03, 0x06, 0x06, 0x0c, 0x0c, 0x18, 0x18, 0x30, 0x30, 0x60, 0x60},
        {0x00, 0x00, 0x3c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3c},
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc3, 0x66, 0x3c, 0x18},
        {0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x38, 0x30, 0x70},
        {0x00, 0x00, 0x7f, 0xc3, 0xc3, 0x7f, 0x03, 0xc3, 0x7e, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0xfe, 0xc3, 0xc3, 0xc3, 0xc3, 0xfe, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0},
        {0x00, 0x00, 0x7e, 0xc3, 0xc0, 0xc0, 0xc0, 0xc3, 0x7e, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0x7f, 0xc3, 0xc3, 0xc3, 0xc3, 0x7f, 0x03, 0x03, 0x03, 0x03, 0x03},
        {0x00, 0x00, 0x7f, 0xc0, 0xc0, 0xfe, 0xc3, 0xc3, 0x7e, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x30, 0xfc, 0x30, 0x30, 0x30, 0x33, 0x1e},
        {0x7e, 0xc3, 0x03, 0x03, 0x7f, 0xc3, 0xc3, 0xc3, 0x7e, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xfe, 0xc0, 0xc0, 0xc0, 0xc0},
        {0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x18, 0x00},
        {0x38, 0x6c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x00, 0x00, 0x0c, 0x00},
        {0x00, 0x00, 0xc6, 0xcc, 0xf8, 0xf0, 0xd8, 0xcc, 0xc6, 0xc0, 0xc0, 0xc0, 0xc0},
        {0x00, 0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x78},
        {0x00, 0x00, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xfe, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xfc, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00},
        {0xc0, 0xc0, 0xc0, 0xfe, 0xc3, 0xc3, 0xc3, 0xc3, 0xfe, 0x00, 0x00, 0x00, 0x00},
        {0x03, 0x03, 0x03, 0x7f, 0xc3, 0xc3, 0xc3, 0xc3, 0x7f, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xe0, 0xfe, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0xfe, 0x03, 0x03, 0x7e, 0xc0, 0xc0, 0x7f, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0x1c, 0x36, 0x30, 0x30, 0x30, 0x30, 0xfc, 0x30, 0x30, 0x30, 0x00},
        {0x00, 0x00, 0x7e, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3, 0xc3, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0xc3, 0xe7, 0xff, 0xdb, 0xc3, 0xc3, 0xc3, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0xc3, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0xc3, 0x00, 0x00, 0x00, 0x00},
        {0xc0, 0x60, 0x60, 0x30, 0x18, 0x3c, 0x66, 0x66, 0xc3, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0xff, 0x60, 0x30, 0x18, 0x0c, 0x06, 0xff, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0x0f, 0x18, 0x18, 0x18, 0x38, 0xf0, 0x38, 0x18, 0x18, 0x18, 0x0f},
        {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18},
        {0x00, 0x00, 0xf0, 0x18, 0x18, 0x18, 0x1c, 0x0f, 0x1c, 0x18, 0x18, 0x18, 0xf0},
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x8f, 0xf1, 0x60, 0x00, 0x00, 0x00}
};



//static const u16 VGA_CONTROL_REGISTER = 0x3d4;
//static const u16 VGA_DATA_REGISTER    = 0x3d5;

inline u32 abs32(i32 n)
{
    u32 mask   = n >> 31;
    u32 result = (mask^n) - mask;
    return result;
}
inline u64 abs64(i64 n)
{
    u64 mask   = n >> 63;
    u64 result = (mask^n) - mask;
    return result;
}

inline int strcmp(const char* a, const char* b)
{
    while (*a != '\0' && *b != '\0')
    {
        if (*a < *b) return -1;
        if (*a > *b) return  1;
        ++a;
        ++b;
    }
    return 0;
}


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


inline void draw(Pixel pixel, int row, int col)
{
    g_graphics->base[g_graphics->pixels_per_scanline * row + col] = pixel;
}


char* usize_to_string(char* buffer, usize n, usize base)
{
    int i = 0;
    while (1)
    {
        usize reminder = n % base;
        buffer[i++] = (reminder < 10) ? ('0' + reminder) : ('A' + reminder - 10);

        if (!(n /= base))
            break;
    }

    buffer[i--] = '\0';

    for (int j = 0; j < i; j++, i--)
    {
        char temp = buffer[j];
        buffer[j] = buffer[i];
        buffer[i] = temp;
    }

    return buffer;
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


void debug_halt()
{
    // Change to 0 in debugger to continue.
    int volatile wait = 1;
    while (wait) {
        __asm__ __volatile__("pause");
    }
}



void fill(Pixel pixel)
{
    for (usize i = 0; i < g_graphics->size / sizeof(Pixel); ++i)
        g_graphics->base[i] = pixel;
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


const intn PAGE_SIZE = 4096;


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
    uintn entries = memory->MemoryMapSize / memory->DescriptorSize;
    uintn base    = (uintn) memory->MemoryMap;

    u8*   largest_memory_segment = NULL;
    usize largest_memory_size    = 0;

    usize total_memory_size = 0;
    usize total_free_memory_size = 0;
    usize total_reserved_memory_size = 0;

    for (uintn i = 0; i < entries; ++i)
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

    debug_halt();

    return allocator;
}


/* ---- GDT (Global Descriptor Table ---- */




//
///* ---- Interrupt Descriptor Table ---- */
//// https://wiki.osdev.org/Exceptions
//typedef struct {
//    u16 offset_1;               /* Offset bits 0-15 of the handler.          */
//    u16 selector;               /* A code segment selector in GDT or LDT.    */
//    u16 options;                /* See below.                                */
//    u16 offset_2;               /* Offset bits 16-31 of the handler.         */
//    u32 offset_3;               /* Offset bits 32-63 of the handler.         */
//    u32 reserved;
//} __attribute__((packed)) InterruptDescriptor;
///*  The options field.
//Bits  |   Name               |           Description
//--------------------------------------------------------------------------------
//0-2	  | Interrupt Stack      |  0: Don't switch stacks, 1-7: Switch to the n-th
//      | Table Index	         |  stack in the Interrupt Stack Table when this
//      |                      |  handler is called.
//3-7	  | Reserved             |
//8	  | 0: Interrupt Gate,   |  If this bit is 0, interrupts are disabled
//      | 1: Trap Gate	     |  when this handler is called.
//9-11  |                      |  Must be one
//12	  |                      |  Must be zero
//13‑14 | Descriptor Privilege |  The minimal privilege level required for
//      |   Level (DPL)	     |  calling this handler.
//15	  | Present              |
//*/
//typedef struct {
//    uint16_t	limit;
//    uint64_t	base;
//} __attribute__((packed)) idtr_t;
//
//
//__attribute__((aligned(0x10)))
//static InterruptDescriptor g_interrupt_descriptor_table[256];
//static idtr_t idtr;
//
//__attribute__((noreturn))
//void exception_handler()
//{
//    __asm__ volatile ("cli; hlt");
//}
//
//void idt_set_descriptor(u8 vector, void* isr, u8 flags)
//{
//    InterruptDescriptor* descriptor = &g_interrupt_descriptor_table[vector];
//
//    descriptor->offset_1  = (uint64_t)isr & 0xFFFF;
//    descriptor->selector  = GDT_OFFSET_KERNEL_CODE;
//    descriptor->options   = (u16) flags;
//    descriptor->offset_2  = ((u64)isr >> 16) & 0xFFFF;
//    descriptor->offset_3  = ((u64)isr >> 32) & 0xFFFFFFFF;
//    descriptor->reserved  = 0;
//}
//
//void idt_init()
//{
//    idtr.base  = (uintptr_t) &g_interrupt_descriptor_table[0];
//    idtr.limit = (u16) sizeof(InterruptDescriptor) * IDT_MAX_DESCRIPTORS - 1;
//
//    for (u8 vector = 0; vector < 32; vector++)
//    {
//        idt_set_descriptor(vector, tab[vector], 0x8E);
//        vectors[vector] = true;
//    }
//
//    __asm__ volatile ("lidt %0" : : "m"(idtr)); // load the new IDT
//    __asm__ volatile ("sti"); // set the interrupt flag
//}



extern "C" int _start(Context* context)
{
    // ---- INITIALIZATION START ---
    Cursor cursor = { 0, 0 };

    g_cursor   = &cursor;
    g_graphics = &context->graphics;
    g_font     = &context->font;






    // ---- INITIALIZATION STOP ---
    fill(BLACK);

    PageAllocator allocator = memory_map(&context->memory);

    printf("Allocator: { base=%x, size=%x }\n", allocator.base, allocator.size);

//    print_memory_map(&context->memory);



//    ASSERT(0, "Trying assertions and %s at line %d!", "other stuff", __LINE__);

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
