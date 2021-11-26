#include <precomp.h>

static INT
LengthOfStrResource(IN HINSTANCE hInst,
                    IN UINT uID)
{
    HRSRC hrSrc;
    HGLOBAL hRes;
    LPWSTR lpName, lpStr;

    if (hInst == NULL)
    {
        return -1;
    }

    /* There are always blocks of 16 strings */
    lpName = (LPWSTR)MAKEINTRESOURCE((uID >> 4) + 1);

    /* Find the string table block */
    if ((hrSrc = FindResourceW(hInst, lpName, (LPWSTR)RT_STRING)) &&
        (hRes = LoadResource(hInst, hrSrc)) &&
        (lpStr = LockResource(hRes)))
    {
        UINT x;

        /* Find the string we're looking for */
        uID &= 0xF; /* position in the block, same as % 16 */
        for (x = 0; x < uID; x++)
        {
            lpStr += (*lpStr) + 1;
        }

        /* Found the string */
        return (int)(*lpStr);
    }
    return -1;
}

INT
AllocAndLoadString(OUT LPTSTR *lpTarget,
                   IN HINSTANCE hInst,
                   IN UINT uID)
{
    INT ln;

    ln = LengthOfStrResource(hInst,
                             uID);
    if (ln++ > 0)
    {
        (*lpTarget) = (LPTSTR)LocalAlloc(LMEM_FIXED,
                                         ln * sizeof(TCHAR));
        if ((*lpTarget) != NULL)
        {
            INT Ret;
            if (!(Ret = LoadString(hInst, uID, *lpTarget, ln)))
            {
                LocalFree((HLOCAL)(*lpTarget));
            }
            return Ret;
        }
    }
    return 0;
}

DWORD
LoadAndFormatString(IN HINSTANCE hInstance,
                    IN UINT uID,
                    OUT LPTSTR *lpTarget,
                    ...)
{
    DWORD Ret = 0;
    LPTSTR lpFormat;
    va_list lArgs;

    if (AllocAndLoadString(&lpFormat,
                           hInstance,
                           uID) > 0)
    {
        va_start(lArgs, lpTarget);
        /* let's use FormatMessage to format it because it has the ability to allocate
           memory automatically */
        Ret = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                            lpFormat,
                            0,
                            0,
                            (LPTSTR)lpTarget,
                            0,
                            &lArgs);
        va_end(lArgs);

        LocalFree((HLOCAL)lpFormat);
    }

    return Ret;
}

BOOL
StatusBarLoadAndFormatString(IN HWND hStatusBar,
                             IN INT PartId,
                             IN HINSTANCE hInstance,
                             IN UINT uID,
                             ...)
{
    BOOL Ret = FALSE;
    LPTSTR lpFormat, lpStr;
    va_list lArgs;

    if (AllocAndLoadString(&lpFormat,
                           hInstance,
                           uID) > 0)
    {
        va_start(lArgs, uID);
        /* let's use FormatMessage to format it because it has the ability to allocate
           memory automatically */
        Ret = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                            lpFormat,
                            0,
                            0,
                            (LPTSTR)&lpStr,
                            0,
                            &lArgs);
        va_end(lArgs);

        if (lpStr != NULL)
        {
            Ret = (BOOL)SendMessage(hStatusBar,
                                    SB_SETTEXT,
                                    (WPARAM)PartId,
                                    (LPARAM)lpStr);
            LocalFree((HLOCAL)lpStr);
        }

        LocalFree((HLOCAL)lpFormat);
    }

    return Ret;
}

BOOL
StatusBarLoadString(IN HWND hStatusBar,
                    IN INT PartId,
                    IN HINSTANCE hInstance,
                    IN UINT uID)
{
    BOOL Ret = FALSE;
    LPTSTR lpStr;

    if (AllocAndLoadString(&lpStr,
                           hInstance,
                           uID) > 0)
    {
        Ret = (BOOL)SendMessage(hStatusBar,
                                SB_SETTEXT,
                                (WPARAM)PartId,
                                (LPARAM)lpStr);
        LocalFree((HLOCAL)lpStr);
    }

    return Ret;
}


INT
GetTextFromEdit(OUT LPTSTR lpString,
                IN HWND hDlg,
                IN UINT Res)
{
    INT len = GetWindowTextLength(GetDlgItem(hDlg, Res));
    if(len > 0)
    {
        GetDlgItemText(hDlg,
                       Res,
                       lpString,
                       len + 1);
    }
    else
        lpString = NULL;

    return len;
}


VOID GetError(DWORD err)
{
    LPVOID lpMsgBuf;

    if (err == 0)
        err = GetLastError();

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  err,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR) &lpMsgBuf,
                  0,
                  NULL );

    MessageBox(NULL, lpMsgBuf, _T("Error!"), MB_OK | MB_ICONERROR);

    LocalFree(lpMsgBuf);
}



/*
 * Toolbar custom control routines
 */

typedef struct _TBCUSTCTL
{
    HWND hWndControl;
    INT iCommand;
    BOOL HideVertical : 1;
    BOOL IsVertical : 1;
} TBCUSTCTL, *PTBCUSTCTL;

BOOL
ToolbarDeleteControlSpace(HWND hWndToolbar,
                          const TBBUTTON *ptbButton)
{
    if ((ptbButton->fsStyle & TBSTYLE_SEP) &&
        ptbButton->dwData != 0)
    {
        PTBCUSTCTL cctl = (PTBCUSTCTL)ptbButton->dwData;

        DestroyWindow(cctl->hWndControl);

        HeapFree(ProcessHeap,
                 0,
                 cctl);
        return TRUE;
    }

    return FALSE;
}

