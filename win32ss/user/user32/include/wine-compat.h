#pragma once

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

/* Unimplemented */

#define GetDpiForWindow(HWND) 96

#define get_input_codepage() CP_ACP

#define NtUserGetPrivateData(HWND, OFFSET, SIZE) GetWindowLongPtrW((HWND), (OFFSET))
#define NtUserSetPrivateData(HWND, OFFSET, SIZE, VALUE) SetWindowLongPtrW((HWND), (OFFSET), (VALUE))

#define NtUserGetWindowSysSubMenu(HWND) GetSubMenu(GetMenu((HWND)), 0)

#define NtUserGetMDIClientInfo(HWND) (MDICLIENTINFO *)GetWindowLongPtr((HWND), GWLP_MDIWND)
#define NtUserSetMDIClientInfo(HWND, INFO) SetWindowLongPtr((HWND), GWLP_MDIWND, (LONG_PTR)(INFO))

/* NtUser functions map */

#define NtUserReleaseDC(HWND, HDC) NtUserxReleaseDC((HDC))
#define NtUserReleaseCapture() NtUserxReleaseCapture()
#define NtUserKillSystemTimer(HWND, ID) NtUserxKillSystemTimer((HWND), (ID))
#define NtUserEnableWindow(HWND, ENABLE) NtUserxEnableWindow((HWND), (ENABLE))
#define NtUserDrawMenuBar(HWND) NtUserxDrawMenuBar((HWND))
#define NtUserArrangeIconicWindows(HWND) NtUserxArrangeIconicWindows((HWND))
/* WINE calls NtUserSetSystemTimer with 3 parameters instead of 4 */
#define NtUserSetSystemTimer(HWND, ID, TIMEOUT) NtUserSetSystemTimer((HWND), (ID), (TIMEOUT), NULL)
/* WINE calls NtUserDrawIconEx with 9 arguments instead of 11 */
#define NtUserDrawIconEx(P1, P2, P3, P4, P5, P6, P7, P8, P9) DrawIconEx((P1), (P2), (P3), (P4), (P5), (P6), (P7), (P8), (P9))
/* WINE calls NtUserSetMenu with 2 parameters instead of 2 */
#define NtUserSetMenu(P1, P2) SetMenu((P1), (P2))

/* standard C */

#define malloc(size) HeapAlloc(GetProcessHeap(), 0, (size))
#define calloc(count, size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (count) * (size))
#define realloc(ptr, size) ptr ? HeapReAlloc(GetProcessHeap(), 0, (ptr), (size)) : HeapAlloc(GetProcessHeap(), 0, (size))
#define free(ptr) HeapFree(GetProcessHeap(), 0, (ptr))

/* MAKE_FNID */

#define NTUSER_WNDPROC_SCROLLBAR    FNID_SCROLLBAR
#define NTUSER_WNDPROC_MENU         FNID_MENU
#define NTUSER_WNDPROC_DESKTOP      FNID_DESKTOP
#define NTUSER_WNDPROC_ICONTITLE    FNID_ICONTITLE
#define NTUSER_WNDPROC_BUTTON       FNID_BUTTON
#define NTUSER_WNDPROC_COMBO        FNID_COMBOBOX
#define NTUSER_WNDPROC_COMBOLBOX    FNID_COMBOLBOX
#define NTUSER_WNDPROC_DIALOG       FNID_DIALOG
#define NTUSER_WNDPROC_EDIT         FNID_EDIT
#define NTUSER_WNDPROC_LISTBOX      FNID_LISTBOX
#define NTUSER_WNDPROC_MDICLIENT    FNID_MDICLIENT
#define NTUSER_WNDPROC_STATIC       FNID_STATIC
#define NTUSER_WNDPROC_IME          FNID_IME
#define NTUSER_WNDPROC_GHOST        FNID_GHOST

#define MAKE_FNID(index) (index)
