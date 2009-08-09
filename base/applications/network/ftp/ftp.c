#define L_SET SEEK_SET
#define L_INCR SEEK_CUR
#define caddr_t void *
/*
 * Copyright (c) 1985, 1989 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static char sccsid[] = "@(#)ftp.c	5.28 (Berkeley) 4/20/89";
#endif /* not lint */
#include <io.h>

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
#include <winsock.h>
#endif

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#include "ftp_var.h"
#include "prototypes.h"
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

#ifdef NOVFPRINTF
#define vfprintf(a,b,c) _doprnt(b,c,a)
#endif

#ifdef sun
/* FD_SET wasn't defined until 4.0. its a cheap test for uid_t  presence */
#ifndef FD_SET
#define	NBBY	8		/* number of bits in a byte */
/*
 * Select uses bit masks of file descriptors in longs.
 * These macros manipulate such bit fields (the filesystem macros use chars).
 * FD_SETSIZE may be defined by the user, but the default here
 * should be >= NOFILE (param.h).
 */
#ifndef	FD_SETSIZE
#define	FD_SETSIZE	256
#endif

typedef long	fd_mask;
#define NFDBITS	(sizeof(fd_mask) * NBBY)	/* bits per mask */
#ifndef howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif

#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)	bzero((char *)(p), sizeof(*(p)))

typedef int uid_t;
typedef int gid_t;
#endif
#endif

struct	sockaddr_in hisctladdr;
struct	sockaddr_in data_addr;
int	data = -1;
int	abrtflag = 0;
int	ptflag = 0;
int	allbinary;
struct	sockaddr_in myctladdr;
uid_t	getuid();
sig_t	lostpeer();
off_t	restart_point = 0;

SOCKET cin, cout;
int	dataconn(const char *mode);

int command(const char *fmt, ...);

char *hostname;

typedef void (*Sig_t)(int);

// Signal Handlers

void psabort(int sig);

char *hookup(char *host, int port)
{
	register struct hostent *hp = 0;
	int len;
	SOCKET s;
	static char hostnamebuf[80];

	bzero((char *)&hisctladdr, sizeof (hisctladdr));
	hisctladdr.sin_addr.s_addr = inet_addr(host);
	if (hisctladdr.sin_addr.s_addr != (unsigned long)-1) {
		hisctladdr.sin_family = AF_INET;
		(void) strncpy(hostnamebuf, host, sizeof(hostnamebuf));
	} else {
		hp = gethostbyname(host);
		if (hp == NULL) {
			fprintf(stderr, "ftp: %s: ", host);
			herror((char *)NULL);
			code = -1;
			return((char *) 0);
		}
		hisctladdr.sin_family = hp->h_addrtype;
		bcopy(hp->h_addr_list[0],
			 (caddr_t)&hisctladdr.sin_addr, hp->h_length);
		(void) strncpy(hostnamebuf, hp->h_name, sizeof(hostnamebuf));
	}
	hostname = hostnamebuf;
	s = socket(hisctladdr.sin_family, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET) {
		perror("ftp: socket");
		code = -1;
		return (0);
	}
	hisctladdr.sin_port = port;
	while (connect(s, (struct sockaddr *)&hisctladdr, sizeof (hisctladdr)) < 0) {
		if (hp && hp->h_addr_list[1]) {
			int oerrno = errno;

			fprintf(stderr, "ftp: connect to address %s: ",
				inet_ntoa(hisctladdr.sin_addr));
			errno = oerrno;
			perror((char *) 0);
			hp->h_addr_list++;
			bcopy(hp->h_addr_list[0],
			     (caddr_t)&hisctladdr.sin_addr, hp->h_length);
			fprintf(stdout, "Trying %s...\n",
				inet_ntoa(hisctladdr.sin_addr));
			(void) fflush(stdout);
			(void) close(s);
			s = socket(hisctladdr.sin_family, SOCK_STREAM, 0);
			if (s == INVALID_SOCKET) {
				perror("ftp: socket");
				code = -1;
				return (0);
			}
			continue;
		}
		perror("ftp: connect");
		code = -1;
		goto bad;
	}
	len = sizeof (myctladdr);
	if (getsockname(s, (struct sockaddr *)&myctladdr, &len) < 0) {
		perror("ftp: getsockname");
		code = -1;
		goto bad;
	}
	cin = cout = s;
	if (verbose) {
		printf("Connected to %s.\n", hostname);
		(void) fflush(stdout);
	}
	if (getreply(0) > 2) { 	/* read startup message from server */
		closesocket(cin);
		code = -1;
		goto bad;
	}
#ifdef SO_OOBINLINE
	{
	int on = 1;

	if (setsockopt(s, SOL_SOCKET, SO_OOBINLINE, (const char *) &on, sizeof(on))
		< 0 && debug) {
			perror("ftp: setsockopt");
		}
	}
#endif //SO_OOBINLINE

	return (hostname);
bad:
	(void) close(s);
	return ((char *)0);
}

int login(const char *host)
{
	char tmp[80];
	char *puser, *ppass, *pacct;
	const char *user, *pass, *acct;
	int n, aflag = 0;

	user = pass = acct = 0;
	n = ruserpass(host, &puser, &ppass, &pacct);
	if (n < 0) {
		code = -1;
		return(0);
	}
	if (0 != n) {
		user = puser;
		pass = ppass;
		acct = pacct;
	}
	while (user == NULL) {
		const char *myname = "none"; // This needs to become the usename env

		if (myname)
			printf("Name (%s:%s): ", host, myname);
		else
			printf("Name (%s): ", host);
		(void) fflush(stdout);
		(void) fgets(tmp, sizeof(tmp) - 1, stdin);
		tmp[strlen(tmp) - 1] = '\0';
		if (*tmp == '\0')
			user = myname;
		else
			user = tmp;
	}
	n = command("USER %s", user);
	if (n == CONTINUE) {
		if (pass == NULL)
			pass = getpass("Password:");
		n = command("PASS %s", pass);
		fflush(stdin);
	}
	if (n == CONTINUE) {
		aflag++;
		acct = getpass("Account:");
		n = command("ACCT %s", acct);
	}
	if (n != COMPLETE) {
		fprintf(stderr, "Login failed.\n");
		return (0);
	}
	if (!aflag && acct != NULL)
		(void) command("ACCT %s", acct);
	if (proxy)
		return(1);
	for (n = 0; n < macnum; ++n) {
		if (!strcmp("init", macros[n].mac_name)) {
			(void) strcpy(line, "$init");
			makeargv();
			domacro(margc, margv);
			break;
		}
	}
	return (1);
}

