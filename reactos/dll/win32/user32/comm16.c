/*
 * DEC 93 Erik Bos <erik@xs4all.nl>
 *
 * Copyright 1996 Marcus Meissner
 *
 * Copyright 2001 Mike McCormack
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * History:
 *
 * Mar 31, 1999. Ove Kåven <ovek@arcticnet.no>
 * - Implemented buffers and EnableCommNotification.
 *
 * Apr 3, 1999.  Lawson Whitney <lawson_whitney@juno.com>
 * - Fixed the modem control part of EscapeCommFunction16.
 *
 * Mar 3, 1999. Ove Kåven <ovek@arcticnet.no>
 * - Use port indices instead of unixfds for win16
 * - Moved things around (separated win16 and win32 routines)
 * - Added some hints on how to implement buffers and EnableCommNotification.
 *
 * May 26, 1997.  Fixes and comments by Rick Richardson <rick@dgii.com> [RER]
 * - ptr->fd wasn't getting cleared on close.
 * - GetCommEventMask() and GetCommError() didn't do much of anything.
 *   IMHO, they are still wrong, but they at least implement the RXCHAR
 *   event and return I/O queue sizes, which makes the app I'm interested
 *   in (analog devices EZKIT DSP development system) work.
 *
 * August 12, 1997.  Take a bash at SetCommEventMask - Lawson Whitney
 *                                     <lawson_whitney@juno.com>
 * July 6, 1998. Fixes and comments by Valentijn Sessink
 *                                     <vsessink@ic.uva.nl> [V]
 * Oktober 98, Rein Klazes [RHK]
 * A program that wants to monitor the modem status line (RLSD/DCD) may
 * poll the modem status register in the commMask structure. I update the bit
 * in GetCommError, waiting for an implementation of communication events.
 *
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "windef.h"
#include "winbase.h"
#include "wine/winuser16.h"
#include "win.h"
#include "user_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(comm);

/* window's semi documented modem status register */
#define COMM_MSR_OFFSET  35
#define MSR_CTS  0x10
#define MSR_DSR  0x20
#define MSR_RI   0x40
#define MSR_RLSD 0x80
#define MSR_MASK (MSR_CTS|MSR_DSR|MSR_RI|MSR_RLSD)

#define FLAG_LPT 0x80

#define MAX_PORTS   9

struct DosDeviceStruct {
    HANDLE handle;
    int suspended;
    int unget,xmit;
    int evtchar;
    /* events */
    int commerror, eventmask;
    /* buffers */
    char *inbuf,*outbuf;
    unsigned ibuf_size,ibuf_head,ibuf_tail;
    unsigned obuf_size,obuf_head,obuf_tail;
    /* notifications */
    HWND wnd;
    int n_read, n_write;
    OVERLAPPED read_ov, write_ov;
    /* save terminal states */
    DCB16 dcb;
    /* pointer to unknown(==undocumented) comm structure */
    SEGPTR seg_unknown;
    BYTE unknown[40];
};

static struct DosDeviceStruct COM[MAX_PORTS];
static struct DosDeviceStruct LPT[MAX_PORTS];

/* update window's semi documented modem status register */
/* see knowledge base Q101417 */
static void COMM_MSRUpdate( HANDLE handle, UCHAR * pMsr )
{
    UCHAR tmpmsr=0;
    DWORD mstat=0;

    if(!GetCommModemStatus(handle,&mstat))
        return;

    if(mstat & MS_CTS_ON) tmpmsr |= MSR_CTS;
    if(mstat & MS_DSR_ON) tmpmsr |= MSR_DSR;
    if(mstat & MS_RING_ON) tmpmsr |= MSR_RI;
    if(mstat & MS_RLSD_ON) tmpmsr |= MSR_RLSD;
    *pMsr = (*pMsr & ~MSR_MASK) | tmpmsr;
}

static struct DosDeviceStruct *GetDeviceStruct(int index)
{
	if ((index&0x7F)<=MAX_PORTS) {
            if (!(index&FLAG_LPT)) {
		if (COM[index].handle)
		    return &COM[index];
	    } else {
		index &= 0x7f;
		if (LPT[index].handle)
		    return &LPT[index];
	    }
	}

	return NULL;
}

