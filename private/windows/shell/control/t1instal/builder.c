/***
 **
 **   Module: Builder
 **
 **   Description:
 **    This is a module of the T1 to TT font converter. The module
 **    contains functions that will write the tables found in a
 **    TrueType font file.
 **
 **   Author: Michael Jansson
 **
 **   Created: 5/26/93
 **
 ***/



/**** INCLUDES */
/* General types and definitions. */
#include <string.h>     /* Prototype for memset */
#include <limits.h>

/* Special types and definitions. */
#include "types.h"
#include "metrics.h"
#include "safemem.h"
#include "encoding.h"
#include "t1msg.h"

/* Module dependent types and prototypes. */
#include "titott.h"
#include "builder.h"
#include "fwriter.h"

#if DBG
#define STATIC
#else
#define STATIC static
#endif


/***** LOCAL TYPES */
struct GlyphKerning {
	USHORT left;
	USHORT right;
	funit delta;
};

struct GlyphList {
   long offset;            /* File offset of the sub-table for this glyph. */
   const struct encoding *code;  /* Encoding key (see the "Encoding" module. */
   funit aw;               /* Advance width. */
   funit lsb;              /* Left side bearing. */
   USHORT pts;             /* Total number of points in the glyph. */
   Point bbox[2];          /* Bounding box of the glyph. */
   USHORT conts;           /* Number of contours. */
};


/* MS cmap encoding sub-table. */
struct MSEncoding {
   USHORT segCount;
   USHORT *startCount;
   USHORT *endCount;
   USHORT *idOffsets;
   USHORT *gi;
   USHORT giCount;
   USHORT giMax;
};


struct TTHandle {
   OutputFile *file;
   Point bbox[2];
   struct GlyphList *pool;

   /* Accumulative 'maxp' entries. */
   USHORT count;
   USHORT maxcnt;
   USHORT maxpts;
   USHORT maxcontours;
   USHORT maxcomppts;
   USHORT maxcompcont;
   USHORT maxcompelements;
   USHORT maxstack;
   USHORT maxinstructions;
   USHORT maxtwilight;
};



/***** CONSTANTS */
#define SHORT_LOCA_MAX  65535
#define KERN_HORIZONTAL 0x0001
#define KERN_PAIR   0x0000
#define KERN_FORMAT0   0x0000

#define GASP_GRIDFIT    0x0001
#define GASP_DOGRAY     0x0002

#define PLT_MAC         (USHORT)1
#define PLT_MS          (USHORT)3

#define ENC_ROMAN       (USHORT)0

// for platform id = 3 cmap table, symbol font or ugl

#define ENC_SYMBOL      (USHORT)0
#define ENC_UGL         (USHORT)1

#define LAN_MS_US       (USHORT)0x0409
#define LAN_MAC_US      (USHORT)0

#define COPYRIGHT       (USHORT)0
#define FAMILY          (USHORT)1
#define SUBFAMILY       (USHORT)2
#define ID              (USHORT)3
#define FULLNAME        (USHORT)4
#define VERSION         (USHORT)5
#define PSNAME          (USHORT)6
#define NOTICE          (USHORT)7


/* Glyph constants. */
#define FLG_ONCURVE     0x01
#define FLG_SHORTX      0x02
#define FLG_SHORTY      0x04
#define FLG_REPEAT      0x08
#define FLG_SAMEX       0x10
#define FLG_SAMEY       0x20

#define ARGS_1_2_ARE_WORDS  0x0001
#define ARGS_ARE_XY_VALUES  0x0002
#define ROUND_XY_TO_GRID    0x0004
#define MORE_COMPONENTS     0x0020

#define GLYPHBUF     64          /* GlyphList's that are allocated each time */
#define MACSIZE      (USHORT)256 /* Length of the Mac encoding vector. */

/* Table constants. */
#define FIRSTCHAR    (USHORT)0x0020 /* First defined char. */
#define LASTCHAR     (USHORT)0xf002 /* Last defined char. */
#define MAXZONES     (USHORT)2      /* Number of zones in the font. */
#define MAXIDEFS     (USHORT)0      /* Number of idefs in the fpgm. */
#define MAXDEPTH     (USHORT)1      /* Number of recursions in composits. */
#define FM_READONLY  (USHORT)2      /* fsType Read Only. */
#define NO_CLASS     (USHORT)0      /* 0 = No class id for the font. */
#define OS2VERSION   (USHORT)0      /* Version of the OS/2 table. */
#define CARET_RISE   (USHORT)1      /* Vertical caret slope rise. */
#define CARET_RUN    (USHORT)0      /* Vertical caret slope run. */
#define RESERVED0    (USHORT)0
#define MAGICCOOKIE  0x5F0F3CF5L    /* Magic cookie. */
#define BASELINEY    (USHORT)0x0001 /* Baseline at y==0 */
#define LOWPPEM      (USHORT)8      /* Lowest PPEM size. */
#define ROMAN        (USHORT)2      /* Direction = left,right&neutrals.*/
#define GLYPH_FORMAT (USHORT)0      /* Current glyphs format. */
#define VERSION0     (USHORT)0      /* Version zero of a table. */
#define NUM_CMAPS    (USHORT)2      /* Number of cmap sub-tables. */
#define SEGMENT_MAP  (USHORT)4      /* MS segment mapping of cmap table. */
#define PAD0         (USHORT)0      /* Padding byte. */
#define MAX_PPEM_SIZE     (USHORT)65535  /* Maximum PPEM size in GASP table. */

/* LOCA constants */
#define SHORTOFFSETS 0
#define LONGOFFSETS  1


/* Weighted average character width. */
STATIC const long Weights[] = {
   64,
   14,
   27,
   35,
   100,
   20,
   14,
   42,
   63,
   3,
   6,
   35,
   20,
   56,
   56,
   17,
   4,
   49,
   56,
   71,
   31,
   10,
   18,
   3,
   18,
   2
};

/***** MACROS */
#define LONGVERSION(v,r)      ((((long)v)<<16L) | (long)r)


/***** STATIC FUNCTIONS */

/***
** Function: SearchRange
**
** Description:
**   Compute the search range key for the CMAP subtable
**   for Windows.
***/
STATIC USHORT SearchRange(const USHORT cnt)
{
   USHORT i;

   i = 0;
   while ((1u<<i) <= cnt) {
      i++;
   }

   return (USHORT)(1<<i);
}



/***
** Function: EntrySelector
**
** Description:
**   Compute the entry selector key for the CMAP subtable
**   for Windows.
***/
STATIC USHORT EntrySelector(const USHORT cnt)
{
   USHORT i;

   i = 0;
   while ((1u<<(i+1)) <= cnt) {
      i++;
   }

   return i;
}



/***
** Function: RangeShift
**
** Description:
**   Compute the range shift key for the CMAP subtable
**   for Windows.
***/
STATIC USHORT RangeShift(const USHORT cnt)
{
   return (USHORT)(2*cnt - SearchRange(cnt));
}



/***
 ** Function: PutGASP
 **
 ** Description:
 **   This function writes the optional 'GASP' table to the
 **   TT font file.
 **
 ***/
STATIC errcode PutGASP(OutputFile *file,
		      const USHORT treshold)
{
   long offset;

   offset = FileTell(file);

   WriteShort(VERSION0, file);
   WriteShort(3, file);

   /* First range 0 - 8 : GRIDFIT */
   WriteShort(8, file);
   WriteShort(GASP_DOGRAY, file);

   /* Second range 8 - onpix : GRIDFIT */
   WriteShort(treshold, file);
   WriteShort(GASP_GRIDFIT, file);

   /* Third range onpix - inf. : GRIDFIT | GRAYSCALE */
   WriteShort(MAX_PPEM_SIZE, file);
   WriteShort(GASP_GRIDFIT | GASP_DOGRAY, file);

   return CompleteTable(offset, TBL_GASP, file);
}


/***
** Function: cmpKern
**
** Description:
**
***/
STATIC int CDECL cmpKern(const void *a1, const void *a2)
{
   const struct GlyphKerning *k1 = a1;
   const struct GlyphKerning *k2 = a2;
   ULONG first;
   ULONG second;

   first = ((k1->left)<<16L) + k1->right;
   second = ((k2->left)<<16L) + k2->right;

   return (int)(first - second);
}


/***
 ** Function: StdEncToGlyphIndex
 **
 ** Description:
 **   This function maps an StdEncoding character code to a
 **   glyph index.
 **
 ***/
