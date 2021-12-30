


PageAllocator memory_map(const Memory* memory)
{
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
