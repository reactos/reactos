/* util.c
 *
 * Copyright (c) 1992-2001 by Mike Gleason.
 * All rights reserved.
 * 
 */

#include "syshdrs.h"
#include "shell.h"
#include "trace.h"
#include "util.h"

uid_t gUid;
char gUser[32];
char gHome[256];
char gShell[256];
char gOurDirectoryPath[260];
char gOurInstallationPath[260];
#ifdef ncftp
static int gResolveSig;
#endif

#if defined(WIN32) || defined(_WINDOWS)
#elif defined(HAVE_SIGSETJMP)
sigjmp_buf gGetHostByNameJmp;
#else	/* HAVE_SIGSETJMP */
jmp_buf gGetHostByNameJmp;
#endif	/* HAVE_SIGSETJMP */

#ifndef HAVE_MEMMOVE
void *memmove(void *dst0, void *src0, size_t length);
#endif

static const unsigned char B64EncodeTable[64] =
{
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'
};

static const unsigned char B64DecodeTable[256] =
{
	'\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',		/* 000-007 */
	'\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',		/* 010-017 */
	'\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',		/* 020-027 */
	'\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',		/* 030-037 */
	'\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',		/* 040-047 */
	'\177', '\177', '\177', '\76', '\177', '\177', '\177', '\77',	/* 050-057 */
	'\64', '\65', '\66', '\67', '\70', '\71', '\72', '\73',		/* 060-067 */
	'\74', '\75', '\177', '\177', '\177', '\100', '\177', '\177',	/* 070-077 */
	'\177', '\0', '\1', '\2', '\3', '\4', '\5', '\6',	/* 100-107 */
	'\7', '\10', '\11', '\12', '\13', '\14', '\15', '\16',	/* 110-117 */
	'\17', '\20', '\21', '\22', '\23', '\24', '\25', '\26',		/* 120-127 */
	'\27', '\30', '\31', '\177', '\177', '\177', '\177', '\177',	/* 130-137 */
	'\177', '\32', '\33', '\34', '\35', '\36', '\37', '\40',	/* 140-147 */
	'\41', '\42', '\43', '\44', '\45', '\46', '\47', '\50',		/* 150-157 */
	'\51', '\52', '\53', '\54', '\55', '\56', '\57', '\60',		/* 160-167 */
	'\61', '\62', '\63', '\177', '\177', '\177', '\177', '\177',	/* 170-177 */
	'\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',		/* 200-207 */
	'\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',		/* 210-217 */
	'\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',		/* 220-227 */
	'\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',		/* 230-237 */
	'\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',		/* 240-247 */
	'\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',		/* 250-257 */
	'\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',		/* 260-267 */
	'\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',		/* 270-277 */
	'\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',		/* 300-307 */
	'\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',		/* 310-317 */
	'\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',		/* 320-327 */
	'\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',		/* 330-337 */
	'\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',		/* 340-347 */
	'\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',		/* 350-357 */
	'\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',		/* 360-367 */
	'\177', '\177', '\177', '\177', '\177', '\177', '\177', '\177',		/* 370-377 */
};

void
ToBase64(void *dst0, const void *src0, size_t n, int terminate)
{
	unsigned char *dst;
	const unsigned char *src, *srclim;
	unsigned int c0, c1, c2;
	unsigned int ch;

	src = src0;
	srclim = src + n;
	dst = dst0;

	while (src < srclim) {
		c0 = *src++;
		if (src < srclim) {
			c1 = *src++;
		} else {
			c1 = 0;
		}
		if (src < srclim) {
			c2 = *src++;
		} else {
			c2 = 0;
		}

		ch = c0 >> 2;
		dst[0] = B64EncodeTable[ch & 077];

		ch = ((c0 << 4) & 060) | ((c1 >> 4) & 017);
		dst[1] = B64EncodeTable[ch & 077];

		ch = ((c1 << 2) & 074) | ((c2 >> 6) & 03);
		dst[2] = B64EncodeTable[ch & 077];

		ch = (c2 & 077);
		dst[3] = B64EncodeTable[ch & 077];

		dst += 4;
	}
	if (terminate != 0)
		*dst = '\0';
}						       /* ToBase64 */



