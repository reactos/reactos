/*
 * Lowlevel memory managment definitions
 */

#ifndef __NTOSKRNL_INCLUDE_INTERNAL_I386_MM_H
#define __NTOSKRNL_INCLUDE_INTERNAL_I386_MM_H

#ifndef AS_INVOKED

struct _EPROCESS;

PULONG MmGetPageEntry(PVOID Address);

#define KERNEL_BASE        (0xc0000000)

#define FLUSH_TLB    __asm__("movl %cr3,%eax\n\tmovl %eax,%cr3\n\t")

PULONG MmGetPageDirectory(VOID);

/*
 * Amount of memory that can be mapped by a page table
 */
#define PAGE_TABLE_SIZE (4*1024*1024)

#define PAGE_MASK(x) (x&(~0xfff))
#define VADDR_TO_PT_OFFSET(x)  (((x/1024)%4096))
#define VADDR_TO_PD_OFFSET(x)  ((x)/(4*1024*1024))

#endif /* !AS_INVOKED */

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_I386_MM_H */
