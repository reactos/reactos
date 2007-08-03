/************************************************
 *
 * Converting binary resources from/to *.rc files
 *
 * Copyright 1999 Juergen Schmied
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#if defined(WIN32)
#define DIR_SEPARATOR "\\"
#define C_SEP '\\'
#define C_BAD_SEP '/'
#else
#define DIR_SEPARATOR "/"
#define C_SEP '/'
#define C_BAD_SEP '\\'
#endif

extern int mkstemps(char *template, int suffix_len);

int process_resources(const char* input_file_name, const char* specific_file_name, 
		      const char* relative_path, const char* output_path,
		      int inserting, int force_processing, int verbose);

static const char* help =
        "Usage: bin2res [OPTIONS] <rsrc.rc>\n"
	"  -a archive binaries into the <rsrc.rc> file\n"
	"  -x extract binaries from the <rsrc.rc> file\n"
	"  -i <filename> archive the named file into the <rsrc.rc> file\n"
	"  -o <filename> extract the named file from the <rsrc.rc> file\n"
	"  -b <path> assume resources are relative to this base path\n"
	"  -f force processing of older resources\n"
	"  -v causes the command to be verbous during processing\n" 
	"  -h print this help screen and exit\n"
	"\n"
	"This tool allows the insertion/extractions of embedded binary\n"
	"resources to/from .rc files, for storage within the cvs tree.\n"
	"This is accomplished by placing a magic marker in a comment\n"
	"just above the resource. The marker consists of the BINRES\n"
	"string followed by the file name. For example, to insert a\n"
	"brand new binary resource in a .rc file, place the marker\n"
	"above empty brackets:\n"
	"    /* BINRES idb_std_small.bmp */\n"
	"   {}\n"
	"To merge the binary resources into the .rc file, run:\n"
	"   bin2res -a myrsrc.rc\n"
	"Only resources that are newer than the .rc are processed.\n"
	"To extract the binary resources from the .rc file, run:\n"
	"  bin2res -x myrsrc.rc\n"
	"Binary files newer than the .rc file are not overwritten.\n"
	"\n"
	"To force processing of all resources, use the -f flag.\n"
	"To process a particular file, use the -i/-o options.\n";

void usage(void)
{
    printf(help);
    exit(1);
}

int insert_hexdump (FILE* outfile, FILE* infile)
{
    int i, c;

    fprintf (outfile, "{\n '");
    for (i = 0; (c = fgetc(infile)) != EOF; i++)
    {
	if (i && (i % 16) == 0) fprintf (outfile, "'\n '");
	if (i % 16)  fprintf (outfile, " ");
        fprintf(outfile, "%02X", c);
    }
    fprintf (outfile, "'\n}");

    return 1;
}

int hex2bin(char c)
{
    if (!isxdigit(c)) return -1024;
    if (isdigit(c)) return c - '0';
    return toupper(c) - 'A' + 10;
}

int extract_hexdump (FILE* outfile, FILE* infile)
{
    int byte, c;

    while ( (c = fgetc(infile)) != EOF && c != '}')
    {
        if (isspace(c) || c == '\'') continue;
	byte = 16 * hex2bin(c);
	c = fgetc(infile);
	if (c == EOF) return 0;
	byte += hex2bin(c);
	if (byte < 0) return 0;
	fputc(byte, outfile);
    }
    return 1;
}

const char* parse_marker(const char *line, time_t* last_updated)
{
    static char res_file_name[PATH_MAX], *rpos, *wpos;
    struct stat st;

    if (!(rpos = strstr(line, "BINRES"))) return 0;
    for (rpos += 6; *rpos && isspace(*rpos); rpos++) /**/;
    for (wpos = res_file_name; *rpos && !isspace(*rpos); ) *wpos++ = *rpos++;
    *wpos = 0;

    *last_updated = (stat(res_file_name, &st) < 0) ? 0 : st.st_mtime;

    return res_file_name;
}

const char* parse_include(const char *line)
{
    static char include_filename[PATH_MAX], *rpos, *wpos;

    if (!(rpos = strstr(line, "#"))) return 0;
    for (rpos += 1; *rpos && isspace(*rpos); rpos++) /**/;
    if (!(rpos = strstr(line, "include"))) return 0;
    for (rpos += 7; *rpos && isspace(*rpos); rpos++) /**/;
    for (; *rpos && (*rpos == '"' || *rpos == '<'); rpos++) /**/;
    for (wpos = include_filename; *rpos && !(*rpos == '"' || *rpos == '<'); ) *wpos++ = *rpos++;
    if (!(rpos = strstr(include_filename, ".rc"))) return 0;
    *wpos = 0;

    return include_filename;
}

char* get_filename_with_full_path(char* output, const char* filename, const char* relative_path)
{
	if (relative_path != NULL && relative_path[0] != 0)
	{
		strcpy(output, relative_path);
		strcat(output, DIR_SEPARATOR);
		strcat(output, filename);
	}
	else
		strcpy(output, filename);
	return output;
}

