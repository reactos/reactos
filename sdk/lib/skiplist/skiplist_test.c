/*
 * PROJECT:     Skiplist implementation for the ReactOS Project
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     A simple program for testing the Skiplist implementation
 * COPYRIGHT:   Copyright 2015 Colin Finck (colin@reactos.org)
 */

#include <windows.h>
#include <stdio.h>
#include "skiplist.h"

void
DumpSkiplist(PSKIPLIST Skiplist)
{
    CHAR i;
    DWORD j;
    PSKIPLIST_NODE pNode;

    printf("======= DUMPING SKIPLIST =======\n");

    for (i = Skiplist->MaximumLevel + 1; --i >= 0;)
    {
        pNode = &Skiplist->Head;
        printf("H");

        while (pNode->Next[i])
        {
            printf("-");

            // By using the Distance array for painting the lines, we verify both the links and the distances for correctness.
            for (j = 1; j < pNode->Distance[i]; j++)
                printf("---");

            printf("%02Iu", (DWORD_PTR)pNode->Next[i]->Element);

            pNode = pNode->Next[i];
        }

        printf("\n");
    }

    printf("================================\n\n");
}

PVOID WINAPI
MyAlloc(DWORD Size)
{
    return HeapAlloc(GetProcessHeap(), 0, Size);
}

int WINAPI
MyCompare(PVOID A, PVOID B)
{
    return (DWORD_PTR)A - (DWORD_PTR)B;
}

void WINAPI
MyFree(PVOID Ptr)
{
    HeapFree(GetProcessHeap(), 0, Ptr);
}

int
main()
{
    PVOID Element;
    DWORD ElementIndex;
    DWORD i;
    SKIPLIST Skiplist;
    PSKIPLIST_NODE pNode;

    system("mode con cols=300");
    InitializeSkiplist(&Skiplist, MyAlloc, MyCompare, MyFree);

    // Insert some random elements with random numbers.
    for (i = 0; i < 40; i++)
        InsertElementSkiplist(&Skiplist, UlongToPtr(rand() % 100));

    // Delete all with index 0 to 29.
    for (i = 0; i < 30; i++)
        DeleteElementSkiplist(&Skiplist, UlongToPtr(i));

    // Insert some more random elements.
    for (i = 0; i < 40; i++)
        InsertElementSkiplist(&Skiplist, UlongToPtr(rand() % 100));

    // Output the third element (with zero-based index 2).
    pNode = LookupNodeByIndexSkiplist(&Skiplist, 2);
    printf("Element = %Iu for index 2\n", (DWORD_PTR)pNode->Element);

    // Check if an element with number 44 is in the list and output its index.
    Element = LookupElementSkiplist(&Skiplist, UlongToPtr(44), &ElementIndex);
    printf("Element = %p, ElementIndex = %lu\n\n", Element, ElementIndex);

    DumpSkiplist(&Skiplist);

    return 0;
}
