/*--------------------------------------------------------------
 *
 * FILE:			DEBUG.H
 *
 * PURPOSE:			Debug Routines using a named pipe to output debug
 *					data.
 *
 * CREATION:		June 1993
 *
 * COPYRIGHT:		Black Diamond Software (C) 1994
 *
 * AUTHOR:			Ronald Moak 
 *
 * NOTES:		
 *					
 * This file, and all others associated with it contains trade secrets
 * and information that is proprietary to Black Diamond Software.
 * It may not be copied copied or distributed to any person or firm 
 * without the express written permission of Black Diamond Software. 
 * This permission is available only in the form of a Software Source 
 * License Agreement.
 *
 * $Header: %Z% %F% %H% %T% %I%
 *
 *------------------------------------------------------------*/

//----------- Function Prototypes   ----------------------------------------

void dbgOut( LPSTR msg );
void dbgErr( LPSTR msg );
void dbgOpen();
void dbgClose();

//----------- Defines   ----------------------------------------

//#define DEBUG				// Define to Enable Debug

#ifdef 	DEBUG

#define DBG_OPEN()			dbgOpen()
#define DBG_CLOSE()			dbgClose()
#define DBG_OUT(msg)		dbgOut(msg)
#define DBG_ERR(msg)		dbgErr(msg)
#define DBG_ERR1(stat, msg)	{if (!stat) dbgErr(msg);}

#else			// DEBUG

#define DBG_OPEN() 
#define DBG_CLOSE()
#define DBG_OUT(msg)	
#define DBG_ERR(msg)	
#define DBG_ERR1(stat, msg)

#endif			// DEBUG
