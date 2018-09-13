/***
**
**   Module: Trans
**
**   Description:
**      This is a module of the T1 to TT font converter. The module
**      contains functions that will convert T1 specific data into
**      corresponding TT data, such as hints and font metrics.
**
**   Author: Michael Jansson
**
**   Created: 5/28/93
**
***/


/**** INCLUDES */
/* General types and definitions. */
#include <string.h>

/* Special types and definitions. */
#include "titott.h"
#include "types.h"
#include "safemem.h"
#include "trig.h"
#include "metrics.h"
#include "encoding.h"
#include "builder.h"
#include "t1msg.h"

/* Module dependent types and prototypes. */
#include "trans.h"
#include "hints.h"


/***** CONSTANTS */

/* PitchAndFamily pitch values (low 4 bits) */
#define DEFAULT_PITCH       0x00
#define FIXED_PITCH         0x01
#define VARIABLE_PITCH      0x02

/* PitchAndFamily family values (high 4 bits) */
#define FF_DONTCARE         0x00
#define FF_ROMAN            0x10
#define FF_SWISS            0x20
#define FF_MODERN           0x30
#define FF_SCRIPT           0x40
#define FF_DECORATIVE       0x50

#define SWISS_LEADING       0x21
#define ROMAN_LEADING       0x11

#define NOCARE_PANOSE   0
#define NO_PANOSE       1
#define COVE_PANOSE     2
#define TEXT_PANOSE     2
#define DECORATIVE_PANOSE 4
#define SCRIPT_PANOSE   3
#define SANS_PANOSE     11
#define FIXED_PANOSE    9



#define BUFMARG      64
#define CLIMIT    8
#define CLIMIT2      4

#define SUBDIVIDE 0
#define CONVERT      1

#define FW_THIN         100
#define FW_EXTRALIGHT   200
#define FW_LIGHT  300
#define FW_NORMAL 400
#define FW_MEDIUM 500
#define FW_SEMIBOLD     600
#define FW_BOLD         700
#define FW_BLACK  900

#define FWIDTH_ULTRA_CONDENSED   1
#define FWIDTH_EXTRA_CONDENSED   2
#define FWIDTH_CONDENSED         3
#define FWIDTH_SEMI_CONDENSED    4
#define FWIDTH_NORMAL            5
#define FWIDTH_SEMI_EXPANDED     6
#define FWIDTH_EXPANDED          7
#define FWIDTH_EXTRA_EXPANDED    8
#define FWIDTH_ULTRA_EXPANDED    9

#define MAC_ITALIC   0x01
#define MAC_BOLD  0x02

#define FS_ITALIC 0x01
#define FS_BOLD      0x20
#define FS_NORMAL 0x40

#define CVTSIZE     5


/***** LOCAL TYPES */
/* None */


/***** MACROS */
#define ATMSCALE(v)  (((v)*31)/32)
#define IP(v,x1,x2,x1p,x2p)   /*lint -e776 */(short)((long)(v-x1)*(long)(x2p-x1p)/(long)(x2-x1)+(long)x1p)/*lint +e776*/

#define ADDCVT(val)   ttm->cvt[ttm->cvt_cnt++] = (short)(val)

#define SGN(v)   ((v)>0 ? 1 : -1)
#define DIR(v,w)  (char)((ABS((v)-(w))<16) ? 0 : SGN((w) - (v)))


/***** PROTOTYPES */
static USHORT SplitSpline(Point *pts, ULONG *onoff,
                          int i, USHORT length,
                          const funit x0, const funit y0,
                          const funit x1, const funit y1, 
                          const funit x2, const funit y2, 
                          const funit x3, const funit y3,
                          const int delta);


/***** STATIC FUNCTIONS */

/***
** Function: LookupComposite
**
** Description:
**   
***/
static struct encoding *LookupComposite(struct Composite *comp, char *name)
{
   while (comp) {
      if (comp->oenc && !strcmp(LookupCharName(comp->oenc), name))
         return comp->oenc;
      comp=comp->next;
   }

   return NULL;
}



/***
** Function: norm
**
** Description:
**   Normalize an angle so that it falls within the
**   range ~[-pi, pi]
***/
static int norm(int a)
{
   if (a>PI)
      a -= 2*PI;
   if (a<-PI)
      a += 2*PI;

   return ABS(a)/16;
}


