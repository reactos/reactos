/* $Id: buildno.c,v 1.1 1999/11/07 08:03:21 ea Exp $
 *
 * buildno - Generate the build number for ReactOS
 *
 * Copyright (c) 1999 Emanuele Aliberti
 *
 *
 * It assumes the last release date is defined in
 * <reactos/version.h> as a macro named
 *
 * KERNEL_RELEASE_DATE
 *
 * as a 32-bit unsigned long YYYYDDMM (UTC).
 *
 * The build number is the number of full days
 * elapsed since the last release date (UTC).
 *
 * The build number is stored in the file
 * <reactos/buildno.h> as a set of macros:
 *
 * KERNEL_VERSION_BUILD
 * KERNEL_VERSION_BUILD_STR
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <reactos/version.h>

#define FALSE 0
#define TRUE  1

/* File to (over)write */
#define BUILDNO_INCLUDE_FILE "../../include/reactos/buildno.h"

static char * argv0 = "";


int
elapsed_days (
	time_t	t_today,
	time_t	t_release_day
	)
{
	double	seconds = difftime (t_today, t_release_day);
	double	days = seconds / (double) 86400.0;
	char	buf [32];
	char	* dot = buf;

	sprintf (buf, "%f", days );

	while ( *dot && *dot != '.') ++dot;
	*dot = '\0';
	
	return atol (buf);
}

void
write_h (int build)
{
	FILE	*h = NULL;

	h = fopen ( BUILDNO_INCLUDE_FILE, "w");
	if (!h) 
	{
		fprintf (
			stderr,
			"%s: can not create file \"%s\"!\n",
			argv0,
			BUILDNO_INCLUDE_FILE
			);
		return;
	}
	fprintf (
		h,
		"/* Do not edit - Machine generated */\n"
		);
	
	fprintf (h, "#ifndef _INC_REACTOS_BUILDNO\n" );
	fprintf (h, "#define _INC_REACTOS_BUILDNO\n" );

	fprintf (
		h,
		"#define KERNEL_VERSION_BUILD\t%d\n",
		build
		);
	fprintf (
		h,
		"#define KERNEL_VERSION_BUILD_STR\t\"%d\"\n",
		build
		);
	fprintf (
		h,
		"#define KERNEL_RELEASE_RC\t\"%d.%d.%d.%d\\0\"\n",
		KERNEL_VERSION_MAJOR,
		KERNEL_VERSION_MINOR,
		KERNEL_VERSION_PATCH_LEVEL,
		build
		);
	fprintf (
		h,
		"#define KERNEL_RELEASE_STR\t\"%d.%d.%d.%d\"\n",
		KERNEL_VERSION_MAJOR,
		KERNEL_VERSION_MINOR,
		KERNEL_VERSION_PATCH_LEVEL,
		build
		);
	fprintf (
		h,
		"#define KERNEL_VERSION_RC\t\"%d.%d.%d\\0\"\n",
		KERNEL_VERSION_MAJOR,
		KERNEL_VERSION_MINOR,
		KERNEL_VERSION_PATCH_LEVEL
		);
	fprintf (
		h,
		"#define KERNEL_VERSION_STR\t\"%d.%d.%d\"\n", 
		KERNEL_VERSION_MAJOR,
		KERNEL_VERSION_MINOR,
		KERNEL_VERSION_PATCH_LEVEL
		);
	fprintf (
		h,
		"#endif\n/* EOF */\n"
		);
	
	fclose (h);
}

void
usage (void)
{
	fprintf (
		stderr,
		"Usage: %s [-q]\n",
		argv0
		);
	exit (EXIT_SUCCESS);
}


int
main (int argc, char * argv [])
{
	int		quiet = FALSE;

	int		year = 0;
	int		month = 0;
	int		day = 0;
	int		build = 0;

	time_t		t0 = 0;
	struct tm	t0_tm = {0};
	time_t		t1 = 0;
	struct tm	* t1_tm = NULL;

	argv0 = argv[0];
		
	switch (argc)
	{
		case 1:
			break;
		case 2:
			if (argv[1][0] == '-')
			{
				if (argv[1][1] == 'q')
				{
					quiet = TRUE;
				}
				else
				{
					usage ();
				}
			}
			else
			{
				usage ();
			}
			break;
		default:
			usage ();
	}
	/*
	 * We are building TODAY!
	 */
	time (& t0);
	/*
	 * "Parse" the release date.
	 */
	day = KERNEL_RELEASE_DATE % 100;
	month = (	(	KERNEL_RELEASE_DATE
				% 10000
				)
				- day
			)
			/ 100;
	year =
		(	KERNEL_RELEASE_DATE
			- (month * 100)
			- day
			)
			/ 10000;
	if (FALSE == quiet)
	{
		printf ( "\n\
ReactOS Build Number Generator\n\n\
Last release: %4d-%02d-%02d\n",
			year,
			month,
			day
			);
	}
	t0_tm.tm_year = year - ((year > 1999) ? 2000 : 1900);
	t0_tm.tm_mon = month;
	t0_tm.tm_mday = day;
	
	t0 = mktime (& t0_tm);

	time (& t1); /* current build time */
	t1_tm = gmtime (& t1);

	t1_tm->tm_year +=
		(t1_tm->tm_year < 70)
		? 2000
		: 1900;
	if (FALSE == quiet)
	{
		printf ( 
			"Current date: %4d-%02d-%02d\n\n",
			t1_tm->tm_year,
			t1_tm->tm_mon,
			t1_tm->tm_mday
			);
	}
	/*
	 * Compute delta days.
	 */
	build =	elapsed_days (t1, t0);

	if (FALSE == quiet)
	{
		printf (
			"Build number: %d (elapsed days since last release)\n",
			build
			);
		printf (
			"ROS Version : %d.%d.%d.%d\n",
			KERNEL_VERSION_MAJOR,
			KERNEL_VERSION_MINOR,
			KERNEL_VERSION_PATCH_LEVEL,
			build
			);
	}
	/*
	 * (Over)write the include file.
	 */
	write_h (build);

	return EXIT_SUCCESS;
}


/* EOF */
