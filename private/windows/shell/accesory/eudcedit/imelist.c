/**************************************************/
/*                                                */
/*                                                */
/*      Chinese IME List Dialog Class             */
/*                                                */
/*                                                */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

#include  <windows.h>
#include  "imm.h"
#include  "imelist.h"
#include  "resource.h"


static const TCHAR  szRegWordCls[] = TEXT("Radical");
static const TCHAR  szImeLinkDlg[] = TEXT("ImeLinkDlg");

static HINSTANCE    hAppInst;

static DWORD    aIds[] =
{
    IDD_RADICAL, IDH_EUDC_LINK_IMELIST,
    0, 0
};

static const COUNTRYSETTING sCountry[] = {
    {
        BIG5_CP, TEXT("BIG5")
    }
    , {
        ALT_BIG5_CP, TEXT("BIG5")
    }
#if defined(UNICODE)
    , {
        UNICODE_CP, TEXT("UNICODE")
    }
#endif
    , {
        GB2312_CP, TEXT("GB2312")
    }
};


/****************************************/
/*                                      */
/*      Create IME string listbox       */
/*                                      */
/****************************************/
LPIMELINKREGWORD
RegWordCreate(
HWND    hWnd)
{
    LPIMERADICALRECT lpImeLinkRadical;
    LPIMELINKREGWORD lpImeLinkRegWord;
    LPREGWORDSTRUCT  lpRegWordStructTmp;
    HDC              hDC;
    HWND             hEudcEditWnd;
    UINT             nLayouts, i, nIMEs;
    HKL FAR         *lphKL;
    DWORD            dwSize;
    TCHAR            szStrBuf[16];
    SIZE             lTextSize;
    RECT             rcRect;
    TCHAR            szTitle[32];
    TCHAR            szMessage[256];

    hEudcEditWnd = GetWindow( GetParent( hWnd), GW_OWNER);

    nLayouts = GetKeyboardLayoutList( 0, NULL);

    if( !(lphKL = GlobalAlloc(GPTR, sizeof(HKL)*nLayouts)))
        return NULL;

    lpImeLinkRegWord = NULL;
    GetKeyboardLayoutList( nLayouts, lphKL);

    for( i = 0, nIMEs = 0; i < nLayouts; i++) {
        LRESULT lRet;
        HKL   hKL;
        TCHAR szImeEudcDic[80];

        hKL = *(lphKL + i);
        if( !(lRet = ImmIsIME( hKL)))
            continue;

        szImeEudcDic[0] = '\0';
        lRet = ImmEscape( hKL, (HIMC)NULL, IME_ESC_GET_EUDC_DICTIONARY,
            szImeEudcDic);

        if( !lRet){
            continue;
        }else if( szImeEudcDic[0]){
        }else{
            continue;
        }
        *(lphKL + nIMEs) = hKL;

        nIMEs++;
    }
    if( !nIMEs){
        LoadString( hAppInst, IDS_NOIME_TITLE, szTitle, sizeof(szTitle) / sizeof(TCHAR));
        LoadString( hAppInst, IDS_NOIME_MSG, szMessage, sizeof(szMessage) / sizeof(TCHAR));

        MessageBox( hEudcEditWnd, szMessage, szTitle, MB_OK);
        goto RegWordCreateFreeHKL;
    }

    dwSize = sizeof( IMELINKREGWORD) - sizeof( REGWORDSTRUCT) +
        sizeof( REGWORDSTRUCT) * nIMEs;
    lpImeLinkRegWord = (LPIMELINKREGWORD)GlobalAlloc( GPTR, dwSize);

    if( !lpImeLinkRegWord){
        LoadString( hAppInst, IDS_NOMEM_TITLE, szTitle, sizeof(szTitle) / sizeof(TCHAR));
        LoadString( hAppInst, IDS_NOMEM_MSG, szMessage, sizeof(szMessage) / sizeof(TCHAR));

        MessageBox( hEudcEditWnd, szMessage, szTitle, MB_OK);
        goto RegWordCreateFreeHKL;
    }

    lpImeLinkRegWord->nEudcIMEs = nIMEs;
    lpRegWordStructTmp = &lpImeLinkRegWord->sRegWordStruct[0];
    for( i = 0; i < nIMEs; i++){
        LRESULT lRet;
#ifndef UNICODE
        UINT    j, uInternal;
#endif
        UINT uReadingSize;

        lpRegWordStructTmp->hKL = *(lphKL + i);
        if( !(lRet = ImmEscape(lpRegWordStructTmp->hKL, (HIMC)NULL,
            IME_ESC_MAX_KEY, NULL))){
            lpImeLinkRegWord->nEudcIMEs--;
            continue;
        }

        uReadingSize = sizeof(TCHAR);

#ifndef UNICODE
        for (j = 0; j < 256; j++) {
            uInternal = ImmEscape(lpRegWordStructTmp->hKL, (HIMC)NULL,
                IME_ESC_SEQUENCE_TO_INTERNAL, &j);
            if (uInternal > 255) {
                uReadingSize = sizeof(WCHAR);
                break;
            }
        }
#endif

        if( lRet * uReadingSize > sizeof(lpRegWordStructTmp->szReading) - sizeof(TCHAR)){
            lpImeLinkRegWord->nEudcIMEs--;
            continue;
        }

        if( !(lRet = ImmEscape(lpRegWordStructTmp->hKL, (HIMC)NULL,
            IME_ESC_IME_NAME, lpRegWordStructTmp->szIMEName))){
            lpImeLinkRegWord->nEudcIMEs--;
            continue;
        }

        lpRegWordStructTmp->szIMEName[
            sizeof(lpRegWordStructTmp->szIMEName) / sizeof(TCHAR) - 1] = '\0';
        lpRegWordStructTmp->uIMENameLen =
            lstrlen( lpRegWordStructTmp->szIMEName);

        lpRegWordStructTmp++;
    }
    if( !lpImeLinkRegWord->nEudcIMEs){
        LoadString( hAppInst, IDS_NOIME_TITLE, szTitle, sizeof(szTitle) / sizeof(TCHAR));
        LoadString( hAppInst, IDS_NOIME_MSG, szMessage, sizeof(szMessage) / sizeof(TCHAR));

        MessageBox( hEudcEditWnd, szMessage, szTitle, MB_OK);
        goto RegWordCreateFreeRegWord;
    }
    LoadString( hAppInst, IDS_CHINESE_CHAR, szStrBuf, sizeof( szStrBuf) / sizeof(TCHAR));
    hDC = GetDC(NULL);
    GetTextExtentPoint( hDC, szStrBuf, lstrlen( szStrBuf), &lTextSize);
    ReleaseDC(NULL, hDC);
    GetWindowRect( hWnd, &rcRect);

    nIMEs = (rcRect.bottom - rcRect.top) / (2 * lTextSize.cy);
    if( lpImeLinkRegWord->nEudcIMEs <= nIMEs){
        nIMEs = lpImeLinkRegWord->nEudcIMEs;
    }
    dwSize = sizeof(IMERADICALRECT) - sizeof(RECT) + sizeof(RECT) *
        RECT_NUMBER * nIMEs;
    lpImeLinkRadical = (LPIMERADICALRECT)GlobalAlloc( GPTR, dwSize);

    if( !lpImeLinkRadical){
        lpImeLinkRegWord->nEudcIMEs = 0;
        LoadString( hAppInst, IDS_NOMEM_TITLE, szTitle, sizeof(szTitle) / sizeof(TCHAR));
        LoadString( hAppInst, IDS_NOMEM_MSG, szMessage, sizeof(szMessage) / sizeof(TCHAR));

        MessageBox( hEudcEditWnd, szMessage, szTitle, MB_OK);
        goto RegWordCreateFreeRegWord;
    }
    lpImeLinkRadical->nStartIME = 0;
    lpImeLinkRadical->nPerPageIMEs = nIMEs;
    lpImeLinkRadical->lTextSize = lTextSize;
    if( lpImeLinkRegWord->nEudcIMEs > nIMEs) {
        SCROLLINFO scInfo;
        lpImeLinkRadical->hScrollWnd = CreateWindowEx( 0,
            TEXT("scrollbar"), NULL, WS_CHILD|WS_VISIBLE|SBS_VERT,
            rcRect.right - rcRect.left - lTextSize.cx, 0,
            lTextSize.cx, rcRect.bottom - rcRect.top,
            hWnd, 0, hAppInst, NULL);

        scInfo.cbSize = sizeof(SCROLLINFO);
        scInfo.fMask = SIF_ALL;
        scInfo.nMin = 0;
        scInfo.nMax = lpImeLinkRegWord->nEudcIMEs - 1 + (nIMEs - 1);
        scInfo.nPage = nIMEs;
        scInfo.nPos = 0;
        scInfo.nTrackPos = 0;
        SetScrollInfo( lpImeLinkRadical->hScrollWnd,
            SB_CTL, &scInfo, FALSE);
    }
    for( i = 0; i < nIMEs; i++){
        UINT j, k;
        j = i * RECT_NUMBER + RECT_IMENAME;
        lpImeLinkRadical->rcRadical[j].left = lTextSize.cx;
        lpImeLinkRadical->rcRadical[j].top = lTextSize.cy *
            (i * 4 + 1) / 2 - UI_MARGIN;

        lpImeLinkRadical->rcRadical[j].right =
            lpImeLinkRadical->rcRadical[j].left + lTextSize.cx * 6;

        lpImeLinkRadical->rcRadical[j].bottom =
            lpImeLinkRadical->rcRadical[j].top + lTextSize.cy +
            UI_MARGIN * 2;

        k = i * RECT_NUMBER + RECT_RADICAL;
        lpImeLinkRadical->rcRadical[k].left =
            lpImeLinkRadical->rcRadical[j].right + lTextSize.cx;

        lpImeLinkRadical->rcRadical[k].top =
            lpImeLinkRadical->rcRadical[j].top;

        lpImeLinkRadical->rcRadical[k].right =
            lpImeLinkRadical->rcRadical[k].left + lTextSize.cx *
            (sizeof(lpRegWordStructTmp->szReading) / sizeof(TCHAR) / 2 - 1);

        lpImeLinkRadical->rcRadical[k].bottom =
            lpImeLinkRadical->rcRadical[k].top + lTextSize.cy +
            UI_MARGIN * 2;
    }
    SetWindowLongPtr(hWnd, GWLP_RADICALRECT, (LONG_PTR)lpImeLinkRadical);

RegWordCreateFreeRegWord:
    if( !lpImeLinkRegWord->nEudcIMEs){
        GlobalFree((HGLOBAL)lpImeLinkRegWord);
        lpImeLinkRegWord = NULL;
    }

RegWordCreateFreeHKL:
    GlobalFree((HGLOBAL)lphKL);
    return( lpImeLinkRegWord);
}

