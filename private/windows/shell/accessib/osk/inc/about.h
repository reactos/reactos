// about.h


//**********************************************************************
// About dialog management
//**********************************************************************

//
// Modal dialog box procedure 
//
INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT message, 
                              WPARAM wParam, LPARAM lParam);

//
// Startup procedure for modal dialog box 
//

INT_PTR AboutDlgFunc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam); 

BOOL AboutDlgDefault(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);




//**********************************************************************
// Initial Warning Message dialog management
//**********************************************************************
//
// Modal dialog box procedure 
//
INT_PTR CALLBACK WarningMsgDlgProc(HWND hDlg, UINT message, 
                              WPARAM wParam, LPARAM lParam);

//
// Startup procedure for modal dialog box 
//

INT_PTR WarningMsgDlgFunc(HWND hWnd); 

BOOL WarningMsgDlgDefault(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

