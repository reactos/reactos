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
    HWND    hwndCaret;
    RECT    rcCaret;
} GUITHREADINFO, FAR * LPGUITHREADINFO;

#define GUI_CARETBLINKING   0x00000001
#define GUI_INMOVESIZE      0x00000002
#define GUI_INMENU          0x00000004


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
// to get an interface pointer to the container.  indexElement is the item
// within the container in question.  Setup a VARIANT with vt VT_I4 and 
// lVal the indexElement and pass that in to all methods.  Then you 
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
// System Sounds (indexElement of system SOUND notification)
//
#define SOUND_SYSTEM_STARTUP            1
#define SOUND_SYSTEM_SHUTDOWN           2
#define SOUND_SYSTEM_BEEP               3
#define SOUND_SYSTEM_ERROR              4
#define SOUND_SYSTEM_QUESTION           5
#define SOUND_SYSTEM_WARNING            6
#define SOUND_SYSTEM_INFORMATION        7
#define SOUND_SYSTEM_MAXIMIZE           8
#define SOUND_SYSTEM_MINIMIZE           9
#define SOUND_SYSTEM_RESTOREUP          10
#define SOUND_SYSTEM_RESTOREDOWN        11
#define SOUND_SYSTEM_APPSTART           12
#define SOUND_SYSTEM_FAULT              13
#define SOUND_SYSTEM_APPEND             14
#define SOUND_SYSTEM_MENUCOMMAND        15
#define SOUND_SYSTEM_MENUPOPUP          16
#define CSOUND_SYSTEM                   16

//
// System Alerts (indexElement of system ALERT notification)
//
#define ALERT_SYSTEM_INFORMATIONAL      1       // MB_INFORMATION
#define ALERT_SYSTEM_WARNING            2       // MB_WARNING
#define ALERT_SYSTEM_ERROR              3       // MB_ERROR
#define ALERT_SYSTEM_QUERY              4       // MB_QUESTION
#define ALERT_SYSTEM_CRITICAL           5       // HardSysErrBox
#define CALERT_SYSTEM                   6



typedef DWORD   HEVENT;

typedef VOID (CALLBACK* WINEVENTPROC)(
    HEVENT  hEvent,
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
HEVENT
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
    HEVENT          hEvent);

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
//      This event is sent when a sound is played.  The system generates
//  this event when a system sound (for menus, etc) is played.  Apps
//  generate this, if accessible, when a private sound is played.  For
//  example, if Mail plays a "New Mail" sound.
//
//  System Sounds:
//  (Generated by PlaySoundEvent in USER itself)
//      hwnd            is NULL
//      idObject        is OBJID_SOUND
//      idChild         is sound child ID if one
//  App Sounds:
//  (PlaySoundEvent won't generate notification; up to app)
//      hwnd + idObject gets interface pointer to Sound object
//      idChild identifies the sound in question
//
#define EVENT_SYSTEM_SOUND              0x0001

//
// EVENT_SYSTEM_ALERT
// System Alerts:
// (Generated by MessageBox() calls for example)
//      hwnd            is hwndMessageBox
//      idObject        is OBJID_ALERT
// App Alerts:
// (Generated whenever)
//      hwnd+idObject gets interface pointer to Alert
//
#define EVENT_SYSTEM_ALERT              0x0002

//
// EVENT_SYSTEM_FOREGROUND
// Sent when the foreground (active) window changes, even if it is changing
// to another window in the same thread as the previous one.
//      hwnd            is hwndNewForeground
//      idObject        is OBJID_WINDOW
//      idChild    is INDEXID_OBJECT
//
#define EVENT_SYSTEM_FOREGROUND         0x0003

//
// Menu
//      hwnd            is window (top level window or popup menu window)
//      idObject        is ID of control (OBJID_MENU, OBJID_SYSMENU, OBJID_SELF for popup)
//      idChild         is CHILDID_SELF
// 
// For MENU_START, hwnd+idObject+idChild refers to the control with the menu bar,
//  or the control bringing up the context menu.
//
// For MENUPOPUP, hwnd+idObject+idChild refers to the NEW popup coming up, not the 
// parent item which is hierarchical.  You can get the parent menu/popup by 
// asking for the accParent object.
//
#define EVENT_SYSTEM_MENUSTART          0x0004
#define EVENT_SYSTEM_MENUEND            0x0005
#define EVENT_SYSTEM_MENUPOPUPSTART     0x0006
#define EVENT_SYSTEM_MENUPOPUPEND       0x0007


//
// EVENT_SYSTEM_CAPTURE
//
#define EVENT_SYSTEM_CAPTURESTART       0x0008
#define EVENT_SYSTEM_CAPTUREEND         0x0009

//
// Move Size
//
#define EVENT_SYSTEM_MOVESIZESTART      0x000A
#define EVENT_SYSTEM_MOVESIZEEND        0x000B

//
// Context Help
//
#define EVENT_SYSTEM_CONTEXTHELPSTART   0x000C
#define EVENT_SYSTEM_CONTEXTHELPEND     0x000D

//
// Drag & Drop
// Send the START notification just before going into drag&drop loop.  Send
//  the END notification just after canceling out.
//
#define EVENT_SYSTEM_DRAGDROPSTART      0x000E
#define EVENT_SYSTEM_DRAGDROPEND        0x000F

