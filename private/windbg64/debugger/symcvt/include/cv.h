/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    cv.h

Abstract:

    This file contains all of the type definitions for accessing
    CODEVIEW data.

Author:

    Wesley A. Witt (wesw) 19-April-1993

Environment:

    Win32, User Mode

--*/
#include <cvtypes.h>
#include <cvinfo.h>
#include <cvexefmt.h>

#ifdef __cplusplus
extern "C" {
#endif

// Global Segment Info table
typedef struct _sgf {
    unsigned short      fRead   :1;
    unsigned short      fWrite  :1;
    unsigned short      fExecute:1;
    unsigned short      f32Bit  :1;
    unsigned short      res1    :4;
    unsigned short      fSel    :1;
    unsigned short      fAbs    :1;
    unsigned short      res2    :2;
    unsigned short      fGroup  :1;
    unsigned short      res3    :3;
} SGF;

typedef struct _sgi {
    SGF                 sgf;        // Segment flags
    unsigned short      iovl;       // Overlay number
    unsigned short      igr;        // Group index
    unsigned short      isgPhy;     // Physical segment index
    unsigned short      isegName;   // Index to segment name
    unsigned short      iclassName; // Index to segment class name
    unsigned long       doffseg;    // Starting offset inside physical segment
    unsigned long       cbSeg;      // Logical segment size
} SGI;

typedef struct _sgm {
    unsigned short      cSeg;       // number of segment descriptors
    unsigned short      cSegLog;    // number of logical segment descriptors
} SGM;

#define FileAlign(x)  ( ((x) + p->optrs.optHdr->FileAlignment - 1) &  \
                            ~(p->optrs.optHdr->FileAlignment - 1) )
#define SectionAlign(x) (((x) + p->optrs.optHdr->SectionAlignment - 1) &  \
                            ~(p->optrs.optHdr->SectionAlignment - 1) )

#define NextSym32(m)  ((DATASYM32 *) \
  (((DWORD)(m) + sizeof(DATASYM32) + \
    ((DATASYM32*)(m))->name[0] + 3) & ~3))

#define NextSym16(m)  ((DATASYM16 *) \
  (((DWORD)(m) + sizeof(DATASYM16) + \
    ((DATASYM16*)(m))->name[0] + 1) & ~1))

#ifdef __cplusplus
} // extern "C" {
#endif
