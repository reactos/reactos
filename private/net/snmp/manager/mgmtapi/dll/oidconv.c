/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    oidconv.c

Abstract:

    Routines to manage conversions between OID descriptions and numerical OIDs.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/

//--------------------------- WINDOWS DEPENDENCIES --------------------------

#include <windows.h>

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <snmp.h>
#include <snmputil.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include "mibcc.h"
#include "mibtree.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include "oidconv.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

/* name to used when converting OID <--> TEXT */
LPSTR lpInputFileName = "mib.bin";

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define FILENODE_SIZE     sizeof(T_FILE_NODE)
#define OID_PREFIX_LEN    (sizeof MIB_Prefix / sizeof(UINT))
#define STR_PREFIX_LEN    (strlen(MIB_StrPrefix))

#define SEEK_SET  0
#define SEEK_CUR  1

//--------------------------- PRIVATE STRUCTS -------------------------------

   //****************************************************************
   //
   //                     Record structure in file
   //
   //    These are the necessary fields to process a conversion request.
   //    When a request is made, the MIB file is searched sequentially
   //    matching subid's.  The field, lNextOffset, is an offset from the
   //    current file position to the current nodes next sibling.
   //
   //    The text subid for each node is stored directly after the
   //    T_FILE_NODE structure in the file.  Its length is stored in the
   //    field, uStrLen.
   //
   //    This is done because there are no limits placed on the size
   //    of a text subid.  Hence, when the T_FILE_NODE structure is
   //    read from the MIB file, the field, lpszTextSubID is not valid.
   //    The field will eventually point to the storage allocated to
   //    hold the text subid.
   //
   //    The order of the nodes in the file is the same as if the MIB
   //    tree was traversed in a "pre-order" manner.
   //
   //****************************************************************

typedef struct _FileNode {
   long                 lNextOffset;      // This field must remain first
   UINT                 uNumChildren;
   UINT                 uStrLen;
   LPSTR                lpszTextSubID;
   UINT                 uNumSubID;
} T_FILE_NODE;

//--------------------------- PRIVATE VARIABLES -----------------------------

LPSTR MIB_StrPrefix = "iso.org.dod.internet.mgmt.mib-2";

UINT MIB_Prefix[] = { 1, 3, 6, 1, 2, 1 };
AsnObjectIdentifier MIB_OidPrefix = { OID_PREFIX_LEN, MIB_Prefix };

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//
// GetNextNode
//    Reads the next record from MIB file into a FILENODE structure.
//
// Notes:
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    None.
//
SNMPAPI GetNextNode(
	   IN HFILE fh,
           OUT T_FILE_NODE * Node
	   )

{
SNMPAPI nResult;


   Node->lpszTextSubID = NULL;

   // Read in node
   if ( FILENODE_SIZE != _lread(fh, (LPSTR)Node, FILENODE_SIZE) )
      {
      nResult = SNMPAPI_ERROR;
      goto Exit;
      }

   // Alloc space for string
   if ( NULL ==
        (Node->lpszTextSubID = SnmpUtilMemAlloc((1+Node->uStrLen) * sizeof(char))) )
      {
      nResult = SNMPAPI_ERROR;
      goto Exit;
      }

   // Read in subid string
   if ( Node->uStrLen != _lread(fh, Node->lpszTextSubID, Node->uStrLen) )
      {
      nResult = SNMPAPI_ERROR;
      goto Exit;
      }

   // NULL terminate the text sub id
   Node->lpszTextSubID[Node->uStrLen] = '\0';

   nResult = SNMPAPI_NOERROR;

Exit:
   if ( SNMPAPI_ERROR == nResult )
      {
      SnmpUtilMemFree( Node->lpszTextSubID );
      }

   return nResult;
} // GetNextNode



//
// WriteNode
//    Writes the node to the MIB file.
//
// Notes:
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    None.
//
SNMPAPI WriteNode(
	   IN HFILE fh,
           IN T_FILE_NODE * Node
	   )

