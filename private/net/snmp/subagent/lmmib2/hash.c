/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    hash.c

Abstract:

    Hash Table and support functions.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <stdio.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>

#include "mib.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include "hash.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define HT_SIZE    101
#define HT_RADIX   18

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

   // Structure of one node in the hash table
typedef struct hash_node
           {
	   MIB_ENTRY        *MibEntry;
	   struct hash_node *Next;
	   } HASH_NODE;

   // Hash table definition
HASH_NODE *MIB_HashTable[HT_SIZE];

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// MIB_HashInit
//    Initializes hash table.
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
SNMPAPI MIB_HashInit()

{
UINT      I;
UINT      HashRes;
HASH_NODE *ht_ptr;
SNMPAPI   nResult;


   // Initialize hash table
   for ( I=0;I < HT_SIZE;I++ )
      {
      MIB_HashTable[I] = NULL;
      }

   // Loop MIB hashing the OID's to find position in Hash Table
   for ( I=0;I < MIB_num_variables;I++ )
      {
      HashRes = MIB_Hash( &Mib[I].Oid );

      // Check for empty bucket
      if ( MIB_HashTable[HashRes] == NULL )
         {
	 // Allocate first node in bucket
         MIB_HashTable[HashRes] = SnmpUtilMemAlloc( sizeof(HASH_NODE) );
	 if ( MIB_HashTable[HashRes] == NULL )
	    {
	    SetLastError( SNMP_MEM_ALLOC_ERROR );

	    nResult = SNMPAPI_ERROR;
	    goto Exit;
	    }

	 // Make copy of position in hash table to save MIB entry
	 ht_ptr = MIB_HashTable[HashRes];
	 }
      else
         {
	 // Find end of bucket
	 ht_ptr = MIB_HashTable[HashRes];
	 while ( ht_ptr->Next != NULL )
	    {
	    ht_ptr = ht_ptr->Next;
	    }

	 // Alloc space for next node
         ht_ptr->Next = SnmpUtilMemAlloc( sizeof(HASH_NODE) );
	 if ( ht_ptr->Next == NULL )
	    {
	    SetLastError( SNMP_MEM_ALLOC_ERROR );

	    nResult = SNMPAPI_ERROR;
	    goto Exit;
	    }

	 // Make copy of position in hash table to save MIB entry
	 ht_ptr = ht_ptr->Next;
	 }

      // Save MIB Entry pointer
      ht_ptr->MibEntry = &Mib[I];
      ht_ptr->Next     = NULL;
      }

Exit:
   return nResult;
} // MIB_HashInit



//
// MIB_Hash
//    Hash an Object Identifier to find its position in the Hash Table.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_Hash(
        IN AsnObjectIdentifier *Oid // OID to hash
	)

{
long I;
UINT Sum;


   Sum = 0;
   for ( I=0;I < (long)Oid->idLength-1;I++ )
      {
      Sum = Sum * HT_RADIX + Oid->ids[I+1];
      }

   return Sum % HT_SIZE;
} // MIB_Hash



//
// MIB_HashLookup
//    Lookup OID in Hash Table and return pointer to MIB Entry.
//
// Notes:
//
// Return Codes:
//    NULL - OID not present in Hash Table.
//
// Error Codes:
//    None.
//
MIB_ENTRY *MIB_HashLookup(
              IN AsnObjectIdentifier *Oid // OID to lookup
	      )

{
HASH_NODE *ht_ptr;
MIB_ENTRY *pResult;
UINT      HashPos;


   // Hash OID to find position in Hash Table
   HashPos = MIB_Hash( Oid );

   // Search hash bucket for match
   ht_ptr = MIB_HashTable[HashPos];
   while ( ht_ptr != NULL )
      {
      if ( !SnmpUtilOidCmp(Oid, &ht_ptr->MibEntry->Oid) )
         {
	 pResult = ht_ptr->MibEntry;
	 goto Exit;
	 }

      ht_ptr = ht_ptr->Next;
      }

   // Check for not found error
   if ( ht_ptr == NULL )
      {
      pResult = NULL;
      }

Exit:
   return pResult;
} // MIB_HashLookup



#if 0 // Left in in case of more testing on hash performance
//
//
// Debugging Code
//
//

void MIB_HashPerformance()

{
UINT I;
UINT LargestBucket;
UINT BucketSize;
HASH_NODE *ht_ptr;
ULONG Sum;
ULONG Count;


   SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: Hash Performance Report\n" );

   LargestBucket = 0;
   Count         = 0;
   Sum           = 0;
   for ( I=0;I < HT_SIZE;I++ )
      {
      BucketSize = 0;
      ht_ptr     = MIB_HashTable[I];

      // Count nodes in bucket
      while ( ht_ptr != NULL )
         {
	 BucketSize++;
	 ht_ptr = ht_ptr->Next;
	 }

      if ( BucketSize )
         {
	 SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2:   %d -- Bucket Size:  %d\n", I, BucketSize ));

         Sum += BucketSize;
	 Count ++;

         LargestBucket = max( LargestBucket, BucketSize );
	 }
      }

   SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2:   Number of Buckets:  %d\n", HT_SIZE ));
   SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2:   Number of MIB Var:  %d\n", MIB_num_variables ));
   SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2:   Hashing Radix    :  %d\n", HashRadix ));

   SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2:   Used bucket Count:  %d\n", Count ));
   SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2:   Avg. Bucket Size :  %d\n", Sum / Count ));
   SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2:   Larg. bucket Size:  %d\n", LargestBucket ));
} // MIB_HashPerformance
#endif
//-------------------------------- END --------------------------------------
