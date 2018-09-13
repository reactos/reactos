/***
**
**   Module: PFB
**
**   Description:
**        This is a module of the T1 to TT font converter. The module
**        contains functions that manages the "printer binary file" file
**        format (Adobe Type 1 for MS-Windows).
**
**   Author: Michael Jansson
**
**   Created: 5/26/93
**
***/


/**** INCLUDES */
/* General types and definitions. */
#include <ctype.h>

/* Special types and definitions. */
#include "titott.h"
#include "types.h"
#include "safemem.h"

/* Module dependent types and prototypes. */
#include "fileio.h"



/***** LOCAL TYPES */
struct t1file {
   struct ioFile *file;
   enum blocktype {none=0, ascii, encoded} type;
   long size;
   long curr;
};



/***** CONSTANTS */
/*-none-*/



/***** MACROS */
#define HEXDIGIT(c)  (((c)>='a') ? ((c) - 'a' + 10) : ((c) - '0')) 
#define HEX(c1,c2)   (HEXDIGIT(c1)*16+HEXDIGIT(c2))



/***** STATIC FUNCTIONS */
/*-none-*/



/***** FUNCTIONS */

/***
** Function: PFBAllocIOBlock
**
** Description:
**   Initiate an I/O stream for a PFB font file.
***/
struct t1file *PFBAllocIOBlock(const char *name)
{
   struct t1file *pfb;

   if ((pfb=Malloc(sizeof(struct t1file)))!=NULL) {

      if ((pfb->file = io_OpenFile(name, READONLY))==NULL) {
         Free(pfb);
         pfb = NULL;
      } else {
         pfb->type = none;
         pfb->size = 0;
         pfb->curr = 0;
      }
   }

   return pfb;
}


/***
** Function: PFBFreeIOBlock
**
** Description:
**   Free an I/O stream for a PFB font file.
***/
errcode FASTCALL PFBFreeIOBlock(struct t1file *pfb)
{
   errcode status = SUCCESS;

   status = io_CloseFile(pfb->file);
   Free(pfb);

   return status;
}


/***
** Function: PFBFileError
**
** Description:
**   Check if an I/O stream is ok.
***/
boolean FASTCALL PFBFileError(const struct t1file *pfb)
{
   return io_FileError(pfb->file);
}


/***
** Function: PFBGetByte
**
** Description:
**   Pull one byte from the opened PFB font file.
**   Please note that this function does not check
**   if it succeedes it reading a byte or not. It is
**   up to the calling module to manage the  error
**   checkes by using the FileError() function when
**   appropriate.
**
***/
short FASTCALL PFBGetByte(struct t1file *pfb)
{
   short b, c1, c2;

   /* Enter a new PFB block? */
   if (pfb->curr>=pfb->size) {
      UBYTE type[2];
      UBYTE size[4];

      type[0]=(UBYTE)io_ReadOneByte(pfb->file);
      type[1]=(UBYTE)io_ReadOneByte(pfb->file);

      size[0]=(UBYTE)io_ReadOneByte(pfb->file);
      size[1]=(UBYTE)io_ReadOneByte(pfb->file);
      size[2]=(UBYTE)io_ReadOneByte(pfb->file);
      size[3]=(UBYTE)io_ReadOneByte(pfb->file);

      pfb->curr = 0;
      pfb->size = (long)MkLong(size[3], size[2], size[1], size[0]);
      pfb->type = ((type[0]==0x80 && (type[1]==0x01 ||
                                      type[1]==0x02)) ? ascii : encoded);
   }


   /* Read a byte. */
   switch (pfb->type) {
      case ascii:
         b = (short)io_ReadOneByte(pfb->file);
         pfb->curr++;
         break;
      case encoded:
         c1 = (short)tolower(io_ReadOneByte(pfb->file));
         c2 = (short)tolower(io_ReadOneByte(pfb->file));
         b = (short)HEX(c1, c2);
         pfb->curr += 2;
         break;
      case none:
      default:
         b = (short)-1;
         break;
   }

   return b;
}
