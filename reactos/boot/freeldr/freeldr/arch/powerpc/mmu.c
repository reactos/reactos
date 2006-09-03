#include <freeldr.h>
#include "mmu.h"

inline int GetMSR() {
    register int res asm ("r3");
    __asm__("mfmsr 3");
    return res;
}

__asm__("\t.globl GetPhys\n"
	"GetPhys:\t\n"
	"mflr  0\n\t"
	"stwu  0,-16(1)\n\t"
	"mfmsr 5\n\t"
	"xori  3,3,4\n\t"     /* Undo effects of LE without swapping */
	"andi. 6,5,0xffef\n\t"/* turn off MSR[DR] */
	"mtmsr 6\n\t"
	"lwz   3,0(3)\n\t"    /* Get actual value at phys addr r3 */
	"mtmsr 5\n\t"
	"lwz   0,0(1)\n\t"
	"addi  1,1,16\n\t"
	"mtlr  0\n\t"
	"blr"
    );

inline int GetSR(int n) {
    register int res asm ("r3");
    switch( n ) {
    case 0:
	__asm__("mfsr 3,0");
	break;
    case 1:
	__asm__("mfsr 3,1");
	break;
    case 2:
	__asm__("mfsr 3,2");
	break;
    case 3:
	__asm__("mfsr 3,3");
	break;
    case 4:
	__asm__("mfsr 3,4");
	break;
    case 5:
	__asm__("mfsr 3,5");
	break;
    case 6:
	__asm__("mfsr 3,6");
	break;
    case 7:
	__asm__("mfsr 3,7");
	break;
    case 8:
	__asm__("mfsr 3,8");
	break;
    case 9:
	__asm__("mfsr 3,9");
	break;
    case 10:
	__asm__("mfsr 3,10");
	break;
    case 11:
	__asm__("mfsr 3,11");
	break;
    case 12:
	__asm__("mfsr 3,12");
	break;
    case 13:
	__asm__("mfsr 3,13");
	break;
    case 14:
	__asm__("mfsr 3,14");
	break;
    case 15:
	__asm__("mfsr 3,15");
	break;
    }
    return res;
}

inline void GetBat( int bat, int inst, int *batHi, int *batLo ) {
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

inline int GetSDR1() {
    register int res asm("r3");
    __asm__("mfsdr1 3");
    return res;
}

inline int BatHit( int bath, int batl, int virt ) {
    int mask = 0xfffe0000 & ~((batl & 0x3f) << 17);
    return (batl & 0x40) && ((virt & mask) == (bath & mask));
}

inline int BatTranslate( int bath, int batl, int virt ) {
    return (virt & 0x007fffff) | (batl & 0xfffe0000);
}

/* translate address */
int PpcVirt2phys( int virt, int inst ) {
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
