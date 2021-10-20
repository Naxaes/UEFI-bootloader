#include <stdarg.h>

#include "efi_main.h"
#include "efi_error.h"

#include "bootloader.h"

#include "elf.h"

#include "format.c"
#include "memory.c"


// https://github.com/torvalds/linux/blob/5bfc75d92efd494db37f5c4c173d3639d4772966/drivers/firmware/efi/libstub/x86-stub.c
EFI_SYSTEM_TABLE*      g_SystemTable;
EFI_BOOT_SERVICES*     g_BootServices;
EFI_RUNTIME_SERVICES*  g_RuntimeServices;
Graphics               g_Graphics;



const CHAR16* EFI_MEMORY_TYPE_STRINGS[] = {
    (const CHAR16*) L"EfiReservedMemoryType",
    (const CHAR16*) L"EfiLoaderCode",
    (const CHAR16*) L"EfiLoaderData",
    (const CHAR16*) L"EfiBootServicesCode",
    (const CHAR16*) L"EfiBootServicesData",
    (const CHAR16*) L"EfiRuntimeServicesCode",
    (const CHAR16*) L"EfiRuntimeServicesData",
    (const CHAR16*) L"EfiConventionalMemory",
    (const CHAR16*) L"EfiUnusableMemory",
    (const CHAR16*) L"EfiACPIReclaimMemory",
    (const CHAR16*) L"EfiACPIMemoryNVS",
    (const CHAR16*) L"EfiMemoryMappedIO",
    (const CHAR16*) L"EfiMemoryMappedIOPortSpace",
    (const CHAR16*) L"EfiPalCode",
    (const CHAR16*) L"EfiPersistentMemory",
    (const CHAR16*) L"EfiUnacceptedMemoryType",
    (const CHAR16*) L"EfiMaxMemoryType"
};


void EfiHalt()
{
    // Change to 0 in debugger to continue.
    int volatile wait = 1;
    while (wait) {
        __asm__ __volatile__("pause");
    }
}


void EfiDelay(UINTN ms)
{
    // The Stall function is set as microseconds. We stall 1 microsecond.
    g_BootServices->Stall(ms);
}


void EfiSetCursorPosition(UINT32 Col, UINT32 Row)
{
    // Sets the Column and Row of the text screen cursor position.
    g_SystemTable->ConOut->SetCursorPosition(g_SystemTable->ConOut, Col, Row);
}

void EfiGetCursorPosition(INT32* x, INT32* y)
{
    *x = g_SystemTable->ConOut->Mode->CursorColumn;
    *y = g_SystemTable->ConOut->Mode->CursorRow;
}


void EfiPrintString(const CHAR16* string)
{
    g_SystemTable->ConOut->OutputString(g_SystemTable->ConOut, string);
}

void EfiPrintChar(CHAR16 character)
{
    CHAR16 string[] = { character, L'\0' };
    g_SystemTable->ConOut->OutputString(g_SystemTable->ConOut, string);
}


void EfiPrintF(const CHAR16* format, ...)
{
    va_list arg;
    va_start(arg, format);

    const CHAR16* character = format;
    while (*character != L'\0')
    {
//        if (*character == '\t')
//        {
//            INT32 x = 0;
//            INT32 y = 0;
//            EfiGetCursorPosition(&x, &y);
//            EfiSetCursorPosition(x + (TAB_SIZE - (x % TAB_SIZE)), y);
//
//            character++;
//        }
        if (*character != L'%')
        {
            EfiPrintChar(*character);
            character++;
        }
        else
        {
            character++;
            switch (*character)
            {
                case L'c':
                {
                    CHAR16 i = va_arg(arg, int);  // Fetch char argument
                    EfiPrintChar(i);
                    break;
                }
                case L'd' :
                {
                    int i = va_arg(arg, int);  // Fetch Decimal/Integer argument
                    if (i < 0)
                    {
                        i = -i;
                        EfiPrintChar(L'-');
                    }
                    EfiPrintString(U64ToString(i, 10).data);
                    break;
                }
                case L'o':
                {
//                    i = va_arg(arg,
//                    unsigned int); //Fetch Octal representation
//                    puts(convert(i, 8));
                    break;
                }
                case L's':
                {
                    CHAR16* s = va_arg(arg, CHAR16*);       //Fetch string
                    EfiPrintString(s);
                    break;
                }
                case L'x':
                {
                    unsigned int i = va_arg(arg, unsigned int); //Fetch Hexadecimal representation
                    EfiPrintString(ToHexStringTruncated(i).data);
                    break;
                }
                case L'z':  // size_t or ssize_t
                {
                    character++;
                    if (*character == L'u')
                    {
                        usize x = va_arg(arg, usize);
                        EfiPrintString(U64ToString(x, 10).data);
                        break;
                    }
                    else
                    {
//                        EFI_ASSERT(EFI_ERROR);
                        return;
                    }
                }
                default:
                {
//                    EFI_ASSERT(EFI_ERROR);
                    return;
                }
            }
            character++;
        }

    }

    //Module 3: Closing argument list to necessary clean-up
    va_end(arg);
}


