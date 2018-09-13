/***
**
**   Module: T1Parser
**
**   Description:
**      This is a module of the T1 to TT font converter. The module
**      will extract information from a T1 font file, by parsing
**      the data/commands found in PFB, PFM and AFM files.
**
**   Author: Michael Jansson
**
**   Created: 5/26/93
**
***/


/**** INCLUDES */
/* General types and definitions. */
#include <string.h>
#include "types.h"

/* Special types and definitions. */
#include "safemem.h"
#include "encoding.h"
#include "metrics.h"
#include "t1msg.h"

/* Module dependent types and prototypes. */
#include "titott.h"
#include "t1parser.h"
#include "charstr.h"
#include "freader.h"
#include "mreader.h"


/***** CONSTANTS */
#define ONE       (USHORT)1
#define BUFLEN    (USHORT)512

#define PS_ANGLE              "/ItalicAngle"
#define PS_ARRAY              "array"
#define PS_BEGIN              "begin"
#define PS_BLUEFUZZ           "/BlueFuzz"
#define PS_BLUESCALE          "/BlueScale"
#define PS_BLUESHIFT          "/BlueShift"
#define PS_BLUEVALUES         "/BlueValues"
#define PS_CHARSTRINGS        "/CharStrings"
#define PS_COPYRIGHT          "/Copyright"
#define PS_DATE               "%%CreationDate:"
#define PS_DUP                "dup"
#define PS_ENCODING           "/Encoding"
#define PS_END                "end"
#define PS_FAMILY             "/FamilyName"
#define PS_FAMILYBLUES        "/FamilyBlues"
#define PS_FAMILYOTHERBLUES   "/FamilyOtherBlues"
#define PS_FONTMATRIX         "/FontMatrix"
#define PS_FORCEBOLD          "/ForceBold"
#define PS_FULLNAME           "/FullName"
#define PS_HYBRID             "hires"
#define PS_ISFIXED            "/isFixedPitch"
#define PS_LENIV              "/lenIV"
#define PS_NAME               "/FontName"
#define PS_NOACCESS           "noaccess"
#define PS_NOTICE             "/Notice"
#define PS_OTHERBLUES         "/OtherBlues"
#define PS_SNAPH              "/StemSnapH"
#define PS_SNAPV              "/StemSnapV"
#define PS_STDENCODING        "StandardEncoding"
#define PS_STDVW              "/StdVW"
#define PS_STDHW              "/StdHW"
#define PS_SUBRS              "/Subrs"
#define PS_UNDERLINE          "/UnderlinePosition"
#define PS_UTHICK             "/UnderlineThickness"
#define PS_ID                 "/UniqueID"
#define PS_VERSION            "/version"
#define PS_WEIGHT             "/Weight"



/***** LOCAL TYPES */

struct T1Handle {
   struct FontFile *ff;

   struct PSState *ps;

   struct Subrs stdenc[256];

   USHORT numsubrs;
   struct Subrs *subrs;
   USHORT leniv;
   struct T1Metrics t1m;
};


/***** MACROS */
/*-none-*/


/***** PROTOTYPES */
/*-none-*/

/***** STATIC FUNCTIONS */


/***
** Function: StrToFix
**
** Description:
**   This is a "strtod" function, that converts from
**   ascii to fixpoint numbers.
***/
static long StrToFix(char *str, char **out, const long base)
{
   char *fstr;
   long num = 0, frac = 0, exp = 0;

   if (out)
      (*out) = str;

   /* Skip white space. */
   while (*str && (*str==' ' || *str=='\t'))
      str++;

   /* A number? */
   if (*str && ((*str>='0' && *str<='9') || *str=='-') || *str=='.') {

      num = atoi(str)*base;

      /* Fraction? */
      fstr = strchr(str, '.');
      if (fstr!=NULL && (strchr(str, ' ')==NULL || fstr<strchr(str, ' '))) {

         do {
            fstr++;
         } while (*fstr>='0' && *fstr<='9');

         /* Exponent? */
         if (*fstr=='E')
            exp = atoi(fstr+1);
         else
            exp = 0;

         fstr--;
         while (*fstr!='.') {
            frac += ((*fstr)-'0')*base;
            frac /= 10;
            fstr--;
         }
         if (num<0)
            num -= frac;
         else
            num += frac;

         /* Handle exponent. */
         if (exp>0) {
            do {
               num *= 10;
            } while (--exp);
         } else if (exp<0) {
            do {
               num /= 10;
            } while (++exp);
         }
      }

      /* Skip digits. */
      while (*str && ((*str>='0' && *str<='9') ||
                      *str=='.' || *str=='-' || *str=='E'))
         str++;

      if (out)
         (*out) = str;
   }

   return num;
}



/***
** Function: FreeT1Composite
**
** Description:
**   This function frees the memory used to represent
**   a composite acented T1 glyph.
***/
static void FreeT1Composite(Composite *comp)
{
   if (comp) {
      if (comp->cchar)
         Free(comp->cchar);
      Free(comp);
   }
}


