//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        proxyapi.cxx
//
// Contents:    Code for Proxy support in NtLM
//              Main entry points in the dll:
//                SpGrantProxy
//                SpRevokeProxy
//                SpInvokeProxy
//                SpRenewProxy
//
//
// History:     ChandanS  25-Jul-1996   Stolen from kerberos\client2\proxyapi.cxx
//
//------------------------------------------------------------------------

#include <global.h>

NTSTATUS NTAPI
SpGrantProxy(
    IN ULONG_PTR CredentialHandle,
    IN OPTIONAL PUNICODE_STRING ProxyName,
    IN PROXY_CLASS ProxyClass,
    IN OPTIONAL PUNICODE_STRING TargetName,
    IN ACCESS_MASK ContainerMask,
    IN ACCESS_MASK ObjectMask,
    IN PTimeStamp ExpirationTime,
    IN PSecBuffer AccessInformation,
    OUT PPROXY_REFERENCE ProxyReference
    )
{
    SspPrint((SSP_API, "Entering SpGrantProxy\n"));

    UNREFERENCED_PARAMETER (CredentialHandle);
    UNREFERENCED_PARAMETER (ProxyName);
    UNREFERENCED_PARAMETER (ProxyClass);
    UNREFERENCED_PARAMETER (TargetName);
    UNREFERENCED_PARAMETER (ContainerMask);
    UNREFERENCED_PARAMETER (ObjectMask);
    UNREFERENCED_PARAMETER (ExpirationTime);
    UNREFERENCED_PARAMETER (AccessInformation);
    UNREFERENCED_PARAMETER (ProxyReference);

    SspPrint((SSP_API, "Leaving SpGrantProxy\n"));
    return(STATUS_NOT_SUPPORTED);
}

NTSTATUS NTAPI
SpRevokeProxy(
    IN ULONG_PTR CredentialHandle,
    IN OPTIONAL PPROXY_REFERENCE ProxyReference,
    IN OPTIONAL PUNICODE_STRING ProxyName
    )
{
    SspPrint((SSP_API, "Entering SpRevokeProxy\n"));

    UNREFERENCED_PARAMETER (CredentialHandle);
    UNREFERENCED_PARAMETER (ProxyReference);
    UNREFERENCED_PARAMETER (ProxyName);

    SspPrint((SSP_API, "Leaving SpRevokeProxy\n"));
    return(STATUS_NOT_SUPPORTED);
}

NTSTATUS NTAPI
SpInvokeProxy(
    IN ULONG_PTR CredentialHandle,
    IN OPTIONAL PPROXY_REFERENCE ProxyReference,
    IN OPTIONAL PUNICODE_STRING ProxyName,
    OUT PULONG_PTR ContextHandle,
    OUT PLUID LogonId,
    OUT PULONG CachedCredentialCount,
    OUT PSECPKG_SUPPLEMENTAL_CRED * CachedCredentials,
    OUT PSecBuffer ContextData
    )
{
    SspPrint((SSP_API, "Entering SpInvokeProxy\n"));

    UNREFERENCED_PARAMETER (CredentialHandle);
    UNREFERENCED_PARAMETER (ProxyReference);
    UNREFERENCED_PARAMETER (ProxyName);
    UNREFERENCED_PARAMETER (ContextHandle);
    UNREFERENCED_PARAMETER (LogonId);
    UNREFERENCED_PARAMETER (CachedCredentialCount);
    UNREFERENCED_PARAMETER (CachedCredentials);
    UNREFERENCED_PARAMETER (ContextData);

    SspPrint((SSP_API, "Leaving SpInvokeProxy\n"));
    return(STATUS_NOT_SUPPORTED);
}


NTSTATUS NTAPI
SpRenewProxy(
    IN ULONG_PTR CredentialHandle,
    IN OPTIONAL PPROXY_REFERENCE ProxyReference,
    IN OPTIONAL PUNICODE_STRING ProxyName,
    IN PTimeStamp ExpirationTime
    )
{
    SspPrint((SSP_API, "Entering SpRenewProxy\n"));

    UNREFERENCED_PARAMETER (CredentialHandle);
    UNREFERENCED_PARAMETER (ProxyReference);
    UNREFERENCED_PARAMETER (ProxyName);
    UNREFERENCED_PARAMETER (ExpirationTime);

    SspPrint((SSP_API, "Leaving SpRenewProxy\n"));
    return(STATUS_NOT_SUPPORTED);
}