USHORT StdEncToGlyphIndex(const struct GlyphList *pool,
						  const USHORT count,
						  const USHORT code)
{
	USHORT i;

	for (i=0; i<count; i++) {
            if (LookupCharCode(pool[i].code, ENC_MSWINDOWS) == code)
                 return i;
	}

	return 0;
}


/***
 ** Function: PutKERN
 **
 ** Description:
 **   This function writes the optional 'KERN' table to the
 **   TT font file.
 **
 ***/
STATIC errcode PutKERN(OutputFile *file,
					   struct kerning *charkerns,
					   const USHORT kernsize,
					   const struct GlyphList *pool,
					   const USHORT count)
{
   struct GlyphKerning *kerns;
   long offset;
   unsigned i;
   USHORT cnt;

   if ((kerns = malloc(sizeof(struct GlyphKerning)*kernsize))==NULL)
	   return FAILURE;

   /* Translate the kerning from char codes to glyph index. */
   for (i=0, cnt=0; i<kernsize; i++) {
           if ((kerns[cnt].left  = StdEncToGlyphIndex(pool, count, charkerns[i].left))!=0 &&
               (kerns[cnt].right = StdEncToGlyphIndex(pool, count, charkerns[i].right))!=0)
           {
		   kerns[cnt].delta = charkerns[i].delta;
		   cnt++;
	   }
   }
   /* Sort the kerning pairs. */
   qsort((void *)kerns, cnt, sizeof(struct GlyphKerning), cmpKern);


   offset = FileTell(file);

   WriteShort(VERSION0, file);
   WriteShort(1, file);

   /* First sub-table header. */
   WriteShort(VERSION0, file);
   WriteShort((USHORT)(2+2+2+ 2+2+2+2+ cnt*(2+2+2)), file);
   WriteShort(KERN_HORIZONTAL | KERN_PAIR | KERN_FORMAT0, file);

   /* First sub-table, format 0 */
   WriteShort(cnt, file);
   WriteShort(SearchRange(cnt), file);
   WriteShort(EntrySelector(cnt), file);
   WriteShort(RangeShift(cnt), file);
   for (i=0; i<cnt; i++) {
      WriteShort((USHORT)kerns[i].left, file);
      WriteShort((USHORT)kerns[i].right, file);
      WriteShort((USHORT)kerns[i].delta, file);
   }

   free(kerns);

   return CompleteTable(offset, TBL_KERN, file);
}


/***
 ** Function: PutCVT
 **
 ** Description:
 **   This function writes the optional 'cvt' table to the
 **   TT font file.
 **
 ***/
STATIC errcode PutCVT(OutputFile *file,
		      const short *ppgm,
		      const USHORT num)
{
   USHORT i;
   long offset;

   offset = FileTell(file);

   for (i=0; i<num; i++)
      WriteShort((USHORT)ppgm[i], file);

   return CompleteTable(offset, TBL_CVT, file);
}



/***
 ** Function: PutPREP
 **
 ** Description:
 **   This function writes the optional 'prep' table to the
 **   TT font file.
 **
 ***/
STATIC errcode PutPREP(OutputFile *file,
		       const UBYTE *prep,
		       const USHORT num)
{
   long offset;

   offset = FileTell(file);

   (void)WriteBytes(prep, num, file);

   return CompleteTable(offset, TBL_PREP, file);
}



/***
 ** Function: PutFPGM
 **
 ** Description:
 **   This function writes the optional 'fpgm' table to the
 **   TT font file.
 **
 ***/
STATIC errcode PutFPGM(OutputFile *file,
		       const UBYTE *fpgm,
		       const USHORT num)
{
   long offset;

   offset = FileTell(file);

   (void)WriteBytes(fpgm, num, file);

   return CompleteTable(offset, TBL_FPGM, file);
}



/***
 ** Function: PutPOST
 **
 ** Description:
 **   This function writes the required 'post' table to the
 **   TT font file.
 **
 ***/
STATIC errcode PutPOST(OutputFile *file,
		       struct GlyphList *pool,
		       USHORT count,
		       struct TTMetrics *ttm)
{
   const char *str;
   long offset;
   USHORT i;

   offset = FileTell(file);
   WriteLong(LONGVERSION(2, 0), file);
   WriteLong((ULONG)ttm->angle, file);
   WriteShort((USHORT)ttm->underline, file);
   WriteShort((USHORT)ttm->uthick, file);
   WriteLong((ULONG)ttm->isFixPitched, file);
   WriteLong(0L, file);
   WriteLong(0L, file);
   WriteLong(0L, file);
   WriteLong(0L, file);

   /* Write the character codes. */
   WriteShort(count, file);
   for (i=0; i<count; i++) {
      if (pool[i].code)
	 WriteShort(LookupCharCode(pool[i].code, ENC_MACCODES), file);
      else
	 WriteShort((USHORT)0, file);
   }

   /* Write the character names. */
   for (i=0; i<count; i++) {
      if (pool[i].code) {
	 str = LookupCharName(pool[i].code);
	 WriteByte((UBYTE)strlen(str), file);
	 (void)WriteBytes((UBYTE*)str, (USHORT)strlen(str), file);
      }
   }

   return CompleteTable(offset, TBL_POST, file);
}



/***
 ** Function: PutMAXP
 **
 ** Description:
 **   This function writes the required 'maxp' table to the
 **   TT font file.
 **
 ***/
STATIC errcode PutMAXP(struct TTHandle *tt,
		       const USHORT maxstorage,
		       const USHORT maxprepstack,
		       const USHORT maxfuns)
{
   long offset;

   offset = FileTell(tt->file);
   WriteLong(LONGVERSION(1, 0), tt->file);
   WriteShort(tt->count, tt->file);
   WriteShort(tt->maxpts, tt->file);
   WriteShort(tt->maxcontours, tt->file);
   WriteShort(tt->maxcomppts, tt->file);
   WriteShort(tt->maxcompcont, tt->file);
   WriteShort(MAXZONES, tt->file);
   WriteShort(tt->maxtwilight, tt->file);
   WriteShort(maxstorage, tt->file);
   WriteShort(maxfuns, tt->file);
   WriteShort(MAXIDEFS, tt->file);
   WriteShort((USHORT)MAX(tt->maxstack, maxprepstack), tt->file);
   WriteShort(tt->maxinstructions, tt->file);
   WriteShort(tt->maxcompelements, tt->file);
   WriteShort(MAXDEPTH, tt->file);
   return CompleteTable(offset, TBL_MAXP, tt->file);
}



/***
 ** Function: PutOS2
 **
 ** Description:
 **   This function writes the required 'OS/2' table to the
 **   TT font file.
 **
 ***/
STATIC errcode PutOS2(OutputFile *file,
		      const struct GlyphList *pool,
		      const USHORT count,
		      const struct TTMetrics *ttm)
{
   long offset;
   long aw;
   USHORT i;

   offset = FileTell(file);

   /* Compute some font metrics. */
   aw = 0;

   /* Do a weighted average? */
   if (ttm->Encoding==NULL) {
      for (i=0; i<count; i++) {
	 short letter = (short)LookupCharCode(pool[i].code, ENC_MACCODES);
	 if (letter==' ') {
	    aw = aw + 166L * pool[i].aw;
	 } else if ((letter>='a' && letter <= 'z')) {
	    aw = aw + pool[i].aw * Weights[letter - 'a'];
	 }
      }
      aw /= 1000;
   } else {
      for (i=0; i<count; i++) {
	 aw += pool[i].aw;
      }
      aw = aw / count;
   }

   WriteShort(OS2VERSION, file);
   WriteShort((USHORT)aw, file);
   WriteShort(ttm->usWeightClass, file);
   WriteShort(ttm->usWidthClass, file);
   WriteShort(FM_READONLY, file);
   WriteShort((USHORT)ttm->subsize.x, file);
   WriteShort((USHORT)ttm->subsize.y, file);
   WriteShort((USHORT)ttm->suboff.x, file);
   WriteShort((USHORT)ttm->suboff.y, file);
   WriteShort((USHORT)ttm->supersize.x, file);
   WriteShort((USHORT)ttm->supersize.y, file);
   WriteShort((USHORT)ttm->superoff.x, file);
   WriteShort((USHORT)ttm->superoff.y, file);
   WriteShort((USHORT)ttm->strikesize, file);
   WriteShort((USHORT)ttm->strikeoff, file);
   WriteShort(NO_CLASS, file);

   /* Panose */
   WriteBytes(ttm->panose, (USHORT)10, file);

   /* Char range. */
   WriteLong(0L, file);
   WriteLong(0L, file);
   WriteLong(0L, file);
   WriteLong(0L, file);

   /* Vend ID. */
   WriteLong(0L, file);

   WriteShort(ttm->fsSelection, file);
   WriteShort(FIRSTCHAR, file);
   WriteShort(LASTCHAR, file);
   WriteShort((USHORT)ttm->typAscender, file);
   WriteShort((USHORT)ttm->typDescender, file);
   WriteShort((USHORT)ttm->typLinegap, file);
   WriteShort((USHORT)ttm->winAscender, file);
   WriteShort((USHORT)ttm->winDescender, file);

   return CompleteTable(offset, TBL_OS2, file);
}



