/*
 * Unit tests for window message handling
 *
 * Copyright 1999 Ove Kaaven
 * Copyright 2003 Dimitrie O. Paun
 * Copyright 2004, 2005 Dmitry Timoshkov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

//#define _WIN32_WINNT 0x0600 /* For WM_CHANGEUISTATE,QS_RAWINPUT,WM_DWMxxxx */
//#define WINVER 0x0600 /* for WM_GETTITLEBARINFOEX */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"

#include "wine/test.h"

#define MDI_FIRST_CHILD_ID 2004

/* undocumented SWP flags - from SDK 3.1 */
#define SWP_NOCLIENTSIZE	0x0800
#define SWP_NOCLIENTMOVE	0x1000
#define SWP_STATECHANGED	0x8000

#define SW_NORMALNA	        0xCC    /* undoc. flag in MinMaximize */

#ifndef WM_KEYF1
#define WM_KEYF1 0x004d
#endif

#ifndef WM_SYSTIMER
#define WM_SYSTIMER	    0x0118
#endif

#define WND_PARENT_ID		1
#define WND_POPUP_ID		2
#define WND_CHILD_ID		3

#ifndef WM_LBTRACKPOINT
#define WM_LBTRACKPOINT  0x0131
#endif

#ifdef __i386__
#define ARCH "x86"
#elif defined __x86_64__
#define ARCH "amd64"
#else
#define ARCH "none"
#endif

static BOOL   (WINAPI *pActivateActCtx)(HANDLE,ULONG_PTR*);
static HANDLE (WINAPI *pCreateActCtxW)(PCACTCTXW);
static BOOL   (WINAPI *pDeactivateActCtx)(DWORD,ULONG_PTR);
static BOOL   (WINAPI *pGetCurrentActCtx)(HANDLE *);
static void   (WINAPI *pReleaseActCtx)(HANDLE);

/* encoded DRAWITEMSTRUCT into an LPARAM */
typedef struct
{
    union
    {
        struct
        {
            UINT type    : 4;  /* ODT_* flags */
            UINT ctl_id  : 4;  /* Control ID */
            UINT item_id : 4;  /* Menu item ID */
            UINT action  : 4;  /* ODA_* flags */
            UINT state   : 16; /* ODS_* flags */
        } item;
        LPARAM lp;
    } u;
} DRAW_ITEM_STRUCT;

static BOOL test_DestroyWindow_flag;
static HWINEVENTHOOK hEvent_hook;
static HHOOK hKBD_hook;
static HHOOK hCBT_hook;
static DWORD cbt_hook_thread_id;

static const WCHAR testWindowClassW[] =
{ 'T','e','s','t','W','i','n','d','o','w','C','l','a','s','s','W',0 };

/*
FIXME: add tests for these
Window Edge Styles (Win31/Win95/98 look), in order of precedence:
 WS_EX_DLGMODALFRAME: double border, WS_CAPTION allowed
 WS_THICKFRAME: thick border
 WS_DLGFRAME: double border, WS_CAPTION not allowed (but possibly shown anyway)
 WS_BORDER (default for overlapped windows): single black border
 none (default for child (and popup?) windows): no border
*/

typedef enum {
    sent=0x1,
    posted=0x2,
    parent=0x4,
    wparam=0x8,
    lparam=0x10,
    defwinproc=0x20,
    beginpaint=0x40,
    optional=0x80,
    hook=0x100,
    winevent_hook=0x200,
    kbd_hook=0x400
} msg_flags_t;

struct message {
    UINT message;          /* the WM_* code */
    msg_flags_t flags;     /* message props */
    WPARAM wParam;         /* expected value of wParam */
    LPARAM lParam;         /* expected value of lParam */
    WPARAM wp_mask;        /* mask for wParam checks */
    LPARAM lp_mask;        /* mask for lParam checks */
};

struct recvd_message {
    UINT message;          /* the WM_* code */
    msg_flags_t flags;     /* message props */
    HWND hwnd;             /* window that received the message */
    WPARAM wParam;         /* expected value of wParam */
    LPARAM lParam;         /* expected value of lParam */
    int line;              /* source line where logged */
    const char *descr;     /* description for trace output */
    char output[512];      /* trace output */
};

/* Empty message sequence */
static const struct message WmEmptySeq[] =
{
    { 0 }
};
/* CreateWindow (for overlapped window, not initially visible) (16/32) */
static const struct message WmCreateOverlappedSeq[] = {
    { HCBT_CREATEWND, hook },
    { WM_GETMINMAXINFO, sent },
    { WM_NCCREATE, sent },
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { 0x0093, sent|defwinproc|optional },
    { 0x0094, sent|defwinproc|optional },
    { EVENT_OBJECT_REORDER, winevent_hook|wparam|lparam, 0, 0 },
    { WM_CREATE, sent },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, 0, 0 },
    { 0 }
};
/* SetWindowPos(SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE)
 * for a not visible overlapped window.
 */
static const struct message WmSWP_ShowOverlappedSeq[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { WM_NCPAINT, sent|wparam|optional, 1 },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    { HCBT_ACTIVATE, hook },
    { EVENT_SYSTEM_FOREGROUND, winevent_hook|wparam|lparam, 0, 0 },
    { WM_NOTIFYFORMAT, sent|optional },
    { WM_QUERYUISTATE, sent|optional },
    { WM_QUERYNEWPALETTE, sent|wparam|lparam|optional, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE }, /* Win9x: SWP_NOSENDCHANGING */
    { WM_ACTIVATEAPP, sent|wparam, 1 },
    { WM_NCACTIVATE, sent },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ACTIVATE, sent|wparam, 1 },
    { HCBT_SETFOCUS, hook },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|wparam|defwinproc, 0 },
    { WM_GETTEXT, sent|optional },
    { WM_NCPAINT, sent|wparam|optional, 1 },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    /* Win9x adds SWP_NOZORDER below */
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_GETTEXT, sent|optional },
    { WM_NCCALCSIZE, sent|wparam|optional, 1 },
    { WM_NCPAINT, sent|wparam|optional, 1 },
    { WM_ERASEBKGND, sent|optional },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SYNCPAINT, sent|optional },
    { WM_GETTITLEBARINFOEX, sent|optional },
    { WM_PAINT, sent|optional },
    { WM_NCPAINT, sent|beginpaint|optional },
    { WM_GETTEXT, sent|beginpaint|defwinproc|optional },
    { WM_ERASEBKGND, sent|beginpaint|optional },
    { 0 }
};
/* SetWindowPos(SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOMOVE)
 * for a visible overlapped window.
 */
static const struct message WmSWP_HideOverlappedSeq[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_HIDE, winevent_hook|wparam|lparam, 0, 0 },
    { HCBT_ACTIVATE, hook|optional },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE },
    { WM_ACTIVATEAPP, sent|wparam|optional, 1 },
    { WM_NCACTIVATE, sent|optional },
    { WM_ACTIVATE, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { 0 }
};

/* SetWindowPos(SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOMOVE)
 * for a visible overlapped window.
 */
static const struct message WmSWP_ResizeSeq[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOACTIVATE|SWP_NOMOVE },
    { WM_GETMINMAXINFO, sent|defwinproc },
    { WM_NCCALCSIZE, sent|wparam|optional, TRUE },
    { WM_NCPAINT, sent|optional },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOCLIENTMOVE },
    { WM_SIZE, sent|defwinproc|optional },
    { WM_NCCALCSIZE, sent|wparam|optional, TRUE },
    { WM_NCPAINT, sent|optional },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam|optional, 0, 0 }, /* XP sends a duplicate */
    { 0 }
};

/* SetWindowPos(SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOMOVE)
 * for a visible popup window.
 */
static const struct message WmSWP_ResizePopupSeq[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOACTIVATE|SWP_NOMOVE },
    { WM_GETMINMAXINFO, sent|defwinproc|optional }, /* Win9x */
    { WM_NCCALCSIZE, sent|wparam|optional, TRUE },
    { WM_NCPAINT, sent|optional },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOCLIENTMOVE },
    { WM_SIZE, sent|defwinproc|wparam|optional, SIZE_RESTORED },
    { WM_NCCALCSIZE, sent|wparam|optional, TRUE },
    { WM_NCPAINT, sent|optional },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { 0 }
};

/* SetWindowPos(SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOSIZE)
 * for a visible overlapped window.
 */
static const struct message WmSWP_MoveSeq[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOACTIVATE|SWP_NOSIZE },
    { WM_NCPAINT, sent|optional },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOCLIENTSIZE },
    { WM_MOVE, sent|defwinproc|wparam, 0 },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { 0 }
};
/* Resize with SetWindowPos(SWP_NOZORDER)
 * for a visible overlapped window
 * SWP_NOZORDER is stripped by the logging code
 */
static const struct message WmSWP_ResizeNoZOrder[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam, /*SWP_NOZORDER|*/SWP_NOACTIVATE },
    { WM_GETMINMAXINFO, sent|defwinproc },
    { WM_NCCALCSIZE, sent|wparam|optional, 1 },
    { WM_NCPAINT, sent|optional },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, /*SWP_NOZORDER|*/SWP_NOACTIVATE, 0,
      SWP_NOMOVE|SWP_NOCLIENTMOVE|SWP_NOSIZE|SWP_NOCLIENTSIZE },
    { WM_MOVE, sent|defwinproc|optional },
    { WM_SIZE, sent|defwinproc|optional },
    { WM_NCCALCSIZE, sent|wparam|optional, 1 }, /* Win9x doesn't send it */
    { WM_NCPAINT, sent|optional }, /* Win9x doesn't send it */
    { WM_GETTEXT, sent|defwinproc|optional }, /* Win9x doesn't send it */
    { WM_ERASEBKGND, sent|optional }, /* Win9x doesn't send it */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { 0 }
};

/* Switch visible mdi children */
static const struct message WmSwitchChild[] = {
    /* Switch MDI child */
    { WM_MDIACTIVATE, sent },/* in the MDI client */
    { WM_WINDOWPOSCHANGING, sent|wparam,SWP_NOSIZE|SWP_NOMOVE },/* in the 1st MDI child */
    { EVENT_OBJECT_REORDER, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_CHILDACTIVATE, sent },/* in the 1st MDI child */
    /* Deactivate 2nd MDI child */
    { WM_NCACTIVATE, sent|wparam|defwinproc, 0 }, /* in the 2nd MDI child */
    { WM_MDIACTIVATE, sent|defwinproc }, /* in the 2nd MDI child */
    { HCBT_MINMAX, hook|lparam, 0, SW_MAXIMIZE },
    /* Preparing for maximize and maximize the 1st MDI child */
    { WM_GETMINMAXINFO, sent|defwinproc }, /* in the 1st MDI child */
    { WM_WINDOWPOSCHANGING, sent|wparam|defwinproc, SWP_FRAMECHANGED|SWP_STATECHANGED }, /* in the 1st MDI child */
    { WM_NCCALCSIZE, sent|wparam|defwinproc, 1 }, /* in the 1st MDI child */
    { WM_CHILDACTIVATE, sent|defwinproc }, /* in the 1st MDI child */
    { WM_WINDOWPOSCHANGED, sent|wparam|defwinproc, SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOCLIENTMOVE|SWP_STATECHANGED }, /* in the 1st MDI child */
    { WM_SIZE, sent|defwinproc|wparam, SIZE_MAXIMIZED }, /* in the 1st MDI child */
    /* Lock redraw 2nd MDI child */
    { WM_SETREDRAW, sent|wparam|defwinproc, 0 }, /* in the 2nd MDI child */
    { HCBT_MINMAX, hook|lparam, 0, SW_NORMALNA },
    /* Restore 2nd MDI child */
    { WM_WINDOWPOSCHANGING, sent|wparam|defwinproc, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_STATECHANGED },/* in the 2nd MDI child */
    { WM_NCCALCSIZE, sent|wparam|defwinproc, 1 },/* in the 2nd MDI child */
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 }, /* in the 2nd MDI child */
    { WM_WINDOWPOSCHANGED, sent|wparam|defwinproc, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOCLIENTMOVE|SWP_STATECHANGED }, /* in the 2nd MDI child */
    { WM_SIZE, sent|defwinproc|wparam, SIZE_RESTORED }, /* in the 2nd MDI child */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* in the 2nd MDI child */
    /* Redraw 2nd MDI child */
    { WM_SETREDRAW, sent|wparam|defwinproc, 1 },/* in the 2nd MDI child */
    /* Redraw MDI frame */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE },/* in MDI frame */
    { WM_NCCALCSIZE, sent|wparam, 1 },/* in MDI frame */
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE}, /* in MDI frame */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* in MDI frame */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* in the 1st MDI child */
    { WM_WINDOWPOSCHANGING, sent|wparam|defwinproc, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE }, /* in the 1st MDI child */
    { WM_NCACTIVATE, sent|wparam|defwinproc, 1 }, /* in the 1st MDI child */
    { HCBT_SETFOCUS, hook },
    { WM_KILLFOCUS, sent|defwinproc }, /* in the 2nd MDI child */
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 0 },/* in the 1st MDI child */
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 }, /* in MDI client */
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent },/* in the MDI client */
    { HCBT_SETFOCUS, hook },
    { WM_KILLFOCUS, sent },/* in the MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 }, /* in the 1st MDI child */
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|defwinproc }, /* in the 1st MDI child */
    { WM_MDIACTIVATE, sent|defwinproc },/* in the 1st MDI child */
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE }, /* in the 1st MDI child */
    { 0 }
};

/* Switch visible not maximized mdi children */
static const struct message WmSwitchNotMaximizedChild[] = {
    /* Switch not maximized MDI child */
    { WM_MDIACTIVATE, sent },/* in the MDI client */
    { WM_WINDOWPOSCHANGING, sent|wparam,SWP_NOSIZE|SWP_NOMOVE },/* in the 2nd MDI child */
    { EVENT_OBJECT_REORDER, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_CHILDACTIVATE, sent },/* in the 2nd MDI child */
    /* Deactivate 1st MDI child */
    { WM_NCACTIVATE, sent|wparam|defwinproc, 0 }, /* in the 1st MDI child */
    { WM_MDIACTIVATE, sent|defwinproc }, /* in the 1st MDI child */
    /* Activate 2nd MDI child */
    { WM_WINDOWPOSCHANGING, sent|wparam|defwinproc, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE}, /* in the 2nd MDI child */
    { WM_NCACTIVATE, sent|wparam|defwinproc, 1 }, /* in the 2nd MDI child */
    { HCBT_SETFOCUS, hook }, /* in the 1st MDI child */
    { WM_KILLFOCUS, sent|defwinproc }, /* in the 1st MDI child */
    { WM_IME_SETCONTEXT, sent|defwinproc|optional }, /* in the 1st MDI child */
    { WM_IME_SETCONTEXT, sent|optional }, /* in the  MDI client */
    { WM_SETFOCUS, sent, 0 }, /* in the  MDI client */
    { HCBT_SETFOCUS, hook },
    { WM_KILLFOCUS, sent }, /* in the  MDI client */
    { WM_IME_SETCONTEXT, sent|optional }, /* in the  MDI client */
    { WM_IME_SETCONTEXT, sent|defwinproc|optional  }, /* in the 1st MDI child */
    { WM_SETFOCUS, sent|defwinproc }, /* in the 2nd MDI child */
    { WM_MDIACTIVATE, sent|defwinproc }, /* in the 2nd MDI child */
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE}, /* in the 2nd MDI child */
    { 0 }
};


/* SetWindowPos(SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|
                SWP_NOZORDER|SWP_FRAMECHANGED)
 * for a visible overlapped window with WS_CLIPCHILDREN style set.
 */
static const struct message WmSWP_FrameChanged_clip[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam|parent, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_FRAMECHANGED },
    { WM_NCCALCSIZE, sent|wparam|parent, 1 },
    { WM_NCPAINT, sent|parent|optional }, /* wparam != 1 */
    { WM_GETTEXT, sent|parent|defwinproc|optional },
    { WM_ERASEBKGND, sent|parent|optional }, /* FIXME: remove optional once Wine is fixed */
    { WM_NCPAINT, sent }, /* wparam != 1 */
    { WM_ERASEBKGND, sent },
    { WM_WINDOWPOSCHANGED, sent|wparam|parent, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { 0 }
};
/* SetWindowPos(SWP_NOSIZE|SWP_NOMOVE|SWP_DEFERERASE|SWP_NOACTIVATE|
                SWP_NOZORDER|SWP_FRAMECHANGED)
 * for a visible overlapped window.
 */
static const struct message WmSWP_FrameChangedDeferErase[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam|parent, SWP_NOSIZE|SWP_NOMOVE|SWP_DEFERERASE|SWP_NOACTIVATE|SWP_FRAMECHANGED },
    { WM_NCCALCSIZE, sent|wparam|parent, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam|parent, SWP_NOSIZE|SWP_NOMOVE|SWP_DEFERERASE|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_PAINT, sent|parent|optional },
    { WM_NCPAINT, sent|beginpaint|parent|optional }, /* wparam != 1 */
    { WM_GETTEXT, sent|beginpaint|parent|defwinproc|optional },
    { WM_PAINT, sent },
    { WM_NCPAINT, sent|beginpaint }, /* wparam != 1 */
    { WM_ERASEBKGND, sent|beginpaint|optional },
    { 0 }
};

/* SetWindowPos(SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|
                SWP_NOZORDER|SWP_FRAMECHANGED)
 * for a visible overlapped window without WS_CLIPCHILDREN style set.
 */
static const struct message WmSWP_FrameChanged_noclip[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam|parent, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_FRAMECHANGED },
    { WM_NCCALCSIZE, sent|wparam|parent, 1 },
    { WM_NCPAINT, sent|parent|optional }, /* wparam != 1 */
    { WM_GETTEXT, sent|parent|defwinproc|optional },
    { WM_ERASEBKGND, sent|parent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam|parent, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { WM_NCPAINT, sent|beginpaint }, /* wparam != 1 */
    { WM_ERASEBKGND, sent|beginpaint|optional },
    { 0 }
};

/* ShowWindow(SW_SHOW) for a not visible overlapped window */
static const struct message WmShowOverlappedSeq[] = {
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_NCPAINT, sent|wparam|optional, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { WM_NCPAINT, sent|wparam|optional, 1 },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    { HCBT_ACTIVATE, hook|optional },
    { EVENT_SYSTEM_FOREGROUND, winevent_hook|wparam|lparam, 0, 0 },
    { WM_QUERYNEWPALETTE, sent|wparam|lparam|optional, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCPAINT, sent|wparam|optional, 1 },
    { WM_ACTIVATEAPP, sent|wparam|optional, 1 },
    { WM_NCACTIVATE, sent|wparam|optional, 1 },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ACTIVATE, sent|wparam|optional, 1 },
    { HCBT_SETFOCUS, hook|optional },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|wparam|defwinproc|optional, 0 },
    { WM_GETTEXT, sent|optional },
    { WM_NCPAINT, sent|wparam|optional, 1 },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    /* Win9x adds SWP_NOZORDER below */
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_NCCALCSIZE, sent|optional },
    { WM_GETTEXT, sent|optional },
    { WM_NCPAINT, sent|optional },
    { WM_ERASEBKGND, sent|optional },
    { WM_SYNCPAINT, sent|optional },
#if 0 /* CreateWindow/ShowWindow(SW_SHOW) also generates WM_SIZE/WM_MOVE
       * messages. Does that mean that CreateWindow doesn't set initial
       * window dimensions for overlapped windows?
       */
    { WM_SIZE, sent },
    { WM_MOVE, sent },
#endif
    { WM_PAINT, sent|optional },
    { WM_NCPAINT, sent|beginpaint|optional },
    { 0 }
};
/* ShowWindow(SW_SHOWMAXIMIZED) for a not visible overlapped window */
static const struct message WmShowMaxOverlappedSeq[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_MAXIMIZE },
    { WM_GETMINMAXINFO, sent },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_STATECHANGED },
    { WM_GETMINMAXINFO, sent|defwinproc },
    { WM_NCCALCSIZE, sent|wparam, TRUE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { HCBT_ACTIVATE, hook|optional },
    { EVENT_SYSTEM_FOREGROUND, winevent_hook|wparam|lparam, 0, 0 },
    { WM_QUERYNEWPALETTE, sent|wparam|lparam|optional, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE },
    { WM_ACTIVATEAPP, sent|wparam|optional, 1 },
    { WM_NCACTIVATE, sent|wparam|optional, 1 },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ACTIVATE, sent|wparam|optional, 1 },
    { HCBT_SETFOCUS, hook|optional },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|wparam|defwinproc|optional, 0 },
    { WM_GETTEXT, sent|optional },
    { WM_NCPAINT, sent|wparam|optional, 1 },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    /* Win9x adds SWP_NOZORDER below */
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_STATECHANGED },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_MAXIMIZED },
    { WM_GETTEXT, sent|optional },
    { WM_NCCALCSIZE, sent|optional },
    { WM_NCPAINT, sent|optional },
    { WM_ERASEBKGND, sent|optional },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SYNCPAINT, sent|optional },
    { WM_GETTITLEBARINFOEX, sent|optional },
    { WM_PAINT, sent|optional },
    { WM_NCPAINT, sent|beginpaint|optional },
    { WM_ERASEBKGND, sent|beginpaint|optional },
    { 0 }
};
/* ShowWindow(SW_RESTORE) for a not visible maximized overlapped window */
static const struct message WmShowRestoreMaxOverlappedSeq[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_RESTORE },
    { WM_GETTEXT, sent|optional },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_STATECHANGED },
    { WM_GETMINMAXINFO, sent|defwinproc },
    { WM_NCCALCSIZE, sent|wparam, TRUE },
    { WM_NCPAINT, sent|optional },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_STATECHANGED, 0, SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_MOVE, sent|defwinproc|optional },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_RESTORED },
    { WM_NCCALCSIZE, sent|wparam|optional, TRUE },
    { WM_NCPAINT, sent|optional },
    { WM_ERASEBKGND, sent|optional },
    { WM_PAINT, sent|optional },
    { WM_GETTITLEBARINFOEX, sent|optional },
    { WM_NCPAINT, sent|beginpaint|optional },
    { WM_ERASEBKGND, sent|beginpaint|optional },
    { 0 }
};
/* ShowWindow(SW_RESTORE) for a not visible minimized overlapped window */
static const struct message WmShowRestoreMinOverlappedSeq[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_RESTORE },
    { WM_QUERYOPEN, sent|optional },
    { WM_GETTEXT, sent|optional },
    { WM_NCACTIVATE, sent|wparam|optional, 1 },
    { WM_WINDOWPOSCHANGING, sent|optional }, /* SWP_NOSIZE|SWP_NOMOVE */
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_NCCALCSIZE, sent|wparam|optional, TRUE },
    { WM_MOVE, sent|optional },
    { WM_SIZE, sent|wparam|optional, SIZE_RESTORED },
    { WM_GETTEXT, sent|optional },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_FRAMECHANGED|SWP_STATECHANGED|SWP_NOCOPYBITS },
    { WM_GETMINMAXINFO, sent|defwinproc|optional },
    { WM_NCCALCSIZE, sent|wparam|optional, TRUE },
    { HCBT_ACTIVATE, hook|optional },
    { WM_QUERYNEWPALETTE, sent|wparam|lparam|optional, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE },
    { WM_ACTIVATEAPP, sent|wparam|optional, 1 },
    { WM_NCACTIVATE, sent|wparam|optional, 1 },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ACTIVATE, sent|wparam|optional, 1 },
    { HCBT_SETFOCUS, hook|optional },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { WM_SETFOCUS, sent|wparam|defwinproc|optional, 0 },
    { WM_GETTEXT, sent|optional },
    { WM_NCPAINT, sent|wparam|optional, 1 },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_STATECHANGED|SWP_FRAMECHANGED|SWP_NOCOPYBITS },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_RESTORED },
    { HCBT_SETFOCUS, hook|optional },
    { WM_SETFOCUS, sent|wparam|optional, 0 },
    { WM_NCCALCSIZE, sent|wparam|optional, TRUE },
    { WM_NCPAINT, sent|wparam|optional, 1 },
    { WM_ERASEBKGND, sent|optional },
    { HCBT_SETFOCUS, hook|optional },
    { WM_SETFOCUS, sent|wparam|optional, 0 },
    { WM_ACTIVATE, sent|wparam, 1 },
    { WM_GETTEXT, sent|optional },
    { WM_PAINT, sent|optional },
    { WM_GETTITLEBARINFOEX, sent|optional },
    { WM_NCPAINT, sent|beginpaint|optional },
    { WM_ERASEBKGND, sent|beginpaint|optional },
    { 0 }
};
/* ShowWindow(SW_SHOWMINIMIZED) for a not visible overlapped window */
static const struct message WmShowMinOverlappedSeq[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_MINIMIZE },
    { HCBT_SETFOCUS, hook|optional },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_KILLFOCUS, sent|optional },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 },
    { WM_IME_NOTIFY, sent|wparam|optional|defwinproc, 1 },
    { WM_GETTEXT, sent|optional },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOCOPYBITS|SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_STATECHANGED },
    { WM_GETMINMAXINFO, sent|defwinproc },
    { WM_NCCALCSIZE, sent|wparam, TRUE },
    { EVENT_OBJECT_REORDER, winevent_hook|wparam|lparam, 0, 0 },
    { WM_NCPAINT, sent|optional },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_WINDOWPOSCHANGED, sent },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc|wparam|lparam, SIZE_MINIMIZED, 0 },
    { WM_NCCALCSIZE, sent|optional },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { EVENT_SYSTEM_MINIMIZESTART, winevent_hook|wparam|lparam, 0, 0 },
    { WM_NCACTIVATE, sent|wparam|optional, 0 },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ACTIVATE, sent|optional },
    { WM_ACTIVATEAPP, sent|wparam|optional, 0 },

    /* Vista sometimes restores the window right away... */
    { WM_SYSCOMMAND, sent|optional|wparam, SC_RESTORE },
    { HCBT_SYSCOMMAND, hook|optional|wparam, SC_RESTORE },
    { HCBT_MINMAX, hook|optional|lparam, 0, SW_RESTORE },
    { WM_QUERYOPEN, sent|optional },
    { WM_WINDOWPOSCHANGING, sent|optional|wparam, SWP_FRAMECHANGED|SWP_STATECHANGED },
    { WM_GETMINMAXINFO, sent|optional|defwinproc },
    { WM_NCCALCSIZE, sent|optional|wparam, TRUE },
    { HCBT_ACTIVATE, hook|optional },
    { WM_ACTIVATEAPP, sent|optional|wparam, 1 },
    { WM_NCACTIVATE, sent|optional },
    { WM_GETTEXT, sent|optional },
    { WM_ACTIVATE, sent|optional|wparam, 1 },
    { HCBT_SETFOCUS, hook|optional },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { WM_SETFOCUS, sent|optional },
    { WM_NCPAINT, sent|optional },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|optional|wparam, SWP_FRAMECHANGED|SWP_STATECHANGED },
    { WM_MOVE, sent|defwinproc|optional },
    { WM_SIZE, sent|defwinproc|optional|wparam, SIZE_RESTORED },
    { WM_ACTIVATE, sent|optional|wparam, 1 },
    { WM_SYSCOMMAND, sent|optional|wparam, SC_RESTORE },
    { HCBT_SYSCOMMAND, hook|optional|wparam, SC_RESTORE },

    { WM_PAINT, sent|optional },
    { WM_NCPAINT, sent|beginpaint|optional },
    { WM_ERASEBKGND, sent|beginpaint|optional },
    { 0 }
};
/* ShowWindow(SW_HIDE) for a visible overlapped window */
static const struct message WmHideOverlappedSeq[] = {
    { WM_SHOWWINDOW, sent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_HIDE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_SIZE, sent|optional }, /* XP doesn't send it */
    { WM_MOVE, sent|optional }, /* XP doesn't send it */
    { WM_NCACTIVATE, sent|wparam|optional, 0 },
    { WM_ACTIVATE, sent|wparam|optional, 0 },
    { WM_ACTIVATEAPP, sent|wparam|optional, 0 },
    { HCBT_SETFOCUS, hook|optional },
    { WM_KILLFOCUS, sent|wparam|optional, 0 },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 },
    { WM_IME_NOTIFY, sent|wparam|optional|defwinproc, 1 },
    { 0 }
};
/* DestroyWindow for a visible overlapped window */
static const struct message WmDestroyOverlappedSeq[] = {
    { HCBT_DESTROYWND, hook },
    { 0x0090, sent|optional },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_HIDE, winevent_hook|wparam|lparam, 0, 0 },
    { 0x0090, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_NCACTIVATE, sent|optional|wparam, 0 },
    { WM_ACTIVATE, sent|optional },
    { WM_ACTIVATEAPP, sent|optional|wparam, 0 },
    { WM_KILLFOCUS, sent|optional|wparam, 0 },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 },
    { WM_IME_NOTIFY, sent|wparam|optional|defwinproc, 1 },
    { EVENT_OBJECT_DESTROY, winevent_hook|wparam|lparam, 0, 0 },
    { WM_DESTROY, sent },
    { WM_NCDESTROY, sent },
    { 0 }
};
/* CreateWindow(WS_MAXIMIZE|WS_VISIBLE) for popup window */
static const struct message WmCreateMaxPopupSeq[] = {
    { HCBT_CREATEWND, hook },
    { WM_NCCREATE, sent },
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { WM_CREATE, sent },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },
    { WM_MOVE, sent },
    { HCBT_MINMAX, hook|lparam, 0, SW_MAXIMIZE },
    { WM_GETMINMAXINFO, sent },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_STATECHANGED },
    { WM_NCCALCSIZE, sent|wparam, TRUE },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOREDRAW|SWP_STATECHANGED },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_MAXIMIZED },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { HCBT_ACTIVATE, hook },
    { EVENT_SYSTEM_FOREGROUND, winevent_hook|wparam|lparam, 0, 0 },
    { WM_QUERYNEWPALETTE, sent|wparam|lparam|optional, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCPAINT, sent|wparam|optional, 1 },
    { WM_ERASEBKGND, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_NOCLIENTMOVE|SWP_NOCLIENTSIZE|SWP_NOMOVE|SWP_NOSIZE },
    { WM_ACTIVATEAPP, sent|wparam, 1 },
    { WM_NCACTIVATE, sent },
    { WM_ACTIVATE, sent|wparam, 1 },
    { HCBT_SETFOCUS, hook },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|wparam|defwinproc, 0 },
    { WM_GETTEXT, sent|optional },
    { WM_SYNCPAINT, sent|wparam|optional, 4 },
    { WM_NCPAINT, sent|wparam|optional, 1 },
    { WM_ERASEBKGND, sent|optional },
    { WM_NCPAINT, sent|wparam|defwinproc|optional, 1 },
    { WM_ERASEBKGND, sent|defwinproc|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOCLIENTMOVE|SWP_NOCLIENTSIZE|SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE },
    { 0 }
};
/* CreateWindow(WS_MAXIMIZE) for popup window, not initially visible */
static const struct message WmCreateInvisibleMaxPopupSeq[] = {
    { HCBT_CREATEWND, hook },
    { WM_NCCREATE, sent },
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { WM_CREATE, sent },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },
    { WM_MOVE, sent },
    { HCBT_MINMAX, hook|lparam, 0, SW_MAXIMIZE },
    { WM_GETMINMAXINFO, sent },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_STATECHANGED  },
    { WM_NCCALCSIZE, sent|wparam, TRUE },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOREDRAW|SWP_STATECHANGED },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_MAXIMIZED },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { 0 }
};
/* ShowWindow(SW_SHOWMAXIMIZED) for a resized not visible popup window */
static const struct message WmShowMaxPopupResizedSeq[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_MAXIMIZE },
    { WM_GETMINMAXINFO, sent },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED },
    { WM_NCCALCSIZE, sent|wparam, TRUE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { HCBT_ACTIVATE, hook },
    { EVENT_SYSTEM_FOREGROUND, winevent_hook|wparam|lparam, 0, 0 },
    { WM_QUERYNEWPALETTE, sent|wparam|lparam|optional, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCPAINT, sent|wparam|optional, 1 },
    { WM_ERASEBKGND, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_ACTIVATEAPP, sent|wparam, 1 },
    { WM_NCACTIVATE, sent },
    { WM_ACTIVATE, sent|wparam, 1 },
    { HCBT_SETFOCUS, hook },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|wparam|defwinproc, 0 },
    { WM_GETTEXT, sent|optional },
    { WM_NCPAINT, sent|wparam|optional, 1 },
    { WM_ERASEBKGND, sent|optional },
    { WM_WINDOWPOSCHANGED, sent },
    /* WinNT4.0 sends WM_MOVE */
    { WM_MOVE, sent|defwinproc|optional },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_MAXIMIZED },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { 0 }
};
/* ShowWindow(SW_SHOWMAXIMIZED) for a not visible popup window */
static const struct message WmShowMaxPopupSeq[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_MAXIMIZE },
    { WM_GETMINMAXINFO, sent },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED },
    { WM_NCCALCSIZE, sent|wparam, TRUE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { HCBT_ACTIVATE, hook },
    { EVENT_SYSTEM_FOREGROUND, winevent_hook|wparam|lparam, 0, 0 },
    { WM_QUERYNEWPALETTE, sent|wparam|lparam|optional, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE },
    { WM_ACTIVATEAPP, sent|wparam, 1 },
    { WM_NCACTIVATE, sent },
    { WM_ACTIVATE, sent|wparam, 1 },
    { HCBT_SETFOCUS, hook },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|wparam|defwinproc, 0 },
    { WM_GETTEXT, sent|optional },
    { WM_SYNCPAINT, sent|wparam|optional, 4 },
    { WM_NCPAINT, sent|wparam|optional, 1 },
    { WM_ERASEBKGND, sent|optional },
    { WM_NCPAINT, sent|wparam|defwinproc|optional, 1 },
    { WM_ERASEBKGND, sent|defwinproc|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE|SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOSIZE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { 0 }
};
/* CreateWindow(WS_VISIBLE) for popup window */
static const struct message WmCreatePopupSeq[] = {
    { HCBT_CREATEWND, hook },
    { WM_NCCREATE, sent },
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { WM_CREATE, sent },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },
    { WM_MOVE, sent },
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { HCBT_ACTIVATE, hook },
    { EVENT_SYSTEM_FOREGROUND, winevent_hook|wparam|lparam, 0, 0 },
    { WM_QUERYNEWPALETTE, sent|wparam|lparam|optional, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCPAINT, sent|wparam|optional, 1 },
    { WM_ERASEBKGND, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_ACTIVATEAPP, sent|wparam, 1 },
    { WM_NCACTIVATE, sent },
    { WM_ACTIVATE, sent|wparam, 1 },
    { HCBT_SETFOCUS, hook },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|wparam|defwinproc, 0 },
    { WM_GETTEXT, sent|optional },
    { WM_SYNCPAINT, sent|wparam|optional, 4 },
    { WM_NCPAINT, sent|wparam|optional, 1 },
    { WM_ERASEBKGND, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOCLIENTMOVE|SWP_NOCLIENTSIZE|SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE },
    { 0 }
};
/* ShowWindow(SW_SHOWMAXIMIZED) for a visible popup window */
static const struct message WmShowVisMaxPopupSeq[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_MAXIMIZE },
    { WM_GETMINMAXINFO, sent },
    { WM_GETTEXT, sent|optional },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_STATECHANGED },
    { WM_GETTEXT, sent|optional },
    { WM_NCCALCSIZE, sent|wparam, TRUE },
    { WM_QUERYNEWPALETTE, sent|wparam|lparam|optional, 0, 0 },
    { WM_NCPAINT, sent|wparam|optional, 1 },
    { WM_ERASEBKGND, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_STATECHANGED },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_MAXIMIZED },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { 0 }
};
/* CreateWindow (for a child popup window, not initially visible) */
static const struct message WmCreateChildPopupSeq[] = {
    { HCBT_CREATEWND, hook },
    { WM_NCCREATE, sent }, 
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { WM_CREATE, sent },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },
    { WM_MOVE, sent },
    { 0 }
};
/* CreateWindow (for a popup window, not initially visible,
 * which sets WS_VISIBLE in WM_CREATE handler)
 */
static const struct message WmCreateInvisiblePopupSeq[] = {
    { HCBT_CREATEWND, hook },
    { WM_NCCREATE, sent }, 
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { WM_CREATE, sent },
    { WM_STYLECHANGING, sent },
    { WM_STYLECHANGED, sent },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },
    { WM_MOVE, sent },
    { 0 }
};
/* SetWindowPos(SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER)
 * for a popup window with WS_VISIBLE style set
 */
static const struct message WmShowVisiblePopupSeq_2[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { 0 }
};
/* SetWindowPos(SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE)
 * for a popup window with WS_VISIBLE style set
 */
static const struct message WmShowVisiblePopupSeq_3[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE },
    { HCBT_ACTIVATE, hook },
    { EVENT_SYSTEM_FOREGROUND, winevent_hook|wparam|lparam, 0, 0 },
    { WM_QUERYNEWPALETTE, sent|wparam|lparam|optional, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCACTIVATE, sent },
    { WM_ACTIVATE, sent|wparam, 1 },
    { HCBT_SETFOCUS, hook },
    { WM_KILLFOCUS, sent|parent },
    { WM_IME_SETCONTEXT, sent|parent|wparam|optional, 0 },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|defwinproc },
    { WM_GETTEXT, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE, 0, SWP_SHOWWINDOW },
    { 0 }
};
/* CreateWindow (for child window, not initially visible) */
static const struct message WmCreateChildSeq[] = {
    { HCBT_CREATEWND, hook },
    { WM_NCCREATE, sent }, 
    /* child is inserted into parent's child list after WM_NCCREATE returns */
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { WM_CREATE, sent },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },
    { WM_MOVE, sent },
    { WM_PARENTNOTIFY, sent|parent|wparam, WM_CREATE },
    { 0 }
};
/* CreateWindow (for maximized child window, not initially visible) */
static const struct message WmCreateMaximizedChildSeq[] = {
    { HCBT_CREATEWND, hook },
    { WM_NCCREATE, sent }, 
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { WM_CREATE, sent },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },
    { WM_MOVE, sent },
    { HCBT_MINMAX, hook|lparam, 0, SW_MAXIMIZE },
    { WM_GETMINMAXINFO, sent },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_STATECHANGED },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOCLIENTMOVE|SWP_STATECHANGED },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_MAXIMIZED },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_PARENTNOTIFY, sent|parent|wparam, WM_CREATE },
    { 0 }
};
/* CreateWindow (for a child window, initially visible) */
static const struct message WmCreateVisibleChildSeq[] = {
    { HCBT_CREATEWND, hook },
    { WM_NCCREATE, sent }, 
    /* child is inserted into parent's child list after WM_NCCREATE returns */
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { EVENT_OBJECT_REORDER, winevent_hook|wparam|lparam, 0, 0 },
    { WM_CREATE, sent },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },
    { WM_MOVE, sent },
    { WM_PARENTNOTIFY, sent|parent|wparam, WM_CREATE },
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { WM_ERASEBKGND, sent|parent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_NCCALCSIZE, sent|wparam|optional, 1 }, /* WinXP */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { 0 }
};
/* ShowWindow(SW_SHOW) for a not visible child window */
static const struct message WmShowChildSeq[] = {
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { WM_ERASEBKGND, sent|parent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { 0 }
};
/* ShowWindow(SW_HIDE) for a visible child window */
static const struct message WmHideChildSeq[] = {
    { WM_SHOWWINDOW, sent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_HIDE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_ERASEBKGND, sent|parent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { 0 }
};
/* ShowWindow(SW_HIDE) for a visible child window checking all parent events*/
static const struct message WmHideChildSeq2[] = {
    { WM_SHOWWINDOW, sent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_HIDE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_ERASEBKGND, sent|parent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { 0 }
};
/* SetWindowPos(SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE)
 * for a not visible child window
 */
static const struct message WmShowChildSeq_2[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { WM_CHILDACTIVATE, sent },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { 0 }
};
/* SetWindowPos(SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE)
 * for a not visible child window
 */
static const struct message WmShowChildSeq_3[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { 0 }
};
/* SetWindowPos(SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE)
 * for a visible child window with a caption
 */
static const struct message WmShowChildSeq_4[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE },
    { WM_CHILDACTIVATE, sent },
    { 0 }
};
/* ShowWindow(SW_MINIMIZE) for child with invisible parent */
static const struct message WmShowChildInvisibleParentSeq_1[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_MINIMIZE },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_NOCOPYBITS|SWP_STATECHANGED, 0, SWP_NOACTIVATE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { WM_CHILDACTIVATE, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_NOREDRAW|SWP_NOCOPYBITS|SWP_STATECHANGED, 0, SWP_NOACTIVATE },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc|wparam|lparam, SIZE_MINIMIZED, 0 },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { EVENT_SYSTEM_MINIMIZESTART, winevent_hook|wparam|lparam, 0, 0 },
    /* FIXME: Wine creates an icon/title window while Windows doesn't */
    { WM_PARENTNOTIFY, sent|parent|wparam|optional, WM_CREATE },
    { WM_GETTEXT, sent|optional },
    { 0 }
};
/* repeated ShowWindow(SW_MINIMIZE) for child with invisible parent */
static const struct message WmShowChildInvisibleParentSeq_1r[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_MINIMIZE },
    { 0 }
};
/* ShowWindow(SW_MAXIMIZE) for child with invisible parent */
static const struct message WmShowChildInvisibleParentSeq_2[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_MAXIMIZE },
    { WM_GETMINMAXINFO, sent },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_STATECHANGED },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { WM_CHILDACTIVATE, sent },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOCLIENTMOVE|SWP_STATECHANGED },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_MAXIMIZED },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { 0 }
};
/* repeated ShowWindow(SW_MAXIMIZE) for child with invisible parent */
static const struct message WmShowChildInvisibleParentSeq_2r[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_MAXIMIZE },
    { 0 }
};
/* ShowWindow(SW_SHOWMINIMIZED) for child with invisible parent */
static const struct message WmShowChildInvisibleParentSeq_3[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_SHOWMINIMIZED },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_NOCOPYBITS|SWP_STATECHANGED },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { WM_CHILDACTIVATE, sent },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_NOREDRAW|SWP_NOCOPYBITS|SWP_STATECHANGED },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc|wparam|lparam, SIZE_MINIMIZED, 0 },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { EVENT_SYSTEM_MINIMIZESTART, winevent_hook|wparam|lparam, 0, 0 },
    /* FIXME: Wine creates an icon/title window while Windows doesn't */
    { WM_PARENTNOTIFY, sent|parent|wparam|optional, WM_CREATE },
    { WM_GETTEXT, sent|optional },
    { 0 }
};
/* repeated ShowWindow(SW_SHOWMINIMIZED) for child with invisible parent */
static const struct message WmShowChildInvisibleParentSeq_3r[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_SHOWMINIMIZED },
    { 0 }
};
/* ShowWindow(SW_SHOWMINNOACTIVE) for child with invisible parent */
static const struct message WmShowChildInvisibleParentSeq_4[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_SHOWMINNOACTIVE },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_STATECHANGED },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOREDRAW|SWP_NOCOPYBITS|SWP_STATECHANGED },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc|wparam|lparam, SIZE_MINIMIZED, 0 },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { EVENT_SYSTEM_MINIMIZESTART, winevent_hook|wparam|lparam, 0, 0 },
    /* FIXME: Wine creates an icon/title window while Windows doesn't */
    { WM_PARENTNOTIFY, sent|parent|wparam|optional, WM_CREATE },
    { WM_GETTEXT, sent|optional },
    { 0 }
};
/* repeated ShowWindow(SW_SHOWMINNOACTIVE) for child with invisible parent */
static const struct message WmShowChildInvisibleParentSeq_4r[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_SHOWMINNOACTIVE },
    { 0 }
};
/* ShowWindow(SW_SHOW) for child with invisible parent */
static const struct message WmShowChildInvisibleParentSeq_5[] = {
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { 0 }
};
/* ShowWindow(SW_HIDE) for child with invisible parent */
static const struct message WmHideChildInvisibleParentSeq[] = {
    { WM_SHOWWINDOW, sent|wparam, 0 },
    { 0 }
};
/* SetWindowPos(SWP_SHOWWINDOW) for child with invisible parent */
static const struct message WmShowChildInvisibleParentSeq_6[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOREDRAW|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { 0 }
};
/* SetWindowPos(SWP_HIDEWINDOW) for child with invisible parent */
static const struct message WmHideChildInvisibleParentSeq_2[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_HIDE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { 0 }
};
/* DestroyWindow for a visible child window */
static const struct message WmDestroyChildSeq[] = {
    { HCBT_DESTROYWND, hook },
    { 0x0090, sent|optional },
    { WM_PARENTNOTIFY, sent|parent|wparam, WM_DESTROY },
    { WM_SHOWWINDOW, sent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_HIDE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_ERASEBKGND, sent|parent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { HCBT_SETFOCUS, hook }, /* set focus to a parent */
    { WM_KILLFOCUS, sent },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 },
    { WM_IME_SETCONTEXT, sent|wparam|parent|optional, 1 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|parent },
    { EVENT_OBJECT_DESTROY, winevent_hook|wparam|lparam, 0, 0 },
    { WM_DESTROY, sent },
    { WM_DESTROY, sent|optional }, /* some other (IME?) window */
    { WM_NCDESTROY, sent|optional }, /* some other (IME?) window */
    { WM_NCDESTROY, sent },
    { 0 }
};
/* visible child window destroyed by thread exit */
static const struct message WmExitThreadSeq[] = {
    { WM_NCDESTROY, sent },  /* actually in grandchild */
    { WM_PAINT, sent|parent },
    { WM_ERASEBKGND, sent|parent|beginpaint },
    { 0 }
};
/* DestroyWindow for a visible child window with invisible parent */
static const struct message WmDestroyInvisibleChildSeq[] = {
    { HCBT_DESTROYWND, hook },
    { 0x0090, sent|optional },
    { WM_PARENTNOTIFY, sent|parent|wparam, WM_DESTROY },
    { WM_SHOWWINDOW, sent|wparam, 0 },
    { EVENT_OBJECT_DESTROY, winevent_hook|wparam|lparam, 0, 0 },
    { WM_DESTROY, sent },
    { WM_NCDESTROY, sent },
    { 0 }
};
/* Moving the mouse in nonclient area */
static const struct message WmMouseMoveInNonClientAreaSeq[] = { /* FIXME: add */
    { WM_NCHITTEST, sent },
    { WM_SETCURSOR, sent },
    { WM_NCMOUSEMOVE, posted },
    { 0 }
};
/* Moving the mouse in client area */
static const struct message WmMouseMoveInClientAreaSeq[] = { /* FIXME: add */
    { WM_NCHITTEST, sent },
    { WM_SETCURSOR, sent },
    { WM_MOUSEMOVE, posted },
    { 0 }
};
/* Moving by dragging the title bar (after WM_NCHITTEST and WM_SETCURSOR) (outline move) */
static const struct message WmDragTitleBarSeq[] = { /* FIXME: add */
    { WM_NCLBUTTONDOWN, sent|wparam, HTCAPTION },
    { WM_SYSCOMMAND, sent|defwinproc|wparam, SC_MOVE+2 },
    { WM_GETMINMAXINFO, sent|defwinproc },
    { WM_ENTERSIZEMOVE, sent|defwinproc },
    { WM_WINDOWPOSCHANGING, sent|wparam|defwinproc, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam|defwinproc, 0 },
    { WM_MOVE, sent|defwinproc },
    { WM_EXITSIZEMOVE, sent|defwinproc },
    { 0 }
};
/* Sizing by dragging the thick borders (after WM_NCHITTEST and WM_SETCURSOR) (outline move) */
static const struct message WmDragThickBordersBarSeq[] = { /* FIXME: add */
    { WM_NCLBUTTONDOWN, sent|wparam, 0xd },
    { WM_SYSCOMMAND, sent|defwinproc|wparam, 0xf004 },
    { WM_GETMINMAXINFO, sent|defwinproc },
    { WM_ENTERSIZEMOVE, sent|defwinproc },
    { WM_SIZING, sent|defwinproc|wparam, 4}, /* one for each mouse movement */
    { WM_WINDOWPOSCHANGING, sent|wparam|defwinproc, 0 },
    { WM_GETMINMAXINFO, sent|defwinproc },
    { WM_NCCALCSIZE, sent|defwinproc|wparam, 1 },
    { WM_NCPAINT, sent|defwinproc|wparam, 1 },
    { WM_GETTEXT, sent|defwinproc },
    { WM_ERASEBKGND, sent|defwinproc },
    { WM_WINDOWPOSCHANGED, sent|wparam|defwinproc, 0 },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_RESTORED },
    { WM_EXITSIZEMOVE, sent|defwinproc },
    { 0 }
};
/* Resizing child window with MoveWindow (32) */
static const struct message WmResizingChildWithMoveWindowSeq[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOACTIVATE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_ERASEBKGND, sent|parent|optional },
    { WM_ERASEBKGND, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOACTIVATE },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_RESTORED },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { 0 }
};
/* Clicking on inactive button */
static const struct message WmClickInactiveButtonSeq[] = { /* FIXME: add */
    { WM_NCHITTEST, sent },
    { WM_PARENTNOTIFY, sent|parent|wparam, WM_LBUTTONDOWN },
    { WM_MOUSEACTIVATE, sent },
    { WM_MOUSEACTIVATE, sent|parent|defwinproc },
    { WM_SETCURSOR, sent },
    { WM_SETCURSOR, sent|parent|defwinproc },
    { WM_LBUTTONDOWN, posted },
    { WM_KILLFOCUS, posted|parent },
    { WM_SETFOCUS, posted },
    { WM_CTLCOLORBTN, posted|parent },
    { BM_SETSTATE, posted },
    { WM_CTLCOLORBTN, posted|parent },
    { WM_LBUTTONUP, posted },
    { BM_SETSTATE, posted },
    { WM_CTLCOLORBTN, posted|parent },
    { WM_COMMAND, posted|parent },
    { 0 }
};
/* Reparenting a button (16/32) */
/* The last child (button) reparented gets topmost for its new parent. */
static const struct message WmReparentButtonSeq[] = { /* FIXME: add */
    { WM_SHOWWINDOW, sent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE },
    { EVENT_OBJECT_HIDE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_ERASEBKGND, sent|parent },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOSIZE },
    { WM_CHILDACTIVATE, sent },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOSIZE|SWP_NOREDRAW },
    { WM_MOVE, sent|defwinproc },
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { 0 }
};
/* Creation of a custom dialog (32) */
static const struct message WmCreateCustomDialogSeq[] = {
    { HCBT_CREATEWND, hook },
    { WM_GETMINMAXINFO, sent },
    { WM_NCCREATE, sent },
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { EVENT_OBJECT_REORDER, winevent_hook|wparam|lparam, 0, 0 },
    { WM_CREATE, sent },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_NOTIFYFORMAT, sent|optional },
    { WM_QUERYUISTATE, sent|optional },
    { WM_WINDOWPOSCHANGING, sent|optional },
    { WM_GETMINMAXINFO, sent|optional },
    { WM_NCCALCSIZE, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|optional },
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { HCBT_ACTIVATE, hook },
    { EVENT_SYSTEM_FOREGROUND, winevent_hook|wparam|lparam, 0, 0 },


    { WM_QUERYNEWPALETTE, sent|wparam|lparam|optional, 0, 0 },

    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE },

    { WM_NCACTIVATE, sent },
    { WM_GETTEXT, sent|optional|defwinproc },
    { WM_GETTEXT, sent|optional|defwinproc },
    { WM_GETTEXT, sent|optional|defwinproc },
    { EVENT_OBJECT_DEFACTIONCHANGE, winevent_hook|wparam|lparam|optional, OBJID_CLIENT, 0 },
    { WM_ACTIVATE, sent|wparam, 1 },
    { WM_GETTEXT, sent|optional },
    { WM_KILLFOCUS, sent|parent },
    { WM_IME_SETCONTEXT, sent|parent|wparam|optional, 0 },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent },
    { WM_GETDLGCODE, sent|defwinproc|wparam, 0 },
    { WM_NCPAINT, sent|wparam, 1 },
    { WM_GETTEXT, sent|optional|defwinproc },
    { WM_GETTEXT, sent|optional|defwinproc },
    { WM_ERASEBKGND, sent },
    { WM_CTLCOLORDLG, sent|optional|defwinproc },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_GETTEXT, sent|optional },
    { WM_GETTEXT, sent|optional },
    { WM_NCCALCSIZE, sent|optional },
    { WM_NCPAINT, sent|optional },
    { WM_GETTEXT, sent|optional|defwinproc },
    { WM_GETTEXT, sent|optional|defwinproc },
    { WM_ERASEBKGND, sent|optional },
    { WM_CTLCOLORDLG, sent|optional|defwinproc },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },
    { WM_MOVE, sent },
    { 0 }
};
/* Calling EndDialog for a custom dialog (32) */
static const struct message WmEndCustomDialogSeq[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_HIDE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_GETTEXT, sent|optional },
    { HCBT_ACTIVATE, hook },
    { WM_NCACTIVATE, sent|wparam, 0 },
    { WM_GETTEXT, sent|optional|defwinproc },
    { WM_GETTEXT, sent|optional|defwinproc },
    { WM_ACTIVATE, sent|wparam, 0 },
    { EVENT_SYSTEM_FOREGROUND, winevent_hook|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_NOACTIVATE|SWP_NOREDRAW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_GETTEXT, sent|optional|defwinproc },
    { WM_GETTEXT, sent|optional|defwinproc },
    { HCBT_SETFOCUS, hook },
    { WM_KILLFOCUS, sent },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 },
    { WM_IME_SETCONTEXT, sent|parent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|optional, 1 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|parent|defwinproc },
    { 0 }
};
/* ShowWindow(SW_SHOW) for a custom dialog (initially invisible) */
static const struct message WmShowCustomDialogSeq[] = {
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { HCBT_ACTIVATE, hook },
    { EVENT_SYSTEM_FOREGROUND, winevent_hook|wparam|lparam, 0, 0 },

    { WM_QUERYNEWPALETTE, sent|wparam|lparam|optional, 0, 0 },

    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE },
    { WM_ACTIVATEAPP, sent|wparam|optional, 1 },
    { WM_NCACTIVATE, sent },
    { WM_ACTIVATE, sent|wparam, 1 },
    { WM_GETTEXT, sent|optional },

    { WM_KILLFOCUS, sent|parent },
    { WM_IME_SETCONTEXT, sent|parent|wparam|optional, 0 },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent },
    { WM_GETDLGCODE, sent|defwinproc|wparam, 0 },
    { WM_NCPAINT, sent|wparam, 1 },
    { WM_ERASEBKGND, sent },
    { WM_CTLCOLORDLG, sent|defwinproc },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { 0 }
};
/* Creation and destruction of a modal dialog (32) */
static const struct message WmModalDialogSeq[] = {
    { WM_CANCELMODE, sent|parent },
    { HCBT_SETFOCUS, hook },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_KILLFOCUS, sent|parent },
    { WM_IME_SETCONTEXT, sent|parent|wparam|optional, 0 },
    { EVENT_OBJECT_STATECHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_ENABLE, sent|parent|wparam, 0 },
    { HCBT_CREATEWND, hook },
    { EVENT_OBJECT_REORDER, winevent_hook|wparam|lparam, 0, 0 },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SETFONT, sent },
    { WM_INITDIALOG, sent },
    { WM_CHANGEUISTATE, sent|optional },
    { WM_UPDATEUISTATE, sent|optional },
    { WM_SHOWWINDOW, sent },
    { HCBT_ACTIVATE, hook },
    { EVENT_SYSTEM_FOREGROUND, winevent_hook|wparam|lparam, 0, 0 },
    { WM_QUERYNEWPALETTE, sent|wparam|lparam|optional, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCACTIVATE, sent },
    { WM_GETTEXT, sent|optional },
    { WM_ACTIVATE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { WM_NCPAINT, sent|optional },
    { WM_GETTEXT, sent|optional },
    { WM_ERASEBKGND, sent|optional },
    { WM_CTLCOLORDLG, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_GETTEXT, sent|optional },
    { WM_NCCALCSIZE, sent|optional },
    { WM_NCPAINT, sent|optional },
    { WM_GETTEXT, sent|optional },
    { WM_ERASEBKGND, sent|optional },
    { WM_CTLCOLORDLG, sent|optional },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_PAINT, sent|optional },
    { WM_CTLCOLORBTN, sent|optional },
    { WM_GETTITLEBARINFOEX, sent|optional },
    { WM_ENTERIDLE, sent|parent|optional },
    { WM_ENTERIDLE, sent|parent|optional },
    { WM_ENTERIDLE, sent|parent|optional },
    { WM_ENTERIDLE, sent|parent|optional },
    { WM_ENTERIDLE, sent|parent|optional },
    { WM_ENTERIDLE, sent|parent|optional },
    { WM_ENTERIDLE, sent|parent|optional },
    { WM_ENTERIDLE, sent|parent|optional },
    { WM_ENTERIDLE, sent|parent|optional },
    { WM_ENTERIDLE, sent|parent|optional },
    { WM_ENTERIDLE, sent|parent|optional },
    { WM_ENTERIDLE, sent|parent|optional },
    { WM_ENTERIDLE, sent|parent|optional },
    { WM_ENTERIDLE, sent|parent|optional },
    { WM_ENTERIDLE, sent|parent|optional },
    { WM_ENTERIDLE, sent|parent|optional },
    { WM_ENTERIDLE, sent|parent|optional },
    { WM_ENTERIDLE, sent|parent|optional },
    { WM_ENTERIDLE, sent|parent|optional },
    { WM_ENTERIDLE, sent|parent|optional },
    { WM_TIMER, sent },
    { EVENT_OBJECT_STATECHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_ENABLE, sent|parent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE },
    { EVENT_OBJECT_HIDE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_GETTEXT, sent|optional },
    { HCBT_ACTIVATE, hook },
    { WM_NCACTIVATE, sent|wparam, 0 },
    { WM_GETTEXT, sent|optional },
    { WM_ACTIVATE, sent|wparam, 0 },
    { EVENT_SYSTEM_FOREGROUND, winevent_hook|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|optional },
    { HCBT_SETFOCUS, hook },
    { WM_IME_SETCONTEXT, sent|parent|wparam|defwinproc|optional, 1 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|parent|defwinproc },
    { EVENT_SYSTEM_DIALOGEND, winevent_hook|wparam|lparam, 0, 0 },
    { HCBT_DESTROYWND, hook },
    { 0x0090, sent|optional },
    { EVENT_OBJECT_DESTROY, winevent_hook|wparam|lparam, 0, 0 },
    { WM_DESTROY, sent },
    { WM_NCDESTROY, sent },
    { 0 }
};
/* Creation of a modal dialog that is resized inside WM_INITDIALOG (32) */
static const struct message WmCreateModalDialogResizeSeq[] = { /* FIXME: add */
    /* (inside dialog proc, handling WM_INITDIALOG) */
    { WM_WINDOWPOSCHANGING, sent|wparam, 0 },
    { WM_NCCALCSIZE, sent },
    { WM_NCACTIVATE, sent|parent|wparam, 0 },
    { WM_GETTEXT, sent|defwinproc },
    { WM_ACTIVATE, sent|parent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent|parent },
    { WM_NCACTIVATE, sent|wparam, 1 },
    { WM_ACTIVATE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, 0 },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_RESTORED },
    /* (setting focus) */
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, 0 },
    { WM_NCPAINT, sent },
    { WM_GETTEXT, sent|defwinproc },
    { WM_ERASEBKGND, sent },
    { WM_CTLCOLORDLG, sent|defwinproc },
    { WM_WINDOWPOSCHANGED, sent|wparam, 0 },
    { WM_PAINT, sent },
    /* (bunch of WM_CTLCOLOR* for each control) */
    { WM_PAINT, sent|parent },
    { WM_ENTERIDLE, sent|parent|wparam, 0 },
    { WM_SETCURSOR, sent|parent },
    { 0 }
};
/* SetMenu for NonVisible windows with size change*/
static const struct message WmSetMenuNonVisibleSizeChangeSeq[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { EVENT_OBJECT_REORDER, winevent_hook|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOREDRAW },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_RESTORED },
    { WM_NCCALCSIZE,sent|wparam|optional, 1 }, /* XP */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam|optional, 0, 0 }, /* XP sends a duplicate */
    { WM_GETTEXT, sent|optional },
    { WM_NCCALCSIZE, sent|wparam|optional, 1 },
    { 0 }
};
/* SetMenu for NonVisible windows with no size change */
static const struct message WmSetMenuNonVisibleNoSizeChangeSeq[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { 0 }
};
/* SetMenu for Visible windows with size change */
static const struct message WmSetMenuVisibleSizeChangeSeq[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { 0x0093, sent|defwinproc|optional },
    { EVENT_OBJECT_REORDER, winevent_hook|wparam|lparam, 0, 0 },
    { WM_NCPAINT, sent|optional }, /* wparam != 1 */
    { 0x0093, sent|defwinproc|optional },
    { 0x0093, sent|defwinproc|optional },
    { 0x0091, sent|defwinproc|optional },
    { 0x0092, sent|defwinproc|optional },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    { WM_ACTIVATE, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_RESTORED },
    { 0x0093, sent|optional },
    { WM_NCCALCSIZE, sent|wparam|optional, 1 },
    { 0x0093, sent|defwinproc|optional },
    { WM_NCPAINT, sent|optional }, /* wparam != 1 */
    { 0x0093, sent|defwinproc|optional },
    { 0x0093, sent|defwinproc|optional },
    { 0x0091, sent|defwinproc|optional },
    { 0x0092, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam|optional, 0, 0 }, /* XP sends a duplicate */
    { 0 }
};
/* SetMenu for Visible windows with no size change */
static const struct message WmSetMenuVisibleNoSizeChangeSeq[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_NCPAINT, sent|optional }, /* wparam != 1 */
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    { WM_ACTIVATE, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { 0 }
};
/* DrawMenuBar for a visible window */
static const struct message WmDrawMenuBarSeq[] =
{
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { 0x0093, sent|defwinproc|optional },
    { WM_NCPAINT, sent|optional }, /* wparam != 1 */
    { 0x0093, sent|defwinproc|optional },
    { 0x0093, sent|defwinproc|optional },
    { 0x0091, sent|defwinproc|optional },
    { 0x0092, sent|defwinproc|optional },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { 0x0093, sent|optional },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message WmSetRedrawFalseSeq[] =
{
    { WM_SETREDRAW, sent|wparam, 0 },
    { 0 }
};

static const struct message WmSetRedrawTrueSeq[] =
{
    { WM_SETREDRAW, sent|wparam, 1 },
    { 0 }
};

static const struct message WmEnableWindowSeq_1[] =
{
    { WM_CANCELMODE, sent|wparam|lparam, 0, 0 },
    { EVENT_OBJECT_STATECHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { HCBT_SETFOCUS, hook|optional },
    { WM_KILLFOCUS, sent|optional },
    { WM_ENABLE, sent|wparam|lparam, FALSE, 0 },
    { 0 }
};

static const struct message WmEnableWindowSeq_2[] =
{
    { EVENT_OBJECT_STATECHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_ENABLE, sent|wparam|lparam, TRUE, 0 },
    { 0 }
};

static const struct message WmGetScrollRangeSeq[] =
{
    { SBM_GETRANGE, sent },
    { 0 }
};
static const struct message WmGetScrollInfoSeq[] =
{
    { SBM_GETSCROLLINFO, sent },
    { 0 }
};
static const struct message WmSetScrollRangeSeq[] =
{
    /* MSDN claims that Windows sends SBM_SETRANGE message, but win2k SP4
       sends SBM_SETSCROLLINFO.
     */
    { SBM_SETSCROLLINFO, sent },
    { 0 }
};
/* SetScrollRange for a window without a non-client area */
static const struct message WmSetScrollRangeHSeq_empty[] =
{
    { EVENT_OBJECT_VALUECHANGE, winevent_hook|wparam|lparam, OBJID_HSCROLL, 0 },
    { 0 }
};
static const struct message WmSetScrollRangeVSeq_empty[] =
{
    { EVENT_OBJECT_VALUECHANGE, winevent_hook|wparam|lparam, OBJID_VSCROLL, 0 },
    { 0 }
};
static const struct message WmSetScrollRangeHVSeq[] =
{
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { EVENT_OBJECT_VALUECHANGE, winevent_hook|lparam|optional, 0/*OBJID_HSCROLL or OBJID_VSCROLL*/, 0 },
    { 0 }
};
/* SetScrollRange for a window with a non-client area */
static const struct message WmSetScrollRangeHV_NC_Seq[] =
{
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { EVENT_OBJECT_REORDER, winevent_hook|wparam|lparam, 0, 0 },
    { WM_NCPAINT, sent|optional },
    { WM_STYLECHANGING, sent|defwinproc|optional },
    { WM_STYLECHANGED, sent|defwinproc|optional },
    { WM_STYLECHANGING, sent|defwinproc|optional },
    { WM_STYLECHANGED, sent|defwinproc|optional },
    { WM_STYLECHANGING, sent|defwinproc|optional },
    { WM_STYLECHANGED, sent|defwinproc|optional },
    { WM_STYLECHANGING, sent|defwinproc|optional },
    { WM_STYLECHANGED, sent|defwinproc|optional },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    { WM_CTLCOLORDLG, sent|defwinproc|optional }, /* sent to a parent of the dialog */
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE|SWP_NOCLIENTMOVE, 0, SWP_NOCLIENTSIZE },
    { WM_SIZE, sent|defwinproc|optional },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { EVENT_OBJECT_VALUECHANGE, winevent_hook|lparam|optional, 0/*OBJID_HSCROLL or OBJID_VSCROLL*/, 0 },
    { WM_GETTEXT, sent|optional },
    { WM_GETTEXT, sent|optional },
    { WM_GETTEXT, sent|optional },
    { WM_GETTEXT, sent|optional },
    { 0 }
};
/* test if we receive the right sequence of messages */
/* after calling ShowWindow( SW_SHOWNA) */
static const struct message WmSHOWNAChildInvisParInvis[] = {
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { 0 }
};
static const struct message WmSHOWNAChildVisParInvis[] = {
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { 0 }
};
static const struct message WmSHOWNAChildVisParVis[] = {
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE },
    { 0 }
};
static const struct message WmSHOWNAChildInvisParVis[] = {
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { WM_ERASEBKGND, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOACTIVATE|SWP_NOCLIENTMOVE },
    { 0 }
};
static const struct message WmSHOWNATopVisible[] = {
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE },
    { WM_NCPAINT, sent|wparam|optional, 1 },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|optional },
    { 0 }
};
static const struct message WmSHOWNATopInvisible[] = {
    { WM_NOTIFYFORMAT, sent|optional },
    { WM_QUERYUISTATE, sent|optional },
    { WM_WINDOWPOSCHANGING, sent|optional },
    { WM_GETMINMAXINFO, sent|optional },
    { WM_NCCALCSIZE, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|optional },
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { WM_NCPAINT, sent|wparam|optional, 1 },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_NCCALCSIZE, sent|wparam|optional, 1 },
    { WM_NCPAINT, sent|wparam|optional, 1 },
    { WM_ERASEBKGND, sent|optional },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },
    { WM_MOVE, sent },
    { 0 }
};

static const struct message WmTrackPopupMenu[] = {
    { HCBT_CREATEWND, hook },
    { WM_ENTERMENULOOP, sent|wparam|lparam, TRUE, 0 },
    { WM_INITMENU, sent|lparam, 0, 0 },
    { WM_INITMENUPOPUP, sent|lparam, 0, 0 },
    { 0x0093, sent|optional },
    { 0x0094, sent|optional },
    { 0x0094, sent|optional },
    { WM_ENTERIDLE, sent|wparam, 2 },
    { WM_CAPTURECHANGED, sent },
    { HCBT_DESTROYWND, hook },
    { WM_UNINITMENUPOPUP, sent|lparam, 0, 0 },
    { WM_MENUSELECT, sent|wparam|lparam, 0xffff0000, 0 },
    { WM_EXITMENULOOP, sent|wparam|lparam, 1, 0 },
    { 0 }
};

static const struct message WmTrackPopupMenuCapture[] = {
    { HCBT_CREATEWND, hook },
    { WM_ENTERMENULOOP, sent|wparam|lparam, TRUE, 0 },
    { WM_CAPTURECHANGED, sent },
    { WM_INITMENU, sent|lparam, 0, 0 },
    { WM_INITMENUPOPUP, sent|lparam, 0, 0 },
    { 0x0093, sent|optional },
    { 0x0094, sent|optional },
    { 0x0094, sent|optional },
    { WM_ENTERIDLE, sent|wparam, 2 },
    { WM_CAPTURECHANGED, sent },
    { HCBT_DESTROYWND, hook },
    { WM_UNINITMENUPOPUP, sent|lparam, 0, 0 },
    { WM_MENUSELECT, sent|wparam|lparam, 0xffff0000, 0 },
    { WM_EXITMENULOOP, sent|wparam|lparam, 1, 0 },
    { 0 }
};

static const struct message WmTrackPopupMenuEmpty[] = {
    { HCBT_CREATEWND, hook },
    { WM_ENTERMENULOOP, sent|wparam|lparam, TRUE, 0 },
    { WM_INITMENU, sent|lparam, 0, 0 },
    { WM_INITMENUPOPUP, sent|lparam, 0, 0 },
    { 0x0093, sent|optional },
    { 0x0094, sent|optional },
    { 0x0094, sent|optional },
    { WM_CAPTURECHANGED, sent },
    { WM_EXITMENULOOP, sent|wparam|lparam, 1, 0 },
    { HCBT_DESTROYWND, hook },
    { WM_UNINITMENUPOPUP, sent|lparam, 0, 0 },
    { 0 }
};

static const struct message WmTrackPopupMenuAbort[] = {
    { HCBT_CREATEWND, hook },
    { WM_ENTERMENULOOP, sent|wparam|lparam, TRUE, 0 },
    { WM_INITMENU, sent|lparam, 0, 0 },
    { WM_INITMENUPOPUP, sent|lparam, 0, 0 },
    { 0x0093, sent|optional },
    { 0x0094, sent|optional },
    { 0x0094, sent|optional },
    { WM_CAPTURECHANGED, sent },
    { HCBT_DESTROYWND, hook },
    { WM_UNINITMENUPOPUP, sent|lparam, 0, 0 },
    { WM_MENUSELECT, sent|wparam|lparam, 0xffff0000, 0 },
    { WM_EXITMENULOOP, sent|wparam|lparam, 1, 0 },
    { 0 }
};

static BOOL after_end_dialog, test_def_id, paint_loop_done;
static int sequence_cnt, sequence_size;
static struct recvd_message* sequence;
static int log_all_parent_messages;
static CRITICAL_SECTION sequence_cs;

/* user32 functions */
static HWND (WINAPI *pGetAncestor)(HWND,UINT);
static BOOL (WINAPI *pGetMenuInfo)(HMENU,LPCMENUINFO);
static void (WINAPI *pNotifyWinEvent)(DWORD, HWND, LONG, LONG);
static BOOL (WINAPI *pSetMenuInfo)(HMENU,LPCMENUINFO);
static HWINEVENTHOOK (WINAPI *pSetWinEventHook)(DWORD, DWORD, HMODULE, WINEVENTPROC, DWORD, DWORD, DWORD);
static BOOL (WINAPI *pTrackMouseEvent)(TRACKMOUSEEVENT*);
static BOOL (WINAPI *pUnhookWinEvent)(HWINEVENTHOOK);
static BOOL (WINAPI *pGetMonitorInfoA)(HMONITOR,LPMONITORINFO);
static HMONITOR (WINAPI *pMonitorFromPoint)(POINT,DWORD);
static BOOL (WINAPI *pUpdateLayeredWindow)(HWND,HDC,POINT*,SIZE*,HDC,POINT*,COLORREF,BLENDFUNCTION*,DWORD);
static UINT_PTR (WINAPI *pSetSystemTimer)(HWND, UINT_PTR, UINT, TIMERPROC);
static UINT_PTR (WINAPI *pKillSystemTimer)(HWND, UINT_PTR);
/* kernel32 functions */
static BOOL (WINAPI *pGetCPInfoExA)(UINT, DWORD, LPCPINFOEXA);

static void init_procs(void)
{
    HMODULE user32 = GetModuleHandleA("user32.dll");
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");

#define GET_PROC(dll, func) \
    p ## func = (void*)GetProcAddress(dll, #func); \
    if(!p ## func) { \
      trace("GetProcAddress(%s) failed\n", #func); \
    }

    GET_PROC(user32, GetAncestor)
    GET_PROC(user32, GetMenuInfo)
    GET_PROC(user32, NotifyWinEvent)
    GET_PROC(user32, SetMenuInfo)
    GET_PROC(user32, SetWinEventHook)
    GET_PROC(user32, TrackMouseEvent)
    GET_PROC(user32, UnhookWinEvent)
    GET_PROC(user32, GetMonitorInfoA)
    GET_PROC(user32, MonitorFromPoint)
    GET_PROC(user32, UpdateLayeredWindow)
    GET_PROC(user32, SetSystemTimer)
    GET_PROC(user32, KillSystemTimer)

    GET_PROC(kernel32, GetCPInfoExA)

#undef GET_PROC
}

static const char *get_winpos_flags(UINT flags)
{
    static char buffer[300];

    buffer[0] = 0;
#define DUMP(flag) do { if (flags & flag) { strcat( buffer, "|" #flag ); flags &= ~flag; } } while(0)
    DUMP( SWP_SHOWWINDOW );
    DUMP( SWP_HIDEWINDOW );
    DUMP( SWP_NOACTIVATE );
    DUMP( SWP_FRAMECHANGED );
    DUMP( SWP_NOCOPYBITS );
    DUMP( SWP_NOOWNERZORDER );
    DUMP( SWP_NOSENDCHANGING );
    DUMP( SWP_DEFERERASE );
    DUMP( SWP_ASYNCWINDOWPOS );
    DUMP( SWP_NOZORDER );
    DUMP( SWP_NOREDRAW );
    DUMP( SWP_NOSIZE );
    DUMP( SWP_NOMOVE );
    DUMP( SWP_NOCLIENTSIZE );
    DUMP( SWP_NOCLIENTMOVE );
    if (flags) sprintf(buffer + strlen(buffer),"|0x%04x", flags);
    return buffer + 1;
#undef DUMP
}

static BOOL ignore_message( UINT message )
{
    /* these are always ignored */
    return (message >= 0xc000 ||
            message == WM_GETICON ||
            message == WM_GETOBJECT ||
            message == WM_TIMECHANGE ||
            message == WM_DISPLAYCHANGE ||
            message == WM_DEVICECHANGE ||
            message == WM_DWMNCRENDERINGCHANGED);
}


#define add_message(msg) add_message_(__LINE__,msg);
static void add_message_(int line, const struct recvd_message *msg)
{
    struct recvd_message *seq;

    EnterCriticalSection( &sequence_cs );
    if (!sequence)
    {
	sequence_size = 10;
	sequence = HeapAlloc( GetProcessHeap(), 0, sequence_size * sizeof(*sequence) );
    }
    if (sequence_cnt == sequence_size) 
    {
	sequence_size *= 2;
	sequence = HeapReAlloc( GetProcessHeap(), 0, sequence, sequence_size * sizeof(*sequence) );
    }
    assert(sequence);

    seq = &sequence[sequence_cnt++];
    seq->hwnd = msg->hwnd;
    seq->message = msg->message;
    seq->flags = msg->flags;
    seq->wParam = msg->wParam;
    seq->lParam = msg->lParam;
    seq->line   = line;
    seq->descr  = msg->descr;
    seq->output[0] = 0;
    LeaveCriticalSection( &sequence_cs );

    if (msg->descr)
    {
        if (msg->flags & hook)
        {
            static const char * const CBT_code_name[10] =
            {
                "HCBT_MOVESIZE",
                "HCBT_MINMAX",
                "HCBT_QS",
                "HCBT_CREATEWND",
                "HCBT_DESTROYWND",
                "HCBT_ACTIVATE",
                "HCBT_CLICKSKIPPED",
                "HCBT_KEYSKIPPED",
                "HCBT_SYSCOMMAND",
                "HCBT_SETFOCUS"
            };
            const char *code_name = (msg->message <= HCBT_SETFOCUS) ? CBT_code_name[msg->message] : "Unknown";

            sprintf( seq->output, "%s: hook %d (%s) wp %08lx lp %08lx",
                     msg->descr, msg->message, code_name, msg->wParam, msg->lParam );
        }
        else if (msg->flags & winevent_hook)
        {
            sprintf( seq->output, "%s: winevent %p %08x %08lx %08lx",
                     msg->descr, msg->hwnd, msg->message, msg->wParam, msg->lParam );
        }
        else
        {
            switch (msg->message)
            {
            case WM_WINDOWPOSCHANGING:
            case WM_WINDOWPOSCHANGED:
            {
                WINDOWPOS *winpos = (WINDOWPOS *)msg->lParam;

                sprintf( seq->output, "%s: %p WM_WINDOWPOS%s wp %08lx lp %08lx after %p x %d y %d cx %d cy %d flags %s",
                          msg->descr, msg->hwnd,
                          (msg->message == WM_WINDOWPOSCHANGING) ? "CHANGING" : "CHANGED",
                          msg->wParam, msg->lParam, winpos->hwndInsertAfter,
                          winpos->x, winpos->y, winpos->cx, winpos->cy,
                          get_winpos_flags(winpos->flags) );

                /* Log only documented flags, win2k uses 0x1000 and 0x2000
                 * in the high word for internal purposes
                 */
                seq->wParam = winpos->flags & 0xffff;
                /* We are not interested in the flags that don't match under XP and Win9x */
                seq->wParam &= ~SWP_NOZORDER;
                break;
            }

            case WM_DRAWITEM:
            {
                DRAW_ITEM_STRUCT di;
                DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)msg->lParam;

                sprintf( seq->output, "%s: %p WM_DRAWITEM: type %x, ctl_id %x, item_id %x, action %x, state %x",
                         msg->descr, msg->hwnd, dis->CtlType, dis->CtlID,
                         dis->itemID, dis->itemAction, dis->itemState);

                di.u.lp = 0;
                di.u.item.type = dis->CtlType;
                di.u.item.ctl_id = dis->CtlID;
                if (dis->CtlType == ODT_LISTBOX ||
                    dis->CtlType == ODT_COMBOBOX ||
                    dis->CtlType == ODT_MENU)
                    di.u.item.item_id = dis->itemID;
                di.u.item.action = dis->itemAction;
                di.u.item.state = dis->itemState;

                seq->lParam = di.u.lp;
                break;
            }
            default:
                if (msg->message >= 0xc000) return;  /* ignore registered messages */
                sprintf( seq->output, "%s: %p %04x wp %08lx lp %08lx",
                         msg->descr, msg->hwnd, msg->message, msg->wParam, msg->lParam );
            }
            if (msg->flags & (sent|posted|parent|defwinproc|beginpaint))
                sprintf( seq->output + strlen(seq->output), " (flags %x)", msg->flags );
        }
    }
}

/* try to make sure pending X events have been processed before continuing */
static void flush_events(void)
{
    MSG msg;
    int diff = 200;
    int min_timeout = 100;
    DWORD time = GetTickCount() + diff;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects( 0, NULL, FALSE, min_timeout, QS_ALLINPUT ) == WAIT_TIMEOUT) break;
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
        diff = time - GetTickCount();
    }
}

static void flush_sequence(void)
{
    EnterCriticalSection( &sequence_cs );
    HeapFree(GetProcessHeap(), 0, sequence);
    sequence = 0;
    sequence_cnt = sequence_size = 0;
    LeaveCriticalSection( &sequence_cs );
}

static void dump_sequence(const struct message *expected, const char *context, const char *file, int line)
{
    const struct recvd_message *actual = sequence;
    unsigned int count = 0;

    trace_(file, line)("Failed sequence %s:\n", context );
    while (expected->message && actual->message)
    {
        if (actual->output[0])
        {
            if (expected->flags & hook)
            {
                trace_(file, line)( "  %u: expected: hook %04x - actual: %s\n",
                                    count, expected->message, actual->output );
            }
            else if (expected->flags & winevent_hook)
            {
                trace_(file, line)( "  %u: expected: winevent %04x - actual: %s\n",
                                    count, expected->message, actual->output );
            }
            else if (expected->flags & kbd_hook)
            {
                trace_(file, line)( "  %u: expected: kbd %04x - actual: %s\n",
                                    count, expected->message, actual->output );
            }
            else
            {
                trace_(file, line)( "  %u: expected: msg %04x - actual: %s\n",
                                    count, expected->message, actual->output );
            }
        }

	if (expected->message == actual->message)
	{
	    if ((expected->flags & defwinproc) != (actual->flags & defwinproc) &&
                (expected->flags & optional))
            {
                /* don't match messages if their defwinproc status differs */
                expected++;
            }
            else
            {
                expected++;
                actual++;
            }
	}
	/* silently drop winevent messages if there is no support for them */
	else if ((expected->flags & optional) || ((expected->flags & winevent_hook) && !hEvent_hook))
	    expected++;
        else
        {
            expected++;
            actual++;
        }
        count++;
    }

    /* optional trailing messages */
    while (expected->message && ((expected->flags & optional) ||
	    ((expected->flags & winevent_hook) && !hEvent_hook)))
    {
        trace_(file, line)( "  %u: expected: msg %04x - actual: nothing\n", count, expected->message );
	expected++;
        count++;
    }

    if (expected->message)
    {
        trace_(file, line)( "  %u: expected: msg %04x - actual: nothing\n", count, expected->message );
        return;
    }

    while (actual->message && actual->output[0])
    {
        trace_(file, line)( "  %u: expected: nothing - actual: %s\n", count, actual->output );
        actual++;
        count++;
    }
}

#define ok_sequence( exp, contx, todo) \
        ok_sequence_( (exp), (contx), (todo), __FILE__, __LINE__)


static void ok_sequence_(const struct message *expected_list, const char *context, BOOL todo,
                         const char *file, int line)
{
    static const struct recvd_message end_of_sequence;
    const struct message *expected = expected_list;
    const struct recvd_message *actual;
    int failcount = 0, dump = 0;
    unsigned int count = 0;

    add_message(&end_of_sequence);

    actual = sequence;

    while (expected->message && actual->message)
    {
	if (expected->message == actual->message &&
            !((expected->flags ^ actual->flags) & (hook|winevent_hook|kbd_hook)))
	{
	    if (expected->flags & wparam)
	    {
		if (((expected->wParam ^ actual->wParam) & ~expected->wp_mask) && todo)
		{
		    todo_wine {
                        failcount ++;
                        if (strcmp(winetest_platform, "wine")) dump++;
                        ok_( file, line) (FALSE,
			    "%s: %u: in msg 0x%04x expecting wParam 0x%lx got 0x%lx\n",
                            context, count, expected->message, expected->wParam, actual->wParam);
		    }
		}
		else
                {
                    ok_( file, line)( ((expected->wParam ^ actual->wParam) & ~expected->wp_mask) == 0,
                                     "%s: %u: in msg 0x%04x expecting wParam 0x%lx got 0x%lx\n",
                                     context, count, expected->message, expected->wParam, actual->wParam);
                    if ((expected->wParam ^ actual->wParam) & ~expected->wp_mask) dump++;
                }

	    }
	    if (expected->flags & lparam)
            {
		if (((expected->lParam ^ actual->lParam) & ~expected->lp_mask) && todo)
		{
		    todo_wine {
                        failcount ++;
                        if (strcmp(winetest_platform, "wine")) dump++;
                        ok_( file, line) (FALSE,
			    "%s: %u: in msg 0x%04x expecting lParam 0x%lx got 0x%lx\n",
                            context, count, expected->message, expected->lParam, actual->lParam);
		    }
		}
		else
                {
                    ok_( file, line)(((expected->lParam ^ actual->lParam) & ~expected->lp_mask) == 0,
                                     "%s: %u: in msg 0x%04x expecting lParam 0x%lx got 0x%lx\n",
                                     context, count, expected->message, expected->lParam, actual->lParam);
                    if ((expected->lParam ^ actual->lParam) & ~expected->lp_mask) dump++;
                }
            }
	    if ((expected->flags & optional) &&
                ((expected->flags ^ actual->flags) & (defwinproc|parent)))
            {
                /* don't match optional messages if their defwinproc or parent status differs */
                expected++;
                count++;
                continue;
            }
	    if ((expected->flags & defwinproc) != (actual->flags & defwinproc) && todo)
	    {
		    todo_wine {
                        failcount ++;
                        if (strcmp(winetest_platform, "wine")) dump++;
                        ok_( file, line) (FALSE,
                            "%s: %u: the msg 0x%04x should %shave been sent by DefWindowProc\n",
                            context, count, expected->message, (expected->flags & defwinproc) ? "" : "NOT ");
		    }
	    }
	    else
            {
	        ok_( file, line) ((expected->flags & defwinproc) == (actual->flags & defwinproc),
		    "%s: %u: the msg 0x%04x should %shave been sent by DefWindowProc\n",
                    context, count, expected->message, (expected->flags & defwinproc) ? "" : "NOT ");
                if ((expected->flags & defwinproc) != (actual->flags & defwinproc)) dump++;
            }

	    ok_( file, line) ((expected->flags & beginpaint) == (actual->flags & beginpaint),
		"%s: %u: the msg 0x%04x should %shave been sent by BeginPaint\n",
                context, count, expected->message, (expected->flags & beginpaint) ? "" : "NOT ");
            if ((expected->flags & beginpaint) != (actual->flags & beginpaint)) dump++;

	    ok_( file, line) ((expected->flags & (sent|posted)) == (actual->flags & (sent|posted)),
		"%s: %u: the msg 0x%04x should have been %s\n",
                context, count, expected->message, (expected->flags & posted) ? "posted" : "sent");
            if ((expected->flags & (sent|posted)) != (actual->flags & (sent|posted))) dump++;

	    ok_( file, line) ((expected->flags & parent) == (actual->flags & parent),
		"%s: %u: the msg 0x%04x was expected in %s\n",
                context, count, expected->message, (expected->flags & parent) ? "parent" : "child");
            if ((expected->flags & parent) != (actual->flags & parent)) dump++;

	    ok_( file, line) ((expected->flags & hook) == (actual->flags & hook),
		"%s: %u: the msg 0x%04x should have been sent by a hook\n",
                context, count, expected->message);
            if ((expected->flags & hook) != (actual->flags & hook)) dump++;

	    ok_( file, line) ((expected->flags & winevent_hook) == (actual->flags & winevent_hook),
		"%s: %u: the msg 0x%04x should have been sent by a winevent hook\n",
                context, count, expected->message);
            if ((expected->flags & winevent_hook) != (actual->flags & winevent_hook)) dump++;

	    ok_( file, line) ((expected->flags & kbd_hook) == (actual->flags & kbd_hook),
		"%s: %u: the msg 0x%04x should have been sent by a keyboard hook\n",
                context, count, expected->message);
            if ((expected->flags & kbd_hook) != (actual->flags & kbd_hook)) dump++;

	    expected++;
	    actual++;
	}
	/* silently drop hook messages if there is no support for them */
	else if ((expected->flags & optional) ||
                 ((expected->flags & hook) && !hCBT_hook) ||
                 ((expected->flags & winevent_hook) && !hEvent_hook) ||
                 ((expected->flags & kbd_hook) && !hKBD_hook))
	    expected++;
	else if (todo)
	{
            failcount++;
            todo_wine {
                if (strcmp(winetest_platform, "wine")) dump++;
                ok_( file, line) (FALSE, "%s: %u: the msg 0x%04x was expected, but got msg 0x%04x instead\n",
                                  context, count, expected->message, actual->message);
            }
            goto done;
        }
        else
        {
            ok_( file, line) (FALSE, "%s: %u: the msg 0x%04x was expected, but got msg 0x%04x instead\n",
                              context, count, expected->message, actual->message);
            dump++;
            expected++;
            actual++;
        }
        count++;
    }

    /* skip all optional trailing messages */
    while (expected->message && ((expected->flags & optional) ||
                                 ((expected->flags & hook) && !hCBT_hook) ||
                                 ((expected->flags & winevent_hook) && !hEvent_hook)))
	expected++;

    if (todo)
    {
        todo_wine {
            if (expected->message || actual->message) {
                failcount++;
                if (strcmp(winetest_platform, "wine")) dump++;
                ok_( file, line) (FALSE, "%s: %u: the msg sequence is not complete: expected %04x - actual %04x\n",
                                  context, count, expected->message, actual->message);
            }
        }
    }
    else
    {
        if (expected->message || actual->message)
        {
            dump++;
            ok_( file, line) (FALSE, "%s: %u: the msg sequence is not complete: expected %04x - actual %04x\n",
                              context, count, expected->message, actual->message);
        }
    }
    if( todo && !failcount) /* succeeded yet marked todo */
        todo_wine {
            if (!strcmp(winetest_platform, "wine")) dump++;
            ok_( file, line)( TRUE, "%s: marked \"todo_wine\" but succeeds\n", context);
        }

done:
    if (dump) dump_sequence(expected_list, context, file, line);
    flush_sequence();
}

#define expect(EXPECTED,GOT) ok((GOT)==(EXPECTED), "Expected %d, got %d\n", (EXPECTED), (GOT))

/******************************** MDI test **********************************/

/* CreateWindow for MDI frame window, initially visible */
static const struct message WmCreateMDIframeSeq[] = {
    { HCBT_CREATEWND, hook },
    { WM_GETMINMAXINFO, sent },
    { WM_NCCREATE, sent },
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { EVENT_OBJECT_REORDER, winevent_hook|wparam|lparam, 0, 0 },
    { WM_CREATE, sent },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_NOTIFYFORMAT, sent|optional },
    { WM_QUERYUISTATE, sent|optional },
    { WM_WINDOWPOSCHANGING, sent|optional },
    { WM_GETMINMAXINFO, sent|optional },
    { WM_NCCALCSIZE, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|optional },
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { HCBT_ACTIVATE, hook },
    { EVENT_SYSTEM_FOREGROUND, winevent_hook|wparam|lparam, 0, 0 },
    { WM_QUERYNEWPALETTE, sent|wparam|lparam|optional, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE }, /* XP */
    { WM_ACTIVATEAPP, sent|wparam|optional, 1 }, /* Win9x doesn't send it */
    { WM_NCACTIVATE, sent },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ACTIVATE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE }, /* Win9x */
    { HCBT_SETFOCUS, hook },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|wparam|defwinproc, 0 },
    /* Win9x adds SWP_NOZORDER below */
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_NCCALCSIZE, sent|wparam|optional, 1 }, /* XP */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },
    { WM_MOVE, sent },
    { 0 }
};
/* DestroyWindow for MDI frame window, initially visible */
static const struct message WmDestroyMDIframeSeq[] = {
    { HCBT_DESTROYWND, hook },
    { 0x0090, sent|optional },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_HIDE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_NCACTIVATE, sent|wparam|optional, 0 }, /* Win9x */
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_NCACTIVATE, sent|wparam|optional, 0 }, /* XP */
    { WM_ACTIVATE, sent|wparam|optional, 0 }, /* Win9x */
    { WM_ACTIVATEAPP, sent|wparam|optional, 0 }, /* Win9x */
    { EVENT_OBJECT_HIDE, winevent_hook|wparam|lparam, OBJID_CARET, 0 },
    { EVENT_OBJECT_DESTROY, winevent_hook|wparam|lparam, 0, 0 },
    { WM_DESTROY, sent },
    { WM_NCDESTROY, sent },
    { 0 }
};
/* CreateWindow for MDI client window, initially visible */
static const struct message WmCreateMDIclientSeq[] = {
    { HCBT_CREATEWND, hook },
    { WM_NCCREATE, sent },
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { EVENT_OBJECT_REORDER, winevent_hook|wparam|lparam|optional, 0, 0 },
    { WM_CREATE, sent },
    { EVENT_OBJECT_REORDER, winevent_hook|wparam|lparam|optional, 0, 0 },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },
    { WM_MOVE, sent },
    { WM_PARENTNOTIFY, sent|wparam, WM_CREATE }, /* in MDI frame */
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { 0 }
};
/* ShowWindow(SW_SHOW) for MDI client window */
static const struct message WmShowMDIclientSeq[] = {
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { 0 }
};
/* ShowWindow(SW_HIDE) for MDI client window */
static const struct message WmHideMDIclientSeq[] = {
    { WM_SHOWWINDOW, sent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam|optional, 0, 0 }, /* win2000 */
    { EVENT_OBJECT_HIDE, winevent_hook|wparam|lparam|optional, 0, 0 }, /* XP */
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { 0 }
};
/* DestroyWindow for MDI client window, initially visible */
static const struct message WmDestroyMDIclientSeq[] = {
    { HCBT_DESTROYWND, hook },
    { 0x0090, sent|optional },
    { WM_PARENTNOTIFY, sent|wparam, WM_DESTROY }, /* in MDI frame */
    { WM_SHOWWINDOW, sent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_HIDE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_DESTROY, winevent_hook|wparam|lparam, 0, 0 },
    { WM_DESTROY, sent },
    { WM_NCDESTROY, sent },
    { 0 }
};
/* CreateWindow for MDI child window, initially visible */
static const struct message WmCreateMDIchildVisibleSeq[] = {
    { HCBT_CREATEWND, hook },
    { WM_NCCREATE, sent }, 
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { WM_CREATE, sent },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },
    { WM_MOVE, sent },
    /* Win2k sends wparam set to
     * MAKEWPARAM(WM_CREATE, MDI_FIRST_CHILD_ID + nTotalCreated),
     * while Win9x doesn't bother to set child window id according to
     * CLIENTCREATESTRUCT.idFirstChild
     */
    { WM_PARENTNOTIFY, sent /*|wparam, WM_CREATE*/ }, /* in MDI client */
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_MDIREFRESHMENU, sent/*|wparam|lparam, 0, 0*/ },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE },
    { WM_CHILDACTIVATE, sent|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam|defwinproc, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },

    /* Win9x: message sequence terminates here. */

    { WM_NCACTIVATE, sent|wparam|defwinproc, 1 },
    { HCBT_SETFOCUS, hook }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 }, /* in MDI client */
    { WM_IME_NOTIFY, sent|wparam|optional, 2 }, /* in MDI client */
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent }, /* in MDI client */
    { HCBT_SETFOCUS, hook },
    { WM_KILLFOCUS, sent }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|defwinproc },
    { WM_MDIACTIVATE, sent|defwinproc },
    { 0 }
};
/* CreateWindow for MDI child window with invisible parent */
static const struct message WmCreateMDIchildInvisibleParentSeq[] = {
    { HCBT_CREATEWND, hook },
    { WM_GETMINMAXINFO, sent },
    { WM_NCCREATE, sent }, 
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { EVENT_OBJECT_REORDER, winevent_hook|wparam|lparam|optional, 0, 0 },
    { WM_CREATE, sent },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },
    { WM_MOVE, sent },
    { WM_PARENTNOTIFY, sent /*|wparam, WM_CREATE*/ }, /* in MDI client */
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_MDIREFRESHMENU, sent }, /* in MDI client */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE },
    { WM_CHILDACTIVATE, sent|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam|defwinproc, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },

    /* Win9x: message sequence terminates here. */

    { WM_NCACTIVATE, sent|wparam|defwinproc, 1 },
    { HCBT_SETFOCUS, hook }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 }, /* in MDI client */
    { WM_IME_NOTIFY, sent|wparam|optional, 2 }, /* in MDI client */
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent }, /* in MDI client */
    { HCBT_SETFOCUS, hook },
    { WM_KILLFOCUS, sent }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|defwinproc },
    { WM_MDIACTIVATE, sent|defwinproc },
    { 0 }
};
/* DestroyWindow for MDI child window, initially visible */
static const struct message WmDestroyMDIchildVisibleSeq[] = {
    { HCBT_DESTROYWND, hook },
    /* Win2k sends wparam set to
     * MAKEWPARAM(WM_DESTROY, MDI_FIRST_CHILD_ID + nTotalCreated),
     * while Win9x doesn't bother to set child window id according to
     * CLIENTCREATESTRUCT.idFirstChild
     */
    { 0x0090, sent|optional },
    { WM_PARENTNOTIFY, sent /*|wparam, WM_DESTROY*/ }, /* in MDI client */
    { WM_SHOWWINDOW, sent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_HIDE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_ERASEBKGND, sent|parent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },

    /* { WM_DESTROY, sent }
     * Win9x: message sequence terminates here.
     */

    { HCBT_SETFOCUS, hook }, /* set focus to MDI client */
    { WM_KILLFOCUS, sent },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 }, /* in MDI client */
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent }, /* in MDI client */

    { HCBT_SETFOCUS, hook }, /* MDI client sets focus back to MDI child */
    { WM_KILLFOCUS, sent }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent }, /* in MDI client */

    { EVENT_OBJECT_DESTROY, winevent_hook|wparam|lparam, 0, 0 },

    { HCBT_SETFOCUS, hook }, /* set focus to MDI client */
    { WM_KILLFOCUS, sent },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 }, /* in MDI client */
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent }, /* in MDI client */

    { HCBT_SETFOCUS, hook }, /* MDI client sets focus back to MDI child */
    { WM_KILLFOCUS, sent }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent }, /* in MDI client */

    { WM_DESTROY, sent },

    { HCBT_SETFOCUS, hook }, /* set focus to MDI client */
    { WM_KILLFOCUS, sent },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 }, /* in MDI client */
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent }, /* in MDI client */

    { HCBT_SETFOCUS, hook }, /* MDI client sets focus back to MDI child */
    { WM_KILLFOCUS, sent }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent }, /* in MDI client */

    { WM_NCDESTROY, sent },
    { 0 }
};
/* CreateWindow for MDI child window, initially invisible */
static const struct message WmCreateMDIchildInvisibleSeq[] = {
    { HCBT_CREATEWND, hook },
    { WM_NCCREATE, sent }, 
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { WM_CREATE, sent },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },
    { WM_MOVE, sent },
    /* Win2k sends wparam set to
     * MAKEWPARAM(WM_CREATE, MDI_FIRST_CHILD_ID + nTotalCreated),
     * while Win9x doesn't bother to set child window id according to
     * CLIENTCREATESTRUCT.idFirstChild
     */
    { WM_PARENTNOTIFY, sent /*|wparam, WM_CREATE*/ }, /* in MDI client */
    { 0 }
};
/* DestroyWindow for MDI child window, initially invisible */
static const struct message WmDestroyMDIchildInvisibleSeq[] = {
    { HCBT_DESTROYWND, hook },
    /* Win2k sends wparam set to
     * MAKEWPARAM(WM_DESTROY, MDI_FIRST_CHILD_ID + nTotalCreated),
     * while Win9x doesn't bother to set child window id according to
     * CLIENTCREATESTRUCT.idFirstChild
     */
    { 0x0090, sent|optional },
    { WM_PARENTNOTIFY, sent /*|wparam, WM_DESTROY*/ }, /* in MDI client */
    { EVENT_OBJECT_DESTROY, winevent_hook|wparam|lparam, 0, 0 },
    { WM_DESTROY, sent },
    { WM_NCDESTROY, sent },
    /* FIXME: Wine destroys an icon/title window while Windows doesn't */
    { WM_PARENTNOTIFY, sent|wparam|optional, WM_DESTROY }, /* MDI client */
    { 0 }
};
/* CreateWindow for the 1st MDI child window, initially visible and maximized */
static const struct message WmCreateMDIchildVisibleMaxSeq1[] = {
    { HCBT_CREATEWND, hook },
    { WM_NCCREATE, sent }, 
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { WM_CREATE, sent },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },
    { WM_MOVE, sent },
    { HCBT_MINMAX, hook|lparam, 0, SW_MAXIMIZE },
    { WM_GETMINMAXINFO, sent },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_STATECHANGED  },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOCLIENTMOVE|SWP_STATECHANGED },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_MAXIMIZED },
     /* in MDI frame */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI frame */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI child */
    /* Win2k sends wparam set to
     * MAKEWPARAM(WM_CREATE, MDI_FIRST_CHILD_ID + nTotalCreated),
     * while Win9x doesn't bother to set child window id according to
     * CLIENTCREATESTRUCT.idFirstChild
     */
    { WM_PARENTNOTIFY, sent /*|wparam, WM_CREATE*/ }, /* in MDI client */
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_MDIREFRESHMENU, sent/*|wparam|lparam, 0, 0*/ },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE },
    { WM_CHILDACTIVATE, sent|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam|defwinproc|optional, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE, 0, SWP_FRAMECHANGED },

    /* Win9x: message sequence terminates here. */

    { WM_NCACTIVATE, sent|wparam|defwinproc|optional, 1 },
    { HCBT_SETFOCUS, hook|optional }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 }, /* in MDI client */
    { WM_IME_NOTIFY, sent|wparam|optional, 2 }, /* in MDI client */
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|optional }, /* in MDI client */
    { HCBT_SETFOCUS, hook|optional },
    { WM_KILLFOCUS, sent|optional }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|defwinproc|optional },
    { WM_MDIACTIVATE, sent|defwinproc|optional },
     /* in MDI frame */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI frame */
    { 0 }
};
/* CreateWindow for the 2nd MDI child window, initially visible and maximized */
static const struct message WmCreateMDIchildVisibleMaxSeq2[] = {
    /* restore the 1st MDI child */
    { WM_SETREDRAW, sent|wparam, 0 },
    { HCBT_MINMAX, hook|lparam, 0, SW_SHOWNORMAL },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_STATECHANGED },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_CHILDACTIVATE, sent|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOCLIENTMOVE|SWP_STATECHANGED },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_RESTORED },
     /* in MDI frame */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI frame */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI child */
    { WM_SETREDRAW, sent|wparam, 1 }, /* in the 1st MDI child */
    /* create the 2nd MDI child */
    { HCBT_CREATEWND, hook },
    { WM_NCCREATE, sent }, 
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { WM_CREATE, sent },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },
    { WM_MOVE, sent },
    { HCBT_MINMAX, hook|lparam, 0, SW_MAXIMIZE },
    { WM_GETMINMAXINFO, sent },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_STATECHANGED },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { EVENT_OBJECT_REORDER, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOCLIENTMOVE|SWP_STATECHANGED },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_MAXIMIZED },
     /* in MDI frame */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI frame */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI child */
    /* Win2k sends wparam set to
     * MAKEWPARAM(WM_CREATE, MDI_FIRST_CHILD_ID + nTotalCreated),
     * while Win9x doesn't bother to set child window id according to
     * CLIENTCREATESTRUCT.idFirstChild
     */
    { WM_PARENTNOTIFY, sent /*|wparam, WM_CREATE*/ }, /* in MDI client */
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_MDIREFRESHMENU, sent/*|wparam|lparam, 0, 0*/ },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE },
    { WM_CHILDACTIVATE, sent|wparam|lparam, 0, 0 },

    { WM_NCACTIVATE, sent|wparam|defwinproc, 0 }, /* in the 1st MDI child */
    { WM_MDIACTIVATE, sent|defwinproc }, /* in the 1st MDI child */

    { WM_WINDOWPOSCHANGING, sent|wparam|defwinproc, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },

    /* Win9x: message sequence terminates here. */

    { WM_NCACTIVATE, sent|wparam|defwinproc, 1 },
    { HCBT_SETFOCUS, hook },
    { WM_KILLFOCUS, sent|defwinproc|optional }, /* in the 1st MDI child */
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 0 }, /* in the 1st MDI child */
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 }, /* in MDI client */
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent }, /* in MDI client */
    { HCBT_SETFOCUS, hook },
    { WM_KILLFOCUS, sent }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|defwinproc },

    { WM_MDIACTIVATE, sent|defwinproc },
     /* in MDI frame */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI frame */
    { 0 }
};
/* WM_MDICREATE MDI child window, initially visible and maximized */
static const struct message WmCreateMDIchildVisibleMaxSeq3[] = {
    { WM_MDICREATE, sent },
    { HCBT_CREATEWND, hook },
    { WM_NCCREATE, sent }, 
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { WM_CREATE, sent },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },
    { WM_MOVE, sent },
    { HCBT_MINMAX, hook|lparam, 0, SW_MAXIMIZE },
    { WM_GETMINMAXINFO, sent },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_STATECHANGED },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOCLIENTMOVE|SWP_STATECHANGED },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_MAXIMIZED },

     /* in MDI frame */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI frame */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI child */

    /* Win2k sends wparam set to
     * MAKEWPARAM(WM_CREATE, MDI_FIRST_CHILD_ID + nTotalCreated),
     * while Win9x doesn't bother to set child window id according to
     * CLIENTCREATESTRUCT.idFirstChild
     */
    { WM_PARENTNOTIFY, sent /*|wparam, WM_CREATE*/ }, /* in MDI client */
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },

    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },

    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_MDIREFRESHMENU, sent/*|wparam|lparam, 0, 0*/ },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE },

    { WM_CHILDACTIVATE, sent|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam|defwinproc, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },

    /* Win9x: message sequence terminates here. */

    { WM_NCACTIVATE, sent|wparam|defwinproc, 1 },
    { WM_SETFOCUS, sent|optional }, /* in MDI client */
    { HCBT_SETFOCUS, hook }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 }, /* in MDI client */
    { WM_IME_NOTIFY, sent|wparam|optional, 2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam|optional, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|optional }, /* in MDI client */
    { HCBT_SETFOCUS, hook|optional },
    { WM_KILLFOCUS, sent }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|defwinproc },

    { WM_MDIACTIVATE, sent|defwinproc },

     /* in MDI child */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },

     /* in MDI frame */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { 0x0093, sent|defwinproc|optional },
    { 0x0093, sent|defwinproc|optional },
    { 0x0093, sent|defwinproc|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_RESTORED },

     /* in MDI client */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOACTIVATE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOCLIENTMOVE },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },

     /* in MDI child */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOACTIVATE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { 0x0093, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOCLIENTMOVE },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_MAXIMIZED },

    { 0x0093, sent|optional },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI child */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI client */
    { WM_NCCALCSIZE, sent|wparam|optional, 1 }, /* XP sends it to MDI frame */
    { 0x0093, sent|defwinproc|optional },
    { 0x0093, sent|defwinproc|optional },
    { 0x0093, sent|defwinproc|optional },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI frame */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* XP sends a duplicate */

    { 0 }
};
/* CreateWindow for the 1st MDI child window, initially invisible and maximized */
static const struct message WmCreateMDIchildInvisibleMaxSeq4[] = {
    { HCBT_CREATEWND, hook },
    { WM_GETMINMAXINFO, sent },
    { WM_NCCREATE, sent }, 
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { EVENT_OBJECT_REORDER, winevent_hook|wparam|lparam, 0, 0 },
    { WM_CREATE, sent },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE, 0, SWP_NOZORDER }, /* MDI frame */
    { WM_NCCALCSIZE, sent|wparam|optional, 1 }, /* MDI frame */
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE, 0, SWP_NOZORDER }, /* MDI frame */
    { WM_MOVE, sent },
    { HCBT_MINMAX, hook|lparam, 0, SW_MAXIMIZE },
    { WM_GETMINMAXINFO, sent },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_STATECHANGED },
    { WM_GETMINMAXINFO, sent|defwinproc },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOREDRAW|SWP_STATECHANGED },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_MAXIMIZED },
     /* in MDI frame */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI frame */
    { WM_NCCALCSIZE, sent|wparam|optional, 1 }, /* MDI child */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI child */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI child */
    /* Win2k sends wparam set to
     * MAKEWPARAM(WM_CREATE, MDI_FIRST_CHILD_ID + nTotalCreated),
     * while Win9x doesn't bother to set child window id according to
     * CLIENTCREATESTRUCT.idFirstChild
     */
    { WM_PARENTNOTIFY, sent /*|wparam, WM_CREATE*/ }, /* in MDI client */
    { 0 }
};
/* WM_SYSCOMMAND/SC_CLOSE for the 2nd MDI child window, initially visible and maximized */
static const struct message WmDestroyMDIchildVisibleMaxSeq2[] = {
    { WM_SYSCOMMAND, sent|wparam, SC_CLOSE },
    { HCBT_SYSCOMMAND, hook },
    { WM_CLOSE, sent|defwinproc },
    { WM_MDIDESTROY, sent }, /* in MDI client */

    /* bring the 1st MDI child to top */
    { WM_WINDOWPOSCHANGING, sent|wparam|defwinproc, SWP_NOSIZE|SWP_NOMOVE }, /* in the 1st MDI child */
    { WM_WINDOWPOSCHANGING, sent|wparam|defwinproc, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE }, /* in the 2nd MDI child */

    { EVENT_OBJECT_REORDER, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },

    { WM_CHILDACTIVATE, sent|defwinproc|wparam|lparam, 0, 0 }, /* in the 1st MDI child */
    { WM_NCACTIVATE, sent|wparam|defwinproc, 0 }, /* in the 1st MDI child */
    { WM_MDIACTIVATE, sent|defwinproc }, /* in the 1st MDI child */

    /* maximize the 1st MDI child */
    { HCBT_MINMAX, hook|lparam, 0, SW_MAXIMIZE },
    { WM_GETMINMAXINFO, sent|defwinproc },
    { WM_WINDOWPOSCHANGING, sent|wparam|defwinproc, SWP_FRAMECHANGED|SWP_STATECHANGED },
    { WM_NCCALCSIZE, sent|defwinproc|wparam, 1 },
    { WM_CHILDACTIVATE, sent|defwinproc|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam|defwinproc, SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOCLIENTMOVE|SWP_STATECHANGED },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_MAXIMIZED },

    /* restore the 2nd MDI child */
    { WM_SETREDRAW, sent|defwinproc|wparam, 0 },
    { HCBT_MINMAX, hook|lparam, 0, SW_NORMALNA },
    { WM_WINDOWPOSCHANGING, sent|wparam|defwinproc, SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_SHOWWINDOW|SWP_STATECHANGED },
    { WM_NCCALCSIZE, sent|defwinproc|wparam, 1 },

    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },

    { WM_WINDOWPOSCHANGED, sent|wparam|defwinproc, SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOCLIENTMOVE|SWP_STATECHANGED },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_RESTORED },

    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI frame */

    { WM_SETREDRAW, sent|defwinproc|wparam, 1 },
     /* in MDI frame */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI frame */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI child */

    /* bring the 1st MDI child to top */
    { WM_WINDOWPOSCHANGING, sent|wparam|defwinproc, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCACTIVATE, sent|wparam|defwinproc, 1 },
    { HCBT_SETFOCUS, hook },
    { WM_KILLFOCUS, sent|defwinproc },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 0 },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 }, /* in MDI client */
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent }, /* in MDI client */
    { HCBT_SETFOCUS, hook },
    { WM_KILLFOCUS, sent }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|defwinproc },
    { WM_MDIACTIVATE, sent|defwinproc },
    { WM_WINDOWPOSCHANGED, sent|wparam|defwinproc, SWP_NOSIZE|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },

    /* apparently ShowWindow(SW_SHOW) on an MDI client */
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_MDIREFRESHMENU, sent },

    { HCBT_DESTROYWND, hook },
    /* Win2k sends wparam set to
     * MAKEWPARAM(WM_DESTROY, MDI_FIRST_CHILD_ID + nTotalCreated),
     * while Win9x doesn't bother to set child window id according to
     * CLIENTCREATESTRUCT.idFirstChild
     */
    { 0x0090, sent|defwinproc|optional },
    { WM_PARENTNOTIFY, sent /*|wparam, WM_DESTROY*/ }, /* in MDI client */
    { WM_SHOWWINDOW, sent|defwinproc|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam|defwinproc, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_HIDE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_ERASEBKGND, sent|parent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam|defwinproc, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },

    { EVENT_OBJECT_DESTROY, winevent_hook|wparam|lparam, 0, 0 },
    { WM_DESTROY, sent|defwinproc },
    { WM_NCDESTROY, sent|defwinproc },
    { 0 }
};
/* WM_MDIDESTROY for the single MDI child window, initially visible and maximized */
static const struct message WmDestroyMDIchildVisibleMaxSeq1[] = {
    { WM_MDIDESTROY, sent }, /* in MDI client */
    { WM_SHOWWINDOW, sent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_HIDE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_ERASEBKGND, sent|parent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },

    { HCBT_SETFOCUS, hook },
    { WM_KILLFOCUS, sent },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 }, /* in MDI client */
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent }, /* in MDI client */
    { HCBT_SETFOCUS, hook },
    { WM_KILLFOCUS, sent }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent },

     /* in MDI child */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI child */

     /* in MDI frame */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { 0x0093, sent|defwinproc|optional },
    { 0x0093, sent|defwinproc|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_RESTORED },

     /* in MDI client */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOACTIVATE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOCLIENTMOVE },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },

     /* in MDI child */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOACTIVATE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOCLIENTMOVE },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_MAXIMIZED },

     /* in MDI child */
    { WM_WINDOWPOSCHANGING, sent|wparam|defwinproc, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam|defwinproc, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam|defwinproc, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI child */

     /* in MDI frame */
    { WM_WINDOWPOSCHANGING, sent|wparam|defwinproc, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam|defwinproc, 1 },
    { 0x0093, sent|defwinproc|optional },
    { 0x0093, sent|defwinproc|optional },
    { 0x0093, sent|defwinproc|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam|defwinproc, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_RESTORED },

     /* in MDI client */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOACTIVATE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOCLIENTMOVE },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },

     /* in MDI child */
    { WM_WINDOWPOSCHANGING, sent|wparam|defwinproc, SWP_NOACTIVATE },
    { WM_NCCALCSIZE, sent|wparam|defwinproc, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam|defwinproc, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOCLIENTMOVE },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_MAXIMIZED },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI child */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI client */

    { 0x0093, sent|defwinproc|optional },
    { WM_NCCALCSIZE, sent|wparam|defwinproc|optional, 1 }, /* XP sends it to MDI frame */
    { 0x0093, sent|defwinproc|optional },
    { 0x0093, sent|defwinproc|optional },
    { 0x0093, sent|defwinproc|optional },
    { 0x0093, sent|optional },

    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI frame */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI child */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI client */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI frame */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* XP sends a duplicate */

     /* in MDI frame */
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam|optional, 1 },
    { 0x0093, sent|defwinproc|optional },
    { 0x0093, sent|defwinproc|optional },
    { 0x0093, sent|defwinproc|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI frame */
    { 0x0093, sent|optional },

    { WM_NCACTIVATE, sent|wparam, 0 },
    { WM_MDIACTIVATE, sent },

    { HCBT_MINMAX, hook|lparam, 0, SW_SHOWNORMAL },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_SHOWWINDOW|SWP_STATECHANGED },
    { WM_NCCALCSIZE, sent|wparam, 1 },

    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },

    { WM_CHILDACTIVATE, sent|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOCLIENTMOVE|SWP_STATECHANGED },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_RESTORED },

     /* in MDI child */
    { WM_WINDOWPOSCHANGING, sent|wparam|defwinproc, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam|defwinproc, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam|defwinproc, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI child */

     /* in MDI frame */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_RESTORED },

     /* in MDI client */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOACTIVATE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOCLIENTMOVE },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI child */
    { WM_NCCALCSIZE, sent|wparam|optional, 1 }, /* XP */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI client */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI frame */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* XP sends a duplicate */

    { HCBT_SETFOCUS, hook },
    { WM_KILLFOCUS, sent },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 }, /* in MDI client */
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent }, /* in MDI client */

    { WM_MDIREFRESHMENU, sent }, /* in MDI client */

    { HCBT_DESTROYWND, hook },
    /* Win2k sends wparam set to
     * MAKEWPARAM(WM_DESTROY, MDI_FIRST_CHILD_ID + nTotalCreated),
     * while Win9x doesn't bother to set child window id according to
     * CLIENTCREATESTRUCT.idFirstChild
     */
    { 0x0090, sent|optional },
    { WM_PARENTNOTIFY, sent /*|wparam, WM_DESTROY*/ }, /* in MDI client */

    { WM_SHOWWINDOW, sent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_HIDE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_ERASEBKGND, sent|parent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },

    { EVENT_OBJECT_DESTROY, winevent_hook|wparam|lparam, 0, 0 },
    { WM_DESTROY, sent },
    { WM_NCDESTROY, sent },
    { 0 }
};
/* ShowWindow(SW_MAXIMIZE) for a not visible MDI child window */
static const struct message WmMaximizeMDIchildInvisibleSeq[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_MAXIMIZE },
    { WM_GETMINMAXINFO, sent },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_STATECHANGED },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { WM_CHILDACTIVATE, sent|wparam|lparam, 0, 0 },

    { WM_WINDOWPOSCHANGING, sent|wparam|optional|defwinproc, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCACTIVATE, sent|wparam|optional|defwinproc, 1 },
    { HCBT_SETFOCUS, hook|optional },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 }, /* in MDI client */
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|optional }, /* in MDI client */
    { HCBT_SETFOCUS, hook|optional },
    { WM_KILLFOCUS, sent|optional }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|optional|defwinproc },
    { WM_MDIACTIVATE, sent|optional|defwinproc },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOCLIENTMOVE|SWP_STATECHANGED },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_MAXIMIZED },
     /* in MDI frame */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI frame */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI child */
    { 0 }
};
/* ShowWindow(SW_MAXIMIZE) for a not visible maximized MDI child window */
static const struct message WmMaximizeMDIchildInvisibleSeq2[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_MAXIMIZE },
    { WM_GETMINMAXINFO, sent },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED },
    { WM_GETMINMAXINFO, sent|defwinproc },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { WM_CHILDACTIVATE, sent|wparam|lparam, 0, 0 },

    { WM_WINDOWPOSCHANGING, sent|wparam|defwinproc|optional, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCACTIVATE, sent|wparam|defwinproc|optional, 1 },
    { HCBT_SETFOCUS, hook|optional },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 }, /* in MDI client */
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|optional }, /* in MDI client */
    { HCBT_SETFOCUS, hook|optional },
    { WM_KILLFOCUS, sent|optional }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 }, /* in MDI client */
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|defwinproc|optional },
    { WM_MDIACTIVATE, sent|defwinproc|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI child */
    { 0 }
};
/* WM_MDIMAXIMIZE for an MDI child window with invisible parent */
static const struct message WmMaximizeMDIchildInvisibleParentSeq[] = {
    { WM_MDIMAXIMIZE, sent }, /* in MDI client */
    { HCBT_MINMAX, hook|lparam, 0, SW_MAXIMIZE },
    { WM_GETMINMAXINFO, sent },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_STATECHANGED },
    { WM_GETMINMAXINFO, sent|defwinproc },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam|optional, 0, 0 }, /* XP doesn't send it */
    { WM_CHILDACTIVATE, sent|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOREDRAW|SWP_STATECHANGED },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_MAXIMIZED },

    { WM_WINDOWPOSCHANGING, sent|wparam|defwinproc, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam|defwinproc, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam|defwinproc, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_NCCALCSIZE, sent|wparam|defwinproc|optional, 1 },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam|optional, 0, 0 }, /* MDI child XP */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam|optional, 0, 0 }, /* MDI client XP */
     /* in MDI frame */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { 0x0093, sent|defwinproc|optional },
    { 0x0094, sent|defwinproc|optional },
    { 0x0094, sent|defwinproc|optional },
    { 0x0094, sent|defwinproc|optional },
    { 0x0094, sent|defwinproc|optional },
    { 0x0093, sent|defwinproc|optional },
    { 0x0093, sent|defwinproc|optional },
    { 0x0091, sent|defwinproc|optional },
    { 0x0092, sent|defwinproc|optional },
    { 0x0092, sent|defwinproc|optional },
    { 0x0092, sent|defwinproc|optional },
    { 0x0092, sent|defwinproc|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_RESTORED },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam|optional, 0, 0 }, /* MDI frame win2000 */
     /* in MDI client */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOACTIVATE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOCLIENTMOVE },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },
     /* in MDI child */
    { WM_WINDOWPOSCHANGING, sent|wparam|defwinproc, SWP_NOACTIVATE },
    { WM_GETMINMAXINFO, sent|defwinproc },
    { WM_NCCALCSIZE, sent|wparam|defwinproc, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam|defwinproc, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOCLIENTMOVE },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_MAXIMIZED },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam|optional, 0, 0 }, /* MDI child win2000 */
    { WM_NCCALCSIZE, sent|wparam|defwinproc|optional, 1 },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam|optional, 0, 0 }, /* MDI child XP */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam|optional, 0, 0 }, /* MDI child XP */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam|optional, 0, 0 }, /* MDI client XP */
     /* in MDI frame */
    { 0x0093, sent|optional },
    { WM_NCCALCSIZE, sent|wparam|optional, 1 },
    { 0x0093, sent|defwinproc|optional },
    { 0x0093, sent|defwinproc|optional },
    { 0x0093, sent|defwinproc|optional },
    { 0x0091, sent|defwinproc|optional },
    { 0x0092, sent|defwinproc|optional },
    { 0x0092, sent|defwinproc|optional },
    { 0x0092, sent|defwinproc|optional },
    { 0x0092, sent|defwinproc|optional },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam|optional, 0, 0 }, /* MDI frame XP */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam|optional, 0, 0 }, /* MDI frame XP */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam|optional, 0, 0 }, /* MDI child XP */
    { 0 }
};
/* ShowWindow(SW_MAXIMIZE) for a visible MDI child window */
static const struct message WmMaximizeMDIchildVisibleSeq[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_MAXIMIZE },
    { WM_GETMINMAXINFO, sent },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_STATECHANGED },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_CHILDACTIVATE, sent|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOCLIENTMOVE|SWP_STATECHANGED },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_MAXIMIZED },
     /* in MDI frame */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI frame */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI child */
    { 0 }
};
/* ShowWindow(SW_RESTORE) for a visible maximized MDI child window */
static const struct message WmRestoreMDIchildVisibleSeq[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_RESTORE },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_STATECHANGED },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_CHILDACTIVATE, sent|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOCLIENTMOVE|SWP_STATECHANGED },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_RESTORED },
     /* in MDI frame */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI frame */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI child */
    { 0 }
};
/* ShowWindow(SW_RESTORE) for a visible minimized MDI child window */
static const struct message WmRestoreMDIchildVisibleSeq_2[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_RESTORE },
    { WM_QUERYOPEN, sent|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOCOPYBITS|SWP_STATECHANGED },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_CHILDACTIVATE, sent|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOCOPYBITS|SWP_STATECHANGED },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_RESTORED },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI child */
    { EVENT_SYSTEM_MINIMIZEEND, winevent_hook|wparam|lparam, 0, 0 }, /* MDI child */
    { HCBT_SETFOCUS, hook },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent },
    { 0 }
};
/* ShowWindow(SW_MINIMIZE) for a visible restored MDI child window */
static const struct message WmMinimizeMDIchildVisibleSeq[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_MINIMIZE },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_STATECHANGED },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_STATECHANGED },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc|wparam|lparam, SIZE_MINIMIZED, 0 },
    { WM_CHILDACTIVATE, sent|wparam|lparam|defwinproc, 0, 0 },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI child */
    { EVENT_SYSTEM_MINIMIZESTART, winevent_hook|wparam|lparam, 0, 0 }, /* MDI child */
    /* FIXME: Wine creates an icon/title window while Windows doesn't */
    { WM_PARENTNOTIFY, sent|parent|wparam|optional, WM_CREATE }, /* MDI client */
    { 0 }
};
/* ShowWindow(SW_RESTORE) for a not visible MDI child window */
static const struct message WmRestoreMDIchildInisibleSeq[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_RESTORE },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_STATECHANGED  },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { WM_CHILDACTIVATE, sent|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOCLIENTMOVE|SWP_STATECHANGED },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_RESTORED },
     /* in MDI frame */
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI frame */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 }, /* MDI child */
    { 0 }
};

static HWND mdi_client;
static WNDPROC old_mdi_client_proc;

static LRESULT WINAPI mdi_client_hook_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct recvd_message msg;

    /* do not log painting messages */
    if (message != WM_PAINT &&
        message != WM_NCPAINT &&
        message != WM_SYNCPAINT &&
        message != WM_ERASEBKGND &&
        message != WM_NCHITTEST &&
        message != WM_GETTEXT &&
        message != WM_MDIGETACTIVE &&
        !ignore_message( message ))
    {
        msg.hwnd = hwnd;
        msg.message = message;
        msg.flags = sent|wparam|lparam;
        msg.wParam = wParam;
        msg.lParam = lParam;
        msg.descr = "mdi client";
        add_message(&msg);
    }

    return CallWindowProcA(old_mdi_client_proc, hwnd, message, wParam, lParam);
}

static LRESULT WINAPI mdi_child_wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct recvd_message msg;

    /* do not log painting messages */
    if (message != WM_PAINT &&
        message != WM_NCPAINT &&
        message != WM_SYNCPAINT &&
        message != WM_ERASEBKGND &&
        message != WM_NCHITTEST &&
        message != WM_GETTEXT &&
        !ignore_message( message ))
    {
        switch (message)
        {
            case WM_MDIACTIVATE:
            {
                HWND active, client = GetParent(hwnd);

                active = (HWND)SendMessageA(client, WM_MDIGETACTIVE, 0, 0);

                if (hwnd == (HWND)lParam) /* if we are being activated */
                    ok (active == (HWND)lParam, "new active %p != active %p\n", (HWND)lParam, active);
                else
                    ok (active == (HWND)wParam, "old active %p != active %p\n", (HWND)wParam, active);
                break;
            }
        }

        msg.hwnd = hwnd;
        msg.message = message;
        msg.flags = sent|wparam|lparam;
        if (defwndproc_counter) msg.flags |= defwinproc;
        msg.wParam = wParam;
        msg.lParam = lParam;
        msg.descr = "mdi child";
        add_message(&msg);
    }

    defwndproc_counter++;
    ret = DefMDIChildProcA(hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static LRESULT WINAPI mdi_frame_wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct recvd_message msg;

    /* do not log painting messages */
    if (message != WM_PAINT &&
        message != WM_NCPAINT &&
        message != WM_SYNCPAINT &&
        message != WM_ERASEBKGND &&
        message != WM_NCHITTEST &&
        message != WM_GETTEXT &&
        !ignore_message( message ))
    {
        msg.hwnd = hwnd;
        msg.message = message;
        msg.flags = sent|wparam|lparam;
        if (defwndproc_counter) msg.flags |= defwinproc;
        msg.wParam = wParam;
        msg.lParam = lParam;
        msg.descr = "mdi frame";
        add_message(&msg);
    }

    defwndproc_counter++;
    ret = DefFrameProcA(hwnd, mdi_client, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static BOOL mdi_RegisterWindowClasses(void)
{
    WNDCLASSA cls;

    cls.style = 0;
    cls.lpfnWndProc = mdi_frame_wnd_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "MDI_frame_class";
    if (!RegisterClassA(&cls)) return FALSE;

    cls.lpfnWndProc = mdi_child_wnd_proc;
    cls.lpszClassName = "MDI_child_class";
    if (!RegisterClassA(&cls)) return FALSE;

    if (!GetClassInfoA(0, "MDIClient", &cls)) assert(0);
    old_mdi_client_proc = cls.lpfnWndProc;
    cls.hInstance = GetModuleHandleA(0);
    cls.lpfnWndProc = mdi_client_hook_proc;
    cls.lpszClassName = "MDI_client_class";
    if (!RegisterClassA(&cls)) assert(0);

    return TRUE;
}

static void test_mdi_messages(void)
{
    MDICREATESTRUCTA mdi_cs;
    CLIENTCREATESTRUCT client_cs;
    HWND mdi_frame, mdi_child, mdi_child2, active_child;
    BOOL zoomed;
    RECT rc;
    HMENU hMenu = CreateMenu();

    if (!mdi_RegisterWindowClasses()) assert(0);

    flush_sequence();

    trace("creating MDI frame window\n");
    mdi_frame = CreateWindowExA(0, "MDI_frame_class", "MDI frame window",
                                WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                                WS_MAXIMIZEBOX | WS_VISIBLE,
                                100, 100, CW_USEDEFAULT, CW_USEDEFAULT,
                                GetDesktopWindow(), hMenu,
                                GetModuleHandleA(0), NULL);
    assert(mdi_frame);
    ok_sequence(WmCreateMDIframeSeq, "Create MDI frame window", FALSE);

    ok(GetActiveWindow() == mdi_frame, "wrong active window %p\n", GetActiveWindow());
    ok(GetFocus() == mdi_frame, "wrong focus window %p\n", GetFocus());

    trace("creating MDI client window\n");
    GetClientRect(mdi_frame, &rc);
    client_cs.hWindowMenu = 0;
    client_cs.idFirstChild = MDI_FIRST_CHILD_ID;
    mdi_client = CreateWindowExA(0, "MDI_client_class",
                                 NULL,
                                 WS_CHILD | WS_VISIBLE | MDIS_ALLCHILDSTYLES,
                                 rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                                 mdi_frame, 0, GetModuleHandleA(0), &client_cs);
    assert(mdi_client);
    ok_sequence(WmCreateMDIclientSeq, "Create visible MDI client window", FALSE);

    ok(GetActiveWindow() == mdi_frame, "wrong active window %p\n", GetActiveWindow());
    ok(GetFocus() == mdi_frame, "input focus should be on MDI frame not on %p\n", GetFocus());

    active_child = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, (LPARAM)&zoomed);
    ok(!active_child, "wrong active MDI child %p\n", active_child);
    ok(!zoomed, "wrong zoomed state %d\n", zoomed);

    SetFocus(0);
    flush_sequence();

    trace("creating invisible MDI child window\n");
    mdi_child = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_class", "MDI child",
                                WS_CHILD,
                                0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandleA(0), NULL);
    assert(mdi_child);

    flush_sequence();
    ShowWindow(mdi_child, SW_SHOWNORMAL);
    ok_sequence(WmShowChildSeq, "ShowWindow(SW_SHOWNORMAL) MDI child window", FALSE);

    ok(GetWindowLongA(mdi_child, GWL_STYLE) & WS_VISIBLE, "MDI child should be visible\n");
    ok(IsWindowVisible(mdi_child), "MDI child should be visible\n");

    ok(GetActiveWindow() == mdi_frame, "wrong active window %p\n", GetActiveWindow());
    ok(GetFocus() == 0, "wrong focus window %p\n", GetFocus());

    active_child = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, (LPARAM)&zoomed);
    ok(!active_child, "wrong active MDI child %p\n", active_child);
    ok(!zoomed, "wrong zoomed state %d\n", zoomed);

    ShowWindow(mdi_child, SW_HIDE);
    ok_sequence(WmHideChildSeq, "ShowWindow(SW_HIDE) MDI child window", FALSE);
    flush_sequence();

    ShowWindow(mdi_child, SW_SHOW);
    ok_sequence(WmShowChildSeq, "ShowWindow(SW_SHOW) MDI child window", FALSE);

    ok(GetWindowLongA(mdi_child, GWL_STYLE) & WS_VISIBLE, "MDI child should be visible\n");
    ok(IsWindowVisible(mdi_child), "MDI child should be visible\n");

    ok(GetActiveWindow() == mdi_frame, "wrong active window %p\n", GetActiveWindow());
    ok(GetFocus() == 0, "wrong focus window %p\n", GetFocus());

    active_child = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, (LPARAM)&zoomed);
    ok(!active_child, "wrong active MDI child %p\n", active_child);
    ok(!zoomed, "wrong zoomed state %d\n", zoomed);

    DestroyWindow(mdi_child);
    flush_sequence();

    trace("creating visible MDI child window\n");
    mdi_child = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_class", "MDI child",
                                WS_CHILD | WS_VISIBLE,
                                0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandleA(0), NULL);
    assert(mdi_child);
    ok_sequence(WmCreateMDIchildVisibleSeq, "Create visible MDI child window", FALSE);

    ok(GetWindowLongA(mdi_child, GWL_STYLE) & WS_VISIBLE, "MDI child should be visible\n");
    ok(IsWindowVisible(mdi_child), "MDI child should be visible\n");

    ok(GetActiveWindow() == mdi_frame, "wrong active window %p\n", GetActiveWindow());
    ok(GetFocus() == mdi_child, "wrong focus window %p\n", GetFocus());

    active_child = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, (LPARAM)&zoomed);
    ok(active_child == mdi_child, "wrong active MDI child %p\n", active_child);
    ok(!zoomed, "wrong zoomed state %d\n", zoomed);
    flush_sequence();

    DestroyWindow(mdi_child);
    ok_sequence(WmDestroyMDIchildVisibleSeq, "Destroy visible MDI child window", TRUE);

    ok(GetActiveWindow() == mdi_frame, "wrong active window %p\n", GetActiveWindow());
    ok(GetFocus() == 0, "wrong focus window %p\n", GetFocus());

    /* Win2k: MDI client still returns a just destroyed child as active
     * Win9x: MDI client returns 0
     */
    active_child = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, (LPARAM)&zoomed);
    ok(active_child == mdi_child || /* win2k */
       !active_child, /* win9x */
       "wrong active MDI child %p\n", active_child);
    ok(!zoomed, "wrong zoomed state %d\n", zoomed);

    flush_sequence();

    trace("creating invisible MDI child window\n");
    mdi_child2 = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_class", "MDI child",
                                WS_CHILD,
                                0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandleA(0), NULL);
    assert(mdi_child2);
    ok_sequence(WmCreateMDIchildInvisibleSeq, "Create invisible MDI child window", FALSE);

    ok(!(GetWindowLongA(mdi_child2, GWL_STYLE) & WS_VISIBLE), "MDI child should not be visible\n");
    ok(!IsWindowVisible(mdi_child2), "MDI child should not be visible\n");

    ok(GetActiveWindow() == mdi_frame, "wrong active window %p\n", GetActiveWindow());
    ok(GetFocus() == 0, "wrong focus window %p\n", GetFocus());

    /* Win2k: MDI client still returns a just destroyed child as active
     * Win9x: MDI client returns mdi_child2
     */
    active_child = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, (LPARAM)&zoomed);
    ok(active_child == mdi_child || /* win2k */
       active_child == mdi_child2, /* win9x */
       "wrong active MDI child %p\n", active_child);
    ok(!zoomed, "wrong zoomed state %d\n", zoomed);
    flush_sequence();

    ShowWindow(mdi_child2, SW_MAXIMIZE);
    ok_sequence(WmMaximizeMDIchildInvisibleSeq, "ShowWindow(SW_MAXIMIZE):invisible MDI child", FALSE);

    ok(GetWindowLongA(mdi_child2, GWL_STYLE) & WS_VISIBLE, "MDI child should be visible\n");
    ok(IsWindowVisible(mdi_child2), "MDI child should be visible\n");

    active_child = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, (LPARAM)&zoomed);
    ok(active_child == mdi_child2, "wrong active MDI child %p\n", active_child);
    ok(zoomed, "wrong zoomed state %d\n", zoomed);
    flush_sequence();

    ok(GetActiveWindow() == mdi_frame, "wrong active window %p\n", GetActiveWindow());
    ok(GetFocus() == mdi_child2 || /* win2k */
       GetFocus() == 0, /* win9x */
       "wrong focus window %p\n", GetFocus());

    SetFocus(0);
    flush_sequence();

    ShowWindow(mdi_child2, SW_HIDE);
    ok_sequence(WmHideChildSeq, "ShowWindow(SW_HIDE):MDI child", FALSE);

    ShowWindow(mdi_child2, SW_RESTORE);
    ok_sequence(WmRestoreMDIchildInisibleSeq, "ShowWindow(SW_RESTORE):invisible MDI child", FALSE);
    flush_sequence();

    ok(GetWindowLongA(mdi_child2, GWL_STYLE) & WS_VISIBLE, "MDI child should be visible\n");
    ok(IsWindowVisible(mdi_child2), "MDI child should be visible\n");

    active_child = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, (LPARAM)&zoomed);
    ok(active_child == mdi_child2, "wrong active MDI child %p\n", active_child);
    ok(!zoomed, "wrong zoomed state %d\n", zoomed);
    flush_sequence();

    SetFocus(0);
    flush_sequence();

    ShowWindow(mdi_child2, SW_HIDE);
    ok_sequence(WmHideChildSeq, "ShowWindow(SW_HIDE):MDI child", FALSE);

    ShowWindow(mdi_child2, SW_SHOW);
    ok_sequence(WmShowChildSeq, "ShowWindow(SW_SHOW):MDI child", FALSE);

    ok(GetActiveWindow() == mdi_frame, "wrong active window %p\n", GetActiveWindow());
    ok(GetFocus() == 0, "wrong focus window %p\n", GetFocus());

    ShowWindow(mdi_child2, SW_MAXIMIZE);
    ok_sequence(WmMaximizeMDIchildVisibleSeq, "ShowWindow(SW_MAXIMIZE):MDI child", FALSE);

    ok(GetActiveWindow() == mdi_frame, "wrong active window %p\n", GetActiveWindow());
    ok(GetFocus() == 0, "wrong focus window %p\n", GetFocus());

    ShowWindow(mdi_child2, SW_RESTORE);
    ok_sequence(WmRestoreMDIchildVisibleSeq, "ShowWindow(SW_RESTORE):maximized MDI child", FALSE);

    ok(GetActiveWindow() == mdi_frame, "wrong active window %p\n", GetActiveWindow());
    ok(GetFocus() == 0, "wrong focus window %p\n", GetFocus());

    ShowWindow(mdi_child2, SW_MINIMIZE);
    ok_sequence(WmMinimizeMDIchildVisibleSeq, "ShowWindow(SW_MINIMIZE):MDI child", TRUE);

    ok(GetActiveWindow() == mdi_frame, "wrong active window %p\n", GetActiveWindow());
    ok(GetFocus() == 0, "wrong focus window %p\n", GetFocus());

    active_child = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, (LPARAM)&zoomed);
    ok(active_child == mdi_child2, "wrong active MDI child %p\n", active_child);
    ok(!zoomed, "wrong zoomed state %d\n", zoomed);
    flush_sequence();

    ShowWindow(mdi_child2, SW_RESTORE);
    ok_sequence(WmRestoreMDIchildVisibleSeq_2, "ShowWindow(SW_RESTORE):minimized MDI child", FALSE);

    ok(GetActiveWindow() == mdi_frame, "wrong active window %p\n", GetActiveWindow());
    ok(GetFocus() == mdi_child2, "wrong focus window %p\n", GetFocus());

    active_child = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, (LPARAM)&zoomed);
    ok(active_child == mdi_child2, "wrong active MDI child %p\n", active_child);
    ok(!zoomed, "wrong zoomed state %d\n", zoomed);
    flush_sequence();

    SetFocus(0);
    flush_sequence();

    ShowWindow(mdi_child2, SW_HIDE);
    ok_sequence(WmHideChildSeq, "ShowWindow(SW_HIDE):MDI child", FALSE);

    ok(GetActiveWindow() == mdi_frame, "wrong active window %p\n", GetActiveWindow());
    ok(GetFocus() == 0, "wrong focus window %p\n", GetFocus());

    DestroyWindow(mdi_child2);
    ok_sequence(WmDestroyMDIchildInvisibleSeq, "Destroy invisible MDI child window", FALSE);

    ok(GetActiveWindow() == mdi_frame, "wrong active window %p\n", GetActiveWindow());
    ok(GetFocus() == 0, "wrong focus window %p\n", GetFocus());

    /* test for maximized MDI children */
    trace("creating maximized visible MDI child window 1\n");
    mdi_child = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_class", "MDI child",
                                WS_CHILD | WS_VISIBLE | WS_MAXIMIZEBOX | WS_MAXIMIZE,
                                0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandleA(0), NULL);
    assert(mdi_child);
    ok_sequence(WmCreateMDIchildVisibleMaxSeq1, "Create maximized visible 1st MDI child window", TRUE);
    ok(IsZoomed(mdi_child), "1st MDI child should be maximized\n");

    ok(GetActiveWindow() == mdi_frame, "wrong active window %p\n", GetActiveWindow());
    ok(GetFocus() == mdi_child || /* win2k */
       GetFocus() == 0, /* win9x */
       "wrong focus window %p\n", GetFocus());

    active_child = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, (LPARAM)&zoomed);
    ok(active_child == mdi_child, "wrong active MDI child %p\n", active_child);
    ok(zoomed, "wrong zoomed state %d\n", zoomed);
    flush_sequence();

    trace("creating maximized visible MDI child window 2\n");
    mdi_child2 = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_class", "MDI child",
                                WS_CHILD | WS_VISIBLE | WS_MAXIMIZEBOX | WS_MAXIMIZE,
                                0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandleA(0), NULL);
    assert(mdi_child2);
    ok_sequence(WmCreateMDIchildVisibleMaxSeq2, "Create maximized visible 2nd MDI child 2 window", TRUE);
    ok(IsZoomed(mdi_child2), "2nd MDI child should be maximized\n");
    ok(!IsZoomed(mdi_child), "1st MDI child should NOT be maximized\n");

    ok(GetActiveWindow() == mdi_frame, "wrong active window %p\n", GetActiveWindow());
    ok(GetFocus() == mdi_child2, "wrong focus window %p\n", GetFocus());

    active_child = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, (LPARAM)&zoomed);
    ok(active_child == mdi_child2, "wrong active MDI child %p\n", active_child);
    ok(zoomed, "wrong zoomed state %d\n", zoomed);
    flush_sequence();

    trace("destroying maximized visible MDI child window 2\n");
    DestroyWindow(mdi_child2);
    ok_sequence(WmDestroyMDIchildVisibleSeq, "Destroy visible MDI child window", TRUE);

    ok(!IsZoomed(mdi_child), "1st MDI child should NOT be maximized\n");

    ok(GetActiveWindow() == mdi_frame, "wrong active window %p\n", GetActiveWindow());
    ok(GetFocus() == 0, "wrong focus window %p\n", GetFocus());

    /* Win2k: MDI client still returns a just destroyed child as active
     * Win9x: MDI client returns 0
     */
    active_child = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, (LPARAM)&zoomed);
    ok(active_child == mdi_child2 || /* win2k */
       !active_child, /* win9x */
       "wrong active MDI child %p\n", active_child);
    flush_sequence();

    ShowWindow(mdi_child, SW_MAXIMIZE);
    ok(IsZoomed(mdi_child), "1st MDI child should be maximized\n");
    flush_sequence();

    ok(GetActiveWindow() == mdi_frame, "wrong active window %p\n", GetActiveWindow());
    ok(GetFocus() == mdi_child, "wrong focus window %p\n", GetFocus());

    trace("re-creating maximized visible MDI child window 2\n");
    mdi_child2 = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_class", "MDI child",
                                WS_CHILD | WS_VISIBLE | WS_MAXIMIZEBOX | WS_MAXIMIZE,
                                0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandleA(0), NULL);
    assert(mdi_child2);
    ok_sequence(WmCreateMDIchildVisibleMaxSeq2, "Create maximized visible 2nd MDI child 2 window", TRUE);
    ok(IsZoomed(mdi_child2), "2nd MDI child should be maximized\n");
    ok(!IsZoomed(mdi_child), "1st MDI child should NOT be maximized\n");

    ok(GetActiveWindow() == mdi_frame, "wrong active window %p\n", GetActiveWindow());
    ok(GetFocus() == mdi_child2, "wrong focus window %p\n", GetFocus());

    active_child = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, (LPARAM)&zoomed);
    ok(active_child == mdi_child2, "wrong active MDI child %p\n", active_child);
    ok(zoomed, "wrong zoomed state %d\n", zoomed);
    flush_sequence();

    SendMessageA(mdi_child2, WM_SYSCOMMAND, SC_CLOSE, 0);
    ok_sequence(WmDestroyMDIchildVisibleMaxSeq2, "WM_SYSCOMMAND/SC_CLOSE on a visible maximized MDI child window", TRUE);
    ok(!IsWindow(mdi_child2), "MDI child 2 should be destroyed\n");

    ok(IsZoomed(mdi_child), "1st MDI child should be maximized\n");
    ok(GetActiveWindow() == mdi_frame, "wrong active window %p\n", GetActiveWindow());
    ok(GetFocus() == mdi_child, "wrong focus window %p\n", GetFocus());

    active_child = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, (LPARAM)&zoomed);
    ok(active_child == mdi_child, "wrong active MDI child %p\n", active_child);
    ok(zoomed, "wrong zoomed state %d\n", zoomed);
    flush_sequence();

    DestroyWindow(mdi_child);
    ok_sequence(WmDestroyMDIchildVisibleSeq, "Destroy visible MDI child window", TRUE);

    ok(GetActiveWindow() == mdi_frame, "wrong active window %p\n", GetActiveWindow());
    ok(GetFocus() == 0, "wrong focus window %p\n", GetFocus());

    /* Win2k: MDI client still returns a just destroyed child as active
     * Win9x: MDI client returns 0
     */
    active_child = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, (LPARAM)&zoomed);
    ok(active_child == mdi_child || /* win2k */
       !active_child, /* win9x */
       "wrong active MDI child %p\n", active_child);
    flush_sequence();

    trace("creating maximized invisible MDI child window\n");
    mdi_child2 = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_class", "MDI child",
                                WS_CHILD | WS_MAXIMIZE | WS_CAPTION | WS_THICKFRAME,
                                0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandleA(0), NULL);
    assert(mdi_child2);
    ok_sequence(WmCreateMDIchildInvisibleMaxSeq4, "Create maximized invisible MDI child window", FALSE);
    ok(IsZoomed(mdi_child2), "MDI child should be maximized\n");
    ok(!(GetWindowLongA(mdi_child2, GWL_STYLE) & WS_VISIBLE), "MDI child should be not visible\n");
    ok(!IsWindowVisible(mdi_child2), "MDI child should be not visible\n");

    /* Win2k: MDI client still returns a just destroyed child as active
     * Win9x: MDI client returns 0
     */
    active_child = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, (LPARAM)&zoomed);
    ok(active_child == mdi_child || /* win2k */
       !active_child || active_child == mdi_child2, /* win9x */
       "wrong active MDI child %p\n", active_child);
    flush_sequence();

    trace("call ShowWindow(mdi_child, SW_MAXIMIZE)\n");
    ShowWindow(mdi_child2, SW_MAXIMIZE);
    ok_sequence(WmMaximizeMDIchildInvisibleSeq2, "ShowWindow(SW_MAXIMIZE):invisible maximized MDI child", FALSE);
    ok(IsZoomed(mdi_child2), "MDI child should be maximized\n");
    ok(GetWindowLongA(mdi_child2, GWL_STYLE) & WS_VISIBLE, "MDI child should be visible\n");
    ok(IsWindowVisible(mdi_child2), "MDI child should be visible\n");

    active_child = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, (LPARAM)&zoomed);
    ok(active_child == mdi_child2, "wrong active MDI child %p\n", active_child);
    ok(zoomed, "wrong zoomed state %d\n", zoomed);
    flush_sequence();

    SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child2, 0);
    flush_sequence();

    /* end of test for maximized MDI children */
    SetFocus(0);
    flush_sequence();
    trace("creating maximized visible MDI child window 1(Switch test)\n");
    mdi_child = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_class", "MDI child",
                                WS_CHILD | WS_VISIBLE | WS_MAXIMIZEBOX | WS_MAXIMIZE,
                                0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandleA(0), NULL);
    assert(mdi_child);
    ok_sequence(WmCreateMDIchildVisibleMaxSeq1, "Create maximized visible 1st MDI child window(Switch test)", TRUE);
    ok(IsZoomed(mdi_child), "1st MDI child should be maximized(Switch test)\n");

    ok(GetActiveWindow() == mdi_frame, "wrong active window %p(Switch test)\n", GetActiveWindow());
    ok(GetFocus() == mdi_child || /* win2k */
       GetFocus() == 0, /* win9x */
       "wrong focus window %p(Switch test)\n", GetFocus());

    active_child = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, (LPARAM)&zoomed);
    ok(active_child == mdi_child, "wrong active MDI child %p(Switch test)\n", active_child);
    ok(zoomed, "wrong zoomed state %d(Switch test)\n", zoomed);
    flush_sequence();

    trace("creating maximized visible MDI child window 2(Switch test)\n");
    mdi_child2 = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_class", "MDI child",
                                WS_CHILD | WS_VISIBLE | WS_MAXIMIZEBOX | WS_MAXIMIZE,
                                0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
                                mdi_client, 0, GetModuleHandleA(0), NULL);
    assert(mdi_child2);
    ok_sequence(WmCreateMDIchildVisibleMaxSeq2, "Create maximized visible 2nd MDI child window (Switch test)", TRUE);

    ok(IsZoomed(mdi_child2), "2nd MDI child should be maximized(Switch test)\n");
    ok(!IsZoomed(mdi_child), "1st MDI child should NOT be maximized(Switch test)\n");

    ok(GetActiveWindow() == mdi_frame, "wrong active window %p(Switch test)\n", GetActiveWindow());
    ok(GetFocus() == mdi_child2, "wrong focus window %p(Switch test)\n", GetFocus());

    active_child = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, (LPARAM)&zoomed);
    ok(active_child == mdi_child2, "wrong active MDI child %p(Switch test)\n", active_child);
    ok(zoomed, "wrong zoomed state %d(Switch test)\n", zoomed);
    flush_sequence();

    trace("Switch child window.\n");
    SendMessageA(mdi_client, WM_MDIACTIVATE, (WPARAM)mdi_child, 0);
    ok_sequence(WmSwitchChild, "Child did not switch correctly", TRUE);
    trace("end of test for switch maximized MDI children\n");
    flush_sequence();

    /* Prepare for switching test of not maximized MDI children  */
    ShowWindow( mdi_child, SW_NORMAL );
    ok(!IsZoomed(mdi_child), "wrong zoomed state for %p(Switch test)\n", mdi_child);
    ok(!IsZoomed(mdi_child2), "wrong zoomed state for %p(Switch test)\n", mdi_child2);
    active_child = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, 0);
    ok(active_child == mdi_child, "wrong active MDI child %p(Switch test)\n", active_child);
    flush_sequence();

    SendMessageA(mdi_client, WM_MDIACTIVATE, (WPARAM)mdi_child2, 0);
    ok_sequence(WmSwitchNotMaximizedChild, "Not maximized child did not switch correctly", FALSE);
    trace("end of test for switch not maximized MDI children\n");
    flush_sequence();

    SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
    flush_sequence();

    SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child2, 0);
    flush_sequence();

    SetFocus(0);
    flush_sequence();
    /* end of tests for switch maximized/not maximized MDI children */

    mdi_cs.szClass = "MDI_child_Class";
    mdi_cs.szTitle = "MDI child";
    mdi_cs.hOwner = GetModuleHandleA(0);
    mdi_cs.x = 0;
    mdi_cs.y = 0;
    mdi_cs.cx = CW_USEDEFAULT;
    mdi_cs.cy = CW_USEDEFAULT;
    mdi_cs.style = WS_CHILD | WS_SYSMENU | WS_VISIBLE | WS_MAXIMIZEBOX | WS_MAXIMIZE;
    mdi_cs.lParam = 0;
    mdi_child = (HWND)SendMessageA(mdi_client, WM_MDICREATE, 0, (LPARAM)&mdi_cs);
    ok(mdi_child != 0, "MDI child creation failed\n");
    ok_sequence(WmCreateMDIchildVisibleMaxSeq3, "WM_MDICREATE for maximized visible MDI child window", TRUE);

    ok(GetMenuItemID(hMenu, GetMenuItemCount(hMenu) - 1) == SC_CLOSE, "SC_CLOSE menu item not found\n");

    active_child = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, (LPARAM)&zoomed);
    ok(active_child == mdi_child, "wrong active MDI child %p\n", active_child);

    ok(IsZoomed(mdi_child), "MDI child should be maximized\n");
    ok(GetActiveWindow() == mdi_frame, "wrong active window %p\n", GetActiveWindow());
    ok(GetFocus() == mdi_child, "wrong focus window %p\n", GetFocus());

    active_child = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, (LPARAM)&zoomed);
    ok(active_child == mdi_child, "wrong active MDI child %p\n", active_child);
    ok(zoomed, "wrong zoomed state %d\n", zoomed);
    flush_sequence();

    SendMessageA(mdi_client, WM_MDIDESTROY, (WPARAM)mdi_child, 0);
    ok_sequence(WmDestroyMDIchildVisibleMaxSeq1, "Destroy visible maximized MDI child window", TRUE);

    ok(!IsWindow(mdi_child), "MDI child should be destroyed\n");
    active_child = (HWND)SendMessageA(mdi_client, WM_MDIGETACTIVE, 0, (LPARAM)&zoomed);
    ok(!active_child, "wrong active MDI child %p\n", active_child);

    SetFocus(0);
    flush_sequence();

    DestroyWindow(mdi_client);
    ok_sequence(WmDestroyMDIclientSeq, "Destroy MDI client window", FALSE);

    /* test maximization of MDI child with invisible parent */
    client_cs.hWindowMenu = 0;
    mdi_client = CreateWindowA("MDI_client_class",
                                 NULL,
                                 WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL | WS_VISIBLE,
                                 0, 0, 660, 430,
                                 mdi_frame, 0, GetModuleHandleA(0), &client_cs);
    ok_sequence(WmCreateMDIclientSeq, "Create MDI client window", FALSE);

    ShowWindow(mdi_client, SW_HIDE);
    ok_sequence(WmHideMDIclientSeq, "Hide MDI client window", FALSE);

    mdi_child = CreateWindowExA(WS_EX_MDICHILD, "MDI_child_class", "MDI child",
                                WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL,
                                0, 0, 650, 440,
                                mdi_client, 0, GetModuleHandleA(0), NULL);
    ok_sequence(WmCreateMDIchildInvisibleParentSeq, "Create MDI child window with invisible parent", FALSE);

    SendMessageA(mdi_client, WM_MDIMAXIMIZE, (WPARAM) mdi_child, 0);
    ok_sequence(WmMaximizeMDIchildInvisibleParentSeq, "Maximize MDI child window with invisible parent", TRUE);
    zoomed = IsZoomed(mdi_child);
    ok(zoomed, "wrong zoomed state %d\n", zoomed);

    ShowWindow(mdi_client, SW_SHOW);
    ok_sequence(WmShowMDIclientSeq, "Show MDI client window", FALSE);

    DestroyWindow(mdi_child);
    ok_sequence(WmDestroyMDIchildVisibleSeq, "Destroy visible maximized MDI child window", TRUE);

    /* end of test for maximization of MDI child with invisible parent */

    DestroyWindow(mdi_client);
    ok_sequence(WmDestroyMDIclientSeq, "Destroy MDI client window", FALSE);

    DestroyWindow(mdi_frame);
    ok_sequence(WmDestroyMDIframeSeq, "Destroy MDI frame window", FALSE);
}
/************************* End of MDI test **********************************/

static void test_WM_SETREDRAW(HWND hwnd)
{
    DWORD style = GetWindowLongA(hwnd, GWL_STYLE);

    flush_events();
    flush_sequence();

    SendMessageA(hwnd, WM_SETREDRAW, FALSE, 0);
    ok_sequence(WmSetRedrawFalseSeq, "SetRedraw:FALSE", FALSE);

    ok(!(GetWindowLongA(hwnd, GWL_STYLE) & WS_VISIBLE), "WS_VISIBLE should NOT be set\n");
    ok(!IsWindowVisible(hwnd), "IsWindowVisible() should return FALSE\n");

    flush_sequence();
    SendMessageA(hwnd, WM_SETREDRAW, TRUE, 0);
    ok_sequence(WmSetRedrawTrueSeq, "SetRedraw:TRUE", FALSE);

    ok(GetWindowLongA(hwnd, GWL_STYLE) & WS_VISIBLE, "WS_VISIBLE should be set\n");
    ok(IsWindowVisible(hwnd), "IsWindowVisible() should return TRUE\n");

    /* restore original WS_VISIBLE state */
    SetWindowLongA(hwnd, GWL_STYLE, style);

    flush_events();
    flush_sequence();
}

static INT_PTR CALLBACK TestModalDlgProcA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct recvd_message msg;

    if (ignore_message( message )) return 0;

    switch (message)
    {
	/* ignore */
	case WM_MOUSEMOVE:
	case WM_NCMOUSEMOVE:
	case WM_NCMOUSELEAVE:
	case WM_SETCURSOR:
            return 0;
        case WM_NCHITTEST:
            return HTCLIENT;
    }

    msg.hwnd = hwnd;
    msg.message = message;
    msg.flags = sent|wparam|lparam;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.descr = "dialog";
    add_message(&msg);

    if (message == WM_INITDIALOG) SetTimer( hwnd, 1, 100, NULL );
    if (message == WM_TIMER) EndDialog( hwnd, 0 );
    return 0;
}

static void test_hv_scroll_1(HWND hwnd, INT ctl, DWORD clear, DWORD set, INT min, INT max)
{
    DWORD style, exstyle;
    INT xmin, xmax;
    BOOL ret;

    exstyle = GetWindowLongA(hwnd, GWL_EXSTYLE);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    /* do not be confused by WS_DLGFRAME set */
    if ((style & WS_CAPTION) == WS_CAPTION) style &= ~WS_CAPTION;

    if (clear) ok(style & clear, "style %08x should be set\n", clear);
    if (set) ok(!(style & set), "style %08x should not be set\n", set);

    ret = SetScrollRange(hwnd, ctl, min, max, FALSE);
    ok( ret, "SetScrollRange(%d) error %d\n", ctl, GetLastError());
    if ((style & (WS_DLGFRAME | WS_BORDER | WS_THICKFRAME)) || (exstyle & WS_EX_DLGMODALFRAME))
        ok_sequence(WmSetScrollRangeHV_NC_Seq, "SetScrollRange(SB_HORZ/SB_VERT) NC", FALSE);
    else
        ok_sequence(WmSetScrollRangeHVSeq, "SetScrollRange(SB_HORZ/SB_VERT)", FALSE);

    style = GetWindowLongA(hwnd, GWL_STYLE);
    if (set) ok(style & set, "style %08x should be set\n", set);
    if (clear) ok(!(style & clear), "style %08x should not be set\n", clear);

    /* a subsequent call should do nothing */
    ret = SetScrollRange(hwnd, ctl, min, max, FALSE);
    ok( ret, "SetScrollRange(%d) error %d\n", ctl, GetLastError());
    ok_sequence(WmEmptySeq, "SetScrollRange(SB_HORZ/SB_VERT) empty sequence", FALSE);

    xmin = 0xdeadbeef;
    xmax = 0xdeadbeef;
    ret = GetScrollRange(hwnd, ctl, &xmin, &xmax);
    ok( ret, "GetScrollRange(%d) error %d\n", ctl, GetLastError());
    ok_sequence(WmEmptySeq, "GetScrollRange(SB_HORZ/SB_VERT) empty sequence", FALSE);
    ok(xmin == min, "unexpected min scroll value %d\n", xmin);
    ok(xmax == max, "unexpected max scroll value %d\n", xmax);
}

static void test_hv_scroll_2(HWND hwnd, INT ctl, DWORD clear, DWORD set, INT min, INT max)
{
    DWORD style, exstyle;
    SCROLLINFO si;
    BOOL ret;

    exstyle = GetWindowLongA(hwnd, GWL_EXSTYLE);
    style = GetWindowLongA(hwnd, GWL_STYLE);
    /* do not be confused by WS_DLGFRAME set */
    if ((style & WS_CAPTION) == WS_CAPTION) style &= ~WS_CAPTION;

    if (clear) ok(style & clear, "style %08x should be set\n", clear);
    if (set) ok(!(style & set), "style %08x should not be set\n", set);

    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE;
    si.nMin = min;
    si.nMax = max;
    SetScrollInfo(hwnd, ctl, &si, TRUE);
    if ((style & (WS_DLGFRAME | WS_BORDER | WS_THICKFRAME)) || (exstyle & WS_EX_DLGMODALFRAME))
        ok_sequence(WmSetScrollRangeHV_NC_Seq, "SetScrollInfo(SB_HORZ/SB_VERT) NC", FALSE);
    else
        ok_sequence(WmSetScrollRangeHVSeq, "SetScrollInfo(SB_HORZ/SB_VERT)", FALSE);

    style = GetWindowLongA(hwnd, GWL_STYLE);
    if (set) ok(style & set, "style %08x should be set\n", set);
    if (clear) ok(!(style & clear), "style %08x should not be set\n", clear);

    /* a subsequent call should do nothing */
    SetScrollInfo(hwnd, ctl, &si, TRUE);
    if (style & WS_HSCROLL)
        ok_sequence(WmSetScrollRangeHSeq_empty, "SetScrollInfo(SB_HORZ/SB_VERT) empty sequence", FALSE);
    else if (style & WS_VSCROLL)
        ok_sequence(WmSetScrollRangeVSeq_empty, "SetScrollInfo(SB_HORZ/SB_VERT) empty sequence", FALSE);
    else
        ok_sequence(WmEmptySeq, "SetScrollInfo(SB_HORZ/SB_VERT) empty sequence", FALSE);

    si.fMask = SIF_PAGE;
    si.nPage = 5;
    SetScrollInfo(hwnd, ctl, &si, FALSE);
    ok_sequence(WmEmptySeq, "SetScrollInfo(SB_HORZ/SB_VERT) empty sequence", FALSE);

    si.fMask = SIF_POS;
    si.nPos = max - 1;
    SetScrollInfo(hwnd, ctl, &si, FALSE);
    ok_sequence(WmEmptySeq, "SetScrollInfo(SB_HORZ/SB_VERT) empty sequence", FALSE);

    si.fMask = SIF_RANGE;
    si.nMin = 0xdeadbeef;
    si.nMax = 0xdeadbeef;
    ret = GetScrollInfo(hwnd, ctl, &si);
    ok( ret, "GetScrollInfo error %d\n", GetLastError());
    ok_sequence(WmEmptySeq, "GetScrollRange(SB_HORZ/SB_VERT) empty sequence", FALSE);
    ok(si.nMin == min, "unexpected min scroll value %d\n", si.nMin);
    ok(si.nMax == max, "unexpected max scroll value %d\n", si.nMax);
}

/* Win9x sends WM_USER+xxx while and NT versions send SBM_xxx messages */
static void test_scroll_messages(HWND hwnd)
{
    SCROLLINFO si;
    INT min, max;
    BOOL ret;

    flush_events();
    flush_sequence();

    min = 0xdeadbeef;
    max = 0xdeadbeef;
    ret = GetScrollRange(hwnd, SB_CTL, &min, &max);
    ok( ret, "GetScrollRange error %d\n", GetLastError());
    if (sequence->message != WmGetScrollRangeSeq[0].message)
        trace("GetScrollRange(SB_CTL) generated unknown message %04x\n", sequence->message);
    /* values of min and max are undefined */
    flush_sequence();

    ret = SetScrollRange(hwnd, SB_CTL, 10, 150, FALSE);
    ok( ret, "SetScrollRange error %d\n", GetLastError());
    if (sequence->message != WmSetScrollRangeSeq[0].message)
        trace("SetScrollRange(SB_CTL) generated unknown message %04x\n", sequence->message);
    flush_sequence();

    min = 0xdeadbeef;
    max = 0xdeadbeef;
    ret = GetScrollRange(hwnd, SB_CTL, &min, &max);
    ok( ret, "GetScrollRange error %d\n", GetLastError());
    if (sequence->message != WmGetScrollRangeSeq[0].message)
        trace("GetScrollRange(SB_CTL) generated unknown message %04x\n", sequence->message);
    /* values of min and max are undefined */
    flush_sequence();

    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE;
    si.nMin = 20;
    si.nMax = 160;
    SetScrollInfo(hwnd, SB_CTL, &si, FALSE);
    if (sequence->message != WmSetScrollRangeSeq[0].message)
        trace("SetScrollInfo(SB_CTL) generated unknown message %04x\n", sequence->message);
    flush_sequence();

    si.fMask = SIF_PAGE;
    si.nPage = 10;
    SetScrollInfo(hwnd, SB_CTL, &si, FALSE);
    if (sequence->message != WmSetScrollRangeSeq[0].message)
        trace("SetScrollInfo(SB_CTL) generated unknown message %04x\n", sequence->message);
    flush_sequence();

    si.fMask = SIF_POS;
    si.nPos = 20;
    SetScrollInfo(hwnd, SB_CTL, &si, FALSE);
    if (sequence->message != WmSetScrollRangeSeq[0].message)
        trace("SetScrollInfo(SB_CTL) generated unknown message %04x\n", sequence->message);
    flush_sequence();

    si.fMask = SIF_RANGE;
    si.nMin = 0xdeadbeef;
    si.nMax = 0xdeadbeef;
    ret = GetScrollInfo(hwnd, SB_CTL, &si);
    ok( ret, "GetScrollInfo error %d\n", GetLastError());
    if (sequence->message != WmGetScrollInfoSeq[0].message)
        trace("GetScrollInfo(SB_CTL) generated unknown message %04x\n", sequence->message);
    /* values of min and max are undefined */
    flush_sequence();

    /* set WS_HSCROLL */
    test_hv_scroll_1(hwnd, SB_HORZ, 0, WS_HSCROLL, 10, 150);
    /* clear WS_HSCROLL */
    test_hv_scroll_1(hwnd, SB_HORZ, WS_HSCROLL, 0, 0, 0);

    /* set WS_HSCROLL */
    test_hv_scroll_2(hwnd, SB_HORZ, 0, WS_HSCROLL, 10, 150);
    /* clear WS_HSCROLL */
    test_hv_scroll_2(hwnd, SB_HORZ, WS_HSCROLL, 0, 0, 0);

    /* set WS_VSCROLL */
    test_hv_scroll_1(hwnd, SB_VERT, 0, WS_VSCROLL, 10, 150);
    /* clear WS_VSCROLL */
    test_hv_scroll_1(hwnd, SB_VERT, WS_VSCROLL, 0, 0, 0);

    /* set WS_VSCROLL */
    test_hv_scroll_2(hwnd, SB_VERT, 0, WS_VSCROLL, 10, 150);
    /* clear WS_VSCROLL */
    test_hv_scroll_2(hwnd, SB_VERT, WS_VSCROLL, 0, 0, 0);
}

static void test_showwindow(void)
{
    HWND hwnd, hchild;
    RECT rc;

    hwnd = CreateWindowExA(0, "TestWindowClass", "Test overlapped", WS_OVERLAPPEDWINDOW,
                           100, 100, 200, 200, 0, 0, 0, NULL);
    ok (hwnd != 0, "Failed to create overlapped window\n");
    hchild = CreateWindowExA(0, "TestWindowClass", "Test child", WS_CHILD,
                             0, 0, 10, 10, hwnd, 0, 0, NULL);
    ok (hchild != 0, "Failed to create child\n");
    flush_sequence();

    /* ShowWindow( SW_SHOWNA) for invisible top level window */
    trace("calling ShowWindow( SW_SHOWNA) for invisible top level window\n");
    ok( ShowWindow(hwnd, SW_SHOWNA) == FALSE, "ShowWindow: window was visible\n" );
    ok_sequence(WmSHOWNATopInvisible, "ShowWindow(SW_SHOWNA) on invisible top level window", FALSE);

    /* ShowWindow( SW_SHOWNA) for now visible top level window */
    trace("calling ShowWindow( SW_SHOWNA) for now visible top level window\n");
    ok( ShowWindow(hwnd, SW_SHOWNA) != FALSE, "ShowWindow: window was invisible\n" );
    ok_sequence(WmSHOWNATopVisible, "ShowWindow(SW_SHOWNA) on visible top level window", FALSE);
    /* back to invisible */
    ShowWindow(hchild, SW_HIDE);
    ShowWindow(hwnd, SW_HIDE);
    flush_sequence();
    /* ShowWindow(SW_SHOWNA) with child and parent invisible */ 
    trace("calling ShowWindow( SW_SHOWNA) for invisible child with invisible parent\n");
    ok( ShowWindow(hchild, SW_SHOWNA) == FALSE, "ShowWindow: window was visible\n" );
    ok_sequence(WmSHOWNAChildInvisParInvis, "ShowWindow(SW_SHOWNA) invisible child and parent", FALSE);
    /* ShowWindow(SW_SHOWNA) with child visible and parent invisible */ 
    ok( ShowWindow(hchild, SW_SHOW) != FALSE, "ShowWindow: window was invisible\n" );
    flush_sequence();
    trace("calling ShowWindow( SW_SHOWNA) for the visible child and invisible parent\n");
    ok( ShowWindow(hchild, SW_SHOWNA) != FALSE, "ShowWindow: window was invisible\n" );
    ok_sequence(WmSHOWNAChildVisParInvis, "ShowWindow(SW_SHOWNA) visible child and invisible parent", FALSE);
    /* ShowWindow(SW_SHOWNA) with child visible and parent visible */
    ShowWindow( hwnd, SW_SHOW);
    flush_sequence();
    trace("calling ShowWindow( SW_SHOWNA) for the visible child and parent\n");
    ok( ShowWindow(hchild, SW_SHOWNA) != FALSE, "ShowWindow: window was invisible\n" );
    ok_sequence(WmSHOWNAChildVisParVis, "ShowWindow(SW_SHOWNA) for the visible child and parent", FALSE);

    /* ShowWindow(SW_SHOWNA) with child invisible and parent visible */
    ShowWindow( hchild, SW_HIDE);
    flush_sequence();
    trace("calling ShowWindow( SW_SHOWNA) for the invisible child and visible parent\n");
    ok( ShowWindow(hchild, SW_SHOWNA) == FALSE, "ShowWindow: window was visible\n" );
    ok_sequence(WmSHOWNAChildInvisParVis, "ShowWindow(SW_SHOWNA) for the invisible child and visible parent", FALSE);

    SetCapture(hchild);
    ok(GetCapture() == hchild, "wrong capture window %p\n", GetCapture());
    DestroyWindow(hchild);
    ok(!GetCapture(), "wrong capture window %p\n", GetCapture());

    DestroyWindow(hwnd);
    flush_sequence();

    /* Popup windows */
    /* Test 1:
     * 1. Create invisible maximized popup window.
     * 2. Move and resize it.
     * 3. Show it maximized.
     */
    trace("calling CreateWindowExA( WS_MAXIMIZE ) for invisible maximized popup window\n");
    hwnd = CreateWindowExA(0, "TestWindowClass", "Test popup", WS_POPUP | WS_MAXIMIZE,
                           100, 100, 200, 200, 0, 0, 0, NULL);
    ok (hwnd != 0, "Failed to create popup window\n");
    ok(IsZoomed(hwnd), "window should be maximized\n");
    ok_sequence(WmCreateInvisibleMaxPopupSeq, "CreateWindow(WS_MAXIMIZED):popup", FALSE);

    GetWindowRect(hwnd, &rc);
    ok( rc.right-rc.left == GetSystemMetrics(SM_CXSCREEN) &&
        rc.bottom-rc.top == GetSystemMetrics(SM_CYSCREEN),
        "Invalid maximized size before ShowWindow (%d,%d)-(%d,%d)\n",
        rc.left, rc.top, rc.right, rc.bottom);
    /* Reset window's size & position */
    SetWindowPos(hwnd, 0, 10, 10, 200, 200, SWP_NOZORDER | SWP_NOACTIVATE);
    ok(IsZoomed(hwnd), "window should be maximized\n");
    flush_sequence();

    trace("calling ShowWindow( SW_SHOWMAXIMIZE ) for invisible maximized popup window\n");
    ShowWindow(hwnd, SW_SHOWMAXIMIZED);
    ok(IsZoomed(hwnd), "window should be maximized\n");
    ok_sequence(WmShowMaxPopupResizedSeq, "ShowWindow(SW_SHOWMAXIMIZED):invisible maximized and resized popup", FALSE);

    GetWindowRect(hwnd, &rc);
    ok( rc.right-rc.left == GetSystemMetrics(SM_CXSCREEN) &&
        rc.bottom-rc.top == GetSystemMetrics(SM_CYSCREEN),
        "Invalid maximized size after ShowWindow (%d,%d)-(%d,%d)\n",
        rc.left, rc.top, rc.right, rc.bottom);
    DestroyWindow(hwnd);
    flush_sequence();

    /* Test 2:
     * 1. Create invisible maximized popup window.
     * 2. Show it maximized.
     */
    trace("calling CreateWindowExA( WS_MAXIMIZE ) for invisible maximized popup window\n");
    hwnd = CreateWindowExA(0, "TestWindowClass", "Test popup", WS_POPUP | WS_MAXIMIZE,
                           100, 100, 200, 200, 0, 0, 0, NULL);
    ok (hwnd != 0, "Failed to create popup window\n");
    ok(IsZoomed(hwnd), "window should be maximized\n");
    ok_sequence(WmCreateInvisibleMaxPopupSeq, "CreateWindow(WS_MAXIMIZED):popup", FALSE);

    trace("calling ShowWindow( SW_SHOWMAXIMIZE ) for invisible maximized popup window\n");
    ShowWindow(hwnd, SW_SHOWMAXIMIZED);
    ok(IsZoomed(hwnd), "window should be maximized\n");
    ok_sequence(WmShowMaxPopupSeq, "ShowWindow(SW_SHOWMAXIMIZED):invisible maximized popup", FALSE);
    DestroyWindow(hwnd);
    flush_sequence();

    /* Test 3:
     * 1. Create visible maximized popup window.
     */
    trace("calling CreateWindowExA( WS_MAXIMIZE ) for maximized popup window\n");
    hwnd = CreateWindowExA(0, "TestWindowClass", "Test popup", WS_POPUP | WS_MAXIMIZE | WS_VISIBLE,
                           100, 100, 200, 200, 0, 0, 0, NULL);
    ok (hwnd != 0, "Failed to create popup window\n");
    ok(IsZoomed(hwnd), "window should be maximized\n");
    ok_sequence(WmCreateMaxPopupSeq, "CreateWindow(WS_MAXIMIZED):popup", FALSE);
    DestroyWindow(hwnd);
    flush_sequence();

    /* Test 4:
     * 1. Create visible popup window.
     * 2. Maximize it.
     */
    trace("calling CreateWindowExA( WS_VISIBLE ) for popup window\n");
    hwnd = CreateWindowExA(0, "TestWindowClass", "Test popup", WS_POPUP | WS_VISIBLE,
                           100, 100, 200, 200, 0, 0, 0, NULL);
    ok (hwnd != 0, "Failed to create popup window\n");
    ok(!IsZoomed(hwnd), "window should NOT be maximized\n");
    ok_sequence(WmCreatePopupSeq, "CreateWindow(WS_VISIBLE):popup", FALSE);

    trace("calling ShowWindow( SW_SHOWMAXIMIZE ) for visible popup window\n");
    ShowWindow(hwnd, SW_SHOWMAXIMIZED);
    ok(IsZoomed(hwnd), "window should be maximized\n");
    ok_sequence(WmShowVisMaxPopupSeq, "ShowWindow(SW_SHOWMAXIMIZED):popup", FALSE);
    DestroyWindow(hwnd);
    flush_sequence();
}

static void test_sys_menu(void)
{
    HWND hwnd;
    HMENU hmenu;
    UINT state;

    hwnd = CreateWindowExA(0, "TestWindowClass", NULL, WS_OVERLAPPEDWINDOW,
                           100, 100, 200, 200, 0, 0, 0, NULL);
    ok (hwnd != 0, "Failed to create overlapped window\n");

    flush_sequence();

    /* test existing window without CS_NOCLOSE style */
    hmenu = GetSystemMenu(hwnd, FALSE);
    ok(hmenu != 0, "GetSystemMenu error %d\n", GetLastError());

    state = GetMenuState(hmenu, SC_CLOSE, MF_BYCOMMAND);
    ok(state != 0xffffffff, "wrong SC_CLOSE state %x\n", state);
    ok(!(state & (MF_DISABLED | MF_GRAYED)), "wrong SC_CLOSE state %x\n", state);

    EnableMenuItem(hmenu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
    ok_sequence(WmEmptySeq, "WmEnableMenuItem", FALSE);

    state = GetMenuState(hmenu, SC_CLOSE, MF_BYCOMMAND);
    ok(state != 0xffffffff, "wrong SC_CLOSE state %x\n", state);
    ok((state & (MF_DISABLED | MF_GRAYED)) == MF_GRAYED, "wrong SC_CLOSE state %x\n", state);

    EnableMenuItem(hmenu, SC_CLOSE, 0);
    ok_sequence(WmEmptySeq, "WmEnableMenuItem", FALSE);

    state = GetMenuState(hmenu, SC_CLOSE, MF_BYCOMMAND);
    ok(state != 0xffffffff, "wrong SC_CLOSE state %x\n", state);
    ok(!(state & (MF_DISABLED | MF_GRAYED)), "wrong SC_CLOSE state %x\n", state);

    /* test whether removing WS_SYSMENU destroys a system menu */
    SetWindowLongW(hwnd, GWL_STYLE, WS_POPUP);
    SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_FRAMECHANGED);
    flush_sequence();
    hmenu = GetSystemMenu(hwnd, FALSE);
    ok(hmenu != 0, "GetSystemMenu error %d\n", GetLastError());

    DestroyWindow(hwnd);

    /* test new window with CS_NOCLOSE style */
    hwnd = CreateWindowExA(0, "NoCloseWindowClass", NULL, WS_OVERLAPPEDWINDOW,
                           100, 100, 200, 200, 0, 0, 0, NULL);
    ok (hwnd != 0, "Failed to create overlapped window\n");

    hmenu = GetSystemMenu(hwnd, FALSE);
    ok(hmenu != 0, "GetSystemMenu error %d\n", GetLastError());

    state = GetMenuState(hmenu, SC_CLOSE, MF_BYCOMMAND);
    ok(state == 0xffffffff, "wrong SC_CLOSE state %x\n", state);

    DestroyWindow(hwnd);

    /* test new window without WS_SYSMENU style */
    hwnd = CreateWindowExA(0, "NoCloseWindowClass", NULL, WS_OVERLAPPEDWINDOW & ~WS_SYSMENU,
                           100, 100, 200, 200, 0, 0, 0, NULL);
    ok(hwnd != 0, "Failed to create overlapped window\n");

    hmenu = GetSystemMenu(hwnd, FALSE);
    ok(!hmenu, "GetSystemMenu error %d\n", GetLastError());

    DestroyWindow(hwnd);
}

/* For shown WS_OVERLAPPEDWINDOW */
static const struct message WmSetIcon_1[] = {
    { WM_SETICON, sent },
    { 0x00AE, sent|defwinproc|optional }, /* XP */
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_GETTEXT, sent|defwinproc|optional }, /* XP sends a duplicate */
    { 0 }
};

/* For WS_POPUP and hidden WS_OVERLAPPEDWINDOW */
static const struct message WmSetIcon_2[] = {
    { WM_SETICON, sent },
    { 0 }
};

/* Sending undocumented 0x3B message with wparam = 0x8000000b */
static const struct message WmInitEndSession[] = {
    { 0x003B, sent },
    { WM_QUERYENDSESSION, sent|defwinproc|wparam|lparam, 0, ENDSESSION_LOGOFF },
    { 0 }
};

/* Sending undocumented 0x3B message with wparam = 0x0000000b */
static const struct message WmInitEndSession_2[] = {
    { 0x003B, sent },
    { WM_QUERYENDSESSION, sent|defwinproc|wparam|lparam, 0, 0 },
    { 0 }
};

/* Sending undocumented 0x3B message with wparam = 0x80000008 */
static const struct message WmInitEndSession_3[] = {
    { 0x003B, sent },
    { WM_ENDSESSION, sent|defwinproc|wparam|lparam, 0, ENDSESSION_LOGOFF },
    { 0 }
};

/* Sending undocumented 0x3B message with wparam = 0x00000008 */
static const struct message WmInitEndSession_4[] = {
    { 0x003B, sent },
    { WM_ENDSESSION, sent|defwinproc|wparam|lparam, 0, 0 },
    { 0 }
};

/* Sending undocumented 0x3B message with wparam = 0x80000001 */
static const struct message WmInitEndSession_5[] = {
    { 0x003B, sent },
    { WM_ENDSESSION, sent|defwinproc/*|wparam*/|lparam, 1, ENDSESSION_LOGOFF },
    { 0 }
};

static const struct message WmOptionalPaint[] = {
    { WM_PAINT, sent|optional },
    { WM_NCPAINT, sent|beginpaint|optional },
    { WM_GETTEXT, sent|beginpaint|defwinproc|optional },
    { WM_ERASEBKGND, sent|beginpaint|optional },
    { 0 }
};

static const struct message WmZOrder[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam, 0, 0 },
    { WM_GETMINMAXINFO, sent|defwinproc|wparam, 0, 0 },
    { HCBT_ACTIVATE, hook },
    { WM_QUERYNEWPALETTE, sent|wparam|lparam|optional, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, 3, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_NOREDRAW|SWP_NOMOVE|SWP_NOSIZE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE, 0 },
    { WM_GETTEXT, sent|optional },
    { WM_NCCALCSIZE, sent|wparam|optional, 1 },
    { WM_ACTIVATEAPP, sent|wparam, 1, 0 },
    { WM_NCACTIVATE, sent|lparam, 1, 0 },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ACTIVATE, sent|wparam|lparam, 1, 0 },
    { HCBT_SETFOCUS, hook },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { WM_SETFOCUS, sent|wparam|defwinproc, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOSIZE|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_GETTEXT, sent|optional },
    { WM_NCCALCSIZE, sent|optional },
    { 0 }
};

static void test_MsgWaitForMultipleObjects(HWND hwnd)
{
    DWORD ret;
    MSG msg;

    ret = MsgWaitForMultipleObjects(0, NULL, FALSE, 0, QS_POSTMESSAGE);
    ok(ret == WAIT_TIMEOUT, "MsgWaitForMultipleObjects returned %x\n", ret);

    PostMessageA(hwnd, WM_USER, 0, 0);

    ret = MsgWaitForMultipleObjects(0, NULL, FALSE, 0, QS_POSTMESSAGE);
    ok(ret == WAIT_OBJECT_0, "MsgWaitForMultipleObjects returned %x\n", ret);

    ok(PeekMessageA( &msg, 0, 0, 0, PM_REMOVE ), "PeekMessage should succeed\n");
    ok(msg.message == WM_USER, "got %04x instead of WM_USER\n", msg.message);

    ret = MsgWaitForMultipleObjects(0, NULL, FALSE, 0, QS_POSTMESSAGE);
    ok(ret == WAIT_TIMEOUT, "MsgWaitForMultipleObjects returned %x\n", ret);

    PostMessageA(hwnd, WM_USER, 0, 0);

    ret = MsgWaitForMultipleObjects(0, NULL, FALSE, 0, QS_POSTMESSAGE);
    ok(ret == WAIT_OBJECT_0, "MsgWaitForMultipleObjects returned %x\n", ret);

    ok(PeekMessageA( &msg, 0, 0, 0, PM_NOREMOVE ), "PeekMessage should succeed\n");
    ok(msg.message == WM_USER, "got %04x instead of WM_USER\n", msg.message);

    /* shows QS_POSTMESSAGE flag is cleared in the PeekMessage call */
    ret = MsgWaitForMultipleObjects(0, NULL, FALSE, 0, QS_POSTMESSAGE);
    ok(ret == WAIT_TIMEOUT, "MsgWaitForMultipleObjects returned %x\n", ret);

    PostMessageA(hwnd, WM_USER, 0, 0);

    /* new incoming message causes it to become signaled again */
    ret = MsgWaitForMultipleObjects(0, NULL, FALSE, 0, QS_POSTMESSAGE);
    ok(ret == WAIT_OBJECT_0, "MsgWaitForMultipleObjects returned %x\n", ret);

    ok(PeekMessageA( &msg, 0, 0, 0, PM_REMOVE ), "PeekMessage should succeed\n");
    ok(msg.message == WM_USER, "got %04x instead of WM_USER\n", msg.message);
    ok(PeekMessageA( &msg, 0, 0, 0, PM_REMOVE ), "PeekMessage should succeed\n");
    ok(msg.message == WM_USER, "got %04x instead of WM_USER\n", msg.message);
}

/* test if we receive the right sequence of messages */
static void test_messages(void)
{
    HWND hwnd, hparent, hchild;
    HWND hchild2, hbutton;
    HMENU hmenu;
    MSG msg;
    LRESULT res;
    POINT pos;

    flush_sequence();

    hwnd = CreateWindowExA(0, "TestWindowClass", "Test overlapped", WS_OVERLAPPEDWINDOW,
                           100, 100, 200, 200, 0, 0, 0, NULL);
    ok (hwnd != 0, "Failed to create overlapped window\n");
    ok_sequence(WmCreateOverlappedSeq, "CreateWindow:overlapped", FALSE);

    /* test ShowWindow(SW_HIDE) on a newly created invisible window */
    ok( ShowWindow(hwnd, SW_HIDE) == FALSE, "ShowWindow: window was visible\n" );
    ok_sequence(WmEmptySeq, "ShowWindow(SW_HIDE):overlapped, invisible", FALSE);

    /* test WM_SETREDRAW on a not visible top level window */
    test_WM_SETREDRAW(hwnd);

    SetWindowPos(hwnd, 0,0,0,0,0, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE);
    flush_events();
    ok_sequence(WmSWP_ShowOverlappedSeq, "SetWindowPos:SWP_SHOWWINDOW:overlapped", FALSE);
    ok(IsWindowVisible(hwnd), "window should be visible at this point\n");

    ok(GetActiveWindow() == hwnd, "window should be active\n");
    ok(GetFocus() == hwnd, "window should have input focus\n");
    ShowWindow(hwnd, SW_HIDE);
    flush_events();
    ok_sequence(WmHideOverlappedSeq, "ShowWindow(SW_HIDE):overlapped", FALSE);

    ShowWindow(hwnd, SW_SHOW);
    flush_events();
    ok_sequence(WmShowOverlappedSeq, "ShowWindow(SW_SHOW):overlapped", TRUE);

    ShowWindow(hwnd, SW_HIDE);
    flush_events();
    ok_sequence(WmHideOverlappedSeq, "ShowWindow(SW_HIDE):overlapped", FALSE);

    ShowWindow(hwnd, SW_SHOWMAXIMIZED);
    flush_events();
    ok_sequence(WmShowMaxOverlappedSeq, "ShowWindow(SW_SHOWMAXIMIZED):overlapped", TRUE);
    flush_sequence();

    if (GetWindowLongW( hwnd, GWL_STYLE ) & WS_MAXIMIZE)
    {
        ShowWindow(hwnd, SW_RESTORE);
        flush_events();
        ok_sequence(WmShowRestoreMaxOverlappedSeq, "ShowWindow(SW_RESTORE):overlapped", TRUE);
        flush_sequence();
    }

    ShowWindow(hwnd, SW_MINIMIZE);
    flush_events();
    ok_sequence(WmShowMinOverlappedSeq, "ShowWindow(SW_SHOWMINIMIZED):overlapped", TRUE);
    flush_sequence();

    if (GetWindowLongW( hwnd, GWL_STYLE ) & WS_MINIMIZE)
    {
        ShowWindow(hwnd, SW_RESTORE);
        flush_events();
        ok_sequence(WmShowRestoreMinOverlappedSeq, "ShowWindow(SW_RESTORE):overlapped", TRUE);
        flush_sequence();
    }

    ShowWindow(hwnd, SW_SHOW);
    flush_events();
    ok_sequence(WmOptionalPaint, "ShowWindow(SW_SHOW):overlapped already visible", FALSE);

    SetWindowPos(hwnd, 0,0,0,0,0, SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOMOVE);
    ok_sequence(WmSWP_HideOverlappedSeq, "SetWindowPos:SWP_HIDEWINDOW:overlapped", FALSE);
    ok(!IsWindowVisible(hwnd), "window should not be visible at this point\n");
    ok(GetActiveWindow() == hwnd, "window should still be active\n");

    /* test WM_SETREDRAW on a visible top level window */
    ShowWindow(hwnd, SW_SHOW);
    flush_events();
    test_WM_SETREDRAW(hwnd);

    trace("testing scroll APIs on a visible top level window %p\n", hwnd);
    test_scroll_messages(hwnd);

    /* test resizing and moving */
    SetWindowPos( hwnd, 0, 0, 0, 300, 300, SWP_NOMOVE|SWP_NOACTIVATE );
    ok_sequence(WmSWP_ResizeSeq, "SetWindowPos:Resize", FALSE );
    flush_events();
    flush_sequence();
    SetWindowPos( hwnd, 0, 200, 200, 0, 0, SWP_NOSIZE|SWP_NOACTIVATE );
    ok_sequence(WmSWP_MoveSeq, "SetWindowPos:Move", FALSE );
    flush_events();
    flush_sequence();
    SetWindowPos( hwnd, 0, 200, 200, 250, 250, SWP_NOZORDER|SWP_NOACTIVATE );
    ok_sequence(WmSWP_ResizeNoZOrder, "SetWindowPos:WmSWP_ResizeNoZOrder", FALSE );
    flush_events();
    flush_sequence();

    /* popups don't get WM_GETMINMAXINFO */
    SetWindowLongW( hwnd, GWL_STYLE, WS_VISIBLE|WS_POPUP );
    SetWindowPos( hwnd, 0, 0, 0, 0, 0, SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_FRAMECHANGED);
    flush_sequence();
    SetWindowPos( hwnd, 0, 0, 0, 200, 200, SWP_NOMOVE|SWP_NOACTIVATE );
    ok_sequence(WmSWP_ResizePopupSeq, "SetWindowPos:ResizePopup", FALSE );

    DestroyWindow(hwnd);
    ok_sequence(WmDestroyOverlappedSeq, "DestroyWindow:overlapped", FALSE);

    hparent = CreateWindowExA(0, "TestParentClass", "Test parent", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              100, 100, 200, 200, 0, 0, 0, NULL);
    ok (hparent != 0, "Failed to create parent window\n");
    flush_sequence();

    hchild = CreateWindowExA(0, "TestWindowClass", "Test child", WS_CHILD | WS_MAXIMIZE,
                             0, 0, 10, 10, hparent, 0, 0, NULL);
    ok (hchild != 0, "Failed to create child window\n");
    ok_sequence(WmCreateMaximizedChildSeq, "CreateWindow:maximized child", FALSE);
    DestroyWindow(hchild);
    flush_sequence();

    /* visible child window with a caption */
    hchild = CreateWindowExA(0, "TestWindowClass", "Test child",
                             WS_CHILD | WS_VISIBLE | WS_CAPTION,
                             0, 0, 10, 10, hparent, 0, 0, NULL);
    ok (hchild != 0, "Failed to create child window\n");
    ok_sequence(WmCreateVisibleChildSeq, "CreateWindow:visible child", FALSE);

    trace("testing scroll APIs on a visible child window %p\n", hchild);
    test_scroll_messages(hchild);

    SetWindowPos(hchild, 0,0,0,0,0, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE);
    ok_sequence(WmShowChildSeq_4, "SetWindowPos(SWP_SHOWWINDOW):child with a caption", FALSE);

    DestroyWindow(hchild);
    flush_sequence();

    hchild = CreateWindowExA(0, "TestWindowClass", "Test child", WS_CHILD,
                             0, 0, 10, 10, hparent, 0, 0, NULL);
    ok (hchild != 0, "Failed to create child window\n");
    ok_sequence(WmCreateChildSeq, "CreateWindow:child", FALSE);
    
    hchild2 = CreateWindowExA(0, "SimpleWindowClass", "Test child2", WS_CHILD,
                               100, 100, 50, 50, hparent, 0, 0, NULL);
    ok (hchild2 != 0, "Failed to create child2 window\n");
    flush_sequence();

    hbutton = CreateWindowExA(0, "TestWindowClass", "Test button", WS_CHILD,
                              0, 100, 50, 50, hchild, 0, 0, NULL);
    ok (hbutton != 0, "Failed to create button window\n");

    /* test WM_SETREDRAW on a not visible child window */
    test_WM_SETREDRAW(hchild);

    ShowWindow(hchild, SW_SHOW);
    ok_sequence(WmShowChildSeq, "ShowWindow(SW_SHOW):child", FALSE);

    /* check parent messages too */
    log_all_parent_messages++;
    ShowWindow(hchild, SW_HIDE);
    ok_sequence(WmHideChildSeq2, "ShowWindow(SW_HIDE):child", FALSE);
    log_all_parent_messages--;

    ShowWindow(hchild, SW_SHOW);
    ok_sequence(WmShowChildSeq, "ShowWindow(SW_SHOW):child", FALSE);

    ShowWindow(hchild, SW_HIDE);
    ok_sequence(WmHideChildSeq, "ShowWindow(SW_HIDE):child", FALSE);

    ShowWindow(hchild, SW_SHOW);
    ok_sequence(WmShowChildSeq, "ShowWindow(SW_SHOW):child", FALSE);

    /* test WM_SETREDRAW on a visible child window */
    test_WM_SETREDRAW(hchild);

    log_all_parent_messages++;
    MoveWindow(hchild, 10, 10, 20, 20, TRUE);
    ok_sequence(WmResizingChildWithMoveWindowSeq, "MoveWindow:child", FALSE);
    log_all_parent_messages--;

    ShowWindow(hchild, SW_HIDE);
    flush_sequence();
    SetWindowPos(hchild, 0,0,0,0,0, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE);
    ok_sequence(WmShowChildSeq_2, "SetWindowPos:show_child_2", FALSE);

    ShowWindow(hchild, SW_HIDE);
    flush_sequence();
    SetWindowPos(hchild, 0,0,0,0,0, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE);
    ok_sequence(WmShowChildSeq_3, "SetWindowPos:show_child_3", FALSE);

    /* DestroyWindow sequence below expects that a child has focus */
    SetFocus(hchild);
    flush_sequence();

    DestroyWindow(hchild);
    ok_sequence(WmDestroyChildSeq, "DestroyWindow:child", FALSE);
    DestroyWindow(hchild2);
    DestroyWindow(hbutton);

    flush_sequence();
    hchild = CreateWindowExA(0, "TestWindowClass", "Test Child Popup", WS_CHILD | WS_POPUP,
                             0, 0, 100, 100, hparent, 0, 0, NULL);
    ok (hchild != 0, "Failed to create child popup window\n");
    ok_sequence(WmCreateChildPopupSeq, "CreateWindow:child_popup", FALSE);
    DestroyWindow(hchild);

    /* test what happens to a window which sets WS_VISIBLE in WM_CREATE */
    flush_sequence();
    hchild = CreateWindowExA(0, "TestPopupClass", "Test Popup", WS_POPUP,
                             0, 0, 100, 100, hparent, 0, 0, NULL);
    ok (hchild != 0, "Failed to create popup window\n");
    ok_sequence(WmCreateInvisiblePopupSeq, "CreateWindow:invisible_popup", FALSE);
    ok(GetWindowLongA(hchild, GWL_STYLE) & WS_VISIBLE, "WS_VISIBLE should be set\n");
    ok(IsWindowVisible(hchild), "IsWindowVisible() should return TRUE\n");
    flush_sequence();
    ShowWindow(hchild, SW_SHOW);
    ok_sequence(WmEmptySeq, "ShowWindow:show_visible_popup", FALSE);
    flush_sequence();
    SetWindowPos(hchild, 0,0,0,0,0, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);
    ok_sequence(WmShowVisiblePopupSeq_2, "SetWindowPos:show_visible_popup_2", FALSE);
    flush_sequence();
    SetWindowPos(hchild, 0,0,0,0,0, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE);
    ok_sequence(WmShowVisiblePopupSeq_3, "SetWindowPos:show_visible_popup_3", FALSE);
    DestroyWindow(hchild);

    /* this time add WS_VISIBLE for CreateWindowEx, but this fact actually
     * changes nothing in message sequences.
     */
    flush_sequence();
    hchild = CreateWindowExA(0, "TestPopupClass", "Test Popup", WS_POPUP | WS_VISIBLE,
                             0, 0, 100, 100, hparent, 0, 0, NULL);
    ok (hchild != 0, "Failed to create popup window\n");
    ok_sequence(WmCreateInvisiblePopupSeq, "CreateWindow:invisible_popup", FALSE);
    ok(GetWindowLongA(hchild, GWL_STYLE) & WS_VISIBLE, "WS_VISIBLE should be set\n");
    ok(IsWindowVisible(hchild), "IsWindowVisible() should return TRUE\n");
    flush_sequence();
    ShowWindow(hchild, SW_SHOW);
    ok_sequence(WmEmptySeq, "ShowWindow:show_visible_popup", FALSE);
    flush_sequence();
    SetWindowPos(hchild, 0,0,0,0,0, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);
    ok_sequence(WmShowVisiblePopupSeq_2, "SetWindowPos:show_visible_popup_2", FALSE);
    DestroyWindow(hchild);

    flush_sequence();
    hwnd = CreateWindowExA(WS_EX_DLGMODALFRAME, "TestDialogClass", NULL, WS_VISIBLE|WS_CAPTION|WS_SYSMENU|WS_DLGFRAME,
                           0, 0, 100, 100, hparent, 0, 0, NULL);
    ok(hwnd != 0, "Failed to create custom dialog window\n");
    ok_sequence(WmCreateCustomDialogSeq, "CreateCustomDialog", TRUE);

    if(0) {
    trace("testing scroll APIs on a visible dialog %p\n", hwnd);
    test_scroll_messages(hwnd);
    }

    flush_sequence();

    test_def_id = TRUE;
    SendMessageA(hwnd, WM_NULL, 0, 0);

    flush_sequence();
    after_end_dialog = TRUE;
    EndDialog( hwnd, 0 );
    ok_sequence(WmEndCustomDialogSeq, "EndCustomDialog", FALSE);

    DestroyWindow(hwnd);
    after_end_dialog = FALSE;
    test_def_id = FALSE;

    ok(GetCursorPos(&pos), "GetCursorPos failed\n");
    ok(SetCursorPos(109, 109), "SetCursorPos failed\n");

    hwnd = CreateWindowExA(0, "TestDialogClass", NULL, WS_POPUP|WS_CHILD,
                           0, 0, 100, 100, 0, 0, GetModuleHandleA(0), NULL);
    ok(hwnd != 0, "Failed to create custom dialog window\n");
    flush_sequence();
    trace("call ShowWindow(%p, SW_SHOW)\n", hwnd);
    ShowWindow(hwnd, SW_SHOW);
    ok_sequence(WmShowCustomDialogSeq, "ShowCustomDialog", TRUE);

    flush_events();
    flush_sequence();
    ok(DrawMenuBar(hwnd), "DrawMenuBar failed: %d\n", GetLastError());
    flush_events();
    ok_sequence(WmDrawMenuBarSeq, "DrawMenuBar", FALSE);
    ok(SetCursorPos(pos.x, pos.y), "SetCursorPos failed\n");
    DestroyWindow(hwnd);

    flush_sequence();
    DialogBoxA( 0, "TEST_DIALOG", hparent, TestModalDlgProcA );
    ok_sequence(WmModalDialogSeq, "ModalDialog", TRUE);

    DestroyWindow(hparent);
    flush_sequence();

    /* Message sequence for SetMenu */
    ok(!DrawMenuBar(hwnd), "DrawMenuBar should return FALSE for a destroyed window\n");
    ok(GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "last error is %d\n", GetLastError());
    ok_sequence(WmEmptySeq, "DrawMenuBar for a window without a menu", FALSE);

    hmenu = CreateMenu();
    ok (hmenu != 0, "Failed to create menu\n");
    ok (InsertMenuA(hmenu, -1, MF_BYPOSITION, 0x1000, "foo"), "InsertMenu failed\n");
    hwnd = CreateWindowExA(0, "TestWindowClass", "Test overlapped", WS_OVERLAPPEDWINDOW,
                           100, 100, 200, 200, 0, hmenu, 0, NULL);
    ok_sequence(WmCreateOverlappedSeq, "CreateWindow:overlapped", FALSE);
    ok (SetMenu(hwnd, 0), "SetMenu\n");
    ok_sequence(WmSetMenuNonVisibleSizeChangeSeq, "SetMenu:NonVisibleSizeChange", FALSE);
    ok (SetMenu(hwnd, 0), "SetMenu\n");
    ok_sequence(WmSetMenuNonVisibleNoSizeChangeSeq, "SetMenu:NonVisibleNoSizeChange", FALSE);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow( hwnd );
    flush_events();
    flush_sequence();
    ok (SetMenu(hwnd, 0), "SetMenu\n");
    ok_sequence(WmSetMenuVisibleNoSizeChangeSeq, "SetMenu:VisibleNoSizeChange", FALSE);
    ok (SetMenu(hwnd, hmenu), "SetMenu\n");
    ok_sequence(WmSetMenuVisibleSizeChangeSeq, "SetMenu:VisibleSizeChange", FALSE);

    UpdateWindow( hwnd );
    flush_events();
    flush_sequence();
    ok(DrawMenuBar(hwnd), "DrawMenuBar\n");
    flush_events();
    ok_sequence(WmDrawMenuBarSeq, "DrawMenuBar", FALSE);

    DestroyWindow(hwnd);
    flush_sequence();

    /* Message sequence for EnableWindow */
    hparent = CreateWindowExA(0, "TestWindowClass", "Test parent", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              100, 100, 200, 200, 0, 0, 0, NULL);
    ok (hparent != 0, "Failed to create parent window\n");
    hchild = CreateWindowExA(0, "TestWindowClass", "Test child", WS_CHILD | WS_VISIBLE,
                             0, 0, 10, 10, hparent, 0, 0, NULL);
    ok (hchild != 0, "Failed to create child window\n");

    SetFocus(hchild);
    flush_events();
    flush_sequence();

    EnableWindow(hparent, FALSE);
    ok_sequence(WmEnableWindowSeq_1, "EnableWindow(FALSE)", FALSE);

    EnableWindow(hparent, TRUE);
    ok_sequence(WmEnableWindowSeq_2, "EnableWindow(TRUE)", FALSE);

    flush_events();
    flush_sequence();

    test_MsgWaitForMultipleObjects(hparent);

    /* the following test causes an exception in user.exe under win9x */
    if (!PostMessageW( hparent, WM_USER, 0, 0 ))
    {
        DestroyWindow(hparent);
        flush_sequence();
        return;
    }
    PostMessageW( hparent, WM_USER+1, 0, 0 );
    /* PeekMessage(NULL) fails, but still removes the message */
    SetLastError(0xdeadbeef);
    ok( !PeekMessageW( NULL, 0, 0, 0, PM_REMOVE ), "PeekMessage(NULL) should fail\n" );
    ok( GetLastError() == ERROR_NOACCESS || /* Win2k */
        GetLastError() == 0xdeadbeef, /* NT4 */
        "last error is %d\n", GetLastError() );
    ok( PeekMessageW( &msg, 0, 0, 0, PM_REMOVE ), "PeekMessage should succeed\n" );
    ok( msg.message == WM_USER+1, "got %x instead of WM_USER+1\n", msg.message );

    DestroyWindow(hchild);
    DestroyWindow(hparent);
    flush_sequence();

    /* Message sequences for WM_SETICON */
    trace("testing WM_SETICON\n");
    hwnd = CreateWindowExA(0, "TestWindowClass", NULL, WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, 300, 300, 0,
                           NULL, NULL, 0);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    flush_events();
    flush_sequence();
    SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadIconA(0, (LPCSTR)IDI_APPLICATION));
    ok_sequence(WmSetIcon_1, "WM_SETICON for shown window with caption", FALSE);

    ShowWindow(hwnd, SW_HIDE);
    flush_events();
    flush_sequence();
    SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadIconA(0, (LPCSTR)IDI_APPLICATION));
    ok_sequence(WmSetIcon_2, "WM_SETICON for hidden window with caption", FALSE);
    DestroyWindow(hwnd);
    flush_sequence();

    hwnd = CreateWindowExA(0, "TestPopupClass", NULL, WS_POPUP,
                           CW_USEDEFAULT, CW_USEDEFAULT, 300, 300, 0,
                           NULL, NULL, 0);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    flush_events();
    flush_sequence();
    SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadIconA(0, (LPCSTR)IDI_APPLICATION));
    ok_sequence(WmSetIcon_2, "WM_SETICON for shown window without caption", FALSE);

    ShowWindow(hwnd, SW_HIDE);
    flush_events();
    flush_sequence();
    SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadIconA(0, (LPCSTR)IDI_APPLICATION));
    ok_sequence(WmSetIcon_2, "WM_SETICON for hidden window without caption", FALSE);

    flush_sequence();
    res = SendMessageA(hwnd, 0x3B, 0x8000000b, 0);
    if (!res)
    {
        todo_wine win_skip( "Message 0x3b not supported\n" );
        goto done;
    }
    ok_sequence(WmInitEndSession, "Handling of undocumented 0x3B message by DefWindowProc wparam=0x8000000b", TRUE);
    ok(res == 1, "SendMessage(hwnd, 0x3B, 0x8000000b, 0) should have returned 1 instead of %ld\n", res);
    res = SendMessageA(hwnd, 0x3B, 0x0000000b, 0);
    ok_sequence(WmInitEndSession_2, "Handling of undocumented 0x3B message by DefWindowProc wparam=0x0000000b", TRUE);
    ok(res == 1, "SendMessage(hwnd, 0x3B, 0x0000000b, 0) should have returned 1 instead of %ld\n", res);
    res = SendMessageA(hwnd, 0x3B, 0x0000000f, 0);
    ok_sequence(WmInitEndSession_2, "Handling of undocumented 0x3B message by DefWindowProc wparam=0x0000000f", TRUE);
    ok(res == 1, "SendMessage(hwnd, 0x3B, 0x0000000f, 0) should have returned 1 instead of %ld\n", res);

    flush_sequence();
    res = SendMessageA(hwnd, 0x3B, 0x80000008, 0);
    ok_sequence(WmInitEndSession_3, "Handling of undocumented 0x3B message by DefWindowProc wparam=0x80000008", TRUE);
    ok(res == 2, "SendMessage(hwnd, 0x3B, 0x80000008, 0) should have returned 2 instead of %ld\n", res);
    res = SendMessageA(hwnd, 0x3B, 0x00000008, 0);
    ok_sequence(WmInitEndSession_4, "Handling of undocumented 0x3B message by DefWindowProc wparam=0x00000008", TRUE);
    ok(res == 2, "SendMessage(hwnd, 0x3B, 0x00000008, 0) should have returned 2 instead of %ld\n", res);

    res = SendMessageA(hwnd, 0x3B, 0x80000004, 0);
    ok_sequence(WmInitEndSession_3, "Handling of undocumented 0x3B message by DefWindowProc wparam=0x80000004", TRUE);
    ok(res == 2, "SendMessage(hwnd, 0x3B, 0x80000004, 0) should have returned 2 instead of %ld\n", res);

    res = SendMessageA(hwnd, 0x3B, 0x80000001, 0);
    ok_sequence(WmInitEndSession_5, "Handling of undocumented 0x3B message by DefWindowProc wparam=0x80000001", TRUE);
    ok(res == 2, "SendMessage(hwnd, 0x3B, 0x80000001, 0) should have returned 2 instead of %ld\n", res);

done:
    DestroyWindow(hwnd);
    flush_sequence();
}

static void test_setwindowpos(void)
{
    HWND hwnd;
    RECT rc;
    LRESULT res;
    const INT winX = 100;
    const INT winY = 100;
    const INT sysX = GetSystemMetrics(SM_CXMINTRACK);

    hwnd = CreateWindowExA(0, "TestWindowClass", NULL, 0,
                           0, 0, winX, winY, 0,
                           NULL, NULL, 0);

    GetWindowRect(hwnd, &rc);
    expect(sysX, rc.right);
    expect(winY, rc.bottom);

    flush_events();
    flush_sequence();
    res = SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, winX, winY, 0);
    ok_sequence(WmZOrder, "Z-Order", TRUE);
    ok(res == TRUE, "SetWindowPos expected TRUE, got %ld\n", res);

    GetWindowRect(hwnd, &rc);
    expect(sysX, rc.right);
    expect(winY, rc.bottom);
    DestroyWindow(hwnd);
}

static void invisible_parent_tests(void)
{
    HWND hparent, hchild;

    hparent = CreateWindowExA(0, "TestParentClass", "Test parent", WS_OVERLAPPEDWINDOW,
                              100, 100, 200, 200, 0, 0, 0, NULL);
    ok (hparent != 0, "Failed to create parent window\n");
    flush_sequence();

    /* test showing child with hidden parent */

    hchild = CreateWindowExA(0, "TestWindowClass", "Test child", WS_CHILD,
                             0, 0, 10, 10, hparent, 0, 0, NULL);
    ok (hchild != 0, "Failed to create child window\n");
    ok_sequence(WmCreateChildSeq, "CreateWindow:child", FALSE);

    ShowWindow( hchild, SW_MINIMIZE );
    ok_sequence(WmShowChildInvisibleParentSeq_1, "ShowWindow(SW_MINIMIZE) child with invisible parent", FALSE);
    ok(GetWindowLongA(hchild, GWL_STYLE) & WS_VISIBLE, "WS_VISIBLE should be set\n");
    ok(!IsWindowVisible(hchild), "IsWindowVisible() should return FALSE\n");

    /* repeat */
    flush_events();
    flush_sequence();
    ShowWindow( hchild, SW_MINIMIZE );
    ok_sequence(WmShowChildInvisibleParentSeq_1r, "ShowWindow(SW_MINIMIZE) child with invisible parent", FALSE);

    DestroyWindow(hchild);
    hchild = CreateWindowExA(0, "TestWindowClass", "Test child", WS_CHILD,
                             0, 0, 10, 10, hparent, 0, 0, NULL);
    flush_sequence();

    ShowWindow( hchild, SW_MAXIMIZE );
    ok_sequence(WmShowChildInvisibleParentSeq_2, "ShowWindow(SW_MAXIMIZE) child with invisible parent", FALSE);
    ok(GetWindowLongA(hchild, GWL_STYLE) & WS_VISIBLE, "WS_VISIBLE should be set\n");
    ok(!IsWindowVisible(hchild), "IsWindowVisible() should return FALSE\n");

    /* repeat */
    flush_events();
    flush_sequence();
    ShowWindow( hchild, SW_MAXIMIZE );
    ok_sequence(WmShowChildInvisibleParentSeq_2r, "ShowWindow(SW_MAXIMIZE) child with invisible parent", FALSE);

    DestroyWindow(hchild);
    hchild = CreateWindowExA(0, "TestWindowClass", "Test child", WS_CHILD,
                             0, 0, 10, 10, hparent, 0, 0, NULL);
    flush_sequence();

    ShowWindow( hchild, SW_RESTORE );
    ok_sequence(WmShowChildInvisibleParentSeq_5, "ShowWindow(SW_RESTORE) child with invisible parent", FALSE);
    ok(GetWindowLongA(hchild, GWL_STYLE) & WS_VISIBLE, "WS_VISIBLE should be set\n");
    ok(!IsWindowVisible(hchild), "IsWindowVisible() should return FALSE\n");

    DestroyWindow(hchild);
    hchild = CreateWindowExA(0, "TestWindowClass", "Test child", WS_CHILD,
                             0, 0, 10, 10, hparent, 0, 0, NULL);
    flush_sequence();

    ShowWindow( hchild, SW_SHOWMINIMIZED );
    ok_sequence(WmShowChildInvisibleParentSeq_3, "ShowWindow(SW_SHOWMINIMIZED) child with invisible parent", FALSE);
    ok(GetWindowLongA(hchild, GWL_STYLE) & WS_VISIBLE, "WS_VISIBLE should be set\n");
    ok(!IsWindowVisible(hchild), "IsWindowVisible() should return FALSE\n");

    /* repeat */
    flush_events();
    flush_sequence();
    ShowWindow( hchild, SW_SHOWMINIMIZED );
    ok_sequence(WmShowChildInvisibleParentSeq_3r, "ShowWindow(SW_SHOWMINIMIZED) child with invisible parent", FALSE);

    DestroyWindow(hchild);
    hchild = CreateWindowExA(0, "TestWindowClass", "Test child", WS_CHILD,
                             0, 0, 10, 10, hparent, 0, 0, NULL);
    flush_sequence();

    /* same as ShowWindow( hchild, SW_MAXIMIZE ); */
    ShowWindow( hchild, SW_SHOWMAXIMIZED );
    ok_sequence(WmShowChildInvisibleParentSeq_2, "ShowWindow(SW_SHOWMAXIMIZED) child with invisible parent", FALSE);
    ok(GetWindowLongA(hchild, GWL_STYLE) & WS_VISIBLE, "WS_VISIBLE should be set\n");
    ok(!IsWindowVisible(hchild), "IsWindowVisible() should return FALSE\n");

    DestroyWindow(hchild);
    hchild = CreateWindowExA(0, "TestWindowClass", "Test child", WS_CHILD,
                             0, 0, 10, 10, hparent, 0, 0, NULL);
    flush_sequence();

    ShowWindow( hchild, SW_SHOWMINNOACTIVE );
    ok_sequence(WmShowChildInvisibleParentSeq_4, "ShowWindow(SW_SHOWMINNOACTIVE) child with invisible parent", FALSE);
    ok(GetWindowLongA(hchild, GWL_STYLE) & WS_VISIBLE, "WS_VISIBLE should be set\n");
    ok(!IsWindowVisible(hchild), "IsWindowVisible() should return FALSE\n");

    /* repeat */
    flush_events();
    flush_sequence();
    ShowWindow( hchild, SW_SHOWMINNOACTIVE );
    ok_sequence(WmShowChildInvisibleParentSeq_4r, "ShowWindow(SW_SHOWMINNOACTIVE) child with invisible parent", FALSE);

    DestroyWindow(hchild);
    hchild = CreateWindowExA(0, "TestWindowClass", "Test child", WS_CHILD,
                             0, 0, 10, 10, hparent, 0, 0, NULL);
    flush_sequence();

    /* FIXME: looks like XP SP2 doesn't know about SW_FORCEMINIMIZE at all */
    ShowWindow( hchild, SW_FORCEMINIMIZE );
    ok_sequence(WmEmptySeq, "ShowWindow(SW_FORCEMINIMIZE) child with invisible parent", TRUE);
todo_wine {
    ok(!(GetWindowLongA(hchild, GWL_STYLE) & WS_VISIBLE), "WS_VISIBLE should be not set\n");
}
    ok(!IsWindowVisible(hchild), "IsWindowVisible() should return FALSE\n");

    DestroyWindow(hchild);
    hchild = CreateWindowExA(0, "TestWindowClass", "Test child", WS_CHILD,
                             0, 0, 10, 10, hparent, 0, 0, NULL);
    flush_sequence();

    ShowWindow( hchild, SW_SHOWNA );
    ok_sequence(WmShowChildInvisibleParentSeq_5, "ShowWindow(SW_SHOWNA) child with invisible parent", FALSE);
    ok(GetWindowLongA(hchild, GWL_STYLE) & WS_VISIBLE, "WS_VISIBLE should be set\n");
    ok(!IsWindowVisible(hchild), "IsWindowVisible() should return FALSE\n");

    /* repeat */
    flush_events();
    flush_sequence();
    ShowWindow( hchild, SW_SHOWNA );
    ok_sequence(WmShowChildInvisibleParentSeq_5, "ShowWindow(SW_SHOWNA) child with invisible parent", FALSE);

    DestroyWindow(hchild);
    hchild = CreateWindowExA(0, "TestWindowClass", "Test child", WS_CHILD,
                             0, 0, 10, 10, hparent, 0, 0, NULL);
    flush_sequence();

    ShowWindow( hchild, SW_SHOW );
    ok_sequence(WmShowChildInvisibleParentSeq_5, "ShowWindow(SW_SHOW) child with invisible parent", FALSE);
    ok(GetWindowLongA(hchild, GWL_STYLE) & WS_VISIBLE, "WS_VISIBLE should be set\n");
    ok(!IsWindowVisible(hchild), "IsWindowVisible() should return FALSE\n");

    /* repeat */
    flush_events();
    flush_sequence();
    ShowWindow( hchild, SW_SHOW );
    ok_sequence(WmEmptySeq, "ShowWindow(SW_SHOW) child with invisible parent", FALSE);

    ShowWindow( hchild, SW_HIDE );
    ok_sequence(WmHideChildInvisibleParentSeq, "ShowWindow:hide child with invisible parent", FALSE);
    ok(!(GetWindowLongA(hchild, GWL_STYLE) & WS_VISIBLE), "WS_VISIBLE should be not set\n");
    ok(!IsWindowVisible(hchild), "IsWindowVisible() should return FALSE\n");

    SetWindowPos(hchild, 0,0,0,0,0, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);
    ok_sequence(WmShowChildInvisibleParentSeq_6, "SetWindowPos:show child with invisible parent", FALSE);
    ok(GetWindowLongA(hchild, GWL_STYLE) & WS_VISIBLE, "WS_VISIBLE should be set\n");
    ok(!IsWindowVisible(hchild), "IsWindowVisible() should return FALSE\n");

    SetWindowPos(hchild, 0,0,0,0,0, SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);
    ok_sequence(WmHideChildInvisibleParentSeq_2, "SetWindowPos:hide child with invisible parent", FALSE);
    ok(!(GetWindowLongA(hchild, GWL_STYLE) & WS_VISIBLE), "WS_VISIBLE should not be set\n");
    ok(!IsWindowVisible(hchild), "IsWindowVisible() should return FALSE\n");

    SetWindowPos(hchild, 0,0,0,0,0, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);
    flush_sequence();
    DestroyWindow(hchild);
    ok_sequence(WmDestroyInvisibleChildSeq, "DestroyInvisibleChildSeq", FALSE);

    DestroyWindow(hparent);
    flush_sequence();
}

/****************** button message test *************************/
#define ID_BUTTON 0x000e

static const struct message WmSetFocusButtonSeq[] =
{
    { HCBT_SETFOCUS, hook },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|wparam, 0 },
    { WM_CTLCOLORBTN, sent|parent },
    { WM_COMMAND, sent|wparam|parent, MAKEWPARAM(ID_BUTTON, BN_SETFOCUS) },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { 0 }
};
static const struct message WmKillFocusButtonSeq[] =
{
    { HCBT_SETFOCUS, hook },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_KILLFOCUS, sent|wparam, 0 },
    { WM_CTLCOLORBTN, sent|parent },
    { WM_COMMAND, sent|wparam|parent, MAKEWPARAM(ID_BUTTON, BN_KILLFOCUS) },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 1 },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { WM_CTLCOLORBTN, sent|parent },
    { 0 }
};
static const struct message WmSetFocusStaticSeq[] =
{
    { HCBT_SETFOCUS, hook },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|wparam, 0 },
    { WM_CTLCOLORSTATIC, sent|parent },
    { WM_COMMAND, sent|wparam|parent, MAKEWPARAM(ID_BUTTON, BN_SETFOCUS) },
    { WM_COMMAND, sent|wparam|parent|optional, MAKEWPARAM(ID_BUTTON, BN_CLICKED) }, /* radio button */
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { 0 }
};
static const struct message WmKillFocusStaticSeq[] =
{
    { HCBT_SETFOCUS, hook },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_KILLFOCUS, sent|wparam, 0 },
    { WM_CTLCOLORSTATIC, sent|parent },
    { WM_COMMAND, sent|wparam|parent, MAKEWPARAM(ID_BUTTON, BN_KILLFOCUS) },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 1 },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { WM_CTLCOLORSTATIC, sent|parent },
    { 0 }
};
static const struct message WmSetFocusOwnerdrawSeq[] =
{
    { HCBT_SETFOCUS, hook },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|wparam, 0 },
    { WM_CTLCOLORBTN, sent|parent },
    { WM_DRAWITEM, sent|wparam|lparam|parent, ID_BUTTON, 0x001040e4 },
    { WM_COMMAND, sent|wparam|parent, MAKEWPARAM(ID_BUTTON, BN_SETFOCUS) },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { 0 }
};
static const struct message WmKillFocusOwnerdrawSeq[] =
{
    { HCBT_SETFOCUS, hook },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_KILLFOCUS, sent|wparam, 0 },
    { WM_CTLCOLORBTN, sent|parent },
    { WM_DRAWITEM, sent|wparam|lparam|parent, ID_BUTTON, 0x000040e4 },
    { WM_COMMAND, sent|wparam|parent, MAKEWPARAM(ID_BUTTON, BN_KILLFOCUS) },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 1 },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { WM_CTLCOLORBTN, sent|parent },
    { WM_DRAWITEM, sent|wparam|lparam|parent, ID_BUTTON, 0x000010e4 },
    { 0 }
};
static const struct message WmLButtonDownSeq[] =
{
    { WM_LBUTTONDOWN, sent|wparam|lparam, 0, 0 },
    { EVENT_SYSTEM_CAPTURESTART, winevent_hook|wparam|lparam, 0, 0 },
    { HCBT_SETFOCUS, hook },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|wparam|defwinproc, 0 },
    { WM_CTLCOLORBTN, sent|defwinproc },
    { BM_SETSTATE, sent|wparam|defwinproc, TRUE },
    { WM_CTLCOLORBTN, sent|defwinproc },
    { EVENT_OBJECT_STATECHANGE, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { 0 }
};
static const struct message WmLButtonUpSeq[] =
{
    { WM_LBUTTONUP, sent|wparam|lparam, 0, 0 },
    { BM_SETSTATE, sent|wparam|defwinproc, FALSE },
    { WM_CTLCOLORBTN, sent|defwinproc },
    { EVENT_OBJECT_STATECHANGE, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { EVENT_SYSTEM_CAPTUREEND, winevent_hook|wparam|lparam, 0, 0 },
    { WM_CAPTURECHANGED, sent|wparam|defwinproc, 0 },
    { 0 }
};
static const struct message WmSetFontButtonSeq[] =
{
    { WM_SETFONT, sent },
    { WM_PAINT, sent },
    { WM_ERASEBKGND, sent|defwinproc|optional },
    { WM_CTLCOLORBTN, sent|defwinproc },
    { 0 }
};
static const struct message WmSetStyleButtonSeq[] =
{
    { BM_SETSTYLE, sent },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { WM_NCPAINT, sent|defwinproc|optional }, /* FIXME: Wine sends it */
    { WM_ERASEBKGND, sent|defwinproc|optional }, /* Win9x doesn't send it */
    { WM_CTLCOLORBTN, sent|parent },
    { 0 }
};
static const struct message WmSetStyleStaticSeq[] =
{
    { BM_SETSTYLE, sent },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { WM_NCPAINT, sent|defwinproc|optional }, /* FIXME: Wine sends it */
    { WM_ERASEBKGND, sent|defwinproc|optional }, /* Win9x doesn't send it */
    { WM_CTLCOLORSTATIC, sent|parent },
    { 0 }
};
static const struct message WmSetStyleUserSeq[] =
{
    { BM_SETSTYLE, sent },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { WM_NCPAINT, sent|defwinproc|optional }, /* FIXME: Wine sends it */
    { WM_ERASEBKGND, sent|defwinproc|optional }, /* Win9x doesn't send it */
    { WM_CTLCOLORBTN, sent|parent },
    { WM_COMMAND, sent|wparam|parent, MAKEWPARAM(ID_BUTTON, BN_PAINT) },
    { 0 }
};
static const struct message WmSetStyleOwnerdrawSeq[] =
{
    { BM_SETSTYLE, sent },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { WM_PAINT, sent },
    { WM_NCPAINT, sent|optional }, /* FIXME: Wine sends it */
    { WM_ERASEBKGND, sent|defwinproc|optional }, /* Win9x doesn't send it */
    { WM_CTLCOLORBTN, sent|parent },
    { WM_CTLCOLORBTN, sent|parent|optional }, /* Win9x doesn't send it */
    { WM_DRAWITEM, sent|wparam|lparam|parent, ID_BUTTON, 0x000010e4 },
    { 0 }
};
static const struct message WmSetStateButtonSeq[] =
{
    { BM_SETSTATE, sent },
    { WM_CTLCOLORBTN, sent|parent },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { 0 }
};
static const struct message WmSetStateStaticSeq[] =
{
    { BM_SETSTATE, sent },
    { WM_CTLCOLORSTATIC, sent|parent },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { 0 }
};
static const struct message WmSetStateUserSeq[] =
{
    { BM_SETSTATE, sent },
    { WM_CTLCOLORBTN, sent|parent },
    { WM_COMMAND, sent|wparam|parent, MAKEWPARAM(ID_BUTTON, BN_HILITE) },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { 0 }
};
static const struct message WmSetStateOwnerdrawSeq[] =
{
    { BM_SETSTATE, sent },
    { WM_CTLCOLORBTN, sent|parent },
    { WM_DRAWITEM, sent|wparam|lparam|parent, ID_BUTTON, 0x000120e4 },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { 0 }
};
static const struct message WmClearStateButtonSeq[] =
{
    { BM_SETSTATE, sent },
    { WM_CTLCOLORBTN, sent|parent },
    { WM_COMMAND, sent|wparam|parent, MAKEWPARAM(ID_BUTTON, BN_UNHILITE) },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { 0 }
};
static const struct message WmClearStateOwnerdrawSeq[] =
{
    { BM_SETSTATE, sent },
    { WM_CTLCOLORBTN, sent|parent },
    { WM_DRAWITEM, sent|wparam|lparam|parent, ID_BUTTON, 0x000020e4 },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { 0 }
};
static const struct message WmSetCheckIgnoredSeq[] =
{
    { BM_SETCHECK, sent },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { 0 }
};
static const struct message WmSetCheckStaticSeq[] =
{
    { BM_SETCHECK, sent },
    { WM_CTLCOLORSTATIC, sent|parent },
    { WM_APP, sent|wparam|lparam, 0, 0 },
    { 0 }
};

static WNDPROC old_button_proc;

static LRESULT CALLBACK button_hook_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct recvd_message msg;

    if (ignore_message( message )) return 0;

    switch (message)
    {
    case WM_SYNCPAINT:
        break;
    case BM_SETSTATE:
        if (GetCapture())
            ok(GetCapture() == hwnd, "GetCapture() = %p\n", GetCapture());
        /* fall through */
    default:
        msg.hwnd = hwnd;
        msg.message = message;
        msg.flags = sent|wparam|lparam;
        if (defwndproc_counter) msg.flags |= defwinproc;
        msg.wParam = wParam;
        msg.lParam = lParam;
        msg.descr = "button";
        add_message(&msg);
    }

    defwndproc_counter++;
    ret = CallWindowProcA(old_button_proc, hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static void subclass_button(void)
{
    WNDCLASSA cls;

    if (!GetClassInfoA(0, "button", &cls)) assert(0);

    old_button_proc = cls.lpfnWndProc;

    cls.hInstance = GetModuleHandleA(NULL);
    cls.lpfnWndProc = button_hook_proc;
    cls.lpszClassName = "my_button_class";
    UnregisterClassA(cls.lpszClassName, cls.hInstance);
    if (!RegisterClassA(&cls)) assert(0);
}

static void test_button_messages(void)
{
    static const struct
    {
	DWORD style;
	DWORD dlg_code;
	const struct message *setfocus;
	const struct message *killfocus;
	const struct message *setstyle;
	const struct message *setstate;
	const struct message *clearstate;
	const struct message *setcheck;
    } button[] = {
	{ BS_PUSHBUTTON, DLGC_BUTTON | DLGC_UNDEFPUSHBUTTON,
	  WmSetFocusButtonSeq, WmKillFocusButtonSeq, WmSetStyleButtonSeq,
          WmSetStateButtonSeq, WmSetStateButtonSeq, WmSetCheckIgnoredSeq },
	{ BS_DEFPUSHBUTTON, DLGC_BUTTON | DLGC_DEFPUSHBUTTON,
	  WmSetFocusButtonSeq, WmKillFocusButtonSeq, WmSetStyleButtonSeq,
          WmSetStateButtonSeq, WmSetStateButtonSeq, WmSetCheckIgnoredSeq },
	{ BS_CHECKBOX, DLGC_BUTTON,
	  WmSetFocusStaticSeq, WmKillFocusStaticSeq, WmSetStyleStaticSeq,
          WmSetStateStaticSeq, WmSetStateStaticSeq, WmSetCheckStaticSeq },
	{ BS_AUTOCHECKBOX, DLGC_BUTTON,
	  WmSetFocusStaticSeq, WmKillFocusStaticSeq, WmSetStyleStaticSeq,
          WmSetStateStaticSeq, WmSetStateStaticSeq, WmSetCheckStaticSeq },
	{ BS_RADIOBUTTON, DLGC_BUTTON | DLGC_RADIOBUTTON,
	  WmSetFocusStaticSeq, WmKillFocusStaticSeq, WmSetStyleStaticSeq,
          WmSetStateStaticSeq, WmSetStateStaticSeq, WmSetCheckStaticSeq },
	{ BS_3STATE, DLGC_BUTTON,
	  WmSetFocusStaticSeq, WmKillFocusStaticSeq, WmSetStyleStaticSeq,
          WmSetStateStaticSeq, WmSetStateStaticSeq, WmSetCheckStaticSeq },
	{ BS_AUTO3STATE, DLGC_BUTTON,
	  WmSetFocusStaticSeq, WmKillFocusStaticSeq, WmSetStyleStaticSeq,
          WmSetStateStaticSeq, WmSetStateStaticSeq, WmSetCheckStaticSeq },
	{ BS_GROUPBOX, DLGC_STATIC,
	  WmSetFocusStaticSeq, WmKillFocusStaticSeq, WmSetStyleStaticSeq,
          WmSetStateStaticSeq, WmSetStateStaticSeq, WmSetCheckIgnoredSeq },
	{ BS_USERBUTTON, DLGC_BUTTON | DLGC_UNDEFPUSHBUTTON,
	  WmSetFocusButtonSeq, WmKillFocusButtonSeq, WmSetStyleUserSeq,
          WmSetStateUserSeq, WmClearStateButtonSeq, WmSetCheckIgnoredSeq },
	{ BS_AUTORADIOBUTTON, DLGC_BUTTON | DLGC_RADIOBUTTON,
	  WmSetFocusStaticSeq, WmKillFocusStaticSeq, WmSetStyleStaticSeq,
          WmSetStateStaticSeq, WmSetStateStaticSeq, WmSetCheckStaticSeq },
	{ BS_OWNERDRAW, DLGC_BUTTON,
	  WmSetFocusOwnerdrawSeq, WmKillFocusOwnerdrawSeq, WmSetStyleOwnerdrawSeq,
          WmSetStateOwnerdrawSeq, WmClearStateOwnerdrawSeq, WmSetCheckIgnoredSeq },
    };
    unsigned int i;
    HWND hwnd, parent;
    DWORD dlg_code;
    HFONT zfont;

    /* selection with VK_SPACE should capture button window */
    hwnd = CreateWindowExA(0, "button", "test", BS_CHECKBOX | WS_VISIBLE | WS_POPUP,
                           0, 0, 50, 14, 0, 0, 0, NULL);
    ok(hwnd != 0, "Failed to create button window\n");
    ReleaseCapture();
    SetFocus(hwnd);
    SendMessageA(hwnd, WM_KEYDOWN, VK_SPACE, 0);
    ok(GetCapture() == hwnd, "Should be captured on VK_SPACE WM_KEYDOWN\n");
    SendMessageA(hwnd, WM_KEYUP, VK_SPACE, 0);
    DestroyWindow(hwnd);

    subclass_button();

    parent = CreateWindowExA(0, "TestParentClass", "Test parent", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                             100, 100, 200, 200, 0, 0, 0, NULL);
    ok(parent != 0, "Failed to create parent window\n");

    for (i = 0; i < sizeof(button)/sizeof(button[0]); i++)
    {
        MSG msg;
        DWORD style, state;

        trace("button style %08x\n", button[i].style);

        hwnd = CreateWindowExA(0, "my_button_class", "test", button[i].style | WS_CHILD | BS_NOTIFY,
                               0, 0, 50, 14, parent, (HMENU)ID_BUTTON, 0, NULL);
	ok(hwnd != 0, "Failed to create button window\n");

        style = GetWindowLongA(hwnd, GWL_STYLE);
        style &= ~(WS_CHILD | BS_NOTIFY);
        /* XP turns a BS_USERBUTTON into BS_PUSHBUTTON */
        if (button[i].style == BS_USERBUTTON)
            ok(style == BS_PUSHBUTTON, "expected style BS_PUSHBUTTON got %x\n", style);
        else
            ok(style == button[i].style, "expected style %x got %x\n", button[i].style, style);

	dlg_code = SendMessageA(hwnd, WM_GETDLGCODE, 0, 0);
	ok(dlg_code == button[i].dlg_code, "%u: wrong dlg_code %08x\n", i, dlg_code);

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);
	SetFocus(0);
	flush_events();
	SetFocus(0);
	flush_sequence();

        log_all_parent_messages++;

        ok(GetFocus() == 0, "expected focus 0, got %p\n", GetFocus());
	SetFocus(hwnd);
        SendMessageA(hwnd, WM_APP, 0, 0); /* place a separator mark here */
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
	ok_sequence(button[i].setfocus, "SetFocus(hwnd) on a button", FALSE);

	SetFocus(0);
        SendMessageA(hwnd, WM_APP, 0, 0); /* place a separator mark here */
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
	ok_sequence(button[i].killfocus, "SetFocus(0) on a button", FALSE);

        ok(GetFocus() == 0, "expected focus 0, got %p\n", GetFocus());

        SendMessageA(hwnd, BM_SETSTYLE, button[i].style | BS_BOTTOM, TRUE);
        SendMessageA(hwnd, WM_APP, 0, 0); /* place a separator mark here */
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        ok_sequence(button[i].setstyle, "BM_SETSTYLE on a button", FALSE);

        style = GetWindowLongA(hwnd, GWL_STYLE);
        style &= ~(WS_VISIBLE | WS_CHILD | BS_NOTIFY);
        /* XP doesn't turn a BS_USERBUTTON into BS_PUSHBUTTON here! */
        ok(style == button[i].style, "expected style %04x got %04x\n", button[i].style, style);

        state = SendMessageA(hwnd, BM_GETSTATE, 0, 0);
        ok(state == 0, "expected state 0, got %04x\n", state);

        flush_sequence();

        SendMessageA(hwnd, BM_SETSTATE, TRUE, 0);
        SendMessageA(hwnd, WM_APP, 0, 0); /* place a separator mark here */
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        ok_sequence(button[i].setstate, "BM_SETSTATE/TRUE on a button", FALSE);

        state = SendMessageA(hwnd, BM_GETSTATE, 0, 0);
        ok(state == 0x0004, "expected state 0x0004, got %04x\n", state);

        style = GetWindowLongA(hwnd, GWL_STYLE);
        style &= ~(WS_CHILD | BS_NOTIFY | WS_VISIBLE);
        ok(style == button[i].style, "expected style %04x got %04x\n", button[i].style, style);

        flush_sequence();

        SendMessageA(hwnd, BM_SETSTATE, FALSE, 0);
        SendMessageA(hwnd, WM_APP, 0, 0); /* place a separator mark here */
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        ok_sequence(button[i].clearstate, "BM_SETSTATE/FALSE on a button", FALSE);

        state = SendMessageA(hwnd, BM_GETSTATE, 0, 0);
        ok(state == 0, "expected state 0, got %04x\n", state);

        style = GetWindowLongA(hwnd, GWL_STYLE);
        style &= ~(WS_CHILD | BS_NOTIFY | WS_VISIBLE);
        ok(style == button[i].style, "expected style %04x got %04x\n", button[i].style, style);

        state = SendMessageA(hwnd, BM_GETCHECK, 0, 0);
        ok(state == BST_UNCHECKED, "expected BST_UNCHECKED, got %04x\n", state);

        flush_sequence();

        SendMessageA(hwnd, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessageA(hwnd, WM_APP, 0, 0); /* place a separator mark here */
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        ok_sequence(WmSetCheckIgnoredSeq, "BM_SETCHECK on a button", FALSE);

        state = SendMessageA(hwnd, BM_GETCHECK, 0, 0);
        ok(state == BST_UNCHECKED, "expected BST_UNCHECKED, got %04x\n", state);

        style = GetWindowLongA(hwnd, GWL_STYLE);
        style &= ~(WS_CHILD | BS_NOTIFY | WS_VISIBLE);
        ok(style == button[i].style, "expected style %04x got %04x\n", button[i].style, style);

        flush_sequence();

        SendMessageA(hwnd, BM_SETCHECK, BST_CHECKED, 0);
        SendMessageA(hwnd, WM_APP, 0, 0); /* place a separator mark here */
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        ok_sequence(button[i].setcheck, "BM_SETCHECK on a button", FALSE);

        state = SendMessageA(hwnd, BM_GETCHECK, 0, 0);
        if (button[i].style == BS_PUSHBUTTON ||
            button[i].style == BS_DEFPUSHBUTTON ||
            button[i].style == BS_GROUPBOX ||
            button[i].style == BS_USERBUTTON ||
            button[i].style == BS_OWNERDRAW)
            ok(state == BST_UNCHECKED, "expected check 0, got %04x\n", state);
        else
            ok(state == BST_CHECKED, "expected check 1, got %04x\n", state);

        style = GetWindowLongA(hwnd, GWL_STYLE);
        style &= ~(WS_CHILD | BS_NOTIFY | WS_VISIBLE);
        if (button[i].style == BS_RADIOBUTTON ||
            button[i].style == BS_AUTORADIOBUTTON)
            ok(style == (button[i].style | WS_TABSTOP), "expected style %04x | WS_TABSTOP got %04x\n", button[i].style, style);
        else
            ok(style == button[i].style, "expected style %04x got %04x\n", button[i].style, style);

        log_all_parent_messages--;

	DestroyWindow(hwnd);
    }

    DestroyWindow(parent);

    hwnd = CreateWindowExA(0, "my_button_class", "test", BS_PUSHBUTTON | WS_POPUP | WS_VISIBLE,
			   0, 0, 50, 14, 0, 0, 0, NULL);
    ok(hwnd != 0, "Failed to create button window\n");

    SetForegroundWindow(hwnd);
    flush_events();

    SetActiveWindow(hwnd);
    SetFocus(0);
    flush_sequence();

    SendMessageA(hwnd, WM_LBUTTONDOWN, 0, 0);
    ok_sequence(WmLButtonDownSeq, "WM_LBUTTONDOWN on a button", FALSE);

    SendMessageA(hwnd, WM_LBUTTONUP, 0, 0);
    ok_sequence(WmLButtonUpSeq, "WM_LBUTTONUP on a button", FALSE);

    flush_sequence();
    zfont = GetStockObject(SYSTEM_FONT);
    SendMessageA(hwnd, WM_SETFONT, (WPARAM)zfont, TRUE);
    UpdateWindow(hwnd);
    ok_sequence(WmSetFontButtonSeq, "WM_SETFONT on a button", FALSE);

    DestroyWindow(hwnd);
}

/****************** static message test *************************/
static const struct message WmSetFontStaticSeq[] =
{
    { WM_SETFONT, sent },
    { WM_PAINT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|defwinproc|optional },
    { WM_CTLCOLORSTATIC, sent|defwinproc|optional },
    { 0 }
};

static WNDPROC old_static_proc;

static LRESULT CALLBACK static_hook_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct recvd_message msg;

    if (ignore_message( message )) return 0;

    msg.hwnd = hwnd;
    msg.message = message;
    msg.flags = sent|wparam|lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.descr = "static";
    add_message(&msg);

    defwndproc_counter++;
    ret = CallWindowProcA(old_static_proc, hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static void subclass_static(void)
{
    WNDCLASSA cls;

    if (!GetClassInfoA(0, "static", &cls)) assert(0);

    old_static_proc = cls.lpfnWndProc;

    cls.hInstance = GetModuleHandleA(NULL);
    cls.lpfnWndProc = static_hook_proc;
    cls.lpszClassName = "my_static_class";
    UnregisterClassA(cls.lpszClassName, cls.hInstance);
    if (!RegisterClassA(&cls)) assert(0);
}

static void test_static_messages(void)
{
    /* FIXME: make as comprehensive as the button message test */
    static const struct
    {
	DWORD style;
	DWORD dlg_code;
	const struct message *setfont;
    } static_ctrl[] = {
	{ SS_LEFT, DLGC_STATIC,
	  WmSetFontStaticSeq }
    };
    unsigned int i;
    HWND hwnd;
    DWORD dlg_code;

    subclass_static();

    for (i = 0; i < sizeof(static_ctrl)/sizeof(static_ctrl[0]); i++)
    {
	hwnd = CreateWindowExA(0, "my_static_class", "test", static_ctrl[i].style | WS_POPUP,
			       0, 0, 50, 14, 0, 0, 0, NULL);
	ok(hwnd != 0, "Failed to create static window\n");

	dlg_code = SendMessageA(hwnd, WM_GETDLGCODE, 0, 0);
	ok(dlg_code == static_ctrl[i].dlg_code, "%u: wrong dlg_code %08x\n", i, dlg_code);

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);
	SetFocus(0);
	flush_sequence();

	trace("static style %08x\n", static_ctrl[i].style);
	SendMessageA(hwnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
	ok_sequence(static_ctrl[i].setfont, "WM_SETFONT on a static", FALSE);

	DestroyWindow(hwnd);
    }
}

/****************** ComboBox message test *************************/
#define ID_COMBOBOX 0x000f

static const struct message WmKeyDownComboSeq[] =
{
    { WM_KEYDOWN, sent|wparam|lparam, VK_DOWN, 0 },
    { WM_COMMAND, sent|wparam|defwinproc, MAKEWPARAM(1000, LBN_SELCHANGE) },
    { WM_COMMAND, sent|wparam|parent, MAKEWPARAM(ID_COMBOBOX, CBN_SELENDOK) },
    { WM_COMMAND, sent|wparam|parent, MAKEWPARAM(ID_COMBOBOX, CBN_SELCHANGE) },
    { WM_CTLCOLOREDIT, sent|parent },
    { WM_KEYUP, sent|wparam|lparam, VK_DOWN, 0 },
    { 0 }
};

static const struct message WmSetPosComboSeq[] =
{
    { WM_WINDOWPOSCHANGING, sent },
    { WM_NCCALCSIZE, sent|wparam, TRUE },
    { WM_CHILDACTIVATE, sent },
    { WM_WINDOWPOSCHANGED, sent },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_RESTORED },
    { WM_WINDOWPOSCHANGING, sent|defwinproc },
    { WM_NCCALCSIZE, sent|defwinproc|wparam, TRUE },
    { WM_WINDOWPOSCHANGED, sent|defwinproc },
    { WM_SIZE, sent|defwinproc|wparam, SIZE_RESTORED },
    { 0 }
};

static WNDPROC old_combobox_proc;

static LRESULT CALLBACK combobox_hook_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct recvd_message msg;

    /* do not log painting messages */
    if (message != WM_PAINT &&
        message != WM_NCPAINT &&
        message != WM_SYNCPAINT &&
        message != WM_ERASEBKGND &&
        message != WM_NCHITTEST &&
        message != WM_GETTEXT &&
        !ignore_message( message ))
    {
        msg.hwnd = hwnd;
        msg.message = message;
        msg.flags = sent|wparam|lparam;
        if (defwndproc_counter) msg.flags |= defwinproc;
        msg.wParam = wParam;
        msg.lParam = lParam;
        msg.descr = "combo";
        add_message(&msg);
    }

    defwndproc_counter++;
    ret = CallWindowProcA(old_combobox_proc, hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static void subclass_combobox(void)
{
    WNDCLASSA cls;

    if (!GetClassInfoA(0, "ComboBox", &cls)) assert(0);

    old_combobox_proc = cls.lpfnWndProc;

    cls.hInstance = GetModuleHandleA(NULL);
    cls.lpfnWndProc = combobox_hook_proc;
    cls.lpszClassName = "my_combobox_class";
    UnregisterClassA(cls.lpszClassName, cls.hInstance);
    if (!RegisterClassA(&cls)) assert(0);
}

static void test_combobox_messages(void)
{
    HWND parent, combo;
    LRESULT ret;

    subclass_combobox();

    parent = CreateWindowExA(0, "TestParentClass", "Test parent", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                             100, 100, 200, 200, 0, 0, 0, NULL);
    ok(parent != 0, "Failed to create parent window\n");
    flush_sequence();

    combo = CreateWindowExA(0, "my_combobox_class", "test", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
                           0, 0, 100, 150, parent, (HMENU)ID_COMBOBOX, 0, NULL);
    ok(combo != 0, "Failed to create combobox window\n");

    UpdateWindow(combo);

    ret = SendMessageA(combo, WM_GETDLGCODE, 0, 0);
    ok(ret == (DLGC_WANTCHARS | DLGC_WANTARROWS), "wrong dlg_code %08lx\n", ret);

    ret = SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)"item 0");
    ok(ret == 0, "expected 0, got %ld\n", ret);
    ret = SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)"item 1");
    ok(ret == 1, "expected 1, got %ld\n", ret);
    ret = SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)"item 2");
    ok(ret == 2, "expected 2, got %ld\n", ret);

    SendMessageA(combo, CB_SETCURSEL, 0, 0);
    SetFocus(combo);
    flush_sequence();

    log_all_parent_messages++;
    SendMessageA(combo, WM_KEYDOWN, VK_DOWN, 0);
    SendMessageA(combo, WM_KEYUP, VK_DOWN, 0);
    log_all_parent_messages--;
    ok_sequence(WmKeyDownComboSeq, "WM_KEYDOWN/VK_DOWN on a ComboBox", FALSE);

    flush_sequence();
    SetWindowPos(combo, 0, 10, 10, 120, 130, SWP_NOZORDER);
    ok_sequence(WmSetPosComboSeq, "repositioning messages on a ComboBox", FALSE);

    DestroyWindow(combo);
    DestroyWindow(parent);
}

/****************** WM_IME_KEYDOWN message test *******************/

static const struct message WmImeKeydownMsgSeq_0[] =
{
    { WM_IME_KEYDOWN, wparam, VK_RETURN },
    { WM_CHAR, wparam, 'A' },
    { 0 }
};

static const struct message WmImeKeydownMsgSeq_1[] =
{
    { WM_KEYDOWN, optional|wparam, VK_RETURN },
    { WM_CHAR,    optional|wparam, VK_RETURN },
    { 0 }
};

static LRESULT WINAPI wmime_keydown_procA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct recvd_message msg;

    msg.hwnd = hwnd;
    msg.message = message;
    msg.flags = wparam|lparam;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.descr = "wmime_keydown";
    add_message(&msg);

    return DefWindowProcA(hwnd, message, wParam, lParam);
}

static void register_wmime_keydown_class(void)
{
    WNDCLASSA cls;

    ZeroMemory(&cls, sizeof(WNDCLASSA));
    cls.lpfnWndProc = wmime_keydown_procA;
    cls.hInstance = GetModuleHandleA(0);
    cls.lpszClassName = "wmime_keydown_class";
    if (!RegisterClassA(&cls)) assert(0);
}

static void test_wmime_keydown_message(void)
{
    HWND hwnd;
    MSG msg;

    trace("Message sequences by WM_IME_KEYDOWN\n");

    register_wmime_keydown_class();
    hwnd = CreateWindowExA(0, "wmime_keydown_class", NULL, WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, 300, 300, 0,
                           NULL, NULL, 0);
    flush_events();
    flush_sequence();

    SendMessageA(hwnd, WM_IME_KEYDOWN, VK_RETURN, 0x1c0001);
    SendMessageA(hwnd, WM_CHAR, 'A', 1);
    ok_sequence(WmImeKeydownMsgSeq_0, "WM_IME_KEYDOWN 0", FALSE);

    while ( PeekMessageA(&msg, 0, 0, 0, PM_REMOVE) )
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    ok_sequence(WmImeKeydownMsgSeq_1, "WM_IME_KEYDOWN 1", FALSE);

    DestroyWindow(hwnd);
}

/************* painting message test ********************/

void dump_region(HRGN hrgn)
{
    DWORD i, size;
    RGNDATA *data = NULL;
    RECT *rect;

    if (!hrgn)
    {
        printf( "null region\n" );
        return;
    }
    if (!(size = GetRegionData( hrgn, 0, NULL ))) return;
    if (!(data = HeapAlloc( GetProcessHeap(), 0, size ))) return;
    GetRegionData( hrgn, size, data );
    printf("%d rects:", data->rdh.nCount );
    for (i = 0, rect = (RECT *)data->Buffer; i < data->rdh.nCount; i++, rect++)
        printf( " (%d,%d)-(%d,%d)", rect->left, rect->top, rect->right, rect->bottom );
    printf("\n");
    HeapFree( GetProcessHeap(), 0, data );
}

static void check_update_rgn( HWND hwnd, HRGN hrgn )
{
    INT ret;
    RECT r1, r2;
    HRGN tmp = CreateRectRgn( 0, 0, 0, 0 );
    HRGN update = CreateRectRgn( 0, 0, 0, 0 );

    ret = GetUpdateRgn( hwnd, update, FALSE );
    ok( ret != ERROR, "GetUpdateRgn failed\n" );
    if (ret == NULLREGION)
    {
        ok( !hrgn, "Update region shouldn't be empty\n" );
    }
    else
    {
        if (CombineRgn( tmp, hrgn, update, RGN_XOR ) != NULLREGION)
        {
            ok( 0, "Regions are different\n" );
            if (winetest_debug > 0)
            {
                printf( "Update region: " );
                dump_region( update );
                printf( "Wanted region: " );
                dump_region( hrgn );
            }
        }
    }
    GetRgnBox( update, &r1 );
    GetUpdateRect( hwnd, &r2, FALSE );
    ok( r1.left == r2.left && r1.top == r2.top && r1.right == r2.right && r1.bottom == r2.bottom,
        "Rectangles are different: %d,%d-%d,%d / %d,%d-%d,%d\n",
        r1.left, r1.top, r1.right, r1.bottom, r2.left, r2.top, r2.right, r2.bottom );

    DeleteObject( tmp );
    DeleteObject( update );
}

static const struct message WmInvalidateRgn[] = {
    { WM_NCPAINT, sent },
    { WM_GETTEXT, sent|defwinproc|optional },
    { 0 }
};

static const struct message WmGetUpdateRect[] = {
    { WM_NCPAINT, sent },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_PAINT, sent },
    { 0 }
};

static const struct message WmInvalidateFull[] = {
    { WM_NCPAINT, sent|wparam, 1 },
    { WM_GETTEXT, sent|defwinproc|optional },
    { 0 }
};

static const struct message WmInvalidateErase[] = {
    { WM_NCPAINT, sent|wparam, 1 },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent },
    { 0 }
};

static const struct message WmInvalidatePaint[] = {
    { WM_PAINT, sent },
    { WM_NCPAINT, sent|wparam|beginpaint, 1 },
    { WM_GETTEXT, sent|beginpaint|defwinproc|optional },
    { 0 }
};

static const struct message WmInvalidateErasePaint[] = {
    { WM_PAINT, sent },
    { WM_NCPAINT, sent|wparam|beginpaint, 1 },
    { WM_GETTEXT, sent|beginpaint|defwinproc|optional },
    { WM_ERASEBKGND, sent|beginpaint|optional },
    { 0 }
};

static const struct message WmInvalidateErasePaint2[] = {
    { WM_PAINT, sent },
    { WM_NCPAINT, sent|beginpaint },
    { WM_GETTEXT, sent|beginpaint|defwinproc|optional },
    { WM_ERASEBKGND, sent|beginpaint|optional },
    { 0 }
};

static const struct message WmErase[] = {
    { WM_ERASEBKGND, sent },
    { 0 }
};

static const struct message WmPaint[] = {
    { WM_PAINT, sent },
    { 0 }
};

static const struct message WmParentOnlyPaint[] = {
    { WM_PAINT, sent|parent },
    { 0 }
};

static const struct message WmInvalidateParent[] = {
    { WM_NCPAINT, sent|parent },
    { WM_GETTEXT, sent|defwinproc|parent|optional },
    { WM_ERASEBKGND, sent|parent },
    { 0 }
};

static const struct message WmInvalidateParentChild[] = {
    { WM_NCPAINT, sent|parent },
    { WM_GETTEXT, sent|defwinproc|parent|optional },
    { WM_ERASEBKGND, sent|parent },
    { WM_NCPAINT, sent },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent },
    { 0 }
};

static const struct message WmInvalidateParentChild2[] = {
    { WM_ERASEBKGND, sent|parent },
    { WM_NCPAINT, sent },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent },
    { 0 }
};

static const struct message WmParentPaint[] = {
    { WM_PAINT, sent|parent },
    { WM_PAINT, sent },
    { 0 }
};

static const struct message WmParentPaintNc[] = {
    { WM_PAINT, sent|parent },
    { WM_PAINT, sent },
    { WM_NCPAINT, sent|beginpaint },
    { WM_GETTEXT, sent|beginpaint|defwinproc|optional },
    { WM_ERASEBKGND, sent|beginpaint|optional },
    { 0 }
};

static const struct message WmChildPaintNc[] = {
    { WM_PAINT, sent },
    { WM_NCPAINT, sent|beginpaint },
    { WM_GETTEXT, sent|beginpaint|defwinproc|optional },
    { WM_ERASEBKGND, sent|beginpaint|optional },
    { 0 }
};

static const struct message WmParentErasePaint[] = {
    { WM_PAINT, sent|parent },
    { WM_NCPAINT, sent|parent|beginpaint },
    { WM_GETTEXT, sent|parent|beginpaint|defwinproc|optional },
    { WM_ERASEBKGND, sent|parent|beginpaint|optional },
    { WM_PAINT, sent },
    { WM_NCPAINT, sent|beginpaint },
    { WM_GETTEXT, sent|beginpaint|defwinproc|optional },
    { WM_ERASEBKGND, sent|beginpaint|optional },
    { 0 }
};

static const struct message WmParentOnlyNcPaint[] = {
    { WM_PAINT, sent|parent },
    { WM_NCPAINT, sent|parent|beginpaint },
    { WM_GETTEXT, sent|parent|beginpaint|defwinproc|optional },
    { 0 }
};

static const struct message WmSetParentStyle[] = {
    { WM_STYLECHANGING, sent|parent },
    { WM_STYLECHANGED, sent|parent },
    { 0 }
};

static void test_paint_messages(void)
{
    BOOL ret;
    RECT rect;
    POINT pt;
    MSG msg;
    HWND hparent, hchild;
    HRGN hrgn = CreateRectRgn( 0, 0, 0, 0 );
    HRGN hrgn2 = CreateRectRgn( 0, 0, 0, 0 );
    HWND hwnd = CreateWindowExA(0, "TestWindowClass", "Test overlapped", WS_OVERLAPPEDWINDOW,
                                100, 100, 200, 200, 0, 0, 0, NULL);
    ok (hwnd != 0, "Failed to create overlapped window\n");

    ShowWindow( hwnd, SW_SHOW );
    UpdateWindow( hwnd );
    flush_events();
    flush_sequence();

    check_update_rgn( hwnd, 0 );
    SetRectRgn( hrgn, 10, 10, 20, 20 );
    ret = RedrawWindow( hwnd, NULL, hrgn, RDW_INVALIDATE );
    ok(ret, "RedrawWindow returned %d instead of TRUE\n", ret);
    check_update_rgn( hwnd, hrgn );
    SetRectRgn( hrgn2, 20, 20, 30, 30 );
    ret = RedrawWindow( hwnd, NULL, hrgn2, RDW_INVALIDATE );
    ok(ret, "RedrawWindow returned %d instead of TRUE\n", ret);
    CombineRgn( hrgn, hrgn, hrgn2, RGN_OR );
    check_update_rgn( hwnd, hrgn );
    /* validate everything */
    ret = RedrawWindow( hwnd, NULL, NULL, RDW_VALIDATE );
    ok(ret, "RedrawWindow returned %d instead of TRUE\n", ret);
    check_update_rgn( hwnd, 0 );

    /* test empty region */
    SetRectRgn( hrgn, 10, 10, 10, 15 );
    ret = RedrawWindow( hwnd, NULL, hrgn, RDW_INVALIDATE );
    ok(ret, "RedrawWindow returned %d instead of TRUE\n", ret);
    check_update_rgn( hwnd, 0 );
    /* test empty rect */
    SetRect( &rect, 10, 10, 10, 15 );
    ret = RedrawWindow( hwnd, &rect, NULL, RDW_INVALIDATE );
    ok(ret, "RedrawWindow returned %d instead of TRUE\n", ret);
    check_update_rgn( hwnd, 0 );

    /* flush pending messages */
    flush_events();
    flush_sequence();

    GetClientRect( hwnd, &rect );
    SetRectRgn( hrgn, 0, 0, rect.right - rect.left, rect.bottom - rect.top );
    /* MSDN: if hwnd parameter is NULL, InvalidateRect invalidates and redraws
     * all windows and sends WM_ERASEBKGND and WM_NCPAINT.
     */
    trace("testing InvalidateRect(0, NULL, FALSE)\n");
    SetRectEmpty( &rect );
    ok(InvalidateRect(0, &rect, FALSE), "InvalidateRect(0, &rc, FALSE) should fail\n");
    check_update_rgn( hwnd, hrgn );
    ok_sequence( WmInvalidateErase, "InvalidateErase", FALSE );
    flush_events();
    ok_sequence( WmPaint, "Paint", FALSE );
    RedrawWindow( hwnd, NULL, NULL, RDW_VALIDATE );
    check_update_rgn( hwnd, 0 );

    /* MSDN: if hwnd parameter is NULL, ValidateRect invalidates and redraws
     * all windows and sends WM_ERASEBKGND and WM_NCPAINT.
     */
    trace("testing ValidateRect(0, NULL)\n");
    SetRectEmpty( &rect );
    if (ValidateRect(0, &rect))  /* not supported on Win9x */
    {
        check_update_rgn( hwnd, hrgn );
        ok_sequence( WmInvalidateErase, "InvalidateErase", FALSE );
        flush_events();
        ok_sequence( WmPaint, "Paint", FALSE );
        RedrawWindow( hwnd, NULL, NULL, RDW_VALIDATE );
        check_update_rgn( hwnd, 0 );
    }

    trace("testing InvalidateRgn(0, NULL, FALSE)\n");
    SetLastError(0xdeadbeef);
    ok(!InvalidateRgn(0, NULL, FALSE), "InvalidateRgn(0, NULL, FALSE) should fail\n");
    ok(GetLastError() == ERROR_INVALID_WINDOW_HANDLE || GetLastError() == 0xdeadbeef,
       "wrong error code %d\n", GetLastError());
    check_update_rgn( hwnd, 0 );
    flush_events();
    ok_sequence( WmEmptySeq, "WmEmptySeq", FALSE );

    trace("testing ValidateRgn(0, NULL)\n");
    SetLastError(0xdeadbeef);
    ok(!ValidateRgn(0, NULL), "ValidateRgn(0, NULL) should fail\n");
    ok(GetLastError() == ERROR_INVALID_WINDOW_HANDLE ||
       broken( GetLastError() == 0xdeadbeef ) /* win9x */,
       "wrong error code %d\n", GetLastError());
    check_update_rgn( hwnd, 0 );
    flush_events();
    ok_sequence( WmEmptySeq, "WmEmptySeq", FALSE );

    trace("testing UpdateWindow(NULL)\n");
    SetLastError(0xdeadbeef);
    ok(!UpdateWindow(NULL), "UpdateWindow(NULL) should fail\n");
    ok(GetLastError() == ERROR_INVALID_WINDOW_HANDLE ||
       broken( GetLastError() == 0xdeadbeef ) /* win9x */,
       "wrong error code %d\n", GetLastError());
    check_update_rgn( hwnd, 0 );
    flush_events();
    ok_sequence( WmEmptySeq, "WmEmptySeq", FALSE );

    /* now with frame */
    SetRectRgn( hrgn, -5, -5, 20, 20 );

    /* flush pending messages */
    flush_events();
    flush_sequence();
    RedrawWindow( hwnd, NULL, hrgn, RDW_INVALIDATE | RDW_FRAME );
    ok_sequence( WmEmptySeq, "EmptySeq", FALSE );

    SetRectRgn( hrgn, 0, 0, 20, 20 );  /* GetUpdateRgn clips to client area */
    check_update_rgn( hwnd, hrgn );

    flush_sequence();
    RedrawWindow( hwnd, NULL, hrgn, RDW_INVALIDATE | RDW_FRAME | RDW_ERASENOW );
    ok_sequence( WmInvalidateRgn, "InvalidateRgn", FALSE );

    flush_sequence();
    RedrawWindow( hwnd, NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_ERASENOW );
    ok_sequence( WmInvalidateFull, "InvalidateFull", FALSE );

    GetClientRect( hwnd, &rect );
    SetRectRgn( hrgn, rect.left, rect.top, rect.right, rect.bottom );
    check_update_rgn( hwnd, hrgn );

    flush_sequence();
    RedrawWindow( hwnd, NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_ERASENOW );
    ok_sequence( WmInvalidateErase, "InvalidateErase", FALSE );

    flush_sequence();
    RedrawWindow( hwnd, NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_ERASENOW | RDW_UPDATENOW );
    ok_sequence( WmInvalidatePaint, "InvalidatePaint", FALSE );
    check_update_rgn( hwnd, 0 );

    flush_sequence();
    RedrawWindow( hwnd, NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_UPDATENOW );
    ok_sequence( WmInvalidateErasePaint, "InvalidateErasePaint", FALSE );
    check_update_rgn( hwnd, 0 );

    flush_sequence();
    SetRectRgn( hrgn, 0, 0, 100, 100 );
    RedrawWindow( hwnd, NULL, hrgn, RDW_INVALIDATE );
    SetRectRgn( hrgn, 0, 0, 50, 100 );
    RedrawWindow( hwnd, NULL, hrgn, RDW_VALIDATE );
    SetRectRgn( hrgn, 50, 0, 100, 100 );
    check_update_rgn( hwnd, hrgn );
    RedrawWindow( hwnd, NULL, hrgn, RDW_VALIDATE | RDW_ERASENOW );
    ok_sequence( WmEmptySeq, "EmptySeq", FALSE );  /* must not generate messages, everything is valid */
    check_update_rgn( hwnd, 0 );

    flush_sequence();
    SetRectRgn( hrgn, 0, 0, 100, 100 );
    RedrawWindow( hwnd, NULL, hrgn, RDW_INVALIDATE | RDW_ERASE );
    SetRectRgn( hrgn, 0, 0, 100, 50 );
    RedrawWindow( hwnd, NULL, hrgn, RDW_VALIDATE | RDW_ERASENOW );
    ok_sequence( WmErase, "Erase", FALSE );
    SetRectRgn( hrgn, 0, 50, 100, 100 );
    check_update_rgn( hwnd, hrgn );

    flush_sequence();
    SetRectRgn( hrgn, 0, 0, 100, 100 );
    RedrawWindow( hwnd, NULL, hrgn, RDW_INVALIDATE | RDW_ERASE );
    SetRectRgn( hrgn, 0, 0, 50, 50 );
    RedrawWindow( hwnd, NULL, hrgn, RDW_VALIDATE | RDW_NOERASE | RDW_UPDATENOW );
    ok_sequence( WmPaint, "Paint", FALSE );

    flush_sequence();
    SetRectRgn( hrgn, -4, -4, -2, -2 );
    RedrawWindow( hwnd, NULL, hrgn, RDW_INVALIDATE | RDW_FRAME );
    SetRectRgn( hrgn, -200, -200, -198, -198 );
    RedrawWindow( hwnd, NULL, hrgn, RDW_VALIDATE | RDW_NOFRAME | RDW_ERASENOW );
    ok_sequence( WmEmptySeq, "EmptySeq", FALSE );

    flush_sequence();
    SetRectRgn( hrgn, -4, -4, -2, -2 );
    RedrawWindow( hwnd, NULL, hrgn, RDW_INVALIDATE | RDW_FRAME );
    SetRectRgn( hrgn, -4, -4, -3, -3 );
    RedrawWindow( hwnd, NULL, hrgn, RDW_VALIDATE | RDW_NOFRAME );
    SetRectRgn( hrgn, 0, 0, 1, 1 );
    RedrawWindow( hwnd, NULL, hrgn, RDW_INVALIDATE | RDW_UPDATENOW );
    ok_sequence( WmPaint, "Paint", FALSE );

    flush_sequence();
    SetRectRgn( hrgn, -4, -4, -1, -1 );
    RedrawWindow( hwnd, NULL, hrgn, RDW_INVALIDATE | RDW_FRAME );
    RedrawWindow( hwnd, NULL, 0, RDW_ERASENOW );
    /* make sure no WM_PAINT was generated */
    flush_events();
    ok_sequence( WmInvalidateRgn, "InvalidateRgn", FALSE );

    flush_sequence();
    SetRectRgn( hrgn, -4, -4, -1, -1 );
    RedrawWindow( hwnd, NULL, hrgn, RDW_INVALIDATE | RDW_FRAME );
    while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE ))
    {
        if (msg.hwnd == hwnd && msg.message == WM_PAINT)
        {
            /* GetUpdateRgn must return empty region since only nonclient area is invalidated */
            INT ret = GetUpdateRgn( hwnd, hrgn, FALSE );
            ok( ret == NULLREGION, "Invalid GetUpdateRgn result %d\n", ret );
            ret = GetUpdateRect( hwnd, &rect, FALSE );
            ok( ret, "Invalid GetUpdateRect result %d\n", ret );
            /* this will send WM_NCPAINT and validate the non client area */
            ret = GetUpdateRect( hwnd, &rect, TRUE );
            ok( !ret, "Invalid GetUpdateRect result %d\n", ret );
        }
        DispatchMessageA( &msg );
    }
    ok_sequence( WmGetUpdateRect, "GetUpdateRect", FALSE );

    DestroyWindow( hwnd );

    /* now test with a child window */

    hparent = CreateWindowExA(0, "TestParentClass", "Test parent", WS_OVERLAPPEDWINDOW,
                              100, 100, 200, 200, 0, 0, 0, NULL);
    ok (hparent != 0, "Failed to create parent window\n");

    hchild = CreateWindowExA(0, "TestWindowClass", "Test child", WS_CHILD | WS_VISIBLE | WS_BORDER,
                           10, 10, 100, 100, hparent, 0, 0, NULL);
    ok (hchild != 0, "Failed to create child window\n");

    ShowWindow( hparent, SW_SHOW );
    UpdateWindow( hparent );
    UpdateWindow( hchild );
    flush_events();
    flush_sequence();
    log_all_parent_messages++;

    SetRect( &rect, 0, 0, 50, 50 );
    RedrawWindow( hparent, &rect, 0, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME );
    RedrawWindow( hparent, NULL, 0, RDW_ERASENOW | RDW_ALLCHILDREN );
    ok_sequence( WmInvalidateParentChild, "InvalidateParentChild", FALSE );

    RedrawWindow( hparent, &rect, 0, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME );
    pt.x = pt.y = 0;
    MapWindowPoints( hchild, hparent, &pt, 1 );
    SetRectRgn( hrgn, 0, 0, 50 - pt.x, 50 - pt.y );
    check_update_rgn( hchild, hrgn );
    SetRectRgn( hrgn, 0, 0, 50, 50 );
    check_update_rgn( hparent, hrgn );
    RedrawWindow( hparent, NULL, 0, RDW_ERASENOW );
    ok_sequence( WmInvalidateParent, "InvalidateParent", FALSE );
    RedrawWindow( hchild, NULL, 0, RDW_ERASENOW );
    ok_sequence( WmEmptySeq, "EraseNow child", FALSE );

    flush_events();
    ok_sequence( WmParentPaintNc, "WmParentPaintNc", FALSE );

    RedrawWindow( hparent, &rect, 0, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN );
    RedrawWindow( hparent, NULL, 0, RDW_ERASENOW );
    ok_sequence( WmInvalidateParent, "InvalidateParent2", FALSE );
    RedrawWindow( hchild, NULL, 0, RDW_ERASENOW );
    ok_sequence( WmEmptySeq, "EraseNow child", FALSE );

    RedrawWindow( hparent, &rect, 0, RDW_INVALIDATE | RDW_ERASE );
    RedrawWindow( hparent, NULL, 0, RDW_ERASENOW | RDW_ALLCHILDREN );
    ok_sequence( WmInvalidateParentChild2, "InvalidateParentChild2", FALSE );

    SetWindowLongA( hparent, GWL_STYLE, GetWindowLongA(hparent,GWL_STYLE) | WS_CLIPCHILDREN );
    flush_sequence();
    RedrawWindow( hparent, &rect, 0, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN );
    RedrawWindow( hparent, NULL, 0, RDW_ERASENOW );
    ok_sequence( WmInvalidateParentChild, "InvalidateParentChild3", FALSE );

    /* flush all paint messages */
    flush_events();
    flush_sequence();

    /* RDW_UPDATENOW on child with WS_CLIPCHILDREN doesn't change corresponding parent area */
    RedrawWindow( hparent, &rect, 0, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN );
    SetRectRgn( hrgn, 0, 0, 50, 50 );
    check_update_rgn( hparent, hrgn );
    RedrawWindow( hchild, NULL, 0, RDW_UPDATENOW );
    ok_sequence( WmInvalidateErasePaint2, "WmInvalidateErasePaint2", FALSE );
    SetRectRgn( hrgn, 0, 0, 50, 50 );
    check_update_rgn( hparent, hrgn );

    /* flush all paint messages */
    flush_events();
    SetWindowLongA( hparent, GWL_STYLE, GetWindowLongA(hparent,GWL_STYLE) & ~WS_CLIPCHILDREN );
    flush_sequence();

    /* RDW_UPDATENOW on child without WS_CLIPCHILDREN will validate corresponding parent area */
    RedrawWindow( hparent, &rect, 0, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME );
    SetRectRgn( hrgn, 0, 0, 50, 50 );
    check_update_rgn( hparent, hrgn );
    RedrawWindow( hchild, NULL, 0, RDW_UPDATENOW );
    ok_sequence( WmInvalidateErasePaint2, "WmInvalidateErasePaint2", FALSE );
    SetRectRgn( hrgn2, 10, 10, 50, 50 );
    CombineRgn( hrgn, hrgn, hrgn2, RGN_DIFF );
    check_update_rgn( hparent, hrgn );
    /* flush all paint messages */
    flush_events();
    flush_sequence();

    /* same as above but parent gets completely validated */
    SetRect( &rect, 20, 20, 30, 30 );
    RedrawWindow( hparent, &rect, 0, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME );
    SetRectRgn( hrgn, 20, 20, 30, 30 );
    check_update_rgn( hparent, hrgn );
    RedrawWindow( hchild, NULL, 0, RDW_UPDATENOW );
    ok_sequence( WmInvalidateErasePaint2, "WmInvalidateErasePaint2", FALSE );
    check_update_rgn( hparent, 0 );  /* no update region */
    flush_events();
    ok_sequence( WmEmptySeq, "WmEmpty", FALSE );  /* and no paint messages */

    /* make sure RDW_VALIDATE on child doesn't have the same effect */
    flush_sequence();
    RedrawWindow( hparent, &rect, 0, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME );
    SetRectRgn( hrgn, 20, 20, 30, 30 );
    check_update_rgn( hparent, hrgn );
    RedrawWindow( hchild, NULL, 0, RDW_VALIDATE | RDW_NOERASE );
    SetRectRgn( hrgn, 20, 20, 30, 30 );
    check_update_rgn( hparent, hrgn );

    /* same as above but normal WM_PAINT doesn't validate parent */
    flush_sequence();
    SetRect( &rect, 20, 20, 30, 30 );
    RedrawWindow( hparent, &rect, 0, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME );
    SetRectRgn( hrgn, 20, 20, 30, 30 );
    check_update_rgn( hparent, hrgn );
    /* no WM_PAINT in child while parent still pending */
    while (PeekMessageA( &msg, hchild, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
    ok_sequence( WmEmptySeq, "No WM_PAINT", FALSE );
    while (PeekMessageA( &msg, hparent, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
    ok_sequence( WmParentErasePaint, "WmParentErasePaint", FALSE );

    flush_sequence();
    RedrawWindow( hparent, &rect, 0, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME );
    /* no WM_PAINT in child while parent still pending */
    while (PeekMessageA( &msg, hchild, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
    ok_sequence( WmEmptySeq, "No WM_PAINT", FALSE );
    RedrawWindow( hparent, &rect, 0, RDW_VALIDATE | RDW_NOERASE | RDW_NOCHILDREN );
    /* now that parent is valid child should get WM_PAINT */
    while (PeekMessageA( &msg, hchild, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
    ok_sequence( WmInvalidateErasePaint2, "WmInvalidateErasePaint2", FALSE );
    while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
    ok_sequence( WmEmptySeq, "No other message", FALSE );

    /* same thing with WS_CLIPCHILDREN in parent */
    flush_sequence();
    SetWindowLongA( hparent, GWL_STYLE, GetWindowLongA(hparent,GWL_STYLE) | WS_CLIPCHILDREN );
    ok_sequence( WmSetParentStyle, "WmSetParentStyle", FALSE );
    /* changing style invalidates non client area, but we need to invalidate something else to see it */
    RedrawWindow( hparent, &rect, 0, RDW_UPDATENOW );
    ok_sequence( WmEmptySeq, "No message", FALSE );
    RedrawWindow( hparent, &rect, 0, RDW_INVALIDATE | RDW_UPDATENOW );
    ok_sequence( WmParentOnlyNcPaint, "WmParentOnlyNcPaint", FALSE );

    flush_sequence();
    RedrawWindow( hparent, &rect, 0, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN );
    SetRectRgn( hrgn, 20, 20, 30, 30 );
    check_update_rgn( hparent, hrgn );
    /* no WM_PAINT in child while parent still pending */
    while (PeekMessageA( &msg, hchild, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
    ok_sequence( WmEmptySeq, "No WM_PAINT", FALSE );
    /* WM_PAINT in parent first */
    while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
    ok_sequence( WmParentPaintNc, "WmParentPaintNc2", FALSE );

    /* no RDW_ERASE in parent still causes RDW_ERASE and RDW_FRAME in child */
    flush_sequence();
    SetRect( &rect, 0, 0, 30, 30 );
    RedrawWindow( hparent, &rect, 0, RDW_INVALIDATE | RDW_ALLCHILDREN );
    SetRectRgn( hrgn, 0, 0, 30, 30 );
    check_update_rgn( hparent, hrgn );
    flush_events();
    ok_sequence( WmParentPaintNc, "WmParentPaintNc3", FALSE );

    /* validate doesn't cause RDW_NOERASE or RDW_NOFRAME in child */
    flush_sequence();
    SetRect( &rect, -10, 0, 30, 30 );
    RedrawWindow( hchild, &rect, 0, RDW_INVALIDATE | RDW_FRAME | RDW_ERASE );
    SetRect( &rect, 0, 0, 20, 20 );
    RedrawWindow( hparent, &rect, 0, RDW_VALIDATE | RDW_ALLCHILDREN );
    RedrawWindow( hparent, NULL, 0, RDW_UPDATENOW );
    ok_sequence( WmChildPaintNc, "WmChildPaintNc", FALSE );

    /* validate doesn't cause RDW_NOERASE or RDW_NOFRAME in child */
    flush_sequence();
    SetRect( &rect, -10, 0, 30, 30 );
    RedrawWindow( hchild, &rect, 0, RDW_INVALIDATE | RDW_FRAME | RDW_ERASE );
    SetRect( &rect, 0, 0, 100, 100 );
    RedrawWindow( hparent, &rect, 0, RDW_VALIDATE | RDW_ALLCHILDREN );
    RedrawWindow( hparent, NULL, 0, RDW_UPDATENOW );
    ok_sequence( WmEmptySeq, "WmChildPaintNc2", FALSE );
    RedrawWindow( hparent, NULL, 0, RDW_ERASENOW );
    ok_sequence( WmEmptySeq, "WmChildPaintNc3", FALSE );

    /* test RDW_INTERNALPAINT behavior */

    flush_sequence();
    RedrawWindow( hparent, NULL, 0, RDW_INTERNALPAINT | RDW_NOCHILDREN );
    flush_events();
    ok_sequence( WmParentOnlyPaint, "WmParentOnlyPaint", FALSE );

    RedrawWindow( hparent, NULL, 0, RDW_INTERNALPAINT | RDW_ALLCHILDREN );
    flush_events();
    ok_sequence( WmParentPaint, "WmParentPaint", FALSE );

    RedrawWindow( hparent, NULL, 0, RDW_INTERNALPAINT );
    flush_events();
    ok_sequence( WmParentOnlyPaint, "WmParentOnlyPaint", FALSE );

    assert( GetWindowLongA(hparent, GWL_STYLE) & WS_CLIPCHILDREN );
    UpdateWindow( hparent );
    flush_events();
    flush_sequence();
    trace("testing SWP_FRAMECHANGED on parent with WS_CLIPCHILDREN\n");
    RedrawWindow( hchild, NULL, 0, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME );
    SetWindowPos( hparent, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
                  SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );
    flush_events();
    ok_sequence(WmSWP_FrameChanged_clip, "SetWindowPos:FrameChanged_clip", FALSE );

    UpdateWindow( hparent );
    flush_events();
    flush_sequence();
    trace("testing SWP_FRAMECHANGED|SWP_DEFERERASE on parent with WS_CLIPCHILDREN\n");
    RedrawWindow( hchild, NULL, 0, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME );
    SetWindowPos( hparent, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_DEFERERASE |
                  SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );
    flush_events();
    ok_sequence(WmSWP_FrameChangedDeferErase, "SetWindowPos:FrameChangedDeferErase", FALSE );

    SetWindowLongA( hparent, GWL_STYLE, GetWindowLongA(hparent,GWL_STYLE) & ~WS_CLIPCHILDREN );
    ok_sequence( WmSetParentStyle, "WmSetParentStyle", FALSE );
    RedrawWindow( hparent, NULL, 0, RDW_INTERNALPAINT );
    flush_events();
    ok_sequence( WmParentPaint, "WmParentPaint", FALSE );

    assert( !(GetWindowLongA(hparent, GWL_STYLE) & WS_CLIPCHILDREN) );
    UpdateWindow( hparent );
    flush_events();
    flush_sequence();
    trace("testing SWP_FRAMECHANGED on parent without WS_CLIPCHILDREN\n");
    RedrawWindow( hchild, NULL, 0, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME );
    SetWindowPos( hparent, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
                  SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );
    flush_events();
    ok_sequence(WmSWP_FrameChanged_noclip, "SetWindowPos:FrameChanged_noclip", FALSE );

    UpdateWindow( hparent );
    flush_events();
    flush_sequence();
    trace("testing SWP_FRAMECHANGED|SWP_DEFERERASE on parent without WS_CLIPCHILDREN\n");
    RedrawWindow( hchild, NULL, 0, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME );
    SetWindowPos( hparent, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_DEFERERASE |
                  SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );
    flush_events();
    ok_sequence(WmSWP_FrameChangedDeferErase, "SetWindowPos:FrameChangedDeferErase", FALSE );

    ok(GetWindowLongA( hparent, GWL_STYLE ) & WS_VISIBLE, "parent should be visible\n");
    ok(GetWindowLongA( hchild, GWL_STYLE ) & WS_VISIBLE, "child should be visible\n");

    UpdateWindow( hparent );
    flush_events();
    flush_sequence();
    trace("testing SetWindowPos(-10000, -10000) on child\n");
    SetWindowPos( hchild, 0, -10000, -10000, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER );
    check_update_rgn( hchild, 0 );
    flush_events();

#if 0 /* this one doesn't pass under Wine yet */
    UpdateWindow( hparent );
    flush_events();
    flush_sequence();
    trace("testing ShowWindow(SW_MINIMIZE) on child\n");
    ShowWindow( hchild, SW_MINIMIZE );
    check_update_rgn( hchild, 0 );
    flush_events();
#endif

    UpdateWindow( hparent );
    flush_events();
    flush_sequence();
    trace("testing SetWindowPos(-10000, -10000) on parent\n");
    SetWindowPos( hparent, 0, -10000, -10000, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER );
    check_update_rgn( hparent, 0 );
    flush_events();

    log_all_parent_messages--;
    DestroyWindow( hparent );
    ok(!IsWindow(hchild), "child must be destroyed with its parent\n");

    /* tests for moving windows off-screen (needs simple WS_POPUP windows) */

    hparent = CreateWindowExA(0, "TestParentClass", "Test parent", WS_POPUP | WS_VISIBLE,
                              100, 100, 200, 200, 0, 0, 0, NULL);
    ok (hparent != 0, "Failed to create parent window\n");

    hchild = CreateWindowExA(0, "TestWindowClass", "Test child", WS_CHILD | WS_VISIBLE,
                           10, 10, 100, 100, hparent, 0, 0, NULL);
    ok (hchild != 0, "Failed to create child window\n");

    ShowWindow( hparent, SW_SHOW );
    UpdateWindow( hparent );
    UpdateWindow( hchild );
    flush_events();
    flush_sequence();

    /* moving child outside of parent boundaries changes update region */
    SetRect( &rect, 0, 0, 40, 40 );
    RedrawWindow( hchild, &rect, 0, RDW_INVALIDATE | RDW_ERASE );
    SetRectRgn( hrgn, 0, 0, 40, 40 );
    check_update_rgn( hchild, hrgn );
    MoveWindow( hchild, -10, 10, 100, 100, FALSE );
    SetRectRgn( hrgn, 10, 0, 40, 40 );
    check_update_rgn( hchild, hrgn );
    MoveWindow( hchild, -10, -10, 100, 100, FALSE );
    SetRectRgn( hrgn, 10, 10, 40, 40 );
    check_update_rgn( hchild, hrgn );

    /* moving parent off-screen does too */
    SetRect( &rect, 0, 0, 100, 100 );
    RedrawWindow( hparent, &rect, 0, RDW_INVALIDATE | RDW_ERASE | RDW_NOCHILDREN );
    SetRectRgn( hrgn, 0, 0, 100, 100 );
    check_update_rgn( hparent, hrgn );
    SetRectRgn( hrgn, 10, 10, 40, 40 );
    check_update_rgn( hchild, hrgn );
    MoveWindow( hparent, -20, -20, 200, 200, FALSE );
    SetRectRgn( hrgn, 20, 20, 100, 100 );
    check_update_rgn( hparent, hrgn );
    SetRectRgn( hrgn, 30, 30, 40, 40 );
    check_update_rgn( hchild, hrgn );

    /* invalidated region is cropped by the parent rects */
    SetRect( &rect, 0, 0, 50, 50 );
    RedrawWindow( hchild, &rect, 0, RDW_INVALIDATE | RDW_ERASE );
    SetRectRgn( hrgn, 30, 30, 50, 50 );
    check_update_rgn( hchild, hrgn );

    DestroyWindow( hparent );
    ok(!IsWindow(hchild), "child must be destroyed with its parent\n");
    flush_sequence();

    DeleteObject( hrgn );
    DeleteObject( hrgn2 );
}

struct wnd_event
{
    HWND hwnd;
    HANDLE grand_child;
    HANDLE start_event;
    HANDLE stop_event;
};

static DWORD WINAPI thread_proc(void *param)
{
    MSG msg;
    struct wnd_event *wnd_event = param;

    wnd_event->hwnd = CreateWindowExA(0, "TestWindowClass", "window caption text", WS_OVERLAPPEDWINDOW,
                                      100, 100, 200, 200, 0, 0, 0, NULL);
    ok(wnd_event->hwnd != 0, "Failed to create overlapped window\n");

    SetEvent(wnd_event->start_event);

    while (GetMessageA(&msg, 0, 0, 0))
    {
	TranslateMessage(&msg);
	DispatchMessageA(&msg);
    }

    ok(IsWindow(wnd_event->hwnd), "window should still exist\n");

    return 0;
}

static DWORD CALLBACK create_grand_child_thread( void *param )
{
    struct wnd_event *wnd_event = param;
    HWND hchild;
    MSG msg;

    hchild = CreateWindowExA(0, "TestWindowClass", "Test child",
                             WS_CHILD | WS_VISIBLE, 0, 0, 10, 10, wnd_event->hwnd, 0, 0, NULL);
    ok (hchild != 0, "Failed to create child window\n");
    flush_events();
    flush_sequence();
    SetEvent( wnd_event->start_event );

    for (;;)
    {
        MsgWaitForMultipleObjects(0, NULL, FALSE, 1000, QS_ALLINPUT);
        if (!IsWindow( hchild )) break;  /* will be destroyed when parent thread exits */
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    }
    return 0;
}

static DWORD CALLBACK create_child_thread( void *param )
{
    struct wnd_event *wnd_event = param;
    struct wnd_event child_event;
    DWORD ret, tid;
    MSG msg;

    child_event.hwnd = CreateWindowExA(0, "TestWindowClass", "Test child",
                             WS_CHILD | WS_VISIBLE, 0, 0, 10, 10, wnd_event->hwnd, 0, 0, NULL);
    ok (child_event.hwnd != 0, "Failed to create child window\n");
    SetFocus( child_event.hwnd );
    flush_events();
    flush_sequence();
    child_event.start_event = wnd_event->start_event;
    wnd_event->grand_child = CreateThread(NULL, 0, create_grand_child_thread, &child_event, 0, &tid);
    for (;;)
    {
        DWORD ret = MsgWaitForMultipleObjects(1, &child_event.start_event, FALSE, 1000, QS_SENDMESSAGE);
        if (ret != 1) break;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    }
    ret = WaitForSingleObject( wnd_event->stop_event, 5000 );
    ok( !ret, "WaitForSingleObject failed %x\n", ret );
    return 0;
}

static const char manifest_dep[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.2.3.4\"  name=\"testdep1\" type=\"win32\" processorArchitecture=\"" ARCH "\"/>"
"    <file name=\"testdep.dll\" />"
"</assembly>";

static const char manifest_main[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.2.3.4\" name=\"Wine.Test\" type=\"win32\" />"
"<dependency>"
" <dependentAssembly>"
"  <assemblyIdentity type=\"win32\" name=\"testdep1\" version=\"1.2.3.4\" processorArchitecture=\"" ARCH "\" />"
" </dependentAssembly>"
"</dependency>"
"</assembly>";

static void create_manifest_file(const char *filename, const char *manifest)
{
    WCHAR path[MAX_PATH];
    HANDLE file;
    DWORD size;

    MultiByteToWideChar( CP_ACP, 0, filename, -1, path, MAX_PATH );
    file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed: %u\n", GetLastError());
    WriteFile(file, manifest, strlen(manifest), &size, NULL);
    CloseHandle(file);
}

static HANDLE test_create(const char *file)
{
    WCHAR path[MAX_PATH];
    ACTCTXW actctx;
    HANDLE handle;

    MultiByteToWideChar(CP_ACP, 0, file, -1, path, MAX_PATH);
    memset(&actctx, 0, sizeof(ACTCTXW));
    actctx.cbSize = sizeof(ACTCTXW);
    actctx.lpSource = path;

    handle = pCreateActCtxW(&actctx);
    ok(handle != INVALID_HANDLE_VALUE, "failed to create context, error %u\n", GetLastError());

    ok(actctx.cbSize == sizeof(actctx), "cbSize=%d\n", actctx.cbSize);
    ok(actctx.dwFlags == 0, "dwFlags=%d\n", actctx.dwFlags);
    ok(actctx.lpSource == path, "lpSource=%p\n", actctx.lpSource);
    ok(actctx.wProcessorArchitecture == 0, "wProcessorArchitecture=%d\n", actctx.wProcessorArchitecture);
    ok(actctx.wLangId == 0, "wLangId=%d\n", actctx.wLangId);
    ok(actctx.lpAssemblyDirectory == NULL, "lpAssemblyDirectory=%p\n", actctx.lpAssemblyDirectory);
    ok(actctx.lpResourceName == NULL, "lpResourceName=%p\n", actctx.lpResourceName);
    ok(actctx.lpApplicationName == NULL, "lpApplicationName=%p\n", actctx.lpApplicationName);
    ok(actctx.hModule == NULL, "hModule=%p\n", actctx.hModule);

    return handle;
}

static void test_interthread_messages(void)
{
    HANDLE hThread, context, handle, event;
    ULONG_PTR cookie;
    DWORD tid;
    WNDPROC proc;
    MSG msg;
    char buf[256];
    int len, expected_len;
    struct wnd_event wnd_event;
    BOOL ret;

    wnd_event.start_event = CreateEventW(NULL, 0, 0, NULL);
    if (!wnd_event.start_event)
    {
        win_skip("skipping interthread message test under win9x\n");
        return;
    }

    hThread = CreateThread(NULL, 0, thread_proc, &wnd_event, 0, &tid);
    ok(hThread != NULL, "CreateThread failed, error %d\n", GetLastError());

    ok(WaitForSingleObject(wnd_event.start_event, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");

    CloseHandle(wnd_event.start_event);

    SetLastError(0xdeadbeef);
    ok(!DestroyWindow(wnd_event.hwnd), "DestroyWindow succeeded\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED || GetLastError() == 0xdeadbeef,
       "wrong error code %d\n", GetLastError());

    proc = (WNDPROC)GetWindowLongPtrA(wnd_event.hwnd, GWLP_WNDPROC);
    ok(proc != NULL, "GetWindowLongPtrA(GWLP_WNDPROC) error %d\n", GetLastError());

    expected_len = lstrlenA("window caption text");
    memset(buf, 0, sizeof(buf));
    SetLastError(0xdeadbeef);
    len = CallWindowProcA(proc, wnd_event.hwnd, WM_GETTEXT, sizeof(buf), (LPARAM)buf);
    ok(len == expected_len, "CallWindowProcA(WM_GETTEXT) error %d, len %d, expected len %d\n", GetLastError(), len, expected_len);
    ok(!lstrcmpA(buf, "window caption text"), "window text mismatch\n");

    msg.hwnd = wnd_event.hwnd;
    msg.message = WM_GETTEXT;
    msg.wParam = sizeof(buf);
    msg.lParam = (LPARAM)buf;
    memset(buf, 0, sizeof(buf));
    SetLastError(0xdeadbeef);
    len = DispatchMessageA(&msg);
    ok((!len && GetLastError() == ERROR_MESSAGE_SYNC_ONLY) || broken(len), /* nt4 */
       "DispatchMessageA(WM_GETTEXT) succeeded on another thread window: ret %d, error %d\n", len, GetLastError());

    /* the following test causes an exception in user.exe under win9x */
    msg.hwnd = wnd_event.hwnd;
    msg.message = WM_TIMER;
    msg.wParam = 0;
    msg.lParam = GetWindowLongPtrA(wnd_event.hwnd, GWLP_WNDPROC);
    SetLastError(0xdeadbeef);
    len = DispatchMessageA(&msg);
    ok(!len && GetLastError() == 0xdeadbeef,
       "DispatchMessageA(WM_TIMER) failed on another thread window: ret %d, error %d\n", len, GetLastError());

    ret = PostMessageA(wnd_event.hwnd, WM_QUIT, 0, 0);
    ok( ret, "PostMessageA(WM_QUIT) error %d\n", GetLastError());

    ok(WaitForSingleObject(hThread, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(hThread);

    ok(!IsWindow(wnd_event.hwnd), "window should be destroyed on thread exit\n");

    wnd_event.hwnd = CreateWindowExA(0, "TestParentClass", "Test parent", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              100, 100, 200, 200, 0, 0, 0, NULL);
    ok (wnd_event.hwnd != 0, "Failed to create parent window\n");
    flush_events();
    flush_sequence();
    log_all_parent_messages++;
    wnd_event.start_event = CreateEventA( NULL, TRUE, FALSE, NULL );
    wnd_event.stop_event = CreateEventA( NULL, TRUE, FALSE, NULL );
    hThread = CreateThread( NULL, 0, create_child_thread, &wnd_event, 0, &tid );
    for (;;)
    {
        ret = MsgWaitForMultipleObjects(1, &wnd_event.start_event, FALSE, 1000, QS_SENDMESSAGE);
        if (ret != 1) break;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    }
    ok( !ret, "MsgWaitForMultipleObjects failed %x\n", ret );
    /* now wait for the thread without processing messages; this shouldn't deadlock */
    SetEvent( wnd_event.stop_event );
    ret = WaitForSingleObject( hThread, 5000 );
    ok( !ret, "WaitForSingleObject failed %x\n", ret );
    CloseHandle( hThread );

    ret = WaitForSingleObject( wnd_event.grand_child, 5000 );
    ok( !ret, "WaitForSingleObject failed %x\n", ret );
    CloseHandle( wnd_event.grand_child );

    CloseHandle( wnd_event.start_event );
    CloseHandle( wnd_event.stop_event );
    flush_events();
    ok_sequence(WmExitThreadSeq, "destroy child on thread exit", FALSE);
    log_all_parent_messages--;
    DestroyWindow( wnd_event.hwnd );

    /* activation context tests */
    if (!pActivateActCtx)
    {
        win_skip("Activation contexts are not supported, skipping\n");
        return;
    }

    create_manifest_file("testdep1.manifest", manifest_dep);
    create_manifest_file("main.manifest", manifest_main);

    context = test_create("main.manifest");
    DeleteFileA("testdep1.manifest");
    DeleteFileA("main.manifest");

    handle = (void*)0xdeadbeef;
    ret = pGetCurrentActCtx(&handle);
    ok(ret, "GetCurentActCtx failed: %u\n", GetLastError());
    ok(handle == 0, "active context %p\n", handle);

    wnd_event.start_event = CreateEventW(NULL, 0, 0, NULL);
    hThread = CreateThread(NULL, 0, thread_proc, &wnd_event, 0, &tid);
    ok(hThread != NULL, "CreateThread failed, error %d\n", GetLastError());
    ok(WaitForSingleObject(wnd_event.start_event, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(wnd_event.start_event);

    /* context is activated after thread creation, so it doesn't inherit it by default */
    ret = pActivateActCtx(context, &cookie);
    ok(ret, "activation failed: %u\n", GetLastError());

    handle = 0;
    ret = pGetCurrentActCtx(&handle);
    ok(ret, "GetCurentActCtx failed: %u\n", GetLastError());
    ok(handle != 0, "active context %p\n", handle);
    pReleaseActCtx(handle);

    /* destination window will test for active context */
    ret = SendMessageA(wnd_event.hwnd, WM_USER+10, 0, 0);
    ok(ret, "thread window returned %d\n", ret);

    event = CreateEventW(NULL, 0, 0, NULL);
    ret = PostMessageA(wnd_event.hwnd, WM_USER+10, 0, (LPARAM)event);
    ok(ret, "thread window returned %d\n", ret);
    ok(WaitForSingleObject(event, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(event);

    ret = PostMessageA(wnd_event.hwnd, WM_QUIT, 0, 0);
    ok(ret, "PostMessageA(WM_QUIT) error %d\n", GetLastError());

    ok(WaitForSingleObject(hThread, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(hThread);

    ret = pDeactivateActCtx(0, cookie);
    ok(ret, "DeactivateActCtx failed: %u\n", GetLastError());
    pReleaseActCtx(context);
}


static const struct message WmVkN[] = {
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'N', 1 }, /* XP */
    { WM_KEYDOWN, wparam|lparam, 'N', 1 },
    { WM_KEYDOWN, sent|wparam|lparam, 'N', 1 },
    { WM_CHAR, wparam|lparam, 'n', 1 },
    { WM_COMMAND, sent|wparam|lparam, MAKEWPARAM(1002,1), 0 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'N', 0xc0000001 }, /* XP */
    { WM_KEYUP, wparam|lparam, 'N', 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, 'N', 0xc0000001 },
    { 0 }
};
static const struct message WmShiftVkN[] = {
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_SHIFT, 1 }, /* XP */
    { WM_KEYDOWN, wparam|lparam, VK_SHIFT, 1 },
    { WM_KEYDOWN, sent|wparam|lparam, VK_SHIFT, 1 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'N', 1 }, /* XP */
    { WM_KEYDOWN, wparam|lparam, 'N', 1 },
    { WM_KEYDOWN, sent|wparam|lparam, 'N', 1 },
    { WM_CHAR, wparam|lparam, 'N', 1 },
    { WM_COMMAND, sent|wparam|lparam, MAKEWPARAM(1001,1), 0 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'N', 0xc0000001 }, /* XP */
    { WM_KEYUP, wparam|lparam, 'N', 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, 'N', 0xc0000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_SHIFT, 0xc0000001 }, /* XP */
    { WM_KEYUP, wparam|lparam, VK_SHIFT, 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, VK_SHIFT, 0xc0000001 },
    { 0 }
};
static const struct message WmCtrlVkN[] = {
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_CONTROL, 1 }, /* XP */
    { WM_KEYDOWN, wparam|lparam, VK_CONTROL, 1 },
    { WM_KEYDOWN, sent|wparam|lparam, VK_CONTROL, 1 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'N', 1 }, /* XP */
    { WM_KEYDOWN, wparam|lparam, 'N', 1 },
    { WM_KEYDOWN, sent|wparam|lparam, 'N', 1 },
    { WM_CHAR, wparam|lparam, 0x000e, 1 },
    { WM_COMMAND, sent|wparam|lparam, MAKEWPARAM(1000,1), 0 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'N', 0xc0000001 }, /* XP */
    { WM_KEYUP, wparam|lparam, 'N', 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, 'N', 0xc0000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_CONTROL, 0xc0000001 }, /* XP */
    { WM_KEYUP, wparam|lparam, VK_CONTROL, 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, VK_CONTROL, 0xc0000001 },
    { 0 }
};
static const struct message WmCtrlVkN_2[] = {
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_CONTROL, 1 }, /* XP */
    { WM_KEYDOWN, wparam|lparam, VK_CONTROL, 1 },
    { WM_KEYDOWN, sent|wparam|lparam, VK_CONTROL, 1 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'N', 1 }, /* XP */
    { WM_KEYDOWN, wparam|lparam, 'N', 1 },
    { WM_COMMAND, sent|wparam|lparam, MAKEWPARAM(1000,1), 0 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'N', 0xc0000001 }, /* XP */
    { WM_KEYUP, wparam|lparam, 'N', 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, 'N', 0xc0000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_CONTROL, 0xc0000001 }, /* XP */
    { WM_KEYUP, wparam|lparam, VK_CONTROL, 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, VK_CONTROL, 0xc0000001 },
    { 0 }
};
static const struct message WmAltVkN[] = {
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_MENU, 0x20000001 }, /* XP */
    { WM_SYSKEYDOWN, wparam|lparam, VK_MENU, 0x20000001 },
    { WM_SYSKEYDOWN, sent|wparam|lparam, VK_MENU, 0x20000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'N', 0x20000001 }, /* XP */
    { WM_SYSKEYDOWN, wparam|lparam, 'N', 0x20000001 },
    { WM_SYSKEYDOWN, sent|wparam|lparam, 'N', 0x20000001 },
    { WM_SYSCHAR, wparam|lparam, 'n', 0x20000001 },
    { WM_SYSCHAR, sent|wparam|lparam, 'n', 0x20000001 },
    { WM_SYSCOMMAND, sent|defwinproc|wparam|lparam, SC_KEYMENU, 'n' },
    { HCBT_SYSCOMMAND, hook },
    { WM_ENTERMENULOOP, sent|defwinproc|wparam|lparam, 0, 0 },
    { EVENT_SYSTEM_CAPTURESTART, winevent_hook|wparam|lparam, 0, 0 },
    { 0x00AE, sent|defwinproc|optional }, /* XP */
    { WM_GETTEXT, sent|defwinproc|optional }, /* XP */
    { WM_INITMENU, sent|defwinproc },
    { EVENT_SYSTEM_MENUSTART, winevent_hook|wparam|lparam, OBJID_SYSMENU, 0 },
    { WM_MENUCHAR, sent|defwinproc|wparam, MAKEWPARAM('n',MF_SYSMENU) },
    { EVENT_SYSTEM_CAPTUREEND, winevent_hook|wparam|lparam, 0, 0 },
    { WM_CAPTURECHANGED, sent|defwinproc },
    { WM_MENUSELECT, sent|defwinproc|wparam, MAKEWPARAM(0,0xffff) },
    { EVENT_SYSTEM_MENUEND, winevent_hook|wparam|lparam, OBJID_SYSMENU, 0 },
    { WM_EXITMENULOOP, sent|defwinproc },
    { WM_MENUSELECT, sent|defwinproc|wparam|optional, MAKEWPARAM(0,0xffff) }, /* Win95 bug */
    { WM_EXITMENULOOP, sent|defwinproc|optional }, /* Win95 bug */
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'N', 0xe0000001 }, /* XP */
    { WM_SYSKEYUP, wparam|lparam, 'N', 0xe0000001 },
    { WM_SYSKEYUP, sent|wparam|lparam, 'N', 0xe0000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_MENU, 0xc0000001 }, /* XP */
    { WM_KEYUP, wparam|lparam, VK_MENU, 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, VK_MENU, 0xc0000001 },
    { 0 }
};
static const struct message WmAltVkN_2[] = {
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_MENU, 0x20000001 }, /* XP */
    { WM_SYSKEYDOWN, wparam|lparam, VK_MENU, 0x20000001 },
    { WM_SYSKEYDOWN, sent|wparam|lparam, VK_MENU, 0x20000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'N', 0x20000001 }, /* XP */
    { WM_SYSKEYDOWN, wparam|lparam, 'N', 0x20000001 },
    { WM_COMMAND, sent|wparam|lparam, MAKEWPARAM(1003,1), 0 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'N', 0xe0000001 }, /* XP */
    { WM_SYSKEYUP, wparam|lparam, 'N', 0xe0000001 },
    { WM_SYSKEYUP, sent|wparam|lparam, 'N', 0xe0000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_MENU, 0xc0000001 }, /* XP */
    { WM_KEYUP, wparam|lparam, VK_MENU, 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, VK_MENU, 0xc0000001 },
    { 0 }
};
static const struct message WmCtrlAltVkN[] = {
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_CONTROL, 1 }, /* XP */
    { WM_KEYDOWN, wparam|lparam, VK_CONTROL, 1 },
    { WM_KEYDOWN, sent|wparam|lparam, VK_CONTROL, 1 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_MENU, 0x20000001 }, /* XP */
    { WM_KEYDOWN, wparam|lparam, VK_MENU, 0x20000001 },
    { WM_KEYDOWN, sent|wparam|lparam, VK_MENU, 0x20000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'N', 0x20000001 }, /* XP */
    { WM_KEYDOWN, wparam|lparam, 'N', 0x20000001 },
    { WM_KEYDOWN, sent|wparam|lparam, 'N', 0x20000001 },
    { WM_CHAR, optional },
    { WM_CHAR, sent|optional },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'N', 0xe0000001 }, /* XP */
    { WM_KEYUP, wparam|lparam, 'N', 0xe0000001 },
    { WM_KEYUP, sent|wparam|lparam, 'N', 0xe0000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_MENU, 0xc0000001 }, /* XP */
    { WM_KEYUP, wparam|lparam, VK_MENU, 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, VK_MENU, 0xc0000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_CONTROL, 0xc0000001 }, /* XP */
    { WM_KEYUP, wparam|lparam, VK_CONTROL, 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, VK_CONTROL, 0xc0000001 },
    { 0 }
};
static const struct message WmCtrlShiftVkN[] = {
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_CONTROL, 1 }, /* XP */
    { WM_KEYDOWN, wparam|lparam, VK_CONTROL, 1 },
    { WM_KEYDOWN, sent|wparam|lparam, VK_CONTROL, 1 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_SHIFT, 1 }, /* XP */
    { WM_KEYDOWN, wparam|lparam, VK_SHIFT, 1 },
    { WM_KEYDOWN, sent|wparam|lparam, VK_SHIFT, 1 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'N', 1 }, /* XP */
    { WM_KEYDOWN, wparam|lparam, 'N', 1 },
    { WM_COMMAND, sent|wparam|lparam, MAKEWPARAM(1004,1), 0 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'N', 0xc0000001 }, /* XP */
    { WM_KEYUP, wparam|lparam, 'N', 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, 'N', 0xc0000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_SHIFT, 0xc0000001 }, /* XP */
    { WM_KEYUP, wparam|lparam, VK_SHIFT, 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, VK_SHIFT, 0xc0000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_CONTROL, 0xc0000001 }, /* XP */
    { WM_KEYUP, wparam|lparam, VK_CONTROL, 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, VK_CONTROL, 0xc0000001 },
    { 0 }
};
static const struct message WmCtrlAltShiftVkN[] = {
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_CONTROL, 1 }, /* XP */
    { WM_KEYDOWN, wparam|lparam, VK_CONTROL, 1 },
    { WM_KEYDOWN, sent|wparam|lparam, VK_CONTROL, 1 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_MENU, 0x20000001 }, /* XP */
    { WM_KEYDOWN, wparam|lparam, VK_MENU, 0x20000001 },
    { WM_KEYDOWN, sent|wparam|lparam, VK_MENU, 0x20000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_SHIFT, 0x20000001 }, /* XP */
    { WM_KEYDOWN, wparam|lparam, VK_SHIFT, 0x20000001 },
    { WM_KEYDOWN, sent|wparam|lparam, VK_SHIFT, 0x20000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'N', 0x20000001 }, /* XP */
    { WM_KEYDOWN, wparam|lparam, 'N', 0x20000001 },
    { WM_COMMAND, sent|wparam|lparam, MAKEWPARAM(1005,1), 0 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'N', 0xe0000001 }, /* XP */
    { WM_KEYUP, wparam|lparam, 'N', 0xe0000001 },
    { WM_KEYUP, sent|wparam|lparam, 'N', 0xe0000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_SHIFT, 0xe0000001 }, /* XP */
    { WM_KEYUP, wparam|lparam, VK_SHIFT, 0xe0000001 },
    { WM_KEYUP, sent|wparam|lparam, VK_SHIFT, 0xe0000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_MENU, 0xc0000001 }, /* XP */
    { WM_KEYUP, wparam|lparam, VK_MENU, 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, VK_MENU, 0xc0000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_CONTROL, 0xc0000001 }, /* XP */
    { WM_KEYUP, wparam|lparam, VK_CONTROL, 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, VK_CONTROL, 0xc0000001 },
    { 0 }
};
static const struct message WmAltPressRelease[] = {
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_MENU, 0x20000001 }, /* XP */
    { WM_SYSKEYDOWN, wparam|lparam, VK_MENU, 0x20000001 },
    { WM_SYSKEYDOWN, sent|wparam|lparam, VK_MENU, 0x20000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_MENU, 0xc0000001 }, /* XP */
    { WM_SYSKEYUP, wparam|lparam, VK_MENU, 0xc0000001 },
    { WM_SYSKEYUP, sent|wparam|lparam, VK_MENU, 0xc0000001 },
    { WM_SYSCOMMAND, sent|defwinproc|wparam|lparam, SC_KEYMENU, 0 },
    { HCBT_SYSCOMMAND, hook },
    { WM_ENTERMENULOOP, sent|defwinproc|wparam|lparam, 0, 0 },
    { EVENT_SYSTEM_CAPTURESTART, winevent_hook|wparam|lparam, 0, 0 },
    { WM_INITMENU, sent|defwinproc },
    { EVENT_SYSTEM_MENUSTART, winevent_hook|wparam|lparam, OBJID_SYSMENU, 0 },
    { WM_MENUSELECT, sent|defwinproc|wparam, MAKEWPARAM(0,MF_SYSMENU|MF_POPUP|MF_HILITE) },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_SYSMENU, 1 },

    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_MENU, 0x30000001 }, /* XP */

    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_SYSMENU, 0 },
    { EVENT_SYSTEM_CAPTUREEND, winevent_hook|wparam|lparam, 0, 0, },
    { WM_CAPTURECHANGED, sent|defwinproc },
    { WM_MENUSELECT, sent|defwinproc|wparam|optional, MAKEWPARAM(0,0xffff) },
    { EVENT_SYSTEM_MENUEND, winevent_hook|wparam|lparam, OBJID_SYSMENU, 0 },
    { WM_EXITMENULOOP, sent|defwinproc },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_MENU, 0xc0000001 }, /* XP */
    { WM_SYSKEYUP, wparam|lparam, VK_MENU, 0xc0000001 },
    { WM_SYSKEYUP, sent|wparam|lparam, VK_MENU, 0xc0000001 },
    { 0 }
};
static const struct message WmShiftMouseButton[] = {
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_SHIFT, 1 }, /* XP */
    { WM_KEYDOWN, wparam|lparam, VK_SHIFT, 1 },
    { WM_KEYDOWN, sent|wparam|lparam, VK_SHIFT, 1 },
    { WM_MOUSEMOVE, wparam|optional, 0, 0 },
    { WM_MOUSEMOVE, sent|wparam|optional, 0, 0 },
    { WM_LBUTTONDOWN, wparam, MK_LBUTTON|MK_SHIFT, 0 },
    { WM_LBUTTONDOWN, sent|wparam, MK_LBUTTON|MK_SHIFT, 0 },
    { WM_LBUTTONUP, wparam, MK_SHIFT, 0 },
    { WM_LBUTTONUP, sent|wparam, MK_SHIFT, 0 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_SHIFT, 0xc0000001 }, /* XP */
    { WM_KEYUP, wparam|lparam, VK_SHIFT, 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, VK_SHIFT, 0xc0000001 },
    { 0 }
};
static const struct message WmF1Seq[] = {
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_F1, 1 }, /* XP */
    { WM_KEYDOWN, wparam|lparam, VK_F1, 1 },
    { WM_KEYDOWN, sent|wparam|lparam, VK_F1, 0x00000001 },
    { WM_KEYF1, wparam|lparam, 0, 0 },
    { WM_KEYF1, sent|wparam|lparam, 0, 0 },
    { WM_HELP, sent|defwinproc },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_F1, 0xc0000001 }, /* XP */
    { WM_KEYUP, wparam|lparam, VK_F1, 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, VK_F1, 0xc0000001 },
    { 0 }
};
static const struct message WmVkAppsSeq[] = {
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_APPS, 1 }, /* XP */
    { WM_KEYDOWN, wparam|lparam, VK_APPS, 1 },
    { WM_KEYDOWN, sent|wparam|lparam, VK_APPS, 0x00000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_APPS, 0xc0000001 }, /* XP */
    { WM_KEYUP, wparam|lparam, VK_APPS, 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, VK_APPS, 0xc0000001 },
    { WM_CONTEXTMENU, lparam, /*hwnd*/0, -1 },
    { WM_CONTEXTMENU, sent|lparam, /*hwnd*/0, -1 },
    { 0 }
};
static const struct message WmVkF10Seq[] = {
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_F10, 1 }, /* XP */
    { WM_SYSKEYDOWN, wparam|lparam, VK_F10, 1 },
    { WM_SYSKEYDOWN, sent|wparam|lparam, VK_F10, 0x00000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_F10, 0xc0000001 }, /* XP */
    { WM_SYSKEYUP, wparam|lparam, VK_F10, 0xc0000001 },
    { WM_SYSKEYUP, sent|wparam|lparam, VK_F10, 0xc0000001 },
    { WM_SYSCOMMAND, sent|defwinproc|wparam, SC_KEYMENU },
    { HCBT_SYSCOMMAND, hook },
    { WM_ENTERMENULOOP, sent|defwinproc|wparam|lparam, 0, 0 },
    { EVENT_SYSTEM_CAPTURESTART, winevent_hook|wparam|lparam, 0, 0 },
    { WM_INITMENU, sent|defwinproc },
    { EVENT_SYSTEM_MENUSTART, winevent_hook|wparam|lparam, OBJID_SYSMENU, 0 },
    { WM_MENUSELECT, sent|defwinproc|wparam, MAKEWPARAM(0,MF_SYSMENU|MF_POPUP|MF_HILITE) },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_SYSMENU, 1 },

    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_F10, 0x10000001 }, /* XP */

    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_F10, 1 }, /* XP */
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_SYSMENU, 0 },
    { EVENT_SYSTEM_CAPTUREEND, winevent_hook|wparam|lparam, 0, 0, },
    { WM_CAPTURECHANGED, sent|defwinproc },
    { WM_MENUSELECT, sent|defwinproc|wparam|optional, MAKEWPARAM(0,0xffff) },
    { EVENT_SYSTEM_MENUEND, winevent_hook|wparam|lparam, OBJID_SYSMENU, 0 },
    { WM_EXITMENULOOP, sent|defwinproc },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_F10, 0xc0000001 }, /* XP */
    { WM_SYSKEYUP, wparam|lparam, VK_F10, 0xc0000001 },
    { WM_SYSKEYUP, sent|wparam|lparam, VK_F10, 0xc0000001 },
    { 0 }
};
static const struct message WmShiftF10Seq[] = {
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_SHIFT, 1 }, /* XP */
    { WM_KEYDOWN, wparam|lparam, VK_SHIFT, 1 },
    { WM_KEYDOWN, sent|wparam|lparam, VK_SHIFT, 0x00000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_F10, 1 }, /* XP */
    { WM_SYSKEYDOWN, wparam|lparam, VK_F10, 1 },
    { WM_SYSKEYDOWN, sent|wparam|lparam, VK_F10, 0x00000001 },
    { WM_CONTEXTMENU, sent|defwinproc|lparam, /*hwnd*/0, -1 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_F10, 0xc0000001 }, /* XP */
    { WM_SYSKEYUP, wparam|lparam, VK_F10, 0xc0000001 },
    { WM_SYSKEYUP, sent|wparam|lparam, VK_F10, 0xc0000001 },
    { WM_SYSCOMMAND, sent|defwinproc|wparam, SC_KEYMENU },
    { HCBT_SYSCOMMAND, hook },
    { WM_ENTERMENULOOP, sent|defwinproc|wparam|lparam, 0, 0 },
    { WM_INITMENU, sent|defwinproc },
    { WM_MENUSELECT, sent|defwinproc|wparam, MAKEWPARAM(0,MF_SYSMENU|MF_POPUP|MF_HILITE) },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_SHIFT, 0xd0000001 }, /* XP */
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_ESCAPE, 0x10000001 }, /* XP */
    { WM_CAPTURECHANGED, sent|defwinproc|wparam|lparam, 0, 0 },
    { WM_MENUSELECT, sent|defwinproc|wparam|lparam, 0xffff0000, 0 },
    { WM_EXITMENULOOP, sent|defwinproc|wparam|lparam, 0, 0 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_ESCAPE, 0xc0000001 }, /* XP */
    { WM_KEYUP, wparam|lparam, VK_ESCAPE, 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, VK_ESCAPE, 0xc0000001 },
    { 0 }
};

static void pump_msg_loop(HWND hwnd, HACCEL hAccel)
{
    MSG msg;

    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
    {
        struct recvd_message log_msg;

        /* ignore some unwanted messages */
        if (msg.message == WM_MOUSEMOVE ||
            msg.message == WM_TIMER ||
            ignore_message( msg.message ))
            continue;

        log_msg.hwnd = msg.hwnd;
        log_msg.message = msg.message;
        log_msg.flags = wparam|lparam;
        log_msg.wParam = msg.wParam;
        log_msg.lParam = msg.lParam;
        log_msg.descr = "accel";
        add_message(&log_msg);

        if (!hAccel || !TranslateAcceleratorA(hwnd, hAccel, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }
}

static void test_accelerators(void)
{
    RECT rc;
    POINT pt;
    SHORT state;
    HACCEL hAccel;
    HWND hwnd = CreateWindowExA(0, "TestWindowClass", NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                100, 100, 200, 200, 0, 0, 0, NULL);
    BOOL ret;

    assert(hwnd != 0);
    UpdateWindow(hwnd);
    flush_events();
    flush_sequence();

    SetFocus(hwnd);
    ok(GetFocus() == hwnd, "wrong focus window %p\n", GetFocus());

    state = GetKeyState(VK_SHIFT);
    ok(!(state & 0x8000), "wrong Shift state %04x\n", state);
    state = GetKeyState(VK_CAPITAL);
    ok(state == 0, "wrong CapsLock state %04x\n", state);

    hAccel = LoadAcceleratorsA(GetModuleHandleA(NULL), MAKEINTRESOURCEA(1));
    assert(hAccel != 0);

    flush_events();
    pump_msg_loop(hwnd, 0);
    flush_sequence();

    trace("testing VK_N press/release\n");
    flush_sequence();
    keybd_event('N', 0, 0, 0);
    keybd_event('N', 0, KEYEVENTF_KEYUP, 0);
    pump_msg_loop(hwnd, hAccel);
    if (!sequence_cnt)  /* we didn't get any message */
    {
        skip( "queuing key events not supported\n" );
        goto done;
    }
    ok_sequence(WmVkN, "VK_N press/release", FALSE);

    trace("testing Shift+VK_N press/release\n");
    flush_sequence();
    keybd_event(VK_SHIFT, 0, 0, 0);
    keybd_event('N', 0, 0, 0);
    keybd_event('N', 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
    pump_msg_loop(hwnd, hAccel);
    ok_sequence(WmShiftVkN, "Shift+VK_N press/release", FALSE);

    trace("testing Ctrl+VK_N press/release\n");
    flush_sequence();
    keybd_event(VK_CONTROL, 0, 0, 0);
    keybd_event('N', 0, 0, 0);
    keybd_event('N', 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
    pump_msg_loop(hwnd, hAccel);
    ok_sequence(WmCtrlVkN, "Ctrl+VK_N press/release", FALSE);

    trace("testing Alt+VK_N press/release\n");
    flush_sequence();
    keybd_event(VK_MENU, 0, 0, 0);
    keybd_event('N', 0, 0, 0);
    keybd_event('N', 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
    pump_msg_loop(hwnd, hAccel);
    ok_sequence(WmAltVkN, "Alt+VK_N press/release", FALSE);

    trace("testing Ctrl+Alt+VK_N press/release 1\n");
    flush_sequence();
    keybd_event(VK_CONTROL, 0, 0, 0);
    keybd_event(VK_MENU, 0, 0, 0);
    keybd_event('N', 0, 0, 0);
    keybd_event('N', 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
    pump_msg_loop(hwnd, hAccel);
    ok_sequence(WmCtrlAltVkN, "Ctrl+Alt+VK_N press/release 1", FALSE);

    ret = DestroyAcceleratorTable(hAccel);
    ok( ret, "DestroyAcceleratorTable error %d\n", GetLastError());

    hAccel = LoadAcceleratorsA(GetModuleHandleA(NULL), MAKEINTRESOURCEA(2));
    assert(hAccel != 0);

    trace("testing VK_N press/release\n");
    flush_sequence();
    keybd_event('N', 0, 0, 0);
    keybd_event('N', 0, KEYEVENTF_KEYUP, 0);
    pump_msg_loop(hwnd, hAccel);
    ok_sequence(WmVkN, "VK_N press/release", FALSE);

    trace("testing Shift+VK_N press/release\n");
    flush_sequence();
    keybd_event(VK_SHIFT, 0, 0, 0);
    keybd_event('N', 0, 0, 0);
    keybd_event('N', 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
    pump_msg_loop(hwnd, hAccel);
    ok_sequence(WmShiftVkN, "Shift+VK_N press/release", FALSE);

    trace("testing Ctrl+VK_N press/release 2\n");
    flush_sequence();
    keybd_event(VK_CONTROL, 0, 0, 0);
    keybd_event('N', 0, 0, 0);
    keybd_event('N', 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
    pump_msg_loop(hwnd, hAccel);
    ok_sequence(WmCtrlVkN_2, "Ctrl+VK_N press/release 2", FALSE);

    trace("testing Alt+VK_N press/release 2\n");
    flush_sequence();
    keybd_event(VK_MENU, 0, 0, 0);
    keybd_event('N', 0, 0, 0);
    keybd_event('N', 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
    pump_msg_loop(hwnd, hAccel);
    ok_sequence(WmAltVkN_2, "Alt+VK_N press/release 2", FALSE);

    trace("testing Ctrl+Alt+VK_N press/release 2\n");
    flush_sequence();
    keybd_event(VK_CONTROL, 0, 0, 0);
    keybd_event(VK_MENU, 0, 0, 0);
    keybd_event('N', 0, 0, 0);
    keybd_event('N', 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
    pump_msg_loop(hwnd, hAccel);
    ok_sequence(WmCtrlAltVkN, "Ctrl+Alt+VK_N press/release 2", FALSE);

    trace("testing Ctrl+Shift+VK_N press/release\n");
    flush_sequence();
    keybd_event(VK_CONTROL, 0, 0, 0);
    keybd_event(VK_SHIFT, 0, 0, 0);
    keybd_event('N', 0, 0, 0);
    keybd_event('N', 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
    pump_msg_loop(hwnd, hAccel);
    ok_sequence(WmCtrlShiftVkN, "Ctrl+Shift+VK_N press/release", FALSE);

    trace("testing Ctrl+Alt+Shift+VK_N press/release\n");
    flush_sequence();
    keybd_event(VK_CONTROL, 0, 0, 0);
    keybd_event(VK_MENU, 0, 0, 0);
    keybd_event(VK_SHIFT, 0, 0, 0);
    keybd_event('N', 0, 0, 0);
    keybd_event('N', 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
    pump_msg_loop(hwnd, hAccel);
    ok_sequence(WmCtrlAltShiftVkN, "Ctrl+Alt+Shift+VK_N press/release", FALSE);

    ret = DestroyAcceleratorTable(hAccel);
    ok( ret, "DestroyAcceleratorTable error %d\n", GetLastError());
    hAccel = 0;

    trace("testing Alt press/release\n");
    flush_sequence();
    keybd_event(VK_MENU, 0, 0, 0);
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_MENU, 0, 0, 0);
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
    pump_msg_loop(hwnd, 0);
    /* this test doesn't pass in Wine for managed windows */
    ok_sequence(WmAltPressRelease, "Alt press/release", TRUE);

    trace("testing VK_F1 press/release\n");
    keybd_event(VK_F1, 0, 0, 0);
    keybd_event(VK_F1, 0, KEYEVENTF_KEYUP, 0);
    pump_msg_loop(hwnd, 0);
    ok_sequence(WmF1Seq, "F1 press/release", FALSE);

    trace("testing VK_APPS press/release\n");
    keybd_event(VK_APPS, 0, 0, 0);
    keybd_event(VK_APPS, 0, KEYEVENTF_KEYUP, 0);
    pump_msg_loop(hwnd, 0);
    ok_sequence(WmVkAppsSeq, "VK_APPS press/release", FALSE);

    trace("testing VK_F10 press/release\n");
    keybd_event(VK_F10, 0, 0, 0);
    keybd_event(VK_F10, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_F10, 0, 0, 0);
    keybd_event(VK_F10, 0, KEYEVENTF_KEYUP, 0);
    pump_msg_loop(hwnd, 0);
    ok_sequence(WmVkF10Seq, "VK_F10 press/release", TRUE);

    trace("testing SHIFT+F10 press/release\n");
    keybd_event(VK_SHIFT, 0, 0, 0);
    keybd_event(VK_F10, 0, 0, 0);
    keybd_event(VK_F10, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_ESCAPE, 0, 0, 0);
    keybd_event(VK_ESCAPE, 0, KEYEVENTF_KEYUP, 0);
    pump_msg_loop(hwnd, 0);
    ok_sequence(WmShiftF10Seq, "SHIFT+F10 press/release", TRUE);

    trace("testing Shift+MouseButton press/release\n");
    /* first, move mouse pointer inside of the window client area */
    GetClientRect(hwnd, &rc);
    MapWindowPoints(hwnd, 0, (LPPOINT)&rc, 2);
    rc.left += (rc.right - rc.left)/2;
    rc.top += (rc.bottom - rc.top)/2;
    SetCursorPos(rc.left, rc.top);
    SetActiveWindow(hwnd);

    flush_events();
    flush_sequence();
    GetCursorPos(&pt);
    if (pt.x == rc.left && pt.y == rc.top)
    {
        int i;
        keybd_event(VK_SHIFT, 0, 0, 0);
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
        pump_msg_loop(hwnd, 0);
        for (i = 0; i < sequence_cnt; i++) if (sequence[i].message == WM_LBUTTONDOWN) break;
        if (i < sequence_cnt)
            ok_sequence(WmShiftMouseButton, "Shift+MouseButton press/release", FALSE);
        else
            skip( "Shift+MouseButton event didn't get to the window\n" );
    }

done:
    if (hAccel) DestroyAcceleratorTable(hAccel);
    DestroyWindow(hwnd);
}

/************* window procedures ********************/

static LRESULT MsgCheckProc (BOOL unicode, HWND hwnd, UINT message, 
			     WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    static LONG beginpaint_counter = 0;
    LRESULT ret;
    struct recvd_message msg;

    if (ignore_message( message )) return 0;

    switch (message)
    {
	case WM_ENABLE:
	{
	    LONG style = GetWindowLongA(hwnd, GWL_STYLE);
	    ok((BOOL)wParam == !(style & WS_DISABLED),
		"wrong WS_DISABLED state: %ld != %d\n", wParam, !(style & WS_DISABLED));
	    break;
	}

	case WM_CAPTURECHANGED:
	    if (test_DestroyWindow_flag)
	    {
		DWORD style = GetWindowLongA(hwnd, GWL_STYLE);
		if (style & WS_CHILD)
		    lParam = GetWindowLongPtrA(hwnd, GWLP_ID);
		else if (style & WS_POPUP)
		    lParam = WND_POPUP_ID;
		else
		    lParam = WND_PARENT_ID;
	    }
	    break;

	case WM_NCDESTROY:
	{
	    HWND capture;

	    ok(!GetWindow(hwnd, GW_CHILD), "children should be unlinked at this point\n");
	    capture = GetCapture();
	    if (capture)
	    {
		ok(capture == hwnd, "capture should NOT be released at this point (capture %p)\n", capture);
		trace("current capture %p, releasing...\n", capture);
		ReleaseCapture();
	    }
	}
	/* fall through */
	case WM_DESTROY:
            if (pGetAncestor)
	        ok(pGetAncestor(hwnd, GA_PARENT) != 0, "parent should NOT be unlinked at this point\n");
	    if (test_DestroyWindow_flag)
	    {
		DWORD style = GetWindowLongA(hwnd, GWL_STYLE);
		if (style & WS_CHILD)
		    lParam = GetWindowLongPtrA(hwnd, GWLP_ID);
		else if (style & WS_POPUP)
		    lParam = WND_POPUP_ID;
		else
		    lParam = WND_PARENT_ID;
	    }
	    break;

	/* test_accelerators() depends on this */
	case WM_NCHITTEST:
	    return HTCLIENT;

	case WM_USER+10:
	{
	    HANDLE handle, event = (HANDLE)lParam;
	    BOOL ret;

	    handle = (void*)0xdeadbeef;
	    ret = pGetCurrentActCtx(&handle);
	    ok(ret, "failed to get current context, %u\n", GetLastError());
	    ok(handle == 0, "got active context %p\n", handle);
	    if (event) SetEvent(event);
	    return 1;
	}

	/* ignore */
	case WM_MOUSEMOVE:
	case WM_MOUSEACTIVATE:
	case WM_NCMOUSEMOVE:
	case WM_SETCURSOR:
	case WM_IME_SELECT:
	    return 0;
    }

    msg.hwnd = hwnd;
    msg.message = message;
    msg.flags = sent|wparam|lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    if (beginpaint_counter) msg.flags |= beginpaint;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.descr = "MsgCheckProc";
    add_message(&msg);

    if (message == WM_GETMINMAXINFO && (GetWindowLongA(hwnd, GWL_STYLE) & WS_CHILD))
    {
	HWND parent = GetParent(hwnd);
	RECT rc;
	MINMAXINFO *minmax = (MINMAXINFO *)lParam;

	GetClientRect(parent, &rc);
	trace("parent %p client size = (%d x %d)\n", parent, rc.right, rc.bottom);
        trace("Reserved=%d,%d MaxSize=%d,%d MaxPos=%d,%d MinTrack=%d,%d MaxTrack=%d,%d\n",
              minmax->ptReserved.x, minmax->ptReserved.y,
              minmax->ptMaxSize.x, minmax->ptMaxSize.y,
              minmax->ptMaxPosition.x, minmax->ptMaxPosition.y,
              minmax->ptMinTrackSize.x, minmax->ptMinTrackSize.y,
              minmax->ptMaxTrackSize.x, minmax->ptMaxTrackSize.y);

	ok(minmax->ptMaxSize.x == rc.right, "default width of maximized child %d != %d\n",
	   minmax->ptMaxSize.x, rc.right);
	ok(minmax->ptMaxSize.y == rc.bottom, "default height of maximized child %d != %d\n",
	   minmax->ptMaxSize.y, rc.bottom);
    }

    if (message == WM_PAINT)
    {
        PAINTSTRUCT ps;
        beginpaint_counter++;
        BeginPaint( hwnd, &ps );
        beginpaint_counter--;
        EndPaint( hwnd, &ps );
        return 0;
    }

    defwndproc_counter++;
    ret = unicode ? DefWindowProcW(hwnd, message, wParam, lParam) 
		  : DefWindowProcA(hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static LRESULT WINAPI MsgCheckProcA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return MsgCheckProc (FALSE, hwnd, message, wParam, lParam);
}

static LRESULT WINAPI MsgCheckProcW(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return MsgCheckProc (TRUE, hwnd, message, wParam, lParam);
}

static LRESULT WINAPI PopupMsgCheckProcA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct recvd_message msg;

    if (ignore_message( message )) return 0;

    switch (message)
    {
    case WM_QUERYENDSESSION:
    case WM_ENDSESSION:
        lParam &= ~0x01;  /* Vista adds a 0x01 flag */
        break;
    }

    msg.hwnd = hwnd;
    msg.message = message;
    msg.flags = sent|wparam|lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.descr = "popup";
    add_message(&msg);

    if (message == WM_CREATE)
    {
	DWORD style = GetWindowLongA(hwnd, GWL_STYLE) | WS_VISIBLE;
	SetWindowLongA(hwnd, GWL_STYLE, style);
    }

    defwndproc_counter++;
    ret = DefWindowProcA(hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static LRESULT WINAPI ParentMsgCheckProcA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    static LONG beginpaint_counter = 0;
    LRESULT ret;
    struct recvd_message msg;

    if (ignore_message( message )) return 0;

    if (log_all_parent_messages ||
        message == WM_PARENTNOTIFY || message == WM_CANCELMODE ||
	message == WM_SETFOCUS || message == WM_KILLFOCUS ||
	message == WM_ENABLE ||	message == WM_ENTERIDLE ||
	message == WM_DRAWITEM || message == WM_COMMAND ||
	message == WM_IME_SETCONTEXT)
    {
        switch (message)
        {
            /* ignore */
            case WM_NCHITTEST:
                return HTCLIENT;
            case WM_SETCURSOR:
            case WM_MOUSEMOVE:
            case WM_NCMOUSEMOVE:
                return 0;

            case WM_ERASEBKGND:
            {
                RECT rc;
                INT ret = GetClipBox((HDC)wParam, &rc);

                trace("WM_ERASEBKGND: GetClipBox()=%d, (%d,%d-%d,%d)\n",
                       ret, rc.left, rc.top, rc.right, rc.bottom);
                break;
            }
        }

        msg.hwnd = hwnd;
        msg.message = message;
        msg.flags = sent|parent|wparam|lparam;
        if (defwndproc_counter) msg.flags |= defwinproc;
        if (beginpaint_counter) msg.flags |= beginpaint;
        msg.wParam = wParam;
        msg.lParam = lParam;
        msg.descr = "parent";
        add_message(&msg);
    }

    if (message == WM_PAINT)
    {
        PAINTSTRUCT ps;
        beginpaint_counter++;
        BeginPaint( hwnd, &ps );
        beginpaint_counter--;
        EndPaint( hwnd, &ps );
        return 0;
    }

    defwndproc_counter++;
    ret = DefWindowProcA(hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static INT_PTR CALLBACK StopQuitMsgCheckProcA(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
{
    if (message == WM_CREATE)
        PostMessageA(hwnd, WM_CLOSE, 0, 0);
    else if (message == WM_CLOSE)
    {
        /* Only the first WM_QUIT will survive the window destruction */
        PostMessageA(hwnd, WM_USER, 0x1234, 0x5678);
        PostMessageA(hwnd, WM_QUIT, 0x1234, 0x5678);
        PostMessageA(hwnd, WM_QUIT, 0x4321, 0x8765);
    }

    return DefWindowProcA(hwnd, message, wp, lp);
}

static LRESULT WINAPI TestDlgProcA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct recvd_message msg;

    if (ignore_message( message )) return 0;

    if (test_def_id)
    {
        DefDlgProcA(hwnd, DM_SETDEFID, 1, 0);
        ret = DefDlgProcA(hwnd, DM_GETDEFID, 0, 0);
        if (after_end_dialog)
            ok( ret == 0, "DM_GETDEFID should return 0 after EndDialog, got %lx\n", ret );
        else
            ok(HIWORD(ret) == DC_HASDEFID, "DM_GETDEFID should return DC_HASDEFID, got %lx\n", ret);
    }

    msg.hwnd = hwnd;
    msg.message = message;
    msg.flags = sent|wparam|lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.descr = "dialog";
    add_message(&msg);

    defwndproc_counter++;
    ret = DefDlgProcA(hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static LRESULT WINAPI ShowWindowProcA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct recvd_message msg;

    /* log only specific messages we are interested in */
    switch (message)
    {
#if 0 /* probably log these as well */
    case WM_ACTIVATE:
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
#endif
    case WM_SHOWWINDOW:
    case WM_SIZE:
    case WM_MOVE:
    case WM_GETMINMAXINFO:
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        break;

    default: /* ignore */
        /*trace("showwindow: %p, %04x, %08x, %08lx\n", hwnd, message, wParam, lParam);*/
        return DefWindowProcA(hwnd, message, wParam, lParam);
    }

    msg.hwnd = hwnd;
    msg.message = message;
    msg.flags = sent|wparam|lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.descr = "show";
    add_message(&msg);

    defwndproc_counter++;
    ret = DefWindowProcA(hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static LRESULT WINAPI PaintLoopProcA(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE: return 0;
        case WM_PAINT:
        {
            MSG msg2;
            static int i = 0;

            if (i < 256)
            {
                i++;
                if (PeekMessageA(&msg2, 0, 0, 0, 1))
                {
                    TranslateMessage(&msg2);
                    DispatchMessageA(&msg2);
                }
                i--;
            }
            else ok(broken(1), "infinite loop\n");
            if ( i == 0)
                paint_loop_done = TRUE;
            return DefWindowProcA(hWnd,msg,wParam,lParam);
        }
    }
    return DefWindowProcA(hWnd,msg,wParam,lParam);
}

static LRESULT WINAPI HotkeyMsgCheckProcA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct recvd_message msg;
    DWORD queue_status;

    if (ignore_message( message )) return 0;

    if ((message >= WM_KEYFIRST && message <= WM_KEYLAST) ||
        message == WM_HOTKEY || message >= WM_APP)
    {
        msg.hwnd = hwnd;
        msg.message = message;
        msg.flags = sent|wparam|lparam;
        if (defwndproc_counter) msg.flags |= defwinproc;
        msg.wParam = wParam;
        msg.lParam = lParam;
        msg.descr = "HotkeyMsgCheckProcA";
        add_message(&msg);
    }

    defwndproc_counter++;
    ret = DefWindowProcA(hwnd, message, wParam, lParam);
    defwndproc_counter--;

    if (message == WM_APP)
    {
        queue_status = GetQueueStatus(QS_HOTKEY);
        ok((queue_status & (QS_HOTKEY << 16)) == QS_HOTKEY << 16, "expected QS_HOTKEY << 16 set, got %x\n", queue_status);
        queue_status = GetQueueStatus(QS_POSTMESSAGE);
        ok((queue_status & (QS_POSTMESSAGE << 16)) == QS_POSTMESSAGE << 16, "expected QS_POSTMESSAGE << 16 set, got %x\n", queue_status);
        PostMessageA(hwnd, WM_APP+1, 0, 0);
    }
    else if (message == WM_APP+1)
    {
        queue_status = GetQueueStatus(QS_HOTKEY);
        ok((queue_status & (QS_HOTKEY << 16)) == 0, "expected QS_HOTKEY << 16 cleared, got %x\n", queue_status);
    }

    return ret;
}

static BOOL RegisterWindowClasses(void)
{
    WNDCLASSA cls;
    WNDCLASSW clsW;

    cls.style = 0;
    cls.lpfnWndProc = MsgCheckProcA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "TestWindowClass";
    if(!RegisterClassA(&cls)) return FALSE;

    cls.lpfnWndProc = HotkeyMsgCheckProcA;
    cls.lpszClassName = "HotkeyWindowClass";
    if(!RegisterClassA(&cls)) return FALSE;

    cls.lpfnWndProc = ShowWindowProcA;
    cls.lpszClassName = "ShowWindowClass";
    if(!RegisterClassA(&cls)) return FALSE;

    cls.lpfnWndProc = PopupMsgCheckProcA;
    cls.lpszClassName = "TestPopupClass";
    if(!RegisterClassA(&cls)) return FALSE;

    cls.lpfnWndProc = ParentMsgCheckProcA;
    cls.lpszClassName = "TestParentClass";
    if(!RegisterClassA(&cls)) return FALSE;

    cls.lpfnWndProc = StopQuitMsgCheckProcA;
    cls.lpszClassName = "StopQuitClass";
    if(!RegisterClassA(&cls)) return FALSE;

    cls.lpfnWndProc = DefWindowProcA;
    cls.lpszClassName = "SimpleWindowClass";
    if(!RegisterClassA(&cls)) return FALSE;

    cls.lpfnWndProc = PaintLoopProcA;
    cls.lpszClassName = "PaintLoopWindowClass";
    if(!RegisterClassA(&cls)) return FALSE;

    cls.style = CS_NOCLOSE;
    cls.lpszClassName = "NoCloseWindowClass";
    if(!RegisterClassA(&cls)) return FALSE;

    ok(GetClassInfoA(0, "#32770", &cls), "GetClassInfo failed\n");
    cls.style = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hbrBackground = 0;
    cls.lpfnWndProc = TestDlgProcA;
    cls.lpszClassName = "TestDialogClass";
    if(!RegisterClassA(&cls)) return FALSE;

    clsW.style = 0;
    clsW.lpfnWndProc = MsgCheckProcW;
    clsW.cbClsExtra = 0;
    clsW.cbWndExtra = 0;
    clsW.hInstance = GetModuleHandleW(0);
    clsW.hIcon = 0;
    clsW.hCursor = LoadCursorW(0, (LPWSTR)IDC_ARROW);
    clsW.hbrBackground = GetStockObject(WHITE_BRUSH);
    clsW.lpszMenuName = NULL;
    clsW.lpszClassName = testWindowClassW;
    RegisterClassW(&clsW);  /* ignore error, this fails on Win9x */

    return TRUE;
}

static BOOL is_our_logged_class(HWND hwnd)
{
    char buf[256];

    if (GetClassNameA(hwnd, buf, sizeof(buf)))
    {
	if (!lstrcmpiA(buf, "TestWindowClass") ||
	    !lstrcmpiA(buf, "ShowWindowClass") ||
	    !lstrcmpiA(buf, "TestParentClass") ||
	    !lstrcmpiA(buf, "TestPopupClass") ||
	    !lstrcmpiA(buf, "SimpleWindowClass") ||
	    !lstrcmpiA(buf, "TestDialogClass") ||
	    !lstrcmpiA(buf, "MDI_frame_class") ||
	    !lstrcmpiA(buf, "MDI_client_class") ||
	    !lstrcmpiA(buf, "MDI_child_class") ||
	    !lstrcmpiA(buf, "my_button_class") ||
	    !lstrcmpiA(buf, "my_edit_class") ||
	    !lstrcmpiA(buf, "static") ||
	    !lstrcmpiA(buf, "ListBox") ||
	    !lstrcmpiA(buf, "ComboBox") ||
	    !lstrcmpiA(buf, "MyDialogClass") ||
	    !lstrcmpiA(buf, "#32770") ||
	    !lstrcmpiA(buf, "#32768"))
        return TRUE;
    }
    return FALSE;
}

static LRESULT CALLBACK cbt_hook_proc(int nCode, WPARAM wParam, LPARAM lParam) 
{ 
    HWND hwnd;

    ok(cbt_hook_thread_id == GetCurrentThreadId(), "we didn't ask for events from other threads\n");

    if (nCode == HCBT_CLICKSKIPPED)
    {
        /* ignore this event, XP sends it a lot when switching focus between windows */
	return CallNextHookEx(hCBT_hook, nCode, wParam, lParam);
    }

    if (nCode == HCBT_SYSCOMMAND || nCode == HCBT_KEYSKIPPED)
    {
	struct recvd_message msg;

        msg.hwnd = 0;
	msg.message = nCode;
	msg.flags = hook|wparam|lparam;
	msg.wParam = wParam;
	msg.lParam = lParam;
        msg.descr = "CBT";
	add_message(&msg);

	return CallNextHookEx(hCBT_hook, nCode, wParam, lParam);
    }

    if (nCode == HCBT_DESTROYWND)
    {
	if (test_DestroyWindow_flag)
	{
	    DWORD style = GetWindowLongA((HWND)wParam, GWL_STYLE);
	    if (style & WS_CHILD)
		lParam = GetWindowLongPtrA((HWND)wParam, GWLP_ID);
	    else if (style & WS_POPUP)
		lParam = WND_POPUP_ID;
	    else
		lParam = WND_PARENT_ID;
	}
    }

    /* Log also SetFocus(0) calls */
    hwnd = wParam ? (HWND)wParam : (HWND)lParam;

    if (is_our_logged_class(hwnd))
    {
        struct recvd_message msg;

        msg.hwnd = hwnd;
        msg.message = nCode;
        msg.flags = hook|wparam|lparam;
        msg.wParam = wParam;
        msg.lParam = lParam;
        msg.descr = "CBT";
        add_message(&msg);
    }
    return CallNextHookEx(hCBT_hook, nCode, wParam, lParam);
}

static void CALLBACK win_event_proc(HWINEVENTHOOK hevent,
				    DWORD event,
				    HWND hwnd,
				    LONG object_id,
				    LONG child_id,
				    DWORD thread_id,
				    DWORD event_time)
{
    ok(thread_id == GetCurrentThreadId(), "we didn't ask for events from other threads\n");

    /* ignore mouse cursor events */
    if (object_id == OBJID_CURSOR) return;

    if (!hwnd || is_our_logged_class(hwnd))
    {
        struct recvd_message msg;

        msg.hwnd = hwnd;
        msg.message = event;
        msg.flags = winevent_hook|wparam|lparam;
        msg.wParam = object_id;
        msg.lParam = child_id;
        msg.descr = "WEH";
        add_message(&msg);
    }
}

static const WCHAR wszUnicode[] = {'U','n','i','c','o','d','e',0};
static const WCHAR wszAnsi[] = {'U',0};

static LRESULT CALLBACK MsgConversionProcW(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case CB_FINDSTRINGEXACT:
        trace("String: %p\n", (LPCWSTR)lParam);
        if (!lstrcmpW((LPCWSTR)lParam, wszUnicode))
            return 1;
        if (!lstrcmpW((LPCWSTR)lParam, wszAnsi))
            return 0;
        return -1;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

static const struct message WmGetTextLengthAfromW[] = {
    { WM_GETTEXTLENGTH, sent },
    { WM_GETTEXT, sent|optional },
    { 0 }
};

static const WCHAR dummy_window_text[] = {'d','u','m','m','y',' ','t','e','x','t',0};

/* dummy window proc for WM_GETTEXTLENGTH test */
static LRESULT CALLBACK get_text_len_proc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    switch(msg)
    {
    case WM_GETTEXTLENGTH:
        return lstrlenW(dummy_window_text) + 37;  /* some random length */
    case WM_GETTEXT:
        lstrcpynW( (LPWSTR)lp, dummy_window_text, wp );
        return lstrlenW( (LPWSTR)lp );
    default:
        return DefWindowProcW( hwnd, msg, wp, lp );
    }
}

static void test_message_conversion(void)
{
    static const WCHAR wszMsgConversionClass[] =
        {'M','s','g','C','o','n','v','e','r','s','i','o','n','C','l','a','s','s',0};
    WNDCLASSW cls;
    LRESULT lRes;
    HWND hwnd;
    WNDPROC wndproc, newproc;
    BOOL ret;

    cls.style = 0;
    cls.lpfnWndProc = MsgConversionProcW;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleW(NULL);
    cls.hIcon = NULL;
    cls.hCursor = LoadCursorW(NULL, (LPWSTR)IDC_ARROW);
    cls.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = wszMsgConversionClass;
    /* this call will fail on Win9x, but that doesn't matter as this test is
     * meaningless on those platforms */
    if(!RegisterClassW(&cls)) return;

    hwnd = CreateWindowExW(0, wszMsgConversionClass, NULL, WS_OVERLAPPED,
                           100, 100, 200, 200, 0, 0, 0, NULL);
    ok(hwnd != NULL, "Window creation failed\n");

    /* {W, A} -> A */

    wndproc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_WNDPROC);
    lRes = CallWindowProcA(wndproc, hwnd, CB_FINDSTRINGEXACT, 0, (LPARAM)wszUnicode);
    ok(lRes == 0, "String should have been converted\n");
    lRes = CallWindowProcW(wndproc, hwnd, CB_FINDSTRINGEXACT, 0, (LPARAM)wszUnicode);
    ok(lRes == 1, "String shouldn't have been converted\n");

    /* {W, A} -> W */

    wndproc = (WNDPROC)GetWindowLongPtrW(hwnd, GWLP_WNDPROC);
    lRes = CallWindowProcA(wndproc, hwnd, CB_FINDSTRINGEXACT, 0, (LPARAM)wszUnicode);
    ok(lRes == 1, "String shouldn't have been converted\n");
    lRes = CallWindowProcW(wndproc, hwnd, CB_FINDSTRINGEXACT, 0, (LPARAM)wszUnicode);
    ok(lRes == 1, "String shouldn't have been converted\n");

    /* Synchronous messages */

    lRes = SendMessageA(hwnd, CB_FINDSTRINGEXACT, 0, (LPARAM)wszUnicode);
    ok(lRes == 0, "String should have been converted\n");
    lRes = SendMessageW(hwnd, CB_FINDSTRINGEXACT, 0, (LPARAM)wszUnicode);
    ok(lRes == 1, "String shouldn't have been converted\n");

    /* Asynchronous messages */

    SetLastError(0);
    lRes = PostMessageA(hwnd, CB_FINDSTRINGEXACT, 0, (LPARAM)wszUnicode);
    ok(lRes == 0 && (GetLastError() == ERROR_MESSAGE_SYNC_ONLY || GetLastError() == ERROR_INVALID_PARAMETER),
        "PostMessage on sync only message returned %ld, last error %d\n", lRes, GetLastError());
    SetLastError(0);
    lRes = PostMessageW(hwnd, CB_FINDSTRINGEXACT, 0, (LPARAM)wszUnicode);
    ok(lRes == 0 && (GetLastError() == ERROR_MESSAGE_SYNC_ONLY || GetLastError() == ERROR_INVALID_PARAMETER),
        "PostMessage on sync only message returned %ld, last error %d\n", lRes, GetLastError());
    SetLastError(0);
    lRes = PostThreadMessageA(GetCurrentThreadId(), CB_FINDSTRINGEXACT, 0, (LPARAM)wszUnicode);
    ok(lRes == 0 && (GetLastError() == ERROR_MESSAGE_SYNC_ONLY || GetLastError() == ERROR_INVALID_PARAMETER),
        "PosThreadtMessage on sync only message returned %ld, last error %d\n", lRes, GetLastError());
    SetLastError(0);
    lRes = PostThreadMessageW(GetCurrentThreadId(), CB_FINDSTRINGEXACT, 0, (LPARAM)wszUnicode);
    ok(lRes == 0 && (GetLastError() == ERROR_MESSAGE_SYNC_ONLY || GetLastError() == ERROR_INVALID_PARAMETER),
        "PosThreadtMessage on sync only message returned %ld, last error %d\n", lRes, GetLastError());
    SetLastError(0);
    lRes = SendNotifyMessageA(hwnd, CB_FINDSTRINGEXACT, 0, (LPARAM)wszUnicode);
    ok(lRes == 0 && (GetLastError() == ERROR_MESSAGE_SYNC_ONLY || GetLastError() == ERROR_INVALID_PARAMETER),
        "SendNotifyMessage on sync only message returned %ld, last error %d\n", lRes, GetLastError());
    SetLastError(0);
    lRes = SendNotifyMessageW(hwnd, CB_FINDSTRINGEXACT, 0, (LPARAM)wszUnicode);
    ok(lRes == 0 && (GetLastError() == ERROR_MESSAGE_SYNC_ONLY || GetLastError() == ERROR_INVALID_PARAMETER),
        "SendNotifyMessage on sync only message returned %ld, last error %d\n", lRes, GetLastError());
    SetLastError(0);
    lRes = SendMessageCallbackA(hwnd, CB_FINDSTRINGEXACT, 0, (LPARAM)wszUnicode, NULL, 0);
    ok(lRes == 0 && (GetLastError() == ERROR_MESSAGE_SYNC_ONLY || GetLastError() == ERROR_INVALID_PARAMETER),
        "SendMessageCallback on sync only message returned %ld, last error %d\n", lRes, GetLastError());
    SetLastError(0);
    lRes = SendMessageCallbackW(hwnd, CB_FINDSTRINGEXACT, 0, (LPARAM)wszUnicode, NULL, 0);
    ok(lRes == 0 && (GetLastError() == ERROR_MESSAGE_SYNC_ONLY || GetLastError() == ERROR_INVALID_PARAMETER),
        "SendMessageCallback on sync only message returned %ld, last error %d\n", lRes, GetLastError());

    /* Check WM_GETTEXTLENGTH A->W behaviour, whether WM_GETTEXT is also sent or not */

    hwnd = CreateWindowW (testWindowClassW, wszUnicode,
                          WS_OVERLAPPEDWINDOW,
                          100, 100, 200, 200, 0, 0, 0, NULL);
    assert(hwnd);
    flush_sequence();
    lRes = SendMessageA (hwnd, WM_GETTEXTLENGTH, 0, 0);
    ok_sequence(WmGetTextLengthAfromW, "ANSI WM_GETTEXTLENGTH to Unicode window", FALSE);
    ok( lRes == WideCharToMultiByte( CP_ACP, 0, wszUnicode, lstrlenW(wszUnicode), NULL, 0, NULL, NULL ),
        "got bad length %ld\n", lRes );

    flush_sequence();
    lRes = CallWindowProcA( (WNDPROC)GetWindowLongPtrA( hwnd, GWLP_WNDPROC ),
                            hwnd, WM_GETTEXTLENGTH, 0, 0);
    ok_sequence(WmGetTextLengthAfromW, "ANSI WM_GETTEXTLENGTH to Unicode window", FALSE);
    ok( lRes == WideCharToMultiByte( CP_ACP, 0, wszUnicode, lstrlenW(wszUnicode), NULL, 0, NULL, NULL ),
        "got bad length %ld\n", lRes );

    wndproc = (WNDPROC)SetWindowLongPtrW( hwnd, GWLP_WNDPROC, (LONG_PTR)get_text_len_proc );
    newproc = (WNDPROC)GetWindowLongPtrA( hwnd, GWLP_WNDPROC );
    lRes = CallWindowProcA( newproc, hwnd, WM_GETTEXTLENGTH, 0, 0 );
    ok( lRes == WideCharToMultiByte( CP_ACP, 0, dummy_window_text, lstrlenW(dummy_window_text),
                                     NULL, 0, NULL, NULL ) ||
        broken(lRes == lstrlenW(dummy_window_text) + 37),
        "got bad length %ld\n", lRes );

    SetWindowLongPtrW( hwnd, GWLP_WNDPROC, (LONG_PTR)wndproc );  /* restore old wnd proc */
    lRes = CallWindowProcA( newproc, hwnd, WM_GETTEXTLENGTH, 0, 0 );
    ok( lRes == WideCharToMultiByte( CP_ACP, 0, dummy_window_text, lstrlenW(dummy_window_text),
                                     NULL, 0, NULL, NULL ) ||
        broken(lRes == lstrlenW(dummy_window_text) + 37),
        "got bad length %ld\n", lRes );

    ret = DestroyWindow(hwnd);
    ok( ret, "DestroyWindow() error %d\n", GetLastError());
}

struct timer_info
{
    HWND hWnd;
    HANDLE handles[2];
    DWORD id;
};

static VOID CALLBACK tfunc(HWND hwnd, UINT uMsg, UINT_PTR id, DWORD dwTime)
{
}

#define TIMER_ID               0x19
#define TIMER_COUNT_EXPECTED   100
#define TIMER_COUNT_TOLERANCE  10

static int count = 0;
static void CALLBACK callback_count(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    count++;
}

static DWORD WINAPI timer_thread_proc(LPVOID x)
{
    struct timer_info *info = x;
    DWORD r;

    r = KillTimer(info->hWnd, 0x19);
    ok(r,"KillTimer failed in thread\n");
    r = SetTimer(info->hWnd,TIMER_ID,10000,tfunc);
    ok(r,"SetTimer failed in thread\n");
    ok(r==TIMER_ID,"SetTimer id different\n");
    r = SetEvent(info->handles[0]);
    ok(r,"SetEvent failed in thread\n");
    return 0;
}

static void test_timers(void)
{
    struct timer_info info;
    DWORD start;
    DWORD id;
    MSG msg;

    info.hWnd = CreateWindowA("TestWindowClass", NULL,
       WS_OVERLAPPEDWINDOW ,
       CW_USEDEFAULT, CW_USEDEFAULT, 300, 300, 0,
       NULL, NULL, 0);

    info.id = SetTimer(info.hWnd,TIMER_ID,10000,tfunc);
    ok(info.id, "SetTimer failed\n");
    ok(info.id==TIMER_ID, "SetTimer timer ID different\n");
    info.handles[0] = CreateEventW(NULL,0,0,NULL);
    info.handles[1] = CreateThread(NULL,0,timer_thread_proc,&info,0,&id);

    WaitForMultipleObjects(2, info.handles, FALSE, INFINITE);

    WaitForSingleObject(info.handles[1], INFINITE);

    CloseHandle(info.handles[0]);
    CloseHandle(info.handles[1]);

    ok( KillTimer(info.hWnd, TIMER_ID), "KillTimer failed\n");

    /* Check the minimum allowed timeout for a timer.  MSDN indicates that it should be 10.0 ms,
     * which occurs sometimes, but most testing on the VMs indicates a minimum timeout closer to
     * 15.6 ms.  Since there is some measurement error between test runs we are allowing for
     * ±9 counts (~4 ms) around the expected value.
     */
    count = 0;
    id = SetTimer(info.hWnd, TIMER_ID, 0, callback_count);
    ok(id != 0, "did not get id from SetTimer.\n");
    ok(id==TIMER_ID, "SetTimer timer ID different\n");
    start = GetTickCount();
    while (GetTickCount()-start < 1001 && GetMessageA(&msg, info.hWnd, 0, 0))
        DispatchMessageA(&msg);
    ok(abs(count-TIMER_COUNT_EXPECTED) < TIMER_COUNT_TOLERANCE /* xp */
       || broken(abs(count-64) < TIMER_COUNT_TOLERANCE) /* most common */
       || broken(abs(count-43) < TIMER_COUNT_TOLERANCE) /* w2k3, win8 */,
       "did not get expected count for minimum timeout (%d != ~%d).\n",
       count, TIMER_COUNT_EXPECTED);
    ok(KillTimer(info.hWnd, id), "KillTimer failed\n");
    /* Perform the same check on SetSystemTimer (only available on w2k3 and older) */
    if (pSetSystemTimer)
    {
        int syscount = 0;

        count = 0;
        id = pSetSystemTimer(info.hWnd, TIMER_ID, 0, callback_count);
        ok(id != 0, "did not get id from SetSystemTimer.\n");
        ok(id==TIMER_ID, "SetTimer timer ID different\n");
        start = GetTickCount();
        while (GetTickCount()-start < 1001 && GetMessageA(&msg, info.hWnd, 0, 0))
        {
            if (msg.message == WM_SYSTIMER)
                syscount++;
            DispatchMessageA(&msg);
        }
        ok(abs(syscount-TIMER_COUNT_EXPECTED) < TIMER_COUNT_TOLERANCE
           || broken(abs(syscount-64) < TIMER_COUNT_TOLERANCE) /* most common */
           || broken(syscount > 4000 && syscount < 12000) /* win2k3sp0 */,
           "did not get expected count for minimum timeout (%d != ~%d).\n",
           syscount, TIMER_COUNT_EXPECTED);
        todo_wine ok(count == 0, "did not get expected count for callback timeout (%d != 0).\n",
                                 count);
        ok(pKillSystemTimer(info.hWnd, id), "KillSystemTimer failed\n");
    }

    ok(DestroyWindow(info.hWnd), "failed to destroy window\n");
}

static void test_timers_no_wnd(void)
{
    UINT_PTR id, id2;
    DWORD start;
    MSG msg;

    count = 0;
    id = SetTimer(NULL, 0, 100, callback_count);
    ok(id != 0, "did not get id from SetTimer.\n");
    id2 = SetTimer(NULL, id, 200, callback_count);
    ok(id2 == id, "did not get same id from SetTimer when replacing (%li expected %li).\n", id2, id);
    Sleep(150);
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    ok(count == 0, "did not get zero count as expected (%i).\n", count);
    Sleep(150);
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    ok(count == 1, "did not get one count as expected (%i).\n", count);
    KillTimer(NULL, id);
    Sleep(250);
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    ok(count == 1, "killing replaced timer did not work (%i).\n", count);

    /* Check the minimum allowed timeout for a timer.  MSDN indicates that it should be 10.0 ms,
     * which occurs sometimes, but most testing on the VMs indicates a minimum timeout closer to
     * 15.6 ms.  Since there is some measurement error between test runs we are allowing for
     * ±9 counts (~4 ms) around the expected value.
     */
    count = 0;
    id = SetTimer(NULL, 0, 0, callback_count);
    ok(id != 0, "did not get id from SetTimer.\n");
    start = GetTickCount();
    while (GetTickCount()-start < 1001 && GetMessageA(&msg, NULL, 0, 0))
        DispatchMessageA(&msg);
    ok(abs(count-TIMER_COUNT_EXPECTED) < TIMER_COUNT_TOLERANCE /* xp */
       || broken(abs(count-64) < TIMER_COUNT_TOLERANCE) /* most common */,
       "did not get expected count for minimum timeout (%d != ~%d).\n",
       count, TIMER_COUNT_EXPECTED);
    KillTimer(NULL, id);
    /* Note: SetSystemTimer doesn't support a NULL window, see test_timers */
}

/* Various win events with arbitrary parameters */
static const struct message WmWinEventsSeq[] = {
    { EVENT_SYSTEM_SOUND, winevent_hook|wparam|lparam, OBJID_WINDOW, 0 },
    { EVENT_SYSTEM_ALERT, winevent_hook|wparam|lparam, OBJID_SYSMENU, 1 },
    { EVENT_SYSTEM_FOREGROUND, winevent_hook|wparam|lparam, OBJID_TITLEBAR, 2 },
    { EVENT_SYSTEM_MENUSTART, winevent_hook|wparam|lparam, OBJID_MENU, 3 },
    { EVENT_SYSTEM_MENUEND, winevent_hook|wparam|lparam, OBJID_CLIENT, 4 },
    { EVENT_SYSTEM_MENUPOPUPSTART, winevent_hook|wparam|lparam, OBJID_VSCROLL, 5 },
    { EVENT_SYSTEM_MENUPOPUPEND, winevent_hook|wparam|lparam, OBJID_HSCROLL, 6 },
    { EVENT_SYSTEM_CAPTURESTART, winevent_hook|wparam|lparam, OBJID_SIZEGRIP, 7 },
    { EVENT_SYSTEM_CAPTUREEND, winevent_hook|wparam|lparam, OBJID_CARET, 8 },
    /* our win event hook ignores OBJID_CURSOR events */
    /*{ EVENT_SYSTEM_MOVESIZESTART, winevent_hook|wparam|lparam, OBJID_CURSOR, 9 },*/
    { EVENT_SYSTEM_MOVESIZEEND, winevent_hook|wparam|lparam, OBJID_ALERT, 10 },
    { EVENT_SYSTEM_CONTEXTHELPSTART, winevent_hook|wparam|lparam, OBJID_SOUND, 11 },
    { EVENT_SYSTEM_CONTEXTHELPEND, winevent_hook|wparam|lparam, OBJID_QUERYCLASSNAMEIDX, 12 },
    { EVENT_SYSTEM_DRAGDROPSTART, winevent_hook|wparam|lparam, OBJID_NATIVEOM, 13 },
    { EVENT_SYSTEM_DRAGDROPEND, winevent_hook|wparam|lparam, OBJID_WINDOW, 0 },
    { EVENT_SYSTEM_DIALOGSTART, winevent_hook|wparam|lparam, OBJID_SYSMENU, 1 },
    { EVENT_SYSTEM_DIALOGEND, winevent_hook|wparam|lparam, OBJID_TITLEBAR, 2 },
    { EVENT_SYSTEM_SCROLLINGSTART, winevent_hook|wparam|lparam, OBJID_MENU, 3 },
    { EVENT_SYSTEM_SCROLLINGEND, winevent_hook|wparam|lparam, OBJID_CLIENT, 4 },
    { EVENT_SYSTEM_SWITCHSTART, winevent_hook|wparam|lparam, OBJID_VSCROLL, 5 },
    { EVENT_SYSTEM_SWITCHEND, winevent_hook|wparam|lparam, OBJID_HSCROLL, 6 },
    { EVENT_SYSTEM_MINIMIZESTART, winevent_hook|wparam|lparam, OBJID_SIZEGRIP, 7 },
    { EVENT_SYSTEM_MINIMIZEEND, winevent_hook|wparam|lparam, OBJID_CARET, 8 },
    { 0 }
};
static const struct message WmWinEventCaretSeq[] = {
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, OBJID_CARET, 0 }, /* hook 1 */
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, OBJID_CARET, 0 }, /* hook 1 */
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, OBJID_CARET, 0 }, /* hook 2 */
    { EVENT_OBJECT_NAMECHANGE, winevent_hook|wparam|lparam, OBJID_CARET, 0 }, /* hook 1 */
    { 0 }
};
static const struct message WmWinEventCaretSeq_2[] = {
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, OBJID_CARET, 0 }, /* hook 1/2 */
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, OBJID_CARET, 0 }, /* hook 1/2 */
    { EVENT_OBJECT_NAMECHANGE, winevent_hook|wparam|lparam, OBJID_CARET, 0 }, /* hook 1/2 */
    { 0 }
};
static const struct message WmWinEventAlertSeq[] = {
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, OBJID_ALERT, 0 },
    { 0 }
};
static const struct message WmWinEventAlertSeq_2[] = {
    /* create window in the thread proc */
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, OBJID_WINDOW, 2 },
    /* our test event */
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, OBJID_ALERT, 2 },
    { 0 }
};
static const struct message WmGlobalHookSeq_1[] = {
    /* create window in the thread proc */
    { HCBT_CREATEWND, hook|lparam, 0, 2 },
    /* our test events */
    { HCBT_SYSCOMMAND, hook|wparam|lparam, SC_PREVWINDOW, 2 },
    { HCBT_SYSCOMMAND, hook|wparam|lparam, SC_NEXTWINDOW, 2 },
    { 0 }
};
static const struct message WmGlobalHookSeq_2[] = {
    { HCBT_SYSCOMMAND, hook|wparam|lparam, SC_NEXTWINDOW, 0 }, /* old local hook */
    { HCBT_SYSCOMMAND, hook|wparam|lparam, SC_NEXTWINDOW, 2 }, /* new global hook */
    { HCBT_SYSCOMMAND, hook|wparam|lparam, SC_PREVWINDOW, 0 }, /* old local hook */
    { HCBT_SYSCOMMAND, hook|wparam|lparam, SC_PREVWINDOW, 2 }, /* new global hook */
    { 0 }
};

static const struct message WmMouseLLHookSeq[] = {
    { WM_MOUSEMOVE, hook },
    { WM_LBUTTONUP, hook },
    { WM_MOUSEMOVE, hook },
    { 0 }
};

static void CALLBACK win_event_global_hook_proc(HWINEVENTHOOK hevent,
					 DWORD event,
					 HWND hwnd,
					 LONG object_id,
					 LONG child_id,
					 DWORD thread_id,
					 DWORD event_time)
{
    char buf[256];

    if (GetClassNameA(hwnd, buf, sizeof(buf)))
    {
	if (!lstrcmpiA(buf, "TestWindowClass") ||
	    !lstrcmpiA(buf, "static"))
	{
	    struct recvd_message msg;

            msg.hwnd = hwnd;
	    msg.message = event;
	    msg.flags = winevent_hook|wparam|lparam;
	    msg.wParam = object_id;
	    msg.lParam = (thread_id == GetCurrentThreadId()) ? child_id : (child_id + 2);
            msg.descr = "WEH_2";
	    add_message(&msg);
	}
    }
}

static HHOOK hCBT_global_hook;
static DWORD cbt_global_hook_thread_id;

static LRESULT CALLBACK cbt_global_hook_proc(int nCode, WPARAM wParam, LPARAM lParam) 
{ 
    HWND hwnd;
    char buf[256];

    if (nCode == HCBT_SYSCOMMAND)
    {
	struct recvd_message msg;

        msg.hwnd = 0;
	msg.message = nCode;
	msg.flags = hook|wparam|lparam;
	msg.wParam = wParam;
	msg.lParam = (cbt_global_hook_thread_id == GetCurrentThreadId()) ? 1 : 2;
        msg.descr = "CBT_2";
	add_message(&msg);

	return CallNextHookEx(hCBT_global_hook, nCode, wParam, lParam);
    }
    /* WH_MOUSE_LL hook */
    if (nCode == HC_ACTION)
    {
        MSLLHOOKSTRUCT *mhll = (MSLLHOOKSTRUCT *)lParam;

        /* we can't test for real mouse events */
        if (mhll->flags & LLMHF_INJECTED)
        {
	    struct recvd_message msg;

	    memset (&msg, 0, sizeof (msg));
	    msg.message = wParam;
	    msg.flags = hook;
            msg.descr = "CBT_2";
	    add_message(&msg);
        }
	return CallNextHookEx(hCBT_global_hook, nCode, wParam, lParam);
    }

    /* Log also SetFocus(0) calls */
    hwnd = wParam ? (HWND)wParam : (HWND)lParam;

    if (GetClassNameA(hwnd, buf, sizeof(buf)))
    {
	if (!lstrcmpiA(buf, "TestWindowClass") ||
	    !lstrcmpiA(buf, "static"))
	{
	    struct recvd_message msg;

            msg.hwnd = hwnd;
	    msg.message = nCode;
	    msg.flags = hook|wparam|lparam;
	    msg.wParam = wParam;
	    msg.lParam = (cbt_global_hook_thread_id == GetCurrentThreadId()) ? 1 : 2;
            msg.descr = "CBT_2";
	    add_message(&msg);
	}
    }
    return CallNextHookEx(hCBT_global_hook, nCode, wParam, lParam);
}

static DWORD WINAPI win_event_global_thread_proc(void *param)
{
    HWND hwnd;
    MSG msg;
    HANDLE hevent = *(HANDLE *)param;

    assert(pNotifyWinEvent);

    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP, 0,0,0,0,0,0,0, NULL);
    assert(hwnd);
    trace("created thread window %p\n", hwnd);

    *(HWND *)param = hwnd;

    flush_sequence();
    /* this event should be received only by our new hook proc,
     * an old one does not expect an event from another thread.
     */
    pNotifyWinEvent(EVENT_OBJECT_LOCATIONCHANGE, hwnd, OBJID_ALERT, 0);
    SetEvent(hevent);

    while (GetMessageA(&msg, 0, 0, 0))
    {
	TranslateMessage(&msg);
	DispatchMessageA(&msg);
    }
    return 0;
}

static DWORD WINAPI cbt_global_hook_thread_proc(void *param)
{
    HWND hwnd;
    MSG msg;
    HANDLE hevent = *(HANDLE *)param;

    flush_sequence();
    /* these events should be received only by our new hook proc,
     * an old one does not expect an event from another thread.
     */

    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP, 0,0,0,0,0,0,0, NULL);
    assert(hwnd);
    trace("created thread window %p\n", hwnd);

    *(HWND *)param = hwnd;

    /* Windows doesn't like when a thread plays games with the focus,
       that leads to all kinds of misbehaviours and failures to activate
       a window. So, better keep next lines commented out.
    SetFocus(0);
    SetFocus(hwnd);*/

    DefWindowProcA(hwnd, WM_SYSCOMMAND, SC_PREVWINDOW, 0);
    DefWindowProcA(hwnd, WM_SYSCOMMAND, SC_NEXTWINDOW, 0);

    SetEvent(hevent);

    while (GetMessageA(&msg, 0, 0, 0))
    {
	TranslateMessage(&msg);
	DispatchMessageA(&msg);
    }
    return 0;
}

static DWORD WINAPI mouse_ll_global_thread_proc(void *param)
{
    HWND hwnd;
    MSG msg;
    HANDLE hevent = *(HANDLE *)param;

    hwnd = CreateWindowExA(0, "static", NULL, WS_POPUP, 0,0,0,0,0,0,0, NULL);
    assert(hwnd);
    trace("created thread window %p\n", hwnd);

    *(HWND *)param = hwnd;

    flush_sequence();

    /* Windows doesn't like when a thread plays games with the focus,
     * that leads to all kinds of misbehaviours and failures to activate
     * a window. So, better don't generate a mouse click message below.
     */
    mouse_event(MOUSEEVENTF_MOVE, -1, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_MOVE, 1, 0, 0, 0);

    SetEvent(hevent);
    while (GetMessageA(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return 0;
}

static void test_winevents(void)
{
    BOOL ret;
    MSG msg;
    HWND hwnd, hwnd2;
    UINT i;
    HANDLE hthread, hevent;
    DWORD tid;
    HWINEVENTHOOK hhook;
    const struct message *events = WmWinEventsSeq;

    hwnd = CreateWindowExA(0, "TestWindowClass", NULL,
			   WS_OVERLAPPEDWINDOW,
			   CW_USEDEFAULT, CW_USEDEFAULT, 300, 300, 0,
			   NULL, NULL, 0);
    assert(hwnd);

    /****** start of global hook test *************/
    hCBT_global_hook = SetWindowsHookExA(WH_CBT, cbt_global_hook_proc, GetModuleHandleA(0), 0);
    if (!hCBT_global_hook)
    {
        ok(DestroyWindow(hwnd), "failed to destroy window\n");
        skip( "cannot set global hook\n" );
        return;
    }

    hevent = CreateEventA(NULL, 0, 0, NULL);
    assert(hevent);
    hwnd2 = hevent;

    hthread = CreateThread(NULL, 0, cbt_global_hook_thread_proc, &hwnd2, 0, &tid);
    ok(hthread != NULL, "CreateThread failed, error %d\n", GetLastError());

    ok(WaitForSingleObject(hevent, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");

    ok_sequence(WmGlobalHookSeq_1, "global hook 1", FALSE);

    flush_sequence();
    /* this one should be received only by old hook proc */
    DefWindowProcA(hwnd, WM_SYSCOMMAND, SC_NEXTWINDOW, 0);
    /* this one should be received only by old hook proc */
    DefWindowProcA(hwnd, WM_SYSCOMMAND, SC_PREVWINDOW, 0);

    ok_sequence(WmGlobalHookSeq_2, "global hook 2", FALSE);

    ret = UnhookWindowsHookEx(hCBT_global_hook);
    ok( ret, "UnhookWindowsHookEx error %d\n", GetLastError());

    PostThreadMessageA(tid, WM_QUIT, 0, 0);
    ok(WaitForSingleObject(hthread, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(hthread);
    CloseHandle(hevent);
    ok(!IsWindow(hwnd2), "window should be destroyed on thread exit\n");
    /****** end of global hook test *************/

    if (!pSetWinEventHook || !pNotifyWinEvent || !pUnhookWinEvent)
    {
	ok(DestroyWindow(hwnd), "failed to destroy window\n");
	return;
    }

    flush_sequence();

    if (0)
    {
    /* this test doesn't pass under Win9x */
    /* win2k ignores events with hwnd == 0 */
    SetLastError(0xdeadbeef);
    pNotifyWinEvent(events[0].message, 0, events[0].wParam, events[0].lParam);
    ok(GetLastError() == ERROR_INVALID_WINDOW_HANDLE || /* Win2k */
       GetLastError() == 0xdeadbeef, /* Win9x */
       "unexpected error %d\n", GetLastError());
    ok_sequence(WmEmptySeq, "empty notify winevents", FALSE);
    }

    for (i = 0; i < sizeof(WmWinEventsSeq)/sizeof(WmWinEventsSeq[0]); i++)
	pNotifyWinEvent(events[i].message, hwnd, events[i].wParam, events[i].lParam);

    ok_sequence(WmWinEventsSeq, "notify winevents", FALSE);

    /****** start of event filtering test *************/
    hhook = pSetWinEventHook(
	EVENT_OBJECT_SHOW, /* 0x8002 */
	EVENT_OBJECT_LOCATIONCHANGE, /* 0x800B */
	GetModuleHandleA(0), win_event_global_hook_proc,
	GetCurrentProcessId(), 0,
	WINEVENT_INCONTEXT);
    ok(hhook != 0, "SetWinEventHook error %d\n", GetLastError());

    hevent = CreateEventA(NULL, 0, 0, NULL);
    assert(hevent);
    hwnd2 = hevent;

    hthread = CreateThread(NULL, 0, win_event_global_thread_proc, &hwnd2, 0, &tid);
    ok(hthread != NULL, "CreateThread failed, error %d\n", GetLastError());

    ok(WaitForSingleObject(hevent, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");

    ok_sequence(WmWinEventAlertSeq, "alert winevent", FALSE);

    flush_sequence();
    /* this one should be received only by old hook proc */
    pNotifyWinEvent(EVENT_OBJECT_CREATE, hwnd, OBJID_CARET, 0); /* 0x8000 */
    pNotifyWinEvent(EVENT_OBJECT_SHOW, hwnd, OBJID_CARET, 0); /* 0x8002 */
    /* this one should be received only by old hook proc */
    pNotifyWinEvent(EVENT_OBJECT_NAMECHANGE, hwnd, OBJID_CARET, 0); /* 0x800C */

    ok_sequence(WmWinEventCaretSeq, "caret winevent", FALSE);

    ret = pUnhookWinEvent(hhook);
    ok( ret, "UnhookWinEvent error %d\n", GetLastError());

    PostThreadMessageA(tid, WM_QUIT, 0, 0);
    ok(WaitForSingleObject(hthread, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(hthread);
    CloseHandle(hevent);
    ok(!IsWindow(hwnd2), "window should be destroyed on thread exit\n");
    /****** end of event filtering test *************/

    /****** start of out of context event test *************/
    hhook = pSetWinEventHook(EVENT_MIN, EVENT_MAX, 0,
        win_event_global_hook_proc, GetCurrentProcessId(), 0,
	WINEVENT_OUTOFCONTEXT);
    ok(hhook != 0, "SetWinEventHook error %d\n", GetLastError());

    hevent = CreateEventA(NULL, 0, 0, NULL);
    assert(hevent);
    hwnd2 = hevent;

    flush_sequence();

    hthread = CreateThread(NULL, 0, win_event_global_thread_proc, &hwnd2, 0, &tid);
    ok(hthread != NULL, "CreateThread failed, error %d\n", GetLastError());

    ok(WaitForSingleObject(hevent, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");

    ok_sequence(WmEmptySeq, "empty notify winevents", FALSE);
    /* process pending winevent messages */
    ok(!PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE), "msg queue should be empty\n");
    ok_sequence(WmWinEventAlertSeq_2, "alert winevent for out of context proc", FALSE);

    flush_sequence();
    /* this one should be received only by old hook proc */
    pNotifyWinEvent(EVENT_OBJECT_CREATE, hwnd, OBJID_CARET, 0); /* 0x8000 */
    pNotifyWinEvent(EVENT_OBJECT_SHOW, hwnd, OBJID_CARET, 0); /* 0x8002 */
    /* this one should be received only by old hook proc */
    pNotifyWinEvent(EVENT_OBJECT_NAMECHANGE, hwnd, OBJID_CARET, 0); /* 0x800C */

    ok_sequence(WmWinEventCaretSeq_2, "caret winevent for incontext proc", FALSE);
    /* process pending winevent messages */
    ok(!PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE), "msg queue should be empty\n");
    ok_sequence(WmWinEventCaretSeq_2, "caret winevent for out of context proc", FALSE);

    ret = pUnhookWinEvent(hhook);
    ok( ret, "UnhookWinEvent error %d\n", GetLastError());

    PostThreadMessageA(tid, WM_QUIT, 0, 0);
    ok(WaitForSingleObject(hthread, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(hthread);
    CloseHandle(hevent);
    ok(!IsWindow(hwnd2), "window should be destroyed on thread exit\n");
    /****** end of out of context event test *************/

    /****** start of MOUSE_LL hook test *************/
    hCBT_global_hook = SetWindowsHookExA(WH_MOUSE_LL, cbt_global_hook_proc, GetModuleHandleA(0), 0);
    /* WH_MOUSE_LL is not supported on Win9x platforms */
    if (!hCBT_global_hook)
    {
        win_skip("Skipping WH_MOUSE_LL test on this platform\n");
        goto skip_mouse_ll_hook_test;
    }

    hevent = CreateEventA(NULL, 0, 0, NULL);
    assert(hevent);
    hwnd2 = hevent;

    hthread = CreateThread(NULL, 0, mouse_ll_global_thread_proc, &hwnd2, 0, &tid);
    ok(hthread != NULL, "CreateThread failed, error %d\n", GetLastError());

    while (WaitForSingleObject(hevent, 100) == WAIT_TIMEOUT)
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

    ok_sequence(WmMouseLLHookSeq, "MOUSE_LL hook other thread", FALSE);
    flush_sequence();

    mouse_event(MOUSEEVENTF_MOVE, -1, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_MOVE, 1, 0, 0, 0);

    ok_sequence(WmMouseLLHookSeq, "MOUSE_LL hook same thread", FALSE);

    ret = UnhookWindowsHookEx(hCBT_global_hook);
    ok( ret, "UnhookWindowsHookEx error %d\n", GetLastError());

    PostThreadMessageA(tid, WM_QUIT, 0, 0);
    ok(WaitForSingleObject(hthread, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(hthread);
    CloseHandle(hevent);
    ok(!IsWindow(hwnd2), "window should be destroyed on thread exit\n");
    /****** end of MOUSE_LL hook test *************/
skip_mouse_ll_hook_test:

    ok(DestroyWindow(hwnd), "failed to destroy window\n");
}

static void test_set_hook(void)
{
    BOOL ret;
    HHOOK hhook;
    HWINEVENTHOOK hwinevent_hook;

    hhook = SetWindowsHookExA(WH_CBT, cbt_hook_proc, GetModuleHandleA(0), GetCurrentThreadId());
    ok(hhook != 0, "local hook does not require hModule set to 0\n");
    UnhookWindowsHookEx(hhook);

    if (0)
    {
    /* this test doesn't pass under Win9x: BUG! */
    SetLastError(0xdeadbeef);
    hhook = SetWindowsHookExA(WH_CBT, cbt_hook_proc, 0, 0);
    ok(!hhook, "global hook requires hModule != 0\n");
    ok(GetLastError() == ERROR_HOOK_NEEDS_HMOD, "unexpected error %d\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    hhook = SetWindowsHookExA(WH_CBT, 0, GetModuleHandleA(0), GetCurrentThreadId());
    ok(!hhook, "SetWinEventHook with invalid proc should fail\n");
    ok(GetLastError() == ERROR_INVALID_FILTER_PROC || /* Win2k */
       GetLastError() == 0xdeadbeef, /* Win9x */
       "unexpected error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ok(!UnhookWindowsHookEx((HHOOK)0xdeadbeef), "UnhookWindowsHookEx succeeded\n");
    ok(GetLastError() == ERROR_INVALID_HOOK_HANDLE || /* Win2k */
       GetLastError() == 0xdeadbeef, /* Win9x */
       "unexpected error %d\n", GetLastError());

    if (!pSetWinEventHook || !pUnhookWinEvent) return;

    /* even process local incontext hooks require hmodule */
    SetLastError(0xdeadbeef);
    hwinevent_hook = pSetWinEventHook(EVENT_MIN, EVENT_MAX, 0, win_event_proc,
        GetCurrentProcessId(), 0, WINEVENT_INCONTEXT);
    ok(!hwinevent_hook, "WINEVENT_INCONTEXT requires hModule != 0\n");
    ok(GetLastError() == ERROR_HOOK_NEEDS_HMOD || /* Win2k */
       GetLastError() == 0xdeadbeef, /* Win9x */
       "unexpected error %d\n", GetLastError());

    /* even thread local incontext hooks require hmodule */
    SetLastError(0xdeadbeef);
    hwinevent_hook = pSetWinEventHook(EVENT_MIN, EVENT_MAX, 0, win_event_proc,
        GetCurrentProcessId(), GetCurrentThreadId(), WINEVENT_INCONTEXT);
    ok(!hwinevent_hook, "WINEVENT_INCONTEXT requires hModule != 0\n");
    ok(GetLastError() == ERROR_HOOK_NEEDS_HMOD || /* Win2k */
       GetLastError() == 0xdeadbeef, /* Win9x */
       "unexpected error %d\n", GetLastError());

    if (0)
    {
    /* these 3 tests don't pass under Win9x */
    SetLastError(0xdeadbeef);
    hwinevent_hook = pSetWinEventHook(1, 0, 0, win_event_proc,
        GetCurrentProcessId(), 0, WINEVENT_OUTOFCONTEXT);
    ok(!hwinevent_hook, "SetWinEventHook with invalid event range should fail\n");
    ok(GetLastError() == ERROR_INVALID_HOOK_FILTER, "unexpected error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    hwinevent_hook = pSetWinEventHook(-1, 1, 0, win_event_proc,
        GetCurrentProcessId(), 0, WINEVENT_OUTOFCONTEXT);
    ok(!hwinevent_hook, "SetWinEventHook with invalid event range should fail\n");
    ok(GetLastError() == ERROR_INVALID_HOOK_FILTER, "unexpected error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    hwinevent_hook = pSetWinEventHook(EVENT_MIN, EVENT_MAX, 0, win_event_proc,
        0, 0xdeadbeef, WINEVENT_OUTOFCONTEXT);
    ok(!hwinevent_hook, "SetWinEventHook with invalid tid should fail\n");
    ok(GetLastError() == ERROR_INVALID_THREAD_ID, "unexpected error %d\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    hwinevent_hook = pSetWinEventHook(0, 0, 0, win_event_proc,
        GetCurrentProcessId(), 0, WINEVENT_OUTOFCONTEXT);
    ok(hwinevent_hook != 0, "SetWinEventHook error %d\n", GetLastError());
    ok(GetLastError() == 0xdeadbeef, "unexpected error %d\n", GetLastError());
    ret = pUnhookWinEvent(hwinevent_hook);
    ok( ret, "UnhookWinEvent error %d\n", GetLastError());

todo_wine {
    /* This call succeeds under win2k SP4, but fails under Wine.
       Does win2k test/use passed process id? */
    SetLastError(0xdeadbeef);
    hwinevent_hook = pSetWinEventHook(EVENT_MIN, EVENT_MAX, 0, win_event_proc,
        0xdeadbeef, 0, WINEVENT_OUTOFCONTEXT);
    ok(hwinevent_hook != 0, "SetWinEventHook error %d\n", GetLastError());
    ok(GetLastError() == 0xdeadbeef, "unexpected error %d\n", GetLastError());
    ret = pUnhookWinEvent(hwinevent_hook);
    ok( ret, "UnhookWinEvent error %d\n", GetLastError());
}

    SetLastError(0xdeadbeef);
    ok(!pUnhookWinEvent((HWINEVENTHOOK)0xdeadbeef), "UnhookWinEvent succeeded\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE || /* Win2k */
	GetLastError() == 0xdeadbeef, /* Win9x */
	"unexpected error %d\n", GetLastError());
}

static const struct message ScrollWindowPaint1[] = {
    { WM_PAINT, sent },
    { WM_ERASEBKGND, sent|beginpaint },
    { WM_GETTEXTLENGTH, sent|optional },
    { WM_PAINT, sent|optional },
    { WM_NCPAINT, sent|beginpaint|optional },
    { WM_GETTEXT, sent|beginpaint|optional },
    { WM_GETTEXT, sent|beginpaint|optional },
    { WM_GETTEXT, sent|beginpaint|optional },
    { WM_GETTEXT, sent|beginpaint|defwinproc|optional },
    { WM_ERASEBKGND, sent|beginpaint|optional },
    { 0 }
};

static const struct message ScrollWindowPaint2[] = {
    { WM_PAINT, sent },
    { 0 }
};

static void test_scrollwindowex(void)
{
    HWND hwnd, hchild;
    RECT rect={0,0,130,130};

    hwnd = CreateWindowExA(0, "TestWindowClass", "Test Scroll",
            WS_VISIBLE|WS_OVERLAPPEDWINDOW,
            100, 100, 200, 200, 0, 0, 0, NULL);
    ok (hwnd != 0, "Failed to create overlapped window\n");
    hchild = CreateWindowExA(0, "TestWindowClass", "Test child", 
            WS_VISIBLE|WS_CAPTION|WS_CHILD,
            10, 10, 150, 150, hwnd, 0, 0, NULL);
    ok (hchild != 0, "Failed to create child\n");
    UpdateWindow(hwnd);
    flush_events();
    flush_sequence();

    /* scroll without the child window */
    trace("start scroll\n");
    ScrollWindowEx( hwnd, 10, 10, &rect, NULL, NULL, NULL,
            SW_ERASE|SW_INVALIDATE);
    ok_sequence(WmEmptySeq, "ScrollWindowEx", FALSE);
    trace("end scroll\n");
    flush_sequence();
    flush_events();
    ok_sequence(ScrollWindowPaint1, "ScrollWindowEx", FALSE);
    flush_events();
    flush_sequence();

    /* Now without the SW_ERASE flag */
    trace("start scroll\n");
    ScrollWindowEx( hwnd, 10, 10, &rect, NULL, NULL, NULL, SW_INVALIDATE);
    ok_sequence(WmEmptySeq, "ScrollWindowEx", FALSE);
    trace("end scroll\n");
    flush_sequence();
    flush_events();
    ok_sequence(ScrollWindowPaint2, "ScrollWindowEx", FALSE);
    flush_events();
    flush_sequence();

    /* now scroll the child window as well */
    trace("start scroll\n");
    ScrollWindowEx( hwnd, 10, 10, &rect, NULL, NULL, NULL,
            SW_SCROLLCHILDREN|SW_ERASE|SW_INVALIDATE);
    /* wine sends WM_POSCHANGING, WM_POSCHANGED messages */
    /* windows sometimes a WM_MOVE */
    ok_sequence(WmEmptySeq, "ScrollWindowEx", TRUE);
    trace("end scroll\n");
    flush_sequence();
    flush_events();
    ok_sequence(ScrollWindowPaint1, "ScrollWindowEx", FALSE);
    flush_events();
    flush_sequence();

    /* now scroll with ScrollWindow() */
    trace("start scroll with ScrollWindow\n");
    ScrollWindow( hwnd, 5, 5, NULL, NULL);
    trace("end scroll\n");
    flush_sequence();
    flush_events();
    ok_sequence(ScrollWindowPaint1, "ScrollWindow", FALSE);

    ok(DestroyWindow(hchild), "failed to destroy window\n");
    ok(DestroyWindow(hwnd), "failed to destroy window\n");
    flush_sequence();
}

static const struct message destroy_window_with_children[] = {
    { EVENT_SYSTEM_CAPTURESTART, winevent_hook|wparam|lparam, 0, 0 }, /* popup */
    { HCBT_DESTROYWND, hook|lparam, 0, WND_PARENT_ID }, /* parent */
    { 0x0090, sent|optional },
    { HCBT_DESTROYWND, hook|lparam, 0, WND_POPUP_ID }, /* popup */
    { 0x0090, sent|optional },
    { EVENT_OBJECT_DESTROY, winevent_hook|wparam|lparam, 0, 0 }, /* popup */
    { WM_DESTROY, sent|wparam|lparam, 0, WND_POPUP_ID }, /* popup */
    { WM_CAPTURECHANGED, sent|wparam|lparam, 0, WND_POPUP_ID }, /* popup */
    { WM_NCDESTROY, sent|wparam|lparam, 0, WND_POPUP_ID }, /* popup */
    { EVENT_OBJECT_DESTROY, winevent_hook|wparam|lparam, 0, 0 }, /* parent */
    { WM_DESTROY, sent|wparam|lparam, 0, WND_PARENT_ID }, /* parent */
    { WM_DESTROY, sent|wparam|lparam, 0, WND_CHILD_ID + 2 }, /* child2 */
    { WM_DESTROY, sent|wparam|lparam, 0, WND_CHILD_ID + 1 }, /* child1 */
    { WM_DESTROY, sent|wparam|lparam, 0, WND_CHILD_ID + 3 }, /* child3 */
    { WM_NCDESTROY, sent|wparam|lparam, 0, WND_CHILD_ID + 2 }, /* child2 */
    { WM_NCDESTROY, sent|wparam|lparam, 0, WND_CHILD_ID + 3 }, /* child3 */
    { WM_NCDESTROY, sent|wparam|lparam, 0, WND_CHILD_ID + 1 }, /* child1 */
    { WM_NCDESTROY, sent|wparam|lparam, 0, WND_PARENT_ID }, /* parent */
    { 0 }
};

static void test_DestroyWindow(void)
{
    BOOL ret;
    HWND parent, child1, child2, child3, child4, test;
    UINT_PTR child_id = WND_CHILD_ID + 1;

    parent = CreateWindowExA(0, "TestWindowClass", NULL, WS_OVERLAPPEDWINDOW,
			     100, 100, 200, 200, 0, 0, 0, NULL);
    assert(parent != 0);
    child1 = CreateWindowExA(0, "TestWindowClass", NULL, WS_CHILD,
			     0, 0, 50, 50, parent, (HMENU)child_id++, 0, NULL);
    assert(child1 != 0);
    child2 = CreateWindowExA(0, "TestWindowClass", NULL, WS_CHILD,
			     0, 0, 50, 50, GetDesktopWindow(), (HMENU)child_id++, 0, NULL);
    assert(child2 != 0);
    child3 = CreateWindowExA(0, "TestWindowClass", NULL, WS_CHILD,
			     0, 0, 50, 50, child1, (HMENU)child_id++, 0, NULL);
    assert(child3 != 0);
    child4 = CreateWindowExA(0, "TestWindowClass", NULL, WS_POPUP,
			     0, 0, 50, 50, parent, 0, 0, NULL);
    assert(child4 != 0);

    /* test owner/parent of child2 */
    test = GetParent(child2);
    ok(test == GetDesktopWindow(), "wrong parent %p\n", test);
    ok(!IsChild(parent, child2), "wrong parent/child %p/%p\n", parent, child2);
    if(pGetAncestor) {
        test = pGetAncestor(child2, GA_PARENT);
        ok(test == GetDesktopWindow(), "wrong parent %p\n", test);
    }
    test = GetWindow(child2, GW_OWNER);
    ok(!test, "wrong owner %p\n", test);

    test = SetParent(child2, parent);
    ok(test == GetDesktopWindow(), "wrong old parent %p\n", test);

    /* test owner/parent of the parent */
    test = GetParent(parent);
    ok(!test, "wrong parent %p\n", test);
    ok(!IsChild(GetDesktopWindow(), parent), "wrong parent/child %p/%p\n", GetDesktopWindow(), parent);
    if(pGetAncestor) {
        test = pGetAncestor(parent, GA_PARENT);
        ok(test == GetDesktopWindow(), "wrong parent %p\n", test);
    }
    test = GetWindow(parent, GW_OWNER);
    ok(!test, "wrong owner %p\n", test);

    /* test owner/parent of child1 */
    test = GetParent(child1);
    ok(test == parent, "wrong parent %p\n", test);
    ok(IsChild(parent, child1), "wrong parent/child %p/%p\n", parent, child1);
    if(pGetAncestor) {
        test = pGetAncestor(child1, GA_PARENT);
        ok(test == parent, "wrong parent %p\n", test);
    }
    test = GetWindow(child1, GW_OWNER);
    ok(!test, "wrong owner %p\n", test);

    /* test owner/parent of child2 */
    test = GetParent(child2);
    ok(test == parent, "wrong parent %p\n", test);
    ok(IsChild(parent, child2), "wrong parent/child %p/%p\n", parent, child2);
    if(pGetAncestor) {
        test = pGetAncestor(child2, GA_PARENT);
        ok(test == parent, "wrong parent %p\n", test);
    }
    test = GetWindow(child2, GW_OWNER);
    ok(!test, "wrong owner %p\n", test);

    /* test owner/parent of child3 */
    test = GetParent(child3);
    ok(test == child1, "wrong parent %p\n", test);
    ok(IsChild(parent, child3), "wrong parent/child %p/%p\n", parent, child3);
    if(pGetAncestor) {
        test = pGetAncestor(child3, GA_PARENT);
        ok(test == child1, "wrong parent %p\n", test);
    }
    test = GetWindow(child3, GW_OWNER);
    ok(!test, "wrong owner %p\n", test);

    /* test owner/parent of child4 */
    test = GetParent(child4);
    ok(test == parent, "wrong parent %p\n", test);
    ok(!IsChild(parent, child4), "wrong parent/child %p/%p\n", parent, child4);
    if(pGetAncestor) {
        test = pGetAncestor(child4, GA_PARENT);
        ok(test == GetDesktopWindow(), "wrong parent %p\n", test);
    }
    test = GetWindow(child4, GW_OWNER);
    ok(test == parent, "wrong owner %p\n", test);

    flush_sequence();

    trace("parent %p, child1 %p, child2 %p, child3 %p, child4 %p\n",
	   parent, child1, child2, child3, child4);

    SetCapture(child4);
    test = GetCapture();
    ok(test == child4, "wrong capture window %p\n", test);

    test_DestroyWindow_flag = TRUE;
    ret = DestroyWindow(parent);
    ok( ret, "DestroyWindow() error %d\n", GetLastError());
    test_DestroyWindow_flag = FALSE;
    ok_sequence(destroy_window_with_children, "destroy window with children", FALSE);

    ok(!IsWindow(parent), "parent still exists\n");
    ok(!IsWindow(child1), "child1 still exists\n");
    ok(!IsWindow(child2), "child2 still exists\n");
    ok(!IsWindow(child3), "child3 still exists\n");
    ok(!IsWindow(child4), "child4 still exists\n");

    test = GetCapture();
    ok(!test, "wrong capture window %p\n", test);
}


static const struct message WmDispatchPaint[] = {
    { WM_NCPAINT, sent },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent },
    { 0 }
};

static LRESULT WINAPI DispatchMessageCheckProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_PAINT) return 0;
    return MsgCheckProcA( hwnd, message, wParam, lParam );
}

static void test_DispatchMessage(void)
{
    RECT rect;
    MSG msg;
    int count;
    HWND hwnd = CreateWindowA( "TestWindowClass", NULL, WS_OVERLAPPEDWINDOW,
                               100, 100, 200, 200, 0, 0, 0, NULL);
    ShowWindow( hwnd, SW_SHOW );
    UpdateWindow( hwnd );
    flush_events();
    flush_sequence();
    SetWindowLongPtrA( hwnd, GWLP_WNDPROC, (LONG_PTR)DispatchMessageCheckProc );

    SetRect( &rect, -5, -5, 5, 5 );
    RedrawWindow( hwnd, &rect, 0, RDW_INVALIDATE|RDW_ERASE|RDW_FRAME );
    count = 0;
    while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE ))
    {
        if (msg.message != WM_PAINT) DispatchMessageA( &msg );
        else
        {
            flush_sequence();
            DispatchMessageA( &msg );
            /* DispatchMessage will send WM_NCPAINT if non client area is still invalid after WM_PAINT */
            if (!count) ok_sequence( WmDispatchPaint, "WmDispatchPaint", FALSE );
            else ok_sequence( WmEmptySeq, "WmEmpty", FALSE );
            if (++count > 10) break;
        }
    }
    ok( msg.message == WM_PAINT && count > 10, "WM_PAINT messages stopped\n" );

    trace("now without DispatchMessage\n");
    flush_sequence();
    RedrawWindow( hwnd, &rect, 0, RDW_INVALIDATE|RDW_ERASE|RDW_FRAME );
    count = 0;
    while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE ))
    {
        if (msg.message != WM_PAINT) DispatchMessageA( &msg );
        else
        {
            HRGN hrgn = CreateRectRgn( 0, 0, 0, 0 );
            flush_sequence();
            /* this will send WM_NCCPAINT just like DispatchMessage does */
            GetUpdateRgn( hwnd, hrgn, TRUE );
            ok_sequence( WmDispatchPaint, "WmDispatchPaint", FALSE );
            DeleteObject( hrgn );
            GetClientRect( hwnd, &rect );
            ValidateRect( hwnd, &rect );  /* this will stop WM_PAINTs */
            ok( !count, "Got multiple WM_PAINTs\n" );
            if (++count > 10) break;
        }
    }

    flush_sequence();
    RedrawWindow( hwnd, &rect, 0, RDW_INVALIDATE|RDW_ERASE|RDW_FRAME );
    count = 0;
    while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE ))
    {
        if (msg.message != WM_PAINT) DispatchMessageA( &msg );
        else
        {
            HDC hdc;

            flush_sequence();
            hdc = BeginPaint( hwnd, NULL );
            ok( !hdc, "got valid hdc %p from BeginPaint\n", hdc );
            ok( !EndPaint( hwnd, NULL ), "EndPaint succeeded\n" );
            ok_sequence( WmDispatchPaint, "WmDispatchPaint", FALSE );
            ok( !count, "Got multiple WM_PAINTs\n" );
            if (++count > 10) break;
        }
    }
    DestroyWindow(hwnd);
}


static const struct message WmUser[] = {
    { WM_USER, sent },
    { 0 }
};

struct sendmsg_info
{
    HWND  hwnd;
    DWORD timeout;
    DWORD ret;
};

static DWORD CALLBACK send_msg_thread( LPVOID arg )
{
    struct sendmsg_info *info = arg;
    SetLastError( 0xdeadbeef );
    info->ret = SendMessageTimeoutA( info->hwnd, WM_USER, 0, 0, 0, info->timeout, NULL );
    if (!info->ret) ok( GetLastError() == ERROR_TIMEOUT ||
                        broken(GetLastError() == 0),  /* win9x */
                        "unexpected error %d\n", GetLastError());
    return 0;
}

static void wait_for_thread( HANDLE thread )
{
    while (MsgWaitForMultipleObjects(1, &thread, FALSE, INFINITE, QS_SENDMESSAGE) != WAIT_OBJECT_0)
    {
        MSG msg;
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA(&msg);
    }
}

static LRESULT WINAPI send_msg_delay_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_USER) Sleep(200);
    return MsgCheckProcA( hwnd, message, wParam, lParam );
}

static void test_SendMessageTimeout(void)
{
    HANDLE thread;
    struct sendmsg_info info;
    DWORD tid;
    BOOL is_win9x;

    info.hwnd = CreateWindowA( "TestWindowClass", NULL, WS_OVERLAPPEDWINDOW,
                               100, 100, 200, 200, 0, 0, 0, NULL);
    flush_events();
    flush_sequence();

    info.timeout = 1000;
    info.ret = 0xdeadbeef;
    thread = CreateThread( NULL, 0, send_msg_thread, &info, 0, &tid );
    wait_for_thread( thread );
    CloseHandle( thread );
    ok( info.ret == 1, "SendMessageTimeout failed\n" );
    ok_sequence( WmUser, "WmUser", FALSE );

    info.timeout = 1;
    info.ret = 0xdeadbeef;
    thread = CreateThread( NULL, 0, send_msg_thread, &info, 0, &tid );
    Sleep(100);  /* SendMessageTimeout should time out here */
    wait_for_thread( thread );
    CloseHandle( thread );
    ok( info.ret == 0, "SendMessageTimeout succeeded\n" );
    ok_sequence( WmEmptySeq, "WmEmptySeq", FALSE );

    /* 0 means infinite timeout (but not on win9x) */
    info.timeout = 0;
    info.ret = 0xdeadbeef;
    thread = CreateThread( NULL, 0, send_msg_thread, &info, 0, &tid );
    Sleep(100);
    wait_for_thread( thread );
    CloseHandle( thread );
    is_win9x = !info.ret;
    if (is_win9x) ok_sequence( WmEmptySeq, "WmEmptySeq", FALSE );
    else ok_sequence( WmUser, "WmUser", FALSE );

    /* timeout is treated as signed despite the prototype (but not on win9x) */
    info.timeout = 0x7fffffff;
    info.ret = 0xdeadbeef;
    thread = CreateThread( NULL, 0, send_msg_thread, &info, 0, &tid );
    Sleep(100);
    wait_for_thread( thread );
    CloseHandle( thread );
    ok( info.ret == 1, "SendMessageTimeout failed\n" );
    ok_sequence( WmUser, "WmUser", FALSE );

    info.timeout = 0x80000000;
    info.ret = 0xdeadbeef;
    thread = CreateThread( NULL, 0, send_msg_thread, &info, 0, &tid );
    Sleep(100);
    wait_for_thread( thread );
    CloseHandle( thread );
    if (is_win9x)
    {
        ok( info.ret == 1, "SendMessageTimeout failed\n" );
        ok_sequence( WmUser, "WmUser", FALSE );
    }
    else
    {
        ok( info.ret == 0, "SendMessageTimeout succeeded\n" );
        ok_sequence( WmEmptySeq, "WmEmptySeq", FALSE );
    }

    /* now check for timeout during message processing */
    SetWindowLongPtrA( info.hwnd, GWLP_WNDPROC, (LONG_PTR)send_msg_delay_proc );
    info.timeout = 100;
    info.ret = 0xdeadbeef;
    thread = CreateThread( NULL, 0, send_msg_thread, &info, 0, &tid );
    wait_for_thread( thread );
    CloseHandle( thread );
    /* we should time out but still get the message */
    ok( info.ret == 0, "SendMessageTimeout failed\n" );
    ok_sequence( WmUser, "WmUser", FALSE );

    DestroyWindow( info.hwnd );
}


/****************** edit message test *************************/
#define ID_EDIT 0x1234
static const struct message sl_edit_setfocus[] =
{
    { HCBT_SETFOCUS, hook },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|wparam, 0 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 10 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 15 },
    { WM_CTLCOLOREDIT, sent|parent },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 11 },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, OBJID_CARET, 0 },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, OBJID_CARET, 0 },
    { WM_COMMAND, sent|parent|wparam, MAKEWPARAM(ID_EDIT, EN_SETFOCUS) },
    { 0 }
};
static const struct message sl_edit_invisible[] =
{
    { HCBT_SETFOCUS, hook },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_KILLFOCUS, sent|parent },
    { WM_SETFOCUS, sent },
    { WM_COMMAND, sent|parent|wparam, MAKEWPARAM(ID_EDIT, EN_SETFOCUS) },
    { 0 }
};
static const struct message ml_edit_setfocus[] =
{
    { HCBT_SETFOCUS, hook },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|wparam, 0 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 10 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 11 },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, OBJID_CARET, 0 },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, OBJID_CARET, 0 },
    { WM_COMMAND, sent|parent|wparam, MAKEWPARAM(ID_EDIT, EN_SETFOCUS) },
    { 0 }
};
static const struct message sl_edit_killfocus[] =
{
    { HCBT_SETFOCUS, hook },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_KILLFOCUS, sent|wparam, 0 },
    { EVENT_OBJECT_HIDE, winevent_hook|wparam|lparam, OBJID_CARET, 0 },
    { EVENT_OBJECT_DESTROY, winevent_hook|wparam|lparam, OBJID_CARET, 0 },
    { WM_COMMAND, sent|parent|wparam, MAKEWPARAM(ID_EDIT, EN_KILLFOCUS) },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 1 },
    { 0 }
};
static const struct message sl_edit_lbutton_dblclk[] =
{
    { WM_LBUTTONDBLCLK, sent },
    { EVENT_SYSTEM_CAPTURESTART, winevent_hook|wparam|lparam, 0, 0 },
    { 0 }
};
static const struct message sl_edit_lbutton_down[] =
{
    { WM_LBUTTONDOWN, sent|wparam|lparam, 0, 0 },
    { HCBT_SETFOCUS, hook },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|wparam|defwinproc, 0 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 10 },
    { WM_CTLCOLOREDIT, sent|parent },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 11 },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, OBJID_CARET, 0 },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, OBJID_CARET, 0 },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, OBJID_CARET, 0 },
    { WM_COMMAND, sent|parent|wparam, MAKEWPARAM(ID_EDIT, EN_SETFOCUS) },
    { EVENT_SYSTEM_CAPTURESTART, winevent_hook|wparam|lparam, 0, 0 },
    { EVENT_OBJECT_HIDE, winevent_hook|wparam|lparam, OBJID_CARET, 0 },
    { WM_CTLCOLOREDIT, sent|parent|optional },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 11 },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, OBJID_CARET, 0 },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, OBJID_CARET, 0 },
    { 0 }
};
static const struct message ml_edit_lbutton_down[] =
{
    { WM_LBUTTONDOWN, sent|wparam|lparam, 0, 0 },
    { EVENT_SYSTEM_CAPTURESTART, winevent_hook|wparam|lparam, 0, 0 },
    { HCBT_SETFOCUS, hook },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|wparam|defwinproc, 0 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 10 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 11 },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, OBJID_CARET, 0 },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, OBJID_CARET, 0 },
    { WM_COMMAND, sent|parent|wparam, MAKEWPARAM(ID_EDIT, EN_SETFOCUS) },
    { 0 }
};
static const struct message sl_edit_lbutton_up[] =
{
    { WM_LBUTTONUP, sent|wparam|lparam, 0, 0 },
    { EVENT_OBJECT_HIDE, winevent_hook|wparam|lparam, OBJID_CARET, 0 },
    { EVENT_SYSTEM_CAPTUREEND, winevent_hook|wparam|lparam, 0, 0 },
    { WM_CAPTURECHANGED, sent|defwinproc },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, OBJID_CARET, 0 },
    { 0 }
};
static const struct message ml_edit_lbutton_up[] =
{
    { WM_LBUTTONUP, sent|wparam|lparam, 0, 0 },
    { EVENT_SYSTEM_CAPTUREEND, winevent_hook|wparam|lparam, 0, 0 },
    { WM_CAPTURECHANGED, sent|defwinproc },
    { 0 }
};

static WNDPROC old_edit_proc;

static LRESULT CALLBACK edit_hook_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct recvd_message msg;

    if (ignore_message( message )) return 0;

    msg.hwnd = hwnd;
    msg.message = message;
    msg.flags = sent|wparam|lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.descr = "edit";
    add_message(&msg);

    defwndproc_counter++;
    ret = CallWindowProcA(old_edit_proc, hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static void subclass_edit(void)
{
    WNDCLASSA cls;

    if (!GetClassInfoA(0, "edit", &cls)) assert(0);

    old_edit_proc = cls.lpfnWndProc;

    cls.hInstance = GetModuleHandleA(NULL);
    cls.lpfnWndProc = edit_hook_proc;
    cls.lpszClassName = "my_edit_class";
    UnregisterClassA(cls.lpszClassName, cls.hInstance);
    if (!RegisterClassA(&cls)) assert(0);
}

static void test_edit_messages(void)
{
    HWND hwnd, parent;
    DWORD dlg_code;

    subclass_edit();
    log_all_parent_messages++;

    parent = CreateWindowExA(0, "TestParentClass", "Test parent", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                             100, 100, 200, 200, 0, 0, 0, NULL);
    ok (parent != 0, "Failed to create parent window\n");

    /* test single line edit */
    hwnd = CreateWindowExA(0, "my_edit_class", "test", WS_CHILD,
			   0, 0, 80, 20, parent, (HMENU)ID_EDIT, 0, NULL);
    ok(hwnd != 0, "Failed to create edit window\n");

    dlg_code = SendMessageA(hwnd, WM_GETDLGCODE, 0, 0);
    ok(dlg_code == (DLGC_WANTCHARS|DLGC_HASSETSEL|DLGC_WANTARROWS), "wrong dlg_code %08x\n", dlg_code);

    flush_sequence();
    SetFocus(hwnd);
    ok_sequence(sl_edit_invisible, "SetFocus(hwnd) on an invisible edit", FALSE);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    SetFocus(0);
    flush_sequence();

    SetFocus(hwnd);
    ok_sequence(sl_edit_setfocus, "SetFocus(hwnd) on an edit", FALSE);

    SetFocus(0);
    ok_sequence(sl_edit_killfocus, "SetFocus(0) on an edit", FALSE);

    SetFocus(0);
    ReleaseCapture();
    flush_sequence();

    SendMessageA(hwnd, WM_LBUTTONDBLCLK, 0, 0);
    ok_sequence(sl_edit_lbutton_dblclk, "WM_LBUTTONDBLCLK on an edit", FALSE);

    SetFocus(0);
    ReleaseCapture();
    flush_sequence();

    SendMessageA(hwnd, WM_LBUTTONDOWN, 0, 0);
    ok_sequence(sl_edit_lbutton_down, "WM_LBUTTONDOWN on an edit", FALSE);

    SendMessageA(hwnd, WM_LBUTTONUP, 0, 0);
    ok_sequence(sl_edit_lbutton_up, "WM_LBUTTONUP on an edit", FALSE);

    DestroyWindow(hwnd);

    /* test multiline edit */
    hwnd = CreateWindowExA(0, "my_edit_class", "test", WS_CHILD | ES_MULTILINE,
			   0, 0, 80, 20, parent, (HMENU)ID_EDIT, 0, NULL);
    ok(hwnd != 0, "Failed to create edit window\n");

    dlg_code = SendMessageA(hwnd, WM_GETDLGCODE, 0, 0);
    ok(dlg_code == (DLGC_WANTCHARS|DLGC_HASSETSEL|DLGC_WANTARROWS|DLGC_WANTALLKEYS),
       "wrong dlg_code %08x\n", dlg_code);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    SetFocus(0);
    flush_sequence();

    SetFocus(hwnd);
    ok_sequence(ml_edit_setfocus, "SetFocus(hwnd) on multiline edit", FALSE);

    SetFocus(0);
    ok_sequence(sl_edit_killfocus, "SetFocus(0) on multiline edit", FALSE);

    SetFocus(0);
    ReleaseCapture();
    flush_sequence();

    SendMessageA(hwnd, WM_LBUTTONDBLCLK, 0, 0);
    ok_sequence(sl_edit_lbutton_dblclk, "WM_LBUTTONDBLCLK on multiline edit", FALSE);

    SetFocus(0);
    ReleaseCapture();
    flush_sequence();

    SendMessageA(hwnd, WM_LBUTTONDOWN, 0, 0);
    ok_sequence(ml_edit_lbutton_down, "WM_LBUTTONDOWN on multiline edit", FALSE);

    SendMessageA(hwnd, WM_LBUTTONUP, 0, 0);
    ok_sequence(ml_edit_lbutton_up, "WM_LBUTTONUP on multiline edit", FALSE);

    DestroyWindow(hwnd);
    DestroyWindow(parent);

    log_all_parent_messages--;
}

/**************************** End of Edit test ******************************/

static const struct message WmKeyDownSkippedSeq[] =
{
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'N', 1 }, /* XP */
    { 0 }
};
static const struct message WmKeyDownWasDownSkippedSeq[] =
{
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'N', 0x40000001 }, /* XP */
    { 0 }
};
static const struct message WmKeyUpSkippedSeq[] =
{
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'N', 0xc0000001 }, /* XP */
    { 0 }
};
static const struct message WmUserKeyUpSkippedSeq[] =
{
    { WM_USER, sent },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'N', 0xc0000001 }, /* XP */
    { 0 }
};

#define EV_STOP 0
#define EV_SENDMSG 1
#define EV_ACK 2

struct peekmsg_info
{
    HWND  hwnd;
    HANDLE hevent[3]; /* 0 - start/stop, 1 - SendMessage, 2 - ack */
};

static DWORD CALLBACK send_msg_thread_2(void *param)
{
    DWORD ret;
    struct peekmsg_info *info = param;

    trace("thread: looping\n");
    SetEvent(info->hevent[EV_ACK]);

    while (1)
    {
        ret = WaitForMultipleObjects(2, info->hevent, FALSE, INFINITE);

        switch (ret)
        {
        case WAIT_OBJECT_0 + EV_STOP:
            trace("thread: exiting\n");
            return 0;

        case WAIT_OBJECT_0 + EV_SENDMSG:
            trace("thread: sending message\n");
            ret = SendNotifyMessageA(info->hwnd, WM_USER, 0, 0);
            ok(ret, "SendNotifyMessageA failed error %u\n", GetLastError());
            SetEvent(info->hevent[EV_ACK]);
            break;

        default:
            trace("unexpected return: %04x\n", ret);
            assert(0);
            break;
        }
    }
    return 0;
}

static void test_PeekMessage(void)
{
    MSG msg;
    HANDLE hthread;
    DWORD tid, qstatus;
    UINT qs_all_input = QS_ALLINPUT;
    UINT qs_input = QS_INPUT;
    BOOL ret;
    struct peekmsg_info info;

    info.hwnd = CreateWindowA("TestWindowClass", NULL, WS_OVERLAPPEDWINDOW,
                              100, 100, 200, 200, 0, 0, 0, NULL);
    assert(info.hwnd);
    ShowWindow(info.hwnd, SW_SHOW);
    UpdateWindow(info.hwnd);
    SetFocus(info.hwnd);

    info.hevent[EV_STOP] = CreateEventA(NULL, 0, 0, NULL);
    info.hevent[EV_SENDMSG] = CreateEventA(NULL, 0, 0, NULL);
    info.hevent[EV_ACK] = CreateEventA(NULL, 0, 0, NULL);

    hthread = CreateThread(NULL, 0, send_msg_thread_2, &info, 0, &tid);
    WaitForSingleObject(info.hevent[EV_ACK], 10000);

    flush_events();
    flush_sequence();

    SetLastError(0xdeadbeef);
    qstatus = GetQueueStatus(qs_all_input);
    if (GetLastError() == ERROR_INVALID_FLAGS)
    {
        trace("QS_RAWINPUT not supported on this platform\n");
        qs_all_input &= ~QS_RAWINPUT;
        qs_input &= ~QS_RAWINPUT;
    }
    if (qstatus & QS_POSTMESSAGE)
    {
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) /* nothing */ ;
        qstatus = GetQueueStatus(qs_all_input);
    }
    ok(qstatus == 0, "wrong qstatus %08x\n", qstatus);

    trace("signalling to send message\n");
    SetEvent(info.hevent[EV_SENDMSG]);
    WaitForSingleObject(info.hevent[EV_ACK], INFINITE);

    /* pass invalid QS_xxxx flags */
    SetLastError(0xdeadbeef);
    qstatus = GetQueueStatus(0xffffffff);
    ok(qstatus == 0 || broken(qstatus)  /* win9x */, "GetQueueStatus should fail: %08x\n", qstatus);
    if (!qstatus)
    {
        ok(GetLastError() == ERROR_INVALID_FLAGS, "wrong error %d\n", GetLastError());
        qstatus = GetQueueStatus(qs_all_input);
    }
    qstatus &= ~MAKELONG( 0x4000, 0x4000 );  /* sometimes set on Win95 */
    ok(qstatus == MAKELONG(QS_SENDMESSAGE, QS_SENDMESSAGE),
       "wrong qstatus %08x\n", qstatus);

    msg.message = 0;
    ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE);
    ok(!ret,
       "PeekMessageA should have returned FALSE instead of msg %04x\n",
        msg.message);
    ok_sequence(WmUser, "WmUser", FALSE);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == 0, "wrong qstatus %08x\n", qstatus);

    keybd_event('N', 0, 0, 0);
    keybd_event('N', 0, KEYEVENTF_KEYUP, 0);
    qstatus = GetQueueStatus(qs_all_input);
    if (!(qstatus & MAKELONG(QS_KEY, QS_KEY)))
    {
        skip( "queuing key events not supported\n" );
        goto done;
    }
    ok(qstatus == MAKELONG(QS_KEY, QS_KEY) ||
       /* keybd_event seems to trigger a sent message on NT4 */
       qstatus == MAKELONG(QS_KEY|QS_SENDMESSAGE, QS_KEY|QS_SENDMESSAGE),
       "wrong qstatus %08x\n", qstatus);

    PostMessageA(info.hwnd, WM_CHAR, 'z', 0);
    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(QS_POSTMESSAGE, QS_POSTMESSAGE|QS_KEY) ||
       qstatus == MAKELONG(QS_POSTMESSAGE, QS_POSTMESSAGE|QS_KEY|QS_SENDMESSAGE),
       "wrong qstatus %08x\n", qstatus);

    InvalidateRect(info.hwnd, NULL, FALSE);
    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(QS_PAINT, QS_PAINT|QS_POSTMESSAGE|QS_KEY) ||
       qstatus == MAKELONG(QS_PAINT, QS_PAINT|QS_POSTMESSAGE|QS_KEY|QS_SENDMESSAGE),
       "wrong qstatus %08x\n", qstatus);

    trace("signalling to send message\n");
    SetEvent(info.hevent[EV_SENDMSG]);
    WaitForSingleObject(info.hevent[EV_ACK], INFINITE);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(QS_SENDMESSAGE, QS_SENDMESSAGE|QS_PAINT|QS_POSTMESSAGE|QS_KEY),
       "wrong qstatus %08x\n", qstatus);

    msg.message = 0;
    ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE | (qs_input << 16));
    if (ret && msg.message == WM_CHAR)
    {
        win_skip( "PM_QS_* flags not supported in PeekMessage\n" );
        goto done;
    }
    ok(!ret,
       "PeekMessageA should have returned FALSE instead of msg %04x\n",
        msg.message);
    if (!sequence_cnt)  /* nt4 doesn't fetch anything with PM_QS_* flags */
    {
        win_skip( "PM_QS_* flags not supported in PeekMessage\n" );
        goto done;
    }
    ok_sequence(WmUser, "WmUser", FALSE);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(0, QS_PAINT|QS_POSTMESSAGE|QS_KEY),
       "wrong qstatus %08x\n", qstatus);

    trace("signalling to send message\n");
    SetEvent(info.hevent[EV_SENDMSG]);
    WaitForSingleObject(info.hevent[EV_ACK], INFINITE);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(QS_SENDMESSAGE, QS_SENDMESSAGE|QS_PAINT|QS_POSTMESSAGE|QS_KEY),
       "wrong qstatus %08x\n", qstatus);

    msg.message = 0;
    ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE | PM_QS_POSTMESSAGE );
    ok(!ret,
       "PeekMessageA should have returned FALSE instead of msg %04x\n",
        msg.message);
    ok_sequence(WmUser, "WmUser", FALSE);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(0, QS_PAINT|QS_POSTMESSAGE|QS_KEY),
       "wrong qstatus %08x\n", qstatus);

    msg.message = 0;
    ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE | PM_QS_POSTMESSAGE);
    ok(ret && msg.message == WM_CHAR && msg.wParam == 'z',
       "got %d and %04x wParam %08lx instead of TRUE and WM_CHAR wParam 'z'\n",
       ret, msg.message, msg.wParam);
    ok_sequence(WmEmptySeq, "WmEmptySeq", FALSE);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(0, QS_PAINT|QS_KEY),
       "wrong qstatus %08x\n", qstatus);

    msg.message = 0;
    ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE | PM_QS_POSTMESSAGE);
    ok(!ret,
       "PeekMessageA should have returned FALSE instead of msg %04x\n",
        msg.message);
    ok_sequence(WmEmptySeq, "WmEmptySeq", FALSE);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(0, QS_PAINT|QS_KEY),
       "wrong qstatus %08x\n", qstatus);

    msg.message = 0;
    ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE | PM_QS_PAINT);
    ok(ret && msg.message == WM_PAINT,
       "got %d and %04x instead of TRUE and WM_PAINT\n", ret, msg.message);
    DispatchMessageA(&msg);
    ok_sequence(WmPaint, "WmPaint", FALSE);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(0, QS_KEY),
       "wrong qstatus %08x\n", qstatus);

    msg.message = 0;
    ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE | PM_QS_PAINT);
    ok(!ret,
       "PeekMessageA should have returned FALSE instead of msg %04x\n",
        msg.message);
    ok_sequence(WmEmptySeq, "WmEmptySeq", FALSE);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(0, QS_KEY),
       "wrong qstatus %08x\n", qstatus);

    trace("signalling to send message\n");
    SetEvent(info.hevent[EV_SENDMSG]);
    WaitForSingleObject(info.hevent[EV_ACK], INFINITE);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(QS_SENDMESSAGE, QS_SENDMESSAGE|QS_KEY),
       "wrong qstatus %08x\n", qstatus);

    PostMessageA(info.hwnd, WM_CHAR, 'z', 0);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(QS_POSTMESSAGE, QS_SENDMESSAGE|QS_POSTMESSAGE|QS_KEY),
       "wrong qstatus %08x\n", qstatus);

    msg.message = 0;
    ret = PeekMessageA(&msg, 0, WM_CHAR, WM_CHAR, PM_REMOVE);
    ok(ret && msg.message == WM_CHAR && msg.wParam == 'z',
       "got %d and %04x wParam %08lx instead of TRUE and WM_CHAR wParam 'z'\n",
       ret, msg.message, msg.wParam);
    ok_sequence(WmUser, "WmUser", FALSE);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(0, QS_KEY),
       "wrong qstatus %08x\n", qstatus);

    msg.message = 0;
    ret = PeekMessageA(&msg, 0, WM_CHAR, WM_CHAR, PM_REMOVE);
    ok(!ret,
       "PeekMessageA should have returned FALSE instead of msg %04x\n",
        msg.message);
    ok_sequence(WmEmptySeq, "WmEmptySeq", FALSE);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(0, QS_KEY),
       "wrong qstatus %08x\n", qstatus);

    PostMessageA(info.hwnd, WM_CHAR, 'z', 0);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(QS_POSTMESSAGE, QS_POSTMESSAGE|QS_KEY),
       "wrong qstatus %08x\n", qstatus);

    trace("signalling to send message\n");
    SetEvent(info.hevent[EV_SENDMSG]);
    WaitForSingleObject(info.hevent[EV_ACK], INFINITE);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(QS_SENDMESSAGE, QS_SENDMESSAGE|QS_POSTMESSAGE|QS_KEY),
       "wrong qstatus %08x\n", qstatus);

    msg.message = 0;
    ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE | (QS_KEY << 16));
    ok(!ret,
       "PeekMessageA should have returned FALSE instead of msg %04x\n",
        msg.message);
    ok_sequence(WmUser, "WmUser", FALSE);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(0, QS_POSTMESSAGE|QS_KEY),
       "wrong qstatus %08x\n", qstatus);

    msg.message = 0;
    if (qs_all_input & QS_RAWINPUT) /* use QS_RAWINPUT only if supported */
        ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE | (QS_RAWINPUT << 16));
    else /* workaround for a missing QS_RAWINPUT support */
        ret = PeekMessageA(&msg, 0, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE);
    ok(ret && msg.message == WM_KEYDOWN && msg.wParam == 'N',
       "got %d and %04x wParam %08lx instead of TRUE and WM_KEYDOWN wParam 'N'\n",
       ret, msg.message, msg.wParam);
    ok_sequence(WmKeyDownSkippedSeq, "WmKeyDownSkippedSeq", FALSE);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(0, QS_POSTMESSAGE|QS_KEY),
       "wrong qstatus %08x\n", qstatus);

    msg.message = 0;
    if (qs_all_input & QS_RAWINPUT) /* use QS_RAWINPUT only if supported */
        ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE | (QS_RAWINPUT << 16));
    else /* workaround for a missing QS_RAWINPUT support */
        ret = PeekMessageA(&msg, 0, WM_KEYUP, WM_KEYUP, PM_REMOVE);
    ok(ret && msg.message == WM_KEYUP && msg.wParam == 'N',
       "got %d and %04x wParam %08lx instead of TRUE and WM_KEYUP wParam 'N'\n",
       ret, msg.message, msg.wParam);
    ok_sequence(WmKeyUpSkippedSeq, "WmKeyUpSkippedSeq", FALSE);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(0, QS_POSTMESSAGE),
       "wrong qstatus %08x\n", qstatus);

    msg.message = 0;
    ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE | PM_QS_SENDMESSAGE);
    ok(!ret,
       "PeekMessageA should have returned FALSE instead of msg %04x\n",
        msg.message);
    ok_sequence(WmEmptySeq, "WmEmptySeq", FALSE);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(0, QS_POSTMESSAGE),
       "wrong qstatus %08x\n", qstatus);

    msg.message = 0;
    ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE);
    ok(ret && msg.message == WM_CHAR && msg.wParam == 'z',
       "got %d and %04x wParam %08lx instead of TRUE and WM_CHAR wParam 'z'\n",
       ret, msg.message, msg.wParam);
    ok_sequence(WmEmptySeq, "WmEmptySeq", FALSE);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == 0,
       "wrong qstatus %08x\n", qstatus);

    msg.message = 0;
    ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE);
    ok(!ret,
       "PeekMessageA should have returned FALSE instead of msg %04x\n",
        msg.message);
    ok_sequence(WmEmptySeq, "WmEmptySeq", FALSE);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == 0,
       "wrong qstatus %08x\n", qstatus);

    /* test whether presence of the quit flag in the queue affects
     * the queue state
     */
    PostQuitMessage(0x1234abcd);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(QS_POSTMESSAGE, QS_POSTMESSAGE),
       "wrong qstatus %08x\n", qstatus);

    PostMessageA(info.hwnd, WM_USER, 0, 0);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(QS_POSTMESSAGE, QS_POSTMESSAGE),
       "wrong qstatus %08x\n", qstatus);

    msg.message = 0;
    ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE);
    ok(ret && msg.message == WM_USER,
       "got %d and %04x instead of TRUE and WM_USER\n", ret, msg.message);
    ok_sequence(WmEmptySeq, "WmEmptySeq", FALSE);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(0, QS_POSTMESSAGE),
       "wrong qstatus %08x\n", qstatus);

    msg.message = 0;
    ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE);
    ok(ret && msg.message == WM_QUIT,
       "got %d and %04x instead of TRUE and WM_QUIT\n", ret, msg.message);
    ok(msg.wParam == 0x1234abcd, "got wParam %08lx instead of 0x1234abcd\n", msg.wParam);
    ok(msg.lParam == 0, "got lParam %08lx instead of 0\n", msg.lParam);
    ok_sequence(WmEmptySeq, "WmEmptySeq", FALSE);

    qstatus = GetQueueStatus(qs_all_input);
todo_wine {
    ok(qstatus == MAKELONG(0, QS_POSTMESSAGE),
       "wrong qstatus %08x\n", qstatus);
}

    msg.message = 0;
    ret = PeekMessageA(&msg, 0, 0, 0, PM_REMOVE);
    ok(!ret,
       "PeekMessageA should have returned FALSE instead of msg %04x\n",
        msg.message);
    ok_sequence(WmEmptySeq, "WmEmptySeq", FALSE);

    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == 0,
       "wrong qstatus %08x\n", qstatus);

    /* some GetMessage tests */

    keybd_event('N', 0, 0, 0);
    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(QS_KEY, QS_KEY), "wrong qstatus %08x\n", qstatus);

    PostMessageA(info.hwnd, WM_CHAR, 'z', 0);
    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(QS_POSTMESSAGE, QS_POSTMESSAGE|QS_KEY), "wrong qstatus %08x\n", qstatus);

    if (qstatus)
    {
        ret = GetMessageA( &msg, 0, 0, 0 );
        ok(ret && msg.message == WM_CHAR && msg.wParam == 'z',
           "got %d and %04x wParam %08lx instead of TRUE and WM_CHAR wParam 'z'\n",
           ret, msg.message, msg.wParam);
        qstatus = GetQueueStatus(qs_all_input);
        ok(qstatus == MAKELONG(0, QS_KEY), "wrong qstatus %08x\n", qstatus);
    }

    if (qstatus)
    {
        ret = GetMessageA( &msg, 0, 0, 0 );
        ok(ret && msg.message == WM_KEYDOWN && msg.wParam == 'N',
           "got %d and %04x wParam %08lx instead of TRUE and WM_KEYDOWN wParam 'N'\n",
           ret, msg.message, msg.wParam);
        ok_sequence(WmKeyDownSkippedSeq, "WmKeyDownSkippedSeq", FALSE);
        qstatus = GetQueueStatus(qs_all_input);
        ok(qstatus == 0, "wrong qstatus %08x\n", qstatus);
    }

    keybd_event('N', 0, 0, 0);
    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(QS_KEY, QS_KEY), "wrong qstatus %08x\n", qstatus);

    PostMessageA(info.hwnd, WM_CHAR, 'z', 0);
    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(QS_POSTMESSAGE, QS_POSTMESSAGE|QS_KEY), "wrong qstatus %08x\n", qstatus);

    if (qstatus & (QS_KEY << 16))
    {
        ret = GetMessageA( &msg, 0, WM_KEYDOWN, WM_KEYUP );
        ok(ret && msg.message == WM_KEYDOWN && msg.wParam == 'N',
           "got %d and %04x wParam %08lx instead of TRUE and WM_KEYDOWN wParam 'N'\n",
           ret, msg.message, msg.wParam);
        ok_sequence(WmKeyDownWasDownSkippedSeq, "WmKeyDownWasDownSkippedSeq", FALSE);
        qstatus = GetQueueStatus(qs_all_input);
        ok(qstatus == MAKELONG(0, QS_POSTMESSAGE), "wrong qstatus %08x\n", qstatus);
    }

    if (qstatus)
    {
        ret = GetMessageA( &msg, 0, WM_CHAR, WM_CHAR );
        ok(ret && msg.message == WM_CHAR && msg.wParam == 'z',
           "got %d and %04x wParam %08lx instead of TRUE and WM_CHAR wParam 'z'\n",
           ret, msg.message, msg.wParam);
        qstatus = GetQueueStatus(qs_all_input);
        ok(qstatus == 0, "wrong qstatus %08x\n", qstatus);
    }

    keybd_event('N', 0, KEYEVENTF_KEYUP, 0);
    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(QS_KEY, QS_KEY), "wrong qstatus %08x\n", qstatus);

    PostMessageA(info.hwnd, WM_CHAR, 'z', 0);
    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(QS_POSTMESSAGE, QS_POSTMESSAGE|QS_KEY), "wrong qstatus %08x\n", qstatus);

    trace("signalling to send message\n");
    SetEvent(info.hevent[EV_SENDMSG]);
    WaitForSingleObject(info.hevent[EV_ACK], INFINITE);
    qstatus = GetQueueStatus(qs_all_input);
    ok(qstatus == MAKELONG(QS_SENDMESSAGE, QS_SENDMESSAGE|QS_POSTMESSAGE|QS_KEY),
       "wrong qstatus %08x\n", qstatus);

    if (qstatus & (QS_KEY << 16))
    {
        ret = GetMessageA( &msg, 0, WM_KEYDOWN, WM_KEYUP );
        ok(ret && msg.message == WM_KEYUP && msg.wParam == 'N',
           "got %d and %04x wParam %08lx instead of TRUE and WM_KEYDOWN wParam 'N'\n",
           ret, msg.message, msg.wParam);
        ok_sequence(WmUserKeyUpSkippedSeq, "WmUserKeyUpSkippedSeq", FALSE);
        qstatus = GetQueueStatus(qs_all_input);
        ok(qstatus == MAKELONG(0, QS_POSTMESSAGE), "wrong qstatus %08x\n", qstatus);
    }

    if (qstatus)
    {
        ret = GetMessageA( &msg, 0, WM_CHAR, WM_CHAR );
        ok(ret && msg.message == WM_CHAR && msg.wParam == 'z',
           "got %d and %04x wParam %08lx instead of TRUE and WM_CHAR wParam 'z'\n",
           ret, msg.message, msg.wParam);
        qstatus = GetQueueStatus(qs_all_input);
        ok(qstatus == 0, "wrong qstatus %08x\n", qstatus);
    }
done:
    trace("signalling to exit\n");
    SetEvent(info.hevent[EV_STOP]);

    WaitForSingleObject(hthread, INFINITE);

    CloseHandle(hthread);
    CloseHandle(info.hevent[0]);
    CloseHandle(info.hevent[1]);
    CloseHandle(info.hevent[2]);

    DestroyWindow(info.hwnd);
}

static void wait_move_event(HWND hwnd, int x, int y)
{
    MSG msg;
    DWORD time;
    BOOL ret, go = FALSE;

    time = GetTickCount();
    while (GetTickCount() - time < 200 && !go) {
	ret = PeekMessageA(&msg, hwnd, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_NOREMOVE);
	go  = ret && msg.pt.x > x && msg.pt.y > y;
        if (!ret) MsgWaitForMultipleObjects( 0, NULL, FALSE, GetTickCount() - time, QS_ALLINPUT );
    }
}

#define STEP 5
static void test_PeekMessage2(void)
{
    HWND hwnd;
    BOOL ret;
    MSG msg;
    UINT message;
    DWORD time1, time2, time3;
    int x1, y1, x2, y2, x3, y3;
    POINT pos;

    time1 = time2 = time3 = 0;
    x1 = y1 = x2 = y2 = x3 = y3 = 0;

    /* Initialise window and make sure it is ready for events */
    hwnd = CreateWindowA("TestWindowClass", "PeekMessage2", WS_OVERLAPPEDWINDOW,
                        10, 10, 800, 800, NULL, NULL, NULL, NULL);
    assert(hwnd);
    trace("Window for test_PeekMessage2 %p\n", hwnd);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    SetFocus(hwnd);
    GetCursorPos(&pos);
    SetCursorPos(100, 100);
    mouse_event(MOUSEEVENTF_MOVE, -STEP, -STEP, 0, 0);
    flush_events();

    /* Do initial mousemove, wait until we can see it
       and then do our test peek with PM_NOREMOVE. */
    mouse_event(MOUSEEVENTF_MOVE, STEP, STEP, 0, 0);
    wait_move_event(hwnd, 100-STEP, 100-STEP);

    ret = PeekMessageA(&msg, hwnd, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_NOREMOVE);
    if (!ret)
    {
        skip( "queuing mouse events not supported\n" );
        goto done;
    }
    else
    {
	trace("1st move event: %04x %x %d %d\n", msg.message, msg.time, msg.pt.x, msg.pt.y);
	message = msg.message;
	time1 = msg.time;
	x1 = msg.pt.x;
	y1 = msg.pt.y;
        ok(message == WM_MOUSEMOVE, "message not WM_MOUSEMOVE, %04x instead\n", message);
    }

    /* Allow time to advance a bit, and then simulate the user moving their
     * mouse around. After that we peek again with PM_NOREMOVE.
     * Although the previous mousemove message was never removed, the
     * mousemove we now peek should reflect the recent mouse movements
     * because the input queue will merge the move events. */
    Sleep(100);
    mouse_event(MOUSEEVENTF_MOVE, STEP, STEP, 0, 0);
    wait_move_event(hwnd, x1, y1);

    ret = PeekMessageA(&msg, hwnd, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_NOREMOVE);
    ok(ret, "no message available\n");
    if (ret) {
	trace("2nd move event: %04x %x %d %d\n", msg.message, msg.time, msg.pt.x, msg.pt.y);
	message = msg.message;
	time2 = msg.time;
	x2 = msg.pt.x;
	y2 = msg.pt.y;
        ok(message == WM_MOUSEMOVE, "message not WM_MOUSEMOVE, %04x instead\n", message);
	ok(time2 > time1, "message time not advanced: %x %x\n", time1, time2);
	ok(x2 != x1 && y2 != y1, "coords not changed: (%d %d) (%d %d)\n", x1, y1, x2, y2);
    }

    /* Have another go, to drive the point home */
    Sleep(100);
    mouse_event(MOUSEEVENTF_MOVE, STEP, STEP, 0, 0);
    wait_move_event(hwnd, x2, y2);

    ret = PeekMessageA(&msg, hwnd, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_NOREMOVE);
    ok(ret, "no message available\n");
    if (ret) {
	trace("3rd move event: %04x %x %d %d\n", msg.message, msg.time, msg.pt.x, msg.pt.y);
	message = msg.message;
	time3 = msg.time;
	x3 = msg.pt.x;
	y3 = msg.pt.y;
        ok(message == WM_MOUSEMOVE, "message not WM_MOUSEMOVE, %04x instead\n", message);
	ok(time3 > time2, "message time not advanced: %x %x\n", time2, time3);
	ok(x3 != x2 && y3 != y2, "coords not changed: (%d %d) (%d %d)\n", x2, y2, x3, y3);
    }

done:
    DestroyWindow(hwnd);
    SetCursorPos(pos.x, pos.y);
    flush_events();
}

static INT_PTR CALLBACK wm_quit_dlg_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
{
    struct recvd_message msg;

    if (ignore_message( message )) return 0;

    msg.hwnd = hwnd;
    msg.message = message;
    msg.flags = sent|wparam|lparam;
    msg.wParam = wp;
    msg.lParam = lp;
    msg.descr = "dialog";
    add_message(&msg);

    switch (message)
    {
    case WM_INITDIALOG:
        PostMessageA(hwnd, WM_QUIT, 0x1234, 0x5678);
        PostMessageA(hwnd, WM_USER, 0xdead, 0xbeef);
        return 0;

    case WM_GETDLGCODE:
        return 0;

    case WM_USER:
        EndDialog(hwnd, 0);
        break;
    }

    return 1;
}

static const struct message WmQuitDialogSeq[] = {
    { HCBT_CREATEWND, hook },
    { WM_SETFONT, sent },
    { WM_INITDIALOG, sent },
    { WM_CHANGEUISTATE, sent|optional },
    { HCBT_DESTROYWND, hook },
    { 0x0090, sent|optional }, /* Vista */
    { WM_DESTROY, sent },
    { WM_NCDESTROY, sent },
    { 0 }
};

static const struct message WmStopQuitSeq[] = {
    { WM_DWMNCRENDERINGCHANGED, posted|optional },
    { WM_CLOSE, posted },
    { WM_QUIT, posted|wparam|lparam, 0x1234, 0 },
    { 0 }
};

static void test_quit_message(void)
{
    MSG msg;
    BOOL ret;

    /* test using PostQuitMessage */
    flush_events();
    PostQuitMessage(0xbeef);

    ret = PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE);
    ok(ret, "PeekMessage failed with error %d\n", GetLastError());
    ok(msg.message == WM_QUIT, "Received message 0x%04x instead of WM_QUIT\n", msg.message);
    ok(msg.wParam == 0xbeef, "wParam was 0x%lx instead of 0xbeef\n", msg.wParam);

    ret = PostThreadMessageA(GetCurrentThreadId(), WM_USER, 0, 0);
    ok(ret, "PostMessage failed with error %d\n", GetLastError());

    ret = GetMessageA(&msg, NULL, 0, 0);
    ok(ret > 0, "GetMessage failed with error %d\n", GetLastError());
    ok(msg.message == WM_USER, "Received message 0x%04x instead of WM_USER\n", msg.message);

    /* note: WM_QUIT message received after WM_USER message */
    ret = GetMessageA(&msg, NULL, 0, 0);
    ok(!ret, "GetMessage return %d with error %d instead of FALSE\n", ret, GetLastError());
    ok(msg.message == WM_QUIT, "Received message 0x%04x instead of WM_QUIT\n", msg.message);
    ok(msg.wParam == 0xbeef, "wParam was 0x%lx instead of 0xbeef\n", msg.wParam);

    ret = PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE);
    ok( !ret || msg.message != WM_QUIT, "Received WM_QUIT again\n" );

    /* now test with PostThreadMessage - different behaviour! */
    PostThreadMessageA(GetCurrentThreadId(), WM_QUIT, 0xdead, 0);

    ret = PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE);
    ok(ret, "PeekMessage failed with error %d\n", GetLastError());
    ok(msg.message == WM_QUIT, "Received message 0x%04x instead of WM_QUIT\n", msg.message);
    ok(msg.wParam == 0xdead, "wParam was 0x%lx instead of 0xdead\n", msg.wParam);

    ret = PostThreadMessageA(GetCurrentThreadId(), WM_USER, 0, 0);
    ok(ret, "PostMessage failed with error %d\n", GetLastError());

    /* note: we receive the WM_QUIT message first this time */
    ret = GetMessageA(&msg, NULL, 0, 0);
    ok(!ret, "GetMessage return %d with error %d instead of FALSE\n", ret, GetLastError());
    ok(msg.message == WM_QUIT, "Received message 0x%04x instead of WM_QUIT\n", msg.message);
    ok(msg.wParam == 0xdead, "wParam was 0x%lx instead of 0xdead\n", msg.wParam);

    ret = GetMessageA(&msg, NULL, 0, 0);
    ok(ret > 0, "GetMessage failed with error %d\n", GetLastError());
    ok(msg.message == WM_USER, "Received message 0x%04x instead of WM_USER\n", msg.message);

    flush_events();
    flush_sequence();
    ret = DialogBoxParamA(GetModuleHandleA(NULL), "TEST_EMPTY_DIALOG", 0, wm_quit_dlg_proc, 0);
    ok(ret == 1, "expected 1, got %d\n", ret);
    ok_sequence(WmQuitDialogSeq, "WmQuitDialogSeq", FALSE);
    memset(&msg, 0xab, sizeof(msg));
    ret = PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE);
    ok(ret, "PeekMessage failed\n");
    ok(msg.message == WM_QUIT, "Received message 0x%04x instead of WM_QUIT\n", msg.message);
    ok(msg.wParam == 0x1234, "wParam was 0x%lx instead of 0x1234\n", msg.wParam);
    ok(msg.lParam == 0, "lParam was 0x%lx instead of 0\n", msg.lParam);

    /* Check what happens to a WM_QUIT message posted to a window that gets
     * destroyed.
     */
    CreateWindowExA(0, "StopQuitClass", "Stop Quit Test", WS_OVERLAPPEDWINDOW,
                    0, 0, 100, 100, NULL, NULL, NULL, NULL);
    flush_sequence();
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
    {
        struct recvd_message rmsg;
        rmsg.hwnd = msg.hwnd;
        rmsg.message = msg.message;
        rmsg.flags = posted|wparam|lparam;
        rmsg.wParam = msg.wParam;
        rmsg.lParam = msg.lParam;
        rmsg.descr = "stop/quit";
        if (msg.message == WM_QUIT)
            /* The hwnd can only be checked here */
            ok(!msg.hwnd, "The WM_QUIT hwnd was %p instead of NULL\n", msg.hwnd);
        add_message(&rmsg);
        DispatchMessageA(&msg);
    }
    ok_sequence(WmStopQuitSeq, "WmStopQuitSeq", FALSE);
}

static const struct message WmMouseHoverSeq[] = {
    { WM_MOUSEACTIVATE, sent|optional },  /* we can get those when moving the mouse in focus-follow-mouse mode under X11 */
    { WM_MOUSEACTIVATE, sent|optional },
    { WM_TIMER, sent|optional }, /* XP sends it */
    { WM_SYSTIMER, sent },
    { WM_MOUSEHOVER, sent|wparam, 0 },
    { 0 }
};

static void pump_msg_loop_timeout(DWORD timeout, BOOL inject_mouse_move)
{
    MSG msg;
    DWORD start_ticks, end_ticks;

    start_ticks = GetTickCount();
    /* add some deviation (50%) to cover not expected delays */
    start_ticks += timeout / 2;

    do
    {
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
        {
            /* Timer proc messages are not dispatched to the window proc,
             * and therefore not logged.
             */
            if ((msg.message == WM_TIMER || msg.message == WM_SYSTIMER) && msg.hwnd)
            {
                struct recvd_message s_msg;

                s_msg.hwnd = msg.hwnd;
                s_msg.message = msg.message;
                s_msg.flags = sent|wparam|lparam;
                s_msg.wParam = msg.wParam;
                s_msg.lParam = msg.lParam;
                s_msg.descr = "msg_loop";
                add_message(&s_msg);
            }
            DispatchMessageA(&msg);
        }

        end_ticks = GetTickCount();

        /* inject WM_MOUSEMOVE to see how it changes tracking */
        if (inject_mouse_move && start_ticks + timeout / 2 >= end_ticks)
        {
            mouse_event(MOUSEEVENTF_MOVE, -1, 0, 0, 0);
            mouse_event(MOUSEEVENTF_MOVE, 1, 0, 0, 0);

            inject_mouse_move = FALSE;
        }
    } while (start_ticks + timeout >= end_ticks);
}

static void test_TrackMouseEvent(void)
{
    TRACKMOUSEEVENT tme;
    BOOL ret;
    HWND hwnd, hchild;
    RECT rc_parent, rc_child;
    UINT default_hover_time, hover_width = 0, hover_height = 0;

#define track_hover(track_hwnd, track_hover_time) \
    tme.cbSize = sizeof(tme); \
    tme.dwFlags = TME_HOVER; \
    tme.hwndTrack = track_hwnd; \
    tme.dwHoverTime = track_hover_time; \
    SetLastError(0xdeadbeef); \
    ret = pTrackMouseEvent(&tme); \
    ok(ret, "TrackMouseEvent(TME_HOVER) error %d\n", GetLastError())

#define track_query(expected_track_flags, expected_track_hwnd, expected_hover_time) \
    tme.cbSize = sizeof(tme); \
    tme.dwFlags = TME_QUERY; \
    tme.hwndTrack = (HWND)0xdeadbeef; \
    tme.dwHoverTime = 0xdeadbeef; \
    SetLastError(0xdeadbeef); \
    ret = pTrackMouseEvent(&tme); \
    ok(ret, "TrackMouseEvent(TME_QUERY) error %d\n", GetLastError());\
    ok(tme.cbSize == sizeof(tme), "wrong tme.cbSize %u\n", tme.cbSize); \
    ok(tme.dwFlags == (expected_track_flags), \
       "wrong tme.dwFlags %08x, expected %08x\n", tme.dwFlags, (expected_track_flags)); \
    ok(tme.hwndTrack == (expected_track_hwnd), \
       "wrong tme.hwndTrack %p, expected %p\n", tme.hwndTrack, (expected_track_hwnd)); \
    ok(tme.dwHoverTime == (expected_hover_time), \
       "wrong tme.dwHoverTime %u, expected %u\n", tme.dwHoverTime, (expected_hover_time))

#define track_hover_cancel(track_hwnd) \
    tme.cbSize = sizeof(tme); \
    tme.dwFlags = TME_HOVER | TME_CANCEL; \
    tme.hwndTrack = track_hwnd; \
    tme.dwHoverTime = 0xdeadbeef; \
    SetLastError(0xdeadbeef); \
    ret = pTrackMouseEvent(&tme); \
    ok(ret, "TrackMouseEvent(TME_HOVER | TME_CANCEL) error %d\n", GetLastError())

    default_hover_time = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = SystemParametersInfoA(SPI_GETMOUSEHOVERTIME, 0, &default_hover_time, 0);
    ok(ret || broken(GetLastError() == 0xdeadbeef),  /* win9x */
       "SystemParametersInfo(SPI_GETMOUSEHOVERTIME) error %u\n", GetLastError());
    if (!ret) default_hover_time = 400;
    trace("SPI_GETMOUSEHOVERTIME returned %u ms\n", default_hover_time);

    SetLastError(0xdeadbeef);
    ret = SystemParametersInfoA(SPI_GETMOUSEHOVERWIDTH, 0, &hover_width, 0);
    ok(ret || broken(GetLastError() == 0xdeadbeef),  /* win9x */
       "SystemParametersInfo(SPI_GETMOUSEHOVERWIDTH) error %u\n", GetLastError());
    if (!ret) hover_width = 4;
    SetLastError(0xdeadbeef);
    ret = SystemParametersInfoA(SPI_GETMOUSEHOVERHEIGHT, 0, &hover_height, 0);
    ok(ret || broken(GetLastError() == 0xdeadbeef),  /* win9x */
       "SystemParametersInfo(SPI_GETMOUSEHOVERHEIGHT) error %u\n", GetLastError());
    if (!ret) hover_height = 4;
    trace("hover rect is %u x %d\n", hover_width, hover_height);

    hwnd = CreateWindowExA(0, "TestWindowClass", NULL,
			  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			  CW_USEDEFAULT, CW_USEDEFAULT, 300, 300, 0,
			  NULL, NULL, 0);
    assert(hwnd);

    hchild = CreateWindowExA(0, "TestWindowClass", NULL,
			  WS_CHILD | WS_BORDER | WS_VISIBLE,
			  50, 50, 200, 200, hwnd,
			  NULL, NULL, 0);
    assert(hchild);

    SetWindowPos( hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE );
    flush_events();
    flush_sequence();

    tme.cbSize = 0;
    tme.dwFlags = TME_QUERY;
    tme.hwndTrack = (HWND)0xdeadbeef;
    tme.dwHoverTime = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pTrackMouseEvent(&tme);
    ok(!ret, "TrackMouseEvent should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER || broken(GetLastError() == 0xdeadbeef),
       "not expected error %u\n", GetLastError());

    tme.cbSize = sizeof(tme);
    tme.dwFlags = TME_HOVER;
    tme.hwndTrack = (HWND)0xdeadbeef;
    tme.dwHoverTime = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pTrackMouseEvent(&tme);
    ok(!ret, "TrackMouseEvent should fail\n");
    ok(GetLastError() == ERROR_INVALID_WINDOW_HANDLE || broken(GetLastError() == 0xdeadbeef),
       "not expected error %u\n", GetLastError());

    tme.cbSize = sizeof(tme);
    tme.dwFlags = TME_HOVER | TME_CANCEL;
    tme.hwndTrack = (HWND)0xdeadbeef;
    tme.dwHoverTime = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pTrackMouseEvent(&tme);
    ok(!ret, "TrackMouseEvent should fail\n");
    ok(GetLastError() == ERROR_INVALID_WINDOW_HANDLE || broken(GetLastError() == 0xdeadbeef),
       "not expected error %u\n", GetLastError());

    GetWindowRect(hwnd, &rc_parent);
    GetWindowRect(hchild, &rc_child);
    SetCursorPos(rc_child.left - 10, rc_child.top - 10);

    /* Process messages so that the system updates its internal current
     * window and hittest, otherwise TrackMouseEvent calls don't have any
     * effect.
     */
    flush_events();
    flush_sequence();

    track_query(0, NULL, 0);
    track_hover(hchild, 0);
    track_query(0, NULL, 0);

    flush_events();
    flush_sequence();

    track_hover(hwnd, 0);
    tme.cbSize = sizeof(tme);
    tme.dwFlags = TME_QUERY;
    tme.hwndTrack = (HWND)0xdeadbeef;
    tme.dwHoverTime = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pTrackMouseEvent(&tme);
    ok(ret, "TrackMouseEvent(TME_QUERY) error %d\n", GetLastError());
    ok(tme.cbSize == sizeof(tme), "wrong tme.cbSize %u\n", tme.cbSize);
    if (!tme.dwFlags)
    {
        skip( "Cursor not inside window, skipping TrackMouseEvent tests\n" );
        DestroyWindow( hwnd );
        return;
    }
    ok(tme.dwFlags == TME_HOVER, "wrong tme.dwFlags %08x, expected TME_HOVER\n", tme.dwFlags);
    ok(tme.hwndTrack == hwnd, "wrong tme.hwndTrack %p, expected %p\n", tme.hwndTrack, hwnd);
    ok(tme.dwHoverTime == default_hover_time, "wrong tme.dwHoverTime %u, expected %u\n",
       tme.dwHoverTime, default_hover_time);

    pump_msg_loop_timeout(default_hover_time, FALSE);
    ok_sequence(WmMouseHoverSeq, "WmMouseHoverSeq", FALSE);

    track_query(0, NULL, 0);

    track_hover(hwnd, HOVER_DEFAULT);
    track_query(TME_HOVER, hwnd, default_hover_time);

    Sleep(default_hover_time / 2);
    mouse_event(MOUSEEVENTF_MOVE, -1, 0, 0, 0);
    mouse_event(MOUSEEVENTF_MOVE, 1, 0, 0, 0);

    track_query(TME_HOVER, hwnd, default_hover_time);

    pump_msg_loop_timeout(default_hover_time, FALSE);
    ok_sequence(WmMouseHoverSeq, "WmMouseHoverSeq", FALSE);

    track_query(0, NULL, 0);

    track_hover(hwnd, HOVER_DEFAULT);
    track_query(TME_HOVER, hwnd, default_hover_time);

    pump_msg_loop_timeout(default_hover_time, TRUE);
    ok_sequence(WmMouseHoverSeq, "WmMouseHoverSeq", FALSE);

    track_query(0, NULL, 0);

    track_hover(hwnd, HOVER_DEFAULT);
    track_query(TME_HOVER, hwnd, default_hover_time);
    track_hover_cancel(hwnd);

    DestroyWindow(hwnd);

#undef track_hover
#undef track_query
#undef track_hover_cancel
}


static const struct message WmSetWindowRgn[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_NCPAINT, sent|optional }, /* wparam != 1 */
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message WmSetWindowRgn_no_redraw[] = {
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE|SWP_NOREDRAW },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE|SWP_NOREDRAW },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { 0 }
};

static const struct message WmSetWindowRgn_clear[] = {
    { WM_WINDOWPOSCHANGING, sent/*|wparam*/, SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE/*|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE only on some Windows versions */ },
    { WM_NCCALCSIZE, sent|wparam, 1 },
    { WM_NCPAINT, sent|optional },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional }, /* FIXME: remove optional once Wine is fixed */
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|wparam|optional, 1 },
    { WM_NCPAINT, sent|optional },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    { WM_WINDOWPOSCHANGING, sent|optional },
    { WM_NCCALCSIZE, sent|optional|wparam, 1 },
    { WM_NCPAINT, sent|optional },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|optional|wparam, SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCCALCSIZE, sent|optional|wparam, 1 },
    { WM_NCPAINT, sent|optional },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { 0 }
};

static void test_SetWindowRgn(void)
{
    HRGN hrgn;
    HWND hwnd = CreateWindowExA(0, "TestWindowClass", "Test overlapped", WS_OVERLAPPEDWINDOW,
                                100, 100, 200, 200, 0, 0, 0, NULL);
    ok( hwnd != 0, "Failed to create overlapped window\n" );

    ShowWindow( hwnd, SW_SHOW );
    UpdateWindow( hwnd );
    flush_events();
    flush_sequence();

    trace("testing SetWindowRgn\n");
    hrgn = CreateRectRgn( 0, 0, 150, 150 );
    SetWindowRgn( hwnd, hrgn, TRUE );
    ok_sequence( WmSetWindowRgn, "WmSetWindowRgn", FALSE );

    hrgn = CreateRectRgn( 30, 30, 160, 160 );
    SetWindowRgn( hwnd, hrgn, FALSE );
    ok_sequence( WmSetWindowRgn_no_redraw, "WmSetWindowRgn_no_redraw", FALSE );

    hrgn = CreateRectRgn( 0, 0, 180, 180 );
    SetWindowRgn( hwnd, hrgn, TRUE );
    ok_sequence( WmSetWindowRgn, "WmSetWindowRgn2", FALSE );

    SetWindowRgn( hwnd, 0, TRUE );
    ok_sequence( WmSetWindowRgn_clear, "WmSetWindowRgn_clear", FALSE );

    DestroyWindow( hwnd );
}

/*************************** ShowWindow() test ******************************/
static const struct message WmShowNormal[] = {
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE },
    { HCBT_ACTIVATE, hook },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE }, /* win2003 doesn't send it */
    { HCBT_SETFOCUS, hook },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { 0 }
};
static const struct message WmShow[] = {
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE },
    { HCBT_ACTIVATE, hook|optional }, /* win2000 doesn't send it */
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE }, /* win2000 doesn't send it */
    { HCBT_SETFOCUS, hook|optional }, /* win2000 doesn't send it */
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { 0 }
};
static const struct message WmShowNoActivate_1[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_SHOWNOACTIVATE },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_STATECHANGED, 0, SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_STATECHANGED, 0, SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_MOVE, sent|defwinproc|optional },
    { WM_SIZE, sent|wparam|defwinproc, SIZE_RESTORED },
    { 0 }
};
static const struct message WmShowNoActivate_2[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_SHOWNOACTIVATE },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOCOPYBITS|SWP_STATECHANGED, 0, SWP_NOACTIVATE },
    { HCBT_ACTIVATE, hook|optional },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE },
    { HCBT_SETFOCUS, hook|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOCOPYBITS|SWP_STATECHANGED, 0, SWP_NOACTIVATE },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|wparam|defwinproc, SIZE_RESTORED },
    { HCBT_SETFOCUS, hook|optional },
    { HCBT_ACTIVATE, hook|optional }, /* win2003 doesn't send it */
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE }, /* win2003 doesn't send it */
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { HCBT_SETFOCUS, hook|optional }, /* win2003 doesn't send it */
    { 0 }
};
static const struct message WmShowNA_1[] = {
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { 0 }
};
static const struct message WmShowNA_2[] = {
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { 0 }
};
static const struct message WmRestore_1[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_RESTORE },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_NOCOPYBITS|SWP_STATECHANGED },
    { HCBT_ACTIVATE, hook|optional }, /* win2000 doesn't send it */
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE }, /* win2000 doesn't send it */
    { HCBT_SETFOCUS, hook|optional }, /* win2000 doesn't send it */
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_NOCOPYBITS|SWP_STATECHANGED },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|wparam|defwinproc, SIZE_RESTORED },
    { HCBT_SETFOCUS, hook|optional }, /* win2000 sends it */
    { 0 }
};
static const struct message WmRestore_2[] = {
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE },
    { HCBT_ACTIVATE, hook|optional }, /* win2000 doesn't send it */
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE }, /* win2000 doesn't send it */
    { HCBT_SETFOCUS, hook|optional }, /* win2000 doesn't send it */
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { 0 }
};
static const struct message WmRestore_3[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_RESTORE },
    { WM_GETMINMAXINFO, sent },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_NOCOPYBITS|SWP_STATECHANGED },
    { HCBT_ACTIVATE, hook|optional }, /* win2003 doesn't send it */
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE }, /* win2003 doesn't send it */
    { HCBT_SETFOCUS, hook|optional }, /* win2003 doesn't send it */
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_NOCOPYBITS|SWP_STATECHANGED },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|wparam|defwinproc, SIZE_MAXIMIZED },
    { HCBT_SETFOCUS, hook|optional }, /* win2003 sends it */
    { 0 }
};
static const struct message WmRestore_4[] = {
    { HCBT_MINMAX, hook|lparam|optional, 0, SW_RESTORE },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_FRAMECHANGED|SWP_STATECHANGED, 0, SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_FRAMECHANGED|SWP_STATECHANGED, 0, SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_MOVE, sent|defwinproc|optional },
    { WM_SIZE, sent|wparam|defwinproc|optional, SIZE_RESTORED },
    { 0 }
};
static const struct message WmRestore_5[] = {
    { HCBT_MINMAX, hook|lparam|optional, 0, SW_SHOWNORMAL },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_FRAMECHANGED|SWP_STATECHANGED, 0, SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE|SWP_NOSIZE|SWP_NOMOVE },
    { HCBT_ACTIVATE, hook|optional },
    { HCBT_SETFOCUS, hook|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_FRAMECHANGED|SWP_STATECHANGED, 0, SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_MOVE, sent|defwinproc|optional },
    { WM_SIZE, sent|wparam|defwinproc|optional, SIZE_RESTORED },
    { 0 }
};
static const struct message WmHide_1[] = {
    { WM_SHOWWINDOW, sent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOMOVE, 0, SWP_NOACTIVATE },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE, 0, SWP_NOACTIVATE },
    { HCBT_ACTIVATE, hook|optional },
    { HCBT_SETFOCUS, hook|optional }, /* win2000 sends it */
    { 0 }
};
static const struct message WmHide_2[] = {
    { WM_SHOWWINDOW, sent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent /*|wparam, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE*/ }, /* win2000 doesn't add SWP_NOACTIVATE */
    { WM_WINDOWPOSCHANGED, sent /*|wparam, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE*/ }, /* win2000 doesn't add SWP_NOACTIVATE */
    { HCBT_ACTIVATE, hook|optional },
    { 0 }
};
static const struct message WmHide_3[] = {
    { WM_SHOWWINDOW, sent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOMOVE },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { HCBT_SETFOCUS, hook|optional },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { 0 }
};
static const struct message WmShowMinimized_1[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_SHOWMINIMIZED },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_NOCOPYBITS|SWP_STATECHANGED },
    { HCBT_ACTIVATE, hook|optional }, /* win2000 doesn't send it */
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE }, /* win2000 doesn't send it */
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_NOCOPYBITS|SWP_STATECHANGED },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|wparam|lparam|defwinproc, SIZE_MINIMIZED, 0 },
    { 0 }
};
static const struct message WmMinimize_1[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_MINIMIZE },
    { HCBT_SETFOCUS, hook|optional }, /* win2000 doesn't send it */
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOCOPYBITS|SWP_STATECHANGED },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOCOPYBITS|SWP_STATECHANGED },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|wparam|lparam|defwinproc, SIZE_MINIMIZED, 0 },
    { 0 }
};
static const struct message WmMinimize_2[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_MINIMIZE },
    { HCBT_SETFOCUS, hook|optional },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOCOPYBITS|SWP_STATECHANGED, 0, SWP_NOACTIVATE },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOCOPYBITS|SWP_STATECHANGED, 0, SWP_NOACTIVATE },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|wparam|lparam|defwinproc, SIZE_MINIMIZED, 0 },
    { 0 }
};
static const struct message WmMinimize_3[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_MINIMIZE },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOCOPYBITS|SWP_STATECHANGED, 0, SWP_NOACTIVATE },
    { HCBT_ACTIVATE, hook|optional },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOCOPYBITS|SWP_STATECHANGED, 0, SWP_NOACTIVATE },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|wparam|lparam|defwinproc, SIZE_MINIMIZED, 0 },
    { 0 }
};
static const struct message WmShowMinNoActivate[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_SHOWMINNOACTIVE },
    { WM_WINDOWPOSCHANGING, sent },
    { WM_WINDOWPOSCHANGED, sent },
    { WM_MOVE, sent|defwinproc|optional },
    { WM_SIZE, sent|wparam|lparam|defwinproc|optional, SIZE_MINIMIZED, 0 },
    { 0 }
};
static const struct message WmMinMax_1[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_SHOWMINIMIZED },
    { 0 }
};
static const struct message WmMinMax_2[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_SHOWMAXIMIZED },
    { WM_GETMINMAXINFO, sent|optional },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_FRAMECHANGED|SWP_STATECHANGED },
    { HCBT_ACTIVATE, hook|optional },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE },
    { HCBT_SETFOCUS, hook|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_FRAMECHANGED|SWP_STATECHANGED, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_MOVE, sent|defwinproc|optional },
    { WM_SIZE, sent|wparam|defwinproc|optional, SIZE_MAXIMIZED },
    { HCBT_SETFOCUS, hook|optional },
    { 0 }
};
static const struct message WmMinMax_3[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_MINIMIZE },
    { HCBT_SETFOCUS, hook|optional },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOCOPYBITS|SWP_STATECHANGED },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOCOPYBITS|SWP_STATECHANGED },
    { WM_MOVE, sent|defwinproc|optional },
    { WM_SIZE, sent|wparam|lparam|defwinproc|optional, SIZE_MINIMIZED, 0 },
    { 0 }
};
static const struct message WmMinMax_4[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_SHOWMINNOACTIVE },
    { 0 }
};
static const struct message WmShowMaximized_1[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_SHOWMAXIMIZED },
    { WM_GETMINMAXINFO, sent },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_NOCOPYBITS|SWP_STATECHANGED },
    { HCBT_ACTIVATE, hook|optional }, /* win2000 doesn't send it */
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE }, /* win2000 doesn't send it */
    { HCBT_SETFOCUS, hook|optional }, /* win2000 doesn't send it */
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_FRAMECHANGED|SWP_NOCOPYBITS|SWP_STATECHANGED },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|wparam|defwinproc, SIZE_MAXIMIZED },
    { HCBT_SETFOCUS, hook|optional }, /* win2003 sends it */
    { 0 }
};
static const struct message WmShowMaximized_2[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_SHOWMAXIMIZED },
    { WM_GETMINMAXINFO, sent },
    { WM_WINDOWPOSCHANGING, sent|optional },
    { HCBT_ACTIVATE, hook|optional },
    { WM_WINDOWPOSCHANGED, sent|optional },
    { WM_MOVE, sent|optional }, /* Win9x doesn't send it */
    { WM_SIZE, sent|wparam|optional, SIZE_MAXIMIZED }, /* Win9x doesn't send it */
    { WM_WINDOWPOSCHANGING, sent|optional },
    { HCBT_SETFOCUS, hook|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_FRAMECHANGED|SWP_NOCOPYBITS|SWP_STATECHANGED },
    { WM_MOVE, sent|defwinproc },
    { WM_SIZE, sent|wparam|defwinproc, SIZE_MAXIMIZED },
    { HCBT_SETFOCUS, hook|optional },
    { 0 }
};
static const struct message WmShowMaximized_3[] = {
    { HCBT_MINMAX, hook|lparam, 0, SW_SHOWMAXIMIZED },
    { WM_GETMINMAXINFO, sent|optional },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_FRAMECHANGED|SWP_STATECHANGED, 0, SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE|SWP_NOSIZE|SWP_NOMOVE },
    { HCBT_ACTIVATE, hook|optional }, /* win2000 doesn't send it */
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE }, /* win2000 doesn't send it */
    { HCBT_SETFOCUS, hook|optional }, /* win2000 doesn't send it */
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_FRAMECHANGED|SWP_STATECHANGED, 0, SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_MOVE, sent|defwinproc|optional },
    { WM_SIZE, sent|wparam|defwinproc, SIZE_MAXIMIZED },
    { 0 }
};

static void test_ShowWindow(void)
{
    /* ShowWindow commands in random order */
    static const struct
    {
        INT cmd; /* ShowWindow command */
        LPARAM ret; /* ShowWindow return value */
        DWORD style; /* window style after the command */
        const struct message *msg; /* message sequence the command produces */
        INT wp_cmd, wp_flags; /* window placement after the command */
        POINT wp_min, wp_max; /* window placement after the command */
        BOOL todo_msg; /* message sequence doesn't match what Wine does */
    } sw[] =
    {
/*  1 */ { SW_SHOWNORMAL, FALSE, WS_VISIBLE, WmShowNormal,
           SW_SHOWNORMAL, 0, {-1,-1}, {-1,-1}, FALSE },
/*  2 */ { SW_SHOWNORMAL, TRUE, WS_VISIBLE, WmEmptySeq,
           SW_SHOWNORMAL, 0, {-1,-1}, {-1,-1}, FALSE },
/*  3 */ { SW_HIDE, TRUE, 0, WmHide_1,
           SW_SHOWNORMAL, 0, {-1,-1}, {-1,-1}, FALSE },
/*  4 */ { SW_HIDE, FALSE, 0, WmEmptySeq,
           SW_SHOWNORMAL, 0, {-1,-1}, {-1,-1}, FALSE },
/*  5 */ { SW_SHOWMINIMIZED, FALSE, WS_VISIBLE|WS_MINIMIZE, WmShowMinimized_1,
           SW_SHOWMINIMIZED, 0, {-32000,-32000}, {-1,-1}, FALSE },
/*  6 */ { SW_SHOWMINIMIZED, TRUE, WS_VISIBLE|WS_MINIMIZE, WmMinMax_1,
           SW_SHOWMINIMIZED, 0, {-32000,-32000}, {-1,-1}, FALSE },
/*  7 */ { SW_HIDE, TRUE, WS_MINIMIZE, WmHide_1,
           SW_SHOWMINIMIZED, 0, {-32000,-32000}, {-1,-1}, FALSE },
/*  8 */ { SW_HIDE, FALSE, WS_MINIMIZE, WmEmptySeq,
           SW_SHOWMINIMIZED, 0, {-32000,-32000}, {-1,-1}, FALSE },
/*  9 */ { SW_SHOWMAXIMIZED, FALSE, WS_VISIBLE|WS_MAXIMIZE, WmShowMaximized_1,
           SW_SHOWMAXIMIZED, WPF_RESTORETOMAXIMIZED, {-32000,-32000}, {-1,-1}, FALSE },
/* 10 */ { SW_SHOWMAXIMIZED, TRUE, WS_VISIBLE|WS_MAXIMIZE, WmMinMax_2,
           SW_SHOWMAXIMIZED, WPF_RESTORETOMAXIMIZED, {-32000,-32000}, {-1,-1}, FALSE },
/* 11 */ { SW_HIDE, TRUE, WS_MAXIMIZE, WmHide_1,
           SW_SHOWMAXIMIZED, WPF_RESTORETOMAXIMIZED, {-32000,-32000}, {-1,-1}, FALSE },
/* 12 */ { SW_HIDE, FALSE, WS_MAXIMIZE, WmEmptySeq,
           SW_SHOWMAXIMIZED, WPF_RESTORETOMAXIMIZED, {-32000,-32000}, {-1,-1}, FALSE },
/* 13 */ { SW_SHOWNOACTIVATE, FALSE, WS_VISIBLE, WmShowNoActivate_1,
           SW_SHOWNORMAL, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 14 */ { SW_SHOWNOACTIVATE, TRUE, WS_VISIBLE, WmEmptySeq,
           SW_SHOWNORMAL, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 15 */ { SW_HIDE, TRUE, 0, WmHide_2,
           SW_SHOWNORMAL, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 16 */ { SW_HIDE, FALSE, 0, WmEmptySeq,
           SW_SHOWNORMAL, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 17 */ { SW_SHOW, FALSE, WS_VISIBLE, WmShow,
           SW_SHOWNORMAL, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 18 */ { SW_SHOW, TRUE, WS_VISIBLE, WmEmptySeq,
           SW_SHOWNORMAL, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 19 */ { SW_MINIMIZE, TRUE, WS_VISIBLE|WS_MINIMIZE, WmMinimize_1,
           SW_SHOWMINIMIZED, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 20 */ { SW_MINIMIZE, TRUE, WS_VISIBLE|WS_MINIMIZE, WmMinMax_3,
           SW_SHOWMINIMIZED, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 21 */ { SW_HIDE, TRUE, WS_MINIMIZE, WmHide_2,
           SW_SHOWMINIMIZED, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 22 */ { SW_SHOWMINNOACTIVE, FALSE, WS_VISIBLE|WS_MINIMIZE, WmShowMinNoActivate,
           SW_SHOWMINIMIZED, 0, {-32000,-32000}, {-1,-1}, TRUE },
/* 23 */ { SW_SHOWMINNOACTIVE, TRUE, WS_VISIBLE|WS_MINIMIZE, WmMinMax_4,
           SW_SHOWMINIMIZED, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 24 */ { SW_HIDE, TRUE, WS_MINIMIZE, WmHide_2,
           SW_SHOWMINIMIZED, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 25 */ { SW_HIDE, FALSE, WS_MINIMIZE, WmEmptySeq,
           SW_SHOWMINIMIZED, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 26 */ { SW_SHOWNA, FALSE, WS_VISIBLE|WS_MINIMIZE, WmShowNA_1,
           SW_SHOWMINIMIZED, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 27 */ { SW_SHOWNA, TRUE, WS_VISIBLE|WS_MINIMIZE, WmShowNA_2,
           SW_SHOWMINIMIZED, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 28 */ { SW_HIDE, TRUE, WS_MINIMIZE, WmHide_2,
           SW_SHOWMINIMIZED, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 29 */ { SW_HIDE, FALSE, WS_MINIMIZE, WmEmptySeq,
           SW_SHOWMINIMIZED, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 30 */ { SW_RESTORE, FALSE, WS_VISIBLE, WmRestore_1,
           SW_SHOWNORMAL, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 31 */ { SW_RESTORE, TRUE, WS_VISIBLE, WmEmptySeq,
           SW_SHOWNORMAL, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 32 */ { SW_HIDE, TRUE, 0, WmHide_3,
           SW_SHOWNORMAL, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 33 */ { SW_HIDE, FALSE, 0, WmEmptySeq,
           SW_SHOWNORMAL, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 34 */ { SW_NORMALNA, FALSE, 0, WmEmptySeq, /* what does this mean?! */
           SW_SHOWNORMAL, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 35 */ { SW_NORMALNA, FALSE, 0, WmEmptySeq,
           SW_SHOWNORMAL, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 36 */ { SW_HIDE, FALSE, 0, WmEmptySeq,
           SW_SHOWNORMAL, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 37 */ { SW_RESTORE, FALSE, WS_VISIBLE, WmRestore_2,
           SW_SHOWNORMAL, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 38 */ { SW_RESTORE, TRUE, WS_VISIBLE, WmEmptySeq,
           SW_SHOWNORMAL, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 39 */ { SW_SHOWNOACTIVATE, TRUE, WS_VISIBLE, WmEmptySeq,
           SW_SHOWNORMAL, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 40 */ { SW_MINIMIZE, TRUE, WS_VISIBLE|WS_MINIMIZE, WmMinimize_2,
           SW_SHOWMINIMIZED, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 41 */ { SW_MINIMIZE, TRUE, WS_VISIBLE|WS_MINIMIZE, WmMinMax_3,
           SW_SHOWMINIMIZED, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 42 */ { SW_SHOWMAXIMIZED, TRUE, WS_VISIBLE|WS_MAXIMIZE, WmShowMaximized_2,
           SW_SHOWMAXIMIZED, WPF_RESTORETOMAXIMIZED, {-32000,-32000}, {-1,-1}, FALSE },
/* 43 */ { SW_SHOWMAXIMIZED, TRUE, WS_VISIBLE|WS_MAXIMIZE, WmMinMax_2,
           SW_SHOWMAXIMIZED, WPF_RESTORETOMAXIMIZED, {-32000,-32000}, {-1,-1}, FALSE },
/* 44 */ { SW_MINIMIZE, TRUE, WS_VISIBLE|WS_MINIMIZE, WmMinimize_1,
           SW_SHOWMINIMIZED, WPF_RESTORETOMAXIMIZED, {-32000,-32000}, {-1,-1}, FALSE },
/* 45 */ { SW_MINIMIZE, TRUE, WS_VISIBLE|WS_MINIMIZE, WmMinMax_3,
           SW_SHOWMINIMIZED, WPF_RESTORETOMAXIMIZED, {-32000,-32000}, {-1,-1}, FALSE },
/* 46 */ { SW_RESTORE, TRUE, WS_VISIBLE|WS_MAXIMIZE, WmRestore_3,
           SW_SHOWMAXIMIZED, WPF_RESTORETOMAXIMIZED, {-32000,-32000}, {-1,-1}, FALSE },
/* 47 */ { SW_RESTORE, TRUE, WS_VISIBLE, WmRestore_4,
           SW_SHOWNORMAL, WPF_RESTORETOMAXIMIZED, {-32000,-32000}, {-1,-1}, FALSE },
/* 48 */ { SW_SHOWMAXIMIZED, TRUE, WS_VISIBLE|WS_MAXIMIZE, WmShowMaximized_3,
           SW_SHOWMAXIMIZED, WPF_RESTORETOMAXIMIZED, {-32000,-32000}, {-1,-1}, FALSE },
/* 49 */ { SW_SHOW, TRUE, WS_VISIBLE|WS_MAXIMIZE, WmEmptySeq,
           SW_SHOWMAXIMIZED, WPF_RESTORETOMAXIMIZED, {-32000,-32000}, {-1,-1}, FALSE },
/* 50 */ { SW_SHOWNORMAL, TRUE, WS_VISIBLE, WmRestore_5,
           SW_SHOWNORMAL, WPF_RESTORETOMAXIMIZED, {-32000,-32000}, {-1,-1}, FALSE },
/* 51 */ { SW_SHOWNORMAL, TRUE, WS_VISIBLE, WmRestore_5,
           SW_SHOWNORMAL, WPF_RESTORETOMAXIMIZED, {-32000,-32000}, {-1,-1}, FALSE },
/* 52 */ { SW_HIDE, TRUE, 0, WmHide_1,
           SW_SHOWNORMAL, WPF_RESTORETOMAXIMIZED, {-32000,-32000}, {-1,-1}, FALSE },
/* 53 */ { SW_HIDE, FALSE, 0, WmEmptySeq,
           SW_SHOWNORMAL, WPF_RESTORETOMAXIMIZED, {-32000,-32000}, {-1,-1}, FALSE },
/* 54 */ { SW_MINIMIZE, FALSE, WS_VISIBLE|WS_MINIMIZE, WmMinimize_3,
           SW_SHOWMINIMIZED, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 55 */ { SW_HIDE, TRUE, WS_MINIMIZE, WmHide_2,
           SW_SHOWMINIMIZED, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 56 */ { SW_SHOWNOACTIVATE, FALSE, WS_VISIBLE, WmShowNoActivate_2,
           SW_SHOWNORMAL, 0, {-32000,-32000}, {-1,-1}, FALSE },
/* 57 */ { SW_SHOW, TRUE, WS_VISIBLE, WmEmptySeq,
           SW_SHOWNORMAL, 0, {-32000,-32000}, {-1,-1}, FALSE }
    };
    HWND hwnd;
    DWORD style;
    LPARAM ret;
    INT i;
    WINDOWPLACEMENT wp;
    RECT win_rc, work_rc = {0, 0, 0, 0};

#define WS_BASE (WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_POPUP|WS_CLIPSIBLINGS)
    hwnd = CreateWindowExA(0, "ShowWindowClass", NULL, WS_BASE,
                          120, 120, 90, 90,
                          0, 0, 0, NULL);
    assert(hwnd);

    style = GetWindowLongA(hwnd, GWL_STYLE) & ~WS_BASE;
    ok(style == 0, "expected style 0, got %08x\n", style);

    flush_events();
    flush_sequence();

    if (pGetMonitorInfoA && pMonitorFromPoint)
    {
        HMONITOR hmon;
        MONITORINFO mi;
        POINT pt = {0, 0};

        SetLastError(0xdeadbeef);
        hmon = pMonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
        ok(hmon != 0, "MonitorFromPoint error %u\n", GetLastError());

        mi.cbSize = sizeof(mi);
        SetLastError(0xdeadbeef);
        ret = pGetMonitorInfoA(hmon, &mi);
        ok(ret, "GetMonitorInfo error %u\n", GetLastError());
        trace("monitor (%d,%d-%d,%d), work (%d,%d-%d,%d)\n",
            mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom,
            mi.rcWork.left, mi.rcWork.top, mi.rcWork.right, mi.rcWork.bottom);
        work_rc = mi.rcWork;
    }

    GetWindowRect(hwnd, &win_rc);
    OffsetRect(&win_rc, -work_rc.left, -work_rc.top);

    wp.length = sizeof(wp);
    SetLastError(0xdeadbeaf);
    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "GetWindowPlacement error %u\n", GetLastError());
    ok(wp.flags == 0, "expected 0, got %#x\n", wp.flags);
    ok(wp.showCmd == SW_SHOWNORMAL, "expected SW_SHOWNORMAL, got %d\n", wp.showCmd);
    ok(wp.ptMinPosition.x == -1 && wp.ptMinPosition.y == -1,
       "expected -1,-1 got %d,%d\n", wp.ptMinPosition.x, wp.ptMinPosition.y);
    ok(wp.ptMaxPosition.x == -1 && wp.ptMaxPosition.y == -1,
       "expected -1,-1 got %d,%d\n", wp.ptMaxPosition.x, wp.ptMaxPosition.y);
    if (work_rc.left || work_rc.top) todo_wine /* FIXME: remove once Wine is fixed */
    ok(EqualRect(&win_rc, &wp.rcNormalPosition),
       "expected %d,%d-%d,%d got %d,%d-%d,%d\n",
        win_rc.left, win_rc.top, win_rc.right, win_rc.bottom,
        wp.rcNormalPosition.left, wp.rcNormalPosition.top,
        wp.rcNormalPosition.right, wp.rcNormalPosition.bottom);
    else
    ok(EqualRect(&win_rc, &wp.rcNormalPosition),
       "expected %d,%d-%d,%d got %d,%d-%d,%d\n",
        win_rc.left, win_rc.top, win_rc.right, win_rc.bottom,
        wp.rcNormalPosition.left, wp.rcNormalPosition.top,
        wp.rcNormalPosition.right, wp.rcNormalPosition.bottom);

    for (i = 0; i < sizeof(sw)/sizeof(sw[0]); i++)
    {
        static const char * const sw_cmd_name[13] =
        {
            "SW_HIDE", "SW_SHOWNORMAL", "SW_SHOWMINIMIZED", "SW_SHOWMAXIMIZED",
            "SW_SHOWNOACTIVATE", "SW_SHOW", "SW_MINIMIZE", "SW_SHOWMINNOACTIVE",
            "SW_SHOWNA", "SW_RESTORE", "SW_SHOWDEFAULT", "SW_FORCEMINIMIZE",
            "SW_NORMALNA" /* 0xCC */
        };
        char comment[64];
        INT idx; /* index into the above array of names */

        idx = (sw[i].cmd == SW_NORMALNA) ? 12 : sw[i].cmd;

        style = GetWindowLongA(hwnd, GWL_STYLE);
        trace("%d: sending %s, current window style %08x\n", i+1, sw_cmd_name[idx], style);
        ret = ShowWindow(hwnd, sw[i].cmd);
        ok(!ret == !sw[i].ret, "%d: cmd %s: expected ret %lu, got %lu\n", i+1, sw_cmd_name[idx], sw[i].ret, ret);
        style = GetWindowLongA(hwnd, GWL_STYLE) & ~WS_BASE;
        ok(style == sw[i].style, "%d: expected style %08x, got %08x\n", i+1, sw[i].style, style);

        sprintf(comment, "%d: ShowWindow(%s)", i+1, sw_cmd_name[idx]);
        ok_sequence(sw[i].msg, comment, sw[i].todo_msg);

        wp.length = sizeof(wp);
        SetLastError(0xdeadbeaf);
        ret = GetWindowPlacement(hwnd, &wp);
        ok(ret, "GetWindowPlacement error %u\n", GetLastError());
        ok(wp.flags == sw[i].wp_flags, "expected %#x, got %#x\n", sw[i].wp_flags, wp.flags);
        ok(wp.showCmd == sw[i].wp_cmd, "expected %d, got %d\n", sw[i].wp_cmd, wp.showCmd);

        /* NT moves the minimized window to -32000,-32000, win9x to 3000,3000 */
        if ((wp.ptMinPosition.x + work_rc.left == -32000 && wp.ptMinPosition.y + work_rc.top == -32000) ||
            (wp.ptMinPosition.x + work_rc.left == 3000 && wp.ptMinPosition.y + work_rc.top == 3000))
        {
            ok((wp.ptMinPosition.x + work_rc.left == sw[i].wp_min.x && wp.ptMinPosition.y + work_rc.top == sw[i].wp_min.y) ||
               (wp.ptMinPosition.x + work_rc.left == 3000 && wp.ptMinPosition.y + work_rc.top == 3000),
               "expected %d,%d got %d,%d\n", sw[i].wp_min.x, sw[i].wp_min.y, wp.ptMinPosition.x, wp.ptMinPosition.y);
        }
        else
        {
            ok(wp.ptMinPosition.x == sw[i].wp_min.x && wp.ptMinPosition.y == sw[i].wp_min.y,
               "expected %d,%d got %d,%d\n", sw[i].wp_min.x, sw[i].wp_min.y, wp.ptMinPosition.x, wp.ptMinPosition.y);
        }

        if (wp.ptMaxPosition.x != sw[i].wp_max.x || wp.ptMaxPosition.y != sw[i].wp_max.y)
        todo_wine
        ok(wp.ptMaxPosition.x == sw[i].wp_max.x && wp.ptMaxPosition.y == sw[i].wp_max.y,
           "expected %d,%d got %d,%d\n", sw[i].wp_max.x, sw[i].wp_max.y, wp.ptMaxPosition.x, wp.ptMaxPosition.y);
        else
        ok(wp.ptMaxPosition.x == sw[i].wp_max.x && wp.ptMaxPosition.y == sw[i].wp_max.y,
           "expected %d,%d got %d,%d\n", sw[i].wp_max.x, sw[i].wp_max.y, wp.ptMaxPosition.x, wp.ptMaxPosition.y);

if (0) /* FIXME: Wine behaves completely different here */
        ok(EqualRect(&win_rc, &wp.rcNormalPosition),
           "expected %d,%d-%d,%d got %d,%d-%d,%d\n",
            win_rc.left, win_rc.top, win_rc.right, win_rc.bottom,
            wp.rcNormalPosition.left, wp.rcNormalPosition.top,
            wp.rcNormalPosition.right, wp.rcNormalPosition.bottom);
    }
    DestroyWindow(hwnd);
    flush_events();
}

static INT_PTR WINAPI test_dlg_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct recvd_message msg;

    if (ignore_message( message )) return 0;

    msg.hwnd = hwnd;
    msg.message = message;
    msg.flags = sent|wparam|lparam;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.descr = "dialog";
    add_message(&msg);

    /* calling DefDlgProc leads to a recursion under XP */

    switch (message)
    {
    case WM_INITDIALOG:
    case WM_GETDLGCODE:
        return 0;
    }
    return 1;
}

static const struct message WmDefDlgSetFocus_1[] = {
    { WM_GETDLGCODE, sent|wparam|lparam, 0, 0 },
    { WM_GETTEXTLENGTH, sent|wparam|lparam|optional, 0, 0 }, /* XP */
    { WM_GETTEXT, sent|wparam|optional, 6 }, /* XP */
    { WM_GETTEXT, sent|wparam|optional, 12 }, /* XP */
    { EM_SETSEL, sent|wparam, 0 }, /* XP sets lparam to text length, Win9x to -2 */
    { HCBT_SETFOCUS, hook },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|wparam, 0 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 10 },
    { WM_CTLCOLOREDIT, sent },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 11 },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, OBJID_CARET, 0 },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, OBJID_CARET, 0 },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, OBJID_CARET, 0 },
    { WM_COMMAND, sent|wparam, MAKEWPARAM(1, EN_SETFOCUS) },
    { 0 }
};
static const struct message WmDefDlgSetFocus_2[] = {
    { WM_GETDLGCODE, sent|wparam|lparam, 0, 0 },
    { WM_GETTEXTLENGTH, sent|wparam|lparam|optional, 0, 0 }, /* XP */
    { WM_GETTEXT, sent|wparam|optional, 6 }, /* XP */
    { WM_GETTEXT, sent|wparam|optional, 12 }, /* XP */
    { EM_SETSEL, sent|wparam, 0 }, /* XP sets lparam to text length, Win9x to -2 */
    { EVENT_OBJECT_HIDE, winevent_hook|wparam|lparam, OBJID_CARET, 0 },
    { WM_CTLCOLOREDIT, sent|optional }, /* XP */
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, OBJID_CARET, 0 },
    { 0 }
};
/* Creation of a dialog */
static const struct message WmCreateDialogParamSeq_1[] = {
    { HCBT_CREATEWND, hook },
    { WM_NCCREATE, sent },
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { WM_CREATE, sent },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },
    { WM_MOVE, sent },
    { WM_SETFONT, sent },
    { WM_INITDIALOG, sent },
    { WM_CHANGEUISTATE, sent|optional },
    { 0 }
};
/* Creation of a dialog */
static const struct message WmCreateDialogParamSeq_2[] = {
    { HCBT_CREATEWND, hook },
    { WM_NCCREATE, sent },
    { WM_NCCALCSIZE, sent|wparam, 0 },
    { WM_CREATE, sent },
    { EVENT_OBJECT_CREATE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SIZE, sent|wparam, SIZE_RESTORED },
    { WM_MOVE, sent },
    { WM_CHANGEUISTATE, sent|optional },
    { 0 }
};

static void test_dialog_messages(void)
{
    WNDCLASSA cls;
    HWND hdlg, hedit1, hedit2, hfocus;
    LRESULT ret;

#define set_selection(hctl, start, end) \
    ret = SendMessageA(hctl, EM_SETSEL, start, end); \
    ok(ret == 1, "EM_SETSEL returned %ld\n", ret);

#define check_selection(hctl, start, end) \
    ret = SendMessageA(hctl, EM_GETSEL, 0, 0); \
    ok(ret == MAKELRESULT(start, end), "wrong selection (%d - %d)\n", LOWORD(ret), HIWORD(ret));

    subclass_edit();

    hdlg = CreateWindowExA(WS_EX_DLGMODALFRAME, "TestDialogClass", NULL,
                          WS_VISIBLE|WS_CAPTION|WS_SYSMENU|WS_DLGFRAME,
                          0, 0, 100, 100, 0, 0, 0, NULL);
    ok(hdlg != 0, "Failed to create custom dialog window\n");

    hedit1 = CreateWindowExA(0, "my_edit_class", NULL,
                           WS_CHILD|WS_BORDER|WS_VISIBLE|WS_TABSTOP,
                           0, 0, 80, 20, hdlg, (HMENU)1, 0, NULL);
    ok(hedit1 != 0, "Failed to create edit control\n");
    hedit2 = CreateWindowExA(0, "my_edit_class", NULL,
                           WS_CHILD|WS_BORDER|WS_VISIBLE|WS_TABSTOP,
                           0, 40, 80, 20, hdlg, (HMENU)2, 0, NULL);
    ok(hedit2 != 0, "Failed to create edit control\n");

    SendMessageA(hedit1, WM_SETTEXT, 0, (LPARAM)"hello");
    SendMessageA(hedit2, WM_SETTEXT, 0, (LPARAM)"bye");

    hfocus = GetFocus();
    ok(hfocus == hdlg, "wrong focus %p\n", hfocus);

    SetFocus(hedit2);
    hfocus = GetFocus();
    ok(hfocus == hedit2, "wrong focus %p\n", hfocus);

    check_selection(hedit1, 0, 0);
    check_selection(hedit2, 0, 0);

    set_selection(hedit2, 0, -1);
    check_selection(hedit2, 0, 3);

    SetFocus(0);
    hfocus = GetFocus();
    ok(hfocus == 0, "wrong focus %p\n", hfocus);

    flush_sequence();
    ret = DefDlgProcA(hdlg, WM_SETFOCUS, 0, 0);
    ok(ret == 0, "WM_SETFOCUS returned %ld\n", ret);
    ok_sequence(WmDefDlgSetFocus_1, "DefDlgProc(WM_SETFOCUS) 1", FALSE);

    hfocus = GetFocus();
    ok(hfocus == hedit1, "wrong focus %p\n", hfocus);

    check_selection(hedit1, 0, 5);
    check_selection(hedit2, 0, 3);

    flush_sequence();
    ret = DefDlgProcA(hdlg, WM_SETFOCUS, 0, 0);
    ok(ret == 0, "WM_SETFOCUS returned %ld\n", ret);
    ok_sequence(WmDefDlgSetFocus_2, "DefDlgProc(WM_SETFOCUS) 2", FALSE);

    hfocus = GetFocus();
    ok(hfocus == hedit1, "wrong focus %p\n", hfocus);

    check_selection(hedit1, 0, 5);
    check_selection(hedit2, 0, 3);

    EndDialog(hdlg, 0);
    DestroyWindow(hedit1);
    DestroyWindow(hedit2);
    DestroyWindow(hdlg);
    flush_sequence();

#undef set_selection
#undef check_selection

    ok(GetClassInfoA(0, "#32770", &cls), "GetClassInfo failed\n");
    cls.lpszClassName = "MyDialogClass";
    cls.hInstance = GetModuleHandleA(NULL);
    /* need a cast since a dlgproc is used as a wndproc */
    cls.lpfnWndProc = test_dlg_proc;
    if (!RegisterClassA(&cls)) assert(0);

    hdlg = CreateDialogParamA(0, "CLASS_TEST_DIALOG_2", 0, test_dlg_proc, 0);
    ok(IsWindow(hdlg), "CreateDialogParam failed\n");
    ok_sequence(WmCreateDialogParamSeq_1, "CreateDialogParam_1", FALSE);
    EndDialog(hdlg, 0);
    DestroyWindow(hdlg);
    flush_sequence();

    hdlg = CreateDialogParamA(0, "CLASS_TEST_DIALOG_2", 0, NULL, 0);
    ok(IsWindow(hdlg), "CreateDialogParam failed\n");
    ok_sequence(WmCreateDialogParamSeq_2, "CreateDialogParam_2", FALSE);
    EndDialog(hdlg, 0);
    DestroyWindow(hdlg);
    flush_sequence();

    UnregisterClassA(cls.lpszClassName, cls.hInstance);
}

static void test_EndDialog(void)
{
    HWND hparent, hother, hactive, hdlg;
    WNDCLASSA cls;

    hparent = CreateWindowExA(0, "TestParentClass", "Test parent",
                              WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_DISABLED,
                              100, 100, 200, 200, 0, 0, 0, NULL);
    ok (hparent != 0, "Failed to create parent window\n");

    hother = CreateWindowExA(0, "TestParentClass", "Test parent 2",
                              WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              100, 100, 200, 200, 0, 0, 0, NULL);
    ok (hother != 0, "Failed to create parent window\n");

    ok(GetClassInfoA(0, "#32770", &cls), "GetClassInfo failed\n");
    cls.lpszClassName = "MyDialogClass";
    cls.hInstance = GetModuleHandleA(NULL);
    cls.lpfnWndProc = test_dlg_proc;
    if (!RegisterClassA(&cls)) assert(0);

    flush_sequence();
    SetForegroundWindow(hother);
    hactive = GetForegroundWindow();
    ok(hother == hactive, "Wrong window has focus (%p != %p)\n", hother, hactive);

    /* create a dialog where the parent is disabled, this parent should still
       receive the focus when the dialog exits (even though "normally" a
       disabled window should not receive the focus) */
    hdlg = CreateDialogParamA(0, "CLASS_TEST_DIALOG_2", hparent, test_dlg_proc, 0);
    ok(IsWindow(hdlg), "CreateDialogParam failed\n");
    SetForegroundWindow(hdlg);
    hactive = GetForegroundWindow();
    ok(hdlg == hactive, "Wrong window has focus (%p != %p)\n", hdlg, hactive);
    EndDialog(hdlg, 0);
    hactive = GetForegroundWindow();
    ok(hparent == hactive, "Wrong window has focus (parent != active) (active: %p, parent: %p, dlg: %p, other: %p)\n", hactive, hparent, hdlg, hother);
    DestroyWindow(hdlg);
    flush_sequence();

    DestroyWindow( hother );
    DestroyWindow( hparent );
    UnregisterClassA(cls.lpszClassName, cls.hInstance);
}

static void test_nullCallback(void)
{
    HWND hwnd;

    hwnd = CreateWindowExA(0, "TestWindowClass", "Test overlapped", WS_OVERLAPPEDWINDOW,
                           100, 100, 200, 200, 0, 0, 0, NULL);
    ok (hwnd != 0, "Failed to create overlapped window\n");

    SendMessageCallbackA(hwnd,WM_NULL,0,0,NULL,0);
    flush_events();
    DestroyWindow(hwnd);
}

/* SetActiveWindow( 0 ) hwnd visible */
static const struct message SetActiveWindowSeq0[] =
{
    { HCBT_ACTIVATE, hook|optional },
    { WM_NCACTIVATE, sent|wparam, 0 },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ACTIVATE, sent|wparam, 0 },
    { WM_ACTIVATEAPP, sent|wparam|optional, 0 },
    { WM_ACTIVATEAPP, sent|wparam|optional, 0 },
    { WM_QUERYNEWPALETTE, sent|wparam|lparam|optional, 0, 0 },
    { WM_KILLFOCUS, sent|optional },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCACTIVATE, sent|wparam|optional, 1 },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ACTIVATE, sent|wparam|optional, 1 },
    { HCBT_SETFOCUS, hook|optional },
    { WM_KILLFOCUS, sent|defwinproc|optional },
    { WM_IME_SETCONTEXT, sent|defwinproc|optional },
    { WM_IME_SETCONTEXT, sent|defwinproc|optional },
    { WM_IME_SETCONTEXT, sent|optional },
    { WM_IME_SETCONTEXT, sent|optional },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { WM_SETFOCUS, sent|defwinproc|optional },
    { WM_GETTEXT, sent|optional },
    { 0 }
};
/* SetActiveWindow( hwnd ) hwnd visible */
static const struct message SetActiveWindowSeq1[] =
{
    { WM_QUERYNEWPALETTE, sent|wparam|lparam|optional, 0, 0 },
    { 0 }
};
/* SetActiveWindow( popup ) hwnd visible, popup visible */
static const struct message SetActiveWindowSeq2[] =
{
    { HCBT_ACTIVATE, hook },
    { WM_NCACTIVATE, sent|wparam, 0 },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ACTIVATE, sent|wparam, 0 },
    { WM_QUERYNEWPALETTE, sent|wparam|lparam|optional, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE },
    { WM_NCPAINT, sent|optional },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ERASEBKGND, sent|optional },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_NCACTIVATE, sent|wparam, 1 },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ACTIVATE, sent|wparam, 1 },
    { HCBT_SETFOCUS, hook },
    { WM_KILLFOCUS, sent|defwinproc },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 0 },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { WM_SETFOCUS, sent|defwinproc },
    { WM_GETTEXT, sent|optional },
    { 0 }
};

/* SetActiveWindow( hwnd ) hwnd not visible */
static const struct message SetActiveWindowSeq3[] =
{
    { HCBT_ACTIVATE, hook },
    { WM_QUERYNEWPALETTE, sent|wparam|lparam|optional, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOSIZE|SWP_NOMOVE },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOACTIVATE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_ACTIVATEAPP, sent|wparam, 1 },
    { WM_ACTIVATEAPP, sent|wparam, 1 },
    { WM_NCACTIVATE, sent|wparam, 1 },
    { WM_ACTIVATE, sent|wparam, 1 },
    { HCBT_SETFOCUS, hook },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { WM_SETFOCUS, sent|defwinproc },
    { 0 }
};
/* SetActiveWindow( popup ) hwnd not visible, popup not visible */
static const struct message SetActiveWindowSeq4[] =
{
    { HCBT_ACTIVATE, hook },
    { WM_NCACTIVATE, sent|wparam, 0 },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ACTIVATE, sent|wparam, 0 },
    { WM_QUERYNEWPALETTE, sent|wparam|lparam|optional, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOSIZE|SWP_NOMOVE },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE|SWP_NOREDRAW|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_WINDOWPOSCHANGED, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOREDRAW|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_NCACTIVATE, sent|wparam, 1 },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ACTIVATE, sent|wparam, 1 },
    { HCBT_SETFOCUS, hook },
    { WM_KILLFOCUS, sent|defwinproc },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 0 },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 1 },
    { WM_IME_NOTIFY, sent|wparam|defwinproc|optional, 2 },
    { WM_SETFOCUS, sent|defwinproc },
    { 0 }
};


static void test_SetActiveWindow(void)
{
    HWND hwnd, popup, ret;

    hwnd = CreateWindowExA(0, "TestWindowClass", "Test SetActiveWindow",
                           WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                           100, 100, 200, 200, 0, 0, 0, NULL);

    popup = CreateWindowExA(0, "TestWindowClass", "Test SetActiveWindow",
                           WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_POPUP,
                           100, 100, 200, 200, hwnd, 0, 0, NULL);

    ok(hwnd != 0, "Failed to create overlapped window\n");
    ok(popup != 0, "Failed to create popup window\n");
    SetForegroundWindow( popup );
    flush_sequence();

    trace("SetActiveWindow(0)\n");
    ret = SetActiveWindow(0);
    ok( ret == popup, "Failed to SetActiveWindow(0)\n");
    ok_sequence(SetActiveWindowSeq0, "SetActiveWindow(0)", FALSE);
    flush_sequence();

    trace("SetActiveWindow(hwnd), hwnd visible\n");
    ret = SetActiveWindow(hwnd);
    if (ret == hwnd) ok_sequence(SetActiveWindowSeq1, "SetActiveWindow(hwnd), hwnd visible", TRUE);
    flush_sequence();

    trace("SetActiveWindow(popup), hwnd visible, popup visible\n");
    ret = SetActiveWindow(popup);
    ok( ret == hwnd, "Failed to SetActiveWindow(popup), popup visible\n");
    ok_sequence(SetActiveWindowSeq2, "SetActiveWindow(popup), hwnd visible, popup visible", FALSE);
    flush_sequence();

    ShowWindow(hwnd, SW_HIDE);
    ShowWindow(popup, SW_HIDE);
    flush_sequence();

    trace("SetActiveWindow(hwnd), hwnd not visible\n");
    ret = SetActiveWindow(hwnd);
    ok( ret == NULL, "SetActiveWindow(hwnd), hwnd not visible, previous is %p\n", ret );
    ok_sequence(SetActiveWindowSeq3, "SetActiveWindow(hwnd), hwnd not visible", TRUE);
    flush_sequence();

    trace("SetActiveWindow(popup), hwnd not visible, popup not visible\n");
    ret = SetActiveWindow(popup);
    ok( ret == hwnd, "Failed to SetActiveWindow(popup)\n");
    ok_sequence(SetActiveWindowSeq4, "SetActiveWindow(popup), hwnd not visible, popup not visible", TRUE);
    flush_sequence();

    trace("done\n");

    DestroyWindow(hwnd);
}

static const struct message SetForegroundWindowSeq[] =
{
    { WM_NCACTIVATE, sent|wparam, 0 },
    { WM_GETTEXT, sent|defwinproc|optional },
    { WM_ACTIVATE, sent|wparam, 0 },
    { WM_ACTIVATEAPP, sent|wparam, 0 },
    { WM_KILLFOCUS, sent },
    { WM_IME_SETCONTEXT, sent|wparam|optional, 0 },
    { WM_IME_NOTIFY, sent|wparam|optional|defwinproc, 1 },
    { 0 }
};

static void test_SetForegroundWindow(void)
{
    HWND hwnd;

    hwnd = CreateWindowExA(0, "TestWindowClass", "Test SetForegroundWindow",
                           WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                           100, 100, 200, 200, 0, 0, 0, NULL);
    ok (hwnd != 0, "Failed to create overlapped window\n");
    SetForegroundWindow( hwnd );
    flush_sequence();

    trace("SetForegroundWindow( 0 )\n");
    SetForegroundWindow( 0 );
    ok_sequence(WmEmptySeq, "SetForegroundWindow( 0 ) away from foreground top level window", FALSE);
    trace("SetForegroundWindow( GetDesktopWindow() )\n");
    SetForegroundWindow( GetDesktopWindow() );
    ok_sequence(SetForegroundWindowSeq, "SetForegroundWindow( desktop ) away from "
                                        "foreground top level window", FALSE);
    trace("done\n");

    DestroyWindow(hwnd);
}

static void test_dbcs_wm_char(void)
{
    BYTE dbch[2];
    WCHAR wch, bad_wch;
    HWND hwnd, hwnd2;
    MSG msg;
    DWORD time;
    POINT pt;
    DWORD_PTR res;
    CPINFOEXA cpinfo;
    UINT i, j, k;
    struct message wmCharSeq[2];
    BOOL ret;

    if (!pGetCPInfoExA)
    {
        win_skip("GetCPInfoExA is not available\n");
        return;
    }

    pGetCPInfoExA( CP_ACP, 0, &cpinfo );
    if (cpinfo.MaxCharSize != 2)
    {
        skip( "Skipping DBCS WM_CHAR test in SBCS codepage '%s'\n", cpinfo.CodePageName );
        return;
    }

    dbch[0] = dbch[1] = 0;
    wch = 0;
    bad_wch = cpinfo.UnicodeDefaultChar;
    for (i = 0; !wch && i < MAX_LEADBYTES && cpinfo.LeadByte[i]; i += 2)
        for (j = cpinfo.LeadByte[i]; !wch && j <= cpinfo.LeadByte[i+1]; j++)
            for (k = 128; k <= 255; k++)
            {
                char str[2];
                WCHAR wstr[2];
                str[0] = j;
                str[1] = k;
                if (MultiByteToWideChar( CP_ACP, 0, str, 2, wstr, 2 ) == 1 &&
                    WideCharToMultiByte( CP_ACP, 0, wstr, 1, str, 2, NULL, NULL ) == 2 &&
                    (BYTE)str[0] == j && (BYTE)str[1] == k &&
                    HIBYTE(wstr[0]) && HIBYTE(wstr[0]) != 0xff)
                {
                    dbch[0] = j;
                    dbch[1] = k;
                    wch = wstr[0];
                    break;
                }
            }

    if (!wch)
    {
        skip( "Skipping DBCS WM_CHAR test, no appropriate char found\n" );
        return;
    }
    trace( "using dbcs char %02x,%02x wchar %04x bad wchar %04x codepage '%s'\n",
           dbch[0], dbch[1], wch, bad_wch, cpinfo.CodePageName );

    hwnd = CreateWindowExW(0, testWindowClassW, NULL,
                           WS_OVERLAPPEDWINDOW, 100, 100, 200, 200, 0, 0, 0, NULL);
    hwnd2 = CreateWindowExW(0, testWindowClassW, NULL,
                           WS_OVERLAPPEDWINDOW, 100, 100, 200, 200, 0, 0, 0, NULL);
    ok (hwnd != 0, "Failed to create overlapped window\n");
    ok (hwnd2 != 0, "Failed to create overlapped window\n");
    flush_sequence();

    memset( wmCharSeq, 0, sizeof(wmCharSeq) );
    wmCharSeq[0].message = WM_CHAR;
    wmCharSeq[0].flags = sent|wparam;
    wmCharSeq[0].wParam = wch;

    /* posted message */
    PostMessageA( hwnd, WM_CHAR, dbch[0], 0 );
    ret = PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( !ret, "got message %x\n", msg.message );
    PostMessageA( hwnd, WM_CHAR, dbch[1], 0 );
    ret = PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( ret, "no message\n" );
    ok( msg.message == WM_CHAR, "unexpected message %x\n", msg.message );
    ok( msg.wParam == wch, "bad wparam %lx/%x\n", msg.wParam, wch );
    ret = PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( !ret, "got message %x\n", msg.message );

    /* posted thread message */
    PostThreadMessageA( GetCurrentThreadId(), WM_CHAR, dbch[0], 0 );
    ret = PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( !ret, "got message %x\n", msg.message );
    PostMessageA( hwnd, WM_CHAR, dbch[1], 0 );
    ret = PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( ret, "no message\n" );
    ok( msg.message == WM_CHAR, "unexpected message %x\n", msg.message );
    ok( msg.wParam == wch, "bad wparam %lx/%x\n", msg.wParam, wch );
    ret = PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( !ret, "got message %x\n", msg.message );

    /* sent message */
    flush_sequence();
    SendMessageA( hwnd, WM_CHAR, dbch[0], 0 );
    ok_sequence( WmEmptySeq, "no messages", FALSE );
    SendMessageA( hwnd, WM_CHAR, dbch[1], 0 );
    ok_sequence( wmCharSeq, "Unicode WM_CHAR", FALSE );
    ret = PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( !ret, "got message %x\n", msg.message );

    /* sent message with timeout */
    flush_sequence();
    SendMessageTimeoutA( hwnd, WM_CHAR, dbch[0], 0, SMTO_NORMAL, 0, &res );
    ok_sequence( WmEmptySeq, "no messages", FALSE );
    SendMessageTimeoutA( hwnd, WM_CHAR, dbch[1], 0, SMTO_NORMAL, 0, &res );
    ok_sequence( wmCharSeq, "Unicode WM_CHAR", FALSE );
    ret = PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( !ret, "got message %x\n", msg.message );

    /* sent message with timeout and callback */
    flush_sequence();
    SendMessageTimeoutA( hwnd, WM_CHAR, dbch[0], 0, SMTO_NORMAL, 0, &res );
    ok_sequence( WmEmptySeq, "no messages", FALSE );
    SendMessageCallbackA( hwnd, WM_CHAR, dbch[1], 0, NULL, 0 );
    ok_sequence( wmCharSeq, "Unicode WM_CHAR", FALSE );
    ret = PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( !ret, "got message %x\n", msg.message );

    /* sent message with callback */
    flush_sequence();
    SendNotifyMessageA( hwnd, WM_CHAR, dbch[0], 0 );
    ok_sequence( WmEmptySeq, "no messages", FALSE );
    SendMessageCallbackA( hwnd, WM_CHAR, dbch[1], 0, NULL, 0 );
    ok_sequence( wmCharSeq, "Unicode WM_CHAR", FALSE );
    ret = PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( !ret, "got message %x\n", msg.message );

    /* direct window proc call */
    flush_sequence();
    CallWindowProcA( (WNDPROC)GetWindowLongPtrA( hwnd, GWLP_WNDPROC ), hwnd, WM_CHAR, dbch[0], 0 );
    ok_sequence( WmEmptySeq, "no messages", FALSE );
    CallWindowProcA( (WNDPROC)GetWindowLongPtrA( hwnd, GWLP_WNDPROC ), hwnd, WM_CHAR, dbch[1], 0 );
    ok_sequence( wmCharSeq, "Unicode WM_CHAR", FALSE );

    /* dispatch message */
    msg.hwnd = hwnd;
    msg.message = WM_CHAR;
    msg.wParam = dbch[0];
    msg.lParam = 0;
    DispatchMessageA( &msg );
    ok_sequence( WmEmptySeq, "no messages", FALSE );
    msg.wParam = dbch[1];
    DispatchMessageA( &msg );
    ok_sequence( wmCharSeq, "Unicode WM_CHAR", FALSE );

    /* window handle is irrelevant */
    flush_sequence();
    SendMessageA( hwnd2, WM_CHAR, dbch[0], 0 );
    ok_sequence( WmEmptySeq, "no messages", FALSE );
    SendMessageA( hwnd, WM_CHAR, dbch[1], 0 );
    ok_sequence( wmCharSeq, "Unicode WM_CHAR", FALSE );
    ret = PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( !ret, "got message %x\n", msg.message );

    /* interleaved post and send */
    flush_sequence();
    PostMessageA( hwnd2, WM_CHAR, dbch[0], 0 );
    SendMessageA( hwnd2, WM_CHAR, dbch[0], 0 );
    ok_sequence( WmEmptySeq, "no messages", FALSE );
    ret = PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( !ret, "got message %x\n", msg.message );
    PostMessageA( hwnd, WM_CHAR, dbch[1], 0 );
    ok_sequence( WmEmptySeq, "no messages", FALSE );
    ret = PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( ret, "no message\n" );
    ok( msg.message == WM_CHAR, "unexpected message %x\n", msg.message );
    ok( msg.wParam == wch, "bad wparam %lx/%x\n", msg.wParam, wch );
    ret = PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( !ret, "got message %x\n", msg.message );
    SendMessageA( hwnd, WM_CHAR, dbch[1], 0 );
    ok_sequence( wmCharSeq, "Unicode WM_CHAR", FALSE );
    ret = PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( !ret, "got message %x\n", msg.message );

    /* interleaved sent message and winproc */
    flush_sequence();
    SendMessageA( hwnd, WM_CHAR, dbch[0], 0 );
    CallWindowProcA( (WNDPROC)GetWindowLongPtrA( hwnd, GWLP_WNDPROC ), hwnd, WM_CHAR, dbch[0], 0 );
    ok_sequence( WmEmptySeq, "no messages", FALSE );
    SendMessageA( hwnd, WM_CHAR, dbch[1], 0 );
    ok_sequence( wmCharSeq, "Unicode WM_CHAR", FALSE );
    CallWindowProcA( (WNDPROC)GetWindowLongPtrA( hwnd, GWLP_WNDPROC ), hwnd, WM_CHAR, dbch[1], 0 );
    ok_sequence( wmCharSeq, "Unicode WM_CHAR", FALSE );

    /* interleaved winproc and dispatch */
    msg.hwnd = hwnd;
    msg.message = WM_CHAR;
    msg.wParam = dbch[0];
    msg.lParam = 0;
    CallWindowProcA( (WNDPROC)GetWindowLongPtrA( hwnd, GWLP_WNDPROC ), hwnd, WM_CHAR, dbch[0], 0 );
    DispatchMessageA( &msg );
    ok_sequence( WmEmptySeq, "no messages", FALSE );
    msg.wParam = dbch[1];
    DispatchMessageA( &msg );
    ok_sequence( wmCharSeq, "Unicode WM_CHAR", FALSE );
    CallWindowProcA( (WNDPROC)GetWindowLongPtrA( hwnd, GWLP_WNDPROC ), hwnd, WM_CHAR, dbch[1], 0 );
    ok_sequence( wmCharSeq, "Unicode WM_CHAR", FALSE );

    /* interleaved sends */
    flush_sequence();
    SendMessageA( hwnd, WM_CHAR, dbch[0], 0 );
    SendMessageCallbackA( hwnd, WM_CHAR, dbch[0], 0, NULL, 0 );
    ok_sequence( WmEmptySeq, "no messages", FALSE );
    SendMessageTimeoutA( hwnd, WM_CHAR, dbch[1], 0, SMTO_NORMAL, 0, &res );
    ok_sequence( wmCharSeq, "Unicode WM_CHAR", FALSE );
    SendMessageA( hwnd, WM_CHAR, dbch[1], 0 );
    ok_sequence( wmCharSeq, "Unicode WM_CHAR", FALSE );

    /* dbcs WM_CHAR */
    flush_sequence();
    SendMessageA( hwnd2, WM_CHAR, (dbch[1] << 8) | dbch[0], 0 );
    ok_sequence( wmCharSeq, "Unicode WM_CHAR", FALSE );
    ret = PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( !ret, "got message %x\n", msg.message );

    /* other char messages are not magic */
    PostMessageA( hwnd, WM_SYSCHAR, dbch[0], 0 );
    ret = PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( ret, "no message\n" );
    ok( msg.message == WM_SYSCHAR, "unexpected message %x\n", msg.message );
    ok( msg.wParam == bad_wch, "bad wparam %lx/%x\n", msg.wParam, bad_wch );
    ret = PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( !ret, "got message %x\n", msg.message );
    PostMessageA( hwnd, WM_DEADCHAR, dbch[0], 0 );
    ret = PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( ret, "no message\n" );
    ok( msg.message == WM_DEADCHAR, "unexpected message %x\n", msg.message );
    ok( msg.wParam == bad_wch, "bad wparam %lx/%x\n", msg.wParam, bad_wch );
    ret = PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( !ret, "got message %x\n", msg.message );

    /* test retrieving messages */

    PostMessageW( hwnd, WM_CHAR, wch, 0 );
    ret = PeekMessageA( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( ret, "no message\n" );
    ok( msg.hwnd == hwnd, "unexpected hwnd %p\n", msg.hwnd );
    ok( msg.message == WM_CHAR, "unexpected message %x\n", msg.message );
    ok( msg.wParam == dbch[0], "bad wparam %lx/%x\n", msg.wParam, dbch[0] );
    ret = PeekMessageA( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( ret, "no message\n" );
    ok( msg.hwnd == hwnd, "unexpected hwnd %p\n", msg.hwnd );
    ok( msg.message == WM_CHAR, "unexpected message %x\n", msg.message );
    ok( msg.wParam == dbch[1], "bad wparam %lx/%x\n", msg.wParam, dbch[0] );
    ret = PeekMessageA( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( !ret, "got message %x\n", msg.message );

    /* message filters */
    PostMessageW( hwnd, WM_CHAR, wch, 0 );
    ret = PeekMessageA( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( ret, "no message\n" );
    ok( msg.hwnd == hwnd, "unexpected hwnd %p\n", msg.hwnd );
    ok( msg.message == WM_CHAR, "unexpected message %x\n", msg.message );
    ok( msg.wParam == dbch[0], "bad wparam %lx/%x\n", msg.wParam, dbch[0] );
    /* message id is filtered, hwnd is not */
    ret = PeekMessageA( &msg, hwnd, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE );
    ok( !ret, "no message\n" );
    ret = PeekMessageA( &msg, hwnd2, 0, 0, PM_REMOVE );
    ok( ret, "no message\n" );
    ok( msg.hwnd == hwnd, "unexpected hwnd %p\n", msg.hwnd );
    ok( msg.message == WM_CHAR, "unexpected message %x\n", msg.message );
    ok( msg.wParam == dbch[1], "bad wparam %lx/%x\n", msg.wParam, dbch[0] );
    ret = PeekMessageA( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( !ret, "got message %x\n", msg.message );

    /* mixing GetMessage and PostMessage */
    PostMessageW( hwnd, WM_CHAR, wch, 0xbeef );
    ok( GetMessageA( &msg, hwnd, 0, 0 ), "no message\n" );
    ok( msg.hwnd == hwnd, "unexpected hwnd %p\n", msg.hwnd );
    ok( msg.message == WM_CHAR, "unexpected message %x\n", msg.message );
    ok( msg.wParam == dbch[0], "bad wparam %lx/%x\n", msg.wParam, dbch[0] );
    ok( msg.lParam == 0xbeef, "bad lparam %lx\n", msg.lParam );
    time = msg.time;
    pt = msg.pt;
    ok( time - GetTickCount() <= 100, "bad time %x\n", msg.time );
    ret = PeekMessageA( &msg, 0, 0, 0, PM_REMOVE );
    ok( ret, "no message\n" );
    ok( msg.hwnd == hwnd, "unexpected hwnd %p\n", msg.hwnd );
    ok( msg.message == WM_CHAR, "unexpected message %x\n", msg.message );
    ok( msg.wParam == dbch[1], "bad wparam %lx/%x\n", msg.wParam, dbch[0] );
    ok( msg.lParam == 0xbeef, "bad lparam %lx\n", msg.lParam );
    ok( msg.time == time, "bad time %x/%x\n", msg.time, time );
    ok( msg.pt.x == pt.x && msg.pt.y == pt.y, "bad point %u,%u/%u,%u\n", msg.pt.x, msg.pt.y, pt.x, pt.y );
    ret = PeekMessageA( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( !ret, "got message %x\n", msg.message );

    /* without PM_REMOVE */
    PostMessageW( hwnd, WM_CHAR, wch, 0 );
    ret = PeekMessageA( &msg, 0, 0, 0, PM_NOREMOVE );
    ok( ret, "no message\n" );
    ok( msg.hwnd == hwnd, "unexpected hwnd %p\n", msg.hwnd );
    ok( msg.message == WM_CHAR, "unexpected message %x\n", msg.message );
    ok( msg.wParam == dbch[0], "bad wparam %lx/%x\n", msg.wParam, dbch[0] );
    ret = PeekMessageA( &msg, 0, 0, 0, PM_REMOVE );
    ok( ret, "no message\n" );
    ok( msg.hwnd == hwnd, "unexpected hwnd %p\n", msg.hwnd );
    ok( msg.message == WM_CHAR, "unexpected message %x\n", msg.message );
    ok( msg.wParam == dbch[0], "bad wparam %lx/%x\n", msg.wParam, dbch[0] );
    ret = PeekMessageA( &msg, 0, 0, 0, PM_NOREMOVE );
    ok( ret, "no message\n" );
    ok( msg.hwnd == hwnd, "unexpected hwnd %p\n", msg.hwnd );
    ok( msg.message == WM_CHAR, "unexpected message %x\n", msg.message );
    ok( msg.wParam == dbch[1], "bad wparam %lx/%x\n", msg.wParam, dbch[0] );
    ret = PeekMessageA( &msg, 0, 0, 0, PM_REMOVE );
    ok( ret, "no message\n" );
    ok( msg.hwnd == hwnd, "unexpected hwnd %p\n", msg.hwnd );
    ok( msg.message == WM_CHAR, "unexpected message %x\n", msg.message );
    ok( msg.wParam == dbch[1], "bad wparam %lx/%x\n", msg.wParam, dbch[0] );
    ret = PeekMessageA( &msg, hwnd, 0, 0, PM_REMOVE );
    ok( !ret, "got message %x\n", msg.message );

    DestroyWindow(hwnd);
    DestroyWindow(hwnd2);
}

static void test_unicode_wm_char(void)
{
    HWND hwnd;
    MSG msg;
    struct message seq[2];
    HKL hkl_orig, hkl_greek;
    DWORD cp;
    LCID thread_locale;

    hkl_orig = GetKeyboardLayout( 0 );
    GetLocaleInfoW( LOWORD( hkl_orig ), LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER, (WCHAR*)&cp, sizeof(cp) / sizeof(WCHAR) );
    if (cp != 1252)
    {
        skip( "Default codepage %d\n", cp );
        return;
    }

    hkl_greek = LoadKeyboardLayoutA( "00000408", 0 );
    if (!hkl_greek || hkl_greek == hkl_orig /* win2k */)
    {
        skip( "Unable to load Greek keyboard layout\n" );
        return;
    }

    hwnd = CreateWindowExW( 0, testWindowClassW, NULL, WS_OVERLAPPEDWINDOW,
                            100, 100, 200, 200, 0, 0, 0, NULL );
    flush_sequence();

    PostMessageW( hwnd, WM_CHAR, 0x3b1, 0 );

    while (GetMessageW( &msg, hwnd, 0, 0 ))
    {
        if (!ignore_message( msg.message )) break;
    }

    ok( msg.hwnd == hwnd, "unexpected hwnd %p\n", msg.hwnd );
    ok( msg.message == WM_CHAR, "unexpected message %x\n", msg.message );
    ok( msg.wParam == 0x3b1, "bad wparam %lx\n", msg.wParam );
    ok( msg.lParam == 0, "bad lparam %lx\n", msg.lParam );

    DispatchMessageW( &msg );

    memset( seq, 0, sizeof(seq) );
    seq[0].message = WM_CHAR;
    seq[0].flags = sent|wparam;
    seq[0].wParam = 0x3b1;

    ok_sequence( seq, "unicode WM_CHAR", FALSE );

    flush_sequence();

    /* greek alpha -> 'a' in cp1252 */
    PostMessageW( hwnd, WM_CHAR, 0x3b1, 0 );

    ok( GetMessageA( &msg, hwnd, 0, 0 ), "no message\n" );
    ok( msg.hwnd == hwnd, "unexpected hwnd %p\n", msg.hwnd );
    ok( msg.message == WM_CHAR, "unexpected message %x\n", msg.message );
    ok( msg.wParam == 0x61, "bad wparam %lx\n", msg.wParam );
    ok( msg.lParam == 0, "bad lparam %lx\n", msg.lParam );

    DispatchMessageA( &msg );

    seq[0].wParam = 0x61;
    ok_sequence( seq, "unicode WM_CHAR", FALSE );

    thread_locale = GetThreadLocale();
    ActivateKeyboardLayout( hkl_greek, 0 );
    ok( GetThreadLocale() == thread_locale, "locale changed from %08x to %08x\n",
        thread_locale, GetThreadLocale() );

    flush_sequence();

    /* greek alpha -> 0xe1 in cp1253 */
    PostMessageW( hwnd, WM_CHAR, 0x3b1, 0 );

    ok( GetMessageA( &msg, hwnd, 0, 0 ), "no message\n" );
    ok( msg.hwnd == hwnd, "unexpected hwnd %p\n", msg.hwnd );
    ok( msg.message == WM_CHAR, "unexpected message %x\n", msg.message );
    ok( msg.wParam == 0xe1, "bad wparam %lx\n", msg.wParam );
    ok( msg.lParam == 0, "bad lparam %lx\n", msg.lParam );

    DispatchMessageA( &msg );

    seq[0].wParam = 0x3b1;
    ok_sequence( seq, "unicode WM_CHAR", FALSE );

    DestroyWindow( hwnd );
    ActivateKeyboardLayout( hkl_orig, 0 );
    UnloadKeyboardLayout( hkl_greek );
}

#define ID_LISTBOX 0x000f

static const struct message wm_lb_setcursel_0[] =
{
    { LB_SETCURSEL, sent|wparam|lparam, 0, 0 },
    { WM_CTLCOLORLISTBOX, sent|parent },
    { WM_DRAWITEM, sent|wparam|lparam|parent, ID_LISTBOX, 0x000120f2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 1 },
    { EVENT_OBJECT_SELECTION, winevent_hook|wparam|lparam, OBJID_CLIENT, 1 },
    { 0 }
};
static const struct message wm_lb_setcursel_1[] =
{
    { LB_SETCURSEL, sent|wparam|lparam, 1, 0 },
    { WM_CTLCOLORLISTBOX, sent|parent },
    { WM_DRAWITEM, sent|wparam|lparam|parent, ID_LISTBOX, 0x000020f2 },
    { WM_CTLCOLORLISTBOX, sent|parent },
    { WM_DRAWITEM, sent|wparam|lparam|parent, ID_LISTBOX, 0x000121f2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 2 },
    { EVENT_OBJECT_SELECTION, winevent_hook|wparam|lparam, OBJID_CLIENT, 2 },
    { 0 }
};
static const struct message wm_lb_setcursel_2[] =
{
    { LB_SETCURSEL, sent|wparam|lparam, 2, 0 },
    { WM_CTLCOLORLISTBOX, sent|parent },
    { WM_DRAWITEM, sent|wparam|lparam|parent, ID_LISTBOX, 0x000021f2 },
    { WM_CTLCOLORLISTBOX, sent|parent },
    { WM_DRAWITEM, sent|wparam|lparam|parent, ID_LISTBOX, 0x000122f2 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 3 },
    { EVENT_OBJECT_SELECTION, winevent_hook|wparam|lparam, OBJID_CLIENT, 3 },
    { 0 }
};
static const struct message wm_lb_click_0[] =
{
    { WM_LBUTTONDOWN, sent|wparam|lparam, 0, MAKELPARAM(1,1) },
    { HCBT_SETFOCUS, hook },
    { WM_KILLFOCUS, sent|parent },
    { WM_IME_SETCONTEXT, sent|wparam|optional|parent, 0 },
    { WM_IME_SETCONTEXT, sent|wparam|defwinproc|optional, 1 },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|defwinproc },

    { WM_DRAWITEM, sent|wparam|lparam|parent, ID_LISTBOX, 0x001142f2 },
    { WM_COMMAND, sent|wparam|parent, MAKEWPARAM(ID_LISTBOX, LBN_SETFOCUS) },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 3 },
    { WM_LBTRACKPOINT, sent|wparam|lparam|parent, 0, MAKELPARAM(1,1) },
    { EVENT_SYSTEM_CAPTURESTART, winevent_hook|wparam|lparam, 0, 0 },

    { WM_DRAWITEM, sent|wparam|lparam|parent, ID_LISTBOX, 0x000142f2 },
    { WM_CTLCOLORLISTBOX, sent|parent },
    { WM_DRAWITEM, sent|wparam|lparam|parent, ID_LISTBOX, 0x000022f2 },
    { WM_CTLCOLORLISTBOX, sent|parent },
    { WM_DRAWITEM, sent|wparam|lparam|parent, ID_LISTBOX, 0x000120f2 },
    { WM_DRAWITEM, sent|wparam|lparam|parent, ID_LISTBOX, 0x001140f2 },

    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 1 },
    { EVENT_OBJECT_SELECTION, winevent_hook|wparam|lparam, OBJID_CLIENT, 1 },

    { WM_LBUTTONUP, sent|wparam|lparam, 0, 0 },
    { EVENT_SYSTEM_CAPTUREEND, winevent_hook|wparam|lparam, 0, 0 },
    { WM_CAPTURECHANGED, sent|wparam|lparam|defwinproc, 0, 0 },
    { WM_COMMAND, sent|wparam|parent, MAKEWPARAM(ID_LISTBOX, LBN_SELCHANGE) },
    { 0 }
};
static const struct message wm_lb_deletestring[] =
{
    { LB_DELETESTRING, sent|wparam|lparam, 0, 0 },
    { WM_DELETEITEM, sent|wparam|parent|optional, ID_LISTBOX, 0 },
    { WM_DRAWITEM, sent|wparam|parent|optional, ID_LISTBOX },
    { WM_DRAWITEM, sent|wparam|parent|optional, ID_LISTBOX },
    { 0 }
};
static const struct message wm_lb_deletestring_reset[] =
{
    { LB_DELETESTRING, sent|wparam|lparam, 0, 0 },
    { LB_RESETCONTENT, sent|wparam|lparam|defwinproc|optional, 0, 0 },
    { WM_DELETEITEM, sent|wparam|parent|optional, ID_LISTBOX, 0 },
    { WM_DRAWITEM, sent|wparam|parent|optional, ID_LISTBOX },
    { WM_DRAWITEM, sent|wparam|parent|optional, ID_LISTBOX },
    { 0 }
};

#define check_lb_state(a1, a2, a3, a4, a5) check_lb_state_dbg(a1, a2, a3, a4, a5, __LINE__)

static LRESULT (WINAPI *listbox_orig_proc)(HWND, UINT, WPARAM, LPARAM);

static LRESULT WINAPI listbox_hook_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
{
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct recvd_message msg;

    /* do not log painting messages */
    if (message != WM_PAINT &&
        message != WM_NCPAINT &&
        message != WM_SYNCPAINT &&
        message != WM_ERASEBKGND &&
        message != WM_NCHITTEST &&
        message != WM_GETTEXT &&
        !ignore_message( message ))
    {
        msg.hwnd = hwnd;
        msg.message = message;
        msg.flags = sent|wparam|lparam;
        if (defwndproc_counter) msg.flags |= defwinproc;
        msg.wParam = wp;
        msg.lParam = lp;
        msg.descr = "listbox";
        add_message(&msg);
    }

    defwndproc_counter++;
    ret = CallWindowProcA(listbox_orig_proc, hwnd, message, wp, lp);
    defwndproc_counter--;

    return ret;
}

static void check_lb_state_dbg(HWND listbox, int count, int cur_sel,
                               int caret_index, int top_index, int line)
{
    LRESULT ret;

    /* calling an orig proc helps to avoid unnecessary message logging */
    ret = CallWindowProcA(listbox_orig_proc, listbox, LB_GETCOUNT, 0, 0);
    ok_(__FILE__, line)(ret == count, "expected count %d, got %ld\n", count, ret);
    ret = CallWindowProcA(listbox_orig_proc, listbox, LB_GETCURSEL, 0, 0);
    ok_(__FILE__, line)(ret == cur_sel, "expected cur sel %d, got %ld\n", cur_sel, ret);
    ret = CallWindowProcA(listbox_orig_proc, listbox, LB_GETCARETINDEX, 0, 0);
    ok_(__FILE__, line)(ret == caret_index ||
                        broken(cur_sel == -1 && caret_index == 0 && ret == -1),  /* nt4 */
                        "expected caret index %d, got %ld\n", caret_index, ret);
    ret = CallWindowProcA(listbox_orig_proc, listbox, LB_GETTOPINDEX, 0, 0);
    ok_(__FILE__, line)(ret == top_index, "expected top index %d, got %ld\n", top_index, ret);
}

static void test_listbox_messages(void)
{
    HWND parent, listbox;
    LRESULT ret;

    parent = CreateWindowExA(0, "TestParentClass", NULL, WS_OVERLAPPEDWINDOW  | WS_VISIBLE,
                             100, 100, 200, 200, 0, 0, 0, NULL);
    listbox = CreateWindowExA(WS_EX_NOPARENTNOTIFY, "ListBox", NULL,
                              WS_CHILD | LBS_NOTIFY | LBS_OWNERDRAWVARIABLE | LBS_HASSTRINGS | WS_VISIBLE,
                              10, 10, 80, 80, parent, (HMENU)ID_LISTBOX, 0, NULL);
    listbox_orig_proc = (WNDPROC)SetWindowLongPtrA(listbox, GWLP_WNDPROC, (ULONG_PTR)listbox_hook_proc);

    check_lb_state(listbox, 0, LB_ERR, 0, 0);

    ret = SendMessageA(listbox, LB_ADDSTRING, 0, (LPARAM)"item 0");
    ok(ret == 0, "expected 0, got %ld\n", ret);
    ret = SendMessageA(listbox, LB_ADDSTRING, 0, (LPARAM)"item 1");
    ok(ret == 1, "expected 1, got %ld\n", ret);
    ret = SendMessageA(listbox, LB_ADDSTRING, 0, (LPARAM)"item 2");
    ok(ret == 2, "expected 2, got %ld\n", ret);

    check_lb_state(listbox, 3, LB_ERR, 0, 0);

    flush_sequence();

    log_all_parent_messages++;

    trace("selecting item 0\n");
    ret = SendMessageA(listbox, LB_SETCURSEL, 0, 0);
    ok(ret == 0, "expected 0, got %ld\n", ret);
    ok_sequence(wm_lb_setcursel_0, "LB_SETCURSEL 0", FALSE );
    check_lb_state(listbox, 3, 0, 0, 0);
    flush_sequence();

    trace("selecting item 1\n");
    ret = SendMessageA(listbox, LB_SETCURSEL, 1, 0);
    ok(ret == 1, "expected 1, got %ld\n", ret);
    ok_sequence(wm_lb_setcursel_1, "LB_SETCURSEL 1", FALSE );
    check_lb_state(listbox, 3, 1, 1, 0);

    trace("selecting item 2\n");
    ret = SendMessageA(listbox, LB_SETCURSEL, 2, 0);
    ok(ret == 2, "expected 2, got %ld\n", ret);
    ok_sequence(wm_lb_setcursel_2, "LB_SETCURSEL 2", FALSE );
    check_lb_state(listbox, 3, 2, 2, 0);

    trace("clicking on item 0\n");
    ret = SendMessageA(listbox, WM_LBUTTONDOWN, 0, MAKELPARAM(1, 1));
    ok(ret == LB_OKAY, "expected LB_OKAY, got %ld\n", ret);
    ret = SendMessageA(listbox, WM_LBUTTONUP, 0, 0);
    ok(ret == LB_OKAY, "expected LB_OKAY, got %ld\n", ret);
    ok_sequence(wm_lb_click_0, "WM_LBUTTONDOWN 0", FALSE );
    check_lb_state(listbox, 3, 0, 0, 0);
    flush_sequence();

    trace("deleting item 0\n");
    ret = SendMessageA(listbox, LB_DELETESTRING, 0, 0);
    ok(ret == 2, "expected 2, got %ld\n", ret);
    ok_sequence(wm_lb_deletestring, "LB_DELETESTRING 0", FALSE );
    check_lb_state(listbox, 2, -1, 0, 0);
    flush_sequence();

    trace("deleting item 0\n");
    ret = SendMessageA(listbox, LB_DELETESTRING, 0, 0);
    ok(ret == 1, "expected 1, got %ld\n", ret);
    ok_sequence(wm_lb_deletestring, "LB_DELETESTRING 0", FALSE );
    check_lb_state(listbox, 1, -1, 0, 0);
    flush_sequence();

    trace("deleting item 0\n");
    ret = SendMessageA(listbox, LB_DELETESTRING, 0, 0);
    ok(ret == 0, "expected 0, got %ld\n", ret);
    ok_sequence(wm_lb_deletestring_reset, "LB_DELETESTRING 0", FALSE );
    check_lb_state(listbox, 0, -1, 0, 0);
    flush_sequence();

    trace("deleting item 0\n");
    ret = SendMessageA(listbox, LB_DELETESTRING, 0, 0);
    ok(ret == LB_ERR, "expected LB_ERR, got %ld\n", ret);
    check_lb_state(listbox, 0, -1, 0, 0);
    flush_sequence();

    log_all_parent_messages--;

    DestroyWindow(listbox);
    DestroyWindow(parent);
}

/*************************** Menu test ******************************/
static const struct message wm_popup_menu_1[] =
{
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_MENU, 0x20000001 },
    { WM_SYSKEYDOWN, sent|wparam|lparam, VK_MENU, 0x20000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'E', 0x20000001 },
    { WM_SYSKEYDOWN, sent|wparam|lparam, 'E', 0x20000001 },
    { WM_SYSCHAR, sent|wparam|lparam, 'e', 0x20000001 },
    { HCBT_SYSCOMMAND, hook|wparam|lparam, SC_KEYMENU, 'e' },
    { WM_ENTERMENULOOP, sent|wparam|lparam, 0, 0 },
    { WM_INITMENU, sent|lparam, 0, 0 },
    { WM_MENUSELECT, sent|wparam, MAKEWPARAM(1,MF_HILITE|MF_POPUP) },
    { WM_INITMENUPOPUP, sent|lparam, 0, 1 },
    { HCBT_CREATEWND, hook|optional }, /* Win9x doesn't create a window */
    { WM_MENUSELECT, sent|wparam, MAKEWPARAM(200,MF_HILITE) },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'E', 0xf0000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_MENU, 0xd0000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_RETURN, 0x10000001, 0, 0x40000000 },
    { HCBT_DESTROYWND, hook|optional }, /* Win9x doesn't create a window */
    { WM_UNINITMENUPOPUP, sent|lparam, 0, 0 },
    { WM_MENUSELECT, sent|wparam|lparam, MAKEWPARAM(0,0xffff), 0 },
    { WM_EXITMENULOOP, sent|wparam|lparam, 0, 0 },
    { WM_MENUCOMMAND, sent }, /* |wparam, 200 - Win9x */
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_RETURN, 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, VK_RETURN, 0xc0000001 },
    { 0 }
};
static const struct message wm_popup_menu_2[] =
{
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_MENU, 0x20000001 },
    { WM_SYSKEYDOWN, sent|wparam|lparam, VK_MENU, 0x20000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'F', 0x20000001 },
    { WM_SYSKEYDOWN, sent|wparam|lparam, 'F', 0x20000001 },
    { WM_SYSCHAR, sent|wparam|lparam, 'f', 0x20000001 },
    { HCBT_SYSCOMMAND, hook|wparam|lparam, SC_KEYMENU, 'f' },
    { WM_ENTERMENULOOP, sent|wparam|lparam, 0, 0 },
    { WM_INITMENU, sent|lparam, 0, 0 },
    { WM_MENUSELECT, sent|wparam, MAKEWPARAM(0,MF_HILITE|MF_POPUP) },
    { WM_INITMENUPOPUP, sent|lparam, 0, 0 },
    { WM_MENUSELECT, sent|wparam|optional, MAKEWPARAM(0,MF_HILITE|MF_POPUP) }, /* Win9x */
    { WM_INITMENUPOPUP, sent|lparam|optional, 0, 0 }, /* Win9x */
    { HCBT_CREATEWND, hook },
    { WM_MENUSELECT, sent }, /*|wparam, MAKEWPARAM(0,MF_HILITE|MF_POPUP) - XP
                               |wparam, MAKEWPARAM(100,MF_HILITE) - Win9x */
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'F', 0xf0000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_MENU, 0xd0000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_RIGHT, 0x10000001 },
    { WM_INITMENUPOPUP, sent|lparam|optional, 0, 0 }, /* Win9x doesn't send it */
    { HCBT_CREATEWND, hook|optional }, /* Win9x doesn't send it */
    { WM_MENUSELECT, sent|wparam|optional, MAKEWPARAM(100,MF_HILITE) },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_RIGHT, 0xd0000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_RETURN, 0x10000001 },
    { HCBT_DESTROYWND, hook },
    { WM_UNINITMENUPOPUP, sent|lparam, 0, 0 },
    { HCBT_DESTROYWND, hook|optional }, /* Win9x doesn't send it */
    { WM_UNINITMENUPOPUP, sent|lparam, 0, 0 },
    { WM_MENUSELECT, sent|wparam|lparam, MAKEWPARAM(0,0xffff), 0 },
    { WM_EXITMENULOOP, sent|wparam|lparam, 0, 0 },
    { WM_MENUCOMMAND, sent }, /* |wparam, 100 - Win9x */
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_RETURN, 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, VK_RETURN, 0xc0000001 },
    { 0 }
};
static const struct message wm_popup_menu_3[] =
{
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_MENU, 0x20000001 },
    { WM_SYSKEYDOWN, sent|wparam|lparam, VK_MENU, 0x20000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'F', 0x20000001 },
    { WM_SYSKEYDOWN, sent|wparam|lparam, 'F', 0x20000001 },
    { WM_SYSCHAR, sent|wparam|lparam, 'f', 0x20000001 },
    { HCBT_SYSCOMMAND, hook|wparam|lparam, SC_KEYMENU, 'f' },
    { WM_ENTERMENULOOP, sent|wparam|lparam, 0, 0 },
    { WM_INITMENU, sent|lparam, 0, 0 },
    { WM_MENUSELECT, sent|wparam, MAKEWPARAM(0,MF_HILITE|MF_POPUP) },
    { WM_INITMENUPOPUP, sent|lparam, 0, 0 },
    { WM_MENUSELECT, sent|wparam|optional, MAKEWPARAM(0,MF_HILITE|MF_POPUP) }, /* Win9x */
    { WM_INITMENUPOPUP, sent|lparam|optional, 0, 0 }, /* Win9x */
    { HCBT_CREATEWND, hook },
    { WM_MENUSELECT, sent }, /*|wparam, MAKEWPARAM(0,MF_HILITE|MF_POPUP) - XP
                               |wparam, MAKEWPARAM(100,MF_HILITE) - Win9x */
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'F', 0xf0000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_MENU, 0xd0000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_RIGHT, 0x10000001 },
    { WM_INITMENUPOPUP, sent|lparam|optional, 0, 0 }, /* Win9x doesn't send it */
    { HCBT_CREATEWND, hook|optional }, /* Win9x doesn't send it */
    { WM_MENUSELECT, sent|wparam|optional, MAKEWPARAM(100,MF_HILITE) },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_RIGHT, 0xd0000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_RETURN, 0x10000001 },
    { HCBT_DESTROYWND, hook },
    { WM_UNINITMENUPOPUP, sent|lparam, 0, 0 },
    { HCBT_DESTROYWND, hook|optional }, /* Win9x doesn't send it */
    { WM_UNINITMENUPOPUP, sent|lparam, 0, 0 },
    { WM_MENUSELECT, sent|wparam|lparam, MAKEWPARAM(0,0xffff), 0 },
    { WM_EXITMENULOOP, sent|wparam|lparam, 0, 0 },
    { WM_COMMAND, sent|wparam|lparam, 100, 0 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_RETURN, 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, VK_RETURN, 0xc0000001 },
    { 0 }
};

static const struct message wm_single_menu_item[] =
{
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_MENU, 0x20000001 },
    { WM_SYSKEYDOWN, sent|wparam|lparam, VK_MENU, 0x20000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'Q', 0x20000001 },
    { WM_SYSKEYDOWN, sent|wparam|lparam, 'Q', 0x20000001 },
    { WM_SYSCHAR, sent|wparam|lparam, 'q', 0x20000001 },
    { HCBT_SYSCOMMAND, hook|wparam|lparam, SC_KEYMENU, 'q' },
    { WM_ENTERMENULOOP, sent|wparam|lparam, 0, 0 },
    { WM_INITMENU, sent|lparam, 0, 0 },
    { WM_MENUSELECT, sent|wparam|optional, MAKEWPARAM(300,MF_HILITE) },
    { WM_MENUSELECT, sent|wparam|lparam, MAKEWPARAM(0,0xffff), 0 },
    { WM_EXITMENULOOP, sent|wparam|lparam, 0, 0 },
    { WM_MENUCOMMAND, sent },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 'Q', 0xe0000001 },
    { WM_SYSKEYUP, sent|wparam|lparam, 'Q', 0xe0000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_MENU, 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, VK_MENU, 0xc0000001 },

    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_ESCAPE, 1 },
    { WM_KEYDOWN, sent|wparam|lparam, VK_ESCAPE, 1 },
    { WM_CHAR,  sent|wparam|lparam, VK_ESCAPE, 0x00000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_ESCAPE, 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, VK_ESCAPE, 0xc0000001 },
    { 0 }
};

static LRESULT WINAPI parent_menu_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
{
    if (message == WM_ENTERIDLE ||
        message == WM_INITMENU ||
        message == WM_INITMENUPOPUP ||
        message == WM_MENUSELECT ||
        message == WM_PARENTNOTIFY ||
        message == WM_ENTERMENULOOP ||
        message == WM_EXITMENULOOP ||
        message == WM_UNINITMENUPOPUP ||
        message == WM_KEYDOWN ||
        message == WM_KEYUP ||
        message == WM_CHAR ||
        message == WM_SYSKEYDOWN ||
        message == WM_SYSKEYUP ||
        message == WM_SYSCHAR ||
        message == WM_COMMAND ||
        message == WM_MENUCOMMAND)
    {
        struct recvd_message msg;

        msg.hwnd = hwnd;
        msg.message = message;
        msg.flags = sent|wparam|lparam;
        msg.wParam = wp;
        msg.lParam = lp;
        msg.descr = "parent_menu_proc";
        add_message(&msg);
    }

    return DefWindowProcA(hwnd, message, wp, lp);
}

static void set_menu_style(HMENU hmenu, DWORD style)
{
    MENUINFO mi;
    BOOL ret;

    mi.cbSize = sizeof(mi);
    mi.fMask = MIM_STYLE;
    mi.dwStyle = style;
    SetLastError(0xdeadbeef);
    ret = pSetMenuInfo(hmenu, &mi);
    ok(ret, "SetMenuInfo error %u\n", GetLastError());
}

static DWORD get_menu_style(HMENU hmenu)
{
    MENUINFO mi;
    BOOL ret;

    mi.cbSize = sizeof(mi);
    mi.fMask = MIM_STYLE;
    mi.dwStyle = 0;
    SetLastError(0xdeadbeef);
    ret = pGetMenuInfo(hmenu, &mi);
    ok(ret, "GetMenuInfo error %u\n", GetLastError());

    return mi.dwStyle;
}

static void test_menu_messages(void)
{
    MSG msg;
    WNDCLASSA cls;
    HMENU hmenu, hmenu_popup;
    HWND hwnd;
    DWORD style;

    if (!pGetMenuInfo || !pSetMenuInfo)
    {
        win_skip("GetMenuInfo and/or SetMenuInfo are not available\n");
        return;
    }
    cls.style = 0;
    cls.lpfnWndProc = parent_menu_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "TestMenuClass";
    UnregisterClassA(cls.lpszClassName, cls.hInstance);
    if (!RegisterClassA(&cls)) assert(0);

    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExA(0, "TestMenuClass", NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                           100, 100, 200, 200, 0, 0, 0, NULL);
    ok(hwnd != 0, "LoadMenuA error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    hmenu = LoadMenuA(GetModuleHandleA(NULL), MAKEINTRESOURCEA(1));
    ok(hmenu != 0, "LoadMenuA error %u\n", GetLastError());

    SetMenu(hwnd, hmenu);
    SetForegroundWindow( hwnd );
    flush_events();

    set_menu_style(hmenu, MNS_NOTIFYBYPOS);
    style = get_menu_style(hmenu);
    ok(style == MNS_NOTIFYBYPOS, "expected MNS_NOTIFYBYPOS, got %u\n", style);

    hmenu_popup = GetSubMenu(hmenu, 0);
    ok(hmenu_popup != 0, "GetSubMenu returned 0 for submenu 0\n");
    style = get_menu_style(hmenu_popup);
    ok(style == 0, "expected 0, got %u\n", style);

    hmenu_popup = GetSubMenu(hmenu_popup, 0);
    ok(hmenu_popup != 0, "GetSubMenu returned 0 for submenu 0\n");
    style = get_menu_style(hmenu_popup);
    ok(style == 0, "expected 0, got %u\n", style);

    /* Alt+E, Enter */
    trace("testing a popup menu command\n");
    flush_sequence();
    keybd_event(VK_MENU, 0, 0, 0);
    keybd_event('E', 0, 0, 0);
    keybd_event('E', 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_RETURN, 0, 0, 0);
    keybd_event(VK_RETURN, 0, KEYEVENTF_KEYUP, 0);
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    if (!sequence_cnt)  /* we didn't get any message */
    {
        skip( "queuing key events not supported\n" );
        goto done;
    }
    /* win98 queues only a WM_KEYUP and doesn't start menu tracking */
    if (sequence[0].message == WM_KEYUP && sequence[0].wParam == VK_MENU)
    {
        win_skip( "menu tracking through VK_MENU not supported\n" );
        goto done;
    }
    ok_sequence(wm_popup_menu_1, "popup menu command", FALSE);

    /* Alt+F, Right, Enter */
    trace("testing submenu of a popup menu command\n");
    flush_sequence();
    keybd_event(VK_MENU, 0, 0, 0);
    keybd_event('F', 0, 0, 0);
    keybd_event('F', 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_RIGHT, 0, 0, 0);
    keybd_event(VK_RIGHT, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_RETURN, 0, 0, 0);
    keybd_event(VK_RETURN, 0, KEYEVENTF_KEYUP, 0);
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    ok_sequence(wm_popup_menu_2, "submenu of a popup menu command", FALSE);

    trace("testing single menu item command\n");
    flush_sequence();
    keybd_event(VK_MENU, 0, 0, 0);
    keybd_event('Q', 0, 0, 0);
    keybd_event('Q', 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_ESCAPE, 0, 0, 0);
    keybd_event(VK_ESCAPE, 0, KEYEVENTF_KEYUP, 0);
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    ok_sequence(wm_single_menu_item, "single menu item command", FALSE);

    set_menu_style(hmenu, 0);
    style = get_menu_style(hmenu);
    ok(style == 0, "expected 0, got %u\n", style);

    hmenu_popup = GetSubMenu(hmenu, 0);
    ok(hmenu_popup != 0, "GetSubMenu returned 0 for submenu 0\n");
    set_menu_style(hmenu_popup, MNS_NOTIFYBYPOS);
    style = get_menu_style(hmenu_popup);
    ok(style == MNS_NOTIFYBYPOS, "expected MNS_NOTIFYBYPOS, got %u\n", style);

    hmenu_popup = GetSubMenu(hmenu_popup, 0);
    ok(hmenu_popup != 0, "GetSubMenu returned 0 for submenu 0\n");
    style = get_menu_style(hmenu_popup);
    ok(style == 0, "expected 0, got %u\n", style);

    /* Alt+F, Right, Enter */
    trace("testing submenu of a popup menu command\n");
    flush_sequence();
    keybd_event(VK_MENU, 0, 0, 0);
    keybd_event('F', 0, 0, 0);
    keybd_event('F', 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_RIGHT, 0, 0, 0);
    keybd_event(VK_RIGHT, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_RETURN, 0, 0, 0);
    keybd_event(VK_RETURN, 0, KEYEVENTF_KEYUP, 0);
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    ok_sequence(wm_popup_menu_3, "submenu of a popup menu command", FALSE);

done:
    DestroyWindow(hwnd);
    DestroyMenu(hmenu);
}


static void test_paintingloop(void)
{
    HWND hwnd;

    paint_loop_done = FALSE;
    hwnd = CreateWindowExA(0x0,"PaintLoopWindowClass",
                               "PaintLoopWindowClass",WS_OVERLAPPEDWINDOW,
                                100, 100, 100, 100, 0, 0, 0, NULL );
    ok(hwnd != 0, "PaintLoop window error %u\n", GetLastError());
    ShowWindow(hwnd,SW_NORMAL);
    SetFocus(hwnd);

    while (!paint_loop_done)
    {
        MSG msg;
        if (PeekMessageA(&msg, 0, 0, 0, 1))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }
    DestroyWindow(hwnd);
}

static void test_defwinproc(void)
{
    HWND hwnd;
    MSG msg;
    BOOL gotwmquit = FALSE;
    hwnd = CreateWindowExA(0, "static", "test_defwndproc", WS_POPUP, 0,0,0,0,0,0,0, NULL);
    assert(hwnd);
    DefWindowProcA( hwnd, WM_ENDSESSION, 1, 0);
    while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE )) {
        if( msg.message == WM_QUIT) gotwmquit = TRUE;
        DispatchMessageA( &msg );
    }
    ok(!gotwmquit, "Unexpected WM_QUIT message!\n");
    DestroyWindow( hwnd);
}

#define clear_clipboard(hwnd)  clear_clipboard_(__LINE__, (hwnd))
static void clear_clipboard_(int line, HWND hWnd)
{
    BOOL succ;
    succ = OpenClipboard(hWnd);
    ok_(__FILE__, line)(succ, "OpenClipboard failed, err=%u\n", GetLastError());
    succ = EmptyClipboard();
    ok_(__FILE__, line)(succ, "EmptyClipboard failed, err=%u\n", GetLastError());
    succ = CloseClipboard();
    ok_(__FILE__, line)(succ, "CloseClipboard failed, err=%u\n", GetLastError());
}

#define expect_HWND(expected, got) expect_HWND_(__LINE__, (expected), (got))
static void expect_HWND_(int line, HWND expected, HWND got)
{
    ok_(__FILE__, line)(got==expected, "Expected %p, got %p\n", expected, got);
}

static WNDPROC pOldViewerProc;

static LRESULT CALLBACK recursive_viewer_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static BOOL recursion_guard;

    if (message == WM_DRAWCLIPBOARD && !recursion_guard)
    {
        recursion_guard = TRUE;
        clear_clipboard(hWnd);
        recursion_guard = FALSE;
    }
    return CallWindowProcA(pOldViewerProc, hWnd, message, wParam, lParam);
}

static void test_clipboard_viewers(void)
{
    static struct message wm_change_cb_chain[] =
    {
        { WM_CHANGECBCHAIN, sent|wparam|lparam, 0, 0 },
        { 0 }
    };
    static const struct message wm_clipboard_destroyed[] =
    {
        { WM_DESTROYCLIPBOARD, sent|wparam|lparam, 0, 0 },
        { 0 }
    };
    static struct message wm_clipboard_changed[] =
    {
        { WM_DRAWCLIPBOARD, sent|wparam|lparam, 0, 0 },
        { 0 }
    };
    static struct message wm_clipboard_changed_and_owned[] =
    {
        { WM_DESTROYCLIPBOARD, sent|wparam|lparam, 0, 0 },
        { WM_DRAWCLIPBOARD, sent|wparam|lparam, 0, 0 },
        { 0 }
    };

    HINSTANCE hInst = GetModuleHandleA(NULL);
    HWND hWnd1, hWnd2, hWnd3;
    HWND hOrigViewer;
    HWND hRet;

    hWnd1 = CreateWindowExA(0, "TestWindowClass", "Clipboard viewer test wnd 1",
        WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        GetDesktopWindow(), NULL, hInst, NULL);
    hWnd2 = CreateWindowExA(0, "SimpleWindowClass", "Clipboard viewer test wnd 2",
        WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        GetDesktopWindow(), NULL, hInst, NULL);
    hWnd3 = CreateWindowExA(0, "SimpleWindowClass", "Clipboard viewer test wnd 3",
        WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        GetDesktopWindow(), NULL, hInst, NULL);
    trace("clipbd viewers: hWnd1=%p, hWnd2=%p, hWnd3=%p\n", hWnd1, hWnd2, hWnd3);
    assert(hWnd1 && hWnd2 && hWnd3);

    flush_sequence();

    /* Test getting the clipboard viewer and setting the viewer to NULL. */
    hOrigViewer = GetClipboardViewer();
    hRet = SetClipboardViewer(NULL);
    ok_sequence(WmEmptySeq, "set viewer to NULL", FALSE);
    expect_HWND(hOrigViewer, hRet);
    expect_HWND(NULL, GetClipboardViewer());

    /* Test registering hWnd1 as a viewer. */
    hRet = SetClipboardViewer(hWnd1);
    wm_clipboard_changed[0].wParam = (WPARAM) GetClipboardOwner();
    ok_sequence(wm_clipboard_changed, "set viewer NULL->1", FALSE);
    expect_HWND(NULL, hRet);
    expect_HWND(hWnd1, GetClipboardViewer());

    /* Test that changing the clipboard actually refreshes the registered viewer. */
    clear_clipboard(hWnd1);
    wm_clipboard_changed[0].wParam = (WPARAM) GetClipboardOwner();
    ok_sequence(wm_clipboard_changed, "clear clipbd (viewer=owner=1)", FALSE);

    /* Again, but with different owner. */
    clear_clipboard(hWnd2);
    wm_clipboard_changed_and_owned[1].wParam = (WPARAM) GetClipboardOwner();
    ok_sequence(wm_clipboard_changed_and_owned, "clear clipbd (viewer=1, owner=2)", FALSE);

    /* Test re-registering same window. */
    hRet = SetClipboardViewer(hWnd1);
    wm_clipboard_changed[0].wParam = (WPARAM) GetClipboardOwner();
    ok_sequence(wm_clipboard_changed, "set viewer 1->1", FALSE);
    expect_HWND(hWnd1, hRet);
    expect_HWND(hWnd1, GetClipboardViewer());

    /* Test ChangeClipboardChain. */
    ChangeClipboardChain(hWnd2, hWnd3);
    wm_change_cb_chain[0].wParam = (WPARAM) hWnd2;
    wm_change_cb_chain[0].lParam = (LPARAM) hWnd3;
    ok_sequence(wm_change_cb_chain, "change chain (viewer=1, remove=2, next=3)", FALSE);
    expect_HWND(hWnd1, GetClipboardViewer());

    ChangeClipboardChain(hWnd2, NULL);
    wm_change_cb_chain[0].wParam = (WPARAM) hWnd2;
    wm_change_cb_chain[0].lParam = 0;
    ok_sequence(wm_change_cb_chain, "change chain (viewer=1, remove=2, next=NULL)", FALSE);
    expect_HWND(hWnd1, GetClipboardViewer());

    ChangeClipboardChain(NULL, hWnd2);
    ok_sequence(WmEmptySeq, "change chain (viewer=1, remove=NULL, next=2)", TRUE);
    expect_HWND(hWnd1, GetClipboardViewer());

    /* Actually change clipboard viewer with ChangeClipboardChain. */
    ChangeClipboardChain(hWnd1, hWnd2);
    ok_sequence(WmEmptySeq, "change chain (viewer=remove=1, next=2)", FALSE);
    expect_HWND(hWnd2, GetClipboardViewer());

    /* Test that no refresh messages are sent when viewer has unregistered. */
    clear_clipboard(hWnd2);
    ok_sequence(WmEmptySeq, "clear clipd (viewer=2, owner=1)", FALSE);

    /* Register hWnd1 again. */
    ChangeClipboardChain(hWnd2, hWnd1);
    ok_sequence(WmEmptySeq, "change chain (viewer=remove=2, next=1)", FALSE);
    expect_HWND(hWnd1, GetClipboardViewer());

    /* Subclass hWnd1 so that when it receives a WM_DRAWCLIPBOARD message, it
     * changes the clipboard. When this happens, the system shouldn't send
     * another WM_DRAWCLIPBOARD (as this could cause an infinite loop).
     */
    pOldViewerProc = (WNDPROC) SetWindowLongPtrA(hWnd1, GWLP_WNDPROC, (LONG_PTR) recursive_viewer_proc);
    clear_clipboard(hWnd2);
    /* The clipboard owner is changed in recursive_viewer_proc: */
    wm_clipboard_changed[0].wParam = (WPARAM) hWnd2;
    ok_sequence(wm_clipboard_changed, "recursive clear clipbd (viewer=1, owner=2)", TRUE);

    /* Test unregistering. */
    ChangeClipboardChain(hWnd1, NULL);
    ok_sequence(WmEmptySeq, "change chain (viewer=remove=1, next=NULL)", FALSE);
    expect_HWND(NULL, GetClipboardViewer());

    clear_clipboard(hWnd1);
    ok_sequence(wm_clipboard_destroyed, "clear clipbd (no viewer, owner=1)", FALSE);

    DestroyWindow(hWnd1);
    DestroyWindow(hWnd2);
    DestroyWindow(hWnd3);
    SetClipboardViewer(hOrigViewer);
}

static void test_PostMessage(void)
{
    static const struct
    {
        HWND hwnd;
        BOOL ret;
    } data[] =
    {
        { HWND_TOP /* 0 */, TRUE },
        { HWND_BROADCAST, TRUE },
        { HWND_BOTTOM, TRUE },
        { HWND_TOPMOST, TRUE },
        { HWND_NOTOPMOST, FALSE },
        { HWND_MESSAGE, FALSE },
        { (HWND)0xdeadbeef, FALSE }
    };
    int i;
    HWND hwnd;
    BOOL ret;
    MSG msg;
    static const WCHAR staticW[] = {'s','t','a','t','i','c',0};

    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExW(0, staticW, NULL, WS_POPUP, 0,0,0,0,0,0,0, NULL);
    if (!hwnd && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("Skipping some PostMessage tests on Win9x/WinMe\n");
        return;
    }
    assert(hwnd);

    flush_events();

    PostMessageA(hwnd, WM_USER+1, 0x1234, 0x5678);
    PostMessageA(0, WM_USER+2, 0x5678, 0x1234);

    for (i = 0; i < sizeof(data)/sizeof(data[0]); i++)
    {
        memset(&msg, 0xab, sizeof(msg));
        ret = PeekMessageA(&msg, data[i].hwnd, 0, 0, PM_NOREMOVE);
        ok(ret == data[i].ret, "%d: hwnd %p expected %d, got %d\n", i, data[i].hwnd, data[i].ret, ret);
        if (data[i].ret)
        {
            if (data[i].hwnd)
                ok(ret && msg.hwnd == 0 && msg.message == WM_USER+2 &&
                   msg.wParam == 0x5678 && msg.lParam == 0x1234,
                   "%d: got ret %d hwnd %p msg %04x wParam %08lx lParam %08lx instead of TRUE/0/WM_USER+2/0x5678/0x1234\n",
                   i, ret, msg.hwnd, msg.message, msg.wParam, msg.lParam);
            else
                ok(ret && msg.hwnd == hwnd && msg.message == WM_USER+1 &&
                   msg.wParam == 0x1234 && msg.lParam == 0x5678,
                   "%d: got ret %d hwnd %p msg %04x wParam %08lx lParam %08lx instead of TRUE/%p/WM_USER+1/0x1234/0x5678\n",
                   i, ret, msg.hwnd, msg.message, msg.wParam, msg.lParam, msg.hwnd);
        }
    }

    DestroyWindow(hwnd);
    flush_events();
}

static const struct
{
    DWORD exp, broken;
    BOOL todo;
} wait_idle_expect[] =
{
/* 0 */  { WAIT_TIMEOUT, WAIT_TIMEOUT, FALSE },
         { WAIT_TIMEOUT, 0,            FALSE },
         { WAIT_TIMEOUT, 0,            FALSE },
         { WAIT_TIMEOUT, WAIT_TIMEOUT, FALSE },
         { WAIT_TIMEOUT, WAIT_TIMEOUT, FALSE },
/* 5 */  { WAIT_TIMEOUT, 0,            FALSE },
         { WAIT_TIMEOUT, 0,            FALSE },
         { WAIT_TIMEOUT, WAIT_TIMEOUT, FALSE },
         { 0,            0,            FALSE },
         { 0,            0,            FALSE },
/* 10 */ { 0,            0,            FALSE },
         { 0,            0,            FALSE },
         { 0,            WAIT_TIMEOUT, FALSE },
         { 0,            0,            FALSE },
         { 0,            0,            FALSE },
/* 15 */ { 0,            0,            FALSE },
         { WAIT_TIMEOUT, 0,            FALSE },
         { WAIT_TIMEOUT, 0,            FALSE },
         { WAIT_TIMEOUT, 0,            FALSE },
         { WAIT_TIMEOUT, 0,            FALSE },
/* 20 */ { WAIT_TIMEOUT, 0,            FALSE },
};

static DWORD CALLBACK do_wait_idle_child_thread( void *arg )
{
    MSG msg;

    PeekMessageA( &msg, 0, 0, 0, PM_NOREMOVE );
    Sleep( 200 );
    MsgWaitForMultipleObjects( 0, NULL, FALSE, 100, QS_ALLINPUT );
    return 0;
}

static void do_wait_idle_child( int arg )
{
    WNDCLASSA cls;
    MSG msg;
    HWND hwnd = 0;
    HANDLE thread;
    DWORD id;
    HANDLE start_event = OpenEventA( EVENT_ALL_ACCESS, FALSE, "test_WaitForInputIdle_start" );
    HANDLE end_event = OpenEventA( EVENT_ALL_ACCESS, FALSE, "test_WaitForInputIdle_end" );

    memset( &cls, 0, sizeof(cls) );
    cls.lpfnWndProc   = DefWindowProcA;
    cls.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    cls.hCursor       = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.lpszClassName = "TestClass";
    RegisterClassA( &cls );

    PeekMessageA( &msg, 0, 0, 0, PM_NOREMOVE );  /* create the msg queue */

    ok( start_event != 0, "failed to create start event, error %u\n", GetLastError() );
    ok( end_event != 0, "failed to create end event, error %u\n", GetLastError() );

    switch (arg)
    {
    case 0:
        SetEvent( start_event );
        break;
    case 1:
        SetEvent( start_event );
        Sleep( 200 );
        PeekMessageA( &msg, 0, 0, 0, PM_REMOVE );
        break;
    case 2:
        SetEvent( start_event );
        Sleep( 200 );
        PeekMessageA( &msg, 0, 0, 0, PM_NOREMOVE );
        PostThreadMessageA( GetCurrentThreadId(), WM_COMMAND, 0x1234, 0xabcd );
        PeekMessageA( &msg, 0, 0, 0, PM_REMOVE );
        break;
    case 3:
        SetEvent( start_event );
        Sleep( 200 );
        SendMessageA( HWND_BROADCAST, WM_WININICHANGE, 0, 0 );
        break;
    case 4:
        SetEvent( start_event );
        Sleep( 200 );
        hwnd = CreateWindowExA(0, "TestClass", NULL, WS_POPUP|WS_VISIBLE, 0, 0, 10, 10, 0, 0, 0, NULL);
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE|PM_NOYIELD )) DispatchMessageA( &msg );
        break;
    case 5:
        SetEvent( start_event );
        Sleep( 200 );
        hwnd = CreateWindowExA(0, "TestClass", NULL, WS_POPUP|WS_VISIBLE, 0, 0, 10, 10, 0, 0, 0, NULL);
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
        break;
    case 6:
        SetEvent( start_event );
        Sleep( 200 );
        hwnd = CreateWindowExA(0, "TestClass", NULL, WS_POPUP|WS_VISIBLE, 0, 0, 10, 10, 0, 0, 0, NULL);
        while (PeekMessageA( &msg, 0, 0, 0, PM_NOREMOVE ))
        {
            GetMessageA( &msg, 0, 0, 0 );
            DispatchMessageA( &msg );
        }
        break;
    case 7:
        SetEvent( start_event );
        Sleep( 200 );
        hwnd = CreateWindowExA(0, "TestClass", NULL, WS_POPUP|WS_VISIBLE, 0, 0, 10, 10, 0, 0, 0, NULL);
        SetTimer( hwnd, 3, 1, NULL );
        Sleep( 200 );
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE|PM_NOYIELD )) DispatchMessageA( &msg );
        break;
    case 8:
        SetEvent( start_event );
        Sleep( 200 );
        PeekMessageA( &msg, 0, 0, 0, PM_NOREMOVE );
        MsgWaitForMultipleObjects( 0, NULL, FALSE, 100, QS_ALLINPUT );
        break;
    case 9:
        SetEvent( start_event );
        Sleep( 200 );
        hwnd = CreateWindowExA(0, "TestClass", NULL, WS_POPUP|WS_VISIBLE, 0, 0, 10, 10, 0, 0, 0, NULL);
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
        for (;;) GetMessageA( &msg, 0, 0, 0 );
        break;
    case 10:
        SetEvent( start_event );
        Sleep( 200 );
        hwnd = CreateWindowExA(0, "TestClass", NULL, WS_POPUP|WS_VISIBLE, 0, 0, 10, 10, 0, 0, 0, NULL);
        SetTimer( hwnd, 3, 1, NULL );
        Sleep( 200 );
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
        break;
    case 11:
        SetEvent( start_event );
        Sleep( 200 );
        return;  /* exiting the process makes WaitForInputIdle return success too */
    case 12:
        PeekMessageA( &msg, 0, 0, 0, PM_NOREMOVE );
        Sleep( 200 );
        MsgWaitForMultipleObjects( 0, NULL, FALSE, 100, QS_ALLINPUT );
        SetEvent( start_event );
        break;
    case 13:
        SetEvent( start_event );
        PeekMessageA( &msg, 0, 0, 0, PM_NOREMOVE );
        Sleep( 200 );
        thread = CreateThread( NULL, 0, do_wait_idle_child_thread, NULL, 0, &id );
        WaitForSingleObject( thread, 10000 );
        CloseHandle( thread );
        break;
    case 14:
        SetEvent( start_event );
        Sleep( 200 );
        PeekMessageA( &msg, HWND_TOPMOST, 0, 0, PM_NOREMOVE );
        break;
    case 15:
        SetEvent( start_event );
        Sleep( 200 );
        PeekMessageA( &msg, HWND_BROADCAST, 0, 0, PM_NOREMOVE );
        break;
    case 16:
        SetEvent( start_event );
        Sleep( 200 );
        PeekMessageA( &msg, HWND_BOTTOM, 0, 0, PM_NOREMOVE );
        break;
    case 17:
        SetEvent( start_event );
        Sleep( 200 );
        PeekMessageA( &msg, (HWND)0xdeadbeef, 0, 0, PM_NOREMOVE );
        break;
    case 18:
        SetEvent( start_event );
        Sleep( 200 );
        PeekMessageA( &msg, HWND_NOTOPMOST, 0, 0, PM_NOREMOVE );
        break;
    case 19:
        SetEvent( start_event );
        Sleep( 200 );
        PeekMessageA( &msg, HWND_MESSAGE, 0, 0, PM_NOREMOVE );
        break;
    case 20:
        SetEvent( start_event );
        Sleep( 200 );
        PeekMessageA( &msg, GetDesktopWindow(), 0, 0, PM_NOREMOVE );
        break;
    }
    WaitForSingleObject( end_event, 2000 );
    CloseHandle( start_event );
    CloseHandle( end_event );
    if (hwnd) DestroyWindow( hwnd );
}

static LRESULT CALLBACK wait_idle_proc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    if (msg == WM_WININICHANGE) Sleep( 200 );  /* make sure the child waits */
    return DefWindowProcA( hwnd, msg, wp, lp );
}

static DWORD CALLBACK wait_idle_thread( void *arg )
{
    WNDCLASSA cls;
    MSG msg;
    HWND hwnd;

    memset( &cls, 0, sizeof(cls) );
    cls.lpfnWndProc   = wait_idle_proc;
    cls.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    cls.hCursor       = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.lpszClassName = "TestClass";
    RegisterClassA( &cls );

    hwnd = CreateWindowExA(0, "TestClass", NULL, WS_POPUP, 0, 0, 10, 10, 0, 0, 0, NULL);
    while (GetMessageA( &msg, 0, 0, 0 )) DispatchMessageA( &msg );
    DestroyWindow(hwnd);
    return 0;
}

static void test_WaitForInputIdle( char *argv0 )
{
    char path[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOA startup;
    BOOL ret;
    HANDLE start_event, end_event, thread;
    unsigned int i;
    DWORD id;
    const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)GetModuleHandleA(0);
    const IMAGE_NT_HEADERS *nt = (const IMAGE_NT_HEADERS *)((const char *)dos + dos->e_lfanew);
    BOOL console_app = (nt->OptionalHeader.Subsystem != IMAGE_SUBSYSTEM_WINDOWS_GUI);

    if (console_app)  /* build the test with -mwindows for better coverage */
        trace( "not built as a GUI app, WaitForInputIdle may not be fully tested\n" );

    start_event = CreateEventA(NULL, 0, 0, "test_WaitForInputIdle_start");
    end_event = CreateEventA(NULL, 0, 0, "test_WaitForInputIdle_end");
    ok(start_event != 0, "failed to create start event, error %u\n", GetLastError());
    ok(end_event != 0, "failed to create end event, error %u\n", GetLastError());

    memset( &startup, 0, sizeof(startup) );
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    thread = CreateThread( NULL, 0, wait_idle_thread, NULL, 0, &id );

    for (i = 0; i < sizeof(wait_idle_expect)/sizeof(wait_idle_expect[0]); i++)
    {
        ResetEvent( start_event );
        ResetEvent( end_event );
#if 0
        sprintf( path, "%s msg %u", argv0, i );
#else
        sprintf( path, "%s msg_queue %u", argv0, i );
#endif
        ret = CreateProcessA( NULL, path, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &pi );
        ok( ret, "CreateProcess '%s' failed err %u.\n", path, GetLastError() );
        if (ret)
        {
            ret = WaitForSingleObject( start_event, 5000 );
            ok( ret == WAIT_OBJECT_0, "%u: WaitForSingleObject failed\n", i );
            if (ret == WAIT_OBJECT_0)
            {
                ret = WaitForInputIdle( pi.hProcess, 1000 );
                if (ret == WAIT_FAILED)
                    ok( console_app ||
                        ret == wait_idle_expect[i].exp ||
                        broken(ret == wait_idle_expect[i].broken),
                        "%u: WaitForInputIdle error %08x expected %08x\n",
                        i, ret, wait_idle_expect[i].exp );
                else if (wait_idle_expect[i].todo)
                    todo_wine
                    ok( ret == wait_idle_expect[i].exp || broken(ret == wait_idle_expect[i].broken),
                        "%u: WaitForInputIdle error %08x expected %08x\n",
                        i, ret, wait_idle_expect[i].exp );
                else
                    ok( ret == wait_idle_expect[i].exp || broken(ret == wait_idle_expect[i].broken),
                        "%u: WaitForInputIdle error %08x expected %08x\n",
                        i, ret, wait_idle_expect[i].exp );
                SetEvent( end_event );
                WaitForSingleObject( pi.hProcess, 1000 );  /* give it a chance to exit on its own */
            }
            TerminateProcess( pi.hProcess, 0 );  /* just in case */
            winetest_wait_child_process( pi.hProcess );
            ret = WaitForInputIdle( pi.hProcess, 100 );
            ok( ret == WAIT_FAILED, "%u: WaitForInputIdle after exit error %08x\n", i, ret );
            CloseHandle( pi.hProcess );
            CloseHandle( pi.hThread );
        }
    }
    CloseHandle( start_event );
    PostThreadMessageA( id, WM_QUIT, 0, 0 );
    WaitForSingleObject( thread, 10000 );
    CloseHandle( thread );
}

static const struct message WmSetParentSeq_1[] = {
    { WM_SHOWWINDOW, sent|wparam, 0 },
    { EVENT_OBJECT_PARENTCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOSIZE },
    { WM_CHILDACTIVATE, sent },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOSIZE|SWP_NOREDRAW|SWP_NOCLIENTSIZE },
    { WM_MOVE, sent|defwinproc|wparam, 0 },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { 0 }
};

static const struct message WmSetParentSeq_2[] = {
    { WM_SHOWWINDOW, sent|wparam, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_HIDE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_HIDEWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { HCBT_SETFOCUS, hook|optional },
    { WM_NCACTIVATE, sent|wparam|optional, 0 },
    { WM_ACTIVATE, sent|wparam|optional, 0 },
    { WM_ACTIVATEAPP, sent|wparam|optional, 0 },
    { WM_KILLFOCUS, sent|wparam, 0 },
    { EVENT_OBJECT_PARENTCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_NOSIZE },
    { HCBT_ACTIVATE, hook|optional },
    { EVENT_SYSTEM_FOREGROUND, winevent_hook|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|wparam|optional, SWP_NOSIZE|SWP_NOMOVE },
    { WM_NCACTIVATE, sent|wparam|optional, 1 },
    { WM_ACTIVATE, sent|wparam|optional, 1 },
    { HCBT_SETFOCUS, hook|optional },
    { EVENT_OBJECT_FOCUS, winevent_hook|wparam|lparam, OBJID_CLIENT, 0 },
    { WM_SETFOCUS, sent|optional|defwinproc },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_NOREDRAW|SWP_NOSIZE|SWP_NOCLIENTSIZE },
    { WM_MOVE, sent|defwinproc|wparam, 0 },
    { EVENT_OBJECT_LOCATIONCHANGE, winevent_hook|wparam|lparam, 0, 0 },
    { WM_SHOWWINDOW, sent|wparam, 1 },
    { WM_WINDOWPOSCHANGING, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE },
    { EVENT_OBJECT_SHOW, winevent_hook|wparam|lparam, 0, 0 },
    { WM_WINDOWPOSCHANGED, sent|wparam, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { 0 }
};


static void test_SetParent(void)
{
    HWND parent1, parent2, child, popup;
    RECT rc, rc_old;

    parent1 = CreateWindowExA(0, "TestParentClass", NULL, WS_OVERLAPPEDWINDOW,
                            100, 100, 200, 200, 0, 0, 0, NULL);
    ok(parent1 != 0, "Failed to create parent1 window\n");

    parent2 = CreateWindowExA(0, "TestParentClass", NULL, WS_OVERLAPPEDWINDOW,
                            400, 100, 200, 200, 0, 0, 0, NULL);
    ok(parent2 != 0, "Failed to create parent2 window\n");

    /* WS_CHILD window */
    child = CreateWindowExA(0, "TestWindowClass", NULL, WS_CHILD | WS_VISIBLE,
                           10, 10, 150, 150, parent1, 0, 0, NULL);
    ok(child != 0, "Failed to create child window\n");

    GetWindowRect(parent1, &rc);
    trace("parent1 (%d,%d)-(%d,%d)\n", rc.left, rc.top, rc.right, rc.bottom);
    GetWindowRect(child, &rc_old);
    MapWindowPoints(0, parent1, (POINT *)&rc_old, 2);
    trace("child (%d,%d)-(%d,%d)\n", rc_old.left, rc_old.top, rc_old.right, rc_old.bottom);

    flush_sequence();

    SetParent(child, parent2);
    flush_events();
    ok_sequence(WmSetParentSeq_1, "SetParent() visible WS_CHILD", TRUE);

    ok(GetWindowLongA(child, GWL_STYLE) & WS_VISIBLE, "WS_VISIBLE should be set\n");
    ok(!IsWindowVisible(child), "IsWindowVisible() should return FALSE\n");

    GetWindowRect(parent2, &rc);
    trace("parent2 (%d,%d)-(%d,%d)\n", rc.left, rc.top, rc.right, rc.bottom);
    GetWindowRect(child, &rc);
    MapWindowPoints(0, parent2, (POINT *)&rc, 2);
    trace("child (%d,%d)-(%d,%d)\n", rc.left, rc.top, rc.right, rc.bottom);

    ok(EqualRect(&rc_old, &rc), "rects do not match (%d,%d-%d,%d) / (%d,%d-%d,%d)\n",
       rc_old.left, rc_old.top, rc_old.right, rc_old.bottom,
       rc.left, rc.top, rc.right, rc.bottom );

    /* WS_POPUP window */
    popup = CreateWindowExA(0, "TestWindowClass", NULL, WS_POPUP | WS_VISIBLE,
                           20, 20, 100, 100, 0, 0, 0, NULL);
    ok(popup != 0, "Failed to create popup window\n");

    GetWindowRect(popup, &rc_old);
    trace("popup (%d,%d)-(%d,%d)\n", rc_old.left, rc_old.top, rc_old.right, rc_old.bottom);

    flush_sequence();

    SetParent(popup, child);
    flush_events();
    ok_sequence(WmSetParentSeq_2, "SetParent() visible WS_POPUP", TRUE);

    ok(GetWindowLongA(popup, GWL_STYLE) & WS_VISIBLE, "WS_VISIBLE should be set\n");
    ok(!IsWindowVisible(popup), "IsWindowVisible() should return FALSE\n");

    GetWindowRect(child, &rc);
    trace("parent2 (%d,%d)-(%d,%d)\n", rc.left, rc.top, rc.right, rc.bottom);
    GetWindowRect(popup, &rc);
    MapWindowPoints(0, child, (POINT *)&rc, 2);
    trace("popup (%d,%d)-(%d,%d)\n", rc.left, rc.top, rc.right, rc.bottom);

    ok(EqualRect(&rc_old, &rc), "rects do not match (%d,%d-%d,%d) / (%d,%d-%d,%d)\n",
       rc_old.left, rc_old.top, rc_old.right, rc_old.bottom,
       rc.left, rc.top, rc.right, rc.bottom );

    DestroyWindow(popup);
    DestroyWindow(child);
    DestroyWindow(parent1);
    DestroyWindow(parent2);

    flush_sequence();
}

static const struct message WmKeyReleaseOnly[] = {
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 0x41, 0x80000001 },
    { WM_KEYUP, sent|wparam|lparam, 0x41, 0x80000001 },
    { 0 }
};
static const struct message WmKeyPressNormal[] = {
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 0x41, 0x1 },
    { WM_KEYDOWN, sent|wparam|lparam, 0x41, 0x1 },
    { 0 }
};
static const struct message WmKeyPressRepeat[] = {
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 0x41, 0x40000001 },
    { WM_KEYDOWN, sent|wparam|lparam, 0x41, 0x40000001 },
    { 0 }
};
static const struct message WmKeyReleaseNormal[] = {
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, 0x41, 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, 0x41, 0xc0000001 },
    { 0 }
};

static void test_keyflags(void)
{
    HWND test_window;
    SHORT key_state;
    BYTE keyboard_state[256];
    MSG msg;

    test_window = CreateWindowExA(0, "TestWindowClass", NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                           100, 100, 200, 200, 0, 0, 0, NULL);

    flush_events();
    flush_sequence();

    /* keyup without a keydown */
    keybd_event(0x41, 0, KEYEVENTF_KEYUP, 0);
    while (PeekMessageA(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
        DispatchMessageA(&msg);
    ok_sequence(WmKeyReleaseOnly, "key release only", TRUE);

    key_state = GetAsyncKeyState(0x41);
    ok((key_state & 0x8000) == 0, "unexpected key state %x\n", key_state);

    key_state = GetKeyState(0x41);
    ok((key_state & 0x8000) == 0, "unexpected key state %x\n", key_state);

    /* keydown */
    keybd_event(0x41, 0, 0, 0);
    while (PeekMessageA(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
        DispatchMessageA(&msg);
    ok_sequence(WmKeyPressNormal, "key press only", FALSE);

    key_state = GetAsyncKeyState(0x41);
    ok((key_state & 0x8000) == 0x8000, "unexpected key state %x\n", key_state);

    key_state = GetKeyState(0x41);
    ok((key_state & 0x8000) == 0x8000, "unexpected key state %x\n", key_state);

    /* keydown repeat */
    keybd_event(0x41, 0, 0, 0);
    while (PeekMessageA(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
        DispatchMessageA(&msg);
    ok_sequence(WmKeyPressRepeat, "key press repeat", FALSE);

    key_state = GetAsyncKeyState(0x41);
    ok((key_state & 0x8000) == 0x8000, "unexpected key state %x\n", key_state);

    key_state = GetKeyState(0x41);
    ok((key_state & 0x8000) == 0x8000, "unexpected key state %x\n", key_state);

    /* keyup */
    keybd_event(0x41, 0, KEYEVENTF_KEYUP, 0);
    while (PeekMessageA(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
        DispatchMessageA(&msg);
    ok_sequence(WmKeyReleaseNormal, "key release repeat", FALSE);

    key_state = GetAsyncKeyState(0x41);
    ok((key_state & 0x8000) == 0, "unexpected key state %x\n", key_state);

    key_state = GetKeyState(0x41);
    ok((key_state & 0x8000) == 0, "unexpected key state %x\n", key_state);

    /* set the key state in this thread */
    GetKeyboardState(keyboard_state);
    keyboard_state[0x41] = 0x80;
    SetKeyboardState(keyboard_state);

    key_state = GetAsyncKeyState(0x41);
    ok((key_state & 0x8000) == 0, "unexpected key state %x\n", key_state);

    /* keydown */
    keybd_event(0x41, 0, 0, 0);
    while (PeekMessageA(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
        DispatchMessageA(&msg);
    ok_sequence(WmKeyPressRepeat, "key press after setkeyboardstate", TRUE);

    key_state = GetAsyncKeyState(0x41);
    ok((key_state & 0x8000) == 0x8000, "unexpected key state %x\n", key_state);

    key_state = GetKeyState(0x41);
    ok((key_state & 0x8000) == 0x8000, "unexpected key state %x\n", key_state);

    /* clear the key state in this thread */
    GetKeyboardState(keyboard_state);
    keyboard_state[0x41] = 0;
    SetKeyboardState(keyboard_state);

    key_state = GetAsyncKeyState(0x41);
    ok((key_state & 0x8000) == 0x8000, "unexpected key state %x\n", key_state);

    /* keyup */
    keybd_event(0x41, 0, KEYEVENTF_KEYUP, 0);
    while (PeekMessageA(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
        DispatchMessageA(&msg);
    ok_sequence(WmKeyReleaseOnly, "key release after setkeyboardstate", TRUE);

    key_state = GetAsyncKeyState(0x41);
    ok((key_state & 0x8000) == 0, "unexpected key state %x\n", key_state);

    key_state = GetKeyState(0x41);
    ok((key_state & 0x8000) == 0, "unexpected key state %x\n", key_state);

    DestroyWindow(test_window);
    flush_sequence();
}

static const struct message WmHotkeyPressLWIN[] = {
    { WM_KEYDOWN, kbd_hook|wparam|lparam, VK_LWIN, LLKHF_INJECTED },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_LWIN, 1 },
    { WM_KEYDOWN, sent|wparam|lparam, VK_LWIN, 1 },
    { 0 }
};
static const struct message WmHotkeyPress[] = {
    { WM_KEYDOWN, kbd_hook|lparam, 0, LLKHF_INJECTED },
    { WM_HOTKEY, sent|wparam, 5 },
    { 0 }
};
static const struct message WmHotkeyRelease[] = {
    { WM_KEYUP, kbd_hook|lparam, 0, LLKHF_INJECTED|LLKHF_UP },
    { HCBT_KEYSKIPPED, hook|lparam|optional, 0, 0x80000001 },
    { WM_KEYUP, sent|lparam, 0, 0x80000001 },
    { 0 }
};
static const struct message WmHotkeyReleaseLWIN[] = {
    { WM_KEYUP, kbd_hook|wparam|lparam, VK_LWIN, LLKHF_INJECTED|LLKHF_UP },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_LWIN, 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, VK_LWIN, 0xc0000001 },
    { 0 }
};
static const struct message WmHotkeyCombined[] = {
    { WM_KEYDOWN, kbd_hook|wparam|lparam, VK_LWIN, LLKHF_INJECTED },
    { WM_KEYDOWN, kbd_hook|lparam, 0, LLKHF_INJECTED },
    { WM_KEYUP, kbd_hook|lparam, 0, LLKHF_INJECTED|LLKHF_UP },
    { WM_KEYUP, kbd_hook|wparam|lparam, VK_LWIN, LLKHF_INJECTED|LLKHF_UP },
    { WM_APP, sent, 0, 0 },
    { WM_HOTKEY, sent|wparam, 5 },
    { WM_APP+1, sent, 0, 0 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_LWIN, 1 },
    { WM_KEYDOWN, sent|wparam|lparam, VK_LWIN, 1 },
    { HCBT_KEYSKIPPED, hook|optional, 0, 0x80000001 },
    { WM_KEYUP, sent, 0, 0x80000001 }, /* lparam not checked so the sequence isn't a todo */
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_LWIN, 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, VK_LWIN, 0xc0000001 },
    { 0 }
};
static const struct message WmHotkeyPrevious[] = {
    { WM_KEYDOWN, kbd_hook|wparam|lparam, VK_LWIN, LLKHF_INJECTED },
    { WM_KEYDOWN, kbd_hook|lparam, 0, LLKHF_INJECTED },
    { WM_KEYUP, kbd_hook|lparam, 0, LLKHF_INJECTED|LLKHF_UP },
    { WM_KEYUP, kbd_hook|wparam|lparam, VK_LWIN, LLKHF_INJECTED|LLKHF_UP },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_LWIN, 1 },
    { WM_KEYDOWN, sent|wparam|lparam, VK_LWIN, 1 },
    { HCBT_KEYSKIPPED, hook|lparam|optional, 0, 1 },
    { WM_KEYDOWN, sent|lparam, 0, 1 },
    { HCBT_KEYSKIPPED, hook|optional|lparam, 0, 0xc0000001 },
    { WM_KEYUP, sent|lparam, 0, 0xc0000001 },
    { HCBT_KEYSKIPPED, hook|wparam|lparam|optional, VK_LWIN, 0xc0000001 },
    { WM_KEYUP, sent|wparam|lparam, VK_LWIN, 0xc0000001 },
    { 0 }
};
static const struct message WmHotkeyNew[] = {
    { WM_KEYDOWN, kbd_hook|lparam, 0, LLKHF_INJECTED },
    { WM_KEYUP, kbd_hook|lparam, 0, LLKHF_INJECTED|LLKHF_UP },
    { WM_HOTKEY, sent|wparam, 5 },
    { HCBT_KEYSKIPPED, hook|optional, 0, 0x80000001 },
    { WM_KEYUP, sent, 0, 0x80000001 }, /* lparam not checked so the sequence isn't a todo */
    { 0 }
};

static int hotkey_letter;

static LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    struct recvd_message msg;

    if (nCode == HC_ACTION)
    {
        KBDLLHOOKSTRUCT *kdbhookstruct = (KBDLLHOOKSTRUCT*)lParam;

        msg.hwnd = 0;
        msg.message = wParam;
        msg.flags = kbd_hook|wparam|lparam;
        msg.wParam = kdbhookstruct->vkCode;
        msg.lParam = kdbhookstruct->flags;
        msg.descr = "KeyboardHookProc";
        add_message(&msg);

        if (wParam == WM_KEYUP || wParam == WM_KEYDOWN)
        {
            ok(kdbhookstruct->vkCode == VK_LWIN || kdbhookstruct->vkCode == hotkey_letter,
               "unexpected keycode %x\n", kdbhookstruct->vkCode);
       }
    }

    return CallNextHookEx(hKBD_hook, nCode, wParam, lParam);
}

static void test_hotkey(void)
{
    HWND test_window, taskbar_window;
    BOOL ret;
    MSG msg;
    DWORD queue_status;
    SHORT key_state;

    SetLastError(0xdeadbeef);
    ret = UnregisterHotKey(NULL, 0);
    ok(ret == FALSE, "expected FALSE, got %i\n", ret);
    ok(GetLastError() == ERROR_HOTKEY_NOT_REGISTERED || broken(GetLastError() == 0xdeadbeef),
       "unexpected error %d\n", GetLastError());

    if (ret == TRUE)
    {
        skip("hotkeys not supported\n");
        return;
    }

    test_window = CreateWindowExA(0, "HotkeyWindowClass", NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                           100, 100, 200, 200, 0, 0, 0, NULL);

    flush_sequence();

    SetLastError(0xdeadbeef);
    ret = UnregisterHotKey(test_window, 0);
    ok(ret == FALSE, "expected FALSE, got %i\n", ret);
    ok(GetLastError() == ERROR_HOTKEY_NOT_REGISTERED || broken(GetLastError() == 0xdeadbeef),
       "unexpected error %d\n", GetLastError());

    /* Search for a Windows Key + letter combination that hasn't been registered */
    for (hotkey_letter = 0x41; hotkey_letter <= 0x51; hotkey_letter ++)
    {
        SetLastError(0xdeadbeef);
        ret = RegisterHotKey(test_window, 5, MOD_WIN, hotkey_letter);

        if (ret == TRUE)
        {
            break;
        }
        else
        {
            ok(GetLastError() == ERROR_HOTKEY_ALREADY_REGISTERED || broken(GetLastError() == 0xdeadbeef),
               "unexpected error %d\n", GetLastError());
        }
    }

    if (hotkey_letter == 0x52)
    {
        ok(0, "Couldn't find any free Windows Key + letter combination\n");
        goto end;
    }

    hKBD_hook = SetWindowsHookExA(WH_KEYBOARD_LL, KeyboardHookProc, GetModuleHandleA(NULL), 0);
    if (!hKBD_hook) win_skip("WH_KEYBOARD_LL is not supported\n");

    /* Same key combination, different id */
    SetLastError(0xdeadbeef);
    ret = RegisterHotKey(test_window, 4, MOD_WIN, hotkey_letter);
    ok(ret == FALSE, "expected FALSE, got %i\n", ret);
    ok(GetLastError() == ERROR_HOTKEY_ALREADY_REGISTERED || broken(GetLastError() == 0xdeadbeef),
       "unexpected error %d\n", GetLastError());

    /* Same key combination, different window */
    SetLastError(0xdeadbeef);
    ret = RegisterHotKey(NULL, 5, MOD_WIN, hotkey_letter);
    ok(ret == FALSE, "expected FALSE, got %i\n", ret);
    ok(GetLastError() == ERROR_HOTKEY_ALREADY_REGISTERED || broken(GetLastError() == 0xdeadbeef),
       "unexpected error %d\n", GetLastError());

    /* Register the same hotkey twice */
    SetLastError(0xdeadbeef);
    ret = RegisterHotKey(test_window, 5, MOD_WIN, hotkey_letter);
    ok(ret == FALSE, "expected FALSE, got %i\n", ret);
    ok(GetLastError() == ERROR_HOTKEY_ALREADY_REGISTERED || broken(GetLastError() == 0xdeadbeef),
       "unexpected error %d\n", GetLastError());

    /* Window on another thread */
    taskbar_window = FindWindowA("Shell_TrayWnd", NULL);
    if (!taskbar_window)
    {
        skip("no taskbar?\n");
    }
    else
    {
        SetLastError(0xdeadbeef);
        ret = RegisterHotKey(taskbar_window, 5, 0, hotkey_letter);
        ok(ret == FALSE, "expected FALSE, got %i\n", ret);
        ok(GetLastError() == ERROR_WINDOW_OF_OTHER_THREAD || broken(GetLastError() == 0xdeadbeef),
           "unexpected error %d\n", GetLastError());
    }

    /* Inject the appropriate key sequence */
    keybd_event(VK_LWIN, 0, 0, 0);
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        DispatchMessageA(&msg);
    ok_sequence(WmHotkeyPressLWIN, "window hotkey press LWIN", FALSE);

    keybd_event(hotkey_letter, 0, 0, 0);
    queue_status = GetQueueStatus(QS_HOTKEY);
    ok((queue_status & (QS_HOTKEY << 16)) == QS_HOTKEY << 16, "expected QS_HOTKEY << 16 set, got %x\n", queue_status);
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_HOTKEY)
        {
            ok(msg.hwnd == test_window, "unexpected hwnd %p\n", msg.hwnd);
            ok(msg.lParam == MAKELPARAM(MOD_WIN, hotkey_letter), "unexpected WM_HOTKEY lparam %lx\n", msg.lParam);
        }
        DispatchMessageA(&msg);
    }
    ok_sequence(WmHotkeyPress, "window hotkey press", FALSE);

    queue_status = GetQueueStatus(QS_HOTKEY);
    ok((queue_status & (QS_HOTKEY << 16)) == 0, "expected QS_HOTKEY << 16 cleared, got %x\n", queue_status);

    key_state = GetAsyncKeyState(hotkey_letter);
    ok((key_state & 0x8000) == 0x8000, "unexpected key state %x\n", key_state);

    keybd_event(hotkey_letter, 0, KEYEVENTF_KEYUP, 0);
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        DispatchMessageA(&msg);
    ok_sequence(WmHotkeyRelease, "window hotkey release", TRUE);

    keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        DispatchMessageA(&msg);
    ok_sequence(WmHotkeyReleaseLWIN, "window hotkey release LWIN", FALSE);

    /* normal posted WM_HOTKEY messages set QS_HOTKEY */
    PostMessageA(test_window, WM_HOTKEY, 0, 0);
    queue_status = GetQueueStatus(QS_HOTKEY);
    ok((queue_status & (QS_HOTKEY << 16)) == QS_HOTKEY << 16, "expected QS_HOTKEY << 16 set, got %x\n", queue_status);
    queue_status = GetQueueStatus(QS_POSTMESSAGE);
    ok((queue_status & (QS_POSTMESSAGE << 16)) == QS_POSTMESSAGE << 16, "expected QS_POSTMESSAGE << 16 set, got %x\n", queue_status);
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        DispatchMessageA(&msg);
    flush_sequence();

    /* Send and process all messages at once */
    PostMessageA(test_window, WM_APP, 0, 0);
    keybd_event(VK_LWIN, 0, 0, 0);
    keybd_event(hotkey_letter, 0, 0, 0);
    keybd_event(hotkey_letter, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);

    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_HOTKEY)
        {
            ok(msg.hwnd == test_window, "unexpected hwnd %p\n", msg.hwnd);
            ok(msg.lParam == MAKELPARAM(MOD_WIN, hotkey_letter), "unexpected WM_HOTKEY lparam %lx\n", msg.lParam);
        }
        DispatchMessageA(&msg);
    }
    ok_sequence(WmHotkeyCombined, "window hotkey combined", FALSE);

    /* Register same hwnd/id with different key combination */
    ret = RegisterHotKey(test_window, 5, 0, hotkey_letter);
    ok(ret == TRUE, "expected TRUE, got %i, err=%d\n", ret, GetLastError());

    /* Previous key combination does not work */
    keybd_event(VK_LWIN, 0, 0, 0);
    keybd_event(hotkey_letter, 0, 0, 0);
    keybd_event(hotkey_letter, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);

    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        DispatchMessageA(&msg);
    ok_sequence(WmHotkeyPrevious, "window hotkey previous", FALSE);

    /* New key combination works */
    keybd_event(hotkey_letter, 0, 0, 0);
    keybd_event(hotkey_letter, 0, KEYEVENTF_KEYUP, 0);

    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_HOTKEY)
        {
            ok(msg.hwnd == test_window, "unexpected hwnd %p\n", msg.hwnd);
            ok(msg.lParam == MAKELPARAM(0, hotkey_letter), "unexpected WM_HOTKEY lparam %lx\n", msg.lParam);
        }
        DispatchMessageA(&msg);
    }
    ok_sequence(WmHotkeyNew, "window hotkey new", FALSE);

    /* Unregister hotkey properly */
    ret = UnregisterHotKey(test_window, 5);
    ok(ret == TRUE, "expected TRUE, got %i, err=%d\n", ret, GetLastError());

    /* Unregister hotkey again */
    SetLastError(0xdeadbeef);
    ret = UnregisterHotKey(test_window, 5);
    ok(ret == FALSE, "expected FALSE, got %i\n", ret);
    ok(GetLastError() == ERROR_HOTKEY_NOT_REGISTERED || broken(GetLastError() == 0xdeadbeef),
       "unexpected error %d\n", GetLastError());

    /* Register thread hotkey */
    ret = RegisterHotKey(NULL, 5, MOD_WIN, hotkey_letter);
    ok(ret == TRUE, "expected TRUE, got %i, err=%d\n", ret, GetLastError());

    /* Inject the appropriate key sequence */
    keybd_event(VK_LWIN, 0, 0, 0);
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
    {
        ok(msg.hwnd != NULL, "unexpected thread message %x\n", msg.message);
        DispatchMessageA(&msg);
    }
    ok_sequence(WmHotkeyPressLWIN, "thread hotkey press LWIN", FALSE);

    keybd_event(hotkey_letter, 0, 0, 0);
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_HOTKEY)
        {
            struct recvd_message message;
            ok(msg.hwnd == NULL, "unexpected hwnd %p\n", msg.hwnd);
            ok(msg.lParam == MAKELPARAM(MOD_WIN, hotkey_letter), "unexpected WM_HOTKEY lparam %lx\n", msg.lParam);
            message.message = msg.message;
            message.flags = sent|wparam|lparam;
            message.wParam = msg.wParam;
            message.lParam = msg.lParam;
            message.descr = "test_hotkey thread message";
            add_message(&message);
        }
        else
            ok(msg.hwnd != NULL, "unexpected thread message %x\n", msg.message);
        DispatchMessageA(&msg);
    }
    ok_sequence(WmHotkeyPress, "thread hotkey press", FALSE);

    keybd_event(hotkey_letter, 0, KEYEVENTF_KEYUP, 0);
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
    {
        ok(msg.hwnd != NULL, "unexpected thread message %x\n", msg.message);
        DispatchMessageA(&msg);
    }
    ok_sequence(WmHotkeyRelease, "thread hotkey release", TRUE);

    keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
    {
        ok(msg.hwnd != NULL, "unexpected thread message %x\n", msg.message);
        DispatchMessageA(&msg);
    }
    ok_sequence(WmHotkeyReleaseLWIN, "thread hotkey release LWIN", FALSE);

    /* Unregister thread hotkey */
    ret = UnregisterHotKey(NULL, 5);
    ok(ret == TRUE, "expected TRUE, got %i, err=%d\n", ret, GetLastError());

    if (hKBD_hook) UnhookWindowsHookEx(hKBD_hook);
    hKBD_hook = NULL;

end:
    UnregisterHotKey(NULL, 5);
    UnregisterHotKey(test_window, 5);
    DestroyWindow(test_window);
    flush_sequence();
}


static const struct message WmSetFocus_1[] = {
    { HCBT_SETFOCUS, hook }, /* child */
    { HCBT_ACTIVATE, hook }, /* parent */
    { WM_QUERYNEWPALETTE, sent|wparam|lparam|parent|optional, 0, 0 },
    { WM_WINDOWPOSCHANGING, sent|parent, 0, SWP_NOSIZE|SWP_NOMOVE },
    { WM_ACTIVATEAPP, sent|wparam|parent, 1 },
    { WM_NCACTIVATE, sent|parent },
    { WM_GETTEXT, sent|defwinproc|parent|optional },
    { WM_GETTEXT, sent|defwinproc|parent|optional },
    { WM_ACTIVATE, sent|wparam|parent, 1 },
    { HCBT_SETFOCUS, hook }, /* parent */
    { WM_SETFOCUS, sent|defwinproc|parent },
    { WM_KILLFOCUS, sent|parent },
    { WM_SETFOCUS, sent },
    { 0 }
};
static const struct message WmSetFocus_2[] = {
    { HCBT_SETFOCUS, hook }, /* parent */
    { WM_KILLFOCUS, sent },
    { WM_SETFOCUS, sent|parent },
    { 0 }
};
static const struct message WmSetFocus_3[] = {
    { HCBT_SETFOCUS, hook }, /* child */
    { 0 }
};
static const struct message WmSetFocus_4[] = {
    { 0 }
};

static void test_SetFocus(void)
{
    HWND parent, old_parent, child, old_focus, old_active;
    MSG msg;
    struct wnd_event wnd_event;
    HANDLE hthread;
    DWORD ret, tid;

    wnd_event.start_event = CreateEventW(NULL, 0, 0, NULL);
    ok(wnd_event.start_event != 0, "CreateEvent error %d\n", GetLastError());
    hthread = CreateThread(NULL, 0, thread_proc, &wnd_event, 0, &tid);
    ok(hthread != 0, "CreateThread error %d\n", GetLastError());
    ret = WaitForSingleObject(wnd_event.start_event, INFINITE);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(wnd_event.start_event);

    parent = CreateWindowExA(0, "TestParentClass", NULL, WS_OVERLAPPEDWINDOW,
                            0, 0, 0, 0, 0, 0, 0, NULL);
    ok(parent != 0, "failed to create parent window\n");
    child = CreateWindowExA(0, "TestWindowClass", NULL, WS_CHILD,
                           0, 0, 0, 0, parent, 0, 0, NULL);
    ok(child != 0, "failed to create child window\n");

    trace("parent %p, child %p, thread window %p\n", parent, child, wnd_event.hwnd);

    SetFocus(0);
    SetActiveWindow(0);

    flush_events();
    flush_sequence();

    ok(GetActiveWindow() == 0, "expected active 0, got %p\n", GetActiveWindow());
    ok(GetFocus() == 0, "expected focus 0, got %p\n", GetFocus());

    log_all_parent_messages++;

    old_focus = SetFocus(child);
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    ok_sequence(WmSetFocus_1, "SetFocus on a child window", TRUE);
    ok(old_focus == parent, "expected old focus %p, got %p\n", parent, old_focus);
    ok(GetActiveWindow() == parent, "expected active %p, got %p\n", parent, GetActiveWindow());
    ok(GetFocus() == child, "expected focus %p, got %p\n", child, GetFocus());

    old_focus = SetFocus(parent);
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    ok_sequence(WmSetFocus_2, "SetFocus on a parent window", FALSE);
    ok(old_focus == child, "expected old focus %p, got %p\n", child, old_focus);
    ok(GetActiveWindow() == parent, "expected active %p, got %p\n", parent, GetActiveWindow());
    ok(GetFocus() == parent, "expected focus %p, got %p\n", parent, GetFocus());

    SetLastError(0xdeadbeef);
    old_focus = SetFocus((HWND)0xdeadbeef);
    ok(GetLastError() == ERROR_INVALID_WINDOW_HANDLE || broken(GetLastError() == 0xdeadbeef),
       "expected ERROR_INVALID_WINDOW_HANDLE, got %d\n", GetLastError());
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    ok_sequence(WmEmptySeq, "SetFocus on an invalid window", FALSE);
    ok(old_focus == 0, "expected old focus 0, got %p\n", old_focus);
    ok(GetActiveWindow() == parent, "expected active %p, got %p\n", parent, GetActiveWindow());
    ok(GetFocus() == parent, "expected focus %p, got %p\n", parent, GetFocus());

    SetLastError(0xdeadbeef);
    old_focus = SetFocus(GetDesktopWindow());
    ok(GetLastError() == ERROR_ACCESS_DENIED /* Vista+ */ ||
       broken(GetLastError() == 0xdeadbeef), "expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    ok_sequence(WmEmptySeq, "SetFocus on a desktop window", TRUE);
    ok(old_focus == 0, "expected old focus 0, got %p\n", old_focus);
    ok(GetActiveWindow() == parent, "expected active %p, got %p\n", parent, GetActiveWindow());
    ok(GetFocus() == parent, "expected focus %p, got %p\n", parent, GetFocus());

    SetLastError(0xdeadbeef);
    old_focus = SetFocus(wnd_event.hwnd);
    ok(GetLastError() == ERROR_ACCESS_DENIED /* Vista+ */ ||
       broken(GetLastError() == 0xdeadbeef), "expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    ok_sequence(WmEmptySeq, "SetFocus on another thread window", TRUE);
    ok(old_focus == 0, "expected old focus 0, got %p\n", old_focus);
    ok(GetActiveWindow() == parent, "expected active %p, got %p\n", parent, GetActiveWindow());
    ok(GetFocus() == parent, "expected focus %p, got %p\n", parent, GetFocus());

    SetLastError(0xdeadbeef);
    old_active = SetActiveWindow((HWND)0xdeadbeef);
    ok(GetLastError() == ERROR_INVALID_WINDOW_HANDLE || broken(GetLastError() == 0xdeadbeef),
       "expected ERROR_INVALID_WINDOW_HANDLE, got %d\n", GetLastError());
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    ok_sequence(WmEmptySeq, "SetActiveWindow on an invalid window", FALSE);
    ok(old_active == 0, "expected old focus 0, got %p\n", old_active);
    ok(GetActiveWindow() == parent, "expected active %p, got %p\n", parent, GetActiveWindow());
    ok(GetFocus() == parent, "expected focus %p, got %p\n", parent, GetFocus());

    SetLastError(0xdeadbeef);
    old_active = SetActiveWindow(GetDesktopWindow());
todo_wine
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %d\n", GetLastError());
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    ok_sequence(WmEmptySeq, "SetActiveWindow on a desktop window", TRUE);
    ok(old_active == 0, "expected old focus 0, got %p\n", old_focus);
    ok(GetActiveWindow() == parent, "expected active %p, got %p\n", parent, GetActiveWindow());
    ok(GetFocus() == parent, "expected focus %p, got %p\n", parent, GetFocus());

    SetLastError(0xdeadbeef);
    old_active = SetActiveWindow(wnd_event.hwnd);
todo_wine
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %d\n", GetLastError());
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    ok_sequence(WmEmptySeq, "SetActiveWindow on another thread window", TRUE);
    ok(old_active == 0, "expected old focus 0, got %p\n", old_active);
    ok(GetActiveWindow() == parent, "expected active %p, got %p\n", parent, GetActiveWindow());
    ok(GetFocus() == parent, "expected focus %p, got %p\n", parent, GetFocus());

    SetLastError(0xdeadbeef);
    ret = AttachThreadInput(GetCurrentThreadId(), tid, TRUE);
    ok(ret, "AttachThreadInput error %d\n", GetLastError());

todo_wine {
    ok(GetActiveWindow() == parent, "expected active %p, got %p\n", parent, GetActiveWindow());
    ok(GetFocus() == parent, "expected focus %p, got %p\n", parent, GetFocus());
}
    flush_events();
    flush_sequence();

    old_focus = SetFocus(wnd_event.hwnd);
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    ok(old_focus == wnd_event.hwnd, "expected old focus %p, got %p\n", wnd_event.hwnd, old_focus);
    ok(GetActiveWindow() == wnd_event.hwnd, "expected active %p, got %p\n", wnd_event.hwnd, GetActiveWindow());
    ok(GetFocus() == wnd_event.hwnd, "expected focus %p, got %p\n", wnd_event.hwnd, GetFocus());

    old_focus = SetFocus(parent);
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    ok(old_focus == parent, "expected old focus %p, got %p\n", parent, old_focus);
    ok(GetActiveWindow() == parent, "expected active %p, got %p\n", parent, GetActiveWindow());
    ok(GetFocus() == parent, "expected focus %p, got %p\n", parent, GetFocus());

    flush_events();
    flush_sequence();

    old_active = SetActiveWindow(wnd_event.hwnd);
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    ok(old_active == parent, "expected old focus %p, got %p\n", parent, old_active);
    ok(GetActiveWindow() == wnd_event.hwnd, "expected active %p, got %p\n", wnd_event.hwnd, GetActiveWindow());
    ok(GetFocus() == wnd_event.hwnd, "expected focus %p, got %p\n", wnd_event.hwnd, GetFocus());

    SetLastError(0xdeadbeef);
    ret = AttachThreadInput(GetCurrentThreadId(), tid, FALSE);
    ok(ret, "AttachThreadInput error %d\n", GetLastError());

    ok(GetActiveWindow() == 0, "expected active 0, got %p\n", GetActiveWindow());
    ok(GetFocus() == 0, "expected focus 0, got %p\n", GetFocus());

    old_parent = SetParent(child, GetDesktopWindow());
    ok(old_parent == parent, "expected old parent %p, got %p\n", parent, old_parent);

    ok(GetActiveWindow() == 0, "expected active 0, got %p\n", GetActiveWindow());
    ok(GetFocus() == 0, "expected focus 0, got %p\n", GetFocus());

    old_focus = SetFocus(parent);
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    ok(old_focus == parent, "expected old focus %p, got %p\n", parent, old_focus);
    ok(GetActiveWindow() == parent, "expected active %p, got %p\n", parent, GetActiveWindow());
    ok(GetFocus() == parent, "expected focus %p, got %p\n", parent, GetFocus());

    flush_events();
    flush_sequence();

    SetLastError(0xdeadbeef);
    old_focus = SetFocus(child);
todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER /* Vista+ */ ||
       broken(GetLastError() == 0) /* XP */ ||
       broken(GetLastError() == 0xdeadbeef), "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    ok_sequence(WmSetFocus_3, "SetFocus on a child window", TRUE);
    ok(old_focus == 0, "expected old focus 0, got %p\n", old_focus);
    ok(GetActiveWindow() == parent, "expected active %p, got %p\n", parent, GetActiveWindow());
    ok(GetFocus() == parent, "expected focus %p, got %p\n", parent, GetFocus());

    SetLastError(0xdeadbeef);
    old_active = SetActiveWindow(child);
    ok(GetLastError() == 0xdeadbeef, "expected 0xdeadbeef, got %d\n", GetLastError());
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
    ok_sequence(WmEmptySeq, "SetActiveWindow on a child window", FALSE);
    ok(old_active == parent, "expected old active %p, got %p\n", parent, old_active);
    ok(GetActiveWindow() == parent, "expected active %p, got %p\n", parent, GetActiveWindow());
    ok(GetFocus() == parent, "expected focus %p, got %p\n", parent, GetFocus());

    log_all_parent_messages--;

    DestroyWindow(child);
    DestroyWindow(parent);

    ret = PostMessageA(wnd_event.hwnd, WM_QUIT, 0, 0);
    ok(ret, "PostMessage(WM_QUIT) error %d\n", GetLastError());
    ret = WaitForSingleObject(hthread, INFINITE);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(hthread);
}

static const struct message WmSetLayeredStyle[] = {
    { WM_STYLECHANGING, sent },
    { WM_STYLECHANGED, sent },
    { WM_GETTEXT, sent|defwinproc|optional },
    { 0 }
};

static const struct message WmSetLayeredStyle2[] = {
    { WM_STYLECHANGING, sent },
    { WM_STYLECHANGED, sent },
    { WM_WINDOWPOSCHANGING, sent|optional|wparam|defwinproc, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE },
    { WM_NCCALCSIZE, sent|optional|wparam|defwinproc, 1 },
    { WM_WINDOWPOSCHANGED, sent|optional|wparam|defwinproc, SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE },
    { WM_MOVE, sent|optional|defwinproc|wparam, 0 },
    { WM_SIZE, sent|optional|defwinproc|wparam, SIZE_RESTORED },
    { 0 }
};

struct layered_window_info
{
    HWND   hwnd;
    HDC    hdc;
    SIZE   size;
    HANDLE event;
    BOOL   ret;
};

static DWORD CALLBACK update_layered_proc( void *param )
{
    struct layered_window_info *info = param;
    POINT src = { 0, 0 };

    info->ret = pUpdateLayeredWindow( info->hwnd, 0, NULL, &info->size,
                                      info->hdc, &src, 0, NULL, ULW_OPAQUE );
    ok( info->ret, "failed\n");
    SetEvent( info->event );
    return 0;
}

static void test_layered_window(void)
{
    HWND hwnd;
    HDC hdc;
    HBITMAP bmp;
    BOOL ret;
    SIZE size;
    POINT pos, src;
    RECT rect, client;
    HANDLE thread;
    DWORD tid;
    struct layered_window_info info;

    if (!pUpdateLayeredWindow)
    {
        win_skip( "UpdateLayeredWindow not supported\n" );
        return;
    }

    hdc = CreateCompatibleDC( 0 );
    bmp = CreateCompatibleBitmap( hdc, 300, 300 );
    SelectObject( hdc, bmp );

    hwnd = CreateWindowExA(0, "TestWindowClass", NULL, WS_CAPTION | WS_THICKFRAME | WS_SYSMENU,
                           100, 100, 300, 300, 0, 0, 0, NULL);
    ok( hwnd != 0, "failed to create window\n" );
    ShowWindow( hwnd, SW_SHOWNORMAL );
    UpdateWindow( hwnd );
    flush_events();
    flush_sequence();

    GetWindowRect( hwnd, &rect );
    GetClientRect( hwnd, &client );
    ok( client.right < rect.right - rect.left, "wrong client area\n" );
    ok( client.bottom < rect.bottom - rect.top, "wrong client area\n" );

    src.x = src.y = 0;
    pos.x = pos.y = 300;
    size.cx = size.cy = 250;
    ret = pUpdateLayeredWindow( hwnd, 0, &pos, &size, hdc, &src, 0, NULL, ULW_OPAQUE );
    ok( !ret, "UpdateLayeredWindow should fail on non-layered window\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    SetWindowLongA( hwnd, GWL_EXSTYLE, GetWindowLongA(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED );
    ok_sequence( WmSetLayeredStyle, "WmSetLayeredStyle", FALSE );

    ret = pUpdateLayeredWindow( hwnd, 0, &pos, &size, hdc, &src, 0, NULL, ULW_OPAQUE );
    ok( ret, "UpdateLayeredWindow failed err %u\n", GetLastError() );
    ok_sequence( WmEmptySeq, "UpdateLayeredWindow", FALSE );
    GetWindowRect( hwnd, &rect );
    ok( rect.left == 300 && rect.top == 300 && rect.right == 550 && rect.bottom == 550,
        "wrong window rect %d,%d,%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    GetClientRect( hwnd, &rect );
    ok( rect.right == client.right - 50 && rect.bottom == client.bottom - 50,
        "wrong client rect %d,%d,%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );

    size.cx = 150;
    pos.y = 200;
    ret = pUpdateLayeredWindow( hwnd, 0, &pos, &size, hdc, &src, 0, NULL, ULW_OPAQUE );
    ok( ret, "UpdateLayeredWindow failed err %u\n", GetLastError() );
    ok_sequence( WmEmptySeq, "UpdateLayeredWindow", FALSE );
    GetWindowRect( hwnd, &rect );
    ok( rect.left == 300 && rect.top == 200 && rect.right == 450 && rect.bottom == 450,
        "wrong window rect %d,%d,%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    GetClientRect( hwnd, &rect );
    ok( rect.right == client.right - 150 && rect.bottom == client.bottom - 50,
        "wrong client rect %d,%d,%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );

    SetWindowLongA( hwnd, GWL_STYLE,
                   GetWindowLongA(hwnd, GWL_STYLE) & ~(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU) );
    ok_sequence( WmSetLayeredStyle2, "WmSetLayeredStyle2", FALSE );

    size.cx = 200;
    pos.x = 200;
    ret = pUpdateLayeredWindow( hwnd, 0, &pos, &size, hdc, &src, 0, NULL, ULW_OPAQUE );
    ok( ret, "UpdateLayeredWindow failed err %u\n", GetLastError() );
    ok_sequence( WmEmptySeq, "UpdateLayeredWindow", FALSE );
    GetWindowRect( hwnd, &rect );
    ok( rect.left == 200 && rect.top == 200 && rect.right == 400 && rect.bottom == 450,
        "wrong window rect %d,%d,%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    GetClientRect( hwnd, &rect );
    ok( (rect.right == 200 && rect.bottom == 250) ||
        broken(rect.right == client.right - 100 && rect.bottom == client.bottom - 50),
        "wrong client rect %d,%d,%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );

    size.cx = 0;
    ret = pUpdateLayeredWindow( hwnd, 0, &pos, &size, hdc, &src, 0, NULL, ULW_OPAQUE );
    ok( !ret, "UpdateLayeredWindow should fail on non-layered window\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER || broken(ERROR_MR_MID_NOT_FOUND) /* win7 */,
        "wrong error %u\n", GetLastError() );
    size.cx = 1;
    size.cy = -1;
    ret = pUpdateLayeredWindow( hwnd, 0, &pos, &size, hdc, &src, 0, NULL, ULW_OPAQUE );
    ok( !ret, "UpdateLayeredWindow should fail on non-layered window\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

    SetWindowLongA( hwnd, GWL_EXSTYLE, GetWindowLongA(hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED );
    ok_sequence( WmSetLayeredStyle, "WmSetLayeredStyle", FALSE );
    GetWindowRect( hwnd, &rect );
    ok( rect.left == 200 && rect.top == 200 && rect.right == 400 && rect.bottom == 450,
        "wrong window rect %d,%d,%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    GetClientRect( hwnd, &rect );
    ok( (rect.right == 200 && rect.bottom == 250) ||
        broken(rect.right == client.right - 100 && rect.bottom == client.bottom - 50),
        "wrong client rect %d,%d,%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );

    SetWindowLongA( hwnd, GWL_EXSTYLE, GetWindowLongA(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED );
    info.hwnd = hwnd;
    info.hdc = hdc;
    info.size.cx = 250;
    info.size.cy = 300;
    info.event = CreateEventA( NULL, TRUE, FALSE, NULL );
    info.ret = FALSE;
    thread = CreateThread( NULL, 0, update_layered_proc, &info, 0, &tid );
    ok( WaitForSingleObject( info.event, 1000 ) == 0, "wait failed\n" );
    ok( info.ret, "UpdateLayeredWindow failed in other thread\n" );
    WaitForSingleObject( thread, 1000 );
    CloseHandle( thread );
    GetWindowRect( hwnd, &rect );
    ok( rect.left == 200 && rect.top == 200 && rect.right == 450 && rect.bottom == 500,
        "wrong window rect %d,%d,%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );
    GetClientRect( hwnd, &rect );
    ok( (rect.right == 250 && rect.bottom == 300) ||
        broken(rect.right == client.right - 50 && rect.bottom == client.bottom),
        "wrong client rect %d,%d,%d,%d\n", rect.left, rect.top, rect.right, rect.bottom );

    DestroyWindow( hwnd );
    DeleteDC( hdc );
    DeleteObject( bmp );
}

static HMENU hpopupmenu;

static LRESULT WINAPI cancel_popup_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ignore_message( message )) return 0;

    switch (message) {
    case WM_ENTERIDLE:
        todo_wine ok(GetCapture() == hwnd, "expected %p, got %p\n", hwnd, GetCapture());
        EndMenu();
        break;
    case WM_INITMENU:
    case WM_INITMENUPOPUP:
    case WM_UNINITMENUPOPUP:
        ok((HMENU)wParam == hpopupmenu, "expected %p, got %lx\n", hpopupmenu, wParam);
        break;
    case WM_CAPTURECHANGED:
        todo_wine ok(!lParam || (HWND)lParam == hwnd, "lost capture to %lx\n", lParam);
        break;
    }

    return MsgCheckProc (FALSE, hwnd, message, wParam, lParam);
}

static LRESULT WINAPI cancel_init_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ignore_message( message )) return 0;

    switch (message) {
    case WM_ENTERMENULOOP:
        ok(EndMenu() == TRUE, "EndMenu() failed\n");
        break;
    }

    return MsgCheckProc (FALSE, hwnd, message, wParam, lParam);
}

static void test_TrackPopupMenu(void)
{
    HWND hwnd;
    BOOL ret;

    hwnd = CreateWindowExA(0, "TestWindowClass", NULL, 0,
                           0, 0, 1, 1, 0,
                           NULL, NULL, 0);
    ok(hwnd != NULL, "CreateWindowEx failed with error %d\n", GetLastError());

    SetWindowLongPtrA( hwnd, GWLP_WNDPROC, (LONG_PTR)cancel_popup_proc);

    hpopupmenu = CreatePopupMenu();
    ok(hpopupmenu != NULL, "CreateMenu failed with error %d\n", GetLastError());

    AppendMenuA(hpopupmenu, MF_STRING, 100, "item 1");
    AppendMenuA(hpopupmenu, MF_STRING, 100, "item 2");

    flush_events();
    flush_sequence();
    ret = TrackPopupMenu(hpopupmenu, 0, 100,100, 0, hwnd, NULL);
    ok_sequence(WmTrackPopupMenu, "TrackPopupMenu", TRUE);
    ok(ret == 1, "TrackPopupMenu failed with error %i\n", GetLastError());

    SetWindowLongPtrA( hwnd, GWLP_WNDPROC, (LONG_PTR)cancel_init_proc);

    flush_events();
    flush_sequence();
    ret = TrackPopupMenu(hpopupmenu, 0, 100,100, 0, hwnd, NULL);
    ok_sequence(WmTrackPopupMenuAbort, "WmTrackPopupMenuAbort", TRUE);
    ok(ret == TRUE, "TrackPopupMenu failed\n");

    SetWindowLongPtrA( hwnd, GWLP_WNDPROC, (LONG_PTR)cancel_popup_proc);

    SetCapture(hwnd);

    flush_events();
    flush_sequence();
    ret = TrackPopupMenu(hpopupmenu, 0, 100,100, 0, hwnd, NULL);
    ok_sequence(WmTrackPopupMenuCapture, "TrackPopupMenuCapture", TRUE);
    ok(ret == 1, "TrackPopupMenuCapture failed with error %i\n", GetLastError());

    DestroyMenu(hpopupmenu);
    DestroyWindow(hwnd);
}

static void test_TrackPopupMenuEmpty(void)
{
    HWND hwnd;
    BOOL ret;

    hwnd = CreateWindowExA(0, "TestWindowClass", NULL, 0,
                           0, 0, 1, 1, 0,
                           NULL, NULL, 0);
    ok(hwnd != NULL, "CreateWindowEx failed with error %d\n", GetLastError());

    SetWindowLongPtrA( hwnd, GWLP_WNDPROC, (LONG_PTR)cancel_popup_proc);

    hpopupmenu = CreatePopupMenu();
    ok(hpopupmenu != NULL, "CreateMenu failed with error %d\n", GetLastError());

    flush_events();
    flush_sequence();
    ret = TrackPopupMenu(hpopupmenu, 0, 100,100, 0, hwnd, NULL);
    ok_sequence(WmTrackPopupMenuEmpty, "TrackPopupMenuEmpty", TRUE);
    todo_wine ok(ret == 0, "TrackPopupMenu succeeded\n");

    DestroyMenu(hpopupmenu);
    DestroyWindow(hwnd);
}

static void init_funcs(void)
{
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");

#define X(f) p##f = (void*)GetProcAddress(hKernel32, #f)
    X(ActivateActCtx);
    X(CreateActCtxW);
    X(DeactivateActCtx);
    X(GetCurrentActCtx);
    X(ReleaseActCtx);
#undef X
}

#if 0
START_TEST(msg)
{
    char **test_argv;
    BOOL ret;
    BOOL (WINAPI *pIsWinEventHookInstalled)(DWORD)= 0;/*GetProcAddress(user32, "IsWinEventHookInstalled");*/
    HMODULE hModuleImm32;
    BOOL (WINAPI *pImmDisableIME)(DWORD);
    int argc;

    init_funcs();

    argc = winetest_get_mainargs( &test_argv );
    if (argc >= 3)
    {
        unsigned int arg;
        /* Child process. */
        sscanf (test_argv[2], "%d", (unsigned int *) &arg);
        do_wait_idle_child( arg );
        return;
    }

    InitializeCriticalSection( &sequence_cs );
    init_procs();

    hModuleImm32 = LoadLibraryA("imm32.dll");
    if (hModuleImm32) {
        pImmDisableIME = (void *)GetProcAddress(hModuleImm32, "ImmDisableIME");
        if (pImmDisableIME)
            pImmDisableIME(0);
    }
    pImmDisableIME = NULL;
    FreeLibrary(hModuleImm32);

    if (!RegisterWindowClasses()) assert(0);

    if (pSetWinEventHook)
    {
        hEvent_hook = pSetWinEventHook(EVENT_MIN, EVENT_MAX,
                                       GetModuleHandleA(0), win_event_proc,
                                       0, GetCurrentThreadId(),
                                       WINEVENT_INCONTEXT);
        if (pIsWinEventHookInstalled && hEvent_hook)
	{
	    UINT event;
	    for (event = EVENT_MIN; event <= EVENT_MAX; event++)
		ok(pIsWinEventHookInstalled(event), "IsWinEventHookInstalled(%u) failed\n", event);
	}
    }
    if (!hEvent_hook) win_skip( "no win event hook support\n" );

    cbt_hook_thread_id = GetCurrentThreadId();
    hCBT_hook = SetWindowsHookExA(WH_CBT, cbt_hook_proc, 0, GetCurrentThreadId());
    if (!hCBT_hook) win_skip( "cannot set global hook, will skip hook tests\n" );

    test_winevents();

    /* Fix message sequences before removing 4 lines below */
    if (pUnhookWinEvent && hEvent_hook)
    {
        ret = pUnhookWinEvent(hEvent_hook);
        ok( ret, "UnhookWinEvent error %d\n", GetLastError());
        pUnhookWinEvent = 0;
    }
    hEvent_hook = 0;
    test_SetFocus();
    test_SetParent();
    test_PostMessage();
    test_ShowWindow();
    test_PeekMessage();
    test_PeekMessage2();
    test_WaitForInputIdle( test_argv[0] );
    test_scrollwindowex();
    test_messages();
    test_setwindowpos();
    test_showwindow();
    invisible_parent_tests();
    test_mdi_messages();
    test_button_messages();
    test_static_messages();
    test_listbox_messages();
    test_combobox_messages();
    test_wmime_keydown_message();
    test_paint_messages();
    test_interthread_messages();
    test_message_conversion();
    test_accelerators();
    test_timers();
    test_timers_no_wnd();
    if (hCBT_hook) test_set_hook();
    test_DestroyWindow();
    test_DispatchMessage();
    test_SendMessageTimeout();
    test_edit_messages();
    test_quit_message();
    test_SetActiveWindow();

    if (!pTrackMouseEvent)
        win_skip("TrackMouseEvent is not available\n");
    else
        test_TrackMouseEvent();

    test_SetWindowRgn();
    test_sys_menu();
    test_dialog_messages();
    test_EndDialog();
    test_nullCallback();
    test_dbcs_wm_char();
    test_unicode_wm_char();
    test_menu_messages();
    test_paintingloop();
    test_defwinproc();
    test_clipboard_viewers();
    test_keyflags();
    test_hotkey();
    test_layered_window();
    if(!winetest_interactive)
       skip("CORE-8299 : Skip Tracking popup menu tests.\n");
    else
    {
    test_TrackPopupMenu();
    test_TrackPopupMenuEmpty();
    }
    /* keep it the last test, under Windows it tends to break the tests
     * which rely on active/foreground windows being correct.
     */
    test_SetForegroundWindow();

    UnhookWindowsHookEx(hCBT_hook);
    if (pUnhookWinEvent && hEvent_hook)
    {
	ret = pUnhookWinEvent(hEvent_hook);
	ok( ret, "UnhookWinEvent error %d\n", GetLastError());
	SetLastError(0xdeadbeef);
	ok(!pUnhookWinEvent(hEvent_hook), "UnhookWinEvent succeeded\n");
	ok(GetLastError() == ERROR_INVALID_HANDLE || /* Win2k */
	   GetLastError() == 0xdeadbeef, /* Win9x */
           "unexpected error %d\n", GetLastError());
    }
    DeleteCriticalSection( &sequence_cs );
}
#endif

static void init_tests()
{
    HMODULE hModuleImm32;
    BOOL (WINAPI *pImmDisableIME)(DWORD);

    init_funcs();

    InitializeCriticalSection( &sequence_cs );
    init_procs();

    hModuleImm32 = LoadLibraryA("imm32.dll");
    if (hModuleImm32) {
        pImmDisableIME = (void *)GetProcAddress(hModuleImm32, "ImmDisableIME");
        if (pImmDisableIME)
            pImmDisableIME(0);
    }
    pImmDisableIME = NULL;
    FreeLibrary(hModuleImm32);

    if (!RegisterWindowClasses()) assert(0);

    cbt_hook_thread_id = GetCurrentThreadId();
    hCBT_hook = SetWindowsHookExA(WH_CBT, cbt_hook_proc, 0, GetCurrentThreadId());
    if (!hCBT_hook) win_skip( "cannot set global hook, will skip hook tests\n" );
}

static void cleanup_tests()
{
    BOOL ret;
    UnhookWindowsHookEx(hCBT_hook);
    if (pUnhookWinEvent && hEvent_hook)
    {
	ret = pUnhookWinEvent(hEvent_hook);
	ok( ret, "UnhookWinEvent error %d\n", GetLastError());
	SetLastError(0xdeadbeef);
	ok(!pUnhookWinEvent(hEvent_hook), "UnhookWinEvent succeeded\n");
	ok(GetLastError() == ERROR_INVALID_HANDLE || /* Win2k */
	   GetLastError() == 0xdeadbeef, /* Win9x */
           "unexpected error %d\n", GetLastError());
    }
    DeleteCriticalSection( &sequence_cs );

}

START_TEST(msg_queue)
{
    int argc;
    char **test_argv;
    argc = winetest_get_mainargs( &test_argv );
    if (argc >= 3)
    {
        unsigned int arg;
        /* Child process. */
        sscanf (test_argv[2], "%d", (unsigned int *) &arg);
        do_wait_idle_child( arg );
        return;
    }

    init_tests();
    test_PostMessage();
    test_PeekMessage();
    test_PeekMessage2();
    test_interthread_messages();
    test_DispatchMessage();
    test_SendMessageTimeout();
    test_quit_message();
    test_WaitForInputIdle( test_argv[0] );
    test_DestroyWindow();
    cleanup_tests();
}

START_TEST(msg_messages)
{
    init_tests();
    test_message_conversion();
    test_messages();
    test_wmime_keydown_message();
    test_nullCallback();
    test_dbcs_wm_char();
    test_unicode_wm_char();
    test_defwinproc();
    cleanup_tests();
}

START_TEST(msg_focus)
{
    init_tests();
    test_SetFocus();
    test_SetActiveWindow();

    /* HACK: For some reason test_SetForegroundWindow fails on Windows unless
     * we do this */
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
    /* keep it the last test, under Windows it tends to break the tests
     * which rely on active/foreground windows being correct.
     */
    test_SetForegroundWindow();
    cleanup_tests();
}

START_TEST(msg_winpos)
{
    init_tests();
    test_SetParent();
    test_ShowWindow();
    test_setwindowpos();
    test_showwindow();
    test_SetWindowRgn();
    invisible_parent_tests();
    cleanup_tests();
}

START_TEST(msg_paint)
{
    init_tests();
    test_scrollwindowex();
    test_paint_messages();
    test_paintingloop();
    cleanup_tests();
}

START_TEST(msg_input)
{
    init_tests();
    test_accelerators();
    if (!pTrackMouseEvent)
        win_skip("TrackMouseEvent is not available\n");
    else
        test_TrackMouseEvent();

    test_keyflags();
    test_hotkey();
    cleanup_tests();
}

START_TEST(msg_timer)
{
    init_tests();
    test_timers();
    test_timers_no_wnd();
    cleanup_tests();
}

typedef BOOL (WINAPI *IS_WINEVENT_HOOK_INSTALLED)(DWORD);

START_TEST(msg_hook)
{
//    HMODULE user32 = GetModuleHandleA("user32.dll");
//    IS_WINEVENT_HOOK_INSTALLED pIsWinEventHookInstalled = (IS_WINEVENT_HOOK_INSTALLED)GetProcAddress(user32, "IsWinEventHookInstalled");
    BOOL (WINAPI *pIsWinEventHookInstalled)(DWORD)= 0;/*GetProcAddress(user32, "IsWinEventHookInstalled");*/

    init_tests();

    if (pSetWinEventHook)
    {
        hEvent_hook = pSetWinEventHook(EVENT_MIN, EVENT_MAX,
                                       GetModuleHandleA(0), win_event_proc,
                                       0, GetCurrentThreadId(),
                                       WINEVENT_INCONTEXT);
        if (pIsWinEventHookInstalled && hEvent_hook)
	{
	    UINT event;
	    for (event = EVENT_MIN; event <= EVENT_MAX; event++)
		ok(pIsWinEventHookInstalled(event), "IsWinEventHookInstalled(%u) failed\n", event);
	}
    }
    if (!hEvent_hook) win_skip( "no win event hook support\n" );

    test_winevents();

    /* Fix message sequences before removing 4 lines below */
    if (pUnhookWinEvent && hEvent_hook)
    {
        BOOL ret;
        ret = pUnhookWinEvent(hEvent_hook);
        ok( ret, "UnhookWinEvent error %d\n", GetLastError());
        pUnhookWinEvent = 0;
    }
    hEvent_hook = 0;
    if (hCBT_hook) test_set_hook();
    cleanup_tests();
}

START_TEST(msg_menu)
{
    init_tests();
    test_sys_menu();
    test_menu_messages();
    if(!winetest_interactive)
       skip("CORE-8299 : Skip Tracking popup menu tests.\n");
    else
    {
    test_TrackPopupMenu();
    test_TrackPopupMenuEmpty();
    }
    cleanup_tests();
}

START_TEST(msg_mdi)
{
    init_tests();
    test_mdi_messages();
    cleanup_tests();
}

START_TEST(msg_controls)
{
    init_tests();
    test_button_messages();
    test_static_messages();
    test_listbox_messages();
    test_combobox_messages();
    test_edit_messages();
    cleanup_tests();
}

START_TEST(msg_layered_window)
{
    init_tests();
    test_layered_window();
    cleanup_tests();
}

START_TEST(msg_dialog)
{
    init_tests();
    test_dialog_messages();
    test_EndDialog();
    cleanup_tests();
}

START_TEST(msg_clipboard)
{
    init_tests();
    test_clipboard_viewers();
    cleanup_tests();
}
