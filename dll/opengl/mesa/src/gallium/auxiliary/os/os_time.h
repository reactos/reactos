/**************************************************************************
 *
 * Copyright 2008-2010 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * @file
 * OS independent time-manipulation functions.
 * 
 * @author Jose Fonseca <jfonseca@vmware.com>
 */

#ifndef _OS_TIME_H_
#define _OS_TIME_H_


#include "pipe/p_config.h"

#if defined(PIPE_OS_UNIX)
#  include <unistd.h> /* usleep */
#endif

#include "pipe/p_compiler.h"


#ifdef	__cplusplus
extern "C" {
#endif


/*
 * Get the current time in microseconds from an unknown base.
 */
int64_t
os_time_get(void);


/*
 * Sleep.
 */
#if defined(PIPE_OS_UNIX)
#define os_time_sleep(_usecs) usleep(_usecs)
#else
void
os_time_sleep(int64_t usecs);
#endif


/*
 * Helper function for detecting time outs, taking in account overflow.
 *
 * Returns true if the current time has elapsed beyond the specified interval.
 */
static INLINE boolean
os_time_timeout(int64_t start,
                int64_t end,
                int64_t curr)
{
   if(start <= end)
      return !(start <= curr && curr < end);
   else
      return !((start <= curr) || (curr < end));
}


#ifdef	__cplusplus
}
#endif

#endif /* _OS_TIME_H_ */
