#include <stdarg.h>

#include "efi_main.h"
#include "efi_error.h"

#include "../bootloader.h"
#include "../elf.h"

#include "../memory.c"


// https://github.com/torvalds/linux/blob/5bfc75d92efd494db37f5c4c173d3639d4772966/drivers/firmware/efi/libstub/x86-stub.c
EFI_SYSTEM_TABLE*      g_SystemTable;
EFI_BOOT_SERVICES*     g_BootServices;
EFI_RUNTIME_SERVICES*  g_RuntimeServices;
Graphics               g_Graphics;

#include "efi_lib.c"


#define ASSERT(condition)  do { if (!(condition)) { EfiPrintF(L"ERROR! " #condition " was false/0/null\n\r"); EfiHalt(); } } while(0)


extern int char_is_unicode_16[sizeof(L"") == 2 ? 0 : -1];


EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable)
{
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
    EFI_ASSERT(g_SystemTable->ConOut->SetAttribute(g_SystemTable->ConOut, EFI_CYAN));
    EFI_ASSERT(g_SystemTable->ConOut->Reset(g_SystemTable->ConOut, EFI_TRUE));
    EfiPrintF(L"Screen initialized!\n\r");


    /* ---- INITIALIZE KEYBOARD ---- */
    // The implementation of Reset is required to clear the contents of any
    // input queues resident in memory used for buffering keystroke data and
    // put the input stream in a known empty state.
    EFI_ASSERT(g_SystemTable->ConIn->Reset(g_SystemTable->ConIn, EFI_TRUE));
    EfiPrintF(L"Keyboard initialized!\n\r");


    /* ---- INITIALIZE GRAPHICS ---- */
    EFI_GRAPHICS_OUTPUT_PROTOCOL* GraphicsOutput = NULL;
    EFI_ASSERT(g_BootServices->LocateProtocol(&EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID, NULL, (void**) &GraphicsOutput));

    ASSERT(GraphicsOutput);

    g_Graphics.base   = (Pixel *) GraphicsOutput->Mode->FrameBufferBase;
    g_Graphics.size   = GraphicsOutput->Mode->FrameBufferSize;
    g_Graphics.width  = GraphicsOutput->Mode->Info->HorizontalResolution;
    g_Graphics.height = GraphicsOutput->Mode->Info->VerticalResolution;
    g_Graphics.pixels_per_scanline = GraphicsOutput->Mode->Info->PixelsPerScanLine;

    EfiPrintF(L"Graphics initialized!\n\r");


    /* ---- INITIALIZE FILESYSTEM ---- */
    EFI_FILE_PROTOCOL* RootDirectory = NULL;
    {
        EFI_LOADED_IMAGE_PROTOCOL* LoadedImage = NULL;
        EFI_ASSERT(g_BootServices->HandleProtocol(ImageHandle, &EFI_LOADED_IMAGE_PROTOCOL_GUID, (void **) &LoadedImage));

        ASSERT(LoadedImage);

        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FileSystem = NULL;
        EFI_ASSERT(g_BootServices->HandleProtocol(LoadedImage->DeviceHandle, &EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID, (void **) &FileSystem));

        ASSERT(FileSystem);

        EFI_ASSERT(FileSystem->OpenVolume(FileSystem, &RootDirectory));

        ASSERT(RootDirectory);
    }

    EfiPrintF(L"Filesystem initialized!\n\r");


    /* ---- LOAD FONT ---- */
    PSF1_Font font = { .scale=1 };
    {
        EFI_FILE_PROTOCOL* FontFile = NULL;
        EFI_ASSERT(RootDirectory->Open(RootDirectory, &FontFile, L"default-font.psf", 0x01, 0));

        ASSERT(FontFile);

        UINTN FontHeaderSize = sizeof(PSF1_Header);
        EFI_ASSERT(FontFile->Read(FontFile, &FontHeaderSize, &font.header));

        ASSERT(font.header.magic[0] == 0x36 && font.header.magic[1] == 0x04);

        UINTN FontDataSize = (font.header.file_mode == 1) ? font.header.font_height * 512 : font.header.font_height * 256;

        EFI_ASSERT(FontFile->SetPosition(FontFile, sizeof(PSF1_Header)));
        EFI_ASSERT(g_BootServices->AllocatePool(EfiLoaderData, FontDataSize, (void**)&font.glyphs));
        EFI_ASSERT(FontFile->Read(FontFile, &FontDataSize, font.glyphs));

        ASSERT(font.glyphs);

        EFI_ASSERT(FontFile->Close(FontFile));
    }
    EfiPrintF(L"Font loaded!\n\r");


    /* ---- LOAD KERNEL FILE ---- */
    void* KernelSource = NULL;
    {
        EFI_FILE_PROTOCOL* KernelFile = NULL;
        EFI_ASSERT(RootDirectory->Open(RootDirectory, &KernelFile, L"kernel", EFI_FILE_MODE_READ, 0));

        ASSERT(KernelFile);

        UINT64 KernelFileSize = 0;
        {
            EFI_FILE_INFO* KernelFileInfo = NULL;
            UINTN BufferSize = 1;

            ASSERT(KernelFile->GetInfo(KernelFile, &EFI_FILE_INFO_ID, &BufferSize, NULL) == EFI_BUFFER_TOO_SMALL);
            EFI_ASSERT(SystemTable->BootServices->AllocatePool(EfiLoaderData, BufferSize, (void**) &KernelFileInfo));
            EFI_ASSERT(KernelFile->GetInfo(KernelFile, &EFI_FILE_INFO_ID, &BufferSize, (void**) &KernelFileInfo));

            KernelFileSize = KernelFileInfo->FileSize;
        }

        ASSERT(KernelFileSize);

        EfiPrintF(L"Kernel size is %zu!\n\r", KernelFileSize);

        EFI_ASSERT(g_BootServices->AllocatePool(EfiLoaderData, KernelFileSize, (void **) &KernelSource));
        EfiPrintF(L"Kernel size is %zu %zu!\n\r", KernelFileSize, (u64) KernelSource);
        EFI_ASSERT(KernelFile->Read(KernelFile, &KernelFileSize, KernelSource));

        ASSERT(KernelSource);

        EFI_ASSERT(KernelFile->Close(KernelFile));
    }
    EfiPrintF(L"Kernel loaded!\n\r");

    /* ---- FIND ENTRY POINT --- */
    typedef __attribute__((sysv_abi)) int (*elf_main_fn)(Context*);
    elf_main_fn entry_point;
    {
        if (!is_elf64(KernelSource))
        {
            EfiPrintF(L"ERROR! KernelSource was not valid elf64\n\r");
            EfiHalt();
        }

        const Elf64Header*        header   = (Elf64Header*) KernelSource;
        const Elf64ProgramHeader* programs = (Elf64ProgramHeader*) (KernelSource + header->program_header_offset);
        // const Elf64SectionHeader* section  = (Elf64SectionHeader*) (KernelSource + header->section_header_offset);

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

                const uint8_t* source = KernelSource + program->file_offset;
                EfiHalt();
                uint64_t source_size  = program->file_size;

                EfiPrintF(L"%zu, %zu\n\r", program->file_offset, (u64)KernelSource);

                EfiPrintF(L"Virtual address %zu, %zu\n\r", destination, dest_size);
                EfiPrintF(L"Source address %zu, %zu\n\r", (u64) source, source_size);

                if (dest_size > source_size)
                {
                    EfiPrintF(L"ERROR! source_size was smaller than dest_size\n\r");
                    EfiHalt();
                }
                memcpy((void *)destination, source, source_size);
            }
        }

        void* entry_point_address = (void *) header->entry_point;
        entry_point = (elf_main_fn) entry_point_address;
    }

    EfiPrintF(L"Entry point found!!\n\r");

    /* ---- EXIT BOOTLOADER ---- */
    // UEFI-spec page. 172
    // The call between GetMemoryMap and ExitBootServices must be done
    // without any additional UEFI-calls (including print, as it could
    // potentially) allocate resources and invalidate the memory map.
    EFI_MEMORY_DESCRIPTOR* MemoryMap = NULL;
    UINTN  MemoryMapSize     = 0;
    UINTN  MapKey            = 0;
    UINTN  DescriptorSize    = 0;
    UINT32 DescriptorVersion = 0;

    SystemTable->BootServices->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
    MemoryMapSize += 2 * DescriptorSize;
    EFI_ASSERT(SystemTable->BootServices->AllocatePool(EfiLoaderData, MemoryMapSize, (void**)&MemoryMap));
    EFI_ASSERT(SystemTable->BootServices->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion));
    EFI_ASSERT(g_SystemTable->BootServices->ExitBootServices(ImageHandle, MapKey));

    Memory memory = {
        .MemoryMap=MemoryMap,
        .MemoryMapSize=MemoryMapSize,
        .DescriptorSize=DescriptorSize
    };

    Context context = {
        .memory=memory,
        .graphics=g_Graphics,
        .services=g_RuntimeServices,
        .font=font,
    };

    int result = entry_point(&context);
    return result;
}

