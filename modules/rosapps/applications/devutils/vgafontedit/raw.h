/*
 * PROJECT:     ReactOS VGA Font Editor
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Raw bitmap fonts
 * COPYRIGHT:   Copyright 2008 Colin Finck (colin@reactos.org)
 */

#ifndef __RAW_H
#define __RAW_H

typedef struct _BITMAP_FONT
{
    UCHAR Bits[2048];
} BITMAP_FONT, *PBITMAP_FONT;

#endif
