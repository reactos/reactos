/* rcmd.c
 *
 * Copyright (c) 1996-2001 Mike Gleason, NCEMRSoft.
 * All rights reserved.
 *
 */

#include "syshdrs.h"

#if !defined(NO_SIGNALS) && (USE_SIO || !defined(SIGALRM) || !defined(SIGPIPE) || !defined(SIGINT))
#	define NO_SIGNALS 1
#endif

#ifndef NO_SIGNALS

#ifdef HAVE_SIGSETJMP
static sigjmp_buf gBrokenCtrlJmp;
#else
static jmp_buf gBrokenCtrlJmp;
#endif	/* HAVE_SIGSETJMP */

static void
BrokenCtrl(int UNUSED(signumIgnored))
{
	LIBNCFTP_USE_VAR(signumIgnored); 		/* Shut up gcc */
#ifdef HAVE_SIGSETJMP
	siglongjmp(gBrokenCtrlJmp, 1);
#else
	longjmp(gBrokenCtrlJmp, 1);
#endif	/* HAVE_SIGSETJMP */
}	/* BrokenCtrl */

#endif	/* NO_SIGNALS */


/* A 'Response' parameter block is simply zeroed to be considered init'ed. */
ResponsePtr
InitResponse(void)
{
	ResponsePtr rp;
	
	rp = (ResponsePtr) calloc(SZ(1), sizeof(Response));
	if (rp != NULL)
		InitLineList(&rp->msg);
	return (rp);
}	/* InitResponse */




/* If we don't print it to the screen, we may want to save it to our
 * trace log.
 */
void
TraceResponse(const FTPCIPtr cip, ResponsePtr rp)
{
	LinePtr lp;
	
	if (rp != NULL)	{
		lp = rp->msg.first;
		if (lp != NULL) {
			PrintF(cip, "%3d: %s\n", rp->code, lp->line);
			for (lp = lp->next; lp != NULL; lp = lp->next)
				PrintF(cip, "     %s\n", lp->line);
		}
	}
}	/* TraceResponse */





void
PrintResponse(const FTPCIPtr cip, LineListPtr llp)
{
	LinePtr lp;
	
	if (llp != NULL) {
		for (lp = llp->first; lp != NULL; lp = lp->next)
			PrintF(cip, "%s\n", lp->line);
	}
}	/* PrintResponse */





static void
SaveLastResponse(const FTPCIPtr cip, ResponsePtr rp)
{
	if (rp == NULL) {
		cip->lastFTPCmdResultStr[0] = '\0';
		cip->lastFTPCmdResultNum = -1;
		DisposeLineListContents(&cip->lastFTPCmdResultLL);
	} else if ((rp->msg.first == NULL) || (rp->msg.first->line == NULL)) {
		cip->lastFTPCmdResultStr[0] = '\0';
		cip->lastFTPCmdResultNum = rp->code;
		DisposeLineListContents(&cip->lastFTPCmdResultLL);
	} else {
		(void) STRNCPY(cip->lastFTPCmdResultStr, rp->msg.first->line);
		cip->lastFTPCmdResultNum = rp->code;

		/* Dispose previous command's line list. */
		DisposeLineListContents(&cip->lastFTPCmdResultLL);

		/* Save this command's line list. */
		cip->lastFTPCmdResultLL = rp->msg;
	}
}	/* SaveLastResponse */



void
DoneWithResponse(const FTPCIPtr cip, ResponsePtr rp)
{
	/* Dispose space taken up by the Response, and clear it out
	 * again.  For some reason, I like to return memory to zeroed
	 * when not in use.
	 */
	if (rp != NULL) {
		TraceResponse(cip, rp);
		if (cip->printResponseProc != 0) {
			if ((rp->printMode & kResponseNoProc) == 0)
				(*cip->printResponseProc)(cip, rp);
		}
		if ((rp->printMode & kResponseNoSave) == 0)
			SaveLastResponse(cip, rp);
		else
			DisposeLineListContents(&rp->msg);
		(void) memset(rp, 0, sizeof(Response));
		free(rp);
	}
}	/* DoneWithResponse */