static int    GetCommPort_ov(const OVERLAPPED *ov, int write)
{
	int x;

	for (x=0; x<MAX_PORTS; x++) {
		if (ov == (write?&COM[x].write_ov:&COM[x].read_ov))
			return x;
	}

	return -1;
}

static int WinError(void)
{
        TRACE("errno = %d\n", errno);
	switch (errno) {
		default:
			return CE_IOE;
		}
}

static unsigned comm_inbuf(const struct DosDeviceStruct *ptr)
{
  return ((ptr->ibuf_tail > ptr->ibuf_head) ? ptr->ibuf_size : 0)
    + ptr->ibuf_head - ptr->ibuf_tail;
}

static unsigned comm_outbuf(const struct DosDeviceStruct *ptr)
{
  return ((ptr->obuf_tail > ptr->obuf_head) ? ptr->obuf_size : 0)
    + ptr->obuf_head - ptr->obuf_tail;
}

static void comm_waitread(struct DosDeviceStruct *ptr);
static void comm_waitwrite(struct DosDeviceStruct *ptr);

static VOID WINAPI COMM16_ReadComplete(DWORD dwErrorCode, DWORD len, LPOVERLAPPED ov)
{
	int prev;
	WORD mask = 0;
	int cid = GetCommPort_ov(ov,0);
	struct DosDeviceStruct *ptr;

	if(cid<0) {
		ERR("async write with bad overlapped pointer\n");
		return;
	}
	ptr = &COM[cid];

	/* we get cancelled when CloseComm is called */
	if (dwErrorCode==ERROR_OPERATION_ABORTED)
	{
		TRACE("Cancelled\n");
		return;
	}

	/* read data from comm port */
	if (dwErrorCode != NO_ERROR) {
		ERR("async read failed, error %d\n",dwErrorCode);
		COM[cid].commerror = CE_RXOVER;
		return;
	}
	TRACE("async read completed %d bytes\n",len);

	prev = comm_inbuf(ptr);

	/* check for events */
	if ((ptr->eventmask & EV_RXFLAG) &&
	    memchr(ptr->inbuf + ptr->ibuf_head, ptr->evtchar, len)) {
		*(WORD*)(COM[cid].unknown) |= EV_RXFLAG;
		mask |= CN_EVENT;
	}
	if (ptr->eventmask & EV_RXCHAR) {
		*(WORD*)(COM[cid].unknown) |= EV_RXCHAR;
		mask |= CN_EVENT;
	}

	/* advance buffer position */
	ptr->ibuf_head += len;
	if (ptr->ibuf_head >= ptr->ibuf_size)
		ptr->ibuf_head = 0;

	/* check for notification */
	if (ptr->wnd && (ptr->n_read>0) && (prev<ptr->n_read) &&
	    (comm_inbuf(ptr)>=ptr->n_read)) {
		/* passed the receive notification threshold */
		mask |= CN_RECEIVE;
	}

	/* send notifications, if any */
	if (ptr->wnd && mask) {
		TRACE("notifying %p: cid=%d, mask=%02x\n", ptr->wnd, cid, mask);
		PostMessageA(ptr->wnd, WM_COMMNOTIFY, cid, mask);
	}

	/* on real windows, this could cause problems, since it is recursive */
	/* restart the receive */
	comm_waitread(ptr);
}

/* this is meant to work like write() */
static INT COMM16_WriteFile(HANDLE hComm, LPCVOID buffer, DWORD len)
{
	OVERLAPPED ov;
	DWORD count= -1;

	ZeroMemory(&ov,sizeof(ov));
	ov.hEvent = CreateEventW(NULL,0,0,NULL);
	if(ov.hEvent==INVALID_HANDLE_VALUE)
		return -1;

	if(!WriteFile(hComm,buffer,len,&count,&ov))
	{
		if(GetLastError()==ERROR_IO_PENDING)
		{
			GetOverlappedResult(hComm,&ov,&count,TRUE);
		}
	}
	CloseHandle(ov.hEvent);

	return count;
}

