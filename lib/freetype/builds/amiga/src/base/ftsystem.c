/***************************************************************************/
/*                                                                         */
/*  ftsystem.c                                                             */
/*                                                                         */
/*    Amiga-specific FreeType low-level system interface (body).           */
/*                                                                         */
/*  Copyright 1996-2001, 2002 by                                           */
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
  /* This file contains the Amiga interface used by FreeType to access     */
  /* low-level, i.e. memory management, i/o access as well as thread       */
  /* synchronisation.                                                      */
  /*                                                                       */
  /*************************************************************************/


// Maintained by Detlef Würkner <TetiSoft@apg.lahn.de>

// TetiSoft: Modified to avoid fopen() fclose() fread() fseek() ftell()
// malloc() realloc() and free() which can't be used in an amiga
// shared run-time library linked with libinit.o

#include <exec/memory.h>

#ifdef __GNUC__
// Avoid warnings "struct X declared inside parameter list"
#include <exec/devices.h>
#include <exec/io.h>
#include <exec/semaphores.h>
#include <dos/exall.h>
#endif

// Necessary with OS3.9 includes
#define __USE_SYSBASE

#include <proto/exec.h>
#include <proto/dos.h>

#ifndef __GNUC__
/* TetiSoft: Missing in alib_protos.h, see amiga.lib autodoc
 * (These amiga.lib functions work under AmigaOS V33 and up)
 */
extern APTR __asm
AsmCreatePool( register __d0 ULONG             memFlags,
               register __d1 ULONG             puddleSize,
               register __d2 ULONG             threshSize,
               register __a6 struct ExecBase*  SysBase );

extern VOID __asm
AsmDeletePool( register __a0 APTR              poolHeader,
               register __a6 struct ExecBase*  SysBase );

extern APTR __asm
AsmAllocPooled( register __a0 APTR              poolHeader,
                register __d0 ULONG             memSize,
                register __a6 struct ExecBase*  SysBase );

extern VOID __asm
AsmFreePooled( register __a0 APTR              poolHeader,
               register __a1 APTR              memory,
               register __d0 ULONG             memSize,
               register __a6 struct ExecBase*  SysBase);
#endif


// TetiSoft: C implementation of AllocVecPooled (see autodoc exec/AllocPooled)
APTR
AllocVecPooled( APTR   poolHeader,
                ULONG  memSize )
{
  ULONG  newSize = memSize + sizeof ( ULONG );
#ifdef __GNUC__
  ULONG  *mem = AllocPooled( poolHeader, newSize );
#else
  ULONG  *mem = AsmAllocPooled( poolHeader, newSize, SysBase );
#endif

  if ( !mem )
    return NULL;
  *mem = newSize;
  return mem + 1;
}


// TetiSoft: C implementation of FreeVecPooled (see autodoc exec/AllocPooled)
void
FreeVecPooled( APTR  poolHeader,
               APTR  memory )
{
  ULONG  *realmem = (ULONG *)memory - 1;

#ifdef __GNUC__
  FreePooled( poolHeader, realmem, *realmem );
#else
 AsmFreePooled( poolHeader, realmem, *realmem, SysBase );
#endif
}


