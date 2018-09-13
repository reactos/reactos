/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    sess_tbl.c

Abstract:

    All routines to support opertions on the LM MIB session table.

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

#include "sess_tbl.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

   // Prefix to the Session table
static UINT                sessSubids[] = { 2, 20, 1 };
static AsnObjectIdentifier MIB_SessPrefix = { 3, sessSubids };

SESSION_TABLE MIB_SessionTable = { 0, NULL };

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define SESS_FIELD_SUBID       (MIB_SessPrefix.idLength+MIB_OidPrefix.idLength)

#define SESS_FIRST_FIELD       SESS_CLIENT_FIELD
#define SESS_LAST_FIELD        SESS_STATE_FIELD

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

UINT MIB_sess_get(
        IN OUT RFC1157VarBind *VarBind
        );

int MIB_sess_match(
       IN AsnObjectIdentifier *Oid,
       OUT UINT *Pos,
       IN BOOL Next
       );

UINT MIB_sess_copyfromtable(
        IN UINT Entry,
        IN UINT Field,
        OUT RFC1157VarBind *VarBind
        );

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// MIB_sess_func
//    High level routine for handling operations on the session table
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_sess_func(
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
         // Fill the Session table with the info from server
         if ( SNMPAPI_ERROR == MIB_sess_lmget() )
            {
            ErrStat = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         // If no elements in table, then return next MIB var, if one
         if ( MIB_SessionTable.Len == 0 )
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
         UINT temp_subs[] = { SESS_FIRST_FIELD };
         AsnObjectIdentifier FieldOid = { 1, temp_subs };


         SnmpUtilOidFree( &VarBind->name );
         SnmpUtilOidCpy( &VarBind->name, &MIB_OidPrefix );
         SnmpUtilOidAppend( &VarBind->name, &MIB_SessPrefix );
         SnmpUtilOidAppend( &VarBind->name, &FieldOid );
         SnmpUtilOidAppend( &VarBind->name, &MIB_SessionTable.Table[0].Oid );
         }

         //
         // Let fall through on purpose
         //

      case MIB_ACTION_GET:
         ErrStat = MIB_sess_get( VarBind );
         break;

      case MIB_ACTION_GETNEXT:
         // Fill the Session table with the info from server
         if ( SNMPAPI_ERROR == MIB_sess_lmget() )
            {
            ErrStat = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         // Determine which field
         Field = VarBind->name.ids[SESS_FIELD_SUBID];

        // Lookup OID in table
         if (Field < SESS_FIRST_FIELD)
         {
             Entry = 0;                 // will take the first entry into the table
             Field = SESS_FIRST_FIELD;  // and the first column of the table
             Found = MIB_TBL_POS_BEFORE;
         }
         else if (Field > SESS_LAST_FIELD)
             Found = MIB_TBL_POS_END;
         else
             Found = MIB_sess_match( &VarBind->name, &Entry, TRUE );

         // Index not found, but could be more fields to base GET on
         if ((Found == MIB_TBL_POS_BEFORE && MIB_SessionTable.Len == 0) ||
              Found == MIB_TBL_POS_END )
            {
            // Index not found in table, get next from field
//            Field ++;

            // Make sure not past last field
//            if ( Field > SESS_LAST_FIELD )
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
            if ( Entry > MIB_SessionTable.Len-1 )
               {
               Entry = 0;
               Field ++;

               /* item not implemented. Skip */

               if (Field == SESS_NUMCONS_FIELD) {
                   Field++;
               }

               if ( Field > SESS_LAST_FIELD )
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
         SnmpUtilOidAppend( &VarBind->name, &MIB_SessPrefix );
         SnmpUtilOidAppend( &VarBind->name, &FieldOid );
         SnmpUtilOidAppend( &VarBind->name, &MIB_SessionTable.Table[Entry].Oid );
         }

         ErrStat = MIB_sess_copyfromtable( Entry, Field, VarBind );

         break;

      case MIB_ACTION_SET:
         // Make sure OID is long enough
         if ( SESS_FIELD_SUBID + 1 > VarBind->name.idLength )
            {
            ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
            goto Exit;
            }

         // Get field number
         Field = VarBind->name.ids[SESS_FIELD_SUBID];

         // If the field being set is not the STATE field, error
         if ( Field != SESS_STATE_FIELD )
            {
            ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
            goto Exit;
            }

         // Check for proper type before setting
         if ( ASN_INTEGER != VarBind->value.asnType )
            {
            ErrStat = SNMP_ERRORSTATUS_BADVALUE;
            goto Exit;
            }

         // Make sure that the value is valid
         if ( VarBind->value.asnValue.number < SESS_STATE_ACTIVE &&
              VarBind->value.asnValue.number > SESS_STATE_DELETED )
            {
            ErrStat = SNMP_ERRORSTATUS_BADVALUE;
            goto Exit;
            }

         ErrStat = MIB_sess_lmset( &VarBind->name, Field, &VarBind->value );

         break;

      default:
         ErrStat = SNMP_ERRORSTATUS_GENERR;
      }

Exit:
   return ErrStat;
} // MIB_sess_func



//
// MIB_sess_get
//    Retrieve session table information.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_sess_get(
        IN OUT RFC1157VarBind *VarBind
        )

