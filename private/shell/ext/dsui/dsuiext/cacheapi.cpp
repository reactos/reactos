#include "pch.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Helper functions used by all
/----------------------------------------------------------------------------*/

//
// Given a cache entry return a BOOL indicating if the class is really a container
// or not, we find this from both the schema and the display specifier.
//

BOOL _IsClassContainer(LPCLASSCACHEENTRY pClassCacheEntry, BOOL fIgnoreTreatAsLeaf)
{
    BOOL fClassIsContainer = FALSE;

    TraceEnter(TRACE_CACHE, "_IsClassContainer");

    // default to the treat as leaf flag, note that this is always
    // valid as it defaults to the schema value if it is not defined
    // in the display specifier.

    Trace(TEXT("fIsContainer is %scached and is %d"), 
                    pClassCacheEntry->dwCached & CLASSCACHE_CONTAINER ? TEXT(""):TEXT("not "),
                    pClassCacheEntry->fIsContainer);

    Trace(TEXT("fTreatAsLeaf is %scached and is %d"), 
                    pClassCacheEntry->dwCached & CLASSCACHE_TREATASLEAF ? TEXT(""):TEXT("not "),
                    pClassCacheEntry->fTreatAsLeaf);

    if ( !(pClassCacheEntry->dwCached & (CLASSCACHE_CONTAINER|CLASSCACHE_TREATASLEAF)) )
    {
        TraceMsg("Neither container or treat as leaf is cached, therefore returning");
        fClassIsContainer = TRUE;
        goto exit_gracefully;
    }

    if ( fIgnoreTreatAsLeaf )
    {
        if ( !(pClassCacheEntry->dwCached & CLASSCACHE_CONTAINER) )
        {
            TraceMsg("Object doesn't have the container flag cached");
            goto exit_gracefully;
        }

        fClassIsContainer = pClassCacheEntry->fIsContainer;
        goto exit_gracefully;
    }

    if ( !(pClassCacheEntry->dwCached & CLASSCACHE_TREATASLEAF) )
    {
        if ( !(pClassCacheEntry->dwCached & CLASSCACHE_CONTAINER) )
        {
            TraceMsg("Object doesn't have the treat as leaf flag cached");
            goto exit_gracefully;
        }

        fClassIsContainer = pClassCacheEntry->fIsContainer;
        goto exit_gracefully;
    }

    fClassIsContainer = pClassCacheEntry->fTreatAsLeaf;

exit_gracefully:

    TraceLeaveValue(fClassIsContainer);
}


/*-----------------------------------------------------------------------------
/ COM API's exposed for accessing display specifiers.
/----------------------------------------------------------------------------*/

class CDsDisplaySpecifier : public IDsDisplaySpecifier, CUnknown
{
private:
    DWORD _dwFlags;
    LPWSTR _pszServer;
    LPWSTR _pszUserName;
    LPWSTR _pszPassword;
    LANGID _langid;

    HRESULT _GetClassCacheInfo(LPCWSTR pszClassName, LPCWSTR pszADsPath, DWORD dwFlags, CLASSCACHEENTRY **ppcce);

public:
    CDsDisplaySpecifier();
    ~CDsDisplaySpecifier();

