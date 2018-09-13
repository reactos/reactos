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


#ifndef FWRITER_H
#define FWRITER_H

#ifndef _ARGS
#  define IN  const
#  define OUT
#  define INOUT
#  define _ARGS(arg) arg
#endif


#define TBL_OS2   (USHORT)0
#define TBL_CMAP  (USHORT)1 
#define TBL_CVT   (USHORT)2 
#define TBL_FPGM  (USHORT)3 
#define TBL_GASP  (USHORT)4 
#define TBL_GLYF  (USHORT)5
#define TBL_HEAD  (USHORT)6 	      
#define TBL_HHEA  (USHORT)7 	      
#define TBL_HMTX  (USHORT)8 	      
#define TBL_KERN	(USHORT)9 
#define TBL_LOCA  (USHORT)10			 	      
#define TBL_MAXP  (USHORT)11			 	      
#define TBL_NAME  (USHORT)12			 	      
#define TBL_POST  (USHORT)13			 	      
#define TBL_PREP  (USHORT)14			 
#define NUMTBL    15L
#define TBLDIRSIZE (4L+4L+4L+4L)

/* Referenced types. */
typedef struct ioFile OutputFile;




/***
** Function: WriteTableHeader
**
** Description:
**   This function initiates a TT font file, by initiating 
**   a handle used when writing the tables and by writing
**   the leading table dictionary of the file.
***/
void        WriteTableHeader  _ARGS((INOUT   OutputFile *file));


/***
** Function: OpenOutputFile
**
** Description:
***/
OutputFile  *OpenOutputFile   _ARGS((IN      char *name));


/***
** Function: CloseOutputFile
**
** Description:
***/
errcode     CloseOutputFile   _ARGS((INOUT   OutputFile *fp));


/***
** Function: FileError
**
** Description:
***/
boolean     FileError         _ARGS((INOUT   OutputFile *fp));


/***
** Function: FileTell
**
** Description:
***/
long        FileTell          _ARGS((INOUT   OutputFile *fp));


/***
** Function: WriteLong
**
** Description:
**   This function writes a 32-bit integer in the
**   Big Endian byte order, regardless of the
**   used byte order.
***/
void        WriteLong         _ARGS((IN      ULONG val,
                                     INOUT   OutputFile *file));

/***
** Function: WriteShort
**
** Description:
**   This function writes a 16-bit integer in the
**   Big Endian byte order, regardless of the used
**   byte order.
***/
void        WriteShort        _ARGS((IN      USHORT val,

                                     INOUT   OutputFile *file));
/***
** Function: WriteByte
**
** Description:
**   This function writes an 8-bit integer in the
**   Big Endian byte order, regardless of used
**   byte order.
***/
void        WriteByte         _ARGS((IN      UBYTE val,
                                     INOUT   OutputFile *file));

/***
** Function: WriteChecksum
**
** Description:
**   This function completes the whole TT font file,
**   by computing the check sum of the whole file and writing
**   it at the designated place.
***/
void        WriteChecksum     _ARGS((IN      long offset,
                                     INOUT   OutputFile *file));

/***
** Function: FileSeek
**
** Description:
***/
long        FileSeek          _ARGS((INOUT   OutputFile *fp,

                                     IN      long where));
/***
** Function: WriteBytes
**
** Description:
***/
USHORT      WriteBytes        _ARGS((IN      UBYTE *buf,
                                     IN      USHORT len,
                                     INOUT   OutputFile *fp));
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
errcode     CompleteTable     _ARGS((IN      long offset,
                                     IN      USHORT num,
                                     INOUT   OutputFile *file));
/***
** Function: RemoveFile
**
** Description:
**  Removes an already closed output file.
***/
void        RemoveFile        _ARGS((IN      char *name));
#endif
