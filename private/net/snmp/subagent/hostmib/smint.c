/*
 *  smint.c v0.12  Mar 12 1996
 *
 ****************************************************************************
 *                                                                          *
 *      (C) Copyright 1995, 1996 DIGITAL EQUIPMENT CORPORATION              *
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
 *    This module contains the SMI envelopes around the callout to the user's
 *    get and set routines.
 *
 *        SMIGetInteger
 *        SMIGetNSMBoolean
 *        SMIGetBIDTEnum
 *        SMIGetOctetString
 *        SMIGetObjectId
 *        SMIGetCounter
 *        SMIGetGauge
 *        SMIGetTimeTicks
 *        SMIGetIpAddress
 *        SMIGetDispString
 *        SMISetInteger
 *        SMISetNSMBoolean
 *        SMISetBIDTEnum
 *        SMISetOctetString
 *        SMISetObjectId
 *        SMISetCounter
 *        SMISetGauge
 *        SMISetTimeTicks
 *        SMISetIpAddress
 *        SMISetDispString
 *        SMIBuildInteger
 *        SMIBuildDIDTEnum
 *        SMIBuildOctetString
 *        SMIBuildObjectId
 *        SMIBuildCounter
 *        SMIBuildGauge
 *        SMIBuildTimeTicks
 *        SMIBuildIpAddress
 *        SMIBuildDispString
 *        SMIFree
 *
 *  Author:
 *     Wayne Duso, Miriam Amos Nihart, Kathy Faust
 *
 *  Date:
 *     2/17/95
 *
 *  Revision History:
 *  v0.1   Jul 20 95     AGS  Added SMIGet/SetBoolean
 *  v0.11  Feb 14 1996   AGS  changed SMIGet/SetBoolean to SMIGet/SetNSMBoolean
 *  v0.12  Mar 12, 1996  KKF  set outvalue.length to 256 for SMISetOctetString,
 *                            SMISetDispString so that instrumentation code knows
 *                            max length of buffer.
 *  v0.13  May 15, 1997  DDB  To Microsoft: 6 changes of "malloc" to
 *                              "SNMP_malloc"
 */


// Necessary includes.

#include <snmp.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>

#include "mib.h"
#include "mib_xtrn.h"
#include "smint.h"        // Wayne's type def file



/*
 *  SMIGetInteger
 *
 *    Encompasses the callouts to variables of the data type INTEGER.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    cindex                     index to the class of the request
 *    vindex                     index to the variable of the request
 *    instance                   address of the instance specification in the
 *                               form of ordered native datatypes
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMIGetInteger( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for get
               IN unsigned long int cindex ,
               IN unsigned long int vindex ,
               IN InstanceName *instance )

{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    Integer outvalue ;
    Access_Credential access ;    // dummy holder for future use

    result = ( *class_info[ cindex ].variable[ vindex ].VarGet )( &outvalue ,
                                                                  &access ,
                                                                  instance ) ;

    if ( result == SNMP_ERRORSTATUS_NOERROR )
    {
        VarBind->value.asnType = ASN_INTEGER ;
        VarBind->value.asnValue.number = (AsnInteger)outvalue ;
    }
    return result ;

} /* end of SMIGetInteger() */



/*
 *  SMIGetNSMBoolean
 *
 *    Encompasses the callouts to variables of the data type Boolean.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    cindex                     index to the class of the request
 *    vindex                     index to the variable of the request
 *    instance                   address of the instance specification in the
 *                               form of ordered native datatypes
 *
 *    In SNMPv1  true = 1 AND false = 2
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMIGetNSMBoolean( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for get
               IN unsigned long int cindex ,
               IN unsigned long int vindex ,
               IN InstanceName *instance )

{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    NSM_Boolean outvalue ;	  // nsm_true = 1, nsm_false = 2
    Access_Credential access ;    // dummy holder for future use

    result = ( *class_info[ cindex ].variable[ vindex ].VarGet )( &outvalue ,
                                                                  &access ,
                                                                  instance ) ;

    if ( result == SNMP_ERRORSTATUS_NOERROR )
    {
        VarBind->value.asnType = ASN_INTEGER ;
        VarBind->value.asnValue.number = (AsnInteger)outvalue ;
    }
    return result ;

} /* end of SMIGetNSMBoolean() */