//
// Dialog
// Send the START notification right after the dialog is completely
//  initialized and visible.  Send the END right before the dialog
//  is hidden and goes away.
//
#define EVENT_SYSTEM_DIALOGSTART        0x0010
#define EVENT_SYSTEM_DIALOGEND          0x0011

//
// EVENT_SYSTEM_SCROLLING
//
#define EVENT_SYSTEM_SCROLLINGSTART     0x0012
#define EVENT_SYSTEM_SCROLLINGEND       0x0013

//
// Alt-Tab Window
// Send the START notification right after the switch window is initialized
// and visible.  Send the END right before it is hidden and goes away.
//
#define EVENT_SYSTEM_SWITCHSTART        0x0014
#define EVENT_SYSTEM_SWITCHEND          0x0015




//
// Object events
//
// The system AND apps generate these.  The system generates these for 
// real windows.  Apps generate these for objects within their window which
// act like a separate control, e.g. an item in a list view.
//
// When the system generate them, dwParam2 is always WMOBJID_SELF.  When
// apps generate them, apps put the has-meaning-to-the-app-only ID value
// in dwParam2.
//

//
// For all EVENT_OBJECT events, 
//      hwnd is the dude to Send the WM_GETOBJECT message to (unless NULL,
//          see above for system things)
//      idObject is the ID of the object that can resolve any queries a
//          client might have.  It's a way to deal with windowless controls,
//          controls that are just drawn on the screen in some larger parent
//          window (like SDM), or standard frame elements of a window.
//      idChild is the piece inside of the object that is affected.  This
//          allows clients to access things that are too small to have full
//          blown objects in their own right.  Like the thumb of a scrollbar.
//          The hwnd/idObject pair gets you to the container, the dude you
//          probably want to talk to most of the time anyway.  The idChild
//          can then be passed into the acc properties to get the name/value 
//          of it as needed.
//           
// Example #1:
//      System propagating a listbox selection change
//      EVENT_OBJECT_SELECTION
//          hwnd == listbox hwnd
//          idObject == OBJID_WINDOW
//          idChild == new selected item, or CHILDID_SELF if
//              nothing now selected within container.
//      Word '97 propagating a listbox selection change
//          hwnd == SDM window
//          idObject == SDM ID to get at listbox 'control'
//          idChild == new selected item, or CHILDID_SELF if 
//              nothing
//
// Example #2:
//      System propagating a menu item selection on the menu bar
//      EVENT_OBJECT_SELECTION
//          hwnd == top level window
//          idObject == OBJID_MENU
//          idChild == ID of child menu bar item selected
//
// Example #3:
//      System propagating a dropdown coming off of said menu bar item
//      EVENT_OBJECT_CREATE
//          hwnd == popup item
//          idObject == OBJID_WINDOW
//          idChild == CHILDID_SELF
//
// Example #4:
//
// For EVENT_OBJECT_REORDER, the object referred to by hwnd/idObject is the
// PARENT container in which the zorder is occurring.  This is because if 
// one child is zordering, all of them are changing their relative zorder.
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
#define EVENT_OBJECT_SELECTIONWITHIN        0x8009  // hwnd + ID is container selection  changed with

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
//
// Examples of when to send an EVENT_OBJECT_STATECHANGE include
//      * It is being enabled/disabled (USER does for windows)
//      * It is being pressed/released (USER does for buttons)
//      * It is being checked/unchecked (USER does for radio/check buttons)

#define EVENT_OBJECT_LOCATIONCHANGE         0x800B  // hwnd + ID + idChild is moved/sized item

//
// Note:
// A LOCATIONCHANGE is not sent for every child object when the parent
// changes shape/moves.  Send one notification for the topmost object
// that is changing.  For example, if the user resizes a top level window,
// USER will generate a LOCATIONCHANGE for it, but not for the menu bar,
// title bar, scrollbars, etc.  that are also changing shape/moving.
// 
// In other words, it only generates LOCATIONCHANGE notifications for
// real windows that are moving/sizing.  It will not generate a LOCATIONCHANGE
// for every non-floating child window when the parent moves (the children are 
// logically moving also on screen, but not relative to the parent).
//
// Now, if the app itself resizes child windows as a result of being
// sized, USER will generate LOCATIONCHANGEs for those dudes also because 
// it doesn't know better.
//
// Note also that USER will generate LOCATIONCHANGE notifications for two
// non-window sys objects:
//      (1) System caret
//      (2) Cursor
//

#define EVENT_OBJECT_NAMECHANGE             0x800C  // hwnd + ID + idChild is item w/ name change
#define EVENT_OBJECT_DESCRIPTIONCHANGE      0x800D  // hwnd + ID + idChild is item w/ desc change
#define EVENT_OBJECT_VALUECHANGE            0x800E  // hwnd + ID + idChild is item w/ value change
#define EVENT_OBJECT_PARENTCHANGE           0x800F  // hwnd + ID + idChild is item w/ new parent
#define EVENT_OBJECT_HELPCHANGE             0x8010  // ""
#define EVENT_OBJECT_DEFACTIONCHANGE        0x8011  // ""
#define EVENT_OBJECT_ACCELERATORCHANGE      0x8012  // ""


#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // !_WINABLE_
