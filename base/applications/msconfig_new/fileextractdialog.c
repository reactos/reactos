/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/fileextractdialog.c
 * PURPOSE:     File Extract Dialog
 * COPYRIGHT:   Copyright 2011-2012 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 */

#include "precomp.h"

#include <wingdi.h>
#include <commdlg.h> // It needs stuff defined in wingdi.h ...
#undef LF_FACESIZE   // but wingdi.h defines LF_FACESIZE, and if LF_FACESIZE is defined ...
#include <shlobj.h>  // shlobj.h wants to define NT_CONSOLE_PROPS which needs COORD structure
                     // to be declared, this means, including wincon.h in a GUI app!!
                     // WTF!! I don't want that!!

// #include <setupapi.h>
#include "fileextractdialog.h"
#include "comctl32supp.h"
#include "utils.h"

// #include "callbacks.h"

// FIXME: This should be present in PSDK commdlg.h
//
// FlagsEx Values
#if (_WIN32_WINNT >= 0x0500)
#define  OFN_EX_NOPLACESBAR         0x00000001
#endif // (_WIN32_WINNT >= 0x0500)


VOID
AddStringToComboList(HWND hWnd,
                     LPCWSTR lpszString)
{
    /* Try to find an already existing occurrence of the string in the list */
    LRESULT hPos = ComboBox_FindStringExact(hWnd, -1, lpszString);

    if (hPos == CB_ERR)
    {
        /* The string doesn't exist, so add it to the list and select it */
        ComboBox_InsertString(hWnd, 0, lpszString);
        ComboBox_SetCurSel(hWnd, 0);
    }
    else
    {
        /* The string exists, so select it */
        ComboBox_SetCurSel(hWnd, hPos);
    }

    return;
}

