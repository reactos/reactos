/*
 * Copyright (C) 1998-1999 François Gouget
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

#define WINOLEAPI STDAPI
#define WINOLEAPI_(type) STDAPI_(type)

#ifdef DBG
#undef DBG
#define DBG 1
#endif

#include_next <objbase.h>

#ifndef _OBJBASE_H_
#define _OBJBASE_H_

#define interface struct

#undef CONST_VTBL
#define CONST_VTBL
#define WINE_DECLARE_INTERFACE(iface) \
         /*typedef*/ interface iface { struct iface##Vtbl *lpVtbl; } /*iface*/; \
         typedef struct iface##Vtbl iface##Vtbl; \
         struct iface##Vtbl
#define WINE_DECLARE_INTERFACE_(iface,ibase) WINE_DECLARE_INTERFACE(iface)

#define ICOM_DEFINE(iface,ibase) WINE_DECLARE_INTERFACE_(iface,ibase) { iface##_METHODS };
#define ICOM_VTABLE(iface)       iface##Vtbl
#define ICOM_VFIELD(iface)       ICOM_VTABLE(iface)* lpVtbl
#define ICOM_THIS(impl,iface)    impl* const This=(impl*)(iface)
#define ICOM_THIS_MULTI(impl,field,iface)  impl* const This=(impl*)((char*)(iface) - offsetof(impl,field))

/*****************************************************************************
 *	Storage API
 */
#define STGM_NOSNAPSHOT		0x00200000

HRESULT WINAPI CoGetPSClsid(REFIID riid,CLSID *pclsid);
HRESULT WINAPI CLSIDFromProgID16(LPCOLESTR16 progid, LPCLSID riid);

#endif /* _OBJBASE_H_ */
