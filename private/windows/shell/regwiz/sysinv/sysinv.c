/*********************************************************************
 * Hardware inventory check.  Works with Register Wizard 
 *
 * 02/20/97 - Denny Dong	Take the code from Sysinv.cpp
 * Copyright (c) 1998   Microsoft Corporation
 * 7/20/98  - Modified to get driver file name for Mouse,Sound card along with device names
              SCSI Adapter is added to the system inventory list. 
			  List of  Devices where Driver file info is gathered
				1) Mouse ( Pointing Device)
				2) Sound Card
				3) SCSI Adapter
			 Dispaly resolution is changed to  give additional information about color depth	
*  8/6/98   Prefix the Display Adapter with Driver string in the color resolution   
   8/17/98  Display Color Depth bug if Color depth is 32 bits aor more is fixed.The value is increased to DWORD LONG for storing the value
   3/9/99  GetSystemInformation() care is teken to release SetupAPI.Dll 's Buffer 
   5/27/99 ProcessType info for ALPHA  will be taken from the folloeing Registry Key
   HKLM\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment 
   - PROCESSOR_ARCHITECTURE"
   - PROCESSOR_IDENTIFIER
 *********************************************************************/

#include <Windows.h>
#include <stdio.h>
#include "sysinv.h"
#include "resource.h"
#include "SETUPAPI.H"

// The packed structures below get messed up with optimizations turned on
#pragma optimize( _T(""), off )

typedef struct _DEVIOCTL_REGISTERS
{
    DWORD reg_EBX;
    DWORD_PTR reg_EDX;
    DWORD reg_ECX;
    DWORD reg_EAX;
    DWORD reg_EDI;
    DWORD reg_ESI;
    DWORD reg_Flags;
} DEVIOCTL_REGISTERS, *PDEVIOCTL_REGISTERS;

#define MAX_SEC_PER_TRACK	64
#pragma pack(1)
typedef struct _DEVICEPARAMS
{
	TBYTE	dpSpecFunc;
	TBYTE	dpDevType;
	WORD	dpDevAttr;
	WORD	dpCylinders;
	TBYTE	dpMediaType;
	WORD	dpBytesPerSec;
	TBYTE	dpSecPerClust;
	WORD	dpResSectors;
	TBYTE	dpFATS;
	WORD	dpRootDirEnts;
	WORD	dpSectors;
	TBYTE	dpMedia;
	WORD	dpFATsecs;
	WORD 	dpSecsPerTrack;
	WORD	dpHeads;
	DWORD	dpHiddenSecs;
	DWORD	dpHugeSectors;
    TBYTE    A_BPB_Reserved[6];			 // Unused 6 BPB bytes
    TBYTE    TrackLayout[MAX_SEC_PER_TRACK * 4 + 2];
}DEVICEPARAMS,*PDEVICEPARAMS;
#pragma pack()

#define VWIN32_DIOC_DOS_IOCTL 1
#define kDrive525_0360   0
#define kDrive525_1200   1
#define kDrive350_0720   2
#define kDrive350_1440   7
#define kDrive350_2880   9
#define kDriveFIXED      5
#define kDriveBadDrvNum  0xFF

// Dynamic Registry enumeration declarations
#define DYNDESC_BUFFERSIZE	128
static _TCHAR vrgchDynDataKey[] = _T("Config Manager\\Enum");
static _TCHAR vrgchLocalMachineEnumKey[] = _T("Enum");
static _TCHAR vrgchHardWareKeyValueName[] = _T("HardWareKey");
static _TCHAR vrgchDriverValueName[] = _T("Driver");
static _TCHAR vrgchDeviceDescValueName[] = _T("DeviceDesc");
static _TCHAR vrgchDynNetExclusion[] = _T("Dial-Up Adapter");
static _TCHAR vrgchHardwareIDValueName[] = _T("HardwareID");
static BOOL vfIsFPUAvailable = TRUE;
static _TCHAR vrgchDynProcessorName[DYNDESC_BUFFERSIZE];
typedef enum
{
	dynNet		= 0,
	dynModem	= 1,
	dynMouse	= 2,
	dynCDRom	= 3,	
	dynMedia	= 4,
	dynSCSI     = 5,  
	dynSystem	= 6,
	dynEnd
}DYN;

static _TCHAR vrgchDynKey[dynEnd][12] = 
{
	_T("Net"),
	_T("Modem"),
	_T("Mouse"),
	_T("CDROM"),
	_T("Media"),
	_T("SCSIAdapter"),
	_T("System")
};

static _TCHAR vrgchDynDesc[dynEnd][DYNDESC_BUFFERSIZE] =
{
	_T(""),
	_T(""),
	_T(""),
	_T(""),
	_T(""),
	_T("")
};

static HANDLE hInstance = NULL;
static TCHAR  sszDriverFilename[256];

// Private functions
void EnumerateDynamicDevices( void );
void ProcessSystemDevices(LPTSTR rgchSystemKey);
BOOL GetProcessorTypeStringFromRegistry(LPTSTR);
void GetProcessorTypeStringFromSystem(LPTSTR);
UINT GetDriveTypeInv(UINT nDrive);
BOOL GetDeviceParameters(PDEVICEPARAMS pDeviceParams, UINT nDrive);
BOOL DoIOCTL(PDEVIOCTL_REGISTERS preg);
void GetSystemInformation(LPCTSTR szDeviceID,LPTSTR szDeviceName, LPTSTR szDriverName);
BOOL WINAPI GetSystemInventoryA(INT type, LPSTR szInventory);


/*
 * Function:
 *	BOOL DllMain(HINSTANCE, DWORD, LPVOID)
 *
 * Purpose:
 *	Entry point of DLL.
 */
BOOL WINAPI DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		hInstance = hDll;
	return TRUE;
}