INT_PTR CALLBACK
FileExtractDialogWndProc(HWND hDlg,
                         UINT message,
                         WPARAM wParam,
                         LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
            return TRUE;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    LPWSTR szCabPathFileName, szFileName, szDestDir;
                    size_t cabPathNum, fileNameNum, destDirNum;

                    cabPathNum = GetWindowTextLengthW(GetDlgItem(hDlg, IDC_DRP_CAB_FILE)) + 1;
                    szCabPathFileName = (LPWSTR)MemAlloc(0, cabPathNum * sizeof(WCHAR));
                    GetDlgItemText(hDlg, IDC_DRP_CAB_FILE, szCabPathFileName, (int)cabPathNum);

                    fileNameNum = GetWindowTextLengthW(GetDlgItem(hDlg, IDC_TXT_FILE_TO_RESTORE)) + 1;
                    szFileName = (LPWSTR)MemAlloc(0, fileNameNum * sizeof(WCHAR));
                    GetDlgItemText(hDlg, IDC_TXT_FILE_TO_RESTORE, szFileName, (int)fileNameNum);

                    destDirNum = GetWindowTextLengthW(GetDlgItem(hDlg, IDC_DRP_DEST_DIR)) + 1;
                    szDestDir = (LPWSTR)MemAlloc(0, destDirNum * sizeof(WCHAR));
                    GetDlgItemText(hDlg, IDC_DRP_DEST_DIR, szDestDir, (int)destDirNum);

#if 0
                    if (!ExtractFromCabinet(szCabPathFileName, szFileName, szDestDir))
                    {
                        MessageBoxW(NULL, L"An error has occurred!!", L"Error", MB_ICONERROR | MB_OK);
                    }
                    else
                    {
                        MessageBoxW(NULL, L"All the files were uncompressed successfully.", L"Info", MB_ICONINFORMATION | MB_OK);

                        // TODO: Save the extraction paths into the registry.

                        /* Quit */
                        EndDialog(hDlg, LOWORD(wParam));
                    }
#else
                    MessageBoxW(NULL, L"File extraction is unimplemented!", L"Info", MB_ICONINFORMATION | MB_OK);
#endif

                    MemFree(szDestDir);
                    MemFree(szFileName);
                    MemFree(szCabPathFileName);

                    return TRUE;
                }

                case IDCANCEL:
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;

                case IDC_BTN_BROWSE_ALL_FILES:
                {
                    unsigned int nMaxFilesNum = 255;
                    size_t newSize = (nMaxFilesNum * (MAX_PATH + 1)) + 1;
                    LPWSTR szPath  = (LPWSTR)MemAlloc(HEAP_ZERO_MEMORY, newSize * sizeof(WCHAR));
                    OPENFILENAMEW ofn;

                    SecureZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner   = hDlg;
                    ofn.lpstrTitle  = L"Files to be restored"; // L"Fichiers à restaurer"; // FIXME: Localize!
                    ofn.Flags       = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_LONGNAMES;
                    // ofn.FlagsEx     = OFN_EX_NOPLACESBAR;
                    ofn.lpstrFilter = L"All files (*.*)\0*.*\0";
                    ofn.nFilterIndex = 0;
                    ofn.lpstrFile   = szPath;
                    ofn.nMaxFile    = (DWORD)newSize; // TODO: size_t to DWORD conversion...

                    if (GetSaveFileName(&ofn))
                    {
                        if ( (ofn.Flags & OFN_EXPLORER) &&
                             (ofn.Flags & OFN_ALLOWMULTISELECT) ) // Must be always true...
                        {
                            LPWSTR lpszFiles = szPath + ofn.nFileOffset;
                            LPWSTR lpszFilePatterns = NULL;

                            LPWSTR lpszTmp = lpszFiles;
                            unsigned int n = 0;
                            size_t numOfChars = 0;

                            /* Truncate the path, if needed */
                            szPath[ofn.nFileOffset - 1] = L'\0';

                            while (*lpszTmp)
                            {
                                ++n;
                                numOfChars += wcslen(lpszTmp)+1 + 3; // 3 = 2 quotation marks + 1 space.
                                lpszTmp += wcslen(lpszTmp)+1;
                            }

                            lpszFilePatterns = (LPWSTR)MemAlloc(HEAP_ZERO_MEMORY, numOfChars*sizeof(WCHAR));

                            if (n >= 2)
                            {
                                while (*lpszFiles)
                                {
                                    wcscat(lpszFilePatterns, L"\"");
                                    wcscat(lpszFilePatterns, lpszFiles);
                                    wcscat(lpszFilePatterns, L"\"");

                                    lpszFiles += wcslen(lpszFiles)+1;
                                    if (*lpszFiles)
                                        wcscat(lpszFilePatterns, L" ");
                                }
                            }
                            else
                            {
                                wcscpy(lpszFilePatterns, lpszFiles);
                            }

                            Edit_SetText(GetDlgItem(hDlg, IDC_TXT_FILE_TO_RESTORE), lpszFilePatterns);
                            AddStringToComboList(GetDlgItem(hDlg, IDC_DRP_DEST_DIR), szPath);

                            SetFocus(GetDlgItem(hDlg, IDC_TXT_FILE_TO_RESTORE));
                            Edit_SetSel(GetDlgItem(hDlg, IDC_TXT_FILE_TO_RESTORE), 0, -1);

                            MemFree(lpszFilePatterns);
                        }
                    }

                    MemFree(szPath);

                    break;
                }

                case IDC_BTN_BROWSE_CAB_FILES:
                {
                    OPENFILENAMEW ofn;
                    WCHAR szPath[MAX_PATH] = L"";

                    SecureZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner   = hDlg;
                    ofn.lpstrTitle  = L"Open an archive file"; // L"Ouvrir un fichier archive"; // FIXME: Localize!
                    ofn.Flags       = OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_READONLY;
                    // ofn.FlagsEx     = OFN_EX_NOPLACESBAR;
                    ofn.lpstrFilter = L"Cabinet files (*.cab)\0*.cab\0";
                    ofn.lpstrDefExt = L"cab";
                    ofn.nFilterIndex = 0;
                    ofn.lpstrFile   = szPath;
                    ofn.nMaxFile    = ARRAYSIZE(szPath);

                    if (GetOpenFileName(&ofn))
                    {
                        AddStringToComboList(GetDlgItem(hDlg, IDC_DRP_CAB_FILE), szPath);
                        SetFocus(GetDlgItem(hDlg, IDC_DRP_CAB_FILE));
                    }

                    break;
                }

                case IDC_BTN_BROWSE_DIRS:
                {
                    BROWSEINFOW bi;
                    WCHAR szPath[MAX_PATH] = L"";

                    SecureZeroMemory(&bi, sizeof(bi));
                    bi.hwndOwner = hDlg;
                    bi.pidlRoot  = NULL;
                    bi.lpszTitle = L"Select the directory where the restored files should be stored:";
                                // L"Choisissez le répertoire dans lequel doivent être enregistrés les fichiers restaurés :"; // FIXME: Localize!
                    bi.ulFlags   = BIF_USENEWUI | BIF_RETURNONLYFSDIRS | BIF_SHAREABLE | BIF_VALIDATE /* | BIF_BROWSEFILEJUNCTIONS <--- only in Windows 7+ */;

                    if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
                    {
                        /*PIDLIST_ABSOLUTE*/ LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
                        if (SHGetPathFromIDListW(pidl, szPath))
                        {
                            AddStringToComboList(GetDlgItem(hDlg, IDC_DRP_DEST_DIR), szPath);
                            SetFocus(GetDlgItem(hDlg, IDC_DRP_DEST_DIR));
                        }

                        CoTaskMemFree(pidl);
                        CoUninitialize();
                    }

                    break;
                }

                default:
                    //break;
                    return FALSE;
            }
        }
    }

    return FALSE;
}
