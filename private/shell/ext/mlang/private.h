#ifndef _PRIVATE_H_
#define _PRIVATE_H_

#define _OLEAUT32_      // get DECLSPEC_IMPORT stuff right for oleaut32.h, we are defing these

#ifdef STRICT
#undef STRICT
#endif

#define STRICT
#pragma warning(disable:4514) // unreferenced inline function has been removed

#include <windows.h>
#include <ole2.h>
#include <advpub.h>
#include <ccstock.h>
#include <debug.h>
#include <inetreg.h>
#include <mlang.h>
#include <urlmon.h> // for JIT stuff

#include "mimedb.h"
#include "enumcp.h"
#include "resource.h"

#include "detect.h"     // LCDETECT
#include "font.h"

//
//  Function prototypes
//
#if defined(__cplusplus)
extern "C" HRESULT WINAPI ConvertINetReset(void);
#else
HRESULT WINAPI ConvertINetReset(void);
#endif
HRESULT WINAPI ConvertINetStringInIStream(LPDWORD lpdwMode, DWORD dwSrcEncoding, DWORD dwDstEncoding, IStream *pstmIn, IStream *pstmOut, DWORD dwFlag, WCHAR *lpFallBack);
HRESULT WINAPI ConvertINetUnicodeToMultiByteEx(LPDWORD lpdwMode, DWORD dwEncoding, LPCWSTR lpSrcStr, LPINT lpnWideCharCount, LPSTR lpDstStr, LPINT lpnMultiCharCount, DWORD dwFlag, WCHAR *lpFallBack);
HRESULT WINAPI ConvertINetMultiByteToUnicodeEx(LPDWORD lpdwMode, DWORD dwEncoding, LPCSTR lpSrcStr, LPINT lpnMultiCharCount, LPWSTR lpDstStr, LPINT lpnWideCharCount, DWORD dwFlag, WCHAR *lpFallBack);
HRESULT WINAPI _DetectInputCodepage(DWORD dwFlag, DWORD uiPrefWinCodepage, CHAR *pSrcStr, INT *pcSrcSize, DetectEncodingInfo *lpEncoding, INT *pnScoores);
HRESULT WINAPI _DetectCodepageInIStream(DWORD dwFlag, DWORD uiPrefWinCodepage, IStream *pstmIn, DetectEncodingInfo *lpEncoding, INT *pnScoores);

void CMLangFontLink_FreeGlobalObjects(void);
int _LoadStringExW(HMODULE, UINT, LPWSTR, int, WORD);

HRESULT RegularizePosLen(long lStrLen, long* plPos, long* plLen);
HRESULT LocaleToCodePage(LCID locale, UINT* puCodePage);
HRESULT StartEndConnection(IUnknown* const pUnkCPC, const IID* const piid, IUnknown* const pUnkSink, DWORD* const pdwCookie, DWORD dwCookie);

HRESULT RegisterServerInfo(void);
HRESULT UnregisterServerInfo(void);

// Legacy registry MIME DB code, keep it for backward compatiblility
BOOL MimeDatabaseInfo(void);

void DllAddRef(void);
void DllRelease(void);

// JIT langpack stuff
HRESULT InstallIEFeature(HWND hWnd, CLSID *clsid, DWORD dwfIODControl);
HRESULT _GetJITClsIDForCodePage(UINT uiCodePage, CLSID *clsid );
HRESULT _AddFontForCP(UINT uiCP);
HRESULT _ValidateCPInfo(UINT uiCP);
HRESULT _InstallNT5Langpack(HWND hwnd, UINT uiCP);
LANGID GetNT5UILanguage(void);
BOOL    _IsValidCodePage(UINT uiCodePage);
BOOL    _IsKOI8RU(unsigned char *pStr, int nSize);
HRESULT  IsNTLangpackAvailable(UINT uiCodePage);
HRESULT _IsCodePageInstallable(UINT uiCodePage);