/***
** Function: UseGlyph
**
** Description:
**   This function determines whether a glyph should be
**   converted or not, based on the name of the glyph
**   and a specification of the desired glyphs.
***/
static int CDECL compare(const void *arg1, const void *arg2)
{
	return strcmp( *((const char **)arg1), *((const char **)arg2) );
}
static boolean UseGlyph(const struct GlyphFilter *filter,
                        Composite *comp,
                        const char *name)
{
   boolean found = FALSE;
   char **result;

   /* Check if the glyph is explicitly specified. */
   if (filter) {

      result = (char **)bsearch((char *)&name,
                                (char *)filter->name, filter->num,
                                sizeof(char *),
                                compare);


      found = (boolean)(result!=NULL);

      /* Check if the glyph is specified indirectly through an accented */
      /* composite glyph. */
      if (!found) {
         Composite *c;

         for (c=comp; c &&
                strcmp(name, c->achar) &&
                strcmp(name, c->bchar); c = c->next);
         found = (boolean)(c!=NULL);
      }
   } else {
      found = TRUE;
   }

   return found;
}



/***
** Function: ReadFontMatrix
**
** Description:
**   Read the command sequence "/FontMatrix[%d %d %d %d]" and
**   record the transformation matrix in the T1 handle.
***/
static errcode ReadFontMatrix(struct T1Handle *t1,
                              char *str,
                              const USHORT len)
{
   errcode status=SUCCESS;
   f16d16 fmatrix[6];
   USHORT i;

   if (GetSeq(t1->ff, str, len)) {
      for (i=0; i<6; i++)
         fmatrix[i] = StrToFix(str, &str, F16D16BASE);

      /* Check if we have the default matrix. */ /*lint -e771 */
      if (fmatrix[2]!=0 ||
          fmatrix[4]!=0 ||
          fmatrix[1]!=0 ||
          fmatrix[5]!=0 ||
          fmatrix[0]!=F16D16PPM ||
          fmatrix[3]!=F16D16PPM ||
          t1->t1m.upem!=2048) {  /*lint +e771 */ /* fmatrix[] IS initialized */

          if ((t1->t1m.fmatrix = Malloc(sizeof(f16d16)*6))==NULL) {
              SetError(status = NOMEM);
          } else {
            t1->t1m.fmatrix[0] = fmatrix[0];
            t1->t1m.fmatrix[1] = fmatrix[1];
            t1->t1m.fmatrix[2] = fmatrix[2];
            t1->t1m.fmatrix[3] = fmatrix[3];
            t1->t1m.fmatrix[4] = fmatrix[4];
            t1->t1m.fmatrix[5] = fmatrix[5];
         }
      } else {
         t1->t1m.fmatrix = NULL;
      }
   } else {
      SetError(status = BADINPUTFILE);
   }

   return status;
}


/***
** Function: ReadEncodingArray
**
** Description:
**   Read the command sequence "/Encoding %d array ..." and
**   build an encoding table, or read "/Encoding StdEncoding def"
**   and used the standard encoding table.
***/
static errcode ReadEncodingArray(struct T1Handle *t1,
                                 char *str,
                                 const USHORT len)
{
   errcode status = SUCCESS;
   USHORT codes[ENC_MAXCODES];
   char *glyph_name = NULL;
   USHORT i, index;

   if (Get_Token(t1->ff, str, len)==NULL) {
      SetError(status = BADINPUTFILE);
   } else {
      if (strcmp(str, PS_STDENCODING) &&
          ((t1->t1m.encSize=(USHORT)atoi(str))!=0)) {
         if ((t1->t1m.encoding = AllocEncodingTable(t1->t1m.encSize))==NULL) {
            SetError(status = NOMEM);
         } else {

            /* Skip leading proc. */
            while (Get_Token(t1->ff, str, len) && strcmp(str, PS_DUP));

            /* Read the encoding entries: "<n> <str> put <comment>\n dup" */
            for (i=0; i<t1->t1m.encSize; i++) {

               /* Get character code. */
               (void)Get_Token(t1->ff, str, len);
               if (str[0]=='8' && str[1]=='#') {   /* Octal? */
                  index = (USHORT)atoi(&str[2]);
                  index = (USHORT)((index/10)*8 + (index%8));
               } else {
                  index = (USHORT)atoi(str);
               }

               /* Get character name. */
               (void)Get_Token(t1->ff, str, len);

               codes[ENC_MSWINDOWS] = index;
               codes[ENC_UNICODE] = index;

               if (index<256) {
                  codes[ENC_STANDARD] = index;
                  codes[ENC_MACCODES] = index;
               } else {
                  codes[ENC_STANDARD] = NOTDEFCODE;
                  codes[ENC_MACCODES] = NOTDEFCODE;
               }
               if ((glyph_name = Strdup(&str[1]))!=NULL)
                  SetEncodingEntry(t1->t1m.encoding, i,
                                   glyph_name,
                                   ENC_MAXCODES,
                                   codes);
               else {
                  status = NOMEM;
                  break;
               }
               
               (void)Get_Token(t1->ff, str, len);   /* Pop "dup" */
               (void)Get_Token(t1->ff, str, len);   /* Pop "put" or comment. */
               if (str[0]=='%') {
                  (void)GetNewLine(t1->ff, str, len);
                  (void)Get_Token(t1->ff, str, len);   /* Pop "put". */
               }

               if (strcmp(str, PS_DUP))
                  break;
            }
            t1->t1m.encSize = (USHORT)(i+1);

            /* Rehash the table. */
            RehashEncodingTable(t1->t1m.encoding, t1->t1m.encSize);
         }
      }
   }

   return status;
}


