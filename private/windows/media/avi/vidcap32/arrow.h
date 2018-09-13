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
 *   arrow.h: Arrow control window 
 *
 *   Vidcap32 Source code
 *
 ***************************************************************************/

/*
 * interface definition for spin arrow class
 */

#define SPINARROW_CLASSNAME     "comarrow"

//reflect an arrow movement in the attached edit box
LONG FAR PASCAL ArrowEditChange( HWND hwndEdit, UINT wParam,
                                 LONG lMin, LONG lMax );


// call  me first (to register class)
BOOL FAR PASCAL ArrowInit(HANDLE hInst);



