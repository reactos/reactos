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
#include <lm.h>
#include <shellapi.h>

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

INT 
GetVersionNumber(BOOL bLocal, LPOSVERSIONINFOEX osvi, LPSERVER_INFO_102 pBuf102)
{
	INT VersionNumber = 255;
	if(pBuf102 != NULL && !bLocal)
	{
		VersionNumber = pBuf102->sv102_version_major * 1000;
		VersionNumber += (pBuf102->sv102_version_minor * 100);
	}
	else if(bLocal)
	{		
		VersionNumber = osvi->dwMajorVersion * 1000;
		VersionNumber += (osvi->dwMinorVersion * 100);
	}

	return VersionNumber;
}

INT 
GetMajValue(BOOL Major, BOOL bLocal, LPOSVERSIONINFOEX osvi, LPSERVER_INFO_102 pBuf102)
{
	INT VersionNumber = 255;
	if(pBuf102 != NULL && !bLocal)
	{
		if(Major)
			VersionNumber = pBuf102->sv102_version_major * 1000;
		else
			VersionNumber = pBuf102->sv102_version_minor * 100;
	}
	else
	{
		if(Major)
			VersionNumber = osvi->dwMajorVersion * 1000;
		else
			VersionNumber = osvi->dwMinorVersion * 100;
	}
	return VersionNumber;
}

INT 
GetSystemRole(BOOL bLocal, LPOSVERSIONINFOEX osvi, LPSERVER_INFO_102 pBuf102)
{

	if(pBuf102 != NULL && !bLocal)
	{
		if ((pBuf102->sv102_type & SV_TYPE_DOMAIN_CTRL) ||
			(pBuf102->sv102_type & SV_TYPE_DOMAIN_BAKCTRL))
			return 1;
		else if(pBuf102->sv102_type & SV_TYPE_SERVER_NT)
			return 2;
		else
			return 3;
	}
	else
	{
		if(osvi->wProductType == VER_NT_DOMAIN_CONTROLLER)
			return 1;
		else if(osvi->wProductType == VER_NT_SERVER)
			return 2;
		else if(osvi->wProductType == VER_NT_WORKSTATION)
			return 3;
	}
	return 255;

}

INT 
GetServicePack(BOOL bLocal, LPOSVERSIONINFOEX osvi, LPSERVER_INFO_102 pBuf102, TCHAR * Server)
{
	INT SPNumber = 255;
	if(!bLocal)
	{
		/* FIXME: Use Registry to get value */
	}
	else
	{
		SPNumber = osvi->wServicePackMajor;
	}
	return SPNumber;
}

INT 
GetBuildNumber(BOOL bLocal, LPOSVERSIONINFOEX osvi, LPSERVER_INFO_102 pBuf102)
{
	INT BuildNum = 255;
	if(!bLocal)
	{
		/* FIXME: Use Registry to get value */
	}
	else
	{
		BuildNum = osvi->dwBuildNumber;
	}
	return BuildNum;
}

INT GetType(BOOL bLocal, LPOSVERSIONINFOEX osvi, LPSERVER_INFO_102 pBuf102)
{
	if(bLocal)
	{
		if(osvi->dwMajorVersion == 5)
		{
			if(osvi->dwMinorVersion == 1)
			{
				if(osvi->wSuiteMask & VER_SUITE_PERSONAL)
					return 1;
				else
					return 2;
			}
			else if(osvi->dwMinorVersion == 2)
			{
				if(osvi->wSuiteMask & VER_SUITE_BLADE)
					return 6;
				else if(osvi->wSuiteMask & VER_SUITE_DATACENTER)
					return 5;
				else if(osvi->wSuiteMask & VER_SUITE_ENTERPRISE)
					return 4;
				else 
					return 3;
			}
		}
	}
	else
	{
		/* FIXME: Get this value from registry */
	}
	return 255;
}

