/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    prnt_tbl.c

Abstract:

    Routines supporting operations on the Print Queue table.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <stdio.h>
#include <memory.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <snmputil.h>

#include "mibfuncs.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include "prnt_tbl.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

   // Prefix to the Print Queue table
static UINT                printQSubids[] = { 2, 29, 1 };
static AsnObjectIdentifier MIB_PrintQPrefix = { 3, printQSubids };

PRINTQ_TABLE     MIB_PrintQTable = { 0, NULL };

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define PRNTQ_FIELD_SUBID     (MIB_PrintQPrefix.idLength+MIB_OidPrefix.idLength)

#define PRNTQ_FIRST_FIELD      PRNTQ_NAME_FIELD
#define PRNTQ_LAST_FIELD       PRNTQ_JOBS_FIELD

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

UINT MIB_prntq_get(
        IN OUT RFC1157VarBind *VarBind
        );

int MIB_prntq_match(
       IN AsnObjectIdentifier *Oid,
       OUT UINT *Pos
       );

UINT MIB_prntq_copyfromtable(
        IN UINT Entry,
        IN UINT Field,
        OUT RFC1157VarBind *VarBind
        );

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// MIB_prntq_func
//    High level routine for handling operations on the print queue table
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_prntq_func(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN OUT RFC1157VarBind *VarBind
        )

{
int     Found;
UINT    Entry;
UINT    Field;
UINT    ErrStat;


   switch ( Action )
      {
      case MIB_ACTION_GETFIRST:
         // Fill the Print Queue table with the info from server
         if ( SNMPAPI_ERROR == MIB_prntq_lmget() )
	    {
	    ErrStat = SNMP_ERRORSTATUS_GENERR;
	    goto Exit;
	    }

         // If no elements in table, then return next MIB var, if one
         if ( MIB_PrintQTable.Len == 0 )
            {
            if ( MibPtr->MibNext == NULL )
               {
               ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
               goto Exit;
               }

            // Do get first on the next MIB var
            ErrStat = (*MibPtr->MibNext->MibFunc)( Action, MibPtr->MibNext,
                                                   VarBind );
            break;
            }

         //
         // Place correct OID in VarBind
         // Assuming the first field in the first record is the "start"
         {
         UINT temp_subs[] = { PRNTQ_FIRST_FIELD };
         AsnObjectIdentifier FieldOid = { 1, temp_subs };


         SnmpUtilOidFree( &VarBind->name );
         SnmpUtilOidCpy( &VarBind->name, &MIB_OidPrefix );
         SnmpUtilOidAppend( &VarBind->name, &MIB_PrintQPrefix );
         SnmpUtilOidAppend( &VarBind->name, &FieldOid );
         SnmpUtilOidAppend( &VarBind->name, &MIB_PrintQTable.Table[0].Oid );
         }

         //
         // Let fall through on purpose
         //

      case MIB_ACTION_GET:
         ErrStat = MIB_prntq_get( VarBind );
	 break;

      case MIB_ACTION_GETNEXT:
         // Fill the Print Queue table with the info from server
         if ( SNMPAPI_ERROR == MIB_prntq_lmget() )
	    {
	    ErrStat = SNMP_ERRORSTATUS_GENERR;
	    goto Exit;
	    }

         // Determine which field
         Field = VarBind->name.ids[PRNTQ_FIELD_SUBID];

        // Lookup OID in table
         if (Field < PRNTQ_FIRST_FIELD)
         {
             Entry = 0;                 // will take the first entry into the table
             Field = PRNTQ_FIRST_FIELD;  // and the first column of the table
             Found = MIB_TBL_POS_BEFORE;
         }
         else if (Field > PRNTQ_LAST_FIELD)
             Found = MIB_TBL_POS_END;
         else
             Found = MIB_prntq_match( &VarBind->name, &Entry );

         // Index not found, but could be more fields to base GET on
         if ((Found == MIB_TBL_POS_BEFORE && MIB_PrintQTable.Len == 0) ||
              Found == MIB_TBL_POS_END )
            {
            // Index not found in table, get next from field
//            Field ++;

            // Make sure not past last field
//            if ( Field > PRNTQ_LAST_FIELD )
//               {
               // Get next VAR in MIB
               ErrStat = (*MibPtr->MibNext->MibFunc)( MIB_ACTION_GETFIRST,
                                                      MibPtr->MibNext,
                                                      VarBind );
               break;
//               }
            }

         // Get next TABLE entry
         if ( Found == MIB_TBL_POS_FOUND )
            {
            Entry ++;
            if ( Entry > MIB_PrintQTable.Len-1 )
               {
               Entry = 0;
               Field ++;
               if ( Field > PRNTQ_LAST_FIELD )
                  {
                  // Get next VAR in MIB
                  ErrStat = (*MibPtr->MibNext->MibFunc)( MIB_ACTION_GETFIRST,
                                                         MibPtr->MibNext,
                                                         VarBind );
                  break;
                  }
               }
            }

         //
         // Place correct OID in VarBind
         // Assuming the first field in the first record is the "start"
         {
         UINT temp_subs[1];
         AsnObjectIdentifier FieldOid;

         temp_subs[0]      = Field;
         FieldOid.idLength = 1;
         FieldOid.ids      = temp_subs;

         SnmpUtilOidFree( &VarBind->name );
         SnmpUtilOidCpy( &VarBind->name, &MIB_OidPrefix );
         SnmpUtilOidAppend( &VarBind->name, &MIB_PrintQPrefix );
         SnmpUtilOidAppend( &VarBind->name, &FieldOid );
         SnmpUtilOidAppend( &VarBind->name, &MIB_PrintQTable.Table[Entry].Oid );
         }

         ErrStat = MIB_prntq_copyfromtable( Entry, Field, VarBind );

         break;

      case MIB_ACTION_SET:
         ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
	 break;

      default:
         ErrStat = SNMP_ERRORSTATUS_GENERR;
      }

Exit:
   return ErrStat;
} // MIB_prntq_func



