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

#ifdef UNICODE
typedef IMEPROW IMEPRO;
typedef PIMEPROW PIMEPRO;
typedef NPIMEPROW NPIMEPRO;
typedef LPIMEPROW LPIMEPRO;
#define IMPGetIME IMPGetIMEW
#define IMPQueryIME IMPQueryIMEW
#define IMPSetIME IMPSetIMEW
#else
typedef IMEPROA IMEPRO;
typedef PIMEPROA PIMEPRO;
typedef NPIMEPROA NPIMEPRO;
typedef LPIMEPROA LPIMEPRO;
#define IMPGetIME IMPGetIMEA
#define IMPQueryIME IMPQueryIMEA
#define IMPSetIME IMPSetIMEA
#endif

BOOL WINAPI IMPGetIMEA(HWND, LPIMEPROA);
BOOL WINAPI IMPGetIMEW(HWND, LPIMEPROW);
BOOL WINAPI IMPQueryIMEA(LPIMEPROA);
BOOL WINAPI IMPQueryIMEW(LPIMEPROW);
BOOL WINAPI IMPSetIMEA(HWND, LPIMEPROA);
BOOL WINAPI IMPSetIMEW(HWND, LPIMEPROW);
UINT WINAPI WINNLSGetIMEHotkey(HWND);
BOOL WINAPI WINNLSEnableIME(HWND, BOOL);
BOOL WINAPI WINNLSGetEnableStatus(HWND);

#ifdef __cplusplus
}
#endif

#endif /* _USERENV_H */
