/*
 * File:        open_msc.c - stuff for calling Common Open Dialog
 *
 * Copyright 1991 by Hilgraeve Inc. -- Monroe, MI
 * All rights reserved
 *
 * $Revision: 7 $
 * $Date: 9/24/99 5:01p $
 */

#include <windows.h>
#pragma hdrstop

// #define      DEBUGSTR        1
#include <commdlg.h>
#include <memory.h>
#include <stdlib.h>
#include <shlobj.h>
#include <tdll\stdtyp.h>
#include <term\res.h>
#include <tdll\mc.h>
#include <tdll\tdll.h>
#include <tdll\globals.h>
#include <tdll\file_msc.h>
#include <tdll\load_res.h>
#include <tdll\tchar.h>
#include <tdll\assert.h>
#include <tdll\misc.h>
#include "registry.h"
#include "open_msc.h"

static OPENFILENAME ofn;
static BROWSEINFO bi;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUEMENTS:
 *
 * RETURNS:
 */
// This function prototype changed from BOOL FAR PASCAL. -- REV 3-6-98.
UINT_PTR APIENTRY gnrcFindDirHookProc(HWND hdlg,
		    						  UINT msg,
			    					  WPARAM wPar,
				    				  LPARAM lPar)
	{
   	TCHAR   acMsg[64];
	WORD    windowID;

	windowID = LOWORD(wPar);

	switch (msg)
		{
		case WM_INITDIALOG:
			break;

		case WM_DESTROY:
			break;

		case WM_COMMAND:
			switch (windowID)
				{
				case IDOK:
            		LoadString(glblQueryDllHinst(),
			        	        40809,
				                acMsg,
				                sizeof(acMsg) / sizeof(TCHAR));
					SetDlgItemText(hdlg, edt1, acMsg);

					// EndDialog(hdlg, 1);
					break;

				case lst2:
					if (HIWORD(lPar) == LBN_DBLCLK)
						{
						SetFocus(GetDlgItem(hdlg, IDOK));
						PostMessage(hdlg,
									WM_COMMAND,
									IDOK,
									MAKELONG((INT_PTR)GetDlgItem(hdlg, IDOK),0));
						}
					break;

				default:
					break;
				}
			break;

		default:
			break;
		}

	return FALSE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUEMENTS:
 *
 * RETURNS:
 */
#if FALSE
BOOL FAR PASCAL gnrcFindFileHookProc(HWND hdlg,
									UINT msg,
									WPARAM wPar,
									LPARAM lPar)
	{
	WORD    windowID;

	windowID = LOWORD(wPar);

	switch (msg)
		{
		case WM_INITDIALOG:
			ofn.lCustData = 0;
			break;

		case WM_DESTROY:
			break;

		default:
			break;
		}

	return FALSE;
	}
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *      gnrcFindFileDialog
 *
 * DESCRIPTION:
 *      This function makes the FindFile common dialog a little bit easier to
 *      call.
 *
 * ARGUEMENTS:
 *      hwnd            -- the window handle to use as the parent
 *      pszTitle        -- text to display as the title
 *      pszDirectory    -- path to use as default directory
 *      pszMasks        -- file name masks
 *
 * RETURNS:
 *      A pointer to a string that contains the file name.  This string is malloced
 *      and must be freed by the caller, or..
 *      NULL which indicates the user canceled the operation.
 */

LPTSTR gnrcFindFileDialogInternal(HWND hwnd,
								LPCTSTR pszTitle,
								LPCTSTR pszDirectory,
								LPCTSTR pszMasks,
								int nFindFlag,
								LPCTSTR pszInitName)
	{
	int index;
	LPTSTR  pszRet = NULL;
	LPTSTR  pszStr;
	LPCTSTR pszWrk;
	TCHAR   acMask[128];
	TCHAR   acTitle[64];
	TCHAR   szExt[4];
	TCHAR   szFile[FNAME_LEN];
	TCHAR   szDir[FNAME_LEN];
	int     iRet;
    int     iSize;
	//DWORD   dwMaxComponentLength;
	//DWORD   dwFileSystemFlags;

	memset((LPTSTR)&ofn, 0, sizeof(ofn));
	memset(szFile, 0, sizeof(szFile));
	memset(szExt, 0, sizeof(szExt));
	memset(acMask, 0, sizeof(acMask));
	memset(acTitle, 0, sizeof(acTitle));

    ofn.lStructSize       = sizeof(OPENFILENAME);
	ofn.hwndOwner             = hwnd;
	ofn.hInstance             = (HANDLE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
	if ((pszMasks == NULL) || (StrCharGetStrLength(pszMasks) == 0))
		{
		resLoadFileMask(glblQueryDllHinst(),
						IDS_CMM_ALL_FILES1,
						1,
						acMask,
						sizeof(acMask) / sizeof(TCHAR));

		ofn.lpstrFilter = acMask;
		}
	else
		{
		ofn.lpstrFilter   = pszMasks;
		pszWrk = pszMasks;
		pszWrk = StrCharFindFirst(pszWrk, TEXT('.'));

		if (*pszWrk == '.')
			{
			pszWrk = StrCharNext(pszWrk);
			pszStr = (LPTSTR)pszWrk;
			index = 0;
			/* This works because we know how the mask are going to be formed */
			while ((index < 3) && (*pszWrk != ')'))
				{
				index += 1;
				pszWrk = StrCharNext(pszWrk);
				}
			if (pszWrk >= pszStr)
				MemCopy(szDir, pszStr, pszWrk - pszStr);
			}

		pszWrk = NULL;
		}

    iSize = StrCharGetByteCount(pszInitName);
    //MPT:10SEP98 if there is no name, just set the dest string to nothing
	if( iSize <= FNAME_LEN && pszInitName && iSize > 0)
		{
		MemCopy(szFile, pszInitName, iSize);
		}
    else
        {
        szFile[0] = TEXT('\0');
        }

	ofn.lpstrDefExt = (LPTSTR)szExt;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
	ofn.nFilterIndex          = 0;
	ofn.lpstrFile             = szFile;
	ofn.nMaxFile              = FNAME_LEN;

	if ((pszDirectory == NULL) || (StrCharGetStrLength(pszDirectory) == 0))
		{
	
#ifdef NT_EDITION
		// mpt:07-30-97
		if (IsNT())
#endif
			GetUserDirectory(szDir, sizeof(szDir) / sizeof(TCHAR));
#ifdef NT_EDITION
		else
			{
			TCHAR acDirTmp[FNAME_LEN];
			GetModuleFileName(glblQueryHinst(), acDirTmp, sizeof(acDirTmp));
			mscStripName(acDirTmp);
			}
#endif
		}

	else
		{
		StrCharCopy(szDir, pszDirectory);
		}

	pszStr = StrCharLast(szDir);
	if (*pszStr == TEXT('\\'))
		*pszStr = TEXT('\0');

	ofn.lpstrInitialDir   = szDir;
	ofn.lpstrFileTitle        = NULL;
	ofn.nMaxFileTitle         = 0;

	if ((pszTitle == NULL) || (StrCharGetByteCount(pszTitle) == 0))
		{
		// ofn.lpstrTitle         = "Select File";
		LoadString(glblQueryDllHinst(),
				IDS_CPF_SELECT_FILE,
				acTitle,
				sizeof(acTitle) / sizeof(TCHAR));

		ofn.lpstrTitle = acTitle;
		}

	else
		{
		ofn.lpstrTitle = pszTitle;
		}

	ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	//ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;

	if (nFindFlag)
		{
		ofn.Flags |= OFN_FILEMUSTEXIST;
		iRet = GetOpenFileName(&ofn);
		}

	else
		{
		#if 0
		// This was added so the common dialog for "Save As" applies
		// the same restrictions as the New Connect dialog when it
		// comes to saving session file names.
		//
		GetVolumeInformation(NULL, NULL, 0, NULL, &dwMaxComponentLength,
								 &dwFileSystemFlags, NULL, 0);

		if(dwMaxComponentLength == 255)
			{
			ofn.nMaxFile = dwMaxComponentLength - 1;
			}
		else
			{
			ofn.nMaxFile = 8;
			}
		#endif

		iRet = GetSaveFileName(&ofn);
		}


	if (iRet == TRUE)
		{
		if (StrCharGetStrLength(szFile) > 0)
			{
			pszRet = malloc(StrCharGetByteCount(szFile) + 1);

			if (pszRet != NULL)
				StrCharCopy(pszRet, szFile);
			}

		return pszRet;
		}

	else
		{
		return(NULL);
		}

	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
LPTSTR gnrcFindFileDialog(HWND hwnd,
						LPCTSTR pszTitle,
						LPCTSTR pszDirectory,
						LPCTSTR pszMasks)
	{
	return gnrcFindFileDialogInternal(hwnd,
									pszTitle,
									pszDirectory,
									pszMasks,
									TRUE,
									NULL);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
LPTSTR gnrcSaveFileDialog(HWND hwnd,
						LPCTSTR pszTitle,
						LPCTSTR pszDirectory,
						LPCTSTR pszMasks,
						LPCTSTR pszInitName)
	{
	return gnrcFindFileDialogInternal(hwnd,
									pszTitle,
									pszDirectory,
									pszMasks,
									FALSE,
									pszInitName);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  gnrcFindDirectoryDialog
 *
 * DESCRIPTION:
 *
 * ARGUEMENTS:
 *
 * RETURNS:
 */
LPTSTR gnrcFindDirectoryDialog(HWND hwnd, HSESSION hSession, LPTSTR pszDir)
	{
#ifndef INCL_USE_NEWFOLDERDLG
    BOOL bRet;
#else
    LPITEMIDLIST pidlRoot = 0, pidlSelected = 0;
    LPSHELLFOLDER lpDesktop = NULL;
    ULONG chEaten = 0;
#endif
	LPTSTR pszStr;
	TCHAR acTitle[64];
	TCHAR acList[64];
	TCHAR szDir[FNAME_LEN];
	TCHAR szFile[FNAME_LEN];

	LoadString(glblQueryDllHinst(),
			IDS_CMM_SEL_DIR,
			acTitle,
			sizeof(acTitle) / sizeof(TCHAR));

	resLoadFileMask(glblQueryDllHinst(),
				IDS_CMM_ALL_FILES1,
				1,
				acList,
				sizeof(acList) / sizeof(TCHAR));

	TCHAR_Fill(szFile, TEXT('\0'), sizeof(szFile) / sizeof(TCHAR));
	memset((LPTSTR)&ofn, 0, sizeof(ofn));

	if ((pszDir == NULL) || (StrCharGetStrLength(pszDir) == 0))
		{

		//changed to use working folder rather than current - mpt 8-18-99
		if ( !GetWorkingDirectory(szDir, FNAME_LEN) )
			{
			GetCurrentDirectory(FNAME_LEN, szDir);
			}
		}
	else
		{
		StrCharCopy(szDir, pszDir);
		}

	pszStr = StrCharLast(szDir);
	if (*pszStr == TEXT('\\'))
		*pszStr = TEXT('\0');

#ifndef INCL_USE_NEWFOLDERDLG
    ofn.lCustData         = 0L;
    ofn.lStructSize       = sizeof(OPENFILENAME);
	ofn.hwndOwner         = hwnd;
	ofn.hInstance         = (HANDLE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
	ofn.lpstrTitle        = acTitle;
	ofn.lpstrFilter       = acList;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
	ofn.nFilterIndex      = 0;
	ofn.lpstrFile         = szFile;
	ofn.nMaxFile          = sizeof(szFile);
	ofn.nFileOffset       = 0;
	ofn.nFileExtension    = 0;
	ofn.lpstrFileTitle    = acTitle;
	ofn.nMaxFileTitle     = sizeof(acTitle);
	ofn.lpstrDefExt       = NULL;
	// If OFN_ENABLEHOOK and/or OFN_ENABLETEMPLATE flags are set the call to
	// GetOpenFileName() will crash the application, then only a single drive
	// will appear in the "Drives:" dropdown list.  This is a bug in the 1691
	// build of Windows 98. -- REV 3-6-98.
	//
	ofn.Flags             = OFN_ENABLEHOOK | OFN_ENABLETEMPLATE |
							OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;

	ofn.lpfnHook          = gnrcFindDirHookProc;
	ofn.lpTemplateName    = MAKEINTRESOURCE(IDD_FINDDIRECTORY);

	ofn.lpstrInitialDir   = szDir;

	bRet = GetOpenFileName(&ofn);

    if (StrCharGetStrLength(szFile) == 0)
		return NULL;

	pszStr = StrCharFindLast(szFile, TEXT('\\'));
	if (*pszStr == TEXT('\\'))
		{
		pszStr = StrCharNext(pszStr);
		*pszStr = TEXT('\0');
		}

#else //INCL_USE_NEWFOLDERDLG

    //TODO:MPT Finish up the new browse for folder mechanism
    //         - Initialize pidlRoot with szDir
    //         - free pidl's when done
    //         - Use callback proc to verify the folder is writable

    CoInitialize( 0 );
    
    bi.pidlRoot = pidlRoot;
    bi.hwndOwner = hwnd;
    bi.pszDisplayName = szDir;
    bi.lpszTitle = acTitle;
    bi.ulFlags = BIF_RETURNONLYFSDIRS;
    bi.lpfn = NULL; //gnrcBrowseFolderHookProc;
    bi.lParam = 0;
    
    pidlSelected = SHBrowseForFolder( &bi );
    SHGetPathFromIDList( pidlSelected, szFile );

    if (StrCharGetStrLength(szFile) == 0)
		return NULL;

#endif	

	fileFinalizeDIR(hSession, szFile, szFile);

	pszStr = malloc(StrCharGetByteCount(szFile) + 1);
	StrCharCopy(pszStr, szFile);

	return pszStr;
	}

#ifdef INCL_USE_NEWFOLDERDLG
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  gnrcBrowseFolderHookProc
 *
 * DESCRIPTION:
 *
 * ARGUEMENTS:
 *
 * RETURNS:
 */
UINT_PTR CALLBACK gnrcBrowseFolderHookProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
    {
    return 0;
    }
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  GetUserDirectory
 *
 * DESCRIPTION:
 *  Returns the default HyperTerminal directory for the current user
 *
 * ARGUMENTS:
 *  pszUserDir  --  where to write the default directory
 *  dwSize      --  size, in characters, of the above buffer
 *
 * RETURNS:
 *  If the function succeeds, the return value is the number of characters
 *  stored into the buffer pointed to by pszDir, not including the
 *  terminating null character.
 *
 *  If the specified environment variable name was not found in the
 *  environment block for the current process, the return value is zero.
 *
 *  If the buffer pointed to by lpBuffer is not large enough, the return
 *  value is the buffer size, in characters, required to hold the value
 *  string and its terminating null character.
 *
 * Author:  JMH, 6-12-96
 */
DWORD GetUserDirectory(LPTSTR pszUserDir, DWORD dwSize)
    {
    DWORD   dwRet = MAX_PATH;
    TCHAR   szProfileDir[_MAX_PATH * sizeof(TCHAR)];
    TCHAR   szProfileDir1[_MAX_PATH * sizeof(TCHAR)];
    TCHAR   szProfileRoot[_MAX_PATH * sizeof(TCHAR)];


    szProfileRoot[0] = TEXT('\0');
    if ( regQueryValue(HKEY_CURRENT_USER,
                       TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"),
                       TEXT("Programs"),
                       szProfileRoot,
                       &dwRet) != 0 )
       {
        assert(0);
        return 0;
        }

    dwRet = MAX_PATH;
    szProfileDir[0] = TEXT('\0');

    if ( regQueryValue(HKEY_CURRENT_USER,
                       TEXT("SOFTWARE\\Hilgraeve Inc\\HyperTerminal PE\\3.0"),
                       TEXT("SessionsPath"),
                       szProfileDir,
                       &dwRet) != 0 )
        {
        dwRet = MAX_PATH;
        szProfileDir[0] = TEXT('\0');
        if ( regQueryValue(HKEY_CURRENT_USER,
                                           TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\GrpConv\\MapGroups"),
                                           TEXT("Communications"),
                                           szProfileDir,
                                           &dwRet) != 0 )
            {
            //mpt:12-16-98 If HyperTerminal was never installed by the operating system, this
            //             key is not present so we need to fake one.
            LoadString(glblQueryDllHinst(), IDS_GNRL_PROFILE_DIR, szProfileDir, sizeof(szProfileDir)/sizeof(TCHAR) );
            }

        szProfileDir1[0] = TEXT('\0');
        LoadString(glblQueryDllHinst(), IDS_GNRL_APPNAME, szProfileDir1, sizeof(szProfileDir)/sizeof(TCHAR) );

        if (*szProfileDir &&
            TEXT('\\') != szProfileDir[lstrlen(szProfileDir)-1])
            strcat( szProfileDir, TEXT("\\") );

        strcat( szProfileDir, szProfileDir1 );

        dwRet = lstrlen(szProfileRoot) + lstrlen(szProfileDir);
        if (dwRet + sizeof(TCHAR) < dwSize)
            {
            wsprintf(pszUserDir, TEXT("%s\\%s"), szProfileRoot, szProfileDir);
            }
        }
    else
        {
        dwRet = lstrlen(szProfileDir);
        if (dwRet + sizeof(TCHAR) < dwSize)
            {
            wsprintf(pszUserDir, TEXT("%s"), szProfileDir);
            }
        }

    return dwRet;
    }


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  CreateUserDirectory (NT_EDITION only)
 *
 * DESCRIPTION:
 *  For NT, if HT is installed after the os update, no directory
 *  will exist for HT in the user profile. This function creates the
 *  directory if necessary, since the rest of the program assumes
 *  it exists.
 *
 * ARGUMENTS:
 *  None.
 *
 * RETURNS:
 *  Nothing.
 *
 * Author:  JMH, 6-13-96
 */
void CreateUserDirectory(void)
    {
    TCHAR   szUserDir[_MAX_PATH];

    GetUserDirectory(szUserDir, sizeof(szUserDir) / sizeof(TCHAR));
    mscCreatePath(szUserDir);
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  IsNT
 *
 * DESCRIPTION: Determines if we are running under Windows NT
 *
 * ARGUMENTS:
 *  None.
 *
 * RETURNS:
 *  True if NT
 *
 * Author:  MPT 7-31-97
 */
BOOL IsNT(void)
	{

	OSVERSIONINFO stOsVersion;

	stOsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	if (GetVersionEx(&stOsVersion))
		{
		return ( stOsVersion.dwPlatformId == VER_PLATFORM_WIN32_NT );
		}

	return FALSE;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  IsNT
 *
 * DESCRIPTION: Determines if we are running under Windows NT
 *
 * ARGUMENTS:
 *  None.
 *
 * RETURNS:
 *  0 if user directory not specified
 *
 * Author:  MPT 8-18-99
 */
DWORD GetWorkingDirectory(LPTSTR pszUserDir, DWORD dwSize)
    {
    DWORD   dwRet = MAX_PATH;

    pszUserDir[0] = TEXT('\0');
    if ( regQueryValue(HKEY_CURRENT_USER,
                       TEXT("SOFTWARE\\Hilgraeve Inc\\HyperTerminal PE\\3.0"),
                       TEXT("WorkingPath"),
                       pszUserDir,
                       &dwRet) != 0 )
		{
        return 0;
		}

	return 1;
	}
