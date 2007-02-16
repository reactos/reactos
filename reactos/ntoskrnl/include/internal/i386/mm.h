/*
 * Lowlevel memory managment definitions
 */

#ifndef __NTOSKRNL_INCLUDE_INTERNAL_I386_MM_H
#define __NTOSKRNL_INCLUDE_INTERNAL_I386_MM_H

#if 0
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
#endif

struct _EPROCESS;
PULONG MmGetPageDirectory(VOID);



#define PAGE_MASK(x)		((x)&(~0xfff))
#define PAE_PAGE_MASK(x)	((x)&(~0xfffLL))

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_I386_MM_H */
