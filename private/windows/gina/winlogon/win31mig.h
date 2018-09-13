/****************************** Module Header ******************************\
* Module Name: win31mig.h
*
* Copyright (c) 1993, Microsoft Corporation
*
* Constants for the Windows 3.1 Migration dialog
*
* History:
* 01-08-93 Stevewo      Created.
* 02-16-99 jimschm      Added IsWin9xUpgrade
\***************************************************************************/

#ifndef RC_INVOKED
BOOL
Windows31Migration(
    PTERMINAL pTerm
    );
#endif  /* !RC_INVOKED */

#define IDD_WIN31MIG                801
#define IDD_WIN31MIG_INIFILES       802
#define IDD_WIN31MIG_GROUPS         803
#define IDD_WIN31MIG_STATUS         804

BOOL
IsWin9xUpgrade (
    VOID
    );
