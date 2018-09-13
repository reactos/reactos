/*--------------------------------------------------------------
 *
 * FILE:			SK_COMM.H
 *
 * PURPOSE:			Function prototypes for Serial Keys Comm Routines
 *
 * CREATION:		June 1994
 *
 * COPYRIGHT:		Black Diamond Software (C) 1994
 *
 * AUTHOR:			Ronald Moak 
 *
 * $Header: %Z% %F% %H% %T% %I%
 *
 *--------------------------------------------------------------*/

// Global Variables ---------------------------------


// Global Function ProtoTypes --------------------------------

BOOL	InitComm();
void	TerminateComm();

void 	SuspendComm();
void 	ResumeComm();

BOOL	StartComm();
void	StopComm();

void	SetCommBaud(int Baud);

