/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "boot.h"
#include "BootFlash.h"
#include <stdio.h>

// gets device ID, sets pof up accordingly
// returns true if device okay or false for unrecognized device

bool BootFlashGetDescriptor( OBJECT_FLASH *pof, KNOWN_FLASH_TYPE * pkft )
{
	bool fSeen=false;
	BYTE baNormalModeFirstTwoBytes[2];
	int nTries=0;
	int nPos=0;

	pof->m_fIsBelievedCapableOfWriteAndErase=true;
	pof->m_szAdditionalErrorInfo[0]='\0';

	baNormalModeFirstTwoBytes[0]=pof->m_pbMemoryMappedStartAddress[0];
	baNormalModeFirstTwoBytes[1]=pof->m_pbMemoryMappedStartAddress[1];

	while(nTries++ <2) { // first we try 29xxx method, then 28xxx if that failed

	
		if(nTries!=1) { // 29xxx protocol

			// make sure the flash state machine is reset

			pof->m_pbMemoryMappedStartAddress[0x5555]=0xf0;
			pof->m_pbMemoryMappedStartAddress[0x5555]=0xaa;
			pof->m_pbMemoryMappedStartAddress[0x2aaa]=0x55;
			pof->m_pbMemoryMappedStartAddress[0x5555]=0xf0;

				// read flash ID

			pof->m_pbMemoryMappedStartAddress[0x5555]=0xaa;
			pof->m_pbMemoryMappedStartAddress[0x2aaa]=0x55;
			pof->m_pbMemoryMappedStartAddress[0x5555]=0x90;

			pof->m_bManufacturerId=pof->m_pbMemoryMappedStartAddress[0];
			pof->m_bDeviceId=pof->m_pbMemoryMappedStartAddress[1];

			pof->m_pbMemoryMappedStartAddress[0x5555]=0xf0;

			pof->m_fDetectedUsing28xxxConventions=false; // mark the flash object as representing a 28xxx job


		} else { // 28xxx protocol, seen on Sharp

				// make sure the flash state machine is reset

			pof->m_pbMemoryMappedStartAddress[0x5555]=0xff;

				// read flash ID

			pof->m_pbMemoryMappedStartAddress[0x5555]=0x90;
			pof->m_bManufacturerId=pof->m_pbMemoryMappedStartAddress[0];

			pof->m_pbMemoryMappedStartAddress[0x5555]=0x90;
			pof->m_bDeviceId=pof->m_pbMemoryMappedStartAddress[1];

			pof->m_pbMemoryMappedStartAddress[0x5555]=0xff;

			pof->m_fDetectedUsing28xxxConventions=true; // mark the flash object as representing a 28xxx job

		}




		if(
			(baNormalModeFirstTwoBytes[0]!=pof->m_bManufacturerId) ||
			(baNormalModeFirstTwoBytes[1]!=pof->m_pbMemoryMappedStartAddress[1])
		) nTries=2;  // don't try any more if we got some result the first time

	} // while
		// interpret device ID info

	{
		bool fMore=true;
		while(fMore) {
			if(!pkft->m_bManufacturerId) {
				fMore=false; continue;
			}
			if((pkft->m_bManufacturerId == pof->m_bManufacturerId) &&
				(pkft->m_bDeviceId == pof->m_bDeviceId)
			) {
				fSeen=true;
				fMore=false;
				nPos+=sprintf(&pof->m_szFlashDescription[nPos], "%s (%dK)", pkft->m_szFlashDescription, pkft->m_dwLengthInBytes/1024);
				pof->m_dwLengthInBytes = pkft->m_dwLengthInBytes;

				if(pof->m_fDetectedUsing28xxxConventions) {
					int n=0;
						// detect master lock situation

					pof->m_pbMemoryMappedStartAddress[0x5555]=0x90;
					if(pof->m_pbMemoryMappedStartAddress[3]!=0) { // master lock bit is set, no erases or writes are going to happen
						pof->m_fIsBelievedCapableOfWriteAndErase=false;
						nPos+=sprintf(&pof->m_szFlashDescription[nPos], "Master Lock SET  ");
					}

						// detect block lock situation

					nPos+=sprintf(&pof->m_szFlashDescription[nPos], "Block Locks: ");
					while(n<pof->m_dwLengthInBytes) {
						pof->m_pbMemoryMappedStartAddress[0x5555]=0x90;
						nPos+=sprintf(&pof->m_szFlashDescription[nPos], "%u", pof->m_pbMemoryMappedStartAddress[n|0x0002]&1);
						n+=0x10000;
					}
					nPos+=sprintf(&pof->m_szFlashDescription[nPos], "  ");
					pof->m_pbMemoryMappedStartAddress[0x5555]=0x50;
					pof->m_pbMemoryMappedStartAddress[0x5555]=0xff;

				}
			}
			pkft++;
		}
	}


	if(!fSeen) {
		if(
			(baNormalModeFirstTwoBytes[0]==pof->m_bManufacturerId) &&
			(baNormalModeFirstTwoBytes[1]==pof->m_pbMemoryMappedStartAddress[1])
		) { // we didn't get anything worth reporting
			sprintf(pof->m_szFlashDescription, "Read Only??? manf=0x%02X, dev=0x%02X", pof->m_bManufacturerId, pof->m_bDeviceId);
		} else { // we got what is probably an unknown flash type
			sprintf(pof->m_szFlashDescription, "manf=0x%02X, dev=0x%02X", pof->m_bManufacturerId, pof->m_bDeviceId);
		}
	}

	return fSeen;
}

 // uses the block erase function on the flash to erase the minimal footprint
 // needed to cover pof->m_dwStartOffset .. (pof->m_dwStartOffset+pof->m_dwLengthUsedArea)

 #define MAX_ERASE_RETRIES_IN_4KBLOCK_BEFORE_FAILING 4
 
