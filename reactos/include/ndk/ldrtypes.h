/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/ldrtypes.h
 * PURPOSE:         Definitions for Loader Types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _LDRTYPES_H
#define _LDRTYPES_H

/* DEPENDENCIES **************************************************************/

/* EXPORTED DATA *************************************************************/

/* CONSTANTS *****************************************************************/
#define RESOURCE_TYPE_LEVEL      0
#define RESOURCE_NAME_LEVEL      1
#define RESOURCE_LANGUAGE_LEVEL  2
#define RESOURCE_DATA_LEVEL      3

/* FIXME: USE CORRRECT LDR_ FLAGS */
#define IMAGE_DLL               0x00000004
#define LOAD_IN_PROGRESS        0x00001000
#define UNLOAD_IN_PROGRESS      0x00002000
#define ENTRY_PROCESSED         0x00004000
#define DONT_CALL_FOR_THREAD    0x00040000
#define PROCESS_ATTACH_CALLED   0x00080000
#define IMAGE_NOT_AT_BASE       0x00200000

/* ENUMERATIONS **************************************************************/

/* TYPES *********************************************************************/
/* FIXME: Update with _LDR_DATA_TABLE_ENTRY and LDR_ flags */
typedef struct _PEB_LDR_DATA
{
    ULONG               Length;
    BOOLEAN             Initialized;
    PVOID               SsHandle;
    LIST_ENTRY          InLoadOrderModuleList;
    LIST_ENTRY          InMemoryOrderModuleList;
    LIST_ENTRY          InInitializationOrderModuleList;
    PVOID               EntryInProgress;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _LDR_DATA_TABLE_ENTRY
{
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG Flags;
    USHORT LoadCount; /* FIXME: HACK!!! FIX ASAP */
    USHORT TlsIndex;  /* FIXME: HACK!!! FIX ASAP */
    LIST_ENTRY HashLinks;
    PVOID SectionPointer;
    ULONG CheckSum;
    ULONG TimeDateStamp;
    PVOID LoadedImports;
    PVOID EntryPointActivationContext;
    PVOID PatchInformation;
#if defined(DBG) || defined(KDBG)
    /* FIXME: THIS _REALLY_ NEEDS TO GO SOMEWHERE ELSE */
    PVOID RosSymInfo;
#endif /* KDBG */
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef struct _LDR_RESOURCE_INFO
{
    ULONG Type;
    ULONG Name;
    ULONG Language;
} LDR_RESOURCE_INFO, *PLDR_RESOURCE_INFO;

#endif
