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

/* FIXME: Update with _LDR_DATA_TABLE_ENTRY and LDR_ flags */
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

typedef struct _LDR_RESOURCE_INFO 
{
    ULONG Type;
    ULONG Name;
    ULONG Language;
} LDR_RESOURCE_INFO, *PLDR_RESOURCE_INFO;

#endif
