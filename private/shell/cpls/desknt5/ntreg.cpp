/**************************************************************************\
* Module Name: ntreg.cpp
*
* CRegistrySettings class
*
*  This class handles getting registry information for display driver
*  information.
*
* Copyright (c) Microsoft Corp.  1992-1998 All Rights Reserved
*
\**************************************************************************/

#include "precomp.h"
#include "ntreg.hxx"

#ifdef WINNT

//
// CRegistrySettings constructor
//

CRegistrySettings::CRegistrySettings(LPTSTR pstrDeviceKey):

    hkServiceReg(NULL), hkVideoRegR(NULL), hkVideoRegW(NULL), pszDrvName(NULL),
    pszKeyName(NULL), pszDevInstanceId(NULL), bdisablable(FALSE) {


    TCHAR szName[MAX_PATH];
    TCHAR szPath[MAX_PATH];
    TCHAR szGroup[MAX_PATH];
    LPTSTR pszPath;
    HKEY hkeyMap;
    DWORD cb;
    LPTSTR pszName = NULL;
    TCHAR   szDeviceInstanceID[DEV_INSTANCE_ID_LENGTH];

    ASSERT(lstrlen(pstrDeviceKey) < MAX_PATH);
    
    // Copy the data to local buffer.
    lstrcpy(szPath, pstrDeviceKey);

    //
    // At this point, szPath has something like:
    //  \REGISTRY\Machine\System\ControlSet001\Services\Jazzg300\Device0
    //
    // To use the Win32 registry calls, we have to strip off the \REGISTRY
    // and convert \Machine to HKEY_LOCAL_MACHINE
    //

    hkeyMap = HKEY_LOCAL_MACHINE;
    pszPath = SubStrEnd(SZ_REGISTRYMACHINE, szPath);

    //
    // try to open the registry key.
    // Get Read access and Write access seperately so we can query stuff
    // when an ordinary user is logged on.
    //

    if (RegOpenKeyEx(hkeyMap,
                     pszPath,
                     0,
                     KEY_READ,
                     &hkVideoRegR) != ERROR_SUCCESS) {

        hkVideoRegR = 0;

    }

    if (RegOpenKeyEx(hkeyMap,
                     pszPath,
                     0,
                     KEY_WRITE,
                     &hkVideoRegW) != ERROR_SUCCESS) {

        hkVideoRegW = 0;

    }

    //
    // Save the mini port driver name
    //

    {
        LPTSTR pszEnd;
        HKEY hkeyDriver;
        LPTSTR pszDeviceId;  

        pszEnd = pszPath + lstrlen(pszPath);

        //
        // Remove the \DeviceX at the end of the path
        //

        while (pszEnd != pszPath && *pszEnd != TEXT('\\')) {

            pszEnd--;
        }

        pszDeviceId = SubStrEnd(SZ_DEVICE, pszEnd); // if pszEnd Points to a string like "\Device0",
                                                 // then, pszDeviceId points to a string "0".
        *pszEnd = UNICODE_NULL;


        //
        // First check if their is a binary name in there that we should use.
        //

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         pszPath,
                         0,
                         KEY_READ,
                         &hkeyDriver) ==  ERROR_SUCCESS) {

            HKEY    hkeyEnum;

            //
            // Open the "Enum" key under the devicename
            //
            if(RegOpenKeyEx(hkeyDriver, 
                            SZ_ENUM,
                            0,
                            KEY_READ,
                            &hkeyEnum) == ERROR_SUCCESS)
            {
                cb = sizeof(szDeviceInstanceID);
                if(RegQueryValueEx(hkeyEnum,
                                    pszDeviceId,    //in the form of "0", "1" etc.,
                                    NULL,
                                    NULL,
                                    (LPBYTE)szDeviceInstanceID,
                                    &cb) == ERROR_SUCCESS)
                {
                    this->pszDevInstanceId = (LPTSTR)LocalAlloc(LMEM_ZEROINIT,
                                              (lstrlen(szDeviceInstanceID) + 1) * sizeof(TCHAR));

                    if (this->pszDevInstanceId != NULL) 
                        CopyMemory(this->pszDevInstanceId, szDeviceInstanceID, lstrlen(szDeviceInstanceID) * sizeof(TCHAR));
                }
                
                RegCloseKey(hkeyEnum);
            }
                            

            //
            // parse the device map and open the registry.
            //

            cb = sizeof(szName);

            if (RegQueryValueEx(hkeyDriver,
                                L"ImagePath",
                                NULL,
                                NULL,
                                (LPBYTE)szName,
                                &cb) == ERROR_SUCCESS) {

                //
                // The is a binary.
                // extract the name, which will be of the form ...\driver.sys
                //

                {

                    LPTSTR pszDriver, pszDriverEnd;

                    pszDriver = szName;
                    pszDriverEnd = pszDriver + lstrlen(pszDriver);

                    while(pszDriverEnd != pszDriver &&
                          *pszDriverEnd != TEXT('.')) {
                        pszDriverEnd--;
                    }

                    *pszDriverEnd = UNICODE_NULL;

                    while(pszDriverEnd != pszDriver &&
                          *pszDriverEnd != TEXT('\\')) {
                        pszDriverEnd--;
                    }

                    pszDriverEnd++;

                    //
                    // If pszDriver and pszDriverEnd are different, we now
                    // have the driver name.
                    //

                    if (pszDriverEnd > pszDriver) {

                        pszName = pszDriverEnd;

                    }
                }
            }

            //
            // while we have this key open, determine if the driver is in
            // the Video group (to know if we can disable it automatically.
            //

            cb = sizeof(szName);

            if (RegQueryValueEx(hkeyDriver,
                                L"Group",
                                NULL,
                                NULL,
                                (LPBYTE)szGroup,
                                &cb) == ERROR_SUCCESS) {

                //
                // Compare the string , case insensitive, to the "Video" group.
                //

                bdisablable = !(BOOL)(lstrcmpi(szGroup, L"Video"));

            }

            RegCloseKey(hkeyDriver);
        }

        while(pszEnd > pszPath && *pszEnd != TEXT('\\')) {
            pszEnd--;
        }
        pszEnd++;

        //
        // Save the key name
        //

        this->pszKeyName = (LPTSTR)LocalAlloc(LMEM_ZEROINIT,
                                              (lstrlen(pszEnd) + 1) * sizeof(TCHAR));

        if (this->pszKeyName != NULL) {

            CopyMemory(this->pszKeyName, pszEnd, lstrlen(pszEnd) * sizeof(TCHAR));

        }

        //
        // something failed trying to get the binary name.
        // just get the device name
        //

        if (!pszName) {

            pszDrvName = pszKeyName;

        } else {

            this->pszDrvName = (LPTSTR)LocalAlloc(LMEM_ZEROINIT,
                                                  (lstrlen(pszName) + 1) * sizeof(TCHAR));

            if (this->pszDrvName != NULL) {

                CopyMemory(this->pszDrvName, pszName, lstrlen(pszName) * sizeof(TCHAR));

            }
        }
    }

    //
    // Finally, Get a "write" handle to the service Key so we can disable the
    // driver at a later time.
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     pszPath,
                     0,
                     KEY_READ | KEY_WRITE,
                     &hkServiceReg) !=  ERROR_SUCCESS) {

        this->hkServiceReg = NULL;
        bdisablable = FALSE;

    }


}

