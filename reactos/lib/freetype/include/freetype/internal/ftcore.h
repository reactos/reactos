#ifndef __FT_CORE_H__
#define __FT_CORE_H__

#include <ft2build.h>
#include FT_TYPES_H
#include FT_SYSTEM_MEMORY_H

FT_BEGIN_HEADER

 /**************************************************************************/
 /**************************************************************************/
 /*****                                                                *****/
 /*****                  C L E A N U P   S T A C K                     *****/
 /*****                                                                *****/
 /**************************************************************************/
 /**************************************************************************/


 /************************************************************************
  *
  * @functype: FT_CleanupFunc
  *
  * @description:
  *   a function used to cleanup a given item on the cleanup stack
  *
  * @input:
  *   item      :: target item pointer
  *   item_data :: optional argument to cleanup routine
  */
  typedef void  (*FT_CleanupFunc)( FT_Pointer  item,
                                   FT_Pointer  item_data );



 /************************************************************************
  *
  * @type: FT_XHandler
  *
  * @description:
  *   handle to an exception-handler structure for the FreeType
  *   exception sub-system
  *
  * @note:
  *   exception handlers are allocated on the stack within a
  *   @FT_XTRY macro. Do not try to access them directly.
  */
  typedef struct FT_XHandlerRec_*  FT_XHandler;


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
  ft_cleanup_throw( FT_CleanupStack  stack,
                    FT_Error         error );



 /**************************************************************************/
 /**************************************************************************/
 /*****                                                                *****/
 /*****                 M E M O R Y   M A N A G E R                    *****/
 /*****                                                                *****/
 /**************************************************************************/
 /**************************************************************************/

  typedef struct FT_MemoryRec_
  {
    FT_Memory_AllocFunc     mem_alloc;   /* shortcut to funcs->mem_alloc */
    FT_Memory_FreeFunc      mem_free;    /* shortcut to funcs->mem_free  */
    FT_Pointer              mem_data;
    const FT_Memory_Funcs   mem_funcs;

    FT_CleanupStackRec      cleanup_stack;
    FT_Pointer              meta_class;

  } FT_MemoryRec;


#define  FT_MEMORY(x)  ((FT_Memory)(x))
#define  FT_MEMORY__ALLOC(x)       FT_MEMORY(x)->mem_alloc
#define  FT_MEMORY__FREE(x)        FT_MEMORY(x)->mem_free
#define  FT_MEMORY__REALLOC(x)     FT_MEMORY(x)->mem_funcs->mem_realloc
#define  FT_MEMORY__CLEANUP(x)     (&FT_MEMORY(x)->cleanup_stack)
#define  FT_MEMORY__META_CLASS(x)  ((FT_MetaClass)(FT_MEMORY(x)->meta_class))


 /**************************************************************************/
 /**************************************************************************/
 /*****                                                                *****/
 /*****             E X C E P T I O N   H A N D L I N G                *****/
 /*****                                                                *****/
 /**************************************************************************/
 /**************************************************************************/


 /************************************************************************
  *
  * @struct: FT_XHandlerRec
  *
  * @description:
  *   exception handler structure
  *
  * @fields:
  *   previous   :: previous handler in chain.
  *   jum_buffer :: processor state used by setjmp/longjmp to implement
  *                 exception control transfer
  *   error      :: exception error code
  *   mark       :: top of cleanup stack when @FT_XTRY is used
  */
  typedef struct FT_XHandlerRec_
  {
    FT_XHandler        previous;
    ft_jmp_buf         jump_buffer;
    volatile FT_Error  error;
    FT_Pointer         mark;

  } FT_XHandlerRec;

  FT_BASE( void )
  ft_xhandler_enter( FT_XHandler  xhandler,
                     FT_Memory    memory );

  FT_BASE( void )
  ft_xhandler_exit( FT_XHandler  xhandler );








FT_END_HEADER

#endif /* __FT_CORE_H__ */
