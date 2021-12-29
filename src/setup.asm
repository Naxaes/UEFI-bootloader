[bits 64]

; Define some handy constants for the GDT segment descriptor offsets, which
; are what segment registers must contain when in protected mode. For example,
; when we set DS = 0x10 in PM, the CPU knows that we mean it to use the
; segment described at offset 0x10 (i.e. 16 bytes) in our GDT, which in our
; case is the DATA segment (0x0 -> NULL; 0x08 -> CODE; 0x10 -> DATA)
CODE_SEGMENT equ GDT.CODE - GDT.START
DATA_SEGMENT equ GDT.DATA - GDT.START


global detect_cpu_id
global setup_identity_paging


detect_cpu_id:
    pushfd
    pop eax

    mov ecx, eax
    xor eax, 1 << 21

    push eax
    popfd

    pushfd
    pop eax

    push ecx
    popfd

    xor eax, ecx
    jz  .no_cpu_detected

    ret

    .no_cpu_detected:
        jmp $


GDT:
    ; Helper label for calculations.
    .START:

    ; The mandatory null descriptor.
    .NULL_DESCRIPTOR:
        dd 0x0
        dd 0x0

    ; The code segment descriptor.
    .CODE:
        ; base  = 0x0
        ; limit = 0xfffff
        ; 1st  flags: 1 (present), 00 (privilege), 1(descriptor type) -> 1001.
        ; type flags: 1 (code), 0 (conforming), 1 (readable), 0 (accessed) -> 1010.
        ; 2nd  flags: 1 (granularity), 1 (32-bit default), 0 (64-bit seg), 0, (AVL) -> 1100.
        dw 0xffff   	; Limit (bits 0-15)
        dw 0x0    		; Base  (bits 0-15)
        db 0x0    		; Base  (bits 16-23)
        db 10011010b 	; 1st flags, type flags
        db 11001111b 	; 2nd flags, Limit (bits 16-19)
        db 0x0     		; Base (bits 24-31)

    ; The data segment descriptor.
    .DATA:
        ; Same as code segment except for the type flags:
        ; type flags: 0 (code), 0 (expand down), 1 (writable), 0 (accessed) -> 0010.
        dw 0xffff   	; Limit (bits 0-15)
        dw 0x0     		; Base  (bits 0-15)
        db 0x0     		; Base  (bits 16-23)
        db 10010010b 	; 1st flags, type flags
        db 11001111b	; 2nd flags, Limit (bits 16-19)
        db 0x0     		; Base (bits 24-31)

    ; The GDT descriptor.
    .DESCRIPTOR:
        dw $ - .START - 1  ; Size of our GDT, always less one of the true size.
        dd .START          ; Start address of our GDT.
