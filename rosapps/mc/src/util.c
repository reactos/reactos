/* Various utilities
   Copyright (C) 1994, 1995, 1996 the Free Software Foundation.
   Written 1994, 1995, 1996 by:
   Miguel de Icaza, Janne Kukonlehto, Dugan Porter,
   Jakub Jelinek, Mauricio Plaza.

   The file_date routine is mostly from GNU's fileutils package,
   written by Richard Stallman and David MacKenzie.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <config.h>
#include <stdio.h>
#if defined(__os2__)            /* OS/2 need io.h! .ado */
#    include <io.h>
#endif
#include <stdlib.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>
#include <signal.h>		/* my_system */
#include <limits.h>		/* INT_MAX */
#ifndef SCO_FLAVOR
#if defined (__MINGW32__) || defined(_MSC_VER)
#	include <sys/time.h___>
#else
#	include <sys/time.h>	/* alex: sys/select.h defines struct timeval */
#endif
#endif /* SCO_FLAVOR */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <errno.h>		/* my_system */
#ifdef SCO_FLAVOR
#	include <sys/timeb.h>	/* alex: for struct timeb, used in time.h */
#endif /* SCO_FLAVOR */
#include <time.h>
#ifndef OS2_NT
#   include <pwd.h>
#   include <grp.h>
#endif
#include <string.h>
#include <ctype.h>
#ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif

#ifdef __linux__
#    if defined(__GLIBC__) && (__GLIBC__ < 2)
#        include <linux/termios.h>	/* This is needed for TIOCLINUX */
#    else
#        include <termios.h>
#    endif
#  include <sys/ioctl.h>
#endif

#include "fs.h"
#include "mountlist.h"

/* From dialog.h (not wanting to include it as
   it requires including a lot of other files, too) */
int message (int error, char *header, char *text, ...);

#include "mad.h"
#if defined(HAVE_RX_H) && defined(HAVE_REGCOMP)
#include <rx.h>
#else
#include "regex.h"
#endif
#include "util.h"
#include "global.h"
#include "profile.h"
#include "user.h"		/* expand_format */
#include "../vfs/vfs.h"

/* "$Id: util.c,v 1.1 2001/12/30 09:55:20 sedwards Exp $" */

char app_text [] = "Midnight-Commander";

int easy_patterns = 1;
int align_extensions = 1;
int tilde_trunc = 1;

struct mount_entry *mount_list = NULL;

#ifndef HAVE_STRDUP
char *strdup (const char *s)
{
    char *t = malloc (strlen (s)+1);
    strcpy (t, s);
    return t;
}
#endif

int is_printable (int c)
{
    static const unsigned char xterm_printable[] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
        1,1,1,1,0,0,1,1,0,1,1,1,1,0,0,0,0,1,1,1,1,1,0,0,0,1,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
    };

    extern int xterm_flag;
    extern int eight_bit_clean;
    extern int full_eight_bits;

    c &= 0xff;
    if (eight_bit_clean){
        if (full_eight_bits){
	    if (xterm_flag)
	        return xterm_printable [c];
	    else
	        return (c > 31 && c != 127);
	} else
	    return ((c >31 && c < 127) || c >= 160);
    } else
	return (c > 31 && c < 127);
}

/* Returns the message dimensions (lines and columns) */
int msglen (char *text, int *lines)
{
    int max = 0;
    int line_len = 0;
    
    for (*lines = 1;*text; text++){
	if (*text == '\n'){
	    line_len = 0;
	    (*lines)++;
	} else {
	    line_len++;
	    if (line_len > max)
		max = line_len;
	}
    }
    return max;
}

char *trim (char *s, char *d, int len)
{
    int source_len = strlen (s);
    
    if (source_len > len){
	strcpy (d, s+(source_len-len));
	d [0] = '.';
	d [1] = '.';
	d [2] = '.';
    } else
	strcpy (d, s);
    return d;
}

char *
name_quote (const char *s, int quote_percent)
{
    char *ret, *d;
    
    d = ret = xmalloc (strlen (s)*2 + 2 + 1, "quote_name");
    if (*s == '-') {
        *d++ = '.';
        *d++ = '/';
    }

    for (; *s; s++, d++) {
	switch (*s)
	{
	    case '%':
		if (quote_percent)
		    *d++ = '%';
		break;
	    case '\'':
	    case '\\':
	    case '\r':
	    case '\n':
	    case '\t':
	    case '$':
	    case '?':
	    case '*':
	    case '(':
	    case ')':
	    case '[':
	    case ']':
	    case '"':
	    case '!':
	    case '&':
	    case '#':
	    case '`':
	    case ' ':
		*d++ = '\\';
	}
	*d = *s;
    }
    *d = '\0';
    return ret;
}

