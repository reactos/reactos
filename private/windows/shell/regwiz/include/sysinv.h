#ifndef __sysinv__
#define __sysinv__

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * Function:
 *	BOOL WINAPI GetSystemInventory(INT type, LPTSTR szInventory);
 *
 * Description:
 *	Get system inventory information by type caller asked
 *
 * Parameters:
 *	INT type:	specify what type information caller.  Should be one of INV_* constants
 *	LPTSTR szInventory:	The description of specific item information on return.  The contents depends on item type.
 *						The length of the buffer has to be at least 256.
 *
 * Return Value:
 *	TRUE on success.
 *	FALSE on failure.  szInventory[0] is also assigned 0
 */
BOOL WINAPI GetSystemInventory(INT, LPTSTR);

#define SYSINV_DLL_PRESENT  1
#define SYSINV_DLL_NOTPRESENT  2

int  CheckSysInvDllPresent();

void GetOEMString(HINSTANCE hInstance, LPTSTR szOEM);
void GetProcessorTypeString(HINSTANCE hInstance, LPTSTR szProcessor);
void GetTotalMemoryString(HINSTANCE hInstance, LPTSTR szTotalMemory);
void GetTotalHardDiskSpaceString(HINSTANCE hInstance, LPTSTR szTotalHardDiskSpace);
void GetDisplayResolutionString(HINSTANCE hInstance, LPTSTR szDisplayResolution);
void GetDisplayColorDepthString(HINSTANCE hInstance, LPTSTR szDisplayColorDepth);
void GetWindowsVersionString(HINSTANCE hInstance, LPTSTR szVersion);
void GetNetworkCardString(HINSTANCE hInstance, LPTSTR szNetwork);
void GetModemString(HINSTANCE hInstance, LPTSTR szModem);
void GetPointingDeviceString(HINSTANCE hInstance, LPTSTR szPointingDevice);
void GetCDRomString(HINSTANCE hInstance, LPTSTR szCDRom);
void GetSoundCardString(HINSTANCE hInstance, LPTSTR szSoundCard);
void GetRemoveableMediaString(HINSTANCE hInstance, LPTSTR szRemoveableMedia, int iBufSize);
void GetSCSIAdapterString(HINSTANCE hInstance, LPTSTR szScsi);

BOOL IsCoProcessorAvailable( HINSTANCE hInstance );
LONG GetTotalHardDiskSpace( void );
void GetDisplayCharacteristics(PINT horizResolution, PINT vertResolution,PINT colorDepth);
void GetWindowsVersion(LONG* lpPlatform, LONG* lpMajorVersion,LONG* lpMinorVersion,LONG* lpBuildNumber);

#define INV_OEM				1	// szInventory: Descriptive string
#define INV_PROCESSORTYPE	2	// szInventory: Descriptive string
#define INV_TOTALMEMORY		3	// szInventory: Descriptive string
#define INV_TOTALHDSPACE	4	// szInventory: Descriptive string
#define INV_DISPRESOLUTION	5	// szInventory: Descriptive string
#define INV_DISPCOLORDEPTH	6	// szInventory: Descriptive string
#define INV_WINVERSION		7	// szInventory: Descriptive string
#define INV_NETCARD			8	// szInventory: Descriptive string
#define INV_MODEM			9	// szInventory: Descriptive string
#define INV_POINTDEVICE		10	// szInventory: Descriptive string
#define INV_CDROM			11	// szInventory: Descriptive string
#define INV_SOUNDCARD		12	// szInventory: Descriptive string
#define INV_REMOVEABLEMEDIA	13	// szInventory: Descriptive string
#define INV_COPRECESSOR		14	// szInventory[0] = 1 for available, 0 for nonavailable
#define INV_SCSIADAPTER     15  
#define INV_DISPLAY_ADAPTER 16  // Display Adapter with Driver
#define INV_DISPLAY_WITH_RESOLUTION 17 // Display Adapter and Color resolution        //
#ifdef __cplusplus
}   /* ... extern "C" */
#endif

#endif
