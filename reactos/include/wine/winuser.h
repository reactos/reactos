#ifndef __WINE_WINUSER_H
#define __WINE_WINUSER_H

/*
 * Compatibility header
 */

#include <w32api.h>
#include <wingdi.h>
#include_next <winuser.h>
#if (__W32API_MAJOR_VERSION >= 2) && (__W32API_MINOR_VERSION < 4)
#ifndef _WINUSER_COMPAT_H
#define _WINUSER_COMPAT_H
typedef LPDLGTEMPLATE LPDLGTEMPLATEA, LPDLGTEMPLATEW;
#if (WINVER >= _W2K)
#define DFCS_TRANSPARENT 0x800
#define DFCS_HOT 0x1000
#endif /* WINVER >= _W2K */
#endif
#endif

#define WS_EX_TRAYWINDOW 0x80000000L
//#if (__W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5)
#define MIM_MENUDATA     0x00000008
//#endif /* __W32API_MAJOR_VERSION < 2 || __W32API_MINOR_VERSION < 5 */

#define DCX_USESTYLE     0x00010000
#define WS_EX_MANAGED    0x40000000L /* Window managed by the window system */

UINT WINAPI PrivateExtractIconsA(LPCSTR,int,int,int,HICON*,UINT*,UINT,UINT);
UINT WINAPI PrivateExtractIconsW(LPCWSTR,int,int,int,HICON*,UINT*,UINT,UINT);

typedef struct tagCWPSTRUCT *LPCWPSTRUCT;

#define WM_ALTTABACTIVE         0x0029

#endif /* __WINE_WINUSER_H */
