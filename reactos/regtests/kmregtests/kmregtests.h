/*
 * PROJECT:         ReactOS kernel
 * FILE:            regtests/kmregtests/kmregtests.h
 * PURPOSE:         Kernel-mode component regression testing
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-07-2003  CSH  Created
 */
#include <ntos.h>

/* KMREGTESTS IOCTL code definitions */

#define FSCTL_KMREGTESTS_BASE     FILE_DEVICE_NAMED_PIPE // ???

#define KMREGTESTS_CTL_CODE(Function, Method, Access) \
  CTL_CODE(FSCTL_KMREGTESTS_BASE, Function, Method, Access)

#define IOCTL_KMREGTESTS_RUN \
  KMREGTESTS_CTL_CODE(0, METHOD_BUFFERED, FILE_ANY_ACCESS)
