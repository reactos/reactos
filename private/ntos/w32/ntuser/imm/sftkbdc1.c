/**************************************************************************\
* Module Name: sftkbdc1.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Softkeyboard support for Simplified Chinese
*
* History:
* 03-Jan-1996 wkwok    Ported from Win95
\**************************************************************************/
#include "precomp.h"
#pragma hdrstop

#include "softkbd.h"

// Virtual Key for Letter Buttons
CONST BYTE SKC1VirtKey[BUTTON_NUM_C1] = {
   VK_OEM_3, '1', '2', '3', '4', '5', '6','7', '8', '9', '0', VK_OEM_MINUS, VK_OEM_EQUAL,
   'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', VK_OEM_LBRACKET, VK_OEM_RBRACKET, VK_OEM_BSLASH,
   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', VK_OEM_SEMICLN, VK_OEM_QUOTE,
   'Z', 'X', 'C', 'V', 'B', 'N', 'M', VK_OEM_COMMA, VK_OEM_PERIOD, VK_OEM_SLASH,
   VK_BACK, VK_TAB, VK_CAPITAL, VK_RETURN, VK_SHIFT, VK_INSERT, VK_DELETE, VK_SPACE,
   VK_ESCAPE
};

POINT gptButtonPos[BUTTON_NUM_C1]; // button point array, in the client area
BOOL  gfSoftKbdC1Init = FALSE;              // init flag

/**********************************************************************\
* InitSKC1ButtonPos -- init gptButtonPos
*
\**********************************************************************/
VOID InitSKC1ButtonPos()
{
    int  i, x, y;

    // init the first row
    y = 0;
    for (i=0, x=X_ROW_LETTER_C1; i < COL_LETTER_C1; i++, x += W_LETTER_BTN_C1) {
      gptButtonPos[i].x = x;
      gptButtonPos[i].y = y;
    }
    gptButtonPos[BACKSP_TYPE_C1].x = x;
    gptButtonPos[BACKSP_TYPE_C1].y = y;

    // init the second row
    y += H_LETTER_BTN_C1;
    x = 0;
    gptButtonPos[TAB_TYPE_C1].x = x;
    gptButtonPos[TAB_TYPE_C1].y = y;
    for (i=0, x=X_ROW2_LETTER_C1; i < COL2_LETTER_C1; i++, x += W_LETTER_BTN_C1) {
      gptButtonPos[i + COL_LETTER_C1].x = x;
      gptButtonPos[i + COL_LETTER_C1].y = y;
    }

    // init the third row
    y += H_LETTER_BTN_C1;
    x = 0;
    gptButtonPos[CAPS_TYPE_C1].x = x;
    gptButtonPos[CAPS_TYPE_C1].y = y;
    for (i=0, x=X_ROW3_LETTER_C1; i < COL3_LETTER_C1; i++, x += W_LETTER_BTN_C1) {
      gptButtonPos[i + COL_LETTER_C1 + COL2_LETTER_C1].x = x;
      gptButtonPos[i + COL_LETTER_C1 + COL2_LETTER_C1].y = y;
    }
    gptButtonPos[ENTER_TYPE_C1].x = x;
    gptButtonPos[ENTER_TYPE_C1].y = y;

    // init the forth row
    y += H_LETTER_BTN_C1;
    x = 0;
    gptButtonPos[SHIFT_TYPE_C1].x = x;
    gptButtonPos[SHIFT_TYPE_C1].y = y;
    for (i=0, x=X_ROW4_LETTER_C1; i < COL4_LETTER_C1; i++, x += W_LETTER_BTN_C1) {
      gptButtonPos[i + COL_LETTER_C1 + COL2_LETTER_C1 + COL3_LETTER_C1].x = x;
      gptButtonPos[i + COL_LETTER_C1 + COL2_LETTER_C1 + COL3_LETTER_C1].y = y;
    }

    // init the bottom row
    y += H_LETTER_BTN_C1;
    x = 0;
    gptButtonPos[INS_TYPE_C1].x = x;
    gptButtonPos[INS_TYPE_C1].y = y;
    x = X_DEL_C1;
    gptButtonPos[DEL_TYPE_C1].x = x;
    gptButtonPos[DEL_TYPE_C1].y = y;
    x += W_DEL_C1 + 2 * BORDER_C1;
    gptButtonPos[SPACE_TYPE_C1].x = x;
    gptButtonPos[SPACE_TYPE_C1].y = y;
    x = X_ESC_C1;
    gptButtonPos[ESC_TYPE_C1].x = x;
    gptButtonPos[ESC_TYPE_C1].y = y;

    return;
}


/**********************************************************************\
* SKC1DrawConvexRect --- draw button
*
*              (x1,y1)     x2-1
*               +----3------>^
*               |+----3-----||y1+1
*               ||          ||
*               33    1     42
*               ||          ||
*               |V          ||
*               |<----4-----+|
*         y2-1  ------2------+
*                             (x2,y2)
*
*  1 - light gray
*  2 - black
*  3 - white
*  4 - dark gray
*
\**********************************************************************/
VOID SKC1DrawConvexRect(
    HDC  hDC,
    int  x,
    int  y,
    int  nWidth,
    int  nHeight)
{
    // paint background
    SelectObject(hDC, GetStockObject(LTGRAY_BRUSH));
    SelectObject(hDC, GetStockObject(BLACK_PEN));
    Rectangle(hDC, x, y, x + nWidth, y + nHeight);

    // paint white border
    SelectObject(hDC, GetStockObject(WHITE_BRUSH));
    PatBlt(hDC, x, y + nHeight - 1, BORDER_C1, -nHeight + 1, PATCOPY);
    PatBlt(hDC, x, y, nWidth - 1 , BORDER_C1, PATCOPY);

    // paint dark gray border
    SelectObject(hDC, GetStockObject(GRAY_BRUSH));
    PatBlt(hDC, x + 1, y + nHeight -1, nWidth - BORDER_C1, -1, PATCOPY);
    PatBlt(hDC, x + nWidth - 1, y + nHeight - 1, -1, -nHeight + BORDER_C1, PATCOPY);

    return;
}


