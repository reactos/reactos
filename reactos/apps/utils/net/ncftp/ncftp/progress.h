/* progress.h
 *
 * Copyright (c) 1992-2001 by Mike Gleason.
 * All rights reserved.
 * 
 */

#define kKilobyte 1024
#define kMegabyte (kKilobyte * 1024)
#define kGigabyte ((long) kMegabyte * 1024L)
#define kTerabyte ((double) kGigabyte * 1024.0)

/* progress.c */
double FileSize(const double size, const char **uStr0, double *const uMult0);
void PrSizeAndRateMeter(const FTPCIPtr, int);
void PrStatBar(const FTPCIPtr, int);
void PrPhilBar(const FTPCIPtr, int);