static void
cmdabort(int sig)
{
	extern jmp_buf ptabort;

	printf("\n");
	(void) fflush(stdout);
	abrtflag++;
	if (ptflag)
		longjmp(ptabort,1);
}

/*VARARGS1*/
int command(const char *fmt, ...)
{
	va_list ap;
	int r;
	void (*oldintr)(int);

	abrtflag = 0;
	if (debug) {
		printf("---> ");
		va_start(ap, fmt);
		vfprintf(stdout, fmt, ap);
		va_end(ap);
		printf("\n");
		(void) fflush(stdout);
	}
	if (cout == 0) {
		perror ("No control connection for command");
		code = -1;
		return (0);
	}
	oldintr = signal(SIGINT,cmdabort);
	{
		char buffer[1024];

		va_start(ap, fmt);
		vsprintf(buffer, fmt, ap);
		va_end(ap);
//DLJ: to work through  firewalls - send the command as a single message
		strcat(buffer,"\r\n");
		fprintfSocket(cout, buffer);
	}
//DLJ: the following two lines are replaced by the strcat above - seems to
// make it work through firewalls.
//	fprintfSocket(cout, "\r\n");
//	(void) fflush(cout);
	cpend = 1;
	r = getreply(!strcmp(fmt, "QUIT"));
	if (abrtflag && oldintr != SIG_IGN)
		(*oldintr)(SIGINT);
//	(void) signal(SIGINT, oldintr);
	return(r);
}

char reply_string[BUFSIZ];		/* last line of previous reply */

#include <ctype.h>

int
getreply(expecteof)
	int expecteof;
{
	register int c, n;
	register int dig;
	register char *cp;
	int originalcode = 0, continuation = 0;
	void (*oldintr)(int);
	int pflag = 0;
	char *pt = pasv;

	oldintr = signal(SIGINT,cmdabort);
	for (;;) {
		dig = n = code = 0;
		cp = reply_string;
		while ((c = fgetcSocket(cin)) != '\n') {
			if (c == IAC) {     /* handle telnet commands */
				switch (c = fgetcSocket(cin)) {
				case WILL:
				case WONT:
					c = fgetcSocket(cin);
					fprintfSocket(cout, "%c%c%c",IAC,DONT,c);
					break;
				case DO:
				case DONT:
					c = fgetcSocket(cin);
					fprintfSocket(cout, "%c%c%c",IAC,WONT,c);
					break;
				default:
					break;
				}
				continue;
			}
			dig++;
			if (c == EOF) {
				if (expecteof) {
//					(void) signal(SIGINT,oldintr);
					code = 221;
					return (0);
				}
				lostpeer();
				if (verbose) {
					printf("421 Service not available, remote server has closed connection\n");
					(void) fflush(stdout);
				}
				code = 421;
				return(4);
			}
			if (c != '\r' && (verbose > 0 ||
			    (verbose > -1 && n == '5' && dig > 4))) {
				if (proxflag &&
				   ((dig == 1 || dig == 5) && verbose == 0))
					printf("%s:",hostname);
				(void) putchar(c);
				(void) fflush(stdout);
			}
			if (dig < 4 && isdigit(c))
				code = code * 10 + (c - '0');
			if (!pflag && code == 227)
				pflag = 1;
			if (dig > 4 && pflag == 1 && isdigit(c))
				pflag = 2;
			if (pflag == 2) {
				if (c != '\r' && c != ')')
					*pt++ = c;
				else {
					*pt = '\0';
					pflag = 3;
				}
			}
			if (dig == 4 && c == '-') {
				if (continuation)
					code = 0;
				continuation++;
			}
			if (n == 0)
				n = c;
			if (cp < &reply_string[sizeof(reply_string) - 1])
				*cp++ = c;
		}
		if (verbose > 0 || (verbose > -1 && n == '5')) {
			(void) putchar(c);
			(void) fflush (stdout);
		}
		if (continuation && code != originalcode) {
			if (originalcode == 0)
				originalcode = code;
			continue;
		}
		*cp = '\0';
		if (n != '1')
			cpend = 0;
		(void) signal(SIGINT,oldintr);
		if (code == 421 || originalcode == 421)
			lostpeer();
		if (abrtflag && oldintr != cmdabort && oldintr != SIG_IGN)
			(*oldintr)(SIGINT);
		return (n - '0');
	}
}

static int
empty(mask, sec)
	struct fd_set *mask;
	int sec;
{
	struct timeval t;

	t.tv_sec = (long) sec;
	t.tv_usec = 0;
	return(select(32, mask, (struct fd_set *) 0, (struct fd_set *) 0, &t));
}

jmp_buf	sendabort;

#if 0
void abortsend()
{

	mflag = 0;
	abrtflag = 0;
	printf("\nsend aborted\n");
	(void) fflush(stdout);
	longjmp(sendabort, 1);
}
#endif

#define HASHBYTES 1024

