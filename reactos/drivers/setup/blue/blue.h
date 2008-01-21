/*
 * PROJECT:         ReactOS Setup Driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/setup/blue/font.h
 * PURPOSE:         Loading specific fonts into VGA
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* DEFINITIONS ***************************************************************/

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

#define TAB_WIDTH          8

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

void ScrLoadFontTable(UINT CodePage);
