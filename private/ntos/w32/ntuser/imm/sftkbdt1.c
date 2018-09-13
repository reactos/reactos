/**************************************************************************\
* Module Name: sftkbdt1.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Soft keyboard support for Traditional Chinese
*
* History:
* 02-Jan-1996 wkwok    - ported from Win95
\**************************************************************************/
#include "precomp.h"
#pragma hdrstop

#include "softkbd.h"


CONST BYTE bSKT1VirtKey[SKT1_TOTAL_KEY_NUM] = {     // Virtual Key for Letter Buttons
//  `          1    2    3    4    5    6    7    8    9    0
    VK_OEM_3, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
//  -             =             \\                line 1 letter above
    VK_OEM_MINUS, VK_OEM_EQUAL, VK_OEM_BSLASH,
//   q    w    e    r    t    y    u    i    o    p
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
//  [                ]                            line 2 letter above
    VK_OEM_LBRACKET, VK_OEM_RBRACKET,
//   a    s    d    f    g    h    j    k    l
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
//   ;               '                            line 3 letter above
    VK_OEM_SEMICLN, VK_OEM_QUOTE,
//   z    x    c    v    b    n    m
    'Z', 'X', 'C', 'V', 'B', 'N', 'M',
//  ,              .              /               line 4 letter above
    VK_OEM_COMMA,  VK_OEM_PERIOD, VK_OEM_SLASH,
//  <-       tab     caps        enter
    VK_BACK, VK_TAB, VK_CAPITAL, VK_RETURN,
//  shift1    shift2    ctrl1       ctrl2
    VK_SHIFT, VK_SHIFT, VK_CONTROL, VK_CONTROL,
//  alt1     alt2     esc        space            special key above
    VK_MENU, VK_MENU, VK_ESCAPE, VK_SPACE
};


LOGFONT lfSKT1Font;


/**********************************************************************/
/* GetSKT1TextMetric                                                  */
/**********************************************************************/

void GetSKT1TextMetric(
     LPTEXTMETRIC   lptm)
{
    HDC     hDC;
    HFONT   hOldFont;
    SIZE    szDbcs;
    const WCHAR wcDbcs = 0x4e11;

    hDC = GetDC((HWND)NULL);

    // get the 9 pixels font
    RtlZeroMemory(&lfSKT1Font, sizeof lfSKT1Font);
    lfSKT1Font.lfHeight = -12;
    lfSKT1Font.lfWeight = FW_NORMAL;

    lfSKT1Font.lfCharSet = CHINESEBIG5_CHARSET;

    lfSKT1Font.lfOutPrecision = OUT_TT_ONLY_PRECIS;
    lfSKT1Font.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lfSKT1Font.lfQuality = PROOF_QUALITY;
    lfSKT1Font.lfPitchAndFamily = FIXED_PITCH|FF_MODERN;

    hOldFont = (HFONT)SelectObject(hDC, CreateFontIndirect(&lfSKT1Font));

    GetTextMetrics(hDC, lptm);

    /*
     * NT5: Dirty hack for "Lucida Console" which was "font linked" on NT5, and
     * GetTextMetrics API does not return correct text metrics.
     */
    if (GetTextExtentPoint32(hDC, &wcDbcs, 1, &szDbcs) && lptm->tmMaxCharWidth < szDbcs.cx) {
        TAGMSG2(DBGTAG_IMM, "GetSKT1TextMetric: tmMaxCharWidth(%d) is smaller than real width(%d).",
                lptm->tmMaxCharWidth, szDbcs.cx);
        lptm->tmMaxCharWidth = szDbcs.cx;
    }

    DeleteObject(SelectObject(hDC, hOldFont));

    ReleaseDC((HWND)NULL, hDC);

    return;
}

