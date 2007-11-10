/*
 * Texture compression
 * Version:  1.1
 *
 * Copyright (C) 2004  Daniel Borca   All Rights Reserved.
 *
 * this is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * this is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Make; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.	
 */


#ifndef UTIL_H_included


/*
 * timer stuff
 */
#ifdef __DJGPP__

typedef unsigned long long t_type;
extern unsigned long long _rdtsc(void);
#define T_START(t) do { t = _rdtsc(); } while (0)
#define T_STOP(t) do { t = _rdtsc() - t; } while (0)
#define T_DELTA(t) (unsigned long)(t & 0xffffffff)

#elif defined(__linux__)

#include <time.h>

typedef clock_t t_type;
#define T_START(t) do { t = clock(); } while (0)
#define T_STOP(t) do { t = clock() - t; } while (0)
#define T_DELTA(t) t

#else  /* !__linux__ */

typedef int t_type;
#define T_START(t) do { t = 0; } while (0)
#define T_STOP(t) do { t = 0 - t; } while (0)
#define T_DELTA(t) t

#endif /* !__linux__ */


/*
 * compressed texture stuff
 */
int tc_stride (int format, unsigned int width);
unsigned int tc_size (unsigned int width, unsigned int height, int format);


/*
 * specific stuff
 */
void *txs_read_fxt1 (const char *filename, int *width, int *height);


#endif /* UTIL_H_included */