//
// MIB_prntq_get
//    Retrieve print queue table information.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_prntq_get(
        IN OUT RFC1157VarBind *VarBind
	)

{
UINT   Entry;
int    Found;
UINT   ErrStat;

   if (VarBind->name.ids[PRNTQ_FIELD_SUBID] < PRNTQ_FIRST_FIELD ||
       VarBind->name.ids[PRNTQ_FIELD_SUBID] > PRNTQ_LAST_FIELD)
       {
       ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
       goto Exit;
       }

   // Fill the Print Queue table with the info from server
   if ( SNMPAPI_ERROR == MIB_prntq_lmget() )
      {
      ErrStat = SNMP_ERRORSTATUS_GENERR;
      goto Exit;
      }

   Found = MIB_prntq_match( &VarBind->name, &Entry );

   // Look for a complete OID match
   if ( Found != MIB_TBL_POS_FOUND )
      {
      ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
      goto Exit;
      }

   // Copy data from table
   ErrStat = MIB_prntq_copyfromtable( Entry,
                                      VarBind->name.ids[PRNTQ_FIELD_SUBID],
                                      VarBind );

Exit:
   return ErrStat;
} // MIB_prntq_get



//
// MIB_prntq_match
//    Match the target OID with a location in the Print Queue table
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None
//
int MIB_prntq_match(
       IN AsnObjectIdentifier *Oid,
       OUT UINT *Pos
       )

{
AsnObjectIdentifier TempOid;
int                 nResult;


   // Remove prefix including field reference
   TempOid.idLength = Oid->idLength - MIB_OidPrefix.idLength -
                      MIB_PrintQPrefix.idLength - 1;
   TempOid.ids = &Oid->ids[MIB_OidPrefix.idLength+MIB_PrintQPrefix.idLength+1];

   *Pos = 0;
   while ( *Pos < MIB_PrintQTable.Len )
      {
      nResult = SnmpUtilOidCmp( &TempOid, &MIB_PrintQTable.Table[*Pos].Oid );
      if ( !nResult )
         {
         nResult = MIB_TBL_POS_FOUND;

         goto Exit;
         }

      if ( nResult < 0 )
         {
         nResult = MIB_TBL_POS_BEFORE;

         goto Exit;
         }

      (*Pos)++;
      }

   nResult = MIB_TBL_POS_END;

Exit:
   return nResult;
}



//
// MIB_prntq_copyfromtable
//    Copy requested data from table structure into Var Bind.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_prntq_copyfromtable(
        IN UINT Entry,
        IN UINT Field,
        OUT RFC1157VarBind *VarBind
        )

{
UINT ErrStat;


   // Get the requested field and save in var bind
   switch( Field )
      {
      case PRNTQ_NAME_FIELD:
         // Alloc space for string
         VarBind->value.asnValue.string.stream = SnmpUtilMemAlloc( sizeof(char)
                       * MIB_PrintQTable.Table[Entry].svPrintQName.length );
         if ( VarBind->value.asnValue.string.stream == NULL )
            {
            ErrStat = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         // Copy string into return position
         memcpy( VarBind->value.asnValue.string.stream,
                       MIB_PrintQTable.Table[Entry].svPrintQName.stream,
                       MIB_PrintQTable.Table[Entry].svPrintQName.length );

         // Set string length
         VarBind->value.asnValue.string.length =
                          MIB_PrintQTable.Table[Entry].svPrintQName.length;
         VarBind->value.asnValue.string.dynamic = TRUE;

         // Set type of var bind
         VarBind->value.asnType = ASN_RFC1213_DISPSTRING;
         break;

      case PRNTQ_JOBS_FIELD:
         VarBind->value.asnValue.number =
                               MIB_PrintQTable.Table[Entry].svPrintQNumJobs;
         VarBind->value.asnType = ASN_INTEGER;
         break;

      default:
         SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: Internal Error Print Queue Table\n" ));
         ErrStat = SNMP_ERRORSTATUS_GENERR;

         goto Exit;
      }

   ErrStat = SNMP_ERRORSTATUS_NOERROR;

Exit:
   return ErrStat;
} // MIB_prntq_copyfromtable

//-------------------------------- END --------------------------------------
