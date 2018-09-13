/*--------------------------------------------------------------
 *
 * FILE:			SK_DLL.H
 *
 * PURPOSE:			The file contains the Functions responsible for
 *					managing information passed between SerialKeys
 *					and the SerialKeys DLL
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

#define	SPI_GETSERIALKEYS	62
#define	SPI_SETSERIALKEYS	63

// Global Variables ---------------------------------

// Global Function Prototypes ---------------------------------

BOOL DoneDLL();
BOOL InitDLL();
void ResumeDLL();
void SuspendDLL();
void TerminateDLL();

