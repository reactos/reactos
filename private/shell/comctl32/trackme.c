//---------------------------------------------------------------------------
//
//     TrackME.C   (TrackMouseEvent)
//
// Created by:  Sankar  on 1/24/96
//
// What:
//     This emulates the TrackMouseEvent() API for the Nashville project
//     in comctl32.dll
//
// How:
//     This subclasses the given window to get mouse messages and uses a 
//     high frequency timer to learn about mouse leaves.
//
//---------------------------------------------------------------------------

#include "ctlspriv.h"

#ifdef TrackMouseEvent
#undef TrackMouseEvent
#endif

extern const TCHAR FAR c_szTMEdata[];

extern DWORD g_dwHoverSelectTimeout;

#define ID_MOUSEHOVER          0xFFFFFFF0L
#define ID_MOUSELEAVE          0xFFFFFFF1L

#define TME_MOUSELEAVE_TIME    (GetDoubleClickTime() / 5)

#define IsKeyDown(Key)   (GetKeyState(Key) & 0x8000)

// This is the structure whose pointer gets added as a property of a window
// being tracked.
typedef struct  tagTMEDATA {
       TRACKMOUSEEVENT TrackMouseEvent;
       RECT            rcMouseHover;  //In screen co-ordinates.
   }  TMEDATA, FAR *LPTMEDATA;


void NEAR TME_ResetMouseHover(LPTRACKMOUSEEVENT lpTME, LPTMEDATA lpTMEdata);
LRESULT CALLBACK TME_SubclassProc(HWND hwnd, UINT message, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, ULONG_PTR dwRefData);

LPTMEDATA NEAR GetTMEdata(HWND hwnd)
{
    LPTMEDATA lpTMEdata;

    GetWindowSubclass(hwnd, TME_SubclassProc, 0, (ULONG_PTR *)&lpTMEdata);

    return lpTMEdata;
}

void NEAR TME_PostMouseLeave(HWND hwnd)
{
  PostMessage(hwnd, WM_MOUSELEAVE, 0, 0L);
}

void NEAR TME_CancelMouseLeave(LPTMEDATA lpTMEdata)
{
  if(!(lpTMEdata->TrackMouseEvent.dwFlags & TME_LEAVE))
      return;

  // Remove the flag.
  lpTMEdata->TrackMouseEvent.dwFlags &= ~(TME_LEAVE);

  // We leave the timer set here since our hover implementation uses it too.
  // TME_CancelTracking will kill it later.
}

void NEAR TME_CancelMouseHover(LPTMEDATA lpTMEdata)
{
  if(!(lpTMEdata->TrackMouseEvent.dwFlags & TME_HOVER))
      return;

  lpTMEdata->TrackMouseEvent.dwFlags &= ~(TME_HOVER);

  KillTimer(lpTMEdata->TrackMouseEvent.hwndTrack, ID_MOUSEHOVER);
}

void NEAR TME_CancelTracking(LPTMEDATA lpTMEdata)
{
  HWND hwndTrack;

  //If either MouseLeave or MouseHover is ON, don't cancel tracking.
  if(lpTMEdata->TrackMouseEvent.dwFlags & (TME_HOVER | TME_LEAVE))
      return;

  hwndTrack = lpTMEdata->TrackMouseEvent.hwndTrack;

  // Uninstall our subclass callback.
  RemoveWindowSubclass(hwndTrack, TME_SubclassProc, 0);

  // Kill the mouseleave timer.
  KillTimer(hwndTrack, ID_MOUSELEAVE);

  // Free the tracking data.
  LocalFree((HANDLE)lpTMEdata);
}

void NEAR TME_RemoveAllTracking(LPTMEDATA lpTMEdata)
{
  TME_CancelMouseLeave(lpTMEdata);
  TME_CancelMouseHover(lpTMEdata);
  TME_CancelTracking(lpTMEdata);
}