char *
fake_name_quote (const char *s, int quote_percent)
{
    return strdup (s);
}

/* If passed an empty txt (this usually means that there is an error)
 * in the upper layers, we return "/"
 */
char *name_trunc (char *txt, int trunc_len)
{
    static char x [MC_MAXPATHLEN+MC_MAXPATHLEN];
    int    txt_len;
    char *p;

    if (!txt)
	txt = PATH_SEP_STR;
    
    if (trunc_len > sizeof (x)-1){
	fprintf (stderr, _("name_trunc: too big"));
	trunc_len = sizeof (x)-1;
    }
    txt_len = strlen (txt);
    if (txt_len <= trunc_len)
	strcpy (x, txt);
    else if (tilde_trunc){
	int y = trunc_len % 2;
	strncpy (x, txt, (trunc_len/2)+y);
	strncpy (x+(trunc_len/2)+y, txt+txt_len-(trunc_len/2), trunc_len/2);
	x [(trunc_len/2)+y] = '~';
    } else {
	strncpy (x, txt, trunc_len-1);
	x [trunc_len-1] = '>';
    }
    x [trunc_len] = 0;
    for (p = x; *p; p++)
        if (!is_printable (*p))
            *p = '?';
    return x;
}

char *size_trunc (long int size)
{
    static char x [30];
    long int divisor = 1;
    char *xtra = "";
    
    if (size > 999999999L){
	divisor = 1024;
	xtra = "kb";
	if (size/divisor > 999999999L){
	    divisor = 1024*1024;
	    xtra = "Mb";
	}
    }
    sprintf (x, "%ld%s", (size/divisor), xtra);
    return x;
}

char *size_trunc_sep (long int size)
{
    static char x [60];
    int  count;
    char *p, *d, *y;

    p = y = size_trunc (size);
    p += strlen (p) - 1;
    d = x + sizeof (x) - 1;
    *d-- = 0;
    while (p >= y && isalpha (*p))
	*d-- = *p--;
    for (count = 0; p >= y; count++){
	if (count == 3){
	    *d-- = ',';
	    count = 0;
	}
	*d-- = *p--;
    }
    d++;
    if (*d == ',')
	d++;
    return d;
}

int is_exe (mode_t mode)
{
    if ((S_IXUSR & mode) || (S_IXGRP & mode) || (S_IXOTH & mode))
	return 1;
    return 0;
}

#define ismode(n,m) ((n & m) == m)

char *string_perm (mode_t mode_bits)
{
    static char mode [11];

    strcpy (mode, "----------");
    if (ismode (mode_bits, S_IFDIR)) mode [0] = 'd';
#ifdef S_IFSOCK
    if (ismode (mode_bits, S_IFSOCK)) mode [0] = 's';
#endif
    if (ismode (mode_bits, S_IXOTH)) mode [9] = 'x';
    if (ismode (mode_bits, S_IWOTH)) mode [8] = 'w';
    if (ismode (mode_bits, S_IROTH)) mode [7] = 'r';
    if (ismode (mode_bits, S_IXGRP)) mode [6] = 'x';
    if (ismode (mode_bits, S_IWGRP)) mode [5] = 'w';
    if (ismode (mode_bits, S_IRGRP)) mode [4] = 'r';
    if (ismode (mode_bits, S_IXUSR)) mode [3] = 'x';
    if (ismode (mode_bits, S_IWUSR)) mode [2] = 'w';
    if (ismode (mode_bits, S_IRUSR)) mode [1] = 'r';
#ifndef OS2_NT
    if (ismode (mode_bits, S_ISUID)) mode [3] = (mode [3] == 'x') ? 's' : 'S';
    if (ismode (mode_bits, S_ISGID)) mode [6] = (mode [6] == 'x') ? 's' : 'S';
    if (ismode (mode_bits, S_IFCHR)) mode [0] = 'c';
    if (ismode (mode_bits, S_IFBLK)) mode [0] = 'b';
    if (ismode (mode_bits, S_ISVTX)) mode [9] = (mode [9] == 'x') ? 't' : 'T';
    if (ismode (mode_bits, S_IFLNK)) mode [0] = 'l';
    if (ismode (mode_bits, S_IFIFO)) mode [0] = 's';
#endif
    return mode;
}

