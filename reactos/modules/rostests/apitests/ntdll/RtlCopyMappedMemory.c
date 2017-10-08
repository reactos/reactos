/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for RtlCopyMappedMemory
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <ndk/rtlfuncs.h>

START_TEST(RtlCopyMappedMemory)
{
    NTSTATUS Status;
    UCHAR Buffer1[32];
    UCHAR Buffer2[32];
    
    StartSeh() RtlCopyMappedMemory(NULL, NULL, 1);      EndSeh(STATUS_ACCESS_VIOLATION);
    StartSeh() RtlCopyMappedMemory(Buffer1, NULL, 1);   EndSeh(STATUS_ACCESS_VIOLATION);
    StartSeh() RtlCopyMappedMemory(NULL, Buffer1, 1);   EndSeh(STATUS_ACCESS_VIOLATION);
    
    StartSeh()
        Status = RtlCopyMappedMemory(NULL, NULL, 0);
    EndSeh(STATUS_SUCCESS);
    ok(Status == STATUS_SUCCESS, "RtlCopyMappedMemory returned %lx\n", Status);
    
    RtlFillMemory(Buffer1, sizeof(Buffer1), 0x11);
    RtlFillMemory(Buffer2, sizeof(Buffer2), 0x22);
    StartSeh()
        Status = RtlCopyMappedMemory(Buffer1, Buffer2, sizeof(Buffer1));
    EndSeh(STATUS_SUCCESS);
    ok(Status == STATUS_SUCCESS, "RtlCopyMappedMemory returned %lx\n", Status);
    ok(RtlCompareMemory(Buffer1, Buffer2, sizeof(Buffer1)) == sizeof(Buffer1), "Data not copied\n");
}