    // *** IUnknown ***
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR *ppv);

    // *** IDsDisplaySpecifier ***
    STDMETHOD(SetServer)(LPCWSTR pszServer, LPCWSTR pszUserName, LPCWSTR pszPassword, DWORD dwFlags);
    STDMETHOD(SetLanguageID)(LANGID langid);
    STDMETHOD(GetDisplaySpecifier)(LPCWSTR pszObjectClass, REFIID riid, void **ppv);
    STDMETHOD(GetIconLocation)(LPCWSTR pszObjectClass, DWORD dwFlags, LPWSTR pszBuffer, INT cchBuffer, INT *presid);
    STDMETHOD_(HICON, GetIcon)(LPCWSTR pszObjectClass, DWORD dwFlags, INT cxIcon, INT cyIcon);
    STDMETHOD(GetFriendlyClassName)(LPCWSTR pszObjectClass, LPWSTR pszBuffer, INT cchBuffer);
    STDMETHOD(GetFriendlyAttributeName)(LPCWSTR pszObjectClass, LPCWSTR pszAttributeName, LPWSTR pszBuffer, UINT cchBuffer);
    STDMETHOD_(BOOL, IsClassContainer)(LPCWSTR pszObjectClass, LPCWSTR pszADsPath, DWORD dwFlags);
    STDMETHOD(GetClassCreationInfo)(LPCWSTR pszObjectClass, LPDSCLASSCREATIONINFO* ppdscci);
    STDMETHOD(EnumClassAttributes)(LPCWSTR pszObjectClass, LPDSENUMATTRIBUTES pcbEnum, LPARAM lParam);
    STDMETHOD_(ADSTYPE, GetAttributeADsType)(LPCWSTR pszAttributeName);
};

//
// construction/destruction 
//

CDsDisplaySpecifier::CDsDisplaySpecifier() :
    _dwFlags(0),
    _pszServer(NULL),
    _pszUserName(NULL),
    _pszPassword(NULL),
    _langid(GetUserDefaultUILanguage())
{
}

CDsDisplaySpecifier::~CDsDisplaySpecifier()
{
    LocalFreeStringW(&_pszServer);
    LocalFreeStringW(&_pszUserName);
    LocalFreeStringW(&_pszPassword);
}


//
// IUnknown
//

#undef CLASS_NAME
#define CLASS_NAME CDsDisplaySpecifier
#include "unknown.inc"

STDMETHODIMP CDsDisplaySpecifier::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[] =
    {
        &IID_IDsDisplaySpecifier, (IDsDisplaySpecifier *)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}

//
// handle create instance
//

STDAPI CDsDisplaySpecifier_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CDsDisplaySpecifier *pdds = new CDsDisplaySpecifier();
    if ( !pdds )
        return E_OUTOFMEMORY;

    HRESULT hres = pdds->QueryInterface(IID_IUnknown, (void **)ppunk);
    pdds->Release();
    return hres;
}


//
// Class cache helper functions
//

HRESULT CDsDisplaySpecifier::_GetClassCacheInfo(LPCWSTR pszObjectClass, LPCWSTR pszADsPath, 
                                                        DWORD dwFlags, CLASSCACHEENTRY **ppcce)
{
    CLASSCACHEGETINFO ccgi = { 0 };

    ccgi.dwFlags = dwFlags;
    ccgi.pObjectClass = (LPWSTR)pszObjectClass;
    ccgi.pPath = (LPWSTR)pszADsPath;
    ccgi.pServer = _pszServer;
    ccgi.pUserName = _pszUserName;
    ccgi.pPassword = _pszPassword;

    if ( _dwFlags & DSSSF_SIMPLEAUTHENTICATE )
        ccgi.dwFlags |= CLASSCACHE_SIMPLEAUTHENTICATE;

    if ( _dwFlags & DSSSF_DSAVAILABLE )
        ccgi.dwFlags |= CLASSCACHE_DSAVAILABLE;
    
    return ClassCache_GetClassInfo(&ccgi, ppcce);
}