/***
 ** Function: PutLOCA
 **
 ** Description:
 **   This function writes the required 'loca' table to the
 **   TT font file.
 **
 ***/
STATIC errcode PutLOCA(OutputFile *file,
		       const struct GlyphList *pool,
		       const USHORT count,
		       short *format)
{
   long offset;
   USHORT i;

   offset = FileTell(file);

   /* Check for offset size format. */
   for (i=0, (*format) = SHORTOFFSETS; i<=count &&
			 (*format)==SHORTOFFSETS; i++) {
      if (pool[i].offset/2>SHORT_LOCA_MAX)
	 (*format) = LONGOFFSETS;
   }

   if ((*format)==LONGOFFSETS)
      for (i=0; i<=count; i++)
	 WriteLong((ULONG)pool[i].offset, file);
   else
      for (i=0; i<=count; i++)
	 WriteShort((USHORT)(pool[i].offset/2), file);

   return CompleteTable(offset, TBL_LOCA, file);
}



/***
 ** Function: PutHMTX
 **
 ** Description:
 **   This function writes the required 'hmtx' table to the
 **   TT font file.
 **
 ***/
STATIC errcode PutHMTX(OutputFile *file,
		       const struct GlyphList *pool,
		       const USHORT count,
		       const funit *widths,
		       const USHORT first,
                       const USHORT last,
                       const  struct encoding *enc)

{
   long offset;
   USHORT std;
   USHORT i;
   USHORT usEnc = (USHORT) (enc? ENC_MACCODES : ENC_UNICODE);

   offset = FileTell(file);

   if (widths) {
      for (i=0; i<count; i++) {
	 if (pool[i].code) {
            std = LookupCharCode(pool[i].code, usEnc);
	 } else {
	    std = NOTDEFGLYPH;
	 }
	 if (std>=first && std<=last)
	    WriteShort((USHORT)widths[std-first], file);
	 else
	    WriteShort((USHORT)pool[i].aw, file);
	 WriteShort((USHORT)pool[i].lsb, file);
      }
   } else {
      for (i=0; i<count; i++) {
	 WriteShort((USHORT)pool[i].aw, file);
	 WriteShort((USHORT)pool[i].lsb, file);
      }
   }
   return CompleteTable(offset, TBL_HMTX, file);
}



/***
 ** Function: PutHHEA
 **
 ** Description:
 **   This function writes the required 'HHEA' table to the
 **   TT font file.
 **
 ***/
STATIC errcode PutHHEA(OutputFile *file,
		       const struct GlyphList *pool,
		       const USHORT count,
		       const Point bbox[2],
                       const funit linegap,
                       const struct TTMetrics *ttm
)
{
   funit awmin, awmax, xmax, lsb;
   long offset;
   USHORT i;

   offset = FileTell(file);

   /* Compute some font metrics. */
   awmax = SHRT_MIN;
   awmin = SHRT_MAX;
   xmax = SHRT_MIN;
   lsb = SHRT_MAX;
   for (i=0; i<count; i++) {
      funit rsb = pool[i].aw - pool[i].lsb -
		  (pool[i].bbox[1].x - pool[i].bbox[0].x);
      funit ext = pool[i].lsb +
		  (pool[i].bbox[1].x - pool[i].bbox[0].x);
      if (ext>xmax)
	 xmax = ext;
      if (rsb<awmin)
	 awmin = rsb;
      if (pool[i].aw>awmax)
	 awmax = pool[i].aw;
      if (pool[i].lsb<lsb)
	 lsb = pool[i].lsb;
   }


   WriteLong(LONGVERSION(1, 0), file);
   WriteShort((USHORT)bbox[1].y, file);
   WriteShort((USHORT)bbox[0].y, file);
   WriteShort((USHORT)linegap, file);
   WriteShort((USHORT)awmax, file);
   WriteShort((USHORT)lsb, file);
   WriteShort((USHORT)awmin, file);
   WriteShort((USHORT)xmax, file);
   WriteShort(CARET_RISE, file);
   WriteShort(CARET_RUN, file);
   WriteShort((USHORT)(ttm->FirstChar   << 8), file);
   WriteShort((USHORT)(ttm->LastChar    << 8), file);
   WriteShort((USHORT)(ttm->DefaultChar << 8), file);
   WriteShort((USHORT)(ttm->BreakChar   << 8), file);
   WriteShort((USHORT)(ttm->CharSet     << 8), file);
   WriteShort(RESERVED0, file);
   WriteShort(count, file);
   return CompleteTable(offset, TBL_HHEA, file);
}



/***
** Function: PutHEAD
**
** Description:
**   This function writes the required 'head' table to the
**   TT font file.
**
***/
STATIC errcode PutHEAD(OutputFile *file,
		       const Point bbox[2],
		       const struct TTMetrics *ttm,
		       const short loca,
		       long *csum)
{
   long offset;

   offset = FileTell(file);

   WriteLong(LONGVERSION(1, 0), file);
   WriteShort(ttm->version.ver, file);
   WriteShort(ttm->version.rev, file);
   (*csum) = (long)FileTell(file);
   WriteLong(0L, file);
   WriteLong(MAGICCOOKIE, file);
   WriteShort(BASELINEY, file);
   WriteShort((USHORT)ttm->emheight, file);
   WriteLong(ttm->created.a, file);WriteLong(ttm->created.b, file);
   WriteLong(ttm->created.a, file);WriteLong(ttm->created.b, file);
   WriteShort((USHORT)bbox[0].x, file);
   WriteShort((USHORT)bbox[0].y, file);
   WriteShort((USHORT)bbox[1].x, file);
   WriteShort((USHORT)bbox[1].y, file);
   WriteShort((USHORT)ttm->macStyle, file);
   WriteShort(LOWPPEM, file);
   WriteShort(ROMAN, file);
   WriteShort((USHORT)loca, file);
   WriteShort(GLYPH_FORMAT, file);

   return CompleteTable(offset, TBL_HEAD, file);
}



/***
** Function: WriteNameEntry
**
** Description:
**   This function writes an entry in the NAME table
**   header for one string.
**
***/
STATIC USHORT WriteNameEntry(OutputFile *file,
			     const USHORT platform,
			     const USHORT encoding,
			     const USHORT language,
			     const USHORT nameid,
			     const char *str,
			     const USHORT off)
{
   USHORT len;

   if (str) {
      len = (USHORT)strlen(str);
      switch (platform) {
	 case PLT_MS:
	    len *= 2;
	    break;
	 case PLT_MAC:
	    len *= 1;
	    break;
	 default:
	    LogError(MSG_WARNING, MSG_PLATFORM, NULL);
	    len *= 1;
	    break;
      }
      WriteShort(platform, file);
      WriteShort(encoding, file);
      WriteShort(language, file);
      WriteShort(nameid, file);
      WriteShort(len, file);
      WriteShort(off, file);
   } else {
      len = 0;
   }

   return len;
}



/***
** Function: WriteNameString
**
** Description:
**   This function write the textual data of a string
**   to the NAME table, according to the platform and
**   encoding schema.
**
***/
STATIC void WriteNameString(OutputFile *file,
			    const USHORT platform,
			    const char *str)
{
   USHORT i;

   if (str) {
      switch (platform) {
	 default:
	 case PLT_MAC:
	    (void)WriteBytes((UBYTE *)str, (USHORT)strlen(str), file);
	    break;
	 case PLT_MS:
	    for (i=0; i<strlen(str); i++)
	       WriteShort(LookupCharCode(DecodeChar(NULL,
						    (short)0,
						    ENC_STANDARD,
						    (USHORT)(UBYTE)str[i]),
					 ENC_UNICODE),
			  file);
	    break;
      }
   }
}



