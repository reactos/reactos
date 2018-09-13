/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    uptime.c

Abstract:

    Contains routines to calculate sysUpTime.

        SnmpSvcInitUptime
        SnmpSvcGetUptime

Environment:

    User Mode - Win32

Revision History:

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <windef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <snmp.h>
#include <snmputil.h>
#include <ntfuncs.h>


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global Variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

DWORD g_dwUpTimeReference = 0;
LONGLONG g_llUpTimeReference = 0;
extern PFNNTQUERYSYSTEMINFORMATION g_pfnNtQuerySystemInformation;
extern PFNRTLEXTENDEDLARGEINTEGERDIVIDE g_pfnRtlExtendedLargeIntegerDivide;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private Definitions                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define INVALID_UPTIME  0xFFFFFFFF


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private Procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

DWORD
SNMP_FUNC_TYPE 
SnmpSvcInitUptime(
    )

/*++

Routine Description:

    Initializes sysUpTime reference for SNMP process.

Arguments:

    None.

Return Values:

    Returns sysUpTime reference to pass to subagents. 

--*/

{
    // obtain reference the lame outdated way
    DWORD g_dwUpTimeReference = GetCurrentTime();

    // check to see if new method is available
    if (g_pfnNtQuerySystemInformation != NULL) {
        
        NTSTATUS NtStatus;
        SYSTEM_TIMEOFDAY_INFORMATION TimeOfDay;

        // query time in spiffy new precise manner        
        NtStatus = (*g_pfnNtQuerySystemInformation)(
                        SystemTimeOfDayInformation,
                        &TimeOfDay,
                        sizeof(SYSTEM_TIMEOFDAY_INFORMATION),
                        NULL
                        );
        
        // validate return code
        if (NT_SUCCESS(NtStatus)) {

            // initialize higher precision startup time         
            g_llUpTimeReference = TimeOfDay.CurrentTime.QuadPart;
        }
    }

    //
    // The algorithm for subagents to calculate sysUpTime
    // is based on GetCurrentTime() which returns the time 
    // in milliseconds and therefore wraps every 49.71 days.
    // RFC1213 specifies that sysUpTime is to be returned in
    // centaseconds but we cannot break existing subagents.
    // The old value is returned to the master agent here 
    // but newer subagents should use SnmpUtilGetUpTime.
    //

    return g_dwUpTimeReference;
} 


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public Procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

DWORD
SNMP_FUNC_TYPE
SnmpSvcGetUptime(
    )

/*++

Routine Description:

    Retrieves sysUpTime for SNMP process.

Arguments:

    None.

Return Values:

    Returns sysUpTime value for use by subagents. 

--*/

{
    DWORD dwUpTime = INVALID_UPTIME;

    // check to see if new method is available
    if ((g_pfnNtQuerySystemInformation != NULL) &&
        (g_pfnRtlExtendedLargeIntegerDivide != NULL) &&
        (g_llUpTimeReference != 0)) {
        
        NTSTATUS NtStatus;
        SYSTEM_TIMEOFDAY_INFORMATION TimeOfDay;

        // query time in spiffy new precise manner        
        NtStatus = (*g_pfnNtQuerySystemInformation)(
                        SystemTimeOfDayInformation,
                        &TimeOfDay,
                        sizeof(SYSTEM_TIMEOFDAY_INFORMATION),
                        NULL
                        );
        
        // validate return code
        if (NT_SUCCESS(NtStatus)) {

            LARGE_INTEGER liUpTime;
            LARGE_INTEGER liUpTimeInCentaseconds;

            // calculate difference from reference
            liUpTime.QuadPart = TimeOfDay.CurrentTime.QuadPart - 
                                                    g_llUpTimeReference;
                                    
            // convert 100ns units (10^-7) into centasecond units (10^-2)
            liUpTimeInCentaseconds = (*g_pfnRtlExtendedLargeIntegerDivide)(
                                            liUpTime,
                                            100000,
                                            NULL
                                            );

            // convert large integer to dword value
            dwUpTime = (DWORD)(LONGLONG)liUpTimeInCentaseconds.QuadPart;
        }
    
    } else if (g_dwUpTimeReference != 0) {

        // calculate difference from reference
        dwUpTime = (GetCurrentTime() - g_dwUpTimeReference) / 10;
    }

    return dwUpTime;
}
