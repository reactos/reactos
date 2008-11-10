/*
 * Copyright 1994 Martin von Loewis
 * Copyright 1998 Bertho A. Stultiens (BS)
 * Copyright 2003 Dimitrie O. Paun
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <signal.h>
#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif

#include "wrc.h"
#include "utils.h"
#include "readres.h"
#include "dumpres.h"
#include "genres.h"
#include "newstruc.h"
#include "parser.h"
#include "wine/wpp.h"

#ifndef INCLUDEDIR
#define INCLUDEDIR "/usr/local/include/wine"
#endif

#ifdef WORDS_BIGENDIAN
#define ENDIAN	"big"
#else
#define ENDIAN	"little"
#endif

static const char usage[] =
	"Usage: wrc [options...] [infile[.rc|.res]] [outfile]\n"
	"   -D id[=val] Define preprocessor identifier id=val\n"
	"   -E          Preprocess only\n"
	"   -F target   Ignored for compatibility with windres\n"
	"   -h          Prints this summary\n"
	"   -i file     The name of the input file\n"
	"   -I path     Set include search dir to path (multiple -I allowed)\n"
	"   -J format   The input format (either `rc' or `rc16')\n"
	"   -l lan      Set default language to lan (default is neutral {0, 0})\n"
	"   -o file     Output to file (default is infile.res)\n"
	"   -O format   The output format (either `res' or `res16`)\n"
	"   -r          Ignored for compatibility with rc\n"
	"   -U id       Undefine preprocessor identifier id\n"
	"   -v          Enable verbose mode\n"
	"The following long options are supported:\n"
	"   --debug=nn            Set debug level to 'nn'\n"
	"   --define              Synonym for -D\n"
	"   --endianess=e         Set output byte-order e={n[ative], l[ittle], b[ig]}\n"
	"                         (win32 only; default is " ENDIAN "-endian)\n"
	"   --help                Synonym for -h\n"
	"   --include-dir         Synonym for -I\n"
	"   --input               Synonym for -i\n"
	"   --input-format        Synonym for -J\n"
	"   --language            Synonym for -l\n"
	"   --no-use-temp-file    Ignored for compatibility with windres\n"
	"   --nostdinc            Disables searching the standard include path\n"
	"   --output -fo          Synonym for -o\n"
	"   --output-format       Synonym for -O\n"
	"   --pedantic            Enable pedantic warnings\n"
	"   --preprocessor        Specifies the preprocessor to use, including arguments\n"
	"   --target              Synonym for -F\n"
	"   --undefine            Synonym for -U\n"
	"   --use-temp-file       Ignored for compatibility with windres\n"
	"   --verbose             Synonym for -v\n"
	"   --verify-translations Check the status of the various translations\n"
	"   --version             Print version and exit\n"
	"Input is taken from stdin if no sourcefile specified.\n"
	"Debug level 'n' is a bitmask with following meaning:\n"
	"    * 0x01 Tell which resource is parsed (verbose mode)\n"
	"    * 0x02 Dump internal structures\n"
	"    * 0x04 Create a parser trace (yydebug=1)\n"
	"    * 0x08 Preprocessor messages\n"
	"    * 0x10 Preprocessor lex messages\n"
	"    * 0x20 Preprocessor yacc trace\n"
	"If no input filename is given and the output name is not overridden\n"
	"with -o, then the output is written to \"wrc.tab.res\"\n"
	;

static const char version_string[] = "Wine Resource Compiler version " PACKAGE_VERSION "\n"
			"Copyright 1998-2000 Bertho A. Stultiens\n"
			"          1994 Martin von Loewis\n";

/*
 * Set if compiling in 32bit mode (default).
 */
int win32 = 1;

