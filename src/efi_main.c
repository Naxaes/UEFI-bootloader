#include "types.h"

#include "efi_main.h"
#include "efi_error.h"

#include "format.c"
#include "memory.c"

// https://github.com/torvalds/linux/blob/5bfc75d92efd494db37f5c4c173d3639d4772966/drivers/firmware/efi/libstub/x86-stub.c

EFI_SYSTEM_TABLE*      g_SystemTable;
EFI_BOOT_SERVICES*     g_BootServices;
EFI_RUNTIME_SERVICES*  g_RuntimeServices;




#define TEXT(x) OutputText((const CHAR16*) L##x)
const CHAR16* OutputText(const CHAR16* Text)
{
    // Output to file for internationalization.
    return Text;
}
void Print(const CHAR16* string)
{
    g_SystemTable->ConOut->OutputString(g_SystemTable->ConOut, string);
}


#define EFI_ASSERT(status) EfiAssert(status, (const CHAR16*) FILE_WCHAR, __LINE__)
void EfiAssert(EFI_STATUS status, const CHAR16* file, int line)
{
    if ((status & EFI_ERROR) == EFI_ERROR) {
        g_SystemTable->ConOut->SetAttribute(g_SystemTable->ConOut, EFI_RED);
        Print(TEXT("[ASSERT FAILED]: "));
        Print(EfiErrorString(status));
        Print(TEXT("\r\n"));
    }
}


Block Allocate(usize size)
{
    Block block = { .data=NULL, .size=size };
    EFI_ASSERT(g_BootServices->AllocatePool(
            EfiLoaderData,
            size,
            (void **)&block.data
    ));
    return block;
}
void Deallocate(Block block)
{

}
Allocator g_Allocator = { .Allocate=Allocate, .Deallocate=Deallocate };


void FillMemoryRepeat(void* destination, int d_size, const void* source, int s_size)
{
    u8* dest = (u8 *) destination;
    const u8* sour = (u8 *) source;
    for (int i = 0; i < d_size; i += s_size)
        for (int j = 0; j < s_size; ++j)
            dest[i + j] = sour[j];
}


void ScreenReset()
{
    // UEFI-spec page 451
    // The Reset() function resets the text output device hardware.
    // The cursor position is set to (0, 0), and the screen is cleared to the
    // default background color for the output device.
    g_SystemTable->ConOut->Reset(g_SystemTable->ConOut, EFI_TRUE);
}

void KeyboardReset()
{
    // The implementation of Reset is required to clear the contents of any
    // input queues resident in memory used for buffering keystroke data and
    // put the input stream in a known empty state.
    g_SystemTable->ConIn->Reset(g_SystemTable->ConIn, EFI_TRUE);
}

EFI_STATUS KeyboardPoll(EFI_INPUT_KEY* Key)
{
    return g_SystemTable->ConIn->ReadKeyStroke(g_SystemTable->ConIn, Key);
}

EFI_INPUT_KEY KeyboardWait()
{
    EFI_INPUT_KEY Key;
    while ((g_SystemTable->ConIn->ReadKeyStroke(g_SystemTable->ConIn, &Key)) == EFI_NOT_READY);
    return Key;
}


EFI_STATUS ScreenBlit(
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

EFI_STATUS ScreenBlitFramebuffer(
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

void HardwareReboot()
{
    g_RuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, 0);
}

void SoftwareReboot()
{
    g_RuntimeServices->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, 0);
}

void Shutdown()
{
    g_RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, 0);
}


EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable)
{
    // The System Table contains pointers to other standard tables that a loaded
    // image may use if the associated pointers are initialized to nonzero values.
    g_SystemTable     = SystemTable;
    g_BootServices    = SystemTable->BootServices;
    g_RuntimeServices = SystemTable->RuntimeServices;

    ScreenReset();
    SystemTable->ConOut->SetAttribute(SystemTable->ConOut, EFI_RED);

    // Printing wide characters
    Print(TEXT("Testing print wide characters...\r\n"));

    // Printing narrow characters
    String  ascii = LITERAL_TO_STRING("Testing print narrow characters...\n\r");
    WString utf16 = AsciiToUtf16(ascii, g_Allocator);
    Print((CHAR16*) utf16.data);


    // Loading this info about this image.
    EFI_LOADED_IMAGE_PROTOCOL* LoadedImage = NULL;
    EFI_ASSERT(SystemTable->BootServices->HandleProtocol(
        ImageHandle,
        &EFI_LOADED_IMAGE_PROTOCOL_GUID,
        (void **)&LoadedImage
    ));

    HexArray hex = ToHexString((usize) LoadedImage->ImageBase);
    String   aaa = { .data=hex.data, .size=20 };
    WString  bbb = AsciiToUtf16(aaa, g_Allocator);
    Print(TEXT("Image loaded at: ")); Print(bbb.data); Print(TEXT("\n\r"));



    EFI_GRAPHICS_OUTPUT_PROTOCOL* GraphicsOutput = NULL;
    EFI_ASSERT(SystemTable->BootServices->LocateProtocol(
            &EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID, 0, (void**) &GraphicsOutput
    ));

//    const size_t size = 100 * 100;
//    EFI_GRAPHICS_OUTPUT_BLT_PIXEL FrameBuffer[100 * 100] = {};
//    FillMemoryRepeat(FrameBuffer, size, &EFI_GRAPHICS_RED, sizeof(EFI_GRAPHICS_RED));
//
//    ScreenBlit(
//        GraphicsOutput,
//        (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) { 0xFF, 0xFF, 0xFF, 0xFF },
//        10, 10, 10, 10
//    );
//
//    EFI_ASSERT(ScreenBlitFramebuffer(GraphicsOutput, FrameBuffer, 0, 0, 100, 100));


    EFI_TIME* Time = NULL;
    EFI_ASSERT(g_BootServices->AllocatePool(
        EfiBootServicesData,
        sizeof(EFI_TIME),
        (void **)&Time
    ));
    EFI_ASSERT(g_RuntimeServices->GetTime(Time, NULL));



    KeyboardReset();
    Print(TEXT("Press q to shutdown | Press r to reboot\n\r"));
    while (1)
    {
        EFI_INPUT_KEY Key = {};
        if (KeyboardPoll(&Key) == EFI_SUCCESS)
        {
            if (Key.UnicodeChar == 'q')
            {
                Shutdown();
                break;
            }
            else if (Key.UnicodeChar == 'r')
            {
                SoftwareReboot();
                break;
            }
            else
            {
                CHAR16 Temp[] = { Key.UnicodeChar, 0 };
                Print(TEXT("Please chose a valid key, not '"));
                Print(Temp);
                Print(TEXT("'...\n\r"));
            }
        }
    }




    int wait = 1;
    while (wait) {
        __asm__ __volatile__("pause");
    }

    return EFI_SUCCESS;
}

