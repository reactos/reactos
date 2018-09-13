#include "precomp.hxx"
#pragma hdrstop

#include "stdext.h"
#include "msg.h"
#include "guids.h"

//
// "Cookies" defined for topics.
//
const DWORD TOPIC_PRINTERS        =  0;
const DWORD TOPIC_STARTMENU       =  1;
const DWORD TOPIC_WINLOOK         =  2;
const DWORD TOPIC_PROGRAMS        =  3;
const DWORD TOPIC_NETWORK         =  4;
const DWORD TOPIC_HARDWARE        =  5;
const DWORD TOPIC_ACCESSIBILITY   =  6;
const DWORD TOPIC_CPANEL          =  7;

#ifdef WINNT
    const INT MSG_TITLE_TOPIC_NETWORK = MSG_TITLE_TOPIC_NETWORK_WINNT;
#else
    const INT MSG_TITLE_TOPIC_NETWORK = MSG_TITLE_TOPIC_NETWORK_WINDOWS;
#endif

const NEW_TOPIC_DATA c_rgDataTopics[] = {

    { TOPIC_PRINTERS,      MSG_TITLE_TOPIC_PRINTERS,       MSG_DESC_TOPIC_PRINTERS,     IDB_TOPIC_PRINTERS,     REST_NOPRINTERADD },
//
// Start menu option deactivated until we find out what's going to 
// happen with SMTidy.exe.
//
//    { TOPIC_STARTMENU,     MSG_TITLE_TOPIC_STARTMENU,      MSG_DESC_TOPIC_STARTMENU,    IDB_TOPIC_STARTMENU,    REST_NOSETTASKBAR },
    { TOPIC_WINLOOK,       MSG_TITLE_TOPIC_WINLOOK,        MSG_DESC_TOPIC_WINLOOK,      IDB_TOPIC_WINLOOK,      REST_NONE         },
    { TOPIC_PROGRAMS,      MSG_TITLE_TOPIC_PROGRAMS,       MSG_DESC_TOPIC_PROGRAMS,     IDB_TOPIC_PROGRAMS,     REST_NONE         },
    { TOPIC_NETWORK,       MSG_TITLE_TOPIC_NETWORK,        MSG_DESC_TOPIC_NETWORK,      IDB_TOPIC_NETWORK,      REST_NOWEB        },
    { TOPIC_HARDWARE,      MSG_TITLE_TOPIC_HARDWARE,       MSG_DESC_TOPIC_HARDWARE,     IDB_TOPIC_HARDWARE,     REST_NONE         },
    { TOPIC_ACCESSIBILITY, MSG_TITLE_TOPIC_ACCESSIBILITY,  MSG_DESC_TOPIC_ACCESSIBILITY,IDB_TOPIC_ACCESSIBILITY,REST_NONE         },
    { TOPIC_CPANEL,        MSG_TITLE_TOPIC_CPANEL,         MSG_DESC_TOPIC_CPANEL,       IDB_TOPIC_CPANEL,       REST_NOSETFOLDERS }};
                

//
// Names of things that get run when topics are selected.
//
const TCHAR c_szRunDLL32[]             = TEXT("rundll32");
const TCHAR c_szShell32_DLL[]          = TEXT("shell32.dll");
const TCHAR c_szSysDm_CPL[]            = TEXT("sysdm.cpl");
const TCHAR c_szControl_RunDLL[]       = TEXT("Control_RunDLL");
const TCHAR c_szInstallDevice_RunDLL[] = TEXT("InstallDevice_RunDLL");
const TCHAR c_szHelpShortcuts_RunDLL[] = TEXT("SHHelpShortcuts_RunDLL");
const TCHAR c_szCPLAppWizard[]         = TEXT("appwiz.cpl");
const TCHAR c_szCPLDesktop[]           = TEXT("desk.cpl");
const TCHAR c_szCPLAccessibility[]     = TEXT("access.cpl");
const TCHAR c_szCPLNewDevice[]         = TEXT("newdev.cpl");
const TCHAR c_szAddPrinter[]           = TEXT("AddPrinter");
const TCHAR c_szInternetCnxWizard[]    = TEXT("icwconn1.exe");


//
// How long to wait before timing out on a launched applet.
// Timing out just means we return to the caller and they'll think
// that the applet is up.  Used for cursor appearance control.
//
static const INT EXEC_WAIT_TIMEOUT = 10000;  // milliseconds.



