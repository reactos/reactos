#include "precomp.h"

/************************************************************************/
/*                                                                      */
/*  Windows Cardfile - Written by Mark Cliggett                         */
/*  (c) Copyright Microsoft Corp. 1985, 1994 - All Rights Reserved      */
/*                                                                      */
/************************************************************************/

/* OLE definitions */
OLECLIENTVTBL clientTbl;
OLESTREAMVTBL streamTbl;
OLECLIPFORMAT vcfLink = 0;
OLECLIPFORMAT vcfOwnerLink = 0;
OLECLIPFORMAT vcfNative = 0;
OLESTATUS oleloadstat;
LHCLIENTDOC lhcdoc = 0;

/* Class name for cardfile - not translatable. */
TCHAR szCardfileClass[] = TEXT("Cardfile");
TCHAR szCardClass[] = TEXT("Card");

/* Name of cardfile that appears in title bar - translatable. */
TCHAR szCardfile[40];
TCHAR szMarginError[160];
TCHAR szFileExtension[5];

/* Headings for sections of win.ini */
TCHAR szWindows[] = TEXT("Windows");
TCHAR szDevice[] = TEXT("Device");
TCHAR szCardfileSect[] = TEXT("Cardfile");

/* Things for Page Setup */
TCHAR chPageText[6][PT_LEN];

TCHAR szMerge[3];

/* variables for the new File Open,File SaveAs and Find Text dialogs */
#define FILTERMAX   100

OPENFILENAME OFN;
FINDREPLACE FR;
PRINTDLG PD;
TCHAR szLastDir [PATHMAX];
HANDLE hFind = NULL;
TCHAR szServerFilter [FILTERMAX * 10]; /* default filter spec. for servers  */
TCHAR szFilterSpec [FILTERMAX * 5];    /* default filter spec. for above    */
TCHAR szCustFilterSpec [FILTERMAX];    /* buffer for custom filters created */
UINT  wFRMsg;                          /* message used in communicating     */
                                       /*    with Find/Replace dialog       */
int wHlpMsg;                           /* message used to invoke Help       */
TCHAR szOpenCaption [CAPTIONMAX];      /* File open dialog caption text     */
TCHAR szSaveCaption [CAPTIONMAX];      /* File Save as dialog caption text  */
TCHAR szMergeCaption[CAPTIONMAX];      /* File Merge dialog caption text    */
TCHAR szLinkCaption [CAPTIONMAX];      /* Link Repair dialog caption text   */

HANDLE hAccel;

NOEXPORT BOOL NEAR ProcessShellOptions (LPTSTR lpLine);

void GetOldData (HANDLE hInstance)
{
#if !defined(WIN32)
    GetInstanceData(hInstance, &CharFixHeight, 2);
    GetInstanceData(hInstance, &CharFixWidth, 2);
    GetInstanceData(hInstance, &ySpacing, 2);
    GetInstanceData(hInstance, &CardWidth, 2);
    GetInstanceData(hInstance, &CardHeight, 2);
    GetInstanceData(hInstance, &EditWidth, 2);
    GetInstanceData(hInstance, &EditHeight, 2);
    GetInstanceData(hInstance, &hArrowCurs, 2);
    GetInstanceData(hInstance, &hWaitCurs, 2);
    GetInstanceData(hInstance, &hAccel, 2);

    GetInstanceData(hInstance, &cxHScrollBar, 2);
    GetInstanceData(hInstance, &cyHScrollBar, 2);
#endif
}

/*
 * do this for every instance
 */
