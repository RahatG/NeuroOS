/* Simple linker script for multiboot2 kernel */

ENTRY(_start)

SECTIONS {
    /* The kernel will be loaded at 1MB */
    . = 1M;

    /* First put the multiboot header */
    .multiboot : {
        *(.multiboot)
    }

    /* Then the .text section */
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
}
