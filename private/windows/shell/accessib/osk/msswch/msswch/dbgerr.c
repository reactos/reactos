/**************************************************************************\
 DbgErr.c
 Copyright (C) 1990-97 Bloorview MacMillan Centre
 Toronto, Ontario  M4G 1R8

 Interprets a Windows API error using FormatMessage and GetLastError.
 Then uses dbg to send the result to the TRACEWIN application.
\**************************************************************************/

#include <windows.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "dbg.h"
#include "dbgerr.h"


#ifdef __cplusplus
extern "C" 
{
#endif  // __cplusplus
//#pragma data_seg("sh_data")		/***** start of shared data segment *****/

//#pragma data_seg()	 /***** end of shared data segment *****/


////////////
// print the error information
void dbgerr( LPTSTR szInfo, BOOL bError )
   {
	LPVOID lpMsgBuf;

	if (bError)
		{
 		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
			);

		// Display the info and error strings.
		DBGMSG( TEXT("%s %s\n"), szInfo, lpMsgBuf );

		// Free the buffer.
		LocalFree( lpMsgBuf );
		}

   }



#ifdef __cplusplus
}
#endif  // __cplusplus