#define EFI_ASSERT(status) EfiAssert(status, (const CHAR16*) FILE_WCHAR, __LINE__)
void EfiAssert(EFI_STATUS status, const CHAR16* file, int line)
{
    if ((status & EFI_ERROR) == EFI_ERROR)
    {
        g_SystemTable->ConOut->SetAttribute(g_SystemTable->ConOut, EFI_RED);
        EfiPrintF(L"[ASSERT FAILED] (%s:%d): '%s'\n\r", file, line, EfiErrorString(status));
        EfiHalt();
    }
}


void EfiKeyboardReset()
{
    // The implementation of Reset is required to clear the contents of any
    // input queues resident in memory used for buffering keystroke data and
    // put the input stream in a known empty state.
    g_SystemTable->ConIn->Reset(g_SystemTable->ConIn, EFI_TRUE);
}

EFI_STATUS EfiKeyboardPoll(EFI_INPUT_KEY* Key)
{
    return g_SystemTable->ConIn->ReadKeyStroke(g_SystemTable->ConIn, Key);
}

void EfiKeyboardWait()
{
    // TODO(ted): This works only once... Why?
//    UINTN Index;
//    EFI_ASSERT(g_BootServices->WaitForEvent(
//        1,
//        &g_SystemTable->ConIn->WaitForKey,
//        &Index
//    ));

    EFI_INPUT_KEY Key;
    while (g_SystemTable->ConIn->ReadKeyStroke(g_SystemTable->ConIn, &Key) == EFI_NOT_READY);
//    return Key;
}


EFI_STATUS EfiScreenBlit(
    EFI_GRAPHICS_OUTPUT_PROTOCOL* GraphicsOutput,
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL Color,
    int x, int y, int w, int h
)
{
    // EfiBltVideoFill. Write data from the BltBuffer pixel (0,0) directly to
    // every pixel of the video display rectangle (DestinationX, DestinationY)
    // (DestinationX + Width, DestinationY + Height). Only one pixel will be
    // used from the BltBuffer. Delta is NOT used.
    return GraphicsOutput->Blt(
        GraphicsOutput, &Color, EfiBltVideoFill, 0, 0, x, y, w, h, 0
    );
}


EFI_STATUS EfiScreenBlitFramebuffer(
    EFI_GRAPHICS_OUTPUT_PROTOCOL*  GraphicsOutput,
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* Color,
    int x, int y, int w, int h
)
{
    // EfiBltBufferToVideo: Write data from the BltBuffer rectangle
    // (SourceX, SourceY) (SourceX + Width, SourceY + Height) directly to the
    // video display rectangle (DestinationX, DestinationY)
    // (DestinationX + Width, DestinationY + Height). If SourceX or SourceY is
    // not zero then Delta must be set to the length in bytes of a row in the
    // BltBuffer.

    return GraphicsOutput->Blt(
        GraphicsOutput, Color, EfiBltBufferToVideo, 0, 0, x, y, w, h, 0
    );
}

void EfiHardwareReboot()
{
    g_RuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, 0);
}

void EfiSoftwareReboot()
{
    g_RuntimeServices->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, 0);
}

void EfiShutdown()
{
    g_RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, 0);
}


EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* EfiInitializeFileSystem(EFI_HANDLE ImageHandle)
{
    EFI_LOADED_IMAGE_PROTOCOL* LoadedImage = NULL;
    EFI_ASSERT(g_BootServices->HandleProtocol(
        ImageHandle,
        &EFI_LOADED_IMAGE_PROTOCOL_GUID,
        (void **) &LoadedImage
    ));

    EFI_DEVICE_PATH_PROTOCOL* DevicePath = NULL;
    EFI_ASSERT(g_BootServices->HandleProtocol(
        LoadedImage->DeviceHandle,
        &EFI_DEVICE_PATH_PROTOCOL_GUID,
        (void **) &DevicePath
    ));

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* Volume = NULL;
    EFI_ASSERT(g_BootServices->HandleProtocol(
        LoadedImage->DeviceHandle,
        &EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID,
        (void **) &Volume
    ));

    EfiPrintF(L"Image loaded at: %x\n\r", (usize) LoadedImage->ImageBase);
    EfiPrintF(L"Image size:      %x\n\r", (usize) LoadedImage->ImageSize);

    return Volume;
}


EFI_FILE_PROTOCOL* EfiOpenFile(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* Volume, CHAR16* Path)
{
    EFI_FILE_PROTOCOL* Root = NULL;
    EFI_ASSERT(Volume->OpenVolume(Volume, &Root));

    EFI_FILE_PROTOCOL* FileHandle = NULL;
    EFI_ASSERT(Root->Open(Root, &FileHandle, Path, 0x01, 0));

    return FileHandle;
}

Array EfiReadFile(EFI_FILE_PROTOCOL* File, UINTN size)
{
    if (File)
    {
        u8* data = NULL;
        EFI_ASSERT(g_BootServices->AllocatePool(
            EfiLoaderData,
            size,
            (void **) &data
        ));
        EFI_ASSERT(File->Read(File, &size, data));

        for (int i = size; i < size; ++i)
            data[i] = 0;

        return (Array) { .data=data, .size=size };
    }
    else
    {
        EfiPrintF(L"[ERROR]: No file handle was given!\n\r");
        return (Array) { .data=NULL, .size=0 };
    }
}


void EfiInitScreen()
{
    // UEFI-spec page 451
    // The Reset() function resets the text output device hardware.
    // The cursor position is set to (0, 0), and the screen is cleared to the
    // default background color for the output device.
    g_SystemTable->ConOut->Reset(g_SystemTable->ConOut, EFI_TRUE);
    g_SystemTable->ConOut->SetAttribute(g_SystemTable->ConOut, EFI_CYAN);
}


void EfiInitGraphics()
{
    EFI_GRAPHICS_OUTPUT_PROTOCOL* GraphicsOutput = NULL;
    EFI_ASSERT(g_BootServices->LocateProtocol(
        &EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID,
        NULL,
        (void**) &GraphicsOutput
    ));

    if (!GraphicsOutput)
    {
        EfiPrintF(L"Couldn't initialize Graphics!\n\r");
        return;
    }

    g_Graphics.base   = (Pixel *) GraphicsOutput->Mode->FrameBufferBase;
    g_Graphics.size   = GraphicsOutput->Mode->FrameBufferSize;
    g_Graphics.width  = GraphicsOutput->Mode->Info->HorizontalResolution;
    g_Graphics.height = GraphicsOutput->Mode->Info->VerticalResolution;
    g_Graphics.pixels_per_scanline = GraphicsOutput->Mode->Info->PixelsPerScanLine;
}


void EfiInit(EFI_SYSTEM_TABLE* SystemTable)
{
    // The System Table contains pointers to other standard tables that a loaded
    // image may use if the associated pointers are initialized to nonzero values.
    g_SystemTable     = SystemTable;
    g_BootServices    = SystemTable->BootServices;
    g_RuntimeServices = SystemTable->RuntimeServices;

    EfiInitScreen();
    EfiKeyboardReset();
    EfiInitGraphics();

    EfiPrintF(L"---- UEFI bootloader up and running! ----\n\r");
}


void EfiCurrentTime()
{
    EFI_TIME* Time = NULL;
    EFI_ASSERT(g_BootServices->AllocatePool(
        EfiBootServicesData,
        sizeof(EFI_TIME),
        (void **)&Time
    ));
    EFI_ASSERT(g_RuntimeServices->GetTime(Time, NULL));
    CHAR16 Hour[] = { L'0' + Time->Hour / 10, L'0' + Time->Hour % 10, 0 };

    EfiPrintF(L"Current hour: %s\n\r", Hour);
}

void EfiLoadTest(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* Volume)
{
    EFI_FILE_PROTOCOL* File = EfiOpenFile(Volume, L"text.txt");
    Array content = EfiReadFile(File, 0x1000);
    if (content.data)
    {
        EfiPrintF(L"Content is: '%s'\n\r", content.data);
    }
}


