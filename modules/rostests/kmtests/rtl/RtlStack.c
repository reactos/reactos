/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Runtime library stack trace test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#define KMT_EMULATE_KERNEL
#include <kmt_test.h>

static PVOID ReturnAddresses[4];

static
VOID
TestStackWalk3(VOID);

DECLSPEC_NOINLINE
static
VOID
TestStackWalk4(VOID)
{
    PVOID Frames[5];
    ULONG Ret;
    ULONG Hash;
    ULONG ExpectedHash;
    ULONG i;
    const ULONG FunctionSizeGuess = 0x1000;

    ReturnAddresses[3] = _ReturnAddress();

    Ret = RtlWalkFrameChain(NULL, 5, 0);
    ok_eq_ulong(Ret, 0);

    RtlFillMemory(Frames, sizeof(Frames), 0x55);
    Ret = RtlWalkFrameChain(Frames, 0, 0);
    ok_eq_ulong(Ret, 0);
    ok_eq_pointer(Frames[0], (PVOID)(ULONG_PTR)0x5555555555555555);

    RtlFillMemory(Frames, sizeof(Frames), 0x55);
    Ret = RtlWalkFrameChain(Frames, 5, 0);
    ok_eq_ulong(Ret, 5);
    ok((ULONG_PTR)Frames[0] > (ULONG_PTR)TestStackWalk4, "Frame is %p, function is %p\n", Frames[0], TestStackWalk4);
    ok((ULONG_PTR)Frames[0] < (ULONG_PTR)TestStackWalk4 + FunctionSizeGuess, "Frame is %p, function is %p\n", Frames[0], TestStackWalk4);
    ok_eq_pointer(Frames[1], ReturnAddresses[3]);
    ok_eq_pointer(Frames[2], ReturnAddresses[2]);
    ok_eq_pointer(Frames[3], ReturnAddresses[1]);
    ok_eq_pointer(Frames[4], ReturnAddresses[0]);

    RtlFillMemory(Frames, sizeof(Frames), 0x55);
    Ret = RtlWalkFrameChain(Frames, 4, 0);
    ok_eq_ulong(Ret, 4);
    ok((ULONG_PTR)Frames[0] > (ULONG_PTR)TestStackWalk4, "Frame is %p, function is %p\n", Frames[0], TestStackWalk4);
    ok((ULONG_PTR)Frames[0] < (ULONG_PTR)TestStackWalk4 + FunctionSizeGuess, "Frame is %p, function is %p\n", Frames[0], TestStackWalk4);
    ok_eq_pointer(Frames[1], ReturnAddresses[3]);
    ok_eq_pointer(Frames[2], ReturnAddresses[2]);
    ok_eq_pointer(Frames[3], ReturnAddresses[1]);
    ok_eq_pointer(Frames[4], (PVOID)(ULONG_PTR)0x5555555555555555);

    KmtStartSeh()
        RtlCaptureStackBackTrace(0, 5, NULL, NULL);
    KmtEndSeh(STATUS_ACCESS_VIOLATION);

    RtlFillMemory(Frames, sizeof(Frames), 0x55);
    Hash = 0x55555555;
    Ret = RtlCaptureStackBackTrace(0, 0, Frames, &Hash);
    ok_eq_ulong(Ret, 0);
    ok_eq_hex(Hash, 0x55555555);
    ok_eq_pointer(Frames[0], (PVOID)(ULONG_PTR)0x5555555555555555);

    RtlFillMemory(Frames, sizeof(Frames), 0x55);
    Hash = 0x55555555;
    Ret = RtlCaptureStackBackTrace(0, 1, Frames, NULL);
    ok_eq_ulong(Ret, 1);
    ok((ULONG_PTR)Frames[0] > (ULONG_PTR)TestStackWalk4, "Frame is %p, function is %p\n", Frames[0], TestStackWalk4);
    ok((ULONG_PTR)Frames[0] < (ULONG_PTR)TestStackWalk4 + FunctionSizeGuess, "Frame is %p, function is %p\n", Frames[0], TestStackWalk4);
    ok_eq_pointer(Frames[1], (PVOID)(ULONG_PTR)0x5555555555555555);

    RtlFillMemory(Frames, sizeof(Frames), 0x55);
    Ret = RtlCaptureStackBackTrace(0, 5, Frames, &Hash);
    ok_eq_ulong(Ret, 5);
    ExpectedHash = 0;
    for (i = 0; i < 5; i++)
        ExpectedHash += (ULONG)(ULONG_PTR)Frames[i];
    ok_eq_hex(Hash, ExpectedHash);
    ok((ULONG_PTR)Frames[0] > (ULONG_PTR)TestStackWalk4, "Frame is %p, function is %p\n", Frames[0], TestStackWalk4);
    ok((ULONG_PTR)Frames[0] < (ULONG_PTR)TestStackWalk4 + FunctionSizeGuess, "Frame is %p, function is %p\n", Frames[0], TestStackWalk4);
    ok_eq_pointer(Frames[1], ReturnAddresses[3]);
    ok_eq_pointer(Frames[2], ReturnAddresses[2]);
    ok_eq_pointer(Frames[3], ReturnAddresses[1]);
    ok_eq_pointer(Frames[4], ReturnAddresses[0]);

    RtlFillMemory(Frames, sizeof(Frames), 0x55);
    Ret = RtlCaptureStackBackTrace(1, 4, Frames, &Hash);
    ok_eq_ulong(Ret, 4);
    ExpectedHash = 0;
    for (i = 0; i < 4; i++)
        ExpectedHash += (ULONG)(ULONG_PTR)Frames[i];
    ok_eq_hex(Hash, ExpectedHash);
    ok_eq_pointer(Frames[0], ReturnAddresses[3]);
    ok_eq_pointer(Frames[1], ReturnAddresses[2]);
    ok_eq_pointer(Frames[2], ReturnAddresses[1]);
    ok_eq_pointer(Frames[3], ReturnAddresses[0]);
    ok_eq_pointer(Frames[4], (PVOID)(ULONG_PTR)0x5555555555555555);
}