/* This takes an existing Response and recycles it, by clearing out
 * the current contents.
 */
void
ReInitResponse(const FTPCIPtr cip, ResponsePtr rp)
{
	if (rp != NULL) {
		TraceResponse(cip, rp);
		if (cip->printResponseProc != 0) {
			if ((rp->printMode & kResponseNoProc) == 0)
				(*cip->printResponseProc)(cip, rp);
		}
		if ((rp->printMode & kResponseNoSave) == 0)
			SaveLastResponse(cip, rp);
		else
			DisposeLineListContents(&rp->msg);
		(void) memset(rp, 0, sizeof(Response));
	}
}	/* ReInitResponse */




#ifndef NO_SIGNALS

/* Since the control stream is defined by the Telnet protocol (RFC 854),
 * we follow Telnet rules when reading the control stream.  We use this
 * routine when we want to read a response from the host.
 */
int
GetTelnetString(const FTPCIPtr cip, char *str, size_t siz, FILE *cin, FILE *cout)
{
	int c;
	size_t n;
	int eofError;
	char *cp;

	cp = str;
	--siz;		/* We'll need room for the \0. */

	if ((cin == NULL) || (cout == NULL)) {
		eofError = -1;
		goto done;
	}

	for (n = (size_t)0, eofError = 0; ; ) {
		c = fgetc(cin);
checkChar:
		if (c == EOF) {
eof:
			eofError = -1;
			break;
		} else if (c == '\r') {
			/* A telnet string can have a CR by itself.  But to denote that,
			 * the protocol uses \r\0;  an end of line is denoted \r\n.
			 */
			c = fgetc(cin);
			if (c == '\n') {
				/* Had \r\n, so done. */
				goto done;
			} else if (c == EOF) {
				goto eof;
			} else if (c == '\0') {
				c = '\r';
				goto addChar;
			} else {
				/* Telnet protocol violation! */
				goto checkChar;
			}
		} else if (c == '\n') {
			/* Really shouldn't get here.  If we do, the other side
			 * violated the TELNET protocol, since eoln's are CR/LF,
			 * and not just LF.
			 */
			PrintF(cip, "TELNET protocol violation:  raw LF.\n");
			goto done;
		} else if (c == IAC) {
			/* Since the control connection uses the TELNET protocol,
			 * we have to handle some of its commands ourselves.
			 * IAC is the protocol's escape character, meaning that
			 * the next character after the IAC (Interpret as Command)
			 * character is a telnet command.  But, if there just
			 * happened to be a character in the text stream with the
			 * same numerical value of IAC, 255, the sender denotes
			 * that by having an IAC followed by another IAC.
			 */
			
			/* Get the telnet command. */
			c = fgetc(cin);
			
			switch (c) {
				case WILL:
				case WONT:
					/* Get the option code. */
					c = fgetc(cin);
					
					/* Tell the other side that we don't want
					 * to do what they're offering to do.
					 */
					(void) fprintf(cout, "%c%c%c",IAC,DONT,c);
					(void) fflush(cout);
					break;
				case DO:
				case DONT:
					/* Get the option code. */
					c = fgetc(cin);
					
					/* The other side said they are DOing (or not)
					 * something, which would happen if our side
					 * asked them to.  Since we didn't do that,
					 * ask them to not do this option.
					 */
					(void) fprintf(cout, "%c%c%c",IAC,WONT,c);
					(void) fflush(cout);
					break;

				case EOF:
					goto eof;

				default:
					/* Just add this character, since it was most likely
					 * just an escaped IAC character.
					 */
					goto addChar;
			}
		} else {
addChar:
			/* If the buffer supplied has room, add this character to it. */
			if (n < siz) {
				*cp++ = c;				
				++n;
			}
		}
	}

done:
	*cp = '\0';
	return (eofError);
}	/* GetTelnetString */

