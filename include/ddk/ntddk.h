/*
 * ntddk.h
 *
 * Windows Device Driver Kit
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * DEFINES:
 *    DBG          - Debugging enabled/disabled (0/1)
 *    POOL_TAGGING - Enable pool tagging
 *    _X86_        - X86 environment
 */

#pragma once

#define _NTDDK_

#if !defined(_NTHAL_) && !defined(_NTIFS_)
#define _NTDDK_INCLUDED_
#define _DDK_DRIVER_
#endif

/* Dependencies */

#define NT_INCLUDED
#define _CTYPE_DISABLE_MACROS

#include <wdm.h>
#include <excpt.h>
#include <ntdef.h>
#include <ntstatus.h>

/* FIXME
#include <bugcodes.h>
#include <ntiologc.h>
*/

#include <stdarg.h> // FIXME
#include <basetyps.h> // FIXME


#ifdef __cplusplus
extern "C" {
#endif

typedef struct _BUS_HANDLER *PBUS_HANDLER;
typedef struct _CALLBACK_OBJECT *PCALLBACK_OBJECT;
typedef struct _DEVICE_HANDLER_OBJECT *PDEVICE_HANDLER_OBJECT;
#if defined(_NTHAL_INCLUDED_)
typedef struct _KPROCESS *PEPROCESS;
typedef struct _ETHREAD *PETHREAD;
typedef struct _KAFFINITY_EX *PKAFFINITY_EX;
#elif defined(_NTIFS_INCLUDED_)
typedef struct _KPROCESS *PEPROCESS;
typedef struct _KTHREAD *PETHREAD;
#else
typedef struct _EPROCESS *PEPROCESS;
typedef struct _ETHREAD *PETHREAD;
#endif
typedef struct _IO_TIMER *PIO_TIMER;
typedef struct _KINTERRUPT *PKINTERRUPT;
typedef struct _KTHREAD *PKTHREAD, *PRKTHREAD;
typedef struct _OBJECT_TYPE *POBJECT_TYPE;
typedef struct _PEB *PPEB;
typedef struct _IMAGE_NT_HEADERS *PIMAGE_NT_HEADERS32;
typedef struct _IMAGE_NT_HEADERS64 *PIMAGE_NT_HEADERS64;

#ifdef _WIN64
typedef PIMAGE_NT_HEADERS64 PIMAGE_NT_HEADERS;
#else
typedef PIMAGE_NT_HEADERS32 PIMAGE_NT_HEADERS;
#endif

#define PsGetCurrentProcess IoGetCurrentProcess

#if (NTDDI_VERSION >= NTDDI_VISTA)
extern NTSYSAPI volatile CCHAR KeNumberProcessors;
#elif (NTDDI_VERSION >= NTDDI_WINXP)
extern NTSYSAPI CCHAR KeNumberProcessors;
#else
extern PCCHAR KeNumberProcessors;
#endif

/* FIXME
#include <mce.h>
*/

#ifdef _X86_

#define KERNEL_STACK_SIZE                   12288
#define KERNEL_LARGE_STACK_SIZE             61440
#define KERNEL_LARGE_STACK_COMMIT           12288

#define SIZE_OF_80387_REGISTERS   80

#if !defined(RC_INVOKED)

#define CONTEXT_i386               0x10000
#define CONTEXT_i486               0x10000
#define CONTEXT_CONTROL            (CONTEXT_i386|0x00000001L)
#define CONTEXT_INTEGER            (CONTEXT_i386|0x00000002L)
#define CONTEXT_SEGMENTS           (CONTEXT_i386|0x00000004L)
#define CONTEXT_FLOATING_POINT     (CONTEXT_i386|0x00000008L)
#define CONTEXT_DEBUG_REGISTERS    (CONTEXT_i386|0x00000010L)
#define CONTEXT_EXTENDED_REGISTERS (CONTEXT_i386|0x00000020L)

#define CONTEXT_FULL (CONTEXT_CONTROL|CONTEXT_INTEGER|CONTEXT_SEGMENTS)
#define CONTEXT_ALL (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS |  \
                     CONTEXT_FLOATING_POINT | CONTEXT_DEBUG_REGISTERS |      \
                     CONTEXT_EXTENDED_REGISTERS)

#define CONTEXT_XSTATE          (CONTEXT_i386 | 0x00000040L)

#endif /* !defined(RC_INVOKED) */

typedef struct _FLOATING_SAVE_AREA {
  ULONG ControlWord;
  ULONG StatusWord;
  ULONG TagWord;
  ULONG ErrorOffset;
  ULONG ErrorSelector;
  ULONG DataOffset;
  ULONG DataSelector;
  UCHAR RegisterArea[SIZE_OF_80387_REGISTERS];
  ULONG Cr0NpxState;
} FLOATING_SAVE_AREA, *PFLOATING_SAVE_AREA;

#include "pshpack4.h"
typedef struct _CONTEXT {
  ULONG ContextFlags;
  ULONG Dr0;
  ULONG Dr1;
  ULONG Dr2;
  ULONG Dr3;
  ULONG Dr6;
  ULONG Dr7;
  FLOATING_SAVE_AREA FloatSave;
  ULONG SegGs;
  ULONG SegFs;
  ULONG SegEs;
  ULONG SegDs;
  ULONG Edi;
  ULONG Esi;
  ULONG Ebx;
  ULONG Edx;
  ULONG Ecx;
  ULONG Eax;
  ULONG Ebp;
  ULONG Eip;
  ULONG SegCs;
  ULONG EFlags;
  ULONG Esp;
  ULONG SegSs;
  UCHAR ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION];
} CONTEXT, *PCONTEXT;
#include "poppack.h"

#endif /* _X86_ */

#ifdef _AMD64_

#define KERNEL_STACK_SIZE 0x6000
#define KERNEL_LARGE_STACK_SIZE 0x12000
#define KERNEL_LARGE_STACK_COMMIT KERNEL_STACK_SIZE

#define KERNEL_MCA_EXCEPTION_STACK_SIZE 0x2000

#define EXCEPTION_READ_FAULT    0
#define EXCEPTION_WRITE_FAULT   1
#define EXCEPTION_EXECUTE_FAULT 8

#if !defined(RC_INVOKED)

#define CONTEXT_AMD64 0x100000

#define CONTEXT_CONTROL (CONTEXT_AMD64 | 0x1L)
#define CONTEXT_INTEGER (CONTEXT_AMD64 | 0x2L)
#define CONTEXT_SEGMENTS (CONTEXT_AMD64 | 0x4L)
#define CONTEXT_FLOATING_POINT (CONTEXT_AMD64 | 0x8L)
#define CONTEXT_DEBUG_REGISTERS (CONTEXT_AMD64 | 0x10L)

#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_FLOATING_POINT)
#define CONTEXT_ALL (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS | CONTEXT_FLOATING_POINT | CONTEXT_DEBUG_REGISTERS)

#define CONTEXT_XSTATE (CONTEXT_AMD64 | 0x20L)

#define CONTEXT_EXCEPTION_ACTIVE 0x8000000
#define CONTEXT_SERVICE_ACTIVE 0x10000000
#define CONTEXT_EXCEPTION_REQUEST 0x40000000
#define CONTEXT_EXCEPTION_REPORTING 0x80000000

#endif /* !defined(RC_INVOKED) */

#define INITIAL_MXCSR                  0x1f80
#define INITIAL_FPCSR                  0x027f

