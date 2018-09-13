/***
**
**   Module: PFM
**
**   Description:
**      This is a module of the T1 to TT font converter. The module
**      will extract information from a T1 font metrics file, by parsing
**      the data/commands found in a PFM file.
**
**      Please note that all data stored in a PFM file is represented
**      in the little-endian order.
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
#include "safemem.h"
#include "metrics.h"
#include "t1msg.h"

/* Module dependent types and prototypes. */
#include "fileio.h"



/***** CONSTANTS */
/*-none-*/


/***** LOCAL TYPES */
/*-none-*/


/***** MACROS */
/*-none-*/


/***** STATIC FUNCTIONS */

/***
** Function: GetNextWord
**
** Description:
**   This function pulls two bytes from a file
**   and convert them into a 16-bit integer.
***/
static short GetNextWord(struct ioFile *file)
{
   short iWord;

   iWord = (short)io_ReadOneByte(file);
   iWord |= (short)(io_ReadOneByte(file) * 256);

   return(iWord);
}


/***
** Function: GetLong
**
** Description:
**   This function pulls four bytes from a file
**   and convert them into a 32-bit integer.
***/
static long GetLong(struct ioFile *file)
{
   short low;
   short high;


   low = GetNextWord(file);
   high = GetNextWord(file);

   return (long)((long)low+((long)high * 65535L));
}



/***
** Function: ReadString
**
** Description:
**   This function pulls a null terminated
**   string from the file.
***/
static void ReadString(UBYTE *dst, int size, struct ioFile *file)
{
   int i;

   i=0;
   while (io_FileError(file)==SUCCESS && i<size) {
      dst[i] = (UBYTE)io_ReadOneByte(file);
      if (dst[i]=='\0')
         break;
      i++;
   }
}






/***** FUNCTIONS */

