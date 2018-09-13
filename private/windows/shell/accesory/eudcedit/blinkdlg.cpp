/**************************************************/
/*                                                */
/*                                                */
/*      Chinese IME Batch Mode                    */
/*              (Dialogbox)                       */
/*                                                */
/*                                                */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

#include    "stdafx.h"
#include    "eudcedit.h"
#if 1 // use function in imeblink.c!
#include    "imeblink.h"
#endif
#include    "blinkdlg.h"
#include    "util.h"

#define     SIGN_CWIN       0x4E495743      /* Sign of CWin */
#define     SIGN__TBL       0x4C42545F      /* Sign of IME Table */

#if 0 // move to imeblink.c!

#define     UNICODE_CP      1200

#define     BIG5_CP         950
#define     ALT_BIG5_CP     938
#define     GB2312_CP       936

typedef struct _tagCOUNTRYSETTING {
    UINT    uCodePage;
    LPCTSTR szCodePage;
} COUNTRYSETTING;

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

#endif // move to imeblink.c!

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/****************************************/
/*                                      */
/*            Constructor               */
/*                                      */
/****************************************/
CBLinkDlg::CBLinkDlg(   CWnd *pParent)
    : CDialog( CBLinkDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CBLinkDlg)
    //}}AFX_DATA_INIT
}

/****************************************/
/*                                      */
/*      MESSAGE "WM_INITDIALOG"         */
/*                                      */
/****************************************/
BOOL
CBLinkDlg::OnInitDialog()
{
    CString DlgTtl;
    CDialog::OnInitDialog();

//  Increment contexthelp mark "?" in dialog caption.
//    LONG WindowStyle = GetWindowLong( this->GetSafeHwnd(), GWL_EXSTYLE);
//    WindowStyle |= WS_EX_CONTEXTHELP;
//    SetWindowLong( this->GetSafeHwnd(), GWL_EXSTYLE, WindowStyle);

//  Set dialog title name.
    DlgTtl.LoadString( IDS_BATCHLNK_DLGTITLE);
    this->SetWindowText( DlgTtl);

    return TRUE;
}

/****************************************/
/*                                      */
/*      COMMAND "BROWSE"                */
/*                                      */
/****************************************/
void
CBLinkDlg::OnBrowsetable()
{
    OPENFILENAME    ofn;
    CString         DlgTtl, DlgMsg;
    CString         sFilter;
    TCHAR           chReplace;
    TCHAR           szFilter[64];
    TCHAR           szFileName[MAX_PATH];
    TCHAR           szDirName[MAX_PATH];
    TCHAR           szTitleName[MAX_PATH];

//  Check size of IME batch table structure
    if( sizeof( USRDICIMHDR) != 256){
        OutputMessageBox( this->GetSafeHwnd(),
            IDS_INTERNAL_TITLE,
            IDS_INTERNAL_MSG, TRUE);
        return;
    }

//  Set filter of file( from string table)
    GetStringRes(szFilter, IDS_BATCHIME_FILTER);
    int StringLength = lstrlen( szFilter);

    chReplace = szFilter[StringLength-1];
    for( int i = 0; szFilter[i]; i++){
        if( szFilter[i] == chReplace)
            szFilter[i] = '\0';
    }

    GetSystemWindowsDirectory( szDirName, sizeof(szDirName));
    lstrcpy( szFileName, TEXT("*.TBL"));
    DlgTtl.LoadString( IDS_BROWSETABLE_DLGTITLE);

//  Set data in structure of OPENFILENAME
    ofn.lStructSize = sizeof( OPENFILENAME);
    ofn.hwndOwner = this->GetSafeHwnd();
    ofn.lpstrFilter = szFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 0;
    ofn.lpstrFileTitle = szTitleName;
    ofn.nMaxFileTitle = sizeof( szTitleName) / sizeof(TCHAR);
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = sizeof( szFileName) / sizeof(TCHAR);
    ofn.lpstrInitialDir = szDirName;
    ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR
        | OFN_CREATEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = NULL;
    ofn.lpstrTitle = DlgTtl;

    if( !GetOpenFileName( &ofn))
        return;

    this->SetDlgItemText( IDC_IMETABLE, ofn.lpstrFile);
    CWnd *cWnd = GetDlgItem( IDOK);
    GotoDlgCtrl( cWnd);
}

/****************************************/
/*                                      */
/*      COMMAND "IDOK"                  */
/*                                      */
/****************************************/
void
CBLinkDlg::OnOK()
{
    if( !RegistStringTable())
        return;

    CDialog::OnOK();
}

