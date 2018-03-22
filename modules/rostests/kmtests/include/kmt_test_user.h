/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Kernel-Mode Test Suite test framework user declarations
 * COPYRIGHT:   Copyright 2011-2018 Thomas Faber <thomas.faber@reactos.org>
 */

#ifndef _KMTEST_TEST_USER_H_
#define _KMTEST_TEST_USER_H_

#if !defined _KMTEST_TEST_H_
#error include kmt_test.h instead of including kmt_test_user.h
#endif /* !defined _KMTEST_TEST_H_ */

static PKMT_RESULTBUFFER KmtAllocateResultBuffer(SIZE_T ResultBufferSize)
{
    PKMT_RESULTBUFFER Buffer = HeapAlloc(GetProcessHeap(), 0, ResultBufferSize);
    if (!Buffer)
        return NULL;

    Buffer->Successes = 0;
    Buffer->Failures = 0;
    Buffer->Skipped = 0;
    Buffer->LogBufferLength = 0;
    Buffer->LogBufferMaxLength = (ULONG)ResultBufferSize - FIELD_OFFSET(KMT_RESULTBUFFER, LogBuffer);

    return Buffer;
}

static VOID KmtFreeResultBuffer(PKMT_RESULTBUFFER Buffer)
{
    HeapFree(GetProcessHeap(), 0, Buffer);
}

#define KmtVSNPrintF vsnprintf

#endif /* !defined _KMTEST_TEST_USER_H_ */
