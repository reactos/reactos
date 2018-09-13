/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WUCOMM.C
 *  WOW32 16-bit User API support
 *
 *  History:
 *  Created   07-Mar-1991 by Jeff Parsons (jeffpar)
 *  made real Dec-1992 by Craig Jones (v-cjones)
 *  made work Apr-1993 by Craig Jones (v-cjones)
 *  made fast Jun-1993 by Craig Jones (v-cjones)
--*/

#include "precomp.h"
#pragma hdrstop
#include <ntddser.h>

MODNAME(wucomm.c);

/* Define the table for mapping Win3.1 idComDev's to 32-bit comm HFILE's. */
/* This table is indexed by the 16-bit idComDev that we return to the app */
/* which is assigned based on the device name (see wucomm.h).  You can    */
/* use GETPWOWPTR(idComDev) to get the ptr to the corresponding WOWPort   */
/* struct from PortTab[].                                                 */

/* This table must contain NUMPORTS (def'd in wucomm.h) entries */
PORTTAB PortTab[] = { {"COM1", NULL},
                      {"COM2", NULL},
                      {"COM3", NULL},
                      {"COM4", NULL},
                      {"COM5", NULL},
                      {"COM6", NULL},
                      {"COM7", NULL},
                      {"COM8", NULL},
                      {"COM9", NULL},
                      {"LPT1", NULL},
                      {"LPT2", NULL},
                      {"LPT3", NULL}
                    };


/* function prototypes for local support functions */
DWORD    Baud16toBaud32(UINT BaudRate);
WORD     Baud32toBaud16(DWORD BaudRate);
void     DCB16toDCB32(PWOWPORT pWOWPort, LPDCB lpdcb32, PDCB16 pdcb16);
void     DCB32toDCB16(PDCB16 pdcb16,  LPDCB lpdcb32,  UINT idComDev,  BOOL fChEvt);
BOOL     DeletePortTabEntry(PWOWPORT pWOWPort);
ULONG    WOWCommWriterThread(LPVOID pWOWPortStruct);
USHORT   EnqueueCommWrite(PWOWPORT pwp, PUCHAR pch, USHORT cb);
UINT     GetModePortTabIndex(PSZ pszModeStr);
BOOL     GetPortName(LPSTR pszMode, LPSTR pszPort);
UINT     GetStrPortTabIndex(PSZ szPort);
BOOL     InitDCB32(LPDCB pdcb32, LPSTR pszModeStr);
VOID     InitDEB16(PCOMDEB16 pComDEB16,  UINT iTab,  WORD QInSize,  WORD QOutSize);
PSZ      StripPortName(PSZ psz);
PSZ      GetPortStringToken(PSZ pszSrc, PSZ pszToken);
BOOL     MSRWait(PWOWPORT pwp);
BOOL     IsQLinkGold(WORD wTDB);

/* prototypes for Modem interrupt emulation thread support */
VOID  WOWModemIntThread(PWOWPORT pWOWPortStruct);
BOOL  WOWStartModemIntThread(PWOWPORT pWOWPort);
DWORD WOWGetCommError(PWOWPORT pWOWPort);



// Win3.1 returns:
//    0 on success OR LPT.
//    -1 on ANY error.
ULONG FASTCALL WU32BuildCommDCB(PVDMFRAME pFrame)
{
    ULONG    ul = (ULONG)-1;
    UINT     len, iTab;
    PSZ      psz1;
    PDCB16   pdcb16;
    DCB      dcb32;
    register PBUILDCOMMDCB16 parg16;

    GETARGPTR(pFrame, sizeof(BUILDCOMMDCB16), parg16);
    GETPSZPTR(parg16->f1, psz1);

    // if valid device name...
    if((INT)(iTab = GetModePortTabIndex(psz1)) >= 0) {

        // Initialize a Win3.1 compatible 32-bit DCB
        if(InitDCB32(&dcb32, psz1)) {

            GETMISCPTR(parg16->f2, pdcb16);

            if(pdcb16) {
                // copy the psz1 fields to the 16-bit struct
                iTab = (VALIDCOM(iTab) ? iTab : TABIDTOLPT(iTab));
                DCB32toDCB16(pdcb16, &dcb32, iTab, FALSE);

                // set timeouts for COMx ports only
                if(VALIDCOM(iTab)) {

                    // 'P' is the only "retry" option supported in Win3.1
                    len = strlen(psz1) - 1;
                    while(psz1[len] != ' ') {  // delete trailing spaces
                        len--;
                    }
                    if((psz1[len] == 'P') || (psz1[len] == 'p')) {
                        pdcb16->RlsTimeout = INFINITE_TIMEOUT;
                        pdcb16->CtsTimeout = INFINITE_TIMEOUT;
                        pdcb16->DsrTimeout = INFINITE_TIMEOUT;
                    }
                }

                FLUSHVDMPTR(parg16->f2, sizeof(DCB16), pdcb16);
                FREEMISCPTR(pdcb16);

                ul = 0; // Win3.1 returns 0 if success
            }
        }
        FREEPSZPTR(psz1);
    }

#ifdef DEBUG
    if(!(ul==0)) {
        LOGDEBUG(0,("WOW::WU32BuildCommDCB: failed\n"));
    }
#endif

    FREEARGPTR(parg16);
    RETURN(ul);
}




// Win3.1 returns:
//    Error word on success OR LPTx.
//    0x8000 on bad idComDev.
ULONG FASTCALL WU32ClearCommBreak(PVDMFRAME pFrame)
{
    ULONG    ul = 0x00008000;
    UINT     idComDev;
    PWOWPORT pWOWPort;
    register PCLEARCOMMBREAK16 parg16;

    GETARGPTR(pFrame, sizeof(CLEARCOMMBREAK16), parg16);

    idComDev = UINT32(parg16->f1);
    if (pWOWPort = GETPWOWPTR(idComDev)) {

        if (VALIDCOM(idComDev)) {
            if(!ClearCommBreak(pWOWPort->h32)) {
                WOWGetCommError(pWOWPort);
            }
        }
        ul = pWOWPort->dwErrCode;
    }

#ifdef DEBUG
    if(!(ul!=0x00008000)) {
        LOGDEBUG(0,("WOW::WU32ClearCommBreak: failed\n"));
    }
#endif

    FREEARGPTR(parg16);
    RETURN(ul);
}




// Win3.1 returns:
//    0 if success OR if LPTx.
//    -1 for bad idComDev OR port not open.
//    -2 for Timeout error.
// We pass back (as a 2nd parameter) the DWORD obtained from the call to
// GlobalDosAlloc() in IOpenComm() in user.exe.  (WOWModemIntThread() support)
ULONG FASTCALL WU32CloseComm(PVDMFRAME pFrame)
{
    ULONG    ul = (ULONG)-1;
    UINT     idComDev;
    PDWORD16 lpdwDEB16;
    PWOWPORT pWOWPort = NULL;
    register PCLOSECOMM16 parg16;

    GETARGPTR(pFrame, sizeof(CLOSECOMM16), parg16);

    idComDev = UINT32(parg16->f1);
    if (pWOWPort = GETPWOWPTR(idComDev)) {

        // pass back the 16:16 ptr for the WOWModemIntThread() support
        GETMISCPTR(parg16->f2, lpdwDEB16);
        if (lpdwDEB16) {
            *lpdwDEB16 = pWOWPort->dwComDEB16;
            FLUSHVDMPTR(parg16->f2, sizeof(DWORD), lpdwDEB16);
            FREEMISCPTR(lpdwDEB16);
        }

        // clean up the PortTab[] entry
        if (DeletePortTabEntry(pWOWPort)) {
            ul = (ULONG)-2; // return Win3.1 timeOut error
        }
        else {
            ul = 0;
        }
    }
    else {
        LOGDEBUG (0, ("WOW::WU32CloseComm: Not a valid COM or LPT\n"));
    }

#ifdef DEBUG
    if(!(ul==0)) {
        LOGDEBUG(0,("WOW::WU32CloseComm: failed\n"));
    }
#endif

    FREEARGPTR(parg16);
    RETURN(ul);
}




// Win3.1 returns:
//    TRUE on success.
//    FALSE if error OR if EnableCommNotification() not supported.
//          User16 validation layer returns 0 for bad hwnd.
ULONG FASTCALL WU32EnableCommNotification(PVDMFRAME pFrame)
{
    ULONG     ul = (ULONG)FALSE;
    UINT      idComDev;
    WORD      cbQue;
    BOOL      fOK = TRUE;
    PWOWPORT  pWOWPort;
    PCOMDEB16 lpComDEB16;
    register  PENABLECOMMNOTIFICATION16 parg16;

    GETARGPTR(pFrame, sizeof(ENABLECOMMNOTIFICATION16), parg16);

    idComDev = UINT32(parg16->f1);
    if ((VALIDCOM(idComDev)) && (pWOWPort = PortTab[idComDev].pWOWPort)) {

        lpComDEB16 = pWOWPort->lpComDEB16;

        // if they are trying to disable notifcation (HWND == NULL)
        if(WORD32(parg16->f2) == 0) {
            lpComDEB16->NotifyHandle = 0;
            lpComDEB16->NotifyFlags  = CN_TRANSMITHI;
            lpComDEB16->RecvTrigger  = (WORD)-1;
            lpComDEB16->SendTrigger  = 0;
            ul = (ULONG)TRUE;
        }

        // Validate non-null hwnd's since hwnd validation is disabled in
        // user16 validation layer
        else if(!IsWindow(HWND32(parg16->f2))) {
            ul = (ULONG)FALSE;
        }

        // else set up the notification mechanisms
        else {

            // if the Modem interrupt thread hasn't started yet -- go start it
            if(pWOWPort->hMiThread == NULL) {

                if(!WOWStartModemIntThread(pWOWPort)) {
                    fOK = FALSE;
                }
            }

            // update the DEB to reflect notification
            if(fOK) {

                lpComDEB16->NotifyHandle = WORD32(parg16->f2);
                lpComDEB16->NotifyFlags  = CN_TRANSMITHI | CN_NOTIFYHI;

                // set trigger values the same way Win3.1 does
                cbQue = WORD32(parg16->f3);
                if((cbQue < lpComDEB16->QInSize) || ((SHORT)cbQue == -1)) {
                    lpComDEB16->RecvTrigger = cbQue;
                }
                else {
                    lpComDEB16->RecvTrigger = lpComDEB16->QInSize - 10;
                }
                cbQue = WORD32(parg16->f4);
                if((cbQue < lpComDEB16->QOutSize) || ((SHORT)cbQue == -1)) {
                    lpComDEB16->SendTrigger = cbQue;
                }
                else {
                    lpComDEB16->SendTrigger = lpComDEB16->QOutSize - 10;
                }

                ul = (ULONG)TRUE;
            }
        }
    }
    // else there is no notification for LPT in Win3.1
    else {
        ul = (ULONG)FALSE;
    }

#ifdef DEBUG
    if(!(ul==1)) {
        LOGDEBUG(0,("WOW::WU32EnableCommNotification: failed\n"));
    }
#endif

    FREEARGPTR(parg16);
    RETURN(ul);
}




