/* Copyright Krzysztof Kowalczyk 2006-2007
   License: GPLv2 */
#include "SumatraDialogs.h"

#include "DisplayModel.h"
#include "dstring.h"
#include "Resource.h"
#include "win_util.h"
#include <assert.h>

static BOOL CALLBACK Dialog_GetPassword_Proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND                       edit;
    HWND                       label;
    DString                    ds;
    Dialog_GetPassword_Data *  data;

    switch (message)
    {
        case WM_INITDIALOG:
            /* TODO: intelligently center the dialog within the parent window? */
            data = (Dialog_GetPassword_Data*)lParam;
            assert(data);
            if (!data)
                return FALSE;
            assert(data->fileName);
            assert(!data->pwdOut);
            SetWindowLongPtr(hDlg, GWL_USERDATA, (LONG_PTR)data);
            DStringInit(&ds);
            DStringSprintf(&ds, "Enter password for %s", data->fileName);
            label = GetDlgItem(hDlg, IDC_GET_PASSWORD_LABEL);
            win_set_text(label, (TCHAR*)ds.pString); /* @note: TCHAR* cast */
            DStringFree(&ds);
            edit = GetDlgItem(hDlg, IDC_GET_PASSWORD_EDIT);
            win_set_text(edit, TEXT("")); /* @note: TEXT() cast */
            SetFocus(edit);
            return FALSE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    data = (Dialog_GetPassword_Data*)GetWindowLongPtr(hDlg, GWL_USERDATA);
                    assert(data);
                    if (!data)
                        return TRUE;
                    edit = GetDlgItem(hDlg, IDC_GET_PASSWORD_EDIT);
                    data->pwdOut = (char*)win_get_text(edit); /* @note: char* cast */
                    EndDialog(hDlg, DIALOG_OK_PRESSED);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hDlg, DIALOG_CANCEL_PRESSED);
                    return TRUE;
            }
            break;
    }
    return FALSE;
}

/* Shows a 'get password' dialog for a given file.
   Returns a password entered by user as a newly allocated string or
   NULL if user cancelled the dialog or there was an error.
   Caller needs to free() the result.
*/
char *Dialog_GetPassword(WindowInfo *win, const char *fileName)
{
    int                     dialogResult;
    Dialog_GetPassword_Data data;
    
    assert(fileName);
    if (!fileName) return NULL;

    data.fileName = fileName;
    data.pwdOut = NULL;
    dialogResult = DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_DIALOG_GET_PASSWORD), win->hwndFrame, Dialog_GetPassword_Proc, (LPARAM)&data);
    if (DIALOG_OK_PRESSED == dialogResult) {
        return data.pwdOut;
    }
    free((void*)data.pwdOut);
    return NULL;
}

static BOOL CALLBACK Dialog_GoToPage_Proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND                    editPageNo;
    HWND                    labelOfPages;
    DString                 ds;
    TCHAR *                 newPageNoTxt;
    Dialog_GoToPage_Data *  data;

    switch (message)
    {
        case WM_INITDIALOG:
            /* TODO: intelligently center the dialog within the parent window? */
            data = (Dialog_GoToPage_Data*)lParam;
            assert(NULL != data);
            if (!data)
                return FALSE;
            SetWindowLongPtr(hDlg, GWL_USERDATA, (LONG_PTR)data);
            assert(INVALID_PAGE_NO != data->currPageNo);
            assert(data->pageCount >= 1);
            DStringInit(&ds);
            DStringSprintf(&ds, "%d", data->currPageNo);
            editPageNo = GetDlgItem(hDlg, IDC_GOTO_PAGE_EDIT);
            win_set_text(editPageNo, (TCHAR*)ds.pString); /* @note: TCHAR* cast */
            DStringFree(&ds);
            DStringSprintf(&ds, "(of %d)", data->pageCount);
            labelOfPages = GetDlgItem(hDlg, IDC_GOTO_PAGE_LABEL_OF);
            win_set_text(labelOfPages, (TCHAR*)ds.pString); /* @note: TCHAR* cast */
            DStringFree(&ds);
            win_edit_select_all(editPageNo);
            SetFocus(editPageNo);
            return FALSE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    data = (Dialog_GoToPage_Data*)GetWindowLongPtr(hDlg, GWL_USERDATA);
                    assert(data);
                    if (!data)
                        return TRUE;
                    data->pageEnteredOut = INVALID_PAGE_NO;
                    editPageNo = GetDlgItem(hDlg, IDC_GOTO_PAGE_EDIT);
                    newPageNoTxt = win_get_text(editPageNo);
                    if (newPageNoTxt) {
                        data->pageEnteredOut = atoi((char*)newPageNoTxt); /* @note: char* cast */
                        free((void*)newPageNoTxt);
                    }
                    EndDialog(hDlg, DIALOG_OK_PRESSED);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hDlg, DIALOG_CANCEL_PRESSED);
                    return TRUE;
            }
            break;
    }
    return FALSE;
}

