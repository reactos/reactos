/*
 * Some of these functions may be wrong
 */

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>

/*
 * @unimplemented
 */
WINBOOL
STDCALL
DeregisterEventSource (
		       HANDLE hEventLog
			)
{
  return(FALSE);
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
RegisterEventSourceA (
    LPCSTR lpUNCServerName,
    LPCSTR lpSourceName
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
RegisterEventSourceW (
    LPCWSTR lpUNCServerName,
    LPCWSTR lpSourceName
    )
{
  return(FALSE);
}

/*
 * @unimplemented
 */
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

/*
 * @unimplemented
 */
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

/*
 * @unimplemented
 */
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