/***
** Function: CompareCurves
**
** Description:
**   Make a estimate of the error between a cubic
**   and a quadric curve, given four control points,
**   and suggest an action (sub-division or convertion).
***/
static boolean FASTCALL CompareCurves(const funit x0, const funit y0,
                                      const funit x1, const funit y1, 
                                      const funit x2, const funit y2, 
                                      const funit x3, const funit y3,
                                      const funit nx, const funit ny,
                                      const int delta)
{
   int a, b;



   if ((ABS(ny-y0)>CLIMIT || ABS(nx-x0)>CLIMIT) &&
       (ABS(ny-y3)>CLIMIT || ABS(nx-x3)>CLIMIT)) {

      if (y0!=y1 || x0!=x1)
         a = norm(Atan2(ny-y0, nx-x0) - Atan2(y1-y0, x1-x0))
             * (ABS(ny-y0) + ABS(nx-x0));
      else if (y0!=y2 || x2!=x0)
         a = norm(Atan2(ny-y0, nx-x0) - Atan2(y2-y0, x2-x0))
             * (ABS(ny-y0) + ABS(nx-x0));
      else
         a = 0;

      if (a>=delta)
         return SUBDIVIDE;



      if (y2!=y3 || x2!=x3)
         b = norm(Atan2(y3-ny, x3-nx) - Atan2(y3-y2, x3-x2))
             * (ABS(ny-y0) + ABS(nx-x0));
      else if (y1!=y3 || x1!=x3)
         b = norm(Atan2(y3-ny, x3-nx) - Atan2(y3-y1, x3-x1))
             * (ABS(ny-y0) + ABS(nx-x0));
      else
         b = 0;


      if (b>=delta)    /* angle too big. */
         return SUBDIVIDE;
   }

   return CONVERT;
}



/***
** Function: ConvertSpline
**
** Description:
**   This function adds a spline to the current contour, by first
**   converting it from a cubic to a quadric spline.
***/
static USHORT ConvertSpline(Point *pts, ULONG *onoff,
                            USHORT length, int i,
                            const funit x0, const funit y0,
                            const funit x1, const funit y1, 
                            const funit x2, const funit y2, 
                            const funit x3, const funit y3,
                            const int delta)
{
   funit nx, ny;
   int oi = i;
   USHORT n = 0;

   ny = (funit)(((-y0+y1+y2+((y1+y2)<<1)-y3 + 4002)/4) - (short)1000);
   nx = (funit)(((-x0+x1+x2+((x1+x2)<<1)-x3 + 4002)/4) - (short)1000);
   if (CompareCurves(x0, y0,
                     x1, y1,
                     x2, y2,
                     x3, y3,
                     nx, ny, delta)==SUBDIVIDE) {
      n = SplitSpline(pts, onoff, i, length,
                      x0, y0, x1, y1, x2, y2, x3, y3, delta);
   } else /* CONVERT */ {
      if (i>1 && !OnCurve(onoff, i-2) &&
          (short)(pts[i-1].x >= pts[i-2].x) != (short)(pts[i-1].x >= nx) &&
          (short)(pts[i-1].y >= pts[i-2].y) != (short)(pts[i-1].y >= ny) &&
          (short)(pts[i-1].x >  pts[i-2].x) != (short)(pts[i-1].x > nx) &&
          (short)(pts[i-1].y >  pts[i-2].y) != (short)(pts[i-1].y > ny) &&
          ABS(pts[i-1].x - (nx+pts[i-2].x)/2)<CLIMIT2 && 
          ABS(pts[i-1].y - (ny+pts[i-2].y)/2)<CLIMIT2) {
      }
      SetOffPoint(onoff, i);
      pts[i].y = ny;
      pts[i].x = nx;
      i++;
      SetOnPoint(onoff, i);
      pts[i].y = y3;
      pts[i].x = x3;
      i++;

      n = (USHORT)(i-oi);
   }

   return n;
}



/***
** Function: SplitSpline
**
** Description:
**   This function converts a cubic spline by first
**   creating two new cubic splines, using de Casteljau's
**   algorithm, and then adding the two new splines to the
**   current path.
***/
static USHORT SplitSpline(Point *pts, ULONG *onoff,
                          int i, USHORT length,
                          const funit x0, const funit y0,
                          const funit x1, const funit y1, 
                          const funit x2, const funit y2, 
                          const funit x3, const funit y3,
                          const int delta)
{
   funit xt, yt;
   funit nx1, ny1;
   funit nx2, ny2;
   funit nx3, ny3;
   funit nx4, ny4;
   funit nx5, ny5;
   USHORT cnt;

   xt = (funit)(((x1+x2+8001)/2)-4000);
   yt = (funit)(((y1+y2+8001)/2)-4000);
   nx1 = (funit)(((x0+x1+8001)/2)-4000);
   ny1 = (funit)(((y0+y1+8001)/2)-4000);
   nx2 = (funit)(((nx1+xt+8001)/2)-4000);
   ny2 = (funit)(((ny1+yt+8001)/2)-4000);
   nx5 = (funit)(((x2+x3+8001)/2)-4000);
   ny5 = (funit)(((y2+y3+8001)/2)-4000);
   nx4 = (funit)(((nx5+xt+8001)/2)-4000);
   ny4 = (funit)(((ny5+yt+8001)/2)-4000);
   nx3 = (funit)(((nx2+nx4+8001)/2)-4000);
   ny3 = (funit)(((ny2+ny4+8001)/2)-4000);

   cnt = ConvertSpline(pts, onoff, length, i,
                       x0, y0,
                       (funit)nx1, (funit)ny1,
                       (funit)nx2, (funit)ny2,
                       (funit)nx3, (funit)ny3,
                       delta);
   cnt = (USHORT)(cnt + ConvertSpline(pts, onoff, length, i+cnt,
                                      (funit)nx3, (funit)ny3,
                                      (funit)nx4, (funit)ny4,
                                      (funit)nx5, (funit)ny5,
                                      x3, y3,
                                      delta));

   return cnt;
}




