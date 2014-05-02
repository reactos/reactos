#include <stdarg.h>
#include "ppcmmu/mmu.h"
#include "ppcmmu/mmuutil.h"
#include "mmuobject.h"

typedef unsigned long ULONG;

/*

The MMU Object:
0x00300 -- Data miss
0x00400 -- Instruction miss
0x10000 -- Entry point
...        Code
0x20000 -- Physical map (PTE + Process Ptr + Address : 16 bytes)

4096 / 16 bytes = 256 entries per page
256 pages = 1Megabyte = 1 page table page

Setup by freeldr and used to build the kernel map, then used by the kernel

Calling:

r3       -- Action
r4 .. r6 -- Args

Actions:
00 Init
01 Map pages
02 erase pages
03 set segment vsid
04 page miss callback
05 inquire page
06 unit test
07 alloc page
08 set memory size
09 get first usable page
10 alloc vsid
11 revoke vsid
*/

#define MMU_ADDR_RESERVED ((vaddr_t)-2)

MmuTrapHandler callback[0x30];
typedef struct _MmuFreePage {
    int page;
    struct _MmuFreePage *next;
} MmuFreePage;
typedef struct _MmuFreeTree {
    struct _MmuFreeTree *next;
} MmuFreeTree;
typedef struct _MmuVsidTree {
    ppc_map_t *leaves[256];
} MmuVsidTree;
typedef struct _MmuVsidInfo {
    int vsid;
    struct _MmuVsidInfo *next;
    MmuVsidTree *tree[256];
} MmuVsidInfo;
MmuFreePage *FreeList = 0;
// Pages are allocated one by one until NextPage == RamSize >> PPC_PAGE_SHIFT
// Then we take only from the free list
int Clock = 0, TreeAlloc = 0, GdbAttach = 0, Booted = 0, Vsid[16];
paddr_t RamSize, FirstUsablePage, NextPage;
MmuVsidTree *NextTreePage = 0;
MmuFreeTree *FreeTree;
MmuVsidInfo *Segs[16], *VsidHead = 0;

extern void fmtout(const char *fmt, ...);
extern char *serport;
int ptegreload(ppc_trap_frame_t *frame, vaddr_t addr);
void SerialSetUp(int deviceType, void *deviceAddr, int baud);
int SerialInterrupt(int n, ppc_trap_frame_t *tf);
void TakeException(int n, ppc_trap_frame_t *tf);
int mmuisfreepage(paddr_t pageno);
void copy(void *t, void *s, int b);
paddr_t mmunewpage();
void dumpmap();
void trapcallback(int action, ppc_trap_frame_t *trap_frame);

int _mmumain(int action, void *arg1, void *arg2, void *arg3, void *tf)
{
    ppc_trap_frame_t *trap_frame = (action >= 0x100) ? tf : arg1;
    int ret = 0, tmp, i;

    switch(action)
    {
        /* Trap Handlers */
    case 3:
	if(!ptegreload(trap_frame, trap_frame->dar))
	{
            trapcallback(action, trap_frame);
        }
	break;
    case 4:
	if(!ptegreload(trap_frame, trap_frame->srr0))
        {
            trapcallback(action, trap_frame);
        }
	break;

    case 5:
        /* EE -- Try to get a serial interrupt if debugging enabled, then fall
         * back to primary handler 
         */
        if (!SerialInterrupt(action, trap_frame) && callback[action]) 
        {
            trapcallback(action, trap_frame);
        }
        break;
    case 0:
    case 2:
    case 6:
    case 7:
    case 8:
    case 9:
    case 0xa:
    case 0xc:
    case 0x20:
        trapcallback(action, trap_frame);
        break;

        /* MMU Functions */
    case 0x100:
	initme();
        trap_frame->srr1 |= 0x8000;
	break;
    case 0x101:
	ret = mmuaddpage(arg1, (int)arg2);
	break;
    case 0x102:
	mmudelpage(arg1, (int)arg2);
	break;
    case 0x103:
	mmusetvsid((int)arg1, (int)arg2, (int)arg3);
	break;
    case 0x104:
	ret = (int)callback[(int)arg1];
	callback[(int)arg1] = (MmuTrapHandler)arg2;
	break;
    case 0x105:
	mmugetpage(arg1, (int)arg2);
	break;
    case 0x106:
	ret = mmunitest();
	break;
    case 0x107:
        callkernel(arg1, arg2);
	break;
    case 0x108:
	mmusetramsize((paddr_t)arg1);
	break;
    case 0x109:
	return FirstUsablePage;
    case 0x10a:
	mmuallocvsid((int)arg1, (int)arg2);
	break;
    case 0x10b:
	mmufreevsid((int)arg1, (int)arg2);
	break;
    case 0x10c:
        ret = mmunewpage();
        break;
    case 0x10d:
        copy(trap_frame, (void *)0xf040, sizeof(*trap_frame));
        __asm__("mr 1,%0\n\tb trap_finish_start" : : "r" 
                (((int)trap_frame) - 16));
        break;
    case 0x10e:
        dumpmap();
        break;

    case 0x200:
        SerialSetUp((int)arg1, arg2, 9600);
        break;
    case 0x201:
        TakeException((int)arg1, trap_frame);
        break;

    default:
	while(1);
    }

    /* Restore bats when we were called voluntarily.  We may not get a chance
     * to do this after returning.
     *
     * At this point, we're in address space that matches physical space.
     * We turn off mapping, restore bats, then let rfi switch us back to where
     * we came.
     */

    if (action >= 0x100)
    {
        __asm__("mfmsr %0" : "=r" (tmp));
        tmp &= ~0x30;
        __asm__("mtmsr %0" : : "r" (tmp));
        
        for(i = 0; i < 4; i++) {
            SetBat(i, 0, GetPhys(0xf000 + i * 16), GetPhys(0xf004 + i * 16));
            SetBat(i, 1, GetPhys(0xf008 + i * 16), GetPhys(0xf00c + i * 16));
        }
    }

    return ret;
}

