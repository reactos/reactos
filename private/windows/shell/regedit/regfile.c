/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGFILE.C
*
*  VERSION:     4.0
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        21 Nov 1993
*
*  File import and export user interface routines for the Registry Editor.
*
*******************************************************************************/

#include "pch.h"
#include "regedit.h"
#include "regkey.h"
#include "regfile.h"
#include "regcdhk.h"
#include "regresid.h"

HWND g_hRegProgressWnd;

VOID
PASCAL
RegEdit_ImportRegFile(
    HWND hWnd,
    BOOL fSilentMode,
    LPTSTR lpFileName
    );

BOOL
PASCAL
RegEdit_GetFileName(
    HWND hWnd,
    BOOL fOpen,
    LPTSTR lpFileName,
    DWORD cchFileName
    );

INT_PTR
CALLBACK
RegProgressDlgProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );

/*******************************************************************************
*
*  RegEdit_ImportRegFile
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*     fSilentMode, TRUE if no messages should be displayed, else FALSE.
*     lpFileName, address of file name buffer.
*
*******************************************************************************/

VOID
PASCAL
RegEdit_ImportRegFile(
    HWND hWnd,
    BOOL fSilentMode,
    LPTSTR lpFileName
    )
{

    if (!fSilentMode && hWnd != NULL) {

        if ((g_hRegProgressWnd = CreateDialogParam(g_hInstance,
            MAKEINTRESOURCE(IDD_REGPROGRESS), hWnd, RegProgressDlgProc,
            (LPARAM) lpFileName)) != NULL)
            EnableWindow(hWnd, FALSE);

    }

    else
        g_hRegProgressWnd = NULL;

    //
    //  Prompt user to confirm importing a .reg file if running in silent mode 
    //  without a window (i.e. invoked .reg from a folder)
    //
    if (!fSilentMode && !hWnd)
    {
        if (InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(IDS_CONFIRMIMPFILE),
            MAKEINTRESOURCE(IDS_REGEDIT), MB_ICONQUESTION | MB_YESNO , lpFileName) != IDYES)
        {
            return;
        }
    }

    ImportRegFile(lpFileName);

    if (g_hRegProgressWnd != NULL) {

        EnableWindow(hWnd, TRUE);
        DestroyWindow(g_hRegProgressWnd);

    }

    if (!fSilentMode || g_FileErrorStringID != IDS_IMPFILEERRSUCCESS) {

        InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(g_FileErrorStringID),
            MAKEINTRESOURCE(IDS_REGEDIT), (g_FileErrorStringID ==
            IDS_IMPFILEERRSUCCESS) ? MB_ICONINFORMATION | MB_OK : MB_ICONERROR |
            MB_OK, lpFileName);

    }

}

/*******************************************************************************
*
*  RegEdit_OnDropFiles
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnDropFiles(
    HWND hWnd,
    HDROP hDrop
    )
{

    TCHAR FileName[MAX_PATH];
    UINT NumberOfDrops;
    UINT CurrentDrop;

    RegEdit_SetWaitCursor(TRUE);

    NumberOfDrops = DragQueryFile(hDrop, (UINT) -1, NULL, 0);

    for (CurrentDrop = 0; CurrentDrop < NumberOfDrops; CurrentDrop++) {

        DragQueryFile(hDrop, CurrentDrop, FileName, sizeof(FileName)/sizeof(TCHAR));
        RegEdit_ImportRegFile(hWnd, FALSE, FileName);

    }

    DragFinish(hDrop);

    RegEdit_OnKeyTreeRefresh(hWnd);

    RegEdit_SetWaitCursor(FALSE);

}

/*******************************************************************************
*
*  RegEdit_OnCommandImportRegFile
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnCommandImportRegFile(
    HWND hWnd
    )
{

    TCHAR FileName[MAX_PATH];

    if (RegEdit_GetFileName(hWnd, TRUE, FileName, sizeof(FileName)/sizeof(TCHAR))) {

        RegEdit_SetWaitCursor(TRUE);
        RegEdit_ImportRegFile(hWnd, FALSE, FileName);
        //  BUGBUG:  Only need to refresh the computer that we imported the
        //  file into, not the whole thing.
        RegEdit_OnKeyTreeRefresh(hWnd);
        RegEdit_SetWaitCursor(FALSE);

    }

}

/*******************************************************************************
*
*  RegEdit_ExportRegFile
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*     fSilentMode, TRUE if no messages should be displayed, else FALSE.
*     lpFileName, address of file name buffer.
*     lpSelectedPath,
*
*******************************************************************************/

