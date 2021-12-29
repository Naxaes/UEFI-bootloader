#include "efi_main.h"

#include "format.c"

#define EFI_ASSERT(status) EfiAssert(status, (const CHAR16*) FILE_WCHAR, __LINE__)
void EfiAssert(EFI_STATUS status, const CHAR16* file, int line);


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
    EFI_ASSERT(g_BootServices->Stall(ms));
}


void EfiPrintString(const CHAR16* string)
{
    EFI_ASSERT(g_SystemTable->ConOut->OutputString(g_SystemTable->ConOut, string));
}

void EfiPrintChar(CHAR16 character)
{
    CHAR16 string[] = { character, L'\0' };
    EFI_ASSERT(g_SystemTable->ConOut->OutputString(g_SystemTable->ConOut, string));
}

void EfiPrintF(const CHAR16* format, ...)
{
    va_list arg;
    va_start(arg, format);

    const CHAR16* character = format;
    while (*character != L'\0')
    {
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
                }
                default:
                {
                    return;
                }
            }
            character++;
        }
    }

    va_end(arg);
}


void EfiAssert(EFI_STATUS status, const CHAR16* file, int line)
{
    if ((status & EFI_ERROR) == EFI_ERROR)
    {
        EFI_ASSERT(g_SystemTable->ConOut->SetAttribute(g_SystemTable->ConOut, EFI_RED));
        EfiPrintF(L"[ASSERT FAILED] (%s:%d): '%s'\n\r", file, line, EfiErrorStringW(status));
        EfiHalt();
    }
}


EFI_STATUS EfiKeyboardPoll(EFI_INPUT_KEY* Key)
{
    return g_SystemTable->ConIn->ReadKeyStroke(g_SystemTable->ConIn, Key);
}

void EfiKeyboardWait()
{
    EFI_INPUT_KEY Key;
    while (g_SystemTable->ConIn->ReadKeyStroke(g_SystemTable->ConIn, &Key) == EFI_NOT_READY);
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


EFI_FILE_PROTOCOL* EfiOpenFile(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* Volume, CHAR16* Path)
{
    EFI_FILE_PROTOCOL* Root = NULL;
    EFI_ASSERT(Volume->OpenVolume(Volume, &Root));

    EFI_FILE_PROTOCOL* FileHandle = NULL;
    EFI_ASSERT(Root->Open(Root, &FileHandle, Path, 0x01, 0));

    return FileHandle;
}


u8* EfiReadFile(EFI_FILE_PROTOCOL* File, UINTN size)
{
//    UINTN FileInfoSize;
//    EFI_FILE_INFO* FileInfo;
//    File->GetInfo(File, &EFI_FILE_INFO_ID_GUI, &FileInfoSize, NULL);
//    g_BootServices->AllocatePool(EfiLoaderData, FileInfoSize, (void**)&FileInfo);
//    File->GetInfo(File, &EFI_FILE_INFO_ID_GUI, &FileInfoSize, (void**)&FileInfo);

    u8* data = NULL;
    EFI_ASSERT(g_BootServices->AllocatePool(
            EfiLoaderData,
            size,
            (void **) &data
    ));
    EFI_ASSERT(File->Read(File, &size, data));

    return data;
}




PSF1_Font EfiLoadFont(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* Volume)
{
    PSF1_Font font = { .scale=1 };

    EFI_FILE_PROTOCOL* File = EfiOpenFile(Volume, L"default-font.psf");
    if (File)
    {
        UINTN size = sizeof(PSF1_Header);
        EFI_ASSERT(File->Read(File, &size, &font.header));

        if (font.header.magic[0] != 0x36 || font.header.magic[1] != 0x04)
        {
            EfiPrintF(L"Wrong magic number for font!\n\r");
            EfiHalt();
        }

        size = font.header.font_height * 256;
        if (font.header.file_mode == 1)
            size = font.header.font_height * 512;

        EFI_ASSERT(File->SetPosition(File, sizeof(PSF1_Header)));
        EFI_ASSERT(g_BootServices->AllocatePool(EfiLoaderData, size, (void**)&font.glyphs));
        EfiPrintF(L"1    %zu  %zu\n\r", (u64) font.glyphs, size);
        EFI_ASSERT(File->Read(File, &size, font.glyphs));

    }

    File->Close(File);

    return font;
}