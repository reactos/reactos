#ifndef _UXTHEME_H
#define _UXTHEME_H

#include <commctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (_WIN32_WINNT >= 0x0501)
#define DTBG_CLIPRECT               0x00000001
#define DTBG_DRAWSOLID              0x00000002
#define DTBG_OMITBORDER             0x00000004
#define DTBG_OMITCONTENT            0x00000008
#define DTBG_COMPUTINGREGION        0x00000010
#define DTBG_MIRRORDC               0x00000020
#define DTT_GRAYED                  0x00000001

/* DTTOPTS.dwFlags bits */
#define DTT_TEXTCOLOR               0x00000001
#define DTT_BORDERCOLOR             0x00000002
#define DTT_SHADOWCOLOR             0x00000004
#define DTT_SHADOWTYPE              0x00000008
#define DTT_SHADOWOFFSET            0x00000010
#define DTT_BORDERSIZE              0x00000020
#define DTT_FONTPROP                0x00000040
#define DTT_COLORPROP               0x00000080
#define DTT_STATEID                 0x00000100
#define DTT_CALCRECT                0x00000200
#define DTT_APPLYOVERLAY            0x00000400
#define DTT_GLOWSIZE                0x00000800
#define DTT_CALLBACK                0x00001000
#define DTT_COMPOSITED              0x00002000
#define DTT_VALIDBITS               0x00003fff

#define ETDT_DISABLE                0x00000001
#define ETDT_ENABLE                 0x00000002
#define ETDT_USETABTEXTURE          0x00000004
#define ETDT_ENABLETAB              (ETDT_ENABLE | ETDT_USETABTEXTURE)
#define STAP_ALLOW_NONCLIENT        0x00000001
#define STAP_ALLOW_CONTROLS         0x00000002
#define STAP_ALLOW_WEBCONTENT       0x00000004
#define HTTB_BACKGROUNDSEG          0x0000
#define HTTB_FIXEDBORDER            0x0002
#define HTTB_CAPTION                0x0004
#define HTTB_RESIZINGBORDER_LEFT    0x0010
#define HTTB_RESIZINGBORDER_TOP     0x0020
#define HTTB_RESIZINGBORDER_RIGHT   0x0040
#define HTTB_RESIZINGBORDER_BOTTOM  0x0080
#define HTTB_RESIZINGBORDER         (HTTB_RESIZINGBORDER_LEFT|HTTB_RESIZINGBORDER_TOP|HTTB_RESIZINGBORDER_RIGHT|HTTB_RESIZINGBORDER_BOTTOM)
#define HTTB_SIZINGTEMPLATE         0x0100
#define HTTB_SYSTEMSIZINGMARGINS    0x0200

#define OTD_FORCE_RECT_SIZING       0x0001
#define OTD_NONCLIENT               0x0002
#define OTD_VALIDBITS               (OTD_FORCE_RECT_SIZING | OTD_NONCLIENT)

typedef HANDLE HPAINTBUFFER;
typedef HANDLE HTHEME;
typedef int (WINAPI *DTT_CALLBACK_PROC)(HDC,LPWSTR,int,RECT*,UINT,LPARAM);

typedef enum _BP_BUFFERFORMAT
{
	BPBF_COMPATIBLEBITMAP,
	BPBF_DIB,
	BPBF_TOPDOWNDIB,
	BPBF_TOPDOWNMONODIB
} BP_BUFFERFORMAT;

typedef struct _BP_PAINTPARAMS
{
	DWORD cbSize;
	DWORD dwFlags;
	const RECT *prcExclude;
	const BLENDFUNCTION *pBlendFunction;
} BP_PAINTPARAMS, *PBP_PAINTPARAMS;

typedef enum PROPERTYORIGIN {
    PO_STATE = 0,
    PO_PART = 1,
    PO_CLASS = 2,
    PO_GLOBAL = 3,
    PO_NOTFOUND = 4
} PROPERTYORIGIN;

typedef enum THEMESIZE {
    TS_MIN,
    TS_TRUE,
    TS_DRAW
} THEMESIZE;

typedef struct _DTBGOPTS {
    DWORD dwSize;
    DWORD dwFlags;
    RECT rcClip;
} DTBGOPTS, *PDTBGOPTS;

#define MAX_INTLIST_COUNT 10

typedef struct _INTLIST {
    int iValueCount;
    int iValues[MAX_INTLIST_COUNT];
} INTLIST, *PINTLIST;

typedef struct _MARGINS {
    int cxLeftWidth;
    int cxRightWidth;
    int cyTopHeight;
    int cyBottomHeight;
} MARGINS, *PMARGINS;

typedef struct _DTTOPTS {
    DWORD dwSize;
    DWORD dwFlags;
    COLORREF crText;
    COLORREF crBorder;
    COLORREF crShadow;
    int iTextShadowType;
    POINT ptShadowOffset;
    int iBorderSize;
    int iFontPropId;
    int iColorPropId;
    int iStateId;
    BOOL fApplyOverlay;
    int iGlowSize;
    DTT_CALLBACK_PROC pfnDrawTextCallback;
    LPARAM lParam;
} DTTOPTS, *PDTTOPTS;

