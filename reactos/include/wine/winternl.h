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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_WINTERNAL_H
#define __WINE_WINTERNAL_H

#include <windef.h>
#include <wine/winnt.h>

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */


/**********************************************************************
 * Fundamental types and data structures
 */

typedef LONG NTSTATUS;

typedef CONST char *PCSZ;

typedef short CSHORT;
typedef CSHORT *PCSHORT;

typedef struct _STRING {
  USHORT Length;
  USHORT MaximumLength;
  PCHAR Buffer;
} STRING, *PSTRING;

typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;
typedef const STRING *PCANSI_STRING;

typedef STRING OEM_STRING;
typedef PSTRING POEM_STRING;
typedef const STRING *PCOEM_STRING;

typedef struct _UNICODE_STRING {
  USHORT Length;        /* bytes */
  USHORT MaximumLength; /* bytes */
  PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef const UNICODE_STRING *PCUNICODE_STRING;

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

typedef struct tagRTL_BITMAP {
    ULONG  SizeOfBitMap; /* Number of bits in the bitmap */
    LPBYTE BitMapBuffer; /* Bitmap data, assumed sized to a DWORD boundary */
} RTL_BITMAP, *PRTL_BITMAP;

typedef const RTL_BITMAP *PCRTL_BITMAP;

typedef struct tagRTL_BITMAP_RUN {
    ULONG StartOfRun; /* Bit position at which run starts - FIXME: Name? */
    ULONG SizeOfRun;  /* Size of the run in bits - FIXME: Name? */
} RTL_BITMAP_RUN, *PRTL_BITMAP_RUN;

typedef const RTL_BITMAP_RUN *PCRTL_BITMAP_RUN;

typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    ULONG               AllocationSize;
    ULONG               Size;
    ULONG               Flags;
    ULONG               DebugFlags;
    HANDLE              hConsole;
    ULONG               ProcessGroup;
    HANDLE              hStdInput;
    HANDLE              hStdOutput;
    HANDLE              hStdError;
    UNICODE_STRING      CurrentDirectoryName;
    HANDLE              CurrentDirectoryHandle;
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
} PEB_LDR_DATA, *PPEB_LDR_DATA;

/***********************************************************************
 * PEB data structure
 */
typedef struct _PEB
{
    BYTE                         Reserved1[2];       /*  00 */
    BYTE                         BeingDebugged;      /*  02 */
    BYTE                         Reserved2[5];       /*  03 */
    HMODULE                      ImageBaseAddress;   /*  08 */
    PPEB_LDR_DATA                LdrData;            /*  0c */
    RTL_USER_PROCESS_PARAMETERS *ProcessParameters;  /*  10 */
    PVOID                        __pad_14;           /*  14 */
    HANDLE                       ProcessHeap;        /*  18 */
    BYTE                         __pad_1c[36];       /*  1c */
    PRTL_BITMAP                  TlsBitmap;          /*  40 */
    ULONG                        TlsBitmapBits[2];   /*  44 */
    BYTE                         __pad_4c[156];      /*  4c */
    PVOID                        Reserved3[59];      /*  e8 */
    ULONG                        SessionId;          /* 1d4 */
} PEB, *PPEB;


/***********************************************************************
 * TEB data structure
 */
#if defined(_NTSYSTEM_) || defined(_KERNEL32_)  /* hack, should go away */
# define WINE_NO_TEB
#endif

#ifndef WINE_NO_TEB  /* don't define TEB if included from thread.h */
# ifndef WINE_TEB_DEFINED
# define WINE_TEB_DEFINED
typedef struct _TEB
{
    NT_TIB          Tib;                        /* 000 */
    PVOID           EnvironmentPointer;         /* 01c */
    CLIENT_ID       ClientId;                   /* 020 */
    PVOID           ActiveRpcHandle;            /* 028 */
    PVOID           ThreadLocalStoragePointer;  /* 02c */
    PPEB            Peb;                        /* 030 */
    ULONG           LastErrorValue;             /* 034 */
    BYTE            __pad038[140];              /* 038 */
    ULONG           CurrentLocale;              /* 0c4 */
    BYTE            __pad0c8[1752];             /* 0c8 */
    PVOID           Reserved2[278];             /* 7a0 */
    UNICODE_STRING  StaticUnicodeString;        /* bf8 used by advapi32 */
    WCHAR           StaticUnicodeBuffer[261];   /* c00 used by advapi32 */
    PVOID           DeallocationStack;          /* e0c */
    PVOID           TlsSlots[64];               /* e10 */
    LIST_ENTRY      TlsLinks;                   /* f10 */
    PVOID           Reserved4[26];              /* f18 */
    PVOID           ReservedForOle;             /* f80 Windows 2000 only */
    PVOID           Reserved5[4];               /* f84 */
    PVOID           TlsExpansionSlots;          /* f94 */
} TEB, *PTEB;
# endif /* WINE_TEB_DEFINED */
#endif  /* WINE_NO_TEB */

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
    FileMaximumInformation
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

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

typedef struct _FILE_POSITION_INFORMATION {
    LARGE_INTEGER CurrentByteOffset;
} FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;

typedef struct _FILE_ALIGNMENT_INFORMATION {
    ULONG AlignmentRequirement;
} FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION;

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

typedef struct _FILE_STREAM_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG StreamNameLength;
    LARGE_INTEGER StreamSize;
    LARGE_INTEGER StreamAllocationSize;
    WCHAR StreamName[1];
} FILE_STREAM_INFORMATION, *PFILE_STREAM_INFORMATION;

struct _FILE_ATTRIBUTE_TAG_INFORMATION
{
    ULONG FileAttributes;
    ULONG ReparseTag;
} FILE_ATTRIBUTE_TAG_INFORMATION, *PFILE_ATTRIBUTE_TAG_INFORMATION;

typedef enum _FSINFOCLASS {
    FileFsVolumeInformation = 1,
    FileFsLabelInformation,
    FileFsSizeInformation,
    FileFsDeviceInformation,
    FileFsAttributeInformation,
    FileFsControlInformation,
    FileFsFullSizeInformation,
    FileFsObjectIdInformation,
    FileFsMaximumInformation
} FS_INFORMATION_CLASS, *PFS_INFORMATION_CLASS;

typedef enum _KEY_INFORMATION_CLASS {
    KeyBasicInformation,
    KeyNodeInformation,
    KeyFullInformation
} KEY_INFORMATION_CLASS;

typedef enum _KEY_VALUE_INFORMATION_CLASS {
    KeyValueBasicInformation,
    KeyValueFullInformation,
    KeyValuePartialInformation,
    KeyValueFullInformationAlign64,
    KeyValuePartialInformationAlign64
} KEY_VALUE_INFORMATION_CLASS;

typedef enum _OBJECT_INFORMATION_CLASS {
    ObjectBasicInformation,
    ObjectNameInformation,
    ObjectTypeInformation,
    ObjectAllInformation,
    ObjectDataInformation
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
    MaxProcessInfoClass
} PROCESSINFOCLASS;

typedef enum _SECTION_INHERIT {
    ViewShare = 1,
    ViewUnmap = 2
} SECTION_INHERIT;

typedef enum SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation = 0,
    Unknown1,
    SystemPerformanceInformation = 2,
    SystemTimeOfDayInformation = 3, /* was SystemTimeInformation */
    Unknown4,
    SystemProcessInformation = 5,
    Unknown6,
    Unknown7,
    SystemProcessorPerformanceInformation = 8,
    Unknown9,
    Unknown10,
    SystemDriverInformation,
    Unknown12,
    Unknown13,
    Unknown14,
    Unknown15,
    SystemHandleList,
    Unknown17,
    Unknown18,
    Unknown19,
    Unknown20,
    SystemCacheInformation,
    Unknown22,
    SystemInterruptInformation = 23,
    SystemExceptionInformation = 33,
    SystemRegistryQuotaInformation = 37,
    SystemLookasideInformation = 45
} SYSTEM_INFORMATION_CLASS, *PSYSTEM_INFORMATION_CLASS;

typedef enum _TIMER_TYPE {
    NotificationTimer,
    SynchronizationTimer
} TIMER_TYPE;

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
    MaxThreadInfoClass
} THREADINFOCLASS;

