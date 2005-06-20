/*
    This file was added by Andrew Greenwood. So, if it messes anything up,
    you know who to call...
*/

#ifndef __INCLUDE_NTOS_KEFUNCS_H
#define __INCLUDE_NTOS_KEFUNCS_H

#define KEBUGCHECK(a) DbgPrint("KeBugCheck (0x%X) at %s:%i\n", a, __FILE__,__LINE__), KeBugCheck(a)
#define KEBUGCHECKEX(a,b,c,d,e) DbgPrint("KeBugCheckEx (0x%X, 0x%X, 0x%X, 0x%X, 0x%X) at %s:%i\n", a, b, c, d, e, __FILE__,__LINE__), KeBugCheckEx(a,b,c,d,e)

#include <arc/arc.h>

#ifdef __NTOSKRNL__
extern CHAR EXPORTED KeNumberProcessors;
extern LOADER_PARAMETER_BLOCK EXPORTED KeLoaderBlock;
extern ULONG EXPORTED KeDcacheFlushCount;
extern ULONG EXPORTED KeIcacheFlushCount;
extern KAFFINITY EXPORTED KeActiveProcessors;
extern ULONG EXPORTED KiDmaIoCoherency; /* RISC Architectures only */
extern ULONG EXPORTED KeMaximumIncrement;
extern ULONG EXPORTED KeMinimumIncrement;
#else
extern CHAR IMPORTED KeNumberProcessors;
extern KAFFINITY IMPORTED KeActiveProcessors;
extern LOADER_PARAMETER_BLOCK IMPORTED KeLoaderBlock;
extern ULONG EXPORTED KeDcacheFlushCount;
extern ULONG EXPORTED KeIcacheFlushCount;
extern ULONG IMPORTED KiDmaIoCoherency; /* RISC Architectures only */
extern ULONG IMPORTED KeMaximumIncrement;
extern ULONG IMPORTED KeMinimumIncrement;
#endif

/* io permission map has a 8k size
 * Each bit in the IOPM corresponds to an io port byte address. The bitmap
 * is initialized to allow IO at any port. [ all bits set ]. 
 */
typedef struct _IOPM
{
  UCHAR  Bitmap[8192];
} IOPM, *PIOPM;

typedef struct _CONFIGURATION_COMPONENT_DATA 
{
  struct _CONFIGURATION_COMPONENT_DATA *Parent;
  struct _CONFIGURATION_COMPONENT_DATA *Child;
  struct _CONFIGURATION_COMPONENT_DATA *Sibling;
  CONFIGURATION_COMPONENT Component;
} CONFIGURATION_COMPONENT_DATA, *PCONFIGURATION_COMPONENT_DATA;

VOID STDCALL
KeCapturePersistentThreadState(
  IN PVOID  CurrentThread,
  IN ULONG  Setting1,
  IN ULONG  Setting2,
  IN ULONG  Setting3,
  IN ULONG  Setting4,
  IN ULONG  Setting5,
  IN PVOID  ThreadState);

BOOLEAN STDCALL
KeConnectInterrupt(
  PKINTERRUPT  InterruptObject);

VOID STDCALL
KeDisconnectInterrupt(
  PKINTERRUPT  InterruptObject);

VOID STDCALL
KeDrainApcQueue(
  VOID);

VOID STDCALL
KeEnterKernelDebugger(
  VOID);

PCONFIGURATION_COMPONENT_DATA STDCALL
KeFindConfigurationNextEntry(
  IN PCONFIGURATION_COMPONENT_DATA Child,
  IN CONFIGURATION_CLASS Class,
  IN CONFIGURATION_TYPE Type,
  IN PULONG ComponentKey OPTIONAL,
  IN PCONFIGURATION_COMPONENT_DATA *NextLink);
                             
PCONFIGURATION_COMPONENT_DATA STDCALL
KeFindConfigurationEntry(
  IN PCONFIGURATION_COMPONENT_DATA Child,
  IN CONFIGURATION_CLASS Class,
  IN CONFIGURATION_TYPE Type,
  IN PULONG ComponentKey OPTIONAL);

VOID STDCALL
KeFlushEntireTb(
  IN BOOLEAN  Unknown,
  IN BOOLEAN  CurrentCpuOnly);

struct _KPROCESS*
STDCALL
KeGetCurrentProcess(
    VOID
);

VOID STDCALL
KeFlushWriteBuffer(
  VOID);

VOID STDCALL
KeInitializeApc(
  IN PKAPC  Apc,
  IN PKTHREAD  Thread,
  IN KAPC_ENVIRONMENT  TargetEnvironment,
  IN PKKERNEL_ROUTINE  KernelRoutine,
  IN PKRUNDOWN_ROUTINE  RundownRoutine,
  IN PKNORMAL_ROUTINE  NormalRoutine,
  IN KPROCESSOR_MODE  Mode,
  IN PVOID  Context);

