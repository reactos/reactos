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

/* ENUMERATIONS **************************************************************/

typedef enum 
{
    ExceptionContinueExecution,
    ExceptionContinueSearch,
    ExceptionNestedException,
    ExceptionCollidedUnwind
} EXCEPTION_DISPOSITION;

/* FUNCTION TYPES ************************************************************/
typedef NTSTATUS
(*PHEAP_ENUMERATION_ROUTINE)(IN PVOID HeapHandle,
                             IN PVOID UserParam);

typedef EXCEPTION_DISPOSITION 
(*PEXCEPTION_HANDLER)(struct _EXCEPTION_RECORD*, 
                      PVOID, 
                      struct _CONTEXT*, 
                      PVOID);
                      
/* TYPES *********************************************************************/

typedef unsigned short RTL_ATOM;
typedef unsigned short *PRTL_ATOM;

typedef struct _ACE
{
    ACE_HEADER Header;
    ACCESS_MASK AccessMask;
} ACE, *PACE;

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

typedef struct _RTL_MESSAGE_RESOURCE_ENTRY 
{
    USHORT Length;
    USHORT Flags;
    UCHAR Text[1];
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