static VOID WINAPI COMM16_WriteComplete(DWORD dwErrorCode, DWORD len, LPOVERLAPPED ov)
{
	int prev, bleft;
	WORD mask = 0;
	int cid = GetCommPort_ov(ov,1);
	struct DosDeviceStruct *ptr;

	if(cid<0) {
		ERR("async write with bad overlapped pointer\n");
		return;
	}
	ptr = &COM[cid];

	/* read data from comm port */
	if (dwErrorCode != NO_ERROR) {
		ERR("async write failed, error %d\n",dwErrorCode);
		COM[cid].commerror = CE_RXOVER;
		return;
	}
	TRACE("async write completed %d bytes\n",len);

	/* update the buffer pointers */
	prev = comm_outbuf(&COM[cid]);
	ptr->obuf_tail += len;
	if (ptr->obuf_tail >= ptr->obuf_size)
		ptr->obuf_tail = 0;

	/* write any TransmitCommChar character */
	if (ptr->xmit>=0) {
		len = COMM16_WriteFile(ptr->handle, &(ptr->xmit), 1);
		if (len > 0) ptr->xmit = -1;
	}

	/* write from output queue */
	bleft = ((ptr->obuf_tail <= ptr->obuf_head) ?
		ptr->obuf_head : ptr->obuf_size) - ptr->obuf_tail;

	/* check for notification */
	if (ptr->wnd && (ptr->n_write>0) && (prev>=ptr->n_write) &&
	  (comm_outbuf(ptr)<ptr->n_write)) {
		/* passed the transmit notification threshold */
		mask |= CN_TRANSMIT;
	}

	/* send notifications, if any */
	if (ptr->wnd && mask) {
		TRACE("notifying %p: cid=%d, mask=%02x\n", ptr->wnd, cid, mask);
		PostMessageA(ptr->wnd, WM_COMMNOTIFY, cid, mask);
	}

	/* start again if necessary */
	if(bleft)
		comm_waitwrite(ptr);
}

static void comm_waitread(struct DosDeviceStruct *ptr)
{
	unsigned int bleft;
	COMSTAT stat;

	/* FIXME: get timeouts working properly so we can read bleft bytes */
	bleft = ((ptr->ibuf_tail > ptr->ibuf_head) ?
		(ptr->ibuf_tail-1) : ptr->ibuf_size) - ptr->ibuf_head;

	/* find out how many bytes are left in the buffer */
	if(ClearCommError(ptr->handle,NULL,&stat))
		bleft = (bleft<stat.cbInQue) ? bleft : stat.cbInQue;
	else
		bleft = 1;

	/* always read at least one byte */
	if(bleft==0)
		bleft++;

	ReadFileEx(ptr->handle,
		ptr->inbuf + ptr->ibuf_head,
		bleft,
		&ptr->read_ov,
		COMM16_ReadComplete);
}

static void comm_waitwrite(struct DosDeviceStruct *ptr)
{
	int bleft;

	bleft = ((ptr->obuf_tail <= ptr->obuf_head) ?
		ptr->obuf_head : ptr->obuf_size) - ptr->obuf_tail;
	WriteFileEx(ptr->handle,
		ptr->outbuf + ptr->obuf_tail,
		bleft,
		&ptr->write_ov,
		COMM16_WriteComplete);
}

/*****************************************************************************
 *	COMM16_DCBtoDCB16	(Internal)
 */
static INT16 COMM16_DCBtoDCB16(const DCB *lpdcb, LPDCB16 lpdcb16)
{
	if(lpdcb->BaudRate<0x10000)
		lpdcb16->BaudRate = lpdcb->BaudRate;
	else if(lpdcb->BaudRate==115200)
			lpdcb16->BaudRate = 57601;
	else {
		WARN("Baud rate can't be converted\n");
		lpdcb16->BaudRate = 57601;
	}
	lpdcb16->ByteSize = lpdcb->ByteSize;
	lpdcb16->fParity = lpdcb->fParity;
	lpdcb16->Parity = lpdcb->Parity;
	lpdcb16->StopBits = lpdcb->StopBits;

	lpdcb16->RlsTimeout = 50;
	lpdcb16->CtsTimeout = 50;
	lpdcb16->DsrTimeout = 50;
	lpdcb16->fNull = 0;
	lpdcb16->fChEvt = 0;
	lpdcb16->fBinary = 1;

	lpdcb16->fDtrflow = (lpdcb->fDtrControl==DTR_CONTROL_HANDSHAKE);
	lpdcb16->fRtsflow = (lpdcb->fRtsControl==RTS_CONTROL_HANDSHAKE);
	lpdcb16->fOutxCtsFlow = lpdcb->fOutxCtsFlow;
	lpdcb16->fOutxDsrFlow = lpdcb->fOutxDsrFlow;
	lpdcb16->fDtrDisable = (lpdcb->fDtrControl==DTR_CONTROL_DISABLE);

	lpdcb16->fInX = lpdcb->fInX;

	lpdcb16->fOutX = lpdcb->fOutX;
/*
	lpdcb16->XonChar =
	lpdcb16->XoffChar =
 */
	lpdcb16->XonLim = 10;
	lpdcb16->XoffLim = 10;

	return 0;
}


