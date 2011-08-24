
#include <sys/stat.h>
#include <sys/types.h>

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
#include <winsock.h>
#endif

#include <signal.h>
#include <direct.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#include "ftp_var.h"
#include "pathnames.h"
#include "prototypes.h"

#include "fake.h"

#include <fcntl.h>
