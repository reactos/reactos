/* errno.c
 *
 * Copyright (c) 1996-2001 Mike Gleason, NCEMRSoft.
 * All rights reserved.
 *
 */

#define _libncftp_errno_c_ 1
#include "syshdrs.h"
	
static const char *gErrList[kErrLast - kErrFirst + 2] = {
	"gethostname() failed",						/* -100 */
	"hostname does not include domain name",			/* -101 */
	"could not set linger mode",					/* -102 */
	"could not set type of service",				/* -103 */
	"could not enable out-of-band data inline",			/* -104 */
	"unknown host",							/* -105 */
	"could not get a new stream socket",				/* -106 */
	"could not duplicate a socket",					/* -107 */
	"fdopen for reading failed",					/* -108 */
	"fdopen for writing failed",					/* -109 */
	"getsockname failed",						/* -110 */
	"could not bind the data socket",				/* -111 */
	"could not listen on the data socket",				/* -112 */
	"passive mode failed",						/* -113 */
	"server sent bogus port number",				/* -114 */
	"could not connect data socket",				/* -115 */
	"could not accept data socket",					/* -116 */
	"could not set restart point",					/* -117 */
	"could not connect to remote host",				/* -118 */
	"could not connect to remote host, but can try again",		/* -119 */
	"remote host refused connection",				/* -120 */
	"bad transfer type",						/* -121 */
	"invalid directory parameter",					/* -122 */
	"malloc failed",						/* -123 */
	"PWD failed",							/* -124 */
	"remote chdir failed",						/* -125 */
	"remote rmdir failed",						/* -126 */
	"bad line list",						/* -127 */
	"unimplemented option",						/* -128 */
	"unimplemented function",					/* -129 */
	"remote directory listing failed",				/* -130 */
	"could not retrieve remote file",				/* -131 */
	"could not send file to remote host",				/* -132 */
	"file write error",						/* -133 */
	"file read error",						/* -134 */
	"socket write error",						/* -135 */
	"socket read error",						/* -136 */
	"could not open file",						/* -137 */
	"bad magic number in FTP library structure",			/* -138 */
	"bad parameter given to library",				/* -139 */
	"remote mkdir failed",						/* -140 */
	"remote cd .. failed",						/* -141 */
	"remote chmod failed",						/* -142 */
	"remote umask failed",						/* -143 */
	"remote delete failed",						/* -144 */
	"remote file size inquiry failed",				/* -145 */
	"remote file timestamp inquiry failed",				/* -146 */
	"remote transfer type change failed",				/* -147 */
	"file size inquiries not understood by remote server",		/* -148 */
	"file timestamp inquiries not understood by remote server",	/* -149 */
	"could not rename remote file",					/* -150 */
	"could not do remote wildcard expansion",			/* -151 */
	"could not set keepalive option",				/* -152 */
	"remote host disconnected during login",			/* -153 */
	"username was not accepted for login",				/* -154 */
	"username and/or password was not accepted for login",		/* -155 */
	"login failed",							/* -156 */
	"invalid reply from server",					/* -157 */
	"remote host closed control connection",			/* -158 */
	"not connected",						/* -159 */
	"could not start data transfer",				/* -160 */
	"data transfer failed",						/* -161 */
	"PORT failed",							/* -162 */
	"PASV failed",							/* -163 */
	"UTIME failed",							/* -164 */
	"utime requests not understood by remote server",		/* -165 */
	"HELP failed",							/* -166 */
	"file deletion on local host failed",				/* -167 */
	"lseek failed",							/* -168 */
	"data transfer aborted by local user",				/* -169 */
	"SYMLINK failed",						/* -170 */
	"symlink requests not understood by remote server",		/* -171 */
	"no match",							/* -172 */
	"server features request failed",				/* -173 */
	"no valid files were specified",				/* -174 */
	"file transfer buffer has not been allocated",			/* -175 */
	"will not overwrite local file with older remote file",		/* -176 */
	"will not overwrite remote file with older local file",		/* -177 */
	"local file appears to be the same as the remote file, no transfer necessary",	/* -178 */
	"could not get extended directory information (MLSD)",		/* -179 */
	"could not get extended file or directory information (MLST)",	/* -180 */
	"could not parse extended file or directory information",	/* -181 */
	"server does not support extended file or directory information",	/* -182 */
	"server does not support extended directory information",	/* -183 */
	"could not get information about specified file",		/* -184 */
	"server does not support file or directory information",	/* -185 */
	"could not get directory information about specified file",	/* -186 */
	"server does not support directory information",		/* -187 */
	"no such file or directory",					/* -188 */
	"server provides no way to determine file existence",		/* -189 */
	"item exists, but cannot tell if it is a file or directory",	/* -190 */
	"not a directory",						/* -191 */
	"directory recursion limit reached",				/* -192 */
	"timed out while waiting for server response",			/* -193 */
	"data transfer timed out",					/* -194 */
	"canceled by user",						/* -195 */
	NULL,								
};

int gLibNcFTP_Uses_Me_To_Quiet_Variable_Unused_Warnings = 0;

const char *
FTPStrError(int e)
{
	if (e == kErrGeneric) {
		return ("miscellaneous error");
	} else if (e == kNoErr) {
		return ("no error");
	} else {
		if (e < 0)
			e = -e;
		if ((e >= kErrFirst) && (e <= kErrLast)) {
			return (gErrList[e - kErrFirst]);
		}
	}
	return ("unrecognized error number");
}	/* FTPStrError */




void
FTPPerror(const FTPCIPtr cip, const int err, const int eerr, const char *const s1, const char *const s2)
{
	if (err != kNoErr) {
		if (err == eerr) {
			if ((s2 == NULL) || (s2[0] == '\0')) {
				if ((s1 == NULL) || (s1[0] == '\0')) { 
					(void) fprintf(stderr, "server said: %s\n", cip->lastFTPCmdResultStr);
				} else {
					(void) fprintf(stderr, "%s: server said: %s\n", s1, cip->lastFTPCmdResultStr);
				}
			} else if ((s1 == NULL) || (s1[0] == '\0')) { 
				(void) fprintf(stderr, "%s: server said: %s\n", s2, cip->lastFTPCmdResultStr);
			} else {
				(void) fprintf(stderr, "%s %s: server said: %s\n", s1, s2, cip->lastFTPCmdResultStr);
			}
		} else {
			if ((s2 == NULL) || (s2[0] == '\0')) {
				if ((s1 == NULL) || (s1[0] == '\0')) { 
					(void) fprintf(stderr, "%s.\n", FTPStrError(cip->errNo));
				} else {
					(void) fprintf(stderr, "%s: %s.\n", s1, FTPStrError(cip->errNo));
				}
			} else if ((s1 == NULL) || (s1[0] == '\0')) { 
				(void) fprintf(stderr, "%s: %s.\n", s2, FTPStrError(cip->errNo));
			} else {
				(void) fprintf(stderr, "%s %s: %s.\n", s1, s2, FTPStrError(cip->errNo));
			}
		}
	}
}	/* FTPPerror */
