/*
 * Winscard definitions
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

#ifndef __WINE_WINSCARD_H
#define __WINE_WINSCARD_H

#include <wtypes.h>
#include <winioctl.h>
#include <winsmcrd.h>
#include <scarderr.h>

/* Valid scopes for contexts */
#define SCARD_SCOPE_USER     0
#define SCARD_SCOPE_TERMINAL 1
#define SCARD_SCOPE_SYSTEM   2

#ifndef _LPCBYTE_DEFINED
#define _LPCBYTE_DEFINED
typedef const BYTE *LPCBYTE;
#endif

typedef ULONG_PTR SCARDCONTEXT, *PSCARDCONTEXT, *LPSCARDCONTEXT;
typedef ULONG_PTR SCARDHANDLE,  *PSCARDHANDLE,  *LPSCARDHANDLE;

typedef struct _SCARD_ATRMASK
{
    DWORD cbAtr;
    BYTE  rgbAtr[36];
    BYTE  rgbMask[36];
} SCARD_ATRMASK, *PSCARD_ATRMASK, *LPSCARD_ATRMASK;

typedef struct
{
    LPCSTR szReader;
    LPVOID pvUserData;
    DWORD  dwCurrentState;
    DWORD  dwEventState;
    DWORD  cbAtr;
    BYTE   rgbAtr[36];
} SCARD_READERSTATEA, *PSCARD_READERSTATEA, *LPSCARD_READERSTATEA;
typedef struct
{
    LPCWSTR szReader;
    LPVOID  pvUserData;
    DWORD   dwCurrentState;
    DWORD   dwEventState;
    DWORD   cbAtr;
    BYTE    rgbAtr[36];
} SCARD_READERSTATEW, *PSCARD_READERSTATEW, *LPSCARD_READERSTATEW;
DECL_WINELIB_TYPE_AW(SCARD_READERSTATE)
DECL_WINELIB_TYPE_AW(PSCARD_READERSTATE)
DECL_WINELIB_TYPE_AW(LPSCARD_READERSTATE)


