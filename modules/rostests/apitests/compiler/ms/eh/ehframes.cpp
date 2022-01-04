// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

/*
 * Exercise lots of different kinds of C++ EH frames.  Compile this with
 * every combination of opts you can to stress the C++ EH frame code in the
 * backend.
 */

#include <stdio.h>
#include <malloc.h>

#ifndef ALIGN
#define ALIGN 64
#endif

extern int TestFunc(int, ...);

int failures;

int global;
bool TestFuncThrows;

struct SmallObj
{
    virtual ~SmallObj()
    {
        TestFunc(1, this);
    };
    
    int x;
};

struct BigObj
{
    virtual ~BigObj()
    {
        TestFunc(1, this);
    };
    
    char x[4096];
};

int Simple(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    SmallObj f;
    return TestFunc(1, &f, &res, &arg);
}

int Try(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    SmallObj f;
    try {
        res = TestFunc(1, &f, &res, &arg);
    } catch (double) {
        res = TestFunc(2, &f, &res, &arg);
    }
    return res;
}

int GSCookie(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    char buf[16];
    SmallObj f;
    return TestFunc(1, buf, &f, &res, &arg);
}

int TryAndGSCookie(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    char buf[16];
    SmallObj f;
    try {
        res = TestFunc(1, &buf, &f, &res, &arg);
    } catch (double) {
        res = TestFunc(2, &buf, &f, &res, &arg);
    }
    return res;
}

int Align(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    __declspec(align(ALIGN)) double d[4];
    SmallObj f;
    return TestFunc(1, d, &f, &res, &arg);
}

int TryAndAlign(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    __declspec(align(ALIGN)) double d[4];
    SmallObj f;
    try {
        res = TestFunc(1, d, &f, &res, &arg);
    } catch (double) {
        res = TestFunc(2, d, &f, &res, &arg);
    }
    return res;
}

int GSCookieAndAlign(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    char buf[16];
    __declspec(align(ALIGN)) double d[4];
    SmallObj f;
    return TestFunc(1, buf, d, &f, &res, &arg);
}

int TryAndGSCookieAndAlign(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    char buf[16];
    __declspec(align(ALIGN)) double d[4];
    SmallObj f;
    try {
        res = TestFunc(1, buf, d, &f, &res, &arg);
    } catch (double) {
        res = TestFunc(2, buf, d, &f, &res, &arg);
    }
    return res;
}

int Alloca(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    SmallObj f;
    return TestFunc(1, _alloca(global), &f, &res, &arg);
}

int TryAndAlloca(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    SmallObj f;
    try {
        res = TestFunc(1, _alloca(global), &f, &res, &arg);
    } catch (double) {
        res = TestFunc(2, &f, &res, &arg);
    }
    return res;
}

int GSCookieAndAlloca(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    char buf[16];
    SmallObj f;
    return TestFunc(1, buf, _alloca(global), &f, &res, &arg);
}

int TryAndGSCookieAndAlloca(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    char buf[16];
    SmallObj f;
    try {
        res = TestFunc(1, &buf, _alloca(global), &f, &res, &arg);
    } catch (double) {
        res = TestFunc(2, &buf, &f, &res, &arg);
    }
    return res;
}

int AlignAndAlloca(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    __declspec(align(ALIGN)) double d[4];
    SmallObj f;
    return TestFunc(1, d, _alloca(global), &f, &res, &arg);
}

int TryAndAlignAndAlloca(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    __declspec(align(ALIGN)) double d[4];
    SmallObj f;
    try {
        res = TestFunc(1, d, _alloca(global), &f, &res, &arg);
    } catch (double) {
        res = TestFunc(2, d, &f, &res, &arg);
    }
    return res;
}

int GSCookieAndAlignAndAlloca(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    char buf[16];
    __declspec(align(ALIGN)) double d[4];
    SmallObj f;
    return TestFunc(1, buf, d, _alloca(global), &f, &res, &arg);
}

int TryAndGSCookieAndAlignAndAlloca(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    char buf[16];
    __declspec(align(ALIGN)) double d[4];
    SmallObj f;
    try {
        res = TestFunc(1, buf, d, _alloca(global), &f, &res, &arg);
    } catch (double) {
        res = TestFunc(2, buf, d, &f, &res, &arg);
    }
    return res;
}

/* The *AndBigLocals set of functions try to trigger EBP adjustment */

int BigLocals(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    BigObj f1;
    return TestFunc(1, &f1, &res, &res, &res, &res, &res, &arg);
}