void sendrequest(const char *cmd, const char *local, const char *remote, int printnames)
{
	FILE *fin;
	int dout = 0;
	int (*closefunc)();
	sig_t (*oldintr)(), (*oldintp)();
	char buf[BUFSIZ], *bufp;
	long bytes = 0, hashbytes = HASHBYTES;
	register int c, d;
	struct stat st;
	struct timeval start, stop;
	const char *mode;

	if (verbose && printnames) {
		if (local && *local != '-')
			printf("local: %s ", local);
		if (remote)
			printf("remote: %s\n", remote);
		(void) fflush(stdout);
	}
	if (proxy) {
		proxtrans(cmd, local, remote);
		return;
	}
	closefunc = NULL;
	oldintr = NULL;
	oldintp = NULL;
	mode = "w";
	if (setjmp(sendabort)) {
		while (cpend) {
			(void) getreply(0);
		}
		if (data >= 0) {
			(void) close(data);
			data = -1;
		}
		if (oldintr)
null();//			(void) signal(SIGINT,oldintr);
		if (oldintp)
null();//			(void) signal(SIGPIPE,oldintp);
		code = -1;
		return;
	}
null();//	oldintr = signal(SIGINT, abortsend);
	if (strcmp(local, "-") == 0)
		fin = stdin;
	else if (*local == '|') {
null();//		oldintp = signal(SIGPIPE,SIG_IGN);
		fin = _popen(local + 1, "r");
		if (fin == NULL) {
			perror(local + 1);
null();//			(void) signal(SIGINT, oldintr);
null();//			(void) signal(SIGPIPE, oldintp);
			code = -1;
			return;
		}
		closefunc = _pclose;
	} else {
		fin = fopen(local, "r");
		if (fin == NULL) {
			perror(local);
null();//			(void) signal(SIGINT, oldintr);
			code = -1;
			return;
		}
		closefunc = fclose;
		if (fstat(fileno(fin), &st) < 0 ||
		    (st.st_mode&S_IFMT) != S_IFREG) {
			fprintf(stdout, "%s: not a plain file.\n", local);
			(void) fflush(stdout);
null();//			(void) signal(SIGINT, oldintr);
			fclose(fin);
			code = -1;
			return;
		}
	}
	if (initconn()) {
null();//		(void) signal(SIGINT, oldintr);
		if (oldintp)
null();//			(void) signal(SIGPIPE, oldintp);
		code = -1;
		if (closefunc != NULL)
			(*closefunc)(fin);
		return;
	}
	if (setjmp(sendabort))
		goto abort;

	if (restart_point &&
	    (strcmp(cmd, "STOR") == 0 || strcmp(cmd, "APPE") == 0)) {
		if (fseek(fin, (long) restart_point, 0) < 0) {
			perror(local);
			restart_point = 0;
			if (closefunc != NULL)
				(*closefunc)(fin);
			return;
		}
		if (command("REST %ld", (long) restart_point)
			!= CONTINUE) {
			restart_point = 0;
			if (closefunc != NULL)
				(*closefunc)(fin);
			return;
		}
		restart_point = 0;
		mode = "r+w";
	}
	if (remote) {
		if (command("%s %s", cmd, remote) != PRELIM) {
null();//			(void) signal(SIGINT, oldintr);
			if (oldintp)
null();//				(void) signal(SIGPIPE, oldintp);
			if (closefunc != NULL)
				(*closefunc)(fin);
			return;
		}
	} else
		if (command("%s", cmd) != PRELIM) {
null();//			(void) signal(SIGINT, oldintr);
			if (oldintp)
null();//				(void) signal(SIGPIPE, oldintp);
			if (closefunc != NULL)
				(*closefunc)(fin);
			return;
		}
	dout = dataconn(mode);
	if (!dout)
		goto abort;
	(void) gettimeofday(&start, (struct timezone *)0);
null();//	oldintp = signal(SIGPIPE, SIG_IGN);
	switch (type) {

	case TYPE_I:
	case TYPE_L:
		errno = d = 0;
		while ((c = read(fileno(fin), buf, sizeof (buf))) > 0) {
			bytes += c;
			for (bufp = buf; c > 0; c -= d, bufp += d)
				if ((d = send(dout, bufp, c, 0)) <= 0)
					break;
			if (hash) {
				while (bytes >= hashbytes) {
					(void) putchar('#');
					hashbytes += HASHBYTES;
				}
				(void) fflush(stdout);
			}
		}
		if (hash && bytes > 0) {
			if (bytes < HASHBYTES)
				(void) putchar('#');
			(void) putchar('\n');
			(void) fflush(stdout);
		}
		if (c < 0)
			perror(local);
		if (d <= 0) {
			if (d == 0)
				fprintf(stderr, "netout: write returned 0?\n");
			else if (errno != EPIPE)
				perror("netout");
			bytes = -1;
		}
		break;

	case TYPE_A:
		{
		char buf[1024];
		static int bufsize = 1024;
		int ipos=0;

		while ((c = getc(fin)) != EOF) {
			if (c == '\n') {
				while (hash && (bytes >= hashbytes)) {
					(void) putchar('#');
					(void) fflush(stdout);
					hashbytes += HASHBYTES;
				}
// Szurgot: The following code is unncessary on Win32.
//				(void) fputcSocket(dout, '\r');
//				bytes++;
			}

			if (ipos >= bufsize) {
				fputSocket(dout,buf,ipos);
				if(!hash) (void) putchar('.');
				ipos=0;
			}
			buf[ipos]=c; ++ipos;
			bytes++;
		}
		if (ipos) {
			fputSocket(dout,buf,ipos);
			ipos=0;
		}
		if (hash) {
			if (bytes < hashbytes)
			(void) putchar('#');
			(void) putchar('\n');
			(void) fflush(stdout);
		}
		else {
			(void) putchar('.');
			(void) putchar('\n');
			(void) fflush(stdout);
		}
		if (ferror(fin))
			perror(local);
//		if (ferror(dout)) {
//			if (errno != EPIPE)
//				perror("netout");
//			bytes = -1;
//		}
		break;
		}
	}
	(void) gettimeofday(&stop, (struct timezone *)0);
	if (closefunc != NULL)
		(*closefunc)(fin);
	if(closesocket(dout)) {
		int iret=WSAGetLastError ();
		fprintf(stdout,"Error closing socket(%d)\n",iret);
		(void) fflush(stdout);
	}
	(void) getreply(0);
null();//	(void) signal(SIGINT, oldintr);
	if (oldintp)
null();//		(void) signal(SIGPIPE, oldintp);
	if (bytes > 0)
		ptransfer("sent", bytes, &start, &stop);
	return;
abort:
	(void) gettimeofday(&stop, (struct timezone *)0);
null();//	(void) signal(SIGINT, oldintr);
	if (oldintp)
null();//		(void) signal(SIGPIPE, oldintp);
	if (!cpend) {
		code = -1;
		return;
	}
	if (data >= 0) {
		(void) close(data);
		data = -1;
	}
	if (dout)
		if(closesocket(dout)) {
			int iret=WSAGetLastError ();
			fprintf(stdout,"Error closing socket(%d)\n",iret);
			(void) fflush(stdout);
		}

	(void) getreply(0);
	code = -1;
	if (closefunc != NULL && fin != NULL)
		(*closefunc)(fin);
	if (bytes > 0)
		ptransfer("sent", bytes, &start, &stop);
}

