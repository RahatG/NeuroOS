/**
 * interrupts.c - Interrupt handling for NeuroOS
 * 
 * This file implements the interrupt handling subsystem, which is responsible for
 * setting up the Interrupt Descriptor Table (IDT) and handling interrupts.
 */

#include "include/interrupts.h"
#include "include/console.h"
#include <string.h>

// Number of IDT entries
#define IDT_ENTRIES 256

// IDT entry flags
#define IDT_FLAG_PRESENT 0x80
#define IDT_FLAG_RING0   0x00
#define IDT_FLAG_RING1   0x20
#define IDT_FLAG_RING2   0x40
#define IDT_FLAG_RING3   0x60
#define IDT_FLAG_32BIT   0x0E

// Interrupt Descriptor Table (IDT) entry structure
typedef struct {
    uint16_t base_low;
    uint16_t selector;
    uint8_t always0;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed)) idt_entry_t;

// IDT pointer structure
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

// Interrupt handler function pointer type
typedef void (*interrupt_handler_t)(void);

// Interrupt handler table
static interrupt_handler_t interrupt_handlers[IDT_ENTRIES];

// IDT entries
static idt_entry_t idt_entries[IDT_ENTRIES];

// IDT pointer
static idt_ptr_t idt_ptr;

// Alias for interrupt_frame_t to maintain compatibility with existing code
typedef interrupt_frame_t registers_t;

// External assembly functions
extern void idt_flush(uint32_t);
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

// Forward declarations
static void idt_init(void);
static void pic_init(void);
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags);
void outb(uint16_t port, uint8_t value);

/**
 * Initialize the interrupt handling subsystem
 */
void interrupts_init(void) {
    // Initialize the IDT
    idt_init();
    
    // Initialize the PIC
    pic_init();
    
    // Initialize the interrupt handler table
    memset(interrupt_handlers, 0, sizeof(interrupt_handler_t) * IDT_ENTRIES);
    
    // Enable interrupts
    __asm__ volatile("sti");
    
    console_printf("Interrupt handling initialized\n");
}

/**
 * Initialize the Interrupt Descriptor Table (IDT)
 */
