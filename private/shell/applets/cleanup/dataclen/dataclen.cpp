#include "common.h"
#include "dataguid.h"
#include "compguid.h"

#include <emptyvc.h>

#include "dataclen.h"
#include "compclen.h"

#include <regstr.h>
#include <olectl.h>
#include <tlhelp32.h>

#define DECL_CRTFREE
#include <crtfree.h>
#ifndef RESOURCE_H
    #include "resource.h"
#endif

#ifdef WINNT
#include <winsvc.h>
#include <shlwapi.h>
#include <shlwapip.h>
#endif

#include <advpub.h>

/*
**------------------------------------------------------------------------------
** Global variables
**------------------------------------------------------------------------------
*/
HINSTANCE   g_hDllModule      = NULL;  // Handle to this DLL itself.
UINT        g_cDllObjects     = 0;     // Count number of existing objects 
UINT        g_cDllLocks       = 0;     // Count number of DLL locks

#define SZSIZEINBYTES(x)    (lstrlen(x)*sizeof(TCHAR)+1)

#define KEEP_FAILURE(hrSum, hrLast) if (FAILED(hrLast)) hrSum = hrLast;

BOOL 
DllInit(
    HINSTANCE hInstance
    );

void 
DllCleanup (
    void
    );


BOOL DllInit (
    HINSTANCE hInstance
    )
{
    //
    //Shouldn't happen but just in case         
    //
    if (hInstance == NULL)
        return FALSE;

    //
    //Initialize global variables
    //
    g_hDllModule   = hInstance;
    g_cDllObjects  = 0;
    g_cDllLocks    = 0;

    //
    //Initialize common controls
    //Note:    it is very likely that the commctrl.dll is
    //         already loaded and properly initialized.
    //
    //InitCommonControls();

    return TRUE;
}

void 
DllCleanup(void)
{
}

// CCover likes the entry point to be DllMain

#ifdef CCOVER
#define LibMain DllMain
#endif
                    
extern "C" int APIENTRY 
LibMain( 
   HINSTANCE   hInstance, 
   DWORD       fdwReason,
   LPVOID      lpvReserved
   )
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            // Initialize DLL
            return DllInit(hInstance);

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            // Cleanup DLL
            DllCleanup();
            break;
    }

    return TRUE;
}


/*
**------------------------------------------------------------------------------
** DllGetClassObject
**
** Purpose:    Provides an IClassFactory for a given CLSID that this DLL is
**             registered to support.  This DLL is placed under the CLSID
**             in the registration database as the InProcServer.
** Parameters:
**  clsID   -  REFCLSID that identifies the class factory desired.
**             Since this parameter is passed this DLL can handle
**             any number of objects simply by returning different
**             different class factories here for different CLSIDs.
**  riid    -  REFIID specifying the interface the caller wants
**             on the class object, usually IID_ClassFactory.
**  ppvOut  -  pointer in which to return the interface pointer.
** Return:     NOERROR on success, otherwise an error code
** Notes;
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
STDAPI 
DllGetClassObject(
    REFCLSID rclsid,
    REFIID   riid, 
    LPVOID * ppvOut
    )
{
    DWORD dw;
    HRESULT hr;


    *ppvOut = NULL;

    //
    // Is the request for one of our cleaner objects?
    //
    if (IsEqualCLSID(rclsid, CLSID_DataDrivenCleaner))
    {
        dw = ID_SYSTEMDATACLEANER;
    }
    else if (IsEqualCLSID(rclsid, CLSID_ContentIndexerCleaner))
    {
        dw = ID_CONTENTINDEXCLEANER;
    }
    else if ( IsEqualCLSID(rclsid, CLSID_CompCleaner) )
    {
        dw = ID_COMPCLEANER;
    }
    //
    // Or is the request for one of our supporting Property Bags?
    //
    else if ( IsEqualCLSID(rclsid, CLSID_OldFilesInRootPropBag) )
    {
        dw = ID_OLDFILESINROOTPROPBAG;
    }
    else if ( IsEqualCLSID(rclsid, CLSID_TempFilesPropBag) )
    {
        dw = ID_TEMPFILESPROPBAG;
    }
    else if ( IsEqualCLSID(rclsid, CLSID_SetupFilesPropBag) )
    {
        dw = ID_SETUPFILESPROPBAG;
    }
    else if ( IsEqualCLSID(rclsid, CLSID_UninstalledFilesPropBag) )
    {
        dw = ID_UNINSTALLEDFILESPROPBAG;
    }
    else if ( IsEqualCLSID(rclsid, CLSID_IndexCleanerPropBag) )
    {
        dw = ID_INDEXCLEANERPROPBAG;
    }
    else
    {
        MiDebugMsg((0, TEXT("DataClen::DllGetClassObject for unknown factory type")));
        return ResultFromScode(CLASS_E_CLASSNOTAVAILABLE);
    }

    MiDebugMsg((0, TEXT("DataClen::DllGetClassObject for factory type %d"), (dw)));
    if (ID_COMPCLEANER == dw)
    {
       // 
       //Return our IClassFactory for making CCompShellExt objects
       //
       CCompCleanerClassFactory * pcf = new CCompCleanerClassFactory();

       if (NULL == pcf)
       {
          //
          //Error - couldn't make factory object
          //
          return ResultFromScode(E_OUTOFMEMORY);
       }
    
        //
        //Make sure the new class factory likes the requested interface
        //
        hr = pcf->QueryInterface (riid, ppvOut);
        if (FAILED (hr))
        {
          // 
          //Error - interface rejected
          //
          delete pcf;
        }
    }
    else
    {
       // 
       //Return our IClassFactory for making CCompShellExt objects
       //
       CCleanerClassFactory * pcf = new CCleanerClassFactory(dw);

       if (NULL == pcf)
       {
          //
          //Error - couldn't make factory object
          //
          return ResultFromScode(E_OUTOFMEMORY);
       }
    
       //
       //Make sure the new class factory likes the requested interface
       //
       hr = pcf->QueryInterface (riid, ppvOut);
       if (FAILED (hr))
       {
          // 
          //Error - interface rejected
          //
          delete pcf;
       }

    }

    return hr;
}

/*
**------------------------------------------------------------------------------
** DllCanUnloadNow
**
** Purpose:       Answers if the DLL can be free, that is, if there are no
**                references to anything this DLL provides.
** Return:        TRUE, if OK to unload (i.e. nobody is using us or has us locked)
**                FALSE, otherwise
** Notes;
** Mod Log:       Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
STDAPI 
DllCanUnloadNow(void)
{
    //  
    //Are there any outstanding objects or dll locks ?!?
    //
    SCODE sc;
    if ((0 == getDllObjectCount()) && (0 == getDllLockCount()))
        sc = S_OK;

    else
        sc = S_FALSE;

    //
    //Return actual result
    //
    return ResultFromScode(sc);
}


HRESULT CallRegInstall(LPCSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));
    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");
        if (pfnri)
        {
            char szTempPath[MAX_PATH];
#ifndef WINNT            
            char szMSDOSPath[MAX_PATH];
            char szMSDOSProfile[MAX_PATH];
            char szWindir[MAX_PATH];
            char szWinSys[MAX_PATH];
            HKEY hk;
#endif
            STRENTRY seReg[] = {
                { "TEMP_PATH", szTempPath },
#ifdef WINNT                
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
#else
                { "25", szWindir },
                { "11", szWinSys },
                { "MSDOS_PATH", szMSDOSPath },
                { "MSDOS_PROFILE_PATH", szMSDOSProfile },
#endif
            };
            STRTABLE stReg = { ARRAYSIZE(seReg), seReg };

            // Get the location of shdocvw.dll
#ifdef WINNT
            lstrcpyA(szTempPath, "%USERPROFILE%\\Local Settings\\Temp");
#else
            szMSDOSPath[0] = '\0';
            szMSDOSProfile[0] = '\0';
            
            GetTempPath(ARRAYSIZE(szTempPath), szTempPath);
            if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_SETUP_SETUP, &hk) == ERROR_SUCCESS)
            {
                DWORD dwType = REG_SZ;
                DWORD cbData = sizeof(szMSDOSPath);
                RegQueryValueEx(hk, REGSTR_VAL_BOOTDIR, NULL, &dwType, (LPBYTE)szMSDOSPath, &cbData);
                RegCloseKey(hk);

                lstrcat(szMSDOSPath, TEXT("MSDOS.SYS"));

                GetPrivateProfileString(TEXT("Paths"), TEXT("UninstallDir"), TEXT(""), szMSDOSProfile, ARRAYSIZE(szMSDOSProfile), szMSDOSPath);
            }

            GetWindowsDirectory( szWindir, ARRAYSIZE( szWindir ));
            GetSystemDirectory( szWinSys, ARRAYSIZE( szWinSys ));
#endif
            hr = pfnri(g_hDllModule, szSection, &stReg);
        }
        // since we only do this from DllInstall() don't load and unload advpack over and over
        // FreeLibrary(hinstAdvPack);
    }
    return hr;
}


STDAPI DllRegisterServer()
{
    HRESULT hrTemp, hr = S_OK;

    // Delete any old registration entries, then add the new ones.
    // Keep ADVPACK.DLL loaded across multiple calls to RegInstall.
    // (The inf engine doesn't guarantee DelReg/AddReg order, that's
    // why we explicitly unreg and reg here.)
    //
    hrTemp = CallRegInstall("RegDll");
    KEEP_FAILURE(hrTemp, hr);

#ifdef WINNT
    // I suppose we should call out NT-only registrations, just in case
    // we ever have to ship a win9x based shell again
    hrTemp = CallRegInstall("RegDllNT");
    KEEP_FAILURE(hrTemp, hr);
#else
    // I suppose we should call out NT-only registrations, just in case
    // we ever have to ship a win9x based shell again
    hrTemp = CallRegInstall("RegDllW95");
    KEEP_FAILURE(hrTemp, hr);
    
#endif

    return hr;
}

STDAPI
DllUnregisterServer()
{
    HRESULT hrTemp, hr = S_OK;
    
    // We only need one unreg call since all our sections share
    // the same backup information
    hrTemp = CallRegInstall("UnregDll");
    KEEP_FAILURE(hrTemp, hr);

    return hr;
}

/*
**------------------------------------------------------------------------------
** getDllModule
**
** Purpose:    returns this Dll's instance handle
** Return:        
** Notes;
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
HINSTANCE 
getDllModule(void)
{
    return g_hDllModule;
}

/*
**------------------------------------------------------------------------------
** DllObjectCount
**
** Purpose:    Object Count operations
** Return:        
** Notes;
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
UINT 
getDllObjectCount(void)
{
    return g_cDllObjects;   
}

UINT 
setDllObjectCount(
    UINT cNew
    )
{
    UINT cOld = g_cDllObjects;  
    g_cDllObjects = cNew;   
    return cOld;
}

UINT 
incDllObjectCount(void)
{
    return ++g_cDllObjects;   
}

UINT 
decDllObjectCount(void)
{
    return --g_cDllObjects;
}

/*
**------------------------------------------------------------------------------
** DllLockCount
**
** Purpose:    Lock Count operations
** Return:        
** Notes;
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
UINT 
getDllLockCount(void)
{
    return g_cDllLocks;   
}

UINT 
setDllLockCount(
    ULONG cNew
    )
{
    ULONG cOld = g_cDllLocks;  
    g_cDllLocks = cNew;   
    return cOld;
}

UINT 
incDllLockCount(void)
{
    return ++g_cDllLocks;   
}

UINT 
decDllLockCount(void)
{
    return --g_cDllLocks;
}



/*
**------------------------------------------------------------------------------
** Cleaner class factory method definitions
**------------------------------------------------------------------------------
*/

