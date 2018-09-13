#include "priv.h"
#include "browsext.h"
#include "tbext.h"
#include <winreg.h>     // For the registry walking
#include "dochost.h"
#include "resource.h"
#include <mluisupp.h>


// {DFEED31E-78ED-11d2-86BA-00C04F8EEA99}
EXTERN_C const IID IID_IToolbarExt = 
{ 0xdfeed31e, 0x78ed, 0x11d2, { 0x86, 0xba, 0x0, 0xc0, 0x4f, 0x8e, 0xea, 0x99 } };

// {D82B85D0-78F4-11d2-86BA-00C04F8EEA99}
EXTERN_C const CLSID CLSID_PrivBrowsExtCommands =
{ 0xd82b85d0, 0x78f4, 0x11d2, { 0x86, 0xba, 0x0, 0xc0, 0x4f, 0x8e, 0xea, 0x99 } };

const TCHAR c_szHelpMenu[]  = TEXT("help");

//+-------------------------------------------------------------------------
// Creates and instance of CBrowserExtension
//--------------------------------------------------------------------------
HRESULT CBrowserExtension_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    *ppunk = NULL;
    CBrowserExtension* p = new CBrowserExtension();
    if (p)
    {
        *ppunk = SAFECAST(p, IToolbarExt*);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

CBrowserExtension::CBrowserExtension()
:   _cRef(1),
    _uStringIndex((UINT)-1),
    _uiImageIndex((UINT)-1)
{
    ASSERT(_pISB == NULL);
    ASSERT(_hdpa == NULL);
    ASSERT(_nExtButtons == 0);
    ASSERT(_fStringInit == FALSE);
    ASSERT(_fImageInit == FALSE);
}

CBrowserExtension::~CBrowserExtension(void)
{
    if (_pISB)
        _pISB->Release();

    if (_hdpa)
    {
        _FreeItems();
        DPA_Destroy(_hdpa);
    }

    _ReleaseImageLists(_uiImageIndex);
}

// *** IUnknown methods ***

HRESULT CBrowserExtension::QueryInterface(REFIID riid, void ** ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CBrowserExtension, IToolbarExt),
        QITABENT(CBrowserExtension, IObjectWithSite),
        QITABENT(CBrowserExtension, IOleCommandTarget),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CBrowserExtension::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CBrowserExtension::Release()
{
    if (InterlockedDecrement(&_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return _cRef;
}

// IToolbarExt interface functions
HRESULT CBrowserExtension::SetSite(IUnknown* pUnkSite)
{
    HRESULT hr = S_OK;

    ATOMICRELEASE(_pISB);

    if (pUnkSite)
    {
        hr = pUnkSite->QueryInterface(IID_IShellBrowser, (LPVOID*)&_pISB);
    }

    // See if we need to init ourselves
    if (NULL == _hdpa)
    {
        // Real construction happens here
        HRESULT hr2 = Update();

        ASSERT(SUCCEEDED(hr2));
    }
    else
    {
        // Update the site for each button/menu extension
        for (int i = 0; i < DPA_GetPtrCount(_hdpa); i++)
        {
            ExtensionItem* pItem = (ExtensionItem*)DPA_GetPtr(_hdpa, i);
            IUnknown_SetSite(pItem->pIBE, _pISB);
        }
    }
    return hr;
}

STDMETHODIMP CBrowserExtension::GetSite(REFIID riid, void ** ppvSite)
{
    HRESULT hr = S_OK;
    *ppvSite = NULL;

    if (_pISB)
    {
        hr = _pISB->QueryInterface(riid, ppvSite);
    }
    return hr;
}

HRESULT CBrowserExtension::GetNumButtons(UINT* pButtons)
{
    ASSERT(pButtons);
    *pButtons = _nExtButtons;
    return S_OK;
}

HRESULT CBrowserExtension::InitButtons(IExplorerToolbar* pxtb, UINT* puStringIndex, const GUID* pguidCommandGroup)
{
    ASSERT(pxtb);

    UINT uiSize;
    pxtb->GetBitmapSize(&uiSize);
    int cx = LOWORD(uiSize);

    // Get the image lists for the current button size and screen resolution
    CImageList* pimlDef;
    CImageList* pimlHot;
    UINT uiImageIndexOld = _uiImageIndex;
    _uiImageIndex = _GetImageLists(&pimlDef, &pimlHot, cx < 20);
    pxtb->SetImageList(pguidCommandGroup, *pimlDef, *pimlHot, NULL);

    // Free the previously used image list
    _ReleaseImageLists(uiImageIndexOld);

    // Add the button text to the toolbar
    if (_uStringIndex == (UINT)-1)
    {
        LRESULT iAddResult = 0; // result of adding the string buffer to the toolbar string list
        HRESULT hr = pxtb->AddString(pguidCommandGroup, MLGetHinst(), IDS_BROWSER_TB_LABELS, &iAddResult);
        _uStringIndex = (UINT)iAddResult;
        _AddCustomStringsToBuffer(pxtb, pguidCommandGroup);
    }

    *puStringIndex = _uStringIndex;
    return S_OK;
}

CBrowserExtension::ExtensionItem* CBrowserExtension::_FindItem(REFGUID rguid)
{
    ExtensionItem* pFound = NULL;
    if (NULL != _hdpa)
    {
        for (int i = 0; i < DPA_GetPtrCount(_hdpa); i++)
        {
            ExtensionItem* pItem = (ExtensionItem*)DPA_GetPtr(_hdpa, i);

            if (pItem && IsEqualGUID(pItem->guid, rguid))
            {
                pFound = pItem;
                break;
            }
        }
    }
    return pFound;
}

void CBrowserExtension::_AddItem(HKEY hkeyExtensions, LPCWSTR pszGuidItem, REFGUID rguidItem)
{
    // Create the dpa used to store our items
    if (NULL == _hdpa)
    {
        _hdpa = DPA_Create(5);
        if (NULL == _hdpa)
        {
            return;
        }
    }

    HKEY hkeyThisExtension;

    if (RegOpenKeyEx(hkeyExtensions, pszGuidItem, 0, KEY_READ, &hkeyThisExtension) == ERROR_SUCCESS)
    {
        // Get the clsid of the object
        WCHAR szCLSID[64];
        ULONG cbCLSID = SIZEOF(szCLSID);
        CLSID clsidCustomButton;

        if (SUCCEEDED(RegQueryValueEx(hkeyThisExtension, TEXT("clsid"), NULL, NULL, (unsigned char *)&szCLSID, &cbCLSID)) &&
            SUCCEEDED(CLSIDFromString(szCLSID, &clsidCustomButton)))
        {
            IBrowserExtension * pibeTemp;

            // Check for our internal object.  Note that our CoCreateInctance wrapper
            // compares to the address of the global clsid, so we want to use the global
            // guid.
            const CLSID* pclsid = &clsidCustomButton;
            if (IsEqualGUID(clsidCustomButton, CLSID_ToolbarExtExec))
            {
                pclsid = &CLSID_ToolbarExtExec;
            }
            else if (IsEqualGUID(clsidCustomButton, CLSID_ToolbarExtBand))
            {
                pclsid = &CLSID_ToolbarExtBand;
            }

            // Create the extension object
            if (SUCCEEDED(CoCreateInstance(*pclsid, NULL, CLSCTX_INPROC_SERVER,
                                 IID_IBrowserExtension, (void **)&pibeTemp)))
            {
                if (SUCCEEDED(pibeTemp->Init(rguidItem)))
                {
                    // Add this item to our array
                    ExtensionItem* pItem = new ExtensionItem;
                    if (pItem)
                    {
                        if (DPA_AppendPtr(_hdpa, pItem) != -1)
                        {
                            VARIANTARG varArg;

                            pItem->idCmd = _GetCmdIdFromClsid(pszGuidItem);
                            pItem->pIBE = pibeTemp;
                            pItem->guid = rguidItem;
                            pibeTemp->AddRef();

                            // See if it's a button
                            if (SUCCEEDED(pibeTemp->GetProperty(TBEX_BUTTONTEXT, NULL)))
                            {
                                _nExtButtons++;
                                pItem->fButton = TRUE;

                                // See if the button default to visible on the toolbar
                                if (SUCCEEDED(pibeTemp->GetProperty(TBEX_DEFAULTVISIBLE, &varArg)))
                                {
                                    ASSERT(varArg.vt == VT_BOOL);
                                    pItem->fVisible = (varArg.boolVal == VARIANT_TRUE);
                                }
                            }

                            // set the target menu
                            
                            pItem->idmMenu = 0;
                            
                            if (SUCCEEDED(pibeTemp->GetProperty(TMEX_MENUTEXT, NULL)))
                            {
                                
                                if (SUCCEEDED(pibeTemp->GetProperty(TMEX_CUSTOM_MENU, &varArg)))
                                {
                                    ASSERT(varArg.vt == VT_BSTR);
                                    ASSERT(IS_VALID_STRING_PTR(varArg.bstrVal, -1));
    
                                    if (!StrCmpNI(varArg.bstrVal, c_szHelpMenu, ARRAYSIZE(c_szHelpMenu)))
                                    {
                                        pItem->idmMenu = FCIDM_MENU_HELP;
                                    }
    
                                    VariantClear(&varArg);
                                }

                                if (pItem->idmMenu == 0)
                                {
                                    pItem->idmMenu = FCIDM_MENU_TOOLS;
                                }
                            }

                            // Pass the site to the object
                            IUnknown_SetSite(pibeTemp, _pISB);
                        }
                        else
                        {
                            delete pItem;
                        }
                    }
                }

                // This will free pibeTemp if we didn't store it away
                pibeTemp->Release();
            }
        }
        RegCloseKey(hkeyThisExtension);
    }
}


//
// All real construction happens here.  In theory this function can be called upon a SysINIChange to update our
// custom toolbar cached information.  This has not been tested.  This opens the Extensions section of the registry
// enumerates all of the subkeys.  Attempts to CoCreate each one.  Upon successful CoCreation it calls
// IObjectWithSite::SetSite(IShellBrowser), if it is implemented.  Next IBrowserExtension::Init is called.  Finally,
// IBrowserExtension::GetProperty(TBEX_BUTTONTEXT, NULL) is called looking for a S_OK to insure that the control in
// question is a Toolbar Button (as opposed to a tools menu item, or...)
//
HRESULT CBrowserExtension::Update()
{
    WCHAR szItemGuid[64];    // sufficient for {clsid}
    DWORD cbItemGuid;
    GUID guidItem;
    HRESULT hr = S_OK;

    // Free previous items
    _nExtButtons = 0;
    _nExtToolsMenuItems = 0;
    _FreeItems();

    // First add extensions from HKCU
    HKEY hkeyExtensions;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Internet Explorer\\Extensions"), 0,
                     KEY_READ, &hkeyExtensions) == ERROR_SUCCESS)
    {
        cbItemGuid = sizeof(szItemGuid);
        for (int iKey = 0;
             RegEnumKeyEx(hkeyExtensions, iKey, szItemGuid, &cbItemGuid, NULL, NULL, NULL, NULL) != ERROR_NO_MORE_ITEMS;
             iKey++)
        {
            if (SUCCEEDED(CLSIDFromString(szItemGuid, &guidItem)))
            {
                _AddItem(hkeyExtensions, szItemGuid, guidItem);
            }
            cbItemGuid = sizeof(szItemGuid);
        }

        RegCloseKey(hkeyExtensions);
    }

    // Next add any unique items from HKLM
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Internet Explorer\\Extensions"), 0,
                     KEY_READ, &hkeyExtensions) == ERROR_SUCCESS)
    {
        cbItemGuid = sizeof(szItemGuid);
        for (int iKey = 0;
             RegEnumKeyEx(hkeyExtensions, iKey, szItemGuid, &cbItemGuid, NULL, NULL, NULL, NULL) != ERROR_NO_MORE_ITEMS;
             iKey++)
        {
            if (SUCCEEDED(CLSIDFromString(szItemGuid, &guidItem)))
            {
                if (_FindItem(guidItem) == NULL)
                {
                    _AddItem(hkeyExtensions, szItemGuid, guidItem);
                }
            }
            cbItemGuid = sizeof(szItemGuid);
        }

        RegCloseKey(hkeyExtensions);
    }

    return hr;
}

