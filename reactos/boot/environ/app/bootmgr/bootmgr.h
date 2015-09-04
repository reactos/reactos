/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Manager
 * FILE:            boot/environ/app/bootmgr.h
 * PURPOSE:         Main Boot Manager Header
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
*/

#ifndef _BOOTMGR_H
#define _BOOTMGR_H

/* INCLUDES ******************************************************************/

/* C Headers */
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>

/* NT Base Headers */
#include <ntifs.h>

/* UEFI Headers */
#include <Uefi.h>

/* Boot Library Headers */
#include <bl.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
BmMain (
    _In_ PBOOT_APPLICATION_PARAMETER_BLOCK BootParameters
    );

#endif
