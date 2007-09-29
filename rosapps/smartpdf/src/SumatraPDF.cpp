/* Copyright Krzysztof Kowalczyk 2006-2007
   License: GPLv2 */

// have to undef _WIN32_IE here to use TB_* constants
// (_WIN32_IE was defined in *.cbp - codeblocks project file)
// ==> we have to use a stable API instead of MinGW 5.1.3 :-\

#ifdef __GNUC__
#undef _WIN32_IE
#endif

#include "SumatraPDF.h"

#include "str_util.h"
#include "file_util.h"
#include "geom_util.h"
#include "win_util.h"
#include "translations.h"

#include "SumatraDialogs.h"
#include "FileHistory.h"
#include "AppPrefs.h"
#include "DisplayModelSplash.h"

/* TODO: this and StandardSecurityHandler::getAuthData() and new GlobalParams
   should be moved to another file (PopplerInit(), PopplerDeinit() */
#include "PDFDoc.h"
#include "SecurityHandler.h"
#include "GlobalParams.h"

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <direct.h> /* for _mkdir() */

#include <shellapi.h>
#include <shlobj.h>

#include "str_strsafe.h"

#include <windowsx.h>

// some stupid things are in headers of MinGW 5.1.3 :-\
// why we have to define these constants & prototypes again (?!)
#ifdef __GNUC__
#ifndef VK_OEM_PLUS
#define VK_OEM_PLUS     0xBB
#endif
#ifndef VK_OEM_MINUS
#define VK_OEM_MINUS 0xBD
#endif

extern "C" {
extern BOOL WINAPI GetDefaultPrinterA(LPSTR,LPDWORD);
extern BOOL WINAPI GetDefaultPrinterW(LPWSTR,LPDWORD);
}
#endif

// this sucks but I don't know any other way
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")

//#define FANCY_UI 1

/* Define if you want to conserve memory by always freeing cached bitmaps
   for pages not visible. Only enable for stress-testing the logic. On
   desktop machine we usually have plenty memory */
//#define CONSERVE_MEMORY 1

/* Next action for the benchmark mode */
#define MSG_BENCH_NEXT_ACTION WM_USER + 1

#define ZOOM_IN_FACTOR      1.2
#define ZOOM_OUT_FACTOR     1.0 / ZOOM_IN_FACTOR

/* if TRUE, we're in debug mode where we show links as blue rectangle on
   the screen. Makes debugging code related to links easier.
   TODO: make a menu item in DEBUG build to turn it on/off. */
#ifdef DEBUG
static BOOL             gDebugShowLinks = TRUE;
#else
static BOOL             gDebugShowLinks = FALSE;
#endif

/* default UI settings */
#define DEFAULT_DISPLAY_MODE DM_CONTINUOUS

//#define DEFAULT_ZOOM            ZOOM_FIT_WIDTH
#define DEFAULT_ZOOM            ZOOM_FIT_PAGE
#define DEFAULT_ROTATION        0

//#define START_WITH_ABOUT        1

/* define if want to use double-buffering for rendering the PDF. Takes more memory!. */
#define DOUBLE_BUFFER 1

#define DRAGQUERY_NUMFILES 0xFFFFFFFF

#define MAX_LOADSTRING 100

#define WM_CREATE_FAILED -1
#define WM_CREATE_OK 0
#define WM_NCPAINT_HANDLED 0
#define WM_VSCROLL_HANDLED 0
#define WM_HSCROLL_HANDLED 0

#define ABOUT_WIN_DX 440
#define ABOUT_WIN_DY 300

#define WM_APP_REPAINT_DELAYED (WM_APP + 10)
#define WM_APP_REPAINT_NOW     (WM_APP + 11)

/* A caption is 4 white/blue 2 pixel line and a 3 pixel white line */
#define CAPTION_DY 2*(2*4)+3

#define COL_CAPTION_BLUE RGB(0,0x50,0xa0)
#define COL_WHITE RGB(0xff,0xff,0xff)
#define COL_BLACK RGB(0,0,0)
#define COL_WINDOW_BG RGB(0xcc, 0xcc, 0xcc)
#define COL_WINDOW_SHADOW RGB(0x40, 0x40, 0x40)

#define FRAME_CLASS_NAME    _T("SUMATRA_PDF_FRAME")
#define CANVAS_CLASS_NAME   _T("SUMATRA_PDF_CANVAS")
#define ABOUT_CLASS_NAME    _T("SUMATRA_PDF_ABOUT")
#define APP_NAME            _T("SumatraPDF")
#define PDF_DOC_NAME        _T("Adobe PDF Document")
#define ABOUT_WIN_TITLE     _TR("About SumatraPDF")
#define PREFS_FILE_NAME     _T("sumatrapdfprefs.txt")
#define APP_SUB_DIR         _T("SumatraPDF")

#define BENCH_ARG_TXT             "-bench"
#define PRINT_TO_ARG_TXT          "-print-to"
#define NO_REGISTER_EXT_ARG_TXT   "-no-register-ext"
#define PRINT_TO_DEFAULT_ARG_TXT  "-print-to-default"
#define EXIT_ON_PRINT_ARG_TXT     "-exit-on-print"
#define ENUM_PRINTERS_ARG_TXT     "-enum-printers"

/* Default size for the window, happens to be american A4 size (I think) */
#define DEF_WIN_DX 612
#define DEF_WIN_DY 792

#define REPAINT_TIMER_ID    1
#define REPAINT_DELAY_IN_MS 400

/* A special "pointer" vlaue indicating that we tried to render this bitmap
   but couldn't (e.g. due to lack of memory) */
#define BITMAP_CANNOT_RENDER (RenderedBitmap*)NULL

#define WS_REBAR (WS_CHILD | WS_CLIPCHILDREN | WS_BORDER | RBS_VARHEIGHT | \
                  RBS_BANDBORDERS | CCS_NODIVIDER | CCS_NOPARENTALIGN)

#define MAX_RECENT_FILES_IN_MENU 15

typedef struct StrList {
    struct StrList *    next;
    char *              str;
} StrList;

static FileHistoryList *            gFileHistoryRoot = NULL;

static HINSTANCE                    ghinst = NULL;
TCHAR                               windowTitle[MAX_LOADSTRING];

static WindowInfo*                  gWindowList;

static HCURSOR                      gCursorArrow;
static HCURSOR                      gCursorHand;
static HCURSOR                      gCursorDrag;
static HBRUSH                       gBrushBg;
static HBRUSH                       gBrushWhite;
static HBRUSH                       gBrushShadow;
static HBRUSH                       gBrushLinkDebug;

static HPEN                         ghpenWhite;
static HPEN                         ghpenBlue;

#ifdef _WINDLL
static bool                         gRunningDLL = true;
#else
static bool                         gRunningDLL = false;
#endif
//static AppVisualStyle               gVisualStyle = VS_WINDOWS;

static char *                       gBenchFileName;
static int                          gBenchPageNum = INVALID_PAGE_NO;
BOOL                                gShowToolbar = TRUE;
BOOL                                gUseFitz = TRUE;
/* If false, we won't ask the user if he wants Sumatra to handle PDF files */
BOOL                                gPdfAssociateDontAskAgain = FALSE;
/* If gPdfAssociateDontAskAgain is TRUE, says whether we should silently associate
   or not */
BOOL                                gPdfAssociateShouldAssociate = TRUE;
#ifdef DOUBLE_BUFFER
static bool                         gUseDoubleBuffer = true;
#else
static bool                         gUseDoubleBuffer = false;
#endif

#define MAX_PAGE_REQUESTS 8
static PageRenderRequest            gPageRenderRequests[MAX_PAGE_REQUESTS];
static int                          gPageRenderRequestsCount = 0;

static HANDLE                       gPageRenderThreadHandle;
static HANDLE                       gPageRenderSem;
static PageRenderRequest *          gCurPageRenderReq;

static int                          gReBarDy;
static int                          gReBarDyFrame;
static HWND                         gHwndAbout;

typedef struct ToolbarButtonInfo {
    /* information provided at compile time */
    int         bitmapResourceId;
    int         cmdId;
    TCHAR *     toolTip;

    /* information calculated at runtime */
    int         index;
} ToolbarButtonInfo;

#define IDB_SEPARATOR  -1

ToolbarButtonInfo gToolbarButtons[] = {
    { IDB_SILK_OPEN,     IDM_OPEN, _TRN(TEXT("Open")), 0 },
    { IDB_SEPARATOR,     IDB_SEPARATOR, 0, 0 },
    { IDB_SILK_PREV,     IDM_GOTO_PREV_PAGE, _TRN(TEXT("Previous Page")), 0 },
    { IDB_SILK_NEXT,     IDM_GOTO_NEXT_PAGE, _TRN(TEXT("Next Page")), 0 },
    { IDB_SEPARATOR,     IDB_SEPARATOR, 0, 0 },
    { IDB_SILK_ZOOM_IN,  IDT_VIEW_ZOOMIN, _TRN(TEXT("Zoom In")), 0 },
    { IDB_SILK_ZOOM_OUT, IDT_VIEW_ZOOMOUT, _TRN(TEXT("Zoom Out")), 0 }
};  /* @note: TEXT() casts */

#define DEFAULT_LANGUAGE "en"

#define TOOLBAR_BUTTONS_COUNT dimof(gToolbarButtons)

static const char *g_currLangName;

struct LangDef {
    const char* _langName;
    int         _langId;
} g_langs[] = {
    {"en", IDM_LANG_EN},
    {"pl", IDM_LANG_PL},
    {"fr", IDM_LANG_FR},
    {"de", IDM_LANG_DE},
};

#define LANGS_COUNT dimof(g_langs)

static void WindowInfo_ResizeToPage(WindowInfo *win, int pageNo);
static void CreateToolbar(WindowInfo *win, HINSTANCE hInst);
static void RebuildProgramMenus(void);

const char* CurrLangNameGet() {
    if (!g_currLangName)
        return DEFAULT_LANGUAGE;
    return g_currLangName;
}

bool CurrLangNameSet(const char* langName) {
    bool validLang = false;
    for (int i=0; i < LANGS_COUNT; i++) {
        if (str_eq(langName, g_langs[i]._langName)) {
            validLang = true;
            break;
        }
    }
    assert(validLang);
    if (!validLang) return false;
    free((void*)g_currLangName);
    g_currLangName = str_dup(langName);

    bool ok = Translations_SetCurrentLanguage(langName);
    assert(ok);

    return true;
}

void CurrLangNameFree() {
    free((void*)g_currLangName);
    g_currLangName = NULL;
}

void LaunchBrowser(const TCHAR *url)
{
    launch_url(url);
}

static BOOL pageRenderAbortCb(void *data)
{
    PageRenderRequest *req = (PageRenderRequest*)data;
    if (req->abort) {
        DBG_OUT("Rendering of page %d aborted\n", req->pageNo);
        return TRUE;
    }
    else
        return FALSE;
}

void RenderQueue_RemoveForDisplayModel(DisplayModel *dm) {
    LockCache();
    int reqCount = gPageRenderRequestsCount;
    int curPos = 0;
    for(int i = 0; i < reqCount; i++) {
        PageRenderRequest *req = &(gPageRenderRequests[i]);
        bool shouldRemove = (req->dm == dm);
        if (i != curPos)
            gPageRenderRequests[curPos] = gPageRenderRequests[i];
        if (shouldRemove)
            --gPageRenderRequestsCount;
        else
            ++curPos;
    }
    UnlockCache();
}

/* Wait until rendering of a page beloging to <dm> has finished. */
/* TODO: this might take some time, would be good to show a dialog to let the
   user know he has to wait until we finish */
void cancelRenderingForDisplayModel(DisplayModel *dm) {

    DBG_OUT("cancelRenderingForDisplayModel()\n");
    bool renderingFinished = false;;
    for (;;) {
        LockCache();
        if (!gCurPageRenderReq || (gCurPageRenderReq->dm != dm))
            renderingFinished = true;
        else
            gCurPageRenderReq->abort = TRUE;
        UnlockCache();
        if (renderingFinished)
            break;
        /* TODO: busy loop is not good, but I don't have a better idea */
        sleep_milliseconds(500);
    }
}

/* Render a bitmap for page <pageNo> in <dm>. */
void RenderQueue_Add(DisplayModel *dm, int pageNo) {
    DBG_OUT("RenderQueue_Add(pageNo=%d)\n", pageNo);
    assert(dm);
    if (!dm) return; //goto Exit; /* @note: because of crosses initialization of [...] */

    LockCache();
    PdfPageInfo *pageInfo = dm->getPageInfo(pageNo);
    int rotation = dm->rotation();
    normalizeRotation(&rotation);
    double zoomLevel = dm->zoomReal();

    if (BitmapCache_Exists(dm, pageNo, zoomLevel, rotation)) {
        goto LeaveCsAndExit;
    }

    if (gCurPageRenderReq && 
        (gCurPageRenderReq->pageNo == pageNo) && (gCurPageRenderReq->dm == dm)) {
        if ((gCurPageRenderReq->zoomLevel != zoomLevel) || (gCurPageRenderReq->rotation != rotation)) {
            /* Currently rendered page is for the same page but with different zoom
            or rotation, so abort it */
            DBG_OUT("  aborting rendering\n");
            gCurPageRenderReq->abort = TRUE;
        } else {
            /* we're already rendering exactly the same page */
            DBG_OUT("  already rendering this page\n");
            goto LeaveCsAndExit;
        }
    }

    for (int i=0; i < gPageRenderRequestsCount; i++) {
        PageRenderRequest* req = &(gPageRenderRequests[i]);
        if ((req->pageNo == pageNo) && (req->dm == dm)) {
            if ((req->zoomLevel == zoomLevel) && (req->rotation == rotation)) {
                /* Request with exactly the same parameters already queued for
                   rendering. Move it to the top of the queue so that it'll
                   be rendered faster. */
                PageRenderRequest tmp;
                tmp = gPageRenderRequests[gPageRenderRequestsCount-1];
                gPageRenderRequests[gPageRenderRequestsCount-1] = *req;
                *req = tmp;
                DBG_OUT("  already queued\n");
                goto LeaveCsAndExit;
            } else {
                /* There was a request queued for the same page but with different
                   zoom or rotation, so only replace this request */
                DBG_OUT("Replacing request for page %d with new request\n", req->pageNo);
                req->zoomLevel = zoomLevel;
                req->rotation = rotation;
                goto LeaveCsAndExit;
            
            }
        }
    }

    PageRenderRequest* newRequest;
    /* add request to the queue */
    if (gPageRenderRequestsCount == MAX_PAGE_REQUESTS) {
        /* queue is full -> remove the oldest items on the queue */
        memmove(&(gPageRenderRequests[0]), &(gPageRenderRequests[1]), sizeof(PageRenderRequest)*(MAX_PAGE_REQUESTS-1));
        newRequest = &(gPageRenderRequests[MAX_PAGE_REQUESTS-1]);
    } else {
        newRequest = &(gPageRenderRequests[gPageRenderRequestsCount]);
        gPageRenderRequestsCount++;
    }
    assert(gPageRenderRequestsCount <= MAX_PAGE_REQUESTS);
    newRequest->dm = dm;
    newRequest->pageNo = pageNo;
    newRequest->zoomLevel = zoomLevel;
    newRequest->rotation = rotation;
    newRequest->abort = FALSE;

    UnlockCache();
    /* tell rendering thread there's a new request to render */
    LONG  prevCount;
    ReleaseSemaphore(gPageRenderSem, 1, &prevCount);
Exit:
    return;
LeaveCsAndExit:
    UnlockCache();
    return;
}

void RenderQueue_Pop(PageRenderRequest *req)
{
    LockCache();
    assert(gPageRenderRequestsCount > 0);
    assert(gPageRenderRequestsCount <= MAX_PAGE_REQUESTS);
    --gPageRenderRequestsCount;
    *req = gPageRenderRequests[gPageRenderRequestsCount];
    assert(gPageRenderRequestsCount >= 0);
    UnlockCache();
}

static HMENU FindMenuItem(WindowInfo *win, UINT id)
{
    HMENU menuMain = GetMenu(win->hwndFrame);

    /* TODO: to be fully valid, it would have to be recursive */
    for (int i = 0; i < GetMenuItemCount(menuMain); i++) {
        UINT thisId = GetMenuItemID(menuMain, i);
        HMENU subMenu = GetSubMenu(menuMain, i);
        if (id == thisId)
            return subMenu;
        for (int j = 0; j < GetMenuItemCount(subMenu); j++) {
            thisId = GetMenuItemID(menuMain, j);
            if (id == thisId)
                return GetSubMenu(subMenu, j);
        }
    }
    return NULL;
}

static HMENU GetFileMenu(HWND hwnd)
{
    return GetSubMenu(GetMenu(hwnd), 0);
}

