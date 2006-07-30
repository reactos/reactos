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
* PROJECT:         ReactOS System Control Panel
* FILE:            lib/cpl/sysdm/virtmem.c
* PURPOSE:         pagefile settings
* PROGRAMMER:      Christian Wallukat cwallukat(at)gmx(dot)at
*/

#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <tchar.h>
#include "resource.h"
#include "sysdm.h"
#include <stdio.h>
#include <winternl.h>



//
// An array of these structs will interally manage system page files
//
typedef struct _sPagefiles
{
    WCHAR szDrive[10];
    INT   nMinValue;
    INT   nMaxValue;
    INT   nUsed;
} sPageFiles;



//
// At most there can be 32 drives, so only 32 page files
//
sPageFiles  g_sPagefile[32];
//
// This is the number of drives that could have page files
//
INT			g_nCount;
//
// Will save the user set action signal saving to the registry on exit
//
BOOL		g_bSave;


//
// ReadRegSettings reads the page file settings from the regisrty
//
VOID ReadRegSettings(HWND hwndDlg);
//
// ParseMemSettings takes the regisrty string and create the internal page file array
//
VOID ParseMemSettings(TCHAR *szSettings, HWND hwndDlg);
//
// SaveSettings writes the paging file array to the registry
//
VOID SaveSettings(HWND hwndDlg);