VOID
PASCAL
RegEdit_ExportRegFile(
    HWND hWnd,
    BOOL fSilentMode,
    BOOL fUseDownlevelFormat,
    LPTSTR lpFileName,
    LPTSTR lpSelectedPath
    )
{

    //
    // Fix a bug where /a or /e is specified and no file is passed in
    //
    if (lpFileName == NULL) {
        InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(IDS_NOFILESPECIFIED),
            MAKEINTRESOURCE(IDS_REGEDIT), MB_ICONERROR | MB_OK, lpFileName);
        return;
    }

    if (fUseDownlevelFormat)
    {
        g_RegEditData.fSaveInDownlevelFormat = TRUE;
        ExportWin40RegFile(lpFileName, lpSelectedPath);
    }
    else
    {
        ExportWinNT50RegFile(lpFileName, lpSelectedPath);
    }

    if (g_FileErrorStringID == IDS_EXPFILEERRFILEWRITE) {

        InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(g_FileErrorStringID),
            MAKEINTRESOURCE(IDS_REGEDIT), MB_ICONERROR | MB_OK, lpFileName);

    }

}

/*******************************************************************************
*
*  RegEdit_OnCommandExportRegFile
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnCommandExportRegFile(
    HWND hWnd
    )
{

    TCHAR FileName[MAX_PATH];
    LPTSTR lpSelectedPath;

    if (RegEdit_GetFileName(hWnd, FALSE, FileName, sizeof(FileName)/sizeof(TCHAR))) {

        RegEdit_SetWaitCursor(TRUE);

        lpSelectedPath = g_fRangeAll ? NULL : g_SelectedPath;
        RegEdit_ExportRegFile(hWnd, FALSE, g_RegEditData.fSaveInDownlevelFormat, FileName, lpSelectedPath);

        RegEdit_SetWaitCursor(FALSE);

    }

}

/*******************************************************************************
*
*  RegEdit_GetFileName
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*     fOpen, TRUE if importing a file, else FALSE if exporting a file.
*     lpFileName, address of file name buffer.
*     cchFileName, size of file name buffer in TCHARacters.
*
*******************************************************************************/

