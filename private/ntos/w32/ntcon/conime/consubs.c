// Copyright (c) 1985 - 1999, Microsoft Corporation
//
//  MODULE:   ConSubs.c
//
//  PURPOSE:   Console IME control.
//
//  PLATFORMS: Windows NT-FE 3.51
//
//  FUNCTIONS:
//
//  History:
//
//  27.Jul.1995 v-HirShi (Hirotoshi Shimizu)    created
//  10.Jul.1996 v-HirShi (Hirotoshi Shimizu)    adupt FE edition
//
//  COMMENTS:
//

#include "precomp.h"
#pragma hdrstop

INT
Create( HWND hWnd )
{
    ghDefaultIMC = ImmGetContext(hWnd) ;

#ifdef DEBUG_MODE
    {
        //
        // Select fixed pitch system font and get its text metrics
        //

        HDC hdc;
        TEXTMETRIC tm;
        WORD       patern = 0xA4A4;
        SIZE       size;
        HFONT hfntFixed;                      // fixed-pitch font
        HFONT hfntOld;                        // default font holder

        hdc = GetDC( hWnd );
        hfntFixed = GetStockObject( SYSTEM_FIXED_FONT );
        hfntOld = SelectObject( hdc, hfntFixed );
        GetTextMetrics( hdc, &tm );

        GetTextExtentPoint32( hdc, (LPWSTR)&patern, sizeof(WORD), (LPSIZE) &size );
        cxMetrics = (UINT) size.cx / 2;
        cyMetrics = (UINT) size.cy;
        ReleaseDC( hWnd, hdc );

        xPos = 0 ;
        CaretWidth = GetSystemMetrics( SM_CXBORDER );
    }
#endif

    return 0;

}

//**********************************************************************
//
// void ImeUIStartComposition()
//
// This handles WM_IME_STARTCOMPOSITION message.
//
//**********************************************************************

void ImeUIStartComposition( HWND hwnd )
{
    PCONSOLE_TABLE ConTbl;

    ConTbl = SearchConsole(LastConsole);
    if (ConTbl == NULL) {
        DBGPRINT(("CONIME: Error! Cannot found registed Console\n"));
        return;
    }

    //
    // Set fInComposition variables.
    //
    ConTbl->fInComposition = TRUE;

#ifdef DEBUG_MODE
    {
        int i ;
        for (i = FIRSTCOL ; i < MAXCOL ; i++) {
            ConvertLine[i] = UNICODE_SPACE ;
            ConvertLineAtr[i] = 0 ;
        }
    }
#endif
#ifdef DEBUG_INFO
    xPos = FIRSTCOL;
    xPosLast = FIRSTCOL;
    HideCaret( hwnd );
    DisplayConvInformation( hwnd ) ;
    ResetCaret( hwnd );
#endif
}

//**********************************************************************
//
// void ImeUIEndComposition
//
// This handles WM_IME_ENDCOMPOSITION message.
//
//**********************************************************************

void ImeUIEndComposition( HWND hwnd )
{
    PCONSOLE_TABLE ConTbl;

    ConTbl = SearchConsole(LastConsole);
    if (ConTbl == NULL) {
        DBGPRINT(("CONIME: Error! Cannot found registed Console\n"));
        return;
    }

    //
    // Reset fInComposition variables.
    //
    ConTbl->fInComposition = FALSE;

    if (ConTbl->lpCompStrMem)
        LocalFree( ConTbl->lpCompStrMem );
    ConTbl->lpCompStrMem = NULL ;

#ifdef DEBUG_MODE
    {
        int i ;
        //
        // Reset the length of composition string to zero.
        //
        for (i = FIRSTCOL ; i < MAXCOL ; i++) {
            ConvertLine[i] = UNICODE_SPACE ;
            ConvertLineAtr[i] = 0 ;
        }
    }
#endif
#ifdef DEBUG_INFO
    xPos = FIRSTCOL;
    xPosLast = FIRSTCOL;
    HideCaret( hwnd );
    DisplayConvInformation( hwnd ) ;
    ResetCaret( hwnd );
#endif
}