/***
** Function: PutNAME
**
** Description:
**   This function writes the required 'name' table to the
**   TT font file.
**
***/



STATIC errcode PutNAME(OutputFile *file, const struct TTMetrics *ttm)
{
   USHORT stroff = 0;
   USHORT count = 0;
   USHORT encId = ttm->Encoding ? ENC_SYMBOL : ENC_UGL;
   ULONG offset;
   char *id;
   char *pszStyle = NULL;

   if (ttm->usWeightClass < 500)
   {
      if (ttm->angle == 0)
      {
         pszStyle = "Regular";
      }
      else
      {
         pszStyle = "Italic";
      }
   }
   else
   {
      if (ttm->angle == 0)
      {
         pszStyle = "Bold";
      }
      else
      {
         pszStyle = "Bold Italic";
      }
   }

   /* Count the number of names. */
   if (ttm->copyright)
      count++;
   if (ttm->family)
      count++;
   if (pszStyle)
      count++;
   if (ttm->id) {
      count++;
      id = ttm->id;
   } else {
      id = ttm->name;
      count++;
   }
   if (ttm->fullname)
      count++;
   if (ttm->verstr)
      count++;
   if (ttm->name)
      count++;
   if (ttm->notice)
      count++;
   count *= 2;


   /* Write the name table. */
   offset = (ULONG)FileTell(file);
   WriteShort(VERSION0, file);
   WriteShort(count, file);
   WriteShort((USHORT)(6+count*12), file);

   /* Mac names */
   stroff = (USHORT)(stroff + WriteNameEntry(file, PLT_MAC, ENC_ROMAN,
		    LAN_MAC_US, COPYRIGHT,
		    ttm->copyright, stroff));
   stroff = (USHORT)(stroff + WriteNameEntry(file, PLT_MAC, ENC_ROMAN,
		    LAN_MAC_US, FAMILY,
		    ttm->family, stroff));
   stroff = (USHORT)(stroff + WriteNameEntry(file, PLT_MAC, ENC_ROMAN,
		    LAN_MAC_US, SUBFAMILY,
                    pszStyle, stroff));
   stroff = (USHORT)(stroff + WriteNameEntry(file, PLT_MAC, ENC_ROMAN,
		    LAN_MAC_US, ID,
		    id, stroff));
   stroff = (USHORT)(stroff + WriteNameEntry(file, PLT_MAC, ENC_ROMAN,
		    LAN_MAC_US, FULLNAME,
		    ttm->fullname, stroff));
   stroff = (USHORT)(stroff + WriteNameEntry(file, PLT_MAC, ENC_ROMAN,
		    LAN_MAC_US, VERSION,
		    ttm->verstr, stroff));
   stroff = (USHORT)(stroff + WriteNameEntry(file, PLT_MAC, ENC_ROMAN,
		    LAN_MAC_US, PSNAME,
		    ttm->name, stroff));
   stroff = (USHORT)(stroff + WriteNameEntry(file, PLT_MAC, ENC_ROMAN,
		    LAN_MAC_US, NOTICE,
		    ttm->notice, stroff));

   /* MS names */
   stroff = (USHORT)(stroff + WriteNameEntry(file, PLT_MS, encId,
		    LAN_MS_US, COPYRIGHT,
		    ttm->copyright, stroff));
   stroff = (USHORT)(stroff + WriteNameEntry(file, PLT_MS, encId,
		    LAN_MS_US, FAMILY,
		    ttm->family, stroff));
   stroff = (USHORT)(stroff + WriteNameEntry(file, PLT_MS, encId,
		    LAN_MS_US, SUBFAMILY,
                    pszStyle, stroff));
   stroff = (USHORT)(stroff + WriteNameEntry(file, PLT_MS, encId,
		    LAN_MS_US, ID,
		    id, stroff));
   stroff = (USHORT)(stroff + WriteNameEntry(file, PLT_MS, encId,
		    LAN_MS_US, FULLNAME,
		    ttm->fullname, stroff));
   stroff = (USHORT)(stroff + WriteNameEntry(file, PLT_MS, encId,
		    LAN_MS_US, VERSION,
		    ttm->verstr, stroff));
   stroff = (USHORT)(stroff + WriteNameEntry(file, PLT_MS, encId,
		    LAN_MS_US, PSNAME,
		    ttm->name, stroff));
   stroff = (USHORT)(stroff + WriteNameEntry(file, PLT_MS, encId,
		    LAN_MS_US, NOTICE,
		    ttm->notice, stroff));

   WriteNameString(file, PLT_MAC, ttm->copyright);
   WriteNameString(file, PLT_MAC, ttm->family);
   WriteNameString(file, PLT_MAC, pszStyle);
   WriteNameString(file, PLT_MAC, id);
   WriteNameString(file, PLT_MAC, ttm->fullname);
   WriteNameString(file, PLT_MAC, ttm->verstr);
   WriteNameString(file, PLT_MAC, ttm->name);
   WriteNameString(file, PLT_MAC, ttm->notice);

   WriteNameString(file, PLT_MS, ttm->copyright);
   WriteNameString(file, PLT_MS, ttm->family);
   WriteNameString(file, PLT_MS, pszStyle);
   WriteNameString(file, PLT_MS, id);
   WriteNameString(file, PLT_MS, ttm->fullname);
   WriteNameString(file, PLT_MS, ttm->verstr);
   WriteNameString(file, PLT_MS, ttm->name);
   WriteNameString(file, PLT_MS, ttm->notice);

   return CompleteTable((long)offset, TBL_NAME, file);
}



/***
** Function: BoundingBox
**
** Description:
**   Extend an already initialized rectangle (two points)
**   so that it encolses a number of coordinates.
***/
STATIC void BoundingBox(Point bbox[2],
			const Point *pts,
			const USHORT cnt)
{
   USHORT i;

   for (i=0; i<cnt; i++) {
      if (bbox[0].x > pts[i].x)
	 bbox[0].x = pts[i].x;
      if (bbox[1].x < pts[i].x)
	 bbox[1].x = pts[i].x;
      if (bbox[0].y > pts[i].y)
	 bbox[0].y = pts[i].y;
      if (bbox[1].y < pts[i].y)
	 bbox[1].y = pts[i].y;
   }
}



/***
** Function: RecordGlyph
**
** Description:
**   Record information about glyph record of the glyf table.
***/
STATIC errcode RecordGlyph(struct TTHandle *tt,
			   const struct encoding *code,
			   const Point *bbox,
			   const funit aw,
			   const USHORT pts,
			   const USHORT conts)
{
   errcode status;
   USHORT i;

   i = tt->count;

   /* Make sure that there is enough memory in the pool. */
   if (tt->count+1>=tt->maxcnt) {
      struct GlyphList *gl;

      if ((gl = Realloc(tt->pool,
			(size_t)(tt->maxcnt+GLYPHBUF)*
			sizeof(struct GlyphList)))==NULL) {
	 SetError(status=NOMEM);
	 return status;
      } else {
	 tt->maxcnt += GLYPHBUF;
	 tt->pool = gl;
      }
   }

   /* Record metrics. */
   tt->count++;
   tt->pool[i].pts = pts;
   tt->pool[i].conts = conts;
   tt->pool[i].lsb = bbox[0].x;
   tt->pool[i].aw = aw;
   tt->pool[i].bbox[0] = bbox[0];
   tt->pool[i].bbox[1] = bbox[1];
   tt->pool[i].code = code;
   tt->pool[i].offset = FileTell(tt->file) - 12L - (long)TBLDIRSIZE*NUMTBL;

   /* Update the global bounding box. */
   BoundingBox(tt->bbox, bbox, (short)2);

   /* Update maxp. */
   if (conts>tt->maxcontours)
      tt->maxcontours = conts;
   if (pts>tt->maxpts)
      tt->maxpts = pts;

   return SUCCESS;
}