/*
**------------------------------------------------------------------------------
** CCleanerClassFactory::CDataDrivenCleanerClassFactory
**
** Purpose:    Constructor
** Parameters:
** Notes;
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
CCleanerClassFactory::CCleanerClassFactory(DWORD dw)
{
    //
    //Nobody is using us yet
    //
    _cRef = 0L;

    // Store which type of class factory we really are.  The same factory is
    // used to create one of 5 different objects.  The dw value determines
    // which of these objects we should create when CreateInstance is called.
    _dwID = dw;

    //
    //Increment our global count of objects in the DLL
    //
    incDllObjectCount();
}
/*
**------------------------------------------------------------------------------
** CCleanerClassFactory::~CDataDrivenCleanerClassFactory
**
** Purpose:    Destructor
** Parameters:
** Notes;
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
CCleanerClassFactory::~CCleanerClassFactory()               
{
    //
    //One less factory to worry about
    //
    decDllObjectCount();
}

/*
**------------------------------------------------------------------------------
** CCleanerClassFactory::QueryInterface
**
** Purpose:    Part of the IUnknown interface
** Parameters:
**    riid  -  interface ID to query on 
**    ppv   -  pointer to interface if we support it
** Notes;
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
STDMETHODIMP CCleanerClassFactory::QueryInterface(
   REFIID      riid,
   LPVOID FAR *ppv
   )
{
    MiDebugMsg((0, TEXT("CDataDrivenCleanerClassFactory::QueryInterface")));
    *ppv = NULL;

    //
    //Is it an interface we support
    //
    if (IsEqualIID(riid, IID_IUnknown))
    {
        MiDebugMsg((0, TEXT("CDataDrivenCleanerClassFactory::QueryInterface for IID_IUnknown")));
        //
        //Typecast to the requested interface so C++ sets up
        //the virtual tables correctly
        //
        *ppv = (LPUNKNOWN)(LPCLASSFACTORY) this;

        AddRef();

        return NOERROR;
    }

   
    if (IsEqualIID(riid, IID_IClassFactory))
    {
        MiDebugMsg((0, TEXT("CDataDrivenCleanerClassFactory::QueryInterface for IID_IClassFactory")));
        //
        //Typecast to the requested interface so C++ sets up
        //the virtual tables correctly
        //
        *ppv = (LPCLASSFACTORY)this;

        AddRef();

        return NOERROR;
    }

    MiDebugMsg((0, TEXT("CDataDrivenCleanerClassFactory::QueryInterface for unknown IID")));
    //
    //Error - We don't support the requested interface
    //
    return E_NOINTERFACE;
}   

/*
**------------------------------------------------------------------------------
** CCleanerClassFactory::AddRef
**
** Purpose:    ups the reference count to this object
** Notes;
** Return:     current refernce count
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
STDMETHODIMP_(ULONG) CCleanerClassFactory::AddRef()
{
    return ++_cRef;
}

/*
**------------------------------------------------------------------------------
** CCleanerClassFactory::Release
**
** Purpose:    downs the reference count to this object
**             and deletes the object if no one is using it
** Notes;
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/

STDMETHODIMP_(ULONG) CCleanerClassFactory::Release()
{
    //
    //Decrement and check
    //
    if (--_cRef)
        return _cRef;

    //  
    //No references left
    //
    delete this;

    return 0L;
}

/*
**------------------------------------------------------------------------------
** CCleanerClassFactory::CreateInstance
**
** Purpose:    Creates an instance of the requested interface
** Notes;
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
STDMETHODIMP CCleanerClassFactory::CreateInstance(
   LPUNKNOWN   pUnkOuter,
   REFIID      riid,
   LPVOID *    ppvObj
   )
{
    HRESULT hr = E_NOINTERFACE;
    *ppvObj = NULL;

    //
    //Shell extensions typically don't support aggregation (inheritance)
    //
    if (pUnkOuter)
    {
        //
        //Error - we don't support aggregation
        //
        return ResultFromScode (CLASS_E_NOAGGREGATION);
    }

#ifdef WINNT
    if ( _dwID == ID_CONTENTINDEXCLEANER )
    {
        MiDebugMsg((0, TEXT("CCleanerClassFactory::CreateInstance creating CContentIndexCleander")));
        
        //
        //Create an instance of the 'ContentIndex Cleaner' object
        //
        LPCCONTENTINDEXCLEANER pContentIndexCleaner = new CContentIndexCleaner();  
        if (NULL == pContentIndexCleaner)
        {
            //Error - not enough memory
            return ResultFromScode (E_OUTOFMEMORY);
        }

        //
        //Make sure the new object likes the requested interface
        //
        hr = pContentIndexCleaner->QueryInterface (riid, ppvObj);
        if (FAILED(hr))
            delete pContentIndexCleaner;
    }
    else 
#endif
    if ( IsEqualIID(riid, IID_IEmptyVolumeCache) )
    {
        MiDebugMsg((0, TEXT("CDCleanerClassFactory::CreateInstance creating CDataDrivenCleaner")));
        //
        //Create an instance of the 'System Data Driven Cleaner' object
        //
        LPCDATADRIVENCLEANER pDataDrivenCleaner = new CDataDrivenCleaner();  
        if (NULL == pDataDrivenCleaner)
        {
            //Error - not enough memory
            return ResultFromScode (E_OUTOFMEMORY);
        }

        //
        //Make sure the new object likes the requested interface
        //
        hr = pDataDrivenCleaner->QueryInterface (riid, ppvObj);
        if (FAILED(hr))
            delete pDataDrivenCleaner;
    }
    else if ( IsEqualIID(riid, IID_IPropertyBag) )
    {
        MiDebugMsg((0, TEXT("CCleanerClassFactory::CreateInstance creating CDataDrivenPropBag(%d)"), (_dwID)));
        //
        //Create an instance of the 'System Data Driven Property Bag' object
        //
        LPCDATADRIVENPROPBAG pDataDrivenPropBag = new CDataDrivenPropBag(_dwID);  
        if (NULL == pDataDrivenPropBag)
        {
            //Error - not enough memory
            return ResultFromScode (E_OUTOFMEMORY);
        }

        //
        //Make sure the new object likes the requested interface
        //
        hr = pDataDrivenPropBag->QueryInterface (riid, ppvObj);
        if (FAILED(hr))
            delete pDataDrivenPropBag;
    }
    else
    {
        MiDebugMsg((0, TEXT("CDataDrivenCleanerClassFactory::CreateInstance called for unknown riid (%d)"), (_dwID)));
    }

    return hr;      
}

/*
**------------------------------------------------------------------------------
** CCleanerClassFactory::LockServer
**
** Purpose:    Locks or unlocks the server
** Notes;      Increments/Decrements the DLL lock count
**             so we know when we can safely remove the DLL
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
STDMETHODIMP CCleanerClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        incDllLockCount ();
    else
        decDllLockCount ();

    return NOERROR;
}





/*
**------------------------------------------------------------------------------
** CDataDrivenCleaner class method definitions
**------------------------------------------------------------------------------
*/

