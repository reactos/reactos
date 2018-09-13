/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    resolve.c

Abstract:

    High level routines to process the variable binding list.

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
#include <snmputil.h>

#include "mib.h"
#include "mibfuncs.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

AsnInteger ResolveVarBind(
              IN RFC1157VarBind *VarBind, // Variable Binding to resolve
              IN UINT PduAction           // Action specified in PDU
              );

SNMPAPI SnmpExtensionQuery(
           IN BYTE ReqType,               // 1157 Request type
           IN OUT RFC1157VarBindList *VarBinds, // Var Binds to resolve
           OUT AsnInteger *ErrorStatus,         // Error status returned
           OUT AsnInteger *ErrorIndex           // Var Bind containing error
           );

//--------------------------- PRIVATE PROCEDURES ----------------------------

//
// ResolveVarBind
//    Resolve a variable binding.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
AsnInteger ResolveVarBind(
              IN RFC1157VarBind *VarBind, // Variable Binding to resolve
              IN UINT PduAction           // Action specified in PDU
              )

{
MIB_ENTRY            *MibPtr;
AsnObjectIdentifier  TempOid;
AsnInteger           nResult;


   // Lookup OID in MIB
   MibPtr = MIB_get_entry( &VarBind->name );

   // Check to see if OID is between LM variables
   if ( MibPtr == NULL && PduAction == MIB_ACTION_GETNEXT )
      {
      UINT I;


      //
      // OPENISSUE - Should change to binary search
      //
      // Search through MIB to see if OID is within the LM MIB's scope
      I = 0;
      while ( MibPtr == NULL && I < MIB_num_variables )
         {
         // Construct OID with complete prefix for comparison purposes
         SnmpUtilOidCpy( &TempOid, &MIB_OidPrefix );
         SnmpUtilOidAppend( &TempOid, &Mib[I].Oid );

         // Check for OID in MIB
         if ( 0 > SnmpUtilOidCmp(&VarBind->name, &TempOid) )
            {
            MibPtr = &Mib[I];
            PduAction = MIB_ACTION_GETFIRST;
            }

         // Free OID memory before copying another
         SnmpUtilOidFree( &TempOid );

         I++;
         } // while
      } // if

   // If OID not within scope of LM MIB, then no such name
   if ( MibPtr == NULL )
      {
      nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
      goto Exit;
      }

   // Call MIB function to apply requested operation
   if ( MibPtr->MibFunc == NULL )
      {
      // If not GET-NEXT, then error
      if ( PduAction != MIB_ACTION_GETNEXT && PduAction != MIB_ACTION_GETFIRST )
         {
         nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
         goto Exit;
         }

      // Since this is AGGREGATE, use GET-FIRST on next variable, then return
      nResult = (*MibPtr->MibNext->MibFunc)( MIB_ACTION_GETFIRST,
                                             MibPtr->MibNext, VarBind );
      }
   else
      {
      // Make complete OID of MIB name
      SnmpUtilOidCpy( &TempOid, &MIB_OidPrefix );
      SnmpUtilOidAppend( &TempOid, &MibPtr->Oid );

      if ( MibPtr->Type == MIB_TABLE && !SnmpUtilOidCmp(&TempOid, &VarBind->name) )
         {
         if ( PduAction == MIB_ACTION_GETNEXT )
            {
            // Supports GET-NEXT on a MIB table's root node
            PduAction = MIB_ACTION_GETFIRST;
            }
         else
            {
            nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
            SnmpUtilOidFree( &TempOid );
            goto Exit;
            }
         }

      nResult = (*MibPtr->MibFunc)( PduAction, MibPtr, VarBind );

      // Free temp memory
      SnmpUtilOidFree( &TempOid );
      }

Exit:
   return nResult;
} // ResolveVarBind

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// SnmpExtensionQuery
//    Loop through var bind list resolving each var bind name to an entry
//    in the LAN Manager MIB.
//
// Notes:
//    Table sets are handled on a case by case basis, because in some cases
//    more than one entry in the Var Bind list will be needed to perform a
//    single SET on the LM MIB.  This is due to the LM API calls.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    None.
//
SNMPAPI SnmpExtensionQuery(
           IN BYTE ReqType,               // 1157 Request type
           IN OUT RFC1157VarBindList *VarBinds, // Var Binds to resolve
           OUT AsnInteger *ErrorStatus,         // Error status returned
           OUT AsnInteger *ErrorIndex           // Var Bind containing error
           )

{
UINT    I;
SNMPAPI nResult;


//
//
// OPENISSUE - Support is not available for TABLE SETS.
//
//
   nResult = SNMPAPI_NOERROR;

   *ErrorIndex = 0;
   // Loop through Var Bind list resolving var binds
   for ( I=0;I < VarBinds->len;I++ )
      {
      *ErrorStatus = ResolveVarBind( &VarBinds->list[I], ReqType );

      // Check for GET-NEXT past end of MIB
      if ( *ErrorStatus == SNMP_ERRORSTATUS_NOSUCHNAME &&
           ReqType == MIB_ACTION_GETNEXT )
         {
         *ErrorStatus = SNMP_ERRORSTATUS_NOERROR;

         // Set Var Bind pointing to next enterprise past LM MIB
         SnmpUtilOidFree( &VarBinds->list[I].name );
         SnmpUtilOidCpy( &VarBinds->list[I].name, &MIB_OidPrefix );
         VarBinds->list[I].name.ids[MIB_PREFIX_LEN-1] ++;
         }

      if ( *ErrorStatus != SNMP_ERRORSTATUS_NOERROR )
         {
         *ErrorIndex = I+1;
         goto Exit;
         }
      }

Exit:
   return nResult;
} // SnmpExtensionQuery

//-------------------------------- END --------------------------------------
