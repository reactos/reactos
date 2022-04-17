/*******************************************************************************
 *
 * Module Name: utxferror - Various error/warning output functions
 *
 ******************************************************************************/

/*
 * Copyright (C) 2000 - 2021, Intel Corp.
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
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
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

#include "acpi.h"
#include "accommon.h"


#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("utxferror")

/*
 * This module is used for the in-kernel ACPICA as well as the ACPICA
 * tools/applications.
 */

#ifndef ACPI_NO_ERROR_MESSAGES /* Entire module */

/*******************************************************************************
 *
 * FUNCTION:    AcpiError
 *
 * PARAMETERS:  ModuleName          - Caller's module name (for error output)
 *              LineNumber          - Caller's line number (for error output)
 *              Format              - Printf format string + additional args
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print "ACPI Error" message with module/line/version info
 *
 ******************************************************************************/

void ACPI_INTERNAL_VAR_XFACE
AcpiError (
    const char              *ModuleName,
    UINT32                  LineNumber,
    const char              *Format,
    ...)
{
    va_list                 ArgList;


    ACPI_MSG_REDIRECT_BEGIN;
    AcpiOsPrintf (ACPI_MSG_ERROR);

    va_start (ArgList, Format);
    AcpiOsVprintf (Format, ArgList);
    ACPI_MSG_SUFFIX;
    va_end (ArgList);

    ACPI_MSG_REDIRECT_END;
}

ACPI_EXPORT_SYMBOL (AcpiError)


/*******************************************************************************
 *
 * FUNCTION:    AcpiException
 *
 * PARAMETERS:  ModuleName          - Caller's module name (for error output)
 *              LineNumber          - Caller's line number (for error output)
 *              Status              - Status value to be decoded/formatted
 *              Format              - Printf format string + additional args
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print an "ACPI Error" message with module/line/version
 *              info as well as decoded ACPI_STATUS.
 *
 ******************************************************************************/

void ACPI_INTERNAL_VAR_XFACE
AcpiException (
    const char              *ModuleName,
    UINT32                  LineNumber,
    ACPI_STATUS             Status,
    const char              *Format,
    ...)
{
    va_list                 ArgList;


    ACPI_MSG_REDIRECT_BEGIN;

    /* For AE_OK, just print the message */

    if (ACPI_SUCCESS (Status))
    {
        AcpiOsPrintf (ACPI_MSG_ERROR);

    }
    else
    {
        AcpiOsPrintf (ACPI_MSG_ERROR "%s, ",
            AcpiFormatException (Status));
    }

    va_start (ArgList, Format);
    AcpiOsVprintf (Format, ArgList);
    ACPI_MSG_SUFFIX;
    va_end (ArgList);

    ACPI_MSG_REDIRECT_END;
}

ACPI_EXPORT_SYMBOL (AcpiException)


/*******************************************************************************
 *
 * FUNCTION:    AcpiWarning
 *
 * PARAMETERS:  ModuleName          - Caller's module name (for warning output)
 *              LineNumber          - Caller's line number (for warning output)
 *              Format              - Printf format string + additional args
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print "ACPI Warning" message with module/line/version info
 *
 ******************************************************************************/

void ACPI_INTERNAL_VAR_XFACE
AcpiWarning (
    const char              *ModuleName,
    UINT32                  LineNumber,
    const char              *Format,
    ...)
{
    va_list                 ArgList;


    ACPI_MSG_REDIRECT_BEGIN;
    AcpiOsPrintf (ACPI_MSG_WARNING);

    va_start (ArgList, Format);
    AcpiOsVprintf (Format, ArgList);
    ACPI_MSG_SUFFIX;
    va_end (ArgList);

    ACPI_MSG_REDIRECT_END;
}

ACPI_EXPORT_SYMBOL (AcpiWarning)


/*******************************************************************************
 *
 * FUNCTION:    AcpiInfo
 *
 * PARAMETERS:  Format              - Printf format string + additional args
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print generic "ACPI:" information message. There is no
 *              module/line/version info in order to keep the message simple.
 *
 ******************************************************************************/