static void SwitchToDisplayMode(WindowInfo *win, DisplayMode displayMode)
{
    HMENU   menuMain;
    UINT    id;
    
    menuMain = GetMenu(win->hwndFrame);
    CheckMenuItem(menuMain, IDM_VIEW_SINGLE_PAGE, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(menuMain, IDM_VIEW_CONTINUOUS, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(menuMain, IDM_VIEW_FACING, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(menuMain, IDM_VIEW_CONTINUOUS_FACING, MF_BYCOMMAND | MF_UNCHECKED);

    win->dm->changeDisplayMode(displayMode);
    if (DM_SINGLE_PAGE == displayMode) {
        id = IDM_VIEW_SINGLE_PAGE;
    } else if (DM_FACING == displayMode) {
        id =  IDM_VIEW_FACING;
    } else if (DM_CONTINUOUS == displayMode) {
        id =  IDM_VIEW_CONTINUOUS;
    } else if (DM_CONTINUOUS_FACING == displayMode) {
        id =  IDM_VIEW_CONTINUOUS_FACING;
    } else
        assert(0);

    CheckMenuItem(menuMain, id, MF_BYCOMMAND | MF_CHECKED);
}

static UINT AllocNewMenuId(void)
{
    static UINT firstId = 1000;
    ++firstId;
    return firstId;
}

#define SEP_ITEM "-----"

typedef struct MenuDef {
    const char *m_title;
    int         m_id;
} MenuDef;

MenuDef menuDefFile[] = {
    { _TRN("&Open\tCtrl-O"),       IDM_OPEN },
    { _TRN("&Close\tCtrl-W"),      IDM_CLOSE  },
    { _TRN("&Save as"),            IDM_SAVEAS },
    { _TRN("&Print"),              IDM_PRINT },
    { SEP_ITEM,              0 },
    { _TRN("Make SumatraPDF a default PDF reader"), IDM_MAKE_DEFAULT_READER },
    { SEP_ITEM ,             0 },
    { _TRN("E&xit\tCtrl-Q"),       IDM_EXIT }
};

MenuDef menuDefView[] = {
    { _TRN("Single page"),                 IDM_VIEW_SINGLE_PAGE },
    { _TRN("Facing"),                      IDM_VIEW_FACING },
    { _TRN("Continuous"),                  IDM_VIEW_CONTINUOUS },
    { _TRN("Continuous facing"),           IDM_VIEW_CONTINUOUS_FACING },
    { SEP_ITEM, 0 },
    { _TRN("Rotate left"),                 IDM_VIEW_ROTATE_LEFT },
    { _TRN("Rotate right"),                IDM_VIEW_ROTATE_RIGHT },
    { SEP_ITEM, 0 },
    { _TRN("Show toolbar"),                IDM_VIEW_SHOW_HIDE_TOOLBAR },
    { SEP_ITEM, 0 },
    { _TRN("Use MuPDF rendering engine"),  IDM_VIEW_USE_FITZ },
};

MenuDef menuDefGoTo[] = {
    { _TRN("Next Page"),                   IDM_GOTO_NEXT_PAGE },
    { _TRN("Previous Page"),               IDM_GOTO_PREV_PAGE },
    { _TRN("First Page\tHome"),            IDM_GOTO_FIRST_PAGE },
    { _TRN("Last Page\tEnd"),              IDM_GOTO_LAST_PAGE },
    { _TRN("Page...\tCtrl-G"),             IDM_GOTO_PAGE },
};

MenuDef menuDefZoom[] = {
    { _TRN("Fit &Page\tCtrl-0"),           IDM_ZOOM_FIT_PAGE },
    { _TRN("Act&ual Size\tCtrl-1"),        IDM_ZOOM_ACTUAL_SIZE },
    { _TRN("Fit Widt&h\tCtrl-2"),          IDM_ZOOM_FIT_WIDTH },
    { SEP_ITEM },
    { _TRN("6400%"),                       IDM_ZOOM_6400 },
    { _TRN("3200%"),                       IDM_ZOOM_3200 },
    { _TRN("1600%"),                       IDM_ZOOM_1600 },
    { _TRN("800%"),                        IDM_ZOOM_800 },
    { _TRN("400%"),                        IDM_ZOOM_400 },
    { _TRN("200%"),                        IDM_ZOOM_200 },
    { _TRN("150%"),                        IDM_ZOOM_150 },
    { _TRN("125%"),                        IDM_ZOOM_125 },
    { _TRN("100%"),                        IDM_ZOOM_100 },
    { _TRN("50%"),                         IDM_ZOOM_50 },
    { _TRN("25%"),                         IDM_ZOOM_25 },
    { _TRN("12.5%"),                       IDM_ZOOM_12_5 },
    { _TRN("8.33%"),                       IDM_ZOOM_8_33 },
};

MenuDef menuDefLang[] = {
    { _TRN("&English"), IDM_LANG_EN },
    { _TRN("&French"),  IDM_LANG_FR },
    { _TRN("&German"),  IDM_LANG_DE },
    { _TRN("&Polish"),  IDM_LANG_PL },
};

MenuDef menuDefHelp[] = {
    { _TRN("&Visit website"),              IDM_VISIT_WEBSITE },
    { _TRN("&About"),                      IDM_ABOUT }
};

static void AddFileMenuItem(HMENU menuFile, FileHistoryList *node)
{
    assert(node);
    if (!node) return;
    assert(menuFile);
    if (!menuFile) return;

    UINT newId = node->menuId;
    if (INVALID_MENU_ID == node->menuId)
        newId = AllocNewMenuId();
    const char* txt = FilePath_GetBaseName(node->state.filePath);
    AppendMenu(menuFile, MF_ENABLED | MF_STRING, newId, (TCHAR*)txt); /* @note: TCHAR* cast */
    node->menuId = newId;
}

static HMENU BuildMenuFromMenuDef(MenuDef menuDefs[], int menuItems)
{
    HMENU m = CreateMenu();
    if (NULL == m) return NULL;
    for (int i=0; i < menuItems; i++) {
        MenuDef md = menuDefs[i];
        const char *title = md.m_title;
        int id = md.m_id;
        if (str_eq(title, SEP_ITEM))
            AppendMenu(m, MF_SEPARATOR, 0, NULL);
        else {
            const WCHAR* wtitle = Translatations_GetTranslationW(title);
            if (wtitle)
                AppendMenuW(m, MF_STRING, (UINT_PTR)id, wtitle);
        }
    }
    return m;
}

static HMENU g_currMenu = NULL;

static void DestroyCurrentMenu()
{
    DestroyMenu(g_currMenu);
    g_currMenu = NULL;
}

static void AddRecentFilesToMenu(HMENU m)
{
    if (!gFileHistoryRoot) return;

    AppendMenu(m, MF_SEPARATOR, 0, NULL);

    int  itemsAdded = 0;
    FileHistoryList *curr = gFileHistoryRoot;
    while (curr) {
        assert(curr->state.filePath);
        if (curr->state.filePath) {
            AddFileMenuItem(m, curr);
            assert(curr->menuId != INVALID_MENU_ID);
            ++itemsAdded;
            if (itemsAdded >= MAX_RECENT_FILES_IN_MENU) {
                DBG_OUT("  not adding, reached max %d items\n", MAX_RECENT_FILES_IN_MENU);
                return;
            }
        }
        curr = curr->next;
    }
}

static HMENU ForceRebuildMenu()
{
    HMENU tmp;
    DestroyCurrentMenu();
    g_currMenu = CreateMenu();
    tmp = BuildMenuFromMenuDef(menuDefFile, dimof(menuDefFile));
    AddRecentFilesToMenu(tmp);
    AppendMenuW(g_currMenu, MF_POPUP | MF_STRING, (UINT_PTR)tmp, _TRW("&File"));
    tmp = BuildMenuFromMenuDef(menuDefView, dimof(menuDefView));
    AppendMenuW(g_currMenu, MF_POPUP | MF_STRING, (UINT_PTR)tmp, _TRW("&View"));
    tmp = BuildMenuFromMenuDef(menuDefGoTo, dimof(menuDefGoTo));
    AppendMenuW(g_currMenu, MF_POPUP | MF_STRING, (UINT_PTR)tmp, _TRW("&Go To"));
    tmp = BuildMenuFromMenuDef(menuDefZoom, dimof(menuDefZoom));
    AppendMenuW(g_currMenu, MF_POPUP | MF_STRING, (UINT_PTR)tmp, _TRW("&Zoom"));
    tmp = BuildMenuFromMenuDef(menuDefLang, dimof(menuDefLang));
    AppendMenuW(g_currMenu, MF_POPUP | MF_STRING, (UINT_PTR)tmp, _TRW("&Language"));
    tmp = BuildMenuFromMenuDef(menuDefHelp, dimof(menuDefHelp));
    AppendMenuW(g_currMenu, MF_POPUP | MF_STRING, (UINT_PTR)tmp, _TRW("&Help"));
    return g_currMenu;
}

static HMENU GetProgramMenu()
{
    if (NULL == g_currMenu)
        ForceRebuildMenu();
    assert(g_currMenu);
    return g_currMenu;
}

/* Return the full exe path of my own executable.
   Caller needs to free() the result. */
static char *ExePathGet(void)
{
    char *cmdline = GetCommandLineA();
    return str_parse_possibly_quoted(&cmdline);
}

/* Set the client area size of the window 'hwnd' to 'dx'/'dy'. */
static void WinResizeClientArea(HWND hwnd, int dx, int dy)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    if ((rect_dx(&rc) == dx) && (rect_dy(&rc) == dy))
        return;
    RECT rw;
    GetWindowRect(hwnd, &rw);
    int win_dx = rect_dx(&rw) + (dx - rect_dx(&rc));
    int win_dy = rect_dy(&rw) + (dy - rect_dy(&rc));
    SetWindowPos(hwnd, NULL, 0, 0, win_dx, win_dy, SWP_NOACTIVATE | SWP_NOREPOSITION | SWP_NOMOVE| SWP_NOZORDER);
}

static void SetCanvasSizeToDxDy(WindowInfo *win, int w, int h)
{
    RECT canvasRect;
    GetWindowRect(win->hwndCanvas, &canvasRect);
    RECT frameRect;
    GetWindowRect(win->hwndFrame, &frameRect);
    int dx = rect_dx(&frameRect) - rect_dx(&canvasRect);
    assert(dx >= 0);
    int dy = rect_dy(&frameRect) - rect_dy(&canvasRect);
    assert(dy >= 0);
    SetWindowPos(win->hwndFrame, NULL, 0, 0, w+dx, h+dy, SWP_NOACTIVATE | SWP_NOREPOSITION | SWP_NOMOVE| SWP_NOZORDER);
    //SetWindowPos(win->hwndCanvas, NULL, 0, 0, w, h, SWP_NOACTIVATE | SWP_NOREPOSITION | SWP_NOMOVE| SWP_NOZORDER);
}

static void CaptionPens_Create(void)
{
    LOGPEN  pen;

    assert(!ghpenWhite);
    pen.lopnStyle = PS_SOLID;
    pen.lopnWidth.x = 1;
    pen.lopnWidth.y = 1;
    pen.lopnColor = COL_WHITE;
    ghpenWhite = CreatePenIndirect(&pen);
    pen.lopnColor = COL_CAPTION_BLUE;
    ghpenBlue = CreatePenIndirect(&pen);
}

static void CaptionPens_Destroy(void)
{
    if (ghpenWhite) {
        DeleteObject(ghpenWhite);
        ghpenWhite = NULL;
    }

    if (ghpenBlue) {
        DeleteObject(ghpenBlue);
        ghpenBlue = NULL;
    }
}

static void AddFileToHistory(const char *filePath)
{
    FileHistoryList *   node;
    uint32_t            oldMenuId = INVALID_MENU_ID;

    assert(filePath);
    if (!filePath) return;

    /* if a history entry with the same name already exists, then delete it.
       That way we don't have duplicates and the file moves to the front of the list */
    node = FileHistoryList_Node_FindByFilePath(&gFileHistoryRoot, filePath);
    if (node) {
        oldMenuId = node->menuId;
        FileHistoryList_Node_RemoveAndFree(&gFileHistoryRoot, node);
    }
    node = FileHistoryList_Node_CreateFromFilePath(filePath);
    if (!node)
        return;
    node->menuId = oldMenuId;
    FileHistoryList_Node_InsertHead(&gFileHistoryRoot, node);
}

extern "C" char *GetPasswordForFile(WindowInfo *win, const char *fileName);

/* Get password for a given 'fileName', can be NULL if user cancelled the
   dialog box.
   Caller needs to free() the result. */
char *GetPasswordForFile(WindowInfo *win, const char *fileName)
{
    fileName = FilePath_GetBaseName(fileName);
    return Dialog_GetPassword(win, fileName);
}

void *StandardSecurityHandler::getAuthData() 
{
    WindowInfo *        win;
    const char *        pwd;
    StandardAuthData *  authData;

    win = (WindowInfo*)doc->getGUIData();
    assert(win);
    if (!win)
        return NULL;

    pwd = GetPasswordForFile(win, doc->getFileName()->getCString());
    if (!pwd)
        return NULL;

    authData = new StandardAuthData(new GooString(pwd), new GooString(pwd));
    free((void*)pwd);
    return (void*)authData;
}

/* Return true if this program has been started from "Program Files" directory
   (which is an indicator that it has been installed */
static bool runningFromProgramFiles(void)
{
    char programFilesDir[MAX_PATH];
    BOOL fOk = SHGetSpecialFolderPath(NULL, (TCHAR*)programFilesDir, CSIDL_PROGRAM_FILES, FALSE); /* @note: TCHAR* cast */
    char *exePath = ExePathGet();
    if (!exePath) return true; // again, assume it is
    bool fromProgramFiles = false;
    if (fOk) {
        if (str_startswithi(exePath, programFilesDir))
            fromProgramFiles = true;
    } else {
        // SHGetSpecialFolderPath() might fail on win95/98 so need a different check
        if (strstr(exePath, "Program Files"))
            fromProgramFiles = true;
    }
    free(exePath);
    return fromProgramFiles;
}

static bool IsRunningInPortableMode(void)
{
    return !runningFromProgramFiles();
}

static void AppGetAppDir(DString* pDs)
{
    char        dir[MAX_PATH];

    SHGetSpecialFolderPath(NULL, (TCHAR*)dir, CSIDL_APPDATA, TRUE); /* @note: TCHAR* cast */
    DStringSprintf(pDs, "%s/%s", dir, APP_SUB_DIR);
    _mkdir(pDs->pString);
}

/* Generate the full path for a filename used by the app in the userdata path. */
static void AppGenDataFilename(char* pFilename, DString* pDs)
{
    assert(0 == pDs->length);
    assert(pFilename);
    if (!pFilename) return;
    assert(pDs);
    if (!pDs) return;

    bool portable = IsRunningInPortableMode();
    if (portable) {
        /* Use the same path as the binary */
        char *exePath = ExePathGet();
        if (!exePath) return;
        char *dir = FilePath_GetDir(exePath);
        if (dir)
            DStringSprintf(pDs, "%s", dir);
        free((void*)exePath);
        free((void*)dir);
    } else {
        AppGetAppDir(pDs);
    }
    if (!str_endswithi(pDs->pString, DIR_SEP_STR) && !(DIR_SEP_CHAR == pFilename[0])) {
        DStringAppend(pDs, DIR_SEP_STR, -1);
    }
    DStringAppend(pDs, pFilename, -1);
}

static void Prefs_GetFileName(DString* pDs)
{
    assert(0 == pDs->length);
    AppGenDataFilename((char*)PREFS_FILE_NAME, pDs); /* @note: char* cast */
}

/* Load preferences from the preferences file. */
static void Prefs_Load(void)
{
    DString             path;
    static bool         loaded = false;
    char *              prefsTxt = NULL;

    assert(!loaded);
    loaded = true;

    DBG_OUT("Prefs_Load()\n");

    DStringInit(&path);
    Prefs_GetFileName(&path);

    size_t prefsFileLen;
    prefsTxt = file_read_all(path.pString, &prefsFileLen);
    if (str_empty(prefsTxt)) {
        DBG_OUT("  no prefs file or is empty\n");
        return;
    }
    DBG_OUT("Prefs file %s:\n%s\n", path.pString, prefsTxt);

    bool fOk = Prefs_Deserialize(prefsTxt, &gFileHistoryRoot);
    assert(fOk);

    DStringFree(&path);
    free((void*)prefsTxt);
}

static struct idToZoomMap {
    UINT id;
    double zoom;
} gZoomMenuItemsId[] = {
    { IDM_ZOOM_6400, 6400.0 },
    { IDM_ZOOM_3200, 3200.0 },
    { IDM_ZOOM_1600, 1600.0 },
    { IDM_ZOOM_800, 800.0 },
    { IDM_ZOOM_400, 400.0 },
    { IDM_ZOOM_200, 200.0 },
    { IDM_ZOOM_150, 150.0 },
    { IDM_ZOOM_125, 125.0 },
    { IDM_ZOOM_100, 100.0 },
    { IDM_ZOOM_50, 50.0 },
    { IDM_ZOOM_25, 25.0 },
    { IDM_ZOOM_12_5, 12.5 },
    { IDM_ZOOM_8_33, 8.33 },
    { IDM_ZOOM_FIT_PAGE, ZOOM_FIT_PAGE },
    { IDM_ZOOM_FIT_WIDTH, ZOOM_FIT_WIDTH },
    { IDM_ZOOM_ACTUAL_SIZE, 100.0 }
};

static UINT MenuIdFromVirtualZoom(double virtualZoom)
{
    for (int i=0; i < dimof(gZoomMenuItemsId); i++) {
        if (virtualZoom == gZoomMenuItemsId[i].zoom)
            return gZoomMenuItemsId[i].id;
    }
    return IDM_ZOOM_ACTUAL_SIZE;
}

static void ZoomMenuItemCheck(HMENU hmenu, UINT menuItemId)
{
    BOOL    found = FALSE;

    for (int i=0; i<dimof(gZoomMenuItemsId); i++) {
        UINT checkState = MF_BYCOMMAND | MF_UNCHECKED;
        if (menuItemId == gZoomMenuItemsId[i].id) {
            assert(!found);
            found = TRUE;
            checkState = MF_BYCOMMAND | MF_CHECKED;
        }
        CheckMenuItem(hmenu, gZoomMenuItemsId[i].id, checkState);
    }
    assert(found);
}

static double ZoomMenuItemToZoom(UINT menuItemId)
{
    for (int i=0; i<dimof(gZoomMenuItemsId); i++) {
        if (menuItemId == gZoomMenuItemsId[i].id) {
            return gZoomMenuItemsId[i].zoom;
        }
    }
    assert(0);
    return 100.0;
}

static void Win32_Win_GetSize(HWND hwnd, int *dxOut, int *dyOut)
{
    RECT    r;
    *dxOut = 0;
    *dyOut = 0;

    if (GetWindowRect(hwnd, &r)) {
        *dxOut = (r.right - r.left);
        *dyOut = (r.bottom - r.top);
    }
}

static void Win32_Win_GetPos(HWND hwnd, int *xOut, int *yOut)
{
    RECT    r;
    *xOut = 0;
    *yOut = 0;

    if (GetWindowRect(hwnd, &r)) {
        *xOut = r.left;
        *yOut = r.top;
    }
}

static void Win32_Win_SetPos(HWND hwnd, int x, int y)
{
    SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE);
}

static void UpdateDisplayStateWindowPos(WindowInfo *win, DisplayState *ds)
{
    int posX, posY;

    Win32_Win_GetPos(win->hwndCanvas, &posX, &posY);

    ds->windowX = posX;
    ds->windowY = posY;
}

static void UpdateCurrentFileDisplayStateForWin(WindowInfo *win)
{
    DisplayState    ds;
    const char *    fileName = NULL;
    FileHistoryList*node = NULL;

    if (!win)
        return;
    if (WS_SHOWING_PDF != win->state)
        return;
    if (!win->dm)
        return;

    fileName = win->dm->fileName();
    assert(fileName);
    if (!fileName)
        return;

    if (!gRunningDLL) 
    {
        node = FileHistoryList_Node_FindByFilePath(&gFileHistoryRoot, fileName);
        assert(node);
        if (!node)
            return;
    }

    DisplayState_Init(&ds);
    if (!displayStateFromDisplayModel(&ds, win->dm))
        return;

    UpdateDisplayStateWindowPos(win, &ds);
    DisplayState_Free(&(node->state));
    node->state = ds;
    node->state.visible = TRUE;
}

static void UpdateCurrentFileDisplayState(void)
{
    WindowInfo *        currWin;
    FileHistoryList *   currFile;

    currFile = gFileHistoryRoot;
    while (currFile) {
        currFile->state.visible = FALSE;
        currFile = currFile->next;
    }

    currWin = gWindowList;
    while (currWin) {
        UpdateCurrentFileDisplayStateForWin(currWin);
        currWin = currWin->next;
    }
}

static void Prefs_Save(void)
{
    DString       path;
    DString       prefsStr;

#if 0
    if (gPrefsSaved)
        return;
    gPrefsSaved = TRUE;
#endif

    DStringInit(&prefsStr);

    /* mark currently shown files as visible */
    UpdateCurrentFileDisplayState();

    bool fOk = Prefs_Serialize(&gFileHistoryRoot, &prefsStr);
    if (!fOk)
        goto Exit;

    DStringInit(&path);
    Prefs_GetFileName(&path);
    DBG_OUT("prefs file=%s\nprefs:\n%s\n", path.pString, prefsStr.pString);
    /* TODO: consider 2-step process:
        * write to a temp file
        * rename temp file to final file */
    write_to_file((TCHAR*)path.pString, (void*)prefsStr.pString, prefsStr.length); /* @note: TCHAR* cast */
    
Exit:
    DStringFree(&prefsStr);
    DStringFree(&path);
}

static bool WindowInfo_Dib_Init(WindowInfo *win) {
    assert(NULL == win->dibInfo);
    win->dibInfo = (BITMAPINFO*)malloc(sizeof(BITMAPINFO) + 12);
    if (!win->dibInfo)
        return false;
    win->dibInfo->bmiHeader.biSize = sizeof(win->dibInfo->bmiHeader);
    win->dibInfo->bmiHeader.biPlanes = 1;
    win->dibInfo->bmiHeader.biBitCount = 24;
    win->dibInfo->bmiHeader.biCompression = BI_RGB;
    win->dibInfo->bmiHeader.biXPelsPerMeter = 2834;
    win->dibInfo->bmiHeader.biYPelsPerMeter = 2834;
    win->dibInfo->bmiHeader.biClrUsed = 0;
    win->dibInfo->bmiHeader.biClrImportant = 0;
    return true;
}

static void WindowInfo_Dib_Deinit(WindowInfo *win) {
    free((void*)win->dibInfo);
    win->dibInfo = NULL;
}

static void WindowInfo_DoubleBuffer_Delete(WindowInfo *win) {
    if (win->bmpDoubleBuffer) {
        DeleteObject(win->bmpDoubleBuffer);
        win->bmpDoubleBuffer = NULL;
    }

    if (win->hdcDoubleBuffer) {
        DeleteDC(win->hdcDoubleBuffer);
        win->hdcDoubleBuffer = NULL;
    }
    win->hdcToDraw = NULL;
}

static bool WindowInfo_DoubleBuffer_New(WindowInfo *win)
{
    WindowInfo_DoubleBuffer_Delete(win);

    win->hdc = GetDC(win->hwndCanvas);
    win->hdcToDraw = win->hdc;
    win->GetCanvasSize();
    if (!gUseDoubleBuffer || (0 == win->winDx()) || (0 == win->winDy()))
        return true;

    win->hdcDoubleBuffer = CreateCompatibleDC(win->hdc);
    if (!win->hdcDoubleBuffer)
        return false;

    win->bmpDoubleBuffer = CreateCompatibleBitmap(win->hdc, win->winDx(), win->winDy());
    if (!win->bmpDoubleBuffer) {
        WindowInfo_DoubleBuffer_Delete(win);
        return false;
    }
    /* TODO: do I need this ? */
    SelectObject(win->hdcDoubleBuffer, win->bmpDoubleBuffer);
    /* fill out everything with background color */
    RECT r = {0};
    r.bottom = win->winDy();
    r.right = win->winDx();
    FillRect(win->hdcDoubleBuffer, &r, gBrushBg);
    win->hdcToDraw = win->hdcDoubleBuffer;
    return TRUE;
}

static void WindowInfo_DoubleBuffer_Show(WindowInfo *win, HDC hdc)
{
    if (win->hdc != win->hdcToDraw) {
        assert(win->hdcToDraw == win->hdcDoubleBuffer);
        BitBlt(hdc, 0, 0, win->winDx(), win->winDy(), win->hdcDoubleBuffer, 0, 0, SRCCOPY);
    }
}

static void WindowInfo_Delete(WindowInfo *win)
{
    if (win->dm) {
        RenderQueue_RemoveForDisplayModel(win->dm);
        cancelRenderingForDisplayModel(win->dm);
    }
    delete win->dm;
    win->dm = NULL;
    WindowInfo_Dib_Deinit(win);
    WindowInfo_DoubleBuffer_Delete(win);
    delete win;
}

static WindowInfo* WindowInfo_FindByHwnd(HWND hwnd)
{
    WindowInfo  *win = gWindowList;
    while (win) {
        if (hwnd == win->hwndFrame)
            return win;
        if (hwnd == win->hwndCanvas)
            return win;
        if (hwnd == win->hwndReBar)
            return win;
        win = win->next;
    }
    return NULL;
}

static WindowInfo *WindowInfo_New(HWND hwndFrame) {
    WindowInfo * win = WindowInfo_FindByHwnd(hwndFrame);
    assert(!win);

    win = new WindowInfo();;
    if (!win)
        return NULL;

    if (!WindowInfo_Dib_Init(win))
        goto Error;

    win->state = WS_ABOUT;
    win->hwndFrame = hwndFrame;
    win->mouseAction = MA_IDLE;
    return win;
Error:
    WindowInfo_Delete(win);
    return NULL;
}

static void WindowInfoList_Add(WindowInfo *win) {
    win->next = gWindowList;
    gWindowList = win;
}

static bool WindowInfoList_ExistsWithError(void) {
    WindowInfo *cur = gWindowList;
    while (cur) {
        if (WS_ERROR_LOADING_PDF == cur->state)
            return true;
        cur = cur->next;
    }
    return false;
}

static void WindowInfoList_Remove(WindowInfo *to_remove) {
    assert(to_remove);
    if (!to_remove)
        return;
    if (gWindowList == to_remove) {
        gWindowList = to_remove->next;
        return;
    }
    WindowInfo* curr = gWindowList;
    while (curr) {
        if (to_remove == curr->next) {
            curr->next = to_remove->next;
            return;
        }
        curr = curr->next;
    }
}

static void WindowInfoList_DeleteAll(void) {
    WindowInfo* curr = gWindowList;
    while (curr) {
        WindowInfo* next = curr->next;
        WindowInfo_Delete(curr);
        curr = next;
    }
    gWindowList = NULL;
}

static int WindowInfoList_Len(void) {
    int len = 0;
    WindowInfo* curr = gWindowList;
    while (curr) {
        ++len;
        curr = curr->next;
    }
    return len;
}

static void WindowInfo_RedrawAll(WindowInfo *win, bool update=false) {
    InvalidateRect(win->hwndCanvas, NULL, false);
    if (update)
        UpdateWindow(win->hwndCanvas);
}

static bool FileCloseMenuEnabled(void) {
    WindowInfo* win = gWindowList;
    while (win) {
        if (win->state == WS_SHOWING_PDF)
            return true;
        win = win->next;
    }
    return false;
}

static void ToolbarUpdateStateForWindow(WindowInfo *win) {
    LPARAM enable = (LPARAM)MAKELONG(1,0);
    LPARAM disable = (LPARAM)MAKELONG(0,0);

    for (int i=0; i < TOOLBAR_BUTTONS_COUNT; i++) {
        int cmdId = gToolbarButtons[i].cmdId;
        if (IDB_SEPARATOR == cmdId)
            continue;
        LPARAM buttonState = enable;
        if (IDM_OPEN != cmdId) {
            if (WS_SHOWING_PDF != win->state)
                buttonState = disable;
        }
        SendMessage(win->hwndToolbar, TB_ENABLEBUTTON, cmdId, buttonState);
    }
}

static void MenuUpdateShowToolbarStateForWindow(WindowInfo *win) {
    HMENU hmenu = GetMenu(win->hwndFrame);
    if (gShowToolbar)
        CheckMenuItem(hmenu, IDM_VIEW_SHOW_HIDE_TOOLBAR, MF_BYCOMMAND | MF_CHECKED);
    else
        CheckMenuItem(hmenu, IDM_VIEW_SHOW_HIDE_TOOLBAR, MF_BYCOMMAND | MF_UNCHECKED);
}

static void MenuUpdateUseFitzStateForWindow(WindowInfo *win) {
    HMENU hmenu = GetMenu(win->hwndFrame);
    if (gUseFitz)
        CheckMenuItem(hmenu, IDM_VIEW_USE_FITZ, MF_BYCOMMAND | MF_CHECKED);
    else
        CheckMenuItem(hmenu, IDM_VIEW_USE_FITZ, MF_BYCOMMAND | MF_UNCHECKED);
}

// show which language is being used via check in Language/* menu
static void MenuUpdateLanguage(WindowInfo *win) {
    HMENU hmenu = GetMenu(win->hwndFrame);
    for (int i = 0; i < LANGS_COUNT; i++) {
        const char *langName = g_langs[i]._langName;
        int langMenuId = g_langs[i]._langId;
        if (str_eq(CurrLangNameGet(), langName))
            CheckMenuItem(hmenu, langMenuId, MF_BYCOMMAND | MF_CHECKED);
        else
            CheckMenuItem(hmenu, langMenuId, MF_BYCOMMAND | MF_UNCHECKED);
    }
}

static void MenuUpdateStateForWindow(WindowInfo *win) {
    static UINT menusToDisableIfNoPdf[] = {
        IDM_VIEW_SINGLE_PAGE, IDM_VIEW_FACING, IDM_VIEW_CONTINUOUS, IDM_VIEW_CONTINUOUS_FACING,
        IDM_VIEW_ROTATE_LEFT, IDM_VIEW_ROTATE_RIGHT, IDM_GOTO_NEXT_PAGE, IDM_GOTO_PREV_PAGE,
        IDM_GOTO_FIRST_PAGE, IDM_GOTO_LAST_PAGE, IDM_GOTO_PAGE, IDM_ZOOM_FIT_PAGE,
        IDM_ZOOM_ACTUAL_SIZE, IDM_ZOOM_FIT_WIDTH, IDM_ZOOM_6400, IDM_ZOOM_3200,
        IDM_ZOOM_1600, IDM_ZOOM_800, IDM_ZOOM_400, IDM_ZOOM_200, IDM_ZOOM_150,
        IDM_ZOOM_125, IDM_ZOOM_100, IDM_ZOOM_50, IDM_ZOOM_25, IDM_ZOOM_12_5,
        IDM_ZOOM_8_33, IDM_SAVEAS };

    bool fileCloseEnabled = FileCloseMenuEnabled();
    HMENU hmenu = GetMenu(win->hwndFrame);
    if (fileCloseEnabled)
        EnableMenuItem(hmenu, IDM_CLOSE, MF_BYCOMMAND | MF_ENABLED);
    else
        EnableMenuItem(hmenu, IDM_CLOSE, MF_BYCOMMAND | MF_GRAYED);

    bool filePrintEnabled = false;
    if (win->dm && win->dm->pdfEngine() && win->dm->pdfEngine()->printingAllowed())
        filePrintEnabled = true;
    if (filePrintEnabled)
        EnableMenuItem(hmenu, IDM_PRINT, MF_BYCOMMAND | MF_ENABLED);
    else
        EnableMenuItem(hmenu, IDM_PRINT, MF_BYCOMMAND | MF_GRAYED);

    MenuUpdateShowToolbarStateForWindow(win);
    MenuUpdateUseFitzStateForWindow(win);
    MenuUpdateLanguage(win);

    for (int i = 0; i < dimof(menusToDisableIfNoPdf); i++) {
        UINT menuId = menusToDisableIfNoPdf[i];
        if (WS_SHOWING_PDF == win->state)
            EnableMenuItem(hmenu, menuId, MF_BYCOMMAND | MF_ENABLED);
        else
            EnableMenuItem(hmenu, menuId, MF_BYCOMMAND | MF_GRAYED);
    }
    /* Hide scrollbars if not showing a PDF */
    /* TODO: doesn't really fit the name of the function */
    if (WS_SHOWING_PDF == win->state)
        ShowScrollBar(win->hwndCanvas, SB_BOTH, TRUE);
    else {
        ShowScrollBar(win->hwndCanvas, SB_BOTH, FALSE);
        win_set_text(win->hwndFrame, APP_NAME);
    }
}

/* Disable/enable menu items and toolbar buttons depending on wheter a
   given window shows a PDF file or not. */
static void MenuToolbarUpdateStateForAllWindows(void) {
    WindowInfo* win = gWindowList;
    while (win) {
        MenuUpdateStateForWindow(win);
        ToolbarUpdateStateForWindow(win);
        win = win->next;
    }
}

static WindowInfo* WindowInfo_CreateEmpty(void) {
    HWND        hwndFrame, hwndCanvas;
    WindowInfo* win;

#if FANCY_UI
    hwndFrame = CreateWindowEx(
//            WS_EX_TOOLWINDOW,
        0,
//            WS_OVERLAPPEDWINDOW,
//            WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE,
        //WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_HSCROLL | WS_VSCROLL,
        FRAME_CLASS_NAME, windowTitle,
        WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT,
        DEF_WIN_DX, DEF_WIN_DY,
        NULL, NULL,
        ghinst, NULL);
#else
    hwndFrame = CreateWindow(
            FRAME_CLASS_NAME, windowTitle,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            DEF_WIN_DX, DEF_WIN_DY,
            NULL, NULL,
            ghinst, NULL);
    HMENU m = GetProgramMenu();
    SetMenu(hwndFrame, m);
#endif

    if (!hwndFrame)
        return NULL;

    win = WindowInfo_New(hwndFrame);
    hwndCanvas = CreateWindow(
            CANVAS_CLASS_NAME, NULL,
            WS_CHILD | WS_HSCROLL | WS_VSCROLL,
            CW_USEDEFAULT, CW_USEDEFAULT,
            DEF_WIN_DX, DEF_WIN_DY,
            hwndFrame, NULL,
            ghinst, NULL);
    if (!hwndCanvas)
        return NULL;
    win->hwndCanvas = hwndCanvas;
    CreateToolbar(win, ghinst);
    return win;
}

BOOL GetDesktopWindowClientRect(RECT *r)
{
    HWND hwnd = GetDesktopWindow();
    if (!hwnd) return FALSE;
    return GetClientRect(hwnd, r);
}

void GetCanvasDxDyDiff(WindowInfo *win, int *dxOut, int *dyOut)
{
    RECT canvasRect;
    GetWindowRect(win->hwndCanvas, &canvasRect);
    RECT totalRect;
    GetWindowRect(win->hwndFrame, &totalRect);
    *dxOut = rect_dx(&totalRect) - rect_dx(&canvasRect);
    // TODO: should figure out why it fires in DLL
    assert(gRunningDLL || *dxOut >= 0);
    *dyOut = rect_dy(&totalRect) - rect_dy(&canvasRect);
    // TODO: should figure out why it fires in DLL
    assert(gRunningDLL || *dyOut >= 0);
}

SizeI GetMaxCanvasSize(WindowInfo *win)
{
    AppBarData abd;
    RECT r;
    GetDesktopWindowClientRect(&r);
    // substract the area of the window not used for canvas
    int dx, dy;
    GetCanvasDxDyDiff(win, &dx, &dy);  // TODO: lame name
    int maxCanvasDx = rect_dx(&r) - dx;
    int maxCanvasDy = rect_dy(&r) - dy;
    if (abd.isHorizontal()) {
        assert(maxCanvasDx >= abd.dx());
        maxCanvasDx -= abd.dx();
    } else {
        assert(abd.isVertical());
        assert(maxCanvasDy >= abd.dy());
        maxCanvasDy -= abd.dy();
    }
    return SizeI(maxCanvasDx, maxCanvasDy);
}


static void RecalcSelectionPosition (WindowInfo *win) {
    SelectionOnPage *   selOnPage = win->selectionOnPage;
    RectD               selD;
    PdfPageInfo*        pageInfo;

    while (selOnPage != NULL) {
        pageInfo = win->dm->getPageInfo(selOnPage->pageNo);
        /* if page is not visible, we hide seletion by simply moving it off
         * the canvas */
        if (!pageInfo->visible) {
            selOnPage->selectionCanvas.x = -100;
            selOnPage->selectionCanvas.y = -100;
            selOnPage->selectionCanvas.dx = 0;
            selOnPage->selectionCanvas.dy = 0;
        } else {//page is visible
            RectD_Copy (&selD, &selOnPage->selectionPage);
            win->dm->rectCvtUserToScreen (selOnPage->pageNo, &selD);
            RectI_FromRectD (&selOnPage->selectionCanvas, &selD);
        }
        selOnPage = selOnPage->next;
    }
}

static WindowInfo* LoadPdf(const char *fileName, bool ignoreHistorySizePos = true, bool ignoreHistory = false)
{
    assert(fileName);
    if (!fileName) return NULL;

    FileHistoryList *   fileFromHistory = NULL;
    if (!ignoreHistory)
        fileFromHistory = FileHistoryList_Node_FindByFilePath(&gFileHistoryRoot, fileName);

    WindowInfo *        win;
    bool reuseExistingWindow = false;
    if ((1 == WindowInfoList_Len()) && (WS_SHOWING_PDF != gWindowList->state)) {
        win = gWindowList;
        reuseExistingWindow = true;
    } else {
        win = WindowInfo_CreateEmpty();
        if (!win)
            return NULL;
     }

    win->GetCanvasSize();
    SizeI maxCanvasSize = GetMaxCanvasSize(win);
    SizeD totalDrawAreaSize(win->winSize());
    if (fileFromHistory && !ignoreHistorySizePos) {
        WinResizeClientArea(win->hwndCanvas, fileFromHistory->state.windowDx, fileFromHistory->state.windowDy);
        totalDrawAreaSize = SizeD(fileFromHistory->state.windowDx, fileFromHistory->state.windowDy);
        Win32_Win_SetPos(win->hwndFrame, fileFromHistory->state.windowX, fileFromHistory->state.windowY);
    }
#if 0 // not ready yet
    else {
        IntelligentWindowResize(win);
    }
#endif

    /* TODO: make sure it doesn't have a stupid position like 
       outside of the screen etc. */
#if 0
    if (totalDrawAreaSize.dxI() > maxCanvasSize.dx)
        totalDrawAreaSize.setDx(maxCanvasSize.dx);
    if (totalDrawAreaSize.dyI() > maxCanvasSize.dy)
        totalDrawAreaSize.setDy(maxCanvasSize.dy);

    WinResizeClientArea(win->hwndCanvas, totalDrawAreaSize.dxI(), totalDrawAreaSize.dyI());
#endif

    /* In theory I should get scrollbars sizes using Win32_GetScrollbarSize(&scrollbarYDx, &scrollbarXDy);
       but scrollbars are not part of the client area on windows so it's better
       not to have them taken into account by DisplayModelSplash code.
       TODO: I think it's broken anyway and DisplayModelSplash needs to know if
             scrollbars are part of client area in order to accomodate windows
             UI properly */
    DisplayMode displayMode = DEFAULT_DISPLAY_MODE;
    int offsetX = 0;
    int offsetY = 0;
    int startPage = 1;
    int scrollbarYDx = 0;
    int scrollbarXDy = 0;
    if (fileFromHistory) {
        startPage = fileFromHistory->state.pageNo;
        displayMode = fileFromHistory->state.displayMode;
        offsetX = fileFromHistory->state.scrollX;
        offsetY = fileFromHistory->state.scrollY;
    }

    if (gUseFitz) {
        win->dm = DisplayModelFitz_CreateFromFileName(fileName, 
            totalDrawAreaSize, scrollbarYDx, scrollbarXDy, displayMode, startPage, win);
    } else {
        win->dm = DisplayModelSplash_CreateFromFileName(fileName, 
            totalDrawAreaSize, scrollbarYDx, scrollbarXDy, displayMode, startPage, win);
    }

    double zoomVirtual = DEFAULT_ZOOM;
    int rotation = DEFAULT_ROTATION;
    if (!win->dm) {
        if (!reuseExistingWindow && WindowInfoList_ExistsWithError()) {
                /* don't create more than one window with errors */
                WindowInfo_Delete(win);
                return NULL;
        }
        win->state = WS_ERROR_LOADING_PDF;
        DBG_OUT("failed to load file %s\n", fileName);
        //goto Exit; /* @note: because of "crosses initialization of [...]" */
    	if (!reuseExistingWindow)
       		WindowInfoList_Add(win);
    	MenuToolbarUpdateStateForAllWindows();
   	assert(win);
    	DragAcceptFiles(win->hwndFrame, TRUE);
    	DragAcceptFiles(win->hwndCanvas, TRUE);
    	ShowWindow(win->hwndFrame, SW_SHOW);
    	ShowWindow(win->hwndCanvas, SW_SHOW);
    	UpdateWindow(win->hwndFrame);
    	UpdateWindow(win->hwndCanvas);
   	return win;
    }

    win->dm->setAppData((void*)win);

    if (!fileFromHistory) {
        AddFileToHistory(fileName);
        RebuildProgramMenus();
    }

    /* TODO: if fileFromHistory, set the state based on gFileHistoryList node for
       this entry */
    win->state = WS_SHOWING_PDF;
    if (fileFromHistory) {
        zoomVirtual = fileFromHistory->state.zoomVirtual;
        rotation = fileFromHistory->state.rotation;
    }

    UINT menuId = MenuIdFromVirtualZoom(zoomVirtual);
    ZoomMenuItemCheck(GetMenu(win->hwndFrame), menuId);

    win->dm->relayout(zoomVirtual, rotation);
    if (!win->dm->validPageNo(startPage))
        startPage = 1;
    /* TODO: need to calculate proper offsetY, currently giving large offsetY
       remembered for continuous mode breaks things (makes all pages invisible) */
    offsetY = 0;
    /* TODO: make sure offsetX isn't bogus */
    win->dm->goToPage(startPage, offsetY, offsetX);

    /* only resize the window if it's a newly opened window */
    if (!reuseExistingWindow && !fileFromHistory)
        WindowInfo_ResizeToPage(win, startPage);

    if (reuseExistingWindow)
        WindowInfo_RedrawAll(win);

Exit:
    if (!reuseExistingWindow)
        WindowInfoList_Add(win);
    MenuToolbarUpdateStateForAllWindows();
    assert(win);
    DragAcceptFiles(win->hwndFrame, TRUE);
    DragAcceptFiles(win->hwndCanvas, TRUE);
    ShowWindow(win->hwndFrame, SW_SHOW);
    ShowWindow(win->hwndCanvas, SW_SHOW);
    UpdateWindow(win->hwndFrame);
    UpdateWindow(win->hwndCanvas);
    return win;
}

static HFONT Win32_Font_GetSimple(HDC hdc, char *fontName, int fontSize)
{
    HFONT       font_dc;
    HFONT       font;
    LOGFONT     lf = {0};

    font_dc = (HFONT)GetStockObject(SYSTEM_FONT);
    if (!GetObject(font_dc, sizeof(LOGFONT), &lf))
        return NULL;

    lf.lfHeight = (LONG)-fontSize;
    lf.lfWidth = 0;
    //lf.lfHeight = -MulDiv(fontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    lf.lfItalic = FALSE;
    lf.lfUnderline = FALSE;
    lf.lfStrikeOut = FALSE;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_TT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    //lf.lfQuality = CLEARTYPE_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH;    
    strcpy_s((char*)lf.lfFaceName, LF_FACESIZE, fontName); /* @note: char* cast */
    lf.lfWeight = FW_DONTCARE;
    font = CreateFontIndirect(&lf);
    return font;
}

static void Win32_Font_Delete(HFONT font)
{
    DeleteObject(font);
}

void DisplayModel::pageChanged(void)
{
    WindowInfo *win = (WindowInfo*)appData();
    assert(win);
    if (!win) return;

#if 0
    if (!win->dmSplash->pdfDoc)
        return;
#endif

    int currPageNo = currentPageNo();
    int pageCount = win->dm->pageCount();
    const char *baseName = FilePath_GetBaseName(win->dm->fileName());
    if (pageCount <= 0)
        win_set_text(win->hwndFrame, (TCHAR*)baseName); /* @note: TCHAR* cast */
    else {
        char titleBuf[256];
        HRESULT hr = StringCchPrintfA(titleBuf, dimof(titleBuf), "%s page %d of %d", baseName, currPageNo, pageCount);
        win_set_text(win->hwndFrame, (TCHAR*)titleBuf); /* @note: TCHAR* cast */
	/* @note: FIXME: hr is unused ATM */
    }
}

/* Call from non-UI thread to cause repainting of the display */
static void triggerRepaintDisplayPotentiallyDelayed(WindowInfo *win, bool delayed)
{
    assert(win);
    if (!win) return;
    if (delayed)
        PostMessage(win->hwndCanvas, WM_APP_REPAINT_DELAYED, 0, 0);
    else
        PostMessage(win->hwndCanvas, WM_APP_REPAINT_NOW, 0, 0);
}

static void triggerRepaintDisplayNow(WindowInfo* win)
{
    triggerRepaintDisplayPotentiallyDelayed(win, false);
}

void DisplayModel::repaintDisplay(bool delayed)
{
    WindowInfo* win = (WindowInfo*)appData();
    triggerRepaintDisplayPotentiallyDelayed(win, delayed);
}

void DisplayModel::setScrollbarsState(void)
{
    WindowInfo *win = (WindowInfo*)this->appData();
    assert(win);
    if (!win) return;

    SCROLLINFO      si = {0};
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;

    int canvasDx = _canvasSize.dxI();
    int canvasDy = _canvasSize.dyI();
    int drawAreaDx = drawAreaSize.dxI();
    int drawAreaDy = drawAreaSize.dyI();

    if (drawAreaDx >= canvasDx) {
        si.nPos = 0;
        si.nMin = 0;
        si.nMax = 99;
        si.nPage = 100;
    } else {
        si.nPos = (int)areaOffset.x;
        si.nMin = 0;
        si.nMax = canvasDx-1;
        si.nPage = drawAreaDx;
    }
    SetScrollInfo(win->hwndCanvas, SB_HORZ, &si, TRUE);

    if (drawAreaDy >= canvasDy) {
        si.nPos = 0;
        si.nMin = 0;
        si.nMax = 99;
        si.nPage = 100;
    } else {
        si.nPos = (int)areaOffset.y;
        si.nMin = 0;
        si.nMax = canvasDy-1;
        si.nPage = drawAreaDy;
    }
    SetScrollInfo(win->hwndCanvas, SB_VERT, &si, TRUE);
}

static void WindowInfo_ResizeToWindow(WindowInfo *win)
{
    assert(win);
    if (!win) return;
    assert(win->dm);
    if (!win->dm) return;

    win->dm->changeTotalDrawAreaSize(win->winSize());
}

void WindowInfo_ResizeToPage(WindowInfo *win, int pageNo)
{
    bool fullScreen = false;

    assert(win);
    if (!win) return;
    assert(win->dm);
    if (!win->dm)
        return;

    /* TODO: should take current monitor into account? */
    HDC hdc = GetDC(win->hwndCanvas);
    int displayDx = GetDeviceCaps(hdc, HORZRES);
    int displayDy = GetDeviceCaps(hdc, VERTRES);

    int  dx, dy;
    if (fullScreen) {
        /* TODO: fullscreen not yet supported */
        assert(0);
        dx = displayDx;
        dy = displayDy;
    } else {
        assert(win->dm->validPageNo(pageNo));
        if (!win->dm->validPageNo(pageNo))
            return;
        PdfPageInfo *pageInfo = win->dm->getPageInfo(pageNo);
        assert(pageInfo);
        if (!pageInfo)
            return;
        DisplaySettings *displaySettings = globalDisplaySettings();
        dx = pageInfo->currDx + displaySettings->paddingPageBorderLeft + displaySettings->paddingPageBorderRight;
        dy = pageInfo->currDy + displaySettings->paddingPageBorderTop + displaySettings->paddingPageBorderBottom;
        if (dx > displayDx - 10)
            dx = displayDx - 10;
        if (dy > displayDy - 10)
            dy = displayDy - 10;
    }

    WinResizeClientArea(win->hwndCanvas, dx, dy);
}

static void WindowInfo_ToggleZoom(WindowInfo *win)
{
    DisplayModel *  dm;

    assert(win);
    if (!win) return;

    dm = win->dm;
    assert(dm);
    if (!dm) return;

    if (ZOOM_FIT_PAGE == dm->zoomVirtual())
        dm->setZoomVirtual(ZOOM_FIT_WIDTH);
    else if (ZOOM_FIT_WIDTH == dm->zoomVirtual())
        dm->setZoomVirtual(ZOOM_FIT_PAGE);
}

static BOOL WindowInfo_PdfLoaded(WindowInfo *win)
{
    assert(win);
    if (!win) return FALSE;
    if (!win->dm) return FALSE;
#if 0
    assert(win->dmSplash->pdfDoc);
    assert(win->dmSplash->pdfDoc->isOk());
#endif
    return TRUE;
}

int WindowsVerMajor()
{
    DWORD version = GetVersion();
    return (int)(version & 0xFF);
}

int WindowsVerMinor()
{
    DWORD version = GetVersion();
    return (int)((version & 0xFF00) >> 8);    
}

bool WindowsVer2000OrGreater()
{
    if (WindowsVerMajor() >= 5)
        return true;
    return false;
}

static bool AlreadyRegisteredForPdfExtentions(void)
{
    bool    registered = false;
    HKEY    key = NULL;
    char    nameBuf[sizeof(APP_NAME)+8];
    DWORD   cbNameBuf = sizeof(nameBuf);
    DWORD   keyType;

    /* HKEY_CLASSES_ROOT\.pdf */
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_CLASSES_ROOT, TEXT(".pdf"), 0, KEY_QUERY_VALUE, &key)) /* @note: TEXT() cast */
        return false;

    if (ERROR_SUCCESS != RegQueryValueEx(key, NULL, NULL, &keyType, (LPBYTE)nameBuf, &cbNameBuf))
        goto Exit;

    if (REG_SZ != keyType)
        goto Exit;

    if (cbNameBuf != sizeof(APP_NAME))
        goto Exit;

    if (0 == memcmp(APP_NAME, nameBuf, sizeof(APP_NAME)))
        registered = true;

