/**************************************************************************\
* Module Name: dipscb.tpl
*
* Template C file for server dispatch generation.
*
* Copyright (c) Microsoft Corp. 1990 All Rights Reserved
*
* Created: 10-Dec-90
*
* History:
* 10-Dec-90 created by SMeans
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define __fnINWPARAMCHAR __fnDWORD
#ifdef FE_SB
/*
 * fnGETDBCSTEXTLENGTHS uses same code as fnGETTEXTLENGTHS for
 * sender/receiver of forward to kernel and receiver of callback
 * to client. Only sender of callback to client uses different code.
 * (see inc\ntcb.h SfnGETDBCSTEXTLENGTHS)
 */
#define __fnGETDBCSTEXTLENGTHS __fnGETTEXTLENGTHS
/*
 * fnEMGETSEL, __fnSETSEL, __fnGBGETEDITSEL
 */
#define __fnEMGETSEL           __fnOPTOUTLPDWORDOPTOUTLPDWORD
#define __fnEMSETSEL           __fnDWORD
#define __fnCBGETEDITSEL       __fnOPTOUTLPDWORDOPTOUTLPDWORD
#endif // FE_SB

typedef DWORD (*PNT_CALLBACK_ROUTINE)(
    IN PCAPTUREBUF CallbackMsg
    );

DWORD __%%FOR_ALL%%(PCAPTUREBUF CallbackMsg);

CONST PNT_CALLBACK_ROUTINE apfnDispatch[] = {
    __%%FOR_ALL_BUT_LAST%%,
    __%%FOR_LAST%%
};

#if DBG

PCSZ apszDispatchNames[] = {
    "%%FOR_ALL_BUT_LAST%%",
    "%%FOR_LAST%%"
};

CONST ULONG ulMaxApiIndex = sizeof(apfnDispatch) / sizeof(PCSR_CALLBACK_ROUTINE);

#endif

