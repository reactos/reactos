/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/sec.c
 * PURPOSE:         Registry functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */
DWORD
STDCALL
GetSecurityDescriptorLength (
			     PSECURITY_DESCRIPTOR pSecurityDescriptor
			      )
{
	return RtlSetSecurityDescriptorLength(pSecurityDescriptor);
}
