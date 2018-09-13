/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    tprofile.c

Abstract:

    User-mode test for profile object.

    Note, this will be added to TEX.C

Author:

    Lou Perazzoli (loup) 24-Sep-1990

Revision History:

--*/
#include <nt.h>

main()
{
    HANDLE Profile, Profile2;
    ULONG Hack;
    PULONG Buffer;
    HANDLE CurrentProcessHandle;
    ULONG Size1;
    NTSTATUS status;

    Buffer = &Hack;

    CurrentProcessHandle = NtCurrentProcess();

    status = NtCreateProfile (&Profile,
                              CurrentProcessHandle,
                              NULL,
                              0xFFFFFFFF,
                              16,
                              Buffer,
                              (ULONG)64*1024,
                              ProfileTime,
                              (KAFFINITY)-1);

    if (status != STATUS_SUCCESS) {
        DbgPrint("(Expected) create profile #1 failed - status %lx\n", status);
    }

    status = NtStartProfile (Profile);
    if (status != STATUS_SUCCESS) {
        DbgPrint("(Expected) start profile #1 failed - status %lx\n", status);
    }

    status = NtStopProfile (Profile);
    if (status != STATUS_SUCCESS) {
        DbgPrint("(Expected) stop profile #1 failed - status %lx\n", status);
    }

    Size1 = 1024*64;
    Buffer = NULL;

    status = NtAllocateVirtualMemory (CurrentProcessHandle, (PVOID *)&Buffer,
                        0, &Size1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    //
    // This should fail as buffersize is too small.
    //

    status = NtCreateProfile (&Profile,
                              NtCurrentProcess(),
                              NULL,
                              0xFFFFFFFF,
                              16,
                              Buffer,
                              (ULONG)64*1024,
                              ProfileTime,
                              (KAFFINITY)-1);

    if (status != STATUS_SUCCESS) {
        DbgPrint("(Expected) create profile #2 failed - status %lx\n", status);
    }

    status = NtStartProfile (Profile);
    if (status != STATUS_SUCCESS) {
        DbgPrint("(Expected) start profile #2 failed - status %lx\n", status);
    }

    status = NtStopProfile (Profile);
    if (status != STATUS_SUCCESS) {
        DbgPrint("(Expected) stop profile #2 failed - status %lx\n", status);
    }

    status = NtClose (Profile);

    //
    // This should succeed.
    //

    status = NtCreateProfile (&Profile,
                              NtCurrentProcess(),
                              NULL,
                              0xFFFFFFFF,
                              18,
                              Buffer,
                              (ULONG)64*1024,
                              ProfileTime,
                              (KAFFINITY)-1);

    if (status != STATUS_SUCCESS) {
        DbgPrint("(Unexpected) create profile #3 failed - status %lx\n", status);
    }

    status = NtStartProfile (Profile);
    if (status != STATUS_SUCCESS) {
        DbgPrint("(Unexpected) start profile #3 failed - status %lx\n", status);
    }

    status = NtStopProfile (Profile);
    if (status != STATUS_SUCCESS) {
        DbgPrint("(Unexpected) stop profile #3 failed - status %lx\n", status);
    }

    status = NtClose (Profile);

    //
    // Attempt to create a profile that can't work because the
    // address range is too big.
    //

    status = NtCreateProfile (&Profile,
                              NtCurrentProcess(),
                              (PVOID)0x203030,
                              0xffffffff,
                              6,
                              Buffer,
                              (ULONG)64*1024,
                              ProfileTime,
                              (KAFFINITY)-1);

    if (status != STATUS_SUCCESS) {
        DbgPrint("(Expected) create profile #4 failed - status %lx\n", status);
    }

    status = NtStartProfile (Profile);
    if (status != STATUS_SUCCESS) {
        DbgPrint("(Expected) start profile #4 failed - status %lx\n", status);
    }

    status = NtStopProfile (Profile);
    if (status != STATUS_SUCCESS) {
        DbgPrint("(Expected) stop profile #4 failed - status %lx\n", status);
    }

    status = NtClose (Profile);

    //
    // Attempt to create a sucessful profile.
    //

    status = NtCreateProfile (&Profile,
                              NtCurrentProcess(),
                              (PVOID)0x80000000,
                              0x7fffffff,
                              17,
                              Buffer,
                              (ULONG)64*1024,
                              ProfileTime,
                              (KAFFINITY)-1);

    if (status != STATUS_SUCCESS) {
        DbgPrint("(Unexpected) create profile #5 failed - status %lx\n", status);
    }

    status = NtStartProfile (Profile);
    if (status != STATUS_SUCCESS) {
        DbgPrint("(Unexpected) start profile #5 failed - status %lx\n", status);
    }

    status = NtStopProfile (Profile);
    if (status != STATUS_SUCCESS) {
        DbgPrint("(Unexpected) stop profile #5 failed - status %lx\n", status);
    }

    //
    // now start it again.
    //

    status = NtStartProfile (Profile);
    if (status != STATUS_SUCCESS) {
        DbgPrint("(Unexpected) start profile #6.1 failed - status %lx\n", status);
    }

    //
    // now start it again, should fail.
    //

    status = NtStartProfile (Profile);
    if (status != STATUS_SUCCESS) {
        DbgPrint("(Expected) start profile #6.2 failed - status %lx\n", status);
    }

    //
    // now create another one (using the same buffer).
    //

    status = NtCreateProfile (&Profile2,
                              NtCurrentProcess(),
                              NULL,
                              0x3000000,
                              15,
                              Buffer,
                              (ULONG)64*1024,
                              ProfileTime,
                              (KAFFINITY)-1);


    if (status != STATUS_SUCCESS) {
        DbgPrint("create profile #7 failed - status %lx\n", status);
    }

    status = NtStartProfile (Profile2);
    if (status != STATUS_SUCCESS) {
        DbgPrint("start profile #7.1 failed - status %lx\n", status);
    }

    status = NtStopProfile (Profile2);
    if (status != STATUS_SUCCESS) {
        DbgPrint("stop profile #7.2 failed - status %lx\n", status);
    }

    status = NtStopProfile (Profile2);
    if (status != STATUS_SUCCESS) {
        DbgPrint("(Expected) stop profile #7.3 failed - status %lx\n", status);
    }

    status = NtStartProfile (Profile2);
    if (status != STATUS_SUCCESS) {
        DbgPrint("start profile #7.4 failed - status %lx\n", status);
    }

    status = NtClose (Profile);
    if (status != STATUS_SUCCESS) {
        DbgPrint("close profile #7.5 failed - status %lx\n", status);
    }

    return status;
}


