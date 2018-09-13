/*
 *  mibtsmi.nth  v0.10
 *  hostmsmi.h
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
 *    This module contains the user defined type definitions.
 *
 *  Author:
 *
 *	David Burns @ Webenable Inc
 *
 *  Date:
 *
 *	Thu Nov 07 16:38:31 1996
 *
 *  Revision History:
 *      generated with v0.10 stub
 *
 */

#if !defined(_HOSTMSMI_H_)
#define _HOSTMSMI_H_

#include <snmp.h>
#include "smint.h"
/*
 *    Boolean ::= INTEGER a truth value
 */
typedef enum
{
    true = 1 ,
    false = 2
} Boolean ;
/*
 *    KBytes ::= INTEGER (0..2147483647)  memory size, expressed in units of 
 *    1024 bytes
 */
typedef Integer KBytes ;
/*
 *    INThrDeviceStatus ::= INTEGER 
 */
typedef enum
{
    unknown0 = 1 ,
    running0 = 2 ,
    warning0 = 3 ,
    testing0 = 4 ,
    down0 = 5
} INThrDeviceStatus ;
/*
 *    INThrPrinterStatus ::= INTEGER 
 */
typedef enum
{
    other1 = 1 ,
    unknown1 = 2 ,
    idle1 = 3 ,
    printing1 = 4 ,
    warmup1 = 5
} INThrPrinterStatus ;
/*
 *    INTAccess ::= INTEGER 
 */
typedef enum
{
    readWrite = 1 ,
    readOnly = 2
} INTAccess ;
/*
 *    INThrDiskStorageMedia ::= INTEGER 
 */
typedef enum
{
    other = 1 ,
    unknown = 2 ,
    hardDisk = 3 ,
    floppyDisk = 4 ,
    opticalDiskROM = 5 ,
    opticalDiskWORM = 6 , /* Write once Read Many */
    opticalDiskRW = 7 ,
    ramDisk = 8
} INThrDiskStorageMedia ;
/*
 *    INTSWType ::= INTEGER 
 */
typedef enum
{
    unknown2 = 1 ,
    operatingSystem2 = 2 ,
    deviceDriver2 = 3 ,
    application2 = 4
} INTSWType ;
/*
 *    INThrSWRunStatus ::= INTEGER 
 */
typedef enum
{
    running = 2 ,
    runnable = 2 , /* waiting for resource (CPU, memory, IO) */
    notRunnable = 3 , /* loaded but waiting for event */
    invalid = 4 /* not loaded */
} INThrSWRunStatus ;
/*
 *    DateAndTime ::= OCTET STRING (SIZE ( 8 | 11))  A date-time specification 
 *    for the local time of day.  This data type is intended toprovide a consistent method of  reporting 
 *    dat
 */
typedef OctetString DateAndTime ;
/*
 *    InternationalDisplayString ::= OCTET STRING This data type is used to 
 *    model textual information in some character set.  A network management station should use a local 
 *    algo
 */
typedef OctetString InternationalDisplayString ;
/*
 *    ProductID ::= OBJECT IDENTIFIER This textual convention is intended to 
 *    identify the manufacturer, model, and version of a specific hardware or software 
 *    product.
 */
typedef ObjectIdentifier ProductID ;
#endif /*_HOSTMSMI_H_*/

