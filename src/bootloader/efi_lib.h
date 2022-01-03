#pragma once
#include "efi.h"

#define MAKE_WIDE_STRING_PREFIX(x) L##x
#define MAKE_WIDE_STRING(x) MAKE_WIDE_STRING_PREFIX(x)
#define FILE_WCHAR MAKE_WIDE_STRING(__FILE__)


#define EfiHalt() debug_break()
void debug_break();

void EfiPrintString(const CHAR16* string);
void EfiPrintChar(CHAR16 character);
#define EfiPrintF(...) printf(__VA_ARGS__)
int printf(const CHAR16* format, ...);

#define EFI_ASSERT(status) EfiAssert(status, (const CHAR16*) FILE_WCHAR, __LINE__);
void EfiAssert(EFI_STATUS status, const CHAR16* file, int line);

EFI_STATUS EfiKeyboardPoll(EFI_INPUT_KEY* Key);
void EfiKeyboardWait();

void EfiDelay(UINTN ms);
void EfiPrintCurrentTime();

void EfiHardwareReboot();
void EfiSoftwareReboot();
void EfiShutdown();