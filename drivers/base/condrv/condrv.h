/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Driver
 * FILE:            drivers/base/condrv/condrv.h
 * PURPOSE:         Console Driver Management Functions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef __CONDRV_H__
#define __CONDRV_H__

/* This is needed for VisualDDK testing */
// #define __USE_VISUALDDK_AT_HOME__

#ifdef __USE_VISUALDDK_AT_HOME__
    #pragma message("Disable __USE_VISUALDDK_AT_HOME__ before committing!!")
    #include "VisualDDKHelpers.h"
#endif

#include <wdm.h>

#define CONDRV_TAG      ' noC'
#define DD_CONDRV_TAG   '1noC'
#define CONDRV_CONS_TAG '2noC'

/* Console Driver object extension */
typedef struct _CONDRV_DRIVER
{
    UNICODE_STRING RegistryPath;
    PDEVICE_OBJECT Controller; // The unique Controller device for the driver.
} CONDRV_DRIVER, *PCONDRV_DRIVER;

NTSTATUS NTAPI
ConDrvCreateController(IN PDRIVER_OBJECT DriverObject,
                       IN PUNICODE_STRING RegistryPath);
NTSTATUS NTAPI
ConDrvDeleteController(IN PDRIVER_OBJECT DriverObject);

#endif // __CONDRV_H__
