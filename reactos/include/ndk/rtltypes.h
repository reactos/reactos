/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/rtltypes.h
 * PURPOSE:         Defintions for Runtime Library Types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _RTLTYPES_H
#define _RTLTYPES_H

/* DEPENDENCIES **************************************************************/
#include "zwtypes.h"

/* EXPORTED DATA *************************************************************/

/* CONSTANTS *****************************************************************/
#define MAXIMUM_LEADBYTES 12

#define PPF_NORMALIZED (1)

#define PEB_BASE        (0x7FFDF000)

#define EXCEPTION_CONTINUE_SEARCH     0
#define EXCEPTION_EXECUTE_HANDLER     1

#define EXCEPTION_UNWINDING 0x02
#define EXCEPTION_EXIT_UNWIND 0x04
#define EXCEPTION_STACK_INVALID 0x8
#define EXCEPTION_NESTED_CALL 0x10
#define EXCEPTION_TARGET_UNWIND 0x20
#define EXCEPTION_COLLIDED_UNWIND 0x20

#define  EH_NONCONTINUABLE   0x01
#define  EH_UNWINDING        0x02
#define  EH_EXIT_UNWIND      0x04
#define  EH_STACK_INVALID    0x08
#define  EH_NESTED_CALL      0x10

#define RTL_RANGE_LIST_ADD_IF_CONFLICT  0x00000001
#define RTL_RANGE_LIST_ADD_SHARED       0x00000002

#define RTL_RANGE_SHARED      0x01
#define RTL_RANGE_CONFLICT    0x02

/* FIXME: Rename these */
#define PDI_MODULES     0x01	/* The loaded modules of the process */
#define PDI_BACKTRACE   0x02	/* The heap stack back traces */
#define PDI_HEAPS       0x04	/* The heaps of the process */
#define PDI_HEAP_TAGS   0x08	/* The heap tags */
#define PDI_HEAP_BLOCKS 0x10	/* The heap blocks */
#define PDI_LOCKS       0x20	/* The locks created by the process */
/* ENUMERATIONS **************************************************************/

typedef enum 
{
    ExceptionContinueExecution,
    ExceptionContinueSearch,
    ExceptionNestedException,
    ExceptionCollidedUnwind
} EXCEPTION_DISPOSITION;

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

/* FUNCTION TYPES ************************************************************/
typedef NTSTATUS
(*PHEAP_ENUMERATION_ROUTINE)(
    IN PVOID HeapHandle,
    IN PVOID UserParam
);

typedef EXCEPTION_DISPOSITION 
(*PEXCEPTION_HANDLER)(
    struct _EXCEPTION_RECORD*, 
    PVOID, 
    struct _CONTEXT*, 
    PVOID
);

typedef LONG (STDCALL *PRTL_VECTORED_EXCEPTION_HANDLER)(
    PEXCEPTION_POINTERS ExceptionPointers
);

typedef DWORD (STDCALL *PTHREAD_START_ROUTINE)(
    LPVOID Parameter
);

typedef VOID
(STDCALL *PRTL_BASE_PROCESS_START_ROUTINE)(
    PTHREAD_START_ROUTINE StartAddress,
    PVOID Parameter
);
                        
/* TYPES *********************************************************************/

typedef unsigned short RTL_ATOM;
typedef unsigned short *PRTL_ATOM;

typedef ACL_REVISION_INFORMATION *PACL_REVISION_INFORMATION;
typedef ACL_SIZE_INFORMATION *PACL_SIZE_INFORMATION;
typedef struct _ACE
{
    ACE_HEADER Header;
    ACCESS_MASK AccessMask;
} ACE, *PACE;

