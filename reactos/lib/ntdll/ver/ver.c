/* $Id: ver.c,v 1.1 2004/11/07 13:08:24 hyperion Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS NT Layer DLL
 * FILE:            lib/ntdll/ver/ver.c
 * PURPOSE:         Operating system version checking
 * PROGRAMMERS:     KJK::Hyperion
 * HISTORY:         2004-11-07: Created (imported from Wine)
 */

#include <ddk/ntddk.h>

/*
 Header hell made me do it, don't blame me. Please move these somewhere more
 sensible
*/
#define VER_EQUAL         1
#define VER_GREATER       2
#define VER_GREATER_EQUAL 3
#define VER_LESS          4
#define VER_LESS_EQUAL    5
#define VER_AND           6
#define VER_OR            7

#define VER_CONDITION_MASK              7
#define VER_NUM_BITS_PER_CONDITION_MASK 3

#define VER_MINORVERSION     0x0000001
#define VER_MAJORVERSION     0x0000002
#define VER_BUILDNUMBER      0x0000004
#define VER_PLATFORMID       0x0000008
#define VER_SERVICEPACKMINOR 0x0000010
#define VER_SERVICEPACKMAJOR 0x0000020
#define VER_SUITENAME        0x0000040
#define VER_PRODUCT_TYPE     0x0000080

#define VER_NT_WORKSTATION       0x0000001
#define VER_NT_DOMAIN_CONTROLLER 0x0000002
#define VER_NT_SERVER            0x0000003

ULONGLONG NTAPI VerSetConditionMask
(
 IN ULONGLONG dwlConditionMask,
 IN DWORD dwTypeBitMask,
 IN BYTE dwConditionMask
)
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