{
SNMPAPI nResult;
T_FILE_NODE LocalNodeCopy;

   // make a copy of the node so we can clean up any pointers
   LocalNodeCopy.lNextOffset = Node->lNextOffset;
   LocalNodeCopy.uNumChildren = Node->uNumChildren;
   LocalNodeCopy.uStrLen = Node->uStrLen;
   LocalNodeCopy.lpszTextSubID = NULL;  /* don't write pointers to disk */
   LocalNodeCopy.uNumSubID = Node->uNumSubID;

   // Write Node portion
   if ( FILENODE_SIZE != _lwrite(fh, (LPSTR)&LocalNodeCopy, FILENODE_SIZE) )
      {
      nResult = SNMPAPI_ERROR;
      goto Exit;
      }

   // Now write out what the pointers pointed to.
   // Save text subid
   if ( Node->uStrLen != _lwrite(fh, Node->lpszTextSubID, Node->uStrLen) )
      {
      nResult = SNMPAPI_ERROR;
      goto Exit;
      }

   nResult = SNMPAPI_NOERROR;

Exit:
   return nResult;
} // WriteNode



//
// SkipSubTree
//    Frees a FILENODE and all information contained in it.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
SNMPAPI SkipSubTree(
           IN HFILE fh,
           IN T_FILE_NODE *Node
	   )

{
SNMPAPI     nResult;


   // Skip entire subtree
   if ( -1 == _llseek(fh, Node->lNextOffset, SEEK_CUR) )
      {
      nResult = SNMPAPI_ERROR;
      goto Exit;
      }

   nResult = SNMPAPI_NOERROR;

Exit:
   return nResult;
} // SkipSubTree

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// SnmpMgrMIB2Disk
//    Writes the MIB contained in memory to a disk file.
//
// Notes:
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    None.
//
SNMPAPI SnmpMgrMIB2Disk(
           IN lpTreeNode lpTree, // Pointer to MIB root
           IN LPSTR lpOutputFileName // file name of mib file
	   )