/**********************************************************************\
* SKC1InvertButton --- Invert Button
*
\**********************************************************************/
VOID SKC1InvertButton(
    HDC  hDC,
    int  uKeyIndex)
{
    int  nWidth, nHeight;

    if (uKeyIndex < 0) return;

    if (uKeyIndex < LETTER_NUM_C1) {
      nWidth = W_LETTER_BTN_C1;
      nHeight = H_LETTER_BTN_C1;
    } else {
      switch (uKeyIndex) {
        case BACKSP_TYPE_C1:
             nWidth = W_BACKSP_C1 + 2 * BORDER_C1;
             nHeight = H_LETTER_BTN_C1;
             break;
        case TAB_TYPE_C1:
             nWidth = W_TAB_C1 + 2 * BORDER_C1;
             nHeight = H_LETTER_BTN_C1;
             break;
        case CAPS_TYPE_C1:
             nWidth = W_CAPS_C1 + 2 * BORDER_C1;
             nHeight = H_LETTER_BTN_C1;
             break;
        case ENTER_TYPE_C1:
             nWidth = W_ENTER_C1 + 2 * BORDER_C1;
             nHeight = H_LETTER_BTN_C1;
             break;
        case SHIFT_TYPE_C1:
             nWidth = W_SHIFT_C1 + 2 * BORDER_C1;
             nHeight = H_LETTER_BTN_C1;
             break;
        case INS_TYPE_C1:
             nWidth = W_INS_C1 + 2 * BORDER_C1;
             nHeight = H_BOTTOM_BTN_C1;
             break;
        case DEL_TYPE_C1:
             nWidth = W_DEL_C1 + 2 * BORDER_C1;
             nHeight = H_BOTTOM_BTN_C1;
             break;
        case SPACE_TYPE_C1:
             nWidth = W_SPACE_C1 + 2 * BORDER_C1;
             nHeight = H_BOTTOM_BTN_C1;
             break;
        case ESC_TYPE_C1:
             nWidth = W_ESC_C1 + 2 * BORDER_C1;
             nHeight = H_BOTTOM_BTN_C1;
             break;
      }
    }

    BitBlt(hDC, gptButtonPos[uKeyIndex].x, gptButtonPos[uKeyIndex].y,
           nWidth, nHeight, hDC, gptButtonPos[uKeyIndex].x , gptButtonPos[uKeyIndex].y,
           DSTINVERT);

    return;
}


/**********************************************************************\
* SKC1DrawBitmap --- Draw bitmap within rectangle
*
\**********************************************************************/
VOID SKC1DrawBitmap(
    HDC hDC,
    int x,
    int y,
    int nWidth,
    int nHeight,
    LPWSTR lpszBitmap)
{
    HDC     hMemDC;
    HBITMAP hBitmap, hOldBmp;

    hBitmap = LoadBitmap(ghInst, lpszBitmap);
    hMemDC = CreateCompatibleDC(hDC);
    hOldBmp = SelectObject(hMemDC, hBitmap);

    BitBlt(hDC, x, y, nWidth, nHeight, hMemDC, 0 , 0, SRCCOPY);

    SelectObject(hMemDC, hOldBmp);
    DeleteDC(hMemDC);
    DeleteObject(hBitmap);

    return;
}


/**********************************************************************\
* SKC1DrawLabel -- Draw the label of button
*
\**********************************************************************/
VOID SKC1DrawLabel(
    HDC hDC,
    LPWSTR lpszLabel)
{
    HDC     hMemDC;
    HBITMAP hBitmap, hOldBmp;
    int     i, x;

    hBitmap = LoadBitmap(ghInst, lpszLabel);
    hMemDC = CreateCompatibleDC(hDC);
    hOldBmp = SelectObject(hMemDC, hBitmap);

    for (i=x=0; i < LETTER_NUM_C1; i++, x += SIZELABEL_C1){
     BitBlt(hDC, gptButtonPos[i].x + X_LABEL_C1, gptButtonPos[i].y + Y_LABEL_C1,
            SIZELABEL_C1, SIZELABEL_C1, hMemDC, x , 0, SRCCOPY);
    }

    SelectObject(hMemDC, hOldBmp);
    DeleteDC(hMemDC);
    DeleteObject(hBitmap);

    return;
}


