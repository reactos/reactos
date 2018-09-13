/***
**
**   Module: Hints
**
**   Description:
**      This is a module of the T1 to TT font converter. This is a
**      sub-module of the T1 to TT data translator module. It deals
**      with hints. Any part pf the T1 font that gets translated into
** TrueType instructions is done within this module.
**
**   Author: Michael Jansson
**
**   Created: 8/24/93
**
***/


/**** INCLUDES */
/* General types and definitions. */
#include <limits.h>
#include <string.h>

/* Special types and definitions. */
#include "titott.h"
#include "trig.h"
#include "types.h"
#include "safemem.h"
#include "metrics.h"
#include "t1msg.h"

/* Module dependent types and prototypes. */
#include "trans.h"
#include "hints.h"
#include "ttprog.h"



/***** CONSTANTS */
#define VERSION_SELECTOR 1    /* GetInfo[] selector for version number. */
#define VERSION_1_5     33    /* Version 1.5 of Windows TrueType rasterizer. */
#define STEMSNAPARGS    6     /* Number of args of the CreateStem TTFUN. */

#ifdef SYMETRICAL_REDUCTION
#define MIN_REDUCTION   4     /* Min reduction of the diag. cntrl. */
#endif
#define REDUCTION_C1    10    /* Min reduction, second method. */

#define STACKINC        500   /* Stack increment for arg-stack + prep. */

#define TARGSIZE        100   /* Size of temporary argument stack. */
#define TTFLEXSIZE      9     /* Largest size of a flex, w/o the points. */

#define TMP_TWILIGHTS         2
#define TWILIGHTS_PER_STEM    4

#define LEFTSTEM        1
#define RIGHTSTEM       2

#define SECONDPAIR      2

#define MAXRANGE        15

#define MAXEXTR         60       /* Max num of IP buckets. */

#define UNDEF           -1

#define STDV_CVT  1
#define STDH_CVT  2
#define SNAPV_CVT(v)       (v+3)
#define SNAPH_CVT(t1m, v)  (t1m->snapv_cnt+3+v)


/* External leading hint programs. */
static const UBYTE roman_hints[] = {
   /* Magic cookie. */
   op_pushb1 + 4, 66, 3, 8, 2, 16,
   op_clear,

   op_svcta | SUBOP_Y,
   op_pushb1, 3,

   /* Push 2pnt, in sub-pels. */
   op_mppem,
   op_mps,
   op_div,
   op_pushb1, 128,
   op_mul,

   /* Push InternalLeading, in sub-pels. */
   op_pushb1+1, 2, 1,
   op_md,
   op_sub,

   /* Push MAX(2pnt - i-leading, 0) */
   op_pushb1, 0,
   op_max,

   /* Add the external leading to the Ascent height. */
   op_shpix,
};
static const UBYTE swiss_hints[] = {
   /* Magic cookie. */
   op_pushb1 + 4, 66, 3, 8, 2, 16,
   op_clear,

   op_svcta | SUBOP_Y,
   op_pushb1, 3,

   /* 0<=height<=12.5 */
   op_mps,
   op_pushw1, HIBYTE(800), LOBYTE(800),   /* 12.5 pnt */
   op_gt,
   op_if,

   /* Push 2pnt, in sub-pels. */
   op_mppem,
   op_mps,
   op_div,
   op_pushb1, 128,
   op_mul,

   op_else,

   /* 12.5 < height <= 13.5 */
   op_mps,
   op_pushw1, HIBYTE(864), LOBYTE(864),   /* 13.5 pnt */
   op_gt,
   op_if,

   /* Push 3pnt, in sub-pels. */
   op_mppem, op_pushb1, 1, op_div,
   op_mps,
   op_div,
   op_pushb1, 192,
   op_mul,

   op_else,

   /* Push 4pnt, in sub-pels. */
   op_mppem, op_pushb1, 1, op_div,
   op_mps,
   op_div,
   op_pushw1, HIBYTE(256), /* LOBYTE(256) */ 0,
   op_mul,

   op_eif,

   op_eif,

   /* Push InternalLeading, in sub-pels. */
   op_pushb1+1, 2, 1,
   op_md,
   op_sub,
   op_dup,

   /* Push MAX(?pnt - i-leading, 0) */
   op_pushb1, 0,
   op_max,

   /* Add the external leading to the Ascent height. */
   op_shpix,

};


/* Pre-program. */
static const UBYTE PrepProg[] = {
   op_pushw1, 0x01, 0xff, op_scanctrl,

   op_pushb1, 1, op_rcvt,
   op_pushb1, 128,
   op_lt,
   op_if,

   op_pushb1 + 1, 4, 0, op_scantype, op_scantype,

   op_else,

   op_pushb1 + 1, 5, 1, op_scantype, op_scantype,

   op_eif,
};


/***** LOCAL TYPES */
/* Used for associating points to stems. */
typedef struct {
   short from;
   short to; 
} Range;


/* Zone bucket - Used for grid fitting a stem that may have
been divided into several stem instructions due to hint replacement. */
typedef struct TTStem { 
   funit side1;
   funit side2;
   short rp1;
   short rp2;
   short ref;
   enum aligntype align;
   Range range[MAXRANGE];
   short cnt;
} TTStem;




/***** MACROS */

/* General macros. */
#define Trans3X     TransX
#define TransRX     TransY

#define CLOSETO(v1, v2, eps)   (ABS((v1)-(v2))<=eps)

#define CHECK_ARGSIZE(args, ta, num, asize)   /* Check argument stack. */ \
/*lint -e571 -e644 */if (((ta)+(int)(num))>(asize)) { \
   short *newarg = NULL;\
   if ((newarg = Realloc(args, sizeof(short)*(USHORT)(ta+num+STACKINC)))==NULL) { \
      Free(args); \
      LogError(MSG_ERROR, MSG_NOMEM, NULL);\
      return 0;\
   } else {\
      args = newarg;\
      asize = (short)(ta+num+STACKINC);\
/*line +e571 +e644 */   }\
}
#define CHECK_PREPSIZE(prep, tp, num, psize)   /* Check prep size. */ \
if (((tp)+(num))>(psize)) { \
   UBYTE *newprep = NULL;\
   if ((newprep = Realloc(prep, tp+num+STACKINC))==NULL) { \
      Free(prep); \
      LogError(MSG_ERROR, MSG_NOMEM, NULL);\
      return 0;\
   } else {\
      prep = newprep;\
      psize = (short)(tp+num+STACKINC);\
   }\
}



/***** STATIC FUNCTIONS */



/***
** Function: ConvertFlex
**
** Description:
**   Convert a T1 flex hint into a TrueType IP[] 
**   intruction sequence that will reduce a flex
**   that is flatter than a given height.
***/
static errcode ConvertFlex(const struct T1Metrics *t1m,
                           const Flex *flexRoot,
                           const short *ttpnts,
                           UBYTE *pgm,
                           short *pc,
                           short *args,
                           short *pcd,
                           short *marg)
{
   errcode status = SUCCESS;
   int cis, last_cis = UNDEF;
   char dir, last_dir = 0;
   short targ[TARGSIZE];
   funit height, diff;
   const Flex *flex;
   short ta = 0;
   int num = 0;


   /* Return to the glyph zone. */
   if (flexRoot) {
      pgm[(*pc)++] = op_szps;
      args[(*pcd)++] = 1;
   }

   for (flex=flexRoot; flex; flex=flex->next) {

      /* Points lost in ConvertOutline? */
      if (ttpnts[flex->start]==UNDEF ||
          ttpnts[flex->mid]==UNDEF ||
          ttpnts[flex->end]==UNDEF) {
         LogError(MSG_WARNING, MSG_FLEX, NULL);
         continue;
      }

      /* Vertical or horizontal flex? */
      if (ABS(flex->midpos.x-flex->pos.x) <
          ABS(flex->midpos.y-flex->pos.y)) {
         dir = SUBOP_Y;
         height = TransY(t1m, (funit)(flex->startpos.y - flex->pos.y));
         diff = TransY(t1m, (funit)(flex->midpos.y - flex->startpos.y));
      } else {
         dir = SUBOP_X;
         height = TransX(t1m, (funit)(flex->startpos.x - flex->pos.x));
         diff = TransX(t1m, (funit)(flex->midpos.x - flex->startpos.x));
      }

      /* Skip flex without depth. */
      if (diff==0)
         continue;

      cis = (int)((long)flex->civ * (long)GetUPEM(t1m) / 100 / ABS(diff));

      if (cis!=last_cis || dir!=last_dir ||
          (ta+TTFLEXSIZE+(ttpnts[flex->end]-ttpnts[flex->start]))>=TARGSIZE) {
         if (last_cis!=UNDEF) {
            AssembleArgs(targ, ta, pgm, pc);
            while(num--)
               pgm[(*pc)++] = op_call;
            pgm[(*pc)++] = op_eif;
            ta = 0;
         }
         pgm[(*pc)++] = (UBYTE)(op_svcta | dir);
         pgm[(*pc)++] = op_mppem;
         pgm[(*pc)++] = op_gt;
         pgm[(*pc)++] = op_if;
         args[(*pcd)++] = (short)(cis+1);
         num = 0;
      }

      status = EmitFlex(targ, &ta, height,
                        ttpnts[flex->start],
                        ttpnts[flex->mid],
                        ttpnts[flex->end]);

      last_dir = dir;
      last_cis = cis;
      num++;

      if (status!=SUCCESS) {
         SetError(status = TTSTACK);
         break;
      }
   }
   if (num) {
      AssembleArgs(targ, ta, pgm, pc);
      while(num--)
         pgm[(*pc)++] = op_call;
      pgm[(*pc)++] = op_eif;
   }

   if ((*marg)<2)
      (*marg) = 2;

   return status;
}



