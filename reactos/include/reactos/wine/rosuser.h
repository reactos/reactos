/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Win32 Graphical Subsystem (WIN32K)
 * FILE:            include/win32k/rosuser.h
 * PURPOSE:         Win32 Shared USER Types for RosUser*
 * PROGRAMMER:      Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES ******************************************************************/

#ifndef __WIN32K_ROSUSER_H
#define __WIN32K_ROSUSER_H

/* DEFINES *******************************************************************/

#define SWM_ROOT_WINDOW_ID 1

#define SWM_EVENT_TYPE_NONE     0
#define SWM_EVENT_TYPE_EXPOSURE 1

/* Event masks */
#define SWM_EVENTMASK(n)    (((ULONG) 1) << (n))

#define SWM_EVENT_MASK_NONE         SWM_EVENTMASK(SWM_EVENT_TYPE_NONE)
#define SWM_EVENT_MASK_EXPOSURE     SWM_EVENTMASK(SWM_EVENT_TYPE_EXPOSURE)

/* ENUMERATIONS **************************************************************/

/* TYPES *********************************************************************/

typedef ULONG_PTR GR_WINDOW_ID;
typedef ULONG SWM_EVENT_TYPE;

typedef struct
{
  SWM_EVENT_TYPE type;		/**< event type */
  GR_WINDOW_ID wid;		/**< window id */
  GR_WINDOW_ID otherid;		/**< new/old focus id for focus events*/
  						/**< for mouse enter only the following are valid:*/
  LONG rootx;		/**< root window x coordinate */
  LONG rooty;		/**< root window y coordinate */
  LONG x;			/**< window x coordinate of mouse */
  LONG y;			/**< window y coordinate of mouse */
} SWM_EVENT_GENERAL;

typedef struct {
  SWM_EVENT_TYPE type;		/**< event type */
  GR_WINDOW_ID wid;		/**< window id */
  LONG x;			/**< window x coordinate of exposure */
  LONG y;			/**< window y coordinate of exposure */
  LONG width;		/**< width of exposure */
  LONG height;		/**< height of exposure */
} SWM_EVENT_EXPOSURE;

typedef union
{
  SWM_EVENT_TYPE type;			/**< event type */
  SWM_EVENT_GENERAL general;		/**< general window events */
  SWM_EVENT_EXPOSURE exposure;		/**< exposure events */
} SWM_EVENT;

/* FUNCTIONS *****************************************************************/

#if 0
BOOL
NTAPI
RosUserEnumDisplayMonitors(
    HDC hdc,
    LPRECT rect,
    MONITORENUMPROC proc,
    LPARAM lp
);

BOOL
NTAPI
RosUserGetMonitorInfo(
    HMONITOR handle,
    LPMONITORINFO info
);
#endif

BOOL NTAPI
RosUserGetCursorPos( LPPOINT pt );

BOOL NTAPI
RosUserSetCursorPos( INT x, INT y );

BOOL NTAPI
RosUserClipCursor( LPCRECT clip );

#if 0
void NTAPI
RosUserSetCursor( ICONINFO* IconInfo );

VOID APIENTRY
RosUserCreateCursorIcon(ICONINFO* IconInfoUnsafe,
                        HCURSOR Handle);

VOID APIENTRY
RosUserDestroyCursorIcon(ICONINFO* IconInfoUnsafe,
                         HCURSOR Handle);
#endif

VOID
APIENTRY
RosUserSetCursor( ICONINFO* IconInfoUnsafe );

LONG
APIENTRY
RosUserChangeDisplaySettings(
   PUNICODE_STRING lpszDeviceName,
   LPDEVMODEW lpDevMode,
   HWND hwnd,
   DWORD dwflags,
   LPVOID lParam);

INT
APIENTRY
RosUserEnumDisplayMonitors(
   OPTIONAL OUT HMONITOR *hMonitorList,
   OPTIONAL OUT PRECTL monitorRectList,
   OPTIONAL IN DWORD listSize);

NTSTATUS
APIENTRY
RosUserEnumDisplaySettings(
   PUNICODE_STRING pusDeviceName,
   DWORD iModeNum,
   LPDEVMODEW lpDevMode,
   DWORD dwFlags );

BOOL
APIENTRY
RosUserGetMonitorInfo(
   IN HMONITOR hMonitor,
   OUT LPMONITORINFO pMonitorInfo);

HKL 
APIENTRY
RosUserGetKeyboardLayout(
   DWORD dwThreadId);

BOOL
APIENTRY
RosUserGetKeyboardLayoutName(
   LPWSTR lpszName);

HKL
APIENTRY
RosUserLoadKeyboardLayoutEx(
   IN HANDLE Handle,
   IN DWORD offTable,
   IN PUNICODE_STRING puszKeyboardName,
   IN HKL hKL,
   IN PUNICODE_STRING puszKLID,
   IN DWORD dwKLID,
   IN UINT Flags);

HKL
APIENTRY
RosUserActivateKeyboardLayout(
   HKL hKl,
   ULONG Flags);

BOOL
APIENTRY
RosUserUnloadKeyboardLayout(
   HKL hKl);

DWORD
APIENTRY
RosUserVkKeyScanEx(
   WCHAR wChar,
   HKL hKeyboardLayout,
   BOOL UsehKL );

DWORD
APIENTRY
RosUserGetKeyNameText( LONG lParam, LPWSTR lpString, int nSize );

int
APIENTRY
RosUserToUnicodeEx(
   UINT wVirtKey,
   UINT wScanCode,
   PBYTE lpKeyState,
   LPWSTR pwszBuff,
   int cchBuff,
   UINT wFlags,
   HKL dwhkl );

UINT
APIENTRY
RosUserMapVirtualKeyEx( UINT Code, UINT Type, DWORD keyboardId, HKL dwhkl );

SHORT
APIENTRY
RosUserGetAsyncKeyState(
   INT key);

BOOL
APIENTRY
RosUserSetAsyncKeyboardState(BYTE key_state_table[]);

BOOL
APIENTRY
RosUserGetAsyncKeyboardState(BYTE key_state_table[]);

BOOL NTAPI
RosUserRegisterShellHookWindow(HWND hWnd);

BOOL NTAPI
RosUserDeRegisterShellHookWindow(HWND hWnd);

BOOL NTAPI
RosUserBuildShellHookHwndList(HWND *list, UINT *cbSize);

GR_WINDOW_ID NTAPI
SwmNewWindow(GR_WINDOW_ID parent, RECT *WindowRect, HWND hWnd, DWORD ex_style);

VOID NTAPI
SwmAddDesktopWindow(HWND hWnd, UINT Width, UINT Height);

VOID NTAPI
SwmDestroyWindow(GR_WINDOW_ID Wid);

VOID NTAPI
SwmSetForeground(GR_WINDOW_ID Wid);

VOID NTAPI
SwmPosChanged(GR_WINDOW_ID Wid, const RECT *WindowRect, const RECT *OldRect, HWND hWndAfter, UINT SwpFlags);

HWND NTAPI
SwmGetWindowFromPoint(LONG x, LONG y);

VOID NTAPI
SwmShowWindow(GR_WINDOW_ID Wid, BOOLEAN Show, UINT SwpFlags);

int NTAPI
SwmPeekEvent(SWM_EVENT *ep);

VOID NTAPI
SwmGetNextEvent(SWM_EVENT *ep);

#endif /* __WIN32K_NTUSER_H */