int TryAndBigLocals(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    BigObj f1;
    try {
        res = TestFunc(1, &f1, &res, &res, &res, &res, &res, &arg);
    } catch (double) {
        res = TestFunc(2, &f1, &res, &res, &res, &res, &res, &arg);
    }
    return res;
}

int GSCookieAndBigLocals(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    char buf[16];
    BigObj f1;
    return TestFunc(1, buf, &f1, &res, &res, &res, &res, &res, &arg);
}

int TryAndGSCookieAndBigLocals(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    char buf[16];
    BigObj f1;
    try {
        res = TestFunc(1, &buf, &f1, &res, &res, &res, &res, &res, &arg);
    } catch (double) {
        res = TestFunc(2, &buf, &f1, &res, &res, &res, &res, &res, &arg);
    }
    return res;
}

int AlignAndBigLocals(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    __declspec(align(ALIGN)) double d[4];
    BigObj f1;
    return TestFunc(1, d, &f1, &res, &res, &res, &res, &res, &arg);
}

int TryAndAlignAndBigLocals(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    __declspec(align(ALIGN)) double d[4];
    BigObj f1;
    try {
        res = TestFunc(1, d, &f1, &res, &res, &res, &res, &res, &arg);
    } catch (double) {
        res = TestFunc(2, d, &f1, &res, &res, &res, &res, &res, &arg);
    }
    return res;
}

int GSCookieAndAlignAndBigLocals(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    char buf[16];
    __declspec(align(ALIGN)) double d[4];
    BigObj f1;
    return TestFunc(1, buf, d, &f1, &res, &res, &res, &res, &res, &arg);
}

int TryAndGSCookieAndAlignAndBigLocals(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    char buf[16];
    __declspec(align(ALIGN)) double d[4];
    BigObj f1;
    try {
        res = TestFunc(1, buf, d, &f1, &res, &res, &res, &res, &res, &arg);
    } catch (double) {
        res = TestFunc(2, buf, d, &f1, &res, &res, &res, &res, &res, &arg);
    }
    return res;
}

int AllocaAndBigLocals(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    BigObj f1;
    return TestFunc(1, _alloca(global), &f1, &res, &res, &res, &res, &res, &arg);
}

int TryAndAllocaAndBigLocals(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    BigObj f1;
    try {
        res = TestFunc(1, _alloca(global), &f1, &res, &res, &res, &res, &res, &arg);
    } catch (double) {
        res = TestFunc(2, &f1, &res, &res, &res, &res, &res, &arg);
    }
    return res;
}

int GSCookieAndAllocaAndBigLocals(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    char buf[16];
    BigObj f1;
    return TestFunc(1, buf, _alloca(global), &f1, &res, &res, &res, &res, &res, &arg);
}

int TryAndGSCookieAndAllocaAndBigLocals(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    char buf[16];
    BigObj f1;
    try {
        res = TestFunc(1, &buf, _alloca(global), &f1, &res, &res, &res, &res, &res, &arg);
    } catch (double) {
        res = TestFunc(2, &buf, &f1, &res, &res, &res, &res, &res, &arg);
    }
    return res;
}

int AlignAndAllocaAndBigLocals(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    __declspec(align(ALIGN)) double d[4];
    BigObj f1;
    return TestFunc(1, d, _alloca(global), &f1, &res, &res, &res, &res, &res, &arg);
}

int TryAndAlignAndAllocaAndBigLocals(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    __declspec(align(ALIGN)) double d[4];
    BigObj f1;
    try {
        res = TestFunc(1, d, _alloca(global), &f1, &res, &res, &res, &res, &res, &arg);
    } catch (double) {
        res = TestFunc(2, d, &f1, &res, &res, &res, &res, &res, &arg);
    }
    return res;
}

int GSCookieAndAlignAndAllocaAndBigLocals(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    char buf[16];
    __declspec(align(ALIGN)) double d[4];
    BigObj f1;
    return TestFunc(1, buf, d, _alloca(global), &f1, &res, &res, &res, &res, &res, &arg);
}

int TryAndGSCookieAndAlignAndAllocaAndBigLocals(int arg)
{
    puts(__FUNCTION__);
    int res = 0;
    char buf[16];
    __declspec(align(ALIGN)) double d[4];
    BigObj f1;
    try {
        res = TestFunc(1, buf, d, _alloca(global), &f1, &res, &res, &res, &res, &res, &arg);
    } catch (double) {
        res = TestFunc(2, buf, d, &f1, &res, &res, &res, &res, &res, &arg);
    }
    return res;
}

