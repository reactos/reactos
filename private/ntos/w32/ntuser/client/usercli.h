/*++ BUILD Version: 0001    // Increment this if a change has global effects

/****************************** Module Header ******************************\
* Module Name: usercli.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Typedefs, defines, and prototypes that are used exclusively by the User
* client-side DLL.
*
* History:
* 04-27-91 DarrinM      Created from PROTO.H, MACRO.H and STRTABLE.H
\***************************************************************************/

#ifndef _USERCLI_
#define _USERCLI_

#define OEMRESOURCE 1

#pragma warning(4:4821)     // Disable all ptr64->ptr32 truncation warnings for now
#include <windows.h>
#include <winnls32.h>

#include <w32gdip.h>
#include <w32err.h>
#include <ddeml.h>
#include "ddemlp.h"
#include "winuserp.h"
#include "w32wow64.h"
#include "winuserk.h"
#include <winnlsp.h>
#include <dde.h>
#include <ddetrack.h>
#include "kbd.h"
#include <wowuserp.h>
#include "immstruc.h"
#include "immuser.h"
#include <winbasep.h>

#include "user.h"

#include "callproc.h"

/*
 * This prototype is needed in client\globals.h which is included unintentionally
 * from usersrv.h
 */
typedef LRESULT (APIENTRY *CFNSCSENDMESSAGE)(HWND, UINT, WPARAM, LPARAM,
        ULONG_PTR, DWORD, BOOL);

/***************************************************************************\
* Typedefs and Macros
*
* Here are defined all types and macros that are shared across the User's
* client-side code modules.  Types and macros that are unique to a single
* module should be defined at the head of that module, not in this file.
*
\***************************************************************************/

#ifdef USE_MIRRORING
#define MIRRORED_HDC(hdc)                 (GetLayout(hdc) & LAYOUT_RTL)
#endif

#if DBG

__inline void DebugUserGlobalUnlock(HANDLE h)
{
    UserAssert(
            "GlobalUnlock on bad handle" &&
            !(GlobalFlags(h) == GMEM_INVALID_HANDLE));

    GlobalUnlock((HANDLE) h);
}

/*
 * Bug 262144 - joejo
 *
 * Changed function to accept a pointer to the handle so we
 * can trash the handle and return it as trashed.
 *
 * Added a local handle variable to accept the return from GlobalFree
 * so we can return it as expected.
 *
 * Trash incoming handle freed so we can track any invalid access on
 * it after it's been free'd.
 */
__inline HANDLE DebugUserGlobalFree(HANDLE* ph)
{
    HANDLE th;

    UserAssert(
            "GlobalFree on bad handle" &&
            !(GlobalFlags(*ph) == GMEM_INVALID_HANDLE));

    th = GlobalFree(*ph);
#if defined(_WIN64)
    *ph = (HANDLE)(PVOID)0xBAADF00DBAADF00D;
#else
    *ph = (HANDLE)(PVOID)0xBAADF00D;
#endif
    return th;
}

__inline HANDLE DebugUserGlobalAlloc(UINT uFlags, SIZE_T dwBytes)
{
    HANDLE h = GlobalAlloc(uFlags, dwBytes);
    /*
     * Assert that FreeDDElParam and _ClientFreeDDEHandle assumption is correct.
     */
    if (h != NULL) {
        UserAssert(h > (HANDLE)0xFFFF);
    }

    return h;
}

#define USERGLOBALUNLOCK(h)             DebugUserGlobalUnlock((HANDLE)(h))
#define UserGlobalFree(h)               DebugUserGlobalFree((HANDLE*)(&h))
#define UserGlobalAlloc(flags, size)    DebugUserGlobalAlloc(flags, size)

#else

#define USERGLOBALUNLOCK(h)             GlobalUnlock((HANDLE)(h))
#define UserGlobalFree(h)               GlobalFree((HANDLE)(h))
#define UserGlobalAlloc(flags, size)    GlobalAlloc(flags, size)
#endif

#define USERGLOBALLOCK(h, p)   p = GlobalLock((HANDLE)(h))
#define UserGlobalReAlloc(pmem, cnt, flags) GlobalReAlloc(pmem,cnt,flags)
#define UserGlobalSize(pmem)                GlobalSize(pmem)
#define WOWGLOBALFREE(pmem)                 GlobalFree(pmem)

#define RESERVED_MSG_BITS   (0xFFFE0000)



/*
 * A macro for testing bits in the message bit-arrays.  Messages in the
 * the bit arrays must be processed
 */
#define FDEFWINDOWMSG(msg, procname) \
    ((msg <= (gSharedInfo.procname.maxMsgs)) && \
            ((gSharedInfo.procname.abMsgs)[msg / 8] & (1 << (msg & 7))))
#define FWINDOWMSG(msg, fnid) \
    ((msg <= (gSharedInfo.awmControl[fnid - FNID_START].maxMsgs)) && \
            ((gSharedInfo.awmControl[fnid - FNID_START].abMsgs)[msg / 8] & (1 << (msg & 7))))

#define CsSendMessage(hwnd, msg, wParam, lParam, xParam, pfn, bAnsi) \
        (((msg) >= WM_USER) ? \
            NtUserMessageCall(hwnd, msg, wParam, lParam, xParam, pfn, bAnsi) : \
            gapfnScSendMessage[MessageTable[msg].iFunction](hwnd, msg, wParam, lParam, xParam, pfn, bAnsi))

#define GetWindowProcess(hwnd) NtUserQueryWindow(hwnd, WindowProcess)
#define GETPROCESSID() (NtCurrentTeb()->ClientId.UniqueProcess)
#define GETTHREADID()  (NtCurrentTeb()->ClientId.UniqueThread)

/*
 * Macro to mask off uniqueness bits for WOW handles
 */
#define SAMEWOWHANDLE(h1, h2)  ((BOOL)!(((ULONG_PTR)(h1) ^ (ULONG_PTR)(h2)) & 0xffff))
#define DIFFWOWHANDLE(h1, h2)  (!SAMEWOWHANDLE(h1, h2))

/*
 * This macro can check to see if a function pointer is a server side
 * procedure.
 */
// #define ISSERVERSIDEPROC(p) (((DWORD)p) >= FNID_START && ((DWORD)p) <= FNID_END)

/*
 * For callbacks to the client - for msg and hook thunks, callback addresses
 * are passed as addresses, not function indexes as they are from client to
 * server.
 */
typedef int (WINAPI *GENERICPROC)();

#define CALLPROC(p) ((GENERICPROC)p)

/* Bug 234292 - joejo
 * Since the called window/dialog proc may have a different calling
 * convention, we must wrap the call and, check esp and replace with
 * a good esp when the call returns. This is what UserCallWinProc* does.
*/
#define CALLPROC_WOWCHECKPWW_DLG(pfn, hwnd, msg, wParam, lParam, pww)       \
    (IsWOWProc(pfn) ? (*pfnWowDlgProcEx)(hwnd, msg, wParam, lParam, PtrToUlong(pfn), pww) : \
        UserCallWinProc((WNDPROC)pfn, hwnd, msg, wParam, lParam))

#define CALLPROC_WOWCHECKPWW(pfn, hwnd, msg, wParam, lParam, pww)       \
    (IsWOWProc(pfn) ? (*pfnWowWndProcEx)(hwnd, msg, wParam, lParam, PtrToUlong(pfn), pww) : \
        UserCallWinProc((WNDPROC)pfn, hwnd, msg, wParam, lParam))

#define CALLPROC_WOWCHECK(pfn, hwnd, msg, wParam, lParam)       \
    CALLPROC_WOWCHECKPWW(pfn, hwnd, msg, wParam, lParam, NULL)

#define RevalidateHwnd(hwnd)    ((PWND)HMValidateHandleNoSecure(hwnd, TYPE_WINDOW))

#define VALIDATEHMENU(hmenu)        ((PMENU)HMValidateHandle(hmenu, TYPE_MENU))
#define VALIDATEHMONITOR(hmonitor)  ((PMONITOR)HMValidateSharedHandle(hmonitor, TYPE_MONITOR))


/*
 * REBASE macros take kernel desktop addresses and convert them into
 * user addresses.
 *
 * REBASEALWAYS converts a kernel address contained in an object
 * REBASEPWND casts REBASEALWAYS to a PWND
 * REBASE only converts if the address is in kernel space.  Also works for NULL
 * REBASEPTR converts a random kernel address
 */

#define REBASEALWAYS(p, elem) ((PVOID)((KERNEL_ULONG_PTR)(p) + ((KERNEL_ULONG_PTR)(p)->elem - (KERNEL_ULONG_PTR)(p)->head.pSelf)))
#define REBASEPTR(obj, p) ((PVOID)((KERNEL_ULONG_PTR)(p) - ((KERNEL_ULONG_PTR)(obj)->head.pSelf - (KERNEL_ULONG_PTR)(obj))))

#define REBASE(p, elem) ((KERNEL_ULONG_PTR)((p)->elem) <= (KERNEL_ULONG_PTR)gHighestUserAddress ? \
        ((PVOID)(p)->elem) : REBASEALWAYS(p, elem))
#define REBASEPWND(p, elem) ((PWND)REBASE(p, elem))

#ifndef USEREXTS

PTHREADINFO PtiCurrent(VOID);

/*
 * Window Proc Window Validation macro. This macro assumes
 * that pwnd and hwnd are existing variables pointing to the window.
 * Checking the BUTTON is for Mavis Beacon.
 */

#define VALIDATECLASSANDSIZE(pwnd, inFNID)                                      \
    switch ((pwnd)->fnid) {                                                     \
    case inFNID:                                                                \
        break;                                                                  \
                                                                                \
    case 0:                                                                     \
        if ((pwnd->cbwndExtra + sizeof(WND)) < (DWORD)(CBFNID(inFNID))) {       \
            RIPMSG3(RIP_ERROR,                                                  \
                   "(%#p %lX) needs at least (%ld) window words for this proc", \
                    pwnd, pwnd->cbwndExtra,                                     \
                    (DWORD)(CBFNID(inFNID)) - sizeof(WND));                     \
            return 0;                                                           \
        }                                                                       \
                                                                                \
        if (inFNID == FNID_BUTTON && *((PULONG_PTR)(pwnd + 1))) {               \
                                                                                \
            RIPMSG3(RIP_WARNING, "Window (%#p) fnid = %lX overrides "           \
                "the extra pointer with %#p\n",                                 \
                pwnd, inFNID, *((PULONG_PTR)(pwnd + 1)));                       \
                                                                                \
            NtUserSetWindowLongPtr(hwnd, 0, 0, FALSE);                          \
        }                                                                       \
                                                                                \
        NtUserSetWindowFNID(hwnd, inFNID);                                      \
        break;                                                                  \
                                                                                \
    case (inFNID | FNID_CLEANEDUP_BIT):                                         \
    case (inFNID | FNID_DELETED_BIT):                                           \
    case (inFNID | FNID_STATUS_BITS):                                           \
        return 0;                                                               \
                                                                                \
    default:                                                                    \
        RIPMSG3(RIP_WARNING,                                                    \
              "Window (%#p) not of correct class; fnid = %lX not %lX",          \
              (pwnd), (DWORD)((pwnd)->fnid), (DWORD)(inFNID));                  \
        return 0;                                                               \
    }

/*
 * This macro initializes the lookaside entry for a control.  It assumes
 * that pwnd and hwnd are existing variables pointing to the control's
 * windows and that fInit exists as a BOOL initialization flag.
 */
