/****************************** Module Header ******************************\
* Module Name: access.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Typedefs, defines, and prototypes that are used by the accessibility
* routines and the various routines that call them (input routines and
* SystemParametersInfo).
*
* History:
* 11 Feb 93 GregoryW    Created
\***************************************************************************/

#ifndef _ACCESS_
#define _ACCESS_

/*
 * Main accessibility routine entry points.
 */
typedef BOOL (* ACCESSIBILITYPROC)(PKE, ULONG, int);

BOOL FilterKeys(PKE, ULONG, int);
BOOL xxxStickyKeys(PKE, ULONG, int);
BOOL MouseKeys(PKE, ULONG, int);
BOOL ToggleKeys(PKE, ULONG, int);
BOOL HighContrastHotKey(PKE, ULONG, int);
BOOL UtilityManager(PKE, ULONG, int);

BOOL AccessProceduresStream(PKE, ULONG, int);
VOID SetAccessEnabledFlag(VOID);
VOID StopFilterKeysTimers(VOID);

/*
 * Sound support.
 */
typedef BOOL (* BEEPPROC)(void);

BOOL HighBeep(void);
BOOL LowBeep(void);
BOOL KeyClick(void);
BOOL UpSiren(void);
BOOL DownSiren(void);
BOOL DoBeep(BEEPPROC BeepProc, UINT Count);

/*
 * Macros for dwFlags support
 */
#define TEST_ACCESSFLAG(s, f)               TEST_FLAG(g##s.dwFlags, f)
#define TEST_BOOL_ACCESSFLAG(s, f)          TEST_BOOL_FLAG(g##s.dwFlags, f)
#define SET_ACCESSFLAG(s, f)                SET_FLAG(g##s.dwFlags, f)
#define CLEAR_ACCESSFLAG(s, f)              CLEAR_FLAG(g##s.dwFlags, f)
#define SET_OR_CLEAR_ACCESSFLAG(s, f, fSet) SET_OR_CLEAR_FLAG(g##s.dwFlags, f, fSet)
#define TOGGLE_ACCESSFLAG(s, f)             TOGGLE_FLAG(g##s.dwFlags, f)


#define RIGHTSHIFTBIT         0x2
#define ONLYRIGHTSHIFTDOWN(state) ((state) == RIGHTSHIFTBIT)
#define FKFIRSTWARNINGTIME    4000
#define FKACTIVATIONDELTA     4000
#define FKEMERGENCY1DELTA     4000
#define FKEMERGENCY2DELTA     4000

//
// Warning: do not change the ordering of these.
//
#define FKIDLE                   0
#define FKFIRSTWARNING           1
#define FKTOGGLE                 2
#define FKFIRSTLEVELEMERGENCY    3
#define FKSECONDLEVELEMERGENCY   4
#define FKMOUSEMOVE              8

/*
 * StickyKeys support.
 */
#define TOGGLE_STICKYKEYS_COUNT 5
#define UNION(x, y) ((x) | (y))
#define LEFTSHIFTKEY(key)  (((key) & 0xff) == VK_LSHIFT)
#define RIGHTSHIFTKEY(key) (((key) & 0xff) == VK_RSHIFT)
#define LEFTORRIGHTSHIFTKEY(key) (LEFTSHIFTKEY(key) || RIGHTSHIFTKEY(key))
BOOL xxxTwoKeysDown(int);
VOID SetGlobalCursorLevel(INT iCursorLevel);
VOID xxxUpdateModifierState(int, int);
VOID xxxTurnOffStickyKeys(VOID);
VOID xxxHardwareMouseKeyUp(DWORD);

/*
 * ToggleKeys support.
 */
#define TOGGLEKEYTOGGLETIME    5000

/*
 * MouseKeys support.
 */

//
// Parameter Constants for xxxButtonEvent()
//
#define MOUSE_BUTTON_LEFT   0x0001
#define MOUSE_BUTTON_RIGHT  0x0002

#define MOUSEKEYMODBITS     0x11
#define LRALT               0x30
#define LRCONTROL           0x0c
#define LRSHIFT             0x03
#define LRWIN               0xc0
#define VK_U                0x55

//
// Mouse cursor movement data.
//
#define MK_UP               0xFF00
#define MK_DOWN             0x0100
#define MK_RIGHT            0x0001
#define MK_LEFT             0x00FF

#define MOUSETIMERRATE      50
#define MOUSETICKS          (1000 / MOUSETIMERRATE)
/*
 * Factor for high-speed movement.
 */
#define MK_CONTROL_SPEED    4


typedef BOOL (* MOUSEPROC)(USHORT);

VOID TurnOffMouseKeys(VOID);
BOOL xxxMKButtonClick(USHORT);
BOOL xxxMKMouseMove(USHORT);
BOOL xxxMKButtonSetState(USHORT);
BOOL MKButtonSelect(USHORT);
BOOL xxxMKButtonDoubleClick(USHORT);
BOOL xxxMKToggleMouseKeys(USHORT);
VOID MKShowMouseCursor(VOID);
VOID MKHideMouseCursor(VOID);
VOID CalculateMouseTable(VOID);

/*
 * TimeOut support.
 */
VOID AccessTimeOutReset(VOID);
VOID xxxAccessTimeOutTimer(PWND, UINT, UINT_PTR, LPARAM);

/*
 * SoundSentry support.
 */
BOOL _UserSoundSentryWorker(VOID);

#endif  // !_ACCESS_