typedef struct DECLSPEC_ALIGN(16) _CONTEXT {
  ULONG64 P1Home;
  ULONG64 P2Home;
  ULONG64 P3Home;
  ULONG64 P4Home;
  ULONG64 P5Home;
  ULONG64 P6Home;
  ULONG ContextFlags;
  ULONG MxCsr;
  USHORT SegCs;
  USHORT SegDs;
  USHORT SegEs;
  USHORT SegFs;
  USHORT SegGs;
  USHORT SegSs;
  ULONG EFlags;
  ULONG64 Dr0;
  ULONG64 Dr1;
  ULONG64 Dr2;
  ULONG64 Dr3;
  ULONG64 Dr6;
  ULONG64 Dr7;
  ULONG64 Rax;
  ULONG64 Rcx;
  ULONG64 Rdx;
  ULONG64 Rbx;
  ULONG64 Rsp;
  ULONG64 Rbp;
  ULONG64 Rsi;
  ULONG64 Rdi;
  ULONG64 R8;
  ULONG64 R9;
  ULONG64 R10;
  ULONG64 R11;
  ULONG64 R12;
  ULONG64 R13;
  ULONG64 R14;
  ULONG64 R15;
  ULONG64 Rip;
  union {
    XMM_SAVE_AREA32 FltSave;
    struct {
      M128A Header[2];
      M128A Legacy[8];
      M128A Xmm0;
      M128A Xmm1;
      M128A Xmm2;
      M128A Xmm3;
      M128A Xmm4;
      M128A Xmm5;
      M128A Xmm6;
      M128A Xmm7;
      M128A Xmm8;
      M128A Xmm9;
      M128A Xmm10;
      M128A Xmm11;
      M128A Xmm12;
      M128A Xmm13;
      M128A Xmm14;
      M128A Xmm15;
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME;
  M128A VectorRegister[26];
  ULONG64 VectorControl;
  ULONG64 DebugControl;
  ULONG64 LastBranchToRip;
  ULONG64 LastBranchFromRip;
  ULONG64 LastExceptionToRip;
  ULONG64 LastExceptionFromRip;
} CONTEXT, *PCONTEXT;

#endif /* _AMD64_ */

typedef enum _WELL_KNOWN_SID_TYPE {
  WinNullSid = 0,
  WinWorldSid = 1,
  WinLocalSid = 2,
  WinCreatorOwnerSid = 3,
  WinCreatorGroupSid = 4,
  WinCreatorOwnerServerSid = 5,
  WinCreatorGroupServerSid = 6,
  WinNtAuthoritySid = 7,
  WinDialupSid = 8,
  WinNetworkSid = 9,
  WinBatchSid = 10,
  WinInteractiveSid = 11,
  WinServiceSid = 12,
  WinAnonymousSid = 13,
  WinProxySid = 14,
  WinEnterpriseControllersSid = 15,
  WinSelfSid = 16,
  WinAuthenticatedUserSid = 17,
  WinRestrictedCodeSid = 18,
  WinTerminalServerSid = 19,
  WinRemoteLogonIdSid = 20,
  WinLogonIdsSid = 21,
  WinLocalSystemSid = 22,
  WinLocalServiceSid = 23,
  WinNetworkServiceSid = 24,
  WinBuiltinDomainSid = 25,
  WinBuiltinAdministratorsSid = 26,
  WinBuiltinUsersSid = 27,
  WinBuiltinGuestsSid = 28,
  WinBuiltinPowerUsersSid = 29,
  WinBuiltinAccountOperatorsSid = 30,
  WinBuiltinSystemOperatorsSid = 31,
  WinBuiltinPrintOperatorsSid = 32,
  WinBuiltinBackupOperatorsSid = 33,
  WinBuiltinReplicatorSid = 34,
  WinBuiltinPreWindows2000CompatibleAccessSid = 35,
  WinBuiltinRemoteDesktopUsersSid = 36,
  WinBuiltinNetworkConfigurationOperatorsSid = 37,
  WinAccountAdministratorSid = 38,
  WinAccountGuestSid = 39,
  WinAccountKrbtgtSid = 40,
  WinAccountDomainAdminsSid = 41,
  WinAccountDomainUsersSid = 42,
  WinAccountDomainGuestsSid = 43,
  WinAccountComputersSid = 44,
  WinAccountControllersSid = 45,
  WinAccountCertAdminsSid = 46,
  WinAccountSchemaAdminsSid = 47,
  WinAccountEnterpriseAdminsSid = 48,
  WinAccountPolicyAdminsSid = 49,
  WinAccountRasAndIasServersSid = 50,
  WinNTLMAuthenticationSid = 51,
  WinDigestAuthenticationSid = 52,
  WinSChannelAuthenticationSid = 53,
  WinThisOrganizationSid = 54,
  WinOtherOrganizationSid = 55,
  WinBuiltinIncomingForestTrustBuildersSid = 56,
  WinBuiltinPerfMonitoringUsersSid = 57,
  WinBuiltinPerfLoggingUsersSid = 58,
  WinBuiltinAuthorizationAccessSid = 59,
  WinBuiltinTerminalServerLicenseServersSid = 60,
  WinBuiltinDCOMUsersSid = 61,
  WinBuiltinIUsersSid = 62,
  WinIUserSid = 63,
  WinBuiltinCryptoOperatorsSid = 64,
  WinUntrustedLabelSid = 65,
  WinLowLabelSid = 66,
  WinMediumLabelSid = 67,
  WinHighLabelSid = 68,
  WinSystemLabelSid = 69,
  WinWriteRestrictedCodeSid = 70,
  WinCreatorOwnerRightsSid = 71,
  WinCacheablePrincipalsGroupSid = 72,
  WinNonCacheablePrincipalsGroupSid = 73,
  WinEnterpriseReadonlyControllersSid = 74,
  WinAccountReadonlyControllersSid = 75,
  WinBuiltinEventLogReadersGroup = 76,
  WinNewEnterpriseReadonlyControllersSid = 77,
  WinBuiltinCertSvcDComAccessGroup = 78,
  WinMediumPlusLabelSid = 79,
  WinLocalLogonSid = 80,
  WinConsoleLogonSid = 81,
  WinThisOrganizationCertificateSid = 82,
} WELL_KNOWN_SID_TYPE;

#define SE_UNSOLICITED_INPUT_PRIVILEGE    6

#ifndef _RTL_RUN_ONCE_DEF
#define _RTL_RUN_ONCE_DEF

#define RTL_RUN_ONCE_INIT {0}

#define RTL_RUN_ONCE_CHECK_ONLY     0x00000001UL
#define RTL_RUN_ONCE_ASYNC          0x00000002UL
#define RTL_RUN_ONCE_INIT_FAILED    0x00000004UL

#define RTL_RUN_ONCE_CTX_RESERVED_BITS 2

#define RTL_HASH_ALLOCATED_HEADER            0x00000001

#define RTL_HASH_RESERVED_SIGNATURE 0

/* RtlVerifyVersionInfo() ComparisonType */

#define VER_EQUAL                       1
#define VER_GREATER                     2
#define VER_GREATER_EQUAL               3
#define VER_LESS                        4
#define VER_LESS_EQUAL                  5
#define VER_AND                         6
#define VER_OR                          7

#define VER_CONDITION_MASK              7
#define VER_NUM_BITS_PER_CONDITION_MASK 3

/* RtlVerifyVersionInfo() TypeMask */

#define VER_MINORVERSION                  0x0000001
#define VER_MAJORVERSION                  0x0000002
#define VER_BUILDNUMBER                   0x0000004
#define VER_PLATFORMID                    0x0000008
#define VER_SERVICEPACKMINOR              0x0000010
#define VER_SERVICEPACKMAJOR              0x0000020
#define VER_SUITENAME                     0x0000040
#define VER_PRODUCT_TYPE                  0x0000080

#define VER_NT_WORKSTATION              0x0000001
#define VER_NT_DOMAIN_CONTROLLER        0x0000002
#define VER_NT_SERVER                   0x0000003

#define VER_PLATFORM_WIN32s             0
#define VER_PLATFORM_WIN32_WINDOWS      1
#define VER_PLATFORM_WIN32_NT           2

typedef union _RTL_RUN_ONCE {
  PVOID Ptr;
} RTL_RUN_ONCE, *PRTL_RUN_ONCE;

typedef ULONG /* LOGICAL */
(NTAPI *PRTL_RUN_ONCE_INIT_FN) (
  IN OUT PRTL_RUN_ONCE RunOnce,
  IN OUT PVOID Parameter OPTIONAL,
  IN OUT PVOID *Context OPTIONAL);

#endif /* _RTL_RUN_ONCE_DEF */

typedef enum _TABLE_SEARCH_RESULT {
  TableEmptyTree,
  TableFoundNode,
  TableInsertAsLeft,
  TableInsertAsRight
} TABLE_SEARCH_RESULT;

typedef enum _RTL_GENERIC_COMPARE_RESULTS {
  GenericLessThan,
  GenericGreaterThan,
  GenericEqual
} RTL_GENERIC_COMPARE_RESULTS;

// Forwarder
struct _RTL_AVL_TABLE;

typedef RTL_GENERIC_COMPARE_RESULTS
(NTAPI *PRTL_AVL_COMPARE_ROUTINE) (
  IN struct _RTL_AVL_TABLE *Table,
  IN PVOID FirstStruct,
  IN PVOID SecondStruct);

typedef PVOID
(NTAPI *PRTL_AVL_ALLOCATE_ROUTINE) (
  IN struct _RTL_AVL_TABLE *Table,
  IN CLONG ByteSize);

typedef VOID
(NTAPI *PRTL_AVL_FREE_ROUTINE) (
  IN struct _RTL_AVL_TABLE *Table,
  IN PVOID Buffer);

typedef NTSTATUS
(NTAPI *PRTL_AVL_MATCH_FUNCTION) (
  IN struct _RTL_AVL_TABLE *Table,
  IN PVOID UserData,
  IN PVOID MatchData);

typedef struct _RTL_BALANCED_LINKS {
  struct _RTL_BALANCED_LINKS *Parent;
  struct _RTL_BALANCED_LINKS *LeftChild;
  struct _RTL_BALANCED_LINKS *RightChild;
  CHAR Balance;
  UCHAR Reserved[3];
} RTL_BALANCED_LINKS, *PRTL_BALANCED_LINKS;

typedef struct _RTL_AVL_TABLE {
  RTL_BALANCED_LINKS BalancedRoot;
  PVOID OrderedPointer;
  ULONG WhichOrderedElement;
  ULONG NumberGenericTableElements;
  ULONG DepthOfTree;
  PRTL_BALANCED_LINKS RestartKey;
  ULONG DeleteCount;
  PRTL_AVL_COMPARE_ROUTINE CompareRoutine;
  PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine;
  PRTL_AVL_FREE_ROUTINE FreeRoutine;
  PVOID TableContext;
} RTL_AVL_TABLE, *PRTL_AVL_TABLE;

#ifndef RTL_USE_AVL_TABLES

struct _RTL_GENERIC_TABLE;

typedef RTL_GENERIC_COMPARE_RESULTS
(NTAPI *PRTL_GENERIC_COMPARE_ROUTINE) (
  IN struct _RTL_GENERIC_TABLE *Table,
  IN PVOID FirstStruct,
  IN PVOID SecondStruct);

typedef PVOID
(NTAPI *PRTL_GENERIC_ALLOCATE_ROUTINE) (
  IN struct _RTL_GENERIC_TABLE *Table,
  IN CLONG ByteSize);

typedef VOID
(NTAPI *PRTL_GENERIC_FREE_ROUTINE) (
  IN struct _RTL_GENERIC_TABLE *Table,
  IN PVOID Buffer);

typedef struct _RTL_GENERIC_TABLE {
  PRTL_SPLAY_LINKS TableRoot;
  LIST_ENTRY InsertOrderList;
  PLIST_ENTRY OrderedPointer;
  ULONG WhichOrderedElement;
  ULONG NumberGenericTableElements;
  PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine;
  PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine;
  PRTL_GENERIC_FREE_ROUTINE FreeRoutine;
  PVOID TableContext;
} RTL_GENERIC_TABLE, *PRTL_GENERIC_TABLE;

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTSYSAPI
VOID
NTAPI
RtlInitializeGenericTable(
  OUT PRTL_GENERIC_TABLE Table,
  IN PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine,
  IN PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine,
  IN PRTL_GENERIC_FREE_ROUTINE FreeRoutine,
  IN PVOID TableContext OPTIONAL);

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTable(
  IN PRTL_GENERIC_TABLE Table,
  IN PVOID Buffer,
  IN CLONG BufferSize,
  OUT PBOOLEAN NewElement OPTIONAL);

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableFull(
  IN PRTL_GENERIC_TABLE Table,
  IN PVOID Buffer,
  IN CLONG BufferSize,
  OUT PBOOLEAN NewElement OPTIONAL,
  IN PVOID NodeOrParent,
  IN TABLE_SEARCH_RESULT SearchResult);

NTSYSAPI
BOOLEAN
NTAPI
RtlDeleteElementGenericTable(
  IN PRTL_GENERIC_TABLE Table,
  IN PVOID Buffer);

NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTable(
  IN PRTL_GENERIC_TABLE Table,
  IN PVOID Buffer);

NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableFull(
  IN PRTL_GENERIC_TABLE Table,
  IN PVOID Buffer,
  OUT PVOID *NodeOrParent,
  OUT TABLE_SEARCH_RESULT *SearchResult);

NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTable(
  IN PRTL_GENERIC_TABLE Table,
  IN BOOLEAN Restart);

NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableWithoutSplaying(
  IN PRTL_GENERIC_TABLE Table,
  IN OUT PVOID *RestartKey);

NTSYSAPI
PVOID
NTAPI
RtlGetElementGenericTable(
  IN PRTL_GENERIC_TABLE Table,
  IN ULONG I);

NTSYSAPI
ULONG
NTAPI
RtlNumberGenericTableElements(
  IN PRTL_GENERIC_TABLE Table);

NTSYSAPI
BOOLEAN
NTAPI
RtlIsGenericTableEmpty(
  IN PRTL_GENERIC_TABLE Table);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#endif /* RTL_USE_AVL_TABLES */

#ifdef RTL_USE_AVL_TABLES

#undef PRTL_GENERIC_COMPARE_ROUTINE
#undef RTL_GENERIC_COMPARE_ROUTINE
#undef PRTL_GENERIC_ALLOCATE_ROUTINE
#undef RTL_GENERIC_ALLOCATE_ROUTINE
#undef PRTL_GENERIC_FREE_ROUTINE
#undef RTL_GENERIC_FREE_ROUTINE
#undef RTL_GENERIC_TABLE
#undef PRTL_GENERIC_TABLE

#define PRTL_GENERIC_COMPARE_ROUTINE PRTL_AVL_COMPARE_ROUTINE
#define RTL_GENERIC_COMPARE_ROUTINE RTL_AVL_COMPARE_ROUTINE
#define PRTL_GENERIC_ALLOCATE_ROUTINE PRTL_AVL_ALLOCATE_ROUTINE
#define RTL_GENERIC_ALLOCATE_ROUTINE RTL_AVL_ALLOCATE_ROUTINE
#define PRTL_GENERIC_FREE_ROUTINE PRTL_AVL_FREE_ROUTINE
#define RTL_GENERIC_FREE_ROUTINE RTL_AVL_FREE_ROUTINE
#define RTL_GENERIC_TABLE RTL_AVL_TABLE
#define PRTL_GENERIC_TABLE PRTL_AVL_TABLE

#define RtlInitializeGenericTable               RtlInitializeGenericTableAvl
#define RtlInsertElementGenericTable            RtlInsertElementGenericTableAvl
#define RtlInsertElementGenericTableFull        RtlInsertElementGenericTableFullAvl
#define RtlDeleteElementGenericTable            RtlDeleteElementGenericTableAvl
#define RtlLookupElementGenericTable            RtlLookupElementGenericTableAvl
#define RtlLookupElementGenericTableFull        RtlLookupElementGenericTableFullAvl
#define RtlEnumerateGenericTable                RtlEnumerateGenericTableAvl
#define RtlEnumerateGenericTableWithoutSplaying RtlEnumerateGenericTableWithoutSplayingAvl
#define RtlGetElementGenericTable               RtlGetElementGenericTableAvl
#define RtlNumberGenericTableElements           RtlNumberGenericTableElementsAvl
#define RtlIsGenericTableEmpty                  RtlIsGenericTableEmptyAvl

#endif /* RTL_USE_AVL_TABLES */

typedef struct _RTL_SPLAY_LINKS {
  struct _RTL_SPLAY_LINKS *Parent;
  struct _RTL_SPLAY_LINKS *LeftChild;
  struct _RTL_SPLAY_LINKS *RightChild;
} RTL_SPLAY_LINKS, *PRTL_SPLAY_LINKS;

typedef struct _RTL_DYNAMIC_HASH_TABLE_ENTRY {
  LIST_ENTRY Linkage;
  ULONG_PTR Signature;
} RTL_DYNAMIC_HASH_TABLE_ENTRY, *PRTL_DYNAMIC_HASH_TABLE_ENTRY;

typedef struct _RTL_DYNAMIC_HASH_TABLE_CONTEXT {
  PLIST_ENTRY ChainHead;
  PLIST_ENTRY PrevLinkage;
  ULONG_PTR Signature;
} RTL_DYNAMIC_HASH_TABLE_CONTEXT, *PRTL_DYNAMIC_HASH_TABLE_CONTEXT;

typedef struct _RTL_DYNAMIC_HASH_TABLE_ENUMERATOR {
  RTL_DYNAMIC_HASH_TABLE_ENTRY HashEntry;
  PLIST_ENTRY ChainHead;
  ULONG BucketIndex;
} RTL_DYNAMIC_HASH_TABLE_ENUMERATOR, *PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR;

typedef struct _RTL_DYNAMIC_HASH_TABLE {
  ULONG Flags;
  ULONG Shift;
  ULONG TableSize;
  ULONG Pivot;
  ULONG DivisorMask;
  ULONG NumEntries;
  ULONG NonEmptyBuckets;
  ULONG NumEnumerators;
  PVOID Directory;
} RTL_DYNAMIC_HASH_TABLE, *PRTL_DYNAMIC_HASH_TABLE;

typedef struct _OSVERSIONINFOA {
  ULONG dwOSVersionInfoSize;
  ULONG dwMajorVersion;
  ULONG dwMinorVersion;
  ULONG dwBuildNumber;
  ULONG dwPlatformId;
  CHAR szCSDVersion[128];
} OSVERSIONINFOA, *POSVERSIONINFOA, *LPOSVERSIONINFOA;

typedef struct _OSVERSIONINFOW {
  ULONG dwOSVersionInfoSize;
  ULONG dwMajorVersion;
  ULONG dwMinorVersion;
  ULONG dwBuildNumber;
  ULONG dwPlatformId;
  WCHAR szCSDVersion[128];
} OSVERSIONINFOW, *POSVERSIONINFOW, *LPOSVERSIONINFOW, RTL_OSVERSIONINFOW, *PRTL_OSVERSIONINFOW;

typedef struct _OSVERSIONINFOEXA {
  ULONG dwOSVersionInfoSize;
  ULONG dwMajorVersion;
  ULONG dwMinorVersion;
  ULONG dwBuildNumber;
  ULONG dwPlatformId;
  CHAR szCSDVersion[128];
  USHORT wServicePackMajor;
  USHORT wServicePackMinor;
  USHORT wSuiteMask;
  UCHAR wProductType;
  UCHAR wReserved;
} OSVERSIONINFOEXA, *POSVERSIONINFOEXA, *LPOSVERSIONINFOEXA;

typedef struct _OSVERSIONINFOEXW {
  ULONG dwOSVersionInfoSize;
  ULONG dwMajorVersion;
  ULONG dwMinorVersion;
  ULONG dwBuildNumber;
  ULONG dwPlatformId;
  WCHAR szCSDVersion[128];
  USHORT wServicePackMajor;
  USHORT wServicePackMinor;
  USHORT wSuiteMask;
  UCHAR wProductType;
  UCHAR wReserved;
} OSVERSIONINFOEXW, *POSVERSIONINFOEXW, *LPOSVERSIONINFOEXW, RTL_OSVERSIONINFOEXW, *PRTL_OSVERSIONINFOEXW;

#ifdef UNICODE
typedef OSVERSIONINFOEXW OSVERSIONINFOEX;
typedef POSVERSIONINFOEXW POSVERSIONINFOEX;
typedef LPOSVERSIONINFOEXW LPOSVERSIONINFOEX;
typedef OSVERSIONINFOW OSVERSIONINFO;
typedef POSVERSIONINFOW POSVERSIONINFO;
typedef LPOSVERSIONINFOW LPOSVERSIONINFO;
#else
typedef OSVERSIONINFOEXA OSVERSIONINFOEX;
typedef POSVERSIONINFOEXA POSVERSIONINFOEX;
typedef LPOSVERSIONINFOEXA LPOSVERSIONINFOEX;
typedef OSVERSIONINFOA OSVERSIONINFO;
typedef POSVERSIONINFOA POSVERSIONINFO;
typedef LPOSVERSIONINFOA LPOSVERSIONINFO;
#endif /* UNICODE */

#define HASH_ENTRY_KEY(x)    ((x)->Signature)

#define RtlInitializeSplayLinks(Links) {    \
  PRTL_SPLAY_LINKS _SplayLinks;            \
  _SplayLinks = (PRTL_SPLAY_LINKS)(Links); \
  _SplayLinks->Parent = _SplayLinks;   \
  _SplayLinks->LeftChild = NULL;       \
  _SplayLinks->RightChild = NULL;      \
}

#define RtlIsLeftChild(Links) \
    (RtlLeftChild(RtlParent(Links)) == (PRTL_SPLAY_LINKS)(Links))

#define RtlIsRightChild(Links) \
    (RtlRightChild(RtlParent(Links)) == (PRTL_SPLAY_LINKS)(Links))

#define RtlRightChild(Links) \
    ((PRTL_SPLAY_LINKS)(Links))->RightChild

#define RtlIsRoot(Links) \
    (RtlParent(Links) == (PRTL_SPLAY_LINKS)(Links))

#define RtlLeftChild(Links) \
    ((PRTL_SPLAY_LINKS)(Links))->LeftChild

#define RtlParent(Links) \
    ((PRTL_SPLAY_LINKS)(Links))->Parent

#define RtlInsertAsLeftChild(ParentLinks,ChildLinks)    \
    {                                                   \
        PRTL_SPLAY_LINKS _SplayParent;                  \
        PRTL_SPLAY_LINKS _SplayChild;                   \
        _SplayParent = (PRTL_SPLAY_LINKS)(ParentLinks); \
        _SplayChild = (PRTL_SPLAY_LINKS)(ChildLinks);   \
        _SplayParent->LeftChild = _SplayChild;          \
        _SplayChild->Parent = _SplayParent;             \
    }

#define RtlInsertAsRightChild(ParentLinks,ChildLinks)   \
    {                                                   \
        PRTL_SPLAY_LINKS _SplayParent;                  \
        PRTL_SPLAY_LINKS _SplayChild;                   \
        _SplayParent = (PRTL_SPLAY_LINKS)(ParentLinks); \
        _SplayChild = (PRTL_SPLAY_LINKS)(ChildLinks);   \
        _SplayParent->RightChild = _SplayChild;         \
        _SplayChild->Parent = _SplayParent;             \
    }

#define RtlEqualLuid(L1, L2) (((L1)->LowPart == (L2)->LowPart) && \
                              ((L1)->HighPart  == (L2)->HighPart))

#define RtlIsZeroLuid(L1) ((BOOLEAN) (((L1)->LowPart | (L1)->HighPart) == 0))

#if !defined(MIDL_PASS)

FORCEINLINE
LUID
NTAPI_INLINE
RtlConvertLongToLuid(
  IN LONG Val)
{
  LUID Luid;
  LARGE_INTEGER Temp;

  Temp.QuadPart = Val;
  Luid.LowPart = Temp.u.LowPart;
  Luid.HighPart = Temp.u.HighPart;
  return Luid;
}

FORCEINLINE
LUID
NTAPI_INLINE
RtlConvertUlongToLuid(
  IN ULONG Val)
{
  LUID Luid;

  Luid.LowPart = Val;
  Luid.HighPart = 0;
  return Luid;
}

#endif /* !defined(MIDL_PASS) */

#if (defined(_M_AMD64) || defined(_M_IA64)) && !defined(_REALLY_GET_CALLERS_CALLER_)
#define RtlGetCallersAddress(CallersAddress, CallersCaller) \
    *CallersAddress = (PVOID)_ReturnAddress(); \
    *CallersCaller = NULL;
#else
#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
VOID
NTAPI
RtlGetCallersAddress(
  OUT PVOID *CallersAddress,
  OUT PVOID *CallersCaller);
#endif
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)

#define RTL_STACK_WALKING_MODE_FRAMES_TO_SKIP_SHIFT     8

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSplay(
  IN OUT PRTL_SPLAY_LINKS Links);

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlDelete(
  IN PRTL_SPLAY_LINKS Links);

NTSYSAPI
VOID
NTAPI
RtlDeleteNoSplay(
  IN PRTL_SPLAY_LINKS Links,
  IN OUT PRTL_SPLAY_LINKS *Root);

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSubtreeSuccessor(
  IN PRTL_SPLAY_LINKS Links);

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSubtreePredecessor(
  IN PRTL_SPLAY_LINKS Links);

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlRealSuccessor(
  IN PRTL_SPLAY_LINKS Links);

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlRealPredecessor(
  IN PRTL_SPLAY_LINKS Links);

NTSYSAPI
BOOLEAN
NTAPI
RtlPrefixUnicodeString(
  IN PCUNICODE_STRING  String1,
  IN PCUNICODE_STRING  String2,
  IN BOOLEAN  CaseInSensitive);

NTSYSAPI
VOID
NTAPI
RtlUpperString(
  IN OUT PSTRING  DestinationString,
  IN const PSTRING  SourceString);

NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeString(
  IN OUT PUNICODE_STRING DestinationString,
  IN PCUNICODE_STRING  SourceString,
  IN BOOLEAN  AllocateDestinationString);

NTSYSAPI
VOID
NTAPI
RtlMapGenericMask(
  IN OUT PACCESS_MASK AccessMask,
  IN PGENERIC_MAPPING GenericMapping);

NTSYSAPI
NTSTATUS
NTAPI
RtlVolumeDeviceToDosName(
  IN PVOID VolumeDeviceObject,
  OUT PUNICODE_STRING DosName);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetVersion(
  IN OUT PRTL_OSVERSIONINFOW lpVersionInformation);

