/* Test Key event to Key message translation
 *
 * Copyright 2003 Rein Klazes
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

/* test whether the right type of messages:
 * WM_KEYUP/DOWN vs WM_SYSKEYUP/DOWN  are sent in case of combined
 * keystrokes.
 *
 * For instance <ALT>-X can be accomplished by
 * the sequence ALT-KEY-DOWN, X-KEY-DOWN, ALT-KEY-UP, X-KEY-UP
 * but also X-KEY-DOWN, ALT-KEY-DOWN, X-KEY-UP, ALT-KEY-UP
 * Whether a KEY or a SYSKEY message is sent is not always clear, it is
 * also not the same in WINNT as in WIN9X */

/* NOTE that there will be test failures under WIN9X
 * No applications are known to me that rely on this
 * so I don't fix it */

/* TODO:
 * 1. extend it to the wm_command and wm_syscommand notifications
 * 2. add some more tests with special cases like dead keys or right (alt) key
 * 3. there is some adapted code from input.c in here. Should really
 *    make that code exactly the same.
 * 4. resolve the win9x case when there is a need or the testing frame work
 *    offers a nice way.
 * 5. The test app creates a window, the user should not take the focus
 *    away during its short existence. I could do something to prevent that
 *    if it is a problem.
 *
 */

#include <stdarg.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "winnls.h"

#include "wine/test.h"

#ifdef __REACTOS__
#include <reactos/undocuser.h>
#endif

/* globals */
static HWND hWndTest;
static LONG timetag = 0x10000000;

static struct {
    LONG last_key_down;
    LONG last_key_up;
    LONG last_syskey_down;
    LONG last_syskey_up;
    LONG last_char;
    LONG last_syschar;
    LONG last_hook_down;
    LONG last_hook_up;
    LONG last_hook_syskey_down;
    LONG last_hook_syskey_up;
    WORD vk;
    BOOL expect_alt;
    BOOL sendinput_broken;
} key_status;

#if (_WIN32_WINNT >= 0x0602)
static BOOL (WINAPI *pGetCurrentInputMessageSource)( INPUT_MESSAGE_SOURCE *source );
static BOOL (WINAPI *pGetPointerType)(UINT32, POINTER_INPUT_TYPE*);
#endif
static int (WINAPI *pGetMouseMovePointsEx) (UINT, LPMOUSEMOVEPOINT, LPMOUSEMOVEPOINT, int, DWORD);
static UINT (WINAPI *pGetRawInputDeviceList) (PRAWINPUTDEVICELIST, PUINT, UINT);
static UINT (WINAPI *pGetRawInputDeviceInfoW) (HANDLE, UINT, void *, UINT *);
static UINT (WINAPI *pGetRawInputDeviceInfoA) (HANDLE, UINT, void *, UINT *);
static int  (WINAPI *pGetWindowRgnBox)(HWND, LPRECT);

#define MAXKEYEVENTS 12
#define MAXKEYMESSAGES MAXKEYEVENTS /* assuming a key event generates one
                                       and only one message */

/* keyboard message names, sorted as their value */
static const char *MSGNAME[]={"WM_KEYDOWN", "WM_KEYUP", "WM_CHAR","WM_DEADCHAR",
    "WM_SYSKEYDOWN", "WM_SYSKEYUP", "WM_SYSCHAR", "WM_SYSDEADCHAR" ,"WM_KEYLAST"};

/* keyevents, add more as needed */
typedef enum KEVtag
{  ALTDOWN = 1, ALTUP, XDOWN, XUP, SHIFTDOWN, SHIFTUP, CTRLDOWN, CTRLUP } KEV;
/* matching VK's */
static const int GETVKEY[]={0, VK_MENU, VK_MENU, 'X', 'X', VK_SHIFT, VK_SHIFT, VK_CONTROL, VK_CONTROL};
/* matching scan codes */
static const int GETSCAN[]={0, 0x38, 0x38, 0x2D, 0x2D, 0x2A, 0x2A, 0x1D, 0x1D };
/* matching updown events */
static const int GETFLAGS[]={0, 0, KEYEVENTF_KEYUP, 0, KEYEVENTF_KEYUP, 0, KEYEVENTF_KEYUP, 0, KEYEVENTF_KEYUP};
/* matching descriptions */
static const char *getdesc[]={"", "+alt","-alt","+X","-X","+shift","-shift","+ctrl","-ctrl"};

/* The MSVC headers ignore our NONAMELESSUNION requests so we have to define our own type */
typedef struct
{
    DWORD type;
    union
    {
        MOUSEINPUT      mi;
        KEYBDINPUT      ki;
        HARDWAREINPUT   hi;
    } u;
} TEST_INPUT;

typedef struct {
    UINT    message;
    WPARAM  wParam;
    LPARAM  lParam;
} KMSG;

/*******************************************
 * add new test sets here
 * the software will make all combinations of the
 * keyevent defined here
 */
static const struct {
    int nrkev;
    KEV keydwn[MAXKEYEVENTS];
    KEV keyup[MAXKEYEVENTS];
} testkeyset[]= {
    { 2, { ALTDOWN, XDOWN }, { ALTUP, XUP}},
    { 3, { ALTDOWN, XDOWN , SHIFTDOWN}, { ALTUP, XUP, SHIFTUP}},
    { 3, { ALTDOWN, XDOWN , CTRLDOWN}, { ALTUP, XUP, CTRLUP}},
    { 3, { SHIFTDOWN, XDOWN , CTRLDOWN}, { SHIFTUP, XUP, CTRLUP}},
    { 0 } /* mark the end */
};

/**********************adapted from input.c **********************************/

static BYTE InputKeyStateTable[256];
static BYTE AsyncKeyStateTable[256];
static BYTE TrackSysKey = 0; /* determine whether ALT key up will cause a WM_SYSKEYUP
                         or a WM_KEYUP message */

