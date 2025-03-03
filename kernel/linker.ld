/* linker.ld - Linker script for the NeuroOS kernel */

ENTRY(_start)

SECTIONS {
    /* The kernel will be loaded at 1MB (conventional for many kernels) */
    . = 1M;

    /* First put the multiboot header, as it needs to be within the first 8KB */
    .multiboot : {
        KEEP(*(.multiboot))
    } :multiboot

    /* Then the .text section (code) - executable but not writable */
    .text : {
        *(.text)
    } :text

    /* Read-only data - not executable, not writable */
    .rodata : {
        *(.rodata)
    } :rodata

    /* Read-write data (initialized) - not executable */
    .data : {
        *(.data)
    } :data

    /* Read-write data (uninitialized) and stack - not executable */
    .bss : {
        *(COMMON)
        *(.bss)
    } :data

    /* Discard .note.GNU-stack sections */
    /DISCARD/ : {
        *(.note.GNU-stack)
    }

    /* End of the kernel */
    kernel_end = .;
}

/* Define memory permissions for sections */
PHDRS {
    multiboot PT_LOAD FLAGS(5); /* 5 = rx (read, execute) - multiboot header needs to be executable */
    text PT_LOAD FLAGS(5);      /* 5 = rx (read, execute) */
    rodata PT_LOAD FLAGS(4);    /* 4 = r (read-only) */
    data PT_LOAD FLAGS(6);      /* 6 = rw (read, write) */
}
