/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    odom_lm.c

Abstract:

    This file contains the routines which actually call Lan Manager and
    retrieve the contents of the other domains table, including cacheing.

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
#include "odom_tbl.h"
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

int __cdecl odom_entry_cmp(
       IN const DOM_OTHER_ENTRY *A,
       IN const DOM_OTHER_ENTRY *B
       ) ;

void build_odom_entry_oids( );

int chrcount(char *s)
{
char *temp;
int i;
temp = s;
i = 1;  // assume one since no terminating space, other code counts tokens
while( NULL != (temp = strchr(temp,' ')) ) {
        i++;
        }
return i;
}

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// MIB_odoms_lmset
//    Perform the necessary actions to set an entry in the Other Domain Table.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
UINT MIB_odoms_lmset(
        IN AsnObjectIdentifier *Index,
        IN UINT Field,
        IN AsnAny *Value
        )

{
LPBYTE bufptr = NULL;
WKSTA_USER_INFO_1101 ODom;
LPBYTE Temp;
UINT   Entry;
UINT   I;
UINT   ErrStat = SNMP_ERRORSTATUS_NOERROR;
#ifdef UNICODE
LPWSTR unitemp ;
#endif


   // Must make sure the table is in memory
   if ( SNMPAPI_ERROR == MIB_odoms_lmget() )
      {
      ErrStat = SNMP_ERRORSTATUS_GENERR;
      goto Exit;
      }

   // See if match in table
   if ( MIB_TBL_POS_FOUND == MIB_odoms_match(Index, &Entry) )
      {
      // If empty string then delete entry
      if ( Value->asnValue.string.length == 0 )
         {
         // Alloc memory for buffer
         bufptr = SnmpUtilMemAlloc( DNLEN * sizeof(char) *
                          (MIB_DomOtherDomainTable.Len-1) +
                          MIB_DomOtherDomainTable.Len-1 );

         // Create the other domain string
         Temp = bufptr;
         for ( I=0;I < MIB_DomOtherDomainTable.Len;I++ )
            {
            if ( I+1 != Entry )
               {
               memcpy( Temp,
                       MIB_DomOtherDomainTable.Table[I].domOtherName.stream,
                       MIB_DomOtherDomainTable.Table[I].domOtherName.length );
               Temp[MIB_DomOtherDomainTable.Table[I].domOtherName.length] = ' ';
               Temp += MIB_DomOtherDomainTable.Table[I].domOtherName.length + 1;
               }
            }
         *(Temp-1) = '\0';
         }
      else
         {
         // Cannot modify the domain entries, so bad value
         ErrStat = SNMP_ERRORSTATUS_BADVALUE;
         goto Exit;
         }
      }
   else
      {
      // Check for addition of NULL string, bad value
      if ( Value->asnValue.string.length == 0 )
         {
         ErrStat = SNMP_ERRORSTATUS_BADVALUE;
         goto Exit;
         }

      //
      // Entry doesn't exist so add it to the list
      //

      // Alloc memory for buffer
      bufptr = SnmpUtilMemAlloc( DNLEN * sizeof(char) *
                       (MIB_DomOtherDomainTable.Len+1) +
                       MIB_DomOtherDomainTable.Len+1 );

      // Create the other domain string
      Temp = bufptr;
      for ( I=0;I < MIB_DomOtherDomainTable.Len;I++ )
         {
         memcpy( Temp, MIB_DomOtherDomainTable.Table[I].domOtherName.stream,
                 MIB_DomOtherDomainTable.Table[I].domOtherName.length );
         Temp[MIB_DomOtherDomainTable.Table[I].domOtherName.length] = ' ';
         Temp += MIB_DomOtherDomainTable.Table[I].domOtherName.length + 1;
         }

      // Add new entry
      memcpy( Temp, Value->asnValue.string.stream,
                    Value->asnValue.string.length );

      // Add NULL terminator
      Temp[Value->asnValue.string.length] = '\0';
      }

   // Set table and check return codes
   #ifdef UNICODE
   SnmpUtilUTF8ToUnicode(         &unitemp,
                                bufptr,
                                TRUE );
   ODom.wkui1101_oth_domains = unitemp;
   #else
   ODom.wkui1101_oth_domains = bufptr;
   #endif
#if 0
   if ( NERR_Success == NetWkstaUserSetInfo(NULL, 1101, (LPBYTE)&ODom, NULL) )
      {
      // Make cache be reloaded next time
      cache_table[C_ODOM_TABLE].bufptr = NULL;
      }
   else
      {
      ErrStat = SNMP_ERRORSTATUS_GENERR;
      }
#else
   ErrStat = SNMP_ERRORSTATUS_GENERR;
#endif

Exit:
   SnmpUtilMemFree( bufptr );

   return ErrStat;
} // MIB_odoms_lmset



//
// MIB_odom_lmget
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
SNMPAPI MIB_odoms_lmget(
           )

