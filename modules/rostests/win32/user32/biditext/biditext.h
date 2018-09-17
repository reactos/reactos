#pragma once

#include <windows.h>
#include <commctrl.h>
#include <strsafe.h>

/* Global instance handle */
extern HINSTANCE g_hInstance;

/* Window procedure for our main window */
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* Register a class for our main window */
BOOL RegisterMainWindowClass(void);

/* Create an instance of our main window */
HWND CreateMainWindow(void);

/* Dialog procedure for our "about" dialog */
INT_PTR CALLBACK AboutDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* Show our "about" dialog */
void ShowAboutDialog(HWND owner);


#define IDI_APPICON                     101
#define IDR_MAINMENU                    102
#define IDR_ACCELERATOR                 103
#define IDD_ABOUTDIALOG                 104
#define ID_FILE_EXIT                    40001
#define ID_HELP_ABOUT                   40002

#ifndef IDC_STATIC
  #define IDC_STATIC                    -1
#endif
