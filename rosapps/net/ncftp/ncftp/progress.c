/* progress.c
 *
 * Copyright (c) 1992-2001 by Mike Gleason.
 * All rights reserved.
 * 
 */

#include "syshdrs.h"

#include "util.h"
#include "trace.h"
#include "progress.h"
#include "readln.h"

#ifdef ncftp
#	include "log.h"
#	include "pref.h"
#endif	/* ncftp */

extern const char *tcap_normal;
extern const char *tcap_boldface;
extern const char *tcap_underline;
extern const char *tcap_reverse;
extern int gScreenColumns;

#ifdef ncftp
extern char gRemoteCWD[];
#endif	/* ncftp */


double
FileSize(const double size, const char **uStr0, double *const uMult0)
{
	double uMult, uTotal;
	const char *uStr;

	/* The comparisons below may look odd, but the reason
	 * for them is that we only want a maximum of 3 digits
	 * before the decimal point.  (I.e., we don't want to
	 * see "1017.2 kB", instead we want "0.99 MB".
	 */
	if (size > (999.5 * kGigabyte)) {
		uStr = "TB";
		uMult = kTerabyte;
	} else if (size > (999.5 * kMegabyte)) {
		uStr = "GB";
		uMult = kGigabyte;
	} else if (size > (999.5 * kKilobyte)) {
		uStr = "MB";
		uMult = kMegabyte;
	} else if (size > 999.5) {
		uStr = "kB";
		uMult = 1024;
	} else {
		uStr = "B";
		uMult = 1;
	}
	if (uStr0 != NULL)
		*uStr0 = uStr;
	if (uMult0 != NULL)
		*uMult0 = uMult;
	uTotal = size / ((double) uMult);
	if (uTotal < 0.0)
		uTotal = 0.0;
	return (uTotal);
}	/* FileSize */