/***
** Function: FreeOutline
**
** Description:
**   This function frees the memory allocated for one 
**   contour.
**   
***/
static void FreeOutline(Outline *path)
{
   Outline *tmp;

   while (path) {
      tmp = path;
      path=path->next;
      Free(tmp->pts);
      Free(tmp->onoff);
      Free(tmp);
   }
}



/***
** Function: ConvertOutline
**
** Description:
**   This function converts an outline by replacing the
**   cubic splines with quadric splines, and by scaling the
**   coordinates to the desired em-height.
**   
***/
static errcode ConvertOutline(const struct T1Metrics *t1m,
                              Outline *src, Outline **dst,
                              const int delta,
                              short *sideboard)
{
   errcode status = SUCCESS;
   f16d16 *fmatrix;
   Outline *path;
   ULONG *onoff = NULL;
   Point *pts = NULL;
   USHORT count;
   USHORT i,j,n;
   USHORT tot = 0;
   USHORT t1tot = 0;


   /* Get the T1 font transformation matrix. */
   fmatrix = GetFontMatrix(t1m);

   while (src) {

      /* Skip paths with less than three points. */
      if (src->count<3) {
         t1tot = (USHORT)(t1tot + src->count);
         src = src->next;
         continue;
      }

      /* Allocate the needed resources */
      count = (USHORT)((src->count+BUFMARG)&~0x0f);
      path = Malloc(sizeof(Outline));
      pts = Malloc(count*sizeof(Point));
      onoff = Malloc(ONOFFSIZE(count));
      if (path==NULL || pts==NULL || onoff==NULL) {
         if (path)
            Free(path);
         if (pts)
            Free(pts);
         if (onoff)
            Free(onoff);
         FreeOutline((*dst));
         (*dst) = NULL;
         SetError(status = NOMEM);
         break;
      }
      memset(onoff, '\0', ONOFFSIZE(count));

      /* Convert the splines. */ /*lint -e771 */
      i=0;
      j=0;
      while (i<src->count) {
         char prev = DIR(src->pts[(i-2+src->count)%src->count].y,
                         src->pts[(i-1+src->count)%src->count].y);
         char this = DIR(src->pts[(i-1+src->count)%src->count].y,
                         src->pts[i].y);

         /* Double the local extremas so that diag-cntrl will work. */
         if (prev && this && prev!=this)
            pts[j++] = src->pts[(i-1+src->count)%src->count];

         if (OnCurve(src->onoff, i)) {
            pts[j++] = src->pts[i++];
         } else {
            /* pts[j] = pts[j-1]; j++; */
            n = ConvertSpline(pts, onoff, count, (int)j,
                              src->pts[i-1].x, src->pts[i-1].y,
                              src->pts[i-0].x, src->pts[i-0].y,
                              src->pts[i+1].x, src->pts[i+1].y,
                              src->pts[i+2].x, src->pts[i+2].y,
                              delta);

            /* Enforce horizontal and vertical tangents. */
            if (OnCurve(onoff, j-1)) {
               if (src->pts[i-1].x==src->pts[i-0].x)
                  pts[j].x = (funit)((pts[j].x + pts[j-1].x)/2);
               if (src->pts[i-1].y==src->pts[i-0].y)
                  pts[j].y = (funit)((pts[j].y + pts[j-1].y)/2);
            }
            if (src->pts[i+1].x==src->pts[i+2].x)
               pts[j+n-2].x = (funit)((pts[j+n-1].x + pts[j+n-2].x)/2);
            if (src->pts[i+1].y==src->pts[i+2].y)
               pts[j+n-2].y = (funit)((pts[j+n-2].y + pts[j+n-1].y)/2);

            j = (USHORT)(j + n);
            i += 3;
         }

         /* Both a line and a curve end with an on-curve point. */
         sideboard[t1tot+i-1] = (short)(j-1+tot);

         /* Extend the pts/onoff arrays. */
         if (j+BUFMARG/2>=count) {
            Point *newpts = NULL;
            ULONG *newonoff = NULL;

            count += BUFMARG;
            newpts = Realloc(pts, count*sizeof(Point));
            newonoff = Realloc(onoff, ONOFFSIZE(count));
            if (newpts==NULL || newonoff==NULL) {
               if (newonoff)
                  Free(newonoff);
               if (newpts)
                  Free(newpts);
               /*lint -e644 */
               if (onoff)
                  Free(onoff);
               if (pts)
                  Free(pts);
               /*lint +e644 */
               FreeOutline((*dst));
               (*dst) = NULL;
               SetError(status=NOMEM);
               break;
            }
            pts = newpts;
            onoff = newonoff;
         }
      }

      if (status!=SUCCESS)
         break;

      /* Scale the points. */
      TransAllPoints(t1m, pts, j, fmatrix);

      t1tot = (USHORT)(t1tot + src->count);
      src = src->next;

      (*dst) = path;
      path->next = NULL;
      path->pts = pts;
      path->onoff = onoff;
      path->count = (USHORT)j;  /*lint +e771 */
      dst = &(path->next);

      tot = (USHORT)(tot + j);
   }

   return status;
}

