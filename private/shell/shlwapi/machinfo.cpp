//
// machinfo.cpp - SHGetMachineInfo and related functions
//
//

#include "priv.h"
#include <dbt.h>
#include <cfgmgr32.h>
#include <apithk.h>

#ifndef UNIX
#include <batclass.h>

const GUID GUID_DEVICE_BATTERY = { 0x72631e54L, 0x78A4, 0x11d0,
              { 0xbc, 0xf7, 0x00, 0xaa, 0x00, 0xb7, 0xb3, 0x2a } };

#include <hydra\winsta.h>
#endif

//
//  Win95 does not decorate BroadcastSystemMessage, so we can't either.
//
#undef BroadcastSystemMessage

extern "C" {
WINUSERAPI long WINAPI
BroadcastSystemMessage(DWORD, LPDWORD, UINT, WPARAM, LPARAM);
};

/*****************************************************************************
 *
 *  DOCK STATE - Win95, Win98, and WinNT all do this differently (yuck)
 *
 *****************************************************************************/

C_ASSERT(GMID_NOTDOCKABLE == CM_HWPI_NOT_DOCKABLE);
C_ASSERT(GMID_UNDOCKED    == CM_HWPI_UNDOCKED);
C_ASSERT(GMID_DOCKED      == CM_HWPI_DOCKED);

#if defined(_X86_) && !defined(UNIX)

typedef struct CMHDR {
    LPVOID  pArgs;
    DWORD   dwService;
    DWORD   dwRet;
} CMHDR, *PCMHDR;

#define CONFIGMG_Get_Hardware_Profile_Info      0x00330052

//
//  GetDockedState95
//
DWORD GetDockedState95()
{
    struct GHWPI95 {                // Get_Hardware_Profile_Info parameter blk
        CMHDR   cmhdr;
        ULONG   ulIndex;
        PHWPROFILEINFO_A pHWProfileInfo;
        ULONG   ulFlags;
        HWPROFILEINFO_A HWProfileInfo;
    } *pghwpi;

    HANDLE hheap;
    DWORD Result = GMID_NOTDOCKABLE;    // assume the worst

#define HEAP_SHARED     0x04000000      /* put heap in shared memory--undoc'd */

    //
    //  Win95 Configmg requires the parameter block to reside in the shared
    //  heap since we're going to do a cross-process SendMessage.
    //

    hheap = HeapCreate(HEAP_SHARED, 1, 4096);
    if (hheap) {
        // Allocate parameter block in shared memory
        pghwpi = (struct GHWPI95 *)HeapAlloc(hheap, HEAP_ZERO_MEMORY,
                                             sizeof(*pghwpi));
        if (pghwpi) {
            DWORD dwRecipients = BSM_VXDS;

            pghwpi->cmhdr.dwRet     = 0;
            pghwpi->cmhdr.dwService = CONFIGMG_Get_Hardware_Profile_Info;
            pghwpi->cmhdr.pArgs     = &pghwpi->ulIndex;
            pghwpi->ulIndex         = 0xFFFFFFFF;
            pghwpi->pHWProfileInfo  = &pghwpi->HWProfileInfo;
            pghwpi->ulFlags         = 0;

            // "Call" the service

            BroadcastSystemMessage(0, &dwRecipients, WM_DEVICECHANGE,
                                   DBT_CONFIGMGAPI32, (LPARAM)pghwpi);

            if (pghwpi->cmhdr.dwRet == CR_SUCCESS) {

                Result = pghwpi->HWProfileInfo.HWPI_dwFlags;
            } else {
                TraceMsg(DM_WARNING, "GetDockedState95: CONFIGMG did not respond");
            }
        }

        HeapDestroy(hheap);
    } else {
        TraceMsg(DM_WARNING, "GetDockedState95: Unable to create shared heap");
    }
    return Result;
}


//
//  On Win98, use the 32-bit interface to configmg.
//

CONFIGRET __cdecl
CallConfigmg98(DWORD dwServiceNumber, ...)
{
    CONFIGRET cr;
    HANDLE hCM;

    hCM = CreateFileA("\\\\.\\CONFIGMG",
                      GENERIC_READ|GENERIC_WRITE,
                      FILE_SHARE_READ|FILE_SHARE_WRITE,
                      NULL, OPEN_EXISTING, 0, NULL);
    if (hCM != INVALID_HANDLE_VALUE) {
        DWORD dwRet;

        // Evil hack that works only on x86.  Fortunately, this code is
        // inside an #ifdef _X86_ block, so we're safe.
        LPVOID pvArg = 1 + &dwServiceNumber;

        if (DeviceIoControl(hCM, dwServiceNumber, &pvArg, sizeof(pvArg),
                            &cr, sizeof(cr), &dwRet, 0) &&
                            dwRet == sizeof(cr)) {
        } else {
            TraceMsg(DM_WARNING, "CallConfigmg98: CONFIGMG did not respond");
            cr = CR_FAILURE;
        }

        CloseHandle(hCM);
    } else {
        TraceMsg(DM_WARNING, "CallConfigmg98: Couldn't connect to CONFIGMG");
        cr = CR_FAILURE;
    }

    return cr;
}

