/*
 * Lowlevel memory managment definitions
 */

#ifndef __INTERNAL_HAL_PAGE_H
#define __INTERNAL_HAL_PAGE_H

#include <ddk/ntddk.h>

#define PAGESIZE (4096)

/*
 * Sets a page entry
 *      vaddr:          The virtual address to set the page entry for
 *      attributes:     The access attributes to give the page
 *      physaddr:       The physical address the page should map to
 */
void set_page(unsigned int vaddr, unsigned int attributes,
              unsigned int physaddr);

#define PAGE_ROUND_UP(x) ( (((ULONG)x)%PAGESIZE) ? ((((ULONG)x)&(~0xfff))+0x1000) : ((ULONG)x) )
#define PAGE_ROUND_DOWN(x) (((ULONG)x)&(~0xfff))

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


/*
 * Amount of memory that can be mapped by a page table
 */
#define PAGE_TABLE_SIZE (4*1024*1024)

#define PAGE_MASK(x) (x&(~0xfff))
#define VADDR_TO_PT_OFFSET(x)  (((x/1024)%4096))
#define VADDR_TO_PD_OFFSET(x)  ((x)/(4*1024*1024))

unsigned int* get_page_entry(unsigned int vaddr);

BOOL is_page_present(unsigned int vaddr);

#endif /* __INTERNAL_HAL_PAGE_H */
