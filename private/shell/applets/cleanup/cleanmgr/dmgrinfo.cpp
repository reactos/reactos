/*
**------------------------------------------------------------------------------
** Module:  Disk Cleanup Applet
** File:    dmgrinfo.c
**
** Purpose: Defines the CleanupMgrInfo class for the property tab
** Notes:   
** Mod Log: Created by Jason Cobb (2/97)
**
** Copyright (c)1997 Microsoft Corporation, All Rights Reserved
**------------------------------------------------------------------------------
*/

/*
**------------------------------------------------------------------------------
** Project include files
**------------------------------------------------------------------------------
*/
#include "common.h"
#include <limits.h>
#include <emptyvc.h>
#include "dmgrinfo.h"
#include "dmgrdlg.h"
#include "diskutil.h"
#include "resource.h"
#include "msprintf.h"



/*
**------------------------------------------------------------------------------
**  Local variables
**------------------------------------------------------------------------------
*/
HINSTANCE   CleanupMgrInfo::hInstance             = NULL;


/*
**------------------------------------------------------------------------------
** Function prototypes
**------------------------------------------------------------------------------
*/
INT_PTR CALLBACK
ScanAbortDlgProc(
    HWND hDlg,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );

void
ScanAbortThread(
    CleanupMgrInfo *pcmi
    );

INT_PTR CALLBACK
PurgeAbortDlgProc(
    HWND hDlg,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );

void
PurgeAbortThread(
    CleanupMgrInfo *pcmi
    );

/*
**------------------------------------------------------------------------------
** Function definitions
**------------------------------------------------------------------------------
*/


void
CleanupMgrInfo::Register(
    HINSTANCE hInstance
    )
{
    CleanupMgrInfo::hInstance = hInstance;
}

void
CleanupMgrInfo::Unregister(
    void
    )
{
    CleanupMgrInfo::hInstance= NULL;
}

/*
**------------------------------------------------------------------------------
** GetCleanupMgrInfoPointer
**
** Purpose:    
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
CleanupMgrInfo * GetCleanupMgrInfoPointer(
    HWND hDlg
    )
{
    //   
    //Get the DriveInfo
    //
    CleanupMgrInfo * pcmi = (CleanupMgrInfo *)GetWindowLongPtr(hDlg, DWLP_USER);

    return pcmi;
}



/*
**------------------------------------------------------------------------------
** CleanupMgrInfo method definitions
**------------------------------------------------------------------------------
*/

