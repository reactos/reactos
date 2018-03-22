/*
 * Internet Messaging APIs
 *
 * Copyright 2006 Robert Shearman for CodeWeavers
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

#pragma once

#include "winsock2.h"
#include "winuser.h"
#include "objbase.h"
#include "imnxport.h"

typedef struct InternetTransport InternetTransport;

typedef void (*INETXPORT_COMPLETION_FUNCTION)(IInternetTransport *, char *, int);

struct InternetTransport
{
    union
    {
        const IInternetTransportVtbl *vtbl;
        const ISMTPTransport2Vtbl *vtblSMTP2;
        const IIMAPTransportVtbl *vtblIMAP;
        const IPOP3TransportVtbl *vtblPOP3;
    } u;

    ITransportCallback *pCallback;
    IXPSTATUS Status;
    INETSERVER ServerInfo;
    SOCKET Socket;
    boolean fCommandLogging;
    boolean fInitialised;
    INETXPORT_COMPLETION_FUNCTION fnCompletion;
    char *pBuffer;
    int cbBuffer;
    int iCurrentBufferOffset;
    HWND hwnd;
};

HRESULT InternetTransport_Init(InternetTransport *This) DECLSPEC_HIDDEN;
HRESULT InternetTransport_GetServerInfo(InternetTransport *This, LPINETSERVER pInetServer) DECLSPEC_HIDDEN;
HRESULT InternetTransport_InetServerFromAccount(InternetTransport *This,
    IImnAccount *pAccount, LPINETSERVER pInetServer) DECLSPEC_HIDDEN;
HRESULT InternetTransport_Connect(InternetTransport *This,
    LPINETSERVER pInetServer, boolean fAuthenticate, boolean fCommandLogging) DECLSPEC_HIDDEN;
HRESULT InternetTransport_HandsOffCallback(InternetTransport *This) DECLSPEC_HIDDEN;
HRESULT InternetTransport_DropConnection(InternetTransport *This) DECLSPEC_HIDDEN;
HRESULT InternetTransport_GetStatus(InternetTransport *This,
    IXPSTATUS *pCurrentStatus) DECLSPEC_HIDDEN;
HRESULT InternetTransport_ChangeStatus(InternetTransport *This, IXPSTATUS Status) DECLSPEC_HIDDEN;
HRESULT InternetTransport_ReadLine(InternetTransport *This,
    INETXPORT_COMPLETION_FUNCTION fnCompletion) DECLSPEC_HIDDEN;
HRESULT InternetTransport_Write(InternetTransport *This, const char *pvData,
    int cbSize, INETXPORT_COMPLETION_FUNCTION fnCompletion) DECLSPEC_HIDDEN;
HRESULT InternetTransport_DoCommand(InternetTransport *This,
    LPCSTR pszCommand, INETXPORT_COMPLETION_FUNCTION fnCompletion) DECLSPEC_HIDDEN;

BOOL InternetTransport_RegisterClass(HINSTANCE hInstance) DECLSPEC_HIDDEN;
void InternetTransport_UnregisterClass(HINSTANCE hInstance) DECLSPEC_HIDDEN;

HRESULT MimeBody_create(IUnknown *outer, void **obj) DECLSPEC_HIDDEN;
HRESULT MimeAllocator_create(IUnknown *outer, void **obj) DECLSPEC_HIDDEN;
HRESULT MimeMessage_create(IUnknown *outer, void **obj) DECLSPEC_HIDDEN;
HRESULT MimeSecurity_create(IUnknown *outer, void **obj) DECLSPEC_HIDDEN;
HRESULT VirtualStream_create(IUnknown *outer, void **obj) DECLSPEC_HIDDEN;
HRESULT MimeHtmlProtocol_create(IUnknown *outer, void **obj) DECLSPEC_HIDDEN;

HRESULT MimeInternational_Construct(IMimeInternational **internat) DECLSPEC_HIDDEN;

HRESULT SMTPTransportCF_Create(REFIID riid, LPVOID *ppv) DECLSPEC_HIDDEN;
HRESULT IMAPTransportCF_Create(REFIID riid, LPVOID *ppv) DECLSPEC_HIDDEN;
HRESULT POP3TransportCF_Create(REFIID riid, LPVOID *ppv) DECLSPEC_HIDDEN;
