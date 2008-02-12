/*
 * CPSR Values
 */
.equ CPSR_IRQ_DISABLE,     0x80
.equ CPSR_FIQ_DISABLE,     0x40
.equ CPSR_THUMB_ENABLE,    0x20   

/*
 * C1 Register Values
 */
.equ C1_MMU_CONTROL,       0x01
.equ C1_ALIGNMENT_CONTROL, 0x02
.equ C1_DCACHE_CONTROL,    0x04
.equ C1_ICACHE_CONTROL,    0x1000
.equ C1_VECTOR_CONTROL,    0x2000

/*
 * Loader Parameter Block Offsets
 */
.equ LpbKernelStack,       0x18

/*
 * PCR
 */
.equ KiPcr,                0xFFFFF000
