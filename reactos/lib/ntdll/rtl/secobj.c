/*
 *  ReactOS kernel
 *  Copyright (C) 2005 Eric Kohl
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
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Security object implementation
 * FILE:              lib/ntdll/rtl/secobj.c
 */

/* INCLUDES ****************************************************************/

#include <ntdll.h>
#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
NTSTATUS
STDCALL
RtlDeleteSecurityObject(IN PSECURITY_DESCRIPTOR *ObjectDescriptor)
{
    DPRINT("RtlDeleteSecurityObject(%p)\n", ObjectDescriptor);

    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                *ObjectDescriptor);

    return STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlNewSecurityObject(IN PSECURITY_DESCRIPTOR ParentDescriptor,
                     IN PSECURITY_DESCRIPTOR CreatorDescriptor,
                     OUT PSECURITY_DESCRIPTOR *NewDescriptor,
                     IN BOOLEAN IsDirectoryObject,
                     IN HANDLE Token,
                     IN PGENERIC_MAPPING GenericMapping)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlQuerySecurityObject(IN PSECURITY_DESCRIPTOR ObjectDescriptor,
                       IN SECURITY_INFORMATION SecurityInformation,
                       OUT PSECURITY_DESCRIPTOR ResultantDescriptor,
                       IN ULONG DescriptorLength,
                       OUT PULONG ReturnLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlSetSecurityObject(IN SECURITY_INFORMATION SecurityInformation,
                     IN PSECURITY_DESCRIPTOR ModificationDescriptor,
                     OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
                     IN PGENERIC_MAPPING GenericMapping,
                     IN HANDLE Token)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