/*
 *  SMIGetBIDTEnum
 *
 *    Encompasses the callouts to variables of the data type INTEGER that
 *    are enumerated.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    cindex                     index to the class of the request
 *    vindex                     index to the variable of the request
 *    instance                   address of the instance specification in the
 *                               form of ordered native datatypes
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMIGetBIDTEnum( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for get
                IN unsigned long int cindex ,
                IN unsigned long int vindex ,
                IN InstanceName *instance )

{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    BIDT_ENUMERATION outvalue ;
    Access_Credential access ;    // dummy holder for future use

    result = ( *class_info[ cindex ].variable[ vindex ].VarGet )( &outvalue ,
                                                                  &access ,
                                                                  instance ) ;

    if ( result == SNMP_ERRORSTATUS_NOERROR )
    {
        VarBind->value.asnType = ASN_INTEGER ;
        VarBind->value.asnValue.number = (AsnInteger)outvalue ;
    }
    return result ;

}  /* end of SMIGetBIDTEnum() */





/*
 *  SMIGetOctetString
 *
 *    Encompasses the callouts to variables of the data type OCTET STRING.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    cindex                     index to the class of the request
 *    vindex                     index to the variable of the request
 *    instance                   address of the instance specification in the
 *                               form of ordered native datatypes
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMIGetOctetString( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for get
                   IN unsigned long int cindex ,
                   IN unsigned long int vindex ,
                   IN InstanceName *instance )

{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    OctetString outvalue ;
    char stream[ MAX_OCTET_STRING ] ;
    Access_Credential access ;    // dummy holder for future use

    outvalue.string = stream ;
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
            VarBind->value.asnType =  ASN_OCTETSTRING ;
            VarBind->value.asnValue.string.dynamic = TRUE ;
        }
    }
    return result ;

}  /* end of SMIGetOctetString() */




/*
 *  SMIGetObjectId
 *
 *    Encompasses the callouts to variables of the data type OBJECT IDENTIFIER.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    cindex                     index to the class of the request
 *    vindex                     index to the variable of the request
 *    instance                   address of the instance specification in the
 *                               form of ordered native datatypes
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMIGetObjectId( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for get
                IN unsigned long int cindex ,
                IN unsigned long int vindex ,
                IN InstanceName *instance )

{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    UINT status ;
    ObjectIdentifier outvalue ;
    Access_Credential access ;    // dummy holder for future use

    memset( &outvalue, '\0', sizeof( ObjectIdentifier ) ) ;
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
			SNMP_free( outvalue.ids ) ;
            VarBind->value.asnType = ASN_OBJECTIDENTIFIER ;
        }
    }
    return result ;

}  /* end of SMIGetObjectId() */




/*
 *  SMIGetCounter
 *
 *    Encompasses the callouts to variables of the data type COUNTER.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    cindex                     index to the class of the request
 *    vindex                     index to the variable of the request
 *    instance                   address of the instance specification in the
 *                               form of ordered native datatypes
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMIGetCounter( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for get
               IN unsigned long int cindex ,
               IN unsigned long int vindex ,
               IN InstanceName *instance )

{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    Counter outvalue ;
    Access_Credential access ;    // dummy holder for future use

    result = ( *class_info[ cindex ].variable[ vindex ].VarGet )( &outvalue ,
                                                                  &access ,
                                                                  instance ) ;
    if ( result == SNMP_ERRORSTATUS_NOERROR )
    {
        VarBind->value.asnType = ASN_RFC1155_COUNTER ;
        VarBind->value.asnValue.counter = outvalue ;
    }
    return result ;

}  /* end of SMIGetCounter() */