Exit:
    RegCloseKey(key);
    return registered;
}

static void AssociateExeWithPdfExtensions()
{
    char        tmp[256];
    HKEY        key = NULL, kicon = NULL, kshell = NULL, kopen = NULL, kcmd = NULL;
    DWORD       disp;
    HRESULT     hr;

    char * exePath = ExePathGet();
    assert(exePath);
    if (!exePath) return;

    HKEY    hkeyToUse = HKEY_CURRENT_USER;
    if (WindowsVer2000OrGreater())
        hkeyToUse = HKEY_LOCAL_MACHINE;

    /* key\.pdf */                                              /* @note: TEXT() && TCHAR* casts */
    if (RegCreateKeyEx(hkeyToUse,
                TEXT(".pdf"), 0, NULL, REG_OPTION_NON_VOLATILE,
                KEY_WRITE, NULL, &key, &disp))
        goto Exit;

    if (RegSetValueEx(key, TEXT(""), 0, REG_SZ, (const BYTE*)APP_NAME, sizeof(APP_NAME)))
        goto Exit;

    RegCloseKey(key);
    key = NULL;

    /* key\APP_NAME */
    if (RegCreateKeyEx(hkeyToUse,
                APP_NAME, 0, NULL, REG_OPTION_NON_VOLATILE,
                KEY_WRITE, NULL, &key, &disp))
        goto Exit;

    if (RegSetValueEx(key, TEXT(""), 0, REG_SZ, (const BYTE*)PDF_DOC_NAME, sizeof(PDF_DOC_NAME)))
        goto Exit;

    /* key\APP_NAME\DefaultIcon */
    if (RegCreateKeyEx(key,
                TEXT("DefaultIcon"), 0, NULL, REG_OPTION_NON_VOLATILE,
                KEY_WRITE, NULL, &kicon, &disp))
        goto Exit;

    /* Note: I don't understand why icon index has to be 0, but it just has to */
    hr = StringCchPrintfA(tmp, dimof(tmp), "%s,0", exePath);
    if (RegSetValueEx(kicon, TEXT(""), 0, REG_SZ, (const BYTE*)tmp, strlen(tmp)+1))
        goto Exit;

    RegCloseKey(kicon);
    kicon = NULL;

    /* HKEY_CLASSES_ROOT\APP_NAME\Shell\Open\Command */
    if (RegCreateKeyEx(key,
                TEXT("shell"), 0, NULL, REG_OPTION_NON_VOLATILE,
                KEY_WRITE, NULL, &kshell, &disp))
        goto Exit;

    if (RegCreateKeyEx(kshell,
                TEXT("open"), 0, NULL, REG_OPTION_NON_VOLATILE,
                KEY_WRITE, NULL, &kopen, &disp))
        goto Exit;

    if (RegCreateKeyEx(kopen,
                TEXT("command"), 0, NULL, REG_OPTION_NON_VOLATILE,
                KEY_WRITE, NULL, &kcmd, &disp))
        goto Exit;

    hr = StringCchPrintfA(tmp,  dimof(tmp), "\"%s\" \"%%1\"", exePath);
    if (RegSetValueEx(kcmd, TEXT(""), 0, REG_SZ, (const BYTE*)tmp, strlen(tmp)+1))
        goto Exit;

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST | SHCNF_FLUSHNOWAIT, 0, 0);

