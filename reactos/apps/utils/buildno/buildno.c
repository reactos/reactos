/* $Id: buildno.c,v 1.3 2000/01/22 14:25:48 ea Exp $
 *
 * buildno - Generate the build number for ReactOS
 *
 * Copyright (c) 1999,2000 Emanuele Aliberti
 *
 * License: GNU GPL
 *
 * It assumes the last release date is defined in
 * <reactos/version.h> as a macro named
 *
 * KERNEL_RELEASE_DATE
 *
 * as a 32-bit unsigned long YYYYMMDD (UTC;
 * MM=01-12; DD=01-31).
 *
 * The build number is the number of full days
 * elapsed since the last release date (UTC).
 *
 * The build number is stored in the file
 * <reactos/buildno.h> as a set of macros:
 *
 * KERNEL_VERSION_BUILD		base 10 number
 * KERNEL_VERSION_BUILD_STR	C string
 * KERNEL_VERSION_BUILD_RC	RC string
 *
 * REVISIONS
 * ---------
 * 2000-01-22 (ea)
 * 	Fixed bugs: tm_year is (current_year - 1900),
 * 	tm_month is 0-11 not 1-12 and code ignored TZ.
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

#ifdef DBG
void
tm_dump (const char *tag, struct tm * t)
{
	printf ("%s->tm_sec   = %d\n", tag, t->tm_sec);
	printf ("%s->tm_min   = %d\n", tag, t->tm_min);
	printf ("%s->tm_hour  = %d\n", tag, t->tm_hour);
	printf ("%s->tm_mday  = %d\n", tag, t->tm_mday);
	printf ("%s->tm_mon   = %d\n", tag, t->tm_mon);
	printf ("%s->tm_year  = %d\n", tag, t->tm_year);
	printf ("%s->tm_wday  = %d\n", tag, t->tm_wday);
	printf ("%s->tm_yday  = %d\n", tag, t->tm_yday);
	printf ("%s->tm_isdst = %d\n\n", tag, t->tm_isdst);
}
#endif


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
		"Usage: %s [-q]\n\n  -q  quiet mode\n",
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
	 * Set TZ information.
	 */
	tzset ();
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
#ifdef DBG
	tm_dump ("t0", & t0_tm);
#endif
	t0_tm.tm_year = (year - 1900);
	t0_tm.tm_mon = --month; /* 0-11 */
	t0_tm.tm_mday = day;
	t0_tm.tm_hour = 0;
	t0_tm.tm_min = 0;
	t0_tm.tm_sec = 1;
	t0_tm.tm_isdst = -1;
	
#ifdef DBG
	tm_dump ("t0", & t0_tm);
#endif

	if (-1 == (t0 = mktime (& t0_tm)))
	{
		fprintf (
			stderr,
			"%s: can not convert release date!\n",
			argv[0]
			);
		return EXIT_FAILURE;
	}

	time (& t1); /* current build time */
	t1_tm = gmtime (& t1);

#ifdef DBG
	tm_dump ("t1", t1_tm);
#endif
	t1_tm->tm_year +=
		(t1_tm->tm_year < 70)
		? 2000
		: 1900;
#ifdef DBG
	tm_dump ("t1", t1_tm);
#endif
	if (FALSE == quiet)
	{
		printf ( 
			"Current date: %4d-%02d-%02d\n\n",
			t1_tm->tm_year,
			(t1_tm->tm_mon + 1),
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
