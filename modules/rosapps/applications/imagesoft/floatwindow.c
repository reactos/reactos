#include "precomp.h"

static const TCHAR szFloatWndClass[] = TEXT("ImageSoftFloatWndClass");

#define ID_TIMER1 1
#define ID_TIMER2 2
#define ID_TIMER3 3

TBBUTTON ToolsButtons[] = {
/*   iBitmap,            idCommand,      fsState,         fsStyle,                                     bReserved[2], dwData, iString */
    {TBICON_RECTSEL,     ID_RECTSEL,     TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_GROUP | TBSTYLE_CHECK, {0}, 0, 0},    /* rectangle select */
    {TBICON_MOVESEL,     ID_MOVESEL,     TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_GROUP | TBSTYLE_CHECK, {0}, 0, 0},    /* move selected pixels */
    {TBICON_LASOO,       ID_LASOO,       TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_GROUP | TBSTYLE_CHECK, {0}, 0, 0},    /* lasso select */
    {TBICON_MOVE,        ID_MOVE,        TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_GROUP | TBSTYLE_CHECK, {0}, 0, 0},    /* move selection */
    {TBICON_ECLIPSESEL,  ID_ECLIPSESEL,  TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_GROUP | TBSTYLE_CHECK, {0}, 0, 0},    /* elipse select */
    {TBICON_ZOOM,        ID_ZOOM,        TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_GROUP | TBSTYLE_CHECK, {0}, 0, 0},    /* zoom */
    {TBICON_MAGICWAND,   ID_MAGICWAND,   TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_GROUP | TBSTYLE_CHECK, {0}, 0, 0},    /* magic wand */
    {TBICON_TEXT,        ID_TEXT,        TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_GROUP | TBSTYLE_CHECK, {0}, 0, 0},    /* text */
    {TBICON_PAINTBRUSH,  ID_PAINTBRUSH,  TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_GROUP | TBSTYLE_CHECK, {0}, 0, 0},    /* paintbrush */
    {TBICON_ERASER,      ID_ERASER,      TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_GROUP | TBSTYLE_CHECK, {0}, 0, 0},    /* eraser */
    {TBICON_PENCIL,      ID_PENCIL,      TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_GROUP | TBSTYLE_CHECK, {0}, 0, 0},    /* pencil */
    {TBICON_COLORPICKER, ID_COLORPICKER, TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_GROUP | TBSTYLE_CHECK, {0}, 0, 0},    /* color picker */
    {TBICON_CLONESTAMP,  ID_CLONESTAMP,  TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_GROUP | TBSTYLE_CHECK, {0}, 0, 0},    /* clone stamp */
    {TBICON_RECOLORING,  ID_RECOLORING,  TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_GROUP | TBSTYLE_CHECK, {0}, 0, 0},    /* recolor */
    {TBICON_PAINTBUCKET, ID_PAINTBUCKET, TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_GROUP | TBSTYLE_CHECK, {0}, 0, 0},    /* paint bucket */
    {TBICON_LINE,        ID_LINE,        TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_GROUP | TBSTYLE_CHECK, {0}, 0, 0},    /* line */
    {TBICON_RECTANGLE,   ID_RECTANGLE,   TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_GROUP | TBSTYLE_CHECK, {0}, 0, 0},    /* rectangle */
    {TBICON_ROUNDRECT,   ID_ROUNDRECT,   TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_GROUP | TBSTYLE_CHECK, {0}, 0, 0},    /* round rectangle */
    {TBICON_ECLIPSE,     ID_ECLIPSE,     TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_GROUP | TBSTYLE_CHECK, {0}, 0, 0},    /* elipse */
    {TBICON_FREEFORM,    ID_FREEFORM,    TBSTATE_ENABLED, BTNS_BUTTON | TBSTYLE_GROUP | TBSTYLE_CHECK, {0}, 0, 0},    /* free form */

    {10, 0, TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},
};

TBBUTTON HistoryButtons[] = {
    {TBICON_BACKSM,     ID_BACK,    TBSTATE_ENABLED, BTNS_BUTTON,  {0}, 0, 0 },   /* back */
    {TBICON_UNDOSM,     ID_UNDO,    TBSTATE_ENABLED, BTNS_BUTTON,  {0}, 0, 0 },   /* undo */
    {TBICON_REDOSM,     ID_REDO,    TBSTATE_ENABLED, BTNS_BUTTON,  {0}, 0, 0 },   /* redo */
    {TBICON_FORWARDSM,  ID_FORWARD, TBSTATE_ENABLED, BTNS_BUTTON,  {0}, 0, 0 },   /* forward */
    {TBICON_DELETESM,   ID_DELETE,  TBSTATE_ENABLED, BTNS_BUTTON,  {0}, 0, 0 },   /* delete */
};


