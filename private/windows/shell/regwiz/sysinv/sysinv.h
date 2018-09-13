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
#define INV_SCSIADAPTER     15  //  
#define INV_DISPLAY_ADAPTER 16  // Display Adapter with Driver
#define INV_DISPLAY_WITH_RESOLUTION 17 // Display Adapter and Color resolution
#ifdef __cplusplus
}   /* ... extern "C" */
#endif

#endif


#include <tchar.h>

void GetOEMString(LPTSTR);
void GetProcessorTypeString(LPTSTR);
void GetTotalMemoryString(LPTSTR);
void GetTotalHardDiskSpaceString(LPTSTR);
void GetDisplayResolutionString(LPTSTR);
void GetDisplayColorDepthString(LPTSTR);
void GetWindowsVersionString(LPTSTR);
void GetNetworkCardString(LPTSTR);
void GetModemString(LPTSTR);
void GetPointingDeviceString(LPTSTR);
void GetCDRomString(LPTSTR);
void GetSoundCardString(LPTSTR);
void GetRemoveableMediaString(LPTSTR);
void GetScsiAdapterString(LPTSTR szInventory);
void GetDisplayAdapter(LPTSTR szDisplayAdapter);
void GetDisplayAdapterWithResolution(LPTSTR);


BOOL IsCoProcessorAvailable(void);
LONG GetTotalHardDiskSpace(void);
void GetDisplayCharacteristics(PINT, PINT, PINT);
void GetWindowsVersion(LONG*, LONG*, LONG*,LONG*);