BOOL WINAPI GetSystemInventoryW(INT type, LPWSTR szInventory)
{

	/*char szInventory[1024];
	wszInventory[0] = 0;
	if (!GetSystemInventoryA(type, szInventory))
		return FALSE;
	if (type == INV_COPRECESSOR)
	{
		wszInventory[0] = szInventory[0];
		return TRUE;
	}
	if (szInventory[0] == 0)
		return TRUE;
	if (MultiByteToWideChar(CP_ACP,0,szInventory,-1,wszInventory,256) == 0)
		return FALSE;
	return TRUE;
	*/
	szInventory[0] = _T('\0');
	
	switch (type)
	{
	case INV_OEM:
		GetOEMString(szInventory);
		return TRUE;
	case INV_PROCESSORTYPE:
		GetProcessorTypeString(szInventory);
		return TRUE;
	case INV_TOTALMEMORY:
		GetTotalMemoryString(szInventory);
		return TRUE;
	case INV_TOTALHDSPACE:
		GetTotalHardDiskSpaceString(szInventory);
		return TRUE;
	case INV_DISPRESOLUTION:
		GetDisplayResolutionString(szInventory);
		return TRUE;
	case INV_DISPCOLORDEPTH:
		GetDisplayColorDepthString(szInventory);
		return TRUE;
	case INV_WINVERSION:
		GetWindowsVersionString(szInventory);
		return TRUE;
	case INV_NETCARD:
		GetNetworkCardString(szInventory);
		return TRUE;
	case INV_MODEM:
		GetModemString(szInventory);
		return TRUE;
	case INV_POINTDEVICE:
		GetPointingDeviceString(szInventory);
		return TRUE;
	case INV_CDROM:
		GetCDRomString(szInventory);
		return TRUE;
	case INV_SOUNDCARD:
		GetSoundCardString(szInventory);
		return TRUE;
	case INV_REMOVEABLEMEDIA:
		GetRemoveableMediaString(szInventory);
		return TRUE;
	case INV_COPRECESSOR:
		szInventory[0] = IsCoProcessorAvailable() ? 1 : 0;
		szInventory[1] = 0;
		return TRUE;
	case INV_SCSIADAPTER :
		GetScsiAdapterString(szInventory);
		return TRUE;
	case INV_DISPLAY_ADAPTER:
		GetDisplayAdapter(szInventory);
		return TRUE;
		break;
	case INV_DISPLAY_WITH_RESOLUTION:
		GetDisplayAdapterWithResolution(szInventory);
		return TRUE;
		break;
	default:
		break;
	}
	return FALSE;
}

BOOL WINAPI GetSystemInventoryA(INT type, LPSTR szInventory)
{
	BOOL bRet;
	int    iMaxOutStrLen;
	ULONG  ulNoOfChars;
	WCHAR  wszInventory[1024];
	iMaxOutStrLen = 256;

	bRet = GetSystemInventoryW(type, wszInventory);

	if(wszInventory[0]) {
		ulNoOfChars = wcslen(wszInventory)+1;
		memset((void *) szInventory,0,iMaxOutStrLen);
		if(WideCharToMultiByte(CP_ACP,0,wszInventory,ulNoOfChars,szInventory,
		iMaxOutStrLen,NULL,NULL) == 0) {
			//dwError = GetLastError();
			//
		}
		
	}else {
		// if empty string
		szInventory[0] = '\0';
	}
	return bRet;
}


/***************************************************************************
Returns TRUE if the file specified by the given pathname actually exists.
****************************************************************************/
BOOL FileExists(LPTSTR szPathName)
{
	SECURITY_ATTRIBUTES sa;
	HANDLE fileHandle;
	BOOL retValue;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	fileHandle = CreateFile(szPathName,GENERIC_READ,0,&sa,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (fileHandle == INVALID_HANDLE_VALUE)
	{
		retValue = FALSE;
	}
	else
	{
		retValue = TRUE;
		CloseHandle(fileHandle);
	}
	return retValue;
}

/*********************************************************************
Returns a string containing the name of the Original Equipment
Manufacturer.
**********************************************************************/
void GetOEMString(LPTSTR szOEM)
{
	_TCHAR szPathName[512];
	DWORD oemLen;
	UINT pathLen = GetSystemDirectory(szPathName, 256);
	szOEM[0] = 0;
	if (pathLen > 0)
	{
		_TCHAR szIniName[256];
		LoadString(hInstance,IDS_OEM_INIFILE,szIniName,256);
		_tcscat(szPathName,_T("\\"));
		_tcscat(szPathName,szIniName);

		if (FileExists(szPathName))
		{
			_TCHAR szIniSection[64];
			_TCHAR szIniKey[64];
			_TCHAR szDefault[28];
			_TCHAR szModelTmp[128];
			LoadString(hInstance,IDS_OEM_INISECTION,szIniSection,64);
			LoadString(hInstance,IDS_OEM_INIKEY,szIniKey,64);
			szDefault[0] = 0;
			oemLen = GetPrivateProfileString(szIniSection,szIniKey,szDefault,szOEM,256,szPathName);

			LoadString(hInstance,IDS_OEM_INIKEY2,szIniKey,64);
			szDefault[0] = 0;
			oemLen = GetPrivateProfileString(szIniSection,szIniKey,szDefault,szModelTmp,256,szPathName);

			if(oemLen)
			{
				_tcscat(szOEM,_T(" ,"));
				_tcscat(szOEM,szModelTmp);
			}

		}
   }
}


/*********************************************************************
Returns a string that describes the processor in the user's system:
- "80386"
- "80486"
- "PENTIUM"
- "INTEL860"
- "MIPS_R2000"
- "MIPS_R3000"
- "MIPS_R4000"
- "ALPHA_21064"
Note: you must allocate at least 64 bytes for the buffer pointed to
by the szProcessor parameter.
**********************************************************************/
void GetProcessorTypeString(LPTSTR szProcessor)
{
	
	_TCHAR szTmp[256];
	_TCHAR szData[256]; 
	_TCHAR szString[256]; 
	HKEY  hKey; 
	LONG regStatus;
	DWORD dwInfoSize;

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);

	szProcessor[0] = 0;
	if( PROCESSOR_ALPHA_21064 == systemInfo.dwProcessorType) {
	 
		// Alpha 
		// Default Value 
		LoadString(hInstance, IDS_PROCESSOR_ALPHA_21064,szProcessor,64);

		// Try to get from Registry
		LoadString(hInstance, IDS_ALPHA_PROCESSOR,szTmp,256);
		regStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szTmp, 0, KEY_READ, &hKey);
		if (regStatus != ERROR_SUCCESS) 
		return;
		
		dwInfoSize = 256;
		LoadString(hInstance, IDS_ALPHA_ARCHITECTURE, szString, 256);

		RegQueryValueEx(hKey, szString, NULL, 0, (LPBYTE)szData, &dwInfoSize);
		_tcscpy(szProcessor, szData);
		_tcscat(szProcessor, _T(", "));
		dwInfoSize = 256;
		
		LoadString(hInstance, IDS_ALPHA_IDENTIFIER, szString, 256);
		
		RegQueryValueEx(hKey, szString, NULL, 0, (LPBYTE)szData, &dwInfoSize);
		_tcscat(szProcessor, szData);
  	    RegCloseKey(hKey);



	}else {
		if (!GetProcessorTypeStringFromRegistry(szProcessor))
		GetProcessorTypeStringFromSystem(szProcessor);
	}
}


