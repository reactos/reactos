/***************************************************************************/
/*                                                                         */
/*  ftsystem.c                                                             */
/*                                                                         */
/*    ANSI-specific FreeType low-level system interface (body).            */
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

  /*************************************************************************/
  /*                                                                       */
  /* This file contains the default interface used by FreeType to access   */
  /* low-level, i.e. memory management, i/o access as well as thread       */
  /* synchronisation.  It can be replaced by user-specific routines if     */
  /* necessary.                                                            */
  /*                                                                       */
  /*************************************************************************/

#include <ddk/ntddk.h>

#include <freetype/config/ftconfig.h>
#include <freetype/internal/ftdebug.h>
#include <freetype/ftsystem.h>
#include <freetype/fterrors.h>
#include <freetype/fttypes.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


  /*************************************************************************/
  /*                                                                       */
  /*                       MEMORY MANAGEMENT INTERFACE                     */
  /*                                                                       */
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* It is not necessary to do any error checking for the                  */
  /* allocation-related functions.  This will be done by the higher level  */
  /* routines like FT_Alloc() or FT_Realloc().                             */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ft_alloc                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The memory allocation function.                                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory :: A pointer to the memory object.                          */
  /*                                                                       */
  /*    size   :: The requested size in bytes.                             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    block  :: The address of newly allocated block.                    */
  /*                                                                       */
  static
  void*  ft_alloc( FT_Memory  memory,
                   long       size )
  {
PVOID newmem;
    FT_UNUSED( memory );

/*    return malloc( size ); */

    return ExAllocatePool(NonPagedPool, size);
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ft_realloc                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The memory reallocation function.                                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory   :: A pointer to the memory object.                        */
  /*                                                                       */
  /*    cur_size :: The current size of the allocated memory block.        */
  /*                                                                       */
  /*    new_size :: The newly requested size in bytes.                     */
  /*                                                                       */
  /*    block    :: The current address of the block in memory.            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The address of the reallocated memory block.                       */
  /*                                                                       */
  static
  void*  ft_realloc( FT_Memory  memory,
                     long       cur_size,
                     long       new_size,
                     void*      block )
  {
    FT_UNUSED( memory );
    FT_UNUSED( cur_size );

/*    return realloc( block, new_size ); */
    ExFreePool(block);
    return ExAllocatePool(NonPagedPool, new_size);
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ft_free                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The memory release function.                                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory  :: A pointer to the memory object.                         */
  /*                                                                       */
  /*    block   :: The address of block in memory to be freed.             */
  /*                                                                       */
  static
  void  ft_free( FT_Memory  memory,
                 void*      block )
  {
    FT_UNUSED( memory );

/*    free( block ); */
    ExFreePool(block);
  }


  /*************************************************************************/
  /*                                                                       */
  /*                     RESOURCE MANAGEMENT INTERFACE                     */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_io

  /* We use the macro STREAM_FILE for convenience to extract the       */
  /* system-specific stream handle from a given FreeType stream object */
#define STREAM_FILE( stream )  ( (FILE*)stream->descriptor.pointer )


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ft_close_stream                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The function to close a stream.                                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream :: A pointer to the stream object.                          */
  /*                                                                       */
  static
  void  ft_close_stream( FT_Stream  stream )
  {
DbgPrint("ftsystem.c: UNIMPLEMENTED CALL\n");
/*    fclose( STREAM_FILE( stream ) ); FIXME */

    stream->descriptor.pointer = NULL;
    stream->size               = 0;
    stream->base               = 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ft_io_stream                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The function to open a stream.                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream :: A pointer to the stream object.                          */
  /*                                                                       */
  /*    offset :: The position in the data stream to start reading.        */
  /*                                                                       */
  /*    buffer :: The address of buffer to store the read data.            */
  /*                                                                       */
  /*    count  :: The number of bytes to read from the stream.             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The number of bytes actually read.                                 */
  /*                                                                       */
  static
  unsigned long  ft_io_stream( FT_Stream       stream,
                               unsigned long   offset,
                               unsigned char*  buffer,
                               unsigned long   count )
  {
    FILE*  file;


    file = STREAM_FILE( stream );

DbgPrint("ftsystem.c: UNIMPLEMENTED CALL\n");
/*    fseek( file, offset, SEEK_SET ); FIXME */

/*    return (unsigned long)fread( buffer, 1, count, file ); FIXME */
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_New_Stream                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Creates a new stream object.                                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    filepathname :: The name of the stream (usually a file) to be      */
  /*                    opened.                                            */
  /*                                                                       */
  /*    stream       :: A pointer to the stream object.                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Error )  FT_New_Stream( const char*  filepathname,
                                             FT_Stream    stream )
  {
/*    FILE*  file; FIXME */


DbgPrint("ftsystem.c: UNIMPLEMENTED CALL\n");

    if ( !stream )
      return FT_Err_Invalid_Stream_Handle;

/*    file = fopen( filepathname, "rb" );
    if ( !file )
    {
      FT_ERROR(( "FT_New_Stream:" ));
      FT_ERROR(( " could not open `%s'\n", filepathname ));

      return FT_Err_Cannot_Open_Resource;
    }

    fseek( file, 0, SEEK_END );
    stream->size = ftell( file );
    fseek( file, 0, SEEK_SET );

    stream->descriptor.pointer = file;
    stream->pathname.pointer   = (char*)filepathname;
    stream->pos                = 0;

    stream->read  = ft_io_stream;
    stream->close = ft_close_stream; FIXME */

    FT_TRACE1(( "FT_New_Stream:" ));
    FT_TRACE1(( " opened `%s' (%d bytes) successfully\n",
                filepathname, stream->size ));

    return FT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_New_Memory                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Creates a new memory object.                                       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    A pointer to the new memory object.  0 in case of error.           */
  /*                                                                       */
  FT_EXPORT_FUNC( FT_Memory )  FT_New_Memory( void )
  {
    FT_Memory  memory;


/*    memory = (FT_Memory)malloc( sizeof ( *memory ) ); FIXME */

    memory = ExAllocatePool(NonPagedPool, sizeof(*memory));

    if ( memory )
    {
      memory->user    = 0;
      memory->alloc   = ft_alloc;
      memory->realloc = ft_realloc;
      memory->free    = ft_free;
    }

    return memory;
  }


/* END */
