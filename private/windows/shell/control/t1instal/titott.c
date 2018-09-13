/***
 **
 **   Module: TItoTT
 **
 **   Description:
 **      This is the main module of the Postscript Type I to TrueType
 **      font converter.
 **
 **   Author: Michael Jansson
 **   Created: 5/26/93
 **
 ***/


/***** INCLUDES */
#include <string.h>
#include "types.h"
#include "safemem.h"
#include "metrics.h"
#include "titott.h"
#include "t1parser.h"
#include "builder.h"
#include "trans.h"


/***** LOCAL TYPES */
/*-none-*/


/***** CONSTANTS */
#define NOTDEFNAME  ".notdef"

static const struct TTGlyph null = {
   NULL,
   0, 0, 0, NULL,
   NULL,
   0, 0
};
static Point mpo3[] = {
   {1150, 10}, {1150, 30}, {1160, 30}, {1170, 20}, {1180, 30}, {1190, 30}, 
   {1190, 10}, {1180, 10}, {1180, 20}, {1170, 10}, {1160, 20}, {1160, 10}
};
static Point mpo2[] = {
   {60, 40}, {60, 1560}, {1160, 1560}, {1160, 40}
};
static Point mpo1[] = {
   {20, 0}, {1200, 0}, {1200, 1600}, {20, 1600}
};
static ULONG onoff[1] = {0L};
static Outline p1 = {
   NULL,
   sizeof(mpo3)/sizeof(mpo3[0]),
   &mpo3[0],
   &onoff[0]
};
static Outline p2 = {
   &p1,
   sizeof(mpo2)/sizeof(mpo2[0]),
   &mpo2[0],
   &onoff[0]
};
static Outline missingPath = {
   &p2,
   sizeof(mpo1)/sizeof(mpo1[0]),
   &mpo1[0],
   &onoff[0]
};

static struct TTGlyph missing = {
   NULL,
   MAXNOTDEFSIZE, 0, 0, NULL,
   &missingPath,
   1500, 0
};


/***** MACROS */
/*-none-*/


/***** GLOBALS */
/*-none-*/


/***** STATIC FUNCTIONS */
/*-none-*/


/**** FUNCTIONS */

/***
 ** Function: ConvertT1toTT
 **
 ** Description:
 **   Convert a T1 font into a TT font file.
 ***/
errcode ConvertT1toTT(const struct TTArg *ttArg,
                      const struct T1Arg *t1Arg,
                      const short (*check)(const char *copyright,
                                           const char *notice,
                                           const char *facename),
                      struct callProgress *cp)
{
   /* Resources */
   struct T1Handle  *t1 = NULL;
   struct TTHandle  *tt = NULL;
   struct T1Metrics *t1m = NULL;

   /* Temporary variables. */
   struct T1Glyph  glyph;
   struct TTGlyph *ttglyph;
   struct Composite *comp;
   struct TTComposite ttcomp;
   struct TTMetrics ttm;
   boolean fStdEncoding;
   boolean done;
   errcode status;

   /* Initiate variables. */
   ttglyph = NULL;
   memset(&glyph, '\0', sizeof(glyph));
   memset(&ttm, '\0', sizeof(ttm));

