//****************************************************************************
//
//  Module:     RNAUI.DLL
//  File:       contain.c
//  Content:    This file contains the persistent-object-binding mechanism
//              which is slightly different from OLE's binding.
//  History:
//  01-07-93 GeorgeP     Modified from win\core\shell\commui\handler.c
//  05-05-93 ViroonT     Modified from win\core\shell\winutils\printer.c
//  01-17-94 ScottH      Changed over to shell monikers
//
//  Copyright (c) Microsoft Corporation 1991-1994
//
//****************************************************************************

#include "rnaui.h"
#include "contain.h"
#include <shguidp.h>       // Remote CLSID

//---------------------------------------------------------------------------
// Global parameters
//---------------------------------------------------------------------------

#pragma data_seg("SHAREDATA")
// This needs to be allocated in shared address space so other processes
// (RNAAPP) can call Remote_GenerateEvents via the wizard.
LPITEMIDLIST g_pidlRemote = NULL;
#pragma data_seg()

HRESULT CALLBACK Remote_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, LPVOID FAR* ppvOut);
void NEAR PASCAL ComposeDisplayPhone(LPSTR szPhoneNum, LPPHONENUM lpPhoneNum, DWORD cb);
BOOL FAR PASCAL  Remote_Setting (HWND hWnd);

STDMETHODIMP_(UINT) RemViewCB_AddRef(IShellFolderViewCB* psfvcb);
STDMETHODIMP_(UINT) RemViewCB_Release(IShellFolderViewCB* psfvcb);


//---------------------------------------------------------------------------
// RemView class
//---------------------------------------------------------------------------

typedef struct  tagConnStat {
    RASCONNSTATE    rasconnstate;
    UINT            uState;
}   CONNSTAT;

// These are the columns this folder supports in details view.
enum
    {
    ICOL_NAME = 0,
    ICOL_PHONE,
    ICOL_DEVICE,
    ICOL_STATUS,
    ICOL_MAX
    } ;

#pragma data_seg(DATASEG_READONLY)

const struct
    {
    UINT    icol;
    UINT    ids;
    UINT    cchCol;
    int     fmt;
    } c_cols[] = {
        {ICOL_NAME,   IDS_ICOL_NAME,    20, LVCFMT_LEFT   },
        {ICOL_PHONE,  IDS_ICOL_PHONE,   15, LVCFMT_RIGHT  },
        {ICOL_DEVICE, IDS_ICOL_DEVICE,  20, LVCFMT_LEFT   },
        {ICOL_STATUS, IDS_ICOL_STATUS,  20, LVCFMT_LEFT   },
        };

const TBBUTTON c_tbRemote[] = {
    { 0, RSVIDM_CREATE,  TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0L, -1 },
    { 1, RSVIDM_CONNECT, 0,               TBSTYLE_BUTTON, {0,0}, 0L, -1 },
    { 0, 0,              TBSTATE_ENABLED, TBSTYLE_SEP   , {0,0}, 0L, -1 },
    };

const CONNSTAT actState[] =
         {{RASCS_OpenPort,            IDS_OPENPORT},
          {RASCS_PortOpened,          IDS_PORTOPENED},
          {RASCS_ConnectDevice,       IDS_CONNECTDEVICE},
          {RASCS_DeviceConnected,     IDS_DEVICECONNECTED},
          {RASCS_AllDevicesConnected, IDS_DEVICECONNECTED},
          {RASCS_StartAuthentication, IDS_STARTAUTHENTICATION},
          {RASCS_Authenticate,        IDS_AUTHENTICATE},
	  {RASCS_PrepareForCallback,  IDS_CALLBACKPREP},
          {RASCS_WaitForCallback,     IDS_WAITRESPOND},
          {RASCS_Authenticated,       IDS_AUTHENTICATED},
          {RASCS_LogonNetwork,        IDS_NETLOGON},
          {RASCS_Connected,           IDS_CONNECTED},
          {RASCS_Disconnected,        IDS_DISCONNECTED} };
#pragma data_seg()


/*----------------------------------------------------------
    IShellFolderViewCB implementation
*/
typedef struct _RemView
    {
    IShellView          sv;
    IShellFolderViewCB  sfvcb;

    LPSHELLVIEW         psv;

    UINT                m_cref;

    LPITEMIDLIST        m_pidl;
    IShellFolder*       m_pshf;
    HWND                m_hwndMain;
    IShellFolderView*   m_psfv;
    } RemView, * PREMVIEW;


/*----------------------------------------------------------
Purpose: Get the selected data objects
Returns: standard hresult
Cond:    --
*/
HRESULT NEAR PASCAL RemView_GetSelectedObjects(
    PREMVIEW this,
    HWND hwnd,
    LPCITEMIDLIST * ppidlSel,
    UINT *lpcidl)
    {
    HRESULT hres = ResultFromScode(E_FAIL);
    LPCITEMIDLIST * apidl;

    // Assume failure
    *ppidlSel = NULL;

    *lpcidl = ShellFolderView_GetSelectedObjects(hwnd, &apidl);
    if (*lpcidl > 0 && apidl)
        {
        // We only ever care about the first item selected
        *ppidlSel = apidl[0];
        hres = NOERROR;
        // We are not supposed to free apidl
        }
    else if (-1 == *lpcidl)
        {
        hres = ResultFromScode(E_OUTOFMEMORY);
        }
    return hres;
    }


/*----------------------------------------------------------
Purpose: Merge the remote menu with the defview's menu bar.

Returns: standard hresult
Cond:    --
*/
HRESULT RemView_MergeMenu(
    PREMVIEW this,
    LPQCMINFO pinfo)
    {
    HMENU hmMain = pinfo->hmenu;

    // Merge the remote menu onto the menu that CDefView created.
    if (hmMain)
        {
        HMENU hmenu;

        hmenu = LoadMenu(ghInstance, MAKEINTRESOURCE(MENU_REMOTE));

        // Append the Dial-up server menu if it is installed
        //
        if (IsServerInstalled())
        {
          HMENU hmenuServ, hmenuPopup;

          hmenuPopup = GetSubMenu(hmenu, 0);
          hmenuServ = LoadMenu(ghInstance, MAKEINTRESOURCE(MENU_SERVER));
          Shell_MergeMenus(hmenuPopup, hmenuServ, 0x0FFFF,
                           0, 0x0FFFF, MM_ADDSEPARATOR);

          // Although the Dial-up server is installed, disabled the menu if it is
          // blocked by admin.
          //
          if (!(SuprvGetAdminConfig() & RNAADMIN_DIALIN))
              EnableMenuItem(hmenu, RSVIDM_DIALIN,  MF_BYCOMMAND | MF_GRAYED);

          DestroyMenu(hmenuServ);
        };

        if (hmenu)
            {
            Shell_MergeMenus(hmMain, hmenu, pinfo->indexMenu,
                             pinfo->idCmdFirst - RSVIDM_FIRST,
                             pinfo->idCmdLast, MM_SUBMENUSHAVEIDS);
            DestroyMenu(hmenu);
            }
        }

    return NOERROR;
    }


/*----------------------------------------------------------
Purpose: WM_COMMAND handler

Returns: varies
Cond:    --
*/
HRESULT NEAR PASCAL RemView_Command(
    PREMVIEW this,
    HWND hwnd,
    UINT uID)
    {
    switch (uID)
        {
    case RSVIDM_CONNECT:
        {
        LPCITEMIDLIST pidlSel;
        UINT          cpidl;

        // Connect or disconnect
        if (RemView_GetSelectedObjects(this, hwnd, &pidlSel, &cpidl) == NOERROR)
            {
            PSUBOBJ pso = (PSUBOBJ)pidlSel;

            ASSERT(cpidl == 1);
            Remote_Dial(hwnd, Subobj_GetName(pso));
            }
        break;
        }
    case RSVIDM_CREATE:
        // Create a new connection
        RunWizard(hwnd, CLIENT_WIZ);
        break;

    case RSVIDM_DIALIN:
        // Dial-In options
        RunDLLThread(hwnd, c_szDialUpServer, SW_SHOW);
        break;

    case RSVIDM_SETTING:
        // Global RNA settings
        Remote_Setting(hwnd);
        break;

        }

    return NOERROR;
    }


