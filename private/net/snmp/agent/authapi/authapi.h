/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    authapi.h

Abstract:

    Communications message decode/encode routines.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
#ifndef authapi_h
#define authapi_h

//--------------------------- PUBLIC CONSTANTS ------------------------------

#define ASN_RFCxxxx_PRIVDATA (ASN_CONTEXTSPECIFIC | ASN_PRIMATIVE | 0x01)

#define ASN_RFCxxxx_SNMPMGMTCOM (ASN_CONTEXTSPECIFIC | ASN_CONSTRUCTOR | 0x01)
#define ASN_RFCxxxx_SNMPAUTHMSG (ASN_CONTEXTSPECIFIC | ASN_CONSTRUCTOR | 0x01)
#define ASN_RFCxxxx_SNMPPRIVMSG (ASN_CONTEXTSPECIFIC | ASN_CONSTRUCTOR | 0x01)

//--------------------------- PUBLIC STRUCTS --------------------------------

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

//--------------------------- PUBLIC PROTOTYPES -----------------------------

//------------------------------- END ---------------------------------------

#endif /* authapi_h */

