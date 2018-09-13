/******************************Module*Header*******************************\
* Module Name: dialog.c
*
* Dialog helper functions
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include "sscommon.h"

#define BUF_SIZE 30
static TCHAR  szSectName[BUF_SIZE];
static TCHAR  szItemName[BUF_SIZE];
static TCHAR  szFname[BUF_SIZE];
static TCHAR  szTmp[BUF_SIZE];
static HINSTANCE hInstance = 0;

/******************************Public*Routine******************************\
* ss_RegistrySetup
*
* Setup for registry access
*
\**************************************************************************/

BOOL ss_RegistrySetup( HINSTANCE hinst, int section, int file )
{
    if( LoadString(hInstance, section, szSectName, BUF_SIZE) &&
        LoadString(hInstance, file, szFname, BUF_SIZE) ) 
    {
        hInstance = hinst;
        return TRUE;
    }
    return FALSE;
}


/******************************Public*Routine******************************\
* ss_GetRegistryInt
*
* Retrieve integer value from registry
*
\**************************************************************************/

int  ss_GetRegistryInt( int name, int iDefault )
{
    if( LoadString( hInstance, name, szItemName, BUF_SIZE ) ) {
        return GetPrivateProfileInt(szSectName, szItemName, iDefault, szFname);
    }
    return 0;
}

/******************************Public*Routine******************************\
* ss_GetRegistryString
*
* Retrieve string from registry
*
\**************************************************************************/

void ss_GetRegistryString( int name, LPTSTR lpDefault, LPTSTR lpDest, 
                           int bufSize )
{
    if( LoadString( hInstance, name, szItemName, BUF_SIZE ) ) {
        GetPrivateProfileString(szSectName, szItemName, lpDefault, lpDest,
                                bufSize, szFname);
    }
}

/******************************Public*Routine******************************\
* ss_WriteRegistryInt
*
* Write integer value to registry
*
\**************************************************************************/

void ss_WriteRegistryInt( int name, int iVal )
{
    if( LoadString(hInstance, name, szItemName, BUF_SIZE) ) {
        wsprintf(szTmp, TEXT("%ld"), iVal);
        WritePrivateProfileString(szSectName, szItemName, szTmp, szFname);
    }
}

/******************************Public*Routine******************************\
* ss_WriteRegistryString
*
* Write string value to registry
*
\**************************************************************************/

void ss_WriteRegistryString( int name, LPTSTR lpString )
{
    if( LoadString(hInstance, name, szItemName, BUF_SIZE) ) {
        WritePrivateProfileString(szSectName, szItemName, lpString, szFname);
    }
}

/******************************Public*Routine******************************\
* GetTrackbarPos
*
* Get the current position of a common control trackbar
\**************************************************************************/

int
ss_GetTrackbarPos( HWND hDlg, int item )
{
    return 
       (int)SendDlgItemMessage( 
            hDlg, 
            item,
            TBM_GETPOS, 
            0,
            0
        );
}

/******************************Public*Routine******************************\
* SetupTrackbar
*
* Setup a common control trackbar
\**************************************************************************/

void
ss_SetupTrackbar( HWND hDlg, int item, int lo, int hi, int lineSize, 
                  int pageSize, int pos )
{
    SendDlgItemMessage( 
        hDlg, 
        item,
        TBM_SETRANGE, 
        (WPARAM) TRUE, 
        (LPARAM) MAKELONG( lo, hi )
    );
    SendDlgItemMessage( 
        hDlg, 
        item,
        TBM_SETPOS, 
        (WPARAM) TRUE, 
        (LPARAM) pos
    );
    SendDlgItemMessage( 
        hDlg, 
        item,
        TBM_SETPAGESIZE, 
        (WPARAM) 0,
        (LPARAM) pageSize 
    );
    SendDlgItemMessage( 
        hDlg, 
        item,
        TBM_SETLINESIZE, 
        (WPARAM) 0,
        (LPARAM) lineSize
    );
}
