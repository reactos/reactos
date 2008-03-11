/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/misc/stubs.c
 * PURPOSE:         User32.dll stubs
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * NOTES:           If you implement a function, remove it from this file
 * UPDATE HISTORY:
 *      08-F05-2001  CSH  Created
 */

#include <user32.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

/*
 * @unimplemented
 */
BOOL
STDCALL
AttachThreadInput(
  DWORD idAttach,
  DWORD idAttachTo,
  BOOL fAttach)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
long
STDCALL
BroadcastSystemMessageA(
  DWORD dwFlags,
  LPDWORD lpdwRecipients,
  UINT uiMessage,
  WPARAM wParam,
  LPARAM lParam)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
long
STDCALL
BroadcastSystemMessageW(
  DWORD dwFlags,
  LPDWORD lpdwRecipients,
  UINT uiMessage,
  WPARAM wParam,
  LPARAM lParam)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetMouseMovePointsEx(
  UINT cbSize,
  LPMOUSEMOVEPOINT lppt,
  LPMOUSEMOVEPOINT lpptBuf,
  int nBufPoints,
  DWORD resolution)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
LockWindowUpdate(
  HWND hWndLock)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
LockWorkStation(VOID)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
WaitForInputIdle(
  HANDLE hProcess,
  DWORD dwMilliseconds)
{
  UNIMPLEMENTED;
  return 0;
}

/******************************************************************************
 * SetDebugErrorLevel [USER32.@]
 * Sets the minimum error level for generating debugging events
 *
 * PARAMS
 *    dwLevel [I] Debugging error level
 *
 * @unimplemented
 */
