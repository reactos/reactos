/*
* PROJECT:         ReactOS api tests
* LICENSE:         GPL - See COPYING in the top level directory
* PURPOSE:         Test for RtlVirtualUnwind
* PROGRAMMER:      Timo Kreuzer
*/
//#define _NTSYSTEM_

#include "precomp.h"
#include <malloc.h>

#define ok_eq_print(value, expected, spec)  ok((value) == (expected), #value " = " spec ", expected " spec "\n", value, expected)
#define ok_eq_hex(value, expected)    ok_eq_print(value, expected, "%x")
#define ok_eq_hex64(value, expected)    ok_eq_print(value, expected, "%I64x")
#define ok_eq_xmm(value, expected)    ok((value).Low == (expected).Low, #value " = %I64x'%08I64x, expected %I64x'%08I64x\n", (value).Low, (value).High, (expected).Low, (expected).High)

#define ok_eq_print_str(str, value, expected, spec)  ok((value) == (expected), "%s: " #value " = " spec ", expected " spec "\n", str, value, expected)
#define ok_eq_hex64_str(str, value, expected)    ok_eq_print_str(str, value, expected, "0x%I64x")

void Test_Generated()
{
}

void Test_EstablisherFrame()
{
}

typedef
VOID
HELPER_FUNCTION(
    PCONTEXT CapturedContext);
typedef HELPER_FUNCTION *PHELPER_FUNCTION;

typedef struct
{
    PCSTR TestName;
    PHELPER_FUNCTION Function;
    ULONG64 CodeOffset;
    ULONG HandlerType;
    ULONG64 ExpectedFrameOffset;
    BOOLEAN ValidateContext;
} TEST_ENTRY;


VOID
UnwindCallWrapper(
    _Out_ PCONTEXT UnwindContext,
    _Out_ PCONTEXT ExpectedContext,
    _In_ PHELPER_FUNCTION Function);

void
Test_SingleFunction(
    _In_z_ PCSTR TestName,
    _In_ PHELPER_FUNCTION Function,
    _In_ ULONG64 CodeOffset,
    _In_ ULONG HandlerType,
    _In_ ULONG64 ExpectedFrameOffset,
    _In_ BOOLEAN ValidateContext
)
{
    ULONG64 EstablisherFrameOffset;
    PEXCEPTION_ROUTINE ExceptionRoutine;
    PRUNTIME_FUNCTION FunctionEntry;
    CONTEXT UnwindContext, ExpectedContext;
    PVOID HandlerData;
    ULONG64 EstablisherFrame;
    ULONG64 CapturedRsp, CapturedRbp;
    ULONG64 ImageBase;

    /* Call the function to cature it's context */
    UnwindCallWrapper(&UnwindContext, &ExpectedContext, Function);

    /* Save the original Rsp */
    CapturedRsp = UnwindContext.Rsp;
    CapturedRbp = UnwindContext.Rbp;

    //UnwindContext.Rip = (ULONG64)Function + CodeOffset;

    FunctionEntry = RtlLookupFunctionEntry(UnwindContext.Rip, &ImageBase, NULL);
    ok(FunctionEntry != NULL, "FunctionEntry is NULL\n");
    if (FunctionEntry == NULL)
    {
        return;
    }

    ExceptionRoutine = RtlVirtualUnwind(HandlerType,
                                        ImageBase,
                                        (ULONG64)Function + CodeOffset,
                                        FunctionEntry,
                                        &UnwindContext,
                                        &HandlerData,
                                        &EstablisherFrame,
                                        NULL);

    /* Check the expected EstablisherFrame */
    EstablisherFrameOffset = EstablisherFrame - CapturedRsp;
    ok_eq_hex64_str(TestName, EstablisherFrameOffset, ExpectedFrameOffset);

    /* Check if this was a function with a frame offset (i.e. alloca) */
    if (ExpectedFrameOffset != 0)
    {
        /* The establisher frame points to the static frame, which is 32 bytes
           (home size) before rbp */
        EstablisherFrameOffset = EstablisherFrame - CapturedRbp;
        ok_eq_hex64_str(TestName, EstablisherFrameOffset, -32);
    }

    if (ValidateContext)
    {
        //ok_eq_hex64_str(TestName, UnwindContext.Rip, ExpectedContext.Rip);
        ok_eq_hex64_str(TestName, UnwindContext.Rsp, ExpectedContext.Rsp);
        if (EstablisherFrameOffset == 0)
        {
            ok_eq_hex64_str(TestName, UnwindContext.Rbp, ExpectedContext.Rbp);
        }

        ok_eq_hex64_str(TestName, UnwindContext.Rbx, ExpectedContext.Rbx);
        ok_eq_hex64_str(TestName, UnwindContext.Rsi, ExpectedContext.Rsi);
        ok_eq_hex64_str(TestName, UnwindContext.Rdi, ExpectedContext.Rdi);
        ok_eq_hex64_str(TestName, UnwindContext.R10, ExpectedContext.R10); // volatile
        ok_eq_hex64_str(TestName, UnwindContext.R11, ExpectedContext.R11); // volatile
        ok_eq_hex64_str(TestName, UnwindContext.R12, ExpectedContext.R12);
        //ok_eq_hex64_str(TestName, UnwindContext.R13, ExpectedContext.R13);
       // ok_eq_hex64_str(TestName, UnwindContext.R14, ExpectedContext.R14);
        //ok_eq_hex64_str(TestName, UnwindContext.R15, ExpectedContext.R15);
    }
}