///////////////////////////////////////////////////////////////////////////////
// SettingsFolderExt
///////////////////////////////////////////////////////////////////////////////

SettingsFolderExt::SettingsFolderExt(
    VOID
    )
{
    //
    // Nothing to do.
    //
}


SettingsFolderExt::~SettingsFolderExt(
    VOID
    )
{
    //
    // Nothing to do.
    //
}


STDMETHODIMP 
SettingsFolderExt::QueryInterface(
    REFIID riid, 
    LPVOID *ppvOut
    )
{
    HRESULT hResult = NO_ERROR;

    if (NULL != ppvOut)
    {
        *ppvOut = NULL;
        if (IID_IUnknown == riid || IID_IShellSettingsFolderExt == riid)
        {
            *ppvOut = (ISettingsFolderExt *)this;
            ((LPUNKNOWN)*ppvOut)->AddRef();
        }
        else
            hResult = E_NOINTERFACE; // Requested interface not supported.
    }
    else
        hResult = E_POINTER; // ppvOut is NULL.

    return hResult;
}


STDMETHODIMP
SettingsFolderExt::EnumTopics(
    PENUM_SETTINGS_TOPICS *ppEnum
    )
{
    HRESULT hResult            = NO_ERROR;
    EnumSettingsTopics *pEnum  = NULL;

    if (NULL != ppEnum)
    {
        PSETTINGS_FOLDER_TOPIC *ppITopics = NULL;
        INT cTopics = 0;
        *ppEnum = NULL;

        try
        {
            //
            // Create an array of topic iface ptrs.
            // Each ptr has a ref count of 1.
            //
            cTopics = CreateTopicObjects(&ppITopics);
            if (NULL != ppITopics)
            {
                //
                // Create and initialize the new enumerator using our temporary list.
                //
                pEnum = new EnumSettingsTopics(ppITopics, cTopics);
                hResult = pEnum->QueryInterface(IID_IEnumSettingsTopics, (LPVOID *)ppEnum);
                //
                // Q.I. should always succeed since we own the enumerator's implementation.
                //
                Assert(SUCCEEDED(hResult));
            }
        }
        catch(OutOfMemory)
        {
            hResult = E_OUTOFMEMORY;
        }

        if (FAILED(hResult))
        {
            DebugMsg(DM_ERROR, TEXT("Failed creating Topic enumerator, hResult = 0x%08X"), hResult);
            
            if (NULL != ppITopics)
            {
                for (INT i = 0; i < cTopics; i++)
                {
                    (*(ppITopics + i))->Release();
                }
            }
            //
            // Need to delete enumerator.
            //
            if (NULL != pEnum)
                delete pEnum;
        }            
        //
        // Delete the array created by CreateTopicObjects().
        // This is deleted if we succeed or fail.  On success, the contents have
        // been transferred to the new enumerator.
        //
        if (NULL != ppITopics)
            delete[] ppITopics;
    }
    else
        hResult = E_POINTER;
        
    return hResult;
}


INT
SettingsFolderExt::RemoveRestrictedItems(
    NEW_TOPIC_DATA **pprgOut,
    NEW_TOPIC_DATA *prgIn,
    INT cIn
    ) throw(OutOfMemory)
{
    Assert(NULL != pprgOut);
    Assert(NULL != prgIn);

    INT cOut = 0;
    NEW_TOPIC_DATA *prgOut = new NEW_TOPIC_DATA[cIn];

    //
    // Create a copy of the input array that includes only those
    // items for which there are no restrictions.
    //
    for (INT i = 0; i < cIn; i++)
    {
        //
        // Don't add the item if it is restricted by system policy.
        //
        if (!SHRestricted(prgIn->restPolicy))
        {
            //
            // Special case for when the UI is started from an 
            // app that has not provided a taskbar properties
            // callback (i.e. rundll32.exe).
            //
            if (!(NULL == PTG.pfnTaskbarPropSheetCallback &&
                  TOPIC_STARTMENU == prgIn->dwCookie))
            {
                *(prgOut + cOut++) = *prgIn;
            }
        }
        prgIn++;
    }
    *pprgOut = prgOut;

    return cOut;
}


