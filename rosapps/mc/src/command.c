/* Command line widget.
   Copyright (C) 1995 Miguel de Icaza

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   This widget is derived from the WInput widget, it's used to cope
   with all the magic of the command input line, we depend on some
   help from the program's callback.

*/

#include <config.h>
#include <errno.h>
#include "tty.h"
#include "fs.h"
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include "mad.h"
#include "dlg.h"
#include "widget.h"
#include "command.h"
#include "complete.h"		/* completion constants */
#include "global.h"		/* home_dir */
#include "dialog.h"		/* message () */
#include "dir.h"		/* required by panel.h */
#include "panel.h"		/* view_tree enum. Also, needed by main.h */
#include "main.h"		/* do_cd */
#include "layout.h"		/* for command_prompt variable */
#include "user.h"		/* expand_format */
#include "subshell.h"
#include "tree.h"		/* for tree_chdir */
#include "color.h"
#include "../vfs/vfs.h"

/* This holds the command line */
WCommand *cmdline;

/*Tries variable substitution, and if a variable CDPATH
of the form e.g. CDPATH=".:~:/usr" exists, we try then all the paths which
are mentioned there. Also, we do not support such extraordinary things as
${var:-value}, etc. Use the eval cd 'path' command instead.
Bugs: No quoting occurrs here, so ${VAR} and $VAR will be always
substituted. I think we can encourage users to use in such extreme
cases instead of >cd path< command a >eval cd 'path'< command, which works
as they might expect :) 
FIXME: Perhaps we should do wildcard matching as well? */
static int examine_cd (char *path)
{
    char *p;
    int result;
    char *q = xmalloc (MC_MAXPATHLEN + 10, "examine_cd"), *r, *s, *t, c;

    /* Variable expansion */
    for (p = path, r = q; *p && r < q + MC_MAXPATHLEN; ) {
        if (*p != '$' || (p [1] == '[' || p [1] == '('))
            *(r++)=*(p++);
        else {
            p++;
            if (*p == '{') {
                p++;
                s = strchr (p, '}');
            } else
            	s = NULL;
            if (s == NULL)
            	s = strchr (p, PATH_SEP);
            if (s == NULL)
            	s = strchr (p, 0);
            c = *s;
            *s = 0;
            t = getenv (p);
            *s = c;
            if (t == NULL) {
                *(r++) = '$';
                if (*(p - 1) != '$')
                    *(r++) = '{';
            } else {
                if (r + strlen (t) < q + MC_MAXPATHLEN) {
                    strcpy (r, t);
                    r = strchr (r, 0);
                }
                if (*s == '}')
                    p = s + 1;
                else
                    p = s;
            }
        }
    }
    *r = 0;
    
    result = do_cd (q, cd_parse_command);

    /* CDPATH handling */
    if (*q != PATH_SEP && !result) {
        p = getenv ("CDPATH");
        if (p == NULL)
            c = 0;
        else
            c = ':';
        while (!result && c == ':') {
            s = strchr (p, ':');
            if (s == NULL)
            	s = strchr (p, 0);
            c = *s;
            *s = 0;
            if (*p) {
		r = concat_dir_and_file (p, q);
                result = do_cd (r, cd_parse_command);
                free (r);
            }
            *s = c;
            p = s + 1;
        }
    }
    free (q);
    return result;
}

/* Execute the cd command on the command line */
void do_cd_command (char *cmd)
{
    int len;

    /* Any final whitespace should be removed here
       (to see why, try "cd fred "). */
    /* NOTE: I think we should not remove the extra space,
       that way, we can cd into hidden directories */
    len = strlen (cmd) - 1;
    while (len >= 0 &&
	   (cmd [len] == ' ' || cmd [len] == '\t' || cmd [len] == '\n')){
	cmd [len] = 0;
	len --;
    }
    
    if (cmd [2] == 0)
	cmd = "cd ";

    if (get_current_type () == view_tree){
	if (cmd [0] == 0){
	    tree_chdir (the_tree, home_dir);
	} else if (strcmp (cmd+3, "..") == 0){
	    char *dir = cpanel->cwd;
	    int len = strlen (dir);
	    while (len && dir [--len] != PATH_SEP);
	    dir [len] = 0;
	    if (len)
		tree_chdir (the_tree, dir);
	    else
		tree_chdir (the_tree, PATH_SEP_STR);
	} else if (cmd [3] == PATH_SEP){
	    tree_chdir (the_tree, cmd+3);
	} else {
	    char *old = cpanel->cwd;
	    char *new;
	    new = concat_dir_and_file (old, cmd+3);
	    tree_chdir (the_tree, new);
	    free (new);
	}
    } else
	if (!examine_cd (&cmd [3])) {
	    message (1, MSG_ERROR, _(" Can't chdir to '%s' \n %s "),
		     &cmd [3], unix_error_string (errno));
	    return;
	}
}

