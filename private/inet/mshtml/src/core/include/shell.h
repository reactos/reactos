#ifndef I_SHELL_H_
#define I_SHELL_H_
#pragma INCMSG("--- Beg 'shell.h'")

#ifdef WIN16
#ifndef X_SHELLAPI_H_
#define X_SHELLAPI_H_
#include "shellapi.h"
#endif
#endif

#ifndef X_SHLOBJ_H_
#define X_SHLOBJ_H_
#include <shlobj.h>
#endif

#ifndef X_SHLOBJP_H_
#define X_SHLOBJP_H_
#include <shlobjp.h>
#endif

#pragma INCMSG("--- End 'shell.h'")
#else
#pragma INCMSG("*** Dup 'shell.h'")
#endif

