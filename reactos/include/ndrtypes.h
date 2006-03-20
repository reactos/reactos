/*
 * NDR Types
 *
 * Copyright 2006 Robert Shearman (for CodeWeavers)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __NDRTYPES_H__
#define __NDRTYPES_H__

typedef struct
{
    unsigned short MustSize : 1; /* 0x0001 - client interpreter MUST size this
     *  parameter, other parameters may be skipped, using the value in
     *  NDR_PROC_PARTIAL_OIF_HEADER::constant_client_buffer_size instead. */
    unsigned short MustFree : 1; /* 0x0002 - server interpreter MUST size this
     *  parameter, other parameters may be skipped, using the value in
     *  NDR_PROC_PARTIAL_OIF_HEADER::constant_server_buffer_size instead. */
    unsigned short IsPipe : 1; /* 0x0004 - The parameter is a pipe handle. See
     *  http://msdn.microsoft.com/library/default.asp?url=/library/en-us/rpc/rpc/pipes.asp
     *  for more information on pipes. */
    unsigned short IsIn : 1; /* 0x0008 - The parameter is an input */
    unsigned short IsOut : 1; /* 0x0010 - The parameter is an output */
    unsigned short IsReturn : 1; /* 0x0020 - The parameter is to be returned */
    unsigned short IsBasetype : 1; /* 0x0040 - The parameter is simple and has the
     *  format defined by NDR_PARAM_OIF_BASETYPE rather than by
     *  NDR_PARAM_OIF_OTHER. */
    unsigned short IsByValue : 1; /* 0x0080 - Set for compound types being sent by
     *  value. Can be of type: structure, union, transmit_as, represent_as,
     *  wire_marshal and SAFEARRAY. */
    unsigned short IsSimpleRef : 1; /* 0x0100 - parameter that is a reference
     *  pointer to anything other than another pointer, and which has no
     *  allocate attributes. */
    unsigned short IsDontCallFreeInst : 1; /*  0x0200 - Used for some represent_as types
     *  for when the free instance routine should not be called. */
    unsigned short SaveForAsyncFinish : 1; /* 0x0400 - Unknown */
    unsigned short Unused : 2;
    unsigned short ServerAllocSize : 3; /* 0xe000 - If non-zero
     *  specifies the size of the object in numbers of 8byte blocks needed.
     *  It will be stored on the server's stack rather than using an allocate
     *  call. */
} PARAM_ATTRIBUTES;

#endif
