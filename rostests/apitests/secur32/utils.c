/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for client/server authentication via secur32 API.
 * PROGRAMMERS:     Samuel Serapión
 *                  Hermes Belusca-Maito
 */

#include "client_server.h"

struct CapName
{
    DWORD bits;
    const char *name;
    const char *comment;
} capNames[] = {
    { SECPKG_FLAG_INTEGRITY,            "SECPKG_FLAG_INTEGRITY",            "Supports integrity on messages"          },
    { SECPKG_FLAG_PRIVACY,              "SECPKG_FLAG_PRIVACY",              "Supports privacy (confidentiality)"      },
    { SECPKG_FLAG_TOKEN_ONLY,           "SECPKG_FLAG_TOKEN_ONLY",           "Only security token needed"              },
    { SECPKG_FLAG_DATAGRAM,             "SECPKG_FLAG_DATAGRAM",             "Datagram RPC support"                    },
    { SECPKG_FLAG_CONNECTION,           "SECPKG_FLAG_CONNECTION",           "Connection oriented RPC support"         },
    { SECPKG_FLAG_MULTI_REQUIRED,       "SECPKG_FLAG_MULTI_REQUIRED",       "Full 3-leg required for re-auth."        },
    { SECPKG_FLAG_CLIENT_ONLY,          "SECPKG_FLAG_CLIENT_ONLY",          "Server side functionality not available" },
    { SECPKG_FLAG_EXTENDED_ERROR,       "SECPKG_FLAG_EXTENDED_ERROR",       "Supports extended error msgs"            },
    { SECPKG_FLAG_IMPERSONATION,        "SECPKG_FLAG_IMPERSONATION",        "Supports impersonation"                  },
    { SECPKG_FLAG_ACCEPT_WIN32_NAME,    "SECPKG_FLAG_ACCEPT_WIN32_NAME",    "Accepts Win32 names"                     },
    { SECPKG_FLAG_STREAM,               "SECPKG_FLAG_STREAM",               "Supports stream semantics"               },
    { SECPKG_FLAG_NEGOTIABLE,           "SECPKG_FLAG_NEGOTIABLE",           "Can be used by the negotiate package"    },
    { SECPKG_FLAG_GSS_COMPATIBLE,       "SECPKG_FLAG_GSS_COMPATIBLE",       "GSS Compatibility Available"             },
    { SECPKG_FLAG_LOGON,                "SECPKG_FLAG_LOGON",                "Supports common LsaLogonUser"            },
    { SECPKG_FLAG_ASCII_BUFFERS,        "SECPKG_FLAG_ASCII_BUFFERS",        "Token Buffers are in ASCII"              },
    { 0xffffffffL, "(fence)", "(fence)" }
};

void initSecLib(
    HINSTANCE* phSec,
    PSecurityFunctionTable* pSecFuncTable)
{
    SECURITY_STATUS rc;
    DWORD numPacks = 0, i, j;
    SecPkgInfoW *pPacks = NULL;

    PSecurityFunctionTable (*pInitSecurityInterfaceW)(void);

    assert(phSec);
    assert(pSecFuncTable);

    *phSec = LoadLibraryW(L"security.dll");
    pInitSecurityInterfaceW = (PSecurityFunctionTable(*)(void))GetProcAddress(*phSec, "InitSecurityInterfaceW");
    if (pInitSecurityInterfaceW == NULL)
    {
        wprintf(L"security.dll loading failed...");
        exit(1);
    }

    *pSecFuncTable = pInitSecurityInterfaceW();
    if (*pSecFuncTable == NULL)
    {
        wprintf(L"no function table?!?");
        exit(1);
    }

    rc = ((*pSecFuncTable)->EnumerateSecurityPackages)(&numPacks, &pPacks);
    if (rc != 0)
    {
        printf("ESP() returned %ld\n", rc);
        exit(1);
    }

    for (i = 0; i < numPacks; ++ i)
    {
        printf("\nPackage: %S\n", pPacks[i].Name);
        printf("  Version:      %hu\n", pPacks[i].wVersion);
        printf("  RPCID:        %hu%S\n", pPacks[i].wRPCID,
            pPacks[i].wRPCID == SECPKG_ID_NONE? " [none]": "");
        printf("  Comment:      \"%S\"\n", pPacks[i].Comment);
        printf("  Capabilities: %08lxh\n", pPacks[i].fCapabilities);
        for (j = 0; capNames[j].bits != 0xffffffffL; ++j)
        {
            if ((capNames[j].bits & pPacks[i].fCapabilities) == capNames[j].bits)
                printf("    %s (%s)\n", capNames[j].name, capNames[j].comment);
        }
    }

    if (pPacks != NULL)
        ((*pSecFuncTable)->FreeContextBuffer)(pPacks);
}


/* wserr() displays winsock errors and aborts. No grace there. */
void wserr(int rc, LPCWSTR const funcname)
{
    DWORD errnum = WSAGetLastError();
    LPWSTR errbuffer;

    if (rc == 0)
        return;

    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL,
                   errnum,
                   LANG_USER_DEFAULT,
                   (LPWSTR)&errbuffer,
                   0,
                   NULL);

    fwprintf(stderr, L"\nWinsock error %d [gle %d] returned by %s().\n",
             rc, errnum);
    fwprintf(stderr, L"\nError string: %s.\n",errbuffer);

    LocalFree(errbuffer);
    WSACleanup();
    // exit(rc);
}

