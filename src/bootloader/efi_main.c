#include "efi.h"
#include "efi_lib.h"
#include "efi_error.h"

#include "../bootloader.h"
#include "../allocator.c"
#include "../x86_64/idt.c"

#include "elf.h"
#include "memory.c"

/* Used internally by GCC */
void abort()
{
    EfiPrintF(L"Process aborted!");
    EfiHalt();
}


// https://github.com/torvalds/linux/blob/5bfc75d92efd494db37f5c4c173d3639d4772966/drivers/firmware/efi/libstub/x86-stub.c
EFI_SYSTEM_TABLE*      g_SystemTable;
EFI_BOOT_SERVICES*     g_BootServices;
EFI_RUNTIME_SERVICES*  g_RuntimeServices;
Graphics               g_Graphics;


PageAllocator page_allocator_new_from_memory_map(const Memory* memory)
{
    usize entries = memory->MemoryMapSize / memory->DescriptorSize;
    usize base    = (u64) memory->MemoryMap;

    void* largest_memory_segment = NULL;
    usize largest_memory_size    = 0;

    for (usize i = 0; i < entries; ++i)
    {
        EFI_MEMORY_DESCRIPTOR* descriptor = (EFI_MEMORY_DESCRIPTOR *)(base + memory->DescriptorSize * i);
        usize memory_size = descriptor->NumberOfPages * PAGE_SIZE;

        if (descriptor->Type == EfiConventionalMemory)
        {
            if (memory_size > largest_memory_size)
            {
                largest_memory_segment = (void *) descriptor->PhysicalStart;
                largest_memory_size    = memory_size;
            }
        }
    }

    PageAllocator allocator = page_allocator_new(largest_memory_segment, largest_memory_size);
    return allocator;
}







EFI_STATUS EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable)
{
    /* ---- INITIALIZE STATICS ----
     * The System Table contains pointers to other standard tables that a loaded
     * image may use if the associated pointers are initialized to nonzero values.
     */
    g_SystemTable     = SystemTable;
    g_BootServices    = SystemTable->BootServices;
    g_RuntimeServices = SystemTable->RuntimeServices;


    /* ---- INITIALIZE SCREEN ----
     * UEFI-spec page 451
     * The Reset() function resets the text output device hardware.
     * The cursor position is set to (0, 0), and the screen is cleared to the
     * default background color for the output device.
     */
    EFI_ASSERT(g_SystemTable->ConOut->Reset(g_SystemTable->ConOut, EFI_TRUE));
    EFI_ASSERT(g_SystemTable->ConOut->SetAttribute(g_SystemTable->ConOut, EFI_CYAN));


    /* ---- INITIALIZE KEYBOARD ----
     * The implementation of Reset is required to clear the contents of any
     * input queues resident in memory used for buffering keystroke data and
     * put the input stream in a known empty state.
     */
    EFI_ASSERT(g_SystemTable->ConIn->Reset(g_SystemTable->ConIn, EFI_TRUE));


    /* ---- INITIALIZE GRAPHICS ---- */
    EFI_GRAPHICS_OUTPUT_PROTOCOL* GraphicsOutput = NULL;
    EFI_ASSERT(g_BootServices->LocateProtocol(&EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID, NULL, (void**) &GraphicsOutput));

    ASSERTF(GraphicsOutput != NULL, "Couldn't initialize Graphics!\r");

    g_Graphics.base   = (Pixel *) GraphicsOutput->Mode->FrameBufferBase;
    g_Graphics.size   = GraphicsOutput->Mode->FrameBufferSize;
    g_Graphics.width  = GraphicsOutput->Mode->Info->HorizontalResolution;
    g_Graphics.height = GraphicsOutput->Mode->Info->VerticalResolution;
    g_Graphics.pixels_per_scanline = GraphicsOutput->Mode->Info->PixelsPerScanLine;


    /* ---- INITIALIZE FILE SYSTEM ---- */
    EFI_FILE_PROTOCOL* RootFolder = NULL;
    {
        EFI_LOADED_IMAGE_PROTOCOL* LoadedImage = NULL;
        EFI_ASSERT(g_BootServices->HandleProtocol(ImageHandle, &EFI_LOADED_IMAGE_PROTOCOL_GUID, (void **) &LoadedImage));

        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* Volume = NULL;
        EFI_ASSERT(g_BootServices->HandleProtocol(LoadedImage->DeviceHandle, &EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID, (void **) &Volume));

        EFI_ASSERT(Volume->OpenVolume(Volume, &RootFolder));
    }


    /* ---- LOAD DEFAULT FONT ---- */
    PSF1_Font Font = { .scale=1 };

    {
        EFI_FILE_PROTOCOL* FontFile = NULL;
        EFI_ASSERT(RootFolder->Open(RootFolder, &FontFile, (CHAR16 *) L"default-font.psf", 0x01, 0));

        ASSERTF(FontFile != NULL, "Couldn't load font!\r");

        UINTN FontDataSize = sizeof(PSF1_Header);
        EFI_ASSERT(FontFile->Read(FontFile, &FontDataSize, &Font.header));

        ASSERTF(Font.header.magic[0] == 0x36 && Font.header.magic[1] == 0x04, "Wrong magic number for font!\r");

        FontDataSize = Font.header.font_height * 256;
        if (Font.header.file_mode == 1)
            FontDataSize = Font.header.font_height * 512;

        EFI_ASSERT(FontFile->SetPosition(FontFile, sizeof(PSF1_Header)));
        EFI_ASSERT(g_BootServices->AllocatePool(EfiLoaderData, FontDataSize, (void**)&Font.glyphs));
        EFI_ASSERT(FontFile->Read(FontFile, &FontDataSize, Font.glyphs));
    }

    /* ---- LOAD KERNEL ---- */
    typedef __attribute__((sysv_abi)) int (*elf_main_fn)(Context*);
    elf_main_fn EntryPoint = NULL;
    Elf64ProgramHeader* Program = NULL;
    u8* KernelSource = NULL;
    {
        EFI_FILE_PROTOCOL* KernelFile = NULL;
        EFI_ASSERT(RootFolder->Open(RootFolder, &KernelFile, (CHAR16 *) L"kernel", 0x01, 0));

        UINTN Size = 0x10000;
        KernelSource = NULL;
        EFI_ASSERT(g_BootServices->AllocatePool(EfiLoaderData, Size, (void **) &KernelSource));
        EFI_ASSERT(KernelFile->Read(KernelFile, &Size, KernelSource));

        if (!KernelSource)
        {
            EfiPrintF(L"Couldn't load kernel!\n\r");
            EfiHalt();
        }

        if (!is_elf64(KernelSource))
            return 1;

        Elf64Header*        Header   = (Elf64Header*) KernelSource;
        Elf64ProgramHeader* Programs = (Elf64ProgramHeader*) (KernelSource + Header->program_header_offset);
        // const Elf64SectionHeader* section  = (Elf64SectionHeader*) (data + Header->section_Header_offset);

        EfiPrintF(L"Entry: %x\n\r", Header->entry_point);

        // https://wiki.osdev.org/ELF
        for (int i = 0; i < Header->program_header_entries; ++i)
        {
            Program = &Programs[i];

            if (Program->type == PT_LOAD)
                break;
        }

        ASSERT(Program->type == PT_LOAD);

        void* EntryPointAddress = (void *) Header->entry_point;
        EntryPoint = (elf_main_fn) EntryPointAddress;
    }

    /* ----- EXIT BOOT SERVICES ---- */
    Memory memory = { 0 };
    {
        /* The call between GetMemoryMap and ExitBootServices must be done
         * without any additional UEFI-calls (including print, as it could
         * potentially allocate resources and invalidate the memory map.
         */
        UINTN  MemoryMapSize     = 0;
        EFI_MEMORY_DESCRIPTOR* MemoryMap = NULL;
        UINTN  MapKey            = 0;
        UINTN  DescriptorSize    = 0;
        UINT32 DescriptorVersion = 0;

        /* Will fail with too small buffer, but return the size. */
        ASSERT(g_BootServices->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion) == EFI_BUFFER_TOO_SMALL);

        /* Considering that a new allocation can split a free memory area into
         * two, you should add space for 2 additional memory descriptors.
         */
        MemoryMapSize += 2 * DescriptorSize;
        EFI_ASSERT(g_BootServices->AllocatePool(EfiLoaderData, MemoryMapSize, (void **) &MemoryMap));
        EFI_ASSERT(g_BootServices->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion));

