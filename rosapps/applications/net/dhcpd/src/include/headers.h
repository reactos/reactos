#ifndef HEADERS_H
#define HEADERS_H

#ifdef  __MINGW32__

#include <stdio.h>
#include <stdlib.h>
#include <winsock.h>

#else

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#endif

#endif