/* FIXME: Review definitions and give these guys a better name */
typedef struct _DEBUG_BUFFER
{
    HANDLE SectionHandle;
    PVOID SectionBase;
    PVOID RemoteSectionBase;
    ULONG SectionBaseDelta;
    HANDLE EventPairHandle;
    ULONG Unknown[2];
    HANDLE RemoteThreadHandle;
    ULONG InfoClassMask;
    ULONG SizeOfInfo;
    ULONG AllocatedSize;
    ULONG SectionSize;
    PVOID ModuleInformation;
    PVOID BackTraceInformation;
    PVOID HeapInformation;
    PVOID LockInformation;
    PVOID Reserved[8];
} DEBUG_BUFFER, *PDEBUG_BUFFER;
typedef struct _DEBUG_MODULE_INFORMATION
{
    ULONG Reserved[2];
    PVOID Base;
    ULONG Size;
    ULONG Flags;
    USHORT Index;
    USHORT Unknown;
    USHORT LoadCount;
    USHORT ModuleNameOffset;
    CHAR ImageName[256];
} DEBUG_MODULE_INFORMATION, *PDEBUG_MODULE_INFORMATION;
typedef struct _DEBUG_HEAP_INFORMATION
{
    PVOID Base;
    ULONG Flags;
    USHORT Granularity;
    USHORT Unknown;
    ULONG Allocated;
    ULONG Committed;
    ULONG TagCount;
    ULONG BlockCount;
    ULONG Reserved[7];
    PVOID Tags;
    PVOID Blocks;
} DEBUG_HEAP_INFORMATION, *PDEBUG_HEAP_INFORMATION;
typedef struct _DEBUG_LOCK_INFORMATION 
{
    PVOID Address;
    USHORT Type;
    USHORT CreatorBackTraceIndex;
    ULONG OwnerThreadId;
    ULONG ActiveCount;
    ULONG ContentionCount;
    ULONG EntryCount;
    ULONG RecursionCount;
    ULONG NumberOfSharedWaiters;
    ULONG NumberOfExclusiveWaiters;
} DEBUG_LOCK_INFORMATION, *PDEBUG_LOCK_INFORMATION;
typedef struct _RTL_HANDLE
{
     struct _RTL_HANDLE *Next;	/* pointer to next free handle */
} RTL_HANDLE, *PRTL_HANDLE;

typedef struct _RTL_HANDLE_TABLE
{
     ULONG TableSize;		/* maximum number of handles */
     ULONG HandleSize;		/* size of handle in bytes */
     PRTL_HANDLE Handles;		/* pointer to handle array */
     PRTL_HANDLE Limit;		/* limit of pointers */
     PRTL_HANDLE FirstFree;	/* pointer to first free handle */
     PRTL_HANDLE LastUsed;	/* pointer to last allocated handle */
} RTL_HANDLE_TABLE, *PRTL_HANDLE_TABLE;
#ifdef READY_FOR_NEW_NTDLL
typedef struct _LOCK_INFORMATION
{
    ULONG LockCount;
    DEBUG_LOCK_INFORMATION LockEntry[1];
} LOCK_INFORMATION, *PLOCK_INFORMATION;
typedef struct _HEAP_INFORMATION
{
    ULONG HeapCount;
    DEBUG_HEAP_INFORMATION HeapEntry[1];
} HEAP_INFORMATION, *PHEAP_INFORMATION;
typedef struct _MODULE_INFORMATION
{
    ULONG ModuleCount;
    DEBUG_MODULE_INFORMATION ModuleEntry[1];
} MODULE_INFORMATION, *PMODULE_INFORMATION;
/* END REVIEW AREA */
#endif

typedef struct _EXCEPTION_REGISTRATION 
{
    struct _EXCEPTION_REGISTRATION*    prev;
    PEXCEPTION_HANDLER        handler;
} EXCEPTION_REGISTRATION, *PEXCEPTION_REGISTRATION;

typedef EXCEPTION_REGISTRATION EXCEPTION_REGISTRATION_RECORD;
typedef PEXCEPTION_REGISTRATION PEXCEPTION_REGISTRATION_RECORD;

