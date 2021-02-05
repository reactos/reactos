// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <stdio.h> 
int c, d; 
struct A
{
    int i;
    A () { i = ++c; printf ("A() %d\n", i); }
    A (const A&) { i = ++c; printf ("A(const A&) %d\n", i); }
    ~A() { printf ("~A() %d\n", i); ++d; }
}; 
int
main ()
{
    try
    {
        try
        {
            printf ("Throwing 1...\n");
            throw A();
        }
        catch (A)
        {
            try
            {
                printf ("Throwing 2...\n");
                throw A();
            }
            catch (A)
            {
                printf ("Throwing 3...\n");
                throw;
            }
        }
    }
    catch (A)
    {
        printf ("Caught.\n");
    }
    printf ("c == %d, d == %d\n", c, d);
    return c != d;
} 
