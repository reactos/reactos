/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              or MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Disk devices helpers
 * COPYRIGHT:   Copyright 2025-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(DISK);

/* DISK IO ERROR SUPPORT *****************************************************/

static LONG lReportError = 0; // >= 0: display errors; < 0: hide errors.

LONG
DiskReportError(
    _In_ BOOLEAN bShowError)
{
    /* Set the reference count */
    if (bShowError) ++lReportError;
    else            --lReportError;
    return lReportError;
}

VOID
DiskError(
    _In_ PCSTR ErrorString,
    _In_ ULONG ErrorCode)
{
    PCSTR ErrorDescription;
    CHAR ErrorCodeString[200];

    if (lReportError < 0)
        return;

    ErrorDescription = DiskGetErrorCodeString(ErrorCode);
    if (ErrorDescription)
    {
        RtlStringCbPrintfA(ErrorCodeString, sizeof(ErrorCodeString),
                           "%s\n\nError Code: 0x%lx\nError: %s",
                           ErrorString, ErrorCode, ErrorDescription);
    }
    else
    {
        RtlStringCbCopyA(ErrorCodeString, sizeof(ErrorCodeString), ErrorString);
    }

    ERR("%s\n", ErrorCodeString);
    UiMessageBox(ErrorCodeString);
}