/***
** Function: GetSnapV
**
** Description:
**   Return the closest snap width entry.
***/
static short GetSnapV(const struct T1Metrics *t1m, const funit width)
{
   USHORT dist = SHRT_MAX;
   USHORT j = 0;
   USHORT i;

   for (i=0; i<t1m->snapv_cnt; i++) {
      if (ABS(width-t1m->stemsnapv[i])<(short)dist) {
         dist = (USHORT)ABS(width-t1m->stemsnapv[i]);
         j = i;
      }
   }

   if (dist==SHRT_MAX)
      return UNDEF;

   return (short)j;
}





/***
** Function: GetSnapH
**
** Description:
**   Return the closest snap width entry.
***/
static short GetSnapH(const struct T1Metrics *t1m, const funit width)
{
   USHORT dist = SHRT_MAX;
   USHORT j = 0;
   USHORT i;

   for (i=0; i<t1m->snaph_cnt; i++) {
      if (ABS(width-t1m->stemsnaph[i])<(short)dist) {
         dist = (USHORT)ABS(width-t1m->stemsnaph[i]);
         j = i;
      }
   }

   if (dist==SHRT_MAX)
      return UNDEF;

   return (short)j;
}




/***
** Function: PosX
**
** Description:
**   This is a call-back function used by
**   Interpolate.
***/
static funit PosX(const Point pnt)
{
   return pnt.x;
}



/***
** Function: PosY
**
** Description:
**   This is a call-back function used by
**   Interpolate.
***/
static funit PosY(const Point pnt)
{
   return pnt.y;
}



/***
** Function: InRange
**
** Description:
**   This is function determines if a point is
**   within range of a hint zone.
***/
static boolean InRange(const short pnt, const Range *range, const short cnt)
{
   short k;

   for (k=0; k<cnt; k++) {
      if ((range[k].from<=pnt) &&
          (range[k].to>=pnt || range[k].to==ENDOFPATH))
         break;
   }

   return (boolean)(k != cnt);
}


/***
** Function: BoundingStems
**
** Description:
**   Determines what stems are located to the
**   left and to the right of a point on the
**   outline, given its position.
**   
***/
static short BoundingStems(short pnt, const short max_pnt,
                           const funit pos, const TTStem *stems,
                           const short cnt,
                           short *left, short *right)
{
   funit min, max;
   short i;

   max = SHRT_MAX;
   min = 1-SHRT_MAX;
   (*right) = UNDEF;
   (*left) = UNDEF;
   do {
      for (i=0; i<cnt; i++) {
         /* Is stem to the left and defined for the point? */
         if ((stems[i].side1<=pos) &&
             (stems[i].side1>min) &&
             InRange(pnt, stems[i].range, stems[i].cnt)) {
            min = stems[i].side1;
            (*left) = (short)i;
         }

         /* Is stem to the right and defined for the point. */
         if ((stems[i].side2>=pos) &&
             (stems[i].side2<max) &&
             InRange(pnt, stems[i].range, stems[i].cnt)) {
            max = stems[i].side2;
            (*right) = (short)i;
         }
      }

   /* Advance to the next point on the outline if we did not find stems. */
   } while (((*left)==UNDEF) && ((*right)==UNDEF) && (++pnt<(short)max_pnt));

   return pnt;
}




/***
** Function: EndOfRegion
**
** Description:
**   Determine what is the closest point, after the
**   given point, for a new hint replacement.
**   
***/
static short EndOfRegion(const short pnt, const TTStem *stem)
{
   short k;

   for (k=0; k<stem->cnt; k++) {
      if ((stem->range[k].from<=pnt) &&
          (stem->range[k].to>=pnt || stem->range[k].to==ENDOFPATH))
         break;
   }

   return (short)((k==stem->cnt || stem->range[k].to==ENDOFPATH)
                  ? SHRT_MAX : stem->range[k].to);
}




/***
** Function: AddToBucket
**
** Description:
**   This function will add a point, that
**   is located between two stems, into a
**   bucket that represents an interpolation
**   zone.
***/
static short AddToBucket(Extremas *extr,
                         short xcnt,
                         const short pnt,
                         const funit left,
                         const funit right,
                         const TTStem *stems)
{
   short rp1, rp2;
   short tmp, j;

   /* Pick the reference points (which are located in the twilight zone). */
   if (left!=UNDEF)
      rp1 = stems[left].rp2;
   else
      rp1 = UNDEF;
   if (right!=UNDEF)
      rp2 = stems[right].rp1;
   else
      rp2 = UNDEF;

   /* Normalize the reference points. */
   tmp = rp1;
   rp1 = (short)MIN(rp1, rp2);
   rp2 = (short)MAX(tmp, rp2);

   /* Create/Fill IP bucket. */
   for (j=0; j<xcnt; j++) 
      if (extr[j].rp1==rp1 && extr[j].rp2==rp2 && extr[j].n<MAXPTS)
         break;
   if (j==xcnt) {
      if (xcnt<MAXEXTR) {
         extr[xcnt].rp1 = rp1;
         extr[xcnt].rp2 = rp2;
         extr[xcnt].n = 0;
         xcnt++;
      } else {
         LogError(MSG_WARNING, MSG_EXTREME1, NULL);
      }
   }

   /* Add the point to the bucket. */
   if (j<MAXEXTR && extr[j].n<MAXPTS &&
       (extr[j].pts[extr[j].n] = pnt)!=UNDEF)
      extr[j].n++;

   return xcnt;
}


/***
** Function: AddSidePntToBucket
**
** Description:
**   Same as AddToBucket, but the points are
**   known to reside exactly on the side of
**   a stem, and should be controled by one
**   reference point alone. This is only needed
**   for sheared fonts, where controling side
**   point w.r.t. two reference poins leads
**   to problems.
***/
static short AddSidePntToBucket(Extremas *extr,
                                short xcnt,
                                const short pnt,
                                const short rp)
{
   short j;

   /* Create/Fill IP bucket. */
   for (j=0; j<xcnt; j++) 
      if (extr[j].rp1==rp && extr[j].rp2==UNDEF && extr[j].n<MAXPTS)
         break;
   if (j==xcnt) {
      if (xcnt<MAXEXTR) {
         extr[xcnt].rp1 = rp;
         extr[xcnt].rp2 = UNDEF;
         extr[xcnt].n = 0;
         xcnt++;
      } else {
         LogError(MSG_WARNING, MSG_EXTREME1, NULL);
      }
   }

   /* Add the point to the bucket. */
   if (j<MAXEXTR && extr[j].n<MAXPTS &&
       (extr[j].pts[extr[j].n] = pnt)!=UNDEF)
      extr[j].n++;

   return xcnt;
}





/***
** Function: PickSides
**
** Description:
**   Select the position of the left and
**   right side boundry of a point, given
**   the stem to the left and right of the
**   current point on the outline.
***/
static void PickSides(short left, short right,
                      funit *left_side,
                      funit *right_side,
                      TTStem *stems)
{
   if (left!=right) {
      if (left!=UNDEF)
         (*left_side) = stems[left].side2;
      else
         (*left_side) = 1-SHRT_MAX/2;
      if (right!=UNDEF)
         (*right_side) = stems[right].side1;
      else
         (*right_side) = SHRT_MAX/2;
   } else {
      (*left_side) = stems[left].side1;
      (*right_side) = stems[right].side2;
   }
}   





/***
** Function: PickSequence
**
** Description:
**   Determine at what point the current
**   hint sequence is ending.
***/
static short PickSequence(short left, short right, short pnt, TTStem *stems)
{
   short left_end;
   short right_end; 
   short new_seq;

   if (left!=UNDEF && right!=UNDEF) {
      left_end = EndOfRegion(pnt, &stems[left]);
      right_end = EndOfRegion(pnt, &stems[right]);
      new_seq = (short)MIN(left_end, right_end);
   } else if (left!=UNDEF) {
      left_end = EndOfRegion(pnt, &stems[left]);
      new_seq = left_end;
   } else {
      right_end = EndOfRegion(pnt, &stems[right]);
      new_seq = right_end;
   }

   return new_seq;
}



