/*
 * SHFileOperation
 *
 * Copyright 2000 Juergen Schmied
 * Copyright 2002 Andriy Palamarchuk
 * Copyright 2004 Dietrich Teickner (from Odin)
 * Copyright 2004 Rolf Kalbermatter
 * Copyright 2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define IsAttrib(x, y)  ((INVALID_FILE_ATTRIBUTES != (x)) && ((x) & (y)))
#define IsAttribFile(x) (!((x) & FILE_ATTRIBUTE_DIRECTORY))
#define IsAttribDir(x)  IsAttrib(x, FILE_ATTRIBUTE_DIRECTORY)
#define IsDotDir(x)     ((x[0] == '.') && ((x[1] == 0) || ((x[1] == '.') && (x[2] == 0))))

#define FO_MASK         0xF

#define NEW_FILENAME_ON_COPY_TRIES 100

static const WCHAR wWildcardFile[] = {'*',0};
static const WCHAR wWildcardChars[] = {'*','?',0};

typedef struct
{
    SHFILEOPSTRUCTW *req;
    DWORD dwYesToAllMask;
    BOOL bManyItems;
    BOOL bCancelled;
    IProgressDialog *progress;
    ULARGE_INTEGER completedSize;
    ULARGE_INTEGER totalSize;
    WCHAR szBuilderString[50];
} FILE_OPERATION;

#define ERROR_SHELL_INTERNAL_FILE_NOT_FOUND 1026

typedef struct
{
    DWORD attributes;
    LPWSTR szDirectory;
    LPWSTR szFilename;
    LPWSTR szFullPath;
    BOOL bFromWildcard;
    BOOL bFromRelative;
    BOOL bExists;
} FILE_ENTRY;

typedef struct
{
    FILE_ENTRY *feFiles;
    DWORD num_alloc;
    DWORD dwNumFiles;
    BOOL bAnyFromWildcard;
    BOOL bAnyDirectories;
    BOOL bAnyDontExist;
} FILE_LIST;

static DWORD SHNotifyCreateDirectoryW(LPCWSTR path, LPSECURITY_ATTRIBUTES sec);
static DWORD SHNotifyRemoveDirectoryW(LPCWSTR path);
static DWORD SHNotifyDeleteFileW(FILE_OPERATION *op, LPCWSTR path);
static DWORD SHNotifyMoveFileW(FILE_OPERATION *op, LPCWSTR src, LPCWSTR dest, BOOL isdir);
static DWORD SHNotifyCopyFileW(FILE_OPERATION *op, LPCWSTR src, LPCWSTR dest, BOOL bFailIfExists);
static DWORD SHFindAttrW(LPCWSTR pName, BOOL fileOnly);
static HRESULT copy_files(FILE_OPERATION *op, BOOL multiDest, const FILE_LIST *flFrom, FILE_LIST *flTo);
static DWORD move_files(FILE_OPERATION *op, BOOL multiDest, const FILE_LIST *flFrom, const FILE_LIST *flTo);

DWORD WINAPI _FileOpCountManager(FILE_OPERATION *op, const FILE_LIST *flFrom);
static BOOL _FileOpCount(FILE_OPERATION *op, LPWSTR pwszBuf, BOOL bFolder, DWORD *ticks);

/* Confirm dialogs with an optional "Yes To All" as used in file operations confirmations
 */
static const WCHAR CONFIRM_MSG_PROP[] = {'W','I','N','E','_','C','O','N','F','I','R','M',0};

struct confirm_msg_info
{
    LPWSTR lpszText;
    LPWSTR lpszCaption;
    HICON hIcon;
    BOOL bYesToAll;
};

/* as some buttons may be hidden and the dialog height may change we may need
 * to move the controls */
static void confirm_msg_move_button(HWND hDlg, INT iId, INT *xPos, INT yOffset, BOOL bShow)
{
    HWND hButton = GetDlgItem(hDlg, iId);
    RECT r;

    if (bShow)
    {
        POINT pt;
        int width;

        GetWindowRect(hButton, &r);
        width = r.right - r.left;
        pt.x = r.left;
        pt.y = r.top;
        ScreenToClient(hDlg, &pt);
        MoveWindow(hButton, *xPos - width, pt.y - yOffset, width, r.bottom - r.top, FALSE);
        *xPos -= width + 5;
    }
    else
        ShowWindow(hButton, SW_HIDE);
}

/* Note: we paint the text manually and don't use the static control to make
 * sure the text has the same height as the one computed in WM_INITDIALOG
 */
static INT_PTR ConfirmMsgBox_Paint(HWND hDlg)
{
    PAINTSTRUCT ps;
    HFONT hOldFont;
    RECT r;
    HDC hdc;

    BeginPaint(hDlg, &ps);
    hdc = ps.hdc;
    SetBkMode(hdc, TRANSPARENT);

    GetClientRect(GetDlgItem(hDlg, IDC_YESTOALL_MESSAGE), &r);
    /* this will remap the rect to dialog coords */
    MapWindowPoints(GetDlgItem(hDlg, IDC_YESTOALL_MESSAGE), hDlg, (LPPOINT)&r, 2);
    hOldFont = (HFONT)SelectObject(hdc, (HFONT)SendDlgItemMessageW(hDlg, IDC_YESTOALL_MESSAGE, WM_GETFONT, 0, 0));
    DrawTextW(hdc, (LPWSTR)GetPropW(hDlg, CONFIRM_MSG_PROP), -1, &r, DT_NOPREFIX | DT_PATH_ELLIPSIS | DT_WORDBREAK);
    SelectObject(hdc, hOldFont);
    EndPaint(hDlg, &ps);

    return TRUE;
}

static INT_PTR ConfirmMsgBox_Init(HWND hDlg, LPARAM lParam)
{
    struct confirm_msg_info *info = (struct confirm_msg_info *)lParam;
    INT xPos, yOffset;
    int width, height;
    HFONT hOldFont;
    HDC hdc;
    RECT r;

    SetWindowTextW(hDlg, info->lpszCaption);
    ShowWindow(GetDlgItem(hDlg, IDC_YESTOALL_MESSAGE), SW_HIDE);
    SetPropW(hDlg, CONFIRM_MSG_PROP, info->lpszText);
    SendDlgItemMessageW(hDlg, IDC_YESTOALL_ICON, STM_SETICON, (WPARAM)info->hIcon, 0);

    /* compute the text height and resize the dialog */
    GetClientRect(GetDlgItem(hDlg, IDC_YESTOALL_MESSAGE), &r);
    hdc = GetDC(hDlg);
    yOffset = r.bottom;
    hOldFont = (HFONT)SelectObject(hdc, (HFONT)SendDlgItemMessageW(hDlg, IDC_YESTOALL_MESSAGE, WM_GETFONT, 0, 0));
    DrawTextW(hdc, info->lpszText, -1, &r, DT_NOPREFIX | DT_PATH_ELLIPSIS | DT_WORDBREAK | DT_CALCRECT);
    SelectObject(hdc, hOldFont);
    yOffset -= r.bottom;
    yOffset = min(yOffset, 35);  /* don't make the dialog too small */
    ReleaseDC(hDlg, hdc);

    GetClientRect(hDlg, &r);
    xPos = r.right - 7;
    GetWindowRect(hDlg, &r);
    width = r.right - r.left;
    height = r.bottom - r.top - yOffset;
    MoveWindow(hDlg, (GetSystemMetrics(SM_CXSCREEN) - width)/2,
        (GetSystemMetrics(SM_CYSCREEN) - height)/2, width, height, FALSE);

    confirm_msg_move_button(hDlg, IDCANCEL,     &xPos, yOffset, info->bYesToAll);
    confirm_msg_move_button(hDlg, IDNO,         &xPos, yOffset, TRUE);
    confirm_msg_move_button(hDlg, IDC_YESTOALL, &xPos, yOffset, info->bYesToAll);
    confirm_msg_move_button(hDlg, IDYES,        &xPos, yOffset, TRUE);

    return TRUE;
}

static INT_PTR CALLBACK ConfirmMsgBoxProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            return ConfirmMsgBox_Init(hDlg, lParam);
        case WM_PAINT:
            return ConfirmMsgBox_Paint(hDlg);
        case WM_COMMAND:
            EndDialog(hDlg, wParam);
            break;
        case WM_CLOSE:
            EndDialog(hDlg, IDCANCEL);
            break;
    }
    return FALSE;
}

int SHELL_ConfirmMsgBox(HWND hWnd, LPWSTR lpszText, LPWSTR lpszCaption, HICON hIcon, BOOL bYesToAll)
{
    struct confirm_msg_info info;

    info.lpszText = lpszText;
    info.lpszCaption = lpszCaption;
    info.hIcon = hIcon;
    info.bYesToAll = bYesToAll;
    return DialogBoxParamW(shell32_hInstance, MAKEINTRESOURCEW(IDD_YESTOALL_MSGBOX), hWnd, ConfirmMsgBoxProc, (LPARAM)&info);
}

/* confirmation dialogs content */
typedef struct
{
    HINSTANCE hIconInstance;
    UINT icon_resource_id;
    UINT caption_resource_id, text_resource_id;
} SHELL_ConfirmIDstruc;

