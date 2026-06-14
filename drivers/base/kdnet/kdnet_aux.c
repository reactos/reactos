/*
 * PROJECT:     ReactOS Networking Debugging Module
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Misc. Functions for kdnet
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "kdnet.h"

VOID
NTAPI
KdSetHiberRange(VOID)
{
    //TODO: PoSetHiberRange ourselves too!

    if (KdNetExtensibilityExports && KdNetExtensibilityExports->KdSetHibernateRange)
        KdNetExtensibilityExports->KdSetHibernateRange();
}
