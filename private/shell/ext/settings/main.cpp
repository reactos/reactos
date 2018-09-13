#include "precomp.hxx"
#pragma hdrstop

#include "gdiobj.h"
#include "resource.h"
#include "folder.h"


//
// Global constants -----------------------------------------------------------
//
//
// Width of the image graphic's shadow in dialog units.
//
const INT IMAGE_SHADOW_WIDTH = 2;
//
// Vertical space between topic menu items in dialog units.
//
const INT INTER_TOPIC_GAP = 4;
//
// Banner title offset from left edge of banner.
//
const INT BANNER_TITLE_INDENT = 14;
//
// Banner title font size in points.
//
const INT BANNER_FONT_SIZE = 22;
//
// Face name of banner title font.
//
const TCHAR c_szBannerFontFace[] = TEXT("Times New Roman");
//
// Default mouse hover time (400 ms) is too slow.
// Speed it up.
//
const DWORD MOUSE_HOVER_TIME = 100;
//
// Name of the system-wide named mutex for this app.
//
const TCHAR c_szAppMutex[] = TEXT("Settings$Dll$Mutex");
//
// Enumeration for "SPOTSTATE" argument to function DrawTopicSpot().
//
const enum SPOTSTATE { SPOT_NORMAL = 0, SPOT_FOCUS };
//
// Enumeration for eAppearance argument to function GetDialogFont().
//
const enum GDF_APPEARANCE { GDF_NORMAL = 0, GDF_HIGHLIGHT, GDF_TITLE };
//
// Topic spot bitmap transparency color.
//
const COLORREF COLOR_TOPICSPOT_MASK = RGB(0, 128, 128);

//
// Module function declarations -----------------------------------------------
//
INT_PTR CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);
LRESULT OnNotify(HWND hwnd, WPARAM wParam, LPARAM lParam);
LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
LRESULT OnDestroy(HWND hwnd);
LRESULT OnClose(HWND hwnd);
LRESULT OnDrawItem(HWND hwnd, UINT idCtl, LPDRAWITEMSTRUCT pdis);
LRESULT OnMeasureItem(HWND hwnd, UINT idCtl, LPMEASUREITEMSTRUCT pmis);
LRESULT OnPaletteChanged(HWND hwnd, WPARAM wParam, LPARAM lParam);
LRESULT OnQueryNewPalette(HWND hwnd, WPARAM wParam, LPARAM lParam);
LRESULT OnSetCursor(HWND hwnd, WPARAM wParam, LPARAM lParam);
LRESULT OnSystemSettingChange(HWND hwnd, WPARAM wParam, LPARAM lParam);
LRESULT OnDisplayChange(VOID);

INT InitializeAndRunApplication(HWND hwndParent);
BOOL InitializeApplication(VOID);
BOOL ActivateExistingInstance(LPHANDLE phMutex);
BOOL InitializeListView(HWND hwndListview);
INT  RunApplication(HWND hwndParent);
BOOL LoadTopicsIntoList(HWND hwndListview);
LONG CalcListviewItemHeight(HWND hwndDlg);
VOID SetListviewClickMode(VOID);
VOID DrawListviewItem(HWND hwnd,UINT idCtl,LPDRAWITEMSTRUCT pdis);
VOID DrawTopicSpot(HDC hdc, LPRECT prc, SPOTSTATE spot);
VOID DrawTopicImage(HWND hwnd, HDC hdc, PSETTINGS_FOLDER_TOPIC pITopic);
VOID DrawBanner(HWND hwnd, HDC hdc);
VOID DrawBannerTitle(HWND hwnd, HDC hdc);
BOOL NotifyTopicsOfPaletteChange(HWND hwnd);
VOID GetDialogFont(CFont& font, HWND hwndDlg, GDF_APPEARANCE eAppearance);
VOID FocusOnListview(VOID);
INT ListViewItemHit(INT xPos, INT yPos);
INT ListViewItemHit(VOID);

BOOL CreatePerThreadStorage(VOID);
VOID DestroyPerThreadStorage(VOID);
HRESULT OnProcessAttach(HINSTANCE hInstDll);
HRESULT OnProcessDetach(VOID);
VOID WINAPI SETTINGS_RUNDLLA(HWND hwnd, HINSTANCE hInstance, LPSTR pszCmdLineA, INT nCmdShow);
VOID WINAPI SETTINGS_RUNDLLW(HWND hwnd, HINSTANCE hInstance, LPWSTR pszCmdLineW, INT nCmdShow);
VOID WINAPI settings_rundllA(HWND hwnd, HINSTANCE hInstance, LPSTR pszCmdLineA,  INT nCmdShow);
VOID WINAPI settings_rundllW(HWND hwnd, HINSTANCE hInstance, LPWSTR pszCmdLineW, INT nCmdShow);
VOID WINAPI OpenSettingsUI(PTRAYPROPSHEETCALLBACK pfnCallback);


//
// Inline functions -----------------------------------------------------------
//
//
//
// Simple MIN/MAX template functions.
//
template <class T>
inline MAX(T a, T b)
    { return a > b ? a : b; }

template <class T>
inline MIN(T a, T b)
    { return a < b ? a : b; }

//
// Dialog Unit <--> Pixel conversion  (X and Y units).
//
inline INT PIXELSX(INT DlgUnits)
    { return (PTG.DialogBaseUnitsX * DlgUnits) / 4; }

inline INT PIXELSY(INT DlgUnits)
    { return (PTG.DialogBaseUnitsY * DlgUnits) / 8; }

inline INT DLGUNITSX(INT pixels)
    { return (pixels * 4) / PTG.DialogBaseUnitsX; }

inline INT DLGUNITSY(INT pixels)
    { return (pixels * 8) / PTG.DialogBaseUnitsY; }

//
// Left margin of topic list makes room for topic spot bitmaps.
// Right margin makes room for a vertical scroll bar if localization
// extends any of the topic titles beyond a single line.
//
inline INT TOPIC_LIST_LMARGIN(VOID)
    { return PTG.cxSmallIcon * 3 / 2; }

inline INT TOPIC_LIST_RMARGIN(VOID)
    { return PTG.cxVertScrollBar; }

//
// Convert a font point size to a value suitable for use in
// the LOGFONT structure member lfHeight.
//
inline INT FONTPTS_TO_LFHEIGHT(HDC hdc, INT pts)
    { return -MulDiv(pts, GetDeviceCaps(hdc, LOGPIXELSY), 72); }



BOOL WINAPI
DllMain(
    HINSTANCE hInstDll,
    DWORD fdwReason,
    LPVOID lpvReserved
    )
{
    BOOL bResult = TRUE;

    switch(fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            bResult = SUCCEEDED(OnProcessAttach(hInstDll));
            break;

        case DLL_PROCESS_DETACH:
            bResult = SUCCEEDED(OnProcessDetach());
            break;
    }

    return bResult;
}


