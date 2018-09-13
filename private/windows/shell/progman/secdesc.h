/****************************** Module Header ******************************\
* Module Name: secdesc.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Defines apis and types used to implement security descriptor helper routines
*
* History:
* 02-06-92 Davidc       Created.
\***************************************************************************/

//
// Types used by security descriptor helper routines
//

typedef LONG    ACEINDEX;
typedef ACEINDEX *PACEINDEX;

typedef struct _MYACE {
    PSID    Sid;
    ACCESS_MASK AccessMask;
    UCHAR   InheritFlags;
} MYACE;
typedef MYACE *PMYACE;


//
// Exported function prototypes
//

PSECURITY_DESCRIPTOR
CreateSecurityDescriptor(
    PMYACE  MyAce,
    ACEINDEX AceCount
    );

BOOL
DeleteSecurityDescriptor(
    PSECURITY_DESCRIPTOR SecurityDescriptor
    );