/* Returns 1 if the we could handle the enter, 0 if not */
static int enter (WCommand *cmdline)
{
    Dlg_head *old_dlg;
    
    if (command_prompt && strlen (input_w (cmdline)->buffer)){
	char *cmd;

	/* Any initial whitespace should be removed at this point */
	cmd = input_w (cmdline)->buffer;
	while (*cmd == ' ' || *cmd == '\t' || *cmd == '\n')
	    cmd++;

	if (strncmp (cmd, "cd ", 3) == 0 || strcmp (cmd, "cd") == 0){
	    do_cd_command (cmd);
	    new_input (input_w (cmdline));
	    return MSG_HANDLED;
	}
#ifdef 	OS2_NT
	else if ( (strncmp (cmd, "set ", 4) == 0 || strncmp (cmd, "Set ", 4) == 0 || strncmp (cmd, "SET ", 4) == 0) && strchr(cmd,'=') ){
	    cmd+=4;
	    while (*cmd == ' ' || *cmd == '\t' || *cmd == '\n')
		cmd++;
	    _putenv(cmd);
	    new_input (input_w (cmdline));
	    return MSG_HANDLED;
	}
#endif 
	else {
	    char *command, *s;
	    int i, j;

	    if (!vfs_current_is_local ())
		return MSG_NOT_HANDLED;
	    command = xmalloc (strlen (cmd) + 1, "main, enter");
	    command [0] = 0;
	    for (i = j = 0; i < strlen (cmd); i ++){
		if (cmd [i] == '%'){
		    i ++;
		    s = expand_format (cmd [i], 1);
		    command = realloc (command, strlen (command) + strlen (s)
				       + strlen (cmd) - i + 1);
		    strcat (command, s);
		    free (s);
		    j = strlen (command);
		} else {
		    command [j] = cmd [i];
		    j ++;
		}
		command [j] = 0;
	    }
	    old_dlg = current_dlg;
	    current_dlg = 0;
	    new_input (input_w (cmdline));
	    execute (command);
	    free (command);
	    
#ifdef HAVE_SUBSHELL_SUPPORT
	    if (quit & SUBSHELL_EXIT){
	        quiet_quit_cmd ();
		return MSG_HANDLED;
	    }
	    if (use_subshell)
		load_prompt (0, 0);
#endif

	    current_dlg = old_dlg;
	}
    }
    return MSG_HANDLED;
}

static int command_callback (Dlg_head *h, WCommand *cmd, int msg, int par)
{
    switch (msg){
    case WIDGET_FOCUS:
	/* We refuse the focus always: needed not to unselect the panel */
	return MSG_NOT_HANDLED;

    case WIDGET_KEY:
	/* Special case: we handle the enter key */
	if (par == '\n'){
	    return enter (cmd);
	}
    }
    return (*cmd->old_callback)(h, cmd, msg, par);
}

WCommand *command_new (int y, int x, int cols)
{
    WInput *in;
    WCommand *cmd = xmalloc (sizeof (WCommand), "command_new");

    in = input_new (y, x, DEFAULT_COLOR, cols, "", "cmdline");
    cmd->input = *in;
    free (in);

    /* Add our hooks */
    cmd->old_callback = (callback_fn) cmd->input.widget.callback;
    cmd->input.widget.callback = (int (*) (Dlg_head *, void *, int, int))
    			command_callback;
    
    cmd->input.completion_flags |= INPUT_COMPLETE_COMMANDS;
    return cmd;
}


