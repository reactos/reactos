/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/sefuncs.h
 * PURPOSE:         Defintions for Security Subsystem Functions not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _SEFUNCS_H
#define _SEFUNCS_H

/* DEPENDENCIES **************************************************************/

/* FUNCTION TYPES ************************************************************/

/* PROTOTYPES ****************************************************************/

NTSTATUS
STDCALL
SeCaptureSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR OriginalSecurityDescriptor,
    IN KPROCESSOR_MODE CurrentMode,
    IN POOL_TYPE PoolType,
    IN BOOLEAN CaptureIfKernel,
    OUT PSECURITY_DESCRIPTOR *CapturedSecurityDescriptor
);

NTSTATUS
STDCALL
SeCreateAccessState(
    PACCESS_STATE AccessState,
    PAUX_DATA AuxData,
    ACCESS_MASK Access,
    PGENERIC_MAPPING GenericMapping
);

VOID
STDCALL
SeDeleteAccessState(IN PACCESS_STATE AccessState);

NTSTATUS
STDCALL
SeReleaseSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR CapturedSecurityDescriptor,
    IN KPROCESSOR_MODE CurrentMode,
    IN BOOLEAN CaptureIfKernelMode
);

VOID
STDCALL
SeSetAccessStateGenericMapping(
    IN PACCESS_STATE AccessState,
    IN PGENERIC_MAPPING GenericMapping
);

SECURITY_IMPERSONATION_LEVEL
STDCALL
SeTokenImpersonationLevel(
    IN PACCESS_TOKEN Token
);

#endif
