#include <urlmon.h>
#include "item.h"
#include <webcheck.h>
#include "utils.h"
#include "parseinf.h"

#include <mluisupp.h>

extern "C" HRESULT GetControlFolderPath(LPTSTR lpszDir, ULONG ulSizeBuf);
typedef HRESULT (STDAPICALLTYPE *PFNASYNCINSTALLDU)(LPCWSTR,LPCWSTR, LPCWSTR, DWORD,
                                                    DWORD,LPCWSTR, IBindCtx *, LPVOID, DWORD );


// registered clipboard formats
//UINT g_cfFileDescriptor = 0;
//UINT g_cfFileContents = 0;
//UINT g_cfURL = 0;
UINT g_cfPrefDropEffect = 0;

///////////////////////////////////////////////////////////////////////////////
// CControlItem methods.

CControlItem::CControlItem() 
{
    DebugMsg(DM_TRACE, TEXT("ci - CControlItem() called."));
    DllAddRef();
    m_cRef = 1;
    m_piciUpdate = NULL;
    m_pcpidlUpdate = NULL;
    m_pcdlbsc = NULL;
}        

CControlItem::~CControlItem()
{
    Assert(m_cRef == 0);                 // we should have zero ref count here

    DebugMsg(DM_TRACE, TEXT("ci - ~CControlItem() called."));

    LocalFree((HLOCAL)m_ppcei);

    if (m_pCFolder != NULL)
        m_pCFolder->Release();          // release the pointer to the sf

    DllRelease();
}

HRESULT CControlItem::Initialize(CControlFolder *pCFolder, UINT cidl, LPCITEMIDLIST *ppidl)
{
    m_ppcei = (LPCONTROLPIDL*)LocalAlloc(LPTR, cidl * sizeof(LPCONTROLPIDL));
    if (m_ppcei == NULL)
        return E_OUTOFMEMORY;
    
    m_cItems = cidl;
    m_pCFolder = pCFolder;

    for (UINT i = 0; i < cidl; i++)
        m_ppcei[i] = (LPCONTROLPIDL)(ppidl[i]);

    m_pCFolder->AddRef();      // we're going to hold onto this pointer, so
                               // we need to AddRef it.
    return NOERROR;
}        

HRESULT CControlItem_CreateInstance(
                               CControlFolder *pCFolder,
                               UINT cidl, 
                               LPCITEMIDLIST *ppidl, 
                               REFIID riid, 
                               void **ppvOut)
{
    *ppvOut = NULL;                 // null the out param

//    if (!_ValidateIDListArray(cidl, ppidl))
//        return E_FAIL;

    CControlItem *pCItem = new CControlItem;
    if (pCItem == NULL)
        return E_OUTOFMEMORY;

    HRESULT hr = pCItem->Initialize(pCFolder, cidl, ppidl);
    if (SUCCEEDED(hr))
    {
        hr = pCItem->QueryInterface(riid, ppvOut);
    }
    pCItem->Release();

    if (g_cfPrefDropEffect == 0)
    {
//        g_cfFileDescriptor = RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR); // "FileContents"
//        g_cfFileContents = RegisterClipboardFormat(CFSTR_FILECONTENTS);     // "FileDescriptor"
//        g_cfURL = RegisterClipboardFormat(TEXT("UniformResourceLocator"));  // "UniformResourceLocator"
        g_cfPrefDropEffect = RegisterClipboardFormat(TEXT("Preferred DropEffect"));// "Preferred DropEffect"
    }

    return hr;
}