/***
** Function: CollectPoints
**
** Description:
**   This function will go through the points
**   that are local extremas and interpolate
**   them w.r.t. the enclosing stem sides.
**   The non-extreme points are handled with
**   an IUP[] instruction when this is done.
***/
static short CollectPoints(const Outline *orgpaths,
                           const short *ttpnts,
                           TTStem *stems,
                           short cnt,
                           Extremas *extr,
                           funit (*Position)(const Point))
{
   const Outline *path;
   short xcnt = 0;
   short i,tot;
   short prev_stem;
   funit pos;
   short left, right;
   funit left_side, right_side;
   funit max, min;
   short max_pnt, min_pnt;
   short new_seq, n;
   short prev_pnt;
   funit prev_pos;
   short first;
   short pnt = UNDEF;


   tot = 0;
   for (path=orgpaths; path; path=path->next) {
      first = BoundingStems(tot,
                            (short)(tot+(short)path->count),
                            Position(path->pts[0]),
                            stems, cnt, &left, &right);
      if (first==tot+(short)path->count) {
         tot = (short)(tot + path->count);
         continue;
      }

      new_seq = PickSequence(left, right, tot, stems);
      PickSides(left, right, &left_side, &right_side, stems);
      max = 1-SHRT_MAX/2;
      min_pnt = UNDEF;
      max_pnt = UNDEF;
      min = SHRT_MAX/2;
      prev_pnt = FALSE;
      prev_pos = UNDEF;
      prev_stem = UNDEF;
      for (i = (short)(first-tot); i<(short)path->count; i++) {
         if (OnCurve(path->onoff, i)) {
            pos = Position(path->pts[i]);
            n = (short)(i+tot);

            /* Have we crossed over a stem side. */
            if ((prev_stem!=RIGHTSTEM && pos<=left_side && max_pnt!=UNDEF) ||
                (prev_stem!=LEFTSTEM && pos>=right_side && min_pnt!=UNDEF)) {

               if (prev_stem!=RIGHTSTEM && max_pnt!=UNDEF) {
                  pnt = max_pnt;
                  prev_pos = max;

               } else if (prev_stem!=LEFTSTEM && min_pnt!=UNDEF) {
                  pnt = min_pnt;
                  prev_pos = min;
               }

               xcnt = AddToBucket(extr, xcnt, ttpnts[pnt], left, right, stems);

               max = 1-SHRT_MAX/2;
               min = SHRT_MAX/2;
               max_pnt = UNDEF;
               min_pnt = UNDEF;
               prev_pnt = TRUE;
            }

            /* Crossing the side of a stem. */
            if ((pos>=right_side) || (pos<=left_side)) {
               if (pos<left_side)
                  prev_stem = RIGHTSTEM;
               else
                  prev_stem = LEFTSTEM;
            }

            /* Change left/right stem sides? */
            if ((n>new_seq) || (pos>=right_side) || (pos<=left_side)) {
               first = BoundingStems(n,
                                     (short)(path->count+tot),
                                     pos, stems, cnt,
                                     &left, &right);
               if (left==UNDEF && right==UNDEF)
                  break;

               i = (short)(i + first - n);
               new_seq = PickSequence(left, right, n, stems);
               PickSides(left, right, &left_side, &right_side, stems);
               max = 1-SHRT_MAX/2;
               min = SHRT_MAX/2;
               max_pnt = UNDEF;
               min_pnt = UNDEF;
            }

            /* Is the point on the side of the stem? */
            if (CLOSETO(pos,left_side,2) || CLOSETO(pos,right_side,2)) {
               if (!prev_pnt || !CLOSETO(prev_pos, pos, 2)) {
                  if (CLOSETO(pos, right_side, 2) ||
                      CLOSETO(pos, left_side, 2)) {
                     pnt = (short)n;
                     prev_pos = pos;

                  } else if (prev_stem!=RIGHTSTEM && max_pnt!=UNDEF) {
                     pnt = max_pnt;
                     prev_pos = max;
                     max_pnt = UNDEF;

                  } else if (prev_stem!=LEFTSTEM && min_pnt!=UNDEF) {
                     pnt = min_pnt;
                     prev_pos = min;
                     min_pnt = UNDEF;
                  }

                  xcnt = AddToBucket(extr, xcnt, ttpnts[pnt],
                                     left, right, stems);
               }

               prev_pnt = TRUE;
               prev_pos = pos;
            } else {
               prev_pnt = FALSE;

               /* New extremum candidate? */
               if (pos>max) {
                  max = pos;
                  max_pnt = (short)n;
               }
               if (pos<min) {
                  min = pos;
                  min_pnt = (short)n;
               }
            }
         }
      }


      if (left!=UNDEF || right!=UNDEF) {
         if (max_pnt!=UNDEF) {
            xcnt = AddToBucket(extr, xcnt, ttpnts[max_pnt],
                               left, right, stems);
         }
         if (min_pnt!=UNDEF && min!=max) {
            xcnt = AddToBucket(extr, xcnt, ttpnts[min_pnt],
                               left, right, stems);
         }
      }

      tot = (short)(tot + path->count);
   }


   return xcnt;
}



/***
** Function: CollectObliquePoints
**
** Description:
**   This function performs the same task as
**   the "CollectPoint" function, with the
**   exception that the outline is known to
**   be sheared. Some of the logics 
**   is changed, bacause the IUP[] instruction
**   and some IP instruction will not behave
**   the same as in a non-sheared font.
**   This differance applies only to vertical
**   stems (hints resulting in horizontal motion of
**   of points).
***/
static short CollectObliquePoints(const Outline *orgpaths,
                                  const short *ttpnts,
                                  TTStem *stems,
                                  short cnt,
                                  Extremas *extr,
                                  funit (*Position)(const Point))
{
   const Outline *path;
   short xcnt = 0;
   short i,tot;
   short prev_stem;
   funit pos;
   short left, right;
   funit left_side, right_side;
   funit max, min;
   short max_pnt, min_pnt;
   short new_seq, n;
   short first;
   short pnt = UNDEF;


   tot = 0;
   for (path=orgpaths; path; path=path->next) {
      first = BoundingStems(tot,
                            (short)(tot+path->count),
                            Position(path->pts[0]),
                            stems, cnt, &left, &right);
      if (first==tot+(short)path->count) {
         tot = (short)(tot + path->count);
         continue;
      }

      new_seq = PickSequence(left, right, tot, stems);
      PickSides(left, right, &left_side, &right_side, stems);
      max = 1-SHRT_MAX/2;
      min_pnt = UNDEF;
      max_pnt = UNDEF;
      min = SHRT_MAX/2;
      prev_stem = UNDEF;
      for (i = (short)(first-tot); i<(short)path->count; i++) {
         if (OnCurve(path->onoff, i)) {
            pos = Position(path->pts[i]);
            n = (short)(i+tot);

            /* Have we crossed over a stem side. */
            if ((prev_stem!=RIGHTSTEM && pos<=left_side && max_pnt!=UNDEF) ||
                (prev_stem!=LEFTSTEM && pos>=right_side && min_pnt!=UNDEF)) {

               if (prev_stem!=RIGHTSTEM && max_pnt!=UNDEF) {
                  pnt = max_pnt;

               } else if (prev_stem!=LEFTSTEM && min_pnt!=UNDEF) {
                  pnt = min_pnt;
               }

               max = 1-SHRT_MAX/2;
               min = SHRT_MAX/2;
               max_pnt = UNDEF;
               min_pnt = UNDEF;
            }

            /* Crossing the side of a stem. */
            if ((pos>=right_side) || (pos<=left_side)) {
               if (pos<left_side)
                  prev_stem = RIGHTSTEM;
               else
                  prev_stem = LEFTSTEM;
            }

            /* Change left/right stem sides? */
            if ((n>new_seq) || (pos>=right_side) || (pos<=left_side)) {
               first = BoundingStems(n,
                                     (short)(path->count+tot),
                                     pos, stems, cnt,
                                     &left, &right);
               if (left==UNDEF && right==UNDEF)
                  break;

               i = (short)(i + first - n);
               new_seq = PickSequence(left, right, n, stems);
               PickSides(left, right, &left_side, &right_side, stems);
               max = 1-SHRT_MAX/2;
               min = SHRT_MAX/2;
               max_pnt = UNDEF;
               min_pnt = UNDEF;
            }

            /* Is the point on the side of the stem? */
            if (CLOSETO(pos,left_side,2) || CLOSETO(pos,right_side,2)) {
               if (CLOSETO(pos, right_side, 2)) {
                  pnt = (short)n;
                  if (stems[right].side1==right_side)
                     xcnt = AddSidePntToBucket(extr, xcnt, ttpnts[pnt],
                                               stems[right].rp1);
                  else
                     xcnt = AddSidePntToBucket(extr, xcnt, ttpnts[pnt],
                                               stems[right].rp2);

               } else if (CLOSETO(pos, left_side, 2)) {
                  pnt = (short)n;
                  if (stems[left].side1==left_side)
                     xcnt = AddSidePntToBucket(extr, xcnt, ttpnts[pnt],
                                               stems[left].rp1);
                  else
                     xcnt = AddSidePntToBucket(extr, xcnt, ttpnts[pnt],
                                               stems[left].rp2);

               } else if (prev_stem!=RIGHTSTEM && max_pnt!=UNDEF) {
                  pnt = max_pnt;
                  max_pnt = UNDEF;

               } else if (prev_stem!=LEFTSTEM && min_pnt!=UNDEF) {
                  pnt = min_pnt;
                  min_pnt = UNDEF;

               }

            } else {

               /* New extremum candidate? */
               if (pos>max) {
                  max = pos;
                  max_pnt = (short)n;
               }
               if (pos<min) {
                  min = pos;
                  min_pnt = (short)n;
               }
            }
         }
      }


      if (left!=UNDEF || right!=UNDEF) {
         if (max_pnt!=UNDEF) {
         }
         if (min_pnt!=UNDEF && min!=max) {
         }
      }

      tot = (short)(tot + path->count);
   }


   return xcnt;
}



