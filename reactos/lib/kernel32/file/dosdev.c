/* $Id: dosdev.c,v 1.3 2000/06/03 14:47:32 ea Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/dosdev.c
 * PURPOSE:         Dos device functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <ddk/ntddk.h>
#include <windows.h>


WINBOOL
STDCALL
DefineDosDeviceA(
    DWORD dwFlags,
    LPCSTR lpDeviceName,
    LPCSTR lpTargetPath
    )
{
	ULONG i;

	WCHAR DeviceNameW[MAX_PATH];
	WCHAR TargetPathW[MAX_PATH];

	i = 0;
	while ((*lpDeviceName)!=0 && i < MAX_PATH)
	{
		DeviceNameW[i] = *lpDeviceName;
		lpDeviceName++;
		i++;
	}
	DeviceNameW[i] = 0;

	i = 0;
	while ((*lpTargetPath)!=0 && i < MAX_PATH)
	{
		TargetPathW[i] = *lpTargetPath;
		lpTargetPath++;
		i++;
	}
	TargetPathW[i] = 0;
	return DefineDosDeviceW(dwFlags,DeviceNameW,TargetPathW);
}



DWORD
STDCALL
QueryDosDeviceA(
    LPCSTR lpDeviceName,
    LPSTR lpTargetPath,
    DWORD ucchMax
    )
{
	ULONG i;

	WCHAR DeviceNameW[MAX_PATH];
	WCHAR TargetPathW[MAX_PATH];

	

    	i = 0;
   	while ((*lpDeviceName)!=0 && i < MAX_PATH)
     	{
		DeviceNameW[i] = *lpDeviceName;
		lpDeviceName++;
		i++;
     	}
   	DeviceNameW[i] = 0;

	i = 0;
   	while ((*lpTargetPath)!=0 && i < MAX_PATH)
     	{
		TargetPathW[i] = *lpTargetPath;
		lpTargetPath++;
		i++;
     	}
   	TargetPathW[i] = 0;
	return QueryDosDeviceW(DeviceNameW,TargetPathW,ucchMax);
}


WINBOOL
STDCALL
DefineDosDeviceW(
    DWORD dwFlags,
    LPCWSTR lpDeviceName,
    LPCWSTR lpTargetPath
    )
{
	return FALSE;
}

DWORD
STDCALL
QueryDosDeviceW(
    LPCWSTR lpDeviceName,
    LPWSTR lpTargetPath,
    DWORD ucchMax
    )
{
	return FALSE;
}

/* EOF */
