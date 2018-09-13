//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 
//
// File: overlayMN.cpp
//
// This file contains the implementation of CFSIconOverlayManager, a COM object
// that manages the IShellIconOverlayIdentifiers list.
// It aslo managess the Sytem Image List OverlayIndexes, since we have limited slots,
// exactly MAX_OVERLAY_IAMGES of them. 
// History:
//         5-2-97  by dli
//------------------------------------------------------------------------
#include "shellprv.h"
#include "ovrlaymn.h"
#include "fstreex.h"
extern "C" {
#include "filetbl.h"
#include "cstrings.h"
#include "ole2dup.h"
}


extern "C" {
    extern UINT const c_SystemImageListIndexes[];
    extern int g_lrFlags;
}

// NOTE: The value of OVERLAYINDEX_RESERVED is not the same as the overall
// size of the s_ReservedOverlays array, we need to reserved the overlay slot
// #3 for the non-existent Read-Only overaly.
// The Read Only overlay was once there in Win95, but got turned off on IE4
// however, because of the stupidity of the original overlay designs,( we used to
// assign overlay 1 to share and 2 to link and 3 to readonly, and the third parties
// just copied our scheme,) we have to keep overlay #3 as a ghost. 
#define OVERLAYINDEX_RESERVED 4

typedef struct _ReservedIconOverlay
{
    int iShellResvrdImageIndex;
    int iImageIndex;
    int iOverlayIndex;
    int iPriority;
} ReservedIconOverlay;

static ReservedIconOverlay s_ReservedOverlays[] = {
    {II_SHARE, II_SHARE, 1, 10}, 
    {II_LINK, II_LINK, 2, 10},
    // Slot 3 should be reserved as a ghost slot because of the stupid read-only overlay
    {II_SLOWFILE, II_SLOWFILE, 4, 10},
};
    
// File system Icon overlay Identifiers
typedef struct _FSIconOverlay {
    IShellIconOverlayIdentifier * psioi;  
    CLSID clsid;
    int iIconIndex;                          // Index of the Overlay Icon in szIconFile
    int iImageIndex;                         // System Image List index of the icon overlay image
    int iOverlayIndex;
    int iPriority;
    TCHAR szIconFile[MAX_PATH];              // Path of the icon overlay
} FSIconOverlay;

#define FSIconOverlay_GROW 3
#define DSA_LAST    0x7fffffff
#define MAX_OVERLAY_PRIORITY  100
class CFSIconOverlayManager : public IShellIconOverlayManager
{
public:
    CFSIconOverlayManager();
    ~CFSIconOverlayManager();
    
    // *** IUnknown Methods
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IShellIconOverlay Methods
    virtual STDMETHODIMP GetFileOverlayInfo(LPCWSTR pwszPath, DWORD dwAttrib, int * pIndex, DWORD dwFlags);
    virtual STDMETHODIMP GetReservedOverlayInfo(LPCWSTR pwszPath, DWORD dwAttrib, int * pIndex, DWORD dwFlags, int iReservedID);
    virtual STDMETHODIMP RefreshOverlayImages(DWORD dwFlags);
    virtual STDMETHODIMP LoadNonloadedOverlayIdentifiers(void);
    virtual STDMETHODIMP SetIndependentOverlay(int iImage, int * piIndex);
                                             
    // *** Public Methods

    // *** Static Methods
    static HRESULT CreateInstance(IUnknown* pUnkOuter, REFIID riid, OUT LPVOID * ppvOut);

protected:
    
    // IUnknown 
    LONG _cRef;
    CRITICAL_SECTION _cs;
    HDSA _hdsaIconOverlays;      // Icon Overlay Identifiers array, this list is ordered by the IOIs' priority
    HRESULT _InitializeHdsaIconOverlays(); // Initialize the Icon Overlay Identifiers array
    HRESULT _DestroyHdsaIconOverlays();
    int     _GetImageIndex(FSIconOverlay * pfsio);
    FSIconOverlay * _FindMatchingID(LPCWSTR pwszPath, DWORD dwAttrib, int iMinPriority, int * pIOverlayIndex);
    HRESULT _SetGetOverlayInfo(FSIconOverlay * pfsio, int iOverlayIndex, int * pIndex, DWORD dwFlags);
    HRESULT _InitializeReservedOverlays();
    HRESULT _LoadIconOverlayIdentifiers(HDSA hdsaOverlays, BOOL bSkipIfLoaded);