/***
** Function: AddRange
**
** Description:
**   This function adds a point range to
**   a stem bucket.
***/
static void AddRange(TTStem *stem, const short i1, const short i2)
{
   short i;

   /* Check if a prior range can be extended. */
   if (i2!=ENDOFPATH) {
      for (i=0; i<stem->cnt; i++) {
         if (stem->range[i].from == i2+1)
            break;
      }
   } else {
      i = stem->cnt;
   }

   if (i==stem->cnt) {
      if (stem->cnt<MAXRANGE) {
         stem->range[stem->cnt].from = i1;
         stem->range[stem->cnt].to = i2;
         stem->cnt++;
      } else {
         LogError(MSG_WARNING, MSG_REPLC, NULL); 
      }
   } else {
      stem->range[i].from = i1;
   }

}


/***
** Function: CreateStemBuckets
**
** Description:
**   This function will create stem buckets.
**   Several duplicated T1 stem instructions
**   may be mapped to the same bucket.
***/
static short CreateStemBuckets(Stem *stemRoot,
                               Stem3 *stem3Root,
                               TTStem **result)
{
   Stem3 *stem3, *stm3;
   Stem *stem, *stm;
   TTStem *stems = NULL;
   short i, j;
   short cnt;
   short tzpnt = TMPPNT1+1;


   /* Count the stems. */
   cnt = 0;
   (*result) = NULL;
   for (stem3=stem3Root; stem3; stem3=stem3->next) {

      /* Skip obsolete stems. */
      if (stem3->stem1.i2 == NORANGE)
         continue;

      /* Look for a duplicate. */
      for (stm3=stem3Root; stm3!=stem3; stm3=stm3->next) {
         if (stm3->stem1.offset==stem3->stem1.offset &&
             stm3->stem2.offset==stem3->stem2.offset &&
             stm3->stem3.offset==stem3->stem3.offset)
            break;
      }

      /* Count this stem if it is not a duplicate. */
      if (stm3==stem3)
         cnt = (short)(cnt + 3);
   }
   for (stem=stemRoot; stem; stem=stem->next) {

      /* Skip obsolete stems. */
      if (stem->i2 == NORANGE)
         continue;

      /* Look for a duplicate. */
      for (stm=stemRoot; stm!=stem; stm=stm->next) {
         if (stm->offset==stem->offset && stm->width==stem->width)
            break;
      }

      /* Don't count this stem if it is a duplicate. */
      if (stm==stem)
         cnt++;
   }



   /* Initiate them. */
   if (cnt) {
      if ((stems = Malloc(sizeof(TTStem)*(USHORT)cnt))==NULL) {
         errcode status;
         SetError(status=NOMEM);
         return status;
      }

      i = (short)(cnt-1);

      /* Initiate the buckets for the stem3s */
      for (stem3=stem3Root; stem3; stem3=stem3->next) {

         /* Skip obsolete stems. */
         if (stem3->stem1.i2 == NORANGE)
            continue;

         /* Skip if bucket exist for this stem already. */
         for (j=(short)(i+1); j<cnt; j++) {
            if (stems[j].side1==stem3->stem1.offset &&
                stems[j].side2==(stem3->stem1.offset+stem3->stem1.width))
               break;
         }

         if (j==cnt) { 

            /* The rightmost stem is positioned w.r.t. to the middle. */
            stems[i].side1 = stem3->stem1.offset;
            stems[i].side2 = stem3->stem1.width + stem3->stem1.offset;
            stems[i].align = at_relative2;
            stems[i].ref = (short)(i-2);
            stems[i].rp1 = tzpnt++;
            stems[i].rp2 = tzpnt++;
            stems[i].cnt = 1;
            stems[i].range[0].from = stem3->stem1.i1;
            stems[i].range[0].to = stem3->stem1.i2;
            tzpnt+=2;
            i--;

            /* The leftmost stem is positioned w.r.t. to the middle. */
            stems[i].side1 = stem3->stem3.offset;
            stems[i].side2 = stem3->stem3.width + stem3->stem3.offset;
            stems[i].align = at_relative1;
            stems[i].ref = (short)(i-1);
            stems[i].rp1 = tzpnt++;
            stems[i].rp2 = tzpnt++;
            stems[i].cnt = 1;
            stems[i].range[0].from = stem3->stem1.i1;
            stems[i].range[0].to = stem3->stem1.i2;
            tzpnt+=2;
            i--;

            /* The middle stem is centered. */
            stems[i].side1 = stem3->stem2.offset;
            stems[i].side2 = stem3->stem2.width + stem3->stem2.offset;
            stems[i].align = at_centered;
            stems[i].rp1 = tzpnt++;
            stems[i].rp2 = tzpnt++;
            stems[i].cnt = 1;
            stems[i].range[0].from = stem3->stem1.i1;
            stems[i].range[0].to = stem3->stem1.i2;
            tzpnt+=2;
            i--;
         } else {
            AddRange(&stems[j-0], stem3->stem1.i1, stem3->stem1.i2);
            AddRange(&stems[j-1], stem3->stem3.i1, stem3->stem3.i2);
            AddRange(&stems[j-2], stem3->stem2.i1, stem3->stem2.i2);
         }
      }      

      /* Initiate the buckets for the stems. */
      for (stem=stemRoot; stem; stem=stem->next) {

         /* Skip obsolete stems. */
         if (stem->i2 == NORANGE)
            continue;

         /* Skip if bucket exist for this stem already. */
         for (j=(short)(i+1); j<(short)cnt; j++) {
            if (stems[j].side1==stem->offset &&
                stems[j].side2==(stem->offset+stem->width))
               break;
         }

         /* Initiate new bucket:
         Plain vstems and hstems are centered by default. Some
         hstems may be top- or bottom-aligen at a latter point.
         Some stems may be positioned w.r.t. another vstem if
         they overlapp and the RELATIVESTEMS compiler flag is
         turned on. */
         if (j==cnt) {
            stems[i].side1 = stem->offset;
            stems[i].side2 = stem->width + stem->offset;
            stems[i].align = at_centered;
            stems[i].rp1 = tzpnt++;
            stems[i].rp2 = tzpnt++;
            stems[i].cnt = 1;
            stems[i].range[0].from = stem->i1;
            stems[i].range[0].to = stem->i2;
            tzpnt+=2;
            i--;
         } else {
            AddRange(&stems[j], stem->i1, stem->i2);
         }
      }

      /* This happens if two stems are defined for the same
      hint replacement region and the same position, which
      is an Adobe Type 1 font error (broken font). The
      converter will recover by ignoring redundant stems. */
      if (i!=-1) {
         /* LogError(MSG_STEM3); */
         for (j=0; j<=i; j++) {
            stems[j].cnt = 0;
         }
      }
   }

   (*result) = stems;

   return (short)cnt;
}


/***
** Function: ResolveRelativeStem
**
** Description:
**   This function decides if two stems should
**   be aligned side1->side1, side2->side2, 
**   side1->side2 or side2->side1.
**   Stem are positition in relation to each
**   other for two reasons: They overlapp, they
**   are aligned side by side or they are
**   members of a stem3 hint.
***/
static void ResolveRelativeStem(TTStem *ref, TTStem *cur)
{
   /* SIDE1->SIDE2 */
   if (cur->side1==ref->side2) {
      cur->ref = ref->rp2;
      cur->align = at_relative1;


      /* SIDE1->SIDE2 */
   } else if (cur->side2==ref->side1) {
      cur->ref = ref->rp1;
      cur->align = at_relative2;


      /* SIDE1->SIDE1 */
   } else if ((cur->side1>ref->side1) &&
              ((cur->side1-ref->side1+10)>=
               (cur->side2-ref->side2))) {
      cur->ref = ref->rp1;
      cur->align = at_relative1;


      /* SIDE2->SIDE2 */
   } else {
      cur->ref = ref->rp2;
      cur->align = at_relative2;
   }
}



