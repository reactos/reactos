/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/reg/reg.c
 * PURPOSE:         Registry functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
		    Modified form Wine [ Marcus Meissner ]
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */


#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include "rtlsec.h"




NTSTATUS 
STDCALL 
RtlLengthSid(LPSID Sid)
{
	return sizeof(DWORD)*Sid->SubAuthorityCount+sizeof(SID); 
} 

NTSTATUS 
STDCALL 
RtlCreateAcl(LPACL Acl,DWORD Size,DWORD Revision) {
	if (Revision!=ACL_REVISION)
		return STATUS_INVALID_PARAMETER;
	if (size<sizeof(ACL))
		return STATUS_BUFFER_TOO_SMALL; 
	if (size>0xFFFF)
 		return STATUS_INVALID_PARAMETER; 52 
	memset(Acl,'\0',sizeof(ACL));
	Acl->AclRevision        = Revision;
	Acl->AclSize            = Size;
	Acl->AceCount           = 0; 
	return STATUS_SUCCESS; 
} 

/**************************************************************************
*                 RtlFirstFreeAce                      [NTDLL]
* looks for the AceCount+1 ACE, and if it is still within the alloced
* ACL, return a pointer to it 
*/
BOOLEAN 
STDCALL 
RtlFirstFreeAce(LPACL Acl,LPACE_HEADER *AceHeader) 
{
	LPACE_HEADER    Ace; 
	int             i; 
	*AceHeader = 0;
	Ace = (LPACE_HEADER)(Acl+1);
	for (i=0;i<Acl->AceCount;i++) {
		if ((DWORD)Ace>=(((DWORD)Acl)+Acl->AclSize))
 			return 0;
		Ace = (LPACE_HEADER)(((BYTE*)Ace)+Ace->AceSize);
	} 
 	if ((DWORD)Ace>=(((DWORD)Acl)+Acl->AclSize))
		return 0;
	*AceHeader = Ace;
	return 1;
} 
/**************************************************************************
 *                 RtlAddAce                            [NTDLL]
 */
NTSTATUS  
STDCALL 
RtlAddAce(LPACL Acl,DWORD Revision,DWORD xnrofaces, LPACE_HEADER AceStart,DWORD AceLen)
{ 
	LPACE_HEADER    ace,targetace;
	int             nrofaces; 
	if (Acl->AclRevision != ACL_REVISION)
		return STATUS_INVALID_PARAMETER;
	if (!RtlFirstFreeAce(Acl,&targetace))
		return STATUS_INVALID_PARAMETER;
	nrofaces=0;
	ace=AceStart;
	while (((DWORD)ace-(DWORD)AceStart)<AceLen) {
		nrofaces++;
		ace = (LPACE_HEADER)(((BYTE*)ace)+ace->AceSize);
	}
	if ((DWORD)targetace+AceLen>(DWORD)Acl+Acl->AclSize) /* too much aces */
		return STATUS_INVALID_PARAMETER;
	memcpy((LPBYTE)targetace,AceStart,AceLen);
	Acl->AceCount+=nrofaces;
	return 0;
} 
/**************************************************************************
 *                 RtlCreateSecurityDescriptor          [NTDLL]
 */

// Win32: InitializeSecurityDescriptor
NTSTATUS  
STDCALL 
RtlCreateSecurityDescriptor(LPSECURITY_DESCRIPTOR SecurityDescriptor ,ULONG Revision)
{
	if (Revision !=SECURITY_DESCRIPTOR_REVISION)
		return STATUS_UNKNOWN_REVISION;
	memset(SecurityDescriptor,'\0',sizeof(*SecurityDescriptor));
	SecurityDescriptor->Revision = SECURITY_DESCRIPTOR_REVISION;
	return STATUS_SUCCESS;
}


// Win32: IsValidSecurityDescriptor
BOOLEAN 
NTAPI 
RtlValidSecurityDescriptor(
    PSECURITY_DESCRIPTOR SecurityDescriptor 
    )
{
	if ( SecurityDescriptor == NULL )
		return FALSE;
	if ( SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION )
		return FALSE;
		
}

// Win32: GetSecurityDescriptorLength
ULONG 
NTAPI 
RtlLengthSecurityDescriptor(
   PSECURITY_DESCRIPTOR SecurityDescriptor 
   )
{
	ULONG Size;
	Size = SECURITY_DESCRIPTOR_MIN_LENGTH;
	if ( SecurityDescriptor == NULL ) 
		return 0;

	if ( SecurityDescriptor->Owner != NULL ) 
		Size += SecurityDescriptor->Owner->SubAuthorityCount;
	if ( SecurityDescriptor->Group != NULL ) 
		Size += SecurityDescriptor->Group->SubAuthorityCount;


	if ( SecurityDescriptor->Sacl != NULL ) 
		Size += SecurityDescriptor->Sacl->AclSize;
	if ( SecurityDescriptor->Dacl != NULL ) 
		Size += SecurityDescriptor->Dacl->AclSize;

	return Size;
	

		
}
 