Exit:
    if (kcmd)
        RegCloseKey(kcmd);
    if (kopen)
        RegCloseKey(kopen);
    if (kshell)
        RegCloseKey(kshell);
    if (key)
        RegCloseKey(key);
    free(exePath);
}

static void RegisterForPdfExtentions(HWND hwnd)
{
    if (AlreadyRegisteredForPdfExtentions())
        return;

    if (IsRunningInPortableMode())
        return;

    /* Ask user for permission, unless he previously said he doesn't want to
       see this dialog */
    if (!gPdfAssociateDontAskAgain) {
        int result = Dialog_PdfAssociate(hwnd, &gPdfAssociateDontAskAgain);
        if (DIALOG_NO_PRESSED == result) {
            gPdfAssociateShouldAssociate = FALSE;
        } else {
            assert(DIALOG_OK_PRESSED == result);
            gPdfAssociateShouldAssociate = TRUE;
        }
    }

    if (gPdfAssociateShouldAssociate)
        AssociateExeWithPdfExtensions();
}

static void OnDropFiles(WindowInfo *win, HDROP hDrop)
{
    int         i;
    char        filename[MAX_PATH];
    const int   files_count = DragQueryFile(hDrop, DRAGQUERY_NUMFILES, 0, 0);

    for (i = 0; i < files_count; i++)
    {
        DragQueryFile(hDrop, i, (TCHAR*)filename, MAX_PATH); /* @note: TCHAR* cast */
        LoadPdf(filename);
    }
    DragFinish(hDrop);

    if (files_count > 0)
        WindowInfo_RedrawAll(win);
}

static void DrawLineSimple(HDC hdc, int sx, int sy, int ex, int ey)
{
    MoveToEx(hdc, sx, sy, NULL);
    LineTo(hdc, ex, ey);
}

#if 0
/* Draw caption area for a given window 'win' in the classic AmigaOS style */
static void AmigaCaptionDraw(WindowInfo *win)
{
    HGDIOBJ prevPen;
    HDC     hdc = win->hdc;

    assert(VS_AMIGA == gVisualStyle);

    prevPen = SelectObject(hdc, ghpenWhite);

    /* white */
    DrawLineSimple(hdc, 0, 0, win->winDx, 0);
    DrawLineSimple(hdc, 0, 1, win->winDx, 1);

    /* white */
    DrawLineSimple(hdc, 0, 4, win->winDx, 4);
    DrawLineSimple(hdc, 0, 5, win->winDx, 5);

    /* white */
    DrawLineSimple(hdc, 0, 8, win->winDx, 8);
    DrawLineSimple(hdc, 0, 9, win->winDx, 9);

    /* white */
    DrawLineSimple(hdc, 0, 12, win->winDx, 12);
    DrawLineSimple(hdc, 0, 13, win->winDx, 13);

    /* white */
    DrawLineSimple(hdc, 0, 16, win->winDx, 16);
    DrawLineSimple(hdc, 0, 17, win->winDx, 17);
    DrawLineSimple(hdc, 0, 18, win->winDx, 18);

    SelectObject(hdc, ghpenBlue);

    /* blue */
    DrawLineSimple(hdc, 0, 2, win->winDx, 2);
    DrawLineSimple(hdc, 0, 3, win->winDx, 3);

    /* blue */
    DrawLineSimple(hdc, 0, 6, win->winDx, 6);
    DrawLineSimple(hdc, 0, 7, win->winDx, 7);

    /* blue */
    DrawLineSimple(hdc, 0, 10, win->winDx, 10);
    DrawLineSimple(hdc, 0, 11, win->winDx, 11);

    /* blue */
    DrawLineSimple(hdc, 0, 14, win->winDx, 14);
    DrawLineSimple(hdc, 0, 15, win->winDx, 15);

    SelectObject(hdc, prevPen);
}
#endif

static void WinResizeIfNeeded(WindowInfo *win, bool resizeWindow=true)
{
    RECT    rc;
    GetClientRect(win->hwndCanvas, &rc);
    int win_dx = rect_dx(&rc);
    int win_dy = rect_dy(&rc);

    if (win->hdcToDraw &&
        (win_dx == win->winDx()) &&
        (win_dy == win->winDy()))
    {
        return;
    }

    WindowInfo_DoubleBuffer_New(win);
    if (resizeWindow)
        WindowInfo_ResizeToWindow(win);
}

static void PostBenchNextAction(HWND hwnd)
{
    PostMessage(hwnd, MSG_BENCH_NEXT_ACTION, 0, 0);
}

static void OnBenchNextAction(WindowInfo *win)
{
    if (!win->dm)
        return;

    if (win->dm->goToNextPage(0))
        PostBenchNextAction(win->hwndFrame);
}

static void DrawCenteredText(HDC hdc, RECT *r, char *txt)
{    
    SetBkMode(hdc, TRANSPARENT);
    DrawText(hdc, (TCHAR*)txt, strlen(txt), r, DT_CENTER | DT_VCENTER | DT_SINGLELINE); /* @note: TCHAR* cast */
}

static void SeeLastError(void) {
    char *msgBuf = NULL;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR) &msgBuf, 0, NULL);
    if (!msgBuf) return;
    printf("SeeLastError(): %s\n", msgBuf);
    OutputDebugStringA(msgBuf);
    LocalFree(msgBuf);
}

static void PaintTransparentRectangle(WindowInfo *win, HDC hdc, RectI *rect) {
    HBITMAP hbitmap;       // bitmap handle
    BITMAPINFO bmi;        // bitmap header
    VOID *pvBits;          // pointer to DIB section
    BLENDFUNCTION bf;      // structure for alpha blending
    HDC rectDC = CreateCompatibleDC(hdc);
    const DWORD selectionColorYellow = 0xfff5fc0c;
    const DWORD selectionColorBlack = 0xff000000;
    const int margin = 1;

    ZeroMemory(&bmi, sizeof(BITMAPINFO));

    bmi.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = rect->dx;
    bmi.bmiHeader.biHeight = rect->dy;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = rect->dx * rect->dy * 4;

    hbitmap = CreateDIBSection (rectDC, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0x0);
    SelectObject(rectDC, hbitmap);

    for (int y = 0; y < rect->dy; y++) {
        for (int x = 0; x < rect->dx; x++) {
            if (x < margin || x > rect->dx - margin - 1 
                    || y < margin || y > rect->dy - margin - 1)
                ((UINT32 *)pvBits)[x + y * rect->dx] = selectionColorBlack;
            else
                ((UINT32 *)pvBits)[x + y * rect->dx] = selectionColorYellow;
        }
    }
    bf.BlendOp = AC_SRC_OVER;
    bf.BlendFlags = 0;
    bf.SourceConstantAlpha = 0x5f;
    bf.AlphaFormat = AC_SRC_ALPHA;

    /*if (!AlphaBlend(hdc, rect->x, rect->y, rect->dx, rect->dy,
        rectDC, 0, 0, rect->dx, rect->dy, bf))
        DBG_OUT("AlphaBlending error\n");*/	
    /* @note: error: 'AlphaBlend' was not declared in this scope; even with WINVER: 0x0500 set in rbuild file, weird; @FIXME */
    DeleteObject (hbitmap);
    DeleteDC (rectDC);
}

static void PaintSelection (WindowInfo *win, HDC hdc) {
    if (win->mouseAction == MA_SELECTING) {
        // during selecting
        RectI selRect;

        selRect.x = min (win->selectionRect.x, 
            win->selectionRect.x + win->selectionRect.dx);
        selRect.y = min (win->selectionRect.y, 
            win->selectionRect.y + win->selectionRect.dy);
        selRect.dx = abs (win->selectionRect.dx);
        selRect.dy = abs (win->selectionRect.dy);

        if (selRect.dx != 0 && selRect.dy != 0)
            PaintTransparentRectangle (win, hdc, &selRect);
    } else {
        // after selection is done
        SelectionOnPage *selOnPage = win->selectionOnPage;
        // TODO: Move recalcing to better place
        RecalcSelectionPosition(win);
        while (selOnPage != NULL) {
            if (selOnPage->selectionCanvas.dx != 0 && selOnPage->selectionCanvas.dy != 0)
                PaintTransparentRectangle(win, hdc, &selOnPage->selectionCanvas);
            selOnPage = selOnPage->next;
        }
    }
}

static void WindowInfo_Paint(WindowInfo *win, HDC hdc, PAINTSTRUCT *ps)
{
    RECT                bounds;
    RenderedBitmap *    renderedBmp = NULL;

    assert(win);
    if (!win) return;
    DisplayModel* dm = win->dm;
    assert(dm);
    if (!dm) return;
#if 0 // TODO: write the equivalent dm->isOk() ?
    assert(dm->pdfDoc);
    if (!dm->pdfDoc) return;
#endif

    assert(win->hdcToDraw);
    hdc = win->hdcToDraw;

    FillRect(hdc, &(ps->rcPaint), gBrushBg);

    DBG_OUT("WindowInfo_Paint() ");
    for (int pageNo = 1; pageNo <= dm->pageCount(); ++pageNo) {
        PdfPageInfo *pageInfo = dm->getPageInfo(pageNo);
        if (!pageInfo->visible)
            continue;
        assert(pageInfo->shown);
        if (!pageInfo->shown)
            continue;

        //BitmapCacheEntry *entry = BitmapCache_Find(dm, pageNo, dm->zoomReal(), dm->rotation());
        BitmapCacheEntry *entry = BitmapCache_Find(dm, pageNo);
        if (entry) {
            if ((dm->rotation() != entry->rotation) || (dm->zoomReal() != entry->zoomLevel))
                entry = NULL;
            else
                renderedBmp = entry->bitmap;
        }

        if (!renderedBmp)
            DBG_OUT("   missing bitmap on visible page %d\n", pageNo);

        int xSrc = (int)pageInfo->bitmapX;
        int ySrc = (int)pageInfo->bitmapY;
        int bmpDx = (int)pageInfo->bitmapDx;
        int bmpDy = (int)pageInfo->bitmapDy;
        int xDest = (int)pageInfo->screenX;
        int yDest = (int)pageInfo->screenY;

        if (!entry) {
            /* TODO: assert is queued for rendering ? */
            HFONT fontRightTxt = Win32_Font_GetSimple(hdc, "Tahoma", 14);
            HFONT origFont = (HFONT)SelectObject(hdc, fontRightTxt); /* Just to remember the orig font */
            bounds.left = xDest;
            bounds.top = yDest;
            bounds.right = xDest + bmpDx;
            bounds.bottom = yDest + bmpDy;
            FillRect(hdc, &bounds, gBrushWhite);
            DrawCenteredText(hdc, &bounds, "Please wait - rendering...");
            DBG_OUT("drawing empty %d ", pageNo);
            if (origFont)
                SelectObject(hdc, origFont);
            Win32_Font_Delete(fontRightTxt);
            continue;
        }

        if (BITMAP_CANNOT_RENDER == renderedBmp) {
            bounds.left = xDest;
            bounds.top = yDest;
            bounds.right = xDest + bmpDx;
            bounds.bottom = yDest + bmpDy;
            FillRect(hdc, &bounds, gBrushWhite);
            DrawCenteredText(hdc, &bounds, "Couldn't render the page");
            continue;
        }

        DBG_OUT("page %d ", pageNo);

        int renderedBmpDx = renderedBmp->dx();
        int renderedBmpDy = renderedBmp->dy();
        int currPageDx = pageInfo->currDx;
        int currPageDy = pageInfo->currDy;
        HBITMAP hbmp = renderedBmp->createDIBitmap(hdc);
        if (!hbmp)
            continue;

        HDC bmpDC = CreateCompatibleDC(hdc);
        if (bmpDC) {
            SelectObject(bmpDC, hbmp);
#if 0
            if ((currPageDx != renderedBmpDx) || (currPageDy != renderedBmpDy))
                StretchBlt(hdc, xDest, yDest, bmpDx, bmpDy, bmpDC, xSrc, ySrc, renderedBmpDx, renderedBmpDy, SRCCOPY);
            else
#endif
                BitBlt(hdc, xDest, yDest, bmpDx, bmpDy, bmpDC, xSrc, ySrc, SRCCOPY);
            DeleteDC(bmpDC);
        }
        DeleteObject(hbmp);
    }

    if (win->showSelection)
        PaintSelection(win, hdc);

    DBG_OUT("\n");
    if (!gDebugShowLinks)
        return;

    RectI drawAreaRect;
    /* debug code to visualize links */
    drawAreaRect.x = (int)dm->areaOffset.x;
    drawAreaRect.y = (int)dm->areaOffset.y;
    drawAreaRect.dx = dm->drawAreaSize.dxI();
    drawAreaRect.dy = dm->drawAreaSize.dyI();

    for (int linkNo = 0; linkNo < dm->linkCount(); ++linkNo) {
        PdfLink *pdfLink = dm->link(linkNo);

        RectI rectLink, intersect;
        rectLink.x = pdfLink->rectCanvas.x;
        rectLink.y = pdfLink->rectCanvas.y;
        rectLink.dx = pdfLink->rectCanvas.dx;
        rectLink.dy = pdfLink->rectCanvas.dy;

        if (RectI_Intersect(&rectLink, &drawAreaRect, &intersect)) {
            RECT rectScreen;
            rectScreen.left = (LONG) ((double)intersect.x - dm->areaOffset.x);
            rectScreen.top = (LONG) ((double)intersect.y - dm->areaOffset.y);
            rectScreen.right = rectScreen.left + rectLink.dx;
            rectScreen.bottom = rectScreen.top + rectLink.dy;
            FillRect(hdc, &rectScreen, gBrushLinkDebug);
            DBG_OUT("  link on screen rotate=%d, (x=%d, y=%d, dx=%d, dy=%d)\n",
                dm->rotation() + dm->pagesInfo[pdfLink->pageNo-1].rotation,
                rectScreen.left, rectScreen.top, rect_dx(&rectScreen), rect_dy(&rectScreen));
        }
    }
}

/* TODO: change the name to DrawAbout.
   Draws the about screen a remember some state for hyperlinking.
   It transcribes the design I did in graphics software - hopeless
   to understand without seeing the design. */
#define ABOUT_RECT_PADDING          8
#define ABOUT_RECT_BORDER_DX_DY     4
#define ABOUT_LINE_OUTER_SIZE       2
#define ABOUT_LINE_RECT_SIZE        5
#define ABOUT_LINE_SEP_SIZE         1
#define ABOUT_LEFT_RIGHT_SPACE_DX   8
#define ABOUT_MARGIN_DX            10
#define ABOUT_BOX_MARGIN_DY         6

#define ABOUT_BORDER_COL            COL_BLACK

#define SUMATRA_TXT             "Sumatra PDF"
#define SUMATRA_TXT_FONT        "Arial Black"
#define SUMATRA_TXT_FONT_SIZE   24
#define BETA_TXT                "Beta v0.7"
#define BETA_TXT_FONT           "Arial Black"
#define BETA_TXT_FONT_SIZE      12
#define LEFT_TXT_FONT           "Arial"
#define LEFT_TXT_FONT_SIZE      12
#define RIGHT_TXT_FONT          "Arial Black"
#define RIGHT_TXT_FONT_SIZE     12

#define ABOUT_BG_COLOR          RGB(255,242,0)
#define ABOUT_RECT_BG_COLOR     RGB(247,148,29)

#define ABOUT_TXT_DY            6

typedef struct AboutLayoutInfoEl {
    /* static data, must be provided */
    const char *    leftTxt;
    const char *    rightTxt;
    const char *    url;

    /* data calculated by the layout */
    int             leftTxtPosX;
    int             leftTxtPosY;
    int             leftTxtDx;
    int             leftTxtDy;

    int             rightTxtPosX;
    int             rightTxtPosY;
    int             rightTxtDx;
    int             rightTxtDy;
} AboutLayoutInfoEl;

AboutLayoutInfoEl gAboutLayoutInfo[] = {
    { "design", "Krzysztof Kowalczyk", "http://blog.kowalczyk.info",
    0, 0, 0, 0, 0, 0, 0, 0 },

    { "programming", "Krzysztof Kowalczyk", "http://blog.kowalczyk.info",
    0, 0, 0, 0, 0, 0, 0, 0 },

    { "pdf rendering 1", "poppler + xpdf", "http://poppler.freedesktop.org/",
    0, 0, 0, 0, 0, 0, 0, 0 },

    { "pdf rendering 2", "MuPDF", "http://ccxvii.net/apparition/",
    0, 0, 0, 0, 0, 0, 0, 0 },

    { "license", "GPL v2", "http://www.gnu.org/copyleft/gpl.html",
    0, 0, 0, 0, 0, 0, 0, 0 },

    { "website", "http://blog.kowalczyk.info/software/sumatra", "http://blog.kowalczyk.info/software/sumatrapdf",
    0, 0, 0, 0, 0, 0, 0, 0 },

    { "forums", "http://blog.kowalczyk.info/forum_sumatra", "http://blog.kowalczyk.info/forum_sumatra",
    0, 0, 0, 0, 0, 0, 0, 0 },

    { "program icon", "Goce Mitevski", "http://monsteer.deviantart.com",
    0, 0, 0, 0, 0, 0, 0, 0 },

    { "toolbar icons", "Mark James", "http://www.famfamfam.com/lab/icons/silk/",
    0, 0, 0, 0, 0, 0, 0, 0 },

    { NULL, NULL, NULL,
    0, 0, 0, 0, 0, 0, 0, 0 }
};

static const char *AboutGetLink(WindowInfo *win, int x, int y)
{
    for (int i = 0; gAboutLayoutInfo[i].leftTxt; i++) {
        if ((x < gAboutLayoutInfo[i].rightTxtPosX) ||
            (x > gAboutLayoutInfo[i].rightTxtPosX + gAboutLayoutInfo[i].rightTxtDx))
            continue;
        if ((y < gAboutLayoutInfo[i].rightTxtPosY) ||
            (y > gAboutLayoutInfo[i].rightTxtPosY + gAboutLayoutInfo[i].rightTxtDy))
            continue;
        return gAboutLayoutInfo[i].url;
    }
    return NULL;
}

