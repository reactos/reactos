/*
 * Text edition support code
 *
 *
 */
#include <config.h>

#ifdef HAVE_X
#error This file is for text-mode editions only.
#endif

#include <stdio.h>

#define WANT_WIDGETS
#include "win.h"
#include "tty.h"
#include "key.h"
#include "widget.h"
#include "main.h"
#include "cons.saver.h"

char *default_edition_colors =
"normal=lightgray,blue:"
"selected=black,cyan:"
"marked=yellow,blue:"
"markselect=yellow,cyan:"
"errors=white,red:"
"menu=white,cyan:"
"reverse=black,lightgray:"
"dnormal=black,lightgray:"
"dfocus=black,cyan:"
"dhotnormal=yellow,lightgray:"
"dhotfocus=yellow,cyan:"
"viewunderline=brightred,blue:"
"menuhot=yellow,cyan:"
"menusel=white,black:"
"menuhotsel=yellow,black:"
"helpnormal=black,lightgray:"
"helpitalic=red,lightgray:"
"helpbold=blue,lightgray:"
"helplink=black,cyan:"
"helpslink=yellow,blue:"
"gauge=white,black:"
"input=black,cyan:"
"directory=white,blue:"
"execute=brightgreen,blue:"
"link=lightgray,blue:"
"device=brightmagenta,blue:"
"core=red,blue:"
"special=black,blue";

void
edition_post_exec (void)
{
    do_enter_ca_mode ();

    /* FIXME: Missing on slang endwin? */
    reset_prog_mode ();
    flushinp ();
    
    keypad (stdscr, TRUE);
    mc_raw_mode ();
    channels_up ();
    if (use_mouse_p)
	init_mouse ();
    if (alternate_plus_minus)
        application_keypad_mode ();
}

void
edition_pre_exec (void)
{
    if (clear_before_exec)
	clr_scr ();
    else {
	if (!(console_flag || xterm_flag))
	    printf ("\n\n");
    }

    channels_down ();
    if (use_mouse_p)
	shut_mouse ();
    
    reset_shell_mode ();
    keypad (stdscr, FALSE);
    endwin ();
    
    numeric_keypad_mode ();
    
    /* on xterms: maybe endwin did not leave the terminal on the shell
     * screen page: do it now.
     *
     * Do not move this before endwin: in some systems rmcup includes
     * a call to clear screen, so it will end up clearing the sheel screen.
     */
    if (!status_using_ncurses){
	do_exit_ca_mode ();
    }
}

void
clr_scr (void)
{
    standend ();
    dlg_erase (midnight_dlg);
    mc_refresh ();
    doupdate ();
}