#endif	/* NO_SIGNALS */



/* Returns 0 if a response was read, or (-1) if an error occurs.
 * This reads the entire response text into a LineList, which is kept
 * in the 'Response' structure.
 */
int
GetResponse(const FTPCIPtr cip, ResponsePtr rp)
{
	longstring str;
	int eofError;
	str16 code;
	char *cp;
	int continuation;
	volatile FTPCIPtr vcip;
#ifdef NO_SIGNALS
	int result;
#else	/* NO_SIGNALS */
	volatile FTPSigProc osigpipe;
	int sj;
#endif	/* NO_SIGNALS */

	/* RFC 959 states that a reply may span multiple lines.  A single
	 * line message would have the 3-digit code <space> then the msg.
	 * A multi-line message would have the code <dash> and the first
	 * line of the msg, then additional lines, until the last line,
	 * which has the code <space> and last line of the msg.
	 *
	 * For example:
	 *	123-First line
	 *	Second line
	 *	234 A line beginning with numbers
	 *	123 The last line
	 */

#ifdef NO_SIGNALS
	vcip = cip;
#else	/* NO_SIGNALS */
	osigpipe = (volatile FTPSigProc) signal(SIGPIPE, BrokenCtrl);
	vcip = cip;

#ifdef HAVE_SIGSETJMP
	sj = sigsetjmp(gBrokenCtrlJmp, 1);
#else
	sj = setjmp(gBrokenCtrlJmp);
#endif	/* HAVE_SIGSETJMP */

	if (sj != 0) {
		(void) signal(SIGPIPE, (FTPSigProc) osigpipe);
		FTPShutdownHost(vcip);
		vcip->errNo = kErrRemoteHostClosedConnection;
		return(vcip->errNo);
	}
#endif	/* NO_SIGNALS */

#ifdef NO_SIGNALS
	cp = str;
	eofError = 0;
	if (cip->dataTimedOut > 0) {
		/* Give up immediately unless the server had already
		 * sent a message. Odds are since the data is timed
		 * out, so is the control.
		 */
		if (SWaitUntilReadyForReading(cip->ctrlSocketR, 0) == 0) {
			/* timeout */
			Error(cip, kDontPerror, "Could not read reply from control connection -- timed out.\n");
			FTPShutdownHost(vcip);
			cip->errNo = kErrControlTimedOut;
			return (cip->errNo);
		}
	}
	result = SReadline(&cip->ctrlSrl, str, sizeof(str) - 1);
	if (result == kTimeoutErr) {
		/* timeout */
		Error(cip, kDontPerror, "Could not read reply from control connection -- timed out.\n");
		FTPShutdownHost(vcip);
		cip->errNo = kErrControlTimedOut;
		return (cip->errNo);
	} else if (result == 0) {
		/* eof */
		eofError = 1;
		rp->hadEof = 1;
		if (rp->eofOkay == 0)
			Error(cip, kDontPerror, "Remote host has closed the connection.\n");
		FTPShutdownHost(vcip);
		cip->errNo = kErrRemoteHostClosedConnection;
		return (cip->errNo);
	} else if (result < 0) {
		/* error */
		Error(cip, kDoPerror, "Could not read reply from control connection");
		FTPShutdownHost(vcip);
		cip->errNo = kErrInvalidReplyFromServer;
		return (cip->errNo);
	}

	if (str[result - 1] == '\n')
		str[result - 1] = '\0';

#else	/* NO_SIGNALS */
	/* Get the first line of the response. */
	eofError = GetTelnetString(cip, str, sizeof(str), cip->cin, cip->cout);

	cp = str;
	if (*cp == '\0') {
		if (eofError < 0) {
			/* No bytes read for reply, and EOF detected. */
			rp->hadEof = 1;
			if (rp->eofOkay == 0)
				Error(cip, kDontPerror, "Remote host has closed the connection.\n");
			FTPShutdownHost(vcip);
			cip->errNo = kErrRemoteHostClosedConnection;
			(void) signal(SIGPIPE, osigpipe);
			return(cip->errNo);
		}
	}
#endif	/* NO_SIGNALS */

	if (!isdigit((int) *cp)) {
		Error(cip, kDontPerror, "Invalid reply: \"%s\"\n", cp);
		cip->errNo = kErrInvalidReplyFromServer;
#ifndef NO_SIGNALS
		(void) signal(SIGPIPE, osigpipe);
#endif
		return (cip->errNo);
	}

	rp->codeType = *cp - '0';
	cp += 3;
	continuation = (*cp == '-');
	*cp++ = '\0';
	(void) STRNCPY(code, str);
	rp->code = atoi(code);
	(void) AddLine(&rp->msg, cp);
	if (eofError < 0) {
		/* Read reply, but EOF was there also. */
		rp->hadEof = 1;
	}
	
	while (continuation) {

#ifdef NO_SIGNALS
		result = SReadline(&cip->ctrlSrl, str, sizeof(str) - 1);
		if (result == kTimeoutErr) {
			/* timeout */
			Error(cip, kDontPerror, "Could not read reply from control connection -- timed out.\n");
			FTPShutdownHost(vcip);
			cip->errNo = kErrControlTimedOut;
			return (cip->errNo);
		} else if (result == 0) {
			/* eof */
			eofError = 1;
			rp->hadEof = 1;
			if (rp->eofOkay == 0)
				Error(cip, kDontPerror, "Remote host has closed the connection.\n");
			FTPShutdownHost(vcip);
			cip->errNo = kErrRemoteHostClosedConnection;
			return (cip->errNo);
		} else if (result < 0) {
			/* error */
			Error(cip, kDoPerror, "Could not read reply from control connection");
			FTPShutdownHost(vcip);
			cip->errNo = kErrInvalidReplyFromServer;
			return (cip->errNo);
		}

		if (str[result - 1] == '\n')
			str[result - 1] = '\0';
#else	/* NO_SIGNALS */
		eofError = GetTelnetString(cip, str, sizeof(str), cip->cin, cip->cout);
		if (eofError < 0) {
			/* Read reply, but EOF was there also. */
			rp->hadEof = 1;
			continuation = 0;
		}
#endif	/* NO_SIGNALS */
		cp = str;
		if (strncmp(code, cp, SZ(3)) == 0) {
			cp += 3;
			if (*cp == ' ')
				continuation = 0;
			++cp;
		}
		(void) AddLine(&rp->msg, cp);
	}

	if (rp->code == 421) {
		/*
		 *   421 Service not available, closing control connection.
		 *       This may be a reply to any command if the service knows it
		 *       must shut down.
		 */
		if (rp->eofOkay == 0)
			Error(cip, kDontPerror, "Remote host has closed the connection.\n");
		FTPShutdownHost(vcip);
		cip->errNo = kErrRemoteHostClosedConnection;
#ifndef NO_SIGNALS
		(void) signal(SIGPIPE, osigpipe);
#endif
		return(cip->errNo);
	}

#ifndef NO_SIGNALS
	(void) signal(SIGPIPE, osigpipe);
#endif
	return (kNoErr);
}	/* GetResponse */