/**********************************************************************\
* InitSKC1Bitmap -- init bitmap
*
\**********************************************************************/
VOID InitSKC1Bitmap(
    HDC  hDC,
    RECT rcClient)
{
    int  i;

    // draw softkbd rectangle
    SelectObject(hDC, GetStockObject(LTGRAY_BRUSH));
    SelectObject(hDC, GetStockObject(NULL_PEN));
    Rectangle(hDC, rcClient.left, rcClient.top, rcClient.right + 1, rcClient.bottom + 1);

    // draw letter buttons
    for (i = 0; i < LETTER_NUM_C1; i++) {
      SKC1DrawConvexRect(hDC, gptButtonPos[i].x, gptButtonPos[i].y,
                         W_LETTER_BTN_C1, H_LETTER_BTN_C1);
    }
    // draw letter label
    SKC1DrawLabel(hDC, MAKEINTRESOURCEW(LABEL_C1));

    // draw other buttons
    SKC1DrawConvexRect(hDC, gptButtonPos[BACKSP_TYPE_C1].x, gptButtonPos[BACKSP_TYPE_C1].y,
                       W_BACKSP_C1 + 2 * BORDER_C1, H_LETTER_BTN_C1);
    SKC1DrawBitmap(hDC, gptButtonPos[BACKSP_TYPE_C1].x + BORDER_C1, gptButtonPos[BACKSP_TYPE_C1].y + BORDER_C1,
                   W_BACKSP_C1, H_BACKSP_C1, MAKEINTRESOURCEW(BACKSP_C1));

    SKC1DrawConvexRect(hDC, gptButtonPos[TAB_TYPE_C1].x, gptButtonPos[TAB_TYPE_C1].y,
                       W_TAB_C1 + 2 * BORDER_C1, H_LETTER_BTN_C1);
    SKC1DrawBitmap(hDC, gptButtonPos[TAB_TYPE_C1].x + BORDER_C1, gptButtonPos[TAB_TYPE_C1].y + BORDER_C1,
                   W_TAB_C1, H_TAB_C1, MAKEINTRESOURCEW(TAB_C1));

    SKC1DrawConvexRect(hDC, gptButtonPos[CAPS_TYPE_C1].x, gptButtonPos[CAPS_TYPE_C1].y,
                       W_CAPS_C1 + 2 * BORDER_C1, H_LETTER_BTN_C1);
    SKC1DrawBitmap(hDC, gptButtonPos[CAPS_TYPE_C1].x + BORDER_C1, gptButtonPos[CAPS_TYPE_C1].y + BORDER_C1,
                   W_CAPS_C1, H_CAPS_C1, MAKEINTRESOURCEW(CAPS_C1));

    SKC1DrawConvexRect(hDC, gptButtonPos[ENTER_TYPE_C1].x, gptButtonPos[ENTER_TYPE_C1].y,
                       W_ENTER_C1 + 2 * BORDER_C1, H_LETTER_BTN_C1);
    SKC1DrawBitmap(hDC, gptButtonPos[ENTER_TYPE_C1].x + BORDER_C1, gptButtonPos[ENTER_TYPE_C1].y + BORDER_C1,
                   W_ENTER_C1, H_ENTER_C1, MAKEINTRESOURCEW(ENTER_C1));

    SKC1DrawConvexRect(hDC, gptButtonPos[SHIFT_TYPE_C1].x, gptButtonPos[SHIFT_TYPE_C1].y,
                       W_SHIFT_C1 + 2 * BORDER_C1, H_LETTER_BTN_C1);
    SKC1DrawBitmap(hDC, gptButtonPos[SHIFT_TYPE_C1].x + BORDER_C1, gptButtonPos[SHIFT_TYPE_C1].y + BORDER_C1,
                   W_SHIFT_C1, H_SHIFT_C1, MAKEINTRESOURCEW(SHIFT_C1));

    SKC1DrawConvexRect(hDC, gptButtonPos[INS_TYPE_C1].x, gptButtonPos[INS_TYPE_C1].y,
                       W_INS_C1 + 2 * BORDER_C1, H_BOTTOM_BTN_C1);
    SKC1DrawBitmap(hDC, gptButtonPos[INS_TYPE_C1].x + BORDER_C1, gptButtonPos[INS_TYPE_C1].y + BORDER_C1,
                   W_INS_C1, H_INS_C1, MAKEINTRESOURCEW(INS_C1));

    SKC1DrawConvexRect(hDC, gptButtonPos[DEL_TYPE_C1].x, gptButtonPos[DEL_TYPE_C1].y,
                       W_DEL_C1 + 2 * BORDER_C1, H_BOTTOM_BTN_C1);
    SKC1DrawBitmap(hDC, gptButtonPos[DEL_TYPE_C1].x + BORDER_C1, gptButtonPos[DEL_TYPE_C1].y + BORDER_C1,
                   W_DEL_C1, H_DEL_C1, MAKEINTRESOURCEW(DEL_C1));

    SKC1DrawConvexRect(hDC, gptButtonPos[SPACE_TYPE_C1].x, gptButtonPos[SPACE_TYPE_C1].y,
                       W_SPACE_C1 + 2 * BORDER_C1, H_BOTTOM_BTN_C1);

    SKC1DrawConvexRect(hDC, gptButtonPos[ESC_TYPE_C1].x, gptButtonPos[ESC_TYPE_C1].y,
                       W_ESC_C1 + 2 * BORDER_C1, H_BOTTOM_BTN_C1);
    SKC1DrawBitmap(hDC, gptButtonPos[ESC_TYPE_C1].x + BORDER_C1, gptButtonPos[ESC_TYPE_C1].y + BORDER_C1,
                   W_ESC_C1, H_ESC_C1, MAKEINTRESOURCEW(ESC_C1));

    return;
}


/**********************************************************************\
* CreateC1Window
*
* Init softkeyboard gloabl variabls, context and bitmap
*
\**********************************************************************/
LRESULT CreateC1Window(
    HWND hSKWnd)
{
    HGLOBAL   hSKC1Ctxt;
    PSKC1CTXT pSKC1Ctxt;

    // alloc and lock hSKC1CTxt
    hSKC1Ctxt = GlobalAlloc(GHND, sizeof(SKC1CTXT));
    if (!hSKC1Ctxt) {
        return (-1L);
    }

    pSKC1Ctxt = (PSKC1CTXT)GlobalLock(hSKC1Ctxt);
    if (!pSKC1Ctxt) {
        GlobalFree(hSKC1Ctxt);
        return (-1L);
    }

    // save handle in SKC1_CONTEXT
    SetWindowLongPtr(hSKWnd, SKC1_CONTEXT, (LONG_PTR)hSKC1Ctxt);

    // init global varialbles
    if (!gfSoftKbdC1Init){
      InitSKC1ButtonPos();
      gfSoftKbdC1Init = TRUE;
    }

    // no index and default char set
    pSKC1Ctxt->uKeyIndex = -1;
    pSKC1Ctxt->lfCharSet = GB2312_CHARSET;

    // init softkeyboard
    {
      HDC        hDC, hMemDC;
      HBITMAP    hBitmap, hOldBmp;
      RECT       rcClient;

      GetClientRect(hSKWnd, &rcClient);

      hDC = GetDC(hSKWnd);
      hMemDC = CreateCompatibleDC(hDC);
      hBitmap = CreateCompatibleBitmap(hDC, rcClient.right - rcClient.left,
                                       rcClient.bottom - rcClient.top);
      ReleaseDC(hSKWnd, hDC);

      hOldBmp = SelectObject(hMemDC, hBitmap);

      InitSKC1Bitmap(hMemDC, rcClient);

      SelectObject(hMemDC, hOldBmp);
      pSKC1Ctxt->hSoftkbd = hBitmap; // save hBitmap in SKC1CTXT

      DeleteDC(hMemDC);
    }

    // unlock hSKC1CTxt
    GlobalUnlock(hSKC1Ctxt);

    return (0L);
}