static void init_function_pointers(void)
{
    HMODULE hdll = GetModuleHandleA("user32");

#define GET_PROC(func) \
    if (!(p ## func = (void*)GetProcAddress(hdll, #func))) \
      trace("GetProcAddress(%s) failed\n", #func)

#if (_WIN32_WINNT >= 0x0602)
    GET_PROC(GetCurrentInputMessageSource);
#endif
    GET_PROC(GetMouseMovePointsEx);
#if (_WIN32_WINNT >= 0x0602)
    GET_PROC(GetPointerType);
#endif
    GET_PROC(GetRawInputDeviceList);
    GET_PROC(GetRawInputDeviceInfoW);
    GET_PROC(GetRawInputDeviceInfoA);
    GET_PROC(GetWindowRgnBox);
#undef GET_PROC
}

static int KbdMessage( KEV kev, WPARAM *pwParam, LPARAM *plParam )
{
    UINT message;
    int VKey = GETVKEY[kev];
    WORD flags;

    flags = LOBYTE(GETSCAN[kev]);
    if (GETFLAGS[kev] & KEYEVENTF_EXTENDEDKEY) flags |= KF_EXTENDED;

    if (GETFLAGS[kev] & KEYEVENTF_KEYUP )
    {
        message = WM_KEYUP;
        if( (InputKeyStateTable[VK_MENU] & 0x80) && (
                (VKey == VK_MENU) || (VKey == VK_CONTROL) ||
                 !(InputKeyStateTable[VK_CONTROL] & 0x80))) {
            if(  TrackSysKey == VK_MENU || /* <ALT>-down/<ALT>-up sequence */
                    (VKey != VK_MENU)) /* <ALT>-down...<something else>-up */
                message = WM_SYSKEYUP;
            TrackSysKey = 0;
        }
        InputKeyStateTable[VKey] &= ~0x80;
        flags |= KF_REPEAT | KF_UP;
    }
    else
    {
        if (InputKeyStateTable[VKey] & 0x80) flags |= KF_REPEAT;
        if (!(InputKeyStateTable[VKey] & 0x80)) InputKeyStateTable[VKey] ^= 0x01;
        InputKeyStateTable[VKey] |= 0x80;
        AsyncKeyStateTable[VKey] |= 0x80;

        message = WM_KEYDOWN;
        if( (InputKeyStateTable[VK_MENU] & 0x80) &&
                !(InputKeyStateTable[VK_CONTROL] & 0x80)) {
            message = WM_SYSKEYDOWN;
            TrackSysKey = VKey;
        }
    }

    if (InputKeyStateTable[VK_MENU] & 0x80) flags |= KF_ALTDOWN;

    if( plParam) *plParam = MAKELPARAM( 1, flags );
    if( pwParam) *pwParam = VKey;
    return message;
}

/****************************** end copy input.c ****************************/

/*
 * . prepare the keyevents for SendInputs
 * . calculate the "expected" messages
 * . Send the events to our window
 * . retrieve the messages from the input queue
 * . verify
 */
static BOOL do_test( HWND hwnd, int seqnr, const KEV td[] )
{
    TEST_INPUT inputs[MAXKEYEVENTS];
    KMSG expmsg[MAXKEYEVENTS];
    MSG msg;
    char buf[100];
    UINT evtctr=0, ret;
    int kmctr, i;

    buf[0]='\0';
    TrackSysKey=0; /* see input.c */
    for (i = 0; i < MAXKEYEVENTS; i++)
    {
        inputs[evtctr].type = INPUT_KEYBOARD;
        inputs[evtctr].u.ki.wVk = GETVKEY[td[i]];
        inputs[evtctr].u.ki.wScan = GETSCAN[td[i]];
        inputs[evtctr].u.ki.dwFlags = GETFLAGS[td[i]];
        inputs[evtctr].u.ki.dwExtraInfo = 0;
        inputs[evtctr].u.ki.time = ++timetag;
        if (td[i]) evtctr++;

        strcat(buf, getdesc[td[i]]);
        if(td[i])
            expmsg[i].message = KbdMessage(td[i], &(expmsg[i].wParam), &(expmsg[i].lParam));
        else
            expmsg[i].message = 0;
    }
    for( kmctr = 0; kmctr < MAXKEYEVENTS && expmsg[kmctr].message; kmctr++)
        ;
    ok( evtctr <= MAXKEYEVENTS, "evtctr is above MAXKEYEVENTS\n" );
    ret = SendInput(evtctr, (INPUT *)inputs, sizeof(INPUT));
    ok(ret == evtctr, "SendInput failed to send some events\n");
    i = 0;
    if (winetest_debug > 1)
        trace("======== key stroke sequence #%d: %s =============\n",
            seqnr + 1, buf);
    while( PeekMessageA(&msg,hwnd,WM_KEYFIRST,WM_KEYLAST,PM_REMOVE) ) {
        if (winetest_debug > 1)
            trace("message[%d] %-15s wParam %04lx lParam %08lx time %x\n", i,
                  MSGNAME[msg.message - WM_KEYFIRST], msg.wParam, msg.lParam, msg.time);
        if( i < kmctr ) {
            ok( msg.message == expmsg[i].message &&
                msg.wParam == expmsg[i].wParam &&
                msg.lParam == expmsg[i].lParam,
                "%u/%u: wrong message %x/%08lx/%08lx expected %s/%08lx/%08lx\n",
                seqnr, i, msg.message, msg.wParam, msg.lParam,
                MSGNAME[(expmsg[i]).message - WM_KEYFIRST], expmsg[i].wParam, expmsg[i].lParam );
        }
        i++;
    }
    if (winetest_debug > 1)
        trace("%d messages retrieved\n", i);
    if (!i && kmctr)
    {
        skip( "simulated keyboard input doesn't work\n" );
        return FALSE;
    }
    ok( i == kmctr, "message count is wrong: got %d expected: %d\n", i, kmctr);
    return TRUE;
}

/* test all combinations of the specified key events */
static BOOL TestASet( HWND hWnd, int nrkev, const KEV kevdwn[], const KEV kevup[] )
{
    int i,j,k,l,m,n;
    static int count=0;
    KEV kbuf[MAXKEYEVENTS];
    assert( nrkev==2 || nrkev==3);
    for(i=0;i<MAXKEYEVENTS;i++) kbuf[i]=0;
    /* two keys involved gives 4 test cases */
    if(nrkev==2) {
        for(i=0;i<nrkev;i++) {
            for(j=0;j<nrkev;j++) {
                kbuf[0] = kevdwn[i];
                kbuf[1] = kevdwn[1-i];
                kbuf[2] = kevup[j];
                kbuf[3] = kevup[1-j];
                if (!do_test( hWnd, count++, kbuf)) return FALSE;
            }
        }
    }
    /* three keys involved gives 36 test cases */
    if(nrkev==3){
        for(i=0;i<nrkev;i++){
            for(j=0;j<nrkev;j++){
                if(j==i) continue;
                for(k=0;k<nrkev;k++){
                    if(k==i || k==j) continue;
                    for(l=0;l<nrkev;l++){
                        for(m=0;m<nrkev;m++){
                            if(m==l) continue;
                            for(n=0;n<nrkev;n++){
                                if(n==l ||n==m) continue;
                                kbuf[0] = kevdwn[i];
                                kbuf[1] = kevdwn[j];
                                kbuf[2] = kevdwn[k];
                                kbuf[3] = kevup[l];
                                kbuf[4] = kevup[m];
                                kbuf[5] = kevup[n];
                                if (!do_test( hWnd, count++, kbuf)) return FALSE;
                            }
                        }
                    }
                }
            }
        }
    }
    return TRUE;
}

/* test each set specified in the global testkeyset array */
static void TestSysKeys( HWND hWnd)
{
    int i;
    for(i=0; testkeyset[i].nrkev;i++)
        if (!TestASet( hWnd, testkeyset[i].nrkev, testkeyset[i].keydwn, testkeyset[i].keyup)) break;
}

static LRESULT CALLBACK WndProc( HWND hWnd, UINT msg, WPARAM wParam,
        LPARAM lParam )
{
    return DefWindowProcA( hWnd, msg, wParam, lParam );
}

static void test_Input_whitebox(void)
{
    MSG msg;
    WNDCLASSA  wclass;
    HANDLE hInstance = GetModuleHandleA( NULL );

    wclass.lpszClassName = "InputSysKeyTestClass";
    wclass.style         = CS_HREDRAW | CS_VREDRAW;
    wclass.lpfnWndProc   = WndProc;
    wclass.hInstance     = hInstance;
    wclass.hIcon         = LoadIconA( 0, (LPCSTR)IDI_APPLICATION );
    wclass.hCursor       = LoadCursorA( NULL, (LPCSTR)IDC_ARROW );
    wclass.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
    wclass.lpszMenuName = 0;
    wclass.cbClsExtra    = 0;
    wclass.cbWndExtra    = 0;
    RegisterClassA( &wclass );
    /* create the test window that will receive the keystrokes */
    hWndTest = CreateWindowA( wclass.lpszClassName, "InputSysKeyTest",
                              WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 100, 100,
                              NULL, NULL, hInstance, NULL);
    assert( hWndTest );
    ShowWindow( hWndTest, SW_SHOW);
    SetWindowPos( hWndTest, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE );
    SetForegroundWindow( hWndTest );
    UpdateWindow( hWndTest);

    /* flush pending messages */
    while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

    SetFocus( hWndTest );
    TestSysKeys( hWndTest );
    DestroyWindow(hWndTest);
}

static inline BOOL is_keyboard_message( UINT message )
{
    return (message >= WM_KEYFIRST && message <= WM_KEYLAST);
}

static inline BOOL is_mouse_message( UINT message )
{
    return (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST);
}

/* try to make sure pending X events have been processed before continuing */
static void empty_message_queue(void)
{
    MSG msg;
    int diff = 200;
    int min_timeout = 50;
    DWORD time = GetTickCount() + diff;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects(0, NULL, FALSE, min_timeout, QS_ALLINPUT) == WAIT_TIMEOUT) break;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
        {
            if (is_keyboard_message(msg.message) || is_mouse_message(msg.message))
                ok(msg.time != 0, "message %#x has time set to 0\n", msg.message);

            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        diff = time - GetTickCount();
    }
}

struct transition_s {
    WORD wVk;
    BYTE before_state;
    BYTE optional;
};

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
    winevent_hook=0x200
} msg_flags_t;

struct message {
    UINT message;          /* the WM_* code */
    msg_flags_t flags;     /* message props */
    WPARAM wParam;         /* expected value of wParam */
    LPARAM lParam;         /* expected value of lParam */
};

static const struct sendinput_test_s {
    WORD wVk;
    DWORD dwFlags;
    BOOL _todo_wine;
    struct transition_s expected_transitions[MAXKEYEVENTS+1];
    struct message expected_messages[MAXKEYMESSAGES+1];
} sendinput_test[] = {
    /* test ALT+F */
    /* 0 */
    {VK_LMENU, 0, FALSE, {{VK_MENU, 0x00}, {VK_LMENU, 0x00}, {0}},
        {{WM_SYSKEYDOWN, hook|wparam, VK_LMENU}, {WM_SYSKEYDOWN}, {0}}},
    {'F', 0, FALSE, {{'F', 0x00}, {0}},
        {{WM_SYSKEYDOWN, hook}, {WM_SYSKEYDOWN},
        {WM_SYSCHAR},
        {WM_SYSCOMMAND}, {0}}},
    {'F', KEYEVENTF_KEYUP, FALSE, {{'F', 0x80}, {0}},
        {{WM_SYSKEYUP, hook}, {WM_SYSKEYUP}, {0}}},
    {VK_LMENU, KEYEVENTF_KEYUP, FALSE, {{VK_MENU, 0x80}, {VK_LMENU, 0x80}, {0}},
        {{WM_KEYUP, hook}, {WM_KEYUP}, {0}}},

    /* test CTRL+O */
    /* 4 */
    {VK_LCONTROL, 0, FALSE, {{VK_CONTROL, 0x00}, {VK_LCONTROL, 0x00}, {0}},
        {{WM_KEYDOWN, hook}, {WM_KEYDOWN}, {0}}},
    {'O', 0, FALSE, {{'O', 0x00}, {0}},
        {{WM_KEYDOWN, hook}, {WM_KEYDOWN}, {WM_CHAR}, {0}}},
    {'O', KEYEVENTF_KEYUP, FALSE, {{'O', 0x80}, {0}},
        {{WM_KEYUP, hook}, {WM_KEYUP}, {0}}},
    {VK_LCONTROL, KEYEVENTF_KEYUP, FALSE, {{VK_CONTROL, 0x80}, {VK_LCONTROL, 0x80}, {0}},
        {{WM_KEYUP, hook}, {WM_KEYUP}, {0}}},

    /* test ALT+CTRL+X */
    /* 8 */
    {VK_LMENU, 0, FALSE, {{VK_MENU, 0x00}, {VK_LMENU, 0x00}, {0}},
        {{WM_SYSKEYDOWN, hook}, {WM_SYSKEYDOWN}, {0}}},
    {VK_LCONTROL, 0, FALSE, {{VK_CONTROL, 0x00}, {VK_LCONTROL, 0x00}, {0}},
        {{WM_KEYDOWN, hook}, {WM_KEYDOWN}, {0}}},
    {'X', 0, FALSE, {{'X', 0x00}, {0}},
        {{WM_KEYDOWN, hook}, {WM_KEYDOWN}, {0}}},
    {'X', KEYEVENTF_KEYUP, FALSE, {{'X', 0x80}, {0}},
        {{WM_KEYUP, hook}, {WM_KEYUP}, {0}}},
    {VK_LCONTROL, KEYEVENTF_KEYUP, FALSE, {{VK_CONTROL, 0x80}, {VK_LCONTROL, 0x80}, {0}},
        {{WM_SYSKEYUP, hook}, {WM_SYSKEYUP}, {0}}},
    {VK_LMENU, KEYEVENTF_KEYUP, FALSE, {{VK_MENU, 0x80}, {VK_LMENU, 0x80}, {0}},
        {{WM_KEYUP, hook}, {WM_KEYUP}, {0}}},

    /* test SHIFT+A */
    /* 14 */
    {VK_LSHIFT, 0, FALSE, {{VK_SHIFT, 0x00}, {VK_LSHIFT, 0x00}, {0}},
        {{WM_KEYDOWN, hook}, {WM_KEYDOWN}, {0}}},
    {'A', 0, FALSE, {{'A', 0x00}, {0}},
        {{WM_KEYDOWN, hook}, {WM_KEYDOWN}, {WM_CHAR}, {0}}},
    {'A', KEYEVENTF_KEYUP, FALSE, {{'A', 0x80}, {0}},
        {{WM_KEYUP, hook}, {WM_KEYUP}, {0}}},
    {VK_LSHIFT, KEYEVENTF_KEYUP, FALSE, {{VK_SHIFT, 0x80}, {VK_LSHIFT, 0x80}, {0}},
        {{WM_KEYUP, hook}, {WM_KEYUP}, {0}}},
    /* test L-SHIFT & R-SHIFT: */
    /* RSHIFT == LSHIFT */
    /* 18 */
    {VK_RSHIFT, 0, FALSE,
     /* recent windows versions (>= w2k3) correctly report an RSHIFT transition */
       {{VK_SHIFT, 0x00}, {VK_LSHIFT, 0x00, TRUE}, {VK_RSHIFT, 0x00, TRUE}, {0}},
        {{WM_KEYDOWN, hook|wparam, VK_RSHIFT},
        {WM_KEYDOWN}, {0}}},
    {VK_RSHIFT, KEYEVENTF_KEYUP, FALSE,
       {{VK_SHIFT, 0x80}, {VK_LSHIFT, 0x80, TRUE}, {VK_RSHIFT, 0x80, TRUE}, {0}},
        {{WM_KEYUP, hook, hook|wparam, VK_RSHIFT},
        {WM_KEYUP}, {0}}},

    /* LSHIFT | KEYEVENTF_EXTENDEDKEY == RSHIFT */
    /* 20 */
    {VK_LSHIFT, KEYEVENTF_EXTENDEDKEY, FALSE,
        {{VK_SHIFT, 0x00}, {VK_RSHIFT, 0x00}, {0}},
        {{WM_KEYDOWN, hook|wparam|lparam, VK_LSHIFT, LLKHF_EXTENDED},
        {WM_KEYDOWN, wparam|lparam, VK_SHIFT, 0}, {0}}},
    {VK_LSHIFT, KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY, FALSE,
        {{VK_SHIFT, 0x80}, {VK_RSHIFT, 0x80}, {0}},
        {{WM_KEYUP, hook|wparam|lparam, VK_LSHIFT, LLKHF_UP|LLKHF_EXTENDED},
        {WM_KEYUP, wparam|lparam, VK_SHIFT, KF_UP}, {0}}},
    /* RSHIFT | KEYEVENTF_EXTENDEDKEY == RSHIFT */
    /* 22 */
    {VK_RSHIFT, KEYEVENTF_EXTENDEDKEY, FALSE,
        {{VK_SHIFT, 0x00}, {VK_RSHIFT, 0x00}, {0}},
        {{WM_KEYDOWN, hook|wparam|lparam, VK_RSHIFT, LLKHF_EXTENDED},
        {WM_KEYDOWN, wparam|lparam, VK_SHIFT, 0}, {0}}},
    {VK_RSHIFT, KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY, FALSE,
        {{VK_SHIFT, 0x80}, {VK_RSHIFT, 0x80}, {0}},
        {{WM_KEYUP, hook|wparam|lparam, VK_RSHIFT, LLKHF_UP|LLKHF_EXTENDED},
        {WM_KEYUP, wparam|lparam, VK_SHIFT, KF_UP}, {0}}},

    /* Note about wparam for hook with generic key (VK_SHIFT, VK_CONTROL, VK_MENU):
       win2k  - sends to hook whatever we generated here
       winXP+ - Attempts to convert key to L/R key but not always correct
    */
    /* SHIFT == LSHIFT */
    /* 24 */
    {VK_SHIFT, 0, FALSE,
        {{VK_SHIFT, 0x00}, {VK_LSHIFT, 0x00}, {0}},
        {{WM_KEYDOWN, hook/* |wparam */|lparam, VK_SHIFT, 0},
        {WM_KEYDOWN, wparam|lparam, VK_SHIFT, 0}, {0}}},
    {VK_SHIFT, KEYEVENTF_KEYUP, FALSE,
        {{VK_SHIFT, 0x80}, {VK_LSHIFT, 0x80}, {0}},
        {{WM_KEYUP, hook/*|wparam*/|lparam, VK_SHIFT, LLKHF_UP},
        {WM_KEYUP, wparam|lparam, VK_SHIFT, KF_UP}, {0}}},
    /* SHIFT | KEYEVENTF_EXTENDEDKEY == RSHIFT */
    /* 26 */
    {VK_SHIFT, KEYEVENTF_EXTENDEDKEY, FALSE,
        {{VK_SHIFT, 0x00}, {VK_RSHIFT, 0x00}, {0}},
        {{WM_KEYDOWN, hook/*|wparam*/|lparam, VK_SHIFT, LLKHF_EXTENDED},
        {WM_KEYDOWN, wparam|lparam, VK_SHIFT, 0}, {0}}},
    {VK_SHIFT, KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY, FALSE,
        {{VK_SHIFT, 0x80}, {VK_RSHIFT, 0x80}, {0}},
        {{WM_KEYUP, hook/*|wparam*/|lparam, VK_SHIFT, LLKHF_UP|LLKHF_EXTENDED},
        {WM_KEYUP, wparam|lparam, VK_SHIFT, KF_UP}, {0}}},

    /* test L-CONTROL & R-CONTROL: */
    /* RCONTROL == LCONTROL */
    /* 28 */
    {VK_RCONTROL, 0, FALSE,
        {{VK_CONTROL, 0x00}, {VK_LCONTROL, 0x00}, {0}},
        {{WM_KEYDOWN, hook|wparam, VK_RCONTROL},
        {WM_KEYDOWN, wparam|lparam, VK_CONTROL, 0}, {0}}},
    {VK_RCONTROL, KEYEVENTF_KEYUP, FALSE,
        {{VK_CONTROL, 0x80}, {VK_LCONTROL, 0x80}, {0}},
        {{WM_KEYUP, hook|wparam, VK_RCONTROL},
        {WM_KEYUP, wparam|lparam, VK_CONTROL, KF_UP}, {0}}},
    /* LCONTROL | KEYEVENTF_EXTENDEDKEY == RCONTROL */
    /* 30 */
    {VK_LCONTROL, KEYEVENTF_EXTENDEDKEY, FALSE,
        {{VK_CONTROL, 0x00}, {VK_RCONTROL, 0x00}, {0}},
        {{WM_KEYDOWN, hook|wparam|lparam, VK_LCONTROL, LLKHF_EXTENDED},
        {WM_KEYDOWN, wparam|lparam, VK_CONTROL, KF_EXTENDED}, {0}}},
    {VK_LCONTROL, KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY, FALSE,
        {{VK_CONTROL, 0x80}, {VK_RCONTROL, 0x80}, {0}},
        {{WM_KEYUP, hook|wparam|lparam, VK_LCONTROL, LLKHF_UP|LLKHF_EXTENDED},
        {WM_KEYUP, wparam|lparam, VK_CONTROL, KF_UP|KF_EXTENDED}, {0}}},
    /* RCONTROL | KEYEVENTF_EXTENDEDKEY == RCONTROL */
    /* 32 */
    {VK_RCONTROL, KEYEVENTF_EXTENDEDKEY, FALSE,
        {{VK_CONTROL, 0x00}, {VK_RCONTROL, 0x00}, {0}},
        {{WM_KEYDOWN, hook|wparam|lparam, VK_RCONTROL, LLKHF_EXTENDED},
        {WM_KEYDOWN, wparam|lparam, VK_CONTROL, KF_EXTENDED}, {0}}},
    {VK_RCONTROL, KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY, FALSE,
        {{VK_CONTROL, 0x80}, {VK_RCONTROL, 0x80}, {0}},
        {{WM_KEYUP, hook|wparam|lparam, VK_RCONTROL, LLKHF_UP|LLKHF_EXTENDED},
        {WM_KEYUP, wparam|lparam, VK_CONTROL, KF_UP|KF_EXTENDED}, {0}}},
    /* CONTROL == LCONTROL */
    /* 34 */
    {VK_CONTROL, 0, FALSE,
        {{VK_CONTROL, 0x00}, {VK_LCONTROL, 0x00}, {0}},
        {{WM_KEYDOWN, hook/*|wparam, VK_CONTROL*/},
        {WM_KEYDOWN, wparam|lparam, VK_CONTROL, 0}, {0}}},
    {VK_CONTROL, KEYEVENTF_KEYUP, FALSE,
        {{VK_CONTROL, 0x80}, {VK_LCONTROL, 0x80}, {0}},
        {{WM_KEYUP, hook/*|wparam, VK_CONTROL*/},
        {WM_KEYUP, wparam|lparam, VK_CONTROL, KF_UP}, {0}}},
    /* CONTROL | KEYEVENTF_EXTENDEDKEY == RCONTROL */
    /* 36 */
    {VK_CONTROL, KEYEVENTF_EXTENDEDKEY, FALSE,
        {{VK_CONTROL, 0x00}, {VK_RCONTROL, 0x00}, {0}},
        {{WM_KEYDOWN, hook/*|wparam*/|lparam, VK_CONTROL, LLKHF_EXTENDED},
        {WM_KEYDOWN, wparam|lparam, VK_CONTROL, KF_EXTENDED}, {0}}},
    {VK_CONTROL, KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY, FALSE,
        {{VK_CONTROL, 0x80}, {VK_RCONTROL, 0x80}, {0}},
        {{WM_KEYUP, hook/*|wparam*/|lparam, VK_CONTROL, LLKHF_UP|LLKHF_EXTENDED},
        {WM_KEYUP, wparam|lparam, VK_CONTROL, KF_UP|KF_EXTENDED}, {0}}},

    /* test L-MENU & R-MENU: */
    /* RMENU == LMENU */
    /* 38 */
    {VK_RMENU, 0, FALSE,
        {{VK_MENU, 0x00}, {VK_LMENU, 0x00}, {VK_CONTROL, 0x00, 1}, {VK_LCONTROL, 0x01, 1}, {0}},
        {{WM_SYSKEYDOWN, hook|wparam|optional, VK_LCONTROL},
        {WM_SYSKEYDOWN, hook|wparam, VK_RMENU},
        {WM_KEYDOWN, wparam|lparam|optional, VK_CONTROL, 0},
        {WM_SYSKEYDOWN, wparam|lparam, VK_MENU, 0}, {0}}},
    {VK_RMENU, KEYEVENTF_KEYUP, TRUE,
        {{VK_MENU, 0x80}, {VK_LMENU, 0x80}, {VK_CONTROL, 0x81, 1}, {VK_LCONTROL, 0x80, 1}, {0}},
        {{WM_KEYUP, hook|wparam|optional, VK_LCONTROL},
        {WM_KEYUP, hook|wparam, VK_RMENU},
        {WM_SYSKEYUP, wparam|lparam|optional, VK_CONTROL, KF_UP},
        {WM_SYSKEYUP, wparam|lparam, VK_MENU, KF_UP},
        {WM_SYSCOMMAND, optional}, {0}}},
    /* LMENU | KEYEVENTF_EXTENDEDKEY == RMENU */
    /* 40 */
    {VK_LMENU, KEYEVENTF_EXTENDEDKEY, FALSE,
        {{VK_MENU, 0x00}, {VK_RMENU, 0x00}, {0}},
        {{WM_SYSKEYDOWN, hook|wparam|lparam, VK_LMENU, LLKHF_EXTENDED},
        {WM_SYSKEYDOWN, wparam|lparam, VK_MENU, KF_EXTENDED}, {0}}},
    {VK_LMENU, KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY, TRUE,
        {{VK_MENU, 0x80}, {VK_RMENU, 0x80}, {0}},
        {{WM_KEYUP, hook|wparam|lparam, VK_LMENU, LLKHF_UP|LLKHF_EXTENDED},
        {WM_SYSKEYUP, wparam|lparam, VK_MENU, KF_UP|KF_EXTENDED},
        {WM_SYSCOMMAND}, {0}}},
    /* RMENU | KEYEVENTF_EXTENDEDKEY == RMENU */
    /* 42 */
    {VK_RMENU, KEYEVENTF_EXTENDEDKEY, FALSE,
        {{VK_MENU, 0x00}, {VK_RMENU, 0x00}, {VK_CONTROL, 0x00, 1}, {VK_LCONTROL, 0x01, 1}, {0}},
        {{WM_SYSKEYDOWN, hook|wparam|lparam|optional, VK_LCONTROL, 0},
        {WM_SYSKEYDOWN, hook|wparam|lparam, VK_RMENU, LLKHF_EXTENDED},
        {WM_KEYDOWN, wparam|lparam|optional, VK_CONTROL, 0},
        {WM_SYSKEYDOWN, wparam|lparam, VK_MENU, KF_EXTENDED}, {0}}},
    {VK_RMENU, KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY, TRUE,
        {{VK_MENU, 0x80}, {VK_RMENU, 0x80}, {VK_CONTROL, 0x81, 1}, {VK_LCONTROL, 0x80, 1}, {0}},
        {{WM_KEYUP, hook|wparam|lparam|optional, VK_LCONTROL, LLKHF_UP},
        {WM_KEYUP, hook|wparam|lparam, VK_RMENU, LLKHF_UP|LLKHF_EXTENDED},
        {WM_SYSKEYUP, wparam|lparam|optional, VK_CONTROL, KF_UP},
        {WM_SYSKEYUP, wparam|lparam, VK_MENU, KF_UP|KF_EXTENDED},
        {WM_SYSCOMMAND, optional}, {0}}},
    /* MENU == LMENU */
    /* 44 */
    {VK_MENU, 0, FALSE,
        {{VK_MENU, 0x00}, {VK_LMENU, 0x00}, {0}},
        {{WM_SYSKEYDOWN, hook/*|wparam, VK_MENU*/},
        {WM_SYSKEYDOWN, wparam|lparam, VK_MENU, 0}, {0}}},
    {VK_MENU, KEYEVENTF_KEYUP, TRUE,
        {{VK_MENU, 0x80}, {VK_LMENU, 0x80}, {0}},
        {{WM_KEYUP, hook/*|wparam, VK_MENU*/},
        {WM_SYSKEYUP, wparam|lparam, VK_MENU, KF_UP},
        {WM_SYSCOMMAND}, {0}}},
    /* MENU | KEYEVENTF_EXTENDEDKEY == RMENU */
    /* 46 */
    {VK_MENU, KEYEVENTF_EXTENDEDKEY, FALSE,
        {{VK_MENU, 0x00}, {VK_RMENU, 0x00}, {VK_CONTROL, 0x00, 1}, {VK_LCONTROL, 0x01, 1}, {0}},
        {{WM_SYSKEYDOWN, hook|wparam|lparam|optional, VK_CONTROL, 0},
        {WM_SYSKEYDOWN, hook/*|wparam*/|lparam, VK_MENU, LLKHF_EXTENDED},
        {WM_SYSKEYDOWN, wparam|lparam, VK_MENU, KF_EXTENDED}, {0}}},
    {VK_MENU, KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY, TRUE,
        {{VK_MENU, 0x80}, {VK_RMENU, 0x80}, {VK_CONTROL, 0x81, 1}, {VK_LCONTROL, 0x80, 1}, {0}},
        {{WM_KEYUP, hook|wparam|lparam|optional, VK_CONTROL, LLKHF_UP},
        {WM_KEYUP, hook/*|wparam*/|lparam, VK_MENU, LLKHF_UP|LLKHF_EXTENDED},
        {WM_SYSKEYUP, wparam|lparam, VK_MENU, KF_UP|KF_EXTENDED},
        {WM_SYSCOMMAND}, {0}}},

    /* test LSHIFT & RSHIFT */
    /* 48 */
    {VK_LSHIFT, 0, FALSE,
        {{VK_SHIFT, 0x00}, {VK_LSHIFT, 0x00}, {0}},
        {{WM_KEYDOWN, hook|wparam|lparam, VK_LSHIFT, 0},
        {WM_KEYDOWN, wparam|lparam, VK_SHIFT, 0}, {0}}},
    {VK_RSHIFT, KEYEVENTF_EXTENDEDKEY, FALSE,
        {{VK_RSHIFT, 0x00}, {0}},
        {{WM_KEYDOWN, hook|wparam|lparam, VK_RSHIFT, LLKHF_EXTENDED},
        {WM_KEYDOWN, wparam|lparam, VK_SHIFT, 0}, {0}}},
    {VK_RSHIFT, KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY, FALSE,
        {{VK_RSHIFT, 0x80}, {0}},
        {{WM_KEYUP, hook|wparam|lparam, VK_RSHIFT, LLKHF_UP|LLKHF_EXTENDED},
        {WM_KEYUP, optional}, {0}}},
    {VK_LSHIFT, KEYEVENTF_KEYUP, FALSE,
        {{VK_SHIFT, 0x80}, {VK_LSHIFT, 0x80}, {0}},
        {{WM_KEYUP, hook|wparam, VK_LSHIFT},
        {WM_KEYUP, wparam|lparam, VK_SHIFT, KF_UP}, {0}}},

    {0, 0, FALSE, {{0}}, {{0}}} /* end */
};

static struct message sent_messages[MAXKEYMESSAGES];
static UINT sent_messages_cnt;

/* Verify that only specified key state transitions occur */
static void compare_and_check(int id, BYTE *ks1, BYTE *ks2,
    const struct sendinput_test_s *test, BOOL foreground)
{
    int i, failcount = 0;
    const struct transition_s *t = test->expected_transitions;
    UINT actual_cnt = 0;
    const struct message *expected = test->expected_messages;

    while (t->wVk && foreground) {
        /* We won't receive any information from GetKeyboardState() if we're
         * not the foreground window. */
        BOOL matched = ((ks1[t->wVk]&0x80) == (t->before_state&0x80)
                       && (ks2[t->wVk]&0x80) == (~t->before_state&0x80));

        if (!matched && !t->optional && test->_todo_wine)
        {
            failcount++;
            todo_wine {
                ok(matched, "%2d (%x/%x): %02x from %02x -> %02x "
                   "instead of %02x -> %02x\n", id, test->wVk, test->dwFlags,
                   t->wVk, ks1[t->wVk]&0x80, ks2[t->wVk]&0x80, t->before_state,
                   ~t->before_state&0x80);
            }
        } else {
            ok(matched || t->optional, "%2d (%x/%x): %02x from %02x -> %02x "
               "instead of %02x -> %02x\n", id, test->wVk, test->dwFlags,
               t->wVk, ks1[t->wVk]&0x80, ks2[t->wVk]&0x80, t->before_state,
               ~t->before_state&0x80);
        }
        ks2[t->wVk] = ks1[t->wVk]; /* clear the match */
        t++;
    }
    for (i = 0; i < 256; i++)
        if (ks2[i] != ks1[i] && test->_todo_wine)
        {
            failcount++;
            todo_wine
                ok(FALSE, "%2d (%x/%x): %02x from %02x -> %02x unexpected\n",
                   id, test->wVk, test->dwFlags, i, ks1[i], ks2[i]);
        }
        else
            ok(ks2[i] == ks1[i], "%2d (%x/%x): %02x from %02x -> %02x unexpected\n",
               id, test->wVk, test->dwFlags, i, ks1[i], ks2[i]);

    while (expected->message && actual_cnt < sent_messages_cnt)
    {
        const struct message *actual = &sent_messages[actual_cnt];

        if (expected->message == actual->message)
        {
            if (expected->flags & wparam)
            {
                if ((expected->flags & optional) && (expected->wParam != actual->wParam))
                {
                    expected++;
                    continue;
                }
                if (expected->wParam != actual->wParam && test->_todo_wine)
                {
                    failcount++;
                    todo_wine
                        ok(FALSE, "%2d (%x/%x): in msg 0x%04x expecting wParam 0x%lx got 0x%lx\n",
                           id, test->wVk, test->dwFlags, expected->message, expected->wParam, actual->wParam);
                }
                else
                    ok(expected->wParam == actual->wParam,
                       "%2d (%x/%x): in msg 0x%04x expecting wParam 0x%lx got 0x%lx\n",
                       id, test->wVk, test->dwFlags, expected->message, expected->wParam, actual->wParam);
            }
            if (expected->flags & lparam)
            {
                if (expected->lParam != actual->lParam && test->_todo_wine)
                {
                    failcount++;
                    todo_wine
                        ok(FALSE, "%2d (%x/%x): in msg 0x%04x expecting lParam 0x%lx got 0x%lx\n",
                           id, test->wVk, test->dwFlags, expected->message, expected->lParam, actual->lParam);
                }
                else
                    ok(expected->lParam == actual->lParam,
                       "%2d (%x/%x): in msg 0x%04x expecting lParam 0x%lx got 0x%lx\n",
                       id, test->wVk, test->dwFlags, expected->message, expected->lParam, actual->lParam);
            }
            ok((expected->flags & hook) == (actual->flags & hook),
               "%2d (%x/%x): the msg 0x%04x should have been sent by a hook\n",
               id, test->wVk, test->dwFlags, expected->message);

        }
        else if (expected->flags & optional)
        {
            expected++;
            continue;
        }
        else if (!(expected->flags & hook) && !foreground)
        {
            /* If we weren't able to receive foreground status, we won't get
             * any window messages. */
            expected++;
            continue;
        }
        /* NT4 doesn't send SYSKEYDOWN/UP to hooks, only KEYDOWN/UP */
        else if ((expected->flags & hook) &&
                 (expected->message == WM_SYSKEYDOWN || expected->message == WM_SYSKEYUP) &&
                 (actual->message == expected->message - 4))
        {
            ok((expected->flags & hook) == (actual->flags & hook),
               "%2d (%x/%x): the msg 0x%04x should have been sent by a hook\n",
               id, test->wVk, test->dwFlags, expected->message);
        }
        /* For VK_RMENU, at least localized Win2k/XP sends KEYDOWN/UP
         * instead of SYSKEYDOWN/UP to the WNDPROC */
        else if (test->wVk == VK_RMENU && !(expected->flags & hook) &&
                 (expected->message == WM_SYSKEYDOWN || expected->message == WM_SYSKEYUP) &&
                 (actual->message == expected->message - 4))
        {
            ok(expected->wParam == actual->wParam && expected->lParam == actual->lParam,
               "%2d (%x/%x): the msg 0x%04x was expected, but got msg 0x%04x instead\n",
               id, test->wVk, test->dwFlags, expected->message, actual->message);
        }
        else if (test->_todo_wine)
        {
            failcount++;
            todo_wine
            ok(FALSE,
               "%2d (%x/%x): the msg 0x%04x was expected, but got msg 0x%04x instead\n",
               id, test->wVk, test->dwFlags, expected->message, actual->message);
        }
        else
            ok(FALSE,
               "%2d (%x/%x): the msg 0x%04x was expected, but got msg 0x%04x instead\n",
               id, test->wVk, test->dwFlags, expected->message, actual->message);

        actual_cnt++;
        expected++;
    }
    /* skip all optional trailing messages */
    while (expected->message && ((expected->flags & optional) || (!(expected->flags & hook) && !foreground)))
        expected++;


    if (expected->message || actual_cnt < sent_messages_cnt)
    {
        if (test->_todo_wine)
        {
            failcount++;
            todo_wine
                ok(FALSE, "%2d (%x/%x): the msg sequence is not complete: expected %04x - actual %04x\n",
                   id, test->wVk, test->dwFlags, expected->message, sent_messages[actual_cnt].message);
        }
        else
            ok(FALSE, "%2d (%x/%x): the msg sequence is not complete: expected %04x - actual %04x\n",
               id, test->wVk, test->dwFlags, expected->message, sent_messages[actual_cnt].message);
    }

    if( test->_todo_wine && !failcount) /* succeeded yet marked todo */
        todo_wine
            ok(TRUE, "%2d (%x/%x): marked \"todo_wine\" but succeeds\n", id, test->wVk, test->dwFlags);

    sent_messages_cnt = 0;
}

/* WndProc2 checks that we get at least the messages specified */
static LRESULT CALLBACK WndProc2(HWND hWnd, UINT Msg, WPARAM wParam,
                                   LPARAM lParam)
{
    if (winetest_debug > 1) trace("MSG:  %8x W:%8lx L:%8lx\n", Msg, wParam, lParam);

    if ((Msg >= WM_KEYFIRST && Msg <= WM_KEYLAST) || Msg == WM_SYSCOMMAND)
    {
        ok(sent_messages_cnt < MAXKEYMESSAGES, "Too many messages\n");
        if (sent_messages_cnt < MAXKEYMESSAGES)
        {
            sent_messages[sent_messages_cnt].message = Msg;
            sent_messages[sent_messages_cnt].flags = 0;
            sent_messages[sent_messages_cnt].wParam = wParam;
            sent_messages[sent_messages_cnt++].lParam = HIWORD(lParam) & (KF_UP|KF_EXTENDED);
        }
    }
    return DefWindowProcA(hWnd, Msg, wParam, lParam);
}

static LRESULT CALLBACK hook_proc(int code, WPARAM wparam, LPARAM lparam)
{
    KBDLLHOOKSTRUCT *hook_info = (KBDLLHOOKSTRUCT *)lparam;

    if (code == HC_ACTION)
    {
        ok(sent_messages_cnt < MAXKEYMESSAGES, "Too many messages\n");
        if (sent_messages_cnt < MAXKEYMESSAGES)
        {
            sent_messages[sent_messages_cnt].message = wparam;
            sent_messages[sent_messages_cnt].flags = hook;
            sent_messages[sent_messages_cnt].wParam = hook_info->vkCode;
            sent_messages[sent_messages_cnt++].lParam = hook_info->flags & (LLKHF_UP|LLKHF_EXTENDED);
        }

if(0) /* For some reason not stable on Wine */
{
        if (wparam == WM_KEYDOWN || wparam == WM_SYSKEYDOWN)
            ok(!(GetAsyncKeyState(hook_info->vkCode) & 0x8000), "key %x should be up\n", hook_info->vkCode);
        else if (wparam == WM_KEYUP || wparam == WM_SYSKEYUP)
            ok(GetAsyncKeyState(hook_info->vkCode) & 0x8000, "key %x should be down\n", hook_info->vkCode);
}

        if (winetest_debug > 1)
            trace("Hook:   w=%lx vk:%8x sc:%8x fl:%8x %lx\n", wparam,
                  hook_info->vkCode, hook_info->scanCode, hook_info->flags, hook_info->dwExtraInfo);
    }
    return CallNextHookEx( 0, code, wparam, lparam );
}
static void test_Input_blackbox(void)
{
    TEST_INPUT i;
    int ii;
    BYTE ks1[256], ks2[256];
    LONG_PTR prevWndProc;
    BOOL foreground;
    HWND window;
    HHOOK hook;

    if (GetKeyboardLayout(0) != (HKL)(ULONG_PTR)0x04090409)
    {
        skip("Skipping Input_blackbox test on non-US keyboard\n");
        return;
    }
    window = CreateWindowA("Static", NULL, WS_POPUP|WS_HSCROLL|WS_VSCROLL
        |WS_VISIBLE, 0, 0, 200, 60, NULL, NULL,
        NULL, NULL);
    ok(window != NULL, "error: %d\n", (int) GetLastError());
    SetWindowPos( window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE );
    foreground = SetForegroundWindow( window );
    if (!foreground)
        skip("Failed to set foreground window; some tests will be skipped.\n");

    if (!(hook = SetWindowsHookExA(WH_KEYBOARD_LL, hook_proc, GetModuleHandleA( NULL ), 0)))
    {
        DestroyWindow(window);
        win_skip("WH_KEYBOARD_LL is not supported\n");
        return;
    }

    /* must process all initial messages, otherwise X11DRV_KeymapNotify unsets
     * key state set by SendInput(). */
    empty_message_queue();

    prevWndProc = SetWindowLongPtrA(window, GWLP_WNDPROC, (LONG_PTR) WndProc2);
    ok(prevWndProc != 0 || GetLastError() == 0, "error: %d\n", (int) GetLastError());

    i.type = INPUT_KEYBOARD;
    i.u.ki.time = 0;
    i.u.ki.dwExtraInfo = 0;

    for (ii = 0; ii < ARRAY_SIZE(sendinput_test)-1; ii++) {
        GetKeyboardState(ks1);
        i.u.ki.wScan = ii+1 /* useful for debugging */;
        i.u.ki.dwFlags = sendinput_test[ii].dwFlags;
        i.u.ki.wVk = sendinput_test[ii].wVk;
        SendInput(1, (INPUT*)&i, sizeof(TEST_INPUT));
        empty_message_queue();
        GetKeyboardState(ks2);
        compare_and_check(ii, ks1, ks2, &sendinput_test[ii], foreground);
    }

    empty_message_queue();
    DestroyWindow(window);
    UnhookWindowsHookEx(hook);
}

static void reset_key_status(WORD vk)
{
    key_status.last_key_down = -1;
    key_status.last_key_up = -1;
    key_status.last_syskey_down = -1;
    key_status.last_syskey_up = -1;
    key_status.last_char = -1;
    key_status.last_syschar = -1;
    key_status.last_hook_down = -1;
    key_status.last_hook_up = -1;
    key_status.last_hook_syskey_down = -1;
    key_status.last_hook_syskey_up = -1;
    key_status.vk = vk;
    key_status.expect_alt = FALSE;
    key_status.sendinput_broken = FALSE;
}

static void test_unicode_keys(HWND hwnd, HHOOK hook)
{
    TEST_INPUT inputs[2];
    MSG msg;

    /* init input data that never changes */
    inputs[1].type = inputs[0].type = INPUT_KEYBOARD;
    inputs[1].u.ki.dwExtraInfo = inputs[0].u.ki.dwExtraInfo = 0;
    inputs[1].u.ki.time = inputs[0].u.ki.time = 0;

    /* pressing & releasing a single unicode character */
    inputs[0].u.ki.wVk = 0;
    inputs[0].u.ki.wScan = 0x3c0;
    inputs[0].u.ki.dwFlags = KEYEVENTF_UNICODE;

    reset_key_status(VK_PACKET);
    SendInput(1, (INPUT*)inputs, sizeof(INPUT));
    while(PeekMessageW(&msg, hwnd, 0, 0, PM_REMOVE)){
        if(msg.message == WM_KEYDOWN && msg.wParam == VK_PACKET){
            TranslateMessage(&msg);
        }
        DispatchMessageW(&msg);
    }
    if(!key_status.sendinput_broken){
        ok(key_status.last_key_down == VK_PACKET,
            "Last keydown msg should have been VK_PACKET[0x%04x] (was: 0x%x)\n", VK_PACKET, key_status.last_key_down);
        ok(key_status.last_char == 0x3c0,
            "Last char msg wparam should have been 0x3c0 (was: 0x%x)\n", key_status.last_char);
        if(hook)
            ok(key_status.last_hook_down == 0x3c0,
                "Last hookdown msg should have been 0x3c0, was: 0x%x\n", key_status.last_hook_down);
    }

    inputs[1].u.ki.wVk = 0;
    inputs[1].u.ki.wScan = 0x3c0;
    inputs[1].u.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

    reset_key_status(VK_PACKET);
    SendInput(1, (INPUT*)(inputs+1), sizeof(INPUT));
    while(PeekMessageW(&msg, hwnd, 0, 0, PM_REMOVE)){
        if(msg.message == WM_KEYDOWN && msg.wParam == VK_PACKET){
            TranslateMessage(&msg);
        }
        DispatchMessageW(&msg);
    }
    if(!key_status.sendinput_broken){
        ok(key_status.last_key_up == VK_PACKET,
            "Last keyup msg should have been VK_PACKET[0x%04x] (was: 0x%x)\n", VK_PACKET, key_status.last_key_up);
        if(hook)
            ok(key_status.last_hook_up == 0x3c0,
                "Last hookup msg should have been 0x3c0, was: 0x%x\n", key_status.last_hook_up);
    }

    /* holding alt, pressing & releasing a unicode character, releasing alt */
    inputs[0].u.ki.wVk = VK_LMENU;
    inputs[0].u.ki.wScan = 0;
    inputs[0].u.ki.dwFlags = 0;

    inputs[1].u.ki.wVk = 0;
    inputs[1].u.ki.wScan = 0x3041;
    inputs[1].u.ki.dwFlags = KEYEVENTF_UNICODE;

    reset_key_status(VK_PACKET);
    key_status.expect_alt = TRUE;
    SendInput(2, (INPUT*)inputs, sizeof(INPUT));
    while(PeekMessageW(&msg, hwnd, 0, 0, PM_REMOVE)){
        if(msg.message == WM_SYSKEYDOWN && msg.wParam == VK_PACKET){
            TranslateMessage(&msg);
        }
        DispatchMessageW(&msg);
    }
    if(!key_status.sendinput_broken){
        ok(key_status.last_syskey_down == VK_PACKET,
            "Last syskeydown msg should have been VK_PACKET[0x%04x] (was: 0x%x)\n", VK_PACKET, key_status.last_syskey_down);
        ok(key_status.last_syschar == 0x3041,
            "Last syschar msg should have been 0x3041 (was: 0x%x)\n", key_status.last_syschar);
        if(hook)
            ok(key_status.last_hook_syskey_down == 0x3041,
                "Last hooksysdown msg should have been 0x3041, was: 0x%x\n", key_status.last_hook_syskey_down);
    }

    inputs[1].u.ki.wVk = 0;
    inputs[1].u.ki.wScan = 0x3041;
    inputs[1].u.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

    inputs[0].u.ki.wVk = VK_LMENU;
    inputs[0].u.ki.wScan = 0;
    inputs[0].u.ki.dwFlags = KEYEVENTF_KEYUP;

    reset_key_status(VK_PACKET);
    key_status.expect_alt = TRUE;
    SendInput(2, (INPUT*)inputs, sizeof(INPUT));
    while(PeekMessageW(&msg, hwnd, 0, 0, PM_REMOVE)){
        if(msg.message == WM_SYSKEYDOWN && msg.wParam == VK_PACKET){
            TranslateMessage(&msg);
        }
        DispatchMessageW(&msg);
    }
    if(!key_status.sendinput_broken){
        ok(key_status.last_key_up == VK_PACKET,
            "Last keyup msg should have been VK_PACKET[0x%04x] (was: 0x%x)\n", VK_PACKET, key_status.last_key_up);
        if(hook)
            ok(key_status.last_hook_up == 0x3041,
                "Last hook up msg should have been 0x3041, was: 0x%x\n", key_status.last_hook_up);
    }

    /* Press and release, non-zero key code. */
    inputs[0].u.ki.wVk = 0x51;
    inputs[0].u.ki.wScan = 0x123;
    inputs[0].u.ki.dwFlags = KEYEVENTF_UNICODE;

    inputs[1].u.ki.wVk = 0x51;
    inputs[1].u.ki.wScan = 0x123;
    inputs[1].u.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

    reset_key_status(inputs[0].u.ki.wVk);
    SendInput(2, (INPUT*)inputs, sizeof(INPUT));
    while (PeekMessageW(&msg, hwnd, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (!key_status.sendinput_broken)
    {
        ok(key_status.last_key_down == 0x51, "Unexpected key down %#x.\n", key_status.last_key_down);
        ok(key_status.last_key_up == 0x51, "Unexpected key up %#x.\n", key_status.last_key_up);
        if (hook)
            todo_wine
            ok(key_status.last_hook_up == 0x23, "Unexpected hook message %#x.\n", key_status.last_hook_up);
    }
}

static LRESULT CALLBACK unicode_wnd_proc( HWND hWnd, UINT msg, WPARAM wParam,
        LPARAM lParam )
{
    switch(msg){
    case WM_KEYDOWN:
        key_status.last_key_down = wParam;
        break;
    case WM_SYSKEYDOWN:
        key_status.last_syskey_down = wParam;
        break;
    case WM_KEYUP:
        key_status.last_key_up = wParam;
        break;
    case WM_SYSKEYUP:
        key_status.last_syskey_up = wParam;
        break;
    case WM_CHAR:
        key_status.last_char = wParam;
        break;
    case WM_SYSCHAR:
        key_status.last_syschar = wParam;
        break;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static LRESULT CALLBACK llkbd_unicode_hook(int nCode, WPARAM wParam, LPARAM lParam)
{
    if(nCode == HC_ACTION){
        LPKBDLLHOOKSTRUCT info = (LPKBDLLHOOKSTRUCT)lParam;
        if(!info->vkCode){
            key_status.sendinput_broken = TRUE;
            win_skip("SendInput doesn't support unicode on this platform\n");
        }else{
            if(key_status.expect_alt){
                ok(info->vkCode == VK_LMENU, "vkCode should have been VK_LMENU[0x%04x], was: 0x%x\n", VK_LMENU, info->vkCode);
                key_status.expect_alt = FALSE;
            }else
            todo_wine_if(key_status.vk != VK_PACKET)
                ok(info->vkCode == key_status.vk, "Unexpected vkCode %#x, expected %#x.\n", info->vkCode, key_status.vk);
        }
        switch(wParam){
        case WM_KEYDOWN:
            key_status.last_hook_down = info->scanCode;
            break;
        case WM_KEYUP:
            key_status.last_hook_up = info->scanCode;
            break;
        case WM_SYSKEYDOWN:
            key_status.last_hook_syskey_down = info->scanCode;
            break;
        case WM_SYSKEYUP:
            key_status.last_hook_syskey_up = info->scanCode;
            break;
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

static void test_Input_unicode(void)
{
    WCHAR classNameW[] = {'I','n','p','u','t','U','n','i','c','o','d','e',
        'K','e','y','T','e','s','t','C','l','a','s','s',0};
    WCHAR windowNameW[] = {'I','n','p','u','t','U','n','i','c','o','d','e',
        'K','e','y','T','e','s','t',0};
    MSG msg;
    WNDCLASSW wclass;
    HANDLE hInstance = GetModuleHandleW(NULL);
    HHOOK hook;
    HMODULE hModuleImm32;
    BOOL (WINAPI *pImmDisableIME)(DWORD);

    wclass.lpszClassName = classNameW;
    wclass.style         = CS_HREDRAW | CS_VREDRAW;
    wclass.lpfnWndProc   = unicode_wnd_proc;
    wclass.hInstance     = hInstance;
    wclass.hIcon         = LoadIconW(0, (LPCWSTR)IDI_APPLICATION);
    wclass.hCursor       = LoadCursorW( NULL, (LPCWSTR)IDC_ARROW);
    wclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wclass.lpszMenuName  = 0;
    wclass.cbClsExtra    = 0;
    wclass.cbWndExtra    = 0;
    if(!RegisterClassW(&wclass)){
        win_skip("Unicode functions not supported\n");
        return;
    }

    hModuleImm32 = LoadLibraryA("imm32.dll");
    if (hModuleImm32) {
        pImmDisableIME = (void *)GetProcAddress(hModuleImm32, "ImmDisableIME");
        if (pImmDisableIME)
            pImmDisableIME(0);
    }
    pImmDisableIME = NULL;
    FreeLibrary(hModuleImm32);

    /* create the test window that will receive the keystrokes */
    hWndTest = CreateWindowW(wclass.lpszClassName, windowNameW,
                             WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 100, 100,
                             NULL, NULL, hInstance, NULL);

    assert(hWndTest);
    assert(IsWindowUnicode(hWndTest));

    hook = SetWindowsHookExW(WH_KEYBOARD_LL, llkbd_unicode_hook, GetModuleHandleW(NULL), 0);
    if(!hook)
        win_skip("unable to set WH_KEYBOARD_LL hook\n");

    ShowWindow(hWndTest, SW_SHOW);
    SetWindowPos(hWndTest, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
    SetForegroundWindow(hWndTest);
    UpdateWindow(hWndTest);

    /* flush pending messages */
    while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageW(&msg);

    SetFocus(hWndTest);

    test_unicode_keys(hWndTest, hook);

    if(hook)
        UnhookWindowsHookEx(hook);
    DestroyWindow(hWndTest);
}

static void test_keynames(void)
{
    int i, len;
    char buff[256];

    for (i = 0; i < 512; i++)
    {
        strcpy(buff, "----");
        len = GetKeyNameTextA(i << 16, buff, sizeof(buff));
        ok(len || !buff[0], "%d: Buffer is not zeroed\n", i);
    }
}

static POINT pt_old, pt_new;
static BOOL clipped;
#define STEP 3

static LRESULT CALLBACK hook_proc1( int code, WPARAM wparam, LPARAM lparam )
{
    MSLLHOOKSTRUCT *hook = (MSLLHOOKSTRUCT *)lparam;
    POINT pt, pt1;

    if (code == HC_ACTION)
    {
        /* This is our new cursor position */
        pt_new = hook->pt;
        /* Should return previous position */
        GetCursorPos(&pt);
        ok(pt.x == pt_old.x && pt.y == pt_old.y, "GetCursorPos: (%d,%d)\n", pt.x, pt.y);

        /* Should set new position until hook chain is finished. */
        pt.x = pt_old.x + STEP;
        pt.y = pt_old.y + STEP;
        SetCursorPos(pt.x, pt.y);
        GetCursorPos(&pt1);
        if (clipped)
            ok(pt1.x == pt_old.x && pt1.y == pt_old.y, "Wrong set pos: (%d,%d)\n", pt1.x, pt1.y);
        else
            ok(pt1.x == pt.x && pt1.y == pt.y, "Wrong set pos: (%d,%d)\n", pt1.x, pt1.y);
    }
    return CallNextHookEx( 0, code, wparam, lparam );
}

static LRESULT CALLBACK hook_proc2( int code, WPARAM wparam, LPARAM lparam )
{
    MSLLHOOKSTRUCT *hook = (MSLLHOOKSTRUCT *)lparam;
    POINT pt;

    if (code == HC_ACTION)
    {
        ok(hook->pt.x == pt_new.x && hook->pt.y == pt_new.y,
           "Wrong hook coords: (%d %d) != (%d,%d)\n", hook->pt.x, hook->pt.y, pt_new.x, pt_new.y);

        /* Should match position set above */
        GetCursorPos(&pt);
        if (clipped)
            ok(pt.x == pt_old.x && pt.y == pt_old.y, "GetCursorPos: (%d,%d)\n", pt.x, pt.y);
        else
            ok(pt.x == pt_old.x +STEP && pt.y == pt_old.y +STEP, "GetCursorPos: (%d,%d)\n", pt.x, pt.y);
    }
    return CallNextHookEx( 0, code, wparam, lparam );
}

static LRESULT CALLBACK hook_proc3( int code, WPARAM wparam, LPARAM lparam )
{
    POINT pt;

    if (code == HC_ACTION)
    {
        /* MSLLHOOKSTRUCT does not seem to be reliable and contains different data on each run. */
        GetCursorPos(&pt);
        ok(pt.x == pt_old.x && pt.y == pt_old.y, "GetCursorPos: (%d,%d)\n", pt.x, pt.y);
    }
    return CallNextHookEx( 0, code, wparam, lparam );
}

static void test_mouse_ll_hook(void)
{
    HWND hwnd;
    HHOOK hook1, hook2;
    POINT pt_org, pt;
    RECT rc;

    GetCursorPos(&pt_org);
    hwnd = CreateWindowA("static", "Title", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        10, 10, 200, 200, NULL, NULL, NULL, NULL);
    SetCursorPos(100, 100);

    if (!(hook2 = SetWindowsHookExA(WH_MOUSE_LL, hook_proc2, GetModuleHandleA(0), 0)))
    {
        win_skip( "cannot set MOUSE_LL hook\n" );
        goto done;
    }
    hook1 = SetWindowsHookExA(WH_MOUSE_LL, hook_proc1, GetModuleHandleA(0), 0);

    GetCursorPos(&pt_old);
    mouse_event(MOUSEEVENTF_MOVE, -STEP,  0, 0, 0);
    GetCursorPos(&pt_old);
    ok(pt_old.x == pt_new.x && pt_old.y == pt_new.y, "Wrong new pos: (%d,%d)\n", pt_old.x, pt_old.y);
    mouse_event(MOUSEEVENTF_MOVE, +STEP,  0, 0, 0);
    GetCursorPos(&pt_old);
    ok(pt_old.x == pt_new.x && pt_old.y == pt_new.y, "Wrong new pos: (%d,%d)\n", pt_old.x, pt_old.y);
    mouse_event(MOUSEEVENTF_MOVE,  0, -STEP, 0, 0);
    GetCursorPos(&pt_old);
    ok(pt_old.x == pt_new.x && pt_old.y == pt_new.y, "Wrong new pos: (%d,%d)\n", pt_old.x, pt_old.y);
    mouse_event(MOUSEEVENTF_MOVE,  0, +STEP, 0, 0);
    GetCursorPos(&pt_old);
    ok(pt_old.x == pt_new.x && pt_old.y == pt_new.y, "Wrong new pos: (%d,%d)\n", pt_old.x, pt_old.y);

    SetRect(&rc, 50, 50, 151, 151);
    ClipCursor(&rc);
    clipped = TRUE;

    SetCursorPos(40, 40);
    GetCursorPos(&pt_old);
    ok(pt_old.x == 50 && pt_old.y == 50, "Wrong new pos: (%d,%d)\n", pt_new.x, pt_new.y);
    SetCursorPos(160, 160);
    GetCursorPos(&pt_old);
    ok(pt_old.x == 150 && pt_old.y == 150, "Wrong new pos: (%d,%d)\n", pt_new.x, pt_new.y);
    mouse_event(MOUSEEVENTF_MOVE, +STEP, +STEP, 0, 0);
    GetCursorPos(&pt_old);
    ok(pt_old.x == 150 && pt_old.y == 150, "Wrong new pos: (%d,%d)\n", pt_new.x, pt_new.y);

    clipped = FALSE;
    pt_new.x = pt_new.y = 150;
    ClipCursor(NULL);
    UnhookWindowsHookEx(hook1);

    /* Now check that mouse buttons do not change mouse position
       if we don't have MOUSEEVENTF_MOVE flag specified. */

    /* We reusing the same hook callback, so make it happy */
    pt_old.x = pt_new.x - STEP;
    pt_old.y = pt_new.y - STEP;
    mouse_event(MOUSEEVENTF_LEFTUP, 123, 456, 0, 0);
    GetCursorPos(&pt);
    ok(pt.x == pt_new.x && pt.y == pt_new.y, "Position changed: (%d,%d)\n", pt.x, pt.y);
    mouse_event(MOUSEEVENTF_RIGHTUP, 456, 123, 0, 0);
    GetCursorPos(&pt);
    ok(pt.x == pt_new.x && pt.y == pt_new.y, "Position changed: (%d,%d)\n", pt.x, pt.y);

    mouse_event(MOUSEEVENTF_LEFTUP | MOUSEEVENTF_ABSOLUTE, 123, 456, 0, 0);
    GetCursorPos(&pt);
    ok(pt.x == pt_new.x && pt.y == pt_new.y, "Position changed: (%d,%d)\n", pt.x, pt.y);
    mouse_event(MOUSEEVENTF_RIGHTUP | MOUSEEVENTF_ABSOLUTE, 456, 123, 0, 0);
    GetCursorPos(&pt);
    ok(pt.x == pt_new.x && pt.y == pt_new.y, "Position changed: (%d,%d)\n", pt.x, pt.y);

    UnhookWindowsHookEx(hook2);
    hook1 = SetWindowsHookExA(WH_MOUSE_LL, hook_proc3, GetModuleHandleA(0), 0);

    SetRect(&rc, 150, 150, 150, 150);
    ClipCursor(&rc);
    clipped = TRUE;

    SetCursorPos(140, 140);
    GetCursorPos(&pt_old);
    ok(pt_old.x == 150 && pt_old.y == 150, "Wrong new pos: (%d,%d)\n", pt_old.x, pt_old.y);
    SetCursorPos(160, 160);
    GetCursorPos(&pt_old);
    todo_wine
    ok(pt_old.x == 149 && pt_old.y == 149, "Wrong new pos: (%d,%d)\n", pt_old.x, pt_old.y);
    mouse_event(MOUSEEVENTF_MOVE, -STEP, -STEP, 0, 0);
    GetCursorPos(&pt_old);
    ok(pt_old.x == 150 && pt_old.y == 150, "Wrong new pos: (%d,%d)\n", pt_old.x, pt_old.y);
    mouse_event(MOUSEEVENTF_MOVE, +STEP, +STEP, 0, 0);
    GetCursorPos(&pt_old);
    todo_wine
    ok(pt_old.x == 149 && pt_old.y == 149, "Wrong new pos: (%d,%d)\n", pt_old.x, pt_old.y);
    mouse_event(MOUSEEVENTF_MOVE, 0, 0, 0, 0);
    GetCursorPos(&pt_old);
    ok(pt_old.x == 150 && pt_old.y == 150, "Wrong new pos: (%d,%d)\n", pt_old.x, pt_old.y);
    mouse_event(MOUSEEVENTF_MOVE, 0, 0, 0, 0);
    GetCursorPos(&pt_old);
    todo_wine
    ok(pt_old.x == 149 && pt_old.y == 149, "Wrong new pos: (%d,%d)\n", pt_old.x, pt_old.y);

    clipped = FALSE;
    ClipCursor(NULL);

    SetCursorPos(140, 140);
    SetRect(&rc, 150, 150, 150, 150);
    ClipCursor(&rc);
    GetCursorPos(&pt_old);
    ok(pt_old.x == 150 && pt_old.y == 150, "Wrong new pos: (%d,%d)\n", pt_old.x, pt_old.y);
    ClipCursor(NULL);

    SetCursorPos(160, 160);
    SetRect(&rc, 150, 150, 150, 150);
    ClipCursor(&rc);
    GetCursorPos(&pt_old);
    todo_wine
    ok(pt_old.x == 149 && pt_old.y == 149, "Wrong new pos: (%d,%d)\n", pt_old.x, pt_old.y);
    ClipCursor(NULL);

    SetCursorPos(150, 150);
    SetRect(&rc, 150, 150, 150, 150);
    ClipCursor(&rc);
    GetCursorPos(&pt_old);
    todo_wine
    ok(pt_old.x == 149 && pt_old.y == 149, "Wrong new pos: (%d,%d)\n", pt_old.x, pt_old.y);
    ClipCursor(NULL);

    UnhookWindowsHookEx(hook1);

done:
    DestroyWindow(hwnd);
    SetCursorPos(pt_org.x, pt_org.y);
}

static void test_GetMouseMovePointsEx(void)
{
#define BUFLIM  64
#define MYERROR 0xdeadbeef
    int count, retval;
    MOUSEMOVEPOINT in;
    MOUSEMOVEPOINT out[200];
    POINT point;

    /* Get a valid content for the input struct */
    if(!GetCursorPos(&point)) {
        win_skip("GetCursorPos() failed with error %u\n", GetLastError());
        return;
    }
    memset(&in, 0, sizeof(MOUSEMOVEPOINT));
    in.x = point.x;
    in.y = point.y;

    /* test first parameter
     * everything different than sizeof(MOUSEMOVEPOINT)
     * is expected to fail with ERROR_INVALID_PARAMETER
     */
    SetLastError(MYERROR);
    retval = pGetMouseMovePointsEx(0, &in, out, BUFLIM, GMMP_USE_DISPLAY_POINTS);
    if (retval == ERROR_INVALID_PARAMETER)
    {
        win_skip( "GetMouseMovePointsEx broken on WinME\n" );
        return;
    }
    ok(retval == -1, "expected GetMouseMovePointsEx to fail, got %d\n", retval);
    ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == MYERROR,
       "expected error ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    SetLastError(MYERROR);
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT)-1, &in, out, BUFLIM, GMMP_USE_DISPLAY_POINTS);
    ok(retval == -1, "expected GetMouseMovePointsEx to fail, got %d\n", retval);
    ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == MYERROR,
       "expected error ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    SetLastError(MYERROR);
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT)+1, &in, out, BUFLIM, GMMP_USE_DISPLAY_POINTS);
    ok(retval == -1, "expected GetMouseMovePointsEx to fail, got %d\n", retval);
    ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == MYERROR,
       "expected error ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    /* test second and third parameter
     */
    SetLastError(MYERROR);
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT), NULL, out, BUFLIM, GMMP_USE_DISPLAY_POINTS);
    ok(retval == -1, "expected GetMouseMovePointsEx to fail, got %d\n", retval);
    ok(GetLastError() == ERROR_NOACCESS || GetLastError() == MYERROR,
       "expected error ERROR_NOACCESS, got %u\n", GetLastError());

    SetLastError(MYERROR);
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT), &in, NULL, BUFLIM, GMMP_USE_DISPLAY_POINTS);
    ok(retval == -1, "expected GetMouseMovePointsEx to fail, got %d\n", retval);
    ok(ERROR_NOACCESS == GetLastError(),
       "expected error ERROR_NOACCESS, got %u\n", GetLastError());

    SetLastError(MYERROR);
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT), NULL, NULL, BUFLIM, GMMP_USE_DISPLAY_POINTS);
    ok(retval == -1, "expected GetMouseMovePointsEx to fail, got %d\n", retval);
    ok(ERROR_NOACCESS == GetLastError(),
       "expected error ERROR_NOACCESS, got %u\n", GetLastError());

    SetLastError(MYERROR);
    count = 0;
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT), &in, NULL, count, GMMP_USE_DISPLAY_POINTS);
    if (retval == -1)
        ok(GetLastError() == ERROR_POINT_NOT_FOUND, "unexpected error %u\n", GetLastError());
    else
        ok(retval == count, "expected GetMouseMovePointsEx to succeed, got %d\n", retval);

    /* test fourth parameter
     * a value higher than 64 is expected to fail with ERROR_INVALID_PARAMETER
     */
    SetLastError(MYERROR);
    count = -1;
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT), &in, out, count, GMMP_USE_DISPLAY_POINTS);
    ok(retval == count, "expected GetMouseMovePointsEx to fail, got %d\n", retval);
    ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == MYERROR,
       "expected error ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    SetLastError(MYERROR);
    count = 0;
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT), &in, out, count, GMMP_USE_DISPLAY_POINTS);
    if (retval == -1)
        ok(GetLastError() == ERROR_POINT_NOT_FOUND, "unexpected error %u\n", GetLastError());
    else
        ok(retval == count, "expected GetMouseMovePointsEx to succeed, got %d\n", retval);

    SetLastError(MYERROR);
    count = BUFLIM;
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT), &in, out, count, GMMP_USE_DISPLAY_POINTS);
    if (retval == -1)
        ok(GetLastError() == ERROR_POINT_NOT_FOUND, "unexpected error %u\n", GetLastError());
    else
        ok((0 <= retval) && (retval <= count), "expected GetMouseMovePointsEx to succeed, got %d\n", retval);

    SetLastError(MYERROR);
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT), &in, out, BUFLIM+1, GMMP_USE_DISPLAY_POINTS);
    ok(retval == -1, "expected GetMouseMovePointsEx to fail, got %d\n", retval);
    ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == MYERROR,
       "expected error ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    /* it was not possible to force an error with the fifth parameter on win2k */

    /* test combinations of wrong parameters to see which error wins */
    SetLastError(MYERROR);
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT)-1, NULL, out, BUFLIM, GMMP_USE_DISPLAY_POINTS);
    ok(retval == -1, "expected GetMouseMovePointsEx to fail, got %d\n", retval);
    ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == MYERROR,
       "expected error ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    SetLastError(MYERROR);
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT)-1, &in, NULL, BUFLIM, GMMP_USE_DISPLAY_POINTS);
    ok(retval == -1, "expected GetMouseMovePointsEx to fail, got %d\n", retval);
    ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == MYERROR,
       "expected error ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    SetLastError(MYERROR);
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT), NULL, out, BUFLIM+1, GMMP_USE_DISPLAY_POINTS);
    ok(retval == -1, "expected GetMouseMovePointsEx to fail, got %d\n", retval);
    ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == MYERROR,
       "expected error ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

    SetLastError(MYERROR);
    retval = pGetMouseMovePointsEx(sizeof(MOUSEMOVEPOINT), &in, NULL, BUFLIM+1, GMMP_USE_DISPLAY_POINTS);
    ok(retval == -1, "expected GetMouseMovePointsEx to fail, got %d\n", retval);
    ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == MYERROR,
       "expected error ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

