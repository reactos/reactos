/* $Id$
 *
 * buildno - Generate the build number for ReactOS
 *
 * Copyright (c) 1999,2000 Emanuele Aliberti
 *
 * The build number is the day on which the build took
 * place, as YYYYMMDD
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
 * 2000-12-10 (ea)
 * 	Added -p option to make it simply print the
 * 	version number, but skip buildno.h generation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "version.h"
#include "XML.h"
#include "exception.h"

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

void
write_h (int build, char *buildstr)
{
  FILE	*h = NULL;
  char* s;
  char* s1;
  unsigned length;
  int dllversion = KERNEL_VERSION_MAJOR + 42;

  s1 = s = (char *) malloc(256 * 1024);
  
  s = s + sprintf (s, "/* Do not edit - Machine generated */\n");
	
  s = s + sprintf (s, "#ifndef _INC_REACTOS_BUILDNO\n" );
  s = s + sprintf (s, "#define _INC_REACTOS_BUILDNO\n" );
  
  s = s + sprintf (s, "#define KERNEL_VERSION_BUILD\t%d\n", build);
  s = s + sprintf (s, "#define KERNEL_VERSION_BUILD_STR\t\"%s\"\n", buildstr);
  s = s + sprintf (s, "#define KERNEL_VERSION_BUILD_RC\t\"%s\\0\"\n", buildstr);
  s = s + sprintf (s, "#define KERNEL_RELEASE_RC\t\"%d.%d",
		   KERNEL_VERSION_MAJOR, KERNEL_VERSION_MINOR);
  if (0 != KERNEL_VERSION_PATCH_LEVEL)
    {
      s = s + sprintf (s, ".%d", KERNEL_VERSION_PATCH_LEVEL);
    }
  s = s + sprintf (s, "-%S\\0\"\n", KERNEL_VERSION_BUILD_TYPE);
  s = s + sprintf (s, "#define KERNEL_RELEASE_STR\t\"%d.%d",
		   KERNEL_VERSION_MAJOR,
		   KERNEL_VERSION_MINOR);
  if (0 != KERNEL_VERSION_PATCH_LEVEL)
    {
      s = s + sprintf (s, ".%d", KERNEL_VERSION_PATCH_LEVEL);
    }
  s = s + sprintf (s, "-%S\"\n", KERNEL_VERSION_BUILD_TYPE);
  s = s + sprintf (s, "#define KERNEL_VERSION_RC\t\"%d.%d",
		   KERNEL_VERSION_MAJOR,
		   KERNEL_VERSION_MINOR);
  if (0 != KERNEL_VERSION_PATCH_LEVEL)
    {
      s = s + sprintf (s, ".%d", KERNEL_VERSION_PATCH_LEVEL);
    }
  s = s + sprintf (s, "-%S\\0\"\n", KERNEL_VERSION_BUILD_TYPE);
  s = s + sprintf (s, "#define KERNEL_VERSION_STR\t\"%d.%d", 
		   KERNEL_VERSION_MAJOR,
		   KERNEL_VERSION_MINOR);
  if (0 != KERNEL_VERSION_PATCH_LEVEL)
    {
      s = s + sprintf (s, ".%d", KERNEL_VERSION_PATCH_LEVEL);
    }
  s = s + sprintf (s, "-%S\"\n", KERNEL_VERSION_BUILD_TYPE);
  s = s + sprintf (s, "#define REACTOS_DLL_VERSION_MAJOR\t%d\n", dllversion);
  s = s + sprintf (s, "#define REACTOS_DLL_RELEASE_RC\t\"%d.%d",
		   dllversion, KERNEL_VERSION_MINOR);
  if (0 != KERNEL_VERSION_PATCH_LEVEL)
    {
      s = s + sprintf (s, ".%d", KERNEL_VERSION_PATCH_LEVEL);
    }
  s = s + sprintf (s, "-%S\\0\"\n", KERNEL_VERSION_BUILD_TYPE);
  s = s + sprintf (s, "#define REACTOS_DLL_RELEASE_STR\t\"%d.%d",
		   dllversion,
		   KERNEL_VERSION_MINOR);
  if (0 != KERNEL_VERSION_PATCH_LEVEL)
    {
      s = s + sprintf (s, ".%d", KERNEL_VERSION_PATCH_LEVEL);
    }
  s = s + sprintf (s, "-%S\"\n", KERNEL_VERSION_BUILD_TYPE);
  s = s + sprintf (s, "#define REACTOS_DLL_VERSION_RC\t\"%d.%d",
		   dllversion,
		   KERNEL_VERSION_MINOR);
  if (0 != KERNEL_VERSION_PATCH_LEVEL)
    {
      s = s + sprintf (s, ".%d", KERNEL_VERSION_PATCH_LEVEL);
    }
  s = s + sprintf (s, "-%S\\0\"\n", KERNEL_VERSION_BUILD_TYPE);
  s = s + sprintf (s, "#define REACTOS_DLL_VERSION_STR\t\"%d.%d", 
		   dllversion,
		   KERNEL_VERSION_MINOR);
  if (0 != KERNEL_VERSION_PATCH_LEVEL)
    {
      s = s + sprintf (s, ".%d", KERNEL_VERSION_PATCH_LEVEL);
    }
  s = s + sprintf (s, "-%S\"\n", KERNEL_VERSION_BUILD_TYPE);
  s = s + sprintf (s, "#endif\n/* EOF */\n");

  h = fopen (BUILDNO_INCLUDE_FILE, "rb");
  if (h != NULL)
    {
      fseek(h, 0, SEEK_END);
      length = ftell(h);
      if (length == strlen(s1))
	{
	  char* orig;
	  
	  orig = (char *) malloc(length);
	  fseek(h, 0, SEEK_SET);
	  fread(orig, 1, length, h);
	  if (memcmp(s1, orig, length) == 0)
	    {
	      fclose(h);
	      return;
	    }
	}
      fclose(h);
    }

  h = fopen (BUILDNO_INCLUDE_FILE, "wb");
  if (!h) 
    {
      fprintf (stderr,
	       "%s: can not create file \"%s\"!\n",
	       argv0,
	       BUILDNO_INCLUDE_FILE);
      return;
    }
  fwrite(s1, 1, strlen(s1), h);
  fclose (h);
}

