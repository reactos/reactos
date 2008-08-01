#ifndef _FREELDR_ARCH_COMPAT_H
#define _FREELDR_ARCH_COMPAT_H

#define __init
#define __initdata

#define SPRN_MSSCR0     0x3f6   /* Memory Subsystem Control Register 0 */
#define SPRN_MSSSR0     0x3f7   /* Memory Subsystem Status Register 1 */
#define SPRN_LDSTCR     0x3f8   /* Load/Store control register */
#define SPRN_LDSTDB     0x3f4   /* */
#define SPRN_LR         0x008   /* Link Register */
#ifndef SPRN_PIR
#define SPRN_PIR        0x3FF   /* Processor Identification Register */
#endif
#define SPRN_PTEHI      0x3D5   /* 981 7450 PTE HI word (S/W TLB load) */
#define SPRN_PTELO      0x3D6   /* 982 7450 PTE LO word (S/W TLB load) */
#define SPRN_PURR       0x135   /* Processor Utilization of Resources Reg */
#define SPRN_PVR        0x11F   /* Processor Version Register */
#define SPRN_RPA        0x3D6   /* Required Physical Address Register */
#define SPRN_SDA        0x3BF   /* Sampled Data Address Register */
#define SPRN_SDR1       0x019   /* MMU Hash Base Register */
#define SPRN_ASR        0x118   /* Address Space Register */
#define SPRN_SIA        0x3BB   /* Sampled Instruction Address Register */
#define SPRN_SPRG0      0x110   /* Special Purpose Register General 0 */
#define SPRN_SPRG1      0x111   /* Special Purpose Register General 1 */
#define SPRN_SPRG2      0x112   /* Special Purpose Register General 2 */
#define SPRN_SPRG3      0x113   /* Special Purpose Register General 3 */
#define SPRN_SPRG4      0x114   /* Special Purpose Register General 4 */
#define SPRN_SPRG5      0x115   /* Special Purpose Register General 5 */
#define SPRN_SPRG6      0x116   /* Special Purpose Register General 6 */
#define SPRN_SPRG7      0x117   /* Special Purpose Register General 7 */
#define SPRN_SRR0       0x01A   /* Save/Restore Register 0 */
#define SPRN_SRR1       0x01B   /* Save/Restore Register 1 */
#ifndef SPRN_SVR
#define SPRN_SVR        0x11E   /* System Version Register */
#endif
#define SPRN_THRM1      0x3FC           /* Thermal Management Register 1 */
/* these bits were defined in inverted endian sense originally, ugh, confusing */

/* Values for PP (assumes Ks=0, Kp=1) */
#define PP_RWXX 0       /* Supervisor read/write, User none */
#define PP_RWRX 1       /* Supervisor read/write, User read */
#define PP_RWRW 2       /* Supervisor read/write, User read/write */
#define PP_RXRX 3       /* Supervisor read,       User read */

/* Block size masks */
#define BL_128K 0x000
#define BL_256K 0x001
#define BL_512K 0x003
#define BL_1M   0x007
#define BL_2M   0x00F
#define BL_4M   0x01F
#define BL_8M   0x03F
#define BL_16M  0x07F
#define BL_32M  0x0FF
#define BL_64M  0x1FF
#define BL_128M 0x3FF
#define BL_256M 0x7FF

/* BAT Access Protection */
#define BPP_XX  0x00            /* No access */
#define BPP_RX  0x01            /* Read only */
#define BPP_RW  0x02            /* Read/write */

/* Definitions for 40x embedded chips. */
#define _PAGE_GUARDED   0x001   /* G: page is guarded from prefetch */
#define _PAGE_FILE      0x001   /* when !present: nonlinear file mapping */
#define _PAGE_PRESENT   0x002   /* software: PTE contains a translation */
#define _PAGE_NO_CACHE  0x004   /* I: caching is inhibited */
#define _PAGE_WRITETHRU 0x008   /* W: caching is write-through */
#define _PAGE_USER      0x010   /* matches one of the zone permission bits */
#define _PAGE_RW        0x040   /* software: Writes permitted */
#define _PAGE_DIRTY     0x080   /* software: dirty page */
#define _PAGE_HWWRITE   0x100   /* hardware: Dirty & RW, set in exception */
#define _PAGE_HWEXEC    0x200   /* hardware: EX permission */
#define _PAGE_ACCESSED  0x400   /* software: R: page referenced */

#define _PMD_PRESENT    0x400   /* PMD points to page of PTEs */
#define _PMD_BAD        0x802
#define _PMD_SIZE       0x0e0   /* size field, != 0 for large-page PMD entry */
#define _PMD_SIZE_4M    0x0c0
#define _PMD_SIZE_16M   0x0e0
#define PMD_PAGE_SIZE(pmdval)   (1024 << (((pmdval) & _PMD_SIZE) >> 4))

#define PVR_VER(pvr)(((pvr) >>  16) & 0xFFFF) /* Version field */

#define KERNELBASE 0x80000000

typedef unsigned char __u8;
typedef unsigned short __u16;
typedef unsigned int __u32;

typedef struct _pci_reg_property {
    struct {
	int a_hi, a_mid, a_lo;
    } addr;
    int size_hi, size_lo;
} pci_reg_property;

void btext_drawstring(const char *c);
void btext_drawhex(unsigned long v);

void *ioremap(__u32 phys, __u32 size);
void iounmap(void *logical);

__u32 GetPVR();

#endif/*_FREELDR_ARCH_COMPAT_H*/
