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

#ifdef WORDS_BIGENDIAN
#define ENDIAN	"big"
#else
#define ENDIAN	"little"
#endif

static const char usage[] =
	"Usage: wrc [options...] [infile[.rc|.res]]\n"
	"   -b, --target=TARGET        Specify target CPU and platform when cross-compiling\n"
	"   -D, --define id[=val]      Define preprocessor identifier id=val\n"
	"   --debug=nn                 Set debug level to 'nn'\n"
	"   -E                         Preprocess only\n"
	"   --endianess=e              Set output byte-order e={n[ative], l[ittle], b[ig]}\n"
	"                              (win32 only; default is " ENDIAN "-endian)\n"
	"   -F TARGET                  Synonym for -b for compatibility with windres\n"
	"   -fo FILE                   Synonym for -o for compatibility with windres\n"
	"   -h, --help                 Prints this summary\n"
	"   -i, --input=FILE           The name of the input file\n"
	"   -I, --include-dir=PATH     Set include search dir to path (multiple -I allowed)\n"
	"   -J, --input-format=FORMAT  The input format (either `rc' or `rc16')\n"
	"   -l, --language=LANG        Set default language to LANG (default is neutral {0, 0})\n"
	"   -m16, -m32, -m64           Build for 16-bit, 32-bit resp. 64-bit platforms\n"
	"   --no-use-temp-file         Ignored for compatibility with windres\n"
	"   --nostdinc                 Disables searching the standard include path\n"
	"   -o, --output=FILE          Output to file (default is infile.res)\n"
	"   -O, --output-format=FORMAT The output format (`po', `pot', `res', or `res16`)\n"
	"   --pedantic                 Enable pedantic warnings\n"
	"   --po-dir=DIR               Directory containing po files for translations\n"
	"   --preprocessor             Specifies the preprocessor to use, including arguments\n"
	"   -r                         Ignored for compatibility with rc\n"
	"   -U, --undefine id          Undefine preprocessor identifier id\n"
	"   --use-temp-file            Ignored for compatibility with windres\n"
	"   -v, --verbose              Enable verbose mode\n"
	"   --verify-translations      Check the status of the various translations\n"
	"   --version                  Print version and exit\n"
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
static language_t *defaultlanguage;
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

int check_utf8 = 1;  /* whether to check for valid utf8 */

static int pointer_size = sizeof(void *);

static int verify_translations_mode;

static char *output_name;	/* The name given by the -o option */
char *input_name = NULL;	/* The name given on the command-line */
static char *temp_name = NULL;	/* Temporary file for preprocess pipe */

int line_number = 1;		/* The current line */
int char_number = 1;		/* The current char pos within the line */

char *cmdline;			/* The entire commandline */
time_t now;			/* The time of start of wrc */

int parser_debug, yy_flex_debug;

resource_t *resource_top;	/* The top of the parsed resources */

int getopt (int argc, char *const *argv, const char *optstring);
static void cleanup_files(void);
static void segvhandler(int sig);

enum long_options_values
{
    LONG_OPT_NOSTDINC = 1,
    LONG_OPT_TMPFILE,
    LONG_OPT_NOTMPFILE,
    LONG_OPT_PO_DIR,
    LONG_OPT_PREPROCESSOR,
    LONG_OPT_VERSION,
    LONG_OPT_DEBUG,
    LONG_OPT_ENDIANESS,
    LONG_OPT_PEDANTIC,
    LONG_OPT_VERIFY_TRANSL
};

static const char short_options[] =
	"b:D:Ef:F:hi:I:J:l:m:o:O:rU:v";
