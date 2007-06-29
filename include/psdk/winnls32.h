

#ifndef _WINNLS32_
#define _WINNLS32_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _tagDATETIME
{
    WORD year;
    WORD month;
    WORD day;
    WORD hour;
    WORD min;
    WORD sec;
} DATETIME;

typedef struct _tagIMEPROA
{
    HWND hWnd;
    DATETIME InstDate;
    UINT wVersion;
    BYTE szDescription[50];
    BYTE szName[80];
    BYTE szOptions[30];
} IMEPROA,*PIMEPROA,NEAR *NPIMEPROA,FAR *LPIMEPROA;

typedef struct _tagIMEPROW
{
    HWND hWnd;
    DATETIME InstDate;
    UINT wVersion;
    WCHAR szDescription[50];
    WCHAR szName[80];
    WCHAR szOptions[30];
} IMEPROW,*PIMEPROW,NEAR *NPIMEPROW,FAR *LPIMEPROW;

BOOL WINAPI IMPGetIMEA( IN HWND, OUT LPIMEPROA);
BOOL WINAPI IMPQueryIMEA( IN OUT LPIMEPROA);
BOOL WINAPI IMPSetIMEA( IN HWND, IN LPIMEPROA);

BOOL WINAPI IMPGetIMEW( IN HWND, OUT LPIMEPROW);
BOOL WINAPI IMPQueryIMEW( IN OUT LPIMEPROW);
BOOL WINAPI IMPSetIMEW( IN HWND, IN LPIMEPROW);

UINT WINAPI WINNLSGetIMEHotkey( IN HWND);
BOOL WINAPI WINNLSEnableIME( IN HWND, IN BOOL);
BOOL WINAPI WINNLSGetEnableStatus( IN HWND);

#ifdef UNICODE
typedef IMEPROW IMEPRO;
typedef PIMEPROW PIMEPRO;
typedef NPIMEPROW NPIMEPRO;
typedef LPIMEPROW LPIMEPRO;
#define IMPGetIME  IMPGetIMEW
#define IMPQueryIME  IMPQueryIMEW
#define IMPSetIME  IMPSetIMEW
#else
typedef IMEPROA IMEPRO;
typedef PIMEPROA PIMEPRO;
typedef NPIMEPROA NPIMEPRO;
typedef LPIMEPROA LPIMEPRO;
#define IMPGetIME  IMPGetIMEA
#define IMPQueryIME  IMPQueryIMEA
#define IMPSetIME  IMPSetIMEA
#endif

#ifdef __cplusplus
}
#endif

#endif


