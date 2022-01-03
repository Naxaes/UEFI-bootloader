// The GNU C Compiler in freestanding mode requires the existence of the symbols
// memset, memcpy, memmove and memcmp. You *must* supply these functions
// yourself as described in the C standard. You cannot and should not (if you
// could) prevent the compiler from assuming these functions exist as the
// compiler uses them for important optimizations.
#include <stddef.h>

void* memcpy(void* destination, const void* source, size_t size)
{
    u8* dest = (u8 *) destination;
    const u8* src = (const u8 *) source;

    for (size_t i = 0; i < size; ++i)
        dest[i] = src[i];
    return destination;
}

void* memset(void* destination, int value, size_t size)
{
    u8* dest = (u8 *) destination;

    for (size_t i = 0; i < size; ++i)
        dest[i] = (i8) value;
    return destination;
}
