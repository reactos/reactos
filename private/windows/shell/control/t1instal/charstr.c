/***
 **
 **   Module: CharStr
 **
 **   Description:
 **    This is a module of the T1 to TT font converter. The module
 **    contain one function that interprets the commands in a T1
 **    CharString and builds a representation of the glyph for the
 **    it.
 **
 **   Author: Michael Jansson
 **
 **   Created: 5/26/93
 **
 ***/
    

/**** INCLUDES */
/* General types and definitions. */
#include <string.h>

/* Special types and definitions. */
#include "titott.h"
#include "types.h"
#include "metrics.h"
#include "encoding.h"
#include "safemem.h"
#include "t1msg.h"

/* Module dependent types and prototypes. */
#include "charstr.h"



/***** CONSTANTS */
#define MAXSTACK  24

#define BUFMARGIN    256

/* Known OtherSubr entries. */
#define HINT_END_FLEX      0
#define HINT_START_FLEX    1
#define HINT_MID_FLEX      2
#define HINT_REPLACEMENT   3


#define PSOP_HSTEM         1
#define PSOP_VSTEM         3
#define PSOP_VMOVETO       4
#define PSOP_RLINETO       5
#define PSOP_HLINETO       6
#define PSOP_VLINETO       7
#define PSOP_RRCURVETO     8
#define PSOP_CLOSEPATH     9
#define PSOP_CALLSUBR      10
#define PSOP_RETURN        11
#define PSOP_ESCAPE        12
#  define PSOP_DOTSECTION        0
#  define PSOP_VSTEM3            1
#  define PSOP_HSTEM3            2
#  define PSOP_SEAC              6
#  define PSOP_SBW               7
#  define PSOP_DIV               12
#  define PSOP_CALLOTHERSUBR     16
#  define PSOP_POP               17
#  define PSOP_SETCURRENTPOINT   33
#define PSOP_HSBW          13
#define PSOP_ENDCHAR       14
#define PSOP_RMOVETO       21
#define PSOP_HMOVETO       22
#define PSOP_VHCURVETO     30
#define PSOP_HVCURVETO     31




/***** LOCAL TYPES */
/* Referenced types. */
typedef struct PSState {
   long stack[MAXSTACK+2];
   Point *pts;
   ULONG *onoff;
   USHORT totpts;
   USHORT numpts;
   USHORT ptr;
   short hr;
   USHORT maxpts;
   USHORT flex;
   Point flexref;
	int calls;
	long otherargs[16];
	USHORT otherptr;
} PSState;



/***** MACROS */
#define AddPoint(ps, px, py) {ps->pts[ps->numpts].x = px;\
                              ps->pts[ps->numpts].y = py;\
                              ps->numpts++;}
#define CurrXPoint(ps)        ps->pts[ps->numpts-1].x
#define CurrYPoint(ps)        ps->pts[ps->numpts-1].y
#define PrevPoint(ps, index)  ps->pts[ps->numpts-1-index]

#define PushStack(v)    ps->stack[(ps->ptr)++] = (v)
#define PopStack()      ps->stack[--(ps->ptr)]




/***** STATIC FUNCTIONS */

/***
 ** Function: AddCSpline
 **
 ** Description:
 **   Record a cubic spline.
 **
 ***/
static void AddCSpline(PSState *ps,
                       const funit x1, const funit y1,
                       const funit x2, const funit y2,
                       const funit x3, const funit y3)
{
   SetOffPoint(ps->onoff, ps->numpts);
   AddPoint(ps, x1, y1);
   SetOffPoint(ps->onoff, ps->numpts);
   AddPoint(ps, x2, y2);
   SetOnPoint(ps->onoff, ps->numpts);
   AddPoint(ps, x3, y3);
}