#ifdef __cplusplus
extern "C" {
#endif

HANDLE      WINAPI SCardAccessStartedEvent(void);
LONG        WINAPI SCardAddReaderToGroupA(SCARDCONTEXT,LPCSTR,LPCSTR);
LONG        WINAPI SCardAddReaderToGroupW(SCARDCONTEXT,LPCWSTR,LPCWSTR);
#define     SCardAddReaderToGroup WINELIB_NAME_AW(SCardAddReaderToGroup)
LONG        WINAPI SCardBeginTransaction(SCARDHANDLE);
LONG        WINAPI SCardCancel(SCARDCONTEXT);
LONG        WINAPI SCardConnectA(SCARDCONTEXT,LPCSTR,DWORD,DWORD,LPSCARDHANDLE,LPDWORD);
LONG        WINAPI SCardConnectW(SCARDCONTEXT,LPCWSTR,DWORD,DWORD,LPSCARDHANDLE,LPDWORD);
#define     SCardConnect WINELIB_NAME_AW(SCardConnect)
LONG        WINAPI SCardControl(SCARDHANDLE,DWORD,LPCVOID,DWORD,LPVOID,DWORD,LPDWORD);
LONG        WINAPI SCardDisconnect(SCARDHANDLE,DWORD);
LONG        WINAPI SCardEndTransaction(SCARDHANDLE,DWORD);
LONG        WINAPI SCardEstablishContext(DWORD,LPCVOID,LPCVOID,LPSCARDCONTEXT);
LONG        WINAPI SCardForgetCardTypeA(SCARDCONTEXT,LPCSTR);
LONG        WINAPI SCardForgetCardTypeW(SCARDCONTEXT,LPCWSTR);
#define     SCardForgetCardType WINELIB_NAME_AW(SCardForgetCardType)
LONG        WINAPI SCardForgetReaderA(SCARDCONTEXT,LPCSTR);
LONG        WINAPI SCardForgetReaderW(SCARDCONTEXT,LPCWSTR);
#define     SCardForgetReader WINELIB_NAME_AW(SCardForgetReader)
LONG        WINAPI SCardForgetReaderGroupA(SCARDCONTEXT,LPCSTR);
LONG        WINAPI SCardForgetReaderGroupW(SCARDCONTEXT,LPCWSTR);
#define     SCardForgetReaderGroup WINELIB_NAME_AW(SCardForgetReaderGroup)
LONG        WINAPI SCardFreeMemory(SCARDCONTEXT,LPCVOID);
LONG        WINAPI SCardGetAttrib(SCARDHANDLE,DWORD,LPBYTE,LPDWORD);
LONG        WINAPI SCardGetCardTypeProviderNameA(SCARDCONTEXT,LPCSTR,DWORD,LPSTR,LPDWORD);
LONG        WINAPI SCardGetCardTypeProviderNameW(SCARDCONTEXT,LPCWSTR,DWORD,LPWSTR,LPDWORD);
#define     SCardGetCardTypeProviderName WINELIB_NAME_AW(SCardGetCardTypeProviderName)
LONG        WINAPI SCardGetProviderIdA(SCARDCONTEXT,LPCSTR,LPGUID);
LONG        WINAPI SCardGetProviderIdW(SCARDCONTEXT,LPCWSTR,LPGUID);
#define     SCardGetProviderId WINELIB_NAME_AW(SCardGetProviderId)
LONG        WINAPI SCardGetStatusChangeA(SCARDCONTEXT,DWORD,LPSCARD_READERSTATEA,DWORD);
LONG        WINAPI SCardGetStatusChangeW(SCARDCONTEXT,DWORD,LPSCARD_READERSTATEW,DWORD);
#define     SCardGetStatusChange WINELIB_NAME_AW(SCardGetStatusChange)
LONG        WINAPI SCardIntroduceCardTypeA(SCARDCONTEXT,LPCSTR,LPCGUID,LPCGUID,DWORD,LPCBYTE,LPCBYTE,DWORD);
LONG        WINAPI SCardIntroduceCardTypeW(SCARDCONTEXT,LPCWSTR,LPCGUID,LPCGUID,DWORD,LPCBYTE,LPCBYTE,DWORD);
#define     SCardIntroduceCardType WINELIB_NAME_AW(SCardIntroduceCardType)
LONG        WINAPI SCardIntroduceReaderA(SCARDCONTEXT,LPCSTR,LPCSTR);
LONG        WINAPI SCardIntroduceReaderW(SCARDCONTEXT,LPCWSTR,LPCWSTR);
#define     SCardIntroduceReader WINELIB_NAME_AW(SCardIntroduceReader)
LONG        WINAPI SCardIntroduceReaderGroupA(SCARDCONTEXT,LPCSTR);
LONG        WINAPI SCardIntroduceReaderGroupW(SCARDCONTEXT,LPCWSTR);
#define     SCardIntroduceReaderGroup WINELIB_NAME_AW(SCardIntroduceReaderGroup)
LONG        WINAPI SCardIsValidContext(SCARDCONTEXT);
LONG        WINAPI SCardListCardsA(SCARDCONTEXT,LPCBYTE,LPCGUID,DWORD,LPSTR,LPDWORD);
LONG        WINAPI SCardListCardsW(SCARDCONTEXT,LPCBYTE,LPCGUID,DWORD,LPWSTR,LPDWORD);
#define     SCardListCards WINELIB_NAME_AW(SCardListCards)
LONG        WINAPI SCardListInterfacesA(SCARDCONTEXT,LPCSTR,LPGUID,LPDWORD);
LONG        WINAPI SCardListInterfacesW(SCARDCONTEXT,LPCWSTR,LPGUID,LPDWORD);
#define     SCardListInterfaces WINELIB_NAME_AW(SCardListInterfaces)
LONG        WINAPI SCardListReadersA(SCARDCONTEXT,LPCSTR,LPSTR,LPDWORD);
LONG        WINAPI SCardListReadersW(SCARDCONTEXT,LPCWSTR,LPWSTR,LPDWORD);
#define     SCardListReaders WINELIB_NAME_AW(SCardListReaders)
LONG        WINAPI SCardListReaderGroupsA(SCARDCONTEXT,LPSTR,LPDWORD);
LONG        WINAPI SCardListReaderGroupsW(SCARDCONTEXT,LPWSTR,LPDWORD);
#define     SCardListReaderGroups WINELIB_NAME_AW(SCardListReaderGroups)
LONG        WINAPI SCardLocateCardsA(SCARDCONTEXT,LPCSTR,LPSCARD_READERSTATEA,DWORD);
LONG        WINAPI SCardLocateCardsW(SCARDCONTEXT,LPCWSTR,LPSCARD_READERSTATEW,DWORD);
#define     SCardLocateCards WINELIB_NAME_AW(SCardLocateCards)
LONG        WINAPI SCardLocateCardsByATRA(SCARDCONTEXT,LPSCARD_ATRMASK,DWORD,LPSCARD_READERSTATEA,DWORD);
LONG        WINAPI SCardLocateCardsByATRW(SCARDCONTEXT,LPSCARD_ATRMASK,DWORD,LPSCARD_READERSTATEW,DWORD);
#define     SCardLocateCardsByATR WINELIB_NAME_AW(SCardLocateCardsByATR)
LONG        WINAPI SCardReconnect(SCARDHANDLE,DWORD,DWORD,DWORD,LPDWORD);
LONG        WINAPI SCardReleaseContext(SCARDCONTEXT);
void        WINAPI SCardReleaseStartedEvent(void);
LONG        WINAPI SCardRemoveReaderFromGroupA(SCARDCONTEXT,LPCSTR,LPCSTR);
LONG        WINAPI SCardRemoveReaderFromGroupW(SCARDCONTEXT,LPCWSTR,LPCWSTR);
#define     SCardRemoveReaderFromGroup WINELIB_NAME_AW(SCardRemoveReaderFromGroup)
LONG        WINAPI SCardSetAttrib(SCARDHANDLE,DWORD,LPCBYTE,DWORD);
LONG        WINAPI SCardSetCardTypeProviderNameA(SCARDCONTEXT,LPCSTR,DWORD,LPCSTR);
LONG        WINAPI SCardSetCardTypeProviderNameW(SCARDCONTEXT,LPCWSTR,DWORD,LPCWSTR);
#define     SCardSetCardTypeProviderName WINELIB_NAME_AW(SCardSetCardTypeProviderName)
LONG        WINAPI SCardState(SCARDHANDLE,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
LONG        WINAPI SCardStatusA(SCARDHANDLE,LPSTR,LPDWORD,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
LONG        WINAPI SCardStatusW(SCARDHANDLE,LPWSTR,LPDWORD,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
#define     SCardStatus WINELIB_NAME_AW(SCardStatus)
LONG        WINAPI SCardTransmit(SCARDHANDLE,LPCSCARD_IO_REQUEST,LPCBYTE,DWORD,LPSCARD_IO_REQUEST,LPBYTE,LPDWORD);

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_WINSCARD_H */
