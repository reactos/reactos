/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            include/internal/iomgr.h
 * PURPOSE:         Internal io manager declarations
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *               28/05/97: Created
 */

#ifndef __INCLUDE_INTERNAL_IOMGR_H
#define __INCLUDE_INTERNAL_IOMGR_H

#include <ddk/ntddk.h>

/*
 * FUNCTION: Called to initalize a loaded driver
 * ARGUMENTS: 
 *          entry = pointer to the driver initialization routine
 * RETURNS: Success or failure
 */
NTSTATUS InitalizeLoadedDriver(PDRIVER_INITIALIZE entry);

VOID IoInitCancelHandling(VOID);


#endif