//
// Callback function for messages for the windows
//
INT_PTR 
CALLBACK 
VirtMemDlgProc(HWND   hwndDlg,
               UINT   uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    SYSTEM_PROCESS_INFORMATION  siSysPInfo;
    TCHAR						szTemp[255];
    INT							nIndex = 0;

    //
    // We dont need to look at this part of the message
    //
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            //
            // We dont save the changes until set is clicked
            //
            g_bSave  = FALSE;
            //
            // We start with no page files
            //
            g_nCount = 0;
            //
            // Clear out the file pagefile
            //
            g_sPagefile[0].nMaxValue = 0;
            g_sPagefile[0].nMinValue = 0;
            g_sPagefile[0].nUsed = 0;

            //
            // Grad the system stats to get some page file into
            //
            NtQuerySystemInformation(SystemProcessInformation, &siSysPInfo, sizeof(SYSTEM_PROCESS_INFORMATION), NULL); 

            //
            // Convert the pagefile usage to a string
            //
            _itow(siSysPInfo.PeakPagefileUsage, szTemp, 10);

            //
            // Set pagefle useage to its label
            //
            SendDlgItemMessage(hwndDlg, IDC_CURRENT, WM_SETTEXT, (WPARAM)0, (LPARAM)szTemp);

            //
            // Load the pagefile systems from the reg
            //
            ReadRegSettings(hwndDlg);
        };
        break;

    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                {
                    //
                    // If the user hits cancel we just close and dont save
                    //
                    EndDialog(hwndDlg, 0);
                    return TRUE;
                };
                break;


            case IDOK:
                {
                    //
                    // Check to make sure that the user hit set and we will save the settings
                    //
                    if(g_bSave == TRUE)
                    {
                        SaveSettings(hwndDlg);
                    };

                    //
                    // Close out the window
                    //
                    EndDialog(hwndDlg, 0);
                };
                break;


            case IDC_PAGEFILELIST:
                {
                    switch(HIWORD(wParam))
                    {
                    case LBN_SELCHANGE:
                        {
                            //
                            // Get the currently selected page file from the list box
                            //
                            nIndex = (INT)SendDlgItemMessage(hwndDlg, IDC_PAGEFILELIST, LB_GETCURSEL, (WPARAM)0, (LPARAM)0);

                            //
                            // If the index is in our range then process that page file
                            //
                            if(nIndex < g_nCount)
                            {
                                //
                                // Convert the minvalue for the page file
                                //
                                _itow(g_sPagefile[nIndex].nMinValue, szTemp, 10);

                                //
                                // Set the min value to its textbox
                                //
                                SendDlgItemMessage(hwndDlg, IDC_INITIALSIZE, WM_SETTEXT, (WPARAM)0, (LPARAM)szTemp);

                                //
                                // Convert the max value of the page file to a string
                                //
                                _itow(g_sPagefile[nIndex].nMaxValue, szTemp, 10);

                                //
                                // Set the max value to its textbox
                                //
                                SendDlgItemMessage(hwndDlg, IDC_MAXSIZE, WM_SETTEXT, (WPARAM)0, (LPARAM)szTemp);

                                //
                                // Figure out what type of page file it is
                                //
                                if(g_sPagefile[nIndex].nMinValue == 0 && g_sPagefile[nIndex].nMaxValue == 0)
                                {
                                    //
                                    // There is either no page file or a system managed page file so we disable custom sizes
                                    //
                                    EnableWindow(GetDlgItem(hwndDlg, IDC_MAXSIZE), FALSE);
                                    EnableWindow(GetDlgItem(hwndDlg, IDC_INITIALSIZE), FALSE);
                                    //
                                    // Check to see if that drive is used for paging
                                    //
                                    if(g_sPagefile[nIndex].nUsed == 1)
                                    {
                                        //
                                        // This is a system managed page file
                                        //
                                        SendDlgItemMessage(hwndDlg, IDC_NOPAGEFILE, BM_SETCHECK, (WPARAM)0, (LPARAM)0);
                                        SendDlgItemMessage(hwndDlg, IDC_SYSMANSIZE, BM_SETCHECK, (WPARAM)1, (LPARAM)0);
                                        SendDlgItemMessage(hwndDlg, IDC_CUSTOM,     BM_SETCHECK, (WPARAM)0, (LPARAM)0);
                                    }
                                    else
                                    {
                                        //
                                        // There is no page file on this drive
                                        //
                                        SendDlgItemMessage(hwndDlg, IDC_NOPAGEFILE, BM_SETCHECK, (WPARAM)1, (LPARAM)0);
                                        SendDlgItemMessage(hwndDlg, IDC_SYSMANSIZE, BM_SETCHECK, (WPARAM)0, (LPARAM)0);
                                        SendDlgItemMessage(hwndDlg, IDC_CUSTOM,     BM_SETCHECK, (WPARAM)0, (LPARAM)0);
                                    }
                                }
                                else
                                {
                                    //
                                    // THis is a custom sized page file
                                    //
                                    SendDlgItemMessage(hwndDlg, IDC_NOPAGEFILE, BM_SETCHECK, (WPARAM)0, (LPARAM)0);
                                    SendDlgItemMessage(hwndDlg, IDC_SYSMANSIZE, BM_SETCHECK, (WPARAM)0, (LPARAM)0);
                                    SendDlgItemMessage(hwndDlg, IDC_CUSTOM,     BM_SETCHECK, (WPARAM)1, (LPARAM)0);
                                    //
                                    // We need to allow the user to change the sizes
                                    //
                                    EnableWindow(GetDlgItem(hwndDlg, IDC_MAXSIZE), TRUE);
                                    EnableWindow(GetDlgItem(hwndDlg, IDC_INITIALSIZE), TRUE);
                                }
                            };
                        };
                        break;
                    };
                };
                break;


            case IDC_NOPAGEFILE:
                {
                    //
                    // Set the pagefile to no page file
                    //
                    SendDlgItemMessage(hwndDlg, IDC_NOPAGEFILE, BM_SETCHECK, (WPARAM)1, (LPARAM)0);
                    SendDlgItemMessage(hwndDlg, IDC_SYSMANSIZE, BM_SETCHECK, (WPARAM)0, (LPARAM)0);
                    SendDlgItemMessage(hwndDlg, IDC_CUSTOM,     BM_SETCHECK, (WPARAM)0, (LPARAM)0);

                    //
                    // Disable the page file custom size boxes
                    //
                    EnableWindow(GetDlgItem(hwndDlg, IDC_MAXSIZE), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_INITIALSIZE), FALSE);
                };
                break;

            case IDC_SYSMANSIZE:
                {
                    //
                    // User changed this to a system managed page file
                    //
                    SendDlgItemMessage(hwndDlg, IDC_NOPAGEFILE, BM_SETCHECK, (WPARAM)0, (LPARAM)0);
                    SendDlgItemMessage(hwndDlg, IDC_SYSMANSIZE, BM_SETCHECK, (WPARAM)1, (LPARAM)0);
                    SendDlgItemMessage(hwndDlg, IDC_CUSTOM,     BM_SETCHECK, (WPARAM)0, (LPARAM)0);

                    //
                    // Disable the page file custom size boxes
                    //
                    EnableWindow(GetDlgItem(hwndDlg, IDC_MAXSIZE), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_INITIALSIZE), FALSE);
                };
                break;

            case IDC_CUSTOM:
                {
                    //
                    // User changed this to a cutom sized page file
                    //
                    SendDlgItemMessage(hwndDlg, IDC_NOPAGEFILE, BM_SETCHECK, (WPARAM)0, (LPARAM)0);
                    SendDlgItemMessage(hwndDlg, IDC_SYSMANSIZE, BM_SETCHECK, (WPARAM)0, (LPARAM)0);
                    SendDlgItemMessage(hwndDlg, IDC_CUSTOM,     BM_SETCHECK, (WPARAM)1, (LPARAM)0);

                    //
                    // The user should be able to change the size of these files
                    //
                    EnableWindow(GetDlgItem(hwndDlg, IDC_MAXSIZE), TRUE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_INITIALSIZE), TRUE);
                };
                break;

            case IDC_SET:
                {
                    //
                    // The user hit set, so we are clear to save the settings when we are done
                    //
                    g_bSave = TRUE;

                    //
                    // Figure out which pagefile we are saving the settings too
                    //
                    nIndex  = SendDlgItemMessage(hwndDlg, IDC_PAGEFILELIST, LB_GETCURSEL, (WPARAM)0, (LPARAM)0);

                    //
                    // Make sure it isnt out of our range
                    //
                    if(nIndex < g_nCount)
                    {
                        //
                        // Check to see what the current user input settings are
                        //
                        if(SendDlgItemMessage(hwndDlg, IDC_CUSTOM, BM_GETCHECK, (WPARAM)0, (LPARAM)0) == BST_CHECKED)
                        {
                            //
                            // If custom is checked we need to get the min size
                            //
                            SendDlgItemMessage(hwndDlg, IDC_INITIALSIZE, WM_GETTEXT, (WPARAM)254, (LPARAM)szTemp);

                            //
                            // Convert the size to an int and save it
                            //
                            g_sPagefile[nIndex].nMinValue = _wtoi(szTemp);

                            //
                            // If custom is checked we need to get the max size
                            //
                            SendDlgItemMessage(hwndDlg, IDC_MAXSIZE, WM_GETTEXT, (WPARAM)254, (LPARAM)szTemp);

                            //
                            // If custom is checked we need to get the max size
                            //
                            g_sPagefile[nIndex].nMaxValue = _wtoi(szTemp);

                        }
                        else
                        {
                            //
                            // If it is no paging file or system then we set the sizes to zero
                            //
                            g_sPagefile[nIndex].nMinValue = g_sPagefile[nIndex].nMaxValue = 0;
                        }

                        //
                        // We check to see if this drive is used for a paging file anymore
                        //
                        g_sPagefile[nIndex].nUsed = (SendDlgItemMessage(hwndDlg, IDC_NOPAGEFILE, BM_GETCHECK, (WPARAM)0, (LPARAM)0) == BST_UNCHECKED);


                        //
                        // FIXME: Rebuild the paging list
                        //
                    };
                };
                break;
            };
        };
        break;

    case WM_NOTIFY:
        {
        };
        break;
    };

    return FALSE;
};


