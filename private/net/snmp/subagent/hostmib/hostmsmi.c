
/*
 * mibtsmi.ntc v0.10
 *  hostmsmi.c
 *  Generated in conjunction with Management Factory scripts:
 *      script version: SNMPv1, 0.16, Apr 25, 1996
 *      project:        D:\TEMP\EXAMPLE\HOSTMIB
 *
 ****************************************************************************
 *                                                                          *
 *      (C) Copyright 1995 DIGITAL EQUIPMENT CORPORATION                    *
 *                                                                          *
 *      This  software  is  an  unpublished work protected under the        *
 *      the copyright laws of the  United  States  of  America,  all        *
 *      rights reserved.                                                    *
 *                                                                          *
 *      In the event this software is licensed for use by the United        *
 *      States Government, all use, duplication or disclosure by the        *
 *      United States Government is subject to restrictions  as  set        *
 *      forth in either subparagraph  (c)(1)(ii)  of the  Rights  in        *
 *      Technical  Data  And  Computer  Software  Clause  at   DFARS        *
 *      252.227-7013, or the Commercial Computer Software Restricted        *
 *      Rights Clause at FAR 52.221-19, whichever is applicable.            *
 *                                                                          *
 ****************************************************************************
 *
 *  Facility:
 *
 *    SNMP Extension Agent
 *
 *  Abstract:
 *  
 *    This module contains the SMI envelopes around the callout to the
 *    developer's get and set routines.  Note that the developer can modify
 *    these routines to valid that the types actually conform to contraints
 *    for a given type.
 *
 *  Functions:
 *
 *    SMIGetxxx() and SMISetxxx() for each user defined type.
 *
 *  Author:
 *
 *	David Burns @ Webenable Inc
 *
 *  Date:
 *
 *	Thu Nov 07 16:38:30 1996
 *
 *  Revision History:
 *      generated with v0.10 stub
 *
 *      May 15, 1997 - To Microsoft: 4 changes of "malloc" to "SNMP_malloc"
 */

#include <snmp.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>

#include "smint.h"
#include "hostmsmi.h"
#include "mib.h"
#include "mib_xtrn.h"

/*
 *  SMIGetBoolean
 *    Boolean ::= INTEGER a truth value
 *    
 *    Encompasses the callouts to variables for the data type Boolean
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      cindex                    index to the class of the request
 *      vindex                    index to the variable of the request
 *      instance                  address of the instance specification
 *                                in the form of ordered native datatypes
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 *    SNMP_ERRORSTATUS_NOERROR    Successful get
 *    SNMP_ERRORSTATUS_GENERR     Catch-all failure code
 * mibtsmii.c v 0.6
 */

UINT
SMIGetBoolean(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for get
      IN unsigned long int cindex ,
      IN unsigned long int vindex ,
      IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    Boolean outvalue ;
    Access_Credential access ;  // dummy holder for future use

    result = ( *class_info[ cindex ].variable[ vindex ].VarGet )( &outvalue ,
                                                                  &access ,
                                                                  instance ) ;
    if ( result == SNMP_ERRORSTATUS_NOERROR )
    {
        VarBind->value.asnType = ASN_INTEGER ;
        VarBind->value.asnValue.number =
                               (AsnInteger)outvalue ;
    }
    return result ;

} /* end of SMIGetBoolean() */

/*
 *  SMISetBoolean
 *    Boolean ::= INTEGER a truth value
 *    
 *    Encompasses the callouts to variables for the data type Boolean
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      cindex                    index to the class of the request
 *      vindex                    index to the variable of the request
 *      instance                  address of the instance specification
 *                                in the form of ordered native datatypes
 *  Return Codes:
 *
 *    Standard PDU error codes.
 */

UINT
SMISetBoolean(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for set
      IN unsigned long int cindex ,
      IN unsigned long int vindex ,
      IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    Boolean *invalue ;
    Boolean outvalue ;
    Access_Credential access ;   // dummy holder for future use

    invalue = (Boolean *)
              ( &VarBind->value.asnValue.number ) ;
    result = ( *class_info[ cindex ].variable[ vindex ].VarSet )
             ( invalue, &outvalue, &access, instance ) ;
    return result;

} /* end of SMISetBoolean() */

/*
 *  SMIBuildBoolean
 *    Boolean ::= INTEGER a truth value
 *    
 *    Places the variable of datatype Boolean into a Variable Binding.
 *
 *  Arguments:
 *
 *    VarBind            pointer to the variable value pair
 *    invalue            address of the data
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 */