BOOL InitInstance(HANDLE hInstance, LPTSTR lpszCommandLine, int cmdShow)
{
    int           i;
    HWND          hwnd = NULL;
    LPTSTR        lpchTmp;
    LPTSTR        pchTmp;
    TCHAR         buf[3];
    HMENU         hMenu;
    TCHAR        *pszFilterSpec = szFilterSpec;
    LPCARDHEADER  Cards;
#ifndef WIN32
    int           cbName;
#endif

#ifndef OLE_20
    CHAR  aszCardfile[60];
    CHAR  aszUntitled[60];
    CHAR  aszCurIFile[MAX_PATH];
#endif

    if (fOLE && !OleInit(hInstance))
        goto InitError;

    LoadString(hInstance, EINSMEMORY, NotEnoughMem, CharSizeOf(NotEnoughMem));
    LoadString(hInstance, IHELPFILE, szHelpFile, CharSizeOf(szHelpFile));
    LoadString(hInstance, IUNTITLED, szUntitled, CharSizeOf(szUntitled));
    LoadString(hInstance, IWARNING, szWarning, CharSizeOf(szWarning));
    LoadString(hInstance, INOTE, szNote, CharSizeOf(szNote));
    LoadString(hInstance, ISTRINGINSERT, buf, CharSizeOf(buf));
    LoadString(hInstance, IFILEEXTENSION, szFileExtension, CharSizeOf(szFileExtension));
    LoadString(hInstance, ICARDFILE, szCardfile, CharSizeOf(szCardfile));
    LoadString(hInstance, IMARGINERR, szMarginError, CharSizeOf(szMarginError));

    LoadString(hInstance, IDS_OPENDLGTITLE, szOpenCaption, CharSizeOf(szOpenCaption));
    LoadString(hInstance, IDS_SAVEDLGTITLE, szSaveCaption, CharSizeOf(szSaveCaption));
    LoadString(hInstance, IDS_MERGEDLGTITLE, szMergeCaption, CharSizeOf(szMergeCaption));
    LoadString(hInstance, IDS_LINKTITLE, szLinkCaption, CharSizeOf(szLinkCaption));
#ifndef OLE_20
    LoadStringA(hInstance, IDS_OBJNAME, szObjFormat, OBJNAMEMAX );
#else
    LoadStringW(hInstance, IDS_OBJNAME, szObjFormat, OBJNAMEMAX );
#endif

    /* Go grab the default page Setup parameters */
    for (i = 0; i < 6; i++)
        LoadString(hInstance, IHEADER+i, chPageText[i], CharSizeOf(chPageText[i]));

    if (!LoadString(hInstance, EINSMEMORY, NotEnoughMem, CharSizeOf(NotEnoughMem)))
        goto InitError;

    _tcsncpy (szMerge, buf, CharSizeOf(szMerge));

    lpDlgProc = MakeProcInstance (DlgProc, hInstance);
    lpfnDial = MakeProcInstance (fnDial, hInstance);
    lpfnAbortProc = MakeProcInstance (fnAbortProc, hInstance);
    lpfnPageDlgProc = MakeProcInstance (PageSetupDlgProc, hInstance);
    lpfnAbortDlgProc = MakeProcInstance (fnAbortDlgProc, hInstance);
    lpfnLinksDlg = MakeProcInstance (fnLinksDlg, hInstance);
    lpfnInvalidLink = MakeProcInstance (fnInvalidLink, hInstance);

    /* unlikely that last one will work but others won't */
    if (!lpfnAbortDlgProc)
        goto InitError;

    fValidate = GetProfileInt (szCardfileSect, szValidateFileWrite, 1);
    hCards = GlobalAlloc (GHND, sizeof(CARDHEADER)); /* alloc first card */
    if (!hCards)
        goto InitError;

    iTopScreenCard = 0;

    /* make a single blank card */
    CurIFile[0] = (TCHAR) 0;        /* file is untitled */
    cCards = 1;
    CurCardHead.line[0] = (TCHAR) 0;
    CurCard.lpObject = NULL;
    CurCardHead.flags = FNEW;
    Cards = (LPCARDHEADER) GlobalLock (hCards);
    Cards[0] = CurCardHead;
    GlobalUnlock (hCards);

    /* this is the main window */

#ifdef JAPAN    /* eight must be less that 400 */
    hwnd = CreateWindow(szCardfileClass, NULL,
        WS_OVERLAPPEDWINDOW, /* | WS_CLIPCHILDREN, */
        CW_USEDEFAULT, 0,
        CardWidth*4/3, min(GetSystemMetrics(SM_CYSCREEN)-GetSystemMetrics(SM_CYBORDER)*2,CardHeight*7/4),
        NULL, NULL, hInstance, NULL);
#else
    hwnd = CreateWindow(szCardfileClass, NULL,
        WS_OVERLAPPEDWINDOW, /* | WS_CLIPCHILDREN, */
        CW_USEDEFAULT, 0,
        CardWidth*4/3, CardHeight*7/4,
        NULL, NULL, hInstance, NULL);
#endif

    if (!hwnd)
    {
InitError:
        MessageBox(hwnd, NotEnoughMem, NULL, MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);
        return FALSE;
    }

    DragAcceptFiles(hwnd, TRUE);

    hMenu = GetMenu(hwnd);
    CheckMenuItem(hMenu,CCARDFILE,MF_CHECKED);

    MakeTempFile();

    /* init fields of the PRINTDLG structure (not used yet) */
    PD.lStructSize    = sizeof(PRINTDLG);
    PD.hwndOwner      = hwnd;
    PD.hDevMode       = NULL;
    PD.hDevNames      = NULL;
    PD.hDC            = NULL;
    PD.nCopies        = 1;


    ShowWindow(hwnd, cmdShow);

    if (ProcessShellOptions (lpszCommandLine))
    {
        PostMessage (hwnd, WM_CLOSE, 0, 0L);
        return TRUE;
    }

/* for "foo.crd", you get 07foo.crd00 with
 * lpszCommandLine pointing at foo.  use length given in the
 * previous byte, rather than searching for a null terminator, because
 * real msdos does not null terminate (it terminates with a CR).
 * above worked except for chars above 128 which it thought were < space.
 * 28-Oct-1987. davidhab.
 *
 * This doesn't work on NT so search for NULL terminator since we WILL have
 * one.
 *
 * Remove quotes while at it.
 */
#if defined(WIN32)
    lpchTmp  = lpszCommandLine;
    pchTmp   = CurIFile;
    while (*lpchTmp)
#else
    for (cbName = (int)(*(lpszCommandLine - 1)),
         i = 0, lpchTmp = lpszCommandLine, pchTmp = CurIFile; i < cbName; i++)
#endif
    if (*lpchTmp == TEXT('"') || *lpchTmp == TEXT('\''))
        lpchTmp++ ;
    else
        *pchTmp++ = *lpchTmp++;

    *pchTmp = (TCHAR) 0;

    if (*CurIFile)
    {
        /* Get a fully qualified pathname */
#if defined(WIN32)
        HANDLE          hFind;
        WIN32_FIND_DATA info;
        LPTSTR          lp;
        TCHAR           szFilePath[MAX_PATH];

        GetFullPathName (CurIFile, CharSizeOf(szFilePath), szFilePath, &lp);

        // replace filename given with file system name
        hFind = FindFirstFile (CurIFile, &info);
        if (hFind != INVALID_HANDLE_VALUE)
        {
           lstrcpy (lp, info.cFileName);
           lstrcpy (CurIFile, szFilePath);
           FindClose (hFind);
        }
#else
        MyOpenFile (CurIFile, CurIFile, OF_PARSE);
#endif
    }

    /* construct default filter string in the required format for
     * the new FileOpen and FileSaveAs dialogs.  Pretty brutish...
     */

    LoadString(hInstance, IDS_FILTERSPEC, pszFilterSpec, FILTERMAX);
    pszFilterSpec += lstrlen (pszFilterSpec) + 1;
    *pszFilterSpec++ = TEXT('*');
    *pszFilterSpec++ = TEXT('.');
    lstrcat(pszFilterSpec, szFileExtension);
    pszFilterSpec += lstrlen(pszFilterSpec) + 1;

    LoadString(hInstance, IDS_FILTERSPEC2, pszFilterSpec, FILTERMAX);
    pszFilterSpec += lstrlen (pszFilterSpec) + 1;
    *pszFilterSpec++ = TEXT('*');
    *pszFilterSpec++ = TEXT('.');
    lstrcat(pszFilterSpec, szFileExtension);
    pszFilterSpec += lstrlen(pszFilterSpec) + 1;

    LoadString(hInstance, IDS_FILTERSPEC3, pszFilterSpec, FILTERMAX);
    pszFilterSpec += lstrlen (pszFilterSpec) + 1;
    *pszFilterSpec++ = TEXT('*');
    *pszFilterSpec++ = TEXT('.');
    lstrcat(pszFilterSpec, szFileExtension);
    pszFilterSpec += lstrlen(pszFilterSpec) + 1;

    LoadString(hInstance, IDS_FILTERSPEC4, pszFilterSpec, FILTERMAX);
    pszFilterSpec += lstrlen (pszFilterSpec) + 1;
    lstrcat(pszFilterSpec, TEXT("*.*"));
    pszFilterSpec += lstrlen(pszFilterSpec) + 1;
    *pszFilterSpec++ = TEXT('\0');
    *szCustFilterSpec = TEXT('\0');

    /* init. some fields of the OPENFILENAME struct used by fileopen and
     * filesaveas
     */
    OFN.lStructSize       = sizeof(OPENFILENAME);
    OFN.hwndOwner         = hwnd;
    OFN.nMaxCustFilter    = FILTERMAX;
    OFN.nFilterIndex      = 1;
    OFN.nMaxFile          = PATHMAX;
    OFN.lpfnHook          = NULL;
    OFN.hInstance         = hInstance;
    OFN.lpstrFileTitle    = NULL;
    *szLastDir            = (TCHAR) 0;

    /* init.fields of the FINDREPLACE struct used by FindText() */
    FR.lStructSize        = sizeof(FINDREPLACE);
    FR.hwndOwner          = hwnd;
    FR.hInstance          = hInstance;
    FR.lpTemplateName     = NULL;
    FR.Flags              = FR_HIDEWHOLEWORD | FR_DOWN;
    FR.lpstrReplaceWith   = NULL;  /* not used by FindText() */
    FR.wReplaceWithLen    = 0;     /* not used by FindText() */
    FR.lpfnHook           = NULL;
    FR.wFindWhatLen       = LINELENGTH;
    if (!(hFind = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE, ByteCountOf(PATHMAX))))
        return FALSE;

    /* determine the message number to be used for communication with
     * Find dialog
     */
    if (!(wFRMsg = RegisterWindowMessage (FINDMSGSTRING)))
        return FALSE;
    if (!(wHlpMsg = RegisterWindowMessage (HELPMSGSTRING)))
        return FALSE;