static void DrawAbout(HWND hwnd, HDC hdc, PAINTSTRUCT *ps)
{
    RECT            rcTmp;
    SIZE            txtSize;
    int             totalDx, totalDy;
    int             leftDy, rightDy;
    int             leftLargestDx, rightLargestDx;
    int             sumatraPdfTxtDx, sumatraPdfTxtDy;
    int             betaTxtDx, betaTxtDy;
    int             linePosX, linePosY, lineDy;
    int             currY;
    int             fontDyDiff;
    int             offX, offY;
    int             x, y;
    int             boxDy;

    DString         str;
    DStringInit(&str);

    HBRUSH brushBg = CreateSolidBrush(ABOUT_BG_COLOR);
    HBRUSH brushRectBg = CreateSolidBrush(ABOUT_RECT_BG_COLOR);

    HPEN penRectBorder = CreatePen(PS_SOLID, ABOUT_RECT_BORDER_DX_DY, COL_BLACK);
    HPEN penBorder = CreatePen(PS_SOLID, ABOUT_LINE_OUTER_SIZE, COL_BLACK);
    HPEN penDivideLine = CreatePen(PS_SOLID, ABOUT_LINE_SEP_SIZE, COL_BLACK);

    RECT rc;
    GetClientRect(hwnd, &rc);

    int areaDx = rect_dx(&rc);
    int areaDy = rect_dy(&rc);

    HFONT fontSumatraTxt = Win32_Font_GetSimple(hdc, SUMATRA_TXT_FONT, SUMATRA_TXT_FONT_SIZE);
    HFONT fontBetaTxt = Win32_Font_GetSimple(hdc, BETA_TXT_FONT, BETA_TXT_FONT_SIZE);
    HFONT fontLeftTxt = Win32_Font_GetSimple(hdc, LEFT_TXT_FONT, LEFT_TXT_FONT_SIZE);
    HFONT fontRightTxt = Win32_Font_GetSimple(hdc, RIGHT_TXT_FONT, RIGHT_TXT_FONT_SIZE);

    HFONT origFont = (HFONT)SelectObject(hdc, fontSumatraTxt); /* Just to remember the orig font */

    SetBkMode(hdc, TRANSPARENT);

    /* Layout stuff */
    const char *txt = SUMATRA_TXT;
    GetTextExtentPoint32(hdc, (TCHAR*)txt, strlen(txt), &txtSize); /* @note: TCHAR* cast */
    sumatraPdfTxtDx = txtSize.cx;
    sumatraPdfTxtDy = txtSize.cy;

    boxDy = sumatraPdfTxtDy + ABOUT_BOX_MARGIN_DY * 2;
    txt = BETA_TXT;
    GetTextExtentPoint32(hdc, (TCHAR*)txt, strlen(txt), &txtSize); /* @note: TCHAR* cast */
    betaTxtDx = txtSize.cx;
    betaTxtDy = txtSize.cy;

    (HFONT)SelectObject(hdc, fontLeftTxt);
    leftLargestDx = 0;
    leftDy = 0;
    for (int i = 0; gAboutLayoutInfo[i].leftTxt != NULL; i++) {
        txt = gAboutLayoutInfo[i].leftTxt;
        GetTextExtentPoint32(hdc, (TCHAR*)txt, strlen(txt), &txtSize); /* @note: TCHAR* cast */
        gAboutLayoutInfo[i].leftTxtDx = (int)txtSize.cx;
        gAboutLayoutInfo[i].leftTxtDy = (int)txtSize.cy;
        if (0 == i)
            leftDy = gAboutLayoutInfo[i].leftTxtDy;
        else
            assert(leftDy == gAboutLayoutInfo[i].leftTxtDy);
        if (leftLargestDx < gAboutLayoutInfo[i].leftTxtDx)
            leftLargestDx = gAboutLayoutInfo[i].leftTxtDx;
    }

    (HFONT)SelectObject(hdc, fontRightTxt);
    rightLargestDx = 0;
    rightDy = 0;
    for (int i = 0; gAboutLayoutInfo[i].leftTxt != NULL; i++) {
        txt = gAboutLayoutInfo[i].rightTxt;
        GetTextExtentPoint32(hdc, (TCHAR*)txt, strlen(txt), &txtSize); /* @note: TCHAR* cast */
        gAboutLayoutInfo[i].rightTxtDx = (int)txtSize.cx;
        gAboutLayoutInfo[i].rightTxtDy = (int)txtSize.cy;
        if (0 == i)
            rightDy = gAboutLayoutInfo[i].rightTxtDy;
        else
            assert(rightDy == gAboutLayoutInfo[i].rightTxtDy);
        if (rightLargestDx < gAboutLayoutInfo[i].rightTxtDx)
            rightLargestDx = gAboutLayoutInfo[i].rightTxtDx;
    }

    fontDyDiff = (rightDy - leftDy) / 2;

    /* in the x order */
    totalDx  = ABOUT_LINE_OUTER_SIZE + ABOUT_MARGIN_DX + leftLargestDx;
    totalDx += ABOUT_LEFT_RIGHT_SPACE_DX + ABOUT_LINE_SEP_SIZE + ABOUT_LEFT_RIGHT_SPACE_DX;
    totalDx += rightLargestDx + ABOUT_MARGIN_DX + ABOUT_LINE_OUTER_SIZE;

    totalDy = 0;
    totalDy += boxDy;
    totalDy += ABOUT_LINE_OUTER_SIZE;
    totalDy += (dimof(gAboutLayoutInfo)-1) * (rightDy + ABOUT_TXT_DY);
    totalDy += ABOUT_LINE_OUTER_SIZE + 4;

    offX = (areaDx - totalDx) / 2;
    offY = (areaDy - totalDy) / 2;

    rcTmp.left = offX;
    rcTmp.top = offY;
    rcTmp.right = totalDx + offX;
    rcTmp.bottom = totalDy + offY;

    FillRect(hdc, &rc, brushBg);

    SelectObject(hdc, brushBg);
    SelectObject(hdc, penBorder);

    Rectangle(hdc, offX, offY + ABOUT_LINE_OUTER_SIZE, offX + totalDx, offY + boxDy + ABOUT_LINE_OUTER_SIZE);

    SetTextColor(hdc, ABOUT_BORDER_COL);
    (HFONT)SelectObject(hdc, fontSumatraTxt);
    x = offX + (totalDx - sumatraPdfTxtDx) / 2;
    y = offY + (boxDy - sumatraPdfTxtDy) / 2;
    txt = SUMATRA_TXT;
    TextOut(hdc, x, y, (TCHAR*)txt, strlen(txt)); /* @note: TCHAR* cast */

    //SetTextColor(hdc, ABOUT_RECT_BG_COLOR);
    (HFONT)SelectObject(hdc, fontBetaTxt);
    //SelectObject(hdc, brushRectBg);
    x = offX + (totalDx - sumatraPdfTxtDx) / 2 + sumatraPdfTxtDx + 6;
    y = offY + (boxDy - sumatraPdfTxtDy) / 2;
    txt = BETA_TXT;
    TextOut(hdc, x, y, (TCHAR*)txt, strlen(txt)); /* @note: TCHAR* cast */
    SetTextColor(hdc, ABOUT_BORDER_COL);

    offY += boxDy;
    Rectangle(hdc, offX, offY, offX + totalDx, offY + totalDy - boxDy);

    linePosX = ABOUT_LINE_OUTER_SIZE + ABOUT_MARGIN_DX + leftLargestDx + ABOUT_LEFT_RIGHT_SPACE_DX;
    linePosY = 4;
    lineDy = (dimof(gAboutLayoutInfo)-1) * (rightDy + ABOUT_TXT_DY);

    /* render text on the left*/
    currY = linePosY;
    (HFONT)SelectObject(hdc, fontLeftTxt);
    for (int i = 0; gAboutLayoutInfo[i].leftTxt != NULL; i++) {
        txt = gAboutLayoutInfo[i].leftTxt;
        x = linePosX + offX - ABOUT_LEFT_RIGHT_SPACE_DX - gAboutLayoutInfo[i].leftTxtDx;
        y = currY + fontDyDiff + offY;
        gAboutLayoutInfo[i].leftTxtPosX = x;
        gAboutLayoutInfo[i].leftTxtPosY = y;
        TextOut(hdc, x, y, (TCHAR*)txt, strlen(txt)); /* @note: TCHAR* cast */
        currY += rightDy + ABOUT_TXT_DY;
    }

    /* render text on the rigth */
    currY = linePosY;
    (HFONT)SelectObject(hdc, fontRightTxt);
    for (int i = 0; gAboutLayoutInfo[i].leftTxt != NULL; i++) {
        txt = gAboutLayoutInfo[i].rightTxt;
        x = linePosX + offX + ABOUT_LEFT_RIGHT_SPACE_DX;
        y = currY + offY;
        gAboutLayoutInfo[i].rightTxtPosX = x;
        gAboutLayoutInfo[i].rightTxtPosY = y;
        TextOut(hdc, x, y, (TCHAR*)txt, strlen(txt)); /* @note: TCHAR* cast */
        currY += rightDy + ABOUT_TXT_DY;
    }

    SelectObject(hdc, penDivideLine);
    MoveToEx(hdc, linePosX + offX, linePosY + offY, NULL);
    LineTo(hdc, linePosX + offX, linePosY + lineDy + offY);

    if (origFont)
        SelectObject(hdc, origFont);

    Win32_Font_Delete(fontSumatraTxt);
    Win32_Font_Delete(fontBetaTxt);
    Win32_Font_Delete(fontLeftTxt);
    Win32_Font_Delete(fontRightTxt);

    DeleteObject(brushBg);
    DeleteObject(brushRectBg);
    DeleteObject(penBorder);
    DeleteObject(penDivideLine);
    DeleteObject(penRectBorder);
}

static void WinMoveDocBy(WindowInfo *win, int dx, int dy)
{
    assert(win);
    if (!win) return;
    assert (WS_SHOWING_PDF == win->state);
    if (WS_SHOWING_PDF != win->state) return;
    assert(win->dm);
    if (!win->dm) return;
    assert(!win->linkOnLastButtonDown);
    if (win->linkOnLastButtonDown) return;
    if (0 != dx)
        win->dm->scrollXBy(dx);
    if (0 != dy)
        win->dm->scrollYBy(dy, FALSE);
}

static void CopySelectionTextToClipboard(WindowInfo *win)
{
    SelectionOnPage *   selOnPage;

    assert(win);
    if (!win) return;

    if (!win->selectionOnPage) return;

    HGLOBAL handle;
    unsigned short *ucsbuf;
    int ucsbuflen = 4096;

    if (!OpenClipboard(NULL)) return;

    EmptyClipboard();

    handle = GlobalAlloc(GMEM_MOVEABLE, ucsbuflen * sizeof(unsigned short));
    if (!handle) {
        CloseClipboard();
        return;
    }
    ucsbuf = (unsigned short *) GlobalLock(handle);

    selOnPage = win->selectionOnPage;

    int copied = 0;
    while (selOnPage != NULL) {
        int charCopied = win->dm->getTextInRegion(selOnPage->pageNo, 
            &selOnPage->selectionPage, ucsbuf + copied, ucsbuflen - copied - 1);
        copied += charCopied;
        if (ucsbuflen - copied == 1) 
            break;
        selOnPage = selOnPage->next;
    }
    ucsbuf[copied] = 0;

    GlobalUnlock(handle);

    SetClipboardData(CF_UNICODETEXT, handle);
    CloseClipboard();
}

static void DeleteOldSelectionInfo (WindowInfo *win) {
    SelectionOnPage *selOnPage = win->selectionOnPage;
    while (selOnPage != NULL) {
        SelectionOnPage *tmp = selOnPage->next;
        free(selOnPage);
        selOnPage = tmp;
    }
    win->selectionOnPage = NULL;
}

static void ConvertSelectionRectToSelectionOnPage (WindowInfo *win) {
    RectI pageOnScreen, intersect;

    for (int pageNo = win->dm->pageCount(); pageNo >= 1; --pageNo) {
        PdfPageInfo *pageInfo = win->dm->getPageInfo(pageNo);
        if (!pageInfo->visible)
            continue;
        assert(pageInfo->shown);
        if (!pageInfo->shown)
            continue;

        pageOnScreen.x = pageInfo->screenX;
        pageOnScreen.y = pageInfo->screenY;
        pageOnScreen.dx = pageInfo->bitmapDx;
        pageOnScreen.dy = pageInfo->bitmapDy;

        if (!RectI_Intersect(&win->selectionRect, &pageOnScreen, &intersect))
            continue;

        /* selection intersects with a page <pageNo> on the screen */
        SelectionOnPage *selOnPage = (SelectionOnPage*)malloc(sizeof(SelectionOnPage));
        RectD_FromRectI(&selOnPage->selectionPage, &intersect);

        win->dm->rectCvtScreenToUser (&selOnPage->pageNo, &selOnPage->selectionPage);

        assert (pageNo == selOnPage->pageNo);

        selOnPage->next = win->selectionOnPage;
        win->selectionOnPage = selOnPage;
    }
}

static void OnMouseLeftButtonDown(WindowInfo *win, int x, int y)
{
    assert(win);
    if (!win) return;
    if (WS_SHOWING_PDF == win->state && win->mouseAction == MA_IDLE) {
        assert(win->dm);
        if (!win->dm) return;
        win->linkOnLastButtonDown = win->dm->linkAtPosition(x, y);
        /* dragging mode only starts when we're not on a link */
        if (!win->linkOnLastButtonDown) {
            SetCapture(win->hwndCanvas);
            win->mouseAction = MA_DRAGGING;
            win->dragPrevPosX = x;
            win->dragPrevPosY = y;
            SetCursor(gCursorDrag);
            DBG_OUT(" dragging start, x=%d, y=%d\n", x, y);
        }
    } else if (WS_ABOUT == win->state) {
        win->url = AboutGetLink(win, x, y);
    }
}

static void OnMouseLeftButtonUp(WindowInfo *win, int x, int y)
{
    PdfLink *       link;
    const char *    url;
    int             dragDx, dragDy;

    assert(win);
    if (!win) return;

    if (WS_ABOUT == win->state) {
        url = AboutGetLink(win, x, y);
        if (url == win->url)
            LaunchBrowser(url);
        win->url = NULL;
        return;
    }

    if (WS_SHOWING_PDF != win->state)
        return;

    assert(win->dm);
    if (!win->dm) return;

    if (win->mouseAction == MA_DRAGGING && (GetCapture() == win->hwndCanvas)) {
        dragDx = 0; dragDy = 0;
        dragDx = x - win->dragPrevPosX;
        dragDy = y - win->dragPrevPosY;
        DBG_OUT(" dragging ends, x=%d, y=%d, dx=%d, dy=%d\n", x, y, dragDx, dragDy);
        assert(!win->linkOnLastButtonDown);
        WinMoveDocBy(win, dragDx, -dragDy*2);
        win->dragPrevPosX = x;
        win->dragPrevPosY = y;
        win->mouseAction = MA_IDLE;
        SetCursor(gCursorArrow);
        ReleaseCapture();            
        return;
    }

    if (!win->linkOnLastButtonDown)
        return;

    link = win->dm->linkAtPosition(x, y);
    if (link && (link == win->linkOnLastButtonDown))
        win->dm->handleLink(link);
    win->linkOnLastButtonDown = NULL;
}

static void OnMouseMove(WindowInfo *win, int x, int y, WPARAM flags)
{
    PdfLink *       link;
    const char *    url;
    int             dragDx, dragDy;

    assert(win);
    if (!win) return;

    if (WS_SHOWING_PDF == win->state) {
        assert(win->dm);
        if (!win->dm) return;
        if (win->mouseAction == MA_SELECTING) {
            SetCursor(gCursorArrow);
            win->selectionRect.dx = x - win->selectionRect.x;
            win->selectionRect.dy = y - win->selectionRect.y;
            triggerRepaintDisplayNow(win);
        } else {
            if (win->mouseAction == MA_DRAGGING) {
                dragDx = 0; dragDy = 0;
                dragDx = -(x - win->dragPrevPosX);
                dragDy = -(y - win->dragPrevPosY);
                DBG_OUT(" drag move, x=%d, y=%d, dx=%d, dy=%d\n", x, y, dragDx, dragDy);
                WinMoveDocBy(win, dragDx, dragDy*2);
                win->dragPrevPosX = x;
                win->dragPrevPosY = y;
                return;
            }
            link = win->dm->linkAtPosition(x, y);
            if (link)
                SetCursor(gCursorHand);
            else
                SetCursor(gCursorArrow);
        }
    } else if (WS_ABOUT == win->state) {
        url = AboutGetLink(win, x, y);
        if (url) {
            SetCursor(gCursorHand);
        } else {
            SetCursor(gCursorArrow);
        }
    } else {
        // TODO: be more efficient, this only needs to be set once (I think)
        SetCursor(gCursorArrow);
    }
}

static void OnMouseRightButtonDown(WindowInfo *win, int x, int y)
{
    //DBG_OUT("Right button clicked on %d %d\n", x, y);
    assert (win);
    if (!win) return;

    if (WS_SHOWING_PDF == win->state && win->mouseAction == MA_IDLE) {
        win->documentBlocked = true;
        DeleteOldSelectionInfo (win);

        win->selectionRect.x = x;
        win->selectionRect.y = y;
        win->selectionRect.dx = 0;
        win->selectionRect.dy = 0;
        win->showSelection = true;
        win->mouseAction = MA_SELECTING;

        triggerRepaintDisplayNow(win);
    }
}

static void OnMouseRightButtonUp(WindowInfo *win, int x, int y)
{
    assert (win);
    if (!win) return;

    if (WS_SHOWING_PDF == win->state && win->mouseAction == MA_SELECTING) {
        assert (win->dm);
        if (!win->dm) return;
        win->documentBlocked = false;

        win->selectionRect.dx = abs (x - win->selectionRect.x);
        win->selectionRect.dy = abs (y - win->selectionRect.y);
        win->selectionRect.x = min (win->selectionRect.x, x);
        win->selectionRect.y = min (win->selectionRect.y, y);

        win->mouseAction = MA_IDLE;
        if (win->selectionRect.dx == 0 || win->selectionRect.dy == 0) {
            win->showSelection = false;
        } else {
            ConvertSelectionRectToSelectionOnPage (win);
            CopySelectionTextToClipboard (win);
        }
        triggerRepaintDisplayNow(win);
    }
}

#define ABOUT_ANIM_TIMER_ID 15

static void AnimState_AnimStop(AnimState *state)
{
    KillTimer(state->hwnd, ABOUT_ANIM_TIMER_ID);
}

static void AnimState_NextFrame(AnimState *state)
{
    state->frame += 1;
    InvalidateRect(state->hwnd, NULL, FALSE);
    UpdateWindow(state->hwnd);
}

static void AnimState_AnimStart(AnimState *state, HWND hwnd, UINT freqInMs)
{
    assert(IsWindow(hwnd));
    AnimState_AnimStop(state);
    state->frame = 0;
    state->hwnd = hwnd;
    SetTimer(state->hwnd, ABOUT_ANIM_TIMER_ID, freqInMs, NULL);
    AnimState_NextFrame(state);
}

#define ANIM_FONT_NAME "Georgia"
#define ANIM_FONT_SIZE_START 20
#define SCROLL_SPEED 3

static void DrawAnim2(WindowInfo *win, HDC hdc, PAINTSTRUCT *ps)
{
    AnimState *     state = &(win->animState);
    DString         txt;
    RECT            rc;
    HFONT           fontArial24 = NULL;
    HFONT           origFont = NULL;
    int             curFontSize;
    static int      curTxtPosX = -1;
    static int      curTxtPosY = -1;
    static int      curDir = SCROLL_SPEED;

    GetClientRect(win->hwndCanvas, &rc);

    DStringInit(&txt);

    if (-1 == curTxtPosX)
        curTxtPosX = 40;
    if (-1 == curTxtPosY)
        curTxtPosY = 25;

    int areaDx = rect_dx(&rc);
    int areaDy = rect_dy(&rc);

#if 0
    if (state->frame % 24 <= 12) {
        curFontSize = ANIM_FONT_SIZE_START + (state->frame % 24);
    } else {
        curFontSize = ANIM_FONT_SIZE_START + 12 - (24 - (state->frame % 24));
    }
#else
    curFontSize = ANIM_FONT_SIZE_START;
#endif

    curTxtPosY += curDir;
    if (curTxtPosY < 20)
        curDir = SCROLL_SPEED;
    else if (curTxtPosY > areaDy - 40)
        curDir = -SCROLL_SPEED;

    fontArial24 = Win32_Font_GetSimple(hdc, ANIM_FONT_NAME, curFontSize);
    assert(fontArial24);

    origFont = (HFONT)SelectObject(hdc, fontArial24);
    
    SetBkMode(hdc, TRANSPARENT);
    FillRect(hdc, &rc, gBrushBg);
    //DStringSprintf(&txt, "Welcome to animation %d", state->frame);
    DStringSprintf(&txt, "Welcome to animation");
    //DrawText (hdc, txt.pString, -1, &rc, DT_SINGLELINE);
    TextOut(hdc, curTxtPosX, curTxtPosY, (TCHAR*)txt.pString, txt.length); /* @note: TCHAR* cast */
    WindowInfo_DoubleBuffer_Show(win, hdc);
    if (state->frame > 99)
        state->frame = 0;

    if (origFont)
        SelectObject(hdc, origFont);
    Win32_Font_Delete(fontArial24);
}

static void WindowInfo_DoubleBuffer_Resize_IfNeeded(WindowInfo *win)
{
    WinResizeIfNeeded(win, false);
}

static void OnPaintAbout(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    SetBkMode(hdc, TRANSPARENT);
    DrawAbout(hwnd, hdc, &ps);
    EndPaint(hwnd, &ps);
}

static void OnPaint(WindowInfo *win)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(win->hwndCanvas, &ps);

    SetBkMode(hdc, TRANSPARENT);
    RECT rc;
    GetClientRect(win->hwndCanvas, &rc);

    if (WS_ABOUT == win->state) {
        WindowInfo_DoubleBuffer_Resize_IfNeeded(win);
        DrawAbout(win->hwndCanvas, win->hdcToDraw, &ps);
        WindowInfo_DoubleBuffer_Show(win, hdc);
    } else if (WS_ERROR_LOADING_PDF == win->state) {
        HFONT fontRightTxt = Win32_Font_GetSimple(hdc, "Tahoma", 14);
        HFONT origFont = (HFONT)SelectObject(hdc, fontRightTxt); /* Just to remember the orig font */
        FillRect(hdc, &ps.rcPaint, gBrushBg);
	/* @note: translation function and it's related macros need some care */
        //DrawText(hdc, _TR("Error loading PDF file."), -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER) ;
        DrawText(hdc, TEXT("Error loading PDF file."), -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER) ;
        if (origFont)
            SelectObject(hdc, origFont);
        Win32_Font_Delete(fontRightTxt);
    } else if (WS_SHOWING_PDF == win->state) {
        //TODO: it might cause infinite loop due to showing/hiding scrollbars
        WinResizeIfNeeded(win);
        WindowInfo_Paint(win, hdc, &ps);
#if 0
        if (VS_AMIGA == gVisualStyle)
            AmigaCaptionDraw(win);
#endif
        WindowInfo_DoubleBuffer_Show(win, hdc);
    } else
        assert(0);

    EndPaint(win->hwndCanvas, &ps);
}

static void OnMenuExit(void)
{
    Prefs_Save();
    PostQuitMessage(0);
}

/* Close the document associated with window 'hwnd'.
   Closes the window unless this is the last window in which
   case it switches to empty window and disables the "File\Close"
   menu item. */