static void idt_init(void) {
    // Set up the IDT pointer
    idt_ptr.limit = sizeof(idt_entry_t) * IDT_ENTRIES - 1;
    idt_ptr.base = (uint32_t)(uintptr_t)&idt_entries;
    
    // Clear the IDT entries
    memset(&idt_entries, 0, sizeof(idt_entry_t) * IDT_ENTRIES);
    
    // Remap the PIC
    // Send initialization command to PIC
    outb(0x20, 0x11); // ICW1: Initialize + ICW4
    outb(0xA0, 0x11);
    
    // Set vector offsets
    outb(0x21, 0x20); // ICW2: Master PIC vector offset (32)
    outb(0xA1, 0x28); // ICW2: Slave PIC vector offset (40)
    
    // Tell the PICs how they are connected
    outb(0x21, 0x04); // ICW3: Master has slave on IRQ2
    outb(0xA1, 0x02); // ICW3: Slave is connected to IRQ2 of master
    
    // Set operation mode
    outb(0x21, 0x01); // ICW4: 8086 mode
    outb(0xA1, 0x01);
    
    // Set up the ISR handlers
    idt_set_gate(0, (uint32_t)(uintptr_t)isr0, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(1, (uint32_t)(uintptr_t)isr1, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(2, (uint32_t)(uintptr_t)isr2, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(3, (uint32_t)(uintptr_t)isr3, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(4, (uint32_t)(uintptr_t)isr4, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(5, (uint32_t)(uintptr_t)isr5, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(6, (uint32_t)(uintptr_t)isr6, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(7, (uint32_t)(uintptr_t)isr7, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(8, (uint32_t)(uintptr_t)isr8, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(9, (uint32_t)(uintptr_t)isr9, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(10, (uint32_t)(uintptr_t)isr10, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(11, (uint32_t)(uintptr_t)isr11, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(12, (uint32_t)(uintptr_t)isr12, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(13, (uint32_t)(uintptr_t)isr13, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(14, (uint32_t)(uintptr_t)isr14, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(15, (uint32_t)(uintptr_t)isr15, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(16, (uint32_t)(uintptr_t)isr16, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(17, (uint32_t)(uintptr_t)isr17, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(18, (uint32_t)(uintptr_t)isr18, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(19, (uint32_t)(uintptr_t)isr19, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(20, (uint32_t)(uintptr_t)isr20, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(21, (uint32_t)(uintptr_t)isr21, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(22, (uint32_t)(uintptr_t)isr22, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(23, (uint32_t)(uintptr_t)isr23, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(24, (uint32_t)(uintptr_t)isr24, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(25, (uint32_t)(uintptr_t)isr25, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(26, (uint32_t)(uintptr_t)isr26, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(27, (uint32_t)(uintptr_t)isr27, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(28, (uint32_t)(uintptr_t)isr28, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(29, (uint32_t)(uintptr_t)isr29, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(30, (uint32_t)(uintptr_t)isr30, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(31, (uint32_t)(uintptr_t)isr31, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    
    // Set up the IRQ handlers
    idt_set_gate(32, (uint32_t)(uintptr_t)irq0, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(33, (uint32_t)(uintptr_t)irq1, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(34, (uint32_t)(uintptr_t)irq2, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(35, (uint32_t)(uintptr_t)irq3, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(36, (uint32_t)(uintptr_t)irq4, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(37, (uint32_t)(uintptr_t)irq5, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(38, (uint32_t)(uintptr_t)irq6, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(39, (uint32_t)(uintptr_t)irq7, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(40, (uint32_t)(uintptr_t)irq8, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(41, (uint32_t)(uintptr_t)irq9, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(42, (uint32_t)(uintptr_t)irq10, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(43, (uint32_t)(uintptr_t)irq11, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(44, (uint32_t)(uintptr_t)irq12, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(45, (uint32_t)(uintptr_t)irq13, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(46, (uint32_t)(uintptr_t)irq14, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    idt_set_gate(47, (uint32_t)(uintptr_t)irq15, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_32BIT);
    
    // Load the IDT
    idt_flush((uint32_t)(uintptr_t)&idt_ptr);
}

/**
 * Initialize the Programmable Interrupt Controller (PIC)
 */
static void pic_init(void) {
    // Mask all interrupts
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
    
    // Unmask the timer interrupt (IRQ0)
    outb(0x21, 0xFE);
}

/**
 * Set an IDT gate (entry)
 * 
 * @param num: Gate number
 * @param base: Handler function address
 * @param selector: Code segment selector
 * @param flags: Gate flags
 */
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags) {
    idt_entries[num].base_low = base & 0xFFFF;
    idt_entries[num].base_high = (base >> 16) & 0xFFFF;
    idt_entries[num].selector = selector;
    idt_entries[num].always0 = 0;
    idt_entries[num].flags = flags;
}

/**
 * Common interrupt service routine (ISR) handler
 * 
 * @param regs: Registers state
 */
void isr_handler(registers_t* regs) {
    // Check if we have a handler for this interrupt
    if (interrupt_handlers[regs->int_no]) {
        interrupt_handler_t handler = interrupt_handlers[regs->int_no];
        handler();
    } else {
        console_printf("Unhandled interrupt: %d\n", regs->int_no);
    }
}

/**
 * Common interrupt request (IRQ) handler
 * 
 * @param regs: Registers state
 */
void irq_handler(registers_t* regs) {
    // Send an EOI (End of Interrupt) to the PICs
    if (regs->int_no >= 40) {
        // Send reset signal to slave PIC
        outb(0xA0, 0x20);
    }
    
    // Send reset signal to master PIC
    outb(0x20, 0x20);
    
    // Check if we have a handler for this IRQ
    if (interrupt_handlers[regs->int_no]) {
        interrupt_handler_t handler = interrupt_handlers[regs->int_no];
        handler();
    }
}

/**
 * Register an interrupt handler
 * 
 * @param vector: Interrupt vector number
 * @param handler: Handler function
 * @return: 0 on success, -1 on failure
 */
int interrupts_register_handler(uint8_t vector, interrupt_handler_t handler) {
    // Register the handler
    interrupt_handlers[vector] = handler;
    
    return 0;
}

/**
 * Unregister an interrupt handler
 * 
 * @param vector: Interrupt vector number
 * @return: 0 on success, -1 on failure
 */
int interrupts_unregister_handler(uint8_t vector) {
    
    // Unregister the handler
    interrupt_handlers[vector] = NULL;
    
    return 0;
}

/**
 * Enable interrupts
 */
void interrupts_enable(void) {
    __asm__ volatile("sti");
}

/**
 * Disable interrupts
 */
void interrupts_disable(void) {
    __asm__ volatile("cli");
}

/**
 * Get the interrupt state
 * 
 * @return: 1 if interrupts are enabled, 0 if disabled
 */
int interrupts_are_enabled(void) {
    unsigned long flags;
    
    __asm__ volatile("pushf\n\t"
                     "pop %0"
                     : "=rm"(flags));
    
    return (flags & 0x200) ? 1 : 0;
}

/**
 * Port I/O functions
 */

/**
 * Write a byte to a port
 * 
 * @param port: Port number
 * @param value: Value to write
 */
void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * Read a byte from a port
 * 
 * @param port: Port number
 * @return: Value read
 */
uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * Write a word to a port
 * 
 * @param port: Port number
 * @param value: Value to write
 */
void outw(uint16_t port, uint16_t value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * Read a word from a port
 * 
 * @param port: Port number
 * @return: Value read
 */
uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * Write a long to a port
 * 
 * @param port: Port number
 * @param value: Value to write
 */
void outl(uint16_t port, uint32_t value) {
    __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * Read a long from a port
 * 
 * @param port: Port number
 * @return: Value read
 */
uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
