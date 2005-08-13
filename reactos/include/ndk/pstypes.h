/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/pstypes.h
 * PURPOSE:         Defintions for Process Manager Types not documented in DDK/IFS.
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _PSTYPES_H
#define _PSTYPES_H

/* DEPENDENCIES **************************************************************/
#include "ldrtypes.h"
#include "rtltypes.h"

/* EXPORTED DATA *************************************************************/

extern NTOSAPI struct _EPROCESS* PsInitialSystemProcess;
extern NTOSAPI POBJECT_TYPE PsProcessType;
extern NTOSAPI POBJECT_TYPE PsThreadType;

/* CONSTANTS *****************************************************************/

/* These are not exposed to drivers normally */
#ifndef _NTOS_MODE_USER
    #define JOB_OBJECT_ASSIGN_PROCESS    (1)
    #define JOB_OBJECT_SET_ATTRIBUTES    (2)
    #define JOB_OBJECT_QUERY    (4)
    #define JOB_OBJECT_TERMINATE    (8)
    #define JOB_OBJECT_SET_SECURITY_ATTRIBUTES    (16)
    #define JOB_OBJECT_ALL_ACCESS    (STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|31)
#endif

/* FIXME: This was changed in XP... Ask ThomasW about it */
#define PROCESS_SET_PORT 0x800

#define THREAD_ALERT 0x4

#define USER_SHARED_DATA (0x7FFE0000)

/* Process priority classes */
#define PROCESS_PRIORITY_CLASS_HIGH	(4) /* FIXME */
#define PROCESS_PRIORITY_CLASS_IDLE	(0) /* FIXME */
#define PROCESS_PRIORITY_CLASS_NORMAL	(2) /* FIXME */
#define PROCESS_PRIORITY_CLASS_REALTIME	(5) /* FIXME */
#define PROCESS_PRIORITY_CLASS_BELOW_NORMAL (1) /* FIXME */
#define PROCESS_PRIORITY_CLASS_ABOVE_NORMAL (3) /* FIXME */

/* Global Flags */
#define FLG_STOP_ON_EXCEPTION          0x00000001
#define FLG_SHOW_LDR_SNAPS             0x00000002
#define FLG_DEBUG_INITIAL_COMMAND      0x00000004
#define FLG_STOP_ON_HUNG_GUI           0x00000008
#define FLG_HEAP_ENABLE_TAIL_CHECK     0x00000010
#define FLG_HEAP_ENABLE_FREE_CHECK     0x00000020
#define FLG_HEAP_VALIDATE_PARAMETERS   0x00000040
#define FLG_HEAP_VALIDATE_ALL          0x00000080
#define FLG_POOL_ENABLE_TAIL_CHECK     0x00000100
#define FLG_POOL_ENABLE_FREE_CHECK     0x00000200
#define FLG_POOL_ENABLE_TAGGING        0x00000400
#define FLG_HEAP_ENABLE_TAGGING        0x00000800
#define FLG_USER_STACK_TRACE_DB        0x00001000
#define FLG_KERNEL_STACK_TRACE_DB      0x00002000
#define FLG_MAINTAIN_OBJECT_TYPELIST   0x00004000
#define FLG_HEAP_ENABLE_TAG_BY_DLL     0x00008000
#define FLG_IGNORE_DEBUG_PRIV          0x00010000
#define FLG_ENABLE_CSRDEBUG            0x00020000
#define FLG_ENABLE_KDEBUG_SYMBOL_LOAD  0x00040000
#define FLG_DISABLE_PAGE_KERNEL_STACKS 0x00080000
#define FLG_HEAP_ENABLE_CALL_TRACING   0x00100000
#define FLG_HEAP_DISABLE_COALESCING    0x00200000
#define FLG_ENABLE_CLOSE_EXCEPTIONS    0x00400000
#define FLG_ENABLE_EXCEPTION_LOGGING   0x00800000
#define FLG_ENABLE_HANDLE_TYPE_TAGGING 0x01000000
#define FLG_HEAP_PAGE_ALLOCS           0x02000000
#define FLG_DEBUG_INITIAL_COMMAND_EX   0x04000000

/* ENUMERATIONS **************************************************************/

/* FUNCTION TYPES ************************************************************/
typedef VOID (STDCALL *PPEBLOCKROUTINE)(PVOID);

typedef NTSTATUS 
(STDCALL *PW32_PROCESS_CALLBACK)(
    struct _EPROCESS *Process,
    BOOLEAN Create
);

