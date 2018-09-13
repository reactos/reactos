/**************************************************************\
    FILE: rundlg.cpp

    DESCRIPTION:
        CRunDlg implements the Dialog that can navigate through
    the Shell Name Space and ShellExec() commands.
\**************************************************************/

#include "shellprv.h"
extern "C" {
#include <regstr.h>
#include <shellp.h>
#include "ole2dup.h"
#include "ids.h"
#include "defview.h"
#include "lvutil.h"
#include "idlcomm.h"
#include "filetbl.h"
#include "undo.h"
#include "vdate.h"
} ;
#include "cnctnpt.h"

#ifndef SAFERELEASE
#define SAFERELEASE(x) { if (x) { x->Release(); x = NULL; } }
#endif

#ifdef WINNT
BOOL g_bCheckRunInSep = FALSE;
HANDLE g_hCheckNow = NULL;
HANDLE h_hRunDlgCS = NULL;
#endif // WINNT

const TCHAR c_szRunMRU[] = REGSTR_PATH_EXPLORER TEXT("\\RunMRU");
const TCHAR c_szRunDlgReady[] = TEXT("MSShellRunDlgReady");
const TCHAR c_szWaitingThreadID[] = TEXT("WaitingThreadID");
const TCHAR c_szQuote[] = TEXT("\"");
const TCHAR c_szAutoCompleteReg[] = REGSTR_PATH_EXPLORER TEXT("\\AutoComplete");
const TCHAR c_szACRunDlg[] = TEXT("Use AutoComplete");


BOOL RunDlgNotifyParent(HWND hDlg, HWND hwnd, LPTSTR pszCmd, LPCTSTR pszWorkingDir);
void ExchangeWindowPos(HWND hwnd0, HWND hwnd1);

#define WM_SETUPAUTOCOMPLETE (WM_APP)

/**************************************************************\
    CLASS: CRunDlg

    DESCRIPTION:
        CRunDlg implements the Dialog that can navigate through
    the Shell Name Space and ShellExec() commands.
\**************************************************************/
class CRunDlg
                : public IDropTarget
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj);

    // *** IDropTarget methods ***
    virtual STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragLeave(void);
    virtual STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);


protected:
    //////////////////////////////////////////////////////
    // Private Member Functions
    //////////////////////////////////////////////////////

    // Constructor / Destructor
    CRunDlg();
    ~CRunDlg(void);        // This is now an OLE Object and cannot be used as a normal Class.

    BOOL OKPushed(void);
    void ExitRunDlg(BOOL bOK);
    void InitRunDlg(HWND hDlg);
    void InitRunDlg2(HWND hDlg);
    void BrowsePushed(void);

    // Friend Functions
    friend HRESULT CRunDlg_CreateInstance(IUnknown *punkOuter, REFIID riid, IUnknown **ppunk);
    friend DWORD CALLBACK CheckRunInSeparateThreadProc(void *pv);
    friend BOOL_PTR CALLBACK RunDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    friend WINAPI RunFileDlg(HWND hwndParent, HICON hIcon, LPCTSTR pszWorkingDir, LPCTSTR pszTitle,
        LPCTSTR pszPrompt, DWORD dwFlags);

    //////////////////////////////////////////////////////
    //  Private Member Variables 
    //////////////////////////////////////////////////////
    UINT            m_cRef;

    // local data
    HWND            m_hDlg;

    // parameters
    HICON           m_hIcon;
    LPCTSTR         m_pszWorkingDir;
    LPCTSTR         m_pszTitle;
    LPCTSTR         m_pszPrompt;
    DWORD           m_dwFlags;
    HANDLE          m_hEventReady;
    DWORD           m_dwThreadId;

    BITBOOL         m_fDone : 1;
    BITBOOL         m_fAutoCompInitialized : 1;
};


// optimistic cache for this
HANDLE g_hMRURunDlg = NULL;

HANDLE OpenRunDlgMRU()
{
    HANDLE hmru = InterlockedExchangePointer(&g_hMRURunDlg, NULL);
    if (hmru == NULL)
    {
        MRUINFO mi =  {
            SIZEOF(MRUINFO),
            26,
            MRU_CACHEWRITE,
            HKEY_CURRENT_USER,
            c_szRunMRU,
            NULL        // NOTE: use default string compare
                        // since this is a GLOBAL MRU
        } ;
        hmru = CreateMRUList(&mi);
    }
    return hmru;
}

void CloseRunDlgMRU(HANDLE hmru)
{
    hmru = InterlockedExchangePointer(&g_hMRURunDlg, hmru);
    if (hmru)
        FreeMRUList(hmru);  // race, destroy copy
}

STDAPI_(void) FlushRunDlgMRU(void)
{
    CloseRunDlgMRU(NULL);
}


//================================================================= 
// Implementation of CRunDlg
//=================================================================


