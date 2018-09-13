/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1996
*
*  TITLE:       BATSTATE.C
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        17 Oct, 1996
*
*  DESCRIPTION:
*   BATSTATE.C contains helper function which maintain the global battery
*   state list.
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <commctrl.h>

#include <dbt.h>           
#include <devioctl.h>
#include <ntpoapi.h>
#include <poclass.h>

#include "powrprof.h"
#include "batmeter.h"

// Simulated battery only for debug build.
#ifndef DEBUG
#undef SIM_BATTERY
#endif

/*******************************************************************************
*
*                     G L O B A L    D A T A
*
*******************************************************************************/

// Global battery state list. This list has the composite system battery state
// as it's always present head. individual battery devices are linked to this
// head. Use WalkBatteryState(ALL, ... to walk the entire list, including the
// head. Use WalkBatteryState(DEVICES, ... to walk just the device list. If a
// battery is in this list, it's displayable. g_ulBatCount is the count of
// battery devices in this list. The composite battery is not counted.

extern BATTERY_STATE   g_bs;
extern ULONG           g_ulBatCount;
extern HWND            g_hwndBatMeter;

#ifdef WINNT
/*******************************************************************************
*
*  RegisterForDeviceNotification
*
*  DESCRIPTION:
*    Do registration for WM_DEVICECHANGED.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL RegisterForDeviceNotification(PBATTERY_STATE pbs)
{
   DEV_BROADCAST_HANDLE dbh;

   memset(&dbh, 0, sizeof(DEV_BROADCAST_HANDLE));

   dbh.dbch_size        = sizeof(DEV_BROADCAST_HANDLE);
   dbh.dbch_devicetype  = DBT_DEVTYP_HANDLE;
   dbh.dbch_handle      = pbs->hDevice;
   
   if (!g_hwndBatMeter) {
      DebugPrint( "RegisterForDeviceNotification, NULL g_hwndBatMeter");
      return FALSE;
   }

   pbs->hDevNotify = RegisterDeviceNotification(g_hwndBatMeter,
                                                &dbh, 
                                                DEVICE_NOTIFY_WINDOW_HANDLE);
   
   if (!pbs->hDevNotify) { 
      DebugPrint( "RegisterDeviceNotification failed");
      return FALSE;
   }
   return TRUE;
}

/*******************************************************************************
*
*  UnregisterForDeviceNotification
*
*  DESCRIPTION:
*    
*
*  PARAMETERS:
*
*******************************************************************************/

void UnregisterForDeviceNotification(PBATTERY_STATE pbs)
{
   if (pbs->hDevNotify) {
      UnregisterDeviceNotification(pbs->hDevNotify);
      pbs->hDevNotify = NULL;
   }
}
#endif

/*******************************************************************************
*
*  SystemPowerStatusToBatteryState
*
*  DESCRIPTION:
*   Fill in BATTERY_STATE fields based on passed SYSTEM_POWER_STATUS.
*
*  PARAMETERS:
*
*******************************************************************************/

void SystemPowerStatusToBatteryState(
    LPSYSTEM_POWER_STATUS lpsps,
    PBATTERY_STATE pbs
)
{
    pbs->ulPowerState = 0;
    if (lpsps->ACLineStatus == AC_LINE_ONLINE) {
        pbs->ulPowerState |= BATTERY_POWER_ON_LINE;
    }
    if (lpsps->BatteryFlag & BATTERY_FLAG_CHARGING) {
        pbs->ulPowerState |= BATTERY_CHARGING;
    }
    if (lpsps->BatteryFlag & BATTERY_FLAG_CRITICAL) {
        pbs->ulPowerState |= BATTERY_CRITICAL;
    }
    pbs->ulBatLifePercent = lpsps->BatteryLifePercent;
    pbs->ulBatLifeTime    = lpsps->BatteryLifeTime;
}

