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
#     define FASCALL
#  endif
#endif

struct t1file;

/***
** Function: PFBAllocIOBlock
**
** Description:
**   Initiate an I/O stream for a PFB font file.
***/
struct t1file     *PFBAllocIOBlock  _ARGS((IN      char *name));


/***
** Function: PFBFreeIOBlock
**
** Description:
**   Free an I/O stream for a PFB font file.
***/
errcode FASTCALL  PFBFreeIOBlock    _ARGS((INOUT   struct t1file *io));


/***
** Function: PFBFileError
**
** Description:
**   Check if an I/O stream is ok.
***/
boolean FASTCALL  PFBFileError      _ARGS((IN      struct t1file *io));

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
short FASTCALL    PFBGetByte        _ARGS((INOUT   struct t1file *io));
