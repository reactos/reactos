/* $Id: cnotify.c,v 1.2 2000/03/15 12:25:47 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/find.c
 * PURPOSE:         Find functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <windows.h>
#include <wstring.h>


WINBOOL
FindCloseChangeNotification (
	HANDLE	hChangeHandle
	)
{
	return FALSE;
}


HANDLE
STDCALL
FindFirstChangeNotificationA (
	LPCSTR	lpPathName,
	WINBOOL	bWatchSubtree,
	DWORD	dwNotifyFilter
	)
{
	UNICODE_STRING PathNameU;
	ANSI_STRING PathName;
	HANDLE Result;

	RtlInitAnsiString (&PathName,
	                   (LPSTR)lpPathName);

	/* convert ansi (or oem) string to unicode */
	if (bIsFileApiAnsi)
		RtlAnsiStringToUnicodeString (&PathNameU,
		                              &PathName,
		                              TRUE);
	else
		RtlOemStringToUnicodeString (&PathNameU,
		                             &PathName,
		                             TRUE);

	Result = FindFirstChangeNotificationW (PathNameU.Buffer,
	                                       bWatchSubtree,
	                                       dwNotifyFilter);

	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             RootPathNameU.Buffer);

	return Result;
}


HANDLE
STDCALL
FindFirstChangeNotificationW (
	LPCWSTR	lpPathName,
	WINBOOL	bWatchSubtree,
	DWORD	dwNotifyFilter
	)
{
	return NULL;
}


WINBOOL
STDCALL
FindNextChangeNotification (
	HANDLE	hChangeHandle
	)
{
	return FALSE;
}

/* EOF */