/***
** Function: ReadArray
**
** Description:
**   Read an array.
***/
static errcode ReadArray(struct T1Handle *t1,
                         char *str,
                         const USHORT len,
                         funit *array,
                         USHORT maxarr,
                         USHORT *cnt)
{
   errcode status;
   char *nxt;

   if (GetSeq(t1->ff, str, len)) {
      (*cnt)=0;
      do {
         array[(*cnt)] = (funit)(((StrToFix(str, &nxt, 4L)+8002)>>2) - 2000);
         if (nxt==str)
            break;
         str = nxt;
      } while (++(*cnt)<maxarr);
      status=SUCCESS;
   } else {
      SetError(status = BADINPUTFILE);
   }

   return status;
}



/***
** Function: ReadFontSubrs
**
** Description:
**   Read the command sequence "/Subrs %d array dup %d %d RD %x ND ...",
**   decode and decrypt the subroutines and store them in the T1
**   handle.
***/
static errcode ReadFontSubrs(struct T1Handle *t1,
                             char *str, const USHORT len)
{
   errcode status = SUCCESS;
   USHORT index,i,j;
   USHORT count = 0;
   USHORT r;
   short b;

   /* Get the number of subroutines. */
   if (Get_Token(t1->ff, str, len)==NULL) {
      SetError(status = BADINPUTFILE);
   } else {
      count = (USHORT)atoi(str);

      /* Get the "array" keyword". */
      if ((Get_Token(t1->ff, str, len)==NULL) || strcmp(str, PS_ARRAY)) {
         SetError(status = BADINPUTFILE);
      } else {
         if ((t1->subrs = Malloc((USHORT)sizeof(struct Subrs)*count))==NULL) {
            SetError(status = NOMEM);
         } else {
            memset(t1->subrs, '\0', sizeof(struct Subrs)*count);
            t1->numsubrs = count;
            for (i=0; i<count; i++) {

               if (Get_Token(t1->ff, str, len)==NULL) {  /* Get "dup" */
                  SetError(status = BADINPUTFILE);
                  break;
               }
               if (strcmp(str, PS_DUP)) {
                  SetError(status = BADT1HEADER);
                  break;
               }

               if (Get_Token(t1->ff, str, len)==NULL) { /* Get Subr index. */
                  SetError(status=BADINPUTFILE);
                  break;
               }
               index = (USHORT)atoi(str);
               if (t1->subrs[index].code) {
                  LogError(MSG_WARNING, MSG_DBLIDX, NULL);
                  Free(t1->subrs[index].code);
               }

               if (Get_Token(t1->ff, str, len)==NULL) { /* Get length. */
                  SetError(status=BADINPUTFILE);
                  break;
               }
               t1->subrs[index].len = (USHORT)(atoi(str) - t1->leniv);
               if ((t1->subrs[index].code
                    = Malloc(t1->subrs[index].len))==NULL) {
                  SetError(status = NOMEM);
                  break;
               }

               if (Get_Token(t1->ff, str, len)==NULL) { /* Get RD + space */
                  SetError(status=BADINPUTFILE);
                  break;
               }
               /* Skip space. */
               (void)GetByte(t1->ff);

               /* Skip lenIV */
               r = 4330;
               for (j=0; j<t1->leniv; j++) {
                  b=GetByte(t1->ff);
                  (void)Decrypt(&r, (UBYTE)b);
               }
               if (status!=SUCCESS)
                  break;

               /* Get code. */
               for (j=0; j<t1->subrs[index].len; j++) {
                  b=GetByte(t1->ff);
                  t1->subrs[index].code[j] = Decrypt(&r, (UBYTE)b);
               }
               if (status!=SUCCESS)
                  break;

               if (Get_Token(t1->ff, str, len)==NULL) { /* Get ND */
                  SetError(status=BADINPUTFILE);
                  break;
               }
               /* Check for non-ATM compatible equivalent to 'ND' */
               if (!strcmp(str, PS_NOACCESS)) {
                  (void)Get_Token(t1->ff, str, len);
               }

            }
         }
      }
   }

   return status;
}





/***** FUNCTIONS */


/***
** Function: FlushWorkspace
**
** Description:
**   Free the resources allocated for the T1 handle.
***/
void FlushWorkspace(struct T1Handle *t1)
{
   USHORT i;

   /* Free /Subrs */
   if (t1->subrs) {
      for (i=0; i<t1->numsubrs; i++) {
         Free(t1->subrs[i].code);
      }
      Free(t1->subrs);
   }
   t1->subrs = NULL;
}   