/*
 * debuglevel == DEBUGLEVEL_NONE	Don't bother
 * debuglevel & DEBUGLEVEL_CHAT		Say what's done
 * debuglevel & DEBUGLEVEL_DUMP		Dump internal structures
 * debuglevel & DEBUGLEVEL_TRACE	Create parser trace
 * debuglevel & DEBUGLEVEL_PPMSG	Preprocessor messages
 * debuglevel & DEBUGLEVEL_PPLEX	Preprocessor lex trace
 * debuglevel & DEBUGLEVEL_PPTRACE	Preprocessor yacc trace
 */
int debuglevel = DEBUGLEVEL_NONE;

/*
 * Recognize win32 keywords if set (-w 32 enforces this),
 * otherwise set with -e option.
 */
int extensions = 1;

/*
 * Language setting for resources (-l option)
 */
language_t *currentlanguage = NULL;

/*
 * Set when extra warnings should be generated (-W option)
 */
int pedantic = 0;

/*
 * The output byte-order of resources (set with -B)
 */
int byteorder = WRC_BO_NATIVE;

/*
 * Set when _only_ to run the preprocessor (-E option)
 */
int preprocess_only = 0;

/*
 * Set when _not_ to run the preprocessor (-P cat option)
 */
int no_preprocess = 0;

static int verify_translations_mode;

char *output_name = NULL;	/* The name given by the -o option */
char *input_name = NULL;	/* The name given on the command-line */
char *temp_name = NULL;		/* Temporary file for preprocess pipe */

int line_number = 1;		/* The current line */
int char_number = 1;		/* The current char pos within the line */

char *cmdline;			/* The entire commandline */
time_t now;			/* The time of start of wrc */

int parser_debug, yy_flex_debug;

resource_t *resource_top;	/* The top of the parsed resources */

int getopt (int argc, char *const *argv, const char *optstring);
static void cleanup_files(void);
static void segvhandler(int sig);

static const char short_options[] =
	"D:Ef:F:hi:I:J:l:o:O:rU:v";
static const struct option long_options[] = {
	{ "debug", 1, 0, 6 },
	{ "define", 1, 0, 'D' },
	{ "endianess", 1, 0, 7 },
	{ "help", 0, 0, 'h' },
	{ "include-dir", 1, 0, 'I' },
	{ "input", 1, 0, 'i' },
	{ "input-format", 1, 0, 'J' },
	{ "language", 1, 0, 'l' },
	{ "no-use-temp-file", 0, 0, 3 },
	{ "nostdinc", 0, 0, 1 },
	{ "output", 1, 0, 'o' },
	{ "output-format", 1, 0, 'O' },
	{ "pedantic", 0, 0, 8 },
	{ "preprocessor", 1, 0, 4 },
	{ "target", 1, 0, 'F' },
	{ "undefine", 1, 0, 'U' },
	{ "use-temp-file", 0, 0, 2 },
	{ "verbose", 0, 0, 'v' },
	{ "verify-translations", 0, 0, 9 },
	{ "version", 0, 0, 5 },
	{ 0, 0, 0, 0 }
};

static void set_version_defines(void)
{
    char *version = xstrdup( PACKAGE_VERSION );
    char *major, *minor, *patchlevel;
    char buffer[100];

    if ((minor = strchr( version, '.' )))
    {
        major = version;
        *minor++ = 0;
        if ((patchlevel = strchr( minor, '.' ))) *patchlevel++ = 0;
    }
    else  /* pre 0.9 version */
    {
        major = NULL;
        patchlevel = version;
    }
    sprintf( buffer, "__WRC__=%s", major ? major : "0" );
    wpp_add_cmdline_define(buffer);
    sprintf( buffer, "__WRC_MINOR__=%s", minor ? minor : "0" );
    wpp_add_cmdline_define(buffer);
    sprintf( buffer, "__WRC_PATCHLEVEL__=%s", patchlevel ? patchlevel : "0" );
    wpp_add_cmdline_define(buffer);
    free( version );
}

/* clean things up when aborting on a signal */
static void exit_on_signal( int sig )
{
    exit(1);  /* this will call the atexit functions */
}