#undef BUFLIM
#undef MYERROR
}

static void test_GetRawInputDeviceList(void)
{
    RAWINPUTDEVICELIST devices[32];
    UINT ret, oret, devcount, odevcount, i;
    DWORD err;

    SetLastError(0xdeadbeef);
    ret = pGetRawInputDeviceList(NULL, NULL, 0);
    err = GetLastError();
    ok(ret == -1, "expected -1, got %d\n", ret);
    ok(err == ERROR_INVALID_PARAMETER, "expected 87, got %d\n", err);

    SetLastError(0xdeadbeef);
    ret = pGetRawInputDeviceList(NULL, NULL, sizeof(devices[0]));
    err = GetLastError();
    ok(ret == -1, "expected -1, got %d\n", ret);
    ok(err == ERROR_NOACCESS, "expected 998, got %d\n", err);

    devcount = 0;
    ret = pGetRawInputDeviceList(NULL, &devcount, sizeof(devices[0]));
    ok(ret == 0, "expected 0, got %d\n", ret);
    ok(devcount > 0, "expected non-zero\n");

    SetLastError(0xdeadbeef);
    devcount = 0;
    ret = pGetRawInputDeviceList(devices, &devcount, sizeof(devices[0]));
    err = GetLastError();
    ok(ret == -1, "expected -1, got %d\n", ret);
    ok(err == ERROR_INSUFFICIENT_BUFFER, "expected 122, got %d\n", err);
    ok(devcount > 0, "expected non-zero\n");

    /* devcount contains now the correct number of devices */
    ret = pGetRawInputDeviceList(devices, &devcount, sizeof(devices[0]));
    ok(ret > 0, "expected non-zero\n");

    for(i = 0; i < devcount; ++i)
    {
        WCHAR name[128];
        char nameA[128];
        UINT sz, len;
        RID_DEVICE_INFO info;
        HANDLE file;

        /* get required buffer size */
        name[0] = '\0';
        sz = 5;
        ret = pGetRawInputDeviceInfoW(devices[i].hDevice, RIDI_DEVICENAME, name, &sz);
        ok(ret == -1, "GetRawInputDeviceInfo gave wrong failure: %d\n", err);
        ok(sz > 5 && sz < ARRAY_SIZE(name), "Size should have been set and not too large (got: %u)\n", sz);

        /* buffer size for RIDI_DEVICENAME is in CHARs, not BYTEs */
        ret = pGetRawInputDeviceInfoW(devices[i].hDevice, RIDI_DEVICENAME, name, &sz);
        ok(ret == sz, "GetRawInputDeviceInfo gave wrong return: %d\n", err);
        len = lstrlenW(name);
        ok(len + 1 == ret, "GetRawInputDeviceInfo returned wrong length (name: %u, ret: %u)\n", len + 1, ret);

        /* test A variant with same size */
        ret = pGetRawInputDeviceInfoA(devices[i].hDevice, RIDI_DEVICENAME, nameA, &sz);
        ok(ret == sz, "GetRawInputDeviceInfoA gave wrong return: %d\n", err);
        len = strlen(nameA);
        ok(len + 1 == ret, "GetRawInputDeviceInfoA returned wrong length (name: %u, ret: %u)\n", len + 1, ret);

        /* buffer size for RIDI_DEVICEINFO is in BYTEs */
        memset(&info, 0, sizeof(info));
        info.cbSize = sizeof(info);
        sz = sizeof(info) - 1;
        ret = pGetRawInputDeviceInfoW(devices[i].hDevice, RIDI_DEVICEINFO, &info, &sz);
        ok(ret == -1, "GetRawInputDeviceInfo gave wrong failure: %d\n", err);
        ok(sz == sizeof(info), "GetRawInputDeviceInfo set wrong size\n");

        ret = pGetRawInputDeviceInfoW(devices[i].hDevice, RIDI_DEVICEINFO, &info, &sz);
        ok(ret == sizeof(info), "GetRawInputDeviceInfo gave wrong return: %d\n", err);
        ok(sz == sizeof(info), "GetRawInputDeviceInfo set wrong size\n");
        ok(info.dwType == devices[i].dwType, "GetRawInputDeviceInfo set wrong type: 0x%x\n", info.dwType);

        memset(&info, 0, sizeof(info));
        info.cbSize = sizeof(info);
        ret = pGetRawInputDeviceInfoA(devices[i].hDevice, RIDI_DEVICEINFO, &info, &sz);
        ok(ret == sizeof(info), "GetRawInputDeviceInfo gave wrong return: %d\n", err);
        ok(sz == sizeof(info), "GetRawInputDeviceInfo set wrong size\n");
        ok(info.dwType == devices[i].dwType, "GetRawInputDeviceInfo set wrong type: 0x%x\n", info.dwType);

        /* setupapi returns an NT device path, but CreateFile() < Vista can't
         * understand that; so use the \\?\ prefix instead */
        name[1] = '\\';
        file = CreateFileW(name, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        todo_wine_if(info.dwType != RIM_TYPEHID)
            ok(file != INVALID_HANDLE_VALUE, "Failed to open %s, error %u\n", wine_dbgstr_w(name), GetLastError());
        CloseHandle(file);
    }

    /* check if variable changes from larger to smaller value */
    devcount = odevcount = ARRAY_SIZE(devices);
    oret = ret = pGetRawInputDeviceList(devices, &odevcount, sizeof(devices[0]));
    ok(ret > 0, "expected non-zero\n");
    ok(devcount == odevcount, "expected %d, got %d\n", devcount, odevcount);
    devcount = odevcount;
    odevcount = ARRAY_SIZE(devices);
    ret = pGetRawInputDeviceList(NULL, &odevcount, sizeof(devices[0]));
    ok(ret == 0, "expected 0, got %d\n", ret);
    ok(odevcount == oret, "expected %d, got %d\n", oret, odevcount);
}

static void test_GetRawInputData(void)
{
    UINT size;
    UINT ret;

    /* Null raw input handle */
    ret = GetRawInputData(NULL, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));
    ok(ret == ~0U, "Expect ret %u, got %u\n", ~0U, ret);
}