/***
** Function: CleanUpT1
**
** Description:
**   Free the resources allocated for the T1 handle.
***/
errcode CleanUpT1(struct T1Handle *t1)
{
   errcode status = SUCCESS;
   AlignmentControl *align;
   Composite *next;
   Blues *blues;
   USHORT i;

   if (t1) {

      /* Free the PSState */
      if (t1->ps)
         FreePSState(t1->ps);

      /* Free /Subrs */
      if (t1->subrs) {
         for (i=0; i<t1->numsubrs; i++) {
            Free(t1->subrs[i].code);
         }
         Free(t1->subrs);
      }

      /* Clean up font file reader. */
      status = FRCleanUp(t1->ff);

      /* Clean up font matrix. */
      if (t1->t1m.fmatrix)
         Free(t1->t1m.fmatrix);

      /* Clean up seac. */
      while (t1->t1m.used_seac) {
         next = t1->t1m.used_seac->next;
         FreeT1Composite(t1->t1m.used_seac);
         t1->t1m.used_seac = next;
      }
      while (t1->t1m.seac) {
         next = t1->t1m.seac->next;
         FreeT1Composite(t1->t1m.seac);
         t1->t1m.seac = next;
      }

      /* Clean up stdenc. */
      for (i=0; i<256; i++) {
         if (t1->stdenc[i].code) {
            Free(t1->stdenc[i].code);
            t1->stdenc[i].code = NULL;
            t1->stdenc[i].len = 0;
         }
      }

      /* Clean up encoding table. */
      if (t1->t1m.encoding)
         FreeEncoding(t1->t1m.encoding, t1->t1m.encSize);

      /* Free strings */
      if (t1->t1m.date)
         Free(t1->t1m.date);
      if (t1->t1m.copyright)
         Free(t1->t1m.copyright);
      if (t1->t1m.name)
         Free(t1->t1m.name);
      if (t1->t1m.id)
         Free(t1->t1m.id);
      if (t1->t1m.notice)
         Free(t1->t1m.notice);
      if (t1->t1m.fullname)
         Free(t1->t1m.fullname);
      if (t1->t1m.weight)
         Free(t1->t1m.weight);
      if (t1->t1m.family)
         Free(t1->t1m.family);
      if (t1->t1m.widths)
         Free(t1->t1m.widths);
                if (t1->t1m.kerns)
                        Free(t1->t1m.kerns);
      if (t1->t1m.stems.vwidths)
         Free(t1->t1m.stems.vwidths);
      if (t1->t1m.stems.hwidths)
         Free(t1->t1m.stems.hwidths);
      blues = &(t1->t1m.blues);
      align = &(t1->t1m.blues.align);
      for (i=0; i<blues->blue_cnt/2; i++) {
         Free(align->top[i].pos);
      }
      for (i=0; i<blues->oblue_cnt/2; i++) {
         Free(align->bottom[i].pos);
      }

      /* Free handle. */
      Free(t1);
   }

   return status;
}



