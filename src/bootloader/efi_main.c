#include <stdarg.h>

#include "efi_main.h"
#include "efi_error.h"
#include "efi_lib.c"

#include "../bootloader.h"

#include "../elf.h"
#include "../memory.c"


// https://github.com/torvalds/linux/blob/5bfc75d92efd494db37f5c4c173d3639d4772966/drivers/firmware/efi/libstub/x86-stub.c
EFI_SYSTEM_TABLE*      g_SystemTable;
EFI_BOOT_SERVICES*     g_BootServices;
EFI_RUNTIME_SERVICES*  g_RuntimeServices;
Graphics               g_Graphics;


EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable)
{
    /* ---- INITIALIZE STATICS ---- */
    // The System Table contains pointers to other standard tables that a loaded
    // image may use if the associated pointers are initialized to nonzero values.
    g_SystemTable     = SystemTable;
    g_BootServices    = SystemTable->BootServices;
    g_RuntimeServices = SystemTable->RuntimeServices;


    /* ---- INITIALIZE SCREEN ---- */
    // UEFI-spec page 451
    // The Reset() function resets the text output device hardware.
    // The cursor position is set to (0, 0), and the screen is cleared to the
    // default background color for the output device.
    g_SystemTable->ConOut->Reset(g_SystemTable->ConOut, EFI_TRUE);
    g_SystemTable->ConOut->SetAttribute(g_SystemTable->ConOut, EFI_CYAN);


    /* ---- INITIALIZE KEYBOARD ---- */
    // The implementation of Reset is required to clear the contents of any
    // input queues resident in memory used for buffering keystroke data and
    // put the input stream in a known empty state.
    g_SystemTable->ConIn->Reset(g_SystemTable->ConIn, EFI_TRUE);


    /* ---- INITIALIZE GRAPHICS ---- */
    EFI_GRAPHICS_OUTPUT_PROTOCOL* GraphicsOutput = NULL;
    EFI_ASSERT(g_BootServices->LocateProtocol(&EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID, NULL, (void**) &GraphicsOutput));

    if (!GraphicsOutput)
    {
        EfiPrintF(L"Couldn't initialize Graphics!\n\r");
        EfiHalt();
    }

    g_Graphics.base   = (Pixel *) GraphicsOutput->Mode->FrameBufferBase;
    g_Graphics.size   = GraphicsOutput->Mode->FrameBufferSize;
    g_Graphics.width  = GraphicsOutput->Mode->Info->HorizontalResolution;
    g_Graphics.height = GraphicsOutput->Mode->Info->VerticalResolution;
    g_Graphics.pixels_per_scanline = GraphicsOutput->Mode->Info->PixelsPerScanLine;

    EfiPrintF(L"---- UEFI bootloader up and running! ----\n\r");


    /* ---- INITIALIZE FILE SYSTEM ---- */
    EFI_FILE_PROTOCOL* RootFolder = NULL;
    {
        EFI_LOADED_IMAGE_PROTOCOL* LoadedImage = NULL;
        EFI_ASSERT(g_BootServices->HandleProtocol(ImageHandle, &EFI_LOADED_IMAGE_PROTOCOL_GUID, (void **) &LoadedImage));

        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* Volume = NULL;
        EFI_ASSERT(g_BootServices->HandleProtocol(LoadedImage->DeviceHandle, &EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID, (void **) &Volume));

        EfiPrintF(L"Image loaded at: %x\n\r", (usize) LoadedImage->ImageBase);
        EfiPrintF(L"Image size:      %x\n\r", (usize) LoadedImage->ImageSize);

        EFI_ASSERT(Volume->OpenVolume(Volume, &RootFolder));
    }


    /* ---- LOAD DEFAULT FONT ---- */
    PSF1_Font Font = { .scale=1 };

    {
        EFI_FILE_PROTOCOL* FontFile = NULL;
        EFI_ASSERT(RootFolder->Open(RootFolder, &FontFile, L"default-font.psf", 0x01, 0));

        if (!FontFile)
        {
            EfiPrintF(L"Couldn't load font!\n\r");
            EfiHalt();
        }

        EfiPrintF(L"Loading font!\n\r");
        UINTN FontDataSize = sizeof(PSF1_Header);
        EFI_ASSERT(FontFile->Read(FontFile, &FontDataSize, &Font.header));

        if (Font.header.magic[0] != 0x36 || Font.header.magic[1] != 0x04)
        {
            EfiPrintF(L"Wrong magic number for font!\n\r");
            EfiHalt();
        }

        EfiPrintF(L"Font validated!\n\r");

        FontDataSize = Font.header.font_height * 256;
        if (Font.header.file_mode == 1)
            FontDataSize = Font.header.font_height * 512;

        EFI_ASSERT(FontFile->SetPosition(FontFile, sizeof(PSF1_Header)));
        EFI_ASSERT(g_BootServices->AllocatePool(EfiLoaderData, FontDataSize, (void**)&Font.glyphs));
        EfiPrintF(L"Size: %d\n\r", FontDataSize);
        EFI_ASSERT(FontFile->Read(FontFile, &FontDataSize, Font.glyphs));

        EfiPrintF(L"Font Loaded!\n\r");
    }

    /* ---- LOAD KERNEL ---- */
    typedef __attribute__((sysv_abi)) int (*elf_main_fn)(Context*);
    elf_main_fn EntryPoint = NULL;
    {

        EFI_FILE_PROTOCOL* KernelFile = NULL;
        EFI_ASSERT(RootFolder->Open(RootFolder, &KernelFile, L"kernel", 0x01, 0));

        UINTN Size = 0x10000;
        u8* KernelSource = NULL;
        EFI_ASSERT(g_BootServices->AllocatePool(EfiLoaderData, Size, (void **) &KernelSource));
        EFI_ASSERT(KernelFile->Read(KernelFile, &Size, KernelSource));

        if (!KernelSource)
        {
            EfiPrintF(L"Couldn't load kernel!\n\r");
            EfiHalt();
        }

        if (!is_elf64(KernelSource))
            return ELF_ERROR;

        const Elf64Header*        Header   = (Elf64Header*) KernelSource;
        const Elf64ProgramHeader* Programs = (Elf64ProgramHeader*) (KernelSource + Header->program_header_offset);
        // const Elf64SectionHeader* section  = (Elf64SectionHeader*) (data + Header->section_Header_offset);

        // https://wiki.osdev.org/ELF
        for (int i = 0; i < Header->program_header_entries; ++i)
        {
            const Elf64ProgramHeader* Program = &Programs[i];

            if (Program->type == PT_LOAD)
            {
                // Allocate `size` virtual memory at `address` and copy from
                // source to destination.
                // If the file_size and memory_size members differ,
                // the segment is padded with zeros.

                uint64_t destination = Program->virtual_address;
                uint64_t dest_size   = Program->memory_size;

                const uint8_t* source = KernelSource + Program->file_offset;
                uint64_t source_size  = Program->file_size;

                EfiPrintF(L"Sizes %d to %d\r\n", (u64)dest_size, source_size);
                EFI_ASSERT(dest_size >= source_size ? EFI_SUCCESS : EFI_ERROR);

                EfiPrintF(L"Mapping %x to %x\r\n", (u64)source, destination);
                memcpy((void *)destination, source, source_size);
                EfiPrintF(L"Done!\r\n");
            }
        }

        EfiPrintF(L"Entry point: %x\r\n", Header->entry_point);
        void* EntryPointAddress = (void *) Header->entry_point;
        EntryPoint = (elf_main_fn) EntryPointAddress;
    }

    /* ----- EXIT BOOT SERVICES ---- */
    Memory memory = { 0 };
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

        memory = (Memory ){
            .MemoryMap=MemoryMap,
            .MemoryMapSize=MemoryMapSize,
            .DescriptorSize=DescriptorSize
        };
    }

    Context context = {
            .memory=memory,
            .graphics=g_Graphics,
            .services=g_RuntimeServices,
            .font=Font,
    };

    return EntryPoint(&context);
}