static void test_key_map(void)
{
    HKL kl = GetKeyboardLayout(0);
    UINT kL, kR, s, sL;
    int i;
    static const UINT numpad_collisions[][2] = {
        { VK_NUMPAD0, VK_INSERT },
        { VK_NUMPAD1, VK_END },
        { VK_NUMPAD2, VK_DOWN },
        { VK_NUMPAD3, VK_NEXT },
        { VK_NUMPAD4, VK_LEFT },
        { VK_NUMPAD6, VK_RIGHT },
        { VK_NUMPAD7, VK_HOME },
        { VK_NUMPAD8, VK_UP },
        { VK_NUMPAD9, VK_PRIOR },
    };

    s  = MapVirtualKeyExA(VK_SHIFT,  MAPVK_VK_TO_VSC, kl);
    ok(s != 0, "MapVirtualKeyEx(VK_SHIFT) should return non-zero\n");
    sL = MapVirtualKeyExA(VK_LSHIFT, MAPVK_VK_TO_VSC, kl);
    ok(s == sL || broken(sL == 0), /* win9x */
       "%x != %x\n", s, sL);

    kL = MapVirtualKeyExA(0x2a, MAPVK_VSC_TO_VK, kl);
    ok(kL == VK_SHIFT, "Scan code -> vKey = %x (not VK_SHIFT)\n", kL);
    kR = MapVirtualKeyExA(0x36, MAPVK_VSC_TO_VK, kl);
    ok(kR == VK_SHIFT, "Scan code -> vKey = %x (not VK_SHIFT)\n", kR);

    kL = MapVirtualKeyExA(0x2a, MAPVK_VSC_TO_VK_EX, kl);
    ok(kL == VK_LSHIFT || broken(kL == 0), /* win9x */
       "Scan code -> vKey = %x (not VK_LSHIFT)\n", kL);
    kR = MapVirtualKeyExA(0x36, MAPVK_VSC_TO_VK_EX, kl);
    ok(kR == VK_RSHIFT || broken(kR == 0), /* win9x */
       "Scan code -> vKey = %x (not VK_RSHIFT)\n", kR);

    /* test that MAPVK_VSC_TO_VK prefers the non-numpad vkey if there's ambiguity */
    for (i = 0; i < ARRAY_SIZE(numpad_collisions); i++)
    {
        UINT numpad_scan = MapVirtualKeyExA(numpad_collisions[i][0],  MAPVK_VK_TO_VSC, kl);
        UINT other_scan  = MapVirtualKeyExA(numpad_collisions[i][1],  MAPVK_VK_TO_VSC, kl);

        /* do they really collide for this layout? */
        if (numpad_scan && other_scan == numpad_scan)
        {
            UINT vkey = MapVirtualKeyExA(numpad_scan, MAPVK_VSC_TO_VK, kl);
            ok(vkey != numpad_collisions[i][0],
               "Got numpad vKey %x for scan code %x when there was another choice\n",
               vkey, numpad_scan);
        }
    }
}