/**********************************************************************/
/* InitSKT1ButtonPos                                                  */
/**********************************************************************/
void InitSKT1ButtonPos(
    LPSKT1CTXT lpSKT1Ctxt)
{
    TEXTMETRIC  tm;
    int         nButtonWidthUnit, nButtonHeightUnit;
    int         i, nLetterButtonStart, nLetterButtonEnd, xStartPos, yStartPos;

    GetSKT1TextMetric(&tm);

    nButtonWidthUnit = tm.tmMaxCharWidth + SKT1_LABEL_BMP_X - SKT1_XOVERLAP;

#define INIT_WIDTH(name, width) \
            lpSKT1Ctxt->nButtonWidth[SKT1_ ## name ## _TYPE] = (width)

    INIT_WIDTH(LETTER,      nButtonWidthUnit * SKT1_LETTER_WIDTH_TIMES / 2);
    INIT_WIDTH(BACKSPACE,   nButtonWidthUnit * SKT1_BACKSPACE_WIDTH_TIMES / 2 + 1);
    INIT_WIDTH(TAB,         nButtonWidthUnit * SKT1_TAB_WIDTH_TIMES / 2 + (SKT1_XIN + 1) / 2);
    INIT_WIDTH(CAPS,        nButtonWidthUnit * SKT1_CAPS_WIDTH_TIMES / 2 + SKT1_XIN);
    INIT_WIDTH(ENTER,       nButtonWidthUnit * SKT1_ENTER_WIDTH_TIMES / 2 + (SKT1_XIN + 1) / 2);
    INIT_WIDTH(SHIFT,       nButtonWidthUnit * SKT1_SHIFT_WIDTH_TIMES / 2 + SKT1_XIN + (SKT1_XIN + 1) / 2);
    INIT_WIDTH(CTRL,        nButtonWidthUnit * SKT1_CTRL_WIDTH_TIMES / 2 + (SKT1_XIN + 1) / 2);
    INIT_WIDTH(ALT,         nButtonWidthUnit * SKT1_ALT_WIDTH_TIMES / 2 + (SKT1_XIN + 1) / 2);
    INIT_WIDTH(ESC,         nButtonWidthUnit * SKT1_ESC_WIDTH_TIMES / 2 + SKT1_XIN / 2);
    INIT_WIDTH(SPACE,       nButtonWidthUnit * SKT1_SPACE_WIDTH_TIMES / 2 + SKT1_XIN * 5);

#undef INIT_WIDTH

    nButtonHeightUnit = tm.tmHeight + SKT1_LABEL_BMP_Y;
    lpSKT1Ctxt->nButtonHeight[0] = nButtonHeightUnit;
    lpSKT1Ctxt->nButtonHeight[1] = nButtonHeightUnit * 2 + SKT1_YIN; // enter

    // init first row attribute
    xStartPos = gptRaiseEdge.x + SKT1_XOUT + (SKT1_XIN + 1) / 2;
    yStartPos = gptRaiseEdge.y + SKT1_YOUT + (SKT1_YIN + 1) / 2;

    nLetterButtonStart = 0;
    nLetterButtonEnd = nLetterButtonStart + SKT1_ROW1_LETTER_NUM;

    for (i = nLetterButtonStart; i < nLetterButtonEnd; i++) {
        lpSKT1Ctxt->ptButtonPos[i].x = xStartPos;
        lpSKT1Ctxt->ptButtonPos[i].y = yStartPos;
        xStartPos += (lpSKT1Ctxt->nButtonWidth[SKT1_LETTER_TYPE] + SKT1_XIN);
    }
    // backspace
    lpSKT1Ctxt->ptButtonPos[SKT1_BACKSPACE_INDEX].x = xStartPos;
    lpSKT1Ctxt->ptButtonPos[SKT1_BACKSPACE_INDEX].y = yStartPos;

    // init second row attribute
    xStartPos = gptRaiseEdge.x + SKT1_XOUT + (SKT1_XIN + 1) / 2;
    yStartPos += (nButtonHeightUnit + SKT1_YIN);
    // tab
    lpSKT1Ctxt->ptButtonPos[SKT1_TAB_INDEX].x = xStartPos;
    lpSKT1Ctxt->ptButtonPos[SKT1_TAB_INDEX].y = yStartPos;
    xStartPos += (lpSKT1Ctxt->nButtonWidth[SKT1_TAB_TYPE] + SKT1_XIN);
    // letter
    nLetterButtonStart = nLetterButtonEnd;
    nLetterButtonEnd = nLetterButtonStart + SKT1_ROW2_LETTER_NUM;
    for (i = nLetterButtonStart; i < nLetterButtonEnd; i++) {
        lpSKT1Ctxt->ptButtonPos[i].x = xStartPos;
        lpSKT1Ctxt->ptButtonPos[i].y = yStartPos;
        xStartPos += (lpSKT1Ctxt->nButtonWidth[SKT1_LETTER_TYPE] + SKT1_XIN);
    }
    // enter
    lpSKT1Ctxt->ptButtonPos[SKT1_ENTER_INDEX].x = xStartPos;
    lpSKT1Ctxt->ptButtonPos[SKT1_ENTER_INDEX].y = yStartPos;

    // init third row
    xStartPos = gptRaiseEdge.x + SKT1_XOUT + (SKT1_XIN + 1) / 2;
    yStartPos += (nButtonHeightUnit + SKT1_YIN);
    // caps
    lpSKT1Ctxt->ptButtonPos[SKT1_CAPS_INDEX].x = xStartPos;
    lpSKT1Ctxt->ptButtonPos[SKT1_CAPS_INDEX].y = yStartPos;
    xStartPos += (lpSKT1Ctxt->nButtonWidth[SKT1_CAPS_TYPE] + SKT1_XIN);
    // letter
    nLetterButtonStart = nLetterButtonEnd;
    nLetterButtonEnd = nLetterButtonStart + SKT1_ROW3_LETTER_NUM;
    for (i = nLetterButtonStart; i < nLetterButtonEnd; i++) {
        lpSKT1Ctxt->ptButtonPos[i].x = xStartPos;
        lpSKT1Ctxt->ptButtonPos[i].y = yStartPos;
        xStartPos += (lpSKT1Ctxt->nButtonWidth[SKT1_LETTER_TYPE] + SKT1_XIN);
    }

    // init fourth row
    xStartPos = gptRaiseEdge.x + SKT1_XOUT + (SKT1_XIN + 1) / 2;
    yStartPos += (nButtonHeightUnit + SKT1_YIN);
    // shift 1
    lpSKT1Ctxt->ptButtonPos[SKT1_SHIFT_INDEX].x = xStartPos;
    lpSKT1Ctxt->ptButtonPos[SKT1_SHIFT_INDEX].y = yStartPos;
    xStartPos += (lpSKT1Ctxt->nButtonWidth[SKT1_SHIFT_TYPE] + SKT1_XIN);
    // letter
    nLetterButtonStart = nLetterButtonEnd;
    nLetterButtonEnd = nLetterButtonStart + SKT1_ROW4_LETTER_NUM;
    for (i = nLetterButtonStart; i < nLetterButtonEnd; i++) {
        lpSKT1Ctxt->ptButtonPos[i].x = xStartPos;
        lpSKT1Ctxt->ptButtonPos[i].y = yStartPos;
        xStartPos += (lpSKT1Ctxt->nButtonWidth[SKT1_LETTER_TYPE] + SKT1_XIN);
    }
    // shift 2
    lpSKT1Ctxt->ptButtonPos[SKT1_SHIFT_INDEX + 1].x = xStartPos;
    lpSKT1Ctxt->ptButtonPos[SKT1_SHIFT_INDEX + 1].y = yStartPos;

    // init fifth row
    xStartPos = gptRaiseEdge.x + SKT1_XOUT + (SKT1_XIN + 1) / 2;
    yStartPos += (nButtonHeightUnit + SKT1_YIN);
    // ctrl 1
    lpSKT1Ctxt->ptButtonPos[SKT1_CTRL_INDEX].x = xStartPos;
    lpSKT1Ctxt->ptButtonPos[SKT1_CTRL_INDEX].y = yStartPos;
    xStartPos += (lpSKT1Ctxt->nButtonWidth[SKT1_CTRL_TYPE] + SKT1_XIN);
    // esc
    lpSKT1Ctxt->ptButtonPos[SKT1_ESC_INDEX].x = xStartPos;
    lpSKT1Ctxt->ptButtonPos[SKT1_ESC_INDEX].y = yStartPos;
    xStartPos += (lpSKT1Ctxt->nButtonWidth[SKT1_ESC_TYPE] + SKT1_XIN);
    // alt 1
    lpSKT1Ctxt->ptButtonPos[SKT1_ALT_INDEX].x = xStartPos;
    lpSKT1Ctxt->ptButtonPos[SKT1_ALT_INDEX].y = yStartPos;
    xStartPos += (lpSKT1Ctxt->nButtonWidth[SKT1_ALT_TYPE] + SKT1_XIN);
    // space
    lpSKT1Ctxt->ptButtonPos[SKT1_SPACE_INDEX].x = xStartPos;
    lpSKT1Ctxt->ptButtonPos[SKT1_SPACE_INDEX].y = yStartPos;
    xStartPos += (lpSKT1Ctxt->nButtonWidth[SKT1_SPACE_TYPE] + SKT1_XIN);
    // alt 2 .. skip blank
    lpSKT1Ctxt->ptButtonPos[SKT1_ALT_INDEX + 1].x = xStartPos;
    lpSKT1Ctxt->ptButtonPos[SKT1_ALT_INDEX + 1].y = yStartPos;
    xStartPos += (lpSKT1Ctxt->nButtonWidth[SKT1_ALT_TYPE] + SKT1_XIN) +
        (lpSKT1Ctxt->nButtonWidth[SKT1_ESC_TYPE] + SKT1_XIN);
    // ctrl 2
    lpSKT1Ctxt->ptButtonPos[SKT1_CTRL_INDEX + 1].x = xStartPos;
    lpSKT1Ctxt->ptButtonPos[SKT1_CTRL_INDEX + 1].y = yStartPos;

    return;
}

/**********************************************************************/
/* SKT1DrawConvexRect                                                 */
/*     draw a convex rectangle                                        */
/*              (x,y)     x+nWidth+1                                  */
/*               +-----1------+                                       */
/*               |+----2-----||                                       */
/*               ||          ||                                       */
/*               ||          3|                                       */
/*               ||          ||                                       */
/*               |V          || <---- Rectangle                       */
/*               |<----------+|                                       */
/*  y+nHeight+1  +------------+                                       */
/*  1 - black                                                         */
/*  2 - while                                                         */
/*  3 - dark gray                                                     */
/**********************************************************************/
void SKT1DrawConvexRect(
    HDC hDC,
    int x,
    int y,
    int nWidth,
    int nHeight)
{
    // extend the context rect size
    x -= (SKT1_XIN + 1) / 2;
    y -= (SKT1_YIN + 1) / 2;
    nWidth += (SKT1_XIN + 1);
    nHeight += (SKT1_XIN + 1);

    // 1
    SelectObject(hDC, GetStockObject(BLACK_PEN));
    SelectObject(hDC, GetStockObject(LTGRAY_BRUSH));
    Rectangle(hDC, x, y, x + nWidth, y + nHeight);
    PatBlt(hDC, x, y, 1, 1, PATCOPY);
    PatBlt(hDC, x + nWidth, y, -1, 1, PATCOPY);
    PatBlt(hDC, x, y + nHeight, 1, -1, PATCOPY);
    PatBlt(hDC, x + nWidth, y + nHeight, -1, -1, PATCOPY);

    x++;
    y++;
    nWidth -= 2;
    nHeight -= 2;

    // 2
    PatBlt(hDC, x, y + nHeight, 1, -nHeight, WHITENESS);
    PatBlt(hDC, x, y, nWidth , 1, WHITENESS);
    // 3
    SelectObject(hDC, GetStockObject(GRAY_BRUSH));
    PatBlt(hDC, x, y + nHeight, nWidth, -1, PATCOPY);
    PatBlt(hDC, x + nWidth, y + nHeight, -1, -nHeight, PATCOPY);

    return;
}

/**********************************************************************/
/* SKT1DrawBitmap                                                     */
/**********************************************************************/
void SKT1DrawBitmap(
    HDC hDC,
    int x,
    int y,
    int nWidth,
    int nHeight,
    LPCWSTR lpszBitmap)
{
    HDC     hMemDC;
    HBITMAP hBitmap, hOldBmp;

    hBitmap = (HBITMAP)LoadBitmap(ghInst, lpszBitmap);

    hMemDC = CreateCompatibleDC(hDC);

    hOldBmp = (HBITMAP)SelectObject(hMemDC, hBitmap);

    BitBlt(hDC, x, y, nWidth, nHeight, hMemDC, 0 , 0, SRCCOPY);

    SelectObject(hMemDC, hOldBmp);

    DeleteObject(hBitmap);

    DeleteDC(hMemDC);

    return;
}

/**********************************************************************/
/* SKT1DrawLabel                                                      */
/**********************************************************************/
void SKT1DrawLabel(
    HDC        hDC,
    LPSKT1CTXT lpSKT1Ctxt,
    LPCWSTR     lpszLabel)
{
    HDC     hMemDC;
    HBITMAP hBitmap, hOldBmp;
    int     i;

    hBitmap = LoadBitmap(ghInst, lpszLabel);
    hMemDC = CreateCompatibleDC(hDC);
    hOldBmp = (HBITMAP)SelectObject(hMemDC, hBitmap);

    for (i = 0; i < SKT1_LETTER_KEY_NUM; i++) {
        BitBlt(hDC,
            lpSKT1Ctxt->ptButtonPos[i].x, lpSKT1Ctxt->ptButtonPos[i].y,
            SKT1_LABEL_BMP_X, SKT1_LABEL_BMP_Y,
            hMemDC, i * SKT1_LABEL_BMP_X, 0, SRCCOPY);
    }

    SelectObject(hMemDC, hOldBmp);
    DeleteDC(hMemDC);
    DeleteObject(hBitmap);

    return;
}


/**********************************************************************/
/* InitSKT1Bitmap                                                     */
/**********************************************************************/
void InitSKT1Bitmap(
    HWND hSKWnd,
    LPSKT1CTXT lpSKT1Ctxt)
{
    HDC  hDC, hMemDC;
    RECT rcClient;
    int  i;

    hDC = GetDC(hSKWnd);
    hMemDC = CreateCompatibleDC(hDC);
    GetClientRect(hSKWnd, &rcClient);
    lpSKT1Ctxt->hSKBitmap = CreateCompatibleBitmap(hDC,
        rcClient.right - rcClient.left,
        rcClient.bottom - rcClient.top);
    ReleaseDC(hSKWnd, hDC);
    SelectObject(hMemDC, lpSKT1Ctxt->hSKBitmap);

    // draw SK rectangle
    SelectObject(hMemDC, GetStockObject(NULL_PEN));
    SelectObject(hMemDC, GetStockObject(LTGRAY_BRUSH));
    Rectangle(hMemDC, rcClient.left, rcClient.top,
        rcClient.right + 1, rcClient.bottom + 1);

    DrawEdge(hMemDC, &rcClient, BDR_RAISED, BF_RECT);

    // draw the button from the last line
    // ctrl 1   -> line 5 special key
    SKT1DrawConvexRect(hMemDC,
        lpSKT1Ctxt->ptButtonPos[SKT1_CTRL_INDEX].x,
        lpSKT1Ctxt->ptButtonPos[SKT1_CTRL_INDEX].y,
        lpSKT1Ctxt->nButtonWidth[SKT1_CTRL_TYPE],
        lpSKT1Ctxt->nButtonHeight[0]);
    SKT1DrawBitmap(hMemDC,
        lpSKT1Ctxt->ptButtonPos[SKT1_CTRL_INDEX].x +
        lpSKT1Ctxt->nButtonWidth[SKT1_CTRL_TYPE] / 2 - SKT1_CTRL_BMP_X / 2,
        lpSKT1Ctxt->ptButtonPos[SKT1_CTRL_INDEX].y +
        lpSKT1Ctxt->nButtonHeight[0] / 2 - SKT1_CTRL_BMP_Y / 2,
        SKT1_CTRL_BMP_X,
        SKT1_CTRL_BMP_Y,
        MAKEINTRESOURCEW(CTRL_T1));

    // ctrl 2
    SKT1DrawConvexRect(hMemDC,
        lpSKT1Ctxt->ptButtonPos[SKT1_CTRL_INDEX + 1].x,
        lpSKT1Ctxt->ptButtonPos[SKT1_CTRL_INDEX + 1].y,
        lpSKT1Ctxt->nButtonWidth[SKT1_CTRL_TYPE],
        lpSKT1Ctxt->nButtonHeight[0]);
    SKT1DrawBitmap(hMemDC,
        lpSKT1Ctxt->ptButtonPos[SKT1_CTRL_INDEX + 1].x +
        lpSKT1Ctxt->nButtonWidth[SKT1_CTRL_TYPE] / 2 - SKT1_CTRL_BMP_X / 2,
        lpSKT1Ctxt->ptButtonPos[SKT1_CTRL_INDEX + 1].y +
        lpSKT1Ctxt->nButtonHeight[0] / 2 - SKT1_CTRL_BMP_Y / 2,
        SKT1_CTRL_BMP_X, SKT1_CTRL_BMP_Y,
        MAKEINTRESOURCEW(CTRL_T1));

    // esc
    SKT1DrawConvexRect(hMemDC,
        lpSKT1Ctxt->ptButtonPos[SKT1_ESC_INDEX].x,
        lpSKT1Ctxt->ptButtonPos[SKT1_ESC_INDEX].y,
        lpSKT1Ctxt->nButtonWidth[SKT1_ESC_TYPE],
        lpSKT1Ctxt->nButtonHeight[0]);
    SKT1DrawBitmap(hMemDC,
        lpSKT1Ctxt->ptButtonPos[SKT1_ESC_INDEX].x +
        lpSKT1Ctxt->nButtonWidth[SKT1_ESC_TYPE] / 2 - SKT1_ESC_BMP_X / 2,
        lpSKT1Ctxt->ptButtonPos[SKT1_ESC_INDEX].y +
        lpSKT1Ctxt->nButtonHeight[0] / 2 - SKT1_ESC_BMP_Y / 2,
        SKT1_ESC_BMP_X, SKT1_ESC_BMP_Y,
        MAKEINTRESOURCEW(ESC_T1));

    // alt 1
    SKT1DrawConvexRect(hMemDC,
        lpSKT1Ctxt->ptButtonPos[SKT1_ALT_INDEX].x,
        lpSKT1Ctxt->ptButtonPos[SKT1_ALT_INDEX].y,
        lpSKT1Ctxt->nButtonWidth[SKT1_ALT_TYPE],
        lpSKT1Ctxt->nButtonHeight[0]);
    SKT1DrawBitmap(hMemDC,
        lpSKT1Ctxt->ptButtonPos[SKT1_ALT_INDEX].x +
        lpSKT1Ctxt->nButtonWidth[SKT1_ALT_TYPE] / 2 - SKT1_ALT_BMP_X / 2,
        lpSKT1Ctxt->ptButtonPos[SKT1_ALT_INDEX].y +
        lpSKT1Ctxt->nButtonHeight[0] / 2 - SKT1_ALT_BMP_Y / 2,
        SKT1_ALT_BMP_X, SKT1_ALT_BMP_Y,
        MAKEINTRESOURCEW(ALT_T1));

    // alt 2
    SKT1DrawConvexRect(hMemDC,
        lpSKT1Ctxt->ptButtonPos[SKT1_ALT_INDEX + 1].x,
        lpSKT1Ctxt->ptButtonPos[SKT1_ALT_INDEX + 1].y,
        lpSKT1Ctxt->nButtonWidth[SKT1_ALT_TYPE],
        lpSKT1Ctxt->nButtonHeight[0]);
    SKT1DrawBitmap(hMemDC,
        lpSKT1Ctxt->ptButtonPos[SKT1_ALT_INDEX + 1].x +
        lpSKT1Ctxt->nButtonWidth[SKT1_ALT_TYPE] / 2 - SKT1_ALT_BMP_X / 2,
        lpSKT1Ctxt->ptButtonPos[SKT1_ALT_INDEX + 1].y +
        lpSKT1Ctxt->nButtonHeight[0] / 2 - SKT1_ALT_BMP_Y / 2,
        SKT1_ALT_BMP_X, SKT1_ALT_BMP_Y,
        MAKEINTRESOURCEW(ALT_T1));

    // space
    SKT1DrawConvexRect(hMemDC,
        lpSKT1Ctxt->ptButtonPos[SKT1_SPACE_INDEX].x,
        lpSKT1Ctxt->ptButtonPos[SKT1_SPACE_INDEX].y,
        lpSKT1Ctxt->nButtonWidth[SKT1_SPACE_TYPE],
        lpSKT1Ctxt->nButtonHeight[0]);

    // line 4
    // shift 1
    SKT1DrawConvexRect(hMemDC,
        lpSKT1Ctxt->ptButtonPos[SKT1_SHIFT_INDEX].x,
        lpSKT1Ctxt->ptButtonPos[SKT1_SHIFT_INDEX].y,
        lpSKT1Ctxt->nButtonWidth[SKT1_SHIFT_TYPE],
        lpSKT1Ctxt->nButtonHeight[0]);
    SKT1DrawBitmap(hMemDC,
        lpSKT1Ctxt->ptButtonPos[SKT1_SHIFT_INDEX].x +
        lpSKT1Ctxt->nButtonWidth[SKT1_SHIFT_TYPE] / 2 - SKT1_SHIFT_BMP_X / 2,
        lpSKT1Ctxt->ptButtonPos[SKT1_SHIFT_INDEX].y +
        lpSKT1Ctxt->nButtonHeight[0] / 2 - SKT1_SHIFT_BMP_Y / 2,
        SKT1_SHIFT_BMP_X, SKT1_SHIFT_BMP_Y,
        MAKEINTRESOURCEW(SHIFT_T1));

    // shift 2
    SKT1DrawConvexRect(hMemDC,
        lpSKT1Ctxt->ptButtonPos[SKT1_SHIFT_INDEX + 1].x,
        lpSKT1Ctxt->ptButtonPos[SKT1_SHIFT_INDEX + 1].y,
        lpSKT1Ctxt->nButtonWidth[SKT1_SHIFT_TYPE],
        lpSKT1Ctxt->nButtonHeight[0]);
    SKT1DrawBitmap(hMemDC,
        lpSKT1Ctxt->ptButtonPos[SKT1_SHIFT_INDEX + 1].x +
        lpSKT1Ctxt->nButtonWidth[SKT1_SHIFT_TYPE] / 2 - SKT1_SHIFT_BMP_X / 2,
        lpSKT1Ctxt->ptButtonPos[SKT1_SHIFT_INDEX + 1].y +
        lpSKT1Ctxt->nButtonHeight[0] / 2 - SKT1_SHIFT_BMP_Y / 2,
        SKT1_SHIFT_BMP_X, SKT1_SHIFT_BMP_Y,
        MAKEINTRESOURCEW(SHIFT_T1));

    // line 3
    // caps
    SKT1DrawConvexRect(hMemDC,
        lpSKT1Ctxt->ptButtonPos[SKT1_CAPS_INDEX].x,
        lpSKT1Ctxt->ptButtonPos[SKT1_CAPS_INDEX].y,
        lpSKT1Ctxt->nButtonWidth[SKT1_CAPS_TYPE],
        lpSKT1Ctxt->nButtonHeight[0]);
    SKT1DrawBitmap(hMemDC,
        lpSKT1Ctxt->ptButtonPos[SKT1_CAPS_INDEX].x +
        lpSKT1Ctxt->nButtonWidth[SKT1_CAPS_TYPE] / 2 - SKT1_CAPS_BMP_X / 2,
        lpSKT1Ctxt->ptButtonPos[SKT1_CAPS_INDEX].y +
        lpSKT1Ctxt->nButtonHeight[0] / 2 - SKT1_CAPS_BMP_Y / 2,
        SKT1_CAPS_BMP_X, SKT1_CAPS_BMP_Y,
        MAKEINTRESOURCEW(CAPS_T1));

    // line 2
    // tab
    SKT1DrawConvexRect(hMemDC,
        lpSKT1Ctxt->ptButtonPos[SKT1_TAB_INDEX].x,
        lpSKT1Ctxt->ptButtonPos[SKT1_TAB_INDEX].y,
        lpSKT1Ctxt->nButtonWidth[SKT1_TAB_TYPE],
        lpSKT1Ctxt->nButtonHeight[0]);
    SKT1DrawBitmap(hMemDC,
        lpSKT1Ctxt->ptButtonPos[SKT1_TAB_INDEX].x +
        lpSKT1Ctxt->nButtonWidth[SKT1_TAB_TYPE] / 2 - SKT1_TAB_BMP_X / 2,
        lpSKT1Ctxt->ptButtonPos[SKT1_TAB_INDEX].y +
        lpSKT1Ctxt->nButtonHeight[0] / 2 - SKT1_TAB_BMP_Y / 2,
        SKT1_TAB_BMP_X, SKT1_TAB_BMP_Y,
        MAKEINTRESOURCEW(TAB_T1));

    // enter
    SKT1DrawConvexRect(hMemDC,
        lpSKT1Ctxt->ptButtonPos[SKT1_ENTER_INDEX].x,
        lpSKT1Ctxt->ptButtonPos[SKT1_ENTER_INDEX].y,
        lpSKT1Ctxt->nButtonWidth[SKT1_ENTER_TYPE],
        lpSKT1Ctxt->nButtonHeight[1]);
    SKT1DrawBitmap(hMemDC,
        lpSKT1Ctxt->ptButtonPos[SKT1_ENTER_INDEX].x +
        lpSKT1Ctxt->nButtonWidth[SKT1_ENTER_TYPE] / 2 - SKT1_ENTER_BMP_X / 2,
        lpSKT1Ctxt->ptButtonPos[SKT1_ENTER_INDEX].y +
        lpSKT1Ctxt->nButtonHeight[1] / 2 - SKT1_ENTER_BMP_Y / 2,
        SKT1_ENTER_BMP_X, SKT1_ENTER_BMP_Y,
        MAKEINTRESOURCEW(ENTER_T1));

    // line 1
    // backspace
    SKT1DrawConvexRect(hMemDC,
        lpSKT1Ctxt->ptButtonPos[SKT1_BACKSPACE_INDEX].x,
        lpSKT1Ctxt->ptButtonPos[SKT1_BACKSPACE_INDEX].y,
        lpSKT1Ctxt->nButtonWidth[SKT1_BACKSPACE_TYPE],
        lpSKT1Ctxt->nButtonHeight[0]);
    SKT1DrawBitmap(hMemDC,
        lpSKT1Ctxt->ptButtonPos[SKT1_BACKSPACE_INDEX].x +
        lpSKT1Ctxt->nButtonWidth[SKT1_BACKSPACE_TYPE] / 2 - SKT1_BACKSPACE_BMP_X / 2,
        lpSKT1Ctxt->ptButtonPos[SKT1_BACKSPACE_INDEX].y +
        lpSKT1Ctxt->nButtonHeight[0] / 2 - SKT1_BACKSPACE_BMP_Y / 2,
        SKT1_BACKSPACE_BMP_X, SKT1_BACKSPACE_BMP_Y,
        MAKEINTRESOURCEW(BACK_T1));

    // draw letter buttons
    for (i = SKT1_LETTER_KEY_NUM - 1; i >= 0 ; i--) {
        SKT1DrawConvexRect(hMemDC,
            lpSKT1Ctxt->ptButtonPos[i].x,
            lpSKT1Ctxt->ptButtonPos[i].y,
            lpSKT1Ctxt->nButtonWidth[SKT1_LETTER_TYPE],
            lpSKT1Ctxt->nButtonHeight[0]);
    }

    // draw latter label
    SKT1DrawLabel(hMemDC, lpSKT1Ctxt, MAKEINTRESOURCEW(LABEL_T1));

    DeleteDC(hMemDC);

    return;
}


/**********************************************************************/
/* CreateT1Window                                                     */
/**********************************************************************/
LRESULT CreateT1Window(
    HWND hSKWnd)
{
    HGLOBAL    hSKT1Ctxt;
    LPSKT1CTXT lpSKT1Ctxt;

    hSKT1Ctxt = GlobalAlloc(GHND, sizeof(SKT1CTXT));
    if (!hSKT1Ctxt) {
        return (-1);
    }

    lpSKT1Ctxt = (LPSKT1CTXT)GlobalLock(hSKT1Ctxt);
    if (!lpSKT1Ctxt) {
        GlobalFree(hSKT1Ctxt);
        return (-1);
    }

    SetWindowLongPtr(hSKWnd, SKT1_CONTEXT, (LONG_PTR)hSKT1Ctxt);

    lpSKT1Ctxt->uKeyIndex = SKT1_TOTAL_INDEX;
    lpSKT1Ctxt->ptSkOffset.x = SKT1_NOT_DRAG;
    lpSKT1Ctxt->ptSkOffset.y = SKT1_NOT_DRAG;
    lpSKT1Ctxt->lfCharSet = CHINESEBIG5_CHARSET;

    InitSKT1ButtonPos(lpSKT1Ctxt);

    InitSKT1Bitmap(hSKWnd, lpSKT1Ctxt);

    GlobalUnlock(hSKT1Ctxt);

    return (0L);
}

/**********************************************************************/
/* SKT1DrawDragBorder()                                               */
/**********************************************************************/
void SKT1DrawDragBorder(
    HWND    hWnd,               // window of IME is dragged
    LPPOINT lpptCursor,         // the cursor position
    LPPOINT lpptOffset)         // the offset form cursor to window org
{
    HDC  hDC;
    int  cxBorder, cyBorder;
    int  x, y;
    RECT rcWnd;

    cxBorder = GetSystemMetrics(SM_CXBORDER);   // width of border
    cyBorder = GetSystemMetrics(SM_CYBORDER);   // height of border

    x = lpptCursor->x - lpptOffset->x;
    y = lpptCursor->y - lpptOffset->y;

    // check for the max boundary of the display
    GetWindowRect(hWnd, &rcWnd);

    // draw the moving track
    hDC = CreateDC(L"DISPLAY", NULL, NULL, NULL);
    SelectObject(hDC, GetStockObject(GRAY_BRUSH));

    // ->
    PatBlt(hDC, x, y, rcWnd.right - rcWnd.left - cxBorder, cyBorder,
        PATINVERT);
    // v
    PatBlt(hDC, x, y + cyBorder, cxBorder, rcWnd.bottom - rcWnd.top -
        cyBorder, PATINVERT);
    // _>
    PatBlt(hDC, x + cxBorder, y + rcWnd.bottom - rcWnd.top,
        rcWnd.right - rcWnd.left - cxBorder, -cyBorder, PATINVERT);
    //  v
    PatBlt(hDC, x + rcWnd.right - rcWnd.left, y,
        - cxBorder, rcWnd.bottom - rcWnd.top - cyBorder, PATINVERT);

    DeleteDC(hDC);
    return;
}

/**********************************************************************/
/* DestroyT1Window                                                    */
/**********************************************************************/
void DestroyT1Window(
    HWND hSKWnd)
{
    HGLOBAL    hSKT1Ctxt;
    LPSKT1CTXT lpSKT1Ctxt;
    HWND       hUIWnd;

    hSKT1Ctxt = (HGLOBAL)GetWindowLongPtr(hSKWnd, SKT1_CONTEXT);
    if (!hSKT1Ctxt) {
        return;
    }

    lpSKT1Ctxt = (LPSKT1CTXT)GlobalLock(hSKT1Ctxt);
    if (!lpSKT1Ctxt) {
        return;
    }

    if (lpSKT1Ctxt->ptSkOffset.x != SKT1_NOT_DRAG &&
            lpSKT1Ctxt->ptSkOffset.y != SKT1_NOT_DRAG) {
        SKT1DrawDragBorder(hSKWnd, &lpSKT1Ctxt->ptSkCursor,
            &lpSKT1Ctxt->ptSkOffset);
    }

    DeleteObject(lpSKT1Ctxt->hSKBitmap);

    GlobalUnlock(hSKT1Ctxt);
    GlobalFree(hSKT1Ctxt);

    hUIWnd = GetWindow(hSKWnd, GW_OWNER);
    if (!hUIWnd) {
        return;
    }

    SendMessage(hUIWnd, WM_IME_NOTIFY, IMN_SOFTKBDDESTROYED, 0);

    return;
}

/**********************************************************************/
/* SKT1InvertButton                                                   */
/**********************************************************************/
void SKT1InvertButton(
    HWND       hSKWnd,
    HDC        hPaintDC,
    LPSKT1CTXT lpSKT1Ctxt,
    UINT       uKeyIndex)
{
    HDC hDC;
    int nWidth, nHeight;

    if (uKeyIndex >= SKT1_TOTAL_INDEX) {
        return;
    }

    nWidth = 0;
    if (hPaintDC) {
        hDC = hPaintDC;
    } else {
        hDC = GetDC(hSKWnd);
    }

    if (uKeyIndex < SKT1_LETTER_KEY_NUM) {
        nWidth = lpSKT1Ctxt->nButtonWidth[SKT1_LETTER_TYPE];
    } else {
        switch (uKeyIndex) {
        case SKT1_BACKSPACE_INDEX:
            nWidth = lpSKT1Ctxt->nButtonWidth[SKT1_BACKSPACE_TYPE];
            break;
        case SKT1_TAB_INDEX:
            nWidth = lpSKT1Ctxt->nButtonWidth[SKT1_TAB_TYPE];
            break;
        case SKT1_CAPS_INDEX:
            nWidth = 0;
            MessageBeep((UINT) -1);
            break;
        case SKT1_ENTER_INDEX:
            nWidth = lpSKT1Ctxt->nButtonWidth[SKT1_ENTER_TYPE];
            break;
        case SKT1_SHIFT_INDEX:
        case SKT1_SHIFT_INDEX + 1:
        case SKT1_CTRL_INDEX:
        case SKT1_CTRL_INDEX + 1:
        case SKT1_ALT_INDEX:
        case SKT1_ALT_INDEX + 1:
            nWidth = 0;
            MessageBeep((UINT) -1);
            break;
        case SKT1_ESC_INDEX:
            nWidth = lpSKT1Ctxt->nButtonWidth[SKT1_ESC_TYPE];
            break;
        case SKT1_SPACE_INDEX:
            nWidth = lpSKT1Ctxt->nButtonWidth[SKT1_SPACE_TYPE];
            break;
        default:
            break;
        }
    }

    if (uKeyIndex == SKT1_ENTER_INDEX) {
        nHeight = lpSKT1Ctxt->nButtonHeight[1];
    } else {
        nHeight = lpSKT1Ctxt->nButtonHeight[0];
    }

    if (nWidth) {
        // do not reverse border
        PatBlt(hDC, lpSKT1Ctxt->ptButtonPos[uKeyIndex].x - 1,
            lpSKT1Ctxt->ptButtonPos[uKeyIndex].y - 1,
            nWidth + 2, nHeight + 2, DSTINVERT);
    }

    if (!hPaintDC) {
        ReleaseDC(hSKWnd, hDC);
    }

    return;
}

/**********************************************************************/
/* UpdateSKT1Window                                                   */
/**********************************************************************/
void UpdateSKT1Window(
    HDC  hDC,
    HWND hSKWnd)
{
    HGLOBAL    hSKT1Ctxt;
    LPSKT1CTXT lpSKT1Ctxt;
    HDC        hMemDC;
    RECT       rcClient;

    hSKT1Ctxt = (HGLOBAL)GetWindowLongPtr(hSKWnd, SKT1_CONTEXT);
    if (!hSKT1Ctxt) {
        return;
    }

    lpSKT1Ctxt = (LPSKT1CTXT)GlobalLock(hSKT1Ctxt);
    if (!lpSKT1Ctxt) {
        return;
    }

    hMemDC = CreateCompatibleDC(hDC);

    SelectObject(hMemDC, lpSKT1Ctxt->hSKBitmap);

    GetClientRect(hSKWnd, &rcClient);

    BitBlt(hDC, 0, 0, rcClient.right - rcClient.left,
        rcClient.bottom - rcClient.top,
        hMemDC, 0, 0, SRCCOPY);

    DeleteDC(hMemDC);

    if (lpSKT1Ctxt->uKeyIndex < SKT1_TOTAL_INDEX) {
        SKT1InvertButton(hSKWnd, hDC, lpSKT1Ctxt, lpSKT1Ctxt->uKeyIndex);
    }

    GlobalUnlock(hSKT1Ctxt);

    return;
}

/**********************************************************************/
/* SKT1MousePosition()                                                */
/**********************************************************************/
UINT SKT1MousePosition(
    LPSKT1CTXT lpSKT1Ctxt,
    LPPOINT    lpptCursor)
{
    int   i;

    // check letter button
    for (i = 0; i < SKT1_LETTER_KEY_NUM; i++) {
        if (ImmPtInRect(lpSKT1Ctxt->ptButtonPos[i].x,
                lpSKT1Ctxt->ptButtonPos[i].y,
                lpSKT1Ctxt->nButtonWidth[SKT1_LETTER_TYPE],
                lpSKT1Ctxt->nButtonHeight[0],
                lpptCursor)) {
            return i;
        }
    }

    // backSapce
    if (ImmPtInRect(lpSKT1Ctxt->ptButtonPos[SKT1_BACKSPACE_INDEX].x,
            lpSKT1Ctxt->ptButtonPos[SKT1_BACKSPACE_INDEX].y,
            lpSKT1Ctxt->nButtonWidth[SKT1_BACKSPACE_TYPE],
            lpSKT1Ctxt->nButtonHeight[0],
            lpptCursor)) {
        return SKT1_BACKSPACE_INDEX;
    }

    // tab
    if (ImmPtInRect(lpSKT1Ctxt->ptButtonPos[SKT1_TAB_INDEX].x,
            lpSKT1Ctxt->ptButtonPos[SKT1_TAB_INDEX].y,
            lpSKT1Ctxt->nButtonWidth[SKT1_TAB_TYPE],
            lpSKT1Ctxt->nButtonHeight[0],
            lpptCursor)) {
        return SKT1_TAB_INDEX;
    }

    // caps
    if (ImmPtInRect(lpSKT1Ctxt->ptButtonPos[SKT1_CAPS_INDEX].x,
            lpSKT1Ctxt->ptButtonPos[SKT1_CAPS_INDEX].y,
            lpSKT1Ctxt->nButtonWidth[SKT1_CAPS_TYPE],
            lpSKT1Ctxt->nButtonHeight[0],
            lpptCursor)) {
        return SKT1_CAPS_INDEX;
    }

    // enter
    if (ImmPtInRect(lpSKT1Ctxt->ptButtonPos[SKT1_ENTER_INDEX].x,
            lpSKT1Ctxt->ptButtonPos[SKT1_ENTER_INDEX].y,
            lpSKT1Ctxt->nButtonWidth[SKT1_ENTER_TYPE],
            lpSKT1Ctxt->nButtonHeight[1],
            lpptCursor)) {
        return SKT1_ENTER_INDEX;
    }

    // shift 1
    if (ImmPtInRect(lpSKT1Ctxt->ptButtonPos[SKT1_SHIFT_INDEX].x,
            lpSKT1Ctxt->ptButtonPos[SKT1_SHIFT_INDEX].y,
            lpSKT1Ctxt->nButtonWidth[SKT1_SHIFT_TYPE],
            lpSKT1Ctxt->nButtonHeight[0],
            lpptCursor)) {
        return SKT1_SHIFT_INDEX;
    }

    // shift 2
    if (ImmPtInRect(lpSKT1Ctxt->ptButtonPos[SKT1_SHIFT_INDEX + 1].x,
            lpSKT1Ctxt->ptButtonPos[SKT1_SHIFT_INDEX + 1].y,
            lpSKT1Ctxt->nButtonWidth[SKT1_SHIFT_TYPE],
            lpSKT1Ctxt->nButtonHeight[0],
            lpptCursor)) {
        return SKT1_SHIFT_INDEX;
    }

    // ctrl 1
    if (ImmPtInRect(lpSKT1Ctxt->ptButtonPos[SKT1_CTRL_INDEX].x,
            lpSKT1Ctxt->ptButtonPos[SKT1_CTRL_INDEX].y,
            lpSKT1Ctxt->nButtonWidth[SKT1_CTRL_TYPE],
            lpSKT1Ctxt->nButtonHeight[0],
            lpptCursor)) {
        return SKT1_CTRL_INDEX;
    }

    // ctrl 2
    if (ImmPtInRect(lpSKT1Ctxt->ptButtonPos[SKT1_CTRL_INDEX + 1].x,
            lpSKT1Ctxt->ptButtonPos[SKT1_CTRL_INDEX + 1].y,
            lpSKT1Ctxt->nButtonWidth[SKT1_CTRL_TYPE],
            lpSKT1Ctxt->nButtonHeight[0],
            lpptCursor)) {
        return SKT1_CTRL_INDEX;
    }

    // alt 1
    if (ImmPtInRect(lpSKT1Ctxt->ptButtonPos[SKT1_ALT_INDEX].x,
            lpSKT1Ctxt->ptButtonPos[SKT1_ALT_INDEX].y,
            lpSKT1Ctxt->nButtonWidth[SKT1_ALT_TYPE],
            lpSKT1Ctxt->nButtonHeight[0],
            lpptCursor)) {
        return SKT1_ALT_INDEX;
    }

    // alt 2
    if (ImmPtInRect(lpSKT1Ctxt->ptButtonPos[SKT1_ALT_INDEX + 1].x,
            lpSKT1Ctxt->ptButtonPos[SKT1_ALT_INDEX + 1].y,
            lpSKT1Ctxt->nButtonWidth[SKT1_ALT_TYPE],
            lpSKT1Ctxt->nButtonHeight[0],
            lpptCursor)) {
        return SKT1_ALT_INDEX;
    }

    // esc
    if (ImmPtInRect(lpSKT1Ctxt->ptButtonPos[SKT1_ESC_INDEX].x,
            lpSKT1Ctxt->ptButtonPos[SKT1_ESC_INDEX].y,
            lpSKT1Ctxt->nButtonWidth[SKT1_ESC_TYPE],
            lpSKT1Ctxt->nButtonHeight[0],
            lpptCursor)) {
        return SKT1_ESC_INDEX;
    }

    // space
    if (ImmPtInRect(lpSKT1Ctxt->ptButtonPos[SKT1_SPACE_INDEX].x,
            lpSKT1Ctxt->ptButtonPos[SKT1_SPACE_INDEX].y,
            lpSKT1Ctxt->nButtonWidth[SKT1_SPACE_TYPE],
            lpSKT1Ctxt->nButtonHeight[0],
            lpptCursor)) {
        return SKT1_SPACE_INDEX;
    }

    return SKT1_TOTAL_INDEX;      // This means out of range
}

/**********************************************************************/
/* SKT1IsValidButton                                                  */
/**********************************************************************/
BOOL SKT1IsValidButton(
    UINT       uKeyIndex,
    LPSKT1CTXT lpSKT1Ctxt)
{
    BOOL fRet;

    if (uKeyIndex < SKT1_LETTER_KEY_NUM) {
        if (lpSKT1Ctxt->wCodeTable[uKeyIndex]) {
            return TRUE;
        } else {
            return FALSE;
        }
    }

    // special key
    switch (uKeyIndex) {
    case SKT1_BACKSPACE_INDEX:
    case SKT1_TAB_INDEX:
    case SKT1_ENTER_INDEX:
    case SKT1_ESC_INDEX:
    case SKT1_SPACE_INDEX:
        fRet = TRUE;
        break;
    default:
        fRet = FALSE;
        break;
    }

    return fRet;
}

/**********************************************************************/
/* SKT1SetCursor                                                      */
/**********************************************************************/
BOOL SKT1SetCursor(
    HWND   hSKWnd,
    LPARAM lParam)
{
    HGLOBAL    hSKT1Ctxt;
    LPSKT1CTXT lpSKT1Ctxt;
    UINT       uKeyIndex;
    RECT       rcWnd;
    RECT       rcWork;
    SIZE       szWork;
    extern     void GetAllMonitorSize(LPRECT lprc);

    hSKT1Ctxt = (HGLOBAL)GetWindowLongPtr(hSKWnd, SKT1_CONTEXT);
    if (!hSKT1Ctxt) {
        return (FALSE);
    }

    lpSKT1Ctxt = (LPSKT1CTXT)GlobalLock(hSKT1Ctxt);
    if (!lpSKT1Ctxt) {
        return (FALSE);
    }

    if (lpSKT1Ctxt->ptSkOffset.x != SKT1_NOT_DRAG &&
            lpSKT1Ctxt->ptSkOffset.y != SKT1_NOT_DRAG) {
        // in drag operation
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        goto UnlockSKT1Ctxt;
    }

    GetCursorPos(&lpSKT1Ctxt->ptSkCursor);
    ScreenToClient(hSKWnd, &lpSKT1Ctxt->ptSkCursor);

    uKeyIndex = SKT1MousePosition(lpSKT1Ctxt, &lpSKT1Ctxt->ptSkCursor);

    if (uKeyIndex < SKT1_TOTAL_INDEX) {
        SetCursor(LoadCursor(NULL, IDC_HAND));
    } else {
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));
    }


    if (HIWORD(lParam) != WM_LBUTTONDOWN) {
        goto UnlockSKT1Ctxt;
    }

    SetCapture(hSKWnd);

    if (lpSKT1Ctxt->uKeyIndex < SKT1_TOTAL_INDEX) {
        UINT uVirtKey;

        uVirtKey = bSKT1VirtKey[lpSKT1Ctxt->uKeyIndex];
        keybd_event((BYTE)uVirtKey, (BYTE)guScanCode[uVirtKey],
            (DWORD)KEYEVENTF_KEYUP, 0);
        SKT1InvertButton(hSKWnd, NULL, lpSKT1Ctxt, lpSKT1Ctxt->uKeyIndex);
        lpSKT1Ctxt->uKeyIndex = SKT1_TOTAL_INDEX;
    }

    if (uKeyIndex < SKT1_TOTAL_INDEX) {
        UINT uVirtKey;

        if (SKT1IsValidButton(uKeyIndex, lpSKT1Ctxt)) {
            uVirtKey = bSKT1VirtKey[uKeyIndex];
            keybd_event((BYTE)uVirtKey, (BYTE)guScanCode[uVirtKey],
                0, 0);
            lpSKT1Ctxt->uKeyIndex = uKeyIndex;
            SKT1InvertButton(hSKWnd, NULL, lpSKT1Ctxt, lpSKT1Ctxt->uKeyIndex);
        } else {
            MessageBeep((UINT)-1);
        }
        goto UnlockSKT1Ctxt;
    }

    GetAllMonitorSize(&rcWork);
    szWork.cx = rcWork.right - rcWork.left;
    szWork.cy = rcWork.bottom - rcWork.top;

    GetCursorPos(&lpSKT1Ctxt->ptSkCursor);
    GetWindowRect(hSKWnd, &rcWnd);
    lpSKT1Ctxt->ptSkOffset.x = lpSKT1Ctxt->ptSkCursor.x - rcWnd.left;
    lpSKT1Ctxt->ptSkOffset.y = lpSKT1Ctxt->ptSkCursor.y - rcWnd.top;

    SKT1DrawDragBorder(hSKWnd, &lpSKT1Ctxt->ptSkCursor,
        &lpSKT1Ctxt->ptSkOffset);