/*----------------------------------------------------------
Purpose: DVM_GETBUTTONS handler
Returns: --
Cond:    --
*/
void NEAR PASCAL RemView_OnGetButtons(
    PREMVIEW this,
    HWND hwndMain,
    UINT idCmdFirst,
    LPTBBUTTON ptbbutton)
    {
    UINT i;
    UINT iBtnOffset;
    IShellBrowser * psb = FileCabinet_GetIShellBrowser(hwndMain);
    TBADDBITMAP ab;

    // add the toolbar button bitmap, get it's offset
    ab.hInst = ghInstance;
    ab.nID   = IDB_REM_TB_SMALL;        // std bitmaps
    psb->lpVtbl->SendControlMsg(psb, FCW_TOOLBAR, TB_ADDBITMAP, 2, (LPARAM)&ab, &iBtnOffset);

    for (i = 0; i < ARRAYSIZE(c_tbRemote); i++)
        {
        ptbbutton[i] = c_tbRemote[i];

        if (!(c_tbRemote[i].fsStyle & TBSTYLE_SEP))
            {
            ptbbutton[i].idCommand += idCmdFirst;
            ptbbutton[i].iBitmap += iBtnOffset;
            }
        }
    }


/*----------------------------------------------------------
Purpose: DVM_INITMENUPOPUP handler
Returns: standard hresult
Cond:    --
*/
HRESULT RemView_InitMenuPopup(
    PREMVIEW this,
    HWND hwnd,
    UINT idCmdFirst,
    int nIndex,
    HMENU hmenu)
    {
    HRESULT hres;
    LPCITEMIDLIST pidlSel;
    UINT          cpidl;

    // Get the currently selected object
    hres = RemView_GetSelectedObjects(this, hwnd, &pidlSel, &cpidl);

    // Only enable Connect if there is one item selected and it is
    // not New Connection.
    ASSERT((hres != NOERROR) || (pidlSel != NULL));
    if ((hres == NOERROR) && (cpidl == 1) &&
        IsFlagClear(Subobj_GetFlags((PSUBOBJ)pidlSel), SOF_NEWREMOTE))
        {
        EnableMenuItem(hmenu, idCmdFirst+RSVIDM_CONNECT, MF_ENABLED);
        }
    else
        {
        EnableMenuItem(hmenu, idCmdFirst+RSVIDM_CONNECT, MF_GRAYED);
        }

    return NOERROR;
    }


/*----------------------------------------------------------
Purpose: DVM_GETDETAILSOF handler

Returns: E_NOTIMPL if iColumn is greater than supported range
         otherwise NOERROR

Cond:    --
*/
HRESULT PRIVATE RemView_OnGetDetailsOf(
    PREMVIEW this,
    HWND hwndMain,
    UINT iColumn,
    PDETAILSINFO lpDetails)
    {
    LPITEMIDLIST pidl = (LPITEMIDLIST)lpDetails->pidl;
    PSUBOBJ pso;
    PCONNENTRY pconnentry;

    if (iColumn >= ICOL_MAX)
        {
        // Finished with listing the columns.
        return E_NOTIMPL;
        }

    lpDetails->str.uType = STRRET_CSTR;
    lpDetails->str.cStr[0] = '\0';

    if (NULL == pidl)
        {
        LoadString(ghInstance, c_cols[iColumn].ids, lpDetails->str.cStr,
                   sizeof(lpDetails->str.cStr));
        lpDetails->fmt = c_cols[iColumn].fmt;
        lpDetails->cxChar = c_cols[iColumn].cchCol;
        return NOERROR;
        }

    pso = (PSUBOBJ)pidl;
    switch (iColumn)
        {
    case ICOL_NAME:
        lpDetails->str.uType = STRRET_OFFSET;
        lpDetails->str.uOffset = Subobj_GetName(pso) - (LPSTR)pso;
        break;

    case ICOL_PHONE:
    case ICOL_DEVICE:
        // Get the entry information
        pconnentry = RnaGetConnEntry(Subobj_GetName(pso), FALSE, TRUE);
        if (pconnentry)
            {
            if (ICOL_PHONE == iColumn)
                {
                ComposeDisplayPhone(lpDetails->str.cStr, &pconnentry->pn,
                                    sizeof(lpDetails->str.cStr));
                }
            else
                {
                lstrcpyn(lpDetails->str.cStr,
                         pconnentry->pDevConfig->di.szDeviceName,
                         sizeof(lpDetails->str.cStr));
                };

            RnaFreeConnEntry(pconnentry);
            }
        break;

    case ICOL_STATUS:
        {
        HRASCONN hrasconn;
        RASCONNSTATUS rcstat;
        UINT      i;

        // Get the connection handle
        //
        if ((hrasconn = Remote_GetConnHandle(Subobj_GetName(pso))) != NULL)
          {
          rcstat.dwSize = sizeof(rcstat);
          if (RasGetConnectStatus(hrasconn, &rcstat) == ERROR_SUCCESS)
            {
            for (i=0; i < (sizeof(actState)/sizeof(CONNSTAT)); i++)
              {
              // Update the state
              if (rcstat.rasconnstate == actState[i].rasconnstate)
                {
                LoadString(ghInstance, actState[i].uState,
                           lpDetails->str.cStr,
                           sizeof(lpDetails->str.cStr));
                break;
                }
              }
            }
          }
        }
        break;
        }
    return NOERROR;
    }


/*----------------------------------------------------------
Purpose: DVM_COLUMNCLICK handler

Returns: NOERROR
Cond:    --
*/
HRESULT PRIVATE RemView_OnColumnClick(
    PREMVIEW this,
    HWND hwndMain,
    UINT iColumn)
    {
    Assert(iColumn < ICOL_MAX);

    ShellFolderView_ReArrange(hwndMain, iColumn);
    return NOERROR;
    }


/*----------------------------------------------------------
Purpose: DVM_SELCHANGE handler

Returns: standard hresult
         E_FAIL means we did not update the status area
Cond:    --
*/
HRESULT NEAR PASCAL RemView_OnSelChange(
    PREMVIEW this,
    HWND hwndMain,
    UINT idCmdFirst)
    {
    IShellBrowser * psb = FileCabinet_GetIShellBrowser(hwndMain);
    HRESULT hres;
    LPCITEMIDLIST pidlSel;
    UINT          cpidl;
    BOOL          fEnable;

    // Get the currently selected object
    hres = RemView_GetSelectedObjects(this, hwndMain, &pidlSel, &cpidl);

    // Only enable Connect if there is one item selected and it is
    // not New Connection.
    ASSERT((hres != NOERROR) || (pidlSel != NULL));
    fEnable = ((hres == NOERROR) && (cpidl == 1) &&
               IsFlagClear(Subobj_GetFlags((PSUBOBJ)pidlSel), SOF_NEWREMOTE));

    // Enable/disable toolbar button
    Assert(psb);
    if (psb)
        {
        psb->lpVtbl->SendControlMsg(psb, FCW_TOOLBAR, TB_ENABLEBUTTON,
            idCmdFirst + RSVIDM_CONNECT,
            (LPARAM)fEnable, NULL);
        }
    return E_FAIL;     // (we did not update the status area)
    }


/*----------------------------------------------------------
Purpose: SFVM_GETVIEWS handler

Returns: --
Cond:    --
*/
#if 1
#include "sfvlist.h" // RemView_OnGetViews() definition
#else
#pragma data_seg(DATASEG_READONLY)
TCHAR const c_szExtViews[] = TEXT("ExtShellFolderViews");
TCHAR const c_szWinDir[] = TEXT("%WinDir%");
#pragma data_seg()

