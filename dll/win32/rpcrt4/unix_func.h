
#define POLLIN 001
#define POLLPRI 002
#define POLLOUT 004
#define POLLNORM POLLIN
#define POLLERR 010
#define POLLHUP 020
#define POLLNVAL 040

struct pollfd
{
    int fd;           /* file descriptor */
    short events;     /* requested events */
    short revents;    /* returned events */
};

#define F_SETFL		4	/* set file->f_flags */

#ifndef O_NONBLOCK
#define O_NONBLOCK	00004000
#endif

#define	EINTR		 4	/* Interrupted system call */

int poll(struct pollfd *fds, unsigned long nfds, int timo);
int socketpair (int af, int type, int protocol, SOCKET socket[2]);
const char * inet_ntop (int af, const void *src, char *dst, size_t cnt);
int fcntl(int fd, int cmd, long arg);
