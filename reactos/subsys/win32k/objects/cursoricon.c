#undef WIN32_LEAN_AND_MEAN
#include <ddk/ntddk.h>
#include <ddk/ntddmou.h>
#include <win32k/win32k.h>
#include <windows.h>
#include <stdlib.h>
#include <win32k/cursoricon.h>
#include <win32k/bitmaps.h>
#include <include/winsta.h>
#include <include/error.h>
#include <include/mouse.h>
#include <include/window.h>
#include <internal/safe.h>

#define NDEBUG
#include <win32k/debug1.h>

/*
 * @unimplemented
 */
DWORD
STDCALL
NtUserGetIconInfo(
  HICON hIcon,
  PBOOL fIcon,
  PDWORD xHotspot,
  PDWORD yHotspot,
  HBITMAP *hbmMask,
  HBITMAP *hbmColor)
{
  UNIMPLEMENTED

  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
NtUserGetIconSize(
  HICON hIcon,
  BOOL *fIcon,
  LONG *Width,
  LONG *Height)
{
  UNIMPLEMENTED

  return FALSE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
NtUserGetCursorFrameInfo(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
NtUserGetCursorInfo(
  PCURSORINFO pci)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @implemented
 */
BOOL
STDCALL
NtUserClipCursor(
  RECT *UnsafeRect)
{
  /* FIXME - check if process has WINSTA_WRITEATTRIBUTES */
  
  PWINSTATION_OBJECT WinStaObject;
  PSYSTEM_CURSORINFO CurInfo;
  RECT Rect;
  PWINDOW_OBJECT DesktopWindow = NULL;

  NTSTATUS Status = ValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				       KernelMode,
				       0,
				       &WinStaObject);
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("Validation of window station handle (0x%X) failed\n",
      PROCESS_WINDOW_STATION());
    SetLastWin32Error(Status);
    return FALSE;
  }

  if (NULL != UnsafeRect && ! NT_SUCCESS(MmCopyFromCaller(&Rect, UnsafeRect, sizeof(RECT))))
  {
    ObDereferenceObject(WinStaObject);
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  CurInfo = &WinStaObject->SystemCursor;
  if(WinStaObject->ActiveDesktop)
    DesktopWindow = IntGetWindowObject(WinStaObject->ActiveDesktop->DesktopWindow);
  
  if((Rect.right > Rect.left) && (Rect.bottom > Rect.top)
     && DesktopWindow)
  {
    CurInfo->CursorClipInfo.IsClipped = TRUE;
    CurInfo->CursorClipInfo.Left = max(Rect.left, DesktopWindow->WindowRect.left);
    CurInfo->CursorClipInfo.Top = max(Rect.top, DesktopWindow->WindowRect.top);
    CurInfo->CursorClipInfo.Right = min(Rect.right - 1, DesktopWindow->WindowRect.right - 1);
    CurInfo->CursorClipInfo.Bottom = min(Rect.bottom - 1, DesktopWindow->WindowRect.bottom - 1);
    IntReleaseWindowObject(DesktopWindow);
    
    MouseMoveCursor(CurInfo->x, CurInfo->y);  
  }
  else
    WinStaObject->SystemCursor.CursorClipInfo.IsClipped = FALSE;
    
  ObDereferenceObject(WinStaObject);
  
  return TRUE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
NtUserDestroyCursor(
  HCURSOR hCursor,
  DWORD Unknown)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
NtUserFindExistingCursorIcon(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @implemented
 */
BOOL
STDCALL
NtUserGetClipCursor(
  RECT *lpRect)
{
  /* FIXME - check if process has WINSTA_READATTRIBUTES */
  
  PWINSTATION_OBJECT WinStaObject;
  RECT Rect;
  NTSTATUS Status;
  
  if(!lpRect)
    return FALSE;

  Status = ValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
	       KernelMode,
	       0,
	       &WinStaObject);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("Validation of window station handle (0x%X) failed\n",
      PROCESS_WINDOW_STATION());
    SetLastNtError(Status);
    return FALSE;
  }
  
  if(WinStaObject->SystemCursor.CursorClipInfo.IsClipped)
  {
    Rect.left = WinStaObject->SystemCursor.CursorClipInfo.Left;
    Rect.top = WinStaObject->SystemCursor.CursorClipInfo.Top;
    Rect.right = WinStaObject->SystemCursor.CursorClipInfo.Right;
    Rect.bottom = WinStaObject->SystemCursor.CursorClipInfo.Bottom;
  }
  else
  {
    Rect.left = 0;
    Rect.top = 0;
    Rect.right = NtUserGetSystemMetrics(SM_CXSCREEN);
    Rect.bottom = NtUserGetSystemMetrics(SM_CYSCREEN);
  }
  
  Status = MmCopyToCaller((PRECT)lpRect, &Rect, sizeof(RECT));
  if(!NT_SUCCESS(Status))
  {
    ObDereferenceObject(WinStaObject);
    SetLastNtError(Status);
    return FALSE;
  }
    
  ObDereferenceObject(WinStaObject);
  
  return TRUE;
}


/*
 * @unimplemented
 */
HCURSOR
STDCALL
NtUserSetCursor(
  HCURSOR hCursor)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
NtUserSetCursorContents(
  HCURSOR hCursor,
  DWORD Unknown)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
NtUserSetCursorIconData(
  HICON hIcon,
  PBOOL fIcon,
  PDWORD xHotspot,
  PDWORD yHotspot)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @implemented
 */
BOOL
STDCALL
NtUserSetSystemCursor(
  HCURSOR hcur,
  DWORD id)
{
  BOOL res = FALSE;

  return res;
}