/*********************************************************************
Returns a string that describes the processor in the user's system:
- "80386"
- "80486"
- "PENTIUM"
- "INTEL860"
- "MIPS_R2000"
- "MIPS_R3000"
- "MIPS_R4000"
- "ALPHA_21064"
Note: you must allocate at least 64 bytes for the buffer pointed to
by the szProcessor parameter.
**********************************************************************/
void GetProcessorTypeStringFromSystem(LPTSTR szProcessor)
{
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	switch (systemInfo.dwProcessorType)
	{
		case PROCESSOR_INTEL_386:
			LoadString(hInstance, IDS_PROCESSOR_386,szProcessor,64);
			break;
		case PROCESSOR_INTEL_486:
			LoadString(hInstance, IDS_PROCESSOR_486,szProcessor,64);
			break;
		case PROCESSOR_INTEL_PENTIUM:
			LoadString(hInstance, IDS_PROCESSOR_PENTIUM,szProcessor,64);
			break;
/*		case PROCESSOR_INTEL_860:
			LoadString(hInstance, IDS_PROCESSOR_860,szProcessor,64);
			break;
		case PROCESSOR_MIPS_R2000:
			LoadString(hInstance, IDS_PROCESSOR_MIPS_R2000,szProcessor,64);
			break;
		case PROCESSOR_MIPS_R3000:
			LoadString(hInstance, IDS_PROCESSOR_MIPS_R3000,szProcessor,64);
			break;														   */
		case PROCESSOR_MIPS_R4000:
			LoadString(hInstance, IDS_PROCESSOR_MIPS_R4000,szProcessor,64);
			break;
		case PROCESSOR_ALPHA_21064:
			LoadString(hInstance, IDS_PROCESSOR_ALPHA_21064,szProcessor,64);
			break;
		default:
			szProcessor[0] = 0;
			break;
	}
}


/*********************************************************************
Retrieves the name of the processor in use from the Registry.

Returns:
FALSE if the proper key in the Registry does not exist.
**********************************************************************/
BOOL GetProcessorTypeStringFromRegistry(LPTSTR szProcessor)
{
	HKEY  hKey; 
	_TCHAR uszRegKey[256];
	LONG regStatus;
	DWORD dwInfoSize;
	LoadString(hInstance, IDS_PROCESSOR_ENTRY, uszRegKey, 256);

	regStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, uszRegKey, 0, KEY_READ, &hKey);
	if (regStatus != ERROR_SUCCESS) 
		return FALSE;
	else
	{
		_TCHAR szData[256]; 
		_TCHAR szString[256]; 
		dwInfoSize = 256;
		LoadString(hInstance, IDS_CPU_VENDOR_ENTRY, szString, 256);

		RegQueryValueEx(hKey, szString, NULL, 0, (LPBYTE)szData, &dwInfoSize);
		_tcscpy(szProcessor, szData);
		_tcscat(szProcessor, _T(", "));
		dwInfoSize = 256;
		
		LoadString(hInstance, IDS_CPU_ENTRY, szString, 256);
		
		RegQueryValueEx(hKey, szString, NULL, 0, (LPBYTE)szData, &dwInfoSize);
		_tcscat(szProcessor, szData);
  	    RegCloseKey(hKey);
	}
	return TRUE;
}

/*********************************************************************
Returns a string that describes the amount of physical RAM available.

Note: you must allocate at least 64 bytes for the buffer pointed to
by the szTotalMemory parameter.
**********************************************************************/
void GetTotalMemoryString(LPTSTR szTotalMemory)
{
	_TCHAR szSuffix[32];
	MEMORYSTATUS memoryStatus;
	DWORD_PTR totalRam;
	memoryStatus.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&memoryStatus);
	totalRam = memoryStatus.dwTotalPhys / 1024;
	LoadString(hInstance, IDS_SIZE_SUFFIX1, szSuffix, 31);
	_stprintf(szTotalMemory, _T("%li %s"), totalRam, szSuffix);
}

/*********************************************************************
Returns a string that describes the total amount of disk space
(in KB) available on all hard disk drives attached to the user's
system.

Note: you must allocate at least 64 bytes for the buffer pointed to
by the szTotalMemory parameter.
**********************************************************************/
void GetTotalHardDiskSpaceString(LPTSTR szTotalHardDiskSpace)
{
	_TCHAR szSuffix[32];
	LONG totalHardDiskSpace = GetTotalHardDiskSpace();
	LoadString(hInstance, IDS_SIZE_SUFFIX1, szSuffix, 31);
	_stprintf(szTotalHardDiskSpace, _T("%li %s"), totalHardDiskSpace, szSuffix);
}


/*********************************************************************
Returns a string that describes the horizontal x vertical resolution
(in pixels) of the user's main screen.
It also prefixes teh Display adapter Name
**********************************************************************/
void GetDisplayResolutionString(LPTSTR szDisplayResolution)
{
	int horizResolution, vertResolution;
	int colorBits;
	DWORDLONG colorDepth;
	_TCHAR szSuffix[24];
	char czDispAdapter[256];
	
	
	szSuffix[0] = _T('\0'); // 
	GetDisplayCharacteristics(&horizResolution, &vertResolution, NULL);
	// Color Depth 
	GetDisplayCharacteristics(NULL,NULL,&colorBits);
	colorDepth = (DWORDLONG) 1 << colorBits;
	
	szSuffix[0] = 0;
	if (colorBits > 15)
	{
		colorDepth = colorDepth / 1024;
		LoadString(hInstance,IDS_SIZE_SUFFIX2,szSuffix,24);
	}
	_stprintf(szDisplayResolution, _T("%i x %i x %I64d%s"), horizResolution, vertResolution, colorDepth,szSuffix);
}