VOID UnwindStub1(_Out_ PCONTEXT UnwindContext);
VOID UnwindStub2(_Out_ PCONTEXT UnwindContext);
VOID UnwindStub3(_Out_ PCONTEXT UnwindContext);
VOID UnwindStub4(_Out_ PCONTEXT UnwindContext);

TEST_ENTRY g_Tests[] =
{
    { "Stub1_PrologEnd",  UnwindStub1, 0x04, UNW_FLAG_UHANDLER, 0,  TRUE },
    { "Stub1_Body",       UnwindStub1, 0x09, UNW_FLAG_UHANDLER, 0,  TRUE },
    { "Stub1_Epilog",     UnwindStub1, 0x0B, UNW_FLAG_UHANDLER, 0,  TRUE },
    { "Stub1_PostEpilog", UnwindStub1, 0x0F, UNW_FLAG_UHANDLER, 0,  TRUE },
    { "Stub2_Prolog1",    UnwindStub2, 0x05, UNW_FLAG_UHANDLER, 0,  FALSE },
    { "Stub2_PrologEnd",  UnwindStub2, 0x22, UNW_FLAG_UHANDLER, 0,  FALSE },
    { "Stub2_Body",       UnwindStub2, 0x27, UNW_FLAG_UHANDLER, 0,  TRUE },
    { "Stub2_Epilog",     UnwindStub2, 0x2A, UNW_FLAG_UHANDLER, 0,  TRUE },
    { "Stub2_PostEpilog", UnwindStub2, 0x33, UNW_FLAG_UHANDLER, 0,  FALSE },
    { "Stub3_Prolog1",    UnwindStub3, 0x05, UNW_FLAG_UHANDLER, 0,  FALSE },
    { "Stub3_PrologEnd",  UnwindStub3, 0x0A, UNW_FLAG_UHANDLER, 0,  FALSE },
    { "Stub3_Body1",      UnwindStub3, 0x0F, UNW_FLAG_UHANDLER, 16, TRUE },
    { "Stub3_Body2",      UnwindStub3, 0x21, UNW_FLAG_UHANDLER, 16, TRUE },
    { "Stub3_Epilog1",    UnwindStub3, 0x28, UNW_FLAG_UHANDLER, 16, TRUE },
    { "Stub3_Epilog2",    UnwindStub3, 0x2C, UNW_FLAG_UHANDLER, 16, FALSE },
    { "Stub3_PostEpilog", UnwindStub3, 0x30, UNW_FLAG_UHANDLER, 16, FALSE },
    { "Stub4_PrologEnd",  UnwindStub4, 0x04, UNW_FLAG_UHANDLER, 0,  TRUE },
    { "Stub4_Body",       UnwindStub4, 0x09, UNW_FLAG_UHANDLER, 0,  TRUE },
    { "Stub4_Epilog",     UnwindStub4, 0x0B, UNW_FLAG_UHANDLER, 0,  TRUE },
};



START_TEST(RtlVirtualUnwind)
{
    ULONG i;

    for (i = 0; i < ARRAYSIZE(g_Tests); i++)
    {
        Test_SingleFunction(g_Tests[i].TestName,
                            g_Tests[i].Function,
                            g_Tests[i].CodeOffset,
                            g_Tests[i].HandlerType,
                            g_Tests[i].ExpectedFrameOffset,
                            g_Tests[i].ValidateContext);
    }
}
