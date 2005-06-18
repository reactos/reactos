/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/ketypes.h
 * PURPOSE:         Definitions for Kernel Types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _KETYPES_H
#define _KETYPES_H

/* DEPENDENCIES **************************************************************/
#include "haltypes.h"

/* EXPORTED DATA *************************************************************/
extern CHAR NTOSAPI KeNumberProcessors;
extern LOADER_PARAMETER_BLOCK NTOSAPI KeLoaderBlock;
extern ULONG NTOSAPI KeDcacheFlushCount;
extern ULONG NTOSAPI KeIcacheFlushCount;
extern KAFFINITY NTOSAPI KeActiveProcessors;
extern ULONG NTOSAPI KiDmaIoCoherency; /* RISC Architectures only */
extern ULONG NTOSAPI KeMaximumIncrement;
extern ULONG NTOSAPI KeMinimumIncrement;
extern SSDT_ENTRY NTOSAPI KeServiceDescriptorTable[SSDT_MAX_ENTRIES];
extern SSDT_ENTRY NTOSAPI KeServiceDescriptorTableShadow[SSDT_MAX_ENTRIES];

/* CONSTANTS *****************************************************************/
#define SSDT_MAX_ENTRIES 4
#define PROCESSOR_FEATURE_MAX 64

/* ENUMERATIONS **************************************************************/

/* TYPES *********************************************************************/

typedef enum _KAPC_ENVIRONMENT 
{
    OriginalApcEnvironment,
    AttachedApcEnvironment,
    CurrentApcEnvironment
} KAPC_ENVIRONMENT;

typedef struct _KDPC_DATA 
{
    LIST_ENTRY  DpcListHead;
    ULONG  DpcLock;
    ULONG  DpcQueueDepth;
    ULONG  DpcCount;
} KDPC_DATA, *PKDPC_DATA;

typedef struct _KTRAP_FRAME 
{
    PVOID DebugEbp;
    PVOID DebugEip;
    PVOID DebugArgMark;
    PVOID DebugPointer;
    PVOID TempCs;
    PVOID TempEip;
    ULONG Dr0;
    ULONG Dr1;
    ULONG Dr2;
    ULONG Dr3;
    ULONG Dr6;
    ULONG Dr7;
    USHORT Gs;
    USHORT Reserved1;
    USHORT Es;
    USHORT Reserved2;
    USHORT Ds;
    USHORT Reserved3;
    ULONG Edx;
    ULONG Ecx;
    ULONG Eax;
    ULONG PreviousMode;
    PVOID ExceptionList;
    USHORT Fs;
    USHORT Reserved4;
    ULONG Edi;
    ULONG Esi;
    ULONG Ebx;
    ULONG Ebp;
    ULONG ErrorCode;
    ULONG Eip;
    ULONG Cs;
    ULONG Eflags;
    ULONG Esp;
    USHORT Ss;
    USHORT Reserved5;
    USHORT V86_Es;
    USHORT Reserved6;
    USHORT V86_Ds;
    USHORT Reserved7;
    USHORT V86_Fs;
    USHORT Reserved8;
    USHORT V86_Gs;
    USHORT Reserved9;
} KTRAP_FRAME, *PKTRAP_FRAME;

/* i386 Doesn't have Exception Frames */
typedef struct _KEXCEPTION_FRAME {

} KEXCEPTION_FRAME, *PKEXCEPTION_FRAME;

typedef struct _KINTERRUPT 
{
    CSHORT              Type;
    CSHORT              Size;
    LIST_ENTRY          InterruptListEntry;
    PKSERVICE_ROUTINE   ServiceRoutine;
    PVOID               ServiceContext;
    KSPIN_LOCK          SpinLock;
    ULONG               TickCount;
    PKSPIN_LOCK         ActualLock;
    PVOID               DispatchAddress;
    ULONG               Vector;
    KIRQL               Irql;
    KIRQL               SynchronizeIrql;
    BOOLEAN             FloatingSave;
    BOOLEAN             Connected;
    CHAR                Number;
    UCHAR               ShareVector;
    KINTERRUPT_MODE     Mode;
    ULONG               ServiceCount;
    ULONG               DispatchCount;
    ULONG               DispatchCode[106];
} KINTERRUPT, *PKINTERRUPT;

/* FIXME: Add KOBJECTS Here */

#endif