VOID
ToolbarUpdateControlSpaces(HWND hWndToolbar,
                           ToolbarChangeControlCallback ChangeCallback)
{
    BOOL Vert;
    DWORD nButtons, i;
    TBBUTTON tbtn;

    Vert = ((SendMessage(hWndToolbar,
                         TB_GETSTYLE,
                         0,
                         0) & CCS_VERT) != 0);

    nButtons = (DWORD)SendMessage(hWndToolbar,
                                  TB_BUTTONCOUNT,
                                  0,
                                  0);

    for (i = 0;
         i != nButtons;
         i++)
    {
        if (SendMessage(hWndToolbar,
                        TB_GETBUTTON,
                        (WPARAM)i,
                        (LPARAM)&tbtn))
        {
            if ((tbtn.fsStyle & TBSTYLE_SEP) && tbtn.dwData != 0)
            {
                PTBCUSTCTL cctl = (PTBCUSTCTL)tbtn.dwData;

                cctl->IsVertical = Vert;

                if (cctl->HideVertical)
                {
                    ShowWindow(cctl->hWndControl,
                               (Vert ? SW_HIDE : SW_SHOW));
                    goto ShowHideSep;
                }
                else if (cctl->IsVertical != Vert)
                {
                    ChangeCallback(hWndToolbar,
                                   cctl->hWndControl,
                                   Vert);

ShowHideSep:
                    /* show/hide the separator */
                    SendMessage(hWndToolbar,
                                TB_HIDEBUTTON,
                                (WPARAM)cctl->iCommand,
                                (LPARAM)Vert && cctl->HideVertical);
                }
            }
        }
    }
}

BOOL
ToolbarInsertSpaceForControl(HWND hWndToolbar,
                             HWND hWndControl,
                             INT Index,
                             INT iCmd,
                             BOOL HideVertical)
{
    PTBCUSTCTL cctl;
    RECT rcControl, rcItem;

    cctl = HeapAlloc(ProcessHeap,
                     0,
                     sizeof(TBCUSTCTL));
    if (cctl == NULL)
        return FALSE;

    cctl->HideVertical = HideVertical;
    cctl->hWndControl = hWndControl;
    cctl->iCommand = iCmd;

    if (GetWindowRect(hWndControl,
                      &rcControl))
    {
        TBBUTTON tbtn = {0};

        tbtn.iBitmap = rcControl.right - rcControl.left;
        tbtn.idCommand = iCmd;
        tbtn.fsStyle = TBSTYLE_SEP;
        tbtn.dwData = (DWORD_PTR)cctl;

        if (SendMessage(hWndToolbar,
                        TB_GETSTYLE,
                        0,
                        0) & CCS_VERT)
        {
            if (HideVertical)
                tbtn.fsState |= TBSTATE_HIDDEN;

            cctl->IsVertical = TRUE;
        }
        else
            cctl->IsVertical = FALSE;

        if (SendMessage(hWndToolbar,
                        TB_INSERTBUTTON,
                        (WPARAM)Index,
                        (LPARAM)&tbtn))
        {
            if (SendMessage(hWndToolbar,
                            TB_GETITEMRECT,
                            (WPARAM)Index,
                            (LPARAM)&rcItem))
            {
                SetWindowPos(hWndControl,
                             NULL,
                             rcItem.left,
                             rcItem.top,
                             rcItem.right - rcItem.left,
                             rcItem.bottom - rcItem.top,
                             SWP_NOZORDER);

                ShowWindow(hWndControl,
                           SW_SHOW);

                return TRUE;
            }
            else if (tbtn.fsState & TBSTATE_HIDDEN)
            {
                ShowWindow(hWndControl,
                           SW_HIDE);
            }
        }
    }

    return FALSE;
}


HIMAGELIST
InitImageList(UINT StartResource,
              UINT NumImages)
{
    UINT EndResource = StartResource + NumImages - 1;
    HBITMAP hBitmap;
    HIMAGELIST hImageList;
    UINT i;
    INT Ret = 0;

    /* Create the toolbar icon image list */
    hImageList = ImageList_Create(TB_BMP_WIDTH,
                                  TB_BMP_HEIGHT,
                                  ILC_MASK | ILC_COLOR24,
                                  NumImages,
                                  0);
    if (hImageList == NULL)
        return NULL;

    /* Add all icons to the image list */
    for (i = StartResource; i <= EndResource && Ret != -1; i++)
    {
        hBitmap = LoadImage(hInstance,
                            MAKEINTRESOURCE(i),
                            IMAGE_BITMAP,
                            TB_BMP_WIDTH,
                            TB_BMP_HEIGHT,
                            LR_LOADTRANSPARENT);
        if (hBitmap == NULL)
        {
            Ret = -1;
            break;
        }

        Ret = ImageList_AddMasked(hImageList,
                                  hBitmap,
                                  RGB(255, 255, 254));

        DeleteObject(hBitmap);
    }

    if (Ret == -1)
    {
        ImageList_Destroy(hImageList);
        hImageList = NULL;
    }

    return hImageList;

}

/*
static BOOL
DestroyImageList(HIMAGELIST hImageList)
{
    if (! ImageList_Destroy(hImageList))
        return FALSE;
    else
        return TRUE;
}
*/