    BOOL _IsIdentifierLoaded(REFCLSID clsid);
//    int  _GetAvailableOverlayIndex(int imyhdsa);
//    HRESULT _SortIOIList();      // Sort the IOI's in the list according to their priority
}; 


HRESULT CFSIconOverlayManager::RefreshOverlayImages(DWORD dwFlags)
{
    EnterCriticalSection(&_cs);

    _InitializeReservedOverlays();

    if (dwFlags && _hdsaIconOverlays)
    {
        for (int ihdsa = 0; ihdsa < DSA_GetItemCount(_hdsaIconOverlays); ihdsa++)
        {
            FSIconOverlay * pfsio = (FSIconOverlay *)DSA_GetItemPtr(_hdsaIconOverlays, ihdsa);
            if (dwFlags & SIOM_ICONINDEX)
                pfsio->iImageIndex = -1;
            if (dwFlags & SIOM_OVERLAYINDEX)
                pfsio->iOverlayIndex = -1;
        }
    }
    LeaveCriticalSection(&_cs);
    return S_OK;
}


HRESULT CFSIconOverlayManager::SetIndependentOverlay(int iImage, int * piIndex)
{
    HRESULT hres = E_FAIL;
    *piIndex = -1;
    int i;
    for (i = 0; i < ARRAYSIZE(s_ReservedOverlays); i++)
    {
        if (s_ReservedOverlays[i].iImageIndex == iImage)
        {
            *piIndex = s_ReservedOverlays[i].iOverlayIndex;
            hres = S_OK;
            break;
        }
    }

    if (i == ARRAYSIZE(s_ReservedOverlays))
    {
        EnterCriticalSection(&_cs);
        if (_hdsaIconOverlays)
        {
            int nOverlays = DSA_GetItemCount(_hdsaIconOverlays);

            // 1. Try to find this overlay image in the list 
            int i;
            for (i = 0; i < nOverlays; i++)
            {
                FSIconOverlay * pfsio = (FSIconOverlay *)DSA_GetItemPtr(_hdsaIconOverlays, i);
                if (pfsio && pfsio->iImageIndex == iImage)
                {
                    *piIndex = pfsio->iOverlayIndex;
                    hres = S_OK;
                    break;
                }
            }

            // 2. Can't find it, let's add it
            if ((i == nOverlays) && (nOverlays < NUM_OVERLAY_IMAGES))
            {
                FSIconOverlay fsio = {0};
                fsio.iImageIndex = iImage;
                fsio.iOverlayIndex = nOverlays + OVERLAYINDEX_RESERVED + 1;
                if (DSA_InsertItem(_hdsaIconOverlays, DSA_LAST, &fsio) >= 0)
                {
                    if (!ImageList_SetOverlayImage(g_himlIcons, iImage, fsio.iOverlayIndex) || 
                        !ImageList_SetOverlayImage(g_himlIconsSmall, iImage, fsio.iOverlayIndex))
                        DSA_DeleteItem(_hdsaIconOverlays, nOverlays);
                    else
                    {
                        *piIndex = fsio.iOverlayIndex;
                        hres = S_OK;
                    }
                }
            }
        }
        LeaveCriticalSection(&_cs);
    }
    return hres;
}


