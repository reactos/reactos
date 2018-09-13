/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    uses_tbl.c

Abstract:

    Routines to perform operations on the Workstation Uses table.

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

#include "uses_tbl.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

   // Prefix to the Uses table
static UINT                usesSubids[] = { 3, 8, 1 };
static AsnObjectIdentifier MIB_UsesPrefix = { 3, usesSubids };

WKSTA_USES_TABLE MIB_WkstaUsesTable = { 0, NULL };

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define USES_FIELD_SUBID       (MIB_UsesPrefix.idLength+MIB_OidPrefix.idLength)

#define USES_FIRST_FIELD       USES_LOCAL_FIELD
#define USES_LAST_FIELD        USES_STATUS_FIELD

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------


//--------------------------- PRIVATE PROTOTYPES ----------------------------

UINT MIB_wsuses_get(
        IN OUT RFC1157VarBind *VarBind
        );

int MIB_wsuses_match(
       IN AsnObjectIdentifier *Oid,
       OUT UINT *Pos,
       IN BOOL Next
       );

UINT MIB_wsuses_copyfromtable(
        IN UINT Entry,
        IN UINT Field,
        OUT RFC1157VarBind *VarBind
        );

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// MIB_wsuses_func
//    High level routine for handling operations on the uses table
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_wsuses_func(
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
         // Fill the Uses table with the info from server
         if ( SNMPAPI_ERROR == MIB_wsuses_lmget() )
            {
            ErrStat = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         // If no elements in table, then return next MIB var, if one
         if ( MIB_WkstaUsesTable.Len == 0 )
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
         UINT temp_subs[] = { USES_FIRST_FIELD };
         AsnObjectIdentifier FieldOid = { 1, temp_subs };


         SnmpUtilOidFree( &VarBind->name );
         SnmpUtilOidCpy( &VarBind->name, &MIB_OidPrefix );
         SnmpUtilOidAppend( &VarBind->name, &MIB_UsesPrefix );
         SnmpUtilOidAppend( &VarBind->name, &FieldOid );
         SnmpUtilOidAppend( &VarBind->name, &MIB_WkstaUsesTable.Table[0].Oid );
         }

         //
         // Let fall through on purpose
         //

      case MIB_ACTION_GET:
         ErrStat = MIB_wsuses_get( VarBind );
         break;

      case MIB_ACTION_GETNEXT:
         // Fill the Uses table with the info from server
         if ( SNMPAPI_ERROR == MIB_wsuses_lmget() )
            {
            ErrStat = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         // Determine which field
         Field = VarBind->name.ids[USES_FIELD_SUBID];

         // Lookup OID in table
         if (Field < USES_FIRST_FIELD)
         {
             Entry = 0;                 // will take the first entry into the table
             Field = USES_FIRST_FIELD;  // and the first column of the table
             Found = MIB_TBL_POS_BEFORE;
         }
         else if (Field > USES_LAST_FIELD)
             Found = MIB_TBL_POS_END;
         else
             Found = MIB_wsuses_match( &VarBind->name, &Entry, TRUE );

         // Index not found, but could be more fields to base GET on
         if ((Found == MIB_TBL_POS_BEFORE && MIB_WkstaUsesTable.Len == 0) ||
              Found == MIB_TBL_POS_END )
            {
            // Index not found in table, get next from field
//            Field ++;

            // Make sure not past last field
//            if ( Field > USES_LAST_FIELD )
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
            if ( Entry > MIB_WkstaUsesTable.Len-1 )
               {
               Entry = 0;
               Field ++;
               if ( Field > USES_LAST_FIELD )
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
         SnmpUtilOidAppend( &VarBind->name, &MIB_UsesPrefix );
         SnmpUtilOidAppend( &VarBind->name, &FieldOid );
         SnmpUtilOidAppend( &VarBind->name, &MIB_WkstaUsesTable.Table[Entry].Oid );
         }

         ErrStat = MIB_wsuses_copyfromtable( Entry, Field, VarBind );

         break;

      case MIB_ACTION_SET:
         ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
         break;

      default:
         ErrStat = SNMP_ERRORSTATUS_GENERR;
      }

Exit:
   return ErrStat;
} // MIB_wsuses_func



//
// MIB_wsuses_get
//    Retrieve Uses table information.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_wsuses_get(
        IN OUT RFC1157VarBind *VarBind
        )