//
// This takes a TBBUTTON[] and fills in the Custom Buttons.  A couple of usage points:
// (1) The caller should allocate a TBBUTTON[] big enough for NUM_STD_BUTTONS + GetNumExtButtons()
//     Then they should copy the standard buttons into the array, and pass the pointer to the remainder
//     of the array here.
// (2) This function should *by design* never be called before AddCustomImagesToImageList and
//     AddCustomStringsToBuffer have both been called.  An attempt to do so in DEBUG mode will hit
//     a break point.
//
HRESULT CBrowserExtension::GetButtons(TBBUTTON * tbArr, int nNumButtons, BOOL fInit)
{
    ASSERT(_fStringInit && _fImageInit);

    if (_hdpa)
    {
        ASSERT(nNumButtons == _nExtButtons);
        ASSERT(tbArr != NULL)
        int iBtn = 0;

        for (int i = 0; i < DPA_GetPtrCount(_hdpa); i++)
        {
            ExtensionItem* pItem = (ExtensionItem*)DPA_GetPtr(_hdpa, i);
            if (!pItem->fButton)
                continue;

            // We use the MAKELONG(n, 1) to insure that we are using the alternate image list.
            tbArr[iBtn].iBitmap    = MAKELONG(pItem->iImageID, 1);
            tbArr[iBtn].idCommand  = pItem->idCmd;
            tbArr[iBtn].fsState    = TBSTATE_ENABLED;
            tbArr[iBtn].fsStyle    = BTNS_BUTTON;
            tbArr[iBtn].dwData     = 0;
            tbArr[iBtn].iString    = pItem->iStringID;

            //
            // Default to hidden during initialization so that it defaults to the left well
            // of the the customize dialog (defaults off the toolbar)
            //
            if (fInit && !pItem->fVisible)
            {
                tbArr[iBtn].fsState = TBSTATE_HIDDEN;
            }

            ++iBtn;
        }
    }
    return S_OK;
}

