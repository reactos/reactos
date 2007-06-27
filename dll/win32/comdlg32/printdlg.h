/*
 * COMMDLG - Print Dialog
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
 * Copyright 1999 Klaas van Gend
 * Copyright 2000 Huw D M Davies
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _WINE_PRINTDLG_H
#define _WINE_PRINTDLG_H

#include "cdlg.h"

/* This PRINTDLGA internal structure stores
 * pointers to several throughout useful structures.
 */

typedef struct
{
  LPDEVMODEA        lpDevMode;
  LPPRINTDLGA       lpPrintDlg;
  LPPRINTER_INFO_2A lpPrinterInfo;
  LPDRIVER_INFO_3A  lpDriverInfo;
  UINT              HelpMessageID;
  HICON             hCollateIcon;    /* PrintDlg only */
  HICON             hNoCollateIcon;  /* PrintDlg only */
  HICON             hPortraitIcon;   /* PrintSetupDlg only */
  HICON             hLandscapeIcon;  /* PrintSetupDlg only */
  HWND              hwndUpDown;
} PRINT_PTRA;

typedef struct
{
  LPDEVMODEW        lpDevMode;
  LPPRINTDLGW       lpPrintDlg;
  LPPRINTER_INFO_2W lpPrinterInfo;
  LPDRIVER_INFO_3W  lpDriverInfo;
  UINT              HelpMessageID;
  HICON             hCollateIcon;    /* PrintDlg only */
  HICON             hNoCollateIcon;  /* PrintDlg only */
  HICON             hPortraitIcon;   /* PrintSetupDlg only */
  HICON             hLandscapeIcon;  /* PrintSetupDlg only */
  HWND              hwndUpDown;
} PRINT_PTRW;

/* Debugging info */
static struct pd_flags {
  DWORD  flag;
  LPCSTR name;
} pd_flags[] = {
  {PD_SELECTION, "PD_SELECTION "},
  {PD_PAGENUMS, "PD_PAGENUMS "},
  {PD_NOSELECTION, "PD_NOSELECTION "},
  {PD_NOPAGENUMS, "PD_NOPAGENUMS "},
  {PD_COLLATE, "PD_COLLATE "},
  {PD_PRINTTOFILE, "PD_PRINTTOFILE "},
  {PD_PRINTSETUP, "PD_PRINTSETUP "},
  {PD_NOWARNING, "PD_NOWARNING "},
  {PD_RETURNDC, "PD_RETURNDC "},
  {PD_RETURNIC, "PD_RETURNIC "},
  {PD_RETURNDEFAULT, "PD_RETURNDEFAULT "},
  {PD_SHOWHELP, "PD_SHOWHELP "},
  {PD_ENABLEPRINTHOOK, "PD_ENABLEPRINTHOOK "},
  {PD_ENABLESETUPHOOK, "PD_ENABLESETUPHOOK "},
  {PD_ENABLEPRINTTEMPLATE, "PD_ENABLEPRINTTEMPLATE "},
  {PD_ENABLESETUPTEMPLATE, "PD_ENABLESETUPTEMPLATE "},
  {PD_ENABLEPRINTTEMPLATEHANDLE, "PD_ENABLEPRINTTEMPLATEHANDLE "},
  {PD_ENABLESETUPTEMPLATEHANDLE, "PD_ENABLESETUPTEMPLATEHANDLE "},
  {PD_USEDEVMODECOPIES, "PD_USEDEVMODECOPIES[ANDCOLLATE] "},
  {PD_DISABLEPRINTTOFILE, "PD_DISABLEPRINTTOFILE "},
  {PD_HIDEPRINTTOFILE, "PD_HIDEPRINTTOFILE "},
  {PD_NONETWORKBUTTON, "PD_NONETWORKBUTTON "},
  {-1, NULL}
};

/* Internal Functions
 * Do not Export to other applications or dlls
 */

INT PRINTDLG_SetUpPrinterListComboA(HWND hDlg, UINT id, LPCSTR name);
BOOL PRINTDLG_ChangePrinterA(HWND hDlg, char *name,
				   PRINT_PTRA *PrintStructures);
BOOL PRINTDLG_OpenDefaultPrinter(HANDLE *hprn);
LRESULT PRINTDLG_WMCommandA(HWND hDlg, WPARAM wParam,
			LPARAM lParam, PRINT_PTRA* PrintStructures);

#endif /* _WINE_PRINTDLG_H */