/**************************************************************************
 *         BuildCommDCB		(USER.213)
 *
 * According to the ECMA-234 (368.3) the function will return FALSE on
 * success, otherwise it will return -1.
 */
INT16 WINAPI BuildCommDCB16(LPCSTR device, LPDCB16 lpdcb)
{
	/* "COM1:96,n,8,1"	*/
	/*  012345		*/
	int port;
	DCB dcb;

	TRACE("(%s), ptr %p\n", device, lpdcb);

	if (strncasecmp(device,"COM",3))
		return -1;
	port = device[3] - '0';

	if (port-- == 0) {
		ERR("BUG ! COM0 can't exist!\n");
		return -1;
	}

	memset(lpdcb, 0, sizeof(DCB16)); /* initialize */

	lpdcb->Id = port;
	dcb.DCBlength = sizeof(DCB);

	if (strchr(device,'=')) /* block new style */
		return -1;

	if(!BuildCommDCBA(device,&dcb))
		return -1;

	return COMM16_DCBtoDCB16(&dcb, lpdcb);
}

/*****************************************************************************
 *	OpenComm		(USER.200)
 */
INT16 WINAPI OpenComm16(LPCSTR device,UINT16 cbInQueue,UINT16 cbOutQueue)
{
	int port;
	HANDLE handle;

    	TRACE("%s, %d, %d\n", device, cbInQueue, cbOutQueue);

	if (strlen(device) < 4)
	   return IE_BADID;

	port = device[3] - '0';

	if (port-- == 0)
		ERR("BUG ! COM0 or LPT0 don't exist !\n");

	if (!strncasecmp(device,"COM",3))
        {
		if (COM[port].handle)
			return IE_OPEN;

		handle = CreateFileA(device, GENERIC_READ|GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
			FILE_FLAG_OVERLAPPED|FILE_FLAG_NO_BUFFERING, 0 );
		if (handle == INVALID_HANDLE_VALUE) {
			return IE_HARDWARE;
		} else {
			memset(COM[port].unknown, 0, sizeof(COM[port].unknown));
                        COM[port].seg_unknown = 0;
			COM[port].handle = handle;
			COM[port].commerror = 0;
			COM[port].eventmask = 0;
			COM[port].evtchar = 0; /* FIXME: default? */
                        /* save terminal state */
			GetCommState16(port,&COM[port].dcb);
			/* init priority characters */
			COM[port].unget = -1;
			COM[port].xmit = -1;
			/* allocate buffers */
			COM[port].ibuf_size = cbInQueue;
			COM[port].ibuf_head = COM[port].ibuf_tail = 0;
			COM[port].obuf_size = cbOutQueue;
			COM[port].obuf_head = COM[port].obuf_tail = 0;

			COM[port].inbuf = HeapAlloc(GetProcessHeap(), 0, cbInQueue);
			if (COM[port].inbuf) {
			  COM[port].outbuf = HeapAlloc( GetProcessHeap(), 0, cbOutQueue);
			  if (!COM[port].outbuf)
			    HeapFree( GetProcessHeap(), 0, COM[port].inbuf);
			} else COM[port].outbuf = NULL;
			if (!COM[port].outbuf) {
			  /* not enough memory */
			  CloseHandle(COM[port].handle);
			  ERR("out of memory\n");
			  return IE_MEMORY;
			}

			ZeroMemory(&COM[port].read_ov,sizeof (OVERLAPPED));
			ZeroMemory(&COM[port].write_ov,sizeof (OVERLAPPED));

                        comm_waitread( &COM[port] );
			USER16_AlertableWait++;

			return port;
		}
	}
	else
	if (!strncasecmp(device,"LPT",3)) {

		if (LPT[port].handle)
			return IE_OPEN;

		handle = CreateFileA(device, GENERIC_READ|GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, 0 );
		if (handle == INVALID_HANDLE_VALUE) {
			return IE_HARDWARE;
		} else {
			LPT[port].handle = handle;
			LPT[port].commerror = 0;
			LPT[port].eventmask = 0;
			return port|FLAG_LPT;
		}
	}
	return IE_BADID;
}

