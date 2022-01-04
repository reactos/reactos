// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for
// full license information.

#include <stdio.h>
#include <windows.h>
#ifdef _M_CEE
#define MAX_REC 200
#else
#define MAX_REC 200
#endif

#define NLOOPS 10

#if defined(_M_SH) || defined(TRISC)
#undef MAX_REC
#define MAX_REC 3
#endif

#if defined(_M_AMD64)
#undef MAX_REC
#define MAX_REC 50
#endif

void foo(int level)
{
    if(level < MAX_REC)
    {
        try
        {
            foo(level+1);
        }
        catch(int& throw_level)
        {
            //printf("Level %d catched level %d\n",level,throw_level);
            throw_level = level;
            //printf("Level %d throwing\n",level);
            throw level;
        }
    }
    else
    {
        printf("Level MAX_REC throwing.\n");
        throw level;
    }
}

int main(void)
{
    int n;
    LARGE_INTEGER freq,start,end;

#if !defined(_M_AMD64) && !defined(_M_ARM) && !defined(_M_ARM64)
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
#endif

    //printf("Max Recursion level: %d\n", MAX_REC);
    printf("N loops: %d\n", NLOOPS);

    for(n=0;n<NLOOPS;n++)
    {
        try
        {
            foo(0);
        }
        catch(int& throw_level)
        {
            printf("main catched level %d\n",throw_level);
        }
    }
#if !defined(_M_AMD64) && !defined(_M_ARM) && !defined(_M_ARM64)
    QueryPerformanceCounter(&end);
#endif

    //printf("Exceptions per sec %lf\n",(double)(NLOOPS*MAX_REC)/((double)(end.QuadPart-start.QuadPart)/(double)freq.QuadPart));
}
