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
#include <stdarg.h>
#include <memory.h>

#ifndef ALIGN
#define ALIGN 64
#endif

#define ARG(x) &x, sizeof(x)
#define ARG2(x) ARG(x), ARG(x)
#define ARG5(x) ARG(x), ARG(x), ARG(x), ARG(x), ARG(x)

extern int TestFunc(int, ...);

int failures;

int one = 1;
int zero = 0;

size_t global = 16;
volatile bool TestFuncThrows;

struct SmallObj {
  int x;
};

struct BigObj {
  char x[1024];
};

int Simple(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  SmallObj f;
  __try { TestFunc(1, ARG(f), ARG(res), ARG(arg), NULL); }
  __finally { res = TestFunc(1, ARG(f), ARG(res), ARG(arg), NULL); }
  return res;
}

int Try(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  SmallObj f;
  __try { res = TestFunc(1, ARG(f), ARG(res), ARG(arg), NULL); }
  __except(TestFunc(2, ARG(f), ARG(res), ARG(arg), NULL), zero) {
    res = TestFunc(2, ARG(f), ARG(res), ARG(arg), NULL);
  }
  return res;
}

int GSCookie(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  char buf[16];
  SmallObj f;
  __try { TestFunc(1, ARG(buf), ARG(f), ARG(res), ARG(arg), NULL); }
  __finally { res = TestFunc(1, ARG(buf), ARG(f), ARG(res), ARG(arg), NULL); }
  return res;
}

int TryAndGSCookie(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  char buf[16];
  SmallObj f;
  __try { res = TestFunc(1, ARG(buf), ARG(f), ARG(res), ARG(arg), NULL); }
  __except(TestFunc(2, ARG(buf), ARG(f), ARG(res), ARG(arg), NULL), zero) {
    res = TestFunc(2, ARG(buf), ARG(f), ARG(res), ARG(arg), NULL);
  }
  return res;
}

int Align(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  __declspec(align(ALIGN)) double d[4];
  SmallObj f;
  __try { TestFunc(1, ARG(d), ARG(f), ARG(res), ARG(arg), NULL); }
  __finally { res = TestFunc(1, ARG(d), ARG(f), ARG(res), ARG(arg), NULL); }
  return res;
}

int TryAndAlign(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  __declspec(align(ALIGN)) double d[4];
  SmallObj f;
  __try { res = TestFunc(1, ARG(d), ARG(f), ARG(res), ARG(arg), NULL); }
  __except(TestFunc(2, ARG(d), ARG(f), ARG(res), ARG(arg), NULL), zero) {
    res = TestFunc(2, ARG(d), ARG(f), ARG(res), ARG(arg), NULL);
  }
  return res;
}

int GSCookieAndAlign(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  char buf[16];
  __declspec(align(ALIGN)) double d[4];
  SmallObj f;
  __try { TestFunc(1, ARG(buf), ARG(d), ARG(f), ARG(res), ARG(arg), NULL); }
  __finally {
    res = TestFunc(1, ARG(buf), ARG(d), ARG(f), ARG(res), ARG(arg), NULL);
  }
  return res;
}

int TryAndGSCookieAndAlign(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  char buf[16];
  __declspec(align(ALIGN)) double d[4];
  SmallObj f;
  __try {
    res = TestFunc(1, ARG(buf), ARG(d), ARG(f), ARG(res), ARG(arg), NULL);
  }
  __except(TestFunc(2, ARG(buf), ARG(d), ARG(f), ARG(res), ARG(arg), NULL),
           zero) {
    res = TestFunc(2, ARG(buf), ARG(d), ARG(f), ARG(res), ARG(arg), NULL);
  }
  return res;
}

int Alloca(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  SmallObj f;
  __try {
    TestFunc(1, _alloca(global), global, ARG(f), ARG(res), ARG(arg), NULL);
  }
  __finally { res = TestFunc(1, ARG(f), ARG(res), ARG(arg), NULL); }
  return res;
}