/*******************************************************************************
*
* WalkBatteryState
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL WalkBatteryState(
    PBATTERY_STATE pbsStart,
    WALKENUMPROC pfnWalkEnumProc,
    HWND hWnd,
    LPARAM lParam1,
    LPARAM lParam2
)
{
    PBATTERY_STATE pbsTmp;

    while (pbsStart) {
        // Save the next entry in case the current one is deleted.
        pbsTmp = pbsStart->bsNext;
        if (!pfnWalkEnumProc(pbsStart, hWnd, lParam1, lParam2)) {
            return FALSE;
        }
        pbsStart = pbsTmp;
    }

    return TRUE;
}

/*******************************************************************************
*
* UpdateBatInfoProc
*
*  DESCRIPTION:
*   Updates battery information for an individual battery device.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL UpdateBatInfoProc(
    PBATTERY_STATE pbs,
    HWND hWnd,
    LPARAM lParam1,
    LPARAM lParam2
)
{
    DWORD                       dwByteCount, dwIOCTL, dwWait;
    BATTERY_STATUS              bs;
    BATTERY_WAIT_STATUS         bws;
    BATTERY_INFORMATION         bi;
    BATTERY_QUERY_INFORMATION   bqi;

    if (pbs->hDevice == INVALID_HANDLE_VALUE) {
        DebugPrint( "UpdateBatInfoProc, Bad battery driver handle, LastError: 0x%X", GetLastError());
        return FALSE;
    }

    // If no tag, then don't update the battery info.
    dwIOCTL = IOCTL_BATTERY_QUERY_TAG;
    dwWait = 0;
    if (DeviceIoControl(pbs->hDevice, dwIOCTL,
                        &dwWait, sizeof(dwWait),
                        &(pbs->ulTag), sizeof(ULONG),
                        &dwByteCount, NULL)) {

        bqi.BatteryTag = pbs->ulTag;
        bqi.InformationLevel = BatteryInformation;
        bqi.AtRate = 0;
        
        dwIOCTL = IOCTL_BATTERY_QUERY_INFORMATION;
        if (DeviceIoControl(pbs->hDevice, dwIOCTL,
                            &bqi, sizeof(bqi),
                            &bi,  sizeof(bi),
                            &dwByteCount, NULL)) {

            if (bi.FullChargedCapacity != UNKNOWN_CAPACITY) {
                pbs->ulFullChargedCapacity = bi.FullChargedCapacity;
            }
            else {
                pbs->ulFullChargedCapacity = bi.DesignedCapacity;
            }

            memset(&bws, 0, sizeof(BATTERY_WAIT_STATUS));
            bws.BatteryTag = pbs->ulTag;
            dwIOCTL = IOCTL_BATTERY_QUERY_STATUS;
            if (DeviceIoControl(pbs->hDevice, dwIOCTL,
                                &bws, sizeof(BATTERY_WAIT_STATUS),
                                &bs,  sizeof(BATTERY_STATUS),
                                &dwByteCount, NULL)) {

                pbs->ulPowerState = bs.PowerState;
                if (pbs->ulFullChargedCapacity < bs.Capacity) {
                    pbs->ulFullChargedCapacity = bs.Capacity;
                    DebugPrint( "UpdateBatInfoProc, unable to calculate ulFullChargedCapacity");
                }
                if (pbs->ulFullChargedCapacity == 0) {
                    pbs->ulBatLifePercent = 0;
                }
                else {
                    pbs->ulBatLifePercent =
                        (100 * bs.Capacity) / pbs->ulFullChargedCapacity;
                }
                return TRUE;
            }
        }
    }
    else {
        pbs->ulTag = BATTERY_TAG_INVALID;

        // No battery tag, that's ok, the user may have removed the battery.
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            return TRUE;
        }
    }
    DebugPrint( "UpdateBatInfoProc, IOCTL: %X Failure, BatNum: %d, LastError: %d\n", dwIOCTL, pbs->ulBatNum, GetLastError());
    return FALSE;
}

/*******************************************************************************
*
* SimUpdateBatInfoProc
*
*  DESCRIPTION:
*   Simulate the update of battery information for an individual batter device.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL SimUpdateBatInfoProc(
    PBATTERY_STATE pbs,
    HWND hWnd,
    LPARAM lParam1,
    LPARAM lParam2
)
{
    pbs->ulTag = pbs->ulBatNum;
    if (pbs->ulBatNum == 1) {
        pbs->ulFullChargedCapacity  = 2000;
        pbs->ulFullChargedCapacity  = 1991;
        pbs->ulPowerState           = BATTERY_CHARGING | BATTERY_POWER_ON_LINE;
        pbs->ulBatLifePercent       =   75;
    }
    else {
        pbs->ulFullChargedCapacity  = 3000;
        pbs->ulFullChargedCapacity  = 2991;
        pbs->ulPowerState           = BATTERY_DISCHARGING | BATTERY_CRITICAL;
        pbs->ulBatLifePercent       =  3;
    }
    return TRUE;
}

/*******************************************************************************
*
*  AddBatteryStateDevice
*
*  DESCRIPTION:
*   Add only displayable batteries to the battery list. New entry is appended
*   to battery state list.
*
*  PARAMETERS:
*
*******************************************************************************/

PBATTERY_STATE AddBatteryStateDevice(LPTSTR lpszName, ULONG ulBatNum)
{
    PBATTERY_STATE  pbs, pbsTemp = &g_bs;
    LPTSTR          lpsz = NULL;

    if (!lpszName) {
        return NULL;
    }

    // Append to end of list
    while (pbsTemp->bsNext) {
        pbsTemp = pbsTemp->bsNext;
    }

    // Allocate storage for new battery device state.
    if (pbs = LocalAlloc(LPTR, sizeof(BATTERY_STATE))) {
        if (lpsz = LocalAlloc(0, STRSIZE(lpszName))) {
            lstrcpy(lpsz, lpszName);
            pbs->lpszDeviceName = lpsz;
            pbs->ulSize = sizeof(BATTERY_STATE);
            pbs->ulBatNum = ulBatNum;

            // Open a handle to the battery driver.
            pbs->hDevice = CreateFile(lpszName,
                                      GENERIC_READ | GENERIC_WRITE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL, OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL, NULL);
#ifdef WINNT
            // Setup for notification by PNP when battery goes away. 
            RegisterForDeviceNotification(pbs);
#endif
            // Get the current battery info from the battery driver.
            if (UpdateBatInfoProc(pbs, NULL, 0, 0)) {

                // Link the new battery device state into the list.
                pbsTemp->bsNext = pbs;
                pbs->bsPrev = pbsTemp;
                return pbs;
            }
            LocalFree(lpsz);
        }
        LocalFree(pbs);
    }
    return NULL;
}