/***
** Function: ConvertVStems
**
** Description:
**   This function translate vstem and vstem3 to TT instructions.
***/
static errcode ConvertVStems(struct T1Metrics *t1m,
                             const Hints *hints,
                             const Outline *orgpaths,
                             const short *ttpnts,
                             UBYTE *pgm,
                             short *pc_ptr,
                             short *args,
                             short *pcd_ptr,
                             USHORT *twilight_ptr)
{
   Extremas extr[MAXEXTR];
   short xcnt = 0;
   errcode status = SUCCESS;
   short pc = *pc_ptr;
   short pcd = *pcd_ptr;
   TTStem *stems = NULL;
   short i;
   short cnt;


   /* Create the buckets. */
   if ((cnt = CreateStemBuckets(hints->vstems,
                                hints->vstems3,
                                &(stems)))==NOMEM) {
      status = NOMEM;
   } else {

      /* Update Max num of twilight points. */
      if ((cnt*TWILIGHTS_PER_STEM+TMP_TWILIGHTS) > (long)(*twilight_ptr))
         (*twilight_ptr) = (USHORT)(cnt * TWILIGHTS_PER_STEM + TMP_TWILIGHTS);

      if (cnt && stems) {

#if RELATIVESTEMS
         /* Do counter- and overlappning stem control? */
         for (i=0; i<cnt; i++) {
            short j;

            if (stems[i].align==at_centered) {
               funit prox = (funit)(ABS(MAX(100,
                                            stems[i].side2 -
                                            stems[i].side1)));
               funit prox2;
               prox2 = (funit)(prox/2);
               for (j=0; j<i; j++) {
                  if (stems[j].cnt &&
                      !((stems[i].side1 - (funit)prox > stems[j].side2) ||
                        (stems[i].side2 + (funit)prox < stems[j].side1)) &&
                      (ABS(stems[i].side2-stems[i].side1-
                           (stems[j].side2-stems[j].side1)) < prox2 ||
                       (short)(stems[i].side1 > stems[j].side2) !=
                       (short)(stems[i].side2 < stems[j].side1)))
                     break;
               }
               if (i!=j) {
                  if (stems[j].side1 < stems[i].side1)
                     stems[i].align = at_relative1;
                  else
                     stems[i].align = at_relative2;
                  stems[i].ref = j;
               }
            }
         }
#endif

         /** Vertical stem hints */
         EmitVerticalStems(pgm, &pc, args, &pcd);

         /* Handle sheared fonts by settin the projection
         vector to the italic angle. The TT instructions for
         the T1 hints can handle any projection vector. */
         if (t1m->fmatrix!=DEFAULTMATRIX && GetFontMatrix(t1m)[2]!=0) {
            Point pt;

            pt.x = 0; pt.y = 1000;
            TransAllPoints(t1m, &pt, (short)1, GetFontMatrix(t1m));
            SetProjection(pgm, &pc, args, &pcd, pt.x, pt.y);
         }

         /* Convert the buckets into instructions. */
         for (i=0; i<cnt; i++) {
            if (stems[i].cnt==0)
               continue;

            /* Resolve relative stems */
            if ((stems[i].align == at_relative1 ||
                 stems[i].align == at_relative2) &&
                stems[i].ref != UNDEF)
               ResolveRelativeStem(&stems[stems[i].ref], &stems[i]);

            /* Emit the instructions. */
            status = EmitVStem(pgm, &pc, args, &pcd, t1m,
                               ABS(stems[i].side2 - stems[i].side1),
                               TransRX(t1m, stems[i].side1),
                               TransRX(t1m, stems[i].side2),
                               Trans3X(t1m, stems[i].side1),
                               Trans3X(t1m, stems[i].side2),
                               (short)MIN(stems[i].rp1, stems[i].rp2),
                               stems[i].align,
                               stems[i].ref);

            if (status!=SUCCESS)
               break;
         }

         /* Collect extremas residing within and between stem sides. */
         if (SyntheticOblique(t1m)) {
            xcnt = CollectObliquePoints(orgpaths, ttpnts,
                                        stems, cnt, extr, PosX);
         } else {
            xcnt = CollectPoints(orgpaths, ttpnts,  stems, cnt,
                                 extr, PosX);
         }

         /* Do the 3% scaling */
         ScaleDown3(extr, xcnt, pgm, &pc, args, &pcd);

         /* Switch over to GLYPHZONE */
         pgm[pc++] = op_szp2;
         args[pcd++] = 1;

         /* Interpolate the local extremas. */
         EmitIP(extr, xcnt, pgm, &pc, args, &pcd, (short)SECONDPAIR);

         /* Interpolate/Shift the rest. */
         pgm[pc++] = op_iup | SUBOP_X;


         /* Free used resources */
         if (stems)
            Free(stems);
      }
   }

   *pc_ptr = pc;
   *pcd_ptr = pcd;

   return status;
}



/***
** Function: ResolveBlueHStem3
**
** Description:
**   This function attemts to resolves a conflict between
**   a hstem3 that has one of its stems in an alignment zone,
**   if there is such a conflict.
***/
static short ResolveBlueHStem3(TTStem *stems,
                               const short cnt,
                               const short k)
{
   short ref = stems[k].ref;
   TTStem tmp;
   short i;

   /* The parent stem of a hstem3 must be first in the 'stems' array,
   i.e. the order of the stems is important.  The children stems may
   therefore have to be swaped with the parten to enforce this condition. */

   if ((stems[k].align==at_relative1 ||
        stems[k].align==at_relative2) &&
       (stems[ref].align!=at_relative1 &&
        stems[ref].align!=at_relative2 &&
        stems[ref].align!=at_side1 &&
        stems[ref].align!=at_side2)) {
      tmp = stems[k];
      stems[k] = stems[ref];
      stems[k].align = at_relative1;
      stems[k].ref = ref;
      stems[ref] = tmp;
      for (i=0; i<cnt; i++) {
         if (i!=k && i!=ref &&
             (stems[i].align==at_relative1 ||
              stems[i].align==at_relative2) &&
             stems[i].ref == ref) {
            stems[i].ref = (short)k;
            if (i<k) {
               tmp = stems[k];
               stems[k] = stems[i];
               stems[i] = tmp;
            }
            break;
         }
      }

   } else {
      ref = k;
   }

   return ref;
}



/***
** Function: ConvertHStems
**
** Description:
**   This function converts hstem and hstem3 T1 instructions.
***/
static errcode ConvertHStems(struct T1Metrics *t1m,
                             const Hints *hints,
                             const Outline *orgpaths,
                             const short *ttpnts,
                             UBYTE *pgm,
                             short *pc_ptr,
                             short *args,
                             short *pcd_ptr,
                             USHORT *twilight_ptr)
{
   Extremas extr[MAXEXTR];
   short xcnt = 0;
   errcode status = SUCCESS;
   short pc = *pc_ptr;
   short pcd = *pcd_ptr;
   TTStem *stems = NULL;
   short i, k;
   short cnt;
   short cvt;

   /* Create the stem buckets. */
   cnt = CreateStemBuckets(hints->hstems, hints->hstems3, &(stems));
   if (cnt==NOMEM)
      return NOMEM;

   /* Update Max num of twilight points. */
   if ((USHORT)(cnt*TWILIGHTS_PER_STEM+TMP_TWILIGHTS) > (*twilight_ptr))
      (*twilight_ptr) = (USHORT)(cnt * TWILIGHTS_PER_STEM + TMP_TWILIGHTS);

#if RELATIVESTEMS
   /* Do counter- and overlappning stem control? */
   for (i=0; i<cnt; i++) {
      short j;

      if (stems[i].align==at_centered) {
         funit prox = (funit)(ABS(MAX(100, stems[i].side2 - stems[i].side1)));
         funit prox2;
         prox2 = (funit)(prox/2);
         for (j=0; j<i; j++) {
            if (stems[j].cnt &&
                !((stems[i].side1 - (funit)prox > stems[j].side2) ||
                  (stems[i].side2 + (funit)prox < stems[j].side1)) &&
                (ABS(stems[i].side2-stems[i].side1-
                     (stems[j].side2-stems[j].side1)) < prox2 ||
                 (short)(stems[i].side1 > stems[j].side2) !=
                 (short)(stems[i].side2 < stems[j].side1)))
               break;
         }
         if (i!=j) {
            if (stems[j].side1 < stems[i].side1)
               stems[i].align = at_relative1;
            else
               stems[i].align = at_relative2;
            stems[i].ref = j;
         }
      }
   }
#endif

   /* Do alignment control. */
   for (i=0; i<cnt; i++) {
      if ((cvt=GetBottomPos(GetBlues(t1m),
                            GetAlignment(t1m),
                            stems[i].side1))!=UNDEF) {
         k = ResolveBlueHStem3(stems, cnt, i);
         stems[k].ref = cvt;
         stems[k].align = at_side1;
      } else if ((cvt=GetTopPos(GetBlues(t1m),
                                GetAlignment(t1m),
                                stems[i].side2))!=UNDEF) {
         k = ResolveBlueHStem3(stems, cnt, i);
         stems[k].ref = cvt;
         stems[k].align = at_side2;
      } 
   }


   if (cnt && stems) {

      /** Horizontal stem hints */
      EmitHorizontalStems(pgm, &pc, args, &pcd);

      /* Convert the buckets into instructions. */
      for (i=0; i<cnt; i++) {

         if (stems[i].cnt==0)
            continue;

         /* Resolve relative stems */
         if ((stems[i].align == at_relative1 ||
              stems[i].align == at_relative2) &&
             stems[i].ref != UNDEF)
            ResolveRelativeStem(&stems[stems[i].ref], &stems[i]);

         /* Emit the instructions. */
         status = EmitHStem(pgm, &pc, args, &pcd, t1m,
                            stems[i].side2 - stems[i].side1,
                            TransY(t1m, stems[i].side1),
                            TransY(t1m, stems[i].side2),
                            (short)MIN(stems[i].rp1, stems[i].rp2),
                            stems[i].align,
                            stems[i].ref);

         if (status!=SUCCESS)
            break;
      }


      /* Interpolate extremas residing within and between stem sides. */
      xcnt = CollectPoints(orgpaths, ttpnts, stems, cnt, extr, PosY);

      /* Switch over to GLYPHZONE */
      pgm[pc++] = op_szp2;
      args[pcd++] = 1;

      /* Interpolate the local extremas. */
      EmitIP(extr, xcnt, pgm, &pc, args, &pcd, (short)0);

      /* Interpoalte/Shift the rest. */
      pgm[pc++] = op_iup | SUBOP_Y;

      /* Free used resources */
      if (stems)
         Free(stems);
   }

   *pcd_ptr = pcd;
   *pc_ptr = pc;

   return status;
}