{
   // STACK structure for writing MIB to file
typedef struct _Stack {
   lpTreeNode      lpNode;
   UINT            uNumChildrenToResolve; /* counter of lpNode->uNumChildren */
   long            lFilePos;
   struct _Stack * lpNext;
} T_STACK;

T_FILE_NODE   FileNode;
T_STACK     * lpFileTop = NULL;
T_STACK     * lpResolveTop = NULL;
T_STACK     * lpTemp = NULL;
lpTreeNode    Node;
OFSTRUCT      of;
HFILE         fh;
UINT          I;
SNMPAPI       nResult;


   // Open file and check for errors
   if ( -1 == (fh =
        OpenFile(lpOutputFileName, &of, OF_CREATE|OF_WRITE|OF_SHARE_EXCLUSIVE)) )
      {
      nResult = SNMPAPI_ERROR;
      goto Exit;
      }

   // Initialize file STACK.  The top, and only entry, is root
   if ( NULL == (lpFileTop = SnmpUtilMemAlloc(sizeof(T_STACK))) )
      {
      nResult = SNMPAPI_ERROR;
      goto Exit;
      }
   lpFileTop->lpNode   = lpTree;
   lpFileTop->uNumChildrenToResolve = lpFileTop->lpNode->uNumChildren;
   lpFileTop->lFilePos = 0;
   lpFileTop->lpNext   = NULL;   // terminates list

   // Initialize resolve STACK.  Starts empty
   lpResolveTop = NULL;

   // Keep processing until empty next list
   while ( NULL != lpFileTop )
      {
      // Pop node from FILE stack
      Node      = lpFileTop->lpNode;
      Node->uNumChildren = lpFileTop->uNumChildrenToResolve;
      lpTemp    = lpFileTop;
      lpFileTop = lpFileTop->lpNext;

      // Push node onto RESOLVE stack
      lpTemp->lpNext = lpResolveTop;
      lpResolveTop   = lpTemp;
      lpResolveTop->uNumChildrenToResolve = lpResolveTop->lpNode->uNumChildren;

      // Convert tree node to file node
      FileNode.uNumChildren  = Node->uNumChildren;
      FileNode.uStrLen       = strlen( Node->lpszTextSubID );
      FileNode.lpszTextSubID = Node->lpszTextSubID;
      FileNode.uNumSubID     = Node->uNumSubID;
      FileNode.lNextOffset   = 0;

      // Save position in file on RESOLVE stack
      if ( -1 == (lpResolveTop->lFilePos = _llseek(fh, 0, SEEK_CUR)) )
         {
         nResult = SNMPAPI_ERROR;
         goto Exit;
         }

      // Save node to file
      if ( SNMPAPI_ERROR == WriteNode(fh, &FileNode) )
         {
         nResult = SNMPAPI_ERROR;
         goto Exit;
         }

      // Push children on stack in reverse order
      I = Node->uNumChildren;
      while ( I )
	 {
         if ( NULL == (lpTemp = SnmpUtilMemAlloc(sizeof(T_STACK))) )
            {
	    nResult = SNMPAPI_ERROR;
	    goto Exit;
            }

         lpTemp->lpNode = Node->lpChildArray[--I];
         lpTemp->uNumChildrenToResolve=lpTemp->lpNode->uNumChildren;
         lpTemp->lpNext = lpFileTop;
         lpFileTop = lpTemp;
	 } // while

      // Test top of RESOLVE stack to see if node needs resolving
      while ( NULL != lpResolveTop && 0 == lpResolveTop->uNumChildrenToResolve)
         {
         long  lSavePos;
         long  lOffset;


         // Save file position
         if ( -1 == (lSavePos = _llseek(fh, 0, SEEK_CUR)) )
            {
            nResult = SNMPAPI_ERROR;
            goto Exit;
            }

         // Calculate offset
         lOffset = lSavePos - lpResolveTop->lFilePos -
                   FILENODE_SIZE - strlen(lpResolveTop->lpNode->lpszTextSubID);

         // Only write if offset is positive
         if ( lOffset )
            {
            // Position file pointer to beginning of record to update
            if ( -1 == _llseek(fh, lpResolveTop->lFilePos, SEEK_SET) )
               {
               nResult = SNMPAPI_ERROR;
               goto Exit;
               }

            // Write to file position field
            //    Remember this field must be first in structure
            if ( sizeof(long) != _lwrite(fh, (LPSTR)&lOffset, sizeof(long)) )
               {
               nResult = SNMPAPI_ERROR;
               goto Exit;
               }

            // Restore file position
            if ( -1 == _llseek(fh, lSavePos, SEEK_SET) )
               {
               nResult = SNMPAPI_ERROR;
               goto Exit;
               }
            }

         // Pop RESOLVE stack
         lpTemp       = lpResolveTop;
         lpResolveTop = lpResolveTop->lpNext;
         SnmpUtilMemFree( lpTemp );
         } // while

      // Decrement the child pointer if any nodes on RESOLVE stack
      if ( NULL != lpResolveTop )
         {
         lpResolveTop->uNumChildrenToResolve --;
         }
      } // while

   nResult = SNMPAPI_NOERROR;

Exit:
   // Close file name if opened successfully
   if ( -1 != fh )
     {
     _lclose( fh );
     }

   // Free FILE stack if alloc'ed
   while ( NULL != lpFileTop )
      {
      lpTemp = lpFileTop;
      lpFileTop = lpFileTop->lpNext;
      }

   // Free RESOLVE stack if alloc'ed
   while ( NULL != lpResolveTop )
      {
      lpTemp = lpResolveTop;
      lpResolveTop = lpResolveTop->lpNext;
      }

    if (NULL != lpTemp) {
        SnmpUtilMemFree( lpTemp );
        lpTemp = NULL;
    }

   return nResult;
} // SnmpMgrMIB2Disk



//
// SnmpMgrOid2Text
//    Converts an OID to its textual description.
//
// Notes:
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    None.
//
SNMPAPI SnmpMgrOid2Text(
           IN AsnObjectIdentifier *Oid, // Pointer to OID to convert
	   OUT LPSTR *lpszTextOid       // Resulting text OID
	   )