//
// This function takes the ImageLists for hot and normal icons and adds the appropriate icon to each
// list for each custom toolbar button.  The resultant ImageID is then stored in our _rgExtensionItem struct
// so that the IDs can be placed in a TBBUTTON[] when AddExtButtonsTBArray is called.
//
HRESULT CBrowserExtension::_AddCustomImagesToImageList(CImageList& rimlNormal, CImageList& rimlHot, BOOL fSmallIcons)
{
#ifdef DEBUG
    _fImageInit = TRUE;
#endif DEBUG

    if (NULL != _hdpa)
    {
        for (int i = 0; i < DPA_GetPtrCount(_hdpa); i++)
        {
            ExtensionItem* pItem = (ExtensionItem*)DPA_GetPtr(_hdpa, i);
            if (!pItem->fButton)
                continue;

            VARIANTARG varArg;

            pItem->iImageID = rimlNormal.GetImageIndex(pItem->guid);
            if (-1 == pItem->iImageID &&
                SUCCEEDED(pItem->pIBE->GetProperty((fSmallIcons ? TBEX_GRAYICONSM : TBEX_GRAYICON), &varArg)))
            {
                if (varArg.vt == VT_BYREF)
                {
                    pItem->iImageID = rimlNormal.AddIcon((HICON)varArg.byref, pItem->guid);
                }
                else if (varArg.vt == VT_I4)
                {
                    // It's one of our built-in images
                    pItem->iImageID = varArg.lVal;
                }
                else
                {
                    ASSERT(FALSE);
                }
            }

            int iHot = rimlHot.GetImageIndex(pItem->guid);
            if (-1 == iHot &&
                SUCCEEDED(pItem->pIBE->GetProperty((fSmallIcons ? TBEX_HOTICONSM : TBEX_HOTICON), &varArg)))
            {
                if (varArg.vt == VT_BYREF)
                {
                    iHot = rimlHot.AddIcon((HICON)varArg.byref, pItem->guid);
                }
                else if (varArg.vt == VT_I4)
                {
                    // It's one of our built-in images
                    iHot = varArg.lVal;
                }
                else
                {
                    ASSERT(FALSE);
                }
            }

            ASSERT(iHot == pItem->iImageID);
        }
    }

    return S_OK;
}

