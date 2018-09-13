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


#ifndef _ARGS
#  define IN  const
#  define OUT
#  define INOUT
#  define _ARGS(arg) arg
#endif
#ifndef FASTCALL
#  ifdef MSDOS
#     define FASTCALL   __fastcall
#  else
#     define FASTCALL
#  endif
#endif

enum ftype {pfb_file, mac_file, ascii_file};

struct FRHandle;
struct FontFile;



/***
** Function: GetByte
**
** Description:
**   Pull one byte out of the T1 font file.
***/
short FASTCALL  GetByte     _ARGS((INOUT   struct FontFile *ff));


/***
** Function: Decrypt
**
** Description:
**   Decrypt a byte.
***/
UBYTE FASTCALL  Decrypt     _ARGS((INOUT   USHORT *r, IN UBYTE b));


/***
** Function: FRCleanUp
**
** Description:
**   Free the resources used when reading/decoding data from
**   a T1 font file.
***/
errcode         FRCleanUp   _ARGS((INOUT   struct FontFile *ff));


/***
** Function: FRInit
**
** Description:
**   Initite the resources needed to read/decode data from
**   a T1 font file.
***/
errcode         FRInit      _ARGS((IN      char *name,
				   IN      enum ftype,
				   OUT     struct  FontFile **));
/***
** Function: GetSeq
**
** Description:
**   Pull one sequence of bytes that are delimited by 
**   a given pair of characters, e.g. '[' and ']'.
***/
char            *GetSeq     _ARGS((INOUT   struct FontFile *ff,
				   OUT     char *buf,
				   IN      USHORT len));
/***
** Function: Get_Token
**
** Description:
**   Pull one token from the T1 font file. A token 
**   is delimited by white space and various brackets.
***/
char            *Get_Token   _ARGS((INOUT   struct FontFile *ff,
				   OUT     char *buf,
				   IN      USHORT len));
/***
** Function: GetNewLine
**
** Description:
**   Pull one whole line from the T1 font file, starting at
**   the current position.
***/
char            *GetNewLine    _ARGS((INOUT   struct FontFile *ff,
				   OUT     char *buf,
				   IN      USHORT len));
