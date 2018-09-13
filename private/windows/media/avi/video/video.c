/****************************************************************************
    video.c

    Contains video APIs

    Copyright (c) Microsoft Corporation 1992. All rights reserved

****************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include <win32.h>
#include <winerror.h>

#include "msviddrv.h"
#include "ivideo32.h" // includes msvideo.h
#include "msvideoi.h"

#ifdef _WIN32
#include "profile.h"
#include <mmddk.h>
#include <stdlib.h>
#include <winver.h>
#else
#include <ver.h>
#endif


#include "debug.h"

#ifndef DEVNODE
typedef	DWORD	   DEVNODE;	// Devnode.
#endif

#ifndef LPHKEY
typedef HKEY FAR * LPHKEY;
#endif

#if defined _WIN32 && !defined UNICODE
HDRVR WINAPI OpenDriverA( LPCSTR szDriverName, LPCSTR szSectionName, LPARAM lParam2);
#endif


/*****************************************************************************
 * Variables
 *
 ****************************************************************************/

#ifdef WIN16 // All of this is currently only used in 16 bit land

SZCODE  szVideo[]           = TEXT("msvideo");

#ifndef UNICODE
// Registry settings of interest to capture drivers
SZCODE  szRegKey[]          = TEXT("SYSTEM\\CurrentControlSet\\Control\\MediaResources\\msvideo");
SZCODE  szRegActive[]       = TEXT("Active");
SZCODE  szRegDisabled[]     = TEXT("Disabled");
SZCODE  szRegDescription[]  = TEXT("Description");
SZCODE  szRegDevNode[]      = TEXT("DevNode");
SZCODE  szRegDriver[]       = TEXT("Driver");
SZCODE  szRegSoftwareKey[]  = TEXT("SOFTWAREKEY");
#endif

#ifndef _WIN32
SZCODE  szDrivers[]     = "Drivers";
#else
STATICDT SZCODE  szDrivers[]     = DRIVERS_SECTION;
#endif

STATICDT SZCODE  szSystemIni[]   = TEXT("system.ini");

static SZCODE	szNull[] = TEXT("");

UINT    wTotalVideoDevs;                  // total video devices
LPCAPDRIVERINFO aCapDriverList[MAXVIDEODRIVERS]; // Array of all capture drivers

/*****************************************************************************
 * @doc INTERNAL  VIDEO validation code for VIDEOHDRs
 ****************************************************************************/

#define IsVideoHeaderPrepared(hVideo, lpwh)      ((lpwh)->dwFlags &  VHDR_PREPARED)
#define MarkVideoHeaderPrepared(hVideo, lpwh)    ((lpwh)->dwFlags |= VHDR_PREPARED)
#define MarkVideoHeaderUnprepared(hVideo, lpwh)  ((lpwh)->dwFlags &=~VHDR_PREPARED)


/*****************************************************************************
 * @doc INTERNAL  VIDEO
 *
 * @api BOOL | videoRegOpenMSVideoKey | This function returns a key
 *      for the msvideo node in the registry.
 *      If the key does not exist it will be created,
 *      and the default entries made.
 *
 * @rdesc Returns Key on success, else NULL.
 ****************************************************************************/
HKEY videoRegOpenMSVideoKey (void)
{
    HKEY hKey = NULL;

    // Get the key if it already exists
    if (RegOpenKey (
                HKEY_LOCAL_MACHINE,
                szRegKey,
                &hKey) != ERROR_SUCCESS) {

        // Otherwise make a new key
        if (RegCreateKey (
                        HKEY_LOCAL_MACHINE,
                        szRegKey,
                        &hKey) == ERROR_SUCCESS) {
            // Add the default entries to the msvideo node?

        }
    }
    return hKey;
}

/*****************************************************************************
 * @doc INTERNAL  VIDEO
 *
 * @api BOOL | videoRegGetDriverByIndex | This function returns information
 *      about a capture driver by index from the registry.
 *
 * @parm DWORD | dwDeviceID | Identifies the video device to open.
 *      The value of <p dwDeviceID> varies from zero to one less
 *      than the number of video capture devices installed in the system.
 *
 * @parm LPDEVNODE | lpDevnode | Specifies a far pointer to a buffer
 *   used to return an <t DEVNODE> handle.  For non Plug-and-Play devices,
 *   this return value will be NULL.
 *
 * @parm LPBOOL | lpEnabled | Specifies a far pointer to a buffer
 *   used to return a <t BOOL> flag.  If this value is TRUE, the driver is
 *   enabled, if FALSE, the corresponding device is disabled.
 *
 * @rdesc Returns TRUE if successful, or FALSE if a driver was not found
 *  with the <p dwDeviceID> index.
 *
 * @comm Because the indexes of the MSVIDEO devices in the SYSTEM.INI
 *       file can be non-contiguous, applications should not assume
 *       the indexes range between zero and the number of devices minus
 *       one.
 *
 ****************************************************************************/


BOOL videoRegGetKeyByIndex (
        HKEY            hKeyMSVideoRoot,
        DWORD           dwDeviceID,
        LPCAPDRIVERINFO lpCapDriverInfo,
        LPHKEY          phKeyChild)
{
    BOOL fOK = FALSE;
    HKEY hKeyEnum;
    int i;

    *phKeyChild = (HKEY) 0;

    for (i=0; i < MAXVIDEODRIVERS; i++) {

        if (RegEnumKey (
                hKeyMSVideoRoot,
                i,
                lpCapDriverInfo-> szKeyEnumName,
                sizeof(lpCapDriverInfo->szKeyEnumName)/sizeof(TCHAR)) != ERROR_SUCCESS)
            break;

        // Found a subkey, does it match the requested index?
        if (i == (int) dwDeviceID) {

            if (RegOpenKey (
                        hKeyMSVideoRoot,
                        lpCapDriverInfo-> szKeyEnumName,
                        &hKeyEnum) == ERROR_SUCCESS) {

                *phKeyChild = hKeyEnum;  // Found it!!!
                fOK = TRUE;

            }
            break;
        }
    } // endof all driver indices
    return fOK;
}

// Fetches driver info listed in the registry.
// Returns: TRUE if the index was valid, FALSE if no driver at that index
// Note: Registry entry ordering is random.

BOOL videoRegGetDriverByIndex (
        DWORD           dwDeviceID,
        LPCAPDRIVERINFO lpCapDriverInfo)
{
    DWORD dwType;
    DWORD dwSize;
    BOOL fOK;
    HKEY hKeyChild;
    HKEY hKeyMSVideoRoot;

    // Always start clean since the entry may be recycled
    _fmemset (lpCapDriverInfo, 0, sizeof (CAPDRIVERINFO));

    if (!(hKeyMSVideoRoot = videoRegOpenMSVideoKey()))
        return FALSE;

    if (fOK = videoRegGetKeyByIndex (
                hKeyMSVideoRoot,
                dwDeviceID,
                lpCapDriverInfo,
                &hKeyChild)) {

        // Fetch the values:
        //      Active
        //      Disabled
        //      Description
        //      DEVNODE
        //      Driver
        //      SOFTWAREKEY

        dwSize = sizeof(BOOL);          // Active
        RegQueryValueEx(
                   hKeyChild,
                   szRegActive,
                   NULL,
                   &dwType,
                   (LPBYTE) &lpCapDriverInfo->fActive,
                   &dwSize);

        dwSize = sizeof(BOOL);          // Enabled
        RegQueryValueEx(
                   hKeyChild,
                   szRegDisabled,
                   NULL,
                   &dwType,
                   (LPBYTE) &lpCapDriverInfo->fDisabled,
                   &dwSize);
        // Convert this silly thing to a bool
        lpCapDriverInfo->fDisabled = (lpCapDriverInfo->fDisabled == '1');

        // DriverDescription
        dwSize = sizeof (lpCapDriverInfo->szDriverDescription) / sizeof (TCHAR);
        RegQueryValueEx(
                   hKeyChild,
                   szRegDescription,
                   NULL,
                   &dwType,
                   (LPBYTE) lpCapDriverInfo->szDriverDescription,
                   &dwSize);

        // DEVNODE
        dwSize = sizeof(DEVNODE);
        RegQueryValueEx(
                   hKeyChild,
                   szRegDevNode,
                   NULL,
                   &dwType,
                   (LPBYTE) &lpCapDriverInfo->dnDevNode,
                   &dwSize);

        // DriverName
        dwSize = sizeof (lpCapDriverInfo->szDriverName) / sizeof (TCHAR);
        RegQueryValueEx(
                   hKeyChild,
                   szRegDriver,
                   NULL,
                   &dwType,
                   (LPBYTE) lpCapDriverInfo->szDriverName,
                   &dwSize);

        // SoftwareKey
        dwSize = sizeof (lpCapDriverInfo->szSoftwareKey) / sizeof (TCHAR);
        RegQueryValueEx(
                   hKeyChild,
                   szRegSoftwareKey,
                   NULL,
                   &dwType,
                   (LPBYTE) lpCapDriverInfo->szSoftwareKey,
                   &dwSize);

        RegCloseKey (hKeyChild);

    } // if the subkey could be opened

    RegCloseKey (hKeyMSVideoRoot);

    return fOK;
}

