/* log.h
 *
 * Copyright (c) 1992-2001 by Mike Gleason.
 * All rights reserved.
 * 
 */

#if defined(WIN32) || defined(_WINDOWS)
#	define kLogFileName				"log.txt"
#else
#	define kLogFileName				"log"
#endif

/* trace.c */
void EndLog(void);
void InitLog(void);
void LogOpen(const char *const host);
void LogXfer(const char *const mode, const char *const url);
