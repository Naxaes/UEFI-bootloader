#pragma once

#include <stdint.h>

typedef enum ElfResult {
    ELF_NO  = 0,
    ELF_YES = 1,
    ELF_ERROR = -1
} ElfResult;


inline ElfResult is_elf64(const uint8_t* source)
{
    if (source[4] == 1)
        return ELF_NO;
    else if (source[4] == 2)
        return ELF_YES;
    else
        return ELF_ERROR;  // Invalid
}


typedef struct Elf64Header {
    uint32_t magic_number;      // 0x7F followed by 'ELF'
    uint8_t  bits;              // 1 = 32 bit, 2 = 64 bit
    uint8_t  endianness;        // 1 = LE, 2 = BE
    uint8_t  version_1;
    uint8_t  abi;               // 0 = System V
    uint8_t  _unused[8];
    uint16_t type;              // 1 = relocatable, 2 = executable, 3 = shared
    uint16_t instruction_set;   // 0x03 = x86, 0x28 = ARM, 0x3E = x86-64, 0xB7 = AArch64
    uint32_t version_2;
    uint64_t entry_point;
    uint64_t program_header_offset;
    uint64_t section_header_offset;
    uint32_t flags;
    uint16_t header_size;
    uint16_t program_header_entry_size;
    uint16_t program_header_entries;
    uint16_t section_header_entry_size;
    uint16_t section_header_entries;
    uint16_t section_name_index;
} __attribute__((packed)) Elf64Header;


typedef struct Elf64ProgramHeader {
    uint32_t type;              // 0 = unused, 1 = load, 2 = dynamic, 3 = interp, 4 = note
    uint32_t flags;             // 1 = executable, 2 = writable, 4 = readable
    uint64_t file_offset;
    uint64_t virtual_address;
    uint64_t _unused0;          // On systems where physical address is relevant, reserved for segment's physical address.
    uint64_t file_size;
    uint64_t memory_size;
    uint64_t alignment;
} __attribute__((packed)) Elf64ProgramHeader;


typedef struct Elf64SectionHeader {
    uint32_t name;      // Offset into section occupied_pages->section_name_index.
    uint32_t type;      // 4 = rela
    uint64_t flags;
    uint64_t address;
    uint64_t offset;
    uint64_t size;
    uint32_t link;
    uint32_t info;
    uint64_t align;
    uint64_t entry_size;
} __attribute__((packed)) Elf64SectionHeader;





// ---- PROGRAM TYPES ----
const uint32_t PT_NULL    = 0x00000000;  // Program occupied_pages table entry unused.
const uint32_t PT_LOAD    = 0x00000001;  // Loadable segment.
const uint32_t PT_DYNAMIC = 0x00000002;  // Dynamic linking information.
const uint32_t PT_INTERP  = 0x00000003;  // Interpreter information.
const uint32_t PT_NOTE    = 0x00000004;  // Auxiliary information.
const uint32_t PT_SHLIB   = 0x00000005;  // Reserved.
const uint32_t PT_PHDR    = 0x00000006;  // Segment containing program occupied_pages table itself.
const uint32_t PT_TLS     = 0x00000007;  // Thread-Local Storage template.
const uint32_t PT_LOOS    = 0x60000000;  // Reserved inclusive range. Operating system specific.
const uint32_t PT_HIOS    = 0x6FFFFFFF;  // Reserved inclusive range. Operating system specific.
const uint32_t PT_LOPROC  = 0x70000000;  // Reserved inclusive range. Processor specific.
const uint32_t PT_HIPROC  = 0x7FFFFFFF;  // Reserved inclusive range. Processor specific.


