/*
 * ntddbeep.h
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

#ifndef _NTDDBEEP_
#define _NTDDBEEP_


#ifdef __cplusplus
extern "C" {
#endif

#define DD_BEEP_DEVICE_NAME    "\\Device\\Beep"
#define DD_BEEP_DEVICE_NAME_U  L"\\Device\\Beep"
#define BEEP_FREQUENCY_MINIMUM 0x25
#define BEEP_FREQUENCY_MAXIMUM 0x7FFF
#define IOCTL_BEEP_SET         CTL_CODE(FILE_DEVICE_BEEP, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _BEEP_SET_PARAMETERS
{
   ULONG Frequency;
   ULONG Duration;
} BEEP_SET_PARAMETERS, *PBEEP_SET_PARAMETERS;

#ifdef __cplusplus
}
#endif
#endif