typedef NTSTATUS
(STDCALL *PW32_THREAD_CALLBACK)(
    struct _ETHREAD *Thread,
    BOOLEAN Create
);

/* TYPES *********************************************************************/

struct _ETHREAD;
typedef struct _W32PROCESS *PW32PROCESS;
typedef struct _W32THREAD *PW32THREAD;

typedef struct _CURDIR 
{
    UNICODE_STRING DosPath;
    PVOID Handle;
} CURDIR, *PCURDIR;

typedef struct _DESCRIPTOR_TABLE_ENTRY
{
    ULONG Selector;
    LDT_ENTRY Descriptor;
} DESCRIPTOR_TABLE_ENTRY, *PDESCRIPTOR_TABLE_ENTRY;

typedef struct _PEB_FREE_BLOCK 
{
    struct _PEB_FREE_BLOCK* Next;
    ULONG Size;
} PEB_FREE_BLOCK, *PPEB_FREE_BLOCK;

typedef struct _PEB
{
    UCHAR InheritedAddressSpace;                     /* 00h */
    UCHAR ReadImageFileExecOptions;                  /* 01h */
    UCHAR BeingDebugged;                             /* 02h */
    BOOLEAN SpareBool;                               /* 03h */
    HANDLE Mutant;                                   /* 04h */
    PVOID ImageBaseAddress;                          /* 08h */
    PPEB_LDR_DATA Ldr;                               /* 0Ch */
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;  /* 10h */
    PVOID SubSystemData;                             /* 14h */
    PVOID ProcessHeap;                               /* 18h */
    PVOID FastPebLock;                               /* 1Ch */
    PPEBLOCKROUTINE FastPebLockRoutine;              /* 20h */
    PPEBLOCKROUTINE FastPebUnlockRoutine;            /* 24h */
    ULONG EnvironmentUpdateCount;                    /* 28h */
    PVOID* KernelCallbackTable;                      /* 2Ch */
    PVOID EventLogSection;                           /* 30h */
    PVOID EventLog;                                  /* 34h */
    PPEB_FREE_BLOCK FreeList;                        /* 38h */
    ULONG TlsExpansionCounter;                       /* 3Ch */
    PVOID TlsBitmap;                                 /* 40h */
    ULONG TlsBitmapBits[0x2];                        /* 44h */
    PVOID ReadOnlySharedMemoryBase;                  /* 4Ch */
    PVOID ReadOnlySharedMemoryHeap;                  /* 50h */
    PVOID* ReadOnlyStaticServerData;                 /* 54h */
    PVOID AnsiCodePageData;                          /* 58h */
    PVOID OemCodePageData;                           /* 5Ch */
    PVOID UnicodeCaseTableData;                      /* 60h */
    ULONG NumberOfProcessors;                        /* 64h */
    ULONG NtGlobalFlag;                              /* 68h */
    LARGE_INTEGER CriticalSectionTimeout;            /* 70h */
    ULONG HeapSegmentReserve;                        /* 78h */
    ULONG HeapSegmentCommit;                         /* 7Ch */
    ULONG HeapDeCommitTotalFreeThreshold;            /* 80h */
    ULONG HeapDeCommitFreeBlockThreshold;            /* 84h */
    ULONG NumberOfHeaps;                             /* 88h */
    ULONG MaximumNumberOfHeaps;                      /* 8Ch */
    PVOID* ProcessHeaps;                             /* 90h */
    PVOID GdiSharedHandleTable;                      /* 94h */
    PVOID ProcessStarterHelper;                      /* 98h */
    PVOID GdiDCAttributeList;                        /* 9Ch */
    PVOID LoaderLock;                                /* A0h */
    ULONG OSMajorVersion;                            /* A4h */
    ULONG OSMinorVersion;                            /* A8h */
    USHORT OSBuildNumber;                            /* ACh */
    USHORT OSCSDVersion;                             /* AEh */
    ULONG OSPlatformId;                              /* B0h */
    ULONG ImageSubSystem;                            /* B4h */
    ULONG ImageSubSystemMajorVersion;                /* B8h */
    ULONG ImageSubSystemMinorVersion;                /* BCh */
    ULONG ImageProcessAffinityMask;                  /* C0h */
    ULONG GdiHandleBuffer[0x22];                     /* C4h */
    PVOID PostProcessInitRoutine;                    /* 14Ch */
    PVOID *TlsExpansionBitmap;                       /* 150h */
    ULONG TlsExpansionBitmapBits[0x20];              /* 154h */
    ULONG SessionId;                                 /* 1D4h */
    PVOID AppCompatInfo;                             /* 1D8h */
    UNICODE_STRING CSDVersion;                       /* 1DCh */
} PEB;

