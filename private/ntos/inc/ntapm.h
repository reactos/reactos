/*++
;
; Copyright (c) 1998  Microsoft Corporation
;
; Module Name:
;
;   ntapm.h
;
; Abstract:
;
;   This module contains definitions specific to legacy APM support
;   in NT5, including special HAL interfaces
;
; Author:
;
;   Bryan Willman (bryanwi) 13 Feb 1998
;
; Revision History:
;
;
--*/


//
// Arguments to HalInitPowerManagment
//
#define HAL_APM_SIGNATURE   0x004D5041      // APM
#define HAL_APM_VERSION     500             // 5.00

#define HAL_APM_TABLE_SIZE  (sizeof(PM_DISPATCH_TABLE)+sizeof(PVOID))

#define HAL_APM_SLEEP_VECTOR    0
#define HAL_APM_OFF_VECTOR      1


//
// Values used in the Parameters.Other part of an IO_STACK_LOCATION
// to set up a link between a battery and ntapm.sys
//
typedef struct _NTAPM_LINK {
    ULONG   Signature;      // overlay Argument1
    ULONG   Version;        // overlay Argument2
    ULONG   BattLevelPtr;   // overlay Argument3, pointer to pointer to a pvoid void function
    ULONG   ChangeNotify;   // overlay Argument4, address of notify function
} NTAPM_LINK, *PNTAPM_LINK;

#define NTAPM_LINK_SIGNATURE    0x736d7061  // apms  = Argument1
#define NTAPM_LINK_VERSION      500         // 5.00  = Argument2

//
// Major code is IRP_MJ_INTERNAL_DEVICE_CONTROL
// Minor code is 0.
//

//
// BattLevelPtr gets the address of a routine with prototype:
//
//ULONG BatteryLevel();
//

//
// Data returned by NtApmGetBatteryLevel in NTAPM
// is a ULONG, cracked with these defines.
//
#define     NTAPM_ACON                  0x1000
#define     NTAPM_NO_BATT               0x2000
#define     NTAPM_NO_SYS_BATT           0x4000
#define     NTAPM_BATTERY_STATE         0x0f00
#define     NTAPM_BATTERY_STATE_SHIFT   8
#define     NTAPM_POWER_PERCENT         0x00ff



//
// ChangeNotify provides the address of a routine with prototype
//
//VOID ChangeNotify();
//