static char *
strip_password (char *path)
{
    char *at, *inner_colon, *dir;
    
    if ((dir = strchr (path, PATH_SEP)) != NULL)
	*dir = '\0';
    /* search for any possible user */
    at = strchr (path, '@');

    /* We have a username */
    if (at) {
        *at = 0;
        inner_colon = strchr (path, ':');
	*at = '@';
        if (inner_colon)
            strcpy (inner_colon, at);
    }
    if (dir)
	*dir = PATH_SEP;
    return (path);
}

char *strip_home_and_password(char *dir)
{
    static char newdir [MC_MAXPATHLEN], *p, *q;

    if (home_dir && !strncmp (dir, home_dir, strlen (home_dir))){
	newdir [0] = '~';
	strcpy (&newdir [1], &dir [strlen (home_dir)]);
	return newdir;
    } 
#ifdef USE_NETCODE    
    else if (!strncmp (dir, "ftp://", 6)) {
	strip_password (strcpy (newdir, dir) + 6);
        if ((p = strchr (newdir + 6, PATH_SEP)) != NULL) {
            *p = 0;
	    q = ftpfs_gethome (newdir);
	    *p = PATH_SEP;
            if (q != 0 && strcmp (q, PATH_SEP_STR) && !strncmp (p, q, strlen (q) - 1)) {
                strcpy (p, "/~");
                strcat (newdir, p + strlen (q) - 1);
            }
        }
        return newdir;
    } else if (!strncmp (dir, "mc:", 3)) {
        char  *pth;
	strcpy (newdir, dir);
	if (newdir[3] == '/' && newdir[4] == '/') { 
	    pth = newdir + 5;
    	    strip_password ( newdir + 5);
	} else { 
	    pth = newdir + 3;
	    strip_password (newdir + 3);
	}
        if ((p = strchr (pth, PATH_SEP)) != NULL) {
            *p = 0;
	    q = mcfs_gethome (newdir);
            *p = PATH_SEP;
            if (q != NULL ) { 
		if (strcmp (q, PATH_SEP_STR) && !strncmp (p, q, strlen (q) - 1)) {
                   strcpy (p, "/~");
                   strcat (newdir, p + strlen (q) - 1);
		}
                free (q);
            }	    
        }
	return (newdir);
    }
#endif    
    return dir;
}

static char *maybe_start_group (char *d, int do_group, int *was_wildcard)
{
    if (!do_group)
	return d;
    if (*was_wildcard)
	return d;
    *was_wildcard = 1;
    *d++ = '\\';
    *d++ = '(';
    return d;
}

static char *maybe_end_group (char *d, int do_group, int *was_wildcard)
{
    if (!do_group)
	return d;
    if (!*was_wildcard)
	return d;
    *was_wildcard = 0;
    *d++ = '\\';
    *d++ = ')';
    return d;
}

/* If shell patterns are on converts a shell pattern to a regular
   expression. Called by regexp_match and mask_rename. */
/* Shouldn't we support [a-fw] type wildcards as well ?? */
char *convert_pattern (char *pattern, int match_type, int do_group)
{
    char *s, *d;
    char *new_pattern;
    int was_wildcard = 0;

    if (easy_patterns){
	new_pattern = malloc (MC_MAXPATHLEN);
	d = new_pattern;
	if (match_type == match_file)
	    *d++ = '^';
	for (s = pattern; *s; s++, d++){
	    switch (*s){
	    case '*':
		d = maybe_start_group (d, do_group, &was_wildcard);
		*d++ = '.';
		*d   = '*';
		break;
		
	    case '?':
		d = maybe_start_group (d, do_group, &was_wildcard);
		*d = '.';
		break;
		
	    case '.':
		d = maybe_end_group (d, do_group, &was_wildcard);
		*d++ = '\\';
		*d   = '.';
		break;

	    default:
		d = maybe_end_group (d, do_group, &was_wildcard);
		*d = *s;
		break;
	    }
	}
	d = maybe_end_group (d, do_group, &was_wildcard);
	if (match_type == match_file)
	    *d++ = '$';
	*d = 0;
	return new_pattern;
    } else
	return strdup (pattern);
}

