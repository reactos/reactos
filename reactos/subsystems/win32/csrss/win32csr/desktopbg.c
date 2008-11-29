/* $Id$
 *
 * reactos/subsys/csrss/win32csr/desktopbg.c
 *
 * Desktop background window functions
 *
 * ReactOS Operating System
 */

#include "w32csr.h"

#define NDEBUG
#include <debug.h>

#define DESKTOP_WINDOW_ATOM 32880

#define PM_SHOW_DESKTOP 1
#define PM_HIDE_DESKTOP 2

typedef struct tagDTBG_THREAD_DATA
{
  HDESK Desktop;
  HANDLE Event;
  NTSTATUS Status;
} DTBG_THREAD_DATA, *PDTBG_THREAD_DATA;

typedef struct tagPRIVATE_NOTIFY_DESKTOP
{
  NMHDR hdr;
  union
  {
    struct /* PM_SHOW_DESKTOP */
    {
      int Width;
      int Height;
    } ShowDesktop;
  };
} PRIVATE_NOTIFY_DESKTOP, *PPRIVATE_NOTIFY_DESKTOP;

static BOOL BgInitialized = FALSE;
static HWND VisibleDesktopWindow = NULL;

static LRESULT CALLBACK
DtbgWindowProc(HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  switch(Msg)
    {
      case WM_ERASEBKGND:
        return 1;

      case WM_PAINT:
      {
        PAINTSTRUCT PS;
        RECT rc;
        HDC hDC;

        if(GetUpdateRect(Wnd, &rc, FALSE) &&
           (hDC = BeginPaint(Wnd, &PS)))
        {
          PaintDesktop(hDC);
          EndPaint(Wnd, &PS);
        }
        return 0;
      }

      case WM_SETCURSOR:
	return (LRESULT) SetCursor(LoadCursorW(0, (LPCWSTR)IDC_ARROW));

      case WM_NCCREATE:
        return (LRESULT) TRUE;

      case WM_CREATE:
        return 0;

      case WM_NOTIFY:
      {
        PPRIVATE_NOTIFY_DESKTOP nmh = (PPRIVATE_NOTIFY_DESKTOP)lParam;

        /* Use WM_NOTIFY for private messages since it can't be sent between
           processes! */
        switch(nmh->hdr.code)
        {
          case PM_SHOW_DESKTOP:
          {
            LRESULT Result;

            Result = ! SetWindowPos(Wnd,
                                    NULL, 0, 0,
                                    nmh->ShowDesktop.Width,
                                    nmh->ShowDesktop.Height,
                                    SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW);
            UpdateWindow(Wnd);
            VisibleDesktopWindow = Wnd;
            return Result;
          }

          case PM_HIDE_DESKTOP:
          {
            LRESULT Result;

            Result = ! SetWindowPos(Wnd,
                                    NULL, 0, 0, 0, 0,
                                    SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE |
                                    SWP_HIDEWINDOW);
            UpdateWindow(Wnd);
            VisibleDesktopWindow = NULL;
            return Result;
          }

          default:
            DPRINT("Unknown notification code 0x%x sent to the desktop window!\n", nmh->hdr.code);
            return 0;
        }
      }
    }

  return 0;
}

static BOOL FASTCALL
DtbgInit()
{
  WNDCLASSEXW Class;
  ATOM ClassAtom;

  /*
   * Create the desktop window class
   */
  Class.cbSize = sizeof(WNDCLASSEXW);
  Class.style = 0;
  Class.lpfnWndProc = DtbgWindowProc;
  Class.cbClsExtra = 0;
  Class.cbWndExtra = 0;
  Class.hInstance = (HINSTANCE) GetModuleHandleW(NULL);
  Class.hIcon = NULL;
  Class.hCursor = NULL;
  Class.hbrBackground = GetSysColorBrush(COLOR_BACKGROUND);
  Class.lpszMenuName = NULL;
  Class.lpszClassName = (LPCWSTR) DESKTOP_WINDOW_ATOM;
  ClassAtom = RegisterClassExW(&Class);
  if ((ATOM) 0 == ClassAtom)
    {
      DPRINT1("Unable to register desktop background class (error %d)\n",
              GetLastError());
      return FALSE;
    }
  VisibleDesktopWindow = NULL;

  return TRUE;
}