//
// This function takes the StringList and adds the caption (ToolbarText) for each of the custom toolbar buttons
// to it.  The resultant StringID is then stored in our _rgExtensionItem struct so that the ID can be placed in
// a TBBUTTON[] when AddExtButtonsTBArray is called.
//
HRESULT CBrowserExtension::_AddCustomStringsToBuffer(IExplorerToolbar * pxtb, const GUID* pguidCommandGroup)
{
#ifdef DEBUG
    _fStringInit = TRUE;
#endif DEBUG

    if (NULL != _hdpa)
    {
        for (int i = 0; i < DPA_GetPtrCount(_hdpa); i++)
        {
            ExtensionItem* pItem = (ExtensionItem*)DPA_GetPtr(_hdpa, i);
            if (!pItem->fButton)
                continue;

            VARIANTARG varArg;

            if (SUCCEEDED(pItem->pIBE->GetProperty(TBEX_BUTTONTEXT, &varArg)))
            {
                // We need to double-null terminate the string!
                WCHAR szBuf[70];    // should be ample for button text!
                ZeroInit(szBuf, sizeof(szBuf));
                StrNCpy(szBuf, varArg.bstrVal, ARRAYSIZE(szBuf) - 2);
                LRESULT iResult;

                if (SUCCEEDED(pxtb->AddString(pguidCommandGroup, 0, (LPARAM)szBuf, &iResult)))
                {
                    pItem->iStringID = (SHORT)iResult;
                }

                VariantClear(&varArg);
            }
        }
    }

    return S_OK;
}

int CBrowserExtension::_GetCmdIdFromClsid(LPCWSTR pszGuid)
{
    DWORD dwDisposition;
    HRESULT hr = S_OK;
    int nReturn = DVIDM_MENUEXT_FIRST; 

    HKEY hkeyExtensionMapping;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Internet Explorer\\Extensions\\CmdMapping"), 0, NULL, 0,
                       KEY_READ | KEY_WRITE, NULL, &hkeyExtensionMapping, &dwDisposition) == ERROR_SUCCESS)
    {
        DWORD dwType = REG_DWORD, dwData, cbData = sizeof(dwData);
        
        if ( (SHQueryValueEx(hkeyExtensionMapping, pszGuid, NULL, &dwType, &dwData, &cbData) == ERROR_SUCCESS) &&
             (dwType == REG_DWORD) )
        {
            //the item has a mapping
            nReturn = dwData;
        }
        else
        {
            //it's a new item, get and store the next available id in the default value of the Mapping key
            if ( (SHQueryValueEx(hkeyExtensionMapping, L"NextId", NULL, &dwType, &dwData, &cbData) != ERROR_SUCCESS) ||
                 (dwType != REG_DWORD) )
            {
                dwData = DVIDM_MENUEXT_FIRST;
            }
            nReturn = dwData;

            dwType = REG_DWORD;
            cbData = sizeof(dwData);
            EVAL(SHSetValueW(hkeyExtensionMapping, NULL, pszGuid, dwType, &dwData, cbData) == ERROR_SUCCESS);

            dwData++;
            ASSERT(dwData < DVIDM_MENUEXT_LAST); //ugh, we've used up our whole range. we need to look for holes.
            EVAL(SHSetValueW(hkeyExtensionMapping, NULL, L"NextId", dwType, &dwData, cbData) == ERROR_SUCCESS);
        }
        RegCloseKey(hkeyExtensionMapping);
    }

    return nReturn;
}

