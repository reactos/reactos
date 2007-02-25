//
//	password.c
//
//	Password support for Win9x
//
#include <windows.h>
#include <tchar.h>

typedef BOOL (WINAPI *VERIFYSCREENSAVEPWD)(HWND hwnd);
typedef VOID (WINAPI *PWDCHANGEPASSWORD)(LPCTSTR lpcRegkeyname, HWND hwnd,UINT uiReserved1,UINT uiReserved2);

BOOL VerifyPassword(HWND hwnd)
{ 
	// Under NT, we return TRUE immediately. This lets the saver quit,
	// and the system manages passwords. Under '95, we call VerifyScreenSavePwd.
	// This checks the appropriate registry key and, if necessary,
	// pops up a verify dialog

	HINSTANCE			hpwdcpl;
	VERIFYSCREENSAVEPWD VerifyScreenSavePwd;
	BOOL				fResult;
	
	if(GetVersion() < 0x80000000)
		return TRUE;

	hpwdcpl = LoadLibrary(_T("PASSWORD.CPL"));

	if(hpwdcpl == NULL) 
	{
		return FALSE;
	}

	
	VerifyScreenSavePwd = (VERIFYSCREENSAVEPWD)GetProcAddress(hpwdcpl, "VerifyScreenSavePwd");

	if(VerifyScreenSavePwd == NULL)
	{ 
		FreeLibrary(hpwdcpl);
		return FALSE;
	}

	fResult = VerifyScreenSavePwd(hwnd); 
	FreeLibrary(hpwdcpl);

	return fResult;
}

BOOL ChangePassword(HWND hwnd)
{ 
	// This only ever gets called under '95, when started with the /a option.
	HINSTANCE hmpr = LoadLibrary(_T("MPR.DLL"));
	PWDCHANGEPASSWORD PwdChangePassword;

	if(hmpr == NULL) 
		return FALSE;

	PwdChangePassword = (PWDCHANGEPASSWORD)GetProcAddress(hmpr, "PwdChangePasswordA");
  
	if(PwdChangePassword == NULL)
	{ 
		FreeLibrary(hmpr);
		return FALSE;
	}

	PwdChangePassword(_T("SCRSAVE"), hwnd, 0, 0); 
	FreeLibrary(hmpr);

	return TRUE;
}