int TryAndAlloca(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  SmallObj f;
  __try {
    res =
        TestFunc(1, _alloca(global), global, ARG(f), ARG(res), ARG(arg), NULL);
  }
  __except(TestFunc(2, ARG(f), ARG(res), ARG(arg), NULL), zero) {
    res = TestFunc(2, ARG(f), ARG(res), ARG(arg), NULL);
  }
  return res;
}

int GSCookieAndAlloca(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  char buf[16];
  SmallObj f;
  __try {
    TestFunc(1, ARG(buf), _alloca(global), global, ARG(f), ARG(res), ARG(arg),
             NULL);
  }
  __finally { res = TestFunc(1, ARG(buf), ARG(f), ARG(res), ARG(arg), NULL); }
  return res;
}

int TryAndGSCookieAndAlloca(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  char buf[16];
  SmallObj f;
  __try {
    res = TestFunc(1, ARG(buf), _alloca(global), global, ARG(f), ARG(res),
                   ARG(arg), NULL);
  }
  __except(TestFunc(2, ARG(buf), ARG(f), ARG(res), ARG(arg), NULL), zero) {
    res = TestFunc(2, ARG(buf), ARG(f), ARG(res), ARG(arg), NULL);
  }
  return res;
}

int AlignAndAlloca(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  __declspec(align(ALIGN)) double d[4];
  SmallObj f;
  __try {
    TestFunc(1, ARG(d), _alloca(global), global, ARG(f), ARG(res), ARG(arg),
             NULL);
  }
  __finally { res = TestFunc(1, ARG(d), ARG(f), ARG(res), ARG(arg), NULL); }
  return res;
}

int TryAndAlignAndAlloca(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  __declspec(align(ALIGN)) double d[4];
  SmallObj f;
  __try {
    res = TestFunc(1, ARG(d), _alloca(global), global, ARG(f), ARG(res),
                   ARG(arg), NULL);
  }
  __except(TestFunc(2, ARG(d), ARG(f), ARG(res), ARG(arg), NULL), zero) {
    res = TestFunc(2, ARG(d), ARG(f), ARG(res), ARG(arg), NULL);
  }
  return res;
}

int GSCookieAndAlignAndAlloca(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  char buf[16];
  __declspec(align(ALIGN)) double d[4];
  SmallObj f;
  __try {
    TestFunc(1, ARG(buf), ARG(d), _alloca(global), global, ARG(f), ARG(res),
             ARG(arg), NULL);
  }
  __finally {
    res = TestFunc(1, ARG(buf), ARG(d), ARG(f), ARG(res), ARG(arg), NULL);
  }
  return res;
}

int TryAndGSCookieAndAlignAndAlloca(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  char buf[16];
  __declspec(align(ALIGN)) double d[4];
  SmallObj f;
  __try {
    res = TestFunc(1, ARG(buf), ARG(d), _alloca(global), global, ARG(f),
                   ARG(res), ARG(arg), NULL);
  }
  __except(TestFunc(2, ARG(buf), ARG(d), ARG(f), ARG(res), ARG(arg), NULL),
           zero) {
    res = TestFunc(2, ARG(buf), ARG(d), ARG(f), ARG(res), ARG(arg), NULL);
  }
  return res;
}

/*
 * The BigLocals variants try to trigger EBP adjustment, and generally do in
 * the /O1 case for the non-aligned stacks.
 */

int BigLocals(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  BigObj f1;
  __try { TestFunc(1, ARG(f1), ARG5(res), ARG(arg), NULL); }
  __finally { res = TestFunc(1, ARG(f1), ARG5(res), ARG(arg), NULL); }
  return res;
}

