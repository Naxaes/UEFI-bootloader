#include "bit.h"
#include "page_allocator.h"

#define PAGE_ENTRY_PRESENT_SET(data)                BIT_SET(data, 0)
#define PAGE_ENTRY_READ_WRITE_SET(data)             BIT_SET(data, 1)
#define PAGE_ENTRY_SUPER_USER_SET(data)             BIT_SET(data, 2)
#define PAGE_ENTRY_WRITE_THROUGH_SET(data)          BIT_SET(data, 3)
#define PAGE_ENTRY_CACHE_DISABLED_SET(data)         BIT_SET(data, 4)
#define PAGE_ENTRY_ACCESSED_SET(data)               BIT_SET(data, 5)
#define PAGE_ENTRY_LARGER_PAGES_SET(data)           BIT_SET(data, 7)
#define PAGE_ENTRY_AVAILABLE_SET(data, value)       ((data) |= ((((value) & 0b111) << 9)))
#define PAGE_ENTRY_ADDRESS_SET(data, address)       ((data) |= (((u64) (address)) >> 12))

#define PAGE_ENTRY_PRESENT_IS_SET(data)             BIT_CHECK(data, 0)
#define PAGE_ENTRY_READ_WRITE_IS_SET(data)          BIT_CHECK(data, 1)
#define PAGE_ENTRY_SUPER_USER_IS_SET(data)          BIT_CHECK(data, 2)
#define PAGE_ENTRY_WRITE_THROUGH_IS_SET(data)       BIT_CHECK(data, 3)
#define PAGE_ENTRY_CACHE_DISABLED_IS_SET(data)      BIT_CHECK(data, 4)
#define PAGE_ENTRY_ACCESSED_IS_SET(data)            BIT_CHECK(data, 5)
#define PAGE_ENTRY_LARGER_PAGES_IS_SET(data)        BIT_CHECK(data, 7)
#define PAGE_ENTRY_ADDRESS_GET(data)                ((((u64) (data)) << 12) & 0x000FFFFFFFFFFFFF)


typedef struct
{
    u16 level_0;
    u16 level_1;
    u16 level_2;
    u16 level_3;
} PageIndex;


PageIndex map_virtual_address(u64 virtual_address)
{
    PageIndex page_index = { 0 };
    virtual_address >>= 12;
    page_index.level_0 = virtual_address & 0x1FF;
    virtual_address >>= 9;
    page_index.level_1 = virtual_address & 0x1FF;
    virtual_address >>= 9;
    page_index.level_2 = virtual_address & 0x1FF;
    virtual_address >>= 9;
    page_index.level_3 = virtual_address & 0x1FF;
    return page_index;
}


typedef struct
{
    u64 data;
} PageEntry;

typedef struct PageTable
{
    PageEntry entries[512];
} __attribute__((aligned(PAGE_SIZE))) PageTable;

void* memset(void* source, int value, size_t size);

void map_memory(PageTable* pml4, PageAllocator* allocator, u64 virtual_address, u64 physical_address)
{
    PageIndex index = map_virtual_address(virtual_address);
    PageEntry entry = { 0 };

    LOGF("Mapping %x to %x with index %d - %d - %d - %d\r", virtual_address, physical_address, index.level_3, index.level_2, index.level_1, index.level_0);

    PageTable* page_directory_pointer = 0;
    entry = pml4->entries[index.level_3];
    if (!entry.data)
    {
        page_directory_pointer = page_allocator_request_page(allocator);

        PAGE_ENTRY_PRESENT_SET(entry.data);
        PAGE_ENTRY_READ_WRITE_SET(entry.data);
        PAGE_ENTRY_ADDRESS_SET(entry.data, page_directory_pointer);

        LOGF("Level 3 address set %x - %x\r", (usize) page_directory_pointer, PAGE_ENTRY_ADDRESS_GET(entry.data));
        pml4->entries[index.level_3] = entry;
    }
    else
    {
        LOGF("Level 3 address got %x - %x\r", (usize) page_directory_pointer, PAGE_ENTRY_ADDRESS_GET(entry.data));
        page_directory_pointer = (PageTable*) PAGE_ENTRY_ADDRESS_GET(entry.data);
    }


    PageTable* page_directory = 0;
    entry = page_directory_pointer->entries[index.level_2];
    if (!entry.data)
    {
        page_directory = page_allocator_request_page(allocator);

        PAGE_ENTRY_PRESENT_SET(entry.data);
        PAGE_ENTRY_READ_WRITE_SET(entry.data);
        PAGE_ENTRY_ADDRESS_SET(entry.data, page_directory);

        LOGF("Level 2 address set %x - %x\r", (usize) page_directory, PAGE_ENTRY_ADDRESS_GET(entry.data));
        page_directory_pointer->entries[index.level_2] = entry;
    }
    else
    {
        LOGF("Level 2 address got %x - %x\r", (usize) page_directory, PAGE_ENTRY_ADDRESS_GET(entry.data));
        page_directory = (PageTable*) PAGE_ENTRY_ADDRESS_GET(entry.data);
    }


    PageTable* page_table = 0;
    entry = page_directory->entries[index.level_1];
    if (!entry.data)
    {
        page_table = page_allocator_request_page(allocator);

        PAGE_ENTRY_PRESENT_SET(entry.data);
        PAGE_ENTRY_READ_WRITE_SET(entry.data);
        PAGE_ENTRY_ADDRESS_SET(entry.data, page_table);

        LOGF("Level 1 address set %x - %x\r", (usize) page_table, PAGE_ENTRY_ADDRESS_GET(entry.data));
        page_directory->entries[index.level_1] = entry;
    }
    else
    {
        LOGF("Level 1 address got %x - %x\r", (usize) page_table, PAGE_ENTRY_ADDRESS_GET(entry.data));
        page_table = (PageTable*) PAGE_ENTRY_ADDRESS_GET(entry.data);
    }

    entry = page_table->entries[index.level_0];
    PAGE_ENTRY_PRESENT_SET(entry.data);
    PAGE_ENTRY_READ_WRITE_SET(entry.data);
    PAGE_ENTRY_ADDRESS_SET(entry.data, physical_address);
    page_table->entries[index.level_0] = entry;
    LOGF("Level 0 address got %x\r", PAGE_ENTRY_ADDRESS_GET(entry.data));
}



void page_table_identity_map(PageTable* pml4, PageAllocator* allocator)
{
    for (usize i = 0; i < allocator->pages_total; ++i)
    {
        map_memory(pml4, allocator, i * PAGE_SIZE, i * PAGE_SIZE);
    }
}