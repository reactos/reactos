/****************************************************************************\
*
*     PROGRAM: fontview.c
*
*     PURPOSE: Loads and displays fonts from the given filename
*
*     COMMENTS:
*
*     HISTORY:
*       02-Oct-1995 JonPa       Created It
*
\****************************************************************************/

#include <windows.h>                /* required for all Windows applications */
#include <commdlg.h>
#include <shellapi.h>
#ifdef WINNT
#   include <wingdip.h>             /* prototype for GetFontRsourceInfo     */
#endif
#include <objbase.h>
#include "fontdefs.h"               /* specific to this program             */
#include "fvmsg.h"
#include "fvrc.h"
#include "ttdefs.h"



HANDLE hInst;                       /* current instance                     */
HWND ghwndView = NULL;
HWND ghwndFrame = NULL;
BOOL    gfPrint = FALSE;
TCHAR   gszFontPath[2 * MAX_PATH];
LPTSTR  gpszSampleText;
LPTSTR  gpszSampleAlph[3];
FFTYPE  gfftFontType;
LOGFONT glfFont;
DISPTEXT gdtDisplay;
HBRUSH  ghbr3DFace;
HBRUSH  ghbr3DShadow;


int gyScroll = 0;              // Vertical scroll offset in pels
int gcyLine = 0;

int gcxMinWinSize = CX_MIN_WINSIZE;
int gcyMinWinSize = CY_MIN_WINSIZE;

BOOL gbIsDBCS = FALSE;    // Indicates whether system default langID is DBCS
int  gNumOfFonts = 0;     // number of fonts in the file.
int  gIndexOfFonts = 0;   // current index of the fonts.
LPLOGFONT glpLogFonts;    // get global data by GetFontResourceInfo()

int apts[] = { 12, 18, 24, 36, 48, 60, 72 };
#define C_POINTS_LIST  (sizeof(apts) / sizeof(apts[0]))

#define CPTS_BTN_AREA   28
int gcyBtnArea = CPTS_BTN_AREA;
BTNREC gabtCmdBtns[] = {
    {   6,  6, 36, 16, IDB_DONE,      NULL, MSG_DONE,      NULL },
    {  -6,  6, 36, 16, IDB_PRINT,     NULL, MSG_PRINT,     NULL },
    {  68,  6, 20, 16, IDB_PREV_FONT, NULL, MSG_PREV_FONT, NULL }, // DBCS only.
    { -68,  6, 20, 16, IDB_NEXT_FONT, NULL, MSG_NEXT_FONT, NULL }  // DBCS only.
};

#define C_DBCSBUTTONS  2  // Prev & Next font are DBCS specific.
//
// This may be recalculated in WinMain to adjust for a DBCS locale.
//
int C_BUTTONS = (sizeof(gabtCmdBtns) / sizeof(gabtCmdBtns[0]));


#if DBG
void DDPrint( LPTSTR sz, DWORD dw ) {
    TCHAR szBuff[246];
    wsprintf( szBuff, sz, dw );

    OutputDebugString( szBuff );
}

#   define DDPRINT( s, d )  DDPrint( s, d )
#else
#   define DDPRINT( s, d )
#endif


#define IsZeroFSig( fs )  ( (fs)->fsUsb[0] == 0 && (fs)->fsUsb[1] == 0 && (fs)->fsUsb[2] == 0 && \
                                (fs)->fsUsb[3] == 0 && (fs)->fsCsb[0] == 0 && (fs)->fsCsb[1] == 0 )

BOOL NativeCodePageSupported(LPLOGFONT lplf) {
    HDC hdc = CreateCompatibleDC(NULL);
    HFONT hf, hfOld;
    FONTSIGNATURE fsig;
    CHARSETINFO  csi;
    BOOL fRet;

    DDPRINT( TEXT("System default code page: %d\n"), GetACP() );

    TranslateCharsetInfo( (LPDWORD)GetACP(), &csi, TCI_SRCCODEPAGE );

    hf = CreateFontIndirect( lplf );

    hfOld = SelectObject( hdc, hf );

    GetTextCharsetInfo( hdc, &fsig, 0 );

    SelectObject( hdc, hfOld );

    DeleteObject(hf);

    if (IsZeroFSig( &fsig ) ) {
        // Font does not support GetTextCharsetInfo(), just go off of the lfCharSet value

        DDPRINT( TEXT("Font does not support GetTextCharsetInfo... \nTesting %d (font cs) against"), lplf->lfCharSet );
        DDPRINT( TEXT("%d (sys charset)\n"), csi.ciCharset );

        fRet = (lplf->lfCharSet == csi.ciCharset);

    } else {
        DDPRINT( TEXT("GTCI() worked...\nChecking font charset bits %08x"),  fsig.fsCsb[0] );
        DDPRINT( TEXT(" %08x against"),  fsig.fsCsb[1] );
        DDPRINT( TEXT(" system charset bits %08x "), csi.fs.fsCsb[0] );
        DDPRINT( TEXT("  %08x\n"), csi.fs.fsCsb[1] );

        fRet = ((csi.fs.fsCsb[0] &  fsig.fsCsb[0]) || (csi.fs.fsCsb[1] &  fsig.fsCsb[1]));
    }

    DeleteDC(hdc);

    return fRet;
}

