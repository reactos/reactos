/*
 * util.h - Utility routines description.
 */


/* Prototypes
 *************/

/* util .c */

extern BOOL IsPathDirectory(PCSTR);
extern BOOL KeyExists(HKEY, PCSTR);

BOOL 
StrToIntExW(
    LPCWSTR   pwszString,
    DWORD     dwFlags,          // STIF_ bitfield 
    int FAR * piRet);
BOOL 
StrToIntExA(
    LPCSTR    pszString,
    DWORD     dwFlags,          // STIF_ bitfield 
    int FAR * piRet);
#ifdef UNICODE
#define StrToIntEx  StrToIntExW
#else
#define StrToIntEx  StrToIntExA
#endif 

// Avoid conflict with Nashville commctrl
#ifdef STIF_SUPPORT_HEX
#undef STIF_DEFAULT
#undef STIF_SUPPORT_HEX
#endif

// Flags for StrToIntEx
#define STIF_DEFAULT        0x00000000L
#define STIF_SUPPORT_HEX    0x00000001L

int
StrSpnW(
    LPCWSTR psz,
    LPCWSTR pszSet);
int
StrSpnA(
    LPCSTR psz,
    LPCSTR pszSet);
#ifdef UNICODE
#define StrSpn      StrSpnW
#else
#define StrSpn      StrSpnA
#endif 


LPWSTR
StrPBrkW(
    IN LPCWSTR psz,
    IN LPCWSTR pszSet);
LPSTR
StrPBrkA(
    LPCSTR psz,
    LPCSTR pszSet);
#ifdef UNICODE
#define StrPBrk     StrPBrkW
#else
#define StrPBrk     StrPBrkA
#endif 


/* Win95 Kernel only stubs lstrcpyW.  Memphis Kernel supports it.  
** Use SHLWAPI's version.
*/

#define lstrcpyW    StrCpyW


#ifdef DEBUG

extern BOOL IsStringContained(PCSTR, PCSTR);

#endif

