/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    hooks.h

Abstract:

    HEADER for hooks.c

Environment:

    LINUX 2.2.X
    Kernel mode only

Author: 

    Klaus P. Gerlicher

Revision History:

    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/
void DeinstallHooks(void);	
//ULONG HookInt(ULONG dwInt,ULONG NewIntHandler);
//void UnhookInt(ULONG dwInt);
void MaskIrqs(void);
void UnmaskIrqs(void);
ULONG SetGlobalInt(ULONG dwInt,ULONG NewIntHandler);
ULONG GetIRQVector(ULONG dwInt);
void TakeIdtSnapshot(void);
void RestoreIdt(void);

// structure of an IDT entry
typedef struct IdtEntry
{
	USHORT LoOffset;
	USHORT SegSel;
	USHORT Flags;
	USHORT HiOffset;
}IDTENTRY,*PIDTENTRY;