/****************************************/
/*                                      */
/*      Switch To Mouse Clicked IME     */
/*                                      */
/****************************************/
void
SwitchToThisIME(
HWND 	hWnd,
UINT 	uIndex)
{
    LPIMELINKREGWORD  lpImeLinkRegWord;
    LPREGWORDSTRUCT   lpRegWordStructTmp;
    LPIMERADICALRECT  lpImeLinkRadical;
    DWORD             fdwConversionMode, fdwSentenceMode;

    lpImeLinkRegWord = (LPIMELINKREGWORD)GetWindowLongPtr( hWnd,
        GWLP_IMELINKREGWORD);

    if( lpImeLinkRegWord->nCurrIME == uIndex)
        return;

    if( uIndex >= lpImeLinkRegWord->nEudcIMEs){
        MessageBeep((UINT)-1);
        return;
    }
    lpImeLinkRadical = (LPIMERADICALRECT)GetWindowLongPtr(hWnd,
        GWLP_RADICALRECT);

    if( uIndex < lpImeLinkRadical->nStartIME){
        lpImeLinkRadical->nStartIME = uIndex;
    }else if(( uIndex - lpImeLinkRadical->nStartIME) >=
        lpImeLinkRadical->nPerPageIMEs){
        lpImeLinkRadical->nStartIME = uIndex -
            (lpImeLinkRadical->nPerPageIMEs - 1);
    }else{
    }
    SendMessage(hWnd, WM_EUDC_COMPMSG, 0, FALSE);
    lpRegWordStructTmp = &lpImeLinkRegWord->sRegWordStruct[uIndex];
    ActivateKeyboardLayout(lpRegWordStructTmp->hKL, 0);

    ImmGetConversionStatus(lpImeLinkRegWord->hRegWordIMC,
        &fdwConversionMode, &fdwSentenceMode);
    fdwConversionMode = (fdwConversionMode | IME_CMODE_EUDC |
        IME_CMODE_NATIVE) | (fdwConversionMode & IME_CMODE_SOFTKBD);

    ImmSetConversionStatus(lpImeLinkRegWord->hRegWordIMC,
        fdwConversionMode, fdwSentenceMode);

    SendMessage(hWnd, WM_EUDC_COMPMSG, 0, TRUE);
    lpImeLinkRegWord->nCurrIME = uIndex;

    if(lpImeLinkRadical->hScrollWnd){
        SCROLLINFO scInfo;

        scInfo.cbSize = sizeof(SCROLLINFO);
        scInfo.fMask = SIF_POS;
        scInfo.nPos = lpImeLinkRegWord->nCurrIME;

        SetScrollInfo(lpImeLinkRadical->hScrollWnd,
            SB_CTL, &scInfo, FALSE);
    }
    InvalidateRect(hWnd, NULL, TRUE);

    *(LPTSTR)&lpRegWordStructTmp->szReading[
        lpRegWordStructTmp->dwReadingLen] = '\0';

    ImmSetCompositionString(lpImeLinkRegWord->hRegWordIMC, SCS_SETSTR,
        NULL, 0, lpRegWordStructTmp->szReading,
        lpRegWordStructTmp->dwReadingLen * sizeof(TCHAR));
    SetFocus(hWnd);
    return;
}

