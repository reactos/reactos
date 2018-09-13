//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// Chanmenu.cpp 
//
//   IConextMenu for folder items.
//
//   History:
//
//       6/12/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "stdinc.h"
#include "cdfidl.h"
#include "xmlutil.h"
#include "chanmenu.h"
#include "dll.h"
#include "persist.h"
#include "resource.h"
#include "chanapi.h"
#include "chanmgrp.h"
#include "chanmgri.h"
#define _SHDOCVW_
#include <shdocvw.h>

#include <mluisupp.h>

#ifdef UNIX
EXTERN_C char *MwGetXDisplayString( void );

#ifdef UNICODE
#define unixInvokeEditor unixInvokeEditorW
#else
#define unixInvokeEditor unixInvokeEditorA
#endif /* UNICODE */

static void unixInvokeEditorA(LPCSTR lpszPath)
{
    HKEY  hkeyUnix;
    CHAR  szCommand [2*MAX_PATH];
    CHAR  szCmdTempl[MAX_PATH+1];
    CHAR  szName    [MAX_PATH+1];
    CHAR  hKeyName  [MAX_PATH+1];

    STARTUPINFOA st;
    PROCESS_INFORMATION pi;

    BOOL  bIsKnownEdit  = FALSE;
    char  displayString [2*MAX_PATH];
    DWORD editors       = 0;
    DWORD type          = REG_SZ;
    DWORD dwLength      = sizeof(szCmdTempl);

    if( MwGetXDisplayString() )
        sprintf( displayString, "-display %s", MwGetXDisplayString() );
    else
        sprintf( displayString, " ");


    // Get user preferred editor.
    if( getenv("EDITOR" ) )
       strcpy(szName, getenv("EDITOR") );
    else
       strcpy(szName, "vi");

    // Check editor against the list of known editors in
    // registry.

    sprintf( hKeyName,
             "Software\\Microsoft\\Internet Explorer\\Unix\\Editors\\%s",
             szName );

    LONG lResult = RegOpenKeyExA(
       HKEY_CURRENT_USER,
       hKeyName,
       0,
       KEY_QUERY_VALUE,
       &hkeyUnix);

    // Create proper command and append dissplay string to make the
    // editor appear on the same XServer as the Iexplorer.
    if( !bIsKnownEdit )
    {
        // Default use vi
        sprintf( szCommand, "xterm %s -e vi %s ", displayString, lpszPath  );
    }
    else
    {
        // Use template command from registry to create actual command.
        sprintf( szCommand, szCmdTempl, displayString, lpszPath );
    }

    // Initialize startup info struct.
    st.cb = sizeof(STARTUPINFO);
    st.lpReserved = NULL;
    st.lpDesktop  = NULL;
    st.lpTitle    = NULL;
    st.dwFlags    = 0;
    st.wShowWindow= SW_SHOWNORMAL;
    st.cbReserved2= 0;
    st.lpReserved2= NULL;

    // Launch the command
    if ( CreateProcessA( NULL, szCommand, NULL, NULL, TRUE,
                         CREATE_NEW_CONSOLE, NULL, NULL, &st, &pi ))
    {
        return;
    }
    return;
}

static void unixInvokeEditorW(LPCWSTR lpwszPath)
{
   char szFileName[MAX_PATH+2]; /* same as in ViewSource */
   
   SHUnicodeToAnsi(lpwszPath,szFileName, ARRAYSIZE(szFileName));
   unixInvokeEditorA(szFileName);
 
   return;
}
#endif /* UNIX */



