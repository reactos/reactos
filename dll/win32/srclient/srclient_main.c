/*
 * Copyright 2009 Christoph von Wittich
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WIN32_NO_STATUS

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <srrestoreptapi.h>

#define NDEBUG
#include <debug.h>

BOOL
WINAPI
SRSetRestorePointA(PRESTOREPOINTINFOA pRestorePtSpec, PSTATEMGRSTATUS pSMgrStatus)
{
    DPRINT1("SRSetRestorePointA is unimplemented\n");

    if (!pSMgrStatus)
        return FALSE;

    pSMgrStatus->nStatus = ERROR_SUCCESS;
    pSMgrStatus->llSequenceNumber = pRestorePtSpec->llSequenceNumber + 1;

    return TRUE;
}


BOOL
WINAPI
SRSetRestorePointW(PRESTOREPOINTINFOW pRestorePtSpec, PSTATEMGRSTATUS pSMgrStatus)
{
    DPRINT1("SRSetRestorePointW is unimplemented\n");

    if (!pSMgrStatus)
        return FALSE;

    pSMgrStatus->nStatus = ERROR_SUCCESS;
    pSMgrStatus->llSequenceNumber = pRestorePtSpec->llSequenceNumber + 1;

    return TRUE;
}


DWORD
WINAPI
SRRemoveRestorePoint(DWORD dwNumber)
{
    DPRINT1("SRRemoveRestorePoint is unimplemented\n");
    return ERROR_SUCCESS;
}

