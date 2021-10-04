// UEFI From Scratch Tutorials - ThatOSDev ( 2021 )
// https://github.com/ThatOSDev/UEFI-Tuts

#pragma once

// UEFI 2.9 Specs PDF Page 2194 - 2196

#define EFI_SUCCESS               0x0000000000000000
#define EFI_ERROR                 0x8000000000000000 
#define EFI_LOAD_ERROR            (EFI_ERROR | 0x01)
#define EFI_INVALID_PARAMETER     (EFI_ERROR | 0x02)
#define EFI_UNSUPPORTED           (EFI_ERROR | 0x03)
#define EFI_BAD_BUFFER_SIZE       (EFI_ERROR | 0x04)
#define EFI_BUFFER_TOO_SMALL      (EFI_ERROR | 0x05)
#define EFI_NOT_READY             (EFI_ERROR | 0x06)
#define EFI_DEVICE_ERROR          (EFI_ERROR | 0x07)
#define EFI_WRITE_PROTECTED       (EFI_ERROR | 0x08)
#define EFI_OUT_OF_RESOURCES      (EFI_ERROR | 0x09)
#define EFI_VOLUME_CORRUPTED      (EFI_ERROR | 0x0A)
#define EFI_VOLUME_FULL           (EFI_ERROR | 0x0B)
#define EFI_NO_MEDIA              (EFI_ERROR | 0x0C)
#define EFI_MEDIA_CHANGED         (EFI_ERROR | 0x0D)
#define EFI_NOT_FOUND             (EFI_ERROR | 0x0E)
#define EFI_ACCESS_DENIED         (EFI_ERROR | 0x0F)
#define EFI_NO_RESPONSE           (EFI_ERROR | 0x10)
#define EFI_NO_MAPPING            (EFI_ERROR | 0x11)
#define EFI_TIMEOUT               (EFI_ERROR | 0x12)
#define EFI_NOT_STARTED           (EFI_ERROR | 0x13)
#define EFI_ALREADY_STARTED       (EFI_ERROR | 0x14)
#define EFI_ABORTED               (EFI_ERROR | 0x15)
#define EFI_ICMP_ERROR            (EFI_ERROR | 0x16)
#define EFI_TFTP_ERROR            (EFI_ERROR | 0x17)
#define EFI_PROTOCOL_ERROR        (EFI_ERROR | 0x18)
#define EFI_INCOMPATIBLE_VERSION  (EFI_ERROR | 0x19)
#define EFI_SECURITY_VIOLATION    (EFI_ERROR | 0x1A)
#define EFI_CRC_ERROR             (EFI_ERROR | 0x1B)
#define EFI_END_OF_MEDIA          (EFI_ERROR | 0x1C)
#define EFI_END_OF_FILE           (EFI_ERROR | 0x1D)
#define EFI_INVALID_LANGUAGE      (EFI_ERROR | 0x1E)
#define EFI_COMPROMISED_DATA      (EFI_ERROR | 0x1F)
#define EFI_IP_ADDRESS_CONFLICT   (EFI_ERROR | 0x20)
#define EFI_HTTP_ERROR            (EFI_ERROR | 0x21)

#define EFI_WARN_UNKNOWN_GLYPH     0x01
#define EFI_WARN_DELETE_FAILURE    0x02
#define EFI_WARN_WRITE_FAILURE     0x03
#define EFI_WARN_BUFFER_TOO_SMALL  0x04
#define EFI_WARN_STALE_DATA        0x05
#define EFI_WARN_FILE_SYSTEM       0x06
#define EFI_WARN_RESET_REQUIRED    0x07


