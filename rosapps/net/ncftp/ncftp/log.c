/* log.c
 *
 * Copyright (c) 1992-2001 by Mike Gleason.
 * All rights reserved.
 * 
 */

#include "syshdrs.h"

#include "util.h"
#include "log.h"

extern int gMaxLogSize;
char gLogFileName[256];

extern char gOurDirectoryPath[];


void
InitLog(void)
{
	OurDirectoryPath(gLogFileName, sizeof(gLogFileName), kLogFileName);
}	/* InitLog */



void
LogXfer(const char *const mode, const char *const url)
{
	FILE *fp;

	if (gMaxLogSize == 0)
		return;		/* Don't log */

	fp = fopen(gLogFileName, FOPEN_APPEND_TEXT);
	if (fp != NULL) {
		(void) fprintf(fp, "  %s %s\n", mode, url);
		(void) fclose(fp);
	}
}	/* LogOpen */



void
LogOpen(const char *const host)
{
	time_t now;
	FILE *fp;

	if (gMaxLogSize == 0)
		return;		/* Don't log */

	time(&now);
	fp = fopen(gLogFileName, FOPEN_APPEND_TEXT);
	if (fp != NULL) {
		(void) fprintf(fp, "%s at %s", host, ctime(&now));
		(void) fclose(fp);
	}
}	/* LogOpen */




void
EndLog(void)
{
	FILE *new, *old;
	struct Stat st;
	long fat;
	char str[512];
	char tmpLog[256];

	if (gOurDirectoryPath[0] == '\0')
		return;		/* Don't create in root directory. */

	/* If the user wants to, s/he can specify the maximum size of the log file,
	 * so it doesn't waste too much disk space.  If the log is too fat, trim the
	 * older lines (at the top) until we're under the limit.
	 */
	if ((gMaxLogSize <= 0) || (Stat(gLogFileName, &st) < 0))
		return;						   /* Never trim, or no log. */

	if ((size_t)st.st_size < (size_t)gMaxLogSize)
		return;						   /* Log size not over limit yet. */

	if ((old = fopen(gLogFileName, FOPEN_READ_TEXT)) == NULL)
		return;

	/* Want to make it so we're about 30% below capacity.
	 * That way we won't trim the log each time we run the program.
	 */
	fat = (long) st.st_size - (long) gMaxLogSize + (long) (0.30 * gMaxLogSize);
	while (fat > 0L) {
		if (fgets(str, (int) sizeof(str), old) == NULL)
			return;
		fat -= (long) strlen(str);
	}
	/* skip lines until a new site was opened */
	for (;;) {
		if (fgets(str, (int) sizeof(str), old) == NULL) {
			(void) fclose(old);
			(void) remove(gLogFileName);
			return;					   /* Nothing left, start anew next time. */
		}
		if (! isspace(*str))
			break;
	}

	/* Copy the remaining lines in "old" to "new" */
	OurDirectoryPath(tmpLog, sizeof(tmpLog), "log.tmp");
	if ((new = fopen(tmpLog, FOPEN_WRITE_TEXT)) == NULL) {
		(void) fclose(old);
		return;
	}
	(void) fputs(str, new);
	while (fgets(str, (int) sizeof(str), old) != NULL)
		(void) fputs(str, new);
	(void) fclose(old);
	(void) fclose(new);
	if (remove(gLogFileName) < 0)
		return;
	if (rename(tmpLog, gLogFileName) < 0)
		return;
}	/* EndLog */
