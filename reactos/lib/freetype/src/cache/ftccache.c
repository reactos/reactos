/***************************************************************************/
/*                                                                         */
/*  ftccache.c                                                             */
/*                                                                         */
/*    The FreeType internal cache interface (body).                        */
/*                                                                         */
/*  Copyright 2000-2001, 2002, 2003 by                                     */
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
#include FT_CACHE_MANAGER_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H

#include "ftcerror.h"


#define FTC_HASH_MAX_LOAD  2
#define FTC_HASH_MIN_LOAD  1
#define FTC_HASH_SUB_LOAD  ( FTC_HASH_MAX_LOAD - FTC_HASH_MIN_LOAD )

/* this one _must_ be a power of 2! */
#define FTC_HASH_INITIAL_SIZE  8


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                   CACHE NODE DEFINITIONS                      *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_EXPORT_DEF( void )
  ftc_node_done( FTC_Node   node,
                 FTC_Cache  cache )
  {
    FTC_Family       family;
    FTC_FamilyEntry  entry;


    entry  = cache->manager->families.entries + node->fam_index;
    family = entry->family;

    /* remove from parent set table - eventually destroy the set */
    if ( --family->num_nodes == 0 )
      FT_LruList_Remove( cache->families, (FT_LruNode) family );
  }


  /* add a new node to the head of the manager's circular MRU list */
  static void
  ftc_node_mru_link( FTC_Node     node,
                     FTC_Manager  manager )
  {
    FTC_Node  first = manager->nodes_list;


    if ( first )
    {
      FTC_Node  last = first->mru_prev;


      FT_ASSERT( last->mru_next == first );

      node->mru_prev = last;
      node->mru_next = first;

      last->mru_next  = node;
      first->mru_prev = node;
    }
    else
    {
      FT_ASSERT( manager->num_nodes == 0 );

      node->mru_next = node;
      node->mru_prev = node;
    }

    manager->nodes_list = node;
    manager->num_nodes++;
  }


  /* remove a node from the manager's MRU list */
  static void
  ftc_node_mru_unlink( FTC_Node     node,
                       FTC_Manager  manager )
  {
    FTC_Node  first = manager->nodes_list;
    FTC_Node  prev  = node->mru_prev;
    FTC_Node  next  = node->mru_next;


    FT_ASSERT( first != NULL && manager->num_nodes > 0 );
    FT_ASSERT( next->mru_prev == node );
    FT_ASSERT( prev->mru_next == node );

    next->mru_prev = prev;
    prev->mru_next = next;

    if ( node == first )
    {
      /* this is the last node in the list; update its head pointer */
      if ( node == next )
        manager->nodes_list = NULL;
      else
        manager->nodes_list = next;
    }

    node->mru_next = NULL;
    node->mru_prev = NULL;
    manager->num_nodes--;
  }


  /* move a node to the head of the manager's MRU list */
  static void
  ftc_node_mru_up( FTC_Node     node,
                   FTC_Manager  manager )
  {
    FTC_Node  first = manager->nodes_list;


    if ( node != first )
    {
      FTC_Node  prev = node->mru_prev;
      FTC_Node  next = node->mru_next;
      FTC_Node  last;


      prev->mru_next = next;
      next->mru_prev = prev;

      last            = first->mru_prev;
      node->mru_next  = first;
      node->mru_prev  = last;
      first->mru_prev = node;
      last->mru_next  = node;

      manager->nodes_list = node;
    }
  }


  /* remove a node from its cache's hash table */
  static FT_Error
  ftc_node_hash_unlink( FTC_Node   node,
                        FTC_Cache  cache )
  {
    FT_Error   error = 0;
    FTC_Node  *pnode;
    FT_UInt    idx, num_buckets;


    idx = (FT_UInt)( node->hash & cache->mask );
    if ( idx < cache->p )
      idx = (FT_UInt)( node->hash & ( 2 * cache->mask + 1 ) );

    pnode = cache->buckets + idx;

    for (;;)
    {
      if ( *pnode == NULL )
      {
        FT_ERROR(( "ftc_node_hash_unlink: unknown node!\n" ));
        return FTC_Err_Ok;
      }

      if ( *pnode == node )
      {
        *pnode     = node->link;
        node->link = NULL;
        break;
      }

      pnode = &(*pnode)->link;
    }

    num_buckets = ( cache->p + cache->mask + 1 );

    if ( ++cache->slack > (FT_Long)num_buckets * FTC_HASH_SUB_LOAD )
    {
      FT_UInt    p         = cache->p;
      FT_UInt    mask      = cache->mask;
      FT_UInt    old_index = p + mask;
      FTC_Node*  pold;


      if ( old_index + 1 <= FTC_HASH_INITIAL_SIZE )
        goto Exit;

      if ( p == 0 )
      {
        FT_Memory  memory = cache->memory;


        cache->mask >>= 1;
        p             = cache->mask;

        if ( FT_RENEW_ARRAY( cache->buckets, ( mask + 1 ) * 2, (mask+1) ) )
        {
          FT_ERROR(( "ftc_node_hash_unlink: couldn't shunk buckets!\n" ));
          goto Exit;
        }
      }
      else
        p--;

      pnode = cache->buckets + p;
      while ( *pnode )
        pnode = &(*pnode)->link;

      pold   = cache->buckets + old_index;
      *pnode = *pold;
      *pold  = NULL;

      cache->slack -= FTC_HASH_MAX_LOAD;
      cache->p      = p;
    }

  Exit:
    return error;
  }



  /* add a node to the "top" of its cache's hash table */
  static FT_Error
  ftc_node_hash_link( FTC_Node   node,
                      FTC_Cache  cache )
  {
    FTC_Node  *pnode;
    FT_UInt    idx;
    FT_Error   error = 0;


    idx = (FT_UInt)( node->hash & cache->mask );
    if ( idx < cache->p )
      idx = (FT_UInt)( node->hash & (2 * cache->mask + 1 ) );

    pnode = cache->buckets + idx;

    node->link = *pnode;
    *pnode     = node;

    if ( --cache->slack < 0 )
    {
      FT_UInt    p     = cache->p;
      FT_UInt    mask  = cache->mask;
      FTC_Node   new_list;


      /* split a single bucket */
      new_list = NULL;
      pnode    = cache->buckets + p;

      for (;;)
      {
        node = *pnode;
        if ( node == NULL )
          break;

        if ( node->hash & ( mask + 1 ) )
        {
          *pnode     = node->link;
          node->link = new_list;
          new_list   = node;
        }
        else
          pnode = &node->link;
      }

      cache->buckets[p + mask + 1] = new_list;

      cache->slack += FTC_HASH_MAX_LOAD;

      if ( p >= mask )
      {
        FT_Memory  memory = cache->memory;


        if ( FT_RENEW_ARRAY( cache->buckets,
                             ( mask + 1 ) * 2, ( mask + 1 ) * 4 ) )
        {
          FT_ERROR(( "ftc_node_hash_link: couldn't expand buckets!\n" ));
          goto Exit;
        }

        cache->mask = 2 * mask + 1;
        cache->p    = 0;
      }
      else
        cache->p = p + 1;
    }

  Exit:
    return error;
  }




  /* remove a node from the cache manager */
  FT_EXPORT_DEF( void )
  ftc_node_destroy( FTC_Node     node,
                    FTC_Manager  manager )
  {
    FT_Memory        memory  = manager->library->memory;
    FTC_Cache        cache;
    FTC_FamilyEntry  entry;
    FTC_Cache_Class  clazz;


#ifdef FT_DEBUG_ERROR
    /* find node's cache */
    if ( node->fam_index >= manager->families.count )
    {
      FT_ERROR(( "ftc_node_destroy: invalid node handle\n" ));
      return;
    }
#endif

    entry = manager->families.entries + node->fam_index;
    cache = entry->cache;

#ifdef FT_DEBUG_ERROR
    if ( cache == NULL )
    {
      FT_ERROR(( "ftc_node_destroy: invalid node handle\n" ));
      return;
    }
#endif

    clazz = cache->clazz;

    manager->cur_weight -= clazz->node_weight( node, cache );

    /* remove node from mru list */
    ftc_node_mru_unlink( node, manager );

    /* remove node from cache's hash table */
    ftc_node_hash_unlink( node, cache );

    /* now finalize it */
    if ( clazz->node_done )
      clazz->node_done( node, cache );

    FT_FREE( node );

#if 0
    /* check, just in case of general corruption :-) */
    if ( manager->num_nodes == 0 )
      FT_ERROR(( "ftc_node_destroy: invalid cache node count! = %d\n",
                  manager->num_nodes ));
#endif
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                   CACHE FAMILY DEFINITIONS                    *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  FT_EXPORT_DEF( FT_Error )
  ftc_family_init( FTC_Family  family,
                   FTC_Query   query,
                   FTC_Cache   cache )
  {
    FT_Error         error;
    FTC_Manager      manager = cache->manager;
    FT_Memory        memory  = manager->library->memory;
    FTC_FamilyEntry  entry;


    family->cache     = cache;
    family->num_nodes = 0;

    /* now add to manager's family table */
    error = ftc_family_table_alloc( &manager->families, memory, &entry );
    if ( !error )
    {
      entry->cache      = cache;
      entry->family     = family;
      family->fam_index = entry->index;

      query->family = family;   /* save family in query */
    }

    return error;
  }


  FT_EXPORT_DEF( void )
  ftc_family_done( FTC_Family  family )
  {
    if ( family && family->cache )
    {
      FTC_Manager  manager = family->cache->manager;


      /* remove from manager's family table */
      ftc_family_table_free( &manager->families, family->fam_index );
    }
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    ABSTRACT CACHE CLASS                       *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/



  FT_EXPORT_DEF( FT_Error )
  ftc_cache_init( FTC_Cache  cache )
  {
    FT_Memory        memory = cache->memory;
    FTC_Cache_Class  clazz  = cache->clazz;
    FT_Error         error;


    cache->p     = 0;
    cache->mask  = FTC_HASH_INITIAL_SIZE - 1;
    cache->slack = FTC_HASH_INITIAL_SIZE * FTC_HASH_MAX_LOAD;

    if ( FT_NEW_ARRAY( cache->buckets, FTC_HASH_INITIAL_SIZE * 2 ) )
      goto Exit;

    /* now, initialize the lru list of families for this cache */
    if ( clazz->family_size > 0 )
    {
      FT_LruList_ClassRec*  lru_class = &cache->family_class;


      lru_class->list_size = sizeof( FT_LruListRec );
      lru_class->list_init = NULL;
      lru_class->list_done = NULL;

      lru_class->node_size    = clazz->family_size;
      lru_class->node_init    = (FT_LruNode_InitFunc)   clazz->family_init;
      lru_class->node_done    = (FT_LruNode_DoneFunc)   clazz->family_done;
      lru_class->node_flush   = (FT_LruNode_FlushFunc)  NULL;
      lru_class->node_compare = (FT_LruNode_CompareFunc)clazz->family_compare;

      error = FT_LruList_New( (FT_LruList_Class) lru_class,
                              0,    /* max items == 0 => unbounded list */
                              cache,
                              memory,
                              &cache->families );
      if ( error )
        FT_FREE( cache->buckets );
    }

  Exit:
    return error;
  }


  FT_EXPORT_DEF( void )
  ftc_cache_clear( FTC_Cache  cache )
  {
    if ( cache )
    {
      FT_Memory        memory  = cache->memory;
      FTC_Cache_Class  clazz   = cache->clazz;
      FTC_Manager      manager = cache->manager;
      FT_UFast         i;
      FT_UInt          count;

      count = cache->p + cache->mask + 1;

      for ( i = 0; i < count; i++ )
      {
        FTC_Node  *pnode = cache->buckets + i, next, node = *pnode;


        while ( node )
        {
          next        = node->link;
          node->link  = NULL;

          /* remove node from mru list */
          ftc_node_mru_unlink( node, manager );

          /* now finalize it */
          manager->cur_weight -= clazz->node_weight( node, cache );

          if ( clazz->node_done )
            clazz->node_done( node, cache );

          FT_FREE( node );
          node = next;
        }
        cache->buckets[i] = NULL;
      }

      cache->p = 0;

      /* destroy the families */
      if ( cache->families )
        FT_LruList_Reset( cache->families );
    }
  }


  FT_EXPORT_DEF( void )
  ftc_cache_done( FTC_Cache  cache )
  {
    if ( cache )
    {
      FT_Memory  memory = cache->memory;


      ftc_cache_clear( cache );

      FT_FREE( cache->buckets );
      cache->mask  = 0;
      cache->slack = 0;

      if ( cache->families )
      {
        FT_LruList_Destroy( cache->families );
        cache->families = NULL;
      }
    }
  }


  /* Look up a node in "top" of its cache's hash table. */
  /* If not found, create a new node.                   */
  /*                                                    */
  FT_EXPORT_DEF( FT_Error )
  ftc_cache_lookup( FTC_Cache   cache,
                    FTC_Query   query,
                    FTC_Node   *anode )
  {
    FT_Error     error = FTC_Err_Ok;
    FTC_Manager  manager;
    FT_LruNode   lru;
    FT_UInt      free_count = 0;


    if ( !cache || !query || !anode )
      return FTC_Err_Invalid_Argument;

    *anode = NULL;

    query->hash   = 0;
    query->family = NULL;

    manager = cache->manager;

   /*  here's a small note explaining what's happening in the code below.
    *
    *  We need to deal intelligently with out-of-memory (OOM) conditions
    *  when trying to create a new family or cache node during the lookup.
    *
    *  When an OOM is detected, we try to free one or more "old" nodes
    *  from the cache, then try again.  It may be necessary to do that
    *  several times, so a loop is needed.
    *
    *  The local variable "free_count" holds the number of "old" nodes to
    *  discard on each attempt.  It starts at 1 and doubles on each
    *  iteration.  The loop stops when:
    *
    *   - a non-OOM error is detected
    *   - a succesful lookup is performed
    *   - there are no more unused nodes in the cache
    *
    *  For the record, remember that all used nodes appear _before_
    *  unused ones in the manager's MRU node list.
    */

    for (;;)
    {
      {
        /* first of all, find the relevant family */
        FT_LruList              list    = cache->families;
        FT_LruNode              fam, *pfam;
        FT_LruNode_CompareFunc  compare = list->clazz->node_compare;

        pfam = &list->nodes;
        for (;;)
        {
          fam = *pfam;
          if ( fam == NULL )
          {
            error = FT_LruList_Lookup( list, query, &lru );
            if ( error )
              goto Fail;

            goto Skip;
          }

          if ( compare( fam, query, list->data ) )
            break;

          pfam = &fam->next;
        }

        FT_ASSERT( fam != NULL );

        /* move to top of list when needed */
        if ( fam != list->nodes )
        {
          *pfam       = fam->next;
          fam->next   = list->nodes;
          list->nodes = fam;
        }

        lru = fam;

      Skip:
        ;
      }

      {
        FTC_Family   family  = (FTC_Family) lru;
        FT_UFast     hash    = query->hash;
        FTC_Node*    bucket;
        FT_UInt      idx;


        idx = hash & cache->mask;
        if ( idx < cache->p )
          idx = hash & ( cache->mask * 2 + 1 );

        bucket  = cache->buckets + idx;


        if ( query->family     != family                 ||
             family->fam_index >= manager->families.size )
        {
          FT_ERROR((
            "ftc_cache_lookup: invalid query (bad 'family' field)\n" ));
          error = FTC_Err_Invalid_Argument;
          goto Exit;
        }

        if ( *bucket )
        {
          FTC_Node*             pnode   = bucket;
          FTC_Node_CompareFunc  compare = cache->clazz->node_compare;


          for ( ;; )
          {
            FTC_Node  node;


            node = *pnode;
            if ( node == NULL )
              break;

            if ( node->hash == hash                            &&
                 (FT_UInt)node->fam_index == family->fam_index &&
                 compare( node, query, cache ) )
            {
              /* move to head of bucket list */
              if ( pnode != bucket )
              {
                *pnode     = node->link;
                node->link = *bucket;
                *bucket    = node;
              }

              /* move to head of MRU list */
              if ( node != manager->nodes_list )
                ftc_node_mru_up( node, manager );

              *anode = node;
              goto Exit;
            }

            pnode = &node->link;
          }
        }

        /* didn't find a node, create a new one */
        {
          FTC_Cache_Class  clazz   = cache->clazz;
          FT_Memory        memory  = cache->memory;
          FTC_Node         node;


          if ( FT_ALLOC( node, clazz->node_size ) )
            goto Fail;

          node->fam_index = (FT_UShort) family->fam_index;
          node->hash      = query->hash;
          node->ref_count = 0;

          error = clazz->node_init( node, query, cache );
          if ( error )
          {
            FT_FREE( node );
            goto Fail;
          }

          error = ftc_node_hash_link( node, cache );
          if ( error )
          {
            clazz->node_done( node, cache );
            FT_FREE( node );
            goto Fail;
          }

          ftc_node_mru_link( node, cache->manager );

          cache->manager->cur_weight += clazz->node_weight( node, cache );

          /* now try to compress the node pool when necessary */
          if ( manager->cur_weight >= manager->max_weight )
          {
            node->ref_count++;
            FTC_Manager_Compress( manager );
            node->ref_count--;
          }

          *anode = node;
        }

       /* all is well, exit now
        */
        goto Exit;
      }

    Fail:
      if ( error != FTC_Err_Out_Of_Memory )
        goto Exit;

     /* There is not enough memory; try to release some unused nodes
      * from the cache to make room for a new one.
      */
      {
        FT_UInt  new_count;


        new_count = 1 + free_count * 2;

        /* check overflow and bounds */
        if ( new_count < free_count || free_count > manager->num_nodes )
          goto Exit;

        free_count = new_count;

        /* try to remove "new_count" nodes from the list */
        {
          FTC_Node  first = manager->nodes_list;
          FTC_Node  node;


          if ( first == NULL )  /* empty list! */
            goto Exit;

          /* go to last node - it's a circular list */
          node = first->mru_prev;
          for ( ; node && new_count > 0; new_count-- )
          {
            FTC_Node  prev = node->mru_prev;


            /* Used nodes always appear before unused one in the MRU
             * list.  If we find one here, we'd better stop right now
             * our iteration.
             */
            if ( node->ref_count > 0 )
            {
              /* if there are no unused nodes in the list, we'd better exit */
              if ( new_count == free_count )
                goto Exit;

              break;
            }

            ftc_node_destroy( node, manager );

            if ( node == first )
              break;

            node = prev;
          }
        }
      }
    }

  Exit:
    return error;
  }


/* END */
