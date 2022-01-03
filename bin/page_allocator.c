#include <stdlib.h>
#include "../src/allocator.c"


void debug_break()
{

}

typedef struct
{
    int x;
} Temp __attribute__((aligned(PAGE_SIZE)));

int main()
{
    usize memory_size = PAGE_SIZE * 128;
    Temp* temp        = (Temp*) malloc(memory_size);
    void* memory      = (void *) temp;

    PageAllocator allocator = page_allocator_new(memory, memory_size);

    printf(
            "Total memory:    %zu Kib\n"
            "Free memory:     %zu Kib\n"
            "Reserved memory: %zu Kib\n"
            "Used memory:     %zu Kib\n\n",
            (allocator.pages_total * PAGE_SIZE) / 1024, (allocator.pages_free * PAGE_SIZE) / 1024, (allocator.pages_reserved * PAGE_SIZE) / 1024, (allocator.pages_used * PAGE_SIZE) / 1024
    );


    PageTable* pml4 = page_allocator_request_page(&allocator);
    page_table_identity_map(pml4, &allocator);

    printf(
            "Total memory:    %zu Kib\n"
            "Free memory:     %zu Kib\n"
            "Reserved memory: %zu Kib\n"
            "Used memory:     %zu Kib\n\n",
            (allocator.pages_total * PAGE_SIZE) / 1024, (allocator.pages_free * PAGE_SIZE) / 1024, (allocator.pages_reserved * PAGE_SIZE) / 1024, (allocator.pages_used * PAGE_SIZE) / 1024
    );
}