BOOL
ShowHideWindow(HWND hwnd)
{
    if (IsWindowVisible(hwnd))
        return ShowWindow(hwnd, SW_HIDE);
    else
        return ShowWindow(hwnd, SW_SHOW);
}


BOOL
FloatToolbarCreateToolsGui(PMAIN_WND_INFO Info)
{
    HWND hTb;
    HIMAGELIST hImageList;
    UINT NumButtons;

    NumButtons = ARRAYSIZE(ToolsButtons);
    hTb = CreateWindowEx(0,
                         TOOLBARCLASSNAME,
                         NULL,
                         WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_WRAPABLE | CCS_NODIVIDER,
                         0, 0, 32, 200,
                         Info->fltTools->hSelf,
                         NULL,
                         hInstance,
                         NULL);
    if (hTb != NULL)
    {
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

        hImageList = InitImageList(IDB_TOOLSRECTSEL, NumButtons - 1); // -1 because of the last separator.

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

        return TRUE;
    }

    return FALSE;
}


VOID
FloatWindowPaintColorPicker(HWND hColorPicker)
{
    HDC hdc;
    RECT rect;

    InvalidateRect(hColorPicker,
                   NULL,
                   TRUE);
    UpdateWindow(hColorPicker);

    hdc = GetDC(hColorPicker);

    GetClientRect(hColorPicker,
                  &rect);

    Ellipse(hdc,
            rect.left,
            rect.top,
            rect.right,
            rect.bottom);

    ReleaseDC(hColorPicker,
              hdc);

}

VOID
FloatWindowPaintHueSlider(HWND hHueSlider)
{
    HDC hdc;
    RECT rect;

    InvalidateRect(hHueSlider,
                   NULL,
                   TRUE);
    UpdateWindow(hHueSlider);

    hdc = GetDC(hHueSlider);

    GetClientRect(hHueSlider,
                  &rect);

    Rectangle(hdc,
              rect.left,
              rect.top,
              rect.right,
              rect.bottom);

    ReleaseDC(hHueSlider,
              hdc);

}


BOOL
FloatToolbarCreateColorsGui(PMAIN_WND_INFO Info)
{
    HWND hColorPicker;
    HWND hHueSlider;
    HWND hMouseButton;
    HWND hMore;
    RECT rect;
    HBITMAP hMoreBitmap;

    GetClientRect(Info->fltColors->hSelf,
                  &rect);

    hColorPicker = CreateWindowEx(0,
                                  WC_STATIC,
                                  NULL,
                                  WS_CHILD | WS_VISIBLE | CBS_DROPDOWN,
                                  2,
                                  2,
                                  (int) (rect.right * 0.65),
                                  rect.bottom - 2,
                                  Info->fltColors->hSelf,
                                  NULL,
                                  hInstance,
                                  NULL);
    if (hColorPicker == NULL)
        return FALSE;

    hHueSlider = CreateWindowEx(0,
                                WC_STATIC,
                                NULL,
                                WS_CHILD | WS_VISIBLE | CBS_DROPDOWN,
                                145,
                                35,
                                25,
                                135,
                                Info->fltColors->hSelf,
                                NULL,
                                hInstance,
                                NULL);
    if (hHueSlider == NULL)
        return FALSE;

    hMouseButton = CreateWindowEx(0,
                                  WC_COMBOBOX,
                                  NULL,
                                  WS_CHILD | WS_VISIBLE | CBS_DROPDOWN,
                                  118, 5, 75, 25,
                                  Info->fltColors->hSelf,
                                  NULL,
                                  hInstance,
                                  NULL);
    if (hMouseButton == NULL)
        return FALSE;

    MakeFlatCombo(hMouseButton);

    /* temp, just testing */
    SendMessage(hMouseButton, CB_ADDSTRING, 0, (LPARAM)_T("Primary"));
    SendMessage(hMouseButton, CB_ADDSTRING, 0, (LPARAM)_T("Secondary"));
    SendMessage(hMouseButton, CB_SETCURSEL, 0, 0);


    hMore = CreateWindowEx(WS_EX_STATICEDGE,
                           WC_BUTTON,
                           NULL,
                           WS_CHILD | WS_VISIBLE | BS_BITMAP,
                           rect.right - 15,
                           rect.bottom - 15,
                           15, 15,
                           Info->fltColors->hSelf,
                           NULL,
                           hInstance,
                           NULL);
    if (hMore == NULL)
        return FALSE;

    hMoreBitmap = (HBITMAP)LoadImage(hInstance,
                                     MAKEINTRESOURCE(IDB_COLORSMORE),
                                     IMAGE_BITMAP,
                                     12,
                                     11,
                                     LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS);
    if (hMoreBitmap != NULL)
    {
        SendMessage(hMore,
                    BM_SETIMAGE,
                    IMAGE_BITMAP,
                    (LPARAM)hMoreBitmap);
    }


    /* temp functions for playing about with possible layouts */
    FloatWindowPaintHueSlider(hHueSlider);
    FloatWindowPaintColorPicker(hColorPicker);

    if (hColorPicker != NULL)
    {
        HDC hDc = GetDC(hColorPicker);
        TextOut(hDc, 8, 75, _T("Possible layout?"), 16);
        ReleaseDC(hColorPicker, hDc);
    }

    return TRUE;

}