jmp_buf	recvabort;

#if 0
void abortrecv()
{

	mflag = 0;
	abrtflag = 0;
	printf("\n");
	(void) fflush(stdout);
	longjmp(recvabort, 1);
}
#endif

void recvrequest(const char *cmd, const char *local, const char *remote, const char *mode,
                int printnames)
{
	FILE *fout = stdout;
	int din = 0;
	int (*closefunc)();
	void (*oldintr)(int), (*oldintp)(int);
	int oldverbose = 0, oldtype = 0, is_retr, tcrflag, nfnd, bare_lfs = 0;
	char msg;
//	static char *buf; // Szurgot: Shouldn't this go SOMEWHERE?
	char buf[1024];
	static int bufsize = 1024;
	long bytes = 0, hashbytes = HASHBYTES;
//	struct
		fd_set mask;
	register int c, d;
	struct timeval start, stop;
//	struct stat st;

	is_retr = strcmp(cmd, "RETR") == 0;
	if (is_retr && verbose && printnames) {
		if (local && *local != '-')
			printf("local: %s ", local);
		if (remote)
			printf("remote: %s\n", remote);
		(void) fflush(stdout);
	}
	if (proxy && is_retr) {
		proxtrans(cmd, local, remote);
		return;
	}
	closefunc = NULL;
	oldintr = NULL;
	oldintp = NULL;
	tcrflag = !crflag && is_retr;
	if (setjmp(recvabort)) {
		while (cpend) {
			(void) getreply(0);
		}
		if (data >= 0) {
			(void) close(data);
			data = -1;
		}
		if (oldintr)
null();//			(void) signal(SIGINT, oldintr);
		code = -1;
		return;
	}
null();//	oldintr = signal(SIGINT, abortrecv);
	if (strcmp(local, "-") && *local != '|') {
#ifndef _WIN32
// This whole thing is a problem... access Won't work on non-existent files
		if (access(local, 2) < 0) {
			char *dir = rindex(local, '/');

			if (errno != ENOENT && errno != EACCES) {
				perror(local);
				(void) signal(SIGINT, oldintr);
				code = -1;
				return;
			}
			if (dir != NULL)
				*dir = 0;
			d = access(dir ? local : ".", 2);
			if (dir != NULL)
				*dir = '/';
			if (d < 0) {
				perror(local);
				(void) signal(SIGINT, oldintr);
				code = -1;
				return;
			}
			if (!runique && errno == EACCES &&
			    chmod(local, 0600) < 0) {
				perror(local);
				(void) signal(SIGINT, oldintr);
				code = -1;
				return;
			}
			if (runique && errno == EACCES &&
			    (local = gunique(local)) == NULL) {
				(void) signal(SIGINT, oldintr);
				code = -1;
				return;
			}
		}
		else if (runique && (local = gunique(local)) == NULL) {
			(void) signal(SIGINT, oldintr);
			code = -1;
			return;
		}
#endif
	}
	if (initconn()) {
null();//		(void) signal(SIGINT, oldintr);
		code = -1;
		return;
	}
	if (setjmp(recvabort))
		goto abort;
	if (!is_retr) {
		if (type != TYPE_A && (allbinary == 0 || type != TYPE_I)) {
			oldtype = type;
			oldverbose = verbose;
			if (!debug)
				verbose = 0;
			setascii();
			verbose = oldverbose;
		}
	} else if (restart_point) {
		if (command("REST %ld", (long) restart_point) != CONTINUE)
			return;
	}
	if (remote) {
		if (command("%s %s", cmd, remote) != PRELIM) {
null();//			(void) signal(SIGINT, oldintr);
			if (oldtype) {
				if (!debug)
					verbose = 0;
				switch (oldtype) {
					case TYPE_I:
						setbinary();
						break;
					case TYPE_E:
						setebcdic();
						break;
					case TYPE_L:
						settenex();
						break;
				}
				verbose = oldverbose;
			}
			return;
		}
	} else {
		if (command("%s", cmd) != PRELIM) {
null();//			(void) signal(SIGINT, oldintr);
			if (oldtype) {
				if (!debug)
					verbose = 0;
				switch (oldtype) {
					case TYPE_I:
						setbinary();
						break;
					case TYPE_E:
						setebcdic();
						break;
					case TYPE_L:
						settenex();
						break;
				}
				verbose = oldverbose;
			}
			return;
		}
	}
	din = dataconn("r");
	if (!din)
		goto abort;
	if (strcmp(local, "-") == 0)
		fout = stdout;
	else if (*local == '|') {
null();//		oldintp = signal(SIGPIPE, SIG_IGN);
		fout = _popen(local + 1, "w");
		if (fout == NULL) {
			perror(local+1);
			goto abort;
		}
		closefunc = _pclose;
	} else {
		fout = fopen(local, mode);
		if (fout == NULL) {
			perror(local);
			goto abort;
		}
		closefunc = fclose;
	}
	(void) gettimeofday(&start, (struct timezone *)0);
	switch (type) {

	case TYPE_I:
	case TYPE_L:
		if (restart_point &&
		    lseek(fileno(fout), (long) restart_point, L_SET) < 0) {
			perror(local);
			if (closefunc != NULL)
				(*closefunc)(fout);
			return;
		}
		errno = d = 0;
//		while ((c = recv(din, buf, bufsize, 1)) > 0) {
//			if ((d = write(fileno(fout), buf, c)) != c)
//			if ((d = write(fileno(fout), buf, c)) != c)
//				break;
		while ((c = recv(din, buf, bufsize, 0)) > 0) {
			write(fileno(fout), buf, c);
			bytes += c;
			if (hash) {
				while (bytes >= hashbytes) {
					(void) putchar('#');
					hashbytes += HASHBYTES;
				}
				(void) fflush(stdout);
			}
		}
		if (hash && bytes > 0) {
			if (bytes < HASHBYTES)
				(void) putchar('#');
			(void) putchar('\n');
			(void) fflush(stdout);
		}
//		if (c < 0) {
//			if (errno != EPIPE)
//				perror("netin");
//			bytes = -1;
//		}
//		if (d < c) {
//			if (d < 0)
//				perror(local);
//			else
//				fprintf(stderr, "%s: short write\n", local);
//		}
		break;

	case TYPE_A:
		if (restart_point) {
			register int i, n, c;

			if (fseek(fout, 0L, L_SET) < 0)
				goto done;
			n = restart_point;
			i = 0;
			while (i++ < n) {
				if ((c=getc(fout)) == EOF)
					goto done;
				if (c == '\n')
					i++;
			}
			if (fseek(fout, 0L, L_INCR) < 0) {
done:
				perror(local);
				if (closefunc != NULL)
					(*closefunc)(fout);
				return;
			}
		}
		while ((c = fgetcSocket(din)) != EOF) {
			if (c == '\n')
				bare_lfs++;
			while (c == '\r') {
				while (hash && (bytes >= hashbytes)) {
					(void) putchar('#');
					(void) fflush(stdout);
					hashbytes += HASHBYTES;
				}
				bytes++;
				if ((c = fgetcSocket(din)) != '\n' || tcrflag) {
					if (ferror(fout))
					goto break2;
					(void) putc('\r', fout);
					if (c == '\0') {
						bytes++;
						goto contin2;
					}
					if (c == EOF)
						goto contin2;
				}
			}
			(void) putc(c, fout);
			bytes++;
	contin2:	;
		}
break2:
		if (bare_lfs) {
			printf("WARNING! %d bare linefeeds received in ASCII mode\n", bare_lfs);
			printf("File may not have transferred correctly.\n");
			(void) fflush(stdout);
		}
		if (hash) {
			if (bytes < hashbytes)
				(void) putchar('#');
			(void) putchar('\n');
			(void) fflush(stdout);
		}
//		if (ferror(din)) {
//			if (errno != EPIPE)
//				perror("netin");
//			bytes = -1;
//		}
		if (ferror(fout))
			perror(local);
		break;
	}
	if (closefunc != NULL)
		(*closefunc)(fout);
null();//	(void) signal(SIGINT, oldintr);
	if (oldintp)
null();//		(void) signal(SIGPIPE, oldintp);
	(void) gettimeofday(&stop, (struct timezone *)0);
	if(closesocket(din)) {
		int iret=WSAGetLastError ();
		fprintf(stdout,"Error closing socket(%d)\n",iret);
		(void) fflush(stdout);
	}

	(void) getreply(0);
	if (bytes > 0 && is_retr)
		ptransfer("received", bytes, &start, &stop);
	if (oldtype) {
		if (!debug)
			verbose = 0;
		switch (oldtype) {
			case TYPE_I:
				setbinary();
				break;
			case TYPE_E:
				setebcdic();
				break;
			case TYPE_L:
				settenex();
				break;
		}
		verbose = oldverbose;
	}
	return;
abort:

/* abort using RFC959 recommended IP,SYNC sequence  */

	(void) gettimeofday(&stop, (struct timezone *)0);
	if (oldintp)
null();//		(void) signal(SIGPIPE, oldintr);
null();//	(void) signal(SIGINT,SIG_IGN);
	if (oldtype) {
		if (!debug)
			verbose = 0;
		switch (oldtype) {
			case TYPE_I:
				setbinary();
				break;
			case TYPE_E:
				setebcdic();
				break;
			case TYPE_L:
				settenex();
				break;
		}
		verbose = oldverbose;
	}
	if (!cpend) {
		code = -1;
null();//		(void) signal(SIGINT,oldintr);
		return;
	}

	fprintfSocket(cout,"%c%c",IAC,IP);
	msg = (char)IAC;
/* send IAC in urgent mode instead of DM because UNIX places oob mark */
/* after urgent byte rather than before as now is protocol */
	if (send(cout,&msg,1,MSG_OOB) != 1) {
		perror("abort");
	}
	fprintfSocket(cout,"%cABOR\r\n",DM);
	FD_ZERO(&mask);
	FD_SET(cin, &mask); // Need to correct this
	if (din) {
		FD_SET(din, &mask); // Need to correct this
	}
	if ((nfnd = empty(&mask,10)) <= 0) {
		if (nfnd < 0) {
			perror("abort");
		}
		code = -1;
		lostpeer();
	}
	if (din && FD_ISSET(din, &mask)) {
		while ((c = recv(din, buf, bufsize, 0)) > 0)
			;
	}
	if ((c = getreply(0)) == ERROR && code == 552) { /* needed for nic style abort */
		if (data >= 0) {
			(void) close(data);
			data = -1;
		}
		(void) getreply(0);
	}
	(void) getreply(0);
	code = -1;
	if (data >= 0) {
		(void) close(data);
		data = -1;
	}
	if (closefunc != NULL && fout != NULL)
		(*closefunc)(fout);
	if (din)
		if(closesocket(din)) {
			int iret=WSAGetLastError ();
			fprintf(stdout,"Error closing socket(%d)\n",iret);
			(void) fflush(stdout);
		}

	if (bytes > 0)
		ptransfer("received", bytes, &start, &stop);
null();//	(void) signal(SIGINT,oldintr);
}