/*
 *  SMIGetGauge
 *
 *    Encompasses the callouts to variables of the data type GAUGE.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    cindex                     index to the class of the request
 *    vindex                     index to the variable of the request
 *    instance                   address of the instance specification in the
 *                               form of ordered native datatypes
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMIGetGauge( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for get
             IN unsigned long int cindex ,
             IN unsigned long int vindex ,
             IN InstanceName *instance )

{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    Gauge outvalue ;
    Access_Credential access ;    // dummy holder for future use

    result = ( *class_info[ cindex ].variable[ vindex ].VarGet )( &outvalue ,
                                                                  &access ,
                                                                  instance ) ;
    if ( result == SNMP_ERRORSTATUS_NOERROR )
    {
        VarBind->value.asnType = ASN_RFC1155_GAUGE ;
        VarBind->value.asnValue.gauge = outvalue ;
    }
    return result ;

}  /* end of SMIGetGauge() */




/*
 *  SMIGetTimeTicks
 *
 *    Encompasses the callouts to variables of the data type TIMETICKS.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    cindex                     index to the class of the request
 *    vindex                     index to the variable of the request
 *    instance                   address of the instance specification in the
 *                               form of ordered native datatypes
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMIGetTimeTicks( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for get
                 IN unsigned long int cindex ,
                 IN unsigned long int vindex ,
                 IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    TimeTicks outvalue ;
    Access_Credential access ;    // dummy holder for future use

    result = ( *class_info[ cindex ].variable[ vindex ].VarGet )( &outvalue ,
                                                                  &access ,
                                                                  instance ) ;
    if ( result == SNMP_ERRORSTATUS_NOERROR )
    {
        VarBind->value.asnType = ASN_RFC1155_TIMETICKS ;
        VarBind->value.asnValue.ticks = outvalue ;
    }
    return result ;

}  /* end of SMIGetTimeTicks() */




/*
 *  SMIGetIpAddress
 *
 *    Encompasses the callouts to variables of the data type IP ADDRESS.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    cindex                     index to the class of the request
 *    vindex                     index to the variable of the request
 *    instance                   address of the instance specification in the
 *                               form of ordered native datatypes
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMIGetIpAddress( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for get
                 IN unsigned long int cindex ,
                 IN unsigned long int vindex ,
                 IN InstanceName *instance )

{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    IpAddress outvalue ;
    Access_Credential access ;    // dummy holder for future use

    result = ( *class_info[ cindex ].variable[ vindex ].VarGet )( &outvalue ,
                                                                  &access ,
                                                                  instance ) ;
    if ( result == SNMP_ERRORSTATUS_NOERROR )
    {
        VarBind->value.asnValue.address.length = 4 ;
//        VarBind->value.asnValue.address.stream = malloc( 4 * sizeof( char ) ) ;
// Changed 5/15/97 DDB
        VarBind->value.asnValue.address.stream = SNMP_malloc( 4 * sizeof( char ) ) ;
        if ( VarBind->value.asnValue.address.stream == NULL )
            result = SNMP_ERRORSTATUS_GENERR ;
        else
        {
            memcpy( VarBind->value.asnValue.address.stream ,
                    (BYTE *)(&outvalue),
                    4 ) ;
            VarBind->value.asnType = ASN_RFC1155_IPADDRESS ;
            VarBind->value.asnValue.address.dynamic = TRUE ;
        }
    }
    return result ;

}  /* end of SMIGetIpAddress() */



/*
 *  SMIGetDispString
 *
 *    Encompasses the callouts to variables of the data type DISPLAY STRING.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    cindex                     index to the class of the request
 *    vindex                     index to the variable of the request
 *    instance                   address of the instance specification in the
 *                               form of ordered native datatypes
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMIGetDispString( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for get
                  IN unsigned long int cindex ,
                  IN unsigned long int vindex ,
                  IN InstanceName *instance )

{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    Simple_DisplayString outvalue ;
    char stream[ MAX_OCTET_STRING ] ;
    Access_Credential access ;    // dummy holder for future use

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
                    VarBind->value.asnValue.string.length ) ;
            VarBind->value.asnType = ASN_RFC1213_DISPSTRING ;
            VarBind->value.asnValue.string.dynamic = TRUE ;
        }
    }
    return result ;

}  /* end of SMIGetDispString() */




