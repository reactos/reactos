#ifndef __WINE_WINUSER_H
#define __WINE_WINUSER_H

/*
 * Compatibility header
 */

#include <wingdi.h>

#if !defined(_MSC_VER)
#include_next "winuser.h"
#endif

#define WS_EX_TRAYWINDOW 0x80000000L
#define DCX_USESTYLE     0x00010000
#define WS_EX_MANAGED    0x40000000L /* Window managed by the window system */
#define LB_CARETOFF      0x01a4

WINUSERAPI UINT WINAPI PrivateExtractIconsA(LPCSTR,int,int,int,HICON*,UINT*,UINT,UINT);
WINUSERAPI UINT WINAPI PrivateExtractIconsW(LPCWSTR,int,int,int,HICON*,UINT*,UINT,UINT);

#define WM_ALTTABACTIVE         0x0029

#ifndef E_PROP_ID_UNSUPPORTED
#define E_PROP_ID_UNSUPPORTED            ((HRESULT)0x80070490)
#endif
#ifndef E_PROP_SET_UNSUPPORTED
#define E_PROP_SET_UNSUPPORTED           ((HRESULT)0x80070492)
#endif

/* MapVirtualKey translation types */
#if 0
#define MAPVK_VK_TO_VSC     0
#define MAPVK_VSC_TO_VK     1
#define MAPVK_VK_TO_CHAR    2
#define MAPVK_VSC_TO_VK_EX  3
#define MAPVK_VK_TO_VSC_EX  4
#endif
#define WM_SETVISIBLE           0x0009

#define MAKEINTATOMA(atom)  ((LPCSTR)((ULONG_PTR)((WORD)(atom))))
#define MAKEINTATOMW(atom)  ((LPCWSTR)((ULONG_PTR)((WORD)(atom))))

#endif /* __WINE_WINUSER_H */
