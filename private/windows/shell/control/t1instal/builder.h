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


#ifndef _ARGS
#  define IN  const
#  define OUT
#  define INOUT
#  define _ARGS(arg) arg
#endif

#define PREPSIZE        1000
#define MAXNOTDEFSIZE   1024


/* Referenced types. */
struct TTArg;
struct TTHandle;


/* Argument types. */
struct TTGlyph {
   struct encoding *code;

   USHORT num;
   USHORT stack;
   USHORT twilights;
   UBYTE *hints;

   Outline *paths;

   funit aw;
   funit lsb;
};

struct TTComposite {
   struct encoding *aenc;
   struct encoding *benc;
   struct encoding *cenc;
   struct encoding *oenc;
   funit dx;
   funit dy;
   funit aw;
   funit lsb;
};

typedef struct {
   ULONG a;
   ULONG b;
} longdate;

struct TTMetrics {
   struct {
      USHORT ver;
      USHORT rev;
   } version;
   longdate created;
   char *family;
   char *copyright;
   char *name;
   char *id;
   char *notice;
   char *fullname;
   char *weight;
   char *verstr;
   f16d16 angle;
   funit underline;
   funit uthick;
   USHORT macStyle;
   USHORT usWeightClass;
   USHORT usWidthClass;
   USHORT fsSelection;

   /* True Typographical metrics. */
   funit typAscender;
   funit typDescender;
   funit typLinegap;
   Point superoff;
   Point supersize;
   Point suboff;
   Point subsize;
   funit strikeoff;
   funit strikesize;
   short isFixPitched; 

   /* Windows based metrics. */
   funit winAscender;
   funit winDescender;
   UBYTE panose[10];

   /* Mac based metrics. */
   funit macLinegap;

   funit emheight;
   USHORT FirstChar;
   USHORT LastChar;
   USHORT DefaultChar;
   USHORT BreakChar;
   USHORT CharSet;
   funit *widths;
   short *cvt;
   USHORT cvt_cnt;
   struct kerning *kerns;
   USHORT kernsize;

   /* Copy of the encoding table. */
   struct encoding *Encoding;
   USHORT encSize;

   /* Hint specific information. */
   const UBYTE *prep;      /* PreProgram. */
   USHORT prep_size;
   const UBYTE *fpgm;      /* FontProgram. */
   USHORT fpgm_size;
   USHORT maxstorage;
   USHORT maxprepstack;    /* Max stack depth in pre-program. */
   USHORT maxfpgm;         /* Max number of function in the font program. */
   USHORT onepix;          /* Treshold where stems become >= 1.0 pixles. */
};


/***
** Function: InitTTOutput
**
** Description:
**   This function allocates the resources needed to
**   write a TT font file.
***/
errcode  InitTTOutput      _ARGS((IN      struct TTArg *,
                                  OUT     struct TTHandle **));

/***
** Function: CleanUpTT
**
** Description:
**   This function free's the resources used while
**   writing a TT font file.
***/
errcode  CleanUpTT         _ARGS((INOUT   struct TTHandle *,
                                  IN      struct TTArg *,
                                  IN      errcode status));

/***
** Function: PutTTNotDefGlyph
**
** Description:
**   This function adds a record for a the ".notdef" glyph to the
**   'glyf' table of the TT font file.
**   
***/
errcode  PutTTNotDefGlyph        _ARGS((INOUT   struct TTHandle *,
                                        IN      struct TTGlyph*));


/** Function: PutTTGlyph
**
** Description:
**   This function adds a record for a simple glyph to the
**   'glyf' table of the TT font file.
**   
***/
errcode  PutTTGlyph        _ARGS((INOUT   struct TTHandle *,
                                  IN      struct TTGlyph*,
											 IN		boolean fStdEncoding));


/***
** Function: PutTTOther
**
** Description:
**   This function writes the required TT tables to the
**   TT font file, except for the 'glyf' table which is
**   only completed (check sum is computed, etc.).
**   
***/
errcode  PutTTOther        _ARGS((INOUT   struct TTHandle *,
                                  INOUT   struct TTMetrics *));

/***
** Function: FreeTTGlyph
**
** Description:
**   This function will free the memory used to represent a 
**   a TrueType glyph.
**   
***/
void     FreeTTGlyph       _ARGS((INOUT   struct TTGlyph *));


/***
** Function: PutTTComposite
**
** Description:
**   
***/
errcode  PutTTComposite    _ARGS((INOUT   struct TTHandle *,
                                  OUT     struct TTComposite *));

/***
** Function: WindowsBBox
**
** Description:
**   Compute the bounding box of the characters that are
**   used in Windows character set.
***/
void     WindowsBBox       _ARGS((IN      struct TTHandle *tt,
                                  OUT     Point *bbox));

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
void     MacBBox           _ARGS((IN      struct TTHandle *tt,
                                  OUT     Point *bbox));


// the name says it

void     GlobalBBox         _ARGS((IN      struct TTHandle *tt,
                                  OUT     Point *bbox));




/***
** Function: TypographicalAscender
**
** Description:
**   Compute the typographical ascender height, as ymax of
**   the letter 'b'.
***/
funit    TypographicalDescender _ARGS((IN struct TTHandle *tt));


/***
** Function: TypographicalDescender
**
** Description:
**   Compute the typographical descender height, as ymin of
**   the letter 'g'.
***/
funit    TypographicalAscender   _ARGS((IN struct TTHandle *tt));


/***
** Function: FreeTTMetrics
**
** Description:
**   This function free's the resources used to represent
**   TT specific metrics and auxiliary font information.
***/
void     FreeTTMetrics     _ARGS((INOUT struct TTMetrics *));


/***
** Function: UsePrep
**
** Description:
**   This function records the pre-program in the
**   TTMetrics record, until an appropriate time
**   when the data can be stored in the TT file.
**   
***/
void     UsePrep           _ARGS((INOUT struct TTMetrics *,
                                  IN    UBYTE *prep,
                                  IN    USHORT size));


/***
** Function: SetFPGM
**
** Description:
**   This function records the font-program in the
**   TTMetrics record, until an appropriate time
**   when the data can be stored in the TT file.
**   
***/
void     SetFPGM           _ARGS((INOUT struct TTMetrics *,
                                  IN    UBYTE *fpgm,
                                  IN    USHORT size,
                                  IN    USHORT num));

/***
** Function: GetPrep
**
** Description:
**   This function allocates needed space for the
**   pre-program.
**   
***/
UBYTE    *GetPrep          _ARGS((IN   int size));
