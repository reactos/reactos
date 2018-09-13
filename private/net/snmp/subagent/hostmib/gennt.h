/*
 *  gennt.h v0.15   March 21, 1996
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
 *    Agent.
 *
 *    This module contains the definitions for the table driven SNMP dlls.
 *
 *  Author:
 *     Miriam Amos Nihart, Kathy Faust
 *
 *  Date:
 *     2/17/95
 *
 *  Revision History:
 *  v0.11   11/15/95  ags  added mib-2 definitions.
 *  v0.12   Feb 14, 1996  ags  changed SMIGet/SetBoolean to SMIGet/SetNSMBoolean
 *  v0.13   Mar 12, 1996  kkf  revised trap_control_block so that VarBindList
 *                        is built prior to queuing to the trap_queue
 *  v0.14   Mar 19, 1996  kkf  fixed numerous trap related bugs
 *  v0.15   Mar 22, 1996  kkf  fixed mib-2 code definition (should be 1 not 2)
 *
 */

#ifndef gennt_h
#define gennt_h

// Necessary includes.

#include <snmp.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>


// MIB function actions.

#define MIB_ACTION_GET         ASN_RFC1157_GETREQUEST
#define MIB_ACTION_SET         ASN_RFC1157_SETREQUEST
#define MIB_ACTION_GETNEXT     ASN_RFC1157_GETNEXTREQUEST

typedef enum
    { NSM_READ_ONLY, NSM_WRITE_ONLY, NSM_READ_WRITE, NSM_NO_ACCESS } access_mode_t ;

typedef enum
{
    NON_TABLE = 0 ,
    TABLE = 1
} table_type_t ;

typedef int (* PF)() ;
typedef void (* PFV)() ;

//
//  The variable structure is used to construct the variable table.
//  Each variable is represented by an entry in the table.  The table
//  driven design indexes into the table to access information specific
//  to the variable, such as its oid, access, get and set routines.
//  "Holes" in the sequence of variables are represented by NULL entries.
//

typedef struct variable
{
    AsnObjectIdentifier *oid ;
    BYTE type ;
    access_mode_t access_mode ;
    PF VarGet ;
    PF VarSet ;
    PF SMIGet ;
    PF SMISet ;
} variable_t ;

//
//  The class structure is used to construct the class_info table.  It
//  is this table that represents the groups composing the mib view of
//  this dll.  The table driven design uses this table in conjuction
//  with the variable tables to process the SNMP requests.
//

typedef struct class
{
    table_type_t table ;
    AsnObjectIdentifier *oid ;
    unsigned long int var_index ;
    unsigned long int min_index ;
    unsigned long int max_index ;
    PF FindInstance ;
    PF FindNextInstance ;
    PF ConvertInstance ;
    PFV FreeInstance ;
    variable_t *variable ;
} class_t ;

//
//  The trap structure is used to construct the trap_info table.  It
//  is this table that represents the trap for this mib view.  The
//  table driven design uses this table to process the trap.  A event
//  is sent to tell the Extendible Agent to call this dll's SnmpExtensionTrap
//  routine to "collect" a trap.  This routine dequeues a trap from the
//  trap queue and then indexes into the trap_info table to fill in the
//  trap information for the trap pdu.
//

typedef struct trap_variable
{
    AsnObjectIdentifier *oid ;
    PF SMIBuild ;
} tvt_t ;

typedef struct trap
{
    AsnObjectIdentifier *oid ;  /* enterprise OID */
    AsnInteger type ;		/* SNMP_GENERICTRAP_ENTERSPECIFIC */
    AsnInteger specific ;	/* trap value */
    UINT number_of_variables ;
    tvt_t *variables ;
} trap_t ;

typedef struct q_hdr
{
    char *lifo_a ;
    char *fifo_a ;
} q_hdr_t ;

typedef struct trap_control_block
{
    q_hdr_t chain_q ;
    AsnObjectIdentifier enterprise ;
    AsnInteger genericTrap ;
    AsnInteger specificTrap ;
    AsnTimeticks timeStamp ;
    RFC1157VarBindList varBindList ;
} tcb_t ;

//
// Definitions of the oid sequence : 1.3.6.1.4.1.36.2 also described as:
//    iso.memberbody.dod.internet.mib.private.dec.ema
//

