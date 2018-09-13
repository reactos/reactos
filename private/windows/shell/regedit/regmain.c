/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGMAIN.C
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        05 Mar 1994
*
*******************************************************************************/

#include "pch.h"
#include <regstr.h>
#include "regedit.h"
#include "regfile.h"
#include "regbined.h"
#include "regresid.h"

//  Instance handle of this application.
HINSTANCE g_hInstance;

//  TRUE if accelerator table should not be used, such as during a rename
//  operation.
BOOL g_fDisableAccelerators = FALSE;

TCHAR g_KeyNameBuffer[MAXKEYNAME];
TCHAR g_ValueNameBuffer[MAXVALUENAME_LENGTH];
BYTE g_ValueDataBuffer[MAXDATA_LENGTH * sizeof(TCHAR)];

COLORREF g_clrWindow;
COLORREF g_clrWindowText;
COLORREF g_clrHighlight;
COLORREF g_clrHighlightText;

PTSTR g_pHelpFileName;

TCHAR g_NullString[] = TEXT("");

#define PARSERET_CONTINUE               0
#define PARSERET_REFRESH                1
#define PARSERET_EXIT                   2

UINT
PASCAL
ParseCommandLine(
    VOID
    );

BOOL
PASCAL
IsRegistryToolDisabled(
    VOID
    );

int
PASCAL
ModuleEntry(
    VOID
    )
{

    HWND hRegEditWnd;
    HWND hPopupWnd;
    HACCEL hRegEditAccel;
    MSG Msg;
    USHORT wLanguageId = LANGIDFROMLCID(GetThreadLocale());

    g_hInstance = GetModuleHandle(NULL);

    hRegEditWnd = FindWindow(g_RegEditClassName, NULL);

    //
    //  Check if we were given a commandline and handle if appropriate.
    //

    switch (ParseCommandLine()) {

        case PARSERET_REFRESH:
            if (hRegEditWnd != NULL)
                PostMessage(hRegEditWnd, WM_COMMAND, ID_REFRESH, 0);
            //  FALL THROUGH

        case PARSERET_EXIT:
            goto ModuleExit;

    }

    //
    //  Allow only one instance of the Registry Editor.
    //

    if (hRegEditWnd != NULL) {

        if (IsIconic(hRegEditWnd))
            ShowWindow(hRegEditWnd, SW_RESTORE);

        else {

            BringWindowToTop(hRegEditWnd);

            if ((hPopupWnd = GetLastActivePopup(hRegEditWnd)) != hRegEditWnd)
                BringWindowToTop(hPopupWnd);

            SetForegroundWindow(hPopupWnd);

        }

        goto ModuleExit;

    }

    //
    //  At this point, we're about to bring up an instance of the Registry
    //  Editor window.  To prevent users from corrupting their registries,
    //  administrators can set a policy switch to prevent editing.  Check that
    //  switch now.
    //

    if (IsRegistryToolDisabled()) {

        InternalMessageBox(g_hInstance, NULL, MAKEINTRESOURCE(IDS_REGEDITDISABLED),
            MAKEINTRESOURCE(IDS_REGEDIT), MB_ICONERROR | MB_OK);

        goto ModuleExit;

    }

    //
    //  Initialize and create an instance of the Registry Editor window.
    //

    if ((g_pHelpFileName = LoadDynamicString(IDS_HELPFILENAME)) == NULL)
        goto ModuleExit;

    if (!RegisterRegEditClass() || !RegisterHexEditClass())
        goto ModuleExit;

    if ((hRegEditAccel = LoadAccelerators(g_hInstance,
        MAKEINTRESOURCE(IDACCEL_REGEDIT))) == NULL)
        goto ModuleExit;

    if ((hRegEditWnd = CreateRegEditWnd()) != NULL) {

        while (GetMessage(&Msg, NULL, 0, 0)) {

            if (g_fDisableAccelerators || !TranslateAccelerator(hRegEditWnd,
                hRegEditAccel, &Msg)) {

                TranslateMessage(&Msg);
                DispatchMessage(&Msg);

            }

        }

    }

ModuleExit:
    ExitProcess(0);

    return 0;

}

/*******************************************************************************
*
*  ParseCommandline
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     (returns), TRUE to continuing loading, else FALSE to stop immediately.
*
*******************************************************************************/