#ifndef OLE_20
    WideCharToMultiByte (CP_ACP, 0, szCardfile, -1, aszCardfile, 60, NULL, NULL);
    WideCharToMultiByte (CP_ACP, 0, szUntitled, -1, aszUntitled, 60, NULL, NULL);
    WideCharToMultiByte (CP_ACP, 0, CurIFile, -1, aszCurIFile, 60, NULL, NULL);
#endif

    if (*CurIFile)
    {
#ifndef OLE_20
        if (fOLE && OLE_OK != OleRegisterClientDoc (aszCardfile, aszCurIFile, 0L, &lhcdoc))
#else
        if (fOLE && OLE_OK != OleRegisterClientDoc (szCardfile, CurIFile, 0L, &lhcdoc))
#endif
            ErrorMessage (W_FAILED_TO_NOTIFY);

        if (!DoOpen(CurIFile))
            goto FallThrough;
        SetCurCard (iFirstCard);     /* Don't discard too soon */
        SetNumOfCards ();
    }
    else
    {
FallThrough:

        CurIFile[0] = (TCHAR) 0;     /* failed, make untitled */
#ifndef OLE_20
        if (fOLE && OLE_OK != OleRegisterClientDoc (aszCardfile, aszUntitled, 0L, &lhcdoc))
#else
        if (fOLE && OLE_OK != OleRegisterClientDoc (szCardfile, szUntitled, 0L, &lhcdoc))
#endif
            ErrorMessage (W_FAILED_TO_NOTIFY);
    }

    return(TRUE);
}