int TryAndBigLocals(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  BigObj f1;
  __try { res = TestFunc(1, ARG(f1), ARG5(res), ARG(arg), NULL); }
  __except(TestFunc(2, ARG(f1), ARG5(res), ARG(arg), NULL), zero) {
    res = TestFunc(2, ARG(f1), ARG5(res), ARG(arg), NULL);
  }
  return res;
}

int GSCookieAndBigLocals(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  char buf[16];
  BigObj f1;
  __try { TestFunc(1, ARG(buf), ARG(f1), ARG5(res), ARG(arg), NULL); }
  __finally { res = TestFunc(1, ARG(buf), ARG(f1), ARG5(res), ARG(arg), NULL); }
  return res;
}

int TryAndGSCookieAndBigLocals(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  char buf[16];
  BigObj f1;
  __try { res = TestFunc(1, ARG(buf), ARG(f1), ARG5(res), ARG(arg), NULL); }
  __except(TestFunc(2, ARG(buf), ARG(f1), ARG5(res), ARG(arg), NULL), zero) {
    res = TestFunc(2, ARG(buf), ARG(f1), ARG5(res), ARG(arg), NULL);
  }
  return res;
}

int AlignAndBigLocals(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  __declspec(align(ALIGN)) double d[4];
  BigObj f1;
  __try { TestFunc(1, ARG(d), ARG(f1), ARG5(res), ARG(arg), NULL); }
  __finally { res = TestFunc(1, ARG(d), ARG(f1), ARG5(res), ARG(arg), NULL); }
  return res;
}

int TryAndAlignAndBigLocals(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  __declspec(align(ALIGN)) double d[4];
  BigObj f1;
  __try { res = TestFunc(1, ARG(d), ARG(f1), ARG5(res), ARG(arg), NULL); }
  __except(TestFunc(2, ARG(d), ARG(f1), ARG5(res), ARG(arg), NULL), zero) {
    res = TestFunc(2, ARG(d), ARG(f1), ARG5(res), ARG(arg), NULL);
  }
  return res;
}

int GSCookieAndAlignAndBigLocals(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  char buf[16];
  __declspec(align(ALIGN)) double d[4];
  BigObj f1;
  __try { TestFunc(1, ARG(buf), ARG(d), ARG(f1), ARG5(res), ARG(arg), NULL); }
  __finally {
    res = TestFunc(1, ARG(buf), ARG(d), ARG(f1), ARG5(res), ARG(arg), NULL);
  }
  return res;
}

int TryAndGSCookieAndAlignAndBigLocals(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  char buf[16];
  __declspec(align(ALIGN)) double d[4];
  BigObj f1;
  __try {
    res = TestFunc(1, ARG(buf), ARG(d), ARG(f1), ARG5(res), ARG(arg), NULL);
  }
  __except(TestFunc(2, ARG(buf), ARG(d), ARG(f1), ARG5(res), ARG(arg), NULL),
           zero) {
    res = TestFunc(2, ARG(buf), ARG(d), ARG(f1), ARG5(res), ARG(arg), NULL);
  }
  return res;
}

int AllocaAndBigLocals(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  BigObj f1;
  __try {
    TestFunc(1, _alloca(global), global, ARG(f1), ARG5(res), ARG(arg), NULL);
  }
  __finally { res = TestFunc(1, ARG(f1), ARG5(res), ARG(arg), NULL); }
  return res;
}

int TryAndAllocaAndBigLocals(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  BigObj f1;
  __try {
    res = TestFunc(1, _alloca(global), global, ARG(f1), ARG5(res), ARG(arg),
                   NULL);
  }
  __except(TestFunc(2, ARG(f1), ARG5(res), ARG(arg), NULL), zero) {
    res = TestFunc(2, ARG(f1), ARG5(res), ARG(arg), NULL);
  }
  return res;
}

int GSCookieAndAllocaAndBigLocals(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  char buf[16];
  BigObj f1;
  __try {
    TestFunc(1, ARG(buf), _alloca(global), global, ARG(f1), ARG5(res), ARG(arg),
             NULL);
  }
  __finally { res = TestFunc(1, ARG(buf), ARG(f1), ARG5(res), ARG(arg), NULL); }
  return res;
}