/*
 *  SMISetInteger
 *
 *    Encompasses the callouts to variables of the data type INTEGER.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    cindex                     index to the class of the request
 *    vindex                     index to the variable of the request
 *    instance                   address of the instance specification in the
 *                               form of ordered native datatypes
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMISetInteger( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for set
               IN unsigned long int cindex ,
               IN unsigned long int vindex ,
               IN InstanceName *instance )

{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    Integer *invalue ;
    Integer outvalue ;
    Access_Credential access ;    // dummy holder for future use

    invalue = (Integer *)( &VarBind->value.asnValue.number ) ;
    result = ( *class_info[ cindex ].variable[ vindex ].VarSet )
             ( invalue, &outvalue, &access, instance ) ;
    return result ;

}  /* end of SMISetInteger() */




/*
 *  SMISetNSMBoolean
 *
 *    Encompasses the callouts to variables of the data type Boolean
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    cindex                     index to the class of the request
 *    vindex                     index to the variable of the request
 *    instance                   address of the instance specification in the
 *                               form of ordered native datatypes
 *    In SNMPv1  true = 1 AND false = 2
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMISetNSMBoolean( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for set
               IN unsigned long int cindex ,
               IN unsigned long int vindex ,
               IN InstanceName *instance )

{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    NSM_Boolean *invalue ;        // nsm_true = 1, nsm_false = 2
    NSM_Boolean outvalue ;
    Access_Credential access ;    // dummy holder for future use

    invalue = (NSM_Boolean *)( &VarBind->value.asnValue.number ) ;
    result = ( *class_info[ cindex ].variable[ vindex ].VarSet )
             ( invalue, &outvalue, &access, instance ) ;
    return result ;

}  /* end of SMISetNSMBoolean() */




/*
 *  SMISetBIDTEmun
 *
 *    Encompasses the callouts to variables of the data type INTEGER that
 *    is enumerated.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    cindex                     index to the class of the request
 *    vindex                     index to the variable of the request
 *    instance                   address of the instance specification in the
 *                               form of ordered native datatypes
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMISetBIDTEnum( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for set
               IN unsigned long int cindex ,
               IN unsigned long int vindex ,
               IN InstanceName *instance )

{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    BIDT_ENUMERATION *invalue ;
    BIDT_ENUMERATION outvalue ;
    Access_Credential access ;    // dummy holder for future use

    invalue = (BIDT_ENUMERATION *)( &VarBind->value.asnValue.number ) ;
    result = ( *class_info[ cindex ].variable[ vindex ].VarSet )
             ( invalue, &outvalue, &access, instance ) ;
    return result ;

}  /* end of SMISetBIDTEnum() */





/*
 *  SMISetOctetString
 *
 *    Encompasses the callouts to variables of the data type OCTET STRING.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    cindex                     index to the class of the request
 *    vindex                     index to the variable of the request
 *    instance                   address of the instance specification in the
 *                               form of ordered native datatypes
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMISetOctetString( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for set
                   IN unsigned long int cindex ,
                   IN unsigned long int vindex ,
                   IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    OctetString invalue ;
    OctetString outvalue ;
    char out_stream[ MAX_OCTET_STRING ] ;
    AsnOctetString *tmp ;
    Access_Credential access ;    // dummy holder for future use

    tmp = &VarBind->value.asnValue.string ;
    invalue.length = tmp->length ;
    invalue.string = tmp->stream ;
    outvalue.string = out_stream ;
    outvalue.length = 256 ;
    result = ( *class_info[ cindex ].variable[ vindex ].VarSet )
             ( &invalue, &outvalue, &access, instance ) ;
    return result ;

}  /* end of SMISetOctetString() */




