// --------------------------------------------------------------------------
//
//  WINABLE.H
//
//  Hooking mechanism to receive system events.
//
// --------------------------------------------------------------------------

#ifndef _WINABLE_
#define _WINABLE_

#if !defined(_WINABLE_)
#define WINABLEAPI  DECLSPEC_IMPORT
#else
#define WINABLEAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdarg.h>

#if (WINVER < 0x0500) // these structures and functions
		    // are in NT 5.00 and above winuser.h
//
// In USER32
//

//
// This gets GUI information out of context.  If you pass in a NULL thread ID,
// we will get the 'global' information, using the foreground thread.  This
// is guaranteed to be the real active window, focus window, etc.  Yes, you
// could do it yourself by calling GetForegorundWindow, getting the thread ID
// of that window via GetWindowThreadProcessId, then passing the ID into
// GetGUIThreadInfo().  However, that takes three calls and aside from being
// a pain, anything could happen in the middle.  So passing in NULL gets
// you stuff in one call and hence also works right.
//
typedef struct tagGUITHREADINFO
{
    DWORD   cbSize;
    DWORD   flags;
    HWND    hwndActive;
    HWND    hwndFocus;
    HWND    hwndCapture;
    HWND    hwndMenuOwner;
    HWND    hwndMoveSize;
    HWND    hwndCaret;
    RECT    rcCaret;
} GUITHREADINFO, FAR * LPGUITHREADINFO;

#define GUI_CARETBLINKING   0x00000001
#define GUI_INMOVESIZE      0x00000002
#define GUI_INMENUMODE      0x00000004
#define GUI_SYSTEMMENUMODE  0x00000008
#define GUI_POPUPMENUMODE   0x00000010


BOOL
WINAPI
GetGUIThreadInfo(
    DWORD   idThread,
    LPGUITHREADINFO lpgui
);


UINT
WINAPI
GetWindowModuleFileNameW(
    HWND    hwnd,
    LPWSTR  lpFileName,
    UINT    cchFileName
);

UINT
WINAPI
GetWindowModuleFileNameA(
    HWND    hwnd,
    LPSTR   lpFileName,
    UINT    cchFileName
);

#ifdef UNICODE
#define GetWindowModuleFileName        GetWindowModuleFileNameW
#else
#define GetWindowModuleFileName        GetWindowModuleFileNameA
#endif

#endif // WINVER < 0x0500

//
// This returns FALSE if the caller doesn't have permissions to do this
// esp. if someone else is dorking with input.  I.E., if some other thread
// disabled input, and thread 2 tries to diable/enable it, the call will
// fail since thread 1 has the cookie.
//
BOOL
WINAPI
BlockInput(
    BOOL fBlockIt
);



#if (_WIN32_WINNT < 0x0403) // these structures and this function prototype
							// are in NT 4.03 and above winuser.h

//
// Note that the dwFlags field uses the same flags as keybd_event and
// mouse_event, depending on what type of input this is.
//
typedef struct tagMOUSEINPUT {
    LONG    dx;
    LONG    dy;
    DWORD   mouseData;
    DWORD   dwFlags;
    DWORD   time;
    DWORD   dwExtraInfo;
} MOUSEINPUT, *PMOUSEINPUT, FAR* LPMOUSEINPUT;

typedef struct tagKEYBDINPUT {
    WORD    wVk;
    WORD    wScan;
    DWORD   dwFlags;
    DWORD   time;
    DWORD   dwExtraInfo;
} KEYBDINPUT, *PKEYBDINPUT, FAR* LPKEYBDINPUT;

typedef struct tagHARDWAREINPUT {
    DWORD   uMsg;
    WORD    wParamL;
    WORD    wParamH;
	DWORD	dwExtraInfo;
} HARDWAREINPUT, *PHARDWAREINPUT, FAR* LPHARDWAREINPUT;

#define INPUT_MOUSE     0
#define INPUT_KEYBOARD  1
#define INPUT_HARDWARE  2

typedef struct tagINPUT {
    DWORD   type;

    union
    {
        MOUSEINPUT      mi;
        KEYBDINPUT      ki;
        HARDWAREINPUT   hi;
    };
} INPUT, *PINPUT, FAR* LPINPUT;

//
// This returns the number of inputs played back.  It will disable input
// first, play back as many as possible, then reenable input.  In the middle
// it will pulse the RIT to make sure that the fixed input queue doesn't
// fill up.
//
UINT
WINAPI
SendInput(
    UINT    cInputs,     // number of input in the array
    LPINPUT pInputs,     // array of inputs
    int     cbSize);     // sizeof(INPUT)

#endif // (_WIN32_WINNT < 0x0403)


#define     CCHILDREN_FRAME     7

