/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * PURPOSE:         Runtime code
 * FILE:            lib/rtl/version.c
 * PROGRAMERS:      Filip Navara
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

NTSTATUS
NTAPI
RtlGetVersion(OUT PRTL_OSVERSIONINFOW lpVersionInformation);

/* FUNCTIONS ****************************************************************/

static BYTE
RtlpVerGetCondition(IN ULONGLONG dwlConditionMask,
                    IN DWORD dwTypeBitMask);

static BOOLEAN
RtlpVerCompare(ULONG left, ULONG right, UCHAR condition)
{
    switch (condition)
    {
        case VER_EQUAL:
            return (left == right);
        case VER_GREATER:
            return (left > right);
        case VER_GREATER_EQUAL:
            return (left >= right);
        case VER_LESS:
            return (left < right);
        case VER_LESS_EQUAL:
            return (left <= right);
        default:
            break;
    }
    return FALSE;
}

/*
* @implemented
*/
NTSTATUS
NTAPI
RtlVerifyVersionInfo(IN PRTL_OSVERSIONINFOEXW VersionInfo,
                     IN ULONG TypeMask,
                     IN ULONGLONG ConditionMask)
{
    RTL_OSVERSIONINFOEXW ver;
    NTSTATUS status;
    BOOLEAN comparison;

    /* FIXME:
        - Check the following special case on Windows (various versions):
          o lp->wSuiteMask == 0 and ver.wSuiteMask != 0 and VER_AND/VER_OR
          o lp->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXW)
        - MSDN talks about some tests being impossible. Check what really happens.
     */

    ver.dwOSVersionInfoSize = sizeof(ver);
    status = RtlGetVersion((PRTL_OSVERSIONINFOW)&ver);
    if (status != STATUS_SUCCESS) return status;

    if (!TypeMask || !ConditionMask) return STATUS_INVALID_PARAMETER;

    if (TypeMask & VER_PRODUCT_TYPE)
    {
        comparison = RtlpVerCompare(ver.wProductType,
                                    VersionInfo->wProductType,
                                    RtlpVerGetCondition(ConditionMask, VER_PRODUCT_TYPE));
        if (!comparison)
            return STATUS_REVISION_MISMATCH;
    }

    if (TypeMask & VER_SUITENAME)
    {
        switch (RtlpVerGetCondition(ConditionMask, VER_SUITENAME))
        {
            case VER_AND:
                if ((VersionInfo->wSuiteMask & ver.wSuiteMask) != VersionInfo->wSuiteMask)
                {
                    return STATUS_REVISION_MISMATCH;
                }
                break;
            case VER_OR:
                if (!(VersionInfo->wSuiteMask & ver.wSuiteMask) && VersionInfo->wSuiteMask)
                {
                    return STATUS_REVISION_MISMATCH;
                }
                break;
            default:
                return STATUS_INVALID_PARAMETER;
        }
    }

    if (TypeMask & VER_PLATFORMID)
    {
        comparison = RtlpVerCompare(ver.dwPlatformId,
                                    VersionInfo->dwPlatformId,
                                    RtlpVerGetCondition(ConditionMask, VER_PLATFORMID));
        if (!comparison)
            return STATUS_REVISION_MISMATCH;
    }

    if (TypeMask & VER_BUILDNUMBER)
    {
        comparison = RtlpVerCompare(ver.dwBuildNumber,
                                    VersionInfo->dwBuildNumber,
                                    RtlpVerGetCondition(ConditionMask, VER_BUILDNUMBER));
        if (!comparison)
            return STATUS_REVISION_MISMATCH;
    }

    TypeMask &= VER_MAJORVERSION|VER_MINORVERSION|VER_SERVICEPACKMAJOR|VER_SERVICEPACKMINOR;
    if (TypeMask)
    {
        BOOLEAN do_next_check = TRUE;
        /*
         * Select the leading comparison operator (for example, the comparison
         * operator for VER_MAJORVERSION supersedes the others for VER_MINORVERSION,
         * VER_SERVICEPACKMAJOR and VER_SERVICEPACKMINOR).
         */
        BYTE condition = RtlpVerGetCondition(ConditionMask, TypeMask);

        comparison = TRUE;
        if (TypeMask & VER_MAJORVERSION)
        {
            comparison = RtlpVerCompare(ver.dwMajorVersion,
                                        VersionInfo->dwMajorVersion,
                                        condition);
            do_next_check = (ver.dwMajorVersion == VersionInfo->dwMajorVersion) &&
                ((condition != VER_EQUAL) || comparison);
        }
        if ((TypeMask & VER_MINORVERSION) && do_next_check)
        {
            comparison = RtlpVerCompare(ver.dwMinorVersion,
                                        VersionInfo->dwMinorVersion,
                                        condition);
            do_next_check = (ver.dwMinorVersion == VersionInfo->dwMinorVersion) &&
                ((condition != VER_EQUAL) || comparison);
        }
        if ((TypeMask & VER_SERVICEPACKMAJOR) && do_next_check)
        {
            comparison = RtlpVerCompare(ver.wServicePackMajor,
                                        VersionInfo->wServicePackMajor,
                                        condition);
            do_next_check = (ver.wServicePackMajor == VersionInfo->wServicePackMajor) &&
                ((condition != VER_EQUAL) || comparison);
        }
        if ((TypeMask & VER_SERVICEPACKMINOR) && do_next_check)
        {
            comparison = RtlpVerCompare(ver.wServicePackMinor,
                                        VersionInfo->wServicePackMinor,
                                        condition);
        }

        if (!comparison)
            return STATUS_REVISION_MISMATCH;
    }

    return STATUS_SUCCESS;
}