/*
 *  SMISetObjectId
 *
 *    Encompasses the callouts to variables of the data type OBJECT IDENTIFIER.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    cindex                     index to the class of the request
 *    vindex                     index to the variable of the request
 *    instance                   address of the instance specification in the
 *                               form of ordered native datatypes
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMISetObjectId( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for set
                IN unsigned long int cindex ,
                IN unsigned long int vindex ,
                IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    ObjectIdentifier *invalue ;
    ObjectIdentifier outvalue ;
    Access_Credential access ;    // dummy holder for future use

    invalue = &VarBind->value.asnValue.object ;
    memset( &outvalue, '\0', sizeof ( ObjectIdentifier ) ) ;
    result = ( *class_info[ cindex ].variable[ vindex ].VarSet )
             ( invalue, &outvalue, &access, instance ) ;
    if ( outvalue.idLength != 0 )
		SNMP_free( outvalue.ids ) ;

    return result ;

}  /* end of SMISetObjectId() */




/*
 *  SMISetCounter
 *
 *    Encompasses the callouts to variables of the data type COUNTER.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    cindex                     index to the class of the request
 *    vindex                     index to the variable of the request
 *    instance                   address of the instance specification in the
 *                               form of ordered native datatypes
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMISetCounter( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for set
               IN unsigned long int cindex ,
               IN unsigned long int vindex ,
               IN InstanceName *instance )

{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    Counter *invalue ;
    Counter outvalue ;
    Access_Credential access ;    // dummy holder for future use

    invalue = (Counter *)( &VarBind->value.asnValue.counter ) ;
    result = ( *class_info[ cindex ].variable[ vindex ].VarSet )
             ( invalue, &outvalue, &access, instance ) ;
    return result ;

}  /* end of SMISetCounter() */



/*
 *  SMISetGauge
 *
 *    Encompasses the callouts to variables of the data type GAUGE.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    cindex                     index to the class of the request
 *    vindex                     index to the variable of the request
 *    instance                   address of the instance specification in the
 *                               form of ordered native datatypes
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMISetGauge( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for set
             IN unsigned long int cindex ,
             IN unsigned long int vindex ,
             IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    Gauge *invalue ;
    Gauge outvalue ;
    Access_Credential access ;    // dummy holder for future use

    invalue = (Gauge *)( &VarBind->value.asnValue.gauge ) ;
    result = ( *class_info[ cindex ].variable[ vindex ].VarSet )
             ( invalue, &outvalue, &access, instance ) ;
    return result ;

}  /* end of SMISetGauge() */



/*
 *  SMISetTimeTicks
 *
 *    Encompasses the callouts to variables of the data type TIMETICKS.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    cindex                     index to the class of the request
 *    vindex                     index to the variable of the request
 *    instance                   address of the instance specification in the
 *                               form of ordered native datatypes
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMISetTimeTicks( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for set
                 IN unsigned long int cindex ,
                 IN unsigned long int vindex ,
                 IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    TimeTicks *invalue ;
    TimeTicks outvalue ;
    Access_Credential access ;    // dummy holder for future use

    invalue = (TimeTicks *)( &VarBind->value.asnValue.ticks ) ;
    result = ( *class_info[ cindex ].variable[ vindex ].VarSet )
             ( invalue , &outvalue, &access, instance ) ;
    return result ;

}  /* end of SMISetTimeTicks() */




/*
 *  SMISetIpAddress
 *
 *    Encompasses the callouts to variables of the data type IP ADDRESS.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    cindex                     index to the class of the request
 *    vindex                     index to the variable of the request
 *    instance                   address of the instance specification in the
 *                               form of ordered native datatypes
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMISetIpAddress( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for set
                 IN unsigned long int cindex ,
                 IN unsigned long int vindex ,
                 IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    IpAddress invalue ;
    IpAddress outvalue ;
    Access_Credential access ;    // dummy holder for future use

    memcpy( &invalue, VarBind->value.asnValue.address.stream , 4 ) ;
    result = ( *class_info[ cindex ].variable[ vindex ].VarSet )
             ( &invalue, &outvalue, &access, instance ) ;
    return result ;

}  /* end of SMISetIpAddress() */