void
FromBase64(void *dst0, const void *src0, size_t n, int terminate)
{
	unsigned char *dst;
	const unsigned char *src, *srclim;
	unsigned int c0, c1, c2, c3;
	unsigned int ch;

	src = src0;
	srclim = src + n;
	dst = dst0;

	while (src < srclim) {
		c0 = *src++;
		if (src < srclim) {
			c1 = *src++;
		} else {
			c1 = 0;
		}
		if (src < srclim) {
			c2 = *src++;
		} else {
			c2 = 0;
		}
		if (src < srclim) {
			c3 = *src++;
		} else {
			c3 = 0;
		}

		ch = (((unsigned int) B64DecodeTable[c0]) << 2) | (((unsigned int) B64DecodeTable[c1]) >> 4);
		dst[0] = (unsigned char) ch;

		ch = (((unsigned int) B64DecodeTable[c1]) << 4) | (((unsigned int) B64DecodeTable[c2]) >> 2);
		dst[1] = (unsigned char) ch;

		ch = (((unsigned int) B64DecodeTable[c2]) << 6) | (((unsigned int) B64DecodeTable[c3]));
		dst[2] = (unsigned char) ch;

		dst += 3;
	}
	if (terminate != 0)
		*dst = '\0';
}						       /* FromBase64 */

/* This should only be called if the program wouldn't function
 * usefully without the memory requested.
 */
void
OutOfMemory(void)
{
	(void) fprintf(stderr, "Out of memory!\n");
	exit(1);
}	/* OutOfMemory */



void
MyInetAddr(char *dst, size_t siz, char **src, int i)
{
	struct in_addr *ia;
#ifndef HAVE_INET_NTOP
	char *cp;
#endif

	(void) Strncpy(dst, "???", siz);
	if (src != (char **) 0) {
		ia = (struct in_addr *) src[i];
#ifdef HAVE_INET_NTOP	/* Mostly to workaround bug in IRIX 6.5's inet_ntoa */
		(void) inet_ntop(AF_INET, ia, dst, siz - 1);
#else
		cp = inet_ntoa(*ia);
		if ((cp != (char *) 0) && (cp != (char *) -1) && (cp[0] != '\0'))
			(void) Strncpy(dst, cp, siz);
#endif
	}
}	/* MyInetAddr */




/* On entry, you should have 'host' be set to a symbolic name (like
 * cse.unl.edu), or set to a numeric address (like 129.93.3.1).
 * If the function fails, it will return NULL, but if the host was
 * a numeric style address, you'll have the ip_address to fall back on.
 */

struct hostent *
GetHostEntry(const char *host, struct in_addr *ip_address)
{
	struct in_addr ip;
	struct hostent *hp;
	
	/* See if the host was given in the dotted IP format, like "36.44.0.2."
	 * If it was, inet_addr will convert that to a 32-bit binary value;
	 * it not, inet_addr will return (-1L).
	 */
	ip.s_addr = inet_addr(host);
	if (ip.s_addr != INADDR_NONE) {
		hp = gethostbyaddr((char *) &ip, (int) sizeof(ip), AF_INET);
	} else {
		/* No IP address, so it must be a hostname, like ftp.wustl.edu. */
		hp = gethostbyname(host);
		if (hp != NULL)
			ip = * (struct in_addr *) hp->h_addr_list;
	}
	if (ip_address != NULL)
		*ip_address = ip;
	return (hp);
}	/* GetHostEntry */



/* This simplifies a pathname, by converting it to the
 * equivalent of "cd $dir ; dir=`pwd`".  In other words,
 * if $PWD==/usr/spool/uucp, and you had a path like
 * "$PWD/../tmp////./../xx/", it would be converted to
 * "/usr/spool/xx".
 */