/* This creates the complete command text to send, and writes it
 * on the stream.
 */
#ifdef NO_SIGNALS

static int
SendCommand(const FTPCIPtr cip, const char *cmdspec, va_list ap)
{
	longstring command;
	int result;

	if (cip->ctrlSocketW != kClosedFileDescriptor) {
#ifdef HAVE_VSNPRINTF
		(void) vsnprintf(command, sizeof(command) - 1, cmdspec, ap);
		command[sizeof(command) - 1] = '\0';
#else
		(void) vsprintf(command, cmdspec, ap);
#endif
		if ((strncmp(command, "PASS", SZ(4)) != 0) || ((strcmp(cip->user, "anonymous") == 0) && (cip->firewallType == kFirewallNotInUse)))
			PrintF(cip, "Cmd: %s\n", command);
		else
			PrintF(cip, "Cmd: %s\n", "PASS xxxxxxxx");
		(void) STRNCAT(command, "\r\n");	/* Use TELNET end-of-line. */
		cip->lastFTPCmdResultStr[0] = '\0';
		cip->lastFTPCmdResultNum = -1;

		result = SWrite(cip->ctrlSocketW, command, strlen(command), (int) cip->ctrlTimeout, 0);

		if (result < 0) {
			cip->errNo = kErrSocketWriteFailed;
			Error(cip, kDoPerror, "Could not write to control stream.\n");
			return (cip->errNo);
		}
		return (kNoErr);
	}
	return (kErrNotConnected);
}	/* SendCommand */

