#include <ft2build.h>
#include FT_INTERNAL_OBJECT_H
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_OBJECTS_H

#define  FT_MAGIC_DEATH   0xDEADdead
#define  FT_MAGIC_CLASS   0x12345678

#define  FT_TYPE_HASH(x)  (( (FT_UInt32)(x) >> 2 )^( (FT_UInt32)(x) >> 10 ))

#define  FT_OBJECT_CHECK(o)                                  \
           ( FT_OBJECT(o)               != NULL           && \
             FT_OBJECT(o)->clazz        != NULL           && \
             FT_OBJECT(o)->ref_count    >= 1              && \
             FT_OBJECT(o)->clazz->magic == FT_MAGIC_CLASS )

#define  FT_CLASS_CHECK(c)  \
           ( FT_CLASS(c) != NULL && FT_CLASS(c)->magic == FT_MAGIC_CLASS )

#define  FT_ASSERT_IS_CLASS(c)  FT_ASSERT( FT_CLASS_CHECK(c) )

 /*******************************************************************/
 /*******************************************************************/
 /*****                                                         *****/
 /*****                                                         *****/
 /*****                  M E T A - C L A S S                    *****/
 /*****                                                         *****/
 /*****                                                         *****/
 /*******************************************************************/
 /*******************************************************************/

 /* forward declaration */
  FT_BASE_DEF( FT_Error )
  ft_metaclass_init( FT_MetaClass  meta,
                     FT_Library    library );

  /* forward declaration */
  FT_BASE_DEF( void )
  ft_metaclass_done( FT_MetaClass  meta );


  /* class type for the meta-class itself */
  static const FT_TypeRec  ft_meta_class_type =
  {
    "FT2.MetaClass",
    NULL,

    sizeof( FT_MetaClassRec ),
    (FT_Object_InitFunc)  ft_metaclass_init,
    (FT_Object_DoneFunc)  ft_metaclass_done,

    sizeof( FT_ClassRec ),
    (FT_Object_InitFunc)  NULL,
    (FT_Object_DoneFunc)  NULL
  };




 /* destroy a given class */
  static void
  ft_class_hnode_destroy( FT_ClassHNode  node )
  {
    FT_Class   clazz  = node->clazz;
    FT_Memory  memory = clazz->memory;

    if ( clazz->class_done )
      clazz->class_done( (FT_Object) clazz );

    FT_FREE( clazz );

    node->clazz = NULL;
    node->type  = NULL;

    FT_FREE( node );
  }


  static FT_Int
  ft_type_equal( FT_Type  type1,
                 FT_Type  type2 )
  {
    if ( type1 == type2 )
      goto Ok;

    if ( type1 == NULL || type2 == NULL )
      goto Fail;

    /* compare parent types */
    if ( type1->super != type2->super )
    {
      if ( type1->super == NULL           ||
           type2->super == NULL           ||
           !ft_type_equal( type1, type2 ) )
        goto Fail;
    }

    /* compare type names */
    if ( type1->name != type2->name )
    {
      if ( type1->name == NULL                        ||
           type2->name == NULL                        ||
           ft_strcmp( type1->name, type2->name ) != 0 )
        goto Fail;
    }

    /* compare the other type fields */
    if ( type1->class_size != type2->class_size ||
         type1->class_init != type2->class_init ||
         type1->class_done != type2->class_done ||
         type1->obj_size   != type2->obj_size   ||
         type1->obj_init   != type2->obj_init   ||
         type1->obj_done   != type2->obj_done   )
      goto Fail;

  Ok:
    return 1;

  Fail:
    return 0;
  }


  static FT_Int
  ft_class_hnode_equal( const FT_ClassHNode  node1,
                        const FT_ClassHNode  node2 )
  {
    FT_Type  type1 = node1->type;
    FT_Type  type2 = node2->type;

    /* comparing the pointers should work in 99% of cases */
    return ( type1 == type2 ) ? 1 : ft_type_equal( type1, type2 );
  }


  FT_BASE_DEF( void )
  ft_metaclass_done( FT_MetaClass  meta )
  {
    /* clear all classes */
    ft_hash_done( &meta->type_to_class,
                  (FT_Hash_ForeachFunc) ft_class_hnode_destroy,
                   NULL );

    meta->clazz.object.clazz     = NULL;
    meta->clazz.object.ref_count = 0;
    meta->clazz.magic            = FT_MAGIC_DEATH;
  }


  FT_BASE_DEF( FT_Error )
  ft_metaclass_init( FT_MetaClass  meta,
                     FT_Library    library )
  {
    FT_ClassRec*  clazz = (FT_ClassRec*) &meta->clazz;

    /* the meta-class is its OWN class !! */
    clazz->object.clazz     = (FT_Class) clazz;
    clazz->object.ref_count = 1;
    clazz->magic            = FT_MAGIC_CLASS;
    clazz->library          = library;
    clazz->memory           = library->memory;
    clazz->type             = &ft_meta_class_type;
    clazz->info             = NULL;

    clazz->class_done       = (FT_Object_DoneFunc) ft_metaclass_done;

    clazz->obj_size         = sizeof( FT_ClassRec );
    clazz->obj_init         = NULL;
    clazz->obj_done         = NULL;

    return ft_hash_init( &meta->type_to_class,
                        (FT_Hash_EqualFunc) ft_class_hnode_equal,
                        library->memory );
  }


 /* find or create the class corresponding to a given type */
 /* note that this function will retunr NULL in case of    */
 /* memory overflow                                        */
 /*                                                        */
  static FT_Class
  ft_metaclass_get_class( FT_MetaClass  meta,
                          FT_Type       ctype )
  {
    FT_ClassHNodeRec   keynode, *node, **pnode;
    FT_Memory          memory;
    FT_ClassRec*       clazz;
    FT_Class           parent;
    FT_Error           error;

    keynode.hnode.hash = FT_TYPE_HASH( ctype );
    keynode.type       = ctype;

    pnode = (FT_ClassHNode*) ft_hash_lookup( &meta->type_to_class,
                                             (FT_HashNode) &keynode );
    node  = *pnode;
    if ( node != NULL )
    {
      clazz = (FT_ClassRec*) node->clazz;
      goto Exit;
    }

    memory = FT_CLASS__MEMORY(meta);
    clazz  = NULL;
    parent = NULL;
    if ( ctype->super != NULL )
    {
      FT_ASSERT( ctype->super->class_size <= ctype->class_size );
      FT_ASSERT( ctype->super->obj_size   <= ctype->obj_size   );

      parent = ft_metaclass_get_class( meta, ctype->super );
    }

    if ( !FT_NEW( node ) )
    {
      if ( !FT_ALLOC( clazz, ctype->class_size ) )
      {
        if ( parent )
          FT_MEM_COPY( (FT_ClassRec*)clazz, parent, parent->type->class_size );

        clazz->object.clazz     = (FT_Class) meta;
        clazz->object.ref_count = 1;

        clazz->memory  = memory;
        clazz->library = FT_CLASS__LIBRARY(meta);
        clazz->super   = parent;
        clazz->type    = ctype;
        clazz->info    = NULL;
        clazz->magic   = FT_MAGIC_CLASS;

        clazz->class_done = ctype->class_done;
        clazz->obj_size   = ctype->obj_size;
        clazz->obj_init   = ctype->obj_init;
        clazz->obj_done   = ctype->obj_done;

        if ( parent )
        {
          if ( clazz->class_done == NULL )
            clazz->class_done = parent->class_done;

          if ( clazz->obj_init == NULL )
            clazz->obj_init = parent->obj_init;

          if ( clazz->obj_done == NULL )
            clazz->obj_done = parent->obj_done;
        }

        /* find class initializer, if any */
        {
          FT_Type             ztype = ctype;
          FT_Object_InitFunc  cinit = NULL;

          do
          {
            cinit = ztype->class_init;
            if ( cinit != NULL )
              break;

            ztype = ztype->super;
          }
          while ( ztype != NULL );

          /* then call it when needed */
          if ( cinit != NULL )
            error = cinit( (FT_Object) clazz, NULL );
        }
      }

      if (error)
      {
        if ( clazz )
        {
          /* we always call the class destructor when    */
          /* an error was detected in the constructor !! */
          if ( clazz->class_done )
            clazz->class_done( (FT_Object) clazz );

          FT_FREE( clazz );
        }
        FT_FREE( node );
      }
    }

  Exit:
    return  (FT_Class) clazz;
  }














  FT_BASE_DEF( FT_Int )
  ft_object_check( FT_Pointer  obj )
  {
    return FT_OBJECT_CHECK(obj);
  }


  FT_BASE_DEF( FT_Int )
  ft_object_is_a( FT_Pointer  obj,
                  FT_Class    clazz )
  {
    if ( FT_OBJECT_CHECK(obj) )
    {
      FT_Class   c = FT_OBJECT__CLASS(obj);

      do
      {
        if ( c == clazz )
          return 1;

        c = c->super;
      }
      while ( c == NULL );

      return (clazz == NULL);
    }
    return 0;
  }


  FT_BASE_DEF( FT_Error )
  ft_object_create( FT_Object  *pobject,
                    FT_Class    clazz,
                    FT_Pointer  init_data )
  {
    FT_Memory  memory;
    FT_Error   error;
    FT_Object  obj;

    FT_ASSERT_IS_CLASS(clazz);

    memory = FT_CLASS__MEMORY(clazz);
    if ( !FT_ALLOC( obj, clazz->obj_size ) )
    {
      obj->clazz     = clazz;
      obj->ref_count = 1;

      if ( clazz->obj_init )
      {
        error = clazz->obj_init( obj, init_data );
        if ( error )
        {
          /* IMPORTANT: call the destructor when an error  */
          /*            was detected in the constructor !! */
          if ( clazz->obj_done )
            clazz->obj_done( obj );

          FT_FREE( obj );
        }
      }
    }
    *pobject = obj;
    return error;
  }


  FT_BASE_DEF( FT_Class )
  ft_class_find_by_type( FT_Type     type,
                         FT_Library  library )
  {
    FT_MetaClass  meta = &library->meta_class;

    return ft_metaclass_get_class( meta, type );
  }


  FT_BASE_DEF( FT_Error )
  ft_object_create_from_type( FT_Object  *pobject,
                              FT_Type     type,
                              FT_Pointer  init_data,
                              FT_Library  library )
  {
    FT_Class  clazz;
    FT_Error  error;

    clazz = ft_class_find_by_type( type, library );
    if ( clazz )
      error = ft_object_create( pobject, clazz, init_data );
    else
    {
      *pobject = NULL;
      error    = FT_Err_Out_Of_Memory;
    }

    return error;
  }