// String functions
WCHAR *MLStrCpyNW(WCHAR *strDest, const WCHAR *strSource, int nCount);
WCHAR *MLStrCpyW(WCHAR *strDest, const WCHAR *strSource);
int MLStrCmpIW( const wchar_t *string1, const wchar_t *string2 );
int MLStrCmpI(LPCTSTR pwsz1, LPCTSTR pwsz2);
LPTSTR MLPathCombine(LPTSTR szPath, LPTSTR szPath1, LPTSTR szPath2);
LPTSTR MLStrCpyN(LPTSTR pstrDest, const LPTSTR pstrSource, UINT nCount);
LPTSTR MLStrStr(const LPTSTR Str, const LPTSTR subStr);
DWORD HexToNum(LPTSTR lpsz);
LPTSTR MLStrChr( const TCHAR *string, int c );
BOOL AnsiFromUnicode(LPSTR * ppszAnsi, LPCWSTR pwszWide, LPSTR pszBuf, int cchBuf);
int WINAPI MLStrToIntW(LPCWSTR lpSrc);
int WINAPI MLStrToIntA(LPCSTR lpSrc);
int MLStrCmpNI(LPCTSTR pstr1, LPCTSTR pstr2, int nChar);
int MLStrCmpNIA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar);
int MLStrCmpNIW(LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar);
UINT MLGetWindowsDirectory(LPTSTR lpBuffer, UINT uSize);
int LowAsciiStrCmpNIA(LPCSTR  lpstr1, LPCSTR lpstr2, int count);

int CALLBACK EnumFontFamExProc(ENUMLOGFONTEX *lpelf, NEWTEXTMETRICEX *lpntm, int iFontType, LPARAM lParam);
INT_PTR CALLBACK LangpackDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#ifdef UNICODE
#define MLStrToInt MLStrToIntW
#else
#define MLStrToInt MLStrToIntA
#endif

//
//  Globals
//
extern HINSTANCE    g_hInst;
extern HINSTANCE    g_hUrlMon;
extern UINT         g_cRfc1766;
extern PRFC1766INFOA g_pRfc1766Reg;

extern CRITICAL_SECTION g_cs;

extern BOOL g_bIsNT5;
extern BOOL g_bIsNT;
extern BOOL g_bIsWin98;

#ifdef  __cplusplus

extern LCDetect * g_pLCDetect; // LCDETECT

#endif  // __cplusplus

//
//  Macros
//
#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#define VERIFY(f) AssertE(f)


#define ASSIGN_IF_FAILED(hr, exp) {HRESULT hrTemp = (exp); if (FAILED(hrTemp) && SUCCEEDED(hr)) (hr) = hrTemp;}

#define ASSERT_READ_PTR(p) ASSERT(!::IsBadReadPtr((p), sizeof(*p)))
#define ASSERT_READ_PTR_OR_NULL(p) ASSERT(!(p) || !::IsBadReadPtr((p), sizeof(*p)))
#define ASSERT_WRITE_PTR(p) ASSERT(!::IsBadWritePtr((p), sizeof(*p)))
#define ASSERT_WRITE_PTR_OR_NULL(p) ASSERT(!(p) || !::IsBadWritePtr((p), sizeof(*p)))
#define ASSERT_READ_BLOCK(p,s) ASSERT(!::IsBadReadPtr((p), sizeof(*p) * (s)))
#define ASSERT_READ_BLOCK_OR_NULL(p,s) ASSERT(!(p) || !::IsBadReadPtr((p), sizeof(*p) * (s)))
#define ASSERT_WRITE_BLOCK(p,s) ASSERT(!::IsBadWritePtr((p), sizeof(*p) * (s)))
#define ASSERT_WRITE_BLOCK_OR_NULL(p,s) ASSERT(!(p) || !::IsBadWritePtr((p), sizeof(*p) * (s)))
#define ASSERT_TSTR_PTR(p) ASSERT(!::IsBadStringPtr((p), (UINT)-1))
#define ASSERT_TSTR_PTR_OR_NULL(p) ASSERT(!(p) || !::IsBadStringPtr((p), (UINT)-1))
#define ASSERT_WSTR_PTR(p) ASSERT(!::IsBadStringPtrW((p), (UINT)-1))
#define ASSERT_WSTR_PTR_OR_NULL(p) ASSERT(!(p) || !::IsBadStringPtrW((p), (UINT)-1))
#define ASSERT_STR_PTR(p) ASSERT(!::IsBadStringPtrA((p), (UINT)-1))
#define ASSERT_STR_PTR_OR_NULL(p) ASSERT(!(p) || !::IsBadStringPtrA((p), (UINT)-1))
#define ASSERT_CODE_PTR(p) ASSERT(!::IsBadCodePtr((FARPROC)(p)))
#define ASSERT_CODE_PTR_OR_NULL(p) ASSERT(!(p) || !::IsBadCodePtr((FARPROC)(p)))
#define ASSERT_THIS ASSERT_WRITE_PTR(this)