/*
 * Processes command line params specified by the shell
 *
 * If command line for cardfile has
 *      "/PC <filename>" - print all the cards
 *      "/PL <filename>" - print the list
 */
NOEXPORT BOOL NEAR ProcessShellOptions (LPTSTR lpLine)
{
    TCHAR           PrintCode;
    TCHAR           szFile[PATHMAX];
#if defined(WIN32)
    HANDLE          hFind;
    WIN32_FIND_DATA info;
    LPTSTR          lpTemp;
#endif

    CharUpper(lpLine);

    // skip spaces
    while (*lpLine == TEXT(' ') || *lpLine == TEXT('\t'))
        lpLine++;

    if (lpLine[0] != TEXT('/') || lpLine[1] != TEXT('P'))
        return FALSE;

    lpLine += 2;
    if(*lpLine == TEXT('C') ||  *lpLine == TEXT('L')) /* Code specifies list or cards. */
        PrintCode = *lpLine++;
    else if(*lpLine == TEXT(' '))         /* Deafault print card files. /P */
        PrintCode = TEXT('C');
    else                                  /* Unrecognizable code.          */
        return FALSE;

    /* skip spaces */
    while (*lpLine == TEXT(' ') || *lpLine == TEXT('\t'))
        lpLine++;

    if (!*lpLine)
        return FALSE;

    UpdateWindow(PD.hwndOwner);

    /* Get a fully qualified pathname */
#if defined(WIN32)
    // get rid of trailing quote
    lpTemp = lpLine;

    while (*lpTemp && !(*lpTemp == TEXT('"') || *lpTemp == TEXT('\'')))
       lpTemp++;

    if (*lpTemp)
       *lpTemp = TEXT('\0');

    GetFullPathName (lpLine, CharSizeOf(szFile), szFile, &lpTemp);

    hFind = FindFirstFile (lpLine, &info);
    if (hFind != INVALID_HANDLE_VALUE)
    {
       lstrcpy (lpTemp, info.cFileName);
       FindClose (hFind);
    }
    else
       return (FALSE);
#else
    MyOpenFile (lpLine, szFile, OF_PARSE);
#endif

    if (!OpenNewFile(szFile))
        return TRUE;

    if (PrintCode == TEXT('C'))    /* print cards */
        PrintCards(cCards);
    else
        PrintList();

    return TRUE;
}

