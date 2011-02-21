/*
 * Copyright 2006 Mike McCormack
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

#ifndef __OLE_ENUM_H__
#define __OLE_ENUM_H__

typedef struct tagEnumSTATPROPSETSTG_impl enumx_impl;

extern HRESULT WINAPI enumx_QueryInterface(enumx_impl *, REFIID, void**);
extern ULONG WINAPI enumx_AddRef(enumx_impl *);
extern ULONG WINAPI enumx_Release(enumx_impl *);
extern HRESULT WINAPI enumx_Next(enumx_impl *, ULONG, void *, ULONG *);
extern HRESULT WINAPI enumx_Skip(enumx_impl *, ULONG);
extern HRESULT WINAPI enumx_Reset(enumx_impl *);
extern HRESULT WINAPI enumx_Clone(enumx_impl *, enumx_impl **);
extern enumx_impl *enumx_allocate(REFIID, const void *, ULONG);
extern void *enumx_add_element(enumx_impl *, const void *);

#endif
