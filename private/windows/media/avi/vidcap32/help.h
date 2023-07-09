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
 *   help.h: Help system include file
 *
 *   Vidcap32 Source code
 *
 ***************************************************************************/


// call DialogBoxParam, but ensuring correct help processing:
// assumes that each Dialog Box ID is a context number in the help file.
// calls MakeProcInstance as necessary. Uses instance data passed to
// HelpInit().
INT_PTR
DoDialog(
   HWND hwndParent,     // parent window
   int DialogID,        // dialog resource id
   DLGPROC fnDialog,    // dialog proc
   LPARAM lParam          // passed as lparam in WM_INITDIALOG
);


// set the help context id for a dialog displayed other than by DoDialog
// (eg by GetOpenFileName). Returns the old help context that you must
// restore by a further call to this function
int SetCurrentHelpContext(int DialogID);


// help init - initialise the support for the F1 key help
BOOL HelpInit(HINSTANCE hinstance, LPSTR helpfilepath, HWND hwndApp);


// shutdown the help system
void HelpShutdown(void);

// start help at the contents page
void HelpContents(void);


