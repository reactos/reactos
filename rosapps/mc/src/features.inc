/* This just computes a nice value for the features variable */

#ifndef VERSION
#    define VERSION "undefined"
#endif

char *features = 
    "Edition: "
#ifdef HAVE_X
#    ifdef HAVE_XVIEW
         "XView"
#    else
         "Tk"
#    endif
#else
    "text mode"
#endif
    ".\n"

#ifdef USE_VFS
    "Virtual File System: tarfs, extfs"
#ifdef USE_NETCODE
    ", ftpfs"
#   ifdef HSC_PROXY
    " (proxies: hsc proxy)"
#   endif
    ", mcfs"
#   ifdef USE_TERMNET
    " (with termnet support)"
#   endif
#endif
#ifdef USE_EXT2FSLIB
    ", undelfs"
#endif
    ".\n"
#endif
    
#ifdef USE_INTERNAL_EDIT
    "With builtin Editor\n"
#endif

    "Using "
#ifdef HAVE_SLANG
#   ifdef HAVE_SYSTEM_SLANG
	"system-installed "
#   endif
    "S-lang library with "

#   ifdef SLANG_TERMINFO
        "terminfo"
#   else
#       ifdef USE_TERMCAP
            "termcap"
#       else
            "an unknown terminal"
#       endif
#   endif
    " database"
#else
#   ifdef USE_NCURSES
        "the ncurses library"
#   else
        "some unknown curses library"
#   endif
#endif
    "\n"
#ifdef HAVE_SUBSHELL_SUPPORT
    "With subshell support: "
#   ifdef SUBSHELL_OPTIONAL
        "optional"
#   else
        "as default"
#   endif
    "\n"
#endif

#ifdef HAVE_DUSUM
    "With DUSUM command\n"
#endif

#ifdef WITH_BACKGROUND
    "With support for background operations\n"
#endif
;

static const int status_mouse_support = 
#ifdef HAVE_LIBGPM
    1;
#else
    0;
#endif

const int status_using_ncurses =
#ifdef HAVE_SLANG
    0;
#else
#ifdef USE_NCURSES
    1;
#else
    0;
#endif
#endif