#ifdef MSDOS
#pragma auto_inline(off)
#endif
static long Mul2(long a, long b, long c, long d)
{
   return a*b+c*d;
}
#ifdef MSDOS
#pragma auto_inline(on)
#endif


/***** FUNCTIONS */

/***
** Function: TransAllPoints
**
** Description:
**   Translate a coordinate according to a transformation matrix.
***/
void FASTCALL TransAllPoints(const struct T1Metrics *t1m,
                             Point *pts,
                             const USHORT cnt,
                             const f16d16 *fmatrix)
{
   if (fmatrix==NULL) {
      register Point *p;
      register int i;

      i = cnt;
      p = pts;
      while (i--) {
         p->x = (funit)((p->x<<1)+(((p->x<<1)+
                                    p->x+(p->x/16)+
                                    8224)/64) - 128);
         p++;
      }
      i = cnt;
      p = pts;
      while (i--) {
         p->y = (funit)((p->y<<1)+(((p->y<<1)+
                                    p->y+
                                    (p->y/16)+
                                    8224)/64) - 128);
         p++;
      }

   } else {
      Point *p;
      int i;
      long u,v;

      i = cnt;
      p = pts;
      while (i--) {
         v = (GetUPEM(t1m) * (Mul2(fmatrix[0], (long)p->x,
                                   fmatrix[2], (long)p->y) +
                              fmatrix[4]) + F16D16HALF) / 524288L;
         u = (GetUPEM(t1m) * (Mul2(fmatrix[1], (long)p->x,
                                   fmatrix[3], (long)p->y) +
                              fmatrix[5]) + F16D16HALF) / 524288L;
         p->x = (funit)v;
         p->y = (funit)u;
         p++;
      }
   }
}



/***
** Function: TransX
**
** Description:
**   Translate a horizontal coordinate according to a transformation matrix.
***/
funit FASTCALL TransX(const struct T1Metrics *t1m, const funit x)
{
   f16d16 *fmatrix = GetFontMatrix(t1m);
   funit pos;

   if (fmatrix) {
      pos = (funit)((GetUPEM(t1m)* ATMSCALE(fmatrix[0] * x) +
                     F16D16HALF) / F16D16BASE);
   } else {
      pos = (funit)(((int)x<<1)-((((int)x+((int)x/64)+8224)/64) - 128));
   }

   return pos;
}


/***
** Function: TransY
**
** Description:
**   Translate a vertical coordinate according to a transformation matrix.
***/
funit FASTCALL TransY(const struct T1Metrics *t1m, const funit y)
{
   f16d16 *fmatrix = GetFontMatrix(t1m);
   funit pos;

   if (fmatrix) {
      pos = (funit)((GetUPEM(t1m)*fmatrix[3] * y +
                     F16D16HALF) / F16D16BASE);
   } else {
      pos = (funit)(((int)y<<1)+((((int)y<<1)+
                                  (int)y+
                                  ((int)y/16)+
                                  8224)/64) - 128);
   }

   return pos;
}


/***
** Function: ConvertGlyph
**
** Description:
**   This function convertes the data associated to a T1 font glyph
**   into the corresponding data used in a TT font glyph.
***/
errcode FASTCALL ConvertGlyph(struct T1Metrics *t1m,
                              const struct T1Glyph *t1glyph,
                              struct TTGlyph **ttglyph,
                              const int delta)
{
   errcode status = SUCCESS;
   struct encoding *code;

   if ((code = LookupPSName(CurrentEncoding(t1m),
                            EncodingSize(t1m),
                            t1glyph->name))==NULL &&
       (code = LookupComposite(Composites(t1m), t1glyph->name))==NULL &&
       strcmp(t1glyph->name, ".notdef")) {
      LogError(MSG_INFO, MSG_BADENC, t1glyph->name);
      status = SUCCESS;
   } else {

      if (((*ttglyph) = Malloc(sizeof(struct TTGlyph)))==NULL) {
         SetError(status = NOMEM);
      } else {
         short *sideboard = NULL;
         Outline *path;
         USHORT tot;

         memset((*ttglyph), '\0', sizeof(struct TTGlyph));
         if (t1glyph->width.y!=0) {
            LogError(MSG_WARNING, MSG_BADAW, NULL);
         }
         (*ttglyph)->aw = TransY(t1m, t1glyph->width.x);
         (*ttglyph)->lsb = TransY(t1m, t1glyph->lsb.x);
         (*ttglyph)->code = code;
         (*ttglyph)->num = 0;
         (*ttglyph)->twilights = 0;

         /* Initiate the side board. */
         for (path=t1glyph->paths, tot=0; path; path=path->next)
            tot = (USHORT)(tot + path->count);
         if (tot && (sideboard = Malloc((unsigned)tot*sizeof(short)))==NULL) {
            SetError(status=NOMEM);
         } else if ((status = ConvertOutline(t1m, t1glyph->paths,
                                             &((*ttglyph)->paths),
                                             delta,
                                             sideboard))==SUCCESS)
            status = ConvertHints(t1m,
                                  &t1glyph->hints,
                                  t1glyph->paths,
                                  (*ttglyph)->paths,
                                  sideboard,
                                  &(*ttglyph)->hints,
                                  &(*ttglyph)->num,
                                  &(*ttglyph)->stack,
                                  &(*ttglyph)->twilights);

         if (sideboard)
            Free(sideboard);


         /* Pick default std widths. */
         if (t1glyph->name[0]=='l' && t1glyph->name[1]=='\0') {
            if (GetStdVW(t1m)==0 && t1glyph->hints.vstems)
               SetDefStdVW(t1m, t1glyph->hints.vstems->width);
         }
         if (t1glyph->name[0]=='z' && t1glyph->name[1]=='\0') {
            if (GetStdHW(t1m)==0) {
               if (t1glyph->hints.hstems && t1glyph->hints.hstems->width)
                  SetDefStdHW(t1m, t1glyph->hints.hstems->width);
               else if (t1glyph->hints.vstems && t1glyph->hints.vstems->width)
                  SetDefStdHW(t1m, t1glyph->hints.vstems->width);
            }
         }
      }
   } 

   return status;
}