//
// CRegistrySettings destructor
//
//  (gets called whenever a CRegistrySettings is destroyed or goes out of scope)
//

CRegistrySettings::~CRegistrySettings() {

    //
    // Close the registry
    //

    if (hkVideoRegW) {
        RegCloseKey(hkVideoRegW);
    }

    if (hkVideoRegR) {
        RegCloseKey(hkVideoRegR);
    }

    if (hkServiceReg) {
        RegCloseKey(hkServiceReg);
    }

    //
    // Free the strings
    //
    if (pszKeyName) {
        LocalFree(pszKeyName);
    }

    if (pszKeyName != pszDrvName && pszDrvName) {
        LocalFree(pszDrvName);
    }

    if(pszDevInstanceId) {
        LocalFree(pszDevInstanceId);
    }

}

//
// CloneDescription
//
// Gets the descriptive name of the driver out of the registry.
// (eg. "Stealth Pro" instead of "S3").  If there is no
// DeviceDescription value in the registry, then it returns
// the generic driver name (like 'S3' or 'ATI')
//
// NOTE: The caller must LocalFree the returned pointer when they
// are done with it.  (Which is why it is called Clone instead of Get)
//

LPTSTR CRegistrySettings::CloneDescription(void) {

    DWORD cb, dwType;
    LPTSTR psz = NULL;
    LONG lRet;

    //
    // query the size of the string
    //

    cb = 0;
    lRet = RegQueryValueEx(hkVideoRegR,
                           SZ_DEVICEDESCRIPTION,
                           NULL,
                           &dwType,
                           NULL,
                           &cb);

    //
    // check to see if there is a string, and the string is more than just
    // a UNICODE_NULL (detection will put an empty string there).
    //

    if ( (lRet == ERROR_SUCCESS || lRet == ERROR_MORE_DATA) &&
         (cb > 2) ) {

        //
        // alloc a string buffer
        //

        psz = (LPTSTR)LocalAlloc(LPTR, cb);

        if (psz) {

            //
            // get the string
            //

            if (RegQueryValueEx(hkVideoRegR,
                                SZ_DEVICEDESCRIPTION,
                                NULL,
                                &dwType,
                                (LPBYTE)psz,
                                &cb) != ERROR_SUCCESS) {

                LocalFree(psz);
                psz = NULL;

            }
        }
    }

    if (!psz) {

        //
        // we can't read the registry, just us the generic name
        //

        ULONG length = lstrlen(pszDrvName) * sizeof(TCHAR);

        psz = (LPTSTR)LocalAlloc(LPTR, length);

        if (psz) {

            CopyMemory(psz, pszDrvName, length);

        }
    }

    ASSERT(psz != NULL);

    return psz;
}

