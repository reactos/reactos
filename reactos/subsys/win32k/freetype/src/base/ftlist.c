/***************************************************************************/
/*                                                                         */
/*  ftlist.c                                                               */
/*                                                                         */
/*    Generic list support for FreeType (body).                            */
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
  /*  This file implements functions relative to list processing.  Its     */
  /*  data structures are defined in `freetype/internal/ftlist.h'.         */
  /*                                                                       */
  /*************************************************************************/


#include <freetype/internal/ftlist.h>
#include <freetype/internal/ftdebug.h>
#include <freetype/internal/ftobjs.h>


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_list


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_List_Find                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finds the list node for a given listed object.                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    list :: A pointer to the parent list.                              */
  /*    data :: The address of the listed object.                          */
  /*                                                                       */
  /* <Return>                                                              */
  /*    List node.  NULL if it wasn't found.                               */
  /*                                                                       */
  BASE_FUNC( FT_ListNode )  FT_List_Find( FT_List  list,
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


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_List_Add                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Appends an element to the end of a list.                           */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    list :: A pointer to the parent list.                              */
  /*    node :: The node to append.                                        */
  /*                                                                       */
  BASE_FUNC( void )  FT_List_Add( FT_List      list,
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


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_List_Insert                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Inserts an element at the head of a list.                          */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    list :: A pointer to parent list.                                  */
  /*    node :: The node to insert.                                        */
  /*                                                                       */
  BASE_FUNC( void )  FT_List_Insert( FT_List      list,
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


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_List_Remove                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Removes a node from a list.  This function doesn't check whether   */
  /*    the node is in the list!                                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    node :: The node to remove.                                        */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    list :: A pointer to the parent list.                              */
  /*                                                                       */
  BASE_FUNC( void )  FT_List_Remove( FT_List      list,
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


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_List_Up                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Moves a node to the head/top of a list.  Used to maintain LRU      */
  /*    lists.                                                             */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    list :: A pointer to the parent list.                              */
  /*    node :: The node to move.                                          */
  /*                                                                       */
  BASE_FUNC( void )  FT_List_Up( FT_List      list,
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


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_List_Iterate                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Parses a list and calls a given iterator function on each element. */
  /*    Note that parsing is stopped as soon as one of the iterator calls  */
  /*    returns a non-zero value.                                          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    list     :: A handle to the list.                                  */
  /*    iterator :: An interator function, called on each node of the      */
  /*                list.                                                  */
  /*    user     :: A user-supplied field which is passed as the second    */
  /*                argument to the iterator.                              */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The result (a FreeType error code) of the last iterator call.      */
  /*                                                                       */
  BASE_FUNC( FT_Error )  FT_List_Iterate( FT_List            list,
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


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_List_Finalize                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Destroys all elements in the list as well as the list itself.      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    list    :: A handle to the list.                                   */
  /*                                                                       */
  /*    destroy :: A list destructor that will be applied to each element  */
  /*               of the list.                                            */
  /*                                                                       */
  /*    memory  :: The current memory object which handles deallocation.   */
  /*                                                                       */
  /*    user    :: A user-supplied field which is passed as the last       */
  /*               argument to the destructor.                             */
  /*                                                                       */
  BASE_FUNC( void )  FT_List_Finalize( FT_List             list,
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

      FREE( cur );
      cur = next;
    }

    list->head = 0;
    list->tail = 0;
  }


/* END */
