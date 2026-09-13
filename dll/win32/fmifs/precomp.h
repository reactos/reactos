/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         File Management IFS Utility functions
 * FILE:            reactos/dll/win32/fmifs/precomp.h
 * PURPOSE:         Win32 FMIFS API Library Header
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Herv� Poussineau (hpoussin@reactos.org)
 */

#ifndef _FMIFS_PCH_
#define _FMIFS_PCH_

/* INCLUDES ******************************************************************/

#include <stdio.h>

#define WIN32_NO_STATUS

/* PSDK/NDK Headers */
#include <windef.h>
#include <winbase.h>

#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>

/* FMIFS Public Header */
#include <fmifs/fmifs.h>

extern LIST_ENTRY ProviderListHead;

typedef struct _IFS_PROVIDER
{
    LIST_ENTRY ListEntry;

    PWSTR Name;
    PWSTR DllFile;

    HMODULE hModule;
    PULIB_CHKDSK Chkdsk;
    PVOID ChkdskEx;
    PVOID Extend;
    PULIB_FORMAT Format;
    PVOID FormatEx;

} IFS_PROVIDER, *PIFS_PROVIDER;

/* init.c */
PIFS_PROVIDER
LoadProvider(
    _In_ PWCHAR FileSytem);

BOOLEAN
UnloadProvider(
    _In_ PIFS_PROVIDER Provider);

#endif /* _FMIFS_PCH_ */