/**************************************************************************
*                 RtlSetDaclSecurityDescriptor         [NTDLL]
*/
// Win32: SetSecurityDescriptorDacl
NTSTATUS 
NTAPI 
RtlSetDaclSecurityDescriptor(
    PSECURITY_DESCRIPTOR SecurityDescriptor, 
    BOOLEAN DaclPresent, 
    PACL Dacl, 
    BOOLEAN DaclDefaulted 
    )
{
	if (SecurityDescriptor->Revision!=SECURITY_DESCRIPTOR_REVISION)
		return STATUS_UNKNOWN_REVISION;
	if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
		return STATUS_INVALID_SECURITY_DESCR;
	if (!DaclPresent) {
		SecurityDescriptor->Control &= ~SE_DACL_PRESENT;
		return 0;
	}
	SecurityDescriptor->Control |= SE_DACL_PRESENT;
	SecurityDescriptor->Dacl = Dacl;
	if (DaclDefaulted)
		SecurityDescriptor->Control |= SE_DACL_DEFAULTED;
	else
		SecurityDescriptor->Control &= ~SE_DACL_DEFAULTED;
	return STATUS_SUCCESS;
}


// Win32: GetSecurityDescriptorDacl
NTSTATUS 
NTAPI 
RtlGetDaclSecurityDescriptor(
    PSECURITY_DESCRIPTOR SecurityDescriptor, 
    PBOOLEAN DaclPresent, 
    PACL *Dacl, 
    PBOOLEAN DaclDefaulted 
    )
{
	if ( SecurityDescriptor == NULL || DaclPresent == NULL || Dacl == NULL || DaclDefaulted == NULL )
		return STATUS_INVALID_PARAMETER;
	if ( SecurityDescriptor->Dacl != NULL )  {
		Dacl = SecurityDescriptor->Dacl;
		if ( Dacl->AceCount > 0 ) 
			*DaclPresent = TRUE;
		else
			*DaclPresent = FALSE; 
		if ( SecurityDescriptor->Control & SE_DACL_DEFAULTED ==
			SE_DACL_DEFAULTED ) {
			*OwnerDefaulted = TRUE;
		else
			*OwnerDefaulted = FALSE;
	}	
	 
}

/**************************************************************************
*                 RtlSetSaclSecurityDescriptor         [NTDLL]147  */
// Win32: SetSecurityDescriptorSacl
// NOTE: NT does not export RtlGetSaclSecurityDescriptor!
NTSTATUS
STDCALL 
RtlSetSaclSecurityDescriptor (
	LPSECURITY_DESCRIPTOR SecurityDescriptor,BOOLEAN SaclPresent,LPACL Sacl,BOOLEAN SaclDefaulted)
{
	if (SecurityDescriptor->Revision!=SECURITY_DESCRIPTOR_REVISION)
		return STATUS_UNKNOWN_REVISION;
	if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
		return STATUS_INVALID_SECURITY_DESCR;
	if (!SaclPresent) {
		SecurityDescriptor->Control &= ~SE_SACL_PRESENT;
		return 0;
	}
	SecurityDescriptor->Control |= SE_SACL_PRESENT;
	SecurityDescriptor->Sacl = Sacl;
	if (SaclDefaulted)
		SecurityDescriptor->Control |= SE_SACL_DEFAULTED;
	else
		SecurityDescriptor->Control &= ~SE_SACL_DEFAULTED;
	return NULL;
}
/**************************************************************************
*                 RtlSetOwnerSecurityDescriptor                [NTDLL]
*/
NTSTATUS 
STDCALL 
RtlSetOwnerSecurityDescriptor (LPSECURITY_DESCRIPTOR SecurityDescriptor,LPSID Owner,BOOLEAN OwnerDefaulted)
{
	if (SecurityDescriptor->Revision!=SECURITY_DESCRIPTOR_REVISION)
		return STATUS_UNKNOWN_REVISION;
	if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
		return STATUS_INVALID_SECURITY_DESCR;180 
	SecurityDescriptor->Owner = Owner;
	if (OwnerDefaulted)
		SecurityDescriptor->Control |= SE_OWNER_DEFAULTED;
	else
		SecurityDescriptor->Control &= ~SE_OWNER_DEFAULTED;
	return STATUS_SUCCESS;
}


// Win32: GetSecurityDescriptorOwner
NTSTATUS 
NTAPI 
RtlGetOwnerSecurityDescriptor(
    PSECURITY_DESCRIPTOR SecurityDescriptor, 
    PSID *Owner, 
    PBOOLEAN OwnerDefaulted 
)
{
	if ( SecurityDescriptor == NULL || Owner == NULL || OwnerDefaulted == NULL )
		return STATUS_INVALID_PARAMETER;
	Owner = SecurityDescriptor->Owner;
	if ( Owner != NULL )  {
		if ( SecurityDescriptor->Control & SE_OWNER_DEFAULTED ==
			SE_OWNER_DEFAULTED ) {
			*OwnerDefaulted = TRUE;
		else
			*OwnerDefaulted = FALSE;
	}
}


/**************************************************************************
*                 RtlSetOwnerSecurityDescriptor                [NTDLL]
*/
// Win32: SetSecurityDescriptorGroup
NTSTATUS 
STDCALL 
RtlSetGroupSecurityDescriptor (LPSECURITY_DESCRIPTOR SecurityDescriptor,LPSID Group,BOOLEAN GroupDefaulted)
{
	if (SecurityDescriptor->Revision!=SECURITY_DESCRIPTOR_REVISION)
		return STATUS_UNKNOWN_REVISION;
	if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
		return STATUS_INVALID_SECURITY_DESCR;
	SecurityDescriptor->Group = Group;
	if (GroupDefaulted)
		SecurityDescriptor->Control |= SE_GROUP_DEFAULTED;
	else
		SecurityDescriptor->Control &= ~SE_GROUP_DEFAULTED;
	return STATUS_SUCCESS;
}

// Win32: GetSecurityDescriptorGroup
NTSTATUS 
NTAPI 
RtlGetGroupSecurityDescriptor(
    PSECURITY_DESCRIPTOR SecurityDescriptor, 
    PSID *Group, 
    PBOOLEAN GroupDefaulted 
)
{
	if ( SecurityDescriptor == NULL || Group == NULL || GroupDefaulted == NULL )
		return STATUS_INVALID_PARAMETER;
	Group = SecurityDescriptor->Group;
	if ( Group != NULL )  {
		if ( SecurityDescriptor->Control & SE_GROUP_DEFAULTED ==
			SE_GROUP_DEFAULTED ) {
			*GroupDefaulted = TRUE;
		else
			*GroupDefaulted = FALSE;
	}
			
	
		
}



 
// Win32: GetSidLengthRequired 
ULONG 
NTAPI 
RtlLengthRequiredSid(
     UCHAR SubAuthorityCount 
     )
{
	return sizeof(DWORD)*SubAuthorityCount+sizeof(SID); 
}

NTSTATUS 
STDCALL 
RtlInitializeSid(LPSID Sid,LPSID_IDENTIFIER_AUTHORITY SidIdentifierAuthority, DWORD nSubAuthorityCount)
{
	BYTE    a = c&0xff;
	if (a>=SID_MAX_SUB_AUTHORITIES)
		return a;
	Sid->SubAuthorityCount = nSubAuthorityCount;
	Sid->Revision          = SID_REVISION;
	memcpy(&(Sid->IdentifierAuthority),SidIdentifierAuthority,sizeof(SID_IDENTIFIER_AUTHORITY));
	return STATUS_SUCCESS;
}
 

// Win32: GetSidSubAuthority
PULONG 
NTAPI 
RtlSubAuthoritySid(
    PSID pSid, 
    ULONG nSubAuthority 
    )
{
	return &(pSid->SubAuthority[nSubAuthority]);
} 

// Win32: IsEqualSid
BOOLEAN 
NTAPI 
RtlEqualSid(
    PSID Sid1, 
    PSID Sid2 
    )
{
	if ( Sid1 == NULL || Sid2 == NULL ) 
		return STATUS_INVALID_PARAMETER;
	
	if ( Sid1 == Sid2 )
		return STATUS_SUCCESS;

	
}


// Win32: MakeSelfRelativeSD
NTSTATUS 
NTAPI 
RtlAbsoluteToSelfRelativeSD(
    PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor, 
    PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor, 
    PULONG BufferLength 
    )
{
	
}

NTSTATUS
NTAPI
RtlValidSid(
	PSID Sid
	)
{
	if ( Sid == NULL )
		return STATUS_INVALID_PARAMETER;
		
	if ( Sid->Revision == SID_REVISION )
		return STATUS_SUCCESS;
}



LPBYTE STDCALL RtlSubAuthorityCountSid(LPSID Sid)
{
         return ((LPBYTE)Sid)+1;
} 


DWORD STDCALL RtlCopySid(DWORD Length,LPSID DestinationSid,LPSID SourceSid)
{
	if (Length<(SourceSid->SubAuthorityCount*4+8))
		return STATUS_BUFFER_TOO_SMALL;
	memmove(DestinationSid,SourceSid,SourceSid->SubAuthorityCount*4+8);
	return STATUS_SUCCESS;
}
 
