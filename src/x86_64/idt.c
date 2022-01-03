#include "x86_64.h"

// Each define here is for a specific flag in the descriptor.
// Refer to the intel documentation for a description of what each one does.
#define SEG_DESCTYPE(x)  ((x) << 0x04) // Descriptor type (0 for system, 1 for code/data)
#define SEG_PRES(x)      ((x) << 0x07) // Present
#define SEG_SAVL(x)      ((x) << 0x0C) // Available for system use
#define SEG_LONG(x)      ((x) << 0x0D) // Long mode
#define SEG_SIZE(x)      ((x) << 0x0E) // Size (0 for 16-bit, 1 for 32)
#define SEG_GRAN(x)      ((x) << 0x0F) // Granularity (0 for 1B - 1MB, 1 for 4KB - 4GB)
#define SEG_PRIV(x)     (((x) &  0x03) << 0x05)   // Set privilege level (0 - 3)

#define SEG_DATA_RD        0x00  // Read-Only
#define SEG_DATA_RDA       0x01  // Read-Only, accessed
#define SEG_DATA_RDWR      0x02  // Read/Write
#define SEG_DATA_RDWRA     0x03  // Read/Write, accessed
#define SEG_DATA_RDEXPD    0x04  // Read-Only, expand-down
#define SEG_DATA_RDEXPDA   0x05  // Read-Only, expand-down, accessed
#define SEG_DATA_RDWREXPD  0x06  // Read/Write, expand-down
#define SEG_DATA_RDWREXPDA 0x07  // Read/Write, expand-down, accessed
#define SEG_CODE_EX        0x08  // Execute-Only
#define SEG_CODE_EXA       0x09  // Execute-Only, accessed
#define SEG_CODE_EXRD      0x0A  // Execute/Read
#define SEG_CODE_EXRDA     0x0B  // Execute/Read, accessed
#define SEG_CODE_EXC       0x0C  // Execute-Only, conforming
#define SEG_CODE_EXCA      0x0D  // Execute-Only, conforming, accessed
#define SEG_CODE_EXRDC     0x0E  // Execute/Read, conforming
#define SEG_CODE_EXRDCA    0x0F  // Execute/Read, conforming, accessed

#define GDT_CODE_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                     SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                     SEG_PRIV(0)     | SEG_CODE_EXRD

#define GDT_DATA_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                     SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                     SEG_PRIV(0)     | SEG_DATA_RDWR

#define GDT_CODE_PL3 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                     SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                     SEG_PRIV(3)     | SEG_CODE_EXRD

#define GDT_DATA_PL3 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                     SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                     SEG_PRIV(3)     | SEG_DATA_RDWR


#define IDT_INTERRUPT_GATE  0xE
#define IDT_TRAP_GATE       0xF
#define IDT_RING0           (0 << 5)
#define IDT_RING3           (3 << 5)
#define IDT_PRESENT         (1 << 7)


struct TaskStateSegment
{
    u32 reserved_0;
    u64 privilege_stack_table[3];
    u64 reserved_1;
    u64 interrupt_stack_table[7];
    u64 reserved_2;
    u16 reserved_3;
    u16 io_map_base_address;
} __attribute__((packed)) tss;


typedef struct
{
    u16 limit_0_15;
    u16 base_0;
    u8  base_1;
    u8  access;
    u8  type_flags_and_limit_16_19;
    u8  base_2;
} __attribute__((packed)) GlobalDescriptorEntry;

#define GDT_ENTRY(access_, flags) (GlobalDescriptorEntry) {                   \
    .limit_0_15=0,                                                            \
    .base_0=0,                                                                \
    .base_1=0,                                                                \
    .access=(access_),                                                        \
    .type_flags_and_limit_16_19=(flags),                                      \
    .base_2=0                                                                 \
}

typedef struct
{
    usize ip;
    usize cs;
    usize flags;
    usize sp;
    usize ss;
} __attribute__((packed)) InterruptFrame;