{

DWORD totalentries;
LPBYTE bufptr = NULL;
unsigned lmCode;
WKSTA_USER_INFO_1101 *DataTable;
DOM_OTHER_ENTRY *MIB_DomOtherDomainTableElement ;
char *p;
char *next;
time_t curr_time ;
unsigned i;
SNMPAPI nResult = SNMPAPI_NOERROR;



   time(&curr_time);    // get the time


   //
   //
   // If cached, return piece of info.
   //
   //


   if((NULL != cache_table[C_ODOM_TABLE].bufptr) &&
      (curr_time <
        (cache_table[C_ODOM_TABLE].acquisition_time
                 + cache_expire[C_ODOM_TABLE]              ) ) )
        { // it has NOT expired!

        goto Exit; // the global table is valid

        }

   //
   //
   // Do network call to gather information and put it in a nice array
   //
   //


     //
     // remember to free the existing data
     //

     MIB_DomOtherDomainTableElement = MIB_DomOtherDomainTable.Table ;

     // iterate over the whole table
     for(i=0; i<MIB_DomOtherDomainTable.Len ;i++)
     {
        // free any alloc'ed elements of the structure
        SnmpUtilOidFree(&(MIB_DomOtherDomainTableElement->Oid));
        SafeFree(MIB_DomOtherDomainTableElement->domOtherName.stream);

        MIB_DomOtherDomainTableElement ++ ;  // increment table entry
     }
     SafeFree(MIB_DomOtherDomainTable.Table) ;  // free the base Table
     MIB_DomOtherDomainTable.Table = NULL ;     // just for safety
     MIB_DomOtherDomainTable.Len = 0 ;          // just for safety

        lmCode =
        NetWkstaUserGetInfo(
                        0,                      // required
                        1101,                   // level 0,
                        &bufptr                 // data structure to return
                        );


    DataTable = (WKSTA_USER_INFO_1101 *) bufptr ;

    if((NERR_Success == lmCode) || (ERROR_MORE_DATA == lmCode))
        {  // valid so process it, otherwise error
        if(NULL==DataTable->wkui1101_oth_domains) {
                totalentries = 0;

                // alloc the table space
                MIB_DomOtherDomainTable.Table = SnmpUtilMemAlloc(totalentries *
                                                sizeof(DOM_OTHER_ENTRY) );
        } else {  // compute it
        totalentries = chrcount((char *)DataTable->wkui1101_oth_domains);
        if(0 == MIB_DomOtherDomainTable.Len) {  // 1st time, alloc the whole table
                // alloc the table space
                MIB_DomOtherDomainTable.Table = SnmpUtilMemAlloc(totalentries *
                                                sizeof(DOM_OTHER_ENTRY) );
        }

        MIB_DomOtherDomainTableElement = MIB_DomOtherDomainTable.Table  ;

        // make a pointer to the beginning of the string field

        #ifdef UNICODE
        SnmpUtilUnicodeToUTF8(
                &p,
                DataTable->wkui1101_oth_domains,
                TRUE);
        #else
        p =  DataTable->wkui1101_oth_domains  ;
        #endif

        // scan through the field, making an entry for each space
        // separated domain
        while(  (NULL != p ) &
                ('\0' != *p)  ) {  // once for each entry in the buffer

                // increment the entry number

                MIB_DomOtherDomainTable.Len ++;

                // find the end of this one
                next = strchr(p,' ');

                // if more to come, ready next pointer and mark end of this one
                if(NULL != next) {
                        *next='\0' ;    // replace space with EOS
                        next++ ;        // point to beginning of next domain
                }


                MIB_DomOtherDomainTableElement->domOtherName.stream = SnmpUtilMemAlloc (
                                strlen( p ) ) ;
                MIB_DomOtherDomainTableElement->domOtherName.length =
                                strlen( p ) ;
                MIB_DomOtherDomainTableElement->domOtherName.dynamic = TRUE;
                memcpy( MIB_DomOtherDomainTableElement->domOtherName.stream,
                        p,
                        strlen( p ) ) ;


                MIB_DomOtherDomainTableElement ++ ;  // and table entry

           DataTable ++ ;  // advance pointer to next sess entry in buffer

        } // while still more to do

        } // if there really were entries
        } // if data is valid to process

    else
       {
       // Signal error
       nResult = SNMPAPI_ERROR;
       goto Exit;
       }


   // free all of the lan man data
   SafeBufferFree( bufptr ) ;


    // iterate over the table populating the Oid field
    build_odom_entry_oids();

   // Sort the table information using MSC QuickSort routine
   qsort( (void *)&MIB_DomOtherDomainTable.Table[0], (size_t)MIB_DomOtherDomainTable.Len,
          (size_t)sizeof(DOM_OTHER_ENTRY), odom_entry_cmp );

   //
   //
   // Cache table
   //
   //


   if(0 != MIB_DomOtherDomainTable.Len) {

        cache_table[C_ODOM_TABLE].acquisition_time = curr_time ;

        cache_table[C_ODOM_TABLE].bufptr = bufptr ;
   }

   //
   //
   // Return piece of information requested
   //
   //

Exit:
   return nResult;
} // MIB_odom_get

//
// MIB_odom_cmp
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
int __cdecl odom_entry_cmp(
       IN const DOM_OTHER_ENTRY *A,
       IN const DOM_OTHER_ENTRY *B
       )

{
   // Compare the OID's
   return SnmpUtilOidCmp( (AsnObjectIdentifier *)&A->Oid,
                       (AsnObjectIdentifier *)&B->Oid );
} // MIB_odom_cmp


//
//    None.
//
void build_odom_entry_oids(
       )

{
AsnOctetString OSA ;
DOM_OTHER_ENTRY *DomOtherEntry ;
unsigned i;

// start pointer at 1st guy in the table
DomOtherEntry = MIB_DomOtherDomainTable.Table ;

// now iterate over the table, creating an oid for each entry
for( i=0; i<MIB_DomOtherDomainTable.Len ; i++)  {
   // for each entry in the session table

   OSA.stream = DomOtherEntry->domOtherName.stream ;
   OSA.length =  DomOtherEntry->domOtherName.length ;
   OSA.dynamic = FALSE;

   // Make the entry's OID from string index
   MakeOidFromStr( &OSA, &DomOtherEntry->Oid );

   DomOtherEntry++; // point to the next guy in the table

   } // for

} // build_odom_entry_oids
//-------------------------------- END --------------------------------------