/*
 * Need to start a listen on the data channel
 * before we send the command, otherwise the
 * server's connect may fail.
 */
int sendport = -1;

int
initconn()
{
	register char *p, *a;
	int result, len, tmpno = 0;
	int on = 1;
	int a0, a1, a2, a3, p0, p1;


	if (passivemode) {
		data = socket(AF_INET, SOCK_STREAM, 0);
		if (data < 0) {
			perror("ftp: socket");
			return(1);
		}
		if ((options & SO_DEBUG) &&
		    setsockopt(data, SOL_SOCKET, SO_DEBUG, (char *)&on,
		    sizeof (on)) < 0)
			perror("ftp: setsockopt (ignored)");
		if (command("PASV") != COMPLETE) {
			printf("Passive mode refused.\n");
			goto bad;
		}

		/*
		 * What we've got at this point is a string of comma
		 * separated one-byte unsigned integer values.
		 * The first four are the an IP address. The fifth is
		 * the MSB of the port number, the sixth is the LSB.
		 * From that we'll prepare a sockaddr_in.
		 */

		if (sscanf(pasv,"%d,%d,%d,%d,%d,%d",
		    &a0, &a1, &a2, &a3, &p0, &p1) != 6) {
			printf("Passive mode address scan failure. Shouldn't happen!\n");
			goto bad;
		}

		bzero(&data_addr, sizeof(data_addr));
		data_addr.sin_family = AF_INET;
		a = (char *)&data_addr.sin_addr.s_addr;
		a[0] = a0 & 0xff;
		a[1] = a1 & 0xff;
		a[2] = a2 & 0xff;
		a[3] = a3 & 0xff;
		p = (char *)&data_addr.sin_port;
		p[0] = p0 & 0xff;
		p[1] = p1 & 0xff;

		if (connect(data, (struct sockaddr *)&data_addr,
		    sizeof(data_addr)) < 0) {
			perror("ftp: connect");
			goto bad;
		}
		return(0);
	}


noport:
	data_addr = myctladdr;
	if (sendport)
		data_addr.sin_port = 0;	/* let system pick one */
	if (data != -1)
		(void) close (data);
	data = socket(AF_INET, SOCK_STREAM, 0);
	if (data < 0) {
		perror("ftp: socket");
		if (tmpno)
			sendport = 1;
		return (1);
	}
	if (!sendport)
		if (setsockopt(data, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof (on)) < 0) {
			perror("ftp: setsockopt (reuse address)");
			goto bad;
		}
	if (bind(data, (struct sockaddr *)&data_addr, sizeof (data_addr)) < 0) {
		perror("ftp: bind");
		goto bad;
	}
	if (options & SO_DEBUG &&
	    setsockopt(data, SOL_SOCKET, SO_DEBUG, (char *)&on, sizeof (on)) < 0)
		perror("ftp: setsockopt (ignored)");
	len = sizeof (data_addr);
	if (getsockname(data, (struct sockaddr *)&data_addr, &len) < 0) {
		perror("ftp: getsockname");
		goto bad;
	}
	if (listen(data, 1) < 0)
		perror("ftp: listen");
	if (sendport) {
		a = (char *)&data_addr.sin_addr;
		p = (char *)&data_addr.sin_port;
#define	UC(b)	(((int)b)&0xff)
		result =
		    command("PORT %d,%d,%d,%d,%d,%d",
		      UC(a[0]), UC(a[1]), UC(a[2]), UC(a[3]),
		      UC(p[0]), UC(p[1]));
		if (result == ERROR && sendport == -1) {
			sendport = 0;
			tmpno = 1;
			goto noport;
		}
		return (result != COMPLETE);
	}
	if (tmpno)
		sendport = 1;
	return (0);
bad:
	(void) fflush(stdout);
	(void) close(data), data = -1;
	if (tmpno)
		sendport = 1;
	return (1);
}

