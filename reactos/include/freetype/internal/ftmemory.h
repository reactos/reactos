/***************************************************************************/
/*                                                                         */
/*  ftmemory.h                                                             */
/*                                                                         */
/*    The FreeType memory management macros (specification).               */
/*                                                                         */
/*  Copyright 1996-2000 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg                       */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef FTMEMORY_H
#define FTMEMORY_H


#include <freetype/config/ftconfig.h>
#include <freetype/fttypes.h>


  /*************************************************************************/
  /*                                                                       */
  /* <Macro>                                                               */
  /*    FT_SET_ERROR                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This macro is used to set an implicit `error' variable to a given  */
  /*    expression's value (usually a function call), and convert it to a  */
  /*    boolean which is set whenever the value is != 0.                   */
  /*                                                                       */
#undef  FT_SET_ERROR
#define FT_SET_ERROR( expression ) \
          ( ( error = (expression) ) != 0 )


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                           M E M O R Y                           ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

  BASE_DEF( FT_Error )  FT_Alloc( FT_Memory  memory,
                                  FT_Long    size,
                                  void**     P );

  BASE_DEF( FT_Error )  FT_Realloc( FT_Memory  memory,
                                    FT_Long    current,
                                    FT_Long    size,
                                    void**     P );

  BASE_DEF( void )  FT_Free( FT_Memory  memory,
                             void**     P );



  /* This `#include' is needed by the MEM_xxx() macros; it should be */
  /* available on all platforms we know of.                          */
#include <string.h>

#define MEM_Set( dest, byte, count )  memset( dest, byte, count )

#define MEM_Copy( dest, source, count )  memcpy( dest, source, count )

#define MEM_Move( dest, source, count )  memmove( dest, source, count )


  /*************************************************************************/
  /*                                                                       */
  /* We now support closures to produce completely reentrant code.  This   */
  /* means the allocation functions now takes an additional argument       */
  /* (`memory').  It is a handle to a given memory object, responsible for */
  /* all low-level operations, including memory management and             */
  /* synchronisation.                                                      */
  /*                                                                       */
  /* In order to keep our code readable and use the same macros in the     */
  /* font drivers and the rest of the library, MEM_Alloc(), ALLOC(), and   */
  /* ALLOC_ARRAY() now use an implicit variable, `memory'.  It must be     */
  /* defined at all locations where a memory operation is queried.         */
  /*                                                                       */
#define MEM_Alloc( _pointer_, _size_ )                     \
          FT_Alloc( memory, _size_, (void**)&(_pointer_) )

#define MEM_Alloc_Array( _pointer_, _count_, _type_ )    \
          FT_Alloc( memory, (_count_)*sizeof ( _type_ ), \
                    (void**)&(_pointer_) )

#define MEM_Realloc( _pointer_, _current_, _size_ )                     \
          FT_Realloc( memory, _current_, _size_, (void**)&(_pointer_) )

#define MEM_Realloc_Array( _pointer_, _current_, _new_, _type_ )        \
          FT_Realloc( memory, (_current_)*sizeof ( _type_ ),            \
                      (_new_)*sizeof ( _type_ ), (void**)&(_pointer_) )

#define ALLOC( _pointer_, _size_ )                       \
          FT_SET_ERROR( MEM_Alloc( _pointer_, _size_ ) )

#define REALLOC( _pointer_, _current_, _size_ )                       \
          FT_SET_ERROR( MEM_Realloc( _pointer_, _current_, _size_ ) )

#define ALLOC_ARRAY( _pointer_, _count_, _type_ )       \
          FT_SET_ERROR( MEM_Alloc( _pointer_,           \
                        (_count_)*sizeof ( _type_ ) ) )

#define REALLOC_ARRAY( _pointer_, _current_, _count_, _type_ ) \
          FT_SET_ERROR( MEM_Realloc( _pointer_,                \
                        (_current_)*sizeof ( _type_ ),         \
                        (_count_)*sizeof ( _type_ ) ) )

#define FREE( _pointer_ )  FT_Free( memory, (void**)&(_pointer_) )


#endif /* FTMEMORY_H */


/* END */