void
CompressPath(char *const dst, const char *const src, const size_t dsize)
{
	int c;
	const char *s;
	char *d, *lim;
	char *a, *b;

	if (src[0] == '\0') {
		*dst = '\0';
		return;
	}

	s = src;
	d = dst;
	lim = d + dsize - 1;	/* leave room for nul byte. */
	for (;;) {
		c = *s;
		if (c == '.') {
			if (((s == src) || (s[-1] == '/')) && ((s[1] == '/') || (s[1] == '\0'))) {
				/* Don't copy "./" */
				if (s[1] == '/')
					++s;
				++s;
			} else if (d < lim) {
				*d++ = *s++;
			} else {
				++s;
			}
		} else if (c == '/') {
			/* Don't copy multiple slashes. */
			if (d < lim)
				*d++ = *s++;
			else
				++s;
			for (;;) {
				c = *s;
				if (c == '/') {
					/* Don't copy multiple slashes. */
					++s;
				} else if (c == '.') {
					c = s[1];
					if (c == '/') {
						/* Skip "./" */
						s += 2;
					} else if (c == '\0') {
						/* Skip "./" */
						s += 1;
					} else {
						break;
					}
				} else {
					break;
				}
			}
		} else if (c == '\0') {
			/* Remove trailing slash. */
			if ((d[-1] == '/') && (d > (dst + 1)))
				d[-1] = '\0';
			*d = '\0';
			break;
		} else if (d < lim) {
			*d++ = *s++;
		} else {
			++s;
		}
	}
	a = dst;

	/* fprintf(stderr, "<%s>\n", dst); */
	/* Go through and remove .. in the path when we know what the
	 * parent directory is.  After we get done with this, the only
	 * .. nodes in the path will be at the front.
	 */
	while (*a != '\0') {
		b = a;
		for (;;) {
			/* Get the next node in the path. */
			if (*a == '\0')
				return;
			if (*a == '/') {
				++a;
				break;
			}
			++a;
		}
		if ((b[0] == '.') && (b[1] == '.')) {
			if (b[2] == '/') {
				/* We don't know what the parent of this
				 * node would be.
				 */
				continue;
			}
		}
		if ((a[0] == '.') && (a[1] == '.')) {
			if (a[2] == '/') {
				/* Remove the .. node and the one before it. */
				if ((b == dst) && (*dst == '/'))
					(void) memmove(b + 1, a + 3, strlen(a + 3) + 1);
				else
					(void) memmove(b, a + 3, strlen(a + 3) + 1);
				a = dst;	/* Start over. */
			} else if (a[2] == '\0') {
				/* Remove a trailing .. like:  /aaa/bbb/.. */
				if ((b <= dst + 1) && (*dst == '/'))
					dst[1] = '\0';
				else
					b[-1] = '\0';
				a = dst;	/* Start over. */
			} else {
				/* continue processing this node.
				 * It is probably some bogus path,
				 * like ".../", "..foo/", etc.
				 */
			}
		}
	}
}	/* CompressPath */



void
PathCat(char *const dst, const size_t dsize, const char *const cwd, const char *const src)
{
	char *cp;
	char tmp[512];

	if (src[0] == '/') {
		CompressPath(dst, src, dsize);
		return;
	}
	cp = Strnpcpy(tmp, (char *) cwd, sizeof(tmp) - 1);
	*cp++ = '/';
	*cp = '\0';
	(void) Strnpcat(cp, (char *) src, sizeof(tmp) - (cp - tmp));
	CompressPath(dst, tmp, dsize);
}	/* PathCat */



char *
FileToURL(char *url, size_t urlsize, const char *const fn, const char *const rcwd, const char *const startdir, const char *const user, const char *const pass, const char *const hname, const unsigned int port)
{
	size_t ulen, dsize;
	char *dst, pbuf[32];
	int isUser;

	/* //<user>:<password>@<host>:<port>/<url-path> */
	/* Note that if an absolute path is given,
	 * you need to escape the first entry, i.e. /pub -> %2Fpub
	 */
	(void) Strncpy(url, "ftp://", urlsize);
	isUser = 0;
	if ((user != NULL) && (user[0] != '\0') && (strcmp(user, "anonymous") != 0) && (strcmp(user, "ftp") != 0)) {
		isUser = 1;
		(void) Strncat(url, user, urlsize);
		if ((pass != NULL) && (pass[0] != '\0')) {
			(void) Strncat(url, ":", urlsize);
			(void) Strncat(url, "PASSWORD", urlsize);
		}
		(void) Strncat(url, "@", urlsize);
	}
	(void) Strncat(url, hname, urlsize);
	if ((port != 21) && (port != 0)) {
		(void) sprintf(pbuf, ":%u", (unsigned int) port);
		(void) Strncat(url, pbuf, urlsize);
	}

	ulen = strlen(url);
	dst = url + ulen;
	dsize = urlsize - ulen;
	PathCat(dst, dsize, rcwd, fn);
	if ((startdir != NULL) && (startdir[0] != '\0') && (startdir[1] /* i.e. not "/" */ != '\0')) {
		if (strncmp(dst, startdir, strlen(startdir)) == 0) {
			/* Form relative URL. */
			memmove(dst, dst + strlen(startdir), strlen(dst) - strlen(startdir) + 1);
		} else if (isUser != 0) {
			/* Absolute URL, but different from start dir.
			 * Make sure to use %2f as first slash so that
			 * the translation uses "/pub" instead of "pub"
			 * since the / characters are just delimiters.
			 */
			dst[dsize - 1] = '\0';
			dst[dsize - 2] = '\0';
			dst[dsize - 3] = '\0';
			dst[dsize - 4] = '\0';
			memmove(dst + 4, dst + 1, strlen(dst + 1));
			dst[0] = '/';
			dst[1] = '%';
			dst[2] = '2';
			dst[3] = 'F';
		}
	}

	return (url);
}	/* FileToURL */




