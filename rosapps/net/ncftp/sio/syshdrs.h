/* syshdrs.h
 *
 * Copyright (c) 1999 Mike Gleason, NCEMRSoft.
 * All rights reserved.
 *
 */

#if defined(HAVE_CONFIG_H)
#	include "config.h"
#endif

#if defined(WIN32) || defined(_WINDOWS)
	/* Include "wincfg.h" in place of "config.h" */
#	include "wincfg.h"
#	include <winsock2.h>	/* includes <windows.h> */
#	define _POSIX_ 1
#	ifdef HAVE_UNISTD_H
#		include <unistd.h>
#	endif
#	include <errno.h>
#	include <stdio.h>
#	include <string.h>
#	ifdef HAVE_STRINGS_H
#		include <strings.h>
#	endif
#	include <stddef.h>
#	include <stdlib.h>
#	include <ctype.h>
#	include <stdarg.h>
#	include <time.h>
#	include <io.h>
#	include <sys/types.h>
#	include <sys/stat.h>
#	include <fcntl.h>
#	define strcasecmp stricmp
#	define strncasecmp strnicmp
#	define sleep(a) Sleep(a * 1000)
#	ifndef S_ISREG
#		define S_ISREG(m)      (((m) & _S_IFMT) == _S_IFREG)
#		define S_ISDIR(m)      (((m) & _S_IFMT) == _S_IFDIR)
#	endif
#	ifndef open
#		define open _open
#		define write _write
#		define read _read
#		define close _close
#		define lseek _lseek
#		define stat _stat
#		define fstat _fstat
#		define dup _dup
#	endif
#	ifndef unlink
#		define unlink remove
#	endif
#	define NO_SIGNALS 1
#	define NO_UNIX_DOMAIN_SOCKETS 1
#else	/* UNIX */

#if defined(AIX) || defined(_AIX)
#	define _ALL_SOURCE 1
#endif
#ifdef HAVE_UNISTD_H
#	include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#if !defined(HAVE_GETCWD) && defined(HAVE_GETWD)
#	include <sys/param.h>
#endif

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#	include <strings.h>
#endif
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <setjmp.h>
#include <stdarg.h>
#include <time.h>
#include <fcntl.h>

#ifdef HAVE_NET_ERRNO_H
#	include <net/errno.h>
#endif
#ifdef HAVE_ARPA_NAMESER_H
#	include <arpa/nameser.h>
#endif
#ifdef HAVE_NSERVE_H
#	include <nserve.h>
#endif
#ifdef HAVE_RESOLV_H
#	include <resolv.h>
#endif
#ifdef CAN_USE_SYS_SELECT_H
#	include <sys/select.h>
#endif

#ifdef HAVE_GETCWD
#	ifndef HAVE_UNISTD_H
		extern char *getcwd();
#	endif
#else
#	ifdef HAVE_GETWD
#		include <sys/param.h>
#		ifndef MAXPATHLEN
#			define MAXPATHLEN 1024
#		endif
		extern char *getwd(char *);
#	endif
#endif

#define HAVE_RECVMSG 1

#endif	/* UNIX */

#if defined(HAVE_LONG_LONG) && defined(HAVE_OPEN64)
#	define Open open64
#else
#	define Open open
#endif

#if defined(HAVE_LONG_LONG) && defined(HAVE_STAT64) && defined(HAVE_STRUCT_STAT64)
#	define Stat stat64
#	ifdef HAVE_FSTAT64
#		define Fstat fstat64
#	else
#		define Fstat fstat
#	endif
#	ifdef HAVE_LSTAT64
#		define Lstat lstat64
#	else
#		define Lstat lstat
#	endif
#else
#	define Stat stat
#	define Fstat fstat
#	define Lstat lstat
#endif

#if defined(HAVE_LONG_LONG) && defined(HAVE_LSEEK64)
#	define Lseek(a,b,c) lseek64(a, (longest_int) b, c)
#elif defined(HAVE_LONG_LONG) && defined(HAVE_LLSEEK)
#	if 1
#		if defined(LINUX) && (LINUX <= 23000)
#			define Lseek(a,b,c) lseek(a, (off_t) b, c)
#		else
#			define Lseek(a,b,c) llseek(a, (longest_int) b, c)
#		endif
#	else
#		define Lseek(a,b,c) lseek(a, (off_t) b, c)
#	endif
#else
#	define Lseek(a,b,c) lseek(a, (off_t) b, c)
#endif


#ifndef IAC

/*
 * Definitions for the TELNET protocol.
 */
#define IAC     255             /* interpret as command: */
#define DONT    254             /* you are not to use option */
#define DO      253             /* please, you use option */
#define WONT    252             /* I won't use option */
#define WILL    251             /* I will use option */
#define SB      250             /* interpret as subnegotiation */
#define GA      249             /* you may reverse the line */
#define EL      248             /* erase the current line */
#define EC      247             /* erase the current character */
#define AYT     246             /* are you there */
#define AO      245             /* abort output--but let prog finish */
#define IP      244             /* interrupt process--permanently */
#define BREAK   243             /* break */
#define DM      242             /* data mark--for connect. cleaning */
#define NOP     241             /* nop */
#define SE      240             /* end sub negotiation */
#define EOR     239             /* end of record (transparent mode) */
#define ABORT   238             /* Abort process */
#define SUSP    237             /* Suspend process */
#define xEOF    236             /* End of file: EOF is already used... */

#define SYNCH   242             /* for telfunc calls */
#endif

#include "sio.h"			/* Library header. */
#ifndef NO_UNIX_DOMAIN_SOCKETS
#	include "usio.h"
#endif
/* eof */