/***
** Function: ConvertComposite
**
** Description:
**   This function convertes the data associated to a T1 font seac glyph
**   into the corresponding data used in a TT font composite glyph.
**
***/
errcode FASTCALL ConvertComposite(struct T1Metrics *t1m,
                                  const struct Composite *comp,
                                  struct TTComposite *ttcomp)
{
   Point pt;

   pt.x = comp->adx;
   pt.y = comp->ady;
   TransAllPoints(t1m, &pt, 1, GetFontMatrix(t1m));
   ttcomp->dx = pt.x + (pt.x - TransX(t1m, comp->adx));
   ttcomp->dy = pt.y;
   ttcomp->aw = TransY(t1m, comp->aw);
   ttcomp->lsb = TransY(t1m, comp->asbx);
   ttcomp->aenc = LookupPSName(CurrentEncoding(t1m),
                               EncodingSize(t1m), comp->achar);
   ttcomp->benc = LookupPSName(CurrentEncoding(t1m),
                               EncodingSize(t1m), comp->bchar);
   if ((ttcomp->cenc = LookupPSName(CurrentEncoding(t1m),
                                    EncodingSize(t1m), comp->cchar))==NULL) {
      LogError(MSG_INFO, MSG_BADENC, comp->cchar);
   }
   ttcomp->oenc = comp->oenc;

   if (ttcomp->aenc && ttcomp->benc)
      return SUCCESS;
   return SKIP;
}





