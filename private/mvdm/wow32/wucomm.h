/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WUCOMM.H
 *  WOW32 16-bit User API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
 *  Updated    Dec-1992 by Craig Jones (v-cjones)
--*/

#include "wowcomm.h"

// these limits set as doc'd in Win3.1 Prog. ref. for OpenComm()
#define NUMCOMS        9          // max avail COM's
#define NUMLPTS        3          // max available LPT's
#define NUMPORTS  NUMCOMS+NUMLPTS // max # of entries in PortTab[]

// com port indicies into PortTab[]
#define COM1           0
#define COM2           1
#define COM3           2
#define COM4           3
#define COM5           4
#define COM6           5
#define COM7           6
#define COM8           7
#define COM9           8
#define LPT1           NUMCOMS
#define LPT2           LPT1+1
#define LPT3           LPT1+2
#define AUX            COM1
#define PRN            LPT1

// DOS comm IRQ assignments
#define IRQ3   3
#define IRQ4   4
#define IRQ5   5
#define IRQ7   7

// LPT assignments a la Win3.1
#define LPTFIRST       0x80                   // 0x80 == LPT1
#define LPTLAST        LPTFIRST + NUMLPTS - 1 // 0x82 == LPT3 

// other useful deinitions & macros
#define COMMASK        0x00FF                    // strip garbage from idComDev
#define LPTMASK        0x007F                    // get 0-based LPT #
#define GETLPTID(id)   ((id & LPTMASK) + LPT1)   // 0x80 LPT to PortTab[] index
#define TABIDTOLPT(id) (id + LPTFIRST - NUMCOMS) // PortTab[] index to LPT 0x80
#define VALIDCOM(id)   (((id >= COM1)     && (id <  NUMCOMS)) ? TRUE : FALSE)
#define VALIDLPT(id)   (((id >= LPTFIRST) && (id <= LPTLAST)) ? TRUE : FALSE)

#define GETPWOWPTR(id) (VALIDCOM(id) ? PortTab[id].pWOWPort : (VALIDLPT(id) ? PortTab[GETLPTID(id)].pWOWPort : NULL))

#define RM_BIOS_DATA   0x00400000                // bios data real mode seg:0

// for Win3.1 compatibility in EscapeCommFunction() API thunk support
#define RESETDEV      7
#define GETMAXLPT     8
#define GETMAXCOM     9
#define GETBASEIRQ   10

// notifications for EnableCommNotification() support
#define CN_RECEIVE    0x0001
#define CN_TRANSMIT   0x0002
#define CN_EVENT      0x0004
#define CN_RECEIVEHI  0x0100
#define CN_TRANSMITHI 0x0200
#define CN_NOTIFYHI   0x0400

#define WOW_WM_COMMNOTIFY 0x0044

// set all the events that can be masked on NT (a sub-set of Win3.1)
#define EV_NTEVENTS (EV_BREAK | EV_CTS    | EV_DSR    | EV_ERR  | EV_TXEMPTY | \
                     EV_RLSD  | EV_RXCHAR | EV_RXFLAG | EV_RING)

// constants for how Win3.1 expects to see the MSR
#define MSR_DELTAONLY   0x0000000F // strip off MSR state bits
#define MSR_STATEONLY   0x000000F0 // strip off MSR delta bits
#define MSR_DCTS        0x01       // bit for delta CTS
#define MSR_DDSR        0x02       // bit for delta DSR
#define MSR_TERI        0x04       // bit for TERI
#define MSR_DDCD        0x08       // bit for delta DCD
#define MSR_CTS         0x10       // bit for CTS
#define MSR_DSR         0x20       // bit for DSR
#define MSR_RI          0x40       // bit for RI
#define MSR_DCD         0x80       // bit for DCD

// Win3.1 constants for RLSD, CTS, and DSR timeout support
#define CE_RLSDTO       0x0080
#define CE_CTSTO        0x0020
#define CE_DSRTO        0x0040

// constants for the Event Word 
#define EV_CTSS     0x00000400 // bit for Win3.1 showing CTS state
#define EV_DSRS     0x00000800 // bit for Win3.1 showing DSR state
#define EV_RLSDS    0x00001000 // bit for Win3.1 showing RLSD state
#define EV_RingTe   0x00002000 // bit for Win3.1 showing RingTe state

#define ERR_XMIT         0x4000 // can't xmit a char Win3.1
#define INFINITE_TIMEOUT 0xFFFF // infinite timeout Win3.1
#define IGNORE_TIMEOUT   0x0000 // Win3.1 ignore RLSD, CTS, & DSR timeouts

#define COMBUF 2 // max. # of bytes we'll queue for WriteComm()

#define MAXCOMNAME     4             // max length of a comm device name
#define MAXCOMNAMENULL MAXCOMNAME+1  // length of a comm device name + NULL

