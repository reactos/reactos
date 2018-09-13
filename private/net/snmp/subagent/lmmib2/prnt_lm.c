/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    prnt_lm.c

Abstract:

    This file contains the routines which actually call Lan Manager and
    retrieve the contents of the print queue table, including cacheing.

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
#include <winspool.h>
#endif

#include <tchar.h>
#include <string.h>
#include <search.h>
#include <stdlib.h>
#include <time.h>
//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------


#include "mib.h"
#include "mibfuncs.h"
#include "prnt_tbl.h"
#include "lmcache.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define SafeBufferFree(x)	if(NULL != x) NetApiBufferFree( x )
#define SafeFree(x)             if(NULL != x) SnmpUtilMemFree( x )

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------


int __cdecl prnt_entry_cmp(
       IN const PRINTQ_ENTRY *A,
       IN const PRINTQ_ENTRY *B
       ) ;

void build_prnt_entry_oids( );

//--------------------------- PUBLIC PROCEDURES -----------------------------


//
// MIB_prnt_lmget
//    Retrieve print queue table information from Lan Manager.
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
SNMPAPI MIB_prntq_lmget(
	   )

{

DWORD entriesread;
DWORD totalentries;
DWORD bytesNeeded;
LPBYTE bufptr;
unsigned lmCode;
unsigned i;
PRINTER_INFO_2 *DataTable;
PRINTQ_ENTRY *MIB_PrintQTableElement ;
int First_of_this_block;
time_t curr_time ;
BOOL result;
SNMPAPI nResult = SNMPAPI_NOERROR;


   time(&curr_time);	// get the time

   //
   //
   // If cached, return piece of info.
   //
   //

   if((NULL != cache_table[C_PRNT_TABLE].bufptr) &&
      (curr_time <
    	(cache_table[C_PRNT_TABLE].acquisition_time
        	 + cache_expire[C_PRNT_TABLE]              ) ) )
   	{ // it has NOT expired!
     	
     	goto Exit ; // the global table is valid
	
	}
	
     //
     // remember to free the existing data
     //

     MIB_PrintQTableElement = MIB_PrintQTable.Table ;

     // iterate over the whole table
     for(i=0; i<MIB_PrintQTable.Len ;i++)
     {
     	// free any alloc'ed elements of the structure
     	SnmpUtilOidFree(&(MIB_PrintQTableElement->Oid));
     	SafeFree(MIB_PrintQTableElement->svPrintQName.stream);
     	
	MIB_PrintQTableElement ++ ;  // increment table entry
     }
     SafeFree(MIB_PrintQTable.Table) ;	// free the base Table
     MIB_PrintQTable.Table = NULL ;	// just for safety
     MIB_PrintQTable.Len = 0 ;		// just for safety


   //
   //
   // Do network call to gather information and put it in a nice array
   //
   //

   	
#if 0 // This is done above
   // init the length
   MIB_PrintQTable.Len = 0;
#endif


#if 0
   lmCode =
	NetShareEnum( 	"",			// local server
	2,			// level 2, no admin priv.
	&bufptr,		// data structure to return
	MAX_PREFERRED_LENGTH,
	&entriesread,
	&totalentries,
	NULL
	);
#endif


    // call it with zero length buffer to get the size
    //
    result = EnumPrinters(
                    PRINTER_ENUM_SHARED |
                    PRINTER_ENUM_LOCAL,     // what type to enum
                    NULL,                   // local server
                    2,                      // level
                    NULL,                   // where to put it
                    0,                      // max of above
                    &bytesNeeded,           // additional bytes req'd
                    &entriesread );         // how many we got this time



    bufptr = SnmpUtilMemAlloc(bytesNeeded); // SnmpUtilMemAlloc the buffer
    if(NULL==bufptr) goto Exit ;      // error, get out with 0 table

#if 0
    if( !result ){
        i = GetLastError();
        SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: LastError after EnumPrinters = %u\n", i ));
    }

    if( result && (ERROR_INSUFFICIENT_BUFFER == GetLastError()) ) {
        // then read the rest of it

        // call it again
        result = EnumPrinters(
                        PRINTER_ENUM_SHARED |
                        PRINTER_ENUM_LOCAL,     // what type to enum
                        NULL,                   // local server
                        2,                      // level
                        bufptr,                 // where to put it
                        bytesNeeded,// max of above
                        &bytesNeeded,           // additional bytes req'd
                        &entriesread );     // how many we got this time


    }
#else


    if((ERROR_INSUFFICIENT_BUFFER == GetLastError()) ) {
        // then read the rest of it

        // call it again
        result = EnumPrinters(
                        PRINTER_ENUM_SHARED |
                        PRINTER_ENUM_LOCAL,     // what type to enum
                        NULL,                   // local server
                        2,                      // level
                        bufptr,                 // where to put it
                        bytesNeeded,// max of above
                        &bytesNeeded,           // additional bytes req'd
                        &entriesread );     // how many we got this time


    }
#endif

    if (!result) {
       // Signal error
       nResult = SNMPAPI_ERROR;
       goto Exit;
#if 0
        return;     // got an error, return empty table
#endif
    }


    DataTable = (PRINTER_INFO_2 *) bufptr ;

   	
   	if(0 == MIB_PrintQTable.Len) {  // 1st time, alloc the whole table
   		// alloc the table space
                MIB_PrintQTable.Table = SnmpUtilMemAlloc(entriesread *
   						sizeof(PRINTQ_ENTRY) );
   	}
	
	MIB_PrintQTableElement = MIB_PrintQTable.Table  ;
	
   	for(i=0; i<entriesread; i++) {  // once for each entry in the buffer
   		
   		// increment the entry number
   		
   		MIB_PrintQTable.Len ++;
   		
   		// Stuff the data into each item in the table
   		
   		// client name
   		MIB_PrintQTableElement->svPrintQName.dynamic = TRUE;
   		
		#ifdef UNICODE
		if (SnmpUtilUnicodeToUTF8(
			&MIB_PrintQTableElement->svPrintQName.stream,
   			DataTable->pPrinterName,
			TRUE))
        {
            MIB_PrintQTableElement->svPrintQName.stream = NULL;
            MIB_PrintQTableElement->svPrintQName.length = 0;
        }
        else
        {
            MIB_PrintQTableElement->svPrintQName.length = 
                strlen (MIB_PrintQTableElement->svPrintQName.stream);
        }
		#else
        MIB_PrintQTableElement->svPrintQName.stream = SnmpUtilMemAlloc (
   				strlen( DataTable->pPrinterName ) + 1 ) ;
   		MIB_PrintQTableElement->svPrintQName.length =
   				strlen( DataTable->pPrinterName ) ;

   		memcpy(	MIB_PrintQTableElement->svPrintQName.stream,
   			DataTable->pPrinterName,
   			strlen( DataTable->pPrinterName ) ) ;
   		#endif
   		
   		// number of connections
   		MIB_PrintQTableElement->svPrintQNumJobs =
   			DataTable->cJobs;
   		
     		
		MIB_PrintQTableElement ++ ;  // and table entry
	
   	   DataTable ++ ;  // advance pointer to next sess entry in buffer
		
   	} // for each entry in the data table
   	
   	// free all of the printer enum data
    if(NULL!=bufptr)                // free the table
        SnmpUtilMemFree( bufptr ) ;
	
   	


    // iterate over the table populating the Oid field
    build_prnt_entry_oids();

   // Sort the table information using MSC QuickSort routine
   qsort( &MIB_PrintQTable.Table[0], MIB_PrintQTable.Len,
          sizeof(PRINTQ_ENTRY), prnt_entry_cmp );

   //
   //
   // Cache table
   //
   //

   if(0 != MIB_PrintQTable.Len) {
   	
   	cache_table[C_PRNT_TABLE].acquisition_time = curr_time ;

   	cache_table[C_PRNT_TABLE].bufptr = bufptr ;
   }

   //
   //
   // Return piece of information requested in global table
   //
   //

Exit:
   return nResult;
} // MIB_prnt_get

