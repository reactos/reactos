/* $Id: error.cpp,v 1.1 2002/07/26 00:23:13 robertk Exp $
*/
/*
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS OS/2 sub system
 * FILE:             dll/process.cpp
 * PURPOSE:          Kernelservices for OS/2 apps
 * PROGRAMMER:       Robert K. nonvolatil@yahoo.de
 * REVISION HISTORY:
 *  13-03-2002  Created
 *	25-07-2002	Work to make it compile	
 */


#define INCL_DOSPROCESS
#define INCL_DOSERRORS
#include "ros2.h"
// we need the extra definitions of this file
namespace NT {
#include <ddk/ntddbeep.h>
}



APIRET STDCALL DosBeep(ULONG freq, ULONG dur)
{
    NT::BEEP_SET_PARAMETERS BeepSetParameters;
	NT::HANDLE	hBeep;
	NT::IO_STATUS_BLOCK ComplStatus;
	NT::UNICODE_STRING unistr; 
	NT::NTSTATUS stat;
	NT::OBJECT_ATTRIBUTES oa = {sizeof oa, 0, &unistr, NT::OBJ_CASE_INSENSITIVE, 0, 0};

	// init String still bevore use.
	NT::RtlInitUnicodeString( &unistr, L"\\\\.\\Beep" );
	
	if( freq<0x25 || freq>0x7FFF )
		return ERROR_INVALID_FREQUENCY; //395;	// 

    /* Set beep data */
    BeepSetParameters.Frequency = freq;
    BeepSetParameters.Duration  = dur;

	/* open the beep dirver */
	stat = NT::ZwOpenFile( &hBeep,
				FILE_READ_DATA | FILE_WRITE_DATA,
				&oa,
				&ComplStatus,
				0,	// no sharing
				FILE_OPEN );
	
	if (!NT_SUCCESS(stat))
	{
		return ERROR_NOT_READY;
	}
	
	/* actually beep */
    NT::ZwDeviceIoControlFile(hBeep, 0, // Event
					0, // APC-routine
					0, // UserAPCContext
					&ComplStatus,   IOCTL_BEEP_SET,
                    &BeepSetParameters,
                    sizeof(NT::BEEP_SET_PARAMETERS),
                    NULL,
                    0 );
    NT::ZwClose(hBeep);

    return NO_ERROR;
}


/* EOF */
