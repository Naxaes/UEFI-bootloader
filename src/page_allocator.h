#pragma once

#include "types.h"
#include "assert.h"

#define PAGE_SIZE 4096


usize bitmask_is_set(const u8* bitmask, usize index);
void  bitmask_set(u8* bitmask, usize index);
void  bitmask_unset(u8* bitmask, usize index);


typedef struct
{
    u8*     base;
    usize   pages_total;
    usize   pages_used;
    usize   pages_free;
    usize   pages_reserved;
} PageAllocator;

PageAllocator page_allocator_new(void* memory, usize size);

void* page_allocator_request_page(PageAllocator* allocator);


/// Make page marked as used for conventional usage.
void page_allocator_lock_page(PageAllocator* allocator, void* address);
void page_allocator_lock_pages(PageAllocator* allocator, void* address, usize count);

/// Make page available from conventional usage.
void page_allocator_free_page(PageAllocator* allocator, void* address);
void page_allocator_free_pages(PageAllocator* allocator, void* address, usize count);

/// Make page marked as used for internal/reserved usage.
void page_allocator_reserve_page(PageAllocator* allocator, void* address);
void page_allocator_reserve_pages(PageAllocator* allocator, void* address, usize count);

/// Make page available from internal/reserved usage.
void page_allocator_release_page(PageAllocator* allocator, void* address);
void page_allocator_release_pages(PageAllocator* allocator, void* address, usize count);