/****************************************************\
    FUNCTION: CRunDlg_CreateInstance
  
    DESCRIPTION:
        This function will create an instance of the
    CRunDlg COM object.
\****************************************************/
HRESULT CRunDlg_CreateInstance(IUnknown *punkOuter, REFIID riid, IUnknown **ppunk)
{
    if (punkOuter)
        return E_FAIL;

    *ppunk = NULL;
    CRunDlg * p = new CRunDlg();
    if (p) 
    {
    	*ppunk = SAFECAST(p, IDropTarget *);
	    return NOERROR;
    }

    return E_OUTOFMEMORY;
}




/****************************************************\
  
    Run Dialog Constructor
  
\****************************************************/
CRunDlg::CRunDlg()
{
    m_cRef = 1;

    // This needs to be allocated in Zero Inited Memory.
    // ASSERT that all Member Variables are inited to Zero.
    ASSERT(!m_hDlg);
    ASSERT(!m_hIcon);
    ASSERT(!m_pszWorkingDir);
    ASSERT(!m_pszTitle);
    ASSERT(!m_pszPrompt);
    ASSERT(!m_dwFlags);
    ASSERT(!m_hEventReady);
    ASSERT(!m_fDone);
    ASSERT(!m_dwThreadId);
}


/****************************************************\
  
    Run Dialog destructor
  
\****************************************************/
CRunDlg::~CRunDlg()
{
}


//===========================
// *** IUnknown Interface ***
HRESULT CRunDlg::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDropTarget))
    {
        *ppvObj = SAFECAST(this, IDropTarget*);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}



/****************************************************\
    FUNCTION: AddRef
\****************************************************/
ULONG CRunDlg::AddRef()
{
    m_cRef++;
    return m_cRef;
}

/****************************************************\
    FUNCTION: Release
\****************************************************/
ULONG CRunDlg::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}


//================================
// *** IDropTarget Interface ***

STDMETHODIMP CRunDlg::DragEnter(IDataObject * pdtobj, DWORD grfKeyState, POINTL ptl, LPDWORD pdwEffect)
{
    DebugMsg(DM_TRACE, TEXT("sh - TR CRunDlg::DragEnter called"));
    ASSERT(pdtobj);
    _DragEnter(m_hDlg, ptl, pdtobj);
    *pdwEffect &= DROPEFFECT_LINK | DROPEFFECT_COPY;
    return NOERROR;
}

STDMETHODIMP CRunDlg::DragOver(DWORD grfKeyState, POINTL ptl, LPDWORD pdwEffect)
{
    _DragMove(m_hDlg, ptl);
    *pdwEffect &= DROPEFFECT_LINK | DROPEFFECT_COPY;
    return NOERROR;
}

STDMETHODIMP CRunDlg::DragLeave(void)
{
    DebugMsg(DM_TRACE, TEXT("sh - TR CRunDlg::DragLeave called"));
    DAD_DragLeave();
    return NOERROR;
}

typedef struct {
    HRESULT (*pfnGetData)(STGMEDIUM *, LPTSTR pszFile);
    FORMATETC fmte;
} DATA_HANDLER;

HRESULT _GetHDROPFromData(STGMEDIUM *pmedium, LPTSTR pszPath)
{
    return DragQueryFile((HDROP)pmedium->hGlobal, 0, pszPath, MAX_PATH) ? S_OK : E_FAIL;
}

HRESULT _GetText(STGMEDIUM *pmedium, LPTSTR pszPath)
{
    LPCSTR psz = (LPCSTR)GlobalLock(pmedium->hGlobal);
    if (psz)
    {
        SHAnsiToTChar(psz, pszPath, MAX_PATH);
        GlobalUnlock(pmedium->hGlobal);
        return S_OK;
    }
    return E_FAIL;
}

HRESULT _GetUnicodeText(STGMEDIUM *pmedium, LPTSTR pszPath)
{
    LPCWSTR pwsz = (LPCWSTR)GlobalLock(pmedium->hGlobal);
    if (pwsz)
    {
        SHUnicodeToTChar(pwsz, pszPath, MAX_PATH);
        GlobalUnlock(pmedium->hGlobal);
        return S_OK;
    }
    return E_FAIL;
}

