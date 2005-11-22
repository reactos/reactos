/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    sefuncs.h

Abstract:

    Function definitions for the security manager.

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

--*/

#ifndef _SEFUNCS_H
#define _SEFUNCS_H

//
// Security Descriptors
//
NTSTATUS
NTAPI
SeCaptureSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR OriginalSecurityDescriptor,
    IN KPROCESSOR_MODE CurrentMode,
    IN POOL_TYPE PoolType,
    IN BOOLEAN CaptureIfKernel,
    OUT PSECURITY_DESCRIPTOR *CapturedSecurityDescriptor
);

NTSTATUS
NTAPI
SeReleaseSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR CapturedSecurityDescriptor,
    IN KPROCESSOR_MODE CurrentMode,
    IN BOOLEAN CaptureIfKernelMode
);

//
// Access States
//
NTSTATUS
NTAPI
SeCreateAccessState(
    PACCESS_STATE AccessState,
    PAUX_DATA AuxData,
    ACCESS_MASK Access,
    PGENERIC_MAPPING GenericMapping
);

VOID
NTAPI
SeDeleteAccessState(
    IN PACCESS_STATE AccessState
);

//
// Impersonation
//
SECURITY_IMPERSONATION_LEVEL
NTAPI
SeTokenImpersonationLevel(
    IN PACCESS_TOKEN Token
);

#endif