HRESULT WINAPI CloseThemeData(HTHEME);
HRESULT WINAPI DrawThemeBackground(HTHEME,HDC,int,int,const RECT*,const RECT*);
HRESULT WINAPI DrawThemeBackgroundEx(HTHEME,HDC,int,int,const RECT*,const DTBGOPTS*);
HRESULT WINAPI DrawThemeEdge(HTHEME,HDC,int,int,const RECT*,UINT,UINT,RECT*);
HRESULT WINAPI DrawThemeIcon(HTHEME,HDC,int,int,const RECT*,HIMAGELIST,int);
HRESULT WINAPI DrawThemeParentBackground(HWND,HDC,RECT*);
HRESULT WINAPI DrawThemeText(HTHEME,HDC,int,int,LPCWSTR,int,DWORD,DWORD,const RECT*);
HRESULT WINAPI EnableThemeDialogTexture(HWND,DWORD);
HRESULT WINAPI EnableTheming(BOOL);
HRESULT WINAPI GetCurrentThemeName(LPWSTR,int,LPWSTR,int,LPWSTR,int);
DWORD WINAPI GetThemeAppProperties(void);
HRESULT WINAPI GetThemeBackgroundContentRect(HTHEME,HDC,int,int,const RECT*,RECT*);
HRESULT WINAPI GetThemeBackgroundExtent(HTHEME,HDC,int,int,const RECT*,RECT*);
HRESULT WINAPI GetThemeBackgroundRegion(HTHEME,HDC,int,int,const RECT*,HRGN*);
HRESULT WINAPI GetThemeBool(HTHEME,int,int,int,BOOL*);
HRESULT WINAPI GetThemeColor(HTHEME,int,int,int,COLORREF*);
HRESULT WINAPI GetThemeDocumentationProperty(LPCWSTR,LPCWSTR,LPWSTR,int);
HRESULT WINAPI GetThemeEnumValue(HTHEME,int,int,int,int*);
HRESULT WINAPI GetThemeFilename(HTHEME,int,int,int,LPWSTR,int);
HRESULT WINAPI GetThemeFont(HTHEME,HDC,int,int,int,LOGFONTW*);
HRESULT WINAPI GetThemeInt(HTHEME,int,int,int,int*);
HRESULT WINAPI GetThemeIntList(HTHEME,int,int,int,INTLIST*);
HRESULT WINAPI GetThemeMargins(HTHEME,HDC,int,int,int,RECT*,MARGINS*);
HRESULT WINAPI GetThemeMetric(HTHEME,HDC,int,int,int,int*);
HRESULT WINAPI GetThemePartSize(HTHEME,HDC,int,int,RECT*,THEMESIZE,SIZE*);
HRESULT WINAPI GetThemePosition(HTHEME,int,int,int,POINT*);
HRESULT WINAPI GetThemePropertyOrigin(HTHEME,int,int,int,PROPERTYORIGIN*);
HRESULT WINAPI GetThemeRect(HTHEME,int,int,int,RECT*);
HRESULT WINAPI GetThemeString(HTHEME,int,int,int,LPWSTR,int);
BOOL WINAPI GetThemeSysBool(HTHEME,int);
COLORREF WINAPI GetThemeSysColor(HTHEME,int);
HBRUSH WINAPI GetThemeSysColorBrush(HTHEME,int);
HRESULT WINAPI GetThemeSysFont(HTHEME,int,LOGFONTW*);
HRESULT WINAPI GetThemeSysInt(HTHEME,int,int*);
int WINAPI GetThemeSysSize(HTHEME,int);
HRESULT WINAPI GetThemeSysString(HTHEME,int,LPWSTR,int);
HRESULT WINAPI GetThemeTextExtent(HTHEME,HDC,int,int,LPCWSTR,int,DWORD,const RECT*,RECT*);
HRESULT WINAPI GetThemeTextMetrics(HTHEME,HDC,int,int,TEXTMETRICW*);
HTHEME WINAPI GetWindowTheme(HWND);
HRESULT WINAPI HitTestThemeBackground(HTHEME,HDC,int,int,DWORD,const RECT*,HRGN,POINT,WORD*);
BOOL WINAPI IsAppThemed(void);
BOOL WINAPI IsThemeActive(void);
BOOL WINAPI IsThemeBackgroundPartiallyTransparent(HTHEME,int,int);
BOOL WINAPI IsThemeDialogTextureEnabled(HWND);
BOOL WINAPI IsThemePartDefined(HTHEME,int,int);
HTHEME WINAPI OpenThemeData(HWND,LPCWSTR);
HTHEME WINAPI OpenThemeDataEx(HWND,LPCWSTR,DWORD);
void WINAPI SetThemeAppProperties(DWORD);
HRESULT WINAPI SetWindowTheme(HWND,LPCWSTR,LPCWSTR);

/* Undocumented and not exported in Windows XP/2003
 * In public headers since Vista+ */
HRESULT
WINAPI
DrawThemeTextEx(
    _In_ HTHEME hTheme,
    _In_ HDC hdc,
    _In_ int iPartId,
    _In_ int iStateId,
    _In_ LPCWSTR pszText,
    _In_ int iCharCount,
    _In_ DWORD dwTextFlags,
    _Inout_ LPRECT pRect,
    _In_ const DTTOPTS *options
);
#endif

#ifdef __cplusplus
}
#endif
#endif
