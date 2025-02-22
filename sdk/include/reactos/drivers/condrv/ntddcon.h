/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Driver
 * FILE:            include/reactos/drivers/condrv/ntddcon.h
 * PURPOSE:         Console Driver IOCTL Interface
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#if (_MSC_VER >= 1020)
#pragma once
#endif

#ifndef _NTDDCON_H_
#define _NTDDCON_H_

#ifdef __cplusplus
extern "C" {
#endif

// FIXME: Redo all the drawing since this one is quite old...

/************************************************************************************************
 *                          Architecture of the Generic Terminal Driver
 ************************************************************************************************

IN  = CONIN$  = stdin
OUT = CONOUT$ = stdout
ERR           = stderr

                                    +------------------------------+
        +-------------+             | +-----------------+          |
        |             |----- ERR ---->|                 |------+   |
        | Console App |----- OUT ---->| Virtual Console |----+ |   |
        |      1      |<---- IN  -----|       1-1       |--+ | |   |
        +-------------+             | +-----------------+  | | |   |
                                    |                      | | \   |
        +-------------+             | +-----------------+  | |  >--X----- ERR ---->
        |             |----- ERR ---->|                 |--|-|-/   |
        | Console App |----- OUT ---->| Virtual Console |--|-+-----X----- OUT ---->
        |      2      |<---- IN  -----|       1-2       |--+---\   |
        +-------------+       ^     | +-----------------+       \--X<---- IN  -----
                              |     |                              |       ^
                       --+    |     |        . . .                 |       |
\Console +-- \CurrentIn  |    |     |                              |       |
         +-- \CurrentOut +----+     |                              |       |
             \CurrentErr |          |          Terminal 1          |       |
                       --+          |                              |       |
                                    +------------------------------+       |
                                                                           |
                                                                      +----+----+
                            . . .                                     | \Input  |
                                                                        \Output
                                                                        \Error

*/

/*
 * Remarks on the symbolic links :
 *
 * - \DosDevices\ is an alias to \??\
 *
 * - Using "\DosDevices\Global\<name>" allows the driver to ALWAYS
 *   create the symbolic link in the global object namespace. Indeed,
 *   under Windows NT-2000, the \DosDevices\ directory was always
 *   global, but starting with Windows XP, it became local to a session.
 *   One would then use \GLOBAL??\ to access to the global directory.
 *   However, this name doesn't exist under Windows NT-2000.
 *   Therefore, we use the trick to use the 'Global' symbolic link
 *   defined both under Windows NT-2000 and Windows XP and later,
 *   which exists in \DosDevices\, to access to \??\ (global) under
 *   Windows NT-2000, and to \GLOBAL??\ under Windows XP and later.
 */

//
// Controller device
//
#define DD_CONDRV_CTRL_DEVICE_NAME    "\\Device\\ConDrv"
#define DD_CONDRV_CTRL_DEVICE_NAME_U L"\\Device\\ConDrv"
#define DD_CONDRV_CTRL_SYMLNK_NAME    "\\DosDevices\\Global\\ConDrv"
#define DD_CONDRV_CTRL_SYMLNK_NAME_U L"\\DosDevices\\Global\\ConDrv"


//
// Console
//
#define DD_CONDRV_CONSOLE_DEVICE_NAME    "\\Device\\Console"
#define DD_CONDRV_CONSOLE_DEVICE_NAME_U L"\\Device\\Console"
#define DD_CONDRV_CONSOLE_SYMLNK_NAME    "\\DosDevices\\Global\\Console"
#define DD_CONDRV_CONSOLE_SYMLNK_NAME_U L"\\DosDevices\\Global\\Console"


#ifdef TELETYPE
//
// Virtual files associated with a given console
//
#define CONDRV_CONSOLE_FILE_CURRIN     "\\CurrentIn"
#define CONDRV_CONSOLE_FILE_CURRIN_U  L"\\CurrentIn"

#define CONDRV_CONSOLE_FILE_CURROUT    "\\CurrentOut"
#define CONDRV_CONSOLE_FILE_CURROUT_U L"\\CurrentOut"

//#define CONDRV_CONSOLE_FILE_CURRERR    "\\CurrentErr"
//#define CONDRV_CONSOLE_FILE_CURRERR_U L"\\CurrentErr"

#define CONDRV_VC_FILE_SCRBUF      "\\ScreenBuffer"
#define CONDRV_VC_FILE_SCRBUF_U   L"\\ScreenBuffer"

/*** Original names from Windows-8 condrv.sys ***

L"\Connect"
L"\Reference"
L"\Server"
L"\Broker"

L"\Console"
L"\Display"

L"\Input"
L"\Output"
L"\CurrentIn"
L"\CurrentOut"
L"\ScreenBuffer"

***/

#endif


//
// IO codes
//
#ifndef CTL_CODE
    #error "CTL_CODE undefined. Include winioctl.h or wdm.h"
#endif

#define IOCTL_CONDRV_CREATE_CONSOLE  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ALL_ACCESS)
#define IOCTL_CONDRV_DELETE_CONSOLE  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#ifdef __cplusplus
}
#endif

#endif // _NTDDCON_H_

/* EOF */
