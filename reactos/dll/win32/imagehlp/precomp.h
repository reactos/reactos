/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/imagehlp/precomp.h
 * PURPOSE:         Imagehlp Libary Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* Definitions */
#define _CRT_SECURE_NO_DEPRECATE
#define NTOS_MODE_USER
#define WIN32_NO_STATUS

/* PSDK/NDK Headers */
#include <windows.h>
#include <imagehlp.h>
#include <ndk/umtypes.h>
#include <ndk/rtlfuncs.h>

/* C STDLIB Headers */
#include <stdio.h>

/* TYPES *********************************************************************/

typedef struct _BOUND_FORWARDER_REFS
{
    struct _BOUND_FORWARDER_REFS *Next;
    ULONG TimeDateStamp;
    LPSTR ModuleName;
} BOUND_FORWARDER_REFS, *PBOUND_FORWARDER_REFS;

typedef struct _IMPORT_DESCRIPTOR
{
    struct _IMPORT_DESCRIPTOR *Next;
    LPSTR ModuleName;
    ULONG TimeDateStamp;
    USHORT ForwaderReferences;
    PBOUND_FORWARDER_REFS Forwarders;
} IMPORT_DESCRIPTOR, *PIMPORT_DESCRIPTOR;

/* DATA **********************************************************************/

extern HANDLE IMAGEHLP_hHeap;

/* FUNCTIONS *****************************************************************/

BOOL
IMAGEAPI
UnloadAllImages(VOID);

/* EOF */