/****************************************/
/*                                      */
/*      MESSAGE "WM_IMECOMPOSITION"     */
/*                                      */
/****************************************/
void
WmImeComposition(
HWND   	hWnd,
LPARAM 	lParam)
{
    LPIMELINKREGWORD lpImeLinkRegWord;
    LPREGWORDSTRUCT  lpRegWordStructTmp;
    TCHAR            szReading[sizeof(lpRegWordStructTmp->szReading)/sizeof(TCHAR)];
    LONG             lRet;
    BOOL             bUpdate;

    lpImeLinkRegWord = (LPIMELINKREGWORD)GetWindowLongPtr( hWnd,
        GWLP_IMELINKREGWORD);
    lpRegWordStructTmp = &lpImeLinkRegWord->sRegWordStruct[
        lpImeLinkRegWord->nCurrIME];

    lRet = ImmGetCompositionString(lpImeLinkRegWord->hRegWordIMC,
        GCS_COMPREADSTR, szReading, sizeof(szReading));
    if (lRet < 0) {
        lpRegWordStructTmp->bUpdate = UPDATE_ERROR;
        return;
    }
    if (lRet > (sizeof(szReading) - sizeof(TCHAR))) {
        lRet = sizeof(szReading) - sizeof(TCHAR);
    }
    szReading[lRet / sizeof(TCHAR)] = '\0';

    if( lpRegWordStructTmp->dwReadingLen != (DWORD)lRet / sizeof(TCHAR)){
        bUpdate = TRUE;
    }else if( lstrcmp(lpRegWordStructTmp->szReading, szReading)){
        bUpdate = TRUE;
    }else  bUpdate = FALSE;

    if( bUpdate){
        LPIMERADICALRECT lpImeLinkRadical;
        UINT             i;
        UINT             j, k;

        lpImeLinkRadical = (LPIMERADICALRECT)GetWindowLongPtr(hWnd,
            GWLP_RADICALRECT);

        lstrcpy( lpRegWordStructTmp->szReading, szReading);

        if( lParam & GCS_RESULTSTR){
            lpRegWordStructTmp->bUpdate = UPDATE_FINISH;
        }else{
            lpRegWordStructTmp->bUpdate = UPDATE_START;
        }

        lpRegWordStructTmp->dwReadingLen = (DWORD)lRet / sizeof(TCHAR);

        if( !IsWindowEnabled(lpImeLinkRadical->hRegWordButton)){
            EnableWindow(lpImeLinkRadical->hRegWordButton, TRUE);
        }

        i = lpImeLinkRegWord->nCurrIME - lpImeLinkRadical->nStartIME;
        j = i * RECT_NUMBER + RECT_IMENAME;
        InvalidateRect(hWnd, &lpImeLinkRadical->rcRadical[j], FALSE);

        k = i * RECT_NUMBER + RECT_RADICAL;

        InvalidateRect(hWnd, &lpImeLinkRadical->rcRadical[k], FALSE);
    }else if( lParam & GCS_RESULTSTR){
        LPIMERADICALRECT lpImeLinkRadical;
        UINT i;
        UINT j, k;

        lpImeLinkRadical = (LPIMERADICALRECT)GetWindowLongPtr(hWnd,
            GWLP_RADICALRECT);
        if( lpRegWordStructTmp->bUpdate){
            lpRegWordStructTmp->bUpdate = UPDATE_FINISH;
        }
        i = lpImeLinkRegWord->nCurrIME - lpImeLinkRadical->nStartIME;
        j = i * RECT_NUMBER + RECT_IMENAME;
        InvalidateRect(hWnd, &lpImeLinkRadical->rcRadical[j], FALSE);

        k = i * RECT_NUMBER + RECT_RADICAL;
        InvalidateRect(hWnd, &lpImeLinkRadical->rcRadical[k], FALSE);
    }
    return;
}

/************************************************************/
/*                                                          */
/*  lstrcmpn                                                */
/*                                                          */
/************************************************************/
int lstrcmpn(
    LPCTSTR lpctszStr1,
    LPCTSTR lpctszStr2,
    int     cCount)
{
    int i;

    for (i = 0; i < cCount; i++) {
        int iCmp = *lpctszStr1++ - *lpctszStr2++;
        if (iCmp) { return iCmp; }
    }

    return 0;
}

