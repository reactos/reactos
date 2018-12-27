/******************************************************************************
 *
 * Module Name: nsdump - table dumping routines for debug
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2018, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#include "acpi.h"


/* TBD: This entire module is apparently obsolete and should be removed */

#define _COMPONENT          ACPI_NAMESPACE
        ACPI_MODULE_NAME    ("nsdumpdv")

#ifdef ACPI_OBSOLETE_FUNCTIONS
#if defined(ACPI_DEBUG_OUTPUT) || defined(ACPI_DEBUGGER)

#include "acnamesp.h"

/*******************************************************************************
 *
 * FUNCTION:    AcpiNsDumpOneDevice
 *
 * PARAMETERS:  Handle              - Node to be dumped
 *              Level               - Nesting level of the handle
 *              Context             - Passed into WalkNamespace
 *              ReturnValue         - Not used
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Dump a single Node that represents a device
 *              This procedure is a UserFunction called by AcpiNsWalkNamespace.
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiNsDumpOneDevice (
    ACPI_HANDLE             ObjHandle,
    UINT32                  Level,
    void                    *Context,
    void                    **ReturnValue)
{
    ACPI_BUFFER             Buffer;
    ACPI_DEVICE_INFO        *Info;
    ACPI_STATUS             Status;
    UINT32                  i;


    ACPI_FUNCTION_NAME (NsDumpOneDevice);


    Status = AcpiNsDumpOneObject (ObjHandle, Level, Context, ReturnValue);

    Buffer.Length = ACPI_ALLOCATE_LOCAL_BUFFER;
    Status = AcpiGetObjectInfo (ObjHandle, &Buffer);
    if (ACPI_SUCCESS (Status))
    {
        Info = Buffer.Pointer;
        for (i = 0; i < Level; i++)
        {
            ACPI_DEBUG_PRINT_RAW ((ACPI_DB_TABLES, " "));
        }

        ACPI_DEBUG_PRINT_RAW ((ACPI_DB_TABLES,
            "    HID: %s, ADR: %8.8X%8.8X\n",
            Info->HardwareId.Value, ACPI_FORMAT_UINT64 (Info->Address)));
        ACPI_FREE (Info);
    }

    return (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiNsDumpRootDevices
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Dump all objects of type "device"
 *
 ******************************************************************************/

void
AcpiNsDumpRootDevices (
    void)
{
    ACPI_HANDLE             SysBusHandle;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_NAME (NsDumpRootDevices);


    /* Only dump the table if tracing is enabled */

    if (!(ACPI_LV_TABLES & AcpiDbgLevel))
    {
        return;
    }

    Status = AcpiGetHandle (NULL, METHOD_NAME__SB_, &SysBusHandle);
    if (ACPI_FAILURE (Status))
    {
        return;
    }

    ACPI_DEBUG_PRINT ((ACPI_DB_TABLES,
        "Display of all devices in the namespace:\n"));

    Status = AcpiNsWalkNamespace (ACPI_TYPE_DEVICE, SysBusHandle,
        ACPI_UINT32_MAX, ACPI_NS_WALK_NO_UNLOCK,
        AcpiNsDumpOneDevice, NULL, NULL, NULL);
}

#endif
#endif
