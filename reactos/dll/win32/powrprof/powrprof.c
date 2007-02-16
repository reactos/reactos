/*
 * Copyright (C) 2005 Benjamin Cutler
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */


#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winreg.h"
#include "winternl.h"
#include "powrprof.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "stdlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(powrprof);

/* Notes to implementors:
 * #1: The native implementation of these functions attempted to read in
 * registry entries that I was unable to locate on any of the Windows
 * machines I checked, but I only had desktops available, so maybe
 * laptop users will have better luck. They return FNF errors because
 * that's what the native DLL was returning during my tests.
 * #2: These functions call NtPowerInformation but I don't know what they
 * do with the results, and NtPowerInformation doesn't do much in WINE yet
 * anyway.
 * #3: Since I can't get several other functions working (see note #1),
 * implementing these functions is going to have to wait until somebody can
 * cobble together some sane test input. */

static const WCHAR szPowerCfgSubKey[] = { 'S', 'o', 'f', 't', 'w', 'a', 'r', 'e',
	'\\', 'M', 'i', 'c', 'r', 'o', 's', 'o', 'f', 't', '\\', 'W', 'i',
	'n', 'd', 'o', 'w', 's', '\\', 'C', 'u', 'r', 'r', 'e', 'n', 't',
	'V', 'e', 'r', 's', 'i', 'o', 'n', '\\', 'C', 'o', 'n', 't', 'r',
	'o', 'l', 's', ' ', 'F', 'o', 'l', 'd', 'e', 'r', '\\', 'P', 'o',
	'w', 'e', 'r', 'C', 'f', 'g', 0 };
static const WCHAR szSemaphoreName[] = { 'P', 'o', 'w', 'e', 'r', 'P', 'r', 'o',
	'f', 'i', 'l', 'e', 'R', 'e', 'g', 'i', 's', 't', 'r', 'y', 'S',
	'e', 'm', 'a', 'p', 'h', 'o', 'r', 'e', 0 };
static const WCHAR szDiskMax[] = { 'D', 'i', 's', 'k', 'S', 'p', 'i', 'n', 'd',
	'o', 'w', 'n', 'M', 'a', 'x', 0 };
static const WCHAR szDiskMin[] = { 'D', 'i', 's', 'k', 'S', 'p', 'i', 'n', 'd',
	'o', 'w', 'n', 'M', 'i', 'n', 0 };
static const WCHAR szLastID[] = { 'L', 'a', 's', 't', 'I', 'D', 0 };
static HANDLE PPRegSemaphore = NULL;

NTSTATUS WINAPI CallNtPowerInformation(
	POWER_INFORMATION_LEVEL InformationLevel,
	PVOID lpInputBuffer, ULONG nInputBufferSize,
	PVOID lpOutputBuffer, ULONG nOutputBufferSize)
{
   return NtPowerInformation(InformationLevel, lpInputBuffer,
      nInputBufferSize, lpOutputBuffer, nOutputBufferSize);
}

BOOLEAN WINAPI CanUserWritePwrScheme(VOID)
{
   HKEY hKey = NULL;
   LONG r;
   BOOLEAN bSuccess = TRUE;

   TRACE("()\n");

   r = RegOpenKeyExW(HKEY_LOCAL_MACHINE, szPowerCfgSubKey, 0, KEY_READ | KEY_WRITE, &hKey);

   if (r != ERROR_SUCCESS) {
      TRACE("RegOpenKeyEx failed: %ld\n", r);
      bSuccess = FALSE;
   }

   SetLastError(r);
   RegCloseKey(hKey);
   return bSuccess;
}

BOOLEAN WINAPI DeletePwrScheme(UINT uiIndex)
{
   /* FIXME: See note #1 */
   FIXME("(%d) stub!\n", uiIndex);
   SetLastError(ERROR_FILE_NOT_FOUND);
   return FALSE;
}

BOOLEAN WINAPI EnumPwrSchemes(PWRSCHEMESENUMPROC lpfnPwrSchemesEnumProc,
			LPARAM lParam)
{
   /* FIXME: See note #1 */
   FIXME("(%p, %ld) stub!\n", lpfnPwrSchemesEnumProc, lParam);
   SetLastError(ERROR_FILE_NOT_FOUND);
   return FALSE;
}

BOOLEAN WINAPI GetActivePwrScheme(PUINT puiID)
{
   /* FIXME: See note #1 */
   FIXME("(%p) stub!\n", puiID);
   SetLastError(ERROR_FILE_NOT_FOUND);
   return FALSE;
}

