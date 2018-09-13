/*********************************************************************
Registration Wizard
RegWizard.h

10/12/94 - Tracy Ferrier
(c) 1994-95 Microsoft Corporation
*********************************************************************/
#ifndef __regwiz__
#define __regwiz__

#ifndef __CRegWizard__
#include "CRegWiz.h"
#endif

#include <tchar.h>
// Global variable references
extern vDialogInitialized;
typedef enum
{
	kNoneFound = 0,
	kModemFound = 1,
	kConfigErr = 2,
	kModemTooSlow = 3
}ModemStatus;




ModemStatus DetectModem(HINSTANCE hInstance);
extern INT_PTR  DoRegistrationWizard(HINSTANCE hInstance, CRegWizard* clRegWizard, LPTSTR szProductPath);

BOOL CheckOEMdll(void);
typedef	int		(*pfnDialogFunc)(HWND,int,LPCTSTR );

int DisplayDialog(HWND,int nDialogType, LPCTSTR lpszFieldName);

typedef	int		(WINAPI *OEMStartDialog)(WORD,HBITMAP,HPALETTE,HWND *,pfnDialogFunc);

typedef	BOOL	(WINAPI *OEMValidate)(LPSTR,LPTSTR,WORD,LPBOOL,LPWORD);

typedef	BOOL	(WINAPI *OEMGetData)(WORD,LPBOOL,LPBYTE,WORD);

typedef	int		(WINAPI *OEMDataCount)();

typedef	void	(WINAPI *OEMRegistered)(BOOL);

static HBITMAP hOemBitmap;
#endif