UnlockSKT1Ctxt:
    GlobalUnlock(hSKT1Ctxt);

    return (TRUE);
}

/**********************************************************************/
/* SKT1MouseMove                                                      */
/**********************************************************************/
BOOL SKT1MouseMove(
    HWND hSKWnd)
{
    HGLOBAL    hSKT1Ctxt;
    LPSKT1CTXT lpSKT1Ctxt;

    hSKT1Ctxt = (HGLOBAL)GetWindowLongPtr(hSKWnd, SKT1_CONTEXT);
    if (!hSKT1Ctxt) {
        return FALSE;
    }

    lpSKT1Ctxt = (LPSKT1CTXT)GlobalLock(hSKT1Ctxt);
    if (!lpSKT1Ctxt) {
        return FALSE;
    }

    if (lpSKT1Ctxt->ptSkOffset.x == SKT1_NOT_DRAG ||
            lpSKT1Ctxt->ptSkOffset.y == SKT1_NOT_DRAG) {
        GlobalUnlock(hSKT1Ctxt);
        return FALSE;
    }

    SKT1DrawDragBorder(hSKWnd, &lpSKT1Ctxt->ptSkCursor,
        &lpSKT1Ctxt->ptSkOffset);

    GetCursorPos(&lpSKT1Ctxt->ptSkCursor);

    SKT1DrawDragBorder(hSKWnd, &lpSKT1Ctxt->ptSkCursor,
        &lpSKT1Ctxt->ptSkOffset);

    GlobalUnlock(hSKT1Ctxt);

    return TRUE;
}

