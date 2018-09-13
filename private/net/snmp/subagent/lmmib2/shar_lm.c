/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    shar_lm.c

Abstract:

    This file contains MIB_shar_lmget, which actually call lan manager
    for the share table, copies it into structures, and sorts it to
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
#include "shar_tbl.h"
#include "lmcache.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define SafeBufferFree(x)	if(NULL != x) NetApiBufferFree( x )
#define SafeFree(x)             if(NULL != x) SnmpUtilMemFree( x )

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

int __cdecl shar_entry_cmp(
       IN const SHARE_ENTRY *A,
       IN const SHARE_ENTRY *B
       ) ;

void build_shar_entry_oids( );

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------


//
// MIB_shar_lmget
//    Retrieve sharion table information from Lan Manager.
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
SNMPAPI MIB_shares_lmget(
	   )

{

DWORD entriesread;
DWORD totalentries;
LPBYTE bufptr;
unsigned lmCode;
unsigned i;
SHARE_INFO_2 *DataTable;
SHARE_ENTRY *MIB_ShareTableElement ;
int First_of_this_block;
time_t curr_time ;
SNMPAPI nResult = SNMPAPI_NOERROR;
DWORD resumehandle=0;


   time(&curr_time);	// get the time


   //
   //
   // If cached, return piece of info.
   //
   //


   if((NULL != cache_table[C_SHAR_TABLE].bufptr) &&
      (curr_time <
    	(cache_table[C_SHAR_TABLE].acquisition_time
        	 + cache_expire[C_SHAR_TABLE]              ) ) )
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

     MIB_ShareTableElement = MIB_ShareTable.Table ;

     // iterate over the whole table
     for(i=0; i<MIB_ShareTable.Len ;i++)
     {
     	// free any alloc'ed elements of the structure
     	SnmpUtilOidFree(&(MIB_ShareTableElement->Oid));
     	SafeFree(MIB_ShareTableElement->svShareName.stream);
     	SafeFree(MIB_ShareTableElement->svSharePath.stream);
     	SafeFree(MIB_ShareTableElement->svShareComment.stream);
     	
	MIB_ShareTableElement ++ ;  // increment table entry
     }
     SafeFree(MIB_ShareTable.Table) ;	// free the base Table
     MIB_ShareTable.Table = NULL ;	// just for safety
     MIB_ShareTable.Len = 0 ;		// just for safety


   	
#if 0 // Done above
   // init the length
   MIB_ShareTable.Len = 0;
#endif
   First_of_this_block = 0;
   	
   do {  //  as long as there is more data to process

    lmCode =
	     NetShareEnum(NULL,      // local server
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
   	
   	if(0 == MIB_ShareTable.Len) {  // 1st time, alloc the whole table
   		// alloc the table space
                MIB_ShareTable.Table = SnmpUtilMemAlloc(totalentries *
   						sizeof(SHARE_ENTRY) );
   	}
	
	MIB_ShareTableElement = MIB_ShareTable.Table + First_of_this_block ;
	
   	for(i=0; i<entriesread; i++) {  // once for each entry in the buffer
   		// increment the entry number
   		
   		MIB_ShareTable.Len ++;
   		
   		// Stuff the data into each item in the table
   		
   		// share name
   		MIB_ShareTableElement->svShareName.dynamic = TRUE;
		
		#ifdef UNICODE
		if (SnmpUtilUnicodeToUTF8(
			&MIB_ShareTableElement->svShareName.stream,
   			DataTable->shi2_netname,
			TRUE))
        {
            MIB_ShareTableElement->svShareName.length = 0;
            MIB_ShareTableElement->svShareName.stream = NULL;
        }
        else
        {
            MIB_ShareTableElement->svShareName.length = 
                strlen(MIB_ShareTableElement->svShareName.stream);
        }
		#else
        MIB_ShareTableElement->svShareName.stream = SnmpUtilMemAlloc (
   				strlen( DataTable->shi2_netname ) + 1 ) ;
   		MIB_ShareTableElement->svShareName.length =
   				strlen( DataTable->shi2_netname ) ;
   		memcpy(	MIB_ShareTableElement->svShareName.stream,
   			DataTable->shi2_netname,
   			strlen( DataTable->shi2_netname ) ) ;
   		#endif
   		
   		// Share Path
   		MIB_ShareTableElement->svSharePath.dynamic = TRUE;
   		
		#ifdef UNICODE
		if (SnmpUtilUnicodeToUTF8(
			&MIB_ShareTableElement->svSharePath.stream,
   			DataTable->shi2_path,
			TRUE))
        {
            MIB_ShareTableElement->svSharePath.length = 0;
            MIB_ShareTableElement->svSharePath.stream = NULL;
        }
        else
        {
            MIB_ShareTableElement->svSharePath.length =
                strlen(MIB_ShareTableElement->svSharePath.stream);
        }
		#else
        MIB_ShareTableElement->svSharePath.stream = SnmpUtilMemAlloc (
   				strlen( DataTable->shi2_path ) + 1 ) ;
   		MIB_ShareTableElement->svSharePath.length =
   				strlen( DataTable->shi2_path ) ;

   		memcpy(	MIB_ShareTableElement->svSharePath.stream,
   			DataTable->shi2_path,
   			strlen( DataTable->shi2_path ) ) ;
   		#endif
   		
   		// Share Comment/Remark
   		MIB_ShareTableElement->svShareComment.dynamic = TRUE;
   		
		#ifdef UNICODE
		if (SnmpUtilUnicodeToUTF8(
			&MIB_ShareTableElement->svShareComment.stream,
   			DataTable->shi2_remark,
			TRUE))
        {
            MIB_ShareTableElement->svShareComment.length = 0;
            MIB_ShareTableElement->svShareComment.stream = NULL;
        }
        else
        {
            MIB_ShareTableElement->svShareComment.length =
                strlen(MIB_ShareTableElement->svShareComment.stream);
        }
		#else
        MIB_ShareTableElement->svShareComment.stream = SnmpUtilMemAlloc (
   				strlen( DataTable->shi2_remark ) + 1 ) ;
   		MIB_ShareTableElement->svShareComment.length =
   				strlen( DataTable->shi2_remark ) ;

   		memcpy(	MIB_ShareTableElement->svShareComment.stream,
   			DataTable->shi2_remark,
   			strlen( DataTable->shi2_remark ) ) ;
   		#endif
   		
   		DataTable ++ ;  // advance pointer to next shar entry in buffer
		MIB_ShareTableElement ++ ;  // and table entry
		
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
    build_shar_entry_oids();

   // Sort the table information using MSC QuickSort routine
   qsort( &MIB_ShareTable.Table[0], MIB_ShareTable.Len,
          sizeof(SHARE_ENTRY), shar_entry_cmp );

   //
   //
   // Cache table
   //
   //


   if(0 != MIB_ShareTable.Len) {
   	
   	cache_table[C_SHAR_TABLE].acquisition_time = curr_time ;

   	cache_table[C_SHAR_TABLE].bufptr = bufptr ;
   }

   //
   //
   // Return piece of information requested
   //
   //

Exit:
   return nResult;
} // MIB_shar_get

//
// MIB_shar_cmp
//    Routine for sorting the sharion table.
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
int __cdecl shar_entry_cmp(
       IN const SHARE_ENTRY *A,
       IN const SHARE_ENTRY *B
       )

{
   // Compare the OID's
   return SnmpUtilOidCmp( (AsnObjectIdentifier *)&A->Oid,
                       (AsnObjectIdentifier *)&B->Oid );
} // MIB_shar_cmp


//
//    None.
//
void build_shar_entry_oids(
       )

{
AsnOctetString OSA ;
SHARE_ENTRY *ShareEntry ;
unsigned i;

// start pointer at 1st guy in the table
ShareEntry = MIB_ShareTable.Table ;

// now iterate over the table, creating an oid for each entry
for( i=0; i<MIB_ShareTable.Len ; i++)  {
   // for each entry in the sharion table

   OSA.stream = ShareEntry->svShareName.stream ;
   OSA.length =  ShareEntry->svShareName.length ;
   OSA.dynamic = FALSE;

   // Make the entry's OID from string index
   MakeOidFromStr( &OSA, &ShareEntry->Oid );

   ShareEntry++; // point to the next guy in the table

   } // for

} // build_shar_entry_oids


VOID
AdminFilter(
    DWORD           Level,
    LPDWORD         pEntriesRead,
    LPBYTE          ShareInfo
    )

/*++

Routine Description:

    This function filters out the admin shares (ones denoted by a
    a $ as the last character in the name) from a NetShareEnum
    buffer.

    This function only supports info levels 0,1, and 2.  If any other
    level is passed in, the function doesn't perform the filter
    operation.

Arguments:

    Level - Indicates the info level of the enumeration buffer passed in.

    pEntriesRead - Pointer to a location which on entry indicates the
        number of entries to be filtered.  On exit it will indicate
        the number of entries after filtering.

    ShareInfo - Pointer to the buffer containing the enumerated structures.

Return Value:

    none.

--*/
{
    LPBYTE          pFiltered = ShareInfo;
    DWORD           filteredEntries=0;
    DWORD           i;
    DWORD           entrySize;
    DWORD           namePtrOffset;
    LPWSTR          pName;

    switch(Level) {
    case 0:
        entrySize = sizeof(SHARE_INFO_0);
        namePtrOffset = (DWORD)((LPBYTE)&(((LPSHARE_INFO_0)ShareInfo)->shi0_netname) -
                         ShareInfo);
        break;
    case 1:
        entrySize = sizeof(SHARE_INFO_1);
        namePtrOffset = (DWORD)((LPBYTE)&(((LPSHARE_INFO_1)ShareInfo)->shi1_netname) -
                         ShareInfo);
        break;
    case 2:
        entrySize = sizeof(SHARE_INFO_2);
        namePtrOffset = (DWORD)((LPBYTE)&(((LPSHARE_INFO_2)ShareInfo)->shi2_netname) -
                         ShareInfo);
        break;
    default:
        return;
    }

    for (i=0; i < *pEntriesRead; i++) {
        pName = *((LPWSTR *)(ShareInfo+namePtrOffset));
        if (pName[wcslen(pName)-1] != L'$') {
            filteredEntries++;
            if (pFiltered != ShareInfo) {
                memcpy(pFiltered, ShareInfo,entrySize);
            }
            pFiltered += entrySize;
        }
        ShareInfo += entrySize;
    }
    *pEntriesRead = filteredEntries;
}
//-------------------------------- END --------------------------------------
