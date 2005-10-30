/*
 * ReactOS Win32 Applications
 * Copyright (C) 2005 ReactOS Team
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS arp utility
 * FILE:        apps/utils/gettype/gettype.c
 * PURPOSE:
 * PROGRAMMERS: Brandon Turner (turnerb7@msu.edu)
 * REVISIONS:
 *   GM 30/10/05 Created
 *
 * FIXME: gettype only supports local computer.
 */
#include <windows.h>
#include <tchar.h>
#include <stdio.h>

enum
{
	GETTYPE_ROLE	= 0x001,
	GETTYPE_HELP	= 0x002,
	GETTYPE_SP		= 0x004,
	GETTYPE_VER		= 0x008,
	GETTYPE_MINV	= 0x010,
	GETTYPE_MAJV	= 0x020,
	GETTYPE_TYPE	= 0x040,
	GETTYPE_BUILD	= 0x080
};

void GetBasicInfo(OSVERSIONINFOEX osvi, char * HostName, char * OSName, char * Version, char * Role, char * Components)
{
	/* Host Name - COMPUTERNAME*/
	DWORD  bufCharCount = 1024;
	GetComputerName(HostName, &bufCharCount);

	/* OSName - Windows XP Home Editition */
	if(osvi.dwMajorVersion == 4)
	{
		_tcscpy(OSName, _T("Microsoft Windows NT 4.0 "));
	}
	else if(osvi.dwMajorVersion == 5)
	{
		if(osvi.dwMajorVersion == 0)
		{
			_tcscpy(OSName, _T("Microsoft Windows 2000 "));
		}
		else if(osvi.dwMinorVersion == 1)
		{
			_tcscpy(OSName, _T("Microsoft Windows XP "));
		}
		else if(osvi.dwMinorVersion == 2)
		{
			_tcscpy(OSName, _T("Microsoft Windows Server 2003 "));
		}
	}
	else if(osvi.dwMajorVersion == 6)
	{
		_tcscpy(OSName, _T("Microsoft Windows Vista "));
	}
	else 
	{
		_tcscpy(OSName, _T("Microsoft Windows "));
	}

	if(osvi.wSuiteMask & VER_SUITE_BLADE)
		_tcscat(OSName, _T("Web Edition"));
	if(osvi.wSuiteMask & VER_SUITE_DATACENTER)
		_tcscat(OSName, _T("Datacenter"));
	if(osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
		_tcscat(OSName, _T("Enterprise"));
	if(osvi.wSuiteMask & VER_SUITE_EMBEDDEDNT)
		_tcscat(OSName, _T("Embedded"));
	if(osvi.wSuiteMask & VER_SUITE_PERSONAL)
		_tcscat(OSName, _T("Home Edition"));
	if(osvi.wSuiteMask & VER_SUITE_SMALLBUSINESS_RESTRICTED && osvi.wSuiteMask & VER_SUITE_SMALLBUSINESS)
		_tcscat(OSName, _T("Small Bussiness Edition"));

	/* Version - 5.1 Build 2600 Serivce Pack 2 */
	_stprintf(Version, _T("%d.%d Build %d %s"),(int)osvi.dwMajorVersion,(int)osvi.dwMinorVersion,(int)osvi.dwBuildNumber, osvi.szCSDVersion);

	/* Role - Workgroup / Server / Domain Controller */
	if(osvi.wProductType == VER_NT_DOMAIN_CONTROLLER)
		_tcscpy(Role, _T("Domain Controller"));
	else if(osvi.wProductType == VER_NT_SERVER)
		_tcscpy(Role, _T("Server"));
	else if(osvi.wProductType == VER_NT_WORKSTATION)
		_tcscpy(Role, _T("Workgroup"));

	/* Components - N/A */
	BOOL bCompInstalled = FALSE;
	_tcscpy(Components, "");
	if(osvi.wSuiteMask & VER_SUITE_BACKOFFICE)
	{
		_tcscat(Components, _T("Microsoft BackOffice"));
		bCompInstalled = TRUE;
	}
	if(osvi.wSuiteMask & VER_SUITE_TERMINAL)
	{
		if(bCompInstalled)
			_tcscat(OSName, ";");
		_tcscat(Components, _T("Terminal Services"));
		bCompInstalled = TRUE;
	}
	if(!bCompInstalled)
		_tcscat(Components, _T("Not Installed"));

}
int main (int argc, char *argv[])
{
	DWORD Operations;
	INT i = 0;
	OSVERSIONINFOEX osvi;
	
	/* get the struct we will pull all the info from */
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	if(!GetVersionEx((OSVERSIONINFO*)&osvi))
		return 255;

	/* no params, just print some info */
	if(argc == 1)
	{
		TCHAR HostName[1024] = _T("");
		TCHAR OSName[1024] = _T("");
		TCHAR VersionInfo[1024] = _T("");
		TCHAR Role[1024] = _T("");
		TCHAR Components[1024] = _T("");
		GetBasicInfo(osvi, HostName, OSName, VersionInfo, Role, Components);
		_tprintf(_T("\nHostname: %s\nName: %s\nVersion: %s\nRole: %s\nComponent: %s\n"),HostName, OSName, VersionInfo, Role, Components);
		return 0;
	}

	/* read the commands */
	for (i = 1; i < argc; i++)
	{
		if(!_tcsicmp(argv[i], _T("/ROLE")))
			Operations |= GETTYPE_ROLE;
		else if(!_tcsicmp(argv[i], _T("/VER")))
			Operations |= GETTYPE_VER;
		else if(!_tcsicmp(argv[i], _T("/MAJV")))
			Operations |= GETTYPE_MAJV;
		else if(!_tcsicmp(argv[i], _T("/MINV")))
			Operations |= GETTYPE_MINV;
		else if(!_tcsicmp(argv[i], _T("/SP")))
			Operations |= GETTYPE_SP;
		else if(!_tcsicmp(argv[i], _T("/BUILD")))
			Operations |= GETTYPE_BUILD;
		else if(!_tcsicmp(argv[i], _T("/TYPE")))
			Operations |= GETTYPE_TYPE;
		else if(!_tcsicmp(argv[i], _T("/?")))
			Operations |= GETTYPE_HELP;
		else
		{
			_tprintf(_T("Unsupported parameter"));
			return 255;
		}
	}

	/* preform the operations */
	if(Operations & GETTYPE_VER)
	{
		INT VersionNumber = 0;
		VersionNumber = osvi.dwMajorVersion * 1000;
		VersionNumber += (osvi.dwMinorVersion * 100);
		return VersionNumber;
	}
	else if(Operations & GETTYPE_MAJV)
	{
		INT VersionNumber = 0;
		VersionNumber = osvi.dwMajorVersion;
		return VersionNumber;
	}
	else if(Operations & GETTYPE_MINV)
	{
		INT VersionNumber = 0;
		VersionNumber = osvi.dwMinorVersion;
		return VersionNumber;
	}
	else if(Operations & GETTYPE_ROLE)
	{
		if(osvi.wProductType == VER_NT_DOMAIN_CONTROLLER)
			return 1;
		else if(osvi.wProductType == VER_NT_SERVER)
			return 2;
		else if(osvi.wProductType == VER_NT_WORKSTATION)
			return 3;
	}
	else if(Operations & GETTYPE_SP)
	{
		INT SPNumber = 0;
		SPNumber = osvi.wServicePackMajor;
		return SPNumber;
	}
	else if(Operations & GETTYPE_BUILD)
	{
		INT BuildNumber = 0;
		BuildNumber = osvi.dwBuildNumber;
		return BuildNumber;
	}
	else if(Operations & GETTYPE_TYPE)
	{
		if(osvi.dwMajorVersion == 5)
		{
			if(osvi.dwMinorVersion == 1)
			{
				if(osvi.wSuiteMask & VER_SUITE_PERSONAL)
					return 1;
				else
					return 2;
			}
			else if(osvi.dwMinorVersion == 2)
			{
				if(osvi.wSuiteMask & VER_SUITE_BLADE)
					return 6;
				else if(osvi.wSuiteMask & VER_SUITE_DATACENTER)
					return 5;
				else if(osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
					return 4;
				else 
					return 3;
			}
		}
	}
	else if(Operations & GETTYPE_HELP)
	{
		_tprintf(_T("GETTYPE  [/ROLE | /SP | /VER | /MAJV | /MINV | /TYPE | /BUILD]"));
		return 0;
	}

	return 255;
}