static BYTE
RtlpVerGetCondition(IN ULONGLONG dwlConditionMask,
                    IN DWORD dwTypeBitMask)
{
    BYTE bConditionMask = 0;

    if (dwTypeBitMask & VER_PRODUCT_TYPE)
        bConditionMask |= dwlConditionMask >> (7 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (dwTypeBitMask & VER_SUITENAME)
        bConditionMask |= dwlConditionMask >> (6 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (dwTypeBitMask & VER_PLATFORMID)
        bConditionMask |= dwlConditionMask >> (3 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (dwTypeBitMask & VER_BUILDNUMBER)
        bConditionMask |= dwlConditionMask >> (2 * VER_NUM_BITS_PER_CONDITION_MASK);
    /*
     * We choose here the lexicographical order on the 4D space
     * {(Major ; Minor ; SP Major ; SP Minor)} to select the
     * appropriate comparison operator.
     * Therefore the following 'else if' instructions must be in this order.
     */
    else if (dwTypeBitMask & VER_MAJORVERSION)
        bConditionMask |= dwlConditionMask >> (1 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (dwTypeBitMask & VER_MINORVERSION)
        bConditionMask |= dwlConditionMask >> (0 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (dwTypeBitMask & VER_SERVICEPACKMAJOR)
        bConditionMask |= dwlConditionMask >> (5 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (dwTypeBitMask & VER_SERVICEPACKMINOR)
        bConditionMask |= dwlConditionMask >> (4 * VER_NUM_BITS_PER_CONDITION_MASK);

    bConditionMask &= VER_CONDITION_MASK;

    return bConditionMask;
}

/*
 * @implemented
 */
ULONGLONG
NTAPI
VerSetConditionMask(IN ULONGLONG dwlConditionMask,
                    IN DWORD dwTypeBitMask,
                    IN BYTE bConditionMask)
{
    ULONGLONG ullCondMask;

    if (dwTypeBitMask == 0)
        return dwlConditionMask;

    bConditionMask &= VER_CONDITION_MASK;

    if (bConditionMask == 0)
        return dwlConditionMask;

    ullCondMask = bConditionMask;
    if (dwTypeBitMask & VER_PRODUCT_TYPE)
        dwlConditionMask |= ullCondMask << (7 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (dwTypeBitMask & VER_SUITENAME)
        dwlConditionMask |= ullCondMask << (6 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (dwTypeBitMask & VER_SERVICEPACKMAJOR)
        dwlConditionMask |= ullCondMask << (5 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (dwTypeBitMask & VER_SERVICEPACKMINOR)
        dwlConditionMask |= ullCondMask << (4 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (dwTypeBitMask & VER_PLATFORMID)
        dwlConditionMask |= ullCondMask << (3 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (dwTypeBitMask & VER_BUILDNUMBER)
        dwlConditionMask |= ullCondMask << (2 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (dwTypeBitMask & VER_MAJORVERSION)
        dwlConditionMask |= ullCondMask << (1 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (dwTypeBitMask & VER_MINORVERSION)
        dwlConditionMask |= ullCondMask << (0 * VER_NUM_BITS_PER_CONDITION_MASK);

    return dwlConditionMask;
}

/* EOF */