int regexp_match (char *pattern, char *string, int match_type)
{
    static regex_t r;
    static char *old_pattern = NULL;
    static int old_type;
    int    rval;

    if (!old_pattern || STRCOMP (old_pattern, pattern) || old_type != match_type){
	if (old_pattern){
	    regfree (&r);
	    free (old_pattern);
	}
	pattern = convert_pattern (pattern, match_type, 0);
	if (regcomp (&r, pattern, REG_EXTENDED|REG_NOSUB|MC_ARCH_FLAGS)) {
	    free (pattern);
	    return -1;
	}
	old_pattern = pattern;
	old_type = match_type;
    }
    rval = !regexec (&r, string, 0, NULL, 0);
    return rval;
}

char *extension (char *filename)
{
    char *d;

    if (!strlen (filename))
	return "";
    
    d = filename + strlen (filename) - 1;
    for (;d >= filename; d--){
	if (*d == '.')
	    return d+1;
    }
    return "";
}

/* This routine uses the fact that x is at most 14 chars or so */
char *split_extension (char *x, int pad)
{
    return x;

    /* Buggy code 
    if (!align_extensions)
	return x;

    if (strlen (x) >= pad)
	return x;
    
    if ((ext = extension (x)) == x || *ext == 0)
	return x;

    strcpy (xbuf, x);
    for (i = strlen (x); i < pad; i++)
	xbuf [i] = ' ';
    xbuf [pad] = 0;

    l = strlen (ext);
    for (i = 0; i < l; i++)
	xbuf [pad-i] = *(ext+l-i-1);
    for (i = xbuf + (ext - x); i < 
    return xbuf; */
}

#ifndef HAVE_MAD
void *do_xmalloc (int size)
{
    void *m = malloc (size);

    if (!m){
	fprintf (stderr, "memory exhausted\n");
	exit (1);
    }
    return m;
}
#endif /* HAVE_MAD */

int get_int (char *file, char *key, int def)
{
    return GetPrivateProfileInt (app_text, key, def, file);
}

int set_int (char *file, char *key, int value)
{
    char buffer [30];

    sprintf (buffer,  "%d", value);
    return WritePrivateProfileString (app_text, key, buffer, file);
}

int exist_file (char *name)
{
    return access (name, R_OK) == 0;
}