/* This will abbreviate a string so that it fits into max characters.
 * It will use ellipses as appropriate.  Make sure the string has
 * at least max + 1 characters allocated for it.
 */
void
AbbrevStr(char *dst, const char *src, size_t max, int mode)
{
	int len;

	len = (int) strlen(src);
	if (len > (int) max) {
		if (mode == 0) {
			/* ...Put ellipses at left */
			(void) strcpy(dst, "...");
			(void) Strncat(dst, (char *) src + len - (int) max + 3, max + 1);
		} else {
			/* Put ellipses at right... */
			(void) Strncpy(dst, (char *) src, max + 1);
			(void) strcpy(dst + max - 3, "...");
		}
	} else {
		(void) Strncpy(dst, (char *) src, max + 1);
	}
}	/* AbbrevStr */




char *
Path(char *const dst, const size_t siz, const char *const parent, const char *const fname)
{
	(void) Strncpy(dst, parent, siz);
	(void) Strncat(dst, LOCAL_PATH_DELIM_STR, siz);
	return (Strncat(dst, fname, siz));
}	/* Path */




char *
OurDirectoryPath(char *const dst, const size_t siz, const char *const fname)
{
	return (Path(dst, siz, gOurDirectoryPath, fname));
}	/* OurDirectoryPath */



char *
OurInstallationPath(char *const dst, const size_t siz, const char *const fname)
{
	return (Path(dst, siz, gOurInstallationPath, fname));
}	/* OurInstallationPath */




/* Create, if necessary, a directory in the user's home directory to
 * put our incredibly important stuff in.
 */
void
InitOurDirectory(void)
{
#if defined(WIN32) || defined(_WINDOWS)
	DWORD dwType, dwSize;
	HKEY hkey;
	char *cp;
	int rc;

	ZeroMemory(gOurDirectoryPath, (DWORD) sizeof(gOurDirectoryPath));
	ZeroMemory(gOurInstallationPath, (DWORD) sizeof(gOurInstallationPath));

	if (RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\ncftp.exe",
		(DWORD) 0,
		KEY_QUERY_VALUE,
		&hkey) == ERROR_SUCCESS)
	{
		dwSize = (DWORD) (sizeof(gOurInstallationPath) - 1);
		dwType = 0;
		if (RegQueryValueEx(
			hkey, 
			NULL, 
			(DWORD *) 0, 
			&dwType, 
			(LPBYTE) gOurInstallationPath, 
			&dwSize) == ERROR_SUCCESS)
		{
			// This gave us the path to ncftp.exe;
			// But we use a subdirectory in that directory.
			//
			cp = StrRFindLocalPathDelim(gOurInstallationPath);
			if (cp == NULL)
				ZeroMemory(gOurInstallationPath, (DWORD) sizeof(gOurInstallationPath));
			else
				ZeroMemory(cp, (DWORD) (cp - gOurInstallationPath));
		}
		RegCloseKey(hkey);
	}
	
	if (gOurInstallationPath[0] == '\0') {
		if (GetModuleFileName(NULL, gOurInstallationPath, (DWORD) sizeof(gOurInstallationPath) - 1) <= 0) {
			ZeroMemory(gOurInstallationPath, (DWORD) sizeof(gOurInstallationPath));
		} else {
			// This gave us the path to the current .exe;
			// But we use a subdirectory in that directory.
			//
			cp = StrRFindLocalPathDelim(gOurInstallationPath);
			if (cp == NULL)
				ZeroMemory(gOurInstallationPath, (DWORD) sizeof(gOurInstallationPath));
			else
				ZeroMemory(cp, (DWORD) (cp - gOurInstallationPath));
		}
	}

	if (gOurInstallationPath[0] != '\0') {
		if ((cp = getenv("NCFTPDIR")) != NULL) {
			if (*cp == '"')
				cp++;
			(void) STRNCPY(gOurDirectoryPath, cp);
			cp = strrchr(gOurDirectoryPath, '"');
			if ((cp != NULL) && (cp[1] == '\0'))
				*cp = '\0';
		} else if ((cp = getenv("HOME")) != NULL) {
			if (*cp == '"')
				cp++;
			(void) STRNCPY(gOurDirectoryPath, cp);
			cp = strrchr(gOurDirectoryPath, '"');
			if ((cp != NULL) && (cp[1] == '\0'))
				*cp = '\0';
		} else {
			STRNCPY(gOurDirectoryPath, gOurInstallationPath);
			if (gUser[0] == '\0') {
				STRNCAT(gOurDirectoryPath, "\\Users\\default");
			} else {
				STRNCAT(gOurDirectoryPath, "\\Users\\");
				STRNCAT(gOurDirectoryPath, gUser);
			}
		}
		rc = MkDirs(gOurDirectoryPath, 00755);
	}

