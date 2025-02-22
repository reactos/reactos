/*
 * PROJECT:         ReactOS Tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            queuetest.c
 * PURPOSE:         Usermode QueueUserWorkItem() testing
 * PROGRAMMERS:     Thomas Weidenmueller (w3seek@reactos.org)
 */

#include <windows.h>
#include <stdio.h>

#define WT_EXECUTEINPERSISTENTIOTHREAD 0x00000040
BOOL WINAPI QueueUserWorkItem(LPTHREAD_START_ROUTINE,PVOID,ULONG);

#define TestProc(n) \
DWORD CALLBACK TestProc##n(void *ctx)\
{\
    printf("TestProc%d thread 0x%lx context 0x%p\n", n, GetCurrentThreadId(), ctx);\
    return 0;\
}

TestProc(1)
TestProc(2)
TestProc(3)
TestProc(4)
TestProc(5)
TestProc(6)

int __cdecl
main(int argc, char* argv[])
{
    PVOID x = (PVOID)0x12345;
    QueueUserWorkItem(TestProc1, x, 0);
    QueueUserWorkItem(TestProc2, x, WT_EXECUTELONGFUNCTION);
    QueueUserWorkItem(TestProc3, x, WT_EXECUTEINIOTHREAD);
    QueueUserWorkItem(TestProc4, x, WT_EXECUTEINIOTHREAD | WT_EXECUTELONGFUNCTION);
    QueueUserWorkItem(TestProc5, x, WT_EXECUTEINPERSISTENTTHREAD);
    QueueUserWorkItem(TestProc6, x, WT_EXECUTEINPERSISTENTIOTHREAD);
    Sleep(INFINITE);
    return 0;
}