HRESULT
OnProcessAttach(
    HINSTANCE hInstDll
    )
{
    HRESULT hResult = NO_ERROR;

#ifdef DEBUG
    //
    // Default is DM_NONE.
    //
    SetDebugMask(DM_ASSERT | DM_ERROR);
#endif

    g_hInstance = hInstDll;

    DisableThreadLibraryCalls(g_hInstance);

    //
    // Allocate storage for per-thread global data.
    //
    g_dwTlsIndex = TlsAlloc();
    if (TLS_OUT_OF_INDEXES == g_dwTlsIndex)
    {
        hResult = E_FAIL;
    }

    return hResult;
}



HRESULT
OnProcessDetach(
    VOID
    )
{
    if (TLS_OUT_OF_INDEXES != g_dwTlsIndex)
    {
        TlsFree(g_dwTlsIndex);
        g_dwTlsIndex = TLS_OUT_OF_INDEXES;
    }

    #ifdef DEBUG_ALLOC
        //
        // If the DM_ALLOCSUM flag is set, dump out the final statistics for the
        // allocator.  If any blocks are still allocated, this will list them.
        //
        if (DM_ALLOCSUM & GetDebugMask())
        {
            DebugMsg(DM_ALLOCSUM, TEXT("-----------------------------------------"));
            DebugMsg(DM_ALLOCSUM, TEXT("ALLOC - Heap Allocation Summary."));
            DebugMsg(DM_ALLOCSUM, TEXT("-----------------------------------------"));
            g_FreeStore.DumpStatistics();
        }
#endif

    return NO_ERROR;
}


//
// BUGBUG:  I should probably eliminate the Tls and just create a 
//          "MainWindow" class.
//
BOOL
CreatePerThreadStorage(
    VOID
    )
{
    BOOL bResult = FALSE;

    if (TLS_OUT_OF_INDEXES != g_dwTlsIndex)
    {
        if (NULL == TlsGetValue(g_dwTlsIndex))
        {
            try
            {
                PerThreadGlobals *pPTG = new PerThreadGlobals;
                if (TlsSetValue(g_dwTlsIndex, (LPVOID)pPTG))
                {
                    bResult = TRUE;
                }
                else
                {
                    //
                    // Created data block but failed to store in Tls.
                    // Don't know why this might happen but just in case...
                    //
                    delete pPTG;
                }
            }
            catch(...)
            {
                //
                // Catch any exception.  bResult remains FALSE so
                // we return "failure".
                //
            }
        }
        else
        {
            //
            // A process' primary thread allocates per-thread storage
            // in OnProcessAttach.  So... It's already been created.
            //
            bResult = TRUE;
        }
    }
    return bResult;
}

VOID
DestroyPerThreadStorage(
    VOID
    )
{
    if (TLS_OUT_OF_INDEXES != g_dwTlsIndex)
    {
        //
        // Free up the per-thread-data.
        //
        PerThreadGlobals *pPTG = (PerThreadGlobals *)TlsGetValue(g_dwTlsIndex);
        delete pPTG;
        TlsSetValue(g_dwTlsIndex, NULL);
    }
}



VOID WINAPI
SETTINGS_RUNDLLA(
    HWND hwnd,
    HINSTANCE hInstance,
    LPSTR pszCmdLineA,
    INT nCmdShow
    )
{
    if (NULL != pszCmdLineA)
    {
        LPWSTR pszCmdLineW = NULL;
        INT cchCmdLine = MultiByteToWideChar(CP_ACP,
                                             0,
                                             pszCmdLineA,
                                             -1,
                                             NULL,
                                             0);
        pszCmdLineW = new WCHAR[cchCmdLine];
        if (NULL != pszCmdLineW)
        {
            MultiByteToWideChar(CP_ACP,
                                0,
                                pszCmdLineA,
                                -1,
                                pszCmdLineW,
                                cchCmdLine);

            SETTINGS_RUNDLLW(hwnd, hInstance, pszCmdLineW, nCmdShow);

            delete[] pszCmdLineW;
        }
        else
        {
            //
            // Insufficient memory to convert ANSI arg string to UNICODE.
            // No use continuing.
            //
            SettingsMsgBox(
                hwnd,
                MSG_OUTOFMEMORY,
                MSG_MAINWINDOW_TITLE,
                MB_ICONSTOP | MB_OK);
        }
    }
}


VOID WINAPI
SETTINGS_RUNDLLW(
    HWND hwnd,
    HINSTANCE hInstance,
    LPWSTR pszCmdLineW,
    INT nCmdShow
    )
{
    CreatePerThreadStorage();
    InitializeAndRunApplication(hwnd);
    DestroyPerThreadStorage();
}


//
// Lower case versions of entry rundll32.exe entry points.
//
VOID WINAPI
settings_rundllA(
    HWND hwnd,
    HINSTANCE hInstance,
    LPSTR pszCmdLineA,
    INT nCmdShow
    )
{
    SETTINGS_RUNDLLA(hwnd, hInstance, pszCmdLineA, nCmdShow);
}

VOID WINAPI
settings_rundllW(
    HWND hwnd,
    HINSTANCE hInstance,
    LPWSTR pszCmdLineW,
    INT nCmdShow
    )
{
    SETTINGS_RUNDLLW(hwnd, hInstance, pszCmdLineW, nCmdShow);
}


//
// Exported entry point for clients like explorer.
// BUGBUG: This API should include a parent window handle.
//         Only use the desktop window as a default.
//
VOID WINAPI
OpenSettingsUI(
    PTRAYPROPSHEETCALLBACK pfnCallback
    )
{
    CreatePerThreadStorage();
    PTG.pfnTaskbarPropSheetCallback = pfnCallback;
    InitializeAndRunApplication(GetDesktopWindow());
    DestroyPerThreadStorage();
}


INT
InitializeAndRunApplication(
    HWND hwndParent
    )
{
    INT iReturn = -1;
    HANDLE hMutexApp = NULL;

    try
    {
        if (!SHRestricted(REST_NOSETTINGSASSIST))
        {
            if (!ActivateExistingInstance(&hMutexApp))
            {
                //
                // Perform initialization of global stuff.
                //
                if (InitializeApplication())
                {
                    //
                    // Run the app.
                    //
                    iReturn = RunApplication(hwndParent);
                }

                if (0 != iReturn)
                {
                    //
                    // Something failed during initialization or trying
                    // to run the app.  There's nothing here that should be a
                    // run-time error that is not a programming or out-of-memory
                    // error.  It's sufficient to just say "failed initialization".
                    //
                    SettingsMsgBox(
                        hwndParent,
                        MSG_ERROR_INITIALIZATION,
                        MSG_MAINWINDOW_TITLE,
                        MB_ICONSTOP | MB_OK);
                }
            }
        }
        else
        {
            SettingsMsgBox(
                hwndParent,
                MSG_RESTRICTED_NOSETTINGSASSIST,
                MSG_MAINWINDOW_TITLE,
                MB_ICONSTOP | MB_OK);
        }
    }
    catch(OutOfMemory)
    {
        SettingsMsgBox(
            hwndParent,
            MSG_OUTOFMEMORY,
            MSG_MAINWINDOW_TITLE,
            MB_ICONSTOP | MB_OK);
    }
    catch(...)
    {
        //
        // Some unknown exception caught.
        //
        SettingsMsgBox(
            hwndParent,
            MSG_ERROR_UNKNOWN_ABORT,
            MSG_MAINWINDOW_TITLE,
            MB_ICONSTOP | MB_OK);
    }

    if (NULL != hMutexApp)
        CloseHandle(hMutexApp);

    return iReturn;
}