/****************************************************************************
*
*     FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)
*
*     PURPOSE: calls initialization function, processes message loop
*
*
\****************************************************************************/
int APIENTRY WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpstrCmdLine,
    int nCmdShow
    )
{
    int i, iCpts;
    MSG msg;
    HACCEL  hAccel;
    HICON hIcon;
    USHORT wLanguageId;
    BOOL bCoInitialized = FALSE;

    //
    // Initialize the gbIsDBCS flag based on the current default language.
    //
    wLanguageId = LANGIDFROMLCID(GetThreadLocale());

    gbIsDBCS    = (LANG_JAPANESE == PRIMARYLANGID(wLanguageId)) ||
                  (LANG_KOREAN   == PRIMARYLANGID(wLanguageId)) ||
                  (LANG_CHINESE  == PRIMARYLANGID(wLanguageId));

    //
    // In a DBCS locale, exclude the Prev-Next font buttons.
    //
    if (!gbIsDBCS)
         C_BUTTONS -= C_DBCSBUTTONS;
    //
    // Need to initialize COM so that SHGetFileInfo will load the IExtractIcon handler
    // implemented in fontext.dll.
    //
    if (SUCCEEDED(CoInitialize(NULL)))
        bCoInitialized = TRUE;

    /*
     * Parse the Command Line
     *
     *  Use GetCommandLine() here (instead of lpstrCmdLine) so the
     *  command string will be in Unicode on NT
     */
    FillMemory( &gdtDisplay, sizeof(gdtDisplay), 0 );

    if (!ParseCommand( GetCommandLine(), gszFontPath, &gfPrint ) ||
        (gfftFontType = LoadFontFile( gszFontPath, &gdtDisplay, &hIcon )) == FFT_BAD_FILE) {

        // Bad font file, inform user, and exit

        FmtMessageBox( NULL, MSG_APP_TITLE, NULL, MB_OK | MB_ICONSTOP,
                FALSE, MSG_BADFILENAME, gszFontPath );

        if (bCoInitialized)
            CoUninitialize();

        ExitProcess(1);
    }

    /*
     * Now finish initializing the display structure
     */
    gpszSampleAlph[0] = FmtSprintf(MSG_SAMPLEALPH_0);
    gpszSampleAlph[1] = FmtSprintf(MSG_SAMPLEALPH_1);
    gpszSampleAlph[2] = FmtSprintf(MSG_SAMPLEALPH_2);

    // find next line on display
    for( i = 0; i < CLINES_DISPLAY; i++ ) {
        if (gdtDisplay.atlDsp[i].dtyp == DTP_UNUSED)
            break;
    }

    // fill in sample alphabet
    gdtDisplay.atlDsp[i].pszText = gpszSampleAlph[0];
    gdtDisplay.atlDsp[i].cchText = lstrlen(gpszSampleAlph[0]);
    gdtDisplay.atlDsp[i].dtyp    = DTP_SHRINKTEXT;
    gdtDisplay.atlDsp[i].cptsSize = CPTS_SAMPLE_ALPHA;

    i++;
    gdtDisplay.atlDsp[i] = gdtDisplay.atlDsp[i-1];
    gdtDisplay.atlDsp[i].pszText = gpszSampleAlph[1];
    gdtDisplay.atlDsp[i].cchText = lstrlen(gpszSampleAlph[1]);

    i++;
    gdtDisplay.atlDsp[i] = gdtDisplay.atlDsp[i-1];
    gdtDisplay.atlDsp[i].pszText = gpszSampleAlph[2];
    gdtDisplay.atlDsp[i].cchText = lstrlen(gpszSampleAlph[2]);
    gdtDisplay.atlDsp[i].fLineUnder = TRUE;


    // now fill in sample Sentences
    iCpts = 0;

    if (gbIsDBCS)
    {
        //
        // Determine with string to use: the default or the language
        // specific.
        //
        switch (gdtDisplay.lfTestFont.lfCharSet) {
            case SYMBOL_CHARSET:
            case ANSI_CHARSET:
            case DEFAULT_CHARSET:
            case OEM_CHARSET:
                gpszSampleText = FmtSprintf(MSG_SAMPLETEXT);
                break;

            default:
                gpszSampleText = FmtSprintf(MSG_SAMPLETEXT_ALT);
                break;
        }
    }
    else
    {
        if(NativeCodePageSupported(&(gdtDisplay.lfTestFont))) {
            //
            // Native code page is supported, select that codepage
            // and print the localized string.
            //
            CHARSETINFO csi;

            TranslateCharsetInfo( (LPDWORD)GetACP(), &csi, TCI_SRCCODEPAGE );

            gdtDisplay.lfTestFont.lfCharSet = (BYTE)csi.ciCharset;

            gpszSampleText =  FmtSprintf(MSG_SAMPLETEXT);

        } else {
            //
            // Font does not support the local code page.  Print
            // a random string up instead using the font's default charset.
            //
            gpszSampleText =  FmtSprintf(MSG_ALTSAMPLE);
        }
    }

    for( i += 1; i < CLINES_DISPLAY && iCpts < C_POINTS_LIST; i++ ) {
        if (gdtDisplay.atlDsp[i].dtyp == DTP_UNUSED) {
            gdtDisplay.atlDsp[i].pszText = gpszSampleText;
            gdtDisplay.atlDsp[i].cchText = lstrlen(gpszSampleText);
            gdtDisplay.atlDsp[i].dtyp    = DTP_TEXTOUT;
            gdtDisplay.atlDsp[i].cptsSize = apts[iCpts++];
        }
    }

    /*
     * Init the title font LOGFONT, and other variables
     */
    InitGlobals();

    if (!hPrevInstance) {
        if (!InitApplication(hInstance, hIcon)) {
            msg.wParam = FALSE;
            goto ExitProg;
        }
    }

    /* Perform initializations that apply to a specific instance */

    if (!InitInstance(hInstance, nCmdShow, gdtDisplay.atlDsp[0].pszText)) {
        msg.wParam = FALSE;
        goto ExitProg;
    }

    /* Acquire and dispatch messages until a WM_QUIT message is received. */
    hAccel = LoadAccelerators(hInstance, TEXT("fviewAccel"));

    while (GetMessage(&msg, NULL, 0L, 0L)) {
        if (!TranslateAccelerator(ghwndView, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

ExitProg:
    for ( i = 0; i < C_BUTTONS; i++ )
        FmtFree( gabtCmdBtns[i].pszText );

    if (gbIsDBCS && glpLogFonts)
        FreeMem(glpLogFonts);

    RemoveFontResource( gszFontPath );

    if (bCoInitialized)
        CoUninitialize();

    return (int)(msg.wParam);
}


/****************************************************************************
*
*     FUNCTION: InitApplication(HANDLE)
*
*     PURPOSE: Initializes window data and registers window class
*
*     COMMENTS:
*
*         This function is called at initialization time only if no other
*         instances of the application are running.  This function performs
*         initialization tasks that can be done once for any number of running
*         instances.
*
*         In this case, we initialize a window class by filling out a data
*         structure of type WNDCLASS and calling the Windows RegisterClass()
*         function.  Since all instances of this application use the same window
*         class, we only need to do this when the first instance is initialized.
*
*
\****************************************************************************/

BOOL InitApplication(HANDLE hInstance, HICON hIcon)       /* current instance             */
{
    WNDCLASS  wc;
    BOOL fRet = FALSE;

    /* Fill in window class structure with parameters that describe the       */
    /* main window.                                                           */

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = FrameWndProc;

    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;           /* Application that owns the class.   */
    wc.hIcon = hIcon ? hIcon : LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = ghbr3DFace;
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = TEXT("FontViewWClass");

    /* Register the window class and return success/failure code. */

    if (RegisterClass(&wc)) {
        /* Fill in window class structure with parameters that describe the       */
        /* main window.                                                           */

        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = ViewWndProc;

        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hInstance;           /* Application that owns the class.   */
        wc.hIcon = NULL;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = GetStockObject(WHITE_BRUSH);
        wc.lpszMenuName =  NULL;
        wc.lpszClassName = TEXT("FontDisplayClass");

        fRet = RegisterClass(&wc);
    }

    return fRet;
}


/****************************************************************************
*
*     FUNCTION:  InitInstance(HANDLE, int)
*
*     PURPOSE:  Saves instance handle and creates main window
*
*     COMMENTS:
*
*         This function is called at initialization time for every instance of
*         this application.  This function performs initialization tasks that
*         cannot be shared by multiple instances.
*
*         In this case, we save the instance handle in a static variable and
*         create and display the main program window.
*
\****************************************************************************/

BOOL InitInstance( HANDLE  hInstance, int nCmdShow, LPTSTR  pszTitle)
{

    /* Save the instance handle in static variable, which will be used in  */
    /* many subsequence calls from this application to Windows.            */

    hInst = hInstance;

    /* Create a main window for this application instance.  */

    ghwndFrame = CreateWindow( TEXT("FontViewWClass"), pszTitle,
            WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL );

    /* If window could not be created, return "failure" */

    if (!ghwndFrame)
        return (FALSE);

    return (TRUE);               /* Returns the value from PostQuitMessage */

}

/****************************************************************************
*
*     FUNCTION: InitLogFont
*
\****************************************************************************/
void InitGlobals( void ) {
    TCHAR szMsShellDlg2[LF_FACESIZE];
    INT cyDPI,i, cxFiller, cxMaxTxt, cxTxt, cxMax;
    HDC hdc;
    HFONT hfOld;
    RECT rc;

    FillMemory( &glfFont, sizeof(glfFont), 0 );

    glfFont.lfCharSet         = DEFAULT_CHARSET;
    glfFont.lfOutPrecision    = OUT_DEFAULT_PRECIS;
    glfFont.lfClipPrecision   = CLIP_DEFAULT_PRECIS;
    glfFont.lfQuality         = DEFAULT_QUALITY;
    glfFont.lfPitchAndFamily  = DEFAULT_PITCH | FF_DONTCARE;

    if (LoadString(hInst, IDS_FONTFACE, szMsShellDlg2, sizeof(szMsShellDlg2)))
        lstrcpy(glfFont.lfFaceName, szMsShellDlg2);
    else
        lstrcpy(glfFont.lfFaceName, TEXT("MS Shell Dlg2"));

    hdc = CreateCompatibleDC(NULL);
    cyDPI = GetDeviceCaps(hdc, LOGPIXELSY );

    hfOld = SelectObject( hdc, GetStockObject(DEFAULT_GUI_FONT));

    // Find out size of padding around text
    SetRect(&rc, 0, 0, 0, 0 );
    DrawText(hdc, TEXT("####"), -1, &rc, DT_CALCRECT | DT_CENTER);
    cxFiller = rc.right - rc.left;

    gcyBtnArea = MulDiv( gcyBtnArea, cyDPI, C_PTS_PER_INCH );
    cxMax = cxMaxTxt = 0;
    for( i = 0; i < C_BUTTONS; i++ ) {
        gabtCmdBtns[i].x  = MulDiv( gabtCmdBtns[i].x,  cyDPI, C_PTS_PER_INCH );
        gabtCmdBtns[i].y  = MulDiv( gabtCmdBtns[i].y,  cyDPI, C_PTS_PER_INCH );
        gabtCmdBtns[i].cx = MulDiv( gabtCmdBtns[i].cx, cyDPI, C_PTS_PER_INCH );
        gabtCmdBtns[i].cy = MulDiv( gabtCmdBtns[i].cy, cyDPI, C_PTS_PER_INCH );

        if (gabtCmdBtns[i].cx > cxMax)
            cxMax = gabtCmdBtns[i].cx;

        gabtCmdBtns[i].pszText = FmtSprintf( gabtCmdBtns[i].idText );
        SetRect(&rc, 0, 0, 0, 0 );
        DrawText(hdc, gabtCmdBtns[i].pszText, -1, &rc, DT_CALCRECT | DT_CENTER);

        cxTxt = rc.right - rc.left + cxFiller;

        if (cxMaxTxt < cxTxt) {
            cxMaxTxt = cxTxt;
        }
    }

    //
    // Make sure buttons are big enough for text! (So localizer's won't have
    // to change code.
    //
    if (cxMax < cxMaxTxt) {
        for( i = 0; i < C_BUTTONS; i++ ) {
            gabtCmdBtns[i].cx = gabtCmdBtns[i].cx * cxMaxTxt / cxMax;
        }
    }

    //
    // Make sure buttons don't overlap
    //
    i = C_BUTTONS - 1;
    cxMax = gabtCmdBtns[0].x + gabtCmdBtns[0].cx + gabtCmdBtns[0].x + gabtCmdBtns[i].cx + (-gabtCmdBtns[i].x) +
            (2 * GetSystemMetrics(SM_CXSIZEFRAME));

    if (cxMax > gcxMinWinSize)
        gcxMinWinSize = cxMax;

    SelectObject(hdc, hfOld);
    DeleteDC(hdc);

    gcyLine = MulDiv( CPTS_INFO_SIZE, cyDPI, C_PTS_PER_INCH );

    ghbr3DFace   = GetSysColorBrush(COLOR_3DFACE);
    ghbr3DShadow = GetSysColorBrush(COLOR_3DSHADOW);
}

/****************************************************************************
*
*     FUNCTION: SkipWhiteSpace
*
\****************************************************************************/
LPTSTR SkipWhiteSpace( LPTSTR psz ) {

    while( *psz == TEXT(' ') || *psz == TEXT('\t') || *psz == TEXT('\n') ) {
        psz = CharNext( psz );
    }

    return psz;
}


/****************************************************************************
*
*     FUNCTION: CloneString
*
\****************************************************************************/
LPTSTR CloneString(LPTSTR psz) {
    int cch;
    LPTSTR pszRet;
    cch = (lstrlen( psz ) + 1) * sizeof(TCHAR);

    pszRet = AllocMem(cch);
    lstrcpy( pszRet, psz );
    return pszRet;
}


/****************************************************************************
*
*     FUNCTION: GetFileSizeFromName(pszFontPath)
*
\****************************************************************************/
DWORD GetFileSizeFromName( LPCTSTR pszPath ) {
    HANDLE hfile;
    DWORD cb = 0;

    hfile = CreateFile( pszPath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );
    if (hfile != INVALID_HANDLE_VALUE) {
        cb = GetFileSize( hfile, NULL );
        CloseHandle(hfile);
    }

    return cb;
}



BOOL  bFindPfb (
    TCHAR *pszPFM,
    TCHAR *achPFB
);



/****************************************************************************
*
*     FUNCTION: ParseCommand
*
\****************************************************************************/


BOOL ParseCommand( LPTSTR lpstrCmdLine, LPTSTR pszFontPath, BOOL *pfPrint ) {
    LPTSTR psz;
    BOOL fInQuote = FALSE;
    DWORD cwc;
    TCHAR achPfmPfb[2*MAX_PATH];

    //
    // Skip program name
    //
    for( psz = SkipWhiteSpace(lpstrCmdLine);
            *psz != TEXT('\0') && (fInQuote || *psz != TEXT(' ')); psz = CharNext(psz) ) {

        if (*psz == TEXT('\"')) {
            fInQuote = !fInQuote;
        }
    }

    if (*psz == TEXT('\0')) {
        *pszFontPath = TEXT('\0');
        return FALSE;
    }

    psz = SkipWhiteSpace(psz);

    //
    // Check for "/p"
    //
    if (psz[0] == TEXT('/') && (psz[1] == TEXT('p') || psz[1] == TEXT('P'))) {
        *pfPrint = TRUE;
        psz += 2;           // DBCS OK since we already verified that the
                            // chars were '/' and 'p', they can't be lead bytes
    } else
        *pfPrint = FALSE;

    psz = SkipWhiteSpace(psz);

    cwc = lstrlen(psz) + 1;
    if ((cwc >= 5) && !lstrcmpi(&psz[cwc - 5], TEXT(".PFM")))
    {
        lstrcpy(achPfmPfb, psz);

        if (bFindPfb(achPfmPfb, &achPfmPfb[cwc]))
        {
        // go and concatenate:

            achPfmPfb[cwc-1] = TEXT('|');
            psz = achPfmPfb;
        }
    }

    lstrcpy( pszFontPath, psz );
    return *psz != TEXT('\0');
}



/****************************************************************************
*
*     FUNCTION: GetGDILangID
*
*   REVIEW!  I believe this is how GDI determines the LangID, verify on
*   international builds.
*
\****************************************************************************/
WORD   GetGDILangID() {
    return (WORD)GetSystemDefaultLangID();
}



/****************************************************************************
*
*     FUNCTION: GetAlignedTTName
*
*   NOTE: This function returns an allocated string that must be freed
*   after use.
*
*   This function allocs a buffer to recopy the string into incase we are
*   running on a RISC machine with NT.  Since the string will be UNICODE
*   (ie. each char is a WORD), those strings must be aligned on WORD
*   boundaries.  Unfortunatly, TrueType files do not neccesarily align
*   the embedded unicode strings.  Furthur more, on NT we can not simply
*   return a pointer to the data stored in the input buffer, since the
*   'Unicode' strings stored in the TTF file are stored in Motorola (big
*   endian) format, and we need the unicode chars in Intel (little endian)
*   format. Last but not least, we need the returned string to be null terminated
*   so we need to either alloc the buffer for that case anyway.
*
\****************************************************************************/
#ifdef UNICODE
void ConvertTTStrToWinZStr( LPWSTR pwsz, LPVOID pvTTS, int cbMW ) {
    int i, cch;
    LPMWORD lpmw = pvTTS;

    cch = cbMW / sizeof(MWORD);

    for( i = 0; i < cch; i++ ) {
        *pwsz++ = MWORD2INT(*lpmw);
        lpmw++;
    }

    *pwsz = L'\0';
}
#else
#pragma error("write ANSI code for this" )
    Since apparently TTF files ONLY have Unicode strings, (or MacANSI)
    we need to:

        1. Convert Motorola format Unicode to Intel format unicode
        2. Alloc a buffer for the ANSI string
        3. Call WideCharToMultiByte to covert the string to ANSI

#endif




VOID ConvertDBCSTTStrToWinZStr( LPTSTR pwsz, LPCSTR pvTTS, ULONG cbMW ) {
    BYTE Name[256];
    WORD wordChar;
    BYTE *ansiName = Name;
    WORD *srcString = (WORD *)pvTTS;
    int length = 0;
    int cb = cbMW;

    for(;cb;cb-=2) {
        wordChar = *srcString;
        if(wordChar & 0x00FF) {
            *ansiName++ = (CHAR)((wordChar & 0x00FF));
            *ansiName++ = (CHAR)((wordChar & 0xFF00) >> 8);
            length += 2;
        } else {
            *ansiName++ = (CHAR)((wordChar & 0xFF00) >> 8);
            length++;
        }
        srcString++;
    }

    ansiName[length] = '\0';

#ifdef UNICODE
    MultiByteToWideChar(CP_ACP,0,Name,length,pwsz,cbMW);
#else
    lstrcpy(pwsz,ansiName);
#endif // UNICODE
}

/****************************************************************************
*
*     FUNCTION: FindNameString
*
*   helper function for GetAlignedTTName
*
\****************************************************************************/
LPTSTR FindNameString(PBYTE pbTTData, int cNameRec, int idName, WORD wLangID)
{
    PTTNAMETBL ptnt;
    PTTNAMEREC ptnr;
    LPTSTR     psz;
    int        i;

    ptnt = (PTTNAMETBL)pbTTData;

    for( i = 0; i < cNameRec; i++ ) {
        LPVOID pvTTStr;

        ptnr = &(ptnt->anrNames[i]);
        if (MWORD2INT(ptnr->mwidPlatform) != TTID_PLATFORM_MS ||
            MWORD2INT(ptnr->mwidName) != idName               ||
            MWORD2INT(ptnr->mwidLang) != wLangID) {
            continue;
        }

        pvTTStr = (LPVOID)(pbTTData + MWORD2INT(ptnt->mwoffStrings)
                                    + MWORD2INT(ptnr->mwoffString));

        psz = AllocMem((MWORD2INT(ptnr->mwcbString) + sizeof(TEXT('\0'))) * 2);

        if ((MWORD2INT(ptnr->mwidEncoding) == TTID_MS_GB) ||
            (MWORD2INT(ptnr->mwidEncoding) == TTID_MS_WANSUNG) ||
            (MWORD2INT(ptnr->mwidEncoding) == TTID_MS_BIG5)) {
            ConvertDBCSTTStrToWinZStr( psz, pvTTStr, MWORD2INT(ptnr->mwcbString) );
        } else {
            ConvertTTStrToWinZStr( psz, pvTTStr, MWORD2INT(ptnr->mwcbString) );
        }

        return psz;
    }

    return NULL;
}



LPTSTR GetAlignedTTName( PBYTE pbTTData, int idName ) {
    PTTNAMEREC ptnr;
    PTTNAMETBL ptnt;
    int cNameRec,i;
    LPTSTR psz;
    BOOL bFirstRetry;
    WORD wLangID = GetGDILangID();
    LCID lcid = GetThreadLocale();

    ptnt = (PTTNAMETBL)pbTTData;
    cNameRec = MWORD2INT(ptnt->mwcNameRec);

    //
    // Look For Microsoft Platform ID's
    //
    if (gbIsDBCS)
    {
        if ((psz = FindNameString(pbTTData, cNameRec, idName, wLangID)) != NULL) {
            return psz;
        }
        //
        // If we didn't find it, try English if we haven't already.
        //
        if ( wLangID != 0x0409 ) {
            if ((psz = FindNameString(pbTTData, cNameRec, idName, 0x0409)) != NULL) {
                return psz;
            }
        }
    }
    else
    {
        bFirstRetry = TRUE;

retry_lang:

        for( i = 0; i < cNameRec; i++ ) {
            LPVOID pvTTStr;
            ptnr = &(ptnt->anrNames[i]);
            if (MWORD2INT(ptnr->mwidPlatform) != TTID_PLATFORM_MS ||
                MWORD2INT(ptnr->mwidName) != idName               ||
                MWORD2INT(ptnr->mwidLang) != wLangID) {
                continue;
            }

            pvTTStr = (LPVOID)(pbTTData + MWORD2INT(ptnt->mwoffStrings) + MWORD2INT(ptnr->mwoffString));
            psz = AllocMem(MWORD2INT(ptnr->mwcbString) + sizeof(TEXT('\0')));

            ConvertTTStrToWinZStr( psz, pvTTStr, MWORD2INT(ptnr->mwcbString) );
            return psz;
        }

        //
        // Give 0x409 a try if there is no specified MAC language.
        //
        if (bFirstRetry && wLangID != 0x0409) {
            bFirstRetry = FALSE;
            wLangID     = 0x0409;
            goto retry_lang;
        }
    }

    //
    // Didn't find MS Platform, try Macintosh
    //
    for( i = 0; i < cNameRec; i++ ) {
        int cch;
        LPSTR pszMacStr;

        ptnr = &(ptnt->anrNames[i]);
        if (MWORD2INT(ptnr->mwidPlatform) != TTID_PLATFORM_MAC ||
            MWORD2INT(ptnr->mwidName) != idName                ||
            MWORD2INT(ptnr->mwidLang) != wLangID) {
            continue;
        }

        pszMacStr = (LPVOID)(pbTTData + MWORD2INT(ptnt->mwoffStrings) + MWORD2INT(ptnr->mwoffString));

        cch = MultiByteToWideChar(CP_MACCP, 0, pszMacStr, MWORD2INT(ptnr->mwcbString), NULL, 0);
        if (cch == 0)
            continue;

        cch += 1; // for null
        psz = AllocMem(cch * sizeof(TCHAR));
        if (psz == NULL)
            continue;

        cch = MultiByteToWideChar(CP_MACCP, 0, pszMacStr, MWORD2INT(ptnr->mwcbString), psz, cch);
        if (cch == 0) {
            FreeMem(psz);
            continue;
        }

        return psz;
    }

    //
    // Didn't find MS Platform nor Macintosh
    // 1. Try change Thread Locale to data Locale
    // 2. MultiByteToWideChar with Thread code page CP_THREAD_ACP
    //
    for( i = 0; i < cNameRec; i++ ) {
        int cch;
        LPSTR pszStr;

        ptnr = &(ptnt->anrNames[i]);
        if (MWORD2INT(ptnr->mwidName) != idName ||
            MWORD2INT(ptnr->mwidLang) == 0) {
            continue;
        }

        if (LANGIDFROMLCID(lcid) != MWORD2INT(ptnr->mwidLang)) {
            lcid = MAKELCID(MWORD2INT(ptnr->mwidLang), SORT_DEFAULT);
            if (!SetThreadLocale(lcid)) {
                break;
            }
        }

        pszStr = (LPVOID)(pbTTData + MWORD2INT(ptnt->mwoffStrings) + MWORD2INT(ptnr->mwoffString));

        cch = MultiByteToWideChar(CP_THREAD_ACP, 0, pszStr, MWORD2INT(ptnr->mwcbString), NULL, 0);
        if (cch == 0)
            continue;

        cch += 1; // for null
        psz = AllocMem(cch * sizeof(TCHAR));
        if (psz == NULL)
            continue;

        cch = MultiByteToWideChar(CP_THREAD_ACP, 0, pszStr, MWORD2INT(ptnr->mwcbString), psz, cch);
        if (cch == 0) {
            FreeMem(psz);
            continue;
        }

        return psz;
    }

    return NULL;
}


/****************************************************************************
*
*     FUNCTION: LoadFontFile
*
\****************************************************************************/
#ifndef WINNT
FFTYPE LoadFontFile( LPTSTR pszFontPath, PDISPTEXT pdtSmpl, HICON *phIcon ) {
    HANDLE hfile;
    FFTYPE fft;
    LPTSTR pszFName;

    /*
     * Open the file
     */

    hfile = CreateFile( pszFontPath, GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );

    if (hfile == INVALID_HANDLE_VALUE)
        return FFT_BAD_FILE;

    /*
     * Now determine the file type
     */
    fDone = FALSE;

    for(fft = FFT_TRUETYPE; !fDone && fft < FFT_BAD_FILE; fft++ ) {
        fDone = (afnGetFaceName[fft])( hfile, PTXTLN ptlList );
    }

    CloseHandle( hfile );

    /*
     * if type is not bad
     *   addfontresource( filename )
     */
    *phIcon = NULL;

    if ( fft != FFT_BAD_FILE ) {
        AddFontResource( pszFontPath );
        if(SHGetFileInfo( pszFontPath, 0, &sfi, sizeof(sfi), SHGFI_ICON )) {
            *phIcon = sfi.hIcon;
        }
    }

    return fft;
}
#else

FFTYPE LoadFontFile( LPTSTR pszFontPath, PDISPTEXT pdtSmpl, HICON *phIcon ) {
    int cFonts;
    FFTYPE fft = FFT_BAD_FILE;
    SHFILEINFO sfi;
    LPTSTR pszAdobe;
    TCHAR szFPBuf[MAX_PATH];

    cFonts = AddFontResource( pszFontPath );

    if (gbIsDBCS)
    {
        //
        // save cFonts value to global variable.
        //
        gNumOfFonts = cFonts;
    }

    if (cFonts != 0) {
        LPLOGFONT lplf;
        DWORD cb;
        DWORD cbCFF = 0, cbMMSD = 0, cbDSIG = 0; // for OpenType
        BYTE *pbDSIG = NULL; // for OpenType
        BOOL  fIsTT;

        cb = sizeof(LOGFONT) * cFonts;

        if (gbIsDBCS)
        {
            //
            // save lplf to global variable.
            //
            glpLogFonts = lplf = AllocMem(cb);
        }
        else
        {
            lplf = AllocMem(cb);
        }

        // ?? Should this be GetFontResourceInfo (doesn't matter; but why force W)
        if (GetFontResourceInfoW( (LPTSTR)pszFontPath, &cb, lplf, GFRI_LOGFONTS )) {
            HDC hdc;
            HFONT hf, hfOld;
            LOGFONT lf;
            int nIndex;
            int cLoopReps = 1;

            BOOL fIsTrueTypeFont;
            DWORD dwSize = sizeof(BOOL);

            if(GetFontResourceInfoW((LPTSTR) pszFontPath, &dwSize, &fIsTrueTypeFont, GFRI_ISTRUETYPE)) {
                // If there is a raster & true type font on the system at the same time, 
                // and the height/width requested is supported by both fonts, the 
                // the font methods (which take the LOGFONT struct, *lplf) will select
                // the raster font (by design).  THis causes a problem when the user wants
                // to view the true type font; so, an extra check needs to be done to see if
                // the font requested is a true type, and if so then specify in the LOGFONT
                // struct to only show the true type font
                if(fIsTrueTypeFont) {
                    lplf->lfOutPrecision = OUT_TT_ONLY_PRECIS;
                }
            }

            //
            // This DBCS-aware code was originally placed within #ifdef DBCS
            // preprocessor statements.  For single-binary, these had to be
            // replaced with runtime checks.  The original code did some funky
            // things to execute a loop in DBCS builds but only a single iteration
            // in non-DBCS builds.  To do this, the "for" statement and it's
            // closing brace were placed in #ifdef DBCS like this:
            //
            // #ifdef DBCS
            //     for (nIndex = 0; nIndex < cFonts; nIndex++)
            //     {
            //          //
            //          // Other DBCS-specific code.
            //          //
            // #endif
            //          //
            //          // Code for both DBCS and non-DBCS systems
            //          // executes only once.
            //          //
            // #ifdef DBCS
            //     }
            // #endif
            //
            // While effective in a multi-binary configuration, this doesn't
            // translate well to a single-binary build.
            // To preserve the original logic without having to do major
            // reconstruction, I've replaced the loop sentinel variable with
            // "cLoopReps".  In non-DBCS locales, it is set to 1.  In DBCS
            // locales, it is assigned the value in "cFonts".
            //
            // [BrianAu 5/4/97]
            //

          if (gbIsDBCS)
              cLoopReps = cFonts;

          for (nIndex = 0; nIndex < cLoopReps; nIndex++) {
            if (gbIsDBCS)
            {
                lf = *(lplf + nIndex);

                //
                // Skip vertical font
                //
                if (lf.lfFaceName[0] == TEXT('@')) {
                    gNumOfFonts = (cFonts == 2) ? gNumOfFonts-1 : gNumOfFonts;
                    continue;
                }

                hf = CreateFontIndirect(&lf);
            }
            else
            {
                hf = CreateFontIndirect(lplf);
            }

            hdc = CreateCompatibleDC(NULL);

            hfOld = SelectObject(hdc, hf);

            // Only otf fonts will have CFF table, tag is ' FFC'.

            cbCFF = GetFontData(hdc,' FFC', 0, NULL, 0);
            cbDSIG = GetFontData(hdc,'GISD', 0, NULL, 0);

            if (cbDSIG != GDI_ERROR)
            {
                if ((pbDSIG = AllocMem(cbDSIG)) == NULL)
                {
                    // Can't determine what's in the DSIG table.
                    // Continue as though the DSIG table does not exist.
                    cbDSIG = 0;
                }
                else
                {
                    if (GetFontData (hdc, 'GISD', 0, pbDSIG, cbDSIG) == GDI_ERROR)
                    {
                        // Continue as though the DSIG table does not exist
                        cbDSIG = 0;
                    }
                    FreeMem(pbDSIG);
                }
            }


            if (cbCFF == GDI_ERROR)
                cbCFF = 0;

            if (cbDSIG == GDI_ERROR)
                cbDSIG = 0;

            if (cbCFF || cbDSIG)
            {
                fft = FFT_OTF;
                if (cbCFF)
                {
                    cbMMSD = GetFontData(hdc,'DSMM', 0, NULL, 0);
                    if (cbMMSD == GDI_ERROR)
                        cbMMSD = 0;
                }
            }

            cb = GetFontData(hdc, TT_TBL_NAME, 0, NULL, 0);

            if (fft != FFT_OTF)
            {
                fIsTT = (cb != 0 && cb != GDI_ERROR);
                fft = fIsTT ? FFT_TRUETYPE : FFT_BITMAP;
            }

            if ((fft == FFT_TRUETYPE) || (fft == FFT_OTF)) {
                int i;
                LPBYTE lpTTData;
                LPTSTR pszTmp;

                lpTTData = AllocMem(cb);
                GetFontData(hdc, TT_TBL_NAME, 0, lpTTData, cb);

                i = 0;

                //
                // Title String
                //
                pdtSmpl->atlDsp[i].dtyp = DTP_SHRINKDRAW;
                pdtSmpl->atlDsp[i].cptsSize = CPTS_TITLE_SIZE;
                pdtSmpl->atlDsp[i].fLineUnder = TRUE;

                pszTmp = GetAlignedTTName( lpTTData, TTID_NAME_FULLFONTNM );
                if (pszTmp != NULL) {
                    if (gbIsDBCS)
                    {
                        //
                        // TTC Support.
                        //
                        if (nIndex == 0) {
                            pdtSmpl->atlDsp[i].pszText = CloneString(pszTmp);
                        } else {
                            pdtSmpl->atlDsp[i].pszText = FmtSprintf(MSG_TTC_CONCAT,
                                                                    pdtSmpl->atlDsp[i].pszText,
                                                                    pszTmp);
                        }

                        if (nIndex + 1 == cFonts) {
                            //
                            // If last this is last font, append "(True Type)"
                            //
                        pdtSmpl->atlDsp[i].pszText = FmtSprintf((fft == FFT_TRUETYPE) ? MSG_PTRUETYPEP : MSG_POPENTYPEP,
                                                                pdtSmpl->atlDsp[i].pszText);
                        }
                        pdtSmpl->atlDsp[i].cchText = lstrlen(pdtSmpl->atlDsp[i].pszText);
                        FreeMem(pszTmp);
                    }
                    else
                    {
                        pdtSmpl->atlDsp[i].pszText = FmtSprintf((fft == FFT_TRUETYPE) ? MSG_PTRUETYPEP : MSG_POPENTYPEP, pszTmp);
                        pdtSmpl->atlDsp[i].cchText = lstrlen(pdtSmpl->atlDsp[i].pszText);
                        FreeMem(pszTmp);
                    }
                } else {
                    if (gbIsDBCS)
                    {
                        //
                        // TTC support
                        //
                        if (nIndex == 0) {
                            pdtSmpl->atlDsp[i].pszText = CloneString(lf.lfFaceName);
                        } else {
                            pdtSmpl->atlDsp[i].pszText = FmtSprintf(MSG_TTC_CONCAT,
                                                                    pdtSmpl->atlDsp[i].pszText,
                                                                    lf.lfFaceName);
                        }

                        if (nIndex + 1 == cFonts) {
                            //
                            // If last this is last font, append "(True Type)"
                            //
                            pdtSmpl->atlDsp[i].pszText = FmtSprintf((fft == FFT_TRUETYPE) ? MSG_PTRUETYPEP : MSG_POPENTYPEP,
                                                                    pdtSmpl->atlDsp[i].pszText);
                        }
                        pdtSmpl->atlDsp[i].cchText = lstrlen(pdtSmpl->atlDsp[i].pszText);
                    }
                    else
                    {
                        pdtSmpl->atlDsp[i].pszText = CloneString(lplf->lfFaceName);
                        pdtSmpl->atlDsp[i].cchText = lstrlen(pdtSmpl->atlDsp[0].pszText);
                    }
                }
                i++;
                pdtSmpl->atlDsp[i] = pdtSmpl->atlDsp[i-1];

                //// insert an extra line to provide better description of the font

                if (fft == FFT_OTF)
                {
                    LPTSTR pszTemp = NULL;
                    WCHAR awcTmp[256];
                    awcTmp[0] = 0; // zero init

                    pdtSmpl->atlDsp[i].dtyp = DTP_NORMALDRAW;
                    pdtSmpl->atlDsp[i].cptsSize = CPTS_INFO_SIZE;
                    pdtSmpl->atlDsp[i].fLineUnder = FALSE;

                    pdtSmpl->atlDsp[i].pszText = FmtSprintf(
                                      MSG_POTF,
                                      awcTmp);

                    if (cbDSIG)
                    {
                        pszTemp = pdtSmpl->atlDsp[i].pszText;

                        pdtSmpl->atlDsp[i].pszText = FmtSprintf(
                                      MSG_PDSIG,
                                      pdtSmpl->atlDsp[i].pszText);

                        FmtFree(pszTemp);
                    }

                    pszTemp = pdtSmpl->atlDsp[i].pszText;
                    pdtSmpl->atlDsp[i].pszText = FmtSprintf(
                                  cbCFF ? MSG_PPSGLYPHS : MSG_PTTGLYPHS,
                                  pdtSmpl->atlDsp[i].pszText);
                    FmtFree(pszTemp);

                    pszTemp = pdtSmpl->atlDsp[i].pszText;
                    pdtSmpl->atlDsp[i].pszText = FmtSprintf(
                                  MSG_PINSTRUCTIONS,
                                  pdtSmpl->atlDsp[i].pszText);
                    FmtFree(pszTemp);

                    if (cbCFF)
                    {
                        pszTemp = pdtSmpl->atlDsp[i].pszText;
                        pdtSmpl->atlDsp[i].pszText = FmtSprintf(
                                      cbMMSD ? MSG_PMULTIPLEMASTER : MSG_PSINGLEMASTER,
                                      pdtSmpl->atlDsp[i].pszText);
                        FmtFree(pszTemp);
                    }

                    pdtSmpl->atlDsp[i].cchText = lstrlen(pdtSmpl->atlDsp[i].pszText);

                    i++;
                    pdtSmpl->atlDsp[i] = pdtSmpl->atlDsp[i-1];
                }

                //
                // Typeface Name:
                //
                pdtSmpl->atlDsp[i].cptsSize = CPTS_INFO_SIZE;
                pdtSmpl->atlDsp[i].dtyp = DTP_NORMALDRAW;
                pdtSmpl->atlDsp[i].fLineUnder = FALSE;
                pszTmp = GetAlignedTTName( lpTTData, TTID_NAME_FONTFAMILY );
                if (pszTmp != NULL) {
                    pdtSmpl->atlDsp[i].pszText = FmtSprintf(MSG_TYPEFACENAME, pszTmp);
                    pdtSmpl->atlDsp[i].cchText = lstrlen(pdtSmpl->atlDsp[i].pszText);
                    FreeMem(pszTmp);
                    i++;
                    pdtSmpl->atlDsp[i] = pdtSmpl->atlDsp[i-1];
                }

                //
                // File size:
                //
                pdtSmpl->atlDsp[i].pszText = FmtSprintf(MSG_FILESIZE,
                        ROUND_UP_DIV(GetFileSizeFromName(pszFontPath), CB_ONE_K));
                pdtSmpl->atlDsp[i].cchText = lstrlen(pdtSmpl->atlDsp[i].pszText);

                //
                // Version:
                //
                pszTmp = GetAlignedTTName( lpTTData, TTID_NAME_VERSIONSTR );
                if (pszTmp != NULL) {
                    i++;
                    pdtSmpl->atlDsp[i] = pdtSmpl->atlDsp[i-1];
                    pdtSmpl->atlDsp[i].pszText = FmtSprintf(MSG_VERSION, pszTmp);
                    pdtSmpl->atlDsp[i].cchText = lstrlen(pdtSmpl->atlDsp[i].pszText);
                    FreeMem( pszTmp );
                }

                //
                // Copyright string
                //
                pszTmp = GetAlignedTTName( lpTTData, TTID_NAME_COPYRIGHT );
                if (pszTmp != NULL) {
                    i++;
                    pdtSmpl->atlDsp[i] = pdtSmpl->atlDsp[i-1];
                    pdtSmpl->atlDsp[i].cptsSize = CPTS_COPYRIGHT_SIZE;
                    pdtSmpl->atlDsp[i].dtyp = DTP_WRAPDRAW;
                    pdtSmpl->atlDsp[i].pszText = FmtSprintf(MSG_COPYRIGHT, pszTmp);
                    pdtSmpl->atlDsp[i].cchText = lstrlen(pdtSmpl->atlDsp[i].pszText);
                    FreeMem( pszTmp );
                }

                pdtSmpl->atlDsp[i].fLineUnder = TRUE;

                if (gbIsDBCS)
                {
                    //
                    // TTC Support.
                    //
                    FreeMem(lpTTData);
                }
            } else {

                // Title String (Non TrueType case)

                pdtSmpl->atlDsp[0].dtyp = DTP_SHRINKDRAW;
                pdtSmpl->atlDsp[0].cptsSize = CPTS_TITLE_SIZE;
                pdtSmpl->atlDsp[0].fLineUnder = TRUE;
                pdtSmpl->atlDsp[0].pszText = CloneString(lplf->lfFaceName);
                pdtSmpl->atlDsp[0].cchText = lstrlen(pdtSmpl->atlDsp[0].pszText);

                // Use Default quality, so we can see GDI scaling of Bitmap Fonts
                lplf->lfQuality = DEFAULT_QUALITY;
                lplf->lfWidth = 0;
            }

            // If LPK is loaded then GetFontResourceInfo(GFRI_LOGFONTS) may return ANSI_CHARSET for some DBCS fonts.
            // Get the native char set.
            if (gbIsDBCS & NativeCodePageSupported(lplf)) {
                    //
                    // Native code page is supported, set that codepage
                    //
                    CHARSETINFO csi;
        
                    TranslateCharsetInfo( (LPDWORD)GetACP(), &csi, TCI_SRCCODEPAGE );
        
                    lplf->lfCharSet = (BYTE)csi.ciCharset;
            }

            SelectObject(hdc, hfOld);
            DeleteObject(hf);
            DeleteDC(hdc);

          } // for
            pdtSmpl->lfTestFont = *lplf;
        }

        if (!gbIsDBCS)
        {
            FreeMem(lplf);
        }
    }


    //
    // MAJOR HACK!
    //
    // Since ATM-Type1 fonts are split between two files, (*.PFM and *.PFB) we have done a hack
    // earlier in the code to find the missing filename and concatinate them together in
    // the form "FOO.PFM|FOO.PFB", so we can then call AddFontResource() with only one string.
    //
    // Since SHGetFileInfo does not understand this hacked filename format, we must split ATM-Type1
    // names appart here and then reconcat them after we call the shell api.
    //
    pszAdobe = pszFontPath;

    while( *pszAdobe && *pszAdobe != TEXT('|') )
        pszAdobe = CharNext(pszAdobe);

    if ( *pszAdobe ) {

        *pszAdobe = TEXT('\0');

        pdtSmpl->atlDsp[0].pszText = FmtSprintf(MSG_PTYPE1, pdtSmpl->atlDsp[0].pszText);
        pdtSmpl->atlDsp[0].cchText = lstrlen(pdtSmpl->atlDsp[0].pszText);

    } else {
        pszAdobe = NULL;
    }
    // end of HACK


    //
    // Get the associated icon for this font file type
    //
    if ( fft != FFT_BAD_FILE && SHGetFileInfo( pszFontPath, 0, &sfi, sizeof(sfi), SHGFI_ICON )) {
        *phIcon = sfi.hIcon;
    } else
        *phIcon = NULL;

    //
    // HACK - restore the '|' we nuked above
    //
    if ( pszAdobe != NULL ) {
        *pszAdobe = TEXT('|');
    }
    // end of HACK

    return fft;
}
#endif


/****************************************************************************
*
*     FUNCTION: DrawFontSample
*
* Parameters:
*
*   lprcPage    Size of the page in pels.  A page is either a printed
*               sheet (on a printer) or the Window.
*
*   cyOffset    Offset into the virtual sample text.  Used to "scroll" the
*               window up and down.  Positive number means start further
*               down in the virtual sample text as the top line in the
*               lprcPage.
*
*   lprcPaint   Rectangle to draw.  It is in the same coord space as
*               lprcPage.  Used to optimize window repaints, and to
*               support banding to printers.
*
*
\****************************************************************************/
int DrawFontSample( HDC hdc, LPRECT lprcPage, int cyOffset, LPRECT lprcPaint, BOOL fReallyDraw ) {
    int cyDPI;
    HFONT hfOld, hfText, hfDesk;
    LOGFONT lfTmp;
    int yBaseline = -cyOffset;
    int taOld,i;
    TCHAR szNumber[10];
    int cyShkTxt = -1, cptsShkTxt = -1;
    SIZE sz;
    int cxPage;

    DPRINT((DBTX("PAINTING")));

    cyDPI = GetDeviceCaps(hdc, LOGPIXELSY );
    taOld = SetTextAlign(hdc, TA_BASELINE);

    glfFont.lfHeight = MulDiv( -CPTS_COPYRIGHT_SIZE, cyDPI, C_PTS_PER_INCH );
    hfDesk = CreateFontIndirect(&glfFont);

    // Get hfOld for later
    hfOld = SelectObject(hdc, hfDesk);


    if (gbIsDBCS)
    {
        //
        // if two or more fonts exist, set correct typeface name
        //
        if (gNumOfFonts > 1 && gfftFontType == FFT_TRUETYPE) {
            gdtDisplay.atlDsp[INDEX_TYPEFACENAME].pszText =
                                FmtSprintf(MSG_TYPEFACENAME, gdtDisplay.lfTestFont.lfFaceName);
            gdtDisplay.atlDsp[INDEX_TYPEFACENAME].cchText =
                                lstrlen(gdtDisplay.atlDsp[INDEX_TYPEFACENAME].pszText);
        }
    }

    //
    // Find the longest shrinktext line so we can make sure they will fit
    // on the screen
    //
    cxPage = lprcPage->right - lprcPage->left;
    for( i = 0; i < CLINES_DISPLAY && gdtDisplay.atlDsp[i].dtyp != DTP_UNUSED; i++ ) {
        PTXTLN ptlCurrent = &(gdtDisplay.atlDsp[i]);

        if (ptlCurrent->dtyp == DTP_SHRINKTEXT) {
            lfTmp = gdtDisplay.lfTestFont;

            if (cptsShkTxt == -1)
                cptsShkTxt = ptlCurrent->cptsSize;

            cyShkTxt = MulDiv( -cptsShkTxt, cyDPI, C_PTS_PER_INCH );

            lfTmp.lfHeight = cyShkTxt;

            hfText = CreateFontIndirect( &lfTmp );
            SelectObject(hdc, hfText);

            GetTextExtentPoint32(hdc, ptlCurrent->pszText, ptlCurrent->cchText, &sz );

            SelectObject(hdc, hfOld);
            DeleteObject(hfText);

            // Make sure shrink lines are not too long
            if (sz.cx > cxPage) {

                DPRINT((DBTX(">>>Old lfH:%d sz.cx:%d cxPage:%d"), lfTmp.lfHeight, sz.cx, cxPage));

                cptsShkTxt = cptsShkTxt * cxPage / sz.cx;
                cyShkTxt = MulDiv( -cptsShkTxt, cyDPI, C_PTS_PER_INCH );

                DPRINT((DBTX(">>>New lfH:%d"),lfTmp.lfHeight));
            }
        }
    }


    //
    // Paint the screen/page
    //
    for( i = 0; i < CLINES_DISPLAY && gdtDisplay.atlDsp[i].dtyp != DTP_UNUSED; i++ ) {
        TEXTMETRIC tm;
        PTXTLN ptlCurrent = &(gdtDisplay.atlDsp[i]);

        // Create and select the font for this line

        if (ptlCurrent->dtyp == DTP_TEXTOUT || ptlCurrent->dtyp == DTP_SHRINKTEXT )
            lfTmp = gdtDisplay.lfTestFont;
        else
            lfTmp = glfFont;

        if (ptlCurrent->dtyp == DTP_SHRINKTEXT) {
            DPRINT((DBTX("PAINT:Creating ShrinkText Font:%s height:%d"), lfTmp.lfFaceName, lfTmp.lfHeight ));
            lfTmp.lfHeight = cyShkTxt;
        }
        else
            lfTmp.lfHeight = MulDiv( -ptlCurrent->cptsSize, cyDPI, C_PTS_PER_INCH );

        hfText = CreateFontIndirect( &lfTmp );
        SelectObject(hdc, hfText);


        // Get size characteristics for this line in the selected font
        if (ptlCurrent->dtyp == DTP_SHRINKDRAW) {

            GetTextExtentPoint32(hdc, ptlCurrent->pszText, ptlCurrent->cchText, &sz );

            // Make sure shrink lines are not too long
            if (sz.cx > cxPage) {

                SelectObject(hdc, hfOld);
                DeleteObject(hfText);

                DPRINT((DBTX("===Old lfH:%d sz.cx:%d cxPage:%d"), lfTmp.lfHeight, sz.cx, cxPage));

                lfTmp.lfHeight = MulDiv( -ptlCurrent->cptsSize * cxPage / sz.cx, cyDPI, C_PTS_PER_INCH );

                DPRINT((DBTX("===New lfH:%d"),lfTmp.lfHeight));

                hfText = CreateFontIndirect( &lfTmp );
                SelectObject(hdc, hfText);
            }
        }



        GetTextMetrics(hdc, &tm);

        yBaseline += (tm.tmAscent + tm.tmExternalLeading);
        DPRINT((DBTX("tmH:%d tmA:%d tmD:%d tmIL:%d tmEL:%d"), tm.tmHeight, tm.tmAscent, tm.tmDescent, tm.tmInternalLeading, tm.tmExternalLeading));

        // Draw the text
        switch(ptlCurrent->dtyp) {
            case DTP_NORMALDRAW:
            case DTP_SHRINKDRAW:
            case DTP_SHRINKTEXT:
                if (fReallyDraw) {
                    ExtTextOut(hdc, lprcPage->left, yBaseline, ETO_CLIPPED, lprcPaint,
                            ptlCurrent->pszText, ptlCurrent->cchText, NULL);
                }

                //
                // Bob says "This looks nice!" (Adding a little extra white space before the underline)
                //
                if (ptlCurrent->fLineUnder)
                    yBaseline += tm.tmDescent;

                break;

            case DTP_WRAPDRAW: {
                RECT rc;
                int cy;

                yBaseline += tm.tmDescent;
                SetRect(&rc, lprcPage->left, yBaseline - tm.tmHeight, lprcPage->right, yBaseline );

                DPRINT((DBTX("**** Org RC:(%d, %d, %d, %d)  tmH:%d"), rc.left, rc.top, rc.right, rc.bottom, tm.tmHeight));
                cy = DrawText(hdc, ptlCurrent->pszText, ptlCurrent->cchText, &rc,
                        DT_NOPREFIX | DT_WORDBREAK | DT_CALCRECT);


                DPRINT((DBTX("**** Cmp RC:(%d, %d, %d, %d)  cy:%d"), rc.left, rc.top, rc.right, rc.bottom, cy));
                if( cy > tm.tmHeight )
                    yBaseline = rc.bottom = rc.top + cy;

                if (fReallyDraw) {
                    SetTextAlign(hdc, taOld);
                    DrawText(hdc, ptlCurrent->pszText, ptlCurrent->cchText, &rc, DT_NOPREFIX | DT_WORDBREAK);
                    SetTextAlign(hdc, TA_BASELINE);
                }
                break;
            }

            case DTP_TEXTOUT:
                if (fReallyDraw) {
                    SIZE szNum;
                    int cchNum;
                    SelectObject(hdc, hfDesk );
                    wsprintf( szNumber, TEXT("%d"), ptlCurrent->cptsSize );
                    cchNum = lstrlen(szNumber);
                    ExtTextOut(hdc, lprcPage->left, yBaseline, ETO_CLIPPED, lprcPaint, szNumber, cchNum, NULL);


                    GetTextExtentPoint32(hdc, szNumber, cchNum, &szNum);

                    SelectObject(hdc, hfText);
                    ExtTextOut(hdc, lprcPage->left + szNum.cx * 2, yBaseline, ETO_CLIPPED, lprcPaint,
                            ptlCurrent->pszText, ptlCurrent->cchText, NULL);
                }
                break;
        }

        yBaseline += tm.tmDescent;

        if (fReallyDraw && ptlCurrent->fLineUnder) {
            MoveToEx( hdc, lprcPage->left, yBaseline, NULL);
            LineTo( hdc, lprcPage->right, yBaseline );

            // Leave space for the line we just drew
            yBaseline += 1;
        }

        SelectObject( hdc, hfOld );
        DeleteObject( hfText );
    }

    SelectObject(hdc, hfOld);
    SetTextAlign(hdc, taOld);
    DeleteObject(hfDesk);

    return yBaseline;
}

/****************************************************************************
*
*     FUNCTION: PaintSampleWindow
*
\****************************************************************************/
void PaintSampleWindow( HWND hwnd, HDC hdc, PAINTSTRUCT *pps ) {
    RECT rcClient;

    GetClientRect(hwnd, &rcClient);

    DrawFontSample( hdc, &rcClient, gyScroll, &(pps->rcPaint), TRUE );

}


/****************************************************************************
*
*     FUNCTION: FrameWndProc(HWND, unsigned, WORD, LONG)
*
*     PURPOSE:  Processes messages
*
*     MESSAGES:
*
*         WM_COMMAND    - application menu (About dialog box)
*         WM_DESTROY    - destroy window
*
*     COMMENTS:
*
*         To process the IDM_ABOUT message, call MakeProcInstance() to get the
*         current instance address of the About() function.  Then call Dialog
*         box which will create the box according to the information in your
*         fontview.rc file and turn control over to the About() function.  When
*         it returns, free the intance address.
*
\****************************************************************************/

LRESULT APIENTRY FrameWndProc(
        HWND hwnd,                /* window handle                   */
        UINT message,             /* type of message                 */
        WPARAM wParam,            /* additional information          */
        LPARAM lParam)            /* additional information          */
{
    static SIZE szWindow = {0, 0};

    switch (message) {

        case WM_PAINT: {
            HDC hdc;
            RECT rc;
            PAINTSTRUCT ps;
            int x;

            hdc = BeginPaint(hwnd, &ps);

            // get the window rect
            GetClientRect(hwnd, &rc);

            // extend only down by gcyBtnArea
            rc.bottom = rc.top + gcyBtnArea;

            // Fill rect with button face color (handled by class background brush)
            // FillRect(hdc, &rc, ghbr3DFace);

            // Fill small rect at bottom with edge color
            rc.top = rc.bottom - 2;
            FillRect(hdc, &rc, ghbr3DShadow);

            ReleaseDC(hwnd, hdc);

            EndPaint(hwnd, &ps);
            break;
        }

        case WM_CREATE: {
            HDC hdc;
            RECT rc;
            int i;

            GetClientRect(hwnd, &rc);
            szWindow.cx = rc.right - rc.left;
            szWindow.cy = rc.bottom - rc.top;

            for( i = 0; i < C_BUTTONS; i++ ) {
                int x = gabtCmdBtns[i].x;
                HWND hwndBtn;

                if (gbIsDBCS)
                {
                    DWORD dwStyle = 0;

                    //
                    // If font is not TrueType font or not TTC font,
                    // AND button id is previous/next,
                    // then just continue.
                    //
                    if ((gfftFontType != FFT_TRUETYPE ||
                         gNumOfFonts <= 1) &&
                        (gabtCmdBtns[i].id == IDB_PREV_FONT ||
                         gabtCmdBtns[i].id == IDB_NEXT_FONT)) {
                            continue;
                    }
                    //
                    // Set x potision for each button.
                    //
                    switch (gabtCmdBtns[i].id) {
                        case IDB_PREV_FONT:
                            x = szWindow.cx / 2 - gabtCmdBtns[i].cx - 5;
                            dwStyle = WS_DISABLED;  // initially disabled.
                            break;
                        case IDB_NEXT_FONT:
                            x = szWindow.cx / 2 + 5;
                            break;
                        default:
                            if (x < 0)
                                x = szWindow.cx + x - gabtCmdBtns[i].cx;
                    }
                    gabtCmdBtns[i].hwnd = hwndBtn = CreateWindow( TEXT("button"),
                            gabtCmdBtns[i].pszText,
                            BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | dwStyle,
                            x, gabtCmdBtns[i].y,
                            gabtCmdBtns[i].cx, gabtCmdBtns[i].cy,
                            hwnd, (HMENU)gabtCmdBtns[i].id,
                            hInst, NULL);
                }
                else
                {
                    if (x < 0)
                        x = szWindow.cx + x - gabtCmdBtns[i].cx;

                    gabtCmdBtns[i].hwnd = hwndBtn = CreateWindow( TEXT("button"),
                            gabtCmdBtns[i].pszText, BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE,
                            x, gabtCmdBtns[i].y,
                            gabtCmdBtns[i].cx, gabtCmdBtns[i].cy,
                            hwnd, (HMENU)gabtCmdBtns[i].id,
                            hInst, NULL);

                }
                if (hwndBtn != NULL) {
                    SendMessage(hwndBtn,
                                WM_SETFONT,
                                (WPARAM)GetStockObject(DEFAULT_GUI_FONT),
                                MAKELPARAM(TRUE, 0));
                }
            }

            ghwndView = CreateWindow( TEXT("FontDisplayClass"), NULL, WS_CHILD | WS_VSCROLL | WS_VISIBLE,
                    0, gcyBtnArea, szWindow.cx, szWindow.cy - gcyBtnArea, hwnd, 0, hInst, NULL );

            break;
        }

        case WM_GETMINMAXINFO: {
            LPMINMAXINFO lpmmi = (LPMINMAXINFO) lParam;

            lpmmi->ptMinTrackSize.x = gcxMinWinSize;
            lpmmi->ptMinTrackSize.y = gcyMinWinSize;

            break;
        }

        case WM_SIZE: {
            int cxNew, cyNew;
            HDC hdc;
            RECT rc;
            SCROLLINFO sci;

            cxNew = LOWORD(lParam);
            cyNew = HIWORD(lParam);

            if (cyNew != szWindow.cy || cxNew != szWindow.cx) {
                int i;

                if (gbIsDBCS)
                {
                    for( i = 0; i < C_BUTTONS; i++ ) {
                        int x = gabtCmdBtns[i].x;

                        //
                        // If font is not TrueType font or not TTC font,
                        // AND button id is previous/next,
                        // then just continue.
                        //
                        if ((gfftFontType != FFT_TRUETYPE ||
                             gNumOfFonts <= 1) &&
                            (gabtCmdBtns[i].id == IDB_PREV_FONT ||
                             gabtCmdBtns[i].id == IDB_NEXT_FONT)) {
                                continue;
                        }
                        //
                        // Set x potision for each button.
                        //
                        switch (gabtCmdBtns[i].id) {
                            case IDB_PREV_FONT:
                                SetWindowPos(gabtCmdBtns[i].hwnd,
                                             NULL,
                                             cxNew / 2 - gabtCmdBtns[i].cx - 5,
                                             gabtCmdBtns[i].y,
                                             0,
                                             0,
                                             SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
                                break;
                            case IDB_NEXT_FONT:
                                SetWindowPos(gabtCmdBtns[i].hwnd,
                                             NULL,
                                             cxNew /2 + 5,
                                             gabtCmdBtns[i].y,
                                             0,
                                             0,
                                             SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
                                break;
                            default:
                                if (x < 0) {
                                    SetWindowPos(gabtCmdBtns[i].hwnd,
                                                 NULL,
                                                 cxNew + x - gabtCmdBtns[i].cx,
                                                 gabtCmdBtns[i].y,
                                                 0,
                                                 0,
                                                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
                                }
                        }
                    }
                }
                else // !DBCS
                {
                    for( i = 0; i < C_BUTTONS; i++ ) {
                        int x = gabtCmdBtns[i].x;

                        if (x < 0) {
                            SetWindowPos(gabtCmdBtns[i].hwnd, NULL, cxNew + x - gabtCmdBtns[i].cx, gabtCmdBtns[i].y, 0, 0,
                                    SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
                        }
                    }
                } // DBCS

                szWindow.cx = cxNew;
                szWindow.cy = cyNew;

                SetWindowPos(ghwndView, NULL, 0, gcyBtnArea, szWindow.cx, szWindow.cy - gcyBtnArea,
                        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );

            }
            break;
        }

        case WM_COMMAND:           /* message: command from application menu */
            if (LOWORD(wParam) != IDB_DONE)
                return SendMessage(ghwndView, message, wParam, lParam);

            PostMessage(ghwndFrame, WM_CLOSE, 0, 0);
            break;

        case WM_DESTROY: {
            int i;

            DestroyWindow(ghwndView);
            for( i = 0; i < C_BUTTONS; i++ ) {
                DestroyWindow(gabtCmdBtns[i].hwnd);
            }

            PostQuitMessage(0);
            break;
        }

        default:                          /* Passes it on if unproccessed    */
            return (DefWindowProc(hwnd, message, wParam, lParam));
    }
    return (0L);
}

/****************************************************************************
*
*     FUNCTION: ViewWndProc(HWND, unsigned, WORD, LONG)
*
*     PURPOSE:  Processes messages
*
*     MESSAGES:
*
*         WM_COMMAND    - application menu (About dialog box)
*         WM_DESTROY    - destroy window
*
*     COMMENTS:
*
*         To process the IDM_ABOUT message, call MakeProcInstance() to get the
*         current instance address of the About() function.  Then call Dialog
*         box which will create the box according to the information in your
*         fontview.rc file and turn control over to the About() function.  When
*         it returns, free the intance address.
*
\****************************************************************************/

LRESULT APIENTRY ViewWndProc(
        HWND hwnd,                /* window handle                   */
        UINT message,             /* type of message                 */
        WPARAM wParam,            /* additional information          */
        LPARAM lParam)            /* additional information          */
{
    static SIZE szWindow = {0, 0};
    static int  cyVirtPage = 0;

    switch (message) {

        case WM_CREATE: {
            HDC hdc;
            RECT rc;
            SCROLLINFO sci;
            int i;

            GetClientRect(hwnd, &rc);
            szWindow.cx = rc.right - rc.left;
            szWindow.cy = rc.bottom - rc.top;

            hdc = CreateCompatibleDC(NULL);
            cyVirtPage = DrawFontSample(hdc, &rc, 0, NULL, FALSE);
            DeleteDC(hdc);


            gyScroll = 0;

            sci.cbSize = sizeof(sci);
            sci.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
            sci.nMin = 0;
            sci.nMax = cyVirtPage;
            sci.nPage = szWindow.cy;
            sci.nPos = gyScroll;

            SetScrollInfo(hwnd, SB_VERT, &sci, TRUE );

            if (gfPrint)
                PostMessage(hwnd, WM_COMMAND, IDB_PRINT, 0);
            break;
        }

        case WM_SIZE: {
            int cxNew, cyNew;
            HDC hdc;
            RECT rc;
            SCROLLINFO sci;

            cxNew = LOWORD(lParam);
            cyNew = HIWORD(lParam);

            if (cyNew != szWindow.cy || cxNew != szWindow.cx) {
                int i;

                szWindow.cx = cxNew;
                szWindow.cy = cyNew;

                hdc = CreateCompatibleDC(NULL);
                SetRect(&rc, 0, 0, szWindow.cx, szWindow.cy);
                cyVirtPage = DrawFontSample(hdc, &rc, 0, NULL, FALSE);
                DeleteDC(hdc);

                if (cyVirtPage <= cyNew) {
                    // Disable the scrollbar
                    gyScroll = 0;
                }

                if (cyVirtPage > szWindow.cy && gyScroll > cyVirtPage - szWindow.cy)
                    gyScroll = cyVirtPage - szWindow.cy;

                sci.cbSize = sizeof(sci);
                sci.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
                sci.nMin = 0;
                sci.nMax = cyVirtPage;
                sci.nPage = cyNew;
                sci.nPos = gyScroll;

                SetScrollInfo(hwnd, SB_VERT, &sci, TRUE );
            }
            break;
        }

        case WM_VSCROLL: {
            int iCode = (int)LOWORD(wParam);
            int yPos = (int)HIWORD(wParam);
            int yNewScroll = gyScroll;

            switch( iCode ) {

            case SB_THUMBPOSITION:
            case SB_THUMBTRACK:
                if (yPos != yNewScroll)
                    yNewScroll = yPos;
                break;

            case SB_LINEUP:
                yNewScroll -= gcyLine;
                break;

            case SB_PAGEUP:
                yNewScroll -= szWindow.cy;
                break;

            case SB_LINEDOWN:
                yNewScroll += gcyLine;
                break;

            case SB_PAGEDOWN:
                yNewScroll += szWindow.cy;
                break;

            case SB_TOP:
                yNewScroll = 0;
                break;

            case SB_BOTTOM:
                yNewScroll = cyVirtPage;
                break;
            }

            if (yNewScroll < 0)
                yNewScroll = 0;

            if (yNewScroll > cyVirtPage - szWindow.cy)
                yNewScroll = cyVirtPage - szWindow.cy;

            if (yNewScroll < 0)
                yNewScroll = 0;

            if (gyScroll != yNewScroll) {
                SCROLLINFO sci;
                int dyScroll;

                dyScroll = gyScroll - yNewScroll;

                if (ABS(dyScroll) < szWindow.cy) {
                    ScrollWindowEx(hwnd, 0, dyScroll, NULL, NULL, NULL, NULL, SW_ERASE | SW_INVALIDATE);
                } else
                    InvalidateRect(hwnd, NULL, TRUE);

                gyScroll = yNewScroll;

                sci.cbSize = sizeof(sci);
                sci.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
                sci.nMin = 0;
                sci.nMax = cyVirtPage;
                sci.nPage = szWindow.cy;
                sci.nPos = gyScroll;

                SetScrollInfo(hwnd, SB_VERT, &sci, TRUE );
            }

            break;
        }


        case WM_COMMAND:           /* message: command from application menu */
            if( !DoCommand( hwnd, wParam, lParam ) )
                return (DefWindowProc(hwnd, message, wParam, lParam));
            break;

        case WM_PAINT: {
            HDC hdc;
            PAINTSTRUCT ps;

            hdc = BeginPaint( hwnd, &ps );
            PaintSampleWindow( hwnd, hdc, &ps );
            EndPaint( hwnd, &ps );
            break;
        }

        default:                          /* Passes it on if unproccessed    */
            return (DefWindowProc(hwnd, message, wParam, lParam));
    }
    return (0L);
}

/*********************************************\
*
* PRINT DLGS
*
*
\*********************************************/
HDC PromptForPrinter(HWND hwnd, HINSTANCE hInst, int *pcCopies ) {
    PRINTDLG pd;

    FillMemory(&pd, sizeof(pd), 0);

    pd.lStructSize = sizeof(pd);
    pd.hwndOwner = hwnd;
    pd.Flags = PD_RETURNDC | PD_NOSELECTION;
    pd.nCopies = 1;
    pd.hInstance = hInst;

    if (PrintDlg(&pd)) {
        *pcCopies = pd.nCopies;
        return pd.hDC;
    } else
        return NULL;
}

/****************************************************************************\
*
*     FUNCTION: PrintSampleWindow(hwnd)
*
*       Prompts for a printer and then draws the sample text to the printer
*
\****************************************************************************/
void PrintSampleWindow(HWND hwnd) {
    HDC hdc;
    DOCINFO di;
    int cxDPI, cyDPI, iPage, cCopies;
    RECT rcPage;
    HCURSOR hcur;

    hdc = PromptForPrinter(hwnd, hInst, &cCopies);
    if (hdc == NULL)
        return;

    hcur = SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT)));

    cyDPI = GetDeviceCaps(hdc, LOGPIXELSY );
    cxDPI = GetDeviceCaps(hdc, LOGPIXELSX );

    /*
     * Set a one inch margine around the page
     */
    SetRect(&rcPage, 0, 0, GetDeviceCaps(hdc, HORZRES), GetDeviceCaps(hdc, VERTRES));

    rcPage.left    += cxDPI;
    rcPage.right   -= cxDPI;


    di.cbSize = sizeof(di);
    di.lpszDocName = gdtDisplay.atlDsp[0].pszText;
    di.lpszOutput = NULL;
    di.lpszDatatype = NULL;
    di.fwType = 0;

    StartDoc(hdc, &di);

    for( iPage = 0; iPage < cCopies; iPage++ ) {
        StartPage(hdc);

        DrawFontSample( hdc, &rcPage, -cyDPI, &rcPage, TRUE );

        EndPage(hdc);
    }

    EndDoc(hdc);

    DeleteDC(hdc);

    SetCursor(hcur);
}


/****************************************************************************\
*
*     FUNCTION: EnableCommandButtons(id, bEnable)
*
*       Enable/disable command button.
*
\****************************************************************************/
BOOL EnableCommandButton(int id, BOOL bEnable)
{
    int  i;
    HWND hwnd = NULL;

    for( i = 0; i < C_BUTTONS; i++ ) {
        if (gabtCmdBtns[i].id == id) {
            hwnd = gabtCmdBtns[i].hwnd;
            break;
        }
    }
    return (hwnd == NULL) ? FALSE: EnableWindow(hwnd, bEnable);
}


/****************************************************************************\
*
*     FUNCTION: ViewNextFont(iInc)
*
*       Show the previous/next font.
*
\****************************************************************************/
void ViewNextFont(int iInc)
{
    int index = gIndexOfFonts + iInc;

    while (1) {
        if ( index < 0 || index >= gNumOfFonts ) {
            //
            // if out of range, then return.
            //
            MessageBeep(MB_OK);
            return;
        }
        else if ((*(glpLogFonts + index)).lfFaceName[0] == TEXT('@')) {
            //
            // if the font is vertical font, skip this font and
            // try next/previous font.
            //
            index += iInc;
        }
        else {
            break;
        }
    }

    //
    // Enable/Disable Prev/Next buttons.
    //
    if (index == 0) {
        // first font
        EnableCommandButton(IDB_PREV_FONT, FALSE);
        EnableCommandButton(IDB_NEXT_FONT, TRUE);
    }
    else if (index == gNumOfFonts - 1) {
        // last font
        EnableCommandButton(IDB_PREV_FONT, TRUE);
        EnableCommandButton(IDB_NEXT_FONT, FALSE);
    }
    else {
        // other
        EnableCommandButton(IDB_PREV_FONT, TRUE);
        EnableCommandButton(IDB_NEXT_FONT, TRUE);
    }

    //
    // Show the new font.
    //
    gIndexOfFonts = index;
    gdtDisplay.lfTestFont = *(glpLogFonts + index);
    InvalidateRect(ghwndView, NULL, TRUE);
}


/****************************************************************************\
*
*     FUNCTION: DoCommand(HWND, unsigned, WORD, LONG)
*
*     PURPOSE:  Processes messages for "About" dialog box
*
*     MESSAGES:
*
*         WM_INITDIALOG - initialize dialog box
*         WM_COMMAND    - Input received
*
*     COMMENTS:
*
*         No initialization is needed for this particular dialog box, but TRUE
*         must be returned to Windows.
*
*         Wait for user to click on "Ok" button, then close the dialog box.
*
\****************************************************************************/
BOOL DoCommand( HWND hWnd, WPARAM wParam, LPARAM lParam )
{

    switch(LOWORD(wParam)){
        case IDB_PRINT: {
            PrintSampleWindow(hWnd);
            break;
        }

        case IDB_DONE: {
            PostMessage(ghwndFrame, WM_CLOSE, 0, 0);
            break;
        }

        case IDK_UP: {
            SendMessage(hWnd, WM_VSCROLL, SB_LINEUP, (LPARAM)NULL );
            break;
        }

        case IDK_DOWN: {
            SendMessage(hWnd, WM_VSCROLL, SB_LINEDOWN, (LPARAM)NULL );
            break;
        }

        case IDK_PGUP: {
            SendMessage(hWnd, WM_VSCROLL, SB_PAGEUP, (LPARAM)NULL );
            break;
        }

        case IDK_PGDWN: {
            SendMessage(hWnd, WM_VSCROLL, SB_PAGEDOWN, (LPARAM)NULL );
            break;
        }

        case IDB_PREV_FONT: {
            ViewNextFont(-1);
            break;
        }

        case IDB_NEXT_FONT: {
            ViewNextFont(1);
            break;
        }

        default: {
            return FALSE;
        }
    }

    return TRUE;
}



BOOL bFileExists(TCHAR*pszFile)
{
    HANDLE  hf;

    if ((hf = CreateFile(pszFile,
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL)) != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hf);
        return TRUE;
    }

    return FALSE;
}


/******************************Public*Routine******************************\
*
* FindPfb, given pfm file, see if pfb file exists in the same dir or in the
* parent directory of the pfm file
*
* History:
*  14-Jun-1994 -by- Bodin Dresevic [BodinD]
* Wrote it.
*
* Returns: 16-bit encoded value indicating error and type of file where
*          error occurred.  (see fvscodes.h) for definitions.
*          The following table lists the "status" portion of the codes
*          returned.
*
*           FVS_SUCCESS
*           FVS_INVALID_FONTFILE
*           FVS_FILE_OPEN_ERR
*
\**************************************************************************/




BOOL  bFindPfb (
    TCHAR *pszPFM,
    TCHAR *achPFB
)
{
    DWORD  cjKey;
    TCHAR *pszParent = NULL; // points to the where parent dir of the inf file is
    TCHAR *pszBare = NULL;   // "bare" .inf name, initialization essential

// example:
// if pszPFM -> "c:\psfonts\pfm\foo_____.pfm"
// then pszParent -> "pfm\foo_____.pfm"

    cjKey = lstrlen(pszPFM) + 1;

    if (cjKey < 5)          // 5 = lstrlen(".pfm") + 1;
        return FALSE;

// go on to check if .pfb file exists:
// We will first check .pfb file exists in the same dir as .pfm

    lstrcpy(achPFB, pszPFM);
    lstrcpy(&achPFB[cjKey - 5],TEXT(".PFB"));

    if (!bFileExists(achPFB))
    {
    // we did not find the .pfb file in the same dir as .pfm
    // Now check the parent directory of the .pfm file

        pszBare = &pszPFM[cjKey - 5];
        for ( ; pszBare > pszPFM; pszBare--)
        {
            if ((*pszBare == TEXT('\\')) || (*pszBare == TEXT(':')))
            {
                pszBare++; // found it
                break;
            }
        }

    // check if full path to .pfm was passed in or a bare
    // name itself was passed in to look for .pfm file in the current dir

        if ((pszBare > pszPFM) && (pszBare[-1] == TEXT('\\')))
        {
        // skip '\\' and search backwards for another '\\':

            for (pszParent = &pszBare[-2]; pszParent > pszPFM; pszParent--)
            {
                if ((*pszParent == TEXT('\\')) || (*pszParent == TEXT(':')))
                {
                    pszParent++; // found it
                    break;
                }
            }

        // create .pfb file name in the .pfm parent directory:

            lstrcpy(&achPFB[pszParent - pszPFM], pszBare);
            lstrcpy(&achPFB[lstrlen(achPFB) - 4], TEXT(".PFB"));

        }
        else if (pszBare == pszPFM)
        {
        // bare name was passed in, to check for the inf file in the "." dir:

            lstrcpy(achPFB, TEXT("..\\"));
            lstrcpy(&achPFB[3], pszBare);   // 3 == lstrlen("..\\")
            lstrcpy(&achPFB[lstrlen(achPFB) - 4], TEXT(".PFB"));
        }
        else
        {
            return FALSE;
        }

   // check again if we can find the file, if not fail.

       if (!bFileExists(achPFB))
       {
           return FALSE;
       }
    }

// now we have paths to .pfb file in the buffer provided by the caller.

    return TRUE;
}
