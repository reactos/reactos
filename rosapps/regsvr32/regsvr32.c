/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS
 * FILE:            apps\regsvr32\regsvr32.c
 * PURPOSE:         Register a COM component in the registry
 * PROGRAMMER:      jurgen van gael [jurgen.vangael@student.kuleuven.ac.be]
 * UPDATE HISTORY:
 *                  Created 31/12/2001
 */
/********************************************************************


This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.


********************************************************************/
//
//	regsvr32 [/u] [/s] [/n] [/i[:cmdline]] dllname
//	[/u]	unregister server
//	[/s]	silent (no message boxes)
//	[/i]	Call DllInstall passing it an optional [cmdline]; when used with /u calls dll uninstall
//	[/n]	Do not call DllRegisterServer; this option must be used with [/i]
//
#include <stdio.h>
#include <windows.h>
#include <ole32\olectl.h>

typedef HRESULT (*DLLREGISTER)		(void);
typedef HRESULT (*DLLUNREGISTER)	(void);

int Silent = 0;

int Usage()
{
	printf("regsvr32 [/u] [/s] [/n] [/i[:cmdline]] dllname\n");
	printf("\t[/u]	unregister server\n");
	printf("\t[/s]	silent (no message boxes)\n");
	printf("\t[/i]	Call DllInstall passing it an optional [cmdline]; when used with /u calls dll uninstall\n");
	printf("\t[/n]	Do not call DllRegisterServer; this option must be used with [/i]\n");

	return 0;
}

int RegisterDll(char* strDll)
{
	HRESULT hr = S_OK;
	DLLREGISTER pfRegister;
	HMODULE	DllHandle = NULL;

	DllHandle = LoadLibrary(strDll);
	if(!DllHandle)
	{
		if(!Silent)
			printf("Dll not found\n");

		return -1;
	}
	pfRegister = (VOID*) GetProcAddress(DllHandle, "DllRegisterServer");
	if(!pfRegister)
	{
		if(!Silent)
			printf("DllRegisterServer not implemented\n");

		return -1;
	}
	hr = pfRegister();
	if(FAILED(hr))
	{
		if(!Silent)
			printf("Failed to register dll\n");

		return -1;
	}
	if(!Silent)
		printf("Succesfully registered dll\n");

	//	clean
	if(DllHandle)
		FreeLibrary(DllHandle);
}

int UnregisterDll(char* strDll)
{
	HRESULT hr = S_OK;
	HMODULE	DllHandle = NULL;
	DLLUNREGISTER pfUnregister;

	DllHandle = LoadLibrary(strDll);
	if(!DllHandle)
	{
		if(!Silent)
			printf("Dll not found\n");

		return -1;
	}
	pfUnregister = (VOID*) GetProcAddress(DllHandle, "DllUnregisterServer");
	if(!pfUnregister)
	{
		if(!Silent)
			printf("DllUnregisterServer not implemented\n");

		return -1;
	}
	hr = pfUnregister();
	if(FAILED(hr))
	{
		if(!Silent)
			printf("Failed to unregister dll\n");

		return -1;
	}
	if(!Silent)
		printf("Succesfully unregistered dll\n");

	//	clean
	if(DllHandle)
		FreeLibrary(DllHandle);
}

int main(int argc, char* argv[])
{
	int		i = 0;
	int		Unregister = 0;
	char	DllName[MAX_PATH];

	DllName[0] = '\0';

	for(i = 0; i < argc; i++)
	{
		if(argv[i][0] == '/')
		{
			if(argv[i][1] == 'u')
				Unregister = 1;
			else if(argv[i][1] == 's')
				Silent = 1;
			else if(argv[i][1] == 'i')
				;	//	not implemented yet
			else if(argv[i][1] == 'n')
				;	//	not implemented yet
		}
		else
			strcpy(DllName, argv[i]);
	}

	if(!strcmp(DllName, "regsvr32") || !strcmp(DllName, "regsvr32.exe"))
	{
		if(!Silent)
			return Usage();
		else
			return -1;
	}

	if(Unregister == 0)
		return RegisterDll(DllName);
	else
		return UnregisterDll(DllName);

	return 0;
}