/***
** Function: InitT1Input
**
** Description:
**   Allocate and initiate a handle for a T1 font file, including
**   extracting data from the font prolog that is needed to
**   read the glyphs, such as /FontMatrix, /Subrs and /lenIV.
***/
errcode InitT1Input(const struct T1Arg *arg,
                    struct T1Handle **t1ref,
                    struct T1Metrics **t1mref,
                    const short (*check)(const char *,
                                         const char *,
                                         const char *))
{
   errcode status = SUCCESS;
   struct T1Handle *t1;
   struct PSState *ps;
   Blues *blues;
   boolean hybrid = FALSE;
   struct T1Metrics *t1m = NULL;
   char str[BUFLEN];
   USHORT i;

   /* Allocate the handle. */
   if (((*t1ref)=Malloc((USHORT)sizeof(struct T1Handle)))==NULL ||
       (ps = AllocPSState())==NULL) {
      if ((*t1ref))
         Free((*t1ref));
      SetError(status = NOMEM);
   } else {

      /* Initiate the T1 record. */
      t1 = (*t1ref);
      t1m = &t1->t1m;
      (*t1mref) = t1m;
      blues = GetBlues(t1m);
      memset(t1, '\0', sizeof(*t1));
      t1->ps = ps;
      t1->leniv = 4;
      t1m->upem = arg->upem;
      t1m->defstdhw = 70;
      t1m->defstdvw = 80;

      blues->blueScale = 39;   /* Should really be 39.625 */
      blues->blueFuzz = 1;
      blues->blueShift = 7 * F8D8;
      blues->align.cvt = 3;
      t1m->stems.storage = 15;

      /* Initiate font file reader. */
      if ((status=FRInit(arg->name, pfb_file, &t1->ff))==SUCCESS) {

         /* Read /FontMatrix and /Subrs. */
         while (status==SUCCESS) {
            if (Get_Token(t1->ff, str, BUFLEN)==NULL) {
               SetError(status=BADINPUTFILE);

               /**** /ForceBold true def ****/
            } else if (!strcmp(str, PS_FORCEBOLD)) {
               if (Get_Token(t1->ff, str, BUFLEN)) {
                  if (!strcmp(str, "true") || !strcmp(str, "True"))
                     t1m->forcebold = TRUE;
                  else
                     t1m->forcebold = FALSE;
                  status = SUCCESS;
               } else {
                  status = BADINPUTFILE;
               }

               /**** /BlueFuzz 1 def ****/
            } else if (!strcmp(str, PS_BLUEFUZZ)) {
               if (Get_Token(t1->ff, str, BUFLEN)) {
                  blues->blueFuzz = (UBYTE)atoi(str);
                  status = SUCCESS;
               } else {
                  status = BADINPUTFILE;
               }

               /**** /BlueScale 0.043625 def ****/
            } else if (!strcmp(str, PS_BLUESCALE)) {
               if (Get_Token(t1->ff, str, BUFLEN)) {
                  str[5] = '\0';
                  blues->blueScale = (UBYTE)atoi(&str[2]);
                  status = SUCCESS;
               } else {
                  status = BADINPUTFILE;
               }

               /**** /BlueShift 7 def ****/
            } else if (!strcmp(str, PS_BLUESHIFT)) {
               if (Get_Token(t1->ff, str, BUFLEN)) {
                  blues->blueShift = (short)StrToFix(str, NULL, (long)F8D8);
                  status = SUCCESS;
               } else {
                  status = BADINPUTFILE;
               }

               /**** /Encoding StandardEncodind def ****/
            } else if (!strcmp(str, PS_ENCODING)) {
               status = ReadEncodingArray(t1, str, BUFLEN);

               /**** /StdVW [118] def ****/
            } else if (!strcmp(str, PS_STDVW)) {
               USHORT dummy;
               status = ReadArray(t1, str, BUFLEN,
                                  &t1m->stdvw, ONE, &dummy);


               /**** /StdHW [118] def ****/
            } else if (!strcmp(str, PS_STDHW)) {
               USHORT dummy;
               status = ReadArray(t1, str, BUFLEN,
                                  &t1m->stdhw, ONE, &dummy);

               /**** /StemSnapV [118 120] def ****/
            } else if (!strcmp(str, PS_SNAPV)) {
               status = ReadArray(t1, str, BUFLEN,
                                  &t1m->stemsnapv[0],
                                  MAXSNAP, &t1m->snapv_cnt);

               /* Add space for the snap enties in the CV table. */
               if (status==SUCCESS)
                  blues->align.cvt = (USHORT)(blues->align.cvt +
                                             t1m->snapv_cnt);

               /**** /StemSnapH [118 120] def ****/
            } else if (!strcmp(str, PS_SNAPH)) {
               status = ReadArray(t1, str, BUFLEN,
                                  &t1m->stemsnaph[0],
                                  MAXSNAP, &t1m->snaph_cnt);

               /* Add space for the snap enties in the CV table. */
               if (status==SUCCESS)
                  blues->align.cvt = (USHORT)(blues->align.cvt +
                                              t1m->snaph_cnt);

               /**** /BlueValues [-15 0] def ****/
            } else if (!strcmp(str, PS_BLUEVALUES)) {
               status = ReadArray(t1, str, BUFLEN,
                                  &(blues->bluevalues[0]),
                                  MAXBLUE, &(blues->blue_cnt));
               if (blues->blue_cnt%2)
                  SetError(status = BADINPUTFILE);

               /**** /OtherBlues [-15 0] def ****/
            } else if (!strcmp(str, PS_OTHERBLUES)) {
               status = ReadArray(t1, str, BUFLEN,
                                  &(blues->otherblues[0]),
                                  MAXBLUE, &(blues->oblue_cnt));
               if (blues->oblue_cnt%2)
                  SetError(status = BADINPUTFILE);

               /**** /FamilyBlues [-15 0] def ****/
            } else if (!strcmp(str, PS_FAMILYBLUES)) {
               status = ReadArray(t1, str, BUFLEN,
                                  &(blues->familyblues[0]),
                                  MAXBLUE, &(blues->fblue_cnt));

               /**** /FamilyOtherBlues [-15 0] def ****/
            } else if (!strcmp(str, PS_FAMILYOTHERBLUES)) {
               status = ReadArray(t1, str, BUFLEN,
                                  &(blues->familyotherblues[0]),
                                  MAXBLUE, &(blues->foblue_cnt));

               /**** /CharString ... */
            } else if (!strcmp(str, PS_CHARSTRINGS)) {
               break;

               /**** /FontMatrix [0 0.001 0 0.001 0] def ****/
            } else if (GetFontMatrix(t1m)==NULL &&
                       !strcmp(str, PS_FONTMATRIX)) {
               status = ReadFontMatrix(t1, str, BUFLEN);
            } else if (!strcmp(str, PS_SUBRS)) {
               /* Discard prior lores /Subrs. */
               FlushWorkspace(t1);

               /* Read new subrs. */
               status = ReadFontSubrs(t1,str, BUFLEN);

               /**** /lenIV 4 def ****/
            } else if (!strcmp(str, PS_LENIV)) {
               if (Get_Token(t1->ff, str, BUFLEN)) {
                  t1->leniv = (USHORT)atoi(str);
                  status = SUCCESS;
               } else {
                  status = BADINPUTFILE;
               }
            } else if (t1m->date==NULL && !strcmp(str, PS_DATE)) {
               if ((GetNewLine(t1->ff, str, BUFLEN))==NULL) {
                  SetError(status = BADINPUTFILE);
               } else if ((t1m->date=Strdup(str))==NULL) {
                  SetError(status = NOMEM);
               }
            } else if (t1m->copyright==NULL &&
                       !strcmp(str, PS_COPYRIGHT)) {
               if ((GetSeq(t1->ff, str, BUFLEN))==NULL) {
                  SetError(status = BADINPUTFILE);
               } else if ((t1m->copyright=Strdup(str))==NULL) {
                  SetError(status = NOMEM);
               }
            } else if (t1m->name==NULL && !strcmp(str, PS_NAME)) {
               if ((Get_Token(t1->ff, str, BUFLEN))==NULL) {
                  SetError(status = BADINPUTFILE);
               } else if ((t1m->name=Strdup(&str[1]))==NULL) {
                  SetError(status = NOMEM);
               }
            } else if (t1m->id==NULL && !strcmp(str, PS_ID)) {
               if ((Get_Token(t1->ff, str, BUFLEN))==NULL) {
                  SetError(status = BADINPUTFILE);
               } else if ((t1m->id=Strdup(str))==NULL) {
                  SetError(status = NOMEM);
               }
            } else if (t1m->version.ver==0 && !strcmp(str, PS_VERSION)) {
               if ((GetSeq(t1->ff, str, BUFLEN))==NULL) {
                  SetError(status = BADINPUTFILE);
               } else {
                  t1m->version.ver = (USHORT)atoi(str);
                  if (strchr(str, '.'))
                     t1m->version.rev = (USHORT)atoi(strchr(str, '.')+1);
                  else
                     t1m->version.rev = 0;
               }
            } else if (t1m->notice==NULL && !strcmp(str, PS_NOTICE)) {
               if ((GetSeq(t1->ff, str, BUFLEN))==NULL) {
                  SetError(status = BADINPUTFILE);
               } else if ((t1m->notice=Strdup(str))==NULL) {
                  SetError(status = NOMEM);
               }
            } else if (t1m->fullname==NULL && !strcmp(str, PS_FULLNAME)) {
               if ((GetSeq(t1->ff, str, BUFLEN))==NULL) {
                  SetError(status = BADINPUTFILE);
               } else if ((t1m->fullname=Strdup(str))==NULL) {
                  SetError(status = NOMEM);
               }
            } else if (t1m->family==NULL && !strcmp(str, PS_FAMILY)) {
               if ((GetSeq(t1->ff, str, BUFLEN))==NULL) {
                  SetError(status = BADINPUTFILE);
               } else if ((t1m->family=Strdup(str))==NULL) {
                  SetError(status = NOMEM);
               }
            } else if (t1m->weight==NULL && !strcmp(str, PS_WEIGHT)) {
               if ((GetSeq(t1->ff, str, BUFLEN))==NULL) {
                  SetError(status = BADINPUTFILE);
               } else if ((t1m->weight=Strdup(str))==NULL) {
                  SetError(status = NOMEM);
               }
            } else if (t1m->angle==0 && !strcmp(str, PS_ANGLE)) {
               if ((Get_Token(t1->ff, str, BUFLEN))==NULL) {
                  SetError(status = BADINPUTFILE);
               } else 
                  t1m->angle = StrToFix(str, NULL, F16D16BASE);
            } else if (t1m->underline==0 && !strcmp(str, PS_UNDERLINE)) {
               if ((Get_Token(t1->ff, str, BUFLEN))==NULL) {
                  SetError(status = BADINPUTFILE);
               } else
                  t1m->underline = (funit)StrToFix(str, NULL, 1L);
            } else if (t1m->uthick==0 && !strcmp(str, PS_UTHICK)) {
               if ((Get_Token(t1->ff, str, BUFLEN))==NULL) {
                  SetError(status = BADINPUTFILE);
               } else
                  t1m->uthick = (funit)StrToFix(str, NULL, 1L);
            } else if (!strcmp(str, PS_ISFIXED)) {
               if ((Get_Token(t1->ff, str, BUFLEN))==NULL) {
                  SetError(status = BADINPUTFILE);
               } else
                  if (!strcmp(str, "true") ||
                      !strcmp(str, "True") ||
                      !strcmp(str, "TRUE"))
                     t1m->fixedPitch = TRUE;
            } else if (!strcmp(str, PS_HYBRID)) {
               hybrid = TRUE;
            }
         }

         /* Change the baseline zone into an OtherBlues[] zone. */
         if (blues->blue_cnt) {
            blues->otherblues[blues->oblue_cnt++] = blues->bluevalues[0];
            blues->otherblues[blues->oblue_cnt++] = blues->bluevalues[1];
            for (i=2; i<blues->blue_cnt; i++)
               blues->bluevalues[i-2] = blues->bluevalues[i];
            blues->blue_cnt -= 2;
         }
         if (blues->fblue_cnt) {
            blues->familyotherblues[blues->foblue_cnt++]
                  = blues->familyblues[0];
            blues->familyotherblues[blues->foblue_cnt++]
                  = blues->familyblues[1];
            for (i=2; i<blues->fblue_cnt; i++)
               blues->familyblues[i-2] = blues->familyblues[i];
            blues->fblue_cnt -= 2;
         }

         /* Allocate the space for the blue buckets. */
         for (i=0; i<blues->blue_cnt; i+=2) {
            USHORT size = (USHORT)((ABS(blues->bluevalues[i+1] -
                                        blues->bluevalues[i]) +
                                    1 + 2*blues->blueFuzz)*
                                   (USHORT)sizeof(struct CVTPos));
            if ((blues->align.top[i/2].pos = Malloc(size))==NULL) {
               SetError(status = NOMEM);
               break;
            }

            /* Make sure that first value is larger than second value. */
            if (blues->bluevalues[i] > blues->bluevalues[i+1]) {
               LogError(MSG_WARNING, MSG_INVBLUES, NULL);
               SWAPINT(blues->bluevalues[i], blues->bluevalues[i+1]);
            }
         }
         for (i=0; i<blues->oblue_cnt; i+=2) {
            USHORT size = (USHORT)((ABS(blues->otherblues[i+1] -
                                        blues->otherblues[i]) +
                                    1 + 2*blues->blueFuzz)*
                                   (USHORT)sizeof(struct CVTPos));
            if ((blues->align.bottom[i/2].pos = Malloc(size))==NULL) {
               SetError(status = NOMEM);
               break;
            }

            /* Make sure that first value is larger than second value. */
            if (blues->otherblues[i] > blues->otherblues[i+1]) {
               LogError(MSG_WARNING, MSG_INVBLUES, NULL);
               SWAPINT(blues->otherblues[i], blues->otherblues[i+1]);
            }
         }


         /* Advance to the first glyph. */
         if (status==SUCCESS) {
            while (Get_Token(t1->ff, str, BUFLEN) &&
                   strcmp(str, PS_BEGIN));

            if (strcmp(str, PS_BEGIN)) {
               SetError(status = BADT1HEADER);
            }

            /* Skip lores chars if hybrid font. */
            if (status==SUCCESS && hybrid) {
               USHORT count;

               /* Skip Charstring dictionary. */
               do {
                  /* Glyph name, or end. */
                  if (Get_Token(t1->ff, str, BUFLEN)==NULL) {
                     SetError(status = BADINPUTFILE);
                     break;
                  }
                  if (!strcmp(str, PS_END))
                     break;

                  /* Charstring length. */
                  if (Get_Token(t1->ff, str, BUFLEN)==NULL) {
                     SetError(status = BADINPUTFILE);
                     break;
                  }
                  count = (USHORT)(atoi(str)+1);

                  /* Delimiter. */
                  if (Get_Token(t1->ff, str, BUFLEN)==NULL) {
                     SetError(status = BADINPUTFILE);
                     break;
                  }

                  /* Charstring */
                  for (i=0; i<count; i++)
                     (void)GetByte(t1->ff);

                  /* Delimiter */
                  if (Get_Token(t1->ff, str, BUFLEN)==NULL) {
                     SetError(status = BADINPUTFILE);
                     break;
                  }
               } while (status==SUCCESS);

               /* Skip to the beginning of next charstring. */
               while (Get_Token(t1->ff, str, BUFLEN) &&
                      strcmp(str, PS_BEGIN));

               if (strcmp(str, PS_BEGIN)) {
                  SetError(status = BADT1HYBRID);
               }
            }
         }
      }
   }

   if ((status==SUCCESS) && t1m && check(t1m->name,
                                         t1m->copyright,
                                         t1m->notice)!=SUCCESS)
      status = NOCOPYRIGHT;

   return status;
}



