//
//  MSDOS.C     Wizard started by SHELL.VxD
//
//  Copyright (C) Microsoft, 1994,1995 All Rights Reserved.
//
//  History:
//  3/20/95  [stevecat] - NT port & real clean up, unicode, etc.
//
//
#include "appwiz.h"
#include <wshioctl.h>


//
//  This exported entry point is called by RUNDLL32 when a VxD calls the
//  Shell_SuggestSingleMSDOSMode service.
//

void WINAPI SingleMSDOSWizard(HWND hwnd, HINSTANCE hAppInstance, LPTSTR lpszCmdLine, int nCmdShow)
{
    WIZDATA wd;

    memset(&wd, 0, sizeof(wd));

    wd.hwnd = hwnd;

    if (GetSingleAppInfo(&wd))
    {
        if (wd.hProps != 0)
        {
            MSDOSPropOnlyWizard(&wd);
        }
        else
        {
            wd.dwFlags |= WDFLAG_SINGLEAPP;
            LinkWizard(&wd);
        }
    }

    //
    //    Note, we close properties here in case GetSingleAppInfo returned false,
    //    but still had opened the hProps.
    //

    if (wd.hProps != 0)
    {
        PifMgr_CloseProperties(wd.hProps, CLOSEPROPS_NONE);
    }
}


//
//  Inline helper function.  If the name ends with the extension .EXE, .BAT,
//  or .COM then we'll accept it.  Otherwise, try to take the given name and
//  convert it to a program name we can execute.  This fixes the case where
//  Flight Simulator's setup program runs FS5.OVL.  The user should really
//  run FS5.BAT.
//

BOOL _inline CleanUpName(LPWIZDATA lpwd)
{
    LPCTSTR PathDirs[] = {lpwd->szWorkingDir, NULL};
    LPTSTR  pszExt = PathFindExtension(lpwd->szExeName);
    LPTSTR  pszCurExt;
    TCHAR   szValidExt[100];

    if (*pszExt)
        pszExt++;    // NSL: *pszExt == '.' so this should be fine

    //
    //    Make sure that the extension is a real progarm extension (not .OVL
    //    for example.  If it is, search for the matching name.
    //

    LoadAndStrip(IDS_EXTENSIONS, szValidExt, ARRAYSIZE(szValidExt));

    pszCurExt = szValidExt;

    while (*pszCurExt)
    {
        if (lstrcmpi(pszExt, pszCurExt) == 0)
        {
            return PathResolve(lpwd->szExeName, PathDirs, PRF_VERIFYEXISTS | PRF_TRYPROGRAMEXTENSIONS);
        }

        pszCurExt = SkipStr(pszCurExt);
    }

    pszCurExt = szValidExt;

    while (*pszCurExt)
    {
        lstrcpy(pszExt, pszCurExt);

        if (PathResolve(lpwd->szExeName, PathDirs, PRF_VERIFYEXISTS | PRF_TRYPROGRAMEXTENSIONS))
        {
            return TRUE;
        }

        pszCurExt = SkipStr(pszCurExt);
    }
    return FALSE;
}