   /* Inititate input and output */
   if ((status = InitT1Input(t1Arg, &t1, &t1m, check))==SUCCESS &&
       (status = InitTTOutput(ttArg, &tt))==SUCCESS) {
     
      done = FALSE;

      fStdEncoding = (CurrentEncoding(t1m)==NULL);

      /* Create the missing and the null glyph. */
      if ((missing.hints = Malloc(MAXNOTDEFSIZE))==NULL) {
         status = NOMEM;
         done = TRUE;
      } else {
         memset(missing.hints, 0x22, MAXNOTDEFSIZE);
         (void)PutTTGlyph(tt, &missing, fStdEncoding);
         (void)PutTTGlyph(tt, &null, fStdEncoding);
         Free(missing.hints);
      }

      /* Convert the simple glyphs. */
      while(!done) {
         status = GetT1Glyph(t1, &glyph, t1Arg->filter);
         if (status == SUCCESS) {
            if ((status = ConvertGlyph(t1m,
                                       &glyph,
                                       &ttglyph,
                                       (int)ttArg->precision))!=SUCCESS ||
                (status = PutTTGlyph(tt, ttglyph, fStdEncoding))!=SUCCESS) {
               done = TRUE;
            } else {

               FreeTTGlyph(ttglyph);
               ttglyph=NULL;
               if (cp)
                  cp->cb((short)0, &glyph, cp->arg);
            }
         } else if (status<=FAILURE || status==DONE) {
            done = TRUE;
         } else {
            /* Handle the missing glyph ".notdef" */
            if (!strcmp(glyph.name, NOTDEFNAME)) {
               if ((status = ConvertGlyph(t1m,
                                          &glyph,
                                          &ttglyph,
                                          (int)ttArg->precision))!=SUCCESS ||
                   (status = PutTTNotDefGlyph(tt, ttglyph))!=SUCCESS) {
                  done = TRUE;
               } else {
                  FreeTTGlyph(ttglyph);
                  ttglyph=NULL;
                  if (cp)
                     cp->cb((short)0, &glyph, cp->arg);
               }
            }
         }
         FreeT1Glyph(&glyph);
      }
      
      if (status==DONE) {

         /* Convert the composite glyphs. */
         while ((comp = GetT1Composite(t1))!=NULL) {

            /* Check if the base glyph is converted */
            if ((status = GetT1BaseGlyph(t1, comp, &glyph))==SUCCESS) {
               if ((status = ConvertGlyph(t1m,
                                          &glyph,
                                          &ttglyph,
                                          (int)ttArg->precision))!=SUCCESS ||
                   (status = PutTTGlyph(tt, ttglyph, fStdEncoding))!=SUCCESS) {
                  break;
               }
               FreeTTGlyph(ttglyph);
               ttglyph=NULL;
               if (cp)
                  cp->cb((short)0, &glyph, cp->arg);
            } else if (status<=FAILURE)
               break;
            FreeT1Glyph(&glyph);

            /* Check if the base accent is converted */
            if ((status = GetT1AccentGlyph(t1, comp, &glyph))==SUCCESS) {
               if ((status = ConvertGlyph(t1m,
                                          &glyph,
                                          &ttglyph,
                                          (int)ttArg->precision))!=SUCCESS ||
                   (status = PutTTGlyph(tt, ttglyph, fStdEncoding))!=SUCCESS) {
                  break;
               }
               FreeTTGlyph(ttglyph);
               ttglyph=NULL;
               if (cp)
                  cp->cb((short)0, &glyph, cp->arg);
            } else if (status<=FAILURE)
               break;
            FreeT1Glyph(&glyph);


            /* Convert and store accented glyph. */
            if (status>=SUCCESS && 
                ((status = ConvertComposite(t1m, comp, &ttcomp))!=SUCCESS ||
                 (status = PutTTComposite(tt, &ttcomp))!=SUCCESS)) {
               break;
            }
            if (cp)
               cp->cb((short)1, &comp, cp->arg);
         }

         /* Flush out un-used work space. */
         FlushWorkspace(t1);

         /* Convert the metrics. */
         if (status==SUCCESS || status==DONE || status==SKIP) {
            if ((status = ReadOtherMetrics(t1m,
                                           t1Arg->metrics))==SUCCESS &&
                (status = ConvertMetrics(tt, t1m, &ttm,
                                         ttArg->tag))==SUCCESS) {
               if (cp)
                  cp->cb((short)2, NULL, cp->arg);
               status = PutTTOther(tt, &ttm);
            }
         }
      }
   }                               

   /* More progress. */
   if (cp)
      cp->cb((short)3, NULL, cp->arg);

   FreeTTMetrics(&ttm);
   FreeTTGlyph(ttglyph);
   FreeT1Glyph(&glyph);
   if (CleanUpTT(tt, ttArg, status)!=SUCCESS && status==SUCCESS)
      status = BADINPUTFILE;
   if (CleanUpT1(t1)!=SUCCESS && status==SUCCESS)
      status = BADINPUTFILE;

   /* All done! */
   if (cp)
      cp->cb((short)4, NULL, cp->arg);


   return status;
}