/***
** Function: ReadPFMMetrics
**
** Description:
**   This function parses a Printer Font Metrics
**   (*.pfm) file. 
***/
errcode ReadPFMMetrics(const char *metrics, struct T1Metrics *t1m)
{
   errcode status = SUCCESS;
   struct ioFile *file;
   UBYTE buf[256];
   long kernoffset;
   long widthoffset;
   long etmoffset;
   long faceoffset;
   short ver;
   short i;

   if (metrics==NULL || (file = io_OpenFile(metrics, READONLY))==NULL) {
      status = NOMETRICS;
   } else {

      (void)io_ReadOneByte(file);     /* Skip the revision number. */
      ver = (short)io_ReadOneByte(file);

      if (ver>3) {
         SetError(status=UNSUPPORTEDFORMAT);
      } else {

         (void)GetLong(file);        /* dfSize */

         /* Get Copyright */
         if (t1m->copyright)
            Free(t1m->copyright);
         if ((t1m->copyright = Malloc(60))==NULL) {
            SetError(status=NOMEM);
         } else {
            (void)io_ReadBytes((UBYTE *)t1m->copyright, (USHORT)60, file);

            (void)GetNextWord(file);                      /* dfType */
            (void)GetNextWord(file);                      /* dfPoints */
            (void)GetNextWord(file);                      /* dfVertRes */
            (void)GetNextWord(file);                      /* dfHorizRes */
            t1m->ascent = GetNextWord(file);              /* dfAscent */
            t1m->intLeading = GetNextWord(file);          /* dfInternalLeading */
            t1m->extLeading = GetNextWord(file);          /* dfExternalLeading */
            (void)io_ReadOneByte(file);               /* dfItalic */
            (void)io_ReadOneByte(file);               /* dfUnderline */
            (void)io_ReadOneByte(file);               /* dfStrikeOut */
            t1m->tmweight = (USHORT)GetNextWord(file);    /* dfWeight */
            t1m->CharSet = (UBYTE)io_ReadOneByte(file);   /* dfCharSet */
            (void)GetNextWord(file);                      /* dfPixWidth */
            (void)GetNextWord(file);                      /* dfPixHeight */
            t1m->pitchfam = (UBYTE)io_ReadOneByte(file);/* dfPitchAndFamily */
            t1m->avgCharWidth = GetNextWord(file);        /* dfAvgWidth */
            (void)GetNextWord(file);                      /* dfMaxWidth */
            t1m->firstChar = (UBYTE)io_ReadOneByte(file);   /* dfFirstChar */
            t1m->lastChar = (UBYTE)io_ReadOneByte(file);    /* dfLastChar */
            t1m->DefaultChar = (UBYTE)io_ReadOneByte(file); /* dfDefaultChar */
            t1m->BreakChar   = (UBYTE)io_ReadOneByte(file); /* dfBreakChar */
            (void)GetNextWord(file);                      /* dfWidthBytes */
            (void)GetLong(file);                      /* dfDevice */
	    faceoffset = GetLong(file);             /* dfFace */
            (void)GetLong(file);                      /* dfBitsPointer */
            (void)GetLong(file);                      /* dfBitsOffset */
            (void)GetNextWord(file);                      /* dfSizeFields */
            etmoffset = GetLong(file);                /* dfExtMetricsOffset */
            widthoffset = GetLong(file);              /* dfExtentTable */
            (void)GetLong(file);                      /* dfOriginTable */
            kernoffset = GetLong(file);               /* dfPairKernTable */
            (void)GetLong(file);                      /* dfTrackKernTable */
	    (void)GetLong(file);                      /* dfDriverInfo */
            (void)GetLong(file);                      /* dfReserved */

            if (io_FileError(file)!=SUCCESS) {
               SetError(status = BADMETRICS);
            }

            /* Get extended type metrics */
            (void)io_FileSeek(file, etmoffset);

            (void)GetNextWord(file);             /* etmSize */
            (void)GetNextWord(file);             /* etmPointSize */
            (void)GetNextWord(file);             /* etmOrientation */
            (void)GetNextWord(file);             /* etmMasterHeight */
            (void)GetNextWord(file);             /* etmMinScale */
            (void)GetNextWord(file);             /* etmMaxScale */
            (void)GetNextWord(file);             /* etmMasterUnits */
            (void)GetNextWord(file);             /* etmCapHeight */
            (void)GetNextWord(file);             /* etmXHeight */
            (void)GetNextWord(file);             /* etmLowerCaseAscent */
            t1m->descent = GetNextWord(file);    /* etmLowerCaseDecent */
            (void)GetNextWord(file);             /* etmSlant */
            t1m->superoff = GetNextWord(file);   /* etmSuperScript */
            t1m->suboff = GetNextWord(file);     /* etmSubScript */
            t1m->supersize = GetNextWord(file);  /* etmSuperScriptSize */
            t1m->subsize = GetNextWord(file);    /* etmSubScriptSize */
            (void)GetNextWord(file);             /* etmUnderlineOffset */
            (void)GetNextWord(file);             /* etmUnderlineWidth */
            (void)GetNextWord(file);             /* etmDoubleUpperUnderlineOffset*/
            (void)GetNextWord(file);             /* etmDoubleLowerUnderlineOffset*/
            (void)GetNextWord(file);             /* etmDoubleUpperUnderlineWidth */
            (void)GetNextWord(file);             /* etmDoubleLowerUnderlineWidth */
            t1m->strikeoff = GetNextWord(file);  /* etmStrikeOutOffset */
            t1m->strikesize = GetNextWord(file); /* etmStrikeOutWidth */
            (void)GetNextWord(file);             /* etmNKernPairs */
            (void)GetNextWord(file);             /* etmNKernTracks */

            /* Get the advance width for the characters. */
            if ((t1m->widths = Malloc(sizeof(funit)*
                                      (t1m->lastChar -
                                       t1m->firstChar + 1)))==NULL) {
               SetError(status=NOMEM);
            } else {
               (void)io_FileSeek(file, widthoffset);
               for (i=0; i<=t1m->lastChar-t1m->firstChar; i++)
                  t1m->widths[i] = GetNextWord(file);

               if (io_FileError(file)!=SUCCESS) {
                  SetError(status = BADMETRICS);
               }
            }

            /* Get the face name. */
            if ((status==SUCCESS) && faceoffset) {
               (void)io_FileSeek(file, faceoffset);
               if (t1m->family)
                  Free(t1m->family);
               ReadString(buf, sizeof(buf), file);
               if (io_FileError(file)) {
                  SetError(status = BADMETRICS);
               } else {
                  if ((t1m->family = Strdup((char*)buf))==NULL) {
                     SetError(status=NOMEM);
                  }
               }
            }

            /* Get the pair-kerning the typeface. */
            if ((status==SUCCESS) && kernoffset) {
               (void)io_FileSeek(file, kernoffset);
               t1m->kernsize = (USHORT)GetNextWord(file);
               if (io_FileError(file)!=SUCCESS) {
                  SetError(status = BADMETRICS);
               } else {
                  if ((t1m->kerns = Malloc(sizeof(struct kerning)*
                                            t1m->kernsize))==NULL) {
                     SetError(status=NOMEM);
                  } else {
                     for (i=0; i<(int)t1m->kernsize; i++) {
                        t1m->kerns[i].left = (UBYTE)io_ReadOneByte(file);
                        t1m->kerns[i].right = (UBYTE)io_ReadOneByte(file);
                        t1m->kerns[i].delta = GetNextWord(file);
                     }

                     if (io_FileError(file)!=SUCCESS) {
                        SetError(status = BADMETRICS);
                     }
                  }
               }
            }
         }
      }

      if (io_CloseFile(file)!=SUCCESS)
         status = BADMETRICS;
   }

   return status;
}
