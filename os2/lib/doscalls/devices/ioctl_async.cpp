/* $Id: ioctl_async.cpp,v 1.1 2002/09/04 22:19:47 robertk Exp $
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
*/
void EvaluateAsyncIoCtl( HFILE hDevice, ULONG function,
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
		break;
/**
pppp
|----------+--------------------------------------------------|
|   43h    |Extended Set Bit Rate                             |
|----------+--------------------------------------------------|
|   44h    |Transmit Byte Immediate                           |
|----------+--------------------------------------------------|
|   45h    |Set Break OFF                                     |
|----------+--------------------------------------------------|
|   46h    |Set Modem Control Signals                         |
|----------+--------------------------------------------------|
|   47h    |Behave as if XOFF Received (stop transmit)        |
|----------+--------------------------------------------------|
|   48h    |Behave as if XON Received (start transmit)        |
|----------+--------------------------------------------------|
|   49h    |Reserved                                          |
|----------+--------------------------------------------------|
|   4Bh    |Set Break ON                                      |
|----------+--------------------------------------------------|
|   53h    |Set Device Control Block (DCB) Parameters         |
|----------+--------------------------------------------------|
|   54h    |Set Enhanced Mode Parameters                      |
|----------+--------------------------------------------------|
|   61h    |Query Current Bit Rate                            |
|----------+--------------------------------------------------|
|   62h    |Query Line Characteristics                        |
|----------+--------------------------------------------------|
|   63h    |Extended Query Bit Rate                           |
|----------+--------------------------------------------------|
|   64h    |Query COM Status                                  |
|----------+--------------------------------------------------|
|   65h    |Query Transmit Data Status                        |
|----------+--------------------------------------------------|
|   66h    |Query Modem Control Output Signals                |
|----------+--------------------------------------------------|
|   67h    |Query Current Modem Input Signals                 |
|----------+--------------------------------------------------|
|   68h    |Query Number of Characters in Receive Queue       |
|----------+--------------------------------------------------|
|   69h    |Query Number of Characters in Transmit Queue      |
|----------+--------------------------------------------------|
|   6Dh    |Query COM Error                                   |
|----------+--------------------------------------------------|
|   72h    |Query COM Event Information                       |
|----------+--------------------------------------------------|
|   73h    |Query Device Control Block (DCB) Parameters       |
|----------+--------------------------------------------------|
|   74h    |Query Enhanced Mode Parameters                    |
+-------------------------------------------------------------+
pppp */


	}
}






/* EOF */