//---------------------------------------------------------------------------
//
// TME_MouseHasLeft()
//     The mouse has left the region being tracked. Send the MOUSELEAVE msg
// and then cancel all tracking.
//
//---------------------------------------------------------------------------
void NEAR TME_MouseHasLeft(LPTMEDATA  lpTMEdata)
{
  DWORD  dwFlags;

  //Is WM_MOUSELEAVE notification requied?
  if((dwFlags = lpTMEdata->TrackMouseEvent.dwFlags) & TME_LEAVE)
      TME_PostMouseLeave(lpTMEdata->TrackMouseEvent.hwndTrack); //Then, do it!

  // Cancel all the tracking since the mouse has left.
  TME_RemoveAllTracking(lpTMEdata);
}

// --------------------------------------------------------------------------
//  
//  TME_SubclassWndProc()
//  
//  The subclass proc used for TrackMouseEvent()...!
//
// --------------------------------------------------------------------------

LRESULT CALLBACK TME_SubclassProc(HWND hwnd, UINT message, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, ULONG_PTR dwRefData)
{
      LPTMEDATA lpTMEdata = (LPTMEDATA)dwRefData;

      ASSERT(lpTMEdata);

      switch(message)
        {
          case WM_DESTROY:
          case WM_NCDESTROY:
              TME_RemoveAllTracking(lpTMEdata);
              break;

          case WM_ENTERMENULOOP:
              // If the window being tracked enters menu mode, then we need to
              // act asif the mouse has left.
              // NOTE: Because when we are in menu mode, the SCREEN_CAPTURE has occurred
              // and we don't see any mouse moves. This is the only way out!

              // Post mouse leave and cancel all tracking!
              TME_MouseHasLeft(lpTMEdata);
              break;

          case WM_LBUTTONDOWN:
          case WM_LBUTTONUP:
          case WM_MBUTTONDOWN:
          case WM_MBUTTONUP:
          case WM_RBUTTONDOWN:
          case WM_RBUTTONUP:
          case WM_NCLBUTTONDOWN:
          case WM_NCLBUTTONUP:
          case WM_NCMBUTTONDOWN:
          case WM_NCMBUTTONUP:
          case WM_NCRBUTTONDOWN:
          case WM_NCRBUTTONUP:
              //Whenever there is a mouse click, reset mouse hover.
              if(lpTMEdata->TrackMouseEvent.dwFlags & TME_HOVER)
                  TME_ResetMouseHover(&(lpTMEdata->TrackMouseEvent), lpTMEdata);
              break;

          case WM_NCMOUSEMOVE:
              TME_MouseHasLeft(lpTMEdata);
              break;

          case WM_MOUSEMOVE:
              {
                POINT Pt;

                Pt.x = GET_X_LPARAM(lParam);
                Pt.y = GET_Y_LPARAM(lParam);

                ClientToScreen(hwnd, &Pt);
 
                //Check if the mouse is within the hover rect.
                if((lpTMEdata->TrackMouseEvent.dwFlags & TME_HOVER) &&
                   !PtInRect(&(lpTMEdata->rcMouseHover), Pt))
                    TME_ResetMouseHover(&(lpTMEdata->TrackMouseEvent), lpTMEdata);
              }
              break;
        }

      return DefSubclassProc(hwnd, message, wParam, lParam);
}

// --------------------------------------------------------------------------
//  
//  TME_CheckInWindow()
//  
//  This get the current cursor position and checks if it still lies in the
//  "valid" area.
//   Returns TRUE, if it lies in the valid area.
//   FALSE, otherwise.
//
// --------------------------------------------------------------------------

BOOL NEAR TME_CheckInWindow(LPTRACKMOUSEEVENT lpTME, LPPOINT lpPt)
{
    POINT      pt;
    HWND       hwnd;   // Given window.
    HWND       hwndPt; //Window from the given point.
    HWND       hwndCapture;

    hwnd = lpTME->hwndTrack;  //Given window handle.

    //See if anyone has captured the mouse input.
    if((hwndCapture = GetCapture()) && IsWindow(hwndCapture))
      {
        // If tracking is required for a window other than the one that
        // has the capture, forget it! It is not possible!

        if(hwndCapture != hwnd)
            return(FALSE);
      }

    GetCursorPos(&pt);  //Get cursor point in screen co-ordinates.

    if (!hwndCapture)
    {
        hwndPt = WindowFromPoint(pt);

        if (!hwndPt || !IsWindow(hwndPt) || (hwnd != hwndPt))
            return FALSE;

        if (SendMessage(hwnd, WM_NCHITTEST, 0,
            MAKELPARAM((SHORT)pt.x, (SHORT)pt.y)) != HTCLIENT)
        {
            return FALSE;
        }
    }

    // The current point falls on the same area of the same window.
    // It is a valid location.
    if (lpPt)
        *lpPt = pt;

    return(TRUE);
}

