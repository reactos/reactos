#include <precomp.h>

static const TCHAR szFloatWndClass[] = TEXT("ImageSoftFloatWndClass");

#define ID_TIMER 1

TBBUTTON ToolsButtons[] = {
/*   iBitmap,            idCommand,      fsState,         fsStyle,     bReserved[2], dwData, iString */
    {TBICON_RECTSEL,     ID_RECTSEL,     TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* redo */
    {TBICON_MOVESEL,     ID_MOVESEL,     TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* redo */
    {TBICON_LASOO,       ID_LASOO,       TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* cut */
    {TBICON_MOVE,        ID_MOVE,        TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* undo */
    {TBICON_ECLIPSESEL,  ID_ECLIPSESEL,  TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* save */
    {TBICON_ZOOM,        ID_ZOOM,        TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* redo */
    {TBICON_MAGICWAND,   ID_MAGICWAND,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* paste */
    {TBICON_TEXT,        ID_TEXT,        TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* redo */
    {TBICON_PAINTBRUSH,  ID_PAINTBRUSH,  TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* redo */
    {TBICON_ERASER,      ID_ERASER,      TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* print */
    {TBICON_PENCIL,      ID_PENCIL,      TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* redo */
    {TBICON_COLORPICKER, ID_COLORPICKER, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* new */
    {TBICON_CLONESTAMP,  ID_CLONESTAMP,  TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* new */
    {TBICON_RECOLORING,  ID_RECOLORING,  TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* redo */
    {TBICON_PAINTBUCKET, ID_PAINTBUCKET, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* redo */
    {TBICON_LINE,        ID_LINE,        TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* copy */
    {TBICON_RECTANGLE,   ID_RECTANGLE,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* redo */
    {TBICON_ROUNDRECT,   ID_ROUNDRECT,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* redo */
    {TBICON_ECLIPSE,     ID_ECLIPSE,     TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* open */
    {TBICON_FREEFORM,    ID_FREEFORM,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},    /* print preview */

    {10, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},
};


BOOL
ShowHideWindow(HWND hwnd)
{
    static BOOL Hidden = FALSE;

    ShowWindow(hwnd, Hidden ? SW_SHOW : SW_HIDE);
    Hidden = ~Hidden;

    return Hidden;
}


VOID
FloatToolbarCreateToolsGui(PFLT_WND FltTools)
{
    HWND hTb;
    HIMAGELIST hImageList;
    INT NumButtons;

    NumButtons = sizeof(ToolsButtons) / sizeof(ToolsButtons[0]);

    hTb = CreateWindowEx(0,
                         TOOLBARCLASSNAME,
                         NULL,
                         WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_WRAPABLE | CCS_NODIVIDER,
                         0, 0, 32, 200,
                         FltTools->hSelf,
                         NULL,
                         hInstance,
                         NULL);

    SendMessage(hTb,
                TB_SETEXTENDEDSTYLE,
                0,
                TBSTYLE_EX_HIDECLIPPEDBUTTONS);

    SendMessage(hTb,
                TB_BUTTONSTRUCTSIZE,
                sizeof(ToolsButtons[0]),
                0);

    SendMessage(hTb,
                TB_SETBITMAPSIZE,
                0,
                (LPARAM)MAKELONG(16, 16));

    hImageList = InitImageList(20,
                               IDB_TOOLSRECTSEL);

    ImageList_Destroy((HIMAGELIST)SendMessage(hTb,
                                              TB_SETIMAGELIST,
                                              0,
                                              (LPARAM)hImageList));

    SendMessage(hTb,
                TB_ADDBUTTONS,
                NumButtons,
                (LPARAM)ToolsButtons);

    SendMessage(hTb,
                TB_AUTOSIZE,
                0,
                0);

    return;
}


VOID
FloatToolbarCreateColorsGui(PFLT_WND FltColors)
{

    return;
}


VOID
FloatToolbarCreateHistoryGui(PFLT_WND FltHistory)
{

    return;
}

LRESULT CALLBACK
FloatToolbarWndProc(HWND hwnd,
                    UINT Message,
                    WPARAM wParam,
                    LPARAM lParam)
{
    switch(Message)
    {
        static BOOL bOpaque = FALSE;

        case WM_CREATE:

            SetWindowLong(hwnd,
                          GWL_EXSTYLE,
                          GetWindowLong(hwnd,
                                        GWL_EXSTYLE) | WS_EX_LAYERED);

            /* set the tranclucency to 60% */
            SetLayeredWindowAttributes(hwnd,
                                       0,
                                       (255 * 60) / 100,
                                       LWA_ALPHA);

        break;

        case WM_TIMER:
        {
            POINT pt;

            if (bOpaque != TRUE)
            {
                KillTimer(hwnd,
                          ID_TIMER);
                break;
            }

            if (GetCursorPos(&pt))
            {
                RECT rect;

                if (GetWindowRect(hwnd,
                                  &rect))
                {
                    if (! PtInRect(&rect,
                                   pt))
                    {
                        KillTimer(hwnd,
                                  ID_TIMER);

                        bOpaque = FALSE;

                        SetWindowLong(hwnd,
                                      GWL_EXSTYLE,
                                      GetWindowLong(hwnd,
                                                    GWL_EXSTYLE) | WS_EX_LAYERED);

                        /* set the tranclucency to 60% */
                        SetLayeredWindowAttributes(hwnd,
                                                   0,
                                                   (255 * 60) / 100,
                                                   LWA_ALPHA);

                    }
                }
            }
        }
        break;

        case WM_NCMOUSEMOVE:
        case WM_MOUSEMOVE:
            if (bOpaque == FALSE)
            {
                SetWindowLong(hwnd,
                          GWL_EXSTYLE,
                          GetWindowLong(hwnd,
                                        GWL_EXSTYLE) & ~WS_EX_LAYERED);

                RedrawWindow(hwnd,
                             NULL,
                             NULL,
                             RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);

                bOpaque = TRUE;
                SetTimer(hwnd,
                         ID_TIMER,
                         200,
                         NULL);
            }
        break;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDCANCEL)
                ShowHideWindow(hwnd);

            switch(LOWORD(wParam))
            {
                case ID_NEW:
                    MessageBox(hwnd, _T("Kapow!"), _T("Hit test"), MB_OK | MB_ICONEXCLAMATION);
                break;

                case ID_CLONESTAMP:
                case ID_COLORPICKER:
                case ID_ECLIPSE:
                case ID_ECLIPSESEL:
                case ID_ERASER:
                case ID_FREEFORM:
                case ID_LASOO:
                case ID_LINE:
                case ID_MAGICWAND:
                case ID_MOVE:
                case ID_MOVESEL:
                case ID_PAINTBRUSH:
                case ID_PAINTBUCKET:
                case ID_PENCIL:
                case ID_RECOLORING:
                case ID_RECTANGLE:
                case ID_ROUNDRECT:
                case ID_TEXT:
                case ID_ZOOM:
                    /*SendMessage(Info->hSelf,
                                LOWORD(wParam),
                                wParam,
                                lParam);*/
                break;
            }
        break;

        case WM_NCACTIVATE:
            /* FIXME: needs fully implementing */
            return DefWindowProc(hwnd,
                                 Message,
                                 TRUE,
                                 lParam);

        case WM_CLOSE:
            ShowHideWindow(hwnd);
        break;

        default:
            return DefWindowProc(hwnd,
                                 Message,
                                 wParam,
                                 lParam);
    }

    return 0;
}


BOOL
InitFloatWndClass(VOID)
{
    WNDCLASSEX wc = {0};

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = FloatToolbarWndProc;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL,
                            IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = szFloatWndClass;
    wc.hIconSm = NULL;

    return RegisterClassEx(&wc) != (ATOM)0;
}

VOID
UninitFloatWndImpl(VOID)
{
    UnregisterClass(szFloatWndClass,
                    hInstance);
}

