/*
 * Internal NT APIs and data structures
 *
 * Copyright (C) the Wine project
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_WINTERNL_H
#define __WINE_WINTERNL_H

#ifndef __REACTOS__
#include <ntdef.h>
#endif /* __REACTOS__ */
#include <windef.h>

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#ifdef __REACTOS__
typedef enum _TIMER_TYPE {
    NotificationTimer,
    SynchronizationTimer
} TIMER_TYPE;
typedef enum _EVENT_TYPE {
  NotificationEvent,
  SynchronizationEvent
} EVENT_TYPE, *PEVENT_TYPE;
#define FSCTL_PIPE_LISTEN CTL_CODE(FILE_DEVICE_NAMED_PIPE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define DUPLICATE_SAME_ATTRIBUTES 0x00000004

#define THREAD_SET_LIMITED_INFORMATION 0x0400

#ifndef _NTDEF_
typedef struct _RTL_BALANCED_NODE
{
    union
    {
        struct _RTL_BALANCED_NODE *Children[2];
        struct
        {
            struct _RTL_BALANCED_NODE *Left;
            struct _RTL_BALANCED_NODE *Right;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
#define RTL_BALANCED_NODE_RESERVED_PARENT_MASK 3
    union
    {
        UCHAR Red : 1;
        UCHAR Balance : 2;
        ULONG_PTR ParentValue;
    } DUMMYUNIONNAME2;
} RTL_BALANCED_NODE, *PRTL_BALANCED_NODE;
#endif // _NTDEF_

typedef struct _CONTEXT_CHUNK
{
    LONG Offset;
    ULONG Length;
} CONTEXT_CHUNK, *PCONTEXT_CHUNK;

typedef struct _CONTEXT_EX
{
    CONTEXT_CHUNK All;
    CONTEXT_CHUNK Legacy;
    CONTEXT_CHUNK XState;
#ifdef _WIN64
    ULONG64 align;
#endif
} CONTEXT_EX, *PCONTEXT_EX;

typedef struct _RTL_RB_TREE
{
    RTL_BALANCED_NODE *root;
    RTL_BALANCED_NODE *min;
} RTL_RB_TREE, *PRTL_RB_TREE;

#endif /* __REACTOS__ */

/**********************************************************************
 * Fundamental types and data structures
 */

#ifndef WINE_NTSTATUS_DECLARED
#define WINE_NTSTATUS_DECLARED
typedef LONG NTSTATUS;
#ifdef __REACTOS__
typedef NTSTATUS *PNTSTATUS;
#endif /* __REACTOS__ */
#endif

typedef const char *PCSZ;

typedef short CSHORT;
typedef CSHORT *PCSHORT;

#ifndef __STRING_DEFINED__
#define __STRING_DEFINED__
typedef struct _STRING {
  USHORT Length;
  USHORT MaximumLength;
  PCHAR Buffer;
} STRING, *PSTRING;
#endif

typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;
typedef const STRING *PCANSI_STRING;

typedef STRING OEM_STRING;
typedef PSTRING POEM_STRING;
typedef const STRING *PCOEM_STRING;

#ifndef __UNICODE_STRING_DEFINED__
#define __UNICODE_STRING_DEFINED__
typedef struct _UNICODE_STRING {
  USHORT Length;        /* bytes */
  USHORT MaximumLength; /* bytes */
  PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
#endif

typedef const UNICODE_STRING *PCUNICODE_STRING;

#ifndef _FILETIME_
#define _FILETIME_
/* 64 bit number of 100 nanoseconds intervals since January 1, 1601 */
typedef struct _FILETIME
{
  DWORD  dwLowDateTime;
  DWORD  dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;
#endif /* _FILETIME_ */

/*
 * RTL_SYSTEM_TIME and RTL_TIME_ZONE_INFORMATION are the same as
 * the SYSTEMTIME and TIME_ZONE_INFORMATION structures defined
 * in winbase.h, however we need to define them separately so
 * winternl.h doesn't depend on winbase.h.  They are used by
 * RtlQueryTimeZoneInformation and RtlSetTimeZoneInformation.
 * The names are guessed; if anybody knows the real names, let me know.
 */
typedef struct _RTL_SYSTEM_TIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} RTL_SYSTEM_TIME, *PRTL_SYSTEM_TIME;

#ifdef __REACTOS__
typedef struct _TIME_FIELDS {
    CSHORT Year;
    CSHORT Month;
    CSHORT Day;
    CSHORT Hour;
    CSHORT Minute;
    CSHORT Second;
    CSHORT Milliseconds;
    CSHORT Weekday;
} TIME_FIELDS, *PTIME_FIELDS;
#endif // __REACTOS__

typedef struct _RTL_TIME_ZONE_INFORMATION {
    LONG Bias;
    WCHAR StandardName[32];
#ifdef __REACTOS__
    TIME_FIELDS StandardDate;
#else
    RTL_SYSTEM_TIME StandardDate;
#endif
    LONG StandardBias;
    WCHAR DaylightName[32];
#ifdef __REACTOS__
    TIME_FIELDS DaylightDate;
#else
    RTL_SYSTEM_TIME DaylightDate;
#endif
    LONG DaylightBias;
} RTL_TIME_ZONE_INFORMATION, *PRTL_TIME_ZONE_INFORMATION;

typedef struct _RTL_TIME_DYNAMIC_ZONE_INFORMATION
{
    LONG Bias;
    WCHAR StandardName[32];
#ifdef __REACTOS__
    TIME_FIELDS StandardDate;
#else
    RTL_SYSTEM_TIME StandardDate;
#endif
    LONG StandardBias;
    WCHAR DaylightName[32];
#ifdef __REACTOS__
    TIME_FIELDS DaylightDate;
#else
    RTL_SYSTEM_TIME DaylightDate;
#endif
    LONG DaylightBias;
    WCHAR TimeZoneKeyName[128];
    BOOLEAN DynamicDaylightTimeDisabled;
} RTL_DYNAMIC_TIME_ZONE_INFORMATION, *PRTL_DYNAMIC_TIME_ZONE_INFORMATION;

typedef struct _CLIENT_ID
{
   HANDLE UniqueProcess;
   HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _CURDIR
{
    UNICODE_STRING DosPath;
    PVOID Handle;
} CURDIR, *PCURDIR;

typedef struct RTL_DRIVE_LETTER_CURDIR
{
    USHORT              Flags;
    USHORT              Length;
    ULONG               TimeStamp;
    UNICODE_STRING      DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

typedef struct _RTL_RELATIVE_NAME
{
    UNICODE_STRING RelativeName;
    HANDLE         ContainerDirectory;
    void          *CurDirRef;
} RTL_RELATIVE_NAME, *PRTL_RELATIVE_NAME;

typedef struct tagRTL_BITMAP {
    ULONG  SizeOfBitMap; /* Number of bits in the bitmap */
    PULONG Buffer; /* Bitmap data, assumed sized to a DWORD boundary */
} RTL_BITMAP, *PRTL_BITMAP;

typedef const RTL_BITMAP *PCRTL_BITMAP;

typedef struct tagRTL_BITMAP_RUN {
    ULONG StartingIndex; /* Bit position at which run starts */
    ULONG NumberOfBits;  /* Size of the run in bits */
} RTL_BITMAP_RUN, *PRTL_BITMAP_RUN;

typedef const RTL_BITMAP_RUN *PCRTL_BITMAP_RUN;

typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    ULONG               AllocationSize;
    ULONG               Size;
    ULONG               Flags;
    ULONG               DebugFlags;
    HANDLE              ConsoleHandle;
    ULONG               ConsoleFlags;
    HANDLE              hStdInput;
    HANDLE              hStdOutput;
    HANDLE              hStdError;
    CURDIR              CurrentDirectory;
    UNICODE_STRING      DllPath;
    UNICODE_STRING      ImagePathName;
    UNICODE_STRING      CommandLine;
    PWSTR               Environment;
    ULONG               dwX;
    ULONG               dwY;
    ULONG               dwXSize;
    ULONG               dwYSize;
    ULONG               dwXCountChars;
    ULONG               dwYCountChars;
    ULONG               dwFillAttribute;
    ULONG               dwFlags;
    ULONG               wShowWindow;
    UNICODE_STRING      WindowTitle;
    UNICODE_STRING      Desktop;
    UNICODE_STRING      ShellInfo;
    UNICODE_STRING      RuntimeInfo;
    RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20];
    ULONG_PTR           EnvironmentSize;
    ULONG_PTR           EnvironmentVersion;
    PVOID               PackageDependencyData;
    ULONG               ProcessGroupId;
    ULONG               LoaderThreads;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

/* value for Flags field (FIXME: not the correct name) */
#define PROCESS_PARAMS_FLAG_NORMALIZED 1

typedef struct _PEB_LDR_DATA
{
    ULONG               Length;
    BOOLEAN             Initialized;
    PVOID               SsHandle;
    LIST_ENTRY          InLoadOrderModuleList;
    LIST_ENTRY          InMemoryOrderModuleList;
    LIST_ENTRY          InInitializationOrderModuleList;
    PVOID               EntryInProgress;
    BOOLEAN             ShutdownInProgress;
    HANDLE              ShutdownThreadId;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _GDI_TEB_BATCH
{
    ULONG  Offset;
    HANDLE HDC;
    ULONG  Buffer[0x136];
} GDI_TEB_BATCH;

typedef struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME
{
    struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME *Previous;
    struct _ACTIVATION_CONTEXT                 *ActivationContext;
    ULONG                                       Flags;
} RTL_ACTIVATION_CONTEXT_STACK_FRAME, *PRTL_ACTIVATION_CONTEXT_STACK_FRAME;

typedef struct _ACTIVATION_CONTEXT_STACK
{
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *ActiveFrame;
    LIST_ENTRY                          FrameListCache;
    ULONG                               Flags;
    ULONG                               NextCookieSequenceNumber;
    ULONG_PTR                           StackId;
} ACTIVATION_CONTEXT_STACK, *PACTIVATION_CONTEXT_STACK;

typedef struct _TEB_ACTIVE_FRAME_CONTEXT
{
    ULONG       Flags;
    const char *FrameName;
} TEB_ACTIVE_FRAME_CONTEXT, *PTEB_ACTIVE_FRAME_CONTEXT;

typedef struct _TEB_ACTIVE_FRAME_CONTEXT_EX
{
    TEB_ACTIVE_FRAME_CONTEXT BasicContext;
    const char              *SourceLocation;
} TEB_ACTIVE_FRAME_CONTEXT_EX, *PTEB_ACTIVE_FRAME_CONTEXT_EX;

typedef struct _TEB_ACTIVE_FRAME
{
    ULONG                     Flags;
    struct _TEB_ACTIVE_FRAME *Previous;
    TEB_ACTIVE_FRAME_CONTEXT *Context;
} TEB_ACTIVE_FRAME, *PTEB_ACTIVE_FRAME;

typedef struct _TEB_ACTIVE_FRAME_EX
{
    TEB_ACTIVE_FRAME BasicFrame;
    void            *ExtensionIdentifier;
} TEB_ACTIVE_FRAME_EX, *PTEB_ACTIVE_FRAME_EX;

typedef struct _FLS_CALLBACK
{
    void                  *unknown;
    PFLS_CALLBACK_FUNCTION callback; /* ~0 if NULL callback is set, NULL if FLS index is free. */
} FLS_CALLBACK, *PFLS_CALLBACK;

typedef struct _FLS_INFO_CHUNK
{
    ULONG           count;         /* number of allocated FLS indexes in the chunk. */
    FLS_CALLBACK    callbacks[1];  /* the size is 0x10 for chunk 0 and is twice as
                                    * the previous chunk size for the rest. */
} FLS_INFO_CHUNK, *PFLS_INFO_CHUNK;

typedef struct _GLOBAL_FLS_DATA
{
    FLS_INFO_CHUNK *fls_callback_chunks[8];
    LIST_ENTRY      fls_list_head;
    ULONG           fls_high_index;
} GLOBAL_FLS_DATA, *PGLOBAL_FLS_DATA;

typedef struct _TEB_FLS_DATA
{
    LIST_ENTRY      fls_list_entry;
    void          **fls_data_chunks[8];
} TEB_FLS_DATA, *PTEB_FLS_DATA;

/* undocumented layout of WOW64INFO.CrossProcessWorkList and CHPEV2_PROCESS_INFO.CrossProcessWorkList */

typedef struct
{
    UINT      next;
    UINT      id;
    ULONGLONG addr;
    ULONGLONG size;
    UINT      args[4];
} CROSS_PROCESS_WORK_ENTRY;

typedef union
{
    struct
    {
        UINT first;
        UINT counter;
    };
    volatile LONGLONG hdr;
} CROSS_PROCESS_WORK_HDR;

typedef struct
{
    CROSS_PROCESS_WORK_HDR   free_list;
    CROSS_PROCESS_WORK_HDR   work_list;
    ULONGLONG                unknown[4];
    CROSS_PROCESS_WORK_ENTRY entries[1];
} CROSS_PROCESS_WORK_LIST;

typedef enum
{
    CrossProcessPreVirtualAlloc    = 0,
    CrossProcessPostVirtualAlloc   = 1,
    CrossProcessPreVirtualFree     = 2,
    CrossProcessPostVirtualFree    = 3,
    CrossProcessPreVirtualProtect  = 4,
    CrossProcessPostVirtualProtect = 5,
    CrossProcessFlushCache         = 6,
    CrossProcessFlushCacheHeavy    = 7,
    CrossProcessMemoryWrite        = 8,
} CROSS_PROCESS_NOTIFICATION;

#define CROSS_PROCESS_LIST_FLUSH 0x80000000
#define CROSS_PROCESS_LIST_ENTRY(list,pos) \
    ((CROSS_PROCESS_WORK_ENTRY *)((char *)(list) + ((pos) & ~CROSS_PROCESS_LIST_FLUSH)))

typedef struct _CHPE_V2_CPU_AREA_INFO
{
    BOOLEAN             InSimulation;         /* 000 */
    BOOLEAN             InSyscallCallback;    /* 001 */
    ULONG64             EmulatorStackBase;    /* 008 */
    ULONG64             EmulatorStackLimit;   /* 010 */
    ARM64EC_NT_CONTEXT *ContextAmd64;         /* 018 */
    ULONG              *SuspendDoorbell;      /* 020 */
    ULONG64             LoadingModuleModflag; /* 028 */
    void               *EmulatorData[4];      /* 030 */
    ULONG64             EmulatorDataInline;   /* 050 */
} CHPE_V2_CPU_AREA_INFO, *PCHPE_V2_CPU_AREA_INFO;

/* equivalent of WOW64INFO, stored after the 64-bit PEB */
typedef struct _CHPEV2_PROCESS_INFO
{
    ULONG                    Wow64ExecuteFlags;    /* 000 */
    USHORT                   NativeMachineType;    /* 004 */
    USHORT                   EmulatedMachineType;  /* 006 */
    HANDLE                   SectionHandle;        /* 008 */
    CROSS_PROCESS_WORK_LIST *CrossProcessWorkList; /* 010 */
    void                    *unknown;              /* 018 */
} CHPEV2_PROCESS_INFO, *PCHPEV2_PROCESS_INFO;

#define TEB_ACTIVE_FRAME_CONTEXT_FLAG_EXTENDED 0x00000001
#define TEB_ACTIVE_FRAME_FLAG_EXTENDED         0x00000001

typedef NTSTATUS (WINAPI *KERNEL_CALLBACK_PROC)(void *, ULONG); /* FIXME: not the correct name */

/***********************************************************************
 * PEB data structure
 */
typedef struct _PEB
{                                                                 /* win32/win64 */
    BOOLEAN                      InheritedAddressSpace;             /* 000/000 */
    BOOLEAN                      ReadImageFileExecOptions;          /* 001/001 */
    BOOLEAN                      BeingDebugged;                     /* 002/002 */
    UCHAR                        ImageUsedLargePages : 1;           /* 003/003 */
    UCHAR                        IsProtectedProcess : 1;
    UCHAR                        IsImageDynamicallyRelocated : 1;
    UCHAR                        SkipPatchingUser32Forwarders : 1;
    UCHAR                        IsPackagedProcess : 1;
    UCHAR                        IsAppContainer: 1;
    UCHAR                        IsProtectedProcessLight : 1;
    UCHAR                        IsLongPathAwareProcess : 1;
    HANDLE                       Mutant;                            /* 004/008 */
    HMODULE                      ImageBaseAddress;                  /* 008/010 */
    PPEB_LDR_DATA                LdrData;                           /* 00c/018 */
    RTL_USER_PROCESS_PARAMETERS *ProcessParameters;                 /* 010/020 */
    PVOID                        SubSystemData;                     /* 014/028 */
    HANDLE                       ProcessHeap;                       /* 018/030 */
    PRTL_CRITICAL_SECTION        FastPebLock;                       /* 01c/038 */
    PVOID                        AtlThunkSListPtr;                  /* 020/040 */
    PVOID                        IFEOKey;                           /* 024/048 */
    ULONG                        ProcessInJob : 1;                  /* 028/050 */
    ULONG                        ProcessInitializing : 1;
    ULONG                        ProcessUsingVEH : 1;
    ULONG                        ProcessUsingVCH : 1;
    ULONG                        ProcessUsingFTH : 1;
    ULONG                        ProcessPreviouslyThrottled : 1;
    ULONG                        ProcessCurrentlyThrottled : 1;
    ULONG                        ProcessImagesHotPatched : 1;
    ULONG                        ReservedBits0 : 24;
    KERNEL_CALLBACK_PROC        *KernelCallbackTable;               /* 02c/058 */
    ULONG                        Reserved;                          /* 030/060 */
    ULONG                        AtlThunkSListPtr32;                /* 034/064 */
    PVOID                        ApiSetMap;                         /* 038/068 */
    ULONG                        TlsExpansionCounter;               /* 03c/070 */
    PRTL_BITMAP                  TlsBitmap;                         /* 040/078 */
    ULONG                        TlsBitmapBits[2];                  /* 044/080 */
    PVOID                        ReadOnlySharedMemoryBase;          /* 04c/088 */
    PVOID                        SharedData;                        /* 050/090 */
    PVOID                       *ReadOnlyStaticServerData;          /* 054/098 */
    PVOID                        AnsiCodePageData;                  /* 058/0a0 */
    PVOID                        OemCodePageData;                   /* 05c/0a8 */
    PVOID                        UnicodeCaseTableData;              /* 060/0b0 */
    ULONG                        NumberOfProcessors;                /* 064/0b8 */
    ULONG                        NtGlobalFlag;                      /* 068/0bc */
    LARGE_INTEGER                CriticalSectionTimeout;            /* 070/0c0 */
    SIZE_T                       HeapSegmentReserve;                /* 078/0c8 */
    SIZE_T                       HeapSegmentCommit;                 /* 07c/0d0 */
    SIZE_T                       HeapDeCommitTotalFreeThreshold;    /* 080/0d8 */
    SIZE_T                       HeapDeCommitFreeBlockThreshold;    /* 084/0e0 */
    ULONG                        NumberOfHeaps;                     /* 088/0e8 */
    ULONG                        MaximumNumberOfHeaps;              /* 08c/0ec */
    PVOID                       *ProcessHeaps;                      /* 090/0f0 */
    PVOID                        GdiSharedHandleTable;              /* 094/0f8 */
    PVOID                        ProcessStarterHelper;              /* 098/100 */
    PVOID                        GdiDCAttributeList;                /* 09c/108 */
    PVOID                        LoaderLock;                        /* 0a0/110 */
    ULONG                        OSMajorVersion;                    /* 0a4/118 */
    ULONG                        OSMinorVersion;                    /* 0a8/11c */
    ULONG                        OSBuildNumber;                     /* 0ac/120 */
    ULONG                        OSPlatformId;                      /* 0b0/124 */
    ULONG                        ImageSubSystem;                    /* 0b4/128 */
    ULONG                        ImageSubSystemMajorVersion;        /* 0b8/12c */
    ULONG                        ImageSubSystemMinorVersion;        /* 0bc/130 */
    KAFFINITY                    ActiveProcessAffinityMask;         /* 0c0/138 */
#ifdef _WIN64
    ULONG                        GdiHandleBuffer[60];               /*    /140 */
#else
    ULONG                        GdiHandleBuffer[34];               /* 0c4/    */
#endif
    PVOID                        PostProcessInitRoutine;            /* 14c/230 */
    PRTL_BITMAP                  TlsExpansionBitmap;                /* 150/238 */
    ULONG                        TlsExpansionBitmapBits[32];        /* 154/240 */
    ULONG                        SessionId;                         /* 1d4/2c0 */
    ULARGE_INTEGER               AppCompatFlags;                    /* 1d8/2c8 */
    ULARGE_INTEGER               AppCompatFlagsUser;                /* 1e0/2d0 */
    PVOID                        ShimData;                          /* 1e8/2d8 */
    PVOID                        AppCompatInfo;                     /* 1ec/2e0 */
    UNICODE_STRING               CSDVersion;                        /* 1f0/2e8 */
    PVOID                        ActivationContextData;             /* 1f8/2f8 */
    PVOID                        ProcessAssemblyStorageMap;         /* 1fc/300 */
    PVOID                        SystemDefaultActivationData;       /* 200/308 */
    PVOID                        SystemAssemblyStorageMap;          /* 204/310 */
    SIZE_T                       MinimumStackCommit;                /* 208/318 */
    PVOID                       *FlsCallback;                       /* 20c/320 */
    LIST_ENTRY                   FlsListHead;                       /* 210/328 */
    union
    {
        PRTL_BITMAP              FlsBitmap;                         /* 218/338 */
#ifdef _WIN64
        CHPEV2_PROCESS_INFO     *ChpeV2ProcessInfo;                 /*    /338 */
#endif
    };
    ULONG                        FlsBitmapBits[4];                  /* 21c/340 */
    ULONG                        FlsHighIndex;                      /* 22c/350 */
    PVOID                        WerRegistrationData;               /* 230/358 */
    PVOID                        WerShipAssertPtr;                  /* 234/360 */
    PVOID                        EcCodeBitMap;                      /* 238/368 */
    PVOID                        pImageHeaderHash;                  /* 23c/370 */
    ULONG                        HeapTracingEnabled : 1;            /* 240/378 */
    ULONG                        CritSecTracingEnabled : 1;
    ULONG                        LibLoaderTracingEnabled : 1;
    ULONG                        SpareTracingBits : 29;
    ULONGLONG                    CsrServerReadOnlySharedMemoryBase; /* 248/380 */
    ULONG                        TppWorkerpListLock;                /* 250/388 */
    LIST_ENTRY                   TppWorkerpList;                    /* 254/390 */
    PVOID                        WaitOnAddressHashTable [0x80];     /* 25c/3a0 */
    PVOID                        TelemetryCoverageHeader;           /* 45c/7a0 */
    ULONG                        CloudFileFlags;                    /* 460/7a8 */
    ULONG                        CloudFileDiagFlags;                /* 464/7ac */
    CHAR                         PlaceholderCompatibilityMode;      /* 468/7b0 */
    CHAR                         PlaceholderCompatibilityModeReserved[7]; /* 469/7b1 */
    PVOID                        LeapSecondData;                    /* 470/7b8 */
    ULONG                        LeapSecondFlags;                   /* 474/7c0 */
    ULONG                        NtGlobalFlag2;                     /* 478/7c4 */
} PEB, *PPEB;


/***********************************************************************
 * TEB data structure
 */
typedef struct _TEB
{                                                                 /* win32/win64 */
    NT_TIB                       Tib;                               /* 000/0000 */
    PVOID                        EnvironmentPointer;                /* 01c/0038 */
    CLIENT_ID                    ClientId;                          /* 020/0040 */
    PVOID                        ActiveRpcHandle;                   /* 028/0050 */
    PVOID                        ThreadLocalStoragePointer;         /* 02c/0058 */
    PPEB                         Peb;                               /* 030/0060 */
    ULONG                        LastErrorValue;                    /* 034/0068 */
    ULONG                        CountOfOwnedCriticalSections;      /* 038/006c */
    PVOID                        CsrClientThread;                   /* 03c/0070 */
    PVOID                        Win32ThreadInfo;                   /* 040/0078 */
    ULONG                        User32Reserved[26];                /* 044/0080 */
    ULONG                        UserReserved[5];                   /* 0ac/00e8 */
    PVOID                        WOW32Reserved;                     /* 0c0/0100 */
    ULONG                        CurrentLocale;                     /* 0c4/0108 */
    ULONG                        FpSoftwareStatusRegister;          /* 0c8/010c */
    PVOID                        ReservedForDebuggerInstrumentation[16]; /* 0cc/0110 */
#ifdef _WIN64
    PVOID                        SystemReserved1[30];               /*    /0190 */
#else
    PVOID                        SystemReserved1[26];               /* 10c/     used for krnl386 private data in Wine */
#endif
    char                         PlaceholderCompatibilityMode;      /* 174/0280 */
    BOOLEAN                      PlaceholderHydrationAlwaysExplicit;/* 175/0281 */
    char                         PlaceholderReserved[10];           /* 176/0282 */
    DWORD                        ProxiedProcessId;                  /* 180/028c */
    ACTIVATION_CONTEXT_STACK     ActivationContextStack;            /* 184/0290 */
    UCHAR                        WorkingOnBehalfOfTicket[8];        /* 19c/02b8 */
    LONG                         ExceptionCode;                     /* 1a4/02c0 */
    ACTIVATION_CONTEXT_STACK    *ActivationContextStackPointer;     /* 1a8/02c8 */
    ULONG_PTR                    InstrumentationCallbackSp;         /* 1ac/02d0 */
    ULONG_PTR                    InstrumentationCallbackPreviousPc; /* 1b0/02d8 */
    ULONG_PTR                    InstrumentationCallbackPreviousSp; /* 1b4/02e0 */
#ifdef _WIN64
    ULONG                        TxFsContext;                       /*    /02e8 */
    BOOLEAN                      InstrumentationCallbackDisabled;   /*    /02ec */
    BOOLEAN                      UnalignedLoadStoreExceptions;      /*    /02ed */
#else
    BOOLEAN                      InstrumentationCallbackDisabled;   /* 1b8/     */
    BYTE                         SpareBytes1[23];                   /* 1b9/     */
    ULONG                        TxFsContext;                       /* 1d0/     */
#endif
    GDI_TEB_BATCH                GdiTebBatch;                       /* 1d4/02f0 used for ntdll private data in Wine */
    CLIENT_ID                    RealClientId;                      /* 6b4/07d8 */
    HANDLE                       GdiCachedProcessHandle;            /* 6bc/07e8 */
    ULONG                        GdiClientPID;                      /* 6c0/07f0 */
    ULONG                        GdiClientTID;                      /* 6c4/07f4 */
    PVOID                        GdiThreadLocaleInfo;               /* 6c8/07f8 */
    ULONG_PTR                    Win32ClientInfo[62];               /* 6cc/0800 used for user32 private data in Wine */
    PVOID                        glDispatchTable[233];              /* 7c4/09f0 */
    PVOID                        glReserved1[29];                   /* b68/1138 */
    PVOID                        glReserved2;                       /* bdc/1220 */
    PVOID                        glSectionInfo;                     /* be0/1228 */
    PVOID                        glSection;                         /* be4/1230 */
    PVOID                        glTable;                           /* be8/1238 */
    PVOID                        glCurrentRC;                       /* bec/1240 */
    PVOID                        glContext;                         /* bf0/1248 */
    ULONG                        LastStatusValue;                   /* bf4/1250 */
    UNICODE_STRING               StaticUnicodeString;               /* bf8/1258 */
    WCHAR                        StaticUnicodeBuffer[261];          /* c00/1268 */
    PVOID                        DeallocationStack;                 /* e0c/1478 */
    PVOID                        TlsSlots[64];                      /* e10/1480 */
    LIST_ENTRY                   TlsLinks;                          /* f10/1680 */
    PVOID                        Vdm;                               /* f18/1690 */
    PVOID                        ReservedForNtRpc;                  /* f1c/1698 */
    PVOID                        DbgSsReserved[2];                  /* f20/16a0 */
    ULONG                        HardErrorMode;                     /* f28/16b0 */
#ifdef _WIN64
    PVOID                        Instrumentation[11];               /*    /16b8 */
#else
    PVOID                        Instrumentation[9];                /* f2c/ */
#endif
    GUID                         ActivityId;                        /* f50/1710 */
    PVOID                        SubProcessTag;                     /* f60/1720 */
    PVOID                        PerflibData;                       /* f64/1728 */
    PVOID                        EtwTraceData;                      /* f68/1730 */
    PVOID                        WinSockData;                       /* f6c/1738 */
    ULONG                        GdiBatchCount;                     /* f70/1740 */
    ULONG                        IdealProcessorValue;               /* f74/1744 */
    ULONG                        GuaranteedStackBytes;              /* f78/1748 */
    PVOID                        ReservedForPerf;                   /* f7c/1750 */
    PVOID                        ReservedForOle;                    /* f80/1758 */
    ULONG                        WaitingOnLoaderLock;               /* f84/1760 */
    PVOID                        SavedPriorityState;                /* f88/1768 */
    ULONG_PTR                    ReservedForCodeCoverage;           /* f8c/1770 */
    PVOID                        ThreadPoolData;                    /* f90/1778 */
    PVOID                       *TlsExpansionSlots;                 /* f94/1780 */
#ifdef _WIN64
    union {
        PVOID                    DeallocationBStore;                /*    /1788 */
        CHPE_V2_CPU_AREA_INFO   *ChpeV2CpuAreaInfo;                 /*    /1788 */
    } DUMMYUNIONNAME;
    PVOID                        BStoreLimit;                       /*    /1790 */
#endif
    ULONG                        MuiGeneration;                     /* f98/1798 */
    ULONG                        IsImpersonating;                   /* f9c/179c */
    PVOID                        NlsCache;                          /* fa0/17a0 */
    PVOID                        ShimData;                          /* fa4/17a8 */
    ULONG                        HeapVirtualAffinity;               /* fa8/17b0 */
    PVOID                        CurrentTransactionHandle;          /* fac/17b8 */
    TEB_ACTIVE_FRAME            *ActiveFrame;                       /* fb0/17c0 */
    TEB_FLS_DATA                *FlsSlots;                          /* fb4/17c8 */
    PVOID                        PreferredLanguages;                /* fb8/17d0 */
    PVOID                        UserPrefLanguages;                 /* fbc/17d8 */
    PVOID                        MergedPrefLanguages;               /* fc0/17e0 */
    ULONG                        MuiImpersonation;                  /* fc4/17e8 */
    USHORT                       CrossTebFlags;                     /* fc8/17ec */
    USHORT                       SameTebFlags;                      /* fca/17ee */
    PVOID                        TxnScopeEnterCallback;             /* fcc/17f0 */
    PVOID                        TxnScopeExitCallback;              /* fd0/17f8 */
    PVOID                        TxnScopeContext;                   /* fd4/1800 */
    ULONG                        LockCount;                         /* fd8/1808 */
    LONG                         WowTebOffset;                      /* fdc/180c */
    PVOID                        ResourceRetValue;                  /* fe0/1810 */
    PVOID                        ReservedForWdf;                    /* fe4/1818 */
    ULONGLONG                    ReservedForCrt;                    /* fe8/1820 */
    GUID                         EffectiveContainerId;              /* ff0/1828 */
} TEB, *PTEB;


/***********************************************************************
 * The 32-bit/64-bit version of the PEB and TEB for WoW64
 */
#ifndef __REACTOS__
typedef struct _NT_TIB32
{
    ULONG ExceptionList;        /* 0000 */
    ULONG StackBase;            /* 0004 */
    ULONG StackLimit;           /* 0008 */
    ULONG SubSystemTib;         /* 000c */
    ULONG FiberData;            /* 0010 */
    ULONG ArbitraryUserPointer; /* 0014 */
    ULONG Self;                 /* 0018 */
} NT_TIB32;

typedef struct _NT_TIB64
{
    ULONG64 ExceptionList;        /* 0000 */
    ULONG64 StackBase;            /* 0008 */
    ULONG64 StackLimit;           /* 0010 */
    ULONG64 SubSystemTib;         /* 0018 */
    ULONG64 FiberData;            /* 0020 */
    ULONG64 ArbitraryUserPointer; /* 0028 */
    ULONG64 Self;                 /* 0030 */
} NT_TIB64;
#endif

typedef struct _CLIENT_ID32
{
   ULONG UniqueProcess;
   ULONG UniqueThread;
} CLIENT_ID32;

typedef struct _CLIENT_ID64
{
   ULONG64 UniqueProcess;
   ULONG64 UniqueThread;
} CLIENT_ID64;

#ifndef __REACTOS__
typedef struct _LIST_ENTRY32
{
    ULONG Flink;
    ULONG Blink;
} LIST_ENTRY32;

typedef struct _LIST_ENTRY64
{
  ULONG64 Flink;
  ULONG64 Blink;
} LIST_ENTRY64;
#endif

typedef struct _UNICODE_STRING32
{
    USHORT  Length;
    USHORT  MaximumLength;
    ULONG   Buffer;
} UNICODE_STRING32;

typedef struct _UNICODE_STRING64
{
  USHORT  Length;
  USHORT  MaximumLength;
  ULONG64 Buffer;
} UNICODE_STRING64;

typedef struct _ACTIVATION_CONTEXT_STACK32
{
    ULONG        ActiveFrame;
    LIST_ENTRY32 FrameListCache;
    ULONG        Flags;
    ULONG        NextCookieSequenceNumber;
    ULONG32      StackId;
} ACTIVATION_CONTEXT_STACK32;

typedef struct _ACTIVATION_CONTEXT_STACK64
{
    ULONG64      ActiveFrame;
    LIST_ENTRY64 FrameListCache;
    ULONG        Flags;
    ULONG        NextCookieSequenceNumber;
    ULONG64      StackId;
} ACTIVATION_CONTEXT_STACK64;

typedef struct _CURDIR32
{
    UNICODE_STRING32 DosPath;
    ULONG Handle;
} CURDIR32;

typedef struct _CURDIR64
{
    UNICODE_STRING64 DosPath;
    ULONG64 Handle;
} CURDIR64;

typedef struct RTL_DRIVE_LETTER_CURDIR32
{
    USHORT              Flags;
    USHORT              Length;
    ULONG               TimeStamp;
    UNICODE_STRING32    DosPath;
} RTL_DRIVE_LETTER_CURDIR32;

typedef struct RTL_DRIVE_LETTER_CURDIR64
{
    USHORT              Flags;
    USHORT              Length;
    ULONG               TimeStamp;
    UNICODE_STRING64    DosPath;
} RTL_DRIVE_LETTER_CURDIR64;

typedef struct _RTL_USER_PROCESS_PARAMETERS32
{
    ULONG               AllocationSize;
    ULONG               Size;
    ULONG               Flags;
    ULONG               DebugFlags;
    ULONG               ConsoleHandle;
    ULONG               ConsoleFlags;
    ULONG               hStdInput;
    ULONG               hStdOutput;
    ULONG               hStdError;
    CURDIR32            CurrentDirectory;
    UNICODE_STRING32    DllPath;
    UNICODE_STRING32    ImagePathName;
    UNICODE_STRING32    CommandLine;
    ULONG               Environment;
    ULONG               dwX;
    ULONG               dwY;
    ULONG               dwXSize;
    ULONG               dwYSize;
    ULONG               dwXCountChars;
    ULONG               dwYCountChars;
    ULONG               dwFillAttribute;
    ULONG               dwFlags;
    ULONG               wShowWindow;
    UNICODE_STRING32    WindowTitle;
    UNICODE_STRING32    Desktop;
    UNICODE_STRING32    ShellInfo;
    UNICODE_STRING32    RuntimeInfo;
    RTL_DRIVE_LETTER_CURDIR32 DLCurrentDirectory[0x20];
    ULONG               EnvironmentSize;
    ULONG               EnvironmentVersion;
    ULONG               PackageDependencyData;
    ULONG               ProcessGroupId;
    ULONG               LoaderThreads;
} RTL_USER_PROCESS_PARAMETERS32;

typedef struct _RTL_USER_PROCESS_PARAMETERS64
{
    ULONG               AllocationSize;
    ULONG               Size;
    ULONG               Flags;
    ULONG               DebugFlags;
    ULONG64             ConsoleHandle;
    ULONG               ConsoleFlags;
    ULONG64             hStdInput;
    ULONG64             hStdOutput;
    ULONG64             hStdError;
    CURDIR64            CurrentDirectory;
    UNICODE_STRING64    DllPath;
    UNICODE_STRING64    ImagePathName;
    UNICODE_STRING64    CommandLine;
    ULONG64             Environment;
    ULONG               dwX;
    ULONG               dwY;
    ULONG               dwXSize;
    ULONG               dwYSize;
    ULONG               dwXCountChars;
    ULONG               dwYCountChars;
    ULONG               dwFillAttribute;
    ULONG               dwFlags;
    ULONG               wShowWindow;
    UNICODE_STRING64    WindowTitle;
    UNICODE_STRING64    Desktop;
    UNICODE_STRING64    ShellInfo;
    UNICODE_STRING64    RuntimeInfo;
    RTL_DRIVE_LETTER_CURDIR64 DLCurrentDirectory[0x20];
    ULONG64             EnvironmentSize;
    ULONG64             EnvironmentVersion;
    ULONG64             PackageDependencyData;
    ULONG               ProcessGroupId;
    ULONG               LoaderThreads;
} RTL_USER_PROCESS_PARAMETERS64;

typedef struct _PEB_LDR_DATA32
{
    ULONG               Length;
    BOOLEAN             Initialized;
    ULONG               SsHandle;
    LIST_ENTRY32        InLoadOrderModuleList;
    LIST_ENTRY32        InMemoryOrderModuleList;
    LIST_ENTRY32        InInitializationOrderModuleList;
    ULONG               EntryInProgress;
    BOOLEAN             ShutdownInProgress;
    ULONG               ShutdownThreadId;
} PEB_LDR_DATA32, *PPEB_LDR_DATA32;

typedef struct _PEB_LDR_DATA64
{
    ULONG               Length;
    BOOLEAN             Initialized;
    ULONG64             SsHandle;
    LIST_ENTRY64        InLoadOrderModuleList;
    LIST_ENTRY64        InMemoryOrderModuleList;
    LIST_ENTRY64        InInitializationOrderModuleList;
    ULONG64             EntryInProgress;
    BOOLEAN             ShutdownInProgress;
    ULONG64             ShutdownThreadId;
} PEB_LDR_DATA64, *PPEB_LDR_DATA64;

typedef struct _PEB32
{
    BOOLEAN                      InheritedAddressSpace;             /* 0000 */
    BOOLEAN                      ReadImageFileExecOptions;          /* 0001 */
    BOOLEAN                      BeingDebugged;                     /* 0002 */
    UCHAR                        ImageUsedLargePages : 1;           /* 0003 */
    UCHAR                        IsProtectedProcess : 1;
    UCHAR                        IsImageDynamicallyRelocated : 1;
    UCHAR                        SkipPatchingUser32Forwarders : 1;
    UCHAR                        IsPackagedProcess : 1;
    UCHAR                        IsAppContainer: 1;
    UCHAR                        IsProtectedProcessLight : 1;
    UCHAR                        IsLongPathAwareProcess : 1;
    ULONG                        Mutant;                            /* 0004 */
    ULONG                        ImageBaseAddress;                  /* 0008 */
    ULONG                        LdrData;                           /* 000c */
    ULONG                        ProcessParameters;                 /* 0010 */
    ULONG                        SubSystemData;                     /* 0014 */
    ULONG                        ProcessHeap;                       /* 0018 */
    ULONG                        FastPebLock;                       /* 001c */
    ULONG                        AtlThunkSListPtr;                  /* 0020 */
    ULONG                        IFEOKey;                           /* 0024 */
    ULONG                        ProcessInJob : 1;                  /* 0028 */
    ULONG                        ProcessInitializing : 1;
    ULONG                        ProcessUsingVEH : 1;
    ULONG                        ProcessUsingVCH : 1;
    ULONG                        ProcessUsingFTH : 1;
    ULONG                        ProcessPreviouslyThrottled : 1;
    ULONG                        ProcessCurrentlyThrottled : 1;
    ULONG                        ProcessImagesHotPatched : 1;
    ULONG                        ReservedBits0 : 24;
    ULONG                        KernelCallbackTable;               /* 002c */
    ULONG                        Reserved;                          /* 0030 */
    ULONG                        AtlThunkSListPtr32;                /* 0034 */
    ULONG                        ApiSetMap;                         /* 0038 */
    ULONG                        TlsExpansionCounter;               /* 003c */
    ULONG                        TlsBitmap;                         /* 0040 */
    ULONG                        TlsBitmapBits[2];                  /* 0044 */
    ULONG                        ReadOnlySharedMemoryBase;          /* 004c */
    ULONG                        SharedData;                        /* 0050 */
    ULONG                        ReadOnlyStaticServerData;          /* 0054 */
    ULONG                        AnsiCodePageData;                  /* 0058 */
    ULONG                        OemCodePageData;                   /* 005c */
    ULONG                        UnicodeCaseTableData;              /* 0060 */
    ULONG                        NumberOfProcessors;                /* 0064 */
    ULONG                        NtGlobalFlag;                      /* 0068 */
    LARGE_INTEGER                CriticalSectionTimeout;            /* 0070 */
    ULONG                        HeapSegmentReserve;                /* 0078 */
    ULONG                        HeapSegmentCommit;                 /* 007c */
    ULONG                        HeapDeCommitTotalFreeThreshold;    /* 0080 */
    ULONG                        HeapDeCommitFreeBlockThreshold;    /* 0084 */
    ULONG                        NumberOfHeaps;                     /* 0088 */
    ULONG                        MaximumNumberOfHeaps;              /* 008c */
    ULONG                        ProcessHeaps;                      /* 0090 */
    ULONG                        GdiSharedHandleTable;              /* 0094 */
    ULONG                        ProcessStarterHelper;              /* 0098 */
    ULONG                        GdiDCAttributeList;                /* 009c */
    ULONG                        LoaderLock;                        /* 00a0 */
    ULONG                        OSMajorVersion;                    /* 00a4 */
    ULONG                        OSMinorVersion;                    /* 00a8 */
    ULONG                        OSBuildNumber;                     /* 00ac */
    ULONG                        OSPlatformId;                      /* 00b0 */
    ULONG                        ImageSubSystem;                    /* 00b4 */
    ULONG                        ImageSubSystemMajorVersion;        /* 00b8 */
    ULONG                        ImageSubSystemMinorVersion;        /* 00bc */
    ULONG                        ActiveProcessAffinityMask;         /* 00c0 */
    ULONG                        GdiHandleBuffer[34];               /* 00c4 */
    ULONG                        PostProcessInitRoutine;            /* 014c */
    ULONG                        TlsExpansionBitmap;                /* 0150 */
    ULONG                        TlsExpansionBitmapBits[32];        /* 0154 */
    ULONG                        SessionId;                         /* 01d4 */
    ULARGE_INTEGER               AppCompatFlags;                    /* 01d8 */
    ULARGE_INTEGER               AppCompatFlagsUser;                /* 01e0 */
    ULONG                        ShimData;                          /* 01e8 */
    ULONG                        AppCompatInfo;                     /* 01ec */
    UNICODE_STRING32             CSDVersion;                        /* 01f0 */
    ULONG                        ActivationContextData;             /* 01f8 */
    ULONG                        ProcessAssemblyStorageMap;         /* 01fc */
    ULONG                        SystemDefaultActivationData;       /* 0200 */
    ULONG                        SystemAssemblyStorageMap;          /* 0204 */
    ULONG                        MinimumStackCommit;                /* 0208 */
    ULONG                        FlsCallback;                       /* 020c */
    LIST_ENTRY32                 FlsListHead;                       /* 0210 */
    ULONG                        FlsBitmap;                         /* 0218 */
    ULONG                        FlsBitmapBits[4];                  /* 021c */
    ULONG                        FlsHighIndex;                      /* 022c */
    ULONG                        WerRegistrationData;               /* 0230 */
    ULONG                        WerShipAssertPtr;                  /* 0234 */
    ULONG                        pUnused;                           /* 0238 */
    ULONG                        pImageHeaderHash;                  /* 023c */
    ULONG                        HeapTracingEnabled : 1;            /* 0240 */
    ULONG                        CritSecTracingEnabled : 1;
    ULONG                        LibLoaderTracingEnabled : 1;
    ULONG                        SpareTracingBits : 29;
    ULONGLONG                    CsrServerReadOnlySharedMemoryBase; /* 0248 */
    ULONG                        TppWorkerpListLock;                /* 0250 */
    LIST_ENTRY32                 TppWorkerpList;                    /* 0254 */
    ULONG                        WaitOnAddressHashTable [0x80];     /* 025c */
    ULONG                        TelemetryCoverageHeader;           /* 045c */
    ULONG                        CloudFileFlags;                    /* 0460 */
    ULONG                        CloudFileDiagFlags;                /* 0464 */
    CHAR                         PlaceholderCompatibilityMode;      /* 0468 */
    CHAR                         PlaceholderCompatibilityModeReserved[7]; /* 0469 */
    ULONG                        LeapSecondData;                    /* 0470 */
    ULONG                        LeapSecondFlags;                   /* 0474 */
    ULONG                        NtGlobalFlag2;                     /* 0478 */
} PEB32;

C_ASSERT( sizeof(PEB32) == 0x480 );

typedef struct _PEB64
{
    BOOLEAN                      InheritedAddressSpace;             /* 0000 */
    BOOLEAN                      ReadImageFileExecOptions;          /* 0001 */
    BOOLEAN                      BeingDebugged;                     /* 0002 */
    UCHAR                        ImageUsedLargePages : 1;           /* 0003 */
    UCHAR                        IsProtectedProcess : 1;
    UCHAR                        IsImageDynamicallyRelocated : 1;
    UCHAR                        SkipPatchingUser32Forwarders : 1;
    UCHAR                        IsPackagedProcess : 1;
    UCHAR                        IsAppContainer: 1;
    UCHAR                        IsProtectedProcessLight : 1;
    UCHAR                        IsLongPathAwareProcess : 1;
    ULONG64                      Mutant;                            /* 0008 */
    ULONG64                      ImageBaseAddress;                  /* 0010 */
    ULONG64                      LdrData;                           /* 0018 */
    ULONG64                      ProcessParameters;                 /* 0020 */
    ULONG64                      SubSystemData;                     /* 0028 */
    ULONG64                      ProcessHeap;                       /* 0030 */
    ULONG64                      FastPebLock;                       /* 0038 */
    ULONG64                      AtlThunkSListPtr;                  /* 0040 */
    ULONG64                      IFEOKey;                           /* 0048 */
    ULONG                        ProcessInJob : 1;                  /* 0050 */
    ULONG                        ProcessInitializing : 1;
    ULONG                        ProcessUsingVEH : 1;
    ULONG                        ProcessUsingVCH : 1;
    ULONG                        ProcessUsingFTH : 1;
    ULONG                        ProcessPreviouslyThrottled : 1;
    ULONG                        ProcessCurrentlyThrottled : 1;
    ULONG                        ProcessImagesHotPatched : 1;
    ULONG                        ReservedBits0 : 24;
    ULONG64                      KernelCallbackTable;               /* 0058 */
    ULONG                        Reserved;                          /* 0060 */
    ULONG                        AtlThunkSListPtr32;                /* 0064 */
    ULONG64                      ApiSetMap;                         /* 0068 */
    ULONG                        TlsExpansionCounter;               /* 0070 */
    ULONG64                      TlsBitmap;                         /* 0078 */
    ULONG                        TlsBitmapBits[2];                  /* 0080 */
    ULONG64                      ReadOnlySharedMemoryBase;          /* 0088 */
    ULONG64                      SharedData;                        /* 0090 */
    ULONG64                      ReadOnlyStaticServerData;          /* 0098 */
    ULONG64                      AnsiCodePageData;                  /* 00a0 */
    ULONG64                      OemCodePageData;                   /* 00a8 */
    ULONG64                      UnicodeCaseTableData;              /* 00b0 */
    ULONG                        NumberOfProcessors;                /* 00b8 */
    ULONG                        NtGlobalFlag;                      /* 00bc */
    LARGE_INTEGER                CriticalSectionTimeout;            /* 00c0 */
    ULONG64                      HeapSegmentReserve;                /* 00c8 */
    ULONG64                      HeapSegmentCommit;                 /* 00d0 */
    ULONG64                      HeapDeCommitTotalFreeThreshold;    /* 00d8 */
    ULONG64                      HeapDeCommitFreeBlockThreshold;    /* 00e0 */
    ULONG                        NumberOfHeaps;                     /* 00e8 */
    ULONG                        MaximumNumberOfHeaps;              /* 00ec */
    ULONG64                      ProcessHeaps;                      /* 00f0 */
    ULONG64                      GdiSharedHandleTable;              /* 00f8 */
    ULONG64                      ProcessStarterHelper;              /* 0100 */
    ULONG64                      GdiDCAttributeList;                /* 0108 */
    ULONG64                      LoaderLock;                        /* 0110 */
    ULONG                        OSMajorVersion;                    /* 0118 */
    ULONG                        OSMinorVersion;                    /* 011c */
    ULONG                        OSBuildNumber;                     /* 0120 */
    ULONG                        OSPlatformId;                      /* 0124 */
    ULONG                        ImageSubSystem;                    /* 0128 */
    ULONG                        ImageSubSystemMajorVersion;        /* 012c */
    ULONG                        ImageSubSystemMinorVersion;        /* 0130 */
    ULONG64                      ActiveProcessAffinityMask;         /* 0138 */
    ULONG                        GdiHandleBuffer[60];               /* 0140 */
    ULONG64                      PostProcessInitRoutine;            /* 0230 */
    ULONG64                      TlsExpansionBitmap;                /* 0238 */
    ULONG                        TlsExpansionBitmapBits[32];        /* 0240 */
    ULONG                        SessionId;                         /* 02c0 */
    ULARGE_INTEGER               AppCompatFlags;                    /* 02c8 */
    ULARGE_INTEGER               AppCompatFlagsUser;                /* 02d0 */
    ULONG64                      ShimData;                          /* 02d8 */
    ULONG64                      AppCompatInfo;                     /* 02e0 */
    UNICODE_STRING64             CSDVersion;                        /* 02e8 */
    ULONG64                      ActivationContextData;             /* 02f8 */
    ULONG64                      ProcessAssemblyStorageMap;         /* 0300 */
    ULONG64                      SystemDefaultActivationData;       /* 0308 */
    ULONG64                      SystemAssemblyStorageMap;          /* 0310 */
    ULONG64                      MinimumStackCommit;                /* 0318 */
    ULONG64                      FlsCallback;                       /* 0320 */
    LIST_ENTRY64                 FlsListHead;                       /* 0328 */
    union
    {
        ULONG64                  FlsBitmap;                         /* 0338 */
        ULONG64                  ChpeV2ProcessInfo;                 /* 0338 */
    };
    ULONG                        FlsBitmapBits[4];                  /* 0340 */
    ULONG                        FlsHighIndex;                      /* 0350 */
    ULONG64                      WerRegistrationData;               /* 0358 */
    ULONG64                      WerShipAssertPtr;                  /* 0360 */
    ULONG64                      pUnused;                           /* 0368 */
    ULONG64                      pImageHeaderHash;                  /* 0370 */
    ULONG                        HeapTracingEnabled : 1;            /* 0378 */
    ULONG                        CritSecTracingEnabled : 1;
    ULONG                        LibLoaderTracingEnabled : 1;
    ULONG                        SpareTracingBits : 29;
    ULONGLONG                    CsrServerReadOnlySharedMemoryBase; /* 0380 */
    ULONG                        TppWorkerpListLock;                /* 0388 */
    LIST_ENTRY64                 TppWorkerpList;                    /* 0390 */
    ULONG64                      WaitOnAddressHashTable [0x80];     /* 03a0 */
    ULONG64                      TelemetryCoverageHeader;           /* 07a0 */
    ULONG                        CloudFileFlags;                    /* 07a8 */
    ULONG                        CloudFileDiagFlags;                /* 07ac */
    CHAR                         PlaceholderCompatibilityMode;      /* 07b0 */
    CHAR                         PlaceholderCompatibilityModeReserved[7]; /* 07b1 */
    ULONG64                      LeapSecondData;                    /* 07b8 */
    ULONG                        LeapSecondFlags;                   /* 07c0 */
    ULONG                        NtGlobalFlag2;                     /* 07c4 */
} PEB64;

C_ASSERT( sizeof(PEB64) == 0x7c8 );

typedef struct _TEB32
{
    NT_TIB32                     Tib;                               /* 0000 */
    ULONG                        EnvironmentPointer;                /* 001c */
    CLIENT_ID32                  ClientId;                          /* 0020 */
    ULONG                        ActiveRpcHandle;                   /* 0028 */
    ULONG                        ThreadLocalStoragePointer;         /* 002c */
    ULONG                        Peb;                               /* 0030 */
    ULONG                        LastErrorValue;                    /* 0034 */
    ULONG                        CountOfOwnedCriticalSections;      /* 0038 */
    ULONG                        CsrClientThread;                   /* 003c */
    ULONG                        Win32ThreadInfo;                   /* 0040 */
    ULONG                        User32Reserved[26];                /* 0044 */
    ULONG                        UserReserved[5];                   /* 00ac */
    ULONG                        WOW32Reserved;                     /* 00c0 */
    ULONG                        CurrentLocale;                     /* 00c4 */
    ULONG                        FpSoftwareStatusRegister;          /* 00c8 */
    ULONG                        ReservedForDebuggerInstrumentation[16]; /* 00cc */
    ULONG                        SystemReserved1[26];               /* 010c */
    char                         PlaceholderCompatibilityMode;      /* 0174 */
    BOOLEAN                      PlaceholderHydrationAlwaysExplicit;/* 0175 */
    char                         PlaceholderReserved[10];           /* 0176 */
    DWORD                        ProxiedProcessId;                  /* 0180 */
    ACTIVATION_CONTEXT_STACK32   ActivationContextStack;            /* 0184 */
    UCHAR                        WorkingOnBehalfOfTicket[8];        /* 019c */
    LONG                         ExceptionCode;                     /* 01a4 */
    ULONG                        ActivationContextStackPointer;     /* 01a8 */
    ULONG                        InstrumentationCallbackSp;         /* 01ac */
    ULONG                        InstrumentationCallbackPreviousPc; /* 01b0 */
    ULONG                        InstrumentationCallbackPreviousSp; /* 01b4 */
    BOOLEAN                      InstrumentationCallbackDisabled;   /* 01b8 */
    BYTE                         SpareBytes1[23];                   /* 01b9 */
    ULONG                        TxFsContext;                       /* 01d0 */
    ULONG                        GdiTebBatch[0x138];                /* 01d4 */
    CLIENT_ID32                  RealClientId;                      /* 06b4 */
    ULONG                        GdiCachedProcessHandle;            /* 06bc */
    ULONG                        GdiClientPID;                      /* 06c0 */
    ULONG                        GdiClientTID;                      /* 06c4 */
    ULONG                        GdiThreadLocaleInfo;               /* 06c8 */
    ULONG                        Win32ClientInfo[62];               /* 06cc */
    ULONG                        glDispatchTable[233];              /* 07c4 */
    ULONG                        glReserved1[29];                   /* 0b68 */
    ULONG                        glReserved2;                       /* 0bdc */
    ULONG                        glSectionInfo;                     /* 0be0 */
    ULONG                        glSection;                         /* 0be4 */
    ULONG                        glTable;                           /* 0be8 */
    ULONG                        glCurrentRC;                       /* 0bec */
    ULONG                        glContext;                         /* 0bf0 */
    ULONG                        LastStatusValue;                   /* 0bf4 */
    UNICODE_STRING32             StaticUnicodeString;               /* 0bf8 */
    WCHAR                        StaticUnicodeBuffer[261];          /* 0c00 */
    ULONG                        DeallocationStack;                 /* 0e0c */
    ULONG                        TlsSlots[64];                      /* 0e10 */
    LIST_ENTRY32                 TlsLinks;                          /* 0f10 */
    ULONG                        Vdm;                               /* 0f18 */
    ULONG                        ReservedForNtRpc;                  /* 0f1c */
    ULONG                        DbgSsReserved[2];                  /* 0f20 */
    ULONG                        HardErrorMode;                     /* 0f28 */
    ULONG                        Instrumentation[9];                /* 0f2c */
    GUID                         ActivityId;                        /* 0f50 */
    ULONG                        SubProcessTag;                     /* 0f60 */
    ULONG                        PerflibData;                       /* 0f64 */
    ULONG                        EtwTraceData;                      /* 0f68 */
    ULONG                        WinSockData;                       /* 0f6c */
    ULONG                        GdiBatchCount;                     /* 0f70 */
    ULONG                        IdealProcessorValue;               /* 0f74 */
    ULONG                        GuaranteedStackBytes;              /* 0f78 */
    ULONG                        ReservedForPerf;                   /* 0f7c */
    ULONG                        ReservedForOle;                    /* 0f80 */
    ULONG                        WaitingOnLoaderLock;               /* 0f84 */
    ULONG                        SavedPriorityState;                /* 0f88 */
    ULONG                        ReservedForCodeCoverage;           /* 0f8c */
    ULONG                        ThreadPoolData;                    /* 0f90 */
    ULONG                        TlsExpansionSlots;                 /* 0f94 */
    ULONG                        MuiGeneration;                     /* 0f98 */
    ULONG                        IsImpersonating;                   /* 0f9c */
    ULONG                        NlsCache;                          /* 0fa0 */
    ULONG                        ShimData;                          /* 0fa4 */
    ULONG                        HeapVirtualAffinity;               /* 0fa8 */
    ULONG                        CurrentTransactionHandle;          /* 0fac */
    ULONG                        ActiveFrame;                       /* 0fb0 */
    ULONG                        FlsSlots;                          /* 0fb4 */
    ULONG                        PreferredLanguages;                /* 0fb8 */
    ULONG                        UserPrefLanguages;                 /* 0fbc */
    ULONG                        MergedPrefLanguages;               /* 0fc0 */
    ULONG                        MuiImpersonation;                  /* 0fc4 */
    USHORT                       CrossTebFlags;                     /* 0fc8 */
    USHORT                       SameTebFlags;                      /* 0fca */
    ULONG                        TxnScopeEnterCallback;             /* 0fcc */
    ULONG                        TxnScopeExitCallback;              /* 0fd0 */
    ULONG                        TxnScopeContext;                   /* 0fd4 */
    ULONG                        LockCount;                         /* 0fd8 */
    LONG                         WowTebOffset;                      /* 0fdc */
    ULONG                        ResourceRetValue;                  /* 0fe0 */
    ULONG                        ReservedForWdf;                    /* 0fe4 */
    ULONGLONG                    ReservedForCrt;                    /* 0fe8 */
    GUID                         EffectiveContainerId;              /* 0ff0 */
} TEB32;

C_ASSERT( sizeof(TEB32) == 0x1000 );

typedef struct _TEB64
{
    NT_TIB64                     Tib;                               /* 0000 */
    ULONG64                      EnvironmentPointer;                /* 0038 */
    CLIENT_ID64                  ClientId;                          /* 0040 */
    ULONG64                      ActiveRpcHandle;                   /* 0050 */
    ULONG64                      ThreadLocalStoragePointer;         /* 0058 */
    ULONG64                      Peb;                               /* 0060 */
    ULONG                        LastErrorValue;                    /* 0068 */
    ULONG                        CountOfOwnedCriticalSections;      /* 006c */
    ULONG64                      CsrClientThread;                   /* 0070 */
    ULONG64                      Win32ThreadInfo;                   /* 0078 */
    ULONG                        User32Reserved[26];                /* 0080 */
    ULONG                        UserReserved[5];                   /* 00e8 */
    ULONG64                      WOW32Reserved;                     /* 0100 */
    ULONG                        CurrentLocale;                     /* 0108 */
    ULONG                        FpSoftwareStatusRegister;          /* 010c */
    ULONG64                      ReservedForDebuggerInstrumentation[16]; /* 0110 */
    ULONG64                      SystemReserved1[30];               /* 0190 */
    char                         PlaceholderCompatibilityMode;      /* 0280 */
    BOOLEAN                      PlaceholderHydrationAlwaysExplicit;/* 0281 */
    char                         PlaceholderReserved[10];           /* 0282 */
    DWORD                        ProxiedProcessId;                  /* 028c */
    ACTIVATION_CONTEXT_STACK64   ActivationContextStack;            /* 0290 */
    UCHAR                        WorkingOnBehalfOfTicket[8];        /* 02b8 */
    LONG                         ExceptionCode;                     /* 02c0 */
    ULONG64                      ActivationContextStackPointer;     /* 02c8 */
    ULONG64                      InstrumentationCallbackSp;         /* 02d0 */
    ULONG64                      InstrumentationCallbackPreviousPc; /* 02d8 */
    ULONG64                      InstrumentationCallbackPreviousSp; /* 02e0 */
    ULONG                        TxFsContext;                       /* 02e8 */
    BOOLEAN                      InstrumentationCallbackDisabled;   /* 02ec */
    BOOLEAN                      UnalignedLoadStoreExceptions;      /* 02ed */
    ULONG64                      GdiTebBatch[0x9d];                 /* 02f0 */
    CLIENT_ID64                  RealClientId;                      /* 07d8 */
    ULONG64                      GdiCachedProcessHandle;            /* 07e8 */
    ULONG                        GdiClientPID;                      /* 07f0 */
    ULONG                        GdiClientTID;                      /* 07f4 */
    ULONG64                      GdiThreadLocaleInfo;               /* 07f8 */
    ULONG64                      Win32ClientInfo[62];               /* 0800 */
    ULONG64                      glDispatchTable[233];              /* 09f0 */
    ULONG64                      glReserved1[29];                   /* 1138 */
    ULONG64                      glReserved2;                       /* 1220 */
    ULONG64                      glSectionInfo;                     /* 1228 */
    ULONG64                      glSection;                         /* 1230 */
    ULONG64                      glTable;                           /* 1238 */
    ULONG64                      glCurrentRC;                       /* 1240 */
    ULONG64                      glContext;                         /* 1248 */
    ULONG                        LastStatusValue;                   /* 1250 */
    UNICODE_STRING64             StaticUnicodeString;               /* 1258 */
    WCHAR                        StaticUnicodeBuffer[261];          /* 1268 */
    ULONG64                      DeallocationStack;                 /* 1478 */
    ULONG64                      TlsSlots[64];                      /* 1480 */
    LIST_ENTRY64                 TlsLinks;                          /* 1680 */
    ULONG64                      Vdm;                               /* 1690 */
    ULONG64                      ReservedForNtRpc;                  /* 1698 */
    ULONG64                      DbgSsReserved[2];                  /* 16a0 */
    ULONG                        HardErrorMode;                     /* 16b0 */
    ULONG64                      Instrumentation[11];               /* 16b8 */
    GUID                         ActivityId;                        /* 1710 */
    ULONG64                      SubProcessTag;                     /* 1720 */
    ULONG64                      PerflibData;                       /* 1728 */
    ULONG64                      EtwTraceData;                      /* 1730 */
    ULONG64                      WinSockData;                       /* 1738 */
    ULONG                        GdiBatchCount;                     /* 1740 */
    ULONG                        IdealProcessorValue;               /* 1744 */
    ULONG                        GuaranteedStackBytes;              /* 1748 */
    ULONG64                      ReservedForPerf;                   /* 1750 */
    ULONG64                      ReservedForOle;                    /* 1758 */
    ULONG                        WaitingOnLoaderLock;               /* 1760 */
    ULONG64                      SavedPriorityState;                /* 1768 */
    ULONG64                      ReservedForCodeCoverage;           /* 1770 */
    ULONG64                      ThreadPoolData;                    /* 1778 */
    ULONG64                      TlsExpansionSlots;                 /* 1780 */
    union {
        ULONG64                  DeallocationBStore;                /* 1788 */
        ULONG64                  ChpeV2CpuAreaInfo;                 /* 1788 */
    } DUMMYUNIONNAME;
    ULONG64                      BStoreLimit;                       /* 1790 */
    ULONG                        MuiGeneration;                     /* 1798 */
    ULONG                        IsImpersonating;                   /* 179c */
    ULONG64                      NlsCache;                          /* 17a0 */
    ULONG64                      ShimData;                          /* 17a8 */
    ULONG                        HeapVirtualAffinity;               /* 17b0 */
    ULONG64                      CurrentTransactionHandle;          /* 17b8 */
    ULONG64                      ActiveFrame;                       /* 17c0 */
    ULONG64                      FlsSlots;                          /* 17c8 */
    ULONG64                      PreferredLanguages;                /* 17d0 */
    ULONG64                      UserPrefLanguages;                 /* 17d8 */
    ULONG64                      MergedPrefLanguages;               /* 17e0 */
    ULONG                        MuiImpersonation;                  /* 17e8 */
    USHORT                       CrossTebFlags;                     /* 17ec */
    USHORT                       SameTebFlags;                      /* 17ee */
    ULONG64                      TxnScopeEnterCallback;             /* 17f0 */
    ULONG64                      TxnScopeExitCallback;              /* 17f8 */
    ULONG64                      TxnScopeContext;                   /* 1800 */
    ULONG                        LockCount;                         /* 1808 */
    LONG                         WowTebOffset;                      /* 180c */
    ULONG64                      ResourceRetValue;                  /* 1810 */
    ULONG64                      ReservedForWdf;                    /* 1818 */
    ULONGLONG                    ReservedForCrt;                    /* 1820 */
    GUID                         EffectiveContainerId;              /* 1828 */
} TEB64;

C_ASSERT( sizeof(TEB64) == 0x1838 );

#ifdef _WIN64
C_ASSERT( sizeof(PEB) == sizeof(PEB64) );
C_ASSERT( sizeof(TEB) == sizeof(TEB64) );
#else
C_ASSERT( sizeof(PEB) == sizeof(PEB32) );
C_ASSERT( sizeof(TEB) == sizeof(TEB32) );
#endif

/* reserved TEB64 TLS slots for Wow64 */
#define WOW64_TLS_CPURESERVED      1
#define WOW64_TLS_TEMPLIST         3
#define WOW64_TLS_USERCALLBACKDATA 5
#define WOW64_TLS_APCLIST          7
#define WOW64_TLS_FILESYSREDIR     8
#define WOW64_TLS_WOW64INFO        10
#define WOW64_TLS_MAX_NUMBER       19


/***********************************************************************
 * Enums
 */

typedef enum _FILE_INFORMATION_CLASS {
    FileDirectoryInformation = 1,
    FileFullDirectoryInformation,
    FileBothDirectoryInformation,
    FileBasicInformation,
    FileStandardInformation,
    FileInternalInformation,
    FileEaInformation,
    FileAccessInformation,
    FileNameInformation,
    FileRenameInformation,
    FileLinkInformation,
    FileNamesInformation,
    FileDispositionInformation,
    FilePositionInformation,
    FileFullEaInformation,
    FileModeInformation,
    FileAlignmentInformation,
    FileAllInformation,
    FileAllocationInformation,
    FileEndOfFileInformation,
    FileAlternateNameInformation,
    FileStreamInformation,
    FilePipeInformation,
    FilePipeLocalInformation,
    FilePipeRemoteInformation,
    FileMailslotQueryInformation,
    FileMailslotSetInformation,
    FileCompressionInformation,
    FileObjectIdInformation,
    FileCompletionInformation,
    FileMoveClusterInformation,
    FileQuotaInformation,
    FileReparsePointInformation,
    FileNetworkOpenInformation,
    FileAttributeTagInformation,
    FileTrackingInformation,
    FileIdBothDirectoryInformation,
    FileIdFullDirectoryInformation,
    FileValidDataLengthInformation,
    FileShortNameInformation,
    FileIoCompletionNotificationInformation,
    FileIoStatusBlockRangeInformation,
    FileIoPriorityHintInformation,
    FileSfioReserveInformation,
    FileSfioVolumeInformation,
    FileHardLinkInformation,
    FileProcessIdsUsingFileInformation,
    FileNormalizedNameInformation,
    FileNetworkPhysicalNameInformation,
    FileIdGlobalTxDirectoryInformation,
    FileIsRemoteDeviceInformation,
    FileAttributeCacheInformation,
    FileNumaNodeInformation,
    FileStandardLinkInformation,
    FileRemoteProtocolInformation,
    FileRenameInformationBypassAccessCheck,
    FileLinkInformationBypassAccessCheck,
    FileVolumeNameInformation,
    FileIdInformation,
    FileIdExtdDirectoryInformation,
    FileReplaceCompletionInformation,
    FileHardLinkFullIdInformation,
    FileIdExtdBothDirectoryInformation,
    FileDispositionInformationEx,
    FileRenameInformationEx,
    FileRenameInformationExBypassAccessCheck,
    FileDesiredStorageClassInformation,
    FileStatInformation,
    FileMemoryPartitionInformation,
    FileStatLxInformation,
    FileCaseSensitiveInformation,
    FileLinkInformationEx,
    FileLinkInformationExBypassAccessCheck,
    FileStorageReserveIdInformation,
    FileCaseSensitiveInformationForceAccessCheck,
    FileKnownFolderInformation,
    FileMaximumInformation
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

typedef struct _FILE_DIRECTORY_INFORMATION {
    ULONG               NextEntryOffset;
    ULONG               FileIndex;
    LARGE_INTEGER       CreationTime;
    LARGE_INTEGER       LastAccessTime;
    LARGE_INTEGER       LastWriteTime;
    LARGE_INTEGER       ChangeTime;
    LARGE_INTEGER       EndOfFile;
    LARGE_INTEGER       AllocationSize;
    ULONG               FileAttributes;
    ULONG               FileNameLength;
    WCHAR               FileName[ANYSIZE_ARRAY];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;

typedef struct _FILE_FULL_DIRECTORY_INFORMATION {
    ULONG               NextEntryOffset;
    ULONG               FileIndex;
    LARGE_INTEGER       CreationTime;
    LARGE_INTEGER       LastAccessTime;
    LARGE_INTEGER       LastWriteTime;
    LARGE_INTEGER       ChangeTime;
    LARGE_INTEGER       EndOfFile;
    LARGE_INTEGER       AllocationSize;
    ULONG               FileAttributes;
    ULONG               FileNameLength;
    ULONG               EaSize;
    WCHAR               FileName[ANYSIZE_ARRAY];
} FILE_FULL_DIRECTORY_INFORMATION, *PFILE_FULL_DIRECTORY_INFORMATION,
  FILE_FULL_DIR_INFORMATION, *PFILE_FULL_DIR_INFORMATION;

typedef struct _FILE_ID_FULL_DIRECTORY_INFORMATION {
    ULONG               NextEntryOffset;
    ULONG               FileIndex;
    LARGE_INTEGER       CreationTime;
    LARGE_INTEGER       LastAccessTime;
    LARGE_INTEGER       LastWriteTime;
    LARGE_INTEGER       ChangeTime;
    LARGE_INTEGER       EndOfFile;
    LARGE_INTEGER       AllocationSize;
    ULONG               FileAttributes;
    ULONG               FileNameLength;
    ULONG               EaSize;
    LARGE_INTEGER       FileId;
    WCHAR               FileName[ANYSIZE_ARRAY];
} FILE_ID_FULL_DIRECTORY_INFORMATION, *PFILE_ID_FULL_DIRECTORY_INFORMATION;

typedef struct _FILE_BOTH_DIRECTORY_INFORMATION {
    ULONG               NextEntryOffset;
    ULONG               FileIndex;
    LARGE_INTEGER       CreationTime;
    LARGE_INTEGER       LastAccessTime;
    LARGE_INTEGER       LastWriteTime;
    LARGE_INTEGER       ChangeTime;
    LARGE_INTEGER       EndOfFile;
    LARGE_INTEGER       AllocationSize;
    ULONG               FileAttributes;
    ULONG               FileNameLength;
    ULONG               EaSize;
    CHAR                ShortNameLength;
    WCHAR               ShortName[12];
    WCHAR               FileName[ANYSIZE_ARRAY];
} FILE_BOTH_DIRECTORY_INFORMATION, *PFILE_BOTH_DIRECTORY_INFORMATION,
  FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;

typedef struct _FILE_ID_BOTH_DIRECTORY_INFORMATION {
    ULONG               NextEntryOffset;
    ULONG               FileIndex;
    LARGE_INTEGER       CreationTime;
    LARGE_INTEGER       LastAccessTime;
    LARGE_INTEGER       LastWriteTime;
    LARGE_INTEGER       ChangeTime;
    LARGE_INTEGER       EndOfFile;
    LARGE_INTEGER       AllocationSize;
    ULONG               FileAttributes;
    ULONG               FileNameLength;
    ULONG               EaSize;
    CHAR                ShortNameLength;
    WCHAR               ShortName[12];
    LARGE_INTEGER       FileId;
    WCHAR               FileName[ANYSIZE_ARRAY];
} FILE_ID_BOTH_DIRECTORY_INFORMATION, *PFILE_ID_BOTH_DIRECTORY_INFORMATION;

typedef struct _FILE_ID_GLOBAL_TX_DIR_INFORMATION {
    ULONG               NextEntryOffset;
    ULONG               FileIndex;
    LARGE_INTEGER       CreationTime;
    LARGE_INTEGER       LastAccessTime;
    LARGE_INTEGER       LastWriteTime;
    LARGE_INTEGER       ChangeTime;
    LARGE_INTEGER       EndOfFile;
    LARGE_INTEGER       AllocationSize;
    ULONG               FileAttributes;
    ULONG               FileNameLength;
    LARGE_INTEGER       FileId;
    GUID                LockingTransactionId;
    ULONG               TxInfoFlags;
    WCHAR               FileName[ANYSIZE_ARRAY];
} FILE_ID_GLOBAL_TX_DIR_INFORMATION, *PFILE_ID_GLOBAL_TX_DIR_INFORMATION;

typedef struct _FILE_BASIC_INFORMATION {
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    ULONG FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

typedef struct _FILE_STANDARD_INFORMATION {
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG NumberOfLinks;
    BOOLEAN DeletePending;
    BOOLEAN Directory;
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

typedef struct _FILE_INTERNAL_INFORMATION {
    LARGE_INTEGER IndexNumber;
} FILE_INTERNAL_INFORMATION, *PFILE_INTERNAL_INFORMATION;

typedef struct _FILE_ID_128 {
    UCHAR Identifier[16];
} FILE_ID_128, *PFILE_ID_128;

typedef struct _FILE_ID_INFORMATION {
    ULONGLONG VolumeSerialNumber;
    FILE_ID_128 FileId;
} FILE_ID_INFORMATION, *PFILE_ID_INFORMATION;

typedef struct _FILE_EA_INFORMATION {
    ULONG EaSize;
} FILE_EA_INFORMATION, *PFILE_EA_INFORMATION;

typedef struct _FILE_ACCESS_INFORMATION {
    ACCESS_MASK AccessFlags;
} FILE_ACCESS_INFORMATION, *PFILE_ACCESS_INFORMATION;

typedef struct _FILE_NAME_INFORMATION {
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;

typedef struct _FILE_RENAME_INFORMATION {
    union {
        BOOLEAN ReplaceIfExists;
        ULONG Flags;
    } DUMMYUNIONNAME;
    HANDLE RootDirectory;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_RENAME_INFORMATION, *PFILE_RENAME_INFORMATION;

#define FILE_RENAME_REPLACE_IF_EXISTS                       0x00000001
#define FILE_RENAME_POSIX_SEMANTICS                         0x00000002
#define FILE_RENAME_SUPPRESS_PIN_STATE_INHERITANCE          0x00000004
#define FILE_RENAME_SUPPRESS_STORAGE_RESERVE_INHERITANCE    0x00000008
#define FILE_RENAME_NO_INCREASE_AVAILABLE_SPACE             0x00000010
#define FILE_RENAME_NO_DECREASE_AVAILABLE_SPACE             0x00000020
#define FILE_RENAME_PRESERVE_AVAILABLE_SPACE                0x00000030
#define FILE_RENAME_IGNORE_READONLY_ATTRIBUTE               0x00000040

typedef struct _FILE_LINK_INFORMATION {
    union {
        BOOLEAN ReplaceIfExists;
        ULONG Flags;
    } DUMMYUNIONNAME;
    HANDLE RootDirectory;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_LINK_INFORMATION, *PFILE_LINK_INFORMATION;

#define FILE_LINK_REPLACE_IF_EXISTS                         0x00000001
#define FILE_LINK_POSIX_SEMANTICS                           0x00000002
#define FILE_LINK_SUPPRESS_STORAGE_RESERVE_INHERITANCE      0x00000008
#define FILE_LINK_NO_INCREASE_AVAILABLE_SPACE               0x00000010
#define FILE_LINK_NO_DECREASE_AVAILABLE_SPACE               0x00000020
#define FILE_LINK_PRESERVE_AVAILABLE_SPACE                  0x00000030
#define FILE_LINK_IGNORE_READONLY_ATTRIBUTE                 0x00000040
#define FILE_LINK_FORCE_RESIZE_TARGET_SR                    0x00000080
#define FILE_LINK_FORCE_RESIZE_SOURCE_SR                    0x00000100
#define FILE_LINK_FORCE_RESIZE_SR                           0x00000180

typedef struct _FILE_NAMES_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_NAMES_INFORMATION, *PFILE_NAMES_INFORMATION;

typedef struct _FILE_DISPOSITION_INFORMATION {
    BOOLEAN DoDeleteFile;
} FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION;

typedef struct _FILE_DISPOSITION_INFORMATION_EX {
    ULONG Flags;
} FILE_DISPOSITION_INFORMATION_EX, *PFILE_DISPOSITION_INFORMATION_EX;

#define FILE_DISPOSITION_DO_NOT_DELETE              0x00000000
#define FILE_DISPOSITION_DELETE                     0x00000001
#define FILE_DISPOSITION_POSIX_SEMANTICS            0x00000002
#define FILE_DISPOSITION_FORCE_IMAGE_SECTION_CHECK  0x00000004
#define FILE_DISPOSITION_ON_CLOSE                   0x00000008
#define FILE_DISPOSITION_IGNORE_READONLY_ATTRIBUTE  0x00000010

typedef struct _FILE_POSITION_INFORMATION {
    LARGE_INTEGER CurrentByteOffset;
} FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;

typedef struct _FILE_ALIGNMENT_INFORMATION {
    ULONG AlignmentRequirement;
} FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION;

typedef struct _FILE_ALLOCATION_INFORMATION {
    LARGE_INTEGER AllocationSize;
} FILE_ALLOCATION_INFORMATION, *PFILE_ALLOCATION_INFORMATION;

typedef struct _FILE_END_OF_FILE_INFORMATION {
    LARGE_INTEGER EndOfFile;
} FILE_END_OF_FILE_INFORMATION, *PFILE_END_OF_FILE_INFORMATION;

typedef struct _FILE_NETWORK_OPEN_INFORMATION {
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG FileAttributes;
} FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;

typedef struct _FILE_FULL_EA_INFORMATION {
    ULONG NextEntryOffset;
    UCHAR Flags;
    UCHAR EaNameLength;
    USHORT EaValueLength;
    CHAR EaName[1];
} FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;

typedef struct _FILE_MODE_INFORMATION {
    ULONG Mode;
} FILE_MODE_INFORMATION, *PFILE_MODE_INFORMATION;

typedef struct _FILE_STREAM_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG StreamNameLength;
    LARGE_INTEGER StreamSize;
    LARGE_INTEGER StreamAllocationSize;
    WCHAR StreamName[1];
} FILE_STREAM_INFORMATION, *PFILE_STREAM_INFORMATION;

typedef struct _FILE_ATTRIBUTE_TAG_INFORMATION
{
    ULONG FileAttributes;
    ULONG ReparseTag;
} FILE_ATTRIBUTE_TAG_INFORMATION, *PFILE_ATTRIBUTE_TAG_INFORMATION;

typedef struct _FILE_MAILSLOT_QUERY_INFORMATION {
    ULONG MaximumMessageSize;
    ULONG MailslotQuota;
    ULONG NextMessageSize;
    ULONG MessagesAvailable;
    LARGE_INTEGER ReadTimeout;
} FILE_MAILSLOT_QUERY_INFORMATION, *PFILE_MAILSLOT_QUERY_INFORMATION;

typedef struct _FILE_MAILSLOT_SET_INFORMATION {
    LARGE_INTEGER ReadTimeout;
} FILE_MAILSLOT_SET_INFORMATION, *PFILE_MAILSLOT_SET_INFORMATION;

typedef struct _FILE_PIPE_INFORMATION {
    ULONG ReadMode;
    ULONG CompletionMode;
} FILE_PIPE_INFORMATION, *PFILE_PIPE_INFORMATION;

typedef struct _FILE_PIPE_LOCAL_INFORMATION {
    ULONG NamedPipeType;
    ULONG NamedPipeConfiguration;
    ULONG MaximumInstances;
    ULONG CurrentInstances;
    ULONG InboundQuota;
    ULONG ReadDataAvailable;
    ULONG OutboundQuota;
    ULONG WriteQuotaAvailable;
    ULONG NamedPipeState;
    ULONG NamedPipeEnd;
} FILE_PIPE_LOCAL_INFORMATION, *PFILE_PIPE_LOCAL_INFORMATION;

#define FILE_PIPE_DISCONNECTED_STATE        0x00000001
#define FILE_PIPE_LISTENING_STATE           0x00000002
#define FILE_PIPE_CONNECTED_STATE           0x00000003
#define FILE_PIPE_CLOSING_STATE             0x00000004

typedef struct _FILE_OBJECTID_BUFFER
{
    BYTE ObjectId[16];
    union
    {
        struct
        {
            BYTE BirthVolumeId[16];
            BYTE BirthObjectId[16];
            BYTE DomainId[16];
        } DUMMYSTRUCTNAME;
        BYTE ExtendedInfo[48];
    } DUMMYUNIONNAME;
} FILE_OBJECTID_BUFFER, *PFILE_OBJECTID_BUFFER;

typedef struct _FILE_OBJECTID_INFORMATION {
    LONGLONG FileReference;
    UCHAR ObjectId[16];
    union {
        struct {
            UCHAR BirthVolumeId[16];
            UCHAR BirthObjectId[16];
            UCHAR DomainId[16];
        } DUMMYSTRUCTNAME;
        UCHAR ExtendedInfo[48];
    } DUMMYUNIONNAME;
} FILE_OBJECTID_INFORMATION, *PFILE_OBJECTID_INFORMATION;

typedef struct _FILE_QUOTA_INFORMATION {
    ULONG NextEntryOffset;
    ULONG SidLength;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER QuotaUsed;
    LARGE_INTEGER QuotaThreshold;
    LARGE_INTEGER QuotaLimit;
    SID Sid;
} FILE_QUOTA_INFORMATION, *PFILE_QUOTA_INFORMATION;

typedef struct _FILE_REPARSE_POINT_INFORMATION {
    LONGLONG FileReference;
    ULONG Tag;
} FILE_REPARSE_POINT_INFORMATION, *PFILE_REPARSE_POINT_INFORMATION;

typedef struct _FILE_ALL_INFORMATION {
    FILE_BASIC_INFORMATION     BasicInformation;
    FILE_STANDARD_INFORMATION  StandardInformation;
    FILE_INTERNAL_INFORMATION  InternalInformation;
    FILE_EA_INFORMATION        EaInformation;
    FILE_ACCESS_INFORMATION    AccessInformation;
    FILE_POSITION_INFORMATION  PositionInformation;
    FILE_MODE_INFORMATION      ModeInformation;
    FILE_ALIGNMENT_INFORMATION AlignmentInformation;
    FILE_NAME_INFORMATION      NameInformation;
} FILE_ALL_INFORMATION, *PFILE_ALL_INFORMATION;

typedef struct _FILE_STAT_INFORMATION {
    LARGE_INTEGER FileId;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG FileAttributes;
    ULONG ReparseTag;
    ULONG NumberOfLinks;
    ULONG EffectiveAccess;
} FILE_STAT_INFORMATION, *PFILE_STAT_INFORMATION;

typedef struct _FILE_IO_COMPLETION_NOTIFICATION_INFORMATION {
    ULONG Flags;
} FILE_IO_COMPLETION_NOTIFICATION_INFORMATION, *PFILE_IO_COMPLETION_NOTIFICATION_INFORMATION;

#define FILE_SKIP_COMPLETION_PORT_ON_SUCCESS 0x1
#define FILE_SKIP_SET_EVENT_ON_HANDLE        0x2
#define FILE_SKIP_SET_USER_EVENT_ON_FAST_IO  0x4

typedef struct _PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION
{
    ULONG  Version;
    ULONG  Reserved;
    VOID  *Callback;
} PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION, *PPROCESS_INSTRUMENTATION_CALLBACK_INFORMATION;

typedef enum _FSINFOCLASS {
    FileFsVolumeInformation = 1,
    FileFsLabelInformation,
    FileFsSizeInformation,
    FileFsDeviceInformation,
    FileFsAttributeInformation,
    FileFsControlInformation,
    FileFsFullSizeInformation,
    FileFsObjectIdInformation,
    FileFsDriverPathInformation,
    FileFsVolumeFlagsInformation,
    FileFsSectorSizeInformation,
    FileFsDataCopyInformation,
    FileFsMetadataSizeInformation,
    FileFsFullSizeInformationEx,
    FileFsMaximumInformation
} FS_INFORMATION_CLASS, *PFS_INFORMATION_CLASS;

typedef enum _KEY_INFORMATION_CLASS {
    KeyBasicInformation,
    KeyNodeInformation,
    KeyFullInformation,
    KeyNameInformation,
    KeyCachedInformation,
    KeyFlagsInformation,
    KeyVirtualizationInformation,
    KeyHandleTagsInformation,
    KeyTrustInformation,
    KeyLayerInformation,
    MaxKeyInfoClass
} KEY_INFORMATION_CLASS;

typedef enum _KEY_VALUE_INFORMATION_CLASS {
    KeyValueBasicInformation,
    KeyValueFullInformation,
    KeyValuePartialInformation,
    KeyValueFullInformationAlign64,
    KeyValuePartialInformationAlign64,
    KeyValueLayerInformation,
} KEY_VALUE_INFORMATION_CLASS;

typedef enum _OBJECT_INFORMATION_CLASS {
    ObjectBasicInformation,
    ObjectNameInformation,
    ObjectTypeInformation,
    ObjectTypesInformation,
    ObjectHandleFlagInformation,
    ObjectSessionInformation,
    ObjectSessionObjectInformation,
} OBJECT_INFORMATION_CLASS, *POBJECT_INFORMATION_CLASS;

typedef enum _PROCESSINFOCLASS {
    ProcessBasicInformation = 0,
    ProcessQuotaLimits = 1,
    ProcessIoCounters = 2,
    ProcessVmCounters = 3,
    ProcessTimes = 4,
    ProcessBasePriority = 5,
    ProcessRaisePriority = 6,
    ProcessDebugPort = 7,
    ProcessExceptionPort = 8,
    ProcessAccessToken = 9,
    ProcessLdtInformation = 10,
    ProcessLdtSize = 11,
    ProcessDefaultHardErrorMode = 12,
    ProcessIoPortHandlers = 13,
    ProcessPooledUsageAndLimits = 14,
    ProcessWorkingSetWatch = 15,
    ProcessUserModeIOPL = 16,
    ProcessEnableAlignmentFaultFixup = 17,
    ProcessPriorityClass = 18,
    ProcessWx86Information = 19,
    ProcessHandleCount = 20,
    ProcessAffinityMask = 21,
    ProcessPriorityBoost = 22,
    ProcessDeviceMap = 23,
    ProcessSessionInformation = 24,
    ProcessForegroundInformation = 25,
    ProcessWow64Information = 26,
    ProcessImageFileName = 27,
    ProcessLUIDDeviceMapsEnabled = 28,
    ProcessBreakOnTermination = 29,
    ProcessDebugObjectHandle = 30,
    ProcessDebugFlags = 31,
    ProcessHandleTracing = 32,
    ProcessIoPriority = 33,
    ProcessExecuteFlags = 34,
    ProcessTlsInformation = 35,
    ProcessCookie = 36,
    ProcessImageInformation = 37,
    ProcessCycleTime = 38,
    ProcessPagePriority = 39,
    ProcessInstrumentationCallback = 40,
    ProcessThreadStackAllocation = 41,
    ProcessWorkingSetWatchEx = 42,
    ProcessImageFileNameWin32 = 43,
    ProcessImageFileMapping = 44,
    ProcessAffinityUpdateMode = 45,
    ProcessMemoryAllocationMode = 46,
    ProcessGroupInformation = 47,
    ProcessTokenVirtualizationEnabled = 48,
    ProcessConsoleHostProcess = 49,
    ProcessWindowInformation = 50,
    ProcessHandleInformation = 51,
    ProcessMitigationPolicy = 52,
    ProcessDynamicFunctionTableInformation = 53,
    ProcessHandleCheckingMode = 54,
    ProcessKeepAliveCount = 55,
    ProcessRevokeFileHandles = 56,
    ProcessWorkingSetControl = 57,
    ProcessHandleTable = 58,
    ProcessCheckStackExtentsMode = 59,
    ProcessCommandLineInformation = 60,
    ProcessProtectionInformation = 61,
    ProcessMemoryExhaustion = 62,
    ProcessFaultInformation = 63,
    ProcessTelemetryIdInformation = 64,
    ProcessCommitReleaseInformation = 65,
    ProcessDefaultCpuSetsInformation = 66,
    ProcessAllowedCpuSetsInformation = 67,
    ProcessSubsystemProcess = 68,
    ProcessJobMemoryInformation = 69,
    ProcessInPrivate = 70,
    ProcessRaiseUMExceptionOnInvalidHandleClose = 71,
    ProcessIumChallengeResponse = 72,
    ProcessChildProcessInformation = 73,
    ProcessHighGraphicsPriorityInformation = 74,
    ProcessSubsystemInformation = 75,
    ProcessEnergyValues = 76,
    ProcessPowerThrottlingState = 77,
    ProcessReserved3Information = 78,
    ProcessWin32kSyscallFilterInformation = 79,
    ProcessDisableSystemAllowedCpuSets = 80,
    ProcessWakeInformation = 81,
    ProcessEnergyTrackingState = 82,
    ProcessManageWritesToExecutableMemory = 83,
    ProcessCaptureTrustletLiveDump = 84,
    ProcessTelemetryCoverage = 85,
    ProcessEnclaveInformation = 86,
    ProcessEnableReadWriteVmLogging = 87,
    ProcessUptimeInformation = 88,
    ProcessImageSection = 89,
    ProcessDebugAuthInformation = 90,
    ProcessSystemResourceManagement = 91,
    ProcessSequenceNumber = 92,
    ProcessLoaderDetour = 93,
    ProcessSecurityDomainInformation = 94,
    ProcessCombineSecurityDomainsInformation = 95,
    ProcessEnableLogging = 96,
    ProcessLeapSecondInformation = 97,
    ProcessFiberShadowStackAllocation = 98,
    ProcessFreeFiberShadowStackAllocation = 99,
    ProcessAltSystemCallInformation = 100,
    ProcessDynamicEHContinuationTargets = 101,
    ProcessDynamicEnforcedCetCompatibleRanges = 102,
    ProcessCreateStateChange = 103,
    ProcessApplyStateChange = 104,
    ProcessEnableOptionalXStateFeatures = 105,
    ProcessAltPrefetchParam = 106,
    ProcessAssignCpuPartitions = 107,
    ProcessPriorityClassEx = 108,
    ProcessMembershipInformation = 109,
    ProcessEffectiveIoPriority = 110,
    ProcessEffectivePagePriority = 111,
    MaxProcessInfoClass,
#ifdef __WINESRC__
    ProcessWineMakeProcessSystem = 1000,
    ProcessWineLdtCopy,
    ProcessWineGrantAdminToken,
#endif
} PROCESSINFOCLASS;

#define MEM_EXECUTE_OPTION_DISABLE   0x01
#define MEM_EXECUTE_OPTION_ENABLE    0x02
#define MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION 0x04
#define MEM_EXECUTE_OPTION_PERMANENT 0x08

typedef enum _SECTION_INHERIT {
    ViewShare = 1,
    ViewUnmap = 2
} SECTION_INHERIT;

typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation = 0,
    SystemCpuInformation = 1,
    SystemPerformanceInformation = 2,
    SystemTimeOfDayInformation = 3, /* was SystemTimeInformation */
    SystemPathInformation = 4,
    SystemProcessInformation = 5,
    SystemCallCountInformation = 6,
    SystemDeviceInformation = 7,
    SystemProcessorPerformanceInformation = 8,
    SystemFlagsInformation = 9,
    SystemCallTimeInformation = 10,
    SystemModuleInformation = 11,
    SystemLocksInformation = 12,
    SystemStackTraceInformation = 13,
    SystemPagedPoolInformation = 14,
    SystemNonPagedPoolInformation = 15,
    SystemHandleInformation = 16,
    SystemObjectInformation = 17,
    SystemPageFileInformation = 18,
    SystemVdmInstemulInformation = 19,
    SystemVdmBopInformation = 20,
    SystemFileCacheInformation = 21,
    SystemPoolTagInformation = 22,
    SystemInterruptInformation = 23,
    SystemDpcBehaviorInformation = 24,
    SystemFullMemoryInformation = 25,
    SystemNotImplemented6 = 25,
    SystemLoadGdiDriverInformation = 26,
    SystemUnloadGdiDriverInformation = 27,
    SystemTimeAdjustmentInformation = 28,
    SystemTimeAdjustment = 28,
    SystemSummaryMemoryInformation = 29,
    SystemMirrorMemoryInformation = 30,
    SystemPerformanceTraceInformation = 31,
    SystemObsolete0 = 32,
    SystemExceptionInformation = 33,
    SystemCrashDumpStateInformation = 34,
    SystemKernelDebuggerInformation = 35,
    SystemContextSwitchInformation = 36,
    SystemRegistryQuotaInformation = 37,
    SystemExtendServiceTableInformation = 38,
    SystemPrioritySeparation = 39,
    SystemVerifierAddDriverInformation = 40,
    SystemVerifierRemoveDriverInformation = 41,
    SystemProcessorIdleInformation = 42,
    SystemLegacyDriverInformation = 43,
    SystemCurrentTimeZoneInformation = 44,
    SystemLookasideInformation = 45,
    SystemTimeSlipNotification = 46,
    SystemSessionCreate = 47,
    SystemSessionDetach = 48,
    SystemSessionInformation = 49,
    SystemRangeStartInformation = 50,
    SystemVerifierInformation = 51,
    SystemVerifierThunkExtend = 52,
    SystemSessionProcessesInformation	= 53,
    SystemLoadGdiDriverInSystemSpace = 54,
    SystemNumaProcessorMap = 55,
    SystemPrefetcherInformation = 56,
    SystemExtendedProcessInformation = 57,
    SystemRecommendedSharedDataAlignment = 58,
    SystemComPlusPackage = 59,
    SystemNumaAvailableMemory = 60,
    SystemProcessorPowerInformation = 61,
    SystemEmulationBasicInformation = 62,
    SystemEmulationProcessorInformation = 63,
    SystemExtendedHandleInformation = 64,
    SystemLostDelayedWriteInformation = 65,
    SystemBigPoolInformation = 66,
    SystemSessionPoolTagInformation = 67,
    SystemSessionMappedViewInformation = 68,
    SystemHotpatchInformation = 69,
    SystemObjectSecurityMode = 70,
    SystemWatchdogTimerHandler = 71,
    SystemWatchdogTimerInformation = 72,
    SystemLogicalProcessorInformation = 73,
    SystemWow64SharedInformationObsolete = 74,
    SystemRegisterFirmwareTableInformationHandler = 75,
    SystemFirmwareTableInformation = 76,
    SystemModuleInformationEx = 77,
    SystemVerifierTriageInformation = 78,
    SystemSuperfetchInformation = 79,
    SystemMemoryListInformation = 80,
    SystemFileCacheInformationEx = 81,
    SystemThreadPriorityClientIdInformation = 82,
    SystemProcessorIdleCycleTimeInformation = 83,
    SystemVerifierCancellationInformation = 84,
    SystemProcessorPowerInformationEx = 85,
    SystemRefTraceInformation = 86,
    SystemSpecialPoolInformation = 87,
    SystemProcessIdInformation = 88,
    SystemErrorPortInformation = 89,
    SystemBootEnvironmentInformation = 90,
    SystemHypervisorInformation = 91,
    SystemVerifierInformationEx = 92,
    SystemTimeZoneInformation = 93,
    SystemImageFileExecutionOptionsInformation = 94,
    SystemCoverageInformation = 95,
    SystemPrefetchPatchInformation = 96,
    SystemVerifierFaultsInformation = 97,
    SystemSystemPartitionInformation = 98,
    SystemSystemDiskInformation = 99,
    SystemProcessorPerformanceDistribution = 100,
    SystemNumaProximityNodeInformation = 101,
    SystemDynamicTimeZoneInformation = 102,
    SystemCodeIntegrityInformation = 103,
    SystemProcessorMicrocodeUpdateInformation = 104,
    SystemProcessorBrandString = 105,
    SystemVirtualAddressInformation = 106,
    SystemLogicalProcessorInformationEx = 107,
    SystemProcessorCycleTimeInformation = 108,
    SystemStoreInformation = 109,
    SystemRegistryAppendString = 110,
    SystemAitSamplingValue = 111,
    SystemVhdBootInformation = 112,
    SystemCpuQuotaInformation = 113,
    SystemNativeBasicInformation = 114,
    SystemErrorPortTimeouts = 115,
    SystemLowPriorityIoInformation = 116,
    SystemTpmBootEntropyInformation = 117,
    SystemVerifierCountersInformation = 118,
    SystemPagedPoolInformationEx = 119,
    SystemSystemPtesInformationEx = 120,
    SystemNodeDistanceInformation = 121,
    SystemAcpiAuditInformation = 122,
    SystemBasicPerformanceInformation = 123,
    SystemQueryPerformanceCounterInformation = 124,
    SystemSessionBigPoolInformation = 125,
    SystemBootGraphicsInformation = 126,
    SystemScrubPhysicalMemoryInformation = 127,
    SystemBadPageInformation = 128,
    SystemProcessorProfileControlArea = 129,
    SystemCombinePhysicalMemoryInformation = 130,
    SystemEntropyInterruptTimingInformation = 131,
    SystemConsoleInformation = 132,
    SystemPlatformBinaryInformation = 133,
    SystemPolicyInformation = 134,
    SystemHypervisorProcessorCountInformation = 135,
    SystemDeviceDataInformation = 136,
    SystemDeviceDataEnumerationInformation = 137,
    SystemMemoryTopologyInformation = 138,
    SystemMemoryChannelInformation = 139,
    SystemBootLogoInformation = 140,
    SystemProcessorPerformanceInformationEx = 141,
    SystemCriticalProcessErrorLogInformation = 142,
    SystemSecureBootPolicyInformation = 143,
    SystemPageFileInformationEx = 144,
    SystemSecureBootInformation = 145,
    SystemEntropyInterruptTimingRawInformation = 146,
    SystemPortableWorkspaceEfiLauncherInformation = 147,
    SystemFullProcessInformation = 148,
    SystemKernelDebuggerInformationEx = 149,
    SystemBootMetadataInformation = 150,
    SystemSoftRebootInformation = 151,
    SystemElamCertificateInformation = 152,
    SystemOfflineDumpConfigInformation = 153,
    SystemProcessorFeaturesInformation = 154,
    SystemRegistryReconciliationInformation = 155,
    SystemEdidInformation = 156,
    SystemManufacturingInformation = 157,
    SystemEnergyEstimationConfigInformation = 158,
    SystemHypervisorDetailInformation = 159,
    SystemProcessorCycleStatsInformation = 160,
    SystemVmGenerationCountInformation = 161,
    SystemTrustedPlatformModuleInformation = 162,
    SystemKernelDebuggerFlags = 163,
    SystemCodeIntegrityPolicyInformation = 164,
    SystemIsolatedUserModeInformation = 165,
    SystemHardwareSecurityTestInterfaceResultsInformation = 166,
    SystemSingleModuleInformation = 167,
    SystemAllowedCpuSetsInformation = 168,
    SystemVsmProtectionInformation = 169,
    SystemInterruptCpuSetsInformation = 170,
    SystemSecureBootPolicyFullInformation = 171,
    SystemCodeIntegrityPolicyFullInformation = 172,
    SystemAffinitizedInterruptProcessorInformation = 173,
    SystemRootSiloInformation = 174,
    SystemCpuSetInformation = 175,
    SystemCpuSetTagInformation = 176,
    SystemWin32WerStartCallout = 177,
    SystemSecureKernelProfileInformation = 178,
    SystemCodeIntegrityPlatformManifestInformation = 179,
    SystemInterruptSteeringInformation = 180,
    SystemSupportedProcessorArchitectures = 181,
    SystemMemoryUsageInformation = 182,
    SystemCodeIntegrityCertificateInformation = 183,
    SystemPhysicalMemoryInformation = 184,
    SystemControlFlowTransition = 185,
    SystemKernelDebuggingAllowed = 186,
    SystemActivityModerationExeState = 187,
    SystemActivityModerationUserSettings = 188,
    SystemCodeIntegrityPoliciesFullInformation = 189,
    SystemCodeIntegrityUnlockInformation = 190,
    SystemIntegrityQuotaInformation = 191,
    SystemFlushInformation = 192,
    SystemProcessorIdleMaskInformation = 193,
    SystemSecureDumpEncryptionInformation = 194,
    SystemWriteConstraintInformation = 195,
    SystemKernelVaShadowInformation = 196,
    SystemHypervisorSharedPageInformation = 197,
    SystemFirmwareBootPerformanceInformation = 198,
    SystemCodeIntegrityVerificationInformation = 199,
    SystemFirmwarePartitionInformation = 200,
    SystemSpeculationControlInformation = 201,
    SystemDmaGuardPolicyInformation = 202,
    SystemEnclaveLaunchControlInformation = 203,
    SystemWorkloadAllowedCpuSetsInformation = 204,
    SystemCodeIntegrityUnlockModeInformation = 205,
    SystemLeapSecondInformation = 206,
    SystemFlags2Information = 207,
    SystemSecurityModelInformation = 208,
    SystemCodeIntegritySyntheticCacheInformation = 209,
    SystemFeatureConfigurationInformation = 210,
    SystemFeatureConfigurationSectionInformation = 211,
    SystemFeatureUsageSubscriptionInformation = 212,
    SystemSecureSpeculationControlInformation = 213,
    SystemSpacesBootInformation = 214,
    SystemFwRamdiskInformation = 215,
    SystemWheaIpmiHardwareInformation = 216,
    SystemDifSetRuleClassInformation = 217,
    SystemDifClearRuleClassInformation = 218,
    SystemDifApplyPluginVerificationOnDriver = 219,
    SystemDifRemovePluginVerificationOnDriver = 220,
    SystemShadowStackInformation = 221,
    SystemBuildVersionInformation = 222,
    SystemPoolLimitInformation = 223,
    SystemCodeIntegrityAddDynamicStore = 224,
    SystemCodeIntegrityClearDynamicStores = 225,
    SystemDifPoolTrackingInformation = 226,
    SystemPoolZeroingInformation = 227,
    SystemDpcWatchdogInformation = 228,
    SystemDpcWatchdogInformation2 = 229,
    SystemSupportedProcessorArchitectures2 = 230,
    SystemSingleProcessorRelationshipInformation = 231,
    SystemXfgCheckFailureInformation = 232,
    SystemIommuStateInformation = 233,
    SystemHypervisorMinrootInformation = 234,
    SystemHypervisorBootPagesInformation = 235,
    SystemPointerAuthInformation = 236,
    SystemSecureKernelDebuggerInformation = 237,
    SystemOriginalImageFeatureInformation = 238,
#ifdef __WINESRC__
    SystemWineVersionInformation = 1000,
#endif
} SYSTEM_INFORMATION_CLASS, *PSYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_CODEINTEGRITY_INFORMATION
{
    ULONG Length;
    ULONG CodeIntegrityOptions;
} SYSTEM_CODEINTEGRITY_INFORMATION, *PSYSTEM_CODEINTEGRITY_INFORMATION;

#define CODEINTEGRITY_OPTION_ENABLED                      0x0001
#define CODEINTEGRITY_OPTION_TESTSIGN                     0x0002
#define CODEINTEGRITY_OPTION_UMCI_ENABLED                 0x0004
#define CODEINTEGRITY_OPTION_UMCI_AUDITMODE_ENABLED       0x0008
#define CODEINTEGRITY_OPTION_UMCI_EXCLUSIONPATHS_ENABLED  0x0010
#define CODEINTEGRITY_OPTION_TEST_BUILD                   0x0020
#define CODEINTEGRITY_OPTION_PREPRODUCTION_BUILD          0x0040
#define CODEINTEGRITY_OPTION_DEBUGMODE_ENABLED            0x0080
#define CODEINTEGRITY_OPTION_FLIGHT_BUILD                 0x0100
#define CODEINTEGRITY_OPTION_FLIGHTING_ENABLED            0x0200
#define CODEINTEGRITY_OPTION_HVCI_KMCI_ENABLED            0x0400
#define CODEINTEGRITY_OPTION_HVCI_KMCI_AUDITMODE_ENABLED  0x0800
#define CODEINTEGRITY_OPTION_HVCI_KMCI_STRICTMODE_ENABLED 0x1000
#define CODEINTEGRITY_OPTION_HVCI_IUM_ENABLED             0x2000

typedef enum _THREADINFOCLASS {
    ThreadBasicInformation = 0,
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
    ThreadCpuAccountingInformation,
    ThreadSuspendCount,
    ThreadHeterogeneousCpuPolicy,
    ThreadContainerId,
    ThreadNameInformation,
    ThreadSelectedCpuSets,
    ThreadSystemThreadInformation,
    ThreadActualGroupAffinity,
    ThreadDynamicCodePolicyInfo,
    ThreadExplicitCaseSensitivity,
    ThreadWorkOnBehalfTicket,
    ThreadSubsystemInformation,
    ThreadDbgkWerReportActive,
    ThreadAttachContainer,
    ThreadManageWritesToExecutableMemory,
    ThreadPowerThrottlingState,
    ThreadWorkloadClass,
    ThreadCreateStateChange,
    ThreadApplyStateChange,
    ThreadStrongerBadHandleChecks,
    ThreadEffectiveIoPriority,
    ThreadEffectivePagePriority,
    MaxThreadInfoClass,
#ifdef __WINESRC__
    ThreadWineNativeThreadName = 1000,
#endif
} THREADINFOCLASS;

typedef struct _THREAD_BASIC_INFORMATION
{
    NTSTATUS  ExitStatus;
    PVOID     TebBaseAddress;
    CLIENT_ID ClientId;
    ULONG_PTR AffinityMask;
    LONG      Priority;
    LONG      BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

typedef struct _THREAD_DESCRIPTOR_INFORMATION
{
    DWORD       Selector;
    LDT_ENTRY   Entry;
} THREAD_DESCRIPTOR_INFORMATION, *PTHREAD_DESCRIPTOR_INFORMATION;

typedef struct _THREAD_NAME_INFORMATION
{
    UNICODE_STRING ThreadName;
} THREAD_NAME_INFORMATION, *PTHREAD_NAME_INFORMATION;

typedef struct _MANAGE_WRITES_TO_EXECUTABLE_MEMORY
{
    ULONG Version : 8;
    ULONG ProcessEnableWriteExceptions : 1;
    ULONG ThreadAllowWrites : 1;
    ULONG Spare : 22;
    PVOID KernelWriteToExecutableSignal;
} MANAGE_WRITES_TO_EXECUTABLE_MEMORY, *PMANAGE_WRITES_TO_EXECUTABLE_MEMORY;

typedef struct _KERNEL_USER_TIMES {
    LARGE_INTEGER  CreateTime;
    LARGE_INTEGER  ExitTime;
    LARGE_INTEGER  KernelTime;
    LARGE_INTEGER  UserTime;
} KERNEL_USER_TIMES, *PKERNEL_USER_TIMES;

typedef enum _WINSTATIONINFOCLASS {
    WinStationInformation = 8
} WINSTATIONINFOCLASS;

typedef enum _MEMORY_INFORMATION_CLASS {
    MemoryBasicInformation,
    MemoryWorkingSetInformation,
    MemoryMappedFilenameInformation,
    MemoryRegionInformation,
    MemoryWorkingSetExInformation,
    MemorySharedCommitInformation,
    MemoryImageInformation,
    MemoryRegionInformationEx,
    MemoryPrivilegedBasicInformation,
    MemoryEnclaveImageInformation,
    MemoryBasicInformationCapped,
    MemoryPhysicalContiguityInformation,
    MemoryBadInformation,
    MemoryBadInformationAllProcesses,
#ifdef __WINESRC__
    MemoryWineUnixFuncs = 1000,
    MemoryWineUnixWow64Funcs,
#endif
} MEMORY_INFORMATION_CLASS;

typedef struct _MEMORY_SECTION_NAME
{
    UNICODE_STRING SectionFileName;
} MEMORY_SECTION_NAME, *PMEMORY_SECTION_NAME;

typedef union _MEMORY_WORKING_SET_EX_BLOCK {
    ULONG_PTR Flags;
    struct {
        ULONG_PTR Valid : 1;
        ULONG_PTR ShareCount : 3;
        ULONG_PTR Win32Protection : 11;
        ULONG_PTR Shared : 1;
        ULONG_PTR Node : 6;
        ULONG_PTR Locked : 1;
        ULONG_PTR LargePage : 1;
    } DUMMYSTRUCTNAME;
} MEMORY_WORKING_SET_EX_BLOCK, *PMEMORY_WORKING_SET_EX_BLOCK;

typedef struct _MEMORY_WORKING_SET_EX_INFORMATION {
    PVOID                       VirtualAddress;
    MEMORY_WORKING_SET_EX_BLOCK VirtualAttributes;
} MEMORY_WORKING_SET_EX_INFORMATION, *PMEMORY_WORKING_SET_EX_INFORMATION;

typedef struct _MEMORY_REGION_INFORMATION
{
    PVOID AllocationBase;
    ULONG AllocationProtect;
    union
    {
        ULONG RegionType;
        struct
        {
            ULONG Private : 1;
            ULONG MappedDataFile : 1;
            ULONG MappedImage : 1;
            ULONG MappedPageFile : 1;
            ULONG MappedPhysical : 1;
            ULONG DirectMapped : 1;
            ULONG Reserved : 26;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    SIZE_T RegionSize;
    SIZE_T CommitSize;
    ULONG_PTR PartitionId;
    ULONG_PTR NodePreference;
} MEMORY_REGION_INFORMATION, *PMEMORY_REGION_INFORMATION;

typedef struct _MEMORY_IMAGE_INFORMATION
{
    PVOID ImageBase;
    SIZE_T SizeOfImage;
    union
    {
        ULONG ImageFlags;
        struct
        {
            ULONG ImagePartialMap : 1;
            ULONG ImageNotExecutable : 1;
            ULONG ImageSigningLevel : 4;
            ULONG Reserved : 26;
        };
    };
} MEMORY_IMAGE_INFORMATION, *PMEMORY_IMAGE_INFORMATION;

typedef enum _MUTANT_INFORMATION_CLASS
{
    MutantBasicInformation
} MUTANT_INFORMATION_CLASS, *PMUTANT_INFORMATION_CLASS;

typedef struct _MUTANT_BASIC_INFORMATION {
    LONG        CurrentCount;
    BOOLEAN     OwnedByCaller;
    BOOLEAN     AbandonedState;
} MUTANT_BASIC_INFORMATION, *PMUTANT_BASIC_INFORMATION;

typedef enum _TIMER_INFORMATION_CLASS
{
    TimerBasicInformation = 0
} TIMER_INFORMATION_CLASS;

typedef struct _TIMER_BASIC_INFORMATION
{
    LARGE_INTEGER RemainingTime;
    BOOLEAN       TimerState;
} TIMER_BASIC_INFORMATION, *PTIMER_BASIC_INFORMATION;

typedef enum
{
    VmPrefetchInformation,
    VmPagePriorityInformation,
    VmCfgCallTargetInformation,
    VmPageDirtyStateInformation,
    VmImageHotPatchInformation,
    VmPhysicalContiguityInformation,
    VmVirtualMachinePrepopulateInformation,
    VmRemoveFromWorkingSetInformation,
} VIRTUAL_MEMORY_INFORMATION_CLASS, *PVIRTUAL_MEMORY_INFORMATION_CLASS;

typedef struct _MEMORY_RANGE_ENTRY
{
    PVOID  VirtualAddress;
    SIZE_T NumberOfBytes;
} MEMORY_RANGE_ENTRY, *PMEMORY_RANGE_ENTRY;


/* return type of RtlDetermineDosPathNameType_U (FIXME: not the correct names) */
typedef enum
{
    INVALID_PATH = 0,
    UNC_PATH,              /* "//foo" */
    ABSOLUTE_DRIVE_PATH,   /* "c:/foo" */
    RELATIVE_DRIVE_PATH,   /* "c:foo" */
    ABSOLUTE_PATH,         /* "/foo" */
    RELATIVE_PATH,         /* "foo" */
    DEVICE_PATH,           /* "//./foo" */
    UNC_DOT_PATH           /* "//." */
} DOS_PATHNAME_TYPE;


/***********************************************************************
 * Types and data structures
 */

/* This is used by NtQuerySystemInformation */
typedef struct _SYSTEM_THREAD_INFORMATION
{                                    /* win32/win64 */
    LARGE_INTEGER KernelTime;          /* 00/00 */
    LARGE_INTEGER UserTime;            /* 08/08 */
    LARGE_INTEGER CreateTime;          /* 10/10 */
    DWORD         dwTickCount;         /* 18/18 */
    LPVOID        StartAddress;        /* 1c/20 */
    CLIENT_ID     ClientId;            /* 20/28 */
    DWORD         dwCurrentPriority;   /* 28/38 */
    DWORD         dwBasePriority;      /* 2c/3c */
    DWORD         dwContextSwitches;   /* 30/40 */
    DWORD         dwThreadState;       /* 34/44 */
    DWORD         dwWaitReason;        /* 38/48 */
    DWORD         dwUnknown;           /* 3c/4c */
} SYSTEM_THREAD_INFORMATION, *PSYSTEM_THREAD_INFORMATION;

typedef struct _SYSTEM_EXTENDED_THREAD_INFORMATION
{
    SYSTEM_THREAD_INFORMATION ThreadInfo;          /* 00/00 */
    void                     *StackBase;           /* 40/50 */
    void                     *StackLimit;          /* 44/58 */
    void                     *Win32StartAddress;   /* 48/60 */
    void                     *TebBase;             /* 4c/68 */
    ULONG_PTR                 Reserved2;           /* 50/70 */
    ULONG_PTR                 Reserved3;           /* 54/78 */
    ULONG_PTR                 Reserved4;           /* 58/80 */
} SYSTEM_EXTENDED_THREAD_INFORMATION, *PSYSTEM_EXTENDED_THREAD_INFORMATION;

typedef struct _IO_STATUS_BLOCK {
  union {
    NTSTATUS Status;
    PVOID Pointer;
  } DUMMYUNIONNAME;

  ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef void (WINAPI * PIO_APC_ROUTINE)(PVOID,PIO_STATUS_BLOCK,ULONG);

typedef struct _KEY_BASIC_INFORMATION {
    LARGE_INTEGER LastWriteTime;
    ULONG         TitleIndex;
    ULONG         NameLength;
    WCHAR         Name[1];
} KEY_BASIC_INFORMATION, *PKEY_BASIC_INFORMATION;

typedef struct _KEY_NODE_INFORMATION
{
    LARGE_INTEGER LastWriteTime;
    ULONG         TitleIndex;
    ULONG         ClassOffset;
    ULONG         ClassLength;
    ULONG         NameLength;
    WCHAR         Name[1];
   /* Class[1]; */
} KEY_NODE_INFORMATION, *PKEY_NODE_INFORMATION;

typedef struct _KEY_FULL_INFORMATION
{
    LARGE_INTEGER LastWriteTime;
    ULONG         TitleIndex;
    ULONG         ClassOffset;
    ULONG         ClassLength;
    ULONG         SubKeys;
    ULONG         MaxNameLen;
    ULONG         MaxClassLen;
    ULONG         Values;
    ULONG         MaxValueNameLen;
    ULONG         MaxValueDataLen;
    WCHAR         Class[1];
} KEY_FULL_INFORMATION, *PKEY_FULL_INFORMATION;

typedef struct _KEY_NAME_INFORMATION {
    ULONG         NameLength;
    WCHAR         Name[1];
} KEY_NAME_INFORMATION, *PKEY_NAME_INFORMATION;

typedef struct _KEY_CACHED_INFORMATION
{
    LARGE_INTEGER LastWriteTime;
    ULONG         TitleIndex;
    ULONG         SubKeys;
    ULONG         MaxNameLen;
    ULONG         Values;
    ULONG         MaxValueNameLen;
    ULONG         MaxValueDataLen;
    ULONG         NameLength;
} KEY_CACHED_INFORMATION, *PKEY_CACHED_INFORMATION;

typedef struct _KEY_VALUE_ENTRY
{
    PUNICODE_STRING ValueName;
    ULONG           DataLength;
    ULONG           DataOffset;
    ULONG           Type;
} KEY_VALUE_ENTRY, *PKEY_VALUE_ENTRY;

typedef struct _KEY_VALUE_BASIC_INFORMATION {
    ULONG TitleIndex;
    ULONG Type;
    ULONG NameLength;
    WCHAR Name[1];
} KEY_VALUE_BASIC_INFORMATION, *PKEY_VALUE_BASIC_INFORMATION;

typedef struct _KEY_VALUE_FULL_INFORMATION {
    ULONG TitleIndex;
    ULONG Type;
    ULONG DataOffset;
    ULONG DataLength;
    ULONG NameLength;
    WCHAR Name[1];
} KEY_VALUE_FULL_INFORMATION, *PKEY_VALUE_FULL_INFORMATION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION {
    ULONG TitleIndex;
    ULONG Type;
    ULONG DataLength;
    UCHAR Data[1];
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION_ALIGN64 {
    ULONG Type;
    ULONG DataLength;
    UCHAR Data[1];
} KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, *PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64;

typedef enum _MEMORY_RESERVE_OBJECT_TYPE
{
    MemoryReserveObjectTypeUserApc,
    MemoryReserveObjectTypeIoCompletion
} MEMORY_RESERVE_OBJECT_TYPE, PMEMORY_RESERVE_OBJECT_TYPE;

#ifndef __OBJECT_ATTRIBUTES_DEFINED__
#define __OBJECT_ATTRIBUTES_DEFINED__
typedef struct _OBJECT_ATTRIBUTES {
  ULONG Length;
  HANDLE RootDirectory;
  PUNICODE_STRING ObjectName;
  ULONG Attributes;
  PVOID SecurityDescriptor;       /* type SECURITY_DESCRIPTOR */
  PVOID SecurityQualityOfService; /* type SECURITY_QUALITY_OF_SERVICE */
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;
#endif

typedef struct _OBJECT_HANDLE_FLAG_INFORMATION {
    BOOLEAN Inherit;
    BOOLEAN ProtectFromClose;
} OBJECT_HANDLE_FLAG_INFORMATION, *POBJECT_HANDLE_FLAG_INFORMATION;

typedef struct _OBJECT_BASIC_INFORMATION {
    ULONG  Attributes;
    ACCESS_MASK  GrantedAccess;
    ULONG  HandleCount;
    ULONG  PointerCount;
    ULONG  PagedPoolUsage;
    ULONG  NonPagedPoolUsage;
    ULONG  Reserved[3];
    ULONG  NameInformationLength;
    ULONG  TypeInformationLength;
    ULONG  SecurityDescriptorLength;
    LARGE_INTEGER  CreateTime;
} OBJECT_BASIC_INFORMATION, *POBJECT_BASIC_INFORMATION;

typedef struct _OBJECT_NAME_INFORMATION {
    UNICODE_STRING Name;
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

typedef struct __OBJECT_TYPE_INFORMATION {
    UNICODE_STRING TypeName;
    ULONG TotalNumberOfObjects;
    ULONG TotalNumberOfHandles;
    ULONG TotalPagedPoolUsage;
    ULONG TotalNonPagedPoolUsage;
    ULONG TotalNamePoolUsage;
    ULONG TotalHandleTableUsage;
    ULONG HighWaterNumberOfObjects;
    ULONG HighWaterNumberOfHandles;
    ULONG HighWaterPagedPoolUsage;
    ULONG HighWaterNonPagedPoolUsage;
    ULONG HighWaterNamePoolUsage;
    ULONG HighWaterHandleTableUsage;
    ULONG InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ULONG ValidAccessMask;
    BOOLEAN SecurityRequired;
    BOOLEAN MaintainHandleCount;
    UCHAR TypeIndex;
    CHAR ReservedByte;
    ULONG PoolType;
    ULONG DefaultPagedPoolCharge;
    ULONG DefaultNonPagedPoolCharge;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

typedef struct _OBJECT_TYPES_INFORMATION
{
    ULONG NumberOfTypes;
} OBJECT_TYPES_INFORMATION, *POBJECT_TYPES_INFORMATION;

typedef struct _PROCESS_BASIC_INFORMATION {
#ifdef __WINESRC__
    NTSTATUS  ExitStatus;
    PEB      *PebBaseAddress;
    ULONG_PTR AffinityMask;
    LONG      BasePriority;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR InheritedFromUniqueProcessId;
#else
    PVOID Reserved1;
    PPEB PebBaseAddress;
    PVOID Reserved2[2];
    ULONG_PTR UniqueProcessId;
    PVOID Reserved3;
#endif
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

typedef struct _PROCESS_BASIC_INFORMATION64 {
    NTSTATUS  ExitStatus;
    UINT64    PebBaseAddress;
    UINT64    AffinityMask;
    LONG      BasePriority;
    UINT64    UniqueProcessId;
    UINT64    InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION64;

#define PROCESS_PRIOCLASS_IDLE          1
#define PROCESS_PRIOCLASS_NORMAL        2
#define PROCESS_PRIOCLASS_HIGH          3
#define PROCESS_PRIOCLASS_REALTIME      4
#define PROCESS_PRIOCLASS_BELOW_NORMAL  5
#define PROCESS_PRIOCLASS_ABOVE_NORMAL  6

typedef struct _PROCESS_PRIORITY_CLASS {
    BOOLEAN     Foreground;
    UCHAR       PriorityClass;
} PROCESS_PRIORITY_CLASS, *PPROCESS_PRIORITY_CLASS;

typedef struct _PROCESS_CYCLE_TIME_INFORMATION {
    ULONGLONG   AccumulatedCycles;
    ULONGLONG   CurrentCycleCount;
} PROCESS_CYCLE_TIME_INFORMATION, *PPROCESS_CYCLE_TIME_INFORMATION;

typedef struct _PROCESS_STACK_ALLOCATION_INFORMATION
{
    SIZE_T ReserveSize;
    SIZE_T ZeroBits;
    PVOID  StackBase;
} PROCESS_STACK_ALLOCATION_INFORMATION, *PPROCESS_STACK_ALLOCATION_INFORMATION;

typedef struct _PROCESS_STACK_ALLOCATION_INFORMATION_EX
{
    ULONG PreferredNode;
    ULONG Reserved0;
    ULONG Reserved1;
    ULONG Reserved2;
    PROCESS_STACK_ALLOCATION_INFORMATION AllocInfo;
} PROCESS_STACK_ALLOCATION_INFORMATION_EX, *PPROCESS_STACK_ALLOCATION_INFORMATION_EX;

typedef NTSTATUS (NTAPI RTL_HEAP_COMMIT_ROUTINE)(PVOID base, PVOID *address, PSIZE_T size);
typedef RTL_HEAP_COMMIT_ROUTINE *PRTL_HEAP_COMMIT_ROUTINE;

typedef struct _RTL_HEAP_PARAMETERS
{
    ULONG Length;
    SIZE_T SegmentReserve;
    SIZE_T SegmentCommit;
    SIZE_T DeCommitFreeblockThreshold;
    SIZE_T DeCommitTotalFreeThreshold;
    SIZE_T MaximumAllocationSize;
    SIZE_T VirtualMemoryThreshold;
    SIZE_T InitialCommit;
    SIZE_T InitialReserve;
    PRTL_HEAP_COMMIT_ROUTINE CommitRoutine;
    SIZE_T Reserved[2];
} RTL_HEAP_PARAMETERS, *PRTL_HEAP_PARAMETERS;

typedef struct _RTL_RWLOCK {
    RTL_CRITICAL_SECTION rtlCS;

    HANDLE hSharedReleaseSemaphore;
    UINT   uSharedWaiters;

    HANDLE hExclusiveReleaseSemaphore;
    UINT   uExclusiveWaiters;

    INT    iNumberActive;
    HANDLE hOwningThreadId;
    DWORD  dwTimeoutBoost;
    PVOID  pDebugInfo;
} RTL_RWLOCK, *LPRTL_RWLOCK;

/* System Information Class 0x00 */

typedef struct _SYSTEM_BASIC_INFORMATION {
#ifdef __WINESRC__
    DWORD     unknown;
    ULONG     KeMaximumIncrement;
    ULONG     PageSize;
    ULONG     MmNumberOfPhysicalPages;
    ULONG     MmLowestPhysicalPage;
    ULONG     MmHighestPhysicalPage;
    ULONG_PTR AllocationGranularity;
    PVOID     LowestUserAddress;
    PVOID     HighestUserAddress;
    ULONG_PTR ActiveProcessorsAffinityMask;
    BYTE      NumberOfProcessors;
#else
    BYTE Reserved1[24];
    PVOID Reserved2[4];
    CCHAR NumberOfProcessors;
#endif
} SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;

/* System Information Class 0x01 */

typedef struct _SYSTEM_CPU_INFORMATION {
    USHORT ProcessorArchitecture;
    USHORT ProcessorLevel;
    USHORT ProcessorRevision;
    USHORT MaximumProcessors;
    ULONG  ProcessorFeatureBits;
} SYSTEM_CPU_INFORMATION, *PSYSTEM_CPU_INFORMATION;

/* definitions of bits in the Feature set for the x86 processors */
#define CPU_FEATURE_VME    0x00000005   /* Virtual 86 Mode Extensions */
#define CPU_FEATURE_TSC    0x00000002   /* Time Stamp Counter available */
#define CPU_FEATURE_CMOV   0x00000008   /* Conditional Move instruction*/
#define CPU_FEATURE_PGE    0x00000014   /* Page table Entry Global bit */ 
#define CPU_FEATURE_PSE    0x00000024   /* Page Size Extension */
#define CPU_FEATURE_MTRR   0x00000040   /* Memory Type Range Registers */
#define CPU_FEATURE_CX8    0x00000080   /* Compare and eXchange 8 byte instr. */
#define CPU_FEATURE_MMX    0x00000100   /* Multi Media eXtensions */
#define CPU_FEATURE_X86    0x00000200   /* seems to be always ON, on the '86 */
#define CPU_FEATURE_PAT    0x00000400   /* Page Attribute Table */
#define CPU_FEATURE_FXSR   0x00000800   /* FXSAVE and FXSTORE instructions */
#define CPU_FEATURE_SEP    0x00001000   /* SYSENTER and SYSEXIT instructions */
#define CPU_FEATURE_SSE    0x00002000   /* SSE extensions (ext. MMX) */
#define CPU_FEATURE_3DNOW  0x00004000   /* 3DNOW instructions available */
#define CPU_FEATURE_SSE2   0x00010000   /* SSE2 extensions (XMMI64) */
#define CPU_FEATURE_DS     0x00020000   /* Debug Store */
#define CPU_FEATURE_HTT    0x00040000   /* Hyper Threading Technology */
#define CPU_FEATURE_SSE3   0x00080000   /* SSE3 extensions */
#define CPU_FEATURE_CX128  0x00100000   /* cmpxchg16b instruction */
#define CPU_FEATURE_XSAVE  0x00800000   /* XSAVE instructions */
#define CPU_FEATURE_2NDLEV 0x04000000   /* Second-level address translation */
#define CPU_FEATURE_VIRT   0x08000000   /* Virtualization support */
#define CPU_FEATURE_RDFS   0x10000000   /* RDFSBASE etc. instructions */
#define CPU_FEATURE_NX     0x20000000   /* Data execution prevention */

/* FIXME: following values are made up, actual flags are unknown */
#define CPU_FEATURE_SSSE3         0x00008000   /* SSSE3 instructions */
#define CPU_FEATURE_SSE41         0x01000000   /* SSE41 instructions */
#define CPU_FEATURE_SSE42         0x02000000   /* SSE42 instructions */
#define CPU_FEATURE_AVX           0x40000000   /* AVX instructions */
#define CPU_FEATURE_AVX2          0x80000000   /* AVX2 instructions */
#define CPU_FEATURE_PAE           0x00200000
#define CPU_FEATURE_DAZ           0x00400000

#define CPU_FEATURE_ARM_VFP_32     0x00000001
#define CPU_FEATURE_ARM_NEON       0x00000002
#define CPU_FEATURE_ARM_V8_CRC32   0x00000004
#define CPU_FEATURE_ARM_V8_CRYPTO  0x00000008
#define CPU_FEATURE_ARM_V81_ATOMIC 0x00000010
#define CPU_FEATURE_ARM_V82_DP     0x00000020
#define CPU_FEATURE_ARM_V83_JSCVT  0x00000040
#define CPU_FEATURE_ARM_V83_LRCPC  0x00000080
#define CPU_FEATURE_ARM_SVE        0x00000100
#define CPU_FEATURE_ARM_SVE2       0x00000200
#define CPU_FEATURE_ARM_SVE2_1     0x00000400
#define CPU_FEATURE_ARM_SVE_AES    0x00000800
#define CPU_FEATURE_ARM_SVE_PMULL128 0x00001000
#define CPU_FEATURE_ARM_SVE_BITPERM  0x00002000
#define CPU_FEATURE_ARM_SVE_BF16     0x00004000
#define CPU_FEATURE_ARM_SVE_EBF16    0x00008000
#define CPU_FEATURE_ARM_SVE_B16B16   0x00010000
#define CPU_FEATURE_ARM_SVE_SHA3     0x00020000
#define CPU_FEATURE_ARM_SVE_SM4      0x00040000
#define CPU_FEATURE_ARM_SVE_I8MM     0x00080000
#define CPU_FEATURE_ARM_SVE_F32MM    0x00100000
#define CPU_FEATURE_ARM_SVE_F64MM    0x00200000

typedef struct _SYSTEM_PROCESSOR_FEATURES_INFORMATION
{
    ULONGLONG ProcessorFeatureBits;
    ULONGLONG Reserved[3];
} SYSTEM_PROCESSOR_FEATURES_INFORMATION, *PSYSTEM_PROCESSOR_FEATURES_INFORMATION;

/* System Information Class 0x02 */

/* Documented in "Windows NT/2000 Native API Reference" by Gary Nebbett. */
typedef struct _SYSTEM_PERFORMANCE_INFORMATION {
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;
    ULONG ReadOperationCount;
    ULONG WriteOperationCount;
    ULONG OtherOperationCount;
    ULONG AvailablePages;
    ULONG TotalCommittedPages;
    ULONG TotalCommitLimit;
    ULONG PeakCommitment;
    ULONG PageFaults;
    ULONG WriteCopyFaults;
    ULONG TransitionFaults;
    ULONG Reserved1;
    ULONG DemandZeroFaults;
    ULONG PagesRead;
    ULONG PageReadIos;
    ULONG Reserved2[2];
    ULONG PagefilePagesWritten;
    ULONG PagefilePageWriteIos;
    ULONG MappedFilePagesWritten;
    ULONG MappedFilePageWriteIos;
    ULONG PagedPoolUsage;
    ULONG NonPagedPoolUsage;
    ULONG PagedPoolAllocs;
    ULONG PagedPoolFrees;
    ULONG NonPagedPoolAllocs;
    ULONG NonPagedPoolFrees;
    ULONG TotalFreeSystemPtes;
    ULONG SystemCodePage;
    ULONG TotalSystemDriverPages;
    ULONG TotalSystemCodePages;
    ULONG SmallNonPagedLookasideListAllocateHits;
    ULONG SmallPagedLookasideListAllocateHits;
    ULONG Reserved3;
    ULONG MmSystemCachePage;
    ULONG PagedPoolPage;
    ULONG SystemDriverPage;
    ULONG FastReadNoWait;
    ULONG FastReadWait;
    ULONG FastReadResourceMiss;
    ULONG FastReadNotPossible;
    ULONG FastMdlReadNoWait;
    ULONG FastMdlReadWait;
    ULONG FastMdlReadResourceMiss;
    ULONG FastMdlReadNotPossible;
    ULONG MapDataNoWait;
    ULONG MapDataWait;
    ULONG MapDataNoWaitMiss;
    ULONG MapDataWaitMiss;
    ULONG PinMappedDataCount;
    ULONG PinReadNoWait;
    ULONG PinReadWait;
    ULONG PinReadNoWaitMiss;
    ULONG PinReadWaitMiss;
    ULONG CopyReadNoWait;
    ULONG CopyReadWait;
    ULONG CopyReadNoWaitMiss;
    ULONG CopyReadWaitMiss;
    ULONG MdlReadNoWait;
    ULONG MdlReadWait;
    ULONG MdlReadNoWaitMiss;
    ULONG MdlReadWaitMiss;
    ULONG ReadAheadIos;
    ULONG LazyWriteIos;
    ULONG LazyWritePages;
    ULONG DataFlushes;
    ULONG DataPages;
    ULONG ContextSwitches;
    ULONG FirstLevelTbFills;
    ULONG SecondLevelTbFills;
    ULONG SystemCalls;
} SYSTEM_PERFORMANCE_INFORMATION, *PSYSTEM_PERFORMANCE_INFORMATION;

/* System Information Class 0x03 */

typedef struct _SYSTEM_TIMEOFDAY_INFORMATION {
#ifdef __WINESRC__
    LARGE_INTEGER BootTime;
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER TimeZoneBias;
    ULONG TimeZoneId;
    ULONG Reserved;
    ULONGLONG BootTimeBias;
    ULONGLONG SleepTimeBias;
#else
    BYTE Reserved1[48];
#endif
} SYSTEM_TIMEOFDAY_INFORMATION, *PSYSTEM_TIMEOFDAY_INFORMATION; /* was SYSTEM_TIME_INFORMATION */

/* System Information Class 0x08 */

typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION {
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER Reserved1[2];
    ULONG Reserved2;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION, *PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

/* System Information Class 0x0b */

typedef struct _SYSTEM_DRIVER_INFORMATION {
    PVOID pvAddress;
    DWORD dwUnknown1;
    DWORD dwUnknown2;
    DWORD dwEntryIndex;
    DWORD dwUnknown3;
    char szName[MAX_PATH + 1];
} SYSTEM_DRIVER_INFORMATION, *PSYSTEM_DRIVER_INFORMATION;

/* System Information Class 0x10 */

typedef struct _SYSTEM_HANDLE_ENTRY {
    ULONG  OwnerPid;
    BYTE   ObjectType;
    BYTE   HandleFlags;
    USHORT HandleValue;
    PVOID  ObjectPointer;
    ULONG  AccessMask;
} SYSTEM_HANDLE_ENTRY, *PSYSTEM_HANDLE_ENTRY;

typedef struct _SYSTEM_HANDLE_INFORMATION {
    ULONG               Count;
    SYSTEM_HANDLE_ENTRY Handle[1];
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX
{
    void *Object;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR HandleValue;
    ULONG GrantedAccess;
    USHORT CreatorBackTraceIndex;
    USHORT ObjectTypeIndex;
    ULONG HandleAttributes;
    ULONG Reserved;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX;

typedef struct _SYSTEM_HANDLE_INFORMATION_EX
{
    ULONG_PTR NumberOfHandles;
    ULONG_PTR Reserved;
    SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Handles[1];
} SYSTEM_HANDLE_INFORMATION_EX;

/* System Information Class 0x15 */

typedef struct _SYSTEM_CACHE_INFORMATION {
    SIZE_T CurrentSize;
    SIZE_T PeakSize;
    ULONG PageFaultCount;
    SIZE_T MinimumWorkingSet;
    SIZE_T MaximumWorkingSet;
    SIZE_T CurrentSizeIncludingTransitionInPages;
    SIZE_T PeakSizeIncludingTransitionInPages;
    ULONG TransitionRePurposeCount;
    ULONG Flags;
} SYSTEM_CACHE_INFORMATION, *PSYSTEM_CACHE_INFORMATION;

/* System Information Class 0x17 */

typedef struct _SYSTEM_INTERRUPT_INFORMATION {
    ULONG ContextSwitches;
    ULONG DpcCount;
    ULONG DpcRate;
    ULONG TimeIncrement;
    ULONG DpcBypassCount;
    ULONG ApcBypassCount;
} SYSTEM_INTERRUPT_INFORMATION, *PSYSTEM_INTERRUPT_INFORMATION;

typedef struct _SYSTEM_CONFIGURATION_INFO {
    union {
        ULONG	OemId;
        struct {
	    WORD ProcessorArchitecture;
	    WORD Reserved;
	} tag1;
    } tag2;
    ULONG PageSize;
    PVOID MinimumApplicationAddress;
    PVOID MaximumApplicationAddress;
    ULONG ActiveProcessorMask;
    ULONG NumberOfProcessors;
    ULONG ProcessorType;
    ULONG AllocationGranularity;
    WORD  ProcessorLevel;
    WORD  ProcessorRevision;
} SYSTEM_CONFIGURATION_INFO, *PSYSTEM_CONFIGURATION_INFO;

typedef struct _SYSTEM_EXCEPTION_INFORMATION {
    BYTE Reserved1[16];
} SYSTEM_EXCEPTION_INFORMATION, *PSYSTEM_EXCEPTION_INFORMATION;

typedef struct _SYSTEM_LOOKASIDE_INFORMATION {
    BYTE Reserved1[32];
} SYSTEM_LOOKASIDE_INFORMATION, *PSYSTEM_LOOKASIDE_INFORMATION;

typedef struct _SYSTEM_KERNEL_DEBUGGER_INFORMATION {
	BOOLEAN  DebuggerEnabled;
	BOOLEAN  DebuggerNotPresent;
} SYSTEM_KERNEL_DEBUGGER_INFORMATION, *PSYSTEM_KERNEL_DEBUGGER_INFORMATION;

typedef struct _SYSTEM_KERNEL_DEBUGGER_INFORMATION_EX {
    BOOLEAN  DebuggerAllowed;
    BOOLEAN  DebuggerEnabled;
    BOOLEAN  DebuggerPresent;
} SYSTEM_KERNEL_DEBUGGER_INFORMATION_EX, *PSYSTEM_KERNEL_DEBUGGER_INFORMATION_EX;

typedef struct _VM_COUNTERS
{
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

typedef struct _VM_COUNTERS_EX
{
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

typedef struct _SYSTEM_PROCESS_INFORMATION {
#ifdef __WINESRC__                  /* win32/win64 */
    ULONG NextEntryOffset;             /* 00/00 */
    DWORD dwThreadCount;               /* 04/04 */
    LARGE_INTEGER WorkingSetPrivateSize; /* 08/08 */
    ULONG HardFaultCount;              /* 10/10 */
    ULONG NumberOfThreadsHighWatermark;/* 14/14 */
    ULONGLONG CycleTime;               /* 18/18 */
    LARGE_INTEGER CreationTime;        /* 20/20 */
    LARGE_INTEGER UserTime;            /* 28/28 */
    LARGE_INTEGER KernelTime;          /* 30/30 */
    UNICODE_STRING ProcessName;        /* 38/38 */
    DWORD dwBasePriority;              /* 40/48 */
    HANDLE UniqueProcessId;            /* 44/50 */
    HANDLE ParentProcessId;            /* 48/58 */
    ULONG HandleCount;                 /* 4c/60 */
    ULONG SessionId;                   /* 50/64 */
    ULONG_PTR UniqueProcessKey;        /* 54/68 */
    VM_COUNTERS_EX vmCounters;         /* 58/70 */
    IO_COUNTERS ioCounters;            /* 88/d0 */
    SYSTEM_THREAD_INFORMATION ti[1];   /* b8/100 */
#else
    ULONG NextEntryOffset;             /* 00/00 */
    BYTE Reserved1[52];                /* 04/04 */
    PVOID Reserved2[3];                /* 38/38 */
    HANDLE UniqueProcessId;            /* 44/50 */
    PVOID Reserved3;                   /* 48/58 */
    ULONG HandleCount;                 /* 4c/60 */
    BYTE Reserved4[4];                 /* 50/64 */
    PVOID Reserved5[11];               /* 54/68 */
    SIZE_T PeakPagefileUsage;          /* 80/c0 */
    SIZE_T PrivatePageCount;           /* 84/c8 */
    LARGE_INTEGER Reserved6[6];        /* 88/d0 */
#endif
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

typedef struct _SYSTEM_REGISTRY_QUOTA_INFORMATION {
    ULONG RegistryQuotaAllowed;
    ULONG RegistryQuotaUsed;
    PVOID Reserved1;
} SYSTEM_REGISTRY_QUOTA_INFORMATION, *PSYSTEM_REGISTRY_QUOTA_INFORMATION;

typedef struct _SYSTEM_TIME_ADJUSTMENT_QUERY {
    ULONG   TimeAdjustment;
    ULONG   TimeIncrement;
    BOOLEAN TimeAdjustmentDisabled;
} SYSTEM_TIME_ADJUSTMENT_QUERY, *PSYSTEM_TIME_ADJUSTMENT_QUERY;

typedef struct _SYSTEM_TIME_ADJUSTMENT {
    ULONG   TimeAdjustment;
    BOOLEAN TimeAdjustmentDisabled;
} SYSTEM_TIME_ADJUSTMENT, *PSYSTEM_TIME_ADJUSTMENT;

typedef enum _SYSTEM_FIRMWARE_TABLE_ACTION
{
    SystemFirmwareTable_Enumerate = 0,
    SystemFirmwareTable_Get = 1
} SYSTEM_FIRMWARE_TABLE_ACTION, *PSYSTEM_FIRMWARE_TABLE_ACTION;

/* System Information Class 0x4C */

typedef struct _SYSTEM_FIRMWARE_TABLE_INFORMATION
{
    ULONG ProviderSignature;
    SYSTEM_FIRMWARE_TABLE_ACTION Action;
    ULONG TableID;
    ULONG TableBufferLength;
    UCHAR TableBuffer[1];
} SYSTEM_FIRMWARE_TABLE_INFORMATION, *PSYSTEM_FIRMWARE_TABLE_INFORMATION;

typedef struct _SYSTEM_PROCESS_ID_INFORMATION
{
    ULONG_PTR ProcessId;
    UNICODE_STRING ImageName;
} SYSTEM_PROCESS_ID_INFORMATION, *PSYSTEM_PROCESS_ID_INFORMATION;

#ifndef __REACTOS__
typedef struct _TIME_FIELDS
{   CSHORT Year;
    CSHORT Month;
    CSHORT Day;
    CSHORT Hour;
    CSHORT Minute;
    CSHORT Second;
    CSHORT Milliseconds;
    CSHORT Weekday;
} TIME_FIELDS, *PTIME_FIELDS;
#endif

typedef struct _WINSTATIONINFORMATIONW {
  BYTE Reserved2[70];
  ULONG LogonId;
  BYTE Reserved3[1140];
} WINSTATIONINFORMATIONW, *PWINSTATIONINFORMATIONW;

typedef BOOLEAN (WINAPI * PWINSTATIONQUERYINFORMATIONW)(HANDLE,ULONG,WINSTATIONINFOCLASS,PVOID,ULONG,PULONG);

typedef struct _LDR_RESOURCE_INFO
{
    ULONG_PTR Type;
    ULONG_PTR Name;
    ULONG Language;
} LDR_RESOURCE_INFO, *PLDR_RESOURCE_INFO;


/* debug buffer definitions */

typedef struct _DEBUG_BUFFER {
  HANDLE SectionHandle;
  PVOID  SectionBase;
  PVOID  RemoteSectionBase;
  ULONG  SectionBaseDelta;
  HANDLE EventPairHandle;
  SIZE_T Unknown[2];
  HANDLE RemoteThreadHandle;
  ULONG  InfoClassMask;
  SIZE_T SizeOfInfo;
  SIZE_T AllocatedSize;
  ULONG  SectionSize;
  PVOID  ModuleInformation;
  PVOID  BackTraceInformation;
  PVOID  HeapInformation;
  PVOID  LockInformation;
  PVOID  Reserved[8];
} DEBUG_BUFFER, *PDEBUG_BUFFER;

#define PDI_MODULES                       0x01
#define PDI_BACKTRACE                     0x02
#define PDI_HEAPS                         0x04
#define PDI_HEAP_TAGS                     0x08
#define PDI_HEAP_BLOCKS                   0x10
#define PDI_LOCKS                         0x20

typedef struct _DEBUG_MODULE_INFORMATION {
  ULONG  Reserved[2];
  ULONG  Base;
  ULONG  Size;
  ULONG  Flags;
  USHORT Index;
  USHORT Unknown;
  USHORT LoadCount;
  USHORT ModuleNameOffset;
  CHAR   ImageName[256];
} DEBUG_MODULE_INFORMATION, *PDEBUG_MODULE_INFORMATION;

typedef struct _DEBUG_HEAP_INFORMATION {
  ULONG  Base;
  ULONG  Flags;
  USHORT Granularity;
  USHORT Unknown;
  ULONG  Allocated;
  ULONG  Committed;
  ULONG  TagCount;
  ULONG  BlockCount;
  ULONG  Reserved[7];
  PVOID  Tags;
  PVOID  Blocks;
} DEBUG_HEAP_INFORMATION, *PDEBUG_HEAP_INFORMATION;

typedef struct _DEBUG_LOCK_INFORMATION {
  PVOID  Address;
  USHORT Type;
  USHORT CreatorBackTraceIndex;
  ULONG  OwnerThreadId;
  ULONG  ActiveCount;
  ULONG  ContentionCount;
  ULONG  EntryCount;
  ULONG  RecursionCount;
  ULONG  NumberOfSharedWaiters;
  ULONG  NumberOfExclusiveWaiters;
} DEBUG_LOCK_INFORMATION, *PDEBUG_LOCK_INFORMATION;

typedef struct _PORT_MESSAGE_HEADER {
  USHORT DataSize;
  USHORT MessageSize;
  USHORT MessageType;
  USHORT VirtualRangesOffset;
  CLIENT_ID ClientId;
  ULONG MessageId;
  ULONG SectionSize;
} PORT_MESSAGE_HEADER, *PPORT_MESSAGE_HEADER, PORT_MESSAGE, *PPORT_MESSAGE;

typedef unsigned short RTL_ATOM, *PRTL_ATOM;

typedef enum _ATOM_INFORMATION_CLASS {
   AtomBasicInformation         = 0,
   AtomTableInformation         = 1,
} ATOM_INFORMATION_CLASS;

typedef struct _ATOM_BASIC_INFORMATION {
   USHORT       ReferenceCount;
   USHORT       Pinned;
   USHORT       NameLength;
   WCHAR        Name[1];
} ATOM_BASIC_INFORMATION, *PATOM_BASIC_INFORMATION;

/* FIXME: names probably not correct */
typedef struct _RTL_HANDLE
{
    struct _RTL_HANDLE * Next;
} RTL_HANDLE;

/* FIXME: names probably not correct */
typedef struct _RTL_HANDLE_TABLE
{
    ULONG MaxHandleCount;  /* 0x00 */
    ULONG HandleSize;      /* 0x04 */
    ULONG Unused[2];       /* 0x08-0x0c */
    PVOID NextFree;        /* 0x10 */
    PVOID FirstHandle;     /* 0x14 */
    PVOID ReservedMemory;  /* 0x18 */
    PVOID MaxHandle;       /* 0x1c */
} RTL_HANDLE_TABLE;

typedef struct _RTL_ATOM_TABLE_ENTRY
{
    struct _RTL_ATOM_TABLE_ENTRY *HashLink;
    WORD                          HandleIndex;
    WORD                          Atom;
    WORD                          ReferenceCount;
    UCHAR                         Flags;
    UCHAR                         NameLength;
    WCHAR                         Name[1];
} RTL_ATOM_TABLE_ENTRY, *PRTL_ATOM_TABLE_ENTRY;

typedef struct _RTL_ATOM_TABLE
{
    ULONG                 Signature;
    RTL_CRITICAL_SECTION  CriticalSection;
    RTL_HANDLE_TABLE      HandleTable;
    ULONG                 NumberOfBuckets;
    RTL_ATOM_TABLE_ENTRY *Buckets[1];
} *RTL_ATOM_TABLE, **PRTL_ATOM_TABLE;

/***********************************************************************
 * Defines
 */

/* flags for NtCreateFile and NtOpenFile */
#define FILE_DIRECTORY_FILE             0x00000001
#define FILE_WRITE_THROUGH              0x00000002
#define FILE_SEQUENTIAL_ONLY            0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING  0x00000008
#define FILE_SYNCHRONOUS_IO_ALERT       0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT    0x00000020
#define FILE_NON_DIRECTORY_FILE         0x00000040
#define FILE_CREATE_TREE_CONNECTION     0x00000080
#define FILE_COMPLETE_IF_OPLOCKED       0x00000100
#define FILE_NO_EA_KNOWLEDGE            0x00000200
#define FILE_OPEN_FOR_RECOVERY          0x00000400
#define FILE_RANDOM_ACCESS              0x00000800
#define FILE_DELETE_ON_CLOSE            0x00001000
#define FILE_OPEN_BY_FILE_ID            0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT     0x00004000
#define FILE_NO_COMPRESSION             0x00008000
#define FILE_RESERVE_OPFILTER           0x00100000
#define FILE_OPEN_REPARSE_POINT         0x00200000
#define FILE_OPEN_OFFLINE_FILE          0x00400000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY  0x00800000

#define FILE_ATTRIBUTE_VALID_FLAGS      0x00007fb7
#define FILE_ATTRIBUTE_VALID_SET_FLAGS  0x000031a7

/* status for NtCreateFile or NtOpenFile */
#define FILE_SUPERSEDED                 0
#define FILE_OPENED                     1
#define FILE_CREATED                    2
#define FILE_OVERWRITTEN                3
#define FILE_EXISTS                     4
#define FILE_DOES_NOT_EXIST             5

/* disposition for NtCreateFile */
#define FILE_SUPERSEDE                  0
#define FILE_OPEN                       1
#define FILE_CREATE                     2
#define FILE_OPEN_IF                    3
#define FILE_OVERWRITE                  4
#define FILE_OVERWRITE_IF               5
#define FILE_MAXIMUM_DISPOSITION        5

/* Characteristics of a File System */
#define FILE_REMOVABLE_MEDIA                      0x00000001
#define FILE_READ_ONLY_DEVICE                     0x00000002
#define FILE_FLOPPY_DISKETTE                      0x00000004
#define FILE_WRITE_ONE_MEDIA                      0x00000008
#define FILE_REMOTE_DEVICE                        0x00000010
#define FILE_DEVICE_IS_MOUNTED                    0x00000020
#define FILE_VIRTUAL_VOLUME                       0x00000040
#define FILE_AUTOGENERATED_DEVICE_NAME            0x00000080
#define FILE_DEVICE_SECURE_OPEN                   0x00000100
#define FILE_CHARACTERISTIC_PNP_DEVICE            0x00000800
#define FILE_CHARACTERISTIC_TS_DEVICE             0x00001000
#define FILE_CHARACTERISTIC_WEBDAV_DEVICE         0x00002000
#define FILE_CHARACTERISTIC_CSV                   0x00010000
#define FILE_DEVICE_ALLOW_APPCONTAINER_TRAVERSAL  0x00020000
#define FILE_PORTABLE_DEVICE                      0x00040000

/* options for NtCreateNamedPipeFile */
#define FILE_PIPE_INBOUND               0x00000000
#define FILE_PIPE_OUTBOUND              0x00000001
#define FILE_PIPE_FULL_DUPLEX           0x00000002

/* options for pipe's type */
#define FILE_PIPE_TYPE_MESSAGE          0x00000001
#define FILE_PIPE_TYPE_BYTE             0x00000000
/* options for pipe's message mode */
#define FILE_PIPE_MESSAGE_MODE          0x00000001
#define FILE_PIPE_BYTE_STREAM_MODE      0x00000000
/* options for pipe's blocking mode */
#define FILE_PIPE_COMPLETE_OPERATION    0x00000001
#define FILE_PIPE_QUEUE_OPERATION       0x00000000
/* and client / server end */
#define FILE_PIPE_SERVER_END            0x00000001
#define FILE_PIPE_CLIENT_END            0x00000000

#define INTERNAL_TS_ACTIVE_CONSOLE_ID ( *((volatile ULONG*)(0x7ffe02d8)) )

#define LOGONID_CURRENT    ((ULONG)-1)

#define OBJ_PROTECT_CLOSE    0x00000001
#define OBJ_INHERIT          0x00000002
#define OBJ_PERMANENT        0x00000010
#define OBJ_EXCLUSIVE        0x00000020
#define OBJ_CASE_INSENSITIVE 0x00000040
#define OBJ_OPENIF           0x00000080
#define OBJ_OPENLINK         0x00000100
#define OBJ_KERNEL_HANDLE    0x00000200
#define OBJ_VALID_ATTRIBUTES 0x000003F2

#define SERVERNAME_CURRENT ((HANDLE)NULL)

typedef void (CALLBACK *PNTAPCFUNC)(ULONG_PTR,ULONG_PTR,ULONG_PTR); /* FIXME: not the right name */
typedef void (CALLBACK *PRTL_THREAD_START_ROUTINE)(LPVOID); /* FIXME: not the right name */
typedef DWORD (CALLBACK *PRTL_WORK_ITEM_ROUTINE)(LPVOID); /* FIXME: not the right name */
typedef void (NTAPI *RTL_WAITORTIMERCALLBACKFUNC)(PVOID,BOOLEAN); /* FIXME: not the right name */


/* DbgPrintEx default levels */
#define DPFLTR_ERROR_LEVEL     0
#define DPFLTR_WARNING_LEVEL   1
#define DPFLTR_TRACE_LEVEL     2
#define DPFLTR_INFO_LEVEL      3
#define DPFLTR_MASK    0x8000000

/* Well-known LUID values */
#define SE_MIN_WELL_KNOWN_PRIVILEGE       2
#define SE_CREATE_TOKEN_PRIVILEGE         2
#define SE_ASSIGNPRIMARYTOKEN_PRIVILEGE   3
#define SE_LOCK_MEMORY_PRIVILEGE          4
#define SE_INCREASE_QUOTA_PRIVILEGE       5
#define SE_UNSOLICITED_INPUT_PRIVILEGE    6 /* obsolete */
#define SE_MACHINE_ACCOUNT_PRIVILEGE      6
#define SE_TCB_PRIVILEGE                  7
#define SE_SECURITY_PRIVILEGE             8
#define SE_TAKE_OWNERSHIP_PRIVILEGE       9
#define SE_LOAD_DRIVER_PRIVILEGE         10
#define SE_SYSTEM_PROFILE_PRIVILEGE      11
#define SE_SYSTEMTIME_PRIVILEGE          12
#define SE_PROF_SINGLE_PROCESS_PRIVILEGE 13
#define SE_INC_BASE_PRIORITY_PRIVILEGE   14
#define SE_CREATE_PAGEFILE_PRIVILEGE     15
#define SE_CREATE_PERMANENT_PRIVILEGE    16
#define SE_BACKUP_PRIVILEGE              17
#define SE_RESTORE_PRIVILEGE             18
#define SE_SHUTDOWN_PRIVILEGE            19
#define SE_DEBUG_PRIVILEGE               20
#define SE_AUDIT_PRIVILEGE               21
#define SE_SYSTEM_ENVIRONMENT_PRIVILEGE  22
#define SE_CHANGE_NOTIFY_PRIVILEGE       23
#define SE_REMOTE_SHUTDOWN_PRIVILEGE     24
#define SE_UNDOCK_PRIVILEGE              25
#define SE_SYNC_AGENT_PRIVILEGE          26
#define SE_ENABLE_DELEGATION_PRIVILEGE   27
#define SE_MANAGE_VOLUME_PRIVILEGE       28
#define SE_IMPERSONATE_PRIVILEGE         29
#define SE_CREATE_GLOBAL_PRIVILEGE       30
#define SE_MAX_WELL_KNOWN_PRIVILEGE      SE_CREATE_GLOBAL_PRIVILEGE

/* NtGlobalFlag bits */
#define FLG_STOP_ON_EXCEPTION            0x00000001
#define FLG_SHOW_LDR_SNAPS               0x00000002
#define FLG_DEBUG_INITIAL_COMMAND        0x00000004
#define FLG_STOP_ON_HUNG_GUI             0x00000008
#define FLG_HEAP_ENABLE_TAIL_CHECK       0x00000010
#define FLG_HEAP_ENABLE_FREE_CHECK       0x00000020
#define FLG_HEAP_VALIDATE_PARAMETERS     0x00000040
#define FLG_HEAP_VALIDATE_ALL            0x00000080
#define FLG_APPLICATION_VERIFIER         0x00000100
#define FLG_POOL_ENABLE_TAGGING          0x00000400
#define FLG_HEAP_ENABLE_TAGGING          0x00000800
#define FLG_USER_STACK_TRACE_DB          0x00001000
#define FLG_KERNEL_STACK_TRACE_DB        0x00002000
#define FLG_MAINTAIN_OBJECT_TYPELIST     0x00004000
#define FLG_HEAP_ENABLE_TAG_BY_DLL       0x00008000
#define FLG_DISABLE_STACK_EXTENSION      0x00010000
#define FLG_ENABLE_CSRDEBUG              0x00020000
#define FLG_ENABLE_KDEBUG_SYMBOL_LOAD    0x00040000
#define FLG_DISABLE_PAGE_KERNEL_STACKS   0x00080000
#define FLG_ENABLE_SYSTEM_CRIT_BREAKS    0x00100000
#define FLG_HEAP_DISABLE_COALESCING      0x00200000
#define FLG_ENABLE_CLOSE_EXCEPTIONS      0x00400000
#define FLG_ENABLE_EXCEPTION_LOGGING     0x00800000
#define FLG_ENABLE_HANDLE_TYPE_TAGGING   0x01000000
#define FLG_HEAP_PAGE_ALLOCS             0x02000000
#define FLG_DEBUG_INITIAL_COMMAND_EX     0x04000000
#define FLG_DISABLE_DBGPRINT             0x08000000
#define FLG_CRITSEC_EVENT_CREATION       0x10000000
#define FLG_LDR_TOP_DOWN                 0x20000000
#define FLG_ENABLE_HANDLE_EXCEPTIONS     0x40000000
#define FLG_DISABLE_PROTDLLS             0x80000000

/* Rtl*Registry* functions structs and defines */
#define RTL_REGISTRY_ABSOLUTE             0
#define RTL_REGISTRY_SERVICES             1
#define RTL_REGISTRY_CONTROL              2
#define RTL_REGISTRY_WINDOWS_NT           3
#define RTL_REGISTRY_DEVICEMAP            4
#define RTL_REGISTRY_USER                 5

#define RTL_REGISTRY_HANDLE       0x40000000
#define RTL_REGISTRY_OPTIONAL     0x80000000

#define RTL_QUERY_REGISTRY_SUBKEY         0x00000001
#define RTL_QUERY_REGISTRY_TOPKEY         0x00000002
#define RTL_QUERY_REGISTRY_REQUIRED       0x00000004
#define RTL_QUERY_REGISTRY_NOVALUE        0x00000008
#define RTL_QUERY_REGISTRY_NOEXPAND       0x00000010
#define RTL_QUERY_REGISTRY_DIRECT         0x00000020
#define RTL_QUERY_REGISTRY_DELETE         0x00000040
#define RTL_QUERY_REGISTRY_TYPECHECK      0x00000100

#define RTL_QUERY_REGISTRY_TYPECHECK_SHIFT 24

typedef NTSTATUS (WINAPI *PRTL_QUERY_REGISTRY_ROUTINE)( PCWSTR ValueName,
                                                        ULONG  ValueType,
                                                        PVOID  ValueData,
                                                        ULONG  ValueLength,
                                                        PVOID  Context,
                                                        PVOID  EntryContext);

typedef struct _RTL_QUERY_REGISTRY_TABLE
{
  PRTL_QUERY_REGISTRY_ROUTINE  QueryRoutine;
  ULONG  Flags;
  PWSTR  Name;
  PVOID  EntryContext;
  ULONG  DefaultType;
  PVOID  DefaultData;
  ULONG  DefaultLength;
} RTL_QUERY_REGISTRY_TABLE, *PRTL_QUERY_REGISTRY_TABLE;

typedef struct _KEY_MULTIPLE_VALUE_INFORMATION
{
  PUNICODE_STRING ValueName;
  ULONG DataLength;
  ULONG DataOffset;
  ULONG Type;
} KEY_MULTIPLE_VALUE_INFORMATION, *PKEY_MULTIPLE_VALUE_INFORMATION;

typedef VOID (CALLBACK *PRTL_OVERLAPPED_COMPLETION_ROUTINE)(DWORD,DWORD,LPVOID);

typedef VOID (CALLBACK *PTIMER_APC_ROUTINE) ( PVOID, ULONG, LONG );

typedef enum _EVENT_INFORMATION_CLASS {
  EventBasicInformation
} EVENT_INFORMATION_CLASS, *PEVENT_INFORMATION_CLASS;

typedef struct _EVENT_BASIC_INFORMATION {
  EVENT_TYPE EventType;
  LONG EventState;
} EVENT_BASIC_INFORMATION, *PEVENT_BASIC_INFORMATION;

typedef enum _SEMAPHORE_INFORMATION_CLASS {
  SemaphoreBasicInformation
} SEMAPHORE_INFORMATION_CLASS, *PSEMAPHORE_INFORMATION_CLASS;

typedef struct _SEMAPHORE_BASIC_INFORMATION {
  ULONG CurrentCount;
  ULONG MaximumCount;
} SEMAPHORE_BASIC_INFORMATION, *PSEMAPHORE_BASIC_INFORMATION;

typedef enum _SECTION_INFORMATION_CLASS
{
  SectionBasicInformation,
  SectionImageInformation,
  SectionRelocationInformation,
  SectionOriginalBaseInformation,
  SectionInternalImageInformation
} SECTION_INFORMATION_CLASS;

typedef struct _SECTION_BASIC_INFORMATION {
  PVOID BaseAddress;
  ULONG Attributes;
  LARGE_INTEGER Size;
} SECTION_BASIC_INFORMATION, *PSECTION_BASIC_INFORMATION;

typedef struct _SECTION_IMAGE_INFORMATION {
  PVOID TransferAddress;
  ULONG ZeroBits;
  SIZE_T MaximumStackSize;
  SIZE_T CommittedStackSize;
  ULONG SubSystemType;
  USHORT MinorSubsystemVersion;
  USHORT MajorSubsystemVersion;
  USHORT MajorOperatingSystemVersion;
  USHORT MinorOperatingSystemVersion;
  USHORT ImageCharacteristics;
  USHORT DllCharacteristics;
  USHORT Machine;
  BOOLEAN ImageContainsCode;
  union
  {
      UCHAR ImageFlags;
      struct
      {
          UCHAR ComPlusNativeReady        : 1;
          UCHAR ComPlusILOnly             : 1;
          UCHAR ImageDynamicallyRelocated : 1;
          UCHAR ImageMappedFlat           : 1;
          UCHAR BaseBelow4gb              : 1;
          UCHAR ComPlusPrefer32bit        : 1;
          UCHAR Reserved                  : 2;
      } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME;
  ULONG LoaderFlags;
  ULONG ImageFileSize;
  ULONG CheckSum;
} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION;

typedef struct _LPC_SECTION_WRITE {
  ULONG Length;
  HANDLE SectionHandle;
  ULONG SectionOffset;
  ULONG ViewSize;
  PVOID ViewBase;
  PVOID TargetViewBase;
} LPC_SECTION_WRITE, *PLPC_SECTION_WRITE;

typedef struct _LPC_SECTION_READ {
  ULONG Length;
  ULONG ViewSize;
  PVOID ViewBase;
} LPC_SECTION_READ, *PLPC_SECTION_READ;

typedef struct _LPC_MESSAGE {
  USHORT DataSize;
  USHORT MessageSize;
  USHORT MessageType;
  USHORT VirtualRangesOffset;
  CLIENT_ID ClientId;
  ULONG_PTR MessageId;
  ULONG_PTR SectionSize;
  UCHAR Data[ANYSIZE_ARRAY];
} LPC_MESSAGE, *PLPC_MESSAGE;

typedef struct _RTL_USER_PROCESS_INFORMATION
{
  ULONG Length;
  HANDLE Process;
  HANDLE Thread;
  CLIENT_ID ClientId;
  SECTION_IMAGE_INFORMATION ImageInformation;
} RTL_USER_PROCESS_INFORMATION, *PRTL_USER_PROCESS_INFORMATION;

typedef enum _SHUTDOWN_ACTION {
  ShutdownNoReboot,
  ShutdownReboot,
  ShutdownPowerOff
} SHUTDOWN_ACTION, *PSHUTDOWN_ACTION;

typedef enum _KPROFILE_SOURCE {
  ProfileTime,
  ProfileAlignmentFixup,
  ProfileTotalIssues,
  ProfilePipelineDry,
  ProfileLoadInstructions,
  ProfilePipelineFrozen,
  ProfileBranchInstructions,
  ProfileTotalNonissues,
  ProfileDcacheMisses,
  ProfileIcacheMisses,
  ProfileCacheMisses,
  ProfileBranchMispredictions,
  ProfileStoreInstructions,
  ProfileFpInstructions,
  ProfileIntegerInstructions,
  Profile2Issue,
  Profile3Issue,
  Profile4Issue,
  ProfileSpecialInstructions,
  ProfileTotalCycles,
  ProfileIcacheIssues,
  ProfileDcacheAccesses,
  ProfileMemoryBarrierCycles,
  ProfileLoadLinkedIssues,
  ProfileMaximum
} KPROFILE_SOURCE, *PKPROFILE_SOURCE;

typedef struct _DIRECTORY_BASIC_INFORMATION {
  UNICODE_STRING ObjectName;
  UNICODE_STRING ObjectTypeName;
} DIRECTORY_BASIC_INFORMATION, *PDIRECTORY_BASIC_INFORMATION;

typedef struct _INITIAL_TEB {
    void *OldStackBase;
    void *OldStackLimit;
    void *StackBase;
    void *StackLimit;
    void *DeallocationStack;
} INITIAL_TEB, *PINITIAL_TEB;

typedef enum _PORT_INFORMATION_CLASS {
  PortNoInformation
} PORT_INFORMATION_CLASS, *PPORT_INFORMATION_CLASS;

typedef enum _IO_COMPLETION_INFORMATION_CLASS {
  IoCompletionBasicInformation
} IO_COMPLETION_INFORMATION_CLASS, *PIO_COMPLETION_INFORMATION_CLASS;

typedef struct _FILE_COMPLETION_INFORMATION {
    HANDLE CompletionPort;
    ULONG_PTR CompletionKey;
} FILE_COMPLETION_INFORMATION, *PFILE_COMPLETION_INFORMATION;

#define IO_COMPLETION_QUERY_STATE  0x0001
#define IO_COMPLETION_MODIFY_STATE 0x0002
#define IO_COMPLETION_ALL_ACCESS   (STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|0x3)

typedef struct _FILE_IO_COMPLETION_INFORMATION {
    ULONG_PTR CompletionKey;
    ULONG_PTR CompletionValue;
    IO_STATUS_BLOCK IoStatusBlock;
} FILE_IO_COMPLETION_INFORMATION, *PFILE_IO_COMPLETION_INFORMATION;

typedef enum _HARDERROR_RESPONSE_OPTION {
  OptionAbortRetryIgnore,
  OptionOk,
  OptionOkCancel,
  OptionRetryCancel,
  OptionYesNo,
  OptionYesNoCancel,
  OptionShutdownSystem
} HARDERROR_RESPONSE_OPTION, *PHARDERROR_RESPONSE_OPTION;

typedef enum _HARDERROR_RESPONSE {
  ResponseReturnToCaller,
  ResponseNotHandled,
  ResponseAbort,
  ResponseCancel,
  ResponseIgnore,
  ResponseNo,
  ResponseOk,
  ResponseRetry,
  ResponseYes
} HARDERROR_RESPONSE, *PHARDERROR_RESPONSE;

typedef enum _SYSDBG_COMMAND {
  SysDbgQueryModuleInformation,
  SysDbgQueryTraceInformation,
  SysDbgSetTracepoint,
  SysDbgSetSpecialCall,
  SysDbgClearSpecialCalls,
  SysDbgQuerySpecialCalls,
  SysDbgBreakPoint,
  SysDbgQueryVersion,
  SysDbgReadVirtual,
  SysDbgWriteVirtual,
  SysDbgReadPhysical,
  SysDbgWritePhysical,
  SysDbgReadControlSpace,
  SysDbgWriteControlSpace,
  SysDbgReadIoSpace,
  SysDbgWriteIoSpace,
  SysDbgReadMsr,
  SysDbgWriteMsr,
  SysDbgReadBusData,
  SysDbgWriteBusData,
  SysDbgCheckLowMemory,
  SysDbgEnableKernelDebugger,
  SysDbgDisableKernelDebugger,
  SysDbgGetAutoKdEnable,
  SysDbgSetAutoKdEnable,
  SysDbgGetPrintBufferSize,
  SysDbgSetPrintBufferSize,
  SysDbgGetKdUmExceptionEnable,
  SysDbgSetKdUmExceptionEnable,
  SysDbgGetTriageDump,
  SysDbgGetKdBlockEnable,
  SysDbgSetKdBlockEnable,
  SysDbgRegisterForUmBreakInfo,
  SysDbgGetUmBreakPid,
  SysDbgClearUmBreakPid,
  SysDbgGetUmAttachPid,
  SysDbgClearUmAttachPid,
  SysDbgGetLiveKernelDump,
  SysDbgKdPullRemoteFile,
  SysDbgMaxInfoClass
} SYSDBG_COMMAND, *PSYSDBG_COMMAND;

typedef struct _CPTABLEINFO
{
    USHORT  CodePage;
    USHORT  MaximumCharacterSize;
    USHORT  DefaultChar;
    USHORT  UniDefaultChar;
    USHORT  TransDefaultChar;
    USHORT  TransUniDefaultChar;
    USHORT  DBCSCodePage;
    UCHAR   LeadByte[12];
    USHORT *MultiByteTable;
    void   *WideCharTable;
    USHORT *DBCSRanges;
    USHORT *DBCSOffsets;
} CPTABLEINFO, *PCPTABLEINFO;

typedef struct _NLSTABLEINFO
{
    CPTABLEINFO OemTableInfo;
    CPTABLEINFO AnsiTableInfo;
    USHORT     *UpperCaseTable;
    USHORT     *LowerCaseTable;
} NLSTABLEINFO, *PNLSTABLEINFO;

/*************************************************************************
 * Loader structures
 *
 * Those are not part of standard Winternl.h
 */

typedef struct _LDR_SERVICE_TAG_RECORD
{
    struct _LDR_SERVICE_TAG_RECORD *Next;
    ULONG ServiceTag;
} LDR_SERVICE_TAG_RECORD, *PLDR_SERVICE_TAG_RECORD;

typedef struct _LDRP_CSLIST
{
    SINGLE_LIST_ENTRY *Tail;
} LDRP_CSLIST, *PLDRP_CSLIST;

typedef struct _LDR_DEPENDENCY
{
    SINGLE_LIST_ENTRY dependency_to_entry;
    struct _LDR_DDAG_NODE *dependency_to;
    SINGLE_LIST_ENTRY dependency_from_entry;
    struct _LDR_DDAG_NODE *dependency_from;
} LDR_DEPENDENCY, *PLDR_DEPENDENCY;

typedef enum _LDR_DDAG_STATE
{
    LdrModulesMerged = -5,
    LdrModulesInitError = -4,
    LdrModulesSnapError = -3,
    LdrModulesUnloaded = -2,
    LdrModulesUnloading = -1,
    LdrModulesPlaceHolder = 0,
    LdrModulesMapping = 1,
    LdrModulesMapped = 2,
    LdrModulesWaitingForDependencies = 3,
    LdrModulesSnapping = 4,
    LdrModulesSnapped = 5,
    LdrModulesCondensed = 6,
    LdrModulesReadyToInit = 7,
    LdrModulesInitializing = 8,
    LdrModulesReadyToRun = 9,
} LDR_DDAG_STATE;

typedef struct _LDR_DDAG_NODE
{
    LIST_ENTRY Modules;
    LDR_SERVICE_TAG_RECORD *ServiceTagList;
    LONG LoadCount;
    ULONG LoadWhileUnloadingCount;
    ULONG LowestLink;
    LDRP_CSLIST Dependencies;
    LDRP_CSLIST IncomingDependencies;
    LDR_DDAG_STATE State;
    SINGLE_LIST_ENTRY CondenseLink;
    ULONG PreorderNumber;
} LDR_DDAG_NODE, *PLDR_DDAG_NODE;

typedef enum _LDR_DLL_LOAD_REASON
{
    LoadReasonStaticDependency,
    LoadReasonStaticForwarderDependency,
    LoadReasonDynamicForwarderDependency,
    LoadReasonDelayloadDependency,
    LoadReasonDynamicLoad,
    LoadReasonAsImageLoad,
    LoadReasonAsDataLoad,
    LoadReasonUnknown = -1
} LDR_DLL_LOAD_REASON, *PLDR_DLL_LOAD_REASON;

typedef struct _LDR_DATA_TABLE_ENTRY
{
    LIST_ENTRY          InLoadOrderLinks;
    LIST_ENTRY          InMemoryOrderLinks;
    LIST_ENTRY          InInitializationOrderLinks;
    void*               DllBase;
    void*               EntryPoint;
    ULONG               SizeOfImage;
    UNICODE_STRING      FullDllName;
    UNICODE_STRING      BaseDllName;
    ULONG               Flags;
    SHORT               LoadCount;
    SHORT               TlsIndex;
    LIST_ENTRY          HashLinks;
    ULONG               TimeDateStamp;
    HANDLE              ActivationContext;
    void*               Lock;
    LDR_DDAG_NODE*      DdagNode;
    LIST_ENTRY          NodeModuleLink;
    struct _LDRP_LOAD_CONTEXT *LoadContext;
    void*               ParentDllBase;
    void*               SwitchBackContext;
    RTL_BALANCED_NODE   BaseAddressIndexNode;
    RTL_BALANCED_NODE   MappingInfoIndexNode;
    ULONG_PTR           OriginalBase;
    LARGE_INTEGER       LoadTime;
    ULONG               BaseNameHashValue;
    LDR_DLL_LOAD_REASON LoadReason;
    ULONG               ImplicitPathOptions;
    ULONG               ReferenceCount;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef struct _LDR_DLL_LOADED_NOTIFICATION_DATA
{
    ULONG Flags;
    const UNICODE_STRING *FullDllName;
    const UNICODE_STRING *BaseDllName;
    void *DllBase;
    ULONG SizeOfImage;
} LDR_DLL_LOADED_NOTIFICATION_DATA, *PLDR_DLL_LOADED_NOTIFICATION_DATA;

typedef struct _LDR_DLL_UNLOADED_NOTIFICATION_DATA
{
    ULONG Flags;
    const UNICODE_STRING *FullDllName;
    const UNICODE_STRING *BaseDllName;
    void *DllBase;
    ULONG SizeOfImage;
} LDR_DLL_UNLOADED_NOTIFICATION_DATA, *PLDR_DLL_UNLOADED_NOTIFICATION_DATA;

typedef union _LDR_DLL_NOTIFICATION_DATA
{
    LDR_DLL_LOADED_NOTIFICATION_DATA Loaded;
    LDR_DLL_UNLOADED_NOTIFICATION_DATA Unloaded;
} LDR_DLL_NOTIFICATION_DATA, *PLDR_DLL_NOTIFICATION_DATA;

typedef void (CALLBACK *PLDR_DLL_NOTIFICATION_FUNCTION)(ULONG, LDR_DLL_NOTIFICATION_DATA*, void*);

/* those defines are (some of the) regular LDR_DATA_TABLE_ENTRY.Flags values */
#define LDR_IMAGE_IS_DLL                0x00000004
#define LDR_LOAD_IN_PROGRESS            0x00001000
#define LDR_UNLOAD_IN_PROGRESS          0x00002000
#define LDR_NO_DLL_CALLS                0x00040000
#define LDR_PROCESS_ATTACHED            0x00080000
#define LDR_COR_IMAGE                   0x00400000
#define LDR_COR_ILONLY                  0x01000000

/* these ones is Wine specific */
#define LDR_DONT_RESOLVE_REFS           0x40000000
#define LDR_WINE_INTERNAL               0x80000000

/* flag for LdrAddRefDll */
#define LDR_ADDREF_DLL_PIN              0x00000001

/* flags for LdrGetDllHandleEx */
#define LDR_GET_DLL_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT 0x00000001
#define LDR_GET_DLL_HANDLE_EX_FLAG_PIN                0x00000002

#define LDR_DLL_NOTIFICATION_REASON_LOADED   1
#define LDR_DLL_NOTIFICATION_REASON_UNLOADED 2

/* FIXME: to be checked */
#define MAXIMUM_FILENAME_LENGTH 256

typedef struct _RTL_PROCESS_MODULE_INFORMATION
{
    PVOID               Section;                        /* 00/00 */
    PVOID               MappedBaseAddress;              /* 04/08 */
    PVOID               ImageBaseAddress;               /* 08/10 */
    ULONG               ImageSize;                      /* 0c/18 */
    ULONG               Flags;                          /* 10/1c */
    WORD                LoadOrderIndex;                 /* 14/20 */
    WORD                InitOrderIndex;                 /* 16/22 */
    WORD                LoadCount;                      /* 18/24 */
    WORD                NameOffset;                     /* 1a/26 */
    BYTE                Name[MAXIMUM_FILENAME_LENGTH];  /* 1c/28 */
} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES
{
    ULONG               ModulesCount;
    RTL_PROCESS_MODULE_INFORMATION Modules[1]; /* FIXME: should be Modules[0] */
} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES;

#define PROCESS_CREATE_FLAGS_BREAKAWAY              0x00000001
#define PROCESS_CREATE_FLAGS_NO_DEBUG_INHERIT       0x00000002
#define PROCESS_CREATE_FLAGS_INHERIT_HANDLES        0x00000004
#define PROCESS_CREATE_FLAGS_OVERRIDE_ADDRESS_SPACE 0x00000008
#define PROCESS_CREATE_FLAGS_LARGE_PAGES            0x00000010
#define PROCESS_CREATE_FLAGS_LARGE_PAGE_SYSTEM_DLL  0x00000020
#define PROCESS_CREATE_FLAGS_PROTECTED_PROCESS      0x00000040
#define PROCESS_CREATE_FLAGS_CREATE_SESSION         0x00000080
#define PROCESS_CREATE_FLAGS_INHERIT_FROM_PARENT    0x00000100
#define PROCESS_CREATE_FLAGS_SUSPENDED              0x00000200
#define PROCESS_CREATE_FLAGS_EXTENDED_UNKNOWN       0x00000400

typedef struct _RTL_PROCESS_MODULE_INFORMATION_EX
{
    USHORT NextOffset;
    RTL_PROCESS_MODULE_INFORMATION BaseInfo;
    ULONG ImageCheckSum;
    ULONG TimeDateStamp;
    void *DefaultBase;
} RTL_PROCESS_MODULE_INFORMATION_EX;

#define THREAD_CREATE_FLAGS_CREATE_SUSPENDED        0x00000001
#define THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH      0x00000002
#define THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER      0x00000004
#define THREAD_CREATE_FLAGS_HAS_SECURITY_DESCRIPTOR 0x00000010
#define THREAD_CREATE_FLAGS_ACCESS_CHECK_IN_TARGET  0x00000020
#define THREAD_CREATE_FLAGS_INITIAL_THREAD          0x00000080

#ifdef __WINESRC__

/* Wine-specific exceptions codes */

#define EXCEPTION_WINE_STUB       0x80000100  /* stub entry point called */
#define EXCEPTION_WINE_ASSERTION  0x80000101  /* assertion failed */

/* Wine extension; Windows doesn't have a name for this code.  This is an
   undocumented exception understood by MS VC debugger, allowing the program
   to name a particular thread. */
#define EXCEPTION_WINE_NAME_THREAD     0x406D1388

/* used for C++ exceptions in msvcrt
 * parameters:
 * [0] CXX_FRAME_MAGIC
 * [1] pointer to exception object
 * [2] pointer to type
 */
#define EXCEPTION_WINE_CXX_EXCEPTION   0xe06d7363
#define EXCEPTION_WINE_CXX_FRAME_MAGIC 0x19930520

#endif

typedef LONG (CALLBACK *PRTL_EXCEPTION_FILTER)(PEXCEPTION_POINTERS);

typedef void (CALLBACK *PTP_IO_CALLBACK)(PTP_CALLBACK_INSTANCE,void*,void*,IO_STATUS_BLOCK*,PTP_IO);

#define PS_ATTRIBUTE_THREAD   0x00010000
#define PS_ATTRIBUTE_INPUT    0x00020000
#define PS_ATTRIBUTE_ADDITIVE 0x00040000

typedef enum _PS_ATTRIBUTE_NUM
{
    PsAttributeParentProcess,
    PsAttributeDebugPort,
    PsAttributeToken,
    PsAttributeClientId,
    PsAttributeTebAddress,
    PsAttributeImageName,
    PsAttributeImageInfo,
    PsAttributeMemoryReserve,
    PsAttributePriorityClass,
    PsAttributeErrorMode,
    PsAttributeStdHandleInfo,
    PsAttributeHandleList,
    PsAttributeGroupAffinity,
    PsAttributePreferredNode,
    PsAttributeIdealProcessor,
    PsAttributeUmsThread,
    PsAttributeMitigationOptions,
    PsAttributeProtectionLevel,
    PsAttributeSecureProcess,
    PsAttributeJobList,
    PsAttributeChildProcessPolicy,
    PsAttributeAllApplicationPackagesPolicy,
    PsAttributeWin32kFilter,
    PsAttributeSafeOpenPromptOriginClaim,
    PsAttributeBnoIsolation,
    PsAttributeDesktopAppPolicy,
    PsAttributeChpe,
    PsAttributeMitigationAuditOptions,
    PsAttributeMachineType,
    PsAttributeComponentFilter,
    PsAttributeEnableOptionalXStateFeatures,
    PsAttributeMax
} PS_ATTRIBUTE_NUM;

#define PS_ATTRIBUTE_PARENT_PROCESS     (PsAttributeParentProcess | PS_ATTRIBUTE_INPUT | PS_ATTRIBUTE_ADDITIVE)
#define PS_ATTRIBUTE_DEBUG_PORT         (PsAttributeDebugPort | PS_ATTRIBUTE_INPUT | PS_ATTRIBUTE_ADDITIVE)
#define PS_ATTRIBUTE_TOKEN              (PsAttributeToken | PS_ATTRIBUTE_INPUT | PS_ATTRIBUTE_ADDITIVE)
#define PS_ATTRIBUTE_CLIENT_ID          (PsAttributeClientId | PS_ATTRIBUTE_THREAD)
#define PS_ATTRIBUTE_TEB_ADDRESS        (PsAttributeTebAddress | PS_ATTRIBUTE_THREAD)
#define PS_ATTRIBUTE_IMAGE_NAME         (PsAttributeImageName | PS_ATTRIBUTE_INPUT)
#define PS_ATTRIBUTE_IMAGE_INFO         (PsAttributeImageInfo)
#define PS_ATTRIBUTE_MEMORY_RESERVE     (PsAttributeMemoryReserve | PS_ATTRIBUTE_INPUT)
#define PS_ATTRIBUTE_PRIORITY_CLASS     (PsAttributePriorityClass | PS_ATTRIBUTE_INPUT)
#define PS_ATTRIBUTE_ERROR_MODE         (PsAttributeErrorMode | PS_ATTRIBUTE_INPUT)
#define PS_ATTRIBUTE_STD_HANDLE_INFO    (PsAttributeStdHandleInfo | PS_ATTRIBUTE_INPUT)
#define PS_ATTRIBUTE_HANDLE_LIST        (PsAttributeHandleList | PS_ATTRIBUTE_INPUT)
#define PS_ATTRIBUTE_GROUP_AFFINITY     (PsAttributeGroupAffinity | PS_ATTRIBUTE_THREAD | PS_ATTRIBUTE_INPUT)
#define PS_ATTRIBUTE_PREFERRED_NODE     (PsAttributePreferredNode | PS_ATTRIBUTE_INPUT)
#define PS_ATTRIBUTE_IDEAL_PROCESSOR    (PsAttributeIdealProcessor | PS_ATTRIBUTE_THREAD | PS_ATTRIBUTE_INPUT)
#define PS_ATTRIBUTE_MITIGATION_OPTIONS (PsAttributeMitigationOptions | PS_ATTRIBUTE_INPUT)
#define PS_ATTRIBUTE_PROTECTION_LEVEL   (PsAttributeProtectionLevel | PS_ATTRIBUTE_INPUT | PS_ATTRIBUTE_ADDITIVE)
#define PS_ATTRIBUTE_SECURE_PROCESS     (PsAttributeSecureProcess | PS_ATTRIBUTE_INPUT)
#define PS_ATTRIBUTE_JOB_LIST           (PsAttributeJobList | PS_ATTRIBUTE_INPUT)
#define PS_ATTRIBUTE_CHILD_PROCESS_POLICY (PsAttributeChildProcessPolicy | PS_ATTRIBUTE_INPUT)
#define PS_ATTRIBUTE_ALL_APPLICATION_PACKAGES_POLICY (PsAttributeAllApplicationPackagesPolicy | PS_ATTRIBUTE_INPUT)
#define PS_ATTRIBUTE_WIN32K_FILTER      (PsAttributeWin32kFilter | PS_ATTRIBUTE_INPUT)
#define PS_ATTRIBUTE_SAFE_OPEN_PROMPT_ORIGIN_CLAIM (PsAttributeSafeOpenPromptOriginClaim | PS_ATTRIBUTE_INPUT)
#define PS_ATTRIBUTE_BNO_ISOLATION      (PsAttributeBnoIsolation | PS_ATTRIBUTE_INPUT)
#define PS_ATTRIBUTE_DESKTOP_APP_POLICY (PsAttributeDesktopAppPolicy | PS_ATTRIBUTE_INPUT)
#define PS_ATTRIBUTE_CHPE               (PsAttributeChpe | PS_ATTRIBUTE_INPUT | PS_ATTRIBUTE_ADDITIVE)
#define PS_ATTRIBUTE_MITIGATION_AUDIT_OPTIONS (PsAttributeMitigationAuditOptions | PS_ATTRIBUTE_INPUT)
#define PS_ATTRIBUTE_MACHINE_TYPE       (PsAttributeMachineType | PS_ATTRIBUTE_INPUT | PS_ATTRIBUTE_ADDITIVE)
#define PS_ATTRIBUTE_COMPONENT_FILTER   (PsAttributeComponentFilter | PS_ATTRIBUTE_INPUT)
#define PS_ATTRIBUTE_ENABLE_OPTIONAL_XSTATE_FEATURES (PsAttributeEnableOptionalXStateFeatures | PS_ATTRIBUTE_THREAD | PS_ATTRIBUTE_INPUT)

typedef struct _PS_ATTRIBUTE
{
    ULONG_PTR Attribute;
    SIZE_T    Size;
    union
    {
        ULONG_PTR Value;
        void     *ValuePtr;
    };
    SIZE_T *ReturnLength;
} PS_ATTRIBUTE;

typedef struct _PS_ATTRIBUTE_LIST
{
    SIZE_T       TotalLength;
    PS_ATTRIBUTE Attributes[1];
} PS_ATTRIBUTE_LIST, *PPS_ATTRIBUTE_LIST;

typedef enum _PS_CREATE_STATE
{
    PsCreateInitialState,
    PsCreateFailOnFileOpen,
    PsCreateFailOnSectionCreate,
    PsCreateFailExeFormat,
    PsCreateFailMachineMismatch,
    PsCreateFailExeName,
    PsCreateSuccess,
    PsCreateMaximumStates
} PS_CREATE_STATE;

typedef struct _PS_CREATE_INFO
{
    SIZE_T Size;
    PS_CREATE_STATE State;
    union
    {
        struct
        {
            union
            {
                ULONG InitFlags;
                struct
                {
                    UCHAR WriteOutputOnExit : 1;
                    UCHAR DetectManifest : 1;
                    UCHAR IFEOSkipDebugger : 1;
                    UCHAR IFEODoNotPropagateKeyState : 1;
                    UCHAR SpareBits1 : 4;
                    UCHAR SpareBits2 : 8;
                    USHORT ProhibitedImageCharacteristics : 16;
                };
            };
            ACCESS_MASK AdditionalFileAccess;
        } InitState;
        struct
        {
            HANDLE FileHandle;
        } FailSection;
        struct
        {
            USHORT DllCharacteristics;
        } ExeFormat;
        struct
        {
            HANDLE IFEOKey;
        } ExeName;
        struct
        {
            union
            {
                ULONG OutputFlags;
                struct
                {
                    UCHAR ProtectedProcess : 1;
                    UCHAR AddressSpaceOverride : 1;
                    UCHAR DevOverrideEnabled : 1;
                    UCHAR ManifestDetected : 1;
                    UCHAR ProtectedProcessLight : 1;
                    UCHAR SpareBits1 : 3;
                    UCHAR SpareBits2 : 8;
                    USHORT SpareBits3 : 16;
                };
            };
            HANDLE FileHandle;
            HANDLE SectionHandle;
            ULONGLONG UserProcessParametersNative;
            ULONG UserProcessParametersWow64;
            ULONG CurrentParameterFlags;
            ULONGLONG PebAddressNative;
            ULONG PebAddressWow64;
            ULONGLONG ManifestAddress;
            ULONG ManifestSize;
        } SuccessState;
    };
} PS_CREATE_INFO, *PPS_CREATE_INFO;

typedef struct _DBGKM_EXCEPTION
{
    EXCEPTION_RECORD ExceptionRecord;
    ULONG FirstChance;
} DBGKM_EXCEPTION, *PDBGKM_EXCEPTION;

typedef struct _DBGKM_CREATE_THREAD
{
    ULONG SubSystemKey;
    PVOID StartAddress;
} DBGKM_CREATE_THREAD, *PDBGKM_CREATE_THREAD;

typedef struct _DBGKM_CREATE_PROCESS
{
    ULONG SubSystemKey;
    HANDLE FileHandle;
    PVOID BaseOfImage;
    ULONG DebugInfoFileOffset;
    ULONG DebugInfoSize;
    DBGKM_CREATE_THREAD InitialThread;
} DBGKM_CREATE_PROCESS, *PDBGKM_CREATE_PROCESS;

typedef struct _DBGKM_EXIT_THREAD
{
    NTSTATUS ExitStatus;
} DBGKM_EXIT_THREAD, *PDBGKM_EXIT_THREAD;

typedef struct _DBGKM_EXIT_PROCESS
{
    NTSTATUS ExitStatus;
} DBGKM_EXIT_PROCESS, *PDBGKM_EXIT_PROCESS;

typedef struct _DBGKM_LOAD_DLL
{
    HANDLE FileHandle;
    PVOID BaseOfDll;
    ULONG DebugInfoFileOffset;
    ULONG DebugInfoSize;
    PVOID NamePointer;
} DBGKM_LOAD_DLL, *PDBGKM_LOAD_DLL;

typedef struct _DBGKM_UNLOAD_DLL
{
    PVOID BaseAddress;
} DBGKM_UNLOAD_DLL, *PDBGKM_UNLOAD_DLL;

typedef enum _DBG_STATE
{
    DbgIdle,
    DbgReplyPending,
    DbgCreateThreadStateChange,
    DbgCreateProcessStateChange,
    DbgExitThreadStateChange,
    DbgExitProcessStateChange,
    DbgExceptionStateChange,
    DbgBreakpointStateChange,
    DbgSingleStepStateChange,
    DbgLoadDllStateChange,
    DbgUnloadDllStateChange
} DBG_STATE, *PDBG_STATE;

typedef struct _DBGUI_CREATE_THREAD
{
    HANDLE HandleToThread;
    DBGKM_CREATE_THREAD NewThread;
} DBGUI_CREATE_THREAD, *PDBGUI_CREATE_THREAD;

typedef struct _DBGUI_CREATE_PROCESS
{
    HANDLE HandleToProcess;
    HANDLE HandleToThread;
    DBGKM_CREATE_PROCESS NewProcess;
} DBGUI_CREATE_PROCESS, *PDBGUI_CREATE_PROCESS;

typedef struct _DBGUI_WAIT_STATE_CHANGE
{
    DBG_STATE NewState;
    CLIENT_ID AppClientId;
    union
    {
        DBGKM_EXCEPTION Exception;
        DBGUI_CREATE_THREAD CreateThread;
        DBGUI_CREATE_PROCESS CreateProcessInfo;
        DBGKM_EXIT_THREAD ExitThread;
        DBGKM_EXIT_PROCESS ExitProcess;
        DBGKM_LOAD_DLL LoadDll;
        DBGKM_UNLOAD_DLL UnloadDll;
    } StateInfo;
} DBGUI_WAIT_STATE_CHANGE, *PDBGUI_WAIT_STATE_CHANGE;

struct _DEBUG_EVENT;

#define DEBUG_READ_EVENT        0x0001
#define DEBUG_PROCESS_ASSIGN    0x0002
#define DEBUG_SET_INFORMATION   0x0004
#define DEBUG_QUERY_INFORMATION 0x0008
#define DEBUG_ALL_ACCESS        (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x0f)

#define DEBUG_KILL_ON_CLOSE 0x1

typedef enum _DEBUGOBJECTINFOCLASS
{
    DebugObjectKillProcessOnExitInformation = 1,
    MaxDebugObjectInfoClass
} DEBUGOBJECTINFOCLASS, *PDEBUGOBJECTINFOCLASS;


typedef struct _WOW64_CPURESERVED
{
    USHORT          Flags;
    USHORT          Machine;
    /* CONTEXT context */
    /* CONTEXT_EX *context_ex */
} WOW64_CPURESERVED, *PWOW64_CPURESERVED;

#define WOW64_CPURESERVED_FLAG_RESET_STATE 1

typedef struct _WOW64_CPU_AREA_INFO
{
    void              *Context;
    void              *ContextEx;
    void              *ContextFlagsLocation;
    WOW64_CPURESERVED *CpuReserved;
    ULONG              ContextFlag;
    USHORT             Machine;
} WOW64_CPU_AREA_INFO, *PWOW64_CPU_AREA_INFO;

typedef struct _WOW64INFO
{
    ULONG     NativeSystemPageSize;
    ULONG     CpuFlags;
    ULONG     Wow64ExecuteFlags;
    ULONG     unknown;
    ULONGLONG SectionHandle;
    ULONGLONG CrossProcessWorkList;
    USHORT    NativeMachineType;
    USHORT    EmulatedMachineType;
} WOW64INFO;
C_ASSERT( sizeof(WOW64INFO) == 40 );

#define WOW64_CPUFLAGS_MSFT64   0x01
#define WOW64_CPUFLAGS_SOFTWARE 0x02

/* wow64.dll functions */
void *    WINAPI Wow64AllocateTemp(SIZE_T);
void      WINAPI Wow64ApcRoutine(ULONG_PTR,ULONG_PTR,ULONG_PTR,CONTEXT*);
NTSTATUS  WINAPI Wow64KiUserCallbackDispatcher(ULONG,void*,ULONG,void**,ULONG*);
void      WINAPI Wow64PassExceptionToGuest(EXCEPTION_POINTERS*);
void      WINAPI Wow64PrepareForException(EXCEPTION_RECORD*,CONTEXT*);
void      WINAPI Wow64ProcessPendingCrossProcessItems(void);
NTSTATUS  WINAPI Wow64RaiseException(int,EXCEPTION_RECORD*);
NTSTATUS  WINAPI Wow64SystemServiceEx(UINT,UINT*);

#ifdef __WINESRC__
/* undocumented layout of LdrSystemDllInitBlock */
/* this varies across Windows version; we are using the win10-2004 layout */
typedef struct
{
    ULONG   version;
    ULONG   unknown1[3];
    ULONG64 unknown2;
    ULONG64 pLdrInitializeThunk;
    ULONG64 pKiUserExceptionDispatcher;
    ULONG64 pKiUserApcDispatcher;
    ULONG64 pKiUserCallbackDispatcher;
    ULONG64 pRtlUserThreadStart;
    ULONG64 pRtlpQueryProcessDebugInformationRemote;
    ULONG64 ntdll_handle;
    ULONG64 pLdrSystemDllInitBlock;
    ULONG64 pRtlpFreezeTimeBias;
} SYSTEM_DLL_INIT_BLOCK;
#endif

typedef struct
{
    ULONG_PTR *ServiceTable;
    ULONG_PTR *CounterTable;
    ULONG_PTR  ServiceLimit;
    BYTE      *ArgumentTable;
} SYSTEM_SERVICE_TABLE;

/* ApiSet structures (format for version 6) */

typedef struct _API_SET_NAMESPACE
{
    ULONG Version;
    ULONG Size;
    ULONG Flags;
    ULONG Count;
    ULONG EntryOffset;
    ULONG HashOffset;
    ULONG HashFactor;
} API_SET_NAMESPACE;

typedef struct _API_SET_HASH_ENTRY
{
    ULONG Hash;
    ULONG Index;
} API_SET_HASH_ENTRY;

typedef struct _API_SET_NAMESPACE_ENTRY
{
    ULONG Flags;
    ULONG NameOffset;
    ULONG NameLength;
    ULONG HashedLength;
    ULONG ValueOffset;
    ULONG ValueCount;
} API_SET_NAMESPACE_ENTRY;

typedef struct _API_SET_VALUE_ENTRY
{
    ULONG Flags;
    ULONG NameOffset;
    ULONG NameLength;
    ULONG ValueOffset;
    ULONG ValueLength;
} API_SET_VALUE_ENTRY;

typedef enum _KCONTINUE_TYPE
{
    KCONTINUE_UNWIND,
    KCONTINUE_RESUME,
    KCONTINUE_LONGJUMP,
    KCONTINUE_SET,
    KCONTINUE_LAST,
} KCONTINUE_TYPE;

typedef struct _KCONTINUE_ARGUMENT
{
    KCONTINUE_TYPE ContinueType;
    ULONG          ContinueFlags;
    ULONGLONG      Reserved[2];
} KCONTINUE_ARGUMENT, *PKCONTINUE_ARGUMENT;

#define KCONTINUE_FLAG_TEST_ALERT  0x01
#define KCONTINUE_FLAG_DELIVER_APC 0x02


#define HASH_STRING_ALGORITHM_DEFAULT  0
#define HASH_STRING_ALGORITHM_X65599   1
#define HASH_STRING_ALGORITHM_INVALID  0xffffffff

/***********************************************************************
 * Function declarations
 */

NTSYSAPI NTSTATUS  WINAPI ApiSetQueryApiSetPresence(const UNICODE_STRING*,BOOLEAN*);
NTSYSAPI NTSTATUS  WINAPI ApiSetQueryApiSetPresenceEx(const UNICODE_STRING*,BOOLEAN*,BOOLEAN*);
NTSYSAPI void      WINAPI DbgBreakPoint(void);
NTSYSAPI NTSTATUS WINAPIV DbgPrint(LPCSTR fmt, ...);
NTSYSAPI NTSTATUS WINAPIV DbgPrintEx(ULONG iComponentId, ULONG Level, LPCSTR fmt, ...);
NTSYSAPI NTSTATUS  WINAPI DbgUiConnectToDbg(void);
NTSYSAPI NTSTATUS  WINAPI DbgUiContinue(CLIENT_ID*,NTSTATUS);
NTSYSAPI NTSTATUS  WINAPI DbgUiConvertStateChangeStructure(DBGUI_WAIT_STATE_CHANGE*,struct _DEBUG_EVENT*);
NTSYSAPI NTSTATUS WINAPI  DbgUiDebugActiveProcess(HANDLE);
NTSYSAPI HANDLE    WINAPI DbgUiGetThreadDebugObject(void);
NTSYSAPI NTSTATUS  WINAPI DbgUiIssueRemoteBreakin(HANDLE);
NTSYSAPI void      WINAPI DbgUiRemoteBreakin(void*);
NTSYSAPI void      WINAPI DbgUiSetThreadDebugObject(HANDLE);
NTSYSAPI NTSTATUS  WINAPI DbgUiStopDebugging(HANDLE);
NTSYSAPI NTSTATUS  WINAPI DbgUiWaitStateChange(DBGUI_WAIT_STATE_CHANGE*,LARGE_INTEGER*);
NTSYSAPI void      WINAPI DbgUserBreakPoint(void);
NTSYSAPI NTSTATUS  WINAPI LdrAccessResource(HMODULE,const IMAGE_RESOURCE_DATA_ENTRY*,void**,PULONG);
NTSYSAPI NTSTATUS  WINAPI LdrAddDllDirectory(const UNICODE_STRING*,void**);
NTSYSAPI NTSTATUS  WINAPI LdrAddRefDll(ULONG,HMODULE);
NTSYSAPI NTSTATUS  WINAPI LdrDisableThreadCalloutsForDll(HMODULE);
NTSYSAPI NTSTATUS  WINAPI LdrFindEntryForAddress(const void*, PLDR_DATA_TABLE_ENTRY*);
NTSYSAPI NTSTATUS  WINAPI LdrFindResourceDirectory_U(HMODULE,const LDR_RESOURCE_INFO*,ULONG,const IMAGE_RESOURCE_DIRECTORY**);
NTSYSAPI NTSTATUS  WINAPI LdrFindResource_U(HMODULE,const LDR_RESOURCE_INFO*,ULONG,const IMAGE_RESOURCE_DATA_ENTRY**);
NTSYSAPI NTSTATUS  WINAPI LdrGetDllDirectory(UNICODE_STRING*);
NTSYSAPI NTSTATUS  WINAPI LdrGetDllFullName(HMODULE, UNICODE_STRING*);
NTSYSAPI NTSTATUS  WINAPI LdrGetDllHandle(LPCWSTR, ULONG, const UNICODE_STRING*, HMODULE*);
NTSYSAPI NTSTATUS  WINAPI LdrGetDllHandleEx(ULONG, LPCWSTR, ULONG *, const UNICODE_STRING*, HMODULE*);
NTSYSAPI NTSTATUS  WINAPI LdrGetDllPath(PCWSTR,ULONG,PWSTR*,PWSTR*);
NTSYSAPI NTSTATUS  WINAPI LdrGetProcedureAddress(HMODULE, const ANSI_STRING*, ULONG, void**);
NTSYSAPI NTSTATUS  WINAPI LdrLoadDll(LPCWSTR, DWORD, const UNICODE_STRING*, HMODULE*);
NTSYSAPI NTSTATUS  WINAPI LdrLockLoaderLock(ULONG,ULONG*,ULONG_PTR*);
IMAGE_BASE_RELOCATION * WINAPI LdrProcessRelocationBlock(void*,UINT,USHORT*,INT_PTR);
NTSYSAPI NTSTATUS  WINAPI LdrQueryImageFileExecutionOptions(const UNICODE_STRING*,LPCWSTR,ULONG,void*,ULONG,ULONG*);
NTSYSAPI NTSTATUS  WINAPI LdrQueryProcessModuleInformation(RTL_PROCESS_MODULES*, ULONG, ULONG*);
NTSYSAPI NTSTATUS  WINAPI LdrRegisterDllNotification(ULONG,PLDR_DLL_NOTIFICATION_FUNCTION,void*,void**);
NTSYSAPI NTSTATUS  WINAPI LdrRemoveDllDirectory(void*);
NTSYSAPI NTSTATUS  WINAPI LdrSetDefaultDllDirectories(ULONG);
NTSYSAPI NTSTATUS  WINAPI LdrSetDllDirectory(const UNICODE_STRING*);
NTSYSAPI void      WINAPI LdrShutdownProcess(void);
NTSYSAPI void      WINAPI LdrShutdownThread(void);
NTSYSAPI NTSTATUS  WINAPI LdrUnloadDll(HMODULE);
NTSYSAPI NTSTATUS  WINAPI LdrUnlockLoaderLock(ULONG,ULONG_PTR);
NTSYSAPI NTSTATUS  WINAPI LdrUnregisterDllNotification(void*);
NTSYSAPI NTSTATUS  WINAPI NtAcceptConnectPort(PHANDLE,ULONG,PLPC_MESSAGE,BOOLEAN,PLPC_SECTION_WRITE,PLPC_SECTION_READ);
NTSYSAPI NTSTATUS  WINAPI NtAccessCheck(PSECURITY_DESCRIPTOR,HANDLE,ACCESS_MASK,PGENERIC_MAPPING,PPRIVILEGE_SET,PULONG,PULONG,NTSTATUS*);
NTSYSAPI NTSTATUS  WINAPI NtAccessCheckAndAuditAlarm(PUNICODE_STRING,HANDLE,PUNICODE_STRING,PUNICODE_STRING,PSECURITY_DESCRIPTOR,ACCESS_MASK,PGENERIC_MAPPING,BOOLEAN,PACCESS_MASK,PBOOLEAN,PBOOLEAN);
NTSYSAPI NTSTATUS  WINAPI NtAddAtom(const WCHAR*,ULONG,RTL_ATOM*);
NTSYSAPI NTSTATUS  WINAPI NtAdjustGroupsToken(HANDLE,BOOLEAN,PTOKEN_GROUPS,ULONG,PTOKEN_GROUPS,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtAdjustPrivilegesToken(HANDLE,BOOLEAN,PTOKEN_PRIVILEGES,DWORD,PTOKEN_PRIVILEGES,PDWORD);
NTSYSAPI NTSTATUS  WINAPI NtAlertResumeThread(HANDLE,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtAlertThread(HANDLE ThreadHandle);
NTSYSAPI NTSTATUS  WINAPI NtAlertThreadByThreadId(HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtAllocateLocallyUniqueId(PLUID lpLuid);
NTSYSAPI NTSTATUS  WINAPI NtAllocateReserveObject(HANDLE *handle,const OBJECT_ATTRIBUTES *attr,MEMORY_RESERVE_OBJECT_TYPE type);
NTSYSAPI NTSTATUS  WINAPI NtAllocateUuids(PULARGE_INTEGER,PULONG,PULONG,PUCHAR);
NTSYSAPI NTSTATUS  WINAPI NtAllocateVirtualMemory(HANDLE,PVOID*,ULONG_PTR,SIZE_T*,ULONG,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtAllocateVirtualMemoryEx(HANDLE,PVOID*,SIZE_T*,ULONG,ULONG,MEM_EXTENDED_PARAMETER*,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtAreMappedFilesTheSame(PVOID,PVOID);
NTSYSAPI NTSTATUS  WINAPI NtAssignProcessToJobObject(HANDLE,HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtCallbackReturn(PVOID,ULONG,NTSTATUS);
NTSYSAPI NTSTATUS  WINAPI NtCancelIoFile(HANDLE,PIO_STATUS_BLOCK);
NTSYSAPI NTSTATUS  WINAPI NtCancelIoFileEx(HANDLE,PIO_STATUS_BLOCK,PIO_STATUS_BLOCK);
NTSYSAPI NTSTATUS  WINAPI NtCancelSynchronousIoFile(HANDLE,PIO_STATUS_BLOCK,PIO_STATUS_BLOCK);
NTSYSAPI NTSTATUS  WINAPI NtCancelTimer(HANDLE, BOOLEAN*);
NTSYSAPI NTSTATUS  WINAPI NtClearEvent(HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtClose(HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtCloseObjectAuditAlarm(PUNICODE_STRING,HANDLE,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI NtCommitTransaction(HANDLE,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI NtCompareObjects(HANDLE,HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtCompareTokens(HANDLE,HANDLE,BOOLEAN*);
NTSYSAPI NTSTATUS  WINAPI NtCompleteConnectPort(HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtConnectPort(PHANDLE,PUNICODE_STRING,PSECURITY_QUALITY_OF_SERVICE,PLPC_SECTION_WRITE,PLPC_SECTION_READ,PULONG,PVOID,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtContinue(PCONTEXT,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI NtContinueEx(CONTEXT*,KCONTINUE_ARGUMENT*);
NTSYSAPI NTSTATUS  WINAPI NtCreateDebugObject(HANDLE*,ACCESS_MASK,OBJECT_ATTRIBUTES*,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtCreateDirectoryObject(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSYSAPI NTSTATUS  WINAPI NtCreateEvent(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES *,EVENT_TYPE,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI NtCreateEventPair(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSYSAPI NTSTATUS  WINAPI NtCreateFile(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,PLARGE_INTEGER,ULONG,ULONG,ULONG,ULONG,PVOID,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtCreateIoCompletion(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtCreateJobObject(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*);
NTSYSAPI NTSTATUS  WINAPI NtCreateKey(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*,ULONG,const UNICODE_STRING*,ULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtCreateKeyTransacted(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*,ULONG,const UNICODE_STRING*,ULONG,HANDLE,ULONG*);
NTSYSAPI NTSTATUS  WINAPI NtCreateKeyedEvent(HANDLE*,ACCESS_MASK,const OBJECT_ATTRIBUTES*,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtCreateLowBoxToken(HANDLE*,HANDLE,ACCESS_MASK,OBJECT_ATTRIBUTES*,SID*,ULONG,SID_AND_ATTRIBUTES*,ULONG,HANDLE*);
NTSYSAPI NTSTATUS  WINAPI NtCreateMailslotFile(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,ULONG,ULONG,ULONG,PLARGE_INTEGER);
NTSYSAPI NTSTATUS  WINAPI NtCreateMutant(HANDLE*,ACCESS_MASK,const OBJECT_ATTRIBUTES*,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI NtCreateNamedPipeFile(PHANDLE,ULONG,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,ULONG,ULONG,ULONG,ULONG,ULONG,ULONG,ULONG,ULONG,ULONG,PLARGE_INTEGER);
NTSYSAPI NTSTATUS  WINAPI NtCreatePagingFile(PUNICODE_STRING,PLARGE_INTEGER,PLARGE_INTEGER,PLARGE_INTEGER);
NTSYSAPI NTSTATUS  WINAPI NtCreatePort(PHANDLE,POBJECT_ATTRIBUTES,ULONG,ULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtCreateProcess(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,HANDLE,BOOLEAN,HANDLE,HANDLE,HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtCreateProfile(PHANDLE,HANDLE,PVOID,ULONG,ULONG,PVOID,ULONG,KPROFILE_SOURCE,KAFFINITY);
NTSYSAPI NTSTATUS  WINAPI NtCreateSection(HANDLE*,ACCESS_MASK,const OBJECT_ATTRIBUTES*,const LARGE_INTEGER*,ULONG,ULONG,HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtCreateSemaphore(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*,LONG,LONG);
NTSYSAPI NTSTATUS  WINAPI NtCreateSymbolicLinkObject(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PUNICODE_STRING);
NTSYSAPI NTSTATUS  WINAPI NtCreateThread(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,HANDLE,PCLIENT_ID,PCONTEXT,PINITIAL_TEB,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI NtCreateThreadEx(HANDLE*,ACCESS_MASK,OBJECT_ATTRIBUTES*,HANDLE,PRTL_THREAD_START_ROUTINE,void*,ULONG,ULONG_PTR,SIZE_T,SIZE_T,PS_ATTRIBUTE_LIST*);
NTSYSAPI NTSTATUS  WINAPI NtCreateTimer(HANDLE*, ACCESS_MASK, const OBJECT_ATTRIBUTES*, TIMER_TYPE);
NTSYSAPI NTSTATUS  WINAPI NtCreateToken(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,TOKEN_TYPE,PLUID,PLARGE_INTEGER,PTOKEN_USER,PTOKEN_GROUPS,PTOKEN_PRIVILEGES,PTOKEN_OWNER,PTOKEN_PRIMARY_GROUP,PTOKEN_DEFAULT_DACL,PTOKEN_SOURCE);
NTSYSAPI NTSTATUS  WINAPI NtCreateTransaction(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,LPGUID,HANDLE,ULONG,ULONG,ULONG,PLARGE_INTEGER,PUNICODE_STRING);
NTSYSAPI NTSTATUS  WINAPI NtCreateUserProcess(HANDLE*,HANDLE*,ACCESS_MASK,ACCESS_MASK,OBJECT_ATTRIBUTES*,OBJECT_ATTRIBUTES*,ULONG,ULONG,RTL_USER_PROCESS_PARAMETERS*,PS_CREATE_INFO*,PS_ATTRIBUTE_LIST*);
NTSYSAPI NTSTATUS  WINAPI NtDebugActiveProcess(HANDLE,HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtDebugContinue(HANDLE,CLIENT_ID*,NTSTATUS);
NTSYSAPI NTSTATUS  WINAPI NtDelayExecution(BOOLEAN,const LARGE_INTEGER*);
NTSYSAPI NTSTATUS  WINAPI NtDeleteAtom(RTL_ATOM);
NTSYSAPI NTSTATUS  WINAPI NtDeleteFile(POBJECT_ATTRIBUTES);
NTSYSAPI NTSTATUS  WINAPI NtDeleteKey(HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtDeleteValueKey(HANDLE,const UNICODE_STRING *);
NTSYSAPI NTSTATUS  WINAPI NtDeviceIoControlFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,ULONG,PVOID,ULONG,PVOID,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtDisplayString(PUNICODE_STRING);
NTSYSAPI NTSTATUS  WINAPI NtDuplicateObject(HANDLE,HANDLE,HANDLE,PHANDLE,ACCESS_MASK,ULONG,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtDuplicateToken(HANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,BOOLEAN,TOKEN_TYPE,PHANDLE);
NTSYSAPI NTSTATUS  WINAPI NtEnumerateKey(HANDLE,ULONG,KEY_INFORMATION_CLASS,void *,DWORD,DWORD *);
NTSYSAPI NTSTATUS  WINAPI NtEnumerateValueKey(HANDLE,ULONG,KEY_VALUE_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtExtendSection(HANDLE,PLARGE_INTEGER);
NTSYSAPI NTSTATUS  WINAPI NtFilterToken(HANDLE,ULONG,TOKEN_GROUPS*,TOKEN_PRIVILEGES*,TOKEN_GROUPS*,HANDLE*);
NTSYSAPI NTSTATUS  WINAPI NtFindAtom(const WCHAR*,ULONG,RTL_ATOM*);
NTSYSAPI NTSTATUS  WINAPI NtFlushBuffersFile(HANDLE,IO_STATUS_BLOCK*);
NTSYSAPI NTSTATUS  WINAPI NtFlushBuffersFileEx(HANDLE,ULONG,void*,ULONG,IO_STATUS_BLOCK*);
NTSYSAPI NTSTATUS  WINAPI NtFlushInstructionCache(HANDLE,LPCVOID,SIZE_T);
NTSYSAPI NTSTATUS  WINAPI NtFlushKey(HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtFlushProcessWriteBuffers(void);
NTSYSAPI NTSTATUS  WINAPI NtFlushVirtualMemory(HANDLE,LPCVOID*,SIZE_T*,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtFlushWriteBuffer(VOID);
NTSYSAPI NTSTATUS  WINAPI NtFreeVirtualMemory(HANDLE,PVOID*,SIZE_T*,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtFsControlFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,ULONG,PVOID,ULONG,PVOID,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtGetContextThread(HANDLE,CONTEXT*);
NTSYSAPI ULONG     WINAPI NtGetCurrentProcessorNumber(void);
NTSYSAPI NTSTATUS  WINAPI NtGetNextThread(HANDLE,HANDLE,ACCESS_MASK,ULONG,ULONG,HANDLE*);
NTSYSAPI NTSTATUS  WINAPI NtGetNlsSectionPtr(ULONG,ULONG,void*,void**,SIZE_T*);
NTSYSAPI NTSTATUS  WINAPI NtGetPlugPlayEvent(ULONG,ULONG,PVOID,ULONG);
NTSYSAPI ULONG     WINAPI NtGetTickCount(VOID);
NTSYSAPI NTSTATUS  WINAPI NtGetWriteWatch(HANDLE,ULONG,PVOID,SIZE_T,PVOID*,ULONG_PTR*,ULONG*);
NTSYSAPI NTSTATUS  WINAPI NtImpersonateAnonymousToken(HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtImpersonateClientOfPort(HANDLE,PPORT_MESSAGE);
NTSYSAPI NTSTATUS  WINAPI NtImpersonateThread(HANDLE,HANDLE,PSECURITY_QUALITY_OF_SERVICE);
NTSYSAPI NTSTATUS  WINAPI NtInitializeNlsFiles(void**,LCID*,LARGE_INTEGER*);
NTSYSAPI NTSTATUS  WINAPI NtInitializeRegistry(BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI NtInitiatePowerAction(POWER_ACTION,SYSTEM_POWER_STATE,ULONG,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI NtIsProcessInJob(HANDLE,HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtListenPort(HANDLE,PLPC_MESSAGE);
NTSYSAPI NTSTATUS  WINAPI NtLoadDriver(const UNICODE_STRING *);
NTSYSAPI NTSTATUS  WINAPI NtLoadKey(const OBJECT_ATTRIBUTES *,OBJECT_ATTRIBUTES *);
NTSYSAPI NTSTATUS  WINAPI NtLoadKey2(const OBJECT_ATTRIBUTES *,OBJECT_ATTRIBUTES *,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtLoadKeyEx(const OBJECT_ATTRIBUTES *,OBJECT_ATTRIBUTES *,ULONG,HANDLE,HANDLE,ACCESS_MASK,HANDLE *,IO_STATUS_BLOCK *);
NTSYSAPI NTSTATUS  WINAPI NtLockFile(HANDLE,HANDLE,PIO_APC_ROUTINE,void*,PIO_STATUS_BLOCK,PLARGE_INTEGER,PLARGE_INTEGER,ULONG*,BOOLEAN,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI NtLockVirtualMemory(HANDLE,PVOID*,SIZE_T*,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtMakePermanentObject(HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtMakeTemporaryObject(HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtMapViewOfSection(HANDLE,HANDLE,PVOID*,ULONG_PTR,SIZE_T,const LARGE_INTEGER*,SIZE_T*,SECTION_INHERIT,ULONG,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtMapViewOfSectionEx(HANDLE,HANDLE,PVOID*,const LARGE_INTEGER*,SIZE_T*,ULONG,ULONG,MEM_EXTENDED_PARAMETER*,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtNotifyChangeDirectoryFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,PVOID,ULONG,ULONG,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI NtNotifyChangeKey(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,ULONG,BOOLEAN,PVOID,ULONG,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI NtNotifyChangeMultipleKeys(HANDLE,ULONG,OBJECT_ATTRIBUTES*,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,ULONG,BOOLEAN,PVOID,ULONG,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI NtOpenDirectoryObject(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*);
NTSYSAPI NTSTATUS  WINAPI NtOpenEvent(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES *);
NTSYSAPI NTSTATUS  WINAPI NtOpenEventPair(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*);
NTSYSAPI NTSTATUS  WINAPI NtOpenFile(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,ULONG,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtOpenIoCompletion(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*);
NTSYSAPI NTSTATUS  WINAPI NtOpenJobObject(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*);
NTSYSAPI NTSTATUS  WINAPI NtOpenKey(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES *);
NTSYSAPI NTSTATUS  WINAPI NtOpenKeyEx(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtOpenKeyTransacted(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*,HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtOpenKeyTransactedEx(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*,ULONG,HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtOpenKeyedEvent(HANDLE*,ACCESS_MASK,const OBJECT_ATTRIBUTES*);
NTSYSAPI NTSTATUS  WINAPI NtOpenMutant(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*);
NTSYSAPI NTSTATUS  WINAPI NtOpenObjectAuditAlarm(PUNICODE_STRING,PHANDLE,PUNICODE_STRING,PUNICODE_STRING,PSECURITY_DESCRIPTOR,HANDLE,ACCESS_MASK,ACCESS_MASK,PPRIVILEGE_SET,BOOLEAN,BOOLEAN,PBOOLEAN);
NTSYSAPI NTSTATUS  WINAPI NtOpenProcess(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*,const CLIENT_ID*);
NTSYSAPI NTSTATUS  WINAPI NtOpenProcessToken(HANDLE,DWORD,HANDLE *);
NTSYSAPI NTSTATUS  WINAPI NtOpenProcessTokenEx(HANDLE,DWORD,DWORD,HANDLE *);
NTSYSAPI NTSTATUS  WINAPI NtOpenSection(HANDLE*,ACCESS_MASK,const OBJECT_ATTRIBUTES*);
NTSYSAPI NTSTATUS  WINAPI NtOpenSemaphore(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*);
NTSYSAPI NTSTATUS  WINAPI NtOpenSymbolicLinkObject(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*);
NTSYSAPI NTSTATUS  WINAPI NtOpenThread(HANDLE*,ACCESS_MASK,const OBJECT_ATTRIBUTES*,const CLIENT_ID*);
NTSYSAPI NTSTATUS  WINAPI NtOpenThreadToken(HANDLE,DWORD,BOOLEAN,HANDLE *);
NTSYSAPI NTSTATUS  WINAPI NtOpenThreadTokenEx(HANDLE,DWORD,BOOLEAN,DWORD,HANDLE *);
NTSYSAPI NTSTATUS  WINAPI NtOpenTimer(HANDLE*, ACCESS_MASK, const OBJECT_ATTRIBUTES*);
NTSYSAPI NTSTATUS  WINAPI NtPowerInformation(POWER_INFORMATION_LEVEL,PVOID,ULONG,PVOID,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtPrivilegeCheck(HANDLE,PPRIVILEGE_SET,PBOOLEAN);
NTSYSAPI NTSTATUS  WINAPI NtPrivilegeObjectAuditAlarm(PUNICODE_STRING,HANDLE,HANDLE,ULONG,PPRIVILEGE_SET,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI NtPrivilegedServiceAuditAlarm(PUNICODE_STRING,PUNICODE_STRING,HANDLE,PPRIVILEGE_SET,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI NtProtectVirtualMemory(HANDLE,PVOID*,SIZE_T*,ULONG,ULONG*);
NTSYSAPI NTSTATUS  WINAPI NtPulseEvent(HANDLE,LONG*);
NTSYSAPI NTSTATUS  WINAPI NtQueueApcThread(HANDLE,PNTAPCFUNC,ULONG_PTR,ULONG_PTR,ULONG_PTR);
NTSYSAPI NTSTATUS  WINAPI NtQueueApcThreadEx(HANDLE,HANDLE,PNTAPCFUNC,ULONG_PTR,ULONG_PTR,ULONG_PTR);
NTSYSAPI NTSTATUS  WINAPI NtQueryAttributesFile(const OBJECT_ATTRIBUTES*,FILE_BASIC_INFORMATION*);
NTSYSAPI NTSTATUS  WINAPI NtQueryDefaultLocale(BOOLEAN,LCID*);
NTSYSAPI NTSTATUS  WINAPI NtQueryDefaultUILanguage(LANGID*);
NTSYSAPI NTSTATUS  WINAPI NtQueryDirectoryFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,PVOID,ULONG,FILE_INFORMATION_CLASS,BOOLEAN,PUNICODE_STRING,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI NtQueryDirectoryObject(HANDLE,PDIRECTORY_BASIC_INFORMATION,ULONG,BOOLEAN,BOOLEAN,PULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtQueryEaFile(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,BOOLEAN,PVOID,ULONG,PULONG,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI NtQueryEvent(HANDLE,EVENT_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtQueryFullAttributesFile(const OBJECT_ATTRIBUTES*,FILE_NETWORK_OPEN_INFORMATION*);
NTSYSAPI NTSTATUS  WINAPI NtQueryInformationAtom(RTL_ATOM,ATOM_INFORMATION_CLASS,PVOID,ULONG,ULONG*);
NTSYSAPI NTSTATUS  WINAPI NtQueryInformationFile(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,FILE_INFORMATION_CLASS);
NTSYSAPI NTSTATUS  WINAPI NtQueryInformationJobObject(HANDLE,JOBOBJECTINFOCLASS,PVOID,ULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtQueryInformationPort(HANDLE,PORT_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtQueryInformationProcess(HANDLE,PROCESSINFOCLASS,PVOID,ULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtQueryInformationThread(HANDLE,THREADINFOCLASS,PVOID,ULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtQueryInformationToken(HANDLE,TOKEN_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtQueryInstallUILanguage(LANGID*);
NTSYSAPI NTSTATUS  WINAPI NtQueryIntervalProfile(KPROFILE_SOURCE,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtQueryIoCompletion(HANDLE,IO_COMPLETION_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtQueryKey(HANDLE,KEY_INFORMATION_CLASS,void *,DWORD,DWORD *);
NTSYSAPI NTSTATUS  WINAPI NtQueryMultipleValueKey(HANDLE,PKEY_MULTIPLE_VALUE_INFORMATION,ULONG,PVOID,ULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtQueryMutant(HANDLE,MUTANT_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtQueryObject(HANDLE, OBJECT_INFORMATION_CLASS, PVOID, ULONG, PULONG);
NTSYSAPI NTSTATUS  WINAPI NtQueryOpenSubKeys(POBJECT_ATTRIBUTES,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtQueryPerformanceCounter(PLARGE_INTEGER, PLARGE_INTEGER);
NTSYSAPI NTSTATUS  WINAPI NtQuerySecurityObject(HANDLE,SECURITY_INFORMATION,PSECURITY_DESCRIPTOR,ULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtQuerySection(HANDLE,SECTION_INFORMATION_CLASS,PVOID,SIZE_T,SIZE_T*);
NTSYSAPI NTSTATUS  WINAPI NtQuerySemaphore(HANDLE,SEMAPHORE_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtQuerySymbolicLinkObject(HANDLE,PUNICODE_STRING,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtQuerySystemEnvironmentValue(PUNICODE_STRING,PWCHAR,ULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtQuerySystemEnvironmentValueEx(PUNICODE_STRING,GUID*,void*,ULONG*,ULONG*);
NTSYSAPI NTSTATUS  WINAPI NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtQuerySystemInformationEx(SYSTEM_INFORMATION_CLASS,void*,ULONG,void*,ULONG,ULONG*);
NTSYSAPI NTSTATUS  WINAPI NtQuerySystemTime(PLARGE_INTEGER);
NTSYSAPI NTSTATUS  WINAPI NtQueryTimer(HANDLE,TIMER_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtQueryTimerResolution(PULONG,PULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtQueryValueKey(HANDLE,const UNICODE_STRING *,KEY_VALUE_INFORMATION_CLASS,void *,DWORD,DWORD *);
NTSYSAPI NTSTATUS  WINAPI NtQueryLicenseValue(const UNICODE_STRING *,ULONG *,PVOID,ULONG,ULONG *);
NTSYSAPI NTSTATUS  WINAPI NtQueryVirtualMemory(HANDLE,LPCVOID,MEMORY_INFORMATION_CLASS,PVOID,SIZE_T,SIZE_T*);
NTSYSAPI NTSTATUS  WINAPI NtQueryVolumeInformationFile(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,FS_INFORMATION_CLASS);
NTSYSAPI NTSTATUS  WINAPI NtRaiseException(PEXCEPTION_RECORD,PCONTEXT,BOOL);
NTSYSAPI NTSTATUS  WINAPI NtRaiseHardError(NTSTATUS,ULONG,ULONG,PVOID*,HARDERROR_RESPONSE_OPTION,PHARDERROR_RESPONSE);
NTSYSAPI NTSTATUS  WINAPI NtReadFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,PVOID,ULONG,PLARGE_INTEGER,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtReadFileScatter(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,FILE_SEGMENT_ELEMENT*,ULONG,PLARGE_INTEGER,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtReadRequestData(HANDLE,PLPC_MESSAGE,ULONG,PVOID,ULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtReadVirtualMemory(HANDLE,const void*,void*,SIZE_T,SIZE_T*);
NTSYSAPI NTSTATUS  WINAPI NtRegisterThreadTerminatePort(HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtReleaseKeyedEvent(HANDLE,const void*,BOOLEAN,const LARGE_INTEGER*);
NTSYSAPI NTSTATUS  WINAPI NtReleaseMutant(HANDLE,PLONG);
NTSYSAPI NTSTATUS  WINAPI NtReleaseSemaphore(HANDLE,ULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtRemoveIoCompletion(HANDLE,PULONG_PTR,PULONG_PTR,PIO_STATUS_BLOCK,PLARGE_INTEGER);
NTSYSAPI NTSTATUS  WINAPI NtRemoveIoCompletionEx(HANDLE,FILE_IO_COMPLETION_INFORMATION*,ULONG,ULONG*,LARGE_INTEGER*,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI NtRemoveProcessDebug(HANDLE,HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtRenameKey(HANDLE,UNICODE_STRING*);
NTSYSAPI NTSTATUS  WINAPI NtReplaceKey(POBJECT_ATTRIBUTES,HANDLE,POBJECT_ATTRIBUTES);
NTSYSAPI NTSTATUS  WINAPI NtReplyPort(HANDLE,PLPC_MESSAGE);
NTSYSAPI NTSTATUS  WINAPI NtReplyWaitReceivePort(HANDLE,PULONG,PLPC_MESSAGE,PLPC_MESSAGE);
NTSYSAPI NTSTATUS  WINAPI NtReplyWaitReceivePortEx(HANDLE,PVOID*,PPORT_MESSAGE,PPORT_MESSAGE,PLARGE_INTEGER);
NTSYSAPI NTSTATUS  WINAPI NtReplyWaitReplyPort(HANDLE,PLPC_MESSAGE);
NTSYSAPI NTSTATUS  WINAPI NtRequestPort(HANDLE,PLPC_MESSAGE);
NTSYSAPI NTSTATUS  WINAPI NtRequestWaitReplyPort(HANDLE,PLPC_MESSAGE,PLPC_MESSAGE);
NTSYSAPI NTSTATUS  WINAPI NtResetEvent(HANDLE,LONG*);
NTSYSAPI NTSTATUS  WINAPI NtResetWriteWatch(HANDLE,PVOID,SIZE_T);
NTSYSAPI NTSTATUS  WINAPI NtRestoreKey(HANDLE,HANDLE,ULONG);
#ifndef __REACTOS__
NTSYSAPI NTSTATUS  WINAPI NtResumeProcess(HANDLE);
#endif
NTSYSAPI NTSTATUS  WINAPI NtResumeThread(HANDLE,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtRollbackTransaction(HANDLE,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI NtSaveKey(HANDLE,HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtSecureConnectPort(PHANDLE,PUNICODE_STRING,PSECURITY_QUALITY_OF_SERVICE,PLPC_SECTION_WRITE,PSID,PLPC_SECTION_READ,PULONG,PVOID,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtSetContextThread(HANDLE,const CONTEXT*);
NTSYSAPI NTSTATUS  WINAPI NtSetDebugFilterState(ULONG,ULONG,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI NtSetDefaultHardErrorPort(HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtSetDefaultLocale(BOOLEAN,LCID);
NTSYSAPI NTSTATUS  WINAPI NtSetDefaultUILanguage(LANGID);
NTSYSAPI NTSTATUS  WINAPI NtSetEaFile(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtSetEvent(HANDLE,LONG*);
NTSYSAPI NTSTATUS  WINAPI NtSetHighEventPair(HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtSetHighWaitLowEventPair(HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtSetHighWaitLowThread(VOID);
NTSYSAPI NTSTATUS  WINAPI NtSetInformationDebugObject(HANDLE,DEBUGOBJECTINFOCLASS,PVOID,ULONG,ULONG*);
NTSYSAPI NTSTATUS  WINAPI NtSetInformationFile(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,FILE_INFORMATION_CLASS);
NTSYSAPI NTSTATUS  WINAPI NtSetInformationJobObject(HANDLE,JOBOBJECTINFOCLASS,PVOID,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtSetInformationKey(HANDLE,const int,PVOID,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtSetInformationObject(HANDLE, OBJECT_INFORMATION_CLASS, PVOID, ULONG);
NTSYSAPI NTSTATUS  WINAPI NtSetInformationProcess(HANDLE,PROCESSINFOCLASS,PVOID,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtSetInformationThread(HANDLE,THREADINFOCLASS,LPCVOID,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtSetInformationToken(HANDLE,TOKEN_INFORMATION_CLASS,PVOID,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtSetInformationVirtualMemory(HANDLE,VIRTUAL_MEMORY_INFORMATION_CLASS,ULONG_PTR,PMEMORY_RANGE_ENTRY,PVOID,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtSetIntervalProfile(ULONG,KPROFILE_SOURCE);
NTSYSAPI NTSTATUS  WINAPI NtSetIoCompletion(HANDLE,ULONG_PTR,ULONG_PTR,NTSTATUS,SIZE_T);
NTSYSAPI NTSTATUS  WINAPI NtSetIoCompletionEx(HANDLE,HANDLE,ULONG_PTR,ULONG_PTR,NTSTATUS,SIZE_T);
NTSYSAPI NTSTATUS  WINAPI NtSetLdtEntries(ULONG,LDT_ENTRY,ULONG,LDT_ENTRY);
NTSYSAPI NTSTATUS  WINAPI NtSetLowEventPair(HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtSetLowWaitHighEventPair(HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtSetLowWaitHighThread(VOID);
NTSYSAPI NTSTATUS  WINAPI NtSetSecurityObject(HANDLE,SECURITY_INFORMATION,PSECURITY_DESCRIPTOR);
NTSYSAPI NTSTATUS  WINAPI NtSetSystemEnvironmentValue(PUNICODE_STRING,PUNICODE_STRING);
NTSYSAPI NTSTATUS  WINAPI NtSetSystemInformation(SYSTEM_INFORMATION_CLASS,PVOID,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtSetSystemPowerState(POWER_ACTION,SYSTEM_POWER_STATE,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtSetSystemTime(const LARGE_INTEGER*,LARGE_INTEGER*);
NTSYSAPI NTSTATUS  WINAPI NtSetThreadExecutionState(EXECUTION_STATE,EXECUTION_STATE*);
NTSYSAPI NTSTATUS  WINAPI NtSetTimer(HANDLE, const LARGE_INTEGER*, PTIMER_APC_ROUTINE, PVOID, BOOLEAN, ULONG, BOOLEAN*);
NTSYSAPI NTSTATUS  WINAPI NtSetTimerResolution(ULONG,BOOLEAN,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtSetValueKey(HANDLE,const UNICODE_STRING *,ULONG,ULONG,const void *,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtSetVolumeInformationFile(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,FS_INFORMATION_CLASS);
NTSYSAPI NTSTATUS  WINAPI NtSignalAndWaitForSingleObject(HANDLE,HANDLE,BOOLEAN,const LARGE_INTEGER*);
NTSYSAPI NTSTATUS  WINAPI NtShutdownSystem(SHUTDOWN_ACTION);
NTSYSAPI NTSTATUS  WINAPI NtStartProfile(HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtStopProfile(HANDLE);
#ifndef __REACTOS__
NTSYSAPI NTSTATUS  WINAPI NtSuspendProcess(HANDLE);
#endif
NTSYSAPI NTSTATUS  WINAPI NtSuspendThread(HANDLE,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtSystemDebugControl(SYSDBG_COMMAND,PVOID,ULONG,PVOID,ULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtTerminateJobObject(HANDLE,NTSTATUS);
NTSYSAPI NTSTATUS  WINAPI NtTerminateProcess(HANDLE,LONG);
NTSYSAPI NTSTATUS  WINAPI NtTerminateThread(HANDLE,LONG);
NTSYSAPI NTSTATUS  WINAPI NtTestAlert(VOID);
NTSYSAPI NTSTATUS  WINAPI NtTraceControl(ULONG,void*,ULONG,void*,ULONG,ULONG*);
NTSYSAPI NTSTATUS  WINAPI NtUnloadDriver(const UNICODE_STRING *);
NTSYSAPI NTSTATUS  WINAPI NtUnloadKey(POBJECT_ATTRIBUTES);
NTSYSAPI NTSTATUS  WINAPI NtUnloadKeyEx(POBJECT_ATTRIBUTES,HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtUnlockFile(HANDLE,PIO_STATUS_BLOCK,PLARGE_INTEGER,PLARGE_INTEGER,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtUnlockVirtualMemory(HANDLE,PVOID*,SIZE_T*,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtUnmapViewOfSection(HANDLE,PVOID);
NTSYSAPI NTSTATUS  WINAPI NtUnmapViewOfSectionEx(HANDLE,PVOID,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtVdmControl(ULONG,PVOID);
NTSYSAPI NTSTATUS  WINAPI NtWaitForAlertByThreadId(const void*,const LARGE_INTEGER*);
NTSYSAPI NTSTATUS  WINAPI NtWaitForDebugEvent(HANDLE,BOOLEAN,LARGE_INTEGER*,DBGUI_WAIT_STATE_CHANGE*);
NTSYSAPI NTSTATUS  WINAPI NtWaitForKeyedEvent(HANDLE,const void*,BOOLEAN,const LARGE_INTEGER*);
NTSYSAPI NTSTATUS  WINAPI NtWaitForSingleObject(HANDLE,BOOLEAN,const LARGE_INTEGER*);
NTSYSAPI NTSTATUS  WINAPI NtWaitForMultipleObjects(ULONG,const HANDLE*,BOOLEAN,BOOLEAN,const LARGE_INTEGER*);
NTSYSAPI NTSTATUS  WINAPI NtWaitHighEventPair(HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtWaitLowEventPair(HANDLE);
NTSYSAPI NTSTATUS  WINAPI NtWriteFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,const void*,ULONG,PLARGE_INTEGER,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtWriteFileGather(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,FILE_SEGMENT_ELEMENT*,ULONG,PLARGE_INTEGER,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtWriteRequestData(HANDLE,PLPC_MESSAGE,ULONG,PVOID,ULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI NtWriteVirtualMemory(HANDLE,void*,const void*,SIZE_T,SIZE_T*);
NTSYSAPI NTSTATUS  WINAPI NtYieldExecution(void);
NTSYSAPI NTSTATUS  WINAPI RtlAbsoluteToSelfRelativeSD(PSECURITY_DESCRIPTOR,PSECURITY_DESCRIPTOR,PULONG);
NTSYSAPI void      WINAPI RtlAcquirePebLock(void);
NTSYSAPI BYTE      WINAPI RtlAcquireResourceExclusive(LPRTL_RWLOCK,BYTE);
NTSYSAPI BYTE      WINAPI RtlAcquireResourceShared(LPRTL_RWLOCK,BYTE);
NTSYSAPI void      WINAPI RtlAcquireSRWLockExclusive(RTL_SRWLOCK*);
NTSYSAPI void      WINAPI RtlAcquireSRWLockShared(RTL_SRWLOCK*);
NTSYSAPI NTSTATUS  WINAPI RtlActivateActivationContext(DWORD,HANDLE,ULONG_PTR*);
NTSYSAPI NTSTATUS  WINAPI RtlActivateActivationContextEx(ULONG,TEB*,HANDLE,ULONG_PTR*);
NTSYSAPI NTSTATUS  WINAPI RtlAddAccessAllowedAce(PACL,DWORD,DWORD,PSID);
NTSYSAPI NTSTATUS  WINAPI RtlAddAccessAllowedAceEx(PACL,DWORD,DWORD,DWORD,PSID);
NTSYSAPI NTSTATUS  WINAPI RtlAddAccessAllowedObjectAce(PACL,DWORD,DWORD,DWORD,GUID*,GUID*,PSID);
NTSYSAPI NTSTATUS  WINAPI RtlAddAccessDeniedAce(PACL,DWORD,DWORD,PSID);
NTSYSAPI NTSTATUS  WINAPI RtlAddAccessDeniedAceEx(PACL,DWORD,DWORD,DWORD,PSID);
NTSYSAPI NTSTATUS  WINAPI RtlAddAccessDeniedObjectAce(PACL,DWORD,DWORD,DWORD,GUID*,GUID*,PSID);
NTSYSAPI NTSTATUS  WINAPI RtlAddAce(PACL,DWORD,DWORD,PACE_HEADER,DWORD);
NTSYSAPI NTSTATUS  WINAPI RtlAddAtomToAtomTable(RTL_ATOM_TABLE,const WCHAR*,RTL_ATOM*);
NTSYSAPI NTSTATUS  WINAPI RtlAddAuditAccessAce(PACL,DWORD,DWORD,PSID,BOOL,BOOL);
NTSYSAPI NTSTATUS  WINAPI RtlAddAuditAccessAceEx(PACL,DWORD,DWORD,DWORD,PSID,BOOL,BOOL);
NTSYSAPI NTSTATUS  WINAPI RtlAddAuditAccessObjectAce(PACL,DWORD,DWORD,DWORD,GUID*,GUID*,PSID,BOOL,BOOL);
NTSYSAPI NTSTATUS  WINAPI RtlAddMandatoryAce(PACL,DWORD,DWORD,DWORD,DWORD,PSID);
NTSYSAPI NTSTATUS  WINAPI RtlAddProcessTrustLabelAce(PACL,DWORD,DWORD,PSID,DWORD,DWORD);
NTSYSAPI void      WINAPI RtlAddRefActivationContext(HANDLE);
NTSYSAPI PVOID     WINAPI RtlAddVectoredContinueHandler(ULONG,PVECTORED_EXCEPTION_HANDLER);
NTSYSAPI PVOID     WINAPI RtlAddVectoredExceptionHandler(ULONG,PVECTORED_EXCEPTION_HANDLER);
NTSYSAPI PVOID     WINAPI RtlAddressInSectionTable(const IMAGE_NT_HEADERS*,HMODULE,DWORD);
NTSYSAPI NTSTATUS  WINAPI RtlAdjustPrivilege(ULONG,BOOLEAN,BOOLEAN,PBOOLEAN);
NTSYSAPI NTSTATUS  WINAPI RtlAllocateAndInitializeSid(PSID_IDENTIFIER_AUTHORITY,BYTE,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,PSID *);
NTSYSAPI RTL_HANDLE * WINAPI RtlAllocateHandle(RTL_HANDLE_TABLE *,ULONG *);
NTSYSAPI BOOLEAN   WINAPI RtlFreeHeap(HANDLE,ULONG,PVOID);
NTSYSAPI PVOID     WINAPI RtlAllocateHeap(HANDLE,ULONG,SIZE_T) __WINE_ALLOC_SIZE(3) __WINE_DEALLOC(RtlFreeHeap,3) __WINE_MALLOC;
NTSYSAPI WCHAR     WINAPI RtlAnsiCharToUnicodeChar(LPSTR *);
NTSYSAPI DWORD     WINAPI RtlAnsiStringToUnicodeSize(const STRING *);
NTSYSAPI NTSTATUS  WINAPI RtlAnsiStringToUnicodeString(PUNICODE_STRING,PCANSI_STRING,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI RtlAppendAsciizToString(STRING *,LPCSTR);
NTSYSAPI NTSTATUS  WINAPI RtlAppendStringToString(STRING *,const STRING *);
NTSYSAPI NTSTATUS  WINAPI RtlAppendUnicodeStringToString(UNICODE_STRING *,const UNICODE_STRING *);
NTSYSAPI NTSTATUS  WINAPI RtlAppendUnicodeToString(UNICODE_STRING *,LPCWSTR);
NTSYSAPI BOOLEAN   WINAPI RtlAreAllAccessesGranted(ACCESS_MASK,ACCESS_MASK);
NTSYSAPI BOOLEAN   WINAPI RtlAreAnyAccessesGranted(ACCESS_MASK,ACCESS_MASK);
NTSYSAPI BOOLEAN   WINAPI RtlAreBitsSet(PCRTL_BITMAP,ULONG,ULONG);
NTSYSAPI BOOLEAN   WINAPI RtlAreBitsClear(PCRTL_BITMAP,ULONG,ULONG);
NTSYSAPI NTSTATUS  WINAPI RtlCharToInteger(PCSZ,ULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI RtlCheckRegistryKey(ULONG, PWSTR);
NTSYSAPI void      WINAPI RtlClearAllBits(PRTL_BITMAP);
NTSYSAPI void      WINAPI RtlClearBits(PRTL_BITMAP,ULONG,ULONG);
NTSYSAPI ULONG     WINAPI RtlCompactHeap(HANDLE,ULONG);
NTSYSAPI LONG      WINAPI RtlCompareUnicodeString(const UNICODE_STRING*,const UNICODE_STRING*,BOOLEAN);
NTSYSAPI LONG      WINAPI RtlCompareUnicodeStrings(const WCHAR*,SIZE_T,const WCHAR*,SIZE_T,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI RtlCompressBuffer(USHORT,PUCHAR,ULONG,PUCHAR,ULONG,ULONG,PULONG,PVOID);
NTSYSAPI DWORD     WINAPI RtlComputeCrc32(DWORD,const BYTE*,INT);
NTSYSAPI NTSTATUS  WINAPI RtlConvertSidToUnicodeString(PUNICODE_STRING,PSID,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI RtlConvertToAutoInheritSecurityObject(PSECURITY_DESCRIPTOR,PSECURITY_DESCRIPTOR,PSECURITY_DESCRIPTOR*,GUID*,BOOL,PGENERIC_MAPPING);
NTSYSAPI NTSTATUS  WINAPI RtlCopyContext(CONTEXT*,DWORD,CONTEXT*);
NTSYSAPI NTSTATUS  WINAPI RtlCopyExtendedContext(CONTEXT_EX*,ULONG,CONTEXT_EX*);
NTSYSAPI void      WINAPI RtlCopyLuid(PLUID,const LUID*);
NTSYSAPI void      WINAPI RtlCopyLuidAndAttributesArray(ULONG,const LUID_AND_ATTRIBUTES*,PLUID_AND_ATTRIBUTES);
NTSYSAPI NTSTATUS  WINAPI RtlCopySecurityDescriptor(PSECURITY_DESCRIPTOR,PSECURITY_DESCRIPTOR);
NTSYSAPI BOOLEAN   WINAPI RtlCopySid(DWORD,PSID,PSID);
NTSYSAPI void      WINAPI RtlCopyUnicodeString(UNICODE_STRING*,const UNICODE_STRING*);
NTSYSAPI NTSTATUS  WINAPI RtlCreateAcl(PACL,DWORD,DWORD);
NTSYSAPI NTSTATUS  WINAPI RtlCreateActivationContext(HANDLE*,const void*);
NTSYSAPI NTSTATUS  WINAPI RtlCreateAtomTable(ULONG,RTL_ATOM_TABLE*);
NTSYSAPI NTSTATUS  WINAPI RtlCreateEnvironment(BOOLEAN, PWSTR*);
NTSYSAPI HANDLE    WINAPI RtlCreateHeap(ULONG,PVOID,SIZE_T,SIZE_T,PVOID,PRTL_HEAP_PARAMETERS);
NTSYSAPI NTSTATUS  WINAPI RtlCreateProcessParameters(RTL_USER_PROCESS_PARAMETERS**,const UNICODE_STRING*,const UNICODE_STRING*,const UNICODE_STRING*,const UNICODE_STRING*,PWSTR,const UNICODE_STRING*,const UNICODE_STRING*,const UNICODE_STRING*,const UNICODE_STRING*);
NTSYSAPI NTSTATUS  WINAPI RtlCreateProcessParametersEx(RTL_USER_PROCESS_PARAMETERS**,const UNICODE_STRING*,const UNICODE_STRING*,const UNICODE_STRING*,const UNICODE_STRING*,PWSTR,const UNICODE_STRING*,const UNICODE_STRING*,const UNICODE_STRING*,const UNICODE_STRING*,ULONG);
NTSYSAPI PDEBUG_BUFFER WINAPI RtlCreateQueryDebugBuffer(ULONG,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI RtlCreateRegistryKey(ULONG,PWSTR);
NTSYSAPI NTSTATUS  WINAPI RtlCreateSecurityDescriptor(PSECURITY_DESCRIPTOR,DWORD);
NTSYSAPI NTSTATUS  WINAPI RtlCreateTimer(HANDLE,HANDLE*,RTL_WAITORTIMERCALLBACKFUNC, PVOID, DWORD, DWORD, ULONG);
NTSYSAPI NTSTATUS  WINAPI RtlCreateTimerQueue(PHANDLE);
NTSYSAPI BOOLEAN   WINAPI RtlCreateUnicodeString(PUNICODE_STRING,LPCWSTR);
NTSYSAPI BOOLEAN   WINAPI RtlCreateUnicodeStringFromAsciiz(PUNICODE_STRING,LPCSTR);
NTSYSAPI NTSTATUS  WINAPI RtlCreateUserProcess(UNICODE_STRING*,ULONG,RTL_USER_PROCESS_PARAMETERS*,SECURITY_DESCRIPTOR*,SECURITY_DESCRIPTOR*,HANDLE,BOOLEAN,HANDLE,HANDLE,RTL_USER_PROCESS_INFORMATION*);
NTSYSAPI NTSTATUS  WINAPI RtlCreateUserStack(SIZE_T,SIZE_T,ULONG,SIZE_T,SIZE_T,INITIAL_TEB*);
NTSYSAPI NTSTATUS  WINAPI RtlCreateUserThread(HANDLE,SECURITY_DESCRIPTOR*,BOOLEAN,ULONG,SIZE_T,SIZE_T,PRTL_THREAD_START_ROUTINE,void*,HANDLE*,CLIENT_ID*);
NTSYSAPI NTSTATUS  WINAPI RtlCustomCPToUnicodeN(CPTABLEINFO*,WCHAR*,DWORD,DWORD*,const char*,DWORD);
NTSYSAPI PRTL_USER_PROCESS_PARAMETERS WINAPI RtlDeNormalizeProcessParams(RTL_USER_PROCESS_PARAMETERS*);
NTSYSAPI void      WINAPI RtlDeactivateActivationContext(DWORD,ULONG_PTR);
NTSYSAPI PVOID     WINAPI RtlDecodePointer(PVOID);
NTSYSAPI NTSTATUS  WINAPI RtlDecompressBuffer(USHORT,PUCHAR,ULONG,PUCHAR,ULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI RtlDecompressFragment(USHORT,PUCHAR,ULONG,PUCHAR,ULONG,ULONG,PULONG,PVOID);
NTSYSAPI NTSTATUS  WINAPI RtlDefaultNpAcl(PACL*);
NTSYSAPI NTSTATUS  WINAPI RtlDeleteAce(PACL,DWORD);
NTSYSAPI NTSTATUS  WINAPI RtlDeleteAtomFromAtomTable(RTL_ATOM_TABLE,RTL_ATOM);
NTSYSAPI NTSTATUS  WINAPI RtlDeleteCriticalSection(RTL_CRITICAL_SECTION *);
NTSYSAPI NTSTATUS  WINAPI RtlDeleteRegistryValue(ULONG, PCWSTR, PCWSTR);
NTSYSAPI void      WINAPI RtlDeleteResource(LPRTL_RWLOCK);
NTSYSAPI NTSTATUS  WINAPI RtlDeleteSecurityObject(PSECURITY_DESCRIPTOR*);
NTSYSAPI NTSTATUS  WINAPI RtlDeleteTimer(HANDLE, HANDLE, HANDLE);
NTSYSAPI NTSTATUS  WINAPI RtlDeleteTimerQueueEx(HANDLE, HANDLE);
NTSYSAPI PRTL_USER_PROCESS_PARAMETERS WINAPI RtlDeNormalizeProcessParams(RTL_USER_PROCESS_PARAMETERS*);
NTSYSAPI NTSTATUS  WINAPI RtlDeregisterWait(HANDLE);
NTSYSAPI NTSTATUS  WINAPI RtlDeregisterWaitEx(HANDLE,HANDLE);
NTSYSAPI NTSTATUS  WINAPI RtlDestroyAtomTable(RTL_ATOM_TABLE);
NTSYSAPI NTSTATUS  WINAPI RtlDestroyEnvironment(PWSTR);
NTSYSAPI NTSTATUS  WINAPI RtlDestroyHandleTable(RTL_HANDLE_TABLE *);
NTSYSAPI HANDLE    WINAPI RtlDestroyHeap(HANDLE);
NTSYSAPI void      WINAPI RtlDestroyProcessParameters(RTL_USER_PROCESS_PARAMETERS*);
NTSYSAPI NTSTATUS  WINAPI RtlDestroyQueryDebugBuffer(PDEBUG_BUFFER);
NTSYSAPI DOS_PATHNAME_TYPE WINAPI RtlDetermineDosPathNameType_U(PCWSTR);
NTSYSAPI BOOLEAN   WINAPI RtlDllShutdownInProgress(void);
NTSYSAPI BOOLEAN   WINAPI RtlDoesFileExists_U(LPCWSTR);
NTSYSAPI BOOLEAN   WINAPI RtlDosPathNameToNtPathName_U(PCWSTR,PUNICODE_STRING,PWSTR*,CURDIR*);
NTSYSAPI NTSTATUS  WINAPI RtlDosPathNameToNtPathName_U_WithStatus(PCWSTR,PUNICODE_STRING,PWSTR*,CURDIR*);
NTSYSAPI BOOLEAN   WINAPI RtlDosPathNameToRelativeNtPathName_U(PCWSTR,PUNICODE_STRING,PWSTR*,RTL_RELATIVE_NAME*);
NTSYSAPI NTSTATUS  WINAPI RtlDosPathNameToRelativeNtPathName_U_WithStatus(PCWSTR,PUNICODE_STRING,PWSTR*,RTL_RELATIVE_NAME*);
NTSYSAPI ULONG     WINAPI RtlDosSearchPath_U(LPCWSTR, LPCWSTR, LPCWSTR, ULONG, LPWSTR, LPWSTR*);
NTSYSAPI WCHAR     WINAPI RtlDowncaseUnicodeChar(WCHAR);
NTSYSAPI NTSTATUS  WINAPI RtlDowncaseUnicodeString(UNICODE_STRING*,const UNICODE_STRING*,BOOLEAN);
NTSYSAPI void      WINAPI RtlDumpResource(LPRTL_RWLOCK);
NTSYSAPI NTSTATUS  WINAPI RtlDuplicateUnicodeString(int,const UNICODE_STRING*,UNICODE_STRING*);
NTSYSAPI NTSTATUS  WINAPI RtlEmptyAtomTable(RTL_ATOM_TABLE,BOOLEAN);
NTSYSAPI PVOID     WINAPI RtlEncodePointer(PVOID);
NTSYSAPI NTSTATUS  WINAPI RtlEnterCriticalSection(RTL_CRITICAL_SECTION *);
NTSYSAPI void      WINAPI RtlEraseUnicodeString(UNICODE_STRING*);
NTSYSAPI NTSTATUS  WINAPI RtlEqualComputerName(const UNICODE_STRING*,const UNICODE_STRING*);
NTSYSAPI NTSTATUS  WINAPI RtlEqualDomainName(const UNICODE_STRING*,const UNICODE_STRING*);
NTSYSAPI BOOLEAN   WINAPI RtlEqualLuid(const LUID*,const LUID*);
NTSYSAPI BOOL      WINAPI RtlEqualPrefixSid(PSID,PSID);
NTSYSAPI BOOL      WINAPI RtlEqualSid(PSID,PSID);
NTSYSAPI BOOLEAN   WINAPI RtlEqualUnicodeString(const UNICODE_STRING*,const UNICODE_STRING*,BOOLEAN);
NTSYSAPI void      WINAPI RtlEraseUnicodeString(UNICODE_STRING*);
NTSYSAPI void      DECLSPEC_NORETURN WINAPI RtlExitUserProcess(ULONG);
NTSYSAPI void      DECLSPEC_NORETURN WINAPI RtlExitUserThread(ULONG);
NTSYSAPI NTSTATUS  WINAPI RtlExpandEnvironmentStrings(const WCHAR*,WCHAR*,SIZE_T,WCHAR*,SIZE_T,SIZE_T*);
NTSYSAPI NTSTATUS  WINAPI RtlExpandEnvironmentStrings_U(PCWSTR, const UNICODE_STRING*, UNICODE_STRING*, ULONG*);
NTSYSAPI NTSTATUS  WINAPI RtlFindActivationContextSectionString(ULONG,const GUID*,ULONG,const UNICODE_STRING*,PVOID);
NTSYSAPI NTSTATUS  WINAPI RtlFindActivationContextSectionGuid(ULONG,const GUID*,ULONG,const GUID*,PVOID);
NTSYSAPI NTSTATUS  WINAPI RtlFindCharInUnicodeString(int,const UNICODE_STRING*,const UNICODE_STRING*,USHORT*);
NTSYSAPI ULONG     WINAPI RtlFindClearBits(PCRTL_BITMAP,ULONG,ULONG);
NTSYSAPI ULONG     WINAPI RtlFindClearBitsAndSet(PRTL_BITMAP,ULONG,ULONG);
NTSYSAPI ULONG     WINAPI RtlFindClearRuns(PCRTL_BITMAP,PRTL_BITMAP_RUN,ULONG,BOOLEAN);
NTSYSAPI void *    WINAPI RtlFindExportedRoutineByName(HMODULE,const char*);
NTSYSAPI ULONG     WINAPI RtlFindLastBackwardRunSet(PCRTL_BITMAP,ULONG,PULONG);
NTSYSAPI ULONG     WINAPI RtlFindLastBackwardRunClear(PCRTL_BITMAP,ULONG,PULONG);
NTSYSAPI CCHAR     WINAPI RtlFindLeastSignificantBit(ULONGLONG);
NTSYSAPI ULONG     WINAPI RtlFindLongestRunSet(PCRTL_BITMAP,PULONG);
NTSYSAPI ULONG     WINAPI RtlFindLongestRunClear(PCRTL_BITMAP,PULONG);
NTSYSAPI NTSTATUS  WINAPI RtlFindMessage(HMODULE,ULONG,ULONG,ULONG,const MESSAGE_RESOURCE_ENTRY**);
NTSYSAPI CCHAR     WINAPI RtlFindMostSignificantBit(ULONGLONG);
NTSYSAPI ULONG     WINAPI RtlFindNextForwardRunSet(PCRTL_BITMAP,ULONG,PULONG);
NTSYSAPI ULONG     WINAPI RtlFindNextForwardRunClear(PCRTL_BITMAP,ULONG,PULONG);
NTSYSAPI ULONG     WINAPI RtlFindSetBits(PCRTL_BITMAP,ULONG,ULONG);
NTSYSAPI ULONG     WINAPI RtlFindSetBitsAndClear(PRTL_BITMAP,ULONG,ULONG);
NTSYSAPI ULONG     WINAPI RtlFindSetRuns(PCRTL_BITMAP,PRTL_BITMAP_RUN,ULONG,BOOLEAN);
NTSYSAPI BOOLEAN   WINAPI RtlFirstFreeAce(PACL,PACE_HEADER *);
NTSYSAPI NTSTATUS  WINAPI RtlFlsAlloc(PFLS_CALLBACK_FUNCTION,ULONG *);
NTSYSAPI NTSTATUS  WINAPI RtlFlsFree(ULONG);
NTSYSAPI NTSTATUS  WINAPI RtlFlsGetValue(ULONG,void **);
NTSYSAPI NTSTATUS  WINAPI RtlFlsSetValue(ULONG,void *);
NTSYSAPI NTSTATUS  WINAPI RtlFormatCurrentUserKeyPath(PUNICODE_STRING);
#ifdef __ms_va_list
NTSYSAPI NTSTATUS  WINAPI RtlFormatMessage(LPCWSTR,ULONG,BOOLEAN,BOOLEAN,BOOLEAN,__ms_va_list *,LPWSTR,ULONG,ULONG*);
NTSYSAPI NTSTATUS  WINAPI RtlFormatMessageEx(LPCWSTR,ULONG,BOOLEAN,BOOLEAN,BOOLEAN,__ms_va_list *,LPWSTR,ULONG,ULONG*,ULONG);
#endif
NTSYSAPI void      WINAPI RtlFreeActivationContextStack(ACTIVATION_CONTEXT_STACK *);
NTSYSAPI void      WINAPI RtlFreeAnsiString(PANSI_STRING);
NTSYSAPI BOOLEAN   WINAPI RtlFreeHandle(RTL_HANDLE_TABLE *,RTL_HANDLE *);
NTSYSAPI void      WINAPI RtlFreeOemString(POEM_STRING);
NTSYSAPI DWORD     WINAPI RtlFreeSid(PSID);
NTSYSAPI void      WINAPI RtlFreeThreadActivationContextStack(void);
NTSYSAPI void      WINAPI RtlFreeUnicodeString(PUNICODE_STRING);
NTSYSAPI void      WINAPI RtlFreeUserStack(void*);
NTSYSAPI NTSTATUS  WINAPI RtlGUIDFromString(PUNICODE_STRING,GUID*);
NTSYSAPI NTSTATUS  WINAPI RtlGetAce(PACL,DWORD,LPVOID *);
NTSYSAPI NTSTATUS  WINAPI RtlGetActiveActivationContext(HANDLE*);
NTSYSAPI NTSTATUS  WINAPI RtlGetCompressionWorkSpaceSize(USHORT,PULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI RtlGetControlSecurityDescriptor(PSECURITY_DESCRIPTOR, PSECURITY_DESCRIPTOR_CONTROL,LPDWORD);
NTSYSAPI ULONG     WINAPI RtlGetCurrentDirectory_U(ULONG, LPWSTR);
NTSYSAPI PEB *     WINAPI RtlGetCurrentPeb(void);
NTSYSAPI void      WINAPI RtlGetCurrentProcessorNumberEx(PROCESSOR_NUMBER*);
NTSYSAPI HANDLE    WINAPI RtlGetCurrentTransaction(void);
NTSYSAPI NTSTATUS  WINAPI RtlGetDaclSecurityDescriptor(PSECURITY_DESCRIPTOR,PBOOLEAN,PACL *,PBOOLEAN);
NTSYSAPI ULONG64   WINAPI RtlGetEnabledExtendedFeatures(ULONG64);
NTSYSAPI NTSTATUS  WINAPI RtlGetExePath(PCWSTR,PWSTR*);
NTSYSAPI NTSTATUS  WINAPI RtlGetExtendedContextLength(ULONG,ULONG*);
NTSYSAPI NTSTATUS  WINAPI RtlGetExtendedContextLength2(ULONG,ULONG*,ULONG64);
NTSYSAPI ULONG64   WINAPI RtlGetExtendedFeaturesMask(CONTEXT_EX*);
NTSYSAPI TEB_ACTIVE_FRAME * WINAPI RtlGetFrame(void);
NTSYSAPI ULONG     WINAPI RtlGetFullPathName_U(PCWSTR,ULONG,PWSTR,PWSTR*);
NTSYSAPI NTSTATUS  WINAPI RtlGetGroupSecurityDescriptor(PSECURITY_DESCRIPTOR,PSID *,PBOOLEAN);
NTSYSAPI NTSTATUS  WINAPI RtlGetLastNtStatus(void);
NTSYSAPI DWORD     WINAPI RtlGetLastWin32Error(void);
NTSYSAPI NTSTATUS  WINAPI RtlGetLocaleFileMappingAddress(void**,LCID*,LARGE_INTEGER*);
NTSYSAPI DWORD     WINAPI RtlGetLongestNtPathLength(void);
NTSYSAPI NTSTATUS  WINAPI RtlGetNativeSystemInformation(SYSTEM_INFORMATION_CLASS,void*,ULONG,ULONG*);
NTSYSAPI ULONG     WINAPI RtlGetNtGlobalFlags(void);
NTSYSAPI BOOLEAN   WINAPI RtlGetNtProductType(LPDWORD);
NTSYSAPI void      WINAPI RtlGetNtVersionNumbers(LPDWORD,LPDWORD,LPDWORD);
NTSYSAPI NTSTATUS  WINAPI RtlGetOwnerSecurityDescriptor(PSECURITY_DESCRIPTOR,PSID *,PBOOLEAN);
NTSYSAPI ULONG     WINAPI RtlGetProcessHeaps(ULONG,HANDLE*);
NTSYSAPI NTSTATUS  WINAPI RtlGetProcessPreferredUILanguages(DWORD,ULONG*,WCHAR*,ULONG*);
NTSYSAPI BOOLEAN   WINAPI RtlGetProductInfo(DWORD,DWORD,DWORD,DWORD,PDWORD);
NTSYSAPI NTSTATUS  WINAPI RtlGetSaclSecurityDescriptor(PSECURITY_DESCRIPTOR,PBOOLEAN,PACL *,PBOOLEAN);
NTSYSAPI NTSTATUS  WINAPI RtlGetSearchPath(PWSTR*);
NTSYSAPI NTSTATUS  WINAPI RtlGetSystemPreferredUILanguages(DWORD,ULONG,ULONG*,WCHAR*,ULONG*);
NTSYSAPI LONGLONG  WINAPI RtlGetSystemTimePrecise(void);
NTSYSAPI DWORD     WINAPI RtlGetThreadErrorMode(void);
NTSYSAPI NTSTATUS  WINAPI RtlGetThreadPreferredUILanguages(DWORD,ULONG*,WCHAR*,ULONG*);
NTSYSAPI BOOLEAN   WINAPI RtlGetUserInfoHeap(HANDLE,ULONG,void*,void**,ULONG*);
NTSYSAPI NTSTATUS  WINAPI RtlGetUserPreferredUILanguages(DWORD,ULONG,ULONG*,WCHAR*,ULONG*);
NTSYSAPI NTSTATUS  WINAPI RtlGetVersion(RTL_OSVERSIONINFOEXW*);
NTSYSAPI NTSTATUS  WINAPI RtlHashUnicodeString(const UNICODE_STRING*,BOOLEAN,ULONG,ULONG*);
NTSYSAPI PSID_IDENTIFIER_AUTHORITY WINAPI RtlIdentifierAuthoritySid(PSID);
NTSYSAPI NTSTATUS  WINAPI RtlIdnToAscii(DWORD,const WCHAR*,INT,WCHAR*,INT*);
NTSYSAPI NTSTATUS  WINAPI RtlIdnToNameprepUnicode(DWORD,const WCHAR*,INT,WCHAR*,INT*);
NTSYSAPI NTSTATUS  WINAPI RtlIdnToUnicode(DWORD,const WCHAR*,INT,WCHAR*,INT*);
NTSYSAPI PVOID     WINAPI RtlImageDirectoryEntryToData(HMODULE,BOOL,WORD,ULONG *);
NTSYSAPI PIMAGE_NT_HEADERS WINAPI RtlImageNtHeader(HMODULE);
NTSYSAPI PIMAGE_SECTION_HEADER WINAPI RtlImageRvaToSection(const IMAGE_NT_HEADERS *,HMODULE,DWORD);
NTSYSAPI PVOID     WINAPI RtlImageRvaToVa(const IMAGE_NT_HEADERS *,HMODULE,DWORD,IMAGE_SECTION_HEADER **);
NTSYSAPI NTSTATUS  WINAPI RtlImpersonateSelf(SECURITY_IMPERSONATION_LEVEL);
NTSYSAPI void      WINAPI RtlInitAnsiString(PANSI_STRING,PCSZ);
NTSYSAPI NTSTATUS  WINAPI RtlInitAnsiStringEx(PANSI_STRING,PCSZ);
NTSYSAPI void      WINAPI RtlInitCodePageTable(USHORT*,CPTABLEINFO*);
NTSYSAPI void      WINAPI RtlInitNlsTables(USHORT*,USHORT*,USHORT*,NLSTABLEINFO*);
NTSYSAPI void      WINAPI RtlInitString(PSTRING,PCSZ);
NTSYSAPI void      WINAPI RtlInitUnicodeString(PUNICODE_STRING,PCWSTR);
NTSYSAPI NTSTATUS  WINAPI RtlInitUnicodeStringEx(PUNICODE_STRING,PCWSTR);
NTSYSAPI void      WINAPI RtlInitializeBitMap(PRTL_BITMAP,PULONG,ULONG);
NTSYSAPI void      WINAPI RtlInitializeConditionVariable(RTL_CONDITION_VARIABLE *);
NTSYSAPI NTSTATUS  WINAPI RtlInitializeCriticalSection(RTL_CRITICAL_SECTION *);
NTSYSAPI NTSTATUS  WINAPI RtlInitializeCriticalSectionAndSpinCount(RTL_CRITICAL_SECTION *,ULONG);
NTSYSAPI NTSTATUS  WINAPI RtlInitializeCriticalSectionEx(RTL_CRITICAL_SECTION *,ULONG,ULONG);
NTSYSAPI NTSTATUS  WINAPI RtlInitializeExtendedContext(void*,ULONG,CONTEXT_EX**);
NTSYSAPI NTSTATUS  WINAPI RtlInitializeExtendedContext2(void*,ULONG,CONTEXT_EX**,ULONG64);
NTSYSAPI void      WINAPI RtlInitializeHandleTable(ULONG,ULONG,RTL_HANDLE_TABLE *);
NTSYSAPI NTSTATUS  WINAPI RtlInitializeNtUserPfn(const void*,ULONG,const void*,ULONG,const void*,ULONG);
NTSYSAPI void      WINAPI RtlInitializeResource(LPRTL_RWLOCK);
NTSYSAPI void      WINAPI RtlInitializeSRWLock(RTL_SRWLOCK*);
NTSYSAPI NTSTATUS  WINAPI RtlInitializeSid(PSID,PSID_IDENTIFIER_AUTHORITY,BYTE);
NTSYSAPI NTSTATUS  WINAPI RtlInt64ToUnicodeString(ULONGLONG,ULONG,UNICODE_STRING *);
NTSYSAPI NTSTATUS  WINAPI RtlIntegerToChar(ULONG,ULONG,ULONG,PCHAR);
NTSYSAPI NTSTATUS  WINAPI RtlIntegerToUnicodeString(ULONG,ULONG,UNICODE_STRING *);
NTSYSAPI BOOLEAN   WINAPI RtlIsActivationContextActive(HANDLE);
NTSYSAPI BOOL      WINAPI RtlIsCriticalSectionLocked(RTL_CRITICAL_SECTION *);
NTSYSAPI BOOL      WINAPI RtlIsCriticalSectionLockedByThread(RTL_CRITICAL_SECTION *);
NTSYSAPI BOOLEAN   WINAPI RtlIsCurrentProcess(HANDLE);
NTSYSAPI BOOLEAN   WINAPI RtlIsCurrentThread(HANDLE);
NTSYSAPI ULONG     WINAPI RtlIsDosDeviceName_U(PCWSTR);
NTSYSAPI BOOLEAN   WINAPI RtlIsNameLegalDOS8Dot3(const UNICODE_STRING*,POEM_STRING,PBOOLEAN);
NTSYSAPI NTSTATUS  WINAPI RtlIsNormalizedString(ULONG,const WCHAR*,INT,BOOLEAN*);
NTSYSAPI BOOLEAN   WINAPI RtlIsProcessorFeaturePresent(UINT);
NTSYSAPI BOOLEAN   WINAPI RtlIsTextUnicode(LPCVOID,INT,INT *);
NTSYSAPI BOOLEAN   WINAPI RtlIsValidHandle(const RTL_HANDLE_TABLE *, const RTL_HANDLE *);
NTSYSAPI BOOLEAN   WINAPI RtlIsValidIndexHandle(const RTL_HANDLE_TABLE *, ULONG Index, RTL_HANDLE **);
NTSYSAPI BOOLEAN   WINAPI RtlIsValidLocaleName(const WCHAR*,ULONG);
NTSYSAPI NTSTATUS  WINAPI RtlLcidToLocaleName(LCID,UNICODE_STRING*,ULONG,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI RtlLeaveCriticalSection(RTL_CRITICAL_SECTION *);
NTSYSAPI DWORD     WINAPI RtlLengthRequiredSid(DWORD);
NTSYSAPI ULONG     WINAPI RtlLengthSecurityDescriptor(PSECURITY_DESCRIPTOR);
NTSYSAPI DWORD     WINAPI RtlLengthSid(PSID);
NTSYSAPI NTSTATUS  WINAPI RtlLocalTimeToSystemTime(const LARGE_INTEGER*,PLARGE_INTEGER);
NTSYSAPI NTSTATUS  WINAPI RtlLocaleNameToLcid(const WCHAR*,LCID*,ULONG);
NTSYSAPI void *    WINAPI RtlLocateExtendedFeature(CONTEXT_EX*,ULONG,ULONG*);
NTSYSAPI void *    WINAPI RtlLocateExtendedFeature2(CONTEXT_EX*,ULONG,XSTATE_CONFIGURATION*,ULONG*);
NTSYSAPI void *    WINAPI RtlLocateLegacyContext(CONTEXT_EX*,ULONG*);
NTSYSAPI BOOLEAN   WINAPI RtlLockHeap(HANDLE);
NTSYSAPI NTSTATUS  WINAPI RtlLookupAtomInAtomTable(RTL_ATOM_TABLE,const WCHAR*,RTL_ATOM*);
NTSYSAPI NTSTATUS  WINAPI RtlMakeSelfRelativeSD(PSECURITY_DESCRIPTOR,PSECURITY_DESCRIPTOR,LPDWORD);
NTSYSAPI NTSTATUS  WINAPI RtlMultiByteToUnicodeN(LPWSTR,DWORD,LPDWORD,LPCSTR,DWORD);
NTSYSAPI NTSTATUS  WINAPI RtlMultiByteToUnicodeSize(DWORD*,LPCSTR,ULONG);
NTSYSAPI NTSTATUS  WINAPI RtlNewSecurityObject(PSECURITY_DESCRIPTOR,PSECURITY_DESCRIPTOR,PSECURITY_DESCRIPTOR*,BOOLEAN,HANDLE,PGENERIC_MAPPING);
NTSYSAPI NTSTATUS  WINAPI RtlNewSecurityObjectEx(PSECURITY_DESCRIPTOR,PSECURITY_DESCRIPTOR,PSECURITY_DESCRIPTOR*,GUID*,BOOLEAN,ULONG,HANDLE,PGENERIC_MAPPING);
NTSYSAPI NTSTATUS  WINAPI RtlNewSecurityObjectWithMultipleInheritance(PSECURITY_DESCRIPTOR,PSECURITY_DESCRIPTOR,PSECURITY_DESCRIPTOR*,
    GUID **,ULONG,BOOLEAN,ULONG,HANDLE,PGENERIC_MAPPING);
NTSYSAPI PRTL_USER_PROCESS_PARAMETERS WINAPI RtlNormalizeProcessParams(RTL_USER_PROCESS_PARAMETERS*);
NTSYSAPI NTSTATUS  WINAPI RtlNormalizeString(ULONG,const WCHAR*,INT,WCHAR*,INT*);
NTSYSAPI ULONG     WINAPI RtlNtStatusToDosError(NTSTATUS);
NTSYSAPI ULONG     WINAPI RtlNtStatusToDosErrorNoTeb(NTSTATUS);
NTSYSAPI ULONG     WINAPI RtlNumberOfSetBits(PCRTL_BITMAP);
NTSYSAPI ULONG     WINAPI RtlNumberOfClearBits(PCRTL_BITMAP);
NTSYSAPI ULONG     WINAPI RtlOemStringToUnicodeSize(const STRING*);
NTSYSAPI NTSTATUS  WINAPI RtlOemStringToUnicodeString(UNICODE_STRING*,const STRING*,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI RtlOemToUnicodeN(LPWSTR,DWORD,LPDWORD,LPCSTR,DWORD);
NTSYSAPI NTSTATUS  WINAPI RtlOpenCurrentUser(ACCESS_MASK,PHANDLE);
NTSYSAPI NTSTATUS  WINAPI RtlPinAtomInAtomTable(RTL_ATOM_TABLE,RTL_ATOM);
NTSYSAPI void      WINAPI RtlPopFrame(TEB_ACTIVE_FRAME*);
NTSYSAPI BOOLEAN   WINAPI RtlPrefixString(const STRING*,const STRING*,BOOLEAN);
NTSYSAPI void      WINAPI RtlProcessFlsData(void*,ULONG);
NTSYSAPI void      WINAPI RtlPushFrame(TEB_ACTIVE_FRAME*);
NTSYSAPI NTSTATUS  WINAPI RtlQueryActivationContextApplicationSettings(DWORD,HANDLE,const WCHAR*,const WCHAR*,WCHAR*,SIZE_T,SIZE_T*);
NTSYSAPI NTSTATUS  WINAPI RtlQueryAtomInAtomTable(RTL_ATOM_TABLE,RTL_ATOM,ULONG*,ULONG*,WCHAR*,ULONG*);
NTSYSAPI NTSTATUS  WINAPI RtlQueryDynamicTimeZoneInformation(RTL_DYNAMIC_TIME_ZONE_INFORMATION*);
NTSYSAPI NTSTATUS  WINAPI RtlQueryEnvironmentVariable(WCHAR*,const WCHAR*,SIZE_T,WCHAR*,SIZE_T,SIZE_T*);
NTSYSAPI NTSTATUS  WINAPI RtlQueryEnvironmentVariable_U(PWSTR,PUNICODE_STRING,PUNICODE_STRING);
NTSYSAPI NTSTATUS  WINAPI RtlQueryHeapInformation(HANDLE,HEAP_INFORMATION_CLASS,PVOID,SIZE_T,PSIZE_T);
NTSYSAPI NTSTATUS  WINAPI RtlQueryInformationAcl(PACL,LPVOID,DWORD,ACL_INFORMATION_CLASS);
NTSYSAPI NTSTATUS  WINAPI RtlQueryInformationActivationContext(ULONG,HANDLE,PVOID,ULONG,PVOID,SIZE_T,SIZE_T*);
NTSYSAPI NTSTATUS  WINAPI RtlQueryPackageIdentity(HANDLE,WCHAR*,SIZE_T*,WCHAR*,SIZE_T*,BOOLEAN*);
NTSYSAPI BOOL      WINAPI RtlQueryPerformanceCounter(LARGE_INTEGER*);
NTSYSAPI BOOL      WINAPI RtlQueryPerformanceFrequency(LARGE_INTEGER*);
NTSYSAPI NTSTATUS  WINAPI RtlQueryProcessDebugInformation(ULONG,ULONG,PDEBUG_BUFFER);
NTSYSAPI NTSTATUS  WINAPI RtlQueryRegistryValues(ULONG, PCWSTR, PRTL_QUERY_REGISTRY_TABLE, PVOID, PVOID);
NTSYSAPI NTSTATUS  WINAPI RtlQueryTimeZoneInformation(RTL_TIME_ZONE_INFORMATION*);
NTSYSAPI BOOL      WINAPI RtlQueryUnbiasedInterruptTime(ULONGLONG*);
NTSYSAPI NTSTATUS  WINAPI RtlQueueWorkItem(PRTL_WORK_ITEM_ROUTINE,PVOID,ULONG);
NTSYSAPI void      DECLSPEC_NORETURN WINAPI RtlRaiseStatus(NTSTATUS);
NTSYSAPI ULONG     WINAPI RtlRandom(PULONG);
NTSYSAPI ULONG     WINAPI RtlRandomEx(PULONG);
NTSYSAPI void      WINAPI RtlRbInsertNodeEx(RTL_RB_TREE*,RTL_BALANCED_NODE*,BOOLEAN,RTL_BALANCED_NODE*);
NTSYSAPI void      WINAPI RtlRbRemoveNode(RTL_RB_TREE*,RTL_BALANCED_NODE*);
NTSYSAPI PVOID     WINAPI RtlReAllocateHeap(HANDLE,ULONG,PVOID,SIZE_T) __WINE_ALLOC_SIZE(4) __WINE_DEALLOC(RtlFreeHeap,3);
NTSYSAPI NTSTATUS  WINAPI RtlRegisterWait(PHANDLE,HANDLE,RTL_WAITORTIMERCALLBACKFUNC,PVOID,ULONG,ULONG);
NTSYSAPI void      WINAPI RtlReleaseActivationContext(HANDLE);
NTSYSAPI void      WINAPI RtlReleasePath(PWSTR);
NTSYSAPI void      WINAPI RtlReleasePebLock(void);
NTSYSAPI void      WINAPI RtlReleaseRelativeName(RTL_RELATIVE_NAME*);
NTSYSAPI void      WINAPI RtlReleaseResource(LPRTL_RWLOCK);
NTSYSAPI void      WINAPI RtlReleaseSRWLockExclusive(RTL_SRWLOCK*);
NTSYSAPI void      WINAPI RtlReleaseSRWLockShared(RTL_SRWLOCK*);
NTSYSAPI ULONG     WINAPI RtlRemoveVectoredContinueHandler(PVOID);
NTSYSAPI ULONG     WINAPI RtlRemoveVectoredExceptionHandler(PVOID);
NTSYSAPI NTSTATUS  WINAPI RtlRetrieveNtUserPfn(const void**,const void**,const void**);
NTSYSAPI NTSTATUS  WINAPI RtlResetNtUserPfn(void);
NTSYSAPI void      WINAPI RtlResetRtlTranslations(const NLSTABLEINFO*);
NTSYSAPI void      WINAPI RtlRestoreLastWin32Error(DWORD);
NTSYSAPI void      WINAPI RtlSecondsSince1970ToTime(DWORD,LARGE_INTEGER *);
NTSYSAPI void      WINAPI RtlSecondsSince1980ToTime(DWORD,LARGE_INTEGER *);
NTSYSAPI NTSTATUS  WINAPI RtlSelfRelativeToAbsoluteSD(PSECURITY_DESCRIPTOR,PSECURITY_DESCRIPTOR,PDWORD,PACL,PDWORD,PACL,PDWORD,PSID,PDWORD,PSID,PDWORD);
NTSYSAPI void      WINAPI RtlSetAllBits(PRTL_BITMAP);
NTSYSAPI void      WINAPI RtlSetBits(PRTL_BITMAP,ULONG,ULONG);
NTSYSAPI NTSTATUS  WINAPI RtlSetControlSecurityDescriptor(PSECURITY_DESCRIPTOR,SECURITY_DESCRIPTOR_CONTROL,SECURITY_DESCRIPTOR_CONTROL);
NTSYSAPI ULONG     WINAPI RtlSetCriticalSectionSpinCount(RTL_CRITICAL_SECTION*,ULONG);
NTSYSAPI NTSTATUS  WINAPI RtlSetCurrentDirectory_U(const UNICODE_STRING*);
NTSYSAPI void      WINAPI RtlSetCurrentEnvironment(PWSTR, PWSTR*);
NTSYSAPI BOOL      WINAPI RtlSetCurrentTransaction(HANDLE);
NTSYSAPI NTSTATUS  WINAPI RtlSetDaclSecurityDescriptor(PSECURITY_DESCRIPTOR,BOOLEAN,PACL,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI RtlSetEnvironmentVariable(PWSTR*,PUNICODE_STRING,PUNICODE_STRING);
NTSYSAPI void      WINAPI RtlSetExtendedFeaturesMask(CONTEXT_EX*,ULONG64);
NTSYSAPI NTSTATUS  WINAPI RtlSetGroupSecurityDescriptor(PSECURITY_DESCRIPTOR,PSID,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI RtlSetHeapInformation(HANDLE,HEAP_INFORMATION_CLASS,PVOID,SIZE_T);
NTSYSAPI NTSTATUS  WINAPI RtlSetIoCompletionCallback(HANDLE,PRTL_OVERLAPPED_COMPLETION_ROUTINE,ULONG);
NTSYSAPI void      WINAPI RtlSetLastWin32Error(DWORD);
NTSYSAPI void      WINAPI RtlSetLastWin32ErrorAndNtStatusFromNtStatus(NTSTATUS);
NTSYSAPI NTSTATUS  WINAPI RtlSetOwnerSecurityDescriptor(PSECURITY_DESCRIPTOR,PSID,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI RtlSetProcessPreferredUILanguages(DWORD,PCZZWSTR,ULONG*);
NTSYSAPI NTSTATUS  WINAPI RtlSetSaclSecurityDescriptor(PSECURITY_DESCRIPTOR,BOOLEAN,PACL,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI RtlSetSearchPathMode(ULONG);
NTSYSAPI NTSTATUS  WINAPI RtlSetThreadErrorMode(DWORD,LPDWORD);
NTSYSAPI NTSTATUS  WINAPI RtlSetThreadPreferredUILanguages(DWORD,PCZZWSTR,ULONG*);
NTSYSAPI NTSTATUS  WINAPI RtlSetTimeZoneInformation(const RTL_TIME_ZONE_INFORMATION*);
NTSYSAPI void      WINAPI RtlSetUnhandledExceptionFilter(PRTL_EXCEPTION_FILTER);
NTSYSAPI BOOLEAN   WINAPI RtlSetUserFlagsHeap(HANDLE,ULONG,void*,ULONG,ULONG);
NTSYSAPI BOOLEAN   WINAPI RtlSetUserValueHeap(HANDLE,ULONG,void*,void*);
NTSYSAPI SIZE_T    WINAPI RtlSizeHeap(HANDLE,ULONG,const void*);
NTSYSAPI NTSTATUS  WINAPI RtlSleepConditionVariableCS(RTL_CONDITION_VARIABLE*,RTL_CRITICAL_SECTION*,const LARGE_INTEGER*);
NTSYSAPI NTSTATUS  WINAPI RtlSleepConditionVariableSRW(RTL_CONDITION_VARIABLE*,RTL_SRWLOCK*,const LARGE_INTEGER*,ULONG);
NTSYSAPI NTSTATUS  WINAPI RtlStringFromGUID(REFGUID,PUNICODE_STRING);
NTSYSAPI LPDWORD   WINAPI RtlSubAuthoritySid(PSID,DWORD);
NTSYSAPI LPBYTE    WINAPI RtlSubAuthorityCountSid(PSID);
NTSYSAPI NTSTATUS  WINAPI RtlSystemTimeToLocalTime(const LARGE_INTEGER*,PLARGE_INTEGER);
NTSYSAPI BOOLEAN   WINAPI RtlTimeFieldsToTime(PTIME_FIELDS,PLARGE_INTEGER);
NTSYSAPI void      WINAPI RtlTimeToElapsedTimeFields(const LARGE_INTEGER *,PTIME_FIELDS);
NTSYSAPI BOOLEAN   WINAPI RtlTimeToSecondsSince1970(const LARGE_INTEGER *,LPDWORD);
NTSYSAPI BOOLEAN   WINAPI RtlTimeToSecondsSince1980(const LARGE_INTEGER *,LPDWORD);
NTSYSAPI void      WINAPI RtlTimeToTimeFields(const LARGE_INTEGER*,PTIME_FIELDS);
NTSYSAPI BOOLEAN   WINAPI RtlTryAcquireSRWLockExclusive(RTL_SRWLOCK *);
NTSYSAPI BOOLEAN   WINAPI RtlTryAcquireSRWLockShared(RTL_SRWLOCK *);
NTSYSAPI BOOL      WINAPI RtlTryEnterCriticalSection(RTL_CRITICAL_SECTION *);
NTSYSAPI NTSTATUS  WINAPI RtlUTF8ToUnicodeN(WCHAR*,DWORD,DWORD*,const char*,DWORD);
NTSYSAPI DWORD     WINAPI RtlUnicodeStringToAnsiSize(const UNICODE_STRING*);
NTSYSAPI NTSTATUS  WINAPI RtlUnicodeStringToAnsiString(PANSI_STRING,PCUNICODE_STRING,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI RtlUnicodeStringToInteger(const UNICODE_STRING *,ULONG,ULONG *);
NTSYSAPI DWORD     WINAPI RtlUnicodeStringToOemSize(const UNICODE_STRING*);
NTSYSAPI NTSTATUS  WINAPI RtlUnicodeStringToOemString(POEM_STRING,PCUNICODE_STRING,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI RtlUnicodeToCustomCPN(CPTABLEINFO*,char*,DWORD,DWORD*,const WCHAR*,DWORD);
NTSYSAPI NTSTATUS  WINAPI RtlUnicodeToMultiByteN(LPSTR,DWORD,LPDWORD,LPCWSTR,DWORD);
NTSYSAPI NTSTATUS  WINAPI RtlUnicodeToMultiByteSize(PULONG,PCWSTR,ULONG);
NTSYSAPI NTSTATUS  WINAPI RtlUnicodeToOemN(LPSTR,DWORD,LPDWORD,LPCWSTR,DWORD);
NTSYSAPI NTSTATUS  WINAPI RtlUnicodeToUTF8N(LPSTR,DWORD,LPDWORD,LPCWSTR,DWORD);
NTSYSAPI ULONG     WINAPI RtlUniform(PULONG);
NTSYSAPI BOOLEAN   WINAPI RtlUnlockHeap(HANDLE);
NTSYSAPI WCHAR     WINAPI RtlUpcaseUnicodeChar(WCHAR);
NTSYSAPI NTSTATUS  WINAPI RtlUpcaseUnicodeStringToAnsiString(STRING*,const UNICODE_STRING*,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI RtlUpcaseUnicodeStringToCountedOemString(STRING*,const UNICODE_STRING*,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI RtlUpcaseUnicodeStringToOemString(STRING*,const UNICODE_STRING*,BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI RtlUpcaseUnicodeToCustomCPN(CPTABLEINFO*,char*,DWORD,DWORD*,const WCHAR*,DWORD);
NTSYSAPI NTSTATUS  WINAPI RtlUpcaseUnicodeToMultiByteN(LPSTR,DWORD,LPDWORD,LPCWSTR,DWORD);
NTSYSAPI NTSTATUS  WINAPI RtlUpcaseUnicodeToOemN(LPSTR,DWORD,LPDWORD,LPCWSTR,DWORD);
NTSYSAPI NTSTATUS  WINAPI RtlUpdateTimer(HANDLE, HANDLE, DWORD, DWORD);
NTSYSAPI void      WINAPI RtlUserThreadStart(PRTL_THREAD_START_ROUTINE,void*);
NTSYSAPI BOOLEAN   WINAPI RtlValidAcl(PACL);
NTSYSAPI BOOLEAN   WINAPI RtlValidRelativeSecurityDescriptor(PSECURITY_DESCRIPTOR,ULONG,SECURITY_INFORMATION);
NTSYSAPI BOOLEAN   WINAPI RtlValidSecurityDescriptor(PSECURITY_DESCRIPTOR);
NTSYSAPI BOOLEAN   WINAPI RtlValidSid(PSID);
NTSYSAPI BOOLEAN   WINAPI RtlValidateHeap(HANDLE,ULONG,LPCVOID);
NTSYSAPI NTSTATUS  WINAPI RtlVerifyVersionInfo(const RTL_OSVERSIONINFOEXW*,DWORD,DWORDLONG);
NTSYSAPI NTSTATUS  WINAPI RtlWaitOnAddress(const void *,const void *,SIZE_T,const LARGE_INTEGER *);
NTSYSAPI void      WINAPI RtlWakeAddressAll(const void *);
NTSYSAPI void      WINAPI RtlWakeAddressSingle(const void *);
NTSYSAPI void      WINAPI RtlWakeAllConditionVariable(RTL_CONDITION_VARIABLE *);
NTSYSAPI void      WINAPI RtlWakeConditionVariable(RTL_CONDITION_VARIABLE *);
NTSYSAPI NTSTATUS  WINAPI RtlWalkHeap(HANDLE,PVOID);
NTSYSAPI NTSTATUS  WINAPI RtlWow64EnableFsRedirection(BOOLEAN);
NTSYSAPI NTSTATUS  WINAPI RtlWow64EnableFsRedirectionEx(ULONG,ULONG*);
NTSYSAPI USHORT    WINAPI RtlWow64GetCurrentMachine(void);
NTSYSAPI NTSTATUS  WINAPI RtlWow64GetProcessMachines(HANDLE,USHORT*,USHORT*);
NTSYSAPI NTSTATUS  WINAPI RtlWow64GetSharedInfoProcess(HANDLE,BOOLEAN*,WOW64INFO*);
NTSYSAPI NTSTATUS  WINAPI RtlWow64IsWowGuestMachineSupported(USHORT,BOOLEAN*);
NTSYSAPI NTSTATUS  WINAPI RtlWriteRegistryValue(ULONG,PCWSTR,PCWSTR,ULONG,PVOID,ULONG);
NTSYSAPI NTSTATUS  WINAPI RtlZombifyActivationContext(HANDLE);
NTSYSAPI NTSTATUS  WINAPI RtlpNtCreateKey(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*,ULONG,const UNICODE_STRING*,ULONG,PULONG);
NTSYSAPI NTSTATUS  WINAPI RtlpNtEnumerateSubKey(HANDLE,UNICODE_STRING *, ULONG);
NTSYSAPI NTSTATUS  WINAPI RtlpNtMakeTemporaryKey(HANDLE);
NTSYSAPI NTSTATUS  WINAPI RtlpNtOpenKey(PHANDLE,ACCESS_MASK,OBJECT_ATTRIBUTES*);
NTSYSAPI NTSTATUS  WINAPI RtlpNtSetValueKey(HANDLE,ULONG,const void*,ULONG);
NTSYSAPI NTSTATUS  WINAPI RtlpWaitForCriticalSection(RTL_CRITICAL_SECTION *);
NTSYSAPI NTSTATUS  WINAPI RtlpUnWaitCriticalSection(RTL_CRITICAL_SECTION *);
NTSYSAPI NTSTATUS  WINAPI TpAllocCleanupGroup(TP_CLEANUP_GROUP **);
NTSYSAPI NTSTATUS  WINAPI TpAllocIoCompletion(TP_IO **,HANDLE,PTP_IO_CALLBACK,void *,TP_CALLBACK_ENVIRON *);
NTSYSAPI NTSTATUS  WINAPI TpAllocPool(TP_POOL **,PVOID);
NTSYSAPI NTSTATUS  WINAPI TpAllocTimer(TP_TIMER **,PTP_TIMER_CALLBACK,PVOID,TP_CALLBACK_ENVIRON *);
NTSYSAPI NTSTATUS  WINAPI TpAllocWait(TP_WAIT **,PTP_WAIT_CALLBACK,PVOID,TP_CALLBACK_ENVIRON *);
NTSYSAPI NTSTATUS  WINAPI TpAllocWork(TP_WORK **,PTP_WORK_CALLBACK,PVOID,TP_CALLBACK_ENVIRON *);
NTSYSAPI void      WINAPI TpCallbackLeaveCriticalSectionOnCompletion(TP_CALLBACK_INSTANCE *,RTL_CRITICAL_SECTION *);
NTSYSAPI NTSTATUS  WINAPI TpCallbackMayRunLong(TP_CALLBACK_INSTANCE *);
NTSYSAPI void      WINAPI TpCallbackReleaseMutexOnCompletion(TP_CALLBACK_INSTANCE *,HANDLE);
NTSYSAPI void      WINAPI TpCallbackReleaseSemaphoreOnCompletion(TP_CALLBACK_INSTANCE *,HANDLE,DWORD);
NTSYSAPI void      WINAPI TpCallbackSetEventOnCompletion(TP_CALLBACK_INSTANCE *,HANDLE);
NTSYSAPI void      WINAPI TpCallbackUnloadDllOnCompletion(TP_CALLBACK_INSTANCE *,HMODULE);
NTSYSAPI void      WINAPI TpCancelAsyncIoOperation(TP_IO *);
NTSYSAPI void      WINAPI TpDisassociateCallback(TP_CALLBACK_INSTANCE *);
NTSYSAPI BOOL      WINAPI TpIsTimerSet(TP_TIMER *);
NTSYSAPI void      WINAPI TpPostWork(TP_WORK *);
NTSYSAPI NTSTATUS  WINAPI TpQueryPoolStackInformation(TP_POOL *, TP_POOL_STACK_INFORMATION *stack_info);
NTSYSAPI void      WINAPI TpReleaseCleanupGroup(TP_CLEANUP_GROUP *);
NTSYSAPI void      WINAPI TpReleaseCleanupGroupMembers(TP_CLEANUP_GROUP *,BOOL,PVOID);
NTSYSAPI void      WINAPI TpReleaseIoCompletion(TP_IO *);
NTSYSAPI void      WINAPI TpReleasePool(TP_POOL *);
NTSYSAPI void      WINAPI TpReleaseTimer(TP_TIMER *);
NTSYSAPI void      WINAPI TpReleaseWait(TP_WAIT *);
NTSYSAPI void      WINAPI TpReleaseWork(TP_WORK *);
NTSYSAPI void      WINAPI TpSetPoolMaxThreads(TP_POOL *,DWORD);
NTSYSAPI BOOL      WINAPI TpSetPoolMinThreads(TP_POOL *,DWORD);
NTSYSAPI NTSTATUS  WINAPI TpSetPoolStackInformation(TP_POOL *, TP_POOL_STACK_INFORMATION *stack_info);
NTSYSAPI void      WINAPI TpSetTimer(TP_TIMER *, LARGE_INTEGER *,LONG,LONG);
NTSYSAPI void      WINAPI TpSetWait(TP_WAIT *,HANDLE,LARGE_INTEGER *);
NTSYSAPI NTSTATUS  WINAPI TpSimpleTryPost(PTP_SIMPLE_CALLBACK,PVOID,TP_CALLBACK_ENVIRON *);
NTSYSAPI void      WINAPI TpStartAsyncIoOperation(TP_IO *);
NTSYSAPI void      WINAPI TpWaitForIoCompletion(TP_IO *,BOOL);
NTSYSAPI void      WINAPI TpWaitForTimer(TP_TIMER *,BOOL);
NTSYSAPI void      WINAPI TpWaitForWait(TP_WAIT *,BOOL);
NTSYSAPI void      WINAPI TpWaitForWork(TP_WORK *,BOOL);
#ifdef __ms_va_list
NTSYSAPI NTSTATUS  WINAPI vDbgPrintEx(ULONG,ULONG,LPCSTR,__ms_va_list);
NTSYSAPI NTSTATUS  WINAPI vDbgPrintExWithPrefix(LPCSTR,ULONG,ULONG,LPCSTR,__ms_va_list);
#endif

/* 32-bit or 64-bit only functions */

#ifdef _WIN64
NTSYSAPI void      WINAPI RtlOpenCrossProcessEmulatorWorkConnection(HANDLE,HANDLE*,void**);
NTSYSAPI NTSTATUS  WINAPI RtlWow64GetCpuAreaInfo(WOW64_CPURESERVED*,ULONG,WOW64_CPU_AREA_INFO*);
NTSYSAPI NTSTATUS  WINAPI RtlWow64GetCurrentCpuArea(USHORT*,void**,void**);
NTSYSAPI NTSTATUS  WINAPI RtlWow64GetThreadContext(HANDLE,WOW64_CONTEXT*);
NTSYSAPI NTSTATUS  WINAPI RtlWow64GetThreadSelectorEntry(HANDLE,THREAD_DESCRIPTOR_INFORMATION*,ULONG,ULONG*);
NTSYSAPI CROSS_PROCESS_WORK_ENTRY * WINAPI RtlWow64PopAllCrossProcessWorkFromWorkList(CROSS_PROCESS_WORK_HDR*,BOOLEAN*);
NTSYSAPI CROSS_PROCESS_WORK_ENTRY * WINAPI RtlWow64PopCrossProcessWorkFromFreeList(CROSS_PROCESS_WORK_HDR*);
NTSYSAPI BOOLEAN   WINAPI RtlWow64PushCrossProcessWorkOntoFreeList(CROSS_PROCESS_WORK_HDR*,CROSS_PROCESS_WORK_ENTRY*);
NTSYSAPI BOOLEAN   WINAPI RtlWow64PushCrossProcessWorkOntoWorkList(CROSS_PROCESS_WORK_HDR*,CROSS_PROCESS_WORK_ENTRY*,void**);
NTSYSAPI BOOLEAN   WINAPI RtlWow64RequestCrossProcessHeavyFlush(CROSS_PROCESS_WORK_HDR*);
NTSYSAPI NTSTATUS  WINAPI RtlWow64SetThreadContext(HANDLE,const WOW64_CONTEXT*);
#else
NTSYSAPI NTSTATUS  WINAPI NtWow64AllocateVirtualMemory64(HANDLE,ULONG64*,ULONG64,ULONG64*,ULONG,ULONG);
NTSYSAPI NTSTATUS  WINAPI NtWow64GetNativeSystemInformation(SYSTEM_INFORMATION_CLASS,void*,ULONG,ULONG*);
NTSYSAPI NTSTATUS  WINAPI NtWow64IsProcessorFeaturePresent(UINT);
NTSYSAPI NTSTATUS  WINAPI NtWow64QueryInformationProcess64(HANDLE,PROCESSINFOCLASS,void*,ULONG,ULONG*);
NTSYSAPI NTSTATUS  WINAPI NtWow64ReadVirtualMemory64(HANDLE,ULONG64,void*,ULONG64,ULONG64*);
NTSYSAPI NTSTATUS  WINAPI NtWow64WriteVirtualMemory64(HANDLE,ULONG64,const void*,ULONG64,ULONG64*);
NTSYSAPI LONGLONG  WINAPI RtlConvertLongToLargeInteger(LONG);
NTSYSAPI ULONGLONG WINAPI RtlConvertUlongToLargeInteger(ULONG);
NTSYSAPI LONGLONG  WINAPI RtlEnlargedIntegerMultiply(INT,INT);
NTSYSAPI ULONGLONG WINAPI RtlEnlargedUnsignedMultiply(UINT,UINT);
NTSYSAPI UINT      WINAPI RtlEnlargedUnsignedDivide(ULONGLONG,UINT,UINT *);
NTSYSAPI LONGLONG  WINAPI RtlExtendedMagicDivide(LONGLONG,LONGLONG,INT);
NTSYSAPI LONGLONG  WINAPI RtlExtendedIntegerMultiply(LONGLONG,INT);
NTSYSAPI LONGLONG  WINAPI RtlExtendedLargeIntegerDivide(LONGLONG,INT,INT *);
NTSYSAPI LONGLONG  WINAPI RtlInterlockedCompareExchange64(LONGLONG*,LONGLONG,LONGLONG);
NTSYSAPI LONGLONG  WINAPI RtlLargeIntegerAdd(LONGLONG,LONGLONG);
NTSYSAPI LONGLONG  WINAPI RtlLargeIntegerArithmeticShift(LONGLONG,INT);
NTSYSAPI LONGLONG  WINAPI RtlLargeIntegerNegate(LONGLONG);
NTSYSAPI LONGLONG  WINAPI RtlLargeIntegerShiftLeft(LONGLONG,INT);
NTSYSAPI LONGLONG  WINAPI RtlLargeIntegerShiftRight(LONGLONG,INT);
NTSYSAPI LONGLONG  WINAPI RtlLargeIntegerSubtract(LONGLONG,LONGLONG);
NTSYSAPI NTSTATUS  WINAPI RtlLargeIntegerToChar(const ULONGLONG *,ULONG,ULONG,PCHAR);
#endif

/* Wine internal functions */

NTSYSAPI NTSTATUS WINAPI wine_nt_to_unix_file_name( const OBJECT_ATTRIBUTES *attr, char *nameA, ULONG *size,
                                                    UINT disposition );
NTSYSAPI NTSTATUS WINAPI wine_unix_to_nt_file_name( const char *name, WCHAR *buffer, ULONG *size );


/***********************************************************************
 * Inline functions
 */

#define InitializeObjectAttributes(p,n,a,r,s) \
    do { \
        (p)->Length = sizeof(OBJECT_ATTRIBUTES); \
        (p)->RootDirectory = r; \
        (p)->Attributes = a; \
        (p)->ObjectName = n; \
        (p)->SecurityDescriptor = s; \
        (p)->SecurityQualityOfService = NULL; \
    } while (0)

#define NtCurrentProcess() ((HANDLE)~(ULONG_PTR)0)
#define NtCurrentThread()  ((HANDLE)~(ULONG_PTR)1)

#if !defined(__REACTOS__) || !defined(RtlFillMemory)
#define RtlFillMemory(Destination,Length,Fill) memset((Destination),(Fill),(Length))
#endif // !defined(__REACTOS__) || !defined(RtlFillMemory)
#if !defined(__REACTOS__) || !defined(RtlMoveMemory)
#define RtlMoveMemory(Destination,Source,Length) memmove((Destination),(Source),(Length))
#endif // !defined(__REACTOS__) || !defined(RtlMoveMemory)
#define RtlStoreUlong(p,v)  do { ULONG _v = (v); memcpy((p), &_v, sizeof(_v)); } while (0)
#define RtlStoreUlonglong(p,v) do { ULONGLONG _v = (v); memcpy((p), &_v, sizeof(_v)); } while (0)
#define RtlRetrieveUlong(p,s) memcpy((p), (s), sizeof(ULONG))
#define RtlRetrieveUlonglong(p,s) memcpy((p), (s), sizeof(ULONGLONG))
#if !defined(__REACTOS__) || !defined(RtlZeroMemory)
#define RtlZeroMemory(Destination,Length) memset((Destination),0,(Length))
#endif // !defined(__REACTOS__) || !defined(RtlZeroMemory)

static inline BOOLEAN RtlCheckBit(PCRTL_BITMAP lpBits, ULONG ulBit)
{
    if (lpBits && ulBit < lpBits->SizeOfBitMap &&
        lpBits->Buffer[ulBit >> 5] & (1 << (ulBit & 31)))
        return TRUE;
    return FALSE;
}

/* These are implemented as __fastcall, so we can't let Winelib apps link with them.
 * Moreover, they're always inlined and not exported on 64bit systems.
 */
static inline USHORT RtlUshortByteSwap(USHORT s)
{
    return (s >> 8) | (s << 8);
}
static inline ULONG RtlUlongByteSwap(ULONG i)
{
    return ((ULONG)RtlUshortByteSwap((USHORT)i) << 16) | RtlUshortByteSwap((USHORT)(i >> 16));
}
static inline ULONGLONG RtlUlonglongByteSwap(ULONGLONG i)
{
    return ((ULONGLONG)RtlUlongByteSwap((ULONG)i) << 32) | RtlUlongByteSwap((ULONG)(i >> 32));
}

/* list manipulation macros */
#define InitializeListHead(le)  (void)((le)->Flink = (le)->Blink = (le))
#define InsertHeadList(le,e)    do { PLIST_ENTRY f = (le)->Flink; (e)->Flink = f; (e)->Blink = (le); f->Blink = (e); (le)->Flink = (e); } while (0)
#define InsertTailList(le,e)    do { PLIST_ENTRY b = (le)->Blink; (e)->Flink = (le); (e)->Blink = b; b->Flink = (e); (le)->Blink = (e); } while (0)
#define IsListEmpty(le)         ((le)->Flink == (le))
#define RemoveEntryList(e)      do { PLIST_ENTRY f = (e)->Flink, b = (e)->Blink; f->Blink = b; b->Flink = f; (e)->Flink = (e)->Blink = NULL; } while (0)
static inline PLIST_ENTRY RemoveHeadList(PLIST_ENTRY le)
{
    PLIST_ENTRY f, b, e;

    e = le->Flink;
    f = le->Flink->Flink;
    b = le->Flink->Blink;
    f->Blink = b;
    b->Flink = f;

    if (e != le) e->Flink = e->Blink = NULL;
    return e;
}
static inline PLIST_ENTRY RemoveTailList(PLIST_ENTRY le)
{
    PLIST_ENTRY f, b, e;

    e = le->Blink;
    f = le->Blink->Flink;
    b = le->Blink->Blink;
    f->Blink = b;
    b->Flink = f;

    if (e != le) e->Flink = e->Blink = NULL;
    return e;
}


#ifdef __WINESRC__

/* Wine internal functions */

NTSYSAPI NTSTATUS WINAPI __wine_unix_spawnvp( char * const argv[], int wait );

/* The thread information for 16-bit threads */
/* NtCurrentTeb()->SubSystemTib points to this */
typedef struct
{
    void           *unknown;    /* 00 unknown */
    UNICODE_STRING *exe_name;   /* 04 exe module name */

    /* the following fields do not exist under Windows */
    UNICODE_STRING  exe_str;    /* exe name string pointed to by exe_name */
    CURDIR          curdir;     /* current directory */
    WCHAR           curdir_buffer[MAX_PATH];
} WIN16_SUBSYSTEM_TIB;

/* Undocumented: layout of the locale data in the locale.nls file */

typedef struct
{
    UINT   sname;                  /* 000 LOCALE_SNAME */
    UINT   sopentypelanguagetag;   /* 004 LOCALE_SOPENTYPELANGUAGETAG */
    USHORT ilanguage;              /* 008 LOCALE_ILANGUAGE */
    USHORT unique_lcid;            /* 00a unique id if lcid == 0x1000 */
    USHORT idigits;                /* 00c LOCALE_IDIGITS */
    USHORT inegnumber;             /* 00e LOCALE_INEGNUMBER */
    USHORT icurrdigits;            /* 010 LOCALE_ICURRDIGITS*/
    USHORT icurrency;              /* 012 LOCALE_ICURRENCY */
    USHORT inegcurr;               /* 014 LOCALE_INEGCURR */
    USHORT ilzero;                 /* 016 LOCALE_ILZERO */
    USHORT inotneutral;            /* 018 LOCALE_INEUTRAL (inverted) */
    USHORT ifirstdayofweek;        /* 01a LOCALE_IFIRSTDAYOFWEEK (monday=0) */
    USHORT ifirstweekofyear;       /* 01c LOCALE_IFIRSTWEEKOFYEAR */
    USHORT icountry;               /* 01e LOCALE_ICOUNTRY */
    USHORT imeasure;               /* 020 LOCALE_IMEASURE */
    USHORT idigitsubstitution;     /* 022 LOCALE_IDIGITSUBSTITUTION */
    UINT   sgrouping;              /* 024 LOCALE_SGROUPING (as binary string) */
    UINT   smongrouping;           /* 028 LOCALE_SMONGROUPING  (as binary string) */
    UINT   slist;                  /* 02c LOCALE_SLIST */
    UINT   sdecimal;               /* 030 LOCALE_SDECIMAL */
    UINT   sthousand;              /* 034 LOCALE_STHOUSAND */
    UINT   scurrency;              /* 038 LOCALE_SCURRENCY */
    UINT   smondecimalsep;         /* 03c LOCALE_SMONDECIMALSEP */
    UINT   smonthousandsep;        /* 040 LOCALE_SMONTHOUSANDSEP */
    UINT   spositivesign;          /* 044 LOCALE_SPOSITIVESIGN */
    UINT   snegativesign;          /* 048 LOCALE_SNEGATIVESIGN */
    UINT   s1159;                  /* 04c LOCALE_S1159 */
    UINT   s2359;                  /* 050 LOCALE_S2359 */
    UINT   snativedigits;          /* 054 LOCALE_SNATIVEDIGITS (array of single digits) */
    UINT   stimeformat;            /* 058 LOCALE_STIMEFORMAT (array of formats) */
    UINT   sshortdate;             /* 05c LOCALE_SSHORTDATE (array of formats) */
    UINT   slongdate;              /* 060 LOCALE_SLONGDATE (array of formats) */
    UINT   syearmonth;             /* 064 LOCALE_SYEARMONTH (array of formats) */
    UINT   sduration;              /* 068 LOCALE_SDURATION (array of formats) */
    USHORT idefaultlanguage;       /* 06c LOCALE_IDEFAULTLANGUAGE */
    USHORT idefaultansicodepage;   /* 06e LOCALE_IDEFAULTANSICODEPAGE */
    USHORT idefaultcodepage;       /* 070 LOCALE_IDEFAULTCODEPAGE */
    USHORT idefaultmaccodepage;    /* 072 LOCALE_IDEFAULTMACCODEPAGE */
    USHORT idefaultebcdiccodepage; /* 074 LOCALE_IDEFAULTEBCDICCODEPAGE */
    USHORT old_geoid;              /* 076 LOCALE_IGEOID (older version?) */
    USHORT ipapersize;             /* 078 LOCALE_IPAPERSIZE */
    BYTE   islamic_cal[2];         /* 07a calendar id for islamic calendars (?) */
    UINT   scalendartype;          /* 07c string, first char is LOCALE_ICALENDARTYPE, next chars are LOCALE_IOPTIONALCALENDAR */
    UINT   sabbrevlangname;        /* 080 LOCALE_SABBREVLANGNAME */
    UINT   siso639langname;        /* 084 LOCALE_SISO639LANGNAME */
    UINT   senglanguage;           /* 088 LOCALE_SENGLANGUAGE */
    UINT   snativelangname;        /* 08c LOCALE_SNATIVELANGNAME */
    UINT   sengcountry;            /* 090 LOCALE_SENGCOUNTRY */
    UINT   snativectryname;        /* 094 LOCALE_SNATIVECTRYNAME */
    UINT   sabbrevctryname;        /* 098 LOCALE_SABBREVCTRYNAME */
    UINT   siso3166ctryname;       /* 09c LOCALE_SISO3166CTRYNAME */
    UINT   sintlsymbol;            /* 0a0 LOCALE_SINTLSYMBOL */
    UINT   sengcurrname;           /* 0a4 LOCALE_SENGCURRNAME */
    UINT   snativecurrname;        /* 0a8 LOCALE_SNATIVECURRNAME */
    UINT   fontsignature;          /* 0ac LOCALE_FONTSIGNATURE (binary string) */
    UINT   siso639langname2;       /* 0b0 LOCALE_SISO639LANGNAME2 */
    UINT   siso3166ctryname2;      /* 0b4 LOCALE_SISO3166CTRYNAME2 */
    UINT   sparent;                /* 0b8 LOCALE_SPARENT */
    UINT   sdayname;               /* 0bc LOCALE_SDAYNAME1 (array of days 1..7) */
    UINT   sabbrevdayname;         /* 0c0 LOCALE_SABBREVDAYNAME1  (array of days 1..7) */
    UINT   smonthname;             /* 0c4 LOCALE_SMONTHNAME1 (array of months 1..13) */
    UINT   sabbrevmonthname;       /* 0c8 LOCALE_SABBREVMONTHNAME1 (array of months 1..13) */
    UINT   sgenitivemonth;         /* 0cc equivalent of LOCALE_SMONTHNAME1 for genitive months */
    UINT   sabbrevgenitivemonth;   /* 0d0 equivalent of LOCALE_SABBREVMONTHNAME1 for genitive months */
    UINT   calnames;               /* 0d4 array of calendar names */
    UINT   customsorts;            /* 0d8 array of custom sort names */
    USHORT inegativepercent;       /* 0dc LOCALE_INEGATIVEPERCENT */
    USHORT ipositivepercent;       /* 0de LOCALE_IPOSITIVEPERCENT */
    USHORT unknown1;               /* 0e0 */
    USHORT ireadinglayout;         /* 0e2 LOCALE_IREADINGLAYOUT */
    USHORT unknown2[2];            /* 0e4 */
    UINT   unused1;                /* 0e8 unused? */
    UINT   sengdisplayname;        /* 0ec LOCALE_SENGLISHDISPLAYNAME */
    UINT   snativedisplayname;     /* 0f0 LOCALE_SNATIVEDISPLAYNAME */
    UINT   spercent;               /* 0f4 LOCALE_SPERCENT */
    UINT   snan;                   /* 0f8 LOCALE_SNAN */
    UINT   sposinfinity;           /* 0fc LOCALE_SPOSINFINITY */
    UINT   sneginfinity;           /* 100 LOCALE_SNEGINFINITY */
    UINT   unused2;                /* 104 unused? */
    UINT   serastring;             /* 108 CAL_SERASTRING */
    UINT   sabbreverastring;       /* 10c CAL_SABBREVERASTRING */
    UINT   unused3;                /* 110 unused? */
    UINT   sconsolefallbackname;   /* 114 LOCALE_SCONSOLEFALLBACKNAME */
    UINT   sshorttime;             /* 118 LOCALE_SSHORTTIME (array of formats) */
    UINT   sshortestdayname;       /* 11c LOCALE_SSHORTESTDAYNAME1 (array of days 1..7) */
    UINT   unused4;                /* 120 unused? */
    UINT   ssortlocale;            /* 124 LOCALE_SSORTLOCALE */
    UINT   skeyboardstoinstall;    /* 128 LOCALE_SKEYBOARDSTOINSTALL */
    UINT   sscripts;               /* 12c LOCALE_SSCRIPTS */
    UINT   srelativelongdate;      /* 130 LOCALE_SRELATIVELONGDATE */
    UINT   igeoid;                 /* 134 LOCALE_IGEOID */
    UINT   sshortestam;            /* 138 LOCALE_SSHORTESTAM */
    UINT   sshortestpm;            /* 13c LOCALE_SSHORTESTPM */
    UINT   smonthday;              /* 140 LOCALE_SMONTHDAY (array of formats) */
    UINT   keyboard_layout;        /* 144 keyboard layouts */
} NLS_LOCALE_DATA;

typedef struct
{
    UINT   id;                     /* 00 lcid */
    USHORT idx;                    /* 04 index in locales array */
    USHORT name;                   /* 06 locale name */
} NLS_LOCALE_LCID_INDEX;

typedef struct
{
    USHORT name;                   /* 00 locale name */
    USHORT idx;                    /* 02 index in locales array */
    UINT   id;                     /* 04 lcid */
} NLS_LOCALE_LCNAME_INDEX;

typedef struct
{
    UINT   offset;                 /* 00 offset to version, always 8? */
    UINT   unknown1;               /* 04 */
    UINT   version;                /* 08 file format version */
    UINT   magic;                  /* 0c magic 'NSDS' */
    UINT   unknown2[3];            /* 10 */
    USHORT header_size;            /* 1c size of this header (?) */
    USHORT nb_lcids;               /* 1e number of lcids in index */
    USHORT nb_locales;             /* 20 number of locales in array */
    USHORT locale_size;            /* 22 size of NLS_LOCALE_DATA structure */
    UINT   locales_offset;         /* 24 offset of locales array */
    USHORT nb_lcnames;             /* 28 number of lcnames in index */
    USHORT pad;                    /* 2a */
    UINT   lcids_offset;           /* 2c offset of lcids index */
    UINT   lcnames_offset;         /* 30 offset of lcnames index */
    UINT   unknown3;               /* 34 */
    USHORT nb_calendars;           /* 38 number of calendars in array */
    USHORT calendar_size;          /* 3a size of calendar structure */
    UINT   calendars_offset;       /* 3c offset of calendars array */
    UINT   strings_offset;         /* 40 offset of strings data */
    USHORT unknown4[4];            /* 44 */
} NLS_LOCALE_HEADER;

#endif /* __WINESRC__ */

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif  /* __WINE_WINTERNL_H */
