#ifndef _UXTHEME_H_
#define _UXTHEME_H_

#include <commctrl.h>

#if !defined(THEMEAPI)
# if !defined(_UXTHEME_)
#  define THEMEAPI        EXTERN_C DECLSPEC_IMPORT HRESULT STDAPICALLTYPE
#  define THEMEAPI_(type) EXTERN_C DECLSPEC_IMPORT type STDAPICALLTYPE
# else
#  define THEMEAPI        STDAPI
#  define THEMEAPI_(type) STDAPI_(type)
# endif
#endif

typedef HANDLE HTHEME;

THEMEAPI_(HTHEME) OpenThemeData
(
 IN HWND hwnd,
 IN LPCWSTR pszClassList
);

THEMEAPI CloseThemeData
(
 IN HTHEME hTheme
);

THEMEAPI DrawThemeBackground
(
 IN HTHEME hTheme,
 IN HDC hdc,
 IN int iPartId,
 IN int iStateId,
 IN const RECT * pRect,
 IN OPTIONAL const RECT * pClipRect
);

#define DTT_GRAYED (1)

THEMEAPI DrawThemeText
(
 IN HTHEME hTheme,
 IN HDC hdc,
 IN int iPartId,
 IN int iStateId,
 IN LPCWSTR pszText,
 IN int iCharCount,
 IN DWORD dwTextFlags,
 IN DWORD dwTextFlags2,
 IN const RECT *pRect
);

THEMEAPI GetThemeBackgroundContentRect
(
 IN HTHEME hTheme,
 IN OPTIONAL HDC hdc,
 IN int iPartId,
 IN int iStateId,
 IN const RECT * pBoundingRect,
 OUT RECT * pContentRect
);

THEMEAPI GetThemeBackgroundExtent
(
 IN HTHEME hTheme,
 IN OPTIONAL HDC hdc,
 IN int iPartId,
 IN int iStateId,
 IN const RECT * pContentRect,
 OUT RECT * pExtentRect
);

enum THEMESIZE
{
 TS_MIN,
 TS_TRUE,
 TS_DRAW,
};

THEMEAPI GetThemePartSize
(
 IN HTHEME hTheme,
 IN HDC hdc,
 IN int iPartId,
 IN int iStateId, 
 IN OPTIONAL RECT * prc,
 IN enum THEMESIZE eSize,
 OUT SIZE * psz
);

THEMEAPI GetThemeTextExtent
(
 IN HTHEME hTheme,
 IN HDC hdc,
 IN int iPartId,
 IN int iStateId,
 IN LPCWSTR pszText,
 IN int iCharCount,
 IN DWORD dwTextFlags,
 IN OPTIONAL const RECT * pBoundingRect,
 OUT RECT * pExtentRect
);

THEMEAPI GetThemeTextMetrics
(
 IN HTHEME hTheme,
 IN OPTIONAL HDC hdc,
 IN int iPartId,
 IN int iStateId,
 OUT TEXTMETRIC * ptm
);

THEMEAPI GetThemeBackgroundRegion
(
 IN HTHEME hTheme,
 IN OPTIONAL HDC hdc,
 IN int iPartId,
 IN int iStateId,
 IN const RECT * pRect,
 OUT HRGN * pRegion
);

#define HTTB_BACKGROUNDSEG         (0x00000000)
#define HTTB_FIXEDBORDER           (0x00000002)
#define HTTB_CAPTION               (0x00000004)
#define HTTB_RESIZINGBORDER_LEFT   (0x00000010)
#define HTTB_RESIZINGBORDER_TOP    (0x00000020)
#define HTTB_RESIZINGBORDER_RIGHT  (0x00000040)
#define HTTB_RESIZINGBORDER_BOTTOM (0x00000080)
#define HTTB_SIZINGTEMPLATE        (0x00000100)
#define HTTB_SYSTEMSIZINGMARGINS   (0x00000200)

#define HTTB_RESIZINGBORDER \
 ( \
  HTTB_RESIZINGBORDER_LEFT  | \
  HTTB_RESIZINGBORDER_TOP   | \
  HTTB_RESIZINGBORDER_RIGHT | \
  HTTB_RESIZINGBORDER_BOTTOM \
 )