{
UINT   Entry;
int    Found;
UINT   ErrStat;

   if (VarBind->name.ids[SESS_FIELD_SUBID] < SESS_FIRST_FIELD ||
       VarBind->name.ids[SESS_FIELD_SUBID] > SESS_LAST_FIELD)
       {
       ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
       goto Exit;
       }

   // Fill the Session table with the info from server
   if ( SNMPAPI_ERROR == MIB_sess_lmget() )
      {
      ErrStat = SNMP_ERRORSTATUS_GENERR;
      goto Exit;
      }

   Found = MIB_sess_match( &VarBind->name, &Entry, FALSE );

   // Look for a complete OID match
   if ( Found != MIB_TBL_POS_FOUND )
      {
      ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
      goto Exit;
      }

   if ( VarBind->name.ids[SESS_FIELD_SUBID] == SESS_NUMCONS_FIELD )
      {
      ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
      goto Exit;
      }

   // Copy data from table
   ErrStat = MIB_sess_copyfromtable( Entry, VarBind->name.ids[SESS_FIELD_SUBID],
                                     VarBind );

Exit:
   return ErrStat;
} // MIB_sess_get



//
// MIB_sess_match
//    Match the target OID with a location in the Session table
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None
//
int MIB_sess_match(
       IN AsnObjectIdentifier *Oid,
       OUT UINT *Pos,
       IN BOOL Next
       )

{
AsnObjectIdentifier TempOid;
int                 nResult;


   // Remove prefix including field reference
   TempOid.idLength = Oid->idLength - MIB_OidPrefix.idLength -
                      MIB_SessPrefix.idLength - 1;
   TempOid.ids = &Oid->ids[MIB_OidPrefix.idLength+MIB_SessPrefix.idLength+1];

   *Pos = 0;
   while ( *Pos < MIB_SessionTable.Len )
      {
      nResult = SnmpUtilOidCmp( &TempOid, &MIB_SessionTable.Table[*Pos].Oid );
      if ( !nResult )
         {
         nResult = MIB_TBL_POS_FOUND;
         if (Next) {
             while ( ( (*Pos) + 1 < MIB_SessionTable.Len ) &&
                     !SnmpUtilOidCmp( &TempOid, &MIB_SessionTable.Table[(*Pos)+1].Oid)) {
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
}



//
// MIB_sess_copyfromtable
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
UINT MIB_sess_copyfromtable(
        IN UINT Entry,
        IN UINT Field,
        OUT RFC1157VarBind *VarBind
        )

{
UINT ErrStat;


   // Get the requested field and save in var bind
   switch( Field )
      {
      case SESS_CLIENT_FIELD:
         // Alloc space for string
         VarBind->value.asnValue.string.stream = SnmpUtilMemAlloc( sizeof(char)
                       * MIB_SessionTable.Table[Entry].svSesClientName.length );
         if ( VarBind->value.asnValue.string.stream == NULL )
            {
            ErrStat = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         // Copy string into return position
         memcpy( VarBind->value.asnValue.string.stream,
                       MIB_SessionTable.Table[Entry].svSesClientName.stream,
                       MIB_SessionTable.Table[Entry].svSesClientName.length );

         // Set string length
         VarBind->value.asnValue.string.length =
                          MIB_SessionTable.Table[Entry].svSesClientName.length;
         VarBind->value.asnValue.string.dynamic = TRUE;

         // Set type of var bind
         VarBind->value.asnType = ASN_RFC1213_DISPSTRING;
         break;

      case SESS_USER_FIELD:
         // Alloc space for string
         VarBind->value.asnValue.string.stream = SnmpUtilMemAlloc( sizeof(char)
                       * MIB_SessionTable.Table[Entry].svSesUserName.length );
         if ( VarBind->value.asnValue.string.stream == NULL )
            {
            ErrStat = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         // Copy string into return position
         memcpy( VarBind->value.asnValue.string.stream,
                       MIB_SessionTable.Table[Entry].svSesUserName.stream,
                       MIB_SessionTable.Table[Entry].svSesUserName.length );

         // Set string length
         VarBind->value.asnValue.string.length =
                          MIB_SessionTable.Table[Entry].svSesUserName.length;
         VarBind->value.asnValue.string.dynamic = TRUE;

         // Set type of var bind
         VarBind->value.asnType = ASN_RFC1213_DISPSTRING;
         break;

      case SESS_NUMCONS_FIELD:
         VarBind->value.asnValue.number =
                               MIB_SessionTable.Table[Entry].svSesNumConns;
         VarBind->value.asnType = ASN_INTEGER;
         break;

      case SESS_NUMOPENS_FIELD:
         VarBind->value.asnValue.number =
                               MIB_SessionTable.Table[Entry].svSesNumOpens;
         VarBind->value.asnType = ASN_INTEGER;
         break;

      case SESS_TIME_FIELD:
         VarBind->value.asnValue.number =
                               MIB_SessionTable.Table[Entry].svSesTime;
         VarBind->value.asnType = ASN_RFC1155_COUNTER;
         break;

      case SESS_IDLETIME_FIELD:
         VarBind->value.asnValue.number =
                               MIB_SessionTable.Table[Entry].svSesIdleTime;
         VarBind->value.asnType = ASN_RFC1155_COUNTER;
         break;

      case SESS_CLIENTTYPE_FIELD:
         VarBind->value.asnValue.number =
                               MIB_SessionTable.Table[Entry].svSesClientType;
         VarBind->value.asnType = ASN_INTEGER;
         break;

      case SESS_STATE_FIELD:
         VarBind->value.asnValue.number =
                               MIB_SessionTable.Table[Entry].svSesState;
         VarBind->value.asnType = ASN_INTEGER;
         break;

      default:
         SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: Internal Error Session Table\n" ));
         ErrStat = SNMP_ERRORSTATUS_GENERR;

         goto Exit;
      }

   ErrStat = SNMP_ERRORSTATUS_NOERROR;

Exit:
   return ErrStat;
} // MIB_sess_copyfromtable

//-------------------------------- END --------------------------------------
