#include "efi.h"

// The GUID to set the correct Protocol.
// These GUIDs are all over the UEFI 2.9 Specs PDF.
struct EFI_GUID EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID    = {0x9042a9de,  0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}};
struct EFI_GUID EFI_LOADED_IMAGE_PROTOCOL_GUID       = {0x5b1b31a1,  0x9562, 0x11d2, {0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};
struct EFI_GUID EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID = {0x0964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};
struct EFI_GUID EFI_DEVICE_PATH_PROTOCOL_GUID        = {0x09576e91,  0x6d3f, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};
struct EFI_GUID EFI_FILE_INFO_ID_GUI                 = {0x09576e92,  0x6d3f, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};

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

const char* EFI_MEMORY_TYPE_STRINGS_CHAR[] = {
        "EfiReservedMemoryType",
        "EfiLoaderCode",
        "EfiLoaderData",
        "EfiBootServicesCode",
        "EfiBootServicesData",
        "EfiRuntimeServicesCode",
        "EfiRuntimeServicesData",
        "EfiConventionalMemory",
        "EfiUnusableMemory",
        "EfiACPIReclaimMemory",
        "EfiACPIMemoryNVS",
        "EfiMemoryMappedIO",
        "EfiMemoryMappedIOPortSpace",
        "EfiPalCode",
        "EfiPersistentMemory",
        "EfiUnacceptedMemoryType",
        "EfiMaxMemoryType"
};