//**********************************************************************
//
// void ImeUIComposition()
//
// This handles WM_IME_COMPOSITION message. It here just handles
// composition string and result string. For normal case, it should
// examine all posibile flags indicated by CompFlag, then do some
// actitions to reflect what kinds of composition info. IME conversion
// engine informs.
//
//**********************************************************************

void ImeUIComposition( HWND hwnd, WPARAM CompChar, LPARAM CompFlag )
{

    DBGPRINT(("CONIME: WM_IME_COMPOSITION %08x %08x\n",CompChar,CompFlag));

#ifdef DEBUG_MODE
    {
        int i ;
        for (i = FIRSTCOL ; i < MAXCOL ; i++) {
            ConvertLine[i] = UNICODE_SPACE ;
            ConvertLineAtr[i] = 0 ;
        }
        xPos = FIRSTCOL;
        xPosLast = FIRSTCOL;
    }
#endif

    if ( CompFlag == 0 ) {
        DBGPRINT(("                           None\n"));
        GetCompositionStr( hwnd, CompFlag, CompChar);
    }
    if ( CompFlag & GCS_RESULTSTR ) {
        DBGPRINT(("                           GCS_RESULTSTR\n"));
        GetCompositionStr( hwnd, ( CompFlag & GCS_RESULTSTR ), CompChar );
    }
    if ( CompFlag & GCS_COMPSTR ) {
        DBGPRINT(("                           GCS_COMPSTR\n"));
        GetCompositionStr( hwnd, ( CompFlag & (GCS_COMPSTR|GCS_COMPATTR)), CompChar);
    }
    if ( CompFlag & CS_INSERTCHAR ) {
        DBGPRINT(("                           CS_INSERTCHAR\n"));
        GetCompositionStr( hwnd, ( CompFlag & (CS_INSERTCHAR|GCS_COMPATTR)), CompChar);
    }
    if ( CompFlag & CS_NOMOVECARET ) {
        DBGPRINT(("                           CS_NOMOVECARET\n"));
        GetCompositionStr( hwnd, ( CompFlag & (CS_NOMOVECARET|GCS_COMPATTR)), CompChar);
    }
}


#ifdef DEBUG_INFO
//*********************************************************************
//
// void DisplayCompString()
//
// This displays composition string.
//
// This function send string to Console.
//
//*********************************************************************

void DisplayCompString( HWND hwnd, int Length, PWCHAR CharBuf, PUCHAR AttrBuf )
{
    int         i;
    CopyMemory(ConvertLine, CharBuf, Length * sizeof(WCHAR) ) ;
    if ( AttrBuf == NULL ) {
        for ( i = 0 ; i < Length ; i++ )
            ConvertLineAtr[i] = 0 ;
    }
    else {
        CopyMemory(ConvertLineAtr, AttrBuf, Length) ;
    }
    HideCaret( hwnd );
    DisplayConvInformation( hwnd ) ;
    ResetCaret( hwnd );

}

//*********************************************************************
//
// void DisplayResultString()
//
// This displays result string.
//
// This function supports only fixed pitch font.
//
//*********************************************************************

void DisplayResultString( HWND hwnd, LPWSTR lpwStr )
{

    int         StrLen = lstrlenW( lpwStr );

    CopyMemory(ConvertLine, lpwStr, StrLen*sizeof(WCHAR)) ;
    HideCaret( hwnd );
    DisplayConvInformation( hwnd ) ;
    ResetCaret( hwnd );

    // gImeUIData.uCompLen = 0;

}
#endif

//**********************************************************************
//
// BOOL ImeUINotify()
//
// This handles WM_IME_NOTIFY message.
//
//**********************************************************************

