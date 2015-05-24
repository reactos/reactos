/*
* PROJECT:         ReactOS kernel-mode tests
* LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
* PURPOSE:         Kernel-Mode Test Suite FsRtl Test
* PROGRAMMER:      Ashuha Arseny, Moscow State Technical University
*                  Marina Volosnikova, Moscow State Technical University
*                  Denis Petkevich, Moscow State Technical University
*/

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

/*
Tested with the system kmtest 
the following functions:
FsRtlInitializeTunnelCache
FsRtlDeleteTunnelCache
FsRtlAddToTunnelCache
FsRtlDeleteKeyFromTunnelCache
FsRtlFindInTunnelCache
*/

static PTUNNEL T;
static PTUNNEL Tb;

#define BufSize 10000

PUNICODE_STRING CopyUS(PUNICODE_STRING a)
{
    PUNICODE_STRING b = (PUNICODE_STRING)ExAllocatePool(PagedPool,sizeof(UNICODE_STRING));
    ok(b != NULL, "US is NULL after allocated memory\n");
    b->Length = 0;
    b->MaximumLength =a->MaximumLength;
    if (b->MaximumLength)
    {
        b->Buffer = (PWSTR)ExAllocatePoolWithTag(PagedPool, b->MaximumLength, 1633);
        ok(b->Buffer != NULL, "US->Buffer is NULL after allocated memory\n");
        RtlCopyUnicodeString(b, a);
    }
    else
    {
        b->Buffer = NULL;
    }
    return b;
}

void TestFsRtlInitializeTunnelCache()
{
    SIZE_T eq;
    T = ExAllocatePool(PagedPool, sizeof(TUNNEL));
    ok(T != NULL, "PTUNEL is NULL after allocated memory\n");
    Tb = ExAllocatePool(PagedPool, sizeof(TUNNEL));
    ok(Tb != NULL, "PTUNEL is NULL after allocated memory\n");

    memset((void*)T, 0, sizeof(TUNNEL));
    memset((void*)Tb, 0, sizeof(TUNNEL));

    FsRtlInitializeTunnelCache(T);

    eq = RtlCompareMemory((const VOID*)T, (const VOID*)Tb,  sizeof(TUNNEL));

    ok ( eq != sizeof(TUNNEL), "FsRtlInitializeTunnelCache function did not change anything in the memory at the address PTUNEL.\n"); 
}

void TestFsRtlAddToTunnelCache(ULONGLONG DirectoryKey, PUNICODE_STRING s_name, PUNICODE_STRING l_name, BOOLEAN KeyByShortName)
{
    SIZE_T eq;
    LONG b;
    PUNICODE_STRING bs_name;
    PUNICODE_STRING bl_name;
    PVOID Bufb;
    PVOID Buf;

    Buf = ExAllocatePool(PagedPool, BufSize);
    ok(Buf != NULL, "Buff in TestFsRtlAddToTunnelCache is NULL after allocated memory\n");
    Bufb = ExAllocatePool(PagedPool, BufSize);
    ok(Bufb != NULL, "Buff in TestFsRtlAddToTunnelCache is NULL after allocated memory\n");

    // Allocate memory for the  bufs_name
    bs_name = CopyUS(s_name);

    // Allocate memory for the l_name and bl_name
    bl_name = CopyUS(l_name);

    memset((void*)Buf, 0, BufSize);
    memset((void*)Bufb, 0, BufSize);

    FsRtlAddToTunnelCache(T, DirectoryKey, s_name, l_name, KeyByShortName, BufSize, Buf);

    eq = RtlCompareMemory((const VOID*)Buf, (const VOID*)Bufb,  BufSize);

    ok( eq !=  sizeof(TUNNEL),"FsRtlAddToTunnelCache function did not change anything in the memory at the address Buf.\n"); 

    b = RtlCompareUnicodeString(l_name, bl_name, TRUE);
    ok (b == 0, "long name after call FsRtlAddToTunnelCache != long name befo call FsRtlAddToTunnelCache\n\n");
    b = RtlCompareUnicodeString(s_name, bs_name, TRUE);
    ok (b == 0, "short name after call FsRtlAddToTunnelCache != short name befo call FsRtlAddToTunnelCache\n\n");

    if (bs_name->Buffer) ExFreePool(bs_name->Buffer);
    ExFreePool(bs_name);
    if (bl_name->Buffer) ExFreePool(bl_name->Buffer);
    ExFreePool(bl_name);
    ExFreePool(Bufb);
    ExFreePool(Buf);
}

