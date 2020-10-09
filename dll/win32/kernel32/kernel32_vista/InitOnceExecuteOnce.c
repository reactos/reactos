
#include "k32_vista.h"

#include <ndk/exfuncs.h>
#include <wine/config.h>
#include <wine/port.h>

DWORD WINAPI RtlRunOnceExecuteOnce( RTL_RUN_ONCE *once, PRTL_RUN_ONCE_INIT_FN func,
                                           void *param, void **context );

/* Taken from Wine kernel32/sync.c */

/*
 * @implemented
 */
BOOL NTAPI InitOnceExecuteOnce( INIT_ONCE *once, PINIT_ONCE_FN func, void *param, void **context )
{
    return !RtlRunOnceExecuteOnce( once, (PRTL_RUN_ONCE_INIT_FN)func, param, context );
}