/***
** Function: BuildMacCMAP
**
** Description:
**   Compute the CMAP subtable for the Mac.
***/
STATIC void BuildMacCMAP(const struct GlyphList *pool,
                         const USHORT count,
                         UBYTE *ascii2gi,
                         const struct encoding *encRoot,
                         const int encSize)
{
   const struct encoding *notdef = LookupNotDef();
   USHORT code;
   UBYTE i;

   /* Initiate the ascii to glyph-index array. Glyph 0 is the "notdef"
		character, so any unassigned character will be mapped to "notdef". */
   memset(ascii2gi, NOTDEFGLYPH, (unsigned int)MACSIZE);

   /* Build the ascii to glyph-index array. */
   if (encRoot==NULL)
   {
      for (i=2; i<MIN(255,count); i++)
      {
         if (pool[i].code!=NULL)
         {
            /* i = glyph index, Lookup..() = character code.
            Map glyph i only if it is a valid Mac character. */
            if (pool[i].code!=NULL &&
                (code = LookupCharCode(pool[i].code,ENC_MACCODES))!=NOTDEFCODE)
               ascii2gi[code] = i;
         }
      }
   }
   else
   {
      for (i=2; i<MIN(255,count); i++)
      {
         if (pool[i].code!=NULL && pool[i].code!=notdef)
         {
            const struct encoding *encGlyph;
            encGlyph = LookupFirstEnc(encRoot, encSize, pool[i].code);
            do
            {
               if ((code = LookupCharCode(encGlyph, ENC_MACCODES))!=NOTDEFCODE)
                  ascii2gi[code] = i;
            } while (encGlyph = LookupNextEnc(encRoot, encSize, encGlyph));
         }
      }
   }

   /* Constant Mac glyph/encoding mapping for standard encoded fonts */

   if (encRoot==NULL)
   {
      /* Missing glyphs. */
      for (i=1; i<=31; i++)
              ascii2gi[i] = NOTDEFGLYPH;
      ascii2gi[127] = NOTDEFGLYPH;

      /* Null glyphs. */
      ascii2gi[0] = 1;
      ascii2gi[8] = 1;
      ascii2gi[13] = 1;
      ascii2gi[29] = 1;

      /* No countours + positive advance width. */
      ascii2gi[9] = ascii2gi[32];
      ascii2gi[13] = ascii2gi[32];
      ascii2gi[202] = ascii2gi[32];
   }
}



/***
** Function: FreeMSEncoding
**
** Description:
**   Free resourses used while computing the CMAP subtable
**   for Windows.
***/
STATIC void FreeMSEncoding(struct MSEncoding *ms)
{
   if (ms->startCount)
      Free(ms->startCount);

   if (ms->gi)
      Free(ms->gi);
}



/***
** Function: BuildMSCMAP
**
** Description:
**   Compute the CMAP subtable for Windows.
***/
STATIC errcode BuildMSCMAP(const struct GlyphList *pool,
const  USHORT           count,
struct MSEncoding      *ms,
const  struct encoding *encRoot,
const  int              encSize
)
{
   USHORT *twobyte = NULL;
   USHORT idOffset;
   USHORT code, max;
   USHORT i, j, k, big, n;

   /* Get the range of the UGL characters. */
   max = 0;
   big = 0;

   if (encRoot==NULL)
   {
      for (i=2; i<count; i++)
      {
         if (pool[i].code!=NULL)
         {
            if ((code = LookupCharCode(pool[i].code, ENC_UNICODE))!=NOTDEFCODE)
            {
               if (code<=0xff)
               {
                  if (code>max)
                     max = code;
               }
               else
               {
                  big++;
               }
            }
         }
      }
   }
   else
   /* A non-standard encoded font, i.e. a fonts with an explicit
      encoding array may reference the same glyph more than once,
           though each glyph only refers to one encoding item. We have to
           enumerate through all code point for each glyph in this case.
   */
   {
      for (i=2; i<count; i++)
      {
         if (pool[i].code!=NULL)
         {
            const struct encoding *encGlyph = LookupFirstEnc(encRoot,
                                                             encSize,pool[i].code);
            do
            {
               if ((code = LookupCharCode(encGlyph, ENC_MACCODES))!=NOTDEFCODE)
               {
                  if (code>max)
                     max = code;
               }

            } while (encGlyph = LookupNextEnc(encRoot, encSize, encGlyph));
         }
      }
   }

   max++;
   max = (USHORT)(max + big);
   if ((ms->gi = Malloc(sizeof(USHORT)*max))==NULL) {
      return NOMEM;
   }
   memset(ms->gi, NOTDEFGLYPH, max*sizeof(USHORT));

   if (big && (twobyte = Malloc(sizeof(USHORT)*big))==NULL) {
      Free(ms->gi);
      ms->gi = NULL;
      return NOMEM;
   }
	
   j = 0;
   if (encRoot==NULL)
   {
      /* Glyph zero and Glyp one are the "notdef" and the "null" glyph,
              and are not encoded here, so skip the first two glyph.
      */
      for (i=2; i<count; i++)
      {
         code = LookupCharCode(pool[i].code, ENC_UNICODE);
         if (pool[i].code && code!=NOTDEFCODE)
         {
            if (code<=0xff)
            {
               ms->gi[code] = i;
            }
            else
            {
               for (k=0; k<j; k++)
                  if (twobyte[k]>code)
                     break;
               for (n=j; n>k; n--)
               {
                  twobyte[n] = twobyte[n-1];
                  ms->gi[max-big+n] = ms->gi[max-big+n-1];
               }
               twobyte[k] = code;
               ms->gi[max-big+k] = i;
               j++;
            }
         }
      }
   }
   else
   {
      for (i=2; i<count; i++)
      {
         const struct encoding *encGlyph;

         if (pool[i].code)
         {
            encGlyph = LookupFirstEnc(encRoot, encSize, pool[i].code);
            do
            {
               if ((code = LookupCharCode(encGlyph, ENC_MACCODES))!=NOTDEFCODE)
               {
                  ms->gi[code] = i;
               }
            } while (encGlyph = LookupNextEnc(encRoot, encSize, encGlyph));
         }
      }
   }

   /* Count the segments. */
   ms->segCount=(USHORT)(2+big);
   for (i=0; i<max-big-1; i++) {
      if (ms->gi[i]!=NOTDEFGLYPH && ms->gi[i+1]==NOTDEFGLYPH) {
	 ms->segCount++;
      }
   }

   ms->startCount = Malloc(3 * (sizeof(USHORT)*ms->segCount));

   if (ms->startCount==NULL) {
      if (twobyte)
	 Free(twobyte);
      FreeMSEncoding(ms);
      return NOMEM;
   }

   ms->endCount =  (USHORT *)((char *)ms->startCount + sizeof(USHORT)*ms->segCount);
   ms->idOffsets = (USHORT *)((char *)ms->endCount +  sizeof(USHORT)*ms->segCount);

   /* i=UGL index, j=segment index, k=glyph index. */
   for (i=0, j=0, k=0; i<max-big; i++) {
      if (ms->gi[i]!=NOTDEFGLYPH) {
	 if (i==0 || (ms->gi[i-1]==NOTDEFGLYPH)) {
	    ms->startCount[j] = i;
	    ms->idOffsets[j] = (USHORT)((ms->segCount-j+k)*2);
	 }
	 if ((i==max-1-big) || (ms->gi[i+1]==NOTDEFGLYPH)) {
	    ms->endCount[j] = i;
	    j++;
	 }
	 k++;
      }
   }

   /* Segment for the double byte characters. */
   idOffset = (USHORT)((ms->segCount-j+k)*2);
   for (i=0; i<big; i++) {
      ms->startCount[j] = twobyte[i];
      ms->idOffsets[j] = idOffset;
      ms->endCount[j] = twobyte[i];
      k++;
      j++;
   }

   ms->giCount = k;
   ms->giMax = max;

   /* Sentinel segments. */
   ms->startCount[ms->segCount-1] = 0xffff;
   ms->endCount[ms->segCount-1] = 0xffff;
   ms->idOffsets[ms->segCount-1] = 0;

   if (twobyte)
      Free(twobyte);

   return SUCCESS;
}