void GetDisplayAdapterWithResolution( LPTSTR szDisplayWithResolution)
{
	TCHAR czDispAdapter[256];
	TCHAR czResolution[128];

	GetDisplayAdapter(czDispAdapter);
	GetDisplayResolutionString(czResolution);
	if(czDispAdapter[0] != _T('\0') )  {
		_tcscpy(szDisplayWithResolution,czDispAdapter);
		_tcscat(szDisplayWithResolution,_T("  "));
		_tcscat(szDisplayWithResolution,czResolution);
	}else {
		szDisplayWithResolution[0] = '\0';
	}

}

/*********************************************************************
Returns a string that describes the color depth (number of colors
available).
// We are  getting the 
**********************************************************************/
void GetDisplayColorDepthString(LPTSTR szDisplayColorDepth)
{
	int colorBits;
	LONG colorDepth;
	_TCHAR szSuffix[24];

	GetDisplayCharacteristics(NULL,NULL,&colorBits);
	colorDepth = 1 << colorBits;
	
	szSuffix[0] = 0;
	if (colorBits > 15)
	{
		colorDepth = colorDepth / 1024;
		LoadString(hInstance,IDS_SIZE_SUFFIX2,szSuffix,24);
	}
	_stprintf(szDisplayColorDepth,_T("%li%s"),colorDepth,szSuffix);
}


/*********************************************************************
Returns a string describing the platform and verson of the currently
operating Windows OS.
**********************************************************************/
void GetWindowsVersionString(LPTSTR szVersion)
{
	LONG platform, majorVersion, minorVersion, dwBuildNo;
	_TCHAR szPlatform[64];
	_TCHAR szOsName[128];
	HKEY  hKey; 
	_TCHAR uszRegKey[256];
	LONG dwStatus;
	DWORD dwInfoSize;
	_TCHAR szBuildNo[64];
	_TCHAR szString[64];
	int idsPlatform;

	GetWindowsVersion(&platform, &majorVersion, &minorVersion, &dwBuildNo);

	if (platform == VER_PLATFORM_WIN32_WINDOWS)
		idsPlatform = IDS_PLATFORM_WIN95;
	else if (platform == VER_PLATFORM_WIN32_NT)
		idsPlatform = IDS_PLATFORM_WINNT;
	else
		idsPlatform = IDS_PLATFORM_WIN;
	
	if(	idsPlatform == IDS_PLATFORM_WIN95)
	{
		

		_tcscpy(uszRegKey, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"));
		dwStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, uszRegKey, 0, KEY_READ, &hKey);

		if (dwStatus != ERROR_SUCCESS) 
			LoadString(hInstance, idsPlatform, szPlatform, sizeof(szPlatform));
		else
		{
			dwInfoSize = 64;
			LoadString(hInstance, IDS_PRODUCT_NAME, szString, 64);
			RegQueryValueEx(hKey, szString, NULL, 0, (LPBYTE)szPlatform, &dwInfoSize);
  		    dwInfoSize = 64;
			LoadString(hInstance, IDS_PRODUCT_VERSION, szString, 64);
			RegQueryValueEx(hKey, szString, NULL, 0, (LPBYTE)szBuildNo,&dwInfoSize);
	    	RegCloseKey(hKey);
		}

		LoadString(hInstance,IDS_PRODUCT_VERSION_DISPLAY,szString,64);
		_tcscpy(szVersion,szPlatform);
		_tcscat(szVersion,szString);
		_tcscat(szVersion,szBuildNo);
	}
	else
	{
		// get OS name from Registry
		_tcscpy(uszRegKey, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"));
		dwStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, uszRegKey, 0, KEY_READ, &hKey);
		if (dwStatus == ERROR_SUCCESS) {
			dwInfoSize = 128;
			LoadString(hInstance, IDS_PRODUCT_NAME, szString, 64);
			RegQueryValueEx(hKey, szString, NULL, 0, (LPBYTE)szOsName, &dwInfoSize);
	       	RegCloseKey(hKey);
		}


		LoadString(hInstance,idsPlatform,szPlatform,sizeof(szPlatform));
		_stprintf(szVersion,szPlatform,szOsName,dwBuildNo);
	}
}


/*********************************************************************
Returns a string describing the network card installed.  If no card
is installed, an empty string will be returned.
**********************************************************************/
void GetNetworkCardString(LPTSTR szNetwork)
{
	sszDriverFilename[0] = _T('\0');
	GetSystemInformation(_T("net"), szNetwork, sszDriverFilename);
	_tcscpy(vrgchDynDesc[dynNet],szNetwork);
}


/*********************************************************************
Returns a string describing the modem (if any) installed. If no modem
is installed, an empty string will be returned.
**********************************************************************/
void GetModemString(LPTSTR szModem)
{

	sszDriverFilename[0] = _T('\0');
	GetSystemInformation(_T("modem"), vrgchDynDesc[dynModem],sszDriverFilename);
	_tcscpy(szModem, vrgchDynDesc[dynModem]);
	

}


/*********************************************************************
Returns a string describing all pointing devices (mouse, tablet, etc.)
available.
**********************************************************************/
void GetPointingDeviceString(LPTSTR szPointingDevice)
{
	TCHAR czTemp[256];
	sszDriverFilename[0] = _T('\0');
	GetSystemInformation(_T("mouse"),vrgchDynDesc[dynMouse], sszDriverFilename);
	_tcscpy(szPointingDevice,vrgchDynDesc[dynMouse]);

	if( sszDriverFilename[0] != _T('\0')) {
		// Copy the Driver file Name
		_stprintf(czTemp,_T("  (%s.sys) "),sszDriverFilename);   
		_tcscat(szPointingDevice,czTemp);
	}
	_tcscpy(vrgchDynDesc[dynModem],szPointingDevice);

}


/*********************************************************************
Returns a string describing any CD-Rom devices installed.  If no
CD-ROM device is installed, an empty string will be returned.
**********************************************************************/
void GetCDRomString(LPTSTR szCDRom)
{
	sszDriverFilename[0] = _T('\0');
	GetSystemInformation(_T("cdrom"),vrgchDynDesc[dynCDRom],sszDriverFilename);
	_tcscpy(szCDRom,vrgchDynDesc[dynCDRom]);
}

