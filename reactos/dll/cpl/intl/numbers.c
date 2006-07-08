/*
 *  ReactOS
 *  Copyright (C) 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * PROJECT:         ReactOS International Control Panel
 * FILE:            lib/cpl/intl/numbers.c
 * PURPOSE:         Numbers property page
 * PROGRAMMER:      Eric Kohl
 */

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <stdio.h>

#include "intl.h"
#include "resource.h"

//TODO no saving has been implamented at this time, needs to be done
//changing for example, the list sperator will not add it to the dropdown list and should
//the dropdown that contain certian strings are not formated acording to the LOCALE settings

static VOID
UpdateNumbersSample(HWND hWnd)
{
  WCHAR SampleNumber[13];
  WCHAR OutBuffer[25];

  wcscpy(SampleNumber,L"123456789.00");
  GetNumberFormat(LOCALE_USER_DEFAULT,0,SampleNumber,NULL,OutBuffer,sizeof(OutBuffer));
  SendMessageW(GetDlgItem(hWnd,IDC_POSITIVESAMPLE), WM_SETTEXT, 0, (LPARAM)OutBuffer);

  wcscpy(SampleNumber,L"-123456789.00");
  GetNumberFormat(LOCALE_USER_DEFAULT,0,SampleNumber,NULL,OutBuffer,sizeof(OutBuffer));
  SendMessageW(GetDlgItem(hWnd,IDC_NEGITIVESAMPLE), WM_SETTEXT, 0, (LPARAM)OutBuffer);
}


