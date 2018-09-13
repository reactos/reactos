/*
 *  mib_xtrn.h v0.10
 *  Generated in conjunction with Management Factory scripts:
 *      script version: SNMPv1, 0.16, Apr 25, 1996
 *      project:        D:\TEMP\EXAMPLE\HOSTMIB
 *
 ****************************************************************************
 *                                                                          *
 *      (C) Copyright 1994 DIGITAL EQUIPMENT CORPORATION                    *
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
 *    Windows NT SNMP Extension Agent
 *
 *  Abstract:
 *  
 *    This module contains the external declarations for the MIB.
 *
 *  Author:
 *
 *	David Burns @ Webenable Inc
 *
 *  Date:
 *
 *	Thu Nov 07 16:38:28 1996
 *
 *  Revision History:
 *
 */

#ifndef mib_xtrn_h
#define mib_xtrn_h

// Necessary includes.

#include <snmp.h>
#include "gennt.h"
#include "hostmsmi.h"

//
// External declarations for the table
//
//extern table declarations

extern class_t class_info[] ;
extern trap_t trap_info[] ;
extern UINT number_of_traps ;
extern char *EventLogString ;

#endif /* mib_xtrn_h */
