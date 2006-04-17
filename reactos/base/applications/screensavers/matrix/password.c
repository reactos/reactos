//
//	password.c
//
//	Password support for Win9x
//
#include <windows.h>

typedef BOOL (WINAPI *VERIFYSCREENSAVEPWD)(HWND hwnd);
typedef VOID (WINAPI *PWDCHANGEPASSWORD)(LPCSTR lpcRegkeyname, HWND hwnd,UINT uiReserved1,UINT uiReserved2);

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

	hpwdcpl = LoadLibrary("PASSWORD.CPL");

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
	HINSTANCE hmpr = LoadLibrary("MPR.DLL");
	PWDCHANGEPASSWORD PwdChangePassword;

	if(hmpr == NULL) 
		return FALSE;

	PwdChangePassword = (PWDCHANGEPASSWORD)GetProcAddress(hmpr, "PwdChangePasswordA");
  
	if(PwdChangePassword == NULL)
	{ 
		FreeLibrary(hmpr);
		return FALSE;
	}

	PwdChangePassword("SCRSAVE", hwnd, 0, 0); 
	FreeLibrary(hmpr);

	return TRUE;
}
