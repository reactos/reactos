/***
**
**   Module: T1Parser
**
**   Description:
**  This is a module of the T1 to TT font converter. The module
**  contains functions that is used by the Builder moduler, to
**  manage the lowlevel writing to the TT font file, as well as
**  generic check sum, table length and table offset computations.
**
**   Author: Michael Jansson
**
**   Created: 5/26/93
**
***/


/**** INCLUDES */
/* General types and definitions. */
/*-none-*/

/* Special types and definitions. */
#include "types.h"

/* Module dependent types and prototypes. */
#include "fileio.h"
#include "fwriter.h"



/***** LOCAL TYPES */
/*-none-*/



/***** CONSTANTS */
static const char *dir[] = {
   "OS/2",
   "cmap",
   "cvt ",
   "fpgm",
   "gasp",
   "glyf",
   "head",
   "hhea",
   "hmtx",
   "kern",
   "loca",
   "maxp",
   "name",
   "post",
   "prep",
};

#define MAGIC_CHECKSUM  0xB1B0AFBA


/***** MACROS */
/*-none-*/



/***** STATIC FUNCTIONS */
/***
** Function: SumCheckSum
**
** Description:
**   This function computes the check sum of
**   a section of the output file.
***/
static ULONG SumCheckSum(OutputFile *file, long length)
{
   ULONG sum = 0;
   UBYTE tbl[32];


   /* Unwrap the loop a bit. */
   while (length>16) {
      (void)io_ReadBytes(tbl, (USHORT)16, file);
      sum += MkLong(tbl[0],  tbl[1],  tbl[2],  tbl[3]);
      sum += MkLong(tbl[4],  tbl[5],  tbl[6],  tbl[7]);
      sum += MkLong(tbl[8],  tbl[9],  tbl[10], tbl[11]);
      sum += MkLong(tbl[12], tbl[13], tbl[14], tbl[15]);
      length -= 16;
   }

   /* Do the sentinel DWORDS. */
   while (length>0) {
      (void)io_ReadBytes(tbl, (USHORT)4, file);
      sum += MkLong(tbl[0], tbl[1], tbl[2], tbl[3]);
      length -= 4;
   }

   return sum;
}



/***** FUNCTIONS */


/***
** Function: WriteLong
**
** Description:
**   This function writes a 32-bit integer in the
**   Big Endian byte order, regardless of the
**   used byte order.
***/
void WriteLong(const ULONG val, OutputFile *file)
{
   UBYTE bytes[4];

   bytes[0] = (UBYTE)((val>>24)&0xff);
   bytes[1] = (UBYTE)((val>>16)&0xff);
   bytes[2] = (UBYTE)((val>>8)&0xff);
   bytes[3] = (UBYTE)((val)&0xff);
   (void)WriteBytes(bytes, (USHORT)4, file);
}



/***
** Function: WriteShort
**
** Description:
**   This function writes a 16-bit integer in the
**   Big Endian byte order, regardless of the used
**   byte order.
***/
void WriteShort(const USHORT val, OutputFile *file)
{
   UBYTE bytes[2];

   bytes[0] = (UBYTE)((val>>8)&0xff); 
   bytes[1] = (UBYTE)((val)&0xff);
   (void)WriteBytes(bytes, (USHORT)2, file);
}



/***
** Function: WriteByte
**
** Description:
**   This function writes an 8-bit integer in the
**   Big Endian byte order, regardless of used
**   byte order.
***/
void WriteByte(const UBYTE byte, OutputFile *file)
{
   (void)WriteBytes(&byte, (USHORT)1, file);
}




/***
** Function: CompleteTable
**
** Description:
**   This function completes a TT font file table,
**   by computing the check sum and writing it, the
**   table length and table offset to the table directory
**   of the TT font file.
**
**   Please note the dependency that this function must
**   be called right after the last byte of the contents
**   of the table have been written.
***/
errcode CompleteTable(const long offset,
                      const USHORT num,
                      OutputFile *file)
{
   long end;
   long length;
   ULONG sum = 0;
   long curr;
   short i;

   /* Determine the end of the table. */
   end = FileTell(file);

   /* Write pad bytes. */
   length = end - offset;
   if (length%4)
      for (i=0; i<(4-(long)(length%4)); i++)
         WriteByte(0, file);

   /* Record end of file position. */
   curr = io_FileTell(file);

   /* Compute the check sum */
   (void)io_FileSeek(file, offset);
   sum = SumCheckSum(file, end - offset);

   /* Write table directory entry */
   (void)io_FileSeek(file, (ULONG)(12L + TBLDIRSIZE*num + 4L));
   WriteLong(sum, file);
   WriteLong((ULONG)offset, file);
   WriteLong((ULONG)length, file);

   /* Go to end of file. */
   (void)io_FileSeek(file, curr);

   return FileError(file);
}



/***
** Function: WriteChecksum
**
** Description:
**   This function completes the whole TT font file,
**   by computing the check sum of the whole file and writing
**   it at the designated place.
***/
void WriteChecksum(const long offset, OutputFile *file)
{
   long end;
   ULONG sum = 0;

   end = io_FileTell(file);
   (void)io_FileSeek(file, 0L);
   sum = SumCheckSum(file, end);
   sum = MAGIC_CHECKSUM - sum;
   (void)io_FileSeek(file, offset);
   WriteLong(sum, file);
}




/***
** Function: WriteTableHeader
**
** Description:
**   This function initiates a TT font file, by initiating 
**   a handle used when writing the tables and by writing
**   the leading table dictionary of the file.
***/
void WriteTableHeader(OutputFile *file)
{
   USHORT segcount;
   USHORT i;


   /* Count the segcount */ /*lint -e650 */
   for (segcount=0; (1UL<<(segcount+1)) <= NUMTBL; segcount++)
      continue; /*lint +e650*/

   /* Write the offset table. */
   WriteLong(0x00010000L, file);
   WriteShort((USHORT)NUMTBL, file);
   WriteShort((USHORT)((1<<segcount)*16), file);
   WriteShort(segcount, file);
   WriteShort((USHORT)(NUMTBL*16-(1<<segcount)*16), file);

   /* Write the table directory entries. */
   for (i=0; i<NUMTBL; i++) {
      (void)WriteBytes((UBYTE*)&(dir[i][0]), (USHORT)4, file);
      WriteLong(0L, file);
      WriteLong(0L, file);
      WriteLong(0L, file);
   }
}



/***
** Function: OpenOutputFile
**
** Description:
***/
OutputFile *OpenOutputFile(const  char *name)
{
   return io_OpenFile(name, READWRITE);
}



/***
** Function: CloseOutputFile
**
** Description:
***/
errcode CloseOutputFile(OutputFile *fp)
{
   return io_CloseFile(fp);
}


/***
** Function: WriteBytes
**
** Description:
***/
USHORT WriteBytes(const UBYTE *buf,
                  const USHORT len,
                  OutputFile *fp)
{
   return io_WriteBytes(buf, len, fp);
}



/***
** Function: FileError
**
** Description:
***/
boolean FileError(OutputFile *fp)
{
   return io_FileError(fp);
}



/***
** Function: FileTell
**
** Description:
***/
long FileTell(OutputFile *fp)
{
   return io_FileTell(fp);
}



/***
** Function: FileSeek
**
** Description:
***/
long FileSeek(OutputFile *fp,
              const long where)
{
   return io_FileSeek(fp, where);
}


/***
** Function: RemoveFile
**
** Description:
**  Removes an already closed output file.
***/
void RemoveFile(const char *name)
{
   io_RemoveFile(name);
}