/* SetGlobalFont( font )
 *
 * sets the global 'hFont' variable and recalcs the font metrics.
 *
 */

VOID SetGlobalFont( HFONT font, INT iNewPointSize )
{
    HFONT  hOldFont;
    TEXTMETRIC Metrics;
    RECT       rc;                  // rectangle of indexwnd for setting card offset

    iPointSize= iNewPointSize;      // remember for printing

    // if we had a previous font, delete it
    if( hFont )
        DeleteObject( hFont );

    hFont = font;                   // set global font variable

    hOldFont= SelectObject( hDisplayDC, hFont );
    /* Setup the fonts  and find out about them */
    GetTextMetrics( hDisplayDC, &Metrics );
    SelectObject(hDisplayDC, hOldFont);


    // tmHeight may come back positive
    CharFixHeight = abs(Metrics.tmHeight) + Metrics.tmExternalLeading;
    ExtLeading    = Metrics.tmExternalLeading;
    CharFixWidth  = Metrics.tmAveCharWidth;
    ySpacing      = CharFixHeight + 1;
    /* We must add CXBORDER also; Otherwise, the bitmap in Scrollbar
     * control is shrunk by USER and looks ugly;
     * Fix for Bug #8559 --SANKAR-- 01-28-90;
     */
    cxHScrollBar= GetSystemMetrics(SM_CXHSCROLL)+GetSystemMetrics(SM_CXBORDER);
    cyHScrollBar= GetSystemMetrics(SM_CYHSCROLL);

    CardWidth  = (LINELENGTH * CharFixWidth) + 3;
    CardHeight = (CARDLINES * CharFixHeight) + CharFixHeight + 1 + 2 + 2;

    EditWidth  = (LINELENGTH * CharFixWidth) + 1;
    EditHeight = (CARDLINES * CharFixHeight);

    /* make sure at least one header line shows */
    if( GetClientRect( hIndexWnd, &rc ) )
    {
        yFirstCard = max(TOPMARGIN, (rc.bottom-rc.top) - BOTTOMMARGIN - CardHeight);
    } 
    else
    {
        yFirstCard= TOPMARGIN;
    }
    xFirstCard = LEFTMARGIN;

}

/*
 * do one time global initilization for Cardfile
 */

