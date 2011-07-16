/* 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsystem/win32/csrss/win32csr/file.h
 * PURPOSE:         File handling
 * PROGRAMMERS:     Pierre Schweitzer (pierre.schweitzer@reactos.org)
 * NOTE:            Belongs to basesrv.dll
 */

#pragma once

#include "api.h"

typedef struct tagCSRSS_DOS_DEVICE_HISTORY_ENTRY
{
    UNICODE_STRING Device;
    UNICODE_STRING Target;
    LIST_ENTRY Entry;
} CSRSS_DOS_DEVICE_HISTORY_ENTRY, *PCSRSS_DOS_DEVICE_HISTORY_ENTRY;

/* Api functions */
CSR_API(CsrGetTempFile);
CSR_API(CsrDefineDosDevice);

/* functions */
void CsrCleanupDefineDosDevice();

/* EOF */
