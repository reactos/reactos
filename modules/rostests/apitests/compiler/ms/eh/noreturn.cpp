// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <stdio.h>

int Count = 0;

class A
{
    public:
	A() { Count++; }
	~A() { Count--; }

	__declspec(noreturn)
	A(const A &a) { throw 1; }
};

__declspec(noreturn)
int bar(A)
{
    throw 1;
}

__declspec(noreturn)
A foobar()
{
    throw 1;
}

void foo(const A& a, int i)
{
    A a1 = a;
    bar(a1);
}

int main()
{
    try {
	A a;
    	foo(A(a), bar(a));
    } catch (int i) {}

    try {
	A a;
    	foo(foobar(), bar(a));
    } catch (int i) {}

    if (Count == 0)
	printf("Passed");
    else
        printf("FAILED\n");

}
