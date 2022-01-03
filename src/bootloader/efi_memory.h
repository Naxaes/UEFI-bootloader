#include "../preamble.h"

#define PAGE_SIZE 4096


typedef struct
{
    u8*   data;
    usize size;
} Bitmask;

Bitmask bitmask_new(u8* buffer, usize size)
{
    return (Bitmask) { buffer, size };
}

usize bitmask_is_set(const Bitmask* bitmask, usize index)
{
    usize j = index / 8;
    usize i = index % 8;

    usize result = BIT_CHECK(bitmask->data[j], i);
    return result;
}

void bitmask_set(Bitmask* bitmask, usize index)
{
    usize j = index / 8;
    usize i = index % 8;

    BIT_SET(bitmask->data[j], i);
}
void bitmask_unset(Bitmask* bitmask, usize index)
{
    usize j = index / 8;
    usize i = index % 8;

    BIT_CLEAR(bitmask->data[j], i);
}


typedef struct
{
    Bitmask header;
    u8*     base;
    usize   size;
    usize   total_memory;
    usize   used_memory;
    usize   free_memory;
    usize   reserved_memory;
} PageAllocator;



/// Make page marked as used for conventional usage.
void page_allocator_lock_page(PageAllocator* allocator, void* address)
{
    usize index = (usize) (address) / PAGE_SIZE;
    ASSERTF(bitmask_is_set(&allocator->header, index), L"Page already locked!");
    bitmask_set(&allocator->header, index);
    allocator->free_memory -= PAGE_SIZE;
    allocator->used_memory += PAGE_SIZE;
}
void page_allocator_lock_pages(PageAllocator* allocator, void* address, usize count)
{
    for (usize i = 0; i < count; ++i)
        page_allocator_lock_page(allocator, (void *) ((usize) (address) + count * PAGE_SIZE));
}

/// Make page available from conventional usage.
void page_allocator_free_page(PageAllocator* allocator, void* address)
{
    usize index = (usize) (address) / PAGE_SIZE;
    ASSERTF(!bitmask_is_set(&allocator->header, index), L"Page already free!");
    bitmask_unset(&allocator->header, index);
    allocator->free_memory += PAGE_SIZE;
    allocator->used_memory -= PAGE_SIZE;
}
void page_allocator_free_pages(PageAllocator* allocator, void* address, usize count)
{
    for (usize i = 0; i < count; ++i)
        page_allocator_free_page(allocator, (void *) ((usize) (address) + count * PAGE_SIZE));
}

/// Make page marked as used for internal/reserved usage.
void page_allocator_reserve_page(PageAllocator* allocator, void* address)
{
    usize index = (usize) (address) / PAGE_SIZE;
    ASSERTF(bitmask_is_set(&allocator->header, index), L"Page already reserved!");
    bitmask_set(&allocator->header, index);
    allocator->free_memory     -= PAGE_SIZE;
    allocator->reserved_memory += PAGE_SIZE;
}
void page_allocator_reserve_pages(PageAllocator* allocator, void* address, usize count)
{
    for (usize i = 0; i < count; ++i)
        page_allocator_reserve_page(allocator, (void *) ((usize) (address) + count * PAGE_SIZE));
}

/// Make page available from internal/reserved usage.
void page_allocator_release_page(PageAllocator* allocator, void* address)
{
    usize index = (usize) (address) / PAGE_SIZE;
    ASSERTF(!bitmask_is_set(&allocator->header, index), L"Page already released!");
    bitmask_unset(&allocator->header, index);
    allocator->free_memory     += PAGE_SIZE;
    allocator->reserved_memory -= PAGE_SIZE;
}
void page_allocator_release_pages(PageAllocator* allocator, void* address, usize count)
{
    for (usize i = 0; i < count; ++i)
        page_allocator_release_page(allocator, (void *) ((usize) (address) + count * PAGE_SIZE));
}

