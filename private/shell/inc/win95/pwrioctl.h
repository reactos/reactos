/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       PWRIOCTL.H
*
*  VERSION:     1.0
*
*  DATE:        01 Feb 1994
*
*  AUTHOR:      TCS
*
*  Definitions for the Virtual Power Management Device DeviceIoControl
*  interface.
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  01 Feb 1994 TCS Original implementation.
*
*******************************************************************************/

#ifndef _INC_PWRIOCTL
#define _INC_PWRIOCTL

#ifndef Not_VxD
#define Not_VxD
#endif

#ifndef IS_32
#define IS_32
#endif

#include <vpowerd.h>

//
//  The IOCTL codes match up exactly to the order of the services in the VPOWERD
//  service table (plus one).  The input buffer for each IOCTL code follows the
//  same structure as the corresponding VPOWERD service would expect.  The
//  output buffer is used to store the return value from the VPOWERD service.
//

//  VPOWERD_IOCTL_GET_VERSION
//      Gets the version of the installed VPOWERD device.
//
//  lpvInBuffer:    Not used for this operation, set to NULL.
//  lpvOutBuffer:   Pointer to DWORD to receive the VPOWERD version.
#define VPOWERD_IOCTL_GET_VERSION               0x00000001

//  VPOWERD_IOCTL_GET_APM_BIOS_VERSION
//      Gets the version of the APM BIOS that VPOWERD has detected.
//
//  lpvInBuffer:    Not used for this operation, set to NULL.
//  lpvOutBuffer:   Pointer to DWORD to receive the APM BIOS version.
#define VPOWERD_IOCTL_GET_APM_BIOS_VERSION      0x00000002

//  VPOWERD_IOCTL_GET_PM_LEVEL
//      Gets the level of power management interaction between VPOWERD and the
//      APM BIOS.
//
//  lpvInBuffer:    Not used for this operation, set to NULL.
//  lpvOutBuffer:   Pointer to DWORD to receive the current power management
//                  level.
#define VPOWERD_IOCTL_GET_PM_LEVEL              0x00000003

//  VPOWERD_IOCTL_SET_PM_LEVEL
//      Sets the level of power management interaction between VPOWERD and the
//      APM BIOS.
//
//  lpvInBuffer:    Pointer to VPOWERD_SET_PM_LEVEL_PARAM structure.
//  lpvOutBuffer:   Pointer to DWORD to receive the VPOWERD return code.
#define VPOWERD_IOCTL_SET_PM_LEVEL              0x00000004

typedef struct _VPOWERD_SET_PM_LEVEL_PARAM {
    DWORD PowerManagementLevel;
}   VPOWERD_SET_PM_LEVEL_PARAM;

//  VPOWERD_IOCTL_SET_DEVICE_STATE
//      Sets the power state (e.g., OFF) of the specified device ID.  Not valid
//      for the system device (all devices power managed by the APM BIOS).
//
//  lpvInBuffer:    Pointer to VPOWERD_SET_DEVICE_PARAM structure.
//  lpvOutBuffer:   Pointer to DWORD to receive the VPOWERD return code.
#define VPOWERD_IOCTL_SET_DEVICE_STATE          0x00000005

typedef struct _VPOWERD_SET_DEVICE_PARAM {
    POWER_DEVICE_ID PowerDeviceID;
    POWER_STATE PowerState;
}   VPOWERD_SET_DEVICE_PARAM;

//  VPOWERD_IOCTL_RESTORE_DEFAULTS
//      Reinitializes all APM BIOS power-on defaults.
//
//  lpvInBuffer:    Not used for this operation, set to NULL.
//  lpvOutBuffer:   Pointer to DWORD to receive the VPOWERD return code.
#define VPOWERD_IOCTL_RESTORE_DEFAULTS          0x00000007

//  VPOWERD_IOCTL_GET_STATUS
//      Gets the current power status of the specified device ID.
//
//  lpvInBuffer:    Pointer to VPOWERD_GET_STATUS_PARAM structure.
//  lpvOutBuffer:   Pointer to DWORD to receive the VPOWERD return code.
#define VPOWERD_IOCTL_GET_STATUS                0x00000008

typedef struct _VPOWERD_GET_STATUS_PARAM {
    POWER_DEVICE_ID PowerDeviceID;
    LPPOWER_STATUS lpPowerStatus;
}   VPOWERD_GET_STATUS_PARAM;

//  VPOWERD_IOCTL_GET_STATE
//      Gets the power state (e.g., OFF) of the specified device ID.
//
//  lpvInBuffer:    Pointer to VPOWERD_GET_STATE_PARAM structure.
//  lpvOutBuffer:   Pointer to DWORD to receive the VPOWERD return code.
#define VPOWERD_IOCTL_GET_STATE                 0x00000009

typedef struct _VPOWERD_GET_STATE_PARAM {
    POWER_DEVICE_ID PowerDeviceID;
    LPPOWER_STATE lpPowerState;
}   GET_STATE_PARAM;