BOOLEAN TestFsRtlFindInTunnelCache(ULONG DirectoryKey, PUNICODE_STRING name, PUNICODE_STRING s_name, PUNICODE_STRING l_name)
{
    // Allocate memory for the Buf
    ULONG BufsizeTemp = BufSize;
    PVOID Buf = ExAllocatePool(PagedPool, BufSize*2);
    ok(Buf != NULL, "Buff in FsRtlFindInTunnelCache is NULL after allocated memory\n");

    return FsRtlFindInTunnelCache(T, DirectoryKey, name, s_name, l_name, &BufsizeTemp, Buf); 
}

void TestFsRtlDeleteKeyFromTunnelCache(ULONGLONG a)
{
    FsRtlDeleteKeyFromTunnelCache(T, a);
}

START_TEST(FsRtlTunnel)
{
    PUNICODE_STRING s_name;
    PUNICODE_STRING l_name;
    PUNICODE_STRING name;
    PUNICODE_STRING a;
    BOOLEAN is;

    //Initialize Cash
    TestFsRtlInitializeTunnelCache();

    s_name = (PUNICODE_STRING)ExAllocatePool(PagedPool,sizeof(UNICODE_STRING));
    ok(s_name != NULL, "s_name in TestFsRtlAddToTunnelCache is NULL after allocated memory\n");
    RtlInitUnicodeString(s_name, L"smal");

    l_name = (PUNICODE_STRING)ExAllocatePool(PagedPool,sizeof(UNICODE_STRING));
    ok(l_name != NULL, "l_name in TestFsRtlAddToTunnelCache is NULL after allocated memory\n");
    RtlInitUnicodeString(l_name, L"bigbigbigbigbig");

    // Add elem
    TestFsRtlAddToTunnelCache(12345, s_name, l_name, TRUE);

    name = (PUNICODE_STRING)ExAllocatePool(PagedPool,sizeof(UNICODE_STRING));
    ok(name != NULL, "name in FsRtlFindInTunnelCache is NULL after allocated memory\n");
    RtlInitUnicodeString(name, L"smal");

    // Find
    is = TestFsRtlFindInTunnelCache(12345, name, s_name, l_name);
    ok(is == TRUE, "FsRtlFindInTunnelCache dosn't find elem id = 12345\n");

    TestFsRtlDeleteKeyFromTunnelCache(12345);	//Delete
    is = TestFsRtlFindInTunnelCache(12345, name, s_name, l_name);
    ok(is == FALSE, "TestFsRtlDeleteKeyFromTunnelCache dosn't delete elem id = 12345\n");

    is = TestFsRtlFindInTunnelCache(12347, name, s_name, l_name);
    ok(is == FALSE, "FsRtlDeleteTunnelCache dosn't clear cash\n");

    TestFsRtlAddToTunnelCache(12345, s_name, l_name, TRUE);
    TestFsRtlAddToTunnelCache(12347, s_name, l_name, TRUE);
    a = (PUNICODE_STRING)ExAllocatePool(PagedPool,sizeof(UNICODE_STRING));
    RtlInitUnicodeString(a, NULL);
    TestFsRtlAddToTunnelCache(12346, a, l_name, FALSE);

    //Clear all
    FsRtlDeleteTunnelCache(T);

    is = TestFsRtlFindInTunnelCache(12345, name, s_name, l_name);
    ok(is == FALSE, "FsRtlDeleteTunnelCache dosn't clear cash\n");

    is = TestFsRtlFindInTunnelCache(12346, name, a, l_name);
    ok(is == FALSE, "FsRtlDeleteTunnelCache dosn't clear cash\n");

    is = TestFsRtlFindInTunnelCache(12347, name, s_name, l_name);
    ok(is == FALSE, "FsRtlDeleteTunnelCache dosn't clear cash\n");

    ExFreePool(a);
    ExFreePool(name);
    ExFreePool(l_name);
    ExFreePool(s_name);

    ExFreePool(Tb);
    ExFreePool(T);
}