#else
	struct stat st;
	char *cp;

#ifdef BINDIR
	(void) STRNCPY(gOurInstallationPath, BINDIR);
#else
	memset(gOurInstallationPath, 0, sizeof(gOurInstallationPath));
#endif

	cp = getenv("NCFTPDIR");
	if (cp != NULL) {
		(void) STRNCPY(gOurDirectoryPath, cp);
	} else if (STREQ(gHome, "/")) {
		/* Don't create it if you're root and your home directory
		 * is the root directory.
		 *
		 * If you are root and you want to store your ncftp
		 * config files, move your home directory somewhere else,
		 * such as /root or /home/root.
		 */
		gOurDirectoryPath[0] = '\0';
		return;
	} else {
		(void) Path(gOurDirectoryPath,
			sizeof(gOurDirectoryPath),
			gHome,
			kOurDirectoryName
		);
	}

	if (stat(gOurDirectoryPath, &st) < 0) {
		if (mkdir(gOurDirectoryPath, 00755) < 0) {
			gOurDirectoryPath[0] = '\0';
		}
	}
#endif
}	/* InitOurDirectory */



void
InitUserInfo(void)
{
#if defined(WIN32) || defined(_WINDOWS)
	DWORD nSize;
	char *cp;

	memset(gUser, 0, sizeof(gUser));
	nSize = sizeof(gUser) - 1;
	if (! GetUserName(gUser, &nSize))
		STRNCPY(gUser, "default");

	memset(gHome, 0, sizeof(gHome));
	(void) GetTempPath((DWORD) sizeof(gHome) - 1, gHome);
	cp = strrchr(gHome, '\\');
	if ((cp != NULL) && (cp[1] == '\0'))
		*cp = '\0';

	memset(gShell, 0, sizeof(gShell));
#else
	struct passwd *pwptr;
	char *envp;

	gUid = geteuid();
	pwptr = getpwuid(gUid);

	if (pwptr == NULL) {
		envp = getenv("LOGNAME");
		if (envp == NULL) {
			(void) fprintf(stderr, "Who are you?\n");
			(void) fprintf(stderr, "You have a user id number of %d, but no username associated with it.\n", (int) gUid);
			(void) STRNCPY(gUser, "unknown");
		} else {
			(void) STRNCPY(gUser, envp);
		}

		envp = getenv("HOME");
		if (envp == NULL)
			(void) STRNCPY(gHome, "/");
		else
			(void) STRNCPY(gHome, envp);

		envp = getenv("SHELL");
		if (envp == NULL)
			(void) STRNCPY(gShell, "/bin/sh");
		else
			(void) STRNCPY(gShell, envp);
	} else {
		/* Copy home directory. */
		(void) STRNCPY(gHome, pwptr->pw_dir);

		/* Copy user name. */
		(void) STRNCPY(gUser, pwptr->pw_name);

		/* Copy shell. */
		(void) STRNCPY(gShell, pwptr->pw_shell);
	}
#endif

	InitOurDirectory();
}	/* InitUserInfo */




