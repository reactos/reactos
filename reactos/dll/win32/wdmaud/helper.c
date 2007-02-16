/*
 *
 * COPYRIGHT:           See COPYING in the top level directory
 * PROJECT:             ReactOS Multimedia
 * FILE:                lib/wdmaud/helper.c
 * PURPOSE:             Multimedia User Mode Driver - Helper Funcs
 * PROGRAMMER:          Andrew Greenwood
 * UPDATE HISTORY:
 *                      Nov 13, 2005: Created
 */

#include <windows.h>
#include <mmsystem.h>

/*
	TranslateWinError converts Win32 error codes (returned by
	GetLastError, typically) into MMSYSERR codes.
*/

MMRESULT TranslateWinError(DWORD error)
{
	switch(error)
	{
		case NO_ERROR :
		case ERROR_IO_PENDING :
			return MMSYSERR_NOERROR;

		case ERROR_BUSY :
			return MMSYSERR_ALLOCATED;

		case ERROR_NOT_SUPPORTED :
		case ERROR_INVALID_FUNCTION :
			return MMSYSERR_NOTSUPPORTED;

		case ERROR_NOT_ENOUGH_MEMORY :
			return MMSYSERR_NOMEM;

		case ERROR_ACCESS_DENIED :
			return MMSYSERR_BADDEVICEID;

		case ERROR_INSUFFICIENT_BUFFER :
			return MMSYSERR_INVALPARAM;
	}

	return MMSYSERR_ERROR;
}