{
UINT   Entry;
int    Found;
UINT   ErrStat;

   if (VarBind->name.ids[USES_FIELD_SUBID] < USES_FIRST_FIELD ||
       VarBind->name.ids[USES_FIELD_SUBID] > USES_LAST_FIELD)
       {
       ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
       goto Exit;
       }

   // Fill the Uses table with the info from server
   if ( SNMPAPI_ERROR == MIB_wsuses_lmget() )
      {
      ErrStat = SNMP_ERRORSTATUS_GENERR;
      goto Exit;
      }

   Found = MIB_wsuses_match( &VarBind->name, &Entry, FALSE );

   // Look for a complete OID match
   if ( Found != MIB_TBL_POS_FOUND )
      {
      ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
      goto Exit;
      }

   // Copy data from table
   ErrStat = MIB_wsuses_copyfromtable( Entry, VarBind->name.ids[USES_FIELD_SUBID],
                                     VarBind );

Exit:
   return ErrStat;
} // MIB_wsuses_get



//
// MIB_wsuses_match
//    Match the target OID with a location in the Uses table
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None
//
int MIB_wsuses_match(
       IN AsnObjectIdentifier *Oid,
       OUT UINT *Pos,
       IN BOOL Next
       )

{
AsnObjectIdentifier TempOid;
int                 nResult;

   // Remove prefix including field reference
   TempOid.idLength = Oid->idLength - MIB_OidPrefix.idLength -
                      MIB_UsesPrefix.idLength - 1;
   TempOid.ids = &Oid->ids[MIB_OidPrefix.idLength+MIB_UsesPrefix.idLength+1];

   *Pos = 0;
   while ( *Pos < MIB_WkstaUsesTable.Len )
      {
      nResult = SnmpUtilOidCmp( &TempOid, &MIB_WkstaUsesTable.Table[*Pos].Oid );
      if ( !nResult )
         {
         nResult = MIB_TBL_POS_FOUND;
         if (Next) {
             while ( ( (*Pos) + 1 < MIB_WkstaUsesTable.Len ) &&
                     !SnmpUtilOidCmp( &TempOid, &MIB_WkstaUsesTable.Table[(*Pos)+1].Oid)) {
                 (*Pos)++;
             }
         }

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
} // MIB_wsuses_match



//
// MIB_wsuses_copyfromtable
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
UINT MIB_wsuses_copyfromtable(
        IN UINT Entry,
        IN UINT Field,
        OUT RFC1157VarBind *VarBind
        )

{
UINT ErrStat;


   // Get the requested field and save in var bind
   switch( Field )
      {
      case USES_LOCAL_FIELD:
         // Alloc space for string
         VarBind->value.asnValue.string.stream = SnmpUtilMemAlloc( sizeof(char)
                       * MIB_WkstaUsesTable.Table[Entry].useLocalName.length );
         if ( VarBind->value.asnValue.string.stream == NULL )
            {
            ErrStat = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         // Copy string into return position
         memcpy( VarBind->value.asnValue.string.stream,
                       MIB_WkstaUsesTable.Table[Entry].useLocalName.stream,
                       MIB_WkstaUsesTable.Table[Entry].useLocalName.length );

         // Set string length
         VarBind->value.asnValue.string.length =
                          MIB_WkstaUsesTable.Table[Entry].useLocalName.length;
         VarBind->value.asnValue.string.dynamic = TRUE;

         // Set type of var bind
         VarBind->value.asnType = ASN_RFC1213_DISPSTRING;
         break;

      case USES_REMOTE_FIELD:
         // Alloc space for string
         VarBind->value.asnValue.string.stream = SnmpUtilMemAlloc( sizeof(char)
                       * MIB_WkstaUsesTable.Table[Entry].useRemote.length );
         if ( VarBind->value.asnValue.string.stream == NULL )
            {
            ErrStat = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         // Copy string into return position
         memcpy( VarBind->value.asnValue.string.stream,
                       MIB_WkstaUsesTable.Table[Entry].useRemote.stream,
                       MIB_WkstaUsesTable.Table[Entry].useRemote.length );

         // Set string length
         VarBind->value.asnValue.string.length =
                          MIB_WkstaUsesTable.Table[Entry].useRemote.length;
         VarBind->value.asnValue.string.dynamic = TRUE;

         // Set type of var bind
         VarBind->value.asnType = ASN_RFC1213_DISPSTRING;
         break;

      case USES_STATUS_FIELD:
         VarBind->value.asnValue.number =
                               MIB_WkstaUsesTable.Table[Entry].useStatus;
         VarBind->value.asnType = ASN_INTEGER;
         break;

      default:
         SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: Internal Error WorkstationUses Table\n" ));
         ErrStat = SNMP_ERRORSTATUS_GENERR;

         goto Exit;
      }

   ErrStat = SNMP_ERRORSTATUS_NOERROR;

Exit:
   return ErrStat;
} // MIB_wsuses_copyfromtable

//-------------------------------- END --------------------------------------