HRESULT CFSIconOverlayManager::_InitializeReservedOverlays()
{
    int i;
    TCHAR szModule[MAX_PATH];

    if (g_himlIcons == NULL)
        FileIconInit(FALSE);

    if (g_himlIcons == NULL)
        return E_FAIL;
    
    HKEY hkeyIcons = SHGetExplorerSubHkey(HKEY_LOCAL_MACHINE, TEXT("Shell Icons"), FALSE);
        
    GetModuleFileName(HINST_THISDLL, szModule, ARRAYSIZE(szModule));

    for (i = 0; i < ARRAYSIZE(s_ReservedOverlays); i++)
    {
        ASSERT(s_ReservedOverlays[i].iShellResvrdImageIndex > 0);
        ASSERT(s_ReservedOverlays[i].iOverlayIndex > 0);
        ASSERT(s_ReservedOverlays[i].iOverlayIndex <= MAX_OVERLAY_IMAGES);
        
        //
        // Warning: This is used by non explorer processes on NT only
        // because their image list was initialized with only 4 icons
        //
        HICON hIcon = NULL;
        HICON hSmallIcon = NULL;
        int iIndex = s_ReservedOverlays[i].iShellResvrdImageIndex;

        // if we have an index that is no longer valid...
        if ( s_ReservedOverlays[i].iImageIndex >= ImageList_GetImageCount(g_himlIcons))
        {
            // check to see if icon is overridden in the registry
            if (hkeyIcons)
            {
                TCHAR val[10];
                TCHAR ach[MAX_PATH];
                DWORD cb = SIZEOF(ach);

                wsprintf(val, TEXT("%d"), iIndex);

                ach[0] = 0;
                SHQueryValueEx(hkeyIcons, val, NULL, NULL, (LPBYTE)ach, &cb);

                if (ach[0])
                {
                    HICON hIcons[2] = {0, 0};
                    int iIcon = PathParseIconLocation(ach);

                    ExtractIcons(ach, iIcon,
                                 MAKELONG(g_cxIcon,g_cxSmIcon),
                                 MAKELONG(g_cyIcon,g_cySmIcon),
                                 hIcons, NULL, 2, g_lrFlags);

                    hIcon = hIcons[0];
                    hSmallIcon = hIcons[1];
                }
            }

			if ( ! hIcon )
            {
                hIcon      = (HICON)LoadImage(HINST_THISDLL, MAKEINTRESOURCE(c_SystemImageListIndexes[iIndex]), IMAGE_ICON, g_cxIcon, g_cyIcon, g_lrFlags);
                hSmallIcon = (HICON)LoadImage(HINST_THISDLL, MAKEINTRESOURCE(c_SystemImageListIndexes[iIndex]), IMAGE_ICON, g_cxSmIcon, g_cySmIcon, g_lrFlags);
            }
            
            if (hIcon)
                s_ReservedOverlays[i].iImageIndex = SHAddIconsToCache(hIcon, hSmallIcon, szModule, iIndex, 0);
        }
        ImageList_SetOverlayImage(g_himlIcons, s_ReservedOverlays[i].iImageIndex, s_ReservedOverlays[i].iOverlayIndex);
        ImageList_SetOverlayImage(g_himlIconsSmall, s_ReservedOverlays[i].iImageIndex, s_ReservedOverlays[i].iOverlayIndex);
    }
    
    return S_OK;
}

//===========================================================================
// Initialize the IShellIconOverlayIdentifiers 
//===========================================================================
HRESULT CFSIconOverlayManager::_InitializeHdsaIconOverlays() 
{
    HRESULT hres = S_FALSE; // Already initialized.

    if (NULL == _hdsaIconOverlays)
    {
        hres = _InitializeReservedOverlays();
        if (SUCCEEDED(hres))
        {
            _hdsaIconOverlays = DSA_Create(SIZEOF(FSIconOverlay), FSIconOverlay_GROW);

            if(NULL != _hdsaIconOverlays)
            {
                hres = _LoadIconOverlayIdentifiers(_hdsaIconOverlays, FALSE);
            }
            else
            {
                hres = E_OUTOFMEMORY;
            }
        }
    }
    return hres;
}



HRESULT CFSIconOverlayManager::LoadNonloadedOverlayIdentifiers(void)
{
    HRESULT hres;
    EnterCriticalSection(&_cs);

    if (NULL == _hdsaIconOverlays)
    {
        //
        // No overlay HDSA yet.  We should never hit this but just in case,
        // this will be valid behavior.
        //
        hres = _InitializeHdsaIconOverlays();
    }
    else
    {
        //
        // Load unloaded identifiers into existing HDSA.
        //
        hres = _LoadIconOverlayIdentifiers(_hdsaIconOverlays, TRUE);
    }

    LeaveCriticalSection(&_cs);
    return hres;
}