#else	/* NO_SIGNALS */

static int
SendCommand(const FTPCIPtr cip, const char *cmdspec, va_list ap)
{
	longstring command;
	int result;
	volatile FTPCIPtr vcip;
	volatile FTPSigProc osigpipe;
	int sj;

	if (cip->cout != NULL) {
#ifdef HAVE_VSNPRINTF
		(void) vsnprintf(command, sizeof(command) - 1, cmdspec, ap);
		command[sizeof(command) - 1] = '\0';
#else
		(void) vsprintf(command, cmdspec, ap);
#endif
		if ((strncmp(command, "PASS", SZ(4)) != 0) || ((strcmp(cip->user, "anonymous") == 0) && (cip->firewallType == kFirewallNotInUse)))
			PrintF(cip, "Cmd: %s\n", command);
		else
			PrintF(cip, "Cmd: %s\n", "PASS xxxxxxxx");
		(void) STRNCAT(command, "\r\n");	/* Use TELNET end-of-line. */
		cip->lastFTPCmdResultStr[0] = '\0';
		cip->lastFTPCmdResultNum = -1;

		osigpipe = (volatile FTPSigProc) signal(SIGPIPE, BrokenCtrl);
		vcip = cip;

#ifdef HAVE_SIGSETJMP
		sj = sigsetjmp(gBrokenCtrlJmp, 1);
#else
		sj = setjmp(gBrokenCtrlJmp);
#endif	/* HAVE_SIGSETJMP */

		if (sj != 0) {
			(void) signal(SIGPIPE, (FTPSigProc) osigpipe);
			FTPShutdownHost(vcip);
			if (vcip->eofOkay == 0) {
				Error(cip, kDontPerror, "Remote host has closed the connection.\n");
				vcip->errNo = kErrRemoteHostClosedConnection;
				return(vcip->errNo);
			}
			return (kNoErr);
		}

		result = fputs(command, cip->cout);
		if (result < 0) {
			(void) signal(SIGPIPE, osigpipe);
			cip->errNo = kErrSocketWriteFailed;
			Error(cip, kDoPerror, "Could not write to control stream.\n");
			return (cip->errNo);
		}
		result = fflush(cip->cout);
		if (result < 0) {
			(void) signal(SIGPIPE, osigpipe);
			cip->errNo = kErrSocketWriteFailed;
			Error(cip, kDoPerror, "Could not write to control stream.\n");
			return (cip->errNo);
		}
		(void) signal(SIGPIPE, osigpipe);
		return (kNoErr);
	}
	return (kErrNotConnected);
}	/* SendCommand */
#endif	/* NO_SIGNALS */



/* For "simple" (i.e. not data transfer) commands, this routine is used
 * to send the command and receive one response.  It returns the codeType
 * field of the 'Response' as the result, or a negative number upon error.
 */