DWORD GetDockedState98()
{
    CONFIGRET cr;
    DWORD Result = GMID_NOTDOCKABLE;    // assume the worst
    HWPROFILEINFO_A HWProfileInfo;

    cr = CallConfigmg98(
            0x80000000 + LOWORD(CONFIGMG_Get_Hardware_Profile_Info),
            -1,                     // ulIndex, -1 means "current profile"
            &HWProfileInfo,         // PHWPROFILEINFO
            0);                     // ulFlags

    if (cr == CR_SUCCESS) {
        Result = HWProfileInfo.HWPI_dwFlags;
    }

    return Result;
}

#endif

typedef BOOL (WINAPI *GETCURRENTHWPROFILEA)(LPHW_PROFILE_INFOA);
DWORD GetDockedStateNT()
{
    HW_PROFILE_INFOA hpi;
    GETCURRENTHWPROFILEA GetCurrentHwProfileA;
    DWORD Result = GMID_NOTDOCKABLE;    // assume the worst

    GetCurrentHwProfileA = (GETCURRENTHWPROFILEA)
                            GetProcAddress(GetModuleHandle("ADVAPI32"),
                            "GetCurrentHwProfileA");

    if (GetCurrentHwProfileA && GetCurrentHwProfileA(&hpi)) {
        Result = hpi.dwDockInfo & (DOCKINFO_UNDOCKED | DOCKINFO_DOCKED);

        // Wackiness: If the machine does not support docking, then
        // NT returns >both< flags set.  Go figure.
        if (Result == (DOCKINFO_UNDOCKED | DOCKINFO_DOCKED)) {
            Result = GMID_NOTDOCKABLE;
        }
    } else {
        TraceMsg(DM_WARNING, "GetDockedStateNT: GetCurrentHwProfile failed");
    }
    return Result;
}

#if defined(_X86_) && !defined(UNIX)

//
//  Platforms that support Win95/Win98 need to do version switching
//
DWORD GetDockedState()
{
    if (g_bRunningOnNT) {
        return GetDockedStateNT();
    } else if (g_bRunningOnMemphis) {
        return GetDockedState98();
    } else {
        return GetDockedState95();
    }
}

#else

//
//  Platforms that do not support Win95/Win98 can just call the NT version.
//
#define GetDockedState()            GetDockedStateNT()

#endif


#ifndef UNIX

/*****************************************************************************
 *
 *  BATTERY STATE - Once again, Win95 and Win98 and NT all do it differently
 *
 *****************************************************************************/

//
//  Values for SYSTEM_POWER_STATUS.ACLineStatus
//
#define SPSAC_OFFLINE       0
#define SPSAC_ONLINE        1

//
//  Values for SYSTEM_POWER_STATUS.BatteryFlag
//
#define SPSBF_NOBATTERY     128

