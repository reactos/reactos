/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    testmib.c

Abstract:

    Sample SNMP Extension Agent for Windows NT.

    These files (testdll.c, testmib.c, and testmib.h) provide an example of 
    how to structure an Extension Agent DLL which works in conjunction with 
    the SNMP Extendible Agent for Windows NT.

    Extensive comments have been included to describe its structure and
    operation.  See also "Microsoft Windows NT SNMP Programmer's Reference".

--*/


// This Extension Agent implements the Internet toaster MIB.  It's 
// definition follows here:
//
//
//         TOASTER-MIB DEFINITIONS ::= BEGIN
//
//         IMPORTS
//                 enterprises
//                         FROM RFC1155-SMI
//                 OBJECT-TYPE
//                         FROM RFC-1212
//                 DisplayString
//                         FROM RFC-1213;
//
//         epilogue        OBJECT IDENTIFIER ::= { enterprises 12 }
//         toaster         OBJECT IDENTIFIER ::= { epilogue 2 }
//
//         -- toaster MIB
//
//         toasterManufacturer OBJECT-TYPE
//             SYNTAX  DisplayString
//             ACCESS  read-only
//             STATUS  mandatory
//             DESCRIPTION
//                     "The name of the toaster's manufacturer. For instance,
//                      Sunbeam."
//             ::= { toaster 1 }
//
//         toasterModelNumber OBJECT-TYPE
//             SYNTAX  DisplayString
//             ACCESS  read-only
//             STATUS  mandatory
//             DESCRIPTION
//                     "The name of the toaster's model. For instance,
//                      Radiant Automatic."
//             ::= { toaster 2 }
//
//         toasterControl OBJECT-TYPE
//             SYNTAX  INTEGER  {
//                         up(1),
//                         down(2)
//                     }
//             ACCESS  read-write
//             STATUS  mandatory
//             DESCRIPTION
//                     "This variable controls the current state of the 
//                      toaster. To begin toasting, set it to down(2). To 
//                      abort toasting (perhaps in the event of an 
//                      emergency), set it to up(2)."
//             ::= { toaster 3 }
//
//         toasterDoneness OBJECT-TYPE
//             SYNTAX  INTEGER (1..10)
//             ACCESS  read-write
//             STATUS  mandatory
//             DESCRIPTION
//                     "This variable controls how well done ensuing toast 
//                      should be on a scale of 1 to 10. Toast made at 10 
//                      is generally considered unfit for human consumption; 
//                      toast made at 1 is lightly warmed."
//             ::= { toaster 4 }
//
//         toasterToastType OBJECT-TYPE
//             SYNTAX  INTEGER  {
//                         white-bread(1),
//                         wheat-bread(2),
//                         wonder-bread(3),
//                         frozen-waffle(4),
//                         frozen-bagel(5),
//                         hash-brown(6),
//                         other(7)
//                     }
//             ACCESS  read-write
//             STATUS  mandatory
//             DESCRIPTION
//                     "This variable informs the toaster of the type of 
//                      material being toasted. The toaster uses this 
//                      information combined with toasterToastDoneness to 
//                      compute how long the material must be toasted for 
//                      to achieve the desired doneness."
//             ::= { toaster 5 }
//
//         END


// Necessary includes.

#include <windows.h>

#include <snmp.h>


// Contains definitions for the table structure describing the MIB.  This
// is used in conjunction with testmib.c where the MIB requests are resolved.

#include "testmib.h"


// If an addition or deletion to the MIB is necessary, there are several
// places in the code that must be checked and possibly changed.
//
// The last field in each MIB entry is used to point to the NEXT
// leaf variable.  If an addition or deletetion is made, these pointers
// may need to be updated to reflect the modification.


// The prefix to all of these MIB variables is 1.3.6.1.4.1.12

UINT OID_Prefix[] = { 1, 3, 6, 1, 4, 1, 12 };
AsnObjectIdentifier MIB_OidPrefix = { OID_SIZEOF(OID_Prefix), OID_Prefix };



//                         //
// OID definitions for MIB //
//                         //


// Definition of the toaster group

UINT MIB_toaster[]  = { 2 };


// Definition of leaf variables under the toaster group
// All leaf variables have a zero appended to their OID to indicate
// that it is the only instance of this variable and it exists.

UINT MIB_toasterManufacturer[]     = { 2, 1, 0 };
UINT MIB_toasterModelNumber[]      = { 2, 2, 0 };
UINT MIB_toasterControl[]          = { 2, 3, 0 };
UINT MIB_toasterDoneness[]         = { 2, 4, 0 };
UINT MIB_toasterToastType[]        = { 2, 5, 0 };



//                             //
// Storage definitions for MIB //
//                             //