/*VARARGS*/
int
FTPCmd(const FTPCIPtr cip, const char *const cmdspec, ...)
{
	va_list ap;
	int result;
	ResponsePtr rp;

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	rp = InitResponse();
	if (rp == NULL) {
		result = kErrMallocFailed;
		cip->errNo = kErrMallocFailed;
		Error(cip, kDontPerror, "Malloc failed.\n");
		return (cip->errNo);
	}

	va_start(ap, cmdspec);
#ifndef NO_SIGNALS
	if (cip->ctrlTimeout > 0)
		(void) alarm(cip->ctrlTimeout);
#endif	/* NO_SIGNALS */
	result = SendCommand(cip, cmdspec, ap);
	va_end(ap);
	if (result < 0) {
#ifndef NO_SIGNALS
		if (cip->ctrlTimeout > 0)
			(void) alarm(0);
#endif	/* NO_SIGNALS */
		return (result);
	}

	/* Get the response to the command we sent. */
	result = GetResponse(cip, rp);
#ifndef NO_SIGNALS
	if (cip->ctrlTimeout > 0)
		(void) alarm(0);
#endif	/* NO_SIGNALS */

	if (result == kNoErr)
		result = rp->codeType;
	DoneWithResponse(cip, rp);
	return (result);
}	/* FTPCmd */




/* This is for debugging the library -- don't use. */
/*VARARGS*/
int
FTPCmdNoResponse(const FTPCIPtr cip, const char *const cmdspec, ...)
{
	va_list ap;

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	va_start(ap, cmdspec);
#ifndef NO_SIGNALS
	if (cip->ctrlTimeout > 0)
		(void) alarm(cip->ctrlTimeout);
#endif	/* NO_SIGNALS */
	(void) SendCommand(cip, cmdspec, ap);
#ifndef NO_SIGNALS
	if (cip->ctrlTimeout > 0)
		(void) alarm(0);
#endif	/* NO_SIGNALS */
	va_end(ap);

	return (kNoErr);
}	/* FTPCmdNoResponse */




int
WaitResponse(const FTPCIPtr cip, unsigned int sec)
{
	int result;
	fd_set ss;
	struct timeval tv;
	int fd;

#ifdef NO_SIGNALS
	fd = cip->ctrlSocketR;
#else	/* NO_SIGNALS */
	if (cip->cin == NULL)
		return (-1);
	fd = fileno(cip->cin);
#endif	/* NO_SIGNALS */
	if (fd < 0)
		return (-1);
	FD_ZERO(&ss);
	FD_SET(fd, &ss);
	tv.tv_sec = (unsigned long) sec;
	tv.tv_usec = 0;
	result = select(fd + 1, SELECT_TYPE_ARG234 &ss, NULL, NULL, &tv);
	return (result);
}	/* WaitResponse */




/* For "simple" (i.e. not data transfer) commands, this routine is used
 * to send the command and receive one response.  It returns the codeType
 * field of the 'Response' as the result, or a negative number upon error.
 */

/*VARARGS*/
int
RCmd(const FTPCIPtr cip, ResponsePtr rp, const char *cmdspec, ...)
{
	va_list ap;
	int result;

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	va_start(ap, cmdspec);
#ifndef NO_SIGNALS
	if (cip->ctrlTimeout > 0)
		(void) alarm(cip->ctrlTimeout);
#endif	/* NO_SIGNALS */
	result = SendCommand(cip, cmdspec, ap);
	va_end(ap);
	if (result < 0) {
#ifndef NO_SIGNALS
		if (cip->ctrlTimeout > 0)
			(void) alarm(0);
#endif	/* NO_SIGNALS */
		return (result);
	}

	/* Get the response to the command we sent. */
	result = GetResponse(cip, rp);
#ifndef NO_SIGNALS
	if (cip->ctrlTimeout > 0)
		(void) alarm(0);
#endif	/* NO_SIGNALS */

	if (result == kNoErr)
		result = rp->codeType;
	return (result);
}	/* RCmd */



/* Returns -1 if an error occurred, or 0 if not.
 * This differs from RCmd, which returns the code class of a response.
 */

