/******************************************************************
 *
 *  fthash.h  - fast dynamic hash tables
 *
 *  Copyright 2002 by
 *  David Turner, Robert Wilhelm, and Werner Lemberg
 *
 *  This file is part of the FreeType project, and may only be used,
 *  modified, and distributed under the terms of the FreeType project
 *  license, LICENSE.TXT.  By continuing to use, modify, or distribute
 *  this file you indicate that you have read the license and
 *  understand and accept it fully.
 *
 *
 *  This header is used to define dynamic hash tables as described
 *  by the article "Main-Memory Linear Hashing - Some Enhancements
 *  of Larson's Algorithm" by Mikael Petterson.
 *
 *  Basically, linear hashing prevents big "stalls" during
 *  resizes of the buckets array by only splitting one bucket
 *  at a time. This ensures excellent response time even when
 *  the table is frequently resized..
 *
 *
 *  Note that the use of the FT_Hash type is rather unusual in order
 *  to be as generic and efficient as possible. See the comments in the
 *  following definitions for more details.
 */

#ifndef __FT_HASH_H__
#define __FT_HASH_H__

#include <ft2build.h>
#include FT_TYPES_H

FT_BEGIN_HEADER

 /***********************************************************
  *
  * @type: FT_Hash
  *
  * @description:
  *   handle to a @FT_HashRec structure used to model a
  *   dynamic hash table
  */
  typedef struct FT_HashRec_*      FT_Hash;


 /***********************************************************
  *
  * @type: FT_HashNode
  *
  * @description:
  *   handle to a @FT_HashNodeRec structure used to model a
  *   single node of a hash table
  */
  typedef struct FT_HashNodeRec_*  FT_HashNode;


 /***********************************************************
  *
  * @type: FT_HashLookup
  *
  * @description:
  *   handle to a @FT_HashNode pointer. This is returned by
  *   the @ft_hash_lookup function and can later be used by
  *   @ft_hash_add or @ft_hash_remove
  */
  typedef FT_HashNode*     FT_HashLookup;


 /***********************************************************
  *
  * @type: FT_Hash_EqualFunc
  *
  * @description:
  *   a function used to compare two nodes of the hash table
  *
  * @input:
  *   node1 :: handle to first node
  *   node2 :: handle to second node
  *
  * @return:
  *   1 iff the 'keys' in 'node1' and 'node2' are identical.
  *   0 otherwise.
  */
  typedef FT_Int  (*FT_Hash_EqualFunc)( FT_HashNode  node1,
                                        FT_HashNode  node2 );


 /***********************************************************
  *
  * @struct: FT_HashRec
  *
  * @description:
  *   a structure used to model a dynamic hash table.
  *
  * @fields:
  *   memory       :: memory manager used to allocate
  *                   the buckets array and the hash nodes
  *
  *   buckets      :: array of hash buckets
  *
  *   node_size    :: size of node in bytes
  *   node_compare :: a function used to compare two nodes
  *   node_hash    :: a function used to compute the hash
  *                   value of a given node
  *   p            ::
  *   mask         ::
  *   slack        ::
  *
  * @note:
  *   'p', 'mask' and 'slack' are control values managed by
  *   the hash table. Do not try to interpret them directly.
  *
  *   You can grab the hash table size by calling
  *   '@ft_hash_get_size'.
  */
  typedef struct FT_HashRec_
  {
    FT_HashNode*         buckets;
    FT_UInt              p;
    FT_UInt              mask;  /* really maxp-1 */
    FT_Long              slack;
    FT_Hash_EqualFunc    node_equal;
    FT_Memory            memory;

  } FT_HashRec;


 /***********************************************************
  *
  * @struct: FT_HashNodeRec
  *
  * @description:
  *   a structure used to model the root fields of a dynamic
  *   hash table node.
  *
  *   it's up to client applications to "sub-class" this
  *   structure to add relevant (key,value) definitions
  *
  * @fields:
  *   link :: pointer to next node in bucket's collision list
  *   hash :: 32-bit hash value for this node
  *
  * @note:
  *   it's up to client applications to "sub-class" this structure
  *   to add relevant (key,value) type definitions. For example,
  *   if we want to build a "string -> int" mapping, we could use
  *   something like:
  *
  *   {
  *     typedef struct MyNodeRec_
  *     {
  *       FT_HashNodeRec  hnode;
  *       const char*     key;
  *       int             value;
  *
  *     } MyNodeRec, *MyNode;
  *   }
  *
  */
  typedef struct FT_HashNodeRec_
  {
    FT_HashNode  link;
    FT_UInt32    hash;

  } FT_HashNodeRec;


 /****************************************************************
  *
  * @function: ft_hash_init
  *
  * @description:
  *   initialize a dynamic hash table
  *
  * @input:
  *   table      :: handle to target hash table structure
  *   node_equal :: node comparison function
  *   memory     :: memory manager handle used to allocate the
  *                 buckets array within the hash table
  *
  * @return:
  *   error code. 0 means success
  *
  * @note:
  *   the node comparison function should only compare node _keys_
  *   and ignore values !! with good hashing computation (which the
  *   user must perform itself), the comparison function should be
  *   pretty seldom called.
  *
  *   here is a simple example:
  *
  *   {
  *     static int my_equal( MyNode  node1,
  *                          MyNode  node2 )
  *     {
  *       // compare keys of 'node1' and 'node2'
  *       return (strcmp( node1->key, node2->key ) == 0);
  *     }
  *
  *     ....
  *
  *     ft_hash_init( &hash, (FT_Hash_EqualFunc) my_compare, memory );
  *     ....
  *   }
  */
  FT_BASE( FT_Error )
  ft_hash_init( FT_Hash              table,
                FT_Hash_EqualFunc  compare,
                FT_Memory            memory );


 /****************************************************************
  *
  * @function: ft_hash_lookup
  *
  * @description:
  *   search a hash table to find a node corresponding to a given
  *   key.
  *
  * @input:
  *   table   :: handle to target hash table structure
  *   keynode :: handle to a reference hash node that will be
  *              only used for key comparisons with the table's
  *              elements
  *
  * @return:
  *   a pointer-to-hash-node value, which must be used as followed:
  *
  *   - if '*result' is NULL, the key wasn't found in the hash
  *     table. The value of 'result' can be used to add new elements
  *     through @ft_hash_add however..
  *
  *   - if '*result' is not NULL, it's a handle to the first table
  *     node that corresponds to the search key. The value of 'result'
  *     can be used to remove this element through @ft_hash_remove
  *
  * @note:
  *   here is an example:
  *
  *   {
  *     // maps a string to an integer with a hash table
  *     // returns -1 in case of failure
  *     //
  *     int  my_lookup( FT_Hash      table,
  *                     const char*  key )
  *     {
  *       MyNode*    pnode;
  *       MyNodeRec  noderec;
  *
  *       // set-up key node. It's 'hash' and 'key' fields must
  *       // be set correctly.. we ignore 'link' and 'value'
  *       //
  *       noderec.hnode.hash = strhash( key );
  *       noderec.key        = key;
  *
  *       // perform search - return value
  *       //
  *       pnode = (MyNode) ft_hash_lookup( table, &noderec );
  *       if ( *pnode )
  *       {
  *         // we found it
  *         return (*pnode)->value;
  *       }
  *       return -1;
  *     }
  *   }
  */
  FT_BASE_DEF( FT_HashLookup )
  ft_hash_lookup( FT_Hash      table,
                  FT_HashNode  keynode );


 /****************************************************************
  *
  * @function: ft_hash_add
  *
  * @description:
  *   add a new node to a dynamic hash table. the user must
  *   call @ft_hash_lookup and allocate a new node before calling
  *   this function.
  *
  * @input:
  *   table    :: hash table handle
  *   lookup   :: pointer-to-hash-node value returned by @ft_hash_lookup
  *   new_node :: handle to new hash node. All its fields must be correctly
  *               set, including 'hash'.
  *
  * @return:
  *   error code. 0 means success
  *
  * @note:
  *   this function should always be used _after_ a call to @ft_hash_lookup
  *   that returns a pointer to a NULL  handle. Here's an example:
  *
  *   {
  *     // sets the value corresponding to a given string key
  *     //
  *     void  my_set( FT_Hash      table,
  *                   const char*  key,
  *                   int          value )
  *     {
  *       MyNode*    pnode;
  *       MyNodeRec  noderec;
  *       MyNode     node;
  *
  *       // set-up key node. It's 'hash' and 'key' fields must
  *       // be set correctly..
  *       noderec.hnode.hash = strhash( key );
  *       noderec.key        = key;
  *
  *       // perform search - return value
  *       pnode = (MyNode) ft_hash_lookup( table, &noderec );
  *       if ( *pnode )
  *       {
  *         // we found it, simply replace the value in the node
  *         (*pnode)->value = value;
  *         return;
  *       }
  *
  *       // allocate a new node - and set it up
  *       node = (MyNode) malloc( sizeof(*node) );
  *       if ( node == NULL ) .....
  *
  *       node->hnode.hash = noderec.hnode.hash;
  *       node->key        = key;
  *       node->value      = value;
  *
  *       // add it to the hash table
  *       error = ft_hash_add( table, pnode, node );
  *       if (error) ....
  *     }
  */
  FT_BASE( FT_Error )
  ft_hash_add( FT_Hash        table,
               FT_HashLookup  lookup,
               FT_HashNode    new_node );


 /****************************************************************
  *
  * @function: ft_hash_remove
  *
  * @description:
  *   try to remove the node corresponding to a given key from
  *   a hash table. This must be called after @ft_hash_lookup
  *
  * @input:
  *   table   :: hash table handle
  *   lookup  :: pointer-to-hash-node value returned by @ft_hash_lookup
  *
  * @note:
  *   this function doesn't free the node itself !! Here's an example:
  *
  *   {
  *     // sets the value corresponding to a given string key
  *     //
  *     void  my_remove( FT_Hash      table,
  *                      const char*  key )
  *     {
  *       MyNodeRec  noderec;
  *       MyNode     node;
  *
  *       noderec.hnode.hash = strhash(key);
  *       noderec.key        = key;
  *       node               = &noderec;
  *
  *       pnode = ft_hash_lookup( table, &noderec );
  *       node  = *pnode;
  *       if ( node != NULL )
  *       {
  *         error = ft_hash_remove( table, pnode );
  *         if ( !error )
  *           free( node );
  *       }
  *     }
  *   }
  */
  FT_BASE( FT_Error )
  ft_hash_remove( FT_Hash        table,
                  FT_HashLookup  lookup );



 /****************************************************************
  *
  * @function: ft_hash_get_size
  *
  * @description:
  *   return the number of elements in a given hash table
  *
  * @input:
  *   table   :: handle to target hash table structure
  *
  * @return:
  *   number of elements. 0 if empty
  */
  FT_BASE( FT_UInt )
  ft_hash_get_size( FT_Hash  table );



 /****************************************************************
  *
  * @functype: FT_Hash_ForeachFunc
  *
  * @description:
  *   a function used to iterate over all elements of a given
  *   hash table
  *
  * @input:
  *   node :: handle to target @FT_HashNodeRec node structure
  *   data :: optional argument to iteration routine
  *
  * @also:  @ft_hash_foreach
  */
  typedef void  (*FT_Hash_ForeachFunc)( const FT_HashNode  node,
                                        const FT_Pointer   data );


 /****************************************************************
  *
  * @function: ft_hash_foreach
  *
  * @description:
  *   parse over all elements in a hash table
  *
  * @input:
  *   table        :: handle to target hash table structure
  *   foreach_func :: iteration routine called for each element
  *   foreach_data :: optional argument to the iteration routine
  *
  * @note:
  *   this function is often used to release all elements from a
  *   hash table. See the example given for @ft_hash_done
  */
  FT_BASE( void )
  ft_hash_foreach( FT_Hash              table,
                   FT_Hash_ForeachFunc  foreach_func,
                   const FT_Pointer     foreach_data );



 /****************************************************************
  *
  * @function: ft_hash_done
  *
  * @description:
  *   finalize a given hash table
  *
  * @input:
  *   table     :: handle to target hash table structure
  *   node_func :: optional iteration function pointer. this
  *                can be used to destroy all nodes explicitely
  *   node_data :: optional argument to the node iterator
  *
  * @note:
  *   this function simply frees the hash table's buckets.
  *   you probably will need to call @ft_hash_foreach to
  *   destroy all its elements before @ft_hash_done, as in
  *   the following example:
  *
  *   {
  *     static void  my_node_clear( const MyNode  node )
  *     {
  *       free( node );
  *     }
  *
  *     static void  my_done( FT_Hash  table )
  *     {
  *       ft_hash_done( table, (FT_Hash_ForeachFunc) my_node_clear, NULL );
  *     }
  *   }
  */
  FT_BASE( void )
  ft_hash_done( FT_Hash              table,
                FT_Hash_ForeachFunc  item_func,
                const FT_Pointer     item_data );

 /* */

 /* compute bucket index from hash value in a dynamic hash table */
 /* this is only used to break encapsulation to speed lookups in */
 /* the FreeType cache manager !!                                */
 /*                                                              */

#define  FT_HASH_COMPUTE_INDEX(_table,_hash,_index)                  \
             {                                                       \
               FT_UInt  _mask  = (_table)->mask;                     \
               FT_UInt  _hash0 = (_hash);                            \
                                                                     \
               (_index) = (FT_UInt)( (_hash0) & _mask ) );           \
               if ( (_index) < (_table)->p )                         \
                 (_index) = (FT_uInt)( (_hash0) & ( 2*_mask+1 ) );   \
             }


FT_END_HEADER

#endif /* __FT_HASH_H__ */
