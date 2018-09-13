/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    user_tbl.c

Abstract:

    All routines to perform operations on the User Table.

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

#include "user_tbl.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

   // Prefix to the Users table
static UINT                userSubids[] = { 2, 25, 1 };
static AsnObjectIdentifier MIB_UserPrefix = { 3, userSubids };

USER_TABLE MIB_UserTable = { 0, NULL };

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define USER_FIELD_SUBID       (MIB_UserPrefix.idLength+MIB_OidPrefix.idLength)

#define USER_FIRST_FIELD       USER_NAME_FIELD
#define USER_LAST_FIELD        USER_NAME_FIELD

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

UINT MIB_users_get(
        IN OUT RFC1157VarBind *VarBind
	);

int MIB_users_match(
       IN AsnObjectIdentifier *Oid,
       OUT UINT *Pos
       );

UINT MIB_users_copyfromtable(
        IN UINT Entry,
        IN UINT Field,
        OUT RFC1157VarBind *VarBind
        );

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// MIB_users_func
//    High level routine for handling operations on the Users table
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_users_func(
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
         // Fill the Users Table with the info from server
         if ( SNMPAPI_ERROR == MIB_users_lmget() )
	    {
	    ErrStat = SNMP_ERRORSTATUS_GENERR;
	    goto Exit;
	    }

         // If no elements in table, then return next MIB var, if one
         if ( MIB_UserTable.Len == 0 )
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
         UINT temp_subs[] = { USER_FIRST_FIELD };
         AsnObjectIdentifier FieldOid = { 1, temp_subs };


         SnmpUtilOidFree( &VarBind->name );
         SnmpUtilOidCpy( &VarBind->name, &MIB_OidPrefix );
         SnmpUtilOidAppend( &VarBind->name, &MIB_UserPrefix );
         SnmpUtilOidAppend( &VarBind->name, &FieldOid );
         SnmpUtilOidAppend( &VarBind->name, &MIB_UserTable.Table[0].Oid );
         }

         //
         // Let fall through on purpose
         //

      case MIB_ACTION_GET:
         ErrStat = MIB_users_get( VarBind );
	 break;

      case MIB_ACTION_GETNEXT:
         // Fill the Users table with the info from server
         if ( SNMPAPI_ERROR == MIB_users_lmget() )
	    {
	    ErrStat = SNMP_ERRORSTATUS_GENERR;
	    goto Exit;
	    }

         // Determine which field
         Field = VarBind->name.ids[USER_FIELD_SUBID];

         // Lookup OID in table
         if (Field < USER_FIRST_FIELD)
         {
             Entry = 0;                 // will take the first entry into the table
             Field = USER_FIRST_FIELD;  // and the first column of the table
             Found = MIB_TBL_POS_BEFORE;
         }
         else if (Field > USER_LAST_FIELD)
             Found = MIB_TBL_POS_END;
         else
             Found = MIB_users_match( &VarBind->name, &Entry );

         // Index not found, but could be more fields to base GET on
         if ((Found == MIB_TBL_POS_BEFORE && MIB_UserTable.Len == 0) ||
              Found == MIB_TBL_POS_END )
            {
            // Index not found in table, get next from field
//            Field ++;

            // Make sure not past last field
//            if ( Field > USER_LAST_FIELD )
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
            if ( Entry > MIB_UserTable.Len-1 )
               {
               Entry = 0;
               Field ++;
               if ( Field > USER_LAST_FIELD )
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
         SnmpUtilOidAppend( &VarBind->name, &MIB_UserPrefix );
         SnmpUtilOidAppend( &VarBind->name, &FieldOid );
         SnmpUtilOidAppend( &VarBind->name, &MIB_UserTable.Table[Entry].Oid );
         }

         ErrStat = MIB_users_copyfromtable( Entry, Field, VarBind );

         break;

      case MIB_ACTION_SET:
         ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
	 break;

      default:
         ErrStat = SNMP_ERRORSTATUS_GENERR;
      }

Exit:
   return ErrStat;
} // MIB_users_func



//
// MIB_users_get
//    Retrieve Users table information.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_users_get(
        IN OUT RFC1157VarBind *VarBind
	)

{
UINT   Entry;
int    Found;
UINT   ErrStat;

   if (VarBind->name.ids[USER_FIELD_SUBID] < USER_FIRST_FIELD ||
       VarBind->name.ids[USER_FIELD_SUBID] > USER_LAST_FIELD)
   {
       ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
       goto Exit;
   }

   // Fill the Users table with the info from server
   if ( SNMPAPI_ERROR == MIB_users_lmget() )
      {
      ErrStat = SNMP_ERRORSTATUS_GENERR;
      goto Exit;
      }

   Found = MIB_users_match( &VarBind->name, &Entry );

   // Look for a complete OID match
   if ( Found != MIB_TBL_POS_FOUND )
      {
      ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
      goto Exit;
      }

   // Copy data from table
   ErrStat = MIB_users_copyfromtable( Entry,
                                      VarBind->name.ids[USER_FIELD_SUBID],
                                      VarBind );

Exit:
   return ErrStat;
} // MIB_users_get



//
// MIB_users_match
//    Match the target OID with a location in the Users table
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None
//
int MIB_users_match(
       IN AsnObjectIdentifier *Oid,
       OUT UINT *Pos
       )

{
AsnObjectIdentifier TempOid;
int                 nResult;


   // Remove prefix including field reference
   TempOid.idLength = Oid->idLength - MIB_OidPrefix.idLength -
                      MIB_UserPrefix.idLength - 1;
   TempOid.ids = &Oid->ids[MIB_OidPrefix.idLength+MIB_UserPrefix.idLength+1];

   *Pos = 0;
   while ( *Pos < MIB_UserTable.Len )
      {
      nResult = SnmpUtilOidCmp( &TempOid, &MIB_UserTable.Table[*Pos].Oid );
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
// MIB_users_copyfromtable
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
UINT MIB_users_copyfromtable(
        IN UINT Entry,
        IN UINT Field,
        OUT RFC1157VarBind *VarBind
        )

{
UINT ErrStat;


   // Get the requested field and save in var bind
   switch( Field )
      {
      case USER_NAME_FIELD:
         // Alloc space for string
         VarBind->value.asnValue.string.stream = SnmpUtilMemAlloc( sizeof(char)
                       * MIB_UserTable.Table[Entry].svUserName.length );
         if ( VarBind->value.asnValue.string.stream == NULL )
            {
            ErrStat = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         // Copy string into return position
         memcpy( VarBind->value.asnValue.string.stream,
                       MIB_UserTable.Table[Entry].svUserName.stream,
                       MIB_UserTable.Table[Entry].svUserName.length );

         // Set string length
         VarBind->value.asnValue.string.length =
                          MIB_UserTable.Table[Entry].svUserName.length;
         VarBind->value.asnValue.string.dynamic = TRUE;

         // Set type of var bind
         VarBind->value.asnType = ASN_RFC1213_DISPSTRING;
         break;

      default:
         SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: Internal Error Users Table\n" ));
         ErrStat = SNMP_ERRORSTATUS_GENERR;

         goto Exit;
      }

   ErrStat = SNMP_ERRORSTATUS_NOERROR;

Exit:
   return ErrStat;
} // MIB_users_copyfromtable