/*VARARGS*/
int
FTPStartDataCmd(const FTPCIPtr cip, int netMode, int type, longest_int startPoint, const char *cmdspec, ...)
{
	va_list ap;
	int result;
	int respCode;
	ResponsePtr rp;

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	result = FTPSetTransferType(cip, type);
	if (result < 0)
		return (result);

	/* Re-set the cancellation flag. */
	cip->cancelXfer = 0;

	/* To transfer data, we do these things in order as specifed by
	 * the RFC.
	 * 
	 * First, we tell the other side to set up a data line.  This
	 * is done below by calling OpenDataConnection(), which sets up
	 * the socket.  When we do that, the other side detects a connection
	 * attempt, so it knows we're there.  Then tell the other side
	 * (by using listen()) that we're willing to receive a connection
	 * going to our side.
	 */

	if ((result = OpenDataConnection(cip, cip->dataPortMode)) < 0)
		goto done;

	/* If asked, attempt to start at a later position in the remote file. */
	if (startPoint != (longest_int) 0) {
		if ((startPoint == kSizeUnknown) || ((result = SetStartOffset(cip, startPoint)) != 0))
			startPoint = (longest_int) 0;
	}
	cip->startPoint = startPoint;

	/* Now we tell the server what we want to do.  This sends the
	 * the type of transfer we want (RETR, STOR, LIST, etc) and the
	 * parameters for that (files to send, directories to list, etc).
	 */
	va_start(ap, cmdspec);
#ifndef NO_SIGNALS
	if (cip->ctrlTimeout > 0)
		(void) alarm(cip->ctrlTimeout);
#endif	/* NO_SIGNALS */
	result = SendCommand(cip, cmdspec, ap);
	va_end(ap);
	if (result < 0) { 
#ifndef NO_SIGNALS
		if (cip->ctrlTimeout > 0)
			(void) alarm(0);
#endif	/* NO_SIGNALS */
		goto done;
	}

	/* Get the response to the transfer command we sent, to see if
	 * they can accomodate the request.  If everything went okay,
	 * we will get a preliminary response saying that the transfer
	 * initiation was successful and that the data is there for
	 * reading (for retrieves;  for sends, they will be waiting for
	 * us to send them something).
	 */
	rp = InitResponse();
	if (rp == NULL) {
		Error(cip, kDontPerror, "Malloc failed.\n");
		cip->errNo = kErrMallocFailed;
		result = cip->errNo;
		goto done;
	}
	result = GetResponse(cip, rp);
#ifndef NO_SIGNALS
	if (cip->ctrlTimeout > 0)
		(void) alarm(0);
#endif	/* NO_SIGNALS */

	if (result < 0)
		goto done;
	respCode = rp->codeType;
	DoneWithResponse(cip, rp);

	if (respCode > 2) {
		cip->errNo = kErrCouldNotStartDataTransfer;
		result = cip->errNo;
		goto done;
	}

	/* Now we accept the data connection that the other side is offering
	 * to us.  Then we can do the actual I/O on the data we want.
	 */
	cip->netMode = netMode;
	if ((result = AcceptDataConnection(cip)) < 0)
		goto done;
	return (kNoErr);

done:
	(void) FTPEndDataCmd(cip, 0);
	return (result);
}	/* FTPStartDataCmd */




