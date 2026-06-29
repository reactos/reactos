#ifndef _TELNET_PCH_
#define _TELNET_PCH_

#include <stdarg.h>

#define WIN32_NO_STATUS

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wincon.h>
#include <winnls.h>
#define _INC_WINDOWS
#include <winsock.h>

#if defined(__BORLANDC__) && (__BORLANDC < 0x0500)
#include <mem.h>
#endif

#ifdef __BORLANDC__
#include <fstream.h>
#else
#include <fstream>
#endif

#include "src/tnconfig.h"
#include "src/tnmsg.h"
#include "src/tparser.h"
#include "src/ansiprsr.h"
#include "src/tmapldr.h"
#include "src/tnclass.h"
#include "src/tnmisc.h"
#include "src/tncon.h"

#endif /* _TELNET_PCH_ */
