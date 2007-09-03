#ifndef PPCMMU_H
#define PPCMMU_H

/* PPC MMU object --
 * Always called from kernel mode, maps the first 16 megabytes and uses 16
 * bytes per page between 0x30000 and 16 megs.  Maximum memory size is 3 gig.
 *
 * Physical Memory Map:
 * 0x00300 -- Data Miss
 * 0x00400 -- Code Miss
 * 0x10000 -- MMU ucode
 * 0x20000 -- PTEG
 * 0x30000 -- Full map
 *
 * Actions:
 * 00 -- Initialize
 *  -- No arguments
 * 01 -- Map page
 *  r4 -- virtual address
 *  r5 -- ppc_map_info_t
 * 02 -- Erase page
 *  r4 -- virtual address
 * 03 -- Set segment VSID
 *  r4 -- Start seg
 *  r5 -- End seg
 *  r6 -- Vsid
 * 04 -- Set page miss callback
 *  r4 -- Callback address (VA)
 * 05 -- Query page
 *  r4 -- Page addr
 *  r5 -- Address of info struct
 * 06 -- Unit Test
 * 07 -- Turn on paging
 * 08 -- Unmap process
 */

#define MMUCODE 0x10000
#define HTABORG 0x20000
#define HTABSIZ 0x10000
#define PAGETAB 0x30000

#define PpcHashedPTE ((ppc_pteg_t*)(HTABORG))
#define PpcPageTable ((ppc_map_t*)(PAGETAB))

#define PPC_PAGE_ADDR(x) ((x) << 12)
#define PPC_PAGE_NUMBER(x) ((x) >> 12)
#define PPC_VSID_MASK 0xffffff
#define PPC_PAGE_MASK 0xfff

#define MMU_NONE   0
#define MMU_KR     8
#define MMU_KW     4
#define MMU_UR     2
#define MMU_UW     1
#define MMU_ALL_R  10
#define MMU_KRW    12
#define MMU_KRW_UR 14
#define MMU_ALL_RW 15

#define MMU_PAGE_ACCESS 0x40000000
#define MMU_PAGE_DIRTY  0x80000000

#define MMU_KMASK  12
#define MMU_UMASK   3

extern char _binary_mmucode_start[], _binary_mmucode_end[];

/* thanks geist */
typedef unsigned long paddr_t;
typedef unsigned long vaddr_t;

typedef struct _ppc_pte_t {
    unsigned long pteh, ptel;
} ppc_pte_t;

typedef struct _ppc_pteg_t {
    ppc_pte_t block[8];
} ppc_pteg_t;

typedef struct _ppc_map_t {
    ppc_pte_t pte;
    unsigned long proc;
    vaddr_t addr;
} ppc_map_t;

typedef struct _ppc_map_info_t {
    unsigned long flags, proc;
    vaddr_t addr;
    paddr_t phys;
} ppc_map_info_t;

typedef struct _ppc_trap_frame_t {
    unsigned long gpr[32];
    unsigned long lr, cr, ctr, srr0, srr1, dsisr, dar, xer;
} ppc_trap_frame_t;

typedef int (*MmuPageCallback)(int inst, ppc_trap_frame_t *trap);

#include "mmuutil.h"