//
// ppITopicOut is the address of a pointer to an array of pointers.
// On return, each iface ptr has a ref count of 1.
//
INT
SettingsFolderExt::CreateTopicObjects(
    PSETTINGS_FOLDER_TOPIC **pppITopicOut
    ) throw(OutOfMemory)
{
    SettingsFolderTopic *pTopic       = NULL;
    PSETTINGS_FOLDER_TOPIC *ppITopics = NULL;
    NEW_TOPIC_DATA *prgTopicData      = NULL;

    INT cTopics = RemoveRestrictedItems(&prgTopicData, 
                                        (NEW_TOPIC_DATA *)c_rgDataTopics, 
                                        ARRAYSIZE(c_rgDataTopics));

    if (0 < cTopics && NULL != prgTopicData)
    {
        Assert(NULL != pppITopicOut);

        try
        {
            //
            // Create an array of ISettingsFolderTopic ptrs.
            //
            ppITopics = new PSETTINGS_FOLDER_TOPIC[cTopics];
            ZeroMemory(ppITopics, cTopics * sizeof(ppITopics[0]));

            //
            // Create each topic object and add the iface ptr to the array.
            //
            INT iTopic = 0;
            for (INT i = 0; i < cTopics; i++)
            {
                pTopic = NULL;
                pTopic = new SettingsFolderTopic(prgTopicData + i);
                Assert(iTopic < cTopics);
                HRESULT hResult = pTopic->QueryInterface(IID_ISettingsFolderTopic, 
                                                         (LPVOID *)&ppITopics[iTopic++]);
                //
                // Q.I. should always succeed since we own the Topic implementation.
                //
                Assert(SUCCEEDED(hResult));
            }
        }
        catch(OutOfMemory)
        {
            if (NULL != ppITopics)
            {
                for (INT i = 0; i < cTopics; i++)
                {
                    delete (*(ppITopics + i));
                }
                delete[] ppITopics;
            }
            delete pTopic;
            delete[] prgTopicData;

            throw;
        }
        delete[] prgTopicData;
    }

    *pppITopicOut = ppITopics;
    return cTopics;
}



///////////////////////////////////////////////////////////////////////////////
// EnumSettingsTopics
///////////////////////////////////////////////////////////////////////////////
EnumSettingsTopics::EnumSettingsTopics(
    PSETTINGS_FOLDER_TOPIC *ppITopics,
    UINT cItems
    ) : m_pEnum(NULL)
{
    m_pEnum = new EnumIFacePtrs<PSETTINGS_FOLDER_TOPIC>(ppITopics, cItems);
}


EnumSettingsTopics::EnumSettingsTopics(
    const EnumSettingsTopics& rhs
    ) : m_pEnum(NULL)
{
    m_pEnum = new EnumIFacePtrs<PSETTINGS_FOLDER_TOPIC>(*(rhs.m_pEnum));
}


EnumSettingsTopics::~EnumSettingsTopics(
    VOID
    )
{
    if (NULL != m_pEnum)
        delete m_pEnum;
}

STDMETHODIMP 
EnumSettingsTopics::QueryInterface(
    REFIID riid, 
    LPVOID *ppvOut
    )
{
    HRESULT hResult = NO_ERROR;

    if (NULL != ppvOut)
    {
        *ppvOut = NULL;
        if (IID_IUnknown == riid || IID_IEnumSettingsTopics == riid)
        {
            *ppvOut = (IEnumSettingsTopics *)this;
            ((LPUNKNOWN)*ppvOut)->AddRef();
        }
        else
            hResult = E_NOINTERFACE; // Requested interface not supported.
    }
    else
        hResult = E_POINTER; // ppvOut is NULL.

    return hResult;
}


