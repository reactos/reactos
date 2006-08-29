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
#include "i386/v86m.h"
#include "ob.h"
#include "mm.h"
#include "ex.h"
#include "ps.h"
#include "cc.h"
#include "io.h"
#include "po.h"
#include "se.h"
#include "ldr.h"
#include "kd.h"
#include "fsrtl.h"
#include "lpc.h"
#include "rtl.h"
#ifdef KDBG
#include "../kdbg/kdb.h"
#endif
#include "dbgk.h"
#include "tag.h"
#include "test.h"
#include "inbv.h"
#include "vdm.h"

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
//extern GDT_DESCRIPTOR KiGdt[256];

/*
 * Initalization functions (called once by main())
 */
VOID MmInitSystem(ULONG Phase, PROS_LOADER_PARAMETER_BLOCK LoaderBlock, ULONG LastKernelAddress);
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
VOID KdInitSystem(ULONG Reserved, PROS_LOADER_PARAMETER_BLOCK LoaderBlock);

/* FIXME - RtlpCreateUnicodeString is obsolete and should be removed ASAP! */
BOOLEAN FASTCALL
RtlpCreateUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PCWSTR  Source,
   IN POOL_TYPE PoolType);

VOID
NTAPI
RtlpLogException(IN PEXCEPTION_RECORD ExceptionRecord,
                 IN PCONTEXT ContextRecord,
                 IN PVOID ContextData,
                 IN ULONG Size);

/* FIXME: Interlocked functions that need to be made into a public header */
FORCEINLINE
LONG
InterlockedAnd(IN OUT LONG volatile *Target,
               IN LONG Set)
{
    LONG i;
    LONG j;

    j = *Target;
    do {
        i = j;
        j = InterlockedCompareExchange((PLONG)Target,
                                       i & Set,
                                       i);

    } while (i != j);

    return j;
}

FORCEINLINE
LONG
InterlockedOr(IN OUT LONG volatile *Target,
              IN LONG Set)
{
    LONG i;
    LONG j;

    j = *Target;
    do {
        i = j;
        j = InterlockedCompareExchange((PLONG)Target,
                                       i | Set,
                                       i);

    } while (i != j);

    return j;
}

/*
 * generic information class probing code
 */

#define ICIF_QUERY               0x1
#define ICIF_SET                 0x2
#define ICIF_QUERY_SIZE_VARIABLE 0x4
#define ICIF_SET_SIZE_VARIABLE   0x8
#define ICIF_SIZE_VARIABLE (ICIF_QUERY_SIZE_VARIABLE | ICIF_SET_SIZE_VARIABLE)

typedef struct _INFORMATION_CLASS_INFO
{
  ULONG RequiredSizeQUERY;
  ULONG RequiredSizeSET;
  ULONG AlignmentSET;
  ULONG AlignmentQUERY;
  ULONG Flags;
} INFORMATION_CLASS_INFO, *PINFORMATION_CLASS_INFO;

#define ICI_SQ_SAME(Type, Alignment, Flags)                                    \
  { Type, Type, Alignment, Alignment, Flags }

#define ICI_SQ(TypeQuery, TypeSet, AlignmentQuery, AlignmentSet, Flags)        \
  { TypeQuery, TypeSet, AlignmentQuery, AlignmentSet, Flags }

//
// TEMPORARY
//
#define IQS_SAME(Type, Alignment, Flags)                                    \
  { sizeof(Type), sizeof(Type), sizeof(Alignment), sizeof(Alignment), Flags }

#define IQS(TypeQuery, TypeSet, AlignmentQuery, AlignmentSet, Flags)        \
  { sizeof(TypeQuery), sizeof(TypeSet), sizeof(AlignmentQuery), sizeof(AlignmentSet), Flags }

static __inline NTSTATUS
DefaultSetInfoBufferCheck(UINT Class,
                          const INFORMATION_CLASS_INFO *ClassList,
                          UINT ClassListEntries,
                          PVOID Buffer,
                          ULONG BufferLength,
                          KPROCESSOR_MODE PreviousMode)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (Class >= 0 && Class < ClassListEntries)
    {
        if (!(ClassList[Class].Flags & ICIF_SET))
        {
            Status = STATUS_INVALID_INFO_CLASS;
        }
        else if (ClassList[Class].RequiredSizeSET > 0 &&
                 BufferLength != ClassList[Class].RequiredSizeSET)
        {
            if (!(ClassList[Class].Flags & ICIF_SET_SIZE_VARIABLE))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
        }

        if (NT_SUCCESS(Status))
        {
            if (PreviousMode != KernelMode)
            {
                _SEH_TRY
                {
                    ProbeForRead(Buffer,
                                 BufferLength,
                                 ClassList[Class].AlignmentSET);
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;
            }
        }
    }
    else
        Status = STATUS_INVALID_INFO_CLASS;

    return Status;
}

static __inline NTSTATUS
DefaultQueryInfoBufferCheck(UINT Class,
                            const INFORMATION_CLASS_INFO *ClassList,
                            UINT ClassListEntries,
                            PVOID Buffer,
                            ULONG BufferLength,
                            PULONG ReturnLength,
                            KPROCESSOR_MODE PreviousMode)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (Class >= 0 && Class < ClassListEntries)
    {
        if (!(ClassList[Class].Flags & ICIF_QUERY))
        {
            Status = STATUS_INVALID_INFO_CLASS;
        }
        else if (ClassList[Class].RequiredSizeQUERY > 0 &&
                 BufferLength != ClassList[Class].RequiredSizeQUERY)
        {
            if (!(ClassList[Class].Flags & ICIF_QUERY_SIZE_VARIABLE))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
        }

        if (NT_SUCCESS(Status))
        {
            if (PreviousMode != KernelMode)
            {
                _SEH_TRY
                {
                    if (Buffer != NULL)
                    {
                        ProbeForWrite(Buffer,
                                      BufferLength,
                                      ClassList[Class].AlignmentQUERY);
                    }

                    if (ReturnLength != NULL)
                    {
                        ProbeForWriteUlong(ReturnLength);
                    }
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;
            }
        }
    }
    else
        Status = STATUS_INVALID_INFO_CLASS;

    return Status;
}

/*
 * Use IsPointerOffset to test whether a pointer should be interpreted as an offset
 * or as a pointer
 */
#if defined(_X86_) || defined(_M_AMD64)

/* for x86 and x86-64 the MSB is 1 so we can simply test on that */
#define IsPointerOffset(Ptr) ((LONG_PTR)(Ptr) >= 0)

#elif defined(_IA64_)

/* on Itanium if the 24 most significant bits are set, we're not dealing with
   offsets anymore. */
#define IsPointerOffset(Ptr)  (((ULONG_PTR)(Ptr) & 0xFFFFFF0000000000ULL) == 0)

#else
#error IsPointerOffset() needs to be defined for this architecture
#endif

#endif

#endif /* INCLUDE_INTERNAL_NTOSKRNL_H */
