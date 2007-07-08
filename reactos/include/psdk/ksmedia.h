/*
    KS Media
    (c) Andrew Greenwood, 2007.

    Please see COPYING in the top level directory for license information.
*/

#ifndef KSMEDIA_H
#define KSMEDIA_H

#include <ks.h>

#define STATIC_KSCATEGORY_WDMAUD \
    0x3e227e76L, 0x690d, 0x11d2, 0x81, 0x61, 0x00, 0x00, 0xf8, 0x77, 0x5b, 0xf1
DEFINE_GUIDSTRUCT("3E227E76-690D-11D2-8161-0000F8775BF1", KSCATEGORY_WDMAUD);
#define KSCATEGORY_WDMAUD DEFINE_GUIDNAMED(KSCATEGORY_WDMAUD)

#endif