STDMETHODIMP CRunDlg::Drop(IDataObject * pdtobj, DWORD grfKeyState, 
                           POINTL pt, DWORD *pdwEffect)
{
    TCHAR szPath[MAX_PATH];

    DAD_DragLeave();

    szPath[0] = 0;

    DATA_HANDLER rg_data_handlers[] = {
        _GetHDROPFromData,  {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
        _GetUnicodeText,    {CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
        _GetText,           {g_cfShellURL, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
        _GetText,           {CF_TEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
    };

    IEnumFORMATETC *penum;

    if (SUCCEEDED(pdtobj->EnumFormatEtc(DATADIR_GET, &penum)))
    {
        FORMATETC fmte;
        while (penum->Next(1, &fmte, NULL) == S_OK)
        {
            for (int i = 0; i < ARRAYSIZE(rg_data_handlers); i++)
            {
                STGMEDIUM medium;
                if ((rg_data_handlers[i].fmte.cfFormat == fmte.cfFormat) && 
                    SUCCEEDED(pdtobj->GetData(&rg_data_handlers[i].fmte, &medium)))
                {
                    HRESULT hres = rg_data_handlers[i].pfnGetData(&medium, szPath);
                    ReleaseStgMedium(&medium);

                    if (SUCCEEDED(hres))
                        goto Done;
                }
            }
        }
Done:
        penum->Release();
    }

    if (szPath[0])
    {
        TCHAR szText[MAX_PATH + MAX_PATH];

        GetDlgItemText(m_hDlg, IDD_COMMAND, szText, ARRAYSIZE(szText) - ARRAYSIZE(szPath));
        if (szText[0])
            lstrcat(szText, c_szSpace);

        if (StrChr(szPath, TEXT(' '))) 
            PathQuoteSpaces(szPath);    // there's a space in the file... add qutoes

        lstrcat(szText, szPath);

        SetDlgItemText(m_hDlg, IDD_COMMAND, szText);
        EnableOKButtonFromID(m_hDlg, IDD_COMMAND);

#ifdef WINNT
        if (g_hCheckNow)
            SetEvent(g_hCheckNow);
#endif // WINNT

        *pdwEffect &= DROPEFFECT_COPY | DROPEFFECT_LINK;
    }
    else
        *pdwEffect = 0;

    return NOERROR;
}


BOOL PromptForMedia(HWND hwnd, LPCTSTR pszPath)
{
    BOOL fContinue = TRUE;
    TCHAR szPathTemp[MAX_URL_STRING];
    
    StrCpyN(szPathTemp, pszPath, ARRAYSIZE(szPathTemp));
    PathRemoveArgs(szPathTemp);
    PathUnquoteSpaces(szPathTemp);

    // We only want to check for media if it's a drive path
    // because the Start->Run dialog can receive all kinds of
    // wacky stuff. (Relative paths, URLs, App Path exes, 
    // any shell exec hooks, etc.)
    if (-1 != PathGetDriveNumber(szPathTemp))
    {
        if (FAILED(SHPathPrepareForWrite(hwnd, NULL, szPathTemp, SHPPFW_IGNOREFILENAME)))
            fContinue = FALSE;      // User decliened to insert or format media.
    }

    return fContinue;
}

//================================
// *** Private Members ***

BOOL CRunDlg::OKPushed(void)
{
    HWND hwndOwner;
    TCHAR szText[MAX_PATH];
    TCHAR szTitle[64];
    DWORD dwFlags;
    BOOL fSuccess = FALSE;
    int iRun;
    HWND hDlg = m_hDlg;
    TCHAR szNotExp[ MAX_PATH ];

    if (m_fDone)
        return TRUE;

    // Get out of the "synchronized input queues" state
    if (m_dwThreadId)
    {
        AttachThreadInput(GetCurrentThreadId(), m_dwThreadId, FALSE);
    }

    // Get the command line and dialog title, leave some room for the slash on the end
    GetDlgItemText(hDlg, IDD_COMMAND, szNotExp, ARRAYSIZE(szNotExp) - 2);
    PathRemoveBlanks(szNotExp);

    // This used to happen only on NT, do it everywhere:
    SHExpandEnvironmentStrings(szNotExp, szText, ARRAYSIZE(szText) - 2);

    // We will go ahead if this isn't a file path.  If it is, we
    if (PromptForMedia(hDlg, szText))
    {
        GetWindowText(hDlg, szTitle, ARRAYSIZE(szTitle));

        // Hide this dialog (REVIEW, to avoid save bits window flash)
        SetWindowPos(hDlg, 0, 0, 0, 0, 0, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER);

        //
        // HACK: We need to activate the owner window before we call
        //  ShellexecCmdLine, so that our folder DDE code can find an
        //  explorer window as the ForegroundWindow.
        //
        hwndOwner = GetWindow(hDlg, GW_OWNER);
        if (hwndOwner)
        {
            SetActiveWindow(hwndOwner);
        }
        else
        {
            hwndOwner = hDlg;
        }

        iRun = RunDlgNotifyParent(hDlg, hwndOwner, szText, m_pszWorkingDir);
        switch (iRun)
        {
        case RFR_NOTHANDLED:
            if (m_dwFlags & RFD_USEFULLPATHDIR)
            {
                dwFlags = SECL_USEFULLPATHDIR;
            }
            else
            {
                dwFlags = 0;
            }

#ifdef WINNT
            if ((!(m_dwFlags & RFD_NOSEPMEMORY_BOX)) && (m_dwFlags & RFD_WOW_APP))
            {
                if (IsDlgButtonChecked( hDlg, IDD_RUNINSEPARATE ) == 1 )
                {
                    dwFlags |= SECL_SEPARATE_VDM;
                }
            }
#endif

#ifdef WINNT
            ShrinkWorkingSet();
#endif

            fSuccess = ShellExecCmdLine( hwndOwner,
                                         szText,
                                         m_pszWorkingDir,
                                         SW_SHOWNORMAL,
                                         szTitle,
                                         dwFlags
                                        );
            break;

        case RFR_SUCCESS:
            fSuccess = TRUE;
            break;

        case RFR_FAILURE:
            fSuccess = FALSE;
            break;
        }
    }

    // Get back into "synchronized input queues" state
    if (m_dwThreadId)
    {
        AttachThreadInput(GetCurrentThreadId(), m_dwThreadId, TRUE);
    }

    if (fSuccess)
    {
        HANDLE hmru = OpenRunDlgMRU();
        if (hmru)
        {
            // NB the old MRU format has a slash and the show cmd on the end
            // we need to maintain that so we don't end up with garbage on
            // the end of the line
            lstrcat(szNotExp, TEXT("\\1"));
            AddMRUString(hmru, szNotExp);

            CloseRunDlgMRU(hmru);
        }
        return TRUE;
    }

    // Something went wrong. Put the dialog back up.
    SetWindowPos(hDlg, 0, 0, 0, 0, 0, SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER);
    SetForegroundWindow(hDlg);
    SetFocus(GetDlgItem(hDlg, IDD_COMMAND));
    return FALSE;
}


void CRunDlg::ExitRunDlg(BOOL bOK)
{
    if (!m_fDone) 
    {
        RevokeDragDrop(m_hDlg);
        m_fDone = TRUE;
    }

#ifdef WINNT

    if (!(m_dwFlags & RFD_NOSEPMEMORY_BOX))
    {
        g_bCheckRunInSep = FALSE;
        SetEvent( g_hCheckNow );
    }

#endif // WINNT

    EndDialog(m_hDlg, bOK);
}


void CRunDlg::InitRunDlg(HWND hDlg)
{
    HWND hCB;

    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)this);

    if (m_pszTitle)
        SetWindowText(hDlg, m_pszTitle);

    if (m_pszPrompt)
        SetDlgItemText(hDlg, IDD_PROMPT, m_pszPrompt);

    if (m_hIcon)
        Static_SetIcon(GetDlgItem(hDlg, IDD_ICON), m_hIcon);

    if (m_dwFlags & RFD_NOBROWSE)
    {
        HWND hBrowse = GetDlgItem(hDlg, IDD_BROWSE);

        ExchangeWindowPos(hBrowse, GetDlgItem(hDlg, IDCANCEL));
        ExchangeWindowPos(hBrowse, GetDlgItem(hDlg, IDOK));

        ShowWindow(hBrowse, SW_HIDE);
    }

    if (m_dwFlags & RFD_NOSHOWOPEN)
        ShowWindow(GetDlgItem(hDlg, IDD_RUNDLGOPENPROMPT), SW_HIDE);

    hCB = GetDlgItem(hDlg, IDD_COMMAND);
    SendMessage(hCB, CB_LIMITTEXT, MAX_PATH - 1, 0L);

    HANDLE hmru = OpenRunDlgMRU();
    if (hmru)
    {
        for (int nMax = EnumMRUList(hmru, -1, NULL, 0), i=0; i<nMax; ++i)
        {
            TCHAR szCommand[MAX_PATH];
            if (EnumMRUList(hmru, i, szCommand, ARRAYSIZE(szCommand)) > 0)
            {
                // old MRU format has a slash at the end with the show cmd
                LPTSTR pszField = StrRChr(szCommand, NULL, TEXT('\\'));
                if (pszField)
                    *pszField = 0;

                // The command to run goes in the combobox.
                SendMessage(hCB, CB_ADDSTRING, 0, (LPARAM)(LPTSTR)szCommand);
            }
        }
        CloseRunDlgMRU(hmru);
    }

    if (!(m_dwFlags & RFD_NODEFFILE))
        SendMessage(hCB, CB_SETCURSEL, 0, 0L);

    SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDD_COMMAND, CBN_SELCHANGE), (LPARAM) hCB);

    // Make sure the OK button is initialized properly
    EnableOKButtonFromID(hDlg, IDD_COMMAND);

#ifdef WINNT
    //
    // Create the thread that will take care of the
    // "Run in Separate Memory Space" checkbox in the background.
    //
    if (m_dwFlags & RFD_NOSEPMEMORY_BOX)
    {
        ShowWindow(GetDlgItem(hDlg, IDD_RUNINSEPARATE), SW_HIDE);
    }
    else
    {
        HANDLE hThread = NULL;
        ASSERT( g_hCheckNow == NULL );
        g_hCheckNow = CreateEvent( NULL, TRUE, FALSE, NULL );
        if (g_hCheckNow) 
        {
            DWORD dwDummy;
            g_bCheckRunInSep = TRUE;
            hThread = CreateThread(NULL, 0, CheckRunInSeparateThreadProc, hDlg, 0, &dwDummy);
        }

        if ((g_hCheckNow==NULL) || (!g_bCheckRunInSep) || (hThread==NULL)) 
        {
            // We've encountered a problem setting up, so make the user
            // choose.
            CheckDlgButton( hDlg, IDD_RUNINSEPARATE, 1 );
            EnableWindow( GetDlgItem( hDlg, IDD_RUNINSEPARATE ), TRUE );
            g_bCheckRunInSep = FALSE;
        }

        //
        // These calls will just do nothing if either handle is NULL.
        //
        if (hThread)
            CloseHandle( hThread );
        if (g_hCheckNow)
            SetEvent( g_hCheckNow );
    }
#endif // WINNT
}