VOID
ReadRegSettings(HWND hwndDlg)
{
    HKEY  hk		 = NULL;
    DWORD dwType	 = 0x0;
    DWORD dwDataSize = 0x0;
    TCHAR  szValue[2048];


    //
    // Open or create this key to read our settings
    //
    if(RegCreateKeyEx(HKEY_LOCAL_MACHINE, 
        _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), 
        0x0, 
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE | KEY_READ, 
        NULL, 
        &hk, 
        NULL) == ERROR_SUCCESS)
    {

        //
        // Get the size of the data
        //
        if(RegQueryValueEx(hk, _T("PagingFiles"), 0, &dwType, (PBYTE)NULL, &dwDataSize) == ERROR_SUCCESS)
        {

            //
            // Read paging file settings from the regisrty
            //
            if(RegQueryValueEx(hk, _T("PagingFiles"), 0, &dwType, (PBYTE)(LPTSTR)szValue, &dwDataSize) == ERROR_SUCCESS)
            {
                //
                // Parse our settings and set up controls accordingly
                //
                ParseMemSettings(szValue, hwndDlg);					 
            }
            else
            {
                //
                // We had a problem :(
                //
                OutputDebugString(_T("Err read pagefile\n"));
            };
        }
        else
        {
            //
            // We had a problem :(
            //
            OutputDebugString(_T("Err open reg pagefile\n"));
        };

        //
        // Close our key
        //
        RegCloseKey(hk);
    };
};