/*********************************************************************
Returns a string describing any sound card with driver  installed.  If none are
installed, an empty string will be returned.
**********************************************************************/
void GetSoundCardString(LPTSTR szSoundCard)
{
	TCHAR czTemp[256];
	sszDriverFilename[0] = _T('\0');
	GetSystemInformation(_T("media"),vrgchDynDesc[dynMedia],sszDriverFilename);
	_tcscpy(szSoundCard,vrgchDynDesc[dynMedia]);

	if( sszDriverFilename[0] != _T('\0')) {
		// Copy the Driver file Name
		_stprintf(czTemp,_T("  (%s.sys) "),sszDriverFilename);   
		_tcscat(szSoundCard,czTemp);
	}
	_tcscpy(vrgchDynDesc[dynMedia],szSoundCard);
}

void GetDisplayAdapter( LPTSTR szDisplayAdapter)
{
	TCHAR czTemp[256];
	sszDriverFilename[0] = _T('\0');
	GetSystemInformation(_T("Display"),szDisplayAdapter,sszDriverFilename);
	

	if( sszDriverFilename[0] != _T('\0')) {
		// Copy the Driver file Name
		_stprintf(czTemp,_T("  (%s.sys) "),sszDriverFilename);   
		_tcscat(szDisplayAdapter,czTemp);
	}
}

/*
	Returns SCSI Adapter with Driver name persent in the system
*/
void GetScsiAdapterString(LPTSTR szScsiAdapter)
{
	TCHAR czTemp[256];
	sszDriverFilename[0] = _T('\0');
	GetSystemInformation(vrgchDynKey[dynSCSI],vrgchDynDesc[dynSCSI], sszDriverFilename);
	_tcscpy(szScsiAdapter,vrgchDynDesc[dynSCSI]);

	if( sszDriverFilename[0] != _T('\0')) {
		// Copy the Driver file Name
		_stprintf(czTemp,_T("  (%s.sys) "),sszDriverFilename);   
		_tcscat(szScsiAdapter,czTemp);
	}
	_tcscpy(vrgchDynDesc[dynSCSI],szScsiAdapter);

}

/*	Value  -- > "CurrentDriveLetterAssignment"		Data -- > "A"
	Value  -- > "Removable"							Data -- > 01
	Value  -- > "Class"								Data -- > "DiskDrive"*/
#define     REGFIND_ERROR      1
#define     REGFIND_RECURSE    2
#define     REGFIND_FINISH     3

int RegFindValueInAllSubKey(HKEY key, LPCTSTR szSubKeyNameToFind, LPCTSTR szValueToFind, LPTSTR szIdentifier, int nType)
{
	DWORD   dwRet = ERROR_PATH_NOT_FOUND, dwIndex, dwSubkeyLen;
	TCHAR   szSubKey[256], szFloppy[256];
	BOOL    bType = FALSE, bRemovable = FALSE, bPrevMassStorage, bPrevFloppy;
	HKEY    hKey;
	static BOOL bMassStorage = FALSE;
	static BOOL bFloppy = FALSE;
	
	bPrevMassStorage =	bMassStorage;
	bPrevFloppy		=	bFloppy;

	if (szSubKeyNameToFind != NULL)
		dwRet = RegOpenKeyEx(key, szSubKeyNameToFind, 0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &hKey);

	if (dwRet == ERROR_SUCCESS)
	{
		dwIndex = 0;
        while (dwRet == ERROR_SUCCESS )
        {
            dwSubkeyLen = 256;
			dwRet = RegEnumKeyEx(hKey, dwIndex, szSubKey, &dwSubkeyLen,
                           NULL, NULL, NULL, NULL);
 
            if (dwRet == ERROR_NO_MORE_ITEMS)
            {
				_TCHAR		valueName[80];
				DWORD		valueNameSize,valueSize,n = 0;
				TBYTE		value[80];
				
                do
				{
					valueNameSize=80* sizeof(_TCHAR);
					valueSize=80* sizeof(TBYTE);
					dwRet = RegEnumValue(hKey, n, valueName, &valueNameSize,
										 NULL, NULL, (LPBYTE)value, &valueSize);
					if (dwRet == ERROR_SUCCESS)
					{
						if (nType == 1)
						{
							if (!_tcscmp(valueName,_T("Type"))) 
							{
								if (!_tcscmp(szValueToFind,(LPCTSTR)value))
									bType = TRUE;
							}
							if (!_tcscmp(valueName,_T("Identifier"))) 
								_tcscpy(szIdentifier,(LPCTSTR)value);
						}
						else if(nType == 2)
						{
							if (!_tcscmp(valueName,_T("Class"))) 
							{
								if (!_tcscmp(szValueToFind,(LPCTSTR)value))
									bType = TRUE;
							}
							if (!_tcscmp(valueName,_T("DeviceDesc"))) 
							{
// bFloppy and bMassStorage are used for handling the conditions when there are multiple 
// Floppy and mass storage media present.
								_tcscpy(szFloppy,(LPCTSTR)value);
								_tcsupr(szFloppy);
								if(_tcsstr(szFloppy,_T("FLOPPY")) != NULL)
								{
									if(!bFloppy)
									{
										_tcscpy(szFloppy,(LPCTSTR)value);
										bFloppy = TRUE;
									}
								}
								else
// if it is not removable or it is a cdrom the condition for type and removable 
// takes care of it.
								{
									if(!bMassStorage)
										bMassStorage = TRUE;
								}

							}
							if (!_tcscmp(valueName,_T("Removable"))) 
							{
								if (*value == 0x01 )
									bRemovable = TRUE;
							}
						}
						n++;
					}

				} while (dwRet == ERROR_SUCCESS);

				if (nType == 1)
				{
					if(bType)
						return REGFIND_FINISH;
					else
						return REGFIND_RECURSE;
				}
				else if (nType == 2)
				{
					if( bType && bRemovable )
					{
						if (bFloppy != bPrevFloppy )
							_tcscpy(szIdentifier,szFloppy);	
						if (bFloppy && bMassStorage)
						{
							_TCHAR szMassString[64];
							LoadString(hInstance,IDS_MASS_STRORAGE_ENTRY,szMassString,64);
							_tcscat(szIdentifier,szMassString);	
							return REGFIND_FINISH;
						}
						return REGFIND_RECURSE;
					}
// The bMassStorage flag has to be reset to the previous state. 
					else
					{
						bMassStorage = bPrevMassStorage;
						if(bFloppy != bPrevFloppy)
							bFloppy = bPrevFloppy;
						return REGFIND_RECURSE;
					}
				}            
			}
            else
			{
				if (dwRet == ERROR_SUCCESS)
				{
					int nStatus;
					nStatus = RegFindValueInAllSubKey(hKey, szSubKey, szValueToFind, szIdentifier, nType);

					switch(nStatus)
					{
						case REGFIND_FINISH:
							return REGFIND_FINISH;
						case REGFIND_ERROR:
							return REGFIND_ERROR;
						default :
							if (bFloppy != bPrevFloppy)
								bPrevFloppy = bFloppy;
							break;
					}
					dwIndex++;
				}
			}
		}
		RegCloseKey(hKey);
	}
 
	return REGFIND_ERROR;
}

