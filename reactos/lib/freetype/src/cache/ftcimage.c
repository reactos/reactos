/***************************************************************************/
/*                                                                         */
/*  ftcimage.c                                                             */
/*                                                                         */
/*    FreeType Image cache (body).                                         */
/*                                                                         */
/*  Copyright 2000-2001 by                                                 */
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
#include FT_CACHE_IMAGE_H
#include FT_CACHE_INTERNAL_GLYPH_H
#include FT_INTERNAL_MEMORY_H

#include "ftcerror.h"


  /* the FT_Glyph image node type */
  typedef struct  FTC_ImageNodeRec_
  {
    FTC_GlyphNodeRec  gnode;
    FT_Glyph          glyph;

  } FTC_ImageNodeRec, *FTC_ImageNode;


#define FTC_IMAGE_NODE( x )         ( (FTC_ImageNode)( x ) )
#define FTC_IMAGE_NODE_GINDEX( x )  FTC_GLYPH_NODE_GINDEX( x )


  /* the glyph image query */
  typedef struct  FTC_ImageQueryRec_
  {
    FTC_GlyphQueryRec  gquery;
    FTC_ImageTypeRec   type;

  } FTC_ImageQueryRec, *FTC_ImageQuery;


#define FTC_IMAGE_QUERY( x )  ( (FTC_ImageQuery)( x ) )


  /* the glyph image set type */
  typedef struct  FTC_ImageFamilyRec_
  {
    FTC_GlyphFamilyRec  gfam;
    FTC_ImageTypeRec    type;

  } FTC_ImageFamilyRec, *FTC_ImageFamily;