BOOL ImeUINotify( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
    switch (wParam )
    {
        case IMN_OPENSTATUSWINDOW:
            ImeUIOpenStatusWindow(hwnd) ;
            break;
        case IMN_CHANGECANDIDATE:
            ImeUIChangeCandidate( hwnd, (DWORD)lParam );
            break;
        case IMN_CLOSECANDIDATE:
            ImeUICloseCandidate( hwnd, (DWORD)lParam );
            break;
        case IMN_OPENCANDIDATE:
            ImeUIOpenCandidate( hwnd, (DWORD)lParam, TRUE);
            break;
        case IMN_SETCONVERSIONMODE:
            ImeUISetConversionMode(hwnd) ;
            // IMN_SETCONVERSIONMODE should be pass to DefWindowProc
            // becuase ImeNotifyHandler in User32 does notify to shell and keyboard.
            return FALSE;
        case IMN_SETOPENSTATUS:
            ImeUISetOpenStatus( hwnd );
            // IMN_SETOPENSTATUS should be pass to DefWindowProc
            // becuase ImeNotifyHandler in User32 does notify to shell and keyboard.
            return FALSE;
        case IMN_GUIDELINE:
            ImeUIGuideLine(hwnd) ;
            break;
        default:
            return FALSE;

    }
    return TRUE;
}

/***************************************************************************\
* BOOL IsConsoleFullWidth(DWORD CodePage,WCHAR wch)
*
* Determine if the given Unicode char is fullwidth or not.
*
* History:
* 04-08-92 ShunK       Created.
* Jul-27-1992 KazuM    Added Screen Information and Code Page Information.
* Jan-29-1992 V-Hirots Substruct Screen Information.
* Oct-06-1996 KazuM    Not use RtlUnicodeToMultiByteSize and WideCharToMultiByte
*                      Because 950 only defined 13500 chars,
*                      and unicode defined almost 18000 chars.
*                      So there are almost 4000 chars can not be mapped to big5 code.
\***************************************************************************/

BOOL IsUnicodeFullWidth(
    IN WCHAR wch
    )
{
    if (0x20 <= wch && wch <= 0x7e)
        /* ASCII */
        return FALSE;
    else if (0x3041 <= wch && wch <= 0x3094)
        /* Hiragana */
        return TRUE;
    else if (0x30a1 <= wch && wch <= 0x30f6)
        /* Katakana */
        return TRUE;
    else if (0x3105 <= wch && wch <= 0x312c)
        /* Bopomofo */
        return TRUE;
    else if (0x3131 <= wch && wch <= 0x318e)
        /* Hangul Elements */
        return TRUE;
    else if (0xac00 <= wch && wch <= 0xd7a3)
        /* Korean Hangul Syllables */
        return TRUE;
    else if (0xff01 <= wch && wch <= 0xff5e)
        /* Fullwidth ASCII variants */
        return TRUE;
    else if (0xff61 <= wch && wch <= 0xff9f)
        /* Halfwidth Katakana variants */
        return FALSE;
    else if ( (0xffa0 <= wch && wch <= 0xffbe) ||
              (0xffc2 <= wch && wch <= 0xffc7) ||
              (0xffca <= wch && wch <= 0xffcf) ||
              (0xffd2 <= wch && wch <= 0xffd7) ||
              (0xffda <= wch && wch <= 0xffdc)   )
        /* Halfwidth Hangule variants */
        return FALSE;
    else if (0xffe0 <= wch && wch <= 0xffe6)
        /* Fullwidth symbol variants */
        return TRUE;
    else if (0x4e00 <= wch && wch <= 0x9fa5)
        /* Han Ideographic */
        return TRUE;
    else if (0xf900 <= wch && wch <= 0xfa2d)
        /* Han Ideographic Compatibility */
        return TRUE;
    else
    {
#if 0
        /*
         * Hack this block for I don't know FONT of Console Window.
         *
         * If you would like perfect result from IsUnicodeFullWidth routine,
         * then you should enable this block and
         * you should know FONT of Console Window.
         */

        INT Width;
        TEXTMETRIC tmi;

        /* Unknown character */

        GetTextMetricsW(hDC, &tmi);
        if (IS_ANY_DBCS_CHARSET(tmi.tmCharSet))
            tmi.tmMaxCharWidth /= 2;

        GetCharWidth32(hDC, wch, wch, &Width);
        if (Width == tmi.tmMaxCharWidth)
            return FALSE;
        else if (Width == tmi.tmMaxCharWidth*2)
            return TRUE;
#else
        ULONG MultiByteSize;

        RtlUnicodeToMultiByteSize(&MultiByteSize, &wch, sizeof(WCHAR));
        if (MultiByteSize == 2)
            return TRUE ;
        else
            return FALSE ;
#endif
    }
    ASSERT(FALSE);
    return FALSE;
#if 0
    ULONG MultiByteSize;

    RtlUnicodeToMultiByteSize(&MultiByteSize, &wch, sizeof(WCHAR));
    if (MultiByteSize == 2)
        return TRUE ;
    else
        return FALSE ;
#endif
}


