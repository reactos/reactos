/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/lsasrv/service.c
 * PURPOSE:     Main file
 * COPYRIGHT:   Copyright 2016 Eric Kohl
 */

/* INCLUDES ****************************************************************/

#include "lsasrv.h"

/* FUNCTIONS ***************************************************************/


NTSTATUS
WINAPI
ServiceInit(VOID)
{
    TRACE("ServiceInit() called\n");
    return STATUS_SUCCESS;
}

/* EOF */