HRESULT CControlItem::QueryInterface(REFIID iid, void **ppv)
{
    DebugMsg(DM_TRACE, TEXT("ci - QueryInterface() called."));
    
    if ((iid == IID_IUnknown) || (iid == IID_IContextMenu))
    {
        *ppv = (LPVOID)(IContextMenu*)this;
    }
    else if (iid == IID_IDataObject) 
    {
        *ppv = (LPVOID)(IDataObject*)this;
    }
    else if (iid == IID_IExtractIcon) 
    {
        *ppv = (LPVOID)(IExtractIcon*)this;
    }
    else if (iid == CLSID_ControlFolder)    // really should be CLSID_ControlFolderItem
    {
        *ppv = (void *)this; // for our friends
    }
    else
    {
        *ppv = NULL;     // null the out param
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG CControlItem::AddRef()
{
    return ++m_cRef;
}

ULONG CControlItem::Release()
{
    if (--m_cRef)
        return m_cRef;

    delete this;
    return 0;   
}

HRESULT CControlItem::GetData(LPFORMATETC pFEIn, LPSTGMEDIUM pSTM)
{
    HRESULT hres;

#ifdef _DEBUG_
    TCHAR szName[64];
    if (!GetClipboardFormatName(pFEIn->cfFormat, szName, sizeof(szName)))
        wsprintf(szName, "#%d", pFEIn->cfFormat);

    DebugMsg(DM_TRACE, TEXT("ci - do - GetData(%s)"), szName);
#endif

    pSTM->hGlobal = NULL;
    pSTM->pUnkForRelease = NULL;

    if ((pFEIn->cfFormat == g_cfPrefDropEffect) && (pFEIn->tymed & TYMED_HGLOBAL))
        hres = CreatePrefDropEffect(pSTM);
    else 
        hres = E_FAIL;      // FAIL WHEN YOU DON'T SUPPORT IT!!!

    return hres;
}

HRESULT CControlItem::GetDataHere(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
    DebugMsg(DM_TRACE, TEXT("ci - do - GetDataHere() called."));
    return E_NOTIMPL;
}

HRESULT CControlItem::QueryGetData(LPFORMATETC pFEIn)
{
#ifdef _DEBUG_
    TCHAR szName[64];
    if (!GetClipboardFormatName(pFEIn->cfFormat, szName, sizeof(szName)))
        wsprintf(szName, "#%d", pFEIn->cfFormat);

    DebugMsg(DM_TRACE, TEXT("ci - do - QueryGetData(%s)"), szName);
#endif

    if (pFEIn->cfFormat == g_cfPrefDropEffect)
    {
        DebugMsg(DM_TRACE, TEXT("                  format supported."));
        return NOERROR;
    }

    return S_FALSE;
}

HRESULT CControlItem::GetCanonicalFormatEtc(LPFORMATETC pFEIn, LPFORMATETC pFEOut)
{
    DebugMsg(DM_TRACE, TEXT("ci - do - GetCanonicalFormatEtc() called."));
    return DATA_S_SAMEFORMATETC;
}

HRESULT CControlItem::SetData(LPFORMATETC pFE, LPSTGMEDIUM pSTM, BOOL fRelease)
{
    DebugMsg(DM_TRACE,TEXT("ci - do - SetData() called."));
    return E_NOTIMPL;
}

HRESULT CControlItem::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC *ppEnum)
{
    FORMATETC ControlFmte[1] = {
        {(CLIPFORMAT)g_cfPrefDropEffect, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL}
    };

    DebugMsg(DM_TRACE, TEXT("ci - do - EnumFormatEtc() called."));

    return SHCreateStdEnumFmtEtc(ARRAYSIZE(ControlFmte), ControlFmte, ppEnum);
}

HRESULT CControlItem::DAdvise(LPFORMATETC pFE, DWORD grfAdv, LPADVISESINK pAdvSink,
    LPDWORD pdwConnection)
{
    DebugMsg(DM_TRACE, TEXT("ci - do - DAdvise() called."));
    return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT CControlItem::DUnadvise(DWORD dwConnection)
{
    DebugMsg(DM_TRACE, TEXT("ci - do - DUnAdvise() called."));
    return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT CControlItem::EnumDAdvise(LPENUMSTATDATA *ppEnum)
{
    DebugMsg(DM_TRACE, TEXT("ci - do - EnumAdvise() called."));
    return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT CControlItem::CreatePrefDropEffect(LPSTGMEDIUM pSTM)
{    
    pSTM->tymed = TYMED_HGLOBAL;
    pSTM->pUnkForRelease = NULL;
    
    pSTM->hGlobal = GlobalAlloc(GPTR, sizeof(DWORD));

    if (pSTM->hGlobal)
    {
        *((LPDWORD)pSTM->hGlobal) = DROPEFFECT_COPY;
        return S_OK;
    }

    return E_OUTOFMEMORY;    
}

HRESULT CControlItem::Remove(HWND hwnd)
{
    TCHAR szMsg[MESSAGE_MAXSIZE];
    TCHAR szBuf[MESSAGE_MAXSIZE];

    if ( !g_fAllAccess )
    {
        // The current user does not have the access privileges to modify the
        // keys we need to tweak to remove a control, so let 'em know and bail
        // out quickly.
        MLLoadString(IDS_WARNING_USERNOACCESS, szMsg, ARRAYSIZE(szMsg));
        MLLoadString(IDS_MBTITLE_REMOVECONTROL, szBuf, ARRAYSIZE(szBuf));
        MessageBox(hwnd, szMsg, szBuf, MB_OK|MB_ICONWARNING);
        return S_FALSE;
    }

    szMsg[0] = '\0';

    if (m_cItems == 1)
    {
//        if(!PathFileExists(GetStringInfo(m_ppcei[0], SI_LOCATION)) ||
//           IsModuleRemovable(GetStringInfo(m_ppcei[0], SI_LOCATION)))
        {
            MLLoadString(IDS_WARNING_SINGLEREMOVAL, szBuf, ARRAYSIZE(szBuf));
            wsprintf(szMsg, szBuf, GetStringInfo(m_ppcei[0], SI_CONTROL));
        }
    }
    else
    {
        MLLoadString(IDS_WARNING_MULTIPLEREMOVAL, szMsg, ARRAYSIZE(szMsg));
    }

    if (szMsg[0] != '\0')
    {
        MLLoadString(IDS_MBTITLE_REMOVECONTROL, szBuf, ARRAYSIZE(szBuf));

        if (MessageBox(hwnd, szMsg, szBuf, MB_YESNO | MB_ICONWARNING) != IDYES)
        {
            return S_FALSE;
        }
    }

    // set wait cursor
    HRESULT hr = S_OK;
    HCURSOR hCurOld = StartWaitCur();
    LPCTSTR pszTypeLibId = NULL;

    for (UINT i = 0; i < m_cItems; i++)
    {
        Assert(m_ppcei[i] != NULL);
        if (m_ppcei[i] == NULL)
        {
            hr = E_FAIL;
            break;
        }

        pszTypeLibId = GetStringInfo(m_ppcei[i], SI_TYPELIBID);
        if (SUCCEEDED(hr = RemoveControlByName2(
                                   GetStringInfo(m_ppcei[i], SI_LOCATION),
                                   GetStringInfo(m_ppcei[i], SI_CLSID),
                                   (pszTypeLibId[0] == '\0' ? NULL : pszTypeLibId),
                                   TRUE, (m_ppcei[i])->ci.dwIsDistUnit, FALSE)))
        {
            if ( hr == S_FALSE )
            {
                MLLoadString(
                      IDS_ERROR_NOUNINSTALLACTION, 
                      szBuf, 
                      ARRAYSIZE(szBuf));
                wsprintf(szMsg, szBuf, GetStringInfo(m_ppcei[i], SI_CONTROL));
                MLLoadString(
                      IDS_MBTITLE_NOUNINSTALLACTION,
                      szBuf,
                      ARRAYSIZE(szBuf));
                MessageBox(hwnd, szMsg, szBuf, MB_OK|MB_ICONWARNING);
            }

            GenerateEvent(
                     SHCNE_DELETE, 
                     m_pCFolder->m_pidl, 
                     (LPITEMIDLIST)(m_ppcei[i]), 
                     NULL);
        }
        else if (hr == STG_E_SHAREVIOLATION)
        {
            MLLoadString(
                  IDS_CONTROL_INUSE, 
                  szBuf, 
                  ARRAYSIZE(szBuf));
            wsprintf(szMsg, szBuf, GetStringInfo(m_ppcei[i], SI_CONTROL));
            MLLoadString(
                  IDS_MBTITLE_SHAREVIOLATION,
                  szBuf,
                  ARRAYSIZE(szBuf));
            MessageBox(hwnd, szMsg, szBuf, MB_OK|MB_ICONSTOP);
        }
        else
        {
            MLLoadString(
                  IDS_ERROR_REMOVEFAIL,
                  szBuf, 
                  ARRAYSIZE(szBuf));
            wsprintf(szMsg, szBuf, GetStringInfo(m_ppcei[i], SI_CONTROL));
            MLLoadString(
                  IDS_MBTITLE_REMOVEFAIL,
                  szBuf,
                  ARRAYSIZE(szBuf));
            MessageBox(hwnd, szMsg, szBuf, MB_OK|MB_ICONSTOP);
            break;
        }
    }

    EndWaitCur(hCurOld);

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
// IExtractIcon Methods

STDMETHODIMP CControlItem::GetIconLocation(
                            UINT uFlags,
                            LPSTR szIconFile,
                            UINT cchMax,
                            int *piIndex,
                            UINT *pwFlags)
{
    Assert(szIconFile != NULL);
    Assert(m_cItems == 1);

    if (szIconFile == NULL)
        return S_FALSE;

    *piIndex = 0;
    *pwFlags = 0;

    if (uFlags != GIL_FORSHELL)
        return S_FALSE;

    *pwFlags = GIL_NOTFILENAME|GIL_PERINSTANCE;

    if (cchMax > (UINT)lstrlen(GetStringInfo(m_ppcei[0], SI_LOCATION)))
    {
        lstrcpy(szIconFile, GetStringInfo(m_ppcei[0], SI_LOCATION));
        return NOERROR;
    }

    szIconFile[0] = '\0';
    return S_FALSE;
}

STDMETHODIMP CControlItem::Extract(
                    LPCSTR pszFile,
                    UINT nIconIndex,
                    HICON *phiconLarge,
                    HICON *phiconSmall,
                    UINT nIconSize)
{
    *phiconLarge = ExtractIcon(g_hInst, pszFile, nIconIndex);
    if (*phiconLarge == NULL)
    {   
        *phiconLarge = GetDefaultOCIcon( m_ppcei[0] );
        Assert(*phiconLarge != NULL);
    }
    *phiconSmall = *phiconLarge;

    return NOERROR;
}

///////////////////////////////////////////////////////////////////////////////
// IContextMenu Methods

const struct {
    LPCTSTR pszVerb;
    UINT idCmd;
} rgcmds[] = {
    {TEXT("Remove"), IDM_CTRL_REMOVECONTROL},
    {TEXT("Properties"),  IDM_CTRL_PROPERTIES},
    {TEXT("Update"), IDM_CTRL_UPDATE},
    {TEXT("Delete"), IDM_CTRL_REMOVECONTROL},
    {NULL, 0}  // terminator
};

int GetCmdID(LPCTSTR pszCmd)
{
    if ((DWORD_PTR)pszCmd <= 0xFFFF)
        return (int)LOWORD(pszCmd);

    for (int i = 0; rgcmds[i].pszVerb != NULL; i++)
        if (lstrcmpi(pszCmd, rgcmds[i].pszVerb) == 0)
            return rgcmds[i].idCmd;

    return -1;
}

HMENU LoadPopupMenu(UINT id, UINT uSubOffset)
{
    HMENU hmParent, hmPopup;

    hmParent = LoadMenu(MLGetHinst(), MAKEINTRESOURCE(id));
    if (!hmParent)
        return NULL;

    hmPopup = GetSubMenu(hmParent, uSubOffset);
    RemoveMenu(hmParent, uSubOffset, MF_BYPOSITION);
    DestroyMenu(hmParent);

    return hmPopup;
}

UINT MergePopupMenu(
                HMENU *phMenu, 
                UINT idResource, 
                UINT uSubOffset, 
                UINT indexMenu,  
                UINT idCmdFirst, 
                UINT idCmdLast)
{
    HMENU hmMerge;

    if (*phMenu == NULL)
    {
        *phMenu = CreatePopupMenu();
        if (*phMenu == NULL)
            return 0;

        indexMenu = 0;    // at the bottom
    }

    hmMerge = LoadPopupMenu(idResource, uSubOffset);
    if (!hmMerge)
        return 0;

    idCmdLast = Shell_MergeMenus(*phMenu, hmMerge, indexMenu, idCmdFirst, idCmdLast, MM_ADDSEPARATOR);
    
    DestroyMenu(hmMerge);
    return idCmdLast;
}

HRESULT CControlItem::QueryContextMenu(
                                  HMENU hmenu, 
                                  UINT indexMenu, 
                                  UINT idCmdFirst,
                                  UINT idCmdLast, 
                                  UINT uFlags)
{
    UINT idLastMerged = 0;

    DebugMsg(DM_TRACE, TEXT("ci - cm - QueryContextMenu() called."));
    
    if (uFlags & CMF_DVFILE)
    {
        idLastMerged = MergePopupMenu(
                            &hmenu,
                            IDR_FILE_MERGE, 
                            0, 
                            indexMenu, 
                            idCmdFirst,
                            idCmdLast);
        if (IsShowAllFilesEnabled()) {
            CheckMenuItem(hmenu, 1, MF_BYPOSITION | MF_CHECKED);
        }
        else {
            CheckMenuItem(hmenu, 1, MF_BYPOSITION | MF_UNCHECKED);
        }

    }
    else if (!(uFlags & CMF_VERBSONLY))
    {
        DWORD                     dwState = 0;

        // Must have a connection and not be working offline to be able
        // to update.

        if (InternetGetConnectedState(&dwState, 0) && !IsGlobalOffline()) {
            idLastMerged = MergePopupMenu(
                                &hmenu,
                                IDR_POPUP_CONTROLCONTEXT, 
                                0, 
                                indexMenu, 
                                idCmdFirst,
                                idCmdLast);
        }
        else {
            idLastMerged = MergePopupMenu(
                                &hmenu,
                                IDR_POPUP_CONTROLCONTEXT_NO_UPDATE,
                                0, 
                                indexMenu, 
                                idCmdFirst,
                                idCmdLast);
        }
        SetMenuDefaultItem(hmenu, idLastMerged - idCmdFirst, MF_BYPOSITION); // make the last menu, Properties, the default
    }

    return ResultFromShort(idLastMerged - idCmdFirst);    // number of menu items    
}

HRESULT CControlItem::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    UINT i;
    int idCmd = GetCmdID((LPCTSTR)(pici->lpVerb));
    HRESULT hres = S_OK;
//  LPOLESTR                 szMimeType = NULL;
//  LPOLESTR                 szExtension = NULL;
//  LPOLESTR                 szCodeBase = NULL;
//  IBindCtx                *pbc = NULL;
//  CodeDownloadBSC         *pCDLBSC = NULL;

    DebugMsg(DM_TRACE, TEXT("ci - cm - InvokeCommand() called."));

    if (idCmd == IDM_CTRL_REMOVECONTROL)
    {
        hres = Remove(pici->hwnd);
    }
    else if (idCmd == IDM_CTRL_SHOWALL) {
        ToggleShowAllFiles();
        GenerateEvent(SHCNE_UPDATEITEM, m_pCFolder->m_pidl, 0, NULL);
    }
    else
    {
        for (i = 0; i < m_cItems && SUCCEEDED(hres); i++)
            if (m_ppcei[i]) 
            {
                switch (idCmd)
                {
                case IDM_CTRL_PROPERTIES: 
                    hres = CreatePropDialog(pici->hwnd, m_ppcei[i]);
                    break;;

                case IDM_CTRL_UPDATE:
                    hres = Update( pici, m_ppcei[i] );
 /*
                    hres = CreateBindCtx(0, &pbc);
                    if (SUCCEEDED(hres)) {
                        LPITEMIDLIST pidlUpdate = ILCombine(m_pCFolder->m_pidl,(LPITEMIDLIST)(m_ppcei[i]));
                     
                        // destructor of CodeDownloadBSC will free pidlUpdate 
                        if ( pidlUpdate != NULL &&
                             (pCDLBSC = new CodeDownloadBSC( pici->hwnd, pidlUpdate )) != NULL && 
                             SUCCEEDED(hres = RegisterBindStatusCallback(pbc, pCDLBSC, NULL, 0)))
                        {
                            PFNASYNCINSTALLDU        pfnAsyncInstallDU;
                            HINSTANCE                hModule;

                            pCDLBSC->Release();
                            hModule = LoadLibrary("URLMON.DLL");

#ifdef UNICODE
                            WCHAR swzCodeBase =  (m_ppcei[i])->ci.szCodeBase;
                            WCHAR swzDUName = (m_ppcei[i])->ci.szCLSID;
#else
                            MAKE_WIDEPTR_FROMANSI(swzCodeBase, (m_ppcei[i])->ci.szCodeBase);
                            MAKE_WIDEPTR_FROMANSI(swzDUName, (m_ppcei[i])->ci.szCLSID);
#endif

                            pfnAsyncInstallDU = (PFNASYNCINSTALLDU)GetProcAddress((HMODULE)hModule, "AsyncInstallDistributionUnit");
                            pfnAsyncInstallDU( swzDUName, szMimeType, szExtension,
                                               0xFFFFFFFF, 0xFFFFFFFF,
                                               swzCodeBase,
                                               pbc,
                                               NULL, 0);
                            FreeLibrary(hModule);
                        } 
                        else
                        {
                            if ( pCDLBSC != NULL )
                                delete pCDLBSC;
                            else if ( pidlUpdate != NULL )
                                ILFree( pidlUpdate );
                        }

                        if (pbc != NULL) {
                            pbc->Release();
                        }
                    }
*/
                    break;

                default:
                    hres = E_FAIL;
                    break;
                }
            }
    }

    return hres;
}

HRESULT CControlItem::GetCommandString(
                                   UINT_PTR idCmd, 
                                   UINT uFlags, 
                                   UINT *pwReserved,
                                   LPTSTR pszName, 
                                   UINT cchMax)
{
    HRESULT hres = E_FAIL;

    DebugMsg(DM_TRACE, TEXT("ci - cm - GetCommandString() called."));

    pszName[0] = '\0';

    if (uFlags == GCS_VERB)
    {
        for (int i = 0; rgcmds[i].pszVerb != NULL; i++)
            if (idCmd == rgcmds[i].idCmd)
            {
                lstrcpyn(pszName, rgcmds[i].pszVerb, cchMax);
                hres = NOERROR;
            }
    }
    else if (uFlags == GCS_HELPTEXT)
    {
        hres = NOERROR;

        switch (idCmd)
        {
        case IDM_CTRL_REMOVECONTROL:
            MLLoadString(IDS_HELP_REMOVECONTROL, pszName, cchMax);
            break;
        case IDM_CTRL_PROPERTIES:
            MLLoadString(IDS_HELP_PROPERTIES, pszName, cchMax);
            break;
        case IDM_CTRL_UPDATE:
            MLLoadString(IDS_HELP_UPDATE, pszName, cchMax);
            break;
        default:
            hres = E_FAIL;
        }
    }

    return hres;
}

HRESULT
CControlItem::Update(LPCMINVOKECOMMANDINFO pici, LPCONTROLPIDL pcpidl )
{
    HRESULT         hres = NOERROR;

    m_piciUpdate = pici;
    m_pcpidlUpdate = pcpidl;

    if (pici->hwnd)
    {
        INT_PTR nRes = DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_OCUPDATE),
                                  pici->hwnd, CControlItem::DlgProc, (LPARAM)this);
    }

    return hres;
}

INT_PTR CControlItem::DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL fRet = TRUE;
    CControlItem* pctlitem = (CControlItem*)GetWindowLongPtr(hDlg, DWLP_USER);
    HRESULT     hr = S_OK;
    IBindCtx    *pbc = NULL;
    LPOLESTR    szMimeType = NULL;
    LPOLESTR    szExtension = NULL;
    TCHAR       szBuf[MESSAGE_MAXSIZE];

    switch (msg)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        pctlitem = (CControlItem*)lParam;

        MLLoadString(IDS_UPDATE_CAPTION, szBuf, ARRAYSIZE(szBuf));
        lstrcatn( szBuf, pctlitem->m_pcpidlUpdate->ci.szName, ARRAYSIZE(szBuf));
        SetWindowText( hDlg, szBuf );

        hr = CreateBindCtx(0, &pbc);
        if (SUCCEEDED(hr)) 
        {
            LPITEMIDLIST pidlUpdate = ILCombine(pctlitem->m_pCFolder->m_pidl,(LPITEMIDLIST)(pctlitem->m_pcpidlUpdate));
     
            // destructor of CodeDownloadBSC will free pidlUpdate
            // BUGBUG: if new succeeds but register fails, we'll deallocate pidlUpdate twice.
            if ( pidlUpdate != NULL &&
                 (pctlitem->m_pcdlbsc = new CodeDownloadBSC( pctlitem->m_piciUpdate->hwnd, hDlg, pidlUpdate )) != NULL && 
                 SUCCEEDED(hr = RegisterBindStatusCallback(pbc, pctlitem->m_pcdlbsc, NULL, 0)))
            {
                PFNASYNCINSTALLDU        pfnAsyncInstallDU;
                HINSTANCE                hModule;

                hModule = LoadLibrary("URLMON.DLL");

    #ifdef UNICODE
                WCHAR swzCodeBase =  pctlitem->m_pcpidlUpdate->ci.szCodeBase;
                WCHAR swzDUName = pctlitem->m_pcpidlUpdate->ci.szCLSID;
    #else
                MAKE_WIDEPTR_FROMANSI(swzCodeBase, pctlitem->m_pcpidlUpdate->ci.szCodeBase);
                MAKE_WIDEPTR_FROMANSI(swzDUName, pctlitem->m_pcpidlUpdate->ci.szCLSID);
    #endif

                pfnAsyncInstallDU = (PFNASYNCINSTALLDU)GetProcAddress((HMODULE)hModule, "AsyncInstallDistributionUnit");
                if ( pfnAsyncInstallDU != NULL )
                    hr = pfnAsyncInstallDU( swzDUName, szMimeType, szExtension,
                                           0xFFFFFFFF, 0xFFFFFFFF,
                                           swzCodeBase,
                                           pbc,
                                           NULL, 0);
                else
                    hr = E_FAIL;

                FreeLibrary(hModule);
            } 
            else
            {
                if ( pctlitem->m_pcdlbsc != NULL )
                {
                    delete pctlitem->m_pcdlbsc;
                    pctlitem->m_pcdlbsc = NULL;
                }
                else if ( pidlUpdate != NULL )
                    ILFree( pidlUpdate );
            }

            if (pbc != NULL) {
                pbc->Release();
            }
        }

        if ( SUCCEEDED(hr) )
        {
            Animate_Open(GetDlgItem(hDlg, IDC_DOWNLOADANIMATE), IDA_DOWNLOAD);
            Animate_Play(GetDlgItem(hDlg, IDC_DOWNLOADANIMATE), 0, -1, -1);
        }
        else
            EndDialog(hDlg, FALSE);
        fRet = 0;
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDCANCEL:
            Assert( pctlitem->m_pcdlbsc != NULL );
            hr = pctlitem->m_pcdlbsc->Abort();
            Assert( SUCCEEDED( hr ) );
            EndDialog(hDlg, FALSE);
            break;

        case DOWNLOAD_PROGRESS:
            SendMessage(GetDlgItem(hDlg, IDC_DOWNLOADPROGRESS), PBM_SETPOS,
                        lParam, 0);
            break;

        case DOWNLOAD_COMPLETE:
            if (lParam)
                SendMessage(GetDlgItem(hDlg, IDC_DOWNLOADPROGRESS), PBM_SETPOS,
                            100, 0);
            EndDialog(hDlg, lParam);
            break;
        }
        break;

    case WM_CLOSE:
        EndDialog(hDlg, FALSE);
        break;

    case WM_DESTROY:
        Assert( pctlitem->m_pcdlbsc != NULL );
        pctlitem->m_pcdlbsc->_hdlg = NULL;
        pctlitem->m_pcdlbsc->Release();
        break;

    default:
        fRet = FALSE;
    }

    return fRet;
}

BOOL CControlItem::IsGlobalOffline()
{
    DWORD   dwState = 0, dwSize = sizeof(DWORD);
    BOOL    fRet = FALSE;

    if(InternetQueryOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &dwState,
        &dwSize))
    {
        if(dwState & INTERNET_STATE_DISCONNECTED_BY_USER)
            fRet = TRUE;
    }

    return fRet;
}