static void CloseWindow(WindowInfo *win, bool quitIfLast)
{
    assert(win);
    if (!win)  return;

    bool lastWindow = false;
    if (gRunningDLL) {
        lastWindow = true;
    } else {
        if (1 == WindowInfoList_Len())
            lastWindow = true;

        if (lastWindow)
            Prefs_Save();
        else
            UpdateCurrentFileDisplayStateForWin(win);
    }

    win->state = WS_ABOUT;

    if (lastWindow && !quitIfLast) {
        /* last window - don't delete it */
        delete win->dm;
        win->dm = NULL;
        WindowInfo_RedrawAll(win);
    } else {
        HWND hwndToDestroy = win->hwndFrame;
        WindowInfoList_Remove(win);
        WindowInfo_Delete(win);
        DragAcceptFiles(hwndToDestroy, FALSE);
        DestroyWindow(hwndToDestroy);
    }

    if (lastWindow && quitIfLast) {
        assert(0 == WindowInfoList_Len());
        PostQuitMessage(0);
    } else {
        if (!gRunningDLL)
            MenuToolbarUpdateStateForAllWindows();
    }
}

/* Zoom document in window 'hwnd' to zoom level 'zoom'.
   'zoom' is given as a floating-point number, 1.0 is 100%, 2.0 is 200% etc.
*/
static void OnMenuZoom(WindowInfo *win, UINT menuId)
{
    if (!win->dm)
        return;

    double zoom = ZoomMenuItemToZoom(menuId);
    win->dm->zoomTo(zoom);
    ZoomMenuItemCheck(GetMenu(win->hwndFrame), menuId);
}

static bool CheckPrinterStretchDibSupport(HWND hwndForMsgBox, HDC hdc)
{
    // most printers can support stretchdibits,
    // whereas a lot of printers do not support bitblt
    // quit if printer doesn't support StretchDIBits
    int rasterCaps = GetDeviceCaps(hdc, RASTERCAPS);
    int supportsStretchDib = rasterCaps & RC_STRETCHDIB;
    if (supportsStretchDib)
        return true;

    MessageBox(hwndForMsgBox, TEXT("This printer doesn't support StretchDIBits function"), TEXT("Printing problem."), MB_ICONEXCLAMATION | MB_OK);
    return false;
}

// TODO: make it run in a background thread by constructing new PdfEngine()
// from a file name - this should be thread safe
static void PrintToDevice(DisplayModel *dm, HDC hdc, LPDEVMODE devMode, int fromPage, int toPage) {

    assert(toPage >= fromPage);
    assert(dm);
    if (!dm) return;

    PdfEngine *pdfEngine = dm->pdfEngine();
    DOCINFO di = {0};
    di.cbSize = sizeof (DOCINFO);
    di.lpszDocName = (LPCTSTR)pdfEngine->fileName();

    if (StartDoc(hdc, &di) <= 0)
        return;

    // rendering for the same DisplayModel is not thread-safe
    // TODO: in fitz, propably rendering anything might not be thread-safe
    RenderQueue_RemoveForDisplayModel(dm);
    cancelRenderingForDisplayModel(dm);

    // print all the pages the user requested unless
    // bContinue flags there is a problem.
    for (int pageNo = fromPage; pageNo <= toPage; pageNo++) {
        int rotation = pdfEngine->pageRotation(pageNo);

        DBG_OUT(" printing:  drawing bitmap for page %d\n", pageNo);

        // render at a big zoom, 250% should be good enough. It's a compromise
        // between quality and memory usage. TODO: ideally we would use zoom
        // that matches the size of the page in the printer
        // TODO: consider using a greater zoom level e.g. 750.0
        RenderedBitmap *bmp = pdfEngine->renderBitmap(pageNo, 250.0, rotation, NULL, NULL);
        if (!bmp)
            goto Error; /* most likely ran out of memory */

        StartPage(hdc);
        // MM_TEXT: Each logical unit is mapped to one device pixel.
        // Positive x is to the right; positive y is down.
        SetMapMode(hdc, MM_TEXT);

        int pageHeight = GetDeviceCaps(hdc, PHYSICALHEIGHT);
        int pageWidth = GetDeviceCaps(hdc, PHYSICALWIDTH);

        int topMargin = GetDeviceCaps(hdc, PHYSICALOFFSETY);
        int leftMargin = GetDeviceCaps(hdc, PHYSICALOFFSETX);
        if (DMORIENT_LANDSCAPE == devMode->dmOrientation)
            swap_int(&topMargin, &leftMargin);

        bmp->stretchDIBits(hdc, -leftMargin, -topMargin, pageWidth, pageHeight);
        delete bmp;
        if (EndPage(hdc) <= 0) {
            AbortDoc(hdc);
            return;
        }
    }

Error:
    EndDoc(hdc);
}

/* Show Print Dialog box to allow user to select the printer
and the pages to print.

Creates a new dummy page for each page with a large zoom factor,
and then uses StretchDIBits to copy this to the printer's dc.

So far have tested printing from XP to
 - Acrobat Professional 6 (note that acrobat is usually set to
   downgrade the resolution of its bitmaps to 150dpi)
 - HP Laserjet 2300d
 - HP Deskjet D4160
 - Lexmark Z515 inkjet, which should cover most bases.
*/
static void OnMenuPrint(WindowInfo *win)
{
    PRINTDLG            pd;

    assert(win);
    if (!win) return;

    DisplayModel *dm = win->dm;
    assert(dm);
    if (!dm) return;

    /* printing uses the WindowInfo win that is created for the
       screen, it may be possible to create a new WindowInfo
       for printing to so we don't mess with the screen one,
       but the user is not inconvenienced too much, and this
       way we only need to concern ourselves with one dm.
       TODO: don't re-use WindowInfo, use a different, synchronious
       way of creating a bitmap */
    ZeroMemory(&pd, sizeof(pd));
    pd.lStructSize = sizeof(pd);
    pd.hwndOwner   = win->hwndFrame;
    pd.hDevMode    = NULL;   
    pd.hDevNames   = NULL;   
    pd.Flags       = PD_USEDEVMODECOPIESANDCOLLATE | PD_RETURNDC;
    pd.nCopies     = 1;
    /* by default print all pages */
    pd.nFromPage   = 1;
    pd.nToPage     = dm->pageCount();
    pd.nMinPage    = 1;
    pd.nMaxPage    = dm->pageCount();

    BOOL pressedOk = PrintDlg(&pd);
    if (!pressedOk) {
        if (CommDlgExtendedError()) {
            /* if PrintDlg was cancelled then
               CommDlgExtendedError is zero, otherwise it returns the
               error code, which we could look at here if we wanted.
               for now just warn the user that printing has stopped
               becasue of an error */
            MessageBox(win->hwndFrame, TEXT("Cannot initialise printer"), TEXT("Printing problem."), MB_ICONEXCLAMATION | MB_OK);
        } /* @note: TEXT() casts */
        return;
    }

    if (CheckPrinterStretchDibSupport(win->hwndFrame, pd.hDC))
        PrintToDevice(dm, pd.hDC, (LPDEVMODE)pd.hDevMode, pd.nFromPage, pd.nToPage);

    DeleteDC(pd.hDC);
    if (pd.hDevNames != NULL) GlobalFree(pd.hDevNames);
    if (pd.hDevMode != NULL) GlobalFree(pd.hDevMode);
}

static void OnMenuSaveAs(WindowInfo *win)
{
    OPENFILENAME ofn = {0};
    char         dstFileName[MAX_PATH] = {0};
    const char*  srcFileName = NULL;

    assert(win);
    if (!win) return;
    assert(win->dm);
    if (!win->dm) return;

    srcFileName = win->dm->fileName();
    assert(srcFileName);
    if (!srcFileName) return;

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = win->hwndFrame;
    ofn.lpstrFile = (TCHAR*)dstFileName;

    // Set lpstrFile[0] to '\0' so that GetOpenFileName does not
    // use the contents of szFile to initialize itself.
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = dimof(dstFileName);
    ofn.lpstrFilter = TEXT("PDF\0*.pdf\0All\0*.*\0"); /* @note: TEXT() casts */
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT;

    if (FALSE == GetSaveFileName(&ofn))
        return;

    char* realDstFileName = dstFileName;
    if (!str_endswithi(dstFileName, ".pdf")) {
        realDstFileName = str_cat(dstFileName, ".pdf");
    }
    BOOL cancelled = FALSE;
    BOOL ok = CopyFileEx((TCHAR*)srcFileName, (TCHAR*)realDstFileName, NULL, NULL, &cancelled, COPY_FILE_FAIL_IF_EXISTS);  /* @note: TCHAR* cast */
    if (!ok) {
        SeeLastError();
	 /* @note: translation needs some care */
        //MessageBox(win->hwndFrame, _TR("Failed to save a file"), TEXT("Information"), MB_OK);
	MessageBox(win->hwndFrame, TEXT("Failed to save a file"), TEXT("Information"), MB_OK);
    }
    if (realDstFileName != dstFileName)
        free(realDstFileName);
}

static void OnMenuOpen(WindowInfo *win)
{
    OPENFILENAME ofn = {0};
    char         fileName[PATH_MAX];

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = win->hwndFrame;
    ofn.lpstrFile = (TCHAR*)fileName; /* @note: TCHAR* cast */

    // Set lpstrFile[0] to '\0' so that GetOpenFileName does not
    // use the contents of szFile to initialize itself.
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(fileName);
    ofn.lpstrFilter = TEXT("PDF\0*.pdf\0All\0*.*\0");
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (FALSE == GetOpenFileName(&ofn))
        return;

    win = LoadPdf(fileName);
    if (!win)
        return;
}

static void RotateLeft(WindowInfo *win)
{
    assert(win);
    if (!win) return;
    if (!WindowInfo_PdfLoaded(win))
        return;
    win->dm->rotateBy(-90);
}

static void RotateRight(WindowInfo *win)
{
    assert(win);
    if (!win) return;
    if (!WindowInfo_PdfLoaded(win))
        return;
    win->dm->rotateBy(90);
}

static void OnVScroll(WindowInfo *win, WPARAM wParam)
{
	if (win->documentBlocked) return;
    SCROLLINFO   si = {0};
    int          iVertPos;

    si.cbSize = sizeof (si);
    si.fMask  = SIF_ALL;
    GetScrollInfo(win->hwndCanvas, SB_VERT, &si);

    iVertPos = si.nPos;

    switch (LOWORD(wParam))
    {
        case SB_TOP:
           si.nPos = si.nMin;
           break;

        case SB_BOTTOM:
           si.nPos = si.nMax;
           break;

        case SB_LINEUP:
           si.nPos -= 16;
           break;

        case SB_LINEDOWN:
           si.nPos += 16;
           break;

        case SB_PAGEUP:
           si.nPos -= si.nPage;
           break;

        case SB_PAGEDOWN:
           si.nPos += si.nPage;
           break;

        case SB_THUMBTRACK:
           si.nPos = si.nTrackPos;
           break;

        default:
           break;
    }

    // Set the position and then retrieve it.  Due to adjustments
    // by Windows it may not be the same as the value set.
    si.fMask = SIF_POS;
    SetScrollInfo(win->hwndCanvas, SB_VERT, &si, TRUE);
    GetScrollInfo(win->hwndCanvas, SB_VERT, &si);

    // If the position has changed, scroll the window and update it
    if (win->dm && (si.nPos != iVertPos))
        win->dm->scrollYTo(si.nPos);
}

static void OnHScroll(WindowInfo *win, WPARAM wParam)
{
	if (win->documentBlocked) return;
    SCROLLINFO   si = {0};
    int          iVertPos;

    si.cbSize = sizeof (si);
    si.fMask  = SIF_ALL;
    GetScrollInfo(win->hwndCanvas, SB_HORZ, &si);

    iVertPos = si.nPos;

    switch (LOWORD(wParam))
    {
        case SB_TOP:
           si.nPos = si.nMin;
           break;

        case SB_BOTTOM:
           si.nPos = si.nMax;
           break;

        case SB_LINEUP:
           si.nPos -= 16;
           break;

        case SB_LINEDOWN:
           si.nPos += 16;
           break;

        case SB_PAGEUP:
           si.nPos -= si.nPage;
           break;

        case SB_PAGEDOWN:
           si.nPos += si.nPage;
           break;

        case SB_THUMBTRACK:
           si.nPos = si.nTrackPos;
           break;

        default:
           break;
    }

    // Set the position and then retrieve it.  Due to adjustments
    // by Windows it may not be the same as the value set.
    si.fMask = SIF_POS;
    SetScrollInfo(win->hwndCanvas, SB_HORZ, &si, TRUE);
    GetScrollInfo(win->hwndCanvas, SB_HORZ, &si);

    // If the position has changed, scroll the window and update it
    if (win->dm && (si.nPos != iVertPos))
        win->dm->scrollXTo(si.nPos);
}

static void ViewWithAcrobat(WindowInfo *win)
{
    // TODO: write me
}

static void OnMenuViewSinglePage(WindowInfo *win)
{
    assert(win);
    if (!win) return;
    if (!WindowInfo_PdfLoaded(win))
        return;
    SwitchToDisplayMode(win, DM_SINGLE_PAGE);
}

static void OnMenuViewFacing(WindowInfo *win)
{
    assert(win);
    if (!win) return;
    if (!WindowInfo_PdfLoaded(win))
        return;
    SwitchToDisplayMode(win, DM_FACING);
}

static void OneMenuMakeDefaultReader(void)
{
    AssociateExeWithPdfExtensions();
    /* @note: translation need some care */
    //MessageBox(NULL, _TR("SumatraPDF is now a default reader for PDF files."), "Information", MB_OK);
    MessageBox(NULL, TEXT("SumatraPDF is now a default reader for PDF files."), TEXT("Information"), MB_OK);
}

static void OnSize(WindowInfo *win, int dx, int dy)
{
    int rebBarDy = 0;
    if (gShowToolbar) {
        SetWindowPos(win->hwndReBar, NULL, 0, 0, dx, rebBarDy, SWP_NOZORDER);
        rebBarDy = gReBarDy + gReBarDyFrame;
    }
    SetWindowPos(win->hwndCanvas, NULL, 0, rebBarDy, dx, dy-rebBarDy, SWP_NOZORDER);
}

static void ReloadPdfDocument(WindowInfo *win)
{
    if (WS_SHOWING_PDF != win->state)
        return;
    const char *fileName = NULL;
    if (win->dm)
        fileName = (const char*)str_dup(win->dm->fileName());
    CloseWindow(win, false);
    if (fileName) {
        LoadPdf(fileName);
        free((void*)fileName);
    }
}

static void RebuildProgramMenus(void)
{
    HMENU m = ForceRebuildMenu();
    WindowInfo *win = gWindowList;
    while (win) {
        SetMenu(win->hwndFrame, m);
        MenuUpdateStateForWindow(win);
        win = win->next;
    }
}

static void LanguageChanged(const char *langName)
{
    assert(!str_eq(langName, CurrLangNameGet()));

    CurrLangNameSet(langName);

    RebuildProgramMenus();
    // TODO: recreate tooltips
}

static void OnMenuLanguage(int langId)
{
    const char *langName = NULL;
    for (int i=0; i < LANGS_COUNT; i++) {
        if (g_langs[i]._langId == langId) {
            langName = g_langs[i]._langName;
            break;
        }
    }

    assert(langName);
    if (!langName) return;
    if (str_eq(langName, CurrLangNameGet()))
        return;
    LanguageChanged(langName);
}

static void OnMenuViewUseFitz(WindowInfo *win)
{
    assert(win);
    DBG_OUT("OnMenuViewUseFitz()\n");
    if (gUseFitz)
        gUseFitz = FALSE;
    else
        gUseFitz = TRUE;

    ReloadPdfDocument(win);
    win = gWindowList;
    while (win) {
        MenuUpdateUseFitzStateForWindow(win);
        win = win->next;
    }
}

static void OnMenuViewShowHideToolbar()
{
    if (gShowToolbar)
        gShowToolbar = FALSE;
    else
        gShowToolbar = TRUE;

    WindowInfo* win = gWindowList;
    while (win) {
        if (gShowToolbar)
            ShowWindow(win->hwndReBar, SW_SHOW);
        else
            ShowWindow(win->hwndReBar, SW_HIDE);
        int dx, dy, x, y;
        Win32_Win_GetPos(win->hwndFrame, &x, &y);
        Win32_Win_GetSize(win->hwndFrame, &dx, &dy);
        // TODO: a hack. I add 1 to dy to cause sending WM_SIZE msg to hwndFrame
        // but I shouldn't really change the size. But I don't know how to
        // cause sending WM_SIZE otherwise. I tried calling OnSize() directly,
        // but it left scrollbar partially hidden
        MoveWindow(win->hwndFrame, x, y, dx, dy+1, TRUE);
        MenuUpdateShowToolbarStateForWindow(win);
        win = win->next;
    }
}

static void OnMenuViewContinuous(WindowInfo *win)
{
    assert(win);
    if (!win) return;
    if (!WindowInfo_PdfLoaded(win))
        return;
    SwitchToDisplayMode(win, DM_CONTINUOUS);
}

static void OnMenuViewContinuousFacing(WindowInfo *win)
{
    assert(win);
    if (!win) return;
    if (!WindowInfo_PdfLoaded(win))
        return;
    SwitchToDisplayMode(win, DM_CONTINUOUS_FACING);
}

static void OnMenuGoToNextPage(WindowInfo *win)
{
    assert(win);
    if (!win) return;
    if (!WindowInfo_PdfLoaded(win))
        return;
    win->dm->goToNextPage(0);
}

static void OnMenuGoToPrevPage(WindowInfo *win)
{
    assert(win);
    if (!win) return;
    if (!WindowInfo_PdfLoaded(win))
        return;
    win->dm->goToPrevPage(0);
}

static void OnMenuGoToLastPage(WindowInfo *win)
{
    assert(win);
    if (!win) return;
    if (!WindowInfo_PdfLoaded(win))
        return;
    win->dm->goToLastPage();
}

static void OnMenuGoToFirstPage(WindowInfo *win)
{
    assert(win);
    if (!win) return;
    if (!WindowInfo_PdfLoaded(win))
        return;
    win->dm->goToFirstPage();
}

static void OnMenuGoToPage(WindowInfo *win)
{
    assert(win);
    if (!win) return;
    if (!WindowInfo_PdfLoaded(win))
        return;

    int newPageNo = Dialog_GoToPage(win);
    if (win->dm->validPageNo(newPageNo))
        win->dm->goToPage(newPageNo, 0);
}

static void OnMenuViewRotateLeft(WindowInfo *win)
{
    RotateLeft(win);
}

static void OnMenuViewRotateRight(WindowInfo *win)
{
    RotateRight(win);
}

#define KEY_PRESSED_MASK 0x8000
static bool WasKeyDown(int virtKey)
{
    SHORT state = GetKeyState(virtKey);
    if (KEY_PRESSED_MASK & state)
        return true;
    return false;
}

static bool WasShiftPressed()
{
    return WasKeyDown(VK_LSHIFT) || WasKeyDown(VK_RSHIFT);
}

static bool WasCtrlPressed()
{
    return WasKeyDown(VK_LCONTROL) || WasKeyDown(VK_RCONTROL);
}

static void OnKeydown(WindowInfo *win, int key, LPARAM lparam)
{
    if (!win->dm)
        return;
    if (win->documentBlocked)
        return;

    bool shiftPressed = WasShiftPressed();
    bool ctrlPressed = WasCtrlPressed();
    //DBG_OUT("key=%d,%c,shift=%d,ctrl=%d\n", key, (char)key, (int)shiftPressed, (int)ctrlPressed);

    if (VK_PRIOR == key) {
        /* TODO: more intelligence (see VK_NEXT comment). Also, probably
           it's exactly the same as 'n' so the code should be factored out */
        win->dm->goToPrevPage(0);
       /* SendMessage (win->hwnd, WM_VSCROLL, SB_PAGEUP, 0); */
    } else if (VK_NEXT == key) {
        /* TODO: this probably should be more intelligent (scroll if not yet at the bottom,
           go to next page if at the bottom, and something entirely different in continuous mode */
        win->dm->goToNextPage(0);
        /* SendMessage (win->hwnd, WM_VSCROLL, SB_PAGEDOWN, 0); */
    } else if (VK_UP == key) {
        SendMessage (win->hwndCanvas, WM_VSCROLL, SB_LINEUP, 0);
    } else if (VK_DOWN == key) {
        SendMessage (win->hwndCanvas, WM_VSCROLL, SB_LINEDOWN, 0);
    } else if (VK_LEFT == key) {
        SendMessage (win->hwndCanvas, WM_HSCROLL, SB_PAGEUP, 0);
    } else if (VK_RIGHT == key) {
        SendMessage (win->hwndCanvas, WM_HSCROLL, SB_PAGEDOWN, 0);
    } else if (VK_HOME == key) {
        win->dm->goToFirstPage();
    } else if (VK_END == key) {
        win->dm->goToLastPage();    
#if 0 // we do it via accelerators
    } else if ('G' == key) {
        if (ctrlPressed)
            OnMenuGoToPage(win);
#endif
    } else if (VK_OEM_PLUS == key) {
        // Emulate acrobat: "Shift Ctrl +" is rotate clockwise
        if (shiftPressed & ctrlPressed)
            RotateRight(win);
    } else if (VK_OEM_MINUS == key) {
        // Emulate acrobat: "Shift Ctrl -" is rotate counter-clockwise
        if (shiftPressed & ctrlPressed)
            RotateLeft(win);
    }
}

static void OnChar(WindowInfo *win, int key)
{
    if (!win->dm)
        return;
    if (win->documentBlocked)
        return;

//    DBG_OUT("char=%d,%c\n", key, (char)key);

    if (VK_SPACE == key) {
        win->dm->scrollYByAreaDy(true, true);
    } else if (VK_BACK == key) {
        win->dm->scrollYByAreaDy(false, true);
    } else if ('g' == key) {
        OnMenuGoToPage(win);
    } else if ('k' == key) {
        SendMessage(win->hwndCanvas, WM_VSCROLL, SB_LINEDOWN, 0);
    } else if ('j' == key) {
        SendMessage(win->hwndCanvas, WM_VSCROLL, SB_LINEUP, 0);
    } else if ('n' == key) {
        win->dm->goToNextPage(0);
    } else if ('c' == key) {
        // TODO: probably should preserve facing vs. non-facing
        win->dm->changeDisplayMode(DM_CONTINUOUS);
    } else if ('p' == key) {
        win->dm->goToPrevPage(0);
    } else if ('z' == key) {
        WindowInfo_ToggleZoom(win);
    } else if ('q' == key) {
        DestroyWindow(win->hwndFrame);
    } else if ('+' == key) {
            win->dm->zoomBy(ZOOM_IN_FACTOR);
    } else if ('-' == key) {
            win->dm->zoomBy(ZOOM_OUT_FACTOR);
    } else if ('r' == key) {
        ReloadPdfDocument(win);
    }
}

