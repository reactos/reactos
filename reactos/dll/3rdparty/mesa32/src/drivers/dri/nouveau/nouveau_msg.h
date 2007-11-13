/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.
Copyright 2006 Stephane Marchesin. All Rights Reserved

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *   Nicolai Haehnle <prefect_@gmx.net>
 */


#ifndef __NOUVEAU_MSG_H__
#define __NOUVEAU_MSG_H__

#define WARN_ONCE(a, ...) do {\
	        static int warn##__LINE__=1;\
	        if(warn##__LINE__){\
			                fprintf(stderr, "*********************************WARN_ONCE*********************************\n");\
			                fprintf(stderr, "File %s function %s line %d\n", __FILE__, __FUNCTION__, __LINE__);\
			                fprintf(stderr,  a, ## __VA_ARGS__);\
			                fprintf(stderr, "***************************************************************************\n");\
			                warn##__LINE__=0;\
			                } \
	        }while(0)

#define MESSAGE(a, ...) do{\
	                fprintf(stderr, "************************************INFO***********************************\n");\
	                fprintf(stderr, "File %s function %s line %d\n", __FILE__, __FUNCTION__, __LINE__); \
	                fprintf(stderr,  a, ## __VA_ARGS__);\
	                fprintf(stderr, "***************************************************************************\n");\
	        }while(0)

#define FATAL(a, ...) do{\
	                fprintf(stderr, "***********************************FATAL***********************************\n");\
	                fprintf(stderr, "File %s function %s line %d\n", __FILE__, __FUNCTION__, __LINE__); \
	                fprintf(stderr,  a, ## __VA_ARGS__);\
	                fprintf(stderr, "***************************************************************************\n");\
	        }while(0)

#endif /* __NOUVEAU_MSG_H__ */

