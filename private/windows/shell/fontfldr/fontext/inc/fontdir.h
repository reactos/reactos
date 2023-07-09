#ifndef __FONTDIR_H__
#define __FONTDIR_H__

/* font file header (Adaptation Guide section 6.4) */

typedef struct {
    WORD     dfVersion;         /* not in FONTINFO */
    DWORD    dfSize;            /* not in FONTINFO */
    BYTE     dfCopyright[60];   /* not in FONTINFO */
    WORD     dfType;
    WORD     dfPoints;
    WORD     dfVertRes;
    WORD     dfHorizRes;
    WORD     dfAscent;
    WORD     dfInternalLeading;
    WORD     dfExternalLeading;
    BYTE     dfItalic;
    BYTE     dfUnderline;
    BYTE     dfStrikeOut;
    WORD     dfWeight;
    BYTE     dfnCharSet;
    WORD     dfPixWidth;
    WORD     dfPixHeight;
    BYTE     dfPitchAndFamily;
    WORD     dfAvgWidth;
    WORD     dfMaxWidth;
    BYTE     dfFirstChar;
    BYTE     dfLastChar;
    BYTE     dfDefaultCHar;
    BYTE     dfBreakChar;
    WORD     dfWidthBytes;
    DWORD    dfDevice;          /* See Adaptation Guide 6.3.10 and 6.4 */
    DWORD    dfFace;            /* See Adaptation Guide 6.3.10 and 6.4 */
    DWORD    dfBitsPointer;     /* See Adaptation Guide 6.3.10 and 6.4 */
} FFH;

/*
    The lpFDirEntry is a string corresponding to the resource
    index (two bytes) prepended to an Fontdefs.h FFH structure, with device
    and face name strings appended
    First word is number of fonts, skip to first font resource name
*/

typedef struct {
    WORD    dfFontCount;        /* Overall info */
    WORD    dfSkipper;          /* ?? */
    FFH     xFFH;
    char    cfFace;
} FFHWRAP, FAR* LPFHHWRAP;

#endif



/****************************************************************************
 * $lgb$
 * 1.0     7-Mar-94   eric Initial revision.
 * $lge$
 *
 ****************************************************************************/

