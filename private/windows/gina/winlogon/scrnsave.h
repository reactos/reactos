/****************************** Module Header ******************************\
* Module Name: scrnsave.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Secure Attention Key Sequence
*
* History:
* 01-23-91 Davidc       Created.
\***************************************************************************/

//
// Exported function prototypes
//

BOOL
ScreenSaverEnabled(
    PTERMINAL pTerm
    );

int
RunScreenSaver(
    PTERMINAL pTerm,
    BOOL WindowStationLocked,
    BOOL AllowFastUnlock
    );