static inline bool IsEnumPrintersArg(const char *txt)
{
    if (str_ieq(txt, ENUM_PRINTERS_ARG_TXT))
        return true;
    return false;
}

static inline bool IsDontRegisterExtArg(const char *txt)
{
    if (str_ieq(txt, NO_REGISTER_EXT_ARG_TXT))
        return true;
    return false;
}

static inline bool IsPrintToArg(const char *txt)
{
    if (str_ieq(txt, PRINT_TO_ARG_TXT))
        return true;
    return false;
}

static inline bool IsPrintToDefaultArg(const char *txt)
{
    if (str_ieq(txt, PRINT_TO_ARG_TXT))
        return true;
    return false;
}

static inline bool IsExitOnPrintArg(const char *txt)
{
    if (str_ieq(txt, EXIT_ON_PRINT_ARG_TXT))
        return true;
    return false;
}

static inline bool IsBenchArg(const char *txt)
{
    if (str_ieq(txt, BENCH_ARG_TXT))
        return true;
    return false;
}

static bool IsBenchMode(void)
{
    if (NULL != gBenchFileName)
        return true;
    return false;
}

/* Find a file in a file history list that has a given 'menuId'.
   Return a copy of filename or NULL if couldn't be found.
   It's used to figure out if a menu item selected by the user
   is one of the "recent files" menu items in File menu.
   Caller needs to free() the memory.
   */
static const char *RecentFileNameFromMenuItemId(UINT  menuId) {
    FileHistoryList* curr = gFileHistoryRoot;
    while (curr) {
        if (curr->menuId == menuId)
            return str_dup(curr->state.filePath);
        curr = curr->next;
    }
    return NULL;
}

#define FRAMES_PER_SECS 60
#define ANIM_FREQ_IN_MS  1000 / FRAMES_PER_SECS

static void OnMenuAbout() {
    if (gHwndAbout) {
        SetActiveWindow(gHwndAbout);
        return;
    }
    gHwndAbout = CreateWindow(
            ABOUT_CLASS_NAME, (TCHAR*)ABOUT_WIN_TITLE,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            ABOUT_WIN_DX, ABOUT_WIN_DY,
            NULL, NULL,
            ghinst, NULL); /* @note: TCHAR* cast */
    if (!gHwndAbout)
        return;
    ShowWindow(gHwndAbout, SW_SHOW);
}

BOOL PrivateIsAppThemed() {
    BOOL isThemed = FALSE;
    HMODULE hDll = LoadLibrary(TEXT("uxtheme.dll")); /* @note: TEXT() cast */
    if (!hDll) return FALSE;

    FARPROC fp = GetProcAddress(hDll, "IsAppThemed");
    if (fp)
        isThemed = fp();

    FreeLibrary(hDll);
    return isThemed;
}

static TBBUTTON TbButtonFromButtonInfo(int i) {
    TBBUTTON tbButton = {0};
    if (IDB_SEPARATOR == gToolbarButtons[i].cmdId) {
        tbButton.fsStyle = TBSTYLE_SEP;
    } else {
        tbButton.iBitmap = gToolbarButtons[i].index;
        tbButton.idCommand = gToolbarButtons[i].cmdId;
        tbButton.fsState = TBSTATE_ENABLED;
        tbButton.fsStyle = TBSTYLE_BUTTON;
        tbButton.iString = (INT_PTR)gToolbarButtons[i].toolTip;
    }
    return tbButton;
}

#define WS_TOOLBAR (WS_CHILD | WS_CLIPSIBLINGS | \
                    TBSTYLE_TOOLTIPS | TBSTYLE_FLAT | \
                    TBSTYLE_LIST | CCS_NODIVIDER | CCS_NOPARENTALIGN )

static void CreateToolbar(WindowInfo *win, HINSTANCE hInst) {
    BOOL            bIsAppThemed = PrivateIsAppThemed();

    HWND hwndOwner = win->hwndFrame;
    HWND hwndToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, WS_TOOLBAR,
                                 0,0,0,0, hwndOwner,(HMENU)IDC_TOOLBAR, hInst,NULL);
    win->hwndToolbar = hwndToolbar;
    LRESULT lres = SendMessage(hwndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

    ShowWindow(hwndToolbar, SW_SHOW);
    HIMAGELIST himl = 0;
    TBBUTTON tbButtons[TOOLBAR_BUTTONS_COUNT];
    for (int i=0; i < TOOLBAR_BUTTONS_COUNT; i++) {
        if (IDB_SEPARATOR != gToolbarButtons[i].bitmapResourceId) {
            HBITMAP hbmp = LoadBitmap(hInst, MAKEINTRESOURCE(gToolbarButtons[i].bitmapResourceId));
            if (!himl) {
                BITMAP bmp;
                GetObject(hbmp, sizeof(BITMAP), &bmp);
                int dx = bmp.bmWidth;
                int dy = bmp.bmHeight;
                himl = ImageList_Create(dx, dy, ILC_COLORDDB | ILC_MASK, 0, 0);
            }
            int index = ImageList_AddMasked(himl, hbmp, RGB(255,0,255));
            DeleteObject(hbmp);
            gToolbarButtons[i].index = index;
        }
        tbButtons[i] = TbButtonFromButtonInfo(i);
    }
    lres = SendMessage(hwndToolbar, TB_SETIMAGELIST, 0, (LPARAM)himl);

    // TODO: construct disabled image list as well?
    //SendMessage(hwndToolbar, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)himl);

    LRESULT exstyle = SendMessage(hwndToolbar, TB_GETEXTENDEDSTYLE, 0, 0);
    exstyle |= TBSTYLE_EX_MIXEDBUTTONS;
    lres = SendMessage(hwndToolbar, TB_SETEXTENDEDSTYLE, 0, exstyle);

    lres = SendMessage(hwndToolbar, TB_ADDBUTTONS, TOOLBAR_BUTTONS_COUNT, (LPARAM)tbButtons);
    RECT rc;
    lres = SendMessage(hwndToolbar, TB_GETITEMRECT, 0, (LPARAM)&rc);

    DWORD  reBarStyle = WS_REBAR | WS_VISIBLE;
    win->hwndReBar = CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL, reBarStyle,
                             0,0,0,0, hwndOwner, (HMENU)IDC_REBAR, hInst, NULL);
    if (!win->hwndReBar)
        SeeLastError();

    REBARINFO rbi;
    rbi.cbSize = sizeof(REBARINFO);
    rbi.fMask  = 0;
    rbi.himl   = (HIMAGELIST)NULL;
    lres = SendMessage(win->hwndReBar, RB_SETBARINFO, 0, (LPARAM)&rbi);

    REBARBANDINFO rbBand;
    rbBand.cbSize  = sizeof(REBARBANDINFO);
    rbBand.fMask   = /*RBBIM_COLORS | RBBIM_TEXT | RBBIM_BACKGROUND | */
                   RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE /*| RBBIM_SIZE*/;
    rbBand.fStyle  = /*RBBS_CHILDEDGE |*//* RBBS_BREAK |*/ RBBS_FIXEDSIZE /*| RBBS_GRIPPERALWAYS*/;
    if (bIsAppThemed)
        rbBand.fStyle |= RBBS_CHILDEDGE;
    rbBand.hbmBack = NULL;
    rbBand.lpText     = TEXT("Toolbar");
    rbBand.hwndChild  = hwndToolbar;
    rbBand.cxMinChild = (rc.right - rc.left) * TOOLBAR_BUTTONS_COUNT;
    rbBand.cyMinChild = (rc.bottom - rc.top) + 2 * rc.top;
    rbBand.cx         = 0;
    lres = SendMessage(win->hwndReBar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);

    SetWindowPos(win->hwndReBar, NULL, 0, 0, 0, 0, SWP_NOZORDER);
    GetWindowRect(win->hwndReBar, &rc);
    gReBarDy = rc.bottom - rc.top;
    //TODO: this was inherited but doesn't seem to be right (makes toolbar
    // partially unpainted if using classic scheme on xp or vista
    //gReBarDyFrame = bIsAppThemed ? 0 : 2;
    gReBarDyFrame = 0;
}