//  VPOWERD_IOCTL_OEM_APM_FUNCTION
//      Calls an OEM defined APM BIOS extension.
//
//  lpvInBuffer:    Pointer to VPOWERD_OEM_APM_FUNCTION_PARAM structure.
//  lpvOutBuffer:   Pointer to DWORD to receive the VPOWERD return code.
#define VPOWERD_IOCTL_OEM_APM_FUNCTION          0x0000000A

typedef struct _VPOWERD_OEM_APM_FUNCTION_PARAM {
    LPOEM_APM_REGS lpOemApmRegs;
}   VPOWERD_OEM_APM_FUNCTION_PARAM;

//  VPOWERD_IOCTL_W32_GET_SYSTEM_STATUS
//      Gets the current power status of the system.  Follows the Win32
//      GetSystemPowerStatus API convention.
//
//  lpvInBuffer:    Pointer to VPOWERD_W32_GET_SYSTEM_STATUS_PARAM structure.
//  lpvOutBuffer:   Pointer to DWORD to receive the boolean return code.
#define VPOWERD_IOCTL_W32_GET_SYSTEM_STATUS     0x0000000D

typedef struct _VPOWERD_W32_GET_SYSTEM_STATUS_PARAM {
    LPWIN32_SYSTEM_POWER_STATUS lpWin32SystemPowerStatus;
}   VPOWERD_W32_GET_SYSTEM_STATUS_PARAM;

//  VPOWERD_IOCTL_GET_CAPABILITIES
//		Gets the capabilities bitmask of an APM 1.2 machine.
//  lpvInBuffer:    Pointer to VPOWERD_GET_CAPABILITIES_PARAM structure.
//  lpvOutBuffer:   Pointer to DWORD to receive the boolean return code.
#define VPOWERD_IOCTL_GET_CAPABILITIES			0x000000F

typedef struct _VPOWERD_GET_CAPABILITIES_PARAM {
    PAPM_CAPABILITIES	pApmCaps;
}   VPOWERD_GET_CAPABILITIES_PARAM;

// VPOWERD_IOCTL_ENABLE_RING_RESUME
// 		Calls into the BIOS and attempts to enable the Resume on Ring capability.  This is only supported on APM 1.2 machines
//	lpvInBuffer: 	Not used for this operation, set to NULL
//  lpvOutBuffer: 	pointer to dword to receive the VPOWERD return code. (Expect PR_FUNC_NOT_SUPPORTED on APM 1.1 or lower machines.)
#define VPOWERD_IOCTL_ENABLE_RING_RESUME		0x00000010

// VPOWERD_IOCTL_DISABLE_RING_RESUME
// 		Calls into the BIOS and attempts to disable the Resume on Ring capability.  This is only supported on APM 1.2 machines
//	lpvInBuffer: 	Not used for this operation, set to NULL
//  lpvOutBuffer: 	pointer to dword to receive the VPOWERD return code. (Expect PR_FUNC_NOT_SUPPORTED on APM 1.1 or lower machines.)
#define VPOWERD_IOCTL_DISABLE_RING_RESUME		0x00000011

//  VPOWERD_IOCTL_W32_GET_STATUS
//      Gets the current power status of a particular battery on an APM 1.2 machine.
//		BatteryNumber is the battery device ID in which your interested.  
//		The device ID is formed by adding 0x8000 to the *1* based battery number.
//		e.g. if your interested in the first battery, BatteryNumber=0x8001
//  lpvInBuffer:    Pointer to VPOWERD_W32_GET_STATUS_PARAM structure.
//  lpvOutBuffer:   Pointer to DWORD to receive the boolean return code.
#define VPOWERD_IOCTL_W32_GET_STATUS     0x00000017

typedef struct _VPOWERD_W32_GET_STATUS_PARAM {
	DWORD	BatteryNumber;
    LPWIN32_SYSTEM_POWER_STATUS lpWin32SystemPowerStatus;
}   VPOWERD_W32_GET_STATUS_PARAM;

// VPOWERD_IOCTL_GET_RING_RESUME_STATUS
// 		Calls into the BIOS and attempts to determine whether Resume on Ring is enabled or disabled. 
//		This is only supported on APM 1.2 machines.  
//	lpvInBuffer: 	Pointer to VPOWERD_GET_RING_RESUME_STATUS_PARAM structure
//  lpvOutBuffer: 	pointer to dword to receive the VPOWERD return code.  (Expect PR_FUNC_NOT_SUPPORTED on APM 1.1 or lower machines.) 
#define VPOWERD_IOCTL_GET_RING_RESUME_STATUS	0x00000019

typedef struct	_VPOWERD_GET_RING_RESUME_STATUS_PARAM {
LPDWORD	pStatus;
} VPOWERD_GET_RING_RESUME_STATUS_PARAM;

#endif // _INC_PWRIOCTL
