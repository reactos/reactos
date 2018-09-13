/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    csrbeep.c

Abstract:

    This module implements functions that are used by the Win32 Beep APIs to communicate
    with csrss.

Author:

    Michael Zoran (mzoran) 21-Jun-1998

Revision History:

--*/

#include "basedll.h"

VOID
CsrBasepSoundSentryNotification(
    ULONG VideoMode
    )
{

#if defined(BUILD_WOW6432)
    
    NtWow64CsrBasepSoundSentryNotification(VideoMode);

#else

    BASE_API_MSG m;
    PBASE_SOUNDSENTRY_NOTIFICATION_MSG e =
            (PBASE_SOUNDSENTRY_NOTIFICATION_MSG)&m.u.SoundSentryNotification;

    e->VideoMode = VideoMode;

    CsrClientCallServer((PCSR_API_MSG)&m,
                        NULL,
                        CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                             BasepSoundSentryNotification ),
                        sizeof( *e )
                       );
#endif

}


