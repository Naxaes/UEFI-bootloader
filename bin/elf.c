#include "../src/elf.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

int load_elf64(const uint8_t* data)
{
    if (!is_elf64(data))
        return ELF_ERROR;

    uint64_t page_size = getpagesize();

    const Elf64Header*        header   = (Elf64Header*) data;
    const Elf64ProgramHeader* programs = (Elf64ProgramHeader*) (data + header->program_header_offset);
    const Elf64SectionHeader* section  = (Elf64SectionHeader*) (data + header->section_header_offset);

    // https://wiki.osdev.org/ELF
    for (int i = 0; i < header->program_header_entries; ++i)
    {
        const Elf64ProgramHeader* program = &programs[i];

        if (program->type == PT_LOAD)
        {
            // Allocate `size` virtual memory at `address` and copy from
            // source to destination.
            // If the file_size and memory_size members differ,
            // the segment is padded with zeros.

            uint64_t destination = program->virtual_address;
            uint64_t dest_size   = program->memory_size;

            const uint8_t* source = data + program->file_offset;
            uint64_t source_size  = program->file_size;

            uint64_t aligned    = destination;
            uint64_t align_size = dest_size;
            if (aligned % page_size != 0)
            {
                aligned    = aligned & (~(page_size-1));
                align_size = (destination + dest_size) - aligned;
            }

            void* result = mmap(
                    (void *) aligned, align_size,
                    PROT_READ | PROT_WRITE | PROT_EXEC,
                    MAP_PRIVATE | MAP_ANONYMOUS,
                    -1, 0
            );

            assert(result != MAP_FAILED);
            assert((uint64_t) result == aligned);
            memcpy((void *)destination, source, source_size);
        }
    }

    typedef int (*elf_main_fn)(int*);

    void* entry_point_address = (void *) header->entry_point;
    elf_main_fn entry_point = (elf_main_fn) entry_point_address;

    int temp = 1;
    int result = entry_point(&temp);
    return result;
}











int main()
{
    FILE* file = fopen("../../build/kernel", "rb");
    char* source = NULL;
    long  size   = 0;
    if (file)
    {
        if (fseek(file, 0L, SEEK_END) == 0)
        {
            size = ftell(file);
            if (size == -1)
                return -1;

            /* Allocate our buffer to that size. */
            source = malloc(sizeof(char) * (size + 1));
            if (!source)
                return -1;

            /* Go back to the start of the file. */
            if (fseek(file, 0L, SEEK_SET) != 0)
                return -1;

            /* Read the entire file into memory. */
            size_t actual_size = fread(source, sizeof(char), size, file);
            if (ferror(file) != 0)
                fputs("Error reading file", stderr);
            else
                source[actual_size] = '\0';
        }
    }

    return load_elf64((uint8_t*)source);
}