//
// InitRunDlg 2nd phase. It must be called after freeing parent thread.
//
void CRunDlg::InitRunDlg2(HWND hDlg)
{
    // Register ourselves as a drop target. Allow people to drop on
    // both the dlg box and edit control.
    RegisterDragDrop(hDlg, SAFECAST(this, IDropTarget*));
}


void CRunDlg::BrowsePushed(void)
{
    HWND hDlg = m_hDlg;
    TCHAR szText[MAX_PATH];

    // Get out of the "synchronized input queues" state
    if (m_dwThreadId)
    {
        AttachThreadInput(GetCurrentThreadId(), m_dwThreadId, FALSE);
        m_dwThreadId = 0;
    }

    GetDlgItemText(hDlg, IDD_COMMAND, szText, ARRAYSIZE(szText));
    PathUnquoteSpaces(szText);

    if (GetFileNameFromBrowse(hDlg, szText, ARRAYSIZE(szText), m_pszWorkingDir,
            MAKEINTRESOURCE(IDS_EXE), MAKEINTRESOURCE(IDS_PROGRAMSFILTER),
            MAKEINTRESOURCE(IDS_BROWSE)))
    {
        PathQuoteSpaces(szText);
        SetDlgItemText(hDlg, IDD_COMMAND, szText);
        EnableOKButtonFromID(hDlg, IDD_COMMAND);
        SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, IDOK), TRUE);
    }
}




