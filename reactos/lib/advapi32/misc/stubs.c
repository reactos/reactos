/*
 * Some of these functions may be wrong
 */

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>

WINBOOL
STDCALL
DeregisterEventSource (
		       HANDLE hEventLog
			)
{
  return(FALSE);
}

HANDLE
STDCALL
RegisterEventSourceA (
    LPCSTR lpUNCServerName,
    LPCSTR lpSourceName
    )
{
  return(FALSE);
}

HANDLE
STDCALL
RegisterEventSourceW (
    LPCWSTR lpUNCServerName,
    LPCWSTR lpSourceName
    )
{
  return(FALSE);
}

WINBOOL
STDCALL
ReportEventA (
     HANDLE     hEventLog,
     WORD       wType,
     WORD       wCategory,
     DWORD      dwEventID,
     PSID       lpUserSid,
     WORD       wNumStrings,
     DWORD      dwDataSize,
     LPCSTR   *lpStrings,
     LPVOID     lpRawData
    )
{
  return(FALSE);
}

WINBOOL
STDCALL
ReportEventW (
     HANDLE     hEventLog,
     WORD       wType,
     WORD       wCategory,
     DWORD      dwEventID,
     PSID       lpUserSid,
     WORD       wNumStrings,
     DWORD      dwDataSize,
     LPCWSTR   *lpStrings,
     LPVOID     lpRawData
    )
{
  return(FALSE);
}

WINBOOL
STDCALL
SetFileSecurityW (
    LPCWSTR lpFileName,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
  return(FALSE);
}

/* EOF */
