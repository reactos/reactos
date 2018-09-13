/*
 * project.hpp - Shell project header file for Internet Shortcut Shell
 *               extension DLL.
 */


/* Common Headers
 *****************/

#include "project.h"


/* System Headers
 *****************/

#include <stdio.h>

#include <prsht.h>

#define _INTSHCUT_               /* for intshcut.h */
#include <intshcut.h>
#ifdef WINNT_ENV
#include <intshctp.h>
#endif


/* Project Headers
 ******************/

/* The order of the following include files is significant. */

#include "olestock.h"
#include "olevalid.h"
#include "shlstock.h"
#include "shlvalid.h"
#include "inline.hpp"
#include "memmgr.hpp"
#include "comcpp.hpp"
#include "refcount.hpp"
#include "intshcut.hpp"
#include "dataobj.hpp"
#include "persist.hpp"
#include "extricon.h"
#include "propsht.hpp"