// Win3.1 returns:
//    The value from the specified function.
//    The error word for: the line & signal state functions,
//    function not implemented, OR LPTx where function != (RESETDEV||GETMAXLPT).
//    0x8000 for bad idComDev.
ULONG FASTCALL WU32EscapeCommFunction(PVDMFRAME pFrame)
{
    ULONG    ul = 0x00008000;
    UINT     idComDev;
    UINT     nFunction;
    WORD     IRQ;
    VPVOID   vpBiosData;
    PWORD16  pwBiosData;
    PWOWPORT pWOWPort;
    register PESCAPECOMMFUNCTION16 parg16;

    GETARGPTR(pFrame, sizeof(ESCAPECOMMFUNCTION16), parg16);

    // this construct is set up this way because Win3.1 will allow GETMAXCOM
    // & GETMAXLPT to succeed as long as the idComDev is in the valid range.
    // (ie: the app doesn't have to call OpenComm() first to set up the PortTab)
    // for RESETDEV we tell them that we reset the printer. (we're such liars!)

    nFunction = WORD32(parg16->f2);
    idComDev  = UINT32(parg16->f1);
    if (VALIDCOM(idComDev)) {

        if (nFunction == GETMAXCOM) {
            ul = NUMCOMS-1;
        } else if (nFunction == GETBASEIRQ || nFunction == GETBASEIRQ+1) {
            ul = 0xFFFFFFFF;
            if (idComDev < COM5) {
                vpBiosData = (VPVOID) (RM_BIOS_DATA + (idComDev * sizeof(WORD)));
                if (pwBiosData = (PWORD16)GetRModeVDMPointer(vpBiosData)) {
                    if (idComDev == COM1 || idComDev == COM3) {
                        IRQ = IRQ4;
                    } else {
                        IRQ = IRQ3;
                    }
                    ul = MAKELONG((WORD)(*pwBiosData), IRQ);
                    FREEVDMPTR(pwBiosData);
                }
            }
        } else {
            // for the other functions they must have called OpenComm()
            if (pWOWPort = PortTab[idComDev].pWOWPort) {

                switch(nFunction) {

                // line & signal state functions
                case    SETXOFF:
                case    SETXON:
                case    SETRTS:
                case    CLRRTS:
                case    SETDTR:
                case    CLRDTR:
                    if(!EscapeCommFunction(pWOWPort->h32, nFunction)) {
                        WOWGetCommError(pWOWPort);
                    }
                    ul = pWOWPort->dwErrCode;
                    break;

                // 0:
                case         0:
                    ul = 0;  // like WFW
                    break;

                // any other value...
                default:

                    // non-zero is error: use dwErrcode if there is one
                    if(pWOWPort->dwErrCode)
                        ul = pWOWPort->dwErrCode;

                    // else use what WFW seems inclined to return
                    else
                        ul = CE_OVERRUN | CE_RXPARITY;
                    break;
                }
            }
        }
    } else if (VALIDLPT(idComDev)) {
        if(nFunction == RESETDEV) {
            ul = 0;  // no error (ie. "just tell them we did it" - TonyE)
        }
        else if(nFunction == GETMAXLPT) {
            ul = LPTLAST;
        }
        else if (pWOWPort = PortTab[GETLPTID(idComDev)].pWOWPort) {
            ul = pWOWPort->dwErrCode;
        }
        else {
            ul = 0;
        }
    }

#ifdef DEBUG
    if(!(ul>=0)) {
        LOGDEBUG(0,("WOW::WU32EscapeCommFunction: failed\n"));
    }
#endif
    FREEARGPTR(parg16);

    RETURN(ul);
}




// Win3.1 returns:
//    0 on success.
//    0x8000 if bad idComDev.
//    Error word on error or LPTx.
ULONG FASTCALL WU32FlushComm(PVDMFRAME pFrame)
{
    ULONG    ul = 0x00008000;
    UINT     idComDev;
    DWORD    dwAction;
    PWOWPORT pWOWPort;
    register PFLUSHCOMM16 parg16;

    GETARGPTR(pFrame, sizeof(FLUSHCOMM16), parg16);

    idComDev = UINT32(parg16->f1);
    if (pWOWPort = GETPWOWPTR(idComDev)) {

        // is a COMx?
        if (VALIDCOM(idComDev)) {

            // if flush transmit buffer specified
            dwAction = PURGE_RXCLEAR;
            if(parg16->f2 == 0) {
                dwAction = PURGE_TXCLEAR | PURGE_TXABORT;

                //
                // Flush the local writers buffer
                //

                EnterCriticalSection(&pWOWPort->csWrite);
                pWOWPort->pchWriteHead =
                pWOWPort->pchWriteTail = pWOWPort->pchWriteBuf;
                pWOWPort->cbWriteFree  = pWOWPort->cbWriteBuf - 1;
                pWOWPort->cbWritePending = 0;
                LeaveCriticalSection(&pWOWPort->csWrite);
            }


            if(PurgeComm(pWOWPort->h32, dwAction)) {

                if(dwAction == PURGE_RXCLEAR) {
                    pWOWPort->fUnGet = FALSE;
                }

                ul = 0;  // Win3.1 returns 0 on success
            }
            else {
                WOWGetCommError(pWOWPort);
                ul = pWOWPort->dwErrCode;
            }
        }
        // else just return current error code for LPTx
        else {
            ul = pWOWPort->dwErrCode;
        }
    }

#ifdef DEBUG
    if(!(ul==0)) {
        LOGDEBUG(0,("WOW::WU32FlushComm: failed\n"));
    }
#endif

    FREEARGPTR(parg16);
    RETURN(ul);
}




// Win3.1 returns:
//    0x8000 for bad idComDev.
//    The error word for all other cases.
ULONG FASTCALL WU32GetCommError(PVDMFRAME pFrame)
{
    ULONG       ul = 0x00008000;
    UINT        idComDev;
    PWOWPORT    pWOWPort;
    PCOMSTAT16  pcs16;
    register    PGETCOMMERROR16 parg16;

    GETARGPTR(pFrame, sizeof(GETCOMMERROR16), parg16);
    GETMISCPTR(parg16->f2, pcs16);

    idComDev = UINT32(parg16->f1);
    if (pWOWPort = GETPWOWPTR (idComDev)) {

        if (VALIDCOM(idComDev) && pcs16) {

            WOWGetCommError(pWOWPort);


            // Always update the COMSTAT status byte, DynComm depends on it.
            pcs16->status = 0;
            if(pWOWPort->cs.fCtsHold)  pcs16->status |= W31CS_fCtsHold;
            if(pWOWPort->cs.fDsrHold)  pcs16->status |= W31CS_fDsrHold;
            // Note: RlsdHold is zero'd out on Win3.1
            if(pWOWPort->cs.fRlsdHold) pcs16->status |= W31CS_fRlsdHold;
            if(pWOWPort->cs.fXoffHold) pcs16->status |= W31CS_fXoffHold;
            if(pWOWPort->cs.fXoffSent) pcs16->status |= W31CS_fSentHold;
            if(pWOWPort->cs.fEof)      pcs16->status |= W31CS_fEof;
            if(pWOWPort->cs.fTxim)     pcs16->status |= W31CS_fTxim;

            pcs16->cbInQue  = (WORD)pWOWPort->cs.cbInQue;
            pcs16->cbOutQue = (WORD)pWOWPort->cs.cbOutQue;

            // account for the UnGot char (if any)
            if(pWOWPort->fUnGet) {
                pcs16->cbInQue++;
            }
        }

        // if an LPT OR pcs16 == NULL, Win3.1 returns the error code
        else {
            // for LPT's Win3.1 just zero's the COMSTAT & returns the error code
            if(VALIDLPT(idComDev)) {
                if(pcs16) {
                    RtlZeroMemory((PVOID)pcs16, sizeof(COMSTAT16));
                }
            }
        }

        ul = (ULONG)pWOWPort->dwErrCode;

        // clear the error now that the app has got it (but maintain queues)
        pWOWPort->dwErrCode = 0;
        pWOWPort->lpComDEB16->ComErr = 0;
        RtlZeroMemory((PVOID)&(pWOWPort->cs), sizeof(COMSTAT));
        if(pcs16) {
            pWOWPort->cs.cbInQue  = pcs16->cbInQue;
            pWOWPort->cs.cbOutQue = pcs16->cbOutQue;
        }
    }

    FLUSHVDMPTR(parg16->f2, sizeof(COMSTAT16), pcs16);
    FREEMISCPTR(pcs16);
    FREEARGPTR(parg16);
    RETURN(ul);
}




