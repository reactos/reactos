
#define F_SETFL		4	/* set file->f_flags */

#ifndef O_NONBLOCK
#define O_NONBLOCK	00004000
#endif

#define	EINTR		 4	/* Interrupted system call */

int poll(struct pollfd *fds, unsigned long nfds, int timo);
int socketpair (int af, int type, int protocol, SOCKET socket[2]);
int fcntl(int fd, int cmd, long arg);
