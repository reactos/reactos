/*
 * ReactOS FAX API Support
 * Copyright (C) 2004 ReactOS Team
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
/* $Id$
 *
 * PROJECT:         ReactOS FAX API Support
 * FILE:            lib/winfax/winfax.c
 * PURPOSE:         ReactOS FAX API Support
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *
 * UPDATE HISTORY:
 *      10/02/2004  Created
 */
#include "precomp.h"

/*
 * @unimplemented
 */
BOOL WINAPI
FaxAbort(HANDLE FaxHandle, DWORD JobId)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxAccessCheck(HANDLE FaxHandle, DWORD AccessMask)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxClose(HANDLE FaxHandle)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxCompleteJobParamsA(PFAX_JOB_PARAMA *JobParams, PFAX_COVERAGE_INFOA *CoverageInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxCompleteJobParamsW(PFAX_JOB_PARAMW *JobParams, PFAX_COVERAGE_INFOW *CoverageInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxConnectFaxServerA(LPCSTR MachineName, LPHANDLE FaxHandle)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxConnectFaxServerW(LPCWSTR MachineName, LPHANDLE FaxHandle)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxEnableRoutingMethodA(HANDLE FaxPortHandle, LPCSTR RoutingGuid, BOOL Enabled)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxEnableRoutingMethodW(HANDLE FaxPortHandle, LPCWSTR RoutingGuid, BOOL Enabled)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxEnumGlobalRoutingInfoA(HANDLE FaxHandle, PFAX_GLOBAL_ROUTING_INFOA *RoutingInfo, LPDWORD MethodsReturned)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxEnumGlobalRoutingInfoW(HANDLE FaxHandle, PFAX_GLOBAL_ROUTING_INFOW *RoutingInfo, LPDWORD MethodsReturned)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxEnumJobsA(HANDLE FaxHandle, PFAX_JOB_ENTRYA *JobEntry, LPDWORD JobsReturned)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxEnumJobsW(HANDLE FaxHandle, PFAX_JOB_ENTRYW *JobEntry, LPDWORD JobsReturned)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxEnumPortsA(HANDLE FaxHandle, PFAX_PORT_INFOA *PortInfo, LPDWORD PortsReturned)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxEnumPortsW(HANDLE FaxHandle, PFAX_PORT_INFOW *PortInfo, LPDWORD PortsReturned)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxEnumRoutingMethodsA(HANDLE FaxPortHandle, PFAX_ROUTING_METHODA *RoutingMethod, LPDWORD MethodsReturned)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxEnumRoutingMethodsW(HANDLE FaxPortHandle, PFAX_ROUTING_METHODW *RoutingMethod, LPDWORD MethodsReturned)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
VOID WINAPI
FaxFreeBuffer(LPVOID Buffer)
{
  UNIMPLEMENTED;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxGetConfigurationA(HANDLE FaxHandle, PFAX_CONFIGURATIONA *FaxConfig)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxGetConfigurationW(HANDLE FaxHandle, PFAX_CONFIGURATIONW *FaxConfig)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxGetDeviceStatusA(HANDLE FaxPortHandle, PFAX_DEVICE_STATUSA *DeviceStatus)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxGetDeviceStatusW(HANDLE FaxPortHandle, PFAX_DEVICE_STATUSW *DeviceStatus)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxGetJobA(HANDLE FaxHandle, DWORD JobId, PFAX_JOB_ENTRYA *JobEntry)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxGetJobW(HANDLE FaxHandle, DWORD JobId, PFAX_JOB_ENTRYW *JobEntry)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxGetLoggingCategoriesA(HANDLE FaxHandle, PFAX_LOG_CATEGORYA *Categories, LPDWORD NumberCategories)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxGetLoggingCategoriesW(HANDLE FaxHandle, PFAX_LOG_CATEGORYW *Categories, LPDWORD NumberCategories)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxGetPageData(HANDLE FaxHandle, DWORD JobId, LPBYTE *Buffer, LPDWORD BufferSize, LPDWORD ImageWidth, LPDWORD ImageHeight)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxGetPortA(HANDLE FaxPortHandle, PFAX_PORT_INFOA *PortInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxGetPortW(HANDLE FaxPortHandle, PFAX_PORT_INFOW *PortInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxGetRoutingInfoA(HANDLE FaxPortHandle, LPCSTR RoutingGuid, LPBYTE *RoutingInfoBuffer, LPDWORD RoutingInfoBufferSize)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxGetRoutingInfoW(HANDLE FaxPortHandle, LPCWSTR RoutingGuid, LPBYTE *RoutingInfoBuffer, LPDWORD RoutingInfoBufferSize)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxInitializeEventQueue(HANDLE FaxHandle, HANDLE CompletionPort, ULONG_PTR CompletionKey, HWND hWnd, UINT MessageStart)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxOpenPort(HANDLE FaxHandle, DWORD DeviceId, DWORD Flags, LPHANDLE FaxPortHandle)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxPrintCoverPageA(CONST FAX_CONTEXT_INFOA *FaxContextInfo, CONST FAX_COVERAGE_INFOA *CoverPageInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxPrintCoverPageW(CONST FAX_CONTEXT_INFOW *FaxContextInfo, CONST FAX_COVERAGE_INFOW *CoverPageInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxRegisterRoutingExtensionW(HANDLE FaxHandle, LPCWSTR ExtensionName, LPCWSTR FriendlyName, LPCWSTR ImageName, PFAX_ROUTING_INSTALLATION_CALLBACKW CallBack, LPVOID Context)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxRegisterServiceProviderW(LPCWSTR DeviceProvider, LPCWSTR FriendlyName, LPCWSTR ImageName, LPCWSTR TspName)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxSendDocumentA(HANDLE FaxHandle, LPCSTR FileName, PFAX_JOB_PARAMA JobParams, CONST FAX_COVERAGE_INFOA *CoverpageInfo, LPDWORD FaxJobId)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxSendDocumentForBroadcastA(HANDLE FaxHandle, LPCSTR FileName, LPDWORD FaxJobId, PFAX_RECIPIENT_CALLBACKA FaxRecipientCallback, LPVOID Context)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxSendDocumentForBroadcastW(HANDLE FaxHandle, LPCWSTR FileName, LPDWORD FaxJobId, PFAX_RECIPIENT_CALLBACKW FaxRecipientCallback, LPVOID Context)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxSendDocumentW(HANDLE FaxHandle, LPCWSTR FileName, PFAX_JOB_PARAMW JobParams, CONST FAX_COVERAGE_INFOW *CoverpageInfo, LPDWORD FaxJobId)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxSetConfigurationA(HANDLE FaxHandle, CONST FAX_CONFIGURATIONA *FaxConfig)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxSetConfigurationW(HANDLE FaxHandle, CONST FAX_CONFIGURATIONW *FaxConfig)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxSetGlobalRoutingInfoA(HANDLE FaxHandle, CONST FAX_GLOBAL_ROUTING_INFOA *RoutingInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxSetGlobalRoutingInfoW(HANDLE FaxHandle, CONST FAX_GLOBAL_ROUTING_INFOW *RoutingInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxSetJobA(HANDLE FaxHandle, DWORD JobId, DWORD Command, CONST FAX_JOB_ENTRYA *JobEntry)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxSetJobW(HANDLE FaxHandle, DWORD JobId, DWORD Command, CONST FAX_JOB_ENTRYW *JobEntry)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxSetLoggingCategoriesA(HANDLE FaxHandle, CONST FAX_LOG_CATEGORYA *Categories, DWORD NumberCategories)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxSetLoggingCategoriesW(HANDLE FaxHandle, CONST FAX_LOG_CATEGORYW *Categories, DWORD NumberCategories)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxSetPortA(HANDLE FaxPortHandle, CONST FAX_PORT_INFOA *PortInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxSetPortW(HANDLE FaxPortHandle, CONST FAX_PORT_INFOW *PortInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxSetRoutingInfoA(HANDLE FaxPortHandle, LPCSTR RoutingGuid, CONST BYTE *RoutingInfoBuffer, DWORD RoutingInfoBufferSize)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxSetRoutingInfoW(HANDLE FaxPortHandle, LPCWSTR RoutingGuid, CONST BYTE *RoutingInfoBuffer, DWORD RoutingInfoBufferSize)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxStartPrintJobA(LPCSTR PrinterName, CONST FAX_PRINT_INFOA *PrintInfo, LPDWORD FaxJobId, PFAX_CONTEXT_INFOA FaxContextInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
FaxStartPrintJobW(LPCWSTR PrinterName, CONST FAX_PRINT_INFOW *PrintInfo, LPDWORD FaxJobId, PFAX_CONTEXT_INFOW FaxContextInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}

