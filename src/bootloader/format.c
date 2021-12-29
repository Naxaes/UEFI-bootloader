#include "../types.h"

enum Error
{
    OK,
    INDEX_OUT_OF_BOUNDS
};

typedef struct HexString {
    char16 data[20]; // L"0x0000000000000000";
} HexString;

typedef struct IntString {
    char16 data[20];
} IntString;


HexString ToHexString(size_t data)
{
    HexString string = {};
    string.data[0] = L'0';
    string.data[1] = L'x';

    u8 x[] = { 56, 48, 40, 32, 24, 16, 8, 0 };
    for (usize i = 0; i < ARRAY_COUNT(x); ++i)
    {
        uintn shift = x[i];
        u64   mask  = 0xFF;

        u8 a = (data & (mask << shift)) >> shift;
        string.data[2 + 2*i] = (a / 16 >= 10) ? L'A' + (a / 16) - 10: L'0' + (a / 16);
        string.data[3 + 2*i] = (a % 16 >= 10) ? L'A' + (a % 16) - 10: L'0' + (a % 16);
    }

    return string;
}


HexString ToHexStringTruncated(size_t data)
{
    HexString string = {};
    string.data[0] = L'0';
    string.data[1] = L'x';

    if (data == 0)
    {
        string.data[2] = L'0';
        return string;
    }

    uintn index       = 0;
    int   found_first = 0;

    u8 x[] = { 56, 48, 40, 32, 24, 16, 8, 0 };
    for (usize i = 0; i < ARRAY_COUNT(x); ++i)
    {
        uintn shift = x[i];
        u64   mask  = 0xFF;

        u8 a = (data & (mask << shift)) >> shift;
        if (a == 0)
        {
            if (found_first)
            {
                string.data[2 + 2*index] = L'0';
                string.data[3 + 2*index] = L'0';
                ++index;
            }
        }
        else
        {
            string.data[2 + 2*index] = (a / 16 >= 10) ? L'A' + (a / 16) - 10: L'0' + (a / 16);
            string.data[3 + 2*index] = (a % 16 >= 10) ? L'A' + (a % 16) - 10: L'0' + (a % 16);
            ++index;

            found_first = 1;
        }

    }

    return string;
}


IntString U64ToString(usize n, usize base)
{
    IntString string = {};
    int i = 0;
    while (1)
    {
        usize reminder = n % base;
        string.data[i++] = (reminder < 10) ? (L'0' + reminder) : (L'A' + reminder - 10);

        if (!(n /= base))
            break;
    }

    string.data[i--] = L'\0';

    for (int j = 0; j < i; j++, i--)
    {
        usize temp = string.data[j];
        string.data[j] = string.data[i];
        string.data[i] = temp;
    }

    return string;
}