/*
 *  SMISetDispString
 *
 *    Encompasses the callouts to variables of the data type DISPLAY STRING.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    cindex                     index to the class of the request
 *    vindex                     index to the variable of the request
 *    instance                   address of the instance specification in the
 *                               form of ordered native datatypes
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMISetDispString( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for set
                  IN unsigned long int cindex ,
                  IN unsigned long int vindex ,
                  IN InstanceName *instance )
{
    UINT result = SNMP_ERRORSTATUS_NOERROR ;
    Simple_DisplayString invalue ;
    Simple_DisplayString outvalue ;
    char out_stream[ MAX_OCTET_STRING ] ;
    AsnOctetString *tmp ;
    Access_Credential access ;    // dummy holder for future use

    tmp = &VarBind->value.asnValue.string ;
    invalue.length = tmp->length ;
    invalue.string = tmp->stream ;
    outvalue.string = out_stream ;
    outvalue.length = 256 ;
    result = ( *class_info[ cindex ].variable[ vindex ].VarSet )
             ( &invalue, &outvalue, &access, instance ) ;
    return result ;

}  /* end of SMISetDispString() */




/*
 *  SMIBuildInteger
 *
 *    Places the variable of datatype INTEGER into a Variable Binding.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    invalue                     address of the data
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMIBuildInteger( IN OUT RFC1157VarBind *VarBind ,
                 IN char *invalue )

{
    Integer *svalue = (Integer *)invalue ;
    VarBind->value.asnType = ASN_INTEGER ;
    VarBind->value.asnValue.number = *svalue ;

    return SNMP_ERRORSTATUS_NOERROR ;

}  /* end of SMIBuildInteger() */




/*
 *  SMIBuildOctetString
 *
 *    Places the variable of datatype OCTET STRING into a Variable Binding.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    invalue                    address of the data
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMIBuildOctetString( IN OUT RFC1157VarBind *VarBind ,
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
        VarBind->value.asnType =  ASN_OCTETSTRING ;
        VarBind->value.asnValue.string.dynamic = TRUE ;
    }
    return status ;

}  /* end of SMIBuildOctetString() */




/*
 *  SMIBuildObjectId
 *
 *    Places the variable of datatype OBJECT IDENTIFIER into a Variable Binding.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    invalue                    address of the data
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMIBuildObjectId( IN OUT RFC1157VarBind *VarBind ,
                  IN char *invalue )


{
    ObjectIdentifier *svalue = (ObjectIdentifier *)invalue ;
    UINT status = SNMP_ERRORSTATUS_NOERROR ;
    UINT sts = TRUE ;

    sts = SNMP_oidcpy( &VarBind->value.asnValue.object ,
                       (AsnObjectIdentifier *)svalue ) ;
    if ( !sts )
        status = SNMP_ERRORSTATUS_GENERR ;
    else
        VarBind->value.asnType = ASN_OBJECTIDENTIFIER ;

    return status ;

}  /* end of SMIBuildObjectId() */




/*
 *  SMIBuildCounter
 *
 *    Places the variable of datatype COUNTER into a Variable Binding.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    invalue                    address of the data
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMIBuildCounter( IN OUT RFC1157VarBind *VarBind ,
                 IN char *invalue )

{
    Counter *svalue = (Counter *)invalue ;
    VarBind->value.asnType = ASN_RFC1155_COUNTER ;
    VarBind->value.asnValue.counter = *svalue ;

    return SNMP_ERRORSTATUS_NOERROR ;

}  /* end of SMIBuildCounter() */




/*
 *  SMIBuildGauge
 *
 *    Places the variable of datatype GAUGE into a Variable Binding.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    svalue                     address of the data
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMIBuildGauge( IN OUT RFC1157VarBind *VarBind ,
               IN char *invalue )

{
    Gauge *svalue = (Gauge *)invalue ;
    VarBind->value.asnType = ASN_RFC1155_GAUGE ;
    VarBind->value.asnValue.gauge = *svalue ;

    return SNMP_ERRORSTATUS_NOERROR ;

}  /* end of SMIBuildGauge() */




