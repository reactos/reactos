//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Returns TRUE if the string is unique.
typedef BOOL (CALLBACK *PFNISUNIQUE)(LPCTSTR lpsz, UINT nUser);
extern HWND g_hwndProgress;

//---------------------------------------------------------------------------
#define LOGGING

#ifdef LOGGING
void __cdecl _Log(LPCTSTR pszMsg, ...);
#define Log _Log
#else
#define Log 1 ? (void)0 : (void)
#endif

//---------------------------------------------------------------------------
#define Reg_SetStruct(hkey, pszSubKey, pszValue, pData, cbData) Reg_Set(hkey, pszSubKey, pszValue, REG_BINARY, pData, cbData)
#define Reg_SetString(hkey, pszSubKey, pszValue, pszString) Reg_Set(hkey, pszSubKey, pszValue, REG_SZ, (LPTSTR)pszString,(lstrlen(pszString)+1)* SIZEOF(TCHAR))
#define Reg_GetString(hkey, pszSubKey, pszValue, pszString, cbString) Reg_Get(hkey, pszSubKey, pszValue, pszString, cbString)
#define Reg_GetStruct(hkey, pszSubKey, pszValue, pData, cbData) Reg_Get(hkey, pszSubKey, pszValue, pData, cbData)

//---------------------------------------------------------------------------
void    Group_SetProgress(int i);
void    Group_SetProgressNameAndRange(LPCTSTR lpszGroup, int iMax);
void    Group_CreateProgressDlg(void);
void    Group_DestroyProgressDlg(void);
void    ConvertHashesToNulls(LPTSTR p);
int     MyMessageBox(HWND hwnd, UINT idTitle, UINT idMessage, LPCTSTR lpsz, UINT nStyle);
#ifdef UNICODE
LPTSTR  fgets(LPTSTR sz, DWORD cb, HANDLE fh);
#else
LPTSTR   fgets(LPTSTR sz, WORD cb, int fh);
#endif
void    ShellRegisterApp(LPCTSTR lpszExt, LPCTSTR lpszTypeKey, LPCTSTR lpszTypeValue, LPCTSTR lpszCommand, BOOL fOveride);
// BOOL         NEAR PASCAL WritePrivateProfileInt(LPCSTR lpszSection, LPCSTR lpszValue, int i, LPCSTR lpszIniFile);
void    Group_SetProgressDesc(UINT nID);

HRESULT ICoCreateInstance(REFCLSID rclsid, REFIID riid, LPVOID * ppv);
BOOL    WINAPI YetAnotherMakeUniqueName(LPTSTR pszNewName, UINT cbNewName, LPCTSTR pszOldName, PFNISUNIQUE pfnIsUnique, UINT n, BOOL fLFN);
BOOL    WINAPI MakeUniqueName(LPTSTR pszNewName, UINT cbNewName, LPCTSTR pszOldName, UINT nStart, PFNISUNIQUE pfnIsUnique, UINT nUser, BOOL fLFN);
BOOL    WINAPI Reg_SetDWord(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, DWORD dw);
BOOL    WINAPI Reg_GetDWord(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, LPDWORD pdw);
BOOL    WINAPI Reg_Set(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, DWORD dwType, LPVOID pData, DWORD cbData);
BOOL    WINAPI Reg_Get(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, LPVOID pData, DWORD cbData);