typedef struct _GDI_TEB_BATCH 
{
    ULONG Offset;
    ULONG HDC;
    ULONG Buffer[0x136];
} GDI_TEB_BATCH, *PGDI_TEB_BATCH;

typedef struct _INITIAL_TEB
{
  PVOID StackBase;
  PVOID StackLimit;
  PVOID StackCommit;
  PVOID StackCommitMax;
  PVOID StackReserved;
} INITIAL_TEB, *PINITIAL_TEB;

typedef struct _TEB 
{
    NT_TIB Tib;                         /* 00h */
    PVOID EnvironmentPointer;           /* 1Ch */
    CLIENT_ID Cid;                      /* 20h */
    PVOID ActiveRpcInfo;                /* 28h */
    PVOID ThreadLocalStoragePointer;    /* 2Ch */
    struct _PEB *Peb;                   /* 30h */
    ULONG LastErrorValue;               /* 34h */
    ULONG CountOfOwnedCriticalSections; /* 38h */
    PVOID CsrClientThread;              /* 3Ch */
    struct _W32THREAD* Win32ThreadInfo; /* 40h */
    ULONG Win32ClientInfo[0x1F];        /* 44h */
    PVOID WOW32Reserved;                /* C0h */
    LCID CurrentLocale;                 /* C4h */
    ULONG FpSoftwareStatusRegister;     /* C8h */
    PVOID SystemReserved1[0x36];        /* CCh */
    PVOID Spare1;                       /* 1A4h */
    LONG ExceptionCode;                 /* 1A8h */
    UCHAR SpareBytes1[0x28];            /* 1ACh */
    PVOID SystemReserved2[0xA];         /* 1D4h */
    GDI_TEB_BATCH GdiTebBatch;          /* 1FCh */
    ULONG gdiRgn;                       /* 6DCh */
    ULONG gdiPen;                       /* 6E0h */
    ULONG gdiBrush;                     /* 6E4h */
    CLIENT_ID RealClientId;             /* 6E8h */
    PVOID GdiCachedProcessHandle;       /* 6F0h */
    ULONG GdiClientPID;                 /* 6F4h */
    ULONG GdiClientTID;                 /* 6F8h */
    PVOID GdiThreadLocaleInfo;          /* 6FCh */
    PVOID UserReserved[5];              /* 700h */
    PVOID glDispatchTable[0x118];       /* 714h */
    ULONG glReserved1[0x1A];            /* B74h */
    PVOID glReserved2;                  /* BDCh */
    PVOID glSectionInfo;                /* BE0h */
    PVOID glSection;                    /* BE4h */
    PVOID glTable;                      /* BE8h */
    PVOID glCurrentRC;                  /* BECh */
    PVOID glContext;                    /* BF0h */
    NTSTATUS LastStatusValue;           /* BF4h */
    UNICODE_STRING StaticUnicodeString; /* BF8h */
    WCHAR StaticUnicodeBuffer[0x105];   /* C00h */
    PVOID DeallocationStack;            /* E0Ch */
    PVOID TlsSlots[0x40];               /* E10h */
    LIST_ENTRY TlsLinks;                /* F10h */
    PVOID Vdm;                          /* F18h */
    PVOID ReservedForNtRpc;             /* F1Ch */
    PVOID DbgSsReserved[0x2];           /* F20h */
    ULONG HardErrorDisabled;            /* F28h */
    PVOID Instrumentation[0x10];        /* F2Ch */
    PVOID WinSockData;                  /* F6Ch */
    ULONG GdiBatchCount;                /* F70h */
    USHORT _Spare2;                     /* F74h */
    BOOLEAN IsFiber;                    /* F76h */
    UCHAR Spare3;                       /* F77h */
    ULONG _Spare4;                      /* F78h */
    ULONG _Spare5;                      /* F7Ch */
    PVOID ReservedForOle;               /* F80h */
    ULONG WaitingOnLoaderLock;          /* F84h */
    ULONG _Unknown[11];                 /* F88h */
    PVOID FlsSlots;                     /* FB4h */
    PVOID WineDebugInfo;                /* Needed for WINE DLL's  */
} TEB, *PTEB;
            
#endif
