/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/generalpage.c
 * PURPOSE:     General page message handler
 * COPYRIGHT:   Copyright 2005-2006 Christoph von Wittich <Christoph@ApiViewer.de>
 *              Copyright 2011-2012 Hermes BELUSCA - MAITO <hermes.belusca@sfr.fr>
 */

#include "precomp.h"
#include "fileutils.h"
#include "utils.h"
#include "comctl32supp.h"
#include "fileextractdialog.h"

static LPCWSTR lpszRestoreProgPath1 = L"%SystemRoot%\\System32\\rstrui.exe";
static LPCWSTR lpszRestoreProgPath2 = L"%SystemRoot%\\System32\\restore\\rstrui.exe";

static HWND hGeneralPage       = NULL;
static BOOL bIsOriginalBootIni    = TRUE;
// static BOOL bIsStartupNotModified = TRUE;

static VOID EnableSelectiveStartupControls(BOOL bEnable)
{
    assert(hGeneralPage);

    EnableWindow(GetDlgItem(hGeneralPage, IDC_CBX_LOAD_SYSTEM_SERVICES), bEnable);
    EnableWindow(GetDlgItem(hGeneralPage, IDC_CBX_LOAD_STARTUP_ITEMS)  , bEnable);

    EnableWindow(GetDlgItem(hGeneralPage, IDC_CBX_USE_ORIGINAL_BOOTCFG), bEnable);

    // EnableWindow(GetDlgItem(hGeneralPage, IDC_RB_USE_ORIGINAL_BOOTCAT), bEnable);
    // EnableWindow(GetDlgItem(hGeneralPage, IDC_RB_USE_MODIFIED_BOOTCAT), (bEnable ? !bIsOriginalBootIni : FALSE));

    EnableWindow(GetDlgItem(hGeneralPage, IDC_CBX_SYSTEM_INI), bEnable);
    EnableWindow(GetDlgItem(hGeneralPage, IDC_CBX_WIN_INI)   , bEnable);

    return;
}

static VOID CheckSelectiveStartupControls(BOOL bCheck)
{
    assert(hGeneralPage);

    Button_SetCheck(GetDlgItem(hGeneralPage, IDC_CBX_LOAD_SYSTEM_SERVICES), (bCheck ? BST_CHECKED : BST_UNCHECKED));
    Button_SetCheck(GetDlgItem(hGeneralPage, IDC_CBX_LOAD_STARTUP_ITEMS)  , (bCheck ? BST_CHECKED : BST_UNCHECKED));
    Button_SetCheck(GetDlgItem(hGeneralPage, IDC_CBX_USE_ORIGINAL_BOOTCFG), (bCheck ? BST_CHECKED : BST_UNCHECKED));
    Button_SetCheck(GetDlgItem(hGeneralPage, IDC_CBX_SYSTEM_INI)          , (bCheck ? BST_CHECKED : BST_UNCHECKED));
    Button_SetCheck(GetDlgItem(hGeneralPage, IDC_CBX_WIN_INI)             , (bCheck ? BST_CHECKED : BST_UNCHECKED));

    return;
}

