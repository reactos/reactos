/***************************************************************************/
/*                                                                         */
/*  ftsystem.h                                                             */
/*                                                                         */
/*    FreeType low-level system interface definition (specification).      */
/*                                                                         */
/*  Copyright 1996-2000 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef FTSYSTEM_H
#define FTSYSTEM_H


  /*************************************************************************/
  /*                                                                       */
  /*                  M E M O R Y   M A N A G E M E N T                    */
  /*                                                                       */
  /*************************************************************************/


  typedef struct FT_MemoryRec_*  FT_Memory;


  typedef void*  (*FT_Alloc_Func)( FT_Memory  memory,
                                   long       size );

  typedef void  (*FT_Free_Func)( FT_Memory  memory,
                                 void*      block );

  typedef void*  (*FT_Realloc_Func)( FT_Memory  memory,
                                     long       cur_size,
                                     long       new_size,
                                     void*      block );


  struct FT_MemoryRec_
  {
    void*            user;
    FT_Alloc_Func    alloc;
    FT_Free_Func     free;
    FT_Realloc_Func  realloc;
  };


  /*************************************************************************/
  /*                                                                       */
  /*                       I / O   M A N A G E M E N T                     */
  /*                                                                       */
  /*************************************************************************/


  typedef union  FT_StreamDesc_
  {
    long   value;
    void*  pointer;

  } FT_StreamDesc;


  typedef struct FT_StreamRec_*  FT_Stream;


  typedef unsigned long  (*FT_Stream_IO)( FT_Stream       stream,
                                          unsigned long   offset,
                                          unsigned char*  buffer,
                                          unsigned long   count );

  typedef void  (*FT_Stream_Close)( FT_Stream  stream );


  struct  FT_StreamRec_
  {
    unsigned char*   base;
    unsigned long    size;
    unsigned long    pos;

    FT_StreamDesc    descriptor;
    FT_StreamDesc    pathname;    /* ignored by FreeType -- */
                                  /* useful for debugging   */
    FT_Stream_IO     read;
    FT_Stream_Close  close;

    FT_Memory        memory;
    unsigned char*   cursor;
    unsigned char*   limit;
  };


#endif /* FTSYSTEM_H */


/* END */
