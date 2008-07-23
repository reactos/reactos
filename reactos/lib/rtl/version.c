/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * PURPOSE:         Runtime code
 * FILE:            lib/rtl/version.c
 * PROGRAMER:       Filip Navara
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

NTSTATUS
NTAPI
RtlGetVersion(
    OUT PRTL_OSVERSIONINFOW lpVersionInformation
    );

/* FUNCTIONS ****************************************************************/

static inline NTSTATUS version_compare_values(ULONG left, ULONG right, UCHAR condition)
{
    switch (condition) {
        case VER_EQUAL:
            if (left != right) return STATUS_REVISION_MISMATCH;
            break;
        case VER_GREATER:
            if (left <= right) return STATUS_REVISION_MISMATCH;
            break;
        case VER_GREATER_EQUAL:
            if (left < right) return STATUS_REVISION_MISMATCH;
            break;
        case VER_LESS:
            if (left >= right) return STATUS_REVISION_MISMATCH;
            break;
        case VER_LESS_EQUAL:
            if (left > right) return STATUS_REVISION_MISMATCH;
            break;
        default:
            return STATUS_REVISION_MISMATCH;
    }
    return STATUS_SUCCESS;
}

/*
* @implemented
*/
NTSTATUS
NTAPI
RtlVerifyVersionInfo(
    IN PRTL_OSVERSIONINFOEXW VersionInfo,
    IN ULONG TypeMask,
    IN ULONGLONG ConditionMask
    )
{
    RTL_OSVERSIONINFOEXW ver;
    NTSTATUS status;

    /* FIXME:
        - Check the following special case on Windows (various versions):
          o lp->wSuiteMask == 0 and ver.wSuiteMask != 0 and VER_AND/VER_OR
          o lp->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXW)
        - MSDN talks about some tests being impossible. Check what really happens.
     */

    ver.dwOSVersionInfoSize = sizeof(ver);
    status = RtlGetVersion( (PRTL_OSVERSIONINFOW)&ver );
    if (status != STATUS_SUCCESS) return status;

    if(!(TypeMask && ConditionMask)) return STATUS_INVALID_PARAMETER;

    if(TypeMask & VER_PRODUCT_TYPE)
    {
        status = version_compare_values(ver.wProductType, VersionInfo->wProductType, ConditionMask >> 7 * VER_NUM_BITS_PER_CONDITION_MASK & VER_CONDITION_MASK);
        if (status != STATUS_SUCCESS)
            return status;
    }
    if(TypeMask & VER_SUITENAME)
    {
        switch(ConditionMask >> 6 * VER_NUM_BITS_PER_CONDITION_MASK & VER_CONDITION_MASK)
        {
            case VER_AND:
                if((VersionInfo->wSuiteMask & ver.wSuiteMask) != VersionInfo->wSuiteMask)
                    return STATUS_REVISION_MISMATCH;
                break;
            case VER_OR:
                if(!(VersionInfo->wSuiteMask & ver.wSuiteMask) && VersionInfo->wSuiteMask)
                    return STATUS_REVISION_MISMATCH;
                break;
            default:
                return STATUS_INVALID_PARAMETER;
        }
    }
    if(TypeMask & VER_PLATFORMID)
    {
        status = version_compare_values(ver.dwPlatformId, VersionInfo->dwPlatformId, ConditionMask >> 3 * VER_NUM_BITS_PER_CONDITION_MASK & VER_CONDITION_MASK);
        if (status != STATUS_SUCCESS)
            return status;
    }
    if(TypeMask & VER_BUILDNUMBER)
    {
        status = version_compare_values(ver.dwBuildNumber, VersionInfo->dwBuildNumber, ConditionMask >> 2 * VER_NUM_BITS_PER_CONDITION_MASK & VER_CONDITION_MASK);
        if (status != STATUS_SUCCESS)
            return status;
    }
    if(TypeMask & (VER_MAJORVERSION|VER_MINORVERSION|VER_SERVICEPACKMAJOR|VER_SERVICEPACKMINOR))
    {
        unsigned char condition = 0;
        BOOLEAN do_next_check = TRUE;

        if(TypeMask & VER_MAJORVERSION)
            condition = ConditionMask >> 1 * VER_NUM_BITS_PER_CONDITION_MASK & VER_CONDITION_MASK;
        else if(TypeMask & VER_MINORVERSION)
            condition = ConditionMask >> 0 * VER_NUM_BITS_PER_CONDITION_MASK & VER_CONDITION_MASK;
        else if(TypeMask & VER_SERVICEPACKMAJOR)
            condition = ConditionMask >> 5 * VER_NUM_BITS_PER_CONDITION_MASK & VER_CONDITION_MASK;
        else if(TypeMask & VER_SERVICEPACKMINOR)
            condition = ConditionMask >> 4 * VER_NUM_BITS_PER_CONDITION_MASK & VER_CONDITION_MASK;

        if(TypeMask & VER_MAJORVERSION)
        {
            status = version_compare_values(ver.dwMajorVersion, VersionInfo->dwMajorVersion, condition);
            do_next_check = (ver.dwMajorVersion == VersionInfo->dwMajorVersion) &&
                ((condition != VER_EQUAL) || (status == STATUS_SUCCESS));
        }
        if((TypeMask & VER_MINORVERSION) && do_next_check)
        {
            status = version_compare_values(ver.dwMinorVersion, VersionInfo->dwMinorVersion, condition);
            do_next_check = (ver.dwMinorVersion == VersionInfo->dwMinorVersion) &&
                ((condition != VER_EQUAL) || (status == STATUS_SUCCESS));
        }
        if((TypeMask & VER_SERVICEPACKMAJOR) && do_next_check)
        {
            status = version_compare_values(ver.wServicePackMajor, VersionInfo->wServicePackMajor, condition);
            do_next_check = (ver.wServicePackMajor == VersionInfo->wServicePackMajor) &&
                ((condition != VER_EQUAL) || (status == STATUS_SUCCESS));
        }
        if((TypeMask & VER_SERVICEPACKMINOR) && do_next_check)
        {
            status = version_compare_values(ver.wServicePackMinor, VersionInfo->wServicePackMinor, condition);
        }

        if (status != STATUS_SUCCESS)
            return status;
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
ULONGLONG NTAPI
VerSetConditionMask(IN ULONGLONG dwlConditionMask,
                    IN DWORD dwTypeBitMask,
                    IN BYTE dwConditionMask)
{
  if(dwTypeBitMask == 0)
    return dwlConditionMask;

  dwConditionMask &= VER_CONDITION_MASK;

  if(dwConditionMask == 0)
    return dwlConditionMask;

  if(dwTypeBitMask & VER_PRODUCT_TYPE)
    dwlConditionMask |= dwConditionMask << 7 * VER_NUM_BITS_PER_CONDITION_MASK;
  else if(dwTypeBitMask & VER_SUITENAME)
    dwlConditionMask |= dwConditionMask << 6 * VER_NUM_BITS_PER_CONDITION_MASK;
  else if(dwTypeBitMask & VER_SERVICEPACKMAJOR)
    dwlConditionMask |= dwConditionMask << 5 * VER_NUM_BITS_PER_CONDITION_MASK;
  else if(dwTypeBitMask & VER_SERVICEPACKMINOR)
    dwlConditionMask |= dwConditionMask << 4 * VER_NUM_BITS_PER_CONDITION_MASK;
  else if(dwTypeBitMask & VER_PLATFORMID)
    dwlConditionMask |= dwConditionMask << 3 * VER_NUM_BITS_PER_CONDITION_MASK;
  else if(dwTypeBitMask & VER_BUILDNUMBER)
    dwlConditionMask |= dwConditionMask << 2 * VER_NUM_BITS_PER_CONDITION_MASK;
  else if(dwTypeBitMask & VER_MAJORVERSION)
    dwlConditionMask |= dwConditionMask << 1 * VER_NUM_BITS_PER_CONDITION_MASK;
  else if(dwTypeBitMask & VER_MINORVERSION)
    dwlConditionMask |= dwConditionMask << 0 * VER_NUM_BITS_PER_CONDITION_MASK;

  return dwlConditionMask;
}

/* EOF */