/**********************************************************************/
/* SKT1ButtonUp                                                       */
/**********************************************************************/
BOOL SKT1ButtonUp(
    HWND hSKWnd)
{
    HGLOBAL        hSKT1Ctxt;
    LPSKT1CTXT     lpSKT1Ctxt;
    BOOL           fRet;
    POINT          pt;
    HWND           hUIWnd;
    HIMC           hIMC;
    LPINPUTCONTEXT lpIMC;

    fRet = FALSE;

    if (IsWndEqual(GetCapture(), hSKWnd)) {
        ReleaseCapture();
    }

    hSKT1Ctxt = (HGLOBAL)GetWindowLongPtr(hSKWnd, SKT1_CONTEXT);
    if (!hSKT1Ctxt) {
        return (fRet);
    }

    lpSKT1Ctxt = (LPSKT1CTXT)GlobalLock(hSKT1Ctxt);
    if (!lpSKT1Ctxt) {
        return (fRet);
    }

    if (lpSKT1Ctxt->uKeyIndex < SKT1_TOTAL_INDEX) {
        UINT uVirtKey;

        uVirtKey = bSKT1VirtKey[lpSKT1Ctxt->uKeyIndex];
        keybd_event((BYTE)uVirtKey, (BYTE)guScanCode[uVirtKey],
            (DWORD)KEYEVENTF_KEYUP, 0);
        SKT1InvertButton(hSKWnd, NULL, lpSKT1Ctxt, lpSKT1Ctxt->uKeyIndex);
        lpSKT1Ctxt->uKeyIndex = SKT1_TOTAL_INDEX;
        fRet = TRUE;
        goto UnlockSKT1Context;
    }

    if (lpSKT1Ctxt->ptSkOffset.x == SKT1_NOT_DRAG ||
            lpSKT1Ctxt->ptSkOffset.y == SKT1_NOT_DRAG) {
        goto UnlockSKT1Context;
    }

    SKT1DrawDragBorder(hSKWnd, &lpSKT1Ctxt->ptSkCursor,
        &lpSKT1Ctxt->ptSkOffset);

    pt.x = lpSKT1Ctxt->ptSkCursor.x - lpSKT1Ctxt->ptSkOffset.x,
    pt.y = lpSKT1Ctxt->ptSkCursor.y - lpSKT1Ctxt->ptSkOffset.y,

    SetWindowPos(hSKWnd, (HWND)NULL, pt.x, pt.y,
        0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);

    lpSKT1Ctxt->ptSkOffset.x = SKT1_NOT_DRAG;
    lpSKT1Ctxt->ptSkOffset.y = SKT1_NOT_DRAG;

    lpSKT1Ctxt->uKeyIndex = SKT1_TOTAL_INDEX;

    fRet = TRUE;

    hUIWnd = GetWindow(hSKWnd, GW_OWNER);

    hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
    if (!hIMC) {
        goto UnlockSKT1Context;
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        goto UnlockSKT1Context;
    }

    lpIMC->ptSoftKbdPos.x = pt.x;
    lpIMC->ptSoftKbdPos.y = pt.y;
    lpIMC->fdwInit |= INIT_SOFTKBDPOS;

    ImmUnlockIMC(hIMC);

UnlockSKT1Context:
    GlobalUnlock(hSKT1Ctxt);

    return (fRet);
}

