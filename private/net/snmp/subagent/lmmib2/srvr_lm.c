/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    srvr_lm.c

Abstract:

    This file contains the routines which actually call Lan Manager and
    retrieve the contents of the domain server table, including cacheing.

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
#include "srvr_tbl.h"
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

int __cdecl srvr_entry_cmp(
       IN const DOM_SERVER_ENTRY *A,
       IN const DOM_SERVER_ENTRY *B
       ) ;

void build_srvr_entry_oids( );

//--------------------------- PUBLIC PROCEDURES -----------------------------


//
// MIB_srvr_lmget
//    Retrieve domain server table information from Lan Manager.
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
SNMPAPI MIB_svsond_lmget()
{
    DWORD               entriesread;
    DWORD               totalentries;
    LPBYTE              bufptr;
    unsigned            lmCode;
    unsigned            i;
    SERVER_INFO_100     *DataTable;
    DOM_SERVER_ENTRY    *MIB_DomServerTableElement ;
    int                 First_of_this_block;
    time_t              curr_time ;
    SNMPAPI             nResult = SNMPAPI_NOERROR;
    DWORD               resumehandle=0;


    time(&curr_time);	// get the time


    //
    //
    // If cached, return piece of info.
    //
    //
    if((NULL != cache_table[C_SRVR_TABLE].bufptr) && 
       (curr_time < (cache_table[C_SRVR_TABLE].acquisition_time + cache_expire[C_SRVR_TABLE]))
      )
    { // it has NOT expired!
        goto Exit ; // the global table is valid
    }


    // free the old table  LOOK OUT!!
    MIB_DomServerTableElement = MIB_DomServerTable.Table ;

    // iterate over the whole table
    for(i=0; i<MIB_DomServerTable.Len ;i++)
    {
        // free any alloc'ed elements of the structure
        SnmpUtilOidFree(&(MIB_DomServerTableElement->Oid));
        SafeFree(MIB_DomServerTableElement->domServerName.stream);

        MIB_DomServerTableElement ++ ;  // increment table entry
    }
    SafeFree(MIB_DomServerTable.Table) ;	// free the base Table
    MIB_DomServerTable.Table = NULL ;	// just for safety
    MIB_DomServerTable.Len = 0 ;		// just for safety

    First_of_this_block = 0;
    do
    {
        //  as long as there is more data to process


        //
        //
        // Do network call to gather information and put it in a nice array
        //
        //
        lmCode = NetServerEnum(
                    NULL,			// local server NT_PROBLEM
		            100,			// level 100
		            &bufptr,		// data structure to return
		            MAX_PREFERRED_LENGTH,  
		            &entriesread,
		            &totalentries,
		            SV_TYPE_SERVER,
		            NULL,
       		        &resumehandle		//  resume handle
		        );

        DataTable = (SERVER_INFO_100 *) bufptr ;

        if((NERR_Success == lmCode) || (ERROR_MORE_DATA == lmCode))
        {
            // valid so process it, otherwise error

            if(0 == MIB_DomServerTable.Len)
            {
                // 1st time, alloc the whole table
                // alloc the table space
                MIB_DomServerTable.Table = SnmpUtilMemAlloc(totalentries * sizeof(DOM_SERVER_ENTRY) );
            }

            MIB_DomServerTableElement = MIB_DomServerTable.Table + First_of_this_block ;

            for(i=0; i<entriesread; i++)
            {
                // once for each entry in the buffer
                // increment the entry number

                MIB_DomServerTable.Len ++;

                // Stuff the data into each item in the table
                MIB_DomServerTableElement->domServerName.dynamic = TRUE;
#ifdef UNICODE
                if (SnmpUtilUnicodeToUTF8(
                    &MIB_DomServerTableElement->domServerName.stream,
                    DataTable->sv100_name,
                    TRUE))
                {
                    MIB_DomServerTableElement->domServerName.stream = NULL;
                }
                else
                {
                    MIB_DomServerTableElement->domServerName.length =
                        strlen(MIB_DomServerTableElement->domServerName.stream);
                }
#else
                MIB_DomServerTableElement->domServerName.stream = SnmpUtilMemAlloc(strlen( DataTable->sv100_name ) + 1 );
                MIB_DomServerTableElement->domServerName.length = strlen( DataTable->sv100_name ) ;

                // client name
                memcpy(
                    MIB_DomServerTableElement->domServerName.stream,
                    DataTable->sv100_name,
                    strlen(DataTable->sv100_name)) ;
#endif

                MIB_DomServerTableElement ++ ;  // and table entry
                DataTable ++ ;  // advance pointer to next sess entry in buffer

            } // for each entry in the data table


            // free all of the lan man data
            SafeBufferFree( bufptr ) ;

            // indicate where to start adding on next pass, if any
            First_of_this_block = i ;

        } // if data is valid to process
        else
        {
            // if ERROR_NO_BROWSER_SERVERS_FOUND we are not running in an NetBIOS environment
            // in this case we will have an empty table.
            nResult = (lmCode == ERROR_NO_BROWSER_SERVERS_FOUND) ? SNMPAPI_NOERROR : SNMPAPI_ERROR;
            goto Exit;
        }

    } while (ERROR_MORE_DATA == lmCode) ;


    // iterate over the table populating the Oid field
    build_srvr_entry_oids();

    // Sort the table information using MSC QuickSort routine
    qsort( &MIB_DomServerTable.Table[0], MIB_DomServerTable.Len,
          sizeof(DOM_SERVER_ENTRY), srvr_entry_cmp );

    //
    //
    // Cache table
    //
    //

    if(0 != MIB_DomServerTable.Len)
    {
        cache_table[C_SRVR_TABLE].acquisition_time = curr_time ;
        cache_table[C_SRVR_TABLE].bufptr = bufptr ;
    }

    //
    //
    // Return piece of information requested
    //
    //
    Exit:
    return nResult;
} // MIB_srvr_get

//
// MIB_srvr_cmp
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
int __cdecl srvr_entry_cmp(
       IN const DOM_SERVER_ENTRY *A,
       IN const DOM_SERVER_ENTRY *B
       )

{
   // Compare the OID's
   return SnmpUtilOidCmp( (AsnObjectIdentifier *)&A->Oid,
                       (AsnObjectIdentifier *)&B->Oid );
} // MIB_srvr_cmp


//
//    None.
//
void build_srvr_entry_oids(
       )

{
AsnOctetString OSA ;
DOM_SERVER_ENTRY *DomServerEntry ;
unsigned i;

// start pointer at 1st guy in the table
DomServerEntry = MIB_DomServerTable.Table ;

// now iterate over the table, creating an oid for each entry
for( i=0; i<MIB_DomServerTable.Len ; i++)  {
   // for each entry in the session table

   OSA.stream = DomServerEntry->domServerName.stream ;
   OSA.length =  DomServerEntry->domServerName.length ;
   OSA.dynamic = FALSE;

   // Make the entry's OID from string index
   MakeOidFromStr( &OSA, &DomServerEntry->Oid );

   DomServerEntry++; // point to the next guy in the table

   } // for

} // build_srvr_entry_oids
//-------------------------------- END --------------------------------------
