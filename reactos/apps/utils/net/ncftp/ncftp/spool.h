/* spool.h
 *
 * Copyright (c) 1992-2001 by Mike Gleason.
 * All rights reserved.
 * 
 */

#define kSpoolDir "spool"
#if defined(WIN32) || defined(_WINDOWS)
#	define kSpoolLog "batchlog.txt"
#else
#	define kSpoolLog "batchlog"
#endif

/* spool.c */
void TruncBatchLog(void);
int MkSpoolDir(char *, size_t);
void SpoolName(const char *const, char *, size_t, int, int, time_t);
int CanSpool(void);
int HaveSpool(void);
int SpoolX(const char *const, const char *const, const char *const, const char *const, const char *const, const char *const, const char *const, const unsigned int, const char *const, const char *const, int, int, int, int, const char *const, const char *const, const char *const, time_t);
void RunBatch(int, const FTPCIPtr);
void Jobs(void);
void RunBatchIfNeeded(const FTPCIPtr);
