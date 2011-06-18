/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite test declarations
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#ifndef _KMTEST_TEST_H_
#define _KMTEST_TEST_H_

#include <stdarg.h>

typedef VOID KMT_TESTFUNC(VOID);

typedef struct
{
    const char *TestName;
    KMT_TESTFUNC *TestFunction;
} KMT_TEST, *PKMT_TEST;

typedef const KMT_TEST CKMT_TEST, *PCKMT_TEST;

extern const KMT_TEST TestList[];

typedef struct {
    volatile LONG Successes;
    volatile LONG Failures;
    volatile LONG LogBufferLength;
    LONG LogBufferMaxLength;
    CHAR LogBuffer[ANYSIZE_ARRAY];
} KMT_RESULTBUFFER, *PKMT_RESULTBUFFER;

extern PKMT_RESULTBUFFER ResultBuffer;

#if defined KMT_DEFINE_TEST_FUNCTIONS
PKMT_RESULTBUFFER ResultBuffer = NULL;

#if defined KMT_USER_MODE
static PKMT_RESULTBUFFER KmtAllocateResultBuffer(SIZE_T LogBufferMaxLength)
{
    PKMT_RESULTBUFFER Buffer = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(KMT_RESULTBUFFER, LogBuffer[LogBufferMaxLength]));

    Buffer->Successes = 0;
    Buffer->Failures = 0;
    Buffer->LogBufferLength = 0;
    Buffer->LogBufferMaxLength = LogBufferMaxLength;

    return Buffer;
}

static VOID KmtFreeResultBuffer(PKMT_RESULTBUFFER Buffer)
{
    HeapFree(GetProcessHeap(), 0, Buffer);
}
#endif /* defined KMT_USER_MODE */

#define KmtMemCpy memcpy

static VOID KmtAddToLogBuffer(PKMT_RESULTBUFFER Buffer, PCSTR String, SIZE_T Length)
{
    LONG OldLength;
    LONG NewLength;

    do
    {
        OldLength = Buffer->LogBufferLength;
        NewLength = OldLength + Length;
        if (NewLength > Buffer->LogBufferMaxLength)
        {
            /* TODO: indicate failure somehow */
            __debugbreak();
            return;
        }
    } while (InterlockedCompareExchange(&Buffer->LogBufferLength, NewLength, OldLength) != OldLength);

    KmtMemCpy(&Buffer->LogBuffer[OldLength], String, Length);
}

#ifdef KMT_KERNEL_MODE
INT __cdecl KmtVSNPrintF(PSTR Buffer, SIZE_T BufferMaxLength, PCSTR Format, va_list Arguments);
#elif defined KMT_USER_MODE
#define KmtVSNPrintF vsnprintf
#endif /* defined KMT_USER_MODE */

#endif /* defined KMT_DEFINE_TEST_FUNCTIONS */

#endif /* !defined _KMTEST_TEST_H_ */
