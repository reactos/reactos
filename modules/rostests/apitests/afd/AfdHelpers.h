/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Utility function declarations for calling AFD
 * COPYRIGHT:   Copyright 2015 Thomas Faber (thomas.faber@reactos.org)
 *              Copyright 2019 Pierre Schweitzer (pierre@reactos.org)
 */

#pragma once

NTSTATUS
AfdCreateSocket(
    _Out_ PHANDLE SocketHandle,
    _In_ int AddressFamily,
    _In_ int SocketType,
    _In_ int Protocol);

NTSTATUS
AfdBind(
    _In_ HANDLE SocketHandle,
    _In_ const struct sockaddr *Address,
    _In_ ULONG AddressLength);

NTSTATUS
AfdConnect(
    _In_ HANDLE SocketHandle,
    _In_ const struct sockaddr *Address,
    _In_ ULONG AddressLength);

NTSTATUS
AfdSend(
    _In_ HANDLE SocketHandle,
    _In_ const void *Buffer,
    _In_ ULONG BufferLength);

NTSTATUS
AfdSendTo(
    _In_ HANDLE SocketHandle,
    _In_ const void *Buffer,
    _In_ ULONG BufferLength,
    _In_ const struct sockaddr *Address,
    _In_ ULONG AddressLength);

NTSTATUS
AfdSetInformation(
    _In_ HANDLE SocketHandle,
    _In_ ULONG InformationClass,
    _In_opt_ PBOOLEAN Boolean,
    _In_opt_ PULONG Ulong,
    _In_opt_ PLARGE_INTEGER LargeInteger);

NTSTATUS
AfdGetInformation(
    _In_ HANDLE SocketHandle,
    _In_ ULONG InformationClass,
    _In_opt_ PBOOLEAN Boolean,
    _In_opt_ PULONG Ulong,
    _In_opt_ PLARGE_INTEGER LargeInteger);
