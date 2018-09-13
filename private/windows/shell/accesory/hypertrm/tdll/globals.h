/*	File: D:\WACKER\tdll\globals.h (Created: 26-Nov-1993)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:41p $
 */

#if !defined(INCL_GLOBALS)
#define INCL_GLOBALS

/* --- Functions to set and query global paramaters. --- */

LPTSTR    glblQueryHelpFileName(void);
void      glblSetHelpFileName(void);

HINSTANCE glblQueryHinst(void);
void	  glblSetHinst(const HINSTANCE hInst);

HINSTANCE glblQueryDllHinst(void);
void	  glblSetDllHinst(const HINSTANCE hInst);

void	  glblSetAccelHdl(const HACCEL hAccelerator);
HACCEL	  glblQueryAccelHdl(void);

void	  glblSetHwndFrame(const HWND hwnd);
HWND	  glblQueryHwndFrame(void);

int 	  glblAddModelessDlgHwnd(const HWND hwnd);
int 	  glblDeleteModelessDlgHwnd(const HWND hwnd);

int       glblQueryProgramStatus(void);
int       glblSetProgramStatus(int nStatus);

HWND	  glblQueryHwndBanner(void);
void	  glblSetHwndBanner(const HWND hwnd);

#endif