STDMETHODIMP
EnumSettingsTopics::Clone(
    PENUM_SETTINGS_TOPICS *ppEnum
    )
{
    HRESULT hResult           = NO_ERROR;
    EnumSettingsTopics *pEnum = NULL;

    if (NULL != ppEnum)
    {
        *ppEnum = NULL;

        try
        {        
            pEnum = new EnumSettingsTopics((const EnumSettingsTopics&)*this);
            hResult = pEnum->QueryInterface(IID_IEnumSettingsTopics, (LPVOID *)ppEnum);
            //
            // Q.I. should always succeed since we own the enumerator's implementation.
            //
            Assert(SUCCEEDED(hResult));
        }
        catch(OutOfMemory)
        {
            hResult = E_OUTOFMEMORY;
        }
        catch(...)
        {
            hResult = E_UNEXPECTED;
        }

        if (FAILED(hResult) && NULL != pEnum)
        {
            delete pEnum;
            *ppEnum = NULL;
        }

    }
    else
        hResult = E_INVALIDARG;

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
// SettingsFolderTopic
///////////////////////////////////////////////////////////////////////////////
SettingsFolderTopic::SettingsFolderTopic(
    const NEW_TOPIC_DATA *pTopicData
    ) : m_dwCookie(pTopicData->dwCookie),
        m_pszTitle(NULL),
        m_pszCaption(NULL),
        m_idBitmap(pTopicData->idBitmap)
{
    m_pszTitle   = FmtMsgSprintf(pTopicData->idsTitle);
    m_pszCaption = FmtMsgSprintf(pTopicData->idsCaption);
}

SettingsFolderTopic::~SettingsFolderTopic(
    VOID
    )
{
    //
    // Destroy our text strings.
    //
    LocalFree(m_pszTitle);
    LocalFree(m_pszCaption);
}



STDMETHODIMP 
SettingsFolderTopic::QueryInterface(
    REFIID riid, 
    LPVOID *ppvOut
    )
{
    HRESULT hResult = NO_ERROR;

    if (NULL != ppvOut)
    {
        *ppvOut = NULL;
        if (IID_IUnknown == riid || IID_ISettingsFolderTopic == riid)
        {
            *ppvOut = (ISettingsFolderTopic *)this;
            ((LPUNKNOWN)*ppvOut)->AddRef();
        }
        else
            hResult = E_NOINTERFACE; // Requested interface not supported.
    }
    else
        hResult = E_POINTER; // ppvOut is NULL.

    return hResult;
}


HRESULT
SettingsFolderTopic::GetStringMember(
    LPTSTR pszBuffer,
    UINT *pcchBuffer,
    LPTSTR pszSource
    )
{
    HRESULT hResult = NO_ERROR;
    Assert(NULL != pcchBuffer);

    UINT len = lstrlen(pszSource);
    if (NULL != pszBuffer && len < *pcchBuffer)
    {
        lstrcpy(pszBuffer, pszSource);
    }
    else
    {
        *pcchBuffer = len + 1;
        hResult = E_FAIL;
    }
    return hResult;
}


STDMETHODIMP
SettingsFolderTopic::GetTitle(
    LPTSTR pszBuffer, 
    UINT *pcchBuffer
    )
{
    return GetStringMember(pszBuffer, pcchBuffer, m_pszTitle);
}


STDMETHODIMP
SettingsFolderTopic::GetTitle(
    LPCTSTR& pszBuffer
    )
{
    pszBuffer = m_pszTitle;

    return NO_ERROR;
}


STDMETHODIMP
SettingsFolderTopic::GetCaption(
    LPTSTR pszBuffer, 
    UINT *pcchBuffer
    )
{
    return GetStringMember(pszBuffer, pcchBuffer, m_pszCaption);
}


STDMETHODIMP
SettingsFolderTopic::GetCaption(
    LPCTSTR& pszBuffer
    )
{
    pszBuffer = m_pszCaption;
    return NO_ERROR;
}



STDMETHODIMP_(DWORD)
SettingsFolderTopic::GetCookie(
    VOID
    )
{
    return m_dwCookie;
}

BOOL
SettingsFolderTopic::ShellExec(
    SHELLEXECUTEINFO *psei
    )
{
    psei->fMask |= SEE_MASK_NOCLOSEPROCESS;

    BOOL bResult = ShellExecuteEx(psei);
    
    DWORD dwWaitResult = WaitForInputIdle(psei->hProcess, EXEC_WAIT_TIMEOUT);

#ifdef DEBUG
    //
    // Complain through the debugger if the wait didn't work.
    //
    if (0 != dwWaitResult)
    {
        DebugMsg(DM_ERROR, TEXT("Exec Wait %s"),
                 WAIT_TIMEOUT == dwWaitResult ? TEXT("TIMEOUT") : TEXT("FAILED"));
    }
#endif

    return bResult;
}


BOOL
SettingsFolderTopic::ShellExecFileAndArgs(
    LPCTSTR pszFile,
    LPCTSTR pszArgs
    )
{
    SHELLEXECUTEINFO sei;
    ZeroMemory(&sei, sizeof(sei));
    sei.cbSize       = sizeof(sei);
    sei.hwnd         = NULL;
    sei.lpFile       = pszFile;
    sei.lpParameters = pszArgs ? pszArgs : TEXT("");
    sei.nShow        = SW_SHOWNORMAL;

    return ShellExec(&sei);
}


BOOL
SettingsFolderTopic::RunDll32(
    LPCTSTR pszArgs
    )
{
    return ShellExecFileAndArgs(c_szRunDLL32, 
                                pszArgs);
}


//
// Run a system exe.
//
BOOL
SettingsFolderTopic::RunExe(
    LPCTSTR pszExeName,
    LPCTSTR pszArgs
    )
{
    return ShellExecFileAndArgs(pszExeName, 
                                pszArgs ? pszArgs : TEXT(""));
}


//
// Launch a control panel applet.
//
BOOL
SettingsFolderTopic::RunCPL(
    LPCTSTR pszCPLName,
    LPCTSTR pszArgs
    )
{
    TCHAR szArgs[MAX_PATH];
    wsprintf(szArgs, TEXT("%s,%s %s%s"), c_szShell32_DLL,
                                         c_szControl_RunDLL,
                                         pszCPLName,
                                         pszArgs ? pszArgs : TEXT(""));
    return RunDll32(szArgs);
}


//
// Launch a control panel applet using the shell's help shortcuts.
//
BOOL
SettingsFolderTopic::RunHelpShortcut(
    LPCTSTR pszShortcut,
    LPCTSTR pszArgs
    )
{
    TCHAR szArgs[MAX_PATH];
    wsprintf(szArgs, TEXT("%s,%s %s%s"), c_szShell32_DLL,
                                         c_szHelpShortcuts_RunDLL,
                                         pszShortcut,
                                         pszArgs ? pszArgs : TEXT(""));
    return RunDll32(szArgs);
}


//
// Launch the "Add New Harware" wizard.
//
BOOL
SettingsFolderTopic::RunAddNewHardwareWizard(
    VOID
    )
{
    TCHAR szArgs[MAX_PATH];
    wsprintf(szArgs, TEXT("%s,%s ,1"), c_szSysDm_CPL,
                                       c_szInstallDevice_RunDLL);
    return RunDll32(szArgs);
}


//
// Run a system special folder.  i.e. printers, fonts etc.
// 
// IDFolder - Any CSIDL_XXXXXX constant.  See SHGetSpecialFolderLocation for details.
//
BOOL
SettingsFolderTopic::RunSpecialFolder(
    INT IDFolder
    )
{
    BOOL bResult = FALSE;
    LPITEMIDLIST pidl;
    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, IDFolder, &pidl)))
    {
        SHELLEXECUTEINFO sei;
        ZeroMemory(&sei, sizeof(sei));
        sei.cbSize   = sizeof(sei);
        sei.hwnd     = NULL;
        sei.fMask    = SEE_MASK_IDLIST;
        sei.lpIDList = pidl;
        sei.nShow    = SW_SHOWNORMAL;
        sei.lpParameters = TEXT("/S");  // Same as COF_USEOPENSETTINGS

        bResult = ShellExec(&sei);
        ILFree(pidl);
    }
    return bResult;
}


