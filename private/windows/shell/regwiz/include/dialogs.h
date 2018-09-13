/*********************************************************************
Registration Wizard
Dialogs.h

10/13/94 - Tracy Ferrier
(c) 1994-95 Microsoft Corporation
*********************************************************************/

#include <tchar.h>

INT_PTR WelcomeDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NameDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AddressDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ResellerDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
//INT_PTR CALLBACK PIDDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK InformDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SystemInventoryDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ProdInventoryDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK RegisterDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CancelRegWizard(HINSTANCE hInstance,HWND hwndParentDlg);
int RegWizardMessage(HINSTANCE hInstance,HWND hwndParent, int dlgID);
int RegWizardMessageEx(HINSTANCE hInstance,HWND hwndParent, int dlgID, LPTSTR szSub);
void RefreshInventoryList(CRegWizard* pclRegWizard);

//
// The Below Dialog procedures are for Far East countries
INT_PTR CALLBACK AddressForFEDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NameForFEDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);