BOOLEAN WINAPI GetCurrentPowerPolicies(
	PGLOBAL_POWER_POLICY pGlobalPowerPolicy,
	PPOWER_POLICY pPowerPolicy)
{
   /* FIXME: See note #2 */
   SYSTEM_POWER_POLICY ACPower, DCPower;

   FIXME("(%p, %p) stub!\n", pGlobalPowerPolicy, pPowerPolicy);

   NtPowerInformation(SystemPowerPolicyAc, 0, 0, &ACPower, sizeof(SYSTEM_POWER_POLICY));
   NtPowerInformation(SystemPowerPolicyDc, 0, 0, &DCPower, sizeof(SYSTEM_POWER_POLICY));

   return FALSE;
}

BOOLEAN WINAPI GetPwrCapabilities(
	PSYSTEM_POWER_CAPABILITIES lpSystemPowerCapabilities)
{
   NTSTATUS r;

   TRACE("(%p)\n", lpSystemPowerCapabilities);

   r = NtPowerInformation(SystemPowerCapabilities, 0, 0, lpSystemPowerCapabilities, sizeof(SYSTEM_POWER_CAPABILITIES));

   SetLastError(RtlNtStatusToDosError(r));

   return r == STATUS_SUCCESS;
}

BOOLEAN WINAPI GetPwrDiskSpindownRange(PUINT RangeMax, PUINT RangeMin)
{
   HKEY hKey;
   BYTE lpValue[40];
   LONG r;
   DWORD cbValue = sizeof(lpValue);

   TRACE("(%p, %p)\n", RangeMax, RangeMin);

   if (RangeMax == NULL || RangeMin == NULL) {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   SetLastError(ERROR_SUCCESS);

   WaitForSingleObject(PPRegSemaphore, INFINITE);

   r = RegOpenKeyExW(HKEY_LOCAL_MACHINE, szPowerCfgSubKey, 0, KEY_READ, &hKey);
   if (r != ERROR_SUCCESS) {
      TRACE("RegOpenKeyEx failed: %ld\n", r);
      TRACE("Using defaults: 3600, 3\n");
      *RangeMax = 3600;
      *RangeMin = 3;
      ReleaseSemaphore(PPRegSemaphore, 1, NULL);
      return TRUE;
   }

   r = RegQueryValueExW(hKey, szDiskMax, 0, 0, lpValue, &cbValue);
   if (r != ERROR_SUCCESS) {
      TRACE("Couldn't open DiskSpinDownMax: %ld\n", r);
      TRACE("Using default: 3600\n");
      *RangeMax = 3600;
   } else {
      *RangeMax = atoiW((LPCWSTR)lpValue);
   }

   cbValue = sizeof(lpValue);

   r = RegQueryValueExW(hKey, szDiskMin, 0, 0, lpValue, &cbValue);
   if (r != ERROR_SUCCESS) {
      TRACE("Couldn't open DiskSpinDownMin: %ld\n", r);
      TRACE("Using default: 3\n");
      *RangeMin = 3;
   } else {
      *RangeMin = atoiW((LPCWSTR)lpValue);
   }

   RegCloseKey(hKey);

   ReleaseSemaphore(PPRegSemaphore, 1, NULL);

   return TRUE;
}

BOOLEAN WINAPI IsAdminOverrideActive(PADMINISTRATOR_POWER_POLICY p)
{
   FIXME("( %p) stub!\n", p);
   return FALSE;
}

BOOLEAN WINAPI IsPwrHibernateAllowed(VOID)
{
   /* FIXME: See note #2 */
   SYSTEM_POWER_CAPABILITIES PowerCaps;
   FIXME("() stub!\n");
   NtPowerInformation(SystemPowerCapabilities, NULL, 0, &PowerCaps, sizeof(PowerCaps));
   return FALSE;
}

BOOLEAN WINAPI IsPwrShutdownAllowed(VOID)
{
   /* FIXME: See note #2 */
   SYSTEM_POWER_CAPABILITIES PowerCaps;
   FIXME("() stub!\n");
   NtPowerInformation(SystemPowerCapabilities, NULL, 0, &PowerCaps, sizeof(PowerCaps));
   return FALSE;
}

BOOLEAN WINAPI IsPwrSuspendAllowed(VOID)
{
   /* FIXME: See note #2 */
   SYSTEM_POWER_CAPABILITIES PowerCaps;
   FIXME("() stub!\n");
   NtPowerInformation(SystemPowerCapabilities, NULL, 0, &PowerCaps, sizeof(PowerCaps));
   return FALSE;
}

BOOLEAN WINAPI ReadGlobalPwrPolicy(PGLOBAL_POWER_POLICY pGlobalPowerPolicy)
{
   /* FIXME: See note #1 */
   FIXME("(%p) stub!\n", pGlobalPowerPolicy);
   SetLastError(ERROR_FILE_NOT_FOUND);
   return FALSE;
}

BOOLEAN WINAPI ReadProcessorPwrScheme(UINT uiID,
			PMACHINE_PROCESSOR_POWER_POLICY pMachineProcessorPowerPolicy)
{
   /* FIXME: See note #1 */
   FIXME("(%d, %p) stub!\n", uiID, pMachineProcessorPowerPolicy);
   SetLastError(ERROR_FILE_NOT_FOUND);
   return FALSE;
}

BOOLEAN WINAPI ReadPwrScheme(UINT uiID,
	PPOWER_POLICY pPowerPolicy)
{
   /* FIXME: See note #1 */
   FIXME("(%d, %p) stub!\n", uiID, pPowerPolicy);
   SetLastError(ERROR_FILE_NOT_FOUND);
   return FALSE;
}

BOOLEAN WINAPI SetActivePwrScheme(UINT uiID,
	PGLOBAL_POWER_POLICY lpGlobalPowerPolicy,
	PPOWER_POLICY lpPowerPolicy)
{
   /* FIXME: See note #1 */
   FIXME("(%d, %p, %p) stub!\n", uiID, lpGlobalPowerPolicy, lpPowerPolicy);
   SetLastError(ERROR_FILE_NOT_FOUND);
   return FALSE;
}

BOOLEAN WINAPI SetSuspendState(BOOLEAN Hibernate, BOOLEAN ForceCritical,
	BOOLEAN DisableWakeEvent)
{
   /* FIXME: I have NO idea how you're supposed to call NtInitiatePowerAction
    * here, because it's not a documented function that I can find */
   FIXME("(%d, %d, %d) stub!\n", Hibernate, ForceCritical, DisableWakeEvent);
   return TRUE;
}

BOOLEAN WINAPI WriteGlobalPwrPolicy(PGLOBAL_POWER_POLICY pGlobalPowerPolicy)
{
   /* FIXME: See note #3 */
   FIXME("(%p) stub!\n", pGlobalPowerPolicy);
   SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
   return FALSE;
}

BOOLEAN WINAPI WriteProcessorPwrScheme(UINT ID,
	PMACHINE_PROCESSOR_POWER_POLICY pMachineProcessorPowerPolicy)
{
   /* FIXME: See note #3 */
   FIXME("(%d, %p) stub!\n", ID, pMachineProcessorPowerPolicy);
   SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
   return FALSE;
}

BOOLEAN WINAPI WritePwrScheme(PUINT puiID, LPWSTR lpszName, LPWSTR lpszDescription,
	PPOWER_POLICY pPowerPolicy)
{
   /* FIXME: See note #3 */
   FIXME("(%p, %s, %s, %p) stub!\n", puiID, debugstr_w(lpszName), debugstr_w(lpszDescription), pPowerPolicy);
   SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
   return FALSE;
}

BOOLEAN WINAPI ValidatePowerPolicies(PGLOBAL_POWER_POLICY pGPP, PPOWER_POLICY pPP)
{
   /* FIXME: See note #3 */
   FIXME("(%p, %p) stub!\n", pGPP, pPP);
   SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
   return TRUE;

}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
   FIXME("(%p, %ld, %p) not fully implemented\n", hinstDLL, fdwReason, lpvReserved);

   switch(fdwReason) {
      case DLL_PROCESS_ATTACH: {

         HKEY hKey;
         LONG r;

         DisableThreadLibraryCalls(hinstDLL);

         r = RegOpenKeyExW(HKEY_LOCAL_MACHINE, szPowerCfgSubKey, 0, KEY_READ | KEY_WRITE, &hKey);

         if (r != ERROR_SUCCESS) {
            TRACE("Couldn't open registry key HKLM\\%s, using some sane(?) defaults\n", debugstr_w(szPowerCfgSubKey));
         } else {
            BYTE lpValue[40];
            DWORD cbValue = sizeof(lpValue);
            r = RegQueryValueExW(hKey, szLastID, 0, 0, lpValue, &cbValue);
            if (r != ERROR_SUCCESS) {
               TRACE("Couldn't open registry entry HKLM\\%s\\LastID, using some sane(?) defaults\n", debugstr_w(szPowerCfgSubKey));
            }
            RegCloseKey(hKey);
         }

         PPRegSemaphore = CreateSemaphoreW(NULL, 1, 1, szSemaphoreName);
         if (PPRegSemaphore == NULL) {
            ERR("Couldn't create Semaphore: %ld\n", GetLastError());
            return FALSE;
         }
         break;
      }
      case DLL_PROCESS_DETACH:
         CloseHandle(PPRegSemaphore);
         break;
    }
    return TRUE;
}
