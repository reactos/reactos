/* $Id: acl.c,v 1.21 2004/11/06 21:32:16 navaraf Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security manager
 * FILE:              kernel/se/acl.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 * REVISION HISTORY:
 *                 26/07/98: Added stubs for security functions
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

#define TAG_ACL    TAG('A', 'C', 'L', 'T')


/* GLOBALS ******************************************************************/

PACL EXPORTED SePublicDefaultDacl = NULL;
PACL EXPORTED SeSystemDefaultDacl = NULL;

PACL SePublicDefaultUnrestrictedDacl = NULL;
PACL SePublicOpenDacl = NULL;
PACL SePublicOpenUnrestrictedDacl = NULL;
PACL SeUnrestrictedDacl = NULL;


/* FUNCTIONS ****************************************************************/

BOOLEAN INIT_FUNCTION
SepInitDACLs(VOID)
{
  ULONG AclLength;

  /* create PublicDefaultDacl */
  AclLength = sizeof(ACL) +
	      (sizeof(ACE) + RtlLengthSid(SeWorldSid)) +
	      (sizeof(ACE) + RtlLengthSid(SeLocalSystemSid));

  SePublicDefaultDacl = ExAllocatePoolWithTag(NonPagedPool,
					      AclLength,
					      TAG_ACL);
  if (SePublicDefaultDacl == NULL)
    return FALSE;

  RtlCreateAcl(SePublicDefaultDacl,
	       AclLength,
	       ACL_REVISION);

  RtlAddAccessAllowedAce(SePublicDefaultDacl,
			 ACL_REVISION,
			 GENERIC_EXECUTE,
			 SeWorldSid);

  RtlAddAccessAllowedAce(SePublicDefaultDacl,
			 ACL_REVISION,
			 GENERIC_ALL,
			 SeLocalSystemSid);


  /* create PublicDefaultUnrestrictedDacl */
  AclLength = sizeof(ACL) +
	      (sizeof(ACE) + RtlLengthSid(SeWorldSid)) +
	      (sizeof(ACE) + RtlLengthSid(SeLocalSystemSid)) +
	      (sizeof(ACE) + RtlLengthSid(SeAliasAdminsSid)) +
	      (sizeof(ACE) + RtlLengthSid(SeRestrictedCodeSid));

  SePublicDefaultUnrestrictedDacl = ExAllocatePoolWithTag(NonPagedPool,
							  AclLength,
							  TAG_ACL);
  if (SePublicDefaultUnrestrictedDacl == NULL)
    return FALSE;

  RtlCreateAcl(SePublicDefaultUnrestrictedDacl,
	       AclLength,
	       ACL_REVISION);

  RtlAddAccessAllowedAce(SePublicDefaultUnrestrictedDacl,
			 ACL_REVISION,
			 GENERIC_EXECUTE,
			 SeWorldSid);

  RtlAddAccessAllowedAce(SePublicDefaultUnrestrictedDacl,
			 ACL_REVISION,
			 GENERIC_ALL,
			 SeLocalSystemSid);

  RtlAddAccessAllowedAce(SePublicDefaultUnrestrictedDacl,
			 ACL_REVISION,
			 GENERIC_ALL,
			 SeAliasAdminsSid);

  RtlAddAccessAllowedAce(SePublicDefaultUnrestrictedDacl,
			 ACL_REVISION,
			 GENERIC_READ | GENERIC_EXECUTE | READ_CONTROL,
			 SeRestrictedCodeSid);

  /* create PublicOpenDacl */
  AclLength = sizeof(ACL) +
	      (sizeof(ACE) + RtlLengthSid(SeWorldSid)) +
	      (sizeof(ACE) + RtlLengthSid(SeLocalSystemSid)) +
	      (sizeof(ACE) + RtlLengthSid(SeAliasAdminsSid));

  SePublicOpenDacl = ExAllocatePoolWithTag(NonPagedPool,
					   AclLength,
					   TAG_ACL);
  if (SePublicOpenDacl == NULL)
    return FALSE;

  RtlCreateAcl(SePublicOpenDacl,
	       AclLength,
	       ACL_REVISION);

  RtlAddAccessAllowedAce(SePublicOpenDacl,
			 ACL_REVISION,
			 GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE,
			 SeWorldSid);

  RtlAddAccessAllowedAce(SePublicOpenDacl,
			 ACL_REVISION,
			 GENERIC_ALL,
			 SeLocalSystemSid);

  RtlAddAccessAllowedAce(SePublicOpenDacl,
			 ACL_REVISION,
			 GENERIC_ALL,
			 SeAliasAdminsSid);

  /* create PublicOpenUnrestrictedDacl */
  AclLength = sizeof(ACL) +
	      (sizeof(ACE) + RtlLengthSid(SeWorldSid)) +
	      (sizeof(ACE) + RtlLengthSid(SeLocalSystemSid)) +
	      (sizeof(ACE) + RtlLengthSid(SeAliasAdminsSid)) +
	      (sizeof(ACE) + RtlLengthSid(SeRestrictedCodeSid));

  SePublicOpenUnrestrictedDacl = ExAllocatePoolWithTag(NonPagedPool,
						       AclLength,
						       TAG_ACL);
  if (SePublicOpenUnrestrictedDacl == NULL)
    return FALSE;

  RtlCreateAcl(SePublicOpenUnrestrictedDacl,
	       AclLength,
	       ACL_REVISION);

  RtlAddAccessAllowedAce(SePublicOpenUnrestrictedDacl,
			 ACL_REVISION,
			 GENERIC_ALL,
			 SeWorldSid);

  RtlAddAccessAllowedAce(SePublicOpenUnrestrictedDacl,
			 ACL_REVISION,
			 GENERIC_ALL,
			 SeLocalSystemSid);

  RtlAddAccessAllowedAce(SePublicOpenUnrestrictedDacl,
			 ACL_REVISION,
			 GENERIC_ALL,
			 SeAliasAdminsSid);

  RtlAddAccessAllowedAce(SePublicOpenUnrestrictedDacl,
			 ACL_REVISION,
			 GENERIC_READ | GENERIC_EXECUTE,
			 SeRestrictedCodeSid);

  /* create SystemDefaultDacl */
  AclLength = sizeof(ACL) +
	      (sizeof(ACE) + RtlLengthSid(SeLocalSystemSid)) +
	      (sizeof(ACE) + RtlLengthSid(SeAliasAdminsSid));

  SeSystemDefaultDacl = ExAllocatePoolWithTag(NonPagedPool,
					      AclLength,
					      TAG_ACL);
  if (SeSystemDefaultDacl == NULL)
    return FALSE;

  RtlCreateAcl(SeSystemDefaultDacl,
	       AclLength,
	       ACL_REVISION);

  RtlAddAccessAllowedAce(SeSystemDefaultDacl,
			 ACL_REVISION,
			 GENERIC_ALL,
			 SeLocalSystemSid);

  RtlAddAccessAllowedAce(SeSystemDefaultDacl,
			 ACL_REVISION,
			 GENERIC_READ | GENERIC_EXECUTE | READ_CONTROL,
			 SeAliasAdminsSid);

  /* create UnrestrictedDacl */
  AclLength = sizeof(ACL) +
	      (sizeof(ACE) + RtlLengthSid(SeWorldSid)) +
	      (sizeof(ACE) + RtlLengthSid(SeRestrictedCodeSid));

  SeUnrestrictedDacl = ExAllocatePoolWithTag(NonPagedPool,
					     AclLength,
					     TAG_ACL);
  if (SeUnrestrictedDacl == NULL)
    return FALSE;

  RtlCreateAcl(SeUnrestrictedDacl,
	       AclLength,
	       ACL_REVISION);

  RtlAddAccessAllowedAce(SeUnrestrictedDacl,
			 ACL_REVISION,
			 GENERIC_ALL,
			 SeWorldSid);

  RtlAddAccessAllowedAce(SeUnrestrictedDacl,
			 ACL_REVISION,
			 GENERIC_READ | GENERIC_EXECUTE,
			 SeRestrictedCodeSid);

  return(TRUE);
}

/* EOF */