/***
** Function: GetT1Glyph
**
** Description:
**   The current file position of the T1 font file must be
**   at the begining of an entry in the /CharStrings dictionary.
**   The function will decode the font commands, parse them, and
**   finally build a representation of the glyph.
***/
errcode GetT1Glyph(struct T1Handle *t1,
                   struct T1Glyph *glyph,
                   const struct GlyphFilter *filter)
{
   errcode status = SUCCESS;
   /* struct encoding *enc; */
   char  str[BUFLEN];
   UBYTE *code;
   USHORT len;
   USHORT i;
   USHORT r = 4330;   
   short b;

   /* Get glyph name or end. */
   if (Get_Token(t1->ff, str, BUFLEN)==NULL) {
      SetError(status = BADINPUTFILE);
   } else if (!strcmp(str, PS_END)) {
      status = DONE;
   } else if (str[0]!='/') {
      SetError(status = BADCHARSTRING);
   } else {
      if ((glyph->name = Strdup(&str[1]))==NULL) {
         SetError(status = NOMEM);
      } else {

         /* Get length of charstring. */
         (void)Get_Token(t1->ff, str, BUFLEN);
         len = (USHORT)atoi(str);

         /* Get RD + space */
         (void)Get_Token(t1->ff, str, BUFLEN);
         (void)GetByte(t1->ff);

         /* Get commands. */
         if (len<BUFLEN)
            code = (UBYTE *)str;
         else
            if ((code = Malloc(len*sizeof(UBYTE)))==NULL) {
               SetError(status = NOMEM);
            }

         if (code) {
            for (i=0; i<len; i++) {
               b = GetByte(t1->ff);
               code[i] = (UBYTE)Decrypt(&r, (UBYTE)b);
            }

            /* Parse commands. */
            if (status==SUCCESS) {
               if (t1->t1m.encoding!=NULL ||
                   UseGlyph(filter, t1->t1m.seac, glyph->name)) {
                  InitPS(t1->ps);
                  status = ParseCharString(glyph,
                                           &t1->t1m.seac,
                                           t1->ps,
                                           t1->subrs,
                                           &code[t1->leniv],
                                           (USHORT)(len-t1->leniv));

                  /* Skip normal conversion for the ".notdef" glyph. */
                  if (!strcmp(glyph->name, ".notdef"))
                     status = SKIP;

               } else {
                  status = SKIP;

               /***

               Two approaches are implemented for the management of
               composite glyphs:

               1) It is up to the client to specify a GlyphFilter such
               that all 'seac' characters has their dependent base and
               accent character in the filter as well.

               2) The converter manages a list of the dependent characters,
               which are converted when found.

               Approach 2) will typically cause the converter to use more
               memory than what is available in the small memory model,
               which is why the default is to disabled it.

               ***/


#if 0

                  /* Record StandardEncoding glyphs, for 'seac' */
                  if ((enc = LookupPSName(t1->t1m.encoding,
                                          t1->t1m.encSize,
                                          glyph->name)) &&
                      (i = LookupCharCode(enc, ENC_STANDARD))!=0) {
                     if ((t1->stdenc[i].code
                          = Malloc(len-t1->leniv))==NULL) {
                        SetError(status = NOMEM);
                     } else {
                        memcpy(t1->stdenc[i].code,
                               &code[t1->leniv],
                               sizeof(UBYTE) * (len - t1->leniv));
                        t1->stdenc[i].len = len - t1->leniv;
                     }
                  }
#endif
               }

               if (code!=(UBYTE *)str)
                  Free(code);

               /* Get ND */
               (void)Get_Token(t1->ff, str, BUFLEN);
            }
         }
      }
   }


   return status;
}