static BOOL SHELL_ConfirmIDs(int nKindOfDialog, SHELL_ConfirmIDstruc *ids)
{
    ids->hIconInstance = shell32_hInstance;
    switch (nKindOfDialog)
    {
        case ASK_DELETE_FILE:
            ids->icon_resource_id = IDI_SHELL_CONFIRM_DELETE;
            ids->caption_resource_id  = IDS_DELETEITEM_CAPTION;
            ids->text_resource_id  = IDS_DELETEITEM_TEXT;
            return TRUE;

        case ASK_DELETE_FOLDER:
            ids->icon_resource_id = IDI_SHELL_CONFIRM_DELETE;
            ids->caption_resource_id  = IDS_DELETEFOLDER_CAPTION;
            ids->text_resource_id  = IDS_DELETEITEM_TEXT;
            return TRUE;

        case ASK_DELETE_MULTIPLE_ITEM:
            ids->icon_resource_id = IDI_SHELL_CONFIRM_DELETE;
            ids->caption_resource_id  = IDS_DELETEITEM_CAPTION;
            ids->text_resource_id  = IDS_DELETEMULTIPLE_TEXT;
            return TRUE;

        case ASK_TRASH_FILE:
            ids->icon_resource_id = IDI_SHELL_TRASH_FILE;
            ids->caption_resource_id = IDS_DELETEITEM_CAPTION;
            ids->text_resource_id = IDS_TRASHITEM_TEXT;
            return TRUE;

        case ASK_TRASH_FOLDER:
            ids->icon_resource_id = IDI_SHELL_TRASH_FILE;
            ids->caption_resource_id = IDS_DELETEFOLDER_CAPTION;
            ids->text_resource_id = IDS_TRASHFOLDER_TEXT;
            return TRUE;

        case ASK_TRASH_MULTIPLE_ITEM:
            ids->icon_resource_id = IDI_SHELL_TRASH_FILE;
            ids->caption_resource_id = IDS_DELETEITEM_CAPTION;
            ids->text_resource_id = IDS_TRASHMULTIPLE_TEXT;
            return TRUE;

        case ASK_CANT_TRASH_ITEM:
            ids->icon_resource_id = IDI_SHELL_CONFIRM_DELETE;
            ids->caption_resource_id  = IDS_DELETEITEM_CAPTION;
            ids->text_resource_id  = IDS_CANTTRASH_TEXT;
            return TRUE;

        case ASK_DELETE_SELECTED:
            ids->icon_resource_id = IDI_SHELL_CONFIRM_DELETE;
            ids->caption_resource_id  = IDS_DELETEITEM_CAPTION;
            ids->text_resource_id  = IDS_DELETESELECTED_TEXT;
            return TRUE;

      case ASK_OVERWRITE_FILE:
            ids->icon_resource_id = IDI_SHELL_FOLDER_MOVE2;
            ids->caption_resource_id  = IDS_OVERWRITEFILE_CAPTION;
            ids->text_resource_id  = IDS_OVERWRITEFILE_TEXT;
            return TRUE;

      case ASK_OVERWRITE_FOLDER:
            ids->icon_resource_id = IDI_SHELL_FOLDER_MOVE2;
            ids->caption_resource_id  = IDS_OVERWRITEFILE_CAPTION;
            ids->text_resource_id  = IDS_OVERWRITEFOLDER_TEXT;
            return TRUE;

        default:
            FIXME(" Unhandled nKindOfDialog %d stub\n", nKindOfDialog);
    }
    return FALSE;
}

static BOOL SHELL_ConfirmDialogW(HWND hWnd, int nKindOfDialog, LPCWSTR szDir, FILE_OPERATION *op)
{
    WCHAR szCaption[255], szText[255], szBuffer[MAX_PATH + 256];
    SHELL_ConfirmIDstruc ids;
    DWORD_PTR args[1];
    HICON hIcon;
    int ret;

    assert(nKindOfDialog >= 0 && nKindOfDialog < 32);
    if (op && (op->dwYesToAllMask & (1 << nKindOfDialog)))
        return TRUE;

    if (!SHELL_ConfirmIDs(nKindOfDialog, &ids)) return FALSE;

    LoadStringW(shell32_hInstance, ids.caption_resource_id, szCaption, sizeof(szCaption)/sizeof(WCHAR));
    LoadStringW(shell32_hInstance, ids.text_resource_id, szText, sizeof(szText)/sizeof(WCHAR));

    args[0] = (DWORD_PTR)szDir;
    FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
                   szText, 0, 0, szBuffer, sizeof(szBuffer), (va_list*)args);
    hIcon = LoadIconW(ids.hIconInstance, (LPWSTR)MAKEINTRESOURCE(ids.icon_resource_id));

    ret = SHELL_ConfirmMsgBox(hWnd, szBuffer, szCaption, hIcon, op && op->bManyItems);
    if (op)
    {
        if (ret == IDC_YESTOALL)
        {
            op->dwYesToAllMask |= (1 << nKindOfDialog);
            ret = IDYES;
        }
        if (ret == IDCANCEL)
            op->bCancelled = TRUE;
        if (ret != IDYES)
            op->req->fAnyOperationsAborted = TRUE;
    }
    return ret == IDYES;
}

BOOL SHELL_ConfirmYesNoW(HWND hWnd, int nKindOfDialog, LPCWSTR szDir)
{
    return SHELL_ConfirmDialogW(hWnd, nKindOfDialog, szDir, NULL);
}

static DWORD SHELL32_AnsiToUnicodeBuf(LPCSTR aPath, LPWSTR *wPath, DWORD minChars)
{
    DWORD len = MultiByteToWideChar(CP_ACP, 0, aPath, -1, NULL, 0);

    if (len < minChars)
      len = minChars;

    *wPath = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (*wPath)
    {
      MultiByteToWideChar(CP_ACP, 0, aPath, -1, *wPath, len);
      return NO_ERROR;
    }
    return E_OUTOFMEMORY;
}

static void SHELL32_FreeUnicodeBuf(LPWSTR wPath)
{
    HeapFree(GetProcessHeap(), 0, wPath);
}

EXTERN_C HRESULT WINAPI SHIsFileAvailableOffline(LPCWSTR path, LPDWORD status)
{
    FIXME("(%s, %p) stub\n", debugstr_w(path), status);
    return E_FAIL;
}

/**************************************************************************
 * SHELL_DeleteDirectory()  [internal]
 *
 * Asks for confirmation when bShowUI is true and deletes the directory and
 * all its subdirectories and files if necessary.
 */
BOOL SHELL_DeleteDirectoryW(FILE_OPERATION *op, LPCWSTR pszDir, BOOL bShowUI)
{
    BOOL    ret = TRUE;
    HANDLE  hFind;
    WIN32_FIND_DATAW wfd;
    WCHAR   szTemp[MAX_PATH];

    /* Make sure the directory exists before eventually prompting the user */
    PathCombineW(szTemp, pszDir, wWildcardFile);
    hFind = FindFirstFileW(szTemp, &wfd);
    if (hFind == INVALID_HANDLE_VALUE)
      return FALSE;

    if (!bShowUI || (ret = SHELL_ConfirmDialogW(op->req->hwnd, ASK_DELETE_FOLDER, pszDir, NULL)))
    {
        do
        {
            if (IsDotDir(wfd.cFileName))
                continue;
            PathCombineW(szTemp, pszDir, wfd.cFileName);
            if (FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes)
                ret = SHELL_DeleteDirectoryW(op, szTemp, FALSE);
            else
                ret = (SHNotifyDeleteFileW(op, szTemp) == ERROR_SUCCESS);

            if (op->progress != NULL)
                op->bCancelled |= op->progress->HasUserCancelled();
        } while (ret && FindNextFileW(hFind, &wfd) && !op->bCancelled);
    }
    FindClose(hFind);
    if (ret)
        ret = (SHNotifyRemoveDirectoryW(pszDir) == ERROR_SUCCESS);
    return ret;
}

/**************************************************************************
 * Win32CreateDirectory      [SHELL32.93]
 *
 * Creates a directory. Also triggers a change notify if one exists.
 *
 * PARAMS
 *  path       [I]   path to directory to create
 *
 * RETURNS
 *  TRUE if successful, FALSE otherwise
 */

static DWORD SHNotifyCreateDirectoryW(LPCWSTR path, LPSECURITY_ATTRIBUTES sec)
{
    TRACE("(%s, %p)\n", debugstr_w(path), sec);

    if (CreateDirectoryW(path, sec))
    {
      SHChangeNotify(SHCNE_MKDIR, SHCNF_PATHW, path, NULL);
      return ERROR_SUCCESS;
    }
    return GetLastError();
}

/**********************************************************************/

EXTERN_C BOOL WINAPI Win32CreateDirectoryW(LPCWSTR path, LPSECURITY_ATTRIBUTES sec)
{
    return (SHNotifyCreateDirectoryW(path, sec) == ERROR_SUCCESS);
}

/************************************************************************
 * Win32RemoveDirectory      [SHELL32.94]
 *
 * Deletes a directory. Also triggers a change notify if one exists.
 *
 * PARAMS
 *  path       [I]   path to directory to delete
 *
 * RETURNS
 *  TRUE if successful, FALSE otherwise
 */
static DWORD SHNotifyRemoveDirectoryW(LPCWSTR path)
{
    BOOL ret;
    TRACE("(%s)\n", debugstr_w(path));

    ret = RemoveDirectoryW(path);
    if (!ret)
    {
      /* Directory may be write protected */
      DWORD dwAttr = GetFileAttributesW(path);
      if (IsAttrib(dwAttr, FILE_ATTRIBUTE_READONLY))
        if (SetFileAttributesW(path, dwAttr & ~FILE_ATTRIBUTE_READONLY))
          ret = RemoveDirectoryW(path);
    }
    if (ret)
    {
      SHChangeNotify(SHCNE_RMDIR, SHCNF_PATHW, path, NULL);
      return ERROR_SUCCESS;
    }
    return GetLastError();
}

/***********************************************************************/

EXTERN_C BOOL WINAPI Win32RemoveDirectoryW(LPCWSTR path)
{
    return (SHNotifyRemoveDirectoryW(path) == ERROR_SUCCESS);
}

static void _SetOperationTitle(FILE_OPERATION *op) {
    if (op->progress == NULL)
        return;
    WCHAR szTitle[50], szPreflight[50];
    UINT animation_id = NULL;

    switch (op->req->wFunc)
    {
        case FO_COPY:
            LoadStringW(shell32_hInstance, IDS_FILEOOP_COPYING, szTitle, sizeof(szTitle)/sizeof(WCHAR));
            LoadStringW(shell32_hInstance, IDS_FILEOOP_FROM_TO, op->szBuilderString, sizeof( op->szBuilderString)/sizeof(WCHAR));
            animation_id = IDA_SHELL_COPY;
            break;
        case FO_DELETE:
            LoadStringW(shell32_hInstance, IDS_FILEOOP_DELETING, szTitle, sizeof(szTitle)/sizeof(WCHAR));
            LoadStringW(shell32_hInstance, IDS_FILEOOP_FROM, op->szBuilderString, sizeof( op->szBuilderString)/sizeof(WCHAR));
            animation_id = IDA_SHELL_DELETE;
            break;
        case FO_MOVE:
            LoadStringW(shell32_hInstance, IDS_FILEOOP_MOVING, szTitle, sizeof(szTitle)/sizeof(WCHAR));
            LoadStringW(shell32_hInstance, IDS_FILEOOP_FROM_TO, op->szBuilderString, sizeof( op->szBuilderString)/sizeof(WCHAR));
            animation_id = IDA_SHELL_COPY;
            break;
        default:
            return;
    }
    LoadStringW(shell32_hInstance, IDS_FILEOOP_PREFLIGHT, szPreflight, sizeof(szPreflight)/sizeof(WCHAR));

    op->progress->SetTitle(szTitle);
    op->progress->SetLine(1, szPreflight, false, NULL);
    op->progress->SetAnimation(shell32_hInstance, animation_id);
}

