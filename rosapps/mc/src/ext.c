/* Extension dependent execution.
   Copyright (C) 1994, 1995 The Free Software Foundation
   
   Written by: 1995 Jakub Jelinek
               1994 Miguel de Icaza
   
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
#include <ctype.h>
#include <stdlib.h>
#ifdef __os2__
# include <io.h>
#endif

#include "tty.h"
#ifdef HAVE_UNISTD_H
#    include <unistd.h>
#endif
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include "mad.h"
#include "user.h"
#include "main.h"
#include "fs.h"
#include "util.h"
#include "dialog.h"
#include "global.h"
#include "ext.h"
#include "view.h"
#include "main.h"
#include "../vfs/vfs.h"
#include "x.h"

#include "cons.saver.h"
#include "layout.h"
#ifdef SCO_FLAVOR
#include <sys/wait.h>
#endif /* SCO_FLAVOR */

/* "$Id: ext.c,v 1.1 2001/12/30 09:55:26 sedwards Exp $" */

/* If set, we execute the file command to check the file type */
int use_file_to_check_type = 1;

/* This variable points to a copy of the mc.ext file in memory
 * With this we avoid loading/parsing the file each time we 
 * need it 
 */
static char *data = NULL;

#ifdef OS2_NT

__declspec(dllimport) __stdcall unsigned GetTempPathA(unsigned,char*);

static char tmpcmdfilename[255];
char *gettmpcmdname(){
    int i,fd;
    char TmpPath[255];
    memset(TmpPath,0,255);
    memset(tmpcmdfilename,0,255);
    GetTempPathA(255,TmpPath);
    for(i=0;i<32000;++i){
	sprintf(tmpcmdfilename,"%stmp%d.bat",TmpPath,i);
	if((fd=_open(tmpcmdfilename,_O_RDONLY)) != -1)
	    _close(fd);
	else if (errno == ENOENT)
	    break;
    }
    return tmpcmdfilename;
} 
#endif

void
flush_extension_file (void)
{
    if (data){
        free (data);
        data = NULL;
    }
   
}

typedef char *(*quote_func_t)(const char *name, int i);

static char *
quote_block (quote_func_t quote_func, char **quoting_block)
{
	char **p = quoting_block;
	char *result = 0;
	char *tail   = 0;
	int current_len = 0;

	for (p = quoting_block; *p; p++){
		int  temp_len;
		char *temp = quote_func (*p, 0);

		temp_len = strlen (temp);
		current_len += temp_len + 2;
		result = realloc (result, current_len);
		if (!tail)
			tail = result;
		strcpy (tail, temp);
		strcat (tail, " ");
		tail += temp_len + 1;
		free (temp);
	}
	
	return result;
}
	     
