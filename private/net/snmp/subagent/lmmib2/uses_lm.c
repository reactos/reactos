/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    uses_lm.c

Abstract:

    This file contains the routines which actually call Lan Manager and
    retrieve the contents of the workstation uses table, including cacheing.

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
#include "uses_tbl.h"
#include "lmcache.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define SafeBufferFree(x)       if(NULL != x) NetApiBufferFree( x )
#define SafeFree(x)             if(NULL != x) SnmpUtilMemFree( x )

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

int __cdecl uses_entry_cmp(
       IN const WKSTA_USES_ENTRY *A,
       IN const WKSTA_USES_ENTRY *B
       ) ;

void build_uses_entry_oids( );

//--------------------------- PUBLIC PROCEDURES -----------------------------


//
// MIB_wsuses_lmget
//    Retrieve workstation uses table information from Lan Manager.
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
SNMPAPI MIB_wsuses_lmget(
           )

{

DWORD entriesread;
DWORD totalentries;
LPBYTE bufptr;
unsigned lmCode;
unsigned i;
USE_INFO_1 *DataTable;
WKSTA_USES_ENTRY *MIB_WkstaUsesTableElement ;
int First_of_this_block;
time_t curr_time ;
SNMPAPI nResult = SNMPAPI_NOERROR;
DWORD resumehandle=0;


   time(&curr_time);    // get the time


   //
   //
   // If cached, return piece of info.
   //
   //

   if((NULL != cache_table[C_USES_TABLE].bufptr) &&
      (curr_time <
        (cache_table[C_USES_TABLE].acquisition_time
                 + cache_expire[C_USES_TABLE]              ) ) )
        { // it has NOT expired!

        goto Exit ; // the global table is valid

        }


   //
   //
   // Do network call to gather information and put it in a nice array
   //
   //

   // free the old table  LOOK OUT!!


     MIB_WkstaUsesTableElement = MIB_WkstaUsesTable.Table ;

     // iterate over the whole table
     for(i=0; i<MIB_WkstaUsesTable.Len ;i++)
     {
        // free any alloc'ed elements of the structure
        SnmpUtilOidFree(&(MIB_WkstaUsesTableElement->Oid));
        SafeFree(MIB_WkstaUsesTableElement->useLocalName.stream);
        SafeFree(MIB_WkstaUsesTableElement->useRemote.stream);

        MIB_WkstaUsesTableElement ++ ;  // increment table entry
     }
     SafeFree(MIB_WkstaUsesTable.Table) ;       // free the base Table
     MIB_WkstaUsesTable.Table = NULL ;  // just for safety
     MIB_WkstaUsesTable.Len = 0 ;               // just for safety


#if 0 // done above
   // init the length
   MIB_WkstaUsesTable.Len = 0;
#endif
   First_of_this_block = 0;

   do {  //  as long as there is more data to process

   lmCode =
        NetUseEnum(     NULL,                   // local server
        1,                      // level 1, no admin priv.
        &bufptr,                // data structure to return
        MAX_PREFERRED_LENGTH,
        &entriesread,
        &totalentries,
        &resumehandle           //  resume handle
        );


    DataTable = (USE_INFO_1 *) bufptr ;

    if((NERR_Success == lmCode) || (ERROR_MORE_DATA == lmCode))
        {  // valid so process it, otherwise error

        if(0 == MIB_WkstaUsesTable.Len) {  // 1st time, alloc the whole table
                // alloc the table space
                MIB_WkstaUsesTable.Table = SnmpUtilMemAlloc(totalentries *
                                                sizeof(WKSTA_USES_ENTRY) );
        }

        MIB_WkstaUsesTableElement = MIB_WkstaUsesTable.Table + First_of_this_block ;

        for(i=0; i<entriesread; i++) {  // once for each entry in the buffer


                // increment the entry number

                MIB_WkstaUsesTable.Len ++;

                // Stuff the data into each item in the table

                // client name
                MIB_WkstaUsesTableElement->useLocalName.dynamic = TRUE;

#ifdef UNICODE
                if (SnmpUtilUnicodeToUTF8(
                        &MIB_WkstaUsesTableElement->useLocalName.stream,
                        DataTable->ui1_local,
                        TRUE))
                {
                    MIB_WkstaUsesTableElement->useLocalName.length = 0;
                    MIB_WkstaUsesTableElement->useLocalName.stream = NULL;
                }
                else
                {
                    MIB_WkstaUsesTableElement->useLocalName.length =
                        strlen(MIB_WkstaUsesTableElement->useLocalName.stream);
                }
#else
                MIB_WkstaUsesTableElement->useLocalName.stream = SnmpUtilMemAlloc (
                                strlen( DataTable->ui1_local ) + 1 ) ;
                MIB_WkstaUsesTableElement->useLocalName.length =
                                strlen( DataTable->ui1_local ) ;
                memcpy( MIB_WkstaUsesTableElement->useLocalName.stream,
                        DataTable->ui1_local,
                        strlen( DataTable->ui1_local ) ) ;
#endif

                // remote name
                MIB_WkstaUsesTableElement->useRemote.dynamic = TRUE;

#ifdef UNICODE
                if (SnmpUtilUnicodeToUTF8(
                        &MIB_WkstaUsesTableElement->useRemote.stream,
                        DataTable->ui1_remote,
                        TRUE))
                {
                    MIB_WkstaUsesTableElement->useRemote.length = 0;
                    MIB_WkstaUsesTableElement->useRemote.stream = NULL;
                }
                else
                {
                    MIB_WkstaUsesTableElement->useRemote.length = 
                        strlen(MIB_WkstaUsesTableElement->useRemote.stream);
                }
#else
                MIB_WkstaUsesTableElement->useRemote.stream = SnmpUtilMemAlloc (
                                strlen( DataTable->ui1_remote ) + 1 ) ;
                MIB_WkstaUsesTableElement->useRemote.length =
                                strlen( DataTable->ui1_remote ) ;

                memcpy( MIB_WkstaUsesTableElement->useRemote.stream,
                        DataTable->ui1_remote,
                        strlen( DataTable->ui1_remote ) ) ;
#endif

                // status
                MIB_WkstaUsesTableElement->useStatus =
                                DataTable->ui1_status ;


                MIB_WkstaUsesTableElement ++ ;  // and table entry

                DataTable ++ ;  // advance pointer to next sess entry in buffer

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
    build_uses_entry_oids();

   // Sort the table information using MSC QuickSort routine
   qsort( &MIB_WkstaUsesTable.Table[0], MIB_WkstaUsesTable.Len,
          sizeof(WKSTA_USES_ENTRY), uses_entry_cmp );

   //
   //
   // Cache table
   //
   //

   if(0 != MIB_WkstaUsesTable.Len) {

        cache_table[C_USES_TABLE].acquisition_time = curr_time ;

        cache_table[C_USES_TABLE].bufptr = bufptr ;
   }


   //
   //
   // Return piece of information requested
   //
   //
Exit:
   return nResult;
} // MIB_uses_get

//
// MIB_uses_cmp
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
int __cdecl uses_entry_cmp(
       IN const WKSTA_USES_ENTRY *A,
       IN const WKSTA_USES_ENTRY *B
       )

{
   // Compare the OID's
   return SnmpUtilOidCmp( (AsnObjectIdentifier *)&A->Oid,
                       (AsnObjectIdentifier *)&B->Oid );
} // MIB_uses_cmp


//
//    None.
//
void build_uses_entry_oids(
       )

{
AsnOctetString OSA ;
AsnObjectIdentifier RemoteOid ;
WKSTA_USES_ENTRY *WkstaUsesEntry ;
unsigned i;

// start pointer at 1st guy in the table
WkstaUsesEntry = MIB_WkstaUsesTable.Table ;

// now iterate over the table, creating an oid for each entry
for( i=0; i<MIB_WkstaUsesTable.Len ; i++)  {
   // for each entry in the session table

   // copy the local name into the oid buffer first
   MakeOidFromStr( &WkstaUsesEntry->useLocalName, &WkstaUsesEntry->Oid );

   // copy the remote name into a temporary oid buffer
   MakeOidFromStr( &WkstaUsesEntry->useRemote, &RemoteOid );

   // append the two entries forming the index
   SnmpUtilOidAppend( &WkstaUsesEntry->Oid, &RemoteOid );

   // free the temporary buffer
   SnmpUtilOidFree( &RemoteOid );

   WkstaUsesEntry++; // point to the next guy in the table

   } // for

} // build_uses_entry_oids
//-------------------------------- END --------------------------------------