static inline int PPCMMU(int action, void *arg1, void *arg2, void *arg3)
{
    /* Set Bat0 to mmu object address */
    int i, batu, batl, oldbat[8], usebat[2] = { 0, 1 }, gotbat = 0, pc, mask;
    volatile int ret;
    int (*mmumain)(int action, void *arg1, void *arg2, void *arg3) = (void *)MMUCODE;
    __asm__("bl 1f\n\t"
	    "\n1:\n\t"
	    "mflr %0\n\t" : "=r" (pc));

    for(i = 0, gotbat = 0; i < 4 && gotbat < 2; i++)
    {
	GetBat(i, 1, &batu, &batl);
	if(batu & 0xffc)
	{
	    mask = ~(0x1ffff | ((batu & 0xffc)>>2)<<17);
	    if(!(batu & 2) || ((batu & mask) != (pc & mask)))
		usebat[gotbat++] = i;
	} else {
	    mask = ~(0x1ffff | (batl << 17));
	    if(!(batl & 0x40) || ((batu & mask) != (pc & mask)))
		usebat[gotbat++] = i;
	}
    }

    GetBat(usebat[0], 0, &oldbat[0], &oldbat[1]);
    GetBat(usebat[0], 1, &oldbat[2], &oldbat[3]);
    GetBat(usebat[1], 0, &oldbat[4], &oldbat[5]);
    GetBat(usebat[1], 1, &oldbat[6], &oldbat[7]);

    batu = 0xff;
    batl = 0x7f;
    SetBat(usebat[0], 0, batu, batl);
    SetBat(usebat[0], 1, batu, batl);
    batu += 8192 * 1024;
    batl += 8192 * 1024;
    SetBat(usebat[1], 0, batu, batl);
    SetBat(usebat[1], 1, batu, batl);

    ret = mmumain(action, arg1, arg2, arg3);

    /* Ok done ... Whatever happened probably worked */
    SetBat(usebat[0], 0, oldbat[0], oldbat[1]);
    SetBat(usebat[0], 1, oldbat[2], oldbat[3]);
    SetBat(usebat[1], 0, oldbat[4], oldbat[5]);
    SetBat(usebat[1], 1, oldbat[6], oldbat[7]);

    return ret;
}

/* Expand this only if used ... That makes dependence on libmmu_code.a depend
 * on whether MmuInit is called in a clean way.
 */
#define MmuInit() _MmuInit(&_binary_mmucode_start, &_binary_mmucode_end)

/* Copy in the mmu code and call init
 * This bootstrap should only be called the first time (i.e. in the bootloader
 * or the early boot code).  Part of the purpose of this library is to
 * eliminate the need to do a complex mmu handoff between boot stages.
 */
static inline void _MmuInit(void *_start, void *_end)
{
    int target = MMUCODE;
    int *start = (int *)_start;
    while(start < (int *)_end)
    {
	SetPhys(target, *start++);
	target += sizeof(int);
    }
    PPCMMU(0, 0, 0, 0);
}

static inline void MmuMapPage(ppc_map_info_t *info, int count)
{
    PPCMMU(1, info, (void *)count, 0);
}

static inline void MmuUnmapPage(ppc_map_info_t *info, int count)
{
    PPCMMU(2, info, (void *)count, 0);
}

static inline void MmuSetVsid(int start, int end, int vsid)
{
    PPCMMU(3, (void *)start, (void *)end, (void *)vsid);
}

static inline MmuPageCallback MmuSetPageCallback(MmuPageCallback cb)
{
    return (MmuPageCallback)PPCMMU(4, (void *)cb, 0, 0);
}

static inline void MmuInqPage(ppc_map_info_t *info, int count)
{
    PPCMMU(5, info, (void *)count, 0);
}

static inline int MmuUnitTest()
{
    return PPCMMU(6, 0, 0, 0);
}

static inline int MmuTurnOn(void *fun, void *arg)
{
    return PPCMMU(7, fun, arg, 0);
}

static inline void MmuSetMemorySize(paddr_t size)
{
    PPCMMU(8, (void *)size, 0, 0);
}

static inline paddr_t MmuGetFirstPage()
{
    return (paddr_t)PPCMMU(9, 0, 0, 0);
}

static inline void *MmuAllocVsid(int vsid, int mask)
{
    return (void *)PPCMMU(10, (void *)vsid, (void *)mask, 0);
}

static inline void MmuRevokeVsid(int vsid, int mask)
{
    PPCMMU(11, (void *)vsid, (void *)mask, 0);
}

#endif/*PPCMMU_H*/
