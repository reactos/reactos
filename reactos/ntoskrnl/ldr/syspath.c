/* $Id: syspath.c,v 1.3 1999/10/20 23:09:26 rex Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/syspath.c
 * PURPOSE:         Get system path
 * PROGRAMMERS:     EA
 * UPDATE HISTORY:
 *	EA   19990717  GetSystemDirectory()
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <wchar.h>
#include <string.h>


/**********************************************************************
 * NAME
 * 	GetSystemDirectory
 *
 * DESCRIPTION
 * 	Get the ReactOS system directory from the registry.
 * 	(since registry does not work yet, fill the buffer
 * 	with a literal which will remain as the default).
 *
 * RETURN VALUE
 *	NULL on error; otherwise SystemDirectoryName.
 *
 * REVISIONS
 * 	19990717 (EA)
 */
LPWSTR
LdrGetSystemDirectory (
	LPWSTR	SystemDirectoryName,
	DWORD	Size
	)
{
	LPWSTR	DosDev = L"\\??\\";
	LPWSTR	SDName = L"C:\\reactos\\system32";
	
	if (	(NULL == SystemDirectoryName)
		||	(
				(
					(wcslen(DosDev)
					+ wcslen(SDName)
					+ 1
					)
				* sizeof (WCHAR)
				)
				> Size
			)
		)
	{
		DbgPrint("LdrGetSystemDirectory() failed\n");
		return NULL;
	}
	/*
	 * Prefix with the dos devices aliases
	 * directory, since the system directory
	 * is always given as a dos name by users
	 * (in the registry/by the boot loader?).
	 */
	wcscpy( 
		SystemDirectoryName,
		L"\\??\\"
		);
	if (FALSE)
	{
		/*
		 * FIXME: get the system directory
		 * from the registry.
		 */
	}
	else
	{
		/* Default value */
		wcscat(
			SystemDirectoryName,
			SDName
			);
	}
	
	return SystemDirectoryName;
}


/* EOF */