BOOL
ActivateExistingInstance(
    LPHANDLE phMutex
    )
{
    Assert(NULL != phMutex);

    BOOL bResult = FALSE;
    //
    // We use a named mutex to determine if there's an existing
    // instance of this exe already running.
    // If we create the mutex and GetLastError() returns
    // ERROR_ALREADY_EXISTS, that means that there's an existing
    // instance.  Otherwise, there isn't.
    // If there is an existing instance, we find it's window and
    // promote it to the foreground.
    //
    try
    {
        *phMutex = CreateMutex(NULL, FALSE, c_szAppMutex);

        if (NULL != *phMutex &&
            ERROR_ALREADY_EXISTS == GetLastError())
        {
            LPTSTR pszAppWndTitle = FmtMsgSprintf(MSG_MAINWINDOW_TITLE);

            if (NULL != pszAppWndTitle)
            {
                HWND hwndApp = FindWindowEx(NULL,
                                            NULL,
                                            WC_DIALOG,
                                            pszAppWndTitle);
                if (NULL != hwndApp)
                {
                    //
                    // Bring an existing app to the foreground.
                    //
                    ShowWindow(hwndApp, SW_SHOWNORMAL);
                    bResult = SetForegroundWindow(hwndApp);
                }
                LocalFree(pszAppWndTitle);
            }
        }
    }
    catch(...)
    {
        //
        // If we catch an exception, we want the caller to think
        // that an existing instance was not found.  That way,
        // they'll open up a new settings UI window.  This ensures that for
        // whatever reason, the user gets a settings UI.
        //
        bResult = FALSE;
    }

    return bResult;
}


BOOL
InitializeApplication(
    VOID
    ) throw(OutOfMemory)
{
    INITCOMMONCONTROLSEX iccex;

    Assert(NULL != g_hInstance);

    //
    // Init common controls.
    //
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&iccex);

    //
    // Initialize the folder object and set the "current" topic.
    //
    if (PTG.Folder.Initialize() && 0 < PTG.Folder.GetTopicCount())
    {
        PTG.pICurrentTopic = PTG.Folder.GetTopic(0);
        //
        // No need to keep additional ref counts on topic objects.
        // Lifetime scope of the global folder object ensures correct
        // existance of topic objects.
        //
        if (NULL != PTG.pICurrentTopic)
            PTG.pICurrentTopic->Release();
    }

    return NULL != PTG.pICurrentTopic;
}



INT
RunApplication(
    HWND hwndParent
    )
{
    HWND hwndDialog;
    INT iReturn = -1;  // Returns -1 on failure.  0 on success.

    Assert(NULL != g_hInstance);

    hwndDialog = CreateDialog(
        g_hInstance,
        MAKEINTRESOURCE(IDD_SETTINGS),
        hwndParent,
        DlgProc);

    if (NULL != hwndDialog)
    {
        //
        // This isn't very polite.  I need to change the 
        // exported interface to include a parent window instead of 
        // using the desktop window.  Until I'm allowed to make changes
        // to explorer.exe again (Memphis Beta 1 freeze in effect), this 
        // will have to do.
        //
        SetForegroundWindow(hwndDialog);

        ShowWindow(hwndDialog, SW_SHOWNORMAL);
        UpdateWindow(hwndDialog);

        MSG msg;
        HACCEL haccel = PTG.haccelKeyboard;

        while(TRUE == GetMessage(&msg, NULL, 0, 0))
        {
            if (NULL == hwndDialog || !IsDialogMessage(hwndDialog, &msg))
            {
                if (NULL == haccel ||
                   !TranslateAccelerator(hwndDialog,
                                         haccel,
                                         &msg))
                {
                    {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }
            }
        }
        iReturn = (int)msg.wParam;
    }
    return iReturn;
}


INT_PTR CALLBACK
DlgProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
   )
{
    LRESULT lResult = 1;
    try
    {
        switch(uMsg)
        {
            case WM_INITDIALOG:
                lResult = OnInitDialog(hwnd, wParam, lParam);
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, lResult);
                break;

            case WM_NOTIFY:
                lResult = OnNotify(hwnd, wParam, lParam);
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, lResult);
                break;

            case WM_COMMAND:
                lResult = OnCommand(hwnd, wParam, lParam);
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, lResult);
                break;

            case WM_CLOSE:
                lResult = OnClose(hwnd);
                break;

            case WM_ENDSESSION:
            case WM_DESTROY:
                lResult = OnDestroy(hwnd);
                break;

            case WM_DRAWITEM:
                lResult = OnDrawItem(hwnd, (UINT)wParam, (LPDRAWITEMSTRUCT)lParam);
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, lResult);
                break;

            case WM_MEASUREITEM:
                lResult = OnMeasureItem(hwnd, (UINT)wParam, (LPMEASUREITEMSTRUCT)lParam);
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, lResult);
                break;

            case WM_SETCURSOR:
                lResult = OnSetCursor(hwnd, wParam, lParam);
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, lResult);
                break;
        
            case WM_QUERYNEWPALETTE:
                lResult = OnQueryNewPalette(hwnd, wParam, lParam);
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, lResult);
                break;

            case WM_SYSCOLORCHANGE:
                PTG.OnSysColorChange();
                break;

            case WM_DISPLAYCHANGE:
                OnDisplayChange();
                break;

            case WM_PALETTECHANGED:
                lResult = OnPaletteChanged(hwnd, wParam, lParam);
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, lResult);
                break;

            case WM_SETTINGCHANGE:
                lResult = OnSystemSettingChange(hwnd, wParam, lParam);
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, lResult);
                break;

            default:
                lResult = 0;
                break;
        }
    }
    catch(OutOfMemory)
    {
        SettingsMsgBox(
            hwnd,
            MSG_OUTOFMEMORY,
            MSG_MAINWINDOW_TITLE,
            MB_ICONSTOP | MB_OK);

        DestroyWindow(hwnd);
        lResult = 0;
    }
    catch(...)
    {
        //
        // Some unknown exception caught.
        //
        SettingsMsgBox(
            hwnd,
            MSG_ERROR_UNKNOWN,
            MSG_MAINWINDOW_TITLE,
            MB_ICONSTOP | MB_OK);

        DestroyWindow(hwnd);
        lResult = 0;
    }

    return lResult;
}