/***** FUNCTIONS */

/***
** Function: GetRomanHints
**
** Description:
***/
const UBYTE *GetRomanHints(int *size)
{
   (*size) = sizeof(roman_hints);

   return roman_hints;
}


/***
** Function: GetSwissHints
**
** Description:
***/
const UBYTE *GetSwissHints(int *size)
{
   (*size) = sizeof(swiss_hints);

   return swiss_hints;
}


/***
** Function: MatchingFamily
**
** Description:
**   Locate the family alignment zone that is closest to
**   a given alignment zone.
***/
short MatchingFamily(const funit pos,
                     const funit *family,
                     const USHORT fcnt)
{
   funit min_dist = SHRT_MAX;
   short k = UNDEF;
   USHORT j;

   /* Look for the closest family blue. */
   for (j=0; j<fcnt; j+=2) {
      if (ABS(family[j] - pos) < min_dist) {
         k = (short)j;
         min_dist = ABS(family[j] - pos);
      }
   }

   return k;
}




/***
** Function: ConvertHints
**
** Description:
**   This functions converts hstem, hstem3, vstem, vstem3 and flex
**   hints, as well as doing diagonal control.
***/
errcode ConvertHints(struct T1Metrics *t1m,
                     const Hints *hints,
                     const Outline *orgpaths,
                     const Outline *paths,
                     const short *ttpnts,
                     UBYTE **gpgm,
                     USHORT *num,
                     USHORT *stack,
                     USHORT *twilight)
{
   errcode status = SUCCESS;
   UBYTE *pgm = NULL;
   short *args = NULL;
   short pc = 0;
   short pcd = 0;
   short cnt = 0;
   short narg = 0;
   short marg = 0;

   /* Access resources. */
   pgm=GetCodeStack(t1m);
   args=GetArgStack(t1m);


   /* Convert the vertical stem hints. */
   if (status==SUCCESS)
      status = ConvertVStems(t1m, hints, orgpaths, ttpnts,
                             pgm, &pc, args, &pcd, twilight);
   /* Convert the horizontal stem hints. */
   if (status==SUCCESS)
      status = ConvertHStems(t1m, hints, orgpaths, ttpnts,
                             pgm, &pc, args, &pcd, twilight);

   /* Convert flex hints. */
   if (status==SUCCESS)
      status = ConvertFlex(t1m, hints->flex, ttpnts,
                           pgm, &pc, args, &pcd, &marg);

   /********************
   * Adjust diagonals 
   * Do not reduce if dominant vertical stem width is more than 
   * 2.0 pels at 11PPEm and above. This occurs when:
   * 1) StdVW > 187 
   * 2) StdVW < 100 and ForceBold = TRUE
   **/
   if ((ForceBold(t1m)==1 && GetStdVW(t1m)>100 && GetStdVW(t1m)<187) ||
       (ForceBold(t1m)==0 && GetStdVW(t1m)<187))
      narg = ReduceDiagonals(paths, pgm, &pc, args, &pcd);
   if (narg>marg)
      marg = narg;

   if (pc>PGMSIZE) {
      SetError(status = TTSTACK);
   }
   if (pcd>ARGSIZE) {
      SetError(status = ARGSTACK);
   }

   /* Allocate the gpgm */
   (*gpgm) = NULL;
   (*num) = 0;
   (*stack) = 0;
   if (status==SUCCESS) {
      if (pc) {
         if (((*gpgm) = Malloc((USHORT)(pc+pcd*3)))==NULL) {
            SetError(status = NOMEM);
         } else {
            /* Assemble the arguments for the instructions */
            cnt = 0;
            AssembleArgs(args, pcd, (*gpgm), &cnt);
            memcpy(&(*gpgm)[cnt], pgm, (USHORT)pc);
            (*num) = (USHORT)(cnt + pc);
            (*stack) = (USHORT)(pcd + marg);
         }
      }
   }


   return status;
}