/***
** Function: FreeT1Glyph
**
** Description:
**   This function frees the memory used to represent
**   a glyph that has been translated.
***/
void FreeT1Glyph(T1Glyph *glyph)
{
   Flex *flex;
   Stem *stem;
   Stem3 *stem3;


   if (glyph->name)
      Free(glyph->name);
   while (glyph->hints.vstems) {
      stem = glyph->hints.vstems->next;
      Free(glyph->hints.vstems);
      glyph->hints.vstems = stem;
   }
   while (glyph->hints.hstems) {
      stem = glyph->hints.hstems->next;
      Free(glyph->hints.hstems);
      glyph->hints.hstems = stem;
   }
   while (glyph->hints.vstems3) {
      stem3 = glyph->hints.vstems3->next;
      Free(glyph->hints.vstems3);
      glyph->hints.vstems3 = stem3;
   }
   while (glyph->hints.hstems3) {
      stem3 = glyph->hints.hstems3->next;
      Free(glyph->hints.hstems3);
      glyph->hints.hstems3 = stem3;
   }
   while (glyph->hints.flex) {
      flex = glyph->hints.flex->next;
      Free(glyph->hints.flex);
      glyph->hints.flex = flex;
   }
   while (glyph->paths) {
      Outline *path = glyph->paths;
      glyph->paths = path->next;
      if (path->count) {
         Free(path->onoff);
         Free(path->pts);
      }
      Free(path);
   }
   memset((void *)glyph, '\0', sizeof(T1Glyph));
}