BOOL
ImeUIOpenStatusWindow(
    HWND hwnd
    )
{
    PCONSOLE_TABLE ConTbl;
    HIMC        hIMC;                   // Input context handle.
    LPCONIME_UIMODEINFO lpModeInfo ;
    COPYDATASTRUCT CopyData ;

    DBGPRINT(("CONIME: Get IMN_OPENSTATUSWINDOW Message\n"));

    ConTbl = SearchConsole(LastConsole);
    if (ConTbl == NULL) {
        DBGPRINT(("CONIME: Error! Cannot found registed Console\n"));
        return FALSE;
    }

    hIMC = ImmGetContext( hwnd ) ;
    if ( hIMC == 0 )
        return FALSE;

    lpModeInfo = (LPCONIME_UIMODEINFO)LocalAlloc( LPTR, sizeof(CONIME_UIMODEINFO) ) ;
    if ( lpModeInfo == NULL) {
        ImmReleaseContext( hwnd, hIMC );
        return FALSE;
    }

    ImmGetConversionStatus(hIMC,
                           (LPDWORD)&ConTbl->dwConversion,
                           (LPDWORD)&ConTbl->dwSentence) ;

    CopyData.dwData = CI_CONIMEMODEINFO ;
    CopyData.cbData = sizeof(CONIME_UIMODEINFO) ;
    CopyData.lpData = lpModeInfo ;
    if (ImeUIMakeInfoString(ConTbl,
                            lpModeInfo))
    {
        ConsoleImeSendMessage( ConTbl->hWndCon,
                               (WPARAM)hwnd,
                               (LPARAM)&CopyData
                              ) ;
    }

    LocalFree( lpModeInfo );

    ImmReleaseContext( hwnd, hIMC );

    return TRUE ;
}


BOOL
ImeUIChangeCandidate(
   HWND hwnd,
   DWORD lParam
   )
{
    return ImeUIOpenCandidate( hwnd, lParam, FALSE) ;
}


BOOL
ImeUISetOpenStatus(
    HWND hwnd
    )
{
    PCONSOLE_TABLE ConTbl;
    HIMC        hIMC;                   // Input context handle.
    LPCONIME_UIMODEINFO lpModeInfo ;
    COPYDATASTRUCT CopyData ;

    DBGPRINT(("CONIME: Get IMN_SETOPENSTATUS Message\n"));

    ConTbl = SearchConsole(LastConsole);
    if (ConTbl == NULL) {
        DBGPRINT(("CONIME: Error! Cannot found registed Console\n"));
        return FALSE;
    }

    hIMC = ImmGetContext( hwnd ) ;
    if ( hIMC == 0 )
        return FALSE;

    ConTbl->fOpen = GetOpenStatusByCodepage( hIMC, ConTbl ) ;

    ImmGetConversionStatus(hIMC,
                           (LPDWORD)&ConTbl->dwConversion,
                           (LPDWORD)&ConTbl->dwSentence) ;

    if (ConTbl->ScreenBufferSize.X != 0) {

        lpModeInfo = (LPCONIME_UIMODEINFO)LocalAlloc( LPTR, sizeof(CONIME_UIMODEINFO)) ;
        if ( lpModeInfo == NULL) {
            ImmReleaseContext( hwnd, hIMC );
            return FALSE;
        }

        CopyData.dwData = CI_CONIMEMODEINFO ;
        CopyData.cbData = sizeof(CONIME_UIMODEINFO) ;
        CopyData.lpData = lpModeInfo ;
        if (ImeUIMakeInfoString(ConTbl,
                                lpModeInfo))
        {
            ConsoleImeSendMessage( ConTbl->hWndCon,
                                   (WPARAM)hwnd,
                                   (LPARAM)&CopyData
                                  ) ;
        }
        LocalFree( lpModeInfo );
    }
    ImmReleaseContext( hwnd, hIMC );

    return TRUE ;
}