int TryAndGSCookieAndAllocaAndBigLocals(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  char buf[16];
  BigObj f1;
  __try {
    res = TestFunc(1, ARG(buf), _alloca(global), global, ARG(f1), ARG5(res),
                   ARG(arg), NULL);
  }
  __except(TestFunc(2, ARG(buf), ARG(f1), ARG5(res), ARG(arg), NULL), zero) {
    res = TestFunc(2, ARG(buf), ARG(f1), ARG5(res), ARG(arg), NULL);
  }
  return res;
}

int AlignAndAllocaAndBigLocals(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  __declspec(align(ALIGN)) double d[4];
  BigObj f1;
  __try {
    TestFunc(1, ARG(d), _alloca(global), global, ARG(f1), ARG5(res), ARG(arg),
             NULL);
  }
  __finally { res = TestFunc(1, ARG(d), ARG(f1), ARG5(res), ARG(arg), NULL); }
  return res;
}

int TryAndAlignAndAllocaAndBigLocals(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  __declspec(align(ALIGN)) double d[4];
  BigObj f1;
  __try {
    res = TestFunc(1, ARG(d), _alloca(global), global, ARG(f1), ARG5(res),
                   ARG(arg), NULL);
  }
  __except(TestFunc(2, ARG(d), ARG(f1), ARG5(res), ARG(arg), NULL), zero) {
    res = TestFunc(2, ARG(d), ARG(f1), ARG5(res), ARG(arg), NULL);
  }
  return res;
}

int GSCookieAndAlignAndAllocaAndBigLocals(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  char buf[16];
  __declspec(align(ALIGN)) double d[4];
  BigObj f1;
  __try {
    TestFunc(1, ARG(buf), ARG(d), _alloca(global), global, ARG(f1), ARG5(res),
             ARG(arg), NULL);
  }
  __finally {
    res = TestFunc(1, ARG(buf), ARG(d), ARG(f1), ARG5(res), ARG(arg), NULL);
  }
  return res;
}

int TryAndGSCookieAndAlignAndAllocaAndBigLocals(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  char buf[16];
  __declspec(align(ALIGN)) double d[4];
  BigObj f1;
  __try {
    res = TestFunc(1, ARG(buf), ARG(d), _alloca(global), global, ARG(f1),
                   ARG5(res), ARG(arg), NULL);
  }
  __except(TestFunc(2, ARG(buf), ARG(d), ARG(f1), ARG5(res), ARG(arg), NULL),
           zero) {
    res = TestFunc(2, ARG(buf), ARG(d), ARG(f1), ARG5(res), ARG(arg), NULL);
  }
  return res;
}

/*
 * The EbpAdj variants try to trigger EBP adjustment, and generally do in
 * the /O1 case for the non-aligned stacks.  They add a non-GS-protected
 * buffer so the EH node is far from both sides of the local variable
 * allocation.  Doesn't seem to add any testing over what the BigLocals cases
 * already do.
 */

int EbpAdj(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  int a[512];
  BigObj f1;
  __try { TestFunc(1, ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL); }
  __finally { res = TestFunc(1, ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL); }
  return res;
}

int TryAndEbpAdj(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  int a[512];
  BigObj f1;
  __try { res = TestFunc(1, ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL); }
  __except(TestFunc(2, ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL), zero) {
    res = TestFunc(2, ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL);
  }
  return res;
}

int GSCookieAndEbpAdj(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  int a[512];
  char buf[16];
  BigObj f1;
  __try { TestFunc(1, ARG(buf), ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL); }
  __finally {
    res = TestFunc(1, ARG(buf), ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL);
  }
  return res;
}

