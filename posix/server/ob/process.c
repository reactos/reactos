/* $Id: process.c,v 1.1 2002/04/10 21:30:22 ea Exp $
 * 
 * PROJECT    : ReactOS / POSIX+ Environment Subsystem Server
 * FILE       : reactos/subsys/psx/server/ob/session.c
 * DESCRIPTION: terminal
 * DATE       : 2002-04-04
 * AUTHOR     : Emanuele Aliberti <eal@users.sf.net>
 *
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 */
#include <psxss.h>

/**********************************************************************
 *	PsxInitializeProcesses/0
 */
NTSTATUS STDCALL
PsxInitializeProcesses (VOID)
{
    debug_print (L"PSXSS: ->"__FUNCTION__);
    /* TODO */
    debug_print (L"PSXSS: <-"__FUNCTION__);
    return STATUS_SUCCESS;
}
/**********************************************************************
 *	PsxCreateProcess/3
 */
NTSTATUS STDCALL
PsxCreateProcess (
    PLPC_MAX_MESSAGE pRequest,
    HANDLE           hConnectedPort,
    ULONG            ulPortIdentifier
    )
{
    debug_print (L"PSXSS: ->"__FUNCTION__);
    /* TODO */
    debug_print (L"PSXSS: <-"__FUNCTION__);
    return STATUS_SUCCESS;
}
/* EOF */