void ACPI_INTERNAL_VAR_XFACE
AcpiInfo (
    const char              *Format,
    ...)
{
    va_list                 ArgList;


    ACPI_MSG_REDIRECT_BEGIN;
    AcpiOsPrintf (ACPI_MSG_INFO);

    va_start (ArgList, Format);
    AcpiOsVprintf (Format, ArgList);
    AcpiOsPrintf ("\n");
    va_end (ArgList);

    ACPI_MSG_REDIRECT_END;
}

ACPI_EXPORT_SYMBOL (AcpiInfo)


/*******************************************************************************
 *
 * FUNCTION:    AcpiBiosError
 *
 * PARAMETERS:  ModuleName          - Caller's module name (for error output)
 *              LineNumber          - Caller's line number (for error output)
 *              Format              - Printf format string + additional args
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print "ACPI Firmware Error" message with module/line/version
 *              info
 *
 ******************************************************************************/

void ACPI_INTERNAL_VAR_XFACE
AcpiBiosError (
    const char              *ModuleName,
    UINT32                  LineNumber,
    const char              *Format,
    ...)
{
    va_list                 ArgList;


    ACPI_MSG_REDIRECT_BEGIN;
    AcpiOsPrintf (ACPI_MSG_BIOS_ERROR);

    va_start (ArgList, Format);
    AcpiOsVprintf (Format, ArgList);
    ACPI_MSG_SUFFIX;
    va_end (ArgList);

    ACPI_MSG_REDIRECT_END;
}

ACPI_EXPORT_SYMBOL (AcpiBiosError)


/*******************************************************************************
 *
 * FUNCTION:    AcpiBiosException
 *
 * PARAMETERS:  ModuleName          - Caller's module name (for error output)
 *              LineNumber          - Caller's line number (for error output)
 *              Status              - Status value to be decoded/formatted
 *              Format              - Printf format string + additional args
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print an "ACPI Firmware Error" message with module/line/version
 *              info as well as decoded ACPI_STATUS.
 *
 ******************************************************************************/

void ACPI_INTERNAL_VAR_XFACE
AcpiBiosException (
    const char              *ModuleName,
    UINT32                  LineNumber,
    ACPI_STATUS             Status,
    const char              *Format,
    ...)
{
    va_list                 ArgList;


    ACPI_MSG_REDIRECT_BEGIN;

    /* For AE_OK, just print the message */

    if (ACPI_SUCCESS (Status))
    {
        AcpiOsPrintf (ACPI_MSG_BIOS_ERROR);

    }
    else
    {
        AcpiOsPrintf (ACPI_MSG_BIOS_ERROR "%s, ",
            AcpiFormatException (Status));
    }

    va_start (ArgList, Format);
    AcpiOsVprintf (Format, ArgList);
    ACPI_MSG_SUFFIX;
    va_end (ArgList);

    ACPI_MSG_REDIRECT_END;
}

ACPI_EXPORT_SYMBOL (AcpiBiosException)


/*******************************************************************************
 *
 * FUNCTION:    AcpiBiosWarning
 *
 * PARAMETERS:  ModuleName          - Caller's module name (for warning output)
 *              LineNumber          - Caller's line number (for warning output)
 *              Format              - Printf format string + additional args
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print "ACPI Firmware Warning" message with module/line/version
 *              info
 *
 ******************************************************************************/

void ACPI_INTERNAL_VAR_XFACE
AcpiBiosWarning (
    const char              *ModuleName,
    UINT32                  LineNumber,
    const char              *Format,
    ...)
{
    va_list                 ArgList;


    ACPI_MSG_REDIRECT_BEGIN;
    AcpiOsPrintf (ACPI_MSG_BIOS_WARNING);

    va_start (ArgList, Format);
    AcpiOsVprintf (Format, ArgList);
    ACPI_MSG_SUFFIX;
    va_end (ArgList);

    ACPI_MSG_REDIRECT_END;
}

ACPI_EXPORT_SYMBOL (AcpiBiosWarning)

#endif /* ACPI_NO_ERROR_MESSAGES */
