/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

 /* 2003-01-07  andy@warmcat.com  Created
 */

#include "boot.h"
#include "BootFlash.h"
#include "memory_layout.h"

void DisplayFlashProgressBar(int, int, unsigned long);

const KNOWN_FLASH_TYPE aknownflashtypesDefault[] = {
	{ 0x01, 0xa4, "AMD - Am29F040B",0x80000 },
	{ 0x01, 0xd5, "AMD - Am29F080B",0x100000 },
	{ 0x01, 0xda, "AMD - Am29LV800B",0x100000 },
	{ 0x04, 0xd5, "Fujitsu - MBM29F080A",0x100000 },
	{ 0xad, 0xb0, "Hynix - HY29F002TT-90",0x40000 },
	{ 0xad, 0xd5, "Hynix - HY29F080",0x100000 },
	{ 0xc2, 0x36, "Macronix - MX29F022NTPC",0x40000 },
	{ 0x20, 0xb0, "ST - M29F002BT",0x40000 },
	{ 0x20, 0xf1, "ST - M29F080A",0x100000 },
	{ 0xbf, 0x61, "SST SST49LF020",0x40000 },
	{ 0x89, 0xa6, "Sharp LHF08CH1",0x100000 },
	{ 0xda, 0x8c, "Winbond W49F020",0x40000 },
	{ 0xda, 0x0b, "Winbond - W49F002U",0x40000 },
	{ 0, 0, "", 0 } // terminator
};

 // callback to show progress
bool BootFlashUserInterface(void * pvoidObjectFlash, ENUM_EVENTS ee, DWORD dwPos, DWORD dwExtent) {
	if(ee==EE_ERASE_UPDATE){
		DisplayFlashProgressBar(dwPos,dwExtent,0xffffff00);
	}
	else if(ee==EE_PROGRAM_UPDATE){
		DisplayFlashProgressBar(dwPos,dwExtent,0xff00ff00);
	}
	return true;
}

void DisplayFlashProgressBar(int currentVal, int maxVal, unsigned long color) {
        int x,y,l,w,h,m;
        DWORD *fb=(DWORD*)FB_START;

        if(maxVal<2){
                return;
        }

        w=vmode.height;
        h=vmode.width;
        l=w-100;
        y=h-100;
        m=((w-150)>>2)*currentVal/maxVal;
        m*=4;
        m+=50;
        for(x=50;x<l;x++){
                fb[y*w+x]=0xffffffff;
                fb[(y+1)*w+x]=0xffffffff;
                if(x>55 && x<m){
                        int z;
                        for(z=5;z<45;z++){
                                fb[(y+z)*w+x]=color;
                        }

                }
                fb[(y+50)*w+x]=0xffffffff;
                fb[(y+51)*w+x]=0xffffffff;
        }
        for(;y<h-50;y++){
                fb[y*w+51]=0xffffffff;
                fb[y*w+50]=0xffffffff;
                fb[y*w+l-1]=0xffffffff;
                fb[y*w+l-2]=0xffffffff;
        }
}
	// if things go well, we won't be coming back from this
	// note this is copied to RAM, and the flash will be changed during its operation
	// therefore no library code nor interrupts can be had

int BootReflashAndReset(BYTE *pbNewData, DWORD dwStartOffset, DWORD dwLength)
{
	OBJECT_FLASH of;
	bool fMore=true;

	// prep our flash object with start address and params
	of.m_pbMemoryMappedStartAddress=(BYTE *)LPCFlashadress;
	of.m_dwStartOffset=dwStartOffset;
	of.m_dwLengthUsedArea=dwLength;
	of.m_pcallbackFlash=BootFlashUserInterface;

	// check device type and parameters are sane
	if(!BootFlashGetDescriptor(&of, (KNOWN_FLASH_TYPE *)&aknownflashtypesDefault[0])) return 1; // unable to ID device - fail
	if(!of.m_fIsBelievedCapableOfWriteAndErase) return 2; // seems to be write-protected - fail
	if(of.m_dwLengthInBytes<(dwStartOffset+dwLength)) return 3; // requested layout won't fit device - sanity check fail
	
	// committed to reflash now
	while(fMore) {
		if(BootFlashEraseMinimalRegion(&of)) {
			if(BootFlashProgram(&of, pbNewData)) {
				fMore=false;  // good situation
			} else { // failed program
				;
			}
		} else { // failed erase
			;
		}
	}
	return 0; // keep compiler happy
}