VOID
ParseMemSettings(TCHAR *szSettings, HWND   hwndDlg)
{
    TCHAR szVolName[256];
    TCHAR szDrive[25];
    TCHAR szDrvLst[256];
    TCHAR szTemp[256];
    TCHAR szList[256];
    INT   nLastPos = 0;
    INT   nMinSize = 0;
    INT   nMaxSize = 0;
    INT   nTemp = 0;
    BOOL  bFound = FALSE;

    g_nCount = 0;

    //
    // Clear out all our buffers
    //
    _wcsnset(szTemp,    0, sizeof(szTemp));
    _wcsnset(szVolName, 0, sizeof(szVolName));
    _wcsnset(szDrive,   0, sizeof(szDrive));
    _wcsnset(szList,    0, sizeof(szList));

    //
    // Get a list of all the drives
    //
    GetLogicalDriveStrings(255, szDrvLst);

    //
    // Print out our drives and our reg string
    //
    OutputDebugString(szDrvLst);
    OutputDebugString((TCHAR*)szSettings);

    //
    // Loop through all the drives
    //
    while(TRUE)
    {
        //
        // Copy over the drive letter
        //
        wsprintf(szDrive, _T("%s"), szDrvLst + nLastPos);
        nLastPos += 4;

        //
        // We should fixed and removeable drives, no network / cd
        //
        if(GetDriveType(szDrive) == DRIVE_FIXED || GetDriveType(szDrive) == DRIVE_REMOVABLE)
        {
            //
            // Get some info about out drive
            //
            GetVolumeInformation(szDrive, (LPTSTR)szVolName, 255, NULL, NULL, NULL, NULL, 0);

            //
            // Cut off the Drive letter :
            //
            szDrive[2] = 0;

            //
            // Clear out our vars to prepare to walk page file setting string
            //
            nTemp  = 0;
            bFound = FALSE;

            //
            // Walk through the list of page files
            //
            while(TRUE)
            {	
                //
                // Copy over a page file setting for a drive skipping already processed ones
                //
                wsprintf(szTemp, _T("%s"), szSettings + nTemp);


                //
                // Check to see if the drive for the page file setting matches our current drive
                //
                if(!_tcsnicmp(szTemp,szDrive,1))
                {
                    //
                    // Extract the min/max size from the string
                    //
                    nMinSize = (INT)(wcsstr(szTemp + 16, _T(" ")) - (szTemp + 16)) + 1;
                    nMaxSize = _wtoi(szTemp + 16 + nMinSize);
                    nMinSize = _wtoi(szTemp + 16);

                    //
                    // Fill in the page file settings in the internal array 
                    //
                    g_sPagefile[g_nCount].nMaxValue = nMaxSize;
                    g_sPagefile[g_nCount].nMinValue = nMinSize;
                    g_sPagefile[g_nCount].nUsed		= 1;
                    wsprintf(g_sPagefile[g_nCount].szDrive, _T("%s"), szDrive);

                    //
                    // Add up one more page file found
                    //
                    ++g_nCount;

                    //
                    // Indicate that we found a pagefile for this drive
                    //
                    bFound = TRUE;

                    break;
                };

                //
                // Skip passed the just processed drive setting
                //
                nTemp += _tcslen(szSettings + nTemp) + 1;

                //
                // If we are done looking through the page file settings
                // or found a matching setting then break out
                //
                if((_tcslen(szSettings + nTemp) <= 0) || (bFound == TRUE))
                {
                    break;
                };
            };

            //
            // If we found a page file we make a string for the list box with the size
            //
            if(bFound == TRUE)
            {
                wsprintf(szList, _T("%s [%s]                        %i - %i"), szDrive, szVolName, nMinSize, nMaxSize);
            }
            else
            {
                //
                // Otherwise we make a blank pagefile element to show no page file
                //
                g_sPagefile[g_nCount].nMaxValue = 0;
                g_sPagefile[g_nCount].nMinValue = 0;
                g_sPagefile[g_nCount].nUsed		= 0;

                //
                // Copy over the drive for the blank page file element
                //
                wsprintf(g_sPagefile[g_nCount].szDrive, _T("%s"), szDrive);

                //
                // Add up one for the blank page file
                //
                ++g_nCount;

                //
                // We just show the name of the drive if there is no page file
                //
                wsprintf(szList, _T("%s [%s]"), szDrive, szVolName);
            };

            //
            // Add our string to the page file list
            //
            SendDlgItemMessage(hwndDlg, IDC_PAGEFILELIST, LB_ADDSTRING, (WPARAM)NULL, (LPARAM)szList);

            //
            // Print out the newly added string
            //
            OutputDebugString(szList);
        };

        //
        // Once all drive have finished being processed then we break out
        //
        if(_tcslen(szDrvLst + nLastPos) <= 0)
        {
            return;
        };
    };
};