//
// Fill the topic title's list with title strings.
// The lParam of each listview item contains the address of
// a SETTINGS_FOLDER_TOPIC object.
//
BOOL
LoadTopicsIntoList(
    HWND hwndList
    )
{
    Assert(NULL != hwndList);

    INT cTopics = PTG.Folder.GetTopicCount();
    LV_ITEM item;

    item.mask       = LVIF_PARAM;
    item.iSubItem   = 0;

    for (INT i = 0; i < cTopics; i++)
    {
        PSETTINGS_FOLDER_TOPIC pITopic = PTG.Folder.GetTopic(i);

        if (NULL != pITopic)
        {
            item.iItem  = i;
            item.lParam = (LPARAM)pITopic;
            ListView_InsertItem(hwndList, &item);
            //
            // The folder object holds a reference to each topic object.
            // Since the folder's lifetime encloses the lifetime of the
            // listview, there is no need for the listview to maintain a
            // separate reference count on the topic objects.
            //
            pITopic->Release();
        }
    }

    //
    // Success only if all topics were added.
    //
    return ListView_GetItemCount(hwndList) == cTopics;
}



VOID
GetDialogFont(
    CFont& font,
    HWND hwndDlg,
    GDF_APPEARANCE eAppearance
    )
{
    Assert(NULL != hwndDlg);

    LOGFONT lf;
    HFONT hfont;

    hfont = (HFONT)SendMessage(GetDlgItem(hwndDlg, IDC_TXT_IMAGE), WM_GETFONT, 0, 0);
    GetObject(hfont, sizeof(lf), &lf);

    switch(eAppearance)
    {
        case GDF_HIGHLIGHT:
            lf.lfUnderline = TRUE;
            break;

        case GDF_TITLE:
            {
                //
                // Override the dialog font with a serifed font for
                // the title.  It looks better.
                //
                CDC dc(hwndDlg);
                ZeroMemory(&lf, sizeof(lf));
                lstrcpyn(lf.lfFaceName, c_szBannerFontFace, LF_FACESIZE);
                lf.lfWeight = FW_BOLD;
                lf.lfHeight = FONTPTS_TO_LFHEIGHT(dc, BANNER_FONT_SIZE);
            }
            break;

        default:
            //
            // Use the font obtained from the dialog.
            //
            break;
    }

    font = lf;
}



BOOL
InitializeListView(
    HWND hwndList
    )
{
    Assert(NULL != hwndList);

    BOOL bResult = FALSE;
    LV_COLUMN lvc;
    RECT rc;

    GetClientRect(hwndList, &rc);

    //
    // Add our single column.
    // Note that we bring in the right margin to allow for a vertical
    // scroll bar.  If topic title lengths are kept to one line, no
    // scroll bar is needed nor will one appear.  However, if some
    // operation like localization increases any of the topic titles
    // to more than one line, all of the items will grow in height,
    // requiring the automatic addition of the scroll bar.  By bringing
    // in the right margin, we prevent the automatic addition of the
    // horizontal scroll bar.
    //
    lvc.mask     = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM;
    lvc.fmt      = LVCFMT_LEFT;
    lvc.cx       = rc.right - TOPIC_LIST_RMARGIN();
    lvc.iSubItem = 0;
    if (-1 != ListView_InsertColumn(hwndList, 0, &lvc))
    {
        //
        // Make listview transparent so any background watermark
        // shows through.
        //
        ListView_SetBkColor(hwndList, CLR_NONE);

        //
        // Enable mouse tracking (hover select).
        //
        SendMessage(hwndList, LVM_SETEXTENDEDLISTVIEWSTYLE,
                              LVS_EX_TRACKSELECT | LVS_EX_ONECLICKACTIVATE,
                              LVS_EX_TRACKSELECT | LVS_EX_ONECLICKACTIVATE);
        //
        // Set the listview's selection mode (single vs. double click)
        // based on the user's preference as set in the shell.
        //
        SetListviewClickMode();

        //
        // Set a custom hover time.  Default is too slow.
        //
        ListView_SetHoverTime(hwndList, MOUSE_HOVER_TIME);

        bResult = TRUE;
    }
    return bResult;
};


LRESULT
OnInitDialog(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam
    )
{
    Assert(NULL != hwnd);

    LRESULT lResult = 0;

    try
    {
        //
        // Save hwnd for topic list.  We'll be using it a lot.
        //
        PTG.hwndTopicList = GetDlgItem(hwnd, IDC_LIST_TOPICS);

        //
        // The control IDC_BMP_IMAGE is provided in the dialog template
        // as a place to draw the topic image and it's shadow border.
        // Convert it to OWNERDRAW so we control the painting.
        //
        HWND hwndImage = GetDlgItem(hwnd, IDC_BMP_IMAGE);
        LONG lStyle;
        lStyle = GetWindowLong(hwndImage, GWL_STYLE);
        lStyle &= ~SS_BITMAP;
        lStyle |= SS_OWNERDRAW;
        SetWindowLong(hwndImage, GWL_STYLE, lStyle);

        //
        // The control IDC_BMP_BANNER is provided in the dialog template
        // as a place to draw the banner watermark.
        // Convert it to OWNERDRAW so we control the painting.
        //
        HWND hwndBanner = GetDlgItem(hwnd, IDC_BMP_BANNER);
        lStyle = GetWindowLong(hwndBanner, GWL_STYLE);
        lStyle &= ~SS_BITMAP;
        lStyle |= SS_OWNERDRAW;
        SetWindowLong(hwndBanner, GWL_STYLE, lStyle);

        //
        // Text fonts.
        // Use the dialog's font defined in the dialog template.
        //
        GetDialogFont(PTG.hfontTextNormal, hwnd, GDF_NORMAL);
        GetDialogFont(PTG.hfontTextHighlight, hwnd, GDF_HIGHLIGHT);
        GetDialogFont(PTG.hfontBanner, hwnd, GDF_TITLE);

        //
        // Give the window a title.
        // Use the string from MSG_MAINWINDOW_TITLE so we are assured
        // that it is the same string used to find an existing window
        // when we first launch the app (prevent multiple instances).
        //
        LPTSTR pszTitle = FmtMsgSprintf(MSG_MAINWINDOW_TITLE);
        SetWindowText(hwnd, pszTitle);
        LocalFree(pszTitle);

        //
        // Set the window's icon.  Can't set it for the class because
        // we're using a dialog as the main window.
        //
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)PTG.hiconApplicationSm);
        SendMessage(hwnd, WM_SETICON, ICON_BIG,   (LPARAM)PTG.hiconApplicationLg);

        //
        // Set some of our custom styles for the listview and
        // load the topic objects into the list.
        //
        if (InitializeListView(PTG.hwndTopicList) &&
            LoadTopicsIntoList(PTG.hwndTopicList))
        {
            //
            // Set the focus to the listview.
            //
            FocusOnListview();
            lResult = 1;
        }
        else
        {
            SettingsMsgBox(NULL,
                           MSG_ERROR_INITIALIZATION,
                           MSG_MAINWINDOW_TITLE,
                           MB_ICONSTOP | MB_OK);
        }
    }
    //
    // Catch any exceptions that occur during the dialog
    // initialization process.  These must be handled differently
    // than those caught in DlgProc.  If an exception occurs here,
    // we assume the dialog is incomplete so we destroy it.
    //
    catch(OutOfMemory)
    {
        SettingsMsgBox(NULL,
                       MSG_OUTOFMEMORY,
                       MSG_MAINWINDOW_TITLE,
                       MB_ICONSTOP | MB_OK);
    }
    catch(...)
    {
        SettingsMsgBox(NULL,
                       MSG_ERROR_UNKNOWN,
                       MSG_MAINWINDOW_TITLE,
                       MB_ICONSTOP | MB_OK);
    }
    if (1 != lResult)
    {
        DestroyWindow(hwnd);
    }

    return lResult;
}


