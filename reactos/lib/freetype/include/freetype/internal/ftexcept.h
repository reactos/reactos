#ifndef __FT_EXCEPT_H__
#define __FT_EXCEPT_H__

#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H

FT_BEGIN_HEADER



 /* I can't find a better place for this for now */

<<<<<<< ftexcept.h
=======

/* the size of a cleanup chunk in bytes is FT_CLEANUP_CHUNK_SIZE*12 + 4 */
/* this must be a small power of 2 whenever possible..                  */
/*                                                                      */
/* with a value of 5, we have a byte size of 64 bytes per chunk..       */
/*                                                                      */
#define  FT_CLEANUP_CHUNK_SIZE   5



  typedef struct FT_CleanupItemRec_
  {
    FT_Pointer      item;
    FT_CleanupFunc  item_func;
    FT_Pointer      item_data;

  } FT_CleanupItemRec;

  typedef struct FT_CleanupChunkRec_*   FT_CleanupChunk;
  
  typedef struct FT_CleanupChunkRec_
  {
    FT_CleanupChunk    link;
    FT_CleanupItemRec  items[ FT_CLEANUP_CHUNK_SIZE ];

  } FT_CleanupChunkRec;


  typedef struct FT_CleanupStackRec_
  {
    FT_CleanupItem     top;
    FT_CleanupItem     limit;
    FT_CleanupChunk    chunk;
    FT_CleanupChunkRec chunk_0;  /* avoids stupid dynamic allocation */
    FT_Memory          memory;

  } FT_CleanupStackRec, *FT_CleanupStack;


  FT_BASE( void )
  ft_cleanup_stack_push( FT_CleanupStack  stack,
                         FT_Pointer       item,
                         FT_CleanupFunc   item_func,
                         FT_Pointer       item_data );

  FT_BASE( void )
  ft_cleanup_stack_pop( FT_CleanupStack   stack,
                        FT_Int            destroy );

  FT_BASE( FT_CleanupItem )
  ft_cleanup_stack_peek( FT_CleanupStack  stack );

  FT_BASE( void )
  ft_xhandler_enter( FT_XHandler  xhandler,
                     FT_Memory    memory );                         

  FT_BASE( void )
  ft_xhandler_exit( FT_XHandler  xhandler );


  FT_BASE( void )
  ft_cleanup_throw( FT_CleanupStack  stack,
                    FT_Error         error );

>>>>>>> 1.2
FT_END_HEADER

#endif /* __FT_EXCEPT_H__ */