/****************************************/
/*                                      */
/*      CALLBACK  ENUMREADING           */
/*                                      */
/****************************************/
int CALLBACK
EnumReading(
LPCTSTR         lpszReading,
DWORD           dwStyle,
LPCTSTR         lpszString,
LPREGWORDSTRUCT lpRegWordStructTmp)
{
    int     iLen;
    DWORD   dwZeroSeq;
    LRESULT lRet;
    TCHAR   tszZeroSeq[8];

    iLen = lstrlen(lpszReading);

    if (iLen * sizeof(TCHAR) > sizeof(lpRegWordStructTmp->szReading) -
        sizeof(WORD)) {
        return (0);
    }

    lpRegWordStructTmp->dwReadingLen = (DWORD)iLen;

    lstrcpy(lpRegWordStructTmp->szReading, lpszReading);

    dwZeroSeq = 0;
    lRet = ImmEscape(lpRegWordStructTmp->hKL, (HIMC)NULL,
        IME_ESC_SEQUENCE_TO_INTERNAL, &dwZeroSeq);

    if (!lRet) { goto Over; }

    iLen = 0;

    if (LOWORD(lRet)) {
#ifdef UNICODE
        tszZeroSeq[iLen++] = LOWORD(lRet);
#else
        if (LOWORD(lRet) > 0xFF) {
            tszZeroSeq[iLen++] = HIBYTE(LOWORD(lRet));
            tszZeroSeq[iLen++] = LOBYTE(LOWORD(lRet));
        } else {
            tszZeroSeq[iLen++] = LOBYTE(LOWORD(lRet));
        }
#endif
    }

    if (HIWORD(lRet) == 0xFFFF) {
        // This is caused by sign extent in Win9x in the return value of
        // ImmEscape, it causes an invalid internal code.
    } else if (HIWORD(lRet)) {
#ifdef UNICODE
        tszZeroSeq[iLen++] = HIWORD(lRet);
#else
        if (HIWORD(lRet) > 0xFF) {
            tszZeroSeq[iLen++] = HIBYTE(HIWORD(lRet));
            tszZeroSeq[iLen++] = LOBYTE(HIWORD(lRet));
        } else {
            tszZeroSeq[iLen++] = LOBYTE(HIWORD(lRet));
        }
#endif
    } else {
    }

    for (; lpRegWordStructTmp->dwReadingLen > 0;
        lpRegWordStructTmp->dwReadingLen -= iLen) {
        if (lstrcmpn(&lpRegWordStructTmp->szReading[
            lpRegWordStructTmp->dwReadingLen - iLen], tszZeroSeq, iLen) != 0) {
            break;
        }
    }

Over:
    lpRegWordStructTmp->szReading[lpRegWordStructTmp->dwReadingLen] = '\0';

    return (1);
}

/****************************************/
/*                                      */
/*      EUDC CODE Calcurater            */
/*                                      */
/****************************************/
void
EudcCode(
HWND    hWnd,
DWORD   dwComboCode)
{
    LPIMELINKREGWORD lpImeLinkRegWord;
    LPREGWORDSTRUCT  lpRegWordStructTmp;
    UINT             i, uCode;
    BOOL             bUnicodeMode=FALSE;

    WCHAR            TmpCode[1];
    CHAR             TmpCodeAnsi[2];

    lpImeLinkRegWord = (LPIMELINKREGWORD)GetWindowLongPtr(hWnd,
        GWLP_IMELINKREGWORD);

    uCode = LOWORD(dwComboCode);

    if (HIWORD(dwComboCode)) // the code is in unicode
        bUnicodeMode = TRUE;

#ifdef UNICODE
    if (bUnicodeMode) {
        lpImeLinkRegWord->szEudcCodeString[0] = (WCHAR)uCode;
        lpImeLinkRegWord->szEudcCodeString[1] = TEXT('\0');
    }else{
        TmpCodeAnsi[0] = HIBYTE(uCode);
        TmpCodeAnsi[1] = LOBYTE(uCode);
        //
        // convert to Unicode string
        //
        MultiByteToWideChar(GetACP(), MB_PRECOMPOSED,
            (LPCSTR)TmpCodeAnsi, 2,
            (LPWSTR)TmpCode, 1);

        lpImeLinkRegWord->szEudcCodeString[0] = (WCHAR)TmpCode[0];
        lpImeLinkRegWord->szEudcCodeString[1] = TEXT('\0');
    }
#else //UNICODE
    if (bUnicodeMode) {
        TmpCode[0] = (WCHAR) uCode;

        //
        //  Convert to Ansi byte string
        //
        WideCharToMultiByte(GetACP(), WC_COMPOSITECHECK,
            (LPWSTR)TmpCode, 1,
            (LPSTR)TmpCodeAnsi, 2,
            NULL, NULL);

        lpImeLinkRegWord->szEudcCodeString[0] = TmpCodeAnsi[0];
        lpImeLinkRegWord->szEudcCodeString[1] = TmpCodeAnsi[1];
    }else{
        lpImeLinkRegWord->szEudcCodeString[0] = HIBYTE(uCode);
        lpImeLinkRegWord->szEudcCodeString[1] = LOBYTE(uCode);
    }
#endif //UNICODE

    lpImeLinkRegWord->szEudcCodeString[2] =
        lpImeLinkRegWord->szEudcCodeString[3] = '\0';

    lpRegWordStructTmp = &lpImeLinkRegWord->sRegWordStruct[0];

    for( i = 0; i < lpImeLinkRegWord->nEudcIMEs; i++){
        lpRegWordStructTmp->bUpdate = UPDATE_NONE;
        lpRegWordStructTmp->szReading[0] = '\0';
        lpRegWordStructTmp->dwReadingLen = 0;

        ImmEnumRegisterWord(lpRegWordStructTmp->hKL, EnumReading,
            NULL, IME_REGWORD_STYLE_EUDC,
            lpImeLinkRegWord->szEudcCodeString,
            lpRegWordStructTmp);

        lpRegWordStructTmp->dwReadingLen =
            lstrlen( lpRegWordStructTmp->szReading);
        lpRegWordStructTmp++;
    }
    lpRegWordStructTmp = &lpImeLinkRegWord->sRegWordStruct[
        lpImeLinkRegWord->nCurrIME];

    ImmSetCompositionString(lpImeLinkRegWord->hRegWordIMC, SCS_SETSTR,
        NULL, 0, lpRegWordStructTmp->szReading,
        lpRegWordStructTmp->dwReadingLen * sizeof(TCHAR));

    InvalidateRect(hWnd, NULL, FALSE);
    return;
}

/****************************************/
/*                                      */
/*      Change To Mouse Clicked IME     */
/*                                      */
/****************************************/
void
ChangeToOtherIME(
HWND   	hWnd,
LPARAM 	lMousePos)
{
    LPIMERADICALRECT lpImeLinkRadical;
    POINT            ptMouse;
    UINT             i;
    BOOL             bFound;

    ptMouse.x = LOWORD( lMousePos);
    ptMouse.y = HIWORD( lMousePos);

    lpImeLinkRadical = (LPIMERADICALRECT)GetWindowLongPtr( hWnd,
        GWLP_RADICALRECT);
    bFound = FALSE;
    for( i = 0; i < lpImeLinkRadical->nPerPageIMEs; i++){
        UINT j;

        j = i * RECT_NUMBER + RECT_RADICAL;
        if( PtInRect(&lpImeLinkRadical->rcRadical[j], ptMouse)){
            bFound = TRUE;
            break;
        }
    }
    if( !bFound) return;
    SwitchToThisIME( hWnd, lpImeLinkRadical->nStartIME + i);
    return;
}

