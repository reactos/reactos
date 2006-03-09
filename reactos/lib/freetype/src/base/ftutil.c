/***************************************************************************/
/*                                                                         */
/*  ftutil.c                                                               */
/*                                                                         */
/*    FreeType utility file for memory and list management (body).         */
/*                                                                         */
/*  Copyright 2002, 2004, 2005 by                                          */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_MEMORY_H
#include FT_INTERNAL_OBJECTS_H
#include FT_LIST_H


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_memory


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                                                               *****/
  /*****               M E M O R Y   M A N A G E M E N T               *****/
  /*****                                                               *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

  /* documentation is in ftmemory.h */

  FT_BASE_DEF( FT_Error )
  FT_Alloc( FT_Memory  memory,
            FT_Long    size,
            void*     *P )
  {
    FT_ASSERT( P != 0 );

    if ( size > 0 )
    {
      *P = memory->alloc( memory, size );
      if ( !*P )
      {
        FT_ERROR(( "FT_Alloc:" ));
        FT_ERROR(( " Out of memory? (%ld requested)\n",
                   size ));

        return FT_Err_Out_Of_Memory;
      }
      FT_MEM_ZERO( *P, size );
    }
    else
      *P = NULL;

    FT_TRACE7(( "FT_Alloc:" ));
    FT_TRACE7(( " size = %ld, block = 0x%08p, ref = 0x%08p\n",
                size, *P, P ));

    return FT_Err_Ok;
  }


  /* documentation is in ftmemory.h */

  FT_BASE_DEF( FT_Error )
  FT_QAlloc( FT_Memory  memory,
             FT_Long    size,
             void*     *P )
  {
    FT_ASSERT( P != 0 );

    if ( size > 0 )
    {
      *P = memory->alloc( memory, size );
      if ( !*P )
      {
        FT_ERROR(( "FT_QAlloc:" ));
        FT_ERROR(( " Out of memory? (%ld requested)\n",
                   size ));

        return FT_Err_Out_Of_Memory;
      }
    }
    else
      *P = NULL;

    FT_TRACE7(( "FT_QAlloc:" ));
    FT_TRACE7(( " size = %ld, block = 0x%08p, ref = 0x%08p\n",
                size, *P, P ));

    return FT_Err_Ok;
  }


  /* documentation is in ftmemory.h */

  FT_BASE_DEF( FT_Error )
  FT_Realloc( FT_Memory  memory,
              FT_Long    current,
              FT_Long    size,
              void**     P )
  {
    void*  Q;


    FT_ASSERT( P != 0 );

    /* if the original pointer is NULL, call FT_Alloc() */
    if ( !*P )
      return FT_Alloc( memory, size, P );

    /* if the new block if zero-sized, clear the current one */
    if ( size <= 0 )
    {
      FT_Free( memory, P );
      return FT_Err_Ok;
    }

    Q = memory->realloc( memory, current, size, *P );
    if ( !Q )
      goto Fail;

    if ( size > current )
      FT_MEM_ZERO( (char*)Q + current, size - current );

    *P = Q;
    return FT_Err_Ok;

  Fail:
    FT_ERROR(( "FT_Realloc:" ));
    FT_ERROR(( " Failed (current %ld, requested %ld)\n",
               current, size ));
    return FT_Err_Out_Of_Memory;
  }


  /* documentation is in ftmemory.h */

  FT_BASE_DEF( FT_Error )
  FT_QRealloc( FT_Memory  memory,
               FT_Long    current,
               FT_Long    size,
               void**     P )
  {
    void*  Q;


    FT_ASSERT( P != 0 );

    /* if the original pointer is NULL, call FT_QAlloc() */
    if ( !*P )
      return FT_QAlloc( memory, size, P );

    /* if the new block if zero-sized, clear the current one */
    if ( size <= 0 )
    {
      FT_Free( memory, P );
      return FT_Err_Ok;
    }

    Q = memory->realloc( memory, current, size, *P );
    if ( !Q )
      goto Fail;

    *P = Q;
    return FT_Err_Ok;

  Fail:
    FT_ERROR(( "FT_QRealloc:" ));
    FT_ERROR(( " Failed (current %ld, requested %ld)\n",
               current, size ));
    return FT_Err_Out_Of_Memory;
  }


  /* documentation is in ftmemory.h */

  FT_BASE_DEF( void )
  FT_Free( FT_Memory  memory,
           void**     P )
  {
    FT_TRACE7(( "FT_Free:" ));
    FT_TRACE7(( " Freeing block 0x%08p, ref 0x%08p\n",
                P, P ? *P : (void*)0 ));

    if ( P && *P )
    {
      memory->free( memory, *P );
      *P = 0;
    }
  }


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                                                               *****/
  /*****            D O U B L Y   L I N K E D   L I S T S              *****/
  /*****                                                               *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

#undef  FT_COMPONENT
#define FT_COMPONENT  trace_list

  /* documentation is in ftlist.h */

  FT_EXPORT_DEF( FT_ListNode )
  FT_List_Find( FT_List  list,
                void*    data )
  {
    FT_ListNode  cur;


    cur = list->head;
    while ( cur )
    {
      if ( cur->data == data )
        return cur;

      cur = cur->next;
    }

    return (FT_ListNode)0;
  }


  /* documentation is in ftlist.h */

  FT_EXPORT_DEF( void )
  FT_List_Add( FT_List      list,
               FT_ListNode  node )
  {
    FT_ListNode  before = list->tail;


    node->next = 0;
    node->prev = before;

    if ( before )
      before->next = node;
    else
      list->head = node;

    list->tail = node;
  }


  /* documentation is in ftlist.h */

  FT_EXPORT_DEF( void )
  FT_List_Insert( FT_List      list,
                  FT_ListNode  node )
  {
    FT_ListNode  after = list->head;


    node->next = after;
    node->prev = 0;

    if ( !after )
      list->tail = node;
    else
      after->prev = node;

    list->head = node;
  }


  /* documentation is in ftlist.h */

  FT_EXPORT_DEF( void )
  FT_List_Remove( FT_List      list,
                  FT_ListNode  node )
  {
    FT_ListNode  before, after;


    before = node->prev;
    after  = node->next;

    if ( before )
      before->next = after;
    else
      list->head = after;

    if ( after )
      after->prev = before;
    else
      list->tail = before;
  }


  /* documentation is in ftlist.h */

  FT_EXPORT_DEF( void )
  FT_List_Up( FT_List      list,
              FT_ListNode  node )
  {
    FT_ListNode  before, after;


    before = node->prev;
    after  = node->next;

    /* check whether we are already on top of the list */
    if ( !before )
      return;

    before->next = after;

    if ( after )
      after->prev = before;
    else
      list->tail = before;

    node->prev       = 0;
    node->next       = list->head;
    list->head->prev = node;
    list->head       = node;
  }


  /* documentation is in ftlist.h */

  FT_EXPORT_DEF( FT_Error )
  FT_List_Iterate( FT_List            list,
                   FT_List_Iterator   iterator,
                   void*              user )
  {
    FT_ListNode  cur   = list->head;
    FT_Error     error = FT_Err_Ok;


    while ( cur )
    {
      FT_ListNode  next = cur->next;


      error = iterator( cur, user );
      if ( error )
        break;

      cur = next;
    }

    return error;
  }


  /* documentation is in ftlist.h */

  FT_EXPORT_DEF( void )
  FT_List_Finalize( FT_List             list,
                    FT_List_Destructor  destroy,
                    FT_Memory           memory,
                    void*               user )
  {
    FT_ListNode  cur;


    cur = list->head;
    while ( cur )
    {
      FT_ListNode  next = cur->next;
      void*        data = cur->data;


      if ( destroy )
        destroy( memory, data, user );

      FT_FREE( cur );
      cur = next;
    }

    list->head = 0;
    list->tail = 0;
  }


  FT_BASE( FT_UInt32 )
  ft_highpow2( FT_UInt32  value )
  {
    FT_UInt32  value2;


    /*
     *  We simply clear the lowest bit in each iteration.  When
     *  we reach 0, we know that the previous value was our result.
     */
    for ( ;; )
    {
      value2 = value & (value - 1);  /* clear lowest bit */
      if ( value2 == 0 )
        break;

      value = value2;
    }
    return value;
  }


/* END */
