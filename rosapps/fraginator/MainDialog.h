#ifndef MAINDIALOG_H
#define MAINDIALOG_H


#include <windows.h>


#define WM_UPDATEINFO (WM_USER + 1)


INT_PTR CALLBACK MainDialogProc (HWND Dlg, UINT Msg, WPARAM WParam, LPARAM LParam);


#endif // MAINDIALOG_H