; NeuroOS Boot Assembly
; This file contains the multiboot2 header and entry point for the NeuroOS kernel

bits 32  ; Use 32-bit mode

; Multiboot2 header
section .multiboot
align 8
header_start:
    dd 0xE85250D6                ; Multiboot2 magic number
    dd 0                         ; Architecture: 0 = 32-bit (i386) protected mode
    dd header_end - header_start ; Header length
    dd -(0xE85250D6 + 0 + (header_end - header_start)) ; Checksum

    ; End tag
    align 8
    dw 0    ; Type: end
    dw 0    ; Flags
    dd 8    ; Size
header_end:

; Multiboot data
section .data
align 8
multiboot_magic_value:
    dd 0

multiboot_info_ptr:
    dd 0

; Kernel code
section .text
global _start
_start:
    ; Save multiboot info
    mov [multiboot_magic_value], eax
    mov [multiboot_info_ptr], ebx

    ; Set up the stack
    mov esp, stack_top

    ; Call the kernel main function with multiboot parameters
    extern kernel_main
    push dword [multiboot_info_ptr]  ; Second parameter: multiboot info pointer
    push dword [multiboot_magic_value]  ; First parameter: multiboot magic
    call kernel_main

    ; Halt the CPU if kernel_main returns
    cli
.halt:
    hlt
    jmp .halt

; Stack
section .bss
align 16
stack_bottom:
    resb 16384 ; 16 KiB
stack_top:
