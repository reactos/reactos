#include "precomp.hxx"
#pragma hdrstop


//
// Global variables -----------------------------------------------------------
//
DWORD g_dwTlsIndex = TLS_OUT_OF_INDEXES;
HINSTANCE g_hInstance;


PerThreadGlobals::PerThreadGlobals(
    VOID
    ) : dcMem((HDC)NULL)
{
    DialogBaseUnitsX = LOWORD(GetDialogBaseUnits());
    DialogBaseUnitsY = HIWORD(GetDialogBaseUnits());
    cxSmallIcon      = GetSystemMetrics(SM_CXSMICON);
    cySmallIcon      = GetSystemMetrics(SM_CYSMICON);
    cxLargeIcon      = GetSystemMetrics(SM_CXICON);
    cyLargeIcon      = GetSystemMetrics(SM_CYICON);
    cxVertScrollBar  = GetSystemMetrics(SM_CXVSCROLL);
    iLastItemHit     = -1;
    hwndTopicList    = NULL;
    haccelKeyboard   = LoadAccelerators(g_hInstance, MAKEINTRESOURCE(IDR_KBDACCEL));
    pICurrentTopic   = NULL;
    bListviewSingleClick = FALSE;
    bWaitCursor          = FALSE;
    pfnTaskbarPropSheetCallback = NULL;

    //
    // Load the application icon (large and small sizes).
    //
    hiconApplicationSm = (HICON)LoadImage(g_hInstance,
                                          MAKEINTRESOURCE(IDI_SETTINGS),
                                          IMAGE_ICON,
                                          cxSmallIcon,
                                          cySmallIcon,
                                          LR_SHARED);

    hiconApplicationLg = (HICON)LoadImage(g_hInstance,
                                          MAKEINTRESOURCE(IDI_SETTINGS),
                                          IMAGE_ICON,
                                          cxLargeIcon,
                                          cyLargeIcon,
                                          LR_SHARED);
    //
    // Load the hand cursor that we display over the listview.
    // NOTE:  The listview normally provides this hand cursor EXCEPT
    //        when the listview has the LVS_OWNERDRAWFIXED style set.
    //        We have that style set so we have to do our own cursor
    //        management.
    //
    hcursorHand = LoadCursor(g_hInstance, MAKEINTRESOURCE(IDC_CURSOR_HAND));
    hcursorWait = LoadCursor(NULL, IDC_WAIT);

    OnSysColorChange();
    OnDisplayChange();
}

PerThreadGlobals::~PerThreadGlobals(
    VOID
    )
{

}


VOID
PerThreadGlobals::OnSysColorChange(
    VOID
    )
{
    //
    // Do this in one place since we have to do it on startup and
    // on SYSCOLORCHANGE.
    //
    clrTopicTextNormal     = GetSysColor(COLOR_BTNTEXT);
    clrTopicTextHighlight  = GetSysColor(COLOR_ACTIVECAPTION);
    clrTopicTextBackground = GetSysColor(COLOR_MENU);
}


VOID
PerThreadGlobals::OnDisplayChange(
    VOID
    )
{
    //
    // Load the Topic list spot bitmaps.
    //
    rgdibTopicSpot[0].Load(g_hInstance, MAKEINTRESOURCE(IDB_SPOT_NORMAL));
    rgdibTopicSpot[1].Load(g_hInstance, MAKEINTRESOURCE(IDB_SPOT_FOCUS));

    //
    // Load the banner bitmap.
    //
    dibBanner.Load(g_hInstance, MAKEINTRESOURCE(IDB_BANNER));
}