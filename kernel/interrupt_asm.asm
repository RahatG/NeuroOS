; interrupt_asm.asm - Assembly code for interrupt handling in NeuroOS
;
; This file contains the assembly code for interrupt handling, including
; the interrupt service routines (ISRs) and interrupt request handlers (IRQs).

; External C functions
extern isr_handler
extern irq_handler

; This macro creates an ISR stub that does not push an error code
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push 0                  ; Push a dummy error code
    push %1                 ; Push the interrupt number
    jmp isr_common_stub     ; Jump to the common handler
%endmacro

; This macro creates an ISR stub that pushes an error code
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    ; Error code is already pushed by the CPU
    push %1                 ; Push the interrupt number
    jmp isr_common_stub     ; Jump to the common handler
%endmacro

; This macro creates an IRQ stub
%macro IRQ 2
global irq%1
irq%1:
    push 0                  ; Push a dummy error code
    push %2                 ; Push the interrupt number
    jmp irq_common_stub     ; Jump to the common handler
%endmacro

; Define the ISRs
ISR_NOERRCODE 0    ; Divide by zero
ISR_NOERRCODE 1    ; Debug
ISR_NOERRCODE 2    ; Non-maskable interrupt
ISR_NOERRCODE 3    ; Breakpoint
ISR_NOERRCODE 4    ; Overflow
ISR_NOERRCODE 5    ; Bound range exceeded
ISR_NOERRCODE 6    ; Invalid opcode
ISR_NOERRCODE 7    ; Device not available
ISR_ERRCODE   8    ; Double fault
ISR_NOERRCODE 9    ; Coprocessor segment overrun
ISR_ERRCODE   10   ; Invalid TSS
ISR_ERRCODE   11   ; Segment not present
ISR_ERRCODE   12   ; Stack-segment fault
ISR_ERRCODE   13   ; General protection fault
ISR_ERRCODE   14   ; Page fault
ISR_NOERRCODE 15   ; Reserved
ISR_NOERRCODE 16   ; x87 FPU error
ISR_ERRCODE   17   ; Alignment check
ISR_NOERRCODE 18   ; Machine check
ISR_NOERRCODE 19   ; SIMD floating-point exception
ISR_NOERRCODE 20   ; Virtualization exception
ISR_ERRCODE   21   ; Control protection exception
ISR_NOERRCODE 22   ; Reserved
ISR_NOERRCODE 23   ; Reserved
ISR_NOERRCODE 24   ; Reserved
ISR_NOERRCODE 25   ; Reserved
ISR_NOERRCODE 26   ; Reserved
ISR_NOERRCODE 27   ; Reserved
ISR_NOERRCODE 28   ; Reserved
ISR_NOERRCODE 29   ; Reserved
ISR_NOERRCODE 30   ; Reserved
ISR_NOERRCODE 31   ; Reserved

; Define the IRQs
IRQ 0, 32   ; Programmable Interrupt Timer
IRQ 1, 33   ; Keyboard
IRQ 2, 34   ; Cascade for 8259A Slave controller
IRQ 3, 35   ; COM2
IRQ 4, 36   ; COM1
IRQ 5, 37   ; LPT2
IRQ 6, 38   ; Floppy disk
IRQ 7, 39   ; LPT1 / Unreliable "spurious" interrupt
IRQ 8, 40   ; CMOS Real-time clock
IRQ 9, 41   ; Free for peripherals / legacy SCSI / NIC
IRQ 10, 42  ; Free for peripherals / SCSI / NIC
IRQ 11, 43  ; Free for peripherals / SCSI / NIC
IRQ 12, 44  ; PS/2 mouse
IRQ 13, 45  ; FPU / Coprocessor / Inter-processor
IRQ 14, 46  ; Primary ATA hard disk
IRQ 15, 47  ; Secondary ATA hard disk

; Common ISR stub
isr_common_stub:
    ; Save all registers
    pusha
    
    ; Save data segment
    mov ax, ds
    push eax
    
    ; Load kernel data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Call C handler
    call isr_handler
    
    ; Restore data segment
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Restore all registers
    popa
    
    ; Clean up error code and interrupt number
    add esp, 8
    
    ; Return from interrupt
    iret

; Common IRQ stub
irq_common_stub:
    ; Save all registers
    pusha
    
    ; Save data segment
    mov ax, ds
    push eax
    
    ; Load kernel data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Call C handler
    call irq_handler
    
    ; Restore data segment
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Restore all registers
    popa
    
    ; Clean up error code and interrupt number
    add esp, 8
    
    ; Return from interrupt
    iret

; Function to load the IDT
global idt_flush
idt_flush:
    mov eax, [esp+4]  ; In 32-bit, first parameter is on the stack
    lidt [eax]        ; Load the IDT
    ret
