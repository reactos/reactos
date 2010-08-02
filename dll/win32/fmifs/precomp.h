/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         File Management IFS Utility functions
 * FILE:            reactos/dll/win32/fmifs/precomp.h
 * PURPOSE:         Win32 FMIFS API Library Header
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Hervé Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES ******************************************************************/

#define WIN32_NO_STATUS
#define NTOS_MODE_USER
#define UNICODE
#define _UNICODE

/* PSDK/NDK Headers */
#include <windows.h>
#include <ndk/ntndk.h>

/* FMIFS Public Header */
#include <fmifs/fmifs.h>

/* VFATLIB Public Header */
#include <fslib/vfatlib.h>

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

/* EOF */