//================================
// *** Other Functions ***


// Use the common browse dialog to get a filename.
// The working directory of the common dialog will be set to the directory
// part of the file path if it is more than just a filename.
// If the filepath consists of just a filename then the working directory
// will be used.
// The full path to the selected file will be returned in szFilePath.
//    HWND hDlg,           // Owner for browse dialog.
//    LPSTR szFilePath,    // Path to file
//    UINT cchFilePath,     // Max length of file path buffer.
//    LPSTR szWorkingDir,  // Working directory
//    LPSTR szDefExt,      // Default extension to use if the user doesn't
//                         // specify enter one.
//    LPSTR szFilters,     // Filter string.
//    LPSTR szTitle        // Title for dialog.

STDAPI_(BOOL) _GetFileNameFromBrowse(HWND hwnd, LPTSTR szFilePath, UINT cbFilePath,
                                       LPCTSTR szWorkingDir, LPCTSTR szDefExt, LPCTSTR szFilters, LPCTSTR szTitle,
                                       DWORD dwFlags)
{
    OPENFILENAME ofn;                 // Structure used to init dialog.
    TCHAR szBrowserDir[MAX_PATH];      // Directory to start browsing from.
    TCHAR szFilterBuf[MAX_PATH];       // if szFilters is MAKEINTRESOURCE
    TCHAR szDefExtBuf[10];             // if szDefExt is MAKEINTRESOURCE
    TCHAR szTitleBuf[64];              // if szTitleBuf is MAKEINTRESOURCE

    szBrowserDir[0] = TEXT('0'); // By default use CWD.

    // Set up info for browser.
    lstrcpy(szBrowserDir, szFilePath);
    PathRemoveArgs(szBrowserDir);
    PathRemoveFileSpec(szBrowserDir);

    if (*szBrowserDir == TEXT('\0') && szWorkingDir)
    {
        lstrcpyn(szBrowserDir, szWorkingDir, ARRAYSIZE(szBrowserDir));
    }

    // Stomp on the file path so that the dialog doesn't
    // try to use it to initialise the dialog. The result is put
    // in here.
    szFilePath[0] = TEXT('\0');

    // Set up szDefExt
    if (IS_INTRESOURCE(szDefExt))
    {
        LoadString(HINST_THISDLL, (UINT)LOWORD((DWORD_PTR)szDefExt), szDefExtBuf, ARRAYSIZE(szDefExtBuf));
        szDefExt = szDefExtBuf;
    }

    // Set up szFilters
    if (IS_INTRESOURCE(szFilters))
    {
        LPTSTR psz;

        LoadString(HINST_THISDLL, (UINT)LOWORD((DWORD_PTR)szFilters), szFilterBuf, ARRAYSIZE(szFilterBuf));
        psz = szFilterBuf;
        while (*psz)
        {
            if (*psz == TEXT('#'))
#if (defined(DBCS) || (defined(FE_SB) && !defined(UNICODE)))
                *psz++ = TEXT('\0');
            else
                psz = CharNext(psz);
#else
            *psz = TEXT('\0');
            psz = CharNext(psz);
#endif
        }
        szFilters = szFilterBuf;
    }

    // Set up szTitle
    if (IS_INTRESOURCE(szTitle))
    {
        LoadString(HINST_THISDLL, (UINT)LOWORD((DWORD_PTR)szTitle), szTitleBuf, ARRAYSIZE(szTitleBuf));
        szTitle = szTitleBuf;
    }

    // Setup info for comm dialog.
    ofn.lStructSize       = SIZEOF(ofn);
    ofn.hwndOwner         = hwnd;
    ofn.hInstance         = NULL;
    ofn.lpstrFilter       = szFilters;
    ofn.lpstrCustomFilter = NULL;
    ofn.nFilterIndex      = 1;
    ofn.nMaxCustFilter    = 0;
    ofn.lpstrFile         = szFilePath;
    ofn.nMaxFile          = cbFilePath;
    ofn.lpstrInitialDir   = szBrowserDir;
    ofn.lpstrTitle        = szTitle;
    ofn.Flags             = dwFlags;
    ofn.lpfnHook          = NULL;
    ofn.lpstrDefExt       = szDefExt;
    ofn.lpstrFileTitle    = NULL;

    // Call it.
    return GetOpenFileName(&ofn);
}


