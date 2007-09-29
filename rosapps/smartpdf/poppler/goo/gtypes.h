/*
 * gtypes.h
 *
 * Some useful simple types.
 *
 * Copyright 1996-2003 Glyph & Cog, LLC
 */

#ifndef GTYPES_H
#define GTYPES_H

/*
 * These have stupid names to avoid conflicts with some (but not all)
 * C++ compilers which define them.
 */
typedef int GBool;
#define gTrue 1
#define gFalse 0

/*
 * These have stupid names to avoid conflicts with <sys/types.h>,
 * which on various systems defines some random subset of these.
 */
typedef unsigned char Guchar;
typedef unsigned short Gushort;
typedef unsigned int Guint;
typedef unsigned long Gulong;

#endif