/*********************************************************************
Returns a string describing the capacity and format of removeable
drives.
**********************************************************************/
void GetRemoveableMediaString(LPTSTR szRemoveableMedia)
{
	LONG platform, majorVersion, minorVersion, dwBuildNo;
	GetWindowsVersion(&platform, &majorVersion, &minorVersion, &dwBuildNo);

	if (platform != VER_PLATFORM_WIN32_NT)
	{
		_TCHAR szSubKey[64];
		_TCHAR szSubKeyValue[64];
		LoadString(hInstance, IDS_REMOVABLE_MEDIA_ENTRY, szSubKey, 64);
		LoadString(hInstance, IDS_REMOVABLE_MEDIA_VALUE, szSubKeyValue, 64);
		RegFindValueInAllSubKey(HKEY_LOCAL_MACHINE,szSubKey,szSubKeyValue,szRemoveableMedia,2);
	}
	else
	{
		UINT driveType;
		_TCHAR szDrive[64];
		UINT nDrive;
		const iBufSize = 256;
		szRemoveableMedia[0] = 0;
		for (nDrive = 1; nDrive <= 26; nDrive++)
		{
			szDrive[0] = 0;
			driveType = GetDriveTypeInv(nDrive);
			switch (driveType)
			{
				case kDrive525_0360:
					LoadString(hInstance, IDS_DRV525_0360, szDrive, 64);
					break;
				case kDrive525_1200:
					LoadString(hInstance, IDS_DRV525_1200, szDrive, 64);
					break;
				case kDrive350_0720:
					LoadString(hInstance, IDS_DRV350_0720, szDrive, 64);
					break;
				case kDrive350_1440:
					LoadString(hInstance, IDS_DRV350_1440, szDrive, 64);
					break;
				case kDrive350_2880:
					LoadString(hInstance, IDS_DRV350_2880, szDrive, 64);
					break;
			}
			if (szDrive[0])
			{
				_TCHAR szFormattedDrive[70];
				int iNewStrLen;
				wsprintf(szFormattedDrive,_T("%c: %s"),_T('A') + nDrive - 1,szDrive);
				iNewStrLen = (_tcslen(szRemoveableMedia) +1+ _tcslen(szFormattedDrive) + 1);
				if (iNewStrLen < iBufSize)
				{
					if (szRemoveableMedia[0])
						_tcscat(szRemoveableMedia,_T(", ")); // We added 2 to iNewStrLen above to account to this
					_tcscat(szRemoveableMedia,szFormattedDrive);
				}
			}
		}
	}
}


/*********************************************************************
Returns TRUE if a co-processor is installed in the user's system.
**********************************************************************/
BOOL IsCoProcessorAvailable(void)
{
	EnumerateDynamicDevices();
	return vfIsFPUAvailable;
}


/**********************************************************************
Determines the value associated with the specified Registration
Database key and value name.

Returns:
	The cb of the key data if successful, 0 otherwise.
Notes:
	If hRootKey is NULL, HKEY_CLASSES_ROOT is used for the root
***********************************************************************/
UINT GetRegKeyValue32(HKEY hRootKey, LPTSTR const cszcSubKey, LPTSTR const cszcValueName,
					  PDWORD pdwType, PTBYTE pbData, UINT cbData )
{
	HKEY hSubKey;
	LONG lErr;
	DWORD cbSize = (DWORD)cbData;

	if (hRootKey == NULL)
		hRootKey = HKEY_CLASSES_ROOT;

	lErr = RegOpenKeyEx(hRootKey, cszcSubKey, 0, KEY_READ, &hSubKey);
	if (lErr != ERROR_SUCCESS)
	{
		pdwType[0] = 0;
		return 0;	/* Return 0 if the key doesn't exist */
	}

	lErr = RegQueryValueEx(hSubKey, (LPTSTR)cszcValueName, NULL, pdwType, (LPBYTE)pbData,
						   &cbSize);
	RegCloseKey(hSubKey);
	if (lErr != ERROR_SUCCESS)
	{
		pdwType[0] = 0;
		return 0;	/* Return 0 if the value doesn't exist */
	}

	return (UINT)cbSize;
}

