/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    crt.c

Abstract:

    This file implements certain crt apis that are not present in
    libcntpr.lib. This implementation is NOT multi-thread safe.

Author:

    Wesley Witt (wesw) 23-May-1994

Environment:

    User Mode

--*/

#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <time.h>
#include <stdio.h>


void * __cdecl
malloc(
    size_t sz
    )
{

    return RtlAllocateHeap( RtlProcessHeap(), 0, sz );

}

void __cdecl
free(
    void * ptr
    )
{

    RtlFreeHeap( RtlProcessHeap(), 0, ptr );

}


char * __cdecl
ctime(
    const time_t *timp
    )
{
    static char    mnames[] = { "JanFebMarAprMayJunJulAugSepOctNovDec" };
    static char    buf[32];

    LARGE_INTEGER  MyTime;
    TIME_FIELDS    TimeFields;


    RtlSecondsSince1970ToTime( (ULONG)*timp, &MyTime );
    RtlSystemTimeToLocalTime( &MyTime, &MyTime );
    RtlTimeToTimeFields( &MyTime, &TimeFields );

    strncpy( buf, &mnames[(TimeFields.Month - 1) * 3], 3 );
    sprintf( &buf[3], " %02d %02d:%02d:%02d %04d",
             TimeFields.Day, TimeFields.Hour, TimeFields.Minute,
             TimeFields.Second, TimeFields.Year );

    return buf;
}


time_t __cdecl
time(
    time_t *timp
    )
{
    time_t         tm;
    LARGE_INTEGER  MyTime;


    NtQuerySystemTime( &MyTime );
    RtlTimeToSecondsSince1970( &MyTime, (PULONG)&tm );

    if (timp) {
        *timp = tm;
    }

    return tm;
}

#ifdef __cplusplus
}
#endif
