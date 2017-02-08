#include "ansiprsr.h"

#if defined(__BORLANDC__) && (__BORLANDC < 0x0500)
#include <mem.h>
//#else
//#include <memory.h>
#endif

#ifdef __BORLANDC__
#include <fstream.h>
#else
//#include <string>
#include <fstream>
#endif

//#include <windows.h>
//#include <stdlib.h>
//#include <stdio.h>
//#include <stdarg.h>
//#include <string.h>
//#include <locale.h>
#include <io.h>
#include <time.h>
//#include <ctype.h>
//#include <sys/types.h>
#include <sys/stat.h>

//#include "keytrans.h"
//#include "tnerror.h"
//#include "tcharmap.h"
//#include "tnconfig.h"
//#include "tconsole.h"
//#include "tkeydef.h"
//#include "tkeymap.h"
#include "tmapldr.h"
//#include "tmouse.h"
#include "tnclass.h"
#include "tnmisc.h"
//#include "tnclip.h"
#include "tncon.h"
//#include "ttelhndl.h"
//#include "tnetwork.h"
#include "tnmain.h"
#include "tscript.h"
//#include "tscroll.h"
#include "telnet.h"
//#include "tparams.h"