int CBrowserExtension::_GetIdpaFromCmdId(int nCmdId)
{
    if (NULL != _hdpa)
    {
        for (int i = 0; i < DPA_GetPtrCount(_hdpa); i++)
        {
            ExtensionItem* pItem = (ExtensionItem*)DPA_GetPtr(_hdpa, i);
            if (pItem->idCmd == nCmdId)
                return i;
        }
    }
    return -1;
}

// *** IOleCommandTarget methods ***

HRESULT CBrowserExtension::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
    DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    if (!pguidCmdGroup)
        return E_INVALIDARG;

    if (IsEqualGUID(*pguidCmdGroup, CLSID_ToolbarExtButtons))
    {
        int iCmd = _GetIdpaFromCmdId(nCmdID);

        if (iCmd >= 0 && iCmd < DPA_GetPtrCount(_hdpa))
        {
            ExtensionItem* pItem = (ExtensionItem*)DPA_GetPtr(_hdpa, iCmd);
            if (pItem)
                return IUnknown_Exec(pItem->pIBE, NULL, 0, 0, NULL, NULL);
        }
    }
    else if (IsEqualGUID(*pguidCmdGroup, CLSID_PrivBrowsExtCommands))
    {
        switch (nCmdID)
        {
        case PBEC_GETSTRINGINDEX:
            if (pvarargIn && pvarargIn->vt == VT_I4)
            {
                pvarargIn->lVal = _uStringIndex;
                return S_OK;
            }
            break;
        }
    }

    return E_FAIL;
}

HRESULT CBrowserExtension::QueryStatus(const GUID *pguidCmdGroup,
    ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    if (!pguidCmdGroup)
        return E_INVALIDARG;

    if (IsEqualGUID(*pguidCmdGroup, CLSID_ToolbarExtButtons))
    {
        for (ULONG i = 0; i < cCmds; i++)
        {
            int iCmd = _GetIdpaFromCmdId(rgCmds[i].cmdID);

            if (iCmd >= 0 && iCmd < DPA_GetPtrCount(_hdpa))
            {
                ExtensionItem* pItem = (ExtensionItem*)DPA_GetPtr(_hdpa, iCmd);
                if (pItem)
                {
                    // BUGBUG: I don't think this has ever worked.  The command id
                    // isn't the same as the one we use in Exec.
                    IUnknown_QueryStatus(pItem->pIBE, pguidCmdGroup, 1, &rgCmds[i], pcmdtext);
                }
            }
        }

        return S_OK;
    }

    return E_FAIL;
}

//
// This function is a helper for the destructor.  It is also called by Update() so that if we are ever asked
// to Update() we first kill all of our cached information and then we go to the registry...
//

void CBrowserExtension::_FreeItems(void)
{
    if (_hdpa)
    {
        for (int i = DPA_GetPtrCount(_hdpa) - 1; i >= 0; --i)
        {
            ExtensionItem* pItem = (ExtensionItem*)DPA_DeletePtr(_hdpa, i);

            IUnknown_SetSite(pItem->pIBE, NULL);
            pItem->pIBE->Release();
            delete pItem;
        }
    }
}

// this help function is used to isolate the menu-specific
// processing. after using this helper to fill out the BROWSEXT_MENU_INFO
// struct, the OnCustomizableMenuPopup is able to do menu-inspecific
// processing.

