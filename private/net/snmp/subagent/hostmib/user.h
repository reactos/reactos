/*
 *  user.h v0.10
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
 *    Windows NT SNMP Extension Agent
 *
 *  Abstract:
 *
 *    This module is a spane holder for user definitions.
 *
 *  Author:
 *
 *    D. D. Burns @ WebEnable, Inc.
 *
 *
 *  Revision History:
 *
 *    V1.0 - 04/16/97  D. D. Burns     Original Creation
 */

#ifndef user_h
#define user_h

/*
| USER.C - Function Prototypes
*/

/* Spt_GetProcessCount - Retrieve count of number of active processes */
ULONG
Spt_GetProcessCount(
                    void
                    );

/* PartitionTypeToLastArc - Convert Partition Type to Last OID Arc value */
ULONG
PartitionTypeToLastArc (
                        BYTE p_type
                        );                      /* Located in "HRFSENTR.C" */

#endif /* user_h */