int dataconn(const char *mode)
{
	struct sockaddr_in from;
	int s, fromlen = sizeof (from);

	if (passivemode)
		return (data);

	s = accept(data, (struct sockaddr *) &from, &fromlen);
	if (s < 0) {
		perror("ftp: accept");
		(void) closesocket(data), data = -1;
		return 0;
	}
	if(closesocket(data)) {
		int iret=WSAGetLastError ();
		fprintf(stdout,"Error closing socket(%d)\n",iret);
		(void) fflush(stdout);
	}

	data = s;
	return (data);
}

void ptransfer(direction, bytes, t0, t1)
	const char *direction;
	long bytes;
	struct timeval *t0, *t1;
{
	struct timeval td;
	double s, bs;

	if (verbose) {
		tvsub(&td, t1, t0);
		s = td.tv_sec + (td.tv_usec / 1000000.);
#define	nz(x)	((x) == 0 ? 1 : (x))
		bs = bytes / nz(s);
		printf("%ld bytes %s in %.1f seconds (%.0f Kbytes/s)\n",
		    bytes, direction, s, bs / 1024.);
		(void) fflush(stdout);
	}
}

/*tvadd(tsum, t0)
	struct timeval *tsum, *t0;
{

	tsum->tv_sec += t0->tv_sec;
	tsum->tv_usec += t0->tv_usec;
	if (tsum->tv_usec > 1000000)
		tsum->tv_sec++, tsum->tv_usec -= 1000000;
} */

void tvsub(tdiff, t1, t0)
	struct timeval *tdiff, *t1, *t0;
{

	tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
	tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
	if (tdiff->tv_usec < 0)
		tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}

void psabort(int flag)
{
	extern int abrtflag;

	abrtflag++;
}