void trapcallback(int action, ppc_trap_frame_t *trap_frame)
{
    if ((paddr_t)callback[action] < PAGETAB)
        callback[action](action, trap_frame);
    else
    {
        int framecopy = 0xf040;
        copy((void *)framecopy, trap_frame, sizeof(*trap_frame));
        trap_frame->srr0 = (int)callback[action];
        trap_frame->srr1 &= 0x7fff;
        trap_frame->gpr[3] = action;
        trap_frame->gpr[4] = framecopy;
        __asm__("mr 1,%0\n\tsubi 1,1,16\n\tb trap_finish_start" : : "r" (trap_frame));
    }
}

void outchar(char c)
{
    SetPhysByte(0x800003f8, c);
}

void copy(void *target, void *src, int bytes)
{
    while(bytes--) *((char *)target++) = *((char *)src++);
}

void outstr(const char *str)
{
    while(*str) outchar(*str);
}

void outdig(int dig)
{
    if(dig < 10) outchar(dig + '0');
    else outchar(dig - 10 + 'A');
}

void outnum(unsigned long num)
{
    int i;
    for( i = 0; i < 8; i++ ) 
    {
	outdig(num >> 28);
	num <<= 4;
    }
}

void fmtout(const char *str, ...)
{
    va_list ap;
    va_start(ap, str);
    while(*str)
    {
	if(*str == '%')
	{
	    if(str[1] == '%')
	    {
		outchar('%');
	    }
	    else if(str[1] == 's')
	    {
		outstr(va_arg(ap, const char *));
	    }
	    else
	    {
		outnum(va_arg(ap, int));
	    }
	    str++;
	}
	else
	{
	    outchar(*str);
	}
	str++;
    }
    va_end(ap);
}

void mmusetramsize(paddr_t ramsize)
{
    ppc_map_t *last_map = &PpcPageTable[PPC_PAGE_NUMBER(ramsize)];
    if(!RamSize)
    {
	RamSize = ramsize;
	FirstUsablePage = (paddr_t)last_map;
	NextPage = PPC_PAGE_NUMBER(FirstUsablePage) + 1;
    }
}

int ignore(int trapCode, ppc_trap_frame_t *trap)
{
    return 1;
}

int fpenable(int trapCode, ppc_trap_frame_t *trap)
{
        /* Turn on FP */
        trap->srr1 |= 8192;
        return 1;
}

extern int trap_start[], trap_end[];
void copy_trap_handler(int trap)
{
    int i;
    paddr_t targetArea = trap * 0x100;

    /* Set target addr */
    trap_end[0] = (int)_mmumain;

    for (i = 0; i <= trap_end - trap_start; i++)
    {
        SetPhys(targetArea + (i * sizeof(int)), trap_start[i]);
    }
}