UINT
PASCAL
ParseCommandLine(
    VOID
    )
{

    BOOL fSilentMode;
    BOOL fExportMode;
    BOOL fSaveInAnsiRegedit4Format;
    LPTSTR lpCmdLine;
    LPTSTR lpFileName;
    LPTSTR lpSelectedPath;

    fSilentMode = FALSE;
    fExportMode = FALSE;

    lpCmdLine = GetCommandLine();

    //
    //  Skip past the application pathname.  Be sure to handle long filenames
    //  correctly.
    //

    if (*lpCmdLine == TEXT('\"')) {

        do
            lpCmdLine = CharNext(lpCmdLine);
        while (*lpCmdLine != 0 && *lpCmdLine != TEXT('\"'));

        if (*lpCmdLine == TEXT('\"'))
            lpCmdLine = CharNext(lpCmdLine);

    }

    else {

        while (*lpCmdLine > TEXT(' '))
            lpCmdLine = CharNext(lpCmdLine);

    }

    while (*lpCmdLine != 0 && *lpCmdLine <= TEXT(' '))
        lpCmdLine = CharNext(lpCmdLine);

    while (TRUE) {

        while (*lpCmdLine == TEXT(' '))
            lpCmdLine = CharNext(lpCmdLine);

        if (*lpCmdLine != TEXT('/') && *lpCmdLine != TEXT('-'))
            break;

        lpCmdLine = CharNext(lpCmdLine);

        while (*lpCmdLine != 0 && *lpCmdLine != TEXT(' ')) {

            switch (*lpCmdLine) {

                    //
                    //  Specifies the location of the SYSTEM.DAT and USER.DAT
                    //  files in real-mode.  We don't use these switches, but
                    //  we do need to bump past the filename.
                    //
                case TEXT('l'):
                case TEXT('L'):
                case TEXT('r'):
                case TEXT('R'):
                    return PARSERET_EXIT;

                case TEXT('e'):
                case TEXT('E'):
                    fExportMode = TRUE;
                    fSaveInAnsiRegedit4Format = FALSE;
                    break;

                case TEXT('a'):
                case TEXT('A'):
                    fExportMode = TRUE;
                    fSaveInAnsiRegedit4Format = TRUE;
                    break;

                case TEXT('s'):
                case TEXT('S'):
                    //
                    //  Silent mode where we don't show any dialogs when we
                    //  import a registry file script.
                    //
                    fSilentMode = TRUE;
                    //  FALL THROUGH

                case TEXT('v'):
                case TEXT('V'):
                    //
                    //  With the Windows 3.1 Registry Editor, this brought up
                    //  the tree-style view.  Now we always show the tree so
                    //  nothing to do here!
                    //
                    //  FALL THROUGH

                case TEXT('u'):
                case TEXT('U'):
                    //
                    //  Update, don't overwrite existing path entries in
                    //  shell\open\command or shell\open\print.  This isn't even
                    //  used by the Windows 3.1 Registry Editor!
                    //
                    //  FALL THROUGH

                default:
                    break;

            }

            lpCmdLine = CharNext(lpCmdLine);

        }

    }

    if (!fExportMode) {

        if (*lpCmdLine == 0)
            return PARSERET_CONTINUE;

        else {

            lpFileName = GetNextSubstring(lpCmdLine);

            while (lpFileName != NULL) {

                RegEdit_ImportRegFile(NULL, fSilentMode, lpFileName);
                lpFileName = GetNextSubstring(NULL);

            }

            return PARSERET_REFRESH;

        }

    }

    else {

        lpFileName = GetNextSubstring(lpCmdLine);
        lpSelectedPath = GetNextSubstring(NULL);

        if (GetNextSubstring(NULL) == NULL)
            RegEdit_ExportRegFile(NULL, fSilentMode, fSaveInAnsiRegedit4Format, lpFileName,
                lpSelectedPath);

        return PARSERET_EXIT;

    }

}

/*******************************************************************************
*
*  IsRegistryToolDisabled
*
*  DESCRIPTION:
*     Checks the policy section of the registry to see if registry editing
*     tools should be disabled.  This switch is set by administrators to
*     protect novice users.
*
*     The Registry Editor is disabled if and only if this value exists and is
*     set.
*
*  PARAMETERS:
*     (returns), TRUE if registry tool should not be run, else FALSE.
*
*******************************************************************************/

BOOL
PASCAL
IsRegistryToolDisabled(
    VOID
    )
{

    BOOL fRegistryToolDisabled;
    HKEY hKey;
    DWORD Type;
    DWORD ValueBuffer;
    DWORD cbValueBuffer;

    fRegistryToolDisabled = FALSE;

    if (RegOpenKey(HKEY_CURRENT_USER,
		   REGSTR_PATH_POLICIES TEXT("\\") REGSTR_KEY_SYSTEM,
		   &hKey) == ERROR_SUCCESS) {

        cbValueBuffer = sizeof(DWORD);

	if (RegQueryValueEx(hKey, REGSTR_VAL_DISABLEREGTOOLS, NULL, &Type,
            (LPSTR) &ValueBuffer, &cbValueBuffer) == ERROR_SUCCESS) {

            if (Type == REG_DWORD && cbValueBuffer == sizeof(DWORD) &&
                ValueBuffer != FALSE)
                fRegistryToolDisabled = TRUE;

        }

        RegCloseKey(hKey);

    }

    return fRegistryToolDisabled;

}
