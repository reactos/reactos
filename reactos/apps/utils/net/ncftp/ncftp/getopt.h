/* getopt.h
 *
 * Copyright (c) 1992-2001 by Mike Gleason.
 * All rights reserved.
 * 
 */

#define kGetoptBadChar   ((int) '?')
#define kGetoptErrMsg    ""

void GetoptReset(void);
int Getopt(int nargc, const char **const nargv, const char *const ostr);