__attribute__ ((interrupt))
static void panic_interrupt_handler(InterruptFrame* frame)
{
    ERROR_LOGGER(
        "[Interrupt]: Unhandled exception!\n\r"
        "    ip:    %zu\n\r"
        "    cs:    %zu\n\r"
        "    flags: %zu\n\r"
        "    sp:    %zu\n\r"
        "    ss:    %zu\n\r",
        frame->ip, frame->cs, frame->flags, frame->sp, frame->ss
    );

    debug_break();
}


typedef struct
{
    u16 offset_1;        // offset bits 0..15
    u16 selector;        // a code segment selector in GDT or LDT
    u8  ist;             // bits 0..2 holds Interrupt Stack Table offset, rest of bits zero.
    u8  type_attributes; // gate type, dpl, and p fields
    u16 offset_2;        // offset bits 16..31
    u32 offset_3;        // offset bits 32..63
    u32 reserved;        // reserved
} __attribute__((packed)) InterruptDescriptor;


#define IDT_ENTRY(handler, selector_, flags) (InterruptDescriptor) {           \
    .offset_1=(u16)((u64)(handler) & 0xFFFF),                                  \
    .selector=(selector_),                                                     \
    .ist=0,                                                                    \
    .type_attributes=(flags),                                                  \
    .offset_2=(u16)(((u64)(handler) >> 16) & 0xFFFF),                          \
    .offset_3=(u16)(((u64)(handler) >> 32) & 0xFFFF),                          \
    .reserved=0,                                                               \
}


typedef enum {
    //    Name/nr                          Type       Mnemonic     Error code
    DivideByZero=0,                     // Fault        #DE            No
    Debug=1,                            // Fault/Trap   #DB            No
    NonmaskableInterrupt=2,             // Interrupt     -             No
    Breakpoint=3,                       // Trap         #BP            No
    Overflow=4,                         // Trap         #OF            No
    BoundRangeExceeded=5,               // Fault        #BR            No
    InvalidOpcode=6,                    // Fault        #UD            No
    DeviceNotAvailable=7,               // Fault        #NM            No
    DoubleFault=8,                      // Abort        #DF            Yes (Zero)
    CoprocessorSegmentOverrun=9,        // Fault         -             No
    InvalidTSS=10,                      // Fault        #TS            Yes
    SegmentNotPresent=11,               // Fault        #NP            Yes
    StackSegmentFault=12,               // Fault        #SS            Yes
    GeneralProtectionFault=13,          // Fault        #GP            Yes
    PageFault=14,                       // Fault        #PF            Yes

    x87FloatingPointException=16,       // Fault        #MF            No
    AlignmentCheck=17,                  // Fault        #AC            Yes
    MachineCheck=18,                    // Abort        #MC            No
    SIMDFloatingPointException=19,      // Fault        #XM/#XF        No
    VirtualizationException=20,         // Fault        #VE            No
    ControlProtectionException=21,      // Fault        #CP            Yes

    HypervisorInjectionException=28,    // Fault        #HV            No
    VMMCommunicationException=29,       // Fault        #VC            Yes
    SecurityException=30,               // Fault        #SX            Yes

    TripleFault
} InterruptIndex;


// https://wiki.osdev.org/Interrupts
// https://wiki.osdev.org/Exceptions
static InterruptDescriptor   idt[256] = { };
//static GlobalDescriptorEntry gdt[8]   = { };


void set_interrupt_handler(int id, __attribute__ ((interrupt)) void (*handler)(InterruptFrame*))
{
    idt[id] = IDT_ENTRY(handler, 0x8, IDT_INTERRUPT_GATE | IDT_RING0 | IDT_PRESENT);
}


//static u8 temp_stack[5 * 4096] = {};


