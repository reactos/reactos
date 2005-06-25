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
#include <arc/arc.h>

/* CONSTANTS *****************************************************************/
#define SSDT_MAX_ENTRIES 4
#define PROCESSOR_FEATURE_MAX 64

#define CONTEXT_DEBUGGER (CONTEXT_FULL | CONTEXT_FLOATING_POINT)

#define THREAD_WAIT_OBJECTS 4

/* FIXME: Create an ASM Offset File */
#define KTSS_ESP0      (0x4)
#define KTSS_CR3       (0x1C)
#define KTSS_EFLAGS    (0x24)
#define KTSS_IOMAPBASE (0x66)

/* EXPORTED DATA *************************************************************/
extern CHAR NTOSAPI KeNumberProcessors;
extern LOADER_PARAMETER_BLOCK NTOSAPI KeLoaderBlock;
extern ULONG NTOSAPI KeDcacheFlushCount;
extern ULONG NTOSAPI KeIcacheFlushCount;
extern KAFFINITY NTOSAPI KeActiveProcessors;
extern ULONG NTOSAPI KiDmaIoCoherency; /* RISC Architectures only */
extern ULONG NTOSAPI KeMaximumIncrement;
extern ULONG NTOSAPI KeMinimumIncrement;
extern ULONG NTOSAPI NtBuildNumber;
extern SSDT_ENTRY NTOSAPI KeServiceDescriptorTable[SSDT_MAX_ENTRIES];
extern SSDT_ENTRY NTOSAPI KeServiceDescriptorTableShadow[SSDT_MAX_ENTRIES];

/* ENUMERATIONS **************************************************************/

/* TYPES *********************************************************************/

typedef struct _CONFIGURATION_COMPONENT_DATA 
{
  struct _CONFIGURATION_COMPONENT_DATA *Parent;
  struct _CONFIGURATION_COMPONENT_DATA *Child;
  struct _CONFIGURATION_COMPONENT_DATA *Sibling;
  CONFIGURATION_COMPONENT Component;
} CONFIGURATION_COMPONENT_DATA, *PCONFIGURATION_COMPONENT_DATA;


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

typedef struct _LDT_ENTRY {
  WORD LimitLow;
  WORD BaseLow;
  union {
    struct {
      BYTE BaseMid;
      BYTE Flags1;
      BYTE Flags2;
      BYTE BaseHi;
    } Bytes;
    struct {
      DWORD BaseMid : 8;
      DWORD Type : 5;
      DWORD Dpl : 2;
      DWORD Pres : 1;
      DWORD LimitHi : 4;
      DWORD Sys : 1;
      DWORD Reserved_0 : 1;
      DWORD Default_Big : 1;
      DWORD Granularity : 1;
      DWORD BaseHi : 8;
    } Bits;
  } HighWord;
} LDT_ENTRY, *PLDT_ENTRY, *LPLDT_ENTRY;


#include <pshpack1.h>

typedef struct _KTSSNOIOPM
{
    USHORT PreviousTask;
    USHORT Reserved1;
    ULONG  Esp0;
    USHORT Ss0;
    USHORT Reserved2;
    ULONG  Esp1;
    USHORT Ss1;
    USHORT Reserved3;
    ULONG  Esp2;
    USHORT Ss2;
    USHORT Reserved4;
    ULONG  Cr3;
    ULONG  Eip;
    ULONG  Eflags;
    ULONG  Eax;
    ULONG  Ecx;
    ULONG  Edx;
    ULONG  Ebx;
    ULONG  Esp;
    ULONG  Ebp;
    ULONG  Esi;
    ULONG  Edi;
    USHORT Es;
    USHORT Reserved5;
    USHORT Cs;
    USHORT Reserved6;
    USHORT Ss;
    USHORT Reserved7;
    USHORT Ds;
    USHORT Reserved8;
    USHORT Fs;
    USHORT Reserved9;
    USHORT Gs;
    USHORT Reserved10;
    USHORT Ldt;
    USHORT Reserved11;
    USHORT Trap;
    USHORT IoMapBase;
    /* no interrupt redirection map */
    UCHAR IoBitmap[1];
} KTSSNOIOPM;

typedef struct _KTSS
{
    USHORT PreviousTask;
    USHORT Reserved1;
    ULONG  Esp0;
    USHORT Ss0;
    USHORT Reserved2;
    ULONG  Esp1;
    USHORT Ss1;
    USHORT Reserved3;
    ULONG  Esp2;
    USHORT Ss2;
    USHORT Reserved4;
    ULONG  Cr3;
    ULONG  Eip;
    ULONG  Eflags;
    ULONG  Eax;
    ULONG  Ecx;
    ULONG  Edx;
    ULONG  Ebx;
    ULONG  Esp;
    ULONG  Ebp;
    ULONG  Esi;
    ULONG  Edi;
    USHORT Es;
    USHORT Reserved5;
    USHORT Cs;
    USHORT Reserved6;
    USHORT Ss;
    USHORT Reserved7;
    USHORT Ds;
    USHORT Reserved8;
    USHORT Fs;
    USHORT Reserved9;
    USHORT Gs;
    USHORT Reserved10;
    USHORT Ldt;
    USHORT Reserved11;
    USHORT Trap;
    USHORT IoMapBase;
    /* no interrupt redirection map */
    UCHAR  IoBitmap[8193];
} KTSS;

#include <poppack.h>

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

typedef struct _KEVENT_PAIR
{
    CSHORT Type;
    CSHORT Size;
    KEVENT LowEvent;
    KEVENT HighEvent;
} KEVENT_PAIR, *PKEVENT_PAIR;

typedef enum _KOBJECTS {
   EventNotificationObject = 0,
   EventSynchronizationObject = 1,
   MutantObject = 2,
   ProcessObject = 3,
   QueueObject = 4,
   SemaphoreObject = 5,
   ThreadObject = 6,
   GateObject = 7,
   TimerNotificationObject = 8,
   TimerSynchronizationObject = 9,
   Spare2Object = 10,
   Spare3Object = 11,
   Spare4Object = 12,
   Spare5Object = 13,
   Spare6Object = 14,
   Spare7Object = 15,
   Spare8Object = 16,
   Spare9Object = 17,
   ApcObject = 18,
   DpcObject = 19,
   DeviceQueueObject = 20,
   EventPairObject = 21,
   InterruptObject = 22,
   ProfileObject = 23,
   ThreadedDpcObject = 24,
   MaximumKernelObject = 25
} KOBJECTS;

typedef enum _KTHREAD_STATE {
    Initialized,
    Ready,
    Running,
    Standby,
    Terminated,
    Waiting,
    Transition,
    DeferredReady,
} KTHREAD_STATE, *PKTHREAD_STATE;
#endif
