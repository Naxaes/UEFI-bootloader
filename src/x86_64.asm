[bits 64]

global x86_64_interrupt


x86_64_interrupt:
    int 3
    ret