[bits 64]

global x86_64_interrupt_3
global x86_64_cr0_set
global x86_64_cr0_get
global x86_64_cr2_set
global x86_64_cr2_get
global x86_64_cr3_set
global x86_64_cr3_get
global x86_64_load_gdt


x86_64_interrupt_3:
    int 3
    ret

x86_64_cr0_set:
   mov rax, rdi
   mov cr0, rax
   ret
x86_64_cr0_get:
   mov rax, cr0
   ret

x86_64_cr2_set:
  mov rax, rdi
  mov cr2, rax
  ret
x86_64_cr2_get:
  mov rax, cr2
  ret

x86_64_cr3_set:
   mov rax, rdi
   mov cr3, rax
   ret
x86_64_cr3_get:
   mov rax, cr3
   ret



x86_64_load_gdt:
    mov   rax, GDT.DESCRIPTOR
    lgdt  [rax]
;    mov   ax, 0x28    ; TSS segment is at 0x28 in the GDT
;    ltr   ax          ; Load TSS
    mov   ax, 0x10    ; Kernel data segment is at 0x10 in the GDT
    mov   ds, ax      ; Load kernel data segment in data segment registers
    mov   es, ax
    mov   fs, ax
    mov   gs, ax
    mov   ss, ax
    pop   rdi        ; Pop the return address
    mov   rax, 0x08  ; Kernel code segment is 0x08
    push  rax        ; Push the kernel code segment
    push  rdi        ; Push the return address again
    ret

x86_64_load_idt:
    lidt [rdi]
    ret


; ---- DATA ----
; 64-bit Global Descriptor Table
GDT:
   ; Helper label for calculations.
   .START:

   ; The null descriptor.
   .NULL_DESCRIPTOR:
       dd 0x0
       dd 0x0

   ; The kernel code segment descriptor.
   .KERNEL_CODE:
       dw 0x0          ; Limit (bits 0-15)
       dw 0x0          ; Base  (bits 0-15)
       db 0x0          ; Base  (bits 16-23)
       db 10011010b    ; 1st flags, type flags
       db 10100000b    ; 2nd flags, Limit (bits 16-19)
       db 0x0          ; Base (bits 24-31)

   ; The kernel data segment descriptor.
   .KERNEL_DATA:
       ; Same as code segment except for the type flags:
       ; type flags: 0 (code), 0 (expand down), 1 (writable), 0 (accessed) -> 0010.
       dw 0x0          ; Limit (bits 0-15)
       dw 0x0          ; Base  (bits 0-15)
       db 0x0          ; Base  (bits 16-23)
       db 10010010b    ; 1st flags, type flags
       db 10100000b    ; 2nd flags, Limit (bits 16-19)
       db 0x0          ; Base (bits 24-31)
;
;   ; The user code segment descriptor.
;   .USER_CODE:
;       dw 0x0          ; Limit (bits 0-15)
;       dw 0x0          ; Base  (bits 0-15)
;       db 0x0          ; Base  (bits 16-23)
;       db 10011010b    ; 1st flags, type flags
;       db 10100000b    ; 2nd flags, Limit (bits 16-19)
;       db 0x0          ; Base (bits 24-31)
;
;   ; The user data segment descriptor.
;   .USER_DATA:
;       dw 0x0          ; Limit (bits 0-15)
;       dw 0x0          ; Base  (bits 0-15)
;       db 0x0          ; Base  (bits 16-23)
;       db 10010010b    ; 1st flags, type flags
;       db 10100000b    ; 2nd flags, Limit (bits 16-19)
;       db 0x0          ; Base (bits 24-31)
;
;
;   ; The TSS low section segment descriptor.
;   .TSS_LOW:
;       dw 0x0          ; Limit (bits 0-15)
;       dw 0x0          ; Base  (bits 0-15)
;       db 0x0          ; Base  (bits 16-23)
;       db 10011010b    ; 1st flags, type flags
;       db 10001001b    ; 2nd flags, Limit (bits 16-19)
;       db 0x0          ; Base (bits 24-31)
;
;   ; The TSS high section segment descriptor.
;   .TSS_HIGH:
;       dw 0x0          ; Limit (bits 0-15)
;       dw 0x0          ; Base  (bits 0-15)
;       db 0x0          ; Base  (bits 16-23)
;       db 0x0          ; 1st flags, type flags
;       db 0x0          ; 2nd flags, Limit (bits 16-19)
;       db 0x0          ; Base (bits 24-31)

   ; The GDT descriptor.
   .DESCRIPTOR:
       dw $ - .START - 1  ; Size of our GDT, always less one of the true size.
       dq .START          ; Start address of our GDT.