BOOL
FloatToolbarCreateHistoryGui(PMAIN_WND_INFO Info)
{
    HWND hList;
    HWND hButtons;
    HIMAGELIST hImageList;
    UINT NumButtons;

    hList = CreateWindowEx(0,
                           WC_LISTBOX,
                           NULL,
                           WS_CHILD | WS_VISIBLE | LBS_EXTENDEDSEL,
                           0, 0, 143, 100,
                           Info->fltHistory->hSelf,
                           NULL,
                           hInstance,
                           NULL);
    if (hList == NULL)
        return FALSE;

    NumButtons = ARRAYSIZE(HistoryButtons);
    hButtons = CreateWindowEx(0,
                              TOOLBARCLASSNAME,
                              NULL,
                              WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | CCS_BOTTOM | CCS_NODIVIDER,
                              0, 0, 0, 0,
                              Info->fltHistory->hSelf,
                              NULL,
                              hInstance,
                              NULL);
    if (hButtons != NULL)
    {
        SendMessage(hButtons,
                    TB_BUTTONSTRUCTSIZE,
                    sizeof(ToolsButtons[0]),
                    0);

        SendMessage(hButtons,
                    TB_SETBITMAPSIZE,
                    0,
                    (LPARAM)MAKELONG(10, 10));

        hImageList = InitImageList(IDB_HISTBACK, NumButtons);

        ImageList_Destroy((HIMAGELIST)SendMessage(hButtons,
                                                  TB_SETIMAGELIST,
                                                  0,
                                                  (LPARAM)hImageList));

        SendMessage(hButtons,
                    TB_SETBUTTONSIZE,
                    0,
                    MAKELONG(18, 16));

        SendMessage(hButtons,
                    TB_ADDBUTTONS,
                    NumButtons,
                    (LPARAM)HistoryButtons);

        return TRUE;
    }

    return FALSE;
}


