
#ifndef _DBGHELP_PRECOMP_H_
#define _DBGHELP_PRECOMP_H_

#include <wine/config.h>
#include <wine/port.h>

#include <assert.h>
#include <stdio.h>

#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <ntstatus.h>
#define WIN32_NO_STATUS

#include "dbghelp_private.h"

#include <winternl.h>
#include <psapi.h>
#include <wine/debug.h>
#include <wine/mscvpdb.h>

#include "image_private.h"

#endif /* !_DBGHELP_PRECOMP_H_ */