LRESULT
OnDestroy(
    HWND hwnd
    )
{
    Assert(NULL != hwnd);

    //
    // Empty out the listview object.
    // Remember, the listview doesn't hold a reference count for the
    // topic objects.  Don't need to call Release() on the topics.
    // Topic objects are released when the Folder object is destroyed.
    //
    ListView_DeleteAllItems(PTG.hwndTopicList);

    PostQuitMessage(0);
    return 0;
}


LRESULT
OnClose(
    HWND hwnd
    )
{
    Assert(NULL != hwnd);

    DestroyWindow(hwnd);
    return 0;
}


LRESULT
OnCommand(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam
    )
{
    Assert(NULL != hwnd);

    LRESULT lResult  = 1;

    switch(LOWORD(wParam))
    {
        case IDM_CLOSE:
        case IDM_ESCAPE:
        case IDC_BTN_CLOSE:
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            break;

        case IDC_LIST_TOPICS:
            PTG.bWaitCursor = TRUE;            
            SendMessage(hwnd, WM_SETCURSOR, 0, 0);
            PTG.pICurrentTopic->TopicSelected();
            PTG.bWaitCursor = FALSE;
            break;

        default:
            lResult = 0;
            break;
    }
    return lResult;
}


LRESULT
OnDisplayChange(
    VOID
    )
{
    INT cTopics = PTG.Folder.GetTopicCount();

    //
    // Re-load the header and topic spot bitmaps.
    //
    PTG.OnDisplayChange();

    //
    // Tell each topic to re-load it's bitmap.
    //
    for (INT i = 0; i < cTopics; i++)
    {
        PSETTINGS_FOLDER_TOPIC pITopic = PTG.Folder.GetTopic(i);

        if (NULL != pITopic)
        {
            pITopic->DisplayChanged();
            pITopic->Release();
        }
    }
    return 1;
}
        

LRESULT
OnSystemSettingChange(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam
    )
{
    SetListviewClickMode();
    return 0;
}
 


LRESULT
OnMeasureItem(
    HWND hwnd,
    UINT idCtl,
    LPMEASUREITEMSTRUCT pmis
    )
{
    Assert(NULL != hwnd);
    Assert(NULL != pmis);

    if (ODT_LISTVIEW == pmis->CtlType)
    {
        //
        // Height of each item in the listview is set to
        // contain the longest topic title accounting for
        // text wrap.
        //
        pmis->itemHeight = CalcListviewItemHeight(hwnd);
    }
    return 1;
}


//
// Calculate the maximum height that will be required to display
// a topic title in the list.  This is called in response to the
// WM_MEASUREITEM that is received when the listview object
// needs to determine how tall each item is.
//
LONG
CalcListviewItemHeight(
    HWND hwndDlg
    )
{
    Assert(NULL != hwndDlg);

    INT cTopics = PTG.Folder.GetTopicCount();
    LONG cyMax  = 0;
    CDC dc(hwndDlg);
    RECT rcListView, rc;

    //
    // Need to select the dialog's font into the DC so that we get an
    // accurate calculation of the required rectangle height for the
    // listview items.
    //
    HFONT hfont = (HFONT)SendMessage(hwndDlg, WM_GETFONT, 0, 0);
    HFONT hfontOld = (HFONT)SelectObject(dc, hfont);
    //
    // Can't use PTG.hwndTopicList yet because WM_INITDIALOG hasn't
    // been processed.
    //
    GetWindowRect(GetDlgItem(hwndDlg, IDC_LIST_TOPICS), &rcListView);

    for (INT i = 0; i < cTopics; i++)
    {
        PSETTINGS_FOLDER_TOPIC pITopic = PTG.Folder.GetTopic(i);

        if (NULL != pITopic)
        {
            LPCTSTR pszTitle = NULL;
            pITopic->GetTitle(pszTitle);

            Assert(NULL != pszTitle);

            rc        = rcListView;
            rc.left  += TOPIC_LIST_LMARGIN();
            rc.right -= TOPIC_LIST_RMARGIN();
            rc.bottom = rc.top + 1;            // DrawText will extend this.

            DrawText(dc,
                     pszTitle,
                     lstrlen(pszTitle),
                     &rc,
                     DT_WORDBREAK | DT_CALCRECT);

            cyMax = MAX(cyMax, rc.bottom - rc.top);
            pITopic->Release();
        }
    }

    SelectObject(dc, hfontOld);

    //
    // Without INTER_TOPIC_GAP, single-line list items are too scrunched
    // together vertically.  Doesn't look good for this type of stylized UI.
    //
    return cyMax + PIXELSY(INTER_TOPIC_GAP);
}


VOID 
SetListviewClickMode(
    VOID
    )
{
    SHELLSTATE ss;
    DWORD dwStyle;

    //
    // Get the current double-click setting in the shell (user pref).
    //
    SHGetSetSettings(&ss, SSF_WIN95CLASSIC | SSF_WEBVIEW | SSF_DOUBLECLICKINWEBVIEW, FALSE);

    //
    // Get the current listview style bits.
    //
    dwStyle = ListView_GetExtendedListViewStyle(PTG.hwndTopicList);

    //
    // We get single-click if user wants no web view or single-click in web view.
    // SINGLECLICK = WEBVIEW && !DBLCLICKINWEBVIEW
    //
    PTG.bListviewSingleClick = !ss.fWin95Classic && ss.fWebView && !ss.fDoubleClickInWebView;

    if (PTG.bListviewSingleClick)
    {
        if (0 == (dwStyle & LVS_EX_ONECLICKACTIVATE))
        {
            //
            // User wants single click but list is double click..
            // Set listview to single-click mode.
            //
            SendMessage(PTG.hwndTopicList, 
                        LVM_SETEXTENDEDLISTVIEWSTYLE,
                        LVS_EX_ONECLICKACTIVATE,
                        LVS_EX_ONECLICKACTIVATE);
        }
    }
    else if (dwStyle & LVS_EX_ONECLICKACTIVATE)
    {
        //
        // User wants double click but list is single click.
        // Set listview to double click mode.
        //
        SendMessage(PTG.hwndTopicList, 
                    LVM_SETEXTENDEDLISTVIEWSTYLE,
                    LVS_EX_ONECLICKACTIVATE,
                    0);
    }
}



