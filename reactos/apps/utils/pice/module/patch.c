/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    patch.c

Abstract:

    hooking of kernel internal keyboard interrupt handler

Environment:

    Kernel mode only

Author:

    Klaus P. Gerlicher

Revision History:

    10-Jul-1999:	created
    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/

////////////////////////////////////////////////////
// INCLUDES
////
#include "remods.h"
#include "precomp.h"

#include <asm/system.h>

////////////////////////////////////////////////////
// GLOBALS
////

static PUCHAR pPatchAddress;
static ULONG ulOldOffset = 0;
static ULONG ulKeyPatchFlags;
BOOLEAN bPatched = FALSE;
void (*old_handle_scancode)(UCHAR,int);
char tempPatch[256];
UCHAR ucBreakKey = 'D'; // key that will break into debugger in combination with CTRL

////////////////////////////////////////////////////
// FUNCTIONS
////

// the keyboard hook
void pice_handle_scancode(UCHAR scancode,int bKeyPressed)
{
	UCHAR ucKey = scancode & 0x7f;
	static BOOLEAN bControl = FALSE;
	BOOLEAN bForward=TRUE;

    ENTER_FUNC();

    DPRINT((0,"pice_handle_scancode(%x,%u)\n",scancode,bKeyPressed));
    DPRINT((0,"pice_handle_scancode(1): bControl = %u bForward = %u bEnterNow = %u\n",bControl,bForward,bEnterNow));
	if(bKeyPressed)
	{
        // CTRL pressed
		if(ucKey==0x1d)
		{
			bControl=TRUE;
		}
		else if(bControl==TRUE && ucKey==AsciiToScan(ucBreakKey)) // CTRL-F
		{
            // fake a CTRL-F release call
    	    old_handle_scancode(0x1d|0x80,FALSE);
    	    old_handle_scancode(AsciiToScan(ucBreakKey)|0x80,FALSE);
			bForward=FALSE;
            bEnterNow=TRUE;
			bControl=FALSE;
		}
        else if((ucKey == 66|| ucKey == 68) && bStepping)
        {
			bForward=FALSE;
        }

	}
	else
	{
        // CTRL released
		if(ucKey==0x1d)
		{
			bControl=FALSE;
		}
        else if((ucKey == 66|| ucKey == 68) && bStepping)
        {
			bForward=FALSE;
        }
    }

    if(bForward)
    {
        DPRINT((0,"pice_handle_scancode(): forwarding key stroke\n"));
	    old_handle_scancode(scancode,bKeyPressed);
    }

    LEAVE_FUNC();
}

BOOLEAN PatchKeyboardDriver(ULONG AddrOfKbdEvent,ULONG AddrOfScancode)
{
	UCHAR ucPattern[5] = {0xE8,0x0,0x0,0x0,0x0};
	PULONG pOffset = (PULONG)&ucPattern[1];
	ULONG ulOffset,countBytes = 0;

    ENTER_FUNC();

	(void*)old_handle_scancode = AddrOfScancode;
    DPRINT((0,"handle_scancode = %X\n",AddrOfScancode));

	pPatchAddress = (PUCHAR)AddrOfKbdEvent; // handle_kbd_event
    DPRINT((0,"initial patch address = %X\n",AddrOfKbdEvent));
    ulOffset = (ULONG)old_handle_scancode - ((ULONG)pPatchAddress+sizeof(ULONG)+1);
    DPRINT((0,"initial offset = %X\n",ulOffset));
	*pOffset = ulOffset;

	while((memcmp(pPatchAddress,ucPattern,sizeof(ucPattern))!=0) &&
	      (countBytes<0x1000))
	{
/*        DPRINT((0,"offset = %X\n",ulOffset));
        DPRINT((0,"patch address = %X\n",pPatchAddress));
        DPRINT((0,"pattern1 = %.2X %.2X %.2X %.2X %.2X\n",ucPattern[0],ucPattern[1],ucPattern[2],ucPattern[3],ucPattern[4]));
        DPRINT((0,"pattern2 = %.2X %.2X %.2X %.2X %.2X\n",pPatchAddress[0],pPatchAddress[1],pPatchAddress[2],pPatchAddress[3],pPatchAddress[4]));*/

		countBytes++;
		pPatchAddress++;

		ulOffset = (ULONG)old_handle_scancode - ((ULONG)pPatchAddress+sizeof(ULONG)+1);
		*pOffset = ulOffset;
	}
	
	if(memcmp(pPatchAddress,ucPattern,sizeof(ucPattern))==0)
	{
		DPRINT((0,"pattern found @ %x\n",pPatchAddress));
		
		ulOffset = (ULONG)&pice_handle_scancode - ((ULONG)pPatchAddress+sizeof(ULONG)+1);
		ulOldOffset = *(PULONG)(pPatchAddress + 1);
		DPRINT((0,"old offset = %x new offset = %x\n",ulOldOffset,ulOffset));

		save_flags(ulKeyPatchFlags);
		cli();
		*(PULONG)(pPatchAddress + 1) = ulOffset;

		bPatched = TRUE;

		restore_flags(ulKeyPatchFlags);
		DPRINT((0,"PatchKeyboardDriver(): SUCCESS!\n"));
	}

    LEAVE_FUNC();

    return bPatched;
}

void RestoreKeyboardDriver(void)
{
    ENTER_FUNC();
	if(bPatched)
	{
		save_flags(ulKeyPatchFlags);
		cli();
		*(PULONG)(pPatchAddress + 1) = ulOldOffset;
		restore_flags(ulKeyPatchFlags);
	}
    LEAVE_FUNC();
} 