#if WINVER < 0x0500 // these structures and functions
		    // are in NT 5.00 and above winuser.h

//
// This generates a notification that anyone watching for it will get.
// This call is superfast if nobody is hooking anything.
//
WINABLEAPI
void
WINAPI
NotifyWinEvent(
    DWORD   event,
    HWND    hwnd,
    LONG    idObject,
    LONG    idChild
);



//
// hwnd + idObject can be used with OLEACC.DLL's OleGetObjectFromWindow()
// to get an interface pointer to the container.  indexChild is the item
// within the container in question.  Setup a VARIANT with vt VT_I4 and 
// lVal the indexChild and pass that in to all methods.  Then you 
// are raring to go.
//


//
// Common object IDs (cookies, only for sending WM_GETOBJECT to get at the
// thing in question).  Positive IDs are reserved for apps (app specific),
// negative IDs are system things and are global, 0 means "just little old
// me".
//
#define     CHILDID_SELF        0

// Reserved IDs for system objects
#define     OBJID_WINDOW        0x00000000
#define     OBJID_SYSMENU       0xFFFFFFFF
#define     OBJID_TITLEBAR      0xFFFFFFFE
#define     OBJID_MENU          0xFFFFFFFD
#define     OBJID_CLIENT        0xFFFFFFFC
#define     OBJID_VSCROLL       0xFFFFFFFB
#define     OBJID_HSCROLL       0xFFFFFFFA
#define     OBJID_SIZEGRIP      0xFFFFFFF9
#define     OBJID_CARET         0xFFFFFFF8
#define     OBJID_CURSOR        0xFFFFFFF7
#define     OBJID_ALERT         0xFFFFFFF6
#define     OBJID_SOUND         0xFFFFFFF5

#define     CCHILDREN_FRAME     7

//
// System Alerts (indexChild of system ALERT notification)
//
#define ALERT_SYSTEM_INFORMATIONAL      1       // MB_INFORMATION
#define ALERT_SYSTEM_WARNING            2       // MB_WARNING
#define ALERT_SYSTEM_ERROR              3       // MB_ERROR
#define ALERT_SYSTEM_QUERY              4       // MB_QUESTION
#define ALERT_SYSTEM_CRITICAL           5       // HardSysErrBox
#define CALERT_SYSTEM                   6



typedef DWORD   HWINEVENTHOOK;

typedef VOID (CALLBACK* WINEVENTPROC)(
    HWINEVENTHOOK  hEvent,
    DWORD   event,
    HWND    hwnd,
    LONG    idObject,
    LONG    idChild,
    DWORD   idEventThread,
    DWORD   dwmsEventTime);


#define WINEVENT_OUTOFCONTEXT   0x0000  // Events are ASYNC
#define WINEVENT_SKIPOWNTHREAD  0x0001  // Don't call back for events on installer's thread
#define WINEVENT_SKIPOWNPROCESS 0x0002  // Don't call back for events on installer's process
#define WINEVENT_INCONTEXT      0x0004  // Events are SYNC, this causes your dll to be injected into every process
#define WINEVENT_32BITCALLER    0x8000  // ;Internal
#define WINEVENT_VALID          0x8007  // ;Internal


WINABLEAPI
HWINEVENTHOOK
WINAPI
SetWinEventHook(
    DWORD           eventMin,
    DWORD           eventMax,
    HMODULE         hmodWinEventProc,   // Must pass this if global!
    WINEVENTPROC    lpfnWinEventProc,
    DWORD           idProcess,          // Can be zero; all processes
    DWORD           idThread,           // Can be zero; all threads
    DWORD           dwFlags
);

//
// Returns zero on failure, or a DWORD ID if success.  We will clean up any
// event hooks installed by the current process when it goes away, if it
// hasn't cleaned the hooks up itself.  But to dynamically unhook, call
// UnhookWinEvents().
//


WINABLEAPI
BOOL
WINAPI
UnhookWinEvent(
    HWINEVENTHOOK          hEvent);

//
// If idProcess isn't zero but idThread is, will hook all threads in that
//      process.
// If idThread isn't zero but idProcess is, will hook idThread only.
// If both are zero, will hook everything
//


//
// EVENT DEFINITION
//
#define EVENT_MIN           0x00000001
#define EVENT_MAX           0x7FFFFFFF


//
//  EVENT_SYSTEM_SOUND
//  Sent when a sound is played.  Currently nothing is generating this, we
//  are going to be cleaning up the SOUNDSENTRY feature in the control panel
//  and will use this at that time.  Applications implementing WinEvents
//  are perfectly welcome to use it.  Clients of IAccessible* will simply
//  turn around and get back a non-visual object that describes the sound.
//
#define EVENT_SYSTEM_SOUND              0x0001