// for 16-bit to 32-bit comm support
typedef struct _WOWPORT {
    UINT       idComDev;       // idComDev returned to app as handle of port
    HANDLE     h32;            // NT file handle used instead of idComDev
    HANDLE     hREvent;        // structure for overlapped reads
    CRITICAL_SECTION csWrite;  // critsect controls following 4 variables.
    PUCHAR     pchWriteHead;   // oldest byte not yet written to port.
    PUCHAR     pchWriteTail;   // first byte available in buffer.
    WORD       cbWriteFree;    // number of bytes available in write buffer.
    WORD       cbWritePending; // number of bytes now in WriteFile()
    PUCHAR     pchWriteBuf;    // write buffer
    WORD       cbWriteBuf;     // size of the write buffer.  One byte unused.
    HANDLE     hWriteThread;   // thread handle for COM writer.
    HANDLE     hWriteEvent;    // signalled by app thread when empty buffer
                               // made non-empty to wake up writer thread.
    OVERLAPPED olWrite;        // Overlapped structure used for writes.
    BOOL       fWriteDone;     // Indicates app thread completed first write.
    DWORD      cbWritten;      // Valid when fWriteDone == TRUE.
    DWORD      dwThreadID;     // app's thread id for crashed/hung app support
    DWORD      dwErrCode;      // most recent error for this idComDev 
    COMSTAT    cs;             // struct for error handling
    BOOL       fChEvt;         // TRUE if app set fChEvt in DCB struct
  // 16-bit DCB for LPT support only
    PDCB16     pdcb16;         // save DCB for LPT ports
  // for UngetCommChar() support
    BOOL       fUnGet;         // flag specifying an ungot char is pending
    UCHAR      cUnGet;         // ungot char in "buffer" only if fUnGet is set
  // for SetCommEventMask()/EnableCommNotification() support
    HANDLE     hMiThread;      // thread handle for Modem interrupt support
    BOOL       fClose;         // flag to close auxiliary threads
  // for SetCommEventMask() support only
    DWORD      dwComDEB16;     // DWORD obtained by call to GlobalDosAlloc()
    PCOMDEB16  lpComDEB16;     // flat address to above
  // for XonLim & XoffLim checking in SetCommState
    DWORD      cbInQ;          // Actual size of in Queue set in WU32OpenComm
  // for RLSD, CTS, DSR timeout support
    WORD       RLSDTimeout;    // max time in msec to wait for RLSD (0->ignore)
    WORD       CTSTimeout;     // max time in msec to wait for CTS (0->ignore)
    WORD       DSRTimeout;     // max time in msec to wait for DSR (0->ignore)
    DWORD      QLStackSeg;     // Quicklink 1.3 hack See bug #398011
                               // save the seg val of COMDEB16 in low word, &
                               // the QuickLink stack selector in the high word
} WOWPORT, *PWOWPORT;

// Table of above structs, one entry needed for each comm port
typedef struct _PORTTAB {
    CHAR      szPort[MAXCOMNAMENULL]; // port name
    PWOWPORT  pWOWPort;               // pointer to Comm Mapping struct
} PORTTAB, *PPORTTAB;

//
// Macro to calculate the size of chunk to write from the write
// to the filesystem.
//
// This is either the entire pending part of the
// buffer, or, if the buffer wraps, it is the portion
// between the head and the end of the buffer.
//
// In order to keep COMSTAT.cbOutQue moving at a reasonable
// pace, we restrict ourselves to writing at most 1024 bytes
// at a time.  This is because ProComm for Windows uses the
// cbOutQue value in displaying its progress, so if we allow
// larger writes it will only update every 5-10k (assuming
// ProComm's default 16k write buffer),
//

#define CALC_COMM_WRITE_SIZE(pwp)                            \
                min(1024,                                      \
                    (pwp->pchWriteHead < pwp->pchWriteTail)    \
                     ? pwp->pchWriteTail - pwp->pchWriteHead   \
                     : (pwp->pchWriteBuf + pwp->cbWriteBuf) -  \
                        pwp->pchWriteHead                      \
                   );


// Win3.1 timesout Tx after approx. 65000 msec (65 sec)
#define WRITE_TIMEOUT 65000

// bitfields of the 16-bit COMSTAT.status
#define W31CS_fCtsHold       0x01
#define W31CS_fDsrHold       0x02
#define W31CS_fRlsdHold      0x04
#define W31CS_fXoffHold      0x08
#define W31CS_fSentHold      0x10
#define W31CS_fEof           0x20
#define W31CS_fTxim          0x40

// Win3.1 Baud Rate constants
#define W31CBR_110       0xFF10
#define W31CBR_300       0xFF11
#define W31CBR_600       0xFF12
#define W31CBR_1200      0xFF13
#define W31CBR_2400      0xFF14
#define W31CBR_4800      0xFF15
#define W31CBR_9600      0xFF16
#define W31CBR_14400     0xFF17
#define W31CBR_19200     0xFF18
#define W31CBR_reserved1 0xFF19
#define W31CBR_reserved2 0xFF1A
#define W31CBR_38400     0xFF1B
#define W31CBR_reserved3 0xFF1C
#define W31CBR_reserved4 0xFF1D
#define W31CBR_reserved5 0xFF1E
#define W31CBR_56000     0xFF1F

