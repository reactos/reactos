
/*****************************************************************************

                        D I A L O G S   H E A D E R

    Name:       dialogs.h
    Date:       21-Jan-1994
    Creator:    John Fu

    Description:
        This is the header file for dialogs.c

*****************************************************************************/


INT_PTR CALLBACK ConnectDlgProc(
    HWND    hwnd,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam);


INT_PTR CALLBACK ShareDlgProc(
    HWND    hwnd,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam);


INT_PTR CALLBACK KeepAsDlgProc(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam);