void
PrSizeAndRateMeter(const FTPCIPtr cip, int mode)
{
	double rate = 0.0;
	const char *rStr;
	static const char *uStr;
	static double uMult;
	char localName[32];
	char line[128];
	int i;

	switch (mode) {
		case kPrInitMsg:
			if (cip->expectedSize != kSizeUnknown) {
				cip->progress = PrStatBar;
				PrStatBar(cip, mode);
				return;
			}
			(void) FileSize((double) cip->expectedSize, &uStr, &uMult);

			if (cip->lname == NULL) {
				localName[0] = '\0';
			} else {
				AbbrevStr(localName, cip->lname, sizeof(localName) - 2, 0);
				if ((cip->usingTAR) && (strlen(localName) < (sizeof(localName) - 6))) {
					STRNCAT(localName, " (TAR)");
				}
				(void) STRNCAT(localName, ":");
			}
			if (cip->useProgressMeter) {
#ifdef ncftp
				if (cip->usingTAR) {
					if (OneTimeMessage("tar") != 0) {
						(void) fprintf(stderr, "\n\
Note: NcFTP is using on-the-fly TAR on the remote server, which retrieves the\n\
entire directory as one operation.  This allows you to preserve exact file\n\
timestamps, ownerships, and permissions, as well as a slight performance\n\
boost.\n\
\n\
If you would rather retrieve each file individually, use the \"-T\" flag with\n\
\"get\".  TAR mode cannot be resumed if the transfer fails, so if that happens\n\
try \"get -T\" to resume the directory transfer.\n\n");
					}
				}
#endif	/* ncftp */
				(void) fprintf(stderr, "%-32s", localName);
			}
			break;

		case kPrUpdateMsg:
			rate = FileSize(cip->kBytesPerSec * 1024.0, &rStr, NULL);

			if (cip->lname == NULL) {
				localName[0] = '\0';
			} else {
				AbbrevStr(localName, cip->lname, sizeof(localName) - 2, 0);
				if ((cip->usingTAR) && (strlen(localName) < (sizeof(localName) - 6))) {
					STRNCAT(localName, " (TAR)");
				}
				(void) STRNCAT(localName, ":");
			}

#if defined(HAVE_LONG_LONG) && defined(PRINTF_LONG_LONG_LLD)
			(void) sprintf(line, "%-32s  %10lld bytes  %6.2f %s/s",
				localName,
				(longest_int) (cip->bytesTransferred + cip->startPoint),
				rate,
				rStr
			);
#elif defined(HAVE_LONG_LONG) && defined(PRINTF_LONG_LONG_QD)
			(void) sprintf(line, "%-32s  %10qd bytes  %6.2f %s/s",
				localName,
				(longest_int) (cip->bytesTransferred + cip->startPoint),
				rate,
				rStr
			);
#elif defined(HAVE_LONG_LONG) && defined(PRINTF_LONG_LONG_I64D)
			(void) sprintf(line, "%-32s  %10I64d bytes  %6.2f %s/s",
				localName,
				(longest_int) (cip->bytesTransferred + cip->startPoint),
				rate,
				rStr
			);
#else
			(void) sprintf(line, "%-32s  %10ld bytes  %6.2f %s/s",
				localName,
				(long) (cip->bytesTransferred + cip->startPoint),
				rate,
				rStr
			);
#endif

			/* Pad the rest of the line with spaces, to erase any
			 * stuff that might have been left over from the last
			 * update.
			 */
			for (i = (int) strlen(line); i < 80 - 2; i++)
				line[i] = ' ';
			line[i] = '\0';

			/* Print the updated information. */
			(void) fprintf(stderr, "\r%s", line);

#if defined(HAVE_LONG_LONG) && defined(PRINTF_LONG_LONG_LLD)
			SetXtermTitle("%s - [%lld bytes]", cip->lname, (longest_int) (cip->bytesTransferred + cip->startPoint));
#elif defined(HAVE_LONG_LONG) && defined(PRINTF_LONG_LONG_QD)
			SetXtermTitle("%s - [%qd bytes]", cip->lname, (longest_int) (cip->bytesTransferred + cip->startPoint));
#elif defined(HAVE_LONG_LONG) && defined(PRINTF_LONG_LONG_I64D)
			SetXtermTitle("%s - [%I64d bytes]", cip->lname, (longest_int) (cip->bytesTransferred + cip->startPoint));
#else
			SetXtermTitle("%s - [%ld bytes]", cip->lname, (long) (cip->bytesTransferred + cip->startPoint));
#endif
			break;

		case kPrEndMsg:
			(void) fprintf(stderr, "\n\r");
#ifdef ncftp
			if (cip->rname != NULL) {
				char url[256];

				FileToURL(url, sizeof(url), cip->rname, gRemoteCWD, cip->startingWorkingDirectory, cip->user, cip->pass, cip->host, cip->port);
				LogXfer((cip->netMode == kNetReading) ? "get" : "put", url);
			}	
#endif
			break;
	}
}	/* PrSizeAndRateMeter */




