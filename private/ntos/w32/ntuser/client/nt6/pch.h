
#include <windows.h>
#include <windowsx.h>

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif /* __cplusplus */

#define SetWindowPos NtUserSetWindowPos

BOOL
NtUserSetWindowPos(
    IN HWND hwnd,
    IN HWND hwndInsertAfter,
    IN int x,
    IN int y,
    IN int cx,
    IN int cy,
    IN UINT dwFlags);

#define GetDC NtUserGetDC

HDC
NtUserGetDC(
    IN HWND hwnd);

#define DeferWindowPos NtUserDeferWindowPos
HANDLE
NtUserDeferWindowPos(
    IN HDWP hWinPosInfo,
    IN HWND hwnd,
    IN HWND hwndInsertAfter,
    IN int x,
    IN int y,
    IN int cx,
    IN int cy,
    IN UINT wFlags);

#define DestroyWindow NtUserDestroyWindow

BOOL
NtUserDestroyWindow(
    IN HWND hwnd);



#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif       /* __cplusplus */
