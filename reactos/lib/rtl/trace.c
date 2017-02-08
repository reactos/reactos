/*
 * PROJECT:         ReactOS Run-Time Library
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * PURPOSE:         Rtl Trace Routines
 */

/* INCLUDES *******************************************************************/

#include <rtl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
RtlTraceDatabaseAdd(IN PRTL_TRACE_DATABASE Database, 
                    IN ULONG Count, 
                    IN PVOID *Trace,
                    OUT OPTIONAL PRTL_TRACE_BLOCK *TraceBlock)
{
    UNIMPLEMENTED;
    return FALSE;
}

PRTL_TRACE_DATABASE
NTAPI
RtlTraceDatabaseCreate(IN ULONG Buckets, 
                       IN OPTIONAL SIZE_T MaximumSize, 
                       IN ULONG Flags, 
                       IN ULONG Tag, 
                       IN OPTIONAL RTL_TRACE_HASH_FUNCTION HashFunction)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOLEAN
NTAPI
RtlTraceDatabaseDestroy(IN PRTL_TRACE_DATABASE Database)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOLEAN
NTAPI
RtlTraceDatabaseEnumerate(IN PRTL_TRACE_DATABASE Database, 
                          IN PRTL_TRACE_ENUMERATE TraceEnumerate,
                          IN OUT PRTL_TRACE_BLOCK *TraceBlock)
{
    UNIMPLEMENTED;
    return FALSE;
}


BOOLEAN
NTAPI
RtlTraceDatabaseFind(IN PRTL_TRACE_DATABASE Database, 
                     IN ULONG Count,
                     IN PVOID *Trace,
                     OUT OPTIONAL PRTL_TRACE_BLOCK *TraceBlock)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOLEAN
NTAPI
RtlTraceDatabaseLock(IN PRTL_TRACE_DATABASE Database)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOLEAN
NTAPI
RtlTraceDatabaseUnlock(IN PRTL_TRACE_DATABASE Database)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOLEAN
NTAPI
RtlTraceDatabaseValidate(IN PRTL_TRACE_DATABASE Database)
{
    UNIMPLEMENTED;
    return FALSE;
}
