/* $Id: psmgr.c,v 1.15 2003/07/11 01:23:15 royce Exp $
 *
 * COPYRIGHT:               See COPYING in the top level directory
 * PROJECT:                 ReactOS kernel
 * FILE:                    ntoskrnl/ps/psmgr.c
 * PURPOSE:                 Process management
 * PROGRAMMER:              David Welch (welch@mcmail.com)
 */

/* INCLUDES **************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/ps.h>
#include <reactos/version.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

VOID PiShutdownProcessManager(VOID)
{
   DPRINT("PiShutdownMemoryManager()\n");
   
   PiKillMostProcesses();
}

VOID PiInitProcessManager(VOID)
{
   PsInitProcessManagment();
   PsInitThreadManagment();
   PsInitIdleThread();
   PiInitApcManagement();
   PsInitialiseSuspendImplementation();
}


/**********************************************************************
 * NAME							EXPORTED
 *	PsGetVersion
 *
 * DESCRIPTION
 *	Retrieves the current OS version.
 *
 * ARGUMENTS
 *	MajorVersion	Pointer to a variable that will be set to the
 *			major version of the OS. Can be NULL.
 *
 *	MinorVersion	Pointer to a variable that will be set to the
 *			minor version of the OS. Can be NULL.
 *
 *	BuildNumber	Pointer to a variable that will be set to the
 *			build number of the OS. Can be NULL.
 *
 *	CSDVersion	Pointer to a variable that will be set to the
 *			CSD string of the OS. Can be NULL.
 *
 * RETURN VALUE
 *	TRUE	OS is a checked build.
 *	FALSE	OS is a free build.
 *
 * NOTES
 *	The DDK docs say something about a 'CmCSDVersionString'.
 *	How do we determine in the build is checked or free??
 *
 * @unimplemented
 */

BOOLEAN
STDCALL
PsGetVersion (
	PULONG		MajorVersion	OPTIONAL,
	PULONG		MinorVersion	OPTIONAL,
	PULONG		BuildNumber	OPTIONAL,
	PUNICODE_STRING	CSDVersion	OPTIONAL
	)
{
	if (MajorVersion)
		*MajorVersion = KERNEL_VERSION_MAJOR;

	if (MinorVersion)
		*MinorVersion = KERNEL_VERSION_MINOR;

	if (BuildNumber)
		*BuildNumber = NtBuildNumber;

	if (CSDVersion)
	{
		CSDVersion->Length = 0;
		CSDVersion->MaximumLength = 0;
		CSDVersion->Buffer = NULL;
#if 0
		CSDVersion->Length = CmCSDVersionString.Length;
		CSDVersion->MaximumLength = CmCSDVersionString.Maximum;
		CSDVersion->Buffer = CmCSDVersionString.Buffer;
#endif
	}

	/* FIXME: How do we determine if build is checked or free? */
	return FALSE;
}

/* EOF */