INT_PTR CALLBACK
GeneralPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
        {
            hGeneralPage = hDlg;
            PropSheet_UnChanged(GetParent(hGeneralPage), hGeneralPage);

            /* Search for the restore program and enable its button if needed */
            if ( MyFileExists(lpszRestoreProgPath1, NULL) ||
                 MyFileExists(lpszRestoreProgPath2, NULL) )
                Button_Enable(GetDlgItem(hGeneralPage, IDC_BTN_SYSTEM_RESTORE_START), TRUE);
            else
                Button_Enable(GetDlgItem(hGeneralPage, IDC_BTN_SYSTEM_RESTORE_START), FALSE);

#if 0
            /* FIXME */
            SendDlgItemMessage(hDlg, IDC_RB_NORMAL_STARTUP, BM_SETCHECK, BST_CHECKED, 0);
            EnableCheckboxControls(hDlg, FALSE);
#endif

            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_RB_NORMAL_STARTUP:
                {
                    /* Be sure that only this button is activated and the others are not */
                    CheckRadioButton(hGeneralPage, IDC_RB_NORMAL_STARTUP, IDC_RB_SELECTIVE_STARTUP, IDC_RB_NORMAL_STARTUP);

                    bIsOriginalBootIni = TRUE;
                    EnableSelectiveStartupControls(FALSE);
                    CheckSelectiveStartupControls(TRUE);

                    Button_SetCheck(GetDlgItem(hGeneralPage, IDC_CBX_USE_ORIGINAL_BOOTCFG), (bIsOriginalBootIni ? BST_CHECKED : BST_UNCHECKED));

                    PropSheet_Changed(GetParent(hGeneralPage), hGeneralPage);
                    break;
                }

                case IDC_RB_DIAGNOSTIC_STARTUP:
                {
                    /* Be sure that only this button is activated and the others are not */
                    CheckRadioButton(hGeneralPage, IDC_RB_NORMAL_STARTUP, IDC_RB_SELECTIVE_STARTUP, IDC_RB_DIAGNOSTIC_STARTUP);

                    EnableSelectiveStartupControls(FALSE);
                    CheckSelectiveStartupControls(FALSE);

                    PropSheet_Changed(GetParent(hGeneralPage), hGeneralPage);
                    break;
                }

                case IDC_RB_SELECTIVE_STARTUP:
                {
                    /* Be sure that only this button is activated and the others are not */
                    CheckRadioButton(hGeneralPage, IDC_RB_NORMAL_STARTUP, IDC_RB_SELECTIVE_STARTUP, IDC_RB_SELECTIVE_STARTUP);

                    EnableSelectiveStartupControls(TRUE);
                    PropSheet_Changed(GetParent(hGeneralPage), hGeneralPage);

                    break;
                }

                case IDC_CBX_USE_ORIGINAL_BOOTCFG:
                {
                    bIsOriginalBootIni = TRUE;

                    Button_SetCheck(GetDlgItem(hGeneralPage, IDC_CBX_USE_ORIGINAL_BOOTCFG), (bIsOriginalBootIni ? BST_CHECKED : BST_UNCHECKED));
                    EnableWindow(GetDlgItem(hGeneralPage, IDC_CBX_USE_ORIGINAL_BOOTCFG), !bIsOriginalBootIni /*FALSE*/);

                    PropSheet_Changed(GetParent(hGeneralPage), hGeneralPage);

                    break;
                }

                case IDC_BTN_SYSTEM_RESTORE_START:
                {
                    // NOTE: 'err' variable defined for debugging purposes only.
                    DWORD err = RunCommand(lpszRestoreProgPath1, NULL, SW_SHOW);
                    if (err == ERROR_FILE_NOT_FOUND)
                        err = RunCommand(lpszRestoreProgPath2, NULL, SW_SHOW);

                    break;
                }

                case IDC_BTN_FILE_EXTRACTION:
                    DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_FILE_EXTRACT_DIALOG), hGeneralPage /* GetParent(hGeneralPage) */, FileExtractDialogWndProc);
                    break;

                default:
                    //break;
                    return FALSE;
            }
            break;
        }

        case WM_NOTIFY:
        {
            switch (((LPNMHDR)lParam)->code)
            {
                case PSN_APPLY:
                {
                    // TODO: Try to apply the modifications to the system.
                    PropSheet_UnChanged(GetParent(hGeneralPage), hGeneralPage);
                    return TRUE;
                }

                case PSN_HELP:
                {
                    MessageBoxW(hGeneralPage, L"Help not implemented yet!", L"Help", MB_ICONINFORMATION | MB_OK);
                    return TRUE;
                }

                case PSN_KILLACTIVE: // Is going to lose activation.
                {
                    // Changes are always valid of course.
                    SetWindowLongPtr(hGeneralPage, DWLP_MSGRESULT, FALSE);
                    return TRUE;
                }

                case PSN_QUERYCANCEL:
                {
                    // Allows cancellation.
                    SetWindowLongPtr(hGeneralPage, DWLP_MSGRESULT, FALSE);
                    return TRUE;
                }

                case PSN_QUERYINITIALFOCUS:
                {
                    // SetWindowLongPtr(hGeneralPage, DWLP_MSGRESULT,
                    //                  (LONG_PTR)GetDlgItem(hGeneralPage, (bIsOriginalBootIni ? IDC_RB_NORMAL_STARTUP : IDC_RB_SELECTIVE_STARTUP)));
                    return TRUE;
                }

                //
                // DO NOT TOUCH THESE NEXT MESSAGES, THEY ARE OK LIKE THIS...
                //
                case PSN_RESET: // Perform final cleaning, called before WM_DESTROY.
                    return TRUE;

                case PSN_SETACTIVE: // Is going to gain activation.
                {
                    SetWindowLongPtr(hGeneralPage, DWLP_MSGRESULT, 0);
                    return TRUE;
                }

                default:
                    break;
            }

            return FALSE;
        }

        default:
            return FALSE;
    }

    return FALSE;
}