#define shift 1
#define ctrl  2

static const struct tounicode_tests
{
    UINT vk;
    DWORD modifiers;
    WCHAR chr; /* if vk is 0, lookup vk using this char */
    int expect_ret;
    WCHAR expect_buf[4];
} utests[] =
{
    { 'A', 0, 0, 1, {'a',0}},
    { 'A', ctrl, 0, 1, {1, 0}},
    { 'A', shift|ctrl, 0, 1, {1, 0}},
    { VK_TAB, ctrl, 0, 0, {0}},
    { VK_TAB, shift|ctrl, 0, 0, {0}},
    { VK_RETURN, ctrl, 0, 1, {'\n', 0}},
    { VK_RETURN, shift|ctrl, 0, 0, {0}},
    { '4', ctrl, 0, 0, {0}},
    { '4', shift|ctrl, 0, 0, {0}},
    { 0, ctrl, '!', 0, {0}},
    { 0, ctrl, '\"', 0, {0}},
    { 0, ctrl, '#', 0, {0}},
    { 0, ctrl, '$', 0, {0}},
    { 0, ctrl, '%', 0, {0}},
    { 0, ctrl, '\'', 0, {0}},
    { 0, ctrl, '(', 0, {0}},
    { 0, ctrl, ')', 0, {0}},
    { 0, ctrl, '*', 0, {0}},
    { 0, ctrl, '+', 0, {0}},
    { 0, ctrl, ',', 0, {0}},
    { 0, ctrl, '-', 0, {0}},
    { 0, ctrl, '.', 0, {0}},
    { 0, ctrl, '/', 0, {0}},
    { 0, ctrl, ':', 0, {0}},
    { 0, ctrl, ';', 0, {0}},
    { 0, ctrl, '<', 0, {0}},
    { 0, ctrl, '=', 0, {0}},
    { 0, ctrl, '>', 0, {0}},
    { 0, ctrl, '?', 0, {0}},
    { 0, ctrl, '@', 1, {0}},
    { 0, ctrl, '[', 1, {0x1b}},
    { 0, ctrl, '\\', 1, {0x1c}},
    { 0, ctrl, ']', 1, {0x1d}},
    { 0, ctrl, '^', 1, {0x1e}},
    { 0, ctrl, '_', 1, {0x1f}},
    { 0, ctrl, '`', 0, {0}},
    { VK_SPACE, 0, 0, 1, {' ',0}},
    { VK_SPACE, shift, 0, 1, {' ',0}},
    { VK_SPACE, ctrl, 0, 1, {' ',0}},
};

static void test_ToUnicode(void)
{
    WCHAR wStr[4];
    BYTE state[256];
    const BYTE SC_RETURN = 0x1c, SC_TAB = 0x0f, SC_A = 0x1e;
    const BYTE HIGHEST_BIT = 0x80;
    int i, ret;
    BOOL us_kbd = (GetKeyboardLayout(0) == (HKL)(ULONG_PTR)0x04090409);

    for(i=0; i<256; i++)
        state[i]=0;

    wStr[1] = 0xAA;
    SetLastError(0xdeadbeef);
    ret = ToUnicode(VK_RETURN, SC_RETURN, state, wStr, 4, 0);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("ToUnicode is not implemented\n");
        return;
    }

    ok(ret == 1, "ToUnicode for Return key didn't return 1 (was %i)\n", ret);
    if(ret == 1)
    {
        ok(wStr[0]=='\r', "ToUnicode for CTRL + Return was %i (expected 13)\n", wStr[0]);
        ok(wStr[1]==0 || broken(wStr[1]!=0) /* nt4 */,
           "ToUnicode didn't null-terminate the buffer when there was room.\n");
    }

    for (i = 0; i < ARRAY_SIZE(utests); i++)
    {
        UINT vk = utests[i].vk, mod = utests[i].modifiers, scan;

        if(!vk)
        {
            short vk_ret;

            if (!us_kbd) continue;
            vk_ret = VkKeyScanW(utests[i].chr);
            if (vk_ret == -1) continue;
            vk = vk_ret & 0xff;
            if (vk_ret & 0x100) mod |= shift;
            if (vk_ret & 0x200) mod |= ctrl;
        }
        scan = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);

        state[VK_SHIFT]   = state[VK_LSHIFT]   = (mod & shift) ? HIGHEST_BIT : 0;
        state[VK_CONTROL] = state[VK_LCONTROL] = (mod & ctrl) ? HIGHEST_BIT : 0;

        ret = ToUnicode(vk, scan, state, wStr, 4, 0);
        ok(ret == utests[i].expect_ret, "%d: got %d expected %d\n", i, ret, utests[i].expect_ret);
        if (ret)
            ok(!lstrcmpW(wStr, utests[i].expect_buf), "%d: got %s expected %s\n", i, wine_dbgstr_w(wStr),
                wine_dbgstr_w(utests[i].expect_buf));

    }
    state[VK_SHIFT]   = state[VK_LSHIFT]   = 0;
    state[VK_CONTROL] = state[VK_LCONTROL] = 0;

    ret = ToUnicode(VK_TAB, SC_TAB, NULL, wStr, 4, 0);
    ok(ret == 0, "ToUnicode with NULL keystate didn't return 0 (was %i)\n", ret);
    ret = ToUnicode(VK_RETURN, SC_RETURN, NULL, wStr, 4, 0);
    ok(ret == 0, "ToUnicode with NULL keystate didn't return 0 (was %i)\n", ret);
    ret = ToUnicode('A', SC_A, NULL, wStr, 4, 0);
    ok(ret == 0, "ToUnicode with NULL keystate didn't return 0 (was %i)\n", ret);
    ret = ToUnicodeEx(VK_TAB, SC_TAB, NULL, wStr, 4, 0, GetKeyboardLayout(0));
    ok(ret == 0, "ToUnicodeEx with NULL keystate didn't return 0 (was %i)\n", ret);
    ret = ToUnicodeEx(VK_RETURN, SC_RETURN, NULL, wStr, 4, 0, GetKeyboardLayout(0));
    ok(ret == 0, "ToUnicodeEx with NULL keystate didn't return 0 (was %i)\n", ret);
    ret = ToUnicodeEx('A', SC_A, NULL, wStr, 4, 0, GetKeyboardLayout(0));
    ok(ret == 0, "ToUnicodeEx with NULL keystate didn't return 0 (was %i)\n", ret);
}

