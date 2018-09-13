/*	File: D:\WACKER\emu\emudlgs.h (Created: 14-Feb-1994)
 *
 *	Copyright 1994, 1998 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:28p $
 */

// Function prototypes...

BOOL emuSettingsDlg(const HSESSION hSession, const HWND hwndParent,
					const int nEmuId, PSTEMUSET pstEmuSettings);
LRESULT CALLBACK emuTTY_SettingsDlgProc	     (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK emuANSI_SettingsDlgProc	 (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK emuVT52_SettingsDlgProc	 (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK emuVT100_SettingsDlgProc 	 (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK emuVT100J_SettingsDlgProc 	 (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK emuVT220_SettingsDlgProc 	 (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK emuVT320_SettingsDlgProc 	 (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK emuMinitel_SettingsDlgProc(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar);
LRESULT CALLBACK emuViewdata_SettingsDlgProc(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar);

#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
int emuColorSettingsDlg(const HSESSION hSession, 
						const HWND hwndParent,
						PSTEMUSET pstEmuSettings);
LRESULT CALLBACK emuColorSettingsDlgProc(HWND hDlg, 
										UINT wMsg, 
										WPARAM wPar, 
										LPARAM lPar);
#endif