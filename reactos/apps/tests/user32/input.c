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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* test whether the right type of messages:
 * WM_KEYUP/DOWN vs WM_SYSKEYUP/DOWN  are sent in case of combined
 * keystrokes.
 *
 * For instance <ALT>-X can be accompished by
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

#define _WIN32_WINNT 0x401

#include "wine/test.h"
#include "winbase.h"
#include "winuser.h"

#include <assert.h>

/* globals */
HWND hWndTest;
long timetag = 0x10000000;

#define MAXKEYEVENTS 6
#define MAXKEYMESSAGES MAXKEYEVENTS /* assuming a key event generates one
                                       and only one message */

/* keyboard message names, sorted as their value */
static const char *MSGNAME[]={"WM_KEYDOWN", "WM_KEYUP", "WM_CHAR","WM_DEADCHAR",
    "WM_SYSKEYDOWN", "WM_SYSKEYUP", "WM_SYSCHAR", "WM_SYSDEADCHAR" ,"WM_KEYLAST"};

/* keyevents, add more as needed */
typedef enum KEVtag
{  ALTDOWN = 1, ALTUP, XDOWN, XUP, SHIFTDOWN, SHIFTUP, CTRLDOWN, CTRLUP } KEV;
/* matching VK's */
int GETVKEY[]={0, VK_MENU, VK_MENU, 'X', 'X', VK_SHIFT, VK_SHIFT, VK_CONTROL, VK_CONTROL};
/* matching scan codes */
int GETSCAN[]={0, 0x38, 0x38, 0x2D, 0x2D, 0x2A, 0x2A, 0x1D, 0x1D };
/* matching updown events */
int GETUPDOWN[]={0, 0, KEYEVENTF_KEYUP, 0, KEYEVENTF_KEYUP, 0, KEYEVENTF_KEYUP, 0, KEYEVENTF_KEYUP};
/* matching descripts */
char *getdesc[]={"", "+alt","-alt","+X","-X","+shift","-shift","+ctrl","-ctrl"};

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

#define ADDTOINPUTS(kev) \
inputs[evtctr].type = INPUT_KEYBOARD; \
    ((TEST_INPUT*)inputs)[evtctr].u.ki.wVk = GETVKEY[ kev]; \
    ((TEST_INPUT*)inputs)[evtctr].u.ki.wScan = GETSCAN[ kev]; \
    ((TEST_INPUT*)inputs)[evtctr].u.ki.dwFlags = GETUPDOWN[ kev]; \
    ((TEST_INPUT*)inputs)[evtctr].u.ki.dwExtraInfo = 0; \
    ((TEST_INPUT*)inputs)[evtctr].u.ki.time = ++timetag; \
    if( kev) evtctr++;

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
struct { int nrkev;
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

BYTE InputKeyStateTable[256];
BYTE AsyncKeyStateTable[256];
BYTE TrackSysKey = 0; /* determine whether ALT key up will cause a WM_SYSKEYUP
                         or a WM_KEYUP message */
typedef union
{
    struct
    {
	unsigned long count : 16;
	unsigned long code : 8;
	unsigned long extended : 1;
	unsigned long unused : 2;
	unsigned long win_internal : 2;
	unsigned long context : 1;
	unsigned long previous : 1;
	unsigned long transition : 1;
    } lp1;
    unsigned long lp2;
} KEYLP;

int KbdMessage( KEV kev, WPARAM *pwParam, LPARAM *plParam )
{
    UINT message;
    int VKey = GETVKEY[kev];
    KEYLP keylp;

    keylp.lp2 = 0;

    keylp.lp1.count = 1;
    keylp.lp1.code = GETSCAN[kev];
    keylp.lp1.extended = 0 ;/*  FIXME (ki->dwFlags & KEYEVENTF_EXTENDEDKEY) != 0; */
    keylp.lp1.win_internal = 0;

    if (GETUPDOWN[kev] & KEYEVENTF_KEYUP )
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
        keylp.lp1.previous = 1;
        keylp.lp1.transition = 1;
    }
    else
    {
        keylp.lp1.previous = (InputKeyStateTable[VKey] & 0x80) != 0;
        keylp.lp1.transition = 0;
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

    keylp.lp1.context = (InputKeyStateTable[VK_MENU] & 0x80) != 0; /* 1 if alt */

    if( plParam) *plParam = keylp.lp2;
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
void do_test( HWND hwnd, int seqnr, KEV td[] )
{
    INPUT inputs[MAXKEYEVENTS];
    KMSG expmsg[MAXKEYEVENTS];
    MSG msg;
    char buf[100];
    UINT evtctr=0;
    int kmctr, i;
    buf[0]='\0';
    TrackSysKey=0; /* see input.c */
    for( i = 0; i < MAXKEYEVENTS; i++) {
        ADDTOINPUTS(td[i])
        strcat(buf, getdesc[td[i]]);
        if(td[i])
            expmsg[i].message = KbdMessage(td[i], &(expmsg[i].wParam), &(expmsg[i].lParam)); /* see queue_kbd_event() */
        else
            expmsg[i].message = 0;
    }
    for( kmctr = 0; kmctr < MAXKEYEVENTS && expmsg[kmctr].message; kmctr++)
        ;
    assert( evtctr <= MAXKEYEVENTS );
    assert( evtctr == SendInput(evtctr, &inputs[0], sizeof(INPUT)));
    i = 0;
    trace("======== key stroke sequence #%d: %s =============\n",
            seqnr + 1, buf);
    while( PeekMessage(&msg,hwnd,WM_KEYFIRST,WM_KEYLAST,PM_REMOVE) ) {
        trace("message[%d] %-15s wParam %04x lParam %08lx time %lx\n", i,
                MSGNAME[msg.message - WM_KEYFIRST], msg.wParam, msg.lParam, msg.time);
        if( i < kmctr ) {
            ok( msg.message == expmsg[i].message &&
                    msg.wParam == expmsg[i].wParam &&
                    msg.lParam == expmsg[i].lParam,
                    "wrong message! expected:\n"
                    "message[%d] %-15s wParam %04x lParam %08lx\n",i,
                    MSGNAME[(expmsg[i]).message - WM_KEYFIRST],
                    expmsg[i].wParam, expmsg[i].lParam );
        }
        i++;
    }
    trace("%d messages retrieved\n", i);
    ok( i == kmctr, "message count is wrong: got %d expected: %d\n", i, kmctr);
}

/* test all combinations of the specified key events */
void TestASet( HWND hWnd, int nrkev, KEV kevdwn[], KEV kevup[] )
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
                do_test( hWnd, count++, kbuf);
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
                                do_test( hWnd, count++, kbuf);
                            }
                        }
                    }
                }
            }
        }
    }
}