/*
**------------------------------------------------------------------------------
** CDataDrivenCleaner::CDataDrivenCleaner
**
** Purpose:    Default constructor
** Notes:
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
CDataDrivenCleaner::CDataDrivenCleaner()
{
    //  
    //Set to default values
    //
    _cRef         = 0L;
    _lpdObject    = NULL;

    _cbSpaceUsed.QuadPart = 0;
    _cbSpaceFreed.QuadPart = 0;
    _szVolume[0] = (TCHAR)NULL;
    _szFolder[0] = (TCHAR)NULL;
    _filelist[0] = (TCHAR)NULL;
    _dwFlags = 0;

    _head = NULL;

    //
    //increment global object count
    //
    incDllObjectCount();
}

/*
**------------------------------------------------------------------------------
** CDataDrivenCleaner::~CDataDrivenCleaner
**
** Purpose:    Destructor
** Notes:
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
CDataDrivenCleaner::~CDataDrivenCleaner()
{
    //  
    //Cleanup up associated IDataObject interface
    //
    if (_lpdObject != NULL)
        _lpdObject->Release();

    _lpdObject = NULL;

    //
    //Free the list of files
    //
    FreeList(_head);
    _head = NULL;
   
    //
    //Decrement global object count
    //
    decDllObjectCount();
}

/*
**------------------------------------------------------------------------------
** CDataDrivenCleaner::QueryInterface
**
** Purpose:    Part of the IUnknown interface
** Parameters:
**    riid  -  interface ID to query on 
**    ppv   -  pointer to interface if we support it
** Return:     NOERROR on success, E_NOINTERFACE otherwise
** Notes;
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
STDMETHODIMP CDataDrivenCleaner::QueryInterface(
   REFIID      riid, 
   LPVOID FAR *ppv
   )
{
    *ppv = NULL;

    //
    //Check for IUnknown interface request
    //
    if (IsEqualIID (riid, IID_IUnknown))
    {
        // 
        //Typecast to the requested interface so C++ sets up
        //the virtual tables correctly
        //
        *ppv = (LPUNKNOWN) this;
        AddRef();
        return NOERROR;
    }  

    
    //
    //Check for IEmptyVolumeCache interface request
    //
    if (IsEqualIID (riid, IID_IEmptyVolumeCache))
    {
        // 
        //Typecast to the requested interface so C++ sets up
        //the virtual tables correctly
        //
        *ppv = (IEmptyVolumeCache*) this;
        AddRef();
        return NOERROR;
    }  


    //
    //Error - unsupported interface requested
    //
    return E_NOINTERFACE;
}

/*
**------------------------------------------------------------------------------
** CDataDrivenCleaner::AddRef
**
** Purpose:    ups the reference count to this object
** Notes;
** Return:     current refernce count
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
STDMETHODIMP_(ULONG) CDataDrivenCleaner::AddRef()
{
    MiDebugMsg((0, TEXT("CDataDrivenCleaner::AddRef Ref is %d for %s"), (_cRef+1), _szFolder));

    return ++_cRef;
}

/*
**------------------------------------------------------------------------------
** CDataDrivenCleaner::Release
**
** Purpose:    downs the reference count to this object
**             and deletes the object if no one is using it
** Notes;
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
STDMETHODIMP_(ULONG) CDataDrivenCleaner::Release()
{
    MiDebugMsg((0, TEXT("CDataDrivenCleaner::Release Ref is %d"), (_cRef-1)));

    //  
    //Decrement and check
    //
    if (--_cRef)
        return _cRef;

    //
    //No references left to this object
    //
    delete this;

    return 0L;
}


/*
**------------------------------------------------------------------------------
** CDataDrivenCleaner::Initialize
**
** Purpose:     Initializes the System Data Driven Cleaner and returns the 
**              specified IEmptyVolumeCache flags to the cache manager.
** Notes;
** Mod Log:     Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
STDMETHODIMP CDataDrivenCleaner::Initialize(
    HKEY    hRegKey,
    LPCWSTR pszVolume,
    LPWSTR  *ppszDisplayName,
    LPWSTR  *ppszDescription,
    DWORD   *pdwFlags
    )
{
    DWORD   dwType;
    DWORD   cbData;
    TCHAR   szTempFolder[MAX_PATH];
    TCHAR   acsVolume[MAX_PATH];
    ULONG   DaysLastAccessed;
    PTSTR   pTemp;
    LPTSTR  lpSingleFolder;
    BOOL    bFolderOnVolume;

    MiDebugMsg((0, TEXT("CDataDrivenCleaner::Initialize enter")));

    _bPurged = FALSE;

    //
    //This means that cleanmgr.exe will get these values from
    //the registry.
    //
    *ppszDisplayName = NULL;
    *ppszDescription = NULL;

    _ftMinLastAccessTime.dwLowDateTime = 0;
    _ftMinLastAccessTime.dwHighDateTime = 0;

    if (*pdwFlags & EVCF_SETTINGSMODE)
    {
        MiDebugMsg((0, TEXT("We are in settings mode so just returning S_OK")));
        return S_OK;
    }

    //
    //We can't run some cleaners if certain processes are running, this is becasue we
    //might delete the processes temp files right out from under it...bad!.  So we will
    //check if any of the Fail Processes are running and if they are we will not show
    //this cleaner.
    //
    if (bW98ProcessIsRunning(hRegKey))
    {
        return S_FALSE;
    }

    //
    //Convert the volume name to ANSI
    //
    SHUnicodeToTChar( pszVolume, acsVolume, MAX_PATH );

    //
    //Read in the registry values
    //
    _szFolder[0] = TCHAR('\0');
    _dwFlags = 0;
    _filelist[0] = TCHAR('\0');
    _CleanupString[0] = TCHAR('\0');
    
    if (hRegKey)
    {
        dwType = REG_EXPAND_SZ;
        cbData = sizeof(_szFolder);
        RegQueryValueEx(hRegKey, REGSTR_VAL_FOLDER, NULL, &dwType, (LPBYTE)_szFolder, &cbData);
        if ( *_szFolder )
        {
            if ( REG_SZ == dwType )
            {
                // REG_SZ that needs to be converted to a MULTI_SZ
                //
                //Fix up the starting folders.  This can also be either a MULTI_SZ list of starting
                //folders or a list separated by the '|' character.  This character was choosen
                //because it is an invalid filename character.
                //
                pTemp = _szFolder;
                while(*pTemp)
                {
                    if (*pTemp == TEXT('|'))
                    {
                        *pTemp = NULL;
                        pTemp = pTemp++;    // ++ is fine since *pTemp==NULL
                    }
                    else
                    {
                        pTemp = CharNext(pTemp);
                    }
                }
                // double NULL terminated
                pTemp = pTemp++;            // ++ is fine since *pTemp==NULL
                *pTemp = NULL;
            }
            else if ( REG_EXPAND_SZ == dwType )
            {
                // single folder with environment expantion
                if ( ExpandEnvironmentStrings(_szFolder, szTempFolder, MAX_PATH-1) )    // leave extra space for double NULL
                {
                    lstrcpy(_szFolder, szTempFolder);
                }
                // double NULL terminated.
                _szFolder[lstrlen(_szFolder)+1] = NULL;
                
            }
            else if ( REG_MULTI_SZ == dwType )
            {
                // nothing else to do, we're done
            }
            else 
            {
                // invalid data
                _szFolder[0] = NULL;
            }
        }

        dwType = REG_DWORD;
        cbData = sizeof(_dwFlags);
        RegQueryValueEx(hRegKey, REGSTR_VAL_FLAGS, NULL, &dwType, (LPBYTE)&_dwFlags, &cbData);

        dwType = REG_SZ;
        cbData = sizeof(_filelist);
        RegQueryValueEx(hRegKey, REGSTR_VAL_FILELIST, NULL, &dwType, (LPBYTE)_filelist, &cbData);

        dwType = REG_DWORD;
        DaysLastAccessed = 0;
        cbData = sizeof(DaysLastAccessed);
        RegQueryValueEx(hRegKey, REGSTR_VAL_LASTACCESS, NULL, &dwType, (LPBYTE)&DaysLastAccessed, &cbData);     

        dwType = REG_SZ;
        cbData = sizeof(_CleanupString);
        RegQueryValueEx(hRegKey, REGSTR_VAL_CLEANUPSTRING, NULL, &dwType, (LPBYTE)_CleanupString, &cbData);
    }

    //
    //If the DDEVCF_RUNIFOUTOFDISKSPACE bit is set then make sure the EVCF_OUTOFDISKSPACE flag
    //was passed in. If it was not then return S_FALSE so we won't run.
    //
    if ((_dwFlags & DDEVCF_RUNIFOUTOFDISKSPACE) &&
        (!(*pdwFlags & EVCF_OUTOFDISKSPACE)))
    {
        return S_FALSE;
    }

    lstrcpy(_szVolume, (PTCHAR)acsVolume);

    //
    //Fix up the filelist.  The file list can either be a MULTI_SZ list of files or 
    //a list of files separated by the ':' colon character or a '|' bar character. 
    //These characters were choosen because they are invalid filename characters.
    //
    pTemp = _filelist;
    while(*pTemp)
    {
        if (*pTemp == TEXT(':') || *pTemp == TEXT('|'))
        {
            *pTemp = NULL;
            pTemp = pTemp++;    // ++ is fine since *pTemp==NULL
        }
        else
        {
            pTemp = CharNext(pTemp);
        }
    }
    pTemp = pTemp++;            // ++ is fine since *pTemp==NULL
    *pTemp = NULL;

    bFolderOnVolume = FALSE;
    if (_szFolder[0] == '\0')
    {
        // If no folder value is given so use the current volume

        lstrcpy(_szFolder, (PTCHAR)acsVolume);
        bFolderOnVolume = TRUE;
    }
    else
    {
        // A valid folder value was given, loop over each folder to check for "?" and ensure that
        // we are on a drive that contains some of the specified folders

        for(lpSingleFolder = &(_szFolder[0]); *lpSingleFolder; lpSingleFolder += lstrlen(lpSingleFolder) + 1)
        {   
            //
            //Replace the first character of each folder (driver letter) if it is a '?'
            //with the current volume.
            //
            if (*lpSingleFolder == TEXT('?'))
            {
                *lpSingleFolder = acsVolume[0];
                bFolderOnVolume = TRUE;
            }

            //If their is a valid "folder" value in the registry make sure that it is 
            //on the specified volume.  If it is not then return S_FALSE so that we are
            //not displayed on the list of items that can be freed.
            if ( !bFolderOnVolume )
            {
                lstrcpy(szTempFolder, lpSingleFolder);
                szTempFolder[lstrlen((LPCTSTR)acsVolume)] = TCHAR('\0');
                if ( lstrcmpi((LPCTSTR)acsVolume, szTempFolder) == 0 )
                {
                    bFolderOnVolume = TRUE;
                }
            }
        }
    }

    if (bFolderOnVolume == FALSE)
    {
        MiDebugMsg((0, TEXT("CDataDrivenCleaner::Initialize -> None of the specified folders are on this volume (%s)"),
            acsVolume));

        //
        //Don't display us in the list
        //
        return S_FALSE;
    }

    //
    //Determine the LastAccessedTime 
    //
    if (DaysLastAccessed != 0)
    {
        ULARGE_INTEGER  ulTemp, ulLastAccessTime;
        SYSTEMTIME      st;
        FILETIME        ft;

        //Determine the number of days in 100ns units
        ulTemp.LowPart = FILETIME_HOUR_LOW;
        ulTemp.HighPart = FILETIME_HOUR_HIGH;

        ulTemp.QuadPart *= DaysLastAccessed;

        //Get the current FILETIME
        GetSystemTime(&st);
        SystemTimeToFileTime(&st, &ft);
        ulLastAccessTime.LowPart = ft.dwLowDateTime;
        ulLastAccessTime.HighPart = ft.dwHighDateTime;

        //Subtract the Last Access number of days (in 100ns units) from 
        //the current system time.
        ulLastAccessTime.QuadPart -= ulTemp.QuadPart;

        //Save this minimal Last Access time in the FILETIME member variable
        //ftMinLastAccessTime.
        _ftMinLastAccessTime.dwLowDateTime = ulLastAccessTime.LowPart;
        _ftMinLastAccessTime.dwHighDateTime = ulLastAccessTime.HighPart;

        _dwFlags |= DDEVCF_PRIVATE_LASTACCESS;
    }

    //
    //Tell the cache manager to disable this item by default
    //
    *pdwFlags = 0;

    if (_dwFlags & DDEVCF_DONTSHOWIFZERO)
        *pdwFlags |= EVCF_DONTSHOWIFZERO;
    
    MiDebugMsg((0, TEXT("CDataDrivenCleaner::Initialize exit (%s)(%s)(%s)"), _szVolume, _szFolder, _filelist));

    //
    //First check to see if we have anything to clean up.  Don't search the whole disk
    //
    return S_OK;
}

/*
**------------------------------------------------------------------------------
** CDataDrivenCleaner::GetSpaceUsed
**
** Purpose:     Returns the total amount of space that the data driven cleaner
**              can remove.
** Notes;
** Mod Log:     Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
STDMETHODIMP CDataDrivenCleaner::GetSpaceUsed(
    DWORDLONG                   *pdwSpaceUsed,
    IEmptyVolumeCacheCallBack   *picb
    )
{
    LPTSTR      lpSingleFolder;
    
    MiDebugMsg((0, TEXT("CDataDrivenCleaner::GetSpaceUsed (%s)(%s)(%s)"), _szVolume, _szFolder, _filelist));

    _cbSpaceUsed.QuadPart = 0;

    //
    //Walk all of the folders in the folders list scanning for disk space.
    //
    for(lpSingleFolder = &(_szFolder[0]); *lpSingleFolder; lpSingleFolder += lstrlen(lpSingleFolder) + 1)
        WalkForUsedSpace(lpSingleFolder, picb);

    picb->ScanProgress(_cbSpaceUsed.QuadPart, EVCCBF_LASTNOTIFICATION, NULL);
    
    *pdwSpaceUsed =  _cbSpaceUsed.QuadPart;

    return S_OK;
}

/*
**------------------------------------------------------------------------------
** CDataDrivenCleaner::Purge
**
** Purpose:     Purges (deletes) all of the files specified in the "filelist"
**              portion of the registry.
** Notes;
** Mod Log:     Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
STDMETHODIMP CDataDrivenCleaner::Purge(
    DWORDLONG                   dwSpaceToFree,
    IEmptyVolumeCacheCallBack   *picb
    )
{
    MiDebugMsg((0, TEXT("CDataDrivenCleaner::Purge")));

    _bPurged = TRUE;

    //
    //Delete the files
    //
    PurgeFiles(picb, FALSE);
    PurgeFiles(picb, TRUE);

    //
    //Send the last notification to the cleanup manager
    //
    picb->PurgeProgress(_cbSpaceFreed.QuadPart, (_cbSpaceUsed.QuadPart - _cbSpaceFreed.QuadPart),
        EVCCBF_LASTNOTIFICATION, NULL);

    //
    //Free the list of files
    //
    FreeList(_head);
    _head = NULL;

    //
    //Run the "CleanupString" command line if one was provided
    //
    if (*_CleanupString)
    {
        STARTUPINFO StartInfo;
        PROCESS_INFORMATION ProcInfo;

        memset(&StartInfo, 0, sizeof(STARTUPINFO));
        StartInfo.cb = sizeof(STARTUPINFO);
        memset(&ProcInfo, 0, sizeof(PROCESS_INFORMATION));
    
        if (!CreateProcess(NULL, _CleanupString, NULL, NULL, FALSE, 0, NULL, NULL,
            &StartInfo, &ProcInfo))
        {
            MiDebugMsg((0, TEXT("CreateProcess(%s) failed with error %d"),
                _CleanupString, GetLastError()));
        }
    }

    //
    //For now we'll just act like we cleaned up
    //
    return S_OK;
}

/*
**------------------------------------------------------------------------------
** CDataDrivenCleaner::ShowProperties
**
** Purpose:     Currently this is not used in the Data Driven Cleaner.
**
** Mod Log:     Created by Jason Cobb (6/97)
**------------------------------------------------------------------------------
*/
STDMETHODIMP CDataDrivenCleaner::ShowProperties(
    HWND hwnd
    )
{
    return S_OK;
}