/* Shows a 'go to page' dialog and returns a page number entered by the user
   or INVALID_PAGE_NO if user clicked "cancel" button, entered invalid
   page number or there was an error. */
int Dialog_GoToPage(WindowInfo *win)
{
    int                     dialogResult;
    Dialog_GoToPage_Data    data;
    
    assert(win);
    if (!win) return INVALID_PAGE_NO;

    data.currPageNo = win->dm->startPage();
    data.pageCount = win->dm->pageCount();
    dialogResult = DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_DIALOG_GOTO_PAGE), win->hwndFrame, Dialog_GoToPage_Proc, (LPARAM)&data);
    if (DIALOG_OK_PRESSED == dialogResult) {
        if (win->dm->validPageNo(data.pageEnteredOut)) {
            return data.pageEnteredOut;
        }
    }
    return INVALID_PAGE_NO;
}

static BOOL CALLBACK Dialog_PdfAssociate_Proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    Dialog_PdfAssociate_Data *  data;

    switch (message)
    {
        case WM_INITDIALOG:
            /* TODO: intelligently center the dialog within the parent window? */
            data = (Dialog_PdfAssociate_Data*)lParam;
            assert(NULL != data);
            SetWindowLongPtr(hDlg, GWL_USERDATA, (LONG_PTR)data);
            CheckDlgButton(hDlg, IDC_DONT_ASK_ME_AGAIN, BST_UNCHECKED);
            SetFocus(GetDlgItem(hDlg, IDOK));
            return FALSE;

        case WM_COMMAND:
            data = (Dialog_PdfAssociate_Data*)GetWindowLongPtr(hDlg, GWL_USERDATA);
            assert(data);
            if (!data)
                return TRUE;
            data->dontAskAgain = FALSE;
            switch (LOWORD(wParam))
            {
                case IDOK:
                    data = (Dialog_PdfAssociate_Data*)GetWindowLongPtr(hDlg, GWL_USERDATA);
                    assert(data);
                    if (!data)
                        return TRUE;
                    if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_DONT_ASK_ME_AGAIN))
                        data->dontAskAgain = TRUE;
                    EndDialog(hDlg, DIALOG_OK_PRESSED);
                    return TRUE;

                case IDCANCEL:
                    if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_DONT_ASK_ME_AGAIN))
                        data->dontAskAgain = TRUE;
                    EndDialog(hDlg, DIALOG_NO_PRESSED);
                    return TRUE;

                case IDC_DONT_ASK_ME_AGAIN:
                    data = NULL;
                    return TRUE;
            }
            break;
    }
    return FALSE;
}

/* Show "associate this application with PDF files" dialog.
   Returns DIALOG_YES_PRESSED if "Yes" button was pressed or
   DIALOG_NO_PRESSED if "No" button was pressed.
   Returns the state of "don't ask me again" checkbox" in <dontAskAgain> */
int Dialog_PdfAssociate(HWND hwnd, BOOL *dontAskAgainOut)
{
    assert(dontAskAgainOut);

    Dialog_PdfAssociate_Data data;
    int dialogResult = DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_DIALOG_PDF_ASSOCIATE), hwnd, Dialog_PdfAssociate_Proc, (LPARAM)&data);
    if (dontAskAgainOut)
        *dontAskAgainOut = data.dontAskAgain;
    return dialogResult;
}

