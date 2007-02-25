/*
 * Copyright 2005 Eric Kohl
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

#ifndef __RPC_PRIVATE_H
#define __RPC_PRIVATE_H

RPC_STATUS PnpBindRpc(LPCWSTR pszMachine,
                      RPC_BINDING_HANDLE* BindingHandle);
RPC_STATUS PnpUnbindRpc(RPC_BINDING_HANDLE *BindingHandle);

BOOL
PnpGetLocalHandles(RPC_BINDING_HANDLE *BindingHandle,
                   HSTRING_TABLE *StringTable);
RPC_STATUS PnpUnbindLocalHandles(VOID);

#endif /* __RPC_PRIVATE_H */