// ---- SECTION TYPES ----
// https://refspecs.linuxfoundation.org/LSB_3.0.0/LSB-PDA/LSB-PDA.junk/sections.html
const uint32_t SHT_NULL          = 0x0;    // This value marks the section occupied_pages as inactive; it does not have an associated section. Other members of the section occupied_pages have undefined values.
const uint32_t SHT_PROGBITS      = 0x1;    // The section holds information defined by the program, whose format and meaning are determined solely by the program.
const uint32_t SHT_SYMTAB        = 0x2;    // This section holds a symbol table. Currently, an object file may have either a section of SHT_SYMTAB type or a section of SHT_DYNSYM type, but not both. This restriction may be relaxed in the future. Typically, SHT_SYMTAB provides symbols for link editing, though it may also be used for dynamic linking. As a complete symbol table, it may contain many symbols unnecessary for dynamic linking.
const uint32_t SHT_STRTAB        = 0x3;    // The section holds a string table. An object file may have multiple string table sections. See `String Table' below for details.
const uint32_t SHT_RELA          = 0x4;    // The section holds relocation entries with explicit addends, such as type Elf32_Rela for the 32-bit class of object files or type Elf64_Rela for the 64-bit class of object files. An object file may have multiple relocation sections. `Relocation' b
const uint32_t SHT_HASH          = 0x5;    // The section holds a symbol hash table. Currently, an object file shall have only one hash table, but this restriction may be relaxed in the future. See `Hash Table' in the Chapter 5 for details.
const uint32_t HT_DYNAMIC        = 0x6;    // The section holds information for dynamic linking. Currently, an object file shall have only one dynamic section, but this restriction may be relaxed in the future. See `Dynamic Section' in Chapter 5 for details.
const uint32_t SHT_NOTE          = 0x7;    // The section holds information that marks the file in some way. See `Note Section' in Chapter 5 for details.
const uint32_t SHT_NOBITS        = 0x8;    // A section of this type occupies no space in the file but otherwise resembles SHT_PROGBITS. Although this section contains no bytes, the sh_offset member contains the conceptual file offset.
const uint32_t SHT_REL           = 0x9;    // The section holds relocation entries without explicit addends, such as type Elf32_Rel for the 32-bit class of object files or type Elf64_Rel for the 64-bit class of object files. An object file may have multiple relocation sections. See "Relocation"
const uint32_t SHT_DYNSYM        = 0xb;    // This section holds a minimal set of symbols adequate for dynamic linking. See also SHT_SYMTAB. Currently, an object file may have either a section of SHT_SYMTAB type or a section of SHT_DYNSYM type, but not both. This restriction may be relaxed in the future.
const uint32_t SHT_INIT_ARRAY    = 0xe;    // This section contains an array of pointers to initialization functions, as described in `Initialization and Termination Functions' in Chapter 5. Each pointer in the array is taken as a parameterless procedure with a void return.
const uint32_t SHT_FINI_ARRAY    = 0xf;    // This section contains an array of pointers to termination functions, as described in `Initialization and Termination Functions' in Chapter 5. Each pointer in the array is taken as a parameterless procedure with a void return.
const uint32_t SHT_PREINIT_ARRAY = 0x10;   // This section contains an array of pointers to functions that are invoked before all other initialization functions, as described in `Initialization and Termination Functions' in Chapter 5. Each pointer in the array is taken as a parameterless proceure with a void return.
const uint32_t SHT_GROUP         = 0x11;   // Section group
const uint32_t SHT_SYMTAB_SHNDX  = 0x12;   // Extended section indices
const uint32_t SHT_NUM           = 0x13;   // Number of defined types.

// ---- SECTION FLAGS ----
const uint64_t SHF_WRITE            = 0x1;         // Writable
const uint64_t SHF_ALLOC            = 0x2;         // Occupies memory during execution
const uint64_t SHF_EXECINSTR        = 0x4;         // Executable
const uint64_t SHF_MERGE            = 0x10;        // Might be merged
const uint64_t SHF_STRINGS          = 0x20;        // Contains null-terminated strings
const uint64_t SHF_INFO_LINK        = 0x40;        // 'sh_info' contains SHT index
const uint64_t SHF_LINK_ORDER       = 0x80;        // Preserve order after combining
const uint64_t SHF_OS_NONCONFORMING = 0x100;       // Non-standard OS specific handling required
const uint64_t SHF_GROUP            = 0x200;       // Section is member of a group
const uint64_t SHF_TLS              = 0x400;       // Section hold thread-local data
const uint64_t SHF_MASKOS           = 0x0ff00000;  // OS-specific
const uint64_t SHF_MASKPROC         = 0xf0000000;  // Processor-specific
const uint64_t SHF_ORDERED          = 0x4000000;   // Special ordering requirement (Solaris)
const uint64_t SHF_EXCLUDE          = 0x8000000;   // Section is excluded unless referenced or allocated (Solaris)
