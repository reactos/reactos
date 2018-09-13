/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    xport.c

Abstract:

    This module contains the code for the named pipe transport layer
    which explicitly deals with the machanics of doing named pipes.

Author:

    Jim Schaad  (jimsch) 11-June-93
    Wesley Witt (wesw)   25-Nov-93

Environment:

    Win32 User

--*/

#ifdef WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "tchar.h"

#include "cvinfo.h"

#include "odtypes.h"
#include "od.h"
#include "odp.h"
#include "odassert.h"
#include "emdm.h"

#include "xport.h"


#include "dbgver.h"
extern AVS Avs;

#if DBG
DWORD FExpectingReply = 0;
DWORD FExpectingSeq = 0;
CRITICAL_SECTION csExpecting;
#endif

#ifdef WIN32
LPDMINIT     LpDmInit;
LPDMFUNC     LpDmFunc;
LPDMDLLINIT  LpDmDllInit;
LPUISERVERCB LpUiServer;
void EXPENTRY DMInit (DMTLFUNCTYPE, LPV);
#else
void EXPENTRY DMInit (DMTLFUNCTYPE);
#endif

LPDBF lpdbf = (LPDBF)0;         // the debugger helper functions LOCAL
TLCALLBACKTYPE TLCallBack;      // central osdebug callback function


extern BOOL FDMSide;            // this dll was loaded by the remote wrapper

BOOL FConnected = FALSE;
LPB lpbDM;
WORD ibMaxDM;
WORD ibDM;
HPID hpidRoot;
LPSTR  LpszDm;
HANDLE HDm = NULL;
DWORD pkSeq;
HANDLE hRemoteQuit;

#if DBG
#define SENDREQUESTTIMEOUT 300
#else
#define SENDREQUESTTIMEOUT 20
#endif
DWORD TLFuncSendReplyTimeout = SENDREQUESTTIMEOUT;
DWORD DMTLFuncSendReplyTimeout = SENDREQUESTTIMEOUT;

#define MAX_TL_WAIT_THREADS 20

CRITICAL_SECTION TlpCsThreadList;
HANDLE TlpWaitHandlesList[MAX_TL_WAIT_THREADS];
DWORD TlpWaitHandlesCount;

BOOL
TlpWaitForThreadsToExit(
    void
    );

LPTLIS
TlGetInfo(
    VOID
    );


static char Rgb[MAX_INTERNAL_PACKET + sizeof(NLBLK) + sizeof(MPACKET)];
#define Pnlblk   ((PNLBLK) Rgb)
#define PnlblkDm ((PNLBLK) Rgb)

static char RgbReply[MAX_INTERNAL_PACKET + sizeof(NLBLK) + sizeof(MPACKET)];
#define PnlblkRp ((PNLBLK) RgbReply)

static char RgbRequest[MAX_INTERNAL_PACKET + sizeof(NLBLK) + sizeof(MPACKET)];
#define PnlblkRq ((PNLBLK) RgbRequest)

#define ZERO_Rgb(x) ZeroMemory(##x,MAX_INTERNAL_PACKET + sizeof(NLBLK) + sizeof(MPACKET))

XOSD
SendData(
    LPV    lpvOut,
    int    cbOut
    );

XOSD
SendRequest(
    int         mtypeResponse,
    LPV         lpvOut,
    int         cbOut,
    LPV         lpvReply,
    int *       pcbReply,
    DWORD       dwTimeOut
    );

XOSD EXPENTRY
DMTLFunc(
    TLF   wCommand,
    HPID  hpid,
    LPARAM wParam,
    LPARAM  lParam
    );

XOSD EXPENTRY
TLFunc(
    TLF   wCommand,
    HPID  hpid,
    LPARAM wParam,
    LPARAM  lParam
    );

extern BOOL FDMSide;


BOOL
DllVersionMatch(
    HANDLE hMod,
    LPSTR pType
    )
{
    DBGVERSIONPROC pVerProc;
    LPAVS pavs;

    pVerProc = (DBGVERSIONPROC)GetProcAddress(hMod, DBGVERSIONPROCNAME);
    if (!pVerProc) {
        return(FALSE);          // no version entry point
    } else {
        pavs = (*pVerProc)();
        if ((pType[0] != pavs->rgchType[0] || pType[1] != pavs->rgchType[1]) ||
            (Avs.rlvt != pavs->rlvt) || (Avs.iRmj != pavs->iRmj)) {
            return(FALSE);
        }
    }
    return(TRUE);
}

XOSD
LoadDM(
    HPID hpid,
    LPLOADDMSTRUCT lds
    )
/*++

Routine Description:

    Load the DM on the stub side of the transport.

Arguments:

    lds - Supplies a LOADDMSTRUCT with dll name and parameters

Return Value:

    XOSD status

--*/
{

    XOSD xosd = xosdNone;

    DPRINT(("LoadDm('%s', '%s')\n", lds->lpDmName, lds->lpDmParams));

    if (HDm) {
        DPRINT(("LoadDm: Already loaded, HDm == 0x%p\n", HDm));
        return xosdInUse;
    }

    HDm = LoadLibrary(lds->lpDmName);
    if (HDm == NULL) {
        DPRINT(("LoadDm: LoadLibrary failed: Error:%i\n",GetLastError()));
        xosd = xosdUnknown;
    } else
#if !defined(_M_IA64) //to save on headache presume always good
     if (!DllVersionMatch(HDm, "DM")) {
        DPRINT(("LoadDm: DllVersionMatch failed\n"));
        xosd = xosdBadVersion;
    } else
#endif
    if ((LpDmInit = (LPDMINIT) GetProcAddress(HDm, "DMInit")) == NULL) {
        DPRINT(("LoadDm: GetProcAddress(DMInit) failed\n"));
        xosd = xosdUnknown;
    } else
    if ((LpDmFunc = (LPDMFUNC) GetProcAddress(HDm, "DMFunc")) == NULL) {
        DPRINT(("LoadDm: GetProcAddress(DMFunc) failed\n"));
        xosd = xosdUnknown;
    } else
    if ((LpDmDllInit = (LPDMDLLINIT) GetProcAddress(HDm, "DmDllInit")) == NULL) {
        DPRINT(("LoadDm: GetProcAddress(DmDllInit) failed\n"));
        xosd = xosdUnknown;
    } else
    if (LpDmDllInit(lpdbf) == FALSE) {
        DPRINT(("LoadDm: DmDllInit failed\n"));
        xosd = xosdUnknown;
    }

    if (xosd != xosdNone) {
        if (HDm) {
            FreeLibrary(HDm);
            HDm = NULL;
        }
    }

    if (HDm) {
        LpDmInit((DMTLFUNCTYPE)DMTLFunc, (LPVOID) lds->lpDmParams);
    }

    DPRINT(("LoadDm returning %d\n", xosd));

    return xosd;
}

