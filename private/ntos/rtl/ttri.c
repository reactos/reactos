/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tsplay.c

Abstract:

    Test program for the Splay Procedures

Author:

    Gary Kimura     [GaryKi]    24-May-1989

Revision History:

--*/

#define DbgPrint DbgPrint

#include <stdio.h>

#include "nt.h"
#include "ntrtl.h"
#include "triangle.h"

ULONG RtlRandom (IN OUT PULONG Seed);

typedef struct _TREE_NODE {
    CLONG Data;
    TRI_SPLAY_LINKS Links;
} TREE_NODE;
typedef TREE_NODE *PTREE_NODE;

TREE_NODE Buffer[2048];

PTREE_NODE
TreeInsert (
    IN PTREE_NODE Root,
    IN PTREE_NODE Node
    );

VOID
PrintTree (
    IN PTREE_NODE Node
    );

int
_CDECL
main(
    int argc,
    char *argv[]
    )
{
    PTREE_NODE Root;
    ULONG i;
    ULONG Seed;

    DbgPrint("Start TriangleTest()\n");

    Root = NULL;
    Seed = 0;
    for (i=0; i<2048; i++) {
        Buffer[i].Data = RtlRandom(&Seed);
        Buffer[i].Data = Buffer[i].Data % 512;
        TriInitializeSplayLinks(&Buffer[i].Links);
        Root  = TreeInsert(Root, &Buffer[i]);
    }

    PrintTree(Root);

    DbgPrint("End TriangleTest()\n");

    return TRUE;

}

PTREE_NODE
TreeInsert (
    IN PTREE_NODE Root,
    IN PTREE_NODE Node
    )

{
    PTRI_SPLAY_LINKS Temp;

    if (Root == NULL) {

        //DbgPrint("Add as root %u\n", Node->Data);
        return Node;

    }

    while (TRUE) {

        if (Root->Data == Node->Data) {

            //DbgPrint("Delete %u\n", Node->Data);

            Temp = TriDelete(&Root->Links);
            if (Temp == NULL) {
                return NULL;
            } else {
                return CONTAINING_RECORD(Temp, TREE_NODE, Links);
            }

        }

        if (Root->Data < Node->Data) {

            //
            //  Go right
            //

            if (TriRightChild(&Root->Links) == NULL) {

                //DbgPrint("Add as right child %u\n", Node->Data);
                TriInsertAsRightChild(&Root->Links, &Node->Links);
                return CONTAINING_RECORD(TriSplay(&Node->Links), TREE_NODE, Links);

            } else {

                Root = CONTAINING_RECORD(TriRightChild(&Root->Links), TREE_NODE, Links);

            }

        } else {

            //
            //  Go Left
            //

            if (TriLeftChild(&Root->Links) == NULL) {

                //DbgPrint("Add as left child %u\n", Node->Data);
                TriInsertAsLeftChild(&Root->Links, &Node->Links);
                return CONTAINING_RECORD(TriSplay(&Node->Links), TREE_NODE, Links);

            } else {

                Root = CONTAINING_RECORD(TriLeftChild(&Root->Links), TREE_NODE, Links);

            }

        }
    }
}

VOID
PrintTree (
    IN PTREE_NODE Node
    )

{
    PTRI_SPLAY_LINKS Temp;
    ULONG LastValue;

    if (Node == NULL) {
        return;
    }

    //
    //  find smallest value
    //

    while (TriLeftChild(&Node->Links) != NULL) {
        Node = CONTAINING_RECORD(TriLeftChild(&Node->Links), TREE_NODE, Links);
    }
    LastValue = Node->Data;
    //DbgPrint("%u\n", Node->Data);

    //
    //  while the is a real successor we print the successor value
    //

    while ((Temp = TriRealSuccessor(&Node->Links)) != NULL) {
        Node = CONTAINING_RECORD(Temp, TREE_NODE, Links);
        if (LastValue >= Node->Data) {
            DbgPrint("TestSplay Error\n");
        }
        LastValue = Node->Data;
        //DbgPrint("%u\n", Node->Data);
    }

}