static void test_ToAscii(void)
{
    WORD character;
    BYTE state[256];
    const BYTE SC_RETURN = 0x1c, SC_A = 0x1e;
    const BYTE HIGHEST_BIT = 0x80;
    int ret;

    memset(state, 0, sizeof(state));

    character = 0;
    ret = ToAscii(VK_RETURN, SC_RETURN, state, &character, 0);
    ok(ret == 1, "ToAscii for Return key didn't return 1 (was %i)\n", ret);
    ok(character == '\r', "ToAscii for Return was %i (expected 13)\n", character);

    character = 0;
    ret = ToAscii('A', SC_A, state, &character, 0);
    ok(ret == 1, "ToAscii for character 'A' didn't return 1 (was %i)\n", ret);
    ok(character == 'a', "ToAscii for character 'A' was %i (expected %i)\n", character, 'a');

    state[VK_CONTROL] |= HIGHEST_BIT;
    state[VK_LCONTROL] |= HIGHEST_BIT;
    character = 0;
    ret = ToAscii(VK_RETURN, SC_RETURN, state, &character, 0);
    ok(ret == 1, "ToAscii for CTRL + Return key didn't return 1 (was %i)\n", ret);
    ok(character == '\n', "ToAscii for CTRL + Return was %i (expected 10)\n", character);

    character = 0;
    ret = ToAscii('A', SC_A, state, &character, 0);
    ok(ret == 1, "ToAscii for CTRL + character 'A' didn't return 1 (was %i)\n", ret);
    ok(character == 1, "ToAscii for CTRL + character 'A' was %i (expected 1)\n", character);

    state[VK_SHIFT] |= HIGHEST_BIT;
    state[VK_LSHIFT] |= HIGHEST_BIT;
    ret = ToAscii(VK_RETURN, SC_RETURN, state, &character, 0);
    ok(ret == 0, "ToAscii for CTRL + Shift + Return key didn't return 0 (was %i)\n", ret);

    ret = ToAscii(VK_RETURN, SC_RETURN, NULL, &character, 0);
    ok(ret == 0, "ToAscii for NULL keystate didn't return 0 (was %i)\n", ret);
    ret = ToAscii('A', SC_A, NULL, &character, 0);
    ok(ret == 0, "ToAscii for NULL keystate didn't return 0 (was %i)\n", ret);
    ret = ToAsciiEx(VK_RETURN, SC_RETURN, NULL, &character, 0, GetKeyboardLayout(0));
    ok(ret == 0, "ToAsciiEx for NULL keystate didn't return 0 (was %i)\n", ret);
    ret = ToAsciiEx('A', SC_A, NULL, &character, 0, GetKeyboardLayout(0));
    ok(ret == 0, "ToAsciiEx for NULL keystate didn't return 0 (was %i)\n", ret);
}

static void test_get_async_key_state(void)
{
    /* input value sanity checks */
    ok(0 == GetAsyncKeyState(1000000), "GetAsyncKeyState did not return 0\n");
    ok(0 == GetAsyncKeyState(-1000000), "GetAsyncKeyState did not return 0\n");
}

static void test_keyboard_layout_name(void)
{
    BOOL ret;
    char klid[KL_NAMELENGTH];

    if (0) /* crashes on native system */
        ret = GetKeyboardLayoutNameA(NULL);

    SetLastError(0xdeadbeef);
    ret = GetKeyboardLayoutNameW(NULL);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == ERROR_NOACCESS, "got %d\n", GetLastError());

    if (GetKeyboardLayout(0) != (HKL)(ULONG_PTR)0x04090409) return;

    klid[0] = 0;
    ret = GetKeyboardLayoutNameA(klid);
    ok(ret, "GetKeyboardLayoutNameA failed %u\n", GetLastError());
    ok(!strcmp(klid, "00000409"), "expected 00000409, got %s\n", klid);
}

static void test_key_names(void)
{
    char buffer[40];
    WCHAR bufferW[40];
    int ret, prev;
    LONG lparam = 0x1d << 16;

    memset( buffer, 0xcc, sizeof(buffer) );
    ret = GetKeyNameTextA( lparam, buffer, sizeof(buffer) );
    ok( ret > 0, "wrong len %u for '%s'\n", ret, buffer );
    ok( ret == strlen(buffer), "wrong len %u for '%s'\n", ret, buffer );

    memset( buffer, 0xcc, sizeof(buffer) );
    prev = ret;
    ret = GetKeyNameTextA( lparam, buffer, prev );
    ok( ret == prev - 1, "wrong len %u for '%s'\n", ret, buffer );
    ok( ret == strlen(buffer), "wrong len %u for '%s'\n", ret, buffer );

    memset( buffer, 0xcc, sizeof(buffer) );
    ret = GetKeyNameTextA( lparam, buffer, 0 );
    ok( ret == 0, "wrong len %u for '%s'\n", ret, buffer );
    ok( buffer[0] == 0, "wrong string '%s'\n", buffer );

    memset( bufferW, 0xcc, sizeof(bufferW) );
    ret = GetKeyNameTextW( lparam, bufferW, ARRAY_SIZE(bufferW));
    ok( ret > 0, "wrong len %u for %s\n", ret, wine_dbgstr_w(bufferW) );
    ok( ret == lstrlenW(bufferW), "wrong len %u for %s\n", ret, wine_dbgstr_w(bufferW) );

    memset( bufferW, 0xcc, sizeof(bufferW) );
    prev = ret;
    ret = GetKeyNameTextW( lparam, bufferW, prev );
    ok( ret == prev - 1, "wrong len %u for %s\n", ret, wine_dbgstr_w(bufferW) );
    ok( ret == lstrlenW(bufferW), "wrong len %u for %s\n", ret, wine_dbgstr_w(bufferW) );

    memset( bufferW, 0xcc, sizeof(bufferW) );
    ret = GetKeyNameTextW( lparam, bufferW, 0 );
    ok( ret == 0, "wrong len %u for %s\n", ret, wine_dbgstr_w(bufferW) );
    ok( bufferW[0] == 0xcccc, "wrong string %s\n", wine_dbgstr_w(bufferW) );
}

static void simulate_click(BOOL left, int x, int y)
{
    INPUT input[2];
    UINT events_no;

    SetCursorPos(x, y);
    memset(input, 0, sizeof(input));
    input[0].type = INPUT_MOUSE;
    U(input[0]).mi.dx = x;
    U(input[0]).mi.dy = y;
    U(input[0]).mi.dwFlags = left ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_RIGHTDOWN;
    input[1].type = INPUT_MOUSE;
    U(input[1]).mi.dx = x;
    U(input[1]).mi.dy = y;
    U(input[1]).mi.dwFlags = left ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_RIGHTUP;
    events_no = SendInput(2, input, sizeof(input[0]));
    ok(events_no == 2, "SendInput returned %d\n", events_no);
}

static BOOL wait_for_message( MSG *msg )
{
    BOOL ret;

    for (;;)
    {
        ret = PeekMessageA(msg, 0, 0, 0, PM_REMOVE);
        if (ret)
        {
            if (msg->message == WM_PAINT) DispatchMessageA(msg);
            else break;
        }
        else if (MsgWaitForMultipleObjects(0, NULL, FALSE, 100, QS_ALLINPUT) == WAIT_TIMEOUT) break;
    }
    if (!ret) msg->message = 0;
    return ret;
}

static BOOL wait_for_event(HANDLE event, int timeout)
{
    DWORD end_time = GetTickCount() + timeout;
    MSG msg;

    do {
        if(MsgWaitForMultipleObjects(1, &event, FALSE, timeout, QS_ALLINPUT) == WAIT_OBJECT_0)
            return TRUE;
        while(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
            DispatchMessageA(&msg);
        timeout = end_time - GetTickCount();
    }while(timeout > 0);

    return FALSE;
}

static WNDPROC def_static_proc;
static DWORD hittest_no;
static LRESULT WINAPI static_hook_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_NCHITTEST)
    {
        /* break infinite hittest loop */
        if(hittest_no > 50) return HTCLIENT;
        hittest_no++;
    }

    return def_static_proc(hwnd, msg, wp, lp);
}

struct thread_data
{
    HANDLE start_event;
    HANDLE end_event;
    HWND win;
};

static DWORD WINAPI create_static_win(void *arg)
{
    struct thread_data *thread_data = arg;
    HWND win;

    win = CreateWindowA("static", "static", WS_VISIBLE | WS_POPUP,
            100, 100, 100, 100, 0, NULL, NULL, NULL);
    ok(win != 0, "CreateWindow failed\n");
    def_static_proc = (void*)SetWindowLongPtrA(win,
            GWLP_WNDPROC, (LONG_PTR)static_hook_proc);
    thread_data->win = win;

    SetEvent(thread_data->start_event);
    wait_for_event(thread_data->end_event, 5000);
    return 0;
}

static void get_dc_region(RECT *region, HWND hwnd, DWORD flags)
{
    int region_type;
    HRGN hregion;
    HDC hdc;

    hdc = GetDCEx(hwnd, 0, flags);
    ok(hdc != NULL, "GetDCEx failed\n");
    hregion = CreateRectRgn(40, 40, 60, 60);
    ok(hregion != NULL, "CreateRectRgn failed\n");
    GetRandomRgn(hdc, hregion, SYSRGN);
    region_type = GetRgnBox(hregion, region);
    ok(region_type == SIMPLEREGION, "expected SIMPLEREGION, got %d\n", region_type);
    DeleteObject(hregion);
    ReleaseDC(hwnd, hdc);
}

