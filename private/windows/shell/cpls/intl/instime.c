/*++

Copyright (c) 1994-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    instime.c

Abstract:

    This module implements the IME Installer for the Regional Options
    applet.

Revision History:

--*/



//
//  Include Files.
//

#include "intl.h"
#include "intlid.h"

#include <windowsx.h>
#include <setupapi.h>
#include <imm.h>
#include <help.h>
#include "instime.h"




//
//  Context Help Ids.
//

static int aInstallerHelpIds[] =
{
    IDC_BUTTON_SETUP,     IDH_KEYB_IME_INSTALL,
    IDC_REGISTERED_IME,   IDH_KEYB_IME_LIST,
    IDC_UNINSTALL,        IDH_KEYB_IME_REMOVE,
    IDOK,                 IDH_OK,

    IDC_INSTALL_INSTR,    NO_HELP,
    IDC_INSTALL_ICON,     NO_HELP,
    IDC_GROUPBOX1,        NO_HELP,
    IDC_UNINSTALL_INSTR,  NO_HELP,
    IDC_UNINSTALL_ICON,   NO_HELP,

    0, 0
};




//
//  Global Variables.
//

TCHAR g_szRegKbdLayout[]     = TEXT("System\\CurrentControlSet\\Control\\Keyboard Layouts");
TCHAR g_szRegLayoutFile[]    = TEXT("Layout File");
TCHAR g_szRegImeFile[]       = TEXT("Ime File");
TCHAR g_szRegLayoutText[]    = TEXT("Layout Text");
TCHAR g_szRegLayoutID[]      = TEXT("Layout Id");
TCHAR g_szRegKbdLayoutPath[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\KbdLayout Paths");
TCHAR g_szInfClass[3][20]    = {TEXT(""), TEXT("IME"), TEXT("KbdLayout")};

PINF_NODE Inf_pListHead = NULL;
int Inf_nCount = 0;




//
//  Typedef Declarations.
//

typedef struct _WIZPAGE
{
    int     id;
    DLGPROC pfn;
} WIZPAGE;




//
//  Forward Declarations.
//

BOOL CALLBACK
SetupDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);

BOOL CALLBACK
BrowseDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);





////////////////////////////////////////////////////////////////////////////
//
//  Installer_GetValueFromSectionByKey
//
//  Gets the value from the section name and the key name.
//
//    [Section]
//        Key = value
//
////////////////////////////////////////////////////////////////////////////

