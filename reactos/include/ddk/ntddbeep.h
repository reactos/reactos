/*
 * ntddbeep.h
 *
 * Beep device IOCTL interface
 *
 * This file is part of the MinGW package.
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
 * DISCLAMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __NTDDBEEP_H
#define __NTDDBEEP_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif


#define IOCTL_BEEP_SET \
  CTL_CODE(FILE_DEVICE_BEEP,0,METHOD_BUFFERED,FILE_ANY_ACCESS)

typedef struct tagBEEP_SET_PARAMETERS {
    ULONG Frequency;
    ULONG Duration;
} BEEP_SET_PARAMETERS, *PBEEP_SET_PARAMETERS;

#define BEEP_FREQUENCY_MINIMUM  0x25
#define BEEP_FREQUENCY_MAXIMUM  0x7FFF

#ifdef __cplusplus
}
#endif

#endif /* __NTDDBEEP_H */