int
MayUseFirewall(const char *const hn, int firewallType, const char *const firewallExceptionList)
{
#ifdef HAVE_STRSTR
	char buf[256];
	char *tok;
	char *parse;
#endif /* HAVE_STRSTR */

	if (firewallType == kFirewallNotInUse)
		return (0);

	if (firewallExceptionList[0] == '\0') {
		if (strchr(hn, '.') == NULL) {
			/* Unqualified host name,
			 * assume it is in local domain.
			 */
			return (0);
		} else {
			return (1);
		}
	}

	if (strchr(hn, '.') == NULL) {
		/* Unqualified host name,
		 * assume it is in local domain.
		 *
		 * If "localdomain" is in the exception list,
		 * do not use the firewall for this host.
		 */
		(void) STRNCPY(buf, firewallExceptionList);
		for (parse = buf; (tok = strtok(parse, ", \n\t\r")) != NULL; parse = NULL) {
			if (strcmp(tok, "localdomain") == 0)
				return (0);
		}
		/* fall through */
	}

#ifdef HAVE_STRSTR
	(void) STRNCPY(buf, firewallExceptionList);
	for (parse = buf; (tok = strtok(parse, ", \n\t\r")) != NULL; parse = NULL) {
		/* See if host or domain was from exclusion list
		 * matches the host to open.
		 */
		if (strstr(hn, tok) != NULL)
			return (0);
	}
#endif /* HAVE_STRSTR */
	return (1);
}	/* MayUseFirewall */



int 
StrToBool(const char *const s)
{
	int c;
	int result;
	
	c = *s;
	if (isupper(c))
		c = tolower(c);
	result = 0;
	switch (c) {
		case 'f':			       /* false */
			/*FALLTHROUGH*/
		case 'n':			       /* no */
			break;
		case 'o':			       /* test for "off" and "on" */
			c = (int) s[1];
			if (isupper(c))
				c = tolower(c);
			if (c == 'f')
				break;
			/*FALLTHROUGH*/
		case 't':			       /* true */
			/*FALLTHROUGH*/
		case 'y':			       /* yes */
			result = 1;
			break;
		default:			       /* 1, 0, -1, other number? */
			if (atoi(s) != 0)
				result = 1;
	}
	return result;
}						       /* StrToBool */




void
AbsoluteToRelative(char *const dst, const size_t dsize, const char *const dir, const char *const root, const size_t rootlen)
{
	*dst = '\0';
	if (strcmp(dir, root) != 0) {
		if (strcmp(root, "/") == 0) {
			(void) Strncpy(dst, dir + 1, dsize);
		} else if ((strncmp(root, dir, rootlen) == 0) && (dir[rootlen] == '/')) {
			(void) Strncpy(dst, dir + rootlen + 1, dsize);
		} else {
			/* Still absolute. */
			(void) Strncpy(dst, dir, dsize);
		}
	}
}	/* AbsoluteToRelative */




#if defined(WIN32) || defined(_WINDOWS)
#else

/* Some commands may want to jump back to the start too. */
static void
CancelGetHostByName(int sigNum)
{
#ifdef ncftp
	gResolveSig = sigNum;
#endif
#ifdef HAVE_SIGSETJMP
	siglongjmp(gGetHostByNameJmp, (sigNum != 0) ? 1 : 0);
#else	/* HAVE_SIGSETJMP */
	longjmp(gGetHostByNameJmp, (sigNum != 0) ? 1 : 0);
#endif	/* HAVE_SIGSETJMP */
}	/* CancelGetHostByName */

#endif