/***
** Function: PutCMAP
**
** Description:
**   This function writes the required 'cmap' table to the
**   TT font file.
***/
STATIC errcode PutCMAP(
struct TTHandle *tt,
UBYTE *ascii2gi,
const struct encoding *enc,
const int encSize)
{
   struct MSEncoding ms;
   long end, offset;
   errcode status = SUCCESS;
   USHORT i;
   USHORT usBias = (USHORT)(enc ? 0xf000 : 0); // bias for the first glyph

   /* Build Mac encoding table. */
   BuildMacCMAP(tt->pool, tt->count, ascii2gi, enc, encSize);


   /* Build MS encoding table. */
   if ((status = BuildMSCMAP(tt->pool, tt->count, &ms, enc, encSize))!=SUCCESS)
      return status;

   offset = FileTell(tt->file);

   /* Write cmap table. */
   WriteShort(VERSION0, tt->file);
   WriteShort(NUM_CMAPS, tt->file);

   /*== CMAP table directory ==*/
   WriteShort(PLT_MAC, tt->file);
   WriteShort(ENC_ROMAN, tt->file);
   WriteLong(0L, tt->file);
   WriteShort(PLT_MS, tt->file);
   WriteShort((USHORT)(enc ? ENC_SYMBOL : ENC_UGL), tt->file);
   WriteLong(0L, tt->file);

   /* Standard apple encoding. */
   end = FileTell(tt->file);
   (void)FileSeek(tt->file, offset+8);
   WriteLong((ULONG)(end-offset), tt->file);
   (void)FileSeek(tt->file, end);
   WriteShort((USHORT)0, tt->file);
   WriteShort((USHORT)(2+2+2+MACSIZE), tt->file);
   WriteShort((USHORT)0, tt->file);
   (void)WriteBytes(ascii2gi, MACSIZE, tt->file);

   /* Long word align the subtable. */
   end = FileTell(tt->file);
   if ((end-offset)%4)
      for (i=0; (short)i<(4-((end-offset)%4)); i++)
	 WriteByte(0, tt->file);


   /* MS delta encoding. */
   end = FileTell(tt->file);
   (void)FileSeek(tt->file, offset+16);
   WriteLong((ULONG)(end-offset), tt->file);
   (void)FileSeek(tt->file, end);

   /* format */
   WriteShort(SEGMENT_MAP, tt->file);
   /* length */
   WriteShort((USHORT)(16+ms.segCount*(2+2+2+2)+ms.giCount*2), tt->file);
   /* version */
   WriteShort(VERSION0, tt->file);
   /* 2*segCount */
   WriteShort((USHORT)(ms.segCount*2), tt->file);
   /* searchRange */
   WriteShort(SearchRange(ms.segCount), tt->file);
   /* entrySelector */
   WriteShort(EntrySelector(ms.segCount), tt->file);
   /* rangeShift */
   WriteShort(RangeShift(ms.segCount), tt->file);

   /* endCount */

   for (i=0; i<ms.segCount; i++)
      WriteShort((USHORT)(ms.endCount[i] | usBias), tt->file);

   WriteShort(PAD0, tt->file);

   /* startCount */
   for (i=0; i<ms.segCount; i++)
      WriteShort((USHORT)(ms.startCount[i] | usBias), tt->file);

   /* idDelta */
   for (i=0; i<ms.segCount; i++)
      WriteShort(PAD0, tt->file);

   /* rangeOffsets */
   for (i=0; i<ms.segCount; i++)
      WriteShort(ms.idOffsets[i], tt->file);

   for (i=0; i<ms.giMax; i++)
      if (ms.gi[i]!=NOTDEFGLYPH)
	 WriteShort(ms.gi[i], tt->file);


   /* Free resources. */
   FreeMSEncoding(&ms);

   return CompleteTable(offset, TBL_CMAP, tt->file);
}




/***** FUNCTIONS */


/***
** Function: TypographicalAscender
**
** Description:
**   Compute the typographical ascender height, as ymax of
**   the letter 'b'.
***/
funit TypographicalAscender(const struct TTHandle *tt)
{
   USHORT i;
   funit height = 0;

   for (i=0; (i<tt->count) && height==0; i++) {
      if (tt->pool[i].code &&
	  !strcmp(LookupCharName(tt->pool[i].code), "b"))
	 height = tt->pool[i].bbox[1].y;
   }

   return height;
}



/***
** Function: TypographicalDescender
**
** Description:
**   Compute the typographical descender height, as ymin of
**   the letter 'g'.
***/
funit TypographicalDescender(const struct TTHandle *tt)
{
   USHORT i;
   funit height = 0;

   for (i=0; i<tt->count && height==0; i++) {
      if (tt->pool[i].code &&
	  !strcmp(LookupCharName(tt->pool[i].code), "g"))
	 height = tt->pool[i].bbox[0].y;
   }

   return height;
}



/***
** Function: WindowsBBox
**
** Description:
**   Compute the bounding box of the characters that are
**   used in Windows character set.
***/


#ifdef NOT_NEEDED_ON_NT


void WindowsBBox(const struct TTHandle *tt, Point *bbox)
{
   USHORT i;
   funit height = 0;

   bbox[0].x = bbox[0].y = SHRT_MAX;
   bbox[1].x = bbox[1].y = SHRT_MIN;
   for (i=0; i<tt->count && height==0; i++) {
      if (tt->pool[i].code && LookupCharCode(tt->pool[i].code,
					     ENC_MSWINDOWS)) {
	 BoundingBox(bbox, tt->pool[i].bbox, (USHORT)2);
      }
   }
}

#endif

/***
** Function: MacBBox
**
** Description:
**   Compute the bounding box of the characters that are
**   used in Mac character set.
**
**   This is currently set to the global bounding box
**   (tt->bbox) of all characters in the font. This will
**   ensure that accents are not sqeezed on Mac platforms.
***/
void MacBBox(const struct TTHandle *tt, Point *bbox)
{
   bbox[0] = tt->bbox[0];
   bbox[1] = tt->bbox[1];
}


void GlobalBBox(const struct TTHandle *tt, Point *bbox)
{
   bbox[0] = tt->bbox[0];
   bbox[1] = tt->bbox[1];
}







/***
** Function: InitTTOutput
**
** Description:
**   This function allocates the resources needed to
**   write a TT font file.
***/
errcode InitTTOutput(const struct TTArg *arg, struct TTHandle **tt)
{
   errcode status = SUCCESS;

   /* Allocate resources. */
   if (((*tt)=Malloc(sizeof(struct TTHandle)))==NULL) {
      SetError(status = NOMEM);
   } else {

      /* Initiate. */
      memset((*tt), '\0', sizeof(**tt));

      /* Open the file. */
      if (((*tt)->file=OpenOutputFile(arg->name))==NULL) {
	 SetError(status = BADOUTPUTFILE);
      } else {

	 /* Allocate space for glyph records. */
	 if (((*tt)->pool
	      = Malloc(sizeof(struct GlyphList)*GLYPHBUF))==NULL) {
	    SetError(status = NOMEM);
	 } else {

	    /* Initiate. */
	    (*tt)->bbox[0].x = (*tt)->bbox[0].y = SHRT_MAX;
	    (*tt)->bbox[1].x = (*tt)->bbox[1].y = SHRT_MIN;
	    (*tt)->count = 0;
	    (*tt)->maxcnt = GLYPHBUF;
	    (*tt)->maxcontours = 0;
	    (*tt)->maxpts = 0;
	    (*tt)->maxcompelements = 0;
	    (*tt)->maxtwilight = 0;

	    /* Write header. */
	    WriteTableHeader((*tt)->file);

	    /* Check error condition. */
	    if (FileError((*tt)->file))
	       status = BADOUTPUTFILE;
	 }
      }
   }

   return status;
}



/***
** Function: FreeTTMetrics
**
** Description:
**   This function free's the resources used to represent
**   TT specific metrics and auxiliary font information.
***/
void FreeTTMetrics(struct TTMetrics *ttm)
{
   if (ttm->verstr)
      Free(ttm->verstr);
   if (ttm->cvt)
      Free(ttm->cvt);
   if (ttm->widths)
      Free(ttm->widths);
   if (ttm->prep)
      Free((UBYTE *)ttm->prep);
}



/***
** Function: CleanUpTT
**
** Description:
**   This function free's the resources used while
**   writing a TT font file.
***/
errcode CleanUpTT(struct TTHandle *tt,
		  const struct TTArg *ttarg,
		  const errcode status)
{
   errcode rc = SUCCESS;

   if (tt) {
      if (tt->file)
	 rc = CloseOutputFile(tt->file);

      /* Nuke the output file? */
      if (status!=SUCCESS || rc!=SUCCESS)
	 RemoveFile(ttarg->name);

      if (tt->pool)
	 Free(tt->pool);
      Free(tt);
   }

   return rc;
}



/***
** Function: FreeTTGlyph
**
** Description:
**   This function will free the memory used to represent a
**   a TrueType glyph.
**
***/
void FreeTTGlyph(struct TTGlyph *glyph)
{
   Outline *path = NULL;

   /* Free the memory. */
   if (glyph) {
      while (glyph->paths) {
	 path = glyph->paths->next;
	 Free(glyph->paths->pts);
	 Free(glyph->paths->onoff);
	 Free(glyph->paths);
	 glyph->paths = path;
      }
      if (glyph->hints)
	 Free(glyph->hints);
      Free(glyph);
   }
}