/***
** Function: GetT1Composite
**
** Description:
**   This function unlinks the first composite glyph
**   from the list of recorded composite glyphs, which
**   is returned to the caller.
***/
struct Composite  *GetT1Composite(struct T1Handle *t1)
{
   struct Composite *comp;

   comp = t1->t1m.seac;
   if (comp) {
      t1->t1m.seac = comp->next;
      comp->next = t1->t1m.used_seac;
      t1->t1m.used_seac = comp;
   }

   return comp;
}



/***
** Function: GetT1BaseGlyph
**
** Description:
**   This function parses the charstring code associated to the
**   base character of a composite character, if that glyph
**   is not already converted.
***/
errcode GetT1BaseGlyph(struct T1Handle *t1,
                       const struct Composite *comp,
                       struct T1Glyph *glyph)
{
   struct encoding *enc;
   struct Subrs *subr;
   errcode status = SUCCESS;

   if ((enc = LookupPSName(t1->t1m.encoding,
                           t1->t1m.encSize,
                           comp->bchar))==NULL) {
       LogError(MSG_WARNING, MSG_BADENC, comp->bchar);
       return SKIP;
   }

   subr = &t1->stdenc[LookupCharCode(enc, ENC_STANDARD)];

   if (subr->len==0) {
      status = SKIP; /* Missing or already done. */
   } else {
      InitPS(t1->ps);
      if ((glyph->name = Strdup((char*)comp->achar))==NULL) {
         SetError(status = NOMEM);
      } else {
         status = ParseCharString(glyph,
                                  &t1->t1m.seac,
                                  t1->ps,
                                  t1->subrs,
                                  subr->code,
                                  subr->len);
      }
      Free(subr->code);
      subr->code = NULL;
      subr->len = 0;
   }
   return status;
}



/***
** Function: GetT1AccentGlyph
**
** Description:
**   This function parses the charstring code associated to the
**   accent character of a composite character, if that glyph
**   is not already converted.
***/
errcode GetT1AccentGlyph(struct T1Handle *t1,
                         const struct Composite *comp,
                         struct T1Glyph *glyph)
{
   struct encoding *enc;
   struct Subrs *subr;
   errcode status = SUCCESS;

   if ((enc = LookupPSName(t1->t1m.encoding,
                           t1->t1m.encSize,
                           comp->achar))==NULL) {
       LogError(MSG_WARNING, MSG_BADENC, comp->achar);
       return SKIP;
   }

   subr = &t1->stdenc[LookupCharCode(enc, ENC_STANDARD)];

   if (subr->len==0) {
      status = SKIP; /* Missing or already done. */
   } else {
      InitPS(t1->ps);
      if ((glyph->name = Strdup((char *)comp->achar))==NULL) {
         SetError(status = NOMEM);
      } else {
         status = ParseCharString(glyph,
                                  &t1->t1m.seac,
                                  t1->ps,
                                  t1->subrs,
                                  subr->code,
                                  subr->len);
      }
      Free(subr->code);
      subr->code = NULL;
      subr->len = 0;
   }
   return status;
}



/***
** Function: ReadOtherMetrics
**
** Description:
**   Return font level information about the T1 font (mostly
**   metrics).
***/
errcode ReadOtherMetrics(struct T1Metrics *t1m,
                         const char *metrics)
{
   errcode status = SUCCESS;

   if ((status = ReadFontMetrics(metrics, t1m))==NOMETRICS) {
      t1m->flags = DEFAULTMETRICS;
      status = SUCCESS;
   } else {
      t1m->flags = USEMETRICS;
   }

   return status;
}
