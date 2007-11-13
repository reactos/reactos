/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/r128_version.h,v 1.6 2003/01/01 19:16:35 tsi Exp $ */
/*
 * Copyright 2000 through 2003 by Marc Aurele La France (TSI @ UQV), tsi@xfree86.org
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of Marc Aurele La France not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Marc Aurele La France makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as-is" without express or implied warranty.
 *
 * MARC AURELE LA FRANCE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO
 * EVENT SHALL MARC AURELE LA FRANCE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _R128_VERSION_H_
#define _R128_VERSION_H_ 1

#undef  R128_NAME
#undef  R128_DRIVER_NAME
#undef  R128_VERSION_MAJOR
#undef  R128_VERSION_MINOR
#undef  R128_VERSION_PATCH
#undef  R128_VERSION_CURRENT
#undef  R128_VERSION_EVALUATE
#undef  R128_VERSION_STRINGIFY
#undef  R128_VERSION_NAME

#define R128_NAME          "R128"
#define R128_DRIVER_NAME   "r128"

#define R128_VERSION_MAJOR 4
#define R128_VERSION_MINOR 0
#define R128_VERSION_PATCH 1

#ifndef R128_VERSION_EXTRA
#define R128_VERSION_EXTRA ""
#endif

#define R128_VERSION_CURRENT \
    ((R128_VERSION_MAJOR << 20) | \
     (R128_VERSION_MINOR << 10) | \
     (R128_VERSION_PATCH))

#define R128_VERSION_EVALUATE(__x) #__x
#define R128_VERSION_STRINGIFY(_x) R128_VERSION_EVALUATE(_x)
#define R128_VERSION_NAME                                         \
    R128_VERSION_STRINGIFY(R128_VERSION_MAJOR) "."                \
    R128_VERSION_STRINGIFY(R128_VERSION_MINOR) "."                \
    R128_VERSION_STRINGIFY(R128_VERSION_MINOR) R128_VERSION_EXTRA

#endif /* _R128_VERSION_H_ */
