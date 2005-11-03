/*
 * Wine Message Compiler main program
 *
 * Copyright 2000 Bertho A. Stultiens (BS)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "config.h"

#include "wmc.h"
#include "utils.h"
#include "lang.h"
#include "write.h"

static char usage[] =
	"Usage: wmc [options...] [inputfile.mc]\n"
	"   -B x        Set output byte-order x={n[ative], l[ittle], b[ig]}\n"
	"               (default is n[ative] which equals "
#ifdef WORDS_BIGENDIAN
	"big"
#else
	"little"
#endif
	"-endian)\n"
	"   -c          Set 'custom-bit' in values\n"
	"   -d          Use decimal values in output\n"
	"   -D		Set debug flag\n"
	"   -h          This message\n"
	"   -H file     Write headerfile to file (default is inputfile.h)\n"
	"   -i          Inline messagetable(s)\n"
	"   -o file     Output to file (default is inputfile.rc)\n"
	"   -u          Inputfile is in unicode\n"
	"   -U          Output unicode messagetable(s)\n"
	"   -v          Show supported codepages and languages\n"
	"   -V          Print version end exit\n"
	"   -W          Enable pedantic warnings\n"
	"Input is taken from stdin if no inputfile is specified.\n"
	"Byteorder of unicode input is based upon the first couple of\n"
	"bytes read, which should be 0x0000..0x00ff.\n"
	;

static char version_string[] =
	"Wine Message Compiler Version " WMC_FULLVERSION "\n"
	"Copyright 2000 Bertho A. Stultiens\n"
	;

/*
 * The output byte-order of resources (set with -B)
 */
int byteorder = WMC_BO_NATIVE;

/*
 * Custom bit (bit 29) in output values must be set (-c option)
 */
int custombit = 0;

/*
 * Output decimal values (-d option)
 */
int decimal = 0;

/*
 * Enable pedantic warnings; check arg references (-W option)
 */
int pedantic = 0;

/*
 * Unicode input (-u option)
 */
int unicodein = 0;

/*
 * Unicode output (-U option)
 */
int unicodeout = 0;

/*
 * Inline the messagetables (don't write *.bin files; -i option)
 */
int rcinline = 0;

/*
 * Debugging flag (-D option)
 */
int dodebug = 0;

char *output_name = NULL;	/* The name given by the -o option */
char *input_name = NULL;	/* The name given on the command-line */
char *header_name = NULL;	/* The name given by the -H option */

int line_number = 1;		/* The current line */
int char_number = 1;		/* The current char pos within the line */

char *cmdline;			/* The entire commandline */
time_t now;			/* The time of start of wmc */

int getopt (int argc, char *const *argv, const char *optstring);
static void segvhandler(int sig);

int main(int argc,char *argv[])
{
	extern char* optarg;
	extern int   optind;
	int optc;
	int lose = 0;
	int ret;
	int i;
	int cmdlen;

	signal(SIGSEGV, segvhandler);

	now = time(NULL);

	/* First rebuild the commandline to put in destination */
	/* Could be done through env[], but not all OS-es support it */
	cmdlen = 4; /* for "wmc " */
	for(i = 1; i < argc; i++)
		cmdlen += strlen(argv[i]) + 1;
	cmdline = (char *)xmalloc(cmdlen);
	strcpy(cmdline, "wmc ");
	for(i = 1; i < argc; i++)
	{
		strcat(cmdline, argv[i]);
		if(i < argc-1)
			strcat(cmdline, " ");
	}

	while((optc = getopt(argc, argv, "B:cdDhH:io:p:uUvVW")) != EOF)
	{
		switch(optc)
		{
		case 'B':
			switch(optarg[0])
			{
			case 'n':
			case 'N':
				byteorder = WMC_BO_NATIVE;
				break;
			case 'l':
			case 'L':
				byteorder = WMC_BO_LITTLE;
				break;
			case 'b':
			case 'B':
				byteorder = WMC_BO_BIG;
				break;
			default:
				fprintf(stderr, "Byteordering must be n[ative], l[ittle] or b[ig]\n");
				lose++;
			}
			break;
		case 'c':
			custombit = 1;
			break;
		case 'd':
			decimal = 1;
			break;
		case 'D':
			dodebug = 1;
			break;
		case 'h':
			printf("%s", usage);
			exit(0);
			/* No return */
		case 'H':
			header_name = xstrdup(optarg);
			break;
		case 'i':
			rcinline = 1;
			break;
		case 'o':
			output_name = xstrdup(optarg);
			break;
		case 'u':
			unicodein = 1;
			break;
		case 'U':
			unicodeout = 1;
			break;
		case 'v':
			show_languages();
			show_codepages();
			exit(0);
			/* No return */
		case 'V':
			printf(version_string);
			exit(0);
			/* No return */
		case 'W':
			pedantic = 1;
			break;
		default:
			lose++;
			break;
		}
	}

	if(lose)
	{
		fprintf(stderr, "%s", usage);
		return 1;
	}

	yydebug = dodebug;
	if(dodebug)
	{
		setbuf(stdout, 0);
		setbuf(stderr, 0);
	}

	/* Check for input file on command-line */
	if(optind < argc)
	{
		input_name = argv[optind];
	}

	/* Generate appropriate outfile names */
	if(!output_name)
	{
		output_name = dup_basename(input_name, ".mc");
		strcat(output_name, ".rc");
	}

	if(!header_name)
	{
		header_name = dup_basename(input_name, ".mc");
		strcat(header_name, ".h");
	}

	if(input_name)
	{
		if(!(yyin = fopen(input_name, "rb")))
			error("Could not open %s for input\n", input_name);
	}
	else
		yyin = stdin;

	ret = yyparse();

	if(input_name)
		fclose(yyin);

	if(ret)
	{
		/* Error during parse */
		exit(1);
	}

	write_h_file(header_name);
	write_rc_file(output_name);
	if(!rcinline)
		write_bin_files();

	return 0;
}

static void segvhandler(int sig)
{
	fprintf(stderr, "\n%s:%d: Oops, segment violation\n", input_name, line_number);
	fflush(stdout);
	fflush(stderr);
	abort();
}

