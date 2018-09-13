#include "shinc.hpp"
#pragma hdrstop

#define toLowerMask 0x20202020

unsigned hashPbCb(PB pb, CB cb, unsigned long ulMod)
{
    unsigned long   ulHash  = 0;

    // hash leading dwords using Duff's Device
    long    cl      = cb >> 2;
    UNALIGNED unsigned long*        pul     = (unsigned long*)pb;
    UNALIGNED unsigned long*        pulMac  = pul + cl;
    int     dcul    = cl & 7;

    switch (dcul) {
        do {
            dcul = 8;
            ulHash ^= pul[7];
        case 7: ulHash ^= pul[6];
        case 6: ulHash ^= pul[5];
        case 5: ulHash ^= pul[4];
        case 4: ulHash ^= pul[3];
        case 3: ulHash ^= pul[2];
        case 2: ulHash ^= pul[1];
        case 1: ulHash ^= pul[0];
        case 0: ;
        } while ((pul += dcul) < pulMac);
    }

    pb = (PB) pul;

    // hash possible odd word
    if (cb & 2) {
        ulHash ^= *(UNALIGNED unsigned short*)pb;
        pb = (PB)((UNALIGNED unsigned short*)pb + 1);
    }

    // hash possible odd byte
    if (cb & 1) {
        ulHash ^= *(pb++);
    }

    ulHash |= toLowerMask;
    ulHash ^= (ulHash >> 11);

    return (unsigned)((ulHash ^ (ulHash >> 16)) % ulMod);
}