// Fetches driver info listed in system.ini
// Returns: TRUE if the index was valid, FALSE if no driver at that index

BOOL videoIniGetDriverByIndex (
        DWORD           dwDeviceID,
        LPCAPDRIVERINFO lpCapDriverInfo)
{
    TCHAR szKey[sizeof(szVideo)/sizeof(TCHAR) + 2];
    int w = (int) dwDeviceID;
    BOOL fOK = FALSE;

    // Always start clean since the entry may be recycled
    _fmemset (lpCapDriverInfo, 0, sizeof (CAPDRIVERINFO));

    lstrcpy(szKey, szVideo);
    szKey[(sizeof(szVideo)/sizeof(TCHAR)) - 1] = (TCHAR)0;
    if( w > 0 ) {
        szKey[(sizeof(szVideo)/sizeof(TCHAR))] = (TCHAR)0;
        szKey[(sizeof(szVideo)/sizeof(TCHAR))-1] = (TCHAR) TEXT('1' + (w-1) );  // driver ordinal
    }
    if (GetPrivateProfileString(szDrivers, szKey, szNull,
                lpCapDriverInfo->szDriverName,
                sizeof(lpCapDriverInfo->szDriverName)/sizeof(TCHAR),
                szSystemIni)) {

        // Found an entry at the requested index
        // The description and version info will be inserted as
        // requested by the client app.

        lpCapDriverInfo-> fOnlySystemIni = TRUE;

        fOK = TRUE;
    }

    return fOK;
}

DWORD videoFreeDriverList (void)

{
    int i;

    // Free the driver list
    for (i = 0; i < MAXVIDEODRIVERS; i++) {
        if (aCapDriverList[i])
            GlobalFreePtr (aCapDriverList[i]);
        aCapDriverList[i] = NULL;
    }

    return DV_ERR_OK;
}

// This function may be called a number of times to create the
// current driver array.  Since Capscrn assumes it can throw a
// driver into system.ini on the fly and have it immediately accessible,
// this routine is called on videoGetNumDevs() and when AVICapx.dll
// tries to get the driver description and version.
//
// Drivers in the registry will be the first entries in the list.
//
// If a driver is listed in the registry AND in system.ini AND
// the full path to the drivers match, the system.ini entry will NOT
// be in the resulting list.

// The variable wTotalVideoDevs is set as a byproduct of this function.

// Returns DV_ERR_OK on success, even if no drivers are installed.
//
DWORD videoCreateDriverList (void)

{
    int i, j, k;

    wTotalVideoDevs = 0;

    // Delete the existing list
    videoFreeDriverList ();

    // Allocate an array of pointers to all possible capture drivers
    for (i = 0; i < MAXVIDEODRIVERS; i++) {
        aCapDriverList[i] = (LPCAPDRIVERINFO) GlobalAllocPtr (
                GMEM_MOVEABLE |
                GMEM_SHARE |
                GMEM_ZEROINIT,
                sizeof (CAPDRIVERINFO));
        if (aCapDriverList[i] == NULL)
            return DV_ERR_NOMEM;
    }

    // Walk the list of Registry drivers and get each entry
    for (i = 0; i < MAXVIDEODRIVERS; i++) {
        if (videoRegGetDriverByIndex (
                    (DWORD) i, aCapDriverList[wTotalVideoDevs])) {

            wTotalVideoDevs++;

        }
        else
            break;
    }

    if (wTotalVideoDevs == MAXVIDEODRIVERS)
        goto AllDone;

    // Now add the entries listed in system.ini, (msvideo[0-9] = driver.drv)
    // to the driver array, ONLY if the entry doesn't exactly match
    // an existing registry entry.

    for (j = 0; j < MAXVIDEODRIVERS; j++) {
        if (videoIniGetDriverByIndex ((DWORD) j,
                        aCapDriverList[wTotalVideoDevs])) {

            // Found an entry, now see if it is a duplicate of an existing
            // registry entry

            for (k = 0; k < (int) wTotalVideoDevs; k++) {

                if (lstrcmpi (aCapDriverList[k]->szDriverName,
                    aCapDriverList[wTotalVideoDevs]->szDriverName) == 0) {

                    // Found an exact match, so skip it!
                    goto SkipThisEntry;
                }
            }

            if (wTotalVideoDevs >= MAXVIDEODRIVERS - 1)
                break;

            wTotalVideoDevs++;

SkipThisEntry:
            ;
        } // If sytem.ini entry was found
    } // For all system.ini possibilities

AllDone:

    // Decrement wTotalVideoDevs for any entries which are marked as disabled
    // And remove disabled entries from the list
    for (i = 0; i < MAXVIDEODRIVERS; ) {

        if (aCapDriverList[i] && aCapDriverList[i]->fDisabled) {

            GlobalFreePtr (aCapDriverList[i]);

            // Shift down the remaining drivers
            for (j = i; j < MAXVIDEODRIVERS - 1; j++) {
                aCapDriverList[j] = aCapDriverList[j + 1];
            }
            aCapDriverList[MAXVIDEODRIVERS - 1] = NULL;

            wTotalVideoDevs--;
        }
        else
            i++;
    }

    // Free the unused pointers
    for (i = wTotalVideoDevs; i < MAXVIDEODRIVERS; i++) {
        if (aCapDriverList[i])
            GlobalFreePtr (aCapDriverList[i]);
        aCapDriverList[i] = NULL;
    }

    // Put PnP drivers first in the list
    // These are the only entries that have a DevNode
    for (k = i = 0; i < (int) wTotalVideoDevs; i++) {
        if (aCapDriverList[i]-> dnDevNode) {
            LPCAPDRIVERINFO lpCDTemp;

            if (k != i) {
                // Swap the entries
                lpCDTemp = aCapDriverList[k];
                aCapDriverList[k] = aCapDriverList[i];
                aCapDriverList[i] = lpCDTemp;
            }
            k++;   // Index of first non-PnP driver
        }
    }

    return DV_ERR_OK;
}



typedef struct tagVS_VERSION
{
    WORD wTotLen;
    WORD wValLen;
    char szSig[16];
    VS_FIXEDFILEINFO vffInfo;
} VS_VERSION;

typedef struct tagLANGANDCP
{
    WORD wLanguage;
    WORD wCodePage;
} LANGANDCP;

// Extracts the Description string and version info from a driver.
// Drivers listed in the registry will typically already include a
// description string, but not version info.
// Drivers listed in system.ini will fetch both.
//
// Returns DV_ERR_OK on success, even if no version info was available.
//