void SaveSettings(HWND hwndDlg)
{
    HKEY  hk		 = NULL;
    TCHAR szValue[2048];
    TCHAR szTemp[512];
    INT   nCount     = 0;
    INT	  nPos		 = 0;

    //
    // Clear out our buffers
    //
    _wcsnset(szValue, 0, sizeof(szValue));
    _wcsnset(szTemp, 0, sizeof(szTemp));

    //
    // Go through all our internal pagefile records
    //
    for(nCount = 0; nCount < g_nCount; ++nCount)
    {
        //
        // We only save it to the regisrty if is used
        //
        if(g_sPagefile[nCount].nUsed == 1)
        {
            //
            // Clear out our temp buffer
            //
            _wcsnset(szTemp, 0, sizeof(szTemp));
            //
            // Copy the path and sizes to our temp buffer for the page file
            //
            wsprintf(szTemp, _T("%s\\pagefile.sys %i %i"), g_sPagefile[nCount].szDrive, g_sPagefile[nCount].nMinValue, g_sPagefile[nCount].nMaxValue);

            //
            // Add it to our overall regisrty string
            //
            _tcscat(szValue+nPos,szTemp);

            //
            // Record the positioon where the next string will be placed in the buffer
            //
            nPos             += _tcslen(szTemp) + 1;
            // Add our null chars in to seperate the page files(REG_MULTI_SZ string)
            szValue[nPos - 1] = 0;
            szValue[nPos]     = 0;
        };
    };       

    //
    // Now that we have page file setting string we open up the key to save it
    //
    if(RegCreateKeyEx(HKEY_LOCAL_MACHINE, 
        _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), 
        0x0, 
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS, 
        NULL, 
        &hk, 
        NULL) == ERROR_SUCCESS)
    {
        //
        // We save our page file settings to the PagingFiles value
        //
        if (RegSetValueEx(hk,
            _T("PagingFiles"),
            0, 
            REG_MULTI_SZ,
            (LPBYTE) szValue,
            (DWORD) (nPos+2)*sizeof(TCHAR)))
        {
            //
            // We had a problem, so print out a message
            //
            OutputDebugString(_T("Could not save"));

            //
            // Get the error for our last operation
            //
            _ltow(GetLastError(), szValue, 10);

            //
            // Print out the error code
            //
            OutputDebugString(szValue);

        }

        //
        // Close up the regisrty
        //
        RegCloseKey(hk);
    }
    else
    {
        //
        // Print out an error that we couldnt open(create) the registry key.
        //
        OutputDebugString(_T("Could not open key"));
    };
};


/* EOF */
