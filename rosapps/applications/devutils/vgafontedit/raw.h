/*
 * PROJECT:     ReactOS VGA Font Editor
 * LICENSE:     GNU General Public License Version 2.0 or any later version
 * FILE:        devutils/vgafontedit/raw.h
 * PURPOSE:     Raw bitmap fonts
 * COPYRIGHT:   Copyright 2008 Colin Finck <mail@colinfinck.de>
 */

#ifndef __RAW_H
#define __RAW_H

typedef struct _BITMAP_FONT
{
    UCHAR Bits[2048];
} BITMAP_FONT, *PBITMAP_FONT;

#endif