/* Property page dialog callback */
INT_PTR CALLBACK
NumbersPageProc(HWND hwndDlg,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
    {
    WCHAR Buffer[80];
      UpdateNumbersSample(hwndDlg);

    //get decimal symbol
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, Buffer, 80);
    SendMessageW(GetDlgItem(hwndDlg,IDC_DECSYMBOL), CB_ADDSTRING, 0, (LPARAM)Buffer);
    SendMessageW(GetDlgItem(hwndDlg,IDC_DECSYMBOL), CB_SETCURSEL, 0, 0);

    int iindex; //uses in geting item indexes

    //number of decimal places
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IDIGITS, Buffer, 80);
    int i;
    int *countnum;
    int iPlus;
    for(i=0;i<10;i++)
    {
    iPlus = i + 48;
    countnum = &iPlus;
    SendMessageW(GetDlgItem(hwndDlg,IDC_DECDIGITS), CB_ADDSTRING, 0, (LPARAM)countnum);
    }
    iindex = (int)Buffer[0];
    iindex = iindex - 48;

    SendMessageW(GetDlgItem(hwndDlg,IDC_DECDIGITS), CB_SETCURSEL, iindex, 0);

    //digit grouping Symbol
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, Buffer, 80);
    SendMessageW(GetDlgItem(hwndDlg,IDC_DIGGROUPSYMBOL), CB_ADDSTRING, 0, (LPARAM)Buffer);
    SendMessageW(GetDlgItem(hwndDlg,IDC_DIGGROUPSYMBOL), CB_SETCURSEL, 0, 0);

    //digit grouping
    //possably this should accept any setting not just these 3 (what if the regestery is manualy edited?)
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, Buffer, 80);
    SendMessageW(GetDlgItem(hwndDlg,IDC_DIGGROUP), CB_ADDSTRING, 0, (LPARAM)L"12345678");
    SendMessageW(GetDlgItem(hwndDlg,IDC_DIGGROUP), CB_ADDSTRING, 0, (LPARAM)L"12,345,678");
    SendMessageW(GetDlgItem(hwndDlg,IDC_DIGGROUP), CB_ADDSTRING, 0, (LPARAM)L"1,23,45,678");
    if(wcsncmp(Buffer,L"3;2;0",6)) {}else
    {
        SendMessageW(GetDlgItem(hwndDlg,IDC_DIGGROUP), CB_SETCURSEL, 0, 0);
    }

    if(wcsncmp(Buffer,L"3;0",6)) {}else
    {
        SendMessageW(GetDlgItem(hwndDlg,IDC_DIGGROUP), CB_SETCURSEL, 1, 0);
    }

    if(wcsncmp(Buffer,L"0;0",6)) {}else
    {
        SendMessageW(GetDlgItem(hwndDlg,IDC_DIGGROUP), CB_SETCURSEL, 2, 0);
    }

    //Negitive sign
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SNEGATIVESIGN, Buffer, 80);
    SendMessageW(GetDlgItem(hwndDlg,IDC_NEGSIGN), CB_ADDSTRING, 0, (LPARAM)Buffer);
    SendMessageW(GetDlgItem(hwndDlg,IDC_NEGSIGN), CB_SETCURSEL, 0, 0);

    //Negitive number format
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_INEGSIGNPOSN, Buffer, 80);
    //SendMessageW(GetDlgItem(hwndDlg,IDC_NEGFORMAT), CB_ADDSTRING, 0, (LPARAM)Buffer);
    SendMessageW(GetDlgItem(hwndDlg,IDC_NEGFORMAT), CB_ADDSTRING, 0, (LPARAM)L"(1.1)");
    SendMessageW(GetDlgItem(hwndDlg,IDC_NEGFORMAT), CB_ADDSTRING, 0, (LPARAM)L"- 1.1");
    SendMessageW(GetDlgItem(hwndDlg,IDC_NEGFORMAT), CB_ADDSTRING, 0, (LPARAM)L"1.1 -");
    SendMessageW(GetDlgItem(hwndDlg,IDC_NEGFORMAT), CB_ADDSTRING, 0, (LPARAM)L"-1.1");
    SendMessageW(GetDlgItem(hwndDlg,IDC_NEGFORMAT), CB_ADDSTRING, 0, (LPARAM)L"1.1-");
    iindex = (int)Buffer[0];
    iindex = iindex - 48;
    SendMessageW(GetDlgItem(hwndDlg,IDC_NEGFORMAT), CB_SETCURSEL, iindex, 0);

    //Leading zeros
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_ILZERO, Buffer, 80);
    SendMessageW(GetDlgItem(hwndDlg,IDC_LEADZEROS), CB_ADDSTRING, 0, (LPARAM)L".7");
    SendMessageW(GetDlgItem(hwndDlg,IDC_LEADZEROS), CB_ADDSTRING, 0, (LPARAM)L"0.7");
    iindex = (int)Buffer[0];
    iindex = iindex - 48;
    SendMessageW(GetDlgItem(hwndDlg,IDC_LEADZEROS), CB_SETCURSEL, iindex, 0);


    //List seperator
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SLIST, Buffer, 80);
    SendMessageW(GetDlgItem(hwndDlg,IDC_LISTSEPERATOR), CB_ADDSTRING, 0, (LPARAM)Buffer);
    SendMessageW(GetDlgItem(hwndDlg,IDC_LISTSEPERATOR), CB_SETCURSEL, 0, 0);

    //Measurement system
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IMEASURE	, Buffer, 80);
    SendMessageW(GetDlgItem(hwndDlg,IDC_MEASUREMENTSYS), CB_ADDSTRING, 0, (LPARAM)L"Metric");  //hardcoded english strings
    SendMessageW(GetDlgItem(hwndDlg,IDC_MEASUREMENTSYS), CB_ADDSTRING, 0, (LPARAM)L"US");
    iindex = (int)Buffer[0];
    iindex = iindex - 48;
    SendMessageW(GetDlgItem(hwndDlg,IDC_MEASUREMENTSYS), CB_SETCURSEL, iindex, 0);  //this line doesn't work

    //Digit substatution -
    //LOCALE_IDIGITSUBSTITUTION is undefined, to be done elseware?
    //hardcoded english strings should also be removed
    // GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IDIGITSUBSTITUTION, Buffer, 80);
    // SendMessageW(GetDlgItem(hwndDlg,IDC_DIGSUBSTAT), CB_ADDSTRING, 0, (LPARAM)L"Context");
    // SendMessageW(GetDlgItem(hwndDlg,IDC_DIGSUBSTAT), CB_ADDSTRING, 0, (LPARAM)L"None");
    // SendMessageW(GetDlgItem(hwndDlg,IDC_DIGSUBSTAT), CB_ADDSTRING, 0, (LPARAM)L"National");
    // iindex = (int)Buffer[0];
    // iindex = iindex - 48;
    // SendMessageW(GetDlgItem(hwndDlg,IDC_DIGSUBSTAT), CB_SETCURSEL, iindex, 0);
    //the next 2 lines should be removed when LOCALE_IDIGITSUBSTITUTION is avalable
    SendMessageW(GetDlgItem(hwndDlg,IDC_DIGSUBSTAT), CB_ADDSTRING, 0, (LPARAM)L"Not yet avalable");
    SendMessageW(GetDlgItem(hwndDlg,IDC_DIGSUBSTAT), CB_SETCURSEL, 0, 0);


    //todo write data

    }
    case WM_COMMAND:
    {
        //UpdateNumbersSample(GetDlgItem(hwndDlg, IDC_POSITIVESAMPLE));
    }
    case WM_NOTIFY:
    {
        //UpdateNumbersSample(GetDlgItem(hwndDlg, IDC_POSITIVESAMPLE));
    }
  }
  return FALSE;
}

/* EOF */