VOID 
GetBasicInfo(LPOSVERSIONINFOEX osvi, TCHAR * HostName, TCHAR * OSName, TCHAR * Version, TCHAR * Role, TCHAR * Components)
{
	/* Host Name - COMPUTERNAME*/
	DWORD  bufCharCount = 1024;
	GetComputerName(HostName, &bufCharCount);


	/* OSName - Windows XP Home Editition */
	if(osvi->dwMajorVersion == 4)
	{
		_tcscpy(OSName, _T("Microsoft Windows NT 4.0 "));
	}
	else if(osvi->dwMajorVersion == 5)
	{
		if(osvi->dwMajorVersion == 0)
		{
			_tcscpy(OSName, _T("Microsoft Windows 2000 "));
		}
		else if(osvi->dwMinorVersion == 1)
		{
			_tcscpy(OSName, _T("Microsoft Windows XP "));
		}
		else if(osvi->dwMinorVersion == 2)
		{
			_tcscpy(OSName, _T("Microsoft Windows Server 2003 "));
		}
	}
	else if(osvi->dwMajorVersion == 6)
	{
		_tcscpy(OSName, _T("Microsoft Windows Vista "));
	}
	else 
	{
		_tcscpy(OSName, _T("Microsoft Windows "));
	}

	if(osvi->wSuiteMask & VER_SUITE_BLADE)
		_tcscat(OSName, _T("Web Edition"));
	if(osvi->wSuiteMask & VER_SUITE_DATACENTER)
		_tcscat(OSName, _T("Datacenter"));
	if(osvi->wSuiteMask & VER_SUITE_ENTERPRISE)
		_tcscat(OSName, _T("Enterprise"));
	if(osvi->wSuiteMask & VER_SUITE_EMBEDDEDNT)
		_tcscat(OSName, _T("Embedded"));
	if(osvi->wSuiteMask & VER_SUITE_PERSONAL)
		_tcscat(OSName, _T("Home Edition"));
	if(osvi->wSuiteMask & VER_SUITE_SMALLBUSINESS_RESTRICTED && osvi->wSuiteMask & VER_SUITE_SMALLBUSINESS)
		_tcscat(OSName, _T("Small Bussiness Edition"));

	/* Version - 5.1 Build 2600 Serivce Pack 2 */
	_stprintf(Version, _T("%d.%d Build %d %s"),(int)osvi->dwMajorVersion,(int)osvi->dwMinorVersion,(int)osvi->dwBuildNumber, osvi->szCSDVersion);

	/* Role - Workgroup / Server / Domain Controller */
	if(osvi->wProductType == VER_NT_DOMAIN_CONTROLLER)
		_tcscpy(Role, _T("Domain Controller"));
	else if(osvi->wProductType == VER_NT_SERVER)
		_tcscpy(Role, _T("Server"));
	else if(osvi->wProductType == VER_NT_WORKSTATION)
		_tcscpy(Role, _T("Workgroup"));

	/* Components - FIXME: what is something that might be installed? */
	_tcscat(Components, _T("Not Installed"));

}