XOSD
LoadRemoteDM(
    HPID hpid,
    LPLOADDMSTRUCT lds
    )
{
    DWORD cb;
    XOSD xosd;
    LPSTR p;

    ZERO_Rgb(RgbRequest);
    PnlblkRq->mtypeBlk = mtypeLoadDM;

    PnlblkRq->hpid = hpid;
    PnlblkRq->seq = ++pkSeq;

    //memcpy((LPVOID)PnlblkRq->rgchData, (char *) lds, sizeof(LOADDMSTRUCT));
    strcpy((LPSTR)PnlblkRq->rgchData, lds->lpDmName);
    p = (LPSTR)PnlblkRq->rgchData + strlen((LPSTR)PnlblkRq->rgchData) + 1;
    if (lds->lpDmParams) {
        strcpy(p, lds->lpDmParams);
        p += strlen(lds->lpDmParams) + 1;
    } else {
        *p++ = 0;
    }

    PnlblkRq->cchMessage = (short)(p - (LPSTR)(PnlblkRq->rgchData));

    DPRINT(("LoadRemoteDM: sending '%s', '%s'\n",
            lds->lpDmName, lds->lpDmParams));

    cb = ibMaxDM;
    xosd = SendRequest(mtypeLoadDMReply,
                       PnlblkRq,
                       PnlblkRq->cchMessage + sizeof(NLBLK),
                       lpbDM,
                       &cb,
                       60
                      );

    if (xosd == xosdNone) {
        xosd = *(XOSD*)lpbDM;
    }
    return xosd;
}

BOOL TlpInitialized;

void
TlpLocalInit(
    void
    )
{
    if (!TlpInitialized) {
        InitializeCriticalSection(&TlpCsThreadList);
        TlpWaitHandlesList[0] = CreateEvent(0, TRUE, FALSE, NULL);
        TlpWaitHandlesCount = 1;
        TlpInitialized = TRUE;
    }
}

void
TlpLocalDestroy(
    void
    )
{
    if (TlpInitialized) {
        DeleteCriticalSection(&TlpCsThreadList);
        CloseHandle(TlpWaitHandlesList[0]);
        TlpWaitHandlesCount = 0;
        TlpInitialized = FALSE;
    }
}

XOSD
EXPENTRY
TLFunc(
    TLF   wCommand,
    HPID  hpid,
    LPARAM wParam,
    LPARAM  lParam
    )
/*++

Routine Description:

    This function contains the dispatch loop for commands comming into
    the transport layer.  The address of this procedure is exported to
    users of the DLL.

Arguments:

    wCommand - Supplies the command to be executed.

    hpid - Supplies the hpid for which the command is to be executed.

    wParam - Supplies information about the command.

    lParam - Supplies information about the command.

Return Value:

    XOSD error code.  xosdNone means that no errors occured.  Other
    error codes are defined in osdebug\include\od.h.

--*/