void pswitch(int flag)
{
	extern int proxy, abrtflag;
	Sig_t oldintr;
	static struct comvars {
		int connect;
		char name[MAXHOSTNAMELEN];
		struct sockaddr_in mctl;
		struct sockaddr_in hctl;
		SOCKET in;
		SOCKET out;
		int tpe;
		int cpnd;
		int sunqe;
		int runqe;
		int mcse;
		int ntflg;
		char nti[17];
		char nto[17];
		int mapflg;
		char mi[MAXPATHLEN];
		char mo[MAXPATHLEN];
		} proxstruct, tmpstruct;
	struct comvars *ip, *op;

	abrtflag = 0;
	oldintr = signal(SIGINT, psabort);
	if (flag) {
		if (proxy)
			return;
		ip = &tmpstruct;
		op = &proxstruct;
		proxy++;
	}
	else {
		if (!proxy)
			return;
		ip = &proxstruct;
		op = &tmpstruct;
		proxy = 0;
	}
	ip->connect = connected;
	connected = op->connect;
	if (hostname) {
		(void) strncpy(ip->name, hostname, sizeof(ip->name) - 1);
		ip->name[strlen(ip->name)] = '\0';
	} else
		ip->name[0] = 0;
	hostname = op->name;
	ip->hctl = hisctladdr;
	hisctladdr = op->hctl;
	ip->mctl = myctladdr;
	myctladdr = op->mctl;
	ip->in = cin;
	cin = op->in;
	ip->out = cout;
	cout = op->out;
	ip->tpe = type;
	type = op->tpe;
	if (!type)
		type = 1;
	ip->cpnd = cpend;
	cpend = op->cpnd;
	ip->sunqe = sunique;
	sunique = op->sunqe;
	ip->runqe = runique;
	runique = op->runqe;
	ip->mcse = mcase;
	mcase = op->mcse;
	ip->ntflg = ntflag;
	ntflag = op->ntflg;
	(void) strncpy(ip->nti, ntin, 16);
	(ip->nti)[strlen(ip->nti)] = '\0';
	(void) strcpy(ntin, op->nti);
	(void) strncpy(ip->nto, ntout, 16);
	(ip->nto)[strlen(ip->nto)] = '\0';
	(void) strcpy(ntout, op->nto);
	ip->mapflg = mapflag;
	mapflag = op->mapflg;
	(void) strncpy(ip->mi, mapin, MAXPATHLEN - 1);
	(ip->mi)[strlen(ip->mi)] = '\0';
	(void) strcpy(mapin, op->mi);
	(void) strncpy(ip->mo, mapout, MAXPATHLEN - 1);
	(ip->mo)[strlen(ip->mo)] = '\0';
	(void) strcpy(mapout, op->mo);
//	(void) signal(SIGINT, oldintr);
	if (abrtflag) {
		abrtflag = 0;
		(*oldintr)(1);
	}
}

jmp_buf ptabort;
int ptabflg;

#if 0
void
abortpt()
{
	printf("\n");
	(void) fflush(stdout);
	ptabflg++;
	mflag = 0;
	abrtflag = 0;
	longjmp(ptabort, 1);
}
#endif

