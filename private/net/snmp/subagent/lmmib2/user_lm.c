/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    user_lm.c

Abstract:

    This file contains the routines which actually call Lan Manager and
    retrieve the contents of the user table, including cacheing.

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
#include <time.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------


#include "mib.h"
#include "mibfuncs.h"
#include "user_tbl.h"
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


int __cdecl user_entry_cmp(
       IN const USER_ENTRY *A,
       IN const USER_ENTRY *B
       ) ;

void build_user_entry_oids( );

//--------------------------- PUBLIC PROCEDURES -----------------------------


//
// MIB_user_lmget
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
SNMPAPI MIB_users_lmget(
           )

{

DWORD entriesread;
DWORD totalentries;
LPBYTE bufptr;
unsigned lmCode;
unsigned i;
USER_INFO_0 *DataTable;
USER_ENTRY *MIB_UserTableElement ;
int First_of_this_block;
LPSTR ansi_string;
time_t curr_time ;
SNMPAPI nResult = SNMPAPI_NOERROR;
DWORD resumehandle=0;


   time(&curr_time);    // get the time


   //
   //
   // If cached, return piece of info.
   //
   //


   if((NULL != cache_table[C_USER_TABLE].bufptr) &&
      (curr_time <
        (cache_table[C_USER_TABLE].acquisition_time
                 + cache_expire[C_USER_TABLE]              ) ) )
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

     MIB_UserTableElement = MIB_UserTable.Table ;

     // iterate over the whole table
     for(i=0; i<MIB_UserTable.Len ;i++)
     {
        // free any alloc'ed elements of the structure
        SnmpUtilOidFree(&(MIB_UserTableElement->Oid));
        SafeFree(MIB_UserTableElement->svUserName.stream);

        MIB_UserTableElement ++ ;  // increment table entry
     }
     SafeFree(MIB_UserTable.Table) ;    // free the base Table
     MIB_UserTable.Table = NULL ;       // just for safety
     MIB_UserTable.Len = 0 ;            // just for safety



#if 0 // done above
   // init the length
   MIB_UserTable.Len = 0;
#endif
   First_of_this_block = 0;

   do {  //  as long as there is more data to process


        lmCode =
        NetUserEnum(    NULL,                   // local server
                        0,                      // level 0, no admin priv.
            FILTER_NORMAL_ACCOUNT,
                        &bufptr,                // data structure to return
                        MAX_PREFERRED_LENGTH,
                        &entriesread,
                        &totalentries,
                        &resumehandle           //  resume handle
                        );


    DataTable = (USER_INFO_0 *) bufptr ;

    if((NERR_Success == lmCode) || (ERROR_MORE_DATA == lmCode))
        {  // valid so process it, otherwise error

        if(0 == MIB_UserTable.Len) {  // 1st time, alloc the whole table
                // alloc the table space
                MIB_UserTable.Table = SnmpUtilMemAlloc(totalentries *
                                                sizeof(USER_ENTRY) );
        }

        MIB_UserTableElement = MIB_UserTable.Table + First_of_this_block ;

        for(i=0; i<entriesread; i++) {  // once for each entry in the buffer


                // increment the entry number

                MIB_UserTable.Len ++;

                // Stuff the data into each item in the table

                // convert the undocumented unicode to something readable
                SnmpUtilUnicodeToUTF8(
                        &ansi_string,
                        DataTable->usri0_name,
                        TRUE ); // auto alloc the space for ansi

                // client name
                MIB_UserTableElement->svUserName.stream = ansi_string;
                MIB_UserTableElement->svUserName.length =
                                strlen( ansi_string ) ;
                MIB_UserTableElement->svUserName.dynamic = TRUE;
                ansi_string = NULL;
                MIB_UserTableElement ++ ;  // and table entry

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
    build_user_entry_oids();

   // Sort the table information using MSC QuickSort routine
   qsort( &MIB_UserTable.Table[0], MIB_UserTable.Len,
          sizeof(USER_ENTRY), user_entry_cmp );

   //
   //
   // Cache table
   //
   //


   if(0 != MIB_UserTable.Len) {

        cache_table[C_USER_TABLE].acquisition_time = curr_time ;

        cache_table[C_USER_TABLE].bufptr = bufptr ;
   }

   //
   //
   // Return piece of information requested
   //
   //
Exit:
   return nResult;
} // MIB_user_get

//
// MIB_user_cmp
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
int __cdecl user_entry_cmp(
       IN const USER_ENTRY *A,
       IN const USER_ENTRY *B
       )

{
   // Compare the OID's
   return SnmpUtilOidCmp( (AsnObjectIdentifier *)&A->Oid,
                       (AsnObjectIdentifier *)&B->Oid );
} // MIB_user_cmp


//
//    None.
//
void build_user_entry_oids(
       )

{
AsnOctetString OSA ;
USER_ENTRY *UserEntry ;
unsigned i;

// start pointer at 1st guy in the table
UserEntry = MIB_UserTable.Table ;

// now iterate over the table, creating an oid for each entry
for( i=0; i<MIB_UserTable.Len ; i++)  {
   // for each entry in the session table

   OSA.stream = UserEntry->svUserName.stream ;
   OSA.length =  UserEntry->svUserName.length ;
   OSA.dynamic = FALSE;

   // Make the entry's OID from string index
   MakeOidFromStr( &OSA, &UserEntry->Oid );

   UserEntry++; // point to the next guy in the table

   } // for

} // build_user_entry_oids
//-------------------------------- END --------------------------------------
