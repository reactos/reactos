// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

/*        
     Tests some throw and rethrow situations (mostly CRT a test)              
*/


#include <stdlib.h>
#include <stdio.h>


#define FALSE 0
#define TRUE 1
#define NO_CTOR_THROW 1
#define NO_DTOR_THROW 2


int Object[100];
int CurrentObjectNumber, Test;
int MaxTest = 10;
int MaxObjectCount = 1;
int Fail;


void FAIL(int i)
{
    printf("FAILED on %d\n", i);
    Fail++;
}

void dealloc(int i, int no_throw)
{
    /* Make sure i is valid, and object exists */
    if(i<0 || i>=MaxObjectCount || !Object[i]) 
        FAIL(i);
    
    Object[i] = 0;
}

void alloc(int i, int no_throw)
{
    if(CurrentObjectNumber > MaxObjectCount)
        MaxObjectCount = CurrentObjectNumber;
    
    /* Object already exists? */
    if(Object[i]) FAIL(i);
    
    Object[i] = 1;
}

class B
{
public:
    int i;
    int flag;
    B();
    B(int);
    B(const B &b);
    ~B();
};

B::B()
{
    i = CurrentObjectNumber++;
    printf("B ctor.  i = %d\n", i);
    alloc(i, FALSE);
}

B::B(int f)
{
    i = CurrentObjectNumber++;
    flag = f;
    printf("B ctor.  i = %d\n", i);
    alloc(i, flag==NO_CTOR_THROW);
}

B::B(const B &b)
{
    i = CurrentObjectNumber++;
    printf("B copy ctor.  i = %d\n", i);
    alloc(i, FALSE);
}

B::~B()
{
    printf("B dtor.  i = %d\n", i);
    dealloc(i, flag==NO_DTOR_THROW);
}

class A
{
public:
    int i;
    A();
    A(int) 
    {
        i = CurrentObjectNumber++;
        printf("A(int) ctor.  i = %d\n", i);
        alloc(i, FALSE);
    }
    A operator+(A a);
    A(const A &a)
    {
        /* Try objects in ctor */
        B b1 = NO_DTOR_THROW, b2 = NO_DTOR_THROW;
        
        i = CurrentObjectNumber++;
        printf("A copy ctor.  i = %d\n", i);
        alloc(i, FALSE);
    }
    
    ~A(){
        /* Try objects in dtor */
        B b1 = NO_CTOR_THROW, b2 = NO_CTOR_THROW;
        
        printf("A dtor.  i = %d\n", i);
        dealloc(i, FALSE);
    };
};

A::A()
{
    i=CurrentObjectNumber++;
    printf("A ctor.  i = %d\n", i);
    alloc(i, FALSE);
}

A A::operator+(A a)
{
    printf("A%d + A%d\n", i, a.i);
    return A();
}

void Throwa(A a)
{
    printf("Throwing\n");
    throw a;
}

void bar()
{
    A a;
    
    Throwa(a);
}

void foobar()
{
    B b;
    bar();
}

// Somehow, inlining this causes different unwinding order..

__declspec(noinline) void Rethrow2()
{
    A a;
    printf("Rethrowing\n");
    throw;
}

#pragma inline_depth(0)
void Rethrow()
{
    Rethrow2();
}
#pragma inline_depth()

void foobar2()
{
    B b;
    
    try{
        A a;
        bar();
    }catch(A a){
        printf("In catch;\n");
        Rethrow();
    }
}

void foobar3()
{
    B b;
    
    try{
        A a;
        bar();
    }catch(A a){
        printf("In catch\n");
        A a2;
        
        printf("Throwing new a\n");
        throw a2;
    }
}

void foobar4()
{
    B b;
    
    try{
        B b;
        try{
            A a1, a2;
            try {
                A a1, a2;
                foobar2();
            }catch(A a){
                printf("In catch #1\n");
                B b;
                printf("Rethrowing\n");
                throw;
            }
        }catch(A &a){
            printf("In catch #2\n");
            A a2;
            
            printf("Throwing new a\n");
            throw a;
        }
    }catch(A a){
        printf("In catch #3\n");
        B b;
        printf("Rethrowing\n");
        throw;
    }
}

__declspec(noinline) void throw_B_2()
{
    B b;
    printf("Throwing a new b\n");
    throw b;
}

#pragma inline_depth(0)
void throw_B()
{
    throw_B_2();
}
#pragma inline_depth()


void foobar5()
{
    try {
        B b1;
        try {
            B b2;
            try {
                B b3;
                foobar();
            }catch(B b){
                printf("In catch #1\n");
                FAIL(-1);
            }
            FAIL(-1);
        }catch(A a){
            A a2;
            printf("In catch #2\n");
            throw_B();
        }
        FAIL(-1);
    }catch(B b){
        printf("In catch #3\n");
        printf("Throwing a new a\n");
        throw A();
    }
    FAIL(-1);
}