__declspec(noinline)
int TestFunc(int, ...)
{
    if (TestFuncThrows)
    {
        TestFuncThrows = false;
        throw 123;
    }

    return global;
}

void RunTests()
{
    puts("Test pass 1 - no throws");

    try
    {
        Simple(1);
        Try(1);
        GSCookie(1);
        TryAndGSCookie(1);
        Align(1);
        TryAndAlign(1);
        GSCookieAndAlign(1);
        TryAndGSCookieAndAlign(1);
        Alloca(1);
        TryAndAlloca(1);
        GSCookieAndAlloca(1);
        TryAndGSCookieAndAlloca(1);
        AlignAndAlloca(1);
        TryAndAlignAndAlloca(1);
        GSCookieAndAlignAndAlloca(1);
        TryAndGSCookieAndAlignAndAlloca(1);
        BigLocals(1);
        TryAndBigLocals(1);
        GSCookieAndBigLocals(1);
        TryAndGSCookieAndBigLocals(1);
        AlignAndBigLocals(1);
        TryAndAlignAndBigLocals(1);
        GSCookieAndAlignAndBigLocals(1);
        TryAndGSCookieAndAlignAndBigLocals(1);
        AllocaAndBigLocals(1);
        TryAndAllocaAndBigLocals(1);
        GSCookieAndAllocaAndBigLocals(1);
        TryAndGSCookieAndAllocaAndBigLocals(1);
        AlignAndAllocaAndBigLocals(1);
        TryAndAlignAndAllocaAndBigLocals(1);
        GSCookieAndAlignAndAllocaAndBigLocals(1);
        TryAndGSCookieAndAlignAndAllocaAndBigLocals(1);
    }
    catch (...)
    {
        puts("ERROR - throw not expected");
        ++failures;
    }

    puts("Test pass 2 - throws");

    for (int i = 0; i < 32; ++i)
    {
        TestFuncThrows = true;
        bool caught = false;
        try
        {
            switch (i)
            {
            case 0: Simple(1); break;
            case 1: Try(1); break;
            case 2: GSCookie(1); break;
            case 3: TryAndGSCookie(1); break;
            case 4: Align(1); break;
            case 5: TryAndAlign(1); break;
            case 6: GSCookieAndAlign(1); break;
            case 7: TryAndGSCookieAndAlign(1); break;
            case 8: Alloca(1); break;
            case 9: TryAndAlloca(1); break;
            case 10: GSCookieAndAlloca(1); break;
            case 11: TryAndGSCookieAndAlloca(1); break;
            case 12: AlignAndAlloca(1); break;
            case 13: TryAndAlignAndAlloca(1); break;
            case 14: GSCookieAndAlignAndAlloca(1); break;
            case 15: TryAndGSCookieAndAlignAndAlloca(1); break;
            case 16: BigLocals(1); break;
            case 17: TryAndBigLocals(1); break;
            case 18: GSCookieAndBigLocals(1); break;
            case 19: TryAndGSCookieAndBigLocals(1); break;
            case 20: AlignAndBigLocals(1); break;
            case 21: TryAndAlignAndBigLocals(1); break;
            case 22: GSCookieAndAlignAndBigLocals(1); break;
            case 23: TryAndGSCookieAndAlignAndBigLocals(1); break;
            case 24: AllocaAndBigLocals(1); break;
            case 25: TryAndAllocaAndBigLocals(1); break;
            case 26: GSCookieAndAllocaAndBigLocals(1); break;
            case 27: TryAndGSCookieAndAllocaAndBigLocals(1); break;
            case 28: AlignAndAllocaAndBigLocals(1); break;
            case 29: TryAndAlignAndAllocaAndBigLocals(1); break;
            case 30: GSCookieAndAlignAndAllocaAndBigLocals(1); break;
            case 31: TryAndGSCookieAndAlignAndAllocaAndBigLocals(1); break;
            }
        }
        catch (int)
        {
            caught = true;
        }

        if (!caught)
        {
            puts("ERROR - did not catch expected thrown object");
            ++failures;
        }
    }
}

int main()
{
    __try
    {
        RunTests();
    }
    __except (1)
    {
        puts("ERROR - Unexpectedly caught an exception");
        ++failures;
    }

    if (failures)
    {
        printf("Test failed with %d failure%s\n",
               failures, failures == 1 ? "" : "s");
    }
    else
    {
        puts("Test passed");
    }

    return failures;
}