/*****************************************************************************
 *	CloseComm		(USER.207)
 */
INT16 WINAPI CloseComm16(INT16 cid)
{
	struct DosDeviceStruct *ptr;

    	TRACE("cid=%d\n", cid);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no cid=%d found!\n", cid);
		return -1;
	}
	if (!(cid&FLAG_LPT)) {
		/* COM port */
                UnMapLS( COM[cid].seg_unknown );
		USER16_AlertableWait--;
		CancelIo(ptr->handle);

		/* free buffers */
		HeapFree( GetProcessHeap(), 0, ptr->outbuf);
		HeapFree( GetProcessHeap(), 0, ptr->inbuf);

		/* reset modem lines */
		SetCommState16(&COM[cid].dcb);
	}

	if (!CloseHandle(ptr->handle)) {
		ptr->commerror = WinError();
		/* FIXME: should we clear ptr->handle here? */
		return -1;
	} else {
		ptr->commerror = 0;
		ptr->handle = 0;
		return 0;
	}
}

/*****************************************************************************
 *	SetCommBreak		(USER.210)
 */
INT16 WINAPI SetCommBreak16(INT16 cid)
{
	struct DosDeviceStruct *ptr;

	TRACE("cid=%d\n", cid);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no cid=%d found!\n", cid);
		return -1;
	}

	ptr->suspended = 1;
	ptr->commerror = 0;
	return 0;
}

/*****************************************************************************
 *	ClearCommBreak	(USER.211)
 */
INT16 WINAPI ClearCommBreak16(INT16 cid)
{
	struct DosDeviceStruct *ptr;

    	TRACE("cid=%d\n", cid);
	if (!(ptr = GetDeviceStruct(cid))) {
		FIXME("no cid=%d found!\n", cid);
		return -1;
	}
	ptr->suspended = 0;
	ptr->commerror = 0;
	return 0;
}

/*****************************************************************************
 *	EscapeCommFunction	(USER.214)
 */
LONG WINAPI EscapeCommFunction16(UINT16 cid,UINT16 nFunction)
{
	struct  DosDeviceStruct *ptr;

    	TRACE("cid=%d, function=%d\n", cid, nFunction);

	switch(nFunction) {
	case GETMAXCOM:
	        TRACE("GETMAXCOM\n");
                return 4;  /* FIXME */

	case GETMAXLPT:
		TRACE("GETMAXLPT\n");
                return FLAG_LPT + 3;  /* FIXME */

	case GETBASEIRQ:
		TRACE("GETBASEIRQ\n");
		/* FIXME: use tables */
		/* just fake something for now */
		if (cid & FLAG_LPT) {
			/* LPT1: irq 7, LPT2: irq 5 */
			return (cid & 0x7f) ? 5 : 7;
		} else {
			/* COM1: irq 4, COM2: irq 3,
			   COM3: irq 4, COM4: irq 3 */
			return 4 - (cid & 1);
		}
	}

	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no cid=%d found!\n", cid);
		return -1;
	}

	switch (nFunction) {
	case RESETDEV:
	case CLRDTR:
	case CLRRTS:
	case SETDTR:
	case SETRTS:
	case SETXOFF:
	case SETXON:
		if(EscapeCommFunction(ptr->handle,nFunction))
			return 0;
		else {
			ptr->commerror = WinError();
			return -1;
		}

	case CLRBREAK:
	case SETBREAK:
	default:
		WARN("(cid=%d,nFunction=%d): Unknown function\n",
			cid, nFunction);
	}
	return -1;
}

/*****************************************************************************
 *	FlushComm	(USER.215)
 */