int
GetHostByName(char *const volatile dst, size_t dsize, const char *const hn, int t)
{
#if defined(WIN32) || defined(_WINDOWS)
	struct hostent *hp;
	struct in_addr ina;

	if (inet_addr(hn) != (unsigned long) 0xFFFFFFFF) {
		/* Address is an IP address string, which is what we want. */
		(void) Strncpy(dst, hn, dsize);
		return (0);
	}

	hp = gethostbyname(hn);
	if (hp != NULL) {
		(void) memcpy(&ina.s_addr, hp->h_addr_list[0], (size_t) hp->h_length);
		(void) Strncpy(dst, inet_ntoa(ina), dsize);
		return (0);
	}

#else
	int sj;
	vsigproc_t osigpipe, osigint, osigalrm;
	struct hostent *hp;
#ifndef HAVE_INET_NTOP
	struct in_addr ina;
#endif

#ifdef HAVE_INET_ATON
	if (inet_aton(hn, &ina) != 0) {
		/* Address is an IP address string, which is what we want. */
		(void) Strncpy(dst, hn, dsize);
		return (0);
	}
#else
	if (inet_addr(hn) != (unsigned long) 0xFFFFFFFF) {
		/* Address is an IP address string, which is what we want. */
		(void) Strncpy(dst, hn, dsize);
		return (0);
	}
#endif

#ifdef HAVE_SIGSETJMP
	osigpipe = osigint = osigalrm = (sigproc_t) 0;
	sj = sigsetjmp(gGetHostByNameJmp, 1);
#else	/* HAVE_SIGSETJMP */
	osigpipe = osigint = osigalrm = (sigproc_t) 0;
	sj = setjmp(gGetHostByNameJmp);
#endif	/* HAVE_SIGSETJMP */

	if (sj != 0) {
		/* Caught a signal. */
		(void) alarm(0);
		(void) NcSignal(SIGPIPE, osigpipe);
		(void) NcSignal(SIGINT, osigint);
		(void) NcSignal(SIGALRM, osigalrm);
#ifdef ncftp
		Trace(0, "Canceled GetHostByName because of signal %d.\n", gResolveSig);
#endif
	} else {
		osigpipe = NcSignal(SIGPIPE, CancelGetHostByName);
		osigint = NcSignal(SIGINT, CancelGetHostByName);
		osigalrm = NcSignal(SIGALRM, CancelGetHostByName);
		if (t > 0)
			(void) alarm((unsigned int) t);
		hp = gethostbyname(hn);
		if (t > 0)
			(void) alarm(0);
		(void) NcSignal(SIGPIPE, osigpipe);
		(void) NcSignal(SIGINT, osigint);
		(void) NcSignal(SIGALRM, osigalrm);
		if (hp != NULL) {
#ifdef HAVE_INET_NTOP	/* Mostly to workaround bug in IRIX 6.5's inet_ntoa */
			(void) memset(dst, 0, dsize);
			(void) inet_ntop(AF_INET, hp->h_addr_list[0], dst, dsize - 1);
#else
			(void) memcpy(&ina.s_addr, hp->h_addr_list[0], (size_t) hp->h_length);
			(void) Strncpy(dst, inet_ntoa(ina), dsize);
#endif
			return (0);
		}
	}
#endif	/* !Windows */

	*dst = '\0';
	return (-1);
}	/* GetHostByName */




/* Converts a date string, like "19930602204445" into to a time_t.  */
time_t UnDate(char *dstr)
{
#ifndef HAVE_MKTIME
	return ((time_t) -1);
#else
	struct tm ut, *t;
	time_t now;
	time_t result = (time_t) -1;

	(void) time(&now);
	t = localtime(&now);

	/* Copy the whole structure of the 'tm' pointed to by t, so it will
	 * also set all fields we don't specify explicitly to be the same as
	 * they were in t.  That way we copy non-standard fields such as
	 * tm_gmtoff, if it exists or not.
	 */
	ut = *t;

	/* The time we get back from the server is (should be) in UTC. */
	if (sscanf(dstr, "%04d%02d%02d%02d%02d%02d",
		&ut.tm_year,
		&ut.tm_mon,
		&ut.tm_mday,
		&ut.tm_hour,
		&ut.tm_min,
		&ut.tm_sec) == 6)
	{	
		--ut.tm_mon;
		ut.tm_year -= 1900;
		result = mktime(&ut);
	}
	return result;
#endif	/* HAVE_MKTIME */
}	/* UnDate */




#ifndef HAVE_MEMMOVE
/* This code is derived from software contributed to Berkeley by
 * Chris Torek.
 */

/*
 * sizeof(word) MUST BE A POWER OF TWO
 * SO THAT wmask BELOW IS ALL ONES
 */
typedef	int word;		/* "word" used for optimal copy speed */

#define	wsize	sizeof(word)
#define	wmask	(wsize - 1)

/*
 * Copy a block of memory, handling overlap.
 * This is the routine that actually implements
 * (the portable versions of) bcopy, memcpy, and memmove.
 */
