/***************************************************************************/
/*                                                                         */
/*  ftlru.c                                                                */
/*                                                                         */
/*    Simple LRU list-cache (body).                                        */
/*                                                                         */
/*  Copyright 2000-2001, 2002 by                                           */
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
#include FT_CACHE_H
#include FT_CACHE_INTERNAL_LRU_H
#include FT_LIST_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H

#include "ftcerror.h"


  FT_EXPORT_DEF( FT_Error )
  FT_LruList_New( FT_LruList_Class  clazz,
                  FT_UInt           max_nodes,
                  FT_Pointer        user_data,
                  FT_Memory         memory,
                  FT_LruList       *alist )
  {
    FT_Error    error;
    FT_LruList  list;


    if ( !alist || !clazz )
      return FTC_Err_Invalid_Argument;

    *alist = NULL;
    if ( !FT_ALLOC( list, clazz->list_size ) )
    {
      /* initialize common fields */
      list->clazz      = clazz;
      list->memory     = memory;
      list->max_nodes  = max_nodes;
      list->data       = user_data;

      if ( clazz->list_init )
      {
        error = clazz->list_init( list );
        if ( error )
        {
          if ( clazz->list_done )
            clazz->list_done( list );

          FT_FREE( list );
        }
      }

      *alist = list;
    }

    return error;
  }


  FT_EXPORT_DEF( void )
  FT_LruList_Destroy( FT_LruList  list )
  {
    FT_Memory         memory;
    FT_LruList_Class  clazz;


    if ( !list )
      return;

    memory = list->memory;
    clazz  = list->clazz;

    FT_LruList_Reset( list );

    if ( clazz->list_done )
      clazz->list_done( list );

    FT_FREE( list );
  }


  FT_EXPORT_DEF( void )
  FT_LruList_Reset( FT_LruList  list )
  {
    FT_LruNode        node;
    FT_LruList_Class  clazz;
    FT_Memory         memory;


    if ( !list )
      return;

    node   = list->nodes;
    clazz  = list->clazz;
    memory = list->memory;

    while ( node )
    {
      FT_LruNode  next = node->next;


      if ( clazz->node_done )
        clazz->node_done( node, list->data );

      FT_FREE( node );
      node = next;
    }

    list->nodes     = NULL;
    list->num_nodes = 0;
  }


  FT_EXPORT_DEF( FT_Error )
  FT_LruList_Lookup( FT_LruList   list,
                     FT_LruKey    key,
                     FT_LruNode  *anode )
  {
    FT_Error          error = 0;
    FT_LruNode        node, *pnode;
    FT_LruList_Class  clazz;
    FT_LruNode*       plast;
    FT_LruNode        result = NULL;
    FT_Memory         memory;


    if ( !list || !key || !anode )
      return FTC_Err_Invalid_Argument;

    pnode  = &list->nodes;
    plast  = pnode;
    node   = NULL;
    clazz  = list->clazz;
    memory = list->memory;

    if ( clazz->node_compare )
    {
      for (;;)
      {
        node = *pnode;
        if ( node == NULL )
          break;

        if ( clazz->node_compare( node, key, list->data ) )
          break;

        plast = pnode;
        pnode = &(*pnode)->next;
      }
    }
    else
    {
      for (;;)
      {
        node = *pnode;
        if ( node == NULL )
          break;

        if ( node->key == key )
          break;

        plast = pnode;
        pnode = &(*pnode)->next;
      }
    }

    if ( node )
    {
      /* move element to top of list */
      if ( list->nodes != node )
      {
        *pnode      = node->next;
        node->next  = list->nodes;
        list->nodes = node;
      }
      result = node;
      goto Exit;
    }

   /* since we haven't found the relevant element in our LRU list,
    * we're going to "create" a new one.
    *
    * the following code is a bit special, because it tries to handle
    * out-of-memory conditions (OOM) in an intelligent way.
    *
    * more precisely, if not enough memory is available to create a
    * new node or "flush" an old one, we need to remove the oldest
    * elements from our list, and try again. since several tries may
    * be necessary, a loop is needed
    *
    * this loop will only exit when:
    *
    *   - a new node was succesfully created, or an old node flushed
    *   - an error other than FT_Err_Out_Of_Memory is detected
    *   - the list of nodes is empty, and it isn't possible to create
    *     new nodes
    *
    * on each unsucesful attempt, one node will be removed from the list
    *
    */
    
    {
      FT_Int   drop_last = ( list->max_nodes > 0 && 
                             list->num_nodes >= list->max_nodes );

      for (;;)
      {
        node = NULL;

       /* when "drop_last" is true, we should free the last node in
        * the list to make room for a new one. note that we re-use
        * its memory block to save allocation calls.
        */
        if ( drop_last )
        {
         /* find the last node in the list
          */
          pnode = &list->nodes;
          node  = *pnode;
  
          if ( node == NULL )
          {
            FT_ASSERT( list->nodes == 0 );
            error = FT_Err_Out_Of_Memory;
            goto Exit;
          }

          FT_ASSERT( list->num_nodes > 0 );

          while ( node->next )
          {
            pnode = &node->next;
            node  = *pnode;
          }
  
         /* remove it from the list, and try to "flush" it. doing this will
          * save a significant number of dynamic allocations compared to
          * a classic destroy/create cycle
          */
          *pnode = NULL;
          list->num_nodes -= 1;
  
          if ( clazz->node_flush )
          {
            error = clazz->node_flush( node, key, list->data );
            if ( !error )
              goto Success;

           /* note that if an error occured during the flush, we need to
            * finalize it since it is potentially in incomplete state.
            */
          }

         /* we finalize, but do not destroy the last node, we
          * simply re-use its memory block !
          */
          if ( clazz->node_done )
            clazz->node_done( node, list->data );
            
          FT_MEM_ZERO( node, clazz->node_size );
        }
        else
        {
         /* try to allocate a new node when "drop_last" is not TRUE
          * this usually happens on the first pass, when the LRU list
          * is not already full.
          */
          if ( FT_ALLOC( node, clazz->node_size ) )
            goto Fail;
        }
  
        FT_ASSERT( node != NULL );

        node->key = key;
        error = clazz->node_init( node, key, list->data );
        if ( error )
        {
          if ( clazz->node_done )
            clazz->node_done( node, list->data );

          FT_FREE( node );
          goto Fail;
        }

      Success:
        result = node;

        node->next  = list->nodes;
        list->nodes = node;
        list->num_nodes++;
        goto Exit;
  
      Fail:
        if ( error != FT_Err_Out_Of_Memory )
          goto Exit;
        
        drop_last = 1;
        continue;
      }
    }

  Exit:
    *anode = result;
    return error;
  }



  FT_EXPORT_DEF( void )
  FT_LruList_Remove( FT_LruList  list,
                     FT_LruNode  node )
  {
    FT_LruNode  *pnode;


    if ( !list || !node )
      return;

    pnode = &list->nodes;
    for (;;)
    {
      if ( *pnode == node )
      {
        FT_Memory         memory = list->memory;
        FT_LruList_Class  clazz  = list->clazz;


        *pnode     = node->next;
        node->next = NULL;

        if ( clazz->node_done )
          clazz->node_done( node, list->data );

        FT_FREE( node );
        list->num_nodes--;
        break;
      }

      pnode = &(*pnode)->next;
    }
  }


  FT_EXPORT_DEF( void )
  FT_LruList_Remove_Selection( FT_LruList             list,
                               FT_LruNode_SelectFunc  select_func,
                               FT_Pointer             select_data )
  {
    FT_LruNode       *pnode, node;
    FT_LruList_Class  clazz;
    FT_Memory         memory;


    if ( !list || !select_func )
      return;

    memory = list->memory;
    clazz  = list->clazz;
    pnode  = &list->nodes;

    for (;;)
    {
      node = *pnode;
      if ( node == NULL )
        break;

      if ( select_func( node, select_data, list->data ) )
      {
        *pnode     = node->next;
        node->next = NULL;

        if ( clazz->node_done )
          clazz->node_done( node, list );

        FT_FREE( node );
      }
      else
        pnode = &(*pnode)->next;
    }
  }


/* END */