INT16 WINAPI FlushComm16(INT16 cid,INT16 fnQueue)
{
	DWORD queue;
	struct DosDeviceStruct *ptr;

    	TRACE("cid=%d, queue=%d\n", cid, fnQueue);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no cid=%d found!\n", cid);
		return -1;
	}
	switch (fnQueue) {
	case 0:
		queue = PURGE_TXABORT;
		ptr->obuf_tail = ptr->obuf_head;
		break;
	case 1:
		queue = PURGE_RXABORT;
		ptr->ibuf_head = ptr->ibuf_tail;
		break;
	default:
		WARN("(cid=%d,fnQueue=%d):Unknown queue\n",
		            cid, fnQueue);
		return -1;
	}

	if (!PurgeComm(ptr->handle,queue)) {
		ptr->commerror = WinError();
		return -1;
	} else {
		ptr->commerror = 0;
		return 0;
	}
}

/********************************************************************
 *	GetCommError	(USER.203)
 */
INT16 WINAPI GetCommError16(INT16 cid,LPCOMSTAT16 lpStat)
{
	int		temperror;
	struct DosDeviceStruct *ptr;
        unsigned char *stol;

	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!\n",cid);
		return -1;
	}
        if (cid&FLAG_LPT) {
            WARN(" cid %d not comm port\n",cid);
            return CE_MODE;
        }
        stol = (unsigned char *)COM[cid].unknown + COMM_MSR_OFFSET;
	COMM_MSRUpdate( ptr->handle, stol );

       if (lpStat) {
               lpStat->status = 0;

               if (comm_inbuf(ptr) == 0)
                       SleepEx(1,TRUE);

		lpStat->cbOutQue = comm_outbuf(ptr);
		lpStat->cbInQue = comm_inbuf(ptr);

    		TRACE("cid %d, error %d, stat %d in %d out %d, stol %x\n",
			     cid, ptr->commerror, lpStat->status, lpStat->cbInQue,
			     lpStat->cbOutQue, *stol);
	}
	else
		TRACE("cid %d, error %d, lpStat NULL stol %x\n",
			     cid, ptr->commerror, *stol);

	/* Return any errors and clear it */
	temperror = ptr->commerror;
	ptr->commerror = 0;
	return(temperror);
}

/*****************************************************************************
 *	SetCommEventMask	(USER.208)
 */
SEGPTR WINAPI SetCommEventMask16(INT16 cid,UINT16 fuEvtMask)
{
	struct DosDeviceStruct *ptr;
        unsigned char *stol;

    	TRACE("cid %d,mask %d\n",cid,fuEvtMask);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!\n",cid);
            return 0;
	}

	ptr->eventmask = fuEvtMask;

        if (cid&FLAG_LPT) {
            WARN(" cid %d not comm port\n",cid);
            return 0;
        }
        /* it's a COM port ? -> modify flags */
        stol = (unsigned char *)COM[cid].unknown + COMM_MSR_OFFSET;
	COMM_MSRUpdate( ptr->handle, stol );

	TRACE(" modem dcd construct %x\n",*stol);
        if (!COM[cid].seg_unknown) COM[cid].seg_unknown = MapLS( COM[cid].unknown );
        return COM[cid].seg_unknown;
}

/*****************************************************************************
 *	GetCommEventMask	(USER.209)
 */
UINT16 WINAPI GetCommEventMask16(INT16 cid,UINT16 fnEvtClear)
{
	struct DosDeviceStruct *ptr;
	WORD events;

    	TRACE("cid %d, mask %d\n", cid, fnEvtClear);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!\n",cid);
	    return 0;
	}

        if (cid&FLAG_LPT) {
            WARN(" cid %d not comm port\n",cid);
            return 0;
        }

	events = *(WORD*)(COM[cid].unknown) & fnEvtClear;
	*(WORD*)(COM[cid].unknown) &= ~fnEvtClear;
	return events;
}

/*****************************************************************************
 *	SetCommState	(USER.201)
 */