int RemView_GetKeyInfo(
    LPTSTR pszSubKey,
    HKEY* phk)
{
    int nKeys = 0;
    *phk = NULL;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, pszSubKey, 0, KEY_READ, phk))
    {
        RegQueryInfoKey(*phk, NULL, NULL, NULL, &nKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    return(nKeys);
}

int RemView_GetKeyData(HKEY hk, SFVVIEWSDATA* psfv)
{
    return(0);

#if 0
    TCHAR szKey[40];
    DWORD dwLen = SIZEOF(szKey);
    SHELLVIEWID vid;

    if (ERROR_SUCCESS==RegQueryValue(ckViews, NULL, szKey, (LONG*)&dwLen)
        && SUCCEEDED(SHCLSIDFromString(szKey, &vid)))
    {
        m_vidDef = vid;
        m_bGotDef = TRUE;
    }


    for (int i=0; ; ++i)
    {
        LONG lRet = RegEnumKey(ckViews, i, szKey, ARRAYSIZE(szKey));
        if (lRet == ERROR_MORE_DATA)
        {
            continue;
        }
        else if (lRet != ERROR_SUCCESS)
        {
            // I assume this is ERROR_NO_MORE_ITEMS
            break;
        }

        SFVVIEWSDATA sView;

        if (FAILED(SHCLSIDFromString(szKey, &sView.id)))
        {
            continue;
        }

        TCHAR szFile[ARRAYSIZE(sView.wszFile)];
        DWORD dwType;
        dwLen = SIZEOF(szFile);

        CRegKey ckView(ckViews, szKey);
        //
        // Checking for PersistFile and PersistString separately is for clarity
        // in the registry. The code doesn't pay attention to this difference.
        //
        if (ckView &&
            (ERROR_SUCCESS==RegQueryValueEx(ckView, TEXT("PersistFile"),
                NULL, &dwType, (LPBYTE)szFile, &dwLen) ||
             ERROR_SUCCESS==RegQueryValueEx(ckView, TEXT("PersistString"),
                NULL, &dwType, (LPBYTE)szFile, &dwLen))
            && REG_SZ==dwType)
        {
            StrToOleStr(sView.wszFile, szFile);
        }
        else
        {
            sView.wszFile[0] = '\0';
        }

        sView.lParam = 0;
        dwLen = SIZEOF(sView.lParam);
        if( ckView )
        {
            RegQueryValueEx(ckView, TEXT("LParam"),NULL, &dwType,
                (LPBYTE) &(sView.lParam), &dwLen );
        }
        
        Add(&sView);
    }
#endif
}

IEnumSFVViews* RemView_CreateIEnumSFVViews(SFVVIEWSDATA* psfv, int nsfv)
{
    return(NULL);
}

HRESULT PASCAL RemView_OnGetViews(
    SHELLVIEWID FAR *pvid,
    IEnumSFVViews FAR * FAR *ppObj)
{
    HKEY hkFolder, hkRna;
    int nFolderKeys, nRnaKeys, nKeys;
    SFVVIEWSDATA* psfv;

    *ppObj = NULL;

    // RNAs viewlist is the "folder" base class plus the "rna" guid
    nFolderKeys = RemView_GetKeyInfo(TEXT("folder\\shellex\\ExtShellFolderViews"), &hkFolder);
    nRnaKeys = RemView_GetKeyInfo(TEXT("CLSID\\{992CFFA0-F557-101A-88EC-00DD010CCC48}\\shellex\\ExtShellFolderViews"), &hkRna);
    nKeys = nFolderKeys + nRnaKeys;
    if (nKeys)
    {
        psfv = (SFVVIEWSDATA*)LocalAlloc(LPTR, nKeys * sizeof(*psfv));
        if (psfv)
        {
            // The number of valid keys we copy may be less than the number of registry entries
            nFolderKeys = RemView_GetKeyData(hkFolder, psfv);
            nRnaKeys = RemView_GetKeyData(hkRna, psfv+nFolderKeys);

            *ppObj = RemView_CreateIEnumSFVViews(psfv, nFolderKeys + nRnaKeys);

            if (!*ppObj)
                LocalFree(psfv);
        }
    }

    if (hkFolder)
        RegCloseKey(hkFolder);
    if (hkRna)
        RegCloseKey(hkRna);

    return S_OK;
}
#endif

/*----------------------------------------------------------
Purpose: Default View callback handler
Returns: standard hresult
Cond:    --
*/
HRESULT CALLBACK RemView_ViewCallback(
    LPSHELLVIEW psvOuter,
    LPSHELLFOLDER psf,
    HWND hwndMain,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
    {
    PREMVIEW this = IToClass(RemView, sv, psvOuter);
    HRESULT hres = NOERROR;

    switch (uMsg)
        {
    case DVM_WINDOWCREATED:
        // The folder currently doesn't allow users to arrange
        // icons arbitrarily.  There is a bug in defview which
        // sometimes causes some icons to overlap each other,
        // thus hiding some of the connectoids.  Toggling the
        // auto-arrange fixes this problem.  This will have to
        // be removed once moving icons within a folder works.
        ShellFolderView_AutoArrange(hwndMain);

        // Do it again to maintain user's preference
        ShellFolderView_AutoArrange(hwndMain);
        break;

    case DVM_MERGEMENU:
        RemView_MergeMenu(this, (LPQCMINFO)lParam);
        break;

    case DVM_INVOKECOMMAND:
        RemView_Command(this, hwndMain, (UINT)wParam);
        break;

    case DVM_GETHELPTEXT:
    case DVM_GETTOOLTIPTEXT:
        {
        UINT id = LOWORD(wParam);
        UINT cchBuf = HIWORD(wParam);
        LPSTR pszBuf = (LPSTR)lParam;
        UINT ids;

        if (DVM_GETHELPTEXT == uMsg)
            ids = id + IDS_MH_FIRST;
        else
            ids = id + IDS_TT_FIRST;

        if (0 == LoadString(ghInstance, ids, pszBuf, cchBuf))
            {
            hres = ResultFromScode(E_FAIL);
            }
        }
        break;

    case DVM_INITMENUPOPUP:
        RemView_InitMenuPopup(this, hwndMain, LOWORD(wParam), HIWORD(wParam), (HMENU)lParam);
        break;

    case DVM_GETDETAILSOF:
        hres = RemView_OnGetDetailsOf(this, hwndMain, (UINT)wParam, (PDETAILSINFO)lParam);
        break;

    case DVM_COLUMNCLICK:
        hres = RemView_OnColumnClick(this, hwndMain, (UINT)wParam);
        break;

    case DVM_FSNOTIFY:
        if (lParam == SHCNE_CREATE)
        {
          // Reenumerate the address book
          //
          Sos_Init();
        };
        break;

    case DVM_GETBUTTONINFO: {
        LPTBINFO ptbinfo = (LPTBINFO)lParam;
        ptbinfo->cbuttons = ARRAYSIZE(c_tbRemote);
        ptbinfo->uFlags = TBIF_PREPEND;
        }
        break;

    case DVM_GETBUTTONS:
        RemView_OnGetButtons(this, hwndMain, LOWORD(wParam), (LPTBBUTTON)lParam);
        break;

    case DVM_SELCHANGE:
        // lParam contains DVM_SELCHANGE or SFVM_SELCHANGE data. If we ever need this,
        // have MessageSFVCB below map the SFVM_SELCHANGE data into DVM_SELCHANGE
        // data and call this ViewCallback function. We can pass the DFV data into
        // OnSelChange and use it there.
        hres = RemView_OnSelChange(this, hwndMain, LOWORD(wParam));
        break;

    case DVM_DIDDRAGDROP:
        CDataObj_DidDragDrop((IDataObject *)lParam, wParam);
        break;

    case DVM_RELEASE:
        // During our _CreateInstance we called RemViewCB_AddRef() (well,
        // we set m_cref to 1) ... do the corresponding release here.
        //
        RemViewCB_Release(&this->sfvcb);
        break;

    default:
        hres = ResultFromScode(E_FAIL);
        }

    return hres;
    }

HRESULT CALLBACK RemViewCB_MessageSFVCB(
    IShellFolderViewCB* psfvcb,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
    {
    PREMVIEW this = IToClass(RemView, sfvcb, psfvcb);
    HRESULT hres = NOERROR;

    switch (uMsg)
        {
    case SFVM_HWNDMAIN:
        this->m_hwndMain = (HWND)lParam;
        break;

    case SFVM_SETISFV:
        if (this->m_psfv)
            this->m_psfv->lpVtbl->Release(this->m_psfv);
        this->m_psfv = (IShellFolderView*)lParam;
        if (this->m_psfv)
            this->m_psfv->lpVtbl->AddRef(this->m_psfv);
        break;

    case SFVM_GETNOTIFY:
        *(LPCITEMIDLIST*)wParam = this->m_pidl;
        *(LONG*)lParam = SHCNE_RENAMEITEM|SHCNE_CREATE|SHCNE_DELETE|SHCNE_UPDATEDIR|SHCNE_UPDATEITEM;
        break;

    case SFVM_GETVIEWS:
        hres = RemView_OnGetViews((SHELLVIEWID FAR *)wParam, (IEnumSFVViews FAR * FAR *)lParam);
        break;

    default:
        hres = RemView_ViewCallback(&this->sv, this->m_pshf, this->m_hwndMain, uMsg, wParam, lParam);
        }

    return hres;
    }


//---------------------------------------------------------------------------
// IShellFolderViewCB IUnknown functions
//---------------------------------------------------------------------------

/*----------------------------------------------------------
Purpose: IShellFolderViewCB::QueryInterface

Returns: standard
Cond:    --
*/
STDMETHODIMP RemViewCB_QueryInterface(
    IShellFolderViewCB* psfvcb,
    REFIID riid,
    LPVOID FAR* ppvOut)
    {
    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IShellFolderViewCB, riid))
        {
        *ppvOut = psfvcb;
        RemViewCB_AddRef(psfvcb);
        return(S_OK);
        }

    *ppvOut = NULL;
    return(E_NOINTERFACE);
    }


/*----------------------------------------------------------
Purpose: IShellFolderViewCB::AddRef

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) RemViewCB_AddRef(
    IShellFolderViewCB* psfvcb)
    {
    PREMVIEW this = IToClass(RemView, sfvcb, psfvcb);

    return(++this->m_cref);
    }


/*----------------------------------------------------------
Purpose: IShellFolderViewCB::Release

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) RemViewCB_Release(
    IShellFolderViewCB* psfvcb)
    {
    PREMVIEW this = IToClass(RemView, sfvcb, psfvcb);
    UINT cRef;

    cRef = --this->m_cref;

    if (0 == cRef)
    {
        if (this->m_pidl)
            ILFree(this->m_pidl);
    
        if (this->m_pshf)
            this->m_pshf->lpVtbl->Release(this->m_pshf);
    
        Sos_Release();
        LocalFree(this);
    
        ENTEREXCLUSIVE()
            {
            --g_cRef;
            }
        LEAVEEXCLUSIVE()
    }

    return cRef;
    }


//---------------------------------------------------------------------------
// Remote class : Vtables
//---------------------------------------------------------------------------

#pragma data_seg(DATASEG_READONLY)

IShellFolderViewCBVtbl c_RemViewCBVtbl =
    {
    RemViewCB_QueryInterface,
    RemViewCB_AddRef,
    RemViewCB_Release,

    RemViewCB_MessageSFVCB,
    };

#pragma data_seg()


/*----------------------------------------------------------
//
/*----------------------------------------------------------

//---------------------------------------------------------------------------
// Remote IShellView member functions
//
// In Win95, psvOuter is the only pointer to our object
// that DefView holds on us (without a refcount). So we
// need a full IShellView implementation that delegates
// to DefView. Pretty lame, eh? Looks like a poor attempt
// at implementing aggregation... Our implementation
// hands every request off to DefView. We don't even
// pay attention to reference counts, since DefView
// tells us to release by sending a DVM_RELEASE message.
//
// In IE4/Memphis/NT5, the shell holds an IShellFolderViewCB
// interface (with a refcount) which we implement.
//
//---------------------------------------------------------------------------

/*----------------------------------------------------------
Purpose: IShellView::QueryInterface

Returns: standard
Cond:    --
*/
STDMETHODIMP RemView_QueryInterface(
    LPSHELLVIEW psv,
    REFIID riid,
    LPVOID FAR* ppvOut)
    {
    PREMVIEW this = IToClass(RemView, sv, psv);

    return this->psv->lpVtbl->QueryInterface(this->psv, riid, ppvOut);
    }


/*----------------------------------------------------------
Purpose: IShellView::AddRef

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) RemView_AddRef(
    LPSHELLVIEW psv)
    {
    PREMVIEW this = IToClass(RemView, sv, psv);

    return this->psv->lpVtbl->AddRef(this->psv);
    }


/*----------------------------------------------------------
Purpose: IShellView::Release

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) RemView_Release(
    LPSHELLVIEW psv)
    {
    PREMVIEW this = IToClass(RemView, sv, psv);

    return this->psv->lpVtbl->Release(this->psv);
    }


/*----------------------------------------------------------
Purpose: IShellView::GetWindow

Returns: standard
Cond:    --
*/
STDMETHODIMP RemView_GetWindow(
    LPSHELLVIEW psv,
    HWND FAR * phwnd)
    {
    PREMVIEW this = IToClass(RemView, sv, psv);

    return this->psv->lpVtbl->GetWindow(this->psv, phwnd);
    }


/*----------------------------------------------------------
Purpose: IShellView::ContextSensitiveHelp

Returns: standard
Cond:    --
*/
STDMETHODIMP RemView_ContextSensitiveHelp(
    LPSHELLVIEW psv,
    BOOL bEnterMode)
    {
    PREMVIEW this = IToClass(RemView, sv, psv);

    return this->psv->lpVtbl->ContextSensitiveHelp(this->psv, bEnterMode);
    }


/*----------------------------------------------------------
Purpose: IShellView::TranslateAccelerator

Returns: standard
Cond:    --
*/
STDMETHODIMP RemView_TranslateAccelerator(
    LPSHELLVIEW psv,
    LPMSG pmsg)
    {
    PREMVIEW this = IToClass(RemView, sv, psv);

    return this->psv->lpVtbl->TranslateAccelerator(this->psv, pmsg);
    }


/*----------------------------------------------------------
Purpose: IShellView::EnableModeless

Returns: standard
Cond:    --
*/
STDMETHODIMP RemView_EnableModeless(
    LPSHELLVIEW psv,
    BOOL bEnable)
    {
    PREMVIEW this = IToClass(RemView, sv, psv);

    return this->psv->lpVtbl->EnableModeless(this->psv, bEnable);
    }


/*----------------------------------------------------------
Purpose: IShellView::UIActivate

Returns: standard
Cond:    --
*/
STDMETHODIMP RemView_UIActivate(
    LPSHELLVIEW psv,
    UINT uState)
    {
    PREMVIEW this = IToClass(RemView, sv, psv);

    return this->psv->lpVtbl->UIActivate(this->psv, uState);
    }


/*----------------------------------------------------------
Purpose: IShellView::Refresh

Returns: standard
Cond:    --
*/
STDMETHODIMP RemView_Refresh(
    LPSHELLVIEW psv)
    {
    PREMVIEW this = IToClass(RemView, sv, psv);

    return this->psv->lpVtbl->Refresh(this->psv);
    }


/*----------------------------------------------------------
Purpose: IShellView::CreateViewWindow

Returns: standard
Cond:    --
*/
STDMETHODIMP RemView_CreateViewWindow(
    LPSHELLVIEW psv,
    LPSHELLVIEW psvPrevView,
    LPCFOLDERSETTINGS pfs,
    LPSHELLBROWSER psb,
    LPRECT prcView,
    HWND FAR * phwnd)
    {
    PREMVIEW this = IToClass(RemView, sv, psv);

    return this->psv->lpVtbl->CreateViewWindow(this->psv, psvPrevView, pfs,
        psb, prcView, phwnd);
    }


/*----------------------------------------------------------
Purpose: IShellView::DestroyViewWindow

Returns: standard
Cond:    --
*/
STDMETHODIMP RemView_DestroyViewWindow(
    LPSHELLVIEW psv)
    {
    PREMVIEW this = IToClass(RemView, sv, psv);

    return this->psv->lpVtbl->DestroyViewWindow(this->psv);
    }


/*----------------------------------------------------------
Purpose: IShellView::GetCurrentInfo

Returns: standard
Cond:    --
*/
STDMETHODIMP RemView_GetCurrentInfo(
    LPSHELLVIEW psv,
    LPFOLDERSETTINGS pfs)
    {
    PREMVIEW this = IToClass(RemView, sv, psv);

    return this->psv->lpVtbl->GetCurrentInfo(this->psv, pfs);
    }



/*----------------------------------------------------------
Purpose: IShellView::AddPropertySheetPages

Returns: standard
Cond:    --
*/
STDMETHODIMP RemView_AddPropertySheetPages(
    LPSHELLVIEW psv,
    DWORD dwReserved,
    LPFNADDPROPSHEETPAGE pfn,
    LPARAM lParam)
    {
    PREMVIEW this = IToClass(RemView, sv, psv);

    return this->psv->lpVtbl->AddPropertySheetPages(this->psv, dwReserved,
        pfn, lParam);
    }


/*----------------------------------------------------------
Purpose: IShellView::SaveViewState

Returns: standard hresult
Cond:    --
*/
STDMETHODIMP_(UINT) RemView_SaveViewState(
    LPSHELLVIEW psv)
    {
    PREMVIEW this = IToClass(RemView, sv, psv);

    return this->psv->lpVtbl->SaveViewState(this->psv);
    }


/*----------------------------------------------------------
Purpose: IShellView::Release

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) RemView_SelectItem(
    LPSHELLVIEW psv,
    LPCVOID pvID,
    UINT uFlags)
    {
    PREMVIEW this = IToClass(RemView, sv, psv);

    return this->psv->lpVtbl->SelectItem(this->psv, pvID, uFlags);
    }


//---------------------------------------------------------------------------
// Remote class : Vtables
//---------------------------------------------------------------------------

#pragma data_seg(DATASEG_READONLY)

IShellViewVtbl c_RemViewSVVtbl =
    {
    RemView_QueryInterface,
    RemView_AddRef,
    RemView_Release,

    RemView_GetWindow,
    RemView_ContextSensitiveHelp,
    RemView_TranslateAccelerator,
    RemView_EnableModeless,
    RemView_UIActivate,
    RemView_Refresh,

    RemView_CreateViewWindow,
    RemView_DestroyViewWindow,
    RemView_GetCurrentInfo,
    RemView_AddPropertySheetPages,
    RemView_SaveViewState,
    RemView_SelectItem,
    };

#pragma data_seg()


/*----------------------------------------------------------
Purpose: This function creates an instance of IShellView.
         The RemView class does very little.  It keeps
         enough information to keep a correct reference count
         of the subobject space.  The rest of the work is
         done by the default shellview created by the
         shell (below).

Returns: standard
Cond:    --
*/
HRESULT PRIVATE RemView_CreateInstance(
    LPSHELLFOLDER psf,
    LPCITEMIDLIST pidl,
    LPVOID FAR* ppvOut)
    {
    HRESULT hres;
    PREMVIEW this;
    HMODULE hShell;
    HRESULT (*pfnSHCreateShellFolderView)(SFV_CREATE*,LPSHELLVIEW*) = NULL;

    DBG_ENTER("RemView_CreateInstance");

    // Create the callback object with a refcount of 1
    this = (PREMVIEW)LocalAlloc(LPTR, sizeof(*this));
    if (!this)
        {
        hres = ResultFromScode(E_OUTOFMEMORY);
        goto Leave;
        }
    this->sv.lpVtbl = &c_RemViewSVVtbl;
    this->sfvcb.lpVtbl = &c_RemViewCBVtbl;
    this->m_cref = 1;
    this->m_pidl = ILClone(pidl);   // keep track of this for SFVM_ messages
    this->m_pshf = psf;             // keep track of this for SFVM_ messages
    psf->lpVtbl->AddRef(psf);

    // Initialize the subobject space
    Sos_AddRef();
    // Increment the global refcount
    ENTEREXCLUSIVE()
        {
        ++g_cRef;
        }
    LEAVEEXCLUSIVE()

    hShell = GetModuleHandle("SHELL32");
    ASSERT(hShell);
    if (hShell)
        pfnSHCreateShellFolderView = (HRESULT (*)(SFV_CREATE*,LPSHELLVIEW*))GetProcAddress(hShell, MAKEINTRESOURCE(256));
    if (pfnSHCreateShellFolderView)
    {
        SFV_CREATE sfvc;

        // Use the new defview entry point so we get SFVM messages
        // NOTE that we don't need a psvOuter since our sfvcb will
        // keep the reference counts.
        //
        sfvc.cbSize = sizeof(sfvc);
        sfvc.pshf = psf;
        sfvc.psvOuter = NULL;
        sfvc.psfvcb = &this->sfvcb;
    
        hres = pfnSHCreateShellFolderView(&sfvc, (LPSHELLVIEW*)ppvOut);
    }
    else // pre-ie4 shell32
    {
        CSFV csfv;

        // Piggy-back off the shell's default view object.
        //
        csfv.cbSize = sizeof(csfv);
        csfv.pshf = psf;
        csfv.psvOuter = &this->sv;
        csfv.pidl = pidl;
        csfv.lEvents = SHCNE_RENAMEITEM|SHCNE_CREATE|SHCNE_DELETE|SHCNE_UPDATEDIR|SHCNE_UPDATEITEM;
        csfv.pfnCallback = RemView_ViewCallback;
        csfv.fvm = 0;       // Have defview restore the folder view mode

        // Note: we don't have to set ppvOut to point to RemView's IShellView
        // implementation because our IShellView delegates everything to DefView.
        //
        hres = SHCreateShellFolderViewEx(&csfv, (LPSHELLVIEW*)ppvOut);
    }

    if (FAILED(hres))
    {
        // This function frees everything for us
        //
        RemViewCB_Release(&this->sfvcb);
    }

Leave:
    DBG_EXIT_HRES("RemView_CreateInstance", hres);

    return hres;        // S_OK or E_NOINTERFACE
    }



//---------------------------------------------------------------------------
// RemEnum Class
//---------------------------------------------------------------------------


typedef struct _RemEnum
    {
    IEnumIDList eidl;
    UINT cRef;
    UINT uFlags;
    PSUBOBJ psoCur;     // Current subobject
    LPMALLOC pmalloc;

    } RemEnum, * PREMENUM;


/*----------------------------------------------------------
Purpose: Globally clones a pidl using task allocation 

Returns: standard result
Cond:    --
*/
HRESULT PRIVATE OLECloneItemIDList(
    LPMALLOC pmalloc,
    LPITEMIDLIST * ppidl,
    LPCITEMIDLIST pidl)
    {
    HRESULT hres = E_INVALIDARG;

    if (pidl)
        {
        LPITEMIDLIST pidlNew;
        UINT cb = ILGetSize(pidl);

        pidlNew = OLEAlloc(pmalloc, cb);
        if (pidlNew)
            {
            BltByte(pidlNew, pidl, cb);
            hres = NOERROR;
            }
        else
            {
            hres = E_OUTOFMEMORY;
            }
        *ppidl = pidlNew;
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: IEnumIDList::QueryInterface

Returns: standard hresult
Cond:    --
*/
HRESULT STDMETHODCALLTYPE RemEnum_QueryInterface(
    LPENUMIDLIST peidl,
    REFIID riid,
    LPVOID FAR * ppvObj)
    {
    PREMENUM this = IToClass(RemEnum, eidl, peidl);
    HRESULT hres = ResultFromScode(E_NOINTERFACE);

    DBG_ENTER_RIID("RemEnum_QueryInterface", riid);

    *ppvObj = NULL;

    if (IsEqualIID(riid, &IID_IEnumIDList)
        || IsEqualIID(riid, &IID_IUnknown))
        {
        *ppvObj = &this->eidl;
        this->cRef++;
        hres = NOERROR;
        }

    DBG_EXIT_HRES("RemEnum_QueryInterface", hres);

    return hres;
    }


/*----------------------------------------------------------
Purpose: IEnumIDList::AddRef

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) RemEnum_AddRef(
    LPENUMIDLIST peidl)
    {
    PREMENUM this = IToClass(RemEnum, eidl, peidl);

    return ++this->cRef;
    }


/*----------------------------------------------------------
Purpose: IEnumIDList::Release

Returns: reference count
Cond:    --
*/
STDMETHODIMP_(UINT) RemEnum_Release(
    LPENUMIDLIST peidl)
    {
    PREMENUM this = IToClass(RemEnum, eidl, peidl);
    UINT uRet;

    DBG_ENTER("RemEnum_Release");

    uRet = --this->cRef;

    if (0 == this->cRef)
        {
        TRACE_MSG(TF_GENERAL, "Finished enumerating");

        OLERelease(this->pmalloc);
        Sos_Release();
        LocalFree((HLOCAL)this);

        ENTEREXCLUSIVE()
            {
            --g_cRef;
            }
        LEAVEEXCLUSIVE()
        }

    DBG_EXIT_US("RemEnum_Release", uRet);

    return uRet;
    }


/*----------------------------------------------------------
Purpose: IEnumIDList::Next

Returns: standard hresult
Cond:    --
*/
STDMETHODIMP RemEnum_Next(
    LPENUMIDLIST peidl,
    ULONG celt,
    LPITEMIDLIST * rgelt,
    ULONG * pceltFetched)
    {
    PREMENUM this = IToClass(RemEnum, eidl, peidl);
    PSUBOBJ psoCur;
    HRESULT hres = ResultFromScode(S_OK);

    DBG_ENTER("RemEnum_Next");

    if (pceltFetched)
        {
        *pceltFetched = 0;
        }

    if (IsFlagClear(this->uFlags, SHCONTF_NONFOLDERS))
        {
        hres = ResultFromScode(S_FALSE);
        goto Leave;
        }

    psoCur = this->psoCur;
    if (psoCur == NULL)
        {
        hres = ResultFromScode(S_FALSE);    // No more subobjects
        goto Leave;
        }

#ifdef DEBUG

    TRACE_MSG(TF_GENERAL, "Getting subobj: %s", Dbg_SafeStr(Subobj_GetName(psoCur)));

#endif

    // Return a copy of pso.  The returned pidl is freed by caller.
    ASSERT(0 == *(USHORT*)((LPSTR)psoCur + psoCur->cbSize))

    hres = OLECloneItemIDList(this->pmalloc, rgelt, (LPCITEMIDLIST)psoCur);
    if (SUCCEEDED(hres))
        {
        // Set current pointer to next one
        this->psoCur = psoCur->psoNext;
        if (pceltFetched)
            {
            *pceltFetched = 1;  // (We only return 1 object at a time)
            }
        }

Leave:

    DBG_EXIT_HRES("RemEnum_Next", hres);

    return hres;
    }


/*----------------------------------------------------------
Purpose: IEnumIDList::Skip

Returns: standard hresult
Cond:    --
*/
STDMETHODIMP RemEnum_Skip(
    LPENUMIDLIST peidl,
    ULONG celt)
    {
    PREMENUM this = IToClass(RemEnum, eidl, peidl);
    PSUBOBJ pso;

    DBG_ENTER("RemEnum_Skip");

    pso = this->psoCur;
    while (pso && celt-- > 0)
        {
        pso = Sos_NextItem(pso);
        }
    this->psoCur = pso;

    DBG_EXIT_HRES("RemEnum_Skip", ResultFromScode(S_OK));

    return ResultFromScode(S_OK);
    }


/*----------------------------------------------------------
Purpose: IEnumIDList::Reset

Returns: standard hresult
Cond:    --
*/
STDMETHODIMP RemEnum_Reset(
    LPENUMIDLIST peidl)
    {
    PREMENUM this = IToClass(RemEnum, eidl, peidl);

    DBG_ENTER("RemEnum_Reset");

    ENTEREXCLUSIVE()
        {
        this->psoCur = Sos_FirstItem();
        }
    LEAVEEXCLUSIVE()

    DBG_EXIT_HRES("RemEnum_Reset", NOERROR);

    return NOERROR;
    }


/*----------------------------------------------------------
Purpose: IEnumIDList::Clone

         We do not implement this function.

Returns: E_NOTIMPL
Cond:    --
*/
STDMETHODIMP RemEnum_Clone(
    LPENUMIDLIST peidl,
    LPENUMIDLIST FAR *lplpenum)
    {
    return ResultFromScode(E_NOTIMPL);
    }


//---------------------------------------------------------------------------
// RemEnum class : Vtables
//---------------------------------------------------------------------------

#pragma data_seg(DATASEG_READONLY)

IEnumIDListVtbl c_RemEnumVtbl =
    {
    RemEnum_QueryInterface,
    RemEnum_AddRef,
    RemEnum_Release,
    RemEnum_Next,
    RemEnum_Skip,
    RemEnum_Reset,
    RemEnum_Clone,
    } ;

#pragma data_seg()


/*----------------------------------------------------------
Purpose: Creates an instance of IEnumIDList for the remote
         folder.

Returns: standard
Cond:    --
*/
HRESULT PRIVATE RemEnum_CreateInstance(
    REFIID riid,
    DWORD grfFlags,
    LPVOID FAR* ppvOut)
    {
    HRESULT hres;
    PREMENUM this;

    DBG_ENTER_RIID("RemEnum_CreateInstance", riid);

    this = LocalAlloc(LPTR, sizeof(*this));
    if (!this)
        {
        hres = ResultFromScode(E_OUTOFMEMORY);
        goto Leave;
        }
    hres = Sos_AddRef();
    if (FAILED(hres))
        {
        LocalFree(this);
        goto Leave;
        }
    hres = SHGetMalloc(&this->pmalloc);
    if (FAILED(hres))
        {
        Sos_Release();
        LocalFree(this);
        goto Leave;
        }
    this->eidl.lpVtbl = &c_RemEnumVtbl;
    this->cRef = 1;
    this->uFlags = grfFlags;

    ENTEREXCLUSIVE()
        {
        this->psoCur = Sos_FirstItem();
        ++g_cRef;
        }
    LEAVEEXCLUSIVE()

    TRACE_MSG(TF_GENERAL, "Enumerating folder...");

    DEBUG_CODE( Sos_DumpList(); )

    // Note that the Release member will free the object, if
    // QueryInterface failed.
    hres = this->eidl.lpVtbl->QueryInterface(&this->eidl, riid, ppvOut);
    this->eidl.lpVtbl->Release(&this->eidl);

Leave:
    DBG_EXIT_HRES("RemEnum_CreateInstance", hres);

    return hres;        // S_OK or E_NOINTERFACE
    }




//---------------------------------------------------------------------------
// Remote class
//---------------------------------------------------------------------------


// Remote class structure.  This is used for instances of
// IPersistFolder, IShellFolder, and IShellDetails.
typedef struct _Remote
    {
    // We use the pf also as our IUnknown interface
    IPersistFolder      pf;             // 1st base class
    IShellFolder        sf;             // 3rd base class
    IShellDetails       sd;             // 4th base class
    UINT                cRef;           // reference count
    const CLSID FAR *   rclsid;         // reference to class ID
    LPITEMIDLIST        pidl;
    LPMALLOC            pmalloc;
    } Remote, * PREMOTE;



//---------------------------------------------------------------------------
// Remote IUnknown base member functions
//---------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: IUnknown::QueryInterface

Returns: standard
Cond:    --
*/
STDMETHODIMP Remote_QueryInterface(
    LPUNKNOWN punk,
    REFIID riid,
    LPVOID FAR* ppvOut)
    {
    PREMOTE this = IToClass(Remote, pf, punk);
    HRESULT hres = ResultFromScode(E_NOINTERFACE);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IPersistFolder))
        {
        // We use the pf field as our IUnknown as well
        *ppvOut = &this->pf;
        this->cRef++;
        hres = NOERROR;
        }
    else if (IsEqualIID(riid, &IID_IShellFolder))
        {
        (LPSHELLFOLDER)*ppvOut = &this->sf;
        this->cRef++;
        hres = NOERROR;
        }
    else if (IsEqualIID(riid, &IID_IShellDetails))
        {
        (IShellDetails FAR *)*ppvOut = &this->sd;
        this->cRef++;
        hres = NOERROR;
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: IUnknown::AddRef

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) Remote_AddRef(
    LPUNKNOWN punk)
    {
    PREMOTE this = IToClass(Remote, pf, punk);

    return ++this->cRef;
    }


/*----------------------------------------------------------
Purpose: IUnknown::Release

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) Remote_Release(
    LPUNKNOWN punk)
    {
    PREMOTE this = IToClass(Remote, pf, punk);

    if (--this->cRef)
        {
        return this->cRef;
        }

    if (this->pidl)
        {
        ILFree(this->pidl);
        }

    OLERelease(this->pmalloc);
    LocalFree((HLOCAL)this);

    ENTEREXCLUSIVE()
        {
        --g_cRef;
        }
    LEAVEEXCLUSIVE()

    return 0;
    }


//---------------------------------------------------------------------------
// Remote IPersistFolder member functions
//---------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: IPersistFolder::QueryInterface

Returns: standard
Cond:    --
*/
STDMETHODIMP Remote_PF_QueryInterface(
    LPPERSISTFOLDER ppf,
    REFIID riid,
    LPVOID FAR* ppvOut)
    {
    return Remote_QueryInterface((LPUNKNOWN)ppf, riid, ppvOut);
    }


/*----------------------------------------------------------
Purpose: IPersistFolder::AddRef

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) Remote_PF_AddRef(
    LPPERSISTFOLDER ppf)
    {
    return Remote_AddRef((LPUNKNOWN)ppf);
    }


/*----------------------------------------------------------
Purpose: IPersistFolder::Release

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) Remote_PF_Release(
    LPPERSISTFOLDER ppf)
    {
    return Remote_Release((LPUNKNOWN)ppf);
    }


/*----------------------------------------------------------
Purpose: IPersistFolder::GetClassID

Returns: standard hresult
Cond:    --
*/
STDMETHODIMP Remote_PF_GetClassID(
    LPPERSISTFOLDER ppf,
    LPCLSID lpClassID)
    {
    PREMOTE this = IToClass(Remote, pf, ppf);

    DBG_ENTER("Remote_PF_GetClassID");

    *lpClassID = *this->rclsid;

    DBG_EXIT("Remote_PF_GetClassID");

    return NOERROR;
    }


/*----------------------------------------------------------
Purpose: IPersistFolder::Initialize

Returns: standard hresult
Cond:    --
*/
STDMETHODIMP Remote_PF_Initialize(
    LPPERSISTFOLDER ppf,
    LPCITEMIDLIST pidl)     // ID of this container
    {
    PREMOTE this = IToClass(Remote, pf, ppf);
    HRESULT hres = NOERROR;

    DBG_ENTER("Remote_PF_Initialize");

    this->pidl = ILClone(pidl);     // Save this away
    if (NULL == this->pidl)
        hres = ResultFromScode(E_OUTOFMEMORY);

    DBG_EXIT_HRES("Remote_PF_Initialize", hres);

    return hres;
    }


//---------------------------------------------------------------------------
// Remote IShellFolder member functions
//---------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: IShellFolder::QueryInterface

Returns: standard
Cond:    --
*/
STDMETHODIMP Remote_SF_QueryInterface(
    LPSHELLFOLDER psf,
    REFIID riid,
    LPVOID FAR* ppvOut)
    {
    PREMOTE this = IToClass(Remote, sf, psf);

    return Remote_QueryInterface((LPUNKNOWN)&this->pf, riid, ppvOut);
    }


/*----------------------------------------------------------
Purpose: IShellFolder::AddRef

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) Remote_SF_AddRef(
    LPSHELLFOLDER psf)
    {
    PREMOTE this = IToClass(Remote, sf, psf);

    return Remote_AddRef((LPUNKNOWN)&this->pf);
    }


/*----------------------------------------------------------
Purpose: IShellFolder::Release

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) Remote_SF_Release(
    LPSHELLFOLDER psf)
    {
    PREMOTE this = IToClass(Remote, sf, psf);
    return Remote_Release((LPUNKNOWN)&this->pf);
    }


/*----------------------------------------------------------
Purpose: IShellFolder::ParseDisplayName

         This function is called when the caller wants a
         moniker created given a string representation of
         the object.

Returns: standard hresult
Cond:    --
*/
STDMETHODIMP Remote_SF_ParseDisplayName(
    LPSHELLFOLDER psf,
    HWND hwndOwner,
    LPBC pbc,
    LPOLESTR pwzDisplayName,
    ULONG FAR* pchEaten,
    LPITEMIDLIST * ppidlOut,
    ULONG *pdwAttributes)
    {
    PREMOTE this = IToClass(Remote, sf, psf);
    HRESULT hres = ResultFromScode(E_INVALIDARG);
    PSUBOBJ pso;
    char szName[MAXPATHLEN];

    DBG_ENTER("Remote_SF_ParseDisplayName");

    ASSERT(pwzDisplayName);
    ASSERT(ppidlOut);

    if (!pwzDisplayName)
        goto Leave;

    OleStrToStrN(szName, sizeof(szName), pwzDisplayName, (UINT)-1);
    pso = Sos_FindItem(szName);
    if (pso)
        {
        // Wrap this name up into a complete pidl and return the
        // pidl.  The pidl is freed by the caller.
        hres = OLECloneItemIDList(this->pmalloc, ppidlOut, (LPCITEMIDLIST)pso);
        }

Leave:
    DBG_EXIT_HRES("Remote_SF_ParseDisplayName", hres);

    return hres;
    }


/*----------------------------------------------------------
Purpose: IShellFolder::GetAttributesOf

         The shell calls this member function for the attributes
         of an object.

Returns: standard hresult
Cond:    --
*/
STDMETHODIMP Remote_SF_GetAttributesOf(
    LPSHELLFOLDER psf,
    UINT cidl,
    LPCITEMIDLIST FAR * ppidl,           // May be NULL
    ULONG FAR* prgfInOut)
    {
    ULONG ulIn;
    ULONG ulOut;

    DBG_ENTER_SZ("Remote_SF_GetAttributesOf",
                 ppidl && *ppidl ? Subobj_GetName((PSUBOBJ)*ppidl) : NULL);

    // We allow the user to link any of these objects.  We also
    // allow him to rename or delete any object except for the
    // "New Connection".
    ulIn = *prgfInOut;
    ulOut = 0;

    if (ppidl && *ppidl)
        {
        // Per object
        PSUBOBJ pso = (PSUBOBJ)*ppidl;
        ulOut |= SFGAO_CANLINK;

        if (IsFlagClear(Subobj_GetFlags(pso), SOF_NEWREMOTE))
            {
            ulOut |= SFGAO_CANRENAME | SFGAO_CANDELETE;

            if (1 == cidl)
                {
                ulOut |= SFGAO_HASPROPSHEET |
                         SFGAO_CANCOPY      | SFGAO_CANMOVE;
                }
            }
        }
    else
        {
        // For all objects
        ulOut |= SFGAO_CANLINK | SFGAO_CANRENAME;
        }

    *prgfInOut = ulOut;

    DBG_EXIT_HRES("Remote_SF_GetAttributesOf", NOERROR);

    return(NOERROR);
    }


/*----------------------------------------------------------
Purpose: IShellFolder::GetUIObjectOf

         The shell calls this function to obtain IContextMenu,
         IExtractIcon, or IDataObject interfaces to our objects.

Returns: standard hresult
Cond:    --
*/
STDMETHODIMP Remote_SF_GetUIObjectOf(
    LPSHELLFOLDER psf,
    HWND hwndOwner,
    UINT cidl,
    LPCITEMIDLIST FAR* apidl,
    REFIID riid,
    UINT FAR* prgfInOut,
    LPVOID FAR* ppvOut)
    {
    PREMOTE this = IToClass(Remote, sf, psf);
    HRESULT hres = ResultFromScode(E_INVALIDARG);
    PSUBOBJ pso;

    DBG_ENTER_RIID("Remote_SF_GetUIObjectOf", riid);

    pso = cidl > 0 ? (PSUBOBJ)*apidl : NULL;

    if (cidl == 1 && IsEqualIID(riid, &IID_IExtractIcon))
        {
        // Create an IExtractIcon interface
        hres = RemObj_CreateInstance(psf, cidl, apidl, riid, ppvOut);
        }
    else if (cidl > 0 && IsEqualIID(riid, &IID_IContextMenu))
        {
        // Create an IContextMenu interface
        hres = RemObj_CreateInstance(psf, cidl, apidl, riid, ppvOut);
        }
    else if (cidl > 0 && IsEqualIID(riid, &IID_IDataObject))
        {
        hres = CDataObj_CreateInstance(this->pidl, cidl, apidl, (LPDATAOBJECT FAR*)ppvOut);
        }

    DBG_EXIT_HRES("Remote_SF_GetUIObjectOf", hres);

    return(hres);
    }


/*----------------------------------------------------------
Purpose: IShellFolder::EnumObjects

         The shell calls this member function to get an IEnumIDList
         interface.  It does this in order to enumerate the
         contents of this container.

Returns: standard hresult
Cond:    --
*/
STDMETHODIMP Remote_SF_EnumObjects(
    LPSHELLFOLDER psf,
    HWND hwndOwner,
    DWORD grfFlags,
    LPENUMIDLIST FAR* ppeidl)
    {
    HRESULT hres = ResultFromScode(E_FAIL);

    DBG_ENTER("Remote_SF_EnumObjects");

    if (IsFlagSet(grfFlags, SHCONTF_NONFOLDERS))
        {
        hres = RemEnum_CreateInstance(&IID_IEnumIDList, grfFlags, ppeidl);
        }

    DBG_EXIT_HRES("Remote_SF_EnumObjects", hres);

    return hres;
    }


/*----------------------------------------------------------
Purpose: IShellFolder::BindToObject

         This function is called when the shell needs to
         bind to a subfolder inside our container.  We do
         not have subfolders, so we do not support this
         function.

Returns: E_NOTIMPL
Cond:    --
*/
STDMETHODIMP Remote_SF_BindToObject(
    LPSHELLFOLDER psf,
    LPCITEMIDLIST pidl,         // A single object
    LPBC pbc,
    REFIID riid,
    LPVOID FAR* ppvOut)
    {
    HRESULT hres = ResultFromScode(E_NOTIMPL);

    DBG_ENTER("Remote_SF_BindToObject");
    ASSERT(0);      // This function should not be called
    DBG_EXIT_HRES("Remote_SF_BindToObject", hres);

    return hres;
    }


/*----------------------------------------------------------
Purpose: IShellFolder::BindToStorage

         This will not be implemented for 1.0

Returns: standard hresult
Cond:    --
*/
STDMETHODIMP Remote_SF_BindToStorage(
    LPSHELLFOLDER psf,
    LPCITEMIDLIST pidl,     // Single object
    LPBC pbc,
    REFIID riid,
    LPVOID FAR* ppvObj)
    {
    return(ResultFromScode(E_NOTIMPL));
    }


/*----------------------------------------------------------
Purpose: IShellFolder::CompareIDs

         Determines how two objects compare.

Returns: standard hresult
Cond:    --
*/
STDMETHODIMP Remote_SF_CompareIDs(
    LPSHELLFOLDER psf,
    LPARAM lParam,
    LPCITEMIDLIST pidl1,
    LPCITEMIDLIST pidl2)
    {
    PSUBOBJ pso1 = (PSUBOBJ)pidl1;
    PSUBOBJ pso2 = (PSUBOBJ)pidl2;
    int n;

    ASSERT(pso1);
    ASSERT(pso2);

    // "New Connection" is always first
    if (IsFlagSet(pso1->uFlags, SOF_NEWREMOTE))
        n = -1;
    else if (IsFlagSet(pso2->uFlags, SOF_NEWREMOTE))
        n = 1;
    else
        {
        switch (lParam)
            {
        case ICOL_NAME:
            n = lstrcmp(Subobj_GetName(pso1), Subobj_GetName(pso2));
            break;

        case ICOL_PHONE:
        case ICOL_DEVICE:
            {
            // Get the entry information
            PCONNENTRY pconnentry1 = RnaGetConnEntry(Subobj_GetName(pso1), FALSE, TRUE);
            PCONNENTRY pconnentry2 = RnaGetConnEntry(Subobj_GetName(pso2), FALSE, TRUE);
            if (pconnentry1 && pconnentry2)
                {
                if (ICOL_PHONE == lParam)
                    {
                    char    szPhone1[RAS_MaxPhoneNumber+1];
                    char    szPhone2[RAS_MaxPhoneNumber+1];

                    ComposeDisplayPhone(szPhone1, &pconnentry1->pn,
                                        sizeof(szPhone1));
                    ComposeDisplayPhone(szPhone2, &pconnentry2->pn,
                                        sizeof(szPhone2));
                    n = lstrcmp(szPhone1, szPhone2);
                    }
                    else
                    {
                    n = lstrcmp(pconnentry1->pDevConfig->di.szDeviceName,
                                pconnentry2->pDevConfig->di.szDeviceName);
                    };
                }

            if (pconnentry1)
                RnaFreeConnEntry(pconnentry1);
            if (pconnentry2)
                RnaFreeConnEntry(pconnentry2);
            }
            break;

            }
        }

    return(ResultFromShort(n));
    }


/*----------------------------------------------------------
Purpose: IShellFolder::CreateViewObject

         The shell calls this member function to get the
         IShellView or IDropTarget interface to the container.
         If the default view is created, it calls to get the
         IContextMenu interface for the background pane context
         menu.

Returns: standard hresult
Cond:    --
*/
STDMETHODIMP Remote_SF_CreateViewObject(
    LPSHELLFOLDER psf,
    HWND hwnd,
    REFIID riid,
    LPVOID FAR* ppvOut)
    {
    PREMOTE this = IToClass(Remote, sf, psf);
    HRESULT hres = ResultFromScode(E_NOINTERFACE);

    DBG_ENTER_RIID("Remote_SF_CreateViewObject", riid);

    if (IsEqualIID(riid, &IID_IShellView))
        {
        // Interface for the view window
#ifdef DEBUG

        ProcessIniFile();
        DEBUG_BREAK(BF_ONOPEN);

#endif

        hres = RemView_CreateInstance(psf, this->pidl, ppvOut);
        if (SUCCEEDED(hres))
            {
            ENTEREXCLUSIVE()
                {
                if (g_pidlRemote==NULL)
                    {
                    // Copy the pidl the shell gave us.
                    g_pidlRemote = ILGlobalClone(this->pidl);
                    }
                }
            LEAVEEXCLUSIVE()

            RunWizard(hwnd, INTRO_WIZ);
            }
        }
    else if (IsEqualIID(riid, &IID_IDropTarget))
        {
        hres = CIDLDropTarget_Create(hwnd, this->pidl, (LPDROPTARGET *)ppvOut);
        };

    DBG_EXIT_HRES("Remote_SF_CreateViewObject", hres);

    return hres;
    }


/*----------------------------------------------------------
Purpose: IShellFolder::GetDisplayNameOf

         The shell calls this member function to get the display
         name of our object.

         If we store the display name in the CITEMIDLIST that
         is passed in (remember, it is opaque to the shell), then
         we simply return the offset to the name.

         Otherwise we return a pointer to the display name string.

Returns: standard hresult
Cond:    --
*/
STDMETHODIMP Remote_SF_GetDisplayNameOf(
    LPSHELLFOLDER psf,
    LPCITEMIDLIST pidl,         // A pidl
    DWORD dwFlags,
    LPSTRRET lpName)
    {
    PSUBOBJ pso = (PSUBOBJ)pidl;

    // Return the offset to the string because we store the display
    // name in the opaque structure.
    lpName->uType = STRRET_OFFSET;
    lpName->uOffset = _IOffset(SUBOBJ, szName);

    return(NOERROR);
    }


/*----------------------------------------------------------
Purpose: IShellFolder::SetNameOf

         IShellView calls this member function when it wants to
         change the display name of our object.  This can only
         happen if IShellFolder::GetAttributesOf returns the
         SFGAO_CANRENAME flag.

         For us, the IShellView is the shell's DefView.  After
         the user has typed in the new name, the shell calls
         this member function.

Returns: standard hresult
Cond:    --
*/
STDMETHODIMP Remote_SF_SetNameOf(
    LPSHELLFOLDER psf,
    HWND hwndOwner,
    LPCITEMIDLIST pidl,         // A single object
    LPCOLESTR lpszOleName,      // The new name
    DWORD dwFlags,
    LPITEMIDLIST FAR * ppidlOut)// May be null
    {
    PREMOTE this = IToClass(Remote, sf, psf);
    PSUBOBJ pso;
    PSUBOBJ psoNew;
    char szName[MAXPATHLEN];
    HRESULT hres;

    ASSERT(pidl);
    ASSERT(lpszOleName);

    DBG_ENTER_SZ("Remote_SF_SetNameOf", Subobj_GetName((PSUBOBJ)pidl));

    // We call an internal shell API to convert from OLE's string
    // format (UNICODE) to ANSI format that we so love and admire.
    OleStrToStrN(szName, sizeof(szName), lpszOleName, (UINT)-1);

    // Has the subobject already been renamed or deleted?
    pso = Sos_FindItem(Subobj_GetName((PSUBOBJ)pidl));
    if (NULL == pso)
        {
        // Yes; just say everything's okay.
        hres = NOERROR;
        }
    else if (!Remote_RenameObject(hwndOwner, pso, &psoNew, szName))
        {
        hres = ResultFromScode(E_FAIL);
        }
    else if (ppidlOut)
        {
        hres = OLECloneItemIDList(this->pmalloc, ppidlOut, (LPCITEMIDLIST)psoNew);
        Subobj_Destroy(psoNew);
        }

    DBG_EXIT_HRES("Remote_SF_SetNameOf", hres);

    return hres;
    }


//---------------------------------------------------------------------------
// Remote class : Vtables
//---------------------------------------------------------------------------

#pragma data_seg(DATASEG_READONLY)

IPersistFolderVtbl c_RemotePFVtbl =
    {
    Remote_PF_QueryInterface,
    Remote_PF_AddRef,
    Remote_PF_Release,
    Remote_PF_GetClassID,
    Remote_PF_Initialize,
    };

IShellFolderVtbl c_RemoteSFVtbl =
    {
    Remote_SF_QueryInterface,
    Remote_SF_AddRef,
    Remote_SF_Release,
    Remote_SF_ParseDisplayName,
    Remote_SF_EnumObjects,
    Remote_SF_BindToObject,
    Remote_SF_BindToStorage,
    Remote_SF_CompareIDs,
    Remote_SF_CreateViewObject,
    Remote_SF_GetAttributesOf,
    Remote_SF_GetUIObjectOf,
    Remote_SF_GetDisplayNameOf,
    Remote_SF_SetNameOf
    } ;

#pragma data_seg()


/*----------------------------------------------------------
Purpose: This function is called back from within
         IClassFactory::CreateInstance() of the default class
         factory object, which is created by SHCreateClassObject.

Returns: standard
Cond:    --
*/
HRESULT CALLBACK Remote_CreateInstance(
    LPUNKNOWN punkOuter,
    REFIID riid,
    LPVOID FAR* ppvOut)
    {
    HRESULT hres;
    PREMOTE this;

    DBG_ENTER_RIID("Remote_CreateInstance", riid);

    // The remote folder does not support aggregation.
    if (punkOuter)
        {
        hres = ResultFromScode(CLASS_E_NOAGGREGATION);
        goto Leave;
        }

    this = LocalAlloc(LPTR, sizeof(*this));
    if (!this)
        {
        hres = ResultFromScode(E_OUTOFMEMORY);
        goto Leave;
        }
    hres = SHGetMalloc(&this->pmalloc);
    if (FAILED(hres))
        {
        LocalFree(this);
        hres = ResultFromScode(E_OUTOFMEMORY);
        goto Leave;
        }
    this->pf.lpVtbl = &c_RemotePFVtbl;
    this->sf.lpVtbl = &c_RemoteSFVtbl;
    this->cRef = 1;
    this->rclsid = &CLSID_Remote;
    this->pidl = NULL;

    ENTEREXCLUSIVE()
        {
        ++g_cRef;
        }
    LEAVEEXCLUSIVE()

    // Note that the Release member will free the object, if
    // QueryInterface failed.
    hres = this->pf.lpVtbl->QueryInterface(&this->pf, riid, ppvOut);
    this->pf.lpVtbl->Release(&this->pf);

Leave:
    DBG_EXIT_HRES("Remote_CreateInstance", hres);

    return hres;        // S_OK or E_NOINTERFACE
    }


//---------------------------------------------------------------------------
// EXPORTED API
//---------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: Standard shell entry-point

Returns: standard
Cond:    --
*/
STDAPI DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID FAR* ppv)
    {
    if (IsEqualIID(rclsid, &CLSID_Remote))
        {
        // We are supposed return the class object for this class.  Instead
        // of fully implementing it in this DLL, we just call a helper
        // function in the shell DLL which creates a default class factory
        // object for us. When its CreateInstance member is called, it
        // will call back our create instance function.
        //
        return SHCreateDefClassObject(
                    riid,                   // Interface ID
                    ppv,                    // Non-null to aggregate
                    Remote_CreateInstance,  // Callback function
                    &g_cRef,                // Reference count of this DLL
                    &IID_IPersistFolder);   // Init interface
        }

    return ResultFromScode(REGDB_E_CLASSNOTREG);
    }

