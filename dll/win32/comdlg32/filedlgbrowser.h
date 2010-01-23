/*
 *  Implementation of IShellBrowser for the File Open common dialog
 *
 * Copyright 1999 Francois Boisvert
 * Copyright 1999, 2000 Juergen Schmied
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

#ifndef SHBROWSER_H
#define SHBROWSER_H

#ifndef RC_INVOKED
#include <stdarg.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "shlobj.h"
#include "objbase.h"
#include "commdlg.h"

/***********************************************************************
 * Defines and global variables
 */

/* dialog internal property */

#define FODPROP_SAVEDLG 0x0001  /* File dialog is a Save file dialog */
#define FODPROP_USEVIEW 0x0002  /* Indicates the user selection must be taken
				   from the IShellView */

/***********************************************************************
 * Data structure
 */


typedef struct
{
    LPOPENFILENAMEW ofnInfos;
    BOOL unicode;
    LPWSTR initdir;
    LPWSTR filename;
    LPCWSTR title;
    LPCWSTR defext;
    LPCWSTR filter;
    LPCWSTR customfilter;
    SIZE sizedlg; /* remember the size of the dialog */
    POINT initial_size; /* remember the initial size of the dialog */
    struct {
        IShellBrowser *FOIShellBrowser;
        IShellFolder *FOIShellFolder;
        IShellView *FOIShellView;
	IDataObject *FOIDataObject;
    } Shell;

    struct {
        HWND hwndOwner;
        HWND hwndView;
        FOLDERSETTINGS folderSettings;
        LPITEMIDLIST pidlAbsCurrent;
        LPWSTR lpstrCurrentFilter;
    } ShellInfos;

    struct {
        HWND hwndFileTypeCB;
        HWND hwndLookInCB;
        HWND hwndFileName;
	HWND hwndTB;
	HWND hwndGrip;
        HWND hwndCustomDlg;
	DWORD dwDlgProp;
    } DlgInfos;

    struct {
	UINT fileokstring;
	UINT lbselchstring;
	UINT helpmsgstring;
	UINT sharevistring;
    } HookMsg;

} FileOpenDlgInfos;

/***********************************************************************
 * Control ID's
 */
#define IDS_ABOUTBOX                    101
#define IDS_DOCUMENTFOLDERS             102
#define IDS_PERSONAL                    103
#define IDS_FAVORITES                   104
#define IDS_PATH                        105
#define IDS_DESKTOP                     106

#define IDS_FONTS                       108
#define IDS_MYCOMPUTER                  110
#define IDS_SYSTEMFOLDERS               112
#define IDS_LOCALHARDRIVES              113
#define IDS_FILENOTFOUND                114
#define IDS_VERIFYFILE                  115
#define IDS_CREATEFILE                  116
#define IDS_CREATEFOLDER_DENIED         117
#define IDS_FILEOPEN_CAPTION            118
#define IDS_OVERWRITEFILE		119
#define IDS_INVALID_FILENAME_TITLE	120
#define IDS_INVALID_FILENAME		121
#define IDS_PATHNOTEXISTING		122
#define IDS_FILENOTEXISTING		123

/* File Dialog Tooltips string IDs */

#define IDS_UPFOLDER                    150
#define IDS_NEWFOLDER                   151
#define IDS_LISTVIEW                    152
#define IDS_REPORTVIEW                  153
#define IDS_TODESKTOP                   154

#define IDC_OPENREADONLY                chx1

#define IDC_TOOLBARSTATIC		stc1
#define IDC_FILETYPESTATIC              stc2
#define IDC_FILENAMESTATIC              stc3
#define IDC_LOOKINSTATIC                stc4

#define IDC_SHELLSTATIC                 lst1

#define IDC_FILETYPE                    cmb1
#define IDC_LOOKIN                      cmb2

#define IDC_FILENAME                    edt1

#define IDC_TOOLBAR			1

/***********************************************************************
 * Prototypes for the methods of the IShellBrowserImpl class
 */
/* Constructor */
IShellBrowser * IShellBrowserImpl_Construct(HWND hwndOwner);


LPITEMIDLIST GetPidlFromDataObject ( IDataObject *doSelected, UINT nPidlIndex);

/* Functions used by the EDIT box */
void FILEDLG95_FILENAME_FillFromSelection (HWND hwnd);

/**************************************************************************
*   External Prototypes
*/
extern const char FileOpenDlgInfosStr[];

extern IShellFolder*    GetShellFolderFromPidl(LPITEMIDLIST pidlAbs);
extern LPITEMIDLIST     GetParentPidl(LPITEMIDLIST pidl);

extern int     FILEDLG95_LOOKIN_SelectItem(HWND hwnd,LPITEMIDLIST pidl);
extern LRESULT SendCustomDlgNotificationMessage(HWND hwndParentDlg, UINT uCode);

#endif /*SHBROWSER_H*/
