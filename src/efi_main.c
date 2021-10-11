#include "efi_main.h"
#include "efi_error.h"

#include "format.c"
#include "memory.c"

// https://github.com/torvalds/linux/blob/5bfc75d92efd494db37f5c4c173d3639d4772966/drivers/firmware/efi/libstub/x86-stub.c
typedef struct Graphics {
    u8* base;
    u64 size;
    u32 width;
    u32 height;
    u32 pixels_per_scanline;
} Graphics;

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


void EfiDelay(UINTN ms)
{
    // The Stall function is set as microseconds. We stall 1 microsecond.
    g_BootServices->Stall(ms);
}


void EfiSetTextPosition(UINT32 Col, UINT32 Row)
{
    // Sets the Column and Row of the text screen cursor position.
    g_SystemTable->ConOut->SetCursorPosition(g_SystemTable->ConOut, Col, Row);
}


void EfiPrint(const CHAR16* string)
{
    g_SystemTable->ConOut->OutputString(g_SystemTable->ConOut, string);
}


#define EFI_ASSERT(status) EfiAssert(status, (const CHAR16*) FILE_WCHAR, __LINE__)
void EfiAssert(EFI_STATUS status, const CHAR16* file, int line)
{
    if ((status & EFI_ERROR) == EFI_ERROR)
    {
        g_SystemTable->ConOut->SetAttribute(g_SystemTable->ConOut, EFI_RED);
        EfiPrint(L"[ASSERT FAILED]: ");
        EfiPrint(EfiErrorString(status));
        EfiPrint(L"\r\n");
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
    UINTN Index;
    EFI_ASSERT(g_BootServices->WaitForEvent(
        1,
        &g_SystemTable->ConIn->WaitForKey,
        &Index
    ));

//    EFI_INPUT_KEY Key;
//    while (g_SystemTable->ConIn->ReadKeyStroke(g_SystemTable->ConIn, &Key) == EFI_NOT_READY);
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

    EfiPrint(L"Image loaded at: ");
    EfiPrint(ToHexString((usize) LoadedImage->ImageBase).data);
    EfiPrint(L"\n\r");

    EfiPrint(L"Image size: ");
    EfiPrint(ToHexString((usize) LoadedImage->ImageSize).data);
    EfiPrint(L"\n\r");



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
        EfiPrint(L"[ERROR]: No file handle was given!\n\r");
        return (Array) { .data=NULL, .size=0 };
    }
}


void EfiHalt()
{
    // Change to 0 in debugger to continue.
    int volatile wait = 1;
    while (wait) {
        __asm__ __volatile__("pause");
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
        EfiPrint(L"Couldn't initialize Graphics!\n\r");
        return;
    }

    g_Graphics.base   = (u8*) GraphicsOutput->Mode->FrameBufferBase;
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

    EfiPrint(L"---- UEFI bootloader up and running! ----\n\r");
//    for (int i = 0; i < 40; ++i)
//    {
//        EfiDelay(1000000);
//        g_Graphics->base[i] = 0x75;
//    }
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

    EfiPrint(L"Current hour: ");
    EfiPrint(Hour);
    EfiPrint(L"\n\r");
}

void EfiLoadTest(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* Volume)
{
    EFI_FILE_PROTOCOL* File = EfiOpenFile(Volume, L"text.txt");
    Array content = EfiReadFile(File, 0x1000);
    if (content.data)
    {
        EfiPrint(L"Content is: ");
        EfiPrint((const CHAR16 *) content.data);
        EfiPrint(L"\n\r");
    }
}


void EfiLoadKernel(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* Volume)
{
    EFI_FILE_PROTOCOL* KernelFile = EfiOpenFile(Volume, L"kernel.bin");
    Array KernelSource = EfiReadFile(KernelFile, 0x10000);
    if (KernelSource.data)
    {
        // g_SystemTable->BootServices->ExitBootServices()

        typedef __attribute__((ms_abi)) int (*KernelMainFn)();

        // usize entry_point = 0x104;
        u8* KernelMain = &KernelSource.data[0x104];

        EfiPrint(L"Running Kernel...\n\r");
        KernelMainFn kernel_main = (KernelMainFn) KernelMain;
        int result = kernel_main();

        EfiPrint(L"Kernel exited with status code ");
        EfiPrint(ToHexString((usize) result).data);
        EfiPrint(L"\n\r");
    }
}


// UEFI-spec page. 172
void EfiMemoryMap()
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
            EfiSetTextPosition(0, i / 2 + 1);
            EfiPrint(EFI_MEMORY_TYPE_STRINGS[desc->Type]);
            EfiPrint(L" - ");
            EfiPrint(U64ToString(kb, 10).data);
        }
        else
        {
            EfiSetTextPosition(32, i / 2 + 1);
            EfiPrint(EFI_MEMORY_TYPE_STRINGS[desc->Type]);
            EfiPrint(L" - ");
            EfiPrint(U64ToString(kb, 10).data);
        }

        EfiDelay(100000);
    }

    EfiPrint(L"Total memory: ");
    EfiPrint(U64ToString(TotalRam, 10).data);
}


EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable)
{
    EfiInit(SystemTable);

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* Volume = EfiInitializeFileSystem(ImageHandle);

    EfiPrint(L"Press 'q' to shutdown | Press 'r' to reboot | Press 's' to start\n\r");
    while (1)
    {
        EFI_INPUT_KEY Key = {};
        if (EfiKeyboardPoll(&Key) == EFI_SUCCESS)
        {
            if (Key.UnicodeChar == 'q')
            {
                EfiShutdown();
                break;
            }
            else if (Key.UnicodeChar == 'r')
            {
                EfiSoftwareReboot();
                break;
            }
            else if (Key.UnicodeChar == 's')
            {
                EfiLoadKernel(Volume);
            }
            else
            {
                CHAR16 Temp[] = { Key.UnicodeChar, 0 };
                EfiPrint(L"Please chose a valid key, not '");
                EfiPrint(Temp);
                EfiPrint(L"'...\n\r");
            }
        }
    }


    return EFI_SUCCESS;
}

