/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    mibtree.c

Abstract:

    mibtree.c contains the routines used to manipulate the MIB tree.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include<stdlib.h>
#include<stdio.h>
#include<string.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include"mibcc.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include"mibtree.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define DEBUG(x)	FALSE

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------
int yyerror(char *s);

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

void TreeInit (lpTreeNode *lplpRoot)
{
   *lplpRoot = NULL;
}


void TreeDeInit (lpTreeNode *lplpRoot)
{
   unsigned int i;

   if (NULL == *lplpRoot) {
      return;
   } else {
      // tell children to free their children
      for (i = 0; i < (*lplpRoot)->uNumChildren; i++) {
         TreeDeInit(&(*lplpRoot)->lpChildArray[i]);
      }

      // free the array of children, if any
      if (NULL != (*lplpRoot)->lpChildArray)
          free ((*lplpRoot)->lpChildArray);
      (*lplpRoot)->lpChildArray = NULL;
      (*lplpRoot)->uNumChildren = 0;

      // now free our-selves
      free (*lplpRoot);
      *lplpRoot = NULL;
   }
}


void NodeInit (lpTreeNode lpNode)
{
   lpNode->lpParent = NULL;
   lpNode->lpChildArray = NULL;
   lpNode->uNumChildren = 0;
   lpNode->lpszTextSubID = NULL;
   lpNode->uNumSubID = 0;
}


//*************************************************************************
//* FindNodeByName
//*	Returns a pointer to the first child with the given name.
//*	This function does not check children's children.
//*************************************************************************
lpTreeNode FindNodeByName (
   lpTreeNode lpParent,
   LPSTR lpszName)
{
   lpTreeNode lpResult = NULL;
   UINT i;

   if (DEBUG(1))
      fprintf (error_out, "Find Node By Name: LINE: %i NAME: '%s'\n", lineno, lpszName);

   if (lpMIBRoot == lpParent)
      return (FindNodeInSubTree(lpParent, lpszName));

   if (NULL == lpParent) {
      if (DEBUG(1))
         fprintf (error_out, "Find Node By Name: lpParent is NULL!\n");
      goto Exit;
   }

   for (i = 0; i < lpParent->uNumChildren; i++) {
      if (0 == strcmp (lpParent->lpChildArray[i]->lpszTextSubID, lpszName)) {
         lpResult = lpParent->lpChildArray[i];
         break;
      }
   }

Exit:
   return (lpResult);
}


//*************************************************************************
//* FindNodeByNumber
//*	Returns a pointer to the first child with the given Number.
//*	This function does not check children's children.
//*************************************************************************
lpTreeNode FindNodeByNumber (
   lpTreeNode lpParent,
   UINT uNum)
{
   lpTreeNode lpResult = NULL;
   UINT i;

   if (NULL == lpParent) goto Exit;

   if (lpMIBRoot == lpParent) {
      if (lpParent->uNumSubID == uNum) {
         lpResult = lpParent;
         goto Exit;
      }
   }

   for (i = 0; i < lpParent->uNumChildren; i++) {
      if (lpParent->lpChildArray[i]->uNumSubID == uNum) {
         lpResult = lpParent->lpChildArray[i];
         break;
      }
   }

Exit:
   return (lpResult);
}


//*************************************************************************
//* FindNodeAddToTree
//*	Returns a pointer to the first child with the given Number/Name.
//*	If the Number/Name is not present, then it is added
//*	This function does not check children's children.
//*************************************************************************
lpTreeNode FindNodeAddToTree (
   lpTreeNode lpParent,
   LPSTR lpszName,
   UINT uNum)
{
   lpTreeNode lpResult = NULL;
   UINT i;

   if (DEBUG(1))
      fprintf (error_out, "Call to Find Node Add To Tree LINE: %i NUMBER: %i NAME: '%s'\n", lineno, uNum, lpszName);

   if (NULL == lpParent) {
      if (DEBUG(1))
         fprintf (error_out, "Find Node: lpParent is NULL\n");
      goto Exit;
   }

   if (lpMIBRoot == lpParent)
      lpResult = FindNodeInSubTree(lpParent, lpszName);

   if (NULL != lpResult)
      goto Exit;	/* we found the node */

   for (i = 0; i < lpParent->uNumChildren; i++) {
      if (lpParent->lpChildArray[i]->uNumSubID == uNum) {
         if (0 == strcmp (lpParent->lpChildArray[i]->lpszTextSubID, lpszName)) {
           lpResult = lpParent->lpChildArray[i];
           goto Exit;
         } else {
           fprintf (error_out, "error : attempt to overwrite node %s(%i) with node %s(%i).  ",
                    lpParent->lpChildArray[i]->lpszTextSubID,
                    lpParent->lpChildArray[i]->uNumSubID,
                    lpszName, uNum );
           yyerror ("Name overwrite detected");
         }
      }
   }
   // at this point we did not find the node, so add it
   lpResult = NewChildNode (lpszName, uNum);
   InsertChildNode (lpParent, lpResult);
   if (DEBUG(1))
      fprintf (error_out, "   Node ADDED!\n");

Exit:
   return (lpResult);
}


