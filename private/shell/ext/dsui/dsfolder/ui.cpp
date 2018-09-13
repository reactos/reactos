/*----------------------------------------------------------------------------
/ Title;
/   ui.cpp
/
/ Authors;
/   David De Vorchik (daviddv)
/
/ Notes;
/   Misc UI bits that are exploited by the namespace
/----------------------------------------------------------------------------*/
#include "pch.h"
#pragma hdrstop

//
// This function is supposed to ultimately shutdown COM 
// regardless of how many times CoInitialize(NULL) has
// been called.
//
void ShutdownCOM()
{
    for( ;; )
    {
        //
        // Call CoUninitialze() twice
        //
        CoUninitialize();
        CoUninitialize();

        //
        // Call CoInitialize(NULL) to see whether this will be the first
        // COM initialization. S_OK means COM is initialized successfully,
        // S_FALSE means it has been initialized already.
        //
        HRESULT hr = CoInitialize(NULL);

        if (SUCCEEDED(hr))
        {
            // S_OK, S_FALSE case
            if (S_OK == hr)
            {
                CoUninitialize();
                break;
            }
            else
            {
                // The internal COM ref count
                // still hasn't reached zero
                continue;
            }
        }
        else
        {
            // RPC_E_CHANGED_MODE case
            if (RPC_E_CHANGED_MODE == hr)
            {
                continue;
            }
            else
            {
                // Some other failure
                // E_OUTOFMEMORY for example
                break;
            }
        }
    }
}

/*-----------------------------------------------------------------------------
/ Globals etc used for icon extraction
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ GetKeysForIdList
/ ----------------
/   Given an IDLIST (packed or unpacked) attempt to get return the registry
/   keys that reflect that class of object.
/
/ In:
/   pidl / pData = packed/unpacked DS object information
/   cKeys = number of keys to fetch
/   aKeys = array to be filled
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT GetKeysForIdList(LPCITEMIDLIST pidl, LPIDLISTDATA pData, INT cKeys, HKEY* aKeys)
{
    HRESULT hres;
    IDLISTDATA data;

    TraceEnter(TRACE_UI, "GetKeysForIdList");

    if ( !pData )
    {
        if ( !pidl )
            ExitGracefully(hres, E_FAIL, "Must had IDLIST to expand");

        hres = UnpackIdList(ILFindLastID(pidl), DSIDL_HASCLASS, &data);
        FailGracefully(hres, "Failed to unpack IDLIST to a data structure");
        pData = &data;
    }

    hres = GetKeysForClass(pData->pObjectClass, (pData->dwFlags & DSIDL_ISCONTAINER), cKeys, aKeys);
    FailGracefully(hres, "GetKeysForClass failed");

    //hres = S_OK;          // success

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ GetKeysForClass
/ ---------------
/   Given a class and the flags assocaited with that extract the keys that
/   represent it.
/
/ In:
/   pObjectClass = class name to fetch keys for
/   fIsConatiner = object is a container
/   cKeys = number of keys to fetch
/   aKeys = array to be filled with keys
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT GetKeysForClass(LPWSTR pObjectClass, BOOL fIsContainer, INT cKeys, HKEY* aKeys)
{
    HRESULT hres;
    HKEY hkClasses = NULL;
    CLSID clsidBase;
    LPTSTR pMappedClass = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_UI, "GetKeysForClass");

    if ( cKeys < UIKEY_MAX )
        ExitGracefully(hres, E_INVALIDARG, "cKeys < UIKEY_MAX");

    ZeroMemory(aKeys, SIZEOF(HKEY)*cKeys);

    hres = GetKeyForCLSID(CLSID_MicrosoftDS, c_szClasses, &hkClasses);
    FailGracefully(hres, "Failed to get Classes key from registry");

    //
    // Attempt to look up the class name in the registery under the namespaces "classes"
    // sub key.  A class can also be mapped onto another one, if this happens then we have
    // a base CLASS key which we indirect via
    // 

    if ( pObjectClass )
    {
        if ( ERROR_SUCCESS == RegOpenKeyEx(hkClasses, W2T(pObjectClass), NULL, KEY_READ, &aKeys[UIKEY_CLASS]) )
        {
            if ( SUCCEEDED(LocalQueryString(&pMappedClass, aKeys[UIKEY_CLASS], c_szClass)) )
            {
                if ( ERROR_SUCCESS != RegOpenKeyEx(hkClasses, pMappedClass, NULL, KEY_READ, &aKeys[UIKEY_BASECLASS]) )
                {
                    aKeys[UIKEY_BASECLASS] = NULL;
                }
            }
        }
    }

    //
    // Finally we need the root class (container or object)
    //

    hres =  GetKeyForCLSID(CLSID_MicrosoftDS, (fIsContainer) ? c_szAllContainers:c_szAllObjects, &aKeys[UIKEY_ROOT]);
    FailGracefully(hres, "Failed to get root key");

    // hres = S_OK;          // success

exit_gracefully:

    LocalFreeString(&pMappedClass);

    if ( hkClasses )
        RegCloseKey(hkClasses);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ TidyKeys
/ --------
/   Given an array of keys release them and set them back to zero.
/
/ In:
/   cKeys = number of keys in array
/   aKeys = keys to be released
/
/ Out:
/   void
/----------------------------------------------------------------------------*/
void TidyKeys(INT cKeys, HKEY* aKeys)
{
    TraceEnter(TRACE_UI, "TidyKeys");

    while ( --cKeys >= 0 )
    {
        if ( aKeys[cKeys] )
        {
            RegCloseKey(aKeys[cKeys]);
            aKeys[cKeys] = NULL;            // key now closed
        }
    }

    TraceLeaveVoid();
}


