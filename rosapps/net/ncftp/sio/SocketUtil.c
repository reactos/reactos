#include "syshdrs.h"

#ifndef SO_RCVBUF
int
GetSocketBufSize(int UNUSED(sockfd), size_t *const rsize, size_t *const ssize)
{
	LIBSIO_USE_VAR(sockfd);
	if (ssize != NULL)
		*ssize = 0;
	if (rsize != NULL)
		*rsize = 0;
	return (-1);
}	/* GetSocketBufSize */
#else
int
GetSocketBufSize(int sockfd, size_t *const rsize, size_t *const ssize)
{
	int rc = -1;
	int opt;
	int optsize;

	if (ssize != NULL) {
		opt = 0;
		optsize = sizeof(opt);
		rc = getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *) &opt, &optsize);
		if (rc == 0)
			*ssize = (size_t) opt;
		else
			*ssize = 0;
	}
	if (rsize != NULL) {
		opt = 0;
		optsize = sizeof(opt);
		rc = getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *) &opt, &optsize);
		if (rc == 0)
			*rsize = (size_t) opt;
		else
			*rsize = 0;
	}
	return (rc);
}	/* GetSocketBufSize */
#endif




#ifndef SO_RCVBUF
int
SetSocketBufSize(int UNUSED(sockfd), size_t UNUSED(rsize), size_t UNUSED(ssize))
{
	LIBSIO_USE_VAR(sockfd);
	LIBSIO_USE_VAR(rsize);
	LIBSIO_USE_VAR(ssize);
	return (-1);
}	/* SetSocketBufSize */
#else
int
SetSocketBufSize(int sockfd, size_t rsize, size_t ssize)
{
	int rc = -1;
	int opt;
	int optsize;

	if (ssize > 0) {
		opt = (int) ssize;
		optsize = sizeof(opt);
		rc = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const char *) &opt, optsize);
		if (rc < 0)
			return (rc);
	}
	if (rsize > 0) {
		opt = (int) rsize;
		optsize = sizeof(opt);
		rc = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const char *) &opt, optsize);
		if (rc < 0)
			return (rc);
	}
	return (0);
}	/* SetSocketBufSize */
#endif




#ifndef TCP_NODELAY
int
GetSocketNagleAlgorithm(const int UNUSED(fd))
{
	LIBSIO_USE_VAR(fd);
	return (-1);
}	/* GetSocketNagleAlgorithm */
#else
int
GetSocketNagleAlgorithm(const int fd)
{
	int optsize;
	int opt;

	opt = -2;
	optsize = (int) sizeof(opt);
	if (getsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) &opt, &optsize) < 0)
		return (-1);
	return (opt);
}	/* GetSocketNagleAlgorithm */
#endif	/* TCP_NODELAY */





#ifndef TCP_NODELAY
int
SetSocketNagleAlgorithm(const int UNUSED(fd), const int UNUSED(onoff))
{
	LIBSIO_USE_VAR(fd);
	LIBSIO_USE_VAR(onoff);
	return (-1);
}	/* SetSocketNagleAlgorithm */
#else
int
SetSocketNagleAlgorithm(const int fd, const int onoff)
{
	int optsize;
	int opt;

	opt = onoff;
	optsize = (int) sizeof(opt);
	return (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) &opt, optsize));
}	/* SetSocketNagleAlgorithm */
#endif	/* TCP_NODELAY */



#ifndef SO_LINGER
int
GetSocketLinger(const int UNUSED(fd), int *const UNUSED(lingertime))
{
	LIBSIO_USE_VAR(fd);
	LIBSIO_USE_VAR(lingertime);
	return (-1);
}	/* GetSocketLinger */
#else
int
GetSocketLinger(const int fd, int *const lingertime)
{
	int optsize;
	struct linger opt;

	optsize = (int) sizeof(opt);
	opt.l_onoff = 0;
	opt.l_linger = 0;
	if (getsockopt(fd, SOL_SOCKET, SO_LINGER, (char *) &opt, &optsize) < 0)
		return (-1);
	if (lingertime != NULL)
		*lingertime = opt.l_linger;
	return (opt.l_onoff);
}	/* GetSocketLinger */
#endif	/* SO_LINGER */



#ifndef SO_LINGER
int
SetSocketLinger(const int UNUSED(fd), const int UNUSED(l_onoff), const int UNUSED(l_linger))
{
	LIBSIO_USE_VAR(fd);
	LIBSIO_USE_VAR(l_onoff);
	LIBSIO_USE_VAR(l_linger);
	return (-1);
}	/* SetSocketLinger */
#else
int
SetSocketLinger(const int fd, const int l_onoff, const int l_linger)
{
	struct linger opt;
	int optsize;
/*
 * From hpux:
 *
 * Structure used for manipulating linger option.
 *
 * if l_onoff == 0:
 *    close(2) returns immediately; any buffered data is sent later
 *    (default)
 * 
 * if l_onoff != 0:
 *    if l_linger == 0, close(2) returns after discarding any unsent data
 *    if l_linger != 0, close(2) does not return until buffered data is sent
 */
#if 0
struct	linger {
	int	l_onoff;		/* 0 = do not wait to send data */
					/* non-0 = see l_linger         */
	int	l_linger;		/* 0 = discard unsent data      */
					/* non-0 = wait to send data    */
};
#endif
	opt.l_onoff = l_onoff;
	opt.l_linger = l_linger;
	optsize = (int) sizeof(opt);
	return (setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *) &opt, optsize));
}	/* SetSocketLinger */
#endif	/* SO_LINGER */