/*
**------------------------------------------------------------------------------
** CDataDrivenCleaner::Deactivate
**
** Purpose:     Deactivates the System Driven Data cleaner...this basically 
**              does nothing.
** Notes;
** Mod Log:     Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
STDMETHODIMP CDataDrivenCleaner::Deactivate(
    DWORD   *pdwFlags
    )
{
    MiDebugMsg((0, TEXT("CDataDrivenCleaner::Deactivate (%s)(%s)(%s)"), _szVolume, _szFolder, _filelist));

    *pdwFlags = 0;

    //
    //See if this object should be removed.
    //Note that we will only remove a cleaner if it's Purge() method was run.
    //
    if (_bPurged && (_dwFlags & DDEVCF_REMOVEAFTERCLEAN))
        *pdwFlags |= EVCF_REMOVEFROMLIST;
    
    return S_OK;
}

/*
**------------------------------------------------------------------------------
** bLastAccessisOK
**
** Purpose:     This function checks if the file is a specified number of days
**              old (if the "lastaccess" DWORD is in the registry for this cleaner).
**              If the file has not been accessed in the specified number of days
**              then it can safely be deleted.  If the file has been accessed in
**              that number of days then the file will not be deleted.
**
** Notes;
** Mod Log:     Created by Jason Cobb (7/97)
**------------------------------------------------------------------------------
*/
BOOL
CDataDrivenCleaner::bLastAccessisOK(
    FILETIME ftFileLastAccess
    )
{
    if (_dwFlags & DDEVCF_PRIVATE_LASTACCESS)
    {
        //Is the last access FILETIME for this file less than the current
        //FILETIME minus the number of specified days?
        if (CompareFileTime(&ftFileLastAccess, &_ftMinLastAccessTime) == -1)
            return TRUE;

        else
            return FALSE;
    }

    return TRUE;
}

