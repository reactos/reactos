/**************************************************/
/*                                                */
/*                                                */
/*      Chinese IME Link Dialog Class             */
/*                                                */
/*                                                */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

typedef struct _tagREGWORDSTRUCT {
    HKL   hKL;
    BOOL  bUpdate;
    TCHAR szIMEName[16];
    UINT  uIMENameLen;
    TCHAR szReading[14];
    DWORD dwReadingLen;
} REGWORDSTRUCT;
typedef REGWORDSTRUCT FAR *LPREGWORDSTRUCT;

typedef struct _tagIMELINKREGWORD {
    HIMC          hOldIMC;
    HIMC          hRegWordIMC;
    BOOL          fCompMsg;
    UINT          nEudcIMEs;
    UINT          nCurrIME;
    TCHAR         szEudcCodeString[4];
    REGWORDSTRUCT sRegWordStruct[1];
} IMELINKREGWORD;
typedef IMELINKREGWORD FAR *LPIMELINKREGWORD;

typedef struct _tagIMERADICALRECT {
    UINT nStartIME;
    UINT nPerPageIMEs;
    SIZE lTextSize;
    SIZE lCurrReadingExtent;
    HWND hRegWordButton;
    HWND hScrollWnd;
    RECT rcRadical[1];
} IMERADICALRECT;
typedef IMERADICALRECT FAR *LPIMERADICALRECT;

typedef struct _tagCOUNTRYSETTING {
    UINT    uCodePage;
    LPCTSTR szCodePage;
} COUNTRYSETTING;

#define WM_EUDC_CODE            (WM_USER + 0x0400)
#define WM_EUDC_COMPMSG         (WM_USER + 0x0401)
#define WM_EUDC_SWITCHIME       (WM_USER + 0x0402)
#define WM_EUDC_REGISTER_BUTTON (WM_USER + 0x0403)
#define UPDATE_NONE             0
#define UPDATE_START            1
#define UPDATE_FINISH           2
#define UPDATE_ERROR            3
#define UPDATE_REGISTERED       4
#define GWLP_IMELINKREGWORD     0
#define GWLP_RADICALRECT        (GWLP_IMELINKREGWORD + sizeof(PVOID))
#define UI_MARGIN               3
#define CARET_MARGIN            2
#define RECT_IMENAME            0
#define RECT_RADICAL            1
#define RECT_NUMBER             (RECT_RADICAL + 1)
#define UNICODE_CP              1200
#define BIG5_CP                 950
#define ALT_BIG5_CP             938
#define GB2312_CP               936

#define SIGN_CWIN               0x4E495743
#define SIGN__TBL               0x4C42545F

void	ImeLink( HWND hWnd, UINT uCode, BOOL bUnicodeMode, HINSTANCE hInst);