typedef struct RTL_DRIVE_LETTER_CURDIR 
{
    USHORT Flags;
    USHORT Length;
    ULONG TimeStamp;
    UNICODE_STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;
     
typedef struct _RTL_HEAP_DEFINITION 
{
    ULONG Length;
    ULONG Unknown[11];
} RTL_HEAP_DEFINITION, *PRTL_HEAP_DEFINITION;

typedef struct _RTL_RANGE_LIST
{
    LIST_ENTRY ListHead;
    ULONG Flags;  /* RTL_RANGE_LIST_... flags */
    ULONG Count;
    ULONG Stamp;
} RTL_RANGE_LIST, *PRTL_RANGE_LIST;

typedef struct _RTL_RANGE
{
    ULONGLONG Start;
    ULONGLONG End;
    PVOID UserData;
    PVOID Owner;
    UCHAR Attributes;
    UCHAR Flags;  /* RTL_RANGE_... flags */
} RTL_RANGE, *PRTL_RANGE;

typedef BOOLEAN
(STDCALL *PRTL_CONFLICT_RANGE_CALLBACK) (
    PVOID Context,
    PRTL_RANGE Range
);

typedef struct _RTL_RESOURCE
{
    RTL_CRITICAL_SECTION Lock;
    HANDLE SharedSemaphore;
    ULONG SharedWaiters;
    HANDLE ExclusiveSemaphore;
    ULONG ExclusiveWaiters;
    LONG NumberActive;
    HANDLE OwningThread;
    ULONG TimeoutBoost; /* ?? */    
    PVOID DebugInfo; /* ?? */
} RTL_RESOURCE, *PRTL_RESOURCE;

typedef struct _RANGE_LIST_ITERATOR
{
  PLIST_ENTRY RangeListHead;
  PLIST_ENTRY MergedHead;
  PVOID Current;
  ULONG Stamp;
} RTL_RANGE_LIST_ITERATOR, *PRTL_RANGE_LIST_ITERATOR;

typedef struct _RTL_MESSAGE_RESOURCE_ENTRY 
{
    USHORT Length;
    USHORT Flags;
    CHAR Text[1];
} RTL_MESSAGE_RESOURCE_ENTRY, *PRTL_MESSAGE_RESOURCE_ENTRY;

typedef struct _RTL_MESSAGE_RESOURCE_BLOCK 
{
    ULONG LowId;
    ULONG HighId;
    ULONG OffsetToEntries;
} RTL_MESSAGE_RESOURCE_BLOCK, *PRTL_MESSAGE_RESOURCE_BLOCK;

typedef struct _RTL_MESSAGE_RESOURCE_DATA 
{
    ULONG NumberOfBlocks;
    RTL_MESSAGE_RESOURCE_BLOCK Blocks[1];
} RTL_MESSAGE_RESOURCE_DATA, *PRTL_MESSAGE_RESOURCE_DATA;

typedef struct _NLS_FILE_HEADER 
{
    USHORT  HeaderSize;
    USHORT  CodePage;
    USHORT  MaximumCharacterSize;  /* SBCS = 1, DBCS = 2 */
    USHORT  DefaultChar;
    USHORT  UniDefaultChar;
    USHORT  TransDefaultChar;
    USHORT  TransUniDefaultChar;
    USHORT  DBCSCodePage;
    UCHAR   LeadByte[MAXIMUM_LEADBYTES];
} NLS_FILE_HEADER, *PNLS_FILE_HEADER;

typedef struct _RTL_USER_PROCESS_PARAMETERS 
{
    ULONG  AllocationSize;
    ULONG  Size;
    ULONG  Flags;
    ULONG  DebugFlags;
    HANDLE  hConsole;
    ULONG  ProcessGroup;
    HANDLE  hStdInput;
    HANDLE  hStdOutput;
    HANDLE  hStdError;
    UNICODE_STRING  CurrentDirectoryName;
    HANDLE  CurrentDirectoryHandle;
    UNICODE_STRING  DllPath;
    UNICODE_STRING  ImagePathName;
    UNICODE_STRING  CommandLine;
    PWSTR  Environment;
    ULONG  dwX;
    ULONG  dwY;
    ULONG  dwXSize;
    ULONG  dwYSize;
    ULONG  dwXCountChars;
    ULONG  dwYCountChars;
    ULONG  dwFillAttribute;
    ULONG  dwFlags;
    ULONG  wShowWindow;
    UNICODE_STRING  WindowTitle;
    UNICODE_STRING  DesktopInfo;
    UNICODE_STRING  ShellInfo;
    UNICODE_STRING  RuntimeInfo;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;
        
typedef struct _RTL_PROCESS_INFO
{
   ULONG Size;
   HANDLE ProcessHandle;
   HANDLE ThreadHandle;
   CLIENT_ID ClientId;
   SECTION_IMAGE_INFORMATION ImageInfo;
} RTL_PROCESS_INFO, *PRTL_PROCESS_INFO;

/* FIXME: This is a Windows Type which which we are not implementing properly
      The type below however is our own implementation. We will eventually use Windows' */
typedef struct _RTL_ATOM_TABLE 
{
    ULONG TableSize;
    ULONG NumberOfAtoms;
    PVOID Lock;        /* fast mutex (kernel mode)/ critical section (user mode) */
    PVOID HandleTable;
    LIST_ENTRY Slot[0];
} RTL_ATOM_TABLE, *PRTL_ATOM_TABLE;

/* Let Kernel Drivers use this */
#ifndef _WINBASE_H
    typedef struct _SYSTEMTIME 
    {
        WORD wYear;
        WORD wMonth;
        WORD wDayOfWeek;
        WORD wDay;
        WORD wHour;
        WORD wMinute;
        WORD wSecond;
        WORD wMilliseconds;
    } SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

    typedef struct _TIME_ZONE_INFORMATION 
    {
        LONG Bias;
        WCHAR StandardName[32];
        SYSTEMTIME StandardDate;
        LONG StandardBias;
        WCHAR DaylightName[32];
        SYSTEMTIME DaylightDate;
        LONG DaylightBias;
    } TIME_ZONE_INFORMATION, *PTIME_ZONE_INFORMATION, *LPTIME_ZONE_INFORMATION;
#endif

#endif
