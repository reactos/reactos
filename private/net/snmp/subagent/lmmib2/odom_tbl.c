/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    odom_tbl.c

Abstract:

    Routines supporting operations on the Other Domain Table.

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

#include "odom_tbl.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

   // Prefix to the Other Domain table
static UINT                odomSubids[] = { 4, 4, 1 };
static AsnObjectIdentifier MIB_DomOtherDomainPrefix = { 3, odomSubids };

DOM_OTHER_TABLE  MIB_DomOtherDomainTable = { 0, NULL };

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define ODOM_FIELD_SUBID       (MIB_DomOtherDomainPrefix.idLength + \
                                MIB_OidPrefix.idLength)

#define ODOM_FIRST_FIELD       ODOM_NAME_FIELD
#define ODOM_LAST_FIELD        ODOM_NAME_FIELD

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

UINT MIB_odoms_get(
        IN OUT RFC1157VarBind *VarBind
	);

UINT MIB_odoms_copyfromtable(
        IN UINT Entry,
        IN UINT Field,
        OUT RFC1157VarBind *VarBind
        );

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// MIB_odoms_func
//    High level routine for handling operations on the Other Domain table
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_odoms_func(
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
         // Fill the Other Domain table with the info from server
         if ( SNMPAPI_ERROR == MIB_odoms_lmget() )
	    {
	    ErrStat = SNMP_ERRORSTATUS_GENERR;
	    goto Exit;
	    }

         // If no elements in table, then return next MIB var, if one
         if ( MIB_DomOtherDomainTable.Len == 0 )
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
         UINT temp_subs[] = { ODOM_FIRST_FIELD };
         AsnObjectIdentifier FieldOid = { 1, temp_subs };


         SnmpUtilOidFree( &VarBind->name );
         SnmpUtilOidCpy( &VarBind->name, &MIB_OidPrefix );
         SnmpUtilOidAppend( &VarBind->name, &MIB_DomOtherDomainPrefix );
         SnmpUtilOidAppend( &VarBind->name, &FieldOid );
         SnmpUtilOidAppend( &VarBind->name, &MIB_DomOtherDomainTable.Table[0].Oid );
         }

         //
         // Let fall through on purpose
         //

      case MIB_ACTION_GET:
         ErrStat = MIB_odoms_get( VarBind );
	 break;

      case MIB_ACTION_GETNEXT:
         // Fill the Other Domain Table with the info from server
         if ( SNMPAPI_ERROR == MIB_odoms_lmget() )
	    {
	    ErrStat = SNMP_ERRORSTATUS_GENERR;
	    goto Exit;
	    }

         // Determine which field
         Field = VarBind->name.ids[ODOM_FIELD_SUBID];

         // Lookup OID in table
         if (Field < ODOM_FIRST_FIELD)
         {
             Entry = 0;                 // will take the first entry into the table
             Field = ODOM_FIRST_FIELD;  // and the first column of the table
             Found = MIB_TBL_POS_BEFORE;
         }
         else if (Field > ODOM_LAST_FIELD)
             Found = MIB_TBL_POS_END;
         else
             Found = MIB_odoms_match( &VarBind->name, &Entry );

         // Index not found, but could be more fields to base GET on
         if ((Found == MIB_TBL_POS_BEFORE && MIB_DomOtherDomainTable.Len == 0) ||
              Found == MIB_TBL_POS_END )
            {
            // Index not found in table, get next from field
//            Field ++;

            // Make sure not past last field
//            if ( Field > ODOM_LAST_FIELD )
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
            if ( Entry > MIB_DomOtherDomainTable.Len-1 )
               {
               Entry = 0;
               Field ++;
               if ( Field > ODOM_LAST_FIELD )
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
         SnmpUtilOidAppend( &VarBind->name, &MIB_DomOtherDomainPrefix );
         SnmpUtilOidAppend( &VarBind->name, &FieldOid );
         SnmpUtilOidAppend( &VarBind->name, &MIB_DomOtherDomainTable.Table[Entry].Oid );
         }

         ErrStat = MIB_odoms_copyfromtable( Entry, Field, VarBind );

         break;

      case MIB_ACTION_SET:
         // Make sure OID is long enough
	 if ( ODOM_FIELD_SUBID + 1 > VarBind->name.idLength )
            {
	    ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
	    goto Exit;
	    }

	 // Get field number
	 Field = VarBind->name.ids[ODOM_FIELD_SUBID];

	 // If the field being set is not the NAME field, error
	 if ( Field != ODOM_NAME_FIELD )
	    {
	    ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
            goto Exit;
	    }

         // Check for proper type before setting
         if ( ASN_RFC1213_DISPSTRING != VarBind->value.asnType )
	    {
	    ErrStat = SNMP_ERRORSTATUS_BADVALUE;
	    goto Exit;
	    }

	 // Call LM set routine
	 ErrStat = MIB_odoms_lmset( &VarBind->name, Field, &VarBind->value );

         break;

      default:
         ErrStat = SNMP_ERRORSTATUS_GENERR;
      }