static void _SetOperationTexts(FILE_OPERATION *op, LPCWSTR src, LPCWSTR dest) {
    if (op->progress == NULL || src == NULL)
        return;
    LPWSTR fileSpecS, pathSpecS, fileSpecD, pathSpecD;
    WCHAR szFolderS[50], szFolderD[50], szFinalString[260];

    DWORD_PTR args[2];

    fileSpecS = (pathSpecS = (LPWSTR) src);
    fileSpecD = (pathSpecD = (LPWSTR) dest);

    // March across the string to get the file path and it's parent dir.
    for (LPWSTR ptr = (LPWSTR) src; *ptr; ptr++) {
        if (*ptr == '\\') {
            pathSpecS = fileSpecS;
            fileSpecS = ptr+1;
        }
    }
    lstrcpynW(szFolderS, pathSpecS, min(50, fileSpecS - pathSpecS));
    args[0] = (DWORD_PTR) szFolderS;

    switch (op->req->wFunc)
    {
        case FO_COPY:
        case FO_MOVE:
            if (dest == NULL)
                return;
            for (LPWSTR ptr = (LPWSTR) dest; *ptr; ptr++) {
                if (*ptr == '\\') {
                    pathSpecD = fileSpecD;
                    fileSpecD = ptr + 1;
                }
            }
            lstrcpynW(szFolderD, pathSpecD, min(50, fileSpecD - pathSpecD));
            args[1] = (DWORD_PTR) szFolderD;
            break;
        case FO_DELETE:
            break;
        default:
            return;
    }

    FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
                   op->szBuilderString, 0, 0, szFinalString, sizeof(szFinalString), (va_list*)args);

    op->progress->SetLine(1, fileSpecS, false, NULL);
    op->progress->SetLine(2, szFinalString, false, NULL);
}


DWORD CALLBACK SHCopyProgressRoutine(
    LARGE_INTEGER TotalFileSize,
    LARGE_INTEGER TotalBytesTransferred,
    LARGE_INTEGER StreamSize,
    LARGE_INTEGER StreamBytesTransferred,
    DWORD dwStreamNumber,
    DWORD dwCallbackReason,
    HANDLE hSourceFile,
    HANDLE hDestinationFile,
    LPVOID lpData
) {
    FILE_OPERATION *op = (FILE_OPERATION *) lpData;

    if (op->progress) {
        /*
         * This is called at the start of each file. To keop less state,
         * I'm adding the file to the completed size here, and the re-subtracting
         * it when drawing the progress bar.
         */
        if (dwCallbackReason & CALLBACK_STREAM_SWITCH)
            op->completedSize.QuadPart += TotalFileSize.QuadPart;

        op->progress->SetProgress64(op->completedSize.QuadPart -
                                    TotalFileSize.QuadPart +
                                    TotalBytesTransferred.QuadPart
                                  , op->totalSize.QuadPart);


        op->bCancelled = op->progress->HasUserCancelled();
    }

    return 0;
}


/************************************************************************
 * SHNotifyDeleteFileW          [internal]
 *
 * Deletes a file. Also triggers a change notify if one exists.
 *
 * PARAMS
 *  op         [I]   File Operation context
 *  path       [I]   path to source file to move
 *
 * RETURNS
 *  ERORR_SUCCESS if successful
 */
static DWORD SHNotifyDeleteFileW(FILE_OPERATION *op, LPCWSTR path)
{
    BOOL ret;

    TRACE("(%s)\n", debugstr_w(path));

    _SetOperationTexts(op, path, NULL);

    LARGE_INTEGER FileSize;
    FileSize.QuadPart = 0;

    WIN32_FIND_DATAW wfd;
    HANDLE hFile = FindFirstFileW(path, &wfd);
    if (hFile != INVALID_HANDLE_VALUE && IsAttribFile(wfd.dwFileAttributes)) {
        ULARGE_INTEGER tmp;
        tmp.u.LowPart  = wfd.nFileSizeLow;
        tmp.u.HighPart = wfd.nFileSizeHigh;
        FileSize.QuadPart = tmp.QuadPart;
    }
    FindClose(hFile);

    ret = DeleteFileW(path);
    if (!ret)
    {
      /* File may be write protected or a system file */
      DWORD dwAttr = GetFileAttributesW(path);
      if (IsAttrib(dwAttr, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM))
        if (SetFileAttributesW(path, dwAttr & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
            ret = DeleteFileW(path);
    }
    if (ret)
    {
        // Bit of a hack to make the progress bar move. We don't have progress inside the file, so inform when done.
        SHCopyProgressRoutine(FileSize, FileSize, FileSize, FileSize, 0, CALLBACK_STREAM_SWITCH, NULL, NULL, op);
        SHChangeNotify(SHCNE_DELETE, SHCNF_PATHW, path, NULL);
        return ERROR_SUCCESS;
    }
    return GetLastError();
}

/************************************************************************
 * Win32DeleteFile           [SHELL32.164]
 *
 * Deletes a file. Also triggers a change notify if one exists.
 *
 * PARAMS
 *  path       [I]   path to file to delete
 *
 * RETURNS
 *  TRUE if successful, FALSE otherwise
 */
EXTERN_C DWORD WINAPI Win32DeleteFileW(LPCWSTR path)
{
    return (SHNotifyDeleteFileW(NULL, path) == ERROR_SUCCESS);
}

#ifdef __REACTOS__
/************************************************************************
 * CheckForError          [internal]
 *
 * Show message box if operation failed
 *
 * PARAMS
 *  op         [I]   File Operation context
 *  error      [I]   Error code
 *  src        [I]   Source file full name
 *
 */
static DWORD CheckForError(FILE_OPERATION *op, DWORD error, LPCWSTR src)
{
    CStringW strTitle, strMask, strText;
    LPWSTR lpMsgBuffer;

    if (error == ERROR_SUCCESS || (op->req->fFlags & (FOF_NOERRORUI | FOF_SILENT)))
        goto exit;

    strTitle.LoadStringW(op->req->wFunc == FO_COPY ? IDS_COPYERRORTITLE : IDS_MOVEERRORTITLE);

    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL,
                   error,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPWSTR)&lpMsgBuffer,
                   0,
                   NULL);

    strText.Format(op->req->wFunc == FO_COPY ? IDS_COPYERROR : IDS_MOVEERROR,
                   PathFindFileNameW(src),
                   lpMsgBuffer);

    MessageBoxW(op->req->hwnd, strText, strTitle, MB_ICONERROR);
    LocalFree(lpMsgBuffer);

exit:
    return error;
}
#endif

/************************************************************************
 * SHNotifyMoveFile          [internal]
 *
 * Moves a file. Also triggers a change notify if one exists.
 *
 * PARAMS
 *  op         [I]   File Operation context
 *  src        [I]   path to source file to move
 *  dest       [I]   path to target file to move to
 *
 * RETURNS
 *  ERROR_SUCCESS if successful
 */