/***
** Function: HintReplacement
**
** Description:
**   This function limits the range of points which are
**   affected by stem hints. It may be invoked by the 
**   "dotsection" and the more general hint replacement
**   mechanism found in T1 fonts.
***/
static void HintReplacement(T1Glyph *glyph, const short hr)
{
   Stem *stem;
   Stem3 *stem3;

   /* Dead stem hints?, e.g. replaced before ever used? */
   if (hr==ENDOFPATH) {
      for (stem=glyph->hints.hstems; stem; stem=stem->next)
         if (stem->i2==ENDOFPATH)
            stem->i2 = NORANGE;
      for (stem=glyph->hints.vstems; stem; stem=stem->next)
         if (stem->i2==ENDOFPATH)
            stem->i2 = NORANGE;
      for (stem3=glyph->hints.hstems3; stem3; stem3=stem3->next) {
         if (stem3->stem1.i2==ENDOFPATH) {
            stem3->stem1.i2 = NORANGE;
            stem3->stem2.i2 = NORANGE;
            stem3->stem3.i2 = NORANGE;
         }
      }
      for (stem3=glyph->hints.vstems3; stem3; stem3=stem3->next) {
         if (stem3->stem1.i2==ENDOFPATH) {
            stem3->stem1.i2 = NORANGE;
            stem3->stem2.i2 = NORANGE;
            stem3->stem3.i2 = NORANGE;
         }
      }

   /* Set the end-point for the stem hints that are replaced. */
   } else {

      for (stem=glyph->hints.hstems; stem; stem=stem->next)
         if (stem->i2==ENDOFPATH)
            stem->i2 = hr;
      for (stem=glyph->hints.vstems; stem; stem=stem->next)
         if (stem->i2==ENDOFPATH)
            stem->i2 = hr;
      for (stem3=glyph->hints.hstems3; stem3; stem3=stem3->next) {
         if (stem3->stem1.i2==ENDOFPATH) {
            stem3->stem1.i2 = hr;
            stem3->stem2.i2 = hr;
            stem3->stem3.i2 = hr;
         }
      }
      for (stem3=glyph->hints.vstems3; stem3; stem3=stem3->next) {
         if (stem3->stem1.i2==ENDOFPATH) {
            stem3->stem1.i2 = hr;
            stem3->stem2.i2 = hr;
            stem3->stem3.i2 = hr;
         }
      }
   }
}   


/***
** Function: NewFlex
**
** Description:
**   This function records a new flex hint for
**   the current glyph.
***/
static Flex *NewFlex(const Point ref,
                     const Point midpos,
                     const Point startpos,
                     const USHORT start,
                     const USHORT mid,
                     const USHORT end,
                     const funit civ)

{
   Flex *flex = NULL;

   if ((flex = Malloc(sizeof(Flex)))!=NULL) {
      flex->civ = civ;
      flex->pos = ref;
      flex->midpos = midpos;
      flex->startpos = startpos;
      flex->start = start;
      flex->mid = mid;
      flex->end = end;
   }

   return flex;
}



/***
** Function: NewStem
**
** Description:
**   This function records a new stem hint for
**   the current glyph.
***/
static Stem *NewStem(const funit offset,
                     const funit width,
                     const short hr)
{
   Stem *stem = NULL;

   if ((stem = Malloc(sizeof(Stem)))!=NULL) {
      if (width>0) {
         stem->offset = offset;
         stem->width = width;
      } else {
         stem->offset = offset+width;
         stem->width = -width;
      }
      stem->i1 = hr;
      stem->i2 = ENDOFPATH;
   }

   return stem;
}



/***
** Function: NewStem3
**
** Description:
**   This function records a new stem3 hint for the current glyph.
***/
static Stem3 *NewStem3(const funit o1, const funit w1,
                       const funit o2, const funit w2,
                       const funit o3, const funit w3,
                       const short hr)
{
   Stem3 *stem3 = NULL;

   if ((stem3 = Malloc(sizeof(Stem3)))!=NULL) {

      stem3->stem1.offset = o1;
      stem3->stem1.width = w1;
      stem3->stem1.i1 = hr;
      stem3->stem1.i2 = ENDOFPATH;

      stem3->stem2.offset = o2;
      stem3->stem2.width = w2;
      stem3->stem2.i1 = hr;
      stem3->stem2.i2 = ENDOFPATH;

      stem3->stem3.offset = o3;
      stem3->stem3.width = w3;
      stem3->stem3.i1 = hr;
      stem3->stem3.i2 = ENDOFPATH;
   }

   return stem3;
}