#define INITCONTROLLOOKASIDE(plaType, type, pwnditem, count)                \
    if (!*((PULONG_PTR)(pwnd + 1))) {                                       \
        P ## type pType;                                                    \
        if (fInit) {                                                        \
            if (!NT_SUCCESS(InitLookaside(plaType, sizeof(type), count))) { \
                NtUserSetWindowFNID(hwnd, FNID_CLEANEDUP_BIT);              \
                NtUserDestroyWindow(hwnd);                                  \
                return FALSE;                                               \
            }                                                               \
            fInit = FALSE;                                                  \
        }                                                                   \
        if ((pType = (P ## type)AllocLookasideEntry(plaType))) {            \
            NtUserSetWindowLongPtr(hwnd, 0, (LONG_PTR)pType, FALSE);        \
            Lock(&(pType->pwnditem), pwnd);                                 \
        } else {                                                            \
            NtUserSetWindowFNID(hwnd, FNID_CLEANEDUP_BIT);                  \
            NtUserDestroyWindow(hwnd);                                      \
            return FALSE;                                                   \
        }                                                                   \
    }

#endif

#define ISREMOTESESSION()   (NtCurrentPeb()->SessionId != 0)


/*
 * Bitmap related macroes.
 */
#define SetBestStretchMode(hdc, planes, bpp) \
    SetStretchBltMode(hdc, (((planes) * (bpp)) == 1 ? BLACKONWHITE : COLORONCOLOR))

#define BitmapSize(cx, cy, planes, bits) \
        (BitmapWidth(cx, bits) * (cy) * (planes))

#define BitmapWidth(cx, bpp)  (((((cx)*(bpp)) + 31) & ~31) >> 3)

#define RGBX(rgb)  RGB(GetBValue(rgb), GetGValue(rgb), GetRValue(rgb))

/*
 * Typedefs used for capturing string arguments to be passed
 * to the kernel.
 */
typedef struct _IN_STRING {
    UNICODE_STRING strCapture;
    PUNICODE_STRING pstr;
    BOOL fAllocated;
} IN_STRING, *PIN_STRING;

typedef struct _LARGE_IN_STRING {
    LARGE_UNICODE_STRING strCapture;
    PLARGE_UNICODE_STRING pstr;
    BOOL fAllocated;
} LARGE_IN_STRING, *PLARGE_IN_STRING;


/*
 * Lookaside definitions
 */
typedef struct _LOOKASIDE {
    PVOID LookasideBase;
    PVOID LookasideBounds;
    ZONE_HEADER LookasideZone;
    DWORD EntrySize;
#if DBG
    ULONG AllocHiWater;
    ULONG AllocCalls;
    ULONG AllocSlowCalls;
    ULONG DelCalls;
    ULONG DelSlowCalls;
#endif // DBG
} LOOKASIDE, *PLOOKASIDE;

NTSTATUS InitLookaside(PLOOKASIDE pla, DWORD cbEntry, DWORD cEntries);
PVOID AllocLookasideEntry(PLOOKASIDE pla);
void FreeLookasideEntry(PLOOKASIDE pla, PVOID pEntry);

/***************************************************************************\
*
* Thread and structure locking routines - we'll just define these to do
* nothing for now until we figure out what needs to be done
*
\***************************************************************************/

#undef ThreadLock
#undef ThreadLockAlways
#undef ThreadLockWithPti
#undef ThreadLockAlwaysWithPti
#undef ThreadUnlock
#undef Lock
#undef Unlock
#define CheckLock(pobj)
#define ThreadLock(pobj, ptl) DBG_UNREFERENCED_LOCAL_VARIABLE(*ptl)
#define ThreadLockAlways(pobj, ptl) DBG_UNREFERENCED_LOCAL_VARIABLE(*ptl)
#define ThreadLockWithPti(pti, pobj, ptl) DBG_UNREFERENCED_LOCAL_VARIABLE(*ptl)
#define ThreadLockAlwaysWithPti(pti, pobj, ptl) DBG_UNREFERENCED_LOCAL_VARIABLE(*ptl)
#define ThreadUnlock(ptl) (ptl)
#define Lock(ppobj, pobj) (*ppobj = pobj)
#define Unlock(ppobj) (*ppobj = NULL)

#if !defined(_USERRTL_) && !defined(USEREXTS)
typedef struct _TL {
    int iBogus;
} TL;
#endif

/***************************************************************************\
*
* Button Controls
*
\***************************************************************************/

/*
 *  Note: The button data structures are now found in user.h because the
 *        kernel needs to handle a special case of SetWindowWord on index
 *        0L to change the state of the button.
 */

#define BUTTONSTATE(pbutn)   (pbutn->buttonState)

#define BST_CHECKMASK       0x0003
#define BST_INCLICK         0x0010
#define BST_CAPTURED        0x0020
#define BST_MOUSE           0x0040
#define BST_DONTCLICK       0x0080
#define BST_INBMCLICK       0x0100

#define PBF_PUSHABLE     0x0001
#define PBF_DEFAULT      0x0002

/*
 * BNDrawText codes
 */
#define DBT_TEXT    0x0001
#define DBT_FOCUS   0x0002


/***************************************************************************\
*
* ComboBox
*
\***************************************************************************/

/*
 * ID numbers (hMenu) for the child controls in the combo box
 */
#define CBLISTBOXID 1000
#define CBEDITID    1001
#define CBBUTTONID  1002

/*
 * For CBOX.c. BoxType field, we define the following combo box styles. These
 * numbers are the same as the CBS_ style codes as defined in windows.h.
 */
#define SDROPPABLE      CBS_DROPDOWN
#define SEDITABLE       CBS_SIMPLE


#define SSIMPLE         SEDITABLE
#define SDROPDOWNLIST   SDROPPABLE
#define SDROPDOWN       (SDROPPABLE | SEDITABLE)


/*
 * CBOX.OwnerDraw & LBIV.OwnerDraw types
 */
#define OWNERDRAWFIXED 1
#define OWNERDRAWVAR   2

#define UPPERCASE   1
#define LOWERCASE   2

#define CaretCreate(plb)    ((plb)->fCaret = TRUE)

/*
 * Special styles for static controls, edit controls & listboxes so that we
 * can do combo box specific stuff in their wnd procs.
 */
#define LBS_COMBOBOX    0x8000L

/*
 * combo.h - Include file for combo boxes.
 */

/*
 * This macro is used to isolate the combo box style bits.  Ie if it the combo
 * box is simple, atomic, dropdown, or a dropdown listbox.
 */
#define COMBOBOXSTYLE(style)   ((LOBYTE(style)) & 3)

#define IsComboVisible(pcbox) (!pcbox->fNoRedraw && IsVisible(pcbox->spwnd))

/*
 * Note that I depend on the fact that these CBN_ defines are the same as
 * their listbox counterparts.  These defines are found in windows.h.
 * #define CBN_ERRSPACE  (-1)
 * #define CBN_SELCHANGE 1
 * #define CBN_DBLCLK    2
 */


/***************************************************************************\
*
* Edit Control Types/Macros
*
\***************************************************************************/

/* Window extra bytes - we need at least this much space for compatibility */
#define CBEDITEXTRA 6

/*
 * NOTE: Text handle is sized as multiple of this constant
 *       (should be power of 2).
 */
#define CCHALLOCEXTRA   0x20

/* Maximum width in pixels for a line/rectangle */

#define MAXPIXELWIDTH   30000

#define MAXCLIPENDPOS   32764

/* Limit multiline edit controls to at most 1024 characters on a single line.
 * We will force a wrap if the user exceeds this limit.
 */

#define MAXLINELENGTH   1024

/*
 * Allow an initial maximum of 30000 characters in all edit controls since
 * some apps will run into unsigned problems otherwise.  If apps know about
 * the 64K limit, they can set the limit themselves.
 */
#define MAXTEXT         30000

/*
 * Key modifiers which have been pressed.  Code in KeyDownHandler and
 * CharHandler depend on these exact values.
 */
#define NONEDOWN   0 /* Neither shift nor control down */
#define CTRLDOWN   1 /* Control key only down */
#define SHFTDOWN   2 /* Shift key only down */
#define SHCTDOWN   3 /* Shift and control keys down = CTRLDOWN + SHFTDOWN */
#define NOMODIFY   4 /* Neither shift nor control down */


#define CALLWORDBREAKPROC(proc, pText, iStart, cch, iAction)                \
    (IsWOWProc(proc) ?                                                      \
        (* pfnWowEditNextWord)(pText, iStart, cch, iAction, PtrToUlong(proc)) :  \
        (* proc)(pText, iStart, cch, iAction))

/*
 * Types of undo supported in this ped
 */
#define UNDO_NONE   0  /* We can't undo the last operation. */
#define UNDO_INSERT 1  /* We can undo the user's insertion of characters */
#define UNDO_DELETE 2  /* We can undo the user's deletion of characters */

typedef struct tagUNDO {
    UINT    undoType;          /* Current type of undo we support */
    PBYTE   hDeletedText;      /* Pointer to text which has been deleted (for
                                  undo) -- note, the memory is allocated as fixed
                                */
    ICH     ichDeleted;        /* Starting index from which text was deleted */
    ICH     cchDeleted;        /* Count of deleted characters in buffer */
    ICH     ichInsStart;       /* Starting index from which text was
                                  inserted */
    ICH     ichInsEnd;         /* Ending index of inserted text */
} UNDO, *PUNDO;

#define Pundo(ped)             ((PUNDO)&(ped)->undoType)

/*
 * Length of the buffer for ASCII character width caching: for characters
 * 0x00 to 0xff (field charWidthBuffer in PED structure below).
 * As the upper half of the cache was not used by almost anyone and fixing
 * it's usage required a lot of conversion, we decided to get rid of it
 * MCostea #174031
 */
#define CHAR_WIDTH_BUFFER_LENGTH 128

typedef struct tagED {
    HANDLE  hText;             /* Block of text we are editing */
    ICH     cchAlloc;          /* Number of chars we have allocated for hText
                                */
    ICH     cchTextMax;        /* Max number bytes allowed in edit control
                                */
    ICH     cch;               /* Current number of bytes of actual text
                                */
    ICH     cLines;            /* Number of lines of text */

    ICH     ichMinSel;         /* Selection extent.  MinSel is first selected
                                  char */
    ICH     ichMaxSel;         /* MaxSel is first unselected character */
    ICH     ichCaret;          /* Caret location. Caret is on left side of
                                  char */
    ICH     iCaretLine;        /* The line the caret is on. So that if word
                                * wrapping, we can tell if the caret is at end
                                * of a line of at beginning of next line...
                                */
    ICH     ichScreenStart;    /* Index of left most character displayed on
                                * screen for sl ec and index of top most line
                                * for multiline edit controls
                                */
    ICH     ichLinesOnScreen;  /* Number of lines we can display on screen */
    UINT    xOffset;           /* x (horizontal) scroll position in pixels
                                * (for multiline text horizontal scroll bar)
                                */
    UINT    charPasswordChar;  /* If non null, display this character instead
                                * of the real text. So that we can implement
                                * hidden text fields.
                                */
    int     cPasswordCharWidth;/* Width of password char */

    HWND    hwnd;              /* Window for this edit control */
    PWND    pwnd;              /* Pointer to window */
    RECT    rcFmt;             /* Client rectangle */
    HWND    hwndParent;        /* Parent of this edit control window */

                               /* These vars allow us to automatically scroll
                                * when the user holds the mouse at the bottom
                                * of the multiline edit control window.
                                */
    POINT   ptPrevMouse;       /* Previous point for the mouse for system
                                * timer.
                                */
    UINT    prevKeys;          /* Previous key state for the mouse */


    UINT     fSingle       : 1; /* Single line edit control? (or multiline) */
    UINT     fNoRedraw     : 1; /* Redraw in response to a change? */
    UINT     fMouseDown    : 1; /* Is mouse button down? when moving mouse */
    UINT     fFocus        : 1; /* Does ec have the focus ? */
    UINT     fDirty        : 1; /* Modify flag for the edit control */
    UINT     fDisabled     : 1; /* Window disabled? */
    UINT     fNonPropFont  : 1; /* Fixed width font? */
    UINT     fNonPropDBCS  : 1; /* Non-Propotional DBCS font */
    UINT     fBorder       : 1; /* Draw a border? */
    UINT     fAutoVScroll  : 1; /* Automatically scroll vertically */
    UINT     fAutoHScroll  : 1; /* Automatically scroll horizontally */
    UINT     fNoHideSel    : 1; /* Hide sel when we lose focus? */
    UINT     fDBCS         : 1; /* Are we using DBCS font set for editing? */
    UINT     fFmtLines     : 1; /* For multiline only. Do we insert CR CR LF at
                                * word wrap breaks?
                                */
    UINT     fWrap         : 1; /* Do int  wrapping? */
    UINT     fCalcLines    : 1; /* Recalc ped->chLines array? (recalc line
                                * breaks?)
                                */
    UINT     fEatNextChar  : 1; /* Hack for ALT-NUMPAD stuff with combo boxes.
                                * If numlock is up, we want to eat the next
                                * character generated by the keyboard driver
                                * if user enter num pad ascii value...
                                */
    UINT     fStripCRCRLF  : 1; /* CRCRLFs have been added to text. Strip them
                                * before doing any internal edit control
                                * stuff
                                */
    UINT     fInDialogBox  : 1; /* True if the ml edit control is in a dialog
                                * box and we have to specially treat TABS and
                                * ENTER
                                */
    UINT     fReadOnly     : 1; /* Is this a read only edit control? Only
                                * allow scrolling, selecting and copying.
                                */
    UINT     fCaretHidden  : 1; /* This indicates whether the caret is
                                * currently hidden because the width or height
                                * of the edit control is too small to show it.
                                */
    UINT     fTrueType     : 1; /* Is the current font TrueType? */
    UINT     fAnsi         : 1; /* is the edit control Ansi or unicode */
    UINT     fWin31Compat  : 1; /* TRUE if created by Windows 3.1 app */
    UINT     f40Compat     : 1; /* TRUE if created by Windows 4.0 app */
    UINT     fFlatBorder   : 1; /* Do we have to draw this baby ourself? */
    UINT     fSawRButtonDown : 1;
    UINT     fInitialized  : 1; /* If any more bits are needed, then   */
    UINT     fSwapRoOnUp   : 1; /* Swap reading order on next keyup    */
    UINT     fAllowRTL     : 1; /* Allow RTL processing                */
    UINT     fDisplayCtrl  : 1; /* Display unicode control characters  */
    UINT     fRtoLReading  : 1; /* Right to left reading order         */

    BOOL    fInsertCompChr  :1; /* means WM_IME_COMPOSITION:CS_INSERTCHAR will come */
    BOOL    fReplaceCompChr :1; /* means need to replace current composition str. */
    BOOL    fNoMoveCaret    :1; /* means stick to current caret pos. */
    BOOL    fResultProcess  :1; /* means now processing result. */
    BOOL    fKorea          :1; /* for Korea */
    BOOL    fInReconversion :1; /* In reconversion mode */
    BOOL    fLShift         :1; /* L-Shift pressed with Ctrl */

    WORD    wImeStatus;        /* current IME status */

    WORD    cbChar;            /* count of bytes in the char size (1 or 2 if unicode) */
    LPICH   chLines;           /* index of the start of each line */

    UINT    format;            /* Left, center, or right justify multiline
                                * text.
                                */
    EDITWORDBREAKPROCA lpfnNextWord;  /* use CALLWORDBREAKPROC macro to call */

                               /* Next word function */
    int     maxPixelWidth;     /* WASICH Width (in pixels) of longest line */

    UNDO;                      /* Undo buffer */

    HANDLE  hFont;             /* Handle to the font for this edit control.
                                  Null if system font.
                                */
    int     aveCharWidth;      /* Ave width of a character in the hFont */
    int     lineHeight;        /* Height of a line in the hFont */
    int     charOverhang;      /* Overhang associated with the hFont */
    int     cxSysCharWidth;    /* System font ave width */
    int     cySysCharHeight;   /* System font height */
    HWND    listboxHwnd;       /* ListBox hwnd. Non null if we are a combo
                                  box */
    LPINT   pTabStops;         /* Points to an array of tab stops; First
                                * element contains the number of elements in
                                * the array
                                */
    LPINT   charWidthBuffer;
    BYTE    charSet;           /* Character set for currently selected font
                                * needed for all versions
                                */
    UINT    wMaxNegA;          /* The biggest negative A width, */
    UINT    wMaxNegAcharPos;   /* and how many characters it can span accross */
    UINT    wMaxNegC;          /* The biggest negative C width, */
    UINT    wMaxNegCcharPos;   /* and how many characters it can span accross */
    UINT    wLeftMargin;       /* Left margin width in pixels. */
    UINT    wRightMargin;      /* Right margin width in pixels. */

    ICH     ichStartMinSel;
    ICH     ichStartMaxSel;

    PLPKEDITCALLOUT pLpkEditCallout;
    HBITMAP hCaretBitmap;      /* Current caret bitmap handle */
    INT     iCaretOffset;      /* Offset in pixels (for LPK use) */

    HANDLE  hInstance;         /* for WOW */
    UCHAR   seed;              /* used to encode and decode password text */
    BOOLEAN fEncoded;          /* is the text currently encoded */
    int     iLockLevel;        /* number of times the text has been locked */

    BYTE    DBCSVector[8];     /* DBCS vector table */
    HIMC    hImcPrev;          /* place to save hImc if we disable IME */
    POINT   ptScreenBounding;   /* top left corner of edit window in screen */
} ED, *PED, **PPED;

typedef struct tagEDITWND {
    WND wnd;
    PED ped;
} EDITWND, *PEDITWND;

#ifdef FAREAST_CHARSET_BITS
#error FAREAST_CHARSET_BITS should not be defined
#endif
#define FAREAST_CHARSET_BITS   (FS_JISJAPAN | FS_CHINESESIMP | FS_WANSUNG | FS_CHINESETRAD)


// Language pack specific context menu IDs

#define ID_CNTX_RTL         0x00008000L
#define ID_CNTX_DISPLAYCTRL 0x00008001L
#define ID_CNTX_INSERTCTRL  0x00008013L
#define ID_CNTX_ZWJ         0x00008002L
#define ID_CNTX_ZWNJ        0x00008003L
#define ID_CNTX_LRM         0x00008004L
#define ID_CNTX_RLM         0x00008005L
#define ID_CNTX_LRE         0x00008006L
#define ID_CNTX_RLE         0x00008007L
#define ID_CNTX_LRO         0x00008008L
#define ID_CNTX_RLO         0x00008009L
#define ID_CNTX_PDF         0x0000800AL
#define ID_CNTX_NADS        0x0000800BL
#define ID_CNTX_NODS        0x0000800CL
#define ID_CNTX_ASS         0x0000800DL
#define ID_CNTX_ISS         0x0000800EL
#define ID_CNTX_AAFS        0x0000800FL
#define ID_CNTX_IAFS        0x00008010L
#define ID_CNTX_RS          0x00008011L
#define ID_CNTX_US          0x00008012L

/*
 * The following structure is used to store a selection block; In Multiline
 * edit controls, "StPos" and "EndPos" fields contain the Starting and Ending
 * lines of the block. In Single line edit controls, "StPos" and "EndPos"
 * contain the Starting and Ending character positions of the block;
 */
typedef struct tagBLOCK {
    ICH StPos;
    ICH EndPos;
}  BLOCK, *LPBLOCK;

/*  The following structure is used to store complete information about a
 *  a strip of text.
 */
typedef  struct {
    LPSTR   lpString;
    ICH     ichString;
    ICH     nCount;
    int     XStartPos;
}  STRIPINFO;
typedef  STRIPINFO FAR *LPSTRIPINFO;


/***************************************************************************\
*
* ListBox
*
\***************************************************************************/

#define IsLBoxVisible(plb)  (plb->fRedraw && IsVisible(plb->spwnd))

/*
 * Number of list box items we allocated whenever we grow the list box
 * structures.
 */
#define CITEMSALLOC     32

/* Return Values */
#define EQ        0
#define PREFIX    1
#define LT        2
#define GT        3

#define         SINGLESEL       0
#define         MULTIPLESEL     1
#define         EXTENDEDSEL     2

#define LBI_ADD     0x0004

/*
 *  The various bits of wFileDetails field are used as mentioned below:
 *      0x0001    Should the file name be in upper case.
 *      0x0002    Should the file size be shown.
 *      0x0004    Date stamp of the file to be shown ?
 *      0x0008    Time stamp of the file to be shown ?
 *      0x0010    The dos attributes of the file ?
 *      0x0020    In DlgDirSelectEx(), along with file name
 *                all other details also will be returned
 *
 */

#define LBUP_RELEASECAPTURE 0x0001
#define LBUP_RESETSELECTION 0x0002
#define LBUP_NOTIFY         0x0004
#define LBUP_SUCCESS        0x0008
#define LBUP_SELCHANGE      0x0010

/*
 * rgpch is set up as follows:  First there are cMac 2 byte pointers to the
 * start of the strings in hStrings or if ownerdraw, it is 4 bytes of data
 * supplied by the app and hStrings is not used.  Then if multiselection
 * listboxes, there are cMac 1 byte selection state bytes (one for each item
 * in the list box).  If variable height owner draw, there will be cMac 1 byte
 * height bytes (once again, one for each item in the list box.).
 *
 * CHANGES DONE BY SANKAR:
 *      The selection byte in rgpch is divided into two nibbles. The lower
 * nibble is the selection state (1 => Selected; 0 => de-selected)
 * and higher nibble is the display state(1 => Hilited and 0 => de-hilited).
 * You must be wondering why on earth we should store this selection state and
 * the display state seperately.Well! The reason is as follows:
 *      While Ctrl+Dragging or Shift+Ctrl+Dragging, the user can adjust the
 * selection before the mouse button is up. If the user enlarges a range and
 * and before the button is up if he shrinks the range, then the old selection
 * state has to be preserved for the individual items that do not fall in the
 * range finally.
 *      Please note that the display state and the selection state for an item
 * will be the same except when the user is dragging his mouse. When the mouse
 * is dragged, only the display state is updated so that the range is hilited
 * or de-hilited) but the selection state is preserved. Only when the button
 * goes up, for all the individual items in the range, the selection state is
 * made the same as the display state.
 */

typedef struct tagLBItem {
    LONG offsz;
    ULONG_PTR itemData;
} LBItem, *lpLBItem;

typedef struct tagLBODItem {
    ULONG_PTR itemData;
} LBODItem, *lpLBODItem;

void LBEvent(PLBIV, UINT, int);

/***************************************************************************\
*
* Static Controls
*
\***************************************************************************/

typedef struct tagSTAT {
    PWND spwnd;
    union {
        HANDLE hFont;
        BOOL   fDeleteIt;
    };
    HANDLE hImage;
    UINT cicur;
    UINT iicur;
    UINT fPaintKbdCuesOnly : 1;
} STAT, *PSTAT;

typedef struct tagSTATWND {
    WND wnd;
    PSTAT pstat;
} STATWND, *PSTATWND;


typedef struct tagCURSORRESOURCE {
    WORD xHotspot;
    WORD yHotspot;
    BITMAPINFOHEADER bih;
} CURSORRESOURCE, *PCURSORRESOURCE;


#define NextWordBoundary(p)     ((PBYTE)(p) + ((ULONG_PTR)(p) & 1))
#define NextDWordBoundary(p)    ((PBYTE)(p) + ((ULONG_PTR)(-(LONG_PTR)(p)) & 3))

// DDEML stub prototypes

DWORD  Event(PEVENT_PACKET pep);
PVOID CsValidateInstance(HANDLE hInst);

/***************************************************************************\
* WOW Prototypes, Typedefs and Defines
*
* WOW registers resource callback functions so it can load 16 bit resources
* transparently for Win32.  At resource load time, these WOW functions are
* called.
*
\***************************************************************************/

BOOL  APIENTRY _FreeResource(HANDLE hResData, HINSTANCE hModule);
LPSTR APIENTRY _LockResource(HANDLE hResData, HINSTANCE hModule);
BOOL  APIENTRY _UnlockResource(HANDLE hResData, HINSTANCE hModule);

#define FINDRESOURCEA(hModule,lpName,lpType)         ((*(pfnFindResourceExA))(hModule, lpType, lpName, 0))
#define FINDRESOURCEW(hModule,lpName,lpType)         ((*(pfnFindResourceExW))(hModule, lpType, lpName, 0))
#define FINDRESOURCEEXA(hModule,lpName,lpType,wLang) ((*(pfnFindResourceExA))(hModule, lpType, lpName, wLang))
#define FINDRESOURCEEXW(hModule,lpName,lpType,wLang) ((*(pfnFindResourceExW))(hModule, lpType, lpName, wLang))
#define LOADRESOURCE(hModule,hResInfo)               ((*(pfnLoadResource))(hModule, hResInfo))
#define LOCKRESOURCE(hResData, hModule)              ((*(pfnLockResource))(hResData, hModule))
#define UNLOCKRESOURCE(hResData, hModule)            ((*(pfnUnlockResource))(hResData, hModule))
#define FREERESOURCE(hResData, hModule)              ((*(pfnFreeResource))(hResData, hModule))
#define SIZEOFRESOURCE(hModule,hResInfo)             ((*(pfnSizeofResource))(hModule, hResInfo))
#define GETEXPWINVER(hModule)                        ((*(pfnGetExpWinVer))((hModule)?(hModule):GetModuleHandle(NULL)))

/*
 * Pointers to unaligned-bits.  These are necessary for handling
 * bitmap-info's loaded from file.
 */
typedef BITMAPINFO       UNALIGNED *UPBITMAPINFO;
typedef BITMAPINFOHEADER UNALIGNED *UPBITMAPINFOHEADER;
typedef BITMAPCOREHEADER UNALIGNED *UPBITMAPCOREHEADER;

#define CCHFILEMAX      MAX_PATH

HANDLE LocalReallocSafe(HANDLE hMem, DWORD dwBytes, DWORD dwFlags, PPED pped);

HLOCAL WINAPI DispatchLocalAlloc(
    UINT uFlags,
    UINT uBytes,
    HANDLE hInstance);

HLOCAL WINAPI DispatchLocalReAlloc(
    HLOCAL hMem,
    UINT uBytes,
    UINT uFlags,
    HANDLE hInstance,
    PVOID* ppv);

LPVOID WINAPI DispatchLocalLock(
    HLOCAL hMem,
    HANDLE hInstance);

BOOL WINAPI DispatchLocalUnlock(
    HLOCAL hMem,
    HANDLE hInstance);

UINT WINAPI DispatchLocalSize(
    HLOCAL hMem,
    HANDLE hInstance);

HLOCAL WINAPI DispatchLocalFree(
    HLOCAL hMem,
    HANDLE hInstance);

#define UserLocalAlloc(uFlag,uBytes) HeapAlloc(pUserHeap, uFlag, (uBytes))
#define UserLocalReAlloc(p, uBytes, uFlags) HeapReAlloc(pUserHeap, uFlags, (LPSTR)(p), (uBytes))
#define UserLocalFree(p)    HeapFree(pUserHeap, 0, (LPSTR)(p))
#define UserLocalSize(p)    HeapSize(pUserHeap, 0, (LPSTR)(p))
#define UserLocalLock(p)    (LPSTR)(p)
#define UserLocalUnlock(p)
#define UserLocalFlags(p)   0
#define UserLocalHandle(p)  (HLOCAL)(p)

LONG TabTextOut(HDC hdc, int x, int y, LPCWSTR lpstring, int nCount,
        int nTabPositions, CONST INT *lpTabPositions, int iTabOrigin,
        BOOL fDrawTheText, int iCharset);
LONG UserLpkTabbedTextOut(HDC hdc, int x, int y, LPCWSTR lpstring,
        int nCount, int nTabPositions, CONST INT *lpTabPositions,
        int iTabOrigin, BOOL fDrawTheText, int cxCharWidth,
        int cyCharHeight, int iCharset);
void UserLpkPSMTextOut(HDC hdc, int xLeft, int yTop,
        LPWSTR lpsz, int cch, DWORD dwFlags);
void PSMTextOut(HDC hdc, int xLeft, int yTop, LPWSTR lpsz, int cch, DWORD dwFlags);
void ECUpdateFormat(PED ped, DWORD dwStyle, DWORD dwExStyle);

#ifndef _USERK_
int             LoadStringOrError(HANDLE, UINT, LPTSTR, int, WORD);
int             RtlGetIdFromDirectory(PBYTE, BOOL, int, int, DWORD, PDWORD);
BOOL            RtlCaptureAnsiString(PIN_STRING, LPCSTR, BOOL);
BOOL            RtlCaptureLargeAnsiString(PLARGE_IN_STRING, LPCSTR, BOOL);
#endif  // !_USERK_

PWND FASTCALL ValidateHwnd(HWND hwnd);
PWND FASTCALL ValidateHwndNoRip(HWND hwnd);

PSTR ECLock(PED ped);
void ECUnlock(PED ped);
BOOL ECNcCreate(PED, PWND, LPCREATESTRUCT);
void ECInvalidateClient(PED ped, BOOL fErase);
BOOL ECCreate(PED ped, LONG windowStyle);
void ECWord(PED, ICH, BOOL, ICH*, ICH*);
ICH  ECFindTab(LPSTR, ICH);
void ECNcDestroyHandler(PWND, PED);
BOOL ECSetText(PED, LPSTR);
void ECSetPasswordChar(PED, UINT);
ICH  ECCchInWidth(PED, HDC, LPSTR, ICH, int, BOOL);
void ECEmptyUndo(PUNDO);
void ECSaveUndo(PUNDO pundoFrom, PUNDO pundoTo, BOOL fClear);
BOOL ECInsertText(PED, LPSTR, ICH*);
ICH  ECDeleteText(PED);
void ECResetTextInfo(PED ped);
void ECNotifyParent(PED, int);
void ECSetEditClip(PED, HDC, BOOL);
HDC  ECGetEditDC(PED, BOOL);
void ECReleaseEditDC(PED, HDC, BOOL);
ICH  ECGetText(PED, ICH, LPSTR, BOOL);
void ECSetFont(PED, HFONT, BOOL);
void ECSetMargin(PED, UINT, long, BOOL);
ICH  ECCopy(PED);
BOOL ECCalcChangeSelection(PED, ICH, ICH, LPBLOCK, LPBLOCK);
void ECFindXORblks(LPBLOCK, LPBLOCK, LPBLOCK, LPBLOCK);
BOOL ECIsCharNumeric(PED ped, DWORD keyPress);

/*
 * Combine two DBCS WM_CHAR messages to
 * a single WORD value.
 */
WORD DbcsCombine(HWND, WORD);
#define CrackCombinedDbcsLB(c)  ((BYTE)(c))
#define CrackCombinedDbcsTB(c)  ((c) >> 8)

ICH  ECAdjustIch(PED, LPSTR, ICH);
ICH  ECAdjustIchNext(PED, LPSTR, ICH);
int  ECGetDBCSVector(PED, HDC, BYTE);
BOOL ECIsDBCSLeadByte(PED, BYTE);
LPSTR ECAnsiNext(PED, LPSTR);
LPSTR ECAnsiPrev(PED, LPSTR, LPSTR);
ICH  ECPrevIch(PED, LPSTR, ICH);
ICH  ECNextIch(PED, LPSTR, ICH);

void ECEnableDisableIME( PED ped );
void ECImmSetCompositionFont( PED ped );
void ECImmSetCompositionWindow( PED ped, LONG, LONG );
void  ECSetCaretHandler(PED ped);
void  ECInitInsert(PED ped, HKL hkl);
LRESULT ECImeComposition(PED ped, WPARAM wParam, LPARAM lParam);
LRESULT EcImeRequestHandler(PED, WPARAM, LPARAM);  // NT 5.0
BOOL HanjaKeyHandler(PED ped);  // Korean Support

void ECInOutReconversionMode(PED ped, BOOL fIn);


// ECTabTheTextOut draw codes
#define ECT_CALC        0
#define ECT_NORMAL      1
#define ECT_SELECTED    2

#define ECGetCaretWidth() (gpsi->uCaretWidth)

UINT ECTabTheTextOut(HDC, int, int, int, int,
                     LPSTR, int, ICH, PED, int, BOOL, LPSTRIPINFO);
HBRUSH ECGetControlBrush(PED, HDC, LONG);
HBRUSH ECGetBrush(PED ped, HDC hdc);
int  ECGetModKeys(int);
void ECSize( PED, LPRECT, BOOL);

ICH  MLInsertText(PED, LPSTR, ICH, BOOL);
ICH  MLDeleteText(PED);
BOOL MLEnsureCaretVisible(PED);
void MLDrawText(PED, HDC, ICH, ICH, BOOL);
void MLDrawLine(PED, HDC, int, ICH, int, BOOL);
void MLPaintABlock(PED, HDC, int, int);
int  GetBlkEndLine(int, int, BOOL FAR *, int, int);
void MLBuildchLines(PED, ICH, int, BOOL, PLONG, PLONG);
void MLShiftchLines(PED, ICH, int);
BOOL MLInsertchLine(PED, ICH, ICH, BOOL);
void MLSetCaretPosition(PED,HDC);
void MLIchToXYPos(PED, HDC, ICH, BOOL, LPPOINT);
int  MLIchToLine(PED, ICH);
void MLRepaintChangedSelection(PED, HDC, ICH, ICH);
void MLMouseMotion(PED, UINT, UINT, LPPOINT);
ICH  MLLine(PED, ICH);
void MLStripCrCrLf(PED);
int  MLCalcXOffset(PED, HDC, int);
BOOL MLUndo(PED);
LRESULT MLEditWndProc(HWND, PED, UINT, WPARAM, LPARAM);
void MLChar(PED, DWORD, int);
void MLKeyDown(PED, UINT, int);
ICH  MLPasteText(PED);
void MLSetSelection(PED, BOOL, ICH, ICH);
LONG MLCreate(PED, LPCREATESTRUCT);
BOOL MLInsertCrCrLf(PED);
void MLSetHandle(PED, HANDLE);
LONG MLGetLine(PED, ICH, ICH, LPSTR);
ICH  MLLineIndex(PED, ICH);
void MLSize(PED, BOOL);
void MLChangeSelection(PED, HDC, ICH, ICH);
void MLSetRectHandler(PED, LPRECT);
BOOL MLExpandTabs(PED);
BOOL MLSetTabStops(PED, int, LPINT);
LONG MLScroll(PED, BOOL, int, int, BOOL);
int  MLThumbPosFromPed(PED, BOOL);
void MLUpdateiCaretLine(PED ped);
ICH  MLLineLength(PED, ICH);
void MLReplaceSel(PED, LPSTR);

void SLReplaceSel(PED, LPSTR);
BOOL SLUndo(PED);
void SLSetCaretPosition(PED, HDC);
int  SLIchToLeftXPos(PED, HDC, ICH);
void SLChangeSelection(PED, HDC, ICH, ICH);
void SLDrawText(PED, HDC, ICH);
void SLDrawLine(PED, HDC, int, int, ICH, int, BOOL);
int  SLGetBlkEnd(PED, ICH, ICH, BOOL FAR *);
BOOL SLScrollText(PED, HDC);
void SLSetSelection(PED,ICH, ICH);
ICH  SLInsertText(PED, LPSTR, ICH);
ICH  SLPasteText(PED);
void SLChar(PED, DWORD);
void SLKeyDown(PED, DWORD, int);
ICH  SLMouseToIch(PED, HDC, LPPOINT);
void SLMouseMotion(PED, UINT, UINT, LPPOINT);
LONG SLCreate(PED, LPCREATESTRUCT);
void SLPaint(PED, HDC);
void SLSetFocus(PED);
void SLKillFocus(PED, HWND);
LRESULT SLEditWndProc(HWND, PED, UINT, WPARAM, LPARAM);
LRESULT EditWndProc(PWND, UINT, WPARAM, LPARAM);

#define GETAPPVER() GetClientInfo()->dwExpWinVer
#define THREAD_HKL()      (GetClientInfo()->hKL)


UINT HelpMenu(HWND hwnd, PPOINT ppt);

#define ISDELIMETERA(ch) ((ch == ' ') || (ch == '\t'))
#define ISDELIMETERW(ch) ((ch == L' ') || (ch == L'\t'))

#define AWCOMPARECHAR(ped,pbyte,awchar) (ped->fAnsi ? (*(PUCHAR)(pbyte) == (UCHAR)(awchar)) : (*(LPWSTR)(pbyte) == (WCHAR)(awchar)))

/* Menu that comes up when you press the right mouse button on an edit
 * control
 */
#define ID_EC_PROPERTY_MENU      1

#define IDD_MDI_ACTIVATE         9

#ifndef _USERK_
/*
 * String IDs
 */
#define STR_ERROR                        0x00000002L
#define STR_MOREWINDOWS                  0x0000000DL
#define STR_NOMEMBITMAP                  0x0000000EL

/*
 * IME specific context menu string
 */
#define STR_IMEOPEN                 700
#define STR_IMECLOSE                701
#define STR_SOFTKBDOPEN             702
#define STR_SOFTKBDCLOSE            703
#define STR_RECONVERTSTRING         705

#endif  // !_USERK_


BOOL InitClientDrawing();

/***************************************************************************\
* Function Prototypes
*
* NOTE: Only prototypes for GLOBAL (across module) functions should be put
* here.  Prototypes for functions that are global to a single module should
* be put at the head of that module.
*
\***************************************************************************/

int InternalScrollWindowEx(HWND hwnd, int dx, int dy, CONST RECT *prcScroll,
        CONST RECT *prcClip, HRGN hrgnUpdate, LPRECT prcUpdate,
        UINT dwFlags, DWORD dwTime);

BOOL IsMetaFile(HDC hdc);

BOOL DrawDiagonal(HDC hdc, LPRECT lprc, HBRUSH hbrTL, HBRUSH hbrBR, UINT flags);
BOOL FillTriangle(HDC hdc, LPRECT lprc, HBRUSH hbr, UINT flags);

BOOL   _ClientFreeLibrary(HANDLE hmod);
DWORD  _ClientGetListboxString(PWND pwnd, UINT msg, WPARAM wParam, LPSTR lParam,
        ULONG_PTR xParam, PROC xpfn);
LPHLP  HFill(LPCSTR lpszHelp, DWORD ulCommand, ULONG_PTR ulData);
BOOL SetVideoTimeout(DWORD dwVideoTimeout);

DWORD _GetWindowLong(PWND pwnd, int index, BOOL bAnsi);
#ifdef _WIN64
ULONG_PTR _GetWindowLongPtr(PWND pwnd, int index, BOOL bAnsi);
#else
#define _GetWindowLongPtr   _GetWindowLong
#endif
WORD  _GetWindowWord(PWND pwnd, int index);

HWND InternalFindWindowExA(HWND hwndParent, HWND hwndChild, LPCSTR pClassName,
                          LPCSTR pWindowName, DWORD   dwFlag);
HWND InternalFindWindowExW(HWND hwndParent, HWND hwndChild, LPCTSTR pClassName,
                          LPCTSTR pWindowName, DWORD   dwFlag);


/*
 * Message thunks.
 */
#define fnCOPYDATA                      NtUserMessageCall
#define fnDDEINIT                       NtUserMessageCall
#define fnDWORD                         NtUserMessageCall
#define fnDWORDOPTINLPMSG               NtUserMessageCall
#define fnGETTEXTLENGTHS                NtUserMessageCall
#define fnGETDBCSTEXTLENGTHS            NtUserMessageCall
#define fnINLPCREATESTRUCT              NtUserMessageCall
#define fnINLPCOMPAREITEMSTRUCT         NtUserMessageCall
#define fnINLPDELETEITEMSTRUCT          NtUserMessageCall
#define fnINLPDRAWITEMSTRUCT            NtUserMessageCall
#define fnINLPHELPINFOSTRUCT            NtUserMessageCall
#define fnINLPHLPSTRUCT                 NtUserMessageCall
#define fnINLPWINDOWPOS                 NtUserMessageCall
#define fnINOUTDRAG                     NtUserMessageCall
#define fnINOUTLPMEASUREITEMSTRUCT      NtUserMessageCall
#define fnINOUTLPPOINT5                 NtUserMessageCall
#define fnINOUTLPRECT                   NtUserMessageCall
#define fnINOUTLPSCROLLINFO             NtUserMessageCall
#define fnINOUTLPWINDOWPOS              NtUserMessageCall
#define fnINOUTNCCALCSIZE               NtUserMessageCall
#define fnINOUTNEXTMENU                 NtUserMessageCall
#define fnINOUTSTYLECHANGE              NtUserMessageCall
#define fnOPTOUTLPDWORDOPTOUTLPDWORD    NtUserMessageCall
#define fnOUTLPRECT                     NtUserMessageCall
#define fnPOPTINLPUINT                  NtUserMessageCall
#define fnPOUTLPINT                     NtUserMessageCall
#define fnSENTDDEMSG                    NtUserMessageCall
#define fnOUTDWORDINDWORD               NtUserMessageCall
#define fnINOUTMENUGETOBJECT            NtUserMessageCall
#define fnINCBOXSTRING                  NtUserMessageCall
#define fnINCNTOUTSTRING                NtUserMessageCall
#define fnINCNTOUTSTRINGNULL            NtUserMessageCall
#define fnINLBOXSTRING                  NtUserMessageCall
#define fnINLPMDICREATESTRUCT           NtUserMessageCall
#define fnINSTRING                      NtUserMessageCall
#define fnINSTRINGNULL                  NtUserMessageCall
#define fnINWPARAMCHAR                  NtUserMessageCall
#define fnOUTCBOXSTRING                 NtUserMessageCall
#define fnOUTLBOXSTRING                 NtUserMessageCall
#define fnOUTSTRING                     NtUserMessageCall
#define fnKERNELONLY                    NtUserMessageCall

#define MESSAGEPROTO(func) \
LRESULT CALLBACK fn ## func(                               \
        HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, \
        ULONG_PTR xParam, DWORD xpfnWndProc, BOOL bAnsi)

MESSAGEPROTO(COPYGLOBALDATA);
MESSAGEPROTO(INDEVICECHANGE);
MESSAGEPROTO(INPAINTCLIPBRD);
MESSAGEPROTO(INSIZECLIPBRD);
MESSAGEPROTO(IMECONTROL);
MESSAGEPROTO(IMEREQUEST);
MESSAGEPROTO(INWPARAMDBCSCHAR);
MESSAGEPROTO(EMGETSEL);
MESSAGEPROTO(EMSETSEL);
MESSAGEPROTO(CBGETEDITSEL);


/*
 * clhook.c
 */
#define IsHooked(pci, fsHook) \
    ((fsHook & (pci->fsHooks | pci->pDeskInfo->fsHooks)) != 0)

LRESULT fnHkINLPCWPSTRUCTW(PWND pwnd, UINT message, WPARAM wParam,
        LPARAM lParam, ULONG_PTR xParam);
LRESULT fnHkINLPCWPSTRUCTA(PWND pwnd, UINT message, WPARAM wParam,
        LPARAM lParam, ULONG_PTR xParam);
LRESULT fnHkINLPCWPRETSTRUCTW(PWND pwnd, UINT message, WPARAM wParam,
        LPARAM lParam, ULONG_PTR xParam);
LRESULT fnHkINLPCWPRETSTRUCTA(PWND pwnd, UINT message, WPARAM wParam,
        LPARAM lParam, ULONG_PTR xParam);
LRESULT DispatchHookW(int dw, WPARAM wParam, LPARAM lParam, HOOKPROC pfn);
LRESULT DispatchHookA(int dw, WPARAM wParam, LPARAM lParam, HOOKPROC pfn);

/*
 * client.c
 */
LRESULT APIENTRY ButtonWndProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY ButtonWndProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY MenuWndProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY MenuWndProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY DesktopWndProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY DesktopWndProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY ScrollBarWndProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY ScrollBarWndProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY ListBoxWndProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY ListBoxWndProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY StaticWndProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY StaticWndProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY ComboBoxWndProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY ComboBoxWndProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY ComboListBoxWndProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY ComboListBoxWndProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY MDIClientWndProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY MDIClientWndProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR APIENTRY MB_DlgProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR APIENTRY MB_DlgProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR APIENTRY MDIActivateDlgProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR APIENTRY MDIActivateDlgProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY EditWndProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY EditWndProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY ImeWndProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY ImeWndProcW(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT SendMessageWorker(PWND pwnd, UINT message, WPARAM wParam, LPARAM lParam, BOOL fAnsi);
LRESULT SendMessageTimeoutWorker(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam,
            UINT fuFlags, UINT uTimeout, PULONG_PTR lpdwResult, BOOL fAnsi);

void ClientEmptyClipboard(void);
VOID GetActiveKeyboardName(LPWSTR lpszName);
HANDLE OpenKeyboardLayoutFile(LPWSTR lpszKLName, PUINT puFlags, PUINT poffTable, PUINT pKbdInputLocale);
VOID LoadPreloadKeyboardLayouts(void);
void SetWindowState(PWND pwnd, UINT flags);
void ClearWindowState(PWND pwnd, UINT flags);

HKL LoadKeyboardLayoutWorker(HKL hkl, LPCWSTR lpszKLName, UINT uFlags, BOOL fFailSafe);

/*
 * Worker routines called from both the window procs and
 * the callback thunks.
 */
LRESULT DispatchClientMessage(PWND pwnd, UINT message, WPARAM wParam,
        LPARAM lParam, ULONG_PTR pfn);
LRESULT DefWindowProcWorker(PWND pwnd, UINT message, WPARAM wParam,
        LPARAM lParam, DWORD fAnsi);
LRESULT ButtonWndProcWorker(PWND pwnd, UINT msg, WPARAM wParam,
        LPARAM lParam, DWORD fAnsi);
LRESULT ListBoxWndProcWorker(PWND pwnd, UINT msg, WPARAM wParam,
        LPARAM lParam, DWORD fAnsi);
LRESULT StaticWndProcWorker(PWND pwnd, UINT msg, WPARAM wParam,
        LPARAM lParam, DWORD fAnsi);
LRESULT ComboBoxWndProcWorker(PWND pwnd, UINT msg, WPARAM wParam,
        LPARAM lParam, DWORD fAnsi);
LRESULT ComboListBoxWndProcWorker(PWND pwnd, UINT msg, WPARAM wParam,
        LPARAM lParam, DWORD fAnsi);
LRESULT MDIClientWndProcWorker(PWND pwnd, UINT msg, WPARAM wParam,
        LPARAM lParam, DWORD fAnsi);
LRESULT EditWndProcWorker(PWND pwnd, UINT msg, WPARAM wParam,
        LPARAM lParam, DWORD fAnsi);
LRESULT DefDlgProcWorker(PWND pwnd, UINT msg, WPARAM wParam,
        LPARAM lParam, DWORD fAnsi);
LRESULT ImeWndProcWorker(PWND pwnd, UINT msg, WPARAM wParam,
        LPARAM lParam, DWORD fAnsi);

/*
 * Server Stubs - ntstubs.c
 */

LONG _SetWindowLong(
    HWND hWnd,
    int nIndex,
    LONG dwNewLong,
    BOOL bAnsi);

#ifdef _WIN64
LONG_PTR _SetWindowLongPtr(
    HWND hWnd,
    int nIndex,
    LONG_PTR dwNewLong,
    BOOL bAnsi);
#else
#define _SetWindowLongPtr   _SetWindowLong
#endif

BOOL _PeekMessage(
    LPMSG pmsg,
    HWND hwnd,
    UINT wMsgFilterMin,
    UINT wMsgFilterMax,
    UINT wRemoveMsg,
    BOOL bAnsi);

BOOL _DefSetText(
    HWND hwnd,
    LPCWSTR pstr,
    BOOL bAnsi);

HCURSOR _GetCursorFrameInfo(
    HCURSOR hcur,
    LPWSTR id,
    int iFrame,
    LPDWORD pjifRate,
    LPINT pccur);

HWND _CreateWindowEx(
    DWORD dwExStyle,
    LPCTSTR pClassName,
    LPCTSTR pWindowName,
    DWORD dwStyle,
    int x,
    int y,
    int nWidth,
    int nHeight,
    HWND hwndParent,
    HMENU hmenu,
    HANDLE hModule,
    LPVOID pParam,
    DWORD dwFlags);

HKL _LoadKeyboardLayoutEx(
    HANDLE hFile,
    UINT offTable,
    HKL hkl,
    LPCWSTR pwszKL,
    UINT KbdInputLocale,
    UINT Flags);

BOOL _SetCursorIconData(
    HCURSOR hCursor,
    PCURSORDATA pcur);

HCURSOR FindExistingCursorIcon(
    LPWSTR      pszModName,
    LPCWSTR     pszResName,
    PCURSORFIND pcfSearch);

HANDLE CreateLocalMemHandle(
    HANDLE hMem);

HANDLE ConvertMemHandle(
    HANDLE hMem,
    UINT cbNULL);

HHOOK _SetWindowsHookEx(
    HANDLE hmod,
    LPTSTR pszLib,
    DWORD idThread,
    int nFilterType,
    PROC pfnFilterProc,
    DWORD dwFlags);

#if 0
DWORD WINAPI ImmGetReconvertTotalSize(
    DWORD dwSize,
    REQ_CALLER eCaller,
    BOOL bAnsiTarget);

DWORD WINAPI ImmReconversionWorker(
    LPRECONVERTSTRING lpRecTo,
    LPRECONVERTSTRING lpRecFrom,
    BOOL bToAnsi,
    DWORD dwCodePage);
#endif

/*
 * classc.c
 */
ULONG_PTR _GetClassData(
    PCLS pcls,
    PWND pwnd,
    int index,
    BOOL bAnsi);

DWORD _GetClassLong(
    PWND pwnd,
    int index,
    BOOL bAnsi);

#ifdef _WIN64
ULONG_PTR _GetClassLongPtr(
    PWND pwnd,
    int index,
    BOOL bAnsi);
#else
#define _GetClassLongPtr    _GetClassLong
#endif

/*
 * mngrayc.c
 */
BOOL BitBltSysBmp(
    HDC hdc,
    int x,
    int y,
    UINT i);


/*
 * clenum.c
 */
DWORD BuildHwndList(
    HDESK hdesk,
    HWND hwndNext,
    BOOL fEnumChildren,
    DWORD idThread,
    HWND **phwndFirst);

/*
 * cltxt.h
 */
ATOM RegisterClassExWOWA(
    PWNDCLASSEXA lpWndClass,
    LPDWORD pdwWOWstuff,
    WORD fnid);

ATOM RegisterClassExWOWW(
    PWNDCLASSEXW lpWndClass,
    LPDWORD pdwWOWstuff,
    WORD fnid);

void CopyLogFontAtoW(
    PLOGFONTW pdest,
    PLOGFONTA psrc);

void CopyLogFontWtoA(
    PLOGFONTA pdest,
    PLOGFONTW psrc);

/*
 * dlgmgrc.c
 */
PWND _NextControl(
    PWND pwndDlg,
    PWND pwnd,
    UINT uFlags);

PWND _PrevControl(
    PWND pwndDlg,
    PWND pwnd,
    UINT uFlags);

PWND _GetNextDlgGroupItem(
    PWND pwndDlg,
    PWND pwnd,
    BOOL fPrev);

PWND _GetNextDlgTabItem(
    PWND pwndDlg,
    PWND pwnd,
    BOOL fPrev);

PWND _GetChildControl(
    PWND pwndDlg,
    PWND pwndLevel);

/*
 * winmgrc.c
 */
BOOL FChildVisible(
    HWND hwnd);

/*
 * draw.c
 */
BOOL PaintRect(
    HWND hwndBrush,
    HWND hwndPaint,
    HDC hdc,
    HBRUSH hbr,
    LPRECT lprc);

#define NtUserReleaseDC(hwnd,hdc)  NtUserCallOneParam((ULONG_PTR)(hdc), SFI__RELEASEDC)
#define NtUserArrangeIconicWindows(hwnd)  (UINT)NtUserCallHwndLock((hwnd), SFI_XXXARRANGEICONICWINDOWS)
#define NtUserBeginDeferWindowPos(nNumWindows) (HANDLE)NtUserCallOneParam((nNumWindows),SFI__BEGINDEFERWINDOWPOS)
#define NtUserCreateMenu()   (HMENU)NtUserCallNoParam(SFI__CREATEMENU)
#define NtUserDestroyCaret() (BOOL)NtUserCallNoParam(SFI_ZZZDESTROYCARET)
#define NtUserEnableWindow(hwnd, bEnable) (BOOL)NtUserCallHwndParamLock((hwnd), (bEnable),SFI_XXXENABLEWINDOW)
#define NtUserGetMessagePos() (DWORD)NtUserCallNoParam(SFI__GETMESSAGEPOS)
#define NtUserKillSystemTimer(hwnd,nIDEvent)  (BOOL)NtUserCallHwndParam((hwnd), (nIDEvent), SFI__KILLSYSTEMTIMER)
#define NtUserMessageBeep(wType)  (BOOL)NtUserCallOneParam((wType), SFI_XXXMESSAGEBEEP)
#define NtUserSetWindowContextHelpId(hwnd,id) (BOOL)NtUserCallHwndParam((hwnd), (id), SFI__SETWINDOWCONTEXTHELPID)
#define NtUserGetWindowContextHelpId(hwnd)   (BOOL)NtUserCallHwnd((hwnd), SFI__GETWINDOWCONTEXTHELPID)
#define NtUserRedrawFrame(hwnd)   NtUserCallHwndLock((hwnd), SFI_XXXREDRAWFRAME)
#define NtUserRedrawFrameAndHook(hwnd)  NtUserCallHwndLock((hwnd), SFI_XXXREDRAWFRAMEANDHOOK)
#define NtUserRedrawTitle(hwnd, wFlags)  NtUserCallHwndParamLock((hwnd), (wFlags), SFI_XXXREDRAWTITLE)
#define NtUserReleaseCapture()  (BOOL)NtUserCallNoParam(SFI_XXXRELEASECAPTURE)
#define NtUserSetCaretPos(X,Y)  (BOOL)NtUserCallTwoParam((DWORD)(X), (DWORD)(Y), SFI_ZZZSETCARETPOS)
#define NtUserSetCursorPos(X, Y)  (BOOL)NtUserCallTwoParam((X), (Y), SFI_ZZZSETCURSORPOS)
#define NtUserSetForegroundWindow(hwnd)  (BOOL)NtUserCallHwndLock((hwnd), SFI_XXXSTUBSETFOREGROUNDWINDOW)
#define NtUserSetSysMenu(hwnd)  NtUserCallHwndLock((hwnd), SFI_XXXSETSYSMENU)
#define NtUserSetVisible(hwnd,fSet)  NtUserCallHwndParam((hwnd), (fSet), SFI_SETVISIBLE)
#define NtUserShowCursor(bShow)   (int)NtUserCallOneParam((bShow), SFI_ZZZSHOWCURSOR)
#define NtUserUpdateClientRect(hwnd) NtUserCallHwndLock((hwnd), SFI_XXXUPDATECLIENTRECT)

#define CreateCaret         NtUserCreateCaret
#define FillWindow          NtUserFillWindow
#define GetControlBrush     NtUserGetControlBrush
#define GetControlColor     NtUserGetControlColor
#define GetDCEx             NtUserGetDCEx
#define GetWindowPlacement  NtUserGetWindowPlacement
#define RedrawWindow        NtUserRedrawWindow


/*
 * dmmnem.c
 */
int FindMnemChar(
    LPWSTR lpstr,
    WCHAR ch,
    BOOL fFirst,
    BOOL fPrefix);

/*
 * clres.c
 */
BOOL WowGetModuleFileName(
    HMODULE hModule,
    LPWSTR pwsz,
    DWORD  cchMax);

HICON WowServerLoadCreateCursorIcon(
    HANDLE hmod,
    LPTSTR lpModName,
    DWORD dwExpWinVer,
    LPCTSTR lpName,
    DWORD cb,
    PVOID pcr,
    LPTSTR lpType,
    BOOL fClient);

HANDLE InternalCopyImage(
    HANDLE hImage,
    UINT IMAGE_flag,
    int cxNew,
    int cyNew,
    UINT LR_flags);

HMENU CreateMenuFromResource(
    LPBYTE);

/*
 * acons.c
 */
#define BFT_ICON    0x4349  //  'IC'
#define BFT_BITMAP  0x4D42  //  'BM'
#define BFT_CURSOR  0x5450  //  'PT'

typedef struct _FILEINFO {
    LPBYTE  pFileMap;
    LPBYTE  pFilePtr;
    LPBYTE  pFileEnd;
    LPCWSTR pszName;
} FILEINFO, *PFILEINFO;

HANDLE LoadCursorIconFromFileMap(
    IN PFILEINFO   pfi,
    IN OUT LPWSTR *prt,
    IN DWORD       cxDesired,
    IN DWORD       cyDesired,
    IN DWORD       LR_flags,
    OUT LPBOOL     pfAni);

DWORD GetIcoCurWidth(
    DWORD cxOrg,
    BOOL  fIcon,
    UINT  LR_flags,
    DWORD cxDesired);

DWORD GetIcoCurHeight(
    DWORD cyOrg,
    BOOL  fIcon,
    UINT  LR_flags,
    DWORD cyDesired);

DWORD GetIcoCurBpp(
    UINT LR_flags);

HICON LoadIcoCur(
    HINSTANCE hmod,
    LPCWSTR   lpName,
    LPWSTR    type,
    DWORD     cxDesired,
    DWORD     cyDesired,
    UINT      LR_flags);

HANDLE ObjectFromDIBResource(
    HINSTANCE hmod,
    LPCWSTR   lpName,
    LPWSTR    type,
    DWORD     cxDesired,
    DWORD     cyDesired,
    UINT      LR_flags);

HANDLE RtlLoadObjectFromDIBFile(
    LPCWSTR lpszName,
    LPWSTR  type,
    DWORD   cxDesired,
    DWORD   cyDesired,
    UINT    LR_flags);

HCURSOR LoadCursorOrIconFromFile(
    LPCWSTR pszFilename,
    BOOL    fIcon);

HBITMAP ConvertDIBBitmap(
    UPBITMAPINFOHEADER lpbih,
    DWORD              cxDesired,
    DWORD              cyDesired,
    UINT               flags,
    LPBITMAPINFOHEADER *lplpbih,
    LPSTR              *lplpBits);

HICON ConvertDIBIcon(
    LPBITMAPINFOHEADER lpbih,
    HINSTANCE          hmod,
    LPCWSTR            lpName,
    BOOL               fIcon,
    DWORD              cxNew,
    DWORD              cyNew,
    UINT               LR_flags);

int SmartStretchDIBits(
    HDC          hdc,
    int          xD,
    int          yD,
    int          dxD,
    int          dyD,
    int          xS,
    int          yS,
    int          dxS,
    int          dyS,
    LPVOID       lpBits,
    LPBITMAPINFO lpbi,
    UINT         wUsage,
    DWORD        rop);


/*
 * OFFSET for different DPI resources.
 * This allows us to take a resource number and "map" to an actual resource
 * based on what DPI the user selected
 */

#define OFFSET_SCALE_DPI 000
#define OFFSET_96_DPI    100
#define OFFSET_120_DPI   200
#define OFFSET_160_DPI   300

/*
 * defines the highest resource number so we can do math on the resource
 * number.
 */

#define MAX_RESOURCE_INDEX 32768


/*
 * Parameter for xxxAlterHilite()
 */
#define HILITEONLY      0x0001
#define SELONLY         0x0002
#define HILITEANDSEL    (HILITEONLY + SELONLY)

#define HILITE     1

// LATER IanJa: these vary by country!  For US they are VK_OEM_2 VK_OEM_5.
//       Change lboxctl2.c MapVirtualKey to character - and fix the spelling?
#define VERKEY_SLASH     0xBF   /* Vertual key for '/' character */
#define VERKEY_BACKSLASH 0xDC   /* Vertual key for '\' character */

/*
 * Procedures for combo boxes.
 */
LONG  xxxCBCommandHandler(PCBOX, DWORD, HWND);
LRESULT xxxCBMessageItemHandler(PCBOX, UINT, LPVOID);
int   xxxCBDir(PCBOX, UINT, LPWSTR);
VOID  xxxCBPaint(PCBOX, HDC);
VOID  xxxCBCompleteEditWindow(PCBOX pcbox);
BOOL  xxxCBHideListBoxWindow(PCBOX pcbox, BOOL fNotifyParent, BOOL fSelEndOK);
VOID  xxxCBShowListBoxWindow(PCBOX pcbox, BOOL fTrack);
void xxxCBPosition(PCBOX pcbox);

/*
 * combo.h
 */

/* Initialization code */
long  CBNcCreateHandler(PCBOX, PWND);
LRESULT xxxCBCreateHandler(PCBOX, PWND);
void xxxCBCalcControlRects(PCBOX pcbox, LPRECT lprcList);

/* Destruction code */
VOID  xxxCBNcDestroyHandler(PWND, PCBOX);

/* Generic often used routines */
VOID  xxxCBNotifyParent(PCBOX, SHORT);
VOID  xxxCBUpdateListBoxWindow(PCBOX, BOOL);


/* Helpers' */
VOID  xxxCBInternalUpdateEditWindow(PCBOX, HDC);
VOID  xxxCBGetFocusHelper(PCBOX);
VOID  xxxCBKillFocusHelper(PCBOX);
VOID  xxxCBInvertStaticWindow(PCBOX,BOOL,HDC);
VOID  xxxCBSetFontHandler(PCBOX, HANDLE, BOOL);
VOID  xxxCBSizeHandler(PCBOX);
LONG  xxxCBSetEditItemHeight(PCBOX pcbox, int editHeight);


/*
 * String
 */

INT xxxFindString(PLBIV, LPWSTR, INT, INT, BOOL);

VOID  InitHStrings(PLBIV);

int   xxxLBInsertItem(PLBIV, LPWSTR, int, UINT);

/*
 * Selection
 */
BOOL  ISelFromPt(PLBIV, POINT, LPDWORD);
BOOL  IsSelected(PLBIV, INT, UINT);
VOID LBSetCItemFullMax(PLBIV plb);

VOID  xxxLBSelRange(PLBIV, INT, INT, BOOL);

INT xxxLBSetCurSel(PLBIV, INT);

INT LBoxGetSelItems(PLBIV, BOOL, INT, LPINT);

LONG  xxxLBSetSel(PLBIV, BOOL, INT);

VOID  xxxSetISelBase(PLBIV, INT);

VOID  SetSelected(PLBIV, INT, BOOL, UINT);


/*
 * Caret
 */
void xxxLBSetCaret(PLBIV plb, BOOL fSetCaret);
VOID  xxxCaretDestroy(PLBIV);

/*
 * LBox
 */
LONG  xxxLBCreate(PLBIV, PWND, LPCREATESTRUCT);
VOID  xxxDestroyLBox(PLBIV, PWND);
VOID  xxxLBoxDeleteItem(PLBIV, INT);

VOID  xxxLBoxDoDeleteItems(PLBIV);
VOID  xxxLBoxDrawItem(PLBIV, INT, UINT, UINT, LPRECT);


/*
 * Scroll
 */
INT   LBCalcVarITopScrollAmt(PLBIV, INT, INT);

VOID  xxxLBoxCtlHScroll(PLBIV, INT, INT);

VOID  xxxLBoxCtlHScrollMultiColumn(PLBIV, INT, INT);

VOID  xxxLBoxCtlScroll(PLBIV, INT, INT);

VOID  xxxLBShowHideScrollBars(PLBIV);

/*
 * LBoxCtl
 */
INT xxxLBoxCtlDelete(PLBIV, INT);

VOID  xxxLBoxCtlCharInput(PLBIV, UINT, BOOL);
VOID  xxxLBoxCtlKeyInput(PLBIV, UINT, UINT);
VOID  xxxLBPaint(PLBIV, HDC, LPRECT);

BOOL xxxLBInvalidateRect(PLBIV plb, LPRECT lprc, BOOL fErase);
/*
 * Miscellaneous
 */
VOID  xxxAlterHilite(PLBIV, INT, INT, BOOL, INT, BOOL);

INT CItemInWindow(PLBIV, BOOL);

VOID  xxxCheckRedraw(PLBIV, BOOL, INT);

LPWSTR GetLpszItem(PLBIV, INT);

VOID  xxxInsureVisible(PLBIV, INT, BOOL);

VOID  xxxInvertLBItem(PLBIV, INT, BOOL);

VOID  xxxLBBlockHilite(PLBIV, INT, BOOL);

int   LBGetSetItemHeightHandler(PLBIV plb, UINT message, int item, UINT height);
VOID  LBDropObjectHandler(PLBIV, PDROPSTRUCT);
LONG_PTR LBGetItemData(PLBIV, INT);

INT LBGetText(PLBIV, BOOL, BOOL, INT, LPWSTR);

VOID  xxxLBSetFont(PLBIV, HANDLE, BOOL);
int LBSetItemData(PLBIV, INT, LONG_PTR);

BOOL  LBSetTabStops(PLBIV, INT, LPINT);

VOID  xxxLBSize(PLBIV, INT, INT);
INT LastFullVisible(PLBIV);

INT xxxLbDir(PLBIV, UINT, LPWSTR);

INT xxxLbInsertFile(PLBIV, LPWSTR);

VOID  xxxNewITop(PLBIV, INT);
VOID  xxxNewITopEx(PLBIV, INT, DWORD);

VOID  xxxNotifyOwner(PLBIV, INT);

VOID  xxxResetWorld(PLBIV, INT, INT, BOOL);

VOID  xxxTrackMouse(PLBIV, UINT, POINT);
BOOL  xxxDlgDirListHelper(PWND, LPWSTR, LPBYTE, int, int, UINT, BOOL);
BOOL  DlgDirSelectHelper(LPWSTR pFileName, int cbFileName, HWND hwndListBox);
BOOL xxxLBResetContent(PLBIV plb);
VOID xxxLBSetRedraw(PLBIV plb, BOOL fRedraw);
int xxxSetLBScrollParms(PLBIV plb, int nCtl);
void xxxLBButtonUp(PLBIV plb, UINT uFlags);

/*
 * Variable Height OwnerDraw Support Routines
 */
INT CItemInWindowVarOwnerDraw(PLBIV, BOOL);

INT LBPage(PLBIV, INT, BOOL);


/*
 * Multicolumn listbox
 */
VOID  LBCalcItemRowsAndColumns(PLBIV);

/*
 * Both multicol and var height
 */
BOOL  LBGetItemRect(PLBIV, INT, LPRECT);

VOID  LBSetVariableHeightItemHeight(PLBIV, INT, INT);

INT   LBGetVariableHeightItemHeight(PLBIV, INT);

/*
 * No-data (lazy evaluation) listbox
 */
INT  xxxLBSetCount(PLBIV, INT);

UINT LBCalcAllocNeeded(PLBIV, INT);

/*
 * Storage pre-allocation support for LB_INITSTORAGE
 */
LONG xxxLBInitStorage(PLBIV plb, BOOL fAnsi, INT cItems, INT cb);

/***************************************************************************\
*
* Dialog Boxes
*
\***************************************************************************/

HWND        InternalCreateDialog(HANDLE hmod,
             LPDLGTEMPLATE lpDlgTemplate, DWORD cb,
             HWND hwndOwner , DLGPROC pfnWndProc, LPARAM dwInitParam,
             UINT fFlags);

INT_PTR     InternalDialogBox(HANDLE hmod,
             LPDLGTEMPLATE lpDlgTemplate,
             HWND hwndOwner , DLGPROC pfnWndProc, LPARAM dwInitParam,
             UINT fFlags);

PWND        _FindDlgItem(PWND pwndParent, DWORD id);
PWND        _GetDlgItem(PWND, int);
long        _GetDialogBaseUnits(VOID);
PWND        GetParentDialog(PWND pwndDialog);
VOID        xxxRemoveDefaultButton(PWND pwndDlg, PWND pwndStart);
VOID        xxxCheckDefPushButton(PWND pwndDlg, HWND hwndOldFocus, HWND hwndNewFocus);
PWND        xxxGotoNextMnem(PWND pwndDlg, PWND pwndStart, WCHAR ch);
VOID        DlgSetFocus(HWND hwnd);
void        RepositionRect(PMONITOR pMonitor, LPRECT lprc, DWORD dwStyle, DWORD dwExStyle);
BOOL        ValidateDialogPwnd(PWND pwnd);
PMONITOR    GetDialogMonitor(HWND hwndOwner, DWORD dwFlags);

HANDLE      GetEditDS(VOID);
VOID        ReleaseEditDS(HANDLE h);
VOID        TellWOWThehDlg(HWND hDlg);

UINT        GetACPCharSet();

/***************************************************************************\
*
* Menus
*
\***************************************************************************/
// cltxt.h
BOOL GetMenuItemInfoInternalW(HMENU hMenu, UINT uID, BOOL fByPosition, LPMENUITEMINFOW lpmii);

#define MENUAPI_INSERT  0
#define MENUAPI_GET     1
#define MENUAPI_SET     2

// clmenu.c
BOOL InternalInsertMenuItem (HMENU hMenu, UINT uID, BOOL fByPosition, LPCMENUITEMINFO lpmii);
BOOL ValidateMENUITEMINFO(LPMENUITEMINFOW lpmiiIn, LPMENUITEMINFOW lpmii, DWORD dwAPICode);
BOOL ValidateMENUINFO(LPCMENUINFO lpmi, DWORD dwAPICode);


// ntstubs.c
BOOL ThunkedMenuItemInfo(HMENU hMenu, UINT  nPosition, BOOL fByPosition,
                            BOOL fInsert, LPMENUITEMINFOW lpmii, BOOL fAnsi);

// menuc.c
void SetMenuItemInfoStruct(HMENU hMenu, UINT wFlags, UINT_PTR wIDNew, LPWSTR pwszNew,
                              LPMENUITEMINFOW pmii);

/***************************************************************************\
*
* Message Boxes
*
\***************************************************************************/


#define WINDOWLIST_PROP_NAME    TEXT("SysBW")
#define MSGBOX_CALLBACK         TEXT("SysMB")

/* Unicode Right-To-Left mark unicode code point. Look in msgbox.c for more info */
#define UNICODE_RLM             0x200f

/***************************************************************************\
*
* MDI Windows
*
\***************************************************************************/

/* maximum number of MDI children windows listed in "Window" menu */
#define MAXITEMS         10

/*
 * MDI typedefs
 */
typedef struct tagSHORTCREATE {
    int         cy;
    int         cx;
    int         y;
    int         x;
    LONG        style;
    HMENU       hMenu;
} SHORTCREATE, *PSHORTCREATE;

typedef struct tagMDIACTIVATEPOS {
    int     cx;
    int     cy;
    int     cxMin;
    int     cyMin;
} MDIACTIVATEPOS, *PMDIACTIVATEPOS;

BOOL CreateMDIChild(PSHORTCREATE pcs, LPMDICREATESTRUCT pmcs, DWORD dwExpWinVerAndFlags, HMENU *phSysMenu, PWND pwndParent);
BOOL MDICompleteChildCreation(HWND hwndChild, HMENU hSysMenu, BOOL fVisible, BOOL fDisabled);

/*
 * MDI defines
 */
#define WS_MDISTYLE     (WS_CHILD | WS_CLIPSIBLINGS | WS_SYSMENU|WS_CAPTION|WS_THICKFRAME|WS_MAXIMIZEBOX|WS_MINIMIZEBOX)
#define WS_MDICOMMANDS  (WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define WS_MDIALLOWED   (WS_MINIMIZE | WS_MAXIMIZE | WS_CLIPCHILDREN | WS_DISABLED | WS_HSCROLL | WS_VSCROLL | 0x0000FFFFL)

#define HAS_SBVERT      0x0100
#define HAS_SBHORZ      0x0200
#define OTHERMAXING     0x0400
#define CALCSCROLL      0x0800

#define SCROLLSUPPRESS  0x0003
#define SCROLLCOUNT     0x00FF

#define CKIDS(pmdi)     (pmdi->cKids)
#define MAXED(pmdi)     (pmdi->hwndMaxedChild)
#define ACTIVE(pmdi)    (pmdi->hwndActiveChild)
#define WINDOW(pmdi)    (pmdi->hmenuWindow)
#define FIRST(pmdi)     (pmdi->idFirstChild)
#define SCROLL(pmdi)    (pmdi->wScroll)
#define ITILELEVEL(pmdi)    (pmdi->iChildTileLevel)
#define HTITLE(pmdi)    (pmdi->pTitle)

#define PROP_MDICLIENT  MAKEINTRESOURCE(0x8CAC)
#define MDIACTIVATE_PROP_NAME   TEXT("MDIA")

PWND  FindPwndChild(PWND pwndMDI, UINT wChildID);
int   MakeMenuItem(LPWSTR lpOut, PWND pwnd);
VOID  ModifyMenuItem(PWND pwnd);
BOOL  MDIAddSysMenu(HMENU hmenuFrame, HWND hwndChild);
BOOL  MDIRemoveSysMenu(HMENU hMenuFrame, HWND hwndChild);
VOID  ShiftMenuIDs(PWND pwnd, PWND pwndVictim);
HMENU MDISetMenu(PWND,BOOL,HMENU,HMENU);

/*
 * Drag and Drop menus.
 */
#define COM_NO_WINDOWS_H
#define RPC_NO_WINDOWS_H
#include <ole2.h>

/*
 * Prototypes to cast function pointers
 */
typedef HRESULT (* OLEINITIALIZEPROC)(LPVOID);
typedef HRESULT (* OLEUNINITIALIZEPROC)(VOID);
typedef HRESULT (* REGISTERDDPROC)(HWND, LPDROPTARGET);
typedef HRESULT (* REVOKEDDPROC)(HWND);
typedef HRESULT (* DODDPROC)(LPDATAOBJECT, LPDROPSOURCE, DWORD, LPDWORD);

/*
 * Internal IDropTarget interface info
 */
typedef struct tagMNIDROPTARGET
{
   IDropTarget idt;                 /* Interal IDropTarget */
   DWORD dwRefCount;                /* Ref count */
   IDataObject * pido;              /* IDataObject received at DragEnter */
   IDropTarget * pidt;              /* Application IDropTarget, if any */
} MNIDROPTARGET, * PMNIDROPTARGET;

/*
 * OLE procs info (used by LoadOLEOnce GetProcAddress calls)
 */
typedef struct tagGETPROCINFO
{
    FARPROC * ppfn;
    LPCSTR lpsz;
} GETPROCINFO;

/*
 * Special value used by __ClientLoadOLE.
 */
#define OLEWONTLOAD (HINSTANCE)IntToPtr(0xFFFFFFFF)

/*
 * Accelerator table resources list.
 */
typedef struct tagACCELCACHE
{
    struct tagACCELCACHE *pacNext;
    UINT dwLockCount;
    HACCEL hAccel;
    PVOID pRes;
} ACCELCACHE, *PACCELCACHE;


/*
 * x86 callback return function prototype
 */
#if defined(_X86_) && !defined(BUILD_WOW6432)
NTSTATUS
FASTCALL
XyCallbackReturn(
    IN PVOID Buffer,
    IN ULONG Length,
    IN NTSTATUS Status
    );

#define UserCallbackReturn XyCallbackReturn
#else
#define UserCallbackReturn NtCallbackReturn
#endif

/*
 * Reader mode support
 */
typedef LONG (CALLBACK* READERMODEPROC)(LPARAM lParam, int nCode, int dx, int dy);

typedef struct tagREADERMODE {  // rdrm
    UINT cbSize;
    DWORD dwFlags;
    READERMODEPROC pfnReaderModeProc;
    LPARAM lParam;
} READERMODE, *PREADERMODE, *LPREADERMODE;

#define RDRMODE_VERT    0x00000001
#define RDRMODE_HORZ    0x00000002
#define RDRMODE_DIAG    0x00000004

#define RDRCODE_START   1
#define RDRCODE_SCROLL  2
#define RDRCODE_END     3

typedef struct tagREADERINFO {
    READERMODE;
    int dx;
    int dy;
    UINT uCursor;
    HBITMAP hbm;
    UINT dxBmp;
    UINT dyBmp;
} READERINFO, *PREADERINFO;

typedef struct tagREADERWND {
    WND wnd;
    PREADERINFO prdr;
} READERWND, *PREADERWND;

BOOL EnterReaderModeHelper(HWND hwnd);

#include "ddemlcli.h"
#include "globals.h"
#include "cscall.h"
#include "ntuser.h"

/***************************************************************************\
*
* DBCS MESSAGING
*
\***************************************************************************/
/*
 * Message keeper for ...
 *
 * Client to Client.
 */
#define GetDispatchDbcsInfo()          (&(GetClientInfo()->achDbcsCF[0]))
/*
 * Client to Server.
 */
#define GetForwardDbcsInfo()           (&(GetClientInfo()->achDbcsCF[1]))
/*
 * Server to Client.
 */
#define GetCallBackDbcsInfo()          (&(GetClientInfo()->msgDbcsCB))

#if 0
/*
 * GetThreadCodePage()
 */
#define GetThreadCodePage()            (GetClientInfo()->CodePage)
#endif

/*
 * Macros for DBCS Messaging for Recieve side.
 */
#define GET_DBCS_MESSAGE_IF_EXIST(_apiName,_pmsg,_wMsgFilterMin,_wMsgFilterMax,bRemoveMsg)  \
                                                                                            \
        if (GetCallBackDbcsInfo()->wParam) {                                                \
            /*                                                                              \
             * Check message filter... only WM_CHAR message will be pushed                  \
             * into CLIENTINFO. Then if WM_CHAR is filtered out, we should                  \
             * get message from queue...                                                    \
             */                                                                             \
            if ((!(_wMsgFilterMin) && !(_wMsgFilterMax)) ||                                 \
                ((_wMsgFilterMin) <= WM_CHAR && (_wMsgFilterMax) >= WM_CHAR)) {             \
                PKERNEL_MSG pmsgDbcs = GetCallBackDbcsInfo();                               \
                /*                                                                          \
                 * Get pushed message.                                                      \
                 *                                                                          \
                 * Backup current message. this backupped message will be used              \
                 * when Apps peek (or get) message from thier WndProc.                      \
                 * (see GetMessageA(), PeekMessageA()...)                                   \
                 *                                                                          \
                 * pmsg->hwnd    = pmsgDbcs->hwnd;                                          \
                 * pmsg->message = pmsgDbcs->message;                                       \
                 * pmsg->wParam  = pmsgDbcs->wParam;                                        \
                 * pmsg->lParam  = pmsgDbcs->lParam;                                        \
                 * pmsg->time    = pmsgDbcs->time;                                          \
                 * pmsg->pt      = pmsgDbcs->pt;                                            \
                 */                                                                         \
                COPY_KERNELMSG_TO_MSG((_pmsg),pmsgDbcs);                                   \
                /*                                                                          \
                 * if we don't want to clear the cached data, just leave it there.          \
                 */                                                                         \
                if (bRemoveMsg) {                                                           \
                    /*                                                                      \
                     * Invalidate pushed message in CLIENTINFO.                             \
                     */                                                                     \
                    pmsgDbcs->wParam = 0;                                                   \
                }                                                                           \
                /*                                                                          \
                 * Set return value to TRUE.                                                \
                 */                                                                         \
                retval = TRUE;                                                              \
                /*                                                                          \
                 * Exit function..                                                          \
                 */                                                                         \
                goto Exit ## _apiName;                                                      \
            }                                                                               \
        }

/*
 * Macros for DBCS Messaging for Send side.
 */
#define BUILD_DBCS_MESSAGE_TO_SERVER_FROM_CLIENTA(_msg,_wParam,_RetVal)                     \
                                                                                            \
        if (IS_DBCS_ENABLED() && (((_msg) == WM_CHAR) || ((_msg) == EM_SETPASSWORDCHAR))) { \
            /*                                                                              \
             * Chech wParam is DBCS character or not.                                       \
             */                                                                             \
            if (IS_DBCS_MESSAGE((_wParam))) {                                               \
                if ((_wParam) & WMCR_IR_DBCSCHAR) {                                         \
                    /*                                                                      \
                     * This message sent with IR_DBCSCHAR, already aligned for conversion   \
                     */                                                                     \
                } else {                                                                    \
                    /*                                                                      \
                     * Make IR_DBCSCHAR compatible DBCS packed message                      \
                     */                                                                     \
                    (_wParam) = MAKEWPARAM(MAKE_IR_DBCSCHAR(LOWORD((_wParam))),0);          \
                }                                                                           \
            } else {                                                                        \
                PBYTE pchDbcsCF = GetForwardDbcsInfo();                                     \
                /*                                                                          \
                 * If we have cached Dbcs LeadingByte character, build A Dbcs character     \
                 * with the TrailingByte in wParam...                                       \
                 */                                                                         \
                if (*pchDbcsCF) {                                                           \
                    WORD DbcsLeadChar = (WORD)(*pchDbcsCF);                                 \
                    /*                                                                      \
                     * HIBYTE(LOWORD(wParam)) = Dbcs LeadingByte.                           \
                     * LOBYTE(LOWORD(wParam)) = Dbcs TrailingByte.                          \
                     */                                                                     \
                    (_wParam) |= (DbcsLeadChar << 8);                                       \
                    /*                                                                      \
                     * Invalidate cached data..                                             \
                     */                                                                     \
                    *pchDbcsCF = 0;                                                         \
                } else if (IsDBCSLeadByteEx(THREAD_CODEPAGE(),LOBYTE(LOWORD(_wParam)))) { \
                    /*                                                                      \
                     * if this is Dbcs LeadByte character, we should wait Dbcs TrailingByte \
                     * to convert this to Unicode. then we cached it here...                \
                     */                                                                     \
                    *pchDbcsCF = LOBYTE(LOWORD((_wParam)));                                 \
                    /*                                                                      \
                     * Right now, we have nothing to do for this, just return with TRUE.    \
                     */                                                                     \
                    return((_RetVal));                                                      \
                }                                                                           \
            }                                                                               \
        }

#define BUILD_DBCS_MESSAGE_TO_CLIENTW_FROM_CLIENTA(_msg,_wParam,_RetVal)                    \
                                                                                            \
        if (IS_DBCS_ENABLED() && (((_msg) == WM_CHAR) || ((_msg) == EM_SETPASSWORDCHAR))) { \
            /*                                                                              \
             * Check wParam is DBCS character or not.                                       \
             */                                                                             \
            if (IS_DBCS_MESSAGE((_wParam))) {                                               \
                if ((_wParam) & WMCR_IR_DBCSCHAR) {                                         \
                    /*                                                                      \
                     * This message sent with IR_DBCSCHAR, already aligned for conversion   \
                     */                                                                     \
                } else {                                                                    \
                    /*                                                                      \
                     * Make IR_DBCSCHAR compatible DBCS packed message                      \
                     */                                                                     \
                    (_wParam) = MAKEWPARAM(MAKE_IR_DBCSCHAR(LOWORD((_wParam))),0);          \
                }                                                                           \
            } else {                                                                        \
                PBYTE pchDbcsCF = GetDispatchDbcsInfo();                                    \
                /*                                                                          \
                 * If we have cached Dbcs LeadingByte character, build A Dbcs character     \
                 * with the TrailingByte in wParam...                                       \
                 */                                                                         \
                if (*pchDbcsCF) {                                                           \
                    WORD DbcsLeadChar = (WORD)(*pchDbcsCF);                                 \
                    /*                                                                      \
                     * HIBYTE(LOWORD(wParam)) = Dbcs LeadingByte.                           \
                     * LOBYTE(LOWORD(wParam)) = Dbcs TrailingByte.                          \
                     */                                                                     \
                    (_wParam) |= (DbcsLeadChar << 8);                                       \
                    /*                                                                      \
                     * Invalidate cached data..                                             \
                     */                                                                     \
                    *pchDbcsCF = 0;                                                         \
                } else if (IsDBCSLeadByteEx(THREAD_CODEPAGE(),LOBYTE(LOWORD(_wParam)))) { \
                    /*                                                                      \
                     * if this is Dbcs LeadByte character, we should wait Dbcs TrailingByte \
                     * to convert this to Unicode. then we cached it here...                \
                     */                                                                     \
                    *pchDbcsCF = LOBYTE(LOWORD((_wParam)));                                 \
                    /*                                                                      \
                     * Right now, we have nothing to do for this, just return with TRUE.    \
                     */                                                                     \
                    return((_RetVal));                                                      \
                }                                                                           \
            }                                                                               \
        }

#define BUILD_DBCS_MESSAGE_TO_CLIENTA_FROM_SERVER(_pmsg,_dwAnsi,_bIrDbcsFormat,bSaveMsg)    \
        /*                                                                                  \
         * _bIrDbcsFormat parameter is only effective WM_CHAR/EM_SETPASSWORDCHAR message    \
         *                                                                                  \
         * (_bIrDbcsFormat == FALSE) dwAnsi has ....                                        \
         *                                                                                  \
         * HIBYTE(LOWORD(_dwAnsi)) = DBCS TrailingByte character.                           \
         * LOBYTE(LOWORD(_dwAnsi)) = DBCS LeadingByte character                             \
         *                           or SBCS character.                                     \
         *                                                                                  \
         * (_bIrDbcsFormat == TRUE) dwAnsi has ....                                         \
         *                                                                                  \
         * HIBYTE(LOWORD(_dwAnsi)) = DBCS LeadingByte character.                            \
         * LOBYTE(LOWORD(_dwAnsi)) = DBCS TrailingByte character                            \
         *                           or SBCS character.                                     \
         */                                                                                 \
    if (IS_DBCS_ENABLED())                                                 \
        switch ((_pmsg)->message) {                                                         \
        case WM_CHAR:                                                                       \
        case EM_SETPASSWORDCHAR:                                                            \
            if (IS_DBCS_MESSAGE((_dwAnsi))) {                                               \
                /*                                                                          \
                 * This is DBCS character..                                                 \
                 */                                                                         \
                if ((_pmsg)->wParam & WMCR_IR_DBCSCHAR) {                                   \
                    /*                                                                      \
                     * Build IR_DBCSCHAR format message.                                    \
                     */                                                                     \
                    if ((_bIrDbcsFormat)) {                                                 \
                        (_pmsg)->wParam = (WPARAM)(LOWORD((_dwAnsi)));                      \
                    } else {                                                                \
                        (_pmsg)->wParam = MAKE_IR_DBCSCHAR(LOWORD((_dwAnsi)));              \
                    }                                                                       \
                } else {                                                                    \
                    PKERNEL_MSG pDbcsMsg = GetCallBackDbcsInfo();                           \
                    if ((_bIrDbcsFormat)) {                                                 \
                        /*                                                                  \
                         * if the format is IR_DBCSCHAR format, adjust it to regular        \
                         * WPARAM format...                                                 \
                         */                                                                 \
                        (_dwAnsi) = MAKE_WPARAM_DBCSCHAR((_dwAnsi));                        \
                    }                                                                       \
                    if ((bSaveMsg)) {                                                       \
                        /*                                                                  \
                         * Copy this message to CLIENTINFO for next GetMessage              \
                         * or PeekMesssage() call.                                          \
                         */                                                                 \
                        COPY_MSG_TO_KERNELMSG(pDbcsMsg,(_pmsg));                            \
                        /*                                                                  \
                         * Only Dbcs Trailingbyte is nessesary for pushed message. we'll    \
                         * pass this message when GetMessage/PeekMessage is called at next. \
                         */                                                                 \
                        pDbcsMsg->wParam = (WPARAM)(((_dwAnsi) & 0x0000FF00) >> 8);         \
                    }                                                                       \
                    /*                                                                      \
                     * Return DbcsLeading byte to Apps.                                     \
                     */                                                                     \
                    (_pmsg)->wParam =  (WPARAM)((_dwAnsi) & 0x000000FF);                    \
                }                                                                           \
            } else {                                                                        \
                /*                                                                          \
                 * This is single byte character... set it to wParam.                       \
                 */                                                                         \
                (_pmsg)->wParam = (WPARAM)((_dwAnsi) & 0x000000FF);                         \
            }                                                                               \
            break;                                                                          \
        case WM_IME_CHAR:                                                                   \
        case WM_IME_COMPOSITION:                                                            \
            /*                                                                              \
             * if the message is not adjusted to IR_DBCSCHAR format yet,                    \
             * Build WM_IME_xxx format message.                                             \
             */                                                                             \
            if (!(_bIrDbcsFormat)) {                                                        \
                (_pmsg)->wParam = MAKE_IR_DBCSCHAR(LOWORD((_dwAnsi)));                      \
            }                                                                               \
            break;                                                                          \
        default:                                                                            \
            (_pmsg)->wParam = (WPARAM)(_dwAnsi);                                            \
            break;                                                                          \
        } /* switch */                                                                      \
    else                                                                                    \

#define BUILD_DBCS_MESSAGE_TO_CLIENTW_FROM_SERVER(_msg,_wParam)                             \
                                                                                            \
        if (((_msg) == WM_CHAR) || ((_msg) == EM_SETPASSWORDCHAR)) {                        \
            /*                                                                              \
             * Only LOWORD of WPARAM is valid for WM_CHAR....                               \
             * (Mask off DBCS messaging information.)                                       \
             */                                                                             \
            (_wParam) &= 0x0000FFFF;                                                        \
        }

#define BUILD_DBCS_MESSAGE_TO_CLIENTA_FROM_CLIENTW(_hwnd,_msg,_wParam,_lParam,_time,_pt,_bDbcs) \
                                                                                                \
        if (IS_DBCS_ENABLED() && (((_msg) == WM_CHAR) || ((_msg) == EM_SETPASSWORDCHAR))) {     \
            /*                                                                                  \
             * Check this message is DBCS Message or not..                                      \
             */                                                                                 \
            if (IS_DBCS_MESSAGE((_wParam))) {                                                   \
                PKERNEL_MSG pmsgDbcsCB = GetCallBackDbcsInfo();                                 \
                /*                                                                              \
                 * Mark this is DBCS character.                                                 \
                 */                                                                             \
                (_bDbcs) = TRUE;                                                                \
                /*                                                                              \
                 * Backup current message. this backupped message will be used                  \
                 * when Apps peek (or get) message from thier WndProc.                          \
                 * (see GetMessageA(), PeekMessageA()...)                                       \
                 */                                                                             \
                pmsgDbcsCB->hwnd    = (_hwnd);                                                  \
                pmsgDbcsCB->message = (_msg);                                                   \
                pmsgDbcsCB->lParam  = (_lParam);                                                \
                pmsgDbcsCB->time    = (_time);                                                  \
                pmsgDbcsCB->pt      = (_pt);                                                    \
                /*                                                                              \
                 * DbcsLeadByte will be sent below soon, we just need DbcsTrailByte             \
                 * for further usage..                                                          \
                 */                                                                             \
                pmsgDbcsCB->wParam = ((_wParam) & 0x000000FF);                                  \
                /*                                                                              \
                 * Pass the LeadingByte of the DBCS character to an ANSI WndProc.               \
                 */                                                                             \
                (_wParam) = ((_wParam) & 0x0000FF00) >> 8;                                      \
            } else {                                                                            \
                /*                                                                              \
                 * Validate only BYTE for WM_CHAR.                                              \
                 */                                                                             \
                (_wParam) &= 0x000000FF;                                                        \
            }                                                                                   \
        }

#define DISPATCH_DBCS_MESSAGE_IF_EXIST(_msg,_wParam,_bDbcs,_apiName)                            \
        /*                                                                                      \
         * Check we need to send trailing byte or not, if the wParam has Dbcs character         \
         */                                                                                     \
        if (IS_DBCS_ENABLED() && (_bDbcs) && (GetCallBackDbcsInfo()->wParam)) {                 \
            PKERNEL_MSG pmsgDbcsCB = GetCallBackDbcsInfo();                                            \
            /*                                                                                  \
             * If an app didn't peek (or get) the trailing byte from within                     \
             * WndProc, and then pass the DBCS TrailingByte to the ANSI WndProc here            \
             * pmsgDbcsCB->wParam has DBCS TrailingByte here.. see above..                      \
             */                                                                                 \
            (_wParam) = KERNEL_WPARAM_TO_WPARAM(pmsgDbcsCB->wParam);                            \
            /*                                                                                  \
             * Invalidate cached message.                                                       \
             */                                                                                 \
            pmsgDbcsCB->wParam = 0;                                                             \
            /*                                                                                  \
             * Send it....                                                                      \
             */                                                                                 \
            goto _apiName ## Again;                                                             \
        }

#define CalcAnsiStringLengthW(_unicodestring,_unicodeLength,_ansiLength)                        \
        /*                                                                                      \
         * Get AnsiStringLength from UnicodeString,UnicodeLength                                \
         */                                                                                     \
        {                                                                                       \
            RtlUnicodeToMultiByteSize((ULONG *)(_ansiLength),                                   \
                                      (LPWSTR)(_unicodestring),                                 \
                                      (ULONG)((_unicodeLength)*sizeof(WCHAR)));                 \
        }

#define CalcAnsiStringLengthA(_ansistring,_unicodeLength,_ansiLength)                           \
        /*                                                                                      \
         * Get AnsiStringLength from AnsiString,UnicodeLength                                   \
         */                                                                                     \
        {                                                                                       \
            LPSTR _string = (_ansistring);                                                      \
            LONG  _length = (LONG)(_unicodeLength);                                             \
            (*(_ansiLength)) = 0;                                                               \
            while(*_string && _length) {                                                        \
                if(IsDBCSLeadByte(*_string)) {                                                  \
                    (*(_ansiLength)) += 2; _string++;                                           \
                } else {                                                                        \
                    (*(_ansiLength))++;                                                         \
                }                                                                               \
                _string++; _length--;                                                           \
            }                                                                                   \
        }

#define CalcUnicodeStringLengthA(_ansistring,_ansiLength,_unicodeLength)                        \
        /*                                                                                      \
         * Get UnicodeLength from AnsiString,AnsiLength                                         \
         */                                                                                     \
        {                                                                                       \
            RtlMultiByteToUnicodeSize((ULONG *)(_unicodeLength),                                \
                                      (LPSTR)(_ansistring),                                     \
                                      (ULONG)(_ansiLength));                                    \
            (*(_unicodeLength)) /= sizeof(WCHAR);                                               \
        }

#define CalcUnicodeStringLengthW(_unicodestring,_ansiLength,_unicodeLength)                     \
        /*                                                                                      \
         * Get UnicodeLength from UnicodeString,AnsiLength                                      \
         */                                                                                     \
        {                                                                                       \
            LPWSTR _string = (_unicodestring);                                                  \
            LONG   _length = (LONG)(_ansiLength);                                               \
            LONG   _charlength;                                                                 \
            (*(_unicodeLength)) = 0;                                                            \
            while(*_string && (_length > 0)) {                                                  \
                CalcAnsiStringLengthW(_string,1,&_charlength);                                  \
                _length -= _charlength;                                                         \
                if(_length >= 0) {                                                              \
                    (*(_unicodeLength))++;                                                      \
                }                                                                               \
                _string++;                                                                      \
            }                                                                                   \
        }


/*
 * DBCS function defined in userrtl.lib (see ..\rtl\userrtl.h)
 */
DWORD UserGetCodePage(HDC hdc);
BOOL  UserIsFullWidth(DWORD dwCodePage,WCHAR wChar);
BOOL  UserIsFELineBreak(DWORD dwCodePage,WCHAR wChar);


// FE_IME   // fareast.c
typedef struct {
    BOOL (WINAPI* ImmWINNLSEnableIME)(HWND, BOOL);
    BOOL (WINAPI* ImmWINNLSGetEnableStatus)(HWND);
    LRESULT (WINAPI* ImmSendIMEMessageExW)(HWND, LPARAM);
    LRESULT (WINAPI* ImmSendIMEMessageExA)(HWND, LPARAM);
    BOOL (WINAPI* ImmIMPGetIMEW)(HWND, LPIMEPROW);
    BOOL (WINAPI* ImmIMPGetIMEA)(HWND, LPIMEPROA);
    BOOL (WINAPI* ImmIMPQueryIMEW)(LPIMEPROW);
    BOOL (WINAPI* ImmIMPQueryIMEA)(LPIMEPROA);
    BOOL (WINAPI* ImmIMPSetIMEW)(HWND, LPIMEPROW);
    BOOL (WINAPI* ImmIMPSetIMEA)(HWND, LPIMEPROA);

    HIMC (WINAPI* ImmAssociateContext)(HWND, HIMC);
    LRESULT (WINAPI* ImmEscapeA)(HKL, HIMC, UINT, LPVOID);
    LRESULT (WINAPI* ImmEscapeW)(HKL, HIMC, UINT, LPVOID);
    LONG (WINAPI* ImmGetCompositionStringA)(HIMC, DWORD, LPVOID, DWORD);
    LONG (WINAPI* ImmGetCompositionStringW)(HIMC, DWORD, LPVOID, DWORD);
    BOOL (WINAPI* ImmGetCompositionWindow)(HIMC, LPCOMPOSITIONFORM);
    HIMC (WINAPI* ImmGetContext)(HWND);
    HWND (WINAPI* ImmGetDefaultIMEWnd)(HWND);
    BOOL (WINAPI* ImmIsIME)(HKL);
    BOOL (WINAPI* ImmReleaseContext)(HWND, HIMC);
    BOOL (*ImmRegisterClient)(PSHAREDINFO, HINSTANCE);

    BOOL (WINAPI* ImmGetCompositionFontW)(HIMC, LPLOGFONTW);
    BOOL (WINAPI* ImmGetCompositionFontA)(HIMC, LPLOGFONTA);
    BOOL (WINAPI* ImmSetCompositionFontW)(HIMC, LPLOGFONTW);
    BOOL (WINAPI* ImmSetCompositionFontA)(HIMC, LPLOGFONTA);

    BOOL (WINAPI* ImmSetCompositionWindow)(HIMC, LPCOMPOSITIONFORM);
    BOOL (WINAPI* ImmNotifyIME)(HIMC, DWORD, DWORD, DWORD);
    PINPUTCONTEXT (WINAPI* ImmLockIMC)(HIMC);
    BOOL (WINAPI* ImmUnlockIMC)(HIMC);
    BOOL (WINAPI* ImmLoadIME)(HKL);
    BOOL (WINAPI* ImmSetOpenStatus)(HIMC, BOOL);
    BOOL (WINAPI* ImmFreeLayout)(DWORD);
    BOOL (WINAPI* ImmActivateLayout)(HKL);
    BOOL (WINAPI* ImmSetCandidateWindow)(HIMC, LPCANDIDATEFORM);
    BOOL (WINAPI* ImmConfigureIMEW)(HKL, HWND, DWORD, LPVOID);
    BOOL (WINAPI* ImmGetConversionStatus)(HIMC, LPDWORD, LPDWORD);
    BOOL (WINAPI* ImmSetConversionStatus)(HIMC, DWORD, DWORD);
    BOOL (WINAPI* ImmSetStatusWindowPos)(HIMC, LPPOINT);
    BOOL (WINAPI* ImmGetImeInfoEx)(PIMEINFOEX, IMEINFOEXCLASS, PVOID);
    PIMEDPI (WINAPI* ImmLockImeDpi)(HKL);
    VOID (WINAPI* ImmUnlockImeDpi)(PIMEDPI);
    BOOL (WINAPI* ImmGetOpenStatus)(HIMC);
    BOOL (*ImmSetActiveContext)(HWND, HIMC, BOOL);
    BOOL (*ImmTranslateMessage)(HWND, UINT, WPARAM, LPARAM);
    BOOL (*ImmLoadLayout)(HKL, PIMEINFOEX);
    DWORD (WINAPI* ImmProcessKey)(HWND, HKL, UINT, LPARAM, DWORD);
    LRESULT (*ImmPutImeMenuItemsIntoMappedFile)(HIMC);
    DWORD (WINAPI* ImmGetProperty)(HKL hKL, DWORD dwIndex);
    BOOL (WINAPI* ImmSetCompositionStringA)(
        HIMC hImc, DWORD dwIndex, LPCVOID lpComp, DWORD dwCompLen, LPCVOID lpRead, DWORD dwReadLen);
    BOOL (WINAPI* ImmSetCompositionStringW)(
        HIMC hImc, DWORD dwIndex, LPCVOID lpComp, DWORD dwCompLen, LPCVOID lpRead, DWORD dwReadLen);
    BOOL (WINAPI* ImmEnumInputContext)(
        DWORD idThread, IMCENUMPROC lpfn, LPARAM lParam);
    LRESULT (WINAPI* ImmSystemHandler)(HIMC, WPARAM, LPARAM);
} ImmApiEntries;

extern ImmApiEntries gImmApiEntries;
extern HMODULE ghImm32;
VOID InitializeImmEntryTable(VOID);
VOID GetImmFileName(PWSTR);
VOID CliImmInitializeHotKeys(DWORD dwAction, HKL hkl);

#define fpImmAssociateContext       gImmApiEntries.ImmAssociateContext
#define fpImmEscapeA                gImmApiEntries.ImmEscapeA
#define fpImmEscapeW                gImmApiEntries.ImmEscapeW
#define fpImmGetContext             gImmApiEntries.ImmGetContext
#define fpImmGetCompositionStringA  gImmApiEntries.ImmGetCompositionStringA
#define fpImmGetCompositionStringW  gImmApiEntries.ImmGetCompositionStringW
#define fpImmGetCompositionWindow   gImmApiEntries.ImmGetCompositionWindow
#define fpImmGetDefaultIMEWnd       gImmApiEntries.ImmGetDefaultIMEWnd
#define fpImmIsIME                  gImmApiEntries.ImmIsIME
#define fpImmLockIMC                gImmApiEntries.ImmLockIMC
#define fpImmReleaseContext         gImmApiEntries.ImmReleaseContext
#define fpImmRegisterClient         gImmApiEntries.ImmRegisterClient
#define fpImmGetCompositionFontW    gImmApiEntries.ImmGetCompositionFontW
#define fpImmGetCompositionFontA    gImmApiEntries.ImmGetCompositionFontA
#define fpImmSetCompositionFontW    gImmApiEntries.ImmSetCompositionFontW
#define fpImmSetCompositionFontA    gImmApiEntries.ImmSetCompositionFontA
#define fpImmSetCompositionFont     gImmApiEntries.ImmSetCompositionFont
#define fpImmSetCompositionWindow   gImmApiEntries.ImmSetCompositionWindow
#define fpImmNotifyIME              gImmApiEntries.ImmNotifyIME
#define fpImmUnlockIMC              gImmApiEntries.ImmUnlockIMC
#define fpImmLoadIME                gImmApiEntries.ImmLoadIME
#define fpImmSetOpenStatus          gImmApiEntries.ImmSetOpenStatus
#define fpImmFreeLayout             gImmApiEntries.ImmFreeLayout
#define fpImmActivateLayout         gImmApiEntries.ImmActivateLayout
#define fpImmSetCandidateWindow     gImmApiEntries.ImmSetCandidateWindow
#define fpImmConfigureIMEW          gImmApiEntries.ImmConfigureIMEW
#define fpImmGetConversionStatus    gImmApiEntries.ImmGetConversionStatus
#define fpImmSetConversionStatus    gImmApiEntries.ImmSetConversionStatus
#define fpImmSetStatusWindowPos     gImmApiEntries.ImmSetStatusWindowPos
#define fpImmGetImeInfoEx           gImmApiEntries.ImmGetImeInfoEx
#define fpImmLockImeDpi             gImmApiEntries.ImmLockImeDpi
#define fpImmUnlockImeDpi           gImmApiEntries.ImmUnlockImeDpi
#define fpImmGetOpenStatus          gImmApiEntries.ImmGetOpenStatus
#define fpImmSetActiveContext       gImmApiEntries.ImmSetActiveContext
#define fpImmTranslateMessage       gImmApiEntries.ImmTranslateMessage
#define fpImmLoadLayout             gImmApiEntries.ImmLoadLayout
#define fpImmProcessKey             gImmApiEntries.ImmProcessKey
#define fpImmPutImeMenuItemsIntoMappedFile gImmApiEntries.ImmPutImeMenuItemsIntoMappedFile
#define fpImmGetProperty            gImmApiEntries.ImmGetProperty
#define fpImmSetCompositionStringA  gImmApiEntries.ImmSetCompositionStringA
#define fpImmSetCompositionStringW  gImmApiEntries.ImmSetCompositionStringW
#define fpImmEnumInputContext       gImmApiEntries.ImmEnumInputContext
#define fpImmSystemHandler          gImmApiEntries.ImmSystemHandler

BOOL SyncSoftKbdState(HIMC hImc, LPARAM lParam); // imectl.c

// end FE_IME

/*
 * Rebasing functions for shared memory. Need to located after
 * inclusion of globals.h.
 */
__inline PVOID
REBASESHAREDPTRALWAYS(KERNEL_PVOID p)
{
    return (PVOID)(((KERNEL_UINT_PTR)p) - gSharedInfo.ulSharedDelta);
}

__inline PVOID
REBASESHAREDPTR(KERNEL_PVOID p)
{
    return (p) ? REBASESHAREDPTRALWAYS(p) : NULL;
}

/*
 * Multimonitor macros used in RTL. There are similar definitions
 * in kernel\userk.h
 */
__inline PDISPLAYINFO
GetDispInfo(void)
{
    return gSharedInfo.pDispInfo;
}

__inline PMONITOR
GetPrimaryMonitor(void)
{
    return REBASESHAREDPTR(GetDispInfo()->pMonitorPrimary);
}


#endif // !_USERCLI_
