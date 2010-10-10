/*
 * TRAFFIC definitions
 *
 * Copyright (c) 2009 Stefan Leichter
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

#ifndef __WINE_TRAFFIC_H
#define __WINE_TRAFFIC_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

typedef VOID (CALLBACK * TCI_ADD_FLOW_COMPLETE_HANDLER)(HANDLE, ULONG);
typedef VOID (CALLBACK * TCI_DEL_FLOW_COMPLETE_HANDLER)(HANDLE, ULONG);
typedef VOID (CALLBACK * TCI_MOD_FLOW_COMPLETE_HANDLER)(HANDLE, ULONG);
typedef VOID (CALLBACK * TCI_NOTIFY_HANDLER)
                              (HANDLE,HANDLE,ULONG,HANDLE,ULONG,PVOID);

typedef struct _TCI_CLIENT_FUNC_LIST
{
    TCI_NOTIFY_HANDLER ClNotifyHandler;
    TCI_ADD_FLOW_COMPLETE_HANDLER ClAddFlowCompleteHandler;
    TCI_MOD_FLOW_COMPLETE_HANDLER ClModifyFlowCompleteHandler;
    TCI_DEL_FLOW_COMPLETE_HANDLER ClDeleteFlowCompleteHandler;
} TCI_CLIENT_FUNC_LIST, *PTCI_CLIENT_FUNC_LIST;

ULONG WINAPI TcRegisterClient(ULONG,HANDLE,PTCI_CLIENT_FUNC_LIST,PHANDLE);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_TRAFFIC_H */
