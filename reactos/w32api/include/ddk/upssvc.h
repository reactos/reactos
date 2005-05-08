/*
 * upssvc.h
 *
 * UPS service interface
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __UPSSVC_H
#define __UPSSVC_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "ntddk.h"

#if defined(_APCUPS_)
  #define UPSAPI DECLSPEC_EXPORT
#else
  #define UPSAPI DECLSPEC_IMPORT
#endif


#define UPS_ONLINE                        1
#define UPS_ONBATTERY                     2
#define UPS_LOWBATTERY                    4
#define UPS_NOCOMM                        8
#define UPS_CRITICAL                      16

UPSAPI
VOID
DDKAPI
UPSCancelWait(VOID);

UPSAPI
DWORD
DDKAPI
UPSGetState(VOID);

#define UPS_INITUNKNOWNERROR              0
#define UPS_INITOK                        1
#define UPS_INITNOSUCHDRIVER              2
#define UPS_INITBADINTERFACE              3
#define UPS_INITREGISTRYERROR             4
#define UPS_INITCOMMOPENERROR             5
#define UPS_INITCOMMSETUPERROR            6

UPSAPI
DWORD
DDKAPI
UPSInit(VOID);

UPSAPI
VOID
DDKAPI
UPSStop(VOID);

UPSAPI
VOID
DDKAPI
UPSTurnOff(
  IN DWORD  aTurnOffDelay);

UPSAPI
VOID
DDKAPI
UPSWaitForStateChange(
  IN DWORD  aCurrentState,
  IN DWORD  anInterval);

#ifdef __cplusplus
}
#endif

#endif /* __UPSSVC_H */