Exit:
   return ErrStat;
} // MIB_odoms_func



//
// MIB_odoms_get
//    Retrieve Other Domain Table information.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_odoms_get(
        IN OUT RFC1157VarBind *VarBind
	)

{
UINT   Entry;
int    Found;
UINT   ErrStat;

   if (VarBind->name.ids[ODOM_FIELD_SUBID] < ODOM_FIRST_FIELD ||
       VarBind->name.ids[ODOM_FIELD_SUBID] > ODOM_LAST_FIELD)
       {
       ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
       goto Exit;
       }
   
   // Fill the Other Domain Table with the info from server
   if ( SNMPAPI_ERROR == MIB_odoms_lmget() )
      {
      ErrStat = SNMP_ERRORSTATUS_GENERR;
      goto Exit;
      }

   Found = MIB_odoms_match( &VarBind->name, &Entry );

   // Look for a complete OID match
   if ( Found != MIB_TBL_POS_FOUND )
      {
      ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
      goto Exit;
      }

   // Copy data from table
   ErrStat = MIB_odoms_copyfromtable( Entry,
                                      VarBind->name.ids[ODOM_FIELD_SUBID],
                                      VarBind );

Exit:
   return ErrStat;
} // MIB_odoms_get



//
// MIB_odoms_match
//    Match the target OID with a location in the Other Domain Table
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None
//
int MIB_odoms_match(
       IN AsnObjectIdentifier *Oid,
       OUT UINT *Pos
       )

{
AsnObjectIdentifier TempOid;
int                 nResult;


   // Remove prefix including field reference
   TempOid.idLength = Oid->idLength - MIB_OidPrefix.idLength -
                      MIB_DomOtherDomainPrefix.idLength - 1;
   TempOid.ids = &Oid->ids[MIB_OidPrefix.idLength+MIB_DomOtherDomainPrefix.idLength+1];

   *Pos = 0;
   while ( *Pos < MIB_DomOtherDomainTable.Len )
      {
      nResult = SnmpUtilOidCmp( &TempOid, &MIB_DomOtherDomainTable.Table[*Pos].Oid );
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
// MIB_odoms_copyfromtable
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
UINT MIB_odoms_copyfromtable(
        IN UINT Entry,
        IN UINT Field,
        OUT RFC1157VarBind *VarBind
        )

{
UINT ErrStat;


   // Get the requested field and save in var bind
   switch( Field )
      {
      case ODOM_NAME_FIELD:
         // Alloc space for string
         VarBind->value.asnValue.string.stream = SnmpUtilMemAlloc( sizeof(char)
                       * MIB_DomOtherDomainTable.Table[Entry].domOtherName.length );
         if ( VarBind->value.asnValue.string.stream == NULL )
            {
            ErrStat = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         // Copy string into return position
         memcpy( VarBind->value.asnValue.string.stream,
                       MIB_DomOtherDomainTable.Table[Entry].domOtherName.stream,
                       MIB_DomOtherDomainTable.Table[Entry].domOtherName.length );

         // Set string length
         VarBind->value.asnValue.string.length =
                          MIB_DomOtherDomainTable.Table[Entry].domOtherName.length;
         VarBind->value.asnValue.string.dynamic = TRUE;

         // Set type of var bind
         VarBind->value.asnType = ASN_RFC1213_DISPSTRING;
         break;

      default:
         SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: Internal Error Other Domain Table\n" ));
         ErrStat = SNMP_ERRORSTATUS_GENERR;

         goto Exit;
      }

   ErrStat = SNMP_ERRORSTATUS_NOERROR;

Exit:
   return ErrStat;
} // MIB_odoms_copyfromtable

//-------------------------------- END --------------------------------------