static const struct option long_options[] = {
	{ "debug", 1, NULL, LONG_OPT_DEBUG },
	{ "define", 1, NULL, 'D' },
	{ "endianess", 1, NULL, LONG_OPT_ENDIANESS },
	{ "help", 0, NULL, 'h' },
	{ "include-dir", 1, NULL, 'I' },
	{ "input", 1, NULL, 'i' },
	{ "input-format", 1, NULL, 'J' },
	{ "language", 1, NULL, 'l' },
	{ "no-use-temp-file", 0, NULL, LONG_OPT_NOTMPFILE },
	{ "nostdinc", 0, NULL, LONG_OPT_NOSTDINC },
	{ "output", 1, NULL, 'o' },
	{ "output-format", 1, NULL, 'O' },
	{ "pedantic", 0, NULL, LONG_OPT_PEDANTIC },
	{ "po-dir", 1, NULL, LONG_OPT_PO_DIR },
	{ "preprocessor", 1, NULL, LONG_OPT_PREPROCESSOR },
	{ "target", 1, NULL, 'F' },
	{ "undefine", 1, NULL, 'U' },
	{ "use-temp-file", 0, NULL, LONG_OPT_TMPFILE },
	{ "verbose", 0, NULL, 'v' },
	{ "verify-translations", 0, NULL, LONG_OPT_VERIFY_TRANSL },
	{ "version", 0, NULL, LONG_OPT_VERSION },
	{ NULL, 0, NULL, 0 }
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

/* load a single input file */
static int load_file( const char *input_name, const char *output_name )
{
    int ret;

    /* Run the preprocessor on the input */
    if(!no_preprocess)
    {
        FILE *output;
        int ret, fd;
        char *name;

        /*
         * Preprocess the input to a temp-file, or stdout if
         * no output was given.
         */

        if (preprocess_only)
        {
            if (output_name)
            {
                if (!(output = fopen( output_name, "w" )))
                    fatal_perror( "Could not open %s for writing", output_name );
                ret = wpp_parse( input_name, output );
                fclose( output );
            }
            else ret = wpp_parse( input_name, stdout );

            if (ret) return ret;
            output_name = NULL;
            exit(0);
        }

        if (output_name && output_name[0]) name = strmake( "%s.XXXXXX", output_name );
        else name = xstrdup( "wrc.XXXXXX" );

        if ((fd = mkstemps( name, 0 )) == -1)
            error("Could not generate a temp name from %s\n", name);

        temp_name = name;
        if (!(output = fdopen(fd, "wt")))
            error("Could not open fd %s for writing\n", name);

        ret = wpp_parse( input_name, output );
        fclose( output );
        if (ret) return ret;
        input_name = name;
    }

    /* Reset the language */
    currentlanguage = dup_language( defaultlanguage );
    check_utf8 = 1;

    /* Go from .rc to .res */
    chat("Starting parse\n");

    if(!(parser_in = fopen(input_name, "rb")))
        fatal_perror("Could not open %s for input", input_name);

    ret = parser_parse();
    fclose(parser_in);
    parser_lex_destroy();
    if (temp_name)
    {
        unlink( temp_name );
        temp_name = NULL;
    }
    free( currentlanguage );
    return ret;
}

static void set_target( const char *target )
{
    char *p, *cpu = xstrdup( target );

    /* target specification is in the form CPU-MANUFACTURER-OS or CPU-MANUFACTURER-KERNEL-OS */
    if (!(p = strchr( cpu, '-' ))) error( "Invalid target specification '%s'\n", target );
    *p = 0;
    if (!strcmp( cpu, "amd64" ) || !strcmp( cpu, "x86_64" ) || !strcmp( cpu, "ia64" ))
        pointer_size = 8;
    else
        pointer_size = 4;
    free( cpu );
}

int main(int argc,char *argv[])
{
	extern char* optarg;
	extern int   optind;
	int optc;
	int opti = 0;
	int stdinc = 1;
	int lose = 0;
	int nb_files = 0;
	int i;
	int cmdlen;
        int po_mode = 0;
        char *po_dir = NULL;
        char **files = xmalloc( argc * sizeof(*files) );

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
	/* Microsoft RC always searches current directory */
	wpp_add_include_path(".");

	/* First rebuild the commandline to put in destination */
	/* Could be done through env[], but not all OS-es support it */
	cmdlen = 4; /* for "wrc " */
	for(i = 1; i < argc; i++)
		cmdlen += strlen(argv[i]) + 1;
	cmdline = xmalloc(cmdlen);
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
		case LONG_OPT_NOSTDINC:
			stdinc = 0;
			break;
		case LONG_OPT_TMPFILE:
			if (debuglevel) warning("--use-temp-file option not yet supported, ignored.\n");
			break;
		case LONG_OPT_NOTMPFILE:
			if (debuglevel) warning("--no-use-temp-file option not yet supported, ignored.\n");
			break;
		case LONG_OPT_PO_DIR:
			po_dir = xstrdup( optarg );
			break;
		case LONG_OPT_PREPROCESSOR:
			if (strcmp(optarg, "cat") == 0) no_preprocess = 1;
			else fprintf(stderr, "-P option not yet supported, ignored.\n");
			break;
		case LONG_OPT_VERSION:
			printf(version_string);
			exit(0);
			break;
		case LONG_OPT_DEBUG:
			debuglevel = strtol(optarg, NULL, 0);
			break;
		case LONG_OPT_ENDIANESS:
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
		case LONG_OPT_PEDANTIC:
			pedantic = 1;
			wpp_set_pedantic(1);
			break;
		case LONG_OPT_VERIFY_TRANSL:
			verify_translations_mode = 1;
			break;
		case 'D':
			wpp_add_cmdline_define(optarg);
			break;
		case 'E':
			preprocess_only = 1;
			break;
		case 'b':
		case 'F':
			set_target( optarg );
			break;
		case 'h':
			printf(usage);
			exit(0);
		case 'i':
			files[nb_files++] = optarg;
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
				defaultlanguage = new_language(PRIMARYLANGID(lan), SUBLANGID(lan));
			}
			break;
                case 'm':
			if (!strcmp( optarg, "16" )) win32 = 0;
			else if (!strcmp( optarg, "32" )) { win32 = 1; pointer_size = 4; }
			else if (!strcmp( optarg, "64" )) { win32 = 1; pointer_size = 8; }
			else error( "Invalid option: -m%s\n", optarg );
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
			if (strcmp(optarg, "po") == 0) po_mode = 1;
			else if (strcmp(optarg, "pot") == 0) po_mode = 2;
			else if (strcmp(optarg, "res16") == 0) win32 = 0;
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

	if (win32)
	{
		wpp_add_cmdline_define("_WIN32=1");
		if (pointer_size == 8) wpp_add_cmdline_define("_WIN64=1");
	}

	/* If we do need to search standard includes, add them to the path */
	if (stdinc)
	{
        /* ReactOS doesn't use this feature
		wpp_add_include_path(INCLUDEDIR"/msvcrt");
		wpp_add_include_path(INCLUDEDIR"/windows");
        */
	}

	/* Kill io buffering when some kind of debuglevel is enabled */
	if(debuglevel)
	{
		setbuf(stdout, NULL);
		setbuf(stderr, NULL);
	}

	parser_debug = debuglevel & DEBUGLEVEL_TRACE ? 1 : 0;
	yy_flex_debug = debuglevel & DEBUGLEVEL_TRACE ? 1 : 0;

        wpp_set_debug( (debuglevel & DEBUGLEVEL_PPLEX) != 0,
                       (debuglevel & DEBUGLEVEL_PPTRACE) != 0,
                       (debuglevel & DEBUGLEVEL_PPMSG) != 0 );

	/* Check if the user set a language, else set default */
	if(!defaultlanguage)
		defaultlanguage = new_language(0, 0);

	atexit(cleanup_files);

        while (optind < argc) files[nb_files++] = argv[optind++];

        for (i = 0; i < nb_files; i++)
        {
            input_name = files[i];
            if (load_file( input_name, output_name )) exit(1);
        }
	/* stdin special case. NULL means "stdin" for wpp. */
        if (nb_files == 0 && load_file( NULL, output_name )) exit(1);

	if(debuglevel & DEBUGLEVEL_DUMP)
		dump_resources(resource_top);

	if(verify_translations_mode)
	{
		verify_translations(resource_top);
		exit(0);
	}
	if (po_mode)
	{
            if (po_mode == 2)  /* pot file */
            {
                if (!output_name)
                {
                    output_name = dup_basename( nb_files ? files[0] : NULL, ".rc" );
                    strcat( output_name, ".pot" );
                }
                write_pot_file( output_name );
            }
            else write_po_files( output_name );
            output_name = NULL;
            exit(0);
	}
        if (po_dir) add_translations( po_dir );

	/* Convert the internal lists to binary data */
	resources2res(resource_top);

	chat("Writing .res-file\n");
        if (!output_name)
        {
            output_name = dup_basename( nb_files ? files[0] : NULL, ".rc" );
            strcat(output_name, ".res");
        }
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