#include <ft2build.h>
#include FT_CONFIG_CONFIG_H
#include FT_INTERNAL_DEBUG_H
#include FT_SYSTEM_H
#include FT_ERRORS_H
#include FT_TYPES_H

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
  /*    The address of newly allocated block.                              */
  /*                                                                       */
  FT_CALLBACK_DEF( void* )
  ft_alloc( FT_Memory  memory,
            long       size )
  {
//  FT_UNUSED( memory );

//  return malloc( size );
    return AllocVecPooled( memory->user, size );
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
  FT_CALLBACK_DEF( void* )
  ft_realloc( FT_Memory  memory,
              long       cur_size,
              long       new_size,
              void*      block )
  {
//  FT_UNUSED( memory );
//  FT_UNUSED( cur_size );

//  return realloc( block, new_size );

    void* new_block;

    new_block = AllocVecPooled ( memory->user, new_size );
    if ( new_block != NULL )
    {
      CopyMem ( block, new_block,
                ( new_size > cur_size ) ? cur_size : new_size );
      FreeVecPooled ( memory->user, block );
    }
    return new_block;
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
  /*    memory :: A pointer to the memory object.                          */
  /*                                                                       */
  /*    block  :: The address of block in memory to be freed.              */
  /*                                                                       */
  FT_CALLBACK_DEF( void )
  ft_free( FT_Memory  memory,
           void*      block )
  {
//  FT_UNUSED( memory );

//  free( block );

    FreeVecPooled( memory->user, block );
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
// #define STREAM_FILE( stream )  ( (FILE*)stream->descriptor.pointer )
#define STREAM_FILE( stream )  ( (BPTR)stream->descriptor.pointer )     // TetiSoft


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
  FT_CALLBACK_DEF( void )
  ft_close_stream( FT_Stream  stream )
  {
//  fclose( STREAM_FILE( stream ) );
    Close( STREAM_FILE( stream ) );     // TetiSoft

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
  FT_CALLBACK_DEF( unsigned long )
  ft_io_stream( FT_Stream       stream,
                unsigned long   offset,
                unsigned char*  buffer,
                unsigned long   count )
  {
//  FILE*  file;
    BPTR   file;        // TetiSoft


    file = STREAM_FILE( stream );

//  fseek( file, offset, SEEK_SET );
    Seek( file, offset, OFFSET_BEGINNING );     // TetiSoft

//  return (unsigned long)fread( buffer, 1, count, file );
    return (unsigned long)FRead( file, buffer, 1, count);
  }


  /* documentation is in ftobjs.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Stream_Open( FT_Stream    stream,
                  const char*  filepathname )
  {
//  FILE*                  file;
    BPTR                   file; // TetiSoft
    struct FileInfoBlock*  fib;  // TetiSoft


    if ( !stream )
      return FT_Err_Invalid_Stream_Handle;

//  file = fopen( filepathname, "rb" );
    file = Open( filepathname, MODE_OLDFILE );  // TetiSoft
    if ( !file )
    {
      FT_ERROR(( "FT_Stream_Open:" ));
      FT_ERROR(( " could not open `%s'\n", filepathname ));

      return FT_Err_Cannot_Open_Resource;
    }

//  fseek( file, 0, SEEK_END );
//  astream->size = ftell( file );
//  fseek( file, 0, SEEK_SET );
    fib = AllocDosObject( DOS_FIB, NULL );
    if ( !fib )
    {
      Close ( file );
      FT_ERROR(( "FT_Stream_Open:" ));
      FT_ERROR(( " could not open `%s'\n", filepathname ));

      return FT_Err_Cannot_Open_Resource;
    }
    if ( !( ExamineFH( file, fib ) ) )
    {
      FreeDosObject( DOS_FIB, fib );
      Close ( file );
      FT_ERROR(( "FT_Stream_Open:" ));
      FT_ERROR(( " could not open `%s'\n", filepathname ));

      return FT_Err_Cannot_Open_Resource;
    }
    stream->size = fib->fib_Size;
    FreeDosObject( DOS_FIB, fib );

//  stream->descriptor.pointer = file;
    stream->descriptor.pointer = (void *)file;

    stream->pathname.pointer   = (char*)filepathname;
    stream->pos                = 0;

    stream->read  = ft_io_stream;
    stream->close = ft_close_stream;

    FT_TRACE1(( "FT_Stream_Open:" ));
    FT_TRACE1(( " opened `%s' (%d bytes) successfully\n",
                filepathname, stream->size ));

    return FT_Err_Ok;
  }


#ifdef FT_DEBUG_MEMORY

  extern FT_Int
  ft_mem_debug_init( FT_Memory  memory );

  extern void
  ft_mem_debug_done( FT_Memory  memory );

#endif


  /* documentation is in ftobjs.h */

  FT_EXPORT_DEF( FT_Memory )
  FT_New_Memory( void )
  {
    FT_Memory  memory;


//  memory = (FT_Memory)malloc( sizeof ( *memory ) );
    memory = (FT_Memory)AllocVec( sizeof ( *memory ), MEMF_PUBLIC );
    if ( memory )
    {
//    memory->user = 0;
#ifdef __GNUC__
      memory->user = CreatePool( MEMF_PUBLIC, 2048, 2048 );
#else
      memory->user = AsmCreatePool( MEMF_PUBLIC, 2048, 2048, SysBase );
#endif
      if ( memory->user == NULL )
      {
        FreeVec( memory );
        memory = NULL;
      }
      else
      {
        memory->alloc   = ft_alloc;
        memory->realloc = ft_realloc;
        memory->free    = ft_free;
#ifdef FT_DEBUG_MEMORY
        ft_mem_debug_init( memory );
#endif
      }
    }

    return memory;
  }


  /* documentation is in ftobjs.h */

  FT_EXPORT_DEF( void )
  FT_Done_Memory( FT_Memory  memory )
  {
#ifdef FT_DEBUG_MEMORY
    ft_mem_debug_done( memory );
#endif

#ifdef __GNUC__
    DeletePool( memory->user );
#else
    AsmDeletePool( memory->user, SysBase );
#endif
    FreeVec( memory );
  }


/* END */