THEMEAPI HitTestThemeBackground
(
 IN HTHEME hTheme,
 IN OPTIONAL HDC hdc,
 IN int iPartId,
 IN int iStateId,
 IN DWORD dwOptions,
 IN const RECT * pRect,
 IN OPTIONAL HRGN hrgn,
 IN POINT ptTest,
 OUT WORD * pwHitTestCode
);

THEMEAPI DrawThemeEdge
(
 IN HTHEME hTheme,
 IN HDC hdc,
 IN int iPartId,
 IN int iStateId,
 IN const RECT * pDestRect,
 IN UINT uEdge,
 IN UINT uFlags,
 OUT OPTIONAL RECT * pContentRect
);

THEMEAPI DrawThemeIcon
(
 IN HTHEME hTheme,
 IN HDC hdc,
 IN int iPartId,
 IN int iStateId,
 IN const RECT * pRect,
 IN HIMAGELIST himl,
 int iImageIndex
);

THEMEAPI_(BOOL) IsThemePartDefined
(
 IN HTHEME hTheme,
 IN int iPartId, 
 IN int iStateId
);

THEMEAPI_(BOOL) IsThemeBackgroundPartiallyTransparent
(
 IN HTHEME hTheme, 
 IN int iPartId,
 IN int iStateId
);

THEMEAPI GetThemeColor
(
 IN HTHEME hTheme,
 IN int iPartId,
 IN int iStateId,
 IN int iPropId,
 OUT COLORREF * pColor
);

THEMEAPI GetThemeMetric
(
 IN HTHEME hTheme,
 IN OPTIONAL HDC hdc,
 IN int iPartId,
 IN int iStateId,
 IN int iPropId,
 OUT int * piVal
);

THEMEAPI GetThemeString
(
 IN HTHEME hTheme,
 IN int iPartId, 
 IN int iStateId,
 IN int iPropId,
 OUT LPWSTR pszBuff,
 IN int cchMaxBuffChars
);

THEMEAPI GetThemeBool
(
 IN HTHEME hTheme,
 IN int iPartId,
 IN int iStateId,
 IN int iPropId,
 OUT BOOL * pfVal
);

THEMEAPI GetThemeInt
(
 IN HTHEME hTheme,
 IN int iPartId,
 IN int iStateId,
 IN int iPropId,
 OUT int * piVal
);

THEMEAPI GetThemeEnumValue
(
 IN HTHEME hTheme,
 IN int iPartId, 
 IN int iStateId,
 IN int iPropId,
 OUT int * piVal
);

THEMEAPI GetThemePosition
(
 IN HTHEME hTheme,
 IN int iPartId,
 IN int iStateId,
 IN int iPropId,
 OUT POINT * pPoint
);

THEMEAPI GetThemeFont
(
 IN HTHEME hTheme,
 IN OPTIONAL HDC hdc,
 IN int iPartId, 
 IN int iStateId,
 IN int iPropId,
 OUT LOGFONT * pFont
);

THEMEAPI GetThemeRect
(
 IN HTHEME hTheme,
 IN int iPartId,
 IN int iStateId,
 IN int iPropId,
 OUT RECT * pRect
);

typedef struct _MARGINS
{
 int cxLeftWidth;
 int cxRightWidth;
 int cyTopHeight;
 int cyBottomHeight;
}
MARGINS, * PMARGINS;

THEMEAPI GetThemeMargins
(
 IN HTHEME hTheme,
 IN OPTIONAL HDC hdc,
 IN int iPartId,
 IN int iStateId,
 IN int iPropId,
 IN OPTIONAL RECT * prc,
 OUT MARGINS * pMargins
);

#define MAX_INTLIST_COUNT (10)

typedef struct _INTLIST
{
 int iValueCount;
 int iValues[MAX_INTLIST_COUNT];
}
INTLIST, * PINTLIST;

THEMEAPI GetThemeIntList
(
 IN HTHEME hTheme,
 IN int iPartId,
 IN int iStateId,
 IN int iPropId,
 OUT INTLIST * pIntList
);

