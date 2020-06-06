/*
 * PROJECT:     ReactOS Console Text-Mode Device Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Main Header File.
 * COPYRIGHT:   Copyright 1999 Boudewijn Dekker
 *              Copyright 1999-2019 Eric Kohl
 */

#ifndef _BLUE_PCH_
#define _BLUE_PCH_

#include <ntifs.h>

#define TAG_BLUE    'EULB'

#define TAB_WIDTH   8
#define MAX_PATH    260

typedef struct _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES;

// Define material that normally comes from PSDK
// This is mandatory to prevent any inclusion of
// user-mode stuff.
typedef struct tagCOORD
{
    SHORT X;
    SHORT Y;
} COORD, *PCOORD;

typedef struct tagSMALL_RECT
{
    SHORT Left;
    SHORT Top;
    SHORT Right;
    SHORT Bottom;
} SMALL_RECT;

typedef struct tagCONSOLE_SCREEN_BUFFER_INFO
{
    COORD      dwSize;
    COORD      dwCursorPosition;
    USHORT     wAttributes;
    SMALL_RECT srWindow;
    COORD      dwMaximumWindowSize;
} CONSOLE_SCREEN_BUFFER_INFO, *PCONSOLE_SCREEN_BUFFER_INFO;

typedef struct tagCONSOLE_CURSOR_INFO
{
    ULONG dwSize;
    INT   bVisible; // BOOL
} CONSOLE_CURSOR_INFO, *PCONSOLE_CURSOR_INFO;

#define ENABLE_PROCESSED_OUTPUT                 0x0001
#define ENABLE_WRAP_AT_EOL_OUTPUT               0x0002

#include <blue/ntddblue.h>

/*
 * Color attributes for text and screen background
 */
#define FOREGROUND_BLUE                 0x0001
#define FOREGROUND_GREEN                0x0002
#define FOREGROUND_RED                  0x0004
#define FOREGROUND_INTENSITY            0x0008
#define BACKGROUND_BLUE                 0x0010
#define BACKGROUND_GREEN                0x0020
#define BACKGROUND_RED                  0x0040
#define BACKGROUND_INTENSITY            0x0080

typedef struct _CFHEADER
{
    ULONG Signature;        // File signature 'MSCF' (CAB_SIGNATURE)
    ULONG Reserved1;        // Reserved field
    ULONG CabinetSize;      // Cabinet file size
    ULONG Reserved2;        // Reserved field
    ULONG FileTableOffset;  // Offset of first CFFILE
    ULONG Reserved3;        // Reserved field
    USHORT Version;          // Cabinet version (CAB_VERSION)
    USHORT FolderCount;      // Number of folders
    USHORT FileCount;        // Number of files
    USHORT Flags;            // Cabinet flags (CAB_FLAG_*)
    USHORT SetID;            // Cabinet set id
    USHORT CabinetNumber;    // Zero-based cabinet number
} CFHEADER, *PCFHEADER;

typedef struct _CFFILE
{
    ULONG FileSize;         // Uncompressed file size in bytes
    ULONG FileOffset;       // Uncompressed offset of file in the folder
    USHORT FileControlID;    // File control ID (CAB_FILE_*)
    USHORT FileDate;         // File date stamp, as used by DOS
    USHORT FileTime;         // File time stamp, as used by DOS
    USHORT Attributes;       // File attributes (CAB_ATTRIB_*)
    /* After this is the NULL terminated filename */
} CFFILE, *PCFFILE;

#define CAB_SIGNATURE      0x4643534D // "MSCF"

#define VIDMEM_BASE        0xb8000
#define BITPLANE_BASE      0xa0000

#define CRTC_COMMAND       ((PUCHAR)0x3d4)
#define CRTC_DATA          ((PUCHAR)0x3d5)

#define CRTC_COLUMNS       0x01
#define CRTC_OVERFLOW      0x07
#define CRTC_ROWS          0x12
#define CRTC_SCANLINES     0x09
#define CRTC_CURSORSTART   0x0a
#define CRTC_CURSOREND     0x0b
#define CRTC_CURSORPOSHI   0x0e
#define CRTC_CURSORPOSLO   0x0f

#define SEQ_COMMAND        ((PUCHAR)0x3c4)
#define SEQ_DATA           ((PUCHAR)0x3c5)

#define GCT_COMMAND        ((PUCHAR)0x3ce)
#define GCT_DATA           ((PUCHAR)0x3cf)

/* SEQ regs numbers*/
#define SEQ_RESET            0x00
#define SEQ_ENABLE_WRT_PLANE 0x02
#define SEQ_MEM_MODE         0x04

/* GCT regs numbers */
#define GCT_READ_PLANE     0x04
#define GCT_RW_MODES       0x05
#define GCT_GRAPH_MODE     0x06

#define ATTRC_WRITEREG     ((PUCHAR)0x3c0)
#define ATTRC_READREG      ((PUCHAR)0x3c1)
#define ATTRC_INPST1       ((PUCHAR)0x3da)

#define MISC         (PUCHAR)0x3c2
#define SEQ          (PUCHAR)0x3c4
#define SEQDATA      (PUCHAR)0x3c5
#define CRTC         (PUCHAR)0x3d4
#define CRTCDATA     (PUCHAR)0x3d5
#define GRAPHICS     (PUCHAR)0x3ce
#define GRAPHICSDATA (PUCHAR)0x3cf
#define ATTRIB       (PUCHAR)0x3c0
#define STATUS       (PUCHAR)0x3da
#define PELMASK      (PUCHAR)0x3c6
#define PELINDEX     (PUCHAR)0x3c8
#define PELDATA      (PUCHAR)0x3c9

VOID ScrLoadFontTable(_In_ ULONG CodePage);
VOID ScrSetFont(_In_ PUCHAR FontBitfield);

#endif /* _BLUE_PCH_ */