/****************************************/
/*                                      */
/*      ScrollUP or Down IME LISTBOX    */
/*                                      */
/****************************************/
void
ScrollIME(
HWND   	hWnd,
WPARAM 	wParam)
{
    LPIMELINKREGWORD lpImeLinkRegWord;
    LPIMERADICALRECT lpImeLinkRadical;
    int              iLines;
    UINT             uIndex;

    lpImeLinkRegWord = (LPIMELINKREGWORD)GetWindowLongPtr( hWnd,
        GWLP_IMELINKREGWORD);
    lpImeLinkRadical = (LPIMERADICALRECT)GetWindowLongPtr( hWnd,
        GWLP_RADICALRECT);

    switch( LOWORD( wParam)){
    case SB_PAGEDOWN:
        iLines = lpImeLinkRadical->nPerPageIMEs - 1;
        break;
    case SB_LINEDOWN:
        iLines = 1;
        break;
    case SB_PAGEUP:
        iLines = 1 - lpImeLinkRadical->nPerPageIMEs;
        break;
    case SB_LINEUP:
        iLines = -1;
        break;
    case SB_TOP:
        SwitchToThisIME(hWnd, 0);
        return;
    case SB_BOTTOM:
        SwitchToThisIME(hWnd, lpImeLinkRegWord->nEudcIMEs - 1);
        return;
    case SB_THUMBPOSITION:
        SwitchToThisIME(hWnd, HIWORD(wParam));
        return;
    default:
        return;
    }
    uIndex = lpImeLinkRegWord->nCurrIME;
    if( iLines > 0){
        uIndex += (UINT)iLines;
        if( uIndex >= lpImeLinkRegWord->nEudcIMEs){
            uIndex = lpImeLinkRegWord->nEudcIMEs - 1;
        }
    }else{
        UINT uLines;

        uLines = -iLines;
        if( uLines > uIndex){
            uIndex = 0;
        }else  uIndex -= uLines;
    }
    SwitchToThisIME(hWnd, uIndex);
    return;
}

/****************************************/
/*                                      */
/*      ScrollUP or Down IME LISTBOX    */
/*                                      */
/****************************************/
void
ScrollIMEByKey(
HWND   	hWnd,
WPARAM 	wParam)
{
    switch( wParam){
    case VK_NEXT:
        ScrollIME( hWnd, SB_PAGEDOWN);
        break;
    case VK_DOWN:
        ScrollIME( hWnd, SB_LINEDOWN);
        break;
    case VK_PRIOR:
        ScrollIME(hWnd, SB_PAGEUP);
        break;
    case VK_UP:
        ScrollIME(hWnd, SB_LINEUP);
        break;
    default:
        return;
    }
    return;
}

/****************************************/
/*                                      */
/*      Get Focus in RegWord List       */
/*                                      */
/****************************************/
void
RegWordGetFocus(
HWND    hWnd)
{
    LPIMELINKREGWORD lpImeLinkRegWord;
    LPIMERADICALRECT lpImeLinkRadical;
    UINT             i;

    lpImeLinkRegWord = (LPIMELINKREGWORD)GetWindowLongPtr( hWnd,
        GWLP_IMELINKREGWORD);
    lpImeLinkRadical = (LPIMERADICALRECT)GetWindowLongPtr( hWnd,
        GWLP_RADICALRECT);

    CreateCaret( hWnd, NULL, 2, lpImeLinkRadical->lTextSize.cy +
        CARET_MARGIN * 2);
    if( lpImeLinkRegWord->nCurrIME < lpImeLinkRadical->nStartIME){
        lpImeLinkRegWord->nCurrIME = lpImeLinkRadical->nStartIME;
    }else if(( lpImeLinkRegWord->nCurrIME - lpImeLinkRadical->nStartIME) >=
        lpImeLinkRadical->nPerPageIMEs) {
        lpImeLinkRegWord->nCurrIME = lpImeLinkRadical->nStartIME +
            lpImeLinkRadical->nPerPageIMEs - 1;
    }
    i = lpImeLinkRegWord->nCurrIME - lpImeLinkRadical->nStartIME;
    i = (i * RECT_NUMBER) + RECT_RADICAL;
    SetCaretPos(lpImeLinkRadical->rcRadical[i].left +
        lpImeLinkRadical->lCurrReadingExtent.cx + 2,
        lpImeLinkRadical->rcRadical[i].top + UI_MARGIN - CARET_MARGIN);
    ShowCaret(hWnd);
    return;
}