#ifdef NEWMLSTR
// Error Code
#define FACILITY_MLSTR                  0x0A15
#define MLSTR_E_ACCESSDENIED            MAKE_HRESULT(1, FACILITY_MLSTR, 2001)
#define MLSTR_E_BUSY                    MAKE_HRESULT(1, FACILITY_MLSTR, 2002)
#define MLSTR_E_TOOMANYNESTOFLOCK       MAKE_HRESULT(1, FACILITY_MLSTR, 1003)
#define MLSTR_E_STRBUFNOTAVAILABLE      MAKE_HRESULT(1, FACILITY_MLSTR, 1004)

#define MLSTR_LOCK_TIMELIMIT            100
#define MLSTR_CONF_MAX                  0x40000000
#define MAX_LOCK_COUNT                  4
#endif

#define BIT_HEADER_CHARSET              0x1
#define BIT_BODY_CHARSET                0x2
#define BIT_WEB_CHARSET                 0x4
#define BIT_WEB_FIXED_WIDTH_FONT        0x8 
#define BIT_PROPORTIONAL_FONT           0x10
#define BIT_DESCRIPTION                 0x20
#define BIT_FAMILY                      0x40
#define BIT_LEVEL                       0x80
#define BIT_ENCODING                    0x100

#define BIT_DEL_HEADER_CHARSET          0x10000
#define BIT_DEL_BODY_CHARSET            0x20000
#define BIT_DEL_WEB_CHARSET             0x40000
#define BIT_DEL_WEB_FIXED_WIDTH_FONT    0x80000 
#define BIT_DEL_PROPORTIONAL_FONT       0x100000
#define BIT_DEL_DESCRIPTION             0x200000
#define BIT_DEL_FAMILY                  0x400000
#define BIT_DEL_LEVEL                   0x800000
#define BIT_DEL_ENCODING                0x1000000

#define BIT_CODEPAGE                    0x1
#define BIT_INTERNET_ENCODING           0x2
#define BIT_ALIAS_FOR_CHARSET           0x4

#define DETECTION_MAX_LEN               20*1024     // Limit max auto-detect length to 20k
#define IS_DIGITA(ch)    InRange(ch, '0', '9')
#define IS_DIGITW(ch)    InRange(ch, L'0', L'9')
#define IS_CHARA(ch)     (InRange(ch, 'a', 'z') && InRange(ch, 'A', 'Z'))
#define IS_ISCII_CP(x)   (InRange(x, 57002, 57011))

// Internal define for K1 Hanja support
// In future version of MLang, we might need to update this bit define if there is a conflict with system define
#define FS_MLANG_K1HANJA 0x10000000L

#ifdef UNIX // Add some type that's not defined in UNIX SDK
typedef WORD UWORD;
#endif

#define REG_KEY_NT5LPK                    TEXT("W2KLpk")
#endif  // _PRIVATE_H_