VOID
FocusOnListview(
    VOID
    )
{
    INT iFocus = ListView_GetNextItem(PTG.hwndTopicList, -1, LVNI_FOCUSED);

    if (-1 == iFocus)
        iFocus = 0;

    ListView_SetItemState(PTG.hwndTopicList, iFocus, LVIS_FOCUSED | LVIS_SELECTED,
                                                     LVIS_FOCUSED | LVIS_SELECTED);
}

//
// Draw the "spot" bitmap to the left of the topic title.
// The bitmaps use the color RGB(0, 128, 128) to indicate transparency.
// That's sort of a dull teal green.
// This code was taken from MSDN KB article Q79212.
//
VOID
DrawTopicSpot(
    HDC hdc,
    LPRECT prc,
    SPOTSTATE spot
    )
{
    Assert(NULL != hdc);
    Assert(NULL != prc);

    COLORREF cColor;
    HBITMAP bmAndBack, bmAndObject, bmAndMem, bmSave;
    HBITMAP bmBackOld, bmObjectOld, bmMemOld, bmSaveOld, bmTempOld;
    POINT ptSize;
    RECT rcBitmap;
    HPALETTE hOldPalette[2];
    CDIB& refdibTopicSpot = PTG.rgdibTopicSpot[spot];

    //
    // Select the bitmap's palette into the window DC and
    // memory DC and realize it.
    //
    hOldPalette[0] = SelectPalette(hdc, (HPALETTE)refdibTopicSpot, TRUE);
    RealizePalette(hdc);
    hOldPalette[1] = SelectPalette(PTG.dcMem, (HPALETTE)refdibTopicSpot, TRUE);
    RealizePalette(PTG.dcMem);

    //
    // Select the bitmap into a temp DC.
    //
    CDC dcTemp(hdc);
    bmTempOld = (HBITMAP)SelectObject(dcTemp, (HBITMAP)refdibTopicSpot);

    //
    // Get bitmap dimensions and convert to logical coordinates.
    //
    refdibTopicSpot.GetRect(&rcBitmap);
    ptSize.x = rcBitmap.right;
    ptSize.y = rcBitmap.bottom;
    DPtoLP(dcTemp, &ptSize, 1);

    //
    // Create some DCs to hold temporary data.
    //
    CDC dcBack(hdc);
    CDC dcObject(hdc);
    CDC dcSave(hdc);

    //
    // Create a bitmap for each DC.
    //
    bmAndBack   = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);
    bmAndObject = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);
    bmAndMem    = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);
    bmSave      = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);

    //
    // Each DC must select a bitmap object to store pixel data.
    //
    bmBackOld   = (HBITMAP)SelectObject(dcBack,   bmAndBack);
    bmObjectOld = (HBITMAP)SelectObject(dcObject, bmAndObject);
    bmMemOld    = (HBITMAP)SelectObject(PTG.dcMem,  bmAndMem);
    bmSaveOld   = (HBITMAP)SelectObject(dcSave,   bmSave);

    //
    // Set the proper mapping mode.
    //
    SetMapMode(dcTemp, GetMapMode(hdc));

    //
    // Save the bitmap sent here because it will be overwritten.
    //
    BitBlt(dcSave, 0, 0, ptSize.x, ptSize.y, dcTemp, 0, 0, SRCCOPY);

    //
    // Set the background color of the source DC to the color
    // contained in the parts of the bitmap that should be transparent.
    //
    cColor = SetBkColor(dcTemp, COLOR_TOPICSPOT_MASK);

    //
    // Create the object mask for the bitmap by performing a BitBlt
    // from the source bitmap to a monochrome bitmap.
    //
    BitBlt(dcObject, 0, 0, ptSize.x, ptSize.y, dcTemp, 0, 0, SRCCOPY);

    //
    // Set the background color of the source DC back to the original color.
    //
    SetBkColor(dcTemp, cColor);

    //
    // Create the inverse of the object mask.
    //
    BitBlt(dcBack, 0, 0, ptSize.x, ptSize.y, dcObject, 0, 0, NOTSRCCOPY);

    //
    // Copy the background of the main DC to the destination.
    //
    BitBlt(PTG.dcMem, 0, 0, ptSize.x, ptSize.y, hdc, prc->left, prc->top, SRCCOPY);

    //
    // Mask out the places where the bitmap will be placed.
    //
    BitBlt(PTG.dcMem, 0, 0, ptSize.x, ptSize.y, dcObject, 0, 0, SRCAND);

    //
    // Mask out the transparent colored pixels on the bitmap.
    //
    BitBlt(dcTemp, 0, 0, ptSize.x, ptSize.y, dcBack, 0, 0, SRCAND);

    //
    // XOR the bitmap with the background on the destination DC.
    //
    BitBlt(PTG.dcMem, 0, 0, ptSize.x, ptSize.y, dcTemp, 0, 0, SRCPAINT);

    //
    // Copy the destination to the screen.
    //
    BitBlt(hdc, prc->left, prc->top, ptSize.x, ptSize.y, PTG.dcMem, 0, 0, SRCCOPY);

    //
    // Place the original bitmap back into the bitmap sent here.
    //
    BitBlt(dcTemp, 0, 0, ptSize.x, ptSize.y, dcSave, 0, 0, SRCCOPY);

    //
    // Delete the memory bitmaps.
    //
    DeleteObject(SelectObject(dcBack, bmBackOld));
    DeleteObject(SelectObject(dcObject, bmObjectOld));
    DeleteObject(SelectObject(PTG.dcMem, bmMemOld));
    DeleteObject(SelectObject(dcSave, bmSaveOld));
    SelectObject(dcTemp, bmTempOld);

    //
    // Restore the palette.
    //
    SelectPalette(hdc, hOldPalette[0], TRUE);
    RealizePalette(hdc);

    SelectPalette(PTG.dcMem, hOldPalette[1], TRUE);
}


