#include "ppcmmu/mmu.h"
#include "ppcmmu/mmuutil.h"

inline int GetMSR() {
    register int res asm ("r3");
    __asm__("mfmsr 3");
    return res;
}

inline int GetDEC() {
    register int res asm ("r3");
    __asm__("mfdec 3");
    return res;
}

__asm__("\t.globl GetPhys\n"
	"GetPhys:\t\n"
	"mflr  0\n\t"
	"stwu  0,-16(1)\n\t"
	"mfmsr 5\n\t"
	"andi. 6,5,0xffef\n\t"/* turn off MSR[DR] */
	"mtmsr 6\n\t"
	"isync\n\t"
	"sync\n\t"
	"lwz   3,0(3)\n\t"    /* Get actual value at phys addr r3 */
	"mtmsr 5\n\t"
	"isync\n\t"
	"sync\n\t"
	"lwz   0,0(1)\n\t"
	"addi  1,1,16\n\t"
	"mtlr  0\n\t"
	"blr"
    );

__asm__("\t.globl GetPhysHalf\n"
	"GetPhysHalf:\t\n"
	"mflr  0\n\t"
	"stwu  0,-16(1)\n\t"
	"mfmsr 5\n\t"
	"andi. 6,5,0xffef\n\t"/* turn off MSR[DR] */
	"mtmsr 6\n\t"
	"isync\n\t"
	"sync\n\t"
	"lhz   3,0(3)\n\t"    /* Get actual value at phys addr r3 */
	"mtmsr 5\n\t"
	"isync\n\t"
	"sync\n\t"
	"lwz   0,0(1)\n\t"
	"addi  1,1,16\n\t"
	"mtlr  0\n\t"
	"blr"
    );

__asm__("\t.globl GetPhysByte\n"
	"GetPhysByte:\t\n"
	"mflr  0\n\t"
	"stwu  0,-16(1)\n\t"
	"mfmsr 5\n\t"
	"andi. 6,5,0xffef\n\t"/* turn off MSR[DR] */
	"mtmsr 6\n\t"
	"isync\n\t"
	"sync\n\t"
	"lbz   3,0(3)\n\t"    /* Get actual value at phys addr r3 */
	"mtmsr 5\n\t"
	"isync\n\t"
	"sync\n\t"
	"lwz   0,0(1)\n\t"
	"addi  1,1,16\n\t"
	"mtlr  0\n\t"
	"blr"
    );

__asm__("\t.globl SetPhys\n"
	"SetPhys:\t\n"
	"mflr  0\n\t"
	"stwu  0,-16(1)\n\t"
	"mfmsr 5\n\t"
	"andi. 6,5,0xffef\n\t"/* turn off MSR[DR] */
	"mtmsr 6\n\t"
	"sync\n\t"
	"eieio\n\t"
	"stw   4,0(3)\n\t"    /* Set actual value at phys addr r3 */
	"dcbst 0,3\n\t"
	"mtmsr 5\n\t"
	"sync\n\t"
	"eieio\n\t"
	"mr    3,4\n\t"
	"lwz   0,0(1)\n\t"
	"addi  1,1,16\n\t"
	"mtlr  0\n\t"
	"blr"
    );

__asm__("\t.globl SetPhysHalf\n"
	"SetPhysHalf:\t\n"
	"mflr  0\n\t"
	"stwu  0,-16(1)\n\t"
	"mfmsr 5\n\t"
	"andi. 6,5,0xffef\n\t"/* turn off MSR[DR] */
	"mtmsr 6\n\t"
	"sync\n\t"
	"eieio\n\t"
	"sth   4,0(3)\n\t"    /* Set actual value at phys addr r3 */
	"dcbst 0,3\n\t"
	"mtmsr 5\n\t"
	"sync\n\t"
	"eieio\n\t"
	"mr    3,4\n\t"
	"lwz   0,0(1)\n\t"
	"addi  1,1,16\n\t"
	"mtlr  0\n\t"
	"blr"
    );

__asm__("\t.globl SetPhysByte\n"
	"SetPhysByte:\t\n"
	"mflr  0\n\t"
	"stwu  0,-16(1)\n\t"
	"mfmsr 5\n\t"
	"andi. 6,5,0xffef\n\t"/* turn off MSR[DR] */
	"mtmsr 6\n\t"
	"sync\n\t"
	"eieio\n\t"
	"stb   4,0(3)\n\t"    /* Set actual value at phys addr r3 */
	"dcbst 0,3\n\t"
	"mtmsr 5\n\t"
	"sync\n\t"
	"eieio\n\t"
	"mr    3,4\n\t"
	"lwz   0,0(1)\n\t"
	"addi  1,1,16\n\t"
	"mtlr  0\n\t"
	"blr"
    );