HRESULT CFSIconOverlayManager::_LoadIconOverlayIdentifiers(HDSA hdsaOverlays, BOOL bSkipIfLoaded)
{
    ASSERT(NULL != hdsaOverlays);

    HDCA hdca = DCA_Create();
    if (!hdca)
        return E_OUTOFMEMORY;

    HRESULT hrInit = SHCoInitialize();

    // Enumerate all of the Icon Identifiers in
    DCA_AddItemsFromKey(hdca, HKEY_LOCAL_MACHINE, REGSTR_ICONOVERLAYID);
    if (DCA_GetItemCount(hdca) <= 0)
        goto EXIT;

    int idca;
    for (idca = 0; idca < DCA_GetItemCount(hdca); idca++)
    {
        const CLSID * pclsid = DCA_GetItem(hdca, idca);

        if (bSkipIfLoaded && _IsIdentifierLoaded(*pclsid))
            continue;

        FSIconOverlay fsio;
        ZeroMemory(&fsio, sizeof(fsio));
        if (FAILED(DCA_CreateInstance(hdca, idca, IID_IShellIconOverlayIdentifier,
                                      (LPVOID *) &(fsio.psioi))))
            continue;       

        SHPinDllOfCLSID(pclsid);
        
        DWORD dwFlags = 0;
        int iIndex;
        WCHAR wszIconFile[MAX_PATH];
        // Initialize the Overlay Index to -1
        fsio.iOverlayIndex = -1;

        // Try get the overlay icon information from the Overlay Identifiers 
        if (S_OK == fsio.psioi->GetOverlayInfo(wszIconFile, ARRAYSIZE(wszIconFile), &iIndex, &dwFlags))
        {
            if (dwFlags & ISIOI_ICONFILE)
            {
                SHUnicodeToTChar(wszIconFile, fsio.szIconFile, ARRAYSIZE(fsio.szIconFile));
                fsio.iImageIndex = -1;
                if (dwFlags & ISIOI_ICONINDEX)
                    fsio.iIconIndex = iIndex;
                else
                    fsio.iIconIndex = 0;
            }

            if (FAILED(fsio.psioi->GetPriority(&fsio.iPriority)))
                fsio.iPriority = MAX_OVERLAY_PRIORITY;

            CopyMemory(&fsio.clsid, pclsid, sizeof(fsio.clsid));
            DSA_InsertItem(hdsaOverlays, DSA_LAST, &fsio);
        }
        // Now try to look in the registry for the Overlay Icons 
        else
        {
            fsio.iImageIndex = -1;
            const CLSID * pclsid = DCA_GetItem(hdca, idca);
            if (pclsid)
            {
                TCHAR szCLSID[GUIDSTR_MAX];
                TCHAR szRegKey[GUIDSTR_MAX + 40];
                HKEY hkeyIcon;
                SHStringFromGUID(*pclsid, szCLSID, ARRAYSIZE(szCLSID));
                wsprintf(szRegKey, REGSTR_ICONOVERLAYCLSID, szCLSID);
                if (ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT, szRegKey, &hkeyIcon))
                {
                    LONG cb = SIZEOF(fsio.szIconFile);
                    if (SHRegQueryValue(hkeyIcon, c_szDefaultIcon, fsio.szIconFile, &cb) == ERROR_SUCCESS && fsio.szIconFile[0])
                    {
                        fsio.iIconIndex = PathParseIconLocation(fsio.szIconFile);
                        CopyMemory(&fsio.clsid, pclsid, sizeof(fsio.clsid));
                        DSA_InsertItem(hdsaOverlays, DSA_LAST, &fsio);
                    }

                    // Unfinished !!! Code to retrieve the priority here
                    fsio.iPriority = MAX_OVERLAY_PRIORITY;
                    RegCloseKey(hkeyIcon);
                }
            }
        }

        // Stop when we have more than we can handle
        if (DSA_GetItemCount(hdsaOverlays) >= (MAX_OVERLAY_IMAGES - OVERLAYINDEX_RESERVED))
            break;
    }
    
