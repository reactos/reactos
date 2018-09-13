/*
**------------------------------------------------------------------------------
** Module:  Disk Cleanup Manager
** File:    msprintf.h
**
** Purpose: various common stuff for this module
** Notes:   
** Mod Log: Created by Rich Jernigan (??/??)
**          Modified by Shawn Brown (2/95)
**			Modified by Jason Cobb (2/97)
**
** Copyright (c)1995 Microsoft Corporation, All Rights Reserved
**------------------------------------------------------------------------------
*/
#ifndef MSPRINTF_H
#define MSPRINTF_H

/*
**------------------------------------------------------------------------------
** Project include files
**------------------------------------------------------------------------------
*/
#ifndef COMMON_H
   #include "common.h"
#endif

/*
**------------------------------------------------------------------------------
** Global Prototypes
**------------------------------------------------------------------------------
*/

TCHAR * cdecl SHFormatMessage( DWORD dwMessageId, ...);

#endif  // MSPRINTF_H
/*
**------------------------------------------------------------------------------
** End of File
**------------------------------------------------------------------------------
*/

