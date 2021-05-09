// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

/*
** The first call to Test checks to make sure that we don't call terminate
** when an SEH fault is triggered. After catching the SEH fault, we then
** cause a C++ EH fault (where the destructed object is in the try-block
** that threw the exception). The C++ EH fault in the dtor should cause
** a call to terminate(); in this case, we register our own handler to
** trap the call to terminate(). We then run a second C++ EH fault (where
** the destructed object is higher in the call tree than where the 
** exception was thrown). We trap this call to terminate() as well (with
** a different handler) and then exit.
*/

#include <stdio.h>
#include <stdlib.h>
#include <eh.h>

int FGenSEHException;

class C 
{
public:
    C() {}
    ~C()
    {
        printf( "in C::~C()\n");
        if (FGenSEHException)
        {
            printf("generating access violation (SEH)\n");
            *(volatile char*)(0) = 0; // Uh, EHa, don't you think?
        }
        else
        {
            printf("throwing C++ exception\n");
            throw 'a';
        }
    }
};

int Test()
{
    try 
    {
        C c;
        throw 1;
    }
    catch (int) 
    {
        printf("Destructor was not invoked \n");
    }
    catch (char) 
    {
        printf("Destructor exited using an exception\n");
    }
#if 0
    catch (...) 
    {
        printf("Throw caught by wrong handler\n");
    }
#endif

    printf("terminate() was not called\n");
    
    return 1;
}

void Bar(void)
{
    printf("in %s\n", __FUNCTION__);
    throw 1;
}

void Test2(void)
{
    printf("in %s\n", __FUNCTION__);
    C c;
    Bar();
    return;
}

void __cdecl Terminate2(void)
{
    printf("termination handler (%s) called\n", __FUNCTION__);
    exit(0);
}

void __cdecl Terminate1(void)
{
    printf("termination handler (%s) called\n", __FUNCTION__);

    // Set a new handler and run a second C++ EH test case.
    set_terminate(Terminate2);
    Test2();
    exit(0);
}

int main(void)
{
    int i;

    // First check that we don't terminate on an SEH exception
    FGenSEHException = 1;
    __try 
    {
        i = Test();
    }
    __except (1)
    {
        printf("caught SEH exception in %s\n", __FUNCTION__);
    }

    // Now set our own terminate handler and throw a C++ EH exception
    set_terminate(Terminate1);
    FGenSEHException = 0;
    i = Test();

    // Should never get here
    printf("termination handler not called\n");
    return i;
}