static void test_Input_mouse(void)
{
    BOOL got_button_down, got_button_up;
    HWND hwnd, button_win, static_win;
    struct thread_data thread_data;
    HANDLE thread;
    DWORD thread_id;
    WNDCLASSA wclass;
    POINT pt, pt_org;
    int region_type;
    HRGN hregion;
    RECT region;
    MSG msg;
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = GetCursorPos(NULL);
    ok(!ret, "GetCursorPos succeed\n");
    ok(GetLastError() == 0xdeadbeef || GetLastError() == ERROR_NOACCESS, "error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetCursorPos(&pt_org);
    ok(ret, "GetCursorPos failed\n");
    ok(GetLastError() == 0xdeadbeef, "error %u\n", GetLastError());

    button_win = CreateWindowA("button", "button", WS_VISIBLE | WS_POPUP,
            100, 100, 100, 100, 0, NULL, NULL, NULL);
    ok(button_win != 0, "CreateWindow failed\n");

    pt.x = pt.y = 150;
    hwnd = WindowFromPoint(pt);
    if (hwnd != button_win)
    {
        skip("there's another window covering test window\n");
        DestroyWindow(button_win);
        return;
    }

    /* simple button click test */
    simulate_click(TRUE, 150, 150);
    got_button_down = got_button_up = FALSE;
    while (wait_for_message(&msg))
    {
        DispatchMessageA(&msg);

        if (msg.message == WM_LBUTTONDOWN)
        {
            got_button_down = TRUE;
        }
        else if (msg.message == WM_LBUTTONUP)
        {
            got_button_up = TRUE;
            break;
        }
    }
    ok(got_button_down, "expected WM_LBUTTONDOWN message\n");
    ok(got_button_up, "expected WM_LBUTTONUP message\n");

    /* click through HTTRANSPARENT child window */
    static_win = CreateWindowA("static", "static", WS_VISIBLE | WS_CHILD,
            0, 0, 100, 100, button_win, NULL, NULL, NULL);
    ok(static_win != 0, "CreateWindow failed\n");
    def_static_proc = (void*)SetWindowLongPtrA(static_win,
            GWLP_WNDPROC, (LONG_PTR)static_hook_proc);
    simulate_click(FALSE, 150, 150);
    hittest_no = 0;
    got_button_down = got_button_up = FALSE;
    while (wait_for_message(&msg))
    {
        DispatchMessageA(&msg);

        if (msg.message == WM_RBUTTONDOWN)
        {
            ok(msg.hwnd == button_win, "msg.hwnd = %p\n", msg.hwnd);
            got_button_down = TRUE;
        }
        else if (msg.message == WM_RBUTTONUP)
        {
            ok(msg.hwnd == button_win, "msg.hwnd = %p\n", msg.hwnd);
            got_button_up = TRUE;
            break;
        }
    }
    ok(hittest_no && hittest_no<50, "expected WM_NCHITTEST message\n");
    ok(got_button_down, "expected WM_RBUTTONDOWN message\n");
    ok(got_button_up, "expected WM_RBUTTONUP message\n");
    DestroyWindow(static_win);

    /* click through HTTRANSPARENT top-level window */
    static_win = CreateWindowA("static", "static", WS_VISIBLE | WS_POPUP,
            100, 100, 100, 100, 0, NULL, NULL, NULL);
    ok(static_win != 0, "CreateWindow failed\n");
    def_static_proc = (void*)SetWindowLongPtrA(static_win,
            GWLP_WNDPROC, (LONG_PTR)static_hook_proc);
    simulate_click(TRUE, 150, 150);
    hittest_no = 0;
    got_button_down = got_button_up = FALSE;
    while (wait_for_message(&msg))
    {
        DispatchMessageA(&msg);

        if (msg.message == WM_LBUTTONDOWN)
        {
            ok(msg.hwnd == button_win, "msg.hwnd = %p\n", msg.hwnd);
            got_button_down = TRUE;
        }
        else if (msg.message == WM_LBUTTONUP)
        {
            ok(msg.hwnd == button_win, "msg.hwnd = %p\n", msg.hwnd);
            got_button_up = TRUE;
            break;
        }
    }
    ok(hittest_no && hittest_no<50, "expected WM_NCHITTEST message\n");
    ok(got_button_down, "expected WM_LBUTTONDOWN message\n");
    ok(got_button_up, "expected WM_LBUTTONUP message\n");
    DestroyWindow(static_win);

    /* click on HTTRANSPARENT top-level window that belongs to other thread */
    thread_data.start_event = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(thread_data.start_event != NULL, "CreateEvent failed\n");
    thread_data.end_event = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(thread_data.end_event != NULL, "CreateEvent failed\n");
    thread = CreateThread(NULL, 0, create_static_win, &thread_data, 0, NULL);
    ok(thread != NULL, "CreateThread failed\n");
    hittest_no = 0;
    got_button_down = got_button_up = FALSE;
    WaitForSingleObject(thread_data.start_event, INFINITE);
    simulate_click(FALSE, 150, 150);
    while (wait_for_message(&msg))
    {
        DispatchMessageA(&msg);

        if (msg.message == WM_RBUTTONDOWN)
            got_button_down = TRUE;
        else if (msg.message == WM_RBUTTONUP)
            got_button_up = TRUE;
    }
    SetEvent(thread_data.end_event);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    ok(hittest_no && hittest_no<50, "expected WM_NCHITTEST message\n");
    ok(!got_button_down, "unexpected WM_RBUTTONDOWN message\n");
    ok(!got_button_up, "unexpected WM_RBUTTONUP message\n");

    /* click on HTTRANSPARENT top-level window that belongs to other thread,
     * thread input queues are attached */
    thread = CreateThread(NULL, 0, create_static_win, &thread_data, 0, &thread_id);
    ok(thread != NULL, "CreateThread failed\n");
    hittest_no = 0;
    got_button_down = got_button_up = FALSE;
    WaitForSingleObject(thread_data.start_event, INFINITE);
    ok(AttachThreadInput(thread_id, GetCurrentThreadId(), TRUE),
            "AttachThreadInput failed\n");
    while (wait_for_message(&msg)) DispatchMessageA(&msg);
    SetWindowPos(thread_data.win, button_win, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
    simulate_click(TRUE, 150, 150);
    while (wait_for_message(&msg))
    {
        DispatchMessageA(&msg);

        if (msg.message == WM_LBUTTONDOWN)
            got_button_down = TRUE;
        else if (msg.message == WM_LBUTTONUP)
            got_button_up = TRUE;
    }
    SetEvent(thread_data.end_event);
    WaitForSingleObject(thread, INFINITE);
    todo_wine ok(hittest_no > 50, "expected loop with WM_NCHITTEST messages\n");
    ok(!got_button_down, "unexpected WM_LBUTTONDOWN message\n");
    ok(!got_button_up, "unexpected WM_LBUTTONUP message\n");

    /* click after SetCapture call */
    hwnd = CreateWindowA("button", "button", WS_VISIBLE | WS_POPUP,
            0, 0, 100, 100, 0, NULL, NULL, NULL);
    ok(hwnd != 0, "CreateWindow failed\n");
    SetCapture(button_win);
    got_button_down = got_button_up = FALSE;
    simulate_click(FALSE, 50, 50);
    while (wait_for_message(&msg))
    {
        DispatchMessageA(&msg);

        if (msg.message == WM_RBUTTONDOWN)
        {
            ok(msg.hwnd == button_win, "msg.hwnd = %p\n", msg.hwnd);
            got_button_down = TRUE;
        }
        else if (msg.message == WM_RBUTTONUP)
        {
            ok(msg.hwnd == button_win, "msg.hwnd = %p\n", msg.hwnd);
            got_button_up = TRUE;
            break;
        }
    }
    ok(got_button_down, "expected WM_RBUTTONDOWN message\n");
    ok(got_button_up, "expected WM_RBUTTONUP message\n");
    DestroyWindow(hwnd);

    /* click on child window after SetCapture call */
    hwnd = CreateWindowA("button", "button2", WS_VISIBLE | WS_CHILD,
            0, 0, 100, 100, button_win, NULL, NULL, NULL);
    ok(hwnd != 0, "CreateWindow failed\n");
    got_button_down = got_button_up = FALSE;
    simulate_click(TRUE, 150, 150);
    while (wait_for_message(&msg))
    {
        DispatchMessageA(&msg);

        if (msg.message == WM_LBUTTONDOWN)
        {
            ok(msg.hwnd == button_win, "msg.hwnd = %p\n", msg.hwnd);
            got_button_down = TRUE;
        }
        else if (msg.message == WM_LBUTTONUP)
        {
            ok(msg.hwnd == button_win, "msg.hwnd = %p\n", msg.hwnd);
            got_button_up = TRUE;
            break;
        }
    }
    ok(got_button_down, "expected WM_LBUTTONDOWN message\n");
    ok(got_button_up, "expected WM_LBUTTONUP message\n");
    DestroyWindow(hwnd);
    ok(ReleaseCapture(), "ReleaseCapture failed\n");

    wclass.style         = 0;
    wclass.lpfnWndProc   = WndProc;
    wclass.cbClsExtra    = 0;
    wclass.cbWndExtra    = 0;
    wclass.hInstance     = GetModuleHandleA(NULL);
    wclass.hIcon         = LoadIconA(0, (LPCSTR)IDI_APPLICATION);
    wclass.hCursor       = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    wclass.hbrBackground = CreateSolidBrush(RGB(128, 128, 128));
    wclass.lpszMenuName  = NULL;
    wclass.lpszClassName = "InputLayeredTestClass";
    RegisterClassA( &wclass );

    /* click through layered window with alpha channel / color key */
    hwnd = CreateWindowA(wclass.lpszClassName, "InputLayeredTest",
            WS_VISIBLE | WS_POPUP, 100, 100, 100, 100, button_win, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindowEx failed\n");

    static_win = CreateWindowA("static", "Title", WS_VISIBLE | WS_CHILD,
                          10, 10, 20, 20, hwnd, NULL, NULL, NULL);
    ok(static_win != NULL, "CreateWindowA failed %u\n", GetLastError());

    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetWindowLongA(hwnd, GWL_EXSTYLE, GetWindowLongA(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    ret = SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    ok(ret, "SetLayeredWindowAttributes failed\n");
    while (wait_for_message(&msg)) DispatchMessageA(&msg);
    Sleep(100);

    if (pGetWindowRgnBox)
    {
        region_type = pGetWindowRgnBox(hwnd, &region);
        ok(region_type == ERROR, "expected ERROR, got %d\n", region_type);
    }

    get_dc_region(&region, hwnd, DCX_PARENTCLIP);
    ok(region.left == 100 && region.top == 100 && region.right == 200 && region.bottom == 200,
       "expected region (100,100)-(200,200), got %s\n", wine_dbgstr_rect(&region));
    get_dc_region(&region, hwnd, DCX_WINDOW | DCX_USESTYLE);
    ok(region.left == 100 && region.top == 100 && region.right == 200 && region.bottom == 200,
       "expected region (100,100)-(200,200), got %s\n", wine_dbgstr_rect(&region));
    get_dc_region(&region, hwnd, DCX_USESTYLE);
    ok(region.left == 100 && region.top == 100 && region.right == 200 && region.bottom == 200,
       "expected region (100,100)-(200,200), got %s\n", wine_dbgstr_rect(&region));
    get_dc_region(&region, static_win, DCX_PARENTCLIP);
    ok(region.left == 100 && region.top == 100 && region.right == 200 && region.bottom == 200,
       "expected region (100,100)-(200,200), got %s\n", wine_dbgstr_rect(&region));
    get_dc_region(&region, static_win, DCX_WINDOW | DCX_USESTYLE);
    ok(region.left == 110 && region.top == 110 && region.right == 130 && region.bottom == 130,
       "expected region (110,110)-(130,130), got %s\n", wine_dbgstr_rect(&region));
    get_dc_region(&region, static_win, DCX_USESTYLE);
    ok(region.left == 100 && region.top == 100 && region.right == 200 && region.bottom == 200,
       "expected region (100,100)-(200,200), got %s\n", wine_dbgstr_rect(&region));

    got_button_down = got_button_up = FALSE;
    simulate_click(TRUE, 150, 150);
    while (wait_for_message(&msg))
    {
        DispatchMessageA(&msg);

        if (msg.message == WM_LBUTTONDOWN)
        {
            ok(msg.hwnd == hwnd, "msg.hwnd = %p\n", msg.hwnd);
            got_button_down = TRUE;
        }
        else if (msg.message == WM_LBUTTONUP)
        {
            ok(msg.hwnd == hwnd, "msg.hwnd = %p\n", msg.hwnd);
            got_button_up = TRUE;
            break;
        }
    }
    ok(got_button_down, "expected WM_LBUTTONDOWN message\n");
    ok(got_button_up, "expected WM_LBUTTONUP message\n");

    ret = SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
    ok(ret, "SetLayeredWindowAttributes failed\n");
    while (wait_for_message(&msg)) DispatchMessageA(&msg);
    Sleep(100);

    if (pGetWindowRgnBox)
    {
        region_type = pGetWindowRgnBox(hwnd, &region);
        ok(region_type == ERROR, "expected ERROR, got %d\n", region_type);
    }

    got_button_down = got_button_up = FALSE;
    simulate_click(TRUE, 150, 150);
    while (wait_for_message(&msg))
    {
        DispatchMessageA(&msg);

        if (msg.message == WM_LBUTTONDOWN)
        {
            ok(msg.hwnd == button_win, "msg.hwnd = %p\n", msg.hwnd);
            got_button_down = TRUE;
        }
        else if (msg.message == WM_LBUTTONUP)
        {
            ok(msg.hwnd == button_win, "msg.hwnd = %p\n", msg.hwnd);
            got_button_up = TRUE;
            break;
        }
    }
    ok(got_button_down || broken(!got_button_down), "expected WM_LBUTTONDOWN message\n");
    ok(got_button_up, "expected WM_LBUTTONUP message\n");

    ret = SetLayeredWindowAttributes(hwnd, RGB(0, 255, 0), 255, LWA_ALPHA | LWA_COLORKEY);
    ok(ret, "SetLayeredWindowAttributes failed\n");
    while (wait_for_message(&msg)) DispatchMessageA(&msg);
    Sleep(100);

    if (pGetWindowRgnBox)
    {
        region_type = pGetWindowRgnBox(hwnd, &region);
        ok(region_type == ERROR, "expected ERROR, got %d\n", region_type);
    }

    got_button_down = got_button_up = FALSE;
    simulate_click(TRUE, 150, 150);
    while (wait_for_message(&msg))
    {
        DispatchMessageA(&msg);

        if (msg.message == WM_LBUTTONDOWN)
        {
            ok(msg.hwnd == hwnd, "msg.hwnd = %p\n", msg.hwnd);
            got_button_down = TRUE;
        }
        else if (msg.message == WM_LBUTTONUP)
        {
            ok(msg.hwnd == hwnd, "msg.hwnd = %p\n", msg.hwnd);
            got_button_up = TRUE;
            break;
        }
    }
    ok(got_button_down, "expected WM_LBUTTONDOWN message\n");
    ok(got_button_up, "expected WM_LBUTTONUP message\n");

    ret = SetLayeredWindowAttributes(hwnd, RGB(128, 128, 128), 0, LWA_COLORKEY);
    ok(ret, "SetLayeredWindowAttributes failed\n");
    while (wait_for_message(&msg)) DispatchMessageA(&msg);
    Sleep(100);

    if (pGetWindowRgnBox)
    {
        region_type = pGetWindowRgnBox(hwnd, &region);
        ok(region_type == ERROR, "expected ERROR, got %d\n", region_type);
    }

    got_button_down = got_button_up = FALSE;
    simulate_click(TRUE, 150, 150);
    while (wait_for_message(&msg))
    {
        DispatchMessageA(&msg);

        if (msg.message == WM_LBUTTONDOWN)
        {
            todo_wine
            ok(msg.hwnd == button_win, "msg.hwnd = %p\n", msg.hwnd);
            got_button_down = TRUE;
        }
        else if (msg.message == WM_LBUTTONUP)
        {
            todo_wine
            ok(msg.hwnd == button_win, "msg.hwnd = %p\n", msg.hwnd);
            got_button_up = TRUE;
            break;
        }
    }
    ok(got_button_down, "expected WM_LBUTTONDOWN message\n");
    ok(got_button_up, "expected WM_LBUTTONUP message\n");

    SetWindowLongA(hwnd, GWL_EXSTYLE, GetWindowLongA(hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);
    while (wait_for_message(&msg)) DispatchMessageA(&msg);
    Sleep(100);

    if (pGetWindowRgnBox)
    {
        region_type = pGetWindowRgnBox(hwnd, &region);
        ok(region_type == ERROR, "expected ERROR, got %d\n", region_type);
    }

    got_button_down = got_button_up = FALSE;
    simulate_click(TRUE, 150, 150);
    while (wait_for_message(&msg))
    {
        DispatchMessageA(&msg);

        if (msg.message == WM_LBUTTONDOWN)
        {
            ok(msg.hwnd == hwnd, "msg.hwnd = %p\n", msg.hwnd);
            got_button_down = TRUE;
        }
        else if (msg.message == WM_LBUTTONUP)
        {
            ok(msg.hwnd == hwnd, "msg.hwnd = %p\n", msg.hwnd);
            got_button_up = TRUE;
            break;
        }
    }
    ok(got_button_down, "expected WM_LBUTTONDOWN message\n");
    ok(got_button_up, "expected WM_LBUTTONUP message\n");

    hregion = CreateRectRgn(0, 0, 10, 10);
    ok(hregion != NULL, "CreateRectRgn failed\n");
    ret = SetWindowRgn(hwnd, hregion, TRUE);
    ok(ret, "SetWindowRgn failed\n");
    DeleteObject(hregion);
    while (wait_for_message(&msg)) DispatchMessageA(&msg);
    Sleep(1000);

    if (pGetWindowRgnBox)
    {
        region_type = pGetWindowRgnBox(hwnd, &region);
        ok(region_type == SIMPLEREGION, "expected SIMPLEREGION, got %d\n", region_type);
    }

    got_button_down = got_button_up = FALSE;
    simulate_click(TRUE, 150, 150);
    while (wait_for_message(&msg))
    {
        DispatchMessageA(&msg);

        if (msg.message == WM_LBUTTONDOWN)
        {
            ok(msg.hwnd == button_win, "msg.hwnd = %p\n", msg.hwnd);
            got_button_down = TRUE;
        }
        else if (msg.message == WM_LBUTTONUP)
        {
            ok(msg.hwnd == button_win, "msg.hwnd = %p\n", msg.hwnd);
            got_button_up = TRUE;
            break;
        }
    }
    ok(got_button_down, "expected WM_LBUTTONDOWN message\n");
    ok(got_button_up, "expected WM_LBUTTONUP message\n");

    DestroyWindow(static_win);
    DestroyWindow(hwnd);
    SetCursorPos(pt_org.x, pt_org.y);

    CloseHandle(thread_data.start_event);
    CloseHandle(thread_data.end_event);
    DestroyWindow(button_win);
}


static LRESULT WINAPI MsgCheckProcA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_USER+1)
    {
        HWND hwnd = (HWND)lParam;
        ok(GetFocus() == hwnd, "thread expected focus %p, got %p\n", hwnd, GetFocus());
        ok(GetActiveWindow() == hwnd, "thread expected active %p, got %p\n", hwnd, GetActiveWindow());
    }
    return DefWindowProcA(hwnd, message, wParam, lParam);
}

struct wnd_event
{
    HWND hwnd;
    HANDLE wait_event;
    HANDLE start_event;
    DWORD attach_from;
    DWORD attach_to;
    BOOL setWindows;
};

static DWORD WINAPI thread_proc(void *param)
{
    MSG msg;
    struct wnd_event *wnd_event = param;
    BOOL ret;

    if (wnd_event->wait_event)
    {
        ok(WaitForSingleObject(wnd_event->wait_event, INFINITE) == WAIT_OBJECT_0,
           "WaitForSingleObject failed\n");
        CloseHandle(wnd_event->wait_event);
    }

    if (wnd_event->attach_from)
    {
        ret = AttachThreadInput(wnd_event->attach_from, GetCurrentThreadId(), TRUE);
        ok(ret, "AttachThreadInput error %d\n", GetLastError());
    }

    if (wnd_event->attach_to)
    {
        ret = AttachThreadInput(GetCurrentThreadId(), wnd_event->attach_to, TRUE);
        ok(ret, "AttachThreadInput error %d\n", GetLastError());
    }

    wnd_event->hwnd = CreateWindowExA(0, "TestWindowClass", "window caption text", WS_OVERLAPPEDWINDOW,
                                      100, 100, 200, 200, 0, 0, 0, NULL);
    ok(wnd_event->hwnd != 0, "Failed to create overlapped window\n");

    if (wnd_event->setWindows)
    {
        SetFocus(wnd_event->hwnd);
        SetActiveWindow(wnd_event->hwnd);
    }

    SetEvent(wnd_event->start_event);

    while (GetMessageA(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return 0;
}

static void test_attach_input(void)
{
    HANDLE hThread;
    HWND ourWnd, Wnd2;
    DWORD ret, tid;
    struct wnd_event wnd_event;
    WNDCLASSA cls;

    cls.style = 0;
    cls.lpfnWndProc = MsgCheckProcA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorW( NULL, (LPCWSTR)IDC_ARROW);
    cls.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "TestWindowClass";
    if(!RegisterClassA(&cls)) return;

    wnd_event.wait_event = NULL;
    wnd_event.start_event = CreateEventW(NULL, 0, 0, NULL);
    wnd_event.attach_from = 0;
    wnd_event.attach_to = 0;
    wnd_event.setWindows = FALSE;
    if (!wnd_event.start_event)
    {
        win_skip("skipping interthread message test under win9x\n");
        return;
    }

    hThread = CreateThread(NULL, 0, thread_proc, &wnd_event, 0, &tid);
    ok(hThread != NULL, "CreateThread failed, error %d\n", GetLastError());

    ok(WaitForSingleObject(wnd_event.start_event, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(wnd_event.start_event);

    ourWnd = CreateWindowExA(0, "TestWindowClass", NULL, WS_OVERLAPPEDWINDOW,
                            0, 0, 0, 0, 0, 0, 0, NULL);
    ok(ourWnd!= 0, "failed to create ourWnd window\n");

    Wnd2 = CreateWindowExA(0, "TestWindowClass", NULL, WS_OVERLAPPEDWINDOW,
                            0, 0, 0, 0, 0, 0, 0, NULL);
    ok(Wnd2!= 0, "failed to create Wnd2 window\n");

    SetFocus(ourWnd);
    SetActiveWindow(ourWnd);

    ret = AttachThreadInput(GetCurrentThreadId(), tid, TRUE);
    ok(ret, "AttachThreadInput error %d\n", GetLastError());

    ok(GetActiveWindow() == ourWnd, "expected active %p, got %p\n", ourWnd, GetActiveWindow());
    ok(GetFocus() == ourWnd, "expected focus %p, got %p\n", ourWnd, GetFocus());

    SendMessageA(wnd_event.hwnd, WM_USER+1, 0, (LPARAM)ourWnd);

    ret = AttachThreadInput(GetCurrentThreadId(), tid, FALSE);
    ok(ret, "AttachThreadInput error %d\n", GetLastError());
    ok(GetActiveWindow() == ourWnd, "expected active %p, got %p\n", ourWnd, GetActiveWindow());
    ok(GetFocus() == ourWnd, "expected focus %p, got %p\n", ourWnd, GetFocus());

    SendMessageA(wnd_event.hwnd, WM_USER+1, 0, 0);

    ret = AttachThreadInput(GetCurrentThreadId(), tid, TRUE);
    ok(ret, "AttachThreadInput error %d\n", GetLastError());

    ok(GetActiveWindow() == ourWnd, "expected active %p, got %p\n", ourWnd, GetActiveWindow());
    ok(GetFocus() == ourWnd, "expected focus %p, got %p\n", ourWnd, GetFocus());
    SendMessageA(wnd_event.hwnd, WM_USER+1, 0, (LPARAM)ourWnd);

    SetActiveWindow(Wnd2);
    SetFocus(Wnd2);
    ok(GetActiveWindow() == Wnd2, "expected active %p, got %p\n", Wnd2, GetActiveWindow());
    ok(GetFocus() == Wnd2, "expected focus %p, got %p\n", Wnd2, GetFocus());

    SendMessageA(wnd_event.hwnd, WM_USER+1, 0, (LPARAM)Wnd2);

    ret = AttachThreadInput(GetCurrentThreadId(), tid, FALSE);
    ok(ret, "AttachThreadInput error %d\n", GetLastError());
    ok(GetActiveWindow() == Wnd2, "expected active %p, got %p\n", Wnd2, GetActiveWindow());
    ok(GetFocus() == Wnd2, "expected focus %p, got %p\n", Wnd2, GetFocus());

    SendMessageA(wnd_event.hwnd, WM_USER+1, 0, 0);

    ret = PostMessageA(wnd_event.hwnd, WM_QUIT, 0, 0);
    ok(ret, "PostMessageA(WM_QUIT) error %d\n", GetLastError());

    ok(WaitForSingleObject(hThread, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(hThread);

    wnd_event.wait_event = NULL;
    wnd_event.start_event = CreateEventW(NULL, 0, 0, NULL);
    wnd_event.attach_from = 0;
    wnd_event.attach_to = 0;
    wnd_event.setWindows = TRUE;

    hThread = CreateThread(NULL, 0, thread_proc, &wnd_event, 0, &tid);
    ok(hThread != NULL, "CreateThread failed, error %d\n", GetLastError());

    ok(WaitForSingleObject(wnd_event.start_event, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(wnd_event.start_event);

    SetFocus(ourWnd);
    SetActiveWindow(ourWnd);

    ret = AttachThreadInput(GetCurrentThreadId(), tid, TRUE);
    ok(ret, "AttachThreadInput error %d\n", GetLastError());

    ok(GetActiveWindow() == wnd_event.hwnd, "expected active %p, got %p\n", wnd_event.hwnd, GetActiveWindow());
    ok(GetFocus() == wnd_event.hwnd, "expected focus %p, got %p\n", wnd_event.hwnd, GetFocus());

    SendMessageA(wnd_event.hwnd, WM_USER+1, 0, (LPARAM)wnd_event.hwnd);

    ret = AttachThreadInput(GetCurrentThreadId(), tid, FALSE);
    ok(ret, "AttachThreadInput error %d\n", GetLastError());

    ok(GetActiveWindow() == 0, "expected active 0, got %p\n", GetActiveWindow());
    ok(GetFocus() == 0, "expected focus 0, got %p\n", GetFocus());

    SendMessageA(wnd_event.hwnd, WM_USER+1, 0, (LPARAM)wnd_event.hwnd);

    ret = AttachThreadInput(GetCurrentThreadId(), tid, TRUE);
    ok(ret, "AttachThreadInput error %d\n", GetLastError());

    ok(GetActiveWindow() == wnd_event.hwnd, "expected active %p, got %p\n", wnd_event.hwnd, GetActiveWindow());
    ok(GetFocus() == wnd_event.hwnd, "expected focus %p, got %p\n", wnd_event.hwnd, GetFocus());

    SendMessageA(wnd_event.hwnd, WM_USER+1, 0, (LPARAM)wnd_event.hwnd);

    SetFocus(Wnd2);
    SetActiveWindow(Wnd2);
    ok(GetActiveWindow() == Wnd2, "expected active %p, got %p\n", Wnd2, GetActiveWindow());
    ok(GetFocus() == Wnd2, "expected focus %p, got %p\n", Wnd2, GetFocus());

    SendMessageA(wnd_event.hwnd, WM_USER+1, 0, (LPARAM)Wnd2);

    ret = AttachThreadInput(GetCurrentThreadId(), tid, FALSE);
    ok(ret, "AttachThreadInput error %d\n", GetLastError());

    ok(GetActiveWindow() == Wnd2, "expected active %p, got %p\n", Wnd2, GetActiveWindow());
    ok(GetFocus() == Wnd2, "expected focus %p, got %p\n", Wnd2, GetFocus());

    SendMessageA(wnd_event.hwnd, WM_USER+1, 0, 0);

    ret = PostMessageA(wnd_event.hwnd, WM_QUIT, 0, 0);
    ok(ret, "PostMessageA(WM_QUIT) error %d\n", GetLastError());

    ok(WaitForSingleObject(hThread, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(hThread);

    wnd_event.wait_event = CreateEventW(NULL, 0, 0, NULL);
    wnd_event.start_event = CreateEventW(NULL, 0, 0, NULL);
    wnd_event.attach_from = 0;
    wnd_event.attach_to = 0;
    wnd_event.setWindows = TRUE;

    hThread = CreateThread(NULL, 0, thread_proc, &wnd_event, 0, &tid);
    ok(hThread != NULL, "CreateThread failed, error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = AttachThreadInput(GetCurrentThreadId(), tid, TRUE);
    ok(!ret, "AttachThreadInput succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER || broken(GetLastError() == 0xdeadbeef) /* <= Win XP */,
       "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = AttachThreadInput(tid, GetCurrentThreadId(), TRUE);
    ok(!ret, "AttachThreadInput succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER || broken(GetLastError() == 0xdeadbeef) /* <= Win XP */,
       "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetEvent(wnd_event.wait_event);

    ok(WaitForSingleObject(wnd_event.start_event, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(wnd_event.start_event);

    ret = PostMessageA(wnd_event.hwnd, WM_QUIT, 0, 0);
    ok(ret, "PostMessageA(WM_QUIT) error %d\n", GetLastError());

    ok(WaitForSingleObject(hThread, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(hThread);

    wnd_event.wait_event = NULL;
    wnd_event.start_event = CreateEventW(NULL, 0, 0, NULL);
    wnd_event.attach_from = GetCurrentThreadId();
    wnd_event.attach_to = 0;
    wnd_event.setWindows = FALSE;

    SetFocus(ourWnd);
    SetActiveWindow(ourWnd);

    hThread = CreateThread(NULL, 0, thread_proc, &wnd_event, 0, &tid);
    ok(hThread != NULL, "CreateThread failed, error %d\n", GetLastError());

    ok(WaitForSingleObject(wnd_event.start_event, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(wnd_event.start_event);

    ok(GetActiveWindow() == ourWnd, "expected active %p, got %p\n", ourWnd, GetActiveWindow());
    ok(GetFocus() == ourWnd, "expected focus %p, got %p\n", ourWnd, GetFocus());

    ret = PostMessageA(wnd_event.hwnd, WM_QUIT, 0, 0);
    ok(ret, "PostMessageA(WM_QUIT) error %d\n", GetLastError());

    ok(WaitForSingleObject(hThread, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(hThread);

    wnd_event.wait_event = NULL;
    wnd_event.start_event = CreateEventW(NULL, 0, 0, NULL);
    wnd_event.attach_from = 0;
    wnd_event.attach_to = GetCurrentThreadId();
    wnd_event.setWindows = FALSE;

    SetFocus(ourWnd);
    SetActiveWindow(ourWnd);

    hThread = CreateThread(NULL, 0, thread_proc, &wnd_event, 0, &tid);
    ok(hThread != NULL, "CreateThread failed, error %d\n", GetLastError());

    ok(WaitForSingleObject(wnd_event.start_event, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(wnd_event.start_event);

    ok(GetActiveWindow() == ourWnd, "expected active %p, got %p\n", ourWnd, GetActiveWindow());
    ok(GetFocus() == ourWnd, "expected focus %p, got %p\n", ourWnd, GetFocus());

    ret = PostMessageA(wnd_event.hwnd, WM_QUIT, 0, 0);
    ok(ret, "PostMessageA(WM_QUIT) error %d\n", GetLastError());

    ok(WaitForSingleObject(hThread, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(hThread);
    DestroyWindow(ourWnd);
    DestroyWindow(Wnd2);
}

static DWORD WINAPI get_key_state_thread(void *arg)
{
    HANDLE *semaphores = arg;
    DWORD result;

    ReleaseSemaphore(semaphores[0], 1, NULL);
    result = WaitForSingleObject(semaphores[1], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", result);

    result = GetKeyState('X');
    ok((result & 0x8000) || broken(!(result & 0x8000)), /* > Win 2003 */
       "expected that highest bit is set, got %x\n", result);

    ReleaseSemaphore(semaphores[0], 1, NULL);
    result = WaitForSingleObject(semaphores[1], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", result);

    result = GetKeyState('X');
    ok(!(result & 0x8000), "expected that highest bit is unset, got %x\n", result);

    return 0;
}

static void test_GetKeyState(void)
{
    HANDLE semaphores[2];
    HANDLE thread;
    DWORD result;
    HWND hwnd;

    semaphores[0] = CreateSemaphoreA(NULL, 0, 1, NULL);
    ok(semaphores[0] != NULL, "CreateSemaphoreA failed %u\n", GetLastError());
    semaphores[1] = CreateSemaphoreA(NULL, 0, 1, NULL);
    ok(semaphores[1] != NULL, "CreateSemaphoreA failed %u\n", GetLastError());

    hwnd = CreateWindowA("static", "Title", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                         10, 10, 200, 200, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindowA failed %u\n", GetLastError());

    thread = CreateThread(NULL, 0, get_key_state_thread, semaphores, 0, NULL);
    ok(thread != NULL, "CreateThread failed %u\n", GetLastError());
    result = WaitForSingleObject(semaphores[0], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", result);

    SetForegroundWindow(hwnd);
    SetFocus(hwnd);
    keybd_event('X', 0, 0, 0);

    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(semaphores[0], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", result);

    keybd_event('X', 0, KEYEVENTF_KEYUP, 0);

    ReleaseSemaphore(semaphores[1], 1, NULL);
    result = WaitForSingleObject(thread, 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", result);
    CloseHandle(thread);

    DestroyWindow(hwnd);
    CloseHandle(semaphores[0]);
    CloseHandle(semaphores[1]);
}

static void test_OemKeyScan(void)
{
    DWORD ret, expect, vkey, scan;
    WCHAR oem, wchr;
    char oem_char;

    for (oem = 0; oem < 0x200; oem++)
    {
        ret = OemKeyScan( oem );

        oem_char = LOBYTE( oem );
        /* OemKeyScan returns -1 for any character that cannot be mapped,
         * whereas OemToCharBuff changes unmappable characters to question
         * marks. The ASCII characters 0-127, including the real question mark
         * character, are all mappable and are the same in all OEM codepages. */
        if (!OemToCharBuffW( &oem_char, &wchr, 1 ) || (wchr == '?' && oem_char < 0))
            expect = -1;
        else
        {
            vkey = VkKeyScanW( wchr );
            scan = MapVirtualKeyW( LOBYTE( vkey ), MAPVK_VK_TO_VSC );
            if (!scan)
                expect = -1;
            else
            {
                vkey &= 0xff00;
                vkey <<= 8;
                expect = vkey | scan;
            }
        }
        ok( ret == expect, "%04x: got %08x expected %08x\n", oem, ret, expect );
    }
}

#if (_WIN32_WINNT >= 0x0602)

static INPUT_MESSAGE_SOURCE expect_src;

static LRESULT WINAPI msg_source_proc( HWND hwnd, UINT message, WPARAM wp, LPARAM lp )
{
    INPUT_MESSAGE_SOURCE source;
    MSG msg;

    ok( pGetCurrentInputMessageSource( &source ), "GetCurrentInputMessageSource failed\n" );
    switch (message)
    {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        ok( source.deviceType == expect_src.deviceType || /* also accept system-generated WM_MOUSEMOVE */
            (message == WM_MOUSEMOVE && source.deviceType == IMDT_UNAVAILABLE),
            "%x: wrong deviceType %x/%x\n", message, source.deviceType, expect_src.deviceType );
        ok( source.originId == expect_src.originId ||
            (message == WM_MOUSEMOVE && source.originId == IMO_SYSTEM),
            "%x: wrong originId %x/%x\n", message, source.originId, expect_src.originId );
        SendMessageA( hwnd, WM_USER, 0, 0 );
        PostMessageA( hwnd, WM_USER, 0, 0 );
        if (PeekMessageW( &msg, hwnd, WM_USER, WM_USER, PM_REMOVE )) DispatchMessageW( &msg );
        ok( source.deviceType == expect_src.deviceType || /* also accept system-generated WM_MOUSEMOVE */
            (message == WM_MOUSEMOVE && source.deviceType == IMDT_UNAVAILABLE),
            "%x: wrong deviceType %x/%x\n", message, source.deviceType, expect_src.deviceType );
        ok( source.originId == expect_src.originId ||
            (message == WM_MOUSEMOVE && source.originId == IMO_SYSTEM),
            "%x: wrong originId %x/%x\n", message, source.originId, expect_src.originId );
        break;
    default:
        ok( source.deviceType == IMDT_UNAVAILABLE, "%x: wrong deviceType %x\n",
            message, source.deviceType );
        ok( source.originId == 0, "%x: wrong originId %x\n", message, source.originId );
        break;
    }

    return DefWindowProcA( hwnd, message, wp, lp );
}

static void test_input_message_source(void)
{
    WNDCLASSA cls;
    TEST_INPUT inputs[2];
    HWND hwnd;
    RECT rc;
    MSG msg;

    cls.style = 0;
    cls.lpfnWndProc = msg_source_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = 0;
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "message source class";
    RegisterClassA(&cls);
    hwnd = CreateWindowA( cls.lpszClassName, "test", WS_OVERLAPPED, 0, 0, 100, 100,
                          0, 0, 0, 0 );
    ShowWindow( hwnd, SW_SHOWNORMAL );
    UpdateWindow( hwnd );
    SetForegroundWindow( hwnd );
    SetFocus( hwnd );

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].u.ki.dwExtraInfo = 0;
    inputs[0].u.ki.time = 0;
    inputs[0].u.ki.wVk = 0;
    inputs[0].u.ki.wScan = 0x3c0;
    inputs[0].u.ki.dwFlags = KEYEVENTF_UNICODE;
    inputs[1] = inputs[0];
    inputs[1].u.ki.dwFlags |= KEYEVENTF_KEYUP;

    expect_src.deviceType = IMDT_UNAVAILABLE;
    expect_src.originId = IMO_UNAVAILABLE;
    SendMessageA( hwnd, WM_KEYDOWN, 0, 0 );
    SendMessageA( hwnd, WM_MOUSEMOVE, 0, 0 );

    SendInput( 2, (INPUT *)inputs, sizeof(INPUT) );
    while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE ))
    {
        expect_src.deviceType = IMDT_KEYBOARD;
        expect_src.originId = IMO_INJECTED;
        TranslateMessage( &msg );
        DispatchMessageW( &msg );
    }
    GetWindowRect( hwnd, &rc );
    simulate_click( TRUE, (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 );
    simulate_click( FALSE, (rc.left + rc.right) / 2 + 1, (rc.top + rc.bottom) / 2 + 1 );
    while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE ))
    {
        expect_src.deviceType = IMDT_MOUSE;
        expect_src.originId = IMO_INJECTED;
        TranslateMessage( &msg );
        DispatchMessageW( &msg );
    }

    expect_src.deviceType = IMDT_UNAVAILABLE;
    expect_src.originId = IMO_UNAVAILABLE;
    SendMessageA( hwnd, WM_KEYDOWN, 0, 0 );
    SendMessageA( hwnd, WM_LBUTTONDOWN, 0, 0 );
    PostMessageA( hwnd, WM_KEYUP, 0, 0 );
    PostMessageA( hwnd, WM_LBUTTONUP, 0, 0 );
    while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE ))
    {
        TranslateMessage( &msg );
        DispatchMessageW( &msg );
    }

    expect_src.deviceType = IMDT_UNAVAILABLE;
    expect_src.originId = IMO_SYSTEM;
    SetCursorPos( (rc.left + rc.right) / 2 - 1, (rc.top + rc.bottom) / 2 - 1 );
    while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE ))
    {
        TranslateMessage( &msg );
        DispatchMessageW( &msg );
    }

    DestroyWindow( hwnd );
    UnregisterClassA( cls.lpszClassName, GetModuleHandleA(0) );
}

static void test_GetPointerType(void)
{
    BOOL ret;
    POINTER_INPUT_TYPE type = -1;
    UINT id = 0;

    SetLastError(0xdeadbeef);
    ret = pGetPointerType(id, NULL);
    ok(!ret, "GetPointerType should have failed.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "expected error ERROR_INVALID_PARAMETER, got %u.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetPointerType(id, &type);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "expected error ERROR_INVALID_PARAMETER, got %u.\n", GetLastError());
    ok(!ret, "GetPointerType failed, got type %d for %u.\n", type, id );
    ok(type == -1, " type %d\n", type );

    id = 1;
    ret = pGetPointerType(id, &type);
    ok(ret, "GetPointerType failed, got type %d for %u.\n", type, id );
    ok(type == PT_MOUSE, " type %d\n", type );
}

#endif // (_WIN32_WINNT >= 0x0602)

static void test_GetKeyboardLayoutList(void)
{
    int cnt, cnt2;
    HKL *layouts;
    ULONG_PTR baselayout;
    LANGID langid;

    baselayout = GetUserDefaultLCID();
    langid = PRIMARYLANGID(LANGIDFROMLCID(baselayout));
    if (langid == LANG_CHINESE || langid == LANG_JAPANESE || langid == LANG_KOREAN)
        baselayout = MAKELONG( baselayout, 0xe001 ); /* IME */
    else
        baselayout |= baselayout << 16;

    cnt = GetKeyboardLayoutList(0, NULL);
    /* Most users will not have more than a few keyboard layouts installed at a time. */
    ok(cnt > 0 && cnt < 10, "Layout count %d\n", cnt);
    if (cnt > 0)
    {
        layouts = HeapAlloc(GetProcessHeap(), 0, sizeof(*layouts) * cnt );

        cnt2 = GetKeyboardLayoutList(cnt, layouts);
        ok(cnt == cnt2, "wrong value %d!=%d\n", cnt, cnt2);
        for(cnt = 0; cnt < cnt2; cnt++)
        {
            if(layouts[cnt] == (HKL)baselayout)
                break;
        }
        ok(cnt < cnt2, "Didnt find current keyboard\n");

        HeapFree(GetProcessHeap(), 0, layouts);
    }
}

START_TEST(input)
{
    POINT pos;

    init_function_pointers();
    GetCursorPos( &pos );

    test_Input_blackbox();
    test_Input_whitebox();
    test_Input_unicode();
    test_Input_mouse();
    test_keynames();
    test_mouse_ll_hook();
    test_key_map();
    test_ToUnicode();
    test_ToAscii();
    test_get_async_key_state();
    test_keyboard_layout_name();
    test_key_names();
    test_attach_input();
    test_GetKeyState();
    test_OemKeyScan();
    test_GetRawInputData();
    test_GetKeyboardLayoutList();

    if(pGetMouseMovePointsEx)
        test_GetMouseMovePointsEx();
    else
        win_skip("GetMouseMovePointsEx is not available\n");

    if(pGetRawInputDeviceList)
        test_GetRawInputDeviceList();
    else
        win_skip("GetRawInputDeviceList is not available\n");

#if (_WIN32_WINNT >= 0x0602)
    if (pGetCurrentInputMessageSource)
        test_input_message_source();
    else
        win_skip("GetCurrentInputMessageSource is not available\n");
#endif

    SetCursorPos( pos.x, pos.y );

#if (_WIN32_WINNT >= 0x0602)
    if(pGetPointerType)
        test_GetPointerType();
    else
        win_skip("GetPointerType is not available\n");
#endif
}
