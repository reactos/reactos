#ifndef __FLOCK_HXX
#define __FLOCK_HXX
#include <wininetp.h>
#include <mainwin.h>
extern void ShowReadOnlyCacheDialog(char* pszHostName);

#include <sys/types.h>	
#include <signal.h>
#include <stdio.h>	
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#if defined(UNIX) && defined(ux10)
#undef M
#include <net/if.h>
#define M * 1048576
#else
#include <net/if.h>
#endif
#include <netinet/if_ether.h>
#include <netdb.h>

#define LF "lf"
#define LOCKDBF "lockdbf"

#define	FILE_MODE	(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

#define	read_lock(fd, offset, whence, len) \
			lock_reg(fd, F_SETLK, F_RDLCK, offset, whence, len)
#define	readw_lock(fd, offset, whence, len) \
			lock_reg(fd, F_SETLKW, F_RDLCK, offset, whence, len)
#define	write_lock(fd, offset, whence, len) \
			lock_reg(fd, F_SETLK, F_WRLCK, offset, whence, len)
#define	writew_lock(fd, offset, whence, len) \
			lock_reg(fd, F_SETLKW, F_WRLCK, offset, whence, len)
#define	un_lock(fd, offset, whence, len) \
			lock_reg(fd, F_SETLK, F_UNLCK, offset, whence, len)
#define	can_readlock(fd, offset, whence, len) \
			lock_test(fd, F_RDLCK, offset, whence, len)
#define	can_writelock(fd, offset, whence, len) \
			lock_test(fd, F_WRLCK, offset, whence, len)

#endif /* __FLOCK_HXX */