INT16 WINAPI SetCommState16(LPDCB16 lpdcb)
{
	struct DosDeviceStruct *ptr;
	DCB dcb;

    	TRACE("cid %d, ptr %p\n", lpdcb->Id, lpdcb);
	if ((ptr = GetDeviceStruct(lpdcb->Id)) == NULL) {
		FIXME("no handle for cid = %0x!\n",lpdcb->Id);
		return -1;
	}

	memset(&dcb,0,sizeof(dcb));
	dcb.DCBlength = sizeof(dcb);

	/*
	 * according to MSDN, we should first interpret lpdcb->BaudRate as follows:
	 * 1. if the baud rate is a CBR constant, interpret it.
	 * 2. if it is greater than 57600, the baud rate is 115200
	 * 3. use the actual baudrate
	 * steps 2 and 3 are equivalent to 16550 baudrate divisor = 115200/BaudRate
	 */
	switch(lpdcb->BaudRate)
	{
	case CBR_110:    dcb.BaudRate = 110;    break;
	case CBR_300:    dcb.BaudRate = 300;    break;
	case CBR_600:    dcb.BaudRate = 600;    break;
	case CBR_1200:   dcb.BaudRate = 1200;   break;
	case CBR_2400:   dcb.BaudRate = 2400;   break;
	case CBR_4800:   dcb.BaudRate = 4800;   break;
	case CBR_9600:   dcb.BaudRate = 9600;   break;
	case CBR_14400:  dcb.BaudRate = 14400;  break;
	case CBR_19200:  dcb.BaudRate = 19200;  break;
	case CBR_38400:  dcb.BaudRate = 38400;  break;
	case CBR_56000:  dcb.BaudRate = 56000;  break;
	case CBR_128000: dcb.BaudRate = 128000; break;
	case CBR_256000: dcb.BaudRate = 256000; break;
	default:
		if(lpdcb->BaudRate>57600)
		dcb.BaudRate = 115200;
        else
		dcb.BaudRate = lpdcb->BaudRate;
 	}

        dcb.ByteSize=lpdcb->ByteSize;
        dcb.StopBits=lpdcb->StopBits;

	dcb.fParity=lpdcb->fParity;
	dcb.Parity=lpdcb->Parity;

	dcb.fOutxCtsFlow = lpdcb->fOutxCtsFlow;

	if (lpdcb->fDtrflow || lpdcb->fRtsflow)
		dcb.fRtsControl = TRUE;

	if (lpdcb->fDtrDisable)
		dcb.fDtrControl = TRUE;

	ptr->evtchar = lpdcb->EvtChar;

	dcb.fInX = lpdcb->fInX;
	dcb.fOutX = lpdcb->fOutX;

	if (!SetCommState(ptr->handle,&dcb)) {
		ptr->commerror = WinError();
		return -1;
	} else {
		ptr->commerror = 0;
		return 0;
	}
}

/*****************************************************************************
 *	GetCommState	(USER.202)
 */
INT16 WINAPI GetCommState16(INT16 cid, LPDCB16 lpdcb)
{
	struct DosDeviceStruct *ptr;
	DCB dcb;

    	TRACE("cid %d, ptr %p\n", cid, lpdcb);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!\n",cid);
		return -1;
	}
	if (!GetCommState(ptr->handle,&dcb)) {
		ptr->commerror = WinError();
		return -1;
	}

	lpdcb->Id = cid;

	COMM16_DCBtoDCB16(&dcb,lpdcb);

	lpdcb->EvtChar = ptr->evtchar;

	ptr->commerror = 0;
	return 0;
}

/*****************************************************************************
 *	TransmitCommChar	(USER.206)
 */
INT16 WINAPI TransmitCommChar16(INT16 cid,CHAR chTransmit)
{
	struct DosDeviceStruct *ptr;

    	TRACE("cid %d, data %d\n", cid, chTransmit);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!\n",cid);
		return -1;
	}

	if (ptr->suspended) {
		ptr->commerror = IE_HARDWARE;
		return -1;
	}

	if (ptr->xmit >= 0) {
	  /* character already queued */
	  /* FIXME: which error would Windows return? */
	  ptr->commerror = CE_TXFULL;
	  return -1;
	}

	if (ptr->obuf_head == ptr->obuf_tail) {
	  /* transmit queue empty, try to transmit directly */
	  if(1!=COMM16_WriteFile(ptr->handle, &chTransmit, 1))
	  {
	    /* didn't work, queue it */
	    ptr->xmit = chTransmit;
	    comm_waitwrite(ptr);
	  }
	} else {
	  /* data in queue, let this char be transmitted next */
	  ptr->xmit = chTransmit;
	  comm_waitwrite(ptr);
	}

	ptr->commerror = 0;
	return 0;
}

/*****************************************************************************
 *	UngetCommChar	(USER.212)
 */