NTSYSAPI
NTSTATUS
NTAPI
RtlVerifyVersionInfo(
  IN PRTL_OSVERSIONINFOEXW VersionInfo,
  IN ULONG TypeMask,
  IN ULONGLONG ConditionMask);

NTSYSAPI
LONG
NTAPI
RtlCompareString(
  IN const PSTRING String1,
  IN const PSTRING String2,
  IN BOOLEAN CaseInSensitive);

NTSYSAPI
VOID
NTAPI
RtlCopyString(
  OUT PSTRING DestinationString,
  IN const PSTRING SourceString OPTIONAL);

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualString(
  IN const PSTRING String1,
  IN const PSTRING String2,
  IN BOOLEAN CaseInSensitive);

NTSYSAPI
NTSTATUS
NTAPI
RtlCharToInteger(
  IN PCSZ String,
  IN ULONG Base OPTIONAL,
  OUT PULONG Value);

NTSYSAPI
CHAR
NTAPI
RtlUpperChar(
  IN CHAR Character);

NTSYSAPI
ULONG
NTAPI
RtlWalkFrameChain(
  OUT PVOID *Callers,
  IN ULONG Count,
  IN ULONG Flags);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_WINXP)

NTSYSAPI
VOID
NTAPI
RtlInitializeGenericTableAvl(
  OUT PRTL_AVL_TABLE Table,
  IN PRTL_AVL_COMPARE_ROUTINE CompareRoutine,
  IN PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine,
  IN PRTL_AVL_FREE_ROUTINE FreeRoutine,
  IN PVOID TableContext OPTIONAL);

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableAvl(
  IN PRTL_AVL_TABLE Table,
  IN PVOID Buffer,
  IN CLONG BufferSize,
  OUT PBOOLEAN NewElement OPTIONAL);

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableFullAvl(
  IN PRTL_AVL_TABLE Table,
  IN PVOID Buffer,
  IN CLONG BufferSize,
  OUT PBOOLEAN NewElement OPTIONAL,
  IN PVOID NodeOrParent,
  IN TABLE_SEARCH_RESULT SearchResult);

NTSYSAPI
BOOLEAN
NTAPI
RtlDeleteElementGenericTableAvl(
  IN PRTL_AVL_TABLE Table,
  IN PVOID Buffer);

NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableAvl(
  IN PRTL_AVL_TABLE Table,
  IN PVOID Buffer);

NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableFullAvl(
  IN PRTL_AVL_TABLE Table,
  IN PVOID Buffer,
  OUT PVOID *NodeOrParent,
  OUT TABLE_SEARCH_RESULT *SearchResult);

NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableAvl(
  IN PRTL_AVL_TABLE Table,
  IN BOOLEAN Restart);

NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableWithoutSplayingAvl(
  IN PRTL_AVL_TABLE Table,
  IN OUT PVOID *RestartKey);

NTSYSAPI
PVOID
NTAPI
RtlLookupFirstMatchingElementGenericTableAvl(
  IN PRTL_AVL_TABLE Table,
  IN PVOID Buffer,
  OUT PVOID *RestartKey);

NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableLikeADirectory(
  IN PRTL_AVL_TABLE Table,
  IN PRTL_AVL_MATCH_FUNCTION MatchFunction OPTIONAL,
  IN PVOID MatchData OPTIONAL,
  IN ULONG NextFlag,
  IN OUT PVOID *RestartKey,
  IN OUT PULONG DeleteCount,
  IN PVOID Buffer);

NTSYSAPI
PVOID
NTAPI
RtlGetElementGenericTableAvl(
  IN PRTL_AVL_TABLE Table,
  IN ULONG I);

NTSYSAPI
ULONG
NTAPI
RtlNumberGenericTableElementsAvl(
  IN PRTL_AVL_TABLE Table);

NTSYSAPI
BOOLEAN
NTAPI
RtlIsGenericTableEmptyAvl(
  IN PRTL_AVL_TABLE Table);


#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#if (NTDDI_VERSION >= NTDDI_VISTA)

NTSYSAPI
VOID
NTAPI
RtlRunOnceInitialize(
  OUT PRTL_RUN_ONCE RunOnce);

NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceExecuteOnce(
  IN OUT PRTL_RUN_ONCE RunOnce,
  IN PRTL_RUN_ONCE_INIT_FN InitFn,
  IN OUT PVOID Parameter OPTIONAL,
  OUT PVOID *Context OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceBeginInitialize(
  IN OUT PRTL_RUN_ONCE RunOnce,
  IN ULONG Flags,
  OUT PVOID *Context OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceComplete(
  IN OUT PRTL_RUN_ONCE RunOnce,
  IN ULONG Flags,
  IN PVOID Context OPTIONAL);

NTSYSAPI
BOOLEAN
NTAPI
RtlGetProductInfo(
  IN ULONG OSMajorVersion,
  IN ULONG OSMinorVersion,
  IN ULONG SpMajorVersion,
  IN ULONG SpMinorVersion,
  OUT PULONG ReturnedProductType);

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

#if !defined(MIDL_PASS) && !defined(SORTPP_PASS)

#if (NTDDI_VERSION >= NTDDI_WIN7)

FORCEINLINE
VOID
NTAPI
RtlInitHashTableContext(
  IN OUT PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context)
{
  Context->ChainHead = NULL;
  Context->PrevLinkage = NULL;
}

FORCEINLINE
VOID
NTAPI
RtlInitHashTableContextFromEnumerator(
  IN OUT PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context,
  IN PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator)
{
  Context->ChainHead = Enumerator->ChainHead;
  Context->PrevLinkage = Enumerator->HashEntry.Linkage.Blink;
}

FORCEINLINE
VOID
NTAPI
RtlReleaseHashTableContext(
  IN OUT PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context)
{
  UNREFERENCED_PARAMETER(Context);
  return;
}

FORCEINLINE
ULONG
NTAPI
RtlTotalBucketsHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable)
{
  return HashTable->TableSize;
}

FORCEINLINE
ULONG
NTAPI
RtlNonEmptyBucketsHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable)
{
  return HashTable->NonEmptyBuckets;
}

FORCEINLINE
ULONG
NTAPI
RtlEmptyBucketsHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable)
{
  return HashTable->TableSize - HashTable->NonEmptyBuckets;
}

FORCEINLINE
ULONG
NTAPI
RtlTotalEntriesHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable)
{
  return HashTable->NumEntries;
}

FORCEINLINE
ULONG
NTAPI
RtlActiveEnumeratorsHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable)
{
  return HashTable->NumEnumerators;
}

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

#endif /* !defined(MIDL_PASS) && !defined(SORTPP_PASS) */

#if (NTDDI_VERSION >= NTDDI_WIN7)

NTSYSAPI
BOOLEAN
NTAPI
RtlCreateHashTable(
  IN OUT PRTL_DYNAMIC_HASH_TABLE *HashTable OPTIONAL,
  IN ULONG Shift,
  IN ULONG Flags);

NTSYSAPI
VOID
NTAPI
RtlDeleteHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable);

NTSYSAPI
BOOLEAN
NTAPI
RtlInsertEntryHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable,
  IN PRTL_DYNAMIC_HASH_TABLE_ENTRY Entry,
  IN ULONG_PTR Signature,
  IN OUT PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context OPTIONAL);

NTSYSAPI
BOOLEAN
NTAPI
RtlRemoveEntryHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable,
  IN PRTL_DYNAMIC_HASH_TABLE_ENTRY Entry,
  IN OUT PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context OPTIONAL);

NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlLookupEntryHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable,
  IN ULONG_PTR Signature,
  OUT PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context OPTIONAL);

NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlGetNextEntryHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable,
  IN PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context);

NTSYSAPI
BOOLEAN
NTAPI
RtlInitEnumerationHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable,
  OUT PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator);

NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlEnumerateEntryHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable,
  IN OUT PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator);

NTSYSAPI
VOID
NTAPI
RtlEndEnumerationHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable,
  IN OUT PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator);

NTSYSAPI
BOOLEAN
NTAPI
RtlInitWeakEnumerationHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable,
  OUT PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator);

NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlWeaklyEnumerateEntryHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable,
  IN OUT PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator);

NTSYSAPI
VOID
NTAPI
RtlEndWeakEnumerationHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable,
  IN OUT PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator);

NTSYSAPI
BOOLEAN
NTAPI
RtlExpandHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable);

NTSYSAPI
BOOLEAN
NTAPI
RtlContractHashTable(
  IN PRTL_DYNAMIC_HASH_TABLE HashTable);

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

#if defined(_AMD64_) || defined(_IA64_)
//DECLSPEC_DEPRECATED_DDK_WINXP
FORCEINLINE
LARGE_INTEGER
NTAPI_INLINE
RtlLargeIntegerDivide(
  IN LARGE_INTEGER Dividend,
  IN LARGE_INTEGER Divisor,
  OUT PLARGE_INTEGER Remainder OPTIONAL)
{
  LARGE_INTEGER ret;
  ret.QuadPart = Dividend.QuadPart / Divisor.QuadPart;
  if (Remainder)
    Remainder->QuadPart = Dividend.QuadPart % Divisor.QuadPart;
  return ret;
}

#else

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
LARGE_INTEGER
NTAPI
RtlLargeIntegerDivide(
  IN LARGE_INTEGER Dividend,
  IN LARGE_INTEGER Divisor,
  OUT PLARGE_INTEGER Remainder OPTIONAL);
#endif

#endif /* defined(_AMD64_) || defined(_IA64_) */

#define VER_SET_CONDITION(ConditionMask, TypeBitMask, ComparisonType)  \
        ((ConditionMask) = VerSetConditionMask((ConditionMask), \
        (TypeBitMask), (ComparisonType)))

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
ULONGLONG
NTAPI
VerSetConditionMask(
  IN ULONGLONG ConditionMask,
  IN ULONG TypeMask,
  IN UCHAR Condition);
#endif

/** Kernel debugger routines **/

NTSYSAPI
ULONG
NTAPI
DbgPrompt(
  IN PCCH Prompt,
  OUT PCH Response,
  IN ULONG MaximumResponseLength);

#if (NTDDI_VERSION >= NTDDI_WIN7)

#define FILE_CHARACTERISTICS_EXPECT_ORDERLY_REMOVAL_EX     0x00004000
#define FILE_CHARACTERISTICS_EXPECT_SURPRISE_REMOVAL_EX    0x00008000
#define FILE_CHARACTERISTICS_REMOVAL_POLICY_MASK_EX \
    (FILE_CHARACTERISTICS_EXPECT_ORDERLY_REMOVAL_EX | \
     FILE_CHARACTERISTICS_EXPECT_SURPRISE_REMOVAL_EX)

#define FILE_CHARACTERISTICS_EXPECT_ORDERLY_REMOVAL_DEPRECATED 0x00000200
#define FILE_CHARACTERISTICS_EXPECT_SURPRISE_REMOVAL_DEPRECATED 0x00000300
#define FILE_CHARACTERISTICS_REMOVAL_POLICY_MASK_DEPRECATED 0x00000300

#else

#define FILE_CHARACTERISTICS_EXPECT_ORDERLY_REMOVAL     0x00000200
#define FILE_CHARACTERISTICS_EXPECT_SURPRISE_REMOVAL    0x00000300
#define FILE_CHARACTERISTICS_REMOVAL_POLICY_MASK        0x00000300

#define FILE_CHARACTERISTICS_EXPECT_ORDERLY_REMOVAL_EX FILE_CHARACTERISTICS_EXPECT_ORDERLY_REMOVAL
#define FILE_CHARACTERISTICS_EXPECT_SURPRISE_REMOVAL_EX FILE_CHARACTERISTICS_EXPECT_SURPRISE_REMOVAL
#define FILE_CHARACTERISTICS_REMOVAL_POLICY_MASK_EX FILE_CHARACTERISTICS_REMOVAL_POLICY_MASK

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

#define FILE_CHARACTERISTICS_PROPAGATED (   FILE_REMOVABLE_MEDIA   | \
                                            FILE_READ_ONLY_DEVICE  | \
                                            FILE_FLOPPY_DISKETTE   | \
                                            FILE_WRITE_ONCE_MEDIA  | \
                                            FILE_DEVICE_SECURE_OPEN  )

typedef struct _FILE_ALIGNMENT_INFORMATION {
  ULONG AlignmentRequirement;
} FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION;