UINT
SMIBuildBoolean(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for building
      IN char *invalue )
{
    Integer *svalue = (Integer *)invalue;
    VarBind->value.asnType = ASN_INTEGER ;
    VarBind->value.asnValue.number = *svalue ;
    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of SMIBuildBoolean() */


/*
 *  SMIGetKBytes
 *    KBytes ::= INTEGER (0..2147483647)  memory size, expressed in units of 
 *    1024 bytes
 *    
 *    Encompasses the callouts to variables for the data type KBytes
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      cindex                    index to the class of the request
 *      vindex                    index to the variable of the request
 *      instance                  address of the instance specification
 *                                in the form of ordered native datatypes
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 *    SNMP_ERRORSTATUS_NOERROR    Successful get
 *    SNMP_ERRORSTATUS_GENERR     Catch-all failure code
 * mibtsmii.c v 0.6
 */

UINT
SMIGetKBytes(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for get
      IN unsigned long int cindex ,
      IN unsigned long int vindex ,
      IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    KBytes outvalue ;
    Access_Credential access ;  // dummy holder for future use

    result = ( *class_info[ cindex ].variable[ vindex ].VarGet )( &outvalue ,
                                                                  &access ,
                                                                  instance ) ;
    if ( result == SNMP_ERRORSTATUS_NOERROR )
    {
        VarBind->value.asnType = ASN_INTEGER ;
        VarBind->value.asnValue.number =
                               (AsnInteger)outvalue ;
    }
    return result ;

} /* end of SMIGetKBytes() */

/*
 *  SMISetKBytes
 *    KBytes ::= INTEGER (0..2147483647)  memory size, expressed in units of 
 *    1024 bytes
 *    
 *    Encompasses the callouts to variables for the data type KBytes
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      cindex                    index to the class of the request
 *      vindex                    index to the variable of the request
 *      instance                  address of the instance specification
 *                                in the form of ordered native datatypes
 *  Return Codes:
 *
 *    Standard PDU error codes.
 */

UINT
SMISetKBytes(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for set
      IN unsigned long int cindex ,
      IN unsigned long int vindex ,
      IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    KBytes *invalue ;
    KBytes outvalue ;
    Access_Credential access ;   // dummy holder for future use

    invalue = (KBytes *)
              ( &VarBind->value.asnValue.number ) ;
    result = ( *class_info[ cindex ].variable[ vindex ].VarSet )
             ( invalue, &outvalue, &access, instance ) ;
    return result;

} /* end of SMISetKBytes() */

/*
 *  SMIBuildKBytes
 *    KBytes ::= INTEGER (0..2147483647)  memory size, expressed in units of 
 *    1024 bytes
 *    
 *    Places the variable of datatype KBytes into a Variable Binding.
 *
 *  Arguments:
 *
 *    VarBind            pointer to the variable value pair
 *    invalue            address of the data
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 */

UINT
SMIBuildKBytes(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for building
      IN char *invalue )
{
    Integer *svalue = (Integer *)invalue;
    VarBind->value.asnType = ASN_INTEGER ;
    VarBind->value.asnValue.number = *svalue ;
    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of SMIBuildKBytes() */


/*
 *  SMIGetINThrDeviceStatus
 *    INThrDeviceStatus ::= INTEGER 
 *    
 *    Encompasses the callouts to variables for the data type INThrDeviceStatus
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      cindex                    index to the class of the request
 *      vindex                    index to the variable of the request
 *      instance                  address of the instance specification
 *                                in the form of ordered native datatypes
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 *    SNMP_ERRORSTATUS_NOERROR    Successful get
 *    SNMP_ERRORSTATUS_GENERR     Catch-all failure code
 * mibtsmii.c v 0.6
 */

UINT
SMIGetINThrDeviceStatus(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for get
      IN unsigned long int cindex ,
      IN unsigned long int vindex ,
      IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    INThrDeviceStatus outvalue ;
    Access_Credential access ;  // dummy holder for future use

    result = ( *class_info[ cindex ].variable[ vindex ].VarGet )( &outvalue ,
                                                                  &access ,
                                                                  instance ) ;
    if ( result == SNMP_ERRORSTATUS_NOERROR )
    {
        VarBind->value.asnType = ASN_INTEGER ;
        VarBind->value.asnValue.number =
                               (AsnInteger)outvalue ;
    }
    return result ;

} /* end of SMIGetINThrDeviceStatus() */

/*
 *  SMISetINThrDeviceStatus
 *    INThrDeviceStatus ::= INTEGER 
 *    
 *    Encompasses the callouts to variables for the data type INThrDeviceStatus
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      cindex                    index to the class of the request
 *      vindex                    index to the variable of the request
 *      instance                  address of the instance specification
 *                                in the form of ordered native datatypes
 *  Return Codes:
 *
 *    Standard PDU error codes.
 */

UINT
SMISetINThrDeviceStatus(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for set
      IN unsigned long int cindex ,
      IN unsigned long int vindex ,
      IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    INThrDeviceStatus *invalue ;
    INThrDeviceStatus outvalue ;
    Access_Credential access ;   // dummy holder for future use

    invalue = (INThrDeviceStatus *)
              ( &VarBind->value.asnValue.number ) ;
    result = ( *class_info[ cindex ].variable[ vindex ].VarSet )
             ( invalue, &outvalue, &access, instance ) ;
    return result;

} /* end of SMISetINThrDeviceStatus() */

/*
 *  SMIBuildINThrDeviceStatus
 *    INThrDeviceStatus ::= INTEGER 
 *    
 *    Places the variable of datatype INThrDeviceStatus into a Variable Binding.
 *
 *  Arguments:
 *
 *    VarBind            pointer to the variable value pair
 *    invalue            address of the data
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 */

UINT
SMIBuildINThrDeviceStatus(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for building
      IN char *invalue )
{
    Integer *svalue = (Integer *)invalue;
    VarBind->value.asnType = ASN_INTEGER ;
    VarBind->value.asnValue.number = *svalue ;
    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of SMIBuildINThrDeviceStatus() */


/*
 *  SMIGetINThrPrinterStatus
 *    INThrPrinterStatus ::= INTEGER 
 *    
 *    Encompasses the callouts to variables for the data type INThrPrinterStatus
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      cindex                    index to the class of the request
 *      vindex                    index to the variable of the request
 *      instance                  address of the instance specification
 *                                in the form of ordered native datatypes
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 *    SNMP_ERRORSTATUS_NOERROR    Successful get
 *    SNMP_ERRORSTATUS_GENERR     Catch-all failure code
 * mibtsmii.c v 0.6
 */

UINT
SMIGetINThrPrinterStatus(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for get
      IN unsigned long int cindex ,
      IN unsigned long int vindex ,
      IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    INThrPrinterStatus outvalue ;
    Access_Credential access ;  // dummy holder for future use

    result = ( *class_info[ cindex ].variable[ vindex ].VarGet )( &outvalue ,
                                                                  &access ,
                                                                  instance ) ;
    if ( result == SNMP_ERRORSTATUS_NOERROR )
    {
        VarBind->value.asnType = ASN_INTEGER ;
        VarBind->value.asnValue.number =
                               (AsnInteger)outvalue ;
    }
    return result ;

} /* end of SMIGetINThrPrinterStatus() */

/*
 *  SMISetINThrPrinterStatus
 *    INThrPrinterStatus ::= INTEGER 
 *    
 *    Encompasses the callouts to variables for the data type INThrPrinterStatus
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      cindex                    index to the class of the request
 *      vindex                    index to the variable of the request
 *      instance                  address of the instance specification
 *                                in the form of ordered native datatypes
 *  Return Codes:
 *
 *    Standard PDU error codes.
 */

UINT
SMISetINThrPrinterStatus(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for set
      IN unsigned long int cindex ,
      IN unsigned long int vindex ,
      IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    INThrPrinterStatus *invalue ;
    INThrPrinterStatus outvalue ;
    Access_Credential access ;   // dummy holder for future use

    invalue = (INThrPrinterStatus *)
              ( &VarBind->value.asnValue.number ) ;
    result = ( *class_info[ cindex ].variable[ vindex ].VarSet )
             ( invalue, &outvalue, &access, instance ) ;
    return result;

} /* end of SMISetINThrPrinterStatus() */

/*
 *  SMIBuildINThrPrinterStatus
 *    INThrPrinterStatus ::= INTEGER 
 *    
 *    Places the variable of datatype INThrPrinterStatus into a Variable Binding.
 *
 *  Arguments:
 *
 *    VarBind            pointer to the variable value pair
 *    invalue            address of the data
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 */

UINT
SMIBuildINThrPrinterStatus(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for building
      IN char *invalue )
{
    Integer *svalue = (Integer *)invalue;
    VarBind->value.asnType = ASN_INTEGER ;
    VarBind->value.asnValue.number = *svalue ;
    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of SMIBuildINThrPrinterStatus() */


/*
 *  SMIGetINTAccess
 *    INTAccess ::= INTEGER 
 *    
 *    Encompasses the callouts to variables for the data type INTAccess
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      cindex                    index to the class of the request
 *      vindex                    index to the variable of the request
 *      instance                  address of the instance specification
 *                                in the form of ordered native datatypes
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 *    SNMP_ERRORSTATUS_NOERROR    Successful get
 *    SNMP_ERRORSTATUS_GENERR     Catch-all failure code
 * mibtsmii.c v 0.6
 */

UINT
SMIGetINTAccess(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for get
      IN unsigned long int cindex ,
      IN unsigned long int vindex ,
      IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    INTAccess outvalue ;
    Access_Credential access ;  // dummy holder for future use

    result = ( *class_info[ cindex ].variable[ vindex ].VarGet )( &outvalue ,
                                                                  &access ,
                                                                  instance ) ;
    if ( result == SNMP_ERRORSTATUS_NOERROR )
    {
        VarBind->value.asnType = ASN_INTEGER ;
        VarBind->value.asnValue.number =
                               (AsnInteger)outvalue ;
    }
    return result ;

} /* end of SMIGetINTAccess() */

/*
 *  SMISetINTAccess
 *    INTAccess ::= INTEGER 
 *    
 *    Encompasses the callouts to variables for the data type INTAccess
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      cindex                    index to the class of the request
 *      vindex                    index to the variable of the request
 *      instance                  address of the instance specification
 *                                in the form of ordered native datatypes
 *  Return Codes:
 *
 *    Standard PDU error codes.
 */

UINT
SMISetINTAccess(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for set
      IN unsigned long int cindex ,
      IN unsigned long int vindex ,
      IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    INTAccess *invalue ;
    INTAccess outvalue ;
    Access_Credential access ;   // dummy holder for future use

    invalue = (INTAccess *)
              ( &VarBind->value.asnValue.number ) ;
    result = ( *class_info[ cindex ].variable[ vindex ].VarSet )
             ( invalue, &outvalue, &access, instance ) ;
    return result;

} /* end of SMISetINTAccess() */

/*
 *  SMIBuildINTAccess
 *    INTAccess ::= INTEGER 
 *    
 *    Places the variable of datatype INTAccess into a Variable Binding.
 *
 *  Arguments:
 *
 *    VarBind            pointer to the variable value pair
 *    invalue            address of the data
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 */

UINT
SMIBuildINTAccess(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for building
      IN char *invalue )
{
    Integer *svalue = (Integer *)invalue;
    VarBind->value.asnType = ASN_INTEGER ;
    VarBind->value.asnValue.number = *svalue ;
    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of SMIBuildINTAccess() */


/*
 *  SMIGetINThrDiskStorageMedia
 *    INThrDiskStorageMedia ::= INTEGER 
 *    
 *    Encompasses the callouts to variables for the data type INThrDiskStorageMedia
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      cindex                    index to the class of the request
 *      vindex                    index to the variable of the request
 *      instance                  address of the instance specification
 *                                in the form of ordered native datatypes
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 *    SNMP_ERRORSTATUS_NOERROR    Successful get
 *    SNMP_ERRORSTATUS_GENERR     Catch-all failure code
 * mibtsmii.c v 0.6
 */

UINT
SMIGetINThrDiskStorageMedia(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for get
      IN unsigned long int cindex ,
      IN unsigned long int vindex ,
      IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    INThrDiskStorageMedia outvalue ;
    Access_Credential access ;  // dummy holder for future use

    result = ( *class_info[ cindex ].variable[ vindex ].VarGet )( &outvalue ,
                                                                  &access ,
                                                                  instance ) ;
    if ( result == SNMP_ERRORSTATUS_NOERROR )
    {
        VarBind->value.asnType = ASN_INTEGER ;
        VarBind->value.asnValue.number =
                               (AsnInteger)outvalue ;
    }
    return result ;

} /* end of SMIGetINThrDiskStorageMedia() */

/*
 *  SMISetINThrDiskStorageMedia
 *    INThrDiskStorageMedia ::= INTEGER 
 *    
 *    Encompasses the callouts to variables for the data type INThrDiskStorageMedia
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      cindex                    index to the class of the request
 *      vindex                    index to the variable of the request
 *      instance                  address of the instance specification
 *                                in the form of ordered native datatypes
 *  Return Codes:
 *
 *    Standard PDU error codes.
 */

UINT
SMISetINThrDiskStorageMedia(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for set
      IN unsigned long int cindex ,
      IN unsigned long int vindex ,
      IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    INThrDiskStorageMedia *invalue ;
    INThrDiskStorageMedia outvalue ;
    Access_Credential access ;   // dummy holder for future use

    invalue = (INThrDiskStorageMedia *)
              ( &VarBind->value.asnValue.number ) ;
    result = ( *class_info[ cindex ].variable[ vindex ].VarSet )
             ( invalue, &outvalue, &access, instance ) ;
    return result;

} /* end of SMISetINThrDiskStorageMedia() */

/*
 *  SMIBuildINThrDiskStorageMedia
 *    INThrDiskStorageMedia ::= INTEGER 
 *    
 *    Places the variable of datatype INThrDiskStorageMedia into a Variable Binding.
 *
 *  Arguments:
 *
 *    VarBind            pointer to the variable value pair
 *    invalue            address of the data
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 */

UINT
SMIBuildINThrDiskStorageMedia(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for building
      IN char *invalue )
{
    Integer *svalue = (Integer *)invalue;
    VarBind->value.asnType = ASN_INTEGER ;
    VarBind->value.asnValue.number = *svalue ;
    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of SMIBuildINThrDiskStorageMedia() */


/*
 *  SMIGetINTSWType
 *    INTSWType ::= INTEGER 
 *    
 *    Encompasses the callouts to variables for the data type INTSWType
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      cindex                    index to the class of the request
 *      vindex                    index to the variable of the request
 *      instance                  address of the instance specification
 *                                in the form of ordered native datatypes
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 *    SNMP_ERRORSTATUS_NOERROR    Successful get
 *    SNMP_ERRORSTATUS_GENERR     Catch-all failure code
 * mibtsmii.c v 0.6
 */

UINT
SMIGetINTSWType(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for get
      IN unsigned long int cindex ,
      IN unsigned long int vindex ,
      IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    INTSWType outvalue ;
    Access_Credential access ;  // dummy holder for future use

    result = ( *class_info[ cindex ].variable[ vindex ].VarGet )( &outvalue ,
                                                                  &access ,
                                                                  instance ) ;
    if ( result == SNMP_ERRORSTATUS_NOERROR )
    {
        VarBind->value.asnType = ASN_INTEGER ;
        VarBind->value.asnValue.number =
                               (AsnInteger)outvalue ;
    }
    return result ;

} /* end of SMIGetINTSWType() */

/*
 *  SMISetINTSWType
 *    INTSWType ::= INTEGER 
 *    
 *    Encompasses the callouts to variables for the data type INTSWType
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      cindex                    index to the class of the request
 *      vindex                    index to the variable of the request
 *      instance                  address of the instance specification
 *                                in the form of ordered native datatypes
 *  Return Codes:
 *
 *    Standard PDU error codes.
 */

UINT
SMISetINTSWType(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for set
      IN unsigned long int cindex ,
      IN unsigned long int vindex ,
      IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    INTSWType *invalue ;
    INTSWType outvalue ;
    Access_Credential access ;   // dummy holder for future use

    invalue = (INTSWType *)
              ( &VarBind->value.asnValue.number ) ;
    result = ( *class_info[ cindex ].variable[ vindex ].VarSet )
             ( invalue, &outvalue, &access, instance ) ;
    return result;

} /* end of SMISetINTSWType() */

/*
 *  SMIBuildINTSWType
 *    INTSWType ::= INTEGER 
 *    
 *    Places the variable of datatype INTSWType into a Variable Binding.
 *
 *  Arguments:
 *
 *    VarBind            pointer to the variable value pair
 *    invalue            address of the data
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 */

UINT
SMIBuildINTSWType(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for building
      IN char *invalue )
{
    Integer *svalue = (Integer *)invalue;
    VarBind->value.asnType = ASN_INTEGER ;
    VarBind->value.asnValue.number = *svalue ;
    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of SMIBuildINTSWType() */


/*
 *  SMIGetINThrSWRunStatus
 *    INThrSWRunStatus ::= INTEGER 
 *    
 *    Encompasses the callouts to variables for the data type INThrSWRunStatus
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      cindex                    index to the class of the request
 *      vindex                    index to the variable of the request
 *      instance                  address of the instance specification
 *                                in the form of ordered native datatypes
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 *    SNMP_ERRORSTATUS_NOERROR    Successful get
 *    SNMP_ERRORSTATUS_GENERR     Catch-all failure code
 * mibtsmii.c v 0.6
 */

UINT
SMIGetINThrSWRunStatus(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for get
      IN unsigned long int cindex ,
      IN unsigned long int vindex ,
      IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    INThrSWRunStatus outvalue ;
    Access_Credential access ;  // dummy holder for future use

    result = ( *class_info[ cindex ].variable[ vindex ].VarGet )( &outvalue ,
                                                                  &access ,
                                                                  instance ) ;
    if ( result == SNMP_ERRORSTATUS_NOERROR )
    {
        VarBind->value.asnType = ASN_INTEGER ;
        VarBind->value.asnValue.number =
                               (AsnInteger)outvalue ;
    }
    return result ;

} /* end of SMIGetINThrSWRunStatus() */

/*
 *  SMISetINThrSWRunStatus
 *    INThrSWRunStatus ::= INTEGER 
 *    
 *    Encompasses the callouts to variables for the data type INThrSWRunStatus
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      cindex                    index to the class of the request
 *      vindex                    index to the variable of the request
 *      instance                  address of the instance specification
 *                                in the form of ordered native datatypes
 *  Return Codes:
 *
 *    Standard PDU error codes.
 */

UINT
SMISetINThrSWRunStatus(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for set
      IN unsigned long int cindex ,
      IN unsigned long int vindex ,
      IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    INThrSWRunStatus *invalue ;
    INThrSWRunStatus outvalue ;
    Access_Credential access ;   // dummy holder for future use

    invalue = (INThrSWRunStatus *)
              ( &VarBind->value.asnValue.number ) ;
    result = ( *class_info[ cindex ].variable[ vindex ].VarSet )
             ( invalue, &outvalue, &access, instance ) ;
    return result;

} /* end of SMISetINThrSWRunStatus() */

/*
 *  SMIBuildINThrSWRunStatus
 *    INThrSWRunStatus ::= INTEGER 
 *    
 *    Places the variable of datatype INThrSWRunStatus into a Variable Binding.
 *
 *  Arguments:
 *
 *    VarBind            pointer to the variable value pair
 *    invalue            address of the data
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 */

UINT
SMIBuildINThrSWRunStatus(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for building
      IN char *invalue )
{
    Integer *svalue = (Integer *)invalue;
    VarBind->value.asnType = ASN_INTEGER ;
    VarBind->value.asnValue.number = *svalue ;
    return SNMP_ERRORSTATUS_NOERROR ;

} /* end of SMIBuildINThrSWRunStatus() */


/*
 *  SMIGetDateAndTime
 *    DateAndTime ::= OCTET STRING (SIZE ( 8 | 11))  A date-time specification 
 *    for the local time of day.  This data type is intended toprovide a consistent method of  reporting 
 *    dat
 *    
 *    Encompasses the callouts to variables for the data type DateAndTime
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      cindex                    index to the class of the request
 *      vindex                    index to the variable of the request
 *      instance                  address of the instance specification
 *                                in the form of ordered native datatypes
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 *    SNMP_ERRORSTATUS_NOERROR    Successful get
 *    SNMP_ERRORSTATUS_GENERR     Catch-all failure code
 * mibtsmio.c v 0.5
 */

UINT
SMIGetDateAndTime(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for get
      IN unsigned long int cindex ,
      IN unsigned long int vindex ,
      IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    DateAndTime outvalue ;
    char stream[ MAX_OCTET_STRING ] ;
    Access_Credential access ;  // dummy holder for future use

    outvalue.string = stream ;
    outvalue.length = 0 ;
    result = ( *class_info[ cindex ].variable[ vindex ].VarGet )( &outvalue ,
                                                                  &access ,
                                                                  instance ) ;
    if ( result == SNMP_ERRORSTATUS_NOERROR )
    {
        VarBind->value.asnValue.string.length = outvalue.length ;
        VarBind->value.asnValue.string.stream =
//            malloc( outvalue.length * sizeof( char ) ) ;
// Changed 5/15/97 DDB
            SNMP_malloc( outvalue.length * sizeof( char ) ) ;
        if ( VarBind->value.asnValue.string.stream == NULL )
            result = SNMP_ERRORSTATUS_GENERR ;
        else
        {
            memcpy( VarBind->value.asnValue.string.stream ,
                    outvalue.string ,
                    outvalue.length ) ;
            VarBind->value.asnType = ASN_OCTETSTRING ;
            VarBind->value.asnValue.string.dynamic = TRUE ;
        }
    }
    return result ;
} /* end of SMIGetDateAndTime() */

/*
 *  SMISetDateAndTime
 *    DateAndTime ::= OCTET STRING (SIZE ( 8 | 11))  A date-time specification 
 *    for the local time of day.  This data type is intended toprovide a consistent method of  reporting 
 *    dat
 *    
 *    Encompasses the callouts to variables for the data type DateAndTime
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      cindex                    index to the class of the request
 *      vindex                    index to the variable of the request
 *      instance                  address of the instance specification
 *                                in the form of ordered native datatypes
 *  Return Codes:
 *
 *    Standard PDU error codes.
 */

UINT
SMISetDateAndTime(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for set
      IN unsigned long int cindex ,
      IN unsigned long int vindex ,
      IN InstanceName *instance )
{
    UINT result  = SNMP_ERRORSTATUS_NOERROR ;
    DateAndTime invalue ;
    DateAndTime outvalue ;
    char out_stream[ MAX_OCTET_STRING ] ;
    AsnOctetString *tmp ;
    Access_Credential access ;   // dummy holder for future use

    tmp = &VarBind->value.asnValue.string ;
    invalue.length = tmp->length ;
    invalue.string = tmp->stream ;
    outvalue.string = out_stream ;
    outvalue.length = 0 ;
    result = ( *class_info[ cindex ].variable[ vindex ].VarSet )
             ( &invalue, &outvalue, &access, instance ) ;
    return result;
} /* end of SMISetDateAndTime() */

/*
 *  SMIBuildDateAndTime
 *    DateAndTime ::= OCTET STRING (SIZE ( 8 | 11))  A date-time specification 
 *    for the local time of day.  This data type is intended toprovide a consistent method of  reporting 
 *    dat
 *    
 *    Places the variable of datatype DateAndTime into a Variable Binding
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      invalue                   address of the data
 *  Return Codes:
 *
 *    Standard PDU error codes.
 */

UINT
SMIBuildDateAndTime(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for set
      IN char *invalue )
{
    OctetString *svalue = (OctetString *)invalue ;
    UINT status = SNMP_ERRORSTATUS_NOERROR ;

    VarBind->value.asnValue.string.length = svalue->length ;
    VarBind->value.asnValue.string.stream =
//        malloc( svalue->length * sizeof( char ) ) ;
// Changed 5/15/97 DDB
        SNMP_malloc( svalue->length * sizeof( char ) ) ;
    if ( VarBind->value.asnValue.string.stream == NULL )
        status = SNMP_ERRORSTATUS_GENERR ;
    else
    {
        memcpy( VarBind->value.asnValue.string.stream ,
                svalue->string ,
                svalue->length ) ;
        VarBind->value.asnType = ASN_OCTETSTRING ;
        VarBind->value.asnValue.string.dynamic = TRUE ;
    }
    return status ;
} /* end of SMIBuildDateAndTime() */


/*
 *  SMIGetInternationalDisplayString
 *    InternationalDisplayString ::= OCTET STRING This data type is used to 
 *    model textual information in some character set.  A network management station should use a local 
 *    algo
 *    
 *    Encompasses the callouts to variables for the data type InternationalDisplayString
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      cindex                    index to the class of the request
 *      vindex                    index to the variable of the request
 *      instance                  address of the instance specification
 *                                in the form of ordered native datatypes
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 *    SNMP_ERRORSTATUS_NOERROR    Successful get
 *    SNMP_ERRORSTATUS_GENERR     Catch-all failure code
 * mibtsmio.c v 0.5
 */

UINT
SMIGetInternationalDisplayString(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for get
      IN unsigned long int cindex ,
      IN unsigned long int vindex ,
      IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    InternationalDisplayString outvalue ;
    char stream[ MAX_OCTET_STRING ] ;
    Access_Credential access ;  // dummy holder for future use

    outvalue.string = stream ;
    outvalue.length = 0 ;
    result = ( *class_info[ cindex ].variable[ vindex ].VarGet )( &outvalue ,
                                                                  &access ,
                                                                  instance ) ;
    if ( result == SNMP_ERRORSTATUS_NOERROR )
    {
        VarBind->value.asnValue.string.length = outvalue.length ;
        VarBind->value.asnValue.string.stream =
//            malloc( outvalue.length * sizeof( char ) ) ;
// Changed 5/15/97 DDB
            SNMP_malloc( outvalue.length * sizeof( char ) ) ;
        if ( VarBind->value.asnValue.string.stream == NULL )
            result = SNMP_ERRORSTATUS_GENERR ;
        else
        {
            memcpy( VarBind->value.asnValue.string.stream ,
                    outvalue.string ,
                    outvalue.length ) ;
            VarBind->value.asnType = ASN_OCTETSTRING ;
            VarBind->value.asnValue.string.dynamic = TRUE ;
        }
    }
    return result ;
} /* end of SMIGetInternationalDisplayString() */

/*
 *  SMISetInternationalDisplayString
 *    InternationalDisplayString ::= OCTET STRING This data type is used to 
 *    model textual information in some character set.  A network management station should use a local 
 *    algo
 *    
 *    Encompasses the callouts to variables for the data type InternationalDisplayString
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      cindex                    index to the class of the request
 *      vindex                    index to the variable of the request
 *      instance                  address of the instance specification
 *                                in the form of ordered native datatypes
 *  Return Codes:
 *
 *    Standard PDU error codes.
 */

UINT
SMISetInternationalDisplayString(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for set
      IN unsigned long int cindex ,
      IN unsigned long int vindex ,
      IN InstanceName *instance )
{
    UINT result  = SNMP_ERRORSTATUS_NOERROR ;
    InternationalDisplayString invalue ;
    InternationalDisplayString outvalue ;
    char out_stream[ MAX_OCTET_STRING ] ;
    AsnOctetString *tmp ;
    Access_Credential access ;   // dummy holder for future use

    tmp = &VarBind->value.asnValue.string ;
    invalue.length = tmp->length ;
    invalue.string = tmp->stream ;
    outvalue.string = out_stream ;
    outvalue.length = 0 ;
    result = ( *class_info[ cindex ].variable[ vindex ].VarSet )
             ( &invalue, &outvalue, &access, instance ) ;
    return result;
} /* end of SMISetInternationalDisplayString() */

/*
 *  SMIBuildInternationalDisplayString
 *    InternationalDisplayString ::= OCTET STRING This data type is used to 
 *    model textual information in some character set.  A network management station should use a local 
 *    algo
 *    
 *    Places the variable of datatype InternationalDisplayString into a Variable Binding
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      invalue                   address of the data
 *  Return Codes:
 *
 *    Standard PDU error codes.
 */

UINT
SMIBuildInternationalDisplayString(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for set
      IN char *invalue )
{
    OctetString *svalue = (OctetString *)invalue ;
    UINT status = SNMP_ERRORSTATUS_NOERROR ;

    VarBind->value.asnValue.string.length = svalue->length ;
    VarBind->value.asnValue.string.stream =
//        malloc( svalue->length * sizeof( char ) ) ;
// Changed 5/15/97 DDB
        SNMP_malloc( svalue->length * sizeof( char ) ) ;
    if ( VarBind->value.asnValue.string.stream == NULL )
        status = SNMP_ERRORSTATUS_GENERR ;
    else
    {
        memcpy( VarBind->value.asnValue.string.stream ,
                svalue->string ,
                svalue->length ) ;
        VarBind->value.asnType = ASN_OCTETSTRING ;
        VarBind->value.asnValue.string.dynamic = TRUE ;
    }
    return status ;
} /* end of SMIBuildInternationalDisplayString() */


/*
 *  SMIGetProductID
 *    ProductID ::= OBJECT IDENTIFIER This textual convention is intended to 
 *    identify the manufacturer, model, and version of a specific hardware or software 
 *    product.
 *    
 *    Encompasses the callouts to variables for the data type ProductID
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      cindex                    index to the class of the request
 *      vindex                    index to the variable of the request
 *      instance                  address of the instance specification
 *                                in the form of ordered native datatypes
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 *    SNMP_ERRORSTATUS_NOERROR    Successful get
 *    SNMP_ERRORSTATUS_GENERR     Catch-all failure code
 * mibtsmib.c v 0.5
 */

UINT
SMIGetProductID(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for get
      IN unsigned long int cindex ,
      IN unsigned long int vindex ,
      IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    UINT status ;
    ProductID outvalue ;
    Access_Credential access ;  // dummy holder for future use

    memset( &outvalue, '\0', sizeof( ProductID ) ) ;
    result = ( *class_info[ cindex ].variable[ vindex ].VarGet )( &outvalue ,
                                                                  &access ,
                                                                  instance ) ;
    if ( result == SNMP_ERRORSTATUS_NOERROR )
    {
        status = SNMP_oidcpy( &VarBind->value.asnValue.object, &outvalue ) ;
        if ( !status )
            result = SNMP_ERRORSTATUS_GENERR ;
        else
        {
            if ( outvalue.idLength != 0 )
                SnmpUtilOidFree( &outvalue ) ;
            VarBind->value.asnType = ASN_OBJECTIDENTIFIER ;
        }
    }
    return result ;
} /* end of SMIGetProductID() */

/*
 *  SMISetProductID
 *    ProductID ::= OBJECT IDENTIFIER This textual convention is intended to 
 *    identify the manufacturer, model, and version of a specific hardware or software 
 *    product.
 *    
 *    Encompasses the callouts to variables for the data type ProductID
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      cindex                    index to the class of the request
 *      vindex                    index to the variable of the request
 *      instance                  address of the instance specification
 *                                in the form of ordered native datatypes
 *  Return Codes:
 *
 *    Standard PDU error codes.
 */

UINT
SMISetProductID(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for set
      IN unsigned long int cindex ,
      IN unsigned long int vindex ,
      IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    ProductID *invalue ;
    ProductID outvalue ;
    Access_Credential access ;   // dummy holder for future use

    invalue = (ProductID *)&VarBind->value.asnValue.object ;
    memset( &outvalue, '\0', sizeof( ProductID ) ) ;
    result = ( *class_info[ cindex ].variable[ vindex ].VarSet )
             ( invalue, &outvalue, &access, instance ) ;
    if ( outvalue.idLength != 0 )
        SnmpUtilOidFree( &outvalue ) ;
    return result ;
} /* end of SMISetProductID() */


/*
 *  SMIBuildProductID
 *    ProductID ::= OBJECT IDENTIFIER This textual convention is intended to 
 *    identify the manufacturer, model, and version of a specific hardware or software 
 *    product.
 *    
 *    Encompasses the callouts to variables for the data type ProductID
 *
 *  Arguments:
 *	VarBind                   pointer to the variable value pair
 *      invalue                   address of the data
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 */

UINT
SMIBuildProductID(
      IN OUT RFC1157VarBind *VarBind , // Variable binding for set
      IN char *invalue )
{
    AsnObjectIdentifier *svalue = (AsnObjectIdentifier *)invalue ;
    UINT status = SNMP_ERRORSTATUS_NOERROR ;
    UINT sts = TRUE ;

    sts = SNMP_oidcpy( &VarBind->value.asnValue.object ,
                       (AsnObjectIdentifier *)svalue ) ;
    if (!sts)
        status = SNMP_ERRORSTATUS_GENERR ;
    else
        VarBind->value.asnType = ASN_OBJECTIDENTIFIER ;

    return status ;

} /* end of SMIBuildProductID() */


/* end of hostmsmi.c */