/*
 *  SMIBuildTimeTicks
 *
 *    Places the variable of datatype TIME TICKS into a Variable Binding.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    invalue                    address of the data
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMIBuildTimeTicks( IN OUT RFC1157VarBind *VarBind ,
                   IN char *invalue )

{
    TimeTicks *svalue = (TimeTicks *)invalue ;
    VarBind->value.asnType = ASN_RFC1155_TIMETICKS ;
    VarBind->value.asnValue.ticks = *svalue ;

    return SNMP_ERRORSTATUS_NOERROR ;

}  /* end of SMIBuildTimeTicks() */




/*
 *  SMIBuildIpAddress
 *
 *    Places the variable of datatype IpAddress into a Variable Binding.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    invalue                    address of the data
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMIBuildIpAddress( IN OUT RFC1157VarBind *VarBind ,
                   IN char *invalue )

{
    IpAddress *svalue = (IpAddress *)invalue;
    UINT status = SNMP_ERRORSTATUS_NOERROR ;

    VarBind->value.asnValue.address.length = 4 ;
//    VarBind->value.asnValue.address.stream = malloc( 4 * sizeof( char ) ) ;
// Changed 5/15/97 DDB
    VarBind->value.asnValue.address.stream = SNMP_malloc( 4 * sizeof( char ) ) ;
    if ( VarBind->value.asnValue.address.stream == NULL )
        status = SNMP_ERRORSTATUS_GENERR ;
    else
    {
        memcpy( VarBind->value.asnValue.address.stream, (BYTE *)svalue, 4 ) ;
        VarBind->value.asnType = ASN_RFC1155_IPADDRESS ;
        VarBind->value.asnValue.address.dynamic = TRUE ;
    }
    return status ;

}  /* end of SMIBuildIpAddress() */



/*
 *  SMIBuildDispString
 *
 *    Places the variable of datatype DISPLAY STRING into a Variable Binding.
 *
 *  Arguments:
 *
 *    VarBind                    pointer to the variable value pair
 *    invalue                     address of the data
 *
 *  Return Codes:
 *
 *    Standard PDU error codes.
 *
 */

UINT
SMIBuildDispString( IN OUT RFC1157VarBind *VarBind ,
                    IN char *invalue )

{
    Simple_DisplayString *svalue = (Simple_DisplayString *)invalue;
    UINT status = SNMP_ERRORSTATUS_NOERROR ;

    VarBind->value.asnValue.string.length = svalue->length ;
    VarBind->value.asnValue.string.stream =
//        malloc( VarBind->value.asnValue.string.length * sizeof( char ) ) ;
// Changed 5/15/97 DDB
        SNMP_malloc( VarBind->value.asnValue.string.length * sizeof( char ) ) ;
    if ( VarBind->value.asnValue.string.stream == NULL )
        status = SNMP_ERRORSTATUS_GENERR ;
    else
    {
        memcpy( VarBind->value.asnValue.string.stream ,
                svalue->string ,
                VarBind->value.asnValue.string.length ) ;
        VarBind->value.asnType = ASN_RFC1213_DISPSTRING ;
        VarBind->value.asnValue.string.dynamic = TRUE ;
    }
    return status ;

}  /* end of SMIBuildDispString() */

/* end of smi.c */


/*  SMIFree
 *
 *    Free the variable
 *
 *  Arguments:
 *
 *    invalue                     address of data
 *
 *  Return Codes:
 *
 *
 */

void
SMIFree( IN AsnAny *invalue )

{
    switch (invalue->asnType) {

        case ASN_OCTETSTRING:
        case ASN_RFC1155_IPADDRESS:
            if (invalue->asnValue.string.length != 0) {
	            invalue->asnValue.string.length = 0 ;
	            free(invalue->asnValue.string.stream) ;
            }
	    break;

        case ASN_OBJECTIDENTIFIER:
            if (invalue->asnValue.object.idLength != 0)
	            SNMP_free(invalue->asnValue.object.ids) ;
	        break ;

        default:
			break ;
    }
}  /* end of SMIFree */

/* end of smi.c */

