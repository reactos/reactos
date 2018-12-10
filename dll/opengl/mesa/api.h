/* $Id: api.h,v 1.4 1998/02/04 00:38:24 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.4
 * Copyright (C) 1995-1997  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * $Log: api.h,v $
 * Revision 1.4  1998/02/04 00:38:24  brianp
 * WIN32 patch from Oleg Letsinsky
 *
 * Revision 1.3  1998/02/04 00:13:35  brianp
 * updated for Cygnus (Stephane Rehel)
 *
 * Revision 1.2  1997/11/25 03:20:09  brianp
 * simple clean-ups for multi-threading (John Stone)
 *
 * Revision 1.1  1997/08/22 01:42:26  brianp
 * Initial revision
 *
 */


/*
 * The original api.c file has been split into two files:  api1.c and api2.c
 * because some compilers complained that api.c was too big.
 *
 * This header contains stuff only included by api1.c and api2.c
 */


#ifndef API_H
#define API_H


/*
 * Single/multiple thread context selection.
 */
#ifdef THREADS

/* Get the context associated with the calling thread */
#define GET_CONTEXT	GLcontext *CC = gl_get_thread_context()

#else

/* CC is a global pointer for all threads in the address space */
#define GET_CONTEXT

#endif /* THREADS */


/*
 * An optimization in a few performance-critical functions.
 */
#define SHORTCUT


/*
 * Windows 95/NT DLL stuff.
 */
#if !defined(WIN32) && !defined(WINDOWS_NT) && !defined(__CYGWIN32__)
#define APIENTRY
#endif


#endif
