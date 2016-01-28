/*
 * Copyright 2007 Mounir IDRASSI  (mounir.idrassi@idrix.fr, for IDRIX)
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

#include <precomp.h>

#include <winternl.h>

static HANDLE g_startedEvent = NULL;

const SCARD_IO_REQUEST g_rgSCardT0Pci = { SCARD_PROTOCOL_T0, 8 };
const SCARD_IO_REQUEST g_rgSCardT1Pci = { SCARD_PROTOCOL_T1, 8 };
const SCARD_IO_REQUEST g_rgSCardRawPci = { SCARD_PROTOCOL_RAW, 8 };


BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p,%x,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            /* FIXME: for now, we act as if the pcsc daemon is always started */
            g_startedEvent = CreateEventA(NULL,TRUE,TRUE,NULL);
            break;
        case DLL_PROCESS_DETACH:
            if (lpvReserved) break;
            CloseHandle(g_startedEvent);
            break;
    }

    return TRUE;
}

HANDLE WINAPI SCardAccessStartedEvent(void)
{
    return g_startedEvent;
}

LONG WINAPI SCardAddReaderToGroupA(SCARDCONTEXT context, LPCSTR reader, LPCSTR group)
{
    LONG retval;
    UNICODE_STRING readerW, groupW;

    if(reader) RtlCreateUnicodeStringFromAsciiz(&readerW,reader);
    else readerW.Buffer = NULL;
    if(group) RtlCreateUnicodeStringFromAsciiz(&groupW,group);
    else groupW.Buffer = NULL;

    retval = SCardAddReaderToGroupW(context, readerW.Buffer, groupW.Buffer);

    RtlFreeUnicodeString(&readerW);
    RtlFreeUnicodeString(&groupW);

    return retval;
}

LONG WINAPI SCardAddReaderToGroupW(SCARDCONTEXT context, LPCWSTR reader, LPCWSTR group)
{
    FIXME("%x %s %s\n", (unsigned int) context, debugstr_w(reader), debugstr_w(group));
    return SCARD_S_SUCCESS;
}

LONG WINAPI SCardEstablishContext(DWORD dwScope, LPCVOID pvReserved1,
    LPCVOID pvReserved2, LPSCARDCONTEXT phContext)
{
    FIXME("(%x,%p,%p,%p) stub\n", dwScope, pvReserved1, pvReserved2, phContext);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return SCARD_F_INTERNAL_ERROR;
}

LONG WINAPI SCardIsValidContext(SCARDCONTEXT context)
{
    FIXME("(%lx) stub\n", context);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return SCARD_F_INTERNAL_ERROR;
}

LONG WINAPI SCardListCardsA(SCARDCONTEXT hContext, LPCBYTE pbAtr, LPCGUID rgguidInterfaces, DWORD cguidInterfaceCount, LPSTR mszCards, LPDWORD pcchCards)
{
    FIXME(": stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return SCARD_F_INTERNAL_ERROR;
}

LONG WINAPI SCardReleaseContext(SCARDCONTEXT context)
{
    FIXME("(%lx) stub\n", context);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return SCARD_F_INTERNAL_ERROR;
}

LONG WINAPI SCardStatusA(SCARDHANDLE context, LPSTR szReaderName, LPDWORD pcchReaderLen, LPDWORD pdwState, LPDWORD pdwProtocol, LPBYTE pbAtr, LPDWORD pcbAtrLen)
{
    FIXME("(%lx) stub\n", context);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return SCARD_F_INTERNAL_ERROR;
}

LONG WINAPI SCardStatusW(SCARDHANDLE context, LPWSTR szReaderName, LPDWORD pcchReaderLen, LPDWORD pdwState,LPDWORD pdwProtocol,LPBYTE pbAtr,LPDWORD pcbArtLen)
{
    FIXME("(%lx) stub\n", context);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return SCARD_F_INTERNAL_ERROR;
}

void WINAPI SCardReleaseStartedEvent(void)
{
    FIXME("stub\n");
}

LONG WINAPI SCardListReadersA(SCARDCONTEXT context, LPCSTR groups, LPSTR readers, LPDWORD buflen)
{
    FIXME("(%lx, %s, %p, %p) stub\n", context, debugstr_a(groups), readers, buflen);
    return SCARD_F_INTERNAL_ERROR;
}

LONG WINAPI SCardListReadersW(SCARDCONTEXT context, LPCWSTR groups, LPWSTR readers, LPDWORD buflen)
{
    FIXME("(%lx, %s, %p, %p) stub\n", context, debugstr_w(groups), readers, buflen);
    return SCARD_F_INTERNAL_ERROR;
}