bool BootFlashEraseMinimalRegion( OBJECT_FLASH *pof )
{
	DWORD dw=pof->m_dwStartOffset;
	DWORD dwLen=pof->m_dwLengthUsedArea;
	DWORD dwLastEraseAddress=0xffffffff;
	int nCountEraseRetryIn4KBlock=MAX_ERASE_RETRIES_IN_4KBLOCK_BEFORE_FAILING;

	pof->m_szAdditionalErrorInfo[0]='\0';

	if(pof->m_pcallbackFlash!=NULL)
		if(!(pof->m_pcallbackFlash)(pof, EE_ERASE_START, 0, 0)) {
			strcpy(pof->m_szAdditionalErrorInfo, "Erase Aborted");
			return false;
		}

	while(dwLen) {

		if(pof->m_pbMemoryMappedStartAddress[dw]!=0xff) { // something needs erasing

			BYTE b;

			if((dwLastEraseAddress & 0xfffff000)==(dw & 0xfffff000)) { // same 4K block?
				nCountEraseRetryIn4KBlock--;
				if(nCountEraseRetryIn4KBlock==0) { // run out of tries in this 4K block
					if(pof->m_pcallbackFlash!=NULL) {
						(pof->m_pcallbackFlash)(pof, EE_ERASE_ERROR, dw-pof->m_dwStartOffset, pof->m_pbMemoryMappedStartAddress[dw]);
						(pof->m_pcallbackFlash)(pof, EE_ERASE_END, 0, 0);
					}
					sprintf(pof->m_szAdditionalErrorInfo, "Erase failed for block at +0x%x, reads as 0x%02X", dw, pof->m_pbMemoryMappedStartAddress[dw]);
					return false; // failure
				}
			} else {
				nCountEraseRetryIn4KBlock=MAX_ERASE_RETRIES_IN_4KBLOCK_BEFORE_FAILING;  // different block, reset retry counter
				dwLastEraseAddress=dw;
			}

			if(pof->m_fDetectedUsing28xxxConventions) {
				int nCountMinSpin=0x100;
				b=0x00;
				pof->m_pbMemoryMappedStartAddress[0x5555]=0x50; // clear status register
					// erase the block containing the non 0xff guy
				pof->m_pbMemoryMappedStartAddress[dw]=0x20;
				pof->m_pbMemoryMappedStartAddress[dw]=0xd0;

				while((!(b&0x80)) || (nCountMinSpin)) { // busy - Sharp has a problem, does not go busy for ~500nS
					b=pof->m_pbMemoryMappedStartAddress[dw];
					if(nCountMinSpin) nCountMinSpin--;
				}
				pof->m_pbMemoryMappedStartAddress[0x5555]=0x50;
				pof->m_pbMemoryMappedStartAddress[0x5555]=0xff;
				if(b&0x7e) { // uh-oh something wrong
					if(pof->m_pcallbackFlash!=NULL) {
						(pof->m_pcallbackFlash)(pof, EE_ERASE_ERROR, dw-pof->m_dwStartOffset, pof->m_pbMemoryMappedStartAddress[dw]);
						(pof->m_pcallbackFlash)(pof, EE_ERASE_END, 0, 0);
					}
					if(b&8) {
						sprintf(pof->m_szAdditionalErrorInfo, "This chip requires +5V on pin 11 (Vpp).  See the README.");
					} else {
						sprintf(pof->m_szAdditionalErrorInfo, "Chip Status after Erase: 0x%02X", b);
					}
					return false;
				}
			} else { // more common 29xxx style
				DWORD dwCountTries=0;

				pof->m_pbMemoryMappedStartAddress[0x5555]=0xaa;
				pof->m_pbMemoryMappedStartAddress[0x2aaa]=0x55;
				pof->m_pbMemoryMappedStartAddress[0x5555]=0x80;

				pof->m_pbMemoryMappedStartAddress[0x5555]=0xaa;
				pof->m_pbMemoryMappedStartAddress[0x2aaa]=0x55;
				pof->m_pbMemoryMappedStartAddress[dw]=0x50; // erase the block containing the non 0xff guy

				b=pof->m_pbMemoryMappedStartAddress[dw];  // waits until b6 is no longer toggling on each read
				while((pof->m_pbMemoryMappedStartAddress[dw]&0x40)!=(b&0x40)) {
					dwCountTries++; b^=0x40;
				}

				if(dwCountTries<3) { // <3 means never entered busy mode - block erase code 0x50 not supported, try alternate
					pof->m_pbMemoryMappedStartAddress[0x5555]=0xaa;
					pof->m_pbMemoryMappedStartAddress[0x2aaa]=0x55;
					pof->m_pbMemoryMappedStartAddress[0x5555]=0xf0;

					pof->m_pbMemoryMappedStartAddress[0x5555]=0xaa;
					pof->m_pbMemoryMappedStartAddress[0x2aaa]=0x55;
					pof->m_pbMemoryMappedStartAddress[0x5555]=0x80;

					pof->m_pbMemoryMappedStartAddress[0x5555]=0xaa;
					pof->m_pbMemoryMappedStartAddress[0x2aaa]=0x55;
					pof->m_pbMemoryMappedStartAddress[dw]=0x30; // erase the block containing the non 0xff guy

					b=pof->m_pbMemoryMappedStartAddress[dw];
					dwCountTries=0;
					while((pof->m_pbMemoryMappedStartAddress[dw]&0x40)!=(b&0x40)) {
						dwCountTries++; b^=0x40;
					}
				}

					// if we had a couple of unsuccessful tries at block erase already, try chip erase				
					// safety features...
				if(
					(dwCountTries<3) &&  // other commands did not work at all
					(nCountEraseRetryIn4KBlock<2) &&  // had multiple attempts at them already
					(pof->m_dwStartOffset==0) && // reprogramming whole chip .. starting from start
					(pof->m_dwLengthUsedArea == pof->m_dwLengthInBytes)  // and doing the whole length of the chip
				) { // <3 means never entered busy mode - block erase code 0x30 not supported
					#if 1
					printk("Trying to erase whole chip\n");
					#endif
					pof->m_pbMemoryMappedStartAddress[0x5555]=0xaa;
					pof->m_pbMemoryMappedStartAddress[0x2aaa]=0x55;
					pof->m_pbMemoryMappedStartAddress[0x5555]=0xf0;

					pof->m_pbMemoryMappedStartAddress[0x5555]=0xaa;
					pof->m_pbMemoryMappedStartAddress[0x2aaa]=0x55;
					pof->m_pbMemoryMappedStartAddress[0x5555]=0x80;

					pof->m_pbMemoryMappedStartAddress[0x5555]=0xaa;
					pof->m_pbMemoryMappedStartAddress[0x2aaa]=0x55;
					pof->m_pbMemoryMappedStartAddress[0x5555]=0x10; // chip erase ONLY available on W49F020

					b=pof->m_pbMemoryMappedStartAddress[dw];
					dwCountTries=0;
					while((pof->m_pbMemoryMappedStartAddress[dw]&0x40)!=(b&0x40)) {
						dwCountTries++; b^=0x40;
					}
				}

			}


			continue; // retry reading this address without moving on
		}

			// update callback every 1K addresses
		if((dw&0x3ff)==0) {
			if(pof->m_pcallbackFlash!=NULL) {
				if(!(pof->m_pcallbackFlash)(pof, EE_ERASE_UPDATE, dw-pof->m_dwStartOffset, pof->m_dwLengthUsedArea)) {
					strcpy(pof->m_szAdditionalErrorInfo, "Erase Aborted");
					return false;
				}
			}
		}

		dwLen--; dw++;
	}

	if(pof->m_pcallbackFlash!=NULL) if(!(pof->m_pcallbackFlash)(pof, EE_ERASE_END, 0, 0)) return false;

	return true;
}

	// program the flash from the data in pba
	// length of valid data in pba held in pof->m_dwLengthUsedArea

