#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>

#include "../src/types.h"
#include "../src/format.c"



Block Allocate(size_t size)
{
    Block memory = { .data = malloc(size), .size = size };
    return memory;
}

void Deallocate(Block block)
{
    free(block.data);
}


int main()
{
    printf("-- Testing ToHexString --\n");
    printf("%s\n", ToHexString(0 - 1).data);
    printf("%s\n", ToHexString(0xFF00000000000000).data);
    printf("%s\n", ToHexString(0x0123456789ABCDEF).data);
    printf("%s\n", ToHexString(0xFEDCBA9876543210).data);
    printf("%s\n", ToHexString(0x00000000F0FFFF00).data);

    printf("-- Testing ToHexStringTruncated --\n");
    printf("%s\n", ToHexStringTruncated(0 - 1).data);
    printf("%s\n", ToHexStringTruncated(0xFF00000000000000).data);
    printf("%s\n", ToHexStringTruncated(0x0123456789ABCDEF).data);
    printf("%s\n", ToHexStringTruncated(0xFEDCBA9876543210).data);
    printf("%s\n", ToHexStringTruncated(0x00000000F0FFFF00).data);


    printf("-- Testing --\n");

    Allocator allocator = { .Allocate = Allocate, .Deallocate = Deallocate };

    String ascii = LITERAL_TO_STRING("Hello world!\n");
    int*   utf16 = (int *) AsciiToUtf16(ascii, allocator).data;
    wprintf("%ls\n", utf16);


    char16 data[16];
    WString buffer = {.data=data, .size=16};
    U64ToString(1234, buffer, 10);
}