//
// Launch the shell tray's properties sheet.
//
VOID
SettingsFolderTopic::RunTaskbarProperties(
    VOID
    )
{
    INT nStartPage = 1;

    //
    // If this callback ptr is NULL, this topic shouldn't have
    // been included in the UI.  See SettingsFolder::CreateTopics().
    //
    if (NULL != PTG.pfnTaskbarPropSheetCallback)
    {
        (*PTG.pfnTaskbarPropSheetCallback)(nStartPage);
    }
}


BOOL
SettingsFolderTopic::OpenControlPanel(
    VOID
    )
{
    BOOL bResult = FALSE;

    LPITEMIDLIST pidl = NULL;
    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_CONTROLS, &pidl)))
    {
        SHELLEXECUTEINFO sei;
        ZeroMemory(&sei, sizeof(sei));
        sei.cbSize   = sizeof(sei);
        sei.hwnd     = NULL;
        sei.fMask    = SEE_MASK_IDLIST;
        sei.lpIDList = pidl;
        sei.nShow    = SW_SHOWNORMAL;

        bResult = ShellExec(&sei);
        ILFree(pidl);
    }
    return bResult;
}


//
// Topic was selected.  Do your thing.
//
STDMETHODIMP
SettingsFolderTopic::TopicSelected(
    VOID
    )
{
    HRESULT hResult = NO_ERROR;
    switch(m_dwCookie)
    {
        case TOPIC_PRINTERS:
            RunHelpShortcut(c_szAddPrinter);
            break;

        case TOPIC_STARTMENU:
            RunTaskbarProperties();
            break;

        case TOPIC_WINLOOK:
            RunCPL(c_szCPLDesktop);
            break;

        case TOPIC_PROGRAMS:
            RunCPL(c_szCPLAppWizard);
            break;

        case TOPIC_NETWORK:
            RunExe(c_szInternetCnxWizard);
            break;

        case TOPIC_HARDWARE:
#ifdef WINNT
            RunCPL(c_szCPLNewDevice);
#else
            RunAddNewHardwareWizard();
#endif
            break;

        case TOPIC_ACCESSIBILITY:
            RunCPL(c_szCPLAccessibility);
            break;

        case TOPIC_CPANEL:
            OpenControlPanel();
            break;

        default:
            //
            // Invalid topic cookie.
            //
            Assert(FALSE);
            break;
    }
    return hResult;
}