char *load_file (char *filename)
{
    FILE *data_file;
    struct stat s;
    char *data;
    long read_size,i;
    
    if (stat (filename, &s) != 0){
	return 0;
    }
#ifdef OS2_NT
    if ((data_file = fopen (filename, "rt")) == NULL){
#else
    if ((data_file = fopen (filename, "r")) == NULL){
#endif
	return 0;
    }
    data = (char *) xmalloc (s.st_size+1, "util, load_file");
#ifdef OS2_NT
    memset(data,0,s.st_size+1);
#endif
    read_size = fread (data, 1, s.st_size, data_file);
    data [read_size] = 0;
    fclose (data_file);

    if (read_size > 0){
	return data;
    }
    else {
	free (data);
	return 0;
    }
}

char *file_date (time_t when)
{
    static char timebuf [40];
    time_t current_time = time ((time_t) 0);

#ifdef OS2_NT
    char   *p;

    p = ctime (&when);
    strcpy (timebuf, p ? p : "-----");
#else
    strcpy (timebuf, ctime (&when));
#endif
    if (current_time > when + 6L * 30L * 24L * 60L * 60L /* Old. */
	|| current_time < when - 60L * 60L) /* In the future. */
    {
	/* The file is fairly old or in the future.
	   POSIX says the cutoff is 6 months old;
	   approximate this by 6*30 days.
	   Allow a 1 hour slop factor for what is considered "the future",
	   to allow for NFS server/client clock disagreement.
	   Show the year instead of the time of day.  */
	strcpy (timebuf + 11, timebuf + 19);
    }
    timebuf[16] = 0;
    return &timebuf [4];
}

/* Like file_date, but packs the data to fit in 10 columns */
char *file_date_pck (time_t when)
{
    /* FIXME: Should return only 10 chars, not 14 */
    return file_date (when);
}

char *extract_line (char *s, char *top)
{
    static char tmp_line [500];
    char *t = tmp_line;
    
    while (*s && *s != '\n' && (t - tmp_line) < sizeof (tmp_line)-1 && s < top)
	*t++ = *s++;
    *t = 0;
    return tmp_line;
}

/* FIXME: I should write a faster version of this (Aho-Corasick stuff) */
char * _icase_search (char *text, char *data, int *lng)
{
    char *d = text;
    char *e = data;
    int dlng = 0;

    if (lng)
	*lng = 0;
    for (;*e; e++) {
	while (*(e+1) == '\b' && *(e+2)) {
	    e += 2;
	    dlng += 2;
	}
	if (toupper((unsigned char) *d) == toupper((unsigned char) *e))
	    d++;
	else {
	    e -= d - text;
	    d = text;
	    dlng = 0;
	}
	if (!*d) {
	    if (lng)
		*lng = strlen (text) + dlng;
	    return e+1;
	}
    }
    return 0;
}

/* The basename routine */
char *x_basename (char *s)
{
    char  *where;
    return ((where = strrchr (s, PATH_SEP)))? where + 1 : s;
}

char *get_full_name (char *dir, char *file)
{
    int i;
    char *d = malloc (strlen (dir) + strlen (file) + 2);

    strcpy (d, dir);
    i = strlen (dir);
    if (dir [i - 1] != PATH_SEP || dir [i] != 0)
	strcat (d, PATH_SEP_STR);
    file = x_basename (file);
    strcat (d, file);
    return d;
}

void my_putenv (char *name, char *data)
{
    char *full;

    full = xmalloc (strlen (name) + strlen (data) + 2, "util, my_putenv");
    sprintf (full, "%s=%s", name, data);
    putenv (full);
    /* WARNING: NEVER FREE THE full VARIABLE!!!!!!!!!!!!!!!!!!!!!!!! */
    /* It is used by putenv. Freeing it will corrupt the environment */
}

#if 0
static void my_putenv_expand (char *name, char macro_code)
{
    char *data;

    data = expand_format (macro_code);
    my_putenv (name, data);
    free (data);
}

/* Puts some status information in to the environment so that
   processes to be executed can access it. */
static void prepare_environment (void)
{
    my_putenv_expand ("MC_CURRENT_DIR", 'd');
    my_putenv_expand ("MC_OTHER_DIR", 'D');
    my_putenv_expand ("MC_CURRENT_FILE", 'f');
    my_putenv_expand ("MC_OTHER_FILE", 'F');
    my_putenv_expand ("MC_CURRENT_TAGGED", 't');
    my_putenv_expand ("MC_OTHER_TAGGED", 'T');
    /* MC_CONTROL_FILE has been added to environment on startup */
}
#endif

char *unix_error_string (int error_num)
{
    static char buffer [256];
    char *error_msg;
	
#ifdef HAVE_STRERROR
    error_msg = strerror (error_num);
#else
    extern int sys_nerr;
    extern char *sys_errlist [];
    if ((0 <= error_num) && (error_num < sys_nerr))
	error_msg = sys_errlist[error_num];
    else
	error_msg = "strange errno";
#endif
    sprintf (buffer, "%s (%d)", error_msg, error_num);
    return buffer;
}

char *copy_strings (const char *first,...)
{
    va_list ap;
    int len;
    char *data, *result;

    if (!first)
	return 0;
    
    len = strlen (first);
    va_start (ap, first);

    while ((data = va_arg (ap, char *))!=0)
	len += strlen (data);

    len++;

    result = xmalloc (len, "copy_strings");
    va_end (ap);
    va_start (ap, first);
    strcpy (result, first);
    while ((data = va_arg (ap, char *)) != 0)
	strcat (result, data);
    va_end (ap);

    return result;
}
	
long blocks2kilos (int blocks, int bsize)
{
    if (bsize > 1024){
	return blocks * (bsize / 1024);
    } else if (bsize < 1024){
	return blocks / (1024 /bsize);
    } else
	return blocks;
}

void init_my_statfs (void)
{
#ifndef NO_INFOMOUNT
    mount_list = read_filesystem_list (1, 1);
#endif
}

char *skip_separators (char *s)
{
    for (;*s; s++)
	if (*s != ' ' && *s != '\t' && *s != ',')
	    break;
    return s;
}

char *skip_numbers (char *s)
{
    for (;*s; s++)
	if (!isdigit (*s))
	    break;
    return s;
}

/* Remove all control sequences from the argument string.  We define
 * "control sequence", in a sort of pidgin BNF, as follows:
 *
 * control-seq = Esc non-'['
 *	       | Esc '[' (0 or more digits or ';' or '?') (any other char)
 *
 * This scheme works for all the terminals described in my termcap /
 * terminfo databases, except the Hewlett-Packard 70092 and some Wyse
 * terminals.  If I hear from a single person who uses such a terminal
 * with MC, I'll be glad to add support for it.  (Dugan)
 */

char *strip_ctrl_codes (char *s)
{
    int i;  /* Current length of the string's correct (stripped) prefix */
    int j;  /* Number of control characters we have skipped so far */

    if (!s)
	return 0;
    
    for (i = 0, j = 0; s [i+j]; ++i)
	if (s [i+j] != ESC_CHAR){
	    if (j)
		s [i] = s [i+j];
	} else {
	    ++j;
	    if (s [i+j++] == '[')
		while (strchr ("0123456789;?", s [i+j++]))
		    /* Skip the control sequence's arguments */ ;
	    --i;
	}
    s[i] = 0;
    return s;
}

#ifndef HAVE_STRCASECMP
/* At least one version of HP/UX lacks this */
/* Assumes ASCII encoding */
int strcasecmp (const char *s, const char *d)
{
    register signed int result;

    while (1){
	if (result = (0x20 | *s) - (0x20 | *d))
	    break;
	if (!*s)
	    return 0;
	s++;
	d++;
    }
    return result;
}
#endif /* HAVE_STRCASECMP */

/* getwd is better than getcwd, the later uses a popen ("pwd"); */
char *get_current_wd (char *buffer, int size)
{
    char *p;

#ifdef HAVE_GETWD
    p = (char *) getwd (buffer);
#else
    p = getcwd (buffer, size);
#endif
    return p;
}

#define CHECK(x) if (x == -1) return 0;

long get_small_endian_long (int fd)
{
    unsigned char a, b, c, d;

    /* It needs to be read one byte at the time to avoid endianess
       portability problems */
    CHECK (mc_read (fd, &a, 1));
    CHECK (mc_read (fd, &b, 1));
    CHECK (mc_read (fd, &c, 1));
    CHECK (mc_read (fd, &d, 1));
    return (d << 24) | (c << 16) | (b << 8) | a;
}

/* This function returns 0 if the file is not in gunzip format  */
/* or how much memory must be allocated to load the gziped file */
/* Warning: this function moves the current file pointer */
long int is_gunzipable (int fd, int *type)
{
    unsigned char magic [4];
	
    *type = ISGUNZIPABLE_GUNZIP;
	
    /* Read the magic signature */
    CHECK (mc_read (fd, &magic [0], 1));
    CHECK (mc_read (fd, &magic [1], 1));
    CHECK (mc_read (fd, &magic [2], 1));
    CHECK (mc_read (fd, &magic [3], 1));
	
    /* GZIP_MAGIC and OLD_GZIP_MAGIC */
    if (magic [0] == 037 && (magic [1] == 0213 || magic [1] == 0236)){
	/* Read the uncompressed size of the file */
	mc_lseek (fd, -4, SEEK_END);
	return get_small_endian_long (fd);
    }

    /* PKZIP_MAGIC */
    if (magic [0] == 0120 && magic [1] == 0113 && magic [2] == 003 && magic [3] == 004){
	/* Read compression type */
	mc_lseek (fd, 8, SEEK_SET);
	CHECK (mc_read (fd, &magic [0], 1));
	CHECK (mc_read (fd, &magic [1], 1));
	
	/* Gzip can handle only deflated (8) or stored (0) files */
	if ((magic [0] != 8 && magic [0] != 0) || magic [1] != 0)
	     return 0;
        /* Read the uncompressed size of the first file in the archive */
	mc_lseek (fd, 22, SEEK_SET);
	return get_small_endian_long (fd);
    }

    /* PACK_MAGIC and LZH_MAGIC and compress magic */
    if (magic [0] == 037 && (magic [1] ==  036 || magic [1] == 0240 || magic [1] == 0235)){
	 /* In case the file is packed, sco lzhed or compress_magic, the */
	 /* program guesses that the uncompressed size is (at most) four */
	 /* times the length of the compressed size, if the compression  */
	 /* ratio is more than 4:1 the end of the file is not displayed  */
	 return 4*mc_lseek (fd, 0, SEEK_END);
    }

    /* BZIP and BZIP2 files */
    if ((magic[0] == 'B') && (magic[1] == 'Z') &&
	(magic [3] >= '1') && (magic [3] <= '9')){
            switch (magic[2]) {
                case '0':
                    *type = ISGUNZIPABLE_BZIP;
                    return 5*mc_lseek (fd, 0, SEEK_END);
                case 'h': 
	            *type = ISGUNZIPABLE_BZIP2;
	            return 5*mc_lseek (fd, 0, SEEK_END);
            }
    }
    return 0;
}

char *
decompress_command (int type)
{
	switch (type){
	case ISGUNZIPABLE_GUNZIP:
		return "gzip -cdf";
		
	case ISGUNZIPABLE_BZIP:
		return "bzip -d";
		
	case ISGUNZIPABLE_BZIP2:
		return "bzip2 -dc";
	}
	/* Should never reach this place */
	fprintf (stderr, "Fatal: decompress_command called with an unknown argument\n");
	return 0;
}

void
decompress_command_and_arg (int type, char **cmd, char **flags)
{
	switch (type){
	case ISGUNZIPABLE_GUNZIP:
		*cmd   = "gzip";
		*flags = "-cdf";
		return;

	case ISGUNZIPABLE_BZIP:
		*cmd   = "bzip";
		*flags = "-d";
		return;

		
	case ISGUNZIPABLE_BZIP2:
		*cmd   = "bzip2";
		*flags = "-dc";
		return;
	}
	*cmd   = 0;
	*flags = 0;
	
	/* Should never reach this place */
	fprintf (stderr, "Fatal: decompress_command called with an unknown argument\n");
}

/* Hooks */
void add_hook (Hook **hook_list, void (*hook_fn)(void *), void *data)
{
    Hook *new_hook = xmalloc (sizeof (Hook), "add_hook");

    new_hook->hook_fn = hook_fn;
    new_hook->next    = *hook_list;
    new_hook->hook_data = data;
      
    *hook_list = new_hook;
}

void execute_hooks (Hook *hook_list)
{
    Hook *new_hook = 0;
    Hook *p;

    /* We copy the hook list first so tahat we let the hook
     * function call delete_hook
     */
    
    while (hook_list){
	add_hook (&new_hook, hook_list->hook_fn, hook_list->hook_data);
	hook_list = hook_list->next;
    }
    p = new_hook;
    
    while (new_hook){
	(*new_hook->hook_fn)(new_hook->hook_data);
	new_hook = new_hook->next;
    }
    
    for (hook_list = p; hook_list;){
	p = hook_list;
	hook_list = hook_list->next;
	free (p);
    }
}

void delete_hook (Hook **hook_list, void (*hook_fn)(void *))
{
    Hook *current, *new_list, *next;

    new_list = 0;
    
    for (current = *hook_list; current; current = next){
	next = current->next;
	if (current->hook_fn == hook_fn)
	    free (current);
	else
	    add_hook (&new_list, current->hook_fn, current->hook_data);
    }
    *hook_list = new_list;
}

int hook_present (Hook *hook_list, void (*hook_fn)(void *))
{
    Hook *p;
    
    for (p = hook_list; p; p = p->next)
	if (p->hook_fn == hook_fn)
	    return 1;
    return 0;
}

void wipe_password (char *passwd)
{
    char *p = passwd;
    
    for (;*p ; p++)
        *p = 0;
    free (passwd);
}

/* Convert "\E" -> esc character and ^x to control-x key and ^^ to ^ key */
/* Returns a newly allocated string */
char *convert_controls (char *s)
{
    char *valcopy = strdup (s);
    char *p, *q;

    /* Parse the escape special character */
    for (p = s, q = valcopy; *p;){
	if (*p == '\\'){
	    p++;
	    if ((*p == 'e') || (*p == 'E')){
		p++;
		*q++ = ESC_CHAR;
	    }
	} else {
	    if (*p == '^'){
		p++;
		if (*p == '^')
		    *q++ = *p++;
		else {
		    *p = (*p | 0x20);
		    if (*p >= 'a' && *p <= 'z') {
		        *q++ = *p++ - 'a' + 1;
		    } else
		        p++;
		}
	    } else
		*q++ = *p++;
	}
    }
    *q++ = 0;
    return valcopy;
}

/* Reverse the string */
char *reverse_string (char *string)
{
    int len = strlen (string);
    int i;
    const int steps = len/2;
    
    for (i = 0; i < steps; i++){
	char c = string [i];
    
	string [i] = string [len-i-1];
	string [len-i-1] = c;
    }
    return string;
}

char *resolve_symlinks (char *path)
{
    char *buf, *buf2, *p, *q, *r, c;
    int len;
    struct stat mybuf;
    
    if (*path != PATH_SEP)
        return NULL;
    r = buf = xmalloc (MC_MAXPATHLEN, "resolve symlinks");
    buf2 = xmalloc (MC_MAXPATHLEN, "resolve symlinks"); 
    *r++ = PATH_SEP;
    *r = 0;
    p = path;
    for (;;) {
	q = strchr (p + 1, PATH_SEP);
	if (!q) {
	    q = strchr (p + 1, 0);
	    if (q == p + 1)
	        break;
	}
	c = *q;
	*q = 0;
	if (mc_lstat (path, &mybuf) < 0) {
	    free (buf);
	    free (buf2);
	    *q = c;
	    return NULL;
	}
	if (!S_ISLNK (mybuf.st_mode))
	    strcpy (r, p + 1);
	else {
	    len = mc_readlink (path, buf2, MC_MAXPATHLEN);
	    if (len < 0) {
		free (buf);
		free (buf2);
		*q = c;
		return NULL;
	    }
	    buf2 [len] = 0;
	    if (*buf2 == PATH_SEP)
		strcpy (buf, buf2);
	    else
		strcpy (r, buf2);
	}
	canonicalize_pathname (buf);
	r = strchr (buf, 0);
	if (!*r || *(r - 1) != PATH_SEP) {
	    *r++ = PATH_SEP;
	    *r = 0;
	}
	*q = c;
	p = q;
	if (!c)
	    break;
    }
    if (!*buf)
	strcpy (buf, PATH_SEP_STR);
    else if (*(r - 1) == PATH_SEP && r != buf + 1)
	*(r - 1) = 0;
    free (buf2);
    return buf;
}

/* Finds out a relative path from first to second, i.e. goes as many ..
 * as needed up in first and then goes down using second */
char *diff_two_paths (char *first, char *second) 
{
    char *p, *q, *r, *s, *buf = 0;
    int i, j, prevlen = -1, currlen;
    
    first = resolve_symlinks (first);
    if (first == NULL)
        return NULL;
    for (j = 0; j < 2; j++) {
	p = first;
	if (j) {
	    second = resolve_symlinks (second);
	    if (second == NULL) {
		free (first);
	        return buf;
	    }
	}
	q = second;
	for (;;) {
	    r = strchr (p, PATH_SEP);
	    s = strchr (q, PATH_SEP);
	    if (!r || !s)
	      break;
	    *r = 0; *s = 0;
	    if (strcmp (p, q)) {
		*r = PATH_SEP; *s = PATH_SEP;
		break;
	    } else {
		*r = PATH_SEP; *s = PATH_SEP;
	    }
	    p = r + 1;
	    q = s + 1;
	}
	p--;
	for (i = 0; (p = strchr (p + 1, PATH_SEP)) != NULL; i++);
	currlen = (i + 1) * 3 + strlen (q) + 1;
	if (j) {
	    if (currlen < prevlen)
	        free (buf);
	    else {
		free (first);
		free (second);
		return buf;
	    }
	}
	p = buf = xmalloc (currlen, "diff 2 paths");
	prevlen = currlen;
	for (; i >= 0; i--, p += 3)
	  strcpy (p, "../");
	strcpy (p, q);
    }
    free (first);
    free (second);
    return buf;
}

#ifndef HAVE_TRUNCATE
/* On SCO and Windows NT systems */
int my_ftruncate (int fd, long size)
{
#ifdef OS2_NT
    if(_chsize(fd, size))
	return -1;
    else 
	return 0;
#else
    struct flock lk;
    
    lk.l_whence = 0;
    lk.l_start = size;
    lk.l_len = 0;
    
    return fcntl (fd, F_FREESP, &lk);
#endif
}

int truncate (const char *path, long size)
{
    int fd;
    int res;
    
    fd = open (path, O_RDWR, 0);
    if (fd < 0)
	return fd;
    res = my_ftruncate (fd, size);
    if (res < 0)
	return res;
    close (fd);
    return 0;

}

#endif

char *
concat_dir_and_file (const char *dir, const char *file)
{
    int l = strlen (dir);

    if (dir [l-1] == PATH_SEP)
	return copy_strings (dir, file, 0);
    else
	return copy_strings (dir, PATH_SEP_STR, file, 0);
}