/*********************************************************************
Enumerates through the HKEY_DYN_DATA\Config Manager\Enum branch of
the registry, and retrieves device information for all currently
installed Net cards, modems, pointing devices, CDROMs, and sound
cards.  All this information is stored in the static vrgchDynDesc
global array.
**********************************************************************/
void EnumerateDynamicDevices(void)
{
	HKEY hKey;
	// Open the "HKEY_DYN_DATA\Config Manager\Enum" subkey.
	LONG regStatus = RegOpenKeyEx(HKEY_DYN_DATA, vrgchDynDataKey, 0, KEY_READ, &hKey);
	if (regStatus == ERROR_SUCCESS)
	{
		DWORD dwIndex = 0;
		_TCHAR rgchSubkey[256];
		_TCHAR rgchValue[256];
		DWORD dwSubkeySize;
		LONG lEnumErr;
		DWORD dwType;
		DWORD dwValueSize;
		do
		{
			// Enumerate "HKEY_DYN_DATA\Config Manager\Enum\Cxxxxxxx"
			FILETIME ftLastWrite;
			dwSubkeySize = sizeof(rgchSubkey);
			lEnumErr = RegEnumKeyEx(hKey, dwIndex++, rgchSubkey, &dwSubkeySize,
									NULL,NULL,NULL,&ftLastWrite);
			if (lEnumErr == ERROR_SUCCESS)
			{
				// From each subkey, read the value from the "HardWareKey" value name,
				// and make a new HKEY_LOCAL_MACHINE subkey out of it.
				dwValueSize = GetRegKeyValue32(hKey, rgchSubkey, vrgchHardWareKeyValueName, &dwType,
								(PTBYTE)rgchValue, sizeof(rgchValue) );
				if (dwValueSize > 0 && dwType == REG_SZ)
				{
					_TCHAR rgchDriverKey[256];
					wsprintf(rgchDriverKey,_T("%s\\%s"),vrgchLocalMachineEnumKey,rgchValue);

					// From our HKEY_LOCAL_MACHINE subkey, read the value from the "Driver"
					// value name.
					dwValueSize = GetRegKeyValue32(HKEY_LOCAL_MACHINE, rgchDriverKey, vrgchDriverValueName,
												&dwType, (PTBYTE) rgchValue, sizeof(rgchValue) );
					if (dwValueSize > 0  && dwType == REG_SZ)
					{
						// Get the "main" subkey out of the "driver" value (which is of the
						// form "<main>\xxxx").
						LPTSTR sz = rgchValue;
						WORD wDynIndex = 0;
						BOOL fMatch = FALSE;

						while (*sz && *sz != _T('\\'))
							sz = _tcsinc(sz);
						*sz = 0;

						// If the "main" subkey matches any of our desired device types,
						// we get the description of that device from the "DriverDesc"
						// name value field, and save it in our device array.
						while (wDynIndex < dynEnd && fMatch == FALSE)
						{
							if (vrgchDynDesc[wDynIndex][0] == 0)
							{				
								if (_tcsicmp(vrgchDynKey[wDynIndex], rgchValue) == 0)
								{
									if (wDynIndex == dynSystem)
									{
										ProcessSystemDevices(rgchDriverKey);
										fMatch = TRUE;
									}
									else
									{
										dwValueSize = GetRegKeyValue32(HKEY_LOCAL_MACHINE,rgchDriverKey,
											vrgchDeviceDescValueName, &dwType, (PTBYTE) rgchValue, 
											sizeof(rgchValue) );
										if (dwValueSize > 0  && dwType == REG_SZ)
										{
											if (wDynIndex != dynNet || _tcsicmp(vrgchDynNetExclusion,
												rgchValue) != 0)
											{
												// In case the description value is bigger than our 
												// buffer, truncate it to fit.
												if (DYNDESC_BUFFERSIZE < sizeof(rgchValue))
													rgchValue[DYNDESC_BUFFERSIZE - (1*sizeof(_TCHAR))] = 0;
												_tcscpy(vrgchDynDesc[wDynIndex],rgchValue);
												fMatch = TRUE;
											}
										}
									}
								}
							}
							wDynIndex++;
						}
					}
				}
			}
		}while (lEnumErr == ERROR_SUCCESS);
	}
}


/*********************************************************************
Called when EnumerateDynamicDevices detects a "system" device (i.e.
a processor or FPU entry).  The string passed in rgchSystemKey is the
name of the HKEY_LOCAL_MACHINE subkey under which the "system" device
was found.
**********************************************************************/
void ProcessSystemDevices(LPTSTR rgchSystemKey)
{
	_TCHAR rgchValue[256];
	DWORD dwType, dwValueSize;
	// If we've got all the information we can use, we can bail out immediately
	if (vfIsFPUAvailable == TRUE && vrgchDynProcessorName[0] != 0)
		return;

	dwValueSize = GetRegKeyValue32(HKEY_LOCAL_MACHINE, rgchSystemKey, vrgchHardwareIDValueName,
								&dwType, (PTBYTE) rgchValue, sizeof(rgchValue));
	if (dwValueSize > 0 && dwType == REG_SZ)
	{
		if (_tcsstr(rgchValue,_T("*PNP0C04")))
		{
			vfIsFPUAvailable = TRUE;
		}
		else if (_tcsstr(rgchValue,_T("*PNP0C01")))
		{
			dwValueSize = GetRegKeyValue32(HKEY_LOCAL_MACHINE,rgchSystemKey,_T("CPU"),&dwType, 
				(PTBYTE) vrgchDynProcessorName, sizeof(vrgchDynProcessorName) );
		}
	}					
}


/*********************************************************************
Returns a LONG value representing the total amount of disk space
(in KB) available on all hard disk drives attached to the user's
system.
**********************************************************************/
LONG GetTotalHardDiskSpace(void)
{
	_TCHAR szDrivesBuffer[256];
	DWORD bufferLen = GetLogicalDriveStrings(256, szDrivesBuffer);
	LPTSTR szDrive = szDrivesBuffer;
	LONG totalHardDiskSpace = 0;
	while (szDrive[0] != 0)
	{
		UINT driveType = GetDriveType(szDrive);
		if (driveType == DRIVE_FIXED)
		{
			DWORD  sectorsPerCluster; 		
			DWORD  bytesPerSector;
			DWORD  freeClusters;
			DWORD  clusters;
			LONG kilobytesPerCluster;

			GetDiskFreeSpace(szDrive,&sectorsPerCluster,&bytesPerSector,&freeClusters,&clusters);
			kilobytesPerCluster = (bytesPerSector * sectorsPerCluster)/1024;
			totalHardDiskSpace += kilobytesPerCluster * clusters;
		}
		szDrive += ((_tcslen(szDrive)+1) );
	}
	return totalHardDiskSpace;
}


/*********************************************************************
Returns the horizontal and vertical resolution (in pixels) of the 
user's main screen, as well as the color depth (bits per pixel).

Note: NULL can be passed for any parameter that is not of interest.
**********************************************************************/
void GetDisplayCharacteristics(PINT lpHorizResolution, PINT lpVertResolution,PINT lpColorDepth)
{
	HWND hwnd = GetDesktopWindow();
	HDC hdc = GetDC(hwnd);
	if (lpHorizResolution) *lpHorizResolution = GetDeviceCaps(hdc,HORZRES);
	if (lpVertResolution) *lpVertResolution = GetDeviceCaps(hdc,VERTRES);
	if (lpColorDepth) *lpColorDepth = GetDeviceCaps(hdc,BITSPIXEL);
	ReleaseDC(hwnd,hdc);
}