VOID
STDCALL
SetDebugErrorLevel( DWORD dwLevel )
{
    DbgPrint("(%ld): stub\n", dwLevel);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetAppCompatFlags ( HTASK hTask )
{
    PW32THREADINFO ti = GetW32ThreadInfo();

    if (ti == NULL)
    {
        return 0;
    }

  /* NOTE : GetAppCompatFlags retuns which compatible flags should be send back 
   * the return value is any of http://support.microsoft.com/kb/82860
   * This text are direcly copy from the  MSDN URL, so it does not get lost
   *------------------------------------------------------------------------------
   * Bit: 1
   * Symbolic name: GACF_IGNORENODISCARD
   *
   * Meaning: Ignore NODISCARD flag if passed to GlobalAlloc(). C 6.x
   * Runtime install library was allocating global memory
   *  improperly by incorrectly specifying the GMEM_NODISCARD bit.
   *   
   * Problem: Setup for MS apps. does not work on 1M 286 machine.
   *-----------------------------------------------------------------------
   * Bit: 2
   * Symbolic name: GACF_FORCETEXTBAND
   *
   * Meaning: Separate text band from graphics band. Forces separate band
   * for text, disallowing 3.1 optimization where Text and
   * Graphics are printed in the same band. Word Perfect was
   * assuming text had to go in second band.
   *
   * Problem: Can't print graphics in landscape mode. The
   * compatibility switch doesn't completely fix the
   * problem, just fixes it for certain memory
   * configurations.
   *-----------------------------------------------------------------------
   * Bit: 4
   *
   * Symbolic name: GACF_ONELANDGRXBAND
   * Meaning: One graphics band only. Allows only one Landscape graphics
   * band. Take as much memory as possible for this band. What
   * doesn't fit in that band doesn't print.
   *
   * Problem: Can't print graphics in landscape mode. The
   * compatibility switch doesn't completely fix the
   * problem, just fixes it for certain memory configurations.
   *-----------------------------------------------------------------------
   * Bit: 8
   * Symbolic name: GACF_IGNORETOPMOST
   *
   * Meaning: Ignore topmost windows for GetWindow(HWND,GW_HWNDFIRST)
   * 
   * Problem: CCMail would GP fault when running any Windows applet
   * from CCMail because it assumed the applet it starts
   * will be at the top of the window list when winexec
   * returns. Because of the addition of TOPMOST windows in
   * Win , this isn't the case.  The compatibility bit
   * fixes this so GetWindow doesn't return a topmost window.
   *-----------------------------------------------------------------------
   * Bit: 10
   * Symbolic name: GACF_CALLTTDEVICE
   *
   * Meaning: Set the DEVICE_FONTTYPE bit in the FontType for TT fonts
   * returned by EnumFonts().
   *
   * Problem: WordPerfect was assuming TT fonts enumerated by the
   * printer would have the device bit set. TT fonts are not
   * device fonts, so this bit wasn't set. There were
   * various font mapping problems, such as TNR appearing in
   * Script or Symbol.
   *-----------------------------------------------------------------------
   * Bit: 20
   *
   * Symbolic name: GACF_MULTIPLEBANDS
   *
   * Meaning: Manually break graphics output into more than one band when printing.
   *
   * Problem: Freelance wouldn't print graphics when there was enough
   * memory and unidrv used only one band for printing. If
   * the first band was the entire page, it didn't issue any
   * graphics calls, thinking it was the text only band.
   * This forces unidrv to use multiple bands.
   *-----------------------------------------------------------------------
   * Bit: 40
   * Symbolic name: GACF_ALWAYSSENDNCPAINT
   *
   * Meaning: SetWindowPos() must send a WM_NCPAINT message to all
   * children, disallowing the 3.1 optimization where this message
   * is only sent to windows that must be redrawn.
   *
   * Problem: File window overlaps the toolbox and doesn't repaint
   * when a new file is opened.  Pixie used the receipt of
   * WM_NCPAINT messages to determine that they may need to
   * reposition themselves at the top of the list.  Win 3.0
   * used to send the messages to windows even when they
   * didn't need to be sent; in particular if the window was
   * within the bounding rect of any update region involved
   * in a window management operation.
   *   
   * Problem: Repaint problems with dialog boxes left on the screen
   * after file.open or file.new operations.
   *
   * Problem: Tool window is not available when opening the app.
   *-----------------------------------------------------------------------
   * Bit: 80
   *-----------------------------------------------------------------------
   * Bit: 100
   *-----------------------------------------------------------------------
   * Bit: 200
   *-----------------------------------------------------------------------
   * Bit: 800
   *-----------------------------------------------------------------------
   * Bit: 1000
   *-----------------------------------------------------------------------
   * Bit: 2000
   *-----------------------------------------------------------------------
   * Bit: 4000
   *-----------------------------------------------------------------------
   * Bit: 8000
   *
   * Symbolic name: GACF_FORCETTGRAPHICS
   *
   * Meaning: no expain in msdn
   *
   * Problem: Freelance wouldn't print TT unless print TT as graphics was selected.
   *-----------------------------------------------------------------------
   * Bit: 10000
   *
   * Symbolic name: GACF_NOHRGN1
   * Meaning:  This bit affects applications that depend on a bug in the
   * 3.0 GetUpdateRect() function. Under 3.0, GetUpdateRect
   * would not always return the rectangle in logical DC
   * coordinates: if the entire window was invalid, the rectangle
   * was sometimes returned in window coordinates. This bug was
   * fixed for 3.0 and 3.1 apps in Windows 3.1: coordinates are
   * ALWAYS returned in logical coordinates. This bit re-
   * introduces the bug in GetUpdateRect(), for those
   * applications that depend on this behavior.
   *
   * Problem: Canvas not redrawn properly opening specific MSDraw objects in Winword.
   *-----------------------------------------------------------------------
   * Bit: 20000
   *
   * Symbolic name: GACF_NCCALCSIZEONMOVE
   * Meaning: 3.1 and higher optimized WM_NCCALCSIZE if a window was just moving,
   * where 3.0 always sent it. This bit causes it to be sent always, as in 3.0.
   *
   * Problem: Navigator bar of window fails to redraw when the window
   * is moved across the desktop.
   *-----------------------------------------------------------------------
   * Bit: 40000
   *
   * Symbolic name: GACF_SENDMENUDBLCLK
   *
   * Meaning: Passes double-clicks on a menu bar on to the app. With this
   * bit set, if the user double clicks on the menu bar when a
   * menu is visible, we end processing of the menu and pass the
   * double click message on to the application. This allows Just
   * Write to detect double click on the system menu of a
   * maximized MDI child.    The normal (and expected) behavior is
   * for Windows to detect the double click on a sys menu of a
   * maximized child and send the app a WM_SYSCOMMAND SC_CLOSE
   * message which is what happens with a non-maximized MDI child window.
   *
   * Problem: Sub-editors (such as footer and header editors)
   *  couldn't be closed by double-clicking the system menu.
   *				
   *-----------------------------------------------------------------------
   * Bit: 80000
   *
   * Symbolic name: GACF_30AVGWIDTH
   *
   * Meaning: Changed the way we calculate avg width, this fixes it for
   * postscript. Scale all fonts by 7/8. This flag has been
   * added for TurboTax for printing with pscript driver. Turbo
   * Tax has hard coded average width it uses for selecting fonts.
   * Since we changed the way we calculate avg width to match what
   * is in TT, Turbo Tax is broken.
   *
   * Problem: 1040 tax forms wouldn't print correctly.
   */
  
  return ti->dwAppsCompatibleFlags;
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetInternalWindowPos(
		     HWND hwnd,
		     LPRECT rectWnd,
		     LPPOINT ptIcon
		     )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
VOID
STDCALL
LoadLocalFonts ( VOID )
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
LoadRemoteFonts ( VOID )
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
SetInternalWindowPos(
		     HWND    hwnd,
		     UINT    showCmd,
		     LPRECT  rect,
		     LPPOINT pt
		     )
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
RegisterSystemThread ( DWORD flags, DWORD reserved )
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
RegisterTasklist ( DWORD x )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
DragObject(
	   HWND    hwnd1,
	   HWND    hwnd2,
	   UINT    u1,
	   DWORD   dw1,
	   HCURSOR hc1
	   )
{
  return NtUserDragObject(hwnd1, hwnd2, u1, dw1, hc1);
}




/*
 * @implemented
 */
UINT
STDCALL
UserRealizePalette ( HDC hDC )
{
  return NtUserCallOneParam((DWORD) hDC, ONEPARAM_ROUTINE_REALIZEPALETTE);
}

/*
 * @unimplemented
 */
HANDLE
WINAPI
SetSysColorsTemp(
		 const COLORREF *pPens,
		 const HBRUSH   *pBrushes,
		 INT            n
		 )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
WORD
STDCALL
CascadeChildWindows ( HWND hWndParent, WORD wFlags )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
WORD
STDCALL
TileChildWindows ( HWND hWndParent, WORD wFlags )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
HDESK
STDCALL
GetInputDesktop ( VOID )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetAccCursorInfo ( PCURSORINFO pci )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ClientThreadSetup ( VOID )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetRawInputDeviceInfoW(
    HANDLE hDevice,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
LONG
STDCALL
BroadcastSystemMessageExW(
    DWORD dwflags,
    LPDWORD lpdwRecipients,
    UINT uiMessage,
    WPARAM wParam,
    LPARAM lParam,
    PBSMINFO pBSMInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
LONG
STDCALL
CsrBroadcastSystemMessageExW(
    DWORD dwflags,
    LPDWORD lpdwRecipients,
    UINT uiMessage,
    WPARAM wParam,
    LPARAM lParam,
    PBSMINFO pBSMInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetRawInputDeviceInfoA(
    HANDLE hDevice,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
LONG
STDCALL
BroadcastSystemMessageExA(
    DWORD dwflags,
    LPDWORD lpdwRecipients,
    UINT uiMessage,
    WPARAM wParam,
    LPARAM lParam,
    PBSMINFO pBSMInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
AlignRects(LPRECT rect, DWORD b, DWORD c, DWORD d)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
LRESULT
STDCALL
DefRawInputProc(
    PRAWINPUT* paRawInput,
    INT nInput,
    UINT cbSizeHeader)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetLayeredWindowAttributes(
    HWND hwnd,
    COLORREF *pcrKey,
    BYTE *pbAlpha,
    DWORD *pdwFlags)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetRawInputBuffer(
    PRAWINPUT   pData,
    PUINT    pcbSize,
    UINT         cbSizeHeader)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetRawInputData(
    HRAWINPUT    hRawInput,
    UINT         uiCommand,
    LPVOID      pData,
    PUINT    pcbSize,
    UINT         cbSizeHeader)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetRawInputDeviceList(
    PRAWINPUTDEVICELIST pRawInputDeviceList,
    PUINT puiNumDevices,
    UINT cbSize)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetRegisteredRawInputDevices(
    PRAWINPUTDEVICE pRawInputDevices,
    PUINT puiNumDevices,
    UINT cbSize)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
PrintWindow(
    HWND hwnd,
    HDC hdcBlt,
    UINT nFlags)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
RegisterRawInputDevices(
    PCRAWINPUTDEVICE pRawInputDevices,
    UINT uiNumDevices,
    UINT cbSize)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
UINT
STDCALL
WINNLSGetIMEHotkey( HWND hwnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
WINNLSEnableIME( HWND hwnd, BOOL enable)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
WINNLSGetEnableStatus( HWND hwnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
IMPSetIMEW( HWND hwnd, LPIMEPROW ime)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
IMPQueryIMEW( LPIMEPROW ime)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
IMPGetIMEW( HWND hwnd, LPIMEPROW ime)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
IMPSetIMEA( HWND hwnd, LPIMEPROA ime)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
IMPQueryIMEA( LPIMEPROA ime)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
IMPGetIMEA( HWND hwnd, LPIMEPROA ime)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
LRESULT
STDCALL
SendIMEMessageExW(HWND hwnd,LPARAM lparam)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
LRESULT
STDCALL
SendIMEMessageExA(HWND hwnd, LPARAM lparam)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL STDCALL DisplayExitWindowsWarnings(ULONG flags)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL STDCALL ReasonCodeNeedsBugID(ULONG reasoncode)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL STDCALL ReasonCodeNeedsComment(ULONG reasoncode)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL STDCALL CtxInitUser32(VOID)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL STDCALL EnterReaderModeHelper(HWND hwnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetAppCompatFlags2(HTASK hTask)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
VOID STDCALL InitializeLpkHooks(FARPROC *hookfuncs)
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
WORD STDCALL InitializeWin32EntryTable(UCHAR* EntryTablePlus0x1000)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL STDCALL IsServerSideWindow(HWND wnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

typedef BOOL (CALLBACK *THEME_HOOK_FUNC) (DWORD state,PVOID arg2); //return type and 2nd parameter unknown
/*
 * @unimplemented
 */
BOOL STDCALL RegisterUserApiHook(HINSTANCE instance,THEME_HOOK_FUNC proc)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL STDCALL UnregisterUserApiHook(VOID)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
HKL STDCALL LoadKeyboardLayoutEx(DWORD unknown,LPCWSTR pwszKLID,UINT Flags) //1st parameter unknown
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
VOID STDCALL AllowForegroundActivation(VOID)
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID STDCALL ShowStartGlass(DWORD unknown)
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOL STDCALL DdeGetQualityOfService(HWND hWnd, DWORD Reserved, PSECURITY_QUALITY_OF_SERVICE pqosPrev)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
DWORD STDCALL User32InitializeImmEntryTable(PVOID p)
{
  UNIMPLEMENTED;
  return 0;
}

