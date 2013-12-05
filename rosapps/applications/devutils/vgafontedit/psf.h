/*
 * PROJECT:     ReactOS VGA Font Editor
 * LICENSE:     GNU General Public License Version 2.0 or any later version
 * FILE:        devutils/vgafontedit/psf.h
 * PURPOSE:     PSFv1 (PC Screen) Fonts - Version 1
 * COPYRIGHT:   Copyright 2008 Colin Finck <mail@colinfinck.de>
 */

#ifndef __PSF_H
#define __PSF_H

#define PSF1_MAGIC0                   0x36
#define PSF1_MAGIC1                   0x04

typedef struct _PSF1_HEADER
{
    UCHAR uMagic[2];
    UCHAR uMode;
    UCHAR uCharSize;
} PSF1_HEADER, *PPSF1_HEADER;

#endif
