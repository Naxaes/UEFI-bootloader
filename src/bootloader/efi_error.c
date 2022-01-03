#include "efi_error.h"


const CHAR16* EfiErrorString(EFI_STATUS status)
{
    switch (status)
    {
        case EFI_LOAD_ERROR:            return (const CHAR16*) L"Load Error";
        case EFI_INVALID_PARAMETER:     return (const CHAR16*) L"Invalid Parameter";
        case EFI_UNSUPPORTED:           return (const CHAR16*) L"Unsupported";
        case EFI_BAD_BUFFER_SIZE:       return (const CHAR16*) L"Bad Buffer Size";
        case EFI_BUFFER_TOO_SMALL:      return (const CHAR16*) L"Buffer Too Small";
        case EFI_NOT_READY:             return (const CHAR16*) L"Not Ready";
        case EFI_DEVICE_ERROR:          return (const CHAR16*) L"Device Error";
        case EFI_WRITE_PROTECTED:       return (const CHAR16*) L"Write Protected";
        case EFI_OUT_OF_RESOURCES:      return (const CHAR16*) L"Out Of Resources";
        case EFI_VOLUME_CORRUPTED:      return (const CHAR16*) L"Volume Corrupted";
        case EFI_VOLUME_FULL:           return (const CHAR16*) L"Volume Full";
        case EFI_NO_MEDIA:              return (const CHAR16*) L"No Media";
        case EFI_MEDIA_CHANGED:         return (const CHAR16*) L"Media Changed";
        case EFI_NOT_FOUND:             return (const CHAR16*) L"File Not Found";
        case EFI_ACCESS_DENIED:         return (const CHAR16*) L"Access Denied";
        case EFI_NO_RESPONSE:           return (const CHAR16*) L"No Response";
        case EFI_NO_MAPPING:            return (const CHAR16*) L"No Mapping";
        case EFI_TIMEOUT:               return (const CHAR16*) L"Timeout";
        case EFI_NOT_STARTED:           return (const CHAR16*) L"Not Started";
        case EFI_ALREADY_STARTED:       return (const CHAR16*) L"Already Started";
        case EFI_ABORTED:               return (const CHAR16*) L"Aborted";
        case EFI_ICMP_ERROR:            return (const CHAR16*) L"ICMP Error";
        case EFI_TFTP_ERROR:            return (const CHAR16*) L"TFTP Error";
        case EFI_PROTOCOL_ERROR:        return (const CHAR16*) L"Protocol Error";
        case EFI_INCOMPATIBLE_VERSION:  return (const CHAR16*) L"Incompatible Version";
        case EFI_SECURITY_VIOLATION:    return (const CHAR16*) L"Security Violation";
        case EFI_CRC_ERROR:             return (const CHAR16*) L"CRC Error";
        case EFI_END_OF_MEDIA:          return (const CHAR16*) L"End Of Media";
        case EFI_END_OF_FILE:           return (const CHAR16*) L"End Of File";
        case EFI_INVALID_LANGUAGE:      return (const CHAR16*) L"Invalid Language";
        case EFI_COMPROMISED_DATA:      return (const CHAR16*) L"Compromised Data";
        case EFI_IP_ADDRESS_CONFLICT:   return (const CHAR16*) L"IP Address Conflict";
        case EFI_HTTP_ERROR:            return (const CHAR16*) L"End Of File";
        case EFI_WARN_UNKNOWN_GLYPH:    return (const CHAR16*) L"WARNING - Unknown Glyph";
        case EFI_WARN_DELETE_FAILURE:   return (const CHAR16*) L"WARNING - Delete Failure";
        case EFI_WARN_WRITE_FAILURE:    return (const CHAR16*) L"WARNING - Write Failure";
        case EFI_WARN_BUFFER_TOO_SMALL: return (const CHAR16*) L"WARNING - Buffer Too Small";
        case EFI_WARN_STALE_DATA:       return (const CHAR16*) L"WARNING - Stale Data";
        case EFI_WARN_FILE_SYSTEM:      return (const CHAR16*) L"WARNING - File System";
        case EFI_WARN_RESET_REQUIRED:   return (const CHAR16*) L"WARNING - Reset Required";
        case EFI_SUCCESS:               return (const CHAR16*) L"Successful";
        default:                        return (const CHAR16*) L"ERROR";
    }
}