void PrintHexDump(
    DWORD length,
    PBYTE buffer)
{
    DWORD i,count,index;
    CHAR rgbDigits[]="0123456789abcdef";
    CHAR rgbLine[100];
    char cbLine;

    for (index = 0; length;
         length -= count, buffer += count, index += count)
    {
        count = (length > 16) ? 16:length;

        snprintf(rgbLine, 100, "%4.4x  ",index);
        cbLine = 6;

        for (i = 0; i < count; i++)
        {
            rgbLine[cbLine++] = rgbDigits[buffer[i] >> 4];
            rgbLine[cbLine++] = rgbDigits[buffer[i] & 0x0f];
            if (i == 7)
            {
                rgbLine[cbLine++] = ':';
            }
            else
            {
                rgbLine[cbLine++] = ' ';
            }
        }
        for (; i < 16; i++)
        {
            rgbLine[cbLine++] = ' ';
            rgbLine[cbLine++] = ' ';
            rgbLine[cbLine++] = ' ';
        }

        rgbLine[cbLine++] = ' ';

        for (i = 0; i < count; i++)
        {
            if (buffer[i] < 32 || buffer[i] > 126)
            {
                rgbLine[cbLine++] = '.';
            }
            else
            {
                rgbLine[cbLine++] = buffer[i];
            }
        }

        rgbLine[cbLine++] = 0;
        printf("%s\n", rgbLine);
    }
}

BOOL SendMsg(
    SOCKET s,
    PBYTE pBuf,
    DWORD cbBuf)
{
    if (cbBuf == 0)
        return TRUE;

    /* Send the size of the message */
    if (!SendBytes(s,
                   (PBYTE)&cbBuf,
                   sizeof(cbBuf)))
    {
        return FALSE;
    }

    /* Send the body of the message */
    if (!SendBytes(s,
                   pBuf,
                   cbBuf))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL SendBytes(
    SOCKET s,
    PBYTE pBuf,
    DWORD cbBuf)
{
    PBYTE pTemp = pBuf;
    int cbSent, cbRemaining = cbBuf;

    trace("sending %d bytes from buffer %p to socket %x\n", cbBuf, pBuf, s);

    if (!pBuf)
    {
        err("received null buffer!\n");
        return FALSE;
    }

    if (cbBuf == 0)
        return TRUE;

    while (cbRemaining)
    {
        cbSent = send(s,
                      (const char *)pTemp,
                      cbRemaining,
                      0);

        if (SOCKET_ERROR == cbSent)
        {
            err("send failed: %u\n", GetLastError());
            printerr(GetLastError());
            return FALSE;
        }

        pTemp += cbSent;
        cbRemaining -= cbSent;
    }

    return TRUE;
}

BOOL ReceiveMsg(
    SOCKET s,
    PBYTE pBuf,
    DWORD cbBuf,
    DWORD *pcbRead)
{
    DWORD cbRead;
    DWORD cbData;

    /* Retrieve the number of bytes in the message */
    if (!ReceiveBytes(s,
                      (PBYTE)&cbData,
                      sizeof(cbData),
                      &cbRead))
    {
        return FALSE;
    }

    trace("message size = %d\n", cbData);

    if (sizeof(cbData) != cbRead)
        return FALSE;

    /* Read the full message */
    if (!ReceiveBytes(s,
                      pBuf,
                      cbData,
                      &cbRead))
    {
        return FALSE;
    }

    if (cbRead != cbData)
    {
        err("cbRead != cbData, was %d %d\n",cbRead, cbData);
        return FALSE;
    }

    *pcbRead = cbRead;
    return TRUE;
}

BOOL ReceiveBytes(
    SOCKET s,
    PBYTE pBuf,
    DWORD cbBuf,
    DWORD *pcbRead)
{
    PBYTE pTemp = pBuf;
    int cbRead, cbRemaining = cbBuf;

    trace("reading %d bytes from socket %x to buffer %p\n", cbBuf, s, pBuf);

    if (!pBuf)
    {
        err("received null buffer!\n");
        return FALSE;
    }

    while (cbRemaining)
    {
        cbRead = recv(s, (char*)pTemp, cbBuf, 0);
        if (cbRead < 0 || cbRead == SOCKET_ERROR)
        {
            err("recv failed: %d\n", WSAGetLastError());
            printerr(WSAGetLastError());
            fatal_error("abort");
            break;
        }
        trace("received %d bytes, %d remain in buffer\n", cbRead, cbRemaining);

        if (cbRead == 0)
            break;

        cbRemaining -= cbRead;
        pTemp += cbRead;
    }

    *pcbRead = cbBuf - cbRemaining;
    return TRUE;
}

DWORD inet_addr_w(const WCHAR *pszAddr)
{
    char szBuffer[256];

    WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
        pszAddr, -1, szBuffer, 256, NULL, NULL);

    return inet_addr(szBuffer);
}