/****************************************/
/*                                      */
/*      Register reading string         */
/*                                      */
/****************************************/
BOOL
CBLinkDlg::RegistStringTable()
{
    LPUSRDICIMHDR lpIsvUsrDic;
    HANDLE        hIsvUsrDicFile, hIsvUsrDic;
    DWORD         dwSize, dwFileSize;
    BOOL          stFunc;
    TCHAR         szTableFile[MAX_PATH];
    TCHAR         szFileName[MAX_PATH];
    CString       DlgMsg, DlgTtl;

    this->GetDlgItemText( IDC_IMETABLE, szTableFile, sizeof( szTableFile));
    lstrcpy( szFileName, TEXT("*.TBL"));

//  Create file( to read)
    hIsvUsrDicFile = CreateFile( szTableFile, GENERIC_READ, 0, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if( hIsvUsrDicFile == INVALID_HANDLE_VALUE){
        DlgTtl.LoadString( IDS_NOTOPEN_TITLE);
        DlgMsg.LoadString( IDS_NOTOPEN_MSG);
        this->MessageBox( DlgMsg, DlgTtl, MB_OK | MB_ICONHAND |
            MB_TASKMODAL | MB_TOPMOST);
        return FALSE;
    }

#if 0
    for( i = 0; i < sizeof( szFileName); i++){
        if( szFileName[i] == '\\'){
            szFileName[i] = ' ';
        }
    }
#endif

//  Create file mapping( read only)
    hIsvUsrDic = CreateFileMapping((HANDLE)hIsvUsrDicFile, NULL,
        PAGE_READONLY, 0, 0, NULL);
    if( !hIsvUsrDic){
        stFunc = FALSE;
        DlgTtl.LoadString( IDS_NOTOPEN_TITLE);
        DlgMsg.LoadString( IDS_NOTOPEN_MSG);
        this->MessageBox( DlgMsg, DlgTtl, MB_OK | MB_ICONHAND |
            MB_TASKMODAL | MB_TOPMOST);
        goto BatchCloseUsrDicFile;
    }

//  Set view file
    lpIsvUsrDic = (LPUSRDICIMHDR)MapViewOfFile( hIsvUsrDic,
        FILE_MAP_READ, 0, 0, 0);
    if( !lpIsvUsrDic){
        stFunc = FALSE;
        DlgTtl.LoadString( IDS_NOTOPEN_TITLE);
        DlgMsg.LoadString( IDS_NOTOPEN_MSG);
        this->MessageBox( DlgMsg, DlgTtl, MB_OK | MB_ICONHAND |
            MB_TASKMODAL | MB_TOPMOST);
        goto BatchCloseUsrDic;
    }
		
    dwSize = lpIsvUsrDic->ulTableCount * ( sizeof(WORD) + sizeof(WORD)
        + lpIsvUsrDic->cMethodKeySize) + 256;
    dwFileSize = GetFileSize( hIsvUsrDicFile, (LPDWORD)NULL);

#if 0
    dwSize = dwFileSize;
#endif

//  Check table file data
    if( dwSize != dwFileSize){
        stFunc = FALSE;
        OutputMessageBox( this->GetSafeHwnd(),
            IDS_FILESIZE_MSGTITLE,
            IDS_FILESIZE_MSG, TRUE);
    }else if( lpIsvUsrDic->uHeaderSize != 256){
        stFunc = FALSE;
        OutputMessageBox( this->GetSafeHwnd(),
            IDS_FILEHEADER_MSGTITLE,
            IDS_FILEHEADER_MSG, TRUE);
    }else if( lpIsvUsrDic->uInfoSize != 13){
        stFunc = FALSE;
        OutputMessageBox( this->GetSafeHwnd(),
            IDS_INMETHOD_MSGTITLE,
            IDS_INMETHOD_MSG, TRUE);
    }else if( CodePageInfo( lpIsvUsrDic->idCP) == -1){
        stFunc = FALSE;
        OutputMessageBox( this->GetSafeHwnd(),
            IDS_CODEPAGE_MSGTITLE,
            IDS_CODEPAGE_MSG, TRUE);
    }else if( *(DWORD UNALIGNED *)lpIsvUsrDic->idUserCharInfoSign != SIGN_CWIN){
        stFunc = FALSE;
        OutputMessageBox( this->GetSafeHwnd(),
            IDS_SIGN_MSGTITLE,
            IDS_SIGN_MSG, TRUE);
    }else if( *(DWORD UNALIGNED *)((LPBYTE)lpIsvUsrDic->idUserCharInfoSign +
        sizeof(DWORD)) != SIGN__TBL){
        stFunc = FALSE;
        OutputMessageBox( this->GetSafeHwnd(),
            IDS_SIGN_MSGTITLE,
            IDS_SIGN_MSG, TRUE);
    }else{
        stFunc = TRUE;
        if( !RegisterTable( this->GetSafeHwnd(),
            lpIsvUsrDic, dwFileSize, lpIsvUsrDic->idCP)){
            OutputMessageBox( this->GetSafeHwnd(),
                IDS_UNMATCHED_TITLE,
                IDS_UNMATCHED_MSG, TRUE);
            stFunc = FALSE;
        }
    }
    UnmapViewOfFile( lpIsvUsrDic);
					
BatchCloseUsrDic:
    CloseHandle( hIsvUsrDic);

BatchCloseUsrDicFile:
    CloseHandle( hIsvUsrDicFile);

    return stFunc;
}

#if 0 // move to imeblink.c!

/****************************************/
/*                                      */
/*      Register reading string         */
/*                                      */
/****************************************/
HKL
CBLinkDlg::RegisterTable(
HWND            hWnd,
LPUSRDICIMHDR   lpIsvUsrDic,
DWORD           dwFileSize,
UINT            uCodePage)
{
    HKL     hKL;
    DWORD   i;
    LPBYTE  lpCurr, lpEnd;
    BOOL    fRet;
    TCHAR   szStr[16];
    WORD    wInternalCode[256];
    WORD    wAltInternalCode[256];

#ifdef UNICODE
    if (uCodePage == UNICODE_CP) {
        LPUNATSTR lpszMethodName;

        lpszMethodName = (LPUNATSTR)lpIsvUsrDic->achMethodName;

        for (i = 0; i < sizeof(lpIsvUsrDic->achMethodName) / sizeof(TCHAR); i++) {
            szStr[i] = *lpszMethodName++;
        }

        szStr[i] = '\0';
    } else {
        UINT uLen;

        uLen = MultiByteToWideChar(uCodePage, MB_PRECOMPOSED,
            (LPCSTR)lpIsvUsrDic->achMethodName,
            sizeof(lpIsvUsrDic->achMethodName),
            szStr,
            sizeof(szStr) / sizeof(TCHAR));
        szStr[uLen] = '\0';
    }
#else
    for( i = 0; i < sizeof( lpIsvUsrDic->achMethodName); i++){
        szStr[i] = lpIsvUsrDic->achMethodName[i];
    }

    szStr[i] = '\0';
#endif

    hKL = MatchImeName( szStr);

    if( !hKL) return (hKL);

    for( i = 0; i < sizeof(wInternalCode) / sizeof(WORD); i++) {
        LRESULT lRet;

        lRet = ImmEscape(hKL, (HIMC)NULL,
            IME_ESC_SEQUENCE_TO_INTERNAL, &i);

        if (HIWORD(lRet) == 0xFFFF) {
            // This is caused by sign extent in Win9x in the return value of
            // ImmEscape, it causes an invalid internal code.
            wAltInternalCode[i] = 0;
        } else {
            wAltInternalCode[i] = HIWORD(lRet);
        }


#ifndef UNICODE
        if( wAltInternalCode[i] > 0xFF) {
            wAltInternalCode[i] = LOBYTE(wAltInternalCode[i]) << 8 |
            HIBYTE(wAltInternalCode[i]);
        }

        if( wInternalCode[i] > 0xFF) {
            wInternalCode[i] = LOBYTE(wInternalCode[i]) << 8 |
            HIBYTE(wInternalCode[i]);
        }
#endif
    }

    lpCurr = (LPBYTE)(lpIsvUsrDic + 1) + sizeof(WORD);
    lpEnd = (LPBYTE)lpIsvUsrDic + dwFileSize;

    for (; lpCurr < lpEnd; lpCurr += sizeof(WORD) + lpIsvUsrDic->cMethodKeySize + sizeof(WORD)) {
        int j;

        *(LPDWORD)szStr = 0;

#ifdef UNICODE
        if (uCodePage == UNICODE_CP) {
            szStr[0] = *(LPUNATSTR)lpCurr;
        } else {
            CHAR szMultiByte[4];

            szMultiByte[0] = HIBYTE(*(LPUNASTR)lpCurr);
            szMultiByte[1] = LOBYTE(*(LPUNASTR)lpCurr);


            MultiByteToWideChar(uCodePage, MB_PRECOMPOSED,
                szMultiByte, 2, szStr, 2);
        }
#else
        szStr[1] = *lpCurr;
        szStr[0] = *(lpCurr + 1);
#endif

        for (i = 0, j = 0; i < lpIsvUsrDic->cMethodKeySize; i++) {
            if (!wAltInternalCode[*(LPBYTE)(lpCurr + sizeof(WORD) + i)]) {
            } else if (wAltInternalCode[*(LPBYTE)(lpCurr + sizeof(WORD) + i)] < 0xFF) {
                *(LPTSTR)&szStr[4 + j] = (TCHAR)
                    wAltInternalCode[*(LPBYTE)(lpCurr + sizeof(WORD) + i)];
                j += sizeof(TCHAR) / sizeof(TCHAR);
            } else {
                *(LPWSTR)&szStr[4 + j] = (WCHAR)
                    wAltInternalCode[*(LPBYTE)(lpCurr + sizeof(WORD) + i)];
                j += sizeof(WCHAR) / sizeof(TCHAR);
            }

            if (wInternalCode[*(LPBYTE)(lpCurr + sizeof(WORD) + i)] < 0xFF) {
                *(LPTSTR)&szStr[4 + j] = (TCHAR)
                    wInternalCode[*(LPBYTE)(lpCurr + sizeof(WORD) + i)];
                j += sizeof(TCHAR) / sizeof(TCHAR);
            } else {
                *(LPWSTR)&szStr[4 + j] = (WCHAR)
                    wInternalCode[*(LPBYTE)(lpCurr + sizeof(WORD) + i)];
                j += sizeof(WCHAR) / sizeof(TCHAR);
            }
        }

        szStr[4 + j] = szStr[4 + j + 1] = szStr[4 + j + 2] = '\0';

        fRet = ImmRegisterWord(hKL, &szStr[4], IME_REGWORD_STYLE_EUDC,
            szStr);
    }

    return (hKL);
}

/****************************************/
/*                                      */
/*      Get keyboard layout handle      */
/*                                      */
/****************************************/
HKL
CBLinkDlg::MatchImeName(
LPCTSTR szStr)
{
    TCHAR   szImeName[16];
    int     nLayout;
    HKL     hKL;
    HGLOBAL hMem;
    HKL FAR *lpMem;
    int i;

    nLayout = GetKeyboardLayoutList( 0, NULL);

    hMem = GlobalAlloc( GHND, sizeof(HKL) * nLayout);
    if( !hMem)  return NULL;

    lpMem = (HKL FAR *)GlobalLock( hMem);
    if( !lpMem){
        GlobalFree( hMem);
        return NULL;
    }

    GetKeyboardLayoutList( nLayout, lpMem);

    for( i = 0; i < nLayout; i++){
        BOOL fRet;

        hKL = *(lpMem + i);
        for( j = 0; j < uLen; j++){
            szImeName[j] = '\0';
        }
        fRet = ImmEscape(hKL, (HIMC)NULL, IME_ESC_IME_NAME, szImeName);
        if( !fRet)  continue;

        if(lstrcmp(szStr, szImeName) == 0){
            goto MatchOvr;
        }
    }

    hKL = NULL;

MatchOvr:
    GlobalUnlock( hMem);
    GlobalFree( hMem);

    return( hKL);
}

/****************************************/
/*                                      */
/*      CodePage Info                   */
/*                                      */
/****************************************/
int
CBLinkDlg::CodePageInfo(
    UINT uCodePage)
{
    int i;

    for (i = 0; i < sizeof(sCountry) / sizeof(COUNTRYSETTING); i++) {
        if (sCountry[i].uCodePage == uCodePage) {
            return(i);
        }
    }

    return (-1);
}

#endif // move to imeblink.c!

static DWORD    aIds[] =
{
    IDC_STATICIMETBL, IDH_EUDC_BLINK_EDITTBL,
    IDC_IMETABLE, IDH_EUDC_BLINK_EDITTBL,
    IDC_BROWSETABLE, IDH_EUDC_BROWSE,
    0, 0
};

/****************************************/
/*                                      */
/*      Window Procedure                */
/*                                      */
/****************************************/		
LRESULT
CBLinkDlg::WindowProc(
UINT    message,
WPARAM  wParam,
LPARAM  lParam)
{/*
    if( message == WM_HELP){
        ::WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
            HelpPath, HELP_WM_HELP, (DWORD_PTR)(LPDWORD)aIds);
        return(0);
    }
    if( message == WM_CONTEXTMENU){
        ::WinHelp((HWND)wParam, HelpPath,
            HELP_CONTEXTMENU, (DWORD_PTR)(LPDWORD)aIds);
        return(0);
    }
  */
    return CDialog::WindowProc(message, wParam, lParam);
}

BEGIN_MESSAGE_MAP(CBLinkDlg, CDialog)
    //{{AFX_MSG_MAP(CBLinkDlg)
    ON_BN_CLICKED(IDC_BROWSETABLE, OnBrowsetable)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

