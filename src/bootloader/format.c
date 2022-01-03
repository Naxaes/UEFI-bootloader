#include "types.h"
#include "array.h"


enum Error
{
    OK,
    INDEX_OUT_OF_BOUNDS
};

typedef struct
{
    u16 data[20];  // L"0x0000000000000000";
} WideStringArray20;


WideStringArray20 ToHexString(size_t data)
{
    WideStringArray20 string = {};
    string.data[0] = L'0';
    string.data[1] = L'x';

    u8 x[] = { 56, 48, 40, 32, 24, 16, 8, 0 };
    for (usize i = 0; i < ARRAY_COUNT(x); ++i)
    {
        usize shift = x[i];
        u64   mask  = 0xFF;

        u8 a = (u8) ((data & (mask << shift)) >> shift);
        string.data[2 + 2*i] = (a / 16 >= 10) ? L'A' + (a / 16) - 10: L'0' + (a / 16);
        string.data[3 + 2*i] = (a % 16 >= 10) ? L'A' + (a % 16) - 10: L'0' + (a % 16);
    }

    return string;
}


WideStringArray20 ToHexStringTruncated(size_t data)
{
    WideStringArray20 string = {};
    string.data[0] = L'0';
    string.data[1] = L'x';

    if (data == 0)
    {
        string.data[2] = L'0';
        return string;
    }

    usize index       = 0;
    int   found_first = 0;

    u8 x[] = { 56, 48, 40, 32, 24, 16, 8, 0 };
    for (usize i = 0; i < ARRAY_COUNT(x); ++i)
    {
        usize shift = x[i];
        u64   mask  = 0xFF;

        u8 a = (u8) ((data & (mask << shift)) >> shift);
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


WideStringArray20 U64ToString(usize n, usize base)
{
    WideStringArray20 string = {};
    int i = 0;
    while (1)
    {
        usize reminder = n % base;
        string.data[i++] = (reminder < 10) ? (L'0' + (u16) reminder) : (L'A' + (u16) reminder - 10);

        if (!(n /= base))
            break;
    }

    string.data[i--] = L'\0';

    for (int j = 0; j < i; j++, i--)
    {
        u16 temp = string.data[j];
        string.data[j] = string.data[i];
        string.data[i] = temp;
    }

    return string;
}
