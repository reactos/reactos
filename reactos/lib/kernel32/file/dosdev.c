/* $Id: dosdev.c,v 1.7 2003/07/10 18:50:51 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/dosdev.c
 * PURPOSE:         Dos device functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <k32.h>


/*
 * @implemented
 */
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



/*
 * @implemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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