/***
** Function: NewPath
**
** Description:
**   This function adds a new contour to the current glyph.
***/
static errcode NewPath(T1Glyph *glyph,
                       const Point *pts,
                       ULONG *onoff,
                       const USHORT numpts)
{
   errcode status = SUCCESS;
   Outline *path;
   Outline *prev;

   /* Skip 1,2 point paths. */
   if (numpts>2) {
      if ((path=Malloc(sizeof(Outline)))==NULL) {
         SetError(status = NOMEM);
      } else {

         path->pts=Malloc(sizeof(Point)*numpts);
         path->onoff=Malloc(ONOFFSIZE(numpts));
         if (path->pts==NULL || path->onoff==NULL) {
            if (path->pts)
               Free(path->pts);
            if (path->onoff)
               Free(path->onoff);
            Free(path);
            return NOMEM;
         }

         /* Initiate the new sub-path. */
         memcpy(path->pts, pts, sizeof(Point)*numpts);
         memcpy(path->onoff, onoff, ONOFFSIZE(numpts));
         path->count = numpts;

         /* Link it in. */
         path->next = NULL;
         if (glyph->paths==NULL) {
            glyph->paths = path;
         } else {
            for (prev = glyph->paths; prev->next; prev=prev->next);
            prev->next = path;
         }
      }
   }

   return status;
}



/***** FUNCTIONS */

/***
** Function: AllocPSState
**
** Description:
**   This function allocates the workspace
**   used by the t1 parser.
***/
struct PSState *AllocPSState(void)
{
   struct PSState *ps;

   if ((ps = Malloc(sizeof(struct PSState)))!=NULL) {
      memset(ps, '\0', sizeof(struct PSState));
   }

   return ps;      
}


/***
** Function: FreePSState
**
** Description:
**   This function frees the workspace
**   used by the t1 parser.
***/
void FreePSState(struct PSState *ps)
{
   if (ps) {
      if (ps->onoff)
         Free(ps->onoff);
      if (ps->pts)
         Free(ps->pts);
   }

   Free(ps);
}


/***
** Function: InitPS
**
** Description:
**   This function initiate the workspace
**   used by the t1 parser.
***/
void InitPS(struct PSState *ps)
{
   ps->hr = 0;
   ps->totpts = 0;
   ps->ptr = 0;
	ps->calls = 0;
}