void idt_install()
{
//    gdt[0] = GDT_ENTRY(0, 0);  // NULL entry
//    gdt[1] = GDT_ENTRY(0b10011010, 0b10100000);  // The kernel code segment descriptor.
//    gdt[2] = GDT_ENTRY(0b10010010, 0b10100000);  // The kernel data segment descriptor.
//    gdt[3] = GDT_ENTRY(0b10011010, 0b10100000);  // The user code segment descriptor.
//    gdt[4] = GDT_ENTRY(0b10010010, 0b10100000);  // The user data segment descriptor.
//    gdt[5] = GDT_ENTRY(0b10011010, 0b10001001);  // The TSS low section segment descriptor.
//    gdt[6] = GDT_ENTRY(0, 0);                    // The TSS high section segment descriptor.


    x86_64_load_gdt();

//    tss.interrupt_stack_table[0] = (u64) (temp_stack + sizeof(temp_stack));


    /* Sets the special IDT pointer up, just like in 'gdt.c' */
    u16 limit = (sizeof(InterruptDescriptor) * 256) - 1;
    u64 base  = (u64) &idt;

    /* Clear out the entire IDT, initializing it to zeros */
    memset(&idt, 0, sizeof(InterruptDescriptor) * 256);

    /* Add any new ISRs to the IDT here using idt_set_gate */
    for (int i = 0; i < 255; ++i)
    {
        set_interrupt_handler(i, panic_interrupt_handler);
    }

    /* Points the processor's internal register to the new IDT */
    struct __attribute__((packed)) { u16 size; u64 idt; } description  = { limit, base };
    __asm__ __volatile__ ("lidt %0" : : "m"(description));
}



//struct tss {
//    u32 reserved0;
//    u64 rsp0;
//    u64 rsp1;
//    u64 rsp2;
//    u64 reserved1;
//    u64 ist1;
//    u64 ist2;
//    u64 ist3;
//    u64 ist4;
//    u64 ist5;
//    u64 ist6;
//    u64 ist7;
//    u64 reserved2;
//    u16 reserved3;
//    u16 iopb_offset;
//} __attribute__((packed)) tss;
//
//__attribute__((aligned(4096))) struct {
//    struct gdt_entry null;
//    struct gdt_entry kernel_code;
//    struct gdt_entry kernel_data;
//    struct gdt_entry null2;
//    struct gdt_entry user_data;
//    struct gdt_entry user_code;
//    struct gdt_entry ovmf_data;
//    struct gdt_entry ovmf_code;
//    struct gdt_entry tss_low;
//    struct gdt_entry tss_high;
//} gdt_table = {
//        {0, 0, 0, 0x00, 0x00, 0},  /* 0x00 null  */
//        {0, 0, 0, 0x9a, 0xa0, 0},  /* 0x08 kernel code (kernel base selector) */
//        {0, 0, 0, 0x92, 0xa0, 0},  /* 0x10 kernel data */
//        {0, 0, 0, 0x92, 0xa0, 0},  /* 0x20 user data */
//        {0, 0, 0, 0x9a, 0xa0, 0},  /* 0x28 user code */
//        {0, 0, 0, 0x89, 0xa0, 0},  /* 0x40 tss low */
//        {0, 0, 0, 0x00, 0x00, 0},  /* 0x48 tss high */
//};
//
//void setup_gdt()
//{
//    uint64_t tss_base = ((uint64_t)&tss);
//    gdt_table.tss_low.base15_0 = tss_base & 0xffff;
//    gdt_table.tss_low.base23_16 = (tss_base >> 16) & 0xff;
//    gdt_table.tss_low.base31_24 = (tss_base >> 24) & 0xff;
//    gdt_table.tss_low.limit15_0 = sizeof(tss);
//    gdt_table.tss_high.limit15_0 = (tss_base >> 32) & 0xffff;
//    gdt_table.tss_high.base15_0 = (tss_base >> 48) & 0xffff;
//
//    struct table_ptr gdt_ptr = { sizeof(gdt_table)-1, (UINT64)&gdt_table };
//}