#define FTC_IMAGE_FAMILY( x )         ( (FTC_ImageFamily)( x ) )
#define FTC_IMAGE_FAMILY_MEMORY( x )  FTC_GLYPH_FAMILY_MEMORY( &(x)->gfam )


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    GLYPH IMAGE NODES                          *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  /* finalize a given glyph image node */
  FT_CALLBACK_DEF( void )
  ftc_image_node_done( FTC_ImageNode  inode,
                       FTC_Cache      cache )
  {
    if ( inode->glyph )
    {
      FT_Done_Glyph( inode->glyph );
      inode->glyph = NULL;
    }

    ftc_glyph_node_done( FTC_GLYPH_NODE( inode ), cache );
  }


  /* initialize a new glyph image node */
  FT_CALLBACK_DEF( FT_Error )
  ftc_image_node_init( FTC_ImageNode   inode,
                       FTC_GlyphQuery  gquery,
                       FTC_Cache       cache )
  {
    FTC_ImageFamily  ifam = FTC_IMAGE_FAMILY( gquery->query.family );
    FT_Error         error;
    FT_Face          face;
    FT_Size          size;


    /* initialize its inner fields */
    ftc_glyph_node_init( FTC_GLYPH_NODE( inode ),
                         gquery->gindex,
                         FTC_GLYPH_FAMILY( ifam ) );

    /* we will now load the glyph image */
    error = FTC_Manager_Lookup_Size( FTC_FAMILY( ifam )->cache->manager,
                                     &ifam->type.font,
                                     &face, &size );
    if ( !error )
    {
      FT_UInt  gindex = FTC_GLYPH_NODE_GINDEX( inode );


      error = FT_Load_Glyph( face, gindex, ifam->type.flags );
      if ( !error )
      {
        if ( face->glyph->format == FT_GLYPH_FORMAT_BITMAP  ||
             face->glyph->format == FT_GLYPH_FORMAT_OUTLINE )
        {
          /* ok, copy it */
          FT_Glyph  glyph;


          error = FT_Get_Glyph( face->glyph, &glyph );
          if ( !error )
          {
            inode->glyph = glyph;
            goto Exit;
          }
        }
        else
          error = FTC_Err_Invalid_Argument;
      }
    }

    /* in case of error */
    ftc_glyph_node_done( FTC_GLYPH_NODE(inode), cache );

  Exit:
    return error;
  }


  FT_CALLBACK_DEF( FT_ULong )
  ftc_image_node_weight( FTC_ImageNode  inode )
  {
    FT_ULong  size  = 0;
    FT_Glyph  glyph = inode->glyph;


    switch ( glyph->format )
    {
    case FT_GLYPH_FORMAT_BITMAP:
      {
        FT_BitmapGlyph  bitg;


        bitg = (FT_BitmapGlyph)glyph;
        size = bitg->bitmap.rows * labs( bitg->bitmap.pitch ) +
               sizeof ( *bitg );
      }
      break;

    case FT_GLYPH_FORMAT_OUTLINE:
      {
        FT_OutlineGlyph  outg;


        outg = (FT_OutlineGlyph)glyph;
        size = outg->outline.n_points *
                 ( sizeof ( FT_Vector ) + sizeof ( FT_Byte ) ) +
               outg->outline.n_contours * sizeof ( FT_Short ) +
               sizeof ( *outg );
      }
      break;

    default:
      ;
    }

    size += sizeof ( *inode );
    return size;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    GLYPH IMAGE SETS                           *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  FT_CALLBACK_DEF( FT_Error )
  ftc_image_family_init( FTC_ImageFamily  ifam,
                         FTC_ImageQuery   iquery,
                         FTC_Cache        cache )
  {
    FTC_Manager  manager = cache->manager;
    FT_Error     error;
    FT_Face      face;


    ifam->type = iquery->type;

    /* we need to compute "iquery.item_total" now */
    error = FTC_Manager_Lookup_Face( manager,
                                     iquery->type.font.face_id,
                                     &face );
    if ( !error )
    {
      error = ftc_glyph_family_init( FTC_GLYPH_FAMILY( ifam ),
                                     FTC_IMAGE_TYPE_HASH( &ifam->type ),
                                     1,
                                     face->num_glyphs,
                                     FTC_GLYPH_QUERY( iquery ),
                                     cache );
    }

    return error;
  }


  FT_CALLBACK_DEF( FT_Bool )
  ftc_image_family_compare( FTC_ImageFamily  ifam,
                            FTC_ImageQuery   iquery )
  {
    FT_Bool  result;


    result = FT_BOOL( FTC_IMAGE_TYPE_COMPARE( &ifam->type, &iquery->type ) );
    if ( result )
      FTC_GLYPH_FAMILY_FOUND( ifam, iquery );

    return result;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    GLYPH IMAGE CACHE                          *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/



  FT_CALLBACK_TABLE_DEF
  const FTC_Cache_ClassRec  ftc_image_cache_class =
  {
    sizeof ( FTC_CacheRec ),
    (FTC_Cache_InitFunc) ftc_cache_init,
    (FTC_Cache_ClearFunc)ftc_cache_clear,
    (FTC_Cache_DoneFunc) ftc_cache_done,

    sizeof ( FTC_ImageFamilyRec ),
    (FTC_Family_InitFunc)   ftc_image_family_init,
    (FTC_Family_CompareFunc)ftc_image_family_compare,
    (FTC_Family_DoneFunc)   ftc_glyph_family_done,

    sizeof ( FTC_ImageNodeRec ),
    (FTC_Node_InitFunc)   ftc_image_node_init,
    (FTC_Node_WeightFunc) ftc_image_node_weight,
    (FTC_Node_CompareFunc)ftc_glyph_node_compare,
    (FTC_Node_DoneFunc)   ftc_image_node_done
  };


  /* documentation is in ftcimage.h */

  FT_EXPORT_DEF( FT_Error )
  FTC_ImageCache_New( FTC_Manager      manager,
                      FTC_ImageCache  *acache )
  {
    return FTC_Manager_Register_Cache(
             manager,
             (FTC_Cache_Class)&ftc_image_cache_class,
             FTC_CACHE_P( acache ) );
  }


  /* documentation is in ftcimage.h */

  FT_EXPORT_DEF( FT_Error )
  FTC_ImageCache_Lookup( FTC_ImageCache  cache,
                         FTC_ImageType   type,
                         FT_UInt         gindex,
                         FT_Glyph       *aglyph,
                         FTC_Node       *anode )
  {
    FTC_ImageQueryRec  iquery;
    FTC_ImageNode      node;
    FT_Error           error;


    /* some argument checks are delayed to ftc_cache_lookup */
    if ( !aglyph )
      return FTC_Err_Invalid_Argument;

    if ( anode )
      *anode  = NULL;

    iquery.gquery.gindex = gindex;
    iquery.type          = *type;

    error = ftc_cache_lookup( FTC_CACHE( cache ),
                              FTC_QUERY( &iquery ),
                              (FTC_Node*)&node );
    if ( !error )
    {
      *aglyph = node->glyph;

      if ( anode )
      {
        *anode = (FTC_Node)node;
        FTC_NODE( node )->ref_count++;
      }
    }

    return error;
  }


  /* backwards-compatibility functions */

  FT_EXPORT_DEF( FT_Error )
  FTC_Image_Cache_New( FTC_Manager       manager,
                       FTC_Image_Cache  *acache )
  {
    return FTC_ImageCache_New( manager, (FTC_ImageCache*)acache );
  }


  FT_EXPORT_DEF( FT_Error )
  FTC_Image_Cache_Lookup( FTC_Image_Cache  icache,
                          FTC_Image_Desc*  desc,
                          FT_UInt          gindex,
                          FT_Glyph        *aglyph )
  {
    FTC_ImageTypeRec  type0;


    if ( !desc )
      return FTC_Err_Invalid_Argument;

    type0.font = desc->font;

    /* convert image type flags to load flags */
    {
      FT_UInt  load_flags = FT_LOAD_DEFAULT;
      FT_UInt  type       = desc->image_type;


      /* determine load flags, depending on the font description's */
      /* image type                                                */

      if ( ftc_image_format( type ) == ftc_image_format_bitmap )
      {
        if ( type & ftc_image_flag_monochrome )
          load_flags |= FT_LOAD_MONOCHROME;

        /* disable embedded bitmaps loading if necessary */
        if ( type & ftc_image_flag_no_sbits )
          load_flags |= FT_LOAD_NO_BITMAP;
      }
      else
      {
        /* we want an outline, don't load embedded bitmaps */
        load_flags |= FT_LOAD_NO_BITMAP;

        if ( type & ftc_image_flag_unscaled )
          load_flags |= FT_LOAD_NO_SCALE;
      }

      /* always render glyphs to bitmaps */
      load_flags |= FT_LOAD_RENDER;

      if ( type & ftc_image_flag_unhinted )
        load_flags |= FT_LOAD_NO_HINTING;

      if ( type & ftc_image_flag_autohinted )
        load_flags |= FT_LOAD_FORCE_AUTOHINT;

      type0.flags = load_flags;
    }

    return FTC_ImageCache_Lookup( (FTC_ImageCache)icache,
                                   &type0,
                                   gindex,
                                   aglyph,
                                   NULL );
  }


/* END */