DECLSPEC_NOINLINE
static
VOID
TestStackWalk3(VOID)
{
    ReturnAddresses[2] = _ReturnAddress();
    TestStackWalk4();
}

DECLSPEC_NOINLINE
static
VOID
TestStackWalk2(VOID)
{
    ReturnAddresses[1] = _ReturnAddress();
    TestStackWalk3();
}

DECLSPEC_NOINLINE
static
VOID
TestStackWalk1(VOID)
{
    ReturnAddresses[0] = _ReturnAddress();
    TestStackWalk2();
}

#ifdef _M_AMD64
NTSYSAPI
PVOID
NTAPI
RtlPcToFileHeader(
    _In_  PVOID PcValue,
    _Out_ PVOID *BaseOfImage);

extern char __ImageBase;

DECLSPEC_NOINLINE
static
VOID
TestRtlPcToFileHeader(VOID)
{
    PVOID ImageBase, Result;
    PTEB Teb;
    PPEB Peb;

    /* First test a function from this image */
    Result = RtlPcToFileHeader(&TestRtlPcToFileHeader, &ImageBase);
    ok_eq_pointer(Result, ImageBase);
    ok_eq_pointer(ImageBase, &__ImageBase);

#ifdef NTOS_MODE_USER
    Teb = NtCurrentTeb();
#else
    Teb = KeGetCurrentThread()->Teb;
#endif
    ok(Teb != NULL, "Teb is NULL!\n");
    if (Teb == NULL)
    {
        return;
    }

    _SEH2_TRY
    {
        Peb = Teb->ProcessEnvironmentBlock;
        ok(Peb != NULL, "Peb is NULL!\n");
        if (Peb == NULL)
        {
            return;
        }

        /* Test an address somewhere within the main image of the current process */
        Result = RtlPcToFileHeader((PUCHAR)Peb->ImageBaseAddress + 0x1000, &ImageBase);
        ok_eq_pointer(Result, ImageBase);
#ifdef NTOS_MODE_USER
        ok_eq_pointer(ImageBase, Peb->ImageBaseAddress);
#else
        ok_eq_pointer(ImageBase, NULL);
#endif
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ok(FALSE, "Got an exception!\n");
    }
    _SEH2_END
}
#endif // _M_AMD64

START_TEST(RtlStack)
{
    TestStackWalk1();
#ifdef _M_AMD64
    TestRtlPcToFileHeader();
#endif // _M_AMD64
}