// UEFI-spec page. 172
UINTN EfiMemoryMap()
{
    UINTN  MemoryMapSize     = 0;
    EFI_MEMORY_DESCRIPTOR* MemoryMap = NULL;
    UINTN  MapKey            = 0;
    UINTN  DescriptorSize    = 0;
    UINT32 DescriptorVersion = 0;

    // Will fail with too small buffer, but return the size.
    g_BootServices->GetMemoryMap(
        &MemoryMapSize,
        MemoryMap,
        &MapKey,
        &DescriptorSize,
        &DescriptorVersion
    );

    MemoryMapSize += 2 * DescriptorSize;
    EFI_ASSERT(g_BootServices->AllocatePool(
        EfiLoaderData,
        MemoryMapSize,
        (void **) &MemoryMap
    ));

    EFI_ASSERT(g_BootServices->GetMemoryMap(
        &MemoryMapSize,
        MemoryMap,
        &MapKey,
        &DescriptorSize,
        &DescriptorVersion
    ));

    return MapKey;
}

void EfiPrintMemoryMap(UINTN MemoryMapSize, EFI_MEMORY_DESCRIPTOR* MemoryMap, UINTN DescriptorSize)
{
    usize MapEntries = MemoryMapSize / DescriptorSize;
    u64 TotalRam = 0;
    for (usize i = 0; i < MapEntries; ++i)
    {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)
                ((u64)MemoryMap + (i * DescriptorSize));

        usize kb = desc->NumberOfPages * 4096 / 1024;
        TotalRam += kb;

        if (i % 2 == 0)
        {
            EfiPrintF(L"%s - %zu\t - ", EFI_MEMORY_TYPE_STRINGS[desc->Type], kb);
        }
        else
        {
            EfiPrintF(L"%s - %zu\n\r", EFI_MEMORY_TYPE_STRINGS[desc->Type], kb);
        }
    }

    EfiPrintF(L"Total memory: %zu\n\r", TotalRam);
}


// UEFI-spec page. 172
Memory EfiExitBootServices(EFI_HANDLE ImageHandle)
{
    // The call between GetMemoryMap and ExitBootServices must be done
    // without any additional UEFI-calls (including print, as it could
    // potentially) allocate resources and invalidate the memory map.

    UINTN  MemoryMapSize     = 0;
    EFI_MEMORY_DESCRIPTOR* MemoryMap = NULL;
    UINTN  MapKey            = 0;
    UINTN  DescriptorSize    = 0;
    UINT32 DescriptorVersion = 0;

    // Will fail with too small buffer, but return the size.
    g_BootServices->GetMemoryMap(
            &MemoryMapSize,
            MemoryMap,
            &MapKey,
            &DescriptorSize,
            &DescriptorVersion
    );

    MemoryMapSize += 2 * DescriptorSize;
    EFI_ASSERT(g_BootServices->AllocatePool(
            EfiLoaderData,
            MemoryMapSize,
            (void **) &MemoryMap
    ));

    EFI_ASSERT(g_BootServices->GetMemoryMap(
            &MemoryMapSize,
            MemoryMap,
            &MapKey,
            &DescriptorSize,
            &DescriptorVersion
    ));


    EFI_ASSERT(g_SystemTable->BootServices->ExitBootServices(
        ImageHandle, MapKey
    ));

    return (Memory) {
        .MemoryMap=MemoryMap,
        .MemoryMapSize=MemoryMapSize,
        .DescriptorSize=DescriptorSize
    };
}


PSF1_Font EfiLoadFont(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* Volume)
{
    PSF1_Font font = { .scale=1 };

    EFI_FILE_PROTOCOL* File = EfiOpenFile(Volume, L"default-font.psf");
    if (File)
    {
        EfiPrintF(L"Loading font!\n\r");
        UINTN size = sizeof(PSF1_Header);
        EFI_ASSERT(File->Read(File, &size, &font.header));

        if (font.header.magic[0] != 0x36 || font.header.magic[1] != 0x04)
        {
            EfiPrintF(L"Wrong magic number for font!\n\r");
            EfiHalt();
        }
        EfiPrintF(L"Font validated!\n\r");

        size = font.header.font_height * 256;
        if (font.header.file_mode == 1)
            size = font.header.font_height * 512;

        EFI_ASSERT(File->SetPosition(File, sizeof(PSF1_Header)));
        EFI_ASSERT(g_BootServices->AllocatePool(EfiLoaderData, size, (void**)&font.glyphs));
        EfiPrintF(L"Size: %d\n\r", size);
        EFI_ASSERT(File->Read(File, &size, font.glyphs));

        EfiPrintF(L"Font Loaded!\n\r");
    }

    return font;
}


