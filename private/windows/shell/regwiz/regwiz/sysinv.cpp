/*********************************************************************
Registration Wizard

sysinv.cpp
02/24/98  - Suresh Krishnan
07/20/98  - Suresh Krishnan
		    GetSCSIAdapterString() function is addded to get SCSI adapter details
	
(c) 1994-95 Microsoft Corporation
08/06/98   GetDisplayResolutionString() is  modified so it now gets Display Adapter with Resolution
**********************************************************************/
#include <tchar.h>
#include <Windows.h>
#include <stdio.h>
#include "sysinv.h"
#include "resource.h"


// The packed structures below get messed up with optimizations turned on
#pragma optimize( _T(""), off )

typedef BOOL   (APIENTRY *GETSYSTEMINVENTORY) ( INT ,LPTSTR );
HINSTANCE   hSiDllInst=NULL;
GETSYSTEMINVENTORY m_fp = NULL ;
BOOL WINAPI GetSystemInventory(INT type, LPTSTR szInventory)
{
	static int iEntry=0;
	if(!iEntry) {
		hSiDllInst = LoadLibrary(_T("SYSINV.DLL"));

		if(hSiDllInst == NULL ) {
			m_fp = NULL;
			iEntry = 1;
			return FALSE;
		}

	#ifdef UNICODE
		m_fp = (GETSYSTEMINVENTORY ) GetProcAddress(hSiDllInst, "GetSystemInventoryW");
	#else
		m_fp = (GETSYSTEMINVENTORY ) GetProcAddress(hSiDllInst, "GetSystemInventoryA");
	#endif
		iEntry = 1;
	}
	if(m_fp){
		return (*m_fp)(type ,szInventory);
	}else {
		return FALSE;
	}

}

int CheckSysInvDllPresent()
{
	int iRet;
	iRet = SYSINV_DLL_NOTPRESENT;
	HANDLE  hSi;
	hSi = LoadLibrary(_T("SYSINV.DLL"));
	if(hSi) {
		iRet = SYSINV_DLL_PRESENT;
	}
	return iRet;

}

void GetOEMString(HINSTANCE hInstance, LPTSTR szOEM)
/*********************************************************************
Returns a string containing the name of the Original Equipment
Manufacturer.
**********************************************************************/
{
	GetSystemInventory(INV_OEM, szOEM);
	
}


void GetProcessorTypeString(HINSTANCE hInstance, LPTSTR szProcessor)

{
	GetSystemInventory(INV_PROCESSORTYPE, szProcessor);
}

void GetTotalMemoryString(HINSTANCE hInstance, LPTSTR szTotalMemory)
{
	GetSystemInventory(INV_TOTALMEMORY,szTotalMemory);
}

void GetTotalHardDiskSpaceString(HINSTANCE hInstance, LPTSTR szTotalHardDiskSpace)
{
	GetSystemInventory(INV_TOTALHDSPACE,szTotalHardDiskSpace);
}

void GetDisplayResolutionString(HINSTANCE hInstance, LPTSTR szDisplayResolution)

{	
	GetSystemInventory(INV_DISPLAY_WITH_RESOLUTION,szDisplayResolution);
}


void GetDisplayColorDepthString(HINSTANCE hInstance, LPTSTR szDisplayColorDepth)
/*********************************************************************
Returns a string that describes the color depth (number of colors
available).
**********************************************************************/
{
	GetSystemInventory(INV_DISPCOLORDEPTH,szDisplayColorDepth);
}


void GetWindowsVersionString(HINSTANCE hInstance, LPTSTR szVersion)
/*********************************************************************
Returns a string describing the platform and verson of the currently
operating Windows OS.
**********************************************************************/
{
	GetSystemInventory(INV_WINVERSION,szVersion);
}


void GetNetworkCardString(HINSTANCE hInstance, LPTSTR szNetwork)
{
	GetSystemInventory(INV_NETCARD,szNetwork);
}


void GetModemString(HINSTANCE hInstance, LPTSTR szModem)
{
	GetSystemInventory(INV_MODEM,szModem);
}

void GetSCSIAdapterString(HINSTANCE hInstance, LPTSTR szScsi)
{
	GetSystemInventory(INV_SCSIADAPTER,szScsi);
}

void GetPointingDeviceString(HINSTANCE hInstance, LPTSTR szPointingDevice)
/*********************************************************************
Returns a string describing all pointing devices (mouse, tablet, etc.)
available.
**********************************************************************/
{

	GetSystemInventory(INV_POINTDEVICE,szPointingDevice);	

		
}



void GetCDRomString(HINSTANCE hInstance, LPTSTR szCDRom)
{
	GetSystemInventory(INV_CDROM,szCDRom);	
}



void GetSoundCardString(HINSTANCE hInstance, LPTSTR szSoundCard)
{

	GetSystemInventory(INV_SOUNDCARD,szSoundCard);	
}



void GetRemoveableMediaString(HINSTANCE hInstance, LPTSTR szRemoveableMedia, int iBufSize)
/*********************************************************************
Returns a string describing the capacity and format of removeable
drives.
**********************************************************************/
{

		GetSystemInventory(INV_REMOVEABLEMEDIA,szRemoveableMedia);	
}


BOOL IsCoProcessorAvailable(HINSTANCE hInstance)
/*********************************************************************
Returns TRUE if a co-processor is installed in the user's system.
**********************************************************************/
{
	TCHAR czRet[256];
	BOOL  bRet = TRUE;
	GetSystemInventory(INV_COPRECESSOR,czRet);
	if(czRet[0] == _T('\0')){
		bRet = FALSE;
	}
	return bRet;
}




void GetDisplayCharacteristics(PINT lpHorizResolution, PINT lpVertResolution,PINT lpColorDepth)
/*********************************************************************
Returns the horizontal and vertical resolution (in pixels) of the 
user's main screen, as well as the color depth (bits per pixel).

Note: NULL can be passed for any parameter that is not of interest.
**********************************************************************/
{
	HWND hwnd = GetDesktopWindow();
	HDC hdc = GetDC(hwnd);
	if (lpHorizResolution) *lpHorizResolution = GetDeviceCaps(hdc,HORZRES);
	if (lpVertResolution) *lpVertResolution = GetDeviceCaps(hdc,VERTRES);
	if (lpColorDepth) *lpColorDepth = GetDeviceCaps(hdc,BITSPIXEL);
	ReleaseDC(hwnd,hdc);
}


void GetWindowsVersion(LONG* lpPlatform, LONG* lpMajorVersion,LONG* lpMinorVersion,LONG* lpBuildNo)
/*********************************************************************
Returns integers representing the platform, major version number, and
minor version number of the currently running Windows OS.

Platform:
VER_PLATFORM_WIN32_NT:		Windows NT
VER_PLATFORM_WIN32s: 		Win32s with Windows 3.1
VER_PLATFORM_WIN32_WINDOWS:	Win32 on Windows 4.0 or later

Note: NULL can be passed for any parameter that is not of interest.
**********************************************************************/
{
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);
	if (lpMajorVersion) *lpMajorVersion = osvi.dwMajorVersion;
	if (lpMinorVersion) *lpMinorVersion = osvi.dwMinorVersion;
	if (lpPlatform) *lpPlatform = osvi.dwPlatformId;
	if (lpBuildNo) *lpBuildNo = osvi.dwBuildNumber;
}







#pragma optimize( _T(""), on )