//
// EVENT_SYSTEM_ALERT
// Sent when an alert needs to be given to the user.  MessageBoxes generate
// alerts for example.
//
#define EVENT_SYSTEM_ALERT              0x0002

//
// EVENT_SYSTEM_FOREGROUND
// Sent when the foreground (active) window changes, even if it is changing
// to another window in the same thread as the previous one.
//
#define EVENT_SYSTEM_FOREGROUND         0x0003

//
// EVENT_SYSTEM_MENUSTART
// EVENT_SYSTEM_MENUEND
// Sent when entering into and leaving from menu mode (system, app bar, and
// track popups).
//
#define EVENT_SYSTEM_MENUSTART          0x0004
#define EVENT_SYSTEM_MENUEND            0x0005

//
// EVENT_SYSTEM_MENUPOPUPSTART
// EVENT_SYSTEM_MENUPOPUPEND
// Sent when a menu popup comes up and just before it is taken down.  Note
// that for a call to TrackPopupMenu(), a client will see EVENT_SYSTEM_MENUSTART
// followed almost immediately by EVENT_SYSTEM_MENUPOPUPSTART for the popup
// being shown.
//
#define EVENT_SYSTEM_MENUPOPUPSTART     0x0006
#define EVENT_SYSTEM_MENUPOPUPEND       0x0007


//
// EVENT_SYSTEM_CAPTURESTART
// EVENT_SYSTEM_CAPTUREEND
// Sent when a window takes the capture and releases the capture.
//
#define EVENT_SYSTEM_CAPTURESTART       0x0008
#define EVENT_SYSTEM_CAPTUREEND         0x0009

//
// EVENT_SYSTEM_MOVESIZESTART
// EVENT_SYSTEM_MOVESIZEEND
// Sent when a window enters and leaves move-size dragging mode.
//
#define EVENT_SYSTEM_MOVESIZESTART      0x000A
#define EVENT_SYSTEM_MOVESIZEEND        0x000B

//
// EVENT_SYSTEM_CONTEXTHELPSTART
// EVENT_SYSTEM_CONTEXTHELPEND
// Sent when a window enters and leaves context sensitive help mode.
//
#define EVENT_SYSTEM_CONTEXTHELPSTART   0x000C
#define EVENT_SYSTEM_CONTEXTHELPEND     0x000D

//
// EVENT_SYSTEM_DRAGDROPSTART
// EVENT_SYSTEM_DRAGDROPEND
// Sent when a window enters and leaves drag drop mode.  Note that it is up
// to apps and OLE to generate this, since the system doesn't know.  Like
// EVENT_SYSTEM_SOUND, it will be a while before this is prevalent.
//
#define EVENT_SYSTEM_DRAGDROPSTART      0x000E
#define EVENT_SYSTEM_DRAGDROPEND        0x000F

//
// EVENT_SYSTEM_DIALOGSTART
// EVENT_SYSTEM_DIALOGEND
// Sent when a dialog comes up and just before it goes away.
//
#define EVENT_SYSTEM_DIALOGSTART        0x0010
#define EVENT_SYSTEM_DIALOGEND          0x0011

//
// EVENT_SYSTEM_SCROLLINGSTART
// EVENT_SYSTEM_SCROLLINGEND
// Sent when beginning and ending the tracking of a scrollbar in a window,
// and also for scrollbar controls.
//
#define EVENT_SYSTEM_SCROLLINGSTART     0x0012
#define EVENT_SYSTEM_SCROLLINGEND       0x0013

//
// EVENT_SYSTEM_SWITCHSTART
// EVENT_SYSTEM_SWITCHEND
// Sent when beginning and ending alt-tab mode with the switch window.
//
#define EVENT_SYSTEM_SWITCHSTART        0x0014
#define EVENT_SYSTEM_SWITCHEND          0x0015

//
// EVENT_SYSTEM_MINIMIZESTART
// EVENT_SYSTEM_MINIMIZEEND
// Sent when a window minimizes and just before it restores.
//
#define EVENT_SYSTEM_MINIMIZESTART      0x0016
#define EVENT_SYSTEM_MINIMIZEEND        0x0017



