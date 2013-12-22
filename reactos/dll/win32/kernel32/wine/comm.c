/*
 * DEC 93 Erik Bos <erik@xs4all.nl>
 *
 * Copyright 1996 Marcus Meissner
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
 */

#include <k32.h>

typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;
#undef SERIAL_LSRMST_ESCAPE
#undef SERIAL_LSRMST_LSR_DATA
#undef SERIAL_LSRMST_LSR_NODATA
#undef SERIAL_LSRMST_MST
#undef SERIAL_IOC_FCR_FIFO_ENABLE
#undef SERIAL_IOC_FCR_RCVR_RESET
#undef SERIAL_IOC_FCR_XMIT_RESET
#undef SERIAL_IOC_FCR_DMA_MODE
#undef SERIAL_IOC_FCR_RES1
#undef SERIAL_IOC_FCR_RES2
#undef SERIAL_IOC_FCR_RCVR_TRIGGER_LSB
#undef SERIAL_IOC_FCR_RCVR_TRIGGER_MSB
#undef SERIAL_IOC_MCR_DTR
#undef SERIAL_IOC_MCR_RTS
#undef SERIAL_IOC_MCR_OUT1
#undef SERIAL_IOC_MCR_OUT2
#undef SERIAL_IOC_MCR_LOOP
#undef IOCTL_SERIAL_LSRMST_INSERT
#include <ntddser.h>

#define NDEBUG
#include <debug.h>
DEBUG_CHANNEL(comm);

/***********************************************************************
 *           COMM_Parse*   (Internal)
 *
 *  The following COMM_Parse* functions are used by the BuildCommDCB
 *  functions to help parse the various parts of the device control string.
 */
static LPCWSTR COMM_ParseStart(LPCWSTR ptr)
{
	static const WCHAR comW[] = {'C','O','M',0};

	/* The device control string may optionally start with "COMx" followed
	   by an optional ':' and spaces. */
	if(!strncmpiW(ptr, comW, 3))
	{
		ptr += 3;

		/* Allow any com port above 0 as Win 9x does (NT only allows
		   values for com ports which are actually present) */
		if(*ptr < '1' || *ptr > '9')
			return NULL;
		
		/* Advance pointer past port number */
		while(*ptr >= '0' && *ptr <= '9') ptr++;
		
		/* The com port number must be followed by a ':' or ' ' */
		if(*ptr != ':' && *ptr != ' ')
			return NULL;

		/* Advance pointer to beginning of next parameter */
		while(*ptr == ' ') ptr++;
		if(*ptr == ':')
		{
			ptr++;
			while(*ptr == ' ') ptr++;
		}
	}
	/* The device control string must not start with a space. */
	else if(*ptr == ' ')
		return NULL;
	
	return ptr;
}
 
static LPCWSTR COMM_ParseNumber(LPCWSTR ptr, LPDWORD lpnumber)
{
	if(*ptr < '0' || *ptr > '9') return NULL;
	*lpnumber = strtoulW(ptr, NULL, 10);
	while(*ptr >= '0' && *ptr <= '9') ptr++;
	return ptr;
}

static LPCWSTR COMM_ParseParity(LPCWSTR ptr, LPBYTE lpparity)
{
	/* Contrary to what you might expect, Windows only sets the Parity
	   member of DCB and not fParity even when parity is specified in the
	   device control string */

	switch(toupperW(*ptr++))
	{
	case 'E':
		*lpparity = EVENPARITY;
		break;
	case 'M':
		*lpparity = MARKPARITY;
		break;
	case 'N':
		*lpparity = NOPARITY;
		break;
	case 'O':
		*lpparity = ODDPARITY;
		break;
	case 'S':
		*lpparity = SPACEPARITY;
		break;
	default:
		return NULL;
	}

	return ptr;
}

static LPCWSTR COMM_ParseByteSize(LPCWSTR ptr, LPBYTE lpbytesize)
{
	DWORD temp;

	if(!(ptr = COMM_ParseNumber(ptr, &temp)))
		return NULL;

	if(temp >= 5 && temp <= 8)
	{
		*lpbytesize = temp;
		return ptr;
	}
	else
		return NULL;
}

static LPCWSTR COMM_ParseStopBits(LPCWSTR ptr, LPBYTE lpstopbits)
{
	DWORD temp;
	static const WCHAR stopbits15W[] = {'1','.','5',0};

	if(!strncmpW(stopbits15W, ptr, 3))
	{
		ptr += 3;
		*lpstopbits = ONE5STOPBITS;
	}
	else
	{
		if(!(ptr = COMM_ParseNumber(ptr, &temp)))
			return NULL;

		if(temp == 1)
			*lpstopbits = ONESTOPBIT;
		else if(temp == 2)
			*lpstopbits = TWOSTOPBITS;
		else
			return NULL;
	}
	
	return ptr;
}

static LPCWSTR COMM_ParseOnOff(LPCWSTR ptr, LPDWORD lponoff)
{
	static const WCHAR onW[] = {'o','n',0};
	static const WCHAR offW[] = {'o','f','f',0};

	if(!strncmpiW(onW, ptr, 2))
	{
		ptr += 2;
		*lponoff = 1;
	}
	else if(!strncmpiW(offW, ptr, 3))
	{
		ptr += 3;
		*lponoff = 0;
	}
	else
		return NULL;

	return ptr;
}

/***********************************************************************
 *           COMM_BuildOldCommDCB   (Internal)
 *
 *  Build a DCB using the old style settings string eg: "96,n,8,1"
 */
static BOOL COMM_BuildOldCommDCB(LPCWSTR device, LPDCB lpdcb)
{
	WCHAR last = 0;

	if(!(device = COMM_ParseNumber(device, &lpdcb->BaudRate)))
		return FALSE;
	
	switch(lpdcb->BaudRate)
	{
	case 11:
	case 30:
	case 60:
		lpdcb->BaudRate *= 10;
		break;
	case 12:
	case 24:
	case 48:
	case 96:
		lpdcb->BaudRate *= 100;
		break;
	case 19:
		lpdcb->BaudRate = 19200;
		break;
	}

	while(*device == ' ') device++;
	if(*device++ != ',') return FALSE;
	while(*device == ' ') device++;

	if(!(device = COMM_ParseParity(device, &lpdcb->Parity)))
		return FALSE;

	while(*device == ' ') device++;
	if(*device++ != ',') return FALSE;
	while(*device == ' ') device++;
		
	if(!(device = COMM_ParseByteSize(device, &lpdcb->ByteSize)))
		return FALSE;

	while(*device == ' ') device++;
	if(*device++ != ',') return FALSE;
	while(*device == ' ') device++;

	if(!(device = COMM_ParseStopBits(device, &lpdcb->StopBits)))
		return FALSE;

	/* The last parameter for flow control is optional. */
	while(*device == ' ') device++;
	if(*device == ',')
	{
		device++;
		while(*device == ' ') device++;
		if(*device) last = toupperW(*device++);
		while(*device == ' ') device++;
	}

	/* Win NT sets the flow control members based on (or lack of) the last
	   parameter.  Win 9x does not set these members. */
	switch(last)
	{
	case 0:
		lpdcb->fInX = FALSE;
		lpdcb->fOutX = FALSE;
		lpdcb->fOutxCtsFlow = FALSE;
		lpdcb->fOutxDsrFlow = FALSE;
		lpdcb->fDtrControl = DTR_CONTROL_ENABLE;
		lpdcb->fRtsControl = RTS_CONTROL_ENABLE;
		break;
	case 'X':
		lpdcb->fInX = TRUE;
		lpdcb->fOutX = TRUE;
		lpdcb->fOutxCtsFlow = FALSE;
		lpdcb->fOutxDsrFlow = FALSE;
		lpdcb->fDtrControl = DTR_CONTROL_ENABLE;
		lpdcb->fRtsControl = RTS_CONTROL_ENABLE;
		break;
	case 'P':
		lpdcb->fInX = FALSE;
		lpdcb->fOutX = FALSE;
		lpdcb->fOutxCtsFlow = TRUE;
		lpdcb->fOutxDsrFlow = TRUE;
		lpdcb->fDtrControl = DTR_CONTROL_HANDSHAKE;
		lpdcb->fRtsControl = RTS_CONTROL_HANDSHAKE;
		break;
	default:
		return FALSE;
	}

	/* This should be the end of the string. */
	if(*device) return FALSE;
	
	return TRUE;
}