BOOL
ImeUISetConversionMode(
    HWND hwnd
    )
{
    PCONSOLE_TABLE ConTbl;
    HIMC        hIMC;                   // Input context handle.
    LPCONIME_UIMODEINFO lpModeInfo ;
    COPYDATASTRUCT CopyData ;
    DWORD OldConversion ;


    DBGPRINT(("CONIME: Get IMN_SETCONVERSIONMODE Message\n"));

    ConTbl = SearchConsole(LastConsole);
    if (ConTbl == NULL) {
        DBGPRINT(("CONIME: Error! Cannot found registed Console\n"));
        return FALSE;
    }

    hIMC = ImmGetContext( hwnd ) ;
    if ( hIMC == 0 )
        return FALSE;

    lpModeInfo = (LPCONIME_UIMODEINFO)LocalAlloc(LPTR, sizeof(CONIME_UIMODEINFO) ) ;
    if ( lpModeInfo == NULL) {
        ImmReleaseContext( hwnd, hIMC );
        return FALSE;
    }

    OldConversion = ConTbl->dwConversion ;

    ImmGetConversionStatus(hIMC,
                           (LPDWORD)&ConTbl->dwConversion,
                           (LPDWORD)&ConTbl->dwSentence) ;

    CopyData.dwData = CI_CONIMEMODEINFO ;
    CopyData.cbData = sizeof(CONIME_UIMODEINFO) ;
    CopyData.lpData = lpModeInfo ;
    if (ImeUIMakeInfoString(ConTbl,
                            lpModeInfo))
    {
        ConsoleImeSendMessage( ConTbl->hWndCon,
                               (WPARAM)hwnd,
                               (LPARAM)&CopyData
                             ) ;
    }

    LocalFree( lpModeInfo );
    ImmReleaseContext( hwnd, hIMC );
    return TRUE ;

}

BOOL
ImeUIGuideLine(
    HWND hwnd
    )
{
    PCONSOLE_TABLE ConTbl;
    HIMC        hIMC ;                   // Input context handle.
    DWORD       Level ;
    DWORD       Index ;
    DWORD       Length ;
    LPCONIME_UIMESSAGE GuideLine ;
    COPYDATASTRUCT CopyData ;

    DBGPRINT(("CONIME: Get IMN_GUIDELINE Message "));

    ConTbl = SearchConsole(LastConsole);
    if (ConTbl == NULL) {
        DBGPRINT(("CONIME: Error! Cannot found registed Console\n"));
        return FALSE;
    }

    hIMC = ImmGetContext( hwnd ) ;
    if ( hIMC == 0 )
        return FALSE;

    Level = ImmGetGuideLine(hIMC, GGL_LEVEL, NULL, 0) ;
    Index = ImmGetGuideLine(hIMC, GGL_INDEX, NULL, 0) ;
    Length = ImmGetGuideLine(hIMC, GGL_STRING, NULL, 0) ;
    DBGPRINT(("Level=%d Index=%d Length=%d",Level,Index,Length));
    if (Length == 0) {
        CopyData.dwData = CI_CONIMESYSINFO ;
        CopyData.cbData = Length ;
        CopyData.lpData = NULL ;

        ConsoleImeSendMessage( ConTbl->hWndCon,
                               (WPARAM)hwnd,
                               (LPARAM)&CopyData
                              ) ;
    }
    else{
        GuideLine = (LPCONIME_UIMESSAGE)LocalAlloc(LPTR, Length + sizeof(WCHAR)) ;
        if (GuideLine == NULL) {
            ImmReleaseContext( hwnd, hIMC );
            return FALSE;
        }

        CopyData.dwData = CI_CONIMESYSINFO ;
        CopyData.cbData = Length + sizeof(WCHAR) ;
        CopyData.lpData = GuideLine ;
        Length = ImmGetGuideLine(hIMC, GGL_STRING, GuideLine->String, Length) ;

        ConsoleImeSendMessage( ConTbl->hWndCon,
                               (WPARAM)hwnd,
                               (LPARAM)&CopyData
                              ) ;

        LocalFree( GuideLine ) ;
    }
    ImmReleaseContext( hwnd, hIMC );
    DBGPRINT(("\n"));

    return TRUE ;
}