void initme()
{
    int i;

    for(i = 0; i < HTABSIZ / sizeof(int); i++)
    {
	((int *)HTABORG)[i] = 0;
    }

    /* Default to hang on unknown exception */
    for(i = 0; i < 30; i++)
    {
        callback[i] = (MmuTrapHandler)TakeException;
        if (i != 1) /* Preserve reset handler */
            copy_trap_handler(i);
    }

    /* Serial Interrupt */
    callback[5] = 0; /* Do nothing until the user asks */

    /* Program Exception */
    callback[6] = (MmuTrapHandler)TakeException;

    /* Floating point exception */
    callback[8] = fpenable;

    /* Ignore decrementer and EE */
    callback[9] = ignore;

    /* Single Step */
    callback[0x20] = (MmuTrapHandler)TakeException;
}

ppc_map_t *allocpage()
{
    MmuFreePage *FreePage = 0;

    if (FreeList)
    {
        if ((void *)FreeList == (void *)PpcPageTable)
        {
            fmtout("Problem! FreeList: page 0 is free\n");
            while(1);
        }

	FreePage = FreeList;
	FreeList = FreeList->next;
        ((ppc_map_t*)FreePage)->addr = MMU_ADDR_RESERVED;
	return ((ppc_map_t*)FreePage);
    }
    else
    {
        while(!mmuisfreepage(NextPage) && NextPage < PPC_PAGE_NUMBER(RamSize))
        {
            NextPage++;
        }
        if (NextPage < PPC_PAGE_NUMBER(RamSize))
        {
            if (NextPage < 0x30)
            {
                fmtout("Problem! NextPage is low (%x)\n", NextPage);
                while(1);
            }
            
            PpcPageTable[NextPage].addr = MMU_ADDR_RESERVED;
            return &PpcPageTable[NextPage++];
        }
        else
        {
            return NULL;
        }
    }
}

void freepage(ppc_map_t *PagePtr)
{
    MmuFreePage *FreePage = (MmuFreePage*)PagePtr;
    PagePtr->proc = PagePtr->addr = 0;
    FreePage->next = FreeList;
    FreeList = FreePage;
}

MmuVsidTree *allocvsidtree()
{
    if(FreeTree)
    {
	MmuVsidTree *result = (MmuVsidTree*)FreeTree;
	FreeTree = FreeTree->next;
	return result;
    }
    else if(TreeAlloc >= 3 || !NextTreePage)
    {
	ppc_map_t *map = allocpage();
	NextTreePage = (MmuVsidTree*)PPC_PAGE_ADDR((map - PpcPageTable));
	TreeAlloc = 1;
	return NextTreePage;
    }
    else
    {
	return &NextTreePage[TreeAlloc++];
    }
}

void freevsidtree(MmuVsidTree *tree)
{
    int i;
    for(i = 0; i < 256; i++)
	if(tree->leaves[i])
	    freepage(tree->leaves[i]);
    MmuFreeTree *NextFreeTree = (MmuFreeTree *)tree;
    NextFreeTree->next = FreeTree;
    FreeTree = NextFreeTree;
}

void *allocvsid(int vsid)
{
    ppc_map_t *map = allocpage();
    MmuVsidInfo *info;
    if(!map) return 0;
    map->pte.pteh = map->pte.ptel = 0;
    info = (MmuVsidInfo*)PPC_PAGE_ADDR((map - PpcPageTable));
    info->vsid = vsid;
    info->next = VsidHead;
    VsidHead = info;
    return info;
}

void mmuallocvsid(int vsid, int mask)
{
    int i;
    for(i = 0; i < 16; i++)
    {
	if(mask & (1 << i))
	    allocvsid((vsid << 4) + i);
    }
}

MmuVsidInfo *findvsid(int vsid)
{
    MmuVsidInfo *info;
    for(info = VsidHead; info; info = info->next)
    {
	if(info->vsid == vsid) return info;
    }
    return 0;
}

void freevsid(int vsid)
{
    int i;
    MmuVsidInfo *info = findvsid(vsid);
    if(!info) return;
    ppc_map_t *map = &PpcPageTable[PPC_PAGE_NUMBER((paddr_t)info)];
    for(i = 0; i < 256; i++)
    {
	if(info->tree[i]) 
	    freevsidtree(info->tree[i]);
    }
    freepage(map);
}

void mmufreevsid(int vsid, int mask)
{
    int i;
    for(i = 0; i < 16; i++)
    {
	if(mask & (1 << i))
	    freevsid((vsid << 4) + i);
    }    
}