HRESULT
CBrowserExtension::_GetCustomMenuInfo(HMENU hMenuParent, HMENU hMenu, BROWSEXT_MENU_INFO * pMI)
{
    HRESULT hr;

    RIP(IS_VALID_HANDLE(hMenuParent, MENU));
    RIP(IS_VALID_HANDLE(hMenu, MENU));
    RIP(IS_VALID_WRITE_PTR(pMI, BROWSEXT_MENU_INFO *));

    hr = E_FAIL;
    pMI->idmMenu = 0;

    // set idmMenu, idmPlaceholder, and idmModMarker to values
    // reflecting whichever menu's popup we're currently handling

    if (GetMenuFromID(hMenuParent, FCIDM_MENU_HELP) == hMenu)
    {
        pMI->idmMenu = FCIDM_MENU_HELP;
        pMI->idmPlaceholder = FCIDM_HELP_EXT_PLACEHOLDER;
        pMI->idmModMarker = FCIDM_HELP_EXT_MOD_MARKER;
    }
    else if (GetMenuFromID(hMenuParent, FCIDM_MENU_TOOLS) == hMenu)
    {
        pMI->idmMenu = FCIDM_MENU_TOOLS;
        pMI->idmPlaceholder = FCIDM_TOOLS_EXT_PLACEHOLDER;
        pMI->idmModMarker = FCIDM_TOOLS_EXT_MOD_MARKER;
    }

    // set iInsert. using a constant insertion index
    // instead of always inserting by command at
    // the placeholder makes it easier later when
    // we have to stick in the final separator to
    // isolate the custom item group.

    if (pMI->idmMenu != 0)
    {
        int i;
        int cItems;

        cItems = GetMenuItemCount(hMenu);

        for (i = 0; i < cItems; i++)
        {
            MENUITEMINFO    mii;
            BOOL            f;

            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_ID;

            f = GetMenuItemInfo(hMenu, i, TRUE, &mii);

            if (f && mii.wID == pMI->idmPlaceholder)
            {
                pMI->iInsert = i;
                hr = S_OK;
                break;
            }
        }
    }

    return hr;
}

// note, this popup handler can't easily tell whether an item
// has been removed from the DPA. if you remove any items from the
// DPA it is your responsibility to delete them from the menu
// also, if they live on a menu

HRESULT CBrowserExtension::OnCustomizableMenuPopup(HMENU hMenuParent, HMENU hMenu)
{
    HRESULT             hr;
    BROWSEXT_MENU_INFO  menuInfo;

    RIP(IS_VALID_HANDLE(hMenu, MENU));

    hr = _GetCustomMenuInfo(hMenuParent, hMenu, &menuInfo);
    if (SUCCEEDED(hr) && _hdpa != NULL)
    {
        BOOL    fItemInserted;
        UINT    cItems;
        UINT    i;

        ASSERT(IS_VALID_HANDLE(_hdpa, DPA));

        fItemInserted = FALSE;

        // check each extension object we currently have
        // to see whether any of them should go into this
        // menu

        cItems = (UINT)DPA_GetPtrCount(_hdpa);

        for (i = 0; i < cItems; i++)
        {
            ExtensionItem * pItem;

            pItem = (ExtensionItem *)DPA_GetPtr(_hdpa, i);
            ASSERT(IS_VALID_READ_PTR(pItem, ExtensionItem));

            // does this item go into the menu we're currently
            // customizing?

            if (pItem->idmMenu == menuInfo.idmMenu)
            {
                MENUITEMINFO        mii;
                IOleCommandTarget * pOCT;

                mii.fMask   = MIIM_ID;
                mii.wID     = pItem->idCmd;
                mii.cbSize  = sizeof(mii);

                // set the MENUITEMINFO's state information, if applicable

                ASSERT(IS_VALID_CODE_PTR(pItem->pIBE, IBrowserExtension));

                hr = pItem->pIBE->QueryInterface(IID_IOleCommandTarget, (void **)&pOCT);
                if (SUCCEEDED(hr))
                {
                    OLECMD oleCmd = {OLECMDID_OPEN,};

                    ASSERT(IS_VALID_CODE_PTR(pOCT, IOleCommandTarget));

                    hr = pOCT->QueryStatus(NULL, 1, &oleCmd, NULL);
                    if (SUCCEEDED(hr))
                    {
                        mii.fMask |= MIIM_STATE;
                        mii.fState = 0;

                        // enabled state

                        if (oleCmd.cmdf & OLECMDF_ENABLED)
                        {
                            mii.fState |= MFS_ENABLED;
                        }
                        else
                        {
                            mii.fState |= MFS_DISABLED;
                        }

                        // checked state

                        if (oleCmd.cmdf & OLECMDF_LATCHED)
                        {
                            mii.fState |= MFS_CHECKED;
                        }
                        else
                        {
                            mii.fState |= MFS_UNCHECKED;
                        }
                    }

                    pOCT->Release();
                }

                // get the menu text.
                // this changing is an unlikely scenario, but if we're truly
                // supporting dynamic customization, then we need to allow for
                // this possibility.

                VARIANTARG  varArg;

                hr = pItem->pIBE->GetProperty(TMEX_MENUTEXT, &varArg);
                if (SUCCEEDED(hr))
                {
                    BOOL    fItemExists;

                    ASSERT(varArg.vt == VT_BSTR);
                    ASSERT(IS_VALID_STRING_PTR(varArg.bstrVal, -1));

                    fItemExists = GetMenuItemInfo(hMenu, mii.wID, FALSE, &mii);

                    mii.fMask |= MIIM_TYPE;
                    mii.fType = MFT_STRING;
                    mii.cch = SysStringLen(varArg.bstrVal);
                    mii.dwTypeData = varArg.bstrVal;

                    if (fItemExists)
                    {
                        // update the old item using current info

                        SetMenuItemInfo(hMenu, mii.wID, FALSE, &mii);
                    }
                    else
                    {
                        // create a new item using current info

                        if (InsertMenuItem(hMenu, menuInfo.iInsert, TRUE, &mii))
                        {
                            fItemInserted = TRUE;
                        }
                    }

                    VariantClear(&varArg);
                }

            }
        }

        if (fItemInserted)
        {
            MENUITEMINFO    mii;
            BOOL            fModMarkerExists;

            // since we made an insertion, we need to insert
            // a separator, but only if we didn't do it already

            mii.cbSize = sizeof(mii);
            mii.fMask = 0;

            fModMarkerExists = GetMenuItemInfo(hMenu, menuInfo.idmModMarker, FALSE, &mii);

            if (!fModMarkerExists)
            {
                mii.fMask = MIIM_ID | MIIM_TYPE;
                mii.wID = menuInfo.idmModMarker;
                mii.fType = MFT_SEPARATOR;

                InsertMenuItem(hMenu, menuInfo.iInsert, TRUE, &mii);
            }
        }

        // the only thing that is guaranteed to be a complete failure
        // if if we failed to get the info for the menu doing the popup.
        // otherwise, despite the possibility that any particular insertion
        // attempt might have failed, there are potentially many custom
        // items. though some might fail, some might succeed. in either
        // we'll return overall success, because we successfully did the
        // best we could with the items that were present.
        // at least we didn't crash :)

        hr = S_OK;
    }

    return hr;
}

