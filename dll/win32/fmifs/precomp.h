/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         File Management IFS Utility functions
 * FILE:            reactos/dll/win32/fmifs/precomp.h
 * PURPOSE:         Win32 FMIFS API Library Header
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Hervé Poussineau (hpoussin@reactos.org)
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

    CHKDSKEX ChkdskEx;
    PVOID Extend;
    FORMATEX FormatEx;

    WCHAR Name[1];
} IFS_PROVIDER, *PIFS_PROVIDER;

/* init.c */
PIFS_PROVIDER
GetProvider(
    IN PWCHAR FileSytem);

#endif /* _FMIFS_PCH_ */
