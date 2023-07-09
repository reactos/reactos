/*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*/

#include "precomp.h"
#pragma hdrstop
#pragma hdrstop


DWORD
main(VOID)
{
    DWORD Status;
    ULONG BytesTransferred;
    INPUT_RECORD Char;
    BOOL Success;
    HANDLE Array[1];
    int i;

    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    for (i=0;i<2;i++) {
        printf("Waiting...\n");
        Status = WaitForSingleObject(GetStdHandle(STD_INPUT_HANDLE),-1);
        if (Status != 0) {
            printf("wait failed %lx\n",Status);
            return 1;
        }
        Success = ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE),
                           &Char,
                           1,
                           &BytesTransferred
                          );
        if (Status != 0) {
            printf("read failed %lx\n",Status);
            return 1;
        }
        if (BytesTransferred != 1) {
           printf("Read returned %d chars\n",BytesTransferred);
        }
        printf("Read returned %c\n",Char.Event.KeyEvent.uChar.AsciiChar);
    }

    Array[0] = GetStdHandle(STD_INPUT_HANDLE);
    for (i=0;i<2;i++) {
        printf("Waiting...\n");
        Status = WaitForMultipleObjects(1,Array,FALSE,-1);
        if (Status != 0) {
            printf("wait failed %lx\n",Status);
            return 1;
        }
        Success = ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE),
                           &Char,
                           1,
                           &BytesTransferred
                          );
        if (Status != 0) {
            printf("read failed %lx\n",Status);
            return 1;
        }
        if (BytesTransferred != 1) {
           printf("Read returned %d chars\n",BytesTransferred);
        }
        printf("Read returned %c\n",Char.Event.KeyEvent.uChar.AsciiChar);
    }
}
