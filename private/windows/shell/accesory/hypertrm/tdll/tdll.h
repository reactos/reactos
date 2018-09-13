/*	File: D:\WACKER\tdll\tdll.h (Created: 26-Nov-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:21p $
 */

#if !defined(INCL_TDLL)
#define INCL_TDLL

BOOL TerminateApplication(const HINSTANCE hInstance);

BOOL InitInstance(const HINSTANCE hInstance,
					const LPTSTR lpCmdLine,
					const int nCmdShow);

int MessageLoop(void);
INT ExitMessage(const int nMessageNumber);

int GetFileNameFromCmdLine(TCHAR *pachCmdLine, TCHAR *pachFileName, int nSize);

LRESULT CALLBACK SessProc(HWND hwnd, UINT msg, WPARAM uPar, LPARAM lPar);

INT DoDialog(HINSTANCE hInst, LPCTSTR lpTemplateName, HWND hwndParent,
			 DLGPROC lpProc, LPARAM lPar);

HWND DoModelessDialog(HINSTANCE hInst, LPCTSTR lpTemplateName, HWND hwndParent,
			 DLGPROC lpProc, LPARAM lPar);

INT EndModelessDialog(HWND hDlg);

INT_PTR CALLBACK GenericDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar);

INT_PTR CALLBACK TransferSendDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar);
INT_PTR CALLBACK TransferReceiveDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar);

INT_PTR CALLBACK CaptureFileDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar);
INT_PTR CALLBACK PrintEchoDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar);

INT_PTR CALLBACK NewConnectionDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar);

INT_PTR CALLBACK asciiSetupDlgProc(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar);

void AboutDlg(HWND hwndSession);
BOOL RegisterTerminalClass(const HINSTANCE hInstance);

void ProcessMessage(MSG *pmsg);
int  CheckModelessMessage(MSG *pmsg);

int RegisterSidebarClass(const HINSTANCE hInstance);

void Rest(DWORD dwMilliSecs);

// from clipbrd.c

BOOL CopyBufferToClipBoard(const HWND hwnd, const DWORD dwCnt, const void *pvBuf);

#endif
