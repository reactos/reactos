/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/rtlsec.h
 * PURPOSE:         Registry functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */
NTSTATUS 
STDCALL 
RtlLengthSid(LPSID Sid);

NTSTATUS 
STDCALL 
RtlCreateAcl(LPACL Acl,DWORD Size,DWORD Revision);

BOOLEAN 
STDCALL 
RtlFirstFreeAce(LPACL Acl,LPACE_HEADER *AceHeader);

NTSTATUS  
STDCALL 
RtlAddAce(LPACL Acl,DWORD Revision,DWORD xnrofaces, LPACE_HEADER AceStart,DWORD AceLen)

NTSTATUS  
STDCALL 
RtlCreateSecurityDescriptor(LPSECURITY_DESCRIPTOR SecurityDescriptor ,ULONG Revision);

BOOLEAN 
NTAPI 
RtlValidSecurityDescriptor(
    PSECURITY_DESCRIPTOR SecurityDescriptor 
    );

ULONG 
NTAPI 
RtlLengthSecurityDescriptor(
   PSECURITY_DESCRIPTOR SecurityDescriptor 
   );

NTSTATUS 
NTAPI 
RtlSetDaclSecurityDescriptor(
    PSECURITY_DESCRIPTOR SecurityDescriptor, 
    BOOLEAN DaclPresent, 
    PACL Dacl, 
    BOOLEAN DaclDefaulted 
    );

NTSTATUS 
NTAPI 
RtlGetDaclSecurityDescriptor(
    PSECURITY_DESCRIPTOR SecurityDescriptor, 
    PBOOLEAN DaclPresent, 
    PACL *Dacl, 
    PBOOLEAN DaclDefaulted 
    );

NTSTATUS
STDCALL 
RtlSetSaclSecurityDescriptor (
	LPSECURITY_DESCRIPTOR SecurityDescriptor,BOOLEAN SaclPresent,LPACL Sacl,BOOLEAN SaclDefaulted);

NTSTATUS 
STDCALL 
RtlSetOwnerSecurityDescriptor (LPSECURITY_DESCRIPTOR SecurityDescriptor,LPSID Owner,BOOLEAN OwnerDefaulted);

NTSTATUS 
NTAPI 
RtlGetOwnerSecurityDescriptor(
    PSECURITY_DESCRIPTOR SecurityDescriptor, 
    PSID *Owner, 
    PBOOLEAN OwnerDefaulted 
);

NTSTATUS 
STDCALL 
RtlSetGroupSecurityDescriptor (LPSECURITY_DESCRIPTOR SecurityDescriptor,LPSID Group,BOOLEAN GroupDefaulted);

NTSTATUS 
NTAPI 
RtlGetGroupSecurityDescriptor(
    PSECURITY_DESCRIPTOR SecurityDescriptor, 
    PSID *Group, 
    PBOOLEAN GroupDefaulted 
);

ULONG 
NTAPI 
RtlLengthRequiredSid(
     UCHAR SubAuthorityCount 
     );

NTSTATUS 
STDCALL 
RtlInitializeSid(LPSID Sid,LPSID_IDENTIFIER_AUTHORITY SidIdentifierAuthority, DWORD nSubAuthorityCount);

PULONG 
NTAPI 
RtlSubAuthoritySid(
    PSID pSid, 
    ULONG nSubAuthority 
    );

BOOLEAN 
NTAPI 
RtlEqualSid(
    PSID Sid1, 
    PSID Sid2 
    );

NTSTATUS 
NTAPI 
RtlAbsoluteToSelfRelativeSD(
    PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor, 
    PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor, 
    PULONG BufferLength 
    );

NTSTATUS
NTAPI
RtlValidSid(
	PSID Sid
	);

LPBYTE STDCALL RtlSubAuthorityCountSid(LPSID Sid);

DWORD STDCALL RtlCopySid(DWORD Length,LPSID DestinationSid,LPSID SourceSid);




