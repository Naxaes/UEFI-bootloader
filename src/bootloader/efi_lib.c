#include "efi_lib.h"

#include <stdarg.h>

#include "efi_error.h"

extern EFI_SYSTEM_TABLE*      g_SystemTable;
extern EFI_BOOT_SERVICES*     g_BootServices;
extern EFI_RUNTIME_SERVICES*  g_RuntimeServices;

#include "../format.c"

void debug_break()
{
    // Change to 0 in debugger to continue.
    int volatile wait = 1;
    while (wait) {
        __asm__ __volatile__("pause");
    }
}



/* ---- PRINT ---- */
void EfiPrintString(const CHAR16* string)
{
    g_SystemTable->ConOut->OutputString(g_SystemTable->ConOut, string);
}

void EfiPrintChar(CHAR16 character)
{
    CHAR16 string[] = { character, L'\0' };
    g_SystemTable->ConOut->OutputString(g_SystemTable->ConOut, string);
}

int printf(const CHAR16* format, ...)
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
                    int i = va_arg(arg, int);  // Fetch char argument
                    EfiPrintChar((CHAR16) i);
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
                    EfiPrintString((const CHAR16 *) U64ToString((usize) i, 10).data);
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
                    usize i = va_arg(arg, usize); //Fetch Hexadecimal representation
                    EfiPrintString((const CHAR16 *) ToHexStringTruncated(i).data);
                    break;
                }
                case L'z':  // size_t or ssize_t
                {
                    character++;
                    if (*character == L'u')
                    {
                        usize x = va_arg(arg, usize);
                        EfiPrintString((const CHAR16 *) U64ToString(x, 10).data);
                        break;
                    }
                    else
                    {
                        return -1;
                    }
                }
                default:
                    return -1;
            }
            character++;
        }
    }

    va_end(arg);
    return 0;
}


void EfiAssert(EFI_STATUS status, const CHAR16* file, int line)
{
    if ((status & EFI_ERROR) == EFI_ERROR)
    {
        g_SystemTable->ConOut->SetAttribute(g_SystemTable->ConOut, EFI_RED);
        EfiPrintF(L"[ASSERT FAILED] (%s:%d): '%s'\n\r", file, line, EfiErrorString(status));
        EfiHalt();
    }
}


/* ---- KEYBOARD ----*/
EFI_STATUS EfiKeyboardPoll(EFI_INPUT_KEY* Key)
{
    return g_SystemTable->ConIn->ReadKeyStroke(g_SystemTable->ConIn, Key);
}

void EfiKeyboardWait()
{
    EFI_INPUT_KEY Key;
    while (g_SystemTable->ConIn->ReadKeyStroke(g_SystemTable->ConIn, &Key) == EFI_NOT_READY);
}


/* ---- CLOCK ---- */
void EfiDelay(UINTN ms)
{
    // The Stall function is set as microseconds.
    g_BootServices->Stall(ms);
}

void EfiPrintCurrentTime()
{
    EFI_TIME* Time = NULL;
    EFI_ASSERT(g_BootServices->AllocatePool(EfiBootServicesData, sizeof(EFI_TIME), (void **)&Time));
    EFI_ASSERT(g_RuntimeServices->GetTime(Time, NULL));
    CHAR16 Hour[] = { L'0' + Time->Hour / 10, L'0' + Time->Hour % 10, 0 };
    EfiPrintF(L"Current hour: %s\n\r", Hour);
}


/* ---- REBOOT ----*/
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