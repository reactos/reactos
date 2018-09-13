/*
 * File:	open_msc.h - stuff for calling Common Open Dialog
 *
 * Copyright 1991 by Hilgraeve Inc. -- Monroe, MI
 * All rights reserved
 *
 * $Revision: 2 $
 * $Date: 8/18/99 10:52a $
 */

#if !defined(OPEN_MSC_H)
#define OPEN_MSC_H

// Typedef for open file name common dialog callback - mrw

typedef UINT (CALLBACK *OFNPROC)(HWND, UINT, WPARAM, LPARAM);

/* -------------- Function prototypes ------------- */

// extern VOID phbkCallOpenDialog(HWND hwnd);

// extern VOID phbkCallRemoveDialog(HWND hwnd);

// extern VOID phbkCallDuplicateDialog(HWND hwnd);

// extern VOID phbkCallConnectDialog(HWND hwnd);

// extern BOOL phbkCallConnectSpecialDialog(HWND hwnd);

// extern BOOL phbkConnectSpecialDlg(HWND hwnd);

// extern VOID gnrcCallRunScriptDialog(HWND hwnd, HSESSION hSession);

// extern VOID gnrcCallEditScriptDialog(HWND hwnd, HSESSION hSession);

// extern INT xferSendBrowseDialog(HWND hwnd, HSESSION hSession,
//								struct stSendDlgStuff FAR *pstSnd);

extern LPTSTR gnrcFindFileDialog(HWND hwnd,
								LPCTSTR pszTitle,
								LPCTSTR pszDirectory,
								LPCTSTR pszMasks);

extern LPTSTR gnrcSaveFileDialog(HWND hwnd,
								LPCTSTR pszTitle,
								LPCTSTR pszDirectory,
								LPCTSTR pszMasks,
								LPCTSTR pszInitName);

extern LPTSTR gnrcFindDirectoryDialog(HWND hwnd,
									HSESSION hSession,
									LPTSTR pszDirectory);


extern DWORD GetUserDirectory(LPTSTR pszUserDir, DWORD dwSize);
extern void  CreateUserDirectory(void);
extern BOOL IsNT(void);
extern DWORD GetWorkingDirectory(LPTSTR pszUserDir, DWORD dwSize);

#endif