DWORD WINAPI videoGetDriverDescAndVer (
        LPCAPDRIVERINFO lpCapDriverInfo)
{
    LPSTR   lpVersion;
    WORD    wVersionLen;
    BOOL    bRetCode;
    DWORD   dwVerInfoSize;
    DWORD   dwVerHnd;
    TCHAR   szBuf[MAX_PATH];
    BOOL    fGetDesc;
    BOOL    fGetVersion;

    // Only get the description if not already supplied by the registry
    fGetDesc = (lpCapDriverInfo->szDriverDescription[0] == '\0');
    fGetVersion = (lpCapDriverInfo->szDriverVersion[0] == '\0');

    if (fGetDesc)
        lpCapDriverInfo->szDriverDescription[0] = '\0';
    if (fGetVersion)
        lpCapDriverInfo->szDriverVersion [0] = '\0';

    // Copy in the driver name initially, just in case the driver
    // has omitted a description field.
    if (fGetDesc)
        lstrcpyn (lpCapDriverInfo->szDriverDescription,
                lpCapDriverInfo->szDriverName,
                sizeof (lpCapDriverInfo->szDriverDescription) / sizeof (TCHAR));

    // You must find the size first before getting any file info
    dwVerInfoSize =
        GetFileVersionInfoSize(lpCapDriverInfo->szDriverName, &dwVerHnd);

    if (dwVerInfoSize) {
        LPSTR   lpstrVffInfo;             // Pointer to block to hold info

        // Get a block big enough to hold version info
        if (lpstrVffInfo  = GlobalAllocPtr(GMEM_MOVEABLE, dwVerInfoSize)) {

           // Get the File Version first
           if(GetFileVersionInfo(lpCapDriverInfo->szDriverName, 0L,
                           dwVerInfoSize, lpstrVffInfo)) {
           VS_VERSION FAR *pVerInfo = (VS_VERSION FAR *) lpstrVffInfo;

           // fill in the file version
           wsprintf(szBuf,
                   "Version:  %d.%d.%d.%d",
                   HIWORD(pVerInfo->vffInfo.dwFileVersionMS),
                   LOWORD(pVerInfo->vffInfo.dwFileVersionMS),
                   HIWORD(pVerInfo->vffInfo.dwFileVersionLS),
                   LOWORD(pVerInfo->vffInfo.dwFileVersionLS));
           if (fGetVersion)
                   lstrcpyn (lpCapDriverInfo->szDriverVersion, szBuf,
                           sizeof (lpCapDriverInfo->szDriverVersion) / sizeof(TCHAR));
           }

           // Now try to get the FileDescription
           // First try this for the "Translation" entry, and then
           // try the American english translation.
           // Keep track of the string length for easy updating.
           // 040904E4 represents the language ID and the four
           // least significant digits represent the codepage for
           // which the data is formatted.  The language ID is
           // composed of two parts: the low ten bits represent
           // the major language and the high six bits represent
           // the sub language.

           lstrcpy(szBuf, "\\StringFileInfo\\040904E4\\FileDescription");

           wVersionLen   = 0;
           lpVersion     = NULL;

           // Look for the corresponding string.
           bRetCode      =  VerQueryValue((LPVOID)lpstrVffInfo,
                           (LPSTR)szBuf,
                           (void FAR* FAR*)&lpVersion,
                           (UINT FAR *) &wVersionLen);

           if (fGetDesc && bRetCode && wVersionLen && lpVersion)
                lstrcpyn (lpCapDriverInfo->szDriverDescription, lpVersion,
                   sizeof (lpCapDriverInfo->szDriverDescription) / sizeof(TCHAR));

           // Let go of the memory
           GlobalFreePtr(lpstrVffInfo);
        }
        else
            return DV_ERR_NOMEM;
    }
    return DV_ERR_OK;
}


// Called by AVICap and AVICap32 to get driver strings
// Returns:
//      DV_ERR_OK on success
//      DV_ERR_BADINSTALL if the driver at the specified index is not found

DWORD WINAPI videoCapDriverDescAndVer (
        DWORD dwDeviceID,
        LPTSTR lpszDesc, UINT cbDesc,
        LPTSTR lpszVer, UINT cbVer)
{
    DWORD dwR = DV_ERR_OK;

    if (lpszDesc && IsBadWritePtr (lpszDesc, cbDesc))
        return DV_ERR_NONSPECIFIC;
    else if (lpszDesc)
        *lpszDesc = '\0';
    if (lpszVer && IsBadWritePtr (lpszVer, cbVer))
        return DV_ERR_NONSPECIFIC;
    else if (lpszVer)
        *lpszVer = '\0';

    if (!wTotalVideoDevs)
        dwR = videoCreateDriverList ();

    if (dwR != DV_ERR_OK)
        return dwR;

    if ((WORD) dwDeviceID >= wTotalVideoDevs)
        return DV_ERR_BADDEVICEID;

    dwR = videoGetDriverDescAndVer (aCapDriverList[(int)dwDeviceID]);

    if (dwR == DV_ERR_OK) {
       if (lpszDesc)
           lstrcpyn (lpszDesc, aCapDriverList[(int)dwDeviceID]->szDriverDescription, cbDesc);

       if (lpszVer)
           lstrcpyn (lpszVer, aCapDriverList[(int)dwDeviceID]->szDriverVersion, cbVer);

    }
    return (dwR);
}


/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @func DWORD | videoMessage | This function sends messages to a
 *   video device channel.
 *
 * @parm HVIDEO | hVideo | Specifies the handle to the video device channel.
 *
 * @parm UINT | wMsg | Specifies the message to send.
 *
 * @parm DWORD | dwP1 | Specifies the first parameter for the message.
 *
 * @parm DWORD | dwP2 | Specifies the second parameter for the message.
 *
 * @rdesc Returns the message specific value returned from the driver.
 *
 * @comm This function is used for configuration messages such as
 *      <m DVM_SRC_RECT> and <m DVM_DST_RECT>, and
 *      device specific messages.
 *
 * @xref <f videoConfigure>
 *
 ****************************************************************************/
DWORD WINAPI videoMessage(HVIDEO hVideo, UINT msg, DWORD dwP1, DWORD dwP2)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    return SendDriverMessage ((HDRVR)hVideo, msg, dwP1, dwP2);
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoGetNumDevs | This function returns the number of MSVIDEO
 *   devices installed.
 *
 * @rdesc Returns the number of MSVIDEO devices listed in the
 *  [drivers] (or [drivers32] for NT) section of the SYSTEM.INI file
 *  plus drivers listed in the registry.
 *
 * @comm Because the indexes of the MSVIDEO devices in the SYSTEM.INI
 *       file can be non-contiguous, applications should not assume
 *       the indexes range between zero and the number of devices minus
 *       one.
 *
 * @xref <f videoOpen>
 ****************************************************************************/
DWORD WINAPI videoGetNumDevs(void)
{
    if (wTotalVideoDevs)
        return (DWORD)wTotalVideoDevs;

    videoCreateDriverList ();

    return (DWORD)wTotalVideoDevs;
}

/*****************************************************************************
 * @doc EXTERNAL VIDEO
 *
 * @func DWORD | videoGetErrorText | This function retrieves a
 *   description of the error identified by the error number.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *	This might be NULL if the error is not device specific.
 *
 * @parm UINT | wError | Specifies the error number.
 *
 * @parm LPSTR | lpText | Specifies a far pointer to a buffer used to
 *       return the zero-terminated string corresponding to the error number.
 *
 * @parm UINT | wSize | Specifies the length, in bytes, of the buffer
 *       referenced by <p lpText>.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following error is defined:
 *   @flag DV_ERR_BADERRNUM | Specified error number is out of range.
 *   @flag DV_ERR_SIZEFIELD | The return buffer is not large enough
 *         to handle the error text.
 *
 * @comm If the error description is longer than the buffer,
 *   the description is truncated. The returned error string is always
 *   zero-terminated. If <p wSize> is zero, nothing is copied and zero
 *   is returned.
 ****************************************************************************/

#if defined _WIN32
//
// for WIN32 this is the ansi entry point
//
DWORD WINAPI videoGetErrorTextA (HVIDEO hVideo,
    UINT wError, LPSTR lpText, UINT wSize)
{
    VIDEO_GETERRORTEXT_PARMS vet;

    if (IsBadWritePtr (lpText, wSize))
        return DV_ERR_PARAM1;

    lpText[0] = 0;
    if (((wError >= DV_ERR_BASE) && (wError <= DV_ERR_LASTERROR))) {
        if (wSize > 1) {
            if (!LoadStringA(ghInst, wError, lpText, wSize))
                return DV_ERR_BADERRNUM;
            else
                return DV_ERR_OK;
        }
        else
            return DV_ERR_SIZEFIELD;
    }
    else if (wError >= DV_ERR_USER_MSG && hVideo) {
        DWORD dwResult;
        LPWSTR lpwstr = LocalAlloc(LPTR, wSize*sizeof(WCHAR));
        if (NULL == lpwstr) {
            return(DV_ERR_NOMEM);
        }
        vet.dwError = (DWORD) wError;
        vet.lpText = lpwstr;
        vet.dwLength = (DWORD) wSize;
        dwResult = videoMessage (hVideo, DVM_GETERRORTEXT, (DWORD) (LPVOID) &vet,
                        (DWORD) NULL);
        if (DV_ERR_OK == dwResult) {
            wcstombs(lpText, lpwstr, wSize);
        }
        LocalFree(lpwstr);
        return(dwResult);
    }
    else
        return DV_ERR_BADERRNUM;
}
#endif

//
// The unicode/Win16 equivalent of the above
//
#ifdef _WIN32
DWORD WINAPI videoGetErrorTextW (HVIDEO hVideo, UINT wError,
                        LPWSTR lpText, UINT wSize)
#else
DWORD WINAPI videoGetErrorText (HVIDEO hVideo, UINT wError,
                        LPSTR lpText, UINT wSize)