//
//  So many ways to detect batteries, so little time...
//
DWORD GetBatteryState()
{
    //
    //  Since GMIB_HASBATTERY is cumulative (any battery turns it on)
    //  and GMIB_ONBATTERY is subtractive (any AC turns it off), the
    //  state you have to start in before you find a battery is
    //  GMIB_HASBATTERY off and GMIB_ONBATTERY on.
    //
    //  dwResult & GMIB_ONBATTERY means we have yet to find AC power.
    //  dwResult & GMIB_HASBATTERY means we have found a non-UPS battery.
    //
    DWORD dwResult = GMIB_ONBATTERY;

    //------------------------------------------------------------------
    //
    //  First try - IOCTL_BATTERY_QUERY_INFORMATION
    //
    //------------------------------------------------------------------
    //
    //  Windows 98 and Windows 2000 support IOCTL_BATTERY_QUERY_INFORMATION,
    //  which lets us enumerate the batteries and ask each one for information.
    //  Except that on Windows 98, we can enumerate only ACPI batteries.
    //  We still have to use VPOWERD to enumerate APM batteries.
    //  BUGBUG -- deal with Win98 APM batteries

    HDEVINFO hdev = SetupDiGetClassDevs(&GUID_DEVICE_BATTERY, 0, 0,
                        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (hdev != INVALID_HANDLE_VALUE) {
        SP_DEVICE_INTERFACE_DATA did;
        did.cbSize = sizeof(did);
        // Stop at 100 batteries so we don't go haywire
        for (int idev = 0; idev < 100; idev++) {
            // Pre-set the error code because our DLLLOAD wrapper doesn't
            // and Windows NT 4 supports SetupDiGetClassDevs but not
            // SetupDiEnumDeviceInterfaces (go figure).
            SetLastError(ERROR_NO_MORE_ITEMS);
            if (SetupDiEnumDeviceInterfaces(hdev, 0, &GUID_DEVICE_BATTERY, idev, &did)) {
                DWORD cbRequired = 0;

                /*
                 *  Ask for the required size then allocate it then fill it.
                 *
                 *  Sigh.  Windows NT and Windows 98 implement
                 *  SetupDiGetDeviceInterfaceDetail differently if you are
                 *  querying for the buffer size.
                 *
                 *  Windows 98 returns FALSE, and GetLastError() returns
                 *  ERROR_INSUFFICIENT_BUFFER.
                 *
                 *  Windows NT returns TRUE.
                 *
                 *  So we allow the cases either where the call succeeds or
                 *  the call fails with ERROR_INSUFFICIENT_BUFFER.
                 */

                if (SetupDiGetDeviceInterfaceDetail(hdev, &did, 0, 0, &cbRequired, 0) ||
                    GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                    PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd;
                    pdidd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LPTR, cbRequired);
                    if (pdidd) {
                        pdidd->cbSize = sizeof(*pdidd);
                        if (SetupDiGetDeviceInterfaceDetail(hdev, &did, pdidd, cbRequired, &cbRequired, 0)) {
                            /*
                             *  Finally enumerated a battery.  Ask it for information.
                             */
                            HANDLE hBattery = CreateFile(pdidd->DevicePath,
                                                         GENERIC_READ | GENERIC_WRITE,
                                                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                         NULL, OPEN_EXISTING,
                                                         FILE_ATTRIBUTE_NORMAL, NULL);
                            if (hBattery != INVALID_HANDLE_VALUE) {
                                /*
                                 *  Now you have to ask the battery for its tag.
                                 */
                                BATTERY_QUERY_INFORMATION bqi;

                                DWORD dwWait = 0;
                                DWORD dwOut;
                                bqi.BatteryTag = 0;

                                if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_TAG,
                                                    &dwWait, sizeof(dwWait),
                                                    &bqi.BatteryTag, sizeof(bqi.BatteryTag),
                                                    &dwOut, NULL) && bqi.BatteryTag) {
                                    /*
                                     *  With the tag, you can query the battery info.
                                     */
                                    BATTERY_INFORMATION bi;
                                    bqi.InformationLevel = BatteryInformation;
                                    bqi.AtRate = 0;
                                    if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_INFORMATION,
                                                        &bqi, sizeof(bqi),
                                                        &bi,  sizeof(bi),
                                                        &dwOut, NULL)) {
                                        // Only system batteries count
                                        if (bi.Capabilities & BATTERY_SYSTEM_BATTERY)  {
                                            if (!(bi.Capabilities & BATTERY_IS_SHORT_TERM)) {
                                                dwResult |= GMIB_HASBATTERY;
                                            }

                                            /*
                                             *  And then query the battery status.
                                             */
                                            BATTERY_WAIT_STATUS bws;
                                            BATTERY_STATUS bs;
                                            ZeroMemory(&bws, sizeof(bws));
                                            bws.BatteryTag = bqi.BatteryTag;
                                            if (DeviceIoControl(hBattery, IOCTL_BATTERY_QUERY_STATUS,
                                                                &bws, sizeof(bws),
                                                                &bs,  sizeof(bs),
                                                                &dwOut, NULL)) {
                                                if (bs.PowerState & BATTERY_POWER_ON_LINE) {
                                                    dwResult &= ~GMIB_ONBATTERY;
                                                }
                                            }
                                        }
                                    }
                                }
                                CloseHandle(hBattery);
                            }
                        }
                        LocalFree(pdidd);
                    }
                }
            } else {
                // Enumeration failed - perhaps we're out of items
                if (GetLastError() == ERROR_NO_MORE_ITEMS)
                    break;
            }
        }
        SetupDiDestroyDeviceInfoList(hdev);

    }

    //
    //  On Windows NT, SetupDi tells us everything there is to know.
    //  So once you get this far, you're finished.
    //
    if (g_bRunningOnNT) {
        goto finish;
    }

    //------------------------------------------------------------------
    //
    //  Second try - GetSystemPowerStatus
    //
    //------------------------------------------------------------------
    //
    //  On Windows 9x, GetSystemPowerStatus enumerates a disjoint set of
    //  batteries from SetupDi, so it's worth calling to find out.
    //

    SYSTEM_POWER_STATUS status;

    if (GetSystemPowerStatus(&status)) {
        //
        //  HACKHACK:   Some APM BIOS implementations set BatteryFlag = 0
        //              instead of 128 or 255 when they don't have a
        //              battery, so we have to check both.
        //
        if (status.BatteryFlag != 0 &&
            !(status.BatteryFlag & SPSBF_NOBATTERY)) {
            //
            // Found an APM battery.
            //
            dwResult |= GMIB_HASBATTERY;
        }

        if (status.ACLineStatus == SPSAC_ONLINE) {
            dwResult &= ~GMIB_ONBATTERY;
        }
    }