BOOL WINAPI GetFileNameFromBrowse(HWND hwnd, LPTSTR szFilePath, UINT cchFilePath,
        LPCTSTR szWorkingDir, LPCTSTR szDefExt, LPCTSTR szFilters, LPCTSTR szTitle)
{
    return _GetFileNameFromBrowse(hwnd, szFilePath, cchFilePath,
                                 szWorkingDir, szDefExt, szFilters, szTitle,
                                 OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_NODEREFERENCELINKS);

}


#ifdef WINNT

//
// Do checking of the .exe type in the background so the UI doesn't
// get hung up while we scan.  This is particularly important with
// the .exe is over the network or on a floppy.
//
DWORD CALLBACK CheckRunInSeparateThreadProc(void *pv)
{
    DWORD dwBinaryType;
    DWORD cch;
    LPTSTR pszFilePart;
    TCHAR szFile[MAX_PATH+1];
    TCHAR szFullFile[MAX_PATH+1];
    TCHAR szExp[MAX_PATH+1];
    HWND hDlg = (HWND)pv;
    BOOL fCheck = TRUE, fEnable = FALSE;

    HRESULT hr = CoInitialize(0);
    if (EVAL(SUCCEEDED(hr)))
    {
        // BUGBUG: Re-write to use PIDL from CShellUrl because it will prevent from having
        //         to do the Search for the file name a second time.

        DebugMsg( DM_TRACE, TEXT("CheckRunInSeparateThreadProc created and running") );

        while( g_bCheckRunInSep )
        {
            WaitForSingleObject( g_hCheckNow, INFINITE );
            ResetEvent( g_hCheckNow );

            if (g_bCheckRunInSep)
            {
                CRunDlg * prd;
                LPTSTR pszT;
                BOOL f16bit = FALSE;

                szFile[0] = 0;
                szFullFile[0] = 0;
                cch = 0;
                GetWindowText( GetDlgItem( hDlg, IDD_COMMAND ), szFile, ARRAYSIZE(szFile) );
                // Remove & throw away arguments
                PathRemoveBlanks(szFile);

                if (PathIsNetworkPath(szFile))
                {
                    f16bit = TRUE;
                    fCheck = FALSE;
                    fEnable = TRUE;
                    goto ChangeTheBox;
                }

                // if the unquoted string exists as a file, just use it

                if (!PathFileExistsAndAttributes(szFile, NULL))
                {
                    pszT = PathGetArgs(szFile);
                    if (*pszT)
                        *(pszT - 1) = TEXT('\0');

                    PathUnquoteSpaces(szFile);
                }

                if (szFile[0])
                {
                    SHExpandEnvironmentStrings(szFile, szExp, ARRAYSIZE(szExp));

                    if (PathIsUNC(szExp) || IsRemoteDrive(DRIVEID(szExp)))
                    {
                        f16bit = TRUE;
                        fCheck = FALSE;
                        fEnable = TRUE;
                        goto ChangeTheBox;
                    }

                    cch = SearchPath(NULL, szExp, TEXT(".EXE"),
                                     ARRAYSIZE(szExp), szFullFile, &pszFilePart);
                }

                if ((cch != 0) && (cch <= (ARRAYSIZE(szFullFile) - 1)))
                {
                    if ( (GetBinaryType(szFullFile, &dwBinaryType) &&
                         (dwBinaryType == SCS_WOW_BINARY)) )
                    {
                        f16bit = TRUE;
                        fCheck = FALSE;
                        fEnable = TRUE;
                    } 
                    else 
                    {
                        f16bit = FALSE;
                        fCheck = TRUE;
                        fEnable = FALSE;
                    }
                } 
                else 
                {
                    f16bit = FALSE;
                    fCheck = TRUE;
                    fEnable = FALSE;
                }

    ChangeTheBox:
                CheckDlgButton( hDlg, IDD_RUNINSEPARATE, fCheck ? 1 : 0 );
                EnableWindow( GetDlgItem( hDlg, IDD_RUNINSEPARATE ), fEnable );

                prd = (CRunDlg *)GetWindowLongPtr(hDlg, DWLP_USER);
                if (prd)
                {
                    if (f16bit)
                        prd->m_dwFlags |= RFD_WOW_APP;
                    else
                        prd->m_dwFlags &= (~RFD_WOW_APP);
                }
            }
        }
    }
    CloseHandle( g_hCheckNow );
    g_hCheckNow = NULL;

    CoUninitialize();
    return 0;
}

