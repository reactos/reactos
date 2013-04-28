#include <sys/stat.h>
//#include <sys/types.h>

#ifndef _WIN32
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/ftp.h>
#include <arpa/telnet.h>
#include <pwd.h>
#include <varargs.h>
#include <netdb.h>
#else
#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <wincon.h>
#define _INC_WINDOWS
#include <winsock.h>
#endif

#include <signal.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "ftp_var.h"
#include "pathnames.h"
//#include "prototypes.h"
//#include "fake.h"
