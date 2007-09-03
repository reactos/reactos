#include <stdarg.h>
#include "ppcmmu/mmu.h"
#include "ppcmmu/mmuutil.h"
#include "mmuobject.h"
#include "helper.h"

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

MmuPageCallback callback;
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
MmuFreePage *FreeList;
// Pages are allocated one by one until NextPage == RamSize >> PPC_PAGE_SHIFT
// Then we take only from the free list
int Clock = 0, TreeAlloc = 0;
paddr_t RamSize, FirstUsablePage, NextPage;
MmuVsidTree *NextTreePage = 0;
MmuFreeTree *FreeTree;
MmuVsidInfo *Segs[16], *VsidHead = 0;

extern void fmtout(const char *fmt, ...);
int ptegreload(ppc_trap_frame_t *frame, vaddr_t addr);

__asm__(".text\n\t"
	".globl mmumain\n\t"
	".globl _mmumain\n\t"
	".globl oldstack\n\t"
	"mmumain:\n\t"
	"lis 7,oldstack@ha\n\t"
	"addi 7,7,oldstack@l\n\t"
	"mflr 0\n\t"
 	"stw 1,0(7)\n\t"
	"lis 1,2\n\t"
	"subi 1,1,16\n\t"
	"stw 0,0(1)\n\t"
	"bl _mmumain\n\t"
	"lis 7,oldstack@ha\n\t"
	"addi 7,7,oldstack@l\n\t"
	"lwz 0,0(1)\n\t"
	"lwz 1,0(7)\n\t"
	"mtlr 0\n\t"
	"blr\n"
	"oldstack:\n\t"
	".long 0\n\t");

__asm__(".text\n\t"
	".globl data_miss_finish_start\n"
	"data_miss_finish_start:\n\t"
	"lwz 2,8(1)\n\t"
	"lwz 3,12(1)\n\t"
	"lwz 4,16(1)\n\t"
	"lwz 5,20(1)\n\t"
	"lwz 6,24(1)\n\t"
	"lwz 7,28(1)\n\t"
	"lwz 8,32(1)\n\t"
	"lwz 9,36(1)\n\t"
	"lwz 10,40(1)\n\t"
	"lwz 11,44(1)\n\t"
	"lwz 12,48(1)\n\t"
	"lwz 13,52(1)\n\t"
	"lwz 14,56(1)\n\t"
	"lwz 15,60(1)\n\t"
	"lwz 16,64(1)\n\t"
	"lwz 17,68(1)\n\t"
	"lwz 18,72(1)\n\t"
	"lwz 19,76(1)\n\t"
	"lwz 20,80(1)\n\t"
	"lwz 21,84(1)\n\t"
	"lwz 22,88(1)\n\t"
	"lwz 23,92(1)\n\t"
	"lwz 24,96(1)\n\t"
	"lwz 25,100(1)\n\t"
	"lwz 26,104(1)\n\t"
	"lwz 27,108(1)\n\t"
	"lwz 28,112(1)\n\t"
	"lwz 29,116(1)\n\t"
	"lwz 30,120(1)\n\t"
	"lwz 31,124(1)\n\t"
	"lwz 0,128(1)\n\t"
	"mtlr 0\n\t"
	"lwz 0,132(1)\n\t"
	"mtcr 0\n\t"
	"lwz 0,136(1)\n\t"
	"mtctr 0\n\t"
	"lwz 0,0(1)\n\t"
	"mfsprg1 1\n\t"
	"rfi\n\t");
	
/*
 * Trap frame:
 * r0 .. r32
 * lr, ctr, srr0, srr1, dsisr
 */
__asm__(".text\n\t"
	".globl data_miss_start\n\t"
	".globl data_miss_end\n\t"
	"data_miss_start:\n\t"
	"mtsprg1 1\n\t"
	"lis 1,2\n\t"
	"subi 1,1,256\n\t"
	"stw 0,0(1)\n\t"
	"mfsprg1 0\n\t"
	"stw 0,4(1)\n\t"
	"stw 2,8(1)\n\t"
	"stw 3,12(1)\n\t"
	"stw 4,16(1)\n\t"
	"stw 5,20(1)\n\t"
	"stw 6,24(1)\n\t"
	"stw 7,28(1)\n\t"
	"stw 8,32(1)\n\t"
	"stw 9,36(1)\n\t"
	"stw 10,40(1)\n\t"
	"stw 11,44(1)\n\t"
	"stw 12,48(1)\n\t"
	"stw 13,52(1)\n\t"
	"stw 14,56(1)\n\t"
	"stw 15,60(1)\n\t"
	"stw 16,64(1)\n\t"
	"stw 17,68(1)\n\t"
	"stw 18,72(1)\n\t"
	"stw 19,76(1)\n\t"
	"stw 20,80(1)\n\t"
	"stw 21,84(1)\n\t"
	"stw 22,88(1)\n\t"
	"stw 23,92(1)\n\t"
	"stw 24,96(1)\n\t"
	"stw 25,100(1)\n\t"
	"stw 26,104(1)\n\t"
	"stw 27,108(1)\n\t"
	"stw 28,112(1)\n\t"
	"stw 29,116(1)\n\t"
	"stw 30,120(1)\n\t"
	"stw 31,124(1)\n\t"
	"mflr 0\n\t"
	"stw 0,128(1)\n\t"
	"mfcr 0\n\t"
	"stw 0,132(1)\n\t"
	"mfctr 0\n\t"
	"stw 0,136(1)\n\t"
	"mfsrr0 0\n\t"
	"stw 0,140(1)\n\t"
	"mfsrr1 0\n\t"
	"stw 0,144(1)\n\t"
	"mfdsisr 0\n\t"
	"stw 0,148(1)\n\t"
	"mfdar 0\n\t"
	"stw 0,152(1)\n\t"
	"mfxer 0\n\t"
	"stw 0,156(1)\n\t"
	"li 3,100\n\t"
	"mr 4,1\n\t"
	"lis 5,data_miss_finish_start@ha\n\t"
	"addi 5,5,data_miss_finish_start@l\n\t"
	"mtlr 5\n\t"
	"lis 5,_mmumain@ha\n\t"
	"addi 5,5,_mmumain@l\n\t"
	"mtctr 5\n\t"
	"bctr\n"
	"data_miss_end:\n\t"
	".space 4");