static LRESULT CALLBACK WndProcAbout(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:
            assert(!gHwndAbout);
            break;

        case WM_ERASEBKGND:
            // do nothing, helps to avoid flicker
            return TRUE;

        case WM_PAINT:
            OnPaintAbout(hwnd);
            break;

        case WM_DESTROY:
            assert(gHwndAbout);
            gHwndAbout = NULL;
            break;

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

/* TODO: gAccumDelta must be per WindowInfo */
static int      gDeltaPerLine, gAccumDelta;      // for mouse wheel logic

static LRESULT CALLBACK WndProcCanvas(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WindowInfo *    win;
    win = WindowInfo_FindByHwnd(hwnd);
    switch (message)
    {
        case WM_APP_REPAINT_DELAYED:
            if (win)
                SetTimer(win->hwndCanvas, REPAINT_TIMER_ID, REPAINT_DELAY_IN_MS, NULL);
            break;

        case WM_APP_REPAINT_NOW:
            if (win)
                WindowInfo_RedrawAll(win);
            break;

        case WM_VSCROLL:
            OnVScroll(win, wParam);
            return WM_VSCROLL_HANDLED;

        case WM_HSCROLL:
            OnHScroll(win, wParam);
            return WM_HSCROLL_HANDLED;

        case WM_MOUSEMOVE:
            if (win)
                OnMouseMove(win, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);
            break;

        case WM_LBUTTONDOWN:
            if (win)
                OnMouseLeftButtonDown(win, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            break;

        case WM_LBUTTONUP:
            if (win)
                OnMouseLeftButtonUp(win, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            break;

        case WM_RBUTTONDOWN:
            if (win)
                OnMouseRightButtonDown(win, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            break;

        case WM_RBUTTONUP:
            if (win)
                OnMouseRightButtonUp(win, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            break;

        case WM_SETCURSOR:
            if (win && win->mouseAction == MA_DRAGGING) {
                SetCursor(gCursorDrag);
                return TRUE;
            }
            break;

        case WM_TIMER:
            assert(win);
            if (win) {
                if (REPAINT_TIMER_ID == wParam)
                    WindowInfo_RedrawAll(win);
                else
                    AnimState_NextFrame(&win->animState);
            }
            break;

        case WM_DROPFILES:
            if (win)
                OnDropFiles(win, (HDROP)wParam);
            break;

        case WM_ERASEBKGND:
            // do nothing, helps to avoid flicker
            return TRUE;

        case WM_PAINT:
            /* it might happen that we get WM_PAINT after destroying a window */
            if (win) {
                /* blindly kill the timer, just in case it's there */
                KillTimer(win->hwndCanvas, REPAINT_TIMER_ID);
                OnPaint(win);
            }
            break;

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

static LRESULT CALLBACK WndProcFrame(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int             wmId, wmEvent;
    WindowInfo *    win;
    ULONG           ulScrollLines;                   // for mouse wheel logic
    const char *    fileName;

    win = WindowInfo_FindByHwnd(hwnd);

    switch (message)
    {
        case WM_CREATE:
            // do nothing
            goto InitMouseWheelInfo;

        case WM_SIZE:
            if (win) {
                int dx = LOWORD(lParam);
                int dy = HIWORD(lParam);
                OnSize(win, dx, dy);
            }
            break;

        case WM_COMMAND:
            wmId    = LOWORD(wParam);
            wmEvent = HIWORD(wParam);

            fileName = RecentFileNameFromMenuItemId(wmId);
            if (fileName) {
                LoadPdf(fileName);
                free((void*)fileName);
                break;
            }

            switch (wmId)
            {
                case IDM_OPEN:
                case IDT_FILE_OPEN:
                    OnMenuOpen(win);
                    break;
                case IDM_SAVEAS:
                    OnMenuSaveAs(win);
                    break;

                case IDT_FILE_PRINT:
                case IDM_PRINT:
                    OnMenuPrint(win);
                    break;

                case IDM_MAKE_DEFAULT_READER:
                    OneMenuMakeDefaultReader();
                    break;

                case IDT_FILE_EXIT:
                case IDM_CLOSE:
                    CloseWindow(win, FALSE);
                    break;

                case IDM_EXIT:
                    OnMenuExit();
                    break;

                case IDT_VIEW_ZOOMIN:
                    if (win->dm)
                        win->dm->zoomBy(ZOOM_IN_FACTOR);
                    break;

                case IDT_VIEW_ZOOMOUT:
                    if (win->dm)
                        win->dm->zoomBy(ZOOM_OUT_FACTOR);
                    break;

                case IDM_ZOOM_6400:
                case IDM_ZOOM_3200:
                case IDM_ZOOM_1600:
                case IDM_ZOOM_800:
                case IDM_ZOOM_400:
                case IDM_ZOOM_200:
                case IDM_ZOOM_150:
                case IDM_ZOOM_125:
                case IDM_ZOOM_100:
                case IDM_ZOOM_50:
                case IDM_ZOOM_25:
                case IDM_ZOOM_12_5:
                case IDM_ZOOM_8_33:
                case IDM_ZOOM_FIT_PAGE:
                case IDM_ZOOM_FIT_WIDTH:
                case IDM_ZOOM_ACTUAL_SIZE:
                    OnMenuZoom(win, (UINT)wmId);
                    break;

                case IDM_ZOOM_FIT_VISIBLE:
                    /* TODO: implement me */
                    break;

                case IDM_VIEW_SINGLE_PAGE:
                    OnMenuViewSinglePage(win);
                    break;

                case IDM_VIEW_FACING:
                    OnMenuViewFacing(win);
                    break;

                case IDM_VIEW_CONTINUOUS:
                    OnMenuViewContinuous(win);
                    break;

                case IDM_VIEW_SHOW_HIDE_TOOLBAR:
                    OnMenuViewShowHideToolbar();
                    break;

                case IDM_VIEW_USE_FITZ:
                    OnMenuViewUseFitz(win);
                    break;

                case IDM_GOTO_NEXT_PAGE:
                    OnMenuGoToNextPage(win);
                    break;

                case IDM_GOTO_PREV_PAGE:
                    OnMenuGoToPrevPage(win);
                    break;

                case IDM_GOTO_FIRST_PAGE:
                    OnMenuGoToFirstPage(win);
                    break;

                case IDM_GOTO_LAST_PAGE:
                    OnMenuGoToLastPage(win);
                    break;

                case IDM_GOTO_PAGE:
                    OnMenuGoToPage(win);
                    break;

                case IDM_VIEW_CONTINUOUS_FACING:
                    OnMenuViewContinuousFacing(win);
                    break;

                case IDM_VIEW_ROTATE_LEFT:
                    OnMenuViewRotateLeft(win);
                    break;

                case IDM_VIEW_ROTATE_RIGHT:
                    OnMenuViewRotateRight(win);
                    break;

                case IDM_VISIT_WEBSITE:
                    LaunchBrowser(_T("http://blog.kowalczyk.info/software/sumatrapdf/"));
                    break;

                case IDM_LANG_EN:
                case IDM_LANG_PL:
                case IDM_LANG_FR:
                case IDM_LANG_DE:
                    OnMenuLanguage((int)wmId);
                    break;

                case IDM_ABOUT:
                    OnMenuAbout();
                    break;
                default:
                    return DefWindowProc(hwnd, message, wParam, lParam);
            }
            break;

        case WM_CHAR:
            if (win)
                OnChar(win, wParam);
            break;

        case WM_KEYDOWN:
            if (win)
                OnKeydown(win, wParam, lParam);
            break;

        case WM_SETTINGCHANGE:
InitMouseWheelInfo:
            SystemParametersInfo (SPI_GETWHEELSCROLLLINES, 0, &ulScrollLines, 0);
            // ulScrollLines usually equals 3 or 0 (for no scrolling)
            // WHEEL_DELTA equals 120, so iDeltaPerLine will be 40
            if (ulScrollLines)
                gDeltaPerLine = WHEEL_DELTA / ulScrollLines;
            else
                gDeltaPerLine = 0;
            return 0;

        // TODO: I don't understand why WndProcCanvas() doesn't receive this message
        case WM_MOUSEWHEEL:
            if (!win || !win->dm) /* TODO: check for pdfDoc as well ? */
                break;

            if (gDeltaPerLine == 0)
               break;

            gAccumDelta += (short) HIWORD (wParam);     // 120 or -120

            while (gAccumDelta >= gDeltaPerLine)
            {
                SendMessage(win->hwndCanvas, WM_VSCROLL, SB_LINEUP, 0);
                gAccumDelta -= gDeltaPerLine;
            }

            while (gAccumDelta <= -gDeltaPerLine)
            {
                SendMessage(win->hwndCanvas, WM_VSCROLL, SB_LINEDOWN, 0);
                gAccumDelta += gDeltaPerLine;
            }
            return 0;

        case WM_DROPFILES:
            if (win)
                OnDropFiles(win, (HDROP)wParam);
            break;

        case WM_DESTROY:
            /* WM_DESTROY might be sent as a result of File\Close, in which case CloseWindow() has already been called */
            if (win)
                CloseWindow(win, TRUE);
            break;

        case IDM_VIEW_WITH_ACROBAT:
            if (win)
                ViewWithAcrobat(win);
            break;

        case MSG_BENCH_NEXT_ACTION:
            if (win)
                OnBenchNextAction(win);
            break;

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

static BOOL RegisterWinClass(HINSTANCE hInstance)
{
    WNDCLASSEX  wcex;
    ATOM        atom;

    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProcFrame;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SUMATRAPDF));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = NULL;
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = FRAME_CLASS_NAME;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    atom = RegisterClassEx(&wcex);
    if (!atom)
        return FALSE;

    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProcCanvas;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = 0;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = NULL;
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = CANVAS_CLASS_NAME;
    wcex.hIconSm        = 0;
    atom = RegisterClassEx(&wcex);
    if (!atom)
        return FALSE;

    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProcAbout;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = 0;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = NULL;
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = ABOUT_CLASS_NAME;
    wcex.hIconSm        = 0;
    atom = RegisterClassEx(&wcex);
    if (!atom)
        return FALSE;

    return TRUE;
}

#define IDC_HAND            MAKEINTRESOURCE(32649)
static BOOL InstanceInit(HINSTANCE hInstance, int nCmdShow)
{
    ghinst = hInstance;

    globalParams = new GlobalParams("");
    if (!globalParams)
        return FALSE;

    SplashColorsInit();
    gCursorArrow = LoadCursor(NULL, IDC_ARROW);
    gCursorHand  = LoadCursor(NULL, IDC_HAND); // apparently only available if WINVER >= 0x0500
    if (!gCursorHand)
        gCursorHand = LoadCursor(ghinst, MAKEINTRESOURCE(IDC_CURSORDRAG));
    gCursorDrag  = LoadCursor(ghinst, MAKEINTRESOURCE(IDC_CURSORDRAG));
    gBrushBg     = CreateSolidBrush(COL_WINDOW_BG);
    gBrushWhite  = CreateSolidBrush(COL_WHITE);
    gBrushShadow = CreateSolidBrush(COL_WINDOW_SHADOW);
    gBrushLinkDebug = CreateSolidBrush(RGB(0x00,0x00,0xff));
    return TRUE;
}

static void StrList_Reverse(StrList **strListRoot)
{
    StrList *newRoot = NULL;
    StrList *cur, *next;
    if (!strListRoot)
        return;
    cur = *strListRoot;
    while (cur) {
        next = cur->next;
        cur->next = newRoot;
        newRoot = cur;
        cur = next;
    }
    *strListRoot = newRoot;
}

static BOOL StrList_InsertAndOwn(StrList **root, char *txt)
{
    StrList *   el;
    assert(root && txt);
    if (!root || !txt)
        return FALSE;

    el = (StrList*)malloc(sizeof(StrList));
    if (!el)
        return FALSE;
    el->str = txt;
    el->next = *root;
    *root = el;
    return TRUE;
}

static void StrList_Destroy(StrList **root)
{
    StrList *   cur;
    StrList *   next;

    if (!root)
        return;
    cur = *root;
    while (cur) {
        next = cur->next;
        free((void*)cur->str);
        free((void*)cur);
        cur = next;
    }
    *root = NULL;
}

static StrList *StrList_FromCmdLine(char *cmdLine)
{
    char *     exePath;
    StrList *   strList = NULL;
    char *      txt;

    assert(cmdLine);

    if (!cmdLine)
        return NULL;

    exePath = ExePathGet();
    if (!exePath)
        return NULL;
    if (!StrList_InsertAndOwn(&strList, exePath)) {
        free((void*)exePath);
        return NULL;
    }

    for (;;) {
        txt = str_parse_possibly_quoted(&cmdLine);
        if (!txt)
            break;
        if (!StrList_InsertAndOwn(&strList, txt)) {
            free((void*)txt);
            break;
        }
    }
    StrList_Reverse(&strList);
    return strList;
}

static void u_DoAllTests(void)
{
#ifdef DEBUG
    printf("Running tests\n");
    u_RectI_Intersect();
#else
    printf("Not running tests\n");
#endif
}

#define CONSERVE_MEMORY 1

static DWORD WINAPI PageRenderThread(PVOID data)
{
    PageRenderRequest   req;
    RenderedBitmap *    bmp;

    DBG_OUT("PageRenderThread() started\n");
    while (1) {
        //DBG_OUT("Worker: wait\n");
        LockCache();
        gCurPageRenderReq = NULL;
        int count = gPageRenderRequestsCount;
        UnlockCache();
        if (0 == count) {
            DWORD waitResult = WaitForSingleObject(gPageRenderSem, INFINITE);
            if (WAIT_OBJECT_0 != waitResult) {
                DBG_OUT("  WaitForSingleObject() failed\n");
                continue;
            }
        }
        if (0 == gPageRenderRequestsCount) {
            continue;
        }
        LockCache();
        RenderQueue_Pop(&req);
        gCurPageRenderReq = &req;
        UnlockCache();
        DBG_OUT("PageRenderThread(): dequeued %d\n", req.pageNo);
        if (!req.dm->pageVisibleNearby(req.pageNo)) {
            DBG_OUT("PageRenderThread(): not rendering because not visible\n");
            continue;
        }
        assert(!req.abort);
        MsTimer renderTimer;
        bmp = req.dm->renderBitmap(req.pageNo, req.zoomLevel, req.rotation, pageRenderAbortCb, (void*)&req);
        renderTimer.stop();
        LockCache();
        gCurPageRenderReq = NULL;
        UnlockCache();
        if (req.abort) {
            delete bmp;
            continue;
        }
        if (bmp)
            DBG_OUT("PageRenderThread(): finished rendering %d\n", req.pageNo);
        else
            DBG_OUT("PageRenderThread(): failed to render a bitmap of page %d\n", req.pageNo);
        double renderTime = renderTimer.timeInMs();
        BitmapCache_Add(req.dm, req.pageNo, req.zoomLevel, req.rotation, bmp, renderTime);
#ifdef CONSERVE_MEMORY
        BitmapCache_FreeNotVisible();
#endif
        WindowInfo* win = (WindowInfo*)req.dm->appData();
        triggerRepaintDisplayNow(win);
    }
    DBG_OUT("PageRenderThread() finished\n");
    return 0;
}

static void CreatePageRenderThread(void)
{
    LONG semMaxCount = 1000; /* don't really know what the limit should be */
    DWORD dwThread1ID = 0;
    assert(NULL == gPageRenderThreadHandle);

    gPageRenderSem = CreateSemaphore(NULL, 0, semMaxCount, NULL);
    gPageRenderThreadHandle = CreateThread(NULL, 0, PageRenderThread, (void*)NULL, 0, &dwThread1ID);
    assert(NULL != gPageRenderThreadHandle);
}

static void PrintFile(WindowInfo *win, const char *fileName, const char *printerName)
{
    char        devstring[256];      // array for WIN.INI data 
    HANDLE      printer;
    LPDEVMODE   devMode = NULL;
    DWORD       structSize, returnCode;

    if (!win->dm->pdfEngine()->printingAllowed()) {               /* @note: TCHAR* and TEXT() casts */
        MessageBox(win->hwndFrame, TEXT("Cannot print this file"), TEXT("Printing problem."), MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    // Retrieve the printer, printer driver, and 
    // output-port names from WIN.INI. 
    GetProfileString(TEXT("Devices"), (TCHAR*)printerName, TEXT(""), (TCHAR*)devstring, sizeof(devstring));

    // Parse the string of names, setting ptrs as required 
    // If the string contains the required names, use them to 
    // create a device context. 
    char *driver = strtok (devstring, (const char *) ",");
    char *port = strtok((char *) NULL, (const char *) ",");

    if (!driver || !port) {
        MessageBox(win->hwndFrame, TEXT("Printer with given name doesn't exist"), TEXT("Printing problem."), MB_ICONEXCLAMATION | MB_OK);
        return;
    }
    
    BOOL fOk = OpenPrinter((TCHAR*)printerName, &printer, NULL); /* @note: neither LPCTSTR nor LPCTSTR work => TCHAR* cast */
    if (!fOk) {
	 /* @note: translation need some care */
        //MessageBox(win->hwndFrame, _TR("Could not open Printer"), _TR("Printing problem."), MB_ICONEXCLAMATION | MB_OK);
        MessageBox(win->hwndFrame, TEXT("Could not open Printer"), TEXT("Printing problem."), MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    HDC  hdcPrint = NULL;
    structSize = DocumentProperties(NULL,
        printer,                /* Handle to our printer. */ 
        (TCHAR*) printerName,    /* Name of the printer. */  /* @note: neither LPCTSTR nor LPCTSTR work => TCHAR* cast */
        NULL,                   /* Asking for size, so */ 
        NULL,                   /* these are not used. */ 
        0);                     /* Zero returns buffer size. */ 
    devMode = (LPDEVMODE)malloc(structSize);
    if (!devMode) {
         /* @note: "crosses initialization of [...]" issues */
        //goto Exit;
	free(devMode);
	DeleteDC(hdcPrint);
    }

    // Get the default DevMode for the printer and modify it for your needs.
    returnCode = DocumentProperties(NULL,
        printer,
        (TCHAR*) printerName,    /* Name of the printer. */  /* @note: neither LPCTSTR nor LPCTSTR work => TCHAR* cast */
        devMode,        /* The address of the buffer to fill. */ 
        NULL,           /* Not using the input buffer. */ 
        DM_OUT_BUFFER); /* Have the output buffer filled. */ 

    if (IDOK != returnCode) {
        // If failure, inform the user, cleanup and return failure.
        MessageBox(win->hwndFrame, TEXT("Could not obtain Printer properties"), TEXT("Printing problem."), MB_ICONEXCLAMATION | MB_OK);
         /* @note: "crosses initialization of [...]" issues */
        //goto Exit;
	free(devMode);
	DeleteDC(hdcPrint);
    }

    PdfPageInfo * pageInfo = pageInfo = win->dm->getPageInfo(1);

    if (pageInfo->bitmapDx > pageInfo->bitmapDy) {
        devMode->dmOrientation = DMORIENT_LANDSCAPE;
    } else {
        devMode->dmOrientation = DMORIENT_PORTRAIT;
    }

    /*
     * Merge the new settings with the old.
     * This gives the driver an opportunity to update any private
     * portions of the DevMode structure.
     */ 
     DocumentProperties(NULL,
        printer,
        (TCHAR*) printerName,    /* Name of the printer. */  /* @note: neither LPCTSTR nor LPCTSTR work => TCHAR* cast */
        devMode,        /* Reuse our buffer for output. */ 
        devMode,        /* Pass the driver our changes. */ 
        DM_IN_BUFFER |  /* Commands to Merge our changes and */ 
        DM_OUT_BUFFER); /* write the result. */ 

    ClosePrinter(printer);

    hdcPrint = CreateDC((TCHAR*)driver, (TCHAR*)printerName, (TCHAR*)port, devMode); 
    if (!hdcPrint) {
        MessageBox(win->hwndFrame, TEXT("Couldn't initialize printer"), TEXT("Printing problem."), MB_ICONEXCLAMATION | MB_OK);
         /* @note: "crosses initialization of [...]" issues */
        //goto Exit;
	free(devMode);
	DeleteDC(hdcPrint);
    }

    if (CheckPrinterStretchDibSupport(win->hwndFrame, hdcPrint))
        PrintToDevice(win->dm, hdcPrint, devMode, 1, win->dm->pageCount());
Exit:
    free(devMode);
    DeleteDC(hdcPrint);
}

static void EnumeratePrinters()
{
    PRINTER_INFO_5 *info5Arr = NULL;
    DWORD bufSize = 0, printersCount;
    BOOL fOk = EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL, 
        5, (LPBYTE)info5Arr, bufSize, &bufSize, &printersCount);
    if (!fOk) {
        info5Arr = (PRINTER_INFO_5*)malloc(bufSize);
        fOk = EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL, 
        5, (LPBYTE)info5Arr, bufSize, &bufSize, &printersCount);
    }
    if (!info5Arr)
        return;
    assert(fOk);
    if (!fOk) return;
    printf("Printers: %d\n", printersCount);
    for (DWORD i=0; i < printersCount; i++) {
        const char *printerName = (char*)info5Arr[i].pPrinterName; /* @note: char* cast */
        const char *printerPort = (char*)info5Arr[i].pPortName; /* @note: char* cast */
        bool fDefault = false;
        if (info5Arr[i].Attributes & PRINTER_ATTRIBUTE_DEFAULT)
            fDefault = true;
        printf("Name: %s, port: %s, default: %d\n", printerName, printerPort, (int)fDefault);
    }
    TCHAR buf[512];
    bufSize = sizeof(buf);
    fOk = GetDefaultPrinter(buf, &bufSize);
    if (!fOk) {
        if (ERROR_FILE_NOT_FOUND == GetLastError())
            printf("No default printer\n");
    }
    free(info5Arr);
}

/* Get the name of default printer or NULL if not exists.
   The caller needs to free() the result */
char *GetDefaultPrinterName()
{
    char buf[512];
    DWORD bufSize = sizeof(buf);
    if (GetDefaultPrinterA(buf, &bufSize))
        return str_dup(buf);
    return NULL;
}     

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    StrList *           argListRoot;
    StrList *           currArg;
    char *              benchPageNumStr = NULL;
    MSG                 msg = {0};
    HACCEL              hAccelTable;
    WindowInfo*         win;
    int                 pdfOpened = 0;
    bool                exitOnPrint = false;
    bool                printToDefaultPrinter = false;

    UNREFERENCED_PARAMETER(hPrevInstance);

    u_DoAllTests();

    INITCOMMONCONTROLSEX cex;
    cex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    cex.dwICC = ICC_WIN95_CLASSES | ICC_DATE_CLASSES | ICC_USEREX_CLASSES | ICC_COOL_CLASSES;
    InitCommonControlsEx(&cex);

    argListRoot = StrList_FromCmdLine((char*)lpCmdLine); /* @note: char* cast */
    assert(argListRoot);
    if (!argListRoot)
        return 0;

    Prefs_Load();
    /* parse argument list. If BENCH_ARG_TXT was given, then we're in benchmarking mode. Otherwise
    we assume that all arguments are PDF file names.
    BENCH_ARG_TXT can be followed by file or directory name. If file, it can additionally be followed by
    a number which we interpret as page number */
    bool registerForPdfExtentions = true;
    currArg = argListRoot->next;
    char *printerName = NULL;
    while (currArg) {
        if (IsEnumPrintersArg(currArg->str)) {
            EnumeratePrinters();
            /* this is for testing only, exit immediately */
            goto Exit;
        }

        if (IsDontRegisterExtArg(currArg->str)) {
            registerForPdfExtentions = false;
            currArg = currArg->next;
            continue;
        }

        if (IsBenchArg(currArg->str)) {
            currArg = currArg->next;
            if (currArg) {
                gBenchFileName = currArg->str;
                if (currArg->next)
                    benchPageNumStr = currArg->next->str;
            }
            break;
        }

        if (IsExitOnPrintArg(currArg->str)) {
            currArg = currArg->next;
            exitOnPrint = true;
            continue;
        }

        if (IsPrintToDefaultArg(currArg->str)) {
            printToDefaultPrinter = true;
            continue;
        }

        if (IsPrintToArg(currArg->str)) {
            currArg = currArg->next;
            if (currArg) {
                printerName = currArg->str;
                currArg = currArg->next;
            }
            continue;
        }

        // we assume that switches come first and file names to open later
        // TODO: it would probably be better to collect all non-switches
        // in a separate list so that file names can be interspersed with
        // switches
        break;
    }

    if (benchPageNumStr) {
        gBenchPageNum = atoi(benchPageNumStr);
        if (gBenchPageNum < 1)
            gBenchPageNum = INVALID_PAGE_NO;
    }

    LoadString(hInstance, IDS_APP_TITLE, windowTitle, MAX_LOADSTRING);
    if (!RegisterWinClass(hInstance))
        goto Exit;

    CaptionPens_Create();
    if (!InstanceInit(hInstance, nCmdShow))
        goto Exit;

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SUMATRAPDF));

    CreatePageRenderThread();
    /* remaining arguments are names of PDF files */
    if (NULL != gBenchFileName) {
            win = LoadPdf(gBenchFileName);
            if (win)
                ++pdfOpened;
    } else {
        while (currArg) {
            win = LoadPdf(currArg->str);
            if (!win)
                goto Exit;

            if (exitOnPrint)
                ShowWindow(win->hwndFrame, SW_HIDE);

            if (printToDefaultPrinter) {
                printerName = GetDefaultPrinterName();
                if (printerName)
                    PrintFile(win, currArg->str, printerName);
                free(printerName);
            } else if (printerName) {
                // note: this prints all of PDF files. Another option would be to
                // print only the first one
                PrintFile(win, currArg->str, printerName);
            }
           ++pdfOpened;
            currArg = currArg->next;
        }
    }

    if (printerName && exitOnPrint)
        goto Exit;
 
    if (0 == pdfOpened) {
        /* disable benchmark mode if we couldn't open file to benchmark */
        gBenchFileName = 0;
#ifdef REOPEN_FILES_AT_STARTUP
            FileHistoryList * currFile = gFileHistoryRoot;
        while (currFile) {
            if (currFile->state.visible) {
                win = LoadPdf(currFile->state.filePath, false);
                if (win)
                    ++pdfOpened;
            }
            currFile = currFile->next;
        }
#endif
        if (0 == pdfOpened) {
            win = WindowInfo_CreateEmpty();
            if (!win)
                goto Exit;
            WindowInfoList_Add(win);

            /* TODO: should this be part of WindowInfo_CreateEmpty() ? */
            DragAcceptFiles(win->hwndFrame, TRUE);
            ShowWindow(win->hwndCanvas, SW_SHOW);
            UpdateWindow(win->hwndCanvas);
            ShowWindow(win->hwndFrame, SW_SHOW);
            UpdateWindow(win->hwndFrame);
        }
    }

    if (IsBenchMode()) {
        assert(win);
        assert(pdfOpened > 0);
        if (win)
            PostBenchNextAction(win->hwndFrame);
    }

    if (0 == pdfOpened)
        MenuToolbarUpdateStateForAllWindows();

    if (registerForPdfExtentions)
        RegisterForPdfExtentions(win ? win->hwndFrame : NULL);

    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

Exit:
    WindowInfoList_DeleteAll();
    FileHistoryList_Free(&gFileHistoryRoot);
    CaptionPens_Destroy();
    DeleteObject(gBrushBg);
    DeleteObject(gBrushWhite);
    DeleteObject(gBrushShadow);
    DeleteObject(gBrushLinkDebug);

    delete globalParams;
    StrList_Destroy(&argListRoot);
    Translations_FreeData();
    CurrLangNameFree();
    //histDump();
    return (int) msg.wParam;
}

// Code for DLL interace
static WindowInfo* CreateEmpty(HWND parentHandle) {
    WindowInfo* pdfWin;
    HWND        hwndCanvas;
    pdfWin = WindowInfo_New(parentHandle);
    hwndCanvas = CreateWindow(
        CANVAS_CLASS_NAME, NULL,
        WS_CHILD | WS_HSCROLL | WS_VSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT,
        DEF_WIN_DX, DEF_WIN_DY,
        parentHandle, NULL,
        ghinst, NULL);
    if (hwndCanvas)
        pdfWin->hwndCanvas = hwndCanvas;
    return pdfWin;
}

static void OpenPdf(WindowInfo* pdfWin,const char *fileName,  HWND parentHandle)
{
    assert(fileName);
    if (!fileName) return;
    assert(pdfWin);
    if (!pdfWin) return;

    pdfWin->GetCanvasSize();
    SizeI maxCanvasSize = GetMaxCanvasSize(pdfWin);
    SizeD totalDrawAreaSize(pdfWin->winSize());
    DisplayMode displayMode = DEFAULT_DISPLAY_MODE;
    int offsetX = 0;
    int offsetY = 0;
    int startPage = 1;
    int scrollbarYDx = 0;
    int scrollbarXDy = 0;

    if (gUseFitz) {
        pdfWin->dm = DisplayModelFitz_CreateFromFileName(fileName, 
            totalDrawAreaSize, scrollbarYDx, scrollbarXDy, displayMode, startPage, pdfWin);
    } 
    else {
        pdfWin->dm = DisplayModelSplash_CreateFromFileName(fileName, 
            totalDrawAreaSize, scrollbarYDx, scrollbarXDy, displayMode, startPage, pdfWin);
    }

    pdfWin->dm->setAppData((void*)pdfWin);
    pdfWin->state = WS_SHOWING_PDF;
    double zoomVirtual = DEFAULT_ZOOM;
    int rotation = DEFAULT_ROTATION;

    UINT menuId = MenuIdFromVirtualZoom(zoomVirtual);
    ZoomMenuItemCheck(GetMenu(pdfWin->hwndFrame), menuId);

    pdfWin->dm->relayout(zoomVirtual, rotation);
    if (!pdfWin->dm->validPageNo(startPage))
        startPage = 1;
    offsetY = 0;
    pdfWin->dm->goToPage(startPage, offsetY, offsetX);
    WindowInfo_ResizeToPage(pdfWin, startPage);
    WindowInfoList_Add(pdfWin);

    RECT rect;
    if (GetWindowRect(pdfWin->hwndFrame , &rect) != 0)
    {
        int nWidth = rect_dx(&rect);
        int nHeight = rect_dy(&rect);
        WinResizeClientArea(pdfWin->hwndCanvas, nWidth, nHeight);
    }

    ShowWindow(pdfWin->hwndFrame, SW_SHOW);
    ShowWindow(pdfWin->hwndCanvas, SW_SHOW);
    UpdateWindow(pdfWin->hwndFrame);
    UpdateWindow(pdfWin->hwndCanvas);
}

void Sumatra_LoadPDF(WindowInfo* pdfWin, const char *pdfFile)
{
    int  pdfOpened = 0;
    OpenPdf(pdfWin, pdfFile, pdfWin->hwndFrame);
    ++pdfOpened;
    if (pdfWin)
        ShowWindow(pdfWin->hwndFrame, SW_SHOWNORMAL);
}

void Sumatra_PrintPDF(WindowInfo* pdfWin, const char *pdfFile, long showOptionWindow)
{
}

void Sumatra_Print(WindowInfo* pdfWin)
{
    if (WindowInfo_PdfLoaded(pdfWin))
        OnMenuPrint(pdfWin);
}

void Sumatra_ShowPrintDialog(WindowInfo* pdfWin)
{
    if (WindowInfo_PdfLoaded(pdfWin))
        OnMenuPrint(pdfWin);
}

void Sumatra_SetDisplayMode(WindowInfo* pdfWin,long displayMode)
{
    if (WindowInfo_PdfLoaded(pdfWin))
        SwitchToDisplayMode(pdfWin, (DisplayMode)displayMode);
}

long Sumatra_GoToNextPage(WindowInfo* pdfWin)
{
    if (!WindowInfo_PdfLoaded(pdfWin))
        return 0;
    pdfWin->dm->goToNextPage(0);
    return pdfWin->dm->currentPageNo();
}

long Sumatra_GoToPreviousPage(WindowInfo* pdfWin)
{
    if (!WindowInfo_PdfLoaded(pdfWin))
        return 0;
    pdfWin->dm->goToPrevPage(0);
    return pdfWin->dm->currentPageNo();
}

long Sumatra_GoToFirstPage(WindowInfo* pdfWin)
{
    if (!WindowInfo_PdfLoaded(pdfWin))
        return 0;
    pdfWin->dm->goToFirstPage();
    return pdfWin->dm->currentPageNo();
}

long Sumatra_GoToLastPage(WindowInfo* pdfWin)
{
    if (!WindowInfo_PdfLoaded(pdfWin))
        return 0;
    pdfWin->dm->goToLastPage();
    return pdfWin->dm->currentPageNo();
}

long Sumatra_GetNumberOfPages(WindowInfo* pdfWin)
{
    if (!WindowInfo_PdfLoaded(pdfWin))
        return 0;
    return pdfWin->dm->pageCount();
}

long Sumatra_GetCurrentPage(WindowInfo* pdfWin)
{
    if (!WindowInfo_PdfLoaded(pdfWin))
        return 0;
    return pdfWin->dm->currentPageNo();
}

long Sumatra_GoToThisPage(WindowInfo* pdfWin,long pageNumber)
{
    if (!WindowInfo_PdfLoaded(pdfWin))
        return 0;
    if (pdfWin->dm->validPageNo(pageNumber))
        pdfWin->dm->goToPage(pageNumber, 0);
    return pdfWin->dm->currentPageNo();
}

long Sumatra_ZoomIn(WindowInfo* pdfWin)
{
    if (WindowInfo_PdfLoaded(pdfWin))
    {
        long currentZoom = Sumatra_GetCurrentZoom(pdfWin);
        if (currentZoom < 500)
            Sumatra_SetZoom(pdfWin,currentZoom+10);
    }
    return Sumatra_GetCurrentZoom(pdfWin);
}

long Sumatra_ZoomOut(WindowInfo* pdfWin)
{
    if (WindowInfo_PdfLoaded(pdfWin))
    {
        long currentZoom = Sumatra_GetCurrentZoom(pdfWin);
        if (currentZoom > 10)
            Sumatra_SetZoom(pdfWin,currentZoom-10);
    }
    return Sumatra_GetCurrentZoom(pdfWin);
}

long Sumatra_SetZoom(WindowInfo* pdfWin,long zoomValue)
{
    if (WindowInfo_PdfLoaded(pdfWin))
        pdfWin->dm->zoomTo((double)zoomValue);
    return Sumatra_GetCurrentZoom(pdfWin);
}

long Sumatra_GetCurrentZoom(WindowInfo* pdfWin)
{
    double zoomLevel = 0;
    if (WindowInfo_PdfLoaded(pdfWin))
        zoomLevel = pdfWin->dm->zoomReal();
    return (long)zoomLevel;
} 

void Sumatra_Resize(WindowInfo* pdfWin)
{
    RECT rect;
    if (GetWindowRect(pdfWin->hwndFrame , &rect) != 0)
    {
        int nWidth = rect_dx(&rect);
        int nHeight = rect_dy(&rect);
        WinResizeClientArea(pdfWin->hwndCanvas, nWidth, nHeight);
    }
}

void Sumatra_ClosePdf(WindowInfo* pdfWin)
{
    if (WindowInfo_PdfLoaded(pdfWin))
        CloseWindow(pdfWin, FALSE);
}

WindowInfo* Sumatra_Init(HWND pHandle)
{
    WindowInfo* pdfWin;
    gRunningDLL = true;
    HINSTANCE hInstance = NULL;
    HINSTANCE hPrevInstance = NULL;
    int nCmdShow = 0;

    StrList *           argListRoot = NULL;
    StrList *           currArg = NULL;
    MSG                 msg = {0};
    bool                exitOnPrint = false;
    bool                printToDefaultPrinter = false;
    gUseFitz = TRUE;

    UNREFERENCED_PARAMETER(hPrevInstance);

    u_DoAllTests();

    INITCOMMONCONTROLSEX cex;
    cex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    cex.dwICC = ICC_WIN95_CLASSES | ICC_DATE_CLASSES | ICC_USEREX_CLASSES | ICC_COOL_CLASSES;
    InitCommonControlsEx(&cex);
    argListRoot = NULL;

    LoadString(hInstance, IDS_APP_TITLE, windowTitle, MAX_LOADSTRING);

    if (!RegisterWinClass(hInstance))
        Sumatra_Exit();

    CaptionPens_Create();

    if (!InstanceInit(hInstance, nCmdShow))
        Sumatra_Exit();

    CreatePageRenderThread();

    bool reuseExistingWindow = false;

    if (pHandle == 0 ) 
        pHandle = NULL;

    pdfWin = CreateEmpty(pHandle);

    return pdfWin;
}

void Sumatra_Exit()
{
    CaptionPens_Destroy();
    DeleteObject(gBrushBg);
    DeleteObject(gBrushWhite);
    DeleteObject(gBrushShadow);
    DeleteObject(gBrushLinkDebug);
    delete globalParams;
}
