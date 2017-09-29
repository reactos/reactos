/*
 * PROJECT:     ReactOS VGA Font Editor
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     CPI (Code Page Information) MS-DOS-compatible Fonts
 *              We support only screen fonts, not printer fonts.
 *              No DR-DOS/Novell-DOS compressed font format support.
 * COPYRIGHT:   Copyright 2014 Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef __CPI_H
#define __CPI_H

typedef struct _CPI_HEADER
{
    UCHAR   uId0;
    UCHAR   uId[7];
    UCHAR   uReserved[8];
    USHORT  uNumPtr;
    UCHAR   uPtrType;
    ULONG   uFntInfoHdrOffset;

    // FIXME: Put it in another struct ??
    USHORT  uNumCodePages;
} CPI_HEADER, *PCPI_HEADER;

typedef struct _CPENTRY_HEADER
{
    USHORT  uHdrSize;
    ULONG   uNextCPEHOffset;
    USHORT  uDeviceType;
    UCHAR   uDeviceName[8];
    USHORT  uCodePage;
    UCHAR   uReserved[6];
    ULONG   uCPIHOffset;
} CPENTRY_HEADER, *PCPENTRY_HEADER;

typedef struct _CPINFO_HEADER
{
    USHORT  uVersion;
    USHORT  uNumFonts;
    USHORT  uSize;  // uFontSize
} CPINFO_HEADER, *PCPINFO_HEADER;

typedef struct _SCRFONT_HEADER
{
    UCHAR   uHeight;
    UCHAR   uWidth;
    USHORT  uReserved;
    USHORT  uNumChars;
} SCRFONT_HEADER, *PSCRFONT_HEADER;

#endif