char       MIB_toasterManStor[]     = "Microsoft Corporation";
char       MIB_toasterModelStor[]   = 
               "Example SNMP Extension Agent for Windows/NT (TOASTER-MIB).";
AsnInteger MIB_toasterControlStor   = 1;
AsnInteger MIB_toasterDonenessStor  = 2;
AsnInteger MIB_toasterToastTypeStor = 3;



// MIB definiton

MIB_ENTRY Mib[] = {
      { { OID_SIZEOF(MIB_toasterManufacturer), MIB_toasterManufacturer },
        &MIB_toasterManStor, ASN_RFC1213_DISPSTRING,
        MIB_ACCESS_READ, MIB_leaf_func, &Mib[1] },

      { { OID_SIZEOF(MIB_toasterModelNumber), MIB_toasterModelNumber },
        &MIB_toasterModelStor, ASN_RFC1213_DISPSTRING,
        MIB_ACCESS_READ, MIB_leaf_func, &Mib[2] },

      { { OID_SIZEOF(MIB_toasterControl), MIB_toasterControl },
        &MIB_toasterControlStor, ASN_INTEGER,
        MIB_ACCESS_READWRITE, MIB_control_func, &Mib[3] },

      { { OID_SIZEOF(MIB_toasterDoneness), MIB_toasterDoneness },
        &MIB_toasterDonenessStor, ASN_INTEGER,
        MIB_ACCESS_READWRITE, MIB_doneness_func, &Mib[4] },

      { { OID_SIZEOF(MIB_toasterToastType), MIB_toasterToastType },
        &MIB_toasterToastTypeStor, ASN_INTEGER,
        MIB_ACCESS_READWRITE, MIB_toasttype_func, NULL }
      };

UINT MIB_num_variables = sizeof Mib / sizeof( MIB_ENTRY );



//
// ResolveVarBind
//    Resolves a single variable binding.  Modifies the variable on a GET
//    or a GET-NEXT.
//
// Notes:
//
// Return Codes:
//    Standard PDU error codes.
//
// Error Codes:
//    None.
//
UINT ResolveVarBind(
        IN OUT RFC1157VarBind *VarBind, // Variable Binding to resolve
	IN UINT PduAction               // Action specified in PDU
	)

{
MIB_ENTRY            *MibPtr;
AsnObjectIdentifier  TempOid;
int                  CompResult;
UINT                 I;
UINT                 nResult;


   // Search for var bind name in the MIB
   I      = 0;
   MibPtr = NULL;
   while ( MibPtr == NULL && I < MIB_num_variables )
      {
      // Construct OID with complete prefix for comparison purposes
      SnmpUtilOidCpy( &TempOid, &MIB_OidPrefix );
      SnmpUtilOidAppend( &TempOid, &Mib[I].Oid );

      // Check for OID in MIB - On a GET-NEXT the OID does not have to exactly
      // match a variable in the MIB, it must only fall under the MIB root.
      CompResult = SnmpUtilOidCmp( &VarBind->name, &TempOid );
      if ( 0 > CompResult )
	 {
	 // Since there is not an exact match, the only valid action is GET-NEXT
	 if ( MIB_ACTION_GETNEXT != PduAction )
	    {
	    nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
	    goto Exit;
	    }

	 // Since the match was not exact, but var bind name is within MIB,
	 // we are at the NEXT MIB variable down from the one specified.
	 PduAction = MIB_ACTION_GET;
	 MibPtr = &Mib[I];

         // Replace var bind name with new name
         SnmpUtilOidFree( &VarBind->name );
         SnmpUtilOidCpy( &VarBind->name, &MIB_OidPrefix );
         SnmpUtilOidAppend( &VarBind->name, &MibPtr->Oid );
	 }
      else
         {
	 // An exact match was found.
         if ( 0 == CompResult )
            {
	    MibPtr = &Mib[I];
	    }
	 }

      // Free OID memory before checking another variable
      SnmpUtilOidFree( &TempOid );

      I++;
      } // while

   // If OID not within scope of MIB, then no such name
   if ( MibPtr == NULL )
      {
      nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
      goto Exit;
      }

   // Call function to process request.  Each MIB entry has a function pointer
   // that knows how to process its MIB variable.
   nResult = (*MibPtr->MibFunc)( PduAction, MibPtr, VarBind );

   // Free temp memory
   SnmpUtilOidFree( &TempOid );

Exit:
   return nResult;
} // ResolveVarBind



//
// MIB_leaf_func
//    Performs generic actions on LEAF variables in the MIB.
//
// Notes:
//
// Return Codes:
//    Standard PDU error codes.
//
// Error Codes:
//    None.
//
UINT MIB_leaf_func(
        IN UINT Action,
	IN MIB_ENTRY *MibPtr,
	IN RFC1157VarBind *VarBind
	)