#define ISO_CODE 1
#define ISO 1
#define ISO_SEQ ISO_CODE
#define ISO_LENGTH 1

#define ORG_CODE 3
#define ORG 3
#define ORG_SEQ ISO_SEQ, ORG_CODE
#define ORG_LENGTH ( ISO_LENGTH + 1 )

#define DOD_CODE 6
#define DOD 6
#define DOD_SEQ ORG_SEQ, DOD_CODE
#define DOD_LENGTH ( ORG_LENGTH + 1 )

#define INTERNET_CODE 1
#define INTERNET 1
#define INTERNET_SEQ DOD_SEQ, INTERNET_CODE
#define INTERNET_LENGTH ( DOD_LENGTH + 1 )

#define DIRECTORY_CODE 1
#define DIRECTORY 1
#define DIRECTORY_SEQ INTERNET_SEQ, DIRECTORY_CODE
#define DIRECTORY_LENGTH ( INTERNET_LENGTH + 1 )

#define MGMT_CODE 2
#define MGMT 2
#define MGMT_SEQ INTERNET_SEQ, MGMT_CODE
#define MGMT_LENGTH ( INTERNET_LENGTH + 1 )

#define EXPERIMENTAL_CODE 3
#define EXPERIMENTAL 3
#define EXPERIMENTAL_SEQ INTERNET_SEQ, EXPERIMENTAL_CODE
#define EXPERIMENTAL_LENGTH ( INTERNET_LENGTH + 1 )

#define PRIVATE_CODE 4
#define PRIVATE 4
#define PRIVATE_SEQ INTERNET_SEQ, PRIVATE_CODE
#define PRIVATE_LENGTH ( INTERNET_LENGTH + 1 )

#define ENTERPRISES_CODE 1
#define ENTERPRISES 1
#define ENTERPRISES_SEQ PRIVATE_SEQ, ENTERPRISES_CODE
#define ENTERPRISES_LENGTH ( PRIVATE_LENGTH + 1 )

#define DEC_CODE 36
#define DEC 36
#define DEC_SEQ ENTERPRISES_SEQ, DEC_CODE
#define DEC_LENGTH ( ENTERPRISES_LENGTH + 1 )

#define EMA_CODE 2
#define EMA 2
#define EMA_SEQ DEC_SEQ, EMA_CODE
#define EMA_LENGTH ( DEC_LENGTH + 1 )

#define MIB_2_CODE 1
#define MIB_2 2
#define MIB_2_SEQ MGMT_SEQ, MIB_2_CODE
#define MIB_2_LENGTH ( MGMT_LENGTH + 1 )


//
//  Macros
//

#define SUCCESS  1
#define FAILURE  0

#define CHECK_VARIABLE( VarBind, cindex, vindex, status )                   \
{                                                                           \
    if ( VarBind->name.idLength > class_info[ cindex ].var_index )          \
    {                                                                       \
        vindex = VarBind->name.ids[ class_info[ cindex ].var_index - 1 ] ;  \
        if ( ( vindex >= class_info[ cindex ].min_index ) &&                \
             ( vindex <= class_info[ cindex ].max_index ) )                 \
            status = SUCCESS ;                                              \
        else                                                                \
            status = FAILURE ;                                              \
    }                                                                       \
    else                                                                    \
        status = FAILURE ;                                                  \
}


#define CHECK_ACCESS( cindex, vindex, PduAction, status )           \
{                                                                   \
    access_mode_t tmp ;                                             \
                                                                    \
    tmp = class_info[ cindex ].variable[ vindex ].access_mode ;     \
    if ( PduAction == MIB_ACTION_SET )                              \
    {                                                               \
        if ( ( tmp == NSM_WRITE_ONLY ) || ( tmp == NSM_READ_WRITE ) )       \
            status = SUCCESS ;                                      \
        else                                                        \
            status = FAILURE ;                                      \
    }                                                               \
    else                                                            \
    {                                                               \
        if ( ( tmp == NSM_READ_ONLY ) || ( tmp == NSM_READ_WRITE ) )        \
            status = SUCCESS ;                                      \
        else                                                        \
            status = FAILURE ;                                      \
    }                                                               \
}