/***
** Function: PutTTNotDefGlyph
**
** Description:
**   This function adds a record for a the ".notdef" glyph to the
**   'glyf' table of the TT font file.
**
***/
errcode PutTTNotDefGlyph(struct TTHandle *tt, const struct TTGlyph *glyph)
{
   struct TTGlyph ttg;
   long end = FileTell(tt->file);
   errcode status = SUCCESS;
   USHORT oldcount = tt->count;
   Outline *path;
   int conts = 0;
   int size = 0;
   int cnt = 0;


   /* Determine if there is enough room. */
   for (path=glyph->paths; path; path=path->next) {
      cnt += path->count;
      conts += 1;
   }
   size = cnt * sizeof(Point) +     /* coordinates */
	  conts * sizeof(short) +   /* end points */
	  glyph->num +              /* instructions */
	  cnt * sizeof(char) * 2;   /* flag bytes */

   ttg = *glyph;
   if (size > MAXNOTDEFSIZE) {
      ttg.num = 0;
      ttg.stack = 0;
      ttg.twilights = 0;
      ttg.hints = NULL;
      if (size - glyph->num > MAXNOTDEFSIZE) {
	 ttg.paths = NULL;
      }
   }


   /* Move back to glyph #0, i.e. the missing glyph. */
   tt->count = 0;
   (void)FileSeek(tt->file,
						tt->pool[NOTDEFGLYPH].offset+12L+(long)TBLDIRSIZE*NUMTBL);
   status = PutTTGlyph(tt, &ttg, FALSE);
   tt->count = oldcount;
   (void)FileSeek(tt->file, end);

   /* Missing outline? */
   if (ttg.paths==NULL)
      tt->pool[NOTDEFGLYPH].offset = tt->pool[NULLGLYPH].offset;

   return status;
}


/***
** Function: PutTTGlyph
**
** Description:
**   This function adds a record for a simple glyph to the
**   'glyf' table of the TT font file.
**
***/
errcode PutTTGlyph(struct TTHandle *tt, const struct TTGlyph *glyph,
						 const boolean fStdEncoding)
{
   errcode status = SUCCESS;
   UBYTE flag, prev, cnt;
   USHORT i, c, n = 0;
   Outline *path;
   Point bbox[2];
   funit x, y;


   if (glyph!=NULL) {

#ifdef DOT
      /* Replace the '.' character. */
      if (LookupCharCode(glyph->code, ENC_STANDARD)==0x2e) {
         STATIC struct TTGlyph marker;
         STATIC Outline box;
         STATIC ULONG onoff[1];
         STATIC Point pts[4];
         STATIC UBYTE xleading[] = {
	    0x00,
	    0xb9, 0, 3, 0, 0,
	    0x38,    /* SHPIX[], 4, 640 */
	 };


	 marker = *glyph;
	 glyph = &marker;
	 marker.paths = &box;
	 marker.num = sizeof(xleading);
	 marker.hints = xleading;
	 box.next = NULL;
	 box.count = 4;
	 box.onoff = &onoff[0];
	 onoff[0] = 0;
	 box.pts = pts;
	 pts[0].x = 200; pts[0].y = 1400;
	 pts[1].x = 600; pts[1].y = 1400;
	 pts[2].x = 600; pts[2].y = 1800;
	 pts[3].x = 200; pts[3].y = 1800;
      }
#endif

      /* Update maxp */
      if (glyph->num>tt->maxinstructions)
	 tt->maxinstructions = glyph->num;
      if (glyph->stack>tt->maxstack)
	 tt->maxstack = glyph->stack;
      if (glyph->twilights>tt->maxtwilight)
	 tt->maxtwilight = glyph->twilights;

      if (glyph->paths==NULL) {
	 bbox[0].x = bbox[1].x = glyph->lsb;
	 bbox[0].y = bbox[1].y = 0;

	 status=RecordGlyph(tt, glyph->code, bbox,
			    glyph->aw, (USHORT)0, (USHORT)0);
      } else {

	 /* Compute header information. */
	 bbox[0].x = bbox[0].y = SHRT_MAX;
	 bbox[1].x = bbox[1].y = SHRT_MIN;
	 for (c=0, path=glyph->paths; path; path=path->next, c++) {
	    BoundingBox(bbox, path->pts, path->count);
	    n = (USHORT)(n + path->count);
	 }

	 /* Record loca and cmap info. */
	 if ((status=RecordGlyph(tt, glyph->code, bbox,
				 glyph->aw, n, c))==SUCCESS) {

	    /* Write number of contours. */
	    WriteShort(c, tt->file);

	    /* Write bounding box. */
	    if (c) {
	       WriteShort((USHORT)bbox[0].x, tt->file);
	       WriteShort((USHORT)bbox[0].y, tt->file);
	       WriteShort((USHORT)bbox[1].x, tt->file);
	       WriteShort((USHORT)bbox[1].y, tt->file);
	    } else {
	       WriteShort(PAD0, tt->file);
	       WriteShort(PAD0, tt->file);
	       WriteShort(PAD0, tt->file);
	       WriteShort(PAD0, tt->file);
	    }

	    /* Write endPts */
	    for (c=0, path=glyph->paths; path; path=path->next) {
	       c = (USHORT)(c + path->count);
	       WriteShort((short)(c-1), tt->file);
	    }

	    /* Write instruction length. */
	    WriteShort(glyph->num, tt->file);

	    /* Write instruction. */
	    (void)WriteBytes(glyph->hints, glyph->num, tt->file);


	    /* Write the flags. */
	    x=0; y=0;
	    prev = 255;
	    cnt = 0;
	    for (path=glyph->paths; path; path=path->next) {
	       for (i=0; i<path->count; i++) {
		  flag = 0;
		  if (OnCurve(path->onoff, i))
		     flag |= FLG_ONCURVE;

		  if (path->pts[i].x==x) {
		     flag |= FLG_SAMEX;
		  } else if (ABS(path->pts[i].x - x) <= 255) {
		     flag |= FLG_SHORTX;
		     if (path->pts[i].x > x)
			flag |= FLG_SAMEX;
		  }

		  if (path->pts[i].y==y) {
		     flag |= FLG_SAMEY;
		  } else if (ABS(path->pts[i].y - y) <= 255) {
		     flag |= FLG_SHORTY;
		     if (path->pts[i].y > y)
			flag |= FLG_SAMEY;
		  }

		  x = path->pts[i].x;
		  y = path->pts[i].y;
		  if (prev!=255) {
		     if (prev!=flag) {
			if (cnt) {
			   prev |= FLG_REPEAT;
			   WriteByte(prev, tt->file);
			   WriteByte(cnt, tt->file);
			} else {
			   WriteByte(prev, tt->file);
			}
			cnt = 0;
		     } else {
			cnt ++;
		     }
		  }
		  prev = flag;
	       }
	    }
	    if (cnt) {
	       prev |= FLG_REPEAT;
	       WriteByte(prev, tt->file);
	       WriteByte(cnt, tt->file);
	    } else {
	       WriteByte(prev, tt->file);
	    }


	    /* Write the x's */
	    x = 0;
	    for (path=glyph->paths; path; path=path->next) {
	       for (i=0; i<path->count; i++) {
		  if (path->pts[i].x != x) {
		     funit dx = path->pts[i].x - x;
		     if (ABS(dx)<=255) {
			WriteByte((UBYTE)ABS(dx), tt->file);
		     } else {
			WriteShort((USHORT)dx, tt->file);
		     }
		  }
		  x = path->pts[i].x;
	       }
	    }

	    /* Write the y's */
	    y = 0;
	    for (path=glyph->paths; path; path=path->next) {
	       for (i=0; i<path->count; i++) {
		  if (path->pts[i].y != y) {
		     funit dy = path->pts[i].y - y;
		     if (ABS(dy)<=255) {
			WriteByte((UBYTE)ABS(dy), tt->file);
		     } else {
			WriteShort((USHORT)dy, tt->file);
		     }
		  }
		  y = path->pts[i].y;
	       }
	    }


	    /* Word align the glyph entry. */
	    if (FileTell(tt->file) & 1)
	       WriteByte(0, tt->file);

	    /* Poll the file status. */
	    if (FileError(tt->file))
	       status = FAILURE;
	 }
      }


      /* Check for aliases. */
		if (fStdEncoding)
		{
			if (LookupCharCode(glyph->code, ENC_UNICODE)==0x20) {
				struct TTGlyph nobreak;

				nobreak = *glyph;
				nobreak.code = LookupPSName(NULL, 0, "nbspace");
				PutTTGlyph(tt, &nobreak, FALSE);
			}
			if (LookupCharCode(glyph->code, ENC_UNICODE)==0x2d) {
				struct TTGlyph sfthyphen;

				sfthyphen = *glyph;
				sfthyphen.code = LookupPSName(NULL, 0, "sfthyphen");
				PutTTGlyph(tt, &sfthyphen, FALSE);
			}
		}
   }

   return status;
}




