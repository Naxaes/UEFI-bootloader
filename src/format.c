#include "types.h"

typedef struct HexArray {
    char8 data[20]; // L"0x0000000000000000";
} HexArray;


HexArray ToHexString(size_t data)
{
    HexArray string = {};
    string.data[0] = '0';
    string.data[1] = 'x';

    u8 x[] = { 56, 48, 40, 32, 24, 16, 8, 0 };
    for (usize i = 0; i < ARRAY_COUNT(x); ++i)
    {
        uintn shift = x[i];
        u64   mask  = 0xFF;

        u8 a = (data & (mask << shift)) >> shift;
        string.data[2 + 2*i] = (a / 16 >= 10) ? 'A' + (a / 16) - 10: '0' + (a / 16);
        string.data[3 + 2*i] = (a % 16 >= 10) ? 'A' + (a % 16) - 10: '0' + (a % 16);
    }

    return string;
}


WString AsciiToUtf16(String ascii, Allocator allocator)
{
    Block memory  = allocator.Allocate(2 * ascii.size);
    WString utf16 = { .data = memory.data, .size = memory.size };

    for (int i = 0; i < ascii.size; ++i)
    {
        utf16.data[i] = ascii.data[i];
    }

    return utf16;
}