//
//  These macros are used by the trap logic.  There is only one queue
//  used by the generic code - a trap queue.  These macros can be used
//  elsewhere.  The macros assume that the queue structure is the first
//  element in the queue entry structure.
//
#define QUEUE_ENTER( queue_head, entry )             \
{                                                    \
    q_hdr_t *old_chain ;                             \
                                                     \
    old_chain = (q_hdr_t *)(queue_head.lifo_a) ;     \
    entry->chain_q.lifo_a = queue_head.lifo_a ;      \
    queue_head.lifo_a = (char *)entry ;              \
    entry->chain_q.fifo_a = NULL ;                   \
    if ( old_chain == NULL )                         \
        queue_head.fifo_a = (char *)entry ;          \
    else                                             \
        old_chain->fifo_a = (char *)entry ;          \
}

#define QUEUE_REMOVE( queue_head, entry )                  \
{                                                          \
    q_hdr_t *dequeue_chain ;                               \
    q_hdr_t *prev_chain ;                                  \
                                                           \
    dequeue_chain = (q_hdr_t *)(queue_head.fifo_a) ;       \
    entry = (tcb_t *)dequeue_chain ;                       \
    if ( dequeue_chain != NULL )                           \
    {                                                      \
        prev_chain = (q_hdr_t *)(dequeue_chain->fifo_a) ;  \
        queue_head.fifo_a = (char *)prev_chain ;           \
        if ( prev_chain != NULL )                          \
            prev_chain->lifo_a = NULL ;                    \
        else                                               \
            queue_head.lifo_a = NULL ;                     \
    }                                                      \
}

//
// Function Prototypes
//

UINT UserMibInit(
        IN OUT HANDLE *hPollForTrapEvent ) ;

void TrapInit(
        IN OUT HANDLE *hPollForTrapEvent ) ;

UINT ResolveVarBind(
        IN OUT RFC1157VarBind *VarBind , // Variable Binding to resolve
	    IN UINT PduAction ) ;            // Action specified in PDU

UINT FindClass(
        IN RFC1157VarBind *VarBind,      // Variable Binding
        IN OUT UINT *cindex ) ;          // Index into class_info table

UINT ResolveGetNext(
        IN OUT RFC1157VarBind *VarBind,  // Variable Binding
        IN OUT UINT *cindex,             // Class Info table index
        IN OUT UINT *vindex ,            // Variable table index
        OUT AsnObjectIdentifier *instance ) ;

void SetupTrap(
		IN OUT tcb_t **entryBlock ,
		IN int trapIndex ) ;

UINT AddTrapVar(
		IN tcb_t *entry ,
		IN int trapIndex ,
		IN int varIndex ,
		IN AsnObjectIdentifier *instance ,
		IN char *value ) ;

void PostTrap(
		IN tcb_t *entry ,
		IN int trapIndex ) ;

//
// Externals
//

extern UINT SMIGetInteger() ;
extern UINT SMIGetNSMBoolean() ;
extern UINT SMIGetBIDTEnum() ;
extern UINT SMIGetOctetString() ;
extern UINT SMIGetObjectId() ;
extern UINT SMIGetCounter() ;
extern UINT SMIGetGauge() ;
extern UINT SMIGetTimeTicks() ;
extern UINT SMIGetIpAddress() ;
extern UINT SMIGetDispString() ;
extern UINT SMISetInteger() ;
extern UINT SMISetBIDTEnum() ;
extern UINT SMISetOctetString() ;
extern UINT SMISetObjectId() ;
extern UINT SMISetCounter() ;
extern UINT SMISetGauge() ;
extern UINT SMISetTimeTicks() ;
extern UINT SMISetIpAddress() ;
extern UINT SMISetDispString() ;
extern UINT SMIBuildInteger() ;
extern UINT SMISetNSMBoolean() ;
extern UINT SMIBuildBIDTEnum() ;
extern UINT SMIBuildOctetString() ;
extern UINT SMIBuildObjectId() ;
extern UINT SMIBuildCounter() ;
extern UINT SMIBuildGauge() ;
extern UINT SMIBuildTimeTicks() ;
extern UINT SMIBuildIpAddress() ;
extern UINT SMIBuildDispString() ;
extern void SMIFree() ;

extern q_hdr_t trap_q ;
extern HANDLE hEnabledTraps ;
extern HANDLE hTrapQMutex ;

// Microsoft MIB Specifics.

#define MAX_STRING_LEN            255

#endif /* gennt_h */