// --------------------------------------------------------------------------
//  TME_MouseLeaveTimer()
//
//  Timer callback for WM_MOUSELEAVE generation and cancelling HOVER!
//
// --------------------------------------------------------------------------
VOID CALLBACK TME_MouseLeaveTimer(HWND hwnd, UINT msg, UINT id, DWORD dwTime)
{
    LPTMEDATA  lpTMEdata;

    if(!(lpTMEdata = GetTMEdata(hwnd)))
        return;

    // YIELD!!!
    if(TME_CheckInWindow(&(lpTMEdata->TrackMouseEvent), NULL))
        return;  //The mouse is still in the valid region. So, do nothing.

    if (!IsWindow(hwnd))
        return;

    //The mouse has left the valid region. So, post mouse-leave if requested
    //Because we are cancelling mouse-leave, we need to cancel mouse-hover too!
    // There can be no hover tracking, if the mouse has already left!

    TME_MouseHasLeft(lpTMEdata);
}


WPARAM NEAR GetMouseKeyFlags()
{
    WPARAM wParam = 0;

    if (IsKeyDown(VK_LBUTTON))
        wParam |= MK_LBUTTON;
    if (IsKeyDown(VK_RBUTTON))
        wParam |= MK_RBUTTON;
    if (IsKeyDown(VK_MBUTTON))
        wParam |= MK_MBUTTON;
    if (IsKeyDown(VK_SHIFT))
        wParam |= MK_SHIFT;
    if (IsKeyDown(VK_CONTROL))
        wParam |= MK_CONTROL;

    return wParam;
}

// --------------------------------------------------------------------------
//  TME_MouseHoverTimer()
//
//  Timer callback for WM_MOUSEHOVER/WM_NCMOUSEHOVER generation.
//
// --------------------------------------------------------------------------
VOID CALLBACK TME_MouseHoverTimer(HWND hwnd, UINT msg, UINT id, DWORD dwTime)
{
    POINT pt;
    WPARAM wParam;
    LPTMEDATA lpTMEdata;

    if (!(lpTMEdata = GetTMEdata(hwnd)))
        return;

    //BOGUS: we can not detect hwndSysModal from here!
    //Also, tracking is for a per-window basis now!
    //
    // BOGUS: We don't have to worry about JournalPlayback?
    //pt = fJournalPlayback? Lpq(hwnd->hq)->ptLast : ptTrueCursor;

    // YIELD!!!
    if(!TME_CheckInWindow(&(lpTMEdata->TrackMouseEvent), &pt))
      {
        // Mouse has left the valid region of the window. So, cancel all
        // the tracking.
        TME_MouseHasLeft(lpTMEdata);
        return;
      }

    if (!IsWindow(hwnd))
        return;

    if (!PtInRect(&(lpTMEdata->rcMouseHover), pt))
      {
        // Mouse has gone out of the hover rectangle. Reset the hovering.
        TME_ResetMouseHover(&(lpTMEdata->TrackMouseEvent), lpTMEdata);
        return;
      }

    //
    // set up to check the tolerance and 
    //
    wParam = GetMouseKeyFlags();
    ScreenToClient(hwnd, &pt);

    //Mouse is still within the hover rectangle. Let's post hover msg
    PostMessage(hwnd, WM_MOUSEHOVER, wParam, MAKELPARAM(pt.x, pt.y));

    //And then cancel the hovering.
    TME_CancelMouseHover(lpTMEdata);
    TME_CancelTracking(lpTMEdata);  //Cancel the tracking, if needed.
}

BOOL NEAR TME_SubclassWnd(LPTMEDATA lpTMEdata)
{
    BOOL fResult;

    fResult = SetWindowSubclass(lpTMEdata->TrackMouseEvent.hwndTrack,
        TME_SubclassProc, 0, (ULONG_PTR)lpTMEdata);

    ASSERT(fResult);
    return fResult;
}