{
    int        cb;
    LPSTR      p;
    MPACKET *  pMpckt;
    XOSD       xosd = xosdNone;
    LPDBB      lpdbb;
    AVS        dmavs;


    switch ( wCommand ) {
        case tlfInit:
            DEBUG_OUT("TlFunc:  tlfInit\n");
            lpdbf = (LPDBF) wParam;
            TLCallBack = (TLCALLBACKTYPE) lParam;

            TlpLocalInit();
            break;

        case tlfLoadDM:

            assert(!FDMSide);
            //
            // If this is the debugger side, send the data across the wire
            // to the stub side.
            //
            // what happens if the stub already loaded a dm?
            //

            xosd = LoadRemoteDM(hpid, (LPLOADDMSTRUCT)lParam);


            break;


        case tlfDestroy:
            DEBUG_OUT("TlFunc:  tlfDestroy\n");
            if (LpDmInit != NULL) {
                LpDmInit(NULL, NULL);
            }
            if (HDm != NULL) {
                FreeLibrary(HDm);
                HDm = NULL;
                LpDmInit = NULL;

                LpDmFunc = NULL;
            }
            if (LpUiServer) {
                DEBUG_OUT(("NL: tlcbDisconnect\n"));
                LpUiServer(tlcbDisconnect, 0, 0, 0, 0);
            }

            TlDestroyTransport();

            TlpWaitForThreadsToExit();
            TlpLocalDestroy();
            FConnected = FALSE;

            DEBUG_OUT(("NL: tlfDestroy exit\n"));
			SetEvent(hRemoteQuit);
            break;

        case tlfGetProc:
            DEBUG_OUT("TlFunc:  tlfGetProc\n");
            lParam = (LPARAM)TLFunc;
            break;

        case tlfConnect:
            DEBUG_OUT("TlFunc:  tlfConnect\n");
            if (hpid == NULL) { //initial call - CreateTransport
                TlCreateTransport(NULL);
            } else {
#if defined(_M_IA64)
                hRemoteQuit = (HANDLE)hpid; //server part - this is a hack to get the remote shell to unload when the client has disconnected
                xosd = xosdNone;
#endif
                xosd = TlConnectTransport(); //client - second call - connect transport
            }
            break;

        case tlfDisconnect:
            DEBUG_OUT("TlFunc:  tlfDisconnect\n");
            if (FConnected) {
                    
                Pnlblk->mtypeBlk = mtypeDisconnect;
                xosd = SendData(Pnlblk, sizeof(NLBLK));
                TlDisconnectTransport();
                FConnected = FALSE;
            }
            break;

        case tlfRemoteQuit:
            DEBUG_OUT("TlFunc:  tlfRemoteQuit\n");

            //
            // tell the dm that it is disconnected from the debugger
            //
            //
            // This can happen before the DM is initialized...
            //
            if (LpDmFunc) {
                lpdbb = (LPDBB)Rgb;
                lpdbb->dmf = dmfRemoteQuit;
                lpdbb->hpid = 0;
                lpdbb->htid = 0;
                LpDmFunc(sizeof(*lpdbb), (char *) lpdbb);
            }

            TlDisconnectTransport();
            break;

        case tlfSetBuffer:
            DEBUG_OUT("TlFunc:  tlfSetBuffer\n");
            lpbDM = (LPB) lParam;
            ibMaxDM = (short)wParam;
            break;

        case tlfDebugPacket:
            DEBUG_OUT("TlFunc:  tlfDebugPacket\n");
            if (!FConnected) {
                xosd = xosdLineNotConnected;
            } else if (wParam > MAX_INTERNAL_PACKET) {
                ZERO_Rgb(Rgb);
                pMpckt = (MPACKET * ) Pnlblk->rgchData;
                Pnlblk->mtypeBlk = mtypeAsyncMulti;
                Pnlblk->hpid = hpid;
                Pnlblk->cchMessage = MAX_INTERNAL_PACKET + sizeof(MPACKET);
                Pnlblk->seq = ++pkSeq;
                pMpckt->packetNum = 0;

                pMpckt->packetCount = (((short)wParam + MAX_INTERNAL_PACKET - 1) /
                                       MAX_INTERNAL_PACKET);
                while (wParam > MAX_INTERNAL_PACKET) {
                    memcpy((LPVOID)pMpckt->rgchData, (LPB) lParam, MAX_INTERNAL_PACKET);
                    xosd = SendData(Pnlblk, sizeof(NLBLK) + sizeof(MPACKET) +
                                    MAX_INTERNAL_PACKET);
                    if (xosd != xosdNone) {
                        return xosdUnknown;
                    }
                    wParam -= MAX_INTERNAL_PACKET;
                    lParam += MAX_INTERNAL_PACKET;

                    pMpckt->packetNum += 1;
                }

                memcpy((LPVOID)pMpckt->rgchData, (LPB) lParam, (UINT)wParam);

                Pnlblk->cchMessage = (short)(wParam + sizeof(MPACKET));

                xosd = SendData(Pnlblk, sizeof(NLBLK) + sizeof(MPACKET) + (UINT)wParam);
            } else {
                ZERO_Rgb(Rgb);
                Pnlblk->mtypeBlk = mtypeAsync;

                Pnlblk->hpid = hpid;
                Pnlblk->cchMessage = (short)wParam;
                Pnlblk->seq = ++pkSeq;

                memcpy((LPVOID)Pnlblk->rgchData, (LPB) lParam, (UINT)wParam);
                xosd = SendData(Pnlblk, sizeof(NLBLK) + (UINT)wParam);
            }
            break;


        case tlfReply:
            DEBUG_OUT("TlFunc:  tlfReply\n");
#if DBG
            EnterCriticalSection(&csExpecting);
            assert(FExpectingReply);
            FExpectingReply = 0;
            LeaveCriticalSection(&csExpecting);
#endif
            if (!FConnected) {
                xosd = xosdLineNotConnected;
            } else if (wParam > MAX_INTERNAL_PACKET) {
                pMpckt = (MPACKET * ) PnlblkRp->rgchData;
                ZERO_Rgb(RgbReply);
                PnlblkRp->mtypeBlk = mtypeReplyMulti;
                PnlblkRp->hpid = hpid;
                PnlblkRp->cchMessage = MAX_INTERNAL_PACKET + sizeof(MPACKET);
                PnlblkRp->seq = ++pkSeq;
                pMpckt->packetNum = 0;

                pMpckt->packetCount = (((short)wParam + MAX_INTERNAL_PACKET - 1) /
                                       MAX_INTERNAL_PACKET);
                while (wParam > MAX_INTERNAL_PACKET) {
                    memcpy((LPVOID)pMpckt->rgchData, (LPB) lParam, MAX_INTERNAL_PACKET);
                    xosd = SendData(PnlblkRp, sizeof(NLBLK) + sizeof(MPACKET) +
                                    MAX_INTERNAL_PACKET);
                    if (xosd != xosdNone) {
                        return xosdUnknown;
                    }
                    wParam -= MAX_INTERNAL_PACKET;
                    lParam += MAX_INTERNAL_PACKET;

                    pMpckt->packetNum += 1;
                }

                memcpy((LPVOID)pMpckt->rgchData, (LPB) lParam, (UINT)wParam);

                PnlblkRp->cchMessage = (short)(wParam + sizeof(MPACKET));

                xosd = SendData(PnlblkRp, sizeof(NLBLK) + sizeof(MPACKET) + (UINT)wParam);
            } else {
                ZERO_Rgb(RgbReply);
                PnlblkRp->mtypeBlk = mtypeReply;

                PnlblkRp->hpid = hpid;
                PnlblkRp->cchMessage = (short)wParam;
                PnlblkRp->seq = ++pkSeq;

                memcpy((LPVOID)PnlblkRp->rgchData, (LPB) lParam, (UINT)wParam);
                xosd = SendData(PnlblkRp, sizeof(NLBLK) + (UINT)wParam);
            }
            break;

        case tlfRequest:
            DEBUG_OUT("TlFunc:  tlfRequest\n");
            if ( !FConnected ) {
                xosd = xosdLineNotConnected;
            } else {
                cb = ibMaxDM;
                if (wParam > MAX_INTERNAL_PACKET) {
                    ZERO_Rgb(RgbRequest);
                    pMpckt = (MPACKET * ) PnlblkRq->rgchData;
                    PnlblkRq->mtypeBlk = mtypeSyncMulti;
                    PnlblkRq->hpid = hpid;
                    PnlblkRq->cchMessage = MAX_INTERNAL_PACKET + sizeof(MPACKET);
                    PnlblkRq->seq = ++pkSeq;
                    pMpckt->packetNum = 0;

                    pMpckt->packetCount = (((short)wParam + MAX_INTERNAL_PACKET - 1) /
                                           MAX_INTERNAL_PACKET);
                    while (wParam > MAX_INTERNAL_PACKET) {
                        memcpy((LPVOID)pMpckt->rgchData, (LPB) lParam, MAX_INTERNAL_PACKET);
                        xosd = SendData(PnlblkRq, sizeof(NLBLK) + sizeof(MPACKET) +
                                        MAX_INTERNAL_PACKET);
                        if (xosd != xosdNone) {
                            return xosdUnknown;
                        }
                        wParam -= MAX_INTERNAL_PACKET;
                        lParam += MAX_INTERNAL_PACKET;

                        pMpckt->packetNum += 1;
                    }

                    PnlblkRq->cchMessage = (short)(wParam + sizeof(MPACKET));

                    memcpy((LPVOID)pMpckt->rgchData, (LPB) lParam, (UINT)wParam);
                    xosd = SendRequest(mtypeReply,
                                       PnlblkRq,
                                       sizeof(NLBLK) + sizeof(MPACKET) + (UINT)wParam,
                                       lpbDM,
                                       &cb,
                                       TLFuncSendReplyTimeout
                                       );
                } else {
                    ZERO_Rgb(RgbRequest);
                    PnlblkRq->mtypeBlk = mtypeSync;

                    PnlblkRq->cchMessage = (short)wParam;
                    PnlblkRq->hpid = hpid;
                    PnlblkRq->seq = ++pkSeq;

                    memcpy((LPVOID)PnlblkRq->rgchData, (LPVOID) lParam, (UINT)wParam);
                    xosd = SendRequest(mtypeReply,
                                       PnlblkRq,
                                       (UINT)wParam + sizeof(NLBLK),
                                       lpbDM,
                                       &cb,
                                       TLFuncSendReplyTimeout
                                       );
                }
            }
            break;

        case tlfGetVersion:
            DEBUG_OUT("TlFunc:  tlfGetVersion\n");
            {
                int cb = (UINT)wParam;
                //
                // Get the version information from the remote side.  If it doesn't
                // return anything in 10 seconds, time out and return 0's.
                //
                // lParam = buffer to fill in
                // wParam = size of buffer
                //
                // sets globals lpchVersionReply = lParam
                // cchVersionReplyMax = wParam
                //
                //

                DEBUG_OUT("NL: tlfGetVersion\n");
                if (!FConnected) {
                    DEBUG_OUT(("NL: tlfGetVersion not on line\n"));
                    xosd = xosdLineNotConnected;
                    break;
                }

                //
                // must be connected
                // basically works like a tlfRequest with a timeout and a
                // target in the transport rather than the DM/EM.
                //

                //
                // Create the desired packet.  The packet consists of:
                // type = mtypeVersionRequest
                // length = 0
                // hpid = current pid
                //

                ZERO_Rgb(Rgb);
                Pnlblk->mtypeBlk = mtypeVersionRequest;

                Pnlblk->cchMessage = 0;
                Pnlblk->hpid = hpid;
                Pnlblk->seq = ++pkSeq;

                //
                // send the version request packet
                //

				Sleep(100); // v-vadimp - on initial call the transport gets stuck somewhere in NT, waiting for something?
							 // pausing here a little helps. this needs to be investigated
                xosd = SendRequest(mtypeVersionReply, Pnlblk,
                                   sizeof(NLBLK), (void *) lParam, &cb, 15);
                if (xosd == xosdNone) {
                } else {
                    memset((LPCH) lParam, 0, (UINT)wParam);
                }
                DEBUG_OUT("NL: tlfVersionCheck exit\n");
            }
            break;

        case tlfSendVersion:
            DEBUG_OUT("TlFunc:  tlfSendVersion\n");
            if (!FConnected) {
                DPRINT(("tlfSendVersion - Line Not Connected\n"));
                return xosdLineNotConnected;
            } else {
                // Send the version information to
                // the other side.  This is in response to a mtypeVersionRequest
                // (tlfGetVersion)

                // Create a packet with the appropriate data packet
                // type = mtypeVersionReply cb = sizeof(Avs);
                // hpid = hpid
                // data = Version structure

                if (HDm) {
                    DBGVERSIONPROC verproc =
                       (DBGVERSIONPROC)GetProcAddress(HDm, DBGVERSIONPROCNAME);
                    dmavs = *verproc();
                } else {
                    dmavs = Avs;
                    DPRINT(("Version %i,%i\n", Avs.iRmj, Avs.iRmm));
                }

                ZERO_Rgb(Rgb);
                Pnlblk->mtypeBlk = mtypeVersionReply;
                Pnlblk->hpid = hpid;
                Pnlblk->cchMessage = sizeof(AVS);
                Pnlblk->seq = ++pkSeq;
                memcpy((LPVOID)Pnlblk->rgchData,(LPCH)&dmavs, sizeof(AVS));
                SendData(Pnlblk, sizeof(NLBLK) + sizeof(AVS));

                DEBUG_OUT(("Send Reply to version request packet\n"));
            }
            break;

        case tlfGetInfo:
            DEBUG_OUT("TlFunc:  tlfGetInfo\n");
            memcpy((LPVOID)lParam, TlGetInfo(), sizeof(TLIS));
            break;

        case tlfSetup:
            DEBUG_OUT("TlFunc:  tlfSetup\n");
            {
                LPTLSS lptlss = (LPTLSS)lParam;
                //CHAR String[1024];
                //DWORD Size;
                //DWORD Type;

                // struct _TLSS {
                // DWORD fLoad;
                // DWORD fInteractive;
                // DWORD fSave;
                // LPVOID lpvPrivate;
                // LPARAM lParam;
                // LPGETSETPROFILEPROC lpfnGetSet;
                // MPT mpt;
                // BOOL fRMAttached;
                // }
                //
                // typedef LONG (OSDAPI * LPGETSETPROFILEPROC)(
                //      LPTSTR          KeyName,   // SubKey name (must be relative)
                //      LPTSTR          ValueName, // value name
                //      DWORD*          dwType,    // type of data (valid only in the case of Set)
                //      BYTE*           Data,      // pointer to data
                //      DWORD           cbData,    // size of data (in bytes)
                //      BOOL            fSet,      // TRUE = setting, FALSE = getting
                //      LPARAM          lParam     // Instance data from shell
                //     );

                if (lptlss->fRMAttached) {
                    TlSetRemoteStatus(lptlss->mpt);
                }

                TLSetup(lptlss);

                if (lptlss->fLoad) {
                    //Type = REG_SZ;
                    //Size = sizeof(String);
                    //if (lpemss->lpfnGetSet("TLREMOTE",
                    //                       "SomeValue",
                    //                       &Type,
                    //                       (PUCHAR)String,
                    //                       &Size,
                    //                       FALSE,
                    //                       lptlss->lParam
                    //                       )) {
                    //}

                }

                if (lptlss->fInteractive) {
                }

                if (lptlss->fSave) {
                    //lptlss->lpfnGetSet("TLREMOTE",
                    //                   "SomeValue",
                    //                   0,
                    //                   String,
                    //                   0,
                    //                   TRUE,
                    //                   lptlss->lParam
                    //                   );
                }

            }
            break;

        case tlfSetErrorCB:
            DEBUG_OUT("TlFunc:  tlfSetErrorCB\n");
            LpUiServer = (LPUISERVERCB) lParam;
            break;

        default:
            DEBUG_OUT("TlFunc:  **** unknown tlf ****\n");
            //assert ( FALSE );
            break;
    }

    return xosd;
}


