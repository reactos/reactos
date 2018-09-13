/* smint.h  v0.10
// Copyright (C) 1992-1995, All Rights Reserved, by
// Digital Equipment Corporation, Maynard, Mass.
//
// This software is furnished under a license and may be used and copied
// only  in  accordance  with  the  terms  of such  license and with the
// inclusion of the above copyright notice. This software or  any  other
// copies thereof may not be provided or otherwise made available to any
// other person. No title to and ownership of  the  software  is  hereby
// transferred.
//
// The information in this software is subject to change without  notice
// and  should  not be  construed  as  a commitment by Digital Equipment
// Corporation.
//
// Digital assumes no responsibility for the use or  reliability  of its
// software on equipment which is not supplied by Digital.
*/

/*
 *  Facility:
 *
 *    SNMP Extension Agent
 *
 *  Abstract:
 *  
 *    This module contains the native data type definitions.  This file
 *    is taken from the Calaveras Project's system management work done by
 *    Wayne Duso.  By using these native data types (Structure of Management
 *    Information (SMI)) it is intended to align the management APIs on
 *    the Windows NT and UNIX platforms as closely as possible.
 */


#if !defined(_SMINT_H_)
#define _SMINT_H_ 

#if !defined(lint) && defined(INCLUDE_ALL_RCSID)
static char smitydef_h_rcsid[] = "$Header: /calsrv/usr/cal10/duso/calaveras/RCS/smint.h,v 1.6 1993/08/13 15:29:49 duso Exp $";
#endif



/*
// FACILITY:
//
// Calaveras System Management
//
// ABSTRACT:
//
// Structure Of Management Information (SMI) support. Specifically, type 
// declarations targeted for Management Agent use.
//
// AUTHORS:
//
// Wayne W. Duso
//
// CREATION DATE:
// 
// 25-November-1992 
//
// MODIFICATION HISTORY:
//
// $Log: smitydef.h,v $
 * Revision 1.7  1994/03/23  14:02:00  miriam
 * Modifications for Windows NT platform
 *
 * Revision 1.6  1993/08/13  15:29:49  duso
 * Housekeeping.
 *
 * Revision 1.5  1993/03/11  21:26:51  duso
 * Modify interface types to reflect CA X1.1.1 changes.
 *
 * Revision 1.4  1993/03/10  17:36:26  duso
 * Fix free bug resulting from using MOSS to create opaque structures and not
 * using MOSS to free said structures. Manipulation of MOSS opaque structures
 * now done using MOSS supplied routines exclusively. In the spirit of
 * providing opaque types with a 'complete' API, a create() operations has been
 * added to each SMI supported type.
 *
 * Revision 1.3  1993/02/22  18:12:58  duso
 * Support for all SNMP SMI types now in place. Also exclusive copy passed from
 * avlToLocal; not reference copy. This 'inefficiency' was needed to support
 * structure types (length, value). Said types could not be mutated from an
 * avl octet to their true form using references.
 *
 * Revision 1.2  1993/01/28  21:45:24  duso
 * Major clean up and completion to support SNMP centered MOMStub and class
 * Simple test MOC.
*/



/*
// TABLE OF CONTENTS
//
//  Associated Documents
//
//  Usage/Design Notes
//
//  Include Files
//
//  SMI Supported Types (typedef(s))
//      BIDT_ENUMERATION
//      ConstructionStart
//      ConstructionEnd
//      Counter
//      Gauge
//      Integer
//      IpAddress
//      Null
//      ObjectIdentifier
//      OctetString
//      Opaque
//      TimeTicks
*/


/*
// Associated Documents
//
// [1]  Calaveras Managed Object Module Framework Design Specification
// [2]  smidbty.h: Declares the pseudo abstract base psuedo class DtEntry and 
//      its API.
// [3]  smitypes.h: Declares actual instances of DtEntry, one for each typed
//      delcared in this file.
*/          


