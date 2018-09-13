/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    dlog_lm.c

Abstract:

    This file contains MIB_dlog_lmget, which actually call lan manager
    for the dloge table, copies it into structures, and sorts it to
    return ready to use by the higher level functions.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#ifdef WIN32
#include <windows.h>
#include <lm.h>
#endif

#include <string.h>
#include <search.h>
#include <stdlib.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include "mib.h"
#include "mibfuncs.h"
#include "dlog_tbl.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

int dlog_entry_cmp(
       IN DOM_LOGON_ENTRY *A,
       IN DOM_LOGON_ENTRY *B
       ) ;

void build_dlog_entry_oids( );

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------


//
// MIB_dlog_lmget
//    Retrieve dlogion table information from Lan Manager.
//    If not cached, sort it and then
//    cache it.
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
SNMPAPI MIB_dlogons_lmget(
	   )

{
SNMPAPI nResult = SNMPAPI_NOERROR;
#if 0
DWORD entriesread;
DWORD totalentries;
LPBYTE bufptr;
unsigned lmCode;
unsigned i;
SHARE_INFO_2 *DataTable;
DOM_LOGON_ENTRY *MIB_DomLogonTableElement ;
int First_of_this_block;
DWORD resumehandle=0;

   //
   //
   // If cached, return piece of info.
   //
   //

   //
   //
   // Do network call to gather information and put it in a nice array
   //
   //

   // free the old table  LOOK OUT!!
   	
   // init the length
   MIB_DomLogonTable.Len = 0;
   First_of_this_block = 0;
   	
   do {  //  as long as there is more data to process

	lmCode =
    	NetShareEnum(NULL,          // local server
                2,                  // level 2,
                &bufptr,            // data structure to return
                MAX_PREFERRED_LENGTH,
                &entriesread,
                &totalentries,
                &resumehandle       //  resume handle
                );

        //
        // Filter out all the Admin shares (name ending with $).
        //
        AdminFilter(2,&entriesread,bufptr);


    DataTable = (SHARE_INFO_2 *) bufptr ;

    if((NERR_Success == lmCode) || (ERROR_MORE_DATA == lmCode))
    	{  // valid so process it, otherwise error
   	
   	if(0 == MIB_DomLogonTable.Len) {  // 1st time, alloc the whole table
   		// alloc the table space
                MIB_DomLogonTable.Table = SnmpUtilMemAlloc(totalentries *
   						sizeof(DOM_LOGON_ENTRY) );
   	}
	
	MIB_DomLogonTableElement = MIB_DomLogonTable.Table + First_of_this_block ;
	
   	for(i=0; i<entriesread; i++) {  // once for each entry in the buffer
   		// increment the entry number
   		
   		MIB_DomLogonTable.Len ++;
   		
   		// Stuff the data into each item in the table
   		
   		// dloge name
                MIB_DomLogonTableElement->svShareName.stream = SnmpUtilMemAlloc (
   				strlen( DataTable->shi2_netname ) ) ;
   		MIB_DomLogonTableElement->svShareName.length =
   				strlen( DataTable->shi2_netname ) ;
		MIB_DomLogonTableElement->svShareName.dynamic = TRUE;
   		memcpy(	MIB_DomLogonTableElement->svShareName.stream,
   			DataTable->shi2_netname,
   			strlen( DataTable->shi2_netname ) ) ;
   		
   		// Share Path
                MIB_DomLogonTableElement->svSharePath.stream = SnmpUtilMemAlloc (
   				strlen( DataTable->shi2_path ) ) ;
   		MIB_DomLogonTableElement->svSharePath.length =
   				strlen( DataTable->shi2_path ) ;
		MIB_DomLogonTableElement->svSharePath.dynamic = TRUE;
   		memcpy(	MIB_DomLogonTableElement->svSharePath.stream,
   			DataTable->shi2_path,
   			strlen( DataTable->shi2_path ) ) ;
   		
   		
   		// Share Comment/Remark
                MIB_DomLogonTableElement->svShareComment.stream = SnmpUtilMemAlloc (
   				strlen( DataTable->shi2_remark ) ) ;
   		MIB_DomLogonTableElement->svShareComment.length =
   				strlen( DataTable->shi2_remark ) ;
		MIB_DomLogonTableElement->svShareComment.dynamic = TRUE;
   		memcpy(	MIB_DomLogonTableElement->svShareComment.stream,
   			DataTable->shi2_remark,
   			strlen( DataTable->shi2_remark ) ) ;
   		
   		
   		DataTable ++ ;  // advance pointer to next dlog entry in buffer
		MIB_DomLogonTableElement ++ ;  // and table entry
		
   	} // for each entry in the data table
   	
   	// indicate where to start adding on next pass, if any
   	First_of_this_block = i ;
   	
       	} // if data is valid to process
    else
       {
       // Signal error
       nResult = SNMPAPI_ERROR;
       goto Exit;
       }

    } while (ERROR_MORE_DATA == lmCode) ;

    // iterate over the table populating the Oid field
    build_dlog_entry_oids();

   // Sort the table information using MSC QuickSort routine
   qsort( &MIB_DomLogonTable.Table[0], MIB_DomLogonTable.Len,
          sizeof(DOM_LOGON_ENTRY), dlog_entry_cmp );

   //
   //
   // Cache table
   //
   //

   //
   //
   // Return piece of information requested
   //
   //

Exit:
#endif
   return nResult;

} // MIB_dlog_get

//
// MIB_dlog_cmp
//    Routine for sorting the dlogion table.
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
int dlog_entry_cmp(
       IN DOM_LOGON_ENTRY *A,
       IN DOM_LOGON_ENTRY *B
       )

{
   // Compare the OID's
   return SnmpUtilOidCmp( &A->Oid, &B->Oid );
} // MIB_dlog_cmp


//
//    None.
//
void build_dlog_entry_oids(
       )

{
#if 0
AsnOctetString OSA ;
char StrA[MIB_SHARE_NAME_LEN];
DOM_LOGON_ENTRY *ShareEntry ;
unsigned i;

// start pointer at 1st guy in the table
ShareEntry = MIB_DomLogonTable.Table ;

// now iterate over the table, creating an oid for each entry
for( i=0; i<MIB_DomLogonTable.Len ; i++)  {
   // for each entry in the dlogion table

   // Make string to use as index
   memcpy( StrA, ShareEntry->svShareName.stream,
                 ShareEntry->svShareName.length );

   OSA.stream = StrA ;
   OSA.length =  ShareEntry->svShareName.length ;
   OSA.dynamic = FALSE;

   // Make the entry's OID from string index
   MakeOidFromStr( &OSA, &ShareEntry->Oid );

   ShareEntry++; // point to the next guy in the table

   } // for
#endif
} // build_dlog_entry_oids
//-------------------------------- END --------------------------------------