XOSD EXPENTRY
DMTLFunc(
    TLF   wCommand,
    HPID  hpid,
    LPARAM wParam,
    LPARAM  lParam
    )
{
    XOSD xosd = xosdNone;
    int cb;
    MPACKET * pMpckt;

    switch ( wCommand ) {
        case tlfInit:
            DEBUG_OUT( "DMTlFunc:  tlfInit\n" );
            break;

        case tlfDestroy:
            DEBUG_OUT( "DMTlFunc:  tlfDestroy\n" );
            break;

        case tlfConnect:
            DEBUG_OUT( "DMTlFunc:  tlfConnect\n" );
            break;

        case tlfDisconnect:
            DEBUG_OUT( "DMTlFunc:  tlfDisconnect\n" );
            TlDestroyTransport();
            FConnected = FALSE;
            break;

        case tlfSetBuffer: lpbDM = (LPB) lParam;
            DEBUG_OUT( "DMTlFunc:  tlfSetBuffer\n" );
            ibMaxDM = (short)wParam;
            break;

        case tlfDebugPacket:
            DEBUG_OUT( "DMTlFunc:  tlfDebugPacket\n" );
            if (!FConnected) {
                xosd = xosdLineNotConnected;
            } else if (wParam > MAX_INTERNAL_PACKET) {
                pMpckt = (MPACKET * ) PnlblkDm->rgchData;
                PnlblkDm->mtypeBlk = mtypeAsyncMulti;
                PnlblkDm->hpid = hpid;
                PnlblkDm->cchMessage = MAX_INTERNAL_PACKET + sizeof(MPACKET);
                PnlblkDm->seq = ++pkSeq;
                pMpckt->packetNum = 0;

                pMpckt->packetCount = (((short)wParam + MAX_INTERNAL_PACKET - 1) /
                                       MAX_INTERNAL_PACKET);
                while (wParam > MAX_INTERNAL_PACKET) {
                    memcpy((LPVOID)pMpckt->rgchData, (LPB) lParam, MAX_INTERNAL_PACKET);
                    xosd = SendData(PnlblkDm, sizeof(NLBLK) + sizeof(MPACKET) +
                                    MAX_INTERNAL_PACKET);
                    if (xosd != xosdNone) {
                        return xosdUnknown;
                    }
                    wParam -= MAX_INTERNAL_PACKET;
                    lParam += MAX_INTERNAL_PACKET;

                    pMpckt->packetNum += 1;
                }

                memcpy((LPVOID)pMpckt->rgchData, (LPB) lParam, (DWORD)wParam);

                PnlblkDm->cchMessage = (short)(wParam + sizeof(MPACKET));

                xosd = SendData(PnlblkDm, sizeof(NLBLK) + sizeof(MPACKET) + (DWORD)wParam);
            } else {
                PnlblkDm->mtypeBlk = mtypeAsync;

                PnlblkDm->hpid = hpid;
                PnlblkDm->cchMessage = (short)wParam;
                PnlblkDm->seq = ++pkSeq;

                memcpy((LPVOID)PnlblkDm->rgchData, (LPB) lParam, (DWORD)wParam);
                xosd = SendData(PnlblkDm, sizeof(NLBLK) + (DWORD)wParam);
            }
            break;


        case tlfReply:
            DEBUG_OUT( "DMTlFunc:  tlfReply\n" );
#if DBG
            EnterCriticalSection(&csExpecting);
            assert(FExpectingReply);
            FExpectingReply = 0;
            LeaveCriticalSection(&csExpecting);
#endif
            if (!FConnected) {
                xosd = xosdLineNotConnected;
            } else if (wParam > MAX_INTERNAL_PACKET) {
                ZERO_Rgb(RgbReply);
                pMpckt = (MPACKET * ) PnlblkRp->rgchData;
                PnlblkRp->mtypeBlk = mtypeReplyMulti;
                PnlblkRp->hpid = hpid;
                PnlblkRp->cchMessage = MAX_INTERNAL_PACKET + sizeof(MPACKET);
                PnlblkRp->seq = ++pkSeq;
                pMpckt->packetNum = 0;

                pMpckt->packetCount = (((short)wParam + MAX_INTERNAL_PACKET - 1) /
                                       MAX_INTERNAL_PACKET);
                while (wParam > MAX_INTERNAL_PACKET) {
                    memcpy((LPVOID)pMpckt->rgchData, (LPB) lParam, MAX_INTERNAL_PACKET);
                    xosd = SendData(PnlblkRp, sizeof(NLBLK) + sizeof(MPACKET) +
                                    MAX_INTERNAL_PACKET);
                    if (xosd != xosdNone) {
                        return xosdUnknown;
                    }
                    wParam -= MAX_INTERNAL_PACKET;
                    lParam += MAX_INTERNAL_PACKET;

                    pMpckt->packetNum += 1;
                }

                memcpy((LPVOID)pMpckt->rgchData, (LPB) lParam, (DWORD)wParam);

                PnlblkRp->cchMessage = (short)(wParam + sizeof(MPACKET));

                xosd = SendData(PnlblkRp, sizeof(NLBLK) + sizeof(MPACKET) + (DWORD)wParam);
            } else {
                ZERO_Rgb(RgbReply);
                PnlblkRp->mtypeBlk = mtypeReply;

                PnlblkRp->hpid = hpid;
                PnlblkRp->cchMessage = (short)wParam;
                PnlblkRp->seq = ++pkSeq;

                memcpy((LPVOID)PnlblkRp->rgchData, (LPB) lParam, (DWORD)wParam);
                xosd = SendData(PnlblkRp, sizeof(NLBLK) + (DWORD)wParam);
            }
            break;


        case tlfRequest:
            DEBUG_OUT( "DMTlFunc:  tlfRequest\n" );
            if ( !FConnected ) {
                xosd = xosdLineNotConnected;
            } else {
                cb = ibMaxDM;
                if (wParam > MAX_INTERNAL_PACKET) {
                    ZERO_Rgb(RgbRequest);
                    pMpckt = (MPACKET * ) PnlblkRq->rgchData;
                    PnlblkRq->mtypeBlk = mtypeSyncMulti;
                    PnlblkRq->hpid = hpid;
                    PnlblkRq->cchMessage = MAX_INTERNAL_PACKET + sizeof(MPACKET);
                    PnlblkRq->seq = ++pkSeq;
                    pMpckt->packetNum = 0;

                    pMpckt->packetCount = (((short)wParam + MAX_INTERNAL_PACKET - 1) /
                                           MAX_INTERNAL_PACKET);
                    while (wParam > MAX_INTERNAL_PACKET) {
                        memcpy((LPVOID)pMpckt->rgchData, (LPB) lParam, MAX_INTERNAL_PACKET);
                        xosd = SendData(PnlblkRq, sizeof(NLBLK) + sizeof(MPACKET) +
                                        MAX_INTERNAL_PACKET);
                        if (xosd != xosdNone) {
                            return xosdUnknown;
                        }
                        wParam -= MAX_INTERNAL_PACKET;
                        lParam += MAX_INTERNAL_PACKET;

                        pMpckt->packetNum += 1;
                    }

                    PnlblkRq->cchMessage = (short)(wParam + sizeof(MPACKET));

                    memcpy((LPVOID)pMpckt->rgchData, (LPB) lParam, (DWORD)wParam);
                    xosd = SendRequest(mtypeReply,
                                       PnlblkRq,
                                       sizeof(NLBLK) + sizeof(MPACKET) + (DWORD)wParam,
                                       lpbDM,
                                       &cb,
                                       DMTLFuncSendReplyTimeout
                                       );
                } else {
                    ZERO_Rgb(RgbRequest);
                    PnlblkRq->mtypeBlk = mtypeSync;
                    PnlblkRq->cchMessage = (short)wParam;
                    PnlblkRq->hpid = hpid;
                    PnlblkRq->seq = ++pkSeq;
                    memcpy((LPVOID)PnlblkRq->rgchData, (char *) lParam, (DWORD)wParam);
                    xosd = SendRequest(mtypeReply,
                                       PnlblkRq,
                                       (DWORD)wParam + sizeof(NLBLK),
                                       lpbDM,
                                       &cb,
                                       DMTLFuncSendReplyTimeout
                                       );
                }
            }
            break;

        default:
            DEBUG_OUT( "DMTlFunc:  **** unknown tlf ****\n" );
            assert ( FALSE );
            break;
    }
    return xosd;
}



