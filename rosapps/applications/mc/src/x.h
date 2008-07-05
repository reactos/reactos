
#ifndef HAVE_X
#   include "textconf.h"
#endif

#ifdef HAVE_XVIEW
#   include "xvconf.h"
#   include "xvmain.h"
#endif

#ifdef HAVE_TK
#   include "tkconf.h"
#   include "tkmain.h"
#   include "tkwidget.h"
#   include "tkscreen.h"
#endif

#ifdef HAVE_GNOME
#   include <gnome.h>
#   include "gconf.h"
#   undef MIN
#   undef MAX
#   include "gmain.h"
#endif

