/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Version.c

Abstract:

    This module implements a function to compare OS versions. Its the basis for
    VerifyVersionInfoW API. The Rtl version can be called from device drivers.

Author:

    Nar Ganapathy     [Narg]    19-Oct-1998

Environment:

    Pure utility routine

Revision History:

--*/

#include <stdio.h>
#include <ntrtlp.h>
#if !defined(NTOS_KERNEL_RUNTIME)
#include <winerror.h>
#endif

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE, RtlGetVersion)
#endif

//
// The following comment explains the old and the new style layouts for the
// condition masks. The condition mask is passed as a parameter to the
// VerifyVersionInfo API. The condition mask encodes conditions like VER_AND,
// VER_OR, VER_EQUAL for various types like VER_PLATFORMID, VER_MINORVERSION
// etc., When the API was originally designed the application used a macro
// called VER_SET_CONDTION which was defined to be  _m_=(_m_|(_c_<<(1<<_t_))).
// where _c_ is the condition and _t_ is the type. This macro is buggy for
// types >= VER_PLATFORMID. Unfortunately a lot of application code already
// uses this buggy macro (notably this terminal server) and have been shipped.
// To fix this bug, a new API VerSetConditionMask is defined which has a new
// bit layout. To provide backwards compatibility, we need to know if a
// specific condition mask is a new style mask (has the new bit layout) or is
// an old style mask. In both bit layouts bit 64 can never be set.
// So the new API sets this bit to indicate that the condition mask is a new
// style condition mask. So the code in this function that extracts the
// condition uses the new bit layout if bit 63 is set and the old layout if
// bit 63 is not set. This should allow applications that was compiled with
// the old macro to work.
//

//
// Use bit 63 to indicate that the new style bit layout is followed.
//
#define NEW_STYLE_BIT_MASK              0x8000000000000000


//
// Condition extractor for the old style mask.
//
#define OLD_CONDITION(_m_,_t_)  (ULONG)((_m_&(0xff<<(1<<_t_)))>>(1<<_t_))

//
// Test to see  if the mask is an old style mask.
//
#define OLD_STYLE_CONDITION_MASK(_m_)  (((_m_) & NEW_STYLE_BIT_MASK)  == 0)

#define RTL_GET_CONDITION(_m_, _t_) \
        (OLD_STYLE_CONDITION_MASK(_m_) ? (OLD_CONDITION(_m_,_t_)) : \
                RtlpVerGetConditionMask((_m_), (_t_)))

#define LEXICAL_COMPARISON        1     /* Do string comparison. Used for minor numbers */
#define MAX_STRING_LENGTH         20    /* Maximum number of digits for sprintf */

ULONG
RtlpVerGetConditionMask(
	ULONGLONG	ConditionMask,
	ULONG	TypeMask
	);


/*++

Routine Description:

    This function retrieves the OS version information. Its the kernel equivalent of
    the GetVersionExW win 32 API.

Arguments:

    lpVersionInformation - Supplies a pointer to the version info structure.
	In the kernel always assume that the structure is of type
	PRTL_OSVERSIONINFOEXW as its not exported to drivers. The signature
	is kept the same as for the user level RtlGetVersion.

Return Value:

    Always succeeds and returns STATUS_SUCCESS.
--*/
#if defined(NTOS_KERNEL_RUNTIME)
NTSTATUS
RtlGetVersion (
    OUT  PRTL_OSVERSIONINFOW lpVersionInformation
    )
{
	NT_PRODUCT_TYPE	NtProductType;
    RTL_PAGED_CODE();

    lpVersionInformation->dwMajorVersion = NtMajorVersion;
    lpVersionInformation->dwMinorVersion = NtMinorVersion;
    lpVersionInformation->dwBuildNumber = (USHORT)(NtBuildNumber & 0x3FFF);
    lpVersionInformation->dwPlatformId  = 2; // VER_PLATFORM_WIN32_NT from winbase.h
    if (lpVersionInformation->dwOSVersionInfoSize == sizeof( RTL_OSVERSIONINFOEXW )) {
        ((PRTL_OSVERSIONINFOEXW)lpVersionInformation)->wServicePackMajor = ((USHORT)CmNtCSDVersion >> 8) & (0xFF);
        ((PRTL_OSVERSIONINFOEXW)lpVersionInformation)->wServicePackMinor = (USHORT)CmNtCSDVersion & 0xFF;
        ((PRTL_OSVERSIONINFOEXW)lpVersionInformation)->wSuiteMask = (USHORT)(USER_SHARED_DATA->SuiteMask&0xffff);
        ((PRTL_OSVERSIONINFOEXW)lpVersionInformation)->wProductType = (RtlGetNtProductType(&NtProductType) ? NtProductType :0);

        /* Not set as its not needed by VerifyVersionInfoW */
        ((PRTL_OSVERSIONINFOEXW)lpVersionInformation)->wReserved = (UCHAR)0;
    }

    return STATUS_SUCCESS;
}
#else
NTSTATUS
RtlGetVersion(
    OUT  PRTL_OSVERSIONINFOW lpVersionInformation
    )
{
    PPEB Peb;
    NT_PRODUCT_TYPE NtProductType;

    Peb = NtCurrentPeb();
    lpVersionInformation->dwMajorVersion = Peb->OSMajorVersion;
    lpVersionInformation->dwMinorVersion = Peb->OSMinorVersion;
    lpVersionInformation->dwBuildNumber  = Peb->OSBuildNumber;
    lpVersionInformation->dwPlatformId   = Peb->OSPlatformId;
    if (lpVersionInformation->dwOSVersionInfoSize == sizeof( OSVERSIONINFOEXW ))
    {
        ((POSVERSIONINFOEXW)lpVersionInformation)->wServicePackMajor = (Peb->OSCSDVersion >> 8) & 0xFF;
        ((POSVERSIONINFOEXW)lpVersionInformation)->wServicePackMinor = Peb->OSCSDVersion & 0xFF;
        ((POSVERSIONINFOEXW)lpVersionInformation)->wSuiteMask = (USHORT)(USER_SHARED_DATA->SuiteMask&0xffff);
        ((POSVERSIONINFOEXW)lpVersionInformation)->wProductType = 0;
        if (RtlGetNtProductType( &NtProductType )) {
            ((POSVERSIONINFOEXW)lpVersionInformation)->wProductType = (UCHAR)NtProductType;
        }
    }

    return STATUS_SUCCESS;
}
#endif


