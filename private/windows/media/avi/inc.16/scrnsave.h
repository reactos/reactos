/*****************************************************************************\
*                                                                             *
* scrnsave.h    Windows 3.1 screensaver defines and definitions.
*                                                                             *
*               Version 1.0                                                   *
*                                                                             *
*               NOTE: windows.h must be #included first                       *
*                                                                             *
*               Copyright (c) 1992, Microsoft Corp.  All rights reserved.     *
*                                                                             *
\*****************************************************************************/

#ifndef _INC_SCRNSAVE
#define _INC_SCRNSAVE

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* !RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

/* Icon resource ID.
 *
 * This should be the first icon used and must have this resource number.
 * This is needed as the first icon in the file will be grabbed
 */
#define ID_APP      100
#define DLG_CHANGEPASSWORD      2000
#define DLG_ENTERPASSWORD       2001
#define DLG_INVALIDPASSWORD     2002
#define DLG_SCRNSAVECONFIGURE   2003

#define idsIsPassword           1000
#define idsIniFile              1001
#define idsScreenSaver          1002
#define idsPassword             1003
#define idsDifferentPW          1004
#define idsChangePW             1005
#define idsBadOldPW             1006
#define idsAppName              1007
#define idsNoHelpMemory         1008
#define idsHelpFile             1009
#define idsDefKeyword           1010

/* This function is the Window Procedure for the screen saver.  It is
 * up to the programmer to handle any of the messages that wish to be
 * interpretted.  Any unused messages are then passed back to
 * DefScreenSaverProc if desired which will take default action on any
 * unprocessed message...
 */
LRESULT WINAPI ScreenSaverProc(HWND, unsigned, UINT, LPARAM);

/* This function performs default message processing.  Currently handles
 * the following messages:
 *
 * WM_SYSCOMMAND:   return FALSE if wParam is SC_SCREENSAVE or SC_CLOSE
 *
 * WM_DESTROY:      PostQuitMessage(0)
 *
 * WM_SETCURSOR:    By default, this will set the cursor to a null cursor,
 *                  thereby removing it from the screen.
 *
 * WM_LBUTTONDOWN:
 * WM_MBUTTONDOWN:
 * WM_RBUTTONDOWN:
 * WM_KEYDOWN:
 * WM_KEYUP:
 * WM_MOUSEMOVE:    By default, these will cause the program to terminate.
 *                  Unless the password option is enabled.  In that case
 *                  the DlgGetPassword() dialog box is brought up.
 *
 * WM_NCACTIVATE:
 * WM_ACTIVATEAPP:
 * WM_ACTIVATE:     By default, if the wParam parameter is FALSE (signifying
 *                  that transfer is being taken away from the application),
 *                  then the program will terminate.  Termination is
 *                  accomplished by generating a WM_CLOSE message.  This way,
 *                  if the user sets something up in the WM_CREATE, a
 *                  WM_DESTROY will be generated and it can be destroyed
 *                  properly.
 *                  This message is ignored, however is the password option
 *                  is enabled.
 */
LRESULT WINAPI DefScreenSaverProc(HWND, UINT, WPARAM, LPARAM);

/* A function is also needed for configuring the screen saver.  The function
 * should be exactly like it is below and must be exported such that the
 * program can use MAKEPROCINSTANCE on it and call up a dialog box. Further-
 * more, the template used for the dialog must be called
 * ScreenSaverConfigure to allow the main function to access it...
 */
BOOL    WINAPI ScreenSaverConfigureDialog(HWND, UINT, WPARAM, LPARAM);

/* This function is called from the ScreenSaveConfigureDialog() to change
 * the Screen Saver's password.  Note:  passwords are GLOBAL to all
 * screen savers using this model.  Whether or not the password is enabled
 * is LOCAL to a particular screen saver.
 */
BOOL    WINAPI DlgChangePassword(HWND, UINT, WPARAM, LPARAM);

/* To allow the programmer the ability to register child control windows, this
 * function is called prior to the creation of the dialog box.  Any
 * registering that is required should be done here, or return TRUE if none
 * is needed...
 */
BOOL    _cdecl RegisterDialogClasses(HINSTANCE);

/* The following three functions are called by DefScreenSaverProc and must
 * be exported by all screensavers using this model.
 */
BOOL    WINAPI DlgGetPassword(HWND, UINT, WPARAM, LPARAM);
BOOL    WINAPI DlgInvalidPassword(HWND, UINT, WPARAM, LPARAM);
DWORD   WINAPI HelpMessageFilterHookFunction(int, WORD, LPMSG);


//*****************************************************************************   // ;Internal
//***********************   R E B O O T   A P I   *****************************   // ;Internal
//*****************************************************************************   // ;Internal
                                                                                  // ;Internal
BOOL WINAPI DisableReboot( VOID );    // Disables Ctrl-Alt-Del                    // ;Internal
BOOL WINAPI EnableReboot( VOID );     // Enables Ctrl-Alt-Del                     // ;Internal
                                                                                  // ;Internal
                                                                                  // ;Internal
//*****************************************************************************
//***********************   QUICK TIMER   A P I   *****************************
//*****************************************************************************

BOOL PASCAL SetQuickTimer(HWND hWnd, WORD wTimer, WORD wQuickTime);
void PASCAL KillQuickTimer(void);

//*****************************************************************************
//*****************************************************************************

extern int WINAPI GetSystemMetricsX(int);
#define GetSystemMetrics  GetSystemMetricsX

//*****************************************************************************

/*
 * There are only three other points that should be of notice:
 * 1) The screen saver must have a string declared as 'szAppName' contaning the
 *     name of the screen saver, and it must be declared as a global.
 * 2) The screen saver EXE file should be renamed to a file with a SCR
 *     extension so that the screen saver dialog form the control panel can
 *     find it when is searches for screen savers.
 */
#define WS_GT   (WS_GROUP | WS_TABSTOP)
#define MAXFILELEN  13
#define TITLEBARNAMELEN 40
#define BUFFLEN    255

/* The following globals are defined in scrnsave.lib */
extern HINSTANCE _cdecl hMainInstance;
extern HWND _cdecl hMainWindow;
extern HWND _cdecl hParentWindow;
extern BOOL _cdecl fConfigure;
extern BOOL _cdecl fPreview;
extern BOOL _cdecl fInstall;
extern char _cdecl szName[TITLEBARNAMELEN];
extern char _cdecl szIsPassword[22];
extern char _cdecl szIniFile[MAXFILELEN];
extern char _cdecl szScreenSaver[22];
extern char _cdecl szPassword[16];
extern char _cdecl szDifferentPW[BUFFLEN];
extern char _cdecl szChangePW[30];
extern char _cdecl szBadOldPW[BUFFLEN];
extern char _cdecl szHelpFile[MAXFILELEN];
extern char _cdecl szNoHelpMemory[BUFFLEN];
extern HOOKPROC _cdecl fpMessageFilter;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()
#endif  /* !RC_INVOKED */

#endif  /* !_INC_SCRNSAVE */