/**********************************************************************\
* DestroyC1Window
*
* Destroy softkeyboard context and bitmap
*
\**********************************************************************/
VOID DestroyC1Window(
    HWND hSKWnd)
{
    HGLOBAL   hSKC1Ctxt;
    PSKC1CTXT pSKC1Ctxt;
    HWND      hUIWnd;

    // Get and Lock hSKC1Ctxt
    hSKC1Ctxt = (HGLOBAL)GetWindowLongPtr(hSKWnd, SKC1_CONTEXT);
    if (!hSKC1Ctxt) return;

    pSKC1Ctxt = (PSKC1CTXT)GlobalLock(hSKC1Ctxt);
    if (!pSKC1Ctxt) return;

    if (pSKC1Ctxt->uState & FLAG_DRAG_C1) {
       SKC1DrawDragBorder(hSKWnd, &pSKC1Ctxt->ptSkCursor,
                          &pSKC1Ctxt->ptSkOffset);
    }

    DeleteObject(pSKC1Ctxt->hSoftkbd); // delete hBitmap

    // Unlock and Free hSKC1Ctxt
    GlobalUnlock(hSKC1Ctxt);
    GlobalFree(hSKC1Ctxt);

    // send message to parent window
    hUIWnd = GetWindow(hSKWnd, GW_OWNER);
    if (hUIWnd) {
      SendMessage(hUIWnd, WM_IME_NOTIFY, IMN_SOFTKBDDESTROYED, 0);\
    }

    return;
}


/**********************************************************************\
* ShowSKC1Window -- Show softkeyboard
*
\**********************************************************************/
VOID ShowSKC1Window(
    HDC  hDC,
    HWND hSKWnd)
{
    HGLOBAL   hSKC1Ctxt;
    PSKC1CTXT pSKC1Ctxt;

    // Get and Lock hSKC1Ctxt
    hSKC1Ctxt = (HGLOBAL)GetWindowLongPtr(hSKWnd, SKC1_CONTEXT);
    if (!hSKC1Ctxt) return;

    pSKC1Ctxt = (PSKC1CTXT)GlobalLock(hSKC1Ctxt);
    if (!pSKC1Ctxt) return;

    // create mem dc to show softkeyboard
    {
       HDC      hMemDC;
       HBITMAP  hOldBmp;
       RECT     rcClient;

       hMemDC = CreateCompatibleDC(hDC);
       hOldBmp = SelectObject(hMemDC, pSKC1Ctxt->hSoftkbd);
       GetClientRect(hSKWnd, &rcClient);
       BitBlt(hDC, 0, 0, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
              hMemDC, 0, 0, SRCCOPY);
       SelectObject(hMemDC, hOldBmp);
       DeleteDC(hMemDC);
    }

    // Unlock hSKC1Ctxt
    GlobalUnlock(hSKC1Ctxt);

    return;
}


/**********************************************************************\
* UpdateSKC1Window -- update softkeyboard
*
\**********************************************************************/
BOOL UpdateSKC1Window(
    HWND          hSKWnd,
    LPSOFTKBDDATA lpSoftKbdData)
{
    HGLOBAL   hSKC1Ctxt;
    PSKC1CTXT pSKC1Ctxt;
    LOGFONT   lfFont;
    HFONT     hOldFont, hFont;
    HDC       hDC, hMemDC;
    HBITMAP   hOldBmp;
    int       i;

    // check the lpSoftKbdData
    if (lpSoftKbdData->uCount!=2) return FALSE;

    // Get and Lock hSKC1Ctxt
    hSKC1Ctxt = (HGLOBAL)GetWindowLongPtr(hSKWnd, SKC1_CONTEXT);
    if (!hSKC1Ctxt) return FALSE;

    pSKC1Ctxt = (PSKC1CTXT)GlobalLock(hSKC1Ctxt);
    if (!pSKC1Ctxt) return FALSE;

    // create font
    hDC = GetDC(hSKWnd);
    hMemDC = CreateCompatibleDC(hDC);
    hOldBmp = SelectObject(hMemDC, pSKC1Ctxt->hSoftkbd);

    GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(lfFont), &lfFont);
    lfFont.lfHeight = -SIZEFONT_C1;
    if (pSKC1Ctxt->lfCharSet != DEFAULT_CHARSET) {
        lfFont.lfCharSet = (BYTE)pSKC1Ctxt->lfCharSet;
    }

    hFont = CreateFontIndirect(&lfFont);
    hOldFont = SelectObject(hMemDC, hFont);


    // update shift/non-shift chars
    for (i=0; i < LETTER_NUM_C1; i++) {
        pSKC1Ctxt->wNonShiftCode[i] = lpSoftKbdData->wCode[0][SKC1VirtKey[i]];
        pSKC1Ctxt->wShiftCode[i] = lpSoftKbdData->wCode[1][SKC1VirtKey[i]];
    }

    SetBkColor(hMemDC, 0x00BFBFBF);  // set text bk color ??

    for (i=0; i < LETTER_NUM_C1; i++) {
        int  nchar;
        RECT rc;

        // draw shift char.
        rc.left = gptButtonPos[i].x + X_SHIFT_CHAR_C1;
        rc.top = gptButtonPos[i].y + Y_SHIFT_CHAR_C1;
        rc.right = rc.left + SIZEFONT_C1;
        rc.bottom = rc.top + SIZEFONT_C1;

        nchar = (pSKC1Ctxt->wShiftCode[i] == 0) ? 0 : 1;

#if (WINVER >= 0x0400)
        DrawTextEx(hMemDC, (LPWSTR)&pSKC1Ctxt->wShiftCode[i],
                   nchar, &rc, DT_CENTER, NULL);
#else
        ExtTextOut(hMemDC,
            rc.left,
            rc.top,
            ETO_OPAQUE, &rc,
            (LPWSTR)&pSKC1Ctxt->wShiftCode[i], nchar, NULL);
#endif

        // draw non-shift char.
        rc.left = gptButtonPos[i].x + X_NONSHIFT_CHAR_C1;
        rc.top = gptButtonPos[i].y + Y_NONSHIFT_CHAR_C1;
        rc.right = rc.left + SIZEFONT_C1;
        rc.bottom = rc.top + SIZEFONT_C1;

        nchar = (pSKC1Ctxt->wNonShiftCode[i] == 0) ? 0 : 1;

#if (WINVER >= 0x0400)
        DrawTextEx(hMemDC, (LPWSTR)&pSKC1Ctxt->wNonShiftCode[i],
                   nchar, &rc, DT_CENTER, NULL);
#else
        ExtTextOut(hMemDC,
            rc.left,
            rc.top,
            ETO_OPAQUE, &rc,
            (LPWSTR)&pSKC1Ctxt->wNonShiftCode[i], nchar, NULL);
#endif
    }

    // init states
    if (pSKC1Ctxt->uState & FLAG_SHIFT_C1){
       SKC1InvertButton(hMemDC, SHIFT_TYPE_C1);
    }
    pSKC1Ctxt->uState = 0;

    SelectObject(hMemDC, hOldBmp);
    SelectObject(hMemDC, hOldFont);
    DeleteDC(hMemDC);

    DeleteObject(hFont);
    ReleaseDC(hSKWnd,hDC);

    // Unlock hSKC1Ctxt
    GlobalUnlock(hSKC1Ctxt);

    return TRUE;
}