lpTreeNode FindNodeInSubTree (lpTreeNode lpRoot, LPSTR lpszName)
{
   lpTreeNode lpResult = NULL;
   unsigned int i;

   if (NULL == lpRoot) {
      goto Exit;
   } else {
      if (0 == strcmp (lpRoot->lpszTextSubID, lpszName)) {
         lpResult = lpRoot;
      } else {
         for (i = 0; i < lpRoot->uNumChildren; i++) {
            if (0 == strcmp (lpRoot->lpChildArray[i]->lpszTextSubID, lpszName)) {
               lpResult = lpRoot->lpChildArray[i];
               break;
            }
            if (NULL != (lpResult = FindNodeInSubTree(lpRoot->lpChildArray[i],lpszName))) {
               break;
            }
         }
      }
   }
Exit:
   return (lpResult);
}

lpTreeNode NewChildNode (LPSTR lpszName, UINT uNum)
{
   lpTreeNode lpResult;
   lpResult = (lpTreeNode) malloc (sizeof (TreeNode));
   if (NULL != lpResult) {
      NodeInit (lpResult);
      lpResult->uNumSubID = uNum;
      lpResult->lpszTextSubID = malloc (strlen (lpszName)+1);

      if (NULL != lpResult->lpszTextSubID)
         strcpy (lpResult->lpszTextSubID, lpszName);
      else
         fprintf (error_out,"NewChildNode: malloc #2 failed!\n");

   } else {
      fprintf (error_out,"NewChildNode: malloc #1 failed!\n");
   }
   return (lpResult);
}

void InsertChildNode (lpTreeNode lpParent, lpTreeNode lpChildNode)
{
   if (DEBUG(1))
      fprintf (error_out,"Call to InsertChildNode from LINE: %i NUM: '%i' NAME: '%s'\n", lineno, lpChildNode->uNumSubID, lpChildNode->lpszTextSubID);


   if ((NULL != lpParent) && (NULL != lpChildNode)) {
      if (fNodePrint) { /* command line switch */
         printf ("Node Inserted ->  %s(%i).%s(%i)\n",
                 lpParent->lpszTextSubID, lpParent->uNumSubID,
                 lpChildNode->lpszTextSubID, lpChildNode->uNumSubID);
      }
      if (0 == lpParent->uNumChildren) {
         lpParent->lpChildArray = (lpTreeNode *) malloc (sizeof (lpTreeNode *));

         if (NULL != lpParent->lpChildArray) {
            lpParent->lpChildArray[0] = lpChildNode;
            lpParent->uNumChildren = 1;
            lpChildNode->lpParent = lpParent;
         } else {
            fprintf (error_out,"InsertChildNode: malloc #1 failed!\n");
         }

      } else {
         lpParent->uNumChildren++;
         lpParent->lpChildArray =
            (lpTreeNode *) realloc (lpParent->lpChildArray,
                                  lpParent->uNumChildren *
                                  sizeof (lpTreeNode *));
         if (NULL != lpParent->lpChildArray) {
            UINT i;
            // assert (lpParent->uNumChildren >= 2);
            for (i = lpParent->uNumChildren; i > 0 ; i--) {
               //! Note: dependency on short circuit boolean of (1==i)
               // if we're at the ZEROth entry or the i-2 entry has
               // a smaller number than us, then put us here at i-1
               if ((1 == i) || (lpParent->lpChildArray[i-2]->uNumSubID < lpChildNode->uNumSubID)) {
                 lpParent->lpChildArray[i-1] = lpChildNode;
                 lpChildNode->lpParent = lpParent;
                 break;
               } else if (lpParent->lpChildArray[i-2]->uNumSubID == lpChildNode->uNumSubID) {
                 unsigned int j;

                 fprintf (error_out, "error : attempt to insert duplicate node %s(%i) over %s(%i).  ",
                          lpChildNode->lpszTextSubID, lpChildNode->uNumSubID,
                          lpParent->lpChildArray[i-2]->lpszTextSubID,
                          lpParent->lpChildArray[i-2]->uNumSubID);
                 yyerror ("Duplicate OID detected");

                 for (j = i; j <= lpParent->uNumChildren; j++) {
                    lpParent->lpChildArray[j-1] = lpParent->lpChildArray[j];
                 }
                 // let f = lpParent->uNumChildren
                 // Assert (lpParent->lpChildArray[f-2] == lpParent->lpChildArray[f-1])
                 lpParent->uNumChildren--;
                 lpParent->lpChildArray[lpParent->uNumChildren] = NULL; /* clear the duplicate pointer */
                 lpParent->lpChildArray =
                    (lpTreeNode *) realloc (
                                      lpParent->lpChildArray,
                                      lpParent->uNumChildren *
                                      sizeof (lpTreeNode *));
                 break;
               } else {
                 // we want to move i-2 to i-1
                 lpParent->lpChildArray[i-1] = lpParent->lpChildArray[i-2];
               }
            }
            // assert the else clause happened exactly once
            // assert (lpChildNode->lpParent != NULL);
         } else {
            fprintf (error_out,"InsertChildNode: realloc #1 failed!\n");
         }

      }
   }
}

void PrintTree (lpTreeNode lpRoot, unsigned int nIndent)
{
   unsigned int i;

   if (NULL == lpRoot) {
      return;
   } else {
      for (i = 0; i < nIndent; i++) {
         printf (" ");
      }
      printf ("%i  %s\n", lpRoot->uNumSubID, lpRoot->lpszTextSubID);
      for (i = 0; i < lpRoot->uNumChildren; i++) {
         PrintTree(lpRoot->lpChildArray[i], nIndent+2);
      }
   }
}

//-------------------------------- END --------------------------------------

