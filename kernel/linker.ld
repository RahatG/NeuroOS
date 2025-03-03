/* linker.ld - Linker script for the NeuroOS kernel */

ENTRY(_start)

SECTIONS {
    /* The kernel will be loaded at 1MB (conventional for many kernels) */
    . = 1M;

    /* First put the multiboot header, as it needs to be within the first 8KB */
    .multiboot : {
        KEEP(*(.multiboot))
    }

    /* Then the .text section (code) */
    .text : {
        *(.text)
    }

    /* Read-only data */
    .rodata : {
        *(.rodata)
    }

    /* Read-write data (initialized) */
    .data : {
        *(.data)
    }

    /* Read-write data (uninitialized) and stack */
    .bss : {
        *(COMMON)
        *(.bss)
    }

    /* End of the kernel */
    kernel_end = .;
}
