/* $Id: desktopbg.c,v 1.13 2004/12/21 21:38:26 weiden Exp $
 *
 * reactos/subsys/csrss/win32csr/desktopbg.c
 *
 * Desktop background window functions
 *
 * ReactOS Operating System
 */

/*
 * There is a problem with size of LPC_MESSAGE structure. In the old ReactOS
 * headers it doesn't contain the data field and so it has a different size.
 * We must use this workaround to get our Data field 0-sized.
 */

#include <windef.h>
#include <winnt.h>
#undef ANYSIZE_ARRAY
#define ANYSIZE_ARRAY 0
#include <ddk/ntapi.h>

#include <windows.h>
#include <csrss/csrss.h>
#include <user32/regcontrol.h>

#include "api.h"
#include "desktopbg.h"

#define NDEBUG
#include <debug.h>

extern BOOL STDCALL PrivateCsrssIsGUIActive(VOID);

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
            
            DPRINT("Show desktop: 0x%x (%d:%d)\n", Wnd, nmh->ShowDesktop.Width, nmh->ShowDesktop.Height);

            Result = SetWindowPos(Wnd,
                                  NULL, 0, 0,
                                  nmh->ShowDesktop.Width,
                                  nmh->ShowDesktop.Height,
                                  SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW);
            UpdateWindow(Wnd);
            return Result;
          }

          case PM_HIDE_DESKTOP:
          {
            LRESULT Result;
            
            DPRINT("Hide desktop: 0x%x\n", Wnd);

            Result = SetWindowPos(Wnd,
                                  NULL, 0, 0, 0, 0,
                                  SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE |
                                  SWP_HIDEWINDOW);
            UpdateWindow(Wnd);
            return Result;
          }

          default:
            DPRINT("Unknown notification code 0x%x sent to the desktop window!\n", nmh->code);
            return 0;
        }
      }
    }

  return 0;
}

static DWORD STDCALL
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
  
  DPRINT("Desktop thread running... (wnd: 0x%x, PID:%d)\n", BackgroundWnd, GetCurrentProcessId());

  while (GetMessageW(&msg, NULL, 0, 0))
    {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }

  DPRINT("Desktop thread terminating... (wnd: 0x%x)\n", BackgroundWnd);

  return 1;
}

CSR_API(CsrRegisterSystemClasses)
{
  WNDCLASSEXW wc;
  
  /* The hWindowStation handle is only valid while processing this request! */
  
  /*
   * This routine is called when creating an interactive window station. It sets
   * up all system window classes so applications and csrss can use them later.
   */
  
  DPRINT("CsrRegisterSystemClasses\n");
  
  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - LPC_MESSAGE_BASE_SIZE;
  
  /*
   * Register the system window classes (buttons, edit controls, ...) that are
   * managed by user32
   */
  if(!PrivateCsrssRegisterBuiltinSystemWindowClasses(Request->Data.RegisterSystemClassesRequest.hWindowStation))
  {
    DPRINT1("Unable to register builtin system window classes: LastError: %d\n", GetLastError());
    return Reply->Status = STATUS_UNSUCCESSFUL;
  }
  
  /*
   * Register the desktop window class
   */
  wc.cbSize = sizeof(WNDCLASSEXW);
  wc.style = 0;
  wc.lpfnWndProc = DtbgWindowProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = (HINSTANCE) GetModuleHandleW(NULL);
  wc.hIcon = NULL;
  wc.hCursor = NULL;
  wc.hbrBackground = GetSysColorBrush(COLOR_BACKGROUND);
  wc.lpszMenuName = NULL;
  wc.lpszClassName = (LPCWSTR) DESKTOP_WINDOW_ATOM;
  /* we don't support an ansi version of the window procedure, so don't specify it! */
  if(!PrivateCsrssRegisterSystemWindowClass(Request->Data.RegisterSystemClassesRequest.hWindowStation,
                                            &wc,
                                            NULL))
  {
    DPRINT1("Unable to register the desktop window class: LastError: %d\n", GetLastError());
    return Reply->Status = STATUS_UNSUCCESSFUL;
  }
  
  Reply->Data.RegisterSystemClassesReply.hWindowStation = Request->Data.RegisterSystemClassesRequest.hWindowStation;

  return Reply->Status = STATUS_SUCCESS;
}