int main(int argc,char *argv[])
{
	extern char* optarg;
	extern int   optind;
	int optc;
	int opti = 0;
	int stdinc = 1;
	int lose = 0;
	int ret;
	int i;
	int cmdlen;

	signal(SIGSEGV, segvhandler);
        signal( SIGTERM, exit_on_signal );
        signal( SIGINT, exit_on_signal );
#ifdef SIGHUP
        signal( SIGHUP, exit_on_signal );
#endif

	now = time(NULL);

	/* Set the default defined stuff */
        set_version_defines();
	wpp_add_cmdline_define("RC_INVOKED=1");
	wpp_add_cmdline_define("__WIN32__=1");
	wpp_add_cmdline_define("__FLAT__=1");
	/* Microsoft RC always searches current directory */
	wpp_add_include_path(".");

	/* First rebuild the commandline to put in destination */
	/* Could be done through env[], but not all OS-es support it */
	cmdlen = 4; /* for "wrc " */
	for(i = 1; i < argc; i++)
		cmdlen += strlen(argv[i]) + 1;
	cmdline = (char *)xmalloc(cmdlen);
	strcpy(cmdline, "wrc ");
	for(i = 1; i < argc; i++)
	{
		strcat(cmdline, argv[i]);
		if(i < argc-1)
			strcat(cmdline, " ");
	}

	while((optc = getopt_long(argc, argv, short_options, long_options, &opti)) != EOF)
	{
		switch(optc)
		{
		case 1:
			stdinc = 0;
			break;
		case 2:
			if (debuglevel) warning("--use-temp-file option not yet supported, ignored.\n");
			break;
		case 3:
			if (debuglevel) warning("--no-use-temp-file option not yet supported, ignored.\n");
			break;
		case 4:
			if (strcmp(optarg, "cat") == 0) no_preprocess = 1;
			else fprintf(stderr, "-P option not yet supported, ignored.\n");
			break;
		case 5:
			printf(version_string);
			exit(0);
			break;
		case 6:
			debuglevel = strtol(optarg, NULL, 0);
			break;
		case 7:
			switch(optarg[0])
			{
			case 'n':
			case 'N':
				byteorder = WRC_BO_NATIVE;
				break;
			case 'l':
			case 'L':
				byteorder = WRC_BO_LITTLE;
				break;
			case 'b':
			case 'B':
				byteorder = WRC_BO_BIG;
				break;
			default:
				fprintf(stderr, "Byte ordering must be n[ative], l[ittle] or b[ig]\n");
				lose++;
			}
			break;
		case 8:
			pedantic = 1;
			wpp_set_pedantic(1);
			break;
		case 9:
			verify_translations_mode = 1;
			break;
		case 'D':
			wpp_add_cmdline_define(optarg);
			break;
		case 'E':
			preprocess_only = 1;
			break;
		case 'F':
			/* ignored for compatibility with windres */
			break;
		case 'h':
			printf(usage);
			exit(0);
		case 'i':
			if (!input_name) input_name = strdup(optarg);
			else error("Too many input files.\n");
			break;
		case 'I':
			wpp_add_include_path(optarg);
			break;
		case 'J':
			if (strcmp(optarg, "rc16") == 0)  extensions = 0;
			else if (strcmp(optarg, "rc")) error("Output format %s not supported.\n", optarg);
			break;
		case 'l':
			{
				int lan;
				lan = strtol(optarg, NULL, 0);
				if (get_language_codepage(PRIMARYLANGID(lan), SUBLANGID(lan)) == -1)
					error("Language %04x is not supported\n", lan);
				currentlanguage = new_language(PRIMARYLANGID(lan), SUBLANGID(lan));
			}
			break;
		case 'f':
			if (*optarg != 'o') error("Unknown option: -f%s\n",  optarg);
			optarg++;
			/* fall through */
		case 'o':
			if (!output_name) output_name = strdup(optarg);
			else error("Too many output files.\n");
			break;
		case 'O':
			if (strcmp(optarg, "res16") == 0)
			{
				win32 = 0;
				wpp_del_define("__WIN32__");
				wpp_del_define("__FLAT__");
			}
			else if (strcmp(optarg, "res")) warning("Output format %s not supported.\n", optarg);
			break;
		case 'r':
			/* ignored for compatibility with rc */
			break;
		case 'U':
			wpp_del_define(optarg);
			break;
		case 'v':
			debuglevel = DEBUGLEVEL_CHAT;
			break;
		default:
			lose++;
			break;
		}
	}

	if(lose)
	{
		fprintf(stderr, usage);
		return 1;
	}

	/* If we do need to search standard includes, add them to the path */
	if (stdinc)
	{
		wpp_add_include_path(INCLUDEDIR"/msvcrt");
		wpp_add_include_path(INCLUDEDIR"/windows");
	}
	
	/* Check for input file on command-line */
	if(optind < argc)
	{
		if (!input_name) input_name = argv[optind++];
		else error("Too many input files.\n");
	}

	/* Check for output file on command-line */
	if(optind < argc)
	{
		if (!output_name) output_name = argv[optind++];
		else error("Too many output files.\n");
	}

	/* Kill io buffering when some kind of debuglevel is enabled */
	if(debuglevel)
	{
		setbuf(stdout,0);
		setbuf(stderr,0);
	}

	parser_debug = debuglevel & DEBUGLEVEL_TRACE ? 1 : 0;
	yy_flex_debug = debuglevel & DEBUGLEVEL_TRACE ? 1 : 0;

        wpp_set_debug( (debuglevel & DEBUGLEVEL_PPLEX) != 0,
                       (debuglevel & DEBUGLEVEL_PPTRACE) != 0,
                       (debuglevel & DEBUGLEVEL_PPMSG) != 0 );

	/* Check if the user set a language, else set default */
	if(!currentlanguage)
		currentlanguage = new_language(0, 0);

	/* Generate appropriate outfile names */
	if(!output_name && !preprocess_only)
	{
		output_name = dup_basename(input_name, ".rc");
		strcat(output_name, ".res");
	}
	atexit(cleanup_files);

	/* Run the preprocessor on the input */
	if(!no_preprocess)
	{
		/*
		 * Preprocess the input to a temp-file, or stdout if
		 * no output was given.
		 */

		chat("Starting preprocess\n");

                if (!preprocess_only)
                {
                    ret = wpp_parse_temp( input_name, output_name, &temp_name );
                }
                else if (output_name)
                {
                    FILE *output;

                    if (!(output = fopen( output_name, "w" )))
                        error( "Could not open %s for writing\n", output_name );
                    ret = wpp_parse( input_name, output );
                    fclose( output );
                }
                else
                {
                    ret = wpp_parse( input_name, stdout );
                }

		if(ret)
			exit(1);	/* Error during preprocess */

		if(preprocess_only)
		{
			output_name = NULL;
			exit(0);
		}

		input_name = temp_name;
	}

	/* Go from .rc to .res */
	chat("Starting parse\n");

	if(!(parser_in = fopen(input_name, "rb")))
		error("Could not open %s for input\n", input_name);

	ret = parser_parse();

	if(input_name) fclose(parser_in);

	if(ret) exit(1); /* Error during parse */

	if(debuglevel & DEBUGLEVEL_DUMP)
		dump_resources(resource_top);

	if(verify_translations_mode)
	{
		verify_translations(resource_top);
		exit(0);
	}

	/* Convert the internal lists to binary data */
	resources2res(resource_top);

	chat("Writing .res-file\n");
	write_resfile(output_name, resource_top);
	output_name = NULL;

	return 0;
}


static void cleanup_files(void)
{
	if (output_name) unlink(output_name);
	if (temp_name) unlink(temp_name);
}

static void segvhandler(int sig)
{
	fprintf(stderr, "\n%s:%d: Oops, segment violation\n", input_name, line_number);
	fflush(stdout);
	fflush(stderr);
	abort();
}
