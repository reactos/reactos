#include "mmu.h"
#include "mmuutil.h"
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
	".globl data_miss_finish_start\n\t"
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
	"mfctr 0\n\t"
	"lwz 0,4(1)\n\t"
	"mfsprg1 1\n\t"
	"lwz 0,0(1)\n\t"
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
	"mfctr 0\n\t"
	"stw 0,132(1)\n\t"
	"mfsrr0 0\n\t"
	"stw 0,136(1)\n\t"
	"mfsrr1 0\n\t"
	"stw 0,140(1)\n\t"
	"mfdsisr 0\n\t"
	"stw 0,144(1)\n\t"
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
    int ret = 0, i;
    int *start, *end, *target;

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
	allocvsid((int)arg1);
	break;
    case 11:
	freevsid((int)arg1);
	break;
    case 100:
	if(!ptegreload(trap_frame))
	{
	    __asm__("mfmsr 3\n\tori 3,3,0x30\n\tmtmsr 3\n\t");
	    callback(0,arg1);
	}
	break;
    case 101:
	if(!ptegreload(trap_frame))
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

void mmusetramsize(paddr_t ramsize)
{
    ppc_map_t *last_map = &PpcPageTable[PPC_PAGE_NUMBER(ramsize)];
    if(!RamSize)
    {
	RamSize = ramsize;
	FirstUsablePage = ROUND_UP((paddr_t)last_map, PPC_PAGE_ADDR(1));
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
    
    (&data_miss_start)[46]++;
    
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
	NextTreePage = (MmuVsidTree*)PPC_PAGE_ADDR(map - PpcPageTable);
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
    info = (MmuVsidInfo*)PPC_PAGE_ADDR(map - PpcPageTable);
    info->vsid = vsid;
    info->next = VsidHead;
    VsidHead = info;
    return info;
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

int mmuaddpage(ppc_map_info_t *info, int count)
{
    int i, iva, vsid, phys, virt;
    int ptehi;
    int ptelo, vsid_table_hi, vsid_table_lo;
    ppc_map_t *PagePtr;
    MmuFreePage *FreePage;
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

	phys = PPC_PAGE_ADDR(PagePtr - PpcPageTable);
	ptelo = phys & ~PPC_PAGE_MASK;

	/* Update page data */
	PagePtr->pte.pteh = ptehi;
	PagePtr->pte.ptel = ptelo;
	PagePtr->proc = info->proc;
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

void mmudelpage(ppc_map_info_t *info, int count)
{
    int i, j, k, ipa;
    ppc_map_t *PagePtr;
    ppc_pteg_t *PageEntry;
    ppc_pte_t ZeroPte = { 0 };
    MmuFreePage *FreePage;

    for(i = 0; i < count; i++)
    {
	ipa = info[i].phys;

	PagePtr = &PpcPageTable[i];
	for(j = 0; j < 2; j++)
	{
	    PageEntry = PtegFromPage(PagePtr, 0);
	    for(k = 0; k < 8; k++)
	    {
		if(PageMatch(ipa, PageEntry->block[k]))
		    PageEntry->block[k] = ZeroPte;
	    }
	}
	freepage(PagePtr);
	__asm__("tlbie %0\n\tsync\n\tisync" : : "r" (info[i].addr));
    }
}

ppc_map_t *mmuvirtmap(vaddr_t addr, int vsid)
{
    int seg = (addr >> 28) & 15, ptegnum;
    MmuVsidInfo *seginfo = Segs[seg];
    MmuVsidTree *segtree = 0;
    if(!seginfo) return 0;
    segtree = seginfo->tree[(addr >> 20) & 255];
    if(!segtree) return 0;
    return segtree->leaves[(addr >> 12) & 255];
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
	    info->proc = PagePtr->proc;
	    info->addr = PagePtr->addr;
	    info->flags = MMU_ALL_RW;
	} else {
	    vaddr_t addr = info[i].addr;
	    int vsid = ((addr >> 28) & 15) | (info[i].proc << 4);
	    mmuvirtmap(info[i].addr, vsid);
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

int ptegreload(ppc_trap_frame_t *frame)
{
    int addr = frame->srr0, ptegnum = PtegNumber(addr, (Clock >> 3) & 1);
    int vsid = GetSR((addr >> 28) & 15) & PPC_VSID_MASK;
    ppc_map_t *map = mmuvirtmap(addr, vsid);
    if(!map) return 0;
    PpcHashedPTE[ptegnum].block[Clock & 7] = map->pte;
    Clock++;
    __asm__("tlbie %0\n\tsync\n\tisync" : : "r" (addr));
    return 1;
}