EXIT:
    DCA_Destroy(hdca);
    SHCoUninitialize(hrInit);
    return S_OK;
}
     

BOOL CFSIconOverlayManager::_IsIdentifierLoaded(REFCLSID clsid)
{
    if (NULL != _hdsaIconOverlays)
    {
        int cEntries = DSA_GetItemCount(_hdsaIconOverlays);
        for (int i = 0; i < cEntries; i++)
        {
            FSIconOverlay *pfsio = (FSIconOverlay *)DSA_GetItemPtr(_hdsaIconOverlays, i);            
            if (pfsio->clsid == clsid)
                return TRUE;
        }
    }
    return FALSE;
}


CFSIconOverlayManager::CFSIconOverlayManager() : _cRef(1) // _hdsaIconOverlays(NULL)
{
    InitializeCriticalSection(&_cs);
}

HRESULT CFSIconOverlayManager::_DestroyHdsaIconOverlays()
{
    if (_hdsaIconOverlays)
    {
        DSA_Destroy(_hdsaIconOverlays);
    }
    
    return S_OK;
}

CFSIconOverlayManager::~CFSIconOverlayManager()
{
    if (_hdsaIconOverlays)
        _DestroyHdsaIconOverlays();

    DeleteCriticalSection(&_cs);
}

//
// CFSFolder_GetAvailableOverlayIndex:
// This function first tries to find an empty slot in all the available overlay indexes
// If none found, it goes through the _hdsaIconOverlays array elements who have lower
// priorities and grab their overlay indexes if they are using one
//
/*int CFSIconOverlayManager::_GetAvailableOverlayIndex(int imyhdsa)
{
    int ib;
    for (ib = 0; ib < MAX_OVERLAY_IMAGES; ib++)
        if (_bOverlayIndexOccupied[ib] == FALSE)
            break;

    // Add code to grab indexes here.
    return ++ib;
}*/

HRESULT CFSIconOverlayManager::QueryInterface(REFIID riid, LPVOID * ppvObj)
{ 
    // ppvObj must not be NULL
    ASSERT(ppvObj != NULL);
    
    if (IsEqualIID(riid, IID_IUnknown))
    {    
        *ppvObj = SAFECAST(this, IUnknown *);
        DebugMsg(DM_TRACE, TEXT("QI IUnknown succeeded"));
    }
    else if (IsEqualIID(riid, IID_IShellIconOverlayManager))
    {
        *ppvObj = SAFECAST(this, IShellIconOverlayManager*);
        DebugMsg(DM_TRACE, TEXT("QI IShellIconOverlayManager succeeded"));
    } 
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;  // Otherwise, don't delegate to HTMLObj!!
    }
    
    AddRef();
    return S_OK;
}


ULONG CFSIconOverlayManager::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CFSIconOverlayManager::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

int CFSIconOverlayManager::_GetImageIndex(FSIconOverlay * pfsio)
{
    //BUGBUG: (dli) there is similar code in defview.cpp
    int iImage = LookupIconIndex(PathFindFileName(pfsio->szIconFile), pfsio->iIconIndex, GIL_FORSHELL);

    if (iImage == -1)
    {
        // we couldn't find it from the cache
        HICON hIconLarge = NULL;
        HICON hIconSmall = NULL;
        SHDefExtractIcon(pfsio->szIconFile, pfsio->iIconIndex, GIL_FORSHELL, &hIconLarge,
                         &hIconSmall, MAKELONG(g_cxIcon, g_cxSmIcon));

        if (hIconLarge)
            iImage = SHAddIconsToCache(hIconLarge, hIconSmall, pfsio->szIconFile, pfsio->iIconIndex, GIL_FORSHELL);
    }
    
    return iImage;
}

