/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/file.c
 * PURPOSE:         File handling
 * PROGRAMMERS:     Pierre Schweitzer (pierre.schweitzer@reactos.org)
 * NOTE:            Belongs to basesrv.dll
 */

/* INCLUDES ******************************************************************/

#include <w32csr.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

UINT CsrGetTempFileUnique;

/* FUNCTIONS *****************************************************************/

CSR_API(CsrGetTempFile)
{
    DPRINT1("CsrGetTempFile entered\n");

    /* Return 16-bits ID */
    Request->Data.GetTempFile.UniqueID = (++CsrGetTempFileUnique & 0xFFFF);

    DPRINT1("Returning: %u\n", Request->Data.GetTempFile.UniqueID);

    return STATUS_SUCCESS;
}