/***
** Function: ConvertMetrics
**
** Description:
**
***/
errcode FASTCALL ConvertMetrics(const struct TTHandle *tt,
                                struct T1Metrics *t1m,
                                struct TTMetrics *ttm,
                                const char *tag)
{
   const AlignmentControl *align;
   const Blues *blues;
   USHORT prep_size;
   UBYTE *prep = NULL;
   errcode status = SUCCESS;
   Point bbox[2];
   funit em;
   funit PostAsc;
   USHORT i, j;


   ttm->Encoding = CurrentEncoding(t1m);
   ttm->encSize = EncodingSize(t1m);
   ttm->version.ver = t1m->version.ver;
   ttm->version.rev = t1m->version.rev;

   if ((ttm->verstr = Malloc(strlen(tag)+4+1+4+1))==NULL) {
      SetError(status = NOMEM);
   } else {
      strcpy(ttm->verstr, tag);
      (void)_itoa((int)ttm->version.ver, &ttm->verstr[strlen(ttm->verstr)], 4);
      strcat(ttm->verstr, ".");
      (void)_itoa((int)ttm->version.rev, &ttm->verstr[strlen(ttm->verstr)], 4);
      ttm->created.a = 0;
      ttm->created.b = 0;
      ttm->family = t1m->family;
      ttm->copyright = t1m->copyright;
      ttm->name = t1m->name;
      ttm->id = t1m->id;
      ttm->notice = t1m->notice;
      ttm->fullname = t1m->fullname;
      ttm->weight = t1m->weight;
      ttm->angle = t1m->angle;
      ttm->underline = TransY(t1m, t1m->underline);
      ttm->uthick = TransY(t1m, t1m->uthick);
      ttm->usWidthClass = (USHORT)(strstr(t1m->fullname, "Ultra-condensed")
                                   ? FWIDTH_ULTRA_CONDENSED :
         ((strstr(t1m->fullname, "Extra-condensed") ? FWIDTH_EXTRA_CONDENSED :
            ((strstr(t1m->fullname, "Condensed") ? FWIDTH_CONDENSED :
               ((strstr(t1m->fullname, "Semi-condensed") ? FWIDTH_SEMI_CONDENSED :
                  ((strstr(t1m->fullname, "Semi-expanded")
                    ? FWIDTH_SEMI_EXPANDED :
                     ((strstr(t1m->fullname, "Expanded")
                       ? FWIDTH_EXPANDED :
                        ((strstr(t1m->fullname, "Extra-expanded")
                          ? FWIDTH_EXTRA_EXPANDED :
                           ((strstr(t1m->fullname, "Ultra-expanded")
                             ? FWIDTH_ULTRA_EXPANDED :
                              FWIDTH_NORMAL)))))))))))))));


      /* Window based metrics. */

      // ps driver does not compute asc and desc based on the
      // windows charset. So, we will not do it either. We will
      // also use the all glyhs supported in the font.
      // Ps driver acutally trusts the values found in .pfm file.
      // These values, according to afm->pfm converter code, are computed
      // over all glyphs. However, some vendors ship buggy pfm's with
      // zero ascenders or negative descenders. If we took these values
      // literally, as ps driver does, the true type driver would
      // shave off portions of glyphs and the conversion would appear broken.
      // Pcl printing and screen output would be totally broken.
      // Turns out that for these buggy fonts ATM on win31 also
      // corrects the value from .pfm files for screen and pcl printer.
      // [bodind]


      // total bbox: [bodind], replaced WindowsBBox function:

      GlobalBBox(tt, bbox);

      ttm->winAscender = ABS(bbox[1].y);
      ttm->winDescender = ABS(bbox[0].y);


      ttm->panose[0] = NOCARE_PANOSE;
      ttm->panose[1] = NOCARE_PANOSE;
      ttm->panose[2] = NOCARE_PANOSE;
      ttm->panose[3] = NOCARE_PANOSE;
      ttm->panose[4] = NOCARE_PANOSE;
      ttm->panose[5] = NOCARE_PANOSE;
      ttm->panose[6] = NOCARE_PANOSE;
      ttm->panose[6] = NOCARE_PANOSE;
      ttm->panose[7] = NOCARE_PANOSE;
      ttm->panose[8] = NOCARE_PANOSE;
      ttm->panose[9] = NOCARE_PANOSE;
      /* Fixed pitch fonts are not given a panose by ATM. */
      if (!(t1m->fixedPitch)) {
         switch (t1m->pitchfam & 0xf0) {
            case FF_DECORATIVE:
               ttm->panose[0] = (UBYTE)DECORATIVE_PANOSE;
               ttm->panose[1] = (UBYTE)NO_PANOSE;
               break;
            case FF_ROMAN:
               ttm->panose[0] = (UBYTE)TEXT_PANOSE;
               ttm->panose[1] = (UBYTE)COVE_PANOSE;
               break;
            case FF_SWISS:
               ttm->panose[0] = (UBYTE)TEXT_PANOSE;
               ttm->panose[1] = (UBYTE)SANS_PANOSE;
               break;
            case FF_SCRIPT:
               ttm->panose[0] = (UBYTE)SCRIPT_PANOSE;
               ttm->panose[1] = (UBYTE)SANS_PANOSE;
               break;
            case FF_MODERN:
               ttm->panose[0] = (UBYTE)TEXT_PANOSE;
               ttm->panose[1] = (UBYTE)SANS_PANOSE;
               break;
         }
      } 
      ttm->isFixPitched = t1m->fixedPitch;
      ttm->panose[2] = (UBYTE)((t1m->tmweight - 500) * 12 / 900 + 6);

      /* Mac based metrics. */
      MacBBox(tt, bbox);
      ttm->macLinegap = TransY(t1m, (funit)(t1m->extLeading +
                                            (ttm->winAscender +
                                             ttm->winDescender) -
                                            (bbox[1].y-bbox[0].y)));

      /* Typographical metrics. */
      ttm->emheight = GetUPEM(t1m);
      if (t1m->flags==DEFAULTMETRICS) {
         ttm->usWeightClass = (USHORT)(strstr(t1m->fullname, "Thin") ? FW_THIN :
            ((strstr(t1m->fullname, "light") ? FW_EXTRALIGHT :
               ((strstr(t1m->fullname, "Light") ? FW_LIGHT :
                  ((strstr(t1m->fullname, "Medium") ? FW_MEDIUM :
                     ((strstr(t1m->fullname, "emi-bold") ? FW_SEMIBOLD :
                        ((strstr(t1m->fullname, "Bold") ? FW_BOLD :
                           ((strstr(t1m->fullname, "Black") ? FW_BLACK :
                              FW_NORMAL)))))))))))));
         ttm->macStyle = (USHORT)(((ttm->usWeightClass>FW_MEDIUM)?MAC_BOLD : 0) |
                         ((ttm->angle != 0) ? MAC_ITALIC : 0));
         ttm->fsSelection = (USHORT)(((ttm->angle != 0) ? FS_ITALIC : 0) |
                            ((ttm->usWeightClass > FW_MEDIUM) ? FS_BOLD : 0) |
                            ((ttm->usWeightClass==FW_NORMAL)
                                     ? FS_NORMAL : 0));
         ttm->typAscender = TypographicalAscender(tt);
         ttm->typDescender = TypographicalDescender(tt);
         em = ttm->typAscender - ttm->typDescender;
         ttm->superoff.y = (funit)(em / 2);
         ttm->superoff.x = 0;
         ttm->supersize.y = (funit)(em * 2 / 3);
         ttm->supersize.x = (funit)(em * 3 / 4);
         ttm->suboff.y = (funit)(em / 5);
         ttm->suboff.x = 0;
         ttm->subsize.y = (funit)(em * 2 / 3);
         ttm->subsize.x = (funit)(em * 3 / 4);
         ttm->strikeoff = (funit)(ttm->typAscender / 2);
         ttm->strikesize = (funit)(ttm->typAscender / 10);
      } else {
         ttm->usWeightClass = t1m->tmweight;
         ttm->macStyle = (USHORT)(((t1m->tmweight>FW_MEDIUM)?MAC_BOLD : 0) |
                         ((ttm->angle != 0) ? MAC_ITALIC : 0));
         ttm->fsSelection = (USHORT)(((ttm->angle != 0) ? FS_ITALIC : 0) |
                            ((ttm->usWeightClass > FW_MEDIUM) ? FS_BOLD : 0) |
                            ((ttm->usWeightClass==FW_NORMAL)
                                     ? FS_NORMAL : 0));
         ttm->typAscender = TransY(t1m, (funit)(t1m->ascent -
                                                t1m->intLeading));
         ttm->typDescender = (funit)(-TransY(t1m, t1m->descent)-1);
         ttm->typLinegap = TransY(t1m, (funit)(t1m->intLeading +
                                               t1m->extLeading));
         ttm->superoff.y = ABS(TransY(t1m, t1m->superoff));
         ttm->superoff.x = 0;
         ttm->supersize.y = TransY(t1m, t1m->supersize);
         ttm->supersize.x = (funit)(TransY(t1m, t1m->supersize) * 3 / 4);
         ttm->suboff.y = ABS(TransY(t1m, t1m->suboff));
         ttm->suboff.x = 0;
         ttm->subsize.y = TransY(t1m, t1m->subsize);
         ttm->subsize.x = (funit)(TransY(t1m, t1m->subsize) * 3 / 4);
         ttm->strikeoff = ABS(TransY(t1m, t1m->strikeoff));
         ttm->strikesize = TransY(t1m, t1m->strikesize);

         // Adjust usWinAscent so that internal leading matches up.
         // For fonts that do not have buggy pfm files, this adjustment
         // will do nothing, for those for which intLeading is
         // incorrectly set to zero, taking max means that the tops will not
         // be chopped off in the converted font. ttfd shaves off anything
         // that extends beyond ascender or descender. For fonts with buggy
         // pfm's, tt conversions may have bogus internal leadings, but this
         // is better than having glyph bottoms or tops shaved off. [bodind]

         PostAsc = ttm->emheight + TransY(t1m, t1m->intLeading) - ttm->winDescender;

         if (PostAsc > ttm->winAscender)
            ttm->winAscender  = PostAsc;
      }

      /* Gray-scale threshold. */
      if (GetStdVW(t1m)!=0 || GetDefStdVW(t1m)!=0) {
         ttm->onepix = (USHORT)(1 + GetUPEM(t1m)*3/2 /
                                TransY(t1m, ((GetStdVW(t1m) ?
                                              GetStdVW(t1m) :
                                              GetDefStdVW(t1m)))));
      }

      // needed in producing the correct ifimetrics for tt conversion

      ttm->DefaultChar = t1m->DefaultChar;
      ttm->BreakChar   = t1m->BreakChar;
      ttm->CharSet     = t1m->CharSet;  // essential for correct font mapping

      /* Character widths. */
      if (t1m->flags!=DEFAULTMETRICS) {
         ttm->FirstChar   = t1m->firstChar;
         ttm->LastChar    = t1m->lastChar;
         if ((ttm->widths = Malloc(sizeof(funit)*
                                   (t1m->lastChar-t1m->firstChar+1)))==NULL) {
            SetError(status = NOMEM);
         } else {
            for (i=0; i<=(unsigned)(t1m->lastChar-t1m->firstChar); i++) {
               ttm->widths[i] = TransY(t1m, t1m->widths[i]);
            }
         }
      }

      /* Pair kerning. */
      if (t1m->flags!=DEFAULTMETRICS &&
          t1m->kerns!=NULL) {
         if ((ttm->kerns = Malloc(sizeof(struct kerning)*
                                  t1m->kernsize))==NULL) {
            SetError(status = NOMEM);
         } else {
            for (i=0; i<t1m->kernsize; i++) {
               ttm->kerns[i].left = t1m->kerns[i].left;
               ttm->kerns[i].right = t1m->kerns[i].right;
               ttm->kerns[i].delta = TransY(t1m, t1m->kerns[i].delta);
            }
            ttm->kernsize = t1m->kernsize;
         }
      }

      /* Pre program. */
      if ((prep = GetPrep(PREPSIZE))!=NULL &&
          (prep_size = BuildPreProgram(t1m,
                                       GetWeight(t1m),
                                       GetBlues(t1m),
                                       GetAlignment(t1m),
                                       &prep, PREPSIZE,
                                       &(ttm->maxprepstack)))>0) {

         /* Store the pre-program. */
         UsePrep(ttm, prep, prep_size);
      }

      /* CVT entries. */
      blues = GetBlues(t1m);
      if (status!=NOMEM &&
          (ttm->cvt = Malloc(blues->align.cvt * CVTSIZE)) == NULL) {
         SetError(status = NOMEM);
      } else {
         ADDCVT(0);  /* TMPCVT */
         ADDCVT((GetStdVW(t1m)==0) ?
                TransX(t1m, GetDefStdVW(t1m))/2 :
            TransX(t1m, GetStdVW(t1m))/2);
         ADDCVT((GetStdHW(t1m)==0) ?
                TransY(t1m, GetDefStdHW(t1m))/2 :
            TransY(t1m, GetStdHW(t1m))/2);
         for (i=0; i<t1m->snapv_cnt; i++)
            ADDCVT(TransY(t1m, t1m->stemsnapv[i])/2);
         for (i=0; i<t1m->snaph_cnt; i++)
            ADDCVT(TransY(t1m, t1m->stemsnaph[i])/2);

         /* Align the top zones. */
         align = GetAlignment(t1m);
         for (i=0; i<blues->blue_cnt/2; i++) {
            /* Skip empty zones. */
            if (align->top[i].cnt==0)
               continue;
            
            ttm->cvt[align->top[i].blue_cvt]
                  = (short)TransY(t1m, blues->bluevalues[i*2]);
            ttm->cvt[align->top[i].blue_cvt+1]
                  = (short)TransY(t1m, blues->bluevalues[i*2+1]);
            for (j=0; j<align->top[i].cnt; j++) {
               funit pos;
               int k;

               /* Get the closest family. */
               k = MatchingFamily(blues->bluevalues[i*2],
                                  blues->familyblues,
                                  blues->fblue_cnt);

               /* Compute the position in the zone w.r.t. the family blues. */
               if (blues->bluevalues[i*2] != blues->bluevalues[i*2+1])
                  pos = IP(align->top[i].pos[j].y,
                           blues->bluevalues[i*2],
                           blues->bluevalues[i*2+1],
                           blues->familyblues[k],
                           blues->familyblues[k+1]);
               else
                  pos = blues->familyblues[k];

               ttm->cvt[align->top[i].pos[j].cvt]
                     = (short)TransY(t1m, align->top[i].pos[j].y);
               ttm->cvt[align->top[i].pos[j].cvt+1]
                     = (short)TransY(t1m, pos);
            }
         }

         /* Align the bottom zones. */
         for (i=0; i<blues->oblue_cnt/2; i++) {
            /* Skip empty zones. */
            if (align->bottom[i].cnt==0)
               continue;
            
            ttm->cvt[align->bottom[i].blue_cvt]
                  = (short)TransY(t1m, blues->otherblues[i*2+1]);
            for (j=0; j<align->bottom[i].cnt; j++) {
               funit pos;
               int k;

               /* Get the closest family. */
               k = MatchingFamily(blues->otherblues[i*2],
                                  blues->familyotherblues,
                                  blues->foblue_cnt);

               /* Compute the position in the zone w.r.t. the family blues. */
               if (blues->otherblues[i*2] != blues->otherblues[i*2+1])
                  pos = IP(align->bottom[i].pos[j].y,
                           blues->otherblues[i*2],
                           blues->otherblues[i*2+1],
                           blues->familyotherblues[k],
                           blues->familyotherblues[k+1]);
               else
                  pos = blues->familyotherblues[k];

               ttm->cvt[align->bottom[i].pos[j].cvt]
                     = (short)TransY(t1m, align->bottom[i].pos[j].y);
               ttm->cvt[align->bottom[i].pos[j].cvt+1]
                     = (short)TransY(t1m, pos);
            }
         }

         /* Add the family zones. */
         for (i=0; i<blues->fblue_cnt/2; i++) {
            if (blues->family_cvt[i]!=UNDEF_CVT) {
               ttm->cvt[blues->family_cvt[i]]
                     = (short)TransY(t1m, blues->familyblues[i*2]);
               ttm->cvt[blues->family_cvt[i]+1]
                     = (short)TransY(t1m, blues->familyblues[i*2+1]);
            }
         }

         /* Add the family other zones. */
         for (i=0; i<blues->foblue_cnt/2; i++) {
            if (blues->familyother_cvt[i]!=UNDEF_CVT) {
               ttm->cvt[blues->familyother_cvt[i]]
                     = (short)TransY(t1m, blues->familyotherblues[i*2+1]);
            }
         }

         ttm->cvt_cnt = blues->align.cvt;
         ttm->maxstorage = t1m->stems.storage;

         /* Store the font-program. */
         SetFPGM(ttm, GetFontProg(), GetFontProgSize(), GetNumFuns());
      }
   }

   return status;
}