inline int GetSR(int n) {
    register int res = 0;
    switch( n ) {
    case 0:
	__asm__("mfsr %0,0" : "=r" (res));
	break;
    case 1:
	__asm__("mfsr %0,1" : "=r" (res));
	break;
    case 2:
	__asm__("mfsr %0,2" : "=r" (res));
	break;
    case 3:
	__asm__("mfsr %0,3" : "=r" (res));
	break;
    case 4:
	__asm__("mfsr %0,4" : "=r" (res));
	break;
    case 5:
	__asm__("mfsr %0,5" : "=r" (res));
	break;
    case 6:
	__asm__("mfsr %0,6" : "=r" (res));
	break;
    case 7:
	__asm__("mfsr %0,7" : "=r" (res));
	break;
    case 8:
	__asm__("mfsr %0,8" : "=r" (res));
	break;
    case 9:
	__asm__("mfsr %0,9" : "=r" (res));
	break;
    case 10:
	__asm__("mfsr %0,10" : "=r" (res));
	break;
    case 11:
	__asm__("mfsr %0,11" : "=r" (res));
	break;
    case 12:
	__asm__("mfsr %0,12" : "=r" (res));
	break;
    case 13:
	__asm__("mfsr %0,13" : "=r" (res));
	break;
    case 14:
	__asm__("mfsr %0,14" : "=r" (res));
	break;
    case 15:
	__asm__("mfsr %0,15" : "=r" (res));
	break;
    }
    return res;
}

inline void SetSR(int n, int val) {
    switch( n ) {
    case 0:
	__asm__("mtsr 0,%0" : : "r" (val));
	break;
    case 1:
	__asm__("mtsr 1,%0" : : "r" (val));
	break;
    case 2:
	__asm__("mtsr 2,%0" : : "r" (val));
	break;
    case 3:
	__asm__("mtsr 3,%0" : : "r" (val));
	break;
    case 4:
	__asm__("mtsr 4,%0" : : "r" (val));
	break;
    case 5:
	__asm__("mtsr 5,%0" : : "r" (val));
	break;
    case 6:
	__asm__("mtsr 6,%0" : : "r" (val));
	break;
    case 7:
	__asm__("mtsr 7,%0" : : "r" (val));
	break;
    case 8:
	__asm__("mtsr 8,%0" : : "r" (val));
	break;
    case 9:
	__asm__("mtsr 9,%0" : : "r" (val));
	break;
    case 10:
	__asm__("mtsr 10,%0" : : "r" (val));
	break;
    case 11:
	__asm__("mtsr 11,%0" : : "r" (val));
	break;
    case 12:
	__asm__("mtsr 12,%0" : : "r" (val));
	break;
    case 13:
	__asm__("mtsr 13,%0" : : "r" (val));
	break;
    case 14:
	__asm__("mtsr 14,%0" : : "r" (val));
	break;
    case 15:
	__asm__("mtsr 15,%0" : : "r" (val));
	break;
    }
}

void GetBat( int bat, int inst, int *batHi, int *batLo ) {
    register int bh asm("r3"), bl asm("r4");
    if( inst ) {
	switch( bat ) {
	case 0:
	    __asm__("mfibatu 3,0");
	    __asm__("mfibatl 4,0");
	    break;
	case 1:
	    __asm__("mfibatu 3,1");
	    __asm__("mfibatl 4,1");
	    break;
	case 2:
	    __asm__("mfibatu 3,2");
	    __asm__("mfibatl 4,2");
	    break;
	case 3:
	    __asm__("mfibatu 3,3");
	    __asm__("mfibatl 4,3");
	    break;
	}
    } else {
	switch( bat ) {
	case 0:
	    __asm__("mfdbatu 3,0");
	    __asm__("mfdbatl 4,0");
	    break;
	case 1:
	    __asm__("mfdbatu 3,1");
	    __asm__("mfdbatl 4,1");
	    break;
	case 2:
	    __asm__("mfdbatu 3,2");
	    __asm__("mfdbatl 4,2");
	    break;
	case 3:
	    __asm__("mfdbatu 3,3");
	    __asm__("mfdbatl 4,3");
	    break;
	}
    }
    *batHi = bh;
    *batLo = bl;
}