HRESULT CBrowserExtension::OnMenuSelect(UINT nCmdID)
{
    VARIANT varArg;
    HRESULT hr = E_FAIL;

    // We better have stored our menu extensions if we are at this point
    ASSERT(_hdpa != NULL);
    int i = _GetIdpaFromCmdId(nCmdID);
    if (i >= 0 && i < DPA_GetPtrCount(_hdpa))
    {
        ExtensionItem* pItem = (ExtensionItem*)DPA_GetPtr(_hdpa, i);
        ASSERT(pItem->idmMenu != 0);

        hr = pItem->pIBE->GetProperty(TMEX_STATUSBARTEXT, &varArg);
        if (SUCCEEDED(hr) && varArg.vt == VT_BSTR)
        {
            // Set the Status Bar Text
            if (_pISB)
            {
                _pISB->SetStatusTextSB(varArg.bstrVal);
            }
            VariantClear(&varArg);

            hr = S_OK;
        }
    }
    return hr;
}

// Create an image list for the Cut/Copy/Paste buttons
CBrowserExtension::CImageCache CBrowserExtension::_rgImages[3];
/* =
{
    {IDB_IETOOLBAR16,      IDB_IETOOLBARHOT16,      NULL, NULL, 0},
    {IDB_IETOOLBAR,        IDB_IETOOLBARHOT,        NULL, NULL, 0},
    {IDB_IETOOLBARHICOLOR, IDB_IETOOLBARHOTHICOLOR, NULL, NULL, 0},
};
*/

//
// Get the image list for the toolbar. These image lists are shared between instances so
// the caller must call _ReturnImageLists when finished with them.  The index returned from this
// functions is passed to _ReturnImageLists.
//
UINT CBrowserExtension::_GetImageLists(CImageList** ppimlDef, CImageList** ppimlHot, BOOL fSmall)
{
    //
    // Get the index into our image cache
    //   16 color 16x16 (small)
    //   16 color 20x20
    //   256 color 20x20
    //
    int i = fSmall ? 0 : 1;
    if (!fSmall && SHGetCurColorRes() > 8)
    {
        ++i;
    }

    int cx = fSmall ? 16 : 20;

    //
    // Create the images if necessary
    //
    ENTERCRITICAL;

    if (_rgImages[0].uiResDef == 0)
    {
        _rgImages[0].uiResDef = IDB_IETOOLBAR16;
        _rgImages[0].uiResHot = IDB_IETOOLBARHOT16;
        _rgImages[1].uiResDef = IDB_IETOOLBAR;
        _rgImages[1].uiResHot = IDB_IETOOLBARHOT;
        _rgImages[2].uiResDef = IDB_IETOOLBARHICOLOR;
        _rgImages[2].uiResHot = IDB_IETOOLBARHOTHICOLOR;
    }

    if (!_rgImages[i].imlDef.HasImages())
    {
        _rgImages[i].imlDef = ImageList_LoadImage(HINST_THISDLL,
                                           MAKEINTRESOURCE(_rgImages[i].uiResDef),
                                           cx, 0, RGB( 255, 0, 255 ),
                                           IMAGE_BITMAP, LR_CREATEDIBSECTION);
    }

    if (!_rgImages[i].imlHot.HasImages())
    {
        _rgImages[i].imlHot = ImageList_LoadImage(HINST_THISDLL,
                                           MAKEINTRESOURCE(_rgImages[i].uiResHot),
                                           cx, 0, RGB( 255, 0, 255 ),
                                           IMAGE_BITMAP, LR_CREATEDIBSECTION);
    }

    //
    // Add the custom buttons to our image lists
    //
    if (_rgImages[i].imlHot.HasImages() && _rgImages[i].imlHot.HasImages())
    {
        _AddCustomImagesToImageList(_rgImages[i].imlDef, _rgImages[i].imlHot, fSmall);
    }

    ++_rgImages[i].cUsage;

    *ppimlDef = &_rgImages[i].imlDef;
    *ppimlHot = &_rgImages[i].imlHot;
    LEAVECRITICAL;

    return i;
}