/*******************************************************************************
*
*  SimAddBatteryStateDevice
*
*  DESCRIPTION:
*   Simulate the addition of displayable batteries to the battery list.
*   New entry is appended to battery state list.
*
*  PARAMETERS:
*
*******************************************************************************/

PBATTERY_STATE SimAddBatteryStateDevice(LPTSTR lpszName, ULONG ulBatNum)
{
    PBATTERY_STATE  pbs, pbsTemp = &g_bs;
    LPTSTR          lpsz = NULL;

    if (!lpszName) {
        return NULL;
    }

    // Append to end of list
    while (pbsTemp->bsNext) {
        pbsTemp = pbsTemp->bsNext;
    }

    // Allocate storage for new battery device state.
    if (pbs = LocalAlloc(LPTR, sizeof(BATTERY_STATE))) {
        if (lpsz = LocalAlloc(0, STRSIZE(lpszName))) {
            lstrcpy(lpsz, lpszName);
            pbs->lpszDeviceName = lpsz;
            pbs->ulSize = sizeof(BATTERY_STATE);
            pbs->ulBatNum = ulBatNum;

            // Open a handle to the battery driver.
            pbs->hDevice = (HANDLE) -1;

            // Get the current battery info from the battery driver.
            if (SimUpdateBatInfoProc(pbs, NULL, 0, 0)) {

                // Link the new battery device state into the list.
                pbsTemp->bsNext = pbs;
                pbs->bsPrev = pbsTemp;
                return pbs;
            }
            LocalFree(lpsz);
        }
        LocalFree(pbs);
    }
    return NULL;
}


/*******************************************************************************
*
* RemoveBatteryStateDevice
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL RemoveBatteryStateDevice(PBATTERY_STATE pbs)
{
    // Unlink
    if (pbs->bsNext) {
        pbs->bsNext->bsPrev = pbs->bsPrev;
    }
    if (pbs->bsPrev) {
        pbs->bsPrev->bsNext = pbs->bsNext;
    }

#ifdef winnt
    UnregisterForDeviceNotification(pbs);
#endif
    
    // Free the battery driver handle if one was opened.
    if (pbs->hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(pbs->hDevice);
    }

    // Free the device name.
    LocalFree(pbs->lpszDeviceName);

    // Destroy any icons.
    if (pbs->hIconCache) {
        DestroyIcon(pbs->hIconCache);
    }
    if (pbs->hIconCache16) {
        DestroyIcon(pbs->hIconCache16);
    }

    // Free the associated storage.
    LocalFree(pbs);

    return TRUE;
}

/*******************************************************************************
*
*  RemoveMissingProc
*
*  DESCRIPTION:
*   Remove a battery from the global battery state list.
*
*  PARAMETERS:
*   lParam2 - REMOVE_MISSING or REMOVE_ALL
*
*******************************************************************************/

BOOL RemoveMissingProc(
    PBATTERY_STATE   pbs,
    HWND             hWnd,
    LPARAM           lParam1,
    LPARAM           lParam2)
{
    UINT    i;
    LPTSTR  *pszDeviceNames;

    if (lParam2 == REMOVE_MISSING) {
        if ((pszDeviceNames = (LPTSTR *)lParam1) != NULL) {
            for (i = 0; i < NUM_BAT; i++) {
                if (pszDeviceNames[i]) {
                    if (!lstrcmp(pbs->lpszDeviceName, pszDeviceNames[i])) {
                        // Device found in device list, leave it alone.
                        return TRUE;
                    }
                }
                else {
                    continue;
                }
            }
        }
    }

    // Device not in the device names list, remove it.
    RemoveBatteryStateDevice(pbs);
    return TRUE;
}

/*******************************************************************************
*
* FindNameProc
*
*  DESCRIPTION:
*   Returns FALSE (stop searching) if we find the name, else TRUE.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL FindNameProc(PBATTERY_STATE pbs, HWND hWnd, LPARAM lParam1, LPARAM lParam2)
{
    if (lParam1) {
        if (!lstrcmp(pbs->lpszDeviceName, (LPTSTR)lParam1)) {
            // Device found in device list.
            return FALSE;
        }
    }
    return TRUE;
}