//
// Draw the topic's bitmap image on the given DC and in the given rect.
//
STDMETHODIMP
SettingsFolderTopic::DrawImage(
    HDC hdc,
    LPRECT prc
    )
{
    Assert(NULL != hdc);
    Assert(NULL != prc);

    HPALETTE hOldPalette[2];
    HBITMAP hOldBitmap;
    RECT rcBitmap;
    INT iStretchModeOld;

    //
    // Get bitmap from cache or load from resource.
    //
    if (NULL == (HBITMAP)m_dib)
        m_dib.Load(g_hInstance, MAKEINTRESOURCE(m_idBitmap));

    m_dib.GetRect(&rcBitmap);

    Assert(NULL != (HPALETTE)m_dib);
    hOldPalette[0] = SelectPalette(hdc, (HPALETTE)m_dib, TRUE);

    RealizePalette(hdc);

    hOldPalette[1] = SelectPalette(PTG.dcMem, (HPALETTE)m_dib, TRUE);
    RealizePalette(PTG.dcMem);

    Assert(NULL != (HBITMAP)m_dib);
    hOldBitmap = (HBITMAP)SelectObject(PTG.dcMem, (HBITMAP)m_dib);

    iStretchModeOld = SetStretchBltMode(hdc, COLORONCOLOR);

    StretchBlt(hdc,
               prc->left,
               prc->top,
               prc->right - prc->left,
               prc->bottom - prc->top,
               PTG.dcMem,
               0, 
               0, 
               rcBitmap.right,
               rcBitmap.bottom,
               SRCCOPY);
               
    SelectPalette(hdc, hOldPalette[0], TRUE);
    RealizePalette(hdc);

    SetStretchBltMode(hdc, iStretchModeOld);
    SelectPalette(PTG.dcMem, hOldPalette[1], TRUE);
    SelectObject(PTG.dcMem, hOldBitmap);

    return NO_ERROR;
}


//
// hwnd - Settings Folder's main view window.
//
// Returns: S_OK    - System palette changed as a result of the realization.
//          S_FALSE - System palette did not change.
//
STDMETHODIMP
SettingsFolderTopic::PaletteChanged(
    HWND hwnd
    )
{
    HRESULT hResult = S_FALSE;

    //
    // If the image hasn't been displayed yet, the CDIB object won't have
    // an HPALETTE yet.
    //
    if (NULL != (HPALETTE)m_dib)
    {
        CDC dc(hwnd);
        HPALETTE hOldPal = SelectPalette(dc, (HPALETTE)m_dib, TRUE);

        if (RealizePalette(dc))
            hResult = S_OK;        // System palette changed.

        SelectPalette(dc, hOldPal, TRUE);
        RealizePalette(dc);
    }

    return hResult;
}


STDMETHODIMP
SettingsFolderTopic::DisplayChanged(
    VOID
    )
{
    m_dib.Load(g_hInstance, MAKEINTRESOURCE(m_idBitmap));
    return NO_ERROR;
}