extern int data_miss_end, data_miss_start;

int _mmumain(int action, void *arg1, void *arg2, void *arg3)
{
    void (*fun)(void *) = arg1;
    ppc_trap_frame_t *trap_frame = arg1;
    int ret = 0;

    switch(action)
    {
    case 0:
	initme();
	break;
    case 1:
	ret = mmuaddpage(arg1, (int)arg2);
	break;
    case 2:
	mmudelpage(arg1, (int)arg2);
	break;
    case 3:
	mmusetvsid((int)arg1, (int)arg2, (int)arg3);
	break;
    case 4:
	/* Miss callback = arg1 */
	ret = (int)callback;
	callback = arg1;
	break;
    case 5:
	mmugetpage(arg1, (int)arg2);
	break;
    case 6:
	ret = mmunitest();
	break;
    case 7:
	__asm__("mfmsr 3\n\t"
		"ori 3,3,0x30\n\t"
		"mtmsr 3\n\t"
		"mtsdr1 %0\n\t" 
		"mr 0,%2\n\t"
		"mtctr 0\n\t"
		"mr 3,%1\n\t"
		"bctrl\n\t"
		: : "r" (HTABORG), "r" (arg2), "r" (fun));
	/* BYE ! */
	break;
    case 8:
	mmusetramsize((paddr_t)arg1);
	break;
    case 9:
	return FirstUsablePage;
    case 10:
	mmuallocvsid((int)arg1, (int)arg2);
	break;
    case 11:
	mmufreevsid((int)arg1, (int)arg2);
	break;
    case 100:
	if(!ptegreload(trap_frame, trap_frame->dar))
	{
	    __asm__("mfmsr 3\n\tori 3,3,0x30\n\tmtmsr 3\n\t");
	    callback(0,arg1);
	}
	break;
    case 101:
	if(!ptegreload(trap_frame, trap_frame->srr0))
	{
	    __asm__("mfmsr 3\n\tori 3,3,0x30\n\tmtmsr 3\n\t");
	    callback(1,arg1);
	}
	break;
    default:
	while(1);
    }

    return ret;
}

void outchar(char c)
{
    SetPhysByte(0x800003f8, c);
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
	NextPage = PPC_PAGE_NUMBER(FirstUsablePage);
    }
}

void initme()
{
    int i;
    int *target, *start;

    for(i = 0; i < HTABSIZ / sizeof(int); i++)
    {
	((int *)HTABORG)[i] = 0;
    }

    for(target = (int *)0x300, start = &data_miss_start; start < &data_miss_end; start++, target++)
    {
	SetPhys((paddr_t)target, *start);
    }
    
    (&data_miss_start)[50]++;
    
    for(target = (int *)0x400, start = &data_miss_start; start < &data_miss_end; start++, target++)
    {
	SetPhys((paddr_t)target, *start);
    }
}

ppc_map_t *allocpage()
{
    MmuFreePage *FreePage = 0;

    if(NextPage < PPC_PAGE_NUMBER(RamSize)) {
	return &PpcPageTable[NextPage++];
    } else {
	FreePage = FreeList;
	FreeList = FreeList->next;
	return ((ppc_map_t*)FreePage);
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
	    allocvsid((vsid << 4) + i);
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

ppc_map_t *mmuvirtmap(vaddr_t addr, int vsid)
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
	    PagePtr = mmuvirtmap(info[i].proc, info[i].addr);
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
	    PagePtr = mmuvirtmap(info[i].addr, vsid);
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

void mmusetvsid(int start, int end, int vsid)
{
    int i, sr, s_vsid;
    for(i = start; i < end; i++)
    {
	s_vsid = (vsid << 4) | (i & 15);
	sr = (GetSR(i) & ~PPC_VSID_MASK) | s_vsid;
	SetSR(i, sr);
	Segs[i] = findvsid(s_vsid);
    }
}

int ptegreload(ppc_trap_frame_t *frame, vaddr_t addr)
{
    int hfun = (Clock >> 3) & 1, ptegnum = PtegNumber(addr, hfun);
    int vsid = GetSR((addr >> 28) & 15) & PPC_VSID_MASK;
    ppc_map_t *map = mmuvirtmap(addr, vsid);
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