static void
exec_extension (char *filename, char *data, char **drops, int *move_dir, int start_line)
{
    char *file_name;
    int  cmd_file_fd;
    FILE *cmd_file;
    int  expand_prefix_found = 0;
    int  parameter_found = 0;
    char prompt [80];
    int  run_view = 0;
    int  def_hex_mode = default_hex_mode, changed_hex_mode = 0;
    int  def_nroff_flag = default_nroff_flag, changed_nroff_flag = 0;
    int  written_nonspace = 0;
    int  is_cd = 0;
    char buffer [1024];
    char *p = 0;
    char *localcopy = NULL;
    int  do_local_copy;
    time_t localmtime = 0;
    struct stat mystat;
    quote_func_t quote_func = name_quote;

    /* Avoid making a local copy if we are doing a cd */
    if (!vfs_file_is_local(filename))
	do_local_copy = 1;
    else
	do_local_copy = 0;
    
    /* Note: this has to be done after the getlocalcopy call,
     * since it uses tmpnam as well
     */
#ifdef OS2_NT
    file_name =  strdup (gettmpcmdname ());
#else
    file_name =  strdup (tmpnam (NULL));
#endif    
    if ((cmd_file_fd = open (file_name, O_RDWR | O_CREAT | O_TRUNC | O_EXCL, 0600)) == -1){
	message (1, MSG_ERROR, _(" Can't create temporary command file \n %s "),
		 unix_error_string (errno));
	return;
    }
    cmd_file = fdopen (cmd_file_fd, "w");
#ifdef OS2_NT
    fprintf (cmd_file, "REM #!%s\n", shell);
#else
    fprintf (cmd_file, "#!%s\n", shell);
#endif
    prompt [0] = 0;
    for (;*data && *data != '\n'; data++){
	if (parameter_found){
	    if (*data == '}'){
		char *parameter;
		parameter_found = 0;
		parameter = input_dialog (_(" Parameter "), prompt, "");
		if (!parameter){
		    /* User canceled */
		    fclose (cmd_file);
		    unlink (file_name);
		    if (localcopy) {
		        mc_ungetlocalcopy (filename, localcopy, 0);
		    }
		    free (file_name);
		    return;
		}
		fputs (parameter, cmd_file);
		written_nonspace = 1;
		free (parameter);
	    } else {
		int len = strlen (prompt);

		if (len < sizeof (prompt) - 1){
		    prompt [len] = *data;
		    prompt [len+1] = 0;
		}
	    }
	} else if (expand_prefix_found){
	    expand_prefix_found = 0;
	    if (*data == '{')
		parameter_found = 1;
	    else {
	    	int i = check_format_view (data);
		char *v;
		
	    	if (i){
	    	    data += i - 1;
	    	    run_view = 1;
	    	} else if ((i = check_format_cd (data)) > 0) {
	    	    is_cd = 1;
		    quote_func = fake_name_quote;
		    do_local_copy = 0;
	    	    p = buffer;
	    	    data += i - 1;
		} else if ((i = check_format_var (data, &v)) > 0 && v){
		    fputs (v, cmd_file);
		    free (v);
		    data += i;
	        } else {
		    char *text;

		    if (*data == 'f'){
			if (do_local_copy){
			    localcopy = mc_getlocalcopy (filename);
			    if (localcopy == NULL) {
				fclose(cmd_file);
				unlink(file_name);
				free(file_name);
				return;
			    }
			    mc_stat (localcopy, &mystat);
			    localmtime = mystat.st_mtime;
			    text = (*quote_func) (localcopy, 0);
			} else {
			    text = (*quote_func) (filename, 0);
			}
		    } else if (*data == 'q') {
			text = quote_block (quote_func, drops);
		    } else
		        text = expand_format (*data, !is_cd);
		    if (!is_cd)
		        fputs (text, cmd_file);
		    else {
		    	strcpy (p, text);
		    	p = strchr (p, 0);
		    }
		    free (text);
		    written_nonspace = 1;
	        }
	    }
	} else {
	    if (*data == '%')
		expand_prefix_found = 1;
	    else {
	        if (*data != ' ' && *data != '\t')
	            written_nonspace = 1;
	        if (is_cd)
	            *(p++) = *data;
	        else
		    fputc (*data, cmd_file);
	    }
	}
    } /* for */
    fputc ('\n', cmd_file);
    fclose (cmd_file);
    chmod (file_name, S_IRWXU);
    if (run_view){
    	altered_hex_mode = 0;
    	altered_nroff_flag = 0;
    	if (def_hex_mode != default_hex_mode)
    	    changed_hex_mode = 1;
    	if (def_nroff_flag != default_nroff_flag)
    	    changed_nroff_flag = 1;
	
    	/* If we've written whitespace only, then just load filename
	 * into view
	 */
    	if (written_nonspace) 
    	    view (file_name, filename, move_dir, start_line);
    	else
    	    view (0, filename, move_dir, start_line);
    	if (changed_hex_mode && !altered_hex_mode)
    	    default_hex_mode = def_hex_mode;
    	if (changed_nroff_flag && !altered_nroff_flag)
    	    default_nroff_flag = def_nroff_flag;
    	repaint_screen ();
    } else if (is_cd) {
    	char *q;
    	*p = 0;
    	p = buffer;
    	while (*p == ' ' && *p == '\t')
    	    p++;
	    
	/* Search last non-space character. Start search at the end in order
	   not to short filenames containing spaces. */
    	q = p + strlen (p) - 1;
    	while (q >= p && (*q == ' ' || *q == '\t'))
    	    q--;
    	q[1] = 0;
    	do_cd (p, cd_parse_command);
    } else {
	shell_execute (file_name, EXECUTE_INTERNAL | EXECUTE_TEMPFILE);
	if (console_flag)
	{
	    handle_console (CONSOLE_SAVE);
	    if (output_lines && keybar_visible)
	    {
		show_console_contents (output_start_y,
				       LINES-keybar_visible-output_lines-1,
				       LINES-keybar_visible-1);
		
	    }
	}
	
#ifdef OLD_CODE
	if (vfs_current_is_local ())
	    shell_execute (file_name, EXECUTE_INTERNAL);
	else
	    message (1, _(" Warning "), _(" Can't execute commands on a Virtual File System directory "));
#endif
    }
#ifndef PORT_DOES_BACKGROUND_EXEC
    unlink (file_name);
#endif
    if (localcopy) {
        mc_stat (localcopy, &mystat);
        mc_ungetlocalcopy (filename, localcopy, localmtime != mystat.st_mtime);
    }
    free (file_name);
}