BOOL IndexInit(void)
{
    WNDCLASS    rClass;
    HANDLE      hIndexIcon;

    /* get the resource file info, such as icons, and IT tables */
    hArrowCurs = LoadCursor (NULL, IDC_ARROW);
    hWaitCurs  = LoadCursor (NULL, IDC_WAIT);
    hIndexIcon = LoadIcon (hIndexInstance, (LPTSTR) INDEXICON);
    hAccel     = LoadAccelerators (hIndexInstance, (LPTSTR) MAINACC);
    if (!hArrowCurs || !hWaitCurs || !hIndexIcon || !hAccel)
        return(FALSE);

    /* make rClass */
    rClass.lpszClassName = szCardfileClass;
    rClass.hCursor       = hArrowCurs;    /* normal cursor is arrow */
    rClass.hIcon         = hIndexIcon;
    rClass.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);   // this changes
    rClass.style         = CS_VREDRAW | CS_HREDRAW |
                           CS_DBLCLKS | CS_BYTEALIGNCLIENT;
    rClass.lpfnWndProc   = (WNDPROC) IndexWndProc;
    rClass.hInstance     = hIndexInstance;
    rClass.lpszMenuName  = (LPTSTR) MTINDEX;
    rClass.cbClsExtra    = 0;
    rClass.cbWndExtra    = 0;

    if (!RegisterClass(&rClass))
        return FALSE;

    /* make CardClass */
    rClass.lpszClassName = szCardClass;
    rClass.hCursor       = hArrowCurs;    /* normal cursor is arrow */
    rClass.hIcon         = NULL;
    rClass.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    rClass.style         = CS_HREDRAW | CS_VREDRAW |
                           CS_DBLCLKS | CS_BYTEALIGNCLIENT;
    rClass.lpfnWndProc   = (WNDPROC) CardWndProc;
    rClass.hInstance     = hIndexInstance;
    rClass.lpszMenuName  = NULL;

    if (!RegisterClass(&rClass))
        return FALSE;

    return TRUE;
}

BOOL OleInit(HANDLE hInstance)
{
    HANDLE    hobjStream = INVALID_HANDLE_VALUE;
    HANDLE    hobjClient = INVALID_HANDLE_VALUE;

    vcfLink      = RegisterClipboardFormat(TEXT("ObjectLink"));
    vcfNative    = RegisterClipboardFormat(TEXT("Native"));
    vcfOwnerLink = RegisterClipboardFormat(TEXT("OwnerLink"));
    lpclient     = NULL;
    lpStream     = NULL;

    if (!(hobjClient = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                   sizeof(OLECLIENT))))
            goto Error;

    if (!(lpclient = (LPOLECLIENT)(GlobalLock(hobjClient))))
        goto Error;

    lpclient->lpvtbl = (LPOLECLIENTVTBL)&clientTbl;

    if (!(hobjStream = GlobalAlloc (GMEM_MOVEABLE, sizeof(CARDSTREAM))))
        goto Error;

    lpclient->lpvtbl->CallBack = MakeProcInstance(CallBack, hInstance);

    if (!(lpStream = (LPCARDSTREAM)(GlobalLock(hobjStream))))
        goto Error;

    lpStream->lpstbl = (LPOLESTREAMVTBL)&streamTbl;
    pfOldRead        = (DWORD (FAR PASCAL *)(LPOLESTREAM, LPBYTE, DWORD))
                        MakeProcInstance((FARPROC)ReadOldStream, hInstance);
    pfNewRead =
    streamTbl.Get    = (DWORD (FAR PASCAL *)(LPOLESTREAM, LPBYTE, DWORD))
                        MakeProcInstance((FARPROC)ReadStream, hInstance);
    streamTbl.Put    = (DWORD (FAR PASCAL *)(LPOLESTREAM, OLE_CONST void FAR *, DWORD))
                        MakeProcInstance((FARPROC)WriteStream, hInstance);

    lpStream->hobjStream = hobjStream;
    return TRUE;

Error:
    if (lpStream)
        GlobalUnlock (hobjStream);
    if (hobjStream)
        GlobalFree (hobjStream);
    if (lpclient)
        GlobalUnlock (hobjClient);
    if (hobjClient)
        GlobalFree (hobjClient);

    return FALSE;
}
