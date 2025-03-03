/**
 * interrupts.h - Interrupt handling interface for NeuroOS
 * 
 * This file defines the interface for the interrupt handling subsystem, which is
 * responsible for setting up and handling hardware and software interrupts.
 */

#ifndef NEUROOS_INTERRUPTS_H
#define NEUROOS_INTERRUPTS_H

#include <stdint.h>
#include <stddef.h>

// Interrupt vector numbers
#define INT_DIVIDE_ERROR          0
#define INT_DEBUG                 1
#define INT_NMI                   2
#define INT_BREAKPOINT            3
#define INT_OVERFLOW              4
#define INT_BOUND_RANGE_EXCEEDED  5
#define INT_INVALID_OPCODE        6
#define INT_DEVICE_NOT_AVAILABLE  7
#define INT_DOUBLE_FAULT          8
#define INT_COPROCESSOR_SEGMENT   9
#define INT_INVALID_TSS          10
#define INT_SEGMENT_NOT_PRESENT  11
#define INT_STACK_SEGMENT_FAULT  12
#define INT_GENERAL_PROTECTION   13
#define INT_PAGE_FAULT           14
#define INT_RESERVED_15          15
#define INT_X87_FPU_ERROR        16
#define INT_ALIGNMENT_CHECK      17
#define INT_MACHINE_CHECK        18
#define INT_SIMD_FP_EXCEPTION    19
#define INT_VIRTUALIZATION       20
#define INT_CONTROL_PROTECTION   21
#define INT_RESERVED_22          22
#define INT_RESERVED_23          23
#define INT_RESERVED_24          24
#define INT_RESERVED_25          25
#define INT_RESERVED_26          26
#define INT_RESERVED_27          27
#define INT_RESERVED_28          28
#define INT_RESERVED_29          29
#define INT_RESERVED_30          30
#define INT_RESERVED_31          31

// IRQ numbers
#define IRQ_TIMER                0
#define IRQ_KEYBOARD             1
#define IRQ_CASCADE              2
#define IRQ_COM2                 3
#define IRQ_COM1                 4
#define IRQ_LPT2                 5
#define IRQ_FLOPPY               6
#define IRQ_LPT1                 7
#define IRQ_RTC                  8
#define IRQ_ACPI                 9
#define IRQ_AVAILABLE_10        10
#define IRQ_AVAILABLE_11        11
#define IRQ_PS2_MOUSE           12
#define IRQ_FPU                 13
#define IRQ_PRIMARY_ATA         14
#define IRQ_SECONDARY_ATA       15

// IRQ to interrupt vector mapping
#define IRQ_TO_VECTOR(irq)      ((irq) + 32)

// Interrupt handler function type
typedef void (*interrupt_handler_t)(void);

// Interrupt frame structure (passed to handlers)
typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha
    uint32_t int_no, err_code;                       // Interrupt number and error code
    uint32_t eip, cs, eflags, useresp, ss;           // Pushed by the processor automatically
} __attribute__((packed)) interrupt_frame_t;

/**
 * Initialize the interrupt handling subsystem
 * 
 * This function initializes the interrupt handling subsystem and sets up
 * the interrupt descriptor table (IDT).
 */
void interrupts_init(void);

/**
 * Enable interrupts
 * 
 * This function enables hardware interrupts.
 */
void interrupts_enable(void);

/**
 * Disable interrupts
 * 
 * This function disables hardware interrupts.
 */
void interrupts_disable(void);

/**
 * Check if interrupts are enabled
 * 
 * @return: 1 if interrupts are enabled, 0 otherwise
 */
int interrupts_are_enabled(void);

/**
 * Register an interrupt handler
 * 
 * This function registers a handler for the specified interrupt vector.
 * 
 * @param vector: Interrupt vector number
 * @param handler: Interrupt handler function
 * @return: 0 on success, -1 on failure
 */
int interrupts_register_handler(uint8_t vector, interrupt_handler_t handler);

/**
 * Unregister an interrupt handler
 * 
 * This function unregisters the handler for the specified interrupt vector.
 * 
 * @param vector: Interrupt vector number
 * @return: 0 on success, -1 on failure
 */
int interrupts_unregister_handler(uint8_t vector);

/**
 * Register an IRQ handler
 * 
 * This function registers a handler for the specified IRQ.
 * 
 * @param irq: IRQ number
 * @param handler: Interrupt handler function
 * @return: 0 on success, -1 on failure
 */
int interrupts_register_irq_handler(uint8_t irq, interrupt_handler_t handler);

/**
 * Unregister an IRQ handler
 * 
 * This function unregisters the handler for the specified IRQ.
 * 
 * @param irq: IRQ number
 * @return: 0 on success, -1 on failure
 */
int interrupts_unregister_irq_handler(uint8_t irq);

/**
 * Enable an IRQ
 * 
 * This function enables the specified IRQ.
 * 
 * @param irq: IRQ number
 */
void interrupts_enable_irq(uint8_t irq);

/**
 * Disable an IRQ
 * 
 * This function disables the specified IRQ.
 * 
 * @param irq: IRQ number
 */
void interrupts_disable_irq(uint8_t irq);

/**
 * Send an end-of-interrupt signal to the PIC
 * 
 * This function sends an EOI signal to the PIC for the specified IRQ.
 * 
 * @param irq: IRQ number
 */
void interrupts_send_eoi(uint8_t irq);

/**
 * Trigger a software interrupt
 * 
 * This function triggers a software interrupt with the specified vector.
 * 
 * @param vector: Interrupt vector number
 */
void interrupts_trigger(uint8_t vector);

#endif /* NEUROOS_INTERRUPTS_H */