DWORD
GetNLSMode(
    HWND hWnd,
    HANDLE hConsole
    )
{
    PCONSOLE_TABLE ConTbl;
    HIMC hIMC;

    ConTbl = SearchConsole(hConsole);
    if (ConTbl == NULL) {
        DBGPRINT(("CONIME: Error! Cannot found registed Console\n"));
        return 0;
    }

    hIMC = ImmGetContext( hWnd ) ;
    if ( hIMC == (HIMC)NULL )
        return IME_CMODE_DISABLE;

    ImmGetConversionStatus(hIMC,
                           &ConTbl->dwConversion,
                           &ConTbl->dwSentence);
    ConTbl->fOpen = GetOpenStatusByCodepage( hIMC, ConTbl ) ;

    ImmReleaseContext( hWnd, hIMC );


    return ((ConTbl->fOpen ? IME_CMODE_OPEN : 0) + ConTbl->dwConversion);
}

BOOL
SetNLSMode(
    HWND hWnd,
    HANDLE hConsole,
    DWORD fdwConversion
    )
{
    PCONSOLE_TABLE ConTbl;
    HIMC hIMC;

    ConTbl = SearchConsole(hConsole);
    if (ConTbl == NULL) {
        DBGPRINT(("CONIME: Error! Cannot found registed Console\n"));
        return FALSE;
    }

    if (fdwConversion & IME_CMODE_DISABLE)
    {
        ImmSetActiveContextConsoleIME(hWnd, FALSE) ;
        ImmAssociateContext(hWnd, (HIMC)NULL);
        ConTbl->hIMC_Current = (HIMC)NULL;
    }
    else
    {
        ImmAssociateContext(hWnd, ConTbl->hIMC_Original);
        ImmSetActiveContextConsoleIME(hWnd, TRUE) ;
        ConTbl->hIMC_Current = ConTbl->hIMC_Original;
    }

    hIMC = ImmGetContext( hWnd ) ;
    if ( hIMC == (HIMC)NULL )
        return TRUE;

    ConTbl->fOpen =(fdwConversion & IME_CMODE_OPEN) ? TRUE : FALSE ;
    ImmSetOpenStatus(hIMC, ConTbl->fOpen);

    fdwConversion &= ~(IME_CMODE_DISABLE | IME_CMODE_OPEN);
    if (ConTbl->dwConversion != fdwConversion)
    {
        ConTbl->dwConversion = fdwConversion;
        ImmSetConversionStatus(hIMC,
                               ConTbl->dwConversion,
                               ConTbl->dwSentence );
    }

    ImmReleaseContext( hWnd, hIMC );

    return TRUE;
}

BOOL
ConsoleCodepageChange(
    HWND hWnd,
    HANDLE hConsole,
    BOOL Output,
    WORD CodePage
    )
{
    PCONSOLE_TABLE ConTbl;

    ConTbl = SearchConsole(hConsole);
    if (ConTbl == NULL) {
        DBGPRINT(("CONIME: Error! Cannot found registed Console\n"));
        return FALSE;
    }

    if (Output)
    {
        ConTbl->ConsoleOutputCP = CodePage ;
    }
    else
    {
        ConTbl->ConsoleCP = CodePage ;
    }
    return (TRUE) ;
}

BOOL
ImeSysPropertyWindow(
    HWND hWnd,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PCONSOLE_TABLE ConTbl;
    COPYDATASTRUCT CopyData;

    ConTbl = SearchConsole(LastConsole);
    if (ConTbl == NULL) {
        DBGPRINT(("CONIME: Error! Cannot found registed Console\n"));
        return FALSE;
    }

    CopyData.dwData = CI_CONIMEPROPERTYINFO;
    CopyData.cbData = sizeof(WPARAM);
    CopyData.lpData = &wParam;

    ConsoleImeSendMessage( ConTbl->hWndCon,
                           (WPARAM)hWnd,
                           (LPARAM)&CopyData
                          );

    return TRUE;
}
