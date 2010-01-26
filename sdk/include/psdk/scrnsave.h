/*
 * PROJECT:         ReactOS Screen Saver Library
 * LICENSE:         GPL v2 or any later version
 * FILE:            include/psdk/scrnsave.h
 * PURPOSE:         Header file for the library
 * PROGRAMMERS:     Anders Norlander <anorland@hem2.passagen.se>
 *                  Colin Finck <mail@colinfinck.de>
 */

#ifndef _SCRNSAVE_H
#define _SCRNSAVE_H

#ifdef __cplusplus
extern "C" {
#endif

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

// If you add a configuration dialog for your screen saver, it must have this dialog ID.
#define DLG_SCRNSAVECONFIGURE   2003

#define IDS_DESCRIPTION         1
#define ID_APP                  100

#define WS_GT                   (WS_GROUP | WS_TABSTOP)
#define MAXFILELEN              13
#define TITLEBARNAMELEN         40
#define APPNAMEBUFFERLEN        40
#define BUFFLEN                 255

// The dialog procedure of the screen saver configure dialog (if any)
// If you don't have a configuration dialog, just implement a procedure that always returns FALSE.
BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Use this function if you want to register special classes before opening the configuration dialog.
// Return TRUE here if the classes were registered successfully and the configuration dialog shall be opened.
// If you return FALSE, no configuration dialog will be opened.
BOOL WINAPI RegisterDialogClasses(HANDLE hInst);

// The screen saver window procedure
LRESULT WINAPI ScreenSaverProc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam);

// The window procedure, which handles default tasks for screen savers.
// Use this instead of DefWindowProc.
LRESULT WINAPI DefScreenSaverProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// These globals are defined in the screen saver library.
extern HINSTANCE    hMainInstance;
extern HWND         hMainWindow;
extern BOOL         fChildPreview;
extern TCHAR        szName[];
extern TCHAR        szAppName[];
extern TCHAR        szIniFile[];
extern TCHAR        szScreenSaver[];
extern TCHAR        szHelpFile[];
extern TCHAR        szNoHelpMemory[];
extern UINT         MyHelpMessage;

#ifdef __cplusplus
}
#endif

#endif /* _SCRNSAVE_H */
