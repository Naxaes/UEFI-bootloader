// The GNU C Compiler in freestanding mode requires the existence of the symbols
// memset, memcpy, memmove and memcmp. You *must* supply these functions
// yourself as described in the C standard. You cannot and should not (if you
// could) prevent the compiler from assuming these functions exist as the
// compiler uses them for important optimizations.
#include "memory.h"


void* memcpy(void* destination, const void* source, size_t size)
{
    for (size_t i = 0; i < size; ++i)
        ((u8*) destination)[i] = ((const u8*) source)[i];
    return destination;
}

void* memset(void* source, int value, size_t size)
{
    u8* pointer = (u8 *) source;
    while (size--)
        *pointer++ = (unsigned char) value;
    return source;
}
