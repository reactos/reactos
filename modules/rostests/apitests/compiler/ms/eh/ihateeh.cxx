// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <stdlib.h>
#include <stdio.h>

#define FALSE 0
#define TRUE 1
#define NO_CTOR_THROW 1
#define NO_DTOR_THROW 2

#define TEST_DTOR

#define MAX_OBJECTS 500

#define NOEXCEPT(_x)

int Object[MAX_OBJECTS];
int CurrentObjectNumber, ThrowCount, MaxObjectCount = 1;
int Fail;

void FAIL(int i) {
  printf("FAILED on %d\n", i);
  Fail++;
}

void dealloc(int i, int no_throw) {
  /* Make sure i is valid, and object exists */
  if (i < 0 || i >= MaxObjectCount || !Object[i])
    FAIL(i);

  Object[i] = 0;

/* Try throw in this dtor.. */
#ifdef TEST_DTOR
  if (i + MaxObjectCount == ThrowCount && !no_throw) {
    printf("Throwing\n");
    throw(1);
  }
#endif
}

void alloc(int i, int no_throw) {
  if (CurrentObjectNumber > MaxObjectCount)
    MaxObjectCount = CurrentObjectNumber;

  /* Object already exists? */
  if (Object[i])
    FAIL(i);

  /* Try throw in this ctor.. */
  if (i == ThrowCount && !no_throw) {
    printf("Throwing\n");
    throw(1);
  }

  if (i >= MAX_OBJECTS) {
    printf("\n*** Number of objects exceeded.  Increase MAX_OBJECTS ***\n\n");
    FAIL(i);
    i = 0;
  }

  Object[i] = 1;
}

class B {
public:
  int i;
  int flag;
  B();
  B(int);
  ~B() NOEXCEPT(false);
};

B::B() {
  i = CurrentObjectNumber++;
  printf("B ctor.  i = %d\n", i);
  alloc(i, FALSE);
}

B::B(int f) {
  i = CurrentObjectNumber++;
  flag = f;
  printf("B ctor.  i = %d\n", i);
  alloc(i, flag == NO_CTOR_THROW);
}

B::~B() NOEXCEPT(false) {
  printf("B dtor.  i = %d\n", i);
  dealloc(i, flag == NO_DTOR_THROW);
}

class A {
public:
  int i;
  A();
  A(int) {
    i = CurrentObjectNumber++;
    printf("A(int) ctor.  i = %d\n", i);
    alloc(i, FALSE);
  }
  A operator+(A a);
  A(const A &a) {
    /* Try objects in ctor */
    B b1 = NO_DTOR_THROW, b2 = NO_DTOR_THROW;

    i = CurrentObjectNumber++;
    printf("A copy ctor.  i = %d\n", i);
    alloc(i, FALSE);
  }

  ~A() NOEXCEPT(false) {
    /* Try objects in dtor */
    B b1 = NO_CTOR_THROW, b2 = NO_CTOR_THROW;

    printf("A dtor.  i = %d\n", i);
    dealloc(i, FALSE);
  };
};

A::A() {
  i = CurrentObjectNumber++;
  printf("A ctor.  i = %d\n", i);
  alloc(i, FALSE);
}

A A::operator+(A a) {
  printf("A%d + A%d\n", i, a.i);
  return A();
}

A foo(A a1, A a2) { return a1 + a2; };

int bar() {
  A a;

  return 666;
}

void foo2(int i, A a1, A a2, A a3) {
  if (i != 666)
    FAIL(666);
  foo(a1, a3);
}

A test() {
  puts("Try simple ctor");
  A a1;

  puts("Try question op ctor");
  A a2 = (ThrowCount & 1) ? A() : ThrowCount;

  puts("Try a more complex question op ctor");
  A a3 = (ThrowCount & 1) ? A() + a1 + A() + a2 : a2 + A() + A() + ThrowCount;

  puts("Try mbarg copy ctors, and return UDT");
  A a4 = foo(a1, a2);

  puts("Try a more complex mbarg copy ctors, and a function call");
  foo2(bar(), a1 + a2, a2 + A() + a3 + a4, a3);

  puts("Try temporary expressions, and return UDT");
  return a1 + A() + a2 + A() + a3 + a4;
}

int main() {
  int i;

  /* Call test(), with a different ctor/dtor throwing each time */
  for (ThrowCount = 0; ThrowCount < MaxObjectCount * 2; ThrowCount++) {
    printf("ThrowCount = %d   MaxObjectCount = %d\n", ThrowCount,
           MaxObjectCount);

    CurrentObjectNumber = 0;

    try {
      test();
    } catch (int) {
      printf("In catch\n");
    }

    /* Any objects which didn't get dtor'd? */
    for (i = 0; i < MaxObjectCount; i++) {
      if (Object[i]) {
        FAIL(i);
        Object[i] = 0;
      }
    }

    printf("\n");
  }

  printf("\n");
  if (Fail)
    printf("FAILED %d tests\n", Fail);
  else
    printf("Passed\n");
}