/****************************************/
/*                                      */
/*      MESSAGE "WM_PAINT"              */
/*                                      */
/****************************************/
void RegWordPaint(
HWND    hWnd)
{
    LPIMERADICALRECT lpImeLinkRadical;
    LPIMELINKREGWORD lpImeLinkRegWord;
    LPREGWORDSTRUCT  lpRegWordStructTmp;
    PAINTSTRUCT      ps;
    HDC              hDC;
    UINT             i;
    UINT             nShowIMEs;

    lpImeLinkRadical = (LPIMERADICALRECT)GetWindowLongPtr(hWnd,
        GWLP_RADICALRECT);
    lpImeLinkRegWord = (LPIMELINKREGWORD)GetWindowLongPtr(hWnd,
        GWLP_IMELINKREGWORD);
    lpRegWordStructTmp = &lpImeLinkRegWord->sRegWordStruct[
        lpImeLinkRadical->nStartIME];
    HideCaret(hWnd);

    hDC = BeginPaint(hWnd, &ps);

    nShowIMEs = lpImeLinkRegWord->nEudcIMEs - lpImeLinkRadical->nStartIME;
    if( nShowIMEs > lpImeLinkRadical->nPerPageIMEs){
        nShowIMEs = lpImeLinkRadical->nPerPageIMEs;
    }
    for( i = 0; i < nShowIMEs; i++){
        RECT rcSunken;
        UINT j, k;

        k = i * RECT_NUMBER + RECT_RADICAL;
        rcSunken = lpImeLinkRadical->rcRadical[k];
        rcSunken.left -= 2;
        rcSunken.top -= 2;
        rcSunken.right += 2;
        rcSunken.bottom += 2;
        DrawEdge(hDC, &rcSunken, BDR_SUNKENOUTER, BF_RECT);
        SetBkColor(hDC, GetSysColor(COLOR_BTNFACE));
        if( lpRegWordStructTmp->bUpdate == UPDATE_ERROR){
            SetTextColor(hDC, RGB(0xFF, 0x00, 0x00));
        }else if(lpRegWordStructTmp->bUpdate == UPDATE_START){
            SetTextColor(hDC, RGB(0xFF, 0xFF, 0x00));
        }else if(lpRegWordStructTmp->bUpdate == UPDATE_REGISTERED){
            SetTextColor(hDC, RGB(0x00, 0x80, 0x00));
        }else{
            SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
        }
        j = i * RECT_NUMBER + RECT_IMENAME;

        ExtTextOut(hDC, lpImeLinkRadical->rcRadical[j].left,
            lpImeLinkRadical->rcRadical[j].top,
            ETO_OPAQUE | ETO_CLIPPED, &lpImeLinkRadical->rcRadical[j],
            lpRegWordStructTmp->szIMEName,
            lpRegWordStructTmp->uIMENameLen, NULL);

        if(( lpImeLinkRegWord->nCurrIME - lpImeLinkRadical->nStartIME) == i){
            SetBkColor(hDC, GetSysColor(COLOR_WINDOW));
            SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));

            GetTextExtentPoint(hDC, lpRegWordStructTmp->szReading,
                lpRegWordStructTmp->dwReadingLen,
                &lpImeLinkRadical->lCurrReadingExtent);

            SetCaretPos(lpImeLinkRadical->rcRadical[k].left +
                lpImeLinkRadical->lCurrReadingExtent.cx + 2,
                lpImeLinkRadical->rcRadical[k].top +
                UI_MARGIN - CARET_MARGIN);

        }else{
            SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
        }
        ExtTextOut(hDC, lpImeLinkRadical->rcRadical[k].left,
            lpImeLinkRadical->rcRadical[k].top + UI_MARGIN,
            ETO_OPAQUE, &lpImeLinkRadical->rcRadical[k],
            lpRegWordStructTmp->szReading,
            lpRegWordStructTmp->dwReadingLen, NULL);
        lpRegWordStructTmp++;
    }
    EndPaint(hWnd, &ps);
    ShowCaret(hWnd);
    return;
}