/*-----------------------------------------------------------------------------
/ ShowObjectProperties
/ --------------------
/   Display properties for the given objects.  This we do by invoking
/   the tab collector for the given IDataObject.  First however we
/   look inside the object and see if we can find out what objects
/   were selected, having done that we can then find the HKEYs
/
/ In:
/   hwndParent = parent dialog
/   pDataObject = data object that we must use
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT _OverrideProperties(HWND hwndParent, LPDATAOBJECT pDataObject, HKEY* aKeys, INT cKeys, LPCWSTR pPrefix)
{
    HRESULT hres;
    LPTSTR pPropertiesGUID = NULL;
    GUID guidProperties;
    IDsFolderProperties* pDsFolderProperties = NULL;
    TCHAR szBuffer[MAX_PATH];
    INT i;
    USES_CONVERSION;

    TraceEnter(TRACE_UI, "_OverrideProperties");

    // walk all the keys we were given, some will be NULL so ignore those

    StrCpy(szBuffer, W2CT(pPrefix));
    StrCat(szBuffer, c_szDsPropertyUI);

    Trace(TEXT("Prefixed property handler value: %s"), szBuffer);

    for ( i = 0 ; i < cKeys ; i++ )
    {
        LocalFreeString(&pPropertiesGUID);

        if ( aKeys[i] )
        {
            // if we have a handle attempt to get the GUID string from the registry
            // and convert it to a GUID so that we can call CoCreateInstance for the
            // IDsFolderProperites interface.

            if ( FAILED(LocalQueryString(&pPropertiesGUID, aKeys[i], szBuffer)) )
            {
                TraceMsg("Trying non-prefixed property handler");

                if ( FAILED(LocalQueryString(&pPropertiesGUID, aKeys[i], c_szDsPropertyUI)) )
                    continue;
            }

            Trace(TEXT("GUID is: %s"), pPropertiesGUID);

            if ( !GetGUIDFromString(pPropertiesGUID, &guidProperties) )
            {
                TraceMsg("Failed to parse GUID");
                continue;
            }

            if ( SUCCEEDED(CoCreateInstance(guidProperties, NULL, CLSCTX_INPROC_SERVER, 
                                            IID_IDsFolderProperties, (LPVOID*)&pDsFolderProperties)) )
            {
                TraceMsg("Calling IDsFolderProperties::ShowProperties");                    

                hres = pDsFolderProperties->ShowProperties(hwndParent, pDataObject);
                FailGracefully(hres, "Failed when calling ShowProperties");            

                goto exit_gracefully;
            }
        }   
    }

    hres = S_FALSE;               // S_FALSE indicates that the caller should display properties

exit_gracefully:

    LocalFreeString(&pPropertiesGUID);
    DoRelease(pDsFolderProperties);

    TraceLeaveResult(hres);
}

typedef struct
{
    HWND hwndParent;
    IStream* pStream;
} PROPERTIESTHREADDATA;

DWORD WINAPI _ShowObjectPropertiesThread(LPVOID lpParam)
{
    HRESULT hres;
    PROPERTIESTHREADDATA* pThreadData = (PROPERTIESTHREADDATA*)lpParam;
    IADsPathname* pPathname = NULL;
    IDataObject* pDataObject = NULL;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM mediumNames = { TYMED_NULL, NULL, NULL };
    STGMEDIUM mediumOptions = { TYMED_NULL, NULL, NULL };
    LPDSOBJECTNAMES pDsObjects;
    LPDSDISPLAYSPECOPTIONS pDispSpecOptions;
    HKEY hKeys[3] = { NULL, NULL, NULL };
    LPTSTR pTitle = NULL;
    LPCWSTR pPrefix = DS_PROP_SHELL_PREFIX;
    BSTR bstrName = NULL;
    INT i;
    USES_CONVERSION;

    TraceEnter(TRACE_UI, "ShowObjectPropertiesThread");

    CoInitialize(NULL);
    RegisterDsClipboardFormats();

    hres = CoGetInterfaceAndReleaseStream(pThreadData->pStream, IID_IDataObject, (void**)&pDataObject);
    FailGracefully(hres, "Failed to get data object from stream");       
   
    // get the object names that we are showing properites on

    fmte.cfFormat = g_cfDsObjectNames;          
    hres = pDataObject->GetData(&fmte, &mediumNames);
    FailGracefully(hres, "Failed to get selected objects");

    pDsObjects = (LPDSOBJECTNAMES)mediumNames.hGlobal;

    // get the attribute prefix, use this to key information from the registry

    fmte.cfFormat = g_cfDsDispSpecOptions;
    if ( SUCCEEDED(pDataObject->GetData(&fmte, &mediumOptions)) )
    {
        pDispSpecOptions = (LPDSDISPLAYSPECOPTIONS)mediumOptions.hGlobal;
        pPrefix = (LPCWSTR)ByteOffset(pDispSpecOptions, pDispSpecOptions->offsetAttribPrefix);
    }

    Trace(TEXT("Attribute prefix is: %s"), W2CT(pPrefix));

    if ( pDsObjects && (pDsObjects->cItems >= 1) )
    {
        LPWSTR pPath = (LPWSTR)ByteOffset(pDsObjects, pDsObjects->aObjects[0].offsetName);
        LPWSTR pObjectClass = (LPWSTR)ByteOffset(pDsObjects, pDsObjects->aObjects[0].offsetClass);
        BOOL fSelection = ( pDsObjects->cItems > 1 );

        Trace(TEXT("Items %d, 1st object: %s, 1st Class: %s"), pDsObjects->cItems, W2T(pPath), W2T(pObjectClass));

        // attempt to pick up the keys for the first element in the selection and get
        // the keys that map to that object

        hres = GetKeysForClass(pObjectClass,
                             (pDsObjects->aObjects[0].dwFlags & DSOBJECT_ISCONTAINER), 
                             ARRAYSIZE(hKeys), hKeys);

        FailGracefully(hres, "Failed to get keys for class");       

        hres = _OverrideProperties(pThreadData->hwndParent, pDataObject, hKeys, ARRAYSIZE(hKeys), pPrefix);
        FailGracefully(hres, "Failed when trying to call out for properties");

        // if the caller returns S_FALSE then we assume that they want us to display 
        // the property pages for the given selection, so do so.

        if ( hres == S_FALSE )
        {
            hres = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER, IID_IADsPathname, (LPVOID*)&pPathname);
            FailGracefully(hres, "Failed to get the IADsPathname interface");

            hres = pPathname->Set(pPath, ADS_SETTYPE_FULL);
            FailGracefully(hres, "Failed to set the path of the name");

            pPathname->SetDisplayType(ADS_DISPLAY_VALUE_ONLY);

            hres = pPathname->Retrieve(ADS_FORMAT_LEAF, &bstrName);
            FailGracefully(hres, "Failed to get the leaf element");

            hres = FormatMsgResource(&pTitle, 
                                    GLOBAL_HINSTANCE, (fSelection) ? IDS_LARGESEL:IDS_SINGLESEL, 
                                    W2T(bstrName));
            
            FailGracefully(hres, "Failed to format dialog title");

            if ( !SHOpenPropSheet(pTitle, hKeys, ARRAYSIZE(hKeys), NULL, pDataObject, NULL, NULL) )
                ExitGracefully(hres, E_FAIL, "Failed to open property pages");
        }
    }

    hres = S_OK;

exit_gracefully:

    ReleaseStgMedium(&mediumNames);
    ReleaseStgMedium(&mediumOptions);
    TidyKeys(ARRAYSIZE(hKeys), hKeys);

    LocalFreeString(&pTitle);
    SysFreeString(bstrName);

    DoRelease(pPathname);
    DoRelease(pDataObject);

    LocalFree(pThreadData);

#ifdef WINNT

    //
    // We need to shutdwn COM ultimately here as we don't 
    // know how many times CoInitialize(NULL) has been called.
    // Otherwise COM is trying to shutdown itself inside compobj!DllMain,
    // while holding the loader lock which  is causing a very bad deadlock.
    // For more information see bug #395293.
    //
    // However we need to brace this code as NT specific code because 
    // otherwise it may cause problems with the Win9x DSUI client for
    // some weird reason.
    //
    ShutdownCOM();

#endif // WINNT

    TraceLeave();

    InterlockedDecrement(&GLOBAL_REFCOUNT);
    ExitThread(0);
    return 0;
}

HRESULT ShowObjectProperties(HWND hwndParent, LPDATAOBJECT pDataObject)
{
    HRESULT hres;
    PROPERTIESTHREADDATA* pThreadData;
    DWORD dwId;
    HANDLE hThread;

    TraceEnter(TRACE_UI, "ShowObjectProperties");

    // Allocate thread data for the new object we are launching

    pThreadData = (PROPERTIESTHREADDATA*)LocalAlloc(LPTR, SIZEOF(PROPERTIESTHREADDATA));
    TraceAssert(pThreadData);

    if ( !pThreadData )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate thread data");

    // we have thread data lets fill it and spin the properties thread.

    pThreadData->hwndParent = hwndParent;

    hres = CoMarshalInterThreadInterfaceInStream(IID_IDataObject, pDataObject, &(pThreadData->pStream));
    FailGracefully(hres, "Failed to create marshaling object");

    InterlockedIncrement(&GLOBAL_REFCOUNT);

    hThread = CreateThread(NULL, 0, _ShowObjectPropertiesThread, (LPVOID)pThreadData, 0, &dwId);
    TraceAssert(hThread);

    if ( !hThread )
    {
        LocalFree(pThreadData);
        InterlockedDecrement(&GLOBAL_REFCOUNT);
        ExitGracefully(hres, E_UNEXPECTED, "Failed to kick off the thread");
    }

    CloseHandle(hThread);
    hres = S_OK;

exit_gracefully:
    
    TraceLeaveResult(hres);
}