enum PROPERTYORIGIN
{
 PO_STATE,
 PO_PART,
 PO_CLASS,
 PO_GLOBAL,
 PO_NOTFOUND
};

THEMEAPI GetThemePropertyOrigin
(
 IN HTHEME hTheme,
 IN int iPartId,
 IN int iStateId,
 IN int iPropId,
 OUT enum PROPERTYORIGIN * pOrigin
);

THEMEAPI SetWindowTheme
(
 IN HWND hwnd,
 IN LPCWSTR pszSubAppName,
 IN LPCWSTR pszSubIdList
);

THEMEAPI GetThemeFilename
(
 IN HTHEME hTheme,
 IN int iPartId,
 IN int iStateId,
 IN int iPropId,
 OUT LPWSTR pszThemeFileName,
 IN int cchMaxBuffChars
);

THEMEAPI_(COLORREF) GetThemeSysColor
(
 IN HTHEME hTheme,
 IN int iColorId
);

THEMEAPI_(HBRUSH) GetThemeSysColorBrush
(
 IN HTHEME hTheme,
 IN int iColorId
);

THEMEAPI_(BOOL) GetThemeSysBool
(       
 IN HTHEME hTheme,
 IN int iBoolId
);

THEMEAPI_(int) GetThemeSysSize
(    
 IN HTHEME hTheme,
 IN int iSizeId
);

THEMEAPI GetThemeSysFont
(
 IN HTHEME hTheme,
 IN int iFontId,
 OUT LOGFONT * plf
);

THEMEAPI GetThemeSysString
(
 IN HTHEME hTheme,
 IN int iStringId,
 OUT LPWSTR pszStringBuff,
 IN int cchMaxStringChars
);

THEMEAPI GetThemeSysInt
(
 IN HTHEME hTheme,
 IN int iIntId,
 IN int * piValue
);

THEMEAPI_(BOOL) IsThemeActive(void);

THEMEAPI_(BOOL) IsAppThemed(void);

THEMEAPI_(HTHEME) GetWindowTheme
(
 IN HWND hwnd
);

#define ETDT_DISABLE       (0x00000001)
#define ETDT_ENABLE        (0x00000002)
#define ETDT_USETABTEXTURE (0x00000004)
#define ETDT_ENABLETAB     (ETDT_ENABLE | ETDT_USETABTEXTURE)

THEMEAPI EnableThemeDialogTexture
(
 IN HWND hwnd,
 IN DWORD dwFlags
);

THEMEAPI_(BOOL) IsThemeDialogTextureEnabled
(
 IN HWND hwnd
);

#define STAP_ALLOW_NONCLIENT  (0x00000001)
#define STAP_ALLOW_CONTROLS   (0x00000002)
#define STAP_ALLOW_WEBCONTENT (0x00000004)

THEMEAPI_(DWORD) GetThemeAppProperties(void);

THEMEAPI_(void) SetThemeAppProperties
(
 IN DWORD dwFlags
);

THEMEAPI GetCurrentThemeName
(
 OUT LPWSTR pszThemeFileName,
 IN int cchMaxNameChars,
 OUT OPTIONAL LPWSTR pszColorBuff,
 IN int cchMaxColorChars,
 OUT OPTIONAL LPWSTR pszSizeBuff,
 IN int cchMaxSizeChars
);

#define SZ_THDOCPROP_DISPLAYNAME   L"DisplayName"
#define SZ_THDOCPROP_CANONICALNAME L"ThemeName"
#define SZ_THDOCPROP_TOOLTIP       L"ToolTip"
#define SZ_THDOCPROP_AUTHOR        L"author"

THEMEAPI GetThemeDocumentationProperty
(
 IN LPCWSTR pszThemeName,
 IN LPCWSTR pszPropertyName,
 OUT LPWSTR pszValueBuff,
 IN int cchMaxValChars
);

THEMEAPI DrawThemeParentBackground
(
 IN HWND hwnd,
 IN HDC hdc,
 IN OPTIONAL RECT * prc
);

THEMEAPI EnableTheming
(
 IN BOOL fEnable
);

#endif

/* EOF */