/*-----------------------------------------------------------------------------
/ IDsDisplaySpecifier::SetServer
/ ------------------------------
/   To allow us to re-target other servers in the domains we allow the
/   owner of an IDsDisplaySpecifier object to set the prefered server,
/   this consists of the server name, the user name and the password.
/
/   BUGBUG: Currently the password is stored clear text with the object,
/           this should be fixed - daviddv (10oct98)
/
/ In:
/   pServer => server to use
/   pUserName => user name to be used
/   pPassword => password to be used
/   dwFlags => flags for this call
/
/ Out:
    HRESULT
/----------------------------------------------------------------------------*/
STDMETHODIMP CDsDisplaySpecifier::SetServer(LPCWSTR pszServer, LPCWSTR pszUserName, LPCWSTR pszPassword, DWORD dwFlags)
{
    HRESULT hres = S_OK;
    USES_CONVERSION;

    TraceEnter(TRACE_CACHE, "CDsDisplaySpecifier::SetServer");
    Trace(TEXT("pszServer %s"), pszServer ? W2CT(pszServer):TEXT("<none>"));
    Trace(TEXT("pszUserName %s"), pszUserName ? W2CT(pszUserName):TEXT("<none>"));
    Trace(TEXT("pszPassword %s"), pszPassword ? W2CT(pszPassword):TEXT("<none>"));

    // free previous credential information

    LocalFreeStringW(&_pszServer);
    LocalFreeStringW(&_pszUserName);
    LocalFreeStringW(&_pszPassword);

    // allocate as required the new ones

    _dwFlags = dwFlags;

    hres = LocalAllocStringW(&_pszServer, pszServer);
    if ( SUCCEEDED(hres) )
        hres = LocalAllocStringW(&_pszUserName, pszUserName);
    if ( SUCCEEDED(hres) )
        hres = LocalAllocStringW(&_pszPassword, pszPassword);

    // and tidy up if we failed

    if ( FAILED(hres ) )
    {
        LocalFreeStringW(&_pszServer);
        LocalFreeStringW(&_pszUserName);
        LocalFreeStringW(&_pszPassword);
    }

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ IDsDisplaySpecifier::SetLanguageID
/ ----------------------------------
/   Display specifiers are localised, by default we use the process locale
/   read from GetLocale during object creation.  This call allows the
/   locale to be set.
/
/ In:
/   langid == LANGID to be used for display specifier look up.  If this
/             value is zero then we read using GetUserDefaultUILanguage() and set
/             accordingly.
/ Out:
    HRESULT
/----------------------------------------------------------------------------*/
STDMETHODIMP CDsDisplaySpecifier::SetLanguageID(LANGID langid)
{
    TraceEnter(TRACE_CACHE, "CDsDisplaySpecifier::SetLanguageID");
    Trace(TEXT("lcid %0x8"), langid);

    if ( !langid )
        langid = GetUserDefaultUILanguage();

    _langid = langid;                           // can hardly go wrong...

    TraceLeaveResult(S_OK);
}


/*-----------------------------------------------------------------------------
/ IDsDisplaySpecifier::GetDisplaySpecifier
/ ----------------------------------------
/   Bind to the display specifier for a given class, try the users
/   locale, then the default locale calling ADsOpenObject as we
/   go.  We use the specifier server, username and password.
/
/ In:
/   pszObjectClass => object class to look up
/   riid, ppv => used to retrieve the COM object
/
/ Out:
    HRESULT
/----------------------------------------------------------------------------*/
STDMETHODIMP CDsDisplaySpecifier::GetDisplaySpecifier(LPCWSTR pszObjectClass, REFIID riid, void **ppv)
{
    HRESULT hres;
    CLASSCACHEGETINFO ccgi = { 0 };
    USES_CONVERSION;

    TraceEnter(TRACE_CACHE, "CDsDisplaySpecifier::GetDisplaySpecifer");  
    Trace(TEXT("pszObjectClass: %s"), pszObjectClass ? W2CT(pszObjectClass):TEXT("<none>"));

    // fill out the display specifier record

    ccgi.pObjectClass = (LPWSTR)pszObjectClass;
    ccgi.pServer = (LPWSTR)_pszServer;
    ccgi.pUserName = (LPWSTR)_pszUserName;
    ccgi.pPassword = (LPWSTR)_pszPassword;
    ccgi.langid = _langid;

    hres = ::GetDisplaySpecifier(&ccgi, riid, ppv);
    FailGracefully(hres, "Failed when calling GetDisplaySpecifier");

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ IDsDisplaySpecifier::GetFriendlyClassName
/ -----------------------------------------
/   Retrieve the localised (friendly) name for an LDAP object class.  If
/   the display specifier doesn't give a friendly name for the class
/   then we return the name we were originally given.
/
/ In:
/   pszObjectClass => object class to look up
/   pszBuffer, cchBuffer = buffer to recieve the string
/
/ Out:
    HRESULT
/----------------------------------------------------------------------------*/
STDMETHODIMP CDsDisplaySpecifier::GetFriendlyClassName(LPCWSTR pszObjectClass, LPWSTR pszBuffer, INT cchBuffer)
{
    HRESULT hres;
    LPCLASSCACHEENTRY pcce = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_CACHE, "CDsDisplaySpecifier::GetFriendlyClassName");
    
    if ( !pszObjectClass || !pszBuffer )
        ExitGracefully(hres, E_INVALIDARG, "No class, or no buffer failure");

    Trace(TEXT("pszObjectClass: %s"), W2CT(pszObjectClass));

    // fetch a record from the cache, if we found it then set pszObjectClass
    // to be the friendly class name, otherwise we just return the class
    // name we were given.

    hres = _GetClassCacheInfo(pszObjectClass, NULL, CLASSCACHE_FRIENDLYNAME, &pcce);
    FailGracefully(hres, "Failed to get class information from cache");

    if ( pcce->dwCached & CLASSCACHE_FRIENDLYNAME)
    {
        Trace(TEXT("Friendly class name: %s"), W2CT(pcce->pFriendlyClassName));
        pszObjectClass = pcce->pFriendlyClassName;
    }

    StrCpyNW(pszBuffer, pszObjectClass, cchBuffer);
    hres = S_OK;

exit_gracefully:

    ClassCache_ReleaseClassInfo(&pcce);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ IDsDisplaySpecifier::GetFriendlyAttributeName
/ ---------------------------------------------
/   Lookup the classes display speifier, then check the attributeNames property
/   for a property name pair that matches the given attribute name.  With
/   this information return that name to the caller, if that fails then
/   return the original name.
/
/ In:
/   pszObjectClass -> class name to look up in the cache
/   pszAttributeName -> attribute name to look up in the cache
/   pszBuffer -> buffer to be filled
/   cchBuffer = size of the buffer 
/
/ Out:
    HRESULT
/----------------------------------------------------------------------------*/
STDMETHODIMP CDsDisplaySpecifier::GetFriendlyAttributeName(LPCWSTR pszObjectClass, LPCWSTR pszAttributeName, LPWSTR pszBuffer, UINT cchBuffer)
{
    HRESULT hres;
    LPCLASSCACHEENTRY pcce = NULL;
    INT index;
    USES_CONVERSION;

    TraceEnter(TRACE_CACHE, "DsGetFriendlyAttributeName");
   
    if ( !pszObjectClass || !pszAttributeName || !pszBuffer || !cchBuffer )
        ExitGracefully(hres, E_INVALIDARG, "Bad class/attribute/return buffer");

    Trace(TEXT("pszbjectClass: %s"), W2CT(pszObjectClass));
    Trace(TEXT("pszAttributeName: %s"), W2CT(pszAttributeName));
    Trace(TEXT("pszBuffer %x, cchBuffer %d"), pszBuffer, cchBuffer);

    hres = _GetClassCacheInfo(pszObjectClass, NULL, CLASSCACHE_ATTRIBUTENAMES, &pcce);
    FailGracefully(hres, "Failed to get class information from cache");

    if ( pcce->dwCached & CLASSCACHE_ATTRIBUTENAMES )
    {
        ATTRIBUTENAME an = { 0 };
        an.pName = (LPWSTR)pszAttributeName;

        index = DPA_Search(pcce->hdpaAttributeNames, &an, 0, _CompareAttributeNameCB, NULL, DPAS_SORTED);
        if ( index != -1 )
        {
            LPATTRIBUTENAME pAN = (LPATTRIBUTENAME)DPA_GetPtr(pcce->hdpaAttributeNames, index);
            TraceAssert(pAN);

            pszAttributeName = pAN->pDisplayName;
            TraceAssert(pszAttributeName);
        }        
    }

    StrCpyNW(pszBuffer, pszAttributeName, cchBuffer);
    hres = S_OK;

exit_gracefully:

    ClassCache_ReleaseClassInfo(&pcce);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ IDsDisplaySpecifier::IsClassContainer
/ -------------------------------------
/   Return TRUE/FALSE indicating if the specified object class is a container,
/   we determine this both from the schema and the display specifier.
/
/   The schema indicates if the class can container other objects, if so
/   then the object is a container.   In the display specifier we have
/   an attribute "treatAsLeaf" which we use to override this setting, this
/   is used both from the admin tools and client UI.
/
/ In:
/   pszObjectClass => object class to look up
/   pszADsPath => ADsPath of an object in the DS we can bind to and fetch
/                 schema information from.  
/   dwFlags => flags controlling this API:
/       DSICCF_IGNORETREATASLEAF = 1 => return schema attribute only, don't
/                                       override with treatAsLeaf attribute
/                                       from display specifier.
/ Out:
    BOOL
/----------------------------------------------------------------------------*/
STDMETHODIMP_(BOOL) CDsDisplaySpecifier::IsClassContainer(LPCWSTR pszObjectClass, LPCWSTR pszADsPath, DWORD dwFlags)
{
    HRESULT hres;
    BOOL fres = FALSE;
    LPCLASSCACHEENTRY pcce = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_CACHE, "CDsDisplaySpecifier::IsClassContainer");

    if ( !pszObjectClass )
        ExitGracefully(hres, E_INVALIDARG, "No object class failure");

    Trace(TEXT("pszObjectClass: %s"), W2CT(pszObjectClass));
    Trace(TEXT("dwFlags %x"), dwFlags);

    hres = _GetClassCacheInfo(pszObjectClass,pszADsPath, CLASSCACHE_CONTAINER|CLASSCACHE_TREATASLEAF, &pcce);
    FailGracefully(hres, "Failed to get class information from cache");
    
    fres = _IsClassContainer(pcce, dwFlags & DSICCF_IGNORETREATASLEAF);
    Trace(TEXT("_IsClassContainer returns %d"), fres);

exit_gracefully:

    ClassCache_ReleaseClassInfo(&pcce);

    TraceLeaveValue(fres);
}


/*-----------------------------------------------------------------------------
/ IDsDisplaySpecifier::GetIconLocation
/ ------------------------------------
/   Fetch the location of an icon from the DS, returning both the filename and
/   the resource ID as required.   The caller can then load the image, or
/   display this information in a dialog.
/
/ In:
/   pszObjectClass => class to retrieve for
/   dwFlags = flags for extraction:
/
/     One of the following:
/       DSGIF_ISNORMAL => standard icon, or,
/       DSGIF_OPEN => open icon (open folders etc), or,
/       DSGIF_DISABLED => disabled icon (eg. disabled user account).
/
/     Combined with any of the:
/       DSGIF_GETDEFAULTICON => if no icon exists for this object, return the default document
/                               icon from shell32.
/
/   pszBuffer, cchBuffer => buffer to recieve the filename
/   presid => receives the resource id, +ve for index, -ve for resource
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
STDMETHODIMP CDsDisplaySpecifier::GetIconLocation(LPCWSTR pszObjectClass, DWORD dwFlags, LPWSTR pszBuffer, INT cchBuffer, INT* presid)
{
    HRESULT hres;
    LPCLASSCACHEENTRY pcce = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_CACHE, "CDsDisplaySpecifier::GetIconLocation");

    if ( !pszObjectClass || !pszBuffer )
        ExitGracefully(hres, E_INVALIDARG, "No object class/buffer failure");

    Trace(TEXT("pszObjectClass: %s"), W2CT(pszObjectClass));
    Trace(TEXT("dwFlags %x"), dwFlags);

    hres = _GetClassCacheInfo(pszObjectClass, NULL, CLASSCACHE_ICONS, &pcce);
    FailGracefully(hres, "Failed to get class information from cache");

    hres = _GetIconLocation(pcce, dwFlags, pszBuffer, cchBuffer, presid);
    FailGracefully(hres, "Failed calling GetIconLocation");

exit_gracefully:

    ClassCache_ReleaseClassInfo(&pcce);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ IDsDisplaySpecifier::GetIcon
/ ----------------------------
/   Load the icon for the object class given.  Icon information is stored in the
/   display specifier, we support 15 different states (open, closed, disabled etc).
/
/   We look up the resource name from the DS and we then call PrivateExtractIcons
/   to load the object from the file.
/
/ In:
/   pszObjectClass => class to retrieve for
/   dwFlags = flags for extraction:
/
/     One of the following:
/       DSGIF_ISNORMAL => standard icon, or,
/       DSGIF_OPEN => open icon (open folders etc), or,
/       DSGIF_DISABLED => disabled icon (eg. disabled user account).
/
/     Combined with any of the:
/       DSGIF_GETDEFAULTICON => if no icon exists for this object, return the default document
/                               icon from shell32.
/       
/   cxImage, cyImage = size of image to load
/
/ Out:
/   HICON / == NULL if failed
/----------------------------------------------------------------------------*/
STDMETHODIMP_(HICON) CDsDisplaySpecifier::GetIcon(LPCWSTR pszObjectClass, DWORD dwFlags, INT cxImage, INT cyImage)
{
    HRESULT hres;
    HICON hIcon = NULL;
    WCHAR szBuffer[MAX_PATH];
    INT resid;
    USES_CONVERSION;

    TraceEnter(TRACE_CACHE, "CDsDisplaySpecifier::GetIcon");
    
    if ( !pszObjectClass )
        ExitGracefully(hres, E_INVALIDARG, "no object class specified");

    Trace(TEXT("pszObjectClass %s, dwFlags %x, cxImage %d, cyImage %d"), W2CT(pszObjectClass), dwFlags, cxImage, cyImage);

    hres = GetIconLocation(pszObjectClass, dwFlags, szBuffer, ARRAYSIZE(szBuffer), &resid);
    FailGracefully(hres, "Failed when calling GetIconLocation");

    if ( hres == S_OK )
    {
        Trace(TEXT("Calling PrivateExtractIcons on %s,%d"), W2CT(szBuffer), resid);

        if ( 1 != PrivateExtractIcons(szBuffer, resid, cxImage, cyImage, &hIcon, NULL, 1, LR_LOADFROMFILE) )
            ExitGracefully(hres, E_FAIL, "Failed to load the icon given its path etc");

        hres = S_OK;                    // success
    }

exit_gracefully:

    if ( !hIcon && (dwFlags & DSGIF_GETDEFAULTICON) )
    {
        //
        // failed to load the icon and they really want the default document, so give it to them
        //

        TraceMsg("Failed to load the icon, so picking up default document image");

        if ( 1 != PrivateExtractIcons(L"shell32.dll", -1, cxImage, cyImage, &hIcon, NULL, 1, LR_LOADFROMFILE) )
        {
            TraceMsg("Failed to load the default document icon from shell32");
        }
    }

    TraceLeaveValue(hIcon);
}


/*-----------------------------------------------------------------------------
/ IDsDisplaySpecifier::GetClassCreationInfo
/ -----------------------------------------
/   Given an object class return the CLSIDs of the objects that make up
/   its creation wizard.
/
/ In:
/   pszObjectClass -> class to enumerate from
/   ppdscci -> DSCREATECLASSINFO structure pointer to fill
/
/ Out:
    HRESULT
/----------------------------------------------------------------------------*/
STDMETHODIMP CDsDisplaySpecifier::GetClassCreationInfo(LPCWSTR pszObjectClass, LPDSCLASSCREATIONINFO* ppdscci)
{
    HRESULT hres;
    LPDSCLASSCREATIONINFO pdscci = NULL;
    LPCLASSCACHEENTRY pcce = NULL;
    DWORD cbStruct = SIZEOF(DSCLASSCREATIONINFO);
    INT i;
    USES_CONVERSION;

    TraceEnter(TRACE_CACHE, "CDsDisplaySpecifer::GetClassCreationInfo");

    if ( !pszObjectClass || !ppdscci )
        ExitGracefully(hres, E_INVALIDARG, "No object class/pdscci passed");
    
    // call the caching code to retrieve the creation wizard information

    hres = _GetClassCacheInfo(pszObjectClass, NULL, CLASSCACHE_CREATIONINFO, &pcce);
    FailGracefully(hres, "Failed to get class information from cache");

    // now allocate the creation wizard structure and pass it to the 
    // caller with the information filled in.

    if ( pcce->hdsaWizardExtn )
        cbStruct += SIZEOF(GUID)*(DSA_GetItemCount(pcce->hdsaWizardExtn)-1);  // -1 as structure already has 1 in the array!

    Trace(TEXT("Allocating creationg structure: cbStruct %d"), cbStruct);

    pdscci = (LPDSCLASSCREATIONINFO)LocalAlloc(LPTR, cbStruct);
    if ( !pdscci )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate return structure");

    //pdscci->dwFlags = 0;
    //pdscci->clsidWizardDialog = { 0 };
    //pdscci->clsidWizardPimaryPage =  { 0 };
    //pdscci->cWizardExtensions = 0;
    //pdscci->aWizardExtensions = { 0 };

    if ( pcce->dwCached & CLASSCACHE_WIZARDDIALOG )
    {
        TraceGUID("clsidWizardDialog is ", pcce->clsidWizardDialog);
        pdscci->dwFlags |= DSCCIF_HASWIZARDDIALOG;
        pdscci->clsidWizardDialog = pcce->clsidWizardDialog;
    }

    if ( pcce->dwCached & CLASSCACHE_WIZARDPRIMARYPAGE )
    {
        TraceGUID("clsidWizardPrimaryPage is ", pcce->clsidWizardPrimaryPage);
        pdscci->dwFlags |= DSCCIF_HASWIZARDPRIMARYPAGE;
        pdscci->clsidWizardPrimaryPage = pcce->clsidWizardPrimaryPage;
    }

    if ( pcce->hdsaWizardExtn )
    {
        pdscci->cWizardExtensions = DSA_GetItemCount(pcce->hdsaWizardExtn);
        Trace(TEXT("Class has %d wizard extensions"), pdscci->cWizardExtensions);

        for ( i = 0 ; i < DSA_GetItemCount(pcce->hdsaWizardExtn) ; i++ )
        {
            LPGUID pGUID = (LPGUID)DSA_GetItemPtr(pcce->hdsaWizardExtn, i);
            TraceAssert(pGUID);

            TraceGUID("Wizard extension %d is ", *pGUID);
            pdscci->aWizardExtensions[i] = *pGUID;
        }
    }

    hres = S_OK;          // success

exit_gracefully:

    ClassCache_ReleaseClassInfo(&pcce);

    // it failed, therefore release pInfo if we have one, before setting
    // the return pointer for the caller.

    if ( FAILED(hres) && pdscci )
    {
        TraceMsg("Failed, so freeing info structure");
        LocalFree(pdscci);
        pdscci = NULL;
    }

    if ( ppdscci )
    {
        Trace(TEXT("Setting ppInfo to %08x"), pdscci);
        *ppdscci = pdscci;
    }

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ IDsDisplaySpecifier::EnumClassAttributes
/ ----------------------------------------
/   Enumerate all the attributes and their friendly names for the given object class.  
/   The code looks up the display specifier and then calls given callback for each one,
/   passing the attribute name and its given "friendly name".
/
/ In:
/   pszObjectClass -> class to enumerate from
/   pEnumCB -> callback function to enumerate to
/   lParam = lParam to pass to the CB fucntion 
/
/ Out:
    HRESULT
/----------------------------------------------------------------------------*/

// BUGBUG: this should return an enumerator

typedef struct
{
    LPDSENUMATTRIBUTES pcbEnum;
    LPARAM lParam;
} CLASSENUMCBSTATE, * LPCLASSENUMCBSTATE;

INT _EnumClassAttributesCB(LPVOID p, LPVOID pData)
{
    LPATTRIBUTENAME pAttributeName = (LPATTRIBUTENAME)p;
    LPCLASSENUMCBSTATE pState = (LPCLASSENUMCBSTATE)pData;
    return SUCCEEDED(pState->pcbEnum(pState->lParam,
                        pAttributeName->pName, pAttributeName->pDisplayName, pAttributeName->dwFlags));
}

STDMETHODIMP CDsDisplaySpecifier::EnumClassAttributes(LPCWSTR pszObjectClass, LPDSENUMATTRIBUTES pcbEnum, LPARAM lParam)
{
    HRESULT hres;
    LPCLASSCACHEENTRY pcce = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_CACHE, "CDsDisplaySpecifier::EnumClassAttributes");
   
    if ( !pszObjectClass || !pcbEnum )
        ExitGracefully(hres, E_INVALIDARG, "Bad class/cb function");

    Trace(TEXT("pszObjectClass: %s"), W2CT(pszObjectClass));

    // call the cache code to pick up the friendly name, having done this we
    // can then copy it to the user buffer

    hres = _GetClassCacheInfo(pszObjectClass, NULL, CLASSCACHE_ATTRIBUTENAMES, &pcce);
    FailGracefully(hres, "Failed to get class information from cache");

    if ( pcce->dwCached & CLASSCACHE_ATTRIBUTENAMES )
    {
        CLASSENUMCBSTATE state = { pcbEnum, lParam };
        DPA_EnumCallback(pcce->hdpaAttributeNames, _EnumClassAttributesCB, &state);
    }

    hres = S_OK;

exit_gracefully:

    ClassCache_ReleaseClassInfo(&pcce);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ IDsDisplaySpecifier::GetAttributeADsType
/ ----------------------------------------
/   Look up the given attribute for its ADsType.
/
/ In:
/   pszAttributeName = attribute to look up
/
/ Out:
/   ADSTYPE    
/----------------------------------------------------------------------------*/
STDMETHODIMP_(ADSTYPE) CDsDisplaySpecifier::GetAttributeADsType(LPCWSTR pszAttributeName)
{
    TraceEnter(TRACE_CACHE, "CDsDisplaySpecifier::GetAttributeADsType");

    CLASSCACHEGETINFO ccgi = { 0 };
    ccgi.pServer = _pszServer;
    ccgi.pUserName = _pszUserName;
    ccgi.pPassword = _pszPassword;

    if ( _dwFlags & DSSSF_SIMPLEAUTHENTICATE )
        ccgi.dwFlags |= CLASSCACHE_SIMPLEAUTHENTICATE;

    if ( _dwFlags & DSSSF_DSAVAILABLE )
        ccgi.dwFlags |= CLASSCACHE_DSAVAILABLE;

    ADSTYPE adt = ClassCache_GetADsTypeFromAttribute(&ccgi, pszAttributeName);
    TraceLeaveValue(adt);
}


/*-----------------------------------------------------------------------------
/ Externally exported cache APIs
/----------------------------------------------------------------------------*/

CDsDisplaySpecifier g_dsDisplaySpecifier;

//
// these are exported for backwards compatiblity.  We used to expose a series
// of DsXXX APIs which dsquery, dsfolder and dsadmin all called.  We have
// now migrated these to a COM interface.
//

STDAPI_(HICON) DsGetIcon(DWORD dwFlags, LPWSTR pszObjectClass, INT cxImage, INT cyImage)
{
    return g_dsDisplaySpecifier.GetIcon(pszObjectClass, dwFlags, cxImage, cyImage);
}

STDAPI DsGetFriendlyClassName(LPWSTR pszObjectClass, LPWSTR pszBuffer, UINT cchBuffer)
{
    return g_dsDisplaySpecifier.GetFriendlyClassName(pszObjectClass, pszBuffer, cchBuffer);
}