/*
// Usage/Design Notes
//
//  1.  Each type declared in this file has an accompanying API. The APIs
//      are declared in [2,3]. It is strongly recommended that type instances
//      be accessed through their API, not through direct manipulation of 
//      their internal format [NB: Additional operations must be added to the
//      API set to make the type's concrete/indiginous-like. Specifically,
//      there currently is no 'compare' interface; this must/will be corrected].
//   
//  2.  For each SMI type supported - those being any type defined by either 
//      RFC 1155 or by additions to said RFC accepted by the IETF - there is a 
//      typedef defining its 'in-memory' or 'local' representation. This 
//      information is made available to the SMI type database, the managed 
//      object module stub, and managed object classes (management agents) 
//      through this module.
*/   


/*
// Include Files
*/

#include <snmp.h>


 

/*
// SMI Supported Types (typedef(s))
*/
typedef enum
{
    nsm_true = 1 ,
    nsm_false = 2
} NSM_Boolean ;


typedef int BIDT_ENUMERATION;
/*  Signed 32 bit integer is the base data construct used by the MIR to 
//  represent integer enumerations.
*/


typedef unsigned long int Counter;
/*  Unsigned 32 bit integer is the base data construct.
*/


typedef unsigned long int Gauge;
/*  Unsigned 32 bit integer is the base data construct.
*/


typedef int Integer;
/*  Signed 32 bit integer is the base data construct.
*/


typedef unsigned long int IpAddress;
/*  Unsigned 32 bit integer is the base data construct. Interpret as
//  a 4 octet hex value.
*/


typedef char Null;
/*  The semantics of Null are that it should always be a 0 constant.
*/


typedef AsnObjectIdentifier ObjectIdentifier;
/*  
//  Aggregate type: Note that this is typedef to the Windows NT's version
//  of an object identifier to enable use with the Windows NT SNMP routines.
//  The Common Agent's MOSS library is not available for use on Windows NT.
//
//  The fields for the object identifier are:
//	UINT idLength ;		// number of integers in the oid's int array
//	UINT *ids ;		// address of the oid's int array
*/


typedef struct
/*  
//  This one is used to minimize differences across platforms and because
//  Windows NT has no support routines for manipulating an octet string.
//  Should that change consideration should be given to using the native
//  for as the Common Agent is not available on Windows NT.
//  Aggregate type: Note that this is structured exactly as the CA MOSS 
//  definition octet_string found in moss.h. As such, it should either be
//  removed in favor of that definition or its synchronization must be 
//  ensured.
*/
{
    int length;                 /* length of string */
    unsigned long int dataType; /* ASN.1 data type tag (currently optional)*/
    char* string;               /* pointer to counted string */
} OctetString;


typedef struct
/*  
//  Aggregate type
*/
{
    int length;     /* length of string */
    char* string;   /* pointer to counted entity */
} Opaque;


typedef unsigned long int TimeTicks;
/*  Unsigned 32 bit integer is the base data construct.
*/

typedef OctetString Simple_DisplayString ;
/* Included here as oppose to in the simpleema.hxx file on Calaveras to
// reduce the number of header files.
*/

typedef int Access_Credential ;
/* This is a dummy place holder for use in the future
*/

typedef struct 
{
    unsigned int count ;  /* number of identifying variables for an instance */
    char **array ;        /* array of pointers to variables' data */
} InstanceName ;
/* This is a flexible structure for passing the ordered native datatypes
// that compose the instance name.  For example,
//    tcpConnEntry from RFC 1213 p. 49 (MIB-II) is identified by
//
//	INDEX {
//		tcpConnLocalAddress,
//		tcpConnLocalPort,
//		tcpConnRemAddress,
//		tcpConnRemPort
//	      }
//
// The instance name would be an ordered set :
//
//	count = 4
//	array[ 0 ] = address of an IP Address (the local ip address)
//	array[ 1 ] = address of an Integer (the local port)
//	array[ 2 ] = address of an IP Address (the remote ip address)
//	array[ 3 ] = address of an Integer (the remote port)
*/

#define MAX_OCTET_STRING 256

UINT
SMIGetInteger( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for get
               IN unsigned long int cindex ,
               IN unsigned long int vindex ,
               IN InstanceName *instance );
