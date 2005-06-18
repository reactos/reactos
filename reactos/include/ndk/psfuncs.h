/* $Id: pstypes.h,v 1.1.2.1 2004/10/25 01:24:07 ion Exp $
 *
 *  ReactOS Headers
 *  Copyright (C) 1998-2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/psfuncs.h
 * PURPOSE:         Defintions for Process Manager Functions not documented in DDK/IFS.
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _PSFUNCS_H
#define _PSFUNCS_H

/* DEPENDENCIES **************************************************************/

/* FUNCTION TYPES ************************************************************/
typedef VOID (STDCALL *PPEBLOCKROUTINE)(PVOID);

typedef NTSTATUS 
(STDCALL *PW32_PROCESS_CALLBACK)(
    struct _EPROCESS *Process,
    BOOLEAN Create
);

typedef NTSTATUS
(STDCALL *PW32_THREAD_CALLBACK)(
    struct _ETHREAD *Thread,
    BOOLEAN Create
);


/* PROTOTYPES ****************************************************************/

VOID
STDCALL
PsRevertThreadToSelf(
    IN struct _ETHREAD* Thread
);

struct _W32THREAD*
STDCALL
PsGetWin32Thread(
    VOID
);

struct _W32PROCESS*
STDCALL
PsGetWin32Process(
    VOID
);

#endif