void
PrStatBar(const FTPCIPtr cip, int mode)
{
	double rate, done;
	int secLeft, minLeft;
	const char *rStr;
	static const char *uStr;
	static double uTotal, uMult;
	const char *stall;
	char localName[80];
	char line[128];
	int i;

	switch (mode) {
		case kPrInitMsg:
			fflush(stdout);
			if (cip->expectedSize == kSizeUnknown) {
				cip->progress = PrSizeAndRateMeter;
				PrSizeAndRateMeter(cip, mode);
				return;
			}
			uTotal = FileSize((double) cip->expectedSize, &uStr, &uMult);

			if (cip->lname == NULL) {
				localName[0] = '\0';
			} else {
				/* Leave room for a ':' and '\0'. */
				AbbrevStr(localName, cip->lname, sizeof(localName) - 2, 0);
				(void) STRNCAT(localName, ":");
			}

			if (cip->useProgressMeter)
				(void) fprintf(stderr, "%-32s", localName);
			break;

		case kPrUpdateMsg:
			secLeft = (int) (cip->secLeft + 0.5);
			minLeft = secLeft / 60;
			secLeft = secLeft - (minLeft * 60);
			if (minLeft > 999) {
				minLeft = 999;
				secLeft = 59;
			}

			rate = FileSize(cip->kBytesPerSec * 1024.0, &rStr, NULL);
			done = (double) (cip->bytesTransferred + cip->startPoint) / uMult;

			if (cip->lname == NULL) {
				localName[0] = '\0';
			} else {
				AbbrevStr(localName, cip->lname, 31, 0);
				(void) STRNCAT(localName, ":");
			}

			if (cip->stalled < 2)
				stall = " ";
			else if (cip->stalled < 15)
				stall = "-";
			else
				stall = "=";

			(void) sprintf(line, "%-32s   ETA: %3d:%02d  %6.2f/%6.2f %.2s  %6.2f %.2s/s %.1s",
				localName,
				minLeft,
				secLeft,
				done,
				uTotal,
				uStr,
				rate,
				rStr,
				stall
			);

			/* Print the updated information. */
			(void) fprintf(stderr, "\r%s", line);

			SetXtermTitle("%s - [%.1f%%]", cip->lname, cip->percentCompleted);
			break;

		case kPrEndMsg:

			rate = FileSize(cip->kBytesPerSec * 1024.0, &rStr, NULL);
			done = (double) (cip->bytesTransferred + cip->startPoint) / uMult;

			if (cip->expectedSize == (cip->bytesTransferred + cip->startPoint)) {
				if (cip->lname == NULL) {
					localName[0] = '\0';
				} else {
					AbbrevStr(localName, cip->lname, 52, 0);
					(void) STRNCAT(localName, ":");
				}

				(void) sprintf(line, "%-53s  %6.2f %.2s  %6.2f %.2s/s  ",
					localName,
					uTotal,
					uStr,
					rate,
					rStr
				);
			} else {
				if (cip->lname == NULL) {
					localName[0] = '\0';
				} else {
					AbbrevStr(localName, cip->lname, 45, 0);
					(void) STRNCAT(localName, ":");
				}

				(void) sprintf(line, "%-46s  %6.2f/%6.2f %.2s  %6.2f %.2s/s  ",
					localName,
					done,
					uTotal,
					uStr,
					rate,
					rStr
				);
			}

			/* Pad the rest of the line with spaces, to erase any
			 * stuff that might have been left over from the last
			 * update.
			 */
			for (i = (int) strlen(line); i < 80 - 2; i++)
				line[i] = ' ';
			line[i] = '\0';

			/* Print the updated information. */
			(void) fprintf(stderr, "\r%s\n\r", line);
#ifdef ncftp
			if (cip->rname != NULL) {
				char url[256];

				FileToURL(url, sizeof(url), cip->rname, gRemoteCWD, cip->startingWorkingDirectory, cip->user, cip->pass, cip->host, cip->port);
				LogXfer((cip->netMode == kNetReading) ? "get" : "put", url);
			}	
#endif
			break;
	}
}	/* PrStatBar */




/* This one is the brainchild of my comrade, Phil Dietz.  It shows the
 * progress as a fancy bar graph.
 */