/***
** Function: ParseCharString
**
** Description:
**   This function parses a CharString and builds a
**   of the charstring glyph.
***/
errcode ParseCharString(T1Glyph *glyph,
                        struct Composite **comp,
                        PSState *ps,
                        const struct Subrs *subrs,
                        UBYTE *code,
                        USHORT len)
{
   Composite *c;
   Stem3 *stem3;
   Stem *stem;
   Flex *flex;
   long v, w;
   funit dx1, dy1, dx2, dy2, dx3, dy3;
   funit x0, y0, x1, y1, x2, y2, x3, y3;
   funit width, offset;
   funit o1, o2, o3, w1, w2, w3;
   long v1, v2, v3, v4;
   errcode status = SUCCESS;
   long subr, args;
   funit flexciv;
   char *glyph_name = NULL;
   USHORT zero[ENC_MAXCODES];
   char msg[64];
   int i;

   /* Keep track on the number of recursive calls. */
   ps->calls++;
   if (ps->calls>100) {
      LogError(MSG_ERROR, MSG_RECURSION, NULL);
      SetError(status = BADCHARSTRING);
      return status;
   }


   while (len) {
      v = *code++; len--;

      /* Check for space for the paths. */
      if (ps->numpts+4>=ps->maxpts) {
         Point *newpts;
         ULONG *newonoff;

         if ((newpts=Realloc(ps->pts,
                             sizeof(Point)*(ps->maxpts+BUFMARGIN)))==NULL)
            return NOMEM;
         else
            ps->pts = newpts;

         if ((newonoff=Realloc(ps->onoff,
                               ONOFFSIZE(ps->maxpts+BUFMARGIN)))==NULL)
            return NOMEM;
         else
            ps->onoff = newonoff;

         ps->maxpts += BUFMARGIN;
      }



      /* Decode integer. */
      if (v>=32) {
         if (v<=246)
            PushStack(v-139);
         else if (v<=250) {
            w = *code++; len--;
            PushStack((v-247)*256+w+108);
         } else if (v<=254) {
            w = *code++; len--;
            PushStack(-(v-251)*256-w-108);
         } else {
            v1 = *code++; len--;
            v2 = *code++; len--;
            v3 = *code++; len--;
            v4 = *code++; len--;
            PushStack(((v1*256+v2)*256+v3)*256+v4);
         }

         /* Decode command. */
      } else {
         switch (v) {
            case PSOP_HSTEM:
               width  = (funit)PopStack();
               offset = (funit)PopStack();
               if ((stem = NewStem(offset, width, ps->hr))==NULL)
                  return NOMEM;
               stem->next = glyph->hints.hstems;
               glyph->hints.hstems = stem;
               break;
            case PSOP_VSTEM:
               width  = (funit)PopStack();
               offset = (funit)PopStack()+glyph->lsb.x;
               if ((stem = NewStem(offset, width, (ps->hr)))==NULL)
                  return NOMEM;
               stem->next = glyph->hints.vstems;
               glyph->hints.vstems = stem;
               break;
            case PSOP_VMOVETO:
               CurrYPoint(ps) += (funit)PopStack();
               break;
            case PSOP_RLINETO:
               y1 = CurrYPoint(ps) + (funit)PopStack();
               x1 = CurrXPoint(ps) + (funit)PopStack();
               SetOnPoint(ps->onoff, ps->numpts);
               AddPoint(ps, x1, y1);
               break;
            case PSOP_HLINETO:
               x1 = CurrXPoint(ps) + (funit)PopStack();
               y1 = CurrYPoint(ps);
               SetOnPoint(ps->onoff, ps->numpts);
               AddPoint(ps, x1, y1);
               break;
            case PSOP_VLINETO:
               x1 = CurrXPoint(ps);
               y1 = CurrYPoint(ps) + (funit)PopStack();
               SetOnPoint(ps->onoff, ps->numpts);
               AddPoint(ps, x1, y1);
               break;
            case PSOP_RRCURVETO:
               dy3 = (funit)PopStack();
               dx3 = (funit)PopStack();
               dy2 = (funit)PopStack();
               dx2 = (funit)PopStack();
               dy1 = (funit)PopStack();
               dx1 = (funit)PopStack();

               x0 = CurrXPoint(ps);
               y0 = CurrYPoint(ps);
               x1 = x0 + dx1;
               y1 = y0 + dy1;
               x2 = x1 + dx2;
               y2 = y1 + dy2;
               x3 = x2 + dx3;
               y3 = y2 + dy3;

               AddCSpline(ps, x1, y1, x2, y2, x3, y3);
               break;
            case PSOP_CLOSEPATH:
               /* Add the path to the glyph. */
               if ((status = NewPath(glyph, ps->pts,
                                     ps->onoff, ps->numpts))!=SUCCESS)
                  return status;

               /* Current point = last point. */
               SetOnPoint(ps->onoff, 0);
               ps->pts[0] = ps->pts[ps->numpts-1];
               ps->totpts = (USHORT)(ps->totpts + ps->numpts);
               ps->numpts = 1;
               break;
            case PSOP_CALLSUBR:
               v = PopStack();
               if ((status = ParseCharString(glyph, comp, ps,
                                             subrs, subrs[v].code,
                                             subrs[v].len))!=SUCCESS)
                  return status;
               break;
            case PSOP_RETURN:
					ps->calls--;
               return SUCCESS;
            case PSOP_ESCAPE:
               v = *code++; len--;
               switch (v) {
                  case PSOP_DOTSECTION:
                     /* End of dot section? */
                     if (ps->numpts>1) {
                        ps->hr = (short)(ps->totpts+ps->numpts-1);
                        HintReplacement(glyph, (short)(ps->hr-1));
                     }
                     break;
                  case PSOP_VSTEM3:
                     w1 = (funit)PopStack();
                     o1 = (funit)PopStack()+glyph->lsb.x;
                     w2 = (funit)PopStack();
                     o2 = (funit)PopStack()+glyph->lsb.x;
                     w3 = (funit)PopStack();
                     o3 = (funit)PopStack()+glyph->lsb.x; 
                     if (o1>o2) {
                        SWAPINT(o1, o2);
                        SWAPINT(w1, w2);
                     }
                     if (o1>o3) {
                        SWAPINT(o1, o3);
                        SWAPINT(w1, w3);
                     }
                     if (o3<o2) {
                        SWAPINT(o2, o3);
                        SWAPINT(w2, w3);
                     }
                     if ((stem3 = NewStem3(o1, w1,
                                           o2, w2,
                                           o3, w3,
                                           ps->hr))==NULL)
                        return NOMEM;
                     stem3->next = glyph->hints.vstems3;
                     glyph->hints.vstems3 = stem3;
                     break;
                  case PSOP_HSTEM3:
                     w1 = (funit)PopStack(); o1 = (funit)PopStack();
                     w2 = (funit)PopStack(); o2 = (funit)PopStack();
                     w3 = (funit)PopStack(); o3 = (funit)PopStack(); 
                     if ((stem3 = NewStem3(o1, w1,
                                           o2, w2,
                                           o3, w3,
                                           ps->hr))==NULL)
                        return NOMEM;
                     stem3->next = glyph->hints.hstems3;
                     glyph->hints.hstems3 = stem3;
                     break;
                  case PSOP_SEAC:
                     if ((c=Malloc(sizeof(Composite)))==NULL) {
                        return NOMEM;
                     }
                     c->aw = glyph->width.x;
                     c->achar = LookupCharName(DecodeChar(NULL, (USHORT)0,
                                                          ENC_STANDARD,
                                                          (USHORT)PopStack()));
                     c->bchar = LookupCharName(DecodeChar(NULL, (USHORT)0,
                                                          ENC_STANDARD,
                                                          (USHORT)PopStack()));
                     if ((c->cchar = Strdup(glyph->name))==NULL)
                        return NOMEM;
                     c->oenc = NULL;
                     c->ady = (funit)PopStack();
                     c->adx = (funit)PopStack();
                     c->asbx = (funit)PopStack();
                     c->next = *comp;
                     *comp = c;
                     if (glyph->paths==NULL)
                        return SKIP;
                     if ((glyph->name = Malloc(strlen(c->achar)+
                                               strlen(c->bchar)+2))==NULL ||
                         (c->oenc = AllocEncodingTable((USHORT)1))==NULL) {
                        if (glyph->name)
                           Free(glyph->name);
                        if (c->oenc) 
                           Free(c->oenc);
                        return NOMEM;
                     }
                     strcpy(glyph->name, c->bchar);
                     strcat(glyph->name, c->achar);
                     memset(zero, '\0', sizeof(zero[0])*ENC_MAXCODES);
                     if ((glyph_name = Strdup(glyph->name))!=NULL)
                        SetEncodingEntry(c->oenc, (USHORT)0, glyph_name,
                                         ENC_MAXCODES, zero);
                     else
                        return NOMEM;
                     break;
                  case PSOP_SBW:
                     glyph->width.y = (funit)PopStack();
                     glyph->width.x = (funit)PopStack();
                     glyph->lsb.y = (funit)PopStack();
                     glyph->lsb.x = (funit)PopStack();
                     ps->pts[0].x = glyph->lsb.x;
                     ps->pts[0].y = glyph->lsb.y;
                     ps->numpts = 1;
                     SetOnPoint(ps->onoff, 0);
                     break;
                  case PSOP_DIV:
                     /* LogError("Floating point precision lost.\n"); */       
                     y1 = (funit)PopStack();
                     x1 = (funit)PopStack();
                     PushStack(((long)x1+(long)(y1>>1))/(long)y1);
                     break;
                  case PSOP_CALLOTHERSUBR:
                     subr = PopStack();
                     args = PopStack();

							/* Remember the arguments for future pop's. */
							for (i=0; i<args; i++) {
								ps->otherargs[i] = ps->stack[args-i-1];
							}
							ps->otherptr = 0;

                     switch (subr) {
                        case HINT_END_FLEX:
                           y0 = (funit)PopStack();
                           x0 = (funit)PopStack();
                           flexciv = (funit)PopStack();
                           ps->numpts--;
                           SetOffPoint(ps->onoff, ps->numpts-2);
                           SetOffPoint(ps->onoff, ps->numpts-3);
                           SetOffPoint(ps->onoff, ps->numpts-5);
                           SetOffPoint(ps->onoff, ps->numpts-6);
                           if ((flex = NewFlex(ps->flexref,
                                               PrevPoint(ps, 3),
                                               PrevPoint(ps, 6),
                                               (USHORT)(ps->totpts+
                                                        ps->numpts-7),
                                               (USHORT)(ps->totpts+
                                                        ps->numpts-4),
                                               (USHORT)(ps->totpts+
                                                        ps->numpts-1),
                                               flexciv))==NULL)
                              return NOMEM;
                           flex->next = glyph->hints.flex;
                           glyph->hints.flex = flex;
                           break;
                        case HINT_START_FLEX:
                           ps->flex = 1;
                           x0 = CurrXPoint(ps);
                           y0 = CurrYPoint(ps);
						   SetOnPoint(ps->onoff, ps->numpts);
                           AddPoint(ps, x0, y0);
                           break;
                        case HINT_MID_FLEX:
                           y0 = CurrYPoint(ps);
                           x0 = CurrXPoint(ps);
                           if (ps->flex==1) {
                              ps->flexref.x = x0;
                              ps->flexref.y = y0;
                              ps->flex=0;
                           } else {
							  SetOnPoint(ps->onoff, ps->numpts);
                              AddPoint(ps, x0, y0);
                           }
                           break;
                        case HINT_REPLACEMENT:
                           ps->hr = (short)(ps->totpts+ps->numpts-1);
                           HintReplacement(glyph, (short)(ps->hr-1));
                           (void)PopStack();
                           break;
                        default:
                           ps->ptr = (USHORT)(ps->ptr - args);
                           break;
                     }
                     break;
                        case PSOP_POP:
									PushStack(ps->otherargs[ps->otherptr++]);
                           break;
                        case PSOP_SETCURRENTPOINT:
                           (ps->ptr) -= 2;
                           break;
                        case 15:
                           ps->ptr-=2;
                           break;
                        default:
                           LogError(MSG_ERROR, MSG_BADESC, NULL);
                           SetError(status = BADCHARSTRING);
                           return status;
               }
               break;
            case PSOP_HSBW:
               glyph->width.x = (funit)PopStack();
               glyph->width.y = 0;
               glyph->lsb.x = (funit)PopStack();
               glyph->lsb.y = 0;
               ps->pts[0].x = glyph->lsb.x;
               ps->pts[0].y = 0;
               ps->numpts = 1;
               SetOnPoint(ps->onoff, 0);
               break;
            case PSOP_ENDCHAR:
               if (ps->numpts>1) {
                  if ((status = NewPath(glyph, ps->pts,
                                        ps->onoff, ps->numpts))!=SUCCESS)
                     return status;
               }
               return SUCCESS;
            case PSOP_RMOVETO:
               CurrYPoint(ps) += (funit)PopStack();
               CurrXPoint(ps) += (funit)PopStack();
               break;
            case PSOP_HMOVETO:
               CurrXPoint(ps) += (funit)PopStack();
               break;
            case PSOP_VHCURVETO:
               dy3 = 0;
               dx3 = (funit)PopStack();
               dy2 = (funit)PopStack();
               dx2 = (funit)PopStack();
               dy1 = (funit)PopStack();
               dx1 = 0;

               x0 = CurrXPoint(ps);
               y0 = CurrYPoint(ps);
               x1 = x0 + dx1;
               y1 = y0 + dy1;
               x2 = x1 + dx2;
               y2 = y1 + dy2;
               x3 = x2 + dx3;
               y3 = y2 + dy3;

               AddCSpline(ps, x1, y1, x2, y2, x3, y3);
               break;
            case PSOP_HVCURVETO:
               dy3 = (funit)PopStack();
               dx3 = 0;
               dy2 = (funit)PopStack();
               dx2 = (funit)PopStack();
               dy1 = 0;
               dx1 = (funit)PopStack();

               x0 = CurrXPoint(ps);
               y0 = CurrYPoint(ps);
               x1 = x0 + dx1;
               y1 = y0 + dy1;
               x2 = x1 + dx2;
               y2 = y1 + dy2;
               x3 = x2 + dx3;
               y3 = y2 + dy3;

               AddCSpline(ps, x1, y1, x2, y2, x3, y3);
               break;
            case 15:
               ps->ptr -=2;
               break;

            default:
               strcpy(msg,  "'00' (00)");
               msg[1] = (char)('0' + (v/10));
               msg[2] = (char)('0' + (v%10));
               msg[6] = (char)('0' + (ps->ptr/10));
               msg[7] = (char)('0' + (ps->ptr%10));
               LogError(MSG_ERROR, MSG_BADOP, msg);
               SetError(status = BADCHARSTRING);
               return status;
         }
      }

      /* Check number of elements on the stack. */
      if (ps->ptr>MAXSTACK)
         return BADCHARSTRING;
   }

   return SUCCESS;
}