{
UINT   ErrStat;

   switch ( Action )
      {
      case MIB_ACTION_GETNEXT:
	 // If there is no GET-NEXT pointer, this is the end of this MIB
	 if ( MibPtr->MibNext == NULL )
	    {
	    ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
	    goto Exit;
	    }

         // Setup var bind name of NEXT MIB variable
         SnmpUtilOidFree( &VarBind->name );
         SnmpUtilOidCpy( &VarBind->name, &MIB_OidPrefix );
         SnmpUtilOidAppend( &VarBind->name, &MibPtr->MibNext->Oid );

         // Call function to process request.  Each MIB entry has a function
	 // pointer that knows how to process its MIB variable.
         ErrStat = (*MibPtr->MibNext->MibFunc)( MIB_ACTION_GET,
	                                        MibPtr->MibNext, VarBind );
         break;

      case MIB_ACTION_GET:
         // Make sure that this variable's ACCESS is GET'able
	 if ( MibPtr->Access != MIB_ACCESS_READ &&
	      MibPtr->Access != MIB_ACCESS_READWRITE )
	    {
	    ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
	    goto Exit;
	    }

	 // Setup varbind's return value
	 VarBind->value.asnType = MibPtr->Type;
	 switch ( VarBind->value.asnType )
	    {
            case ASN_RFC1155_COUNTER:
            case ASN_RFC1155_GAUGE:
            case ASN_INTEGER:
               VarBind->value.asnValue.number = *(AsnInteger *)(MibPtr->Storage);
	       break;

            case ASN_OCTETSTRING: // This entails ASN_RFC1213_DISPSTRING also
	       VarBind->value.asnValue.string.length =
                                 strlen( (LPSTR)MibPtr->Storage );

	       if ( NULL == 
                    (VarBind->value.asnValue.string.stream =
                    SnmpUtilMemAlloc(VarBind->value.asnValue.string.length *
                           sizeof(char))) )
	          {
	          ErrStat = SNMP_ERRORSTATUS_GENERR;
	          goto Exit;
	          }

	       memcpy( VarBind->value.asnValue.string.stream,
	               (LPSTR)MibPtr->Storage,
	               VarBind->value.asnValue.string.length );
	       VarBind->value.asnValue.string.dynamic = TRUE;

	       break;

	    default:
	       ErrStat = SNMP_ERRORSTATUS_GENERR;
	       goto Exit;
	    }

	 break;

      case MIB_ACTION_SET:
         // Make sure that this variable's ACCESS is SET'able
	 if ( MibPtr->Access != MIB_ACCESS_READWRITE &&
	      MibPtr->Access != MIB_ACCESS_WRITE )
	    {
	    ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
            goto Exit;
	    }

         // Check for proper type before setting
         if ( MibPtr->Type != VarBind->value.asnType )
	    {
	    ErrStat = SNMP_ERRORSTATUS_BADVALUE;
	    goto Exit;
	    }

	 // Save value in MIB
	 switch ( VarBind->value.asnType )
	    {
            case ASN_RFC1155_COUNTER:
            case ASN_RFC1155_GAUGE:
            case ASN_INTEGER:
               *(AsnInteger *)(MibPtr->Storage) = VarBind->value.asnValue.number;
	       break;

            case ASN_OCTETSTRING: // This entails ASN_RFC1213_DISPSTRING also
               // The storage must be adequate to contain the new string
               // including a NULL terminator.
               memcpy( (LPSTR)MibPtr->Storage,
                       VarBind->value.asnValue.string.stream,
                       VarBind->value.asnValue.string.length );

	       ((LPSTR)MibPtr->Storage)[VarBind->value.asnValue.string.length] =
                                                                          '\0';
	       break;

	    default:
	       ErrStat = SNMP_ERRORSTATUS_GENERR;
	       goto Exit;
	    }

         break;

      default:
	 ErrStat = SNMP_ERRORSTATUS_GENERR;
	 goto Exit;
      } // switch

   // Signal no error occurred
   ErrStat = SNMP_ERRORSTATUS_NOERROR;

Exit:
   return ErrStat;
} // MIB_leaf_func



//
// MIB_control_func
//    Performs specific actions on the toasterControl MIB variable
//
// Notes:
//
// Return Codes:
//    Standard PDU error codes.
//
// Error Codes:
//    None.
//
UINT MIB_control_func(
        IN UINT Action,
	IN MIB_ENTRY *MibPtr,
	IN RFC1157VarBind *VarBind
	)