void *
memmove(void *dst0, void *src0, size_t length)
{
	register char *dst = (char *) dst0;
	register const char *src = (char *) src0;
	register size_t t;

	if (length == 0 || dst == src)		/* nothing to do */
		return dst;

	/*
	 * Macros: loop-t-times; and loop-t-times, t>0
	 */
#define	TLOOP(s) if (t) TLOOP1(s)
#define	TLOOP1(s) do { s; } while (--t)

	if ((unsigned long)dst < (unsigned long)src) {
		/*
		 * Copy forward.
		 */
		t = (int)src;	/* only need low bits */
		if ((t | (int)dst) & wmask) {
			/*
			 * Try to align operands.  This cannot be done
			 * unless the low bits match.
			 */
			if ((t ^ (int)dst) & wmask || length < wsize)
				t = length;
			else
				t = wsize - (t & wmask);
			length -= t;
			TLOOP1(*dst++ = *src++);
		}
		/*
		 * Copy whole words, then mop up any trailing bytes.
		 */
		t = length / wsize;
		TLOOP(*(word *)dst = *(word *)src; src += wsize; dst += wsize);
		t = length & wmask;
		TLOOP(*dst++ = *src++);
	} else {
		/*
		 * Copy backwards.  Otherwise essentially the same.
		 * Alignment works as before, except that it takes
		 * (t&wmask) bytes to align, not wsize-(t&wmask).
		 */
		src += length;
		dst += length;
		t = (int)src;
		if ((t | (int)dst) & wmask) {
			if ((t ^ (int)dst) & wmask || length <= wsize)
				t = length;
			else
				t &= wmask;
			length -= t;
			TLOOP1(*--dst = *--src);
		}
		t = length / wsize;
		TLOOP(src -= wsize; dst -= wsize; *(word *)dst = *(word *)src);
		t = length & wmask;
		TLOOP(*--dst = *--src);
	}

	return(dst0);
}	/* MemMove */
#endif	/* ! HAVE_MEMMOVE */




#if defined(HAVE_STRCOLL) && !defined(HAVE_STRNCOLL)
int
strncoll(const char *a, const char *b, size_t n)
{
	int rc;

	if (n < 511) {
		char sa[512], sb[512];

		memset(sa, 0, sizeof(sa));
		memset(sb, 0, sizeof(sb));
		(void) strncpy(sa, a, n);
		(void) strncpy(sb, b, n);
		rc = (strcoll(sa, sb));
		return (rc);
	} else {
		char *ma, *mb;

		n += 5;			/* enough for a 32-bit zero char. */
		ma = (char *) malloc(n);
		if (ma == NULL)
			return (0);	/* panic */
		mb = (char *) malloc(n);
		if (mb == NULL) {
			free(ma);
			return (0);	/* panic */
		}
		(void) strncpy(ma, a, n);
		(void) strncpy(mb, b, n);
		rc = (strcoll(ma, mb));
		free(ma);
		free(mb);
		return (rc);
	}
}	/* strncoll */
#endif




int
DecodeDirectoryURL(
	const FTPCIPtr cip,	/* area pointed to may be modified */
	char *url,		/* always modified */
	LineListPtr cdlist,	/* always modified */
	char *fn,		/* always modified */
	size_t fnsize
)
{
	int rc;
	char urlstr2[256];
	char *cp;

	/* Add a trailing slash, if needed, i.e., convert
	 * "ftp://ftp.gnu.org/pub/gnu" to
	 * "ftp://ftp.gnu.org/pub/gnu/"
	 *
	 * We also generalize and assume that if the user specified
	 * something with a .extension that the user was intending
	 * to specify a file instead of a directory.
	 */
	cp = strrchr(url, '/');
	if ((cp != NULL) && (cp[1] != '\0') && (strchr(cp, '.') == NULL)) {

		(void) STRNCPY(urlstr2, url);
		(void) STRNCAT(urlstr2, "/");
		url = urlstr2;
	}
	rc = FTPDecodeURL(cip, url, cdlist, fn, fnsize, NULL, NULL);
	return (rc);
}	/* DecodeDirectoryURL */




#if defined(WIN32) || defined(_WINDOWS)
void SysPerror(const char *const errMsg)
{
	char reason[128];
	
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reason,
		(DWORD) sizeof(reason),
		NULL
		);

	if (reason[strlen(reason) - 1] = '\n')
		reason[strlen(reason) - 1] = '\0';
	(void) fprintf(stderr, "%s: %s\n", errMsg, reason);
}	/* SysPerror */
#endif