FSIconOverlay * CFSIconOverlayManager::_FindMatchingID(LPCWSTR pwszPath, DWORD dwAttrib, int iMinPriority, int * pIOverlayIndex)
{
    // If we got here, we must have the DSA array
    ASSERT(_hdsaIconOverlays);
    if (_hdsaIconOverlays)
    {
        int ihdsa;
        for (ihdsa = 0; ihdsa < DSA_GetItemCount(_hdsaIconOverlays); ihdsa++)
        {
            FSIconOverlay * pfsio = (FSIconOverlay *)DSA_GetItemPtr(_hdsaIconOverlays, ihdsa);
            ASSERT(pfsio);
            if (pfsio->iPriority >= iMinPriority)
                continue;
            if (pfsio->psioi && pfsio->psioi->IsMemberOf(pwszPath, dwAttrib) == S_OK)
            {
                // Overlay indexes start from 1, and let's not use the reserved ones
                ASSERT(pIOverlayIndex);
                *pIOverlayIndex = ihdsa + OVERLAYINDEX_RESERVED + 1; 
                return pfsio;
            }
        }
    }
    return NULL;
}

HRESULT CFSIconOverlayManager::_SetGetOverlayInfo(FSIconOverlay * pfsio, int iOverlayIndex, int * pIndex, DWORD dwFlags)
{
    HRESULT hres = E_FAIL;
    RIP(pIndex);
    *pIndex = -1;
#if 0    // we don't want to return the priority for now
    if (dwFlags == SIOM_PRIORITY)
    {
        // This must have been initialized in the initialization function
        *pIndex = pfsio->iPriority;
    }
#endif
    if (pfsio->iImageIndex == -1)
    {
        int iImage = _GetImageIndex(pfsio);

        // Either we couldn't get it or we couldn't put it in cache 
        if (iImage == -1)
        {
            // leave this as a zombie
            pfsio->iImageIndex = 0;
            pfsio->iOverlayIndex = 0;
        }
        else
            pfsio->iImageIndex = iImage;
    }

    // Only if we have a reasonable image index will we proceed. 
    if (pfsio->iImageIndex > 0)
    {
        if (dwFlags == SIOM_ICONINDEX)
        {
            *pIndex = pfsio->iImageIndex;
        }
        else
        {
            ASSERT(iOverlayIndex > 0);
            ASSERT(iOverlayIndex <= MAX_OVERLAY_IMAGES);
            if (pfsio->iOverlayIndex == -1)
            {
                // Now set the overlay
                ASSERT(g_himlIcons);
                ASSERT(g_himlIconsSmall);
                ImageList_SetOverlayImage(g_himlIcons, pfsio->iImageIndex, iOverlayIndex);
                ImageList_SetOverlayImage(g_himlIconsSmall, pfsio->iImageIndex, iOverlayIndex);

                pfsio->iOverlayIndex = iOverlayIndex;
            }

            // Must be the overlayindex flag
            ASSERT(dwFlags == SIOM_OVERLAYINDEX);
            *pIndex = pfsio->iOverlayIndex;
        }
        hres = S_OK;

    }
    return hres;
}

HRESULT CFSIconOverlayManager::GetFileOverlayInfo(LPCWSTR pwszPath, DWORD dwAttrib, int * pIndex, DWORD dwFlags)
{
    ASSERT((dwFlags == SIOM_OVERLAYINDEX) || (dwFlags == SIOM_ICONINDEX)); // || (dwFlags == SIOM_PRIORITY));

    HRESULT hres = E_FAIL;
    int iOverlayIndex;
    *pIndex = 0;
    EnterCriticalSection(&_cs);
    if (_hdsaIconOverlays)
    {
        FSIconOverlay * pfsio = _FindMatchingID(pwszPath, dwAttrib, MAX_OVERLAY_PRIORITY, &iOverlayIndex);
        if (pfsio)
            hres = _SetGetOverlayInfo(pfsio, iOverlayIndex, pIndex, dwFlags);
    }
    LeaveCriticalSection(&_cs);
    return hres;
}

