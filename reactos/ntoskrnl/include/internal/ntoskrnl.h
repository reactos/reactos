#ifndef __INCLUDE_INTERNAL_NTOSKRNL_H
#define __INCLUDE_INTERNAL_NTOSKRNL_H

/*
 * Use these to place a function in a specific section of the executable
 */
#define PLACE_IN_SECTION(s)	__attribute__((section (s)))
#define INIT_FUNCTION		PLACE_IN_SECTION("init")
#define PAGE_LOCKED_FUNCTION	PLACE_IN_SECTION("pagelk")
#define PAGE_UNLOCKED_FUNCTION	PLACE_IN_SECTION("pagepo")

#ifdef _NTOSKRNL_

#include "ke.h"
#include "i386/mm.h"
#include "i386/fpu.h"
#include "ob.h"
#include "mm.h"
#include "ps.h"
#include "cc.h"
#include "io.h"
#include "po.h"
#include "se.h"
#include "ldr.h"
#include "kd.h"
#include "ex.h"
#include "xhal.h"
#include "v86m.h"
#include "fs.h"
#include "port.h"
#include "nls.h"
#ifdef KDBG
#include "../kdbg/kdb.h"
#endif
#include "dbgk.h"
#include "tag.h"
#include "test.h"
#include "inbv.h"

#include <pshpack1.h>
/*
 * Defines a descriptor as it appears in the processor tables
 */
typedef struct __DESCRIPTOR
{
  ULONG a;
  ULONG b;
} IDT_DESCRIPTOR, GDT_DESCRIPTOR;

#include <poppack.h>

extern IDT_DESCRIPTOR KiIdt[256];
//extern GDT_DESCRIPTOR KiGdt[256];

/*
 * Initalization functions (called once by main())
 */
VOID MmInitSystem(ULONG Phase, PLOADER_PARAMETER_BLOCK LoaderBlock, ULONG LastKernelAddress);
VOID IoInit(VOID);
VOID IoInit2(BOOLEAN BootLog);
VOID STDCALL IoInit3(VOID);
VOID ObInit(VOID);
VOID PsInit(VOID);
VOID CmInitializeRegistry(VOID);
VOID STDCALL CmInitHives(BOOLEAN SetupBoot);
VOID CmInit2(PCHAR CommandLine);
VOID CmShutdownRegistry(VOID);
BOOLEAN CmImportSystemHive(PCHAR ChunkBase, ULONG ChunkSize);
BOOLEAN CmImportHardwareHive(PCHAR ChunkBase, ULONG ChunkSize);
VOID KdInitSystem(ULONG Reserved, PLOADER_PARAMETER_BLOCK LoaderBlock);

/* FIXME - RtlpCreateUnicodeString is obsolete and should be removed ASAP! */
BOOLEAN FASTCALL
RtlpCreateUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PCWSTR  Source,
   IN POOL_TYPE PoolType);
   
NTSTATUS
RtlCaptureUnicodeString(
    OUT PUNICODE_STRING Dest,
    IN KPROCESSOR_MODE CurrentMode,
    IN POOL_TYPE PoolType,
    IN BOOLEAN CaptureIfKernel,
    IN PUNICODE_STRING UnsafeSrc
);

VOID
RtlReleaseCapturedUnicodeString(
    IN PUNICODE_STRING CapturedString,
    IN KPROCESSOR_MODE CurrentMode,
    IN BOOLEAN CaptureIfKernel
);

/*
 * Inlined Probing Macros
 *
 * NOTE: Alignment of the pointers is not verified!
 */
#define ProbeForWriteGenericType(Ptr, Type)                                    \
    do {                                                                       \
        if ((ULONG_PTR)(Ptr) + sizeof(Type) - 1 < (ULONG_PTR)(Ptr) ||          \
            (ULONG_PTR)(Ptr) + sizeof(Type) - 1 >= (ULONG_PTR)MmUserProbeAddress) { \
            ExRaiseStatus (STATUS_ACCESS_VIOLATION);                           \
        }                                                                      \
        *(volatile Type *)(Ptr) = *(volatile Type *)(Ptr);                     \
    } while (0)