/****************************************/
/*                                      */
/*      Registry Word Window Proc       */
/*                                      */
/****************************************/
LRESULT CALLBACK
RegWordWndProc(
HWND   	hWnd,
UINT   	uMsg,
WPARAM 	wParam,
LPARAM 	lParam)
{
    switch( uMsg){
    case WM_CREATE:
        {
            LPIMELINKREGWORD lpImeLinkRegWord;
            UINT uIndex;

            SetWindowLongPtr( hWnd, GWLP_IMELINKREGWORD, 0L);
            SetWindowLongPtr( hWnd, GWLP_RADICALRECT, 0L);
            if( !(lpImeLinkRegWord = RegWordCreate( hWnd)))
                return (-1);
            lpImeLinkRegWord->fCompMsg = TRUE;
            lpImeLinkRegWord->nCurrIME = 0xFFFFFFFF;
            lpImeLinkRegWord->hRegWordIMC = ImmCreateContext();
            if( !lpImeLinkRegWord->hRegWordIMC){
                return (-1);
            }
            lpImeLinkRegWord->hOldIMC = ImmAssociateContext( hWnd,
                lpImeLinkRegWord->hRegWordIMC);

            SetWindowLongPtr(hWnd, GWLP_IMELINKREGWORD, (LONG_PTR)lpImeLinkRegWord);

            uIndex = 0;
            SwitchToThisIME(hWnd, 0);

            PostMessage( hWnd, WM_EUDC_SWITCHIME, 0, uIndex);
        }
        break;
    case WM_EUDC_COMPMSG:
        {
            LPIMELINKREGWORD lpImeLinkRegWord;

            lpImeLinkRegWord = (LPIMELINKREGWORD)GetWindowLongPtr(hWnd,
                GWLP_IMELINKREGWORD);
            lpImeLinkRegWord->fCompMsg = (BOOL)lParam;
        }
        break;
    case WM_EUDC_SWITCHIME:
        {
            LPIMELINKREGWORD lpImeLinkRegWord;

            lpImeLinkRegWord = (LPIMELINKREGWORD)GetWindowLongPtr( hWnd,
                GWLP_IMELINKREGWORD);

            lpImeLinkRegWord->nCurrIME = 0xFFFFFFFF;

            SwitchToThisIME( hWnd, (UINT)lParam);
        }
        break;
    case WM_IME_STARTCOMPOSITION:
    case WM_IME_ENDCOMPOSITION:
        break;
    case WM_IME_COMPOSITION:
        {
            LPIMELINKREGWORD lpImeLinkRegWord;

            lpImeLinkRegWord = (LPIMELINKREGWORD)GetWindowLongPtr(hWnd,
                GWLP_IMELINKREGWORD);
            if( lpImeLinkRegWord->fCompMsg){
                WmImeComposition(hWnd, lParam);
            }
        }
        break;
    case WM_IME_NOTIFY:
        switch( wParam){
        case IMN_OPENSTATUSWINDOW:
        case IMN_CLOSESTATUSWINDOW:
        case IMN_OPENCANDIDATE:
        case IMN_CHANGECANDIDATE:
        case IMN_CLOSECANDIDATE:
            break;
        default:
            return DefWindowProc( hWnd, uMsg, wParam, lParam);
        }
        break;
    case WM_IME_SETCONTEXT:
        return DefWindowProc( hWnd, uMsg, wParam,
            lParam & ~(ISC_SHOWUIALL));
    case WM_EUDC_REGISTER_BUTTON:
        {
            LPIMERADICALRECT lpImeRadicalRect;

            lpImeRadicalRect = (LPIMERADICALRECT)GetWindowLongPtr(hWnd,
                GWLP_RADICALRECT);
            lpImeRadicalRect->hRegWordButton = (HWND)lParam;
        }
        break;
    case WM_EUDC_CODE:
        EudcCode(hWnd, (DWORD)lParam);
        break;
    case WM_LBUTTONDOWN:
        ChangeToOtherIME(hWnd, lParam);
        break;
    case WM_VSCROLL:
        ScrollIME(hWnd, wParam);
        break;
    case WM_KEYDOWN:
        ScrollIMEByKey(hWnd, wParam);
        break;
    case WM_SETFOCUS:
        RegWordGetFocus(hWnd);
        break;
    case WM_KILLFOCUS:
        DestroyCaret();
        break;
    case WM_PAINT:
        RegWordPaint(hWnd);
        break;
    case WM_DESTROY:
        {
            LPIMERADICALRECT lpImeRadicalRect;
            LPIMELINKREGWORD lpImeLinkRegWord;

            lpImeRadicalRect = (LPIMERADICALRECT)GetWindowLongPtr(hWnd,
                GWLP_RADICALRECT);
            if( lpImeRadicalRect){
                GlobalFree((HGLOBAL)lpImeRadicalRect);
            }
            lpImeLinkRegWord = (LPIMELINKREGWORD)GetWindowLongPtr(hWnd,
                GWLP_IMELINKREGWORD);
            if (!lpImeLinkRegWord) {
                break;
            }

            ImmAssociateContext(hWnd, lpImeLinkRegWord->hOldIMC);
            ImmDestroyContext(lpImeLinkRegWord->hRegWordIMC);
            GlobalFree((HGLOBAL)lpImeLinkRegWord);
        }
        break;
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return (0L);
}

/****************************************/
/*                                      */
/*      Regist IME String               */
/*                                      */
/****************************************/
int
RegisterThisEudc(
HWND    hWnd)
{
    LPIMELINKREGWORD lpImeLinkRegWord;
    LPREGWORDSTRUCT  lpRegWordStructTmp;
    UINT             i;
    int              iRet;

    lpImeLinkRegWord = (LPIMELINKREGWORD)GetWindowLongPtr(hWnd,
        GWLP_IMELINKREGWORD);
    lpRegWordStructTmp = &lpImeLinkRegWord->sRegWordStruct[0];
    iRet = -1;

    for( i = 0; i < lpImeLinkRegWord->nEudcIMEs; i++, lpRegWordStructTmp++){
        if (( lpRegWordStructTmp->bUpdate == UPDATE_NONE) ||
            ( lpRegWordStructTmp->bUpdate == UPDATE_REGISTERED))
				{
        }
				else if( lpRegWordStructTmp->bUpdate != UPDATE_FINISH){
            TCHAR szStrBuf[128];
            int   iYesNo;
            if( iRet != -1){
                continue;
            }
            LoadString( hAppInst, IDS_QUERY_NOTFINISH, szStrBuf,
                sizeof(szStrBuf) / sizeof(TCHAR));

            iYesNo = MessageBox(hWnd, szStrBuf,
                lpRegWordStructTmp->szIMEName,
                MB_APPLMODAL|MB_YESNO|MB_DEFBUTTON1);
            if( iYesNo == IDYES){
                iRet = i;
            }
        }else{
            BOOL  fRet;
            TCHAR szStrBuf[128];
            int   iYesNo;

            fRet = ImmRegisterWord(lpRegWordStructTmp->hKL,
                lpRegWordStructTmp->szReading,
                IME_REGWORD_STYLE_EUDC,
                lpImeLinkRegWord->szEudcCodeString);
            if( fRet){
                lpRegWordStructTmp->bUpdate = UPDATE_REGISTERED;
                continue;
            }else{
                lpRegWordStructTmp->bUpdate = UPDATE_ERROR;
            }
            if( iRet != -1){
                continue;
            }
            LoadString(hAppInst, IDS_QUERY_REGISTER, szStrBuf,
                sizeof(szStrBuf) / sizeof(TCHAR));
            iYesNo = MessageBox(hWnd, szStrBuf,
                lpRegWordStructTmp->szIMEName,
                MB_APPLMODAL|MB_YESNO|MB_DEFBUTTON1);
            if( iYesNo == IDYES){
                iRet = i;
            }
        }
    }
    InvalidateRect( hWnd, NULL, FALSE);
    return (iRet);
}

/****************************************/
/*                                      */
/*      CodePage Info                   */
/*                                      */
/****************************************/
int CodePageInfo(
    UINT    uCodePage)
{
    int i;

    for (i = 0; i < sizeof(sCountry) / sizeof(COUNTRYSETTING); i++) {
        if (sCountry[i].uCodePage == uCodePage) {
            return(i);
        }
    }

    return (-1);
}

/****************************************/
/*                                      */
/*  IME LINK LISTBOX DIALOG             */
/*                                      */
/****************************************/
INT_PTR CALLBACK
ImeLinkDlgProc(
HWND    hDlg,
UINT   	uMsg,
WPARAM 	wParam,
LPARAM 	lParam)
{
    switch( uMsg){
    case WM_INITDIALOG:
        {
            HWND    hRadicalWnd, hRegWordButton;
            int     cbString;
            UINT    uCode;
            BOOL    bUnicodeMode=FALSE;
#ifdef UNICODE
            UINT    uCodePage, uNativeCode;
            int     i;
#endif
            TCHAR  szTitle[128];
//            LONG   WindowStyle;

//            WindowStyle = GetWindowLong( hDlg, GWL_EXSTYLE);
//            WindowStyle |= WS_EX_CONTEXTHELP;
//            SetWindowLong( hDlg, GWL_EXSTYLE, WindowStyle);

            cbString = GetWindowText(hDlg, szTitle, sizeof(szTitle) /
                sizeof(TCHAR));

            uCode = LOWORD((DWORD)lParam);
            if (HIWORD((DWORD)lParam))
                bUnicodeMode = TRUE;

#ifdef UNICODE
            if (bUnicodeMode){
                uCodePage = GetACP();

                i = CodePageInfo(uCodePage);

                if (uCodePage == UNICODE_CP || i == -1) {
                    wsprintf(&szTitle[cbString], TEXT("%4X"), (UINT)lParam);
                } else {
                    uNativeCode = 0;

                    WideCharToMultiByte(uCodePage, WC_COMPOSITECHECK,
                        (LPCWSTR)&uCode, 1,
                        (LPSTR)&uNativeCode, sizeof(uNativeCode),
                        NULL, NULL);

                    // convert to multi byte string
                    uNativeCode = LOBYTE(uNativeCode) << 8 | HIBYTE(uNativeCode);

                    wsprintf(&szTitle[cbString], TEXT("%4X (%s - %4X)"),
                        (UINT)uCode, sCountry[i].szCodePage, (UINT)uNativeCode);
                }
            }else{
                wsprintf( &szTitle[cbString], TEXT("%4X"), (UINT)uCode);
            }
#else
            wsprintf( &szTitle[cbString], "%4X", (UINT)uCode);
#endif            	
            SetWindowText( hDlg, szTitle);
            hRadicalWnd = GetDlgItem(hDlg, IDD_RADICAL);
            SendMessage( hRadicalWnd, WM_EUDC_CODE, 0, lParam);

            hRegWordButton = GetDlgItem(hDlg, IDOK);
            EnableWindow( hRegWordButton, FALSE);
            SendMessage( hRadicalWnd, WM_EUDC_REGISTER_BUTTON, 0,
                (LPARAM)hRegWordButton);
        }
        return (TRUE);
    case WM_COMMAND:
        switch( wParam){
        case IDOK:
            {
                HWND    hRadicalWnd;

                hRadicalWnd = GetDlgItem(hDlg, IDD_RADICAL);
                if( RegisterThisEudc(hRadicalWnd) == -1){
                    EndDialog(hDlg, TRUE);
                }else SetFocus(hRadicalWnd);
            }
            break;
        case IDCANCEL:
            EndDialog(hDlg, FALSE);
            break;
        default:
            return (FALSE);
        }
        return( TRUE);
    case WM_IME_NOTIFY:
        switch( wParam){
        case IMN_OPENSTATUSWINDOW:
        case IMN_CLOSESTATUSWINDOW:
            return (TRUE);
        default:
            return (FALSE);
        }
    case WM_HELP:
        {/*
            TCHAR HelpPath[MAX_PATH];

            if( !GetSystemWindowsDirectory( HelpPath, MAX_PATH))
                return FALSE;
            lstrcat(HelpPath, TEXT("\\HELP\\EUDCEDIT.HLP"));
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                HelpPath, HELP_WM_HELP, (DWORD_PTR)(LPDWORD)aIds);
         */
        }
        return FALSE;
    case WM_CONTEXTMENU:
        {/*
            TCHAR HelpPath[MAX_PATH];

            if( !GetSystemWindowsDirectory( HelpPath, MAX_PATH))
                return FALSE;
            lstrcat(HelpPath, TEXT("\\HELP\\EUDCEDIT.HLP"));
            WinHelp((HWND)wParam, HelpPath,
                HELP_CONTEXTMENU, (DWORD_PTR)(LPDWORD)aIds);
         */
        }
        return FALSE;
    default:
        return( FALSE);
    }
    return (TRUE);
}

/****************************************/
/*                                      */
/*      COMMAND "IME LINK"              */
/*                                      */
/****************************************/
void
ImeLink(
HWND        hWnd,
UINT        uCode,
BOOL        bUnicodeMode,
HINSTANCE   hInst)
{
    static  BOOL bFirstTime[20];
    int ii;
    
    WNDCLASSEX  wcClass;
    UINT        nLayouts;
    HKL FAR    *lphKL;
    TCHAR       szTitle[32];
    TCHAR       szMessage[256];
    UINT        i, nIMEs;
    HKL         hOldKL;

    for (ii = 0; ii < 20; ii++)
      bFirstTime[ii] = TRUE;

    hAppInst = hInst;

    nLayouts = GetKeyboardLayoutList(0, NULL);

    lphKL = GlobalAlloc(GPTR, sizeof(HKL) * nLayouts);

    if (!lphKL) {
        LoadString(hAppInst, IDS_NOMEM_TITLE, szTitle, sizeof(szTitle) / sizeof(TCHAR));
        LoadString(hAppInst, IDS_NOMEM_MSG, szMessage, sizeof(szMessage) /sizeof(TCHAR));
        MessageBox(hWnd, szMessage, szTitle, MB_OK);
        return;
    }

    GetKeyboardLayoutList(nLayouts, lphKL);
    for (i = 0, nIMEs = 0; i < nLayouts; i++) {
        LRESULT  lRet;
        HKL   hKL;
        TCHAR szImeEudcDic[80];

        hKL = *(lphKL + i);
        lRet = ImmIsIME(hKL);
        if (!lRet) {
            continue;
        }
        szImeEudcDic[0] = '\0';

        lRet = ImmEscape(hKL, (HIMC)NULL, IME_ESC_GET_EUDC_DICTIONARY,
            szImeEudcDic);

        if (!lRet) {
            continue;
        }

        if (szImeEudcDic[0]) {
            lRet = TRUE;
        } 
        else if( !bFirstTime[i]) {
        } 
        else {
            lRet = ImmConfigureIME(hKL, hWnd, IME_CONFIG_SELECTDICTIONARY, NULL);
        }
        if (!lRet) {
            continue;
        }
        else {
          bFirstTime[i] = FALSE;
        }

        if (szImeEudcDic[0] == '\0') {
            lRet = ImmEscape(hKL, (HIMC)NULL, IME_ESC_GET_EUDC_DICTIONARY,
                szImeEudcDic);

            if (!lRet) {
                continue;
            } else if (szImeEudcDic[0] == '\0') {
                continue;
            } else {
            }
        } else {
        }
        nIMEs++;
    }
    GlobalFree((HGLOBAL)lphKL);

    if (!nIMEs) {
        LoadString(hAppInst, IDS_NOIME_TITLE, szTitle, sizeof(szTitle) / sizeof(TCHAR));
        LoadString(hAppInst, IDS_NOIME_MSG, szMessage, sizeof(szMessage) / sizeof(TCHAR));
        MessageBox(hWnd, szMessage, szTitle, MB_OK);
        return;
    }

    if( !GetClassInfoEx( hAppInst, szRegWordCls, &wcClass)){
        wcClass.cbSize = sizeof(WNDCLASSEX);
        wcClass.style = CS_HREDRAW|CS_VREDRAW;
        wcClass.lpfnWndProc = RegWordWndProc;
        wcClass.cbClsExtra = 0;
        wcClass.cbWndExtra = 2 * sizeof(PVOID);
        wcClass.hInstance = hAppInst;
        wcClass.hIcon = NULL;
        wcClass.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wcClass.lpszMenuName = NULL;
        wcClass.lpszClassName = szRegWordCls;
        wcClass.hIconSm = NULL;
        RegisterClassEx( &wcClass);
    }
    hOldKL = GetKeyboardLayout(0);
    DialogBoxParam(hAppInst, szImeLinkDlg, hWnd, ImeLinkDlgProc,
        (LPARAM) MAKELONG(uCode , bUnicodeMode ? 0xffff : 0));
    ActivateKeyboardLayout(hOldKL, 0);
    return;
}