int mmuaddpage(ppc_map_info_t *info, int count)
{
    int i, iva = 0, vsid, phys, virt;
    int ptehi;
    int ptelo, vsid_table_hi, vsid_table_lo;
    ppc_map_t *PagePtr;
    MmuVsidInfo *VsidInfo;
    MmuVsidTree *VsidTree;

    for(i = 0; i < count; i++)
    {
        info[i].phys &= ~PPC_PAGE_MASK;
        info[i].addr &= ~PPC_PAGE_MASK;

	virt = info[i].addr;
	vsid = ((info[i].addr >> 28) & 15) | (info[i].proc << 4);
	VsidInfo = findvsid(vsid);

	if(!VsidInfo) return -1;

	ptehi = (1 << 31) | (vsid << 7) | ((virt >> 22) & 0x3f);
	
	if(info[i].phys) {
	    PagePtr = &PpcPageTable[PPC_PAGE_NUMBER(info[i].phys)];
	} else {
	    PagePtr = allocpage();
	    if(!PagePtr)
	    {
		return 0;
	    }
	}

	phys = PPC_PAGE_ADDR((PagePtr - PpcPageTable));
	ptelo = phys & ~PPC_PAGE_MASK;
	
        if (phys < 0x30000)
        {
            /* Should not be allocating physical */
            fmtout("Allocated physical: %x, logical %x\n", phys, virt);
            fmtout("PagePtr %x (page %d)\n", PagePtr, i);
            fmtout("info [ %x %x %x %x ]\n", info[i].proc, info[i].addr, info[i].flags, info[i].phys);
            while(1);
        }

	/* Update page data */
	PagePtr->pte.pteh = ptehi;
	PagePtr->pte.ptel = ptelo;
	PagePtr->proc = info[i].proc;
	PagePtr->addr = virt;

	vsid_table_hi = virt >> 20 & 255;
	vsid_table_lo = virt >> 12 & 255;

	if(!VsidInfo->tree[vsid_table_hi])
	    VsidInfo->tree[vsid_table_hi] = allocvsidtree();
	VsidTree = VsidInfo->tree[vsid_table_hi];
	if(!VsidTree) return 0;
	VsidTree->leaves[vsid_table_lo] = PagePtr;

	__asm__("tlbie %0\n\tsync\n\tisync" : : "r" (iva));
    }
    return 1;
}

paddr_t mmunewpage()
{
    ppc_map_t *PagePtr = allocpage();
    if (!PagePtr) return 0;
    return PPC_PAGE_ADDR(PagePtr - PpcPageTable);
}

ppc_pteg_t *PtegFromPage(ppc_map_t *map, int hfun)
{
    if(!map->proc && !map->addr) return 0;
    return &PpcHashedPTE[PtegNumber(map->addr, hfun)];
}

int PageMatch(vaddr_t addr, ppc_pte_t pte)
{
    int vsid_pte = (pte.pteh >> 7) & 15, api_pte = pte.pteh & 63;
    return 
	(((addr >> 28) & 15) == vsid_pte) &&
	(((addr >> 22) & 63) == api_pte);
}

ppc_map_t *mmuvirtmap(vaddr_t addr)
{
    int seg = (addr >> 28) & 15;
    MmuVsidInfo *seginfo = Segs[seg];
    MmuVsidTree *segtree = 0;
    if(!seginfo) return 0;
    segtree = seginfo->tree[(addr >> 20) & 255];
    if(!segtree) return 0;
    return segtree->leaves[(addr >> 12) & 255];
}

void mmudelpage(ppc_map_info_t *info, int count)
{
    int i, j, k, ipa;
    ppc_map_t *PagePtr;
    ppc_pteg_t *PageEntry;
    ppc_pte_t ZeroPte = { 0 };

    for(i = 0; i < count; i++)
    {
	if (info[i].phys)
	{
	    ipa = info[i].phys;
	    PagePtr = &PpcPageTable[ipa];
	    info[i].proc = PagePtr->proc;
	    info[i].addr = PagePtr->addr;
	}
	else
	{
	    PagePtr = mmuvirtmap(info[i].addr);
	    ipa = PPC_PAGE_ADDR(PagePtr - PpcPageTable);
	}

	for(j = 0; j < 2; j++)
	{
	    PageEntry = PtegFromPage(PagePtr, j);
	    for(k = 0; k < 8; k++)
	    {
		if(PageMatch(ipa, PageEntry->block[k]))
		{
		    if(PageEntry->block[k].ptel & 0x100)
			info[i].flags |= MMU_PAGE_DIRTY;
		    PageEntry->block[k] = ZeroPte;
		}
	    }
	}
	freepage(PagePtr);
	__asm__("tlbie %0\n\tsync\n\tisync" : : "r" (info[i].addr));
    }
}

