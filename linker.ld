/* linker.ld */
ENTRY(_start)

SECTIONS
{
    . = 0x10000;

    .text : {
        *(.text)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
    }

    /* --- NEW: Reserve 768KB for the kernel heap at 0xA0000 --- */
    . = 0xA0000;
    kernel_heap_start = .;
    . += KERNEL_HEAP_SIZE;

    /DISCARD/ : {
        *(.comment)
        *(.note*)
    }
}