BOOLEAN
RtlpVerCompare(
    LONG Condition,
    LONG Value1,
    LONG Value2,
    BOOLEAN *Equal,
    int   Flags
    )
{
    char    String1[MAX_STRING_LENGTH];
    char    String2[MAX_STRING_LENGTH];
    LONG    Comparison;

    if (Flags & LEXICAL_COMPARISON) {
        sprintf(String1, "%d", Value1); 
        sprintf(String2, "%d", Value2);
        Comparison = strcmp(String2, String1);
        Value1 = 0;
        Value2 = Comparison;
    }
    *Equal = (Value1 == Value2);
    switch (Condition) {
        case VER_EQUAL:
            return (Value2 == Value1);

        case VER_GREATER:
            return (Value2 > Value1);

        case VER_LESS:
            return (Value2 < Value1);

        case VER_GREATER_EQUAL:
            return (Value2 >= Value1);

        case VER_LESS_EQUAL:
            return (Value2 <= Value1);

        default:
            break;
    }

    return FALSE;
}



NTSTATUS
RtlVerifyVersionInfo(
    IN PRTL_OSVERSIONINFOEXW VersionInfo,
    IN ULONG TypeMask,
    IN ULONGLONG  ConditionMask
    )

/*+++
    This function verifies a version condition.  Basically, this
    function lets an app query the system to see if the app is
    running on a specific version combination.


Arguments:

    VersionInfo     - a version structure containing the comparison data
    TypeMask        - a mask comtaining the data types to look at
    ConditionMask   - a mask containing conditionals for doing the comparisons


Return Value:

    STATUS_INVALID_PARAMETER if the parameters are not valid.
    STATUS_REVISION_MISMATCH if the versions don't match.
    STATUS_SUCCESS if the versions match.

--*/

