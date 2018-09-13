/*++ BUILD Version: 0001    // Increment this if a change has global effects


Copyright (c) 1991  Microsoft Corporation

Module Name:

    profiles.h

Abstract:

    This header file defines the Global definitions and interfaces for
    communicating the profile information between the loader, ntdetect and
    the kernel.

Author:

    Kenneth D. Ray (kenray) Dec 1997


Revision History:

--*/

#ifndef _PROFILES_H_
#define _PROFILES_H_

//
// Profile information stored in the registry, read from cmboot, and presented
// to the loader.
//


#define HW_PROFILE_STATUS_SUCCESS           0x0000
#define HW_PROFILE_STATUS_ALIAS_MATCH       0x0001
#define HW_PROFILE_STATUS_TRUE_MATCH        0x0002
#define HW_PROFILE_STATUS_PRISTINE_MATCH    0x0003
#define HW_PROFILE_STATUS_FAILURE           0xC001

//
// Docking States for the given profile
//
#define HW_PROFILE_DOCKSTATE_UNSUPPORTED       (0x0)
#define HW_PROFILE_DOCKSTATE_UNDOCKED          (0x1)
#define HW_PROFILE_DOCKSTATE_DOCKED            (0x2)
#define HW_PROFILE_DOCKSTATE_UNKNOWN           (0x3)
#define HW_PROFILE_DOCKSTATE_USER_SUPPLIED     (0x4)
#define HW_PROFILE_DOCKSTATE_USER_UNDOCKED     \
            (HW_PROFILE_DOCKSTATE_USER_SUPPLIED | HW_PROFILE_DOCKSTATE_UNDOCKED)
#define HW_PROFILE_DOCKSTATE_USER_DOCKED       \
            (HW_PROFILE_DOCKSTATE_USER_SUPPLIED | HW_PROFILE_DOCKSTATE_DOCKED)

//
// Capabilites of the given profile
//
#define HW_PROFILE_CAPS_VCR               0x0001 // As apposed to Surprize
#define HW_PROFILE_CAPS_DOCKING_WARM      0x0002
#define HW_PROFILE_CAPS_DOCKING_HOT       0x0004
#define HW_PROFILE_CAPS_RESERVED          0xFFF8

//
// Extension structure to the LOADER_PARAMETER_BLOCK in arc.h
//
typedef struct _PROFILE_PARAMETER_BLOCK {

    USHORT  Status;
    USHORT  Reserved;
    USHORT  DockingState;
    USHORT  Capabilities;
    ULONG   DockID;
    ULONG   SerialNumber;

} PROFILE_PARAMETER_BLOCK;

//
// Block to communcation the current ACPI docking state
//
typedef struct _PROFILE_ACPI_DOCKING_STATE {
    USHORT DockingState;
    USHORT SerialLength;
    WCHAR  SerialNumber[1];
} PROFILE_ACPI_DOCKING_STATE, *PPROFILE_ACPI_DOCKING_STATE;

//
// Desire verbose reporting/tracing of docking station related processing of
// hardware profiles in loader? This must be set to FALSE when compiling kernel
// to eliminate "unresolved external" errors from linker
//
// #define DOCKINFO_VERBOSE TRUE

#endif