/**********************************************************************\
* SKC1DrawDragBorder() -- Draw Drag Border
*
\**********************************************************************/
VOID SKC1DrawDragBorder(
    HWND    hWnd,               // window is dragged
    LPPOINT lpptCursor,         // the cursor position
    LPPOINT lpptOffset)         // the offset form cursor to window org
{
    HDC     hDC;
    RECT    rcWnd, rcWorkArea;
    int     cxBorder, cyBorder;
    int     x, y;
    extern void GetAllMonitorSize(LPRECT lprc);

    // get rectangle of work area
    GetAllMonitorSize(&rcWorkArea);

    cxBorder = GetSystemMetrics(SM_CXBORDER);   // width of border
    cyBorder = GetSystemMetrics(SM_CYBORDER);   // height of border

    // create DISPLAY dc to draw track
    hDC = CreateDC(L"DISPLAY", NULL, NULL, NULL);
    SelectObject(hDC, GetStockObject(GRAY_BRUSH));

    // start point (left,top)
    x = lpptCursor->x - lpptOffset->x;
    y = lpptCursor->y - lpptOffset->y;

    // check for the min boundary of the display
    if (x < rcWorkArea.left) {
        x = rcWorkArea.left;
    }

    if (y < rcWorkArea.top) {
        y = rcWorkArea.top;
    }

    // check for the max boundary of the display
    GetWindowRect(hWnd, &rcWnd);

    if (x + rcWnd.right - rcWnd.left > rcWorkArea.right) {
        x = rcWorkArea.right - (rcWnd.right - rcWnd.left);
    }

    if (y + rcWnd.bottom - rcWnd.top > rcWorkArea.bottom) {
        y = rcWorkArea.bottom - (rcWnd.bottom - rcWnd.top);
    }

    // adjust Offset
    lpptOffset->x = lpptCursor->x - x;
    lpptOffset->y = lpptCursor->y - y;

    // draw rectangle
    PatBlt(hDC, x, y, rcWnd.right - rcWnd.left - cxBorder, cyBorder, PATINVERT);
    PatBlt(hDC, x, y + cyBorder, cxBorder, rcWnd.bottom - rcWnd.top - cyBorder, PATINVERT);
    PatBlt(hDC, x + cxBorder, y + rcWnd.bottom - rcWnd.top, rcWnd.right -
           rcWnd.left - cxBorder, -cyBorder, PATINVERT);
    PatBlt(hDC, x + rcWnd.right - rcWnd.left, y, - cxBorder, rcWnd.bottom -
           rcWnd.top - cyBorder, PATINVERT);

    // delete DISPLAY DC
    DeleteDC(hDC);

    return;
}


/**********************************************************************\
* SKC1MousePosition() -- judge the cursor position
*
\**********************************************************************/

