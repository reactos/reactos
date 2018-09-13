/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992 - 1995  Microsoft Corporation.  All Rights Reserved.
 *
 **************************************************************************/
/****************************************************************************
 *
 *   status.h: Status bar include file
 *
 *   Vidcap32 Source code
 *
 ***************************************************************************/


/* Globals */
// class name of window to create
extern TCHAR	szStatusClass[];


/* Function Prototypes */

BOOL  statusInit(HANDLE hInst, HANDLE hPrev);
void  statusUpdateStatus(HWND hwnd, LPCSTR lpsz);	// update status line

/*
 * returns the recommended height for a status bar based on the
 * character dimensions of the font used
 */
int statusGetHeight(void);



