#ifndef _ASM_SEGMENT_H
#define _ASM_SEGMENT_H

#define KERNEL_CS            (0x8)
#define KERNEL_DS            (0x10)
#define USER_CS              (0x18 + 0x3)
#define USER_DS              (0x20 + 0x3)
#define TSS_SELECTOR         (0x28)
#define PCR_SELECTOR         (0x30)
#define TEB_SELECTOR         (0x38 + 0x3)
#define LDT_SELECTOR         (0x48)
#define TRAP_TSS_SELECTOR    (0x50)

#endif /* _ASM_SEGMENT_H */