typedef struct _FILE_NAME_INFORMATION {
  ULONG FileNameLength;
  WCHAR FileName[1];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;


typedef struct _FILE_ATTRIBUTE_TAG_INFORMATION {
  ULONG FileAttributes;
  ULONG ReparseTag;
} FILE_ATTRIBUTE_TAG_INFORMATION, *PFILE_ATTRIBUTE_TAG_INFORMATION;

typedef struct _FILE_DISPOSITION_INFORMATION {
  BOOLEAN DeleteFile;
} FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION;

typedef struct _FILE_END_OF_FILE_INFORMATION {
  LARGE_INTEGER EndOfFile;
} FILE_END_OF_FILE_INFORMATION, *PFILE_END_OF_FILE_INFORMATION;

typedef struct _FILE_VALID_DATA_LENGTH_INFORMATION {
  LARGE_INTEGER ValidDataLength;
} FILE_VALID_DATA_LENGTH_INFORMATION, *PFILE_VALID_DATA_LENGTH_INFORMATION;

typedef struct _FILE_FS_LABEL_INFORMATION {
  ULONG VolumeLabelLength;
  WCHAR VolumeLabel[1];
} FILE_FS_LABEL_INFORMATION, *PFILE_FS_LABEL_INFORMATION;

typedef struct _FILE_FS_VOLUME_INFORMATION {
  LARGE_INTEGER VolumeCreationTime;
  ULONG VolumeSerialNumber;
  ULONG VolumeLabelLength;
  BOOLEAN SupportsObjects;
  WCHAR VolumeLabel[1];
} FILE_FS_VOLUME_INFORMATION, *PFILE_FS_VOLUME_INFORMATION;

typedef struct _FILE_FS_SIZE_INFORMATION {
  LARGE_INTEGER TotalAllocationUnits;
  LARGE_INTEGER AvailableAllocationUnits;
  ULONG SectorsPerAllocationUnit;
  ULONG BytesPerSector;
} FILE_FS_SIZE_INFORMATION, *PFILE_FS_SIZE_INFORMATION;

typedef struct _FILE_FS_FULL_SIZE_INFORMATION {
  LARGE_INTEGER TotalAllocationUnits;
  LARGE_INTEGER CallerAvailableAllocationUnits;
  LARGE_INTEGER ActualAvailableAllocationUnits;
  ULONG SectorsPerAllocationUnit;
  ULONG BytesPerSector;
} FILE_FS_FULL_SIZE_INFORMATION, *PFILE_FS_FULL_SIZE_INFORMATION;

typedef struct _FILE_FS_OBJECTID_INFORMATION {
  UCHAR ObjectId[16];
  UCHAR ExtendedInfo[48];
} FILE_FS_OBJECTID_INFORMATION, *PFILE_FS_OBJECTID_INFORMATION;

typedef union _FILE_SEGMENT_ELEMENT {
  PVOID64 Buffer;
  ULONGLONG Alignment;
}FILE_SEGMENT_ELEMENT, *PFILE_SEGMENT_ELEMENT;

#define IOCTL_AVIO_ALLOCATE_STREAM      CTL_CODE(FILE_DEVICE_AVIO, 1, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_AVIO_FREE_STREAM          CTL_CODE(FILE_DEVICE_AVIO, 2, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_AVIO_MODIFY_STREAM        CTL_CODE(FILE_DEVICE_AVIO, 3, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

typedef enum _BUS_DATA_TYPE {
  ConfigurationSpaceUndefined = -1,
  Cmos,
  EisaConfiguration,
  Pos,
  CbusConfiguration,
  PCIConfiguration,
  VMEConfiguration,
  NuBusConfiguration,
  PCMCIAConfiguration,
  MPIConfiguration,
  MPSAConfiguration,
  PNPISAConfiguration,
  SgiInternalConfiguration,
  MaximumBusDataType
} BUS_DATA_TYPE, *PBUS_DATA_TYPE;

typedef struct _KEY_NAME_INFORMATION {
  ULONG NameLength;
  WCHAR Name[1];
} KEY_NAME_INFORMATION, *PKEY_NAME_INFORMATION;

typedef struct _KEY_CACHED_INFORMATION {
  LARGE_INTEGER LastWriteTime;
  ULONG TitleIndex;
  ULONG SubKeys;
  ULONG MaxNameLen;
  ULONG Values;
  ULONG MaxValueNameLen;
  ULONG MaxValueDataLen;
  ULONG NameLength;
} KEY_CACHED_INFORMATION, *PKEY_CACHED_INFORMATION;

typedef struct _KEY_VIRTUALIZATION_INFORMATION {
  ULONG VirtualizationCandidate:1;
  ULONG VirtualizationEnabled:1;
  ULONG VirtualTarget:1;
  ULONG VirtualStore:1;
  ULONG VirtualSource:1;
  ULONG Reserved:27;
} KEY_VIRTUALIZATION_INFORMATION, *PKEY_VIRTUALIZATION_INFORMATION;

#define THREAD_CSWITCH_PMU_DISABLE  FALSE
#define THREAD_CSWITCH_PMU_ENABLE   TRUE

#define PROCESS_LUID_DOSDEVICES_ONLY 0x00000001

#define PROCESS_HANDLE_TRACING_MAX_STACKS 16

typedef struct _NT_TIB {
  struct _EXCEPTION_REGISTRATION_RECORD *ExceptionList;
  PVOID StackBase;
  PVOID StackLimit;
  PVOID SubSystemTib;
  _ANONYMOUS_UNION union {
    PVOID FiberData;
    ULONG Version;
  } DUMMYUNIONNAME;
  PVOID ArbitraryUserPointer;
  struct _NT_TIB *Self;
} NT_TIB, *PNT_TIB;

typedef struct _NT_TIB32 {
  ULONG ExceptionList;
  ULONG StackBase;
  ULONG StackLimit;
  ULONG SubSystemTib;
  __GNU_EXTENSION union {
    ULONG FiberData;
    ULONG Version;
  };
  ULONG ArbitraryUserPointer;
  ULONG Self;
} NT_TIB32,*PNT_TIB32;

typedef struct _NT_TIB64 {
  ULONG64 ExceptionList;
  ULONG64 StackBase;
  ULONG64 StackLimit;
  ULONG64 SubSystemTib;
  __GNU_EXTENSION union {
    ULONG64 FiberData;
    ULONG Version;
  };
  ULONG64 ArbitraryUserPointer;
  ULONG64 Self;
} NT_TIB64,*PNT_TIB64;

typedef enum _PROCESSINFOCLASS {
  ProcessBasicInformation,
  ProcessQuotaLimits,
  ProcessIoCounters,
  ProcessVmCounters,
  ProcessTimes,
  ProcessBasePriority,
  ProcessRaisePriority,
  ProcessDebugPort,
  ProcessExceptionPort,
  ProcessAccessToken,
  ProcessLdtInformation,
  ProcessLdtSize,
  ProcessDefaultHardErrorMode,
  ProcessIoPortHandlers,
  ProcessPooledUsageAndLimits,
  ProcessWorkingSetWatch,
  ProcessUserModeIOPL,
  ProcessEnableAlignmentFaultFixup,
  ProcessPriorityClass,
  ProcessWx86Information,
  ProcessHandleCount,
  ProcessAffinityMask,
  ProcessPriorityBoost,
  ProcessDeviceMap,
  ProcessSessionInformation,
  ProcessForegroundInformation,
  ProcessWow64Information,
  ProcessImageFileName,
  ProcessLUIDDeviceMapsEnabled,
  ProcessBreakOnTermination,
  ProcessDebugObjectHandle,
  ProcessDebugFlags,
  ProcessHandleTracing,
  ProcessIoPriority,
  ProcessExecuteFlags,
  ProcessTlsInformation,
  ProcessCookie,
  ProcessImageInformation,
  ProcessCycleTime,
  ProcessPagePriority,
  ProcessInstrumentationCallback,
  ProcessThreadStackAllocation,
  ProcessWorkingSetWatchEx,
  ProcessImageFileNameWin32,
  ProcessImageFileMapping,
  ProcessAffinityUpdateMode,
  ProcessMemoryAllocationMode,
  ProcessGroupInformation,
  ProcessTokenVirtualizationEnabled,
  ProcessConsoleHostProcess,
  ProcessWindowInformation,
  MaxProcessInfoClass
} PROCESSINFOCLASS;

typedef enum _THREADINFOCLASS {
  ThreadBasicInformation,
  ThreadTimes,
  ThreadPriority,
  ThreadBasePriority,
  ThreadAffinityMask,
  ThreadImpersonationToken,
  ThreadDescriptorTableEntry,
  ThreadEnableAlignmentFaultFixup,
  ThreadEventPair_Reusable,
  ThreadQuerySetWin32StartAddress,
  ThreadZeroTlsCell,
  ThreadPerformanceCount,
  ThreadAmILastThread,
  ThreadIdealProcessor,
  ThreadPriorityBoost,
  ThreadSetTlsArrayAddress,
  ThreadIsIoPending,
  ThreadHideFromDebugger,
  ThreadBreakOnTermination,
  ThreadSwitchLegacyState,
  ThreadIsTerminated,
  ThreadLastSystemCall,
  ThreadIoPriority,
  ThreadCycleTime,
  ThreadPagePriority,
  ThreadActualBasePriority,
  ThreadTebInformation,
  ThreadCSwitchMon,
  ThreadCSwitchPmu,
  ThreadWow64Context,
  ThreadGroupInformation,
  ThreadUmsInformation,
  ThreadCounterProfiling,
  ThreadIdealProcessorEx,
  MaxThreadInfoClass
} THREADINFOCLASS;

typedef struct _PAGE_PRIORITY_INFORMATION {
  ULONG PagePriority;
} PAGE_PRIORITY_INFORMATION, *PPAGE_PRIORITY_INFORMATION;

typedef struct _PROCESS_WS_WATCH_INFORMATION {
  PVOID FaultingPc;
  PVOID FaultingVa;
} PROCESS_WS_WATCH_INFORMATION, *PPROCESS_WS_WATCH_INFORMATION;

typedef struct _PROCESS_BASIC_INFORMATION {
  NTSTATUS ExitStatus;
  struct _PEB *PebBaseAddress;
  ULONG_PTR AffinityMask;
  KPRIORITY BasePriority;
  ULONG_PTR UniqueProcessId;
  ULONG_PTR InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION,*PPROCESS_BASIC_INFORMATION;

typedef struct _PROCESS_EXTENDED_BASIC_INFORMATION {
  SIZE_T Size;
  PROCESS_BASIC_INFORMATION BasicInfo;
  union {
    ULONG Flags;
    struct {
      ULONG IsProtectedProcess:1;
      ULONG IsWow64Process:1;
      ULONG IsProcessDeleting:1;
      ULONG IsCrossSessionCreate:1;
      ULONG SpareBits:28;
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME;
} PROCESS_EXTENDED_BASIC_INFORMATION, *PPROCESS_EXTENDED_BASIC_INFORMATION;

typedef struct _PROCESS_DEVICEMAP_INFORMATION {
  __GNU_EXTENSION union {
    struct {
      HANDLE DirectoryHandle;
    } Set;
    struct {
      ULONG DriveMap;
      UCHAR DriveType[32];
    } Query;
  };
} PROCESS_DEVICEMAP_INFORMATION, *PPROCESS_DEVICEMAP_INFORMATION;

typedef struct _PROCESS_DEVICEMAP_INFORMATION_EX {
  union {
    struct {
      HANDLE DirectoryHandle;
    } Set;
    struct {
      ULONG DriveMap;
      UCHAR DriveType[32];
    } Query;
  } DUMMYUNIONNAME;
  ULONG Flags;
} PROCESS_DEVICEMAP_INFORMATION_EX, *PPROCESS_DEVICEMAP_INFORMATION_EX;

typedef struct _PROCESS_SESSION_INFORMATION {
  ULONG SessionId;
} PROCESS_SESSION_INFORMATION, *PPROCESS_SESSION_INFORMATION;

typedef struct _PROCESS_HANDLE_TRACING_ENABLE {
  ULONG Flags;
} PROCESS_HANDLE_TRACING_ENABLE, *PPROCESS_HANDLE_TRACING_ENABLE;

typedef struct _PROCESS_HANDLE_TRACING_ENABLE_EX {
  ULONG Flags;
  ULONG TotalSlots;
} PROCESS_HANDLE_TRACING_ENABLE_EX, *PPROCESS_HANDLE_TRACING_ENABLE_EX;

typedef struct _PROCESS_HANDLE_TRACING_ENTRY {
  HANDLE Handle;
  CLIENT_ID ClientId;
  ULONG Type;
  PVOID Stacks[PROCESS_HANDLE_TRACING_MAX_STACKS];
} PROCESS_HANDLE_TRACING_ENTRY, *PPROCESS_HANDLE_TRACING_ENTRY;

typedef struct _PROCESS_HANDLE_TRACING_QUERY {
  HANDLE Handle;
  ULONG TotalTraces;
  PROCESS_HANDLE_TRACING_ENTRY HandleTrace[1];
} PROCESS_HANDLE_TRACING_QUERY, *PPROCESS_HANDLE_TRACING_QUERY;

#define QUOTA_LIMITS_HARDWS_MIN_ENABLE  0x00000001
#define QUOTA_LIMITS_HARDWS_MIN_DISABLE 0x00000002
#define QUOTA_LIMITS_HARDWS_MAX_ENABLE  0x00000004
#define QUOTA_LIMITS_HARDWS_MAX_DISABLE 0x00000008
#define QUOTA_LIMITS_USE_DEFAULT_LIMITS 0x00000010

typedef struct _QUOTA_LIMITS {
  SIZE_T PagedPoolLimit;
  SIZE_T NonPagedPoolLimit;
  SIZE_T MinimumWorkingSetSize;
  SIZE_T MaximumWorkingSetSize;
  SIZE_T PagefileLimit;
  LARGE_INTEGER TimeLimit;
} QUOTA_LIMITS, *PQUOTA_LIMITS;

typedef union _RATE_QUOTA_LIMIT {
  ULONG RateData;
  struct {
    ULONG RatePercent:7;
    ULONG Reserved0:25;
  } DUMMYSTRUCTNAME;
} RATE_QUOTA_LIMIT, *PRATE_QUOTA_LIMIT;

typedef struct _QUOTA_LIMITS_EX {
  SIZE_T PagedPoolLimit;
  SIZE_T NonPagedPoolLimit;
  SIZE_T MinimumWorkingSetSize;
  SIZE_T MaximumWorkingSetSize;
  SIZE_T PagefileLimit;
  LARGE_INTEGER TimeLimit;
  SIZE_T WorkingSetLimit;
  SIZE_T Reserved2;
  SIZE_T Reserved3;
  SIZE_T Reserved4;
  ULONG Flags;
  RATE_QUOTA_LIMIT CpuRateLimit;
} QUOTA_LIMITS_EX, *PQUOTA_LIMITS_EX;

typedef struct _IO_COUNTERS {
  ULONGLONG ReadOperationCount;
  ULONGLONG WriteOperationCount;
  ULONGLONG OtherOperationCount;
  ULONGLONG ReadTransferCount;
  ULONGLONG WriteTransferCount;
  ULONGLONG OtherTransferCount;
} IO_COUNTERS, *PIO_COUNTERS;

typedef struct _VM_COUNTERS {
  SIZE_T PeakVirtualSize;
  SIZE_T VirtualSize;
  ULONG PageFaultCount;
  SIZE_T PeakWorkingSetSize;
  SIZE_T WorkingSetSize;
  SIZE_T QuotaPeakPagedPoolUsage;
  SIZE_T QuotaPagedPoolUsage;
  SIZE_T QuotaPeakNonPagedPoolUsage;
  SIZE_T QuotaNonPagedPoolUsage;
  SIZE_T PagefileUsage;
  SIZE_T PeakPagefileUsage;
} VM_COUNTERS, *PVM_COUNTERS;

typedef struct _VM_COUNTERS_EX {
  SIZE_T PeakVirtualSize;
  SIZE_T VirtualSize;
  ULONG PageFaultCount;
  SIZE_T PeakWorkingSetSize;
  SIZE_T WorkingSetSize;
  SIZE_T QuotaPeakPagedPoolUsage;
  SIZE_T QuotaPagedPoolUsage;
  SIZE_T QuotaPeakNonPagedPoolUsage;
  SIZE_T QuotaNonPagedPoolUsage;
  SIZE_T PagefileUsage;
  SIZE_T PeakPagefileUsage;
  SIZE_T PrivateUsage;
} VM_COUNTERS_EX, *PVM_COUNTERS_EX;

#define MAX_HW_COUNTERS 16
#define THREAD_PROFILING_FLAG_DISPATCH  0x00000001

typedef enum _HARDWARE_COUNTER_TYPE {
  PMCCounter,
  MaxHardwareCounterType
} HARDWARE_COUNTER_TYPE, *PHARDWARE_COUNTER_TYPE;

typedef struct _HARDWARE_COUNTER {
  HARDWARE_COUNTER_TYPE Type;
  ULONG Reserved;
  ULONG64 Index;
} HARDWARE_COUNTER, *PHARDWARE_COUNTER;

typedef struct _POOLED_USAGE_AND_LIMITS {
  SIZE_T PeakPagedPoolUsage;
  SIZE_T PagedPoolUsage;
  SIZE_T PagedPoolLimit;
  SIZE_T PeakNonPagedPoolUsage;
  SIZE_T NonPagedPoolUsage;
  SIZE_T NonPagedPoolLimit;
  SIZE_T PeakPagefileUsage;
  SIZE_T PagefileUsage;
  SIZE_T PagefileLimit;
} POOLED_USAGE_AND_LIMITS, *PPOOLED_USAGE_AND_LIMITS;

typedef struct _PROCESS_ACCESS_TOKEN {
  HANDLE Token;
  HANDLE Thread;
} PROCESS_ACCESS_TOKEN, *PPROCESS_ACCESS_TOKEN;

#define PROCESS_EXCEPTION_PORT_ALL_STATE_BITS     0x00000003UL
#define PROCESS_EXCEPTION_PORT_ALL_STATE_FLAGS    ((ULONG_PTR)((1UL << PROCESS_EXCEPTION_PORT_ALL_STATE_BITS) - 1))

typedef struct _PROCESS_EXCEPTION_PORT {
  IN HANDLE ExceptionPortHandle;
  IN OUT ULONG StateFlags;
} PROCESS_EXCEPTION_PORT, *PPROCESS_EXCEPTION_PORT;

typedef struct _KERNEL_USER_TIMES {
  LARGE_INTEGER CreateTime;
  LARGE_INTEGER ExitTime;
  LARGE_INTEGER KernelTime;
  LARGE_INTEGER UserTime;
} KERNEL_USER_TIMES, *PKERNEL_USER_TIMES;

/* NtXxx Functions */

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcess(
  OUT PHANDLE ProcessHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes,
  IN PCLIENT_ID ClientId OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationProcess(
  IN HANDLE ProcessHandle,
  IN PROCESSINFOCLASS ProcessInformationClass,
  OUT PVOID ProcessInformation OPTIONAL,
  IN ULONG ProcessInformationLength,
  OUT PULONG ReturnLength OPTIONAL);

struct _LOADER_PARAMETER_BLOCK;
struct _CREATE_DISK;
struct _DRIVE_LAYOUT_INFORMATION_EX;
struct _SET_PARTITION_INFORMATION_EX;

//
// GUID and UUID
//
#ifndef GUID_DEFINED
#include <guiddef.h>
#endif
typedef GUID UUID;

#define MAX_WOW64_SHARED_ENTRIES 16

#define NX_SUPPORT_POLICY_ALWAYSOFF 0
#define NX_SUPPORT_POLICY_ALWAYSON 1
#define NX_SUPPORT_POLICY_OPTIN 2
#define NX_SUPPORT_POLICY_OPTOUT 3

/*
** IRP function codes
*/

#define IRP_MN_QUERY_DIRECTORY            0x01
#define IRP_MN_NOTIFY_CHANGE_DIRECTORY    0x02

#define IRP_MN_USER_FS_REQUEST            0x00
#define IRP_MN_MOUNT_VOLUME               0x01
#define IRP_MN_VERIFY_VOLUME              0x02
#define IRP_MN_LOAD_FILE_SYSTEM           0x03
#define IRP_MN_TRACK_LINK                 0x04
#define IRP_MN_KERNEL_CALL                0x04

#define IRP_MN_LOCK                       0x01
#define IRP_MN_UNLOCK_SINGLE              0x02
#define IRP_MN_UNLOCK_ALL                 0x03
#define IRP_MN_UNLOCK_ALL_BY_KEY          0x04

#define IRP_MN_FLUSH_AND_PURGE          0x01

#define IRP_MN_NORMAL                     0x00
#define IRP_MN_DPC                        0x01
#define IRP_MN_MDL                        0x02
#define IRP_MN_COMPLETE                   0x04
#define IRP_MN_COMPRESSED                 0x08

#define IRP_MN_MDL_DPC                    (IRP_MN_MDL | IRP_MN_DPC)
#define IRP_MN_COMPLETE_MDL               (IRP_MN_COMPLETE | IRP_MN_MDL)
#define IRP_MN_COMPLETE_MDL_DPC           (IRP_MN_COMPLETE_MDL | IRP_MN_DPC)

#define IRP_MN_QUERY_LEGACY_BUS_INFORMATION 0x18

/* DEVICE_OBJECT.Flags */

#define DO_DEVICE_HAS_NAME                  0x00000040
#define DO_SYSTEM_BOOT_PARTITION            0x00000100
#define DO_LONG_TERM_REQUESTS               0x00000200
#define DO_NEVER_LAST_DEVICE                0x00000400
#define DO_LOW_PRIORITY_FILESYSTEM          0x00010000
#define DO_SUPPORTS_TRANSACTIONS            0x00040000
#define DO_FORCE_NEITHER_IO                 0x00080000
#define DO_VOLUME_DEVICE_OBJECT             0x00100000
#define DO_SYSTEM_SYSTEM_PARTITION          0x00200000
#define DO_SYSTEM_CRITICAL_PARTITION        0x00400000
#define DO_DISALLOW_EXECUTE                 0x00800000

#define DRVO_REINIT_REGISTERED          0x00000008
#define DRVO_INITIALIZED                0x00000010
#define DRVO_BOOTREINIT_REGISTERED      0x00000020
#define DRVO_LEGACY_RESOURCES           0x00000040

typedef enum _ARBITER_REQUEST_SOURCE {
  ArbiterRequestUndefined = -1,
  ArbiterRequestLegacyReported,
  ArbiterRequestHalReported,
  ArbiterRequestLegacyAssigned,
  ArbiterRequestPnpDetected,
  ArbiterRequestPnpEnumerated
} ARBITER_REQUEST_SOURCE;

typedef enum _ARBITER_RESULT {
  ArbiterResultUndefined = -1,
  ArbiterResultSuccess,
  ArbiterResultExternalConflict,
  ArbiterResultNullRequest
} ARBITER_RESULT;

typedef enum _ARBITER_ACTION {
  ArbiterActionTestAllocation,
  ArbiterActionRetestAllocation,
  ArbiterActionCommitAllocation,
  ArbiterActionRollbackAllocation,
  ArbiterActionQueryAllocatedResources,
  ArbiterActionWriteReservedResources,
  ArbiterActionQueryConflict,
  ArbiterActionQueryArbitrate,
  ArbiterActionAddReserved,
  ArbiterActionBootAllocation
} ARBITER_ACTION, *PARBITER_ACTION;

typedef struct _ARBITER_CONFLICT_INFO {
  PDEVICE_OBJECT OwningObject;
  ULONGLONG Start;
  ULONGLONG End;
} ARBITER_CONFLICT_INFO, *PARBITER_CONFLICT_INFO;

typedef struct _ARBITER_PARAMETERS {
  union {
    struct {
      IN OUT PLIST_ENTRY ArbitrationList;
      IN ULONG AllocateFromCount;
      IN PCM_PARTIAL_RESOURCE_DESCRIPTOR AllocateFrom;
    } TestAllocation;
    struct {
      IN OUT PLIST_ENTRY ArbitrationList;
      IN ULONG AllocateFromCount;
      IN PCM_PARTIAL_RESOURCE_DESCRIPTOR AllocateFrom;
    } RetestAllocation;
    struct {
      IN OUT PLIST_ENTRY ArbitrationList;
    } BootAllocation;
    struct {
      OUT PCM_PARTIAL_RESOURCE_LIST *AllocatedResources;
    } QueryAllocatedResources;
    struct {
      IN PDEVICE_OBJECT PhysicalDeviceObject;
      IN PIO_RESOURCE_DESCRIPTOR ConflictingResource;
      OUT PULONG ConflictCount;
      OUT PARBITER_CONFLICT_INFO *Conflicts;
    } QueryConflict;
    struct {
      IN PLIST_ENTRY ArbitrationList;
    } QueryArbitrate;
    struct {
      IN PDEVICE_OBJECT ReserveDevice;
    } AddReserved;
  } Parameters;
} ARBITER_PARAMETERS, *PARBITER_PARAMETERS;

#define ARBITER_FLAG_BOOT_CONFIG 0x00000001

typedef struct _ARBITER_LIST_ENTRY {
  LIST_ENTRY ListEntry;
  ULONG AlternativeCount;
  PIO_RESOURCE_DESCRIPTOR Alternatives;
  PDEVICE_OBJECT PhysicalDeviceObject;
  ARBITER_REQUEST_SOURCE RequestSource;
  ULONG Flags;
  LONG_PTR WorkSpace;
  INTERFACE_TYPE InterfaceType;
  ULONG SlotNumber;
  ULONG BusNumber;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR Assignment;
  PIO_RESOURCE_DESCRIPTOR SelectedAlternative;
  ARBITER_RESULT Result;
} ARBITER_LIST_ENTRY, *PARBITER_LIST_ENTRY;

typedef NTSTATUS
(NTAPI *PARBITER_HANDLER)(
  IN OUT PVOID Context,
  IN ARBITER_ACTION Action,
  IN OUT PARBITER_PARAMETERS Parameters);

#define ARBITER_PARTIAL 0x00000001

typedef struct _ARBITER_INTERFACE {
  USHORT Size;
  USHORT Version;
  PVOID Context;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PARBITER_HANDLER ArbiterHandler;
  ULONG Flags;
} ARBITER_INTERFACE, *PARBITER_INTERFACE;

typedef enum _HAL_QUERY_INFORMATION_CLASS {
  HalInstalledBusInformation,
  HalProfileSourceInformation,
  HalInformationClassUnused1,
  HalPowerInformation,
  HalProcessorSpeedInformation,
  HalCallbackInformation,
  HalMapRegisterInformation,
  HalMcaLogInformation,
  HalFrameBufferCachingInformation,
  HalDisplayBiosInformation,
  HalProcessorFeatureInformation,
  HalNumaTopologyInterface,
  HalErrorInformation,
  HalCmcLogInformation,
  HalCpeLogInformation,
  HalQueryMcaInterface,
  HalQueryAMLIIllegalIOPortAddresses,
  HalQueryMaxHotPlugMemoryAddress,
  HalPartitionIpiInterface,
  HalPlatformInformation,
  HalQueryProfileSourceList,
  HalInitLogInformation,
  HalFrequencyInformation,
  HalProcessorBrandString,
  HalHypervisorInformation,
  HalPlatformTimerInformation,
  HalAcpiAuditInformation
} HAL_QUERY_INFORMATION_CLASS, *PHAL_QUERY_INFORMATION_CLASS;

typedef enum _HAL_SET_INFORMATION_CLASS {
  HalProfileSourceInterval,
  HalProfileSourceInterruptHandler,
  HalMcaRegisterDriver,
  HalKernelErrorHandler,
  HalCmcRegisterDriver,
  HalCpeRegisterDriver,
  HalMcaLog,
  HalCmcLog,
  HalCpeLog,
  HalGenerateCmcInterrupt,
  HalProfileSourceTimerHandler,
  HalEnlightenment,
  HalProfileDpgoSourceInterruptHandler
} HAL_SET_INFORMATION_CLASS, *PHAL_SET_INFORMATION_CLASS;

typedef struct _HAL_PROFILE_SOURCE_INTERVAL {
  KPROFILE_SOURCE Source;
  ULONG_PTR Interval;
} HAL_PROFILE_SOURCE_INTERVAL, *PHAL_PROFILE_SOURCE_INTERVAL;

typedef struct _HAL_PROFILE_SOURCE_INFORMATION {
  KPROFILE_SOURCE Source;
  BOOLEAN Supported;
  ULONG Interval;
} HAL_PROFILE_SOURCE_INFORMATION, *PHAL_PROFILE_SOURCE_INFORMATION;

typedef struct _MAP_REGISTER_ENTRY {
  PVOID MapRegister;
  BOOLEAN WriteToDevice;
} MAP_REGISTER_ENTRY, *PMAP_REGISTER_ENTRY;

typedef struct _DEBUG_DEVICE_ADDRESS {
  UCHAR Type;
  BOOLEAN Valid;
  UCHAR Reserved[2];
  PUCHAR TranslatedAddress;
  ULONG Length;
} DEBUG_DEVICE_ADDRESS, *PDEBUG_DEVICE_ADDRESS;

typedef struct _DEBUG_MEMORY_REQUIREMENTS {
  PHYSICAL_ADDRESS Start;
  PHYSICAL_ADDRESS MaxEnd;
  PVOID VirtualAddress;
  ULONG Length;
  BOOLEAN Cached;
  BOOLEAN Aligned;
} DEBUG_MEMORY_REQUIREMENTS, *PDEBUG_MEMORY_REQUIREMENTS;

typedef struct _DEBUG_DEVICE_DESCRIPTOR {
  ULONG Bus;
  ULONG Slot;
  USHORT Segment;
  USHORT VendorID;
  USHORT DeviceID;
  UCHAR BaseClass;
  UCHAR SubClass;
  UCHAR ProgIf;
  BOOLEAN Initialized;
  BOOLEAN Configured;
  DEBUG_DEVICE_ADDRESS BaseAddress[6];
  DEBUG_MEMORY_REQUIREMENTS Memory;
} DEBUG_DEVICE_DESCRIPTOR, *PDEBUG_DEVICE_DESCRIPTOR;

typedef struct _PM_DISPATCH_TABLE {
  ULONG Signature;
  ULONG Version;
  PVOID Function[1];
} PM_DISPATCH_TABLE, *PPM_DISPATCH_TABLE;

typedef enum _RESOURCE_TRANSLATION_DIRECTION {
  TranslateChildToParent,
  TranslateParentToChild
} RESOURCE_TRANSLATION_DIRECTION;

typedef NTSTATUS
(NTAPI *PTRANSLATE_RESOURCE_HANDLER)(
  IN OUT PVOID Context,
  IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
  IN RESOURCE_TRANSLATION_DIRECTION Direction,
  IN ULONG AlternativesCount OPTIONAL,
  IN IO_RESOURCE_DESCRIPTOR Alternatives[],
  IN PDEVICE_OBJECT PhysicalDeviceObject,
  OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target);

typedef NTSTATUS
(NTAPI *PTRANSLATE_RESOURCE_REQUIREMENTS_HANDLER)(
  IN PVOID Context OPTIONAL,
  IN PIO_RESOURCE_DESCRIPTOR Source,
  IN PDEVICE_OBJECT PhysicalDeviceObject,
  OUT PULONG TargetCount,
  OUT PIO_RESOURCE_DESCRIPTOR *Target);

typedef struct _TRANSLATOR_INTERFACE {
  USHORT Size;
  USHORT Version;
  PVOID Context;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PTRANSLATE_RESOURCE_HANDLER TranslateResources;
  PTRANSLATE_RESOURCE_REQUIREMENTS_HANDLER TranslateResourceRequirements;
} TRANSLATOR_INTERFACE, *PTRANSLATOR_INTERFACE;

typedef VOID
(FASTCALL *pHalExamineMBR)(
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG SectorSize,
  IN ULONG MBRTypeIdentifier,
  OUT PVOID *Buffer);

typedef NTSTATUS
(FASTCALL *pHalIoReadPartitionTable)(
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG SectorSize,
  IN BOOLEAN ReturnRecognizedPartitions,
  OUT struct _DRIVE_LAYOUT_INFORMATION **PartitionBuffer);

typedef NTSTATUS
(FASTCALL *pHalIoSetPartitionInformation)(
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG SectorSize,
  IN ULONG PartitionNumber,
  IN ULONG PartitionType);

typedef NTSTATUS
(FASTCALL *pHalIoWritePartitionTable)(
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG SectorSize,
  IN ULONG SectorsPerTrack,
  IN ULONG NumberOfHeads,
  IN struct _DRIVE_LAYOUT_INFORMATION *PartitionBuffer);

typedef PBUS_HANDLER
(FASTCALL *pHalHandlerForBus)(
  IN INTERFACE_TYPE InterfaceType,
  IN ULONG BusNumber);

typedef VOID
(FASTCALL *pHalReferenceBusHandler)(
  IN PBUS_HANDLER BusHandler);

typedef NTSTATUS
(NTAPI *pHalQuerySystemInformation)(
  IN HAL_QUERY_INFORMATION_CLASS InformationClass,
  IN ULONG BufferSize,
  IN OUT PVOID Buffer,
  OUT PULONG ReturnedLength);

typedef NTSTATUS
(NTAPI *pHalSetSystemInformation)(
  IN HAL_SET_INFORMATION_CLASS InformationClass,
  IN ULONG BufferSize,
  IN PVOID Buffer);

typedef NTSTATUS
(NTAPI *pHalQueryBusSlots)(
  IN PBUS_HANDLER BusHandler,
  IN ULONG BufferSize,
  OUT PULONG SlotNumbers,
  OUT PULONG ReturnedLength);

typedef NTSTATUS
(NTAPI *pHalInitPnpDriver)(
  VOID);

typedef NTSTATUS
(NTAPI *pHalInitPowerManagement)(
  IN PPM_DISPATCH_TABLE PmDriverDispatchTable,
  OUT PPM_DISPATCH_TABLE *PmHalDispatchTable);

typedef struct _DMA_ADAPTER*
(NTAPI *pHalGetDmaAdapter)(
  IN PVOID Context,
  IN struct _DEVICE_DESCRIPTION *DeviceDescriptor,
  OUT PULONG NumberOfMapRegisters);

typedef NTSTATUS
(NTAPI *pHalGetInterruptTranslator)(
  IN INTERFACE_TYPE ParentInterfaceType,
  IN ULONG ParentBusNumber,
  IN INTERFACE_TYPE BridgeInterfaceType,
  IN USHORT Size,
  IN USHORT Version,
  OUT PTRANSLATOR_INTERFACE Translator,
  OUT PULONG BridgeBusNumber);

typedef NTSTATUS
(NTAPI *pHalStartMirroring)(
  VOID);

typedef NTSTATUS
(NTAPI *pHalEndMirroring)(
  IN ULONG PassNumber);

typedef NTSTATUS
(NTAPI *pHalMirrorPhysicalMemory)(
  IN PHYSICAL_ADDRESS PhysicalAddress,
  IN LARGE_INTEGER NumberOfBytes);

typedef NTSTATUS
(NTAPI *pHalMirrorVerify)(
  IN PHYSICAL_ADDRESS PhysicalAddress,
  IN LARGE_INTEGER NumberOfBytes);

typedef VOID
(NTAPI *pHalEndOfBoot)(
  VOID);

typedef
BOOLEAN
(NTAPI *pHalTranslateBusAddress)(
  IN INTERFACE_TYPE InterfaceType,
  IN ULONG BusNumber,
  IN PHYSICAL_ADDRESS BusAddress,
  IN OUT PULONG AddressSpace,
  OUT PPHYSICAL_ADDRESS TranslatedAddress);

typedef
NTSTATUS
(NTAPI *pHalAssignSlotResources)(
  IN PUNICODE_STRING RegistryPath,
  IN PUNICODE_STRING DriverClassName OPTIONAL,
  IN PDRIVER_OBJECT DriverObject,
  IN PDEVICE_OBJECT DeviceObject,
  IN INTERFACE_TYPE BusType,
  IN ULONG BusNumber,
  IN ULONG SlotNumber,
  IN OUT PCM_RESOURCE_LIST *AllocatedResources);

typedef
VOID
(NTAPI *pHalHaltSystem)(
  VOID);

typedef
BOOLEAN
(NTAPI *pHalResetDisplay)(
  VOID);

typedef
UCHAR
(NTAPI *pHalVectorToIDTEntry)(
  ULONG Vector);

typedef
BOOLEAN
(NTAPI *pHalFindBusAddressTranslation)(
  IN PHYSICAL_ADDRESS BusAddress,
  IN OUT PULONG AddressSpace,
  OUT PPHYSICAL_ADDRESS TranslatedAddress,
  IN OUT PULONG_PTR Context,
  IN BOOLEAN NextBus);

typedef
NTSTATUS
(NTAPI *pKdSetupPciDeviceForDebugging)(
  IN PVOID LoaderBlock OPTIONAL,
  IN OUT PDEBUG_DEVICE_DESCRIPTOR PciDevice);

typedef
NTSTATUS
(NTAPI *pKdReleasePciDeviceForDebugging)(
  IN OUT PDEBUG_DEVICE_DESCRIPTOR PciDevice);

typedef
PVOID
(NTAPI *pKdGetAcpiTablePhase0)(
  IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
  IN ULONG Signature);

typedef
PVOID
(NTAPI *pHalGetAcpiTable)(
  IN ULONG Signature,
  IN PCSTR OemId OPTIONAL,
  IN PCSTR OemTableId OPTIONAL);
  
typedef
VOID
(NTAPI *pKdCheckPowerButton)(
  VOID);

#if (NTDDI_VERSION >= NTDDI_VISTA)
typedef
PVOID
(NTAPI *pKdMapPhysicalMemory64)(
  IN PHYSICAL_ADDRESS PhysicalAddress,
  IN ULONG NumberPages,
  IN BOOLEAN FlushCurrentTLB);

typedef
VOID
(NTAPI *pKdUnmapVirtualAddress)(
  IN PVOID VirtualAddress,
  IN ULONG NumberPages,
  IN BOOLEAN FlushCurrentTLB);
#else
typedef
PVOID
(NTAPI *pKdMapPhysicalMemory64)(
  IN PHYSICAL_ADDRESS PhysicalAddress,
  IN ULONG NumberPages);

typedef
VOID
(NTAPI *pKdUnmapVirtualAddress)(
  IN PVOID VirtualAddress,
  IN ULONG NumberPages);
#endif


typedef
ULONG
(NTAPI *pKdGetPciDataByOffset)(
  IN ULONG BusNumber,
  IN ULONG SlotNumber,
  OUT PVOID Buffer,
  IN ULONG Offset,
  IN ULONG Length);

typedef
ULONG
(NTAPI *pKdSetPciDataByOffset)(
  IN ULONG BusNumber,
  IN ULONG SlotNumber,
  IN PVOID Buffer,
  IN ULONG Offset,
  IN ULONG Length);

typedef BOOLEAN
(NTAPI *PHAL_RESET_DISPLAY_PARAMETERS)(
  IN ULONG Columns,
  IN ULONG Rows);

typedef
VOID
(NTAPI *PCI_ERROR_HANDLER_CALLBACK)(
  VOID);

typedef
VOID
(NTAPI *pHalSetPciErrorHandlerCallback)(
  IN PCI_ERROR_HANDLER_CALLBACK Callback);

#if 1 /* Not present in WDK 7600 */
typedef VOID
(FASTCALL *pHalIoAssignDriveLetters)(
  IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
  IN PSTRING NtDeviceName,
  OUT PUCHAR NtSystemPath,
  OUT PSTRING NtSystemPathString);
#endif

typedef struct {
  ULONG Version;
  pHalQuerySystemInformation HalQuerySystemInformation;
  pHalSetSystemInformation HalSetSystemInformation;
  pHalQueryBusSlots HalQueryBusSlots;
  ULONG Spare1;
  pHalExamineMBR HalExamineMBR;
#if 1 /* Not present in WDK 7600 */
  pHalIoAssignDriveLetters HalIoAssignDriveLetters;
#endif
  pHalIoReadPartitionTable HalIoReadPartitionTable;
  pHalIoSetPartitionInformation HalIoSetPartitionInformation;
  pHalIoWritePartitionTable HalIoWritePartitionTable;
  pHalHandlerForBus HalReferenceHandlerForBus;
  pHalReferenceBusHandler HalReferenceBusHandler;
  pHalReferenceBusHandler HalDereferenceBusHandler;
  pHalInitPnpDriver HalInitPnpDriver;
  pHalInitPowerManagement HalInitPowerManagement;
  pHalGetDmaAdapter HalGetDmaAdapter;
  pHalGetInterruptTranslator HalGetInterruptTranslator;
  pHalStartMirroring HalStartMirroring;
  pHalEndMirroring HalEndMirroring;
  pHalMirrorPhysicalMemory HalMirrorPhysicalMemory;
  pHalEndOfBoot HalEndOfBoot;
  pHalMirrorVerify HalMirrorVerify;
  pHalGetAcpiTable HalGetCachedAcpiTable;
  pHalSetPciErrorHandlerCallback  HalSetPciErrorHandlerCallback;
#if defined(_IA64_)
  pHalGetErrorCapList HalGetErrorCapList;
  pHalInjectError HalInjectError;
#endif
} HAL_DISPATCH, *PHAL_DISPATCH;

/* GCC/MSVC and WDK compatible declaration */
extern NTKERNELAPI HAL_DISPATCH HalDispatchTable;

#if defined(_NTOSKRNL_) || defined(_BLDR_)
#define HALDISPATCH (&HalDispatchTable)
#else
/* This is a WDK compatibility definition */
#define HalDispatchTable (&HalDispatchTable)
#define HALDISPATCH HalDispatchTable
#endif

#define HAL_DISPATCH_VERSION            3 /* FIXME: when to use 4? */
#define HalDispatchTableVersion         HALDISPATCH->Version
#define HalQuerySystemInformation       HALDISPATCH->HalQuerySystemInformation
#define HalSetSystemInformation         HALDISPATCH->HalSetSystemInformation
#define HalQueryBusSlots                HALDISPATCH->HalQueryBusSlots
#define HalReferenceHandlerForBus       HALDISPATCH->HalReferenceHandlerForBus
#define HalReferenceBusHandler          HALDISPATCH->HalReferenceBusHandler
#define HalDereferenceBusHandler        HALDISPATCH->HalDereferenceBusHandler
#define HalInitPnpDriver                HALDISPATCH->HalInitPnpDriver
#define HalInitPowerManagement          HALDISPATCH->HalInitPowerManagement
#define HalGetDmaAdapter                HALDISPATCH->HalGetDmaAdapter
#define HalGetInterruptTranslator       HALDISPATCH->HalGetInterruptTranslator
#define HalStartMirroring               HALDISPATCH->HalStartMirroring
#define HalEndMirroring                 HALDISPATCH->HalEndMirroring
#define HalMirrorPhysicalMemory         HALDISPATCH->HalMirrorPhysicalMemory
#define HalEndOfBoot                    HALDISPATCH->HalEndOfBoot
#define HalMirrorVerify                 HALDISPATCH->HalMirrorVerify

typedef struct _IMAGE_INFO {
  _ANONYMOUS_UNION union {
    ULONG Properties;
    _ANONYMOUS_STRUCT struct {
      ULONG ImageAddressingMode:8;
      ULONG SystemModeImage:1;
      ULONG ImageMappedToAllPids:1;
      ULONG ExtendedInfoPresent:1;
      ULONG Reserved:22;
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME;
  PVOID ImageBase;
  ULONG ImageSelector;
  SIZE_T ImageSize;
  ULONG ImageSectionNumber;
} IMAGE_INFO, *PIMAGE_INFO;

#define IMAGE_ADDRESSING_MODE_32BIT       3

typedef enum _IO_QUERY_DEVICE_DATA_FORMAT {
  IoQueryDeviceIdentifier = 0,
  IoQueryDeviceConfigurationData,
  IoQueryDeviceComponentInformation,
  IoQueryDeviceMaxData
} IO_QUERY_DEVICE_DATA_FORMAT, *PIO_QUERY_DEVICE_DATA_FORMAT;

typedef struct _DISK_SIGNATURE {
  ULONG PartitionStyle;
  _ANONYMOUS_UNION union {
    struct {
      ULONG Signature;
      ULONG CheckSum;
    } Mbr;
    struct {
      GUID DiskId;
    } Gpt;
  } DUMMYUNIONNAME;
} DISK_SIGNATURE, *PDISK_SIGNATURE;

typedef ULONG_PTR
(NTAPI *PDRIVER_VERIFIER_THUNK_ROUTINE)(
  IN PVOID Context);

typedef struct _DRIVER_VERIFIER_THUNK_PAIRS {
  PDRIVER_VERIFIER_THUNK_ROUTINE PristineRoutine;
  PDRIVER_VERIFIER_THUNK_ROUTINE NewRoutine;
} DRIVER_VERIFIER_THUNK_PAIRS, *PDRIVER_VERIFIER_THUNK_PAIRS;

#define DRIVER_VERIFIER_SPECIAL_POOLING             0x0001
#define DRIVER_VERIFIER_FORCE_IRQL_CHECKING         0x0002
#define DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES  0x0004
#define DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS      0x0008
#define DRIVER_VERIFIER_IO_CHECKING                 0x0010

typedef VOID
(NTAPI *PTIMER_APC_ROUTINE)(
  IN PVOID TimerContext,
  IN ULONG TimerLowValue,
  IN LONG TimerHighValue);

typedef struct _KUSER_SHARED_DATA
{
    ULONG TickCountLowDeprecated;
    ULONG TickCountMultiplier;
    volatile KSYSTEM_TIME InterruptTime;
    volatile KSYSTEM_TIME SystemTime;
    volatile KSYSTEM_TIME TimeZoneBias;
    USHORT ImageNumberLow;
    USHORT ImageNumberHigh;
    WCHAR NtSystemRoot[260];
    ULONG MaxStackTraceDepth;
    ULONG CryptoExponent;
    ULONG TimeZoneId;
    ULONG LargePageMinimum;
    ULONG Reserved2[7];
    NT_PRODUCT_TYPE NtProductType;
    BOOLEAN ProductTypeIsValid;
    ULONG NtMajorVersion;
    ULONG NtMinorVersion;
    BOOLEAN ProcessorFeatures[PROCESSOR_FEATURE_MAX];
    ULONG Reserved1;
    ULONG Reserved3;
    volatile ULONG TimeSlip;
    ALTERNATIVE_ARCHITECTURE_TYPE AlternativeArchitecture;
    ULONG AltArchitecturePad[1];
    LARGE_INTEGER SystemExpirationDate;
    ULONG SuiteMask;
    BOOLEAN KdDebuggerEnabled;
#if (NTDDI_VERSION >= NTDDI_WINXPSP2)
    UCHAR NXSupportPolicy;
#endif
    volatile ULONG ActiveConsoleId;
    volatile ULONG DismountCount;
    ULONG ComPlusPackage;
    ULONG LastSystemRITEventTickCount;
    ULONG NumberOfPhysicalPages;
    BOOLEAN SafeBootMode;
#if (NTDDI_VERSION >= NTDDI_WIN7)
    union {
        UCHAR TscQpcData;
        struct {
            UCHAR TscQpcEnabled:1;
            UCHAR TscQpcSpareFlag:1;
            UCHAR TscQpcShift:6;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    UCHAR TscQpcPad[2];
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
    union {
        ULONG SharedDataFlags;
        struct {
            ULONG DbgErrorPortPresent:1;
            ULONG DbgElevationEnabled:1;
            ULONG DbgVirtEnabled:1;
            ULONG DbgInstallerDetectEnabled:1;
            ULONG DbgSystemDllRelocated:1;
            ULONG DbgDynProcessorEnabled:1;
            ULONG DbgSEHValidationEnabled:1;
            ULONG SpareBits:25;
        } DUMMYSTRUCTNAME2;
    } DUMMYUNIONNAME2;
#else
    ULONG TraceLogging;
#endif
    ULONG DataFlagsPad[1];
    ULONGLONG TestRetInstruction;
    ULONG SystemCall;
    ULONG SystemCallReturn;
    ULONGLONG SystemCallPad[3];
    _ANONYMOUS_UNION union {
        volatile KSYSTEM_TIME TickCount;
        volatile ULONG64 TickCountQuad;
        _ANONYMOUS_STRUCT struct {
            ULONG ReservedTickCountOverlay[3];
            ULONG TickCountPad[1];
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME3;
    ULONG Cookie;
    ULONG CookiePad[1];
#if (NTDDI_VERSION >= NTDDI_WS03)
    LONGLONG ConsoleSessionForegroundProcessId;
    ULONG Wow64SharedInformation[MAX_WOW64_SHARED_ENTRIES];
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
#if (NTDDI_VERSION >= NTDDI_WIN7)
    USHORT UserModeGlobalLogger[16];
#else
    USHORT UserModeGlobalLogger[8];
    ULONG HeapTracingPid[2];
    ULONG CritSecTracingPid[2];
#endif
    ULONG ImageFileExecutionOptions;
#if (NTDDI_VERSION >= NTDDI_VISTASP1)
    ULONG LangGenerationCount;
#else
    /* 4 bytes padding */
#endif
    ULONGLONG Reserved5;
    volatile ULONG64 InterruptTimeBias;
#endif
#if (NTDDI_VERSION >= NTDDI_WIN7)
    volatile ULONG64 TscQpcBias;
    volatile ULONG ActiveProcessorCount;
    volatile USHORT ActiveGroupCount;
    USHORT Reserved4;
    volatile ULONG AitSamplingValue;
    volatile ULONG AppCompatFlag;
    ULONGLONG SystemDllNativeRelocation;
    ULONG SystemDllWowRelocation;
    ULONG XStatePad[1];
    XSTATE_CONFIGURATION XState;
#endif
} KUSER_SHARED_DATA, *PKUSER_SHARED_DATA;

extern NTKERNELAPI PVOID MmHighestUserAddress;
extern NTKERNELAPI PVOID MmSystemRangeStart;
extern NTKERNELAPI ULONG MmUserProbeAddress;


#ifdef _X86_

#define MM_HIGHEST_USER_ADDRESS MmHighestUserAddress
#define MM_SYSTEM_RANGE_START MmSystemRangeStart
#if defined(_LOCAL_COPY_USER_PROBE_ADDRESS_)
#define MM_USER_PROBE_ADDRESS _LOCAL_COPY_USER_PROBE_ADDRESS_
extern ULONG _LOCAL_COPY_USER_PROBE_ADDRESS_;
#else
#define MM_USER_PROBE_ADDRESS MmUserProbeAddress
#endif
#define MM_LOWEST_USER_ADDRESS (PVOID)0x10000
#define MM_KSEG0_BASE       MM_SYSTEM_RANGE_START
#define MM_SYSTEM_SPACE_END 0xFFFFFFFF
#if !defined (_X86PAE_)
#define MM_LOWEST_SYSTEM_ADDRESS (PVOID)0xC0800000
#else
#define MM_LOWEST_SYSTEM_ADDRESS (PVOID)0xC0C00000
#endif

#define KeGetPcr()                      PCR

#define PCR_MINOR_VERSION 1
#define PCR_MAJOR_VERSION 1

typedef struct _KPCR {
  union {
    NT_TIB NtTib;
    struct {
      struct _EXCEPTION_REGISTRATION_RECORD *Used_ExceptionList;
      PVOID Used_StackBase;
      PVOID Spare2;
      PVOID TssCopy;
      ULONG ContextSwitches;
      KAFFINITY SetMemberCopy;
      PVOID Used_Self;
    };
  };
  struct _KPCR *SelfPcr;
  struct _KPRCB *Prcb;
  KIRQL Irql;
  ULONG IRR;
  ULONG IrrActive;
  ULONG IDR;
  PVOID KdVersionBlock;
  struct _KIDTENTRY *IDT;
  struct _KGDTENTRY *GDT;
  struct _KTSS *TSS;
  USHORT MajorVersion;
  USHORT MinorVersion;
  KAFFINITY SetMember;
  ULONG StallScaleFactor;
  UCHAR SpareUnused;
  UCHAR Number;
  UCHAR Spare0;
  UCHAR SecondLevelCacheAssociativity;
  ULONG VdmAlert;
  ULONG KernelReserved[14];
  ULONG SecondLevelCacheSize;
  ULONG HalReserved[16];
} KPCR, *PKPCR;

FORCEINLINE
ULONG
KeGetCurrentProcessorNumber(VOID)
{
    return (ULONG)__readfsbyte(FIELD_OFFSET(KPCR, Number));
}

#endif /* _X86_ */

#ifdef _AMD64_

#define PTI_SHIFT  12L
#define PDI_SHIFT  21L
#define PPI_SHIFT  30L
#define PXI_SHIFT  39L
#define PTE_PER_PAGE 512
#define PDE_PER_PAGE 512
#define PPE_PER_PAGE 512
#define PXE_PER_PAGE 512
#define PTI_MASK_AMD64 (PTE_PER_PAGE - 1)
#define PDI_MASK_AMD64 (PDE_PER_PAGE - 1)
#define PPI_MASK (PPE_PER_PAGE - 1)
#define PXI_MASK (PXE_PER_PAGE - 1)

#define PXE_BASE    0xFFFFF6FB7DBED000ULL
#define PXE_SELFMAP 0xFFFFF6FB7DBEDF68ULL
#define PPE_BASE    0xFFFFF6FB7DA00000ULL
#define PDE_BASE    0xFFFFF6FB40000000ULL
#define PTE_BASE    0xFFFFF68000000000ULL
#define PXE_TOP     0xFFFFF6FB7DBEDFFFULL
#define PPE_TOP     0xFFFFF6FB7DBFFFFFULL
#define PDE_TOP     0xFFFFF6FB7FFFFFFFULL
#define PTE_TOP     0xFFFFF6FFFFFFFFFFULL

#define MM_HIGHEST_USER_ADDRESS           MmHighestUserAddress
#define MM_SYSTEM_RANGE_START             MmSystemRangeStart
#define MM_USER_PROBE_ADDRESS             MmUserProbeAddress
#define MM_LOWEST_USER_ADDRESS   (PVOID)0x10000
#define MM_LOWEST_SYSTEM_ADDRESS (PVOID)0xFFFF080000000000ULL
#define KI_USER_SHARED_DATA       0xFFFFF78000000000ULL

typedef struct _KPCR
{
    _ANONYMOUS_UNION union
    {
        NT_TIB NtTib;
        _ANONYMOUS_STRUCT struct
        {
            union _KGDTENTRY64 *GdtBase;
            struct _KTSS64 *TssBase;
            ULONG64 UserRsp;
            struct _KPCR *Self;
            struct _KPRCB *CurrentPrcb;
            PKSPIN_LOCK_QUEUE LockArray;
            PVOID Used_Self;
        };
    };
    union _KIDTENTRY64 *IdtBase;
    ULONG64 Unused[2];
    KIRQL Irql;
    UCHAR SecondLevelCacheAssociativity;
    UCHAR ObsoleteNumber;
    UCHAR Fill0;
    ULONG Unused0[3];
    USHORT MajorVersion;
    USHORT MinorVersion;
    ULONG StallScaleFactor;
    PVOID Unused1[3];
    ULONG KernelReserved[15];
    ULONG SecondLevelCacheSize;
    ULONG HalReserved[16];
    ULONG Unused2;
    PVOID KdVersionBlock;
    PVOID Unused3;
    ULONG PcrAlign1[24];
} KPCR, *PKPCR;

FORCEINLINE
PKPCR
KeGetPcr(VOID)
{
    return (PKPCR)__readgsqword(FIELD_OFFSET(KPCR, Self));
}

FORCEINLINE
ULONG
KeGetCurrentProcessorNumber(VOID)
{
    return (ULONG)__readgsword(0x184);
}

#endif /* _AMD64_ */

typedef enum _INTERLOCKED_RESULT {
  ResultNegative = RESULT_NEGATIVE,
  ResultZero = RESULT_ZERO,
  ResultPositive = RESULT_POSITIVE
} INTERLOCKED_RESULT;

/* Executive Types */

#define PROTECTED_POOL                    0x80000000

typedef struct _ZONE_SEGMENT_HEADER {
  SINGLE_LIST_ENTRY SegmentList;
  PVOID Reserved;
} ZONE_SEGMENT_HEADER, *PZONE_SEGMENT_HEADER;

typedef struct _ZONE_HEADER {
  SINGLE_LIST_ENTRY FreeList;
  SINGLE_LIST_ENTRY SegmentList;
  ULONG BlockSize;
  ULONG TotalSegmentSize;
} ZONE_HEADER, *PZONE_HEADER;

/* Executive Functions */

static __inline PVOID
ExAllocateFromZone(
  IN PZONE_HEADER Zone)
{
  if (Zone->FreeList.Next)
    Zone->FreeList.Next = Zone->FreeList.Next->Next;
  return (PVOID) Zone->FreeList.Next;
}

static __inline PVOID
ExFreeToZone(
  IN PZONE_HEADER  Zone,
  IN PVOID  Block)
{
  ((PSINGLE_LIST_ENTRY) Block)->Next = Zone->FreeList.Next;
  Zone->FreeList.Next = ((PSINGLE_LIST_ENTRY) Block);
  return ((PSINGLE_LIST_ENTRY) Block)->Next;
}

/*
 * PVOID
 * ExInterlockedAllocateFromZone(
 *   IN PZONE_HEADER  Zone,
 *   IN PKSPIN_LOCK  Lock)
 */
#define ExInterlockedAllocateFromZone(Zone, Lock) \
    ((PVOID) ExInterlockedPopEntryList(&Zone->FreeList, Lock))

/* PVOID
 * ExInterlockedFreeToZone(
 *  IN PZONE_HEADER  Zone,
 *  IN PVOID  Block,
 *  IN PKSPIN_LOCK  Lock);
 */
#define ExInterlockedFreeToZone(Zone, Block, Lock) \
    ExInterlockedPushEntryList(&(Zone)->FreeList, (PSINGLE_LIST_ENTRY)(Block), Lock)

/*
 * BOOLEAN
 * ExIsFullZone(
 *  IN PZONE_HEADER  Zone)
 */
#define ExIsFullZone(Zone) \
  ((Zone)->FreeList.Next == (PSINGLE_LIST_ENTRY) NULL)

/* BOOLEAN
 * ExIsObjectInFirstZoneSegment(
 *     IN PZONE_HEADER Zone,
 *     IN PVOID Object);
 */
#define ExIsObjectInFirstZoneSegment(Zone,Object) \
    ((BOOLEAN)( ((PUCHAR)(Object) >= (PUCHAR)(Zone)->SegmentList.Next) && \
                ((PUCHAR)(Object) <  (PUCHAR)(Zone)->SegmentList.Next + \
                         (Zone)->TotalSegmentSize)) )

#define ExAcquireResourceExclusive ExAcquireResourceExclusiveLite
#define ExAcquireResourceShared ExAcquireResourceSharedLite
#define ExConvertExclusiveToShared ExConvertExclusiveToSharedLite
#define ExDeleteResource ExDeleteResourceLite
#define ExInitializeResource ExInitializeResourceLite
#define ExIsResourceAcquiredExclusive ExIsResourceAcquiredExclusiveLite
#define ExIsResourceAcquiredShared ExIsResourceAcquiredSharedLite
#define ExIsResourceAcquired ExIsResourceAcquiredSharedLite
#define ExReleaseResourceForThread ExReleaseResourceForThreadLite

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTKERNELAPI
NTSTATUS
NTAPI
ExExtendZone(
  IN OUT PZONE_HEADER Zone,
  IN OUT PVOID Segment,
  IN ULONG SegmentSize);

NTKERNELAPI
NTSTATUS
NTAPI
ExInitializeZone(
  OUT PZONE_HEADER Zone,
  IN ULONG BlockSize,
  IN OUT PVOID InitialSegment,
  IN ULONG InitialSegmentSize);

NTKERNELAPI
NTSTATUS
NTAPI
ExInterlockedExtendZone(
  IN OUT PZONE_HEADER Zone,
  IN OUT PVOID Segment,
  IN ULONG SegmentSize,
  IN OUT PKSPIN_LOCK Lock);

NTKERNELAPI
NTSTATUS
NTAPI
ExUuidCreate(
  OUT UUID *Uuid);

NTKERNELAPI
DECLSPEC_NORETURN
VOID
NTAPI
ExRaiseAccessViolation(
  VOID);

NTKERNELAPI
DECLSPEC_NORETURN
VOID
NTAPI
ExRaiseDatatypeMisalignment(
  VOID);

#endif

#ifdef _X86_

NTKERNELAPI
INTERLOCKED_RESULT
FASTCALL
Exfi386InterlockedIncrementLong(
  IN OUT LONG volatile *Addend);

NTKERNELAPI
INTERLOCKED_RESULT
FASTCALL
Exfi386InterlockedDecrementLong(
  IN PLONG  Addend);

NTKERNELAPI
ULONG
FASTCALL
Exfi386InterlockedExchangeUlong(
  IN PULONG  Target,
  IN ULONG  Value);

#endif /* _X86_ */

#ifndef _ARC_DDK_
#define _ARC_DDK_
typedef enum _CONFIGURATION_TYPE {
  ArcSystem,
  CentralProcessor,
  FloatingPointProcessor,
  PrimaryIcache,
  PrimaryDcache,
  SecondaryIcache,
  SecondaryDcache,
  SecondaryCache,
  EisaAdapter,
  TcAdapter,
  ScsiAdapter,
  DtiAdapter,
  MultiFunctionAdapter,
  DiskController,
  TapeController,
  CdromController,
  WormController,
  SerialController,
  NetworkController,
  DisplayController,
  ParallelController,
  PointerController,
  KeyboardController,
  AudioController,
  OtherController,
  DiskPeripheral,
  FloppyDiskPeripheral,
  TapePeripheral,
  ModemPeripheral,
  MonitorPeripheral,
  PrinterPeripheral,
  PointerPeripheral,
  KeyboardPeripheral,
  TerminalPeripheral,
  OtherPeripheral,
  LinePeripheral,
  NetworkPeripheral,
  SystemMemory,
  DockingInformation,
  RealModeIrqRoutingTable,
  RealModePCIEnumeration,
  MaximumType
} CONFIGURATION_TYPE, *PCONFIGURATION_TYPE;
#endif /* !_ARC_DDK_ */

typedef struct _CONTROLLER_OBJECT {
  CSHORT Type;
  CSHORT Size;
  PVOID ControllerExtension;
  KDEVICE_QUEUE DeviceWaitQueue;
  ULONG Spare1;
  LARGE_INTEGER Spare2;
} CONTROLLER_OBJECT, *PCONTROLLER_OBJECT;

typedef struct _CONFIGURATION_INFORMATION {
  ULONG DiskCount;
  ULONG FloppyCount;
  ULONG CdRomCount;
  ULONG TapeCount;
  ULONG ScsiPortCount;
  ULONG SerialCount;
  ULONG ParallelCount;
  BOOLEAN AtDiskPrimaryAddressClaimed;
  BOOLEAN AtDiskSecondaryAddressClaimed;
  ULONG Version;
  ULONG MediumChangerCount;
} CONFIGURATION_INFORMATION, *PCONFIGURATION_INFORMATION;

typedef
NTSTATUS
(NTAPI *PIO_QUERY_DEVICE_ROUTINE)(
  IN PVOID Context,
  IN PUNICODE_STRING PathName,
  IN INTERFACE_TYPE BusType,
  IN ULONG BusNumber,
  IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
  IN CONFIGURATION_TYPE ControllerType,
  IN ULONG ControllerNumber,
  IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
  IN CONFIGURATION_TYPE PeripheralType,
  IN ULONG PeripheralNumber,
  IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation);

typedef
VOID
(NTAPI DRIVER_REINITIALIZE)(
  IN struct _DRIVER_OBJECT *DriverObject,
  IN PVOID Context,
  IN ULONG Count);

typedef DRIVER_REINITIALIZE *PDRIVER_REINITIALIZE;

/** Filesystem runtime library routines **/

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsTotalDeviceFailure(
  IN NTSTATUS Status);
#endif

/* Hardware Abstraction Layer Types */

typedef VOID
(NTAPI *PciPin2Line)(
  IN struct _BUS_HANDLER *BusHandler,
  IN struct _BUS_HANDLER *RootHandler,
  IN PCI_SLOT_NUMBER SlotNumber,
  IN PPCI_COMMON_CONFIG PciData);

typedef VOID
(NTAPI *PciLine2Pin)(
  IN struct _BUS_HANDLER *BusHandler,
  IN struct _BUS_HANDLER *RootHandler,
  IN PCI_SLOT_NUMBER SlotNumber,
  IN PPCI_COMMON_CONFIG PciNewData,
  IN PPCI_COMMON_CONFIG PciOldData);

typedef VOID
(NTAPI *PciReadWriteConfig)(
  IN struct _BUS_HANDLER *BusHandler,
  IN PCI_SLOT_NUMBER Slot,
  IN PVOID Buffer,
  IN ULONG Offset,
  IN ULONG Length);

#define PCI_DATA_TAG ' ICP'
#define PCI_DATA_VERSION 1

typedef struct _PCIBUSDATA {
  ULONG Tag;
  ULONG Version;
  PciReadWriteConfig ReadConfig;
  PciReadWriteConfig WriteConfig;
  PciPin2Line Pin2Line;
  PciLine2Pin Line2Pin;
  PCI_SLOT_NUMBER ParentSlot;
  PVOID Reserved[4];
} PCIBUSDATA, *PPCIBUSDATA;

/* Hardware Abstraction Layer Functions */

#if !defined(NO_LEGACY_DRIVERS)

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTHALAPI
NTSTATUS
NTAPI
HalAssignSlotResources(
  IN PUNICODE_STRING RegistryPath,
  IN PUNICODE_STRING DriverClassName,
  IN PDRIVER_OBJECT DriverObject,
  IN PDEVICE_OBJECT DeviceObject,
  IN INTERFACE_TYPE BusType,
  IN ULONG BusNumber,
  IN ULONG SlotNumber,
  IN OUT PCM_RESOURCE_LIST *AllocatedResources);

NTHALAPI
ULONG
NTAPI
HalGetInterruptVector(
  IN INTERFACE_TYPE InterfaceType,
  IN ULONG BusNumber,
  IN ULONG BusInterruptLevel,
  IN ULONG BusInterruptVector,
  OUT PKIRQL Irql,
  OUT PKAFFINITY Affinity);

NTHALAPI
ULONG
NTAPI
HalSetBusData(
  IN BUS_DATA_TYPE BusDataType,
  IN ULONG BusNumber,
  IN ULONG SlotNumber,
  IN PVOID Buffer,
  IN ULONG Length);

#endif

#endif /* !defined(NO_LEGACY_DRIVERS) */

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTHALAPI
PADAPTER_OBJECT
NTAPI
HalGetAdapter(
  IN PDEVICE_DESCRIPTION DeviceDescription,
  IN OUT PULONG NumberOfMapRegisters);

NTHALAPI
BOOLEAN
NTAPI
HalMakeBeep(
  IN ULONG Frequency);

VOID
NTAPI
HalPutDmaAdapter(
  IN PADAPTER_OBJECT DmaAdapter);

NTHALAPI
VOID
NTAPI
HalAcquireDisplayOwnership(
  IN PHAL_RESET_DISPLAY_PARAMETERS ResetDisplayParameters);

NTHALAPI
ULONG
NTAPI
HalGetBusData(
  IN BUS_DATA_TYPE BusDataType,
  IN ULONG BusNumber,
  IN ULONG SlotNumber,
  OUT PVOID Buffer,
  IN ULONG Length);

NTHALAPI
ULONG
NTAPI
HalGetBusDataByOffset(
  IN BUS_DATA_TYPE BusDataType,
  IN ULONG BusNumber,
  IN ULONG SlotNumber,
  OUT PVOID Buffer,
  IN ULONG Offset,
  IN ULONG Length);

NTHALAPI
ULONG
NTAPI
HalSetBusDataByOffset(
  IN BUS_DATA_TYPE BusDataType,
  IN ULONG BusNumber,
  IN ULONG SlotNumber,
  IN PVOID Buffer,
  IN ULONG Offset,
  IN ULONG Length);

NTHALAPI
BOOLEAN
NTAPI
HalTranslateBusAddress(
  IN INTERFACE_TYPE InterfaceType,
  IN ULONG BusNumber,
  IN PHYSICAL_ADDRESS BusAddress,
  IN OUT PULONG AddressSpace,
  OUT PPHYSICAL_ADDRESS TranslatedAddress);

#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)
NTKERNELAPI
VOID
FASTCALL
HalExamineMBR(
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG SectorSize,
  IN ULONG MBRTypeIdentifier,
  OUT PVOID *Buffer);
#endif

#if defined(USE_DMA_MACROS) && !defined(_NTHAL_) && (defined(_NTDDK_) || defined(_NTDRIVER_)) || defined(_WDM_INCLUDED_) 
// nothing here
#else

#if (NTDDI_VERSION >= NTDDI_WIN2K)
//DECLSPEC_DEPRECATED_DDK
NTHALAPI
VOID
NTAPI
IoFreeAdapterChannel(
  IN PADAPTER_OBJECT AdapterObject);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
BOOLEAN
NTAPI
IoFlushAdapterBuffers(
  IN PADAPTER_OBJECT AdapterObject,
  IN PMDL Mdl,
  IN PVOID MapRegisterBase,
  IN PVOID CurrentVa,
  IN ULONG Length,
  IN BOOLEAN WriteToDevice);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
VOID
NTAPI
IoFreeMapRegisters(
  IN PADAPTER_OBJECT AdapterObject,
  IN PVOID MapRegisterBase,
  IN ULONG NumberOfMapRegisters);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
PVOID
NTAPI
HalAllocateCommonBuffer(
  IN PADAPTER_OBJECT AdapterObject,
  IN ULONG Length,
  OUT PPHYSICAL_ADDRESS LogicalAddress,
  IN BOOLEAN CacheEnabled);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
VOID
NTAPI
HalFreeCommonBuffer(
  IN PADAPTER_OBJECT AdapterObject,
  IN ULONG Length,
  IN PHYSICAL_ADDRESS LogicalAddress,
  IN PVOID VirtualAddress,
  IN BOOLEAN CacheEnabled);

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
ULONG
NTAPI
HalReadDmaCounter(
  IN PADAPTER_OBJECT AdapterObject);

NTHALAPI
NTSTATUS
NTAPI
HalAllocateAdapterChannel(
  IN PADAPTER_OBJECT  AdapterObject,
  IN PWAIT_CONTEXT_BLOCK  Wcb,
  IN ULONG  NumberOfMapRegisters,
  IN PDRIVER_CONTROL  ExecutionRoutine);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#endif /* defined(USE_DMA_MACROS) && !defined(_NTHAL_) && (defined(_NTDDK_) || defined(_NTDRIVER_)) || defined(_WDM_INCLUDED_)  */

/* I/O Manager Functions */

/*
 * VOID IoAssignArcName(
 *   IN PUNICODE_STRING  ArcName,
 *   IN PUNICODE_STRING  DeviceName);
 */
#define IoAssignArcName(_ArcName, _DeviceName) ( \
  IoCreateSymbolicLink((_ArcName), (_DeviceName)))

/*
 * VOID
 * IoDeassignArcName(
 *   IN PUNICODE_STRING  ArcName)
 */
#define IoDeassignArcName IoDeleteSymbolicLink

#if (NTDDI_VERSION >= NTDDI_WIN2K)

#if !(defined(USE_DMA_MACROS) && (defined(_NTDDK_) || defined(_NTDRIVER_)) || defined(_WDM_INCLUDED_))
NTKERNELAPI
NTSTATUS
NTAPI
IoAllocateAdapterChannel(
  IN PADAPTER_OBJECT AdapterObject,
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG NumberOfMapRegisters,
  IN PDRIVER_CONTROL ExecutionRoutine,
  IN PVOID Context);
#endif

//DECLSPEC_DEPRECATED_DDK
NTHALAPI
PHYSICAL_ADDRESS
NTAPI
IoMapTransfer(
  IN PADAPTER_OBJECT AdapterObject,
  IN PMDL Mdl,
  IN PVOID MapRegisterBase,
  IN PVOID CurrentVa,
  IN OUT PULONG Length,
  IN BOOLEAN WriteToDevice);

NTKERNELAPI
VOID
NTAPI
IoAllocateController(
  IN PCONTROLLER_OBJECT ControllerObject,
  IN PDEVICE_OBJECT DeviceObject,
  IN PDRIVER_CONTROL ExecutionRoutine,
  IN PVOID Context OPTIONAL);

NTKERNELAPI
PCONTROLLER_OBJECT
NTAPI
IoCreateController(
  IN ULONG Size);

NTKERNELAPI
VOID
NTAPI
IoDeleteController(
  IN PCONTROLLER_OBJECT ControllerObject);

NTKERNELAPI
VOID
NTAPI
IoFreeController(
  IN PCONTROLLER_OBJECT ControllerObject);

NTKERNELAPI
PCONFIGURATION_INFORMATION
NTAPI
IoGetConfigurationInformation(
  VOID);

NTKERNELAPI
PDEVICE_OBJECT
NTAPI
IoGetDeviceToVerify(
  IN PETHREAD Thread);

NTKERNELAPI
VOID
NTAPI
IoCancelFileOpen(
  IN PDEVICE_OBJECT DeviceObject,
  IN PFILE_OBJECT FileObject);

NTKERNELAPI
PGENERIC_MAPPING
NTAPI
IoGetFileObjectGenericMapping(
  VOID);

NTKERNELAPI
PIRP
NTAPI
IoMakeAssociatedIrp(
  IN PIRP Irp,
  IN CCHAR StackSize);

NTKERNELAPI
NTSTATUS
NTAPI
IoQueryDeviceDescription(
  IN PINTERFACE_TYPE BusType OPTIONAL,
  IN PULONG BusNumber OPTIONAL,
  IN PCONFIGURATION_TYPE ControllerType OPTIONAL,
  IN PULONG ControllerNumber OPTIONAL,
  IN PCONFIGURATION_TYPE PeripheralType OPTIONAL,
  IN PULONG PeripheralNumber OPTIONAL,
  IN PIO_QUERY_DEVICE_ROUTINE CalloutRoutine,
  IN OUT PVOID Context OPTIONAL);

NTKERNELAPI
VOID
NTAPI
IoRaiseHardError(
  IN PIRP Irp,
  IN PVPB Vpb OPTIONAL,
  IN PDEVICE_OBJECT RealDeviceObject);

NTKERNELAPI
BOOLEAN
NTAPI
IoRaiseInformationalHardError(
  IN NTSTATUS ErrorStatus,
  IN PUNICODE_STRING String OPTIONAL,
  IN PKTHREAD Thread OPTIONAL);

NTKERNELAPI
VOID
NTAPI
IoRegisterBootDriverReinitialization(
  IN PDRIVER_OBJECT DriverObject,
  IN PDRIVER_REINITIALIZE DriverReinitializationRoutine,
  IN PVOID Context OPTIONAL);

NTKERNELAPI
VOID
NTAPI
IoRegisterDriverReinitialization(
  IN PDRIVER_OBJECT DriverObject,
  IN PDRIVER_REINITIALIZE DriverReinitializationRoutine,
  IN PVOID Context OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
IoAttachDeviceByPointer(
  IN PDEVICE_OBJECT SourceDevice,
  IN PDEVICE_OBJECT TargetDevice);

NTKERNELAPI
NTSTATUS
NTAPI
IoReportDetectedDevice(
  IN PDRIVER_OBJECT DriverObject,
  IN INTERFACE_TYPE LegacyBusType,
  IN ULONG BusNumber,
  IN ULONG SlotNumber,
  IN PCM_RESOURCE_LIST ResourceList OPTIONAL,
  IN PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirements OPTIONAL,
  IN BOOLEAN ResourceAssigned,
  IN OUT PDEVICE_OBJECT *DeviceObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoReportResourceForDetection(
  IN PDRIVER_OBJECT DriverObject,
  IN PCM_RESOURCE_LIST DriverList OPTIONAL,
  IN ULONG DriverListSize OPTIONAL,
  IN PDEVICE_OBJECT DeviceObject OPTIONAL,
  IN PCM_RESOURCE_LIST DeviceList OPTIONAL,
  IN ULONG DeviceListSize OPTIONAL,
  OUT PBOOLEAN ConflictDetected);

NTKERNELAPI
NTSTATUS
NTAPI
IoReportResourceUsage(
  IN PUNICODE_STRING DriverClassName OPTIONAL,
  IN PDRIVER_OBJECT DriverObject,
  IN PCM_RESOURCE_LIST DriverList OPTIONAL,
  IN ULONG DriverListSize OPTIONAL,
  IN PDEVICE_OBJECT DeviceObject,
  IN PCM_RESOURCE_LIST DeviceList OPTIONAL,
  IN ULONG DeviceListSize OPTIONAL,
  IN BOOLEAN OverrideConflict,
  OUT PBOOLEAN ConflictDetected);

NTKERNELAPI
VOID
NTAPI
IoSetHardErrorOrVerifyDevice(
  IN PIRP Irp,
  IN PDEVICE_OBJECT DeviceObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoAssignResources(
  IN PUNICODE_STRING RegistryPath,
  IN PUNICODE_STRING DriverClassName OPTIONAL,
  IN PDRIVER_OBJECT DriverObject,
  IN PDEVICE_OBJECT DeviceObject OPTIONAL,
  IN PIO_RESOURCE_REQUIREMENTS_LIST RequestedResources OPTIONAL,
  IN OUT PCM_RESOURCE_LIST *AllocatedResources);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_WINXP)

NTKERNELAPI
NTSTATUS
NTAPI
IoCreateDisk(
  IN PDEVICE_OBJECT DeviceObject,
  IN struct _CREATE_DISK* Disk OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
IoReadDiskSignature(
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG BytesPerSector,
  OUT PDISK_SIGNATURE Signature);

NTKERNELAPI
NTSTATUS
FASTCALL
IoReadPartitionTable(
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG SectorSize,
  IN BOOLEAN ReturnRecognizedPartitions,
  OUT struct _DRIVE_LAYOUT_INFORMATION **PartitionBuffer);

NTKERNELAPI
NTSTATUS
NTAPI
IoReadPartitionTableEx(
  IN PDEVICE_OBJECT DeviceObject,
  IN struct _DRIVE_LAYOUT_INFORMATION_EX **PartitionBuffer);

NTKERNELAPI
NTSTATUS
FASTCALL
IoSetPartitionInformation(
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG SectorSize,
  IN ULONG PartitionNumber,
  IN ULONG PartitionType);

NTKERNELAPI
NTSTATUS
NTAPI
IoSetPartitionInformationEx(
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG PartitionNumber,
  IN struct _SET_PARTITION_INFORMATION_EX *PartitionInfo);

NTKERNELAPI
NTSTATUS
NTAPI
IoSetSystemPartition(
  IN PUNICODE_STRING VolumeNameString);

NTKERNELAPI
BOOLEAN
NTAPI
IoSetThreadHardErrorMode(
  IN BOOLEAN EnableHardErrors);

NTKERNELAPI
NTSTATUS
NTAPI
IoVerifyPartitionTable(
  IN PDEVICE_OBJECT DeviceObject,
  IN BOOLEAN FixErrors);

NTKERNELAPI
NTSTATUS
NTAPI
IoVolumeDeviceToDosName(
  IN PVOID VolumeDeviceObject,
  OUT PUNICODE_STRING DosName);

NTKERNELAPI
NTSTATUS
FASTCALL
IoWritePartitionTable(
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG SectorSize,
  IN ULONG SectorsPerTrack,
  IN ULONG NumberOfHeads,
  IN struct _DRIVE_LAYOUT_INFORMATION *PartitionBuffer);

NTKERNELAPI
NTSTATUS
NTAPI
IoWritePartitionTableEx(
  IN PDEVICE_OBJECT DeviceObject,
  IN struct _DRIVE_LAYOUT_INFORMATION_EX *DriveLayout);

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

/* Kernel Functions */

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTKERNELAPI
DECLSPEC_NORETURN
VOID
NTAPI
KeBugCheck(
  IN ULONG BugCheckCode);

NTKERNELAPI
LONG
NTAPI
KePulseEvent(
  IN OUT PRKEVENT Event,
  IN KPRIORITY Increment,
  IN BOOLEAN Wait);

NTKERNELAPI
LONG
NTAPI
KeSetBasePriorityThread(
  IN OUT PRKTHREAD Thread,
  IN LONG Increment);

#endif

/* Memory Manager Types */

typedef struct _PHYSICAL_MEMORY_RANGE {
  PHYSICAL_ADDRESS BaseAddress;
  LARGE_INTEGER NumberOfBytes;
} PHYSICAL_MEMORY_RANGE, *PPHYSICAL_MEMORY_RANGE;

/* Memory Manager Functions */

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTKERNELAPI
PPHYSICAL_MEMORY_RANGE
NTAPI
MmGetPhysicalMemoryRanges(
  VOID);

NTKERNELAPI
PHYSICAL_ADDRESS
NTAPI
MmGetPhysicalAddress(
  IN PVOID BaseAddress);

NTKERNELAPI
BOOLEAN
NTAPI
MmIsNonPagedSystemAddressValid(
  IN PVOID VirtualAddress);

NTKERNELAPI
PVOID
NTAPI
MmAllocateNonCachedMemory(
  IN SIZE_T NumberOfBytes);

NTKERNELAPI
VOID
NTAPI
MmFreeNonCachedMemory(
  IN PVOID BaseAddress,
  IN SIZE_T NumberOfBytes);

NTKERNELAPI
PVOID
NTAPI
MmGetVirtualForPhysical(
  IN PHYSICAL_ADDRESS PhysicalAddress);

NTKERNELAPI
NTSTATUS
NTAPI
MmMapUserAddressesToPage(
  IN PVOID BaseAddress,
  IN SIZE_T NumberOfBytes,
  IN PVOID PageAddress);

NTKERNELAPI
PVOID
NTAPI
MmMapVideoDisplay(
  IN PHYSICAL_ADDRESS PhysicalAddress,
  IN SIZE_T NumberOfBytes,
  IN MEMORY_CACHING_TYPE CacheType);

NTKERNELAPI
NTSTATUS
NTAPI
MmMapViewInSessionSpace(
  IN PVOID Section,
  OUT PVOID *MappedBase,
  IN OUT PSIZE_T ViewSize);

NTKERNELAPI
NTSTATUS
NTAPI
MmMapViewInSystemSpace(
  IN PVOID Section,
  OUT PVOID *MappedBase,
  IN OUT PSIZE_T ViewSize);

NTKERNELAPI
BOOLEAN
NTAPI
MmIsAddressValid(
  IN PVOID VirtualAddress);

NTKERNELAPI
BOOLEAN
NTAPI
MmIsThisAnNtAsSystem(
  VOID);

NTKERNELAPI
VOID
NTAPI
MmLockPagableSectionByHandle(
  IN PVOID ImageSectionHandle);

NTKERNELAPI
NTSTATUS
NTAPI
MmUnmapViewInSessionSpace(
  IN PVOID MappedBase);

NTKERNELAPI
NTSTATUS
NTAPI
MmUnmapViewInSystemSpace(
  IN PVOID MappedBase);

NTKERNELAPI
VOID
NTAPI
MmUnsecureVirtualMemory(
  IN HANDLE SecureHandle);

NTKERNELAPI
NTSTATUS
NTAPI
MmRemovePhysicalMemory(
  IN PPHYSICAL_ADDRESS StartAddress,
  IN OUT PLARGE_INTEGER NumberOfBytes);

NTKERNELAPI
HANDLE
NTAPI
MmSecureVirtualMemory(
  IN PVOID Address,
  IN SIZE_T Size,
  IN ULONG ProbeMode);

NTKERNELAPI
VOID
NTAPI
MmUnmapVideoDisplay(
  IN PVOID BaseAddress,
  IN SIZE_T NumberOfBytes);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

/** Process manager types **/

typedef VOID
(NTAPI *PCREATE_PROCESS_NOTIFY_ROUTINE)(
  IN HANDLE ParentId,
  IN HANDLE ProcessId,
  IN BOOLEAN Create);

typedef VOID
(NTAPI *PCREATE_THREAD_NOTIFY_ROUTINE)(
  IN HANDLE ProcessId,
  IN HANDLE ThreadId,
  IN BOOLEAN Create);

typedef VOID
(NTAPI *PLOAD_IMAGE_NOTIFY_ROUTINE)(
  IN PUNICODE_STRING FullImageName,
  IN HANDLE ProcessId,
  IN PIMAGE_INFO ImageInfo);

/** Process manager routines **/

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTKERNELAPI
NTSTATUS
NTAPI
PsSetLoadImageNotifyRoutine(
  IN PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine);

NTKERNELAPI
NTSTATUS
NTAPI
PsSetCreateThreadNotifyRoutine(
  IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine);

NTKERNELAPI
NTSTATUS
NTAPI
PsSetCreateProcessNotifyRoutine(
  IN PCREATE_PROCESS_NOTIFY_ROUTINE NotifyRoutine,
  IN BOOLEAN Remove);

NTKERNELAPI
HANDLE
NTAPI
PsGetCurrentProcessId(
  VOID);

NTKERNELAPI
HANDLE
NTAPI
PsGetCurrentThreadId(
  VOID);

NTKERNELAPI
BOOLEAN
NTAPI
PsGetVersion(
  OUT PULONG MajorVersion OPTIONAL,
  OUT PULONG MinorVersion OPTIONAL,
  OUT PULONG BuildNumber OPTIONAL,
  OUT PUNICODE_STRING CSDVersion OPTIONAL);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_WINXP)

NTKERNELAPI
HANDLE
NTAPI
PsGetProcessId(
  IN PEPROCESS Process);

NTKERNELAPI
NTSTATUS
NTAPI
PsRemoveCreateThreadNotifyRoutine(
  IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine);

NTKERNELAPI
NTSTATUS
NTAPI
PsRemoveLoadImageNotifyRoutine(
  IN PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine);

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

extern NTKERNELAPI PEPROCESS PsInitialSystemProcess;

/* Security reference monitor routines */

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTKERNELAPI
BOOLEAN
NTAPI
SeSinglePrivilegeCheck(
  IN LUID PrivilegeValue,
  IN KPROCESSOR_MODE PreviousMode);
#endif

/* ZwXxx Functions */

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTSTATUS
NTAPI
ZwCancelTimer(
  IN HANDLE TimerHandle,
  OUT PBOOLEAN CurrentState OPTIONAL);

NTSTATUS
NTAPI
ZwCreateTimer(
  OUT PHANDLE TimerHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
  IN TIMER_TYPE TimerType);

NTSTATUS
NTAPI
ZwOpenTimer(
  OUT PHANDLE TimerHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationThread(
  IN HANDLE ThreadHandle,
  IN THREADINFOCLASS ThreadInformationClass,
  IN PVOID ThreadInformation,
  IN ULONG ThreadInformationLength);

NTSTATUS
NTAPI
ZwSetTimer(
  IN HANDLE TimerHandle,
  IN PLARGE_INTEGER DueTime,
  IN PTIMER_APC_ROUTINE TimerApcRoutine OPTIONAL,
  IN PVOID TimerContext OPTIONAL,
  IN BOOLEAN ResumeTimer,
  IN LONG Period OPTIONAL,
  OUT PBOOLEAN PreviousState OPTIONAL);

#endif

NTSYSAPI
NTSTATUS
NTAPI
ZwDisplayString (
    IN PUNICODE_STRING String
);

#if (VER_PRODUCTBUILD >= 2195)

NTSYSAPI
NTSTATUS
NTAPI
ZwPowerInformation (
    IN POWER_INFORMATION_LEVEL  PowerInformationLevel,
    IN PVOID                    InputBuffer OPTIONAL,
    IN ULONG                    InputBufferLength,
    OUT PVOID                   OutputBuffer OPTIONAL,
    IN ULONG                    OutputBufferLength
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwTerminateProcess (
    IN HANDLE   ProcessHandle OPTIONAL,
    IN NTSTATUS ExitStatus
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcess (
    OUT PHANDLE             ProcessHandle,
    IN ACCESS_MASK          DesiredAccess,
    IN POBJECT_ATTRIBUTES   ObjectAttributes,
    IN PCLIENT_ID           ClientId OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryVolumeInformationFile(
  IN HANDLE FileHandle,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  OUT PVOID FsInformation,
  IN ULONG Length,
  IN FS_INFORMATION_CLASS FsInformationClass);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeviceIoControlFile(
  IN HANDLE FileHandle,
  IN HANDLE Event OPTIONAL,
  IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
  IN PVOID ApcContext OPTIONAL,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  IN ULONG IoControlCode,
  IN PVOID InputBuffer OPTIONAL,
  IN ULONG InputBufferLength,
  OUT PVOID OutputBuffer OPTIONAL,
  IN ULONG OutputBufferLength);

#ifdef __cplusplus
}
#endif