/* test each set specified in the global testkeyset array */
void TestSysKeys( HWND hWnd)
{
    int i;
    for(i=0; testkeyset[i].nrkev;i++)
        TestASet( hWnd, testkeyset[i].nrkev, testkeyset[i].keydwn,
                testkeyset[i].keyup);

}

static LRESULT CALLBACK WndProc( HWND hWnd, UINT msg, WPARAM wParam,
        LPARAM lParam )
{
    switch (msg) {
        case WM_SETFOCUS:
            /* window has focus, now do the test */
            if( hWnd == hWndTest) TestSysKeys( hWnd);
            /* finished :-) */
            DestroyWindow(hWnd);
            break;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;

        default:
            return( DefWindowProcA( hWnd, msg, wParam, lParam ) );
    }

    return 0;
}

START_TEST(input)
{
    MSG msg;
    WNDCLASSA  wclass;
    HANDLE hInstance = GetModuleHandleA( NULL );

    wclass.lpszClassName = "InputSysKeyTestClass";
    wclass.style         = CS_HREDRAW | CS_VREDRAW;
    wclass.lpfnWndProc   = (WNDPROC)WndProc;
    wclass.hInstance     = hInstance;
    wclass.hIcon         = LoadIconA( 0, (LPSTR)IDI_APPLICATION );
    wclass.hCursor       = LoadCursorA( NULL, IDC_ARROW);
    wclass.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1);
    wclass.lpszMenuName = 0;
    wclass.cbClsExtra    = 0;
    wclass.cbWndExtra    = 0;
    assert (RegisterClassA( &wclass ));
    /* create the test window that will receive the keystrokes */
    assert ( hWndTest = CreateWindowA( wclass.lpszClassName, "InputSysKeyTest",
                WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 100, 100,
                NULL, NULL, hInstance, NULL) );
    ShowWindow( hWndTest, SW_SHOW);
    UpdateWindow( hWndTest);
    /* message loop */
    while( GetMessageA( &msg, 0, 0, 0 )) {
        TranslateMessage( &msg );
        DispatchMessageA( &msg );
    }
}