{
UINT   ErrStat;

   switch ( Action )
      {
      case MIB_ACTION_SET:
         // Make sure that this variable's ACCESS is SET'able
	 if ( MibPtr->Access != MIB_ACCESS_READWRITE &&
	      MibPtr->Access != MIB_ACCESS_WRITE )
	    {
	    ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
            goto Exit;
	    }

         // Check for proper type before setting
         if ( MibPtr->Type != VarBind->value.asnType )
	    {
	    ErrStat = SNMP_ERRORSTATUS_BADVALUE;
	    goto Exit;
	    }

         // Make sure the value is valid
         if ( MIB_TOASTER_UP > VarBind->value.asnValue.number ||
              MIB_TOASTER_DOWN < VarBind->value.asnValue.number )
            {
	    ErrStat = SNMP_ERRORSTATUS_BADVALUE;
	    goto Exit;
            }

         // Let fall through purposefully for further processing by
         // generic leaf function.

      case MIB_ACTION_GETNEXT:
      case MIB_ACTION_GET:
	 // Call the more generic function to perform the action
         ErrStat = MIB_leaf_func( Action, MibPtr, VarBind );
         break;

      default:
	 ErrStat = SNMP_ERRORSTATUS_GENERR;
	 goto Exit;
      } // switch

Exit:
   return ErrStat;
} // MIB_control_func



//
// MIB_doneness_func
//    Performs specific actions on the toasterDoneness MIB variable
//
// Notes:
//
// Return Codes:
//    Standard PDU error codes.
//
// Error Codes:
//    None.
//
UINT MIB_doneness_func(
        IN UINT Action,
	IN MIB_ENTRY *MibPtr,
	IN RFC1157VarBind *VarBind
	)

{
UINT   ErrStat;

   switch ( Action )
      {
      case MIB_ACTION_SET:
         // Make sure that this variable's ACCESS is SET'able
	 if ( MibPtr->Access != MIB_ACCESS_READWRITE &&
	      MibPtr->Access != MIB_ACCESS_WRITE )
	    {
	    ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
            goto Exit;
	    }

         // Check for proper type before setting
         if ( MibPtr->Type != VarBind->value.asnType )
	    {
	    ErrStat = SNMP_ERRORSTATUS_BADVALUE;
	    goto Exit;
	    }

         // Make sure the value is valid
         if ( MIB_TOASTER_LIGHTLYWARM > VarBind->value.asnValue.number ||
              MIB_TOASTER_BURNT < VarBind->value.asnValue.number )
            {
	    ErrStat = SNMP_ERRORSTATUS_BADVALUE;
	    goto Exit;
            }

         // Let fall through purposefully for further processing by
         // generic leaf function.

      case MIB_ACTION_GETNEXT:
      case MIB_ACTION_GET:
	 // Call the more generic function to perform the action
         ErrStat = MIB_leaf_func( Action, MibPtr, VarBind );
         break;

      default:
	 ErrStat = SNMP_ERRORSTATUS_GENERR;
	 goto Exit;
      } // switch

Exit:
   return ErrStat;
} // MIB_doneness_func



//
// MIB_toasttype_func
//    Performs specific actions on the toasterToastType MIB variable
//
// Notes:
//
// Return Codes:
//    Standard PDU error codes.
//
// Error Codes:
//    None.
//
UINT MIB_toasttype_func(
        IN UINT Action,
	IN MIB_ENTRY *MibPtr,
	IN RFC1157VarBind *VarBind
	)

{
UINT   ErrStat;

   switch ( Action )
      {
      case MIB_ACTION_SET:
         // Make sure that this variable's ACCESS is SET'able
	 if ( MibPtr->Access != MIB_ACCESS_READWRITE &&
	      MibPtr->Access != MIB_ACCESS_WRITE )
	    {
	    ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
            goto Exit;
	    }

         // Check for proper type before setting
         if ( MibPtr->Type != VarBind->value.asnType )
	    {
	    ErrStat = SNMP_ERRORSTATUS_BADVALUE;
	    goto Exit;
	    }

         // Make sure the value is valid
         if ( MIB_TOASTER_WHITEBREAD > VarBind->value.asnValue.number ||
              MIB_TOASTER_OTHERBREAD < VarBind->value.asnValue.number )
            {
	    ErrStat = SNMP_ERRORSTATUS_BADVALUE;
	    goto Exit;
            }

         // Let fall through purposefully for further processing by
         // generic leaf function.

      case MIB_ACTION_GETNEXT:
      case MIB_ACTION_GET:
	 // Call the more generic function to perform the action
         ErrStat = MIB_leaf_func( Action, MibPtr, VarBind );
         break;

      default:
	 ErrStat = SNMP_ERRORSTATUS_GENERR;
	 goto Exit;
      } // switch

Exit:
   return ErrStat;
} // MIB_toasttype_func