/*********************************************************************
Returns integers representing the platform, major version number, and
minor version number of the currently running Windows OS.

Platform:
VER_PLATFORM_WIN32_NT:		Windows NT
VER_PLATFORM_WIN32s: 		Win32s with Windows 3.1
VER_PLATFORM_WIN32_WINDOWS:	Win32 on Windows 4.0 or later

Note: NULL can be passed for any parameter that is not of interest.
**********************************************************************/
void GetWindowsVersion(LONG* lpPlatform, LONG* lpMajorVersion,LONG* lpMinorVersion,LONG* lpBuildNo)
{
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);
	if (lpMajorVersion) *lpMajorVersion = osvi.dwMajorVersion;
	if (lpMinorVersion) *lpMinorVersion = osvi.dwMinorVersion;
	if (lpPlatform) *lpPlatform = osvi.dwPlatformId;
	if (lpBuildNo) *lpBuildNo = osvi.dwBuildNumber;
}


/*********************************************************************
For the disk drive specified by the nDrive parameter (1 = A, 2 = B, 
etc), GetDriveTypeInv returns a code specifying the drive format.  The 
returned value will be one of the following:

driveSize:
- kDrive525_0360:	5.25", 360K floppy
- kDrive525_0720:	5.25", 720K floppy
- kDrive350_0720:	3.5", 720K floppy
- kDrive350_1440:	3.5", 1.4M floppy
- kDrive350_2880:	3.5", 2.88M floppy
- kDriveFixed:		Hard disk, any size
- kDriveBadDrvNum:	Bad drive number
**********************************************************************/
UINT GetDriveTypeInv(UINT nDrive)
{
	DEVICEPARAMS deviceParams;

	// Must initialize dpDevType, because if nDrive refers to a network
	// drive or a drive letter with no volume attached, DeviceIOControl
	// does not return an error - it just doesn't change .dpDevType at
	// all.
	deviceParams.dpDevType = kDriveBadDrvNum;
	GetDeviceParameters(&deviceParams,nDrive);
	return deviceParams.dpDevType;
}


/*********************************************************************
Returns a block of device parameters for the drive specified by the
nDrive parameter (a zero-based index).
**********************************************************************/
BOOL GetDeviceParameters(PDEVICEPARAMS pDeviceParams, UINT nDrive)
{
    DEVIOCTL_REGISTERS reg;

    reg.reg_EAX = 0x440D;      			 /* IOCTL for block devices */
    reg.reg_EBX = nDrive;      			 /* zero-based drive ID     */
    reg.reg_ECX = 0x0860;      			 /* Get Device Parameters command    */
    reg.reg_EDX = (DWORD_PTR) pDeviceParams; /* receives device parameters info  */

    if (!DoIOCTL(&reg))
        return FALSE;

    if (reg.reg_Flags & 0x8000) /* error if carry flag set */
        return FALSE;

    return TRUE;
}



/*********************************************************************
Performs an IOCTL (Int21h) call via the System virtual device driver.
**********************************************************************/
BOOL DoIOCTL(PDEVIOCTL_REGISTERS preg)
{
    HANDLE hDevice;
    BOOL fResult;
    DWORD cb;

    preg->reg_Flags = 0x8000; /* assume error (carry flag set) */

	 hDevice = CreateFile(_T("\\\\.\\vxdfile"),
        GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
        (LPSECURITY_ATTRIBUTES) NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, (HANDLE) NULL);

    if (hDevice == (HANDLE) INVALID_HANDLE_VALUE) 
        return FALSE;
    else
    { 
        fResult = DeviceIoControl(hDevice, VWIN32_DIOC_DOS_IOCTL,
								  preg, sizeof(*preg), preg, sizeof(*preg), &cb, 0);
        if (!fResult)
            return FALSE;
    }    
                                        
    CloseHandle(hDevice);

    return TRUE;
}

void GetSystemInformation(LPCTSTR szDeviceID, LPTSTR szDeviceName, LPTSTR szDriverName)
{
	HDEVINFO hDevInfo;
	DWORD dwMemberIndex = 0;
	SP_DEVINFO_DATA DeviceInfoData;
	DWORD dwPropertyRegDataType;
	DWORD dwPropertyBufferSize = 256;
	_TCHAR szPropertyBuffer[256];
	DWORD dwRequiredSize;
	DWORD dwReqSize;
	DWORD dwError = 0;
	DWORD dwClassGuidListSize = 256;
	GUID ClassGuidList[256];
	GUID * pGUID;
	DWORD i;
	
	_tcscpy(szPropertyBuffer,_T(""));

	SetupDiClassGuidsFromName(szDeviceID, ClassGuidList, dwClassGuidListSize, &dwRequiredSize);
	pGUID = ClassGuidList;
	DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	for (i = 0; i < dwRequiredSize; i++)
	{
		hDevInfo  = NULL;
		hDevInfo = SetupDiGetClassDevs(pGUID++,	NULL, NULL,	DIGCF_PRESENT);
		dwMemberIndex = 0;
		do
		{
			BOOL bRet = SetupDiEnumDeviceInfo(hDevInfo, dwMemberIndex++, &DeviceInfoData);
			if (bRet == TRUE)
			{ 
				bRet = SetupDiGetDeviceRegistryProperty
						(hDevInfo, &DeviceInfoData, SPDRP_DEVICEDESC,
						 &dwPropertyRegDataType, /* optional */
						 (PBYTE)szPropertyBuffer, dwPropertyBufferSize,
						 &dwReqSize /* optional */
						); 
			
				if(!_tcscmp(szDeviceID,_T("net")))
				{
					if(!_tcsnicmp(szPropertyBuffer,_T("Dial-Up"),7))
						continue;
					if(!_tcsnicmp(szPropertyBuffer,_T("Microsoft Virtual Private Networking"),36))
						continue;
				}
				bRet = SetupDiGetDeviceRegistryProperty
						(hDevInfo, &DeviceInfoData, SPDRP_SERVICE,
						 &dwPropertyRegDataType, /* optional */
						 (PBYTE)szDriverName, dwPropertyBufferSize,
						 &dwReqSize /* optional */
						); 
				break;
			}
			else
				dwError = GetLastError();
		}
		while( dwError != ERROR_NO_MORE_ITEMS);
		if(hDevInfo != NULL) {
			SetupDiDestroyDeviceInfoList(hDevInfo);
		}	

	}
	_tcscpy(szDeviceName,szPropertyBuffer);
}

#pragma optimize( _T(""), on )