INT
_tmain (VOID)
{
	DWORD Operations = 0;
	INT ret = 255;
	INT i = 0;
	INT argc = 0;
	/* True if the target is local host */
	BOOL bLocal = TRUE;
	DWORD nStatus = 0;
	TCHAR ServerName[32];
	TCHAR RemoteResource[32];
	TCHAR UserName[32] = _T("");
	TCHAR Password[32] = _T("");
	LPOSVERSIONINFOEX osvi = NULL;
	LPSERVER_INFO_102 pBuf102 = NULL;
	LPTSTR * argv;
	osvi = (LPOSVERSIONINFOEX)malloc(sizeof(LPOSVERSIONINFOEX));
	pBuf102 = (LPSERVER_INFO_102)malloc(sizeof(LPSERVER_INFO_102));

	/* Get the command line correctly since it is unicode */
	argv = CommandLineToArgvW(GetCommandLineW(), &argc);


	/* Process flags */
	if(argc)
	{
		for (i = 1; i < argc; i++)
		{
			if(!_tcsicmp(argv[i], _T("/ROLE")) && !Operations)
				Operations |= GETTYPE_ROLE;
			else if(!_tcsicmp(argv[i], _T("/VER")) && !Operations)
				Operations |= GETTYPE_VER;
			else if(!_tcsicmp(argv[i], _T("/MAJV")) && !Operations)
				Operations |= GETTYPE_MAJV;
			else if(!_tcsicmp(argv[i], _T("/MINV")) && !Operations)
				Operations |= GETTYPE_MINV;
			else if(!_tcsicmp(argv[i], _T("/SP")) && !Operations)
				Operations |= GETTYPE_SP;
			else if(!_tcsicmp(argv[i], _T("/BUILD")) && !Operations)
				Operations |= GETTYPE_BUILD;
			else if(!_tcsicmp(argv[i], _T("/TYPE")) && !Operations)
				Operations |= GETTYPE_TYPE;
			else if(!_tcsicmp(argv[i], _T("/?")) && !Operations)
				Operations |= GETTYPE_HELP;
			else if(!_tcsicmp(argv[i], _T("/S")) && i + 1 < argc)
			{
				_tcscpy(ServerName,argv[++i]);
				bLocal = FALSE;
			}
			else if(!wcsicmp(argv[i], L"/U") && i + 1 < argc)
				_tcscpy(UserName,argv[++i]);
			else if(!wcsicmp(argv[i], L"/P") && i + 1 < argc)
				_tcscpy(Password,argv[++i]);
			else
			{
				wprintf(L"Error in paramters, please see usage\n");
				return 255;
			}
		}
	}

	/* Some debug info */
	//_tprintf(_T("%s - %s - %s - %d"), ServerName, UserName, Password, (int)Operations);

	if(!bLocal)
	{
		NETRESOURCE nr;


		/* \\*IP or Computer Name*\*Share* */
		_stprintf(RemoteResource, _T("\\\\%s\\IPC$"), ServerName);

		nr.dwType = RESOURCETYPE_ANY;
		nr.lpLocalName = NULL;
		nr.lpProvider= NULL;
		nr.lpRemoteName	= RemoteResource;

		/* open a connection to the server with difference user/pass. */
		nStatus = WNetAddConnection2(&nr, UserName[0]?UserName:NULL,Password[0]?Password:NULL, CONNECT_INTERACTIVE | CONNECT_COMMANDLINE);

		if(nStatus != NO_ERROR)
		{
			_tprintf(_T("Error:%d-%d\n"),(int)nStatus,GetLastError());
			return 255;
		}
	}

	/* Use GetVersionEx for anything that we are looking for locally */
	if(bLocal)
	{
		osvi->dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
		if(!GetVersionEx((LPOSVERSIONINFO)osvi))
		{
			_tprintf(_T("Failed to get local information\n"));
			return 255;
		}
	}
	else
	{
		nStatus = NetServerGetInfo(NULL,102,(LPBYTE *)&pBuf102);
		if (nStatus != NERR_Success)
		{
			_tprintf(_T("Failed to connection to remote machine\n"));
			return 255;
		}

	}

	if(Operations & GETTYPE_VER)
	{
		ret = GetVersionNumber(bLocal, osvi, pBuf102);
	}
	else if(Operations & GETTYPE_MAJV)
	{
		ret = GetMajValue(TRUE, bLocal, osvi, pBuf102);
	}
	else if(Operations & GETTYPE_MINV)
	{
		ret = GetMajValue(FALSE, bLocal, osvi, pBuf102);
	}
	else if(Operations & GETTYPE_ROLE)
	{
		ret = GetSystemRole(bLocal, osvi, pBuf102);
	}
	else if(Operations & GETTYPE_SP)
	{
		ret = GetServicePack(bLocal, osvi, pBuf102, ServerName);
	}
	else if(Operations & GETTYPE_BUILD)
	{
		ret = GetBuildNumber(bLocal, osvi, pBuf102);
	}
	else if(Operations & GETTYPE_TYPE)
	{
		ret = GetType(bLocal, osvi, pBuf102);
	}
	else if(Operations & GETTYPE_HELP)
	{
		wprintf(L"GETTYPE  [/ROLE | /SP | /VER | /MAJV | /MINV | /TYPE | /BUILD]");
		ret = 0;
	}
	else if(!Operations && bLocal)
	{
		/* FIXME: what happens when no flags except remote machine, does it
		it print this info for the remote server? */
		TCHAR HostName[1024] = _T("");
		TCHAR OSName[1024] = _T("");
		TCHAR VersionInfo[1024] = _T("");
		TCHAR Role[1024] = _T("");
		TCHAR Components[1024] = _T("");
		GetBasicInfo(osvi, HostName, OSName, VersionInfo, Role, Components);
		_tprintf(_T("\nHostname: %s\nName: %s\nVersion:%s\n") ,HostName, OSName, VersionInfo);
		_tprintf(_T("Role: %s\nComponent: %s\n"), Role, Components);
		ret = 0;
	}

	/* Clean up some stuff that that was opened */
	if(pBuf102)
		NetApiBufferFree(pBuf102);
	LocalFree(argv);
	if(!bLocal)
	{
		WNetCancelConnection2(RemoteResource,0,TRUE);
	}
	return ret;
}