//
// Called when the imagelist indicated by uiIndex is not longer used by this instance
//
void CBrowserExtension::_ReleaseImageLists(UINT uiIndex)
{
    if (uiIndex >= ARRAYSIZE(_rgImages))
    {
        return;
    }

    ENTERCRITICAL;

    ASSERT(_rgImages[uiIndex].cUsage >= 1);

    // If the image lists are no longer used, we can free them
    if (--_rgImages[uiIndex].cUsage == 0)
    {
        _rgImages[uiIndex].imlDef.FreeImages();
        _rgImages[uiIndex].imlHot.FreeImages();
    }
    LEAVECRITICAL;
}

//+-------------------------------------------------------------------------
// Constructor
//--------------------------------------------------------------------------
CImageList::CImageList(HIMAGELIST himl)
:   _himl(himl)
{
    ASSERT(_hdpa == NULL);
}

//+-------------------------------------------------------------------------
// Destructor
//--------------------------------------------------------------------------
CImageList::~CImageList()
{
    FreeImages();
}

//+-------------------------------------------------------------------------
// Frees an association item from our dpa
//--------------------------------------------------------------------------
int CImageList::_DPADestroyCallback(LPVOID p, LPVOID d)
{
    delete (ImageAssoc*)p;
    return 1;
}

//+-------------------------------------------------------------------------
// Frees our image list and inex associations
//--------------------------------------------------------------------------
void CImageList::FreeImages()
{
    if (_hdpa)
    {
        DPA_DestroyCallback(_hdpa, _DPADestroyCallback, 0);
        _hdpa = NULL;
    }
    if (_himl)
    {
        ImageList_Destroy(_himl);
        _himl = NULL;
    }
}

//+-------------------------------------------------------------------------
// Updates the image list
//--------------------------------------------------------------------------
CImageList& CImageList::operator=(HIMAGELIST himl)
{
    if (himl != _himl)
    {
        FreeImages();
        _himl = himl;
    }
    return *this;
}

//+-------------------------------------------------------------------------
// Returns the index of the images associated with rguid.  Returns -1 if not
// found.
//--------------------------------------------------------------------------
int CImageList::GetImageIndex(REFGUID rguid)
{
    int iIndex = -1;

    if (_hdpa)
    {
        ASSERT(_himl);

        for (int i=0; i < DPA_GetPtrCount(_hdpa); ++i)
        {
            ImageAssoc* pAssoc = (ImageAssoc*)DPA_GetPtr(_hdpa, i);
            if (IsEqualGUID(pAssoc->guid, rguid))
            {
                return pAssoc->iImage;
            }
        }
    }
    return iIndex;
}

//+-------------------------------------------------------------------------
// Adds the icon to the image list and returns the index.  If the image is
// already present, the existing index is returned.  Returns -1 on failure.
//--------------------------------------------------------------------------
int CImageList::AddIcon(HICON hicon, REFGUID rguid)
{
    ASSERT(hicon != NULL);

    // First see is we have already added this image
    int iIndex = GetImageIndex(rguid);
    if (iIndex == -1)
    {
        // Make sure we have a dpa to store our items
        if (NULL == _hdpa)
        {
            _hdpa = DPA_Create(5);
        }

        if (_hdpa && _himl)
        {
            // Add the icon to our image list
            iIndex = ImageList_AddIcon(_himl, hicon);
            if (iIndex != -1)
            {
                ImageAssoc* pAssoc = new ImageAssoc;
                if (pAssoc)
                {
                    pAssoc->guid = rguid;
                    pAssoc->iImage = iIndex;
                    DPA_AppendPtr(_hdpa, pAssoc);
                }
            }
        }
    }
    return iIndex;
}


