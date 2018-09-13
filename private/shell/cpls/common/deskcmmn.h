#ifndef _DESKCMMN_H
#define _DESKCMMN_H


//==========================================================================
//                              Macros
//==========================================================================

#define SZ_REGISTRYMACHINE  TEXT("\\REGISTRY\\MACHINE\\")
#define SZ_PRUNNING_MODE    TEXT("PruningMode")

//This macro casts a TCHAR to be a 32 bit value with the hi word set to 0
// it is useful for calling CharUpper().
#define CHARTOPSZ(ch)       ((LPTSTR)(DWORD)(WORD)(ch))


#define DCDSF_DYNA (0x0001)
#define DCDSF_ASK  (0x0002)

#define DCDSF_PROBABLY      (DCDSF_ASK  | DCDSF_DYNA)
#define DCDSF_PROBABLY_NOT  (DCDSF_ASK  |          0)
#define DCDSF_YES           (0          | DCDSF_DYNA)
#define DCDSF_NO            (0          |          0)


#define REGSTR_VAL_DYNASETTINGSCHANGE TEXT("DynaSettingsChange")


//==========================================================================
//                              Functions
//==========================================================================

// LPTSTR SubStrEnd(LPTSTR pszTarget, LPTSTR pszScan )
//
// If pszScan starts with pszTarget, then the function returns the first
// char of pszScan that follows the pszTarget; other wise it returns pszScan.
//
// eg: SubStrEnd("abc", "abcdefg" ) ==> "defg"
//     SubStrEnd("abc", "abZQRT" ) ==> "abZQRT"
LPTSTR SubStrEnd(LPTSTR pszTarget, LPTSTR pszScan);


BOOL GetDeviceRegKey(LPCTSTR pstrDeviceKey, HKEY* phKey, BOOL* pbReadOnly);


int GetDisplayCPLPreference(LPCTSTR szRegVal);


int GetDynaCDSPreference();


void SetDisplayCPLPreference(LPCTSTR szRegVal, int val);


LONG WINAPI MyStrToLong(LPCTSTR sz);


#endif // _DESKCMMN_H