char *
GetRev(void)
{
  static char Unknown[] = "UNKNOWN";
  static char Revision[10]; /* 999999999 revisions should be enough for everyone... */

  try
    {
      Path path;
      XMLElement *head;

      try
        {
          head = XMLLoadFile(".svn/entries", path);
        }
      catch(FileNotFoundException)
        {
          head = XMLLoadFile("_svn/entries", path);
        }
      XMLElement *entries = head->subElements[0];
      for (size_t i = 0; i < entries->subElements.size(); i++)
	{
          XMLElement *entry = entries->subElements[i];
          if ("entry" == entry->name)
            {
              bool GotName = false;
              bool GotKind = false;
              bool GotRevision = false;
              for (size_t j = 0; j < entry->attributes.size(); j++)
                {
                  XMLAttribute *Attribute = entry->attributes[j];
                  if ("name" == Attribute->name && "" == Attribute->value)
                    {
                      GotName = true;
                    }
                  if ("kind" == Attribute->name && "dir" == Attribute->value)
                    {
                      GotKind = true;
                    }
                  if ("revision" == Attribute->name)
                    {
                      if (sizeof(Revision) <= Attribute->value.length() + 1)
                        {
                          strcpy(Revision, "revtoobig");
                        }
                      else
                        {
                          strcpy(Revision, Attribute->value.c_str());
                        }
                      GotRevision = true;
                    }
                  if (GotName && GotKind && GotRevision)
                    {
                      delete head;
                      return Revision;
                    }
                }
            }
        }

      delete head;
    }
  catch(...)
    {
      ;
    }

  return Unknown;
}


void
usage (void)
{
	fprintf (
		stderr,
		"Usage: %s [-{p|q}]\n\n  -p  print version number and exit\n  -q  run in quiet mode\n",
		argv0
		);
	exit (EXIT_SUCCESS);
}


int
main (int argc, char * argv [])
{
	int		print_only = FALSE;
	int		quiet = FALSE;

	int		build = 0;
	char		buildstr[64];

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
				else  if (argv[1][1] == 'p')
				{
					print_only = TRUE;
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
	if (! quiet)
	{
		printf ( "\nReactOS Build Number Generator\n\n");
	}

	time (& t1); /* current build time */
	t1_tm = gmtime (& t1);

#ifdef DBG
	tm_dump ("t1", t1_tm);
#endif
	t1_tm->tm_year += 1900;
#ifdef DBG
	tm_dump ("t1", t1_tm);
#endif
	if (! quiet)
	{
		printf ( 
			"Current date: %4d-%02d-%02d\n\n",
			t1_tm->tm_year,
			(t1_tm->tm_mon + 1),
			t1_tm->tm_mday
			);
	}
	/*
	 * Compute build number.
	 */
	build =	t1_tm->tm_year * 10000 + (t1_tm->tm_mon + 1) * 100 + t1_tm->tm_mday;

	sprintf(buildstr, "%d-r%s", build, GetRev());

	if (! quiet)
	{
		printf (
			"ROS Version : %d.%d",
			KERNEL_VERSION_MAJOR,
			KERNEL_VERSION_MINOR
			);
		if (0 != KERNEL_VERSION_PATCH_LEVEL)
		{
			printf(".%d", KERNEL_VERSION_PATCH_LEVEL);
		}
		printf("-%S (Build %s)\n\n", KERNEL_VERSION_BUILD_TYPE, buildstr);
	}
	/*
	 * (Over)write the include file, unless
	 * the user switched on -p.
	 */
	if (! print_only)
	{
		write_h (build, buildstr);
	}
	else
	{
		printf ("%s: no code generated", argv [0]);
	}

	return EXIT_SUCCESS;
}


/* EOF */