BOOL Installer_GetValueFromSectionByKey(
    HINF hInf,
    LPCTSTR lpszSection,
    LPCTSTR lpszKey,
    LPTSTR lpszBuffer,
    DWORD dwBufferSizeChars)
{
    INFCONTEXT InfContext;
    DWORD dwDontCare;
    BOOL bResult;

    if (!SetupFindFirstLine(hInf, lpszSection, lpszKey, &InfContext))
    {
        return (FALSE);
    }

    return (SetupGetStringField( &InfContext,
                                 1,
                                 lpszBuffer,
                                 dwBufferSizeChars,
                                 &dwDontCare ));
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_IsFileInUse
//
//  Check if the given file (includes path) is in use.
//
////////////////////////////////////////////////////////////////////////////

BOOL Installer_IsFileInUse(
    LPCTSTR lpszFileName)
{
    HANDLE hFile;

    //
    //  See if the file exists.
    //
    if (GetFileAttributes(lpszFileName) == 0xFFFFFFFF)
    {
        return (FALSE);
    }

    //
    //  See if the file is writable.
    //
    hFile = CreateFile( lpszFileName,
                        GENERIC_WRITE,
                        FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        0 );

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
        return (FALSE);
    }
    else
    {
        return (TRUE);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_ConcatenatePaths
//
//  Concatenate 2 paths.  Insures that only one path separator character
//  is introduced at the junction point.  Returns TRUE if the full path
//  fit in the target buffer.  Otherwise, the path is truncated.
//
//  lpszTarget - supplies first part of path (path is appended to this)
//  lpszPath   - supplies path to be concatenated to target
//
//  uTargetBufferSize - size of target buffer in characters
//  puRequiredSize    - if specified, receives the number of characters
//                      required to hold the fully concatenated path,
//                      including the terminating null.
//
////////////////////////////////////////////////////////////////////////////

BOOL Installer_ConcatenatePaths(
    LPTSTR lpszTarget,
    LPCTSTR lpszPath,
    UINT uTargetBufferSize,
    PUINT puRequiredSize)
{
    UINT uTargetLength, uPathLength;
    BOOL bTrailingBackslash, bLeadingBackslash;
    UINT uEndingLength;

    uTargetLength = lstrlen(lpszTarget);
    uPathLength = lstrlen(lpszPath);

    //
    //  See if the target has a trailing backslash.
    //
    if (uTargetLength && (lpszTarget[uTargetLength - 1] == TEXT('\\')))
    {
        bTrailingBackslash = TRUE;
        uTargetLength--;
    }
    else
    {
        bTrailingBackslash = FALSE;
    }

    //
    //  See if the path has a leading backshash.
    //
    if (lpszPath[0] == TEXT('\\'))
    {
        bLeadingBackslash = TRUE;
        uPathLength--;
    }
    else
    {
        bLeadingBackslash = FALSE;
    }

    //
    //  Calculate the ending length, which is equal to the sum of
    //  the length of the two strings modulo leading/trailing
    //  backslashes, plus one path separator, plus a null.
    //
    uEndingLength = uTargetLength + uPathLength + 2;
    if (puRequiredSize)
    {
        *puRequiredSize = uEndingLength;
    }

    if (!bLeadingBackslash && (uTargetLength < uTargetBufferSize))
    {
        lpszTarget[uTargetLength++] = TEXT('\\');
    }

    if (uTargetBufferSize > uTargetLength)
    {
        lstrcpyn( lpszTarget + uTargetLength,
                  lpszPath,
                  uTargetBufferSize - uTargetLength );
    }

    //
    //  Make sure the buffer is null terminated in all cases.
    //
    if (uTargetBufferSize)
    {
        lpszTarget[uTargetBufferSize - 1] = 0;
    }

    return (uEndingLength <= uTargetBufferSize);
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_PathBuildRoot
//
//  Convert ID to disk driver identifier.  Returns the pointer to the
//  string.
//
//  lpszRoot - buffer to receive driver identifier
//  iDrive   - drive id
//
////////////////////////////////////////////////////////////////////////////

LPTSTR Installer_PathBuildRoot(
    LPTSTR lpszRoot,
    int iDrive)
{
    lpszRoot[0] = (TCHAR)iDrive + (TCHAR)TEXT('A');
    lpszRoot[1] = TEXT(':');
    lpszRoot[2] = TEXT('\\');
    lpszRoot[3] = 0;

    return (lpszRoot);
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_PathFileExists
//
//  Check if the path file exists.  Returns TRUE if the path exists.
//  Otherwise, it returns FALSE.
//
////////////////////////////////////////////////////////////////////////////

BOOL Installer_PathFileExists(
    LPCTSTR lpszPath)
{
    DWORD dwErrMode;
    BOOL bResult = FALSE;

    if (!lpszPath || !(*lpszPath))
    {
        return (bResult);
    }

    dwErrMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    bResult = ((UINT)GetFileAttributes(lpszPath) != (UINT)-1);

    SetErrorMode(dwErrMode);
    return (bResult);
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_CopyToSystemDir
//
//  Copies the file from the specified path to the system directory.
//  Returns TRUE if the function succeeds.
//
////////////////////////////////////////////////////////////////////////////

BOOL Installer_CopyToSystemDir(
    LPCTSTR lpszSrcPath,
    LPCTSTR lpszFileName)
{
    TCHAR szSource[MAX_PATH];
    TCHAR szDest[MAX_PATH];

    lstrcpy(szSource, lpszSrcPath);

    if (!Installer_ConcatenatePaths(szSource, lpszFileName, MAX_PATH, NULL))
    {
        //
        //  Concatenate failed.
        //
        return (FALSE);
    }

    if (GetSystemDirectory(szDest, MAX_PATH) == 0)
    {
        //
        //  Get system directory failed.
        //
        return (FALSE);
    }

    if (!Installer_ConcatenatePaths(szDest, lpszFileName, MAX_PATH, NULL))
    {
        //
        //  Concatenate failed.
        //
        return (FALSE);
    }

    return (CopyFile(szSource, szDest, FALSE));
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_AppendEndingBkslash
//
//  Makes sure the path ends with '\'.  Returns TRUE if the function
//  succeeds.
//
////////////////////////////////////////////////////////////////////////////

BOOL Installer_AppendEndingBkslash(
    LPTSTR lpszPath)
{
    if ((lpszPath) && (lpszPath[0]))
    {
        if (lpszPath[lstrlen(lpszPath) - 1] != TEXT('\\') &&
            lpszPath[lstrlen(lpszPath) - 1] != TEXT(':'))
        {
            lstrcat(lpszPath, TEXT("\\"));
        }
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_Wiz_AddPage
//
//  Registers the wizard page and the function.
//
////////////////////////////////////////////////////////////////////////////

void Installer_Wiz_AddPage(
    LPPROPSHEETHEADER ppsh,
    UINT id,
    DLGPROC pfn)
{
    if (ppsh->nPages < MAX_PAGES)
    {
       PROPSHEETPAGE psp;

       psp.dwSize = sizeof(psp);
       psp.dwFlags = PSP_DEFAULT;
       psp.hInstance = hInstance;
       psp.pszTemplate = MAKEINTRESOURCE(id);
       psp.pfnDlgProc = pfn;
       psp.lParam = (LPARAM)NULL;

       ppsh->phpage[ppsh->nPages] = CreatePropertySheetPage(&psp);

       if (ppsh->phpage[ppsh->nPages])
       {
           ppsh->nPages++;
       }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_Wiz_InitHeaders
//
//  Initializes the Wizard Header structure.
//
////////////////////////////////////////////////////////////////////////////

void Installer_Wiz_InitHeaders(
    LPPROPSHEETHEADER ppsh,
    HPROPSHEETPAGE *rPages,
    HWND hParentWnd)
{
    ppsh->dwSize     = sizeof(*ppsh);
    ppsh->dwFlags    = PSH_PROPTITLE | PSH_WIZARD;
    ppsh->hwndParent = hParentWnd;
    ppsh->hInstance  = hInstance;
    ppsh->pszCaption = NULL;
    ppsh->nPages     = 0;
    ppsh->nStartPage = 0;
    ppsh->phpage     = rPages;
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_Wiz_Do
//
//  Adds each page to the property sheet.
//
////////////////////////////////////////////////////////////////////////////

BOOL Installer_Wiz_Do(
    WIZPAGE wp[],
    int PageCount,
    HWND hParentWnd)
{
    HPROPSHEETPAGE rPages[MAX_PAGES];
    PROPSHEETHEADER psh;
    int i;
    BOOL bResult = FALSE;

    Installer_Wiz_InitHeaders(&psh, rPages, hParentWnd);

    for (i = 0; i < PageCount; i++)
    {
        Installer_Wiz_AddPage(&psh, wp[i].id, wp[i].pfn);
    }

    bResult = PropertySheet(&psh);
    return (bResult);
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_StrToUINT
//
//  Converts a string (hex digits) to a UINT.
//
////////////////////////////////////////////////////////////////////////////

UINT Installer_StrToUINT(
    LPCTSTR lpszNum)
{
    LPTSTR lpszStop;

#ifdef UNICODE
    return (wcstoul(lpszNum, &lpszStop, 16));
#else
    return (strtoul(lpszNum, &lpszStop, 16));
#endif
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_ShowMessage
//
//  Puts up a message box with the given message.
//
////////////////////////////////////////////////////////////////////////////

int Installer_ShowMessage(
    HWND hWnd,
    UINT nStringID,
    UINT uType)
{
    TCHAR szTitle[LINE_LEN];
    TCHAR szMessage[LINE_LEN];

    LoadString(hInstance, IDS_APP_NAME, szTitle, LINE_LEN);
    LoadString(hInstance, nStringID, szMessage, LINE_LEN);

    return (MessageBox(hWnd, szMessage, szTitle, uType));
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_Inf_DeleteList
//
//  Deletes the whole list.
//
////////////////////////////////////////////////////////////////////////////

BOOL Installer_Inf_DeleteList()
{
    PINF_NODE pCurrentNode = NULL;

    while (Inf_pListHead)
    {
        pCurrentNode = Inf_pListHead;
        Inf_pListHead = Inf_pListHead->Next;
        LocalFree((HLOCAL)pCurrentNode);
        Inf_nCount--;
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_Inf_Initialize
//
//  Initializes the inf list and deletes all nodes if needed.
//
////////////////////////////////////////////////////////////////////////////

BOOL Installer_Inf_Initialize()
{
    if (Inf_pListHead)
    {
        return (Installer_Inf_DeleteList());
    }
    else
    {
        return (TRUE);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_Inf_AppendFile
//
//  Allocates a node and appends the IME info to the head of the list.
//
////////////////////////////////////////////////////////////////////////////

BOOL Installer_Inf_AppendFile(
    LPCTSTR lpszSrcPath,
    LPCTSTR lpszInfFile,
    LPCTSTR lpszSection,
    LPCTSTR lpszDesc,
    LPCTSTR lpszIMEFile,
    LPCTSTR lpszKbdLayoutFile,
    LPCTSTR lpszKbdLayoutID,
    LPCTSTR lpszKbdLayout,
    int nInfType)
{
    PINF_NODE pNewNode = NULL;

    pNewNode = (PINF_NODE)LocalAlloc(LPTR, sizeof(INF_NODE));

    if (pNewNode == NULL)
    {
        return (FALSE);
    }

    Inf_nCount++;

    lstrcpy(pNewNode->szSrcPath, lpszSrcPath);
    lstrcpy(pNewNode->szInfFile, lpszInfFile);
    lstrcpy(pNewNode->szSection, lpszSection);
    lstrcpy(pNewNode->szDesc, lpszDesc);

    if (nInfType == INF_IME)
    {
        lstrcpy(pNewNode->szIMEFile, lpszIMEFile);
        pNewNode->szKbdLayoutFile[0] = 0;
        pNewNode->szKbdLayout[0] = 0;
        pNewNode->szKbdLayoutID[0] = 0;
    }
    else
    {
        lstrcpy(pNewNode->szKbdLayoutFile, lpszKbdLayoutFile);
        lstrcpy(pNewNode->szKbdLayout, lpszKbdLayout);
        lstrcpy(pNewNode->szKbdLayoutID, lpszKbdLayoutID);
        pNewNode->szIMEFile[0] = 0;
    }

    pNewNode->nInfType = nInfType;

    pNewNode->Next = NULL;

    if (Inf_pListHead != NULL)
    {
        pNewNode->Next = Inf_pListHead;
    }
    Inf_pListHead = pNewNode;

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_Inf_AddToList
//
//  Fills in the list box and selects the first item.
//
////////////////////////////////////////////////////////////////////////////

BOOL Installer_Inf_AddToList(
    HWND hListBox)
{
    PINF_NODE pCurNode = NULL;
    int nIndex = 0;

    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);

    for (pCurNode = Inf_pListHead; pCurNode; pCurNode = pCurNode->Next)
    {
        if ((nIndex = SendMessage( hListBox,
                                   LB_ADDSTRING,
                                   0,
                                   (LPARAM)(LPCTSTR)pCurNode->szDesc )) != LB_ERR)
        {
            SendMessage( hListBox,
                         LB_SETITEMDATA,
                         (WPARAM)nIndex,
                         (LPARAM)pCurNode );
        }
    }
    SendMessage(hListBox, LB_SETCURSEL, 0, 0);
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_Inf_GetInstallInfo
//
//  Gets Inf node information for the item at the given index in the
//  given list box.
//
////////////////////////////////////////////////////////////////////////////

PINF_NODE Installer_Inf_GetInstallInfo(
    HWND hListBox,
    int nIndex)
{
    PINF_NODE pCurNode = NULL;
    pCurNode = (PINF_NODE)SendMessage( hListBox,
                                       LB_GETITEMDATA,
                                       (WPARAM)nIndex,
                                       (LPARAM)0 );
    if (!pCurNode)
    {
        return (NULL);
    }

    return (pCurNode);
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_IsKBDLayoutSetupFile
//
//  See if this inf file is designed for installing IMEs.
//
//  lpszInfFile - inf file (contains path)
//  lpszClass   - class name, IME or KBDCLASS
//  lpszSection - section name
//
////////////////////////////////////////////////////////////////////////////

BOOL Installer_IsKBDLayoutSetupFile(
    LPCTSTR lpszInfFile,
    int nInfType,
    LPCTSTR lpszSection)
{
    HINF hInf;
    INFCONTEXT InfContext;

    hInf = SetupOpenInfFile( lpszInfFile,
                             g_szInfClass[nInfType],
                             INF_STYLE_WIN4,
                             NULL );

    if (hInf == INVALID_HANDLE_VALUE)
    {
        return (FALSE);
    }

    if (!SetupFindFirstLine(hInf, lpszSection, NULL, &InfContext))
    {
        return (FALSE);
    }

    SetupCloseInfFile(hInf);
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_EnumSetupSectionFromInf
//
//  Enumerates the information in the given inf and creates an inf node
//  for each IME.
//
//  lpszSrcPath - the source path of the inf file
//  lpszInfFile - the inf file name (not including path)
//  nInfType    - the inf type, IME or KBDLAYOUT
//
////////////////////////////////////////////////////////////////////////////

BOOL Installer_EnumSetupSectionFromInf(
    LPCTSTR lpszSrcPath,
    LPCTSTR lpszInfFile,
    int nInfType)
{
    HINF hInf;
    INFCONTEXT InfContext;
    TCHAR szSection[MAX_SECT_NAME_LEN];
    TCHAR szDesc[LINE_LEN];
    TCHAR szIMEFile[LINE_LEN];
    TCHAR szKbdLayout[LINE_LEN];
    TCHAR szKbdLayoutFile[LINE_LEN];
    TCHAR szKbdLayoutID[LINE_LEN];
    TCHAR szSourceFile[MAX_PATH];
    DWORD dwDontCare;
    BOOL bResult;

    if ((nInfType != INF_IME) && (nInfType != INF_KBDLAYOUT))
    {
        return (FALSE);
    }

    lstrcpy(szSourceFile, lpszSrcPath);

    if (!Installer_ConcatenatePaths(szSourceFile, lpszInfFile, MAX_PATH, NULL))
    {
        return (FALSE);
    }

    hInf = SetupOpenInfFile( szSourceFile,
                             g_szInfClass[nInfType],
                             INF_STYLE_WIN4,
                             NULL );
    if (hInf == INVALID_HANDLE_VALUE)
    {
        return (FALSE);
    }

    if (SetupFindFirstLine( hInf,
                            (nInfType == INF_IME)
                              ? SZ_SECTION_IME
                              : SZ_SECTION_KBDLAYOUT,
                            NULL,
                            &InfContext ))
    {
        bResult = TRUE;
        do
        {
            //
            //  Get the sub-section name from [IME].
            //
            bResult = SetupGetStringField( &InfContext,
                                           1,
                                           szSection,
                                           sizeof(szSection) / sizeof(TCHAR),
                                           &dwDontCare );
            if (!bResult)
            {
                break;
            }

            if (nInfType == INF_IME)
            {
                bResult = Installer_GetValueFromSectionByKey(
                              hInf,
                              szSection,
                              SZ_IME_FILE,       // "IME_file"
                              szIMEFile,
                              sizeof(szIMEFile) / sizeof(TCHAR) );

                if (!bResult || !szIMEFile[0])
                {
                    //
                    //  No IME file specified, abort this section.
                    //
                    continue;
                }
            }
            else         // INF_KBDLAYOUT
            {
                bResult = Installer_GetValueFromSectionByKey(
                              hInf,
                              szSection,
                              SZ_LAYOUT_FILE,    // "Layout_file"
                              szKbdLayoutFile,
                              sizeof(szKbdLayoutFile) / sizeof(TCHAR) );

                if (!bResult || !szKbdLayoutFile[0])
                {
                    //
                    //  No layout file specified, abort this section.
                    //
                    continue;
                }

                bResult = Installer_GetValueFromSectionByKey(
                              hInf,
                              szSection,
                              SZ_LAYOUT,         // "Keyboard_Layout"
                              szKbdLayout,
                              sizeof(szKbdLayout) / sizeof(TCHAR) );

                if (!bResult || !szKbdLayout[0])
                {
                    //
                    //  No ID specified, abort this section.
                    //
                    continue;
                }

                bResult = Installer_GetValueFromSectionByKey(
                              hInf,
                              szSection,
                              SZ_LAYOUT_ID,      // "Layout_Id"
                              szKbdLayoutID,
                              sizeof(szKbdLayoutID) / sizeof(TCHAR) );
            }

            bResult = Installer_GetValueFromSectionByKey(
                          hInf,
                          szSection,
                          SZ_LAYOUT_TEXT,        // "Layout_Text"
                          szDesc,
                          sizeof(szDesc) / sizeof(TCHAR) );

            if (!bResult || !szDesc[0])
            {
                //
                //  If description missed, use section name instead.
                //
                lstrcpy(szDesc, szSection);
            }

            Installer_Inf_AppendFile( lpszSrcPath,
                                      lpszInfFile,
                                      szSection,
                                      szDesc,
                                      szIMEFile,
                                      szKbdLayoutFile,
                                      szKbdLayoutID,
                                      szKbdLayout,
                                      nInfType );

        } while (SetupFindNextLine(&InfContext, &InfContext));
    }

    SetupCloseInfFile(hInf);
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_EnumSetupInfFromDir
//
//  Enumerates all qualified inf files from the specified directory.
//  Returns the number of qualified inf files found.
//
////////////////////////////////////////////////////////////////////////////

UINT Installer_EnumSetupInfFromDir(
    LPTSTR lpszSrcPath)
{
    HANDLE hFindHandle;
    WIN32_FIND_DATA fd;
    TCHAR szFoundFile[MAX_PATH];
    TCHAR szSrcPath[MAX_PATH];
    int nInfFound = 0;
    int nInfType = 0;

    lstrcpy(szSrcPath, lpszSrcPath);
    Installer_ConcatenatePaths(szSrcPath, TEXT("*.inf"), MAX_PATH, NULL);

    hFindHandle = FindFirstFile(szSrcPath, &fd);
    if (hFindHandle != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                lstrcpy(szFoundFile, lpszSrcPath);
                Installer_ConcatenatePaths( szFoundFile,
                                            fd.cFileName,
                                            MAX_PATH,
                                            NULL );
                if (Installer_IsKBDLayoutSetupFile( szFoundFile,
                                                    INF_IME,
                                                    SZ_SECTION_IME ))
                {
                    nInfType = INF_IME;
                }
                else if (Installer_IsKBDLayoutSetupFile( szFoundFile,
                                                         INF_KBDLAYOUT,
                                                         SZ_SECTION_KBDLAYOUT ))
                {
                    nInfType = INF_KBDLAYOUT;
                }
                else
                {
                    nInfType = 0;
                }

                if (nInfType != 0)
                {
                    nInfFound++;

                    //
                    //  If found at least one qualified inf file, delete
                    //  the old list (if it exists).
                    //
                    if (nInfFound == 1)
                    {
                        Installer_Inf_Initialize();
                    }

                    Installer_EnumSetupSectionFromInf( lpszSrcPath,
                                                       fd.cFileName,
                                                       nInfType );
                }
            }

        } while (FindNextFile(hFindHandle, &fd));
    }

    FindClose(hFindHandle);

    return (nInfFound);
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_AskForInstallDisk
//
//  Bring up the dialog box to prompt for the installation path.  Returns
//  TRUE if the user pressed OK, FALSE if the user pressed CANCEL.
//
////////////////////////////////////////////////////////////////////////////

BOOL Installer_AskForInstallDisk(
    HWND hDlg,
    LPTSTR lpszPathBuffer,
    DWORD dwSizeofPathBuffer)
{
    TCHAR szPathBuffer[MAX_PATH];
    TCHAR szTitle[256];
    UINT  PromptResult;
    DWORD dwPathRequiredSize;
    DWORD dwErrMode;

    LoadString( hInstance,
                IDS_LOAD_FROM_DISK,
                szTitle,
                sizeof(szTitle) / sizeof(TCHAR) );

    PromptResult = SetupPromptForDisk(
                       hDlg,                          // parent window of dialog box
                       szTitle,                       // title of dialog box
                       NULL,                          // name of disk to insert
                       DISTR_OEMINF_DEFAULTPATH,      // expect source path
                       DISTR_INF_WILDCARD,            // name of file needed
                       NULL,                          // TagFile
                       IDF_NOCOMPRESSED | IDF_NOSKIP | IDF_NOBEEP,
                       lpszPathBuffer,                // receives the source location
                       dwSizeofPathBuffer,            // size of supplied buffer
                       &dwPathRequiredSize );         // buffer size needed

    if (PromptResult == DPROMPT_CANCEL)
    {
        return (FALSE);
    }
    else
    {
        return (TRUE);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_SkipMissingQueueCallback
//
//  Queue callback function.
//
////////////////////////////////////////////////////////////////////////////

UINT Installer_SkipMissingQueueCallback(
    PVOID Context,
    UINT Notification,
    UINT Param1,
    UINT Param2)
{
    return (SetupDefaultQueueCallback(Context, Notification, Param1, Param2));
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_DoInstall
//
//  Do installation from specified section.  Call standard api to do the
//  work.  Returns TRUE if the user pressed OK, FALSE if the user pressed
//  CANCEL.
//
////////////////////////////////////////////////////////////////////////////

BOOL Installer_DoInstall(
    HWND hDlg,
    LPCTSTR lpszInfFile,
    LPCTSTR lpszSection,
    int nInfType)
{
    HINF hInf;
    HSPFILEQ FileQueue;
    PVOID QContext;
    DWORD d;
    BOOL bResult = FALSE;

    if ((hInf = SetupOpenInfFile( lpszInfFile,
                                  g_szInfClass[nInfType],
                                  INF_STYLE_WIN4,
                                  NULL )) == INVALID_HANDLE_VALUE)
    {
        goto exit1;
    }

    if (!SetupOpenAppendInfFile(NULL, hInf, NULL))
    {
        goto exit2;
    }

    if ((FileQueue = SetupOpenFileQueue()) == INVALID_HANDLE_VALUE)
    {
        goto exit2;
    }

    bResult = SetupInstallFilesFromInfSection( hInf,
                                               NULL,
                                               FileQueue,
                                               lpszSection,
                                               NULL,
                                               SP_COPY_NEWER );
    if (!bResult)
    {
        goto exit3;
    }

    if (!(QContext = SetupInitDefaultQueueCallback(hDlg)))
    {
        bResult = FALSE;
        goto exit3;
    }

    if (SetupScanFileQueue( FileQueue,
                            SPQ_SCAN_FILE_VALIDITY | SPQ_SCAN_INFORM_USER,
                            hDlg,
                            NULL,
                            NULL,
                            &d ))
    {
        if (d != 1)
        {
            bResult = SetupCommitFileQueue( hDlg,
                                            FileQueue,
                                            Installer_SkipMissingQueueCallback,
                                            QContext );
            if (!bResult)
            {
                goto exit3;
            }
        }
    }

    bResult = SetupInstallFromInfSection( hDlg,
                                          hInf,
                                          lpszSection,
                                          SPINST_ALL & ~SPINST_FILES,
                                          NULL,
                                          NULL,
                                          0,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL );
exit3:
    SetupTermDefaultQueueCallback(QContext);
    SetupCloseFileQueue(FileQueue);

exit2:
    SetupCloseInfFile(hInf);

exit1:
    return (bResult);
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_RememberIME
//
//  Saves the IME setup information in the registry.
//
//  hkl        - keyboard layout (eg. e0010411)
//  lpszImeInf - name of inf file
//  lpszSec    - name of installation section
//  lpszDesc   - description used to display in selection list
//
////////////////////////////////////////////////////////////////////////////

BOOL Installer_RememberIME(
    HKL hkl,
    LPCTSTR lpszImeInf,
    LPCTSTR lpszSec,
    LPCTSTR lpszDesc)
{
    HKEY hKey;
    TCHAR szKeyName[MAX_PATH];
    LONG lRetVal;
    BOOL bResult = FALSE;

    wsprintf(szKeyName, TEXT("%s\\%08X"), g_szRegKbdLayoutPath, hkl);
    lRetVal = RegCreateKey( HKEY_LOCAL_MACHINE,
                            szKeyName,
                            &hKey );

    if (lRetVal != ERROR_SUCCESS)
    {
        goto err0;
    }

    lRetVal = RegSetValueEx( hKey,
                             SZ_FILE,
                             0,
                             REG_SZ,
                             (CONST BYTE *)lpszImeInf,
                             (lstrlen(lpszImeInf) + 1) * sizeof(TCHAR) );

    if (bResult != ERROR_SUCCESS)
    {
        goto err1;
    }

    lRetVal = RegSetValueEx( hKey,
                             SZ_SECTION,
                             0,
                             REG_SZ,
                             (CONST BYTE *)lpszSec,
                             (lstrlen(lpszSec) + 1) * sizeof(TCHAR) );

    if (lRetVal != ERROR_SUCCESS)
    {
        goto err1;
    }

    lRetVal = RegSetValueEx( hKey,
                             SZ_DESCRIPTION,
                             0,
                             REG_SZ,
                             (CONST BYTE *)lpszDesc,
                             (lstrlen(lpszDesc) + 1) * sizeof(TCHAR) );

    if (lRetVal != ERROR_SUCCESS)
    {
        goto err1;
    }

    bResult = TRUE;

err1:
    RegCloseKey(hKey);

err0:
    return (bResult);
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_UpdateFileInfo
//
//  Get information about the specified file/drive root and sets the Icon
//  and description field in the setup dialog box.
//
////////////////////////////////////////////////////////////////////////////

void Installer_UpdateFileInfo(
    HWND hDlg,
    LPTSTR lpszFileName)
{
    HWND hSearch;
    HICON hOldIcon;
    SHFILEINFO fi;

    if (lpszFileName)
    {
        SHGetFileInfo( lpszFileName,
                       0,
                       &fi,
                       sizeof(fi),
                       SHGFI_ICON | SHGFI_DISPLAYNAME | SHGFI_LARGEICON );

        hSearch = GetDlgItem(hDlg, IDC_SEARCH_ICON);
        hOldIcon = Static_SetIcon(hSearch, fi.hIcon);
        if (hOldIcon)
        {
            DestroyIcon(hOldIcon);
        }
        UpdateWindow(hSearch);

        hSearch = GetDlgItem(hDlg, IDC_SEARCH_NAME);
        Static_SetText(hSearch, fi.szDisplayName);
        UpdateWindow(hSearch);
    }
    else
    {
        hSearch = GetDlgItem(hDlg, IDC_SEARCH_ICON);
        hOldIcon = Static_SetIcon(hSearch, NULL);
        if (hOldIcon)
        {
            DestroyIcon(hOldIcon);
        }
        UpdateWindow(hSearch);

        hSearch = GetDlgItem(hDlg, IDC_SEARCH_NAME);
        Static_SetText(hSearch, TEXT(""));
        UpdateWindow(hSearch);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_Setup_NextPressed
//
//  Press next button in SetupDlgProc.  Looking for installable inf files
//  in A: or CD-ROM.
//
////////////////////////////////////////////////////////////////////////////

BOOL Installer_Setup_NextPressed(
    HWND hDlg)
{
    UINT iDrive;
    UINT iDrvType;
    UINT uInfFound = 0;
    TCHAR szDriveRoot[4];
    TCHAR szText[256];
    TCHAR szPathBuffer[MAX_PATH];
    DWORD dwPathRequiredSize;

    HCURSOR hcurOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

    LoadString( hInstance,
                IDS_SEARCHING,
                szText,
                sizeof(szText) / sizeof(TCHAR));

    Static_SetText(GetDlgItem(hDlg, IDC_SETUP_MSG), szText);

    for (iDrive = 0; (!uInfFound) && (iDrive < 26); iDrive++)
    {
        Installer_PathBuildRoot(szDriveRoot, iDrive);
        iDrvType = GetDriveType(szDriveRoot);

        if ((iDrvType == DRIVE_REMOVABLE) || (iDrvType == DRIVE_CDROM))
        {
            Installer_UpdateFileInfo(hDlg, szDriveRoot);
            if (Installer_PathFileExists(szDriveRoot))
            {
                uInfFound = Installer_EnumSetupInfFromDir(szDriveRoot);
            }
        }
    }

    while (uInfFound == 0)
    {
        if (!Installer_AskForInstallDisk( hDlg,
                                          szPathBuffer,
                                          sizeof(szPathBuffer) ))
        {
            break;
        }
        Installer_AppendEndingBkslash(szPathBuffer);
        if (Installer_PathFileExists(szPathBuffer))
        {
            uInfFound = Installer_EnumSetupInfFromDir(szPathBuffer);
        }

        Installer_UpdateFileInfo(hDlg, NULL);
    }

    SetCursor(hcurOld);
    return (uInfFound != 0);
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_Wizard
//
//  Creates the wizard pages.
//
////////////////////////////////////////////////////////////////////////////

BOOL Installer_Wizard(
    HWND hParentWnd)
{
    static WIZPAGE wp[] = {
                            { DLG_SETUP,        SetupDlgProc  },
                            { DLG_SETUP_BROWSE, BrowseDlgProc }
                          };

    return (Installer_Wiz_Do(wp, ARRAYSIZE(wp), hParentWnd));
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_GetUninstallIME
//
//  Gets uninstall IME/keyboard layout information from registry.
//
////////////////////////////////////////////////////////////////////////////

BOOL Installer_GetUninstallIME(
    HWND hListBox)
{
    HKEY hKeyKbdLayout;
    HKEY hKeyOneIME;
    DWORD dwKeyNameSize;
    int i;
    TCHAR szKeyName[MAX_PATH];
    TCHAR szImeDesc[MAX_PATH];
    int nCurSel = 0;
    LPTSTR lpszStop;
    DWORD dwTmp;
    LONG lRetVal;
    BOOL bResult = FALSE;

    lRetVal = RegOpenKey( HKEY_LOCAL_MACHINE,
                          g_szRegKbdLayoutPath,
                          &hKeyKbdLayout );

    if (lRetVal != ERROR_SUCCESS)
    {
        goto err0;
    }

    SendMessage(hListBox, LB_RESETCONTENT, 0, 0);

    dwKeyNameSize = sizeof(szKeyName);
    for (i = 0;
         RegEnumKey(hKeyKbdLayout, i, szKeyName, dwKeyNameSize) == ERROR_SUCCESS;
         i++)
    {
        lRetVal = RegOpenKey( hKeyKbdLayout,
                              szKeyName,
                              &hKeyOneIME );

        if (lRetVal != ERROR_SUCCESS)
        {
            //
            //  Cannot open this sub-key.
            //
            continue;
        }

        dwTmp = sizeof(szImeDesc);
        lRetVal = RegQueryValueEx( hKeyOneIME,
                                   SZ_DESCRIPTION,
                                   NULL,
                                   NULL,
                                   (LPBYTE)szImeDesc,
                                   &dwTmp );

        if (lRetVal != ERROR_SUCCESS)
        {
            RegCloseKey(hKeyOneIME);
            continue;
        }

        RegCloseKey(hKeyOneIME);

        dwTmp = sizeof(szImeDesc);
        nCurSel = SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)szImeDesc);
        dwTmp = Installer_StrToUINT(szKeyName);
        SendMessage(hListBox, LB_SETITEMDATA, nCurSel, dwTmp);
        dwKeyNameSize = sizeof(szKeyName);
    }
    RegCloseKey(hKeyKbdLayout);

    bResult = TRUE;

err0:
    return (bResult);
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_IsLayoutInUse
//
//  See if the IME is in use.  If the layout cannot be found in the
//  registry, then it is assumed that the layout is not being used by
//  the system.
//
////////////////////////////////////////////////////////////////////////////

BOOL Installer_IsLayoutInUse(
    HKL hkl)
{
    TCHAR szKeyName[MAX_PATH];
    TCHAR szFile[MAX_PATH];
    TCHAR szLayoutPathName[MAX_PATH];
    HKEY hKeyKbdLayout;
    BOOL bInUse = FALSE;
    LONG lRetVal;
    DWORD dwTmp;

    wsprintf(szKeyName, TEXT("%s\\%08X"), g_szRegKbdLayout, hkl);
    lRetVal = RegOpenKey( HKEY_LOCAL_MACHINE,
                          szKeyName,
                          &hKeyKbdLayout );

    if (lRetVal != ERROR_SUCCESS)
    {
        goto err0;
    }

    if (IS_IME(hkl))
    {
        //
        //  IME.
        //
        dwTmp = sizeof(szFile);
        lRetVal = RegQueryValueEx( hKeyKbdLayout,
                                   g_szRegImeFile,
                                   NULL,
                                   NULL,
                                   (LPBYTE)szFile,
                                   &dwTmp );
    }
    else
    {
        //
        //  Keyboard Layout.
        //
        dwTmp = sizeof(szFile);
        lRetVal = RegQueryValueEx( hKeyKbdLayout,
                                   g_szRegLayoutFile,
                                   NULL,
                                   NULL,
                                   (LPBYTE)szFile,
                                   &dwTmp );
    }

    if (lRetVal != ERROR_SUCCESS)
    {
        goto err1;
    }

    GetSystemDirectory(szLayoutPathName, MAX_PATH);

    Installer_ConcatenatePaths(szLayoutPathName, szFile, MAX_PATH, NULL);

    bInUse = Installer_IsFileInUse(szLayoutPathName);

err1:
    RegCloseKey(hKeyKbdLayout);

err0:
    if (lRetVal != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    return (bInUse);
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_GetIMEInfoFromReg
//
//  Get uninstall IME/Keyboard layout information from registry.
//
////////////////////////////////////////////////////////////////////////////

BOOL Installer_GetIMEInfoFromReg(
    HKL hkl,
    LPTSTR lpszInfFile,
    LPTSTR lpszSect)
{
    TCHAR szKeyName[MAX_PATH];
    LONG lRetVal;
    DWORD dwTmp;
    HKEY hKeyKbdLayout;
    BOOL bResult = FALSE;

    wsprintf(szKeyName, TEXT("%s\\%08X"), g_szRegKbdLayoutPath, (DWORD)hkl);

    lRetVal = RegOpenKey( HKEY_LOCAL_MACHINE,
                          szKeyName,
                          &hKeyKbdLayout );

    if (lRetVal != ERROR_SUCCESS)
    {
        goto err0;
    }

    dwTmp = MAX_PATH;
    lRetVal = RegQueryValueEx( hKeyKbdLayout,
                               SZ_FILE,
                               NULL,
                               NULL,
                               (LPBYTE)lpszInfFile,
                               &dwTmp );

    if (lRetVal != ERROR_SUCCESS)
    {
        goto err1;
    }

    dwTmp = MAX_PATH;
    lRetVal = RegQueryValueEx( hKeyKbdLayout,
                               SZ_SECTION,
                               NULL,
                               NULL,
                               (LPBYTE)lpszSect,
                               &dwTmp );

    if (lRetVal != ERROR_SUCCESS)
    {
        goto err1;
    }

    bResult = TRUE;

err1:
    RegCloseKey(hKeyKbdLayout);

err0:
    return (bResult);
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_GetUninstallSectionName
//
//  Get uninstall section name from inf file.
//
////////////////////////////////////////////////////////////////////////////

BOOL Installer_GetUninstallSectionName(
    LPCTSTR lpszInfFile,
    LPCTSTR lpszSect,
    LPTSTR lpszUninst,
    DWORD dwSize,
    int nInfType)
{
    HINF hInf;
    LONG lRetVal;
    BOOL bResult = FALSE;

    hInf = SetupOpenInfFile( lpszInfFile,
                             g_szInfClass[nInfType],
                             INF_STYLE_WIN4,
                             NULL );
    if (hInf == INVALID_HANDLE_VALUE)
    {
        goto err0;
    }

    bResult = Installer_GetValueFromSectionByKey( hInf,
                                                  lpszSect,
                                                  SZ_UNINSTALL,
                                                  lpszUninst,
                                                  dwSize );
    if (!bResult)
    {
        goto err1;
    }

    bResult = TRUE;

err1:
    SetupCloseInfFile(hInf);

err0:
    return (bResult);
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_UninstallIME
//
////////////////////////////////////////////////////////////////////////////

BOOL Installer_UninstallIME(
    HWND hDlg)
{
    HWND hIMEListBox;
    int nCurSel;
    TCHAR szKeyName[MAX_PATH];
    TCHAR szInfFile[MAX_PATH];
    TCHAR szSect[MAX_PATH];
    TCHAR szUninst[MAX_PATH];
    INFCONTEXT InfContext;
    BOOL bDelRegOnly = FALSE;
    HKL hkl = 0;
    BOOL bResult = TRUE;
    LONG lRetVal;

    hIMEListBox = GetDlgItem(hDlg, IDC_REGISTERED_IME);

    if ((nCurSel = (int)SendMessage(hIMEListBox, LB_GETCURSEL, 0, 0)) == LB_ERR)
    {
        //
        //  No selection, do nothing.
        //
        return (FALSE);
    }

    hkl = (HKL)SendMessage(hIMEListBox, LB_GETITEMDATA, (WPARAM)nCurSel, 0);

    if (Installer_IsLayoutInUse(hkl))
    {
        //
        //  IME is being used by some application.  Remind the user to
        //  close applications.
        //
        Installer_ShowMessage(hDlg, IDS_CANNOT_REMOVE_IME, MB_OK);
        return (FALSE);
    }
    if (!Installer_GetIMEInfoFromReg(hkl, szInfFile, szSect))
    {
        //
        //  IME info is missing from the registry.
        //
        return (FALSE);
    }

    if (!Installer_GetUninstallSectionName( szInfFile,
                                            szSect,
                                            szUninst,
                                            sizeof(szUninst) / sizeof(TCHAR),
                                            IS_IME(hkl)
                                              ? INF_IME
                                              : INF_KBDLAYOUT ))
    {
        //
        //  IME inf is incorrect.
        //
        if (Installer_ShowMessage( hDlg,
                                   IDS_UNINSTALL_MISSED,
                                   MB_YESNO ) == IDYES)
        {
            bDelRegOnly = TRUE;
        }
        else
        {
            return (FALSE);
        }
    }

    UnloadKeyboardLayout(hkl);

    if (!bDelRegOnly)
    {
        //
        //  Uninstall now.
        //
        if (!Installer_DoInstall( hDlg,
                                  szInfFile,
                                  szUninst,
                                  IS_IME(hkl) ? INF_IME : INF_KBDLAYOUT ))
        {
            return (FALSE);
        }
    }

    wsprintf(szKeyName, TEXT("%s\\%08X"), g_szRegKbdLayoutPath, hkl);
    lRetVal = RegDeleteKey(HKEY_LOCAL_MACHINE, szKeyName);
    if (lRetVal != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    wsprintf(szKeyName, TEXT("%s\\%08X"), g_szRegKbdLayout, hkl);
    lRetVal = RegDeleteKey(HKEY_LOCAL_MACHINE, szKeyName);
    if (lRetVal != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    if (DeleteFile(szInfFile) == FALSE)
    {
        return (FALSE);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Installer_KbdLayoutInstallReg
//
//  Write keyboard layout information into
//  HKLM,System\CurrentControlSet\Control\Keyboard Layouts.
//
////////////////////////////////////////////////////////////////////////////

HKL Installer_KbdLayoutInstallReg(
    HWND hWnd,
    LPCTSTR lpszKbdLayout,
    LPCTSTR lpszKbdLayoutFile,
    LPCTSTR lpszKbdLayoutText,
    LPCTSTR lpszKbdLayoutID)
{
    HKEY hKeyKbdLayout, hKeyOneIme;
    TCHAR szLayoutFile[MAX_PATH];
    DWORD dwTmp;
    HKL hkl = 0;
    LONG lRetVal = 0;

    lRetVal = RegOpenKey( HKEY_LOCAL_MACHINE,
                          g_szRegKbdLayout,
                          &hKeyKbdLayout );

    if (lRetVal != ERROR_SUCCESS)
    {
        goto err0;
    }

    lRetVal = RegOpenKey( hKeyKbdLayout,
                          lpszKbdLayout,
                          &hKeyOneIme );

    //
    //  Keyboard Layout exists, check layout file.
    //
    //  If the layout file is the same, ask the user if we should
    //  overwrite it.
    //
    if (lRetVal == ERROR_SUCCESS)
    {
        dwTmp = sizeof(szLayoutFile);
        lRetVal = RegQueryValueEx( hKeyOneIme,
                                   TEXT("Layout Text"),
                                   NULL,
                                   NULL,
                                   (LPBYTE)szLayoutFile,
                                   &dwTmp );

        dwTmp = sizeof(szLayoutFile);
        lRetVal = RegQueryValueEx( hKeyOneIme,
                                   g_szRegLayoutFile,
                                   NULL,
                                   NULL,
                                   (LPBYTE)szLayoutFile,
                                   &dwTmp );

        if (lRetVal == ERROR_SUCCESS)
        {
            //
            //  If the layout file is different from the one in the registry,
            //  then the overwrite is dangerous. Give the user a warning.
            //
            if (lstrcmpi(lpszKbdLayoutFile, szLayoutFile) != 0)
            {
                if (Installer_ShowMessage( hWnd,
                                           IDS_LAYOUT_EXIST,
                                           MB_OKCANCEL ) == IDCANCEL)
                {
                    goto err2;
                }
            }
        }
    }
    else
    {
        //
        //  Keyboard Layout didn't exist, create and set value.
        //
        lRetVal = RegCreateKey( hKeyKbdLayout,
                                lpszKbdLayout,
                                &hKeyOneIme );

        if (lRetVal != ERROR_SUCCESS)
        {
            goto err0;
        }
    }

    lRetVal = RegSetValueEx( hKeyOneIme,
                             g_szRegLayoutText,
                             0,
                             REG_SZ,
                             (CONST BYTE *)lpszKbdLayoutText,
                             (lstrlen(lpszKbdLayoutText) + 1) * sizeof(TCHAR) );

    if (lRetVal != ERROR_SUCCESS)
    {
        goto err2;
    }

    lRetVal = RegSetValueEx( hKeyOneIme,
                             g_szRegLayoutFile,
                             0,
                             REG_SZ,
                             (CONST BYTE *)lpszKbdLayoutFile,
                             (lstrlen(lpszKbdLayoutFile) + 1) * sizeof(TCHAR) );

    if (lRetVal != ERROR_SUCCESS)
    {
        goto err2;
    }

    if (lpszKbdLayoutID[0] == 0)
    {
        RegDeleteValue(hKeyOneIme, g_szRegLayoutID);
    }
    else
    {
        lRetVal = RegSetValueEx( hKeyOneIme,
                                 g_szRegLayoutID,
                                 0,
                                 REG_SZ,
                                 (CONST BYTE *)lpszKbdLayoutID,
                                 (lstrlen(lpszKbdLayoutID) + 1) * sizeof(TCHAR) );
    }

    hkl = (HKL)Installer_StrToUINT(lpszKbdLayout);

err2:
    RegCloseKey(hKeyOneIme);
    RegCloseKey(hKeyKbdLayout);

err0:
    return (hkl);
}


////////////////////////////////////////////////////////////////////////////
//
//  BrowseDlgProc
//
//  Dialog procedure for the Browse dialog.
//
////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK BrowseDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    NMHDR *lpnm;
    HWND hListBox = NULL;
    int nCurSel = 0;

    switch (message)
    {
        case ( WM_NOTIFY ) :
        {
            lpnm = (NMHDR *)lParam;
            switch (lpnm->code)
            {
                case ( PSN_SETACTIVE ) :
                {
                    Installer_Inf_AddToList(GetDlgItem(hDlg, IDC_IME_LIST));

                    PropSheet_SetWizButtons( GetParent(hDlg),
                                             PSWIZB_BACK | PSWIZB_FINISH );
                    PostMessage(hDlg, WMPRIV_POKEFOCUS, 0, 0);

                    break;
                }
                case ( PSN_WIZBACK ) :
                {
                    break;
                }
                case ( PSN_WIZFINISH ) :
                {
                    hListBox = GetDlgItem(hDlg, IDC_IME_LIST);
                    if ((nCurSel = SendMessage( hListBox,
                                                LB_GETCURSEL,
                                                0,
                                                0 )) == LB_ERR)
                    {
                        SetDlgMsgResult(hDlg, WM_NOTIFY, 0);
                    }
                    else
                    {
                        PINF_NODE pInfNode = NULL;
                        TCHAR szSourceFile[MAX_PATH];
                        TCHAR szDestFile[MAX_PATH];
                        TCHAR szTempName[MAX_PATH];
                        TCHAR szCurrentPath[MAX_PATH];
                        HKL hkl = NULL;

                        pInfNode = Installer_Inf_GetInstallInfo( hListBox,
                                                                 nCurSel );
                        if (pInfNode)
                        {
                            lstrcpy(szSourceFile, pInfNode->szSrcPath);
                            Installer_ConcatenatePaths( szSourceFile,
                                                        pInfNode->szInfFile,
                                                        MAX_PATH,
                                                        NULL );

                            if (pInfNode->nInfType == INF_IME)
                            {
                                GetCurrentDirectory( sizeof(szCurrentPath),
                                                     szCurrentPath );

                                SetCurrentDirectory(pInfNode->szSrcPath);
#ifdef UNICODE
                                //
                                //  HACK: Should be removed when
                                //        ImmInstallIME is fixed.
                                //
                                Installer_CopyToSystemDir( pInfNode->szSrcPath,
                                                           pInfNode->szIMEFile );
#endif
                                hkl = ImmInstallIME( pInfNode->szIMEFile,
                                                     pInfNode->szDesc );
                                SetCurrentDirectory(szCurrentPath);
                                if (!hkl)
                                {
                                    Installer_ShowMessage( hDlg,
                                                           IDS_CANNOT_INSTALL_IME,
                                                           MB_OK );
                                }
                            }
                            else
                            {
                                hkl = Installer_KbdLayoutInstallReg(
                                          hDlg,
                                          pInfNode->szKbdLayout,
                                          pInfNode->szKbdLayoutFile,
                                          pInfNode->szDesc,
                                          pInfNode->szKbdLayoutID );
                            }

                            if (hkl)
                            {
                                if (Installer_DoInstall( hDlg,
                                                         szSourceFile,
                                                         pInfNode->szSection,
                                                         IS_IME(hkl)
                                                           ? INF_IME
                                                           : INF_KBDLAYOUT ))
                                {
                                    GetWindowsDirectory(szDestFile, MAX_PATH);
                                    Installer_ConcatenatePaths( szDestFile,
                                                                TEXT("inf"),
                                                                MAX_PATH,
                                                                NULL );
                                    wsprintf(szTempName, TEXT("%08X.inf"), hkl);
                                    Installer_ConcatenatePaths( szDestFile,
                                                                szTempName,
                                                                MAX_PATH,
                                                                NULL );
                                    CopyFile(szSourceFile, szDestFile, FALSE);
                                    Installer_RememberIME( hkl,
                                                           szDestFile,
                                                           pInfNode->szSection,
                                                           pInfNode->szDesc );
                                }
                            }
                            else
                            {
                                SetWindowLong(hDlg, DWL_MSGRESULT, -1);
                            }
                        }
                    }
                    break;
                }
                case ( PSN_RESET ) :
                {
                    Installer_Inf_Initialize();
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        case ( WM_INITDIALOG ) :
        {
            break;
        }
        case ( WMPRIV_POKEFOCUS ) :
        {
            PropSheet_SetWizButtons( GetParent(hDlg),
                                     PSWIZB_BACK | PSWIZB_FINISH );
            break;
        }
        case ( WM_DESTROY ) :
        case ( WM_HELP ) :
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case ( IDHELP ) :
                {
                    break;
                }
                case ( IDC_IME_LIST ) :
                {
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case ( LBN_DBLCLK ) :
                        {
                            PropSheet_PressButton( GetParent(hDlg),
                                                   PSBTN_FINISH );
                            break;
                        }
                    }
                    break;
                }
                case ( IDC_HAVE_DISK ) :
                {
                    UINT uInfFound;
                    TCHAR szPathBuffer[MAX_PATH];

                    if (Installer_AskForInstallDisk( hDlg,
                                                     szPathBuffer,
                                                     sizeof(szPathBuffer) ))
                    {
                        Installer_AppendEndingBkslash(szPathBuffer);
                        if (Installer_PathFileExists(szPathBuffer))
                        {
                            uInfFound = Installer_EnumSetupInfFromDir(szPathBuffer);
                            if (uInfFound != 0)
                            {
                                Installer_Inf_AddToList(GetDlgItem( hDlg,
                                                                    IDC_IME_LIST ));
                            }
                        }
                    }
                    break;
                }
            }
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  SetupDlgProc
//
//  Dialog procedure for the Setup dialog.
//
////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK SetupDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    NMHDR *lpnm;

    switch (message)
    {
        case ( WM_NOTIFY ) :
        {
            lpnm = (NMHDR *)lParam;

            switch (lpnm->code)
            {
                case ( PSN_SETACTIVE ) :
                {
                    TCHAR szText[MAX_PATH];
                    HICON hOldIcon;

                    LoadString( hInstance,
                                IDS_INSERT_DISK,
                                szText,
                                ARRAYSIZE(szText) );
                    Static_SetText(GetDlgItem(hDlg, IDC_SETUP_MSG), szText);
                    Static_SetText(GetDlgItem(hDlg, IDC_SEARCH_NAME), NULL);

                    Installer_UpdateFileInfo(hDlg, NULL);

                    Installer_Inf_Initialize();

                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
                    PostMessage(hDlg, WMPRIV_POKEFOCUS, 0, 0);

                    break;
                }
                case ( PSN_WIZNEXT ) :
                {
                    if (!Installer_Setup_NextPressed(hDlg))
                    {
                        Installer_UpdateFileInfo(hDlg, NULL);

                        SetWindowLong(hDlg, DWL_MSGRESULT, -1);
                    }
                    else
                    {
                        SetDlgMsgResult(hDlg, WM_NOTIFY, 0);
                    }

                    break;
                }
                case ( PSN_RESET ) :
                {
                    Installer_Inf_Initialize();
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        case ( WM_INITDIALOG ) :
        {
            break;
        }
        case ( WM_DESTROY ) :
        {
            Installer_Inf_DeleteList();
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  InstallerDlgProc
//
//  Dialog procedure for the Install/Uninstall dialog.
//
////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK InstallerDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    HWND hIMEListBox;
    HWND hRemoveBtn;

    switch (message)
    {
        case ( WM_INITDIALOG ) :
        {
            hIMEListBox = GetDlgItem(hDlg, IDC_REGISTERED_IME);
            Installer_GetUninstallIME(hIMEListBox);
            hRemoveBtn = GetDlgItem(hDlg, IDC_UNINSTALL);
            EnableWindow( hRemoveBtn,
                          (SendMessage( hIMEListBox,
                                        LB_GETCURSEL,
                                        0,
                                        0 ) != LB_ERR) );
            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     NULL,
                     HELP_WM_HELP,
                     (DWORD)(LPTSTR)aInstallerHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND)wParam,
                     NULL,
                     HELP_CONTEXTMENU,
                     (DWORD)(LPTSTR)aInstallerHelpIds );
            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case ( IDC_BUTTON_SETUP ) :
                {
                    //
                    //  Install... button.
                    //
                    Installer_Wizard(hDlg);

                    hIMEListBox = GetDlgItem(hDlg, IDC_REGISTERED_IME);
                    Installer_GetUninstallIME(hIMEListBox);

                    break;
                }
                case ( IDC_UNINSTALL ) :
                {
                    //
                    //  Remove button.
                    //
                    Installer_UninstallIME(hDlg);

                    hIMEListBox = GetDlgItem(hDlg, IDC_REGISTERED_IME);
                    Installer_GetUninstallIME(hIMEListBox);

                    hRemoveBtn = GetDlgItem(hDlg, IDC_UNINSTALL);
                    EnableWindow( hRemoveBtn,
                                  (SendMessage( hIMEListBox,
                                                LB_GETCURSEL,
                                                0,
                                                0 ) != LB_ERR) );
                    break;
                }
                case ( IDC_REGISTERED_IME ) :
                {
                    //
                    //  IME list box.
                    //
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        //
                        //  Double click, remove this installed IME.
                        //
                        case ( LBN_DBLCLK ) :
                        {
                            Installer_UninstallIME(hDlg);

                            hIMEListBox = GetDlgItem(hDlg, IDC_REGISTERED_IME);
                            Installer_GetUninstallIME(hIMEListBox);

                            hRemoveBtn = GetDlgItem(hDlg, IDC_UNINSTALL);
                            EnableWindow( hRemoveBtn,
                                          (SendMessage( hIMEListBox,
                                                        LB_GETCURSEL,
                                                        0,
                                                        0 ) != LB_ERR) );
                            break;
                        }
                        case ( LBN_SELCHANGE ) :
                        {
                            //
                            //  When the selection changed, check if
                            //  Remove button needs to be disabled.
                            //
                            hRemoveBtn = GetDlgItem(hDlg, IDC_UNINSTALL);
                            EnableWindow( hRemoveBtn,
                                          (SendMessage( (HWND)lParam,
                                                        LB_GETCURSEL,
                                                        0,
                                                        0 ) != LB_ERR) );
                            break;
                        }
                    }
                    break;
                }
                case ( IDOK ) :
                {
                    EndDialog(hDlg, TRUE);
                    break;
                }
            }
            break;
        }
        case ( WM_CLOSE ) :
        {
            EndDialog(hDlg, TRUE);
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}