static VOID
DoTimer(PFLT_WND FltInfo,
        UINT_PTR idTimer)
{
    switch (idTimer)
    {
        /* timer to check if cursor is in toolbar coords */
        case ID_TIMER1:
        {
            POINT pt;

            /* kill timer if toobar is not opaque */
            if (FltInfo->bOpaque != TRUE)
            {
                KillTimer(FltInfo->hSelf,
                          ID_TIMER1);
                break;
            }

            if (GetCursorPos(&pt))
            {
                RECT rect;

                if (GetWindowRect(FltInfo->hSelf,
                                  &rect))
                {
                    if (!PtInRect(&rect,
                                  pt))
                    {
                        KillTimer(FltInfo->hSelf,
                                  ID_TIMER1);
                        KillTimer(FltInfo->hSelf,
                                  ID_TIMER2);

                        /* timer to fade out toolbar */
                        SetTimer(FltInfo->hSelf,
                                 ID_TIMER3,
                                 50,
                                 NULL);
                    }
                }
            }
        }
        break;

        /* timer to fade in toolbar */
        case ID_TIMER2:
        {
            SetLayeredWindowAttributes(FltInfo->hSelf,
                                       0,
                                       (255 * FltInfo->Transparancy) / 100,
                                       LWA_ALPHA);

            /* increment transparancy until it is opaque (100) */
            FltInfo->Transparancy += 5;

            if (FltInfo->Transparancy == 100)
            {
                SetWindowLongPtr(FltInfo->hSelf,
                                 GWL_EXSTYLE,
                                 GetWindowLongPtr(FltInfo->hSelf,
                                                  GWL_EXSTYLE) & ~WS_EX_LAYERED);

                FltInfo->bOpaque = TRUE;

                KillTimer(FltInfo->hSelf,
                          ID_TIMER2);
            }
        }
        break;

        case ID_TIMER3:
        {
            LONG_PTR Style;

            Style = GetWindowLongPtr(FltInfo->hSelf,
                                     GWL_EXSTYLE);

            if (Style & ~WS_EX_LAYERED)
            {
                SetWindowLongPtr(FltInfo->hSelf,
                                 GWL_EXSTYLE,
                                 Style | WS_EX_LAYERED);
            }

            FltInfo->Transparancy -= 5;

            if (FltInfo->Transparancy >= 60)
            {
                /* set the tranclucency to 60% */
                SetLayeredWindowAttributes(FltInfo->hSelf,
                                           0,
                                           (255 * FltInfo->Transparancy) / 100,
                                           LWA_ALPHA);

                if (FltInfo->Transparancy == 60)
                {
                    FltInfo->bOpaque = FALSE;

                    KillTimer(FltInfo->hSelf,
                              ID_TIMER3);
                }

            }
        }
        break;
    }
}

LRESULT CALLBACK
FloatToolbarWndProc(HWND hwnd,
                    UINT Message,
                    WPARAM wParam,
                    LPARAM lParam)
{
    PFLT_WND FltInfo;

    /* Get the window context */
    FltInfo = (PFLT_WND)GetWindowLongPtr(hwnd,
                                         GWLP_USERDATA);
    if (FltInfo == NULL && Message != WM_CREATE)
    {
        goto HandleDefaultMessage;
    }

    switch(Message)
    {
        case WM_CREATE:
        {
            FltInfo = (PFLT_WND)(((LPCREATESTRUCT)lParam)->lpCreateParams);

            /*FIXME: read this from registry */
//            FltInfo->bShow = TRUE;

            SetWindowLongPtr(hwnd,
                             GWLP_USERDATA,
                             (LONG_PTR)FltInfo);

            FltInfo->bOpaque = FALSE;

            SetWindowLongPtr(hwnd,
                             GWL_EXSTYLE,
                             GetWindowLongPtr(hwnd,
                                              GWL_EXSTYLE) | WS_EX_LAYERED);

            /* set the tranclucency to 60% */
            FltInfo->Transparancy = 60;
            SetLayeredWindowAttributes(hwnd,
                                       0,
                                       (255 * FltInfo->Transparancy) / 100,
                                       LWA_ALPHA);
        }
        break;

        case WM_TIMER:
        {
            DoTimer(FltInfo,
                    wParam);
        }
        break;

        case WM_NCMOUSEMOVE:
        case WM_MOUSEMOVE:
        {
            if (FltInfo->bOpaque == FALSE)
            {

                RedrawWindow(hwnd,
                             NULL,
                             NULL,
                             RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);

                FltInfo->bOpaque = TRUE;
                //MessageBox(NULL, _T("in"), _T("Hit test"), MB_OK | MB_ICONEXCLAMATION);

                /* timer to check if cursor is in toolbar coords */
                SetTimer(hwnd,
                         ID_TIMER1,
                         200,
                         NULL);

                /* timer to fade in the toolbars */
                SetTimer(hwnd,
                         ID_TIMER2,
                         50,
                         NULL);
            }
        }
        break;

        case WM_CLOSE:
            ShowHideWindow(FltInfo->hSelf);
        break;

        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDCANCEL)
                ShowHideWindow(FltInfo->hSelf);

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
        }
        break;

        case WM_NCACTIVATE:
            /* FIXME: needs fully implementing */
            return DefWindowProc(hwnd,
                                 Message,
                                 TRUE,
                                 lParam);
        break;

        case WM_DESTROY:
            SetWindowLongPtr(hwnd,
                             GWLP_USERDATA,
                             0);
        break;

        default:
HandleDefaultMessage:
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

    wc.cbSize = sizeof(wc);
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