//        EFI_ASSERT(g_SystemTable->BootServices->ExitBootServices(ImageHandle, MapKey));

        memory = (Memory) {
            .MemoryMap=MemoryMap,
            .MemoryMapSize=MemoryMapSize,
            .DescriptorSize=DescriptorSize
        };
    }

    PageAllocator allocator = page_allocator_new_from_memory_map(&memory);
    PageTable* pml4 = page_allocator_request_page(&allocator);

    idt_install();

    LOG("Setting cr3\r");
    LOGF("pml4 at %x\r", (usize) pml4);
    x86_64_cr3_set(pml4);
    LOG("Cr3 set!\r");

    uint64_t destination = Program->virtual_address;
    uint64_t dest_size   = Program->memory_size;

    const uint8_t* source = KernelSource + Program->file_offset;
    uint64_t source_size  = Program->file_size;
    EFI_ASSERT(dest_size >= source_size ? EFI_SUCCESS : EFI_ERROR);

    for (usize j = 0; j < (dest_size-1) / PAGE_SIZE + 1; ++j)
    {
        void* address = page_allocator_request_page(&allocator);
        LOGF("Mapping %x - %x\r", destination + j * PAGE_SIZE, (u64) address);
        map_memory(pml4, &allocator, destination + j * PAGE_SIZE, (u64) address);
        memcpy((void *)destination, source, source_size);
    }


    Context context = {
            .memory=memory,
            .graphics=g_Graphics,
            .services=g_RuntimeServices,
            .font=Font,
            .allocator=allocator,
    };

    LOG("Exiting bootservices\r");

    {
        // The call between GetMemoryMap and ExitBootServices must be done
        // without any additional UEFI-calls (including print, as it could
        // potentially allocate resources and invalidate the memory map.
        UINTN  MemoryMapSize     = 0;
        EFI_MEMORY_DESCRIPTOR* MemoryMap = NULL;
        UINTN  MapKey            = 0;
        UINTN  DescriptorSize    = 0;
        UINT32 DescriptorVersion = 0;

        // Will fail with too small buffer, but return the size.
        g_BootServices->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);

        MemoryMapSize += 2 * DescriptorSize;
        EFI_ASSERT(g_BootServices->AllocatePool(EfiLoaderData, MemoryMapSize, (void **) &MemoryMap));
        EFI_ASSERT(g_BootServices->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion));

        EFI_ASSERT(g_SystemTable->BootServices->ExitBootServices(ImageHandle, MapKey));

        memory = (Memory) {
                .MemoryMap=MemoryMap,
                .MemoryMapSize=MemoryMapSize,
                .DescriptorSize=DescriptorSize
        };
    }


    return (EFI_STATUS) EntryPoint(&context);
}