CSR_API(CsrCreateDesktop)
{
  DTBG_THREAD_DATA ThreadData;
  HANDLE ThreadHandle;

  DPRINT("CsrCreateDesktop (PID:%d)\n", GetCurrentProcessId());

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - LPC_MESSAGE_BASE_SIZE;

  /*
   * the desktop handle we got from win32k is in the scope of CSRSS so we can just use it
   */
  ThreadData.Desktop = Request->Data.CreateDesktopRequest.DesktopHandle;

  ThreadData.Event = CreateEventW(NULL, FALSE, FALSE, NULL);
  if (NULL == ThreadData.Event)
    {
      DPRINT1("Failed to create event (error %d)\n", GetLastError());
      return Reply->Status = STATUS_UNSUCCESSFUL;
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
      return Reply->Status = STATUS_UNSUCCESSFUL;
    }
  /* FIXME - we should wait on the thread handle as well, it may happen that the
             thread crashes or doesn't start at all, we should catch this case
             instead of waiting forever! */
  CloseHandle(ThreadHandle);
  WaitForSingleObject(ThreadData.Event, INFINITE);
  CloseHandle(ThreadData.Event);

  Reply->Status = ThreadData.Status;

  return Reply->Status;
}

CSR_API(CsrShowDesktop)
{
  PRIVATE_NOTIFY_DESKTOP nmh;
  
  /* The hDesktop handle is only valid during processing this request! */

  DPRINT("CsrShowDesktop (hwnd: 0x%x) (PID:%d)\n", Request->Data.ShowDesktopRequest.DesktopWindow, GetCurrentProcessId());
  
  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - LPC_MESSAGE_BASE_SIZE;
  
  /* We need to set the desktop for this thread to be able to send the messages
     to the desktop thread! */
  if(!SetThreadDesktop(Request->Data.ShowDesktopRequest.hDesktop))
  {
    DPRINT1("CsrShowDesktop: Failed to set thread desktop!\n");
    return Reply->Status = STATUS_UNSUCCESSFUL;
  }

  nmh.hdr.hwndFrom = Request->Data.ShowDesktopRequest.DesktopWindow;
  nmh.hdr.idFrom = 0;
  nmh.hdr.code = PM_SHOW_DESKTOP;

  nmh.ShowDesktop.Width = (int)Request->Data.ShowDesktopRequest.Width;
  nmh.ShowDesktop.Height = (int)Request->Data.ShowDesktopRequest.Height;

  Reply->Status = SendMessageW(Request->Data.ShowDesktopRequest.DesktopWindow,
                               WM_NOTIFY,
                               (WPARAM)nmh.hdr.hwndFrom,
                               (LPARAM)&nmh)
                  ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;

  DPRINT("CsrShowDesktop: SendMessageW (Status: 0x%x), LastError: %d\n", Reply->Status, GetLastError());

  return Reply->Status;
}

CSR_API(CsrHideDesktop)
{
  PRIVATE_NOTIFY_DESKTOP nmh;
  
  /* The hDesktop handle is only valid while processing this request! */

  DPRINT1("CsrHideDesktop (hwnd: 0x%x) (PID:%d)\n", Request->Data.ShowDesktopRequest.DesktopWindow, GetCurrentProcessId());

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - LPC_MESSAGE_BASE_SIZE;
  
  /* We need to set the desktop for this thread to be able to send the messages
     to the desktop thread! */
  if(!SetThreadDesktop(Request->Data.ShowDesktopRequest.hDesktop))
  {
    DPRINT1("CsrShowDesktop: Failed to set thread desktop!\n");
    return Reply->Status = STATUS_UNSUCCESSFUL;
  }

  nmh.hdr.hwndFrom = Request->Data.ShowDesktopRequest.DesktopWindow;
  nmh.hdr.idFrom = 0;
  nmh.hdr.code = PM_HIDE_DESKTOP;

  Reply->Status = SendMessageW(Request->Data.ShowDesktopRequest.DesktopWindow,
                               WM_NOTIFY,
                               (WPARAM)nmh.hdr.hwndFrom,
                               (LPARAM)&nmh)
                  ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;

  return Reply->Status;
}

BOOL FASTCALL
DtbgIsDesktopVisible(VOID)
{
  /* FIXME - This is a hack, it's not possible to determine whether a desktop
             is visible or not unless a handle is supplied! we just check through
             a private api if we're running in GUI mode */
  return PrivateCsrssIsGUIActive();
}

/* EOF */