// Win3.1 returns:
//    EvtWord on success.
//    0 for bad idComDev OR LPTx.
ULONG FASTCALL WU32GetCommEventMask(PVDMFRAME pFrame)
{
    ULONG     ul=0;
    DWORD     dwEvtMask;
    UINT      idComDev;
    PWOWPORT  pWOWPort;
    PCOMDEB16 pDEB16;
    register  PGETCOMMEVENTMASK16 parg16;

    GETARGPTR(pFrame, sizeof(GETCOMMEVENTMASK16), parg16);

    idComDev = UINT32(parg16->f1);
    if (VALIDCOM(idComDev)) {
        if(pWOWPort = PortTab[idComDev].pWOWPort) {

            if(pDEB16 = pWOWPort->lpComDEB16) {

                // in Win3.1 the app gets current event word (NOT the EvtMask!!)
                ul = (ULONG)pDEB16->EvtWord;

                // clear event word like Win3.1 does
                dwEvtMask = (DWORD)WORD32(parg16->f2);
                pDEB16->EvtWord = LOWORD((~dwEvtMask) & (DWORD)ul);
            }
        }
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}




// Win3.1 returns:
//    0 for success.
//    -1 for bad idComDev.
//    IE_NOPEN for not opened.
ULONG FASTCALL WU32GetCommState(PVDMFRAME pFrame)
{
    ULONG    ul = (ULONG)-1;
    UINT     idComDev;
    DCB      dcb32;
    PWOWPORT pWOWPort;
    PDCB16   pdcb16;
    register PGETCOMMSTATE16 parg16;

    GETARGPTR(pFrame, sizeof(GETCOMMSTATE16), parg16);

    idComDev = UINT32(parg16->f1);
    if (pWOWPort = GETPWOWPTR(idComDev)) {

        GETMISCPTR(parg16->f2, pdcb16);

        if(pdcb16) {
            if(VALIDCOM(idComDev)) {
                if(GetCommState(pWOWPort->h32, &dcb32)) {

                    DCB32toDCB16(pdcb16, &dcb32, idComDev, pWOWPort->fChEvt);
                    ul = 0; // Win3.1 returns 0 if success
                }
            }

            // else get DCB for LPT's
            else {
                RtlCopyMemory((PVOID)pdcb16,
                              (PVOID)pWOWPort->pdcb16,
                              sizeof(DCB16));
                ul = 0; // Win3.1 returns 0 if success
            }

            FLUSHVDMPTR(parg16->f2, sizeof(DCB16), pdcb16);
            FREEMISCPTR(pdcb16);
        }
    }
    // else if they got a handle that looks good but they didn't open the port
    else if(VALIDCOM(idComDev) || VALIDLPT(idComDev)) {
        ul = (ULONG)IE_NOPEN;
    }

#ifdef DEBUG
    if(!(ul==0)) {
        LOGDEBUG(0,("WOW::WU32GetCommState: failed\n"));
    }
#endif

    FREEARGPTR(parg16);
    RETURN(ul);
}




// Win3.1 returns:
//    An idComDev on success.
//    IE_BADID for bad port name.
//    IE_OPEN if port already open.
//    IE_HARDWARE if hardware in use (ie. by mouse) OR port doesn't exist.
//    IE_MEMORY if both cbInQueue & cbOutQueue == 0 OR can't allocate a queue.
//    IE_NOPEN if can't open port.
//    IE_DEFAULT if initialization fails for various reasons.
// We pass an additional (4th) parameter from IOpenComm() for SetCommEventMask()
// support.  It's a DWORD that is obtained by a call to GlobalDosAlloc().
ULONG FASTCALL WU32OpenComm(PVDMFRAME pFrame)
{
    INT          ret;
    UINT         iTab, idComDev;
    CHAR         COMbuf[] = "COMx:9600,E,7,1";  // Win3.1 default
    CHAR         szPort[MAXCOMNAMENULL];
    DWORD        dwDEBAddr;
    DWORD        cbInQ  = 0;
    DWORD        cbOutQ;
    HANDLE       h32     = 0;
    HANDLE       hREvent = 0;
    DCB          dcb32;
    PSZ          psz1;
    PDCB16       pdcb16  = NULL;
    PWOWPORT     pWOWPort;
    PCOMDEB16    lpComDEB16;
    COMMTIMEOUTS ct;
    PUCHAR       pchWriteBuf = NULL;
    UINT         cbWriteBuf = 0;
    HANDLE       hWriteEvent = 0;
    DWORD        dwWriteThreadId;
    BOOL         fIsLPTPort;
    register     POPENCOMM16 parg16;

    GETARGPTR(pFrame, sizeof(OPENCOMM16), parg16);
    GETPSZPTR(parg16->f1, psz1);

    // see if valid com device name...
    if((iTab = GetModePortTabIndex(psz1)) == (UINT)IE_BADID) {
        ret = IE_BADID;
        goto ErrorExit0;
    }

    // check if named port is already in use
    if(PortTab[iTab].pWOWPort != NULL) {
        ret = IE_OPEN;
        goto ErrorExit0;
    }

    if ( VALIDCOM(iTab) ) {
        idComDev = iTab;
        fIsLPTPort = FALSE;
    } else {
        idComDev = TABIDTOLPT(iTab);
        fIsLPTPort = TRUE;
    }

    // get port name: app may pass in a full mode string in Win3.1
    GetPortName(psz1, szPort);

    // try to open the port
    if((h32 = CreateFile(szPort,
                         GENERIC_READ | GENERIC_WRITE,
                         0,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                         NULL)) == INVALID_HANDLE_VALUE) {

        if(GetLastError() == ERROR_FILE_NOT_FOUND) {
            ret = IE_HARDWARE;
        }
        else {
            LOGDEBUG (LOG_ERROR,("WOW::WU32OpenComm CreateFile failed, lasterror=0x%x\n",GetLastError()));
            ret = IE_NOPEN;
        }
        goto ErrorExit0;
    }



    // ignore LPT's for this check like Win3.1 does
    if( !fIsLPTPort ) {

        // common method apps use to see if a COM port is already open
        if((WORD32(parg16->f2) == 0) &&
           (WORD32(parg16->f3) == 0)) {
            ret = IE_MEMORY;
            goto ErrorExit1;
        }

        // set up the I/O queues
        cbInQ = (DWORD)WORD32(parg16->f2);
        cbOutQ = (DWORD)WORD32(parg16->f3);

        //
        // Allocate write buffer to emulate Win3.1's transmit queue.
        // We allocate one extra byte because the last byte of the
        // buffer is never filled.  If it were, then the head and
        // tail pointers would be equal, which we use to indicate
        // an *empty* buffer.
        //

        cbWriteBuf = cbOutQ + 1;

        if (!(pchWriteBuf = malloc_w(cbWriteBuf))) {
            ret = IE_MEMORY;
            goto ErrorExit1;
        }

        //
        // IO buffers must be a multiple of 2 for SetupComm().
        // Note that SetupComm may ignore the write buffer size
        // entirely, but TonyE says that we should still pass
        // down the size requested, since in any case writes
        // will complete only when the bits are irretrievably
        // sent, I.E. in the UART or other hardware, out of
        // the control of the device driver.
        //

        cbInQ = (cbInQ + 1) & ~1;
        cbOutQ = (cbOutQ + 1) & ~1;
        if(!SetupComm(h32, cbInQ, cbOutQ)) {
            ret = IE_MEMORY;
            goto ErrorExit2;
        }

        //
        // Create an event used by the app thread to wake up
        // the writer thread when the write buffer is
        // empty and the app writes something.  The event
        // is auto-reset, meaning it is reset when the
        // writer wakes up.  The event is initially not
        // signaled, it will be signaled when the first
        // write occurs.
        //

        if (!(hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL))) {
            ret = IE_MEMORY;
            goto ErrorExit2;
        }

        //
        // create an event for ReadComm()'s overlapped structure
        //

        if(!(hREvent = CreateEvent(NULL, TRUE, FALSE, NULL))) {
            ret = IE_NOPEN;
            goto ErrorExit3;
        }

        // set the timeout values
        ct.ReadIntervalTimeout = (DWORD)INFINITE;  // == MAXDWORD
        ct.ReadTotalTimeoutMultiplier=0;
        ct.ReadTotalTimeoutConstant=0;
        ct.WriteTotalTimeoutMultiplier=0;
        ct.WriteTotalTimeoutConstant=WRITE_TIMEOUT;
        if(!SetCommTimeouts(h32, &ct)) {
            ret = IE_DEFAULT;
            goto ErrorExit3;
        }

        // make sure the DCB is Win3.1 compatible
        // NOTE: app can pass in a full mode string in Win3.1
        if((strlen(psz1) < 4) || !InitDCB32(&dcb32, psz1)) {
            if(!InitDCB32(&dcb32, COMbuf)) {
                ret = IE_DEFAULT;
                goto ErrorExit3;
            }
        }

        // set current DCB to Win3.1 compatibility
        if(!SetCommState(h32, &dcb32)) {
            ret = IE_DEFAULT;
            goto ErrorExit3;
        }

        // purge the I/O buffers just to be sure
        PurgeComm(h32, PURGE_TXCLEAR);
        PurgeComm(h32, PURGE_RXCLEAR);

    }

    // we need to set up a default DCB for LPT's
    else {

        if((pdcb16 = malloc_w(sizeof(DCB16))) == NULL) {
            ret = IE_DEFAULT;
            goto ErrorExit1;
        }

        // initialize everything to 0
        RtlZeroMemory((PVOID)pdcb16, sizeof(DCB16));

        // save the idComDev only in the DCB
        pdcb16->Id = LOBYTE(LOWORD(idComDev));
    }

    // allocate the WOWPort structure for this port
    if((pWOWPort = malloc_w(sizeof(WOWPORT))) == NULL) {
        ret = IE_DEFAULT;
        goto ErrorExit3;
    }

    // get seg:sel dword returned by GlobalDosAlloc for the DEB struct
    // we'll treat the 16:16 pDEB as real mode on 32-bit side due to
    // some MIPS issues: v-simonf
    if (!(dwDEBAddr = DWORD32(parg16->f4))) {
        ret = IE_MEMORY;
        goto ErrorExit4;
    }

    // Isolate the segment value
    dwDEBAddr &= 0xFFFF0000;

    // save flat pointer to DEB for use in Modem interrupt thread
    lpComDEB16 = (PCOMDEB16) GetRModeVDMPointer(dwDEBAddr);

    // init the DEB
    InitDEB16(lpComDEB16, iTab, WORD32(parg16->f2), WORD32(parg16->f3));

    // init the support struct
    RtlZeroMemory((PVOID)pWOWPort, sizeof(WOWPORT));

    pWOWPort->h32            = h32;
    pWOWPort->idComDev       = idComDev;
    pWOWPort->dwComDEB16     = DWORD32(parg16->f4);
    pWOWPort->lpComDEB16     = lpComDEB16;
    pWOWPort->dwThreadID     = CURRENTPTD()->dwThreadID;
    pWOWPort->hREvent        = hREvent;
    pWOWPort->cbWriteBuf     = (WORD)cbWriteBuf;
    pWOWPort->cbWriteFree    = cbWriteBuf - 1;  // never use byte before head.
    pWOWPort->pchWriteBuf    = pchWriteBuf;
    pWOWPort->pchWriteHead   = pchWriteBuf;
    pWOWPort->pchWriteTail   = pchWriteBuf;
    pWOWPort->hWriteEvent    = hWriteEvent;
    pWOWPort->cbWritePending = 0;
    InitializeCriticalSection(&pWOWPort->csWrite);
    pWOWPort->pdcb16         = pdcb16;
    pWOWPort->cbInQ          = cbInQ;
    // hack for QuickLink Gold 1.3 -- See bug #398011
    // save QL stack sel in hiword, ComDEB16 seg in the loword
    if(IsQLinkGold(pFrame->wTDB)) {
        pWOWPort->QLStackSeg     = (DWORD32(parg16->f1) & 0xFFFF0000) | 
                                   (pWOWPort->dwComDEB16 & 0x0000FFFF);
    }
    // else pWOWPort->QLStackSeg implicitly set to 0 by RtlZeroMemory above.

    if (!(pWOWPort->olWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL))) {
        LOGDEBUG(0, ("%s", "WU32OpenComm unable to create overlapped write event, failing.\n"));
        ret = IE_MEMORY;
        goto ErrorExit4;
    }

    PortTab[iTab].pWOWPort = pWOWPort;

    //
    // Create the writer thread and pass it pWOWPort as its
    // parameter.
    //

    if (!fIsLPTPort) {
        pWOWPort->hWriteThread = CreateThread(
            NULL,                // lpsa
            0,                   // stack size (default)
            WOWCommWriterThread, // start address
            pWOWPort,            // lpvThreadParm
            0,                   // fdwCreate
            &dwWriteThreadId
            );

        if (!pWOWPort->hWriteThread) {
            ret = IE_MEMORY;
            goto ErrorExit5;
        }
    }

    ret = idComDev;   // return the idComDev
    goto CleanExit;

// this is the error code path
ErrorExit5:
    CloseHandle(pWOWPort->olWrite.hEvent);

ErrorExit4:
    free_w(pWOWPort);

ErrorExit3:
    if (hREvent) { CloseHandle(hREvent); }
    if (hWriteEvent) { CloseHandle(hWriteEvent); }
    if (fIsLPTPort) { free_w(pdcb16); }

ErrorExit2:
    if(pchWriteBuf) { free_w(pchWriteBuf); }

ErrorExit1:
    CloseHandle(h32);

ErrorExit0:
    LOGDEBUG (0, ("WOW::WU32OpenComm failed\n"));

CleanExit:
    FREEVDMPTR(psz1);
    FREEARGPTR(parg16);
    RETURN((ULONG)ret); // return error
}


//
// WriteComm()
//
// Win3.1 returns:
//    # bytes written on success (*= -1 on error).
//    0 for bad idComDev OR if app specifies to write 0 bytes.
//    -1 if port hasn't been opened,
//

ULONG FASTCALL WU32WriteComm(PVDMFRAME pFrame)
{
    register      PWRITECOMM16 parg16;
    LONG          i = -1;
    PSZ           psz2;
    PWOWPORT      pwp;
    UINT          idComDev;
    PWOWPORT      pWOWPort;
    DWORD         cbWritten;


    GETARGPTR(pFrame, sizeof(WRITECOMM16), parg16);
    GETPSZPTR(parg16->f2, psz2);

    idComDev = UINT32(parg16->f1);
    // this will be true only if the (valid) port has been opened
    if (pWOWPort = GETPWOWPTR(idComDev)) {

        if(VALIDCOM(idComDev)) {

            if ((pwp = GETPWOWPTR(UINT32(parg16->f1))) && psz2) {

                // if the app is interested in timeouts...
                if(pwp->lpComDEB16->MSRMask) {

                    // ...see if RLSD, CTS, & DSR timeout before going high
                    if(MSRWait(pwp)) {
                        FREEPSZPTR(psz2);
                        FREEARGPTR(parg16);
                        return(0);  // this is what Win3.1 does for Timeouts
                    }
                }

                i = EnqueueCommWrite(pwp, psz2, parg16->f3);
                if (i != parg16->f3) {
                     i = -i;
                     pwp->dwErrCode |= CE_TXFULL;
                }
            }
        }

        // else LPT's go this way...
        else {

            //
            // This call to WriteFile could block, but I don't think
            // that's a problem.  - DaveHart
            //
            if ((pwp = GETPWOWPTR(UINT32(parg16->f1))) && psz2) {

                if (!WriteFile(pwp->h32, psz2, parg16->f3, &cbWritten, &pwp->olWrite)) {

                    if (ERROR_IO_PENDING == GetLastError() ) {

                        //
                        // Wait for the write to complete or for us to
                        // be alerted that the port is closing.
                        //

                        if (GetOverlappedResult(pwp->h32,
                                                &pwp->olWrite,
                                                &cbWritten,
                                                TRUE
                                                )) {
                            i = cbWritten;
                            goto WriteSuccess;
                        }
                    }
                    LOGDEBUG(0, ("WU32WriteComm: WriteFile to id %u fails (error %u)\n",
                                 pwp->idComDev, GetLastError()));
                    if (cbWritten) {
                        i = cbWritten;
                        i = -i;
                    }
                }
                else {
                    i = cbWritten;
                }
            }
        }
    }
    else if(!(VALIDCOM(idComDev) || VALIDLPT(idComDev))) {
        i = 0;
    }
WriteSuccess:

    FREEPSZPTR(psz2);
    FREEARGPTR(parg16);
    RETURN((ULONG)i);
}