#endif // WINNT


void ExchangeWindowPos(HWND hwnd0, HWND hwnd1)
{
    HWND hParent;
    RECT rc[2];

    hParent = GetParent(hwnd0);
    ASSERT(hParent == GetParent(hwnd1));

    GetWindowRect(hwnd0, &rc[0]);
    GetWindowRect(hwnd1, &rc[1]);

    MapWindowPoints(HWND_DESKTOP, hParent, (LPPOINT)rc, 4);

    SetWindowPos(hwnd0, NULL, rc[1].left, rc[1].top, 0, 0,
            SWP_NOZORDER|SWP_NOSIZE);
    SetWindowPos(hwnd1, NULL, rc[0].left, rc[0].top, 0, 0,
            SWP_NOZORDER|SWP_NOSIZE);
}

BOOL RunDlgNotifyParent(HWND hDlg, HWND hwnd, LPTSTR pszCmd, LPCTSTR pszWorkingDir)
{
    NMRUNFILE rfn;

    rfn.hdr.hwndFrom = hDlg;
    rfn.hdr.idFrom = 0;
    rfn.hdr.code = RFN_EXECUTE;
    rfn.lpszCmd = pszCmd;
    rfn.lpszWorkingDir = pszWorkingDir;
    rfn.nShowCmd = SW_SHOWNORMAL;

    return (BOOL) SendMessage(hwnd, WM_NOTIFY, 0, (LPARAM)&rfn);
}

void MRUSelChange(HWND hDlg)
{
    TCHAR szCmd[MAX_PATH];
    HWND hCB = GetDlgItem(hDlg, IDD_COMMAND);
    int nItem = (int)SendMessage(hCB, CB_GETCURSEL, 0, 0L);
    if (nItem < 0)
        return;

    // BUGBUG REVIEW: Buffer overrun potential here?
    SendMessage(hCB, CB_GETLBTEXT, nItem, (LPARAM)szCmd);

    // We can't use EnableOKButtonFromID here because when we get this message,
    // the window does not have the text yet, so it will fail.
    EnableOKButtonFromString(hDlg, szCmd);
}


/* REVIEW UNDONE - Environment substitution, CL history, WD field, run as styles.
//---------------------------------------------------------------------------*/

const DWORD aRunHelpIds[] = {
        IDD_ICON,             NO_HELP,
        IDD_PROMPT,           NO_HELP,
        IDD_RUNDLGOPENPROMPT, IDH_TRAY_RUN_COMMAND,
        IDD_COMMAND,          IDH_TRAY_RUN_COMMAND,
#ifdef WINNT
        IDD_RUNINSEPARATE,    IDH_TRAY_RUN_SEPMEM,
#endif
        IDD_BROWSE,           IDH_BROWSE,
        IDOK,                 IDH_TRAY_RUN_OK,
        IDCANCEL,             IDH_TRAY_RUN_CANCEL,

        0, 0
};