HRESULT CFSIconOverlayManager::GetReservedOverlayInfo(LPCWSTR pwszPath, DWORD dwAttrib, int * pIndex, DWORD dwFlags, int iReservedID)
{
    ASSERT(iReservedID < OVERLAYINDEX_RESERVED);
    HRESULT hres = S_OK;

    EnterCriticalSection(&_cs);
    if (_hdsaIconOverlays && pwszPath)
    {
        int iOverlayIndex;
        FSIconOverlay * pfsio = _FindMatchingID(pwszPath, dwAttrib, s_ReservedOverlays[iReservedID].iPriority, &iOverlayIndex);
        if (pfsio)
        {
            hres = _SetGetOverlayInfo(pfsio, iOverlayIndex, pIndex, dwFlags);
            LeaveCriticalSection(&_cs);
            return hres;
        }
    }
    
    if (dwFlags == SIOM_ICONINDEX)
        *pIndex =  s_ReservedOverlays[iReservedID].iImageIndex;
    else
    {
        ASSERT(dwFlags == SIOM_OVERLAYINDEX);
        *pIndex =  s_ReservedOverlays[iReservedID].iOverlayIndex;
    }
    LeaveCriticalSection(&_cs);

    return hres;
}


HRESULT CFSIconOverlayManager::CreateInstance(IUnknown* pUnkOuter, REFIID riid, OUT LPVOID * ppvOut)
{
    HRESULT hr;
    
    DebugMsg(DM_TRACE, TEXT("CFSIconOverlayManager::CreateInstance()"));
    
    *ppvOut = NULL;                     // null the out param

    CFSIconOverlayManager *pcfsiom = new CFSIconOverlayManager;

    if (!pcfsiom)
        return E_OUTOFMEMORY;

    hr = pcfsiom->_InitializeHdsaIconOverlays();
    if (SUCCEEDED(hr))
        hr = pcfsiom->QueryInterface(riid, ppvOut);
    pcfsiom->Release();

    return hr;
}


STDAPI CFSIconOverlayManager_CreateInstance(IUnknown* pUnkOuter, REFIID riid, OUT LPVOID *  ppvOut)
{
    return CFSIconOverlayManager::CreateInstance(pUnkOuter, riid, ppvOut);
}

STDAPI_(int) SHGetIconOverlayIndexW(LPCWSTR pwszIconPath, int iIconIndex)
{

    TCHAR szIconPath[MAX_PATH];
    int iRet = -1;
    int iImage = -1;

    // If NULL path is passed in, see if the index matches one of our special indexes
    if (pwszIconPath == NULL)
    {
        switch (iIconIndex)
        {
            case IDO_SHGIOI_SHARE:
                iImage = s_ReservedOverlays[0].iImageIndex;
                break;
            case IDO_SHGIOI_LINK:
                iImage = s_ReservedOverlays[1].iImageIndex;
                break;
            case IDO_SHGIOI_SLOWFILE:
                iImage = s_ReservedOverlays[2].iImageIndex;
                break;
        }
    }
    else if (SHUnicodeToTChar(pwszIconPath, szIconPath, ARRAYSIZE(szIconPath)))        
            // Try to load the image into the shell icon cache            
            iImage = Shell_GetCachedImageIndex(szIconPath, iIconIndex, 0);
    
    if (iImage >= 0)
    {
        IShellIconOverlayManager *psiom;
        if (SUCCEEDED(GetIconOverlayManager(&psiom)))
        {
            int iCandidate = -1;
            if (SUCCEEDED(psiom->SetIndependentOverlay(iImage, &iCandidate)))
            {
                iRet = iCandidate;
            }
            psiom->Release();
        }
    }
    
    return iRet;
}

STDAPI_(int) SHGetIconOverlayIndexA(LPCSTR pszIconPath, int iIconIndex)
{
    int iRet = -1;
    WCHAR wszIconPath[MAX_PATH];
    LPCWSTR pwszIconPath = NULL;
    if (pszIconPath)
    {
        wszIconPath[0] = L'\0';
        SHAnsiToUnicode(pszIconPath, wszIconPath, ARRAYSIZE(wszIconPath));
        pwszIconPath = wszIconPath;
    }
    
    return  SHGetIconOverlayIndexW(pwszIconPath, iIconIndex);
}