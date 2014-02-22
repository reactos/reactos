#ifndef _FTP_H
#define _FTP_H

#include <sys/stat.h>

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
#define _INC_WINDOWS
#include <winsock.h>
#endif

#include <io.h>
#include <stdio.h>
#include <stdlib.h>

#include "ftp_var.h"

#endif /* _FTP_H */
