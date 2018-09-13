/***
**
**   Module: FReader
**
**   Description:
**    This is a module of the T1 to TT font converter. The module
**    contains functions that decodes and decrypts the data of a
**    T1 font file.
**
**   Author: Michael Jansson
**
**   Created: 5/26/93
**
***/


/**** INCLUDES */
/* General types and definitions. */
#include <ctype.h>
#include <string.h>

/* Special types and definitions. */
#include "titott.h"
#include "types.h"
#include "safemem.h"
#include "t1msg.h"

/* Module dependent types and prototypes. */
#include "freader.h"
#include "pfb.h"


/***** LOCAL TYPES */
struct FontFile {

   /* Low-level I/O functions. */
   errcode (FASTCALL *fclose)(struct t1file *);
   short (FASTCALL *fgetc)(struct t1file *);
   struct t1file *(*fopen)(const char *);
   boolean (FASTCALL *fstatus)(const struct t1file *);
   struct t1file *io;

   /* Font file state. */
   enum {prolog, eexec} state;
   short nextbyte;
   USHORT r;
};


/***** CONSTANTS */
static const USHORT c1 = 52845;
static const USHORT c2 = 22719;


/***** MACROS */
#define IOGetByte(f)       ((*f->fgetc)(f->io))
#define IOError(f)         ((*f->fstatus)(f->io))
#define IOOpen(f,n)        ((*f->fopen)(n))
#define IOClose(f)         ((*f->fclose)(f->io))
#define SetNextByte(ff, b) ff->nextbyte = (b)
#define NextByte(ff)       (ff->nextbyte)
#define Eexec(ff)          (boolean)(ff->state == eexec)
#define StartEexec(ff)     ff->state = eexec



/***** STATIC FUNCTIONS */
/*-none-*/



/***** FUNCTIONS */

/***
** Function: GetByte
**
** Description:
**   Pull one byte out of the T1 font file.
***/
short FASTCALL GetByte(struct FontFile *ff)
{
   short b, nb;

   b = IOGetByte(ff);

   /* Decrypt it? */
   if (Eexec(ff))
      b = (short)Decrypt(&ff->r, (UBYTE)b);

   /* Record look-a-head */
   nb = NextByte(ff);
   SetNextByte(ff, b);

   return nb;
}



/***
** Function: GetNewLine
**
** Description:
**   Pull one whole line from the T1 font file, starting at
**   the current position.
***/
char *GetNewLine(struct FontFile *ff, char *buf, const USHORT len)
{
   short i = 0;

   /* Get string. */
   while ((buf[i] = (char)GetByte(ff))!='\n' &&
          buf[i]!='\r' && ++i<((short)len-1));

   /* Skip extra characters. */
   if (buf[i]!='\n' && buf[i]!='\r')
      while (!IOError(ff) && NextByte(ff)!='\n' && NextByte(ff)!='\r')
         (void)GetByte(ff);

   /* Terminate string. */
   buf[i] = '\0';

   /* Check for the start of the eexec section. */
   if (!strcmp(buf, "eexec"))
      StartEexec(ff);

   /* Check error condition. */
   if (IOError(ff))
      return NULL;

   return buf;
}



/***
** Function: Get_Token
**
** Description:
**   Pull one token from the T1 font file. A token 
**   is delimited by white space and various brackets.
***/
char *Get_Token(struct FontFile *ff, char *buf, const USHORT len)
{
   short i = 0;
   short nb;

   /* Skip leading blanks. */
   while (isspace(NextByte(ff)))
      (void)GetByte(ff);

   /* Get string. */
   do {
      buf[i] = (char)GetByte(ff);
      nb = NextByte(ff);
   } while (++i<((short)len-1) && !isspace(nb) && nb!='{' &&
            nb!='(' && nb!='[' && nb!='/');

   /* Skip extra characters. */
   while (!IOError(ff) && !isspace(nb) && nb!='{' &&
          nb!='(' && nb!='[' && nb!='/') {
      (void)GetByte(ff);
      nb = NextByte(ff);
   }

   /* Terminate string. */
   buf[i] = '\0';

   /* Check for the start of the eexec section. */
   if (!strcmp(buf, "eexec"))
      StartEexec(ff);

   /* Check error condition. */
   if (IOError(ff))
      return NULL;

   return buf;
}