int TryAndGSCookieAndEbpAdj(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  int a[512];
  char buf[16];
  BigObj f1;
  __try {
    res = TestFunc(1, ARG(buf), ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL);
  }
  __except(TestFunc(2, ARG(buf), ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL),
           zero) {
    res = TestFunc(2, ARG(buf), ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL);
  }
  return res;
}

int AlignAndEbpAdj(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  int a[512];
  __declspec(align(ALIGN)) double d[4];
  BigObj f1;
  __try { TestFunc(1, ARG(d), ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL); }
  __finally {
    res = TestFunc(1, ARG(d), ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL);
  }
  return res;
}

int TryAndAlignAndEbpAdj(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  int a[512];
  __declspec(align(ALIGN)) double d[4];
  BigObj f1;
  __try {
    res = TestFunc(1, ARG(d), ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL);
  }
  __except(TestFunc(2, ARG(d), ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL),
           zero) {
    res = TestFunc(2, ARG(d), ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL);
  }
  return res;
}

int GSCookieAndAlignAndEbpAdj(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  int a[512];
  char buf[16];
  __declspec(align(ALIGN)) double d[4];
  BigObj f1;
  __try {
    TestFunc(1, ARG(buf), ARG(d), ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL);
  }
  __finally {
    res = TestFunc(1, ARG(buf), ARG(d), ARG(f1), ARG2(a), ARG5(res), ARG(arg),
                   NULL);
  }
  return res;
}

int TryAndGSCookieAndAlignAndEbpAdj(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  int a[512];
  char buf[16];
  __declspec(align(ALIGN)) double d[4];
  BigObj f1;
  __try {
    res = TestFunc(1, ARG(buf), ARG(d), ARG(f1), ARG2(a), ARG5(res), ARG(arg),
                   NULL);
  }
  __except(TestFunc(2, ARG(buf), ARG(d), ARG(f1), ARG2(a), ARG5(res), ARG(arg),
                    NULL),
           zero) {
    res = TestFunc(2, ARG(buf), ARG(d), ARG(f1), ARG2(a), ARG5(res), ARG(arg),
                   NULL);
  }
  return res;
}

int AllocaAndEbpAdj(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  int a[512];
  BigObj f1;
  __try {
    TestFunc(1, _alloca(global), global, ARG(f1), ARG2(a), ARG5(res), ARG(arg),
             NULL);
  }
  __finally { res = TestFunc(1, ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL); }
  return res;
}

int TryAndAllocaAndEbpAdj(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  int a[512];
  BigObj f1;
  __try {
    res = TestFunc(1, _alloca(global), global, ARG(f1), ARG2(a), ARG5(res),
                   ARG(arg), NULL);
  }
  __except(TestFunc(2, ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL), zero) {
    res = TestFunc(2, ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL);
  }
  return res;
}

int GSCookieAndAllocaAndEbpAdj(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  int a[512];
  char buf[16];
  BigObj f1;
  __try {
    TestFunc(1, ARG(buf), _alloca(global), global, ARG(f1), ARG2(a), ARG5(res),
             ARG(arg), NULL);
  }
  __finally {
    res = TestFunc(1, ARG(buf), ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL);
  }
  return res;
}

int TryAndGSCookieAndAllocaAndEbpAdj(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  int a[512];
  char buf[16];
  BigObj f1;
  __try {
    res = TestFunc(1, ARG(buf), _alloca(global), global, ARG(f1), ARG2(a),
                   ARG5(res), ARG(arg), NULL);
  }
  __except(TestFunc(2, ARG(buf), ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL),
           zero) {
    res = TestFunc(2, ARG(buf), ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL);
  }
  return res;
}

int AlignAndAllocaAndEbpAdj(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  int a[512];
  __declspec(align(ALIGN)) double d[4];
  BigObj f1;
  __try {
    TestFunc(1, ARG(d), _alloca(global), global, ARG(f1), ARG2(a), ARG5(res),
             ARG(arg), NULL);
  }
  __finally {
    res = TestFunc(1, ARG(d), ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL);
  }
  return res;
}