/***
** Function: PutTTOther
**
** Description:
**   This function writes the required TT tables to the
**   TT font file, except for the 'glyf' table which is
**   only completed (check sum is computed, etc.).
**
***/
errcode PutTTOther(struct TTHandle *tt, struct TTMetrics *ttm)
{
   long offset = TBLDIRSIZE*NUMTBL+12;
   errcode status = SUCCESS;
   UBYTE ascii2gi[MACSIZE];
   errcode err = SUCCESS;
   short locafmt = 0;
   long csum = 0;


   /*==GLYF===*/
   tt->pool[tt->count].offset = FileTell(tt->file) - offset;
   err = CompleteTable(offset, TBL_GLYF, tt->file);


   /*==CMAP===*/
   if (err==SUCCESS)
      err = PutCMAP(tt, ascii2gi, ttm->Encoding, ttm->encSize);


   /*==LOCA===*/
   if (err==SUCCESS)
      err = PutLOCA(tt->file, tt->pool, tt->count, &locafmt);


   /*==HEAD===*/
   if (err==SUCCESS)
      err = PutHEAD(tt->file, tt->bbox, ttm, locafmt, &csum);


   /*==HHEA===*/
   if (err==SUCCESS)
      err = PutHHEA(tt->file, tt->pool, tt->count,
                    tt->bbox, ttm->macLinegap, ttm);


   /*==HMTX===*/
   if (err==SUCCESS)
      err = PutHMTX(tt->file, tt->pool, tt->count,
                    ttm->widths, ttm->FirstChar, ttm->LastChar,ttm->Encoding);


   /*==OS/2===*/
   if (err==SUCCESS)
      err = PutOS2(tt->file, tt->pool, tt->count, ttm);


   /*==MAXP===*/
   if (err==SUCCESS)
      err = PutMAXP(tt, ttm->maxstorage, ttm->maxprepstack, ttm->maxfpgm);


   /*==Name===*/
   if (err==SUCCESS)
      err = PutNAME(tt->file, ttm);


   /*==POST===*/
   if (err==SUCCESS)
      err = PutPOST(tt->file, tt->pool, tt->count, ttm);

   /*==PREP===*/
   if (err==SUCCESS)
      err = PutPREP(tt->file,
		    ttm->prep, ttm->prep_size);

   /*==FPGM===*/
   if (err==SUCCESS)
      err = PutFPGM(tt->file,
		    ttm->fpgm, ttm->fpgm_size);

   /*==CVT===*/
   if (err==SUCCESS)
      err = PutCVT(tt->file, ttm->cvt, ttm->cvt_cnt);


   /*==GASP==*/
   if (err==SUCCESS)
      err = PutGASP(tt->file, ttm->onepix);


   if (ttm->kerns && (err==SUCCESS))
      err = PutKERN(tt->file, ttm->kerns, ttm->kernsize, tt->pool, tt->count);


   /*=====*/
   /* Compute check sum. */
   if (err==SUCCESS) {
      WriteChecksum(csum, tt->file);
      if (FileError(tt->file))
	 err = BADOUTPUTFILE;
   }


   if (err != SUCCESS)
      SetError(status = err);

   return status;
}




/***
** Function: PutTTComposite
**
** Description:
**
***/
errcode PutTTComposite(struct TTHandle *tt, struct TTComposite *comp)
{
   errcode status;
   Point bbox[2], pts[2];
   USHORT ai=0, bi=0, oi=0;
   USHORT n,c;

   /* Convert the encoding handles to glyph indices. */
   while (ai<tt->count && comp->aenc!=tt->pool[ai].code)
      ai++;
   while (bi<tt->count && comp->benc!=tt->pool[bi].code)
      bi++;
   if (comp->oenc) {
      while (oi<tt->count && comp->oenc!=tt->pool[oi].code)
	 oi++;
   }

   /* Update the bounding box. */
   comp->dx += tt->pool[bi].bbox[0].x - tt->pool[ai].bbox[0].x;
   bbox[0] = tt->pool[bi].bbox[0]; bbox[1] = tt->pool[bi].bbox[1];
   pts[0] = tt->pool[ai].bbox[0]; pts[1] = tt->pool[ai].bbox[1];
   pts[0].x += comp->dx; pts[1].x += comp->dx;
   pts[0].y += comp->dy; pts[1].y += comp->dy;
   BoundingBox(bbox, pts, (USHORT)2);
   bbox[0].x = tt->pool[bi].bbox[0].x; bbox[1].x = tt->pool[bi].bbox[1].x;
   if (comp->oenc)
      BoundingBox(bbox, tt->pool[oi].bbox, (USHORT)2);

   if ((status=RecordGlyph(tt, comp->cenc, bbox,
			   comp->aw, (USHORT)0, (USHORT)0))==FAILURE)
      return status;

   /* Update max composite points/contours/elements. */
   n = (USHORT)(tt->pool[bi].pts + tt->pool[ai].pts);
   c = (USHORT)(tt->pool[bi].conts + tt->pool[ai].conts);
   if (n>tt->maxcomppts)
      tt->maxcomppts = n;
   if (c>tt->maxcompcont)
      tt->maxcompcont = c;
   if (comp->oenc)
      tt->maxcompelements = 3;
   else if (tt->maxcompelements<2)
      tt->maxcompelements = 2;


   /* Write number of contours. */
   WriteShort((USHORT)-1, tt->file);

   /* Write bounding box. */
   WriteShort((USHORT)bbox[0].x, tt->file);
   WriteShort((USHORT)bbox[0].y, tt->file);
   WriteShort((USHORT)bbox[1].x, tt->file);
   WriteShort((USHORT)bbox[1].y, tt->file);

   /* Write flags. */
   WriteShort((USHORT)(MORE_COMPONENTS |
		       ARGS_ARE_XY_VALUES |
		       ROUND_XY_TO_GRID),
	      tt->file);

   /* Write base glyph index. */
   WriteShort(bi, tt->file);
   WriteByte(0, tt->file);
   WriteByte(0, tt->file);

   if (comp->oenc) {
      WriteShort((USHORT)(MORE_COMPONENTS |
			  ARGS_ARE_XY_VALUES |
			  ROUND_XY_TO_GRID),
		 tt->file);
      WriteShort(oi, tt->file);
      WriteByte(0, tt->file);
      WriteByte(0, tt->file);
   }

   WriteShort((USHORT)(ARGS_1_2_ARE_WORDS |
		       ARGS_ARE_XY_VALUES |
		       ROUND_XY_TO_GRID),
	      tt->file);
   WriteShort(ai, tt->file);
   WriteShort((USHORT)comp->dx, tt->file);
   WriteShort((USHORT)comp->dy, tt->file);

   /* Word align the glyph entry. */
   if (FileTell(tt->file) & 1)
      WriteByte(0, tt->file);

   if (FileError(tt->file))
      return FAILURE;
   return SUCCESS;
}




/***
** Function: GetPrep
**
** Description:
**   This function allocates needed space for the
**   pre-program.
**
***/
UBYTE *GetPrep(const int size)
{
   return Malloc((size_t)size);
}


/***
** Function: UsePrep
**
** Description:
**   This function records the pre-program in the
**   TTMetrics record, until an appropriate time
**   when the data can be stored in the TT file.
**
***/
void UsePrep(struct TTMetrics *ttm,
	     const UBYTE *prep,
	     const USHORT prep_size)
{
   ttm->prep = (UBYTE *)prep;
   ttm->prep_size = prep_size;
}

/***
** Function: SetFPGM
**
** Description:
**   This function records the font-program in the
**   TTMetrics record, until an appropriate time
**   when the data can be stored in the TT file.
**
***/
void SetFPGM(struct TTMetrics *ttm,
	     const UBYTE *fpgm,
	     const USHORT fpgm_size,
	     const USHORT num)
{
   ttm->fpgm = fpgm;
   ttm->fpgm_size = fpgm_size;
   ttm->maxfpgm = num;
}