#endif
{
    VIDEO_GETERRORTEXT_PARMS vet;
    lpText[0] = 0;

    if (((wError > DV_ERR_BASE) && (wError <= DV_ERR_LASTERROR))) {
        if (wSize > 1) {
           #ifdef _WIN32
            LPSTR lpsz = LocalAlloc (LPTR, wSize);
            UINT  cch;

            if (!lpsz)
                return DV_ERR_NOMEM;
            cch = LoadString (ghInst, wError, lpsz, wSize);

            lpsz[cch] = 0;
            mbstowcs (lpText, lpsz, cch+1);
            LocalFree (lpsz);

            if (!cch)
           #else
            if (!LoadString(ghInst, wError, lpText, wSize))
           #endif
                return DV_ERR_BADERRNUM;
            else
                return DV_ERR_OK;
        }
        else
            return DV_ERR_SIZEFIELD;
    }
    else if (wError >= DV_ERR_USER_MSG && hVideo) {
        vet.dwError = (DWORD) wError;
        vet.lpText = lpText;
        vet.dwLength = wSize;
        return videoMessage (hVideo, DVM_GETERRORTEXT, (DWORD) (LPVOID) &vet,
                             (DWORD) NULL);
    }
    else
        return DV_ERR_BADERRNUM;
}


/*****************************************************************************
 * @doc EXTERNAL VIDEO
 *
 * @func DWORD | videoGetChannelCaps | This function retrieves a
 *   description of the capabilities of a channel.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *
 * @parm LPCHANNEL_CAPS | lpChannelCaps | Specifies a far pointer to a
 *      <t CHANNEL_CAPS> structure.
 *
 * @parm DWORD | dwSize | Specifies the size, in bytes, of the
 *       <t CHANNEL_CAPS> structure.
 *
 * @rdesc Returns zero if the function is successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag DV_ERR_UNSUPPORTED | Function is not supported.
 *
 * @comm The <t CHANNEL_CAPS> structure returns the capability
 *   information. For example, capability information might
 *   include whether or not the channel can crop and scale images,
 *   or show overlay.
 ****************************************************************************/
DWORD WINAPI videoGetChannelCaps(HVIDEO hVideo, LPCHANNEL_CAPS lpChannelCaps,
			DWORD dwSize)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (IsBadWritePtr (lpChannelCaps, sizeof (CHANNEL_CAPS)))
        return DV_ERR_PARAM1;

    // _fmemset (lpChannelCaps, 0, sizeof (CHANNEL_CAPS));

    lpChannelCaps->dwFlags = 0;
    lpChannelCaps->dwSrcRectXMod = 0;
    lpChannelCaps->dwSrcRectYMod = 0;
    lpChannelCaps->dwSrcRectWidthMod = 0;
    lpChannelCaps->dwSrcRectHeightMod = 0;
    lpChannelCaps->dwDstRectXMod = 0;
    lpChannelCaps->dwDstRectYMod = 0;
    lpChannelCaps->dwDstRectWidthMod = 0;
    lpChannelCaps->dwDstRectHeightMod = 0;

    return videoMessage(hVideo, DVM_GET_CHANNEL_CAPS, (DWORD) lpChannelCaps,
	    (DWORD) dwSize);
}


/*****************************************************************************
 * @doc EXTERNAL VIDEO
 *
 * @func DWORD | videoUpdate | This function directs a channel to
 *   repaint the display.  It applies only to VIDEO_EXTERNALOUT channels.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *
 * @parm HWND | hWnd | Specifies the handle of the window to be used
 *      by the channel for image display.
 *
 * @parm HDC | hDC | Specifies a handle to a device context.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_UNSUPPORTED | Specified message is unsupported.
 *   @flag DV_ERR_INVALHANDLE | Indicates the device handle specified is invalid.
 *
 * @comm This message is normally sent
 *   whenever the client window receives a <m WM_MOVE>, <m WM_SIZE>,
 *   or <m WM_PAINT> message.
 ****************************************************************************/