typedef struct _THREAD_BASIC_INFORMATION
{
    NTSTATUS  ExitStatus;
    PVOID     TebBaseAddress;
    CLIENT_ID ClientId;
    ULONG     AffinityMask;
    LONG      Priority;
    LONG      BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;


typedef enum _WINSTATIONINFOCLASS {
    WinStationInformation = 8
} WINSTATIONINFOCLASS;

typedef enum
{
    MemoryBasicInformation = 0
} MEMORY_INFORMATION_CLASS;

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
 * IA64 specific types and data structures
 */

#ifdef __ia64__

typedef struct _FRAME_POINTERS {
  ULONGLONG MemoryStackFp;
  ULONGLONG BackingStoreFp;
} FRAME_POINTERS, *PFRAME_POINTERS;

#define UNWIND_HISTORY_TABLE_SIZE 12

typedef struct _RUNTIME_FUNCTION {
  ULONG BeginAddress;
  ULONG EndAddress;
  ULONG UnwindInfoAddress;
} RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;

typedef struct _UNWIND_HISTORY_TABLE_ENTRY {
  ULONG64 ImageBase;
  ULONG64 Gp;
  PRUNTIME_FUNCTION FunctionEntry;
} UNWIND_HISTORY_TABLE_ENTRY, *PUNWIND_HISTORY_TABLE_ENTRY;

typedef struct _UNWIND_HISTORY_TABLE {
  ULONG Count;
  UCHAR Search;
  ULONG64 LowAddress;
  ULONG64 HighAddress;
  UNWIND_HISTORY_TABLE_ENTRY Entry[UNWIND_HISTORY_TABLE_SIZE];
} UNWIND_HISTORY_TABLE, *PUNWIND_HISTORY_TABLE;

#endif /* defined(__ia64__) */

/***********************************************************************
 * Types and data structures
 */

/* This is used by NtQuerySystemInformation */
/* FIXME: Isn't THREAD_INFO and THREADINFO the same structure? */
typedef struct {
    FILETIME    ftKernelTime;
    FILETIME    ftUserTime;
    FILETIME    ftCreateTime;
    DWORD       dwTickCount;
    DWORD       dwStartAddress;
    DWORD       dwOwningPID;
    DWORD       dwThreadID;
    DWORD       dwCurrentPriority;
    DWORD       dwBasePriority;
    DWORD       dwContextSwitches;
    DWORD       dwThreadState;
    DWORD       dwWaitReason;
} THREADINFO, *PTHREADINFO;

/* FIXME: Isn't THREAD_INFO and THREADINFO the same structure? */
typedef struct _THREAD_INFO{
    DWORD Unknown1[6];
    DWORD ThreadID;
    DWORD Unknown2[3];
    DWORD Status;
    DWORD WaitReason;
    DWORD Unknown3[4];
} THREAD_INFO, PTHREAD_INFO;

/***********************************************************************
 * Types and data structures
 */

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

typedef struct _OBJECT_ATTRIBUTES {
  ULONG Length;
  HANDLE RootDirectory;
  PUNICODE_STRING ObjectName;
  ULONG Attributes;
  PVOID SecurityDescriptor;       /* type SECURITY_DESCRIPTOR */
  PVOID SecurityQualityOfService; /* type SECURITY_QUALITY_OF_SERVICE */
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef struct _OBJECT_DATA_INFORMATION {
    BOOLEAN InheritHandle;
    BOOLEAN ProtectFromClose;
} OBJECT_DATA_INFORMATION, *POBJECT_DATA_INFORMATION;

typedef struct _PROCESS_BASIC_INFORMATION {
#ifdef __WINESRC__
    DWORD ExitStatus;
    DWORD PebBaseAddress;
    DWORD AffinityMask;
    DWORD BasePriority;
    ULONG UniqueProcessId;
    ULONG InheritedFromUniqueProcessId;
#else
    PVOID Reserved1;
    PPEB PebBaseAddress;
    PVOID Reserved2[2];
    ULONG_PTR UniqueProcessId;
    PVOID Reserved3;
#endif
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

typedef struct _PROCESS_INFO {
    DWORD    Offset;             /* 00 offset to next PROCESS_INFO ok*/
    DWORD    ThreadCount;        /* 04 number of ThreadInfo member ok */
    DWORD    Unknown1[6];
    FILETIME CreationTime;       /* 20 */
    DWORD    Unknown2[5];
    PWCHAR   ProcessName;        /* 3c ok */
    DWORD    BasePriority;
    DWORD    ProcessID;          /* 44 ok*/
    DWORD    ParentProcessID;
    DWORD    HandleCount;
    DWORD    Unknown3[2];        /* 50 */
    ULONG    PeakVirtualSize;
    ULONG    VirtualSize;
    ULONG    PageFaultCount;
    ULONG    PeakWorkingSetSize;
    ULONG    WorkingSetSize;
    ULONG    QuotaPeakPagedPoolUsage;
    ULONG    QuotaPagedPoolUsage;
    ULONG    QuotaPeakNonPagedPoolUsage;
    ULONG    QuotaNonPagedPoolUsage;
    ULONG    PagefileUsage;
    ULONG    PeakPagefileUsage;
    DWORD    PrivateBytes;
    DWORD    Unknown6[4];
    THREAD_INFO ati[ANYSIZE_ARRAY]; /* 94 size=0x40*/
} PROCESS_INFO, PPROCESS_INFO;

typedef struct _RTL_HEAP_DEFINITION {
    ULONG Length; /* = sizeof(RTL_HEAP_DEFINITION) */

    ULONG Unknown[11];
} RTL_HEAP_DEFINITION, *PRTL_HEAP_DEFINITION;

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
    DWORD dwUnknown1;
    ULONG uKeMaximumIncrement;
    ULONG uPageSize;
    ULONG uMmNumberOfPhysicalPages;
    ULONG uMmLowestPhysicalPage;
    ULONG uMmHighestPhysicalPage;
    ULONG uAllocationGranularity;
    PVOID pLowestUserAddress;
    PVOID pMmHighestUserAddress;
    ULONG uKeActiveProcessors;
    BYTE bKeNumberProcessors;
    BYTE bUnknown2;
    WORD wUnknown3;
#else
    BYTE Reserved1[24];
    PVOID Reserved2[4];
    CCHAR NumberOfProcessors;
#endif
} SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;

/* System Information Class 0x15 */
typedef struct {
    ULONG CurrentSize;
    ULONG PeakSize;
    ULONG PageFaultCount;
    ULONG MinimumWorkingSet;
    ULONG MaximumWorkingSet;
    ULONG unused[4];
} SYSTEM_CACHE_INFORMATION;

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

/* System Information Class 0x0b */
typedef struct {
    PVOID pvAddress;
    DWORD dwUnknown1;
    DWORD dwUnknown2;
    DWORD dwEntryIndex;
    DWORD dwUnknown3;
    char szName[MAX_PATH + 1];
} SYSTEM_DRIVER_INFORMATION;

typedef struct _SYSTEM_EXCEPTION_INFORMATION {
    BYTE Reserved1[16];
} SYSTEM_EXCEPTION_INFORMATION, *PSYSTEM_EXCEPTION_INFORMATION;

typedef struct _SYSTEM_LOOKASIDE_INFORMATION {
    BYTE Reserved1[32];
} SYSTEM_LOOKASIDE_INFORMATION, *PSYSTEM_LOOKASIDE_INFORMATION;

typedef struct _SYSTEM_INTERRUPT_INFORMATION {
    BYTE Reserved1[24];
} SYSTEM_INTERRUPT_INFORMATION, *PSYSTEM_INTERRUPT_INFORMATION;

/* System Information Class 0x10 */
typedef struct {
    USHORT dwPID;
    USHORT dwCreatorBackTraceIndex;
    BYTE bObjectType;
    BYTE bHandleAttributes;
    USHORT usHandleOffset;
    DWORD dwKeObject;
    ULONG ulGrantedAccess;
} HANDLEINFO, *PHANDLEINFO; /* FIXME: SYSTEM_HANDLE_INFORMATION? */

typedef struct _SYSTEM_PERFORMANCE_INFORMATION {
    BYTE Reserved1[312];
} SYSTEM_PERFORMANCE_INFORMATION, *PSYSTEM_PERFORMANCE_INFORMATION;

/* System Information Class 0x02 */
typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION {
#ifdef __WINESRC__
    LARGE_INTEGER liIdleTime;
    DWORD dwSpare[10];
#else
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER Reserved1[2];
    ULONG Reserved2;
#endif
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION, *PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

/* System Information Class 0x05 */
typedef struct _SYSTEM_PROCESS_INFORMATION {
#ifdef __WINESRC__
    DWORD dwOffset;
    DWORD dwThreadCount;
    DWORD dwUnknown1[6];
    FILETIME ftCreationTime;
    DWORD dwUnknown2[5];
    WCHAR *pszProcessName;
    DWORD dwBasePriority;
    DWORD dwProcessID;
    DWORD dwParentProcessID;
    DWORD dwHandleCount;
    DWORD dwUnknown3;
    DWORD dwUnknown4;
    DWORD dwVirtualBytesPeak;
    DWORD dwVirtualBytes;
    DWORD dwPageFaults;
    DWORD dwWorkingSetPeak;
    DWORD dwWorkingSet;
    DWORD dwUnknown5;
    DWORD dwPagedPool;
    DWORD dwUnknown6;
    DWORD dwNonPagedPool;
    DWORD dwPageFileBytesPeak;
    DWORD dwPrivateBytes;
    DWORD dwPageFileBytes;
    DWORD dwUnknown7[4];
    THREADINFO ti[1];
#else
    ULONG NextEntryOffset;
    BYTE Reserved1[52];
    PVOID Reserved2[3];
    HANDLE UniqueProcessId;
    PVOID Reserved3;
    ULONG HandleCount;
    BYTE Reserved4[4];
    PVOID Reserved5[11];
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    LARGE_INTEGER Reserved6[6];
#endif
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

typedef struct _SYSTEM_REGISTRY_QUOTA_INFORMATION {
    ULONG RegistryQuotaAllowed;
    ULONG RegistryQuotaUsed;
    PVOID Reserved1;
} SYSTEM_REGISTRY_QUOTA_INFORMATION, *PSYSTEM_REGISTRY_QUOTA_INFORMATION;

typedef struct _SYSTEM_TIME_ADJUSTMENT {
    ULONG   TimeAdjustment;
    BOOLEAN TimeAdjustmentDisabled;
} SYSTEM_TIME_ADJUSTMENT, *PSYSTEM_TIME_ADJUSTMENT;

/* System Information Class 0x03 */
typedef struct _SYSTEM_TIMEOFDAY_INFORMATION {
#ifdef __WINESRC__
    LARGE_INTEGER liKeBootTime;
    LARGE_INTEGER liKeSystemTime;
    LARGE_INTEGER liExpTimeZoneBias;
    ULONG uCurrentTimeZoneId;
    DWORD dwReserved;
#else
    BYTE Reserved1[48];
#endif
} SYSTEM_TIMEOFDAY_INFORMATION, *PSYSTEM_TIMEOFDAY_INFORMATION; /* was SYSTEM_TIME_INFORMATION */

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

typedef struct _WINSTATIONINFORMATIONW {
  BYTE Reserved2[70];
  ULONG LogonId;
  BYTE Reserved3[1140];
} WINSTATIONINFORMATIONW, *PWINSTATIONINFORMATIONW;

typedef struct _VM_COUNTERS_ {
    ULONG PeakVirtualSize;
    ULONG VirtualSize;
    ULONG PageFaultCount;
    ULONG PeakWorkingSetSize;
    ULONG WorkingSetSize;
    ULONG QuotaPeakPagedPoolUsage;
    ULONG QuotaPagedPoolUsage;
    ULONG QuotaPeakNonPagedPoolUsage;
    ULONG QuotaNonPagedPoolUsage;
    ULONG PagefileUsage;
    ULONG PeakPagefileUsage;
} VM_COUNTERS, *PVM_COUNTERS;

typedef BOOLEAN (WINAPI * PWINSTATIONQUERYINFORMATIONW)(HANDLE,ULONG,WINSTATIONINFOCLASS,PVOID,ULONG,PULONG);

typedef struct _LDR_RESOURCE_INFO
{
    ULONG Type;
    ULONG Name;
    ULONG Language;
} LDR_RESOURCE_INFO, *PLDR_RESOURCE_INFO;

/***********************************************************************
 * Defines
 */

/* flags for NtCreateFile and NtOpenFile */
#define FILE_DIRECTORY_FLAG  0x00000001
#define FILE_WRITE_THROUGH   0x00000002
#define FILE_SEQUENTIAL_ONLY 0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING 0x00000008
#define FILE_SYNCHRONOUS_IO_ALERT    0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT 0x00000020
#define FILE_NON_DIRECTORY_FILE      0x00000040
#define FILE_CREATE_TREE_CONNECTION  0x00000080

/* status for NtCreateFile or NtOpenFile */
#define FILE_SUPERSEDED  0x00000000
#define FILE_OPENED      0x00000001
#define FILE_CREATED     0x00000002
#define FILE_OVERWRITTEN 0x00000003
#define FILE_EXISTS      0x00000004
#define FILE_DOES_NOT_EXIST 0x00000005

#if (_WIN32_WINNT >= 0x0501)
#define INTERNAL_TS_ACTIVE_CONSOLE_ID ( *((volatile ULONG*)(0x7ffe02d8)) )
#endif /* (_WIN32_WINNT >= 0x0501) */

#define LOGONID_CURRENT    ((ULONG)-1)

#define OBJ_INHERIT          0x00000002L
#define OBJ_PERMANENT        0x00000010L
#define OBJ_EXCLUSIVE        0x00000020L
#define OBJ_CASE_INSENSITIVE 0x00000040L
#define OBJ_OPENIF           0x00000080L
#define OBJ_OPENLINK         0x00000100L
#define OBJ_KERNEL_HANDLE    0x00000200L
#define OBJ_VALID_ATTRIBUTES 0x000003F2L

#define SERVERNAME_CURRENT ((HANDLE)NULL)

typedef void (CALLBACK *PNTAPCFUNC)(ULONG_PTR,ULONG_PTR,ULONG_PTR); /* FIXME: not the right name */
typedef void (CALLBACK *PRTL_THREAD_START_ROUTINE)(LPVOID); /* FIXME: not the right name */

/***********************************************************************
 * Function declarations
 */

extern LPSTR _strlwr(LPSTR str); /* FIXME: Doesn't belong here */
extern LPSTR _strupr(LPSTR str); /* FIXME: Doesn't belong here */

#if defined(__i386__) && defined(__GNUC__)
static inline void WINAPI DbgBreakPoint(void) { __asm__ __volatile__("int3"); }
static inline void WINAPI DbgUserBreakPoint(void) { __asm__ __volatile__("int3"); }
#else  /* __i386__ && __GNUC__ */
void WINAPI DbgBreakPoint(void);
void WINAPI DbgUserBreakPoint(void);
#endif  /* __i386__ && __GNUC__ */
ULONG DbgPrint(PCH Format,...);
//void WINAPIV DbgPrint(LPCSTR fmt, ...);

NTSTATUS  WINAPI LdrAccessResource(HMODULE,const IMAGE_RESOURCE_DATA_ENTRY*,void**,PULONG);
NTSTATUS  WINAPI LdrFindResourceDirectory_U(HMODULE,const LDR_RESOURCE_INFO*,ULONG,const IMAGE_RESOURCE_DIRECTORY**);
NTSTATUS  WINAPI LdrFindResource_U(HMODULE,const LDR_RESOURCE_INFO*,ULONG,const IMAGE_RESOURCE_DATA_ENTRY**);
NTSTATUS  WINAPI LdrGetDllHandle(ULONG, ULONG, const UNICODE_STRING*, HMODULE*);
NTSTATUS  WINAPI LdrGetProcedureAddress(HMODULE, const ANSI_STRING*, ULONG, void**);
void      WINAPI LdrInitializeThunk(HANDLE,LPVOID,ULONG,ULONG);
NTSTATUS  WINAPI LdrLoadDll(LPCWSTR, DWORD, const UNICODE_STRING*, HMODULE*);
void      WINAPI LdrShutdownProcess(void);
void      WINAPI LdrShutdownThread(void);
NTSTATUS  WINAPI NtAccessCheck(PSECURITY_DESCRIPTOR,HANDLE,ACCESS_MASK,PGENERIC_MAPPING,PPRIVILEGE_SET,PULONG,PULONG,PBOOLEAN);
NTSTATUS  WINAPI NtAdjustPrivilegesToken(HANDLE,BOOLEAN,PTOKEN_PRIVILEGES,DWORD,PTOKEN_PRIVILEGES,PDWORD);
NTSTATUS  WINAPI NtAllocateVirtualMemory(HANDLE,PVOID*,PVOID,ULONG*,ULONG,ULONG);
NTSTATUS  WINAPI NtCancelTimer(HANDLE, BOOLEAN*);
NTSTATUS  WINAPI NtClearEvent(HANDLE);
NTSTATUS  WINAPI NtClose(HANDLE);
NTSTATUS  WINAPI NtCreateEvent(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES *,BOOLEAN,BOOLEAN);
NTSTATUS  WINAPI NtCreateFile(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,PLARGE_INTEGER,ULONG,ULONG,ULONG,ULONG,PVOID,ULONG);
NTSTATUS  WINAPI NtCreateKey(PHKEY,ACCESS_MASK,const OBJECT_ATTRIBUTES*,ULONG,const UNICODE_STRING*,ULONG,PULONG);
NTSTATUS  WINAPI NtCreateSection(HANDLE*,ACCESS_MASK,const OBJECT_ATTRIBUTES*,const LARGE_INTEGER*,ULONG,ULONG,HANDLE);
NTSTATUS  WINAPI NtCreateSemaphore(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*,ULONG,ULONG);
NTSTATUS  WINAPI NtCreateTimer(HANDLE*, ACCESS_MASK, const OBJECT_ATTRIBUTES*, TIMER_TYPE);
NTSTATUS  WINAPI NtDelayExecution(BOOLEAN,const LARGE_INTEGER*);
NTSTATUS  WINAPI NtDeleteKey(HKEY);
NTSTATUS  WINAPI NtDeleteValueKey(HKEY,const UNICODE_STRING *);
NTSTATUS  WINAPI NtDeviceIoControlFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,ULONG,PVOID,ULONG,PVOID,ULONG);
NTSTATUS  WINAPI NtDuplicateObject(HANDLE,HANDLE,HANDLE,PHANDLE,ACCESS_MASK,ULONG,ULONG);
NTSTATUS  WINAPI NtEnumerateKey(HKEY,ULONG,KEY_INFORMATION_CLASS,void *,DWORD,DWORD *);
NTSTATUS  WINAPI NtEnumerateValueKey(HKEY,ULONG,KEY_VALUE_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS  WINAPI NtFlushBuffersFile(HANDLE,IO_STATUS_BLOCK*);
NTSTATUS  WINAPI NtFlushKey(HKEY);
NTSTATUS  WINAPI NtFlushVirtualMemory(HANDLE,LPCVOID*,ULONG*,ULONG);
NTSTATUS  WINAPI NtFreeVirtualMemory(HANDLE,PVOID*,ULONG*,ULONG);
NTSTATUS  WINAPI NtGetContextThread(HANDLE,CONTEXT*);
NTSTATUS  WINAPI NtLoadKey(const OBJECT_ATTRIBUTES *,const OBJECT_ATTRIBUTES *);
NTSTATUS  WINAPI NtLockFile(HANDLE,HANDLE,PIO_APC_ROUTINE,void*,PIO_STATUS_BLOCK,PLARGE_INTEGER,PLARGE_INTEGER,ULONG*,BOOLEAN,BOOLEAN);
NTSTATUS  WINAPI NtLockVirtualMemory(HANDLE,PVOID*,ULONG*,ULONG);
NTSTATUS  WINAPI NtMapViewOfSection(HANDLE,HANDLE,PVOID*,ULONG,ULONG,const LARGE_INTEGER*,ULONG*,SECTION_INHERIT,ULONG,ULONG);
NTSTATUS  WINAPI NtNotifyChangeKey(HKEY,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,ULONG,BOOLEAN,PVOID,ULONG,BOOLEAN);
NTSTATUS  WINAPI NtOpenEvent(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES *);
NTSTATUS  WINAPI NtOpenFile(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,ULONG,ULONG);
NTSTATUS  WINAPI NtOpenKey(PHKEY,ACCESS_MASK,const OBJECT_ATTRIBUTES *);
NTSTATUS  WINAPI NtOpenProcessToken(HANDLE,DWORD,HANDLE *);
NTSTATUS  WINAPI NtOpenSection(HANDLE*,ACCESS_MASK,const OBJECT_ATTRIBUTES*);
NTSTATUS  WINAPI NtOpenThread(HANDLE*,ACCESS_MASK,const OBJECT_ATTRIBUTES*,const CLIENT_ID*);
NTSTATUS  WINAPI NtOpenThreadToken(HANDLE,DWORD,BOOLEAN,HANDLE *);
NTSTATUS  WINAPI NtOpenTimer(HANDLE*, ACCESS_MASK, const OBJECT_ATTRIBUTES*);
NTSTATUS  WINAPI NtProtectVirtualMemory(HANDLE,PVOID*,ULONG*,ULONG,ULONG*);
NTSTATUS  WINAPI NtPulseEvent(HANDLE,PULONG);
NTSTATUS  WINAPI NtQueueApcThread(HANDLE,PNTAPCFUNC,ULONG_PTR,ULONG_PTR,ULONG_PTR);
NTSTATUS  WINAPI NtQueryDefaultLocale(BOOLEAN,LCID*);
NTSTATUS  WINAPI NtQueryInformationFile(HANDLE,PIO_STATUS_BLOCK,PVOID,LONG,FILE_INFORMATION_CLASS);
NTSTATUS  WINAPI NtQueryInformationProcess(HANDLE,PROCESSINFOCLASS,PVOID,ULONG,PULONG);
NTSTATUS  WINAPI NtQueryInformationThread(HANDLE,THREADINFOCLASS,PVOID,ULONG,PULONG);
NTSTATUS  WINAPI NtQueryInformationToken(HANDLE,DWORD,LPVOID,DWORD,LPDWORD);
NTSTATUS  WINAPI NtQueryKey(HKEY,KEY_INFORMATION_CLASS,void *,DWORD,DWORD *);
NTSTATUS  WINAPI NtQueryMultipleValueKey(HKEY,PVALENTW,ULONG,PVOID,ULONG,PULONG);
NTSTATUS  WINAPI NtQueryObject(HANDLE, OBJECT_INFORMATION_CLASS, PVOID, ULONG, PULONG);
NTSTATUS  WINAPI NtQuerySecurityObject(HANDLE,SECURITY_INFORMATION,PSECURITY_DESCRIPTOR,ULONG,PULONG);
NTSTATUS  WINAPI NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS  WINAPI NtQuerySystemTime(PLARGE_INTEGER);
NTSTATUS  WINAPI NtQueryValueKey(HKEY,const UNICODE_STRING *,KEY_VALUE_INFORMATION_CLASS,void *,DWORD,DWORD *);
NTSTATUS  WINAPI NtQueryVirtualMemory(HANDLE,LPCVOID,MEMORY_INFORMATION_CLASS,PVOID,ULONG,ULONG*);
void      WINAPI NtRaiseException(PEXCEPTION_RECORD,PCONTEXT,BOOL);
NTSTATUS  WINAPI NtReadFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,PVOID,ULONG,PLARGE_INTEGER,PULONG);
NTSTATUS  WINAPI NtReadVirtualMemory(HANDLE,const void*,void*,SIZE_T,SIZE_T*);
NTSTATUS  WINAPI NtReleaseSemaphore(HANDLE,ULONG,PULONG);
NTSTATUS  WINAPI NtReplaceKey(POBJECT_ATTRIBUTES,HKEY,POBJECT_ATTRIBUTES);
NTSTATUS  WINAPI NtResetEvent(HANDLE,PULONG);
NTSTATUS  WINAPI NtRestoreKey(HKEY,HANDLE,ULONG);
NTSTATUS  WINAPI NtResumeThread(HANDLE,PULONG);
NTSTATUS  WINAPI NtSaveKey(HKEY,HANDLE);
NTSTATUS  WINAPI NtSetContextThread(HANDLE,const CONTEXT*);
NTSTATUS  WINAPI NtSetDefaultLocale(BOOLEAN,LCID);
NTSTATUS  WINAPI NtSetEvent(HANDLE,PULONG);
NTSTATUS  WINAPI NtSetInformationFile(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,FILE_INFORMATION_CLASS);
NTSTATUS  WINAPI NtSetInformationKey(HKEY,const int,PVOID,ULONG);
NTSTATUS  WINAPI NtSetInformationObject(HANDLE, OBJECT_INFORMATION_CLASS, PVOID, ULONG);
NTSTATUS  WINAPI NtSetInformationThread(HANDLE,THREADINFOCLASS,LPCVOID,ULONG);
NTSTATUS  WINAPI NtSetSecurityObject(HANDLE,SECURITY_INFORMATION,PSECURITY_DESCRIPTOR);
NTSTATUS  WINAPI NtSetSystemTime(const LARGE_INTEGER*,LARGE_INTEGER*);
NTSTATUS  WINAPI NtSetTimer(HANDLE, const LARGE_INTEGER*, PTIMERAPCROUTINE, PVOID, BOOLEAN, ULONG, BOOLEAN*);
NTSTATUS  WINAPI NtSetValueKey(HKEY,const UNICODE_STRING *,ULONG,ULONG,const void *,ULONG);
NTSTATUS  WINAPI NtSuspendThread(HANDLE,PULONG);
NTSTATUS  WINAPI NtTerminateProcess(HANDLE,LONG);
NTSTATUS  WINAPI NtTerminateThread(HANDLE,LONG);
NTSTATUS  WINAPI NtUnloadKey(HKEY);
NTSTATUS  WINAPI NtUnlockFile(HANDLE,PIO_STATUS_BLOCK,PLARGE_INTEGER,PLARGE_INTEGER,PULONG);
NTSTATUS  WINAPI NtUnlockVirtualMemory(HANDLE,PVOID*,ULONG*,ULONG);
NTSTATUS  WINAPI NtUnmapViewOfSection(HANDLE,PVOID);
NTSTATUS  WINAPI NtWaitForSingleObject(HANDLE,BOOLEAN,const LARGE_INTEGER*);
NTSTATUS  WINAPI NtWaitForMultipleObjects(ULONG,const HANDLE*,BOOLEAN,BOOLEAN,const LARGE_INTEGER*);
NTSTATUS  WINAPI NtWriteFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,const void*,ULONG,PLARGE_INTEGER,PULONG);
NTSTATUS  WINAPI NtWriteVirtualMemory(HANDLE,void*,const void*,SIZE_T,SIZE_T*);

void      WINAPI RtlAcquirePebLock(void);
BYTE      WINAPI RtlAcquireResourceExclusive(LPRTL_RWLOCK,BYTE);
BYTE      WINAPI RtlAcquireResourceShared(LPRTL_RWLOCK,BYTE);
NTSTATUS  WINAPI RtlAddAce(PACL,DWORD,DWORD,PACE_HEADER,DWORD);
NTSTATUS  WINAPI RtlAddAccessAllowedAce(PACL,DWORD,DWORD,PSID);
NTSTATUS  WINAPI RtlAddAccessAllowedAceEx(PACL,DWORD,DWORD,DWORD,PSID);
NTSTATUS  WINAPI RtlAddAccessDeniedAce(PACL,DWORD,DWORD,PSID);
NTSTATUS  WINAPI RtlAddAccessDeniedAceEx(PACL,DWORD,DWORD,DWORD,PSID);
PVOID     WINAPI RtlAddVectoredExceptionHandler(ULONG,PVECTORED_EXCEPTION_HANDLER);
DWORD     WINAPI RtlAdjustPrivilege(DWORD,DWORD,DWORD,DWORD);
BOOLEAN   WINAPI RtlAllocateAndInitializeSid(PSID_IDENTIFIER_AUTHORITY,BYTE,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,PSID *);
PVOID     WINAPI RtlAllocateHeap(HANDLE,ULONG,ULONG);
DWORD     WINAPI RtlAnsiStringToUnicodeSize(const STRING *);
NTSTATUS  WINAPI RtlAnsiStringToUnicodeString(PUNICODE_STRING,PCANSI_STRING,BOOLEAN);
NTSTATUS  WINAPI RtlAppendAsciizToString(STRING *,LPCSTR);
NTSTATUS  WINAPI RtlAppendStringToString(STRING *,const STRING *);
NTSTATUS  WINAPI RtlAppendUnicodeStringToString(UNICODE_STRING *,const UNICODE_STRING *);
NTSTATUS  WINAPI RtlAppendUnicodeToString(UNICODE_STRING *,LPCWSTR);
BOOLEAN   WINAPI RtlAreAllAccessesGranted(ACCESS_MASK,ACCESS_MASK);
BOOLEAN   WINAPI RtlAreAnyAccessesGranted(ACCESS_MASK,ACCESS_MASK);
BOOLEAN   WINAPI RtlAreBitsSet(PCRTL_BITMAP,ULONG,ULONG);
BOOLEAN   WINAPI RtlAreBitsClear(PCRTL_BITMAP,ULONG,ULONG);

NTSTATUS  WINAPI RtlCharToInteger(PCSZ,ULONG,PULONG);
void      WINAPI RtlClearAllBits(PRTL_BITMAP);
void      WINAPI RtlClearBits(PRTL_BITMAP,ULONG,ULONG);
ULONG     WINAPI RtlCompactHeap(HANDLE,ULONG);
LONG      WINAPI RtlCompareString(const STRING*,const STRING*,BOOLEAN);
LONG      WINAPI RtlCompareUnicodeString(const UNICODE_STRING*,const UNICODE_STRING*,BOOLEAN);
DWORD     WINAPI RtlComputeCrc32(DWORD,PBYTE,INT);
NTSTATUS  WINAPI RtlConvertSidToUnicodeString(PUNICODE_STRING,PSID,BOOLEAN);
LONGLONG  WINAPI RtlConvertLongToLargeInteger(LONG);
ULONGLONG WINAPI RtlConvertUlongToLargeInteger(ULONG);
void      WINAPI RtlCopyLuid(PLUID,const LUID*);
void      WINAPI RtlCopyLuidAndAttributesArray(ULONG,const LUID_AND_ATTRIBUTES*,PLUID_AND_ATTRIBUTES);
DWORD     WINAPI RtlCopySid(DWORD,PSID,PSID);
void      WINAPI RtlCopyString(STRING*,const STRING*);
void      WINAPI RtlCopyUnicodeString(UNICODE_STRING*,const UNICODE_STRING*);
NTSTATUS  WINAPI RtlCreateAcl(PACL,DWORD,DWORD);
NTSTATUS  WINAPI RtlCreateEnvironment(BOOLEAN, PWSTR*);
HANDLE    WINAPI RtlCreateHeap(ULONG,PVOID,ULONG,ULONG,PVOID,PRTL_HEAP_DEFINITION);
NTSTATUS  WINAPI RtlCreateProcessParameters(RTL_USER_PROCESS_PARAMETERS**,const UNICODE_STRING*,
                                            const UNICODE_STRING*,const UNICODE_STRING*,
                                            const UNICODE_STRING*,PWSTR,const UNICODE_STRING*,
                                            const UNICODE_STRING*,const UNICODE_STRING*,
                                            const UNICODE_STRING*);
NTSTATUS  WINAPI RtlCreateSecurityDescriptor(PSECURITY_DESCRIPTOR,DWORD);
BOOLEAN   WINAPI RtlCreateUnicodeString(PUNICODE_STRING,LPCWSTR);
BOOLEAN   WINAPI RtlCreateUnicodeStringFromAsciiz(PUNICODE_STRING,LPCSTR);
NTSTATUS  WINAPI RtlCreateUserThread(HANDLE,const SECURITY_DESCRIPTOR*,BOOLEAN,PVOID,SIZE_T,SIZE_T,PRTL_THREAD_START_ROUTINE,void*,HANDLE*,CLIENT_ID*);

NTSTATUS  WINAPI RtlDeleteCriticalSection(RTL_CRITICAL_SECTION *);
void      WINAPI RtlDeleteResource(LPRTL_RWLOCK);
DWORD     WINAPI RtlDeleteSecurityObject(DWORD);
PRTL_USER_PROCESS_PARAMETERS WINAPI RtlDeNormalizeProcessParams(RTL_USER_PROCESS_PARAMETERS*);
NTSTATUS  WINAPI RtlDestroyEnvironment(PWSTR);
HANDLE    WINAPI RtlDestroyHeap(HANDLE);
void      WINAPI RtlDestroyProcessParameters(RTL_USER_PROCESS_PARAMETERS*);
DOS_PATHNAME_TYPE WINAPI RtlDetermineDosPathNameType_U(PCWSTR);
BOOLEAN   WINAPI RtlDoesFileExists_U(LPCWSTR);
BOOLEAN   WINAPI RtlDosPathNameToNtPathName_U(LPWSTR,PUNICODE_STRING,PWSTR*,CURDIR*);
ULONG     WINAPI RtlDosSearchPath_U(LPCWSTR, LPCWSTR, LPCWSTR, ULONG, LPWSTR, LPWSTR*);
WCHAR     WINAPI RtlDowncaseUnicodeChar(WCHAR);
NTSTATUS  WINAPI RtlDowncaseUnicodeString(UNICODE_STRING*,const UNICODE_STRING*,BOOLEAN);
void      WINAPI RtlDumpResource(LPRTL_RWLOCK);
NTSTATUS  WINAPI RtlDuplicateUnicodeString(int,const UNICODE_STRING*,UNICODE_STRING*);

LONGLONG  WINAPI RtlEnlargedIntegerMultiply(INT,INT);
ULONGLONG WINAPI RtlEnlargedUnsignedMultiply(UINT,UINT);
UINT      WINAPI RtlEnlargedUnsignedDivide(ULONGLONG,UINT,UINT *);
NTSTATUS  WINAPI RtlEnterCriticalSection(RTL_CRITICAL_SECTION *);
void      WINAPI RtlEraseUnicodeString(UNICODE_STRING*);
NTSTATUS  WINAPI RtlEqualComputerName(const UNICODE_STRING*,const UNICODE_STRING*);
NTSTATUS  WINAPI RtlEqualDomainName(const UNICODE_STRING*,const UNICODE_STRING*);
BOOLEAN   WINAPI RtlEqualLuid(const LUID*,const LUID*);
BOOL      WINAPI RtlEqualPrefixSid(PSID,PSID);
BOOL      WINAPI RtlEqualSid(PSID,PSID);
BOOLEAN   WINAPI RtlEqualString(const STRING*,const STRING*,BOOLEAN);
BOOLEAN   WINAPI RtlEqualUnicodeString(const UNICODE_STRING*,const UNICODE_STRING*,BOOLEAN);
NTSTATUS  WINAPI RtlExpandEnvironmentStrings_U(PWSTR, const UNICODE_STRING*, UNICODE_STRING*, ULONG*);
LONGLONG  WINAPI RtlExtendedMagicDivide(LONGLONG,LONGLONG,INT);
LONGLONG  WINAPI RtlExtendedIntegerMultiply(LONGLONG,INT);
LONGLONG  WINAPI RtlExtendedLargeIntegerDivide(LONGLONG,INT,INT *);

NTSTATUS  WINAPI RtlFindCharInUnicodeString(int,const UNICODE_STRING*,const UNICODE_STRING*,USHORT*);
ULONG     WINAPI RtlFindClearBits(PCRTL_BITMAP,ULONG,ULONG);
ULONG     WINAPI RtlFindClearBitsAndSet(PRTL_BITMAP,ULONG,ULONG);
ULONG     WINAPI RtlFindClearRuns(PCRTL_BITMAP,PRTL_BITMAP_RUN,ULONG,BOOLEAN);
ULONG     WINAPI RtlFindLastBackwardRunSet(PCRTL_BITMAP,ULONG,PULONG);
ULONG     WINAPI RtlFindLastBackwardRunClear(PCRTL_BITMAP,ULONG,PULONG);
CCHAR     WINAPI RtlFindLeastSignificantBit(ULONGLONG);
ULONG     WINAPI RtlFindLongestRunSet(PCRTL_BITMAP,PULONG);
ULONG     WINAPI RtlFindLongestRunClear(PCRTL_BITMAP,PULONG);
NTSTATUS  WINAPI RtlFindMessage(HMODULE,ULONG,ULONG,ULONG,const MESSAGE_RESOURCE_ENTRY**);
CCHAR     WINAPI RtlFindMostSignificantBit(ULONGLONG);
ULONG     WINAPI RtlFindNextForwardRunSet(PCRTL_BITMAP,ULONG,PULONG);
ULONG     WINAPI RtlFindNextForwardRunClear(PCRTL_BITMAP,ULONG,PULONG);
ULONG     WINAPI RtlFindSetBits(PCRTL_BITMAP,ULONG,ULONG);
ULONG     WINAPI RtlFindSetBitsAndClear(PRTL_BITMAP,ULONG,ULONG);
ULONG     WINAPI RtlFindSetRuns(PCRTL_BITMAP,PRTL_BITMAP_RUN,ULONG,BOOLEAN);
BOOLEAN   WINAPI RtlFirstFreeAce(PACL,PACE_HEADER *);
NTSTATUS  WINAPI RtlFormatCurrentUserKeyPath(PUNICODE_STRING);
void      WINAPI RtlFreeAnsiString(PANSI_STRING);
BOOLEAN   WINAPI RtlFreeHeap(HANDLE,ULONG,PVOID);
void      WINAPI RtlFreeOemString(POEM_STRING);
DWORD     WINAPI RtlFreeSid(PSID);
void      WINAPI RtlFreeUnicodeString(PUNICODE_STRING);

DWORD     WINAPI RtlGetAce(PACL,DWORD,LPVOID *);
NTSTATUS  WINAPI RtlGetControlSecurityDescriptor(PSECURITY_DESCRIPTOR, PSECURITY_DESCRIPTOR_CONTROL,LPDWORD);
NTSTATUS  WINAPI RtlGetCurrentDirectory_U(ULONG, LPWSTR);
NTSTATUS  WINAPI RtlGetDaclSecurityDescriptor(PSECURITY_DESCRIPTOR,PBOOLEAN,PACL *,PBOOLEAN);
ULONG     WINAPI RtlGetFullPathName_U(PCWSTR,ULONG,PWSTR,PWSTR*);
NTSTATUS  WINAPI RtlGetGroupSecurityDescriptor(PSECURITY_DESCRIPTOR,PSID *,PBOOLEAN);
DWORD     WINAPI RtlGetLongestNtPathLength(void);
BOOLEAN   WINAPI RtlGetNtProductType(LPDWORD);
NTSTATUS  WINAPI RtlGetOwnerSecurityDescriptor(PSECURITY_DESCRIPTOR,PSID *,PBOOLEAN);
ULONG     WINAPI RtlGetProcessHeaps(ULONG,HANDLE*);
NTSTATUS  WINAPI RtlGetSaclSecurityDescriptor(PSECURITY_DESCRIPTOR,PBOOLEAN,PACL *,PBOOLEAN);
//NTSTATUS  WINAPI RtlGetVersion(RTL_OSVERSIONINFOEXW*);

PSID_IDENTIFIER_AUTHORITY WINAPI RtlIdentifierAuthoritySid(PSID);
PVOID     WINAPI RtlImageDirectoryEntryToData(HMODULE,BOOL,WORD,ULONG *);
PIMAGE_NT_HEADERS WINAPI RtlImageNtHeader(HMODULE);
PIMAGE_SECTION_HEADER WINAPI RtlImageRvaToSection(const IMAGE_NT_HEADERS *,HMODULE,DWORD);
PVOID     WINAPI RtlImageRvaToVa(const IMAGE_NT_HEADERS *,HMODULE,DWORD,IMAGE_SECTION_HEADER **);
BOOL      WINAPI RtlImpersonateSelf(SECURITY_IMPERSONATION_LEVEL);
void      WINAPI RtlInitString(PSTRING,PCSZ);
void      WINAPI RtlInitAnsiString(PANSI_STRING,PCSZ);
void      WINAPI RtlInitUnicodeString(PUNICODE_STRING,PCWSTR);
NTSTATUS  WINAPI RtlInitUnicodeStringEx(PUNICODE_STRING,PCWSTR);
NTSTATUS  WINAPI RtlInitializeCriticalSection(RTL_CRITICAL_SECTION *);
NTSTATUS  WINAPI RtlInitializeCriticalSectionAndSpinCount(RTL_CRITICAL_SECTION *,DWORD);
void      WINAPI RtlInitializeBitMap(PRTL_BITMAP,LPBYTE,ULONG);
void      WINAPI RtlInitializeResource(LPRTL_RWLOCK);
BOOL      WINAPI RtlInitializeSid(PSID,PSID_IDENTIFIER_AUTHORITY,BYTE);

NTSTATUS  WINAPI RtlInt64ToUnicodeString(ULONGLONG,ULONG,UNICODE_STRING *);
NTSTATUS  WINAPI RtlIntegerToChar(ULONG,ULONG,ULONG,PCHAR);
NTSTATUS  WINAPI RtlIntegerToUnicodeString(ULONG,ULONG,UNICODE_STRING *);
ULONG     WINAPI RtlIsDosDeviceName_U(PCWSTR);
BOOLEAN   WINAPI RtlIsNameLegalDOS8Dot3(const UNICODE_STRING*,POEM_STRING,PBOOLEAN);
DWORD     WINAPI RtlIsTextUnicode(LPVOID,DWORD,DWORD *);

LONGLONG  WINAPI RtlLargeIntegerAdd(LONGLONG,LONGLONG);
LONGLONG  WINAPI RtlLargeIntegerArithmeticShift(LONGLONG,INT);
ULONGLONG WINAPI RtlLargeIntegerDivide( ULONGLONG,ULONGLONG,ULONGLONG *);
LONGLONG  WINAPI RtlLargeIntegerNegate(LONGLONG);
LONGLONG  WINAPI RtlLargeIntegerShiftLeft(LONGLONG,INT);
LONGLONG  WINAPI RtlLargeIntegerShiftRight(LONGLONG,INT);
LONGLONG  WINAPI RtlLargeIntegerSubtract(LONGLONG,LONGLONG);
NTSTATUS  WINAPI RtlLargeIntegerToChar(const ULONGLONG *,ULONG,ULONG,PCHAR);
NTSTATUS  WINAPI RtlLeaveCriticalSection(RTL_CRITICAL_SECTION *);
DWORD     WINAPI RtlLengthRequiredSid(DWORD);
ULONG     WINAPI RtlLengthSecurityDescriptor(PSECURITY_DESCRIPTOR);
DWORD     WINAPI RtlLengthSid(PSID);
NTSTATUS  WINAPI RtlLocalTimeToSystemTime(const LARGE_INTEGER*,PLARGE_INTEGER);
BOOLEAN   WINAPI RtlLockHeap(HANDLE);

NTSTATUS  WINAPI RtlMakeSelfRelativeSD(PSECURITY_DESCRIPTOR,PSECURITY_DESCRIPTOR,LPDWORD);
void      WINAPI RtlMapGenericMask(PACCESS_MASK,const GENERIC_MAPPING*);
NTSTATUS  WINAPI RtlMultiByteToUnicodeN(LPWSTR,DWORD,LPDWORD,LPCSTR,DWORD);
NTSTATUS  WINAPI RtlMultiByteToUnicodeSize(DWORD*,LPCSTR,UINT);

DWORD     WINAPI RtlNewSecurityObject(DWORD,DWORD,DWORD,DWORD,DWORD,DWORD);
PRTL_USER_PROCESS_PARAMETERS WINAPI RtlNormalizeProcessParams(RTL_USER_PROCESS_PARAMETERS*);
ULONG     WINAPI RtlNtStatusToDosError(NTSTATUS);
ULONG     WINAPI RtlNumberOfSetBits(PCRTL_BITMAP);
ULONG     WINAPI RtlNumberOfClearBits(PCRTL_BITMAP);

UINT      WINAPI RtlOemStringToUnicodeSize(const STRING*);
NTSTATUS  WINAPI RtlOemStringToUnicodeString(UNICODE_STRING*,const STRING*,BOOLEAN);
NTSTATUS  WINAPI RtlOemToUnicodeN(LPWSTR,DWORD,LPDWORD,LPCSTR,DWORD);
DWORD     WINAPI RtlOpenCurrentUser(ACCESS_MASK,PHKEY);

BOOLEAN   WINAPI RtlPrefixString(const STRING*,const STRING*,BOOLEAN);
BOOLEAN   WINAPI RtlPrefixUnicodeString(const UNICODE_STRING*,const UNICODE_STRING*,BOOLEAN);

NTSTATUS  WINAPI RtlQueryEnvironmentVariable_U(PWSTR,PUNICODE_STRING,PUNICODE_STRING);
NTSTATUS  WINAPI RtlQueryTimeZoneInformation(LPTIME_ZONE_INFORMATION);

void      WINAPI RtlRaiseException(PEXCEPTION_RECORD);
void      WINAPI RtlRaiseStatus(NTSTATUS);
ULONG     WINAPI RtlRandom(PULONG);
PVOID     WINAPI RtlReAllocateHeap(HANDLE,ULONG,PVOID,ULONG);
void      WINAPI RtlReleasePebLock(void);
void      WINAPI RtlReleaseResource(LPRTL_RWLOCK);
ULONG     WINAPI RtlRemoveVectoredExceptionHandler(PVOID);

void      WINAPI RtlSecondsSince1970ToTime(DWORD,LARGE_INTEGER *);
void      WINAPI RtlSecondsSince1980ToTime(DWORD,LARGE_INTEGER *);
void      WINAPI RtlSetAllBits(PRTL_BITMAP);
void      WINAPI RtlSetBits(PRTL_BITMAP,ULONG,ULONG);
NTSTATUS  WINAPI RtlSetCurrentDirectory_U(const UNICODE_STRING*);
void      WINAPI RtlSetCurrentEnvironment(PWSTR, PWSTR*);
NTSTATUS  WINAPI RtlSetDaclSecurityDescriptor(PSECURITY_DESCRIPTOR,BOOLEAN,PACL,BOOLEAN);
NTSTATUS  WINAPI RtlSetEnvironmentVariable(PWSTR*,PUNICODE_STRING,PUNICODE_STRING);
NTSTATUS  WINAPI RtlSetOwnerSecurityDescriptor(PSECURITY_DESCRIPTOR,PSID,BOOLEAN);
NTSTATUS  WINAPI RtlSetGroupSecurityDescriptor(PSECURITY_DESCRIPTOR,PSID,BOOLEAN);
NTSTATUS  WINAPI RtlSetSaclSecurityDescriptor(PSECURITY_DESCRIPTOR,BOOLEAN,PACL,BOOLEAN);
NTSTATUS  WINAPI RtlSetTimeZoneInformation(const TIME_ZONE_INFORMATION*);
ULONG     WINAPI RtlSizeHeap(HANDLE,ULONG,PVOID);
LPDWORD   WINAPI RtlSubAuthoritySid(PSID,DWORD);
LPBYTE    WINAPI RtlSubAuthorityCountSid(PSID);
NTSTATUS  WINAPI RtlSystemTimeToLocalTime(const LARGE_INTEGER*,PLARGE_INTEGER);

void      WINAPI RtlTimeToTimeFields(const LARGE_INTEGER*,PTIME_FIELDS);
BOOLEAN   WINAPI RtlTimeFieldsToTime(PTIME_FIELDS,PLARGE_INTEGER);
void      WINAPI RtlTimeToElapsedTimeFields(const LARGE_INTEGER *,PTIME_FIELDS);
BOOLEAN   WINAPI RtlTimeToSecondsSince1970(const LARGE_INTEGER *,LPDWORD);
BOOLEAN   WINAPI RtlTimeToSecondsSince1980(const LARGE_INTEGER *,LPDWORD);
BOOL      WINAPI RtlTryEnterCriticalSection(RTL_CRITICAL_SECTION *);

ULONGLONG __cdecl RtlUlonglongByteSwap(ULONGLONG);
DWORD     WINAPI RtlUnicodeStringToAnsiSize(const UNICODE_STRING*);
NTSTATUS  WINAPI RtlUnicodeStringToAnsiString(PANSI_STRING,PCUNICODE_STRING,BOOLEAN);
NTSTATUS  WINAPI RtlUnicodeStringToInteger(const UNICODE_STRING *,ULONG,ULONG *);
DWORD     WINAPI RtlUnicodeStringToOemSize(const UNICODE_STRING*);
NTSTATUS  WINAPI RtlUnicodeStringToOemString(POEM_STRING,PCUNICODE_STRING,BOOLEAN);
NTSTATUS  WINAPI RtlUnicodeToMultiByteN(LPSTR,DWORD,LPDWORD,LPCWSTR,DWORD);
NTSTATUS  WINAPI RtlUnicodeToMultiByteSize(PULONG,PCWSTR,ULONG);
NTSTATUS  WINAPI RtlUnicodeToOemN(LPSTR,DWORD,LPDWORD,LPCWSTR,DWORD);
ULONG     WINAPI RtlUniform(PULONG);
BOOLEAN   WINAPI RtlUnlockHeap(HANDLE);
void      WINAPI RtlUnwind(PVOID,PVOID,PEXCEPTION_RECORD,PVOID);
#ifdef __ia64__
void      WINAPI RtlUnwind2(FRAME_POINTERS,PVOID,PEXCEPTION_RECORD,PVOID,PCONTEXT);
void      WINAPI RtlUnwindEx(FRAME_POINTERS,PVOID,PEXCEPTION_RECORD,PVOID,PCONTEXT,PUNWIND_HISTORY_TABLE);
#endif
WCHAR     WINAPI RtlUpcaseUnicodeChar(WCHAR);
NTSTATUS  WINAPI RtlUpcaseUnicodeString(UNICODE_STRING*,const UNICODE_STRING *,BOOLEAN);
NTSTATUS  WINAPI RtlUpcaseUnicodeStringToAnsiString(STRING*,const UNICODE_STRING*,BOOLEAN);
NTSTATUS  WINAPI RtlUpcaseUnicodeStringToCountedOemString(STRING*,const UNICODE_STRING*,BOOLEAN);
NTSTATUS  WINAPI RtlUpcaseUnicodeStringToOemString(STRING*,const UNICODE_STRING*,BOOLEAN);
NTSTATUS  WINAPI RtlUpcaseUnicodeToMultiByteN(LPSTR,DWORD,LPDWORD,LPCWSTR,DWORD);
NTSTATUS  WINAPI RtlUpcaseUnicodeToOemN(LPSTR,DWORD,LPDWORD,LPCWSTR,DWORD);
CHAR      WINAPI RtlUpperChar(CHAR);
void      WINAPI RtlUpperString(STRING *,const STRING *);

NTSTATUS  WINAPI RtlValidSecurityDescriptor(PSECURITY_DESCRIPTOR);
BOOLEAN   WINAPI RtlValidAcl(PACL);
BOOLEAN   WINAPI RtlValidSid(PSID);
BOOLEAN   WINAPI RtlValidateHeap(HANDLE,ULONG,LPCVOID);
//NTSTATUS  WINAPI RtlVerifyVersionInfo(const RTL_OSVERSIONINFOEXW*,DWORD,DWORDLONG);

NTSTATUS  WINAPI RtlWalkHeap(HANDLE,PVOID);

NTSTATUS  WINAPI RtlpWaitForCriticalSection(RTL_CRITICAL_SECTION *);
NTSTATUS  WINAPI RtlpUnWaitCriticalSection(RTL_CRITICAL_SECTION *);

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

#define NtCurrentProcess() ((HANDLE)-1)

//#define RtlFillMemory(Destination,Length,Fill) memset((Destination),(Fill),(Length))
//#define RtlMoveMemory(Destination,Source,Length) memmove((Destination),(Source),(Length))
#define RtlStoreUlong(p,v)  do { ULONG _v = (v); memcpy((p), &_v, sizeof(_v)); } while (0)
#define RtlStoreUlonglong(p,v) do { ULONGLONG _v = (v); memcpy((p), &_v, sizeof(_v)); } while (0)
#define RtlRetrieveUlong(p,s) memcpy((p), (s), sizeof(ULONG))
#define RtlRetrieveUlonglong(p,s) memcpy((p), (s), sizeof(ULONGLONG))
//#define RtlZeroMemory(Destination,Length) memset((Destination),0,(Length))

inline static BOOLEAN RtlCheckBit(PCRTL_BITMAP lpBits, ULONG ulBit)
{
    if (lpBits && ulBit < lpBits->SizeOfBitMap &&
        lpBits->BitMapBuffer[ulBit >> 3] & (1 << (ulBit & 7)))
        return TRUE;
    return FALSE;
}

#define RtlClearAllBits(p) \
    do { \
        PRTL_BITMAP _p = (p); \
        memset(_p->BitMapBuffer,0,((_p->SizeOfBitMap + 31) & 0xffffffe0) >> 3); \
    } while (0)

#define RtlInitializeBitMap(p,b,s) \
    do { \
        PRTL_BITMAP _p = (p); \
        _p->SizeOfBitMap = (s); \
        _p->BitMapBuffer = (b); \
    } while (0)

#define RtlSetAllBits(p) \
    do { \
        PRTL_BITMAP _p = (p); \
        memset(_p->BitMapBuffer,0xff,((_p->SizeOfBitMap + 31) & 0xffffffe0) >> 3); \
    } while (0)

/* These are implemented as __fastcall, so we can't let Winelib apps link with them */
inline static USHORT RtlUshortByteSwap(USHORT s)
{
    return (s >> 8) | (s << 8);
}
inline static ULONG RtlUlongByteSwap(ULONG i)
{
#if defined(__i386__) && defined(__GNUC__)
    ULONG ret;
    __asm__("bswap %0" : "=r" (ret) : "0" (i) );
    return ret;
#else
    return ((ULONG)RtlUshortByteSwap((USHORT)i) << 16) | RtlUshortByteSwap((USHORT)(i >> 16));
#endif
}

/*************************************************************************
 * Loader functions and structures.
 *
 * Those are not part of standard Winternl.h
 */
typedef struct _LDR_MODULE
{
    LIST_ENTRY          InLoadOrderModuleList;
    LIST_ENTRY          InMemoryOrderModuleList;
    LIST_ENTRY          InInitializationOrderModuleList;
    void*               BaseAddress;
    void*               EntryPoint;
    ULONG               SizeOfImage;
    UNICODE_STRING      FullDllName;
    UNICODE_STRING      BaseDllName;
    ULONG               Flags;
    SHORT               LoadCount;
    SHORT               TlsIndex;
    HANDLE              SectionHandle;
    ULONG               CheckSum;
    ULONG               TimeDateStamp;
} LDR_MODULE, *PLDR_MODULE;

/* those defines are (some of the) regular LDR_MODULE.Flags values */
#define LDR_IMAGE_IS_DLL                0x00000004
#define LDR_LOAD_IN_PROGRESS            0x00001000
#define LDR_UNLOAD_IN_PROGRESS          0x00002000
#define LDR_NO_DLL_CALLS                0x00040000
#define LDR_PROCESS_ATTACHED            0x00080000
#define LDR_MODULE_REBASED              0x00200000

/* these ones is Wine specific */
#define LDR_DONT_RESOLVE_REFS           0x40000000
#define LDR_WINE_INTERNAL               0x80000000

/* FIXME: to be checked */
#define MAXIMUM_FILENAME_LENGTH 256

typedef struct _SYSTEM_MODULE
{
    ULONG               Reserved1;
    ULONG               Reserved2;
    PVOID               ImageBaseAddress;
    ULONG               ImageSize;
    ULONG               Flags;
    WORD                Id;
    WORD                Rank;
    WORD                Unknown;
    WORD                NameOffset;
    BYTE                Name[MAXIMUM_FILENAME_LENGTH];
} SYSTEM_MODULE, *PSYSTEM_MODULE;

typedef struct _SYSTEM_MODULE_INFORMATION
{
    ULONG               ModulesCount;
    SYSTEM_MODULE       Modules[1]; /* FIXME: should be Modules[0] */
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

NTSTATUS WINAPI LdrDisableThreadCalloutsForDll(HMODULE);
NTSTATUS WINAPI LdrFindEntryForAddress(const void*, PLDR_MODULE*);
NTSTATUS WINAPI LdrLockLoaderLock(ULONG,ULONG*,ULONG*);
NTSTATUS WINAPI LdrQueryProcessModuleInformation(SYSTEM_MODULE_INFORMATION*, ULONG, ULONG*);
NTSTATUS WINAPI LdrUnloadDll(HMODULE);
NTSTATUS WINAPI LdrUnlockLoaderLock(ULONG,ULONG);

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

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif  /* __WINE_WINTERNAL_H */