/***
** Function: BuildPreProgram
**
** Description:
**   This function builds the pre-program that will compute
**   the CVT and storage entries for the TT stem hint
**   instructions to work. 
***/
USHORT BuildPreProgram(const struct T1Metrics *t1m,
                       const WeightControl *weight,
                       Blues *blues,
                       AlignmentControl *align,
                       UBYTE **glob_prep,
                       const int prepsize,
                       USHORT *maxstack)
{
   UBYTE *prep = (*glob_prep);
   short *args = NULL;
   short ta, tp = 0;
   USHORT i, j;
   long shift;
   funit stdvw, stdhw;
   short cis;
   funit std_width;
   USHORT std_tres;
   funit min_dist;
   short k;
   short argsize = ARGSIZE;
   short psize = (short)prepsize;

   /* Allocate work space. */
   if ((args=Malloc(sizeof(args[0])*(USHORT)argsize))==NULL) {
      LogError(MSG_ERROR, MSG_NOMEM, NULL);
   } else {

      /* Copy the standard pre-program. */
      memcpy(prep, PrepProg, sizeof(PrepProg));
      tp = sizeof(PrepProg);
      (*maxstack) = 0;

      /**********
      * Compute Blue values.
      */

      prep[tp++] = op_pushb1; prep[tp++] = blues->blueScale;
      prep[tp++] = op_mppem;
      prep[tp++] = op_lt;
      prep[tp++] = op_if;
      prep[tp++] = op_pushb1;
      prep[tp++] = ONEPIXEL;
      prep[tp++] = op_smd;
      prep[tp++] = op_pushb1;
      prep[tp++] = TWILIGHT;
      prep[tp++] = op_szps;
      prep[tp++] = op_svcta | SUBOP_Y;
      prep[tp++] = op_rtg;


      /***********************/
      /*** ABOVE BlueScale ***/
      /***********************/

      /* Align the top zones. */
      for (i=0; i<blues->blue_cnt/2; i++) { 
         min_dist = SHRT_MAX;
         k = UNDEF;

         /*** Copy the FamilyBlue entries to the BlueValues if */
         /*** below the Family cut in size.         */
         if (blues->fblue_cnt>0) {

            /* Do the cut in on FamilyBlue/BlueValue. */
            k = MatchingFamily(blues->bluevalues[i*2],
                               blues->familyblues,
                               blues->fblue_cnt);
            min_dist = ABS(blues->bluevalues[i*2] - blues->familyblues[k]);

            /* Always FamilyBlue? */
            if (min_dist) { 
               cis = (short)(GetUPEM(t1m) / TransY(t1m, min_dist));
               tp = (short)FamilyCutIn(prep, (USHORT)tp, cis);
            }

            /* Allocate a cvt if this family has not been used before. */
            if (blues->family_cvt[k/2]==UNDEF_CVT) {
               blues->family_cvt[k/2] = align->cvt;
               align->cvt += 2;
            }

            ta = 2;
            CHECK_ARGSIZE(args, ta, align->top[i].cnt, argsize);
            for (j=0; j<align->top[i].cnt; j++) {
               args[ta++] = (short)align->top[i].pos[j].cvt;
            }
            CHECK_PREPSIZE(prep, tp, 2*ta+10, psize);
            tp = (short)CopyFamilyBlue(prep, tp, args, ta);
            if ((ta+2)>(int)(*maxstack))
               (*maxstack) = (USHORT)(ta+2);

            /* Set up the zone. */
            tp = (short)SetZone(prep, (USHORT)tp,
            (short)(blues->family_cvt[k/2]));

            if (min_dist>0)
               prep[tp++] = op_else;
         }


         /*** Set up the zone. */
         CHECK_PREPSIZE(prep, tp, STACKINC, psize);
         tp = (short)SetZone(prep, (USHORT)tp,
              (short)(align->top[i].blue_cvt));
         if (k!=UNDEF && min_dist) {
            prep[tp++] = op_eif;
         }


         /*** Round and enforce overshoot. */
         ta = 2;
         CHECK_ARGSIZE(args, ta, align->top[i].cnt, argsize);
         for (j=0; j<align->top[i].cnt; j++) {
            if ((align->top[i].pos[j].y -
                 blues->bluevalues[i*2])*F8D8 > blues->blueShift) {
               args[ta++] = (short)align->top[i].pos[j].cvt;
            }
         } 
         if (ta>2) {
            CHECK_PREPSIZE(prep, tp, ta*2+4, psize);
            tp = (short)AlignOvershoot(prep, tp, args, ta);
            if (ta>(short)(*maxstack))
               (*maxstack) = (USHORT)ta;
         }

         ta = 2;
         CHECK_ARGSIZE(args, ta, align->top[i].cnt, argsize);
         for (j=0; j<align->top[i].cnt; j++) {
            if ((align->top[i].pos[j].y -
                 blues->bluevalues[i*2])*F8D8 <= blues->blueShift) {
               args[ta++] = (short)align->top[i].pos[j].cvt;
            }
         } 
         if (ta>2) {
            CHECK_PREPSIZE(prep, tp, ta*2+4, psize);
            tp = (short)AlignFlat(prep, tp, args, ta);
            if (ta>(short)(*maxstack))
               (*maxstack) = (USHORT)(ta+2);
         }
      }



      /* Align the bottom zones. */
      for (i=0; i<blues->oblue_cnt/2; i++) { 
         min_dist = SHRT_MAX;
         k = UNDEF;

         /*** Copy the FamilyBlue entries to the BlueValues if */
         /*** below the Family cut in size.         */
         if (blues->foblue_cnt>0) {

            /* Do the cut in on FamilyBlue/BlueValue. */
            k = MatchingFamily(blues->otherblues[i*2],
                               blues->familyotherblues,
                               blues->foblue_cnt);
            min_dist = ABS(blues->otherblues[i*2] -
                           blues->familyotherblues[k]);

            /* Always FamilyBlue? */
            if (min_dist) { 
               cis = (short)(GetUPEM(t1m) / TransY(t1m, min_dist));
               tp = (short)FamilyCutIn(prep, (USHORT)tp, cis);
            }

            /* Allocate a cvt if this family has not been used before. */
            if (blues->familyother_cvt[k/2]==UNDEF_CVT) {
               blues->familyother_cvt[k/2] = align->cvt++;
            }

            ta = 2;
            CHECK_ARGSIZE(args, ta, align->bottom[i].cnt, argsize);
            for (j=0; j<align->bottom[i].cnt; j++) {
               args[ta++] = (short)align->bottom[i].pos[j].cvt;
            }
            CHECK_PREPSIZE(prep, tp, ta*2+4, psize);
            tp = (short)CopyFamilyBlue(prep, tp, args, ta);
            if (ta>(short)(*maxstack))
               (*maxstack) = (USHORT)ta;


            /* Set up the zone. */
            tp = (short)SetZone(prep, (USHORT)tp,
            (short)blues->familyother_cvt[k/2]);

            if (min_dist>0)
               prep[tp++] = op_else;
         }


         /*** Set up the zone. */
         tp = (short)SetZone(prep, (USHORT)tp,
              (short)align->bottom[i].blue_cvt);
         if (k!=UNDEF && min_dist) {
            prep[tp++] = op_eif;
         }


         /*** Round and enforce overshoot. */
         ta = 2;
         CHECK_ARGSIZE(args, ta, align->bottom[i].cnt, argsize);
         for (j=0; j<align->bottom[i].cnt; j++) {
            if ((align->bottom[i].pos[j].y -
                 blues->otherblues[i*2+1])*F8D8 > blues->blueShift) {
               args[ta++] = (short)align->bottom[i].pos[j].cvt;
            }
         } 
         if (ta>2) {
            CHECK_PREPSIZE(prep, tp, ta*2+4, psize);
            tp = (short)AlignOvershoot(prep, tp, args, ta);
            if (ta>(short)(*maxstack))
               (*maxstack) = (USHORT)ta;
         }

         ta = 2;
         CHECK_ARGSIZE(args, ta, align->bottom[i].cnt, argsize);
         for (j=0; j<align->bottom[i].cnt; j++) {
            if ((align->bottom[i].pos[j].y -
                 blues->otherblues[i*2+1])*F8D8 <= blues->blueShift) {
               args[ta++] = (short)align->bottom[i].pos[j].cvt;
            }
         } 
         if (ta>2) {
            CHECK_PREPSIZE(prep, tp, ta*2+4, psize);
            tp = (short)AlignFlat(prep, tp, args, ta);
            if (ta>(short)(*maxstack))
               (*maxstack) = (USHORT)(ta+2);
         }
      }




      /***********************/
      /*** BELOW BlueScale ***/
      /***********************/
      prep[tp++] = op_else;

      /*** Align the top zones. */

      for (i=0; i<blues->blue_cnt/2; i++) { 

         /* Initiate */
         min_dist = SHRT_MAX;
         k = UNDEF;

         /* switch between blues and family blues. */
         if (blues->fblue_cnt) {

            /* Look for the closest family blue. */
            k = MatchingFamily(blues->bluevalues[i*2],
                               blues->familyblues,
                               blues->fblue_cnt);
            min_dist = ABS(blues->bluevalues[i*2] - blues->familyblues[k]);

            /* Copy/Round the family overshoot position to the zone. */
            if (min_dist) {
               cis = (short)(GetUPEM(t1m) / TransY(t1m, (funit)min_dist));
               tp = (short)FamilyCutIn(prep, (USHORT)tp, cis);
               ta = 2;
               CHECK_ARGSIZE(args, ta, align->top[i].cnt*2, argsize);
               for (j=0; j<align->top[i].cnt; j++) {
                  args[ta++] = (short)(blues->family_cvt[k/2] + 1);
                  args[ta++] = (short)(align->top[i].pos[j].cvt);
               }
               CHECK_PREPSIZE(prep, tp, ta*2+4, psize);
               tp = (short)CopyZone(prep, tp, args, ta);
               if (ta>(short)(*maxstack))
                  (*maxstack) = (USHORT)(ta+2);


               prep[tp++] = op_else;
            }
         }

         /* Copy/Round the blue overshoot position to the zone position. */
         ta = 2;
         CHECK_ARGSIZE(args, ta, align->top[i].cnt*2, argsize);
         for (j=0; j<align->top[i].cnt; j++) {
            args[ta++] = (short)(align->top[i].blue_cvt + 1);
            args[ta++] = (short)(align->top[i].pos[j].cvt);
         }
         CHECK_PREPSIZE(prep, tp, ta*2+4, psize);
         tp = (short)CopyZone(prep, tp, args, ta);
         if (ta>(short)(*maxstack))
            (*maxstack) = (USHORT)ta;

         if (k!=UNDEF && min_dist>0)
            prep[tp++] = op_eif;
      }


      /*** Align the bottom zones. */
      for (i=0; i<blues->oblue_cnt/2; i++) { 

         /* Initiate. */
         min_dist = SHRT_MAX;
         k = UNDEF;

         /* switch between blues and family blues. */
         if (blues->foblue_cnt) {

            /* Look for the closest family blue. */
            k = MatchingFamily(blues->otherblues[i*2],
                               blues->familyotherblues,
                               blues->foblue_cnt);
            min_dist = ABS(blues->otherblues[i*2] -
                           blues->familyotherblues[k]);

            /* Copy/Round the family overshoot position to the zone. */
            if (min_dist) {
               cis = (short)(GetUPEM(t1m) / TransY(t1m, (funit)min_dist));
               tp = (short)FamilyCutIn(prep, (USHORT)tp, cis);
               ta = 2;
               CHECK_ARGSIZE(args, ta, align->bottom[i].cnt*2, argsize);
               for (j=0; j<align->bottom[i].cnt; j++) {
                  args[ta++] = (short)(blues->familyother_cvt[k/2]);
                  args[ta++] = (short)(align->bottom[i].pos[j].cvt);
               }
               CHECK_PREPSIZE(prep, tp, ta*2+4, psize);
               tp = (short)CopyZone(prep, tp, args, ta);
               if (ta>(short)(*maxstack))
                  (*maxstack) = (USHORT)ta;

               prep[tp++] = op_else;
            }
         }

         /* Copy/Round the blue overshoot position to the zone position. */
         ta = 2;
         CHECK_ARGSIZE(args, ta, align->bottom[i].cnt*2, argsize);
         for (j=0; j<align->bottom[i].cnt; j++) {
            args[ta++] = (short)(align->bottom[i].blue_cvt);
            args[ta++] = (short)(align->bottom[i].pos[j].cvt);
         }
         CHECK_PREPSIZE(prep, tp, ta*2+4, psize);
         tp = (short)CopyZone(prep, tp, args, ta);
         if (ta>(short)(*maxstack))
            (*maxstack) = (USHORT)ta;

         if (k!=UNDEF && min_dist>0)
            prep[tp++] = op_eif;
      }


      /* EIF[] MMPEM<BlueScale */
      prep[tp++] = op_eif;


      prep[tp++] = op_pushb1;
      prep[tp++] = 64;
      prep[tp++] = op_smd;


      /**************************************/
      /***      STEM WEIGHT CONTROL       ***/
      /**************************************/

      /****** ForceBold ***/
      if (ForceBold(t1m)) {
         prep[tp++] = op_pushb1+2;
         prep[tp++] = STDV_CVT;
         prep[tp++] = ONEPIXEL*3/4;
         prep[tp++] = STDV_CVT;
         prep[tp++] = op_rcvt;
         prep[tp++] = op_max;
         prep[tp++] = op_wcvtp;
      }


      /******
      * Compute width of horizontal stems. 
      */
      prep[tp++] = op_rtdg;
      prep[tp++] = op_svcta | SUBOP_Y;
      if ((std_width = GetStdHW(t1m))==0)
         std_width = GetDefStdHW(t1m);
      std_width = TransY(t1m, std_width);
      std_tres = (USHORT)(GetUPEM(t1m) * 2 / std_width);
      ta = 0;
      CHECK_ARGSIZE(args, ta, STEMSNAPARGS*weight->cnt_hw, argsize);
      for (i=0; i<weight->cnt_hw; i++) { 
         funit width = TransY(t1m, weight->hwidths[i].width);
         short snap = GetSnapH(t1m, weight->hwidths[i].width);
         USHORT storage = weight->hwidths[i].storage;
         USHORT snap_ci, std_ci;
         short snap_cvt;

         std_ci = CutInSize(width, std_width, std_tres, GetUPEM(t1m));
         if (snap!=UNDEF) {
            snap_ci = CutInSize(width, TransY(t1m, t1m->stemsnaph[snap]),
                                std_tres, GetUPEM(t1m));
            snap_cvt = (short)SNAPH_CVT(t1m, snap);
            ta = (short)SnapStemArgs(args, (USHORT)ta,
                 width, STDH_CVT, (USHORT)snap_cvt,
                 std_ci, snap_ci, storage);
         } else {
            ta = (short)StdStemArgs(args, (USHORT)ta, width, STDH_CVT,
                std_ci, storage);
         }
      } 
      if (ta+2>(short)(*maxstack))   /* Args + loopcnt + fun_num */
         (*maxstack) = (USHORT)(ta+2);
      CHECK_PREPSIZE(prep, tp, ta*2+2, psize);
      AssembleArgs(args, ta, prep, &tp);
      if (t1m->snaph_cnt)
         tp = (short)CreateSnapStems(prep, (USHORT)tp, (short)weight->cnt_hw);
      else
         tp = (short)CreateStdStems(prep, (USHORT)tp,  (short)weight->cnt_hw);


      /******
      * Compute width of vertical stems. 
      */
      prep[tp++] = op_svcta | SUBOP_X;
      if ((std_width = GetStdVW(t1m))==0)
         std_width = GetDefStdVW(t1m);
      std_width = TransX(t1m, std_width);
      std_tres = (USHORT)(GetUPEM(t1m) * 2 / std_width);
      ta = 0;
      CHECK_ARGSIZE(args, ta, STEMSNAPARGS*weight->cnt_vw, argsize);
      for (i=0; i<weight->cnt_vw; i++) { 
         funit width = TransX(t1m, weight->vwidths[i].width);
         short storage = (short)weight->vwidths[i].storage;
         short snap = GetSnapV(t1m, weight->vwidths[i].width);
         USHORT snap_ci, std_ci;
         short snap_cvt;

         std_ci = CutInSize(width, std_width, std_tres, GetUPEM(t1m));
         if (snap!=UNDEF) {
            snap_ci = CutInSize(width, TransX(t1m, t1m->stemsnapv[snap]),
                                std_tres, GetUPEM(t1m));
            snap_cvt = (short)SNAPV_CVT(snap);
            ta = (short)SnapStemArgs(args, (USHORT)ta,
                              width, STDV_CVT, (USHORT)snap_cvt,
                              std_ci, snap_ci, (USHORT)storage);
         } else {
            ta = (short)StdStemArgs(args, (USHORT)ta, width,
                STDV_CVT, std_ci, (USHORT)storage);
         }
      } 
      if (ta+2>(short)(*maxstack))
         (*maxstack) = (USHORT)(ta+2);
      CHECK_PREPSIZE(prep, tp, ta*2+2, psize);
      AssembleArgs(args, ta, prep, &tp);
      if (t1m->snapv_cnt)
         tp = (short)CreateSnapStems(prep, (USHORT)tp, (short)weight->cnt_vw);
      else
         tp = (short)CreateStdStems(prep, (USHORT)tp, (short)weight->cnt_vw);



      prep[tp++] = op_rtg;


      /******
      * Compute diagonal control parameters.
      */
      CHECK_PREPSIZE(prep, tp, STACKINC, psize);
      if ((stdvw = GetStdVW(t1m))==0)
         stdvw = GetDefStdVW(t1m);
      if ((stdhw = GetStdHW(t1m))==0)
         stdhw = GetDefStdHW(t1m);
      if (stdvw && stdhw) {
         cis = (short)(MAX((GetUPEM(t1m) + GetUPEM(t1m)/2) / std_width, 1));
#ifdef SYMETRICAL_REDUCTION
         shift = (long)GetUPEM(t1m);
#else
         shift = (long)GetUPEM(t1m)*(long)MIN(stdvw,stdhw)/
                 (long)MAX(stdvw, stdhw)/2L+(long)GetUPEM(t1m)/2L;
#endif
      } else if (stdvw || stdhw) {
         cis = (short)(1548 / MAX(stdvw, stdhw) + 1);
         shift = (long)GetUPEM(t1m)/2;
      } else {
         cis = 41;
         shift = GetUPEM(t1m)/4;
      }

      prep[tp++] = op_pushb1; prep[tp++] = STORAGE_DIAG;
      prep[tp++] = op_pushb1; prep[tp++] = STDV_CVT;
      prep[tp++] = op_rcvt;
      prep[tp++] = op_pushb1; prep[tp++] = (UBYTE)48;
      prep[tp++] = op_lt;
      prep[tp++] = op_if;

#ifdef SYMETRICAL_REDUCTION
      /* Compute the reduction. */
      shift = (short)(shift/(long)cis/4);
      prep[tp++] = op_npushw;
      prep[tp++] = 2;
      prep[tp++] = (UBYTE)TMPCVT;
      prep[tp++] = 0;
      prep[tp++] = HIBYTE(shift);
      prep[tp++] = LOBYTE(shift);
      prep[tp++] = op_wcvtf;
      prep[tp++] = op_pushb1; prep[tp++] = (UBYTE)TMPCVT;
      prep[tp++] = op_rcvt;
      prep[tp++] = op_pushb1; prep[tp++] = MIN_REDUCTION;
      prep[tp++] = op_add;
#else
      /* Compute the reduction. */
      shift = (short)(shift/(long)cis/2);
      prep[tp++] = op_npushw;
      prep[tp++] = 2;
      prep[tp++] = (UBYTE)TMPCVT;
      prep[tp++] = 0;
      prep[tp++] = HIBYTE(shift);
      prep[tp++] = LOBYTE(shift);
      prep[tp++] = op_wcvtf;
      prep[tp++] = op_pushb1; prep[tp++] = (UBYTE)TMPCVT;
      prep[tp++] = op_rcvt;
      prep[tp++] = op_pushb1; prep[tp++] = REDUCTION_C1;
      prep[tp++] = op_max;
#endif

      prep[tp++] = op_else;
      prep[tp++] = op_pushb1; prep[tp++] = 0;
      prep[tp++] = op_eif;

      prep[tp++] = op_pushb1 + 1;
      prep[tp++] = VERSION_1_5;
      prep[tp++] = VERSION_SELECTOR;
      prep[tp++] = op_getinfo;
      prep[tp++] = op_gt;
      prep[tp++] = op_if;
      prep[tp++] = op_pushb1;
      prep[tp++] = 8;
      prep[tp++] = op_mul;
      prep[tp++] = op_eif;

      prep[tp++] = op_ws;

      Free(args);
   } 

   (*glob_prep) = prep;                             
   return (USHORT)tp;
}




/***
** Function: GetFontProg
**
** Description:
**   Return the font program.
***/
const UBYTE *GetFontProg(void)
{
   return tt_GetFontProg();
}


/***
** Function: GetFontProgSize
**
** Description:
**   Return the size of the font program.
***/
const USHORT GetFontProgSize(void)
{
   return tt_GetFontProgSize();
}


/***
** Function: GetNumFuns
**
** Description:
**   Return the number of functions defined in
**   the font program.
***/
const USHORT GetNumFuns(void)
{
   return tt_GetNumFuns();
}