BOOL_PTR CALLBACK RunDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CRunDlg * prd = (CRunDlg *)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        /* The title will be in the lParam. */
        prd = (CRunDlg *)lParam;
        prd->m_hDlg = hDlg;
        prd->m_fDone = FALSE;
        prd->InitRunDlg(hDlg);
        // Let the parent thread run on if it was waiting for us to
        // grab type-ahead.
        if (prd->m_hEventReady)
        {
            // We need to grab the activation so we can process input.
            // DebugMsg(DM_TRACE, "s.rdp: Getting activation.");
            SetForegroundWindow(hDlg);
            SetFocus(GetDlgItem(hDlg, IDD_COMMAND));
            // Now it's safe to wake the guy up properly.
            // DebugMsg(DM_TRACE, "s.rdp: Waking sleeping parent.");
            SetEvent(prd->m_hEventReady);
            CloseHandle(prd->m_hEventReady);
        }       
        else
        {
            SetForegroundWindow(hDlg);
            SetFocus(GetDlgItem(hDlg, IDD_COMMAND));
        }

        // InitRunDlg 2nd phase (must be called after SetEvent)
        prd->InitRunDlg2(hDlg);

        // We're handling focus changes.
        return FALSE;

    case WM_PAINT:
        if (!prd->m_fAutoCompInitialized)
        {
            prd->m_fAutoCompInitialized = TRUE;
            PostMessage(hDlg, WM_SETUPAUTOCOMPLETE, 0, 0);
        }
        return FALSE;

    case WM_SETUPAUTOCOMPLETE:
        SHAutoComplete(GetWindow(GetDlgItem(hDlg, IDD_COMMAND), GW_CHILD), (SHACF_FILESYSTEM | SHACF_URLALL | SHACF_FILESYS_ONLY));
        break;

    case WM_HELP:
        WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP,
            (ULONG_PTR) (LPTSTR) aRunHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
            (ULONG_PTR) (LPTSTR) aRunHelpIds);
        break;

    case WM_DESTROY:
        break;

    case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDHELP:
                break;

            case IDD_COMMAND:
                switch (GET_WM_COMMAND_CMD(wParam, lParam))
                {
                case CBN_SELCHANGE:
                    MRUSelChange(hDlg);
#ifdef WINNT
                    if ( g_hCheckNow )
                        SetEvent( g_hCheckNow );
#endif // WINNT
                    break;

                case CBN_EDITCHANGE:
                case CBN_SELENDOK:
                    EnableOKButtonFromID(hDlg, IDD_COMMAND);
#ifdef WINNT
                    if ( g_hCheckNow )
                        SetEvent( g_hCheckNow );
#endif // WINNT
                    break;
                }
                break;

            case IDOK:
            // fake an ENTER key press so AutoComplete can do it's thing
            if ( SendMessage( GetDlgItem( hDlg, IDD_COMMAND), WM_KEYDOWN, VK_RETURN, 0x1c0001 ) )
            {
                if (!prd->OKPushed()) {
#ifdef WINNT
                    if (!(prd->m_dwFlags & RFD_NOSEPMEMORY_BOX))
                    {
                        g_bCheckRunInSep = FALSE;
                        SetEvent( g_hCheckNow );
                    }
#endif // WINNT
                    break;
                }
            }
            else
            {
                break;  // AutoComplete wants more user input
            }
            // fall through

            case IDCANCEL:
                prd->ExitRunDlg(FALSE);
                break;

            case IDD_BROWSE:
                prd->BrowsePushed();
#ifdef WINNT
                SetEvent( g_hCheckNow );
#endif // WINNT
                break;

            default:
                return FALSE;
            }
            break;

    default:
        return FALSE;
    }
    return TRUE;
}

//---------------------------------------------------------------------------
// Puts up the standard file.run dialog.
// REVIEW UNDONE This should use a RUNDLG structure for all the various
// options instead of just passing them as parameters, a ptr to the struct
// would be passed to the dialog via the lParam.
STDAPI_(int) RunFileDlg(HWND hwndParent, HICON hIcon, 
                        LPCTSTR pszWorkingDir, LPCTSTR pszTitle,
                        LPCTSTR pszPrompt, DWORD dwFlags)
{
    int rc = 0;
    IDropTarget *pdt;

    CoInitialize(0);

    CRunDlg_CreateInstance(NULL, IID_IDropTarget, (IUnknown **)&pdt);

    if (pdt)
    {
        CRunDlg * prd = (CRunDlg *) pdt;

        prd->m_hIcon = hIcon;
        prd->m_pszWorkingDir = pszWorkingDir;
        prd->m_pszTitle = pszTitle;
        prd->m_pszPrompt = pszPrompt;
        prd->m_dwFlags = dwFlags;

        if (SHRestricted(REST_RUNDLGMEMCHECKBOX))
            ClearFlag(prd->m_dwFlags, RFD_NOSEPMEMORY_BOX);
        else
            SetFlag(prd->m_dwFlags, RFD_NOSEPMEMORY_BOX);

        // prd->m_hEventReady = 0;
        // prd->m_dwThreadId = 0;

        // We do this so we can get type-ahead when we're running on a
        // separate thread. The parent thread needs to block to give us time
        // to do the attach and then get some messages out of the queue hence
        // the event.
        if (hwndParent)
        {
            // HACK The parent signals it's waiting for the dialog to grab type-ahead
            // by sticking it's threadId in a property on the parent.
            prd->m_dwThreadId = PtrToUlong(GetProp(hwndParent, c_szWaitingThreadID));
            if (prd->m_dwThreadId)
            {
                // DebugMsg(DM_TRACE, "s.rfd: Attaching input to %x.", idThread);
                AttachThreadInput(GetCurrentThreadId(), prd->m_dwThreadId, TRUE);
                // NB Hack.
                prd->m_hEventReady = OpenEvent(EVENT_ALL_ACCESS, TRUE, c_szRunDlgReady);
            }
        }

        rc = (int)DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_RUN), hwndParent,
                            RunDlgProc, (LPARAM)prd);

        pdt->Release();
    }

    CoUninitialize();
    return rc;
}