{
T_FILE_NODE  Node;
OFSTRUCT     of;
HFILE        fh;
UINT         Siblings;
UINT         OidSubId;
UINT         uTxtOidLen;
BOOL         bFound;
BOOL         bPartial;
BOOL         bDot;
SNMPAPI      nResult;

   // OPENISSUE - this code does not generate errors if subid 0 is embeded
   // OPENISSUE - opening file every time could be a performance issue
   // OPENISSUE - optimization of file access could improve performance

   *lpszTextOid = NULL;

   // Open file and check for errors
   if ( -1 == (fh = OpenFile(lpInputFileName, &of, OF_READ|OF_SHARE_DENY_WRITE)) )
      {
      nResult = SNMPAPI_ERROR;
      goto Exit;
      }

   // Test for MIB prefix
   bDot = !( bPartial = OID_PREFIX_LEN < Oid->idLength &&
                        !SnmpUtilOidNCmp(Oid, &MIB_OidPrefix, OID_PREFIX_LEN) );

   // Loop until conversion is finished
   OidSubId          = 0;
   uTxtOidLen        = 0;
   Node.uNumChildren = 1;
   while ( OidSubId < Oid->idLength )
      {
      // Init to not found on this level
      bFound   = FALSE;
      Siblings = Node.uNumChildren;

      // While their are siblings and the sub id is not found keep looking
      while ( Siblings && !bFound )
         {
	 Node.lpszTextSubID = NULL;

	 // Get next sibling
         if ( SNMPAPI_ERROR == GetNextNode(fh, &Node) )
            {
            SnmpUtilMemFree( Node.lpszTextSubID );

            nResult = SNMPAPI_ERROR;
            goto Exit;
            }

         Siblings --;

	 // Compare the numeric subid's
         if ( Oid->ids[OidSubId] == Node.uNumSubID )
	    {
	    bFound = TRUE;

	    // If OID is a partial, then skip prefix subid's
	    if ( OidSubId >= OID_PREFIX_LEN || !bPartial )
	       {
	       // Realloc space for text id - add 2 for '.' and NULL terminator
	       if ( NULL == (*lpszTextOid =
	                     SnmpUtilMemReAlloc(*lpszTextOid,
		                  (uTxtOidLen+Node.uStrLen+2) * sizeof(char))) )
	          {
                  SnmpUtilMemFree( Node.lpszTextSubID );

                  nResult = SNMPAPI_ERROR;
	          goto Exit;
	          }

	       // Add DOT separator
	       if ( bDot )
	          {
	          (*lpszTextOid)[uTxtOidLen] = '.';

	          // Save text subid
	          memcpy( &(*lpszTextOid)[uTxtOidLen+1],
	                  Node.lpszTextSubID, Node.uStrLen+1 );

	          // Update length of text oid - add one for separator
	          uTxtOidLen += Node.uStrLen + 1;
	          }
	       else
	          {
	          bDot = TRUE;

	          // Save text subid
	          memcpy( &(*lpszTextOid)[uTxtOidLen],
	                  Node.lpszTextSubID, Node.uStrLen );

	          // Update length of text oid
	          uTxtOidLen += Node.uStrLen;
	          }
	       }

	    // Skip to next OID subid
	    OidSubId ++;
	    }
	 else
	    {
            // Skip over subtree since not a match
            if ( SNMPAPI_ERROR == SkipSubTree(fh, &Node) )
               {
	       SnmpUtilMemFree( Node.lpszTextSubID );

	       nResult = SNMPAPI_ERROR;
               goto Exit;
               }
	    }

	 // Free the text sub id read
	 SnmpUtilMemFree( Node.lpszTextSubID );
         } // while

      // If no sub id matches
      if ( !bFound )
         {
	 break;
	 }
      } // while

   // Make sure that the entire OID was converted
   while ( OidSubId < Oid->idLength )
      {
      char NumChar[100];


      _itoa( Oid->ids[OidSubId], NumChar, 10 );
      // Realloc space for text id - add 2 for '.' and NULL terminator
      if ( NULL ==
           (*lpszTextOid = SnmpUtilMemReAlloc(*lpszTextOid,
                          (uTxtOidLen+strlen(NumChar)+4) * sizeof(char))) )
         {
	 nResult = SNMPAPI_ERROR;
	 goto Exit;
	 }

      // Add DOT separator
      (*lpszTextOid)[uTxtOidLen] = '.';

      // Save text subid
      memcpy( &(*lpszTextOid)[uTxtOidLen+1], NumChar, strlen(NumChar)+1 );

      // Skip to next OID subid
      OidSubId ++;

      // Update length of text oid - add one for separator
      uTxtOidLen += strlen(NumChar) + 1;
      } // while

   nResult = SNMPAPI_NOERROR;

Exit:
   if ( -1 != fh )
      {
      _lclose( fh );
      }

   if ( SNMPAPI_ERROR == nResult )
      {
      SnmpUtilMemFree( *lpszTextOid );
      *lpszTextOid = NULL;
      }

   return nResult;
} // SnmpMgrOid2Text