/*
**------------------------------------------------------------------------------
** CleanupMgrInit::init
**
** Purpose:    sets to default values
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
void 
CleanupMgrInfo::init(void)
{
    dre             = Drive_INV;
    szVolName[0]    = 0;
    vtVolume        = vtINVALID;
    dwUIFlags       = 0;
    bPurgeFiles     = TRUE;
}


/*
**------------------------------------------------------------------------------
** CleanupMgrInfo::destroy
**
** Purpose:    releases any dynamic memory
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
void 
CleanupMgrInfo::destroy(void)
{
    //
    //Set values back to defaults
    //
    init();
}

/*
**------------------------------------------------------------------------------
** CleanupMgrInfo::CleanupMgrInfo
**
** Purpose:    Default constructor
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
CleanupMgrInfo::CleanupMgrInfo (void)
{
    init();
}

/*
**------------------------------------------------------------------------------
** CleanupMgrInfo::CleanupMgrInfo
**
** Purpose:    Constructor
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
CleanupMgrInfo::CleanupMgrInfo(
    LPTSTR lpDrive,
    DWORD dwFlags,
    ULONG ulProfile
    )
{
    HRESULT hr;

    init();
    if (create(lpDrive, dwFlags))
    {
        CoInitialize(NULL);

        dwReturnCode = RETURN_SUCCESS;
        dwUIFlags = dwFlags;
        ulSAGEProfile = ulProfile;
        bAbortScan = FALSE;
        bAbortPurge = FALSE;
    
        volumeCacheCallBack = NULL;
        pIEmptyVolumeCacheCallBack = NULL;
        volumeCacheCallBack = new CVolumeCacheCallBack();
        hr = volumeCacheCallBack->QueryInterface(IID_IEmptyVolumeCacheCallBack,
                                           (LPVOID*)&pIEmptyVolumeCacheCallBack);

        if (hr != NOERROR)
        {
            MiDebugMsg((hr, "CleanupMgrInfo::CleanupMgrInfo failed with error "));
        }

        //
        //Initialize all of the cleanup clients
        //
        if (initializeClients() && !(dwUIFlags & FLAG_TUNEUP) && !(dwUIFlags & FLAG_SAGESET))
        {
            //
            //Have all of the cleanup clients calculate the ammount of disk
            //space that they can free up.
            //
            getSpaceUsedByClients();
        }
    }
}

/*
**------------------------------------------------------------------------------
** CleanupMgrInfo::~CleanupMgrInfo
**
** Purpose:    Destructor
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
CleanupMgrInfo::~CleanupMgrInfo (void)
{
    if (isValid())
    {
        //
        //Cleanup the Volume Cache Clients
        //
        deactivateClients();    

        volumeCacheCallBack->Release();
   
        CoUninitialize();

        destroy();   
    }
}

/*
**------------------------------------------------------------------------------
** CleanupMgrInfo::create
**
** Purpose:    Gets Drive info from drive letter
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
BOOL 
CleanupMgrInfo::create(
    LPTSTR lpDrive,
    DWORD Flags
    )
{
     
    //
    //Note:  Make sure the assigns to zero stay current
    //       otherwise we might get garbage stats if
    //       we fail because of lack of free space
    //
    DWORD cSectorsPerCluster;
    DWORD cBytesPerSector;
    DWORD cBytesPerCluster;
    DWORD cFreeClusters;
    DWORD cUsedClusters;
    DWORD cTotalClusters;
    ULARGE_INTEGER cbFree;
    ULARGE_INTEGER cbUsed;
    ULARGE_INTEGER cbTotal;
#ifdef NEC_98
    drenum drive;
    hardware hw_type;
#endif

    cbFree.QuadPart = 0;
    cbUsed.QuadPart = 0;
    
    //
    //Cleanup up any old stuff
    //
    destroy();

      
    //
    //Check parameters
    //
    if (lpDrive == NULL)
        return FALSE;

      
    //  
    //Is it a valid drive path
    //
    if (!fIsValidDriveString(lpDrive))
        return FALSE;

    //
    //Get drive from path
    //
    if (!GetDriveFromString(lpDrive, dre))
        return FALSE;

    lstrcpy(szRoot, lpDrive);

      
    // 
    // Step 2.  Get general info from drive
    //

    //
    //Get volume name
    //
    if (!GetVolumeInformation (szRoot,                              // Root name
                               szVolName, sizeof(szVolName),        // Volume Name
                               NULL,                                // Volume serial number
                               NULL,                                // Max path length
                               NULL,                                // flags
                               szFileSystem, sizeof(szFileSystem))) // file system name                         
    {
        //Error - failed to get volume name
        goto lblERROR;
    }

    //
    //Get the Driver Icon
    //
    if (Flags & FLAG_SAGESET)
        hDriveIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(ICON_CLEANMGR));
    else
        hDriveIcon = GetDriveIcon(dre, FALSE);

    //  
    //Get Hardware type
    //
    if (!GetHardwareType(dre, hwHardware))
    {
        //Error - failed to get hardware
        goto lblERROR;
    }


#ifdef NEC_98
    drive = Drive_A;
    GetHardwareType (Drive_A, hw_type);

    if (hw_type != hwFixed) 
    {
        drive = Drive_B;
        GetHardwareType (Drive_B, hw_type);

        if (hw_type != hwFixed)
            drive = Drive_C;
    }
#endif


    // 
    //Get disk statistics
    //
    if (!GetDiskFreeSpace (szRoot, 
                           &cSectorsPerCluster, 
                           &cBytesPerSector,
                           &cFreeClusters,
                           &cTotalClusters))
    {
        //Error - couldn't get drive stats
        goto lblERROR;
    }
      
    //  
    //Calculate secondary statistics
    //
    cBytesPerCluster = cBytesPerSector * cSectorsPerCluster;
    if (cTotalClusters >= cFreeClusters)
        cUsedClusters = cTotalClusters - cFreeClusters;
    else
        cUsedClusters = 0L;

    cbFree.QuadPart   = UInt32x32To64(cFreeClusters, cBytesPerCluster);
    cbUsed.QuadPart   = UInt32x32To64(cUsedClusters, cBytesPerCluster);
    cbTotal.QuadPart  = cbFree.QuadPart + cbUsed.QuadPart;

    //
    //Get the current low disk space ratio
    //
    cbLowSpaceThreshold = GetFreeSpaceRatio(dre, cbTotal);

    //
    //Should we also load the agressive cleaners? We only do this if we
    //are below are critical threshold of disk space left.
    //
    if (cbLowSpaceThreshold.QuadPart >= cbFree.QuadPart)
    {
        MiDebugMsg((0, "*****We are in aggressive mode*****"));
        bOutOfDiskSpace = TRUE;
    }
    else
        bOutOfDiskSpace = FALSE;

    // 
    // Step 3.  Save stats
    //

    cbDriveFree          = cbFree;
    cbDriveUsed          = cbUsed;
    cbEstCleanupSpace.QuadPart    = 0;

    //
    //Success
    //
    return TRUE;

lblERROR:
    //  
    //Error
    //
    destroy();
    return FALSE;
}

/*
**------------------------------------------------------------------------------
** CleanupMgrInfo::initializeClients
**
** Purpose:    Initializes all of the Volume Cache Clients
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
BOOL 
CleanupMgrInfo::initializeClients(void)
{
    HKEY    hKeyVolCache;
    DWORD   iSubKey;
    DWORD   dwClient;
    TCHAR   szVolCacheClient[MAX_PATH];
    TCHAR   szGUID[MAX_PATH];
    DWORD   dwGUIDSize;
    DWORD   dwType;
    DWORD   dwState, cb, cw;
    TCHAR   szProfile[64];
    BOOL    bRet = TRUE;
    BOOL    bCleanup;

    iNumVolumeCacheClients = 0;
    pClientInfo = NULL;
    
    MiDebugMsg((0, "CleanupMgrInfo::initializeClients entered"));

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_VOLUMECACHE, 0, KEY_READ, &hKeyVolCache) == ERROR_SUCCESS)
    {
        //
        //Enumerate through all of the clients to see how large we need to make the pClientInfo array
        //

        // BUGBUG: Use RegQueryInfoKey to save some ring transitions

        iSubKey = 0;
        while(RegEnumKey(hKeyVolCache, iSubKey, szVolCacheClient, sizeof(szVolCacheClient)) != ERROR_NO_MORE_ITEMS)
        {
            iSubKey++;        
        }
        
        if ((pClientInfo = (PCLIENTINFO)LocalAlloc(LPTR, (iSubKey * sizeof(CLIENTINFO)))) == NULL)
        {
#ifdef DEBUG
            MessageBox(NULL, TEXT("FATAL ERROR LocalAlloc() failed!"), TEXT("CLEANMGR DEBUG"), MB_OK);
#endif
            RegCloseKey(hKeyVolCache);
            return FALSE;
        }
        
        //
        //Fill in the pClientInfo data structure and initialize all of the volume cache clients
        //   
        iSubKey = 0;
        dwClient = 0;
        while(RegEnumKey(hKeyVolCache, iSubKey, szVolCacheClient, sizeof(szVolCacheClient)) != ERROR_NO_MORE_ITEMS)
        {
            // default is we failed, so cleanup the current item....
            bCleanup = TRUE;
            
            if (RegOpenKeyEx(hKeyVolCache, szVolCacheClient, 0, MAXIMUM_ALLOWED, &(pClientInfo[dwClient].hClientKey)) == ERROR_SUCCESS)
            {
                lstrcpy(pClientInfo[dwClient].szRegKeyName, szVolCacheClient);
            
                dwGUIDSize = sizeof(szGUID);
                dwType = REG_SZ;
                if (RegQueryValueEx(pClientInfo[dwClient].hClientKey, NULL, NULL, &dwType, (LPBYTE)szGUID, &dwGUIDSize) == ERROR_SUCCESS)
                {
                    HRESULT hr;
                    WCHAR   wcsFmtID[39];

#ifdef UNICODE
                    StrCpyN( wcsFmtID, szGUID, ARRAYSIZE( wcsFmtID ));
#else
                    //Convert to Unicode.
                    MultiByteToWideChar(CP_ACP, 0, szGUID, -1, wcsFmtID, ARRAYSIZE( wcsFmtID )) ;
#endif

                    //Convert to GUID.
                    hr = CLSIDFromString((LPOLESTR)wcsFmtID, &(pClientInfo[dwClient].clsid));

                    if (FAILED(hr))
                    {
                        MiDebugMsg((hr, "CLSIDFromString(%s,) returned error ", szGUID));
                    }

                    //
                    //Create an instance of the COM object for this cleanup client
                    //
                    pClientInfo[dwClient].pVolumeCache = NULL;
                    hr = CoCreateInstance(pClientInfo[dwClient].clsid,
                                                    NULL,
                                                    CLSCTX_INPROC_SERVER,
                                                    IID_IEmptyVolumeCache,
                                                    (void **) &(pClientInfo[dwClient].pVolumeCache));

                    if (SUCCEEDED(hr))
                    {
                        WCHAR   wcsRoot[MAX_PATH];

                        MiDebugMsg((hr, "CleanupMgrInfo::initializeClients Created IID_IEmptyVolumeCache"));
                        //
                        //Set the flags to pass to the cleanup client
                        //
                        pClientInfo[dwClient].dwInitializeFlags = 0;
                        if (dwUIFlags & FLAG_SAGESET)
                            pClientInfo[dwClient].dwInitializeFlags |= EVCF_SETTINGSMODE;
                        if (bOutOfDiskSpace)
                            pClientInfo[dwClient].dwInitializeFlags |= EVCF_OUTOFDISKSPACE;

#ifdef UNICODE
                        StrCpyN( wcsRoot, szRoot, ARRAYSIZE( wcsRoot ));
#else
                        //
                        //Convert szRoot to UNICODE
                        //
                        MultiByteToWideChar(CP_ACP, 0, szRoot, -1, wcsRoot, ARRAYSIZE( wcsRoot ));
#endif

                        // Try to use version two of the interface if it is supported
                        IEmptyVolumeCache2 * pEVC2;
                        hr = pClientInfo[dwClient].pVolumeCache->QueryInterface( IID_IEmptyVolumeCache2, (void**)&pEVC2 );
                        if (SUCCEEDED(hr))
                        {
                            // version 2 exists so that we can have a mutli-local enabled data driven cleaner.  It
                            // allows the added Advanced Button to be set to a localized value.  It tells the
                            // object being called which key it is being called for so that one object can support
                            // multiple filters.
                            WCHAR   wcsFilterName[MAX_PATH];
                            MiDebugMsg((hr, "CleanupMgrInfo::initializeClients found V2 interface"));
#ifdef UNICODE
                            StrCpyN( wcsFilterName, szVolCacheClient, ARRAYSIZE( wcsFilterName ));
#else
                            MultiByteToWideChar(CP_ACP, 0, szVolCacheClient, -1, wcsFilterName, ARRAYSIZE( wcsFilterName )) ;
#endif

                            hr = pEVC2->InitializeEx(pClientInfo[dwClient].hClientKey,
                                                    (LPCWSTR)wcsRoot,
                                                    (LPCWSTR)wcsFilterName,
                                                    &((LPWSTR)pClientInfo[dwClient].wcsDisplayName),
                                                    &((LPWSTR)pClientInfo[dwClient].wcsDescription),
                                                    &((LPWSTR)pClientInfo[dwClient].wcsAdvancedButtonText),
                                                    &(pClientInfo[dwClient].dwInitializeFlags));
                            pEVC2->Release();
                        }
                        else
                        {
                            MiDebugMsg((hr, "CleanupMgrInfo::initializeClients using V1 interface"));
                            //
                            //Initialize the cleanup client
                            //
                            if ((pClientInfo[dwClient].wcsDescription = (LPWSTR)CoTaskMemAlloc(DESCRIPTION_LENGTH*sizeof(WCHAR))) == NULL)
                                return FALSE;

                            // We seem to have shipped this thing with a giant leak.  The object is supposted to set
                            // pClientInfo[dwClient].wcsDescription to NULL if the registry value should be used instead
                            // of the buffer.  However we just allocated a buffer for pClientInfo[dwClient].wcsDescription
                            // in the code above (this is the dumbass part).  All the filters then set this pointer to
                            // NULL and it's bye-bye buffer.  I can't simply not allocate this memory because some cleaners
                            // might rely on being able to use this memory and we shipped it that way.
                            LPWSTR wszDumbassLeakProtection = pClientInfo[dwClient].wcsDescription;
                            hr = pClientInfo[dwClient].pVolumeCache->Initialize(pClientInfo[dwClient].hClientKey,
                                                                               (LPCWSTR)wcsRoot,
                                                                               &((LPWSTR)pClientInfo[dwClient].wcsDisplayName),
                                                                               &((LPWSTR)pClientInfo[dwClient].wcsDescription),
                                                                               &(pClientInfo[dwClient].dwInitializeFlags));                                                                
                            if ( wszDumbassLeakProtection != pClientInfo[dwClient].wcsDescription )
                            {
                                // REVIEW: Use try...except around CoTaskMemFree in case some smart cleaner
                                // realized our mistake and deleted the memory for us?
                                MiDebugMsg((hr, "CleanupMgrInfo::initializeClients prevent mem leak hack"));
                                CoTaskMemFree( wszDumbassLeakProtection );
                            }

                            if ( S_OK == hr )
                            {
                                // To make it easier to make a cleaner we have a default implementation of IEmptyVolumeCache
                                // that works entirerly using registry data.  The problem is that display strings are strored
                                // in the registry.  This is invalid for NT because NT must be multi-local localizable and
                                // the only way to do that is to load all display strings from a resource.  As a hack, you
                                // can now implement IPropertyBag using an object with it's guid stored under the propertybag
                                // value in the registry.  We will cocreate this object and query for IPropertyBag.  If this
                                // works then we will attempt to read the localized strings from the property bag before we
                                // fall back on checking the registry.
                                TCHAR   szPropBagGUID[MAX_PATH];
                                HRESULT hrFoo;
                                IPropertyBag * ppb = NULL;
                                VARIANT var;

                                VariantInit( &var );
                                dwGUIDSize = sizeof(szPropBagGUID);
                                dwType = REG_SZ;
                                if (RegQueryValueEx(pClientInfo[dwClient].hClientKey, TEXT("PropertyBag"), NULL, &dwType, (LPBYTE)szPropBagGUID, &dwGUIDSize) == ERROR_SUCCESS)
                                {
                                    WCHAR   wcsFmtID[39];
                                    CLSID   clsid;

                                    MiDebugMsg((hr, "CleanupMgrInfo::initializeClients found PropBag key"));

#ifdef UNICODE
                                    StrCpyN( wcsFmtID, szPropBagGUID, ARRAYSIZE( wcsFmtID ));
#else
                                    MultiByteToWideChar(CP_ACP, 0, szPropBagGUID, -1, wcsFmtID, ARRAYSIZE( wcsFmtID )) ;
#endif

                                    //Convert to GUID.
                                    CLSIDFromString((LPOLESTR)wcsFmtID, &clsid);

                                    //
                                    //Create an instance of the COM object for this cleanup client
                                    //
                                    hrFoo = CoCreateInstance(clsid,
                                                          NULL,
                                                          CLSCTX_INPROC_SERVER,
                                                          IID_IPropertyBag,
                                                          (void **) &ppb);

                                    if ( FAILED(hrFoo) )
                                    {
                                        MiDebugMsg((hrFoo, "CleanupMgrInfo::initializeClients failed to create PropBag"));
                                    }
                                }

                                //
                                //If the client did not return the DisplayName via the Initialize
                                //Interface then we need to get it from the registry.
                                //
                                if ((pClientInfo[dwClient].wcsDisplayName) == NULL)
                                {
                                    LPTSTR  lpszDisplayName;

                                    if ( ppb )
                                    {
                                        WCHAR wszSrc[MAX_PATH];
                                        MiDebugMsg((hr, "CleanupMgrInfo::initializeClients checking PropBag for display"));

                                        SHTCharToUnicode(REGSTR_VAL_DISPLAY, wszSrc, MAX_PATH);

                                        // do propertybag stuff
                                        var.vt = VT_BSTR;
                                        var.bstrVal = NULL;
                                        hrFoo = ppb->Read( wszSrc, &var, NULL );
                                        if (SUCCEEDED(hrFoo))
                                        {
                                            if ( var.vt == VT_BSTR )
                                            {
                                                DWORD dwSize = (lstrlenW(var.bstrVal)+1)*sizeof(WCHAR);
                                                pClientInfo[dwClient].wcsDisplayName = (LPWSTR)CoTaskMemAlloc(dwSize);
                                                StrCpyW( pClientInfo[dwClient].wcsDisplayName, var.bstrVal );
                                            }
                                            VariantClear( &var );
                                        }
                                    }

                                    if ((pClientInfo[dwClient].wcsDisplayName) == NULL)
                                    {
                                        //
                                        //First check if their is a "display" value for the client's 
                                        //name that is displayed in the list box.  If not then use
                                        //the key name itself.
                                        //
                                        cb = 0;
                                        dwType = REG_SZ;
                                        RegQueryValueEx(pClientInfo[dwClient].hClientKey, REGSTR_VAL_DISPLAY, NULL, &dwType, (LPBYTE)NULL, &cb);
                                        if ((lpszDisplayName = (LPTSTR)LocalAlloc(LPTR, max(cb, (ULONG)(lstrlen(szVolCacheClient) + 1))* sizeof (TCHAR ))) != NULL)
                                        {
                                            if (RegQueryValueEx(pClientInfo[dwClient].hClientKey, REGSTR_VAL_DISPLAY, NULL, &dwType, (LPBYTE)lpszDisplayName, &cb) != ERROR_SUCCESS)
                                            {
                                                //
                                                //Count not find "display" value so use the key name instead
                                                //
                                                StrCpy(lpszDisplayName, szVolCacheClient);
                                            }

    #ifdef UNICODE
                                            cw = (lstrlen( lpszDisplayName ) + 1) * sizeof( WCHAR);
    #else
                                            //
                                            //Convert this value to UNICODE
                                            //
                                            cw = (MultiByteToWideChar(CP_ACP, 0, lpszDisplayName, -1, NULL, 0) * sizeof(WCHAR));
    #endif
                                            if ((pClientInfo[dwClient].wcsDisplayName = (LPWSTR)CoTaskMemAlloc(cw)) != NULL)
                                            {
    #ifdef UNICODE
                                                StrCpy( pClientInfo[dwClient].wcsDisplayName, lpszDisplayName );
    #else
                                                MultiByteToWideChar(CP_ACP, 0, lpszDisplayName, -1, (pClientInfo[dwClient].wcsDisplayName), cw);
    #endif
                                            }
                                            LocalFree(lpszDisplayName);
                                        }
                                    }
                                }

                                //
                                //If the client did not return the Description via the Initialize
                                //Interface then we need to get it from the registry.
                                //
                                if ((pClientInfo[dwClient].wcsDescription) == NULL)
                                {
                                    LPTSTR  lpszDescription;


                                    if ( ppb )
                                    {
                                        WCHAR wszSrc[MAX_PATH];
                                        MiDebugMsg((hr, "CleanupMgrInfo::initializeClients checking PropBag for description"));

                                        SHTCharToUnicode(REGSTR_VAL_DESCRIPTION, wszSrc, MAX_PATH);

                                        // do propertybag stuff
                                        var.vt = VT_BSTR;
                                        var.bstrVal = NULL;
                                        hrFoo = ppb->Read( wszSrc, &var, NULL );
                                        if (SUCCEEDED(hrFoo))
                                        {
                                            if ( var.vt == VT_BSTR )
                                            {
                                                DWORD dwSize = (lstrlenW(var.bstrVal)+1)*sizeof(WCHAR);
                                                pClientInfo[dwClient].wcsDescription = (LPWSTR)CoTaskMemAlloc(dwSize);
                                                StrCpyW( pClientInfo[dwClient].wcsDescription, var.bstrVal );
                                            }
                                            VariantClear( &var );
                                        }
                                    }

                                    if ((pClientInfo[dwClient].wcsDescription) == NULL)
                                    {
                                        //
                                        //Check if their is a "description" value for the client 
                                        //
                                        cb = 0;
                                        dwType = REG_SZ;
                                        RegQueryValueEx(pClientInfo[dwClient].hClientKey, REGSTR_VAL_DESCRIPTION, NULL, &dwType, (LPBYTE)NULL, &cb);
                                        if ((lpszDescription = (LPTSTR)LocalAlloc(LPTR, (cb + 1 ) * sizeof( TCHAR ))) != NULL)
                                        {
                                            if (RegQueryValueEx(pClientInfo[dwClient].hClientKey, REGSTR_VAL_DESCRIPTION, NULL, &dwType, (LPBYTE)lpszDescription, &cb) == ERROR_SUCCESS)
                                            {
#ifdef UNICODE
                                                cw = ( lstrlen( lpszDescription ) + 1 ) * sizeof( WCHAR );
#else
                                                //
                                                //Convert this value to UNICODE
                                                //
                                                cw = (MultiByteToWideChar(CP_ACP, 0, lpszDescription, -1, NULL, 0) * sizeof(WCHAR));
#endif
                                                if ((pClientInfo[dwClient].wcsDescription = (LPWSTR)CoTaskMemAlloc(cw)) != NULL)
                                                {
#ifdef UNICODE                                          
                                                    StrCpy( pClientInfo[dwClient].wcsDescription, lpszDescription );
#else
                                                    MultiByteToWideChar(CP_ACP, 0, lpszDescription, -1, (pClientInfo[dwClient].wcsDescription), cw);
#endif
                                                }
                                            }

                                            LocalFree(lpszDescription);
                                        }
                                    }
                                }

                                //
                                //Set the Advanced Button text
                                //
                                pClientInfo[dwClient].wcsAdvancedButtonText = NULL;

                                if (pClientInfo[dwClient].dwInitializeFlags & EVCF_HASSETTINGS)
                                {
                                    if ( ppb )
                                    {
                                        WCHAR wszSrc[MAX_PATH];
                                        MiDebugMsg((hr, "CleanupMgrInfo::initializeClients checking PropBag for button text"));

                                        SHTCharToUnicode(REGSTR_VAL_ADVANCEDBUTTONTEXT, wszSrc, MAX_PATH);

                                        // do propertybag stuff
                                        var.vt = VT_BSTR;
                                        var.bstrVal = NULL;
                                        hrFoo = ppb->Read( wszSrc, &var, NULL );
                                        if (SUCCEEDED(hrFoo))
                                        {
                                            if ( var.vt == VT_BSTR )
                                            {
                                                DWORD dwSize = (lstrlenW(var.bstrVal)+1)*sizeof(WCHAR);
                                                pClientInfo[dwClient].wcsAdvancedButtonText = (LPWSTR)CoTaskMemAlloc(dwSize);
                                                StrCpyW( pClientInfo[dwClient].wcsAdvancedButtonText, var.bstrVal );
                                            }
                                            VariantClear( &var );
                                        }
                                    }
                                    if ( pClientInfo[dwClient].wcsAdvancedButtonText == NULL )
                                    {
                                        LPTSTR  lpszAdvancedButtonText;
                                        TCHAR   szDetails[BUTTONTEXT_LENGTH];

                                        LoadString(g_hInstance, IDS_DETAILS, szDetails, sizeof(szDetails));

                                        cb = 0;
                                        dwType = REG_SZ;
                                        RegQueryValueEx(pClientInfo[dwClient].hClientKey, REGSTR_VAL_ADVANCEDBUTTONTEXT, NULL, &dwType, (LPBYTE)NULL, &cb);
                                        if ((lpszAdvancedButtonText = (LPTSTR)LocalAlloc(LPTR, max(cb, (UINT) (lstrlen(szDetails) + 1)*sizeof(TCHAR)))) != NULL)
                                        {
                                            if (RegQueryValueEx(pClientInfo[dwClient].hClientKey, REGSTR_VAL_ADVANCEDBUTTONTEXT, NULL, &dwType, (LPBYTE)lpszAdvancedButtonText, &cb) != ERROR_SUCCESS)
                                            StrCpy(lpszAdvancedButtonText, szDetails);

#ifdef UNICODE
                                            cw = (lstrlen( lpszAdvancedButtonText ) + 1) * sizeof( WCHAR );
#else
                                            //
                                            //Convert this value to UNICODE
                                            //
                                            cw = (MultiByteToWideChar(CP_ACP, 0, lpszAdvancedButtonText, -1, NULL, 0) * sizeof(WCHAR));
#endif
                                            if ((pClientInfo[dwClient].wcsAdvancedButtonText = (LPWSTR)CoTaskMemAlloc(cw)) != NULL)
                                            {
#ifdef UNICODE
                                                StrCpy( pClientInfo[dwClient].wcsAdvancedButtonText, lpszAdvancedButtonText );
#else
                                                MultiByteToWideChar(CP_ACP, 0, lpszAdvancedButtonText, -1, (pClientInfo[dwClient].wcsAdvancedButtonText), cw);
#endif
                                            }

                                            LocalFree(lpszAdvancedButtonText);
                                        }
                                    }
                                }

                                if (ppb)
                                {
                                    ppb->Release();
                                }
                            }
                        }

                        // Now we're back to stuff that both version 1 and version 2 require
                        if (SUCCEEDED(hr))
                        {
                            if (S_OK == hr)
                            {
                                //
                                //Default to showing this client in the UI
                                //
                                pClientInfo[dwClient].bShow = TRUE;
                            
                                //
                                //Get the "priority" from the registry
                                //
                                cb = sizeof(pClientInfo[dwClient].dwPriority);
                                dwType = REG_DWORD;
                                if (RegQueryValueEx(pClientInfo[dwClient].hClientKey, REGSTR_VAL_PRIORITY, NULL, &dwType, (LPBYTE)&(pClientInfo[dwClient].dwPriority), &cb) != ERROR_SUCCESS)
                                    pClientInfo[dwClient].dwPriority = DEFAULT_PRIORITY;
                                
                                //
                                //Flags
                                //
                                if (dwUIFlags & FLAG_SAGERUN || dwUIFlags & FLAG_SAGESET)
                                    wsprintf(szProfile, TEXT("%s%04d"), SZ_STATE, ulSAGEProfile);
                                else
                                    lstrcpy(szProfile, SZ_STATE);
                                    
                                dwState = 0;
                                cb = sizeof(dwState);
                                dwType = REG_DWORD;
                                if (RegQueryValueEx(pClientInfo[dwClient].hClientKey, szProfile, NULL,
                                    &dwType, (LPBYTE)&dwState, &cb) == ERROR_SUCCESS)
                                {
                                    if (dwUIFlags & FLAG_SAGERUN || dwUIFlags & FLAG_SAGESET)
                                    {
                                        pClientInfo[dwClient].bSelected = (dwState & STATE_SAGE_SELECTED);
                                    }
                                    else
                                    {
                                        pClientInfo[dwClient].bSelected = (dwState & STATE_SELECTED);
                                    }
                                }
                                else
                                {
                                    //
                                    //No registry settings for this profile so use the cleanup clients
                                    //default settings.
                                    //
                                    if (dwUIFlags & FLAG_SAGERUN || dwUIFlags & FLAG_SAGESET)
                                    {
                                        pClientInfo[dwClient].bSelected = (pClientInfo[dwClient].dwInitializeFlags & EVCF_ENABLEBYDEFAULT_AUTO) ? TRUE : FALSE;
                                    }
                                    else
                                    {
                                        pClientInfo[dwClient].bSelected = (pClientInfo[dwClient].dwInitializeFlags & EVCF_ENABLEBYDEFAULT) ? TRUE : FALSE;
                                    }
                                }
                                
                                //
                                //Get the icon of the cleanup client
                                //

                                // first test to see if it is overridden...
                                TCHAR szIconPath[MAX_PATH];
                                cb = sizeof( szIconPath );
                                BOOL fOverridden = FALSE;
                                
                                if ( RegQueryValueEx(pClientInfo[dwClient].hClientKey, TEXT("IconPath"), NULL,
                                    &dwType, (LPBYTE)szIconPath, &cb) == ERROR_SUCCESS )
                                {
                                    fOverridden = TRUE;
                                }
                                else
                                {
                                    lstrcpy( szIconPath, szGUID );
                                }
                                
                                pClientInfo[dwClient].hIcon = GetClientIcon(szIconPath, fOverridden);

                                bCleanup = FALSE;
                            }
                            else
                            {
                                //
                                //This should be S_FALSE.  This means that the client has nothing to 
                                //cleanup now so we don't even need to show it in the list.
                                //Therefor we will just call its Release() function and close it's
                                //registry key.
                                //

                                // drop through and let it cleanup below...
                            }
                        }
                        else
                        {
                            MiDebugMsg((hr, "Client %d Initialize() retuned error ", dwClient));
                        }                                                                      
                    }
                    else
                    {
                        MiDebugMsg((hr, "Client %d %s returned error ", dwClient, szGUID));
                    }
                }
#ifdef DEBUG
                else
                {
                    MessageBox(NULL, szVolCacheClient, TEXT("ERROR Opening GUID key"), MB_OK);
                }                
#endif
            }
#ifdef DEBUG
            else
            {
                MessageBox(NULL, szVolCacheClient, TEXT("ERROR Opening the client key"), MB_OK);
            }
#endif

            if ( bCleanup )
            {
                deactivateSingleClient(&(pClientInfo[dwClient]));
                ZeroMemory( &(pClientInfo[dwClient]), sizeof( CLIENTINFO ));
            }
            else
            {
                dwClient ++;
            }
            iSubKey++;        
        }
        iNumVolumeCacheClients = dwClient;
    }
#ifdef DEBUG
    else
    {
        MessageBox(NULL, TEXT("ERROR Opening up Volume Cache key"), TEXT("CLEANMGR DEBUG"), MB_OK);
    }
#endif

    RegCloseKey(hKeyVolCache);
    return bRet;
}

/*
**------------------------------------------------------------------------------
** CleanupMgrInfo::deactivateClients
**
** Purpose:    Initializes all of the Volume Cache Clients
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
void 
CleanupMgrInfo::deactivateClients(void)
{
    int     i;
    
    for (i=0; i<iNumVolumeCacheClients; i++)
    {
        deactivateSingleClient(&(pClientInfo[i]));
    }

    //
    //Free the pClientInfo array
    //
    if (pClientInfo)
    {
        MiDebugMsg((0, "LocalFree() on ClientInfo structure"));
        LocalFree( pClientInfo);
    }
}


/*
**------------------------------------------------------------------------------
** CleanupMgrInfo::deactivateSingleClient
**
** Purpose:    Deactivate's the given client and closes its registry key
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
void 
CleanupMgrInfo::deactivateSingleClient(PCLIENTINFO pSingleClientInfo)
{
    DWORD   dwDeactivateFlags;
    TCHAR   szProfile[64];
    
    if (pSingleClientInfo->pVolumeCache != NULL)
    {
        //
        //Call the clients Deactivate function
        //
        dwDeactivateFlags = 0;
        pSingleClientInfo->pVolumeCache->Deactivate(&dwDeactivateFlags);

        //
        //Release the client
        //
        pSingleClientInfo->pVolumeCache->Release();
        pSingleClientInfo->pVolumeCache = NULL;
    }
            
    if (pSingleClientInfo->hClientKey != 0)
    {
        DWORD   dwState, cb, dwType, dwSelectedFlag;

        if (dwUIFlags & FLAG_SAVE_STATE)
        {
            //
            //Save the state flags
            //
            if (dwUIFlags & FLAG_SAGESET)
            {
                dwSelectedFlag = STATE_SAGE_SELECTED;
                wsprintf(szProfile, TEXT("%s%04d"), SZ_STATE, ulSAGEProfile);
            }
            else
            {
                dwSelectedFlag = STATE_SELECTED;
                lstrcpy(szProfile, SZ_STATE);
            }

            dwState = 0;
            cb = sizeof(dwState);
            dwType = REG_DWORD;
            RegQueryValueEx(pSingleClientInfo->hClientKey, szProfile, NULL,
                &dwType, (LPBYTE)&dwState, &cb);

            if (pSingleClientInfo->bSelected)
                dwState |= dwSelectedFlag;
            else
                dwState &= ~dwSelectedFlag;

            RegSetValueEx(pSingleClientInfo->hClientKey, szProfile, 0, REG_DWORD,
                (LPBYTE)&dwState, sizeof(dwState));
        }
    
        //
        //Close all of the registry keys
        //
        RegCloseKey(pSingleClientInfo->hClientKey);

        //
        //Should we remove this entry from the registry?
        //
        if (dwDeactivateFlags & EVCF_REMOVEFROMLIST && pSingleClientInfo->bSelected)
        {
            HKEY    hKeyVolCache;
            
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_VOLUMECACHE, 0, KEY_ALL_ACCESS, &hKeyVolCache) == ERROR_SUCCESS)
            {
                SHDeleteKey(hKeyVolCache, pSingleClientInfo->szRegKeyName);
                RegCloseKey(hKeyVolCache);
            }
        }            
            
    }

    //
    //Free the DisplayName and Description memory
    //
    if (pSingleClientInfo->wcsDisplayName)
        CoTaskMemFree(pSingleClientInfo->wcsDisplayName);
        
    if (pSingleClientInfo->wcsDescription)
        CoTaskMemFree(pSingleClientInfo->wcsDescription);
}

/*
**------------------------------------------------------------------------------
** CleanupMgrInfo::getSpaceUsedByClients
**
** Purpose:    Calls the IEmptyVolumeCache->GetSpaceUsed interface for each client
**             to determine the total amount of cache space.  This function is
**             called on a secondary thread because it can take quite a long time.
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
BOOL
CleanupMgrInfo::getSpaceUsedByClients(void)
{
    int         i;
    HRESULT     hr;
    BOOL        bRet = TRUE;
    TCHAR       szDisplayName[256];
        
    cbEstCleanupSpace.QuadPart = 0;
    bAbortScan = FALSE;

    hAbortScanEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    // BUGBUG: Make sure the window created in the ScanAbortThread thread is visible before
    // the hAbortScanEvent event is signaled
    hAbortScanThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ScanAbortThread,
        (LPVOID)this, 0, &dwAbortScanThreadID);

    //
    //Wait until the Abort Scan window is created
    //
    WaitForSingleObject(hAbortScanEvent, INFINITE);

    CloseHandle(hAbortScanEvent);

    volumeCacheCallBack->SetCleanupMgrInfo((PVOID)this);

    for (i=0; i<iNumVolumeCacheClients; i++)
    {
        //
        //Update the progress UI
        //
        szDisplayName[0] = '\0';
        
#ifdef UNICODE
        StrCpyN( szDisplayName, pClientInfo[i].wcsDisplayName, ARRAYSIZE( szDisplayName ));
#else
        WideCharToMultiByte(CP_ACP, 0, pClientInfo[i].wcsDisplayName, -1, szDisplayName, ARRAYSIZE(szDisplayName), NULL, NULL);
#endif
        
        PostMessage(hAbortScanWnd, WMAPP_UPDATEPROGRESS, (WPARAM)i, (LPARAM)szDisplayName);

        //
        //Query the client for the ammount of cache disk space that it could
        //possible free.
        //
        if (pClientInfo[i].pVolumeCache != NULL)
        {
            volumeCacheCallBack->SetCurrentClient((PVOID)&(pClientInfo[i]));
            hr = pClientInfo[i].pVolumeCache->GetSpaceUsed(&(pClientInfo[i].dwUsedSpace.QuadPart), 
                                                                    pIEmptyVolumeCacheCallBack);      
            
            if (FAILED(hr))
            {
                dwReturnCode = RETURN_CLEANER_FAILED;
                MiDebugMsg((hr, "Client %d GetSpaceUsed failed with error ", i));
            }
            
            MiDebugMsg((0, "Client %d has %d disk space it can free", i,
                pClientInfo[i].dwUsedSpace.QuadPart));
        }

        //
        //See if this cleaner wants to be hidden if it has no space to free
        //
        if ((pClientInfo[i].dwUsedSpace.QuadPart == 0) &&
            (pClientInfo[i].dwInitializeFlags & EVCF_DONTSHOWIFZERO))
        {
            MiDebugMsg((0, "Not showing client %d because it has no space to free", i));
            pClientInfo[i].bShow = FALSE;
        }

        cbEstCleanupSpace.QuadPart += pClientInfo[i].dwUsedSpace.QuadPart;

        //
        //Did the user abort?
        //
        if (bAbortScan == TRUE)
        {
            dwReturnCode = RETURN_USER_CANCELED_SCAN;
            bRet = FALSE;
            break;
        }
    }

    // the dismissal of the progress dialog is now delayed until the propsheet comes up..

    return bRet;
}

/*
**------------------------------------------------------------------------------
** CleanupMgrInfo::calculateSpaceToPurge
**
** Purpose:    Calculates the amount of space that is going to be purged
**             by adding up all of the selected clients.  It also calculates
**             the progress bar divisor number.  This is needed because a
**             progress bar has a MAX of 0xFFFF.
**
** Mod Log:    Created by Jason Cobb (6/97)
**------------------------------------------------------------------------------
*/
void
CleanupMgrInfo::calculateSpaceToPurge(void)
{
    int i;
    
    cbSpaceToPurge.QuadPart = 0;

    for (i=0; i<iNumVolumeCacheClients; i++)
    {
        //
        //If this client is not selected or we are not showing it then don't purge it
        //
        if (pClientInfo[i].bShow == FALSE || pClientInfo[i].bSelected == FALSE)
            continue;
    
        cbSpaceToPurge.QuadPart += pClientInfo[i].dwUsedSpace.QuadPart;
    }

    cbProgressDivider.QuadPart = (cbSpaceToPurge.QuadPart / PROGRESS_DIVISOR) + 1;
}