static DWORD WINAPI
DtbgDesktopThread(PVOID Data)
{
  HWND BackgroundWnd;
  MSG msg;
  PDTBG_THREAD_DATA ThreadData = (PDTBG_THREAD_DATA) Data;

  if (! SetThreadDesktop(ThreadData->Desktop))
    {
      DPRINT1("Failed to set thread desktop\n");
      ThreadData->Status = STATUS_UNSUCCESSFUL;
      SetEvent(ThreadData->Event);
      return 1;
    }
  BackgroundWnd = CreateWindowW((LPCWSTR) DESKTOP_WINDOW_ATOM,
                                L"",
                                WS_POPUP | WS_CLIPCHILDREN,
                                0,
                                0,
                                0,
                                0,
                                NULL,
                                NULL,
                                (HINSTANCE) GetModuleHandleW(NULL),
                                NULL);
  if (NULL == BackgroundWnd)
    {
      DPRINT1("Failed to create desktop background window\n");
      ThreadData->Status = STATUS_UNSUCCESSFUL;
      SetEvent(ThreadData->Event);
      return 1;
    }

  ThreadData->Status = STATUS_SUCCESS;
  SetEvent(ThreadData->Event);

  while (GetMessageW(&msg, NULL, 0, 0))
    {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }

  return 1;
}

CSR_API(CsrCreateDesktop)
{
  DTBG_THREAD_DATA ThreadData;
  HANDLE ThreadHandle;

  DPRINT("CsrCreateDesktop\n");

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  if (! BgInitialized)
    {
      BgInitialized = TRUE;
      if (! DtbgInit())
        {
          return STATUS_UNSUCCESSFUL;
        }
    }

  /*
   * the desktop handle we got from win32k is in the scope of CSRSS so we can just use it
   */
  ThreadData.Desktop = Request->Data.CreateDesktopRequest.DesktopHandle;

  ThreadData.Event = CreateEventW(NULL, FALSE, FALSE, NULL);
  if (NULL == ThreadData.Event)
    {
      DPRINT1("Failed to create event (error %d)\n", GetLastError());
      return STATUS_UNSUCCESSFUL;
    }
  ThreadHandle = CreateThread(NULL,
                              0,
                              DtbgDesktopThread,
                              (PVOID) &ThreadData,
                              0,
                              NULL);
  if (NULL == ThreadHandle)
    {
      CloseHandle(ThreadData.Event);
      DPRINT1("Failed to create desktop window thread.\n");
      return STATUS_UNSUCCESSFUL;
    }
  CloseHandle(ThreadHandle);

  WaitForSingleObject(ThreadData.Event, INFINITE);
  CloseHandle(ThreadData.Event);

  return ThreadData.Status;
}

CSR_API(CsrShowDesktop)
{
  PRIVATE_NOTIFY_DESKTOP nmh;
  DPRINT("CsrShowDesktop\n");

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  nmh.hdr.hwndFrom = Request->Data.ShowDesktopRequest.DesktopWindow;
  nmh.hdr.idFrom = 0;
  nmh.hdr.code = PM_SHOW_DESKTOP;

  nmh.ShowDesktop.Width = (int)Request->Data.ShowDesktopRequest.Width;
  nmh.ShowDesktop.Height = (int)Request->Data.ShowDesktopRequest.Height;

  return SendMessageW(Request->Data.ShowDesktopRequest.DesktopWindow,
                      WM_NOTIFY,
                      (WPARAM)nmh.hdr.hwndFrom,
                      (LPARAM)&nmh)
         ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS;
}

CSR_API(CsrHideDesktop)
{
  PRIVATE_NOTIFY_DESKTOP nmh;
  DPRINT("CsrHideDesktop\n");

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  nmh.hdr.hwndFrom = Request->Data.ShowDesktopRequest.DesktopWindow;
  nmh.hdr.idFrom = 0;
  nmh.hdr.code = PM_HIDE_DESKTOP;

  return SendMessageW(Request->Data.ShowDesktopRequest.DesktopWindow,
                      WM_NOTIFY,
                      (WPARAM)nmh.hdr.hwndFrom,
                      (LPARAM)&nmh)
         ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS;
}

BOOL FASTCALL
DtbgIsDesktopVisible(VOID)
{
  if (NULL != VisibleDesktopWindow && ! IsWindowVisible(VisibleDesktopWindow))
    {
      VisibleDesktopWindow = NULL;
    }

  return NULL != VisibleDesktopWindow;
}

/* EOF */