void 
FTPAbortDataTransfer(const FTPCIPtr cip)
{
	ResponsePtr rp;
	int result;

	if (cip->dataSocket != kClosedFileDescriptor) {
		PrintF(cip, "Starting abort sequence.\n");
		SendTelnetInterrupt(cip);		/* Probably could get by w/o doing this. */

		result = FTPCmdNoResponse(cip, "ABOR");
		if (result != kNoErr) {
			/* Linger could cause close to block, so unset it. */
			(void) SetLinger(cip, cip->dataSocket, 0);
			CloseDataConnection(cip);
			PrintF(cip, "Could not send abort command.\n");
			return;
		}

		if (cip->abortTimeout > 0) {
			result = WaitResponse(cip, (unsigned int) cip->abortTimeout);
			if (result <= 0) {
				/* Error or no response received to ABOR in time. */
				(void) SetLinger(cip, cip->dataSocket, 0);
				CloseDataConnection(cip);
				PrintF(cip, "No response received to abort request.\n");
				return;
			}
		}

		rp = InitResponse();
		if (rp == NULL) {
			Error(cip, kDontPerror, "Malloc failed.\n");
			cip->errNo = kErrMallocFailed;
			result = cip->errNo;
			return;
		}

		result = GetResponse(cip, rp);
		if (result < 0) {
			/* Shouldn't happen, and doesn't matter if it does. */
			(void) SetLinger(cip, cip->dataSocket, 0);
			CloseDataConnection(cip);
			PrintF(cip, "Invalid response to abort request.\n");
			DoneWithResponse(cip, rp);
			return;
		}
		DoneWithResponse(cip, rp);

		/* A response to the abort request has been received.
		 * Now the only thing left to do is close the data
		 * connection, making sure to turn off linger mode
		 * since we don't care about straggling data bits.
		 */
		(void) SetLinger(cip, cip->dataSocket, 0);
		CloseDataConnection(cip);		/* Must close (by protocol). */
		PrintF(cip, "End abort.\n");
	}
}	/* FTPAbortDataTransfer */




int
FTPEndDataCmd(const FTPCIPtr cip, int didXfer)
{
	int result;
	int respCode;
	ResponsePtr rp;

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	CloseDataConnection(cip);
	result = kNoErr;
	if (didXfer) {
		/* Get the response to the data transferred.  Most likely a message
		 * saying that the transfer completed succesfully.  However, if
		 * we tried to abort the transfer using ABOR, we will have a response
		 * to that command instead.
		 */
		rp = InitResponse();
		if (rp == NULL) {
			Error(cip, kDontPerror, "Malloc failed.\n");
			cip->errNo = kErrMallocFailed;
			result = cip->errNo;
			return (result);
		}
		result = GetResponse(cip, rp);
		if (result < 0)
			return (result);
		respCode = rp->codeType;
		DoneWithResponse(cip, rp);
		if (respCode != 2) {
			cip->errNo = kErrDataTransferFailed;
			result = cip->errNo;
		} else {
			result = kNoErr;
		}
	}
	return (result);
}	/* FTPEndDataCmd */




int
BufferGets(char *buf, size_t bufsize, int inStream, char *secondaryBuf, char **secBufPtr, char **secBufLimit, size_t secBufSize)
{
	int err;
	char *src;
	char *dst;
	char *dstlim;
	int len;
	int nr;
	int haveEof = 0;

	err = 0;
	dst = buf;
	dstlim = dst + bufsize - 1;		/* Leave room for NUL. */
	src = (*secBufPtr);
	for ( ; dst < dstlim; ) {
		if (src >= (*secBufLimit)) {
			/* Fill the buffer. */

/* Don't need to poll it here.  The routines that use BufferGets don't
 * need any special processing during timeouts (i.e. progress reports),
 * so go ahead and just let it block until there is data to read.
 */
			nr = (int) read(inStream, secondaryBuf, secBufSize);
			if (nr == 0) {
				/* EOF. */
				haveEof = 1;
				goto done;
			} else if (nr < 0) {
				/* Error. */
				err = -1;
				goto done;
			}
			(*secBufPtr) = secondaryBuf;
			(*secBufLimit) = secondaryBuf + nr;
			src = (*secBufPtr);
			if (nr < (int) secBufSize)
				src[nr] = '\0';
		}
		if (*src == '\r') {
			++src;
		} else {
			if (*src == '\n') {
				/* *dst++ = *src++; */	++src;
				goto done;
			}
			*dst++ = *src++;
		}
	}

done:
	(*secBufPtr) = src;
	*dst = '\0';
	len = (int) (dst - buf);
	if (err < 0)
		return (err);
	if ((len == 0) && (haveEof == 1))
		return (-1);
	return (len);	/* May be zero, if a blank line. */
}	/* BufferGets */

/* eof */