void NEAR TME_ResetMouseLeave(LPTRACKMOUSEEVENT lpTME, LPTMEDATA lpTMEdata)
{
  //See if already MouseLeave is being tracked.
  if(lpTMEdata->TrackMouseEvent.dwFlags & TME_LEAVE)
      return;   // Nothing else to do.
  
  //Else, set the flag.
  lpTMEdata ->TrackMouseEvent.dwFlags |= TME_LEAVE;

  //Set the high frequency Timer.
  SetTimer(lpTME->hwndTrack, ID_MOUSELEAVE, TME_MOUSELEAVE_TIME, (TIMERPROC)TME_MouseLeaveTimer);
}

void NEAR TME_ResetMouseHover(LPTRACKMOUSEEVENT lpTME, LPTMEDATA lpTMEdata)
{
    DWORD  dwMouseHoverTime;
    POINT  pt;

    // Even if the hover tracking is already happening, the caller might 
    // change the timer value, restart the timer or change the hover 
    // rectangle.
    lpTMEdata->TrackMouseEvent.dwFlags |= TME_HOVER;

    dwMouseHoverTime = lpTME->dwHoverTime;
    if (!dwMouseHoverTime || (dwMouseHoverTime == HOVER_DEFAULT))
        dwMouseHoverTime = (g_dwHoverSelectTimeout ? g_dwHoverSelectTimeout : GetDoubleClickTime()*4/5); // BUGBUG: Can't we remember this?
    GetCursorPos(&pt);

    //
    // update the tolerance rectangle for the hover window.
    //
    *((POINT *)&(lpTMEdata->rcMouseHover.left)) = *((POINT *)&(lpTMEdata->rcMouseHover.right)) = pt;

    //BOGUS: Can we use globals to remeber these metrics. What about NT?
    InflateRect(&(lpTMEdata->rcMouseHover), g_cxDoubleClk/2, g_cyDoubleClk/2);
                       
    // We need to remember the timer interval we are setting. This value
    // needs to be returned when TME_QUERY is used.
    lpTME->dwHoverTime = dwMouseHoverTime;
    lpTMEdata->TrackMouseEvent.dwHoverTime = dwMouseHoverTime;
    SetTimer(lpTME->hwndTrack, ID_MOUSEHOVER, dwMouseHoverTime, (TIMERPROC)TME_MouseHoverTimer);
}

// --------------------------------------------------------------------------
//  QueryTrackMouseEvent()
//
//  Fills in a TRACKMOUSEEVENT structure describing current tracking state
//  for a given window. The given window is in lpTME->hwndTrack.
//
// --------------------------------------------------------------------------
BOOL NEAR QueryTrackMouseEvent(LPTRACKMOUSEEVENT lpTME)
{
    HWND hwndTrack;
    LPTMEDATA lpTMEdata;

    //
    // if there isn't anything being tracked get out
    //
    if((!(hwndTrack = lpTME->hwndTrack)) || !IsWindow(hwndTrack))
        goto Sorry;

    if(!(lpTMEdata = GetTMEdata(hwndTrack)))
        goto Sorry;

    if(!(lpTMEdata->TrackMouseEvent.dwFlags & (TME_HOVER | TME_LEAVE)))
        goto Sorry;

    //
    // fill in the requested information
    //
    lpTME->dwFlags = lpTMEdata->TrackMouseEvent.dwFlags;

    if (lpTMEdata->TrackMouseEvent.dwFlags & TME_HOVER)
        lpTME->dwHoverTime = lpTMEdata->TrackMouseEvent.dwHoverTime;
    else
        lpTME->dwHoverTime = 0;

    goto Done;

Sorry:
    // zero out the struct
    lpTME->dwFlags = 0;
    lpTME->hwndTrack = NULL;
    lpTME->dwHoverTime = 0;

Done:
    return TRUE;
}