DWORD WINAPI videoUpdate (HVIDEO hVideo, HWND hWnd, HDC hDC)
{
    if ((!hVideo) || (!hWnd) || (!hDC) )
        return DV_ERR_INVALHANDLE;

    return videoMessage(hVideo, DVM_UPDATE, (DWORD) hWnd, (DWORD) hDC);
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoOpen | This function opens a channel on the
 *  specified video device.
 *
 * @parm LPHVIDEO | lphvideo | Specifies a far pointer to a buffer
 *   used to return an <t HVIDEO> handle. The video capture driver
 *   uses this location to return
 *   a handle that uniquely identifies the opened video device channel.
 *   Use the returned handle to identify the device channel when
 *   calling other video functions.
 *
 * @parm DWORD | dwDeviceID | Identifies the video device to open.
 *      The value of <p dwDeviceID> varies from zero to one less
 *      than the number of video capture devices installed in the system.
 *
 * @parm DWORD | dwFlags | Specifies flags for opening the device.
 *      The following flags are defined:
 *
 *   @flag VIDEO_EXTERNALIN| Specifies the channel is opened
 *	     for external input. Typically, external input channels
 *      capture images into a frame buffer.
 *
 *   @flag VIDEO_EXTERNALOUT| Specifies the channel is opened
 *      for external output. Typically, external output channels
 *      display images stored in a frame buffer on an auxilary monitor
 *      or overlay.
 *
 *   @flag VIDEO_IN| Specifies the channel is opened
 *      for video input. Video input channels transfer images
 *      from a frame buffer to system memory buffers.
 *
 *   @flag VIDEO_OUT| Specifies the channel is opened
 *      for video output. Video output channels transfer images
 *      from system memory buffers to a frame buffer.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_BADDEVICEID | Indicates the specified device ID is out of range.
 *   @flag DV_ERR_ALLOCATED | Indicates the specified resource is already allocated.
 *   @flag DV_ERR_NOMEM | Indicates the device is unable to allocate or lock memory.
 *
 * @comm
 *   At a minimum, all capture drivers support a VIDEO_EXTERNALIN
 *   and a VIDEO_IN channel.
 *   Use <f videoGetNumDevs> to determine the number of video
 *   devices present in the system.
 *
 * @xref <f videoClose>
 ****************************************************************************/
DWORD WINAPI videoOpen (LPHVIDEO lphVideo, DWORD dwDeviceID, DWORD dwFlags)
{
    UINT w;
    VIDEO_OPEN_PARMS vop;       // Same as IC_OPEN struct!!!
    DWORD dwVersion = VIDEOAPIVERSION;

    if (IsBadWritePtr ((LPVOID) lphVideo, sizeof (HVIDEO)) )
        return DV_ERR_PARAM1;

    vop.dwSize = sizeof (VIDEO_OPEN_PARMS);
    vop.fccType = OPEN_TYPE_VCAP;       // "vcap"
    vop.fccComp = 0L;
    vop.dwVersion = VIDEOAPIVERSION;
    vop.dwFlags = dwFlags;      // In, Out, External In, External Out
    vop.dwError = DV_ERR_OK;

    w = (UINT) dwDeviceID;
    *lphVideo = NULL;

    if (!wTotalVideoDevs)   // trying to open without finding how many devs.
        videoGetNumDevs();

    if (!wTotalVideoDevs)              // No drivers installed
        return DV_ERR_BADINSTALL;

    if (w >= wTotalVideoDevs)
        return DV_ERR_BADDEVICEID;

    // New for Chicago, drivers are passed a Devnode if PnP at Open time
    vop.dnDevNode = aCapDriverList[w]->dnDevNode;

#ifdef UNICODE
    *lphVideo = (HVIDEO) OpenDriver(aCapDriverList[w]->szDriverName,
                        NULL, (LPARAM) (LPVOID) &vop);
#else

#if defined _WIN32
    *lphVideo = (HVIDEO) OpenDriverA(aCapDriverList[w]->szDriverName,
                        NULL, (LPARAM) (LPVOID) &vop);
#else
    *lphVideo = (HVIDEO) OpenDriver(aCapDriverList[w]->szDriverName,
                        NULL, (LPARAM) (LPVOID) &vop);
#endif
#endif


    if( ! *lphVideo ) {
        if (vop.dwError)    // if driver returned an error code...
            return vop.dwError;
        else {
#ifdef _WIN32
            if (GetFileAttributes(aCapDriverList[w]->szDriverName) == (DWORD) -1)
#else
            OFSTRUCT of;

            if (OpenFile (aCapDriverList[w]->szDriverName, &of, OF_EXIST) == HFILE_ERROR)
#endif
                return (DV_ERR_BADINSTALL);
            else
                return (DV_ERR_NOTDETECTED);
	}
    }

    return DV_ERR_OK;

}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoClose | This function closes the specified video
 *   device channel.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *  If this function is successful, the handle is invalid
 *   after this call.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag DV_ERR_NONSPECIFIC | The driver failed to close the channel.
 *
 * @comm If buffers have been sent with <f videoStreamAddBuffer> and
 *   they haven't been returned to the application,
 *   the close operation fails. You can use <f videoStreamReset> to mark all
 *   pending buffers as done.
 *
 * @xref <f videoOpen> <f videoStreamInit> <f videoStreamFini> <f videoStreamReset>
 ****************************************************************************/
DWORD WINAPI videoClose (HVIDEO hVideo)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (CloseDriver((HDRVR)hVideo, 0L, 0L ))
       return DV_ERR_OK;

    return DV_ERR_NONSPECIFIC;
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoConfigure | This function sets or retrieves
 *      the options for a configurable driver.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *
 * @parm UINT | msg  | Specifies the option to set or retrieve. The
 *       following options are defined:
 *
 *   @flag DVM_PALETTE | Indicates a palette is being sent to the driver
 *         or retrieved from the driver.
 *
 *   @flag DVM_PALETTERGB555 | Indicates an RGB555 palette is being
 *         sent to the driver.
 *
 *   @flag DVM_FORMAT | Indicates format information is being sent to
 *         the driver or retrieved from the driver.
 *
 * @parm DWORD | dwFlags | Specifies flags for configuring or
 *   interrogating the device driver. The following flags are defined:
 *
 *   @flag VIDEO_CONFIGURE_SET | Indicates values are being sent to the driver.
 *
 *   @flag VIDEO_CONFIGURE_GET | Indicates values are being obtained from the driver.
 *
 *   @flag VIDEO_CONFIGURE_QUERY | Determines if the
 *      driver supports the option specified by <p msg>. This flag
 *      should be combined with either the VIDEO_CONFIGURE_SET or
 *      VIDEO_CONFIGURE_GET flag. If this flag is
 *      set, the <p lpData1>, <p dwSize1>, <p lpData2>, and <p dwSize2>
 *      parameters are ignored.
 *
 *   @flag VIDEO_CONFIGURE_QUERYSIZE | Returns the size, in bytes,
 *      of the configuration option in <p lpdwReturn>. This flag is only valid if
 *      the VIDEO_CONFIGURE_GET flag is also set.
 *
 *   @flag VIDEO_CONFIGURE_CURRENT | Requests the current value.
 *      This flag is valid only if  the VIDEO_CONFIGURE_GET flag is also set.
 *   @flag VIDEO_CONFIGURE_NOMINAL | Requests the nominal value.
 *      This flag is valid only if  the VIDEO_CONFIGURE_GET flag is also set.
 *   @flag VIDEO_CONFIGURE_MIN | Requests the minimum value.
 *      This flag is valid only if  the VIDEO_CONFIGURE_GET flag is also set.
 *   @flag VIDEO_CONFIGURE_MAX | Get the maximum value.
 *      This flag is valid only if  the VIDEO_CONFIGURE_GET flag is also set.
 *	
 * @parm LPDWORD | lpdwReturn  | Points to a DWORD used for returning information
 *      from the driver.  If
 *      the VIDEO_CONFIGURE_QUERYSIZE flag is set, <p lpdwReturn> is
 *      filled with the size of the configuration option.
 *
 * @parm LPVOID | lpData1  |Specifies a pointer to message specific data.
 *
 * @parm DWORD | dwSize1  | Specifies the size, in bytes, of the <p lpData1>
 *       buffer.
 *
 * @parm LPVOID | lpData2  | Specifies a pointer to message specific data.
 *
 * @parm DWORD | dwSize2  | Specifies the size, in bytes, of the <p lpData2>
 *       buffer.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag DV_ERR_NOTSUPPORTED | Function is not supported.
 *
 * @xref <f videoOpen> <f videoMessage>
 *
 ****************************************************************************/
DWORD WINAPI videoConfigure (HVIDEO hVideo, UINT msg, DWORD dwFlags,
		LPDWORD lpdwReturn, LPVOID lpData1, DWORD dwSize1,
                LPVOID lpData2, DWORD dwSize2)
{
    VIDEOCONFIGPARMS    vcp;

    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (lpData1)
        if (IsBadHugeReadPtr (lpData1, dwSize1))
            return DV_ERR_CONFIG1;

    if (lpData2)
        if (IsBadHugeReadPtr (lpData2, dwSize2))
            return DV_ERR_CONFIG2;

    if (dwFlags & VIDEO_CONFIGURE_QUERYSIZE) {
        if (!lpdwReturn)
            return DV_ERR_NONSPECIFIC;
        if (IsBadWritePtr (lpdwReturn, sizeof (DWORD)) )
            return DV_ERR_NONSPECIFIC;
    }

    vcp.lpdwReturn = lpdwReturn;
    vcp.lpData1 = lpData1;
    vcp.dwSize1 = dwSize1;
    vcp.lpData2 = lpData2;
    vcp.dwSize2 = dwSize2;

    return videoMessage(hVideo, msg, dwFlags,
	    (DWORD)(LPVIDEOCONFIGPARMS)&vcp );
}



/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoConfigureStorage | This function saves or loads
 *	     all configurable options for a channel.  Options
 *      can be saved and recalled for each application or each application
 *      instance.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *
 * @parm LPSTR | lpstrIdent  | Identifies the application or instance.
 *      Use an arbitrary string which uniquely identifies your application
 *      or instance.
 *
 * @parm DWORD | dwFlags | Specifies any flags for the function. The following
 *   flags are defined:
 *   @flag VIDEO_CONFIGURE_GET | Requests that the values be loaded.
 *   @flag VIDEO_CONFIGURE_SET | Requests that the values be saved.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag DV_ERR_NOTSUPPORTED | Function is not supported.
 *
 * @comm The method used by a driver to save configuration options is
 *      device dependent.
 *
 * @xref <f videoOpen>
 ****************************************************************************/

#if defined _WIN32
//
// for win32 this is the ansi entry point
//
DWORD WINAPI videoConfigureStorageA (
    HVIDEO hVideo,
    LPSTR lpstrIdent,
    DWORD dwFlags)
{
    DWORD ret;
    LPWSTR lpwstr;

    if (!hVideo)
        return DV_ERR_INVALHANDLE;


    // Convert the input string to Unicode
    // Call the driver, free the Unicode string and return the result
    ret = strlen(lpstrIdent);
    lpwstr = LocalAlloc(LPTR, ret*sizeof(WCHAR));
    if (!lpwstr) {
        return(DV_ERR_NOMEM);
    }

    mbstowcs(lpwstr, lpstrIdent, ret);

    ret = videoMessage(hVideo, DVM_CONFIGURESTORAGE,
	    (DWORD)lpwstr, dwFlags);

    LocalFree(lpwstr);
    return(ret);
}

#endif // _WIN32

// this code is correct for WIN32 UNICODE and for win16
// messages we send are expected to contain unicode strings.
// so send them off...
//
#ifdef _WIN32
DWORD WINAPI videoConfigureStorageW (HVIDEO hVideo,
   LPWSTR lpstrIdent, DWORD  dwFlags)
#else
DWORD WINAPI videoConfigureStorage (HVIDEO hVideo,
   LPSTR lpstrIdent, DWORD  dwFlags)
#endif
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    return videoMessage(hVideo, DVM_CONFIGURESTORAGE,
                        (DWORD)lpstrIdent, dwFlags);
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoDialog | This function displays a channel-specific
 *     dialog box used to set configuration parameters.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *
 * @parm HWND | hWndParent | Specifies the parent window handle.
 *
 * @parm DWORD | dwFlags | Specifies flags for the dialog box. The
 *   following flag is defined:
 *   @flag VIDEO_DLG_QUERY | If this flag is set, the driver immediately
 *	     returns zero if it supplies a dialog box for the channel,
 *           or DV_ERR_NOTSUPPORTED if it does not.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag DV_ERR_NOTSUPPORTED | Function is not supported.
 *
 * @comm Typically, each dialog box displayed by this
 *      function lets the user select options appropriate for the channel.
 *      For example, a VIDEO_IN channel dialog box lets the user select
 *      the image dimensions and bit depth.
 *
 * @xref <f videoOpen> <f videoConfigureStorage>
 ****************************************************************************/
DWORD WINAPI videoDialog (HVIDEO hVideo, HWND hWndParent, DWORD dwFlags)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if ((!hWndParent) || (!IsWindow (hWndParent)) )
        return DV_ERR_INVALHANDLE;

    return videoMessage(hVideo, DVM_DIALOG, (DWORD)hWndParent, dwFlags);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/*****************************************************************************
 * @doc INTERNAL  VIDEO
 *
 * @api DWORD | videoPrepareHeader | This function prepares the
 *	header and data
 *	by performing a <f GlobalPageLock>.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it
 *   specifies an error number.
 ****************************************************************************/
DWORD WINAPI videoPrepareHeader(LPVIDEOHDR lpVideoHdr, DWORD dwSize)
{
    if (!HugePageLock(lpVideoHdr, (DWORD)sizeof(VIDEOHDR)))
        return DV_ERR_NOMEM;

    if (!HugePageLock(lpVideoHdr->lpData, lpVideoHdr->dwBufferLength)) {
        HugePageUnlock(lpVideoHdr, (DWORD)sizeof(VIDEOHDR));
        return DV_ERR_NOMEM;
    }

    lpVideoHdr->dwFlags |= VHDR_PREPARED;

    return DV_ERR_OK;
}

/*****************************************************************************
 * @doc INTERNAL  VIDEO
 *
 * @api DWORD | videoUnprepareHeader | This function unprepares the header and
 *   data if the driver returns DV_ERR_NOTSUPPORTED.
 *
 * @rdesc Currently always returns DV_ERR_OK.
 ****************************************************************************/
DWORD WINAPI videoUnprepareHeader(LPVIDEOHDR lpVideoHdr, DWORD dwSize)
{

    HugePageUnlock(lpVideoHdr->lpData, lpVideoHdr->dwBufferLength);
    HugePageUnlock(lpVideoHdr, (DWORD)sizeof(VIDEOHDR));

    lpVideoHdr->dwFlags &= ~VHDR_PREPARED;

    return DV_ERR_OK;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoStreamAllocHdrAndBuffer | This function is used to allow
 *      drivers to optionally allocate video buffers.  Normally, the client
 *      application is responsible for allocating buffer memory, but devices
 *      which have on-board memory may optionally allocate headers and buffers
 *      using this function. Generally, this will avoid an additional data copy,
 *      resulting in faster capture rates.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video
 *   device channel.
 *
 * @parm LPVIDEOHDR FAR * | plpvideoHdr | Specifies a pointer to the address of a
 *   <t VIDEOHDR> structure.  The driver saves the buffer address in this
 *   location, or NULL if it cannot allocate a buffer.
 *
 * @parm DWORD | dwSize | Specifies the size of the <t VIDEOHDR> structure
 *      and associated video buffer in bytes.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Indicates the specified device handle is invalid.
 *   @flag DV_ERR_NOMEM | Indicates the device is unable to allocate or lock memory.
 *   @flag DV_ERR_NOTSUPPORTED | Indicates the driver does not have on-board memory.
 *
 * @comm If the driver
 *   allocates buffers via this method, the <f videoStreamPrepareHeader> and
 *   <f videoStreamUnprepareHeader> functions must not be used.
 *
 *   The buffer allocated must be accessible for DMA by the host.
 *
 * @xref <f videoStreamFreeHdrAndBuffer>
 ****************************************************************************/
DWORD WINAPI videoStreamAllocHdrAndBuffer(HVIDEO hVideo,
		LPVIDEOHDR FAR * plpvideoHdr, DWORD dwSize)
{
    DWORD         wRet;

    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (IsBadWritePtr (plpvideoHdr, sizeof (VIDEOHDR *)) )
        return DV_ERR_PARAM1;

    *plpvideoHdr = NULL;        // Init to NULL ptr

    wRet = (DWORD)videoMessage((HVIDEO)hVideo, DVM_STREAM_ALLOCHDRANDBUFFER,
            (DWORD)plpvideoHdr, (DWORD)dwSize);

    if (*plpvideoHdr == NULL ||
                IsBadHugeWritePtr (*plpvideoHdr, dwSize)) {
        DebugErr(DBF_WARNING,"videoStreamAllocHdrAndBuffer: Allocation failed.");
        *plpvideoHdr = NULL;
        return wRet;
    }

    if (IsVideoHeaderPrepared(HVIDEO, *plpvideoHdr))
    {
        DebugErr(DBF_WARNING,"videoStreamAllocHdrAndBuffer: header is already prepared.");
        return DV_ERR_OK;
    }

    (*plpvideoHdr)->dwFlags = 0;

    if (wRet == DV_ERR_OK)
        MarkVideoHeaderPrepared(hVideo, *plpvideoHdr);

    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoStreamFreeHdrAndBuffer | This function is used to free
 *      buffers allocated by the driver using the <f videoStreamAllocHdrAndBuffer>
 *      function.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video
 *   device channel.
 *
 * @parm LPVIDEOHDR | lpvideoHdr | Specifies a pointer to the
 *   <t VIDEOHDR> structure and associated buffer to be freed.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Indicates the specified device handle is invalid.
 *   @flag DV_ERR_NOTSUPPORTED | Indicates the driver does not have on-board memory.
 *
 * @comm If the driver
 *   allocates buffers via this method, the <f videoStreamPrepareHeader> and
 *   <f videoStreamUnprepareHeader> functions must not be used.
 *
 * @xref <f videoStreamAllocHdrAndBuffer>
 ****************************************************************************/

DWORD WINAPI videoStreamFreeHdrAndBuffer(HVIDEO hVideo,
		LPVIDEOHDR lpvideoHdr)
{
    DWORD         wRet;

    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (IsBadWritePtr (lpvideoHdr, sizeof (VIDEOHDR)) )
        return DV_ERR_PARAM1;

    if (lpvideoHdr->dwFlags & VHDR_INQUEUE)
    {
        DebugErr(DBF_WARNING, "videoStreamFreeHdrAndBuffer: buffer still in queue.");
        return DV_ERR_STILLPLAYING;
    }

    if (!IsVideoHeaderPrepared(hVideo, lpvideoHdr))
    {
        DebugErr(DBF_WARNING,"videoStreamFreeHdrAndBuffer: header is not prepared.");
    }

    wRet = (DWORD)videoMessage((HVIDEO)hVideo, DVM_STREAM_FREEHDRANDBUFFER,
            (DWORD)lpvideoHdr, (DWORD)0);

    if (wRet != DV_ERR_OK)
    {
        DebugErr(DBF_WARNING,"videoStreamFreeHdrAndBuffer: Error freeing buffer.");
    }

    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoStreamPrepareHeader | This function prepares a buffer
 *   for video streaming.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video
 *   device channel.
 *
 * @parm LPVIDEOHDR | lpvideoHdr | Specifies a pointer to a
 *   <t VIDEOHDR> structure identifying the buffer to be prepared.
 *
 * @parm DWORD | dwSize | Specifies the size of the <t VIDEOHDR> structure in bytes.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Indicates the specified device handle is invalid.
 *   @flag DV_ERR_NOMEM | Indicates the device is unable to allocate or lock memory.
 *
 * @comm Use this function after <f videoStreamInit> or
 *   after <f videoStreamReset> to prepare the data buffers
 *   for streaming data.
 *
 *   The <t VIDEOHDR> data structure and the data block pointed to by its
 *   <e VIDEOHDR.lpData> member must be allocated with <f GlobalAlloc> using the
 *   GMEM_MOVEABLE and GMEM_SHARE flags, and locked with <f GlobalLock>.
 *   Preparing a header that has already been prepared will have no effect
 *   and the function will return zero. Typically, this function is used
 *   to ensure that the buffer will be available for use at interrupt time.
 *
 * @xref <f videoStreamUnprepareHeader>
 ****************************************************************************/
DWORD WINAPI videoStreamPrepareHeader(HVIDEO hVideo,
		LPVIDEOHDR lpvideoHdr, DWORD dwSize)
{
    DWORD         wRet;

    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (IsBadWritePtr (lpvideoHdr, sizeof (VIDEOHDR)) )
        return DV_ERR_PARAM1;

    if (IsVideoHeaderPrepared(HVIDEO, lpvideoHdr))
    {
        DebugErr(DBF_WARNING,"videoStreamPrepareHeader: header is already prepared.");
        return DV_ERR_OK;
    }

    lpvideoHdr->dwFlags = 0;

    wRet = (DWORD)videoMessage((HVIDEO)hVideo, DVM_STREAM_PREPAREHEADER,
            (DWORD)lpvideoHdr, (DWORD)dwSize);

    if (wRet == DV_ERR_NOTSUPPORTED)
        wRet = videoPrepareHeader(lpvideoHdr, dwSize);

    if (wRet == DV_ERR_OK)
        MarkVideoHeaderPrepared(hVideo, lpvideoHdr);

    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoStreamUnprepareHeader | This function clears the
 *  preparation performed by <f videoStreamPrepareHeader>.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video
 *   device channel.
 *
 * @parm LPVIDEOHDR | lpvideoHdr |  Specifies a pointer to a <t VIDEOHDR>
 *   structure identifying the data buffer to be unprepared.
 *
 * @parm DWORD | dwSize | Specifies the size of the <t VIDEOHDR> structure in bytes.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Indicates the device handle specified is invalid.
 *   @flag DV_ERR_STILLPLAYING | Indicates the structure identified by <p lpvideoHdr>
 *   is still in the queue.
 *
 * @comm This function is the complementary function to <f videoStreamPrepareHeader>.
 *   You must call this function before freeing the data buffer with <f GlobalFree>.
 *   After passing a buffer to the device driver with <f videoStreamAddBuffer>, you
 *   must wait until the driver is finished with the buffer before calling
 *   <f videoStreamUnprepareHeader>. Unpreparing a buffer that has not been
 *   prepared or has been already unprepared has no effect,
 *   and the function returns zero.
 *
 * @xref <f videoStreamPrepareHeader>
 ****************************************************************************/
DWORD WINAPI videoStreamUnprepareHeader(HVIDEO hVideo, LPVIDEOHDR lpvideoHdr, DWORD dwSize)
{
    DWORD         wRet;

    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (IsBadWritePtr (lpvideoHdr, sizeof (VIDEOHDR)) )
        return DV_ERR_PARAM1;

    if (lpvideoHdr->dwFlags & VHDR_INQUEUE)
    {
        DebugErr(DBF_WARNING, "videoStreamUnprepareHeader: buffer still in queue.");
        return DV_ERR_STILLPLAYING;
    }

    if (!IsVideoHeaderPrepared(hVideo, lpvideoHdr))
    {
        DebugErr(DBF_WARNING,"videoStreamUnprepareHeader: header is not prepared.");
        return DV_ERR_OK;
    }

    wRet = (DWORD)videoMessage((HVIDEO)hVideo, DVM_STREAM_UNPREPAREHEADER,
            (DWORD)lpvideoHdr, (DWORD)dwSize);

    if (wRet == DV_ERR_NOTSUPPORTED)
        wRet = videoUnprepareHeader(lpvideoHdr, dwSize);

    if (wRet == DV_ERR_OK)
        MarkVideoHeaderUnprepared(hVideo, lpvideoHdr);

    return wRet;
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoStreamAddBuffer | This function sends a buffer to a
 *   video-capture device. After the buffer is filled by the device,
 *   the device sends it back to the application.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *
 * @parm LPVIDEOHDR | lpvideoHdr | Specifies a far pointer to a <t VIDEOHDR>
 *   structure that identifies the buffer.
 *
 * @parm DWORD | dwSize | Specifies the size of the <t VIDEOHDR> structure in bytes.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Indicates the device handle specified is invalid.
 *   @flag DV_ERR_UNPREPARED | Indicates the <p lpvideoHdr> structure hasn't been prepared.
 *   @flag DV_ERR_STILLPLAYING | Indicates a buffer is still in the queue.
 *   @flag DV_ERR_PARAM1 | The <p lpvideoHdr> parameter is invalid or
 *       the <e VIDEOHDR.dwBufferLength> member of the <t VIDEOHDR>
 *       structure is not set to the proper value.
 *
 * @comm The data buffer must be prepared with <f videoStreamPrepareHeader>
 *   before it is passed to <f videoStreamAddBuffer>. The <t VIDEOHDR> data
 *   structure and the data buffer referenced by its <e VIDEOHDR.lpData>
 *   member must be allocated with <f GlobalAlloc> using the GMEM_MOVEABLE
 *   and GMEM_SHARE flags, and locked with <f GlobalLock>. Set the
 *   <e VIDEOHDR.dwBufferLength> member to the size of the header.
 *
 * @xref <f videoStreamPrepareHeader>
 ****************************************************************************/
DWORD WINAPI videoStreamAddBuffer(HVIDEO hVideo, LPVIDEOHDR lpvideoHdr, DWORD dwSize)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (IsBadWritePtr (lpvideoHdr, sizeof (VIDEOHDR)) )
        return DV_ERR_PARAM1;

    if (!IsVideoHeaderPrepared(hVideo, lpvideoHdr))
    {
        DebugErr(DBF_WARNING, "videoStreamAddBuffer: buffer not prepared.");
        return DV_ERR_UNPREPARED;
    }

    if (lpvideoHdr->dwFlags & VHDR_INQUEUE)
    {
        DebugErr(DBF_WARNING, "videoStreamAddBuffer: buffer already in queue.");
        return DV_ERR_STILLPLAYING;
    }

    return (DWORD)videoMessage((HVIDEO)hVideo, DVM_STREAM_ADDBUFFER, (DWORD)lpvideoHdr, (DWORD)dwSize);
}



/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoStreamStop | This function stops streaming on a video channel.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video
 *   device channel.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following error is defined:
 *   @flag DV_ERR_INVALHANDLE | Indicates the specified device handle is invalid.
 *
 *   @flag DV_ERR_NOTSUPPORTED | Indicates the device does not support this
 *         function.
 * @comm If there are any buffers in the queue, the current buffer will be
 *   marked as done (the <e VIDEOHDR.dwBytesRecorded> member in
 *   the <t VIDEOHDR> header will contain the actual length of data), but any
 *   empty buffers in the queue will remain there. Calling this
 *   function when the channel is not started has no effect, and the
 *   function returns zero.
 *
 * @xref <f videoStreamStart> <f videoStreamReset>
 ****************************************************************************/
DWORD WINAPI videoStreamStop(HVIDEO hVideo)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    return videoMessage((HVIDEO)hVideo, DVM_STREAM_STOP, 0L, 0L);
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoStreamReset | This function stops streaming
 *	     on the specified video device channel and resets the current position
 *      to zero.  All pending buffers are marked as done and
 *      are returned to the application.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *
 *   @flag DV_ERR_INVALHANDLE | Indicates the device handle specified is invalid.
 *
 *   @flag DV_ERR_NOTSUPPORTED | Indicates the device does not support this
 *         function.
 *
 * @xref <f videoStreamReset> <f videoStreamStop> <f videoStreamAddBuffer> <f videoStreamClose>
/****************************************************************************/
DWORD WINAPI videoStreamReset(HVIDEO hVideo)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    return videoMessage((HVIDEO)hVideo, DVM_STREAM_RESET, 0L, 0L);
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoStreamGetPosition | This function retrieves the current
 *   position of the specified video device channel.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *
 * @parm LPMMTIME | lpInfo | Specifies a far pointer to an <t MMTIME>
 *   structure.
 *
 * @parm DWORD | dwSize | Specifies the size of the <t MMTIME> structure in bytes.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *
 *   @flag DV_ERR_INVALHANDLE | Indicates the specified device handle is invalid.
 *
 * @comm Before using <f videoStreamGetPosition>, set the
 *   <e MMTIME.wType> member of the <t MMTIME> structure to indicate
 *   the time format desired. After
 *   <f videoStreamGetPosition> returns, check the <e MMTIME.wType>
 *   member to  determine if the your time format is supported. If
 *   not, <e MMTIME.wType> specifies an alternate format.
 *   Video capture drivers typically provide the millisecond time
 *   format.
 *
 *   The position is set to zero when streaming is started with
 *   <f videoStreamStart>.
 ****************************************************************************/
DWORD WINAPI videoStreamGetPosition(HVIDEO hVideo, LPMMTIME lpInfo, DWORD dwSize)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (IsBadWritePtr (lpInfo, sizeof (MMTIME)) )
        return DV_ERR_PARAM1;

    return videoMessage(hVideo, DVM_STREAM_GETPOSITION,
            (DWORD)lpInfo, (DWORD)dwSize);
}

// ============================================

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoStreamInit | This function initializes a video
 *     device channel for streaming.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *
 * @parm DWORD | dwMicroSecPerFrame | Specifies the number of microseconds
 *     between frames.
 *
 * @parm DWORD | dwCallback | Specifies the address of a callback
 *   function or a handle to a window called during video
 *   streaming. The callback function or window processes
 *  messages related to the progress of streaming.
 *
 * @parm DWORD | dwCallbackInstance | Specifies user
 *  instance data passed to the callback function. This parameter is not
 *  used with window callbacks.
 *
 * @parm DWORD | dwFlags | Specifies flags for opening the device channel.
 *   The following flags are defined:
 *   @flag CALLBACK_WINDOW | If this flag is specified, <p dwCallback> is
 *      a window handle.
 *   @flag CALLBACK_FUNCTION | If this flag is specified, <p dwCallback> is
 *      a callback procedure address.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_BADDEVICEID | Indicates the device ID specified in
 *         <p hVideo> is not valid.
 *   @flag DV_ERR_ALLOCATED | Indicates the resource specified is already allocated.
 *   @flag DV_ERR_NOMEM | Indicates the device is unable to allocate or lock memory.
 *
 * @comm If a window or function is chosen to receive callback information, the following
 *   messages are sent to it to indicate the
 *   progress of video input:
 *
 *   <m MM_DRVM_OPEN> is sent at the time of <f videoStreamInit>
 *
 *   <m MM_DRVM_CLOSE> is sent at the time of <f videoStreamFini>
 *
 *   <m MM_DRVM_DATA> is sent when a buffer of image data is available
 *
 *   <m MM_DRVM_ERROR> is sent when an error occurs
 *
 *   Callback functions must reside in a DLL.
 *   You do not have to use <f MakeProcInstance> to get
 *   a procedure-instance address for the callback function.
 *
 * @cb void CALLBACK | videoFunc | <f videoFunc> is a placeholder for an
 *   application-supplied function name. The actual name must be exported by
 *   including it in an EXPORTS statement in the DLL's module-definition file.
 *   This is used only when a callback function is specified in
 *   <f videoStreamInit>.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel
 *   associated with the callback.
 *
 * @parm DWORD | wMsg | Specifies the <m MM_DRVM_> messages. Messages indicate
 *       errors and when image data is available. For information on
 *       these messages, see <f videoStreamInit>.
 *
 * @parm DWORD | dwInstance | Specifies the user instance
 *   data specified with <f videoStreamInit>.
 *
 * @parm DWORD | dwParam1 | Specifies a parameter for the message.
 *
 * @parm DWORD | dwParam2 | Specifies a parameter for the message.
 *
 * @comm Because the callback is accessed at interrupt time, it must reside
 *   in a DLL and its code segment must be specified as FIXED in the
 *   module-definition file for the DLL. Any data the callback accesses
 *   must be in a FIXED data segment as well. The callback may not make any
 *   system calls except for <f PostMessage>, <f timeGetSystemTime>,
 *   <f timeGetTime>, <f timeSetEvent>, <f timeKillEvent>,
 *   <f midiOutShortMsg>, <f midiOutLongMsg>, and <f OutputDebugStr>.
 *
 * @xref <f videoOpen> <f videoStreamFini> <f videoClose>
 ****************************************************************************/
DWORD WINAPI videoStreamInit(HVIDEO hVideo,
              DWORD dwMicroSecPerFrame, DWORD_PTR dwCallback,
              DWORD_PTR dwCallbackInst, DWORD dwFlags)
{
    VIDEO_STREAM_INIT_PARMS vsip;

    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (dwCallback && ((dwFlags & CALLBACK_TYPEMASK) == CALLBACK_FUNCTION) ) {
        if (IsBadCodePtr ((FARPROC) dwCallback) )
            return DV_ERR_PARAM2;
        if (!dwCallbackInst)
            return DV_ERR_PARAM2;
    }

    if (dwCallback && ((dwFlags & CALLBACK_TYPEMASK) == CALLBACK_WINDOW) ) {
        if (!IsWindow((HWND) dwCallback) )
            return DV_ERR_PARAM2;
    }

    vsip.dwMicroSecPerFrame = dwMicroSecPerFrame;
    vsip.dwCallback = dwCallback;
    vsip.dwCallbackInst = dwCallbackInst;
    vsip.dwFlags = dwFlags;
    vsip.hVideo = (DWORD_PTR)hVideo;

    return videoMessage(hVideo, DVM_STREAM_INIT,
                (DWORD_PTR) (LPVIDEO_STREAM_INIT_PARMS) &vsip,
                (DWORD) sizeof (VIDEO_STREAM_INIT_PARMS));
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoStreamFini | This function terminates streaming
 *     from the specified device channel.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Indicates the device handle specified is invalid.
 *   @flag DV_ERR_STILLPLAYING | Indicates there are still buffers in the queue.
 *
 * @comm If there are buffers that have been sent with
 *   <f videoStreamAddBuffer> that haven't been returned to the application,
 *   this operation will fail. Use <f videoStreamReset> to return all
 *   pending buffers.
 *
 *   Each call to <f videoStreamInit> must be matched with a call to
 *   <f videoStreamFini>.
 *
 *   For VIDEO_EXTERNALIN channels, this function is used to
 *   halt capturing of data to the frame buffer.
 *
 *   For VIDEO_EXTERNALOUT channels supporting overlay,
 *   this function is used to disable the overlay.
 *
 * @xref <f videoStreamInit>
 ****************************************************************************/
DWORD WINAPI videoStreamFini(HVIDEO hVideo)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    return videoMessage(hVideo, DVM_STREAM_FINI, 0L, 0L);
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoStreamStart | This function starts streaming on the
 *   specified video device channel.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Indicates the device handle specified is invalid.
 *
 *   @flag DV_ERR_NOTSUPPORTED | Indicates the device does not support this
 *         function.
 *
 * @xref <f videoStreamReset> <f videoStreamStop> <f videoStreamAddBuffer> <f videoStreamClose>
/****************************************************************************/
DWORD WINAPI videoStreamStart(HVIDEO hVideo)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    return videoMessage(hVideo, DVM_STREAM_START, 0L, 0L);
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoStreamGetError | This function returns the error
 *   most recently encountered.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *
 * @parm LPDWORD | lpdwErrorID | Specifies a far pointer to the <t DWORD>
 *      used to return the error ID.
 *
 * @parm LPDWORD | lpdwErrorValue | Specifies a far pointer to the <t DWORD>
 *      used to return the number of frames skipped.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following error is defined:
 *   @flag DV_ERR_INVALHANDLE | Indicates the device handle specified is invalid.
 *
 * @comm While streaming video data, a capture
 *      driver can fill buffers faster than the client application can
 *      save the buffers to disk.  In this case, the
 *      DV_ERR_NO_BUFFERS error is returned in <p lpdwErrorID>
 *      and <p lpdwErrorValue> contains a count of the number of
 *      frames missed.  After
 *      receiving this message and returning the error status, a driver
 *      should reset its internal error flag to DV_ERR_OK and
 *      the count of missed frames to zero.
 *
 *      Applications should send this message frequently during capture
 *      since some drivers which do not have access to interrupts use
 *      this message to trigger buffer processing.
 *
 * @xref <f videoOpen>
/****************************************************************************/
DWORD WINAPI videoStreamGetError(HVIDEO hVideo, LPDWORD lpdwError,
        LPDWORD lpdwFramesSkipped)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (IsBadWritePtr (lpdwError, sizeof (DWORD)) )
        return DV_ERR_PARAM1;

    if (IsBadWritePtr (lpdwFramesSkipped, sizeof (DWORD)) )
        return DV_ERR_PARAM2;

    return videoMessage(hVideo, DVM_STREAM_GETERROR, (DWORD) lpdwError,
        (DWORD) lpdwFramesSkipped);
}

/*****************************************************************************
 * @doc EXTERNAL  VIDEO
 *
 * @api DWORD | videoFrame | This function transfers a single frame
 *   to or from a video device channel.
 *
 * @parm HVIDEO | hVideo | Specifies a handle to the video device channel.
 *      The channel must be of type VIDEO_IN or VIDEO_OUT.
 *
 * @parm LPVIDEOHDR | lpVHdr | Specifies a far pointer to an <t VIDEOHDR>
 *      structure.
 *
 * @rdesc Returns zero if the function was successful. Otherwise, it returns
 *   an error number. The following errors are defined:
 *   @flag DV_ERR_INVALHANDLE | Specified device handle is invalid.
 *   @flag DV_ERR_PARAM1 | The <p lpVDHdr> parameter is invalid or
 *       the <e VIDEOHDR.dwBufferLength> member of the <t VIDEOHDR>
 *       structure is not set to the proper value.
 *
 * @comm Use this function with a VIDEO_IN channel to transfer a single
 *      image from the frame buffer.
 *      Use this function with a VIDEO_OUT channel to transfer a single
 *      image to the frame buffer.
 *
 * @xref <f videoOpen>
/****************************************************************************/
DWORD WINAPI videoFrame (HVIDEO hVideo, LPVIDEOHDR lpVHdr)
{
    if (!hVideo)
        return DV_ERR_INVALHANDLE;

    if (!lpVHdr)
        return DV_ERR_PARAM1;

    if (IsBadWritePtr (lpVHdr, sizeof (VIDEOHDR)) )
        return DV_ERR_PARAM1;

    return videoMessage(hVideo, DVM_FRAME, (DWORD) lpVHdr,
                        sizeof(VIDEOHDR));
}

#endif  // ifdef WIN16

/**************************************************************************
* @doc INTERNAL VIDEO
*
* @api void | videoCleanup | clean up video stuff
*   called in MSVIDEOs WEP()
*
**************************************************************************/
void FAR PASCAL videoCleanup(HTASK hTask)
{
#ifdef WIN16
        videoFreeDriverList();
#endif
}

