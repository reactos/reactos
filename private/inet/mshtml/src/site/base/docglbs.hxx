#ifndef I_DOCGLBS_HXX_
#define I_DOCGLBS_HXX_
#pragma INCMSG("--- Beg 'docglbs.hxx'")

//----------------------------------------------------------------------------
//
//  Global functions                                            o
//
//----------------------------------------------------------------------------

void    InitFormClipFormats();
HRESULT InitFormClass();

TCHAR * GetFriendlyUrl(const TCHAR *pchUrl, const TCHAR *pchBaseUrl, BOOL fShowFriendlyUrl,
                       BOOL fPreface, LONG x = -1, LONG y = -1);

BOOL IsUrlOnNet(const TCHAR *pchUrl);

#if DBG == 1
void DisplayChildZOrder(HWND hwnd);
#endif

//----------------------------------------------------------------------------
//
//  Global variables
//
//----------------------------------------------------------------------------

extern SIZE                 g_sizeDragMin;
extern int                  g_iDragScrollDelay;
extern SIZE                 g_sizeDragScrollInset;
extern int                  g_iDragDelay;
extern int                  g_iDragScrollInterval;
extern BOOL                 g_fFormClassInitialized;
extern void *               g_pvLastMouseScrollObject;
extern BOOL                 g_fNoFileMenu;

// window message used by Windows 95, equivalent to WM_MOUSEWHEEL
//
extern UINT                 g_msgMouseWheel;

#ifdef NEVER
// The default size for controls

#define CXL_CTRL_DEFAULT (26 * 150)
#define CYL_CTRL_DEFAULT (26 * 40)
#endif

ExternTag(tagCDoc);
ExternTag(tagCDocPaint);

//
//  Global defines
//

#define COORD_MIN               (LONG_MIN + 65536)
#define COORD_MAX               (LONG_MAX - 65536)

//
//  Baseline font corresponds to smallest,smaller,medium,larger,largest
//

#define BASELINEFONTMIN         0
#define BASELINEFONTMAX         4
#define BASELINEFONTDEFAULT     2

//
// The base constant for creating DISPIDs for controls.  Must be
// larger than the DISPID for all other DISPIDs for the form.
//

#define DISPID_CONTROLBASE      3000


//  Private window messages

#define WM_FORM_FIRST           (WM_SERVER_LAST + 1)
#define WM_DEFERZORDER          (WM_FORM_FIRST +  0)
#define WM_MOUSEOVER            (WM_FORM_FIRST +  1)
#define WM_ACTIVEMOVIE          (WM_FORM_FIRST +  2)
#define WM_PRINTSTATUS          (WM_FORM_FIRST +  3)
#define WM_DEFERBLUR            (WM_FORM_FIRST +  4)
#define WM_DEFERFOCUS           (WM_FORM_FIRST +  5)
// Add new WM_USER messsages here and update WM_FORM_LAST.
#define WM_FORM_LAST            (WM_FORM_FIRST +  5)

#define MSGNAME_WM_HTML_GETOBJECT _T("WM_HTML_GETOBJECT")

#if !defined(NO_IME) && defined(IME_RECONVERSION)
#ifndef RWM_RECONVERT
#define RWM_RECONVERT _T("MSIMEReconvert")
#endif
#endif // !NO_IME

// Distinguished values of wclsids
#define WCLSID_INVALID  0x7FFF

// Timer IDs
#define TIMER_DEFERUPDATEUI             0x1000
#define TIMER_ID_MOUSE_EXIT             0x1001
#define TIMER_IMG_ANIM                  0x1002
#define TIMER_POSTMAN                   0x1003

// Anchor underline options
#define ANCHORUNDERLINE_NO              0x0
#define ANCHORUNDERLINE_YES             0x1
#define ANCHORUNDERLINE_HOVER           0x2

// cookie for menu extensions
// When passed into exec, says that event is already pushed
#define MENUEXT_COOKIE      0x567b4480

//+---------------------------------------------------------------------------
//
//  Enumeration:    INVAL_FLAGS
//
//  Synopsis:       See CDoc::Invalidate for description.
//
//----------------------------------------------------------------------------

enum INVAL_FLAGS
{
    INVAL_CHILDWINDOWS  = 1,    // Invalidate all child windows
    INVAL_GRABHANDLES   = 2     // Invalidate grab handles if any
};

#pragma INCMSG("--- End 'docglbs.hxx'")
#else
#pragma INCMSG("*** Dup 'docglbs.hxx'")
#endif