// Win3.1 returns:
//    # chars read on success.
//    0 for: bad idComDev, cbRead == 0, LPTx, port not open, 0 chars read,
//    OR for general comm error.
ULONG FASTCALL WU32ReadComm(PVDMFRAME pFrame)
{
    ULONG      ul = 0;
    ULONG      cb;
    BOOL       fUnGet = FALSE;
    UINT       idComDev;
    PBYTE      pb2;
    PWOWPORT   pWOWPort;
    OVERLAPPED Rol;
    register   PREADCOMM16 parg16;

    GETARGPTR(pFrame, sizeof(READCOMM16), parg16);
    GETMISCPTR(parg16->f2, pb2);

    cb = (ULONG)UINT32(parg16->f3);
    if((cb != 0) && pb2) {

        idComDev = UINT32(parg16->f1);
        if (VALIDCOM(idComDev) && (pWOWPort = PortTab[idComDev].pWOWPort)) {

            // if an UnGot char is pending
            if (pWOWPort->fUnGet) {
                fUnGet = TRUE;
                pWOWPort->fUnGet = FALSE;
                *pb2++ = pWOWPort->cUnGet;

                // this line commented out 8/3/95
                // cb--;  // we now need one less char

                // In order to make this work correctly we should cb-- above
                // to reflect the ungot char, unfortunately Win3.1 & Win95
                // don't do that so we will maintain this bug for "ouch!"
                // compatibility. a-craigj 8/3/95

            }

            // TonyE claims we should do this before each read to avoid problems
            Rol.Internal     = 0;
            Rol.InternalHigh = 0;
            Rol.Offset       = 0;
            Rol.OffsetHigh   = 0;
            Rol.hEvent       = pWOWPort->hREvent;

            if (!ReadFile(pWOWPort->h32,
                          pb2,
                          cb,
                          (LPDWORD)&ul,
                          &Rol)) {

                if (ERROR_IO_PENDING == GetLastError()) {

                    if (!GetOverlappedResult(pWOWPort->h32,
                                             &Rol,
                                             &ul,
                                             TRUE
                                             )) {

                        LOGDEBUG(0, ("WOW::WU32ReadComm:GetOverlappedResult failed, error = 0x%x\n",
                                     GetLastError()));
                        ul = 0;

                    }

                } else {

                    LOGDEBUG(0, ("WOW::WU32ReadComm:ReadFile failed, error = 0x%x\n",
                                 GetLastError()));
                    ul = 0;
                }
            }

            if(fUnGet) {
                ul++;   // account for ungot char
                pb2--;  // accounts for previous pb2++ for FREEVDMPTR
            }

            FLUSHVDMPTR(parg16->f2, (USHORT)ul, pb2);

        }

        FREEVDMPTR(pb2);
    }

#ifdef DEBUG
    if(!(ul>=0)) {
        LOGDEBUG(0,("WOW::WU32ReadComm: failed\n"));
    }
#endif

    FREEARGPTR(parg16);
    RETURN(ul);
}




// Win3.1 returns:
//    Error word on success OR LPTx.
//    0x8000 on bad idComDev.
ULONG FASTCALL WU32SetCommBreak(PVDMFRAME pFrame)
{
    ULONG    ul = 0x00008000;
    UINT     idComDev;
    PWOWPORT pWOWPort;
    register PSETCOMMBREAK16 parg16;

    GETARGPTR(pFrame, sizeof(SETCOMMBREAK16), parg16);

    idComDev = UINT32(parg16->f1);
    if (pWOWPort = GETPWOWPTR(idComDev)) {
        if(VALIDCOM(idComDev)) {
            if(!SetCommBreak(pWOWPort->h32)) {
                WOWGetCommError(pWOWPort);
            }
        }
        ul = pWOWPort->dwErrCode; // Win3.1 returns last err
    }

#ifdef DEBUG
    if(!(ul!=CE_MODE)) {
        LOGDEBUG(0,("WOW::WU32SetCommBreak: failed\n"));
    }
#endif

    FREEARGPTR(parg16);
    RETURN(ul);
}




// Win3.1 returns:
//    A 16:16 ptr into the DEB struct on success.
//    0 on any error OR LPT.
// The 16:16 ptr that we return to the app was actually obtained in
// IOpenComm() in user.exe.
ULONG FASTCALL WU32SetCommEventMask(PVDMFRAME pFrame)
{
    ULONG      ul = 0;
    BOOL       fOK = TRUE;
    UINT       idComDev;
    DWORD      dwDEBAddr;
    PWOWPORT   pWOWPort;
    register   PSETCOMMEVENTMASK16 parg16;

    GETARGPTR(pFrame, sizeof(SETCOMMEVENTMASK16), parg16);

    idComDev  = UINT32(parg16->f1);
    if ((VALIDCOM(idComDev)) && (pWOWPort = PortTab[idComDev].pWOWPort)) {

        // if the Modem interrupt thread hasn't been started yet -- go start it
        if(pWOWPort->hMiThread == NULL) {

            // start our Modem interrupt thread
            if(!WOWStartModemIntThread(pWOWPort)) {
                fOK = FALSE;
            }
        }

        // if everything is hunky-dory...
        if(fOK) {

            // success: Win3.1 returns 16:16 protect mode ptr to
            // DEB->EvtWord (some apps subtract offset of EvtWord
            // from ptr to get start of DEB).
            dwDEBAddr  = LOWORD(pWOWPort->dwComDEB16) << 16;
            ul = dwDEBAddr + FIELD_OFFSET(COMDEB16, EvtWord);

            // save the mask the app requested
            pWOWPort->lpComDEB16->EvtMask = (WORD)(parg16->f2);
        }
    }

#ifdef DEBUG
    if(!(ul!=0)) {
        LOGDEBUG(0,("WOW::WU32SETCOMMEVENTMASK: failed\n"));
    }
#endif

    FREEARGPTR(parg16);
    RETURN(ul);
}




// Win3.1 returns:
//    0 on success OR LPTx.
//    IE_BADID for bad idComDev.
//    IE_NOPEN if file hasn't been opened.
//    IE_BAUDRATE for bad baud rate.
//    IE_BYTESIZE for bad byte size.
//    IE_DEFAULT for bad parity or stop bits.
ULONG FASTCALL WU32SetCommState(PVDMFRAME pFrame)
{
    ULONG    ul = (ULONG)IE_BADID;
    UINT     idComDev;
    PDCB16   pdcb16;
    DCB      dcb32;
    PWOWPORT pWOWPort;
    register PSETCOMMSTATE16 parg16;
    DWORD    dwMSR;

    GETARGPTR(pFrame, sizeof(SETCOMMSTATE16), parg16);
    GETMISCPTR(parg16->f1, pdcb16);

    if(pdcb16) {

        idComDev = pdcb16->Id;
        if(pWOWPort = GETPWOWPTR(idComDev)) {

            if(VALIDCOM(idComDev)) {
                DCB16toDCB32(pWOWPort, &dcb32, pdcb16);

                if(SetCommState(pWOWPort->h32, &dcb32)) {
                    ul = 0;

                    // Win 3.1 initializes the MSRShadow during SetCommState
                    // so we will too. InterNet in a Box Dialer depends on it.
                    GetCommModemStatus(pWOWPort->h32, &dwMSR);
                    dwMSR &= MSR_STATEONLY;
                    pWOWPort->lpComDEB16->MSRShadow = LOBYTE(LOWORD(dwMSR));
                }
                else {
                    ul = (ULONG)IE_DEFAULT; // we just say something's wrong
                }

            }
            else {
                RtlCopyMemory((PVOID)pWOWPort->pdcb16,
                              (PVOID)pdcb16,
                              sizeof(DCB16));
                ul = 0;
            }
        }
        // else if they got a handle that looks good but they didn't open port
        else if (VALIDCOM(idComDev) || VALIDLPT(idComDev)) {
            ul = (ULONG)IE_NOPEN;
        }

        FREEMISCPTR(pdcb16);
    }


#ifdef DEBUG
    if(!(ul>=0)) {
        LOGDEBUG(0,("WOW::WU32SetCommState: failed\n"));
    }
#endif

    FREEARGPTR(parg16);
    RETURN(ul);
}




// Win3.1 returns:
//    0 for success.
//    0x8000 for bad idComDev.
//    0x4000 if char can't be sent.
ULONG FASTCALL WU32TransmitCommChar(PVDMFRAME pFrame)
{
    ULONG        ul = 0x8000;
    UINT         idComDev;
    CHAR         ch;
    PWOWPORT     pWOWPort;
    DWORD        cbWritten;
    register PTRANSMITCOMMCHAR16 parg16;

    GETARGPTR(pFrame, sizeof(TRANSMITCOMMCHAR16), parg16);

    idComDev = UINT32(parg16->f1);
    if (pWOWPort = GETPWOWPTR(idComDev)) {

        if(VALIDCOM(idComDev)) {
            if(TransmitCommChar(pWOWPort->h32, CHAR32(parg16->f2))) {
                ul = 0;  // Win3.1 returns 0 on success
            }
            else {
                ul = (ULONG)ERR_XMIT;
            }
        }

        // else LPT's go this way...
        else {

            //
            // This call to WriteFile could block, but I don't think
            // that's a problem.  - DaveHart
            //

            ch = CHAR32(parg16->f2);
            ul = ERR_XMIT;
            if (pWOWPort = GETPWOWPTR(UINT32(parg16->f1))) {

                if (!WriteFile(pWOWPort->h32, &ch, 1, &cbWritten, &pWOWPort->olWrite)) {

                    if (ERROR_IO_PENDING == GetLastError() ) {

                        //
                        // Wait for the write to complete or for us to
                        // be alerted that the port is closing.
                        //

                        if (GetOverlappedResult(pWOWPort->h32,
                                                &pWOWPort->olWrite,
                                                &cbWritten,
                                                TRUE
                                                )) {
                            ul = 0;
                            goto TransmitSuccess;
                        }
                    }
                    LOGDEBUG(0, ("WU32TransmitCommChar: WriteFile to id %u fails (error %u)\n",
                                 pWOWPort->idComDev, GetLastError()));
                }
                else {
                    ul = 0;
                }
            }

        }
    }
TransmitSuccess:

    FREEARGPTR(parg16);
    RETURN(ul);
}


