/* $Id: error.cpp,v 1.3 2003/01/07 16:23:11 robd Exp $
*/
/*
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS OS/2 sub system
 * PART:			 doscalls.dll
 * FILE:             error.cpp
 * CONTAINS:		 Error related CP-functions.
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


/*******************************************
 DosBeep generates sound from the        
 speaker.                                

 freq (ULONG) - input 
    Cycles per second (Hertz) in the range of 0x25 to 
    0x7FFF. 
 
 dur (ULONG) - input 
    The length of the sound in milliseconds. 
 
 ulrc (APIRET) - returns 
    Return Code. 

    DosBeep returns one of the following values: 

      0         NO_ERROR 
      395       ERROR_INVALID_FREQUENCY 
*******************************************/
APIRET STDCALL DosBeep(ULONG freq, ULONG dur)
{
    NT::BEEP_SET_PARAMETERS BeepSetParameters;
	NT::HANDLE	hBeep;
	NT::IO_STATUS_BLOCK ComplStatus;
	NT::UNICODE_STRING unistr; 
	NT::NTSTATUS stat;
	NT::OBJECT_ATTRIBUTES oa = {sizeof oa, 0, &unistr, NT::OBJ_CASE_INSENSITIVE, 0, 0};

	// init String still bevore use.
	NT::RtlInitUnicodeString( &unistr, (NT::PWSTR)L"\\\\.\\Beep" );
	
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
	
	if ( stat<0 )
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


/******************************************
	DosError disables or enables error      
	notification to end users.              
 
 error (ULONG) - input 
    Error and Exception pop-up flags. 

    The unused high-order bits are reserved, and must be zero. The following values can be specified 
    for this parameter. They can be combined using the "logical or" ( | ) operator. 

      FERR_DISABLEHARDERR (0x00000000) 
          Disable hard error pop-ups. 

      FERR_ENABLEHARDERR (0x00000001) 
          Enable hard error pop-ups. 

      FERR_ENABLEEXCEPTION (0x00000000) 
          Enable program exception and untrapped numeric-processor exception pop-ups. 

      FERR_DISABLEEXCEPTION (0x00000002) 
          Disable program exception and untrapped numeric-processor exception pop-ups. 
 
 ulrc (APIRET) - returns 
    Return Code. 

    DosError returns one of the following values: 

      0         NO_ERROR 
      87        ERROR_INVALID_PARAMETER 
*******************************************/
APIRET DosError( ULONG error)
{
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/*******************************************
 DosMove moves a file object to another  
 location, and changes its name.         

 pszOld (PSZ) - input 
    Address of the old path name of the file or 
    subdirectory to be moved. 
 
 pszNew (PSZ) - input 
    Address of the new path name of the file or 
    subdirectory. 
 
 ulrc (APIRET) - returns 
    Return Code. 

    DosMove returns the one of following values: 

      0         NO_ERROR 
      2         ERROR_FILE_NOT_FOUND 
      3         ERROR_PATH_NOT_FOUND 
      5         ERROR_ACCESS_DENIED 
      17        ERROR_NOT_SAME_DEVICE 
      26        ERROR_NOT_DOS_DISK 
      32        ERROR_SHARING_VIOLATION 
      36        ERROR_SHARING_BUFFER_EXCEEDED 
      87        ERROR_INVALID_PARAMETER 
      108       ERROR_DRIVE_LOCKED 
      206       ERROR_FILENAME_EXCED_RANGE 
      250       ERROR_CIRCULARITY_REQUESTED 
      251       ERROR_DIRECTORY_IN_CDS 
*******************************************/
APIRET DosMove(PSZ pszOld, PSZ pszNew)
{
	return ERROR_CALL_NOT_IMPLEMENTED;
}

/* EOF */