/*
**------------------------------------------------------------------------------
** bFileIsOpen
**
** Purpose:     This function checks if a file is open by doing a CreateFile
**              with fdwShareMode of 0.  If GetLastError() retuns
**              ERROR_SHARING_VIOLATION then this function retuns TRUE because
**              someone has the file open.  Otherwise this function retuns false.
**
** Notes;
** Mod Log:     Created by Jason Cobb (7/97)
**------------------------------------------------------------------------------
*/
BOOL
bFileIsOpen(
    LPTSTR lpFile,
    LPFILETIME lpftFileLastAccess
    )
{
    HANDLE hFile;

    if (((hFile = CreateFile(lpFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) &&
         (GetLastError() == ERROR_SHARING_VIOLATION))
    {
        //
        //File is currently open by someone
        //
        return TRUE;
    }

    //
    //File is not currently open
    //
    SetFileTime(hFile, NULL, lpftFileLastAccess, NULL);
    CloseHandle(hFile);
    return FALSE;
}

/*
**------------------------------------------------------------------------------
** CDataDrivenCleaner::WalkAllFiles
**
** Purpose:     This function will recursively walk the specified directory and 
**              add all the files under this directory to the delete list.
** Notes;
** Mod Log:     Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
BOOL
CDataDrivenCleaner::WalkAllFiles(
    PTCHAR  lpPath,
    IEmptyVolumeCacheCallBack   *picb
    )
{
    BOOL            bRet = TRUE;
    BOOL            bFind = TRUE;
    HANDLE          hFind;
    WIN32_FIND_DATA wd;
    DWORD           dwAttributes;
    TCHAR           szFindPath[MAX_PATH];
    TCHAR           szAddFile[MAX_PATH];
    ULARGE_INTEGER  dwFileSize;
    static DWORD    dwCount = 0;

    //
    //If this is a directory then tack a *.* onto the end of the path
    //and recurse through the rest of the directories
    //
    dwAttributes = GetFileAttributes(lpPath);
    if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        lstrcpy(szFindPath, lpPath);
        if (szFindPath[lstrlen(szFindPath) - 1] != TCHAR('\\'))
            lstrcat(szFindPath, TEXT("\\"));
        lstrcat(szFindPath, TEXT("*.*"));

        bFind = TRUE;
        hFind = FindFirstFile(szFindPath, &wd);
        while (hFind != INVALID_HANDLE_VALUE && bFind)
        {
            //
            //First check if the attributes of this file are OK for us to delete.
            //
            if (((!(wd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) ||
                (_dwFlags & DDEVCF_REMOVEREADONLY)) &&
                ((!(wd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)) ||
                (_dwFlags & DDEVCF_REMOVESYSTEM)) &&
                ((!(wd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) ||
                (_dwFlags & DDEVCF_REMOVEHIDDEN)))
            {
                lstrcpy(szAddFile, lpPath);
                if (szAddFile[lstrlen(szAddFile) - 1] != TCHAR('\\'))
                    lstrcat(szAddFile, TEXT("\\"));
                lstrcat(szAddFile, wd.cFileName);

                //
                //This is a file so check if it is open
                //
                if ((!(wd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) &&
                    (bFileIsOpen(szAddFile, &wd.ftLastAccessTime) == FALSE) &&
                    (bLastAccessisOK(wd.ftLastAccessTime)))
                {
                    //
                    //File is not open so add it to the list
                    //
                    dwFileSize.HighPart = wd.nFileSizeHigh;
                    dwFileSize.LowPart = wd.nFileSizeLow;
                    AddFileToList(szAddFile, dwFileSize, FALSE);
                }   

                //
                //CallBack the cleanup Manager to update the UI
                //
                if ((dwCount++ % 10) == 0)
                {
                    if (picb->ScanProgress(_cbSpaceUsed.QuadPart, 0, NULL) == E_ABORT)
                    {
                        //
                        //User aborted
                        //
                        FindClose(hFind);
                        return FALSE;
                    }
                }
            }
        
            bFind = FindNextFile(hFind, &wd);
        }
    
        FindClose(hFind);

        //
        //Recurse through all of the directories
        //
        lstrcpy(szFindPath, lpPath);
        if (szFindPath[lstrlen(szFindPath) - 1] != TCHAR('\\'))
            lstrcat(szFindPath, TEXT("\\*.*"));
        else    
            lstrcat(szFindPath, TEXT("*.*"));

        bFind = TRUE;
        hFind = FindFirstFile(szFindPath, &wd);
        while (hFind != INVALID_HANDLE_VALUE && bFind)
        {
            //
            //This is a directory
            //
            if ((wd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                (lstrcmp(wd.cFileName, TEXT(".")) != 0) &&
                (lstrcmp(wd.cFileName, TEXT("..")) != 0))
            {
                lstrcpy(szAddFile, lpPath);
                if (szAddFile[lstrlen(szAddFile) - 1] != TCHAR('\\'))
                    lstrcat(szAddFile, TEXT("\\"));
                lstrcat(szAddFile, wd.cFileName);

                dwFileSize.QuadPart = 0;
                AddFileToList(szAddFile, dwFileSize, TRUE);   
                
                if (WalkAllFiles(szAddFile, picb) == FALSE)
                {
                    //
                    //User cancled
                    //
                    FindClose(hFind);
                    return FALSE;
                }
            }
        
            bFind = FindNextFile(hFind, &wd);
        }

        FindClose(hFind);
    }

    return bRet;
}

/*
**------------------------------------------------------------------------------
** CDataDrivenCleaner::WalkForUsedSpace
**
** Purpose:     This function will walk the specified directory and create a 
**              linked list of files that can be deleted.  It will also
**              increment the member variable to indicate how much disk space
**              these files are taking.
**              It will look at the dwFlags member variable to determine if it
**              needs to recursively walk the tree or not.
** Notes;
** Mod Log:     Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
BOOL
CDataDrivenCleaner::WalkForUsedSpace(
    PTCHAR  lpPath,
    IEmptyVolumeCacheCallBack   *picb
    )
{
    BOOL            bRet = TRUE;
    BOOL            bFind = TRUE;
    HANDLE          hFind;
    WIN32_FIND_DATA wd;
    DWORD           dwAttributes;
    TCHAR           szFindPath[MAX_PATH];
    TCHAR           szAddFile[MAX_PATH];
    ULARGE_INTEGER  dwFileSize;
    static DWORD    dwCount = 0;
    LPTSTR          lpSingleFile;

    //
    //If this is a directory then tack a *.* onto the end of the path
    //and recurse through the rest of the directories
    //
    dwAttributes = GetFileAttributes(lpPath);
    if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        //
        //Enum through the MULTI_SZ filelist
        //
        for(lpSingleFile = _filelist; *lpSingleFile; lpSingleFile += lstrlen(lpSingleFile) + 1)
        {
            lstrcpy(szFindPath, lpPath);
            PathAppend(szFindPath, lpSingleFile);

            bFind = TRUE;
            hFind = FindFirstFile(szFindPath, &wd);
            while (hFind != INVALID_HANDLE_VALUE && bFind)
            {
                if ( StrCmp( wd.cFileName, TEXT(".")) == 0 || StrCmp( wd.cFileName, TEXT("..")) == 0 )
                {
                    // ignore these two, otherwise we'll cover the whole disk..
                    bFind = FindNextFile(hFind, &wd);
                    continue;
                }
                
                //
                //First check if the attributes of this file are OK for us to delete.
                //
                if (((!(wd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) ||
                    (_dwFlags & DDEVCF_REMOVEREADONLY)) &&
                    ((!(wd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)) ||
                    (_dwFlags & DDEVCF_REMOVESYSTEM)) &&
                    ((!(wd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) ||
                    (_dwFlags & DDEVCF_REMOVEHIDDEN)))
                {
                    lstrcpy(szAddFile, lpPath);
                    PathAppend(szAddFile, wd.cFileName);

                    //
                    //Check if this is a subdirectory
                    //
                    if (wd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        if (_dwFlags & DDEVCF_REMOVEDIRS)
                        {
                            dwFileSize.QuadPart = 0;
                            AddFileToList(szAddFile, dwFileSize, TRUE);

                            if (WalkAllFiles(szAddFile, picb) == FALSE)
                            {
                                //
                                //User cancled
                                //
                                FindClose(hFind);
                                return FALSE;
                            }
                        }
                    }

                    //
                    //This is a file so check if it is open
                    //
                    else if ((bFileIsOpen(szAddFile, &wd.ftLastAccessTime) == FALSE) &&
                        (bLastAccessisOK(wd.ftLastAccessTime)))
                    {
                        //
                        //File is not open so add it to the list
                        //
                        dwFileSize.HighPart = wd.nFileSizeHigh;
                        dwFileSize.LowPart = wd.nFileSizeLow;
                        AddFileToList(szAddFile, dwFileSize, FALSE);
                    }                       

                    //
                    //CallBack the cleanup Manager to update the UI
                    //
                    if ((dwCount++ % 10) == 0)
                    {
                        if (picb->ScanProgress(_cbSpaceUsed.QuadPart, 0, NULL) == E_ABORT)
                        {
                            //
                            //User aborted
                            //
                            FindClose(hFind);
                            return FALSE;
                        }
                    }
                }
            
                bFind = FindNextFile(hFind, &wd);
            }
        
            FindClose(hFind);
        }

        if (_dwFlags & DDEVCF_DOSUBDIRS)
        {
            //
            //Recurse through all of the directories
            //
            lstrcpy(szFindPath, lpPath);
            PathAppend(szFindPath, TEXT("*.*"));

            bFind = TRUE;
            hFind = FindFirstFile(szFindPath, &wd);
            while (hFind != INVALID_HANDLE_VALUE && bFind)
            {
                //
                //This is a directory
                //
                if ((wd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                    (lstrcmp(wd.cFileName, TEXT(".")) != 0) &&
                    (lstrcmp(wd.cFileName, TEXT("..")) != 0))
                {
                    lstrcpy(szAddFile, lpPath);
                    PathAppend(szAddFile, wd.cFileName);

                    if (WalkForUsedSpace(szAddFile, picb) == FALSE)
                    {
                        //
                        //User cancled
                        //
                        FindClose(hFind);
                        return FALSE;
                    }
                }
        
                bFind = FindNextFile(hFind, &wd);
            }

            FindClose(hFind);
        }

        if ( _dwFlags & DDEVCF_REMOVEPARENTDIR )
        {
            // add the parent directory to the list if we are told to remove them....
            dwFileSize.QuadPart = 0;
            AddFileToList(lpPath, dwFileSize, TRUE);
        }
    }
    else
    {
        MiDebugMsg((0, TEXT("CDataDrivenCleaner::WalkForUsedSpace -> %s is NOT a directory!"),
            lpPath));
    }

    return bRet;
}

/*
**------------------------------------------------------------------------------
** CDataDrivenCleaner::AddFileToList
**
** Purpose:     Adds a file to the linked list of files.
** Notes;
** Mod Log:     Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
BOOL
CDataDrivenCleaner::AddFileToList(
    PTCHAR  lpFile,
    ULARGE_INTEGER  filesize,
    BOOL bDirectory
    )
{
    BOOL                bRet = TRUE;
    PCLEANFILESTRUCT    pNew;

    pNew = (PCLEANFILESTRUCT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CLEANFILESTRUCT));

    if (pNew == NULL)
    {
        MiDebugMsg((0, TEXT("CDataDrivenCleaner::AddFileToList -> ERROR HeapAlloc() failed with error %d"),
            GetLastError()));
        return FALSE;
    }

    lstrcpy(pNew->file, lpFile);
    pNew->ulFileSize.QuadPart = filesize.QuadPart;
    pNew->bSelected = TRUE;
    pNew->bDirectory = bDirectory;

    if (_head)
        pNew->pNext = _head;
    else
        pNew->pNext = NULL;

    _head = pNew;

    _cbSpaceUsed.QuadPart += filesize.QuadPart;

    return bRet;
}

/*
**------------------------------------------------------------------------------
** CDataDrivenCleaner::PurgeFiles
**
** Purpose:     Removes the files from the disk.
** Notes;
** Mod Log:     Created by Jason Cobb (6/97)
**------------------------------------------------------------------------------
*/
void
CDataDrivenCleaner::PurgeFiles(
    IEmptyVolumeCacheCallBack   *picb,
    BOOL bDoDirectories
    )
{
    PCLEANFILESTRUCT    pCleanFile = _head;

    _cbSpaceFreed.QuadPart = 0;

    while (pCleanFile)
    {
        //
        //Remove a directory
        //
        if (bDoDirectories && pCleanFile->bDirectory)
        {
            SetFileAttributes(pCleanFile->file, FILE_ATTRIBUTE_NORMAL);
            if (!RemoveDirectory(pCleanFile->file))
            {
                MiDebugMsg((0, TEXT("Error RemoveDirectory(%s) returned error %d"),
                    pCleanFile->file, GetLastError()));
            }
        }

        //
        //Remove a file
        //
        else if (!bDoDirectories && !pCleanFile->bDirectory)
        {
            SetFileAttributes(pCleanFile->file, FILE_ATTRIBUTE_NORMAL);
            if (!DeleteFile(pCleanFile->file))
            {
                MiDebugMsg((0, TEXT("Error DeleteFile(%s) returned error %d"),
                    pCleanFile->file, GetLastError()));
            }
        }
        
        //
        //Adjust the cbSpaceFreed
        //
        _cbSpaceFreed.QuadPart+=pCleanFile->ulFileSize.QuadPart;

        //
        //Call back the cleanup manager to update the progress bar
        //
        if (picb->PurgeProgress(_cbSpaceFreed.QuadPart, (_cbSpaceUsed.QuadPart - _cbSpaceFreed.QuadPart),
            0, NULL) == E_ABORT)
        {
            //
            //User aborted so stop removing files
            //
            return;
        }

        pCleanFile = pCleanFile->pNext;
    }
}

/*
**------------------------------------------------------------------------------
** CDataDrivenCleaner::FreeList
**
** Purpose:     Frees the memory allocated by AddFileToList.
** Notes;
** Mod Log:     Created by Jason Cobb (6/97)
**------------------------------------------------------------------------------
*/
void
CDataDrivenCleaner::FreeList(
    PCLEANFILESTRUCT pCleanFile
    )
{
    if (pCleanFile == NULL)
        return;

    if (pCleanFile->pNext)
        FreeList(pCleanFile->pNext);

    HeapFree(GetProcessHeap(), 0, pCleanFile);
}

/*
**------------------------------------------------------------------------------
** FileNameFromPath
**
** Purpose:     Returns a pointer to the filename part of a full path.
**
** Notes:       
**
** Mod Log:     Created by Jason Cobb (11/97)
**------------------------------------------------------------------------------
*/
PTSTR 
FileNameFromPath(
    PTSTR pFilePath
    )
{
    PTSTR pFileName;

    pFileName = pFilePath + lstrlen(pFilePath);
    while (pFileName != pFilePath)
    {
        if ( (*pFileName == TCHAR('\\')) || (*pFileName == TCHAR(':')) )
            return pFileName+1;

        pFileName--;
    }

    return pFilePath;
}

/*
**------------------------------------------------------------------------------
** CDataDrivenCleaner::bW98ProcessIsRunning
**
** Purpose:     Checks to see if a given process name is running by using the
**              Toolhelp APIs.
**
** Notes:       This function will only work on Windows 9x since NT does not
**              support these APIs.  A similar function will be needed for NT.
**              Just the full process name (i.e. setup.exe) needs to be in the
**              registry. This function will not check extensions.
**
** Mod Log:     Created by Jason Cobb (11/97)
**------------------------------------------------------------------------------
*/
typedef BOOL (WINAPI *PROCESSWALK) (HANDLE hSnapshot, LPPROCESSENTRY32 lpee);
typedef HANDLE (WINAPI *CREATESNAPSHOT) (DWORD dwFlags, DWORD th32ProcessID);

static CREATESNAPSHOT pCreateToolhelp32Snapshot = NULL;
static PROCESSWALK pProcess32First = NULL;
static PROCESSWALK pProcess32Next = NULL;

BOOL
CDataDrivenCleaner::bW98ProcessIsRunning(
    HKEY    hRegKey
    )
{
    TCHAR szFailProcess[MAX_PATH];
    PTSTR pFailProcess;
    DWORD dwType, cbData;
    HMODULE hKernel = NULL;
    HANDLE hSnapshot;
    PROCESSENTRY32 pe32;
    BOOL bRet = FALSE;

    //
    //Get the "FailIfProcessRunning" value from the registry
    //
    dwType = REG_EXPAND_SZ;
    cbData = sizeof(szFailProcess);
    if (RegQueryValueEx(hRegKey, REGSTR_VAL_FAILIFPROCESSRUNNING, NULL, &dwType, (LPBYTE)szFailProcess, &cbData) != ERROR_SUCCESS)
        return FALSE;

    pFailProcess = szFailProcess;
    while(*pFailProcess)
    {
        if (*pFailProcess == '|')
            *pFailProcess = 0;

        pFailProcess++;
    }
    pFailProcess++;
    *pFailProcess = 0;


    //
    //Get the Toolhelp APIs that we will need
    //
    if ((hKernel = GetModuleHandle(TEXT("KERNEL32.DLL"))) == NULL)
    {
        return FALSE;
    }

    pCreateToolhelp32Snapshot = (CREATESNAPSHOT)GetProcAddress(hKernel, "CreateToolhelp32Snapshot");
    pProcess32First = (PROCESSWALK)GetProcAddress(hKernel, "Process32First");
    pProcess32Next = (PROCESSWALK)GetProcAddress(hKernel, "Process32Next");

    if (pCreateToolhelp32Snapshot == NULL ||
        pProcess32First == NULL ||
        pProcess32Next == NULL)
    {
        return FALSE;
    }

    //
    //Eunmerate all of the running processes on this machine
    //
    if ((hSnapshot = pCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)) == NULL)
    {
        return FALSE;
    }

    ZeroMemory(&pe32, sizeof(PROCESSENTRY32));
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!pProcess32First(hSnapshot, &pe32))
    {
        CloseHandle(hSnapshot);
        return FALSE;
    }

    do
    {
        //
        //Check if this process is one of the processes that should cause this
        //cleaner to fail.
        //
        for (pFailProcess = &(szFailProcess[0]); *pFailProcess; pFailProcess += (lstrlen(pFailProcess) + 1))
        {
            if(lstrcmpi(FileNameFromPath(pe32.szExeFile), pFailProcess) == 0)
            {
                MiDebugMsg((0, TEXT("Process %s is running so cleaner will not run!"),
                    FileNameFromPath(pe32.szExeFile)));
                bRet = TRUE;
                break;
            }
        }
        
    } while(pProcess32Next(hSnapshot, &pe32));
        

    CloseHandle(hSnapshot);

    return bRet;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
**------------------------------------------------------------------------------
** CDataDrivenPropBag::CDataDrivenPropBag
**
** Purpose:    Default constructor
** Notes:
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
CDataDrivenPropBag::CDataDrivenPropBag(DWORD dw)
{
    //  
    //Set to default values
    //
    _cRef         = 0L;

    _dwFilter     = dw;

    //
    //increment global object count
    //
    incDllObjectCount();
}

/*
**------------------------------------------------------------------------------
** CDataDrivenPropBag::~CDataDrivenPropBag
**
** Purpose:    Destructor
** Notes:
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
CDataDrivenPropBag::~CDataDrivenPropBag()
{
    //
    //Decrement global object count
    //
    decDllObjectCount();
}

/*
**------------------------------------------------------------------------------
** CDataDrivenPropBag::QueryInterface
**
** Purpose:    Part of the IUnknown interface
** Parameters:
**    riid  -  interface ID to query on 
**    ppv   -  pointer to interface if we support it
** Return:     NOERROR on success, E_NOINTERFACE otherwise
** Notes;
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
STDMETHODIMP CDataDrivenPropBag::QueryInterface(
   REFIID      riid, 
   LPVOID FAR *ppv
   )
{
    *ppv = NULL;

    //
    //Check for IUnknown interface request
    //
    if (IsEqualIID (riid, IID_IUnknown))
    {
        // 
        //Typecast to the requested interface so C++ sets up
        //the virtual tables correctly
        //
        *ppv = (LPUNKNOWN) this;
        AddRef();
        return NOERROR;
    }  

    
    //
    //Check for IEmptyVolumeCache interface request
    //
    if (IsEqualIID (riid, IID_IPropertyBag))
    {
        MiDebugMsg((0, TEXT("CDataDrivenPropBag::QueryInterface qi'ing for IID_IPropertyBag")));
        // 
        //Typecast to the requested interface so C++ sets up
        //the virtual tables correctly
        //
        *ppv = (IPropertyBag*) this;
        AddRef();
        return NOERROR;
    }  


    //
    //Error - unsupported interface requested
    //
    return E_NOINTERFACE;
}

/*
**------------------------------------------------------------------------------
** CDataDrivenPropBag::AddRef
**
** Purpose:    ups the reference count to this object
** Notes;
** Return:     current refernce count
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
STDMETHODIMP_(ULONG) CDataDrivenPropBag::AddRef()
{
    MiDebugMsg((0, TEXT("CDataDrivenPropBag::AddRef Ref is %d"), (_cRef+1)));

    return ++_cRef;
}

/*
**------------------------------------------------------------------------------
** CDataDrivenPropBag::Release
**
** Purpose:    downs the reference count to this object
**             and deletes the object if no one is using it
** Notes;
** Mod Log:    Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/
STDMETHODIMP_(ULONG) CDataDrivenPropBag::Release()
{
    MiDebugMsg((0, TEXT("CDataDrivenPropBag::Release Ref is %d"), (_cRef-1)));

    //  
    //Decrement and check
    //
    if (--_cRef)
        return _cRef;

    //
    //No references left to this object
    //
    delete this;

    return 0L;
}

STDMETHODIMP CDataDrivenPropBag::Read (LPCOLESTR pwszProp, VARIANT *pvar, IErrorLog *)
{
    MiDebugMsg((0, TEXT("CDataDrivenPropBag::Read")));
    if ( pvar->vt != VT_BSTR )
    {
        return E_FAIL;
    }

    DWORD dwID = 0;
    DWORD dwDisplay;
    DWORD dwDesc;
    TCHAR szBuf[MAX_PATH];
    WCHAR wszBuf[MAX_PATH];

    switch ( _dwFilter )
    {
    case ID_OLDFILESINROOTPROPBAG:
        dwDisplay = IDS_OLDFILESINROOT_DISP;
        dwDesc = IDS_OLDFILESINROOT_DESC;
        break;
    case ID_TEMPFILESPROPBAG:
        dwDisplay = IDS_TEMPFILES_DISP;
        dwDesc = IDS_TEMPFILES_DESC;
        break;
    case ID_SETUPFILESPROPBAG:
        dwDisplay = IDS_SETUPFILES_DISP;
        dwDesc = IDS_SETUPFILES_DESC;
        break;
    case ID_UNINSTALLEDFILESPROPBAG:
        dwDisplay = IDS_UNINSTALLFILES_DISP;
        dwDesc = IDS_UNINSTALLFILES_DESC;
        break;
    case ID_INDEXCLEANERPROPBAG:
        dwDisplay = IDS_INDEXERFILES_DISP;
        dwDesc = IDS_INDEXERFILES_DESC;
        break;
    default:
        return E_UNEXPECTED;
    }

    if (0 == lstrcmpiW(pwszProp, L"display"))
    {
        dwID = dwDisplay;
    }
    else if (0 == lstrcmpiW(pwszProp, L"description"))
    {
        dwID = dwDesc;
    }
    else
    {
        return E_INVALIDARG;
    }

    if ( LoadString( g_hDllModule, dwID, szBuf, MAX_PATH ) )
    {
        SHTCharToUnicode( szBuf, wszBuf, MAX_PATH );
        pvar->bstrVal = SysAllocString( wszBuf );
        if ( pvar->bstrVal )
        {
            return S_OK;
        }
    }

    return E_OUTOFMEMORY;
}

STDMETHODIMP CDataDrivenPropBag::Write (LPCOLESTR, VARIANT *)
{
    return E_NOTIMPL;
}

#ifdef WINNT
//////////////////////////////////////////////////////////////////////////////////
CContentIndexCleaner::CContentIndexCleaner(void)
{
    _cRef = 0l;
    _pDataDriven = NULL;
    incDllObjectCount();
}

CContentIndexCleaner::~CContentIndexCleaner(void)
{
    if ( _pDataDriven )
    {
        _pDataDriven->Release();
    }
    decDllObjectCount();
}

STDMETHODIMP CContentIndexCleaner::QueryInterface (REFIID riid, LPVOID FAR * ppvObj)
{
    if ( !ppvObj )
        return E_INVALIDARG;
        
    *ppvObj = NULL;
    
    if ( riid == IID_IUnknown || riid == IID_IEmptyVolumeCache )
    {
        *ppvObj = (IEmptyVolumeCache *) this;
        AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CContentIndexCleaner::AddRef (void)
{
    _cRef ++;
    return _cRef;
}

STDMETHODIMP_(ULONG) CContentIndexCleaner::Release (void)
{
    _cRef --;
    if ( !_cRef )
    {
        delete this;
        return 0;
    }
    return _cRef;
}

STDMETHODIMP CContentIndexCleaner::Initialize(
                                    HKEY hRegKey,
                                    LPCWSTR pszVolume,
                                    LPWSTR *ppszDisplayName,
                                    LPWSTR *ppszDescription,
                                    DWORD *pdwFlags
                                    )
{
    // check the volume first to see if it is in the list of cache's know about.
    // If it isn't, then we can just go ahead. If the volume is a known cache, then
    // we must check to see if the service is running...

    HKEY hkeyCatalogs;
    BOOL fFound = FALSE;

    LONG lRes = RegOpenKeyExW( HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\ContentIndex\\Catalogs", 0, KEY_READ, &hkeyCatalogs );
    if ( lRes != ERROR_SUCCESS )
    {
        return S_FALSE;
    }

    int iIndex = 0;

    do
    {
        WCHAR szBuffer[MAX_PATH];
        DWORD dwSize = sizeof( szBuffer );
        lRes = RegEnumKeyExW( hkeyCatalogs, iIndex ++, szBuffer, &dwSize, NULL, NULL, NULL, NULL );
        if ( lRes != ERROR_SUCCESS )
        {
            break;
        }

        WCHAR szData[MAX_PATH];
        dwSize = sizeof( szData );
        lRes = SHGetValueW( hkeyCatalogs, szBuffer, L"Location", NULL, szData, &dwSize );
        if ( lRes == ERROR_SUCCESS )
        {
            // check to see if it is the same volume... (two characters letter and colon)
            if (StrCmpNIW( pszVolume, szData , 2 ) == 0)
            {
                fFound = TRUE;
            }
        }

    }
    while ( TRUE );

    RegCloseKey( hkeyCatalogs );

    if ( fFound )
    {
        // check to see if the index is on or off, if the indexer is on, then we should not allow the user to blow 
        // this sucker off their hard drive...

        SC_HANDLE hSCM = OpenSCManager( NULL, NULL, GENERIC_READ | SC_MANAGER_ENUMERATE_SERVICE );
        if ( hSCM )
        {
            SC_HANDLE hCI;
            hCI = OpenService( hSCM, TEXT("cisvc"), SERVICE_QUERY_STATUS );
            if ( hCI )
            {
                SERVICE_STATUS rgSs;
                if ( QueryServiceStatus( hCI, &rgSs))
                {
                    if ( rgSs.dwCurrentState != SERVICE_RUNNING )
                        fFound = FALSE;
                }
                CloseServiceHandle( hCI );
            }
            CloseServiceHandle( hSCM );
        }

        // if it wasn't inactive, then we can't delete it...
        if ( fFound )
            return S_FALSE;
    }

    LPCDATADRIVENCLEANER pDataDrivenCleaner = new CDataDrivenCleaner;
    if ( !pDataDrivenCleaner )
        return E_OUTOFMEMORY;

    pDataDrivenCleaner->QueryInterface( IID_IEmptyVolumeCache, (LPVOID *) &_pDataDriven );

    if ( !_pDataDriven )
        return E_FAIL;

    return _pDataDriven->Initialize( hRegKey, pszVolume, ppszDisplayName, ppszDescription, pdwFlags );
}
                                    
STDMETHODIMP CContentIndexCleaner::GetSpaceUsed(
                                    DWORDLONG *pdwSpaceUsed,
                                    IEmptyVolumeCacheCallBack *picb
                                    )
{
    if ( _pDataDriven )
        return _pDataDriven->GetSpaceUsed( pdwSpaceUsed, picb );
        
    return E_FAIL;
}
                                    
STDMETHODIMP CContentIndexCleaner::Purge(
                                    DWORDLONG dwSpaceToFree,
                                    IEmptyVolumeCacheCallBack *picb
                                    )
{
    if ( _pDataDriven )
        return _pDataDriven->Purge( dwSpaceToFree, picb );
    return E_FAIL;
}
                                    
STDMETHODIMP CContentIndexCleaner::ShowProperties(
                                    HWND hwnd
                                    )
{
    if ( _pDataDriven )
        return _pDataDriven->ShowProperties( hwnd );
        
    return E_FAIL;
}
                                    
STDMETHODIMP CContentIndexCleaner::Deactivate(
                                    DWORD *pdwFlags
                                    )
{
    if ( _pDataDriven )
        return _pDataDriven->Deactivate( pdwFlags );
    return E_FAIL;
}

#endif // WINNT
/*
**------------------------------------------------------------------------------
** End of File
**------------------------------------------------------------------------------
*/