int TryAndAlignAndAllocaAndEbpAdj(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  int a[512];
  __declspec(align(ALIGN)) double d[4];
  BigObj f1;
  __try {
    res = TestFunc(1, ARG(d), _alloca(global), global, ARG(f1), ARG2(a),
                   ARG5(res), ARG(arg), NULL);
  }
  __except(TestFunc(2, ARG(d), ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL),
           zero) {
    res = TestFunc(2, ARG(d), ARG(f1), ARG2(a), ARG5(res), ARG(arg), NULL);
  }
  return res;
}

int GSCookieAndAlignAndAllocaAndEbpAdj(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  int a[512];
  char buf[16];
  __declspec(align(ALIGN)) double d[4];
  BigObj f1;
  __try {
    TestFunc(1, ARG(buf), ARG(d), _alloca(global), global, ARG(f1), ARG2(a),
             ARG5(res), ARG(arg), NULL);
  }
  __finally {
    res = TestFunc(1, ARG(buf), ARG(d), ARG(f1), ARG2(a), ARG5(res), ARG(arg),
                   NULL);
  }
  return res;
}

int TryAndGSCookieAndAlignAndAllocaAndEbpAdj(int arg) {
  puts(__FUNCTION__);
  int res = 0;
  int a[512];
  char buf[16];
  __declspec(align(ALIGN)) double d[4];
  BigObj f1;
  __try {
    res = TestFunc(1, ARG(buf), ARG(d), _alloca(global), global, ARG(f1),
                   ARG2(a), ARG5(res), ARG(arg), NULL);
  }
  __except(TestFunc(2, ARG(buf), ARG(d), ARG(f1), ARG2(a), ARG5(res), ARG(arg),
                    NULL),
           zero) {
    res = TestFunc(2, ARG(buf), ARG(d), ARG(f1), ARG2(a), ARG5(res), ARG(arg),
                   NULL);
  }
  return res;
}

__declspec(noinline) int TestFunc(int x, ...) {
  va_list ap;
  va_start(ap, x);

  for (;;) {
    void *pbuf = va_arg(ap, void *);
    if (pbuf == NULL) {
      break;
    }
    size_t len = va_arg(ap, size_t);
    memset(pbuf, 0, len);
  }

  if (TestFuncThrows) {
    TestFuncThrows = false;
    *(volatile int *)0;
  }

  return static_cast<int>(global);
}

