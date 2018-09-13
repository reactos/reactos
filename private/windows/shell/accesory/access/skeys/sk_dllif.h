/*--------------------------------------------------------------
 *
 * FILE:			SK_DLLIF.H
 *
 * PURPOSE:			The file contains data structures for the 
 *					transmission of information between the 
 *					SerialKeys and the DLL.
 *
 * CREATION:		June 1994
 *
 * COPYRIGHT:		Black Diamond Software (C) 1994
 *
 * AUTHOR:			Ronald Moak 
 *
 * $Header: %Z% %F% %H% %T% %I%
 *
 *------------------------------------------------------------*/

typedef struct _SKEYDLL
{
	int		Message;					// Get or Set Changes
	DWORD	dwFlags;
	char	szActivePort[MAX_PATH];
	char	szPort[MAX_PATH];
	DWORD	iBaudRate;
	DWORD	iPortState;
	DWORD	iSave;						// TRUE - Write to Registry
} SKEYDLL;


// Messages

#define	SKEY_NAME			TEXT("\\\\.\\PIPE\\SKeys")



