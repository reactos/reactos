/*
 * KSUSER.DLL - ReactOS User CSA Library
 *
 * Copyright 2008 Dmitry Chapyshev
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

#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#include <tchar.h>
#include <ks.h>

KSDDKAPI NTSTATUS NTAPI
KsCreateAllocator(
    HANDLE                ConnectionHandle,
    PKSALLOCATOR_FRAMING  AllocatorFraming,
    PHANDLE              AllocatorHandle)
{
    return STATUS_SUCCESS;
}

KSDDKAPI NTSTATUS NTAPI
KsCreateClock(
    HANDLE           ConnectionHandle,
    PKSCLOCK_CREATE  ClockCreate,
    PHANDLE         ClockHandle)
{
    return STATUS_SUCCESS;
}

KSDDKAPI NTSTATUS NTAPI
KsCreatePin(
    HANDLE          FilterHandle,
    PKSPIN_CONNECT  Connect,
    ACCESS_MASK     DesiredAccess,
    PHANDLE        ConnectionHandle)
{
    return STATUS_SUCCESS;
}

KSDDKAPI NTSTATUS NTAPI
KsCreateTopologyNode(
    HANDLE          ParentHandle,
    PKSNODE_CREATE  NodeCreate,
    ACCESS_MASK     DesiredAccess,
    PHANDLE        NodeHandle)
{
    return STATUS_SUCCESS;
}