void process_includes(const char* input_file_name, const char* specific_file_name, 
		     const char* relative_path, const char* output_path,
		     int inserting, int force_processing, int verbose)
{
    char filename[PATH_MAX];
    char buffer[2048];
    const char *include_file_name;
    FILE *fin;
    int c;

    if (!(fin = fopen(input_file_name, "r"))) return;
    for (c = EOF; fgets(buffer, sizeof(buffer), fin); c = EOF)
    {
	if (!(include_file_name = parse_include(buffer))) continue;
	if ( verbose ) printf ( "Processing included file %s\n", include_file_name);
	process_resources(get_filename_with_full_path(filename, include_file_name, relative_path),
	                  specific_file_name, relative_path, output_path,
		          inserting, force_processing, verbose);
    }
    fclose(fin);
}

int process_resources(const char* input_file_name, const char* specific_file_name, 
		      const char* relative_path, const char* output_path,
		      int inserting, int force_processing, int verbose)
{
    char filename[PATH_MAX];
    char buffer[2048], tmp_file_name[PATH_MAX];
    const char *res_file_name;
    time_t rc_last_update, res_last_update;
    FILE *fin, *fres, *ftmp = 0;
    struct stat st;
    int fd, c;

    if (!(fin = fopen(input_file_name, "r"))) return 0;
    if (stat(input_file_name, &st) < 0) return 0;
    rc_last_update = st.st_mtime;

    if (inserting)
    {
	strcpy(tmp_file_name, input_file_name);
	strcat(tmp_file_name, "-XXXXXX.temp");
	if ((fd = mkstemps(tmp_file_name, 5)) == -1)
	{
	    strcpy(tmp_file_name, "/tmp/bin2res-XXXXXX.temp");
	    if ((fd = mkstemps(tmp_file_name, 5)) == -1) return 0;
	}
	if (!(ftmp = fdopen(fd, "w"))) return 0;
    }

    for (c = EOF; fgets(buffer, sizeof(buffer), fin); c = EOF)
    {
	if (inserting) fprintf(ftmp, "%s", buffer);
	if (!(res_file_name = parse_marker(buffer, &res_last_update))) continue;
        if ( (specific_file_name && strcmp(specific_file_name, res_file_name)) ||
	     (!force_processing && ((rc_last_update < res_last_update) == !inserting)) )
        {
	    if (verbose) printf("skipping '%s'\n", res_file_name);
            continue;
        }

        if (verbose) printf("processing '%s'\n", res_file_name);
	while ( (c = fgetc(fin)) != EOF && c != '{')
	    if (inserting) fputc(c, ftmp);
	if (c == EOF) break;

	if (inserting)
	{
	    if (!(fres = fopen(get_filename_with_full_path(filename, res_file_name, relative_path),
	    "rb"))) break;
	    if (!insert_hexdump(ftmp, fres)) break;
	    while ( (c = fgetc(fin)) != EOF && c != '}') /**/;
	}
	else
	{
	    if (!(fres = fopen(get_filename_with_full_path(filename, res_file_name, output_path),
	    "wb"))) break;
	    if (!extract_hexdump(fres, fin)) break;
	}
	fclose(fres);
    }

    fclose(fin);

    if (inserting)
    {
	fclose(ftmp);
	if (c == EOF)
        {
            if (rename(tmp_file_name, input_file_name) < 0)
            {
                /* try unlinking first, Windows rename is brain-damaged */
                if (unlink(input_file_name) < 0 || rename(tmp_file_name, input_file_name) < 0)
                {
                    unlink(tmp_file_name);
                    return 0;
                }
            }
        }
	else unlink(tmp_file_name);
    }
    else
    {
	process_includes(input_file_name, specific_file_name, relative_path, output_path,
		         inserting, force_processing, verbose);
    }

    return c == EOF;
}

char* fix_path_sep(char* name)
{
    char *new_name, *ptr;

    ptr = new_name = strdup(name);
    while(*ptr)
    {
        if (*ptr == C_BAD_SEP)
        {
            *ptr = C_SEP;
        }
        ptr++;
    }
    return new_name;
}

int main(int argc, char **argv)
{
    int convert_dir = 0, optc;
    int force_overwrite = 0, verbose = 0;
    const char* input_file_name = 0;
    const char* specific_file_name = 0;
    const char* relative_path = 0;
    const char* output_path = 0;

    while((optc = getopt(argc, argv, "axi:o:b:O:fhv")) != EOF)
    {
	switch(optc)
	{
	case 'a':
	case 'x':
	    if (convert_dir) usage();
	    convert_dir = optc;
	break;
	case 'i':
	case 'o':
	    if (specific_file_name) usage();
	    specific_file_name = fix_path_sep(optarg);
	    optc = ((optc == 'i') ? 'a' : 'x');
	    if (convert_dir && convert_dir != optc) usage();
	    convert_dir = optc;
	break;
	case 'O':
	    if (output_path) usage();
	    output_path = fix_path_sep(optarg);
	break;
	case 'b':
	    if (relative_path) usage();
	    relative_path = fix_path_sep(optarg);
	break;
	case 'f':
	    force_overwrite = 1;
	break;
	case 'v':
	    verbose = 1;
	break;
	case 'h':
	    printf(help);
	    exit(0);
	break;
	default:
	    usage();
	}
    }

    if (optind + 1 != argc) usage();
    input_file_name = fix_path_sep(argv[optind]);

    if (!convert_dir) usage();

    if (!process_resources(input_file_name, specific_file_name, relative_path,
			   output_path ? output_path : relative_path,
			   convert_dir == 'a', force_overwrite, verbose))
    {
	perror("Processing failed");
	exit(1);
    }

    return 0;
}
