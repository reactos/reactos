/*
* PROJECT:         Filesystem Filter Manager
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            drivers/filters/fltmgr/Context.c
* PURPOSE:         Contains context routines
* PROGRAMMERS:     Ged Murphy (gedmurphy@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "fltmgr.h"
#include "fltmgrint.h"

#define NDEBUG
#include <debug.h>


/* DATA *********************************************************************/

static
BOOLEAN
IsContextTypeValid(
    _In_ FLT_CONTEXT_TYPE ContextType
);

static
NTSTATUS
SetupContextHeader(
    _In_ PFLT_FILTER Filter,
    _In_ PFLT_CONTEXT_REGISTRATION ContextPtr,
    _Out_ PALLOCATE_CONTEXT_HEADER ContextHeader
);

/* EXPORTED FUNCTIONS ******************************************************/




/* INTERNAL FUNCTIONS ******************************************************/


NTSTATUS
FltpRegisterContexts(_In_ PFLT_FILTER Filter,
                     _In_ const FLT_CONTEXT_REGISTRATION *Context)
{
    UNREFERENCED_PARAMETER(Filter);
    UNREFERENCED_PARAMETER(Context);
    return STATUS_NOT_IMPLEMENTED;
}


/* PRIVATE FUNCTIONS ******************************************************/

static
BOOLEAN
IsContextTypeValid(_In_ FLT_CONTEXT_TYPE ContextType)
{
    switch (ContextType)
    {
    case FLT_VOLUME_CONTEXT:
    case FLT_INSTANCE_CONTEXT:
    case FLT_FILE_CONTEXT:
    case FLT_STREAM_CONTEXT:
    case FLT_STREAMHANDLE_CONTEXT:
    case FLT_TRANSACTION_CONTEXT:
        return TRUE;
    }

    return FALSE;
}

static
NTSTATUS
SetupContextHeader(_In_ PFLT_FILTER Filter,
                   _In_ PFLT_CONTEXT_REGISTRATION ContextPtr,
                   _Out_ PALLOCATE_CONTEXT_HEADER ContextHeader)
{
    return 0;
}