void
PrPhilBar(const FTPCIPtr cip, int mode)
{
	int perc;
	longest_int s;
	int curBarLen;
	static int maxBarLen;
	char spec1[64], spec3[64];
	static char bar[256];
	int i;
	int secsLeft, minLeft;
	static const char *uStr;
	static double uMult;
	double rate;
	const char *rStr;

	switch (mode) {
		case kPrInitMsg:
			if (cip->expectedSize == kSizeUnknown) {
				cip->progress = PrSizeAndRateMeter;
				PrSizeAndRateMeter(cip, mode);
				return;
			}
			(void) FileSize((double) cip->expectedSize, &uStr, &uMult);
			fflush(stdout);
			fprintf(stderr, "%s file: %s \n",
				(cip->netMode == kNetReading) ? "Receiving" : "Sending",
				cip->lname
			);
			
			for (i=0; i < (int) sizeof(bar) - 1; i++)
				bar[i] = '=';
			bar[i] = '\0';

			/* Compute the size of the bar itself.  This sits between
			 * two numbers, one on each side of the screen.  So the
			 * bar length will vary, depending on how many digits we
			 * need to display the size of the file.
			 */
			maxBarLen = gScreenColumns - 1 - 28;
			for (s = cip->expectedSize; s > 0; s /= 10L)
				maxBarLen--;
			
			/* Create a specification we can hand to printf. */
#if defined(HAVE_LONG_LONG) && defined(PRINTF_LONG_LONG_LLD)
			(void) sprintf(spec1, "      0 %%%ds %%lld bytes. ETA: --:--", maxBarLen);
				
			/* Print the first invocation, which is an empty graph
			 * plus the other stuff.
			 */
			fprintf(stderr, spec1, " ", cip->expectedSize);
#elif defined(HAVE_LONG_LONG) && defined(PRINTF_LONG_LONG_QD)
			(void) sprintf(spec1, "      0 %%%ds %%qd bytes. ETA: --:--", maxBarLen);
			fprintf(stderr, spec1, " ", cip->expectedSize);
#elif defined(HAVE_LONG_LONG) && defined(PRINTF_LONG_LONG_I64D)
			(void) sprintf(spec1, "      0 %%%ds %%I64d bytes. ETA: --:--", maxBarLen);
			fprintf(stderr, spec1, " ", cip->expectedSize);
#else
			(void) sprintf(spec1, "      0 %%%ds %%ld bytes. ETA: --:--", maxBarLen);
			fprintf(stderr, spec1, " ", (long) cip->expectedSize);
#endif
			fflush(stdout);
			break;

		case kPrUpdateMsg:
			/* Compute how much of the bar should be colored in. */
			curBarLen = (int) (cip->percentCompleted * 0.01 * (double)maxBarLen);

			/* Colored portion must be at least one character so
			 * the spec isn't '%0s' which would screw the right side
			 * of the indicator.
			 */
			if (curBarLen < 1)
				curBarLen = 1;
			
			bar[curBarLen - 1] = '>';
			bar[curBarLen] = '\0';

			/* Make the spec, so we can print the bar and the other stuff. */
			STRNCPY(spec1, "\r%3d%%  0 ");

#if defined(HAVE_LONG_LONG) && defined(PRINTF_LONG_LONG_LLD)
			(void) sprintf(spec3, "%%%ds %%lld bytes. %s%%3d:%%02d", 
				maxBarLen - curBarLen,
				"ETA:"
			);
#elif defined(HAVE_LONG_LONG) && defined(PRINTF_LONG_LONG_QD)
			(void) sprintf(spec3, "%%%ds %%qd bytes. %s%%3d:%%02d", 
				maxBarLen - curBarLen,
				"ETA:"
			);
#elif defined(HAVE_LONG_LONG) && defined(PRINTF_LONG_LONG_I64D)
			(void) sprintf(spec3, "%%%ds %%I64d bytes. %s%%3d:%%02d", 
				maxBarLen - curBarLen,
				"ETA:"
			);
#else
			(void) sprintf(spec3, "%%%ds %%ld bytes. %s%%3d:%%02d", 
				maxBarLen - curBarLen,
				"ETA:"
			);
#endif
			
			/* We also show the percentage as a number at the left side. */
			perc = (int) (cip->percentCompleted);
			secsLeft = (int) (cip->secLeft);
			minLeft = secsLeft / 60;
			secsLeft = secsLeft - (minLeft * 60);
			if (minLeft > 999) {
				minLeft = 999;
				secsLeft = 59;
			}
			
			/* Print the updated information. */
			fprintf(stderr, spec1, perc);
			fprintf(stderr, "%s%s%s", tcap_reverse, bar, tcap_normal);
			fprintf(stderr, spec3,
				"",
				cip->expectedSize,
				minLeft,
				secsLeft
			);

			bar[curBarLen - 1] = '=';
			bar[curBarLen] = '=';
			fflush(stdout);

			SetXtermTitle("%s - [%.1f%%]", cip->lname, cip->percentCompleted);
			break;
		case kPrEndMsg:
			printf("\n");
			rate = FileSize(cip->kBytesPerSec * 1024.0, &rStr, NULL);

			(void) fprintf(stderr, "%s: finished in %ld:%02ld:%02ld, %.2f %s/s\n",
				(cip->lname == NULL) ? "remote" : cip->lname,
				(long) cip->sec / 3600L,
				((long) cip->sec / 60L) % 60L,
				((long) cip->sec % 60L),
				rate,
				rStr
			);
#ifdef ncftp
			if (cip->rname != NULL) {
				char url[256];

				FileToURL(url, sizeof(url), cip->rname, gRemoteCWD, cip->startingWorkingDirectory, cip->user, cip->pass, cip->host, cip->port);
				LogXfer((cip->netMode == kNetReading) ? "get" : "put", url);
			}	
#endif
			break;
	}
}	/* PrPhilBar */