#ifdef FILE_L
#   define FILE_CMD "file -L "
#else
#   define FILE_CMD "file "
#endif

/* The second argument is action, i.e. Open, View, Edit, Drop, or NULL if
 * we want regex_command to return a list of all user defined actions.
 * Third argument is space separated list of dropped files (for actions
 * other then Drop it should be NULL);
 *
 * This function returns:
 *
 * If action != NULL, then it returns "Success" (not allocated) if it ran
 * some command or NULL if not.
 *
 * If action == NULL, it returns NULL if there are no user defined commands
 * or an allocated space separated list of user defined Actions.
 *
 * If action == "Icon", we are doing again something special. We return
 * icon name and we set the variable regex_command_title to Title for
 * that icon.
 *
 * If action == "View" then a parameter is checked in the form of "View:%d",
 * if the value for %d exists, then the viewer is started up at that line number.
 */
char *regex_command_title = NULL;
char *regex_command (char *filename, char *action, char **drops, int *move_dir)
{
    char *extension_file;
    char *p, *q, *r, c;
    char *buffer;
    int  file_len = strlen (filename);
    int found = 0;
    char content_string [2048];
    int content_shift = 0;
    char *to_return = NULL;
    int old_patterns;
    struct stat mystat;
    int asked_file;
    int view_at_line_number;
    char *include_target;
    int include_target_len;
    
#ifdef FILE_STDIN
    int file_supports_stdin = 1;
#else
    int file_supports_stdin = 0;
#endif    

    /* Check for the special View:%d parameter */
    if (action && strncmp (action, "View:", 5) == 0){
	view_at_line_number = atoi (action + 5);
	action [4] = 0;
    } else {
	view_at_line_number = 0;
    }
    /* Have we asked file for the file contents? */
    asked_file = 0;

    if (data == NULL) {
        int home_error = 0;

        buffer = concat_dir_and_file (home_dir, MC_USER_EXT);
    	if (exist_file (buffer))
	    extension_file = buffer;
        else
check_stock_mc_ext:
	    extension_file = concat_dir_and_file (mc_home, MC_LIB_EXT);
        if ((data = load_file (extension_file)) == NULL) {
	    free (buffer);
	    return 0;
	}
	if (!strstr (data, "default/")) {
	    if (!strstr (data, "regex/") && !strstr (data, "shell/") &&
	        !strstr (data, "type/")) {
	        free (data);
	        data = NULL;
	        if (extension_file == buffer) {
		    home_error = 1;
		    goto check_stock_mc_ext;            
	        } else {
                    char *msg;
                    char *msg2;
                    msg = copy_strings(" ", mc_home, MC_LIB_EXT, _(" file error"), NULL);
                    msg2 = copy_strings(_("Format of the "), 
                                         mc_home, 
("mc.ext file has changed\n\
with version 3.0. It seems that installation\n\
failed. Please fetch a fresh new copy from the\n\
Midnight Commander package or in case you don't\n\
have any, get it from ftp://ftp.nuclecu.unam.mx."), 0);
	            message (1, msg, msg2);
                    free (msg);
                    free (msg2);
		    free (buffer);
		    return 0;
	        }
	    }
	}
	if (home_error) {
            char *msg;
            char *msg2;
            msg = copy_strings(" ~/", MC_USER_EXT, _(" file error "), NULL);
            msg2 = copy_strings(_("Format of the ~/"), MC_USER_EXT, _(" file has changed\n\
with version 3.0. You may want either to\n\
copy it from "), mc_home, _("mc.ext or use that\n\
file as an example of how to write it.\n\
"), mc_home,  _("mc.ext will be used for this moment."), 0);
	    message (1, msg, msg2);
            free (msg);
            free (msg2);
        }
        free (buffer);
    }
    mc_stat (filename, &mystat);
    
    if (regex_command_title){
	free (regex_command_title);
	regex_command_title = NULL;
    }
    old_patterns = easy_patterns;
    easy_patterns = 0; /* Real regular expressions are needed :) */
    include_target = NULL;
    for (p = data; *p; p++) {
    	for (q = p; *q == ' ' || *q == '\t'; q++)
		;
    	if (*q == '\n' || !*q)
    	    p = q; /* empty line */
    	if (*p == '#') /* comment */
    	    while (*p && *p != '\n')
    	    	p++;
	if (*p == '\n')
	    continue;
	if (!*p)
	    break;
	if (p == q) { /* i.e. starts in the first column, should be
	               * keyword/descNL
	               */
	    if (found && action == NULL) /* We have already accumulated all
	    				  * the user actions 
	    				  */
	        break;
	    found = 0;
	    q = strchr (p, '\n'); 
	    if (q == NULL)
	        q = strchr (p, 0);
	    c = *q;
	    *q = 0;
	    if (include_target){
		if ((strncmp (p, "include/", 8) == 0) &&
		    (strncmp (p+8, include_target, include_target_len) == 0))
		    found = 1;
	    } else if (!strncmp (p, "regex/", 6)) {
	        p += 6;
	        /* Do not transform shell patterns, you can use shell/ for
	         * that
	         */
	        if (regexp_match (p, filename, match_normal))
	            found = 1;
	    } else if (!strncmp (p, "directory/", 10)) {
	        if (S_ISDIR (mystat.st_mode) && regexp_match (p+10, filename, match_normal))
	            found = 1;
	    } else if (!strncmp (p, "shell/", 6)) {
	        p += 6;
	        if (*p == '.') {
	            if (!strncmp (p, filename + file_len - (q - p), 
	                q - p))
	                found = 1;
	        } else {
	            if (q - p == file_len && !strncmp (p, filename, q - p))
	                found = 1;
	        }
	    } else if (!strncmp (p, "type/", 5)) {
		int islocal = vfs_file_is_local (filename);
	        p += 5;
		
	        if (islocal || file_supports_stdin) {
	    	    char *pp;
	    	    int hasread = use_file_to_check_type;

		    if (asked_file || !use_file_to_check_type)
			goto match_file_output;

		    hasread = 0;
	    	    if (islocal) {
			char *tmp = name_quote (filename, 0);
	    	        char *command =
			    copy_strings (FILE_CMD, tmp, NULL);
	    	        FILE *f = popen (command, "r");
	    	    
			free (tmp);
	    	        free (command);
	    	        if (f != NULL) {
	    	            hasread = (fgets (content_string, 2047, f) 
	    	                != NULL);
	    	    	    if (!hasread)
	    	    	        content_string [0] = 0;
	    	    	    pclose (f);
#ifdef SCO_FLAVOR
	    	    	    /* 
	    	    	    **	SCO 3.2 does has a buggy pclose(), so 
	    	    	    **	<command> become zombie (alex)
	    	    	    */
	    	    	    waitpid(-1,NULL,WNOHANG);
#endif /* SCO_FLAVOR */
	    	        }
	    	    } else {
#ifdef _OS_NT
			message (1, " Win32 ", " Unimplemented file prediction ");
#else
	    	        int pipehandle, remotehandle;
	    	        pid_t p;
		    
	    	        remotehandle = mc_open (filename, O_RDONLY);
		        if (remotehandle != -1) {
		        /* 8192 is HOWMANY hardcoded value in the file-3.14
		         * sources. Tell me if any other file uses larger
		         * chunk from beginning 
		         */
	    	            pipehandle = mc_doublepopen
			    (remotehandle, 8192, &p,"file", "file", "-", NULL);
			    if (pipehandle != -1) {
	    	                int i;
	    	                while ((i = read (pipehandle, content_string 
	    	                     + hasread, 2047 - hasread)) > 0)
	    	                    hasread += i;
	    	    	        mc_doublepclose (pipehandle, p);
	    	    	        content_string [hasread] = 0;
	    	            }
	    	            mc_close (remotehandle);
	    	        }
#endif /* _OS_NT */
	    	    }
		    asked_file = 1;
match_file_output:
	    	    if (hasread) {
	    	        if ((pp = strchr (content_string, '\n')) != 0)
	    	    	    *pp = 0;
	    	        if (islocal && !strncmp (content_string, 
	    	            filename, file_len)) {
	    	    	    content_shift = file_len;
	    	    	    if (content_string [content_shift] == ':')
	    	    	        for (content_shift++; 
	    	    	            content_string [content_shift] == ' '; 
	    	    	            content_shift++);
	    	        } else if (!islocal 
				   && !strncmp (content_string, 
						"standard input:", 15)) {
	    	            for (content_shift = 15;
	    	                content_string [content_shift] == ' ';
	    	                content_shift++);
	    	        }
	    		if (content_string && 
	    		    regexp_match (p, content_string + 
	    		        content_shift, match_normal)){
	    		    found = 1;
	    		}
	    	    }
	        }
	    } else if (!strncmp (p, "default/", 8)) {
	        p += 8;
	        found = 1;
	    }
	    *q = c;
	    p = q;
	    if (!*p)
	        break;
    	} else { /* List of actions */
    	    p = q;
    	    q = strchr (p, '\n');
    	    if (q == NULL)
    	        q = strchr (p, 0);
    	    if (found) {
    	        r = strchr (p, '=');
    	        if (r != NULL) {
    	            c = *r;
    	            *r = 0;
		    if (strcmp (p, "Include") == 0){
			char *t;

			include_target = p + 8;
			t = strchr (include_target, '\n');
			if (t) *t = 0;
			include_target_len = strlen (include_target);
			if (t) *t = '\n';

			*r = c;
			p = q;
			found = 0;
			
			if (!*p)
			    break;
			continue;
		    }
    	            if (action == NULL) {
    	                if (strcmp (p, "Open") && 
    	                    strcmp (p, "View") &&
    	                    strcmp (p, "Edit") &&
    	                    strcmp (p, "Drop") &&
    	                    strcmp (p, "Icon") &&
			    strcmp (p, "Include") &&
    	                    strcmp (p, "Title")) {
    	                    /* I.e. this is a name of a user defined action */
    	                        static char *q;
    	                        
    	                        if (to_return == NULL) {
    	                            to_return = xmalloc (512, "Action list");
    	                            q = to_return;
    	                        } else 
    	                            *(q++) = '='; /* Mark separator */
    	                        strcpy (q, p);
    	                        q = strchr (q, 0);
    	                }
    	                *r = c;
    	            } else if (!strcmp (action, "Icon")) {
    	                if (!strcmp (p, "Icon") && to_return == NULL) {
    	            	    *r = c;
    	            	    c = *q;
    	            	    *q = 0;
    	                    to_return = strdup (r + 1);
    	                } else if (!strcmp (p, "Title") && regex_command_title == NULL) {
    	            	    *r = c;
    	            	    c = *q;
    	            	    *q = 0;
    	                    regex_command_title = strdup (r + 1);
    	                } else {
    	                    *r = c;
    	                    c = *q;
    	                }
    	                *q = c;
    	                if (to_return != NULL && regex_command_title != NULL)
    	                    break;
    	            } else if (!strcmp (action, p)) {
    	                *r = c;
    	                for (p = r + 1; *p == ' ' || *p == '\t'; p++)
			    ;

			/* Empty commands just stop searching
			 * through, they don't do anything
			 *
			 * We need to copy the filename because exec_extension
			 * may end up invoking update_panels thus making the
			 * filename parameter invalid (ie, most of the time,
			 * we get filename as a pointer from cpanel->dir).
			 */
    	                if (p < q) { 
			    char *filename_copy = strdup (filename);
			    exec_extension (filename_copy, r + 1, drops, move_dir, view_at_line_number);
			    free (filename_copy);
			    
    	                    to_return = "Success";
    	                }
    	                break;
    	            } else
    	            	*r = c;
    	        }
    	    }
    	    p = q;
    	    if (!*p)
    	        break;
    	}
    }
    easy_patterns = old_patterns;
    return to_return;
}
