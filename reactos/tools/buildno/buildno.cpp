/*
 * buildno - Generate the build number for ReactOS
 *
 * Copyright (c) 1999,2000 Emanuele Aliberti
 * Copyright (c) 2006 Christoph von Wittich
 * Copyright (c) 2008 Hervé Poussineau
 *
 * The build number is the day on which the build took
 * place, as YYYYMMDD
 *
 * The build number is stored in the output file as a set of macros
 *
 * REVISIONS
 * ---------
 * 2008-01-12 (hpoussin)
 *  Add -t option to change the build tag
 * 2006-09-09 (cwittich)
 *  Read binary entries files from SVN 1.4.x
 * 2000-01-22 (ea)
 *  Fixed bugs: tm_year is (current_year - 1900),
 *  tm_month is 0-11 not 1-12 and code ignored TZ.
 * 2000-12-10 (ea)
 *  Added -p option to make it simply print the
 *  version number, but skip buildno.h generation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "version.h"
#include "xml.h"

#define FALSE 0
#define TRUE  1

static const char * argv0 = "";
static char * filename = NULL;
static char * build_tag = NULL;

int count_wide_string( const wchar_t *str )
{
	int i;
	for( i = 0; str[i]; i++ )
		;
	return i;
}

long
GetRev(char *Revision, size_t length)
{
	long revno = 0;
	char *p;

	FILE *fp = NULL;
	fp = fopen(".svn/entries", "r");
	if (fp != NULL)
	{
		if (fgets(Revision, length, fp) != NULL)
		{
			/* If the first character of the file is not a digit,
			   then it is probably in XML format. */
			if (isdigit(Revision[0]))
			{
				while (fgets(Revision, length, fp) != NULL)
				{
					revno = strtol(Revision, &p, 10);
					if (revno != 0)
					{
						*p = '\0';
						fclose(fp);
						return revno;
					}
				}
			}
		}
		fclose(fp);
	}

	try
	{
		XMLElement *head;

		try
		{
			head = XMLLoadFile(".svn/entries");
		}
		catch(XMLFileNotFoundException)
		{
			head = XMLLoadFile("_svn/entries");
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
						if (length <= Attribute->value.length() + 1)
						{
							strcpy(Revision, "revtoobig");
						}
						else
						{
							strcpy(Revision, Attribute->value.c_str());
							revno = strtol(Revision, NULL, 10);
						}
						GotRevision = true;
					}
					if (GotName && GotKind && GotRevision)
					{
						delete head;
						return revno;
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

	strcpy(Revision, "UNKNOWN");
	return revno;
}

void
write_h (int build, char *buildstr, long revno)
{
	FILE	*h = NULL;
	char* s;
	char* s1;
	unsigned int length;
	int dllversion = KERNEL_VERSION_MAJOR + 42;

	s1 = s = (char *) malloc(256 * 1024);

	s = s + sprintf (s, "/* Do not edit - Machine generated */\n");

	s = s + sprintf (s, "#ifndef _INC_REACTOS_BUILDNO\n" );
	s = s + sprintf (s, "#define _INC_REACTOS_BUILDNO\n" );

	s = s + sprintf (s, "#define KERNEL_VERSION_BUILD\t%d\n", build);
	s = s + sprintf (s, "#define KERNEL_VERSION_BUILD_HEX\t0x%lx\n", revno);
	s = s + sprintf (s, "#define KERNEL_VERSION_BUILD_STR\t\"%s\"\n", buildstr);
	s = s + sprintf (s, "#define KERNEL_VERSION_BUILD_RC\t\"%s\\0\"\n", buildstr);
	s = s + sprintf (s, "#define KERNEL_RELEASE_RC\t\"%d.%d",
	                KERNEL_VERSION_MAJOR, KERNEL_VERSION_MINOR);
	if (0 != KERNEL_VERSION_PATCH_LEVEL)
	{
		s = s + sprintf (s, ".%d", KERNEL_VERSION_PATCH_LEVEL);
	}
	s = s + sprintf (s, "%s\\0\"\n", build_tag);
	s = s + sprintf (s, "#define KERNEL_RELEASE_STR\t\"%d.%d",
	                 KERNEL_VERSION_MAJOR,
	                 KERNEL_VERSION_MINOR);
	if (0 != KERNEL_VERSION_PATCH_LEVEL)
	{
		s = s + sprintf (s, ".%d", KERNEL_VERSION_PATCH_LEVEL);
	}
	s = s + sprintf (s, "%s\"\n", build_tag);
	s = s + sprintf (s, "#define KERNEL_VERSION_RC\t\"%d.%d",
	                 KERNEL_VERSION_MAJOR,
	                 KERNEL_VERSION_MINOR);
	if (0 != KERNEL_VERSION_PATCH_LEVEL)
	{
		s = s + sprintf (s, ".%d", KERNEL_VERSION_PATCH_LEVEL);
	}
	s = s + sprintf (s, "%s\\0\"\n", build_tag);
	s = s + sprintf (s, "#define KERNEL_VERSION_STR\t\"%d.%d",
	                 KERNEL_VERSION_MAJOR,
	                 KERNEL_VERSION_MINOR);
	if (0 != KERNEL_VERSION_PATCH_LEVEL)
	{
		s = s + sprintf (s, ".%d", KERNEL_VERSION_PATCH_LEVEL);
	}
	s = s + sprintf (s, "%s\"\n", build_tag);
	s = s + sprintf (s, "#define REACTOS_DLL_VERSION_MAJOR\t%d\n", dllversion);
	s = s + sprintf (s, "#define REACTOS_DLL_RELEASE_RC\t\"%d.%d",
	                 dllversion, KERNEL_VERSION_MINOR);
	if (0 != KERNEL_VERSION_PATCH_LEVEL)
	{
		s = s + sprintf (s, ".%d", KERNEL_VERSION_PATCH_LEVEL);
	}
	s = s + sprintf (s, "%s\\0\"\n", build_tag);
	s = s + sprintf (s, "#define REACTOS_DLL_RELEASE_STR\t\"%d.%d",
	                 dllversion,
	                 KERNEL_VERSION_MINOR);
	if (0 != KERNEL_VERSION_PATCH_LEVEL)
	{
		s = s + sprintf (s, ".%d", KERNEL_VERSION_PATCH_LEVEL);
	}
	s = s + sprintf (s, "%s\"\n", build_tag);
	s = s + sprintf (s, "#define REACTOS_DLL_VERSION_RC\t\"%d.%d",
	                 dllversion,
	                 KERNEL_VERSION_MINOR);
	if (0 != KERNEL_VERSION_PATCH_LEVEL)
	{
		s = s + sprintf (s, ".%d", KERNEL_VERSION_PATCH_LEVEL);
	}
	s = s + sprintf (s, "%s\\0\"\n", build_tag);
	s = s + sprintf (s, "#define REACTOS_DLL_VERSION_STR\t\"%d.%d",
	                 dllversion,
	                 KERNEL_VERSION_MINOR);
	if (0 != KERNEL_VERSION_PATCH_LEVEL)
	{
		s = s + sprintf (s, ".%d", KERNEL_VERSION_PATCH_LEVEL);
	}
	s = s + sprintf (s, "%s\"\n", build_tag);
	s = s + sprintf (s, "#endif\n/* EOF */\n");

	h = fopen (filename, "rb");
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
				free(s1);
				return;
			}
		}
		fclose(h);
	}

	h = fopen (filename, "wb");
	if (!h)
	{
		fprintf (stderr,
		         "%s: can not create file \"%s\"!\n",
		         argv0,
		         filename);
		free(s1);
		return;
	}
	fwrite(s1, 1, strlen(s1), h);
	fclose (h);
}

