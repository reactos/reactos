#ifndef _UTIL_H_
#define _UTIL_H_

//** Must use this as initial value for CRC
#define CRC32_INITIAL_VALUE 0L

#define GUID_STR_LEN 40
#define MAXPROVIDERNAME 128

#define MAX_STRING  1024
#define SMALL_BUF   128

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

void * _cdecl operator new(size_t size);
void   _cdecl operator delete(void *ptr);
#if 0
void * malloc(size_t n);
void * calloc(size_t n, size_t s);
void * realloc(void* p, size_t n);
void   free(void* p);
#endif

extern HANDLE g_hHeap;
//
// helper macros
//
#define RegCreate(hk, psz, phk) if (ERROR_SUCCESS != RegCreateKeyEx((hk), psz, 0, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE, NULL, (phk), &dwDummy)) goto CleanUp
#define RegSetStr(hk, psz) if (ERROR_SUCCESS != RegSetValueEx((hk), NULL, 0, REG_SZ, (BYTE*)(psz), lstrlen(psz)+1)) goto CleanUp
#define RegSetStrValue(hk, pszStr, psz)    if(ERROR_SUCCESS != RegSetValueEx((hk), (const char *)(pszStr), 0, REG_SZ, (BYTE*)(psz), lstrlen(psz)+1)) goto CleanUp
#define RegCloseK(hk) RegCloseKey(hk); hk = NULL
#define RegOpenK(hk, psz, phk) if (ERROR_SUCCESS != RegOpenKeyEx(hk, psz, 0, KEY_ALL_ACCESS, phk)) return FALSE


#ifdef UNICODE
   #define MySetText(a, b, c) SetDlgItemText(a, b, c)
#else
   #define MySetText(a, b, c) ConvertAndSetText(a, b, c);
#endif


//=--------------------------------------------------------------------------=
// allocates a temporary buffer that will disappear when it goes out of scope
// NOTE: be careful of that -- make sure you use the string in the same or
// nested scope in which you created this buffer. people should not use this
// class directly.  use the macro(s) below.
//
class TempBuffer {
  public:
    TempBuffer(ULONG cBytes) {
        m_pBuf = (cBytes <= 120) ? &m_szTmpBuf : LocalAlloc(LMEM_FIXED, cBytes);
        m_fHeapAlloc = (cBytes > 120);
    }
    ~TempBuffer() {
        if (m_pBuf && m_fHeapAlloc) LocalFree(m_pBuf);
    }
    void *GetBuffer() {
        return m_pBuf;
    }

  private:
    void *m_pBuf;
    // we'll use this temp buffer for small cases.
    //
    char  m_szTmpBuf[120];
    unsigned m_fHeapAlloc:1;
};

//=--------------------------------------------------------------------------=
// string helpers.
//
// given and ANSI String, copy it into a wide buffer.
// be careful about scoping when using this macro!
//
// how to use the below two macros:
//
//  ...
//  LPSTR pszA;
//  pszA = MyGetAnsiStringRoutine();
//  MAKE_WIDEPTR_FROMANSI(pwsz, pszA);
//  MyUseWideStringRoutine(pwsz);
//  ...
//
// similarily for MAKE_ANSIPTR_FROMWIDE.  note that the first param does not
// have to be declared, and no clean up must be done.
//
#define MAKE_WIDEPTR_FROMANSI(ptrname, ansistr) \
    long __l##ptrname = (lstrlen(ansistr) + 1) * sizeof(WCHAR); \
    TempBuffer __TempBuffer##ptrname(__l##ptrname); \
    MultiByteToWideChar(CP_ACP, 0, ansistr, -1, (LPWSTR)__TempBuffer##ptrname.GetBuffer(), __l##ptrname); \
    LPWSTR ptrname = (LPWSTR)__TempBuffer##ptrname.GetBuffer()

#define MAKE_ANSIPTR_FROMWIDE(ptrname, widestr) \
    long __l##ptrname = (lstrlenW(widestr) + 1) * sizeof(char); \
    TempBuffer __TempBuffer##ptrname(__l##ptrname); \
    WideCharToMultiByte(CP_ACP, 0, widestr, -1, (LPSTR)__TempBuffer##ptrname.GetBuffer(), __l##ptrname, NULL, NULL); \
    LPSTR ptrname = (LPSTR)__TempBuffer##ptrname.GetBuffer()


#define STR_OLESTR 1

#define OLESTRFROMANSI(x)  (LPOLESTR)MakeWideStrFromAnsi((LPSTR)(x))

//------------------------------------------------------------------------------
//
// function prototypes
//
//------------------------------------------------------------------------------

HRESULT CheckTrust(LPSTR szFilename);

BOOL DeleteKeyAndSubKeys(HKEY hkIn, LPSTR pszSubKey);

int StringFromGuid(const CLSID* piid, LPTSTR pszBuf);

void CopyWideStr(LPWSTR pswzTarget, LPWSTR pswzSource);

void ConvertAndSetText(HWND hwnd, UINT resID, LPWSTR pszSource);

LPSTR MakeAnsiStrFromWide(LPWSTR pwsz);

LPWSTR MakeWideStrFromAnsi(LPSTR);

int ErrMsgBox( UINT id, LPCTSTR pszTitle, UINT  mbFlags);

int LoadSz(UINT id, LPTSTR pszBuf, UINT cMaxSize);

HRESULT MakeSureKeyExist( HKEY hKey, LPSTR pSubKey );

/***    CRC32Compute - Compute 32-bit
 *  Entry:
 *      pb    - Pointer to buffer to computer CRC on
 *      cb    - Count of bytes in buffer to CRC
 *      crc32 - Result from previous CRC32Compute call (on first call
 *              to CRC32Compute, must be CRC32_INITIAL_VALUE!!!!).
 *  Exit:
 *      Returns updated CRC value.
 */
ULONG CRC32Compute(BYTE *pb,unsigned cb,ULONG crc32);


#endif // _UTIL_H_