/***
** Function: GetSeq
**
** Description:
**   Pull one sequence of bytes that are delimited by 
**   a given pair of characters, e.g. '[' and ']'.
***/
char *GetSeq(struct FontFile *ff,
             char *buf,
             const USHORT len)
{
   char d1, d2;
   short i = 0;
   short inside = 0;

   /* Skip leading blanks. */
   while (NextByte(ff)!='[' &&
          NextByte(ff)!='{' &&
          NextByte(ff)!='(' &&
          !IOError(ff))
      (void)GetByte(ff);

   /* match the bracket. */
   d1 = (char)NextByte(ff);
   if (d1=='[') 
      d2 = ']';
   else if (d1=='{')
      d2 = '}';
   else if (d1=='(')
      d2 = ')';
   else
      return NULL;


   /* Get string. */ 
   (void)GetByte(ff);
   inside=1;
   do {
      buf[i] = (char)GetByte(ff);
      if (buf[i]==d1)
         inside++;
      if (buf[i]==d2)
         inside--;
   } while (inside && ++i<((short)len-1));

   /* Terminate string. */
   buf[i] = '\0';

   /* Check error condition. */
   if (IOError(ff))
      return NULL;

   return buf;
}



/***
** Function: FRInit
**
** Description:
**   Initite the resources needed to read/decode data from
**   a T1 font file.
***/
errcode FRInit(const char *name, const enum ftype type, struct FontFile **ff)
{
   errcode status = SUCCESS;
   short b;

   if (((*ff)=(struct FontFile *)Malloc(sizeof(struct FontFile)))==NULL) {
      SetError(status = NOMEM);
   } else {

      /* Initiat the handle. */
      memset((*ff), '\0', sizeof(**ff));

      /* Initiate low-level I/O. */
      switch (type) {
         case pfb_file:
            (*ff)->fgetc = PFBGetByte;
            (*ff)->fclose = PFBFreeIOBlock;
            (*ff)->fstatus = PFBFileError;
            (*ff)->fopen = PFBAllocIOBlock;
            break;
         case mac_file:
#if MACFILEFORMAT
            (*ff)->fgetc = MACGetByte;
            (*ff)->fclose = MACFreeIOBlock;
            (*ff)->fstatus = MACFileError;
            (*ff)->fopen = MACAllocIOBlock;
            break;
#endif
         case ascii_file:
#if ASCIIFILEFORMAT
            (*ff)->fgetc = ASCIIGetByte;
            (*ff)->fclose = ASCIIFreeIOBlock;
            (*ff)->fstatus = ASCIFileError;
            (*ff)->fopen = ASCIIAllocIOBlock;
            break;
#endif
         default:
            LogError(MSG_ERROR, MSG_BADFMT, NULL);
            SetError(status = BADINPUTFILE);
            break;
      }

      (*ff)->io = NULL;
      if (((*ff)->io = IOOpen((*ff),name))==NULL) {
         SetError(status = BADINPUTFILE);
      } else {
         (*ff)->state = prolog;
         (*ff)->r = 55665;

         b=GetByte(*ff);
         SetNextByte((*ff), b);
      }
   }

   return status;
}



/***
** Function: FRCleanUp
**
** Description:
**   Free the resources used when reading/decoding data from
**   a T1 font file.
***/
errcode FRCleanUp(struct FontFile *ff)
{
   errcode status = SUCCESS;

   if (ff) {
      if (ff->io)
         status = IOClose(ff);
      Free(ff);
   }

   return status;
}



/***
** Function: Decrypt
**
** Description:
**   Decrypt a byte.
***/
UBYTE FASTCALL Decrypt(USHORT *r, const UBYTE cipher)
{
   UBYTE plain;

   plain = (UBYTE)(cipher ^ (*r>>8));
   *r = (USHORT)((cipher+*r) * c1 + c2);

   return plain;
}
