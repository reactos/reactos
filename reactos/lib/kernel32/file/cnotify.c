/*
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
FindCloseChangeNotification(
	HANDLE hChangeHandle 	
   )
{
	return FALSE;
}

HANDLE
STDCALL
FindFirstChangeNotificationA(
    LPCSTR lpPathName,
    WINBOOL bWatchSubtree,
    DWORD dwNotifyFilter
    )
{
	ULONG i;

	WCHAR PathNameW[MAX_PATH];
	

	

    	i = 0;
   	while ((*lpPathName)!=0 && i < MAX_PATH)
     	{
		PathNameW[i] = *lpPathName;
		lpPathName++;
		i++;
     	}
   	PathNameW[i] = 0;
	return FindFirstChangeNotificationW(PathNameW, bWatchSubtree, dwNotifyFilter );

}

HANDLE
STDCALL
FindFirstChangeNotificationW(
    LPCWSTR lpPathName,
    WINBOOL bWatchSubtree,
    DWORD dwNotifyFilter
    )
{
	return NULL;
}

WINBOOL 
STDCALL
FindNextChangeNotification(
	HANDLE hChangeHandle 	
	)
{
	return FALSE;
}