//
// Object events
//
// The system AND apps generate these.  The system generates these for 
// real windows.  Apps generate these for objects within their window which
// act like a separate control, e.g. an item in a list view.
//
// For all events, if you want detailed accessibility information, callers
// should
//      * Call AccessibleObjectFromWindow() with the hwnd, idObject parameters
//          of the event, and IID_IAccessible as the REFIID, to get back an 
//          IAccessible* to talk to
//      * Initialize and fill in a VARIANT as VT_I4 with lVal the idChild
//          parameter of the event.
//      * If idChild isn't zero, call get_accChild() in the container to see
//          if the child is an object in its own right.  If so, you will get
//          back an IDispatch* object for the child.  You should release the
//          parent, and call QueryInterface() on the child object to get its
//          IAccessible*.  Then you talk directly to the child.  Otherwise,
//          if get_accChild() returns you nothing, you should continue to
//          use the child VARIANT.  You will ask the container for the properties
//          of the child identified by the VARIANT.  In other words, the
//          child in this case is accessible but not a full-blown object.
//          Like a button on a titlebar which is 'small' and has no children.
//          

//
#define EVENT_OBJECT_CREATE                 0x8000  // hwnd + ID + idChild is created item
#define EVENT_OBJECT_DESTROY                0x8001  // hwnd + ID + idChild is destroyed item
#define EVENT_OBJECT_SHOW                   0x8002  // hwnd + ID + idChild is shown item
#define EVENT_OBJECT_HIDE                   0x8003  // hwnd + ID + idChild is hidden item
#define EVENT_OBJECT_REORDER                0x8004  // hwnd + ID + idChild is parent of zordering children
//
// NOTE:
// Minimize the number of notifications!  
//
// When you are hiding a parent object, obviously all child objects are no 
// longer visible on screen.  They still have the same "visible" status, 
// but are not truly visible.  Hence do not send HIDE notifications for the
// children also.  One implies all.  The same goes for SHOW.
//


#define EVENT_OBJECT_FOCUS                  0x8005  // hwnd + ID + idChild is focused item
#define EVENT_OBJECT_SELECTION              0x8006  // hwnd + ID + idChild is selected item (if only one), or idChild is OBJID_WINDOW if complex
#define EVENT_OBJECT_SELECTIONADD           0x8007  // hwnd + ID + idChild is item added
#define EVENT_OBJECT_SELECTIONREMOVE        0x8008  // hwnd + ID + idChild is item removed
#define EVENT_OBJECT_SELECTIONWITHIN        0x8009  // hwnd + ID + idChild is parent of changed selected items

//
// NOTES:
// There is only one "focused" child item in a parent.  This is the place
// keystrokes are going at a given moment.  Hence only send a notification 
// about where the NEW focus is going.  A NEW item getting the focus already 
// implies that the OLD item is losing it.
//
// SELECTION however can be multiple.  Hence the different SELECTION
// notifications.  Here's when to use each:
//
// (1) Send a SELECTION notification in the simple single selection
//     case (like the focus) when the item with the selection is
//     merely moving to a different item within a container.  hwnd + ID
//     is the container control, idChildItem is the new child with the
//     selection.
//
// (2) Send a SELECTIONADD notification when a new item has simply been added 
//     to the selection within a container.  This is appropriate when the
//     number of newly selected items is very small.  hwnd + ID is the
//     container control, idChildItem is the new child added to the selection.
//
// (3) Send a SELECTIONREMOVE notification when a new item has simply been
//     removed from the selection within a container.  This is appropriate
//     when the number of newly selected items is very small, just like
//     SELECTIONADD.  hwnd + ID is the container control, idChildItem is the
//     new child removed from the selection.
//
// (4) Send a SELECTIONWITHIN notification when the selected items within a
//     control have changed substantially.  Rather than propagate a large
//     number of changes to reflect removal for some items, addition of
//     others, just tell somebody who cares that a lot happened.  It will
//     be faster an easier for somebody watching to just turn around and
//     query the container control what the new bunch of selected items
//     are.
//

#define EVENT_OBJECT_STATECHANGE            0x800A  // hwnd + ID + idChild is item w/ state change
#define EVENT_OBJECT_LOCATIONCHANGE         0x800B  // hwnd + ID + idChild is moved/sized item


#define EVENT_OBJECT_NAMECHANGE             0x800C  // hwnd + ID + idChild is item w/ name change
#define EVENT_OBJECT_DESCRIPTIONCHANGE      0x800D  // hwnd + ID + idChild is item w/ desc change
#define EVENT_OBJECT_VALUECHANGE            0x800E  // hwnd + ID + idChild is item w/ value change
#define EVENT_OBJECT_PARENTCHANGE           0x800F  // hwnd + ID + idChild is item w/ new parent
#define EVENT_OBJECT_HELPCHANGE             0x8010  // hwnd + ID + idChild is item w/ help change
#define EVENT_OBJECT_DEFACTIONCHANGE        0x8011  // hwnd + ID + idChild is item w/ def action change
#define EVENT_OBJECT_ACCELERATORCHANGE      0x8012  // hwnd + ID + idChild is item w/ keybd accel change

#endif // WINVER < 0x0500

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // !_WINABLE_