void proxtrans(cmd, local, remote)
	const char *cmd, *local, *remote;
{
//	void (*oldintr)(int);
	int tmptype, oldtype = 0, secndflag = 0, nfnd;
	extern jmp_buf ptabort;
	const char *cmd2;
//	struct
	fd_set mask;

	if (strcmp(cmd, "RETR"))
		cmd2 = "RETR";
	else
		cmd2 = runique ? "STOU" : "STOR";
	if (command("PASV") != COMPLETE) {
		printf("proxy server does not support third part transfers.\n");
		(void) fflush(stdout);
		return;
	}
	tmptype = type;
	pswitch(0);
	if (!connected) {
		printf("No primary connection\n");
		(void) fflush(stdout);
		pswitch(1);
		code = -1;
		return;
	}
	if (type != tmptype) {
		oldtype = type;
		switch (tmptype) {
			case TYPE_A:
				setascii();
				break;
			case TYPE_I:
				setbinary();
				break;
			case TYPE_E:
				setebcdic();
				break;
			case TYPE_L:
				settenex();
				break;
		}
	}
	if (command("PORT %s", pasv) != COMPLETE) {
		switch (oldtype) {
			case 0:
				break;
			case TYPE_A:
				setascii();
				break;
			case TYPE_I:
				setbinary();
				break;
			case TYPE_E:
				setebcdic();
				break;
			case TYPE_L:
				settenex();
				break;
		}
		pswitch(1);
		return;
	}
	if (setjmp(ptabort))
		goto abort;
null();//	oldintr = signal(SIGINT, abortpt);
	if (command("%s %s", cmd, remote) != PRELIM) {
null();//		(void) signal(SIGINT, oldintr);
		switch (oldtype) {
			case 0:
				break;
			case TYPE_A:
				setascii();
				break;
			case TYPE_I:
				setbinary();
				break;
			case TYPE_E:
				setebcdic();
				break;
			case TYPE_L:
				settenex();
				break;
		}
		pswitch(1);
		return;
	}
	sleep(2);
	pswitch(1);
	secndflag++;
	if (command("%s %s", cmd2, local) != PRELIM)
		goto abort;
	ptflag++;
	(void) getreply(0);
	pswitch(0);
	(void) getreply(0);
null();//	(void) signal(SIGINT, oldintr);
	switch (oldtype) {
		case 0:
			break;
		case TYPE_A:
			setascii();
			break;
		case TYPE_I:
			setbinary();
			break;
		case TYPE_E:
			setebcdic();
			break;
		case TYPE_L:
			settenex();
			break;
	}
	pswitch(1);
	ptflag = 0;
	printf("local: %s remote: %s\n", local, remote);
	(void) fflush(stdout);
	return;
abort:
null();//	(void) signal(SIGINT, SIG_IGN);
	ptflag = 0;
	if (strcmp(cmd, "RETR") && !proxy)
		pswitch(1);
	else if (!strcmp(cmd, "RETR") && proxy)
		pswitch(0);
	if (!cpend && !secndflag) {  /* only here if cmd = "STOR" (proxy=1) */
		if (command("%s %s", cmd2, local) != PRELIM) {
			pswitch(0);
			switch (oldtype) {
				case 0:
					break;
				case TYPE_A:
					setascii();
					break;
				case TYPE_I:
					setbinary();
					break;
				case TYPE_E:
					setebcdic();
					break;
				case TYPE_L:
					settenex();
					break;
			}
			if (cpend) {
				char msg[2];

				fprintfSocket(cout,"%c%c",IAC,IP);
				*msg = (char) IAC;
				*(msg+1) = (char) DM;
				if (send(cout,msg,2,MSG_OOB) != 2)
					perror("abort");
				fprintfSocket(cout,"ABOR\r\n");
				FD_ZERO(&mask);
//				FD_SET(fileno(cin), &mask); // Chris: Need to correct this
				if ((nfnd = empty(&mask,10)) <= 0) {
					if (nfnd < 0) {
						perror("abort");
					}
					if (ptabflg)
						code = -1;
					lostpeer();
				}
				(void) getreply(0);
				(void) getreply(0);
			}
		}
		pswitch(1);
		if (ptabflg)
			code = -1;
null();//		(void) signal(SIGINT, oldintr);
		return;
	}
	if (cpend) {
		char msg[2];

		fprintfSocket(cout,"%c%c",IAC,IP);
		*msg = (char)IAC;
		*(msg+1) = (char)DM;
		if (send(cout,msg,2,MSG_OOB) != 2)
			perror("abort");
		fprintfSocket(cout,"ABOR\r\n");
		FD_ZERO(&mask);
//		FD_SET(fileno(cin), &mask); // Chris: Need to correct this...
		if ((nfnd = empty(&mask,10)) <= 0) {
			if (nfnd < 0) {
				perror("abort");
			}
			if (ptabflg)
				code = -1;
			lostpeer();
		}
		(void) getreply(0);
		(void) getreply(0);
	}
	pswitch(!proxy);
	if (!cpend && !secndflag) {  /* only if cmd = "RETR" (proxy=1) */
		if (command("%s %s", cmd2, local) != PRELIM) {
			pswitch(0);
			switch (oldtype) {
				case 0:
					break;
				case TYPE_A:
					setascii();
					break;
				case TYPE_I:
					setbinary();
					break;
				case TYPE_E:
					setebcdic();
					break;
				case TYPE_L:
					settenex();
					break;
			}
			if (cpend) {
				char msg[2];

				fprintfSocket(cout,"%c%c",IAC,IP);
				*msg = (char)IAC;
				*(msg+1) = (char)DM;
				if (send(cout,msg,2,MSG_OOB) != 2)
					perror("abort");
				fprintfSocket(cout,"ABOR\r\n");
				FD_ZERO(&mask);
//				FD_SET(fileno(cin), &mask); // Chris:
				if ((nfnd = empty(&mask,10)) <= 0) {
					if (nfnd < 0) {
						perror("abort");
					}
					if (ptabflg)
						code = -1;
					lostpeer();
				}
				(void) getreply(0);
				(void) getreply(0);
			}
			pswitch(1);
			if (ptabflg)
				code = -1;
null();//			(void) signal(SIGINT, oldintr);
			return;
		}
	}
	if (cpend) {
		char msg[2];

		fprintfSocket(cout,"%c%c",IAC,IP);
		*msg = (char)IAC;
		*(msg+1) = (char)DM;
		if (send(cout,msg,2,MSG_OOB) != 2)
			perror("abort");
		fprintfSocket(cout,"ABOR\r\n");
		FD_ZERO(&mask);
//		FD_SET(fileno(cin), &mask); // Chris:
		if ((nfnd = empty(&mask,10)) <= 0) {
			if (nfnd < 0) {
				perror("abort");
			}
			if (ptabflg)
				code = -1;
			lostpeer();
		}
		(void) getreply(0);
		(void) getreply(0);
	}
	pswitch(!proxy);
	if (cpend) {
		FD_ZERO(&mask);
//		FD_SET(fileno(cin), &mask); // Chris:
		if ((nfnd = empty(&mask,10)) <= 0) {
			if (nfnd < 0) {
				perror("abort");
			}
			if (ptabflg)
				code = -1;
			lostpeer();
		}
		(void) getreply(0);
		(void) getreply(0);
	}
	if (proxy)
		pswitch(0);
	switch (oldtype) {
		case 0:
			break;
		case TYPE_A:
			setascii();
			break;
		case TYPE_I:
			setbinary();
			break;
		case TYPE_E:
			setebcdic();
			break;
		case TYPE_L:
			settenex();
			break;
	}
	pswitch(1);
	if (ptabflg)
		code = -1;
null();//	(void) signal(SIGINT, oldintr);
}

void reset()
{
//	struct
	fd_set mask;
	int nfnd = 1;

	FD_ZERO(&mask);
	while (nfnd > 0) {
//		FD_SET(fileno(cin), &mask); // Chris
		if ((nfnd = empty(&mask,0)) < 0) {
			perror("reset");
			code = -1;
			lostpeer();
		}
		else if (nfnd) {
			(void) getreply(0);
		}
	}
}

#if 0
char *
gunique(local)
	char *local;
{
	static char new[MAXPATHLEN];
	char *cp = rindex(local, '/');
	int d, count=0;
	char ext = '1';

	if (cp)
		*cp = '\0';
	d = access(cp ? local : ".", 2);
	if (cp)
		*cp = '/';
	if (d < 0) {
		perror(local);
		return((char *) 0);
	}
	(void) strcpy(new, local);
	cp = new + strlen(new);
	*cp++ = '.';
	while (!d) {
		if (++count == 100) {
			printf("runique: can't find unique file name.\n");
			(void) fflush(stdout);
			return((char *) 0);
		}
		*cp++ = ext;
		*cp = '\0';
		if (ext == '9')
			ext = '0';
		else
			ext++;
		if ((d = access(new, 0)) < 0)
			break;
		if (ext != '0')
			cp--;
		else if (*(cp - 2) == '.')
			*(cp - 1) = '1';
		else {
			*(cp - 2) = *(cp - 2) + 1;
			cp--;
		}
	}
	return(new);
}
#endif

int null(void)
{
	return 0;
}
