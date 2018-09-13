/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    sess_lm.c

Abstract:

    This file contains MIB_sess_lmget, which actually call lan manager
    for the session table, copies it into structures, and sorts it to
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
#include "sess_tbl.h"
#include "lmcache.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define SafeBufferFree(x)       if(NULL != x) NetApiBufferFree( x )
#define SafeFree(x)             if(NULL != x) SnmpUtilMemFree( x )

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

int __cdecl sess_entry_cmp(
       IN const SESS_ENTRY *A,
       IN const SESS_ENTRY *B
       ) ;

void build_sess_entry_oids( );

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// MIB_sess_lmset
//    Perform the necessary actions to SET a field in the Session Table.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
UINT MIB_sess_lmset(
        IN AsnObjectIdentifier *Index,
        IN UINT Field,
        IN AsnAny *Value
        )

{
NET_API_STATUS lmCode;
int            Found;
UINT           Entry;
AsnInteger     ErrStat = SNMP_ERRORSTATUS_NOERROR;
char           Client[100];
char           User[100];
#ifdef UNICODE
LPWSTR         UniClient;
LPWSTR         UniUser;
#endif


   // Must make sure the table is in memory
   if ( SNMPAPI_ERROR == MIB_sess_lmget() )
      {
      ErrStat = SNMP_ERRORSTATUS_GENERR;
      goto Exit;
      }

   // Find a match in the table
   if ( MIB_TBL_POS_FOUND != MIB_sess_match(Index, &Entry, FALSE) )
      {
      ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
      goto Exit;
      }

   // Check for action on Table
   if ( Value->asnValue.number == SESS_STATE_DELETED )
      {
      strcpy( Client, "\\\\" );
      strncpy( &Client[2], MIB_SessionTable.Table[Entry].svSesClientName.stream,
                       MIB_SessionTable.Table[Entry].svSesClientName.length );
      Client[MIB_SessionTable.Table[Entry].svSesClientName.length+2] = '\0';
      strncpy( User, MIB_SessionTable.Table[Entry].svSesUserName.stream,
                     MIB_SessionTable.Table[Entry].svSesUserName.length );
      User[MIB_SessionTable.Table[Entry].svSesUserName.length] = '\0';

#ifdef UNICODE
      SnmpUtilUTF8ToUnicode(      &UniClient,
                                Client,
                                TRUE );
      SnmpUtilUTF8ToUnicode(      &UniUser,
                                User,
                                TRUE );

      lmCode = NetSessionDel( NULL, UniClient, UniUser );
      SnmpUtilMemFree(UniClient);
      SnmpUtilMemFree(UniUser);
#else
      // Call the LM API to delete it
      lmCode = NetSessionDel( NULL, Client, User );
#endif

      // Check for successful operation
      switch( lmCode )
         {
         case NERR_Success:
            // Make cache be reloaded next time
            cache_table[C_SESS_TABLE].bufptr = NULL;
            break;

         case NERR_ClientNameNotFound:
         case NERR_UserNotFound:
            ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
            break;

         default:
            ErrStat = SNMP_ERRORSTATUS_GENERR;
         }
      }

Exit:
   return ErrStat;
} // MIB_sess_lmset



//
// MIB_sess_lmget
//    Retrieve session table information from Lan Manager.
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
SNMPAPI MIB_sess_lmget(
           )