VOID STDCALL
KeInitializeInterrupt(
  PKINTERRUPT  InterruptObject,
  PKSERVICE_ROUTINE  ServiceRoutine,
  PVOID  ServiceContext,
  PKSPIN_LOCK  SpinLock,
  ULONG  Vector,
  KIRQL  Irql,
  KIRQL  SynchronizeIrql,
  KINTERRUPT_MODE  InterruptMode,
  BOOLEAN  ShareVector,
  CHAR  ProcessorNumber,
  BOOLEAN  FloatingSave);
 
BOOLEAN STDCALL
KeInsertQueueApc(
  PKAPC  Apc,
  PVOID  SystemArgument1,
  PVOID  SystemArgument2,
  KPRIORITY  PriorityBoost);

BOOLEAN STDCALL
KeIsAttachedProcess(
  VOID);

BOOLEAN STDCALL
KeIsExecutingDpc(
  VOID);
 
VOID STDCALL
KeRevertToUserAffinityThread(
  VOID);

BOOLEAN STDCALL
KeRemoveSystemServiceTable(
  IN ULONG  TableIndex);

NTSTATUS STDCALL
KeSetAffinityThread(
  PKTHREAD  Thread,
  KAFFINITY  Affinity);

VOID STDCALL
KeSetDmaIoCoherency(
  IN ULONG  Coherency);

VOID STDCALL
KeSetEventBoostPriority(
  IN PKEVENT  Event,
  IN PKTHREAD  *Thread OPTIONAL);

VOID STDCALL
KeSetProfileIrql(
  IN KIRQL  ProfileIrql);

VOID STDCALL
KeSetSystemAffinityThread(
  IN KAFFINITY  Affinity);

VOID STDCALL
KeTerminateThread(
  IN KPRIORITY  Increment);

NTSTATUS STDCALL
KeUserModeCallback(
  IN ULONG  FunctionID,
  IN PVOID  InputBuffer,
  IN ULONG  InputLength,
  OUT PVOID  *OutputBuffer,
  OUT PULONG  OutputLength);
 
NTSTATUS STDCALL
KeRaiseUserException(
  IN NTSTATUS  ExceptionCode);

/*
 * FUNCTION: Provides the kernel with a new access map for a driver
 * ARGUMENTS:
 * 	NewMap: =  If FALSE the kernel's map is set to all disabled. If TRUE
 *			the kernel disables access to a particular port.
 *	IoPortMap = Caller supplies storage for the io permission map.
 * REMARKS
 *	Each bit in the IOPM corresponds to an io port byte address. The bitmap
 *	is initialized to allow IO at any port. [ all bits set ]. The IOPL determines
 *	the minium privilege level required to perform IO prior to checking the permission map.
 */
BOOL STDCALL
Ke386SetIoAccessMap(
  ULONG  NewMap,
  PULONG  IoPermissionMap);

/*
 * FUNCTION: Queries the io permission  map.
 * ARGUMENTS:
 * 	NewMap: =  If FALSE the kernel's map is set to all disabled. If TRUE
 *			the kernel disables access to a particular port.
 *	IoPortMap = Caller supplies storage for the io permission map.
 * REMARKS
 *	Each bit in the IOPM corresponds to an io port byte address. The bitmap
 *	is initialized to allow IO at any port. [ all bits set ]. The IOPL determines
 *	the minium privilege level required to perform IO prior to checking the permission map.
 */
BOOL STDCALL
Ke386QueryIoAccessMap(
  ULONG  NewMap,
  PULONG  IoPermissionMap);

/* Set the process IOPL. */
BOOL STDCALL
Ke386IoSetAccessProcess(
  struct _EPROCESS  *Process,
  BOOL  EnableIo);

NTSTATUS STDCALL
KeI386FlatToGdtSelector(
  IN ULONG  Base,
  IN USHORT  Length,
  IN USHORT  Selector);

/* Releases a set of Global Descriptor Table Selectors. */
NTSTATUS STDCALL
KeI386ReleaseGdtSelectors(
  OUT PULONG SelArray,
  IN ULONG NumOfSelectors);

/* Allocates a set of Global Descriptor Table Selectors. */
NTSTATUS STDCALL
KeI386AllocateGdtSelectors(
  OUT PULONG SelArray,
  IN ULONG NumOfSelectors);

VOID FASTCALL
KiAcquireSpinLock(
  PKSPIN_LOCK  SpinLock);

VOID STDCALL
KiCoprocessorError(
  VOID);

VOID STDCALL
KiDispatchInterrupt(
  VOID);
 
NTSTATUS STDCALL
KeRaiseUserException(
  IN NTSTATUS ExceptionCode);

VOID FASTCALL
KiReleaseSpinLock(
  PKSPIN_LOCK  SpinLock);

VOID STDCALL
KiUnexpectedInterrupt(
  VOID);

/* REACTOS SPECIFIC */

VOID STDCALL
KeRosDumpStackFrames(
  PULONG  Frame,
  ULONG  FrameCount);

ULONG STDCALL
KeRosGetStackFrames(
  PULONG  Frames,
  ULONG  FrameCount);

#endif /* __INCLUDE_NTOS_KEFUNCS_H */
