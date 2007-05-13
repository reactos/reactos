#include "mmu.h"
#include "mmuutil.h"
#include "mmuobject.h"

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
*/

MmuPageCallback callback;

__asm__(".globl mmumain\n\t"
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

/*
 * Trap frame:
 * r0 .. r32
 * lr, ctr, srr0, srr1, dsisr
 */
__asm__(".globl data_miss_start\n\t"
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
	"lis 5,_mmumain@ha\n\t"
	"addi 5,5,_mmumain@l\n\t"
	"mtlr 5\n\t"
	"blrl\n"
	"data_miss_end:\n\t"
	".space 4");

extern int data_miss_start, data_miss_end;

int _mmumain(int action, void *arg1, void *arg2, void *arg3)
{
    int ret = 0, i;
    int *start, *end, *target;

    switch(action)
    {
    case 0:
	/* Set up segments */
	__asm__("xor 0,0,0\n\t"
                "mtsr 0,0\n\t"
		"mtsr 0,1\n\t"
		"mtsr 0,2\n\t"
		"mtsr 0,3\n\t"
		"mtsr 0,4\n\t"
		"mtsr 0,5\n\t"
		"mtsr 0,6\n\t"
		"mtsr 0,7\n\t"
		"mtsr 0,8\n\t"
		"mtsr 0,9\n\t"
		"mtsr 0,10\n\t"
		"mtsr 0,11\n\t"
		"mtsr 0,12\n\t"
		"mtsr 0,13\n\t"
		"mtsr 0,14\n\t"
		"mtsr 0,15\n\t");
	for(i = 0; i < HTABSIZ / sizeof(int); i++)
	{
	    ((int *)HTABORG)[i] = 0;
	}
	for(target = (int *)0x300, start = &data_miss_start; start < &data_miss_end; start++, target++)
	{
	    *target = *start;
	}

	(&data_miss_start)[46]++;

	for(target = (int *)0x400, start = &data_miss_start; start < &data_miss_end; start++, target++)
	{
	    *target = *start;
	}
	break;
    case 1:
	mmuaddpage(arg1, arg2, (int)arg3);
	break;
    case 2:
	mmudelpage(arg1);
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
	ret = mmugetpage(arg1, arg2);
	break;
    case 6:
	ret = mmunitest();
	break;
    case 7:
	__asm__("mfmsr 3\n\tori 3,3,0x30\n\tmtmsr 3\n\tmtsdr1 %0" : : "r" (HTABORG));
	break;
    case 100:
	__asm__("mfmsr 3\n\tori 3,3,0x30\n\tmtmsr 3\n\t");
	callback(0,arg1);
	break;
    case 101:
	__asm__("mfmsr 3\n\tori 3,3,0x30\n\tmtmsr 3\n\t");
	callback(1,arg1);
	break;
    default:
	while(1);
    }

    return ret;
}

void mmuaddpage(void *va, ppc_map_info_t *info, int count)
{
    int i;
    int iva; 
    for(i = 0; i < count; i++)
    {
	iva = (int)va + (i << 12);
	InsertPageEntry(iva, info[i].phys, -1, HTABORG);
	__asm__("tlbie %0" : : "r" (iva));
    }
}

void mmudelpage(void *va)
{
}

int mmugetpage(void *va, ppc_map_info_t *info)
{
    
}

void mmusetvsid(int start, int end, int vsid)
{
    int i, sr;
    for(i = start; i < end; i++)
    {
	sr = GetSR(i) & ~VSID_MASK | (vsid << 4) | (i & 15);
	SetSR(i, sr);
    }
}