#define CHECK_RECT(name)  \
    if (ImmPtInRect(gptButtonPos[name ## _TYPE_C1].x,   \
            gptButtonPos[name ## _TYPE_C1].y,           \
            W_ ## name ## _C1 + 2 * BORDER_C1,          \
            H_ ## name ## _C1 + 2 * BORDER_C1,          \
            lpptCursor)) {                              \
        return name ## _TYPE_C1;                        \
    }

INT SKC1MousePosition(
    LPPOINT lpptCursor)
{
    int   i;

    // letter buttons
    for (i = 0; i < LETTER_NUM_C1; i++){

       if (ImmPtInRect(gptButtonPos[i].x,
                gptButtonPos[i].y,
                W_LETTER_BTN_C1,
                H_LETTER_BTN_C1,
                lpptCursor)) {
           return i;
       }
    }

    CHECK_RECT(BACKSP);
    CHECK_RECT(TAB);
    CHECK_RECT(CAPS);
    CHECK_RECT(ENTER);
    CHECK_RECT(SHIFT);
    CHECK_RECT(ESC);
    CHECK_RECT(SPACE);
    CHECK_RECT(INS);
    CHECK_RECT(DEL);

    return -1;
}

#undef CHECK_RECT


/**********************************************************************\
* SKC1ButtonDown
*
\**********************************************************************/
VOID SKC1ButtonDown(
    HWND      hSKWnd,
    PSKC1CTXT pSKC1Ctxt)
{
    // capture the mouse activity
    SetCapture(hSKWnd);

    // in drag area
    if (pSKC1Ctxt->uKeyIndex == -1) {
       pSKC1Ctxt->uState |= FLAG_DRAG_C1;

       SKC1DrawDragBorder(hSKWnd, &pSKC1Ctxt->ptSkCursor, &pSKC1Ctxt->ptSkOffset);
    } else {
       UINT uVirtKey = 0xff;
       BOOL bRet = FALSE;

       if (pSKC1Ctxt->uKeyIndex == SHIFT_TYPE_C1) {
          if (!(pSKC1Ctxt->uState & FLAG_SHIFT_C1)) {
             bRet = TRUE;
          }
       } else if (pSKC1Ctxt->uKeyIndex < LETTER_NUM_C1) {
          if (pSKC1Ctxt->uState & FLAG_SHIFT_C1) {
             uVirtKey = pSKC1Ctxt->wShiftCode[pSKC1Ctxt->uKeyIndex];
          }
          else {
             uVirtKey = pSKC1Ctxt->wNonShiftCode[pSKC1Ctxt->uKeyIndex];
          }

          if (uVirtKey) {
             bRet = TRUE;
          } else {
             MessageBeep(0xFFFFFFFF);
             pSKC1Ctxt->uKeyIndex = -1;
          }
       } else {
          bRet = TRUE;
       }

       if (bRet) {
          HDC      hDC, hMemDC;
          HBITMAP  hOldBmp;

          hDC = GetDC(hSKWnd);
          hMemDC = CreateCompatibleDC(hDC);
          hOldBmp = SelectObject(hMemDC, pSKC1Ctxt->hSoftkbd);

          SKC1InvertButton(hDC, pSKC1Ctxt->uKeyIndex);
          SKC1InvertButton(hMemDC, pSKC1Ctxt->uKeyIndex);

          SelectObject(hMemDC, hOldBmp);
          DeleteDC(hMemDC);
          ReleaseDC(hSKWnd,hDC);
       }

       if(uVirtKey) {
          pSKC1Ctxt->uState |= FLAG_FOCUS_C1;
       }
    }

    return;
}


/**********************************************************************\
* SKC1SetCursor
*
\**********************************************************************/
BOOL SKC1SetCursor(
   HWND   hSKWnd,
   LPARAM lParam)
{
    HGLOBAL   hSKC1Ctxt;
    PSKC1CTXT pSKC1Ctxt;
    POINT     ptSkCursor, ptSkOffset;
    int       uKeyIndex;

    // Get and lock hSKC1Ctxt
    hSKC1Ctxt = (HGLOBAL)GetWindowLongPtr(hSKWnd, SKC1_CONTEXT);
    if (!hSKC1Ctxt) {
        return (FALSE);
    }

    pSKC1Ctxt = (PSKC1CTXT)GlobalLock(hSKC1Ctxt);
    if (!pSKC1Ctxt) {
        return (FALSE);
    }

    if (pSKC1Ctxt->uState & FLAG_DRAG_C1){
        // in drag operation
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        GlobalUnlock(hSKC1Ctxt);
        return (TRUE);
    }

    GetCursorPos(&ptSkCursor);
    ptSkOffset = ptSkCursor;
    ScreenToClient(hSKWnd, &ptSkOffset);

    uKeyIndex = SKC1MousePosition(&ptSkOffset);

    if (uKeyIndex != -1) {
       SetCursor(LoadCursor(NULL, IDC_HAND));
    } else {
       SetCursor(LoadCursor(NULL, IDC_SIZEALL));
    }

    if (HIWORD(lParam) != WM_LBUTTONDOWN){
       // unlock hSKC1Ctxt
       GlobalUnlock(hSKC1Ctxt);
       return (TRUE);
    }

    pSKC1Ctxt->ptSkCursor = ptSkCursor;
    pSKC1Ctxt->ptSkOffset = ptSkOffset;
    pSKC1Ctxt->uKeyIndex = uKeyIndex;

    SKC1ButtonDown(hSKWnd, pSKC1Ctxt);

    // unlock hSKC1Ctxt
    GlobalUnlock(hSKC1Ctxt);
    return (TRUE);
}


/**********************************************************************\
* SKC1MouseMove
*
\**********************************************************************/
BOOL SKC1MouseMove(
    HWND   hSKWnd,
    WPARAM wParam,
    LPARAM lParam)
{
    HGLOBAL   hSKC1Ctxt;
    PSKC1CTXT pSKC1Ctxt;

    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    // get and lock hSKC1Ctxt
    hSKC1Ctxt = (HGLOBAL)GetWindowLongPtr(hSKWnd, SKC1_CONTEXT);
    if (!hSKC1Ctxt) {
       return (FALSE);
    }

    pSKC1Ctxt = (PSKC1CTXT)GlobalLock(hSKC1Ctxt);
    if (!pSKC1Ctxt) {
       return (FALSE);
    }

    if (pSKC1Ctxt->uState & FLAG_DRAG_C1) {
       SKC1DrawDragBorder(hSKWnd, &pSKC1Ctxt->ptSkCursor,
                          &pSKC1Ctxt->ptSkOffset);

       GetCursorPos(&pSKC1Ctxt->ptSkCursor);

       SKC1DrawDragBorder(hSKWnd, &pSKC1Ctxt->ptSkCursor,
                          &pSKC1Ctxt->ptSkOffset);
    } else if (pSKC1Ctxt->uKeyIndex != -1) {
       HDC      hDC, hMemDC;
       HBITMAP  hOldBmp;
       POINT    ptSkOffset;
       int      uKeyIndex;

       GetCursorPos(&ptSkOffset);
       ScreenToClient(hSKWnd, &ptSkOffset);
       uKeyIndex = SKC1MousePosition(&ptSkOffset);

       hDC = GetDC(hSKWnd);
       hMemDC = CreateCompatibleDC(hDC);
       hOldBmp = SelectObject(hMemDC, pSKC1Ctxt->hSoftkbd);

       if (((pSKC1Ctxt->uState & FLAG_FOCUS_C1) && (uKeyIndex != pSKC1Ctxt->uKeyIndex)) ||
           (!(pSKC1Ctxt->uState & FLAG_FOCUS_C1) && (uKeyIndex == pSKC1Ctxt->uKeyIndex))) {
          if ((pSKC1Ctxt->uKeyIndex != SHIFT_TYPE_C1) ||
              !(pSKC1Ctxt->uState & FLAG_SHIFT_C1)) {
             SKC1InvertButton(hDC, pSKC1Ctxt->uKeyIndex);
             SKC1InvertButton(hMemDC, pSKC1Ctxt->uKeyIndex);
          }
          pSKC1Ctxt->uState ^= FLAG_FOCUS_C1;
       }

       SelectObject(hMemDC, hOldBmp);
       DeleteDC(hMemDC);
       ReleaseDC(hSKWnd,hDC);
    }

    // unlock hSKC1Ctxt
    GlobalUnlock(hSKC1Ctxt);

    return (TRUE);
}


/**********************************************************************\
* SKC1ButtonUp
*
\**********************************************************************/
BOOL SKC1ButtonUp(
    HWND       hSKWnd,
    WPARAM     wParam,
    LPARAM     lParam)
{
    HGLOBAL   hSKC1Ctxt;
    PSKC1CTXT pSKC1Ctxt;
    POINT     pt;
    UINT      uVirtKey;
    BOOL      bRet = FALSE;

    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    // Get and lock hSKC1Ctxt
    hSKC1Ctxt = (HGLOBAL)GetWindowLongPtr(hSKWnd, SKC1_CONTEXT);
    if (!hSKC1Ctxt) {
        return (bRet);
    }

    pSKC1Ctxt = (PSKC1CTXT)GlobalLock(hSKC1Ctxt);
    if (!pSKC1Ctxt) {
        return (bRet);
    }

    ReleaseCapture();

    if (pSKC1Ctxt->uState & FLAG_DRAG_C1) {
       pSKC1Ctxt->uState &= ~(FLAG_DRAG_C1);

       SKC1DrawDragBorder(hSKWnd, &pSKC1Ctxt->ptSkCursor, &pSKC1Ctxt->ptSkOffset);

       pt.x = pSKC1Ctxt->ptSkCursor.x - pSKC1Ctxt->ptSkOffset.x;
       pt.y = pSKC1Ctxt->ptSkCursor.y - pSKC1Ctxt->ptSkOffset.y;

       SetWindowPos(hSKWnd, (HWND)NULL, pt.x, pt.y,
                     0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);

       // update IMC
       bRet = TRUE;
       {
          HWND          hUIWnd;
          HIMC          hImc;
          PINPUTCONTEXT pInputContext;

          hUIWnd = GetWindow(hSKWnd, GW_OWNER);
          hImc = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
          if (hImc) {
             pInputContext = ImmLockIMC(hImc);
             if (pInputContext) {
                pInputContext->ptSoftKbdPos = pt;
                pInputContext->fdwInit |= INIT_SOFTKBDPOS;
                ImmUnlockIMC(hImc);
             }
          }
       }
    } else if (pSKC1Ctxt->uKeyIndex != -1) {
       if (pSKC1Ctxt->uState & FLAG_FOCUS_C1) {
          if (pSKC1Ctxt->uKeyIndex == SHIFT_TYPE_C1) {
             if (pSKC1Ctxt->uState & FLAG_SHIFT_C1) {
                bRet = TRUE;
             } else {
                pSKC1Ctxt->uState |= FLAG_SHIFT_C1;
             }
          } else if ((pSKC1Ctxt->uKeyIndex < LETTER_NUM_C1) &&
                     (pSKC1Ctxt->uState & FLAG_SHIFT_C1)) {
                keybd_event((BYTE)VK_SHIFT, (BYTE)guScanCode[VK_SHIFT],
                            0, 0);
                uVirtKey = SKC1VirtKey[pSKC1Ctxt->uKeyIndex];
                keybd_event((BYTE)uVirtKey, (BYTE)guScanCode[uVirtKey],
                           0, 0);
                keybd_event((BYTE)uVirtKey, (BYTE)guScanCode[uVirtKey],
                           (DWORD)KEYEVENTF_KEYUP, 0);
                keybd_event((BYTE)VK_SHIFT, (BYTE)guScanCode[VK_SHIFT],
                            (DWORD)KEYEVENTF_KEYUP, 0);
                bRet = TRUE;
          } else {
                uVirtKey = SKC1VirtKey[pSKC1Ctxt->uKeyIndex];
                keybd_event((BYTE)uVirtKey, (BYTE)guScanCode[uVirtKey],
                         0, 0);
                keybd_event((BYTE)uVirtKey, (BYTE)guScanCode[uVirtKey],
                         (DWORD)KEYEVENTF_KEYUP, 0);
                bRet = TRUE;
          }

          if (bRet){
             HDC      hDC, hMemDC;
             HBITMAP  hOldBmp;

             hDC = GetDC(hSKWnd);
             hMemDC = CreateCompatibleDC(hDC);
             hOldBmp = SelectObject(hMemDC, pSKC1Ctxt->hSoftkbd);

             SKC1InvertButton(hDC, pSKC1Ctxt->uKeyIndex);
             SKC1InvertButton(hMemDC, pSKC1Ctxt->uKeyIndex);

             if ((pSKC1Ctxt->uKeyIndex != SHIFT_TYPE_C1) &&
                 (pSKC1Ctxt->uKeyIndex < LETTER_NUM_C1) &&
                 (pSKC1Ctxt->uState & FLAG_SHIFT_C1)) {
                SKC1InvertButton(hDC, SHIFT_TYPE_C1);
                SKC1InvertButton(hMemDC, SHIFT_TYPE_C1);
             }

             if ((pSKC1Ctxt->uKeyIndex < LETTER_NUM_C1) ||
                 (pSKC1Ctxt->uKeyIndex == SHIFT_TYPE_C1)) {
                pSKC1Ctxt->uState &= ~(FLAG_SHIFT_C1);
             }

             SelectObject(hMemDC, hOldBmp);
             DeleteDC(hMemDC);
             ReleaseDC(hSKWnd,hDC);
          }
          pSKC1Ctxt->uState &= ~ (FLAG_FOCUS_C1);
       }
       pSKC1Ctxt->uKeyIndex = -1;
    }

    // unlock hSKC1Ctxt
    GlobalUnlock(hSKC1Ctxt);

    return (bRet);
}


/**********************************************************************\
* SKWndProcC1 -- softkeyboard window procedure
*
\**********************************************************************/
LRESULT SKWndProcC1(
    HWND   hSKWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    LRESULT    lRet = 0L;

    switch (uMsg) {
       case WM_CREATE:
            lRet = CreateC1Window(hSKWnd);
            break;

       case WM_DESTROY:
            DestroyC1Window(hSKWnd);
            break;

       case WM_PAINT:
            {
               HDC         hDC;
               PAINTSTRUCT ps;

               hDC = BeginPaint(hSKWnd, &ps);
               ShowSKC1Window(hDC, hSKWnd);
               EndPaint(hSKWnd, &ps);
            }
            break;

       case WM_MOUSEACTIVATE:
            lRet = MA_NOACTIVATE;
            break;

       case WM_SETCURSOR:
            if (!SKC1SetCursor(hSKWnd, lParam)) {
               lRet = DefWindowProc(hSKWnd, uMsg, wParam, lParam);
            }
            break;

       case WM_MOUSEMOVE:
            if (!SKC1MouseMove(hSKWnd, wParam, lParam)) {
               lRet = DefWindowProc(hSKWnd, uMsg, wParam, lParam);
            }
            break;

       case WM_LBUTTONUP:
            if (!SKC1ButtonUp(hSKWnd, wParam, lParam)) {
               lRet = DefWindowProc(hSKWnd, uMsg, wParam, lParam);
            }
            break;

       case WM_IME_CONTROL:
            switch (wParam) {
               case IMC_GETSOFTKBDFONT:
                    {
                       HDC        hDC;
                       LOGFONT    lfFont;

                       hDC = GetDC(hSKWnd);
                       GetObject(GetStockObject(DEFAULT_GUI_FONT),
                                 sizeof(lfFont), &lfFont);
                       ReleaseDC(hSKWnd, hDC);
                       *(LPLOGFONT)lParam = lfFont;
                    }
                    break;

               case IMC_SETSOFTKBDFONT:
                    {
                       LOGFONT lfFont;

                       GetObject(GetStockObject(DEFAULT_GUI_FONT),
                           sizeof(lfFont), &lfFont);

                       // in differet version of Windows
                       if (lfFont.lfCharSet != ((LPLOGFONT)lParam)->lfCharSet) {
                           HGLOBAL    hSKC1Ctxt;
                           LPSKC1CTXT lpSKC1Ctxt;

                           hSKC1Ctxt = (HGLOBAL)GetWindowLongPtr(hSKWnd,
                               SKC1_CONTEXT);
                           if (!hSKC1Ctxt) {
                               return 1;
                           }

                           lpSKC1Ctxt = (LPSKC1CTXT)GlobalLock(hSKC1Ctxt);
                           if (!lpSKC1Ctxt) {
                               return 1;
                           }

                           lpSKC1Ctxt->lfCharSet =
                               ((LPLOGFONT)lParam)->lfCharSet;

                           GlobalUnlock(hSKC1Ctxt);
                       }
                    }
                    break;

               case IMC_GETSOFTKBDPOS:
                    {
                       RECT rcWnd;

                       GetWindowRect(hSKWnd, &rcWnd);

                       return MAKELRESULT(rcWnd.left, rcWnd.top);
                    }
                    break;

               case IMC_SETSOFTKBDPOS:
                    {
                       SetWindowPos(hSKWnd, NULL,
                            ((LPPOINTS)lParam)->x, ((LPPOINTS)lParam)->y,
                            0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);
                       return (0);
                    }
                    break;

               case IMC_SETSOFTKBDDATA:
                    if (UpdateSKC1Window(hSKWnd, (LPSOFTKBDDATA)lParam)) {
                       InvalidateRect(hSKWnd, NULL, FALSE);
                    } else lRet = -1L;
                    break;

               case IMC_GETSOFTKBDSUBTYPE:
               case IMC_SETSOFTKBDSUBTYPE:
                    {
                       HGLOBAL   hSKC1Ctxt;
                       PSKC1CTXT pSKC1Ctxt;

                       lRet = -1L;

                       hSKC1Ctxt = (HGLOBAL)GetWindowLongPtr(hSKWnd, SKC1_CONTEXT);
                       if (!hSKC1Ctxt) break;

                       pSKC1Ctxt = (PSKC1CTXT)GlobalLock(hSKC1Ctxt);
                       if (!pSKC1Ctxt) break;

                       if (wParam == IMC_GETSOFTKBDSUBTYPE) {
                          lRet = pSKC1Ctxt->uSubtype;
                       } else {
                          lRet = pSKC1Ctxt->uSubtype;
                          pSKC1Ctxt->uSubtype = (UINT)lParam;
                       }

                       GlobalUnlock(hSKC1Ctxt);
                    }
                    break;
            }
            break;

       default:
            lRet = DefWindowProc(hSKWnd, uMsg, wParam, lParam);
    }

    return (lRet);
}