//
// SnmpMgrText2Oid
//    Converts an OID text description to its numerical equivalent.
//
// Notes:
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    None.
//
SNMPAPI SnmpMgrText2Oid(
         IN LPSTR lpszTextOid,           // Pointer to text OID to convert
	 IN OUT AsnObjectIdentifier *Oid // Resulting numeric OID
	 )

{
#define DELIMETERS         ".\0"


T_FILE_NODE  Node;
OFSTRUCT     of;
HFILE        fh;
UINT         Siblings;
LPSTR        lpszSubId;
LPSTR        lpszWrkOid = NULL;
BOOL         bFound;
UINT         uSubId;
SNMPAPI      nResult;

   // OPENISSUE - this code does not generate errors if subid 0 is embeded
   // OPENISSUE - opening file every time could be a performance issue
   // OPENISSUE - optimization of file access could improve performance

   // Init. OID structure
   Oid->idLength = 0;
   Oid->ids      = NULL;

   // check for null string and empty string
   if ( NULL == lpszTextOid || '\0' == lpszTextOid[0] )
      {
      fh = -1;

      nResult = SNMPAPI_NOERROR;
      goto Exit;
      }

   // Open file and check for errors
   if ( -1 == (fh = OpenFile(lpInputFileName, &of, OF_READ|OF_SHARE_DENY_WRITE)) )
      {
      nResult = SNMPAPI_ERROR;
      goto Exit;
      }

   // Make working copy of string
   if ( ('.' == lpszTextOid[0]) )
      {
      if ( NULL == (lpszWrkOid = SnmpUtilMemAlloc((strlen(lpszTextOid)+1) * sizeof(char))) )
         {
         nResult = SNMPAPI_ERROR;
         goto Exit;
         }

      strcpy( lpszWrkOid, lpszTextOid+1 );
      }
   else
      {
      if ( NULL ==
           (lpszWrkOid =
            SnmpUtilMemAlloc((strlen(lpszTextOid)+STR_PREFIX_LEN+1+1) * sizeof(char))) )
         {
         nResult = SNMPAPI_ERROR;
         goto Exit;
         }

      strcpy( lpszWrkOid, MIB_StrPrefix );
      lpszWrkOid[STR_PREFIX_LEN] = '.';
      strcpy( &lpszWrkOid[STR_PREFIX_LEN+1], lpszTextOid );
      }

   Node.uNumChildren = 1;
   lpszSubId = strtok( lpszWrkOid, DELIMETERS );

   // Loop until conversion is finished
   while ( NULL != lpszSubId )
      {

      // Init to not found on this level
      bFound   = FALSE;
      Siblings = Node.uNumChildren;

      // Check for imbedded numbers
      if ( isdigit(*lpszSubId) )
         {
         UINT I;


         // Make sure this is a NUMBER without alpha's
         for ( I=0;I < strlen(lpszSubId);I++ )
            {
            if ( !isdigit(lpszSubId[I]) )
               {
	       nResult = SNMPAPI_ERROR;
	       goto Exit;
	       }
	    }

         uSubId = atoi( lpszSubId );
         }
      else
         {
         uSubId = 0;
         }

      // While their are siblings and the sub id is not found keep looking
      while ( Siblings && !bFound )
         {
	 Node.lpszTextSubID = NULL;

	 // Get next sibling
         if ( SNMPAPI_ERROR == GetNextNode(fh, &Node) )
            {

            SnmpUtilMemFree( Node.lpszTextSubID );

            nResult = SNMPAPI_ERROR;
            goto Exit;
            }

         Siblings --;

	 if ( uSubId )
	    {
	    // Compare the numeric subid's
	    if ( Node.uNumSubID == uSubId )
	       {
	       bFound = TRUE;

	       // Add space for new sub id
	       if ( NULL ==
                    (Oid->ids =
                     SnmpUtilMemReAlloc(Oid->ids, (Oid->idLength+1) * sizeof(UINT))) )
	          {
                  SnmpUtilMemFree( Node.lpszTextSubID );

	          nResult = SNMPAPI_ERROR;
	          goto Exit;
	          }

	       // Append this sub id to end of numeric OID
               Oid->ids[Oid->idLength++] = Node.uNumSubID;
	       }
	    }
         else
            {
	    // Compare the text subid's
	    if ( !strcmp(lpszSubId, Node.lpszTextSubID) )
	       {
	       bFound = TRUE;

	       // Add space for new sub id
	       if ( NULL ==
                    (Oid->ids =
                     SnmpUtilMemReAlloc(Oid->ids, (Oid->idLength+1) * sizeof(UINT))) )
	          {
                  SnmpUtilMemFree( Node.lpszTextSubID );

	          nResult = SNMPAPI_ERROR;
	          goto Exit;
	          }

	       // Append this sub id to end of numeric OID
               Oid->ids[Oid->idLength++] = Node.uNumSubID;
	       }
	    }

         // Skip over subtree since not a match
         if ( !bFound && SNMPAPI_ERROR == SkipSubTree(fh, &Node) )
            {
	    SnmpUtilMemFree( Node.lpszTextSubID );

            nResult = SNMPAPI_ERROR;
            goto Exit;
            }

	 // Free the text sub id read
	 SnmpUtilMemFree( Node.lpszTextSubID );
         } // while

      // If no sub id matches
      if ( !bFound )
         {
	 break;
	 }

      // Advance to next sub id
      lpszSubId = strtok( NULL, DELIMETERS );
      } // while

   // Make sure that the entire OID was converted
   while ( NULL != lpszSubId )
      {
      UINT I;


      // Make sure this is a NUMBER without alpha's
      for ( I=0;I < strlen(lpszSubId);I++ )
         {
         if ( !isdigit(lpszSubId[I]) )
            {
	    nResult = SNMPAPI_ERROR;
	    goto Exit;
	    }
	 }

      // Add space for new sub id
      if ( NULL ==
           (Oid->ids = SnmpUtilMemReAlloc(Oid->ids, (Oid->idLength+1) * sizeof(UINT))) )
         {
         SnmpUtilMemFree( Node.lpszTextSubID );

         nResult = SNMPAPI_ERROR;
	 goto Exit;
	 }

      // Append this sub id to end of numeric OID
      Oid->ids[Oid->idLength++] = atoi( lpszSubId );

      // Advance to next sub id
      lpszSubId = strtok( NULL, DELIMETERS );
      } // while


   // it is illegal for an oid to be less than two subidentifiers
   if (Oid->idLength < 2)
       {
       nResult = SNMPAPI_ERROR;
       goto Exit;
       }


   nResult = SNMPAPI_NOERROR;

Exit:
   if ( -1 != fh )
      {
      _lclose( fh );
      }

   if ( SNMPAPI_ERROR == nResult )
      {
      SnmpUtilOidFree( Oid );
      }

   if ( NULL != lpszWrkOid ) {
        SnmpUtilMemFree ( lpszWrkOid );
   }

   return nResult;
} // SnmpMgrText2Oid

//------------------------------- END ---------------------------------------