void RunTests() {
  puts("Test pass 1 - no exceptions");

  __try {
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
    EbpAdj(1);
    TryAndEbpAdj(1);
    GSCookieAndEbpAdj(1);
    TryAndGSCookieAndEbpAdj(1);
    AlignAndEbpAdj(1);
    TryAndAlignAndEbpAdj(1);
    GSCookieAndAlignAndEbpAdj(1);
    TryAndGSCookieAndAlignAndEbpAdj(1);
    AllocaAndEbpAdj(1);
    TryAndAllocaAndEbpAdj(1);
    GSCookieAndAllocaAndEbpAdj(1);
    TryAndGSCookieAndAllocaAndEbpAdj(1);
    AlignAndAllocaAndEbpAdj(1);
    TryAndAlignAndAllocaAndEbpAdj(1);
    GSCookieAndAlignAndAllocaAndEbpAdj(1);
    TryAndGSCookieAndAlignAndAllocaAndEbpAdj(1);
  }
  __except(one) {
    puts("ERROR - exception not expected");
    ++failures;
  }

  puts("Test pass 2 - exceptions");

  for (int i = 0; i < 48; ++i) {
    TestFuncThrows = true;
    bool caught = false;
    __try {
      switch (i) {
      case 0:
        Simple(1);
        break;
      case 1:
        Try(1);
        break;
      case 2:
        GSCookie(1);
        break;
      case 3:
        TryAndGSCookie(1);
        break;
      case 4:
        Align(1);
        break;
      case 5:
        TryAndAlign(1);
        break;
      case 6:
        GSCookieAndAlign(1);
        break;
      case 7:
        TryAndGSCookieAndAlign(1);
        break;
      case 8:
        Alloca(1);
        break;
      case 9:
        TryAndAlloca(1);
        break;
      case 10:
        GSCookieAndAlloca(1);
        break;
      case 11:
        TryAndGSCookieAndAlloca(1);
        break;
      case 12:
        AlignAndAlloca(1);
        break;
      case 13:
        TryAndAlignAndAlloca(1);
        break;
      case 14:
        GSCookieAndAlignAndAlloca(1);
        break;
      case 15:
        TryAndGSCookieAndAlignAndAlloca(1);
        break;
      case 16:
        BigLocals(1);
        break;
      case 17:
        TryAndBigLocals(1);
        break;
      case 18:
        GSCookieAndBigLocals(1);
        break;
      case 19:
        TryAndGSCookieAndBigLocals(1);
        break;
      case 20:
        AlignAndBigLocals(1);
        break;
      case 21:
        TryAndAlignAndBigLocals(1);
        break;
      case 22:
        GSCookieAndAlignAndBigLocals(1);
        break;
      case 23:
        TryAndGSCookieAndAlignAndBigLocals(1);
        break;
      case 24:
        AllocaAndBigLocals(1);
        break;
      case 25:
        TryAndAllocaAndBigLocals(1);
        break;
      case 26:
        GSCookieAndAllocaAndBigLocals(1);
        break;
      case 27:
        TryAndGSCookieAndAllocaAndBigLocals(1);
        break;
      case 28:
        AlignAndAllocaAndBigLocals(1);
        break;
      case 29:
        TryAndAlignAndAllocaAndBigLocals(1);
        break;
      case 30:
        GSCookieAndAlignAndAllocaAndBigLocals(1);
        break;
      case 31:
        TryAndGSCookieAndAlignAndAllocaAndBigLocals(1);
        break;
      case 32:
        EbpAdj(1);
        break;
      case 33:
        TryAndEbpAdj(1);
        break;
      case 34:
        GSCookieAndEbpAdj(1);
        break;
      case 35:
        TryAndGSCookieAndEbpAdj(1);
        break;
      case 36:
        AlignAndEbpAdj(1);
        break;
      case 37:
        TryAndAlignAndEbpAdj(1);
        break;
      case 38:
        GSCookieAndAlignAndEbpAdj(1);
        break;
      case 39:
        TryAndGSCookieAndAlignAndEbpAdj(1);
        break;
      case 40:
        AllocaAndEbpAdj(1);
        break;
      case 41:
        TryAndAllocaAndEbpAdj(1);
        break;
      case 42:
        GSCookieAndAllocaAndEbpAdj(1);
        break;
      case 43:
        TryAndGSCookieAndAllocaAndEbpAdj(1);
        break;
      case 44:
        AlignAndAllocaAndEbpAdj(1);
        break;
      case 45:
        TryAndAlignAndAllocaAndEbpAdj(1);
        break;
      case 46:
        GSCookieAndAlignAndAllocaAndEbpAdj(1);
        break;
      case 47:
        TryAndGSCookieAndAlignAndAllocaAndEbpAdj(1);
        break;
      }
    }
    __except(one) { caught = true; }

    if (!caught) {
      puts("ERROR - did not see expected exception");
      ++failures;
    }
  }
}

int main() {
  __try { RunTests(); }
  __except(1) {
    puts("ERROR - Unexpectedly caught an exception");
    ++failures;
  }

  if (failures) {
    printf("Test failed with %d failure%s\n", failures,
           failures == 1 ? "" : "s");
  } else {
    puts("Test passed");
  }

  return failures;
}