static DWORD SHNotifyMoveFileW(FILE_OPERATION *op, LPCWSTR src, LPCWSTR dest, BOOL isdir)
{
    BOOL ret;

    TRACE("(%s %s)\n", debugstr_w(src), debugstr_w(dest));

    _SetOperationTexts(op, src, dest);

    ret = MoveFileWithProgressW(src, dest, SHCopyProgressRoutine, op, MOVEFILE_REPLACE_EXISTING);

    /* MOVEFILE_REPLACE_EXISTING fails with dirs, so try MoveFile */
    if (!ret)
        ret = MoveFileW(src, dest);

    if (!ret)
    {
      DWORD dwAttr;

      dwAttr = SHFindAttrW(dest, FALSE);
      if (INVALID_FILE_ATTRIBUTES == dwAttr)
      {
        /* Source file may be write protected or a system file */
        dwAttr = GetFileAttributesW(src);
        if (IsAttrib(dwAttr, FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM))
            if (SetFileAttributesW(src, dwAttr & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
                ret = MoveFileW(src, dest);
      }
    }
    if (ret)
    {
        SHChangeNotify(isdir ? SHCNE_MKDIR : SHCNE_CREATE, SHCNF_PATHW, dest, NULL);
        SHChangeNotify(isdir ? SHCNE_RMDIR : SHCNE_DELETE, SHCNF_PATHW, src, NULL);
        return ERROR_SUCCESS;
    }

#ifdef __REACTOS__
    return CheckForError(op, GetLastError(), src);
#else
    return GetLastError();
#endif
}

/************************************************************************
 * SHNotifyCopyFile          [internal]
 *
 * Copies a file. Also triggers a change notify if one exists.
 *
 * PARAMS
 *  src           [I]   path to source file to move
 *  dest          [I]   path to target file to move to
 *  bFailIfExists [I]   if TRUE, the target file will not be overwritten if
 *                      a file with this name already exists
 *
 * RETURNS
 *  ERROR_SUCCESS if successful
 */
static DWORD SHNotifyCopyFileW(FILE_OPERATION *op, LPCWSTR src, LPCWSTR dest, BOOL bFailIfExists)
{
    BOOL ret;
    DWORD attribs;

    TRACE("(%s %s %s)\n", debugstr_w(src), debugstr_w(dest), bFailIfExists ? "failIfExists" : "");

    _SetOperationTexts(op, src, dest);

    /* Destination file may already exist with read only attribute */
    attribs = GetFileAttributesW(dest);
    if (IsAttrib(attribs, FILE_ATTRIBUTE_READONLY))
        SetFileAttributesW(dest, attribs & ~FILE_ATTRIBUTE_READONLY);

    if (GetFileAttributesW(dest) & FILE_ATTRIBUTE_READONLY)
    {
        SetFileAttributesW(dest, attribs & ~FILE_ATTRIBUTE_READONLY);
        if (GetFileAttributesW(dest) & FILE_ATTRIBUTE_READONLY)
        {
            TRACE("[shell32, SHNotifyCopyFileW] STILL SHIT\n");
        }
    }

    ret = CopyFileExW(src, dest, SHCopyProgressRoutine, op, &op->bCancelled, bFailIfExists);
    if (ret)
    {
        SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW, dest, NULL);
        return ERROR_SUCCESS;
    }

#ifdef __REACTOS__
    return CheckForError(op, GetLastError(), src);
#else
    return GetLastError();
#endif
}

/*************************************************************************
 * SHCreateDirectory         [SHELL32.165]
 *
 * This function creates a file system folder whose fully qualified path is
 * given by path. If one or more of the intermediate folders do not exist,
 * they will be created as well.
 *
 * PARAMS
 *  hWnd       [I]
 *  path       [I]   path of directory to create
 *
 * RETURNS
 *  ERROR_SUCCESS or one of the following values:
 *  ERROR_BAD_PATHNAME if the path is relative
 *  ERROR_FILE_EXISTS when a file with that name exists
 *  ERROR_PATH_NOT_FOUND can't find the path, probably invalid
 *  ERROR_INVALID_NAME if the path contains invalid chars
 *  ERROR_ALREADY_EXISTS when the directory already exists
 *  ERROR_FILENAME_EXCED_RANGE if the filename was to long to process
 *
 * NOTES
 *  exported by ordinal
 *  Win9x exports ANSI
 *  WinNT/2000 exports Unicode
 */
int WINAPI SHCreateDirectory(HWND hWnd, LPCWSTR path)
{
     return SHCreateDirectoryExW(hWnd, path, NULL);
}

/*************************************************************************
 * SHCreateDirectoryExA      [SHELL32.@]
 *
 * This function creates a file system folder whose fully qualified path is
 * given by path. If one or more of the intermediate folders do not exist,
 * they will be created as well.
 *
 * PARAMS
 *  hWnd       [I]
 *  path       [I]   path of directory to create
 *  sec        [I]   security attributes to use or NULL
 *
 * RETURNS
 *  ERROR_SUCCESS or one of the following values:
 *  ERROR_BAD_PATHNAME or ERROR_PATH_NOT_FOUND if the path is relative
 *  ERROR_INVALID_NAME if the path contains invalid chars
 *  ERROR_FILE_EXISTS when a file with that name exists
 *  ERROR_ALREADY_EXISTS when the directory already exists
 *  ERROR_FILENAME_EXCED_RANGE if the filename was too long to process
 *
 *  FIXME: Not implemented yet;
 *  SHCreateDirectoryEx also verifies that the files in the directory will be visible
 *  if the path is a network path to deal with network drivers which might have a limited
 *  but unknown maximum path length. If not:
 *
 *  If hWnd is set to a valid window handle, a message box is displayed warning
 *  the user that the files may not be accessible. If the user chooses not to
 *  proceed, the function returns ERROR_CANCELLED.
 *
 *  If hWnd is set to NULL, no user interface is displayed and the function
 *  returns ERROR_CANCELLED.
 */
int WINAPI SHCreateDirectoryExA(HWND hWnd, LPCSTR path, LPSECURITY_ATTRIBUTES sec)
{
    LPWSTR wPath;
    DWORD retCode;

    TRACE("(%s, %p)\n", debugstr_a(path), sec);

    retCode = SHELL32_AnsiToUnicodeBuf(path, &wPath, 0);
    if (!retCode)
    {
        retCode = SHCreateDirectoryExW(hWnd, wPath, sec);
        SHELL32_FreeUnicodeBuf(wPath);
    }
    return retCode;
}

/*************************************************************************
 * SHCreateDirectoryExW      [SHELL32.@]
 *
 * See SHCreateDirectoryExA.
 */
int WINAPI SHCreateDirectoryExW(HWND hWnd, LPCWSTR path, LPSECURITY_ATTRIBUTES sec)
{
    int ret = ERROR_BAD_PATHNAME;
    TRACE("(%p, %s, %p)\n", hWnd, debugstr_w(path), sec);

    if (PathIsRelativeW(path))
    {
      SetLastError(ret);
    }
    else
    {
        ret = SHNotifyCreateDirectoryW(path, sec);
        /* Refuse to work on certain error codes before trying to create directories recursively */
        if (ret != ERROR_SUCCESS &&
          ret != ERROR_FILE_EXISTS &&
          ret != ERROR_ALREADY_EXISTS &&
          ret != ERROR_FILENAME_EXCED_RANGE)
        {
            WCHAR *pEnd, *pSlash, szTemp[MAX_PATH + 1];  /* extra for PathAddBackslash() */

            lstrcpynW(szTemp, path, MAX_PATH);
            pEnd = PathAddBackslashW(szTemp);
            pSlash = szTemp + 3;

            while (*pSlash)
            {
                while (*pSlash && *pSlash != '\\') pSlash++;
                if (*pSlash)
                {
                    *pSlash = 0;    /* terminate path at separator */

                    ret = SHNotifyCreateDirectoryW(szTemp, pSlash + 1 == pEnd ? sec : NULL);
                }
                *pSlash++ = '\\'; /* put the separator back */
            }
        }

        if (ret && hWnd && (ERROR_CANCELLED != ret && ERROR_ALREADY_EXISTS != ret))
        {
            ShellMessageBoxW(shell32_hInstance, hWnd, MAKEINTRESOURCEW(IDS_CREATEFOLDER_DENIED), MAKEINTRESOURCEW(IDS_CREATEFOLDER_CAPTION),
                                    MB_ICONEXCLAMATION | MB_OK, path);
            ret = ERROR_CANCELLED;
        }
    }

    return ret;
}

/*************************************************************************
 * SHFindAttrW      [internal]
 *
 * Get the Attributes for a file or directory. The difference to GetAttributes()
 * is that this function will also work for paths containing wildcard characters
 * in its filename.

 * PARAMS
 *  path       [I]   path of directory or file to check
 *  fileOnly   [I]   TRUE if only files should be found
 *
 * RETURNS
 *  INVALID_FILE_ATTRIBUTES if the path does not exist, the actual attributes of
 *  the first file or directory found otherwise
 */
static DWORD SHFindAttrW(LPCWSTR pName, BOOL fileOnly)
{
    WIN32_FIND_DATAW wfd;
    BOOL b_FileMask = fileOnly && (NULL != StrPBrkW(pName, wWildcardChars));
    DWORD dwAttr = INVALID_FILE_ATTRIBUTES;
    HANDLE hFind = FindFirstFileW(pName, &wfd);

    TRACE("%s %d\n", debugstr_w(pName), fileOnly);
    if (INVALID_HANDLE_VALUE != hFind)
    {
        do
        {
            if (b_FileMask && IsAttribDir(wfd.dwFileAttributes))
               continue;
            dwAttr = wfd.dwFileAttributes;
            break;
        } while (FindNextFileW(hFind, &wfd));

      FindClose(hFind);
    }
    return dwAttr;
}

/*************************************************************************
 *
 * _ConvertAtoW  helper function for SHFileOperationA
 *
 * Converts a string or string-list to unicode.
 */
static DWORD _ConvertAtoW(PCSTR strSrc, PCWSTR* pStrDest, BOOL isList)
{
    *pStrDest = NULL;

    // If the input is null, nothing to convert.
    if (!strSrc)
        return 0;

    // Measure the total size, depending on if it's a zero-terminated list.
    int sizeA = 0;
    if (isList)
    {
        PCSTR tmpSrc = strSrc;
        int size;
        do
        {
            size = lstrlenA(tmpSrc) + 1;
            sizeA += size;
            tmpSrc += size;
        } while (size != 1);
    }
    else
    {
        sizeA = lstrlenA(strSrc) + 1;
    }

    // Measure the needed allocation size.
    int sizeW = MultiByteToWideChar(CP_ACP, 0, strSrc, sizeA, NULL, 0);
    if (!sizeW)
        return GetLastError();

    PWSTR strDest = (PWSTR) HeapAlloc(GetProcessHeap(), 0, sizeW * sizeof(WCHAR));
    if (!strDest)
        return ERROR_OUTOFMEMORY;

    int err = MultiByteToWideChar(CP_ACP, 0, strSrc, sizeA, strDest, sizeW);
    if (!err)
    {
        HeapFree(GetProcessHeap(), 0, strDest);
        return GetLastError();
    }

    *pStrDest = strDest;
    return 0;
}

/*************************************************************************
 * SHFileOperationA          [SHELL32.@]
 *
 * Function to copy, move, delete and create one or more files with optional
 * user prompts.
 *
 * PARAMS
 *  lpFileOp   [I/O] pointer to a structure containing all the necessary information
 *
 * RETURNS
 *  Success: ERROR_SUCCESS.
 *  Failure: ERROR_CANCELLED.
 *
 * NOTES
 *  exported by name
 */
int WINAPI SHFileOperationA(LPSHFILEOPSTRUCTA lpFileOp)
{
    int errCode, retCode;
    SHFILEOPSTRUCTW nFileOp = { 0 };

    // Convert A information to W
    nFileOp.hwnd = lpFileOp->hwnd;
    nFileOp.wFunc = lpFileOp->wFunc;
    nFileOp.fFlags = lpFileOp->fFlags;

    errCode = _ConvertAtoW(lpFileOp->pFrom, &nFileOp.pFrom, TRUE);
    if (errCode != 0)
        goto cleanup;

    if (FO_DELETE != (nFileOp.wFunc & FO_MASK))
    {
        errCode = _ConvertAtoW(lpFileOp->pTo, &nFileOp.pTo, TRUE);
        if (errCode != 0)
            goto cleanup;
    }

    if (nFileOp.fFlags & FOF_SIMPLEPROGRESS)
    {
        errCode = _ConvertAtoW(lpFileOp->lpszProgressTitle, &nFileOp.lpszProgressTitle, FALSE);
        if (errCode != 0)
            goto cleanup;
    }

    // Call the actual function
    retCode = SHFileOperationW(&nFileOp);
    if (retCode)
    {
        ERR("SHFileOperationW failed with 0x%x\n", retCode);
    }

    // Cleanup
cleanup:
    if (nFileOp.pFrom)
        HeapFree(GetProcessHeap(), 0, (PVOID) nFileOp.pFrom);
    if (nFileOp.pTo)
        HeapFree(GetProcessHeap(), 0, (PVOID) nFileOp.pTo);
    if (nFileOp.lpszProgressTitle)
        HeapFree(GetProcessHeap(), 0, (PVOID) nFileOp.lpszProgressTitle);

    if (errCode != 0)
    {
        lpFileOp->fAnyOperationsAborted = TRUE;
        SetLastError(errCode);

        return errCode;
    }

    // Thankfully, starting with NT4 the name mappings are always unicode, so no need to convert.
    lpFileOp->hNameMappings = nFileOp.hNameMappings;
    lpFileOp->fAnyOperationsAborted = nFileOp.fAnyOperationsAborted;
    return retCode;
}

static void __inline grow_list(FILE_LIST *list)
{
    FILE_ENTRY *newx = (FILE_ENTRY *)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, list->feFiles,
                                  list->num_alloc * 2 * sizeof(*newx) );
    list->feFiles = newx;
    list->num_alloc *= 2;
}

/* adds a file to the FILE_ENTRY struct
 */
