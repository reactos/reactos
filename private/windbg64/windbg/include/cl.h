/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Cl.h

Abstract:

    Call Stack Walking and display routines.

Author:

    David J. Gilman (davegi) 04-May-1992

Environment:

    Win32, User Mode

--*/

#if ! defined( _CL_ )
#define _CL_

#define ifmeMax 100

typedef enum {
    cltNone,
    cltPub,
    cltProc,
    cltBlk
} CLT;

typedef struct _FME {
    HSYM    symbol;
    char      clt;
    ADDR      addrProc;    /* start of procedure, or block */
    ADDR      addrRet;
    ADDR      addrCSIP;    /* current return location */
    HFRAME    hFrame;
    HMOD      module;      /* pointer to module */
    ULONG     ulParams[4];    /* first 3 words off the stack   */
    BOOL      fFar;        /* Far call  */
} FME; // FraMe Entry

typedef struct CIS {
    uint    cEntries;
    FME     frame [ ifmeMax+1 ];
} CIS;

/*
**  EXTERNS
*/

extern CIS G_cisCallsInfo;

/*
**  PROTOTYPES
*/

void CLGetWalkbackStack(LPPD lppd, LPTD lptd);

char*
CLGetProcName(
    int num,
    char* pch,
    int cbMax,
    BOOL bSpecial
    );

PCXF
CLGetFuncCXF(
    PADDR paddr,
    PCXF pcxf
    );

#endif // _CL_