VOID
CRegistrySettings::GetDeviceInstanceId(LPWSTR   lpwDeviceInstanceId, int cch)
{
    if(lpwDeviceInstanceId)
    {
        if(pszDevInstanceId)
        {
#ifdef UNICODE
            lstrcpyn(lpwDeviceInstanceId, pszDevInstanceId, cch);
#else
            MultibyteToWideChar(CP_ACP, 0, pszDevInstanceId, -1, lpwDeviceInstanceId, cch);
#endif
        }
        else
            *lpwDeviceInstanceId = UNICODE_NULL;
    }
}

//
// Method to get the hardware information fields.
//

VOID
CRegistrySettings::GetHardwareInformation(
    PDISPLAY_REGISTRY_HARDWARE_INFO pInfo)
{

    DWORD cb, dwType;
    DWORD i;
    LONG lRet;

    LPWSTR pKeyNames[5] = {
        L"HardwareInformation.MemorySize",
        L"HardwareInformation.ChipType",
        L"HardwareInformation.DacType",
        L"HardwareInformation.AdapterString",
        L"HardwareInformation.BiosString"
    };

    ZeroMemory(pInfo, sizeof(DISPLAY_REGISTRY_HARDWARE_INFO));

    //
    // Query each entry one after the other.
    //

    for (i = 0; i < 5; i++) {

        //
        // query the size of the string
        //

        cb = 256;
        lRet = RegQueryValueExW(hkVideoRegR,
                                pKeyNames[i],
                                NULL,
                                &dwType,
                                NULL,
                                &cb);

        if (lRet == ERROR_SUCCESS) {

            if (i == 0) {

                ULONG mem;

                cb = 4;

                RegQueryValueExW(hkVideoRegR,
                                 pKeyNames[i],
                                 NULL,
                                 &dwType,
                                 (PUCHAR) (&mem),
                                 &cb);

                //
                // If we queried the memory size, we actually have
                // a DWORD.  Transform the DWORD to a string
                //

                // Divide down to Ks

                mem =  mem >> 10;

                // if a MB multiple, divide again.

                if ((mem & 0x3FF) != 0) {

                    wsprintf((LPWSTR)pInfo, L"%d KB", mem );

                } else {

                    wsprintf((LPWSTR)pInfo, L"%d MB", mem >> 10 );

                }

            } else {

                cb = 256;

                //
                // get the string
                //

                RegQueryValueExW(hkVideoRegR,
                                 pKeyNames[i],
                                 NULL,
                                 &dwType,
                                 (LPBYTE) pInfo,
                                 &cb);

            }
        }
        else
        {
            //
            // Put in the default string
            //

            LoadString(hInstance,
                       IDS_UNAVAILABLE,
                       (LPWSTR)pInfo,
                       256);
        }

        pInfo = (PDISPLAY_REGISTRY_HARDWARE_INFO)((PUCHAR)pInfo + 256);
    }


    return;
}