{

DWORD entriesread;
DWORD totalentries;
LPBYTE bufptr=NULL;
unsigned lmCode;
unsigned i;
SESSION_INFO_2 *DataTable;
SESS_ENTRY *MIB_SessionTableElement ;
int First_of_this_block;
time_t curr_time ;
SNMPAPI nResult = SNMPAPI_NOERROR;
LPSTR tempbuff ;
DWORD resumehandle=0;

   time(&curr_time);    // get the time


//return nResult;  // OPENISSUE  remember the problem with the error
                 // every time a free is done from this call to Enum?


   //
   //
   // If cached, return piece of info.
   //
   //


   if((NULL != cache_table[C_SESS_TABLE].bufptr) &&
      (curr_time <
        (cache_table[C_SESS_TABLE].acquisition_time
                 + cache_expire[C_SESS_TABLE]              ) ) )
        { // it has NOT expired!

        goto Exit ; // the global table is valid

        }

   //
   //
   // Do network call to gather information and put it in a nice array
   //
   //

   // free the old table  LOOK OUT!!


     MIB_SessionTableElement = MIB_SessionTable.Table ;

     // iterate over the whole table
     for(i=0; i<MIB_SessionTable.Len ;i++)
     {
        // free any alloc'ed elements of the structure
        SnmpUtilOidFree(&(MIB_SessionTableElement->Oid));
        SafeFree(MIB_SessionTableElement->svSesClientName.stream);
        SafeFree(MIB_SessionTableElement->svSesUserName.stream);

        MIB_SessionTableElement ++ ;  // increment table entry
     }
     SafeFree(MIB_SessionTable.Table) ; // free the base Table
     MIB_SessionTable.Table = NULL ;    // just for safety
     MIB_SessionTable.Len = 0 ;         // just for safety

   First_of_this_block = 0;

   do {  //  as long as there is more data to process

   lmCode =
   NetSessionEnum( NULL,                        // local server
                        NULL,           // get server stats
                        NULL,
                        2,                      // level
                        &bufptr,                // data structure to return
                        MAX_PREFERRED_LENGTH,
                        &entriesread,
                        &totalentries,
                        NULL   //&resumehandle          //  resume handle
                        );


    if(NULL == bufptr)  return nResult ;

    DataTable = (SESSION_INFO_2 *) bufptr ;

    if((NERR_Success == lmCode) || (ERROR_MORE_DATA == lmCode))
        {  // valid so process it, otherwise error

        if(0 == MIB_SessionTable.Len) {  // 1st time, alloc the whole table
                // alloc the table space
                MIB_SessionTable.Table = SnmpUtilMemAlloc(totalentries *
                                                sizeof(SESS_ENTRY) );
        }

        MIB_SessionTableElement = MIB_SessionTable.Table + First_of_this_block ;

        for(i=0; i<entriesread; i++) {  // once for each entry in the buffer
                // increment the entry number

                MIB_SessionTable.Len ++;

                // Stuff the data into each item in the table

                // client name
                MIB_SessionTableElement->svSesClientName.dynamic = TRUE;

#ifdef UNICODE
                if (SnmpUtilUnicodeToUTF8(
                        &MIB_SessionTableElement->svSesClientName.stream,
                        DataTable->sesi2_cname,
                        TRUE))
                {
                    MIB_SessionTableElement->svSesClientName.stream = NULL;
                    MIB_SessionTableElement->svSesClientName.length = 0;
                }
                else
                {
                    MIB_SessionTableElement->svSesClientName.length = 
                        strlen (MIB_SessionTableElement->svSesClientName.stream);
                }
#else
                MIB_SessionTableElement->svSesClientName.stream = SnmpUtilMemAlloc (
                                strlen( DataTable->sesi2_cname )+1 ) ;
                MIB_SessionTableElement->svSesClientName.length =
                                strlen( DataTable->sesi2_cname ) ;
                memcpy( MIB_SessionTableElement->svSesClientName.stream,
                        DataTable->sesi2_cname,
                        strlen( DataTable->sesi2_cname ) ) ;
#endif

                // user name
                MIB_SessionTableElement->svSesUserName.dynamic = TRUE;


#ifdef UNICODE
                if (SnmpUtilUnicodeToUTF8(
                        &MIB_SessionTableElement->svSesUserName.stream,
                        DataTable->sesi2_username,
                        TRUE))
                {
                    MIB_SessionTableElement->svSesUserName.length = 0;
                    MIB_SessionTableElement->svSesUserName.stream = NULL;
                }
                else
                {
                    MIB_SessionTableElement->svSesUserName.length =
                        strlen(MIB_SessionTableElement->svSesUserName.stream);
                }
#else
                MIB_SessionTableElement->svSesUserName.stream = SnmpUtilMemAlloc (
                    strlen( DataTable->sesi2_username ) + 1 ) ;
                MIB_SessionTableElement->svSesUserName.length =
                    strlen( DataTable->sesi2_username ) ;

                memcpy( MIB_SessionTableElement->svSesUserName.stream,
                        DataTable->sesi2_username,
                        strlen( DataTable->sesi2_username ) ) ;
#endif
                // number of connections
                MIB_SessionTableElement->svSesNumConns =
                        // DataTable->sesi2_num_conns ; LM_NOT_THERE
                        0 ;  // so get ready in case somebody implements

                // number of opens
                MIB_SessionTableElement->svSesNumOpens =
                        DataTable->sesi2_num_opens ;

                // session time
                MIB_SessionTableElement->svSesTime =
                        DataTable->sesi2_time ;

                // session idle time
                MIB_SessionTableElement->svSesIdleTime =
                        DataTable->sesi2_idle_time ;

                // client type parsing

                // first convert from unicode if needed
#ifdef UNICODE
                SnmpUtilUnicodeToUTF8(
                        &tempbuff,
                        DataTable->sesi2_cltype_name,
                        TRUE);
#else
                tempbuff = SnmpUtilMemAlloc( strlen(DataTable->sesi2_cltype_name) + 1 );
                memcpy( tempbuff,
                        DataTable->sesi2_cltype_name,
                        strlen( DataTable->sesi2_cltype_name ) ) ;
#endif

                // let's assume 0 is undefined but better than garbage ...
                MIB_SessionTableElement->svSesClientType = 0 ;
                if(0==strcmp(   "DOWN LEVEL",
                                tempbuff))
                        MIB_SessionTableElement->svSesClientType = 1 ;
                else if(0==strcmp("DOS LM",
                                  tempbuff))
                        MIB_SessionTableElement->svSesClientType = 2 ;
                else if(0==strcmp("DOS LM 2.0",
                                  tempbuff))
                        MIB_SessionTableElement->svSesClientType = 3 ;
                else if(0==strcmp("OS/2 LM 1.0",
                                  tempbuff))
                        MIB_SessionTableElement->svSesClientType = 4 ;
                else if(0==strcmp("OS/2 LM 2.0",
                                  tempbuff))
                        MIB_SessionTableElement->svSesClientType = 5 ;
                else if(0==strcmp("DOS LM 2.1",
                                  tempbuff))
                        MIB_SessionTableElement->svSesClientType = 6 ;
                else if(0==strcmp("OS/2 LM 2.1",
                                  tempbuff))
                        MIB_SessionTableElement->svSesClientType = 7 ;
                else if(0==strcmp("AFP 1.1",
                                  tempbuff))
                        MIB_SessionTableElement->svSesClientType = 8 ;
                else if(0==strcmp("AFP 2.0",
                                  tempbuff))
                        MIB_SessionTableElement->svSesClientType = 9 ;
                else if(0==strcmp("NT",
                                  tempbuff))
                        MIB_SessionTableElement->svSesClientType = 10 ;
                SnmpUtilMemFree(tempbuff);

                // state is always active, set uses to indicate delete request
                MIB_SessionTableElement->svSesState = 1; //always active


                DataTable ++ ;  // advance pointer to next sess entry in buffer
                MIB_SessionTableElement ++ ;  // and table entry

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
    build_sess_entry_oids();

   // Sort the table information using MSC QuickSort routine
   qsort( (void *)&MIB_SessionTable.Table[0], (size_t)MIB_SessionTable.Len,
          (size_t)sizeof(SESS_ENTRY), sess_entry_cmp );

   //
   //
   // Cache table
   //
   //

   if(0 != MIB_SessionTable.Len) {

        cache_table[C_SESS_TABLE].acquisition_time = curr_time ;

        cache_table[C_SESS_TABLE].bufptr = bufptr ;
   }


   //
   //
   // Return piece of information requested
   //
   //

Exit:
   return nResult;
} // MIB_sess_get

//
// MIB_sess_cmp
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
int __cdecl sess_entry_cmp(
       IN const SESS_ENTRY *A,
       IN const SESS_ENTRY *B
       )

{
   // Compare the OID's
   return SnmpUtilOidCmp( (AsnObjectIdentifier *)&A->Oid,
                       (AsnObjectIdentifier *)&B->Oid );
} // MIB_sess_cmp


//
//    None.
//
void build_sess_entry_oids(
       )

{
AsnOctetString OSA ;
AsnObjectIdentifier UserNameOid ;
SESS_ENTRY *SessEntry ;
unsigned i;

// start pointer at 1st guy in the table
SessEntry = MIB_SessionTable.Table ;

// now iterate over the table, creating an oid for each entry
for( i=0; i<MIB_SessionTable.Len ; i++)  {
   // for each entry in the session table

   // copy the client name into the oid buffer first
   MakeOidFromStr( &SessEntry->svSesClientName, &SessEntry->Oid );

   // copy the user name into a temporary oid buffer
   MakeOidFromStr( &SessEntry->svSesUserName, &UserNameOid );

   // append the two entries forming the index
   SnmpUtilOidAppend( &SessEntry->Oid, &UserNameOid );

   // free the temporary buffer
   SnmpUtilOidFree( &UserNameOid );

   SessEntry++; // point to the next guy in the table

   } // for

} // build_sess_entry_oids
//-------------------------------- END --------------------------------------