// Win3.1 returns:
//    0 on success OR bad idComDev OR LPTx.
//    -1 if port not open OR if ungot char already pending.
ULONG FASTCALL WU32UngetCommChar(PVDMFRAME pFrame)
{
    ULONG    ul = (ULONG)-1;
    UINT     idComDev;
    PWOWPORT pWOWPort;
    register PUNGETCOMMCHAR16 parg16;

    GETARGPTR(pFrame, sizeof(UNGETCOMMCHAR16), parg16);

    // see if port open...
    idComDev = UINT32(parg16->f1);
    if (VALIDCOM(idComDev)) {

        if (pWOWPort = PortTab[idComDev].pWOWPort) {

            // if ungot char already pending return -1
            if(pWOWPort->fUnGet == FALSE) {
                pWOWPort->fUnGet = TRUE;
                pWOWPort->cUnGet = CHAR32(parg16->f2);
                ul = 0;
            }
        }
    }
    else {
        ul = 0;
    }

#ifdef DEBUG
    if(!(ul==0)) {
        LOGDEBUG(0,("WOW::WU32UngetCommChar: failed\n"));
    }
#endif

    FREEARGPTR(parg16);
    RETURN(ul);
}


DWORD Baud16toBaud32(UINT BaudRate)
{
    UINT DLatch;

    // this function is set up this way on purpose (see SetCom300 ibmsetup.asm)

    // get the equivalent baud
    switch(BaudRate) {

        // it they specified the baud rate directly
        case           CBR_110:
        case           CBR_300:
        case           CBR_600:
        case          CBR_1200:
        case          CBR_2400:
        case          CBR_4800:
        case          CBR_9600:
        case         CBR_19200:
        case         CBR_14400:
        case         CBR_38400:
        case         CBR_56000:   return(BaudRate);

        // Win3.1 baud rate constants
        case        W31CBR_110:   return(CBR_110);
        case        W31CBR_300:   return(CBR_300);
        case        W31CBR_600:   return(CBR_600);
        case       W31CBR_1200:   return(CBR_1200);
        case       W31CBR_2400:   return(CBR_2400);
        case       W31CBR_4800:   return(CBR_4800);
        case       W31CBR_9600:   return(CBR_9600);
        case      W31CBR_19200:   return(CBR_19200);
        case      W31CBR_14400:   return(CBR_14400);
        case      W31CBR_38400:   return(CBR_38400);
        case      W31CBR_56000:   return(CBR_56000);

        // start special cases
        // SmartCom uses this to get 115200
        case     W31CBR_115200:   return(CBR_115200);

        // Win3.1 fails these two (even though they're defined in windows.h)
        // but they just might work on NT
        case     W31CBR_128000:   return(CBR_128000);
        case     W31CBR_256000:   return(CBR_256000);
        // end special cases

        // handle the blank table entries for "reserved"
        case  W31CBR_reserved1:
        case  W31CBR_reserved2:
        case  W31CBR_reserved3:
        case  W31CBR_reserved4:
        case  W31CBR_reserved5:   return(0);

        // avoid divide by zero
        case                 0:
        case                 1:   return(0);

        // handle obscure specifications that will work in Win3.1
        default:

            // get the integer divisor latch value
            DLatch = CBR_115200 / BaudRate;

            switch(DLatch) {
                case    W31_DLATCH_110:   return(CBR_110);
                case    W31_DLATCH_300:   return(CBR_300);
                case    W31_DLATCH_600:   return(CBR_600);
                case   W31_DLATCH_1200:   return(CBR_1200);
                case   W31_DLATCH_2400:   return(CBR_2400);
                case   W31_DLATCH_4800:   return(CBR_4800);
                case   W31_DLATCH_9600:   return(CBR_9600);
                case  W31_DLATCH_19200:   return(CBR_19200);
                case  W31_DLATCH_14400:   return(CBR_14400);
                case  W31_DLATCH_38400:   return(CBR_38400);
                case  W31_DLATCH_56000:   return(CBR_56000);
                case W31_DLATCH_115200:   return(CBR_115200);

                // Win3.1, anything else returns whatever DLatch happens to be
                // since we're mapping to baud we return the specified baud
                default:   return(BaudRate);
            }
    }
}




WORD Baud32toBaud16(DWORD BaudRate)
{
    if(BaudRate >= CBR_115200) {
        switch(BaudRate) {
            case CBR_256000: return(W31CBR_256000);
            case CBR_128000: return(W31CBR_128000);
            case CBR_115200:
            default:         return(W31CBR_115200);
        }
    }
    else {
        return(LOWORD(BaudRate));
    }
}





void DCB16toDCB32(PWOWPORT pWOWPort, LPDCB lpdcb32, PDCB16 pdcb16)
{

    // zero 32-bit struct -> any flags and fields not explicitly set will be 0
    RtlZeroMemory((PVOID)lpdcb32, sizeof(DCB));

    lpdcb32->DCBlength         = sizeof(DCB);
    lpdcb32->BaudRate          = Baud16toBaud32(pdcb16->BaudRate);

    // 16-bit bitfields may align differently with 32-bit compilers
    // we use this mechanism to align them the way Win3.1 expects them
    if(pdcb16->wFlags & W31DCB_fBinary)       lpdcb32->fBinary      = 1;
    if(pdcb16->wFlags & W31DCB_fParity)       lpdcb32->fParity      = 1;
    if(pdcb16->wFlags & W31DCB_fOutxCtsFlow)  lpdcb32->fOutxCtsFlow = 1;
    if(pdcb16->wFlags & W31DCB_fOutxDsrFlow)  lpdcb32->fOutxDsrFlow = 1;

    // set up mechanism for handling event char notification
    if(pdcb16->wFlags & W31DCB_fChEvt) pWOWPort->fChEvt = TRUE;

    if(pdcb16->wFlags & W31DCB_fDtrFlow) {
        lpdcb32->fDtrControl = DTR_CONTROL_HANDSHAKE;
    }
    else if(pdcb16->wFlags & W31DCB_fDtrDisable) {
        lpdcb32->fDtrControl = DTR_CONTROL_DISABLE;
    }
    else {
        lpdcb32->fDtrControl = DTR_CONTROL_ENABLE;
    }

    if(pdcb16->wFlags & W31DCB_fOutX)         lpdcb32->fOutX        = 1;
    if(pdcb16->wFlags & W31DCB_fInX)          lpdcb32->fInX         = 1;
    if(pdcb16->wFlags & W31DCB_fPeChar)       lpdcb32->fErrorChar   = 1;
    if(pdcb16->wFlags & W31DCB_fNull)         lpdcb32->fNull        = 1;

    if(pdcb16->wFlags & W31DCB_fRtsFlow) {
        lpdcb32->fRtsControl = RTS_CONTROL_HANDSHAKE;
    }
    else if(pdcb16->wFlags & W31DCB_fRtsDisable) {
        lpdcb32->fRtsControl = RTS_CONTROL_DISABLE;
    }
    else {
        lpdcb32->fRtsControl = RTS_CONTROL_ENABLE;
    }

    if(pdcb16->wFlags & W31DCB_fDummy2)       lpdcb32->fDummy2      = 1;

    // Check the passed in XonLim & XoffLim values against the cbInQ value.
    // Prodigy's modem detector leaves these values uninitialized.
    if ((pdcb16->XonLim  >= pWOWPort->cbInQ) ||
        (pdcb16->XoffLim >  pWOWPort->cbInQ) ||
        (pdcb16->XonLim  >= pdcb16->XoffLim)) {
        lpdcb32->XonLim = 0;
        lpdcb32->XoffLim = (WORD)(pWOWPort->cbInQ - (pWOWPort->cbInQ >> 2));
    }
    else {
        lpdcb32->XonLim  = pdcb16->XonLim;
        lpdcb32->XoffLim = pdcb16->XoffLim;
    }

    lpdcb32->ByteSize          = pdcb16->ByteSize;
    lpdcb32->Parity            = pdcb16->Parity;
    lpdcb32->StopBits          = pdcb16->StopBits;

    // Digiboard driver doesn't want to see XonChar == XoffChar even if
    // xon/xoff is disabled.
    if ((pdcb16->XonChar == '\0') && (lpdcb32->XoffChar == '\0')) {
        lpdcb32->XonChar = pdcb16->XonChar+1;
    }
    else {
        lpdcb32->XonChar = pdcb16->XonChar;
    }

    lpdcb32->XoffChar          = pdcb16->XoffChar;
    lpdcb32->ErrorChar         = pdcb16->PeChar;
    lpdcb32->EofChar           = pdcb16->EofChar;
    lpdcb32->EvtChar           = pdcb16->EvtChar;

#ifdef FE_SB
// for MSKKBUG #3213 by v-kenich
// MYTALK for Win set NULL these two fields at transfering binary file
// If call SetCommstate as it is, SetCommState return error (Invalid parameter)
// I think this fix doesn't occur any bad thing without condition of MYTALK
// Really correcting parameter check is better. but I don't know where it is.

    if (!lpdcb32->XonChar) lpdcb32->XonChar = 0x11;
    if (!lpdcb32->XoffChar) lpdcb32->XoffChar = 0x13;
#endif // FE_SB

    // set up for RLSD, CTS, and DSR timeout support (not supported on NT)
    pWOWPort->lpComDEB16->MSRMask = 0;

    pWOWPort->RLSDTimeout = pdcb16->RlsTimeout;
    if(pWOWPort->RLSDTimeout != IGNORE_TIMEOUT)
        pWOWPort->lpComDEB16->MSRMask |= LOBYTE(MS_RLSD_ON);

    pWOWPort->CTSTimeout = pdcb16->CtsTimeout;
    if(pWOWPort->CTSTimeout != IGNORE_TIMEOUT)
        pWOWPort->lpComDEB16->MSRMask |= LOBYTE(MS_CTS_ON);

    pWOWPort->DSRTimeout = pdcb16->DsrTimeout;
    if(pWOWPort->DSRTimeout != IGNORE_TIMEOUT)
        pWOWPort->lpComDEB16->MSRMask |= LOBYTE(MS_DSR_ON);

    // these fields remain 0
    //lpdcb32->fDsrSensitivity   = 0;
    //lpdcb32->fTXContinueOnXoff = 0;
    //lpdcb32->fAbortOnError     = 0;
    //lpdcb32->wReserved         = 0;

}



void DCB32toDCB16(PDCB16 pdcb16, LPDCB lpdcb32, UINT idComDev, BOOL fChEvt)
{

    // zero 16-bit struct -> any flags and fields not explicitly set will be 0
    RtlZeroMemory((PVOID)pdcb16, sizeof(DCB16));

    // set this field no matter what
    pdcb16->Id = (BYTE)idComDev;

    // if a COMx (Win3.1 leaves the rest 0 for LPT's)
    if(VALIDCOM(idComDev)) {
        pdcb16->Id = (BYTE)idComDev;

        // these are the "ComX:96,n,8,1" fields
        pdcb16->BaudRate        = Baud32toBaud16(lpdcb32->BaudRate);
        pdcb16->ByteSize        = lpdcb32->ByteSize;
        pdcb16->Parity          = lpdcb32->Parity;
        pdcb16->StopBits        = lpdcb32->StopBits;

        // 16-bit bitfields may align differently with 32-bit compilers
        // we use this mechanism to align them the way Win3.1 expects them
        if(lpdcb32->fBinary)      pdcb16->wFlags |= W31DCB_fBinary;

        if(lpdcb32->fRtsControl == RTS_CONTROL_DISABLE) {
            pdcb16->wFlags |= W31DCB_fRtsDisable;
        }

        if(lpdcb32->fParity)      pdcb16->wFlags |= W31DCB_fParity;
        if(lpdcb32->fOutxCtsFlow) pdcb16->wFlags |= W31DCB_fOutxCtsFlow;
        if(lpdcb32->fOutxDsrFlow) pdcb16->wFlags |= W31DCB_fOutxDsrFlow;

        if(lpdcb32->fDtrControl == DTR_CONTROL_DISABLE) {
            pdcb16->wFlags |= W31DCB_fDtrDisable;
        }

        if(lpdcb32->fOutX)        pdcb16->wFlags |= W31DCB_fOutX;
        if(lpdcb32->fInX)         pdcb16->wFlags |= W31DCB_fInX;
        if(lpdcb32->fErrorChar)   pdcb16->wFlags |= W31DCB_fPeChar;
        if(lpdcb32->fNull)        pdcb16->wFlags |= W31DCB_fNull;

        if(fChEvt)                pdcb16->wFlags |= W31DCB_fChEvt;

        if(lpdcb32->fDtrControl == DTR_CONTROL_HANDSHAKE) {
            pdcb16->wFlags |= W31DCB_fDtrFlow;
        }

        if(lpdcb32->fRtsControl == RTS_CONTROL_HANDSHAKE) {
            pdcb16->wFlags |= W31DCB_fRtsFlow;
        }

        if(lpdcb32->fDummy2)      pdcb16->wFlags |= W31DCB_fDummy2;

        pdcb16->XonChar         = lpdcb32->XonChar;
        pdcb16->XoffChar        = lpdcb32->XoffChar;
        pdcb16->XonLim          = lpdcb32->XonLim;
        pdcb16->XoffLim         = lpdcb32->XoffLim;
        pdcb16->PeChar          = lpdcb32->ErrorChar;
        pdcb16->EofChar         = lpdcb32->EofChar;
        pdcb16->EvtChar         = lpdcb32->EvtChar;

    }

    // these fields remain 0
    //pdcb16->fDummy  = 0;
    //pdcb16->TxDelay = 0;

}