bool BootFlashProgram( OBJECT_FLASH *pof, BYTE *pba )
{
	DWORD dw=pof->m_dwStartOffset;
	DWORD dwLen=pof->m_dwLengthUsedArea;
	DWORD dwSrc=0;
	DWORD dwLastProgramAddress=0xffffffff;
	int nCountProgramRetries=4;

	pof->m_szAdditionalErrorInfo[0]='\0';
	if(pof->m_pcallbackFlash!=NULL)
		if(!(pof->m_pcallbackFlash)(pof, EE_PROGRAM_START, 0, 0)) {
			strcpy(pof->m_szAdditionalErrorInfo, "Program Aborted");
			return false;
		}

		// program

	while(dwLen) {

		if(pof->m_pbMemoryMappedStartAddress[dw]!=pba[dwSrc]) { // needs programming

			if(dwLastProgramAddress==dw) {
				nCountProgramRetries--;
				if(nCountProgramRetries==0) {
					if(pof->m_pcallbackFlash!=NULL) {
						(pof->m_pcallbackFlash)(pof, EE_PROGRAM_ERROR, dw, (((DWORD)pba[dwSrc])<<8) |pof->m_pbMemoryMappedStartAddress[dw] );
						(pof->m_pcallbackFlash)(pof, EE_PROGRAM_END, 0, 0);
					}
					sprintf(pof->m_szAdditionalErrorInfo, "Program failed for byte at +0x%x; wrote 0x%02X, read 0x%02X", dw, pba[dwSrc], pof->m_pbMemoryMappedStartAddress[dw]);
					return false;
				}
			} else {
				nCountProgramRetries=4;
				dwLastProgramAddress=dw;
			}


			if(pof->m_fDetectedUsing28xxxConventions) {
				BYTE b=0x0;
				DWORD dwTimeToLive=0xfffff;  // 1M times around, a few mS
				int nCountMinSpin=2; // force wait for this long, suspect busy is not coming up immediately
				pof->m_pbMemoryMappedStartAddress[dw]=0x40;
				pof->m_pbMemoryMappedStartAddress[dw]=pba[dwSrc]; // perform programming action
				while(((!(b&0x80)) && (--dwTimeToLive)) || (nCountMinSpin)) { // busy - Sharp has a problem, does not go busy for ~500nS
					b=pof->m_pbMemoryMappedStartAddress[dw];
					if(nCountMinSpin) nCountMinSpin--;
				}
				pof->m_pbMemoryMappedStartAddress[dw]=0x50;
				pof->m_pbMemoryMappedStartAddress[dw]=0xff;
				if((b&0x7e)||(!dwTimeToLive)) { // uh-oh something wrong
					if(pof->m_pcallbackFlash!=NULL) {
						(pof->m_pcallbackFlash)(pof, EE_PROGRAM_ERROR, dw-pof->m_dwStartOffset, (((DWORD)pba[dwSrc])<<8) | pof->m_pbMemoryMappedStartAddress[dw]);
						(pof->m_pcallbackFlash)(pof, EE_PROGRAM_END, 0, 0);
					}
					if(dwTimeToLive) {
						sprintf(pof->m_szAdditionalErrorInfo, "Chip Status after Program: 0x%02X", b);
					} else {
						sprintf(pof->m_szAdditionalErrorInfo, "Chip Status after TIMED-OUT Program: 0x%02X", b);
					}
					if(b&8) {
						sprintf(pof->m_szAdditionalErrorInfo, "This chip requires +5V on pin 11 (Vpp).  See the README.");
					}
					return false;
				}
			} else {
				BYTE b;
				pof->m_pbMemoryMappedStartAddress[0x5555]=0xaa;
				pof->m_pbMemoryMappedStartAddress[0x2aaa]=0x55;
				pof->m_pbMemoryMappedStartAddress[0x5555]=0xa0;
				pof->m_pbMemoryMappedStartAddress[dw]=pba[dwSrc]; // perform programming action
				b=pof->m_pbMemoryMappedStartAddress[dw];  // waits until b6 is no longer toggling on each read
				while((pof->m_pbMemoryMappedStartAddress[dw]&0x40)!=(b&0x40)) b^=0x40;
			}


			continue;  // does NOT advance yet
		}

		if((dw&0x3ff)==0)
			if(pof->m_pcallbackFlash!=NULL)
				if(!(pof->m_pcallbackFlash)(pof, EE_PROGRAM_UPDATE, dwSrc, pof->m_dwLengthUsedArea)) {
					strcpy(pof->m_szAdditionalErrorInfo, "Program Aborted");
					return false;
				}

		dwLen--; dw++; dwSrc++;
	}

	if(pof->m_pcallbackFlash!=NULL) if(!(pof->m_pcallbackFlash)(pof, EE_PROGRAM_END, 0, 0)) return false;

		// verify

	if(pof->m_pcallbackFlash!=NULL) if(!(pof->m_pcallbackFlash)(pof, EE_VERIFY_START, 0, 0)) return false;

	dw=pof->m_dwStartOffset;
	dwLen=pof->m_dwLengthUsedArea;
	dwSrc=0;

	while(dwLen) {

		if(pof->m_pbMemoryMappedStartAddress[dw]!=pba[dwSrc]) { // verify error
			if(pof->m_pcallbackFlash!=NULL) if(!(pof->m_pcallbackFlash)(pof, EE_VERIFY_ERROR, dw, (((DWORD)pba[dwSrc])<<8) |pof->m_pbMemoryMappedStartAddress[dw])) return false;
			if(pof->m_pcallbackFlash!=NULL) if(!(pof->m_pcallbackFlash)(pof, EE_VERIFY_END, 0, 0)) return false;
			return false;
		}

		dwLen--; dw++; dwSrc++;
	}

	if(pof->m_pcallbackFlash!=NULL) if(!(pof->m_pcallbackFlash)(pof, EE_VERIFY_END, 0, 0)) return false;
	return true;
}