//
// Constructor and destructor.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CContextMenu::CContextMenu ***
//
//    Constructor for IContextMenu.
//
////////////////////////////////////////////////////////////////////////////////
CChannelMenu::CChannelMenu (
    void
)
: m_cRef(1)
{
    TraceMsg(TF_OBJECTS, "+ IContextMenu (root)");

    DllAddRef();

    ASSERT(NULL == m_pSubscriptionMgr);
    ASSERT(NULL == m_bstrURL);
    ASSERT(NULL == m_bstrName);

    return;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CContextMenu::~CContextMenu ***
//
//    Destructor.
//
////////////////////////////////////////////////////////////////////////////////
CChannelMenu::~CChannelMenu (
    void
)
{
    ASSERT(0 == m_cRef);

    if (NULL != m_bstrURL)
        SysFreeString(m_bstrURL);

    if (NULL != m_bstrName)
        SysFreeString(m_bstrName);
        
    if (NULL != m_pSubscriptionMgr)
        m_pSubscriptionMgr->Release();
    //
    // Matching Release for the constructor Addref.
    //

    TraceMsg(TF_OBJECTS, "- IContextMenu (root)");

    DllRelease();

    return;
}


//
// IUnknown methods.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CContextMenu::CContextMenu ***
//
//    CExtractIcon QI.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CChannelMenu::QueryInterface (
    REFIID riid,
    void **ppv
)
{
    ASSERT(ppv);

    HRESULT hr;

    *ppv = NULL;

    if (IID_IUnknown == riid || IID_IContextMenu == riid)
    {
        *ppv = (IContextMenu*)this;
    }
    else if (IID_IShellExtInit == riid)
    {
        *ppv = (IShellExtInit*)this;
    }

    if (*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        hr = S_OK;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    ASSERT((SUCCEEDED(hr) && *ppv) || (FAILED(hr) && NULL == *ppv));

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CContextMenu::AddRef ***
//
//    CContextMenu AddRef.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CChannelMenu::AddRef (
    void
)
{
    ASSERT(m_cRef != 0);
    ASSERT(m_cRef < (ULONG)-1);

    return ++m_cRef;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CContextMenu::Release ***
//
//    CContextMenu Release.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CChannelMenu::Release (
    void
)
{
    ASSERT (m_cRef != 0);

    ULONG cRef = --m_cRef;
    
    if (0 == cRef)
        delete this;

    return cRef;
}


//
// IContextMenu methods.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CContextMenu::QueryContextMenu ***
//
//
// Description:
//     Adds menu items to the given item's context menu.
//
// Parameters:
//     [In Out]  hmenu      - A handle to the menu.  New items are inserted into
//                            this menu  
//     [In]      indexMenu  - Zero-based position at which to insert the first
//                            menu item.
//     [In]      idCmdFirst - Minimum value that can be used for a new menu item
//                            identifier. 
//     [In]      idCmdLast  - Maximum value the can be used for a menu item id.
//     [In]      uFlags     - CMF_DEFAULTONLY, CMF_EXPLORE, CMF_NORMAL or
//                            CMF_VERBSONLY.
//
// Return:
//     On success the scode contains the the menu identifier offset of the last
//     menu item added plus one.
//
// Comments:
//     CMF_DEFAULTONLY flag indicates the user double-clicked on the item.  In
//     this case no menu is displayed.  The shell is simply querying for the ID
//     of the default action.
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CChannelMenu::QueryContextMenu(
    HMENU hmenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags
)
{
    HRESULT hr = S_OK;
    BOOL fSubscribed;
    HMENU hChannelMenu, hChannelSubMenu;
    
    ASSERT(hmenu);
    ASSERT(idCmdFirst < idCmdLast);

    if (!(CMF_DEFAULTONLY & uFlags))
    {   
        if (NULL != m_pSubscriptionMgr)
        {
            ASSERT(idCmdFirst + IDM_SUBSCRIBE < idCmdLast);

#ifndef UNIX
            if (CanSubscribe(m_bstrURL))
            {
                m_pSubscriptionMgr->IsSubscribed(m_bstrURL, &fSubscribed);
                if (fSubscribed)
                {
                    hChannelMenu = LoadMenu(MLGetHinst(), 
                                           MAKEINTRESOURCE(IDM_SUBSCRIBEDMENU));

                    if (SHRestricted2W(REST_NoRemovingSubscriptions, m_bstrURL, 0))
                    {
                        EnableMenuItem(hChannelMenu, IDM_SUBSCRIBE,
                                       MF_BYCOMMAND | MF_GRAYED);
                    }
                    
                    if (SHRestricted2W(REST_NoManualUpdates, m_bstrURL, 0))
                    {
                        EnableMenuItem(hChannelMenu, IDM_UPDATESUBSCRIPTION,
                                       MF_BYCOMMAND | MF_GRAYED);
                    }
                }
                else
                {
                    int idMenu = !SHRestricted2W(REST_NoAddingSubscriptions, 
                                                 m_bstrURL, 0) 
                                 ? IDM_UNSUBSCRIBEDMENU : IDM_NOSUBSCRIBEMENU;
                                  
                    hChannelMenu = LoadMenu(MLGetHinst(), MAKEINTRESOURCE(idMenu));
                }
            }
            else
#endif /* !UNIX */
            {
                hChannelMenu = LoadMenu(MLGetHinst(),
                                          MAKEINTRESOURCE(IDM_NOSUBSCRIBEMENU));
            }

            if (NULL != hChannelMenu)
            {
                hChannelSubMenu = GetSubMenu(hChannelMenu, 0);

                if (NULL != hChannelSubMenu)
                {
                    hr = Shell_MergeMenus(hmenu, hChannelSubMenu, indexMenu,
                                          idCmdFirst, idCmdLast, MM_ADDSEPARATOR)
                                          - idCmdFirst;
                }
                else
                {
                    hr = E_FAIL;
                }
                DestroyMenu(hChannelMenu);
            }
            else
            {
                hr = E_FAIL;
            }
        }

        RemoveMenuItems(hmenu);
    }

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CContextMenu::InvokeCommand ***
//
//
// Description:
//     Carries out the command for the given menu item id.
//
// Parameters:
//     [In]  lpici - Structure containing the verb, hwnd, menu id, etc.
//
// Return:
//     S_OK if the command was successful.
//     E_FAIL otherwise.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CChannelMenu::InvokeCommand(
    LPCMINVOKECOMMANDINFO lpici
)
{
    HRESULT hr = S_OK;
    
    ASSERT(lpici);

    if (HIWORD(lpici->lpVerb) == 0)
    {
        switch (LOWORD(lpici->lpVerb))
        {
            case IDM_UPDATESUBSCRIPTION:
                ASSERT(NULL != m_pSubscriptionMgr);
                m_pSubscriptionMgr->UpdateSubscription(m_bstrURL);
                break;
                           
            case IDM_SUBSCRIBE:
                ASSERT(NULL != m_pSubscriptionMgr);
                ASSERT( sizeof(SUBSCRIPTIONINFO) == m_si.cbSize);

                hr = Subscribe(lpici->hwnd);
                break;
                
            case IDM_UNSUBSCRIBE:
                ASSERT(NULL != m_pSubscriptionMgr);
                m_pSubscriptionMgr->DeleteSubscription(m_bstrURL, lpici->hwnd);
                break;

            case IDM_EDITSUBSCRIPTION:
                ASSERT(NULL != m_pSubscriptionMgr);
                m_pSubscriptionMgr->ShowSubscriptionProperties(m_bstrURL,
                                                               lpici->hwnd);
                break;

            case IDM_REFRESHCHANNEL:
                Refresh(lpici->hwnd);
                break;

            case IDM_VIEWSOURCE:
                ViewSource(lpici->hwnd);
                break;
        }
    }
    

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CContextMenu::GetCommandString ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CChannelMenu::GetCommandString(
    UINT_PTR idCommand,
    UINT uFLags,
    UINT *pwReserved,
    LPSTR pszName,
    UINT cchMax
)
{
    return E_NOTIMPL;
}

//
//
//

STDMETHODIMP
CChannelMenu::Initialize(
    LPCITEMIDLIST pidl,
    LPDATAOBJECT pdobj,
    HKEY hkey
)
{
    HRESULT hr;
    
    STGMEDIUM stgmed;
    FORMATETC fmtetc = {CF_HDROP, NULL, DVASPECT_CONTENT, -1,
                        TYMED_HGLOBAL};

    ASSERT(pdobj);
    
    hr = pdobj->GetData(&fmtetc, &stgmed);

    if (SUCCEEDED(hr))
    {
        if (DragQueryFile((HDROP)stgmed.hGlobal, 0, m_szPath, 
                          ARRAYSIZE(m_szPath)))
        {
            m_tt.cbTriggerSize = sizeof(TASK_TRIGGER);
            m_si.cbSize        = sizeof(SUBSCRIPTIONINFO);
            m_si.fUpdateFlags |= SUBSINFO_SCHEDULE;
            m_si.schedule      = SUBSSCHED_AUTO;
            m_si.pTrigger = &m_tt;

            hr = GetNameAndURLAndSubscriptionInfo(m_szPath, &m_bstrName, &m_bstrURL,
                                                  &m_si);

            ASSERT((SUCCEEDED(hr) && m_bstrName && m_bstrURL) || FAILED(hr));
        }
        else
        {
            hr = E_FAIL;
        }

        ReleaseStgMedium(&stgmed);

        if (SUCCEEDED(hr))
        {
            hr = CoCreateInstance(CLSID_SubscriptionMgr, NULL,
                                  CLSCTX_INPROC_SERVER, IID_ISubscriptionMgr,
                                  (void**)&m_pSubscriptionMgr);
        }
    }
    //  Return S_OK even if things didn't go as planned so that
    //  RemoveMenus will get called.

    return S_OK;
}


//
// Helper functions
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** Name ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
void
CChannelMenu::RemoveMenuItems(
    HMENU hmenu
)
{
    TCHAR aszRemove[4][62] = {{0}, {0}, {0}, {0}};

    MLLoadString(IDS_SHARING, aszRemove[0], ARRAYSIZE(aszRemove[0]));
    MLLoadString(IDS_RENAME,  aszRemove[1], ARRAYSIZE(aszRemove[1]));
    MLLoadString(IDS_SENDTO,  aszRemove[2], ARRAYSIZE(aszRemove[2]));

    if (SHRestricted2W(REST_NoEditingChannels, NULL, 0))
        MLLoadString(IDS_PROPERTIES, aszRemove[3], ARRAYSIZE(aszRemove[3]));

    TCHAR           szBuffer[62];
    MENUITEMINFO    mii;

    mii.cbSize     = sizeof(MENUITEMINFO);
    mii.fMask      = MIIM_TYPE;

    for (int i = GetMenuItemCount(hmenu) - 1; i >= 0; i--)
    {
        mii.dwTypeData = szBuffer;
        mii.cch = ARRAYSIZE(szBuffer);

        if (GetMenuItemInfo(hmenu, i, TRUE, &mii) && mii.cch)
        {
            for (int j = 0; j < ARRAYSIZE(aszRemove); j++)
            {
                if (StrEql(aszRemove[j], mii.dwTypeData))
                {
                    DeleteMenu(hmenu, i, MF_BYPOSITION);
                    break;
                }
            }
        }
    }

    return;
}

void CChannelMenu::Refresh(HWND hwnd)
{
    IXMLDocument* pIXMLDocument;

    DLL_ForcePreloadDlls(PRELOAD_MSXML);
    
    HRESULT hr = CoCreateInstance(CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IXMLDocument, (void**)&pIXMLDocument);

    if (SUCCEEDED(hr))
    {
        ASSERT(pIXMLDocument);

        if (DownloadCdfUI(hwnd, m_bstrURL, pIXMLDocument))
        {
            UpdateImage(m_szPath);
            SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH | SHCNF_FLUSH,
                           (void*)m_szPath, NULL);
        }

        pIXMLDocument->Release();
    }
}

static TCHAR c_szFileProtocol[] = TEXT("file:");
static TCHAR c_szCDFExtension[] = TEXT(".cdf");
static TCHAR c_szShellEdit[] = TEXT("\\shell\\edit\\command");
static TCHAR c_szEditVerb[] = TEXT("edit");
static TCHAR c_szChannelFile[] = TEXT("ChannelFile");
static TCHAR c_szChannelFileEdit[] = TEXT("ChannelFile\\shell\\edit\\command");
static TCHAR c_szNotepad[] = TEXT("notepad.exe");

void CChannelMenu::ViewSource(HWND hwnd)
{
    TCHAR szProgId[64] = TEXT("");
    TCHAR szBuf[INTERNET_MAX_URL_LENGTH];
    TCHAR szFile[MAX_PATH + 2]; // Leave room for quotes
    DWORD cch, dwType;
    SHELLEXECUTEINFO sei;
    BOOL fFoundProg = FALSE;

    TraceMsg(TF_OBJECTS, "+ IContextMenu ViewSource %ls", m_bstrURL);

    if (SHUnicodeToTChar(m_bstrURL, szBuf, ARRAYSIZE(szBuf)))
    {
        if (SUCCEEDED(URLGetLocalFileName(szBuf, szFile, ARRAYSIZE(szFile),
                                          NULL)))
        {
            if (StrCmpNI(szFile, c_szFileProtocol, 5) == 0)
            {
                ASSERT(ARRAYSIZE(szFile) < ARRAYSIZE(szBuf));
                StrCpy(szBuf, szFile);
                cch = ARRAYSIZE(szFile) - 2;
                PathCreateFromUrl(szBuf, szFile, &cch, 0);
            }
            PathQuoteSpaces(szFile);

            //  
            //  We don't just call ShellExec with edit verb since
            //  who knows what the file extension will be.
            //
            cch = ARRAYSIZE(szProgId);
            if (ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, 
                                            c_szCDFExtension, 
                                            NULL, &dwType, 
                                            szProgId, &cch)
                )
            {
                ASSERT(ARRAYSIZE(szProgId) < ARRAYSIZE(szBuf));
                StrCpy(szBuf, szProgId);
                ASSERT(ARRAYSIZE(szProgId) + ARRAYSIZE(c_szShellEdit) <
                       ARRAYSIZE(szBuf));
                StrCat(szBuf, c_szShellEdit);
                cch = ARRAYSIZE(szBuf);
                                    
                if (ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, szBuf, 
                                                NULL, &dwType, szBuf, &cch)
                    )
                {
                    //
                    // Getting here means they have an edit verb for CDF files
                    //
                    fFoundProg = TRUE;
                }
            }

            //  
            //  If we haven't found a class key yet and the CDF ProgID 
            //  isn't ours, then fall back to our edit verb.
            //
            if (!fFoundProg && StrCmpI(szProgId, c_szChannelFile))
            {
                cch = ARRAYSIZE(szBuf);
                if (ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, 
                                                c_szChannelFileEdit, 
                                                NULL, &dwType, 
                                                szBuf, &cch)
                   )
                {
                    fFoundProg = TRUE;
                    ASSERT(ARRAYSIZE(c_szChannelFile) < ARRAYSIZE(szProgId));
                    StrCpy(szProgId, c_szChannelFile);
                }
            }

            memset(&sei, 0, sizeof(sei));
            sei.cbSize = sizeof(sei);
            sei.hwnd = hwnd;
            sei.nShow = SW_SHOW;
            
            if (fFoundProg)
            {
                sei.fMask = SEE_MASK_CLASSNAME;
                sei.lpVerb = c_szEditVerb;
                sei.lpFile = szFile;
                sei.lpClass = szProgId;
                TraceMsg(TF_OBJECTS, "IContextMenu ViewSource progid=%s file=%s", szProgId, szFile);
            }
            else
            {
                sei.lpFile = c_szNotepad;
                sei.lpParameters = szFile;
                TraceMsg(TF_OBJECTS, "IContextMenu ViewSource Notepad file=%s", szFile);
            }

#ifndef UNIX
            ShellExecuteEx(&sei);
#else
            unixInvokeEditor(szFile);
#endif /* UNIX */
        }
        else
        {
            CDFMessageBox(hwnd, IDS_ERROR_NO_CACHE_ENTRY, IDS_ERROR_DLG_TITLE,
                            MB_OK | MB_ICONEXCLAMATION, szBuf);
        }
    }
    else
    {
        TraceMsg(TF_OBJECTS, "IContextMenu ViewSource couldn't convert to TSTR");
    }
    TraceMsg(TF_OBJECTS, "- IContextMenu ViewSource");
}


HRESULT CChannelMenu::Subscribe(HWND hwnd)
{
    HRESULT hr = S_OK;

    CChannelMgr *pChannelMgr = new CChannelMgr;

    if (pChannelMgr)
    {
        hr = pChannelMgr->AddAndSubscribeEx2(hwnd, m_bstrURL, m_pSubscriptionMgr, TRUE);
        pChannelMgr->Release();

        if (SUCCEEDED(hr) && (NULL != m_pSubscriptionMgr))
        {
            hr = m_pSubscriptionMgr->UpdateSubscription(m_bstrURL);
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