BOOL DeletePortTabEntry(PWOWPORT pWOWPort)
{
    INT      iTab;
    BOOL     fTimeOut;

    iTab = pWOWPort->idComDev;
    if(VALIDLPT(iTab)) {
        iTab = GETLPTID(iTab);
    }

    // flush I/O buffers & attempt to wake up Modem Interrupt thread (if any)
    pWOWPort->fClose = TRUE;
    if(VALIDCOM(iTab)) {
        PurgeComm(pWOWPort->h32, PURGE_TXCLEAR);
        PurgeComm(pWOWPort->h32, PURGE_RXCLEAR);
        SetCommMask(pWOWPort->h32, 0); // this should wake up the Mi thread

        // wake up WOWModemIntThread & tell it to exit
        // (we attempt to block (1.5 second max.) until it does)
        if(pWOWPort->hMiThread) {
            WaitForSingleObject(pWOWPort->hMiThread, 1500);
            CloseHandle(pWOWPort->hMiThread);

            // zero COMDEB
            RtlZeroMemory((PVOID)pWOWPort->lpComDEB16, sizeof(COMDEB16));
        }

        //
        // Wake up WOWCommWriterThread so it will exit, wait up to
        // 5 sec for it to go away.
        //

        SetEvent(pWOWPort->hWriteEvent);

        fTimeOut = (WaitForSingleObject(pWOWPort->hWriteThread, 5000) ==
                    WAIT_TIMEOUT);

#ifdef DEBUG
        if (fTimeOut) {
            LOGDEBUG(LOG_ALWAYS,
                ("WOW DeletePortTabEntry: Comm writer thread for port %d refused\n"
                 "    to die when asked nicely.\n", (int)pWOWPort->idComDev));
        }
#endif

        CloseHandle(pWOWPort->hWriteThread);
        CloseHandle(pWOWPort->hWriteEvent);
        free_w(pWOWPort->pchWriteBuf);

        CloseHandle(pWOWPort->hREvent);
    }
    // else free the LPT DCB support struct
    else {
        free_w(pWOWPort->pdcb16);
        CloseHandle(pWOWPort->olWrite.hEvent);
        fTimeOut = FALSE;
    }

    DeleteCriticalSection(&pWOWPort->csWrite);
    CloseHandle(pWOWPort->h32);

    // QuickLink Gold 1.3 hack.  Bug #398011
    // The app calls OpenComm(), & then SetCommEventMask() to get the ptr to the
    // comdeb16 struct.  It saves the ptr at offset 0xf36 on its stack.  The
    // problem is that the app holds onto the comdeb16 ptr after it calls
    // CloseComm() (when we free the comdeb16 memory) to be able to peek at a
    // status byte from time to time.  This works OK on Win 3.1 but not with
    // our model on NT.  Fortunately, the app tests to see if it has a comdeb16
    // ptr before dereferencing it.  Also, we're lucky because the ptr for
    // lpszDevControl in its call to OpenComm() is from its stack thus allowing
    // us to obtain the stack selector and zero out the comdeb16 ptr stored at
    // stack ss:0xf36 when the app calls CloseComm().
    if(pWOWPort->QLStackSeg) {
        LPDWORD lpQLS;
        VPVOID  vpQLS, vpCD16;

        // construct the 16:16 ptr to where the app saved the ptr to the 
        // COMDEB16 struct on its stack at offset 0xf36
        vpQLS = pWOWPort->QLStackSeg & 0xFFFF0000;
        vpQLS = vpQLS | 0x00000f36;

        GETMISCPTR(vpQLS, lpQLS);    

        // construct realmode 16:16 ptr of the COMDEB16 struct + 0x38 (seg:0x38)
        vpCD16 = pWOWPort->QLStackSeg & 0x0000FFFF;
        vpCD16 = (vpCD16 << 16) | 0x00000038;

        if(lpQLS) {

            // sanity check to see if everything is still what & where we 
            // think it is

            // if seg:0x38 is still stored at offset 0xf36 on the apps stack...
            if(*lpQLS == (DWORD)vpCD16) {

                // zero it out -- forcing app to avoid checking the status byte
                *lpQLS = 0;

                FLUSHVDMPTR(vpQLS, sizeof(DWORD), lpQLS);
                FREEMISCPTR(lpQLS);
            }
        }
    }

    free_w(pWOWPort);
    PortTab[iTab].pWOWPort = NULL;

    return(fTimeOut);
}



UINT GetModePortTabIndex(PSZ pszModeStr)
{

    CHAR  szPort[MAXCOMNAMENULL*2];

    if(pszModeStr) {
        if(GetPortName(pszModeStr, szPort)) {
            return(GetStrPortTabIndex(szPort));
        }
    }

    return((UINT)IE_BADID);

}



BOOL GetPortName(LPSTR pszMode, LPSTR pszPort)
{

    INT   len;
    CHAR  szTemp[80];  // max len we'll take for DOS style MODE command
    BOOL  bRet = FALSE;

    len = strlen(pszMode);
    if((len >= 3) && (len < 80)) {

        // Get the first token from the mode string.
        GetPortStringToken(pszMode, szTemp);

        // map "AUX" or "PRN" to "COM1" or "LPT1" if necessary
        len = strlen(szTemp);
        if((len >= 3) && (len <= MAXCOMNAME)) {  //  "AUX" <= len <= "COMx"

            strcpy(pszPort, szTemp);
            CharUpper(pszPort);

            // filter out duplicate names for the same thing
            if(!WOW32_strcmp(pszPort, "PRN")) {
                strcpy(pszPort, "LPT1");
            }
            else if(!WOW32_strcmp(pszPort, "AUX")) {
                strcpy(pszPort, "COM1");
            }

            bRet = TRUE;
        }
    }

    return(bRet);

}

PSZ StripPortName(PSZ psz)
{
    CHAR dummy[80];  // max len we'll take for DOS style MODE command

    return(GetPortStringToken(psz, dummy));
}

//
// Copy first token to pszToken. Return pointer to next token or NULL if none.
// This code cloned from Win 3.1, COMDEV.C, field(). HGW 3.0 modem registration
// passes "COMx,,," instead of "COMx:,,," so we need to handle all seperators.
//

PSZ GetPortStringToken(PSZ pszSrc, PSZ pszToken)
{
    char   c;

    // While not the end of the string.
    while (c = *pszSrc) {
        pszSrc++;

        //Look for seperators.
        if ((c == ' ') || (c == ':') || (c == ',')) {
            *pszToken = '\0';

            while (*pszSrc == ' ') {
                pszSrc++;
            }

            if (*pszSrc) {
                return(pszSrc);
            }

            return(NULL);
        }

      *pszToken++ = c;
    }

    *pszToken = '\0';

    return(NULL);
}


UINT GetStrPortTabIndex(PSZ szPort)
{
    UINT  iTab;

    for(iTab = COM1; iTab < NUMPORTS; iTab++) {
        if(!WOW32_strcmp((LPCTSTR)PortTab[iTab].szPort, (LPCTSTR)szPort)) {
            return(iTab);
        }
    }

    return((UINT)IE_BADID);
}



BOOL InitDCB32(LPDCB pdcb32, LPSTR pszModeStr)
{
    BOOL   bRet = FALSE;
    LPSTR  pszParams;

    // eliminate "COMx:" from mode string leaving ptr to parameters string
    pszParams = StripPortName(pszModeStr);

    // if there are params...  (some apps pass "com1:" -- hence 2nd test)
    if(pszParams) {

        // initialize everything to 0 (especially the flags)
        RtlZeroMemory((PVOID)pdcb32, sizeof(DCB));

        // NOTE: 32-bit BuildCommDCB ONLY touches fields associated with psz1
        if(BuildCommDCB(pszParams, pdcb32)) {

            pdcb32->DCBlength = sizeof(DCB);

            // fill in specific fields a la Win3.1
            // NOTE: fields are 0 unless explicitly set
            pdcb32->fBinary     = 1;
            pdcb32->fDtrControl = DTR_CONTROL_ENABLE; //same as fDTRDisable == 0
            pdcb32->fRtsControl = RTS_CONTROL_ENABLE; //same as fRTSDisable == 0

            pdcb32->XonLim     = 10;
            pdcb32->XoffLim    = 10;
            pdcb32->XonChar    = 0x11;      // Ctrl-Q
            pdcb32->XoffChar   = 0x13;      // Ctrl-S

            bRet = TRUE;
        }
    }

    return(bRet);
}



VOID InitDEB16(PCOMDEB16 pComDEB16, UINT iTab, WORD QInSize, WORD QOutSize)
{
    VPVOID  vpBiosData;
    PWORD16 pwBiosData;

    // Win3.1 init's most the stuff to zero except as handled below
    RtlZeroMemory((PVOID)pComDEB16, sizeof(COMDEB16));

    // get the I/O base address for the port
    vpBiosData = (VPVOID)(RM_BIOS_DATA + (iTab * sizeof(WORD)));
    if(pwBiosData = (PWORD16)GetRModeVDMPointer(vpBiosData)) {
        pComDEB16->Port = (WORD)*pwBiosData;
        FREEVDMPTR(pwBiosData);
    }

    pComDEB16->RecvTrigger = (WORD)-1;
    pComDEB16->QInSize     = QInSize;
    pComDEB16->QOutSize    = QOutSize;

}

