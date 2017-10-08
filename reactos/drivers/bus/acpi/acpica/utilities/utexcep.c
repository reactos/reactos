/*******************************************************************************
 *
 * Module Name: utexcep - Exception code support
 *
 ******************************************************************************/

/*
 * Copyright (C) 2000 - 2017, Intel Corp.
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

#define EXPORT_ACPI_INTERFACES

#define ACPI_DEFINE_EXCEPTION_TABLE
#include "acpi.h"
#include "accommon.h"


#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("utexcep")


/*******************************************************************************
 *
 * FUNCTION:    AcpiFormatException
 *
 * PARAMETERS:  Status              - The ACPI_STATUS code to be formatted
 *
 * RETURN:      A string containing the exception text. A valid pointer is
 *              always returned.
 *
 * DESCRIPTION: This function translates an ACPI exception into an ASCII
 *              string. Returns "unknown status" string for invalid codes.
 *
 ******************************************************************************/

const char *
AcpiFormatException (
    ACPI_STATUS             Status)
{
    const ACPI_EXCEPTION_INFO   *Exception;


    ACPI_FUNCTION_ENTRY ();


    Exception = AcpiUtValidateException (Status);
    if (!Exception)
    {
        /* Exception code was not recognized */

        ACPI_ERROR ((AE_INFO,
            "Unknown exception code: 0x%8.8X", Status));

        return ("UNKNOWN_STATUS_CODE");
    }

    return (Exception->Name);
}

ACPI_EXPORT_SYMBOL (AcpiFormatException)


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtValidateException
 *
 * PARAMETERS:  Status              - The ACPI_STATUS code to be formatted
 *
 * RETURN:      A string containing the exception text. NULL if exception is
 *              not valid.
 *
 * DESCRIPTION: This function validates and translates an ACPI exception into
 *              an ASCII string.
 *
 ******************************************************************************/

const ACPI_EXCEPTION_INFO *
AcpiUtValidateException (
    ACPI_STATUS             Status)
{
    UINT32                      SubStatus;
    const ACPI_EXCEPTION_INFO   *Exception = NULL;


    ACPI_FUNCTION_ENTRY ();


    /*
     * Status is composed of two parts, a "type" and an actual code
     */
    SubStatus = (Status & ~AE_CODE_MASK);

    switch (Status & AE_CODE_MASK)
    {
    case AE_CODE_ENVIRONMENTAL:

        if (SubStatus <= AE_CODE_ENV_MAX)
        {
            Exception = &AcpiGbl_ExceptionNames_Env [SubStatus];
        }
        break;

    case AE_CODE_PROGRAMMER:

        if (SubStatus <= AE_CODE_PGM_MAX)
        {
            Exception = &AcpiGbl_ExceptionNames_Pgm [SubStatus];
        }
        break;

    case AE_CODE_ACPI_TABLES:

        if (SubStatus <= AE_CODE_TBL_MAX)
        {
            Exception = &AcpiGbl_ExceptionNames_Tbl [SubStatus];
        }
        break;

    case AE_CODE_AML:

        if (SubStatus <= AE_CODE_AML_MAX)
        {
            Exception = &AcpiGbl_ExceptionNames_Aml [SubStatus];
        }
        break;

    case AE_CODE_CONTROL:

        if (SubStatus <= AE_CODE_CTRL_MAX)
        {
            Exception = &AcpiGbl_ExceptionNames_Ctrl [SubStatus];
        }
        break;

    default:

        break;
    }

    if (!Exception || !Exception->Name)
    {
        return (NULL);
    }

    return (Exception);
}
