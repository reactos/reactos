
#ifndef _INC_COMMCTRL_WINE
#define _INC_COMMCTRL_WINE

#define DPA_GetPtr DPA_GetPtr_wine_hack
#define FlatSB_SetScrollProp FlatSB_SetScrollProp_wine_hack

#if (_WIN32_IE < 0x501)
#undef _WIN32_IE
#define _WIN32_IE 0x0501
#endif

#include_next <commctrl.h>

#undef DPA_GetPtr
LPVOID WINAPI DPA_GetPtr(HDPA, INT);

#undef FlatSB_SetScrollProp
BOOL  WINAPI FlatSB_SetScrollProp(HWND, UINT, INT, BOOL);

#define DRAGLISTMSGSTRINGA      "commctrl_DragListMsg"
#if defined(__GNUC__)
# define DRAGLISTMSGSTRINGW (const WCHAR []){ 'c','o','m','m','c','t','r','l', \
  '_','D','r','a','g','L','i','s','t','M','s','g',0 }
#elif defined(_MSC_VER)
# define DRAGLISTMSGSTRINGW     L"commctrl_DragListMsg"
#else
static const WCHAR DRAGLISTMSGSTRINGW[] = { 'c','o','m','m','c','t','r','l',
  '_','D','r','a','g','L','i','s','t','M','s','g',0 };
#endif

#define FLATSB_CLASSA         "flatsb_class32"
#if defined(__GNUC__)
# define FLATSB_CLASSW (const WCHAR []){ 'f','l','a','t','s','b','_', \
  'c','l','a','s','s','3','2',0 }
#elif defined(_MSC_VER)
# define FLATSB_CLASSW        L"flatsb_class32"
#else
static const WCHAR FLATSB_CLASSW[] = { 'f','l','a','t','s','b','_',
  'c','l','a','s','s','3','2',0 };
#endif

typedef TBSAVEPARAMSW *LPTBSAVEPARAMSW;

typedef LVFINDINFOA *LPLVFINDINFOA;
typedef LVFINDINFOW *LPLVFINDINFOW;

#define SB_SETBORDERS (WM_USER+5)
#define TBSTYLE_EX_UNDOC1 0x00000004 /* similar to TBSTYLE_WRAPABLE */

/* these are undocumented and the names are guesses */
typedef struct
{
    NMHDR hdr;
    HWND hwndDialog;
} NMTBINITCUSTOMIZE;

typedef struct
{
    NMHDR hdr;
    INT idNew;
    INT iDirection; /* left is -1, right is 1 */
    DWORD dwReason; /* HICF_* */
} NMTBWRAPHOTITEM;

#define LPNMLVDISPINFO WINELIB_NAME_AW(LPNMLVDISPINFO)

/* undocumented messages in Toolbar */
#define TB_UNKWN45D              (WM_USER+93)
#define TB_UNKWN464              (WM_USER+100)

#endif /* _INC_COMMCTRL_WINE */
