/***************************************************************************/
/*                                                                         */
/*  ftccache.i                                                             */
/*                                                                         */
/*    FreeType template for generic cache.                                 */
/*                                                                         */
/*  Copyright 2002 by                                                      */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef GEN_CACHE_FAMILY_COMPARE
#error "GEN_CACHE_FAMILY_COMPARE not defined in template instantiation"
#endif

#ifndef GEN_CACHE_NODE_COMPARE
#error "GEN_CACHE_NODE_COMPARE not defined in template instantiation"
#endif


  static FT_Error
  GEN_CACHE_LOOKUP( FTC_Cache   cache,
                    FTC_Query   query,
                    FTC_Node   *anode )
  {
    FT_LruNode  lru;
    FTC_Family  family;
    FT_UFast    hash;


    query->hash   = 0;
    query->family = NULL;

    /* XXX: we break encapsulation for the sake of speed! */
    {
      /* first of all, find the relevant family */
      FT_LruList  list  = cache->families;
      FT_LruNode  fam, *pfam;


      pfam = &list->nodes;
      for (;;)
      {
        fam = *pfam;
        if ( fam == NULL )
          goto Normal;

        if ( GEN_CACHE_FAMILY_COMPARE( fam, query, list->data ) )
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
    }

    {
      FTC_Node  node, *pnode, *bucket;


      family = (FTC_Family)lru;
      hash   = query->hash;

      {
        FT_UInt  idx;


        idx = hash & cache->mask;
        if ( idx < cache->p )
          idx = hash & ( cache->mask * 2 + 1 );

        bucket  = cache->buckets + idx;
      }

      pnode = bucket;

      for ( ;; )
      {
        node = *pnode;
        if ( node == NULL )
          goto Normal;

        if ( node->hash == hash                            &&
             (FT_UInt)node->fam_index == family->fam_index &&
             GEN_CACHE_NODE_COMPARE( node, query, cache )  )
        {
          /* we place the following out of the loop to make it */
          /* as small as possible...                           */
          goto Found;
        }

        pnode = &node->link;
      }

  Normal:
      return ftc_cache_lookup( cache, query, anode );

  Found:
      /* move to head of bucket list */
      if ( pnode != bucket )
      {
        *pnode     = node->link;
        node->link = *bucket;
        *bucket    = node;
      }

      /* move to head of MRU list */
      if ( node != cache->manager->nodes_list )
      {
        /* XXX: again, this is an inlined version of ftc_node_mru_up */
        FTC_Manager  manager = cache->manager;
        FTC_Node     first   = manager->nodes_list;
        FTC_Node     prev = node->mru_prev;
        FTC_Node     next = node->mru_next;
        FTC_Node     last;


        prev->mru_next = next;
        next->mru_prev = prev;

        last            = first->mru_prev;
        node->mru_next  = first;
        node->mru_prev  = last;
        first->mru_prev = node;
        last->mru_next  = node;

        manager->nodes_list = node;
      }

      *anode = node;
      return 0;
    }
  }

#undef GEN_CACHE_NODE_COMPARE
#undef GEN_CACHE_FAMILY_COMPARE
#undef GEN_CACHE_LOOKUP


/* END */