/***********************************************************************
 *           COMM_BuildNewCommDCB   (Internal)
 *
 *  Build a DCB using the new style settings string.
 *   eg: "baud=9600 parity=n data=8 stop=1 xon=on to=on"
 */
static BOOL COMM_BuildNewCommDCB(LPCWSTR device, LPDCB lpdcb, LPCOMMTIMEOUTS lptimeouts)
{
	DWORD temp;
	BOOL baud = FALSE, stop = FALSE;
	static const WCHAR baudW[] = {'b','a','u','d','=',0};
	static const WCHAR parityW[] = {'p','a','r','i','t','y','=',0};
	static const WCHAR dataW[] = {'d','a','t','a','=',0};
	static const WCHAR stopW[] = {'s','t','o','p','=',0};
	static const WCHAR toW[] = {'t','o','=',0};
	static const WCHAR xonW[] = {'x','o','n','=',0};
	static const WCHAR odsrW[] = {'o','d','s','r','=',0};
	static const WCHAR octsW[] = {'o','c','t','s','=',0};
	static const WCHAR dtrW[] = {'d','t','r','=',0};
	static const WCHAR rtsW[] = {'r','t','s','=',0};
	static const WCHAR idsrW[] = {'i','d','s','r','=',0};

	while(*device)
	{
		while(*device == ' ') device++;

		if(!strncmpiW(baudW, device, 5))
		{
			baud = TRUE;
			
			if(!(device = COMM_ParseNumber(device + 5, &lpdcb->BaudRate)))
				return FALSE;
		}
		else if(!strncmpiW(parityW, device, 7))
		{
			if(!(device = COMM_ParseParity(device + 7, &lpdcb->Parity)))
				return FALSE;
		}
		else if(!strncmpiW(dataW, device, 5))
		{
			if(!(device = COMM_ParseByteSize(device + 5, &lpdcb->ByteSize)))
				return FALSE;
		}
		else if(!strncmpiW(stopW, device, 5))
		{
			stop = TRUE;
			
			if(!(device = COMM_ParseStopBits(device + 5, &lpdcb->StopBits)))
				return FALSE;
		}
		else if(!strncmpiW(toW, device, 3))
		{
			if(!(device = COMM_ParseOnOff(device + 3, &temp)))
				return FALSE;

			lptimeouts->ReadIntervalTimeout = 0;
			lptimeouts->ReadTotalTimeoutMultiplier = 0;
			lptimeouts->ReadTotalTimeoutConstant = 0;
			lptimeouts->WriteTotalTimeoutMultiplier = 0;
			lptimeouts->WriteTotalTimeoutConstant = temp ? 60000 : 0;
		}
		else if(!strncmpiW(xonW, device, 4))
		{
			if(!(device = COMM_ParseOnOff(device + 4, &temp)))
				return FALSE;

			lpdcb->fOutX = temp;
			lpdcb->fInX = temp;
		}
		else if(!strncmpiW(odsrW, device, 5))
		{
			if(!(device = COMM_ParseOnOff(device + 5, &temp)))
				return FALSE;

			lpdcb->fOutxDsrFlow = temp;
		}
		else if(!strncmpiW(octsW, device, 5))
		{
			if(!(device = COMM_ParseOnOff(device + 5, &temp)))
				return FALSE;

			lpdcb->fOutxCtsFlow = temp;
		}
		else if(!strncmpiW(dtrW, device, 4))
		{
			if(!(device = COMM_ParseOnOff(device + 4, &temp)))
				return FALSE;

			lpdcb->fDtrControl = temp;
		}
		else if(!strncmpiW(rtsW, device, 4))
		{
			if(!(device = COMM_ParseOnOff(device + 4, &temp)))
				return FALSE;

			lpdcb->fRtsControl = temp;
		}
		else if(!strncmpiW(idsrW, device, 5))
		{
			if(!(device = COMM_ParseOnOff(device + 5, &temp)))
				return FALSE;

			/* Win NT sets the fDsrSensitivity member based on the
			   idsr parameter.  Win 9x sets fOutxDsrFlow instead. */
			lpdcb->fDsrSensitivity = temp;
		}
		else
			return FALSE;

		/* After the above parsing, the next character (if not the end of
		   the string) should be a space */
		if(*device && *device != ' ')
			return FALSE;
	}

	/* If stop bits were not specified, a default is always supplied. */
	if(!stop)
	{
		if(baud && lpdcb->BaudRate == 110)
			lpdcb->StopBits = TWOSTOPBITS;
		else
			lpdcb->StopBits = ONESTOPBIT;
	}

	return TRUE;
}

/**************************************************************************
 *         BuildCommDCBA		(KERNEL32.@)
 *
 *  Updates a device control block data structure with values from an
 *  ascii device control string.  The device control string has two forms
 *  normal and extended, it must be exclusively in one or the other form.
 *
 * RETURNS
 *
 *  True on success, false on a malformed control string.
 */
BOOL WINAPI BuildCommDCBA(
    LPCSTR device, /* [in] The ascii device control string used to update the DCB. */
    LPDCB  lpdcb)  /* [out] The device control block to be updated. */
{
	return BuildCommDCBAndTimeoutsA(device,lpdcb,NULL);
}

/**************************************************************************
 *         BuildCommDCBAndTimeoutsA		(KERNEL32.@)
 *
 *  Updates a device control block data structure with values from an
 *  ascii device control string.  Taking timeout values from a timeouts
 *  struct if desired by the control string.
 *
 * RETURNS
 *
 *  True on success, false bad handles etc.
 */
BOOL WINAPI BuildCommDCBAndTimeoutsA(
    LPCSTR         device,     /* [in] The ascii device control string. */
    LPDCB          lpdcb,      /* [out] The device control block to be updated. */
    LPCOMMTIMEOUTS lptimeouts) /* [in] The COMMTIMEOUTS structure to be updated. */
{
	BOOL ret = FALSE;
	UNICODE_STRING deviceW;

	TRACE("(%s,%p,%p)\n",device,lpdcb,lptimeouts);
	if(device) RtlCreateUnicodeStringFromAsciiz(&deviceW,device);
	else deviceW.Buffer = NULL;

	if(deviceW.Buffer) ret = BuildCommDCBAndTimeoutsW(deviceW.Buffer,lpdcb,lptimeouts);

	RtlFreeUnicodeString(&deviceW);
	return ret;
}

/**************************************************************************
 *         BuildCommDCBAndTimeoutsW	(KERNEL32.@)
 *
 *  Updates a device control block data structure with values from a
 *  unicode device control string.  Taking timeout values from a timeouts
 *  struct if desired by the control string.
 *
 * RETURNS
 *
 *  True on success, false bad handles etc
 */
BOOL WINAPI BuildCommDCBAndTimeoutsW(
    LPCWSTR        devid,      /* [in] The unicode device control string. */
    LPDCB          lpdcb,      /* [out] The device control block to be updated. */
    LPCOMMTIMEOUTS lptimeouts) /* [in] The COMMTIMEOUTS structure to be updated. */
{
	DCB dcb;
	COMMTIMEOUTS timeouts;
	BOOL result;
	LPCWSTR ptr = devid;

	TRACE("(%s,%p,%p)\n",debugstr_w(devid),lpdcb,lptimeouts);

	memset(&timeouts, 0, sizeof timeouts);

	/* Set DCBlength. (Windows NT does not do this, but 9x does) */
	lpdcb->DCBlength = sizeof(DCB);

	/* Make a copy of the original data structures to work with since if
	   if there is an error in the device control string the originals
	   should not be modified (except possibly DCBlength) */
	dcb = *lpdcb;
	if(lptimeouts) timeouts = *lptimeouts;

	ptr = COMM_ParseStart(ptr);

	if(ptr == NULL)
		result = FALSE;
	else if(strchrW(ptr, ','))
		result = COMM_BuildOldCommDCB(ptr, &dcb);
	else
		result = COMM_BuildNewCommDCB(ptr, &dcb, &timeouts);

	if(result)
	{
		*lpdcb = dcb;
		if(lptimeouts) *lptimeouts = timeouts;
		return TRUE;
	}
	else
	{
		WARN("Invalid device control string: %s\n", debugstr_w(devid));
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}	
}

/**************************************************************************
 *         BuildCommDCBW		(KERNEL32.@)
 *
 *  Updates a device control block structure with values from an
 *  unicode device control string.  The device control string has two forms
 *  normal and extended, it must be exclusively in one or the other form.
 *
 * RETURNS
 *
 *  True on success, false on a malformed control string.
 */
BOOL WINAPI BuildCommDCBW(
    LPCWSTR devid, /* [in] The unicode device control string. */
    LPDCB   lpdcb) /* [out] The device control block to be updated. */
{
	return BuildCommDCBAndTimeoutsW(devid,lpdcb,NULL);
}

/*****************************************************************************
 *	SetCommBreak		(KERNEL32.@)
 *
 *  Halts the transmission of characters to a communications device.
 *
 * PARAMS
 *      handle  [in] The communications device to suspend
 *
 * RETURNS
 *
 *  True on success, and false if the communications device could not be found,
 *  the control is not supported.
 *
 * BUGS
 *
 *  Only TIOCSBRK and TIOCCBRK are supported.
 */
BOOL WINAPI SetCommBreak(HANDLE handle)
{
    DWORD dwBytesReturned;
    return DeviceIoControl(handle, IOCTL_SERIAL_SET_BREAK_ON, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
}

/*****************************************************************************
 *	ClearCommBreak		(KERNEL32.@)
 *
 *  Resumes character transmission from a communication device.
 *
 * PARAMS
 *
 *      handle [in] The halted communication device whose character transmission is to be resumed
 *
 * RETURNS
 *
 *  True on success and false if the communications device could not be found.
 *
 * BUGS
 *
 *  Only TIOCSBRK and TIOCCBRK are supported.
 */
BOOL WINAPI ClearCommBreak(HANDLE handle)
{
    DWORD dwBytesReturned;
    return DeviceIoControl(handle, IOCTL_SERIAL_SET_BREAK_OFF, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
}

/*****************************************************************************
 *	EscapeCommFunction	(KERNEL32.@)
 *
 *  Directs a communication device to perform an extended function.
 *
 * PARAMS
 *
 *      handle          [in] The communication device to perform the extended function
 *      nFunction       [in] The extended function to be performed
 *
 * RETURNS
 *
 *  True or requested data on successful completion of the command,
 *  false if the device is not present cannot execute the command
 *  or the command failed.
 */
BOOL WINAPI EscapeCommFunction(HANDLE handle, DWORD func)
{
    DWORD       ioc;
    DWORD dwBytesReturned;

    switch (func)
    {
    case CLRDTR:        ioc = IOCTL_SERIAL_CLR_DTR;             break;
    case CLRRTS:        ioc = IOCTL_SERIAL_CLR_RTS;             break;
    case SETDTR:        ioc = IOCTL_SERIAL_SET_DTR;             break;
    case SETRTS:        ioc = IOCTL_SERIAL_SET_RTS;             break;
    case SETXOFF:       ioc = IOCTL_SERIAL_SET_XOFF;            break;
    case SETXON:        ioc = IOCTL_SERIAL_SET_XON;             break;
    case SETBREAK:      ioc = IOCTL_SERIAL_SET_BREAK_ON;        break;
    case CLRBREAK:      ioc = IOCTL_SERIAL_SET_BREAK_OFF;       break;
    case RESETDEV:      ioc = IOCTL_SERIAL_RESET_DEVICE;        break;
    default:
        ERR("Unknown function code (%u)\n", func);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return DeviceIoControl(handle, ioc, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
}

/********************************************************************
 *      PurgeComm        (KERNEL32.@)
 *
 *  Terminates pending operations and/or discards buffers on a
 *  communication resource.
 *
 * PARAMS
 *
 *      handle  [in] The communication resource to be purged
 *      flags   [in] Flags for clear pending/buffer on input/output
 *
 * RETURNS
 *
 *  True on success and false if the communications handle is bad.
 */
BOOL WINAPI PurgeComm(HANDLE handle, DWORD flags)
{
    DWORD dwBytesReturned;
    return DeviceIoControl(handle, IOCTL_SERIAL_PURGE, &flags, sizeof(flags),
                           NULL, 0, &dwBytesReturned, NULL);
}

/*****************************************************************************
 *	ClearCommError	(KERNEL32.@)
 *
 *  Enables further I/O operations on a communications resource after
 *  supplying error and current status information.
 *
 * PARAMS
 *
 *      handle  [in]    The communication resource with the error
 *      errors  [out]   Flags indicating error the resource experienced
 *      lpStat  [out] The status of the communication resource
 * RETURNS
 *
 *  True on success, false if the communication resource handle is bad.
 */
BOOL WINAPI ClearCommError(HANDLE handle, LPDWORD errors, LPCOMSTAT lpStat)
{
    SERIAL_STATUS       ss;
    DWORD dwBytesReturned;

    if (!DeviceIoControl(handle, IOCTL_SERIAL_GET_COMMSTATUS, NULL, 0,
                         &ss, sizeof(ss), &dwBytesReturned, NULL))
        return FALSE;

    if (errors)
    {
        *errors = 0;
        if (ss.Errors & SERIAL_ERROR_BREAK)             *errors |= CE_BREAK;
        if (ss.Errors & SERIAL_ERROR_FRAMING)           *errors |= CE_FRAME;
        if (ss.Errors & SERIAL_ERROR_OVERRUN)           *errors |= CE_OVERRUN;
        if (ss.Errors & SERIAL_ERROR_QUEUEOVERRUN)      *errors |= CE_RXOVER;
        if (ss.Errors & SERIAL_ERROR_PARITY)            *errors |= CE_RXPARITY;
    }
 
    if (lpStat)
    {
        memset(lpStat, 0, sizeof(*lpStat));

        if (ss.HoldReasons & SERIAL_TX_WAITING_FOR_CTS)         lpStat->fCtsHold = TRUE;
        if (ss.HoldReasons & SERIAL_TX_WAITING_FOR_DSR)         lpStat->fDsrHold = TRUE;
        if (ss.HoldReasons & SERIAL_TX_WAITING_FOR_DCD)         lpStat->fRlsdHold = TRUE;
        if (ss.HoldReasons & SERIAL_TX_WAITING_FOR_XON)         lpStat->fXoffHold = TRUE;
        if (ss.HoldReasons & SERIAL_TX_WAITING_XOFF_SENT)       lpStat->fXoffSent = TRUE;
        if (ss.EofReceived)                                     lpStat->fEof = TRUE;
        if (ss.WaitForImmediate)                                lpStat->fTxim = TRUE;
        lpStat->cbInQue = ss.AmountInInQueue;
        lpStat->cbOutQue = ss.AmountInOutQueue;
    }
    return TRUE;
}

/*****************************************************************************
 *      SetupComm       (KERNEL32.@)
 *
 *  Called after CreateFile to hint to the communication resource to use
 *  specified sizes for input and output buffers rather than the default values.
 *
 * PARAMS
 *      handle  [in]    The just created communication resource handle
 *      insize  [in]    The suggested size of the communication resources input buffer in bytes
 *      outsize [in]    The suggested size of the communication resources output buffer in bytes
 *
 * RETURNS
 *
 *  True if successful, false if the communications resource handle is bad.
 *
 * BUGS
 *
 *  Stub.
 */
BOOL WINAPI SetupComm(HANDLE handle, DWORD insize, DWORD outsize)
{
    SERIAL_QUEUE_SIZE   sqs;
    DWORD dwBytesReturned;

    sqs.InSize = insize;
    sqs.OutSize = outsize;
    return DeviceIoControl(handle, IOCTL_SERIAL_SET_QUEUE_SIZE,
                           &sqs, sizeof(sqs), NULL, 0, &dwBytesReturned, NULL);
}

/*****************************************************************************
 *	GetCommMask	(KERNEL32.@)
 *
 *  Obtain the events associated with a communication device that will cause
 *  a call WaitCommEvent to return.
 *
 *  PARAMS
 *
 *      handle  [in]    The communications device
 *      evtmask [out]   The events which cause WaitCommEvent to return
 *
 *  RETURNS
 *
 *   True on success, fail on bad device handle etc.
 */
BOOL WINAPI GetCommMask(HANDLE handle, LPDWORD evtmask)
{
    DWORD dwBytesReturned;
    TRACE("handle %p, mask %p\n", handle, evtmask);
    return DeviceIoControl(handle, IOCTL_SERIAL_GET_WAIT_MASK,
                           NULL, 0, evtmask, sizeof(*evtmask), &dwBytesReturned, NULL);
}

/*****************************************************************************
 *	SetCommMask	(KERNEL32.@)
 *
 *  There be some things we need to hear about yon there communications device.
 *  (Set which events associated with a communication device should cause
 *  a call WaitCommEvent to return.)
 *
 * PARAMS
 *
 *      handle  [in]    The communications device
 *      evtmask [in]    The events that are to be monitored
 *
 * RETURNS
 *
 *  True on success, false on bad handle etc.
 */
BOOL WINAPI SetCommMask(HANDLE handle, DWORD evtmask)
{
    DWORD dwBytesReturned;
    TRACE("handle %p, mask %x\n", handle, evtmask);
    return DeviceIoControl(handle, IOCTL_SERIAL_SET_WAIT_MASK,
                           &evtmask, sizeof(evtmask), NULL, 0, &dwBytesReturned, NULL);
}

static void dump_dcb(const DCB* lpdcb)
{
    TRACE("bytesize=%d baudrate=%d fParity=%d Parity=%d stopbits=%d\n",
          lpdcb->ByteSize, lpdcb->BaudRate, lpdcb->fParity, lpdcb->Parity,
          (lpdcb->StopBits == ONESTOPBIT) ? 1 :
          (lpdcb->StopBits == TWOSTOPBITS) ? 2 : 0);
    TRACE("%sIXON %sIXOFF\n", (lpdcb->fOutX) ? "" : "~", (lpdcb->fInX) ? "" : "~");
    TRACE("fOutxCtsFlow=%d fRtsControl=%d\n", lpdcb->fOutxCtsFlow, lpdcb->fRtsControl);
    TRACE("fOutxDsrFlow=%d fDtrControl=%d\n", lpdcb->fOutxDsrFlow, lpdcb->fDtrControl);
    if (lpdcb->fOutxCtsFlow || lpdcb->fRtsControl == RTS_CONTROL_HANDSHAKE)
        TRACE("CRTSCTS\n");
    else
        TRACE("~CRTSCTS\n");
}

/*****************************************************************************
 *	SetCommState    (KERNEL32.@)
 *
 *  Re-initializes all hardware and control settings of a communications device,
 *  with values from a device control block without affecting the input and output
 *  queues.
 *
 * PARAMS
 *
 *      handle  [in]    The communications device
 *      lpdcb   [out]   The device control block
 *
 * RETURNS
 *
 *  True on success, false on failure, e.g., if the XonChar is equal to the XoffChar.
 */
BOOL WINAPI SetCommState( HANDLE handle, LPDCB lpdcb)
{
    SERIAL_BAUD_RATE           sbr;
    SERIAL_LINE_CONTROL        slc;
    SERIAL_HANDFLOW            shf;
    SERIAL_CHARS               sc;
    DWORD dwBytesReturned;

    if (lpdcb == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    dump_dcb(lpdcb);

    sbr.BaudRate = lpdcb->BaudRate;
             
    slc.StopBits = lpdcb->StopBits;
    slc.Parity = lpdcb->Parity;
    slc.WordLength = lpdcb->ByteSize;

    shf.ControlHandShake = 0;
    shf.FlowReplace = 0;
    if (lpdcb->fOutxCtsFlow)      shf.ControlHandShake |= SERIAL_CTS_HANDSHAKE;
    if (lpdcb->fOutxDsrFlow)      shf.ControlHandShake |= SERIAL_DSR_HANDSHAKE;
    switch (lpdcb->fDtrControl)
    {
    case DTR_CONTROL_DISABLE:                                                  break;
    case DTR_CONTROL_ENABLE:      shf.ControlHandShake |= SERIAL_DTR_CONTROL;  break;
    case DTR_CONTROL_HANDSHAKE:   shf.ControlHandShake |= SERIAL_DTR_HANDSHAKE;break;
    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    switch (lpdcb->fRtsControl)
    {
    case RTS_CONTROL_DISABLE:                                                  break;
    case RTS_CONTROL_ENABLE:      shf.FlowReplace |= SERIAL_RTS_CONTROL;       break;
    case RTS_CONTROL_HANDSHAKE:   shf.FlowReplace |= SERIAL_RTS_HANDSHAKE;     break;
    case RTS_CONTROL_TOGGLE:      shf.FlowReplace |= SERIAL_RTS_CONTROL | 
                                                     SERIAL_RTS_HANDSHAKE;     break;
    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (lpdcb->fDsrSensitivity)   shf.ControlHandShake |= SERIAL_DSR_SENSITIVITY;
    if (lpdcb->fAbortOnError)     shf.ControlHandShake |= SERIAL_ERROR_ABORT;

    if (lpdcb->fErrorChar)        shf.FlowReplace |= SERIAL_ERROR_CHAR;
    if (lpdcb->fNull)             shf.FlowReplace |= SERIAL_NULL_STRIPPING;
    if (lpdcb->fTXContinueOnXoff) shf.FlowReplace |= SERIAL_XOFF_CONTINUE;
    if (lpdcb->fOutX)             shf.FlowReplace |= SERIAL_AUTO_TRANSMIT;
    if (lpdcb->fInX)              shf.FlowReplace |= SERIAL_AUTO_RECEIVE;

    shf.XonLimit = lpdcb->XonLim;
    shf.XoffLimit = lpdcb->XoffLim;

    sc.EofChar = lpdcb->EofChar;
    sc.ErrorChar = lpdcb->ErrorChar;
    sc.BreakChar = 0;
    sc.EventChar = lpdcb->EvtChar;
    sc.XonChar = lpdcb->XonChar;
    sc.XoffChar = lpdcb->XoffChar;

    /* note: change DTR/RTS lines after setting the comm attributes,
     * so flow control does not interfere.
     */
    return (DeviceIoControl(handle, IOCTL_SERIAL_SET_BAUD_RATE,
                            &sbr, sizeof(sbr), NULL, 0, &dwBytesReturned, NULL) &&
            DeviceIoControl(handle, IOCTL_SERIAL_SET_LINE_CONTROL,
                            &slc, sizeof(slc), NULL, 0, &dwBytesReturned, NULL) &&
            DeviceIoControl(handle, IOCTL_SERIAL_SET_HANDFLOW,
                            &shf, sizeof(shf), NULL, 0, &dwBytesReturned, NULL) &&
            DeviceIoControl(handle, IOCTL_SERIAL_SET_CHARS,
                            &sc, sizeof(sc), NULL, 0, &dwBytesReturned, NULL));
}


/*****************************************************************************
 *	GetCommState	(KERNEL32.@)
 *
 *  Fills in a device control block with information from a communications device.
 *
 * PARAMS
 *      handle          [in]    The communications device
 *      lpdcb           [out]   The device control block
 *
 * RETURNS
 *
 *  True on success, false if the communication device handle is bad etc
 *
 * BUGS
 *
 *  XonChar and XoffChar are not set.
 */
BOOL WINAPI GetCommState(HANDLE handle, LPDCB lpdcb)
{
    SERIAL_BAUD_RATE    sbr;
    SERIAL_LINE_CONTROL slc;
    SERIAL_HANDFLOW     shf;
    SERIAL_CHARS        sc;
    DWORD dwBytesReturned;

    TRACE("handle %p, ptr %p\n", handle, lpdcb);

    if (!lpdcb)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    if (!DeviceIoControl(handle, IOCTL_SERIAL_GET_BAUD_RATE,
                         NULL, 0, &sbr, sizeof(sbr), &dwBytesReturned, NULL) ||
        !DeviceIoControl(handle, IOCTL_SERIAL_GET_LINE_CONTROL,
                         NULL, 0, &slc, sizeof(slc), &dwBytesReturned, NULL) ||
        !DeviceIoControl(handle, IOCTL_SERIAL_GET_HANDFLOW,
                         NULL, 0, &shf, sizeof(shf), &dwBytesReturned, NULL) ||
        !DeviceIoControl(handle, IOCTL_SERIAL_GET_CHARS,
                         NULL, 0, &sc, sizeof(sc), &dwBytesReturned, NULL))
        return FALSE;

    memset(lpdcb, 0, sizeof(*lpdcb));
    lpdcb->DCBlength = sizeof(*lpdcb);

    /* yes, they seem no never be (re)set on NT */
    lpdcb->fBinary = 1;
    lpdcb->fParity = 0;

    lpdcb->BaudRate = sbr.BaudRate;

    lpdcb->StopBits = slc.StopBits;
    lpdcb->Parity = slc.Parity;
    lpdcb->ByteSize = slc.WordLength;

    if (shf.ControlHandShake & SERIAL_CTS_HANDSHAKE)    lpdcb->fOutxCtsFlow = 1;
    if (shf.ControlHandShake & SERIAL_DSR_HANDSHAKE)    lpdcb->fOutxDsrFlow = 1;
    switch (shf.ControlHandShake & (SERIAL_DTR_CONTROL | SERIAL_DTR_HANDSHAKE))
    {
    case 0:                     lpdcb->fDtrControl = DTR_CONTROL_DISABLE; break;
    case SERIAL_DTR_CONTROL:    lpdcb->fDtrControl = DTR_CONTROL_ENABLE; break;
    case SERIAL_DTR_HANDSHAKE:  lpdcb->fDtrControl = DTR_CONTROL_HANDSHAKE; break;
    }
    switch (shf.FlowReplace & (SERIAL_RTS_CONTROL | SERIAL_RTS_HANDSHAKE))
    {
    case 0:                     lpdcb->fRtsControl = RTS_CONTROL_DISABLE; break;
    case SERIAL_RTS_CONTROL:    lpdcb->fRtsControl = RTS_CONTROL_ENABLE; break;
    case SERIAL_RTS_HANDSHAKE:  lpdcb->fRtsControl = RTS_CONTROL_HANDSHAKE; break;
    case SERIAL_RTS_CONTROL | SERIAL_RTS_HANDSHAKE:
                                lpdcb->fRtsControl = RTS_CONTROL_TOGGLE; break;
    }
    if (shf.ControlHandShake & SERIAL_DSR_SENSITIVITY)  lpdcb->fDsrSensitivity = 1;
    if (shf.ControlHandShake & SERIAL_ERROR_ABORT)      lpdcb->fAbortOnError = 1;
    if (shf.FlowReplace & SERIAL_ERROR_CHAR)            lpdcb->fErrorChar = 1;
    if (shf.FlowReplace & SERIAL_NULL_STRIPPING)        lpdcb->fNull = 1;
    if (shf.FlowReplace & SERIAL_XOFF_CONTINUE)         lpdcb->fTXContinueOnXoff = 1;
    lpdcb->XonLim = shf.XonLimit;
    lpdcb->XoffLim = shf.XoffLimit;

    if (shf.FlowReplace & SERIAL_AUTO_TRANSMIT) lpdcb->fOutX = 1;
    if (shf.FlowReplace & SERIAL_AUTO_RECEIVE)  lpdcb->fInX = 1;

    lpdcb->EofChar = sc.EofChar;
    lpdcb->ErrorChar = sc.ErrorChar;
    lpdcb->EvtChar = sc.EventChar;
    lpdcb->XonChar = sc.XonChar;
    lpdcb->XoffChar = sc.XoffChar;

    TRACE("OK\n");
    dump_dcb(lpdcb);

    return TRUE;
}

/*****************************************************************************
 *	TransmitCommChar	(KERNEL32.@)
 *
 *  Transmits a single character in front of any pending characters in the
 *  output buffer.  Usually used to send an interrupt character to a host.
 *
 * PARAMS
 *      hComm           [in]    The communication device in need of a command character
 *      chTransmit      [in]    The character to transmit
 *
 * RETURNS
 *
 *  True if the call succeeded, false if the previous command character to the
 *  same device has not been sent yet the handle is bad etc.
 *
 */
BOOL WINAPI TransmitCommChar(HANDLE hComm, CHAR chTransmit)
{
    DWORD dwBytesReturned;
    return DeviceIoControl(hComm, IOCTL_SERIAL_IMMEDIATE_CHAR,
                           &chTransmit, sizeof(chTransmit), NULL, 0, &dwBytesReturned, NULL);
}


/*****************************************************************************
 *	GetCommTimeouts		(KERNEL32.@)
 *
 *  Obtains the request timeout values for the communications device.
 *
 * PARAMS
 *      hComm           [in]    The communications device
 *      lptimeouts      [out]   The struct of request timeouts
 *
 * RETURNS
 *
 *  True on success, false if communications device handle is bad
 *  or the target structure is null.
 */
BOOL WINAPI GetCommTimeouts(HANDLE hComm, LPCOMMTIMEOUTS lptimeouts)
{
    SERIAL_TIMEOUTS     st;
    DWORD dwBytesReturned;

    TRACE("(%p, %p)\n", hComm, lptimeouts);
    if (!lptimeouts)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!DeviceIoControl(hComm, IOCTL_SERIAL_GET_TIMEOUTS,
                         NULL, 0, &st, sizeof(st), &dwBytesReturned, NULL))
        return FALSE;
    lptimeouts->ReadIntervalTimeout         = st.ReadIntervalTimeout;
    lptimeouts->ReadTotalTimeoutMultiplier  = st.ReadTotalTimeoutMultiplier;
    lptimeouts->ReadTotalTimeoutConstant    = st.ReadTotalTimeoutConstant;
    lptimeouts->WriteTotalTimeoutMultiplier = st.WriteTotalTimeoutMultiplier;
    lptimeouts->WriteTotalTimeoutConstant   = st.WriteTotalTimeoutConstant;
    return TRUE;
}

/*****************************************************************************
 *	SetCommTimeouts		(KERNEL32.@)
 *
 * Sets the timeouts used when reading and writing data to/from COMM ports.
 *
 * PARAMS
 *      hComm           [in]    handle of COMM device
 *      lptimeouts      [in]    pointer to COMMTIMEOUTS structure
 *
 * ReadIntervalTimeout
 *     - converted and passes to linux kernel as c_cc[VTIME]
 * ReadTotalTimeoutMultiplier, ReadTotalTimeoutConstant
 *     - used in ReadFile to calculate GetOverlappedResult's timeout
 * WriteTotalTimeoutMultiplier, WriteTotalTimeoutConstant
 *     - used in WriteFile to calculate GetOverlappedResult's timeout
 *
 * RETURNS
 *
 *  True if the timeouts were set, false otherwise.
 */
BOOL WINAPI SetCommTimeouts(HANDLE hComm, LPCOMMTIMEOUTS lptimeouts)
{
    SERIAL_TIMEOUTS     st;
    DWORD dwBytesReturned;

    TRACE("(%p, %p)\n", hComm, lptimeouts);

    if (lptimeouts == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    st.ReadIntervalTimeout         = lptimeouts->ReadIntervalTimeout;
    st.ReadTotalTimeoutMultiplier  = lptimeouts->ReadTotalTimeoutMultiplier;
    st.ReadTotalTimeoutConstant    = lptimeouts->ReadTotalTimeoutConstant;
    st.WriteTotalTimeoutMultiplier = lptimeouts->WriteTotalTimeoutMultiplier;
    st.WriteTotalTimeoutConstant   = lptimeouts->WriteTotalTimeoutConstant;
 
    return DeviceIoControl(hComm, IOCTL_SERIAL_SET_TIMEOUTS,
                           &st, sizeof(st), NULL, 0, &dwBytesReturned, NULL);
}

/***********************************************************************
 *           GetCommModemStatus   (KERNEL32.@)
 *
 *  Obtains the four control register bits if supported by the hardware.
 *
 * PARAMS
 *
 *      hFile           [in]    The communications device
 *      lpModemStat     [out]   The control register bits
 *
 * RETURNS
 *
 *  True if the communications handle was good and for hardware that
 *  control register access, false otherwise.
 */
BOOL WINAPI GetCommModemStatus(HANDLE hFile, LPDWORD lpModemStat)
{
    DWORD dwBytesReturned;
    return DeviceIoControl(hFile, IOCTL_SERIAL_GET_MODEMSTATUS,
                           NULL, 0, lpModemStat, sizeof(DWORD), &dwBytesReturned, NULL);
}

/***********************************************************************
 *           WaitCommEvent   (KERNEL32.@)
 *
 * Wait until something interesting happens on a COMM port.
 * Interesting things (events) are set by calling SetCommMask before
 * this function is called.
 *
 * RETURNS
 *   TRUE if successful
 *   FALSE if failure
 *
 *   The set of detected events will be written to *lpdwEventMask
 *   ERROR_IO_PENDING will be returned the overlapped structure was passed
 *
 * BUGS:
 *  Only supports EV_RXCHAR and EV_TXEMPTY
 */
BOOL WINAPI WaitCommEvent(
    HANDLE hFile,              /* [in] handle of comm port to wait for */
    LPDWORD lpdwEvents,        /* [out] event(s) that were detected */
    LPOVERLAPPED lpOverlapped) /* [in/out] for Asynchronous waiting */
{
    return DeviceIoControl(hFile, IOCTL_SERIAL_WAIT_ON_MASK, NULL, 0,
                           lpdwEvents, sizeof(DWORD), NULL, lpOverlapped);
}

/***********************************************************************
 *           GetCommProperties   (KERNEL32.@)
 *
 * This function fills in a structure with the capabilities of the
 * communications port driver.
 *
 * RETURNS
 *
 *  TRUE on success, FALSE on failure
 *  If successful, the lpCommProp structure be filled in with
 *  properties of the comm port.
 */
BOOL WINAPI GetCommProperties(
    HANDLE hFile,          /* [in] handle of the comm port */
    LPCOMMPROP lpCommProp) /* [out] pointer to struct to be filled */
{
    TRACE("(%p %p)\n",hFile,lpCommProp);
    if(!lpCommProp)
        return FALSE;

    /*
     * These values should be valid for LINUX's serial driver
     * FIXME: Perhaps they deserve an #ifdef LINUX
     */
    memset(lpCommProp,0,sizeof(COMMPROP));
    lpCommProp->wPacketLength       = 1;
    lpCommProp->wPacketVersion      = 1;
    lpCommProp->dwServiceMask       = SP_SERIALCOMM;
    lpCommProp->dwMaxTxQueue        = 4096;
    lpCommProp->dwMaxRxQueue        = 4096;
    lpCommProp->dwMaxBaud           = BAUD_115200;
    lpCommProp->dwProvSubType       = PST_RS232;
    lpCommProp->dwProvCapabilities  = PCF_DTRDSR | PCF_PARITY_CHECK | PCF_RTSCTS | PCF_TOTALTIMEOUTS;
    lpCommProp->dwSettableParams    = SP_BAUD | SP_DATABITS | SP_HANDSHAKING |
                                      SP_PARITY | SP_PARITY_CHECK | SP_STOPBITS ;
    lpCommProp->dwSettableBaud      = BAUD_075 | BAUD_110 | BAUD_134_5 | BAUD_150 |
                BAUD_300 | BAUD_600 | BAUD_1200 | BAUD_1800 | BAUD_2400 | BAUD_4800 |
                BAUD_9600 | BAUD_19200 | BAUD_38400 | BAUD_57600 | BAUD_115200 ;
    lpCommProp->wSettableData       = DATABITS_5 | DATABITS_6 | DATABITS_7 | DATABITS_8 ;
    lpCommProp->wSettableStopParity = STOPBITS_10 | STOPBITS_15 | STOPBITS_20 |
                PARITY_NONE | PARITY_ODD |PARITY_EVEN | PARITY_MARK | PARITY_SPACE;
    lpCommProp->dwCurrentTxQueue    = lpCommProp->dwMaxTxQueue;
    lpCommProp->dwCurrentRxQueue    = lpCommProp->dwMaxRxQueue;

    return TRUE;
}

/***********************************************************************
 * FIXME:
 * The functionality of CommConfigDialogA, GetDefaultCommConfig and
 * SetDefaultCommConfig is implemented in a DLL (usually SERIALUI.DLL).
 * This is dependent on the type of COMM port, but since it is doubtful
 * anybody will get around to implementing support for fancy serial
 * ports in WINE, this is hardcoded for the time being.  The name of
 * this DLL should be stored in and read from the system registry in
 * the hive HKEY_LOCAL_MACHINE, key
 * System\\CurrentControlSet\\Services\\Class\\Ports\\????
 * where ???? is the port number... that is determined by PNP
 * The DLL should be loaded when the COMM port is opened, and closed
 * when the COMM port is closed. - MJM 20 June 2000
 ***********************************************************************/
static const WCHAR lpszSerialUI[] = { 
   's','e','r','i','a','l','u','i','.','d','l','l',0 };


/***********************************************************************
 *           CommConfigDialogA   (KERNEL32.@)
 *
 * Raises a dialog that allows the user to configure a comm port.
 * Fills the COMMCONFIG struct with information specified by the user.
 * This function should call a similar routine in the COMM driver...
 *
 * RETURNS
 *
 *  TRUE on success, FALSE on failure
 *  If successful, the lpCommConfig structure will contain a new
 *  configuration for the comm port, as specified by the user.
 *
 * BUGS
 *  The library with the CommConfigDialog code is never unloaded.
 * Perhaps this should be done when the comm port is closed?
 */
BOOL WINAPI CommConfigDialogA(
    LPCSTR lpszDevice,         /* [in] name of communications device */
    HWND hWnd,                 /* [in] parent window for the dialog */
    LPCOMMCONFIG lpCommConfig) /* [out] pointer to struct to fill */
{
    LPWSTR  lpDeviceW = NULL;
    DWORD   len;
    BOOL    r;

    TRACE("(%s, %p, %p)\n", debugstr_a(lpszDevice), hWnd, lpCommConfig);

    if (lpszDevice)
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpszDevice, -1, NULL, 0 );
        lpDeviceW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, lpszDevice, -1, lpDeviceW, len );
    }
    r = CommConfigDialogW(lpDeviceW, hWnd, lpCommConfig);
    HeapFree( GetProcessHeap(), 0, lpDeviceW );
    return r;
}

/***********************************************************************
 *           CommConfigDialogW   (KERNEL32.@)
 *
 * See CommConfigDialogA.
 */
BOOL WINAPI CommConfigDialogW(
    LPCWSTR lpszDevice,        /* [in] name of communications device */
    HWND hWnd,                 /* [in] parent window for the dialog */
    LPCOMMCONFIG lpCommConfig) /* [out] pointer to struct to fill */
{
    DWORD (WINAPI *pCommConfigDialog)(LPCWSTR, HWND, LPCOMMCONFIG);
    HMODULE hConfigModule;
    DWORD   res = ERROR_INVALID_PARAMETER;

    TRACE("(%s, %p, %p)\n", debugstr_w(lpszDevice), hWnd, lpCommConfig);
    hConfigModule = LoadLibraryW(lpszSerialUI);

    if (hConfigModule) {
        pCommConfigDialog = (void *)GetProcAddress(hConfigModule, "drvCommConfigDialogW");
        if (pCommConfigDialog) {
            res = pCommConfigDialog(lpszDevice, hWnd, lpCommConfig);
        }
        FreeLibrary(hConfigModule);
    }

    if (res) SetLastError(res);
    return (res == ERROR_SUCCESS);
}

/***********************************************************************
 *           GetCommConfig     (KERNEL32.@)
 *
 * Fill in the COMMCONFIG structure for the comm port hFile
 *
 * RETURNS
 *
 *  TRUE on success, FALSE on failure
 *  If successful, lpCommConfig contains the comm port configuration.
 *
 * BUGS
 *
 */
BOOL WINAPI GetCommConfig(
    HANDLE       hFile,        /* [in] The communications device. */
    LPCOMMCONFIG lpCommConfig, /* [out] The communications configuration of the device (if it fits). */
    LPDWORD      lpdwSize)     /* [in/out] Initially the size of the configuration buffer/structure,
                                  afterwards the number of bytes copied to the buffer or
                                  the needed size of the buffer. */
{
    BOOL r;

    TRACE("(%p, %p, %p) *lpdwSize: %u\n", hFile, lpCommConfig, lpdwSize, lpdwSize ? *lpdwSize : 0 );

    if(lpCommConfig == NULL)
        return FALSE;
    r = *lpdwSize < sizeof(COMMCONFIG); /* TRUE if not enough space */
    *lpdwSize = sizeof(COMMCONFIG);
    if(r)
        return FALSE;

    lpCommConfig->dwSize = sizeof(COMMCONFIG);
    lpCommConfig->wVersion = 1;
    lpCommConfig->wReserved = 0;
    r = GetCommState(hFile,&lpCommConfig->dcb);
    lpCommConfig->dwProviderSubType = PST_RS232;
    lpCommConfig->dwProviderOffset = 0;
    lpCommConfig->dwProviderSize = 0;

    return r;
}

/***********************************************************************
 *           SetCommConfig     (KERNEL32.@)
 *
 *  Sets the configuration of the communications device.
 *
 * RETURNS
 *
 *  True on success, false if the handle was bad is not a communications device.
 */
BOOL WINAPI SetCommConfig(
    HANDLE       hFile,		/* [in] The communications device. */
    LPCOMMCONFIG lpCommConfig,	/* [in] The desired configuration. */
    DWORD dwSize) 		/* [in] size of the lpCommConfig struct */
{
    TRACE("(%p, %p, %u)\n", hFile, lpCommConfig, dwSize);
    return SetCommState(hFile,&lpCommConfig->dcb);
}

/***********************************************************************
 *           SetDefaultCommConfigW  (KERNEL32.@)
 *
 * Initializes the default configuration for a communication device.
 *
 * PARAMS
 *  lpszDevice   [I] Name of the device targeted for configuration
 *  lpCommConfig [I] PTR to a buffer with the configuration for the device
 *  dwSize       [I] Number of bytes in the buffer
 *
 * RETURNS
 *  Failure: FALSE
 *  Success: TRUE, and default configuration saved
 *
 */
BOOL WINAPI SetDefaultCommConfigW(LPCWSTR lpszDevice, LPCOMMCONFIG lpCommConfig, DWORD dwSize)
{
    BOOL (WINAPI *lpfnSetDefaultCommConfig)(LPCWSTR, LPCOMMCONFIG, DWORD);
    HMODULE hConfigModule;
    BOOL r = FALSE;

    TRACE("(%s, %p, %u)\n", debugstr_w(lpszDevice), lpCommConfig, dwSize);

    hConfigModule = LoadLibraryW(lpszSerialUI);
    if(!hConfigModule)
        return r;

    lpfnSetDefaultCommConfig = (void *)GetProcAddress(hConfigModule, "drvSetDefaultCommConfigW");
    if (lpfnSetDefaultCommConfig)
        r = lpfnSetDefaultCommConfig(lpszDevice, lpCommConfig, dwSize);

    FreeLibrary(hConfigModule);

    return r;
}


/***********************************************************************
 *           SetDefaultCommConfigA     (KERNEL32.@)
 *
 * Initializes the default configuration for a communication device.
 *
 * See SetDefaultCommConfigW.
 *
 */
BOOL WINAPI SetDefaultCommConfigA(LPCSTR lpszDevice, LPCOMMCONFIG lpCommConfig, DWORD dwSize)
{
    BOOL r;
    LPWSTR lpDeviceW = NULL;
    DWORD len;

    TRACE("(%s, %p, %u)\n", debugstr_a(lpszDevice), lpCommConfig, dwSize);

    if (lpszDevice)
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpszDevice, -1, NULL, 0 );
        lpDeviceW = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, lpszDevice, -1, lpDeviceW, len );
    }
    r = SetDefaultCommConfigW(lpDeviceW,lpCommConfig,dwSize);
    HeapFree( GetProcessHeap(), 0, lpDeviceW );
    return r;
}


/***********************************************************************
 *           GetDefaultCommConfigW   (KERNEL32.@)
 *
 *   Acquires the default configuration of the specified communication device. (unicode)
 *
 *  RETURNS
 *
 *   True on successful reading of the default configuration,
 *   if the device is not found or the buffer is too small.
 */
BOOL WINAPI GetDefaultCommConfigW(
    LPCWSTR      lpszName, /* [in] The unicode name of the device targeted for configuration. */
    LPCOMMCONFIG lpCC,     /* [out] The default configuration for the device. */
    LPDWORD      lpdwSize) /* [in/out] Initially the size of the default configuration buffer,
                              afterwards the number of bytes copied to the buffer or
                              the needed size of the buffer. */
{
    DWORD (WINAPI *pGetDefaultCommConfig)(LPCWSTR, LPCOMMCONFIG, LPDWORD);
    HMODULE hConfigModule;
    DWORD   res = ERROR_INVALID_PARAMETER;

    TRACE("(%s, %p, %p)  *lpdwSize: %u\n", debugstr_w(lpszName), lpCC, lpdwSize, lpdwSize ? *lpdwSize : 0 );
    hConfigModule = LoadLibraryW(lpszSerialUI);

    if (hConfigModule) {
        pGetDefaultCommConfig = (void *)GetProcAddress(hConfigModule, "drvGetDefaultCommConfigW");
        if (pGetDefaultCommConfig) {
            res = pGetDefaultCommConfig(lpszName, lpCC, lpdwSize);
        }
        FreeLibrary(hConfigModule);
    }

    if (res) SetLastError(res);
    return (res == ERROR_SUCCESS);
}

/**************************************************************************
 *         GetDefaultCommConfigA		(KERNEL32.@)
 *
 *   Acquires the default configuration of the specified communication device. (ascii)
 *
 *  RETURNS
 *
 *   True on successful reading of the default configuration,
 *   if the device is not found or the buffer is too small.
 */
BOOL WINAPI GetDefaultCommConfigA(
    LPCSTR       lpszName, /* [in] The ascii name of the device targeted for configuration. */
    LPCOMMCONFIG lpCC,     /* [out] The default configuration for the device. */
    LPDWORD      lpdwSize) /* [in/out] Initially the size of the default configuration buffer,
			      afterwards the number of bytes copied to the buffer or
                              the needed size of the buffer. */
{
	BOOL ret = FALSE;
	UNICODE_STRING lpszNameW;

	TRACE("(%s, %p, %p)  *lpdwSize: %u\n", debugstr_a(lpszName), lpCC, lpdwSize, lpdwSize ? *lpdwSize : 0 );
	if(lpszName) RtlCreateUnicodeStringFromAsciiz(&lpszNameW,lpszName);
	else lpszNameW.Buffer = NULL;

	ret = GetDefaultCommConfigW(lpszNameW.Buffer,lpCC,lpdwSize);

	RtlFreeUnicodeString(&lpszNameW);
	return ret;
}
