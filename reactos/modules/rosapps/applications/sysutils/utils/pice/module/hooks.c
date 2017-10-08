/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    hooks.c

Abstract:

    hooking of interrupts

Environment:

    Kernel mode only

Author:

    Klaus P. Gerlicher

Revision History:

    16-Jul-1998:	created
    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/

////////////////////////////////////////////////////
// INCLUDES
////
#include "remods.h"
#include "precomp.h"

////////////////////////////////////////////////////
// PROTOTYPES
////
void DeinstallHooks(void);

////////////////////////////////////////////////////
// DEFINES
////

////////////////////////////////////////////////////
// GLOBALS
////

// IDT entries
//PIDTENTRY pidt[256];
IDTENTRY oldidt[256]={{0},};

IDTENTRY idt_snapshot[256]={{0},};

// processor flag for interrupt suspension
ULONG ulOldFlags;

////////////////////////////////////////////////////
// PROCEDURES
////

//*************************************************************************
// MaskIrqs()
//
//*************************************************************************
void MaskIrqs(void)
{
    ENTER_FUNC();

    save_flags(ulOldFlags);
    cli();

    LEAVE_FUNC();
}

//*************************************************************************
// UnmaskIrqs()
//
//*************************************************************************
void UnmaskIrqs(void)
{
    ENTER_FUNC();

    restore_flags(ulOldFlags);

    LEAVE_FUNC();
}

//*************************************************************************
// SetGlobalInt()
//
//*************************************************************************
ULONG SetGlobalInt(ULONG dwInt,ULONG NewIntHandler)
{
    ULONG idt[2];
    ULONG OldIntHandler;
    struct IdtEntry* pidt;
    struct IdtEntry oldidt;

    ENTER_FUNC();

	// get linear location of IDT
	__asm__("sidt %0":"=m" (idt));

	// get pointer to idte for int 3
	pidt=((struct IdtEntry*)((idt[1]<<16)|((idt[0]>>16)&0x0000FFFF)))+dwInt;

	oldidt=*pidt;

    // set new handler address
	pidt->HiOffset=(USHORT)(((ULONG)NewIntHandler)>>16);
	pidt->LoOffset=(USHORT)(((ULONG)NewIntHandler)&0x0000FFFF);

    DPRINT((0,"new INT(%0.2x) handler = %0.4x:%x\n",dwInt,pidt->SegSel,(pidt->HiOffset<<16)|(pidt->LoOffset&0x0000FFFF)));

	OldIntHandler=(oldidt.HiOffset<<16)|(oldidt.LoOffset&0x0000FFFF);

    DPRINT((0,"old INT(%0.2x) handler = %0.4x:%x\n",dwInt,pidt->SegSel,OldIntHandler));

    LEAVE_FUNC();

	return OldIntHandler;
}

//*************************************************************************
// TakeIdtSnapshot()
//
//*************************************************************************
void TakeIdtSnapshot(void)
{
    ULONG idt[2],i;
    struct IdtEntry* pidt;

	__asm__("sidt %0":"=m" (idt));

	// get pointer to idte for int 3
	pidt=((struct IdtEntry*)((idt[1]<<16)|((idt[0]>>16)&0x0000FFFF)));

    for(i=0;i<256;i++)
    {
        DPRINT((11,"TakeIdtSnapShot(): saving vector %u\n",i));
        if(IsRangeValid((ULONG)pidt,sizeof(*pidt)) )
        {
            DPRINT((11,"TakeIdtSnapShot(): vector %u valid\n",i));
    	    idt_snapshot[i] = *pidt++;
        }
    }
}

//*************************************************************************
// RestoreIdt()
//
//*************************************************************************
void RestoreIdt(void)
{
    ULONG idt[2],i;
    struct IdtEntry* pidt;

	__asm__("sidt %0":"=m" (idt));

    // get pointer to idte for int 3
	pidt=((struct IdtEntry*)((idt[1]<<16)|((idt[0]>>16)&0x0000FFFF)));

    for(i=0;i<256;i++)
    {
        DPRINT((11,"TakeIdtSnapShot(): restoring vector %u\n",i));
        if(IsRangeValid((ULONG)pidt,sizeof(*pidt)) )
        {
            DPRINT((11,"TakeIdtSnapShot(): vector %u valid\n",i));
    	    *pidt++ = idt_snapshot[i];
        }
    }
}

// EOF