BOOL
CallBack(
    PNLBLK  pnlblk,
    int     cb
    )
{
    MPACKET *           pMpacket;
    DPACKET *           pDpckt;
    XOSD                xosd;
    LOADDMSTRUCT        lds;
    static int          cbMulti = 0;
    static char *       pbMulti = NULL;


    switch( pnlblk->mtypeBlk ) {
        case mtypeVersionRequest:
            DEBUG_OUT("CallBack:  mtypeVersionRequest\n" );
            TLFunc(tlfSendVersion, pnlblk->hpid, 0, 0);
            break;


        case mtypeSyncMulti:
        case mtypeAsyncMulti:
#if DBG
            if (pnlblk->mtypeBlk == mtypeAsyncMulti) {
                DEBUG_OUT("CallBack:  mtypeAsyncMulti\n" );
                EnterCriticalSection(&csExpecting);
                FExpectingReply = 0;
                LeaveCriticalSection(&csExpecting);
            } else {
                DEBUG_OUT("CallBack:  mtypeSyncMulti\n" );
                EnterCriticalSection(&csExpecting);
                FExpectingReply = 1;
                FExpectingSeq = pnlblk->seq;
                LeaveCriticalSection(&csExpecting);
            }
#endif
            //assert( cb == (int) (pnlblk->cchMessage + sizeof(NLBLK)) );
            if (FConnected) {
                pMpacket = (MPACKET *) pnlblk->rgchData;
                if (pMpacket->packetNum == 0) {
                    if (cbMulti < pMpacket->packetCount * MAX_INTERNAL_PACKET) {
                        pbMulti = realloc(pbMulti, pMpacket->packetCount *
                                          MAX_INTERNAL_PACKET);
                    }
                }
                memcpy(pbMulti + pMpacket->packetNum * MAX_INTERNAL_PACKET,
                       (LPVOID)pMpacket->rgchData, pnlblk->cchMessage - sizeof(MPACKET));
                if (pMpacket->packetNum + 1 == pMpacket->packetCount) {
                    cb = pMpacket->packetNum * MAX_INTERNAL_PACKET +
                      pnlblk->cchMessage - sizeof(MPACKET);
                    if (TLCallBack != NULL) {
                        TLCallBack( pnlblk->hpid, cb, (LPARAM) pbMulti);
                    } else if (LpDmFunc != NULL) {
                        LpDmFunc((DWORD) cb, pbMulti);
                    }
                }
            }
            break;

        case mtypeAsync:
        case mtypeSync:
#if DBG
            if (pnlblk->mtypeBlk == mtypeAsync) {
                DEBUG_OUT("CallBack:  mtypeAsync\n" );
                EnterCriticalSection(&csExpecting);
                FExpectingReply = 0;
                LeaveCriticalSection(&csExpecting);
            } else {
                DEBUG_OUT("CallBack:  mtypeSync\n" );
                EnterCriticalSection(&csExpecting);
                FExpectingReply = 1;
                FExpectingSeq = pnlblk->seq;
                LeaveCriticalSection(&csExpecting);
            }
#endif
            //assert( cb == (int) (pnlblk->cchMessage + sizeof(NLBLK)) );
            if (FConnected) {
                if (TLCallBack != NULL) {
                    DPRINT(("CallBack: calling TLCallBack\n"));
                    TLCallBack( pnlblk->hpid, pnlblk->cchMessage,
                               (LPARAM) pnlblk->rgchData);
                } else if (LpDmFunc != NULL) {
                    DPRINT(("CallBack: calling LpDmFunc\n"));
                    LpDmFunc(pnlblk->cchMessage, (LPVOID)pnlblk->rgchData);
                }
            }
            break;

        case mtypeDisconnect:
            DEBUG_OUT("CallBack:  mtypeDisconnect\n" );
            if (TLCallBack != NULL) {
                RTP rtp = {0};
                rtp.dbc = dbcRemoteQuit;
                rtp.hpid = pnlblk->hpid;
                TLCallBack(pnlblk->hpid, sizeof(rtp), (LPARAM)&rtp);
            }
            if (LpUiServer) {
                pDpckt = (DPACKET *) pnlblk->rgchData;
                LpUiServer( tlcbDisconnect,
                            pDpckt->hpid,
                            pDpckt->htid,
                            pDpckt->fContinue,
                            0
                          );
            }
            SetEvent(hRemoteQuit);
            break;

        case mtypeTransportIsDead:
            DEBUG_OUT("CallBack:  mtypeTransportIsDead\n" );
            TransportFailure();
            return FALSE;
            break;

        case mtypeLoadDM:
            DPRINT(("CallBack: LoadDM; '%p', '%p'\n",
                    ((LPLOADDMSTRUCT)pnlblk->rgchData)->lpDmName,
                    ((LPLOADDMSTRUCT)pnlblk->rgchData)->lpDmParams));
            assert(FDMSide);
            lds.lpDmName = (LPVOID)pnlblk->rgchData;
            lds.lpDmParams = (LPSTR)pnlblk->rgchData + strlen(lds.lpDmName) + 1;

            xosd = LoadDM(pnlblk->hpid, &lds);

            ZERO_Rgb(RgbReply);
            PnlblkRp->mtypeBlk = mtypeLoadDMReply;

            PnlblkRp->hpid = pnlblk->hpid;
            PnlblkRp->cchMessage = sizeof(XOSD);
            PnlblkRp->seq = ++pkSeq;

            memcpy((LPVOID)PnlblkRp->rgchData, (LPB) &xosd, sizeof(XOSD));
            DPRINT(("CallBack: LoadDM; replying with 0x%x\n", xosd));
            xosd = SendData(PnlblkRp, sizeof(NLBLK) + sizeof(XOSD));
            break;

        default:
            assert(FALSE);
    }

    return TRUE;
}