/**********************************************************************/
/* SetSKT1Data                                                        */
/**********************************************************************/
LRESULT SetSKT1Data(
    HWND          hSKWnd,
    LPSOFTKBDDATA lpSoftKbdData)
{
    HGLOBAL    hSKT1Ctxt;
    LPSKT1CTXT lpSKT1Ctxt;
    HDC        hDC, hMemDC;
    HGDIOBJ    hOldFont;
    int        i;

    hSKT1Ctxt = (HGLOBAL)GetWindowLongPtr(hSKWnd, SKT1_CONTEXT);
    if (!hSKT1Ctxt) {
        return (1);
    }

    lpSKT1Ctxt = (LPSKT1CTXT)GlobalLock(hSKT1Ctxt);
    if (!lpSKT1Ctxt) {
        return (1);
    }

    hDC = GetDC(hSKWnd);
    hMemDC = CreateCompatibleDC(hDC);
    ReleaseDC(hSKWnd, hDC);
    SelectObject(hMemDC, lpSKT1Ctxt->hSKBitmap);

    SetBkColor(hMemDC, RGB(0xC0, 0xC0, 0xC0));

    if (lpSKT1Ctxt->lfCharSet != DEFAULT_CHARSET) {
        LOGFONT lfFont;
        CopyMemory(&lfFont, &lfSKT1Font, sizeof (LOGFONT));
        lfFont.lfCharSet = (BYTE)lpSKT1Ctxt->lfCharSet;
        hOldFont = (HFONT)SelectObject(hMemDC, CreateFontIndirect(&lfFont));
    }
    else {
        hOldFont = (HFONT)SelectObject(hMemDC, CreateFontIndirect(&lfSKT1Font));
    }

    for (i = 0; i < SKT1_LETTER_KEY_NUM; i++) {
        int  cbCount;
        RECT rcOpaque;

        lpSKT1Ctxt->wCodeTable[i] = lpSoftKbdData->wCode[0][bSKT1VirtKey[i]];

        cbCount = (lpSKT1Ctxt->wCodeTable[i] == 0) ? 0 : 1;

        rcOpaque.left = lpSKT1Ctxt->ptButtonPos[i].x + SKT1_LABEL_BMP_X -
            SKT1_XOVERLAP;
        rcOpaque.top = lpSKT1Ctxt->ptButtonPos[i].y + SKT1_LABEL_BMP_Y;

        rcOpaque.right = rcOpaque.left +
            lpSKT1Ctxt->nButtonWidth[SKT1_LETTER_TYPE] - SKT1_LABEL_BMP_X +
            SKT1_XOVERLAP;
        rcOpaque.bottom = rcOpaque.top + lpSKT1Ctxt->nButtonHeight[0] -
            SKT1_LABEL_BMP_Y;

        ExtTextOut(hMemDC, rcOpaque.left, rcOpaque.top,
            ETO_OPAQUE, &rcOpaque,
            (LPWSTR)&lpSKT1Ctxt->wCodeTable[i], cbCount, NULL);
    }

    DeleteObject(SelectObject(hMemDC, hOldFont));

    DeleteDC(hMemDC);

    GlobalUnlock(hSKT1Ctxt);

    return (0);
}