{
    ULONG i;
    OSVERSIONINFOEXW CurrVersion;
    BOOLEAN SuiteFound = FALSE;
    BOOLEAN Equal;
	NTSTATUS Status;
    ULONG   Condition;


    if (TypeMask == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory( &CurrVersion, sizeof(OSVERSIONINFOEXW) );
    CurrVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

    Status = RtlGetVersion((PRTL_OSVERSIONINFOW)&CurrVersion);
	if (Status != STATUS_SUCCESS)
			return Status;

    if (VersionInfo->wSuiteMask != 0) {
        for (i=0; i<16; i++) {
            if (VersionInfo->wSuiteMask&(1<<i)) {
                switch (RTL_GET_CONDITION(ConditionMask,VER_SUITENAME)) {
                    case VER_AND:
                        if (!(CurrVersion.wSuiteMask&(1<<i))) {
                            return STATUS_REVISION_MISMATCH;
                        }
                        break;

                    case VER_OR:
                        if (CurrVersion.wSuiteMask&(1<<i)) {
                            SuiteFound = TRUE;
                        }
                        break;

                    default:
                        return STATUS_INVALID_PARAMETER;
                }
            }
        }
        if ((RtlpVerGetConditionMask(ConditionMask,VER_SUITENAME) == VER_OR) && (SuiteFound == FALSE)) {
            return STATUS_REVISION_MISMATCH;
        }
    }

    Equal = TRUE;
    Condition = VER_EQUAL;
    if (TypeMask & VER_MAJORVERSION) {
        Condition = RTL_GET_CONDITION( ConditionMask, VER_MAJORVERSION);
        if (RtlpVerCompare(
                Condition,
                VersionInfo->dwMajorVersion,
                CurrVersion.dwMajorVersion,
                &Equal,
                0
                ) == FALSE)
        {
            if (!Equal) {
                return STATUS_REVISION_MISMATCH;
            }
        }
    }

    if (Equal) {
        ASSERT(Condition);
        if (TypeMask & VER_MINORVERSION) {
            if (Condition == VER_EQUAL) {
                Condition = RTL_GET_CONDITION(ConditionMask, VER_MINORVERSION); 
            }
            if (RtlpVerCompare(
                Condition,
                VersionInfo->dwMinorVersion,
                CurrVersion.dwMinorVersion,
                &Equal,
                LEXICAL_COMPARISON
                ) == FALSE)
            {
                if (!Equal) {
                    return STATUS_REVISION_MISMATCH;
                }
            }
        }

        if (Equal) {
            if (TypeMask & VER_SERVICEPACKMAJOR) {
                if (Condition == VER_EQUAL) {
                    Condition = RTL_GET_CONDITION(ConditionMask, VER_SERVICEPACKMAJOR); 
                }
                if (RtlpVerCompare(
                    Condition,
                    VersionInfo->wServicePackMajor,
                    CurrVersion.wServicePackMajor,
                    &Equal,
                    0
                    ) == FALSE)
                {
                    if (!Equal) {
                        return STATUS_REVISION_MISMATCH;
                    }
                }
            }
            if (Equal) {
                if (TypeMask & VER_SERVICEPACKMINOR) {
                    if (Condition == VER_EQUAL) {
                        Condition = RTL_GET_CONDITION(ConditionMask, VER_SERVICEPACKMINOR); 
                    }
                    if (RtlpVerCompare(
                        Condition,
                        (ULONG)VersionInfo->wServicePackMinor,
                        (ULONG)CurrVersion.wServicePackMinor,
                        &Equal,
                        LEXICAL_COMPARISON
                        ) == FALSE)
                    {
                        return STATUS_REVISION_MISMATCH;
                    }
                }
            }
        }
    }

    if ((TypeMask & VER_BUILDNUMBER) &&
        RtlpVerCompare(
            RTL_GET_CONDITION( ConditionMask, VER_BUILDNUMBER),
            VersionInfo->dwBuildNumber,
            CurrVersion.dwBuildNumber,
            &Equal,
            0
            ) == FALSE)
    {
        return STATUS_REVISION_MISMATCH;
    }

    if ((TypeMask & VER_PLATFORMID) &&
        RtlpVerCompare(
            RTL_GET_CONDITION( ConditionMask, VER_PLATFORMID),
            VersionInfo->dwPlatformId,
            CurrVersion.dwPlatformId,
            &Equal,
            0
            ) == FALSE)
    {
        return STATUS_REVISION_MISMATCH;
    }


    if ((TypeMask & VER_PRODUCT_TYPE) &&
        RtlpVerCompare(
            RTL_GET_CONDITION( ConditionMask, VER_PRODUCT_TYPE),
            VersionInfo->wProductType,
            CurrVersion.wProductType,
            &Equal,
            0
            ) == FALSE)
    {
        return STATUS_REVISION_MISMATCH;
    }

    return STATUS_SUCCESS;
}

ULONG
RtlpVerGetConditionMask(
	ULONGLONG	ConditionMask,
	ULONG	TypeMask
	)
{
	ULONG	NumBitsToShift;
	ULONG	Condition = 0;

	if (!TypeMask) {
		return 0;
	}

	for (NumBitsToShift = 0; TypeMask;  NumBitsToShift++) {
		TypeMask >>= 1;
    }

	Condition |=  (ConditionMask) >> ((NumBitsToShift - 1)
									* VER_NUM_BITS_PER_CONDITION_MASK);
	Condition &= VER_CONDITION_MASK;
	return Condition;
}


ULONGLONG
VerSetConditionMask(
	ULONGLONG	ConditionMask,
	ULONG	TypeMask,
	UCHAR	Condition
	)
{
	int	NumBitsToShift;

	Condition &= VER_CONDITION_MASK;

	if (!TypeMask) {
		return 0;
    }

	for (NumBitsToShift = 0; TypeMask;  NumBitsToShift++) {
		TypeMask >>= 1;
    }

    //
    // Mark that we are using a new style condition mask
    //
    ConditionMask |=  NEW_STYLE_BIT_MASK;
	ConditionMask |=  (Condition) << ((NumBitsToShift - 1)
    				* VER_NUM_BITS_PER_CONDITION_MASK);

	return ConditionMask;
}