int EfiLoadElf(EFI_HANDLE ImageHandle, EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* Volume, u8* data)
{
    PSF1_Font font = EfiLoadFont(Volume);

    if (!is_elf64(data))
        return ELF_ERROR;

    const Elf64Header*        header   = (Elf64Header*) data;
    const Elf64ProgramHeader* programs = (Elf64ProgramHeader*) (data + header->program_header_offset);
//    const Elf64SectionHeader* section  = (Elf64SectionHeader*) (data + header->section_header_offset);

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

            EfiPrintF(L"Sizes %d to %d\r\n", (u64)dest_size, source_size);
            EFI_ASSERT(dest_size >= source_size ? EFI_SUCCESS : EFI_ERROR);

            EfiPrintF(L"Mapping %x to %x\r\n", (u64)source, destination);
            memcpy((void *)destination, source, source_size);
            EfiPrintF(L"Done!\r\n");
        }
    }

    EfiPrintF(L"Entry point: %x\r\n", header->entry_point);

    typedef __attribute__((sysv_abi)) int (*elf_main_fn)(Context*);

    void* entry_point_address = (void *) header->entry_point;
    elf_main_fn entry_point = (elf_main_fn) entry_point_address;

    Memory memory = EfiExitBootServices(ImageHandle);
    Context context = {
        .memory=memory,
        .graphics=g_Graphics,
        .services=g_RuntimeServices,
        .font=font,
    };

    int result = entry_point(&context);
    return result;
}


int EfiLoadKernel(EFI_HANDLE ImageHandle, EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* Volume)
{
    EFI_FILE_PROTOCOL* KernelFile = EfiOpenFile(Volume, L"kernel");
    Array KernelSource = EfiReadFile(KernelFile, 0x10000);
    if (KernelSource.data)
    {
        u8* s = KernelSource.data;
        u16 format = *(u16*)s;
        if (format == 0x8664)  // X86_64 (COFF?)
        {
            EfiPrintF(L"Loading x86_64 kernel\n\r");
            u16 entry_point = *((u16*) &s[0x24]);
            Memory memory = EfiExitBootServices(ImageHandle);
            typedef __attribute__((ms_abi)) int (*KernelMainFn)(EFI_RUNTIME_SERVICES*, Graphics, Memory);
            u8* KernelMain = &KernelSource.data[entry_point];
            KernelMainFn kernel_main = (KernelMainFn) KernelMain;
            int result = kernel_main(g_RuntimeServices, g_Graphics, memory);
            return result;
        }
        else if (*(u32*)s == 0x464C457F)
        {
            return EfiLoadElf(ImageHandle, Volume, KernelSource.data);
        }
        else
        {
            EfiPrintF(L"Error at line %d!\n\r", __LINE__);
            return -1;
        }
    }

    return -1;
}


EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable)
{
    EfiInit(SystemTable);

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* Volume = EfiInitializeFileSystem(ImageHandle);

    EfiLoadKernel(ImageHandle, Volume);
    EfiHalt();


    EfiPrintF(L"Press 'q' to shutdown | Press 'r' to reboot | Press 's' to start\n\r");
    while (1)
    {
        EFI_INPUT_KEY Key = {};
        if (EfiKeyboardPoll(&Key) == EFI_SUCCESS)
        {
            if (Key.UnicodeChar == L'q')
            {
                EfiShutdown();
                return EFI_SUCCESS;
            }
            else if (Key.UnicodeChar == L'r')
            {
                EfiSoftwareReboot();
                return EFI_SUCCESS;
            }
            else if (Key.UnicodeChar == L's')
            {
                EfiLoadKernel(ImageHandle, Volume);
                EfiHalt();
                EfiShutdown();
                return EFI_SUCCESS;
            }
            else
            {
                EfiPrintF(L"Please chose a valid key, not '%c'\n\r", Key.UnicodeChar);
            }
        }
    }

    return EFI_SUCCESS;
}