BOOL
PASCAL
RegEdit_GetFileName(
    HWND hWnd,
    BOOL fOpen,
    LPTSTR lpFileName,
    DWORD cchFileName
    )
{

    UINT TitleStringID;
    PTSTR pTitle;
    PTSTR pDefaultExtension;
    PTSTR pFilter;
    PTSTR pFilterChar;
    OPENFILENAME OpenFileName;
    BOOL fSuccess;

    //
    //  Load various strings that will be displayed and used by the common open
    //  or save dialog box.  Note that if any of these fail, the error is not
    //  fatal-- the common dialog box may look odd, but will still work.
    //

    if (fOpen)
        TitleStringID = IDS_IMPORTREGFILETITLE;
    else
        TitleStringID = IDS_EXPORTREGFILETITLE;

    pTitle = LoadDynamicString(TitleStringID);

    pDefaultExtension = LoadDynamicString(IDS_REGFILEDEFEXT);

    if ((pFilter = LoadDynamicString(fOpen ? IDS_REGIMPORTFILEFILTER : 
                                             IDS_REGEXPORTFILEFILTER)) != NULL) {

        //
        //  The common dialog library requires that the substrings of the
        //  filter string be separated by nulls, but we cannot load a string
        //  containing nulls.  So we use some dummy character in the resource
        //  that we now convert to nulls.
        //

        for (pFilterChar = pFilter; *pFilterChar != 0; pFilterChar =
            CharNext(pFilterChar)) {

            if (*pFilterChar == TEXT('#'))
                *pFilterChar++ = 0;

        }

    }

    *lpFileName = 0;

    memset(&OpenFileName, 0, sizeof(OPENFILENAME));

    OpenFileName.lStructSize = sizeof(OPENFILENAME);
    OpenFileName.hwndOwner = hWnd;
    OpenFileName.hInstance = g_hInstance;
    OpenFileName.lpstrFilter = pFilter;
    //  OpenFileName.lpstrCustomFilter = NULL;
    //  OpenFileName.nMaxCustFilter = 0;
    //  OpenFileName.nFilterIndex = 0;
    OpenFileName.lpstrFile = lpFileName;
    OpenFileName.nMaxFile = cchFileName;
    //  OpenFileName.lpstrFileTitle = NULL;
    //  OpenFileName.nMaxFileTitle = 0;
    //  OpenFileName.lpstrInitialDir = NULL;
    OpenFileName.lpstrTitle = pTitle;
    //  OpenFileName.nFileOffset = 0;
    //  OpenFileName.nFileExtension = 0;
    OpenFileName.lpstrDefExt = pDefaultExtension;
    //  OpenFileName.lCustData = 0;
    //  OpenFileName.lpfnHook = NULL;
    //  OpenFileName.lpTemplateName = NULL;

    if (fOpen) {

        OpenFileName.Flags = OFN_HIDEREADONLY | OFN_EXPLORER |
            OFN_FILEMUSTEXIST;

        fSuccess = GetOpenFileName(&OpenFileName);

    }

    else {

        OpenFileName.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT |
            OFN_EXPLORER | OFN_NOREADONLYRETURN | OFN_PATHMUSTEXIST |
            OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;
        OpenFileName.lpfnHook = RegCommDlgHookProc;
        OpenFileName.lpTemplateName = MAKEINTRESOURCE(IDD_REGEXPORT);
        g_RegCommDlgDialogTemplate = IDD_REGEXPORT;

        fSuccess = GetSaveFileName(&OpenFileName);

    }

    //
    //  Delete all of the dynamic strings that we loaded.
    //

    if (pTitle != NULL)
        DeleteDynamicString(pTitle);

    if (pDefaultExtension != NULL)
        DeleteDynamicString(pDefaultExtension);

    if (pFilter != NULL)
        DeleteDynamicString(pFilter);

    return fSuccess;

}

/*******************************************************************************
*
*  RegProgressDlgProc
*
*  DESCRIPTION:
*     Callback procedure for the RegAbort dialog box.
*
*  PARAMETERS:
*     hWnd, handle of RegProgress window.
*     Message,
*     wParam,
*     lParam,
*     (returns),
*
*******************************************************************************/

INT_PTR
CALLBACK
RegProgressDlgProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{

    switch (Message) {

        case WM_INITDIALOG:
            //PathSetDlgItemPath(hWnd, IDC_FILENAME, (LPTSTR)lParam);
            SetDlgItemText(hWnd, IDC_FILENAME, (LPTSTR) lParam);
            break;

        default:
            return FALSE;

    }

    return TRUE;

}

/*******************************************************************************
*
*  ImportRegFileUICallback
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
ImportRegFileUICallback(
    UINT Percentage
    )
{

    if (g_hRegProgressWnd != NULL) {

        SendDlgItemMessage(g_hRegProgressWnd, IDC_PROGRESSBAR, PBM_SETPOS,
            (WPARAM) Percentage, 0);

        while (MessagePump(g_hRegProgressWnd))
            ;

    }

}
