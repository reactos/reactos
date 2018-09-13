#ifndef METRICS_H
#define METRICS_H



#define ONOFFSIZE(n)	((((n+7)/8)+sizeof(long)-1)/sizeof(long)*sizeof(long))
#define OnCurve(arr, n)  !((arr[(USHORT)(n)>>5]) & (ULONG)(1UL<<((USHORT)(n) % 32UL)))
#define SetOffPoint(arr, n)  arr[(unsigned)(n)/sizeof(arr[0])/8] |= \
                              1UL<<((unsigned)(n) % (sizeof(arr[0])*8))
#define SetOnPoint(arr, n)   arr[(unsigned)(n)/sizeof(arr[0])/8] &= \
                              ~(1UL<<((unsigned)(n) % (sizeof(arr[0])*8UL)))

#define USEMETRICS      0
#define DEFAULTMETRICS  1
#define F8D8            256
#define MAXSNAP         (USHORT)12
#define MAXBLUE         (USHORT)20
#define UNDEF_CVT			0
#define ENDOFPATH			-1
#define NORANGE			-2
#define ARGSIZE                 2000
#define PGMSIZE			3000
#define DEFAULTMATRIX	NULL
#define F16D16BASE		(1L<<19L)
#define F16D16HALF		(1L<<18L)
#define F16D16PPM			524




typedef int funit;
typedef struct {
	funit x;
	funit y;
} Point;

typedef struct Outline {
	struct Outline *next;	 /* Next path of the glyph. */
	USHORT count;		 /* Number of 'pts', 'onoff' and 'map'. */
	Point *pts;		 /* X/Y coordinates. */
	ULONG *onoff;		 /* On/Off curve point bit flags. */
} Outline;

typedef struct StemS {
        struct StemS *next;
        funit offset;
        funit width;
        short i1;
        short i2;
} Stem;

typedef struct Stem3S {
        struct Stem3S *next;
        Stem stem1;
        Stem stem2;
        Stem stem3;
} Stem3;

typedef struct FlexS {
   struct FlexS *next;
   funit civ;
   Point pos;
   Point midpos;
   Point startpos;
   USHORT start;
   USHORT mid;
   USHORT end;
} Flex;

typedef struct {
        Stem *vstems;
        Stem *hstems;
        Stem3 *vstems3;
        Stem3 *hstems3;
   Flex *flex;
} Hints;
        
        
typedef struct Composite {
   struct Composite *next;
   funit asbx;
   funit aw;
   funit adx;
   funit ady;
   const char *achar;
   const char *bchar;
   char *cchar;
   struct encoding *oenc;
} Composite;
   

typedef struct T1Glyph {
   char *name;

   Point lsb;
   Point width;

   Outline *paths;

   Hints hints;

} T1Glyph;
   

typedef struct StemWidth {
   funit width;
   USHORT storage;
} StemWidth;


typedef struct WeightControl {
   StemWidth *vwidths;
   USHORT cnt_vw;
   USHORT max_vw;
   StemWidth *hwidths;
   USHORT cnt_hw;
   USHORT max_hw;
   USHORT storage;
} WeightControl;

struct CVTPos {
   funit y;
   USHORT cvt;
};

typedef struct StemPos {
   struct CVTPos *pos;
   USHORT cnt;
   USHORT blue_cvt;
} StemPos;

typedef struct AlignmentControl {
   StemPos top[MAXBLUE/2];
   StemPos bottom[MAXBLUE/2];
   USHORT cvt;
} AlignmentControl;   

typedef struct Blues {
   funit bluevalues[MAXBLUE];
   USHORT blue_cnt;
   funit otherblues[MAXBLUE];
   USHORT oblue_cnt;
   funit familyblues[MAXBLUE];
   USHORT family_cvt[MAXBLUE/2];
   USHORT fblue_cnt;
   funit familyotherblues[MAXBLUE];
   USHORT familyother_cvt[MAXBLUE/2];
   USHORT foblue_cnt;
   short blueShift;      /*  /BlueShift * F8D8 */
   UBYTE blueFuzz;
   UBYTE blueScale;      /* /BlueScale * 1000 */
   AlignmentControl align;
} Blues;

struct kerning {
	UBYTE left;
	UBYTE right;
	funit delta;
};


struct T1Metrics {
   char *date;
   char *copyright;
   char *name;
   char *id;
   char *notice;
   char *fullname;
   char *weight;
   char *family;
   struct {
      USHORT ver;
      USHORT rev;
   } version;
   f16d16 angle;
   funit avgCharWidth;
   funit underline;
   funit uthick;
   funit stdhw;
   funit stdvw;
   funit defstdhw;
   funit defstdvw;
   funit stemsnaph[MAXSNAP];
   USHORT snaph_cnt;
   funit stemsnapv[MAXSNAP];
   USHORT snapv_cnt;
   UBYTE forcebold;
   UBYTE pitchfam;
   USHORT fixedPitch;
   USHORT flags;
   USHORT tmweight;
   funit ascent;
   funit descent;
   funit intLeading;
   funit extLeading;
   funit superoff;
   funit supersize;
   funit suboff;
   funit subsize;
   funit strikeoff;
   funit strikesize;
   UBYTE firstChar;
   UBYTE lastChar;
   UBYTE DefaultChar;
   UBYTE BreakChar;
   UBYTE CharSet;
   funit *widths;       /* Advance widths. */
   struct kerning *kerns;
   USHORT kernsize;
   WeightControl stems;
   Blues blues;
   funit upem;
   f16d16 *fmatrix;
   UBYTE pgm[PGMSIZE];
   short args[ARGSIZE];
   struct encoding *encoding;
   USHORT encSize;


   Composite *seac;
   Composite *used_seac;
};   



/****** MACROS */
#define GetUPEM(t1m)             (t1m->upem)
#define GetFontMatrix(t1m)       (t1m->fmatrix)
#define GetStdVW(t1m)            (t1m->stdvw)
#define GetStdHW(t1m)            (t1m->stdhw)
#define GetDefStdVW(t1m)         (t1m->defstdvw)
#define GetDefStdHW(t1m)         (t1m->defstdhw)
#define SetDefStdVW(t1m, width)  t1m->defstdvw = width
#define SetDefStdHW(t1m, width)  t1m->defstdhw = width
#define GetCodeStack(t1m)        t1m->pgm
#define GetArgStack(t1m)         t1m->args
#define GetWeight(t1m)           &(t1m->stems)
#define ForceBold(t1m)           t1m->forcebold
#define GetAlignment(t1m)        &(t1m->blues.align)
#define GetBlues(t1m)            &(t1m->blues)
#define CurrentEncoding(t1m)     t1m->encoding
#define EncodingSize(t1m)        t1m->encSize
#define Composites(t1m)          t1m->seac
#define SyntheticOblique(t1m)    (t1m->fmatrix && t1m->fmatrix[2])
#endif
