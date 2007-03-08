/* $Id$
 *
 * Convert NTSTATUS codes to Win32 error codes: run it
 * on a NT box AND on a ROS box, then diff the results.
 *
 * This utility should help keeping correct how Ros
 * translates executive's errors codes into Win32 error
 * codes.
 *
 * Usage: nts2w32err [MaxStatusCode] > log.txt
 *
 * 2004-01-10 Emanuele Aliberti
 *
 */
#define WIN32_NO_STATUS
#include <windows.h>
#include <stdlib.h>
#include <ntndk.h>
#include <stdio.h>

int main (int argc, char * argv [])
{
	NTSTATUS Severity = 0;
	NTSTATUS StatusCode = STATUS_SUCCESS;
	NTSTATUS Status = STATUS_SUCCESS;
	DWORD    LastError = ERROR_SUCCESS;
	DWORD    Maximum = 0x40000;

	if (2 == argc)
	{
		sscanf (argv[1], "%lx", & Maximum);
	}

	printf ("NT error codes 0x0-0x%lx that get translated *not* to ERROR_MR_MID_NOT_FOUND (317)\n\n", Maximum);

	for (	Severity = 0;
		Severity < 4;
		Severity ++)
	{
		printf ("--- Severity %ld ---\n", Severity);

		for (	StatusCode = STATUS_SUCCESS;
			StatusCode <= Maximum ;
			StatusCode ++)
		{
			Status = ((Severity << 30) | StatusCode);
			LastError = RtlNtStatusToDosError (Status);
			if (ERROR_MR_MID_NOT_FOUND != LastError)
			{
				printf ("0x%08lx => %ldL\n", Status, LastError);
			}
		}
	}
	return EXIT_SUCCESS;
}
/* EOF */