/*
**------------------------------------------------------------------------------
** CleanupMgrInfo::purgeClients
**
** Purpose:    Calls the IEmptyVolumeCache->Purge interface for each client
**             to have the client cleaner object start removeing their files
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
BOOL
CleanupMgrInfo::purgeClients(void)
{
    int         i;
    HRESULT     hr;
    BOOL        bRet = TRUE;
    TCHAR       szDisplayName[256];
        
    cbTotalPurgedSoFar.QuadPart = 0;
    bAbortPurge = FALSE;

    //
    //Calculate the amount of space that will be purged.
    //
    calculateSpaceToPurge();
    MiDebugMsg((0, "Total number of bytes to delete is %d", cbSpaceToPurge.LowPart));

    hAbortPurgeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    // BUGBUG:  Make sure the window created in PurgeAbortThread is visible before
    // the hAbortPurgeEvent is signaled
    hAbortPurgeThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PurgeAbortThread,
        (LPVOID)this, 0, &dwAbortPurgeThreadID);

    //
    //Wait until the Abort Purge window is created
    //
    WaitForSingleObject(hAbortPurgeEvent, INFINITE);

    CloseHandle(hAbortPurgeEvent);

    volumeCacheCallBack->SetCleanupMgrInfo((PVOID)this);

    for (i=0; i<iNumVolumeCacheClients; i++)
    {
        //
        //If this client is not selected or we are not showing it then don't purge it
        //
        if (pClientInfo[i].bShow == FALSE || pClientInfo[i].bSelected == FALSE)
            continue;
    
#ifdef UNICODE
        StrCpyN( szDisplayName, pClientInfo[i].wcsDisplayName, ARRAYSIZE( szDisplayName ));
#else
        //
        //Convert UNICODE display name to ANSI and then add it to the list
        //
        WideCharToMultiByte(CP_ACP, 0, pClientInfo[i].wcsDisplayName, -1, szDisplayName, sizeof(szDisplayName), NULL, NULL);
#endif

        PostMessage(hAbortPurgeWnd, WMAPP_UPDATESTATUS, 0, (LPARAM)szDisplayName);

        cbCurrentClientPurgedSoFar.QuadPart = 0;

        //
        //Query the client for the ammount of cache disk space that it could
        //possible free.
        //
        if (pClientInfo[i].pVolumeCache != NULL)
        {
            volumeCacheCallBack->SetCurrentClient((PVOID)&(pClientInfo[i]));

            hr = pClientInfo[i].pVolumeCache->Purge(pClientInfo[i].dwUsedSpace.QuadPart, pIEmptyVolumeCacheCallBack);
            
            if (FAILED(hr))
            {
                dwReturnCode = RETURN_CLEANER_FAILED;
                MiDebugMsg((hr, "Client %d Purge failed with error ", i));
            }
        }

        cbTotalPurgedSoFar.QuadPart += pClientInfo[i].dwUsedSpace.QuadPart;
        cbCurrentClientPurgedSoFar.QuadPart = 0;

        //
        //Update the progress bar
        //
        PostMessage(hAbortPurgeWnd, WMAPP_UPDATEPROGRESS, 0, 0);

        //
        //Did the user abort?
        //
        if (bAbortPurge == TRUE)
        {
            dwReturnCode = RETURN_USER_CANCELED_PURGE;
            bRet = FALSE;
            break;
        }

        Sleep(1000);
    }

    if (!bAbortPurge)
    {
        bAbortPurge = TRUE;

        //
        //Wait for Purge thread to finish
        //  
        WaitForSingleObject(hAbortPurgeThread, INFINITE);

        bAbortPurge = FALSE;
    }

    return bRet;
}

/*
**------------------------------------------------------------------------------
** GetClientIcon
**
** Purpose:    Gets the Icon for this client.  
**             The icon will be inferred using the standard OLE mechanism
**             under HKCR\CLSID\{clsid}\DefaultIcon (with the default value
**             for this being the <Module Path>, <icon index>).
**             If no icon is specified the standard windows icon will be used.
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
HICON
CleanupMgrInfo::GetClientIcon(
    LPTSTR  lpGUID,
    BOOL    fIconPath
    )
{
    HKEY    hk;
    HICON   hIconLarge, hIconSmall;
    HICON   hIcon = NULL;
    TCHAR   szIconKey[MAX_PATH];
    TCHAR   szDefaultIcon[MAX_PATH];
    DWORD   dwType, cbBytes;
    TCHAR   szIconExeName[MAX_PATH];
    int     i, iIconIndex;

    if ( fIconPath )
    {
        StrCpy( szDefaultIcon, lpGUID );
    }
    if ( !fIconPath )
    {
        wsprintf(szIconKey, SZ_DEFAULTICONPATH, lpGUID);
        if (RegOpenKey(HKEY_CLASSES_ROOT, szIconKey, &hk) == ERROR_SUCCESS)
        {
            dwType = REG_SZ;
            cbBytes = sizeof(szDefaultIcon);
            if (RegQueryValueEx(hk, NULL, NULL, &dwType, (LPBYTE)szDefaultIcon, &cbBytes) == ERROR_SUCCESS)
            {
                fIconPath = TRUE;
            }
            RegCloseKey(hk);
        }
    }

    if (fIconPath)
    {
        //
        //Parse out the exe where the icon lives
        //
        for(i=0; i<lstrlen(szDefaultIcon); i++)
        {
            if (szDefaultIcon[i] == ',')
                break;

            szIconExeName[i] = szDefaultIcon[i];
        }

        szIconExeName[i] = '\0';

        //
        //Parse out the icon index
        //
        i++;
        iIconIndex = StrToInt(&(szDefaultIcon[i]));

        ExtractIconEx(szIconExeName, iIconIndex, (HICON FAR *)&hIconLarge, (HICON FAR *)&hIconSmall, 1);
        if (hIconSmall)
            hIcon = hIconSmall;
        else
            hIcon = hIconLarge;
    }
    
    if (hIcon == NULL)
    {
        if ((hIcon = LoadIcon(CleanupMgrInfo::hInstance, MAKEINTRESOURCE(ICON_GENERIC))) == NULL)
        {
            MiDebugMsg((0, "LoadIcon failed with error %d", GetLastError()));
        }   
    }
    
    return hIcon;
}

INT_PTR CALLBACK
ScanAbortDlgProc(
    HWND hDlg,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CleanupMgrInfo *pcmi;

    switch(Message)
    {
        case WM_INITDIALOG:
            SetWindowLongPtr (hDlg, DWLP_USER, 0L);

            //   
            //Get the CleanupMgrInfo
            //
            pcmi = (CleanupMgrInfo *)lParam;    
            if (pcmi == NULL)
            {   
                //Error - passed in invalid CleanupMgrInfo info
                return FALSE;
            }       

            //
            //Save pointer to CleanupMgrInfo object
            //
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);

            TCHAR * psz;
            psz = SHFormatMessage( MSG_SCAN_ABORT_TEXT, pcmi->szVolName, pcmi->szRoot[0] );
            SetDlgItemText (hDlg, IDC_ABORT_TEXT, psz);
            LocalFree(psz);

            //
            //Set the limits on the progress bar
            //
            SendDlgItemMessage(hDlg, IDC_ABORT_SCAN_PROGRESS, PBM_SETRANGE,
                0, MAKELPARAM(0, pcmi->iNumVolumeCacheClients));

            PulseEvent(pcmi->hAbortScanEvent);
            break;

        case WMAPP_UPDATEPROGRESS:
            if (lParam != NULL)
                SetDlgItemText(hDlg, IDC_SCAN_STATUS_TEXT, (LPTSTR)lParam);
            else
                SetDlgItemText(hDlg, IDC_SCAN_STATUS_TEXT, TEXT(""));
                
            SendDlgItemMessage(hDlg, IDC_ABORT_SCAN_PROGRESS, PBM_SETPOS,
                (WPARAM)wParam, 0);
            break;

        case WM_CLOSE:
        case WM_COMMAND:
            pcmi = (CleanupMgrInfo *)GetWindowLongPtr (hDlg, DWLP_USER);
            if (pcmi != NULL)
                pcmi->bAbortScan = TRUE;
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

BOOL
PASCAL
MessagePump(
    HWND hDialogWnd
    )
{
    MSG Msg;
    BOOL fGotMessage;

    if ((fGotMessage = PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))) 
    {
        if (!IsDialogMessage(hDialogWnd, &Msg)) 
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
    }

    return fGotMessage;
}


void
ScanAbortThread(
    CleanupMgrInfo *pcmi
    )
{
    if ((pcmi->hAbortScanWnd = CreateDialogParam(g_hInstance, MAKEINTRESOURCE(IDD_SCAN_ABORT),
        NULL, ScanAbortDlgProc, (LPARAM)pcmi)) == NULL)
    {
        return;
    }   

    ShowWindow(pcmi->hAbortScanWnd, SW_SHOW);

    //
    //Keep spinning till the Scan is stopped
    //
    while (!(pcmi->bAbortScan))
    {
        MessagePump(pcmi->hAbortScanWnd);
    }

    //
    //Destroy the Abort Scan dialog
    //
    if (pcmi->hAbortScanWnd != NULL)
    {
        DestroyWindow(pcmi->hAbortScanWnd);
        pcmi->hAbortScanWnd = NULL;
    }
}

INT_PTR CALLBACK
PurgeAbortDlgProc(
    HWND hDlg,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CleanupMgrInfo  *pcmi;
    DWORD           dwCurrent;

    switch(Message)
    {
        case WM_INITDIALOG:
            SetWindowLongPtr (hDlg, DWLP_USER, 0L);

            //   
            //Get the CleanupMgrInfo
            //
            pcmi = (CleanupMgrInfo *)lParam;    
            if (pcmi == NULL)
            {   
                //Error - passed in invalid CleanupMgrInfo info
                return FALSE;
            }       

            //
            //Save pointer to CleanupMgrInfo object
            //
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);

            TCHAR * psz;
            psz = SHFormatMessage( MSG_PURGE_ABORT_TEXT, pcmi->szVolName, pcmi->szRoot[0]);
            SetDlgItemText (hDlg, IDC_PURGE_TEXT, psz);
            LocalFree(psz);

            //
            //Set the limits on the progress bar
            //
            if (pcmi->cbProgressDivider.QuadPart != 0)
                dwCurrent = (DWORD)(pcmi->cbSpaceToPurge.QuadPart / pcmi->cbProgressDivider.QuadPart);
            else
                dwCurrent = (DWORD)(pcmi->cbSpaceToPurge.QuadPart);

            SendDlgItemMessage(hDlg, IDC_ABORT_PURGE_PROGRESS, PBM_SETRANGE,
                0, MAKELPARAM(0, dwCurrent));

            PulseEvent(pcmi->hAbortPurgeEvent);
            break;

        case WMAPP_UPDATESTATUS:
            if (lParam != NULL)
                SetDlgItemText(hDlg, IDC_PURGE_STATUS_TEXT, (LPTSTR)lParam);
            else
                SetDlgItemText(hDlg, IDC_PURGE_STATUS_TEXT, TEXT(""));
            break;

        case WMAPP_UPDATEPROGRESS:
            pcmi = (CleanupMgrInfo *)GetWindowLongPtr (hDlg, DWLP_USER);
            if (pcmi != NULL)
            {
                //
                //BUGBUG:
                //
                if (pcmi->cbProgressDivider.QuadPart != 0)
                    dwCurrent = (DWORD)((pcmi->cbTotalPurgedSoFar.QuadPart +
                        pcmi->cbCurrentClientPurgedSoFar.QuadPart) /
                        pcmi->cbProgressDivider.QuadPart);
                else
                    dwCurrent = (DWORD)(pcmi->cbTotalPurgedSoFar.QuadPart +
                        pcmi->cbCurrentClientPurgedSoFar.QuadPart);

                SendDlgItemMessage(hDlg, IDC_ABORT_PURGE_PROGRESS, PBM_SETPOS,
                    (WPARAM)dwCurrent, 0);
            }
            break;

        case WM_CLOSE:
        case WM_COMMAND:
            pcmi = (CleanupMgrInfo *)GetWindowLongPtr (hDlg, DWLP_USER);
            if (pcmi != NULL)
                pcmi->bAbortPurge = TRUE;
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

void
PurgeAbortThread(
    CleanupMgrInfo *pcmi
    )
{
    if ((pcmi->hAbortPurgeWnd = CreateDialogParam(g_hInstance, MAKEINTRESOURCE(IDD_PURGE_ABORT),
        NULL, PurgeAbortDlgProc, (LPARAM)pcmi)) == NULL)
    {
        return;
    }   

    ShowWindow(pcmi->hAbortPurgeWnd, SW_SHOW);

    //
    //Keep spinning till the Purge is stopped
    //
    while (!(pcmi->bAbortPurge))
    {
        MessagePump(pcmi->hAbortPurgeWnd);
    }

    //
    //Destroy the Abort Purge dialog
    //
    if (pcmi->hAbortPurgeWnd != NULL)
    {
        DestroyWindow(pcmi->hAbortPurgeWnd);
        pcmi->hAbortPurgeWnd = NULL;
    }
}
/*
**------------------------------------------------------------------------------
** End of File
**------------------------------------------------------------------------------
*/
