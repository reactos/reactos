#ifndef _SECEXT_H
#define _SECEXT_H
#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#ifndef RC_INVOKED
#if (_WIN32_WINNT >= 0x0500)
typedef enum 
{
  NameUnknown = 0, 
  NameFullyQualifiedDN = 1, 
  NameSamCompatible = 2, 
  NameDisplay = 3, 
  NameUniqueId = 6, 
  NameCanonical = 7, 
  NameUserPrincipal = 8, 
  NameCanonicalEx = 9, 
  NameServicePrincipal = 10, 
  NameDnsDomain = 12
} EXTENDED_NAME_FORMAT, *PEXTENDED_NAME_FORMAT;

BOOLEAN WINAPI GetComputerObjectNameA(EXTENDED_NAME_FORMAT,LPSTR,PULONG);
BOOLEAN WINAPI GetComputerObjectNameW(EXTENDED_NAME_FORMAT,LPWSTR,PULONG);
BOOLEAN WINAPI GetUserNameExA(EXTENDED_NAME_FORMAT,LPSTR,PULONG);
BOOLEAN WINAPI GetUserNameExW(EXTENDED_NAME_FORMAT,LPWSTR,PULONG);
BOOLEAN WINAPI TranslateNameA(LPCSTR,EXTENDED_NAME_FORMAT,EXTENDED_NAME_FORMAT,LPSTR,PULONG);
BOOLEAN WINAPI TranslateNameW(LPCWSTR,EXTENDED_NAME_FORMAT,EXTENDED_NAME_FORMAT,LPWSTR,PULONG);

#ifdef UNICODE
#define GetComputerObjectName GetComputerObjectNameW
#define GetUserNameEx GetUserNameExW
#define TranslateName TranslateNameW
#else
#define GetComputerObjectName GetComputerObjectNameA
#define GetUserNameEx GetUserNameExA
#define TranslateName TranslateNameA
#endif


#endif /* ! RC_INVOKED */
#endif /* _WIN32_WINNT >= 0x0500 */
#endif /* ! _SECEXT_H */