void
usage (void)
{
	fprintf (
		stderr,
		"Usage: %s [-{p|q}] [-t tag] path-to-header\n\n"
		"  -p  print version number and exit\n"
		"  -q  run in quiet mode\n"
		"  -t  specify a build tag\n",
		argv0);
	exit (EXIT_SUCCESS);
}


int
main (int argc, char * argv [])
{
	int i, length;
	int print_only = FALSE;
	int quiet = FALSE;

	int build = 0;
	long revno;
	char buildstr[64], revision[10];

	time_t t1 = 0;
	struct tm * t1_tm = NULL;

	argv0 = argv[0];

	/* Check arguments */
	for (i = 1; i < argc; i++)
	{
		if (*argv[i] == '-')
		{
			switch (argv[i][1])
			{
				case 'p':
					print_only = TRUE;
					break;
				case 'q':
					quiet = TRUE;
					break;
				case 't':
					if (i + 1 != argc)
					{
						build_tag = argv[++i];
						break;
					}
					/* fall through */
				default:
					usage();
					return EXIT_SUCCESS;
			}
		}
		else if (!filename)
			filename = argv[i];
		else
		{
			usage();
			return EXIT_SUCCESS;
		}
	}
	if (!filename)
	{
		usage();
		return EXIT_SUCCESS;
	}

	/* Set TZ information. */
	tzset ();
	/* We are building TODAY! */
	if (! quiet)
	{
		printf ( "\nReactOS Build Number Generator\n\n");
	}

	time (& t1); /* current build time */
	t1_tm = gmtime (& t1);

	t1_tm->tm_year += 1900;
	if (! quiet)
	{
		printf (
			"Current date: %4d-%02d-%02d\n\n",
			t1_tm->tm_year,
			(t1_tm->tm_mon + 1),
			t1_tm->tm_mday);
	}

	/* Compute build number. */
	build = t1_tm->tm_year * 10000 + (t1_tm->tm_mon + 1) * 100 + t1_tm->tm_mday;

	if (!build_tag)
	{
		/* Create default build tag */
		length = count_wide_string(KERNEL_VERSION_BUILD_TYPE);
		build_tag = (char *)malloc(length+2);
		if (length > 0)
		{
			build_tag[0] = '-';
			for (i = 0; KERNEL_VERSION_BUILD_TYPE[i]; i++)
			{
				build_tag[i + 1] = KERNEL_VERSION_BUILD_TYPE[i];
			}
			build_tag[i+1] = 0;
		}
		else
			build_tag[0] = 0;
	}
	else if (*build_tag)
	{
		/* Prepend '-' */
		length = strlen(build_tag);
		char *new_build_tag = (char *)malloc(length + 2);
		strcpy(new_build_tag, "-");
		strcat(new_build_tag, build_tag);
		build_tag = new_build_tag;
	}

	revno = GetRev(revision, sizeof(revision));
	sprintf(buildstr, "%d-r%s", build, revision);

	if (! quiet)
	{
		printf (
			"ROS Version : %d.%d",
			KERNEL_VERSION_MAJOR,
			KERNEL_VERSION_MINOR);
		if (0 != KERNEL_VERSION_PATCH_LEVEL)
		{
			printf(".%d", KERNEL_VERSION_PATCH_LEVEL);
		}
		printf("%s (Build %s)\n\n", build_tag, buildstr);
	}
	/* (Over)write the include file, unless the user switched on -p. */
	if (! print_only)
	{
		write_h (build, buildstr, revno);
	}
	else
	{
		printf ("%s: no code generated", argv [0]);
	}

	return EXIT_SUCCESS;
}

/* EOF */