VOID
TransportFailure(
    VOID
    )
{
    DEBUG_OUT("*** TransportFailure()\n" );

    if (FDMSide) {
        TLFunc( tlfRemoteQuit, 0, 0, 0 );
    }

    if (LpUiServer) {

        LpUiServer(tlcbDisconnect, 0, 0, 0, 0);

    } else if (TLCallBack) {

        RTP rtp = {0};
        rtp.dbc = dbcRemoteQuit;
        rtp.hpid = hpidRoot;
        TLCallBack( hpidRoot, sizeof(rtp), (LPARAM)&rtp );

    }

    FConnected = FALSE;

    return;
}


XOSD
SendData(
         LPV    lpvOut,
         int    cbOut
         )
{
    if (!TlWriteTransport(lpvOut, cbOut)) {
        return xosdWrite;
    }

    return xosdNone;
}


//
// data structures for tlfreplys
//
// these structures exist in the physical layer of the
// transport layer (pipe, serial, ...)
//

extern CRITICAL_SECTION CsReplys;
extern int              IReplys;
extern REPLY            RgReplys[];


XOSD
SendRequest(
    int         mtypeResponse,
    LPV         lpvOut,
    int         cbOut,
    LPV         lpvReply,
    int *       pcbReply,
    DWORD       dwTimeOut
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    mtypeResponse       - Supplies the packet type to be used as a response

    lpvOut              - Supplies the request packet

    cbOut               - Supplies length of request packet

    lpvIn               - Returns the reply data

    cbIn                - Supplies length of return buffer

    dwTimeOut           - Supplies -1 or # of seconds to wait before timeout

Return Value:

    XOSD error code

--*/

{
    int         i;
    DWORD       Status;
    XOSD        xosd = xosdNone;

    //
    //  Allow us to work with impunity
    //

    EnterCriticalSection(&CsReplys);

    //
    //  Are we in trouble due to overflow?
    //

    if (IReplys == SIZE_OF_REPLYS) {
        LeaveCriticalSection(&CsReplys);
        return xosdUnknown;
    }

    assert( IReplys == 0 );

    //
    //  Setup the reply location
    //

    RgReplys[IReplys].lpb = (char *) lpvReply;
    RgReplys[IReplys].cbBuffer = *pcbReply;
    RgReplys[IReplys].cbRet = 0;

    i = IReplys;

    IReplys += 1;

    //
    //
    //
    ResetEvent( RgReplys[i].hEvent );


    LeaveCriticalSection(&CsReplys);

    //
    //   Now finally mail the request out
    //

    if (!TlWriteTransport(lpvOut, cbOut)) {
        EnterCriticalSection(&CsReplys);
        IReplys -= 1;
        LeaveCriticalSection(&CsReplys);
        return xosdWrite;
    }

    //
    //  Wait for the reply to come back
    //

    if (dwTimeOut != INFINITE) {
        dwTimeOut *= 1000;
    }

    Status = WaitForSingleObject(RgReplys[i].hEvent, dwTimeOut);

    if (Status != WAIT_OBJECT_0) {

        //
        // timed out
        //

        DPRINT(("SendRequest: timed out waiting for reply\n"));
        xosd = xosdTransportTimedOut;

    } else {

        //
        //  Now get the message back
        //

        EnterCriticalSection(&CsReplys);

        RgReplys[i].lpb = NULL;
        *pcbReply = RgReplys[i].cbRet;
        if (RgReplys[i].cbRet == 0) {
            xosd = xosdUnknown;
        }

        assert( IReplys == i + 1 );

        if (IReplys == i + 1) {
            IReplys = i;
        } else {
            xosd = xosdUnknown;
        }

        LeaveCriticalSection(&CsReplys);
    }
    return xosd;
}

VOID
DebugPrint(
    LPSTR szFormat,
    ...
    )
{
    va_list  marker;
    int      n;
    char     rgchDebug[4096];

    va_start( marker, szFormat );
    n = _vsnprintf(rgchDebug, sizeof(rgchDebug), szFormat, marker );
    va_end( marker);

    if (n == -1) {
        rgchDebug[sizeof(rgchDebug)-1] = '\0';
    }

#if 0 //keep debug version from printing things out
    OutputDebugString( rgchDebug );
#endif
    return;
}

VOID
ShowAssert(
    LPSTR condition,
    UINT  line,
    LPSTR file
    )
{
    char text[4096];
    int  id;

    _snprintf(text, sizeof(text), "Assertion failed - Line:%u, File:%Fs, Condition:%Fs", line, file, condition);
    DebugPrint( "%s\r\n", text );
    id = MessageBox( NULL, text, "Pipe Transport", MB_YESNO | MB_ICONHAND | MB_TASKMODAL | MB_SETFOREGROUND );
    if (id != IDYES) {
        DebugBreak();
    }

    return;
}

void
TlRegisterWorkerThread(
    HANDLE hThread
    )
/*++

Routine Description:

    Add a thread to the list of utility threads which must exit before
    the TL may be unloaded.

Arguments:

    hThread - Supplies  a thread handle to add to the list.

Return Value:



--*/
{
    EnterCriticalSection(&TlpCsThreadList);

    //
    // Add a new thread to the list and signal waiting thread to
    // refresh its list and rewait.
    //

    TlpWaitHandlesList[TlpWaitHandlesCount++] = hThread;

    SetEvent(TlpWaitHandlesList[0]);

    LeaveCriticalSection(&TlpCsThreadList);
}

void
TlUnregisterWorkerThread(
    HANDLE hThread
    )
/*++

Routine Description:

    Remove a thread handle from the list of TL utility threads.
    When a thread is removed from the list, its handle will be
    closed.  This function will be called by TlWaitForThreadsToExit
    whenever it sees a thread exit.  It may be called elsewhere as
    well.  It is not an error to call with a thread which has
    already been removed from the list.

Arguments:

    hThread - Supplies the thread handle which is to be closed
              and removed from the waiting list.

Return Value:

    None

--*/
{
    DWORD i;

    EnterCriticalSection(&TlpCsThreadList);

    //
    // remove a thread from the list, signal waiting thread
    // to refresh and rewait.
    //


    for (i = 1; i < TlpWaitHandlesCount; i++) {

        if (hThread == TlpWaitHandlesList[i]) {

            --TlpWaitHandlesCount;
            CloseHandle(hThread);

            while (i < TlpWaitHandlesCount) {
                TlpWaitHandlesList[i] = TlpWaitHandlesList[i+1];
                ++i;
            }

            break;
        }
    }

    SetEvent(TlpWaitHandlesList[0]);

    LeaveCriticalSection(&TlpCsThreadList);
}

BOOL
TlpWaitForThreadsToExit(
    void
    )
/*++

Routine Description:

    This routine is called by TLFunc to ensure that all of the worker threads
    have exited before the TL is unloaded.  It will block until there are no
    longer any running threads in the wait list.

Arguments:

    None

Return Value:

    TRUE if all of the threads have exited.
    FALSE if an error occurs while waiting.


--*/
{
    HANDLE WaitHandlesList[MAX_TL_WAIT_THREADS];
    DWORD WaitHandlesCount;
    DWORD Status;

    //
    // Copy list of threads
    //

    do {

        EnterCriticalSection(&TlpCsThreadList);

        //
        // if list is empty, exit
        //

        if (TlpWaitHandlesCount < 2) {
            LeaveCriticalSection(&TlpCsThreadList);
            return TRUE;
        }

        memcpy(WaitHandlesList, TlpWaitHandlesList, TlpWaitHandlesCount * sizeof(HANDLE));
        WaitHandlesCount = TlpWaitHandlesCount;

        ResetEvent(WaitHandlesList[0]);

        LeaveCriticalSection(&TlpCsThreadList);

        //
        // Wait for signal
        //

        Status = WaitForMultipleObjectsEx(
                            WaitHandlesCount,
                            WaitHandlesList,
                            FALSE,
                            INFINITE,
                            FALSE
                            );

        //
        // if a thread exited, remove it from the list and close the handle.
        //

        if (Status > 0 && Status < WaitHandlesCount) {
            TlUnregisterWorkerThread(WaitHandlesList[Status]);
        }

        //
        // If a thread exited or the list changed,
        // refresh the list and rewait.
        //
    } while (Status < WaitHandlesCount);

    //
    // Since the wait is non-alertable and the objects
    // should never be abandoned, we should not get here.
    //

    DPRINT(("TlWaitForThreadsToExit: WaitForMultipleObjects returned 0x%08x\n", Status));

    return FALSE;
}
