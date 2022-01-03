#include "page_allocator.h"
#include "bit.h"
#include "memory.h"


usize bitmask_is_set(const u8* bitmask, usize index)
{
    usize j = index / 8;
    usize i = index % 8;

    usize result = BIT_CHECK(bitmask[j], i);
    return result;
}

void bitmask_set(u8* bitmask, usize index)
{
    usize j = index / 8;
    usize i = index % 8;

    BIT_SET(bitmask[j], i);
}
void bitmask_unset(u8* bitmask, usize index)
{
    usize j = index / 8;
    usize i = index % 8;

    BIT_CLEAR(bitmask[j], i);
}


PageAllocator page_allocator_new(void* memory, usize size)
{
    PageAllocator allocator = { memory, 0, 0, 0, 0 };

    /* Mark everything as free. */
    usize pages = (size - 1) / PAGE_SIZE + 1;
    for (usize i = 0; i < pages; ++i)
    {
        bitmask_unset(allocator.base, i);
        allocator.pages_free  += 1;
        allocator.pages_total += 1;
    };

    /* Lock pages used by the bitmap. */
    usize bitmask_size = pages / (sizeof(*allocator.base) * 8);
    page_allocator_lock_pages(&allocator, memory, (bitmask_size - 1) / PAGE_SIZE + 1);

    return allocator;
}


void* page_allocator_request_page(PageAllocator* allocator)
{
    for (u64 i = 0; i < allocator->pages_total; ++i)
    {
        if (bitmask_is_set(allocator->base, i))
            continue;

        void* memory = (void*) (allocator->base + i * PAGE_SIZE);
        page_allocator_lock_page(allocator, memory);
        memset(memory, 0, PAGE_SIZE);
        return memory;
    }

    ERROR(INVALID, "Allocator exhausted!");
    return 0; // Page Frame Swap to file
}

static inline usize page_allocator_bitmask_index(PageAllocator* allocator, void* address)
{
    ASSERTF(((usize) address) % PAGE_SIZE == 0 && ((usize) allocator->base) % PAGE_SIZE == 0, "Invalid!");
    return ((usize) address - (usize) allocator->base) / PAGE_SIZE;
}


/// Make page marked as used for conventional usage.
void page_allocator_lock_page(PageAllocator* allocator, void* address)
{
    usize index = page_allocator_bitmask_index(allocator, address);
    ASSERTF(!bitmask_is_set(allocator->base, index), "Page already locked!");
    bitmask_set(allocator->base, index);
    allocator->pages_free -= 1;
    allocator->pages_used += 1;
}

/// Make page available from conventional usage.
void page_allocator_free_page(PageAllocator* allocator, void* address)
{
    usize index = page_allocator_bitmask_index(allocator, address);
    ASSERTF(bitmask_is_set(allocator->base, index), "Page already free!");
    bitmask_unset(allocator->base, index);
    allocator->pages_free += 1;
    allocator->pages_used -= 1;
}

/// Make page marked as used for internal/reserved usage.
void page_allocator_reserve_page(PageAllocator* allocator, void* address)
{
    usize index = page_allocator_bitmask_index(allocator, address);
    ASSERTF(!bitmask_is_set(allocator->base, index), "Page already reserved!");
    bitmask_set(allocator->base, index);
    allocator->pages_free     -= 1;
    allocator->pages_reserved += 1;
}


/// Make page available from internal/reserved usage.
void page_allocator_release_page(PageAllocator* allocator, void* address)
{
    usize index = page_allocator_bitmask_index(allocator, address);
    ASSERTF(bitmask_is_set(allocator->base, index), "Page already released!");
    bitmask_unset(allocator->base, index);
    allocator->pages_free     += PAGE_SIZE;
    allocator->pages_reserved -= PAGE_SIZE;
}




void page_allocator_lock_pages(PageAllocator* allocator, void* address, usize count)
{
    for (usize i = 0; i < count; ++i)
        page_allocator_lock_page(allocator, (void *) ((usize) (address) + i * PAGE_SIZE));
}
void page_allocator_free_pages(PageAllocator* allocator, void* address, usize count)
{
    for (usize i = 0; i < count; ++i)
        page_allocator_free_page(allocator, (void *) ((usize) (address) + i * PAGE_SIZE));
}
void page_allocator_reserve_pages(PageAllocator* allocator, void* address, usize count)
{
    for (usize i = 0; i < count; ++i)
        page_allocator_reserve_page(allocator, (void *) ((usize) (address) + i * PAGE_SIZE));
}
void page_allocator_release_pages(PageAllocator* allocator, void* address, usize count)
{
    for (usize i = 0; i < count; ++i)
        page_allocator_release_page(allocator, (void *) ((usize) (address) + i * PAGE_SIZE));
}