// --------------------------------------------------------------------------
//  EmulateTrackMouseEvent()
//
//  emulate API for requesting extended mouse notifications (hover, leave...)
//
// --------------------------------------------------------------------------
BOOL WINAPI EmulateTrackMouseEvent(LPTRACKMOUSEEVENT lpTME)
{
    HWND    hwnd;
    DWORD   dwFlags;
    LPTMEDATA  lpTMEdata;

    if (lpTME->dwFlags & ~TME_VALID)
        return FALSE;

#ifdef TME_NONCLIENT
    //
    // this implementation does not handle TME_NONCLIENT (anymore)
    // we agreed with the NT team to rip it out until the system uses it...
    //
    if (lpTME->dwFlags & TME_NONCLIENT)
        return FALSE;
#endif

    //
    // implement queries separately
    //
    if (lpTME->dwFlags & TME_QUERY)
        return QueryTrackMouseEvent(lpTME);
    
    // 
    // Check the validity of the request.
    //
    hwnd = lpTME->hwndTrack;
    dwFlags = lpTME->dwFlags;

    if (!IsWindow(hwnd))
        return FALSE;

    // Check if the mouse is currently in a valid position
    // Use GetCursorPos() to get the mouse position and then check if
    // it lies within the client/non-client portion of the window as
    // defined in this call;

    // YIELD!!!
    if(!TME_CheckInWindow(lpTME, NULL))
      {
        //If the mouse leave is requested when the mouse is already outside
        // the window, then generate one mouse leave immly.
        if((dwFlags & TME_LEAVE) && !(dwFlags & TME_CANCEL))
            TME_PostMouseLeave(hwnd);
        
        //Because it is an invalid request, we return immly.
        return(TRUE);
      }

    if (!IsWindow(hwnd))
        return FALSE;

    //It is a valid request, either to install or remove tracking.

    //See if we already have tracking for this window.
    if(!(lpTMEdata = GetTMEdata(hwnd)))
      {
        //We are not tracking this window already.
        if(dwFlags & TME_CANCEL)
            return(TRUE);   //There is nothing to cancel; Ignore!
        
        //Do they want any tracking at all?
        ASSERT(dwFlags & (TME_HOVER | TME_LEAVE));

        //Allocate global mem to remember the tracking data
        if(!(lpTMEdata = (LPTMEDATA)LocalAlloc(LPTR, sizeof(TMEDATA))))
            return(FALSE);

        // copy in the hwnd
        lpTMEdata->TrackMouseEvent.hwndTrack = lpTME->hwndTrack;

        // Make sure our subclass callback is installed.
        if (!TME_SubclassWnd(lpTMEdata))
          {
            TME_CancelTracking(lpTMEdata);
            return(FALSE);
          }
      }

    //Else fall through!

    if(dwFlags & TME_CANCEL)
      {
        if(dwFlags & TME_HOVER)
            TME_CancelMouseHover(lpTMEdata);
        
        if(dwFlags & TME_LEAVE)
            TME_CancelMouseLeave(lpTMEdata);

        // If both hover and leave are cancelled, then we don't need any
        // tracking.
        TME_CancelTracking(lpTMEdata);

        return(TRUE); // Cancelled whatever they asked for.
      }

    if(dwFlags & TME_HOVER)
        TME_ResetMouseHover(lpTME, lpTMEdata);

    if(dwFlags & TME_LEAVE)
        TME_ResetMouseLeave(lpTME, lpTMEdata);

    return(TRUE);
}

typedef BOOL (WINAPI* PFNTME)(LPTRACKMOUSEEVENT);

PFNTME g_pfnTME = NULL;

// --------------------------------------------------------------------------
//  _TrackMouseEvent() entrypoint
//
//  calls TrackMouseEvent if present, otherwise uses EmulateTrackMouseEvent
//
// --------------------------------------------------------------------------
BOOL WINAPI _TrackMouseEvent(LPTRACKMOUSEEVENT lpTME)
{
    if (!g_pfnTME)
    {
        HMODULE hmod = GetModuleHandle(TEXT("USER32"));

        if (hmod)
            g_pfnTME = (PFNTME)GetProcAddress(hmod, "TrackMouseEvent");

        if (!g_pfnTME)
            g_pfnTME = EmulateTrackMouseEvent;
    }

    return g_pfnTME(lpTME);
}