// these are defined in Win3.1 windows.h but aren't supported in comm.drv
#define W31CBR_128000    0xFF23
#define W31CBR_256000    0xFF27

// special way to say 115200
#define W31CBR_115200    0xFEFF

// constants for conversions from Win3.1 baud specifications to 32-bit baud
#define W31_DLATCH_110      1047
#define W31_DLATCH_300       384
#define W31_DLATCH_600       192 
#define W31_DLATCH_1200       96
#define W31_DLATCH_2400       48
#define W31_DLATCH_4800       24
#define W31_DLATCH_9600       12
#define W31_DLATCH_14400       8
#define W31_DLATCH_19200       6
#define W31_DLATCH_38400       3
#define W31_DLATCH_56000       2
#define W31_DLATCH_115200      1

// Win3.1 flags for DCB structure
#define W31DCB_fBinary       0x0001
#define W31DCB_fRtsDisable   0x0002
#define W31DCB_fParity       0x0004
#define W31DCB_fOutxCtsFlow  0x0008
#define W31DCB_fOutxDsrFlow  0x0010
#define W31DCB_fDummy       (0x0020 | 0x0040)
#define W31DCB_fDtrDisable   0x0080
#define W31DCB_fOutX         0x0100
#define W31DCB_fInX          0x0200
#define W31DCB_fPeChar       0x0400
#define W31DCB_fNull         0x0800
#define W31DCB_fChEvt        0x1000
#define W31DCB_fDtrFlow      0x2000
#define W31DCB_fRtsFlow      0x4000
#define W31DCB_fDummy2       0x8000



//+++ DEBUG SUPPORT

#ifdef DEBUG

#define COMMDEBUG(lpszformat) LOGDEBUG(1, lpszformat)

// for watching the modem events
#define DEBUGWATCHMODEMEVENTS(dwE, dwM, dwS, pcE16, pcM16) {    \
    if(dwS) {                                                   \
        if((dwE != (DWORD)pcE16) || (dwM != (DWORD)pcM16)) {    \
            dwE = (DWORD)pcE16;                                 \
            dwM = (DWORD)pcM16;                                 \
            COMMDEBUG(("\nEvt:0x%4X  MSR:0x%2X\n", dwE, dwM));  \
        }                                                       \
        else {                                                  \
         COMMDEBUG(("."));                                      \
        }                                                       \
    }                                                           \
}

// prototype for real-time debug output
void CommIODebug(ULONG fhCommIO, HANDLE hCommIO, LPSZ lpsz, ULONG cb, LPSZ lpszFile);


#else  // endif DEBUG

#define COMMDEBUG(lpszFormat) 
#define DEBUGWATCHMODEMEVENTS(dwE, dwM, dwS, pcE16, pcM16) 
#define CommIODebug(fhCommIO, hCommIO, lpsz, cb, lpszFile)

#endif // endif !DEBUG

//--- DEBUG SUPPORT




// API support function prototypes
ULONG FASTCALL   WU32BuildCommDCB(PVDMFRAME pFrame);
ULONG FASTCALL   WU32ClearCommBreak(PVDMFRAME pFrame);
ULONG FASTCALL   WU32CloseComm(PVDMFRAME pFrame);
ULONG FASTCALL   WU32EnableCommNotification(PVDMFRAME pFrame);
ULONG FASTCALL   WU32EscapeCommFunction(PVDMFRAME pFrame);
ULONG FASTCALL   WU32FlushComm(PVDMFRAME pFrame);
ULONG FASTCALL   WU32GetCommError(PVDMFRAME pFrame);
ULONG FASTCALL   WU32GetCommEventMask(PVDMFRAME pFrame);
ULONG FASTCALL   WU32GetCommState(PVDMFRAME pFrame);
ULONG FASTCALL   WU32OpenComm(PVDMFRAME pFrame);
ULONG FASTCALL   WU32ReadComm(PVDMFRAME pFrame);
ULONG FASTCALL   WU32SetCommBreak(PVDMFRAME pFrame);
ULONG FASTCALL   WU32SetCommEventMask(PVDMFRAME pFrame);
ULONG FASTCALL   WU32SetCommState(PVDMFRAME pFrame);
ULONG FASTCALL   WU32TransmitCommChar(PVDMFRAME pFrame);
ULONG FASTCALL   WU32UngetCommChar(PVDMFRAME pFrame);
ULONG FASTCALL   WU32WriteComm(PVDMFRAME pFrame);

// prototypes for functions exported to the VDM
BYTE    GetCommShadowMSR(WORD idComDev);
HANDLE  GetCommHandle(WORD idComDev);

// prototype for crashed/hung app cleanup support
VOID FreeCommSupportResources(DWORD dwThreadID);
