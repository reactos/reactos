/*
 * Provide __bss_start__ and __bss_end__ symbols for LLD builds.
 * GNU ld provides these through the linker script (freeldr_gcc.lds),
 * but LLD doesn't support linker scripts in PE/COFF mode.
 */
.intel_syntax noprefix

.section .bss$a
.global __bss_start__
__bss_start__:

.section .bss$z
.global __bss_end__
__bss_end__:
