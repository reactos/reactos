/* $Id: ioctl_async.cpp,v 1.2 2004/01/31 01:29:11 robertk Exp $
*/
/*
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS OS/2 sub system
 * PART:			 doscalls.dll
 * FILE:             ipctl_async.cpp
 * PURPOSE:          This file is to be directly included into devices.cpp
 * CONTAINS:		 implementation of the category 1 (async I/O)
 *						ioctl functioncodes.
 * PROGRAMMER:       Robert K. nonvolatil@yahoo.de
 * REVISION HISTORY:
 *  10-11-2002  Created
 */


/*  This function implements the async ioctls. It
	is called from the real DosIOCtl function. This
	function implies that it is only called, if the
	cathegory was 0x01. 
	All other parameters are the same as DosIoCtl. 

	FIXME: implement me
*/
APIRET EvaluateAsyncIoCtl( HFILE hDevice, ULONG function,
		PVOID pParams,  ULONG cbParmLenMax, PULONG pcbParmLen,
		PVOID pData, ULONG cbDataLenMax, PULONG pcbDataLen)
{
	switch( function )
	{
	case 0x41: //ASYNC_SETBAUDRATE
				//+------------------------------------+
				//|Field        Length     C Datatype  |
				//|------------------------------------|
				//|Bit Rate     WORD       USHORT      |
				//+------------------------------------+
		break;
		
	case 0x42: // Set Line Characteristics (stop, parity, data bits)
		return ERROR_INVALID_PARAMETER;	// example
		break;
	case 0x43: // Extended Set Bit Rate                           .
		break;
	case 0x44: // Transmit Byte Immediate                         .
		break;
	case 0x45: // Set Break OFF                                   .
		break;
	case 0x46: // Set Modem Control Signals                       .
		break;
	case 0x47: // Behave as if XOFF Received (stop transmit)      .
		break;
	case 0x48: // Behave as if XON Received (start transmit)      .
		break;
	case 0x49: // Reserved                                        .
		break;
	case 0x53: // Set Device Control Block (DCB) Parameters       .
		break;
	case 0x54: // Set Enhanced Mode Parameters                    .
		break;
	case 0x61: // Query Current Bit Rate                          .
		break;
	case 0x62: // Query Line Characteristics                      .
		break;
	case 0x63: // Extended Query Bit Rate                         .
		break;
	case 0x64: // Query COM Status                                .
		break;
	case 0x65: // Query Transmit Data Status                      .
		break;
	case 0x66: // Query Modem Control Output Signals              .
		break;
	case 0x67: // Query Current Modem Input Signals               .
		break;
	case 0x68: // Query Number of Characters in Receive Queue     .
		break;
	case 0x69: // Query Number of Characters in Transmit Queue    .
		break;
	case 0x72: // Query COM Event Information                     .
		break;
	case 0x73: // Query Device Control Block (DCB) Parameters     .
		break;
	case 0x74: // Query Enhanced Mode Parameters                  .
		break;
	default:
		return ERROR_INVALID_FUNCTION;
		break;
	}
	return ERROR_INVALID_FUNCTION;
}






/* EOF */
