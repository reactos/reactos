/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    srvc_lm.c

Abstract:

    This file contains MIB_srvc_lmget, which actually call lan manager
    for the srvce table, copies it into structures, and sorts it to
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

#include <tchar.h>
#include <string.h>
#include <search.h>
#include <stdlib.h>
#include <time.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------


#include "mib.h"
#include "mibfuncs.h"
#include "srvc_tbl.h"
#include "lmcache.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define SafeBufferFree(x)	if(NULL != x) NetApiBufferFree( x )
#define SafeFree(x)             if(NULL != x) SnmpUtilMemFree( x )

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

int __cdecl srvc_entry_cmp(
       IN const SRVC_ENTRY *A,
       IN const SRVC_ENTRY *B
       ) ;

void build_srvc_entry_oids( );

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------


//
// MIB_srvc_lmget
//    Retrieve srvcion table information from Lan Manager.
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
SNMPAPI MIB_srvcs_lmget(
	   )

{
DWORD entriesread;
DWORD totalentries;
LPBYTE bufptr;
unsigned lmCode;
unsigned i;
SERVICE_INFO_2 *DataTable;
SRVC_ENTRY *MIB_SrvcTableElement ;
int First_of_this_block;
time_t curr_time ;
SNMPAPI nResult = SNMPAPI_NOERROR;
DWORD resumehandle=0;
#ifdef UNICODE
LPSTR stream;
#endif


   time(&curr_time);	// get the time


   //
   //
   // If cached, return piece of info.
   //
   //


   if((NULL != cache_table[C_SRVC_TABLE].bufptr) &&
      (curr_time <
    	(cache_table[C_SRVC_TABLE].acquisition_time
        	 + cache_expire[C_SRVC_TABLE]              ) ) )
   	{ // it has NOT expired!
     	
     	goto Exit ; // the global table is valid
	
	}
	
   //
   //
   // Do network call to gather information and put it in a nice array
   //
   //

   	
     //
     // remember to free the existing data
     //

     MIB_SrvcTableElement = MIB_SrvcTable.Table ;

     // iterate over the whole table
     for(i=0; i<MIB_SrvcTable.Len ;i++)
     {
     	// free any alloc'ed elements of the structure
     	SnmpUtilOidFree(&(MIB_SrvcTableElement->Oid));
     	SafeFree(MIB_SrvcTableElement->svSvcName.stream);
     	
	MIB_SrvcTableElement ++ ;  // increment table entry
     }
     SafeFree(MIB_SrvcTable.Table) ;	// free the base Table
     MIB_SrvcTable.Table = NULL ;	// just for safety
     MIB_SrvcTable.Len = 0 ;		// just for safety

   First_of_this_block = 0;
   	
   do {  //  as long as there is more data to process

	       lmCode =
            NetServiceEnum( NULL,       // local server
                    2,                  // level 2
                    &bufptr,            // data structure to return
                    MAX_PREFERRED_LENGTH,
                    &entriesread,
                    &totalentries,
                    &resumehandle       //  resume handle
	       			);

    DataTable = (SERVICE_INFO_2 *) bufptr ;

    if((NERR_Success == lmCode) || (ERROR_MORE_DATA == lmCode))
    	{  // valid so process it, otherwise error
   	
   	if(0 == MIB_SrvcTable.Len) {  // 1st time, alloc the whole table
   		// alloc the table space
                MIB_SrvcTable.Table = SnmpUtilMemAlloc(totalentries *
   						sizeof(SRVC_ENTRY) );
   	}
	
	MIB_SrvcTableElement = MIB_SrvcTable.Table + First_of_this_block ;
	
   	for(i=0; i<entriesread; i++) {  // once for each entry in the buffer
   		// increment the entry number
   		
   		MIB_SrvcTable.Len ++;
   		
   		// Stuff the data into each item in the table
   		
   		MIB_SrvcTableElement->svSvcName.dynamic = TRUE;

		#ifdef UNICODE
		if (SnmpUtilUnicodeToUTF8(
			&MIB_SrvcTableElement->svSvcName.stream,
   			DataTable->svci2_display_name,
			TRUE))
        {
            MIB_SrvcTableElement->svSvcName.length = 0;
            MIB_SrvcTableElement->svSvcName.stream = NULL;
        }
        else
        {
            MIB_SrvcTableElement->svSvcName.length =
                strlen(MIB_SrvcTableElement->svSvcName.stream);
        }

		#else
   		// service name
        MIB_SrvcTableElement->svSvcName.stream = SnmpUtilMemAlloc (
   				strlen( DataTable->svci2_display_name ) + 1) ;

        memcpy(	MIB_SrvcTableElement->svSvcName.stream,
   			DataTable->svci2_display_name,
   			strlen( DataTable->svci2_display_name ) + 1) ;

        MIB_SrvcTableElement->svSvcName.length =
   				strlen( MIB_SrvcTableElement->svSvcName.stream )) ;
   		#endif
   		
		MIB_SrvcTableElement->svSvcInstalledState =
   			(DataTable->svci2_status & 0x03) + 1;
		MIB_SrvcTableElement->svSvcOperatingState =
   			((DataTable->svci2_status>>2) & 0x03) + 1;
		MIB_SrvcTableElement->svSvcCanBeUninstalled =
   			((DataTable->svci2_status>>4) & 0x01) + 1;
		MIB_SrvcTableElement->svSvcCanBePaused =
   			((DataTable->svci2_status>>5) & 0x01) + 1;
   		
   		DataTable ++ ;  // advance pointer to next srvc entry in buffer
		MIB_SrvcTableElement ++ ;  // and table entry
		
   	} // for each entry in the data table
   	
   	
   	// free all of the lan man data
	SafeBufferFree( bufptr ) ;
	
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
    build_srvc_entry_oids();

   // Sort the table information using MSC QuickSort routine
   qsort( &MIB_SrvcTable.Table[0], MIB_SrvcTable.Len,
          sizeof(SRVC_ENTRY), srvc_entry_cmp );

   //
   //
   // Cache table
   //
   //


   if(0 != MIB_SrvcTable.Len) {
   	
   	cache_table[C_SRVC_TABLE].acquisition_time = curr_time ;

   	cache_table[C_SRVC_TABLE].bufptr = bufptr ;
   }

   //
   //
   // Return piece of information requested
   //
   //
Exit:
   return nResult;
} // MIB_srvc_get

//
// MIB_srvc_cmp
//    Routine for sorting the srvcion table.
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
int __cdecl srvc_entry_cmp(
       IN const SRVC_ENTRY *A,
       IN const SRVC_ENTRY *B
       )

{
   // Compare the OID's
   return SnmpUtilOidCmp( (AsnObjectIdentifier *)&A->Oid,
                       (AsnObjectIdentifier *)&B->Oid );
} // MIB_srvc_cmp


//
//    None.
//
void build_srvc_entry_oids(
       )

{
AsnOctetString OSA ;
SRVC_ENTRY *SrvcEntry ;
unsigned i;

// start pointer at 1st guy in the table
SrvcEntry = MIB_SrvcTable.Table ;

// now iterate over the table, creating an oid for each entry
for( i=0; i<MIB_SrvcTable.Len ; i++)  {
   // for each entry in the srvc table

   OSA.stream =  SrvcEntry->svSvcName.stream;
   OSA.length =  SrvcEntry->svSvcName.length;
   OSA.dynamic = FALSE;

   // Make the entry's OID from string index
   MakeOidFromStr( &OSA, &SrvcEntry->Oid );

   SrvcEntry++; // point to the next guy in the table

   } // for
} // build_srvc_entry_oids
//-------------------------------- END --------------------------------------