/**********************************************************************/
/* SKWndProcT1                                                        */
/**********************************************************************/
LRESULT CALLBACK SKWndProcT1(
    HWND   hSKWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg) {
    case WM_CREATE:
        return CreateT1Window(hSKWnd);
    case WM_DESTROY:
        DestroyT1Window(hSKWnd);
        break;
    case WM_PAINT:
        {
            HDC         hDC;
            PAINTSTRUCT ps;

            hDC = BeginPaint(hSKWnd, &ps);
            UpdateSKT1Window(hDC, hSKWnd);
            EndPaint(hSKWnd, &ps);
        }
        break;
    case WM_SETCURSOR:
        if (!SKT1SetCursor(hSKWnd, lParam)) {
            return DefWindowProc(hSKWnd, uMsg, wParam, lParam);
        }
        break;
    case WM_MOUSEMOVE:
        if (!SKT1MouseMove(hSKWnd)) {
            return DefWindowProc(hSKWnd, uMsg, wParam, lParam);
        }
        break;
    case WM_LBUTTONUP:
        if (!SKT1ButtonUp(hSKWnd)) {
            return DefWindowProc(hSKWnd, uMsg, wParam, lParam);
        }
        break;
    case WM_MOUSEACTIVATE:
        return (MA_NOACTIVATE);
    case WM_SHOWWINDOW:
        if (lParam != 0) {
        } else if ((BOOL)wParam == TRUE) {
        } else {
            // we want to hide the soft keyboard on mouse button down
            SKT1ButtonUp(hSKWnd);
        }

        return DefWindowProc(hSKWnd, uMsg, wParam, lParam);

    case WM_IME_CONTROL:
        switch (wParam) {
        case IMC_GETSOFTKBDFONT:
            {
                HGLOBAL    hSKT1Ctxt;
                LPSKT1CTXT lpSKT1Ctxt;
                BYTE       lfCharSet;

                hSKT1Ctxt = (HGLOBAL)GetWindowLongPtr(hSKWnd, SKT1_CONTEXT);
                if (!hSKT1Ctxt) {
                    return (1);
                }

                lpSKT1Ctxt = (LPSKT1CTXT)GlobalLock(hSKT1Ctxt);
                if (!lpSKT1Ctxt) {
                    return (1);
                }

                lfCharSet = (BYTE)lpSKT1Ctxt->lfCharSet;

                GlobalUnlock(hSKT1Ctxt);

                *(LPLOGFONT)lParam = lfSKT1Font;

                if (lfCharSet != DEFAULT_CHARSET) {
                    ((LPLOGFONT)lParam)->lfCharSet = lfCharSet;
                }

                return (0);
            }
            break;
        case IMC_SETSOFTKBDFONT:
            {
                // in differet version of Windows
                if (lfSKT1Font.lfCharSet != ((LPLOGFONT)lParam)->lfCharSet) {
                    HGLOBAL    hSKT1Ctxt;
                    LPSKT1CTXT lpSKT1Ctxt;

                    hSKT1Ctxt = (HGLOBAL)GetWindowLongPtr(hSKWnd,
                        SKT1_CONTEXT);
                    if (!hSKT1Ctxt) {
                        return 1;
                    }

                    lpSKT1Ctxt = (LPSKT1CTXT)GlobalLock(hSKT1Ctxt);
                    if (!lpSKT1Ctxt) {
                        return 1;
                    }

                    lpSKT1Ctxt->lfCharSet = ((LPLOGFONT)lParam)->lfCharSet;

                    GlobalUnlock(hSKT1Ctxt);
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
                POINT          ptSoftKbdPos;
                HWND           hUIWnd;
                HIMC           hIMC;
                LPINPUTCONTEXT lpIMC;

                ptSoftKbdPos.x = ((LPPOINTS)lParam)->x;
                ptSoftKbdPos.y = ((LPPOINTS)lParam)->y;

                SetWindowPos(hSKWnd, NULL,
                    ptSoftKbdPos.x, ptSoftKbdPos.y,
                    0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);

                // Here we want to get - the owner or parent window
                hUIWnd = GetParent(hSKWnd);

                if (!hUIWnd) {
                    return (1);
                }

                hIMC = (HIMC) GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);

                if (!hIMC) {
                    return (1);
                }

                lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);

                if (!lpIMC) {
                    return (1);
                }

                lpIMC->ptSoftKbdPos = ptSoftKbdPos;

                ImmUnlockIMC(hIMC);

                return (0);
            }
            break;
        case IMC_SETSOFTKBDDATA:
            {
                LRESULT lRet;

                lRet = SetSKT1Data(hSKWnd, (LPSOFTKBDDATA)lParam);
                if (!lRet) {
                    InvalidateRect(hSKWnd, NULL, FALSE);
                    PostMessage(hSKWnd, WM_PAINT, 0, 0);
                }
                return (lRet);
            }
            break;
        case IMC_GETSOFTKBDSUBTYPE:
        case IMC_SETSOFTKBDSUBTYPE:
            {
                HGLOBAL    hSKT1Ctxt;
                LPSKT1CTXT lpSKT1Ctxt;
                LRESULT    lRet;

                lRet = -1;

                hSKT1Ctxt = (HGLOBAL)GetWindowLongPtr(hSKWnd, SKT1_CONTEXT);
                if (!hSKT1Ctxt) {
                    return (lRet);
                }

                lpSKT1Ctxt = (LPSKT1CTXT)GlobalLock(hSKT1Ctxt);
                if (!lpSKT1Ctxt) {
                    return (lRet);
                }

                if (wParam == IMC_GETSOFTKBDSUBTYPE) {
                    lRet = lpSKT1Ctxt->uSubType;
                } else if (wParam == IMC_SETSOFTKBDSUBTYPE) {
                    lRet = lpSKT1Ctxt->uSubType;
                    lpSKT1Ctxt->uSubType = (UINT)lParam;
                } else {
                    lRet = -1;
                }

                GlobalUnlock(hSKT1Ctxt);
                return (lRet);
            }
            break;
        default:
            break;
        }
        break;
    default:
        return DefWindowProc(hSKWnd, uMsg, wParam, lParam);
    }

    return (0L);
}
