/***************************************************************************/
/*                                                                         */
/*  ftsystem.c                                                             */
/*                                                                         */
/*    Unix-specific FreeType low-level system interface (body).            */
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


#include <ftconfig.h>
#include <freetype/internal/ftdebug.h>
#include <freetype/ftsystem.h>
#include <freetype/fterrors.h>
#include <freetype/fttypes.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


  /* memory-mapping includes and definitions                            */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/mman.h>
#ifndef MAP_FILE
#define MAP_FILE  0x00
#endif

  /*************************************************************************/
  /*                                                                       */
  /* The prototype for munmap() is not provided on SunOS.  This needs to   */
  /* have a check added later to see if the GNU C library is being used.   */
  /* If so, then this prototype is not needed.                             */
  /*                                                                       */
#if defined( __sun__ ) && !defined( SVR4 ) && !defined( __SVR4 )
  extern int  munmap( caddr_t  addr,
                      int      len );
#endif

#include <sys/stat.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

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
  /* <Function>                                                            */
  /*    ft_alloc                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The memory allocation function.                                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory :: A pointer to the memory object.                          */
  /*    size   :: The requested size in bytes.                             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    block  :: The address of newly allocated block.                    */
  /*                                                                       */
  static
  void*  ft_alloc( FT_Memory  memory,
                   long       size )
  {
    FT_UNUSED( memory );

    return malloc( size );
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

    return realloc( block, new_size );
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

    free( block );
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
    munmap ( stream->descriptor.pointer, stream->size );
        
    stream->descriptor.pointer = NULL;
    stream->size               = 0;
    stream->base               = 0;
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
    int          file;
    struct stat  stat_buf;


    if ( !stream )
      return FT_Err_Invalid_Stream_Handle;

    /* open the file */
    file = open( filepathname, O_RDONLY );
    if ( file < 0 )
    {
      FT_ERROR(( "FT_New_Stream:" ));
      FT_ERROR(( " could not open `%s'\n", filepathname ));
      return FT_Err_Cannot_Open_Resource;
    }

    if ( fstat( file, &stat_buf ) < 0 )
    {
      FT_ERROR(( "FT_New_Stream:" ));
      FT_ERROR(( " could not `fstat' file `%s'\n", filepathname ));
      goto Fail_Map;
    }
      
    stream->size = stat_buf.st_size;
    stream->pos  = 0;
    stream->base = mmap( NULL,
                         stream->size,
                         PROT_READ,
                         MAP_FILE | MAP_PRIVATE,
                         file,
                         0 );

    if ( (long)stream->base == -1 )
    {
      FT_ERROR(( "FT_New_Stream:" ));
      FT_ERROR(( " could not `mmap' file `%s'\n", filepathname ));
      goto Fail_Map;
    }

    close( file );

    stream->descriptor.pointer = stream->base;
    stream->pathname.pointer   = (char*)filepathname;
    
    stream->close = ft_close_stream;
    stream->read  = 0;
    
    FT_TRACE1(( "FT_New_Stream:" ));
    FT_TRACE1(( " opened `%s' (%d bytes) successfully\n",
                filepathname, stream->size ));

    return FT_Err_Ok;
    
  Fail_Map:
    close( file );

    stream->base = NULL;
    stream->size = 0;
    stream->pos  = 0;
    
    return FT_Err_Cannot_Open_Stream;
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
    

    memory = (FT_Memory)malloc( sizeof ( *memory ) );
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