static void add_file_to_entry(FILE_ENTRY *feFile, LPCWSTR szFile)
{
    DWORD dwLen = lstrlenW(szFile) + 1;
    LPCWSTR ptr;
    LPCWSTR ptr2;

    feFile->szFullPath = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, dwLen * sizeof(WCHAR));
    lstrcpyW(feFile->szFullPath, szFile);

    ptr = StrRChrW(szFile, NULL, '\\');
    ptr2 = StrRChrW(szFile, NULL, '/');
    if (!ptr || ptr < ptr2)
        ptr = ptr2;
    if (ptr)
    {
        dwLen = ptr - szFile + 1;
        feFile->szDirectory = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, dwLen * sizeof(WCHAR));
        lstrcpynW(feFile->szDirectory, szFile, dwLen);

        dwLen = lstrlenW(feFile->szFullPath) - dwLen + 1;
        feFile->szFilename = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, dwLen * sizeof(WCHAR));
        lstrcpyW(feFile->szFilename, ptr + 1); /* skip over backslash */
    }
    feFile->bFromWildcard = FALSE;
}

static LPWSTR wildcard_to_file(LPCWSTR szWildCard, LPCWSTR szFileName)
{
    LPCWSTR ptr;
    LPCWSTR ptr2;
    LPWSTR szFullPath;
    DWORD dwDirLen, dwFullLen;

    ptr = StrRChrW(szWildCard, NULL, '\\');
    ptr2 = StrRChrW(szWildCard, NULL, '/');
    if (!ptr || ptr < ptr2)
        ptr = ptr2;
    dwDirLen = ptr - szWildCard + 1;

    dwFullLen = dwDirLen + lstrlenW(szFileName) + 1;
    szFullPath = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, dwFullLen * sizeof(WCHAR));

    lstrcpynW(szFullPath, szWildCard, dwDirLen + 1);
    lstrcatW(szFullPath, szFileName);

    return szFullPath;
}

static void parse_wildcard_files(FILE_LIST *flList, LPCWSTR szFile, LPDWORD pdwListIndex)
{
    WIN32_FIND_DATAW wfd;
    HANDLE hFile = FindFirstFileW(szFile, &wfd);
    FILE_ENTRY *file;
    LPWSTR szFullPath;
    BOOL res;

    if (hFile == INVALID_HANDLE_VALUE) return;

    for (res = TRUE; res; res = FindNextFileW(hFile, &wfd))
    {
        if (IsDotDir(wfd.cFileName))
            continue;

        if (*pdwListIndex >= flList->num_alloc)
            grow_list( flList );

        szFullPath = wildcard_to_file(szFile, wfd.cFileName);
        file = &flList->feFiles[(*pdwListIndex)++];
        add_file_to_entry(file, szFullPath);
        file->bFromWildcard = TRUE;
        file->attributes = wfd.dwFileAttributes;

        if (IsAttribDir(file->attributes))
            flList->bAnyDirectories = TRUE;

        HeapFree(GetProcessHeap(), 0, szFullPath);
    }

    FindClose(hFile);
}

/* takes the null-separated file list and fills out the FILE_LIST */
static HRESULT parse_file_list(FILE_LIST *flList, LPCWSTR szFiles)
{
    LPCWSTR ptr = szFiles;
    WCHAR szCurFile[MAX_PATH];
    DWORD i = 0;

    if (!szFiles)
        return ERROR_INVALID_PARAMETER;

    flList->bAnyFromWildcard = FALSE;
    flList->bAnyDirectories = FALSE;
    flList->bAnyDontExist = FALSE;
    flList->num_alloc = 32;
    flList->dwNumFiles = 0;

    /* empty list */
    if (!szFiles[0])
        return ERROR_ACCESS_DENIED;

    flList->feFiles = (FILE_ENTRY *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                flList->num_alloc * sizeof(FILE_ENTRY));

    while (*ptr)
    {
        if (i >= flList->num_alloc) grow_list( flList );

        /* change relative to absolute path */
        if (PathIsRelativeW(ptr))
        {
            GetCurrentDirectoryW(MAX_PATH, szCurFile);
            PathCombineW(szCurFile, szCurFile, ptr);
            flList->feFiles[i].bFromRelative = TRUE;
        }
        else
        {
            lstrcpyW(szCurFile, ptr);
            flList->feFiles[i].bFromRelative = FALSE;
        }

        /* parse wildcard files if they are in the filename */
        if (StrPBrkW(szCurFile, wWildcardChars))
        {
            parse_wildcard_files(flList, szCurFile, &i);
            flList->bAnyFromWildcard = TRUE;
            i--;
        }
        else
        {
            FILE_ENTRY *file = &flList->feFiles[i];
            add_file_to_entry(file, szCurFile);
            file->attributes = GetFileAttributesW( file->szFullPath );
            file->bExists = (file->attributes != INVALID_FILE_ATTRIBUTES);

            if (!file->bExists)
                flList->bAnyDontExist = TRUE;

            if (IsAttribDir(file->attributes))
                flList->bAnyDirectories = TRUE;
        }

        /* advance to the next string */
        ptr += lstrlenW(ptr) + 1;
        i++;
    }
    flList->dwNumFiles = i;

    return S_OK;
}

/* free the FILE_LIST */
static void destroy_file_list(FILE_LIST *flList)
{
    DWORD i;

    if (!flList || !flList->feFiles)
        return;

    for (i = 0; i < flList->dwNumFiles; i++)
    {
        HeapFree(GetProcessHeap(), 0, flList->feFiles[i].szDirectory);
        HeapFree(GetProcessHeap(), 0, flList->feFiles[i].szFilename);
        HeapFree(GetProcessHeap(), 0, flList->feFiles[i].szFullPath);
    }

    HeapFree(GetProcessHeap(), 0, flList->feFiles);
}

static CStringW try_find_new_name(LPCWSTR szDestPath)
{
    CStringW mask(szDestPath);
    CStringW ext(PathFindExtensionW(szDestPath));

    // cut off extension before inserting a "new file" mask
    if (!ext.IsEmpty())
    {
        mask = mask.Left(mask.GetLength() - ext.GetLength());
    }
    mask += L" (%d)" + ext;

    CStringW newName;

    // trying to find new file name
    for (int i = 1; i < NEW_FILENAME_ON_COPY_TRIES; i++)
    {
        newName.Format(mask, i);

        if (!PathFileExistsW(newName))
        {
            return newName;
        }
    }

    return CStringW();
}