//
// returns the display drivers
//

LPTSTR CRegistrySettings::CloneDisplayFileNames(BOOL bPreprocess) {
    DWORD cb, dwType;
    LPTSTR psz, pszName, tmppsz;
    LONG lRet;
    DWORD cNumStrings;

    //
    // query the size of the string
    //

    cb = 0;

    lRet = RegQueryValueEx(hkVideoRegR,
                           SZ_INSTALLEDDRIVERS,
                           NULL,
                           &dwType,
                           NULL,
                           &cb);

    if (lRet != ERROR_SUCCESS && lRet != ERROR_MORE_DATA) {

        return NULL;

    }

    //
    // alloc a string buffer
    //

    psz = (LPTSTR)LocalAlloc(LPTR, cb);

    if (psz) {

        //
        // get the string
        //

        if (RegQueryValueEx(hkVideoRegR,
                            SZ_INSTALLEDDRIVERS,
                            NULL,
                            &dwType,
                            (LPBYTE)psz,
                            &cb) != ERROR_SUCCESS) {

            LocalFree(psz);
            return NULL;

        }

        //
        // If the caller want a preprocessed list, we will add the commas,
        // remove the NULLs, etc.
        //

        if (bPreprocess) {

            //
            // if it is a multi_sz, count the number of sub strings.
            //

            if (dwType == REG_MULTI_SZ) {

                tmppsz = psz;
                cNumStrings = 0;

                while(*tmppsz) {

                    while(*tmppsz++);
                    cNumStrings++;

                }

            } else {

                cNumStrings = 1;

            }

            //
            // the buffer must contain enought space for :
            // the miniport name,
            // the .sys extension,
            // all the display driver names,
            // the .dll extension for each of them.
            // and place for ", " between each name
            // we forget about NULL, so our buffer is a bit bigger.
            //

            cb = lstrlen(this->GetMiniPort()) +
                 cb +
                 lstrlen(SZ_DOTSYS) +
                 cNumStrings * (lstrlen(SZ_DOTDLL) + lstrlen(SZ_FILE_SEPARATOR));


            pszName = (LPTSTR)LocalAlloc(LMEM_ZEROINIT, cb * sizeof(TCHAR));

            if (pszName != NULL) {

                lstrcpy(pszName, this->GetMiniPort());
                lstrcat(pszName, SZ_DOTSYS);

                tmppsz = psz;

                while (cNumStrings--) {

                    lstrcat(pszName, SZ_FILE_SEPARATOR);
                    lstrcat(pszName, tmppsz);
                    lstrcat(pszName, SZ_DOTDLL);

                    while (*tmppsz++);
                }
            }

            LocalFree(psz);
            psz = pszName;
        }
    }

    //
    // return it to the caller
    //

    return psz;

}


#endif