#ifdef TRY_NtPowerInformation // Hopefully the Third Try won't be necessary
    SYSTEM_POWER_CAPABILITIES caps;

    //------------------------------------------------------------------
    //
    //  Third try - NtPowerInformation
    //
    //------------------------------------------------------------------
    //
    //  NtPowerInformation is supported on Win98 and Windows 2000, but not
    //  Windows 95 or NT4.
    //
    if (SUCCEEDED(NtPowerInformation(SystemPowerCapabilities, NULL, 0,
                                     &caps, sizeof(caps)))) {

        if (caps.BatteriesAreShortTerm) {
            #error futz futz
        }

        if (caps.SystemBatteriesPresent && !fFoundUPS) {
            #error futz futz
        }

    }

#endif // TRY_NtPowerInformation

finish:
    //
    //  Final cleanup:  If we didn't find a battery, then presume that we
    //  are on AC power.
    //
    if (!(dwResult & GMIB_HASBATTERY))
        dwResult &= ~GMIB_ONBATTERY;

    return dwResult;
}

/*****************************************************************************
 *
 *  TERMINAL SERVER CLIENT
 *
 *  This is particularly gruesome because Terminal Server for NT4 SP3 goes
 *  to extraordinary lengths to prevent you from detecting it.  Even the
 *  semi-documented NtCurrentPeb()->SessionId trick doesn't work on NT4 SP3.
 *  So we have to go to the totally undocumented winsta.dll to find out.
 *
 *****************************************************************************/

BOOL g_fTSClient = -1;  // Tri-state, 0 = no, 1 = yes, -1 = don't know

BOOL IsTSClientNT4(void)
{
    BOOL fTS = FALSE;       // Assume not

    HINSTANCE hinstWinSta = LoadLibrary("winsta.dll");
    if (hinstWinSta) {
        PWINSTATIONQUERYINFORMATIONW WinStationQueryInformationW;
        WINSTATIONINFORMATIONW wi;
        WinStationQueryInformationW = (PWINSTATIONQUERYINFORMATIONW)
                    GetProcAddress(hinstWinSta, "WinStationQueryInformationW");
        if (WinStationQueryInformationW &&
            WinStationQueryInformationW(SERVERNAME_CURRENT, LOGONID_CURRENT, WinStationInformation, &wi, sizeof(wi), NULL) &&
            wi.LogonId != 0) {
            fTS = TRUE;
        }
        FreeLibrary(hinstWinSta);
    }

    return fTS;
}

BOOL IsTSClient(void)
{
    if (!g_bRunningOnNT) {
        // Windows 9x doesn't support Terminal Server
        return FALSE;
    } else if (g_bRunningOnNT5OrHigher) {
        // NT5 has a new system metric to detect this
        return GetSystemMetrics(SM_REMOTESESSION);
    } else {
        // NT4 is gross and evil.  This is slow, so cache the result.
        if (g_fTSClient < 0)
            g_fTSClient = IsTSClientNT4();
        return g_fTSClient;
    }
}

/*****************************************************************************
 *
 *  SHGetMachineInfo
 *
 *****************************************************************************/

//
//  SHGetMachineInfo
//
//  Given an index, returns some info about that index.  See shlwapi.w
//  for documentation on the flags available.
//
STDAPI_(DWORD_PTR) SHGetMachineInfo(UINT gmi)
{
    switch (gmi) {
    case GMI_DOCKSTATE:
        return GetDockedState();

    case GMI_BATTERYSTATE:
        return GetBatteryState();

    //
    //  It smell like a laptop if it has a battery or if it can be docked.
    //
    case GMI_LAPTOP:
        return (GetBatteryState() & GMIB_HASBATTERY) ||
               (GetDockedState() != GMID_NOTDOCKABLE);

    case GMI_TSCLIENT:
        return IsTSClient();
    }

    TraceMsg(DM_WARNING, "SHGetMachineInfo: Unknown info query %d", gmi);
    return 0;
}

#else

STDAPI_(DWORD_PTR) SHGetMachineInfo(UINT gmi)
{
    // IEUNIX : Stubbed out this api to resolve undefind symbol in linking.
    TraceMsg(DM_WARNING, "SHGetMachineInfo: Unknown info query %d", gmi);
    return 0;
}

#endif