//
// MIB_prnt_cmp
//    Routine for sorting the session table.
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
int __cdecl prnt_entry_cmp(
       IN const PRINTQ_ENTRY *A,
       IN const PRINTQ_ENTRY *B
       )

{
   // Compare the OID's
   return SnmpUtilOidCmp( (AsnObjectIdentifier *)&A->Oid,
                       (AsnObjectIdentifier *)&B->Oid );
} // MIB_prnt_cmp


//
//    None.
//
void build_prnt_entry_oids(
       )

{
AsnOctetString OSA ;
PRINTQ_ENTRY *PrintQEntry ;
unsigned i;

// start pointer at 1st guy in the table
PrintQEntry = MIB_PrintQTable.Table ;

// now iterate over the table, creating an oid for each entry
for( i=0; i<MIB_PrintQTable.Len ; i++)  {
   // for each entry in the session table

   OSA.stream = PrintQEntry->svPrintQName.stream ;
   OSA.length =  PrintQEntry->svPrintQName.length ;
   OSA.dynamic = TRUE;

   // Make the entry's OID from string index
   MakeOidFromStr( &OSA, &PrintQEntry->Oid );

   PrintQEntry++; // point to the next guy in the table

   } // for

} // build_prnt_entry_oids
//-------------------------------- END --------------------------------------
