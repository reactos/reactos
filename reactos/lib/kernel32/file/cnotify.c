/* $Id: cnotify.c,v 1.3 2001/03/31 01:17:29 dwelch Exp $
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


WINBOOL STDCALL
FindCloseChangeNotification (HANDLE hChangeHandle)
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
#if 0
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
#endif
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