INT16 WINAPI UngetCommChar16(INT16 cid,CHAR chUnget)
{
	struct DosDeviceStruct *ptr;

    	TRACE("cid %d (char %d)\n", cid, chUnget);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!\n",cid);
		return -1;
	}

	if (ptr->suspended) {
		ptr->commerror = IE_HARDWARE;
		return -1;
	}

	if (ptr->unget>=0) {
	  /* character already queued */
	  /* FIXME: which error would Windows return? */
	  ptr->commerror = CE_RXOVER;
	  return -1;
	}

	ptr->unget = chUnget;

	ptr->commerror = 0;
	return 0;
}

/*****************************************************************************
 *	ReadComm	(USER.204)
 */
INT16 WINAPI ReadComm16(INT16 cid,LPSTR lpvBuf,INT16 cbRead)
{
	int status, length;
	struct DosDeviceStruct *ptr;
	LPSTR orgBuf = lpvBuf;

    	TRACE("cid %d, ptr %p, length %d\n", cid, lpvBuf, cbRead);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!\n",cid);
		return -1;
	}

	if (ptr->suspended) {
		ptr->commerror = IE_HARDWARE;
		return -1;
	}

	if(0==comm_inbuf(ptr))
		SleepEx(1,TRUE);

	/* read unget character */
	if (ptr->unget>=0) {
		*lpvBuf++ = ptr->unget;
		ptr->unget = -1;

		length = 1;
	} else
	 	length = 0;

	/* read from receive buffer */
	while (length < cbRead) {
	  status = ((ptr->ibuf_head < ptr->ibuf_tail) ?
		    ptr->ibuf_size : ptr->ibuf_head) - ptr->ibuf_tail;
	  if (!status) break;
	  if ((cbRead - length) < status)
	    status = cbRead - length;

	  memcpy(lpvBuf, ptr->inbuf + ptr->ibuf_tail, status);
	  ptr->ibuf_tail += status;
	  if (ptr->ibuf_tail >= ptr->ibuf_size)
	    ptr->ibuf_tail = 0;
	  lpvBuf += status;
	  length += status;
	}

	TRACE("%s\n", debugstr_an( orgBuf, length ));
	ptr->commerror = 0;
	return length;
}

/*****************************************************************************
 *	WriteComm	(USER.205)
 */
INT16 WINAPI WriteComm16(INT16 cid, LPSTR lpvBuf, INT16 cbWrite)
{
	int status, length;
	struct DosDeviceStruct *ptr;

    	TRACE("cid %d, ptr %p, length %d\n",
		cid, lpvBuf, cbWrite);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!\n",cid);
		return -1;
	}

	if (ptr->suspended) {
		ptr->commerror = IE_HARDWARE;
		return -1;
	}

	TRACE("%s\n", debugstr_an( lpvBuf, cbWrite ));

	length = 0;
	while (length < cbWrite) {
	  if ((ptr->obuf_head == ptr->obuf_tail) && (ptr->xmit < 0)) {
	    /* no data queued, try to write directly */
	    status = COMM16_WriteFile(ptr->handle, lpvBuf, cbWrite - length);
	    if (status > 0) {
	      lpvBuf += status;
	      length += status;
	      continue;
	    }
	  }
	  /* can't write directly, put into transmit buffer */
	  status = ((ptr->obuf_tail > ptr->obuf_head) ?
		    (ptr->obuf_tail-1) : ptr->obuf_size) - ptr->obuf_head;
	  if (!status) break;
	  if ((cbWrite - length) < status)
	    status = cbWrite - length;
	  memcpy(lpvBuf, ptr->outbuf + ptr->obuf_head, status);
	  ptr->obuf_head += status;
	  if (ptr->obuf_head >= ptr->obuf_size)
	    ptr->obuf_head = 0;
	  lpvBuf += status;
	  length += status;
	  comm_waitwrite(ptr);
	}

	ptr->commerror = 0;
	return length;
}

/***********************************************************************
 *           EnableCommNotification   (USER.245)
 */
BOOL16 WINAPI EnableCommNotification16( INT16 cid, HWND16 hwnd,
                                      INT16 cbWriteNotify, INT16 cbOutQueue )
{
	struct DosDeviceStruct *ptr;

	TRACE("(%d, %x, %d, %d)\n", cid, hwnd, cbWriteNotify, cbOutQueue);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!\n",cid);
		return -1;
	}
	ptr->wnd = WIN_Handle32( hwnd );
	ptr->n_read = cbWriteNotify;
	ptr->n_write = cbOutQueue;
	return TRUE;
}