#define ProbeForWriteBoolean(Ptr) ProbeForWriteGenericType(Ptr, BOOLEAN)
#define ProbeForWriteUchar(Ptr) ProbeForWriteGenericType(Ptr, UCHAR)
#define ProbeForWriteChar(Ptr) ProbeForWriteGenericType(Ptr, Char)
#define ProbeForWriteUshort(Ptr) ProbeForWriteGenericType(Ptr, USHORT)
#define ProbeForWriteShort(Ptr) ProbeForWriteGenericType(Ptr, SHORT)
#define ProbeForWriteUlong(Ptr) ProbeForWriteGenericType(Ptr, ULONG)
#define ProbeForWriteLong(Ptr) ProbeForWriteGenericType(Ptr, LONG)
#define ProbeForWriteUint(Ptr) ProbeForWriteGenericType(Ptr, UINT)
#define ProbeForWriteInt(Ptr) ProbeForWriteGenericType(Ptr, INT)
#define ProbeForWriteUlonglong(Ptr) ProbeForWriteGenericType(Ptr, ULONGLONG)
#define ProbeForWriteLonglong(Ptr) ProbeForWriteGenericType(Ptr, LONGLONG)
#define ProbeForWriteLonglong(Ptr) ProbeForWriteGenericType(Ptr, LONGLONG)
#define ProbeForWritePointer(Ptr) ProbeForWriteGenericType(Ptr, PVOID)
#define ProbeForWriteHandle(Ptr) ProbeForWriteGenericType(Ptr, HANDLE)
#define ProbeForWriteLangid(Ptr) ProbeForWriteGenericType(Ptr, LANGID)
#define ProbeForWriteLargeInteger(Ptr) ProbeForWriteGenericType(&(Ptr)->QuadPart, LONGLONG)
#define ProbeForWriteUlargeInteger(Ptr) ProbeForWriteGenericType(&(Ptr)->QuadPart, ULONGLONG)

#define ProbeForReadGenericType(Ptr, Type, Default)                            \
    (((ULONG_PTR)(Ptr) + sizeof(Type) - 1 < (ULONG_PTR)(Ptr) ||                \
	 (ULONG_PTR)(Ptr) + sizeof(Type) - 1 >= (ULONG_PTR)MmUserProbeAddress) ?   \
	     ExRaiseStatus (STATUS_ACCESS_VIOLATION), Default :                    \
	     *(Type *)(Ptr))

#define ProbeForReadBoolean(Ptr) ProbeForReadGenericType(Ptr, BOOLEAN, FALSE)
#define ProbeForReadUchar(Ptr) ProbeForReadGenericType(Ptr, UCHAR, 0)
#define ProbeForReadChar(Ptr) ProbeForReadGenericType(Ptr, CHAR, 0)
#define ProbeForReadUshort(Ptr) ProbeForReadGenericType(Ptr, USHORT, 0)
#define ProbeForReadShort(Ptr) ProbeForReadGenericType(Ptr, SHORT, 0)
#define ProbeForReadUlong(Ptr) ProbeForReadGenericType(Ptr, ULONG, 0)
#define ProbeForReadLong(Ptr) ProbeForReadGenericType(Ptr, LONG, 0)
#define ProbeForReadUint(Ptr) ProbeForReadGenericType(Ptr, UINT, 0)
#define ProbeForReadInt(Ptr) ProbeForReadGenericType(Ptr, INT, 0)
#define ProbeForReadUlonglong(Ptr) ProbeForReadGenericType(Ptr, ULONGLONG, 0)
#define ProbeForReadLonglong(Ptr) ProbeForReadGenericType(Ptr, LONGLONG, 0)
#define ProbeForReadPointer(Ptr) ProbeForReadGenericType(Ptr, PVOID, NULL)
#define ProbeForReadHandle(Ptr) ProbeForReadGenericType(Ptr, HANDLE, NULL)
#define ProbeForReadLangid(Ptr) ProbeForReadGenericType(Ptr, LANGID, 0)
#define ProbeForReadLargeInteger(Ptr) ((LARGE_INTEGER)ProbeForReadGenericType(&(Ptr)->QuadPart, LONGLONG, 0))
#define ProbeForReadUlargeInteger(Ptr) ((ULARGE_INTEGER)ProbeForReadGenericType(&(Ptr)->QuadPart, ULONGLONG, 0))

/*
 * Use IsKernelPointer to test whether a pointer points to the kernel address
 * space
 */
#if defined(_X86_) || defined(_M_AMD64)

/* for x86 and x86-64 the MSB is 1 so we can simply test on that */
#define IsKernelPointer(Ptr) ((LONG_PTR)(Ptr) < 0)

#elif defined(_IA64_)

/* on Itanium if the 24 most significant bits are set, we're not dealing with
   user mode pointers. */
#define IsKernelPointer(Ptr)  (((ULONG_PTR)(Ptr) & 0xFFFFFF0000000000ULL) != 0)

#else
#error IsKernelPointer() needs to be defined for this architecture
#endif

#endif
/*
 *
 */
#define MM_STACK_SIZE             (3*4096)

#endif /* INCLUDE_INTERNAL_NTOSKRNL_H */
