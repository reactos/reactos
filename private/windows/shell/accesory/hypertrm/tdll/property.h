/*	File: D:\WACKER\tdll\property.h (Created: 19-Jan-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 2/05/99 3:21p $
 */

extern void 	DoInternalProperties(HSESSION hSession, HWND hwnd);
INT_PTR CALLBACK GeneralTabDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar);
INT_PTR CALLBACK TerminalTabDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar);
void 			propLoadEmulationCombo(const HWND hDlg, const HSESSION hSession);
int  			propGetEmuIdfromEmuCombo(HWND hDlg, HSESSION hSession);
void 			propUpdateTitle(HSESSION hSession, HWND hDlg, LPTSTR pachOldName);

