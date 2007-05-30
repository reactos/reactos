/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/misc/hwprofiles.c
 * PURPOSE:         advapi32.dll Hardware Functions
 * PROGRAMMER:      Steven Edwards
 * UPDATE HISTORY:
 *	20042502
 */

#include <advapi32.h>
#include <debug.h>

/******************************************************************************
 * GetCurrentHwProfileA [ADVAPI32.@]
 *
 * Get the current hardware profile.
 *
 * PARAMS
 *  lpHwProfileInfo [O] Destination for hardware profile information.
 *
 * RETURNS
 *  Success: TRUE. lpHwProfileInfo is updated with the hardware profile details.
 *  Failure: FALSE.
 *
 * @implemented
 */
BOOL STDCALL
GetCurrentHwProfileA(LPHW_PROFILE_INFOA lpHwProfileInfo)
{
  HW_PROFILE_INFOW ProfileInfo;
  UNICODE_STRING StringU;
  ANSI_STRING StringA;
  BOOL bResult;
  NTSTATUS Status;

  DPRINT("GetCurrentHwProfileA() called\n");

  bResult = GetCurrentHwProfileW(&ProfileInfo);
  if (bResult == FALSE)
    return FALSE;

  lpHwProfileInfo->dwDockInfo = ProfileInfo.dwDockInfo;

  /* Convert the profile GUID to ANSI */
  StringU.Buffer = (PWCHAR)ProfileInfo.szHwProfileGuid;
  StringU.Length = wcslen(ProfileInfo.szHwProfileGuid) * sizeof(WCHAR);
  StringU.MaximumLength = HW_PROFILE_GUIDLEN * sizeof(WCHAR);
  StringA.Buffer = (PCHAR)&lpHwProfileInfo->szHwProfileGuid;
  StringA.Length = 0;
  StringA.MaximumLength = HW_PROFILE_GUIDLEN;
  Status = RtlUnicodeStringToAnsiString(&StringA,
					&StringU,
					FALSE);
  if (!NT_SUCCESS(Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return FALSE;
    }

  /* Convert the profile name to ANSI */
  StringU.Buffer = (PWCHAR)ProfileInfo.szHwProfileName;
  StringU.Length = wcslen(ProfileInfo.szHwProfileName) * sizeof(WCHAR);
  StringU.MaximumLength = MAX_PROFILE_LEN * sizeof(WCHAR);
  StringA.Buffer = (PCHAR)&lpHwProfileInfo->szHwProfileName;
  StringA.Length = 0;
  StringA.MaximumLength = MAX_PROFILE_LEN;
  Status = RtlUnicodeStringToAnsiString(&StringA,
					&StringU,
					FALSE);
  if (!NT_SUCCESS(Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return FALSE;
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
GetCurrentHwProfileW(LPHW_PROFILE_INFOW lpHwProfileInfo)
{
  WCHAR szKeyName[256];
  HKEY hDbKey;
  HKEY hProfileKey;
  DWORD dwLength;
  DWORD dwConfigId;

  DPRINT("GetCurrentHwProfileW() called\n");

  if (lpHwProfileInfo == NULL)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
    }

  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
		    L"System\\CurrentControlSet\\Control\\IDConfigDB",
		    0,
		    KEY_QUERY_VALUE,
		    &hDbKey))
    {
      SetLastError(ERROR_REGISTRY_CORRUPT);
      return FALSE;
    }

  dwLength = sizeof(DWORD);
  if (RegQueryValueExW(hDbKey,
		       L"CurrentConfig",
		       0,
		       NULL,
		       (LPBYTE)&dwConfigId,
		       &dwLength))
    {
      RegCloseKey(hDbKey);
      SetLastError(ERROR_REGISTRY_CORRUPT);
      return FALSE;
    }

  swprintf(szKeyName,
	   L"Hardware Profile\\%04lu",
	   dwConfigId);

  if (RegOpenKeyExW(hDbKey,
		    szKeyName,
		    0,
		    KEY_QUERY_VALUE | KEY_SET_VALUE,
		    &hProfileKey))
    {
      RegCloseKey(hDbKey);
      SetLastError(ERROR_REGISTRY_CORRUPT);
      return FALSE;
    }

  dwLength = sizeof(DWORD);
  if (RegQueryValueExW(hProfileKey,
		       L"DockState",
		       0,
		       NULL,
		       (LPBYTE)&lpHwProfileInfo->dwDockInfo,
		       &dwLength))
    {
      lpHwProfileInfo->dwDockInfo =
	DOCKINFO_DOCKED | DOCKINFO_UNDOCKED | DOCKINFO_USER_SUPPLIED;
    }

  dwLength = HW_PROFILE_GUIDLEN * sizeof(WCHAR);
  if (RegQueryValueExW(hProfileKey,
		       L"HwProfileGuid",
		       0,
		       NULL,
		       (LPBYTE)&lpHwProfileInfo->szHwProfileGuid,
		       &dwLength))
    {
      /* FIXME: Create a new GUID */
      wcscpy(lpHwProfileInfo->szHwProfileGuid,
	     L"{00000000-0000-0000-0000-000000000000}");

      dwLength = (wcslen(lpHwProfileInfo->szHwProfileGuid) + 1) * sizeof(WCHAR);
      RegSetValueExW(hProfileKey,
		     L"HwProfileGuid",
		     0,
		     REG_SZ,
		     (LPBYTE)lpHwProfileInfo->szHwProfileGuid,
		     dwLength);
    }

  dwLength = MAX_PROFILE_LEN * sizeof(WCHAR);
  if (RegQueryValueExW(hProfileKey,
		       L"FriendlyName",
		       0,
		       NULL,
		       (LPBYTE)&lpHwProfileInfo->szHwProfileName,
		       &dwLength))
    {
      wcscpy(lpHwProfileInfo->szHwProfileName,
	     L"Noname Hardware Profile");
      dwLength = (wcslen(lpHwProfileInfo->szHwProfileName) + 1) * sizeof(WCHAR);
      RegSetValueExW(hProfileKey,
		     L"FriendlyName",
		     0,
		     REG_SZ,
		     (LPBYTE)lpHwProfileInfo->szHwProfileName,
		     dwLength);
    }

  RegCloseKey(hProfileKey);
  RegCloseKey(hDbKey);

  return TRUE;
}

/* EOF */
