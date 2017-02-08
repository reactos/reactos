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

static UCHAR
RtlpVerGetCondition(IN ULONGLONG ConditionMask,
                    IN ULONG TypeMask);

static BOOLEAN
RtlpVerCompare(ULONG left, ULONG right, UCHAR Condition)
{
    switch (Condition)
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
    NTSTATUS Status;
    RTL_OSVERSIONINFOEXW ver;
    BOOLEAN Comparison;

    /* FIXME:
        - Check the following special case on Windows (various versions):
          o lp->wSuiteMask == 0 and ver.wSuiteMask != 0 and VER_AND/VER_OR
          o lp->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXW)
        - MSDN talks about some tests being impossible. Check what really happens.
     */

    ver.dwOSVersionInfoSize = sizeof(ver);
    Status = RtlGetVersion((PRTL_OSVERSIONINFOW)&ver);
    if (Status != STATUS_SUCCESS) return Status;

    if (!TypeMask || !ConditionMask) return STATUS_INVALID_PARAMETER;

    if (TypeMask & VER_PRODUCT_TYPE)
    {
        Comparison = RtlpVerCompare(ver.wProductType,
                                    VersionInfo->wProductType,
                                    RtlpVerGetCondition(ConditionMask, VER_PRODUCT_TYPE));
        if (!Comparison)
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
        Comparison = RtlpVerCompare(ver.dwPlatformId,
                                    VersionInfo->dwPlatformId,
                                    RtlpVerGetCondition(ConditionMask, VER_PLATFORMID));
        if (!Comparison)
            return STATUS_REVISION_MISMATCH;
    }

    if (TypeMask & VER_BUILDNUMBER)
    {
        Comparison = RtlpVerCompare(ver.dwBuildNumber,
                                    VersionInfo->dwBuildNumber,
                                    RtlpVerGetCondition(ConditionMask, VER_BUILDNUMBER));
        if (!Comparison)
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
        UCHAR Condition = RtlpVerGetCondition(ConditionMask, TypeMask);

        Comparison = TRUE;
        if (TypeMask & VER_MAJORVERSION)
        {
            Comparison = RtlpVerCompare(ver.dwMajorVersion,
                                        VersionInfo->dwMajorVersion,
                                        Condition);
            do_next_check = (ver.dwMajorVersion == VersionInfo->dwMajorVersion) &&
                ((Condition != VER_EQUAL) || Comparison);
        }
        if ((TypeMask & VER_MINORVERSION) && do_next_check)
        {
            Comparison = RtlpVerCompare(ver.dwMinorVersion,
                                        VersionInfo->dwMinorVersion,
                                        Condition);
            do_next_check = (ver.dwMinorVersion == VersionInfo->dwMinorVersion) &&
                ((Condition != VER_EQUAL) || Comparison);
        }
        if ((TypeMask & VER_SERVICEPACKMAJOR) && do_next_check)
        {
            Comparison = RtlpVerCompare(ver.wServicePackMajor,
                                        VersionInfo->wServicePackMajor,
                                        Condition);
            do_next_check = (ver.wServicePackMajor == VersionInfo->wServicePackMajor) &&
                ((Condition != VER_EQUAL) || Comparison);
        }
        if ((TypeMask & VER_SERVICEPACKMINOR) && do_next_check)
        {
            Comparison = RtlpVerCompare(ver.wServicePackMinor,
                                        VersionInfo->wServicePackMinor,
                                        Condition);
        }

        if (!Comparison)
            return STATUS_REVISION_MISMATCH;
    }

    return STATUS_SUCCESS;
}

static UCHAR
RtlpVerGetCondition(IN ULONGLONG ConditionMask,
                    IN ULONG TypeMask)
{
    UCHAR Condition = 0;

    if (TypeMask & VER_PRODUCT_TYPE)
        Condition |= ConditionMask >> (7 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (TypeMask & VER_SUITENAME)
        Condition |= ConditionMask >> (6 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (TypeMask & VER_PLATFORMID)
        Condition |= ConditionMask >> (3 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (TypeMask & VER_BUILDNUMBER)
        Condition |= ConditionMask >> (2 * VER_NUM_BITS_PER_CONDITION_MASK);
    /*
     * We choose here the lexicographical order on the 4D space
     * {(Major ; Minor ; SP Major ; SP Minor)} to select the
     * appropriate comparison operator.
     * Therefore the following 'else if' instructions must be in this order.
     */
    else if (TypeMask & VER_MAJORVERSION)
        Condition |= ConditionMask >> (1 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (TypeMask & VER_MINORVERSION)
        Condition |= ConditionMask >> (0 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (TypeMask & VER_SERVICEPACKMAJOR)
        Condition |= ConditionMask >> (5 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (TypeMask & VER_SERVICEPACKMINOR)
        Condition |= ConditionMask >> (4 * VER_NUM_BITS_PER_CONDITION_MASK);

    Condition &= VER_CONDITION_MASK;

    return Condition;
}

/*
 * @implemented
 */
ULONGLONG
NTAPI
VerSetConditionMask(IN ULONGLONG ConditionMask,
                    IN ULONG TypeMask,
                    IN UCHAR Condition)
{
    ULONGLONG ullCondMask;

    if (TypeMask == 0)
        return ConditionMask;

    Condition &= VER_CONDITION_MASK;

    if (Condition == 0)
        return ConditionMask;

    ullCondMask = Condition;
    if (TypeMask & VER_PRODUCT_TYPE)
        ConditionMask |= ullCondMask << (7 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (TypeMask & VER_SUITENAME)
        ConditionMask |= ullCondMask << (6 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (TypeMask & VER_SERVICEPACKMAJOR)
        ConditionMask |= ullCondMask << (5 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (TypeMask & VER_SERVICEPACKMINOR)
        ConditionMask |= ullCondMask << (4 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (TypeMask & VER_PLATFORMID)
        ConditionMask |= ullCondMask << (3 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (TypeMask & VER_BUILDNUMBER)
        ConditionMask |= ullCondMask << (2 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (TypeMask & VER_MAJORVERSION)
        ConditionMask |= ullCondMask << (1 * VER_NUM_BITS_PER_CONDITION_MASK);
    else if (TypeMask & VER_MINORVERSION)
        ConditionMask |= ullCondMask << (0 * VER_NUM_BITS_PER_CONDITION_MASK);

    return ConditionMask;
}

/* EOF */
