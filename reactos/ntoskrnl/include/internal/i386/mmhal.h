/*
 * Lowlevel memory managment definitions
 */

#ifndef __INTERNAL_HAL_I386_MMHAL_H
#define __INTERNAL_HAL_I386_MMHAL_H

struct _EPROCESS;

#define PAGESIZE (4096)

PULONG MmGetPageEntry(PVOID Address);


/*
 * Page access attributes (or these together)
 */
#define PA_READ            (1<<0)
#define PA_WRITE           ((1<<0)+(1<<1))
#define PA_EXECUTE         PA_READ
#define PA_PCD             (1<<4)
#define PA_PWT             (1<<3)

/*
 * Page attributes
 */
#define PA_USER            (1<<2)
#define PA_SYSTEM          (0)

#define KERNEL_BASE        (0xc0000000)

#define FLUSH_TLB    __asm__("movl %cr3,%eax\n\tmovl %eax,%cr3\n\t")

extern inline PULONG get_page_directory(void)
{
        unsigned int page_dir=0;
        __asm__("movl %%cr3,%0\n\t"
                : "=r" (page_dir));
        return((PULONG)page_dir);
}


/*
 * Amount of memory that can be mapped by a page table
 */
#define PAGE_TABLE_SIZE (4*1024*1024)

#define PAGE_MASK(x) (x&(~0xfff))
#define VADDR_TO_PT_OFFSET(x)  (((x/1024)%4096))
#define VADDR_TO_PD_OFFSET(x)  ((x)/(4*1024*1024))

VOID MmSetPage(struct _EPROCESS* Process,
	       PVOID Address, 
	       ULONG flProtect,
	       ULONG PhysicalAddress);


VOID MmSetPageProtect(struct _EPROCESS* Process,
		      PVOID Address,
		      ULONG flProtect);
BOOLEAN MmIsPagePresent(struct _EPROCESS* Process, 
			PVOID Address);

#endif /* __INTERNAL_HAL_I386_MMHAL_H */