void mmugetpage(ppc_map_info_t *info, int count)
{
    int i;
    ppc_map_t *PagePtr;
    
    for( i = 0; i < count; i++ )
    {
	if(!info[i].addr && !info[i].proc)
	{
	    PagePtr = &((ppc_map_t*)PAGETAB)[info[i].phys];
            info[i].proc = PagePtr->proc;
            info[i].addr = PagePtr->addr;
            info[i].flags = MMU_ALL_RW;
	} else {
	    vaddr_t addr = info[i].addr;
	    int vsid = ((addr >> 28) & 15) | (info[i].proc << 4);
	    PagePtr = mmuvirtmap(info[i].addr);
	    if(!PagePtr)
		info[i].phys = 0;
	    else
	    {
		info[i].phys = PPC_PAGE_ADDR(PagePtr - PpcPageTable);
		info[i].flags = MMU_ALL_RW; // HACK
	    }
	}
    }
}

int mmuisfreepage(paddr_t pageno)
{
    ppc_map_t *PagePtr = PpcPageTable + pageno;
    return !PagePtr->addr;
}

void mmusetvsid(int start, int end, int vsid)
{
    int i, sr, s_vsid;
    for(i = start; i < end; i++)
    {
	s_vsid = (vsid << 4) | (i & 15);
	sr = (GetSR(i) & ~PPC_VSID_MASK) | s_vsid;
	if (Booted)
            SetSR(i, sr);
	Segs[i] = findvsid(s_vsid);
        Vsid[i] = vsid;
    }
}

int ptegreload(ppc_trap_frame_t *frame, vaddr_t addr)
{
    int hfun = (Clock >> 3) & 1, ptegnum = PtegNumber(addr, hfun);
    ppc_map_t *map = mmuvirtmap(addr);
    if(!map) return 0;
    map->pte.pteh = (map->pte.pteh & ~64) | (hfun << 6);
    PpcHashedPTE[ptegnum].block[Clock & 7] = map->pte;
#if 0
    fmtout("Reloading addr %x (phys %x) at %x[%x] (%x:%x)\r\n",
	   addr, PPC_PAGE_ADDR(map - PpcPageTable), ptegnum, Clock & 15,
	   PpcHashedPTE[ptegnum].block[Clock&7].pteh,
	   PpcHashedPTE[ptegnum].block[Clock&7].ptel);
#endif
    Clock++;
    __asm__("tlbie %0\n\tsync\n\tisync" : : "r" (addr));
    return 1;
}

void printmap(vaddr_t vaddr, ppc_map_t *map)
{
    fmtout("%x: proc %x addr %x\n", 
           PPC_PAGE_ADDR(map - PpcPageTable), 
           map->proc, vaddr);
}

void dumptree(vaddr_t vaddr, MmuVsidTree *tree)
{
    int j;

    for (j = 0; j < 256; j++)
    {
        if (tree->leaves[j])
        {
            printmap(vaddr | (j << 12), tree->leaves[j]);
        }
    }
}

void dumpvsid(MmuVsidInfo *vsid)
{
    int i;

    fmtout("vsid %d (%x):\n", vsid->vsid>>4, vsid->vsid<<28);
    for (i = 0; i < 256; i++)
    {
        if (vsid->tree[i])
        {
            dumptree((vsid->vsid<<28) | i << 20, vsid->tree[i]);
        }
    }
}

void dumpmap()
{
    int i,j;
    ppc_map_t *map;
    MmuVsidInfo *vsid;
    fmtout("Address spaces:\n");
    for (vsid = VsidHead; vsid; vsid = vsid->next)
    {
        dumpvsid(vsid);
    }
}

void callkernel(void *fun_ptr, void *arg)
{
    int i;

    Booted = 1;

    for (i = 0; i < 16; i++)
    {
        // Patch up the vsid map.  We shouldn't muck with these until we're
        // booted.
        mmusetvsid(i, i+1, Vsid[i]);
    }

    void (*fun)(void *) = fun_ptr;
    __asm__("mfmsr 3\n\t"
            "ori 3,3,0x30\n\t"
            "mtmsr 3\n\t"
            "mtsdr1 %0\n\t" 
            "mr 0,%2\n\t"
            "mtctr 0\n\t"
            "mr 3,%1\n\t"
            "bctrl\n\t"
            : : "r" (HTABORG), "r" (arg), "r" (fun));
    /* BYE ! */
}
