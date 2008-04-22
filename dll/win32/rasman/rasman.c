/*
 * RASMAN
 *
 * Copyright 2007 ReactOS Team
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

#include <stdarg.h>

#include <windows.h>
#include <ras.h>
#include "wine/debug.h"
#include <rasshost.h>

static HINSTANCE hDllInstance;

WINE_DEFAULT_DEBUG_CHANNEL(rasman);

VOID WINAPI
RasSecurityDialogComplete(SECURITY_MESSAGE* pSecMsg)
{
    FIXME("(%p),stub!\n",pSecMsg);
}

DWORD WINAPI
RasSecurityDialogBegin(HPORT hPort, PBYTE pSendBuf,
    DWORD SendBufSize, PBYTE pRecvBuf, DWORD RecvBufSize,
    VOID (WINAPI *RasSecurityDialogComplete)())
{
    FIXME("(%p,%p,0x%08x,%p,0x%08x,%p),stub!\n",hPort,pSendBuf,SendBufSize,
          pRecvBuf,RecvBufSize,*RasSecurityDialogComplete);
    return NO_ERROR;
}

DWORD WINAPI
RasSecurityDialogSend(HPORT hPort, PBYTE pBuffer, WORD BufferLength)
{
    FIXME("(%p,%p,%x),stub!\n",hPort, pBuffer, BufferLength);
    return NO_ERROR;
}

DWORD WINAPI
RasSecurityDialogReceive(HPORT hPort, PBYTE pBuffer, PWORD pBufferLength, DWORD Timeout, HANDLE hEvent)
{
    FIXME("(%p,%p,%p,%x,%p),stub!\n",hPort, pBuffer, pBufferLength, Timeout, hEvent);
    return NO_ERROR;
}

DWORD WINAPI
RasSecurityDialogEnd(HPORT hPort)
{
    FIXME("(%p),stub!\n",hPort);
    return NO_ERROR;
}

DWORD WINAPI
RasSecurityDialogGetInfo(HPORT hPort, RAS_SECURITY_INFO* pBuffer)
{
    FIXME("(%p,%p),stub!\n",hPort,pBuffer);
    return NO_ERROR;
}

BOOL WINAPI
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            hDllInstance = hinstDLL;
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

    return TRUE;
}