#define BATSET(n,t) \
 case n: __asm__("mt" #t "batu " #n ",%0\n\tmt" #t "batl " #n ",%1" \
 : : "r" (batHi), "r" (batLo)); break;

void SetBat( int bat, int inst, int batHi, int batLo ) {
    if( inst ) {
	switch( bat ) {
	    BATSET(0,i);
	    BATSET(1,i);
	    BATSET(2,i);
	    BATSET(3,i);
	}
    } else {
	switch( bat ) {
	    BATSET(0,d);
	    BATSET(1,d);
	    BATSET(2,d);
	    BATSET(3,d);
	}
    }
    __asm__("isync\n\tsync");
}

inline int GetSDR1() {
    register int res asm("r3");
    __asm__("mfsdr1 3");
    return res;
}

inline void SetSDR1( int sdr ) {
    int i,j;
    __asm__("mtsdr1 3");
    __asm__("sync");
    __asm__("isync");
    
    for( i = 0; i < 256; i++ ) {
	j = i << 12;
	__asm__("tlbie %0,0" : : "r" (j));
    }
    __asm__("eieio");
    __asm__("tlbsync");
    __asm__("ptesync");
}

inline int BatTranslate( int batu, int batl, int virt ) {
    int mask;
    if(batu & 0x3fc)
    {
	mask = ~(0x1ffff | ((batu & 0x3fc)>>2)<<17);
	if((batu & 2) && ((batu & mask) == (virt & mask)))
	    return (batl & mask) | (virt & ~mask);
    } else {
	mask = ~(0x1ffff | (batl << 17));
	if(!(batl & 0x40) || ((batu & mask) != (virt & mask)))
	    return (batl & mask) | (virt & ~mask);
    }
    return -1;
}

inline int BatHit( int batu, int batl, int virt ) {
    return BatTranslate( batu, batl, virt ) != -1;
}

/* translate address */
int PpcVirt2phys( vaddr_t virt, int inst ) {
    int msr = GetMSR();
    int txmask = inst ? 0x20 : 0x10;
    int i, bath, batl, sr, sdr1, physbase, vahi, valo;
    int npteg, hash, hashmask, ptehi, ptelo, ptegaddr;
    int vsid, pteh, ptevsid, pteapi;
		
    if( msr & txmask ) {
	sr = GetSR( virt >> 28 );
	vsid = sr & 0xfffffff;
	vahi = vsid >> 4;
	valo = (vsid << 28) | (virt & 0xfffffff);
	if( sr & 0x80000000 ) {
	    return valo;
	}

	for( i = 0; i < 4; i++ ) {
	    GetBat( i, inst, &bath, &batl );
	    if( BatHit( bath, batl, virt ) ) {
		return BatTranslate( bath, batl, virt );
	    }
	}

	sdr1 = GetSDR1();

	physbase = sdr1 & ~0xffff;
	hashmask = ((sdr1 & 0x1ff) << 10) | 0x3ff;
	hash = (vsid & 0x7ffff) ^ ((valo >> 12) & 0xffff);
	npteg = hashmask + 1;

	for( pteh = 0; pteh < 0x80; pteh += 64, hash ^= 0x7ffff ) {
	    ptegaddr = ((hashmask & hash) * 64) + physbase;

	    for( i = 0; i < 8; i++ ) {
		ptehi = GetPhys( ptegaddr + (i * 8) );
		ptelo = GetPhys( ptegaddr + (i * 8) + 4 );

		ptevsid = (ptehi >> 7) & 0xffffff;
		pteapi = ptehi & 0x3f;

		if( (ptehi & 64) != pteh ) continue;
		if( ptevsid != (vsid & 0xffffff) ) continue;
		if( pteapi != ((virt >> 22) & 0x3f) ) continue;
		
		return (ptelo & 0xfffff000) | (virt & 0xfff);
	    }
	}
	return -1;
    } else {
	return virt;
    }
}

int PtegNumber(vaddr_t virt, int hfun)
{
    int sr = GetSR( (virt >> 28) & 0xf );
    int vsid = sr & PPC_VSID_MASK;
    return ((((vsid & 0x7ffff) ^ ((virt >> 12) & 0xffff)) ^ (hfun ? -1 : 0)) & ((HTABSIZ - 1) >> 3) & 0x3ff);
}