VOID
DrawBanner(
    HWND hwnd,
    HDC hdc
    )
{
    Assert(NULL != hwnd);

    HPALETTE hOldPalette[2];
    HBITMAP hOldBitmap;
    RECT rc, rcBitmap;
    INT iStretchModeOld;

    GetClientRect(hwnd, &rc);

    //
    // We need the bitmap size for StretchBlt.
    //
    PTG.dibBanner.GetRect(&rcBitmap);

    hOldPalette[0] = SelectPalette(hdc, (HPALETTE)PTG.dibBanner, TRUE);
    RealizePalette(hdc);

    hOldPalette[1] = SelectPalette(PTG.dcMem, (HPALETTE)PTG.dibBanner, TRUE);
    RealizePalette(PTG.dcMem);
    hOldBitmap = (HBITMAP)SelectObject(PTG.dcMem, (HBITMAP)PTG.dibBanner);

    iStretchModeOld = SetStretchBltMode(hdc, COLORONCOLOR);

    StretchBlt(hdc,
               rc.left,
               rc.top,
               rc.right - rc.left,
               rc.bottom - rc.top,
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

    DrawBannerTitle(hwnd, hdc);
}



VOID
DrawBannerTitle(
    HWND hwnd,
    HDC hdc
    )
{
    Assert(NULL != hwnd);

    LPTSTR pszTitle = FmtMsgSprintf(MSG_BANNER_TITLE);
    INT iBkModeOld  = SetBkMode(hdc, TRANSPARENT);
    HFONT hfontOld  = (HFONT)SelectObject(hdc, PTG.hfontBanner);
    RECT rc;

    GetClientRect(hwnd, &rc);
    rc.left += PIXELSX(BANNER_TITLE_INDENT);

    DrawText(hdc,
             pszTitle,
             lstrlen(pszTitle),
             &rc,
             DT_SINGLELINE | DT_VCENTER);

    LocalFree(pszTitle);

    SelectObject(hdc, hfontOld);
    SetBkMode(hdc, iBkModeOld);
}


//
// Draw the topic image and drop shadow.
//
VOID
DrawTopicImage(
    HWND hwnd,
    HDC hdc,
    PSETTINGS_FOLDER_TOPIC pITopic
    )
{
    Assert(NULL != hwnd);
    Assert(NULL != hdc);
    Assert(NULL != pITopic);

    RECT rc, rcImage;
    INT cxImageShadow = PIXELSX(IMAGE_SHADOW_WIDTH);
    INT cyImageShadow = PIXELSY(IMAGE_SHADOW_WIDTH);

    //
    // Draw the image.
    //
    GetClientRect(hwnd, &rc);
    rcImage = rc;

    rcImage.right  -= cxImageShadow;
    rcImage.bottom -= cyImageShadow;
    pITopic->DrawImage(hdc, &rcImage);

    //
    // Draw the bottom shadow.
    //
    rc.left = cxImageShadow;
    rc.top  = rc.bottom - cyImageShadow;
    FillRect(hdc, &rc, (HBRUSH)GetStockObject(GRAY_BRUSH));

    //
    // Draw the right-edge shadow.
    //
    rc.left   = rc.right - cxImageShadow;
    rc.top    = cyImageShadow;
    FillRect(hdc, &rc, (HBRUSH)GetStockObject(GRAY_BRUSH));

    //
    // Draw a thin black frame.
    //
    FrameRect(hdc, &rcImage, (HBRUSH)GetStockObject(BLACK_BRUSH));
}


//
// Draw an item in the topic list.
//
LRESULT
OnDrawItem(
    HWND hwnd,
    UINT idCtl,
    LPDRAWITEMSTRUCT pdis
    )
{
    Assert(NULL != hwnd);
    Assert(NULL != pdis);

    switch(pdis->CtlType)
    {
        case ODT_LISTVIEW:
            DrawListviewItem(hwnd, idCtl, pdis);
            break;

        case ODT_STATIC:
            switch(pdis->CtlID)
            {
                case IDC_BMP_IMAGE:
                    DrawTopicImage(pdis->hwndItem, pdis->hDC, PTG.pICurrentTopic);
                    break;

                case IDC_BMP_BANNER:
                    DrawBanner(pdis->hwndItem, pdis->hDC);
                    break;

                default:
                    break;
            }
            break;

        default:
            break;
    }
    return 1;
}


VOID
DrawListviewItem(
    HWND hwnd,
    UINT idCtl,
    LPDRAWITEMSTRUCT pdis
    )
{
    COLORREF clrText = PTG.clrTopicTextNormal;
    COLORREF clrOld, clrBkOld;
    INT iBkModeOld;
    HFONT hfontText = (HFONT)PTG.hfontTextNormal;
    HFONT hfontOld;
    SPOTSTATE spot = SPOT_NORMAL;
    PSETTINGS_FOLDER_TOPIC pITopic = (PSETTINGS_FOLDER_TOPIC)pdis->itemData;

    //
    // ODA_DRAWENTIRE is the only ownerdraw action sent by the listview
    // control.  No need to test for the action.
    //
    if (pdis->itemState & (ODS_FOCUS | ODS_SELECTED))
    {
        if (pdis->itemID == (DWORD)PTG.iLastItemHit)
        {
            //
            // Drawing the current "hit" item.  Use highlight
            // font and text color.
            //
            clrText   = PTG.clrTopicTextHighlight;
            hfontText = (HFONT)PTG.hfontTextHighlight;
        }

        spot = SPOT_FOCUS;
        //
        // Tell the dialog that the listview is the "default"
        // control.  If the user presses [Enter] while the listview
        // has focus, the current topic is launched.
        //
        SendMessage(hwnd, DM_SETDEFID, IDC_LIST_TOPICS, 0);
    }
    else
    {
        //
        // Not FOCUSED and not SELECTED
        //
        if (pdis->itemID == (DWORD)PTG.iLastItemHit &&
            PTG.bListviewSingleClick)
        {
            //
            // Drawing the current "hit" item but it is not the
            // currently selected topic.  Still use hightlight font
            // and text color.  This is what makes the highlighting
            // follow the mouse cursor.  Note that this doesn't happen
            // when we're in double-click mode.
            //
            clrText   = PTG.clrTopicTextHighlight;
            hfontText = (HFONT)PTG.hfontTextHighlight;
        }
    }

    //
    // Draw the "spot" to the left of the topic title.
    //
    DrawTopicSpot(pdis->hDC, &pdis->rcItem, spot);

    //
    // Get the topic title from the topic object.
    //
    LPCTSTR pszTitle;
    pITopic->GetTitle(pszTitle);
    Assert(NULL != pszTitle);

    //
    // Need OPAQUE drawing so we erase the underline when
    // updating a previously underlined item with a non-underlined
    // font.
    //
    iBkModeOld = SetBkMode(pdis->hDC, OPAQUE);
    clrBkOld   = SetBkColor(pdis->hDC, PTG.clrTopicTextBackground);
    clrOld     = SetTextColor(pdis->hDC, clrText);
    hfontOld   = (HFONT)SelectObject(pdis->hDC, hfontText);

    //
    // Draw the title string.
    // Note we leave space to the left for the "spot".
    // Also leave space on the right in case a vertical scroll bar is
    // displayed.  This could happen if localizers don't keep their
    // topic title strings short.
    //
    pdis->rcItem.left  += TOPIC_LIST_LMARGIN();
    pdis->rcItem.right -= TOPIC_LIST_RMARGIN();
    DrawText(pdis->hDC,
             pszTitle,
             lstrlen(pszTitle),
             &pdis->rcItem,
             DT_NOCLIP | DT_WORDBREAK);

    SetTextColor(pdis->hDC, clrOld);
    SetBkColor(pdis->hDC, clrBkOld);
    SetBkMode(pdis->hDC, iBkModeOld);
    SelectObject(pdis->hDC, hfontOld);
}


//
// Determine if the mouse pointer is currently over a listview item.
//
INT
ListViewItemHit(
    VOID
    )
{
    INT iItem = -1;
    POINT ptMouse;
    if (GetCursorPos(&ptMouse) && ScreenToClient(PTG.hwndTopicList, &ptMouse))
    {
        iItem = ListViewItemHit(ptMouse.x, ptMouse.y);
    }
    return iItem;
}


//
// Determine if a given x,y coordinate is over a listview item.
//
INT
ListViewItemHit(
    INT xPos,
    INT yPos
    )
{
    INT iItem = -1;
    LV_HITTESTINFO hti;

    hti.pt.x = xPos;
    hti.pt.y = yPos;

    iItem = ListView_HitTest(PTG.hwndTopicList, &hti);
    if (!(LVHT_ONITEM & hti.flags))
        iItem = -1;

    return iItem;
}



LRESULT
OnNotify(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam
    )
{

    LPNMLISTVIEW pnmlv = (LPNMLISTVIEW)lParam;

    switch(pnmlv->hdr.code)
    {
        case LVN_ITEMCHANGED:
            if (LVIS_FOCUSED & pnmlv->uNewState)
            {
                //
                // New item has focus.
                // Set the global "current" topic pointer and force and update
                // of the topic image and image caption to pick up the new
                // image and text.
                //
                PTG.pICurrentTopic = (PSETTINGS_FOLDER_TOPIC)pnmlv->lParam;
                InvalidateRect(GetDlgItem(hwnd, IDC_BMP_IMAGE), NULL, TRUE);
                UpdateWindow(GetDlgItem(hwnd, IDC_BMP_IMAGE));

                //
                // Set the new topic image caption.
                //
                LPCTSTR pszText;
                PTG.pICurrentTopic->GetCaption(pszText);
                SetWindowText(GetDlgItem(hwnd, IDC_TXT_IMAGE), pszText);
            }
            break;

        case LVN_ITEMACTIVATE:
            //
            // Item was selected in the listview.
            // Tell the current topic to do it's thing.
            //
            PTG.bWaitCursor = TRUE;
            SendMessage(hwnd, WM_SETCURSOR, 0, 0);
            PTG.pICurrentTopic->TopicSelected();
            PTG.bWaitCursor = FALSE;
            break;

        default:
            break;
    }

    return 0;
}


LRESULT 
OnSetCursor(
    HWND hwnd, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    LRESULT lResult  = 0;
    INT iItemHit     = ListViewItemHit();
    INT iLastItemHit = PTG.iLastItemHit;

    if (iItemHit != iLastItemHit)
    {
        //
        // Mouse is over a new item or has left the list view.
        // Set the new "last item hit" and update both items.
        // Order of statement execution is important here.
        //
        HWND hwndLV = PTG.hwndTopicList;
        RECT rc;

        ListView_GetItemRect(hwndLV, iLastItemHit, &rc, LVIR_BOUNDS);
        PTG.iLastItemHit = iItemHit;
        InvalidateRect(hwndLV, &rc, FALSE);

        if (-1 != iItemHit)
        {
            //
            // Currently over an item, update it to display the underline
            // font.
            //
            ListView_GetItemRect(hwndLV, iItemHit, &rc, LVIR_BOUNDS);
            InvalidateRect(hwndLV, &rc, FALSE);
        }
    }
    if (PTG.bWaitCursor)
    {
        SetCursor(PTG.hcursorWait);
        lResult = 1;
    }
    else if (-1 != iItemHit && PTG.bListviewSingleClick)
    {
        SetCursor(PTG.hcursorHand);
        lResult = 1;
    }

    return lResult;
}




LRESULT
OnQueryNewPalette(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HPALETTE hOldPal;
    INT i = 0;

    //
    // Re-realize the watermark palette in the foreground and the
    // the old palette in the background.
    //
    CDC dc(hwnd);
    hOldPal = SelectPalette(dc, (HPALETTE)PTG.dibBanner, FALSE);
    i       = RealizePalette(dc);

    SelectPalette(dc, hOldPal, TRUE);
    RealizePalette(dc);
    if (0 == i)
        SendMessage(hwnd, WM_PALETTECHANGED, (WPARAM)hwnd, NULL);
    else
    {
        InvalidateRect(GetDlgItem(hwnd, IDC_BMP_IMAGE), NULL, TRUE);
        InvalidateRect(GetDlgItem(hwnd, IDC_BMP_BANNER), NULL, TRUE);
    }

    return i;
}



LRESULT
OnPaletteChanged(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam
    )
{
    Assert(NULL != hwnd);

    HPALETTE hOldPal = NULL;
    CDC dc(hwnd);
    INT i;

    if ((HWND)wParam != hwnd)
    {
        //
        // Our window didn't cause the change.  OK to realize the
        // watermark palette.
        //
        hOldPal = SelectPalette(dc, (HPALETTE)PTG.dibBanner, TRUE);
        RealizePalette(dc);
    }

    //
    // Realize the topic menu spot palettes.
    //
    HPALETTE hPalTemp = SelectPalette(dc, (HPALETTE)PTG.rgdibTopicSpot[0], TRUE);
    if (NULL == hOldPal)
        hOldPal = hPalTemp;

    i = RealizePalette(dc);
    if (0 < i)
        InvalidateRect(PTG.hwndTopicList, NULL, TRUE);

    //
    // Notify all of the topic objects about the palette change.
    // They'll need to realize any palettes they use.
    //
    if (NotifyTopicsOfPaletteChange(hwnd))
    {
        //
        // System palette changed while realizing topic image palettes.
        // Repaint the image.
        //
        InvalidateRect(GetDlgItem(hwnd, IDC_BMP_BANNER), NULL, TRUE);
        InvalidateRect(GetDlgItem(hwnd, IDC_BMP_IMAGE), NULL, TRUE);
    }

    //
    // Update any areas invalidated by palette realization.
    //
    UpdateWindow(hwnd);

    if (NULL != hOldPal)
    {
        //
        // Restore original palette in DC.
        //
        SelectPalette(dc, hOldPal, TRUE);
        RealizePalette(dc);
    }

    return 0;
}


BOOL
NotifyTopicsOfPaletteChange(
    HWND hwnd
    )
{
    INT cTopics;
    PSETTINGS_FOLDER_TOPIC pITopic = NULL;
    BOOL fSysPalChanged = FALSE;

    cTopics = PTG.Folder.GetTopicCount();

    //
    // Now notify and release the topic objects.
    //
    for (INT i = 0; i < cTopics; i++)
    {
        PSETTINGS_FOLDER_TOPIC pITopic = PTG.Folder.GetTopic(i);
        if (NULL != pITopic)
        {
            HRESULT hResult = pITopic->PaletteChanged(hwnd);

            if (!fSysPalChanged && S_OK == hResult)
                 fSysPalChanged = TRUE;

            pITopic->Release();
        }
    }

    //
    // Tell the caller if the system palette changed.
    //
    return fSysPalChanged;
}
