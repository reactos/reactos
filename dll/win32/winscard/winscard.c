/*
 * ReactOS SmartCard API
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
 * PROJECT:         ReactOS SmartCard API
 * FILE:            lib/winscard/winscard.c
 * PURPOSE:         ReactOS SmartCard API
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *
 * UPDATE HISTORY:
 *      11/07/2004  Created
 */
#include "precomp.h"

const SCARD_IO_REQUEST g_rgSCardT0Pci;
const SCARD_IO_REQUEST g_rgSCardT1Pci;
const SCARD_IO_REQUEST g_rgSCardRawPci;

/*
 * @unimplemented
 */
HANDLE
WINAPI
SCardAccessStartedEvent(VOID)
{
  UNIMPLEMENTED;
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return NULL;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardAddReaderToGroupA(SCARDCONTEXT hContext,
                       LPCSTR szReaderName,
                       LPCSTR szGroupName)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardAddReaderToGroupW(SCARDCONTEXT hContext,
                       LPCWSTR szReaderName,
                       LPCWSTR szGroupName)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardBeginTransaction(SCARDHANDLE hCard)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardCancel(SCARDCONTEXT hContext)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardConnectA(SCARDCONTEXT hContext,
              LPCSTR szReader,
              DWORD dwShareMode,
              DWORD dwPreferredProtocols,
              LPSCARDHANDLE phCard,
              LPDWORD pdwActiveProtocol)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardConnectW(SCARDCONTEXT hContext,
              LPCWSTR szReader,
              DWORD dwShareMode,
              DWORD dwPreferredProtocols,
              LPSCARDHANDLE phCard,
              LPDWORD pdwActiveProtocol)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardControl(SCARDHANDLE hCard,
             DWORD dwControlCode,
             LPCVOID lpInBuffer,
             DWORD nInBufferSize,
             LPVOID lpOutBuffer,
             DWORD nOutBufferSize,
             LPDWORD lpBytesReturned)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardDisconnect(SCARDHANDLE hCard,
                DWORD dwDisposition)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardEndTransaction(SCARDHANDLE hCard,
                    DWORD dwDisposition)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardEstablishContext(DWORD dwScope,
                      LPCVOID pvReserved1,
                      LPCVOID pvReserved2,
                      LPSCARDCONTEXT phContext)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardForgetCardTypeA(SCARDCONTEXT hContext,
                     LPCSTR szCardName)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardForgetCardTypeW(SCARDCONTEXT hContext,
                     LPCWSTR szCardName)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardForgetReaderA(SCARDCONTEXT hContext,
                   LPCSTR szReaderName)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardForgetReaderW(SCARDCONTEXT hContext,
                   LPCWSTR szReaderName)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardForgetReaderGroupA(SCARDCONTEXT hContext,
                        LPCSTR szGroupName)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardForgetReaderGroupW(SCARDCONTEXT hContext,
                        LPCWSTR szGroupName)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardFreeMemory(SCARDCONTEXT hContext,
                LPCVOID pvMem)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardGetAttrib(SCARDHANDLE hCard,
               DWORD dwAttrId,
               LPBYTE pbAttr,
               LPDWORD pcbAttrLen)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardGetCardTypeProviderNameA(SCARDCONTEXT hContext,
                              LPCSTR szCardName,
                              DWORD dwProviderId,
                              LPSTR szProvider,
                              LPDWORD pcchProvider)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardGetCardTypeProviderNameW(SCARDCONTEXT hContext,
                              LPCWSTR szCardName,
                              DWORD dwProviderId,
                              LPWSTR szProvider,
                              LPDWORD pcchProvider)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardGetProviderIdA(SCARDCONTEXT hContext,
                    LPCSTR szCard,
                    LPGUID pguidProviderId)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardGetProviderIdW(SCARDCONTEXT hContext,
                    LPCWSTR szCard,
                    LPGUID pguidProviderId)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardGetStatusChangeA(SCARDCONTEXT hContext,
                      DWORD dwTimeout,
                      LPSCARD_READERSTATEA rgReaderState,
                      DWORD cReaders)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardGetStatusChangeW(SCARDCONTEXT hContext,
                      DWORD dwTimeout,
                      LPSCARD_READERSTATEW rgReaderState,
                      DWORD cReaders)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardIntroduceCardTypeA(SCARDCONTEXT hContext,
                        LPCSTR szCardName,
                        LPCGUID pguidPrimaryProvider,
                        LPCGUID rgguidInterfaces,
                        DWORD dwInterfaceCount,
                        LPCBYTE pbAtr,
                        LPCBYTE pbAtrMask,
                        DWORD cbAtrLen)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardIntroduceCardTypeW(SCARDCONTEXT hContext,
                        LPCWSTR szCardName,
                        LPCGUID pguidPrimaryProvider,
                        LPCGUID rgguidInterfaces,
                        DWORD dwInterfaceCount,
                        LPCBYTE pbAtr,
                        LPCBYTE pbAtrMask,
                        DWORD cbAtrLen)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardIntroduceReaderA(SCARDCONTEXT hContext,
                      LPCSTR szReaderName,
                      LPCSTR szDeviceName)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardIntroduceReaderW(SCARDCONTEXT hContext,
                      LPCWSTR szReaderName,
                      LPCWSTR szDeviceName)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardIntroduceReaderGroupA(SCARDCONTEXT hContext,
                           LPCSTR szGroupName)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardIntroduceReaderGroupW(SCARDCONTEXT hContext,
                           LPCWSTR szGroupName)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardIsValidContext(SCARDCONTEXT hContext)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardListCardsA(SCARDCONTEXT hContext,
                LPCBYTE pbAtr,
                LPCGUID rgguidInterface,
                DWORD cguidInterfaceCount,
                LPCSTR mszCards,
                LPDWORD pcchCards)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardListCardsW(SCARDCONTEXT hContext,
                LPCBYTE pbAtr,
                LPCGUID rgguidInterface,
                DWORD cguidInterfaceCount,
                LPCWSTR mszCards,
                LPDWORD pcchCards)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardListInterfacesA(SCARDCONTEXT hContext,
                     LPCSTR szCard,
                     LPGUID pguidInterfaces,
                     LPDWORD pcguidInteraces)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardListInterfacesW(SCARDCONTEXT hContext,
                     LPCWSTR szCard,
                     LPGUID pguidInterfaces,
                     LPDWORD pcguidInteraces)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardListReaderGroupsA(SCARDCONTEXT hContext,
                       LPSTR mszGroups,
                       LPDWORD pcchGroups)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardListReaderGroupsW(SCARDCONTEXT hContext,
                       LPWSTR mszGroups,
                       LPDWORD pcchGroups)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardListReadersA(SCARDCONTEXT Context,
                  LPCSTR mszGroups,
                  LPSTR mszReaders,
                  LPDWORD pcchReaders)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardListReadersW(SCARDCONTEXT Context,
                  LPCWSTR mszGroups,
                  LPWSTR mszReaders,
                  LPDWORD pcchReaders)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardLocateCardsA(SCARDCONTEXT hContext,
                  LPCSTR mszCards,
                  LPSCARD_READERSTATEA rgReaderStates,
                  DWORD cReaders)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardLocateCardsW(SCARDCONTEXT hContext,
                  LPCWSTR mszCards,
                  LPSCARD_READERSTATEW rgReaderStates,
                  DWORD cReaders)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardLocateCardsByATRA(SCARDCONTEXT hContext,
                       LPSCARD_ATRMASK rgAtrMasks,
                       DWORD cAtrs,
                       LPSCARD_READERSTATEA rgReaderStates,
                       DWORD cReaders)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardLocateCardsByATRW(SCARDCONTEXT hContext,
                       LPSCARD_ATRMASK rgAtrMasks,
                       DWORD cAtrs,
                       LPSCARD_READERSTATEW rgReaderStates,
                       DWORD cReaders)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardReconnect(SCARDHANDLE hCard,
               DWORD dwShareMode,
               DWORD dwPreferredProtocols,
               DWORD dwInitialization,
               LPDWORD pdwActiveProtocol)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardReleaseContext(SCARDCONTEXT hContext)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
VOID
WINAPI
SCardReleaseStartedEvent(HANDLE hStartedEventHandle)
{
  UNIMPLEMENTED;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardRemoveReaderFromGroupA(SCARDCONTEXT hContext,
                            LPCSTR szReaderName,
                            LPCSTR szGroupname)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardRemoveReaderFromGroupW(SCARDCONTEXT hContext,
                            LPCWSTR szReaderName,
                            LPCWSTR szGroupname)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardSetAttrib(SCARDHANDLE hCard,
               DWORD dwAttrId,
               LPCBYTE pbAttr,
               DWORD cbAttrLen)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardSetCardTypeProviderNameA(SCARDCONTEXT hContext,
                              LPCSTR szCardName,
                              DWORD dwProviderId,
                              LPCSTR szProvider)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardSetCardTypeProviderNameW(SCARDCONTEXT hContext,
                              LPCWSTR szCardName,
                              DWORD dwProviderId,
                              LPCWSTR szProvider)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardState(SCARDHANDLE hCard,
           LPDWORD pdwState,
           LPDWORD pdwProtocol,
           LPBYTE pbAtr,
           LPDWORD pcbAtrlen)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardStatusA(SCARDHANDLE hCard,
             LPSTR szReaderName,
             LPDWORD pcchReaderLen,
             LPDWORD pdwState,
             LPDWORD pdwProtocol,
             LPBYTE pbAtr,
             LPDWORD pcbAtrLen)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardStatusW(SCARDHANDLE hCard,
             LPWSTR szReaderName,
             LPDWORD pcchReaderLen,
             LPDWORD pdwState,
             LPDWORD pdwProtocol,
             LPBYTE pbAtr,
             LPDWORD pcbAtrLen)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}


/*
 * @unimplemented
 */
LONG
WINAPI
SCardTransmit(SCARDHANDLE hCard,
              LPCSCARD_IO_REQUEST pioSendPci,
              LPCBYTE pbSendBuffer,
              DWORD cbSendLength,
              LPSCARD_IO_REQUEST pioRecvPci,
              LPBYTE pbRecvBuffer,
              LPDWORD pcbRecvLength)
{
  UNIMPLEMENTED;
  return SCARD_F_UNKNOWN_ERROR;
}

