/*
 * COPYRIGHT:         See COPYING in the top level directory for details
 * PROJECT:           ReactOS kernel
 * FILE:              include/ddk/setypes.h
 * PURPOSE:           Security manager types
 * REVISION HISTORY:
 *                 ??/??/??:    Created with empty stubs by David Welch
 *                 29/08/98:    ACCESS_TOKEN definition from Boudewijn Dekker
 */

#ifndef __INCLUDE_DDK_SETYPES_H
#define __INCLUDE_DDK_SETYPES_H

typedef ULONG ACCESS_MODE, *PACCESS_MODE;
typedef SECURITY_QUALITY_OF_SERVICE* PSECURITY_QUALITY_OF_SERVICE;

typedef struct _SECURITY_SUBJECT_CONTEXT
{
} SECURITY_SUBJECT_CONTEXT, *PSECURITY_SUBJECT_CONTEXT;

typedef struct _SECURITY_DESCRIPTOR_CONTEXT
{
} SECURITY_DESCRIPTOR_CONTEXT, *PSECURITY_DESCRIPTOR_CONTEXT;

typedef struct _ACCESS_TOKEN {
	TOKEN_SOURCE			TokenSource;
	LUID				AuthenticationId;
	LARGE_INTEGER			ExpirationTime;
	LUID				ModifiedId;
	ULONG				UserAndGroupCount;
	ULONG				PrivilegeCount;
	ULONG				VariableLength;
	ULONG				DynamicCharged;
	ULONG				DynamicAvailable;
	ULONG				DefaultOwnerIndex;
	PACL				DefaultDacl;
	TOKEN_TYPE			TokenType;
	SECURITY_IMPERSONATION_LEVEL 	ImpersonationLevel;
	UCHAR				TokenFlags;
	UCHAR				TokenInUse;
	UCHAR				Unused[2];
	PVOID				ProxyData;
	PVOID				AuditData;
	UCHAR				VariablePart[0];
} ACCESS_TOKEN, *PACCESS_TOKEN;

#endif