BOOL GetSingleAppInfo(LPWIZDATA lpwd)
{
    HANDLE  hDevice;
    BOOL    bMakeLink = FALSE;
    SINGLEAPPSTRUC SAS;
    DWORD   dwRetSize;
    LPDWORD lpResult;

    hDevice = CreateFile(SHELLFILENAME, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (hDevice == INVALID_HANDLE_VALUE)
    {
        return(FALSE);
    }

    if (DeviceIoControl(hDevice, WSHIOCTL_GET1APPINFO, NULL, 0,
                        &SAS, sizeof(SAS), &dwRetSize, NULL))
    {

        #define fVMDead ((BOOL)(SAS.SSA_dwFlags & SSAMFLAG_KILLVM))

        int idString = fVMDead ? IDS_VMCLOSED : IDS_VMSTILLALIVE;

        WIZERROR(TEXT("Got info on a single MS-DOS mode app.  Now will show Exe, Command line params, directory, and PIF."));
        WIZERROR(SAS.SSA_ProgName);
        WIZERROR(SAS.SSA_CommandLine);
        WIZERROR(SAS.SSA_CurDir);
        WIZERROR(SAS.SSA_PIFPath);
    
        if (SAS.SSA_dwFlags & SSAMFLAG_FROMREGLIST)
        {
            SHELLEXECUTEINFO ei;

            lpResult = (LPVOID)SAS.SSA_ResultPtr;
            *lpResult = SSR_KILLAPP;

            DeviceIoControl(hDevice, WSHIOCTL_SIGNALSEM, &SAS, sizeof(SAS),
                            NULL, 0, NULL, NULL);
    
            ei.cbSize = sizeof(ei);
            ei.hwnd = lpwd->hwnd;
            ei.lpVerb = NULL;
            ei.fMask = 0;
            ei.lpFile = SAS.SSA_ProgName;

            if (SAS.SSA_CommandLine[0] == 0)
            {
                ei.lpParameters = NULL;
            }
            else
            {
                ei.lpParameters = SAS.SSA_CommandLine;
            }

            if (SAS.SSA_CurDir == TEXT('\0'))
            {
                ei.lpDirectory = NULL;
            }
            else
            {
                ei.lpDirectory = SAS.SSA_CurDir;
            }

            ei.lpClass = NULL;
            ei.nShow = SW_SHOWDEFAULT;
            ei.hInstApp = hInstance;
    
            ShellExecuteEx(&ei);
            goto CloseHandleExit;
        }
    
    
        if (SAS.SSA_dwFlags & SSAMFLAG_REQREALMODE)
        {
            lpwd->dwFlags |= WDFLAG_REALMODEONLY;
        }
    
        lstrcpyn(lpwd->szExeName, SAS.SSA_ProgName,    ARRAYSIZE(lpwd->szExeName));
        lstrcpyn(lpwd->szParams,  SAS.SSA_CommandLine, ARRAYSIZE(lpwd->szParams));
        lstrcpyn(lpwd->szWorkingDir, SAS.SSA_CurDir,   ARRAYSIZE(lpwd->szWorkingDir));
    
    
        if (CleanUpName(lpwd))
        {
            DetermineExeType(lpwd);
            lpwd->dwFlags |= WDFLAG_NOBROWSEPAGE;

            //
            // There are three different possibilities with SSA_PIFPath:
            //        Empty string indicates program run from command line.
            //        " " indicates exe run from shell, but no PIF exists
            //        Name of PIF that was run.
            // If the program was run from command.com then we will force
            // the user to create a shortcut.  If not started from a specific
            // PIF then we'll set the default properties for the program.  If
            // started from a specific PIF then we'll set the properties for
            // that PIF.
            //

            if (SAS.SSA_PIFPath[0] != 0)
            {
                lpwd->hProps = PifMgr_OpenProperties((SAS.SSA_PIFPath[0] == TEXT(' ')) ?
                                    lpwd->szExeName : SAS.SSA_PIFPath,
                                    NULL, 0, OPENPROPS_NONE);

                if (lpwd->hProps != 0)
                {
                    idString = fVMDead ? IDS_CHGPROPCLOSED
                                       : IDS_CHGPROPSTILLALIVE;
                }
            }
        }
        else
        {
            LPTSTR pszStartName = PathFindFileName(lpwd->szExeName);

            *pszStartName = 0;

            LoadStringA(hInstance, IDS_GENERICNAME,
                       lpwd->PropPrg.achTitle, ARRAYSIZE(lpwd->PropPrg.achTitle));
        }
    
        bMakeLink = (IDYES == ShellMessageBox(hInstance,
                                    lpwd->hwnd,
                                    MAKEINTRESOURCE(idString),
                                    MAKEINTRESOURCE(IDS_1APPWARNTITLE),
                                    MB_YESNO | MB_DEFBUTTON1 | MB_ICONEXCLAMATION,
                                    lpwd->PropPrg.achTitle));
    
        lpResult = (LPVOID)SAS.SSA_ResultPtr;

        if (!fVMDead && lpResult != NULL)
        {
            if (bMakeLink)
            {
                *lpResult = (SAS.SSA_dwFlags & SSAMFLAG_FROMREGLIST) ?
                                SSR_KILLAPP : SSR_CLOSEVM;
            }
            else
            {
                *lpResult = SSR_CONTINUE;
            }
    
            DeviceIoControl(hDevice, WSHIOCTL_SIGNALSEM, &SAS, sizeof(SAS),
                            NULL, 0, NULL, NULL);
        }
    }

CloseHandleExit:

    CloseHandle(hDevice);

    return(bMakeLink);

    #undef fVMDead
}