const CHAR16* EfiErrorString(EFI_STATUS status)
{
    switch (status)
    {
        case EFI_LOAD_ERROR:
        {
            return (const CHAR16*) L"Load Error";
        }
        case EFI_INVALID_PARAMETER:
        {
            return (const CHAR16*) L"Invalid Parameter";
        }
        case EFI_UNSUPPORTED:
        {
            return (const CHAR16*) L"Unsupported";
        }
        case EFI_BAD_BUFFER_SIZE:
        {
            return (const CHAR16*) L"Bad Buffer Size";
        }
        case EFI_BUFFER_TOO_SMALL:
        {
            return (const CHAR16*) L"Buffer Too Small";
        }
        case EFI_NOT_READY:
        {
            return (const CHAR16*) L"Not Ready";
        }
        case EFI_DEVICE_ERROR:
        {
            return (const CHAR16*) L"Device Error";
        }
        case EFI_WRITE_PROTECTED:
        {
            return (const CHAR16*) L"Write Protected";
        }
        case EFI_OUT_OF_RESOURCES:
        {
            return (const CHAR16*) L"Out Of Resources";
        }
        case EFI_VOLUME_CORRUPTED:
        {
            return (const CHAR16*) L"Volume Corrupted";
        }
        case EFI_VOLUME_FULL:
        {
            return (const CHAR16*) L"Volume Full";
        }
        case EFI_NO_MEDIA:
        {
            return (const CHAR16*) L"No Media";
        }
        case EFI_MEDIA_CHANGED:
        {
            return (const CHAR16*) L"Media Changed";
        }
        case EFI_NOT_FOUND:
        {
            return (const CHAR16*) L"File Not Found";
        }
        case EFI_ACCESS_DENIED:
        {
            return (const CHAR16*) L"Access Denied";
        }
        case EFI_NO_RESPONSE:
        {
            return (const CHAR16*) L"No Response";
        }
        case EFI_NO_MAPPING:
        {
            return (const CHAR16*) L"No Mapping";
        }
        case EFI_TIMEOUT:
        {
            return (const CHAR16*) L"Timeout";
        }
        case EFI_NOT_STARTED:
        {
            return (const CHAR16*) L"Not Started";
        }
        case EFI_ALREADY_STARTED:
        {
            return (const CHAR16*) L"Already Started";
        }
        case EFI_ABORTED:
        {
            return (const CHAR16*) L"Aborted";
        }
        case EFI_ICMP_ERROR:
        {
            return (const CHAR16*) L"ICMP Error";
        }
        case EFI_TFTP_ERROR:
        {
            return (const CHAR16*) L"TFTP Error";
        }
        case EFI_PROTOCOL_ERROR:
        {
            return (const CHAR16*) L"Protocol Error";
        }
        case EFI_INCOMPATIBLE_VERSION:
        {
            return (const CHAR16*) L"Incompatible Version";
        }
        case EFI_SECURITY_VIOLATION:
        {
            return (const CHAR16*) L"Security Violation";
        }
        case EFI_CRC_ERROR:
        {
            return (const CHAR16*) L"CRC Error";
        }
        case EFI_END_OF_MEDIA:
        {
            return (const CHAR16*) L"End Of Media";
        }
        case EFI_END_OF_FILE:
        {
            return (const CHAR16*) L"End Of File";
        }
        case EFI_INVALID_LANGUAGE:
        {
            return (const CHAR16*) L"Invalid Language";
        }
        case EFI_COMPROMISED_DATA:
        {
            return (const CHAR16*) L"Compromised Data";
        }
        case EFI_IP_ADDRESS_CONFLICT:
        {
            return (const CHAR16*) L"IP Address Conflict";
        }
        case EFI_HTTP_ERROR:
        {
            return (const CHAR16*) L"End Of File";
        }
        case EFI_WARN_UNKNOWN_GLYPH:
        {
            return (const CHAR16*) L"WARNING - Unknown Glyph";
        }
        case EFI_WARN_DELETE_FAILURE:
        {
            return (const CHAR16*) L"WARNING - Delete Failure";
        }
        case EFI_WARN_WRITE_FAILURE:
        {
            return (const CHAR16*) L"WARNING - Write Failure";
        }
        case EFI_WARN_BUFFER_TOO_SMALL:
        {
            return (const CHAR16*) L"WARNING - Buffer Too Small";
        }
        case EFI_WARN_STALE_DATA:
        {
            return (const CHAR16*) L"WARNING - Stale Data";
        }
        case EFI_WARN_FILE_SYSTEM:
        {
            return (const CHAR16*) L"WARNING - File System";
        }
        case EFI_WARN_RESET_REQUIRED:
        {
            return (const CHAR16*) L"WARNING - Reset Required";
        }
        case EFI_SUCCESS:
        {
            return (const CHAR16*) L"Successful";
        }
        default:
        {
            return (const CHAR16*) L"ERROR";
        }
    }
}