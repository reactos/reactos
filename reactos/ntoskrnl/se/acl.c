/* $Id: acl.c,v 1.19 2004/08/05 18:17:37 ion Exp $
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

#include <ddk/ntddk.h>
#include <internal/se.h>

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
  ULONG AclLength2;
  ULONG AclLength3;
  ULONG AclLength4;

  AclLength2 = sizeof(ACL) +
	       2 * (RtlLengthRequiredSid(1) + sizeof(ACE));
  AclLength3 = sizeof(ACL) +
	       3 * (RtlLengthRequiredSid(1) + sizeof(ACE));
  AclLength4 = sizeof(ACL) +
	       4 * (RtlLengthRequiredSid(1) + sizeof(ACE));

  /* create PublicDefaultDacl */
  SePublicDefaultDacl = ExAllocatePoolWithTag(NonPagedPool,
					      AclLength2,
					      TAG_ACL);
  if (SePublicDefaultDacl == NULL)
    return FALSE;

  RtlCreateAcl(SePublicDefaultDacl,
	       AclLength2,
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
  SePublicDefaultUnrestrictedDacl = ExAllocatePoolWithTag(NonPagedPool,
							  AclLength4,
							  TAG_ACL);
  if (SePublicDefaultUnrestrictedDacl == NULL)
    return FALSE;

  RtlCreateAcl(SePublicDefaultUnrestrictedDacl,
	       AclLength4,
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
  SePublicOpenDacl = ExAllocatePoolWithTag(NonPagedPool,
					   AclLength3,
					   TAG_ACL);
  if (SePublicOpenDacl == NULL)
    return FALSE;

  RtlCreateAcl(SePublicOpenDacl,
	       AclLength3,
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
  SePublicOpenUnrestrictedDacl = ExAllocatePoolWithTag(NonPagedPool,
						       AclLength4,
						       TAG_ACL);
  if (SePublicOpenUnrestrictedDacl == NULL)
    return FALSE;

  RtlCreateAcl(SePublicOpenUnrestrictedDacl,
	       AclLength4,
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
  SeSystemDefaultDacl = ExAllocatePoolWithTag(NonPagedPool,
					      AclLength2,
					      TAG_ACL);
  if (SeSystemDefaultDacl == NULL)
    return FALSE;

  RtlCreateAcl(SeSystemDefaultDacl,
	       AclLength2,
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
  SeUnrestrictedDacl = ExAllocatePoolWithTag(NonPagedPool,
					     AclLength2,
					     TAG_ACL);
  if (SeUnrestrictedDacl == NULL)
    return FALSE;

  RtlCreateAcl(SeUnrestrictedDacl,
	       AclLength2,
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