/* start thread for Modem interrupt emulation */
BOOL WOWStartModemIntThread(PWOWPORT pWOWPort)
{
    BOOL       ret = FALSE;
    DWORD      dwUnused;
    HANDLE     hEvent, hMiThread;

    // set up temporary semaphore to sync with Modem interrupt thread
    if((hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL) {
        goto ErrorExit0;
    }

    // use pWOWPort->hMiThread temporarily to help start the thread
    pWOWPort->hMiThread = hEvent;

    // create the MSR thread
    if((hMiThread = CreateThread(NULL,
                                 8192,
                                 (LPTHREAD_START_ROUTINE)WOWModemIntThread,
                                 (PWOWPORT)pWOWPort,
                                 0,
                                 (LPDWORD)&dwUnused))  == NULL) {
        goto ErrorExit1;
    }

    // block until thread notifies us that it has started
    WaitForSingleObject(hEvent, INFINITE);

    pWOWPort->hMiThread = hMiThread;

    CloseHandle(hEvent);
    ret = TRUE;

    goto FunctionExit;


// this is the error code path
ErrorExit1:
    CloseHandle(hEvent);

ErrorExit0:
    pWOWPort->hMiThread  = NULL;


FunctionExit:
#ifdef DEBUG
    if(!(ret)) {
        LOGDEBUG(0,("WOW::W32StartModemIntThread failed\n"));
    }
#endif
    return(ret);

}



// Modem Interrupt thread for SetCommEventMask/EnableCommNotification support
// Tries to emulate the interrupt handling in ibmint.asm of Win3.1 comm.drv.
// Our "interrupts" here are the events from the NT serial comm stuff
VOID WOWModemIntThread(PWOWPORT pWOWPort)
{
    BOOL       fRing     = FALSE;
    UINT       iTab;
    DWORD      dwRing;
    DWORD      dwEvts    = 0;
    DWORD      dwEvtOld  = 0;
    DWORD      dwEvtWord = 0;
    DWORD      dwMSR     = 0;
    DWORD      dwErrCode = 0;
    DWORD      cbTransfer;
    HANDLE     h32;
    PCOMDEB16  lpComDEB16;
    OVERLAPPED ol;

    iTab       = pWOWPort->idComDev;
    lpComDEB16 = pWOWPort->lpComDEB16;
    h32        = pWOWPort->h32;

    // set the current modem status & Event word
    lpComDEB16->MSRShadow    = (BYTE)0;
    lpComDEB16->EvtWord      = (WORD)0;
    lpComDEB16->ComErr       = (WORD)0;
    lpComDEB16->QInCount     = (WORD)0;
    lpComDEB16->QOutCount    = (WORD)0;

    if(VALIDLPT(iTab)) {
        iTab = GETLPTID(iTab);
    }

    ol.Internal     = 0;
    ol.InternalHigh = 0;
    ol.Offset       = 0;
    ol.OffsetHigh   = 0;
    ol.hEvent       = CreateEvent(NULL,
                                  TRUE,
                                  FALSE,
                                  (LPTSTR)PortTab[iTab].szPort);

    // activate modem events in the mask, we want to emulate all the interrupts
    SetCommMask(h32, EV_NTEVENTS);

    // initialize the shadow MSR
    GetCommModemStatus(h32, &dwMSR);
    dwMSR &= MSR_STATEONLY;
    lpComDEB16->MSRShadow = LOBYTE(LOWORD(dwMSR));

    // wake up the thread that created this thread in WOWStartModemIntThread()
    SetEvent(pWOWPort->hMiThread);

    while(!pWOWPort->fClose) {

        // wait for an event - hopefully this will be somewhat similar to
        // the TimerProc in ibmint.asm which gets called every 100ms
        if(!WaitCommEvent(h32, &dwEvts, &ol)) {

            if(GetLastError() == ERROR_IO_PENDING) {

                // ...block here 'til event specified in WaitCommEvent() occurs
                if(!GetOverlappedResult(h32, &ol, &cbTransfer, TRUE)) {
                    LOGDEBUG(0, ("WOW::WUCOMM: WOWModemIntThread: Wait failed\n"));
                }
            }
            else {
                LOGDEBUG(0, ("WOW::WUCOMM: WOWModemIntThread : Overlap failed\n"));
            }
        }
        ResetEvent(ol.hEvent);

        // Get current MSR state, current state of delta bits isn't accurate for us
        GetCommModemStatus(h32, &dwMSR);

        dwMSR &= MSR_STATEONLY;  // throw away delta bits


        // set the DELTA bits in the shadow MSR
        if(dwEvts & EV_CTS)  dwMSR |= MSR_DCTS;

        if(dwEvts & EV_DSR)  dwMSR |= MSR_DDSR;

        if(dwEvts & EV_RLSD) dwMSR |= MSR_DDCD;

        if(dwEvts & EV_RING) {
            fRing      = TRUE;
            dwRing = EV_RING;
        }
        else if(fRing) {
            fRing      = FALSE;
            dwMSR     |= MSR_TERI;
            dwRing = EV_RingTe;
        }
        else {
            dwRing = 0;
        }

        // Form the events
        dwEvtOld  = (DWORD)lpComDEB16->EvtWord;
        dwEvtWord = 0;
        dwEvtWord = dwRing | (dwEvts & (EV_ERR | EV_BREAK | EV_RXCHAR | EV_TXEMPTY | EV_CTS | EV_DSR | EV_RLSD | EV_RXFLAG));

        // we have to figure the state bits out from the MSR

        if(dwMSR & MS_CTS_ON) dwEvtWord |= EV_CTSS;
        if(dwMSR & MS_DSR_ON) dwEvtWord |= EV_DSRS;
        if(dwMSR & MS_RLSD_ON) dwEvtWord |= EV_RLSDS;

        // One of the major tasks of this routine is to update the MSRShadow
        // and EvtWord in the COMDEB16 structure.
        //

        //apply the msr as well
        lpComDEB16->MSRShadow = LOBYTE(LOWORD(dwMSR));

        // apply the event mask the app specified
        lpComDEB16->EvtWord |= LOWORD(dwEvtWord) & lpComDEB16->EvtMask;

        // The following code simluates the COM Notifcation functionality of
        // Win 3.1.
        //
        // Notifications:
        //
        // if they want receive transmit notification & it's time to notify
        // if there wasn't an Rx overflow continue...

        if( lpComDEB16->NotifyHandle ) {

            // get current error code & queue counts
            WOWGetCommError(pWOWPort);

            if((dwEvtWord & ( EV_RXCHAR | EV_RXFLAG )) &&
               !(pWOWPort->dwErrCode & CE_RXOVER)) {

                // if they want receive notification & it's time to notify
                // apps should set RecvTrigger to -1 if they don't want notification
                if((((SHORT)lpComDEB16->RecvTrigger) != -1) &&
                   (lpComDEB16->QInCount >= lpComDEB16->RecvTrigger)) {

                    // if the app hasn't already been notified of this ...
                    if(!(lpComDEB16->NotifyFlags & CN_RECEIVEHI)) {

                        PostMessage(HWND32(lpComDEB16->NotifyHandle),
                                    WOW_WM_COMMNOTIFY,
                                    MAKEWPARAM((WORD)pWOWPort->idComDev, 0),
                                    MAKELPARAM(CN_RECEIVE, 0));

                        lpComDEB16->NotifyFlags |= CN_RECEIVEHI;
                    }
                }
                else {
                    lpComDEB16->NotifyFlags &= ~CN_RECEIVEHI;
                }
            }

            // if they want receive transmit notification & it's time to notify
            if(lpComDEB16->QOutCount < (SHORT)lpComDEB16->SendTrigger) {

                // if the app hasn't already been notified of this ...
                if(!(lpComDEB16->NotifyFlags & CN_TRANSMITHI)) {

                    PostMessage(HWND32(lpComDEB16->NotifyHandle),
                                WOW_WM_COMMNOTIFY,
                                MAKEWPARAM((WORD)pWOWPort->idComDev, 0),
                                MAKELPARAM(CN_TRANSMIT, 0));

                    lpComDEB16->NotifyFlags |= CN_TRANSMITHI;
                }
            }
            else {
                lpComDEB16->NotifyFlags &= ~CN_TRANSMITHI;
            }

            // if we are notifying the app of EV_ event's
            if((lpComDEB16->NotifyFlags & CN_NOTIFYHI) &&
               ((DWORD)lpComDEB16->EvtWord != dwEvtOld)) {

                PostMessage(HWND32(lpComDEB16->NotifyHandle),
                            WOW_WM_COMMNOTIFY,
                            MAKEWPARAM((WORD)pWOWPort->idComDev, 0),
                            MAKELPARAM(CN_EVENT, 0));
            }

            // Now that we've processed all the interrupts, do the TimerProc.
            // if we are notifying the app of anything in Rx queue
            // this mimics the notification in the TimerProc (see ibmint.asm)
            if(((SHORT)lpComDEB16->RecvTrigger != -1) &&
                    (lpComDEB16->QInCount != 0)            &&
                    (!(lpComDEB16->NotifyFlags & CN_RECEIVEHI))) {

                PostMessage(HWND32(lpComDEB16->NotifyHandle),
                            WOW_WM_COMMNOTIFY,
                            MAKEWPARAM((WORD)pWOWPort->idComDev, 0),
                            MAKELPARAM(CN_RECEIVE, 0));

                lpComDEB16->NotifyFlags |= CN_RECEIVEHI;
            }
        }

        // we've handled all interrupts, give control back to app
        Sleep(0);

    } // end thread loop

    CloseHandle(ol.hEvent);

    ExitThread(0);
}



DWORD WOWGetCommError(PWOWPORT pwp)
{
    COMSTAT  cs;
    DWORD    dwErr;

    ClearCommError(pwp->h32, &dwErr, &cs);

    EnterCriticalSection(&pwp->csWrite);

    //
    // We do our own write buffering so we ignore
    // the cbOutQue returned by ClearCommError, which
    // only reflects pending writes.
    //
    // Number of bytes in our write queue is calculated
    // using the size of the queue and the amount free
    // in the queue, minus one.  Minus one because
    // there's one slot in the queue which is never used.
    //

    cs.cbOutQue = (pwp->cbWriteBuf - pwp->cbWriteFree) - 1;


    LeaveCriticalSection(&pwp->csWrite);


    // always update the status & preserve any error condition
    pwp->cs                  = cs;
    pwp->dwErrCode          |= dwErr;
    pwp->lpComDEB16->ComErr |= LOWORD(dwErr);

    // always update the queue counts in the DEB
    pwp->lpComDEB16->QInCount  = LOWORD(cs.cbInQue);
    pwp->lpComDEB16->QOutCount = LOWORD(cs.cbOutQue);

    return(dwErr);
}



/* for hung/crashed app support */
VOID FreeCommSupportResources(DWORD dwThreadID)
{
    UINT     iTab;
    PWOWPORT pWOWPort;

    for(iTab = 0; iTab < NUMPORTS; iTab++) {
        if(pWOWPort = PortTab[iTab].pWOWPort) {
            if(pWOWPort->dwThreadID == dwThreadID) {
                DeletePortTabEntry(pWOWPort);
                break;
            }
        }
    }
}



/* functions exported to the VDM */
/* NOTE: idComDev: COM1 == 0, LPT1 == 0x80 */
BYTE GetCommShadowMSR(WORD idComDev)
{
    BYTE      MSR=0;
    DWORD     dwModemStatus;
    PWOWPORT  pWOWPort;

    if (pWOWPort = GETPWOWPTR (idComDev)) {

        if(pWOWPort->hMiThread) {
            MSR = (BYTE)pWOWPort->lpComDEB16->MSRShadow;
        }
        else {  // get it the slow way if SetCommEventMask() hasn't been called
            GetCommModemStatus(pWOWPort->h32, &dwModemStatus);
            MSR = (BYTE)LOBYTE(LOWORD(dwModemStatus));
        }
    }

    return(MSR);
}



/* NOTE: idComDev: COM1 == 0, LPT1 == 0x80 */
HANDLE GetCommHandle(WORD idComDev)
{
    PWOWPORT  pWOWPort;

    if (pWOWPort = GETPWOWPTR (idComDev)) {
        return(pWOWPort->h32);
    }

    else {
        return(NULL); // will return NULL if bad range of idComDev or if the
    }                 // port wasn't initialized through an OpenComm() API call
}



BOOL IsQLinkGold(WORD wTDB)
{
   PTDB  pTDB;

   pTDB = (PVOID)SEGPTR(wTDB,0);
   if(WOW32_stricmp(pTDB->TDB_ModName, "QLGOLD")) {
       return(FALSE);
   }

   return(TRUE);
}


//
// EnqueueCommWrite - stuff characters into the Comm Write queue
//                    assoicated with pWOWPort.
//
// Returns number of characters queued.
//
// This function takes care of entering/leaving the critsec.
//

USHORT EnqueueCommWrite(PWOWPORT pwp, PUCHAR pch, USHORT cb)
{
    USHORT cbWritten = 0;
    USHORT cbToCopy;
    USHORT cbChunk;
    BOOL   fQueueEmpty;
    BOOL   fDelay = FALSE;

    // WinFax Lite 3 calls WriteFile("AT+FCLASS=1") to set the modem to fax mode
    // when it is recieving a fax.  Some modems appear to be slow to repsond
    // with the "OK" string (especially since we enqueue the "AT+FCLASS=1" write
    // and then write it in overlapped mode) -- so, when we tell the app we sent
    // it, it then follows with a "ATA" string without waiting for the modem's 
    // response to the previous command.  This confuses several different modems
    // and so they never answer.  This mechanism allows us to synchronize the
    // "AT+FCLASS=1" command so that it works more like Win3.1.  See bug #9479
    if(cb == 12) {

        // Handy way to say:
        // if(pch[0]=='A' && pch[1]=='T' && pch[2]=='+' && pch[3]=='F') {
        if((*(DWORD *)pch) == 0x462b5441) {

            // if(pch[0]=='C' && pch[1]=='L' && pch[2]=='A' && pch[3]=='S') {
            if((*(DWORD *)(pch+sizeof(DWORD))) == 0x53414c43) {

                // if(pch[0]=='S' && pch[1]=='=') {
                if((*(WORD *)(pch+(2*sizeof(DWORD)))) == 0x3D53) {
                    fDelay = TRUE;
    }   }   }   }

    EnterCriticalSection(&pwp->csWrite);

    fQueueEmpty = (pwp->pchWriteHead == pwp->pchWriteTail);

    //
    // cbToCopy is the total number of bytes that we are going to enqueue
    //

    cbToCopy = min(cb, pwp->cbWriteFree);

    //
    // Any write can be accomplished in at most two chunks.
    // The first writes up until the buffer wraps, while
    // the second starts at the beginning of the buffer.
    //
    // Do the first half, which may do it all.
    //
    // Number of bytes for the first chunk is the smaller of
    // the total number of bytes free in the write buffer and
    // the number of bytes free before the end of the buffer.
    //

    cbChunk = min(cbToCopy,
                  (pwp->pchWriteBuf + pwp->cbWriteBuf) - pwp->pchWriteTail);

    RtlCopyMemory(pwp->pchWriteTail, pch, cbChunk);
    pwp->cbWriteFree -= cbChunk;
    pwp->pchWriteTail += cbChunk;
    cbWritten += cbChunk;

    //
    // Tail pointer may have moved to point just beyond the buffer.
    //

    if (pwp->pchWriteTail >= pwp->pchWriteBuf + pwp->cbWriteBuf) {
        WOW32ASSERT(pwp->pchWriteTail == pwp->pchWriteBuf + pwp->cbWriteBuf);
        pwp->pchWriteTail = pwp->pchWriteBuf;
    }

    //
    // Are we done?
    //

    if (cbWritten < cbToCopy) {

        //
        // I think this case should only be taken when we've wrapped, so
        // be sure.
        //
        WOW32ASSERT(pwp->pchWriteTail == pwp->pchWriteBuf);

        //
        // Nope, do the second half.
        //

        cbChunk = min((cbToCopy - cbWritten), pwp->cbWriteFree);

        RtlCopyMemory(pwp->pchWriteTail, pch + cbWritten, cbChunk);
        pwp->cbWriteFree -= cbChunk;
        pwp->pchWriteTail += cbChunk;
        cbWritten += cbChunk;

        WOW32ASSERT(pwp->pchWriteTail < pwp->pchWriteBuf + pwp->cbWriteBuf);

    }


    //
    // If the buffer was empty to start with and we made it
    // non-empty, issue the first WriteFile and signal the
    // writer thread to wake up.
    //

    if (fQueueEmpty && cbWritten) {

        pwp->cbWritePending = CALC_COMM_WRITE_SIZE(pwp);

        if (!WriteFile(pwp->h32, pwp->pchWriteHead, pwp->cbWritePending,
                       &pwp->cbWritten, &pwp->olWrite)) {

            if (ERROR_IO_PENDING == GetLastError()) {
                pwp->fWriteDone = FALSE;
            } else {
                pwp->fWriteDone = TRUE;
                LOGDEBUG(0, ("WOW EnqueueCommWrite: WriteFile to id %u fails (error %u)\n",
                             pwp->idComDev, GetLastError()));
            }

        } else {
            pwp->fWriteDone = TRUE;
        }

        //
        // Leave the critical section before setting the event.  Otherwise
        // the other thread could wake up when the event is set and immediately
        // block on the critical section.
        //

        LeaveCriticalSection(&pwp->csWrite);
 
        // avoid setting the event twice
        if(!fDelay) {
            SetEvent(pwp->hWriteEvent);
        }

    } else {
        LeaveCriticalSection(&pwp->csWrite);
    }

    // this gives the writer thread a chance to write out "AT+FCLASS=1" strings
    if(fDelay) {
        SetEvent(pwp->hWriteEvent);
        Sleep(1000);
    }

    return cbWritten;
}



//
// WOWCommWriteThread created for COM ports only.  This thread dequeues
// characters from the write buffer and writes them to the COM port.
// This thread uses pwp->hWriteEvent for two purposes:
//
//   1. The event is signalled by EnqueueCommWrite when the write
//      buffer had been empty but is not now.  This wakes us up
//      so we can write to the port.  Note that we will always
//      be in the WaitForSingleObject at the top of the function
//      in this case, since that's where we sleep when the buffer
//      is empty.
//
//   2. DeletePortTabEntry signals the event after setting
//      pwp->fClose to tell us the port is closing and we
//      need to clean up and terminate this thread.  This
//      thread might be doing anything in this case, but
//      it is careful to check pwp->fClose before sleeping
//      again.
//
//   3. wu32FlushComm() signals the event and marks the queue empty

ULONG WOWCommWriterThread(LPVOID pWOWPortStruct)
{
    PWOWPORT   pwp = (PWOWPORT)pWOWPortStruct;
    HANDLE     ah[2];

    //
    // Copy event handles into array for WaitForMultipleObjects.
    //

    ah[0] = pwp->hWriteEvent;
    ah[1] = pwp->olWrite.hEvent;

WaitForWriteOrder:

    //
    // pwp->fClose is TRUE when the port is closed.
    //

    while (!pwp->fClose) {

        //
        // First wait for something to be written to the buffer.
        //

        WaitForSingleObject(pwp->hWriteEvent, INFINITE);

        //
        // Critical section protects write buffer.
        //

        EnterCriticalSection(&pwp->csWrite);

        //
        // The buffer is empty when head == tail.
        //

        while (pwp->pchWriteHead != pwp->pchWriteTail) {

            //
            // pwp->cbWritePending will be nonzero if
            // the application thread queued a write to
            // an empty buffer and then issued the first
            // WriteFile call.
            //

            if (pwp->cbWritePending) {
                if (!pwp->fWriteDone) {
                    LeaveCriticalSection(&pwp->csWrite);
                    goto WaitForWriteCompletion;
                } else {
                    goto CleanupAfterWriteComplete;
                }
            }

            pwp->cbWritePending = CALC_COMM_WRITE_SIZE(pwp);

            //
            // Leave the critical section before writing.  This is
            // safe because the app thread doesn't change the
            // head pointer.   (Not true if wu32FlushComm was called)
            //

            LeaveCriticalSection(&pwp->csWrite);

            if (!WriteFile(pwp->h32, pwp->pchWriteHead, pwp->cbWritePending,
                           &pwp->cbWritten, &pwp->olWrite)) {

                if (ERROR_IO_PENDING == GetLastError() ) {

WaitForWriteCompletion:
                    //
                    // Wait for the write to complete or for us to
                    // be alerted that the port is closing.
                    //

                    while (WAIT_OBJECT_0 == WaitForMultipleObjects(2, ah, FALSE, INFINITE)) {

                        //
                        // pwp->hWriteEvent was signaled.  This probably
                        // means that the port was closed.
                        //

                        if (pwp->fClose) {
                            goto PortClosed;
                        }
                    }

                    if (GetOverlappedResult(pwp->h32,
                                             &pwp->olWrite,
                                             &pwp->cbWritten,
                                             TRUE
                                             ) )
                       {
                        goto WriteSuccess;
                     }
                }


                LOGDEBUG(0, ("WOWCommWriterThread: WriteFile to id %u fails (error %u)\n",
                             pwp->idComDev, GetLastError()));
                pwp->cbWritePending = 0;
                goto WaitForWriteOrder;

            }


WriteSuccess:

            //
            // Update head pointer to reflect portion written.
            //

            EnterCriticalSection(&pwp->csWrite);

CleanupAfterWriteComplete:
            WOW32ASSERT(pwp->cbWritten == (WORD)pwp->cbWritten);

            pwp->pchWriteHead += pwp->cbWritten;
            pwp->cbWriteFree += (WORD)pwp->cbWritten;
            pwp->cbWritePending = 0;

            //
            // The following is a sanity check on our buffer manipulations.
            //

#ifdef DEBUG
            if (pwp->pchWriteHead >= pwp->pchWriteBuf + pwp->cbWriteBuf) {
                WOW32ASSERT(pwp->pchWriteHead == pwp->pchWriteBuf + pwp->cbWriteBuf);
            }
#endif

            if (pwp->pchWriteHead == pwp->pchWriteBuf + pwp->cbWriteBuf) {
                pwp->pchWriteHead = pwp->pchWriteBuf;
            }
        }

        //
        // We have exhausted the write buffer, leave the critical section
        // and loop back to the wait for the buffer to become non-empty.
        //

        LeaveCriticalSection(&pwp->csWrite);
    }

PortClosed:
    CloseHandle(pwp->olWrite.hEvent);

    return 0;
}



// Checks status on RLSD, CTS, and DSR for timeout support
// see MSRWait() in win3.1 comm.drv code
BOOL MSRWait(PWOWPORT pwp)
{
    DWORD dwStartTime, dwElapsedTime, dwLineStatus;
    DWORD dwErr = 0;


    // start the timeout clock (returns msec)
    dwStartTime = GetTickCount();

    // loop until either all lines are high or a timeout occurs
    while(!dwErr) {

        // get the current status of the lines
        GetCommModemStatus(pwp->h32, &dwLineStatus);

        // if all the required lines are up -- we're done
        if((pwp->lpComDEB16->MSRMask & LOBYTE(dwLineStatus)) == pwp->lpComDEB16->MSRMask)
            break;

        // get the elapsed time
        dwElapsedTime = GetTickCount() - dwStartTime;

        if(pwp->RLSDTimeout != IGNORE_TIMEOUT) {
            // if line is low
            if(!(dwLineStatus & MS_RLSD_ON)) {
                if(dwElapsedTime > UINT32(pwp->RLSDTimeout))
                    dwErr |= CE_RLSDTO;
            }
        }

        if(pwp->CTSTimeout != IGNORE_TIMEOUT) {
            // if line is low
            if(!(dwLineStatus & MS_CTS_ON)) {
                if(dwElapsedTime > UINT32(pwp->CTSTimeout))
                    dwErr |= CE_CTSTO;
            }
        }

        if(pwp->DSRTimeout != IGNORE_TIMEOUT) {
            // if line is low
            if(!(dwLineStatus & MS_DSR_ON)) {
                if(dwElapsedTime > UINT32(pwp->DSRTimeout))
                    dwErr |= CE_DSRTO;
            }
        }
    }

    pwp->dwErrCode |= dwErr;
    pwp->lpComDEB16->ComErr |= LOWORD(dwErr);

    if(dwErr)
       return(TRUE);
    else
       return(FALSE);

}