UINT
SMIGetNSMBoolean( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for get
               IN unsigned long int cindex ,
               IN unsigned long int vindex ,
               IN InstanceName *instance );
UINT
SMIGetBIDTEnum( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for get
                IN unsigned long int cindex ,
                IN unsigned long int vindex ,
                IN InstanceName *instance );
UINT
SMIGetOctetString( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for get
                   IN unsigned long int cindex ,
                   IN unsigned long int vindex ,
                   IN InstanceName *instance );
UINT
SMIGetObjectId( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for get
                IN unsigned long int cindex ,
                IN unsigned long int vindex ,
                IN InstanceName *instance );
UINT
SMIGetCounter( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for get
               IN unsigned long int cindex ,
               IN unsigned long int vindex ,
               IN InstanceName *instance );
UINT
SMIGetGauge( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for get
             IN unsigned long int cindex ,
             IN unsigned long int vindex ,
             IN InstanceName *instance );
UINT
SMIGetTimeTicks( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for get
                 IN unsigned long int cindex ,
                 IN unsigned long int vindex ,
                 IN InstanceName *instance );
UINT
SMIGetIpAddress( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for get
                 IN unsigned long int cindex ,
                 IN unsigned long int vindex ,
                 IN InstanceName *instance );
UINT
SMIGetDispString( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for get
                  IN unsigned long int cindex ,
                  IN unsigned long int vindex ,
                  IN InstanceName *instance );
UINT
SMISetInteger( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for set
               IN unsigned long int cindex ,
               IN unsigned long int vindex ,
               IN InstanceName *instance );
UINT
SMISetNSMBoolean( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for set
               IN unsigned long int cindex ,
               IN unsigned long int vindex ,
               IN InstanceName *instance );
UINT
SMISetBIDTEnum( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for set
               IN unsigned long int cindex ,
               IN unsigned long int vindex ,
               IN InstanceName *instance );
UINT
SMISetOctetString( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for set
                   IN unsigned long int cindex ,
                   IN unsigned long int vindex ,
                   IN InstanceName *instance );
UINT
SMISetObjectId( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for set
                IN unsigned long int cindex ,
                IN unsigned long int vindex ,
                IN InstanceName *instance );
UINT
SMISetCounter( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for set
               IN unsigned long int cindex ,
               IN unsigned long int vindex ,
               IN InstanceName *instance );
UINT
SMISetGauge( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for set
             IN unsigned long int cindex ,
             IN unsigned long int vindex ,
             IN InstanceName *instance );
UINT
SMISetTimeTicks( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for set
                 IN unsigned long int cindex ,
                 IN unsigned long int vindex ,
                 IN InstanceName *instance );
UINT
SMISetIpAddress( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for set
                 IN unsigned long int cindex ,
                 IN unsigned long int vindex ,
                 IN InstanceName *instance );
UINT
SMISetDispString( IN OUT RFC1157VarBind *VarBind ,  // Variable Binding for set
                  IN unsigned long int cindex ,
                  IN unsigned long int vindex ,
                  IN InstanceName *instance );
UINT
SMIBuildInteger( IN OUT RFC1157VarBind *VarBind ,
                 IN char *invalue );
UINT
SMIBuildOctetString( IN OUT RFC1157VarBind *VarBind ,
                     IN char *invalue );
UINT
SMIBuildObjectId( IN OUT RFC1157VarBind *VarBind ,
                  IN char *invalue );
UINT
SMIBuildCounter( IN OUT RFC1157VarBind *VarBind ,
                 IN char *invalue );
UINT
SMIBuildGauge( IN OUT RFC1157VarBind *VarBind ,
               IN char *invalue );
UINT
SMIBuildTimeTicks( IN OUT RFC1157VarBind *VarBind ,
                   IN char *invalue );
UINT
SMIBuildIpAddress( IN OUT RFC1157VarBind *VarBind ,
                   IN char *invalue );
UINT
SMIBuildDispString( IN OUT RFC1157VarBind *VarBind ,
                    IN char *invalue );
void
SMIFree( IN AsnAny *invalue );

#endif  /*_SMINT_H_*/
