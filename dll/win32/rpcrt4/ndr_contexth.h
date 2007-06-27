/*
 * Context Handle Functions
 *
 * Copyright 2006 Saveliy Tretiakov
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
 
 
#ifndef __WINE_NDR_CONTEXTH_H
#define __WINE_NDR_CONTEXTH_H

#include "wine/rpcss_shared.h"

typedef struct _ContextHandleNdr
{
  UINT attributes;
  UUID uuid;
} ContextHandleNdr;

typedef struct _CContextHandle
{
  RpcBinding *Binding;
  ContextHandleNdr Ndr;
} CContextHandle;

/*
   Keep this structure compatible with public rpcndr.h 
   declaration, otherwise NDRSContextValue macro won't work.
   typedef struct {
      void *pad[2];
      void *userContext;
   } *NDR_SCONTEXT;
*/

typedef struct _SContextHandle
{
  PVOID Prev;
  PVOID Next;
  PVOID Value;
  NDR_RUNDOWN Rundown;
  RpcConnection *Conn;
  ContextHandleNdr Ndr;
} SContextHandle;

void RPCRT4_DoContextRundownIfNeeded(RpcConnection *Conn);

#endif //__WINE_NDR_CONTEXTH_H