static void copy_dir_to_dir(FILE_OPERATION *op, const FILE_ENTRY *feFrom, LPCWSTR szDestPath)
{
    WCHAR szFrom[MAX_PATH], szTo[MAX_PATH];
    FILE_LIST flFromNew, flToNew;

    static const WCHAR wildCardFiles[] = {'*','.','*',0};

    if (IsDotDir(feFrom->szFilename))
        return;

    if (PathFileExistsW(szDestPath))
        PathCombineW(szTo, szDestPath, feFrom->szFilename);
    else
        lstrcpyW(szTo, szDestPath);

#ifdef __REACTOS__
    if (PathFileExistsW(szTo))
    {
        if (op->req->fFlags & FOF_RENAMEONCOLLISION)
        {
            CStringW newPath = try_find_new_name(szTo);
            if (!newPath.IsEmpty())
            {
                StringCchCopyW(szTo, _countof(szTo), newPath);
            }
        }
        else if (!(op->req->fFlags & FOF_NOCONFIRMATION))
#else
    if (!(op->req->fFlags & FOF_NOCONFIRMATION) && PathFileExistsW(szTo))
    {
        CStringW newPath;
        if (lstrcmp(feFrom->szDirectory, szDestPath) == 0 && !(newPath = try_find_new_name(szTo)).IsEmpty())
        {
            StringCchCopyW(szTo, _countof(szTo), newPath);
        }
        else
#endif
        {
            if (!SHELL_ConfirmDialogW(op->req->hwnd, ASK_OVERWRITE_FOLDER, feFrom->szFilename, op))
            {
                /* Vista returns an ERROR_CANCELLED even if user pressed "No" */
                if (!op->bManyItems)
                    op->bCancelled = TRUE;
                return;
            }
        }
    }

    szTo[lstrlenW(szTo) + 1] = '\0';
    SHNotifyCreateDirectoryW(szTo, NULL);

    PathCombineW(szFrom, feFrom->szFullPath, wildCardFiles);
    szFrom[lstrlenW(szFrom) + 1] = '\0';

    ZeroMemory(&flFromNew, sizeof(FILE_LIST));
    ZeroMemory(&flToNew, sizeof(FILE_LIST));
    parse_file_list(&flFromNew, szFrom);
    parse_file_list(&flToNew, szTo);

    copy_files(op, FALSE, &flFromNew, &flToNew);

    destroy_file_list(&flFromNew);
    destroy_file_list(&flToNew);
}

static BOOL copy_file_to_file(FILE_OPERATION *op, const WCHAR *szFrom, const WCHAR *szTo)
{
#ifdef __REACTOS__
    if (PathFileExistsW(szTo))
    {
        if (op->req->fFlags & FOF_RENAMEONCOLLISION)
        {
            CStringW newPath = try_find_new_name(szTo);
            if (!newPath.IsEmpty())
            {
                return SHNotifyCopyFileW(op, szFrom, newPath, FALSE) == 0;
            }
        }
        else if (!(op->req->fFlags & FOF_NOCONFIRMATION))
        {
            if (!SHELL_ConfirmDialogW(op->req->hwnd, ASK_OVERWRITE_FILE, PathFindFileNameW(szTo), op))
                return FALSE;
        }
#else
    if (!(op->req->fFlags & FOF_NOCONFIRMATION) && PathFileExistsW(szTo))
    {
        CStringW newPath;
        if (lstrcmp(szFrom, szTo) == 0 && !(newPath = try_find_new_name(szTo)).IsEmpty())
        {
            return SHNotifyCopyFileW(op, szFrom, newPath, FALSE) == 0;
        }

        if (!SHELL_ConfirmDialogW(op->req->hwnd, ASK_OVERWRITE_FILE, PathFindFileNameW(szTo), op))
            return FALSE;
#endif
    }

    return SHNotifyCopyFileW(op, szFrom, szTo, FALSE) == 0;
}

/* copy a file or directory to another directory */
static void copy_to_dir(FILE_OPERATION *op, const FILE_ENTRY *feFrom, const FILE_ENTRY *feTo)
{
    if (!PathFileExistsW(feTo->szFullPath))
        SHNotifyCreateDirectoryW(feTo->szFullPath, NULL);

    if (IsAttribFile(feFrom->attributes))
    {
        WCHAR szDestPath[MAX_PATH];

        PathCombineW(szDestPath, feTo->szFullPath, feFrom->szFilename);
        copy_file_to_file(op, feFrom->szFullPath, szDestPath);
    }
    else if (!(op->req->fFlags & FOF_FILESONLY && feFrom->bFromWildcard))
        copy_dir_to_dir(op, feFrom, feTo->szFullPath);
}

static void create_dest_dirs(LPCWSTR szDestDir)
{
    WCHAR dir[MAX_PATH];
    LPCWSTR ptr = StrChrW(szDestDir, '\\');

    /* make sure all directories up to last one are created */
    while (ptr && (ptr = StrChrW(ptr + 1, '\\')))
    {
        lstrcpynW(dir, szDestDir, ptr - szDestDir + 1);

        if (!PathFileExistsW(dir))
            SHNotifyCreateDirectoryW(dir, NULL);
    }

    /* create last directory */
    if (!PathFileExistsW(szDestDir))
        SHNotifyCreateDirectoryW(szDestDir, NULL);
}

/* the FO_COPY operation */
static HRESULT copy_files(FILE_OPERATION *op, BOOL multiDest, const FILE_LIST *flFrom, FILE_LIST *flTo)
{
    DWORD i;
    const FILE_ENTRY *entryToCopy;
    const FILE_ENTRY *fileDest = &flTo->feFiles[0];

    if (flFrom->bAnyDontExist)
        return ERROR_SHELL_INTERNAL_FILE_NOT_FOUND;

    if (flTo->dwNumFiles == 0)
    {
        /* If the destination is empty, SHFileOperation should use the current directory */
        WCHAR curdir[MAX_PATH+1];

        GetCurrentDirectoryW(MAX_PATH, curdir);
        curdir[lstrlenW(curdir)+1] = 0;

        destroy_file_list(flTo);
        ZeroMemory(flTo, sizeof(FILE_LIST));
        parse_file_list(flTo, curdir);
        fileDest = &flTo->feFiles[0];
    }

    if (multiDest)
    {
        if (flFrom->bAnyFromWildcard)
            return ERROR_CANCELLED;

        if (flFrom->dwNumFiles != flTo->dwNumFiles)
        {
            if (flFrom->dwNumFiles != 1 && !IsAttribDir(fileDest->attributes))
                return ERROR_CANCELLED;

            /* Free all but the first entry. */
            for (i = 1; i < flTo->dwNumFiles; i++)
            {
                HeapFree(GetProcessHeap(), 0, flTo->feFiles[i].szDirectory);
                HeapFree(GetProcessHeap(), 0, flTo->feFiles[i].szFilename);
                HeapFree(GetProcessHeap(), 0, flTo->feFiles[i].szFullPath);
            }

            flTo->dwNumFiles = 1;
        }
        else if (IsAttribDir(fileDest->attributes))
        {
            for (i = 1; i < flTo->dwNumFiles; i++)
                if (!IsAttribDir(flTo->feFiles[i].attributes) ||
                    !IsAttribDir(flFrom->feFiles[i].attributes))
                {
                    return ERROR_CANCELLED;
                }
        }
    }
    else if (flFrom->dwNumFiles != 1)
    {
        if (flTo->dwNumFiles != 1 && !IsAttribDir(fileDest->attributes))
            return ERROR_CANCELLED;

        if (PathFileExistsW(fileDest->szFullPath) &&
            IsAttribFile(fileDest->attributes))
        {
            return ERROR_CANCELLED;
        }

        if (flTo->dwNumFiles == 1 && fileDest->bFromRelative &&
            !PathFileExistsW(fileDest->szFullPath))
        {
            return ERROR_CANCELLED;
        }
    }

    for (i = 0; i < flFrom->dwNumFiles; i++)
    {
        entryToCopy = &flFrom->feFiles[i];

        if ((multiDest) &&
            flTo->dwNumFiles > 1)
        {
            fileDest = &flTo->feFiles[i];
        }

        if (IsAttribDir(entryToCopy->attributes) &&
            !lstrcmpiW(entryToCopy->szFullPath, fileDest->szDirectory))
        {
            return ERROR_SUCCESS;
        }

        create_dest_dirs(fileDest->szDirectory);

        if (!lstrcmpiW(entryToCopy->szFullPath, fileDest->szFullPath))
        {
            if (IsAttribFile(entryToCopy->attributes))
                return ERROR_NO_MORE_SEARCH_HANDLES;
            else
                return ERROR_SUCCESS;
        }

        if ((flFrom->dwNumFiles > 1 && flTo->dwNumFiles == 1) ||
            IsAttribDir(fileDest->attributes))
        {
            copy_to_dir(op, entryToCopy, fileDest);
        }
        else if (IsAttribDir(entryToCopy->attributes))
        {
            copy_dir_to_dir(op, entryToCopy, fileDest->szFullPath);
        }
        else
        {
            if (!copy_file_to_file(op, entryToCopy->szFullPath, fileDest->szFullPath))
            {
                op->req->fAnyOperationsAborted = TRUE;
                return ERROR_CANCELLED;
            }
        }

        if (op->progress != NULL)
            op->bCancelled |= op->progress->HasUserCancelled();
        /* Vista return code. XP would return e.g. ERROR_FILE_NOT_FOUND, ERROR_ALREADY_EXISTS */
        if (op->bCancelled)
            return ERROR_CANCELLED;
    }

    /* Vista return code. On XP if the used pressed "No" for the last item,
     * ERROR_ARENA_TRASHED would be returned */
    return ERROR_SUCCESS;
}

static BOOL confirm_delete_list(HWND hWnd, DWORD fFlags, BOOL fTrash, const FILE_LIST *flFrom)
{
    if (flFrom->dwNumFiles > 1)
    {
        WCHAR tmp[8];
        const WCHAR format[] = {'%','d',0};

        wnsprintfW(tmp, sizeof(tmp)/sizeof(tmp[0]), format, flFrom->dwNumFiles);
        return SHELL_ConfirmDialogW(hWnd, (fTrash?ASK_TRASH_MULTIPLE_ITEM:ASK_DELETE_MULTIPLE_ITEM), tmp, NULL);
    }
    else
    {
        const FILE_ENTRY *fileEntry = &flFrom->feFiles[0];

        if (IsAttribFile(fileEntry->attributes))
            return SHELL_ConfirmDialogW(hWnd, (fTrash?ASK_TRASH_FILE:ASK_DELETE_FILE), fileEntry->szFullPath, NULL);
        else if (!(fFlags & FOF_FILESONLY && fileEntry->bFromWildcard))
            return SHELL_ConfirmDialogW(hWnd, (fTrash?ASK_TRASH_FOLDER:ASK_DELETE_FOLDER), fileEntry->szFullPath, NULL);
    }
    return TRUE;
}

/* the FO_DELETE operation */
static HRESULT delete_files(FILE_OPERATION *op, const FILE_LIST *flFrom)
{
    const FILE_ENTRY *fileEntry;
    DWORD i;
    BOOL bPathExists;
    BOOL bTrash;

    if (!flFrom->dwNumFiles)
        return ERROR_SUCCESS;

    /* Windows also checks only the first item */
    bTrash = (op->req->fFlags & FOF_ALLOWUNDO)
        && TRASH_CanTrashFile(flFrom->feFiles[0].szFullPath);

    if (!(op->req->fFlags & FOF_NOCONFIRMATION) || (!bTrash && op->req->fFlags & FOF_WANTNUKEWARNING))
        if (!confirm_delete_list(op->req->hwnd, op->req->fFlags, bTrash, flFrom))
        {
            op->req->fAnyOperationsAborted = TRUE;
            return 0;
        }

    /* Check files. Do not delete one if one file does not exists */
    for (i = 0; i < flFrom->dwNumFiles; i++)
    {
        fileEntry = &flFrom->feFiles[i];

        if (fileEntry->attributes == (ULONG)-1)
        {
            // This is a windows 2003 server specific value which has been removed.
            // Later versions of windows return ERROR_FILE_NOT_FOUND.
            return ERROR_SHELL_INTERNAL_FILE_NOT_FOUND;
        }
    }

    for (i = 0; i < flFrom->dwNumFiles; i++)
    {
        bPathExists = TRUE;
        fileEntry = &flFrom->feFiles[i];

        if (!IsAttribFile(fileEntry->attributes) &&
            (op->req->fFlags & FOF_FILESONLY && fileEntry->bFromWildcard))
            continue;

        if (bTrash)
        {
            BOOL bDelete;
            if (TRASH_TrashFile(fileEntry->szFullPath))
            {
                SHChangeNotify(SHCNE_DELETE, SHCNF_PATHW, fileEntry->szFullPath, NULL);
                continue;
            }

            /* Note: Windows silently deletes the file in such a situation, we show a dialog */
            if (!(op->req->fFlags & FOF_NOCONFIRMATION) || (op->req->fFlags & FOF_WANTNUKEWARNING))
                bDelete = SHELL_ConfirmDialogW(op->req->hwnd, ASK_CANT_TRASH_ITEM, fileEntry->szFullPath, NULL);
            else
                bDelete = TRUE;

            if (!bDelete)
            {
                op->req->fAnyOperationsAborted = TRUE;
                break;
            }
        }

        /* delete the file or directory */
        if (IsAttribFile(fileEntry->attributes))
        {
            bPathExists = (ERROR_SUCCESS == SHNotifyDeleteFileW(op, fileEntry->szFullPath));
        }
        else
            bPathExists = SHELL_DeleteDirectoryW(op, fileEntry->szFullPath, FALSE);

        if (!bPathExists)
        {
            DWORD err = GetLastError();

            if (ERROR_FILE_NOT_FOUND == err)
            {
                // This is a windows 2003 server specific value which ahs been removed.
                // Later versions of windows return ERROR_FILE_NOT_FOUND.
                return ERROR_SHELL_INTERNAL_FILE_NOT_FOUND;
            }
            else
            {
                return err;
            }
        }

        if (op->progress != NULL)
            op->bCancelled |= op->progress->HasUserCancelled();
        /* Should fire on progress dialog only */
        if (op->bCancelled)
            return ERROR_CANCELLED;
    }

    return ERROR_SUCCESS;
}

static void move_dir_to_dir(FILE_OPERATION *op, const FILE_ENTRY *feFrom, LPCWSTR szDestPath)
{
    WCHAR szFrom[MAX_PATH], szTo[MAX_PATH];
    FILE_LIST flFromNew, flToNew;

    static const WCHAR wildCardFiles[] = {'*','.','*',0};

    if (IsDotDir(feFrom->szFilename))
        return;

    SHNotifyCreateDirectoryW(szDestPath, NULL);

    PathCombineW(szFrom, feFrom->szFullPath, wildCardFiles);
    szFrom[lstrlenW(szFrom) + 1] = '\0';

    lstrcpyW(szTo, szDestPath);
    szTo[lstrlenW(szDestPath) + 1] = '\0';

    ZeroMemory(&flFromNew, sizeof(FILE_LIST));
    ZeroMemory(&flToNew, sizeof(FILE_LIST));
    parse_file_list(&flFromNew, szFrom);
    parse_file_list(&flToNew, szTo);

    move_files(op, FALSE, &flFromNew, &flToNew);

    destroy_file_list(&flFromNew);
    destroy_file_list(&flToNew);

    if (PathIsDirectoryEmptyW(feFrom->szFullPath))
        Win32RemoveDirectoryW(feFrom->szFullPath);
}

/* moves a file or directory to another directory */
static void move_to_dir(FILE_OPERATION *op, const FILE_ENTRY *feFrom, const FILE_ENTRY *feTo)
{
    WCHAR szDestPath[MAX_PATH];

    PathCombineW(szDestPath, feTo->szFullPath, feFrom->szFilename);

    if (IsAttribFile(feFrom->attributes))
        SHNotifyMoveFileW(op, feFrom->szFullPath, szDestPath, FALSE);
    else if (!(op->req->fFlags & FOF_FILESONLY && feFrom->bFromWildcard))
        move_dir_to_dir(op, feFrom, szDestPath);
}

/* the FO_MOVE operation */
static DWORD move_files(FILE_OPERATION *op, BOOL multiDest, const FILE_LIST *flFrom, const FILE_LIST *flTo)
{
    DWORD i;
    INT mismatched = 0;

    const FILE_ENTRY *entryToMove;
    const FILE_ENTRY *fileDest;

    if (!flFrom->dwNumFiles)
        return ERROR_SUCCESS;

    if (!flTo->dwNumFiles)
        return ERROR_FILE_NOT_FOUND;

    if (!(multiDest) &&
        flTo->dwNumFiles > 1 && flFrom->dwNumFiles > 1)
    {
        return ERROR_CANCELLED;
    }

    if (!(multiDest) &&
        !flFrom->bAnyDirectories &&
        flFrom->dwNumFiles > flTo->dwNumFiles &&
        !(flTo->bAnyDirectories && flTo->dwNumFiles == 1))
    {
        return ERROR_CANCELLED;
    }

    if (!PathFileExistsW(flTo->feFiles[0].szDirectory))
        return ERROR_CANCELLED;

    if (multiDest)
        mismatched = flFrom->dwNumFiles - flTo->dwNumFiles;

    fileDest = &flTo->feFiles[0];
    for (i = 0; i < flFrom->dwNumFiles; i++)
    {
        entryToMove = &flFrom->feFiles[i];

        if (!PathFileExistsW(fileDest->szDirectory))
            return ERROR_CANCELLED;

        if (multiDest)
        {
            if (i >= flTo->dwNumFiles)
                break;
            fileDest = &flTo->feFiles[i];
            if (mismatched && !fileDest->bExists)
            {
                create_dest_dirs(flTo->feFiles[i].szFullPath);
                flTo->feFiles[i].bExists = TRUE;
                flTo->feFiles[i].attributes = FILE_ATTRIBUTE_DIRECTORY;
            }
        }

        if (fileDest->bExists && IsAttribDir(fileDest->attributes))
            move_to_dir(op, entryToMove, fileDest);
        else
            SHNotifyMoveFileW(op, entryToMove->szFullPath, fileDest->szFullPath, IsAttribDir(entryToMove->attributes));

        if (op->progress != NULL)
            op->bCancelled |= op->progress->HasUserCancelled();
        /* Should fire on progress dialog only */
        if (op->bCancelled)
            return ERROR_CANCELLED;

    }

    if (mismatched > 0)
    {
        if (flFrom->bAnyDirectories)
            return DE_DESTSAMETREE;
        else
            return DE_SAMEFILE;
    }

    return ERROR_SUCCESS;
}

/* the FO_RENAME files */
static HRESULT rename_files(FILE_OPERATION *op,  const FILE_LIST *flFrom, const FILE_LIST *flTo)
{
    const FILE_ENTRY *feFrom;
    const FILE_ENTRY *feTo;

    if (flFrom->dwNumFiles != 1)
        return ERROR_GEN_FAILURE;

    if (flTo->dwNumFiles != 1)
        return ERROR_CANCELLED;

    feFrom = &flFrom->feFiles[0];
    feTo= &flTo->feFiles[0];

    /* fail if destination doesn't exist */
    if (!feFrom->bExists)
        return ERROR_SHELL_INTERNAL_FILE_NOT_FOUND;

    /* fail if destination already exists */
    if (feTo->bExists)
        return ERROR_ALREADY_EXISTS;

    return SHNotifyMoveFileW(op, feFrom->szFullPath, feTo->szFullPath, IsAttribDir(feFrom->attributes));
}

/* alert the user if an unsupported flag is used */
static void check_flags(FILEOP_FLAGS fFlags)
{
    WORD wUnsupportedFlags = FOF_NO_CONNECTED_ELEMENTS |
        FOF_NOCOPYSECURITYATTRIBS | FOF_NORECURSEREPARSE |
#ifdef __REACTOS__
        FOF_WANTMAPPINGHANDLE;
#else
        FOF_RENAMEONCOLLISION | FOF_WANTMAPPINGHANDLE;
#endif

    if (fFlags & wUnsupportedFlags)
        FIXME("Unsupported flags: %04x\n", fFlags);
}

#ifdef __REACTOS__

static DWORD
validate_operation(LPSHFILEOPSTRUCTW lpFileOp, FILE_LIST *flFrom, FILE_LIST *flTo)
{
    DWORD i, k, dwNumDest;
    WCHAR szFrom[MAX_PATH], szTo[MAX_PATH];
    CStringW strTitle, strText;
    const FILE_ENTRY *feFrom;
    const FILE_ENTRY *feTo;
    UINT wFunc = lpFileOp->wFunc;
    HWND hwnd = lpFileOp->hwnd;

    dwNumDest = flTo->dwNumFiles;

    if (wFunc != FO_COPY && wFunc != FO_MOVE)
        return ERROR_SUCCESS;

    for (k = 0; k < dwNumDest; ++k)
    {
        feTo = &flTo->feFiles[k];
        for (i = 0; i < flFrom->dwNumFiles; ++i)
        {
            feFrom = &flFrom->feFiles[i];
            StringCbCopyW(szFrom, sizeof(szFrom), feFrom->szFullPath);
            StringCbCopyW(szTo, sizeof(szTo), feTo->szFullPath);
            if (IsAttribDir(feTo->attributes))
            {
                PathAppendW(szTo, feFrom->szFilename);
            }

            // same path?
            if (lstrcmpiW(szFrom, szTo) == 0 &&
                (wFunc == FO_MOVE || !(lpFileOp->fFlags & FOF_RENAMEONCOLLISION)))
            {
                if (!(lpFileOp->fFlags & (FOF_NOERRORUI | FOF_SILENT)))
                {
                    if (wFunc == FO_MOVE)
                    {
                        strTitle.LoadStringW(IDS_MOVEERRORTITLE);
                        if (IsAttribDir(feFrom->attributes))
                            strText.Format(IDS_MOVEERRORSAMEFOLDER, feFrom->szFilename);
                        else
                            strText.Format(IDS_MOVEERRORSAME, feFrom->szFilename);
                    }
                    else
                    {
                        strTitle.LoadStringW(IDS_COPYERRORTITLE);
                        strText.Format(IDS_COPYERRORSAME, feFrom->szFilename);
                        return ERROR_SUCCESS;
                    }
                    MessageBoxW(hwnd, strText, strTitle, MB_ICONERROR);
                    return DE_SAMEFILE;
                }
                return DE_OPCANCELLED;
            }

            // subfolder?
            if (IsAttribDir(feFrom->attributes))
            {
                size_t cchFrom = PathAddBackslashW(szFrom) - szFrom;
                size_t cchTo = PathAddBackslashW(szTo) - szTo;
                if (cchFrom <= cchTo)
                {
                    WCHAR ch = szTo[cchFrom];
                    szTo[cchFrom] = 0;
                    int compare = lstrcmpiW(szFrom, szTo);
                    szTo[cchFrom] = ch;

                    if (compare == 0)
                    {
                        if (!(lpFileOp->fFlags & (FOF_NOERRORUI | FOF_SILENT)))
                        {
                            if (wFunc == FO_MOVE)
                            {
                                strTitle.LoadStringW(IDS_MOVEERRORTITLE);
                                strText.Format(IDS_MOVEERRORSUBFOLDER, feFrom->szFilename);
                            }
                            else
                            {
                                strTitle.LoadStringW(IDS_COPYERRORTITLE);
                                strText.Format(IDS_COPYERRORSUBFOLDER, feFrom->szFilename);
                            }
                            MessageBoxW(hwnd, strText, strTitle, MB_ICONERROR);
                            return DE_DESTSUBTREE;
                        }
                        return DE_OPCANCELLED;
                    }
                }
            }
        }
    }

    return ERROR_SUCCESS;
}
#endif
/*************************************************************************
 * SHFileOperationW          [SHELL32.@]
 *
 * See SHFileOperationA
 */
int WINAPI SHFileOperationW(LPSHFILEOPSTRUCTW lpFileOp)
{
    FILE_OPERATION op;
    FILE_LIST flFrom, flTo;
    int ret = 0;

    if (!lpFileOp)
        return ERROR_INVALID_PARAMETER;

    ret = CoInitialize(NULL);
    if (FAILED(ret))
        return ret;

    lpFileOp->fAnyOperationsAborted = FALSE;
    check_flags(lpFileOp->fFlags);

    ZeroMemory(&flFrom, sizeof(FILE_LIST));
    ZeroMemory(&flTo, sizeof(FILE_LIST));

    if ((ret = parse_file_list(&flFrom, lpFileOp->pFrom)))
        return ret;

    if (lpFileOp->wFunc != FO_DELETE)
        parse_file_list(&flTo, lpFileOp->pTo);

    ZeroMemory(&op, sizeof(op));
    op.req = lpFileOp;
    op.totalSize.QuadPart = 0ull;
    op.completedSize.QuadPart = 0ull;
    op.bManyItems = (flFrom.dwNumFiles > 1);

#ifdef __REACTOS__
    ret = validate_operation(lpFileOp, &flFrom, &flTo);
    if (ret)
        goto cleanup;
#endif
    if (lpFileOp->wFunc != FO_RENAME && !(lpFileOp->fFlags & FOF_SILENT)) {
        ret = CoCreateInstance(CLSID_ProgressDialog,
                               NULL,
                               CLSCTX_INPROC_SERVER,
                               IID_PPV_ARG(IProgressDialog, &op.progress));
        if (FAILED(ret))
            goto cleanup;

        op.progress->StartProgressDialog(op.req->hwnd, NULL, PROGDLG_NORMAL & PROGDLG_AUTOTIME, NULL);
        _SetOperationTitle(&op);
        _FileOpCountManager(&op, &flFrom);
    }

    switch (lpFileOp->wFunc)
    {
        case FO_COPY:
            ret = copy_files(&op, op.req->fFlags & FOF_MULTIDESTFILES, &flFrom, &flTo);
            break;
        case FO_DELETE:
            ret = delete_files(&op, &flFrom);
            break;
        case FO_MOVE:
            ret = move_files(&op, op.req->fFlags & FOF_MULTIDESTFILES, &flFrom, &flTo);
            break;
        case FO_RENAME:
            ret = rename_files(&op, &flFrom, &flTo);
            break;
        default:
            ret = ERROR_INVALID_PARAMETER;
            break;
    }

    if (op.progress) {
        op.progress->StopProgressDialog();
        op.progress->Release();
    }

cleanup:
    destroy_file_list(&flFrom);

    if (lpFileOp->wFunc != FO_DELETE)
        destroy_file_list(&flTo);

    if (ret == ERROR_CANCELLED)
        lpFileOp->fAnyOperationsAborted = TRUE;

    CoUninitialize();

    return ret;
}

// Used by SHFreeNameMappings
static int CALLBACK _DestroyCallback(void *p, void *pData)
{
    LPSHNAMEMAPPINGW lp = (SHNAMEMAPPINGW *)p;

    SHFree(lp->pszOldPath);
    SHFree(lp->pszNewPath);

    return TRUE;
}

/*************************************************************************
 * SHFreeNameMappings      [shell32.246]
 *
 * Free the mapping handle returned by SHFileOperation if FOF_WANTSMAPPINGHANDLE
 * was specified.
 *
 * PARAMS
 *  hNameMapping [I] handle to the name mappings used during renaming of files
 *
 * RETURNS
 *  Nothing
 */
void WINAPI SHFreeNameMappings(HANDLE hNameMapping)
{
    if (hNameMapping)
    {
        DSA_DestroyCallback((HDSA) hNameMapping, _DestroyCallback, NULL);
    }
}

/*************************************************************************
 * SheGetDirA [SHELL32.@]
 *
 * drive = 0: returns the current directory path
 * drive > 0: returns the current directory path of the specified drive
 *            drive=1 -> A:  drive=2 -> B:  ...
 * returns 0 if successful
*/
EXTERN_C DWORD WINAPI SheGetDirA(DWORD drive, LPSTR buffer)
{
    WCHAR org_path[MAX_PATH];
    DWORD ret;
    char drv_path[3];

    /* change current directory to the specified drive */
    if (drive) {
        strcpy(drv_path, "A:");
        drv_path[0] += (char)drive-1;

        GetCurrentDirectoryW(MAX_PATH, org_path);

        SetCurrentDirectoryA(drv_path);
    }

    /* query current directory path of the specified drive */
    ret = GetCurrentDirectoryA(MAX_PATH, buffer);

    /* back to the original drive */
    if (drive)
        SetCurrentDirectoryW(org_path);

    if (!ret)
        return GetLastError();

    return 0;
}

/*************************************************************************
 * SheGetDirW [SHELL32.@]
 *
 * drive = 0: returns the current directory path
 * drive > 0: returns the current directory path of the specified drive
 *            drive=1 -> A:  drive=2 -> B:  ...
 * returns 0 if successful
 */
EXTERN_C DWORD WINAPI SheGetDirW(DWORD drive, LPWSTR buffer)
{
    WCHAR org_path[MAX_PATH];
    DWORD ret;
    char drv_path[3];

    /* change current directory to the specified drive */
    if (drive)
    {
        strcpy(drv_path, "A:");
        drv_path[0] += (char)drive-1;

        GetCurrentDirectoryW(MAX_PATH, org_path);

        SetCurrentDirectoryA(drv_path);
    }

    /* query current directory path of the specified drive */
    ret = GetCurrentDirectoryW(MAX_PATH, buffer);

    /* back to the original drive */
    if (drive)
        SetCurrentDirectoryW(org_path);

    if (!ret)
        return GetLastError();

    return 0;
}

/*************************************************************************
 * SheChangeDirA [SHELL32.@]
 *
 * changes the current directory to the specified path
 * and returns 0 if successful
 */
EXTERN_C DWORD WINAPI SheChangeDirA(LPSTR path)
{
    if (SetCurrentDirectoryA(path))
        return 0;
    else
        return GetLastError();
}

/*************************************************************************
 * SheChangeDirW [SHELL32.@]
 *
 * changes the current directory to the specified path
 * and returns 0 if successful
 */
EXTERN_C DWORD WINAPI SheChangeDirW(LPWSTR path)
{
    if (SetCurrentDirectoryW(path))
        return 0;
    else
        return GetLastError();
}

/*************************************************************************
 * IsNetDrive            [SHELL32.66]
 */
EXTERN_C int WINAPI IsNetDrive(int drive)
{
    char root[4];
    strcpy(root, "A:\\");
    root[0] += (char)drive;
    return (GetDriveTypeA(root) == DRIVE_REMOTE);
}


/*************************************************************************
 * RealDriveType                [SHELL32.524]
 */
EXTERN_C INT WINAPI RealDriveType(INT drive, BOOL bQueryNet)
{
    char root[] = "A:\\";
    root[0] += (char)drive;
    return GetDriveTypeA(root);
}

/***********************************************************************
 *              SHPathPrepareForWriteW (SHELL32.@)
 */
EXTERN_C HRESULT WINAPI SHPathPrepareForWriteW(HWND hwnd, IUnknown *modless, LPCWSTR path, DWORD flags)
{
    DWORD res;
    DWORD err;
    LPCWSTR realpath;
    int len;
    WCHAR* last_slash;
    WCHAR* temppath=NULL;

    TRACE("%p %p %s 0x%08x\n", hwnd, modless, debugstr_w(path), flags);

    if (flags & ~(SHPPFW_DIRCREATE|SHPPFW_ASKDIRCREATE|SHPPFW_IGNOREFILENAME))
        FIXME("unimplemented flags 0x%08x\n", flags);

    /* cut off filename if necessary */
    if (flags & SHPPFW_IGNOREFILENAME)
    {
        last_slash = StrRChrW(path, NULL, '\\');
        if (last_slash == NULL)
            len = 1;
        else
            len = last_slash - path + 1;
        temppath = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!temppath)
            return E_OUTOFMEMORY;
        StrCpyNW(temppath, path, len);
        realpath = temppath;
    }
    else
    {
        realpath = path;
    }

    /* try to create the directory if asked to */
    if (flags & (SHPPFW_DIRCREATE|SHPPFW_ASKDIRCREATE))
    {
        if (flags & SHPPFW_ASKDIRCREATE)
            FIXME("treating SHPPFW_ASKDIRCREATE as SHPPFW_DIRCREATE\n");

        SHCreateDirectoryExW(0, realpath, NULL);
    }

    /* check if we can access the directory */
    res = GetFileAttributesW(realpath);

    HeapFree(GetProcessHeap(), 0, temppath);

    if (res == INVALID_FILE_ATTRIBUTES)
    {
        err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND)
            return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
        return HRESULT_FROM_WIN32(err);
    }
    else if (res & FILE_ATTRIBUTE_DIRECTORY)
        return S_OK;
    else
        return HRESULT_FROM_WIN32(ERROR_DIRECTORY);
}

