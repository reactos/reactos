/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    mibtree.h

Abstract:

    mibtree.h contains the definitions used by the MIB tree routines.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
#ifndef mibtree_h
#define mibtree_h
 
//--------------------------- PUBLIC CONSTANTS ------------------------------
//--------------------------- PUBLIC STRUCTS --------------------------------

typedef struct _TreeNode {
   struct _TreeNode *   lpParent;           /* pointer to parent */
   struct _TreeNode * * lpChildArray;       /* array is alloced */
   UINT                 uNumChildren;
   LPSTR                lpszTextSubID;
   UINT                 uNumSubID;
} TreeNode, *lpTreeNode;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

extern lpTreeNode lpMIBRoot;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

void TreeInit (lpTreeNode *lplpRoot);
void TreeDeInit (lpTreeNode *lplpRoot);
void NodeInit (lpTreeNode lpNode);
lpTreeNode FindNodeByName (lpTreeNode lpParent, LPSTR lpszName);
lpTreeNode FindNodeByNumber (lpTreeNode lpParent, UINT uNum);
lpTreeNode FindNodeAddToTree (lpTreeNode lpParent, LPSTR lpszName, UINT uNum);
lpTreeNode FindNodeInSubTree (lpTreeNode lpRoot, LPSTR lpszName);
lpTreeNode NewChildNode (LPSTR lpszName, UINT uNum);
void InsertChildNode (lpTreeNode lpParent, lpTreeNode lpNode);
void PrintTree (lpTreeNode lpRoot, unsigned int nIndent);

//--------------------------- END -------------------------------------------

#endif /* mibtree_h */