/* Simple throw with unwinds */
void test1()
{
    A a;
    foobar();
}

/* Throw followed by a rethrow */
void test2()
{
    A a;
    foobar2();
}

/* Throw followed by a new throw */
void test3()
{
    A a;
    foobar3();
}

/* Nested trys with rethrow/throw/rethrow */
void test4()
{
    A a;
    foobar4();
}

/* Makes sure a new throw skips appropriate unwound frames. */
void test5()
{
    A a;
    foobar5();
}

// Tests 3 level of new throw
void test6()
{
    try{
        B b1;
        try{
            B b2;
            try{
                B b3;
                printf("Throwing a b\n");
                throw(b3);
            }catch(B b){
                B b4;
                printf("In catch #1\n");
                printf("Throwing a new b\n");
                throw(b4);
            }
            FAIL(-1);
        }catch(B b){
            B b5;
            printf("In catch #2\n");
            printf("Throwing a new b\n");
            throw(b5);
        }
        FAIL(-1);
    }catch(B b){
        A a1;
        printf("In catch #3\n");
        printf("Throwing a new a\n");
        throw(a1);
    }
    FAIL(-1);
}

// Testing try/catch inside a catch
void test7()
{
    B b1;
    try{
        B b2;
        try{
            B b3;
            
            printf("Throwing a b\n");
            throw(B());
        }catch(B b){
            B b4;
            printf("In catch #1\n");
            try{
                B b5;
                printf("Rethrowing b\n");
                throw;
            }catch(B b){
                B b5;
                printf("In catch #1 of catch#1\n");
                printf("Rethrowing b\n");
                throw;
            }
        }
    }catch(B b){
        B b6;
        printf("In catch #2\n");
        printf("Throwing a new A\n");
        throw(A());
    }
}

void ThrowB()
{
    B b;
    
    throw(B());
}

void bar8()
{
    try{
        B b5;
        printf("Rethrowing b\n");
        Rethrow();
    }catch(B b){
        B b5;
        printf("In catch #1 of catch#1\n");
        printf("Rethrowing b\n");
        Rethrow();
    }
}

void foo8()
{
    B b;
    try{
        B b3;
        
        printf("Throwing a b\n");
        ThrowB();
    }catch(B b){
        B b4;
        printf("In catch #1\n");
        bar8();
    }
}

// Testing call to try/catch function inside a catch
void test8()
{
    B b1;
    try{
        B b2;
        foo8();
    }catch(B b){
        B b6;
        printf("In catch #2\n");
        printf("Throwing a new A\n");
        throw(A());
    }
}

void foo9()
{
    try {
        puts("Rethrow");
        throw;
    }catch(...){
        puts("In catch #2");
    }
}

void test9()            
{
    try{
        B b;
        puts("Throwing B");
        throw b;
    }catch(...){
        puts("In catch #1");
        foo9();
    }
    puts("End of test9, throwing a A");
    throw A();
}

void foo10()
{
    try {
        puts("Throwing a new B()");
        throw B();
    }catch(...){
        puts("In catch #2");
    }
}

void test10()           
{
    try{
        B b;
        puts("Throwing B");
        throw b;
    }catch(...){
        puts("In catch #1");
        foo10();
    }
    puts("End of test10, throwing a A");
    throw A();
}

int main()
{
    int i;
    
    /* Call test(), with a different ctor/dtor throwing each time */
    for(Test = 1; Test <= MaxTest; Test++)  {
        
        CurrentObjectNumber = 0;
        
        printf("\nTest #%d\n", Test);
        
        try {
            switch(Test){
            case 1:
                test1();
                break;
            case 2:
                test2();
                break;
            case 3:
                test3();
                break;
            case 4:
                test4();
                break;
            case 5:
                test5();
                break;
            case 6:
                test6();
                break;
            case 7:
                test7();
                break;
            case 8:
                test8();
                break;
            case 9:
                test9();
                break;
            case 10:
                test10();
                break;
            }
            
            FAIL(-1);
            
        }catch(A a){
            printf("In main's catch\n");
        }catch(...){
            FAIL(-1);
        }
        
        /* Any objects which didn't get dtor'd? */
        for(i = 0; i < MaxObjectCount; i++) {
            if(Object[i]) {
                FAIL(i);
                Object[i] = 0;
            }
        }
        
        printf("\n");
    }
    
    printf("\n");
    if(Fail)
        printf("FAILED %d tests\n", Fail);
    else
        printf("Passed\n");
    
}