/***********************************************************************
 *              SHPathPrepareForWriteA (SHELL32.@)
 */
EXTERN_C HRESULT WINAPI SHPathPrepareForWriteA(HWND hwnd, IUnknown *modless, LPCSTR path, DWORD flags)
{
    WCHAR wpath[MAX_PATH];
    MultiByteToWideChar( CP_ACP, 0, path, -1, wpath, MAX_PATH);
    return SHPathPrepareForWriteW(hwnd, modless, wpath, flags);
}


/*
 * The two following background operations were modified from filedefext.cpp
 * They use an inordinate amount of mutable state across the string functions,
 * so are not easy to follow and care is required when modifying.
 */

DWORD WINAPI
_FileOpCountManager(FILE_OPERATION *op, const FILE_LIST *from)
{
    DWORD ticks = GetTickCount();
    FILE_ENTRY *entryToCount;

    for (UINT i = 0; i < from->dwNumFiles; i++)
    {
        entryToCount = &from->feFiles[i];

        WCHAR theFileName[MAX_PATH];
        StringCchCopyW(theFileName, MAX_PATH, entryToCount->szFullPath);
        _FileOpCount(op, theFileName, IsAttribDir(entryToCount->attributes), &ticks);
    }
    return 0;
}

// All path manipulations, even when this function is nested, occur on the one buffer.
static BOOL
_FileOpCount(FILE_OPERATION *op, LPWSTR pwszBuf, BOOL bFolder, DWORD *ticks)
{
    /* Find filename position */
    UINT cchBuf = wcslen(pwszBuf);
    WCHAR *pwszFilename = pwszBuf + cchBuf;
    size_t cchFilenameMax = MAX_PATH - cchBuf;
    if (!cchFilenameMax)
        return FALSE;

    if (bFolder) {
        *(pwszFilename++) = '\\';
        --cchFilenameMax;
        /* Find all files, FIXME: shouldn't be "*"? */
        StringCchCopyW(pwszFilename, cchFilenameMax, L"*");
    }

    WIN32_FIND_DATAW wfd;
    HANDLE hFind = FindFirstFileW(pwszBuf, &wfd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        ERR("FindFirstFileW %ls failed\n", pwszBuf);
        return FALSE;
    }

    do
    {
        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            /* Don't process "." and ".." items */
            if (!wcscmp(wfd.cFileName, L".") || !wcscmp(wfd.cFileName, L".."))
                continue;

            StringCchCopyW(pwszFilename, cchFilenameMax, wfd.cFileName);
            _FileOpCount(op, pwszBuf, TRUE, ticks);
        }
        else
        {
            ULARGE_INTEGER FileSize;
            FileSize.u.LowPart  = wfd.nFileSizeLow;
            FileSize.u.HighPart = wfd.nFileSizeHigh;
            op->totalSize.QuadPart += FileSize.QuadPart;
        }
        if (GetTickCount() - *ticks > (DWORD) 500)
        {
            // Check if the dialog has ended. If it has, we'll spin down.
            if (op->progress != NULL)
                op->bCancelled = op->progress->HasUserCancelled();

            if (op->bCancelled)
                break;
            *ticks = GetTickCount();
        }
    } while(FindNextFileW(hFind, &wfd));

    FindClose(hFind);
    return TRUE;
}
