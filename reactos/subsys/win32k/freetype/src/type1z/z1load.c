/***************************************************************************/
/*                                                                         */
/*  z1load.c                                                               */
/*                                                                         */
/*    Experimental Type 1 font loader (body).                              */
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
  /* This is the new and improved Type 1 data loader for FreeType 2.  The  */
  /* old loader has several problems: it is slow, complex, difficult to    */
  /* maintain, and contains incredible hacks to make it accept some        */
  /* ill-formed Type 1 fonts without hiccup-ing.  Moreover, about 5% of    */
  /* the Type 1 fonts on my machine still aren't loaded correctly by it.   */
  /*                                                                       */
  /* This version is much simpler, much faster and also easier to read and */
  /* maintain by a great order of magnitude.  The idea behind it is to     */
  /* _not_ try to read the Type 1 token stream with a state machine (i.e.  */
  /* a Postscript-like interpreter) but rather to perform simple pattern   */
  /* matching.                                                             */
  /*                                                                       */
  /* Indeed, nearly all data definitions follow a simple pattern like      */
  /*                                                                       */
  /*  ... /Field <data> ...                                                */
  /*                                                                       */
  /* where <data> can be a number, a boolean, a string, or an array of     */
  /* numbers.  There are a few exceptions, namely the encoding, font name, */
  /* charstrings, and subrs; they are handled with a special pattern       */
  /* matching routine.                                                     */
  /*                                                                       */
  /* All other common cases are handled very simply.  The matching rules   */
  /* are defined in the file `t1tokens.h' through the use of several       */
  /* macros calls PARSE_XXX.                                               */
  /*                                                                       */
  /* This file is included twice here; the first time to generate parsing  */
  /* callback functions, the second to generate a table of keywords (with  */
  /* pointers to the associated callback).                                 */
  /*                                                                       */
  /* The function `parse_dict' simply scans *linearly* a given dictionary  */
  /* (either the top-level or private one) and calls the appropriate       */
  /* callback when it encounters an immediate keyword.                     */
  /*                                                                       */
  /* This is by far the fastest way one can find to parse and read all     */
  /* data.                                                                 */
  /*                                                                       */
  /* This led to tremendous code size reduction.  Note that later, the     */
  /* glyph loader will also be _greatly_ simplified, and the automatic     */
  /* hinter will replace the clumsy `t1hinter'.                            */
  /*                                                                       */
  /*************************************************************************/


#include <freetype/internal/ftdebug.h>
#include <freetype/config/ftconfig.h>
#include <freetype/ftmm.h>

#include <freetype/internal/t1types.h>
#include <freetype/internal/t1errors.h>


#ifdef FT_FLAT_COMPILE

#include "z1load.h"

#else

#include <freetype/src/type1z/z1load.h>

#endif


#include <string.h>     /* for strncmp(), strcmp() */
#include <ctype.h>      /* for isalnum()           */


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_z1load


#ifndef Z1_CONFIG_OPTION_NO_MM_SUPPORT


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    MULTIPLE MASTERS SUPPORT                   *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  static
  FT_Error  t1_allocate_blend( T1_Face  face,
                               FT_UInt  num_designs,
                               FT_UInt  num_axis )
  {
    T1_Blend*  blend;
    FT_Memory  memory = face->root.memory;
    FT_Error   error  = 0;


    blend = face->blend;
    if ( !blend )
    {
      if ( ALLOC( blend, sizeof ( *blend ) ) )
        goto Exit;

      face->blend = blend;
    }

    /* allocate design data if needed */
    if ( num_designs > 0 )
    {
      if ( blend->num_designs == 0 )
      {
        FT_UInt  nn;
        

        /* allocate the blend `private' and `font_info' dictionaries */
        if ( ALLOC_ARRAY( blend->font_infos[1], num_designs, T1_FontInfo )  ||
             ALLOC_ARRAY( blend->privates[1], num_designs, T1_Private )     ||
             ALLOC_ARRAY( blend->weight_vector, num_designs * 2, FT_Fixed ) )
          goto Exit;

        blend->default_weight_vector = blend->weight_vector + num_designs;

        blend->font_infos[0] = &face->type1.font_info;
        blend->privates  [0] = &face->type1.private_dict;
        
        for ( nn = 2; nn <= num_designs; nn++ )
        {
          blend->privates[nn]   = blend->privates  [nn - 1] + 1;
          blend->font_infos[nn] = blend->font_infos[nn - 1] + 1;
        }
        
        blend->num_designs   = num_designs;
      }
      else if ( blend->num_designs != num_designs )
        goto Fail;
    }

    /* allocate axis data if needed */
    if ( num_axis > 0 )
    {
      if ( blend->num_axis != 0 && blend->num_axis != num_axis )
        goto Fail;

      blend->num_axis = num_axis;
    }

    /* allocate the blend design pos table if needed */
    num_designs = blend->num_designs;
    num_axis    = blend->num_axis;
    if ( num_designs && num_axis && blend->design_pos[0] == 0 )
    {
      FT_UInt  n;


      if ( ALLOC_ARRAY( blend->design_pos[0],
                        num_designs * num_axis, FT_Fixed ) )
        goto Exit;

      for ( n = 1; n < num_designs; n++ )
        blend->design_pos[n] = blend->design_pos[0] + num_axis * n;
    }

  Exit:
    return error;

  Fail:
    error = -1;
    goto Exit;
  }


  LOCAL_FUNC
  FT_Error  Z1_Get_Multi_Master( T1_Face           face,
                                 FT_Multi_Master*  master )
  {
    T1_Blend*  blend = face->blend;
    FT_UInt    n;
    FT_Error   error;


    error = T1_Err_Invalid_Argument;

    if ( blend )
    {
      master->num_axis    = blend->num_axis;
      master->num_designs = blend->num_designs;

      for ( n = 0; n < blend->num_axis; n++ )
      {
        FT_MM_Axis*    axis = master->axis + n;
        T1_DesignMap*  map = blend->design_map + n;


        axis->name    = blend->axis_names[n];
        axis->minimum = map->design_points[0];
        axis->maximum = map->design_points[map->num_points - 1];
      }
      error = 0;
    }
    return error;
  }


  LOCAL_FUNC
  FT_Error  Z1_Set_MM_Blend( T1_Face    face,
                             FT_UInt    num_coords,
                             FT_Fixed*  coords )
  {
    T1_Blend*  blend = face->blend;
    FT_Error   error;
    FT_UInt    n, m;


    error = T1_Err_Invalid_Argument;

    if ( blend && blend->num_axis == num_coords )
    {
      /* recompute the weight vector from the blend coordinates */
      error = FT_Err_Ok;

      for ( n = 0; n < blend->num_designs; n++ )
      {
        FT_Fixed  result = 0x10000L;  /* 1.0 fixed */


        for ( m = 0; m < blend->num_axis; m++ )
        {
          FT_Fixed  factor;


          /* get current blend axis position */
          factor = coords[m];
          if ( factor < 0 )        factor = 0;
          if ( factor > 0x10000L ) factor = 0x10000L;

          if ( ( n & ( 1 << m ) ) == 0 )
            factor = 0x10000L - factor;

          result = FT_MulFix( result, factor );
        }
        blend->weight_vector[n] = result;
      }

      error = FT_Err_Ok;
    }
    return error;
  }


  LOCAL_FUNC
  FT_Error  Z1_Set_MM_Design( T1_Face   face,
                              FT_UInt   num_coords,
                              FT_Long*  coords )
  {
    T1_Blend*  blend = face->blend;
    FT_Error   error;
    FT_UInt    n, p;


    error = T1_Err_Invalid_Argument;
    if ( blend && blend->num_axis == num_coords )
    {
      /* compute the blend coordinates through the blend design map */
      FT_Fixed  final_blends[T1_MAX_MM_DESIGNS];


      for ( n = 0; n < blend->num_axis; n++ )
      {
        FT_Long        design  = coords[n];
        FT_Fixed       the_blend;
        T1_DesignMap*  map     = blend->design_map + n;
        FT_Fixed*      designs = map->design_points;
        FT_Fixed*      blends  = map->blend_points;
        FT_Int         before  = -1, after = -1;

        for ( p = 0; p < map->num_points; p++ )
        {
          FT_Fixed  p_design = designs[p];


          /* exact match ? */
          if ( design == p_design )
          {
            the_blend = blends[p];
            goto Found;
          }

          if ( design < p_design )
          {
            after = p;
            break;
          }

          before = p;
        }

        /* now, interpolate if needed */
        if ( before < 0 )
          the_blend = blends[0];

        else if ( after < 0 )
          the_blend = blends[map->num_points - 1];

        else
          the_blend = FT_MulDiv( design         - designs[before],
                                 blends [after] - blends [before],
                                 designs[after] - designs[before] );

      Found:
        final_blends[n] = the_blend;
      }

      error = Z1_Set_MM_Blend( face, num_coords, final_blends );
    }

    return error;
  }


  LOCAL_FUNC
  void  Z1_Done_Blend( T1_Face  face )
  {
    FT_Memory  memory = face->root.memory;
    T1_Blend*  blend  = face->blend;


    if ( blend )
    {
      FT_UInt  num_designs = blend->num_designs;
      FT_UInt  num_axis    = blend->num_axis;
      FT_UInt  n;


      /* release design pos table */
      FREE( blend->design_pos[0] );
      for ( n = 1; n < num_designs; n++ )
        blend->design_pos[n] = 0;

      /* release blend `private' and `font info' dictionaries */
      FREE( blend->privates[1] );
      FREE( blend->font_infos[1] );

      for ( n = 0; n < num_designs; n++ )
      {
        blend->privates  [n] = 0;
        blend->font_infos[n] = 0;
      }

      /* release weight vectors */
      FREE( blend->weight_vector );
      blend->default_weight_vector = 0;

      /* release axis names */
      for ( n = 0; n < num_axis; n++ )
        FREE( blend->axis_names[n] );

      /* release design map */
      for ( n = 0; n < num_axis; n++ )
      {
        T1_DesignMap*  dmap = blend->design_map + n;


        FREE( dmap->design_points );
        dmap->num_points = 0;
      }

      FREE( face->blend );
    }
  }


  static
  void  parse_blend_axis_types( T1_Face     face,
                                Z1_Loader*  loader )
  {
    Z1_Token_Rec  axis_tokens[ T1_MAX_MM_AXIS ];
    FT_Int        n, num_axis;
    FT_Error      error = 0;
    T1_Blend*     blend;
    FT_Memory     memory;


    /* take an array of objects */
    Z1_ToTokenArray( &loader->parser, axis_tokens,
                     T1_MAX_MM_AXIS, &num_axis );
    if ( num_axis <= 0 || num_axis > T1_MAX_MM_AXIS )
    {
      FT_ERROR(( "parse_blend_axis_types: incorrect number of axes: %d\n",
                 num_axis ));
      error = T1_Err_Invalid_File_Format;
      goto Exit;
    }

    /* allocate blend if necessary */
    error = t1_allocate_blend( face, 0, (FT_UInt)num_axis );
    if ( error )
      goto Exit;

    blend  = face->blend;
    memory = face->root.memory;

    /* each token is an immediate containing the name of the axis */
    for ( n = 0; n < num_axis; n++ )
    {
      Z1_Token_Rec*  token = axis_tokens + n;
      FT_Byte*       name;
      FT_Int         len;

      /* skip first slash, if any */
      if (token->start[0] == '/')
        token->start++;

      len = token->limit - token->start;
      if ( len <= 0 )
      {
        error = T1_Err_Invalid_File_Format;
        goto Exit;
      }

      if ( ALLOC( blend->axis_names[n], len + 1 ) )
        goto Exit;

      name = (FT_Byte*)blend->axis_names[n];
      MEM_Copy( name, token->start, len );
      name[len] = 0;
    }

  Exit:
    loader->parser.error = error;
  }


  static
  void  parse_blend_design_positions( T1_Face     face,
                                      Z1_Loader*  loader )
  {
    Z1_Token_Rec  design_tokens[ T1_MAX_MM_DESIGNS ];
    FT_Int        num_designs;
    FT_Int        num_axis;
    Z1_Parser*    parser = &loader->parser;

    FT_Error      error = 0;
    T1_Blend*     blend;


    /* get the array of design tokens - compute number of designs */
    Z1_ToTokenArray( parser, design_tokens, T1_MAX_MM_DESIGNS, &num_designs );
    if ( num_designs <= 0 || num_designs > T1_MAX_MM_DESIGNS )
    {
      FT_ERROR(( "parse_blend_design_positions:" ));
      FT_ERROR(( " incorrect number of designs: %d\n",
                 num_designs ));
      error = T1_Err_Invalid_File_Format;
      goto Exit;
    }

    {
      FT_Byte*  old_cursor = parser->cursor;
      FT_Byte*  old_limit  = parser->limit;
      FT_UInt   n;


      blend    = face->blend;
      num_axis = 0;  /* make compiler happy */

      for ( n = 0; n < (FT_UInt)num_designs; n++ )
      {
        Z1_Token_Rec   axis_tokens[ T1_MAX_MM_DESIGNS ];
        Z1_Token_Rec*  token;
        FT_Int         axis, n_axis;


        /* read axis/coordinates tokens */
        token = design_tokens + n;
        parser->cursor = token->start - 1;
        parser->limit  = token->limit + 1;
        Z1_ToTokenArray( parser, axis_tokens, T1_MAX_MM_AXIS, &n_axis );

        if ( n == 0 )
        {
          num_axis = n_axis;
          error = t1_allocate_blend( face, num_designs, num_axis );
          if ( error )
            goto Exit;
          blend = face->blend;
        }
        else if ( n_axis != num_axis )
        {
          FT_ERROR(( "parse_blend_design_positions: incorrect table\n" ));
          error = T1_Err_Invalid_File_Format;
          goto Exit;
        }

        /* now, read each axis token into the design position */
        for ( axis = 0; axis < n_axis; axis++ )
        {
          Z1_Token_Rec*  token2 = axis_tokens + axis;


          parser->cursor = token2->start;
          parser->limit  = token2->limit;
          blend->design_pos[n][axis] = Z1_ToFixed( parser, 0 );
        }
      }

      loader->parser.cursor = old_cursor;
      loader->parser.limit  = old_limit;
    }

  Exit:
    loader->parser.error = error;
  }


  static
  void  parse_blend_design_map( T1_Face     face,
                                Z1_Loader*  loader )
  {
    FT_Error      error  = 0;
    Z1_Parser*    parser = &loader->parser;
    T1_Blend*     blend;
    Z1_Token_Rec  axis_tokens[ T1_MAX_MM_AXIS ];
    FT_Int        n, num_axis;
    FT_Byte*      old_cursor;
    FT_Byte*      old_limit;
    FT_Memory     memory = face->root.memory;


    Z1_ToTokenArray( parser, axis_tokens, T1_MAX_MM_AXIS, &num_axis );
    if ( num_axis <= 0 || num_axis > T1_MAX_MM_AXIS )
    {
      FT_ERROR(( "parse_blend_design_map: incorrect number of axes: %d\n",
                 num_axis ));
      error = T1_Err_Invalid_File_Format;
      goto Exit;
    }
    old_cursor = parser->cursor;
    old_limit  = parser->limit;

    error = t1_allocate_blend( face, 0, num_axis );
    if ( error )
      goto Exit;
    blend = face->blend;

    /* now, read each axis design map */
    for ( n = 0; n < num_axis; n++ )
    {
      T1_DesignMap*   map = blend->design_map + n;
      Z1_Token_Rec*   token;
      FT_Int          p, num_points;


      token = axis_tokens + n;
      parser->cursor = token->start;
      parser->limit  = token->limit;

      /* count the number of map points */
      {
        FT_Byte*  p     = token->start;
        FT_Byte*  limit = token->limit;


        num_points = 0;
        for ( ; p < limit; p++ )
          if ( p[0] == '[' )
            num_points++;
      }
      if ( num_points <= 0 || num_points > T1_MAX_MM_MAP_POINTS )
      {
        FT_ERROR(( "parse_blend_design_map: incorrect table\n" ));
        error = T1_Err_Invalid_File_Format;
        goto Exit;
      }

      /* allocate design map data */
      if ( ALLOC_ARRAY( map->design_points, num_points * 2, FT_Fixed ) )
        goto Exit;
      map->blend_points = map->design_points + num_points;
      map->num_points   = (FT_Byte)num_points;

      for ( p = 0; p < num_points; p++ )
      {
        map->design_points[p] = Z1_ToInt( parser );
        map->blend_points [p] = Z1_ToFixed( parser, 0 );
      }
    }

    parser->cursor = old_cursor;
    parser->limit  = old_limit;

  Exit:
    parser->error = error;
  }


  static
  void  parse_weight_vector( T1_Face     face,
                             Z1_Loader*  loader )
  {
    FT_Error      error  = 0;
    Z1_Parser*    parser = &loader->parser;
    T1_Blend*     blend  = face->blend;
    Z1_Token_Rec  master;
    FT_UInt       n;
    FT_Byte*      old_cursor;
    FT_Byte*      old_limit;


    if ( !blend || blend->num_designs == 0 )
    {
      FT_ERROR(( "parse_weight_vector: too early!\n" ));
      error = T1_Err_Invalid_File_Format;
      goto Exit;
    }

    Z1_ToToken( parser, &master );
    if ( master.type != t1_token_array )
    {
      FT_ERROR(( "parse_weight_vector: incorrect format!\n" ));
      error = T1_Err_Invalid_File_Format;
      goto Exit;
    }

    old_cursor = parser->cursor;
    old_limit  = parser->limit;

    parser->cursor = master.start;
    parser->limit  = master.limit;

    for ( n = 0; n < blend->num_designs; n++ )
    {
      blend->default_weight_vector[n] =
      blend->weight_vector[n]         = Z1_ToFixed( parser, 0 );
    }

    parser->cursor = old_cursor;
    parser->limit  = old_limit;

  Exit:
    parser->error = error;
  }


  /* the keyword `/shareddict' appears in some multiple master fonts   */
  /* with a lot of Postscript garbage behind it (that's completely out */
  /* of spec!); we detect it and terminate the parsing                 */
  /*                                                                   */
  static
  void  parse_shared_dict( T1_Face     face,
                           Z1_Loader*  loader )
  {
    Z1_Parser*  parser = &loader->parser;

    FT_UNUSED( face );


    parser->cursor = parser->limit;
    parser->error  = 0;
  }

#endif /* Z1_CONFIG_OPTION_NO_MM_SUPPORT */


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                      TYPE 1 SYMBOL PARSING                    *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* First of all, define the token field static variables.  This is a set */
  /* of Z1_Field_Rec variables used later.                                 */
  /*                                                                       */
  /*************************************************************************/

#define Z1_NEW_STRING( _name, _field )              \
          static                                    \
          const Z1_Field_Rec  t1_field_ ## _field = \
            Z1_FIELD_STRING( T1TYPE, _field );

#define Z1_NEW_BOOL( _name, _field )                \
          static                                    \
          const Z1_Field_Rec  t1_field_ ## _field = \
            Z1_FIELD_BOOL( T1TYPE, _field );

#define Z1_NEW_NUM( _name, _field )                 \
          static                                    \
          const Z1_Field_Rec  t1_field_ ## _field = \
            Z1_FIELD_NUM( T1TYPE, _field );

#define Z1_NEW_FIXED( _name, _field )                 \
          static                                      \
          const Z1_Field_Rec  t1_field_ ## _field =   \
            Z1_FIELD_FIXED( T1TYPE, _field, _power );

#define Z1_NEW_NUM_TABLE( _name, _field, _max, _count )         \
          static                                                \
          const Z1_Field_Rec  t1_field_ ## _field =             \
            Z1_FIELD_NUM_ARRAY( T1TYPE, _field, _count, _max );

#define Z1_NEW_FIXED_TABLE( _name, _field, _max, _count )         \
          static                                                  \
          const Z1_Field_Rec  t1_field_ ## _field =               \
            Z1_FIELD_FIXED_ARRAY( T1TYPE, _field, _count, _max );

#define Z1_NEW_NUM_TABLE2( _name, _field, _max )         \
          static                                         \
          const Z1_Field_Rec  t1_field_ ## _field =      \
            Z1_FIELD_NUM_ARRAY2( T1TYPE, _field, _max );

#define Z1_NEW_FIXED_TABLE2( _name, _field, _max )         \
          static                                           \
          const Z1_Field_Rec  t1_field_ ## _field =        \
            Z1_FIELD_FIXED_ARRAY2( T1TYPE, _field, _max );


#define Z1_FONTINFO_STRING( n, f )          Z1_NEW_STRING( n, f )
#define Z1_FONTINFO_NUM( n, f )             Z1_NEW_NUM( n, f )
#define Z1_FONTINFO_BOOL( n, f )            Z1_NEW_BOOL( n, f )
#define Z1_PRIVATE_NUM( n, f )              Z1_NEW_NUM( n, f )
#define Z1_PRIVATE_FIXED( n, f )            Z1_NEW_FIXED( n, f )
#define Z1_PRIVATE_NUM_TABLE( n, f, m, c )  Z1_NEW_NUM_TABLE( n, f, m, c )
#define Z1_PRIVATE_NUM_TABLE2( n, f, m )    Z1_NEW_NUM_TABLE2( n, f, m )
#define Z1_TOPDICT_NUM( n, f )              Z1_NEW_NUM( n, f )
#define Z1_TOPDICT_NUM_FIXED2( n, f, m )    Z1_NEW_FIXED_TABLE2( n, f, m )


  /* including this file defines all field variables */
#ifdef FT_FLAT_COMPILE

#include "z1tokens.h"

#else

#include <freetype/src/type1z/z1tokens.h>

#endif


  /*************************************************************************/
  /*                                                                       */
  /* Second, define the keyword variables.  This is a set of Z1_KeyWord    */
  /* structures used to model the way each keyword is `loaded'.            */
  /*                                                                       */
  /*************************************************************************/

  typedef void  (*Z1_Parse_Func)( T1_Face     face,
                                  Z1_Loader*  loader );


  typedef enum  Z1_KeyWord_Type_
  {
    t1_keyword_callback = 0,
    t1_keyword_field,
    t1_keyword_field_table

  } Z1_KeyWord_Type;


  typedef enum  Z1_KeyWord_Location_
  {
    t1_keyword_type1 = 0,
    t1_keyword_font_info,
    t1_keyword_private

  } Z1_KeyWord_Location;


  typedef struct  Z1_KeyWord_
  {
    const char*          name;
    Z1_KeyWord_Type      type;
    Z1_KeyWord_Location  location;
    Z1_Parse_Func        parsing;
    const Z1_Field_Rec*  field;

  } Z1_KeyWord;


#define Z1_KEYWORD_CALLBACK( name, callback )                      \
        {                                                          \
          name, t1_keyword_callback, t1_keyword_type1, callback, 0 \
        }

#define Z1_KEYWORD_TYPE1( name, f )                                    \
        {                                                              \
          name, t1_keyword_field, t1_keyword_type1, 0, &t1_field_ ## f \
        }

#define Z1_KEYWORD_FONTINFO( name, f )                                     \
        {                                                                  \
          name, t1_keyword_field, t1_keyword_font_info, 0, &t1_field_ ## f \
        }

#define Z1_KEYWORD_PRIVATE( name, f )                                    \
        {                                                                \
          name, t1_keyword_field, t1_keyword_private, 0, &t1_field_ ## f \
        }

#define Z1_KEYWORD_FONTINFO_TABLE( name, f )                     \
        {                                                        \
          name, t1_keyword_field_table, t1_keyword_font_info, 0, \
          &t1_field_ ## f                                        \
        }

#define Z1_KEYWORD_PRIVATE_TABLE( name, f )                    \
        {                                                      \
          name, t1_keyword_field_table, t1_keyword_private, 0, \
          &t1_field_ ## f                                      \
        }


#undef  Z1_FONTINFO_STRING
#undef  Z1_FONTINFO_NUM
#undef  Z1_FONTINFO_BOOL
#undef  Z1_PRIVATE_NUM
#undef  Z1_PRIVATE_FIXED
#undef  Z1_PRIVATE_NUM_TABLE
#undef  Z1_PRIVATE_NUM_TABLE2
#undef  Z1_TOPDICT_NUM
#undef  Z1_TOPDICT_NUM_FIXED2

#define Z1_FONTINFO_STRING( n, f )          Z1_KEYWORD_FONTINFO( n, f ),
#define Z1_FONTINFO_NUM( n, f )             Z1_KEYWORD_FONTINFO( n, f ),
#define Z1_FONTINFO_BOOL( n, f )            Z1_KEYWORD_FONTINFO( n, f ),
#define Z1_PRIVATE_NUM( n, f )              Z1_KEYWORD_PRIVATE( n, f ),
#define Z1_PRIVATE_FIXED( n, f )            Z1_KEYWORD_PRIVATE( n, f ),
#define Z1_PRIVATE_NUM_TABLE( n, f, m, c )  Z1_KEYWORD_PRIVATE_TABLE( n, f ),
#define Z1_PRIVATE_NUM_TABLE2( n, f, m )    Z1_KEYWORD_PRIVATE_TABLE( n, f ),
#define Z1_TOPDICT_NUM( n, f )              Z1_KEYWORD_TYPE1( n, f ),
#define Z1_TOPDICT_NUM_FIXED2( n, f, m )    Z1_KEYWORD_TYPE1( n, f ),


  static
  FT_Error  t1_load_keyword( T1_Face      face,
                             Z1_Loader*   loader,
                             Z1_KeyWord*  keyword )
  {
    FT_Error   error;
    void*      dummy_object;
    void**     objects;
    FT_UInt    max_objects;
    T1_Blend*  blend = face->blend;


    /* if the keyword has a dedicated callback, call it */
    if ( keyword->type == t1_keyword_callback )
    {
      keyword->parsing( face, loader );
      error = loader->parser.error;
      goto Exit;
    }

    /* now, the keyword is either a simple field, or a table of fields; */
    /* we are now going to take care of it                              */
    switch ( keyword->location )
    {
    case t1_keyword_font_info:
      dummy_object = &face->type1.font_info;
      objects      = &dummy_object;
      max_objects  = 0;

      if ( blend )
      {
        objects     = (void**)blend->font_infos;
        max_objects = blend->num_designs;
      }
      break;

    case t1_keyword_private:
      dummy_object = &face->type1.private_dict;
      objects      = &dummy_object;
      max_objects  = 0;

      if ( blend )
      {
        objects     = (void**)blend->privates;
        max_objects = blend->num_designs;
      }
      break;

    default:
      dummy_object = &face->type1;
      objects      = &dummy_object;
      max_objects  = 0;
    }

    if ( keyword->type == t1_keyword_field_table )
      error = Z1_Load_Field_Table( &loader->parser, keyword->field,
                                   objects, max_objects, 0 );
    else
      error = Z1_Load_Field( &loader->parser, keyword->field,
                             objects, max_objects, 0 );

  Exit:
    return error;
  }


  static
  int  is_space( char  c )
  {
    return ( c == ' ' || c == '\t' || c == '\r' || c == '\n' );
  }


  static
  int  is_alpha( char c )
  {
    return ( isalnum( c ) ||
             ( c == '.' ) ||
             ( c == '_' ) );
  }


  static
  void  skip_whitespace( Z1_Parser*  parser )
  {
    FT_Byte*  cur = parser->cursor;


    while ( cur < parser->limit && is_space( *cur ) )
      cur++;

    parser->cursor = cur;
  }


  static
  void skip_blackspace( Z1_Parser*  parser )
  {
    FT_Byte*  cur = parser->cursor;

    while ( cur < parser->limit && !is_space( *cur ) )
      cur++;

    parser->cursor = cur;
  }


  static
  int  read_binary_data( Z1_Parser*  parser,
                         FT_Int*     size,
                         FT_Byte**   base )
  {
    FT_Byte*  cur;
    FT_Byte*  limit = parser->limit;


    /* the binary data has the following format */
    /*                                          */
    /* `size' [white*] RD white ....... ND      */
    /*                                          */

    skip_whitespace( parser );
    cur = parser->cursor;

    if ( cur < limit && (FT_Byte)( *cur - '0' ) < 10 )
    {
      *size = Z1_ToInt( parser );

      skip_whitespace( parser );
      skip_blackspace( parser );  /* `RD' or `-|' or something else */

      /* there is only one whitespace char after the */
      /* `RD' or `-|' token                          */
      *base = parser->cursor + 1;

      parser->cursor += *size+1;
      return 1;
    }

    FT_ERROR(( "read_binary_data: invalid size field\n" ));
    parser->error = T1_Err_Invalid_File_Format;
    return 0;
  }


  /* we will now define the routines used to handle */
  /* the `/Encoding', `/Subrs', and `/CharStrings'  */
  /* dictionaries                                   */

  static
  void  parse_font_name( T1_Face     face,
                         Z1_Loader*  loader )
  {
    Z1_Parser*  parser = &loader->parser;
    FT_Error    error;
    FT_Memory   memory = parser->memory;
    FT_Int      len;
    FT_Byte*    cur;
    FT_Byte*    cur2;
    FT_Byte*    limit;


    skip_whitespace( parser );

    cur   = parser->cursor;
    limit = parser->limit;

    if ( cur >= limit - 1 || *cur != '/' )
      return;

    cur++;
    cur2 = cur;
    while ( cur2 < limit && is_alpha( *cur2 ) )
      cur2++;

    len = cur2 - cur;
    if ( len > 0 )
    {
      if ( ALLOC( face->type1.font_name, len + 1 ) )
      {
        parser->error = error;
        return;
      }

      MEM_Copy( face->type1.font_name, cur, len );
      face->type1.font_name[len] = '\0';
    }
    parser->cursor = cur2;
  }


  static
  void  parse_font_bbox( T1_Face     face,
                         Z1_Loader*  loader )
  {
    Z1_Parser*  parser = &loader->parser;
    FT_Short    temp[4];
    FT_BBox*    bbox   = &face->type1.font_bbox;


    (void)Z1_ToCoordArray( parser, 4, temp );
    bbox->xMin = temp[0];
    bbox->yMin = temp[1];
    bbox->xMax = temp[2];
    bbox->yMax = temp[3];
  }


  static
  void  parse_font_matrix( T1_Face     face,
                           Z1_Loader*  loader )
  {
    Z1_Parser*  parser = &loader->parser;
    FT_Matrix*  matrix = &face->type1.font_matrix;
    FT_Fixed    temp[4];


    (void)Z1_ToFixedArray( parser, 4, temp, 3 );
    matrix->xx = temp[0];
    matrix->yx = temp[1];
    matrix->xy = temp[2];
    matrix->yy = temp[3];
  }


  static
  void  parse_encoding( T1_Face     face,
                        Z1_Loader*  loader )
  {
    Z1_Parser*  parser = &loader->parser;
    FT_Byte*    cur   = parser->cursor;
    FT_Byte*    limit = parser->limit;


    /* skip whitespace */
    while ( is_space( *cur ) )
    {
      cur++;
      if ( cur >= limit )
      {
        FT_ERROR(( "parse_encoding: out of bounds!\n" ));
        parser->error = T1_Err_Invalid_File_Format;
        return;
      }
    }

    /* if we have a number, then the encoding is an array, */
    /* and we must load it now                             */
    if ( (FT_Byte)( *cur - '0' ) < 10 )
    {
      T1_Encoding*  encode     = &face->type1.encoding;
      FT_Int        count, n;
      Z1_Table*     char_table = &loader->encoding_table;
      FT_Memory     memory     = parser->memory;
      FT_Error      error;


      /* read the number of entries in the encoding, should be 256 */
      count = Z1_ToInt( parser );
      if ( parser->error )
        return;

      /* we use a Z1_Table to store our charnames */
      encode->num_chars = count;
      if ( ALLOC_ARRAY( encode->char_index, count, FT_Short   )       ||
           ALLOC_ARRAY( encode->char_name,  count, FT_String* )       ||
           ( error = Z1_New_Table( char_table, count, memory ) ) != 0 )
      {
        parser->error = error;
        return;
      }

      /* Now, we will need to read a record of the form         */
      /* ... charcode /charname ... for each entry in our table */
      /*                                                        */
      /* We simply look for a number followed by an immediate   */
      /* name.  Note that this ignores correctly the sequence   */
      /* that is often seen in type1 fonts:                     */
      /*                                                        */
      /*   0 1 255 { 1 index exch /.notdef put } for dup        */
      /*                                                        */
      /* used to clean the encoding array before anything else. */
      /*                                                        */
      /* We stop when we encounter a `def'.                     */

      cur   = parser->cursor;
      limit = parser->limit;
      n     = 0;

      for ( ; cur < limit; )
      {
        FT_Byte  c;


        c = *cur;

        /* we stop when we encounter a `def' */
        if ( c == 'd' && cur + 3 < limit )
        {
          if ( cur[1] == 'e' &&
               cur[2] == 'f' &&
               is_space(cur[-1]) &&
               is_space(cur[3]) )
          {
            FT_TRACE6(( "encoding end\n" ));
            break;
          }
        }

        /* otherwise, we must find a number before anything else */
        if ( (FT_Byte)( c - '0' ) < 10 )
        {
          FT_Int  charcode;


          parser->cursor = cur;
          charcode = Z1_ToInt( parser );
          cur = parser->cursor;

          /* skip whitespace */
          while ( cur < limit && is_space( *cur ) )
            cur++;

          if ( cur < limit && *cur == '/' )
          {
            /* bingo, we have an immediate name -- it must be a */
            /* character name                                   */
            FT_Byte*  cur2 = cur + 1;
            FT_Int    len;


            while ( cur2 < limit && is_alpha( *cur2 ) )
              cur2++;

            len = cur2 - cur - 1;

            parser->error = Z1_Add_Table( char_table, charcode,
                                          cur + 1, len + 1 );
            char_table->elements[charcode][len] = '\0';
            if ( parser->error )
              return;

            cur = cur2;
          }
        }
        else
          cur++;
      }

      face->type1.encoding_type = t1_encoding_array;
      parser->cursor            = cur;
    }
    /* Otherwise, we should have either `StandardEncoding' or */
    /* `ExpertEncoding'                                       */
    else
    {
      if ( cur + 17 < limit &&
           strncmp( (const char*)cur, "StandardEncoding", 16 ) == 0 )
        face->type1.encoding_type = t1_encoding_standard;

      else if ( cur + 15 < limit &&
                strncmp( (const char*)cur, "ExpertEncoding", 14 ) == 0 )
        face->type1.encoding_type = t1_encoding_expert;

      else
      {
        FT_ERROR(( "parse_encoding: invalid token!\n" ));
        parser->error = T1_Err_Invalid_File_Format;
      }
    }
  }


  static
  void  parse_subrs( T1_Face     face,
                     Z1_Loader*  loader )
  {
    Z1_Parser*  parser = &loader->parser;
    Z1_Table*   table  = &loader->subrs;
    FT_Memory   memory = parser->memory;
    FT_Error    error;
    FT_Int      n;


    loader->num_subrs = Z1_ToInt( parser );
    if ( parser->error )
      return;

    /* position the parser right before the `dup' of the first subr */
    skip_whitespace( parser );
    skip_blackspace( parser );      /* `array' */
    skip_whitespace( parser );

    /* initialize subrs array */
    error = Z1_New_Table( table, loader->num_subrs, memory );
    if ( error )
      goto Fail;

    /* the format is simple:                                 */
    /*                                                       */
    /*   `index' + binary data                               */
    /*                                                       */

    for ( n = 0; n < loader->num_subrs; n++ )
    {
      FT_Int    index, size;
      FT_Byte*  base;


      /* If the next token isn't `dup', we are also done.  This */
      /* happens when there are `holes' in the Subrs array.     */
      if ( strncmp( (char*)parser->cursor, "dup", 3 ) != 0 )
        break;

      index = Z1_ToInt( parser );
      
      if ( !read_binary_data( parser, &size, &base ) )
        return;

      /* The binary string is followed by one token, e.g. `NP' */
      /* (bound to `noaccess put') or by two separate tokens:  */
      /* `noaccess' & `put'.  We position the parser right     */
      /* before the next `dup', if any.                        */
      skip_whitespace( parser );
      skip_blackspace( parser );    /* `NP' or `I' or `noaccess' */
      skip_whitespace( parser );

      if ( strncmp( (char*)parser->cursor, "put", 3 ) == 0 )
      {
        skip_blackspace( parser );  /* skip `put' */
        skip_whitespace( parser );
      }

      /* some fonts use a value of -1 for lenIV to indicate that */
      /* the charstrings are unencoded                           */
      /*                                                         */
      /* thanks to Tom Kacvinsky for pointing this out           */
      /*                                                         */
      if ( face->type1.private_dict.lenIV >= 0 )
      {
        Z1_Decrypt( base, size, 4330 );
        size -= face->type1.private_dict.lenIV;
        base += face->type1.private_dict.lenIV;
      }

      error = Z1_Add_Table( table, index, base, size );
      if ( error )
        goto Fail;
    }
    return;

  Fail:
    parser->error = error;
  }


  static
  void  parse_charstrings( T1_Face     face,
                           Z1_Loader*  loader )
  {
    Z1_Parser*  parser     = &loader->parser;
    Z1_Table*   code_table = &loader->charstrings;
    Z1_Table*   name_table = &loader->glyph_names;
    FT_Memory   memory     = parser->memory;
    FT_Error    error;

    FT_Byte*    cur;
    FT_Byte*    limit = parser->limit;
    FT_Int      n;


    loader->num_glyphs = Z1_ToInt( parser );
    if ( parser->error )
      return;

    /* initialize tables */
    error = Z1_New_Table( code_table, loader->num_glyphs, memory ) ||
            Z1_New_Table( name_table, loader->num_glyphs, memory );
    if ( error )
      goto Fail;

    n = 0;
    for (;;)
    {
      FT_Int    size;
      FT_Byte*  base;


      /* the format is simple:                    */
      /*   `/glyphname' + binary data             */
      /*                                          */
      /* note that we stop when we find a `def'   */
      /*                                          */
      skip_whitespace( parser );

      cur = parser->cursor;
      if ( cur >= limit )
        break;

      /* we stop when we find a `def' or `end' keyword */
      if ( *cur   == 'd'   &&
           cur + 3 < limit &&
           cur[1] == 'e'   &&
           cur[2] == 'f'   )
        break;

      if ( *cur   == 'e'   &&
           cur + 3 < limit &&
           cur[1] == 'n'   &&
           cur[2] == 'd'   )
        break;

      if ( *cur != '/' )
        skip_blackspace( parser );
      else
      {
        FT_Byte*  cur2 = cur + 1;
        FT_Int    len;


        while ( cur2 < limit && is_alpha( *cur2 ) )
          cur2++;
        len = cur2 - cur - 1;

        error = Z1_Add_Table( name_table, n, cur + 1, len + 1 );
        if ( error )
          goto Fail;

        /* add a trailing zero to the name table */
        name_table->elements[n][len] = '\0';

        parser->cursor = cur2;
        if ( !read_binary_data( parser, &size, &base ) )
          return;

        if ( face->type1.private_dict.lenIV >= 0 )
        {
          Z1_Decrypt( base, size, 4330 );
          size -= face->type1.private_dict.lenIV;
          base += face->type1.private_dict.lenIV;
        }

        error = Z1_Add_Table( code_table, n, base, size );
        if ( error )
          goto Fail;

        n++;
        if ( n >= loader->num_glyphs )
          break;
      }
    }
    loader->num_glyphs = n;
    return;

  Fail:
    parser->error = error;
  }


  static
  const Z1_KeyWord  t1_keywords[] =
  {

#ifdef FT_FLAT_COMPILE

#include "z1tokens.h"

#else

#include <freetype/src/type1z/z1tokens.h>

#endif

    /* now add the special functions... */
    Z1_KEYWORD_CALLBACK( "FontName", parse_font_name ),
    Z1_KEYWORD_CALLBACK( "FontBBox", parse_font_bbox ),
    Z1_KEYWORD_CALLBACK( "FontMatrix", parse_font_matrix ),
    Z1_KEYWORD_CALLBACK( "Encoding", parse_encoding ),
    Z1_KEYWORD_CALLBACK( "Subrs", parse_subrs ),
    Z1_KEYWORD_CALLBACK( "CharStrings", parse_charstrings ),

#ifndef Z1_CONFIG_OPTION_NO_MM_SUPPORT
    Z1_KEYWORD_CALLBACK( "BlendDesignPositions", parse_blend_design_positions ),
    Z1_KEYWORD_CALLBACK( "BlendDesignMap", parse_blend_design_map ),
    Z1_KEYWORD_CALLBACK( "BlendAxisTypes", parse_blend_axis_types ),
    Z1_KEYWORD_CALLBACK( "WeightVector", parse_weight_vector ),
    Z1_KEYWORD_CALLBACK( "shareddict", parse_shared_dict ),
#endif

    Z1_KEYWORD_CALLBACK( 0, 0 )
  };


  static
  FT_Error  parse_dict( T1_Face     face,
                        Z1_Loader*  loader,
                        FT_Byte*    base,
                        FT_Long     size )
  {
    Z1_Parser*  parser = &loader->parser;


    parser->cursor = base;
    parser->limit  = base + size;
    parser->error  = 0;

    {
      FT_Byte*  cur   = base;
      FT_Byte*  limit = cur + size;


      for ( ; cur < limit; cur++ )
      {
        /* look for `FontDirectory', which causes problems on some fonts */
        if ( *cur == 'F' && cur + 25 < limit                 &&
             strncmp( (char*)cur, "FontDirectory", 13 ) == 0 )
        {
          FT_Byte*  cur2;


          /* skip the `FontDirectory' keyword */
          cur += 13;
          cur2 = cur;

          /* lookup the `known' keyword */
          while ( cur < limit && *cur != 'k'        &&
                  strncmp( (char*)cur, "known", 5 ) )
            cur++;

          if ( cur < limit )
          {
            Z1_Token_Rec  token;


            /* skip the `known' keyword and the token following it */
            cur += 5;
            loader->parser.cursor = cur;
            Z1_ToToken( &loader->parser, &token );

            /* if the last token was an array, skip it! */
            if ( token.type == t1_token_array )
              cur2 = parser->cursor;
          }
          cur = cur2;
        }
        /* look for immediates */
        else if ( *cur == '/' && cur + 2 < limit )
        {
          FT_Byte*  cur2;
          FT_Int    len;


          cur++;
          cur2 = cur;
          while ( cur2 < limit && is_alpha( *cur2 ) )
            cur2++;

          len  = cur2 - cur;
          if ( len > 0 && len < 22 )
          {
            if ( !loader->fontdata )
            {
              if ( strncmp( (char*)cur, "FontInfo", 8 ) == 0 )
                loader->fontdata = 1;
            }
            else
            {
              /* now, compare the immediate name to the keyword table */
              Z1_KeyWord*  keyword = (Z1_KeyWord*)t1_keywords;


              for (;;)
              {
                FT_Byte*  name;


                name = (FT_Byte*)keyword->name;
                if ( !name )
                  break;

                if ( cur[0] == name[0]                          &&
                     len == (FT_Int)strlen( (const char*)name ) )
                {
                  FT_Int  n;


                  for ( n = 1; n < len; n++ )
                    if ( cur[n] != name[n] )
                      break;

                  if ( n >= len )
                  {
                    /* we found it -- run the parsing callback! */
                    parser->cursor = cur2;
                    skip_whitespace( parser );
                    parser->error = t1_load_keyword( face, loader, keyword );
                    if ( parser->error )
                      return parser->error;

                    cur = parser->cursor;
                    break;
                  }
                }
                keyword++;
              }
            }
          }
        }
      }
    }
    return parser->error;
  }


  static
  void  t1_init_loader( Z1_Loader*  loader,
                        T1_Face     face )
  {
    FT_UNUSED( face );

    MEM_Set( loader, 0, sizeof ( *loader ) );
    loader->num_glyphs = 0;
    loader->num_chars  = 0;

    /* initialize the tables -- simply set their `init' field to 0 */
    loader->encoding_table.init = 0;
    loader->charstrings.init    = 0;
    loader->glyph_names.init    = 0;
    loader->subrs.init          = 0;
    loader->fontdata            = 0;
  }


  static
  void  t1_done_loader( Z1_Loader*  loader )
  {
    Z1_Parser*  parser = &loader->parser;


    /* finalize tables */
    Z1_Release_Table( &loader->encoding_table );
    Z1_Release_Table( &loader->charstrings );
    Z1_Release_Table( &loader->glyph_names );
    Z1_Release_Table( &loader->subrs );

    /* finalize parser */
    Z1_Done_Parser( parser );
  }


  LOCAL_FUNC
  FT_Error  Z1_Open_Face( T1_Face  face )
  {
    Z1_Loader   loader;
    Z1_Parser*  parser;
    T1_Font*    type1 = &face->type1;
    FT_Error    error;


    t1_init_loader( &loader, face );

    /* default lenIV */
    type1->private_dict.lenIV = 4;

    parser = &loader.parser;
    error = Z1_New_Parser( parser, face->root.stream, face->root.memory );
    if ( error )
      goto Exit;

    error = parse_dict( face, &loader, parser->base_dict, parser->base_len );
    if ( error )
      goto Exit;

    error = Z1_Get_Private_Dict( parser );
    if ( error )
      goto Exit;

    error = parse_dict( face, &loader, parser->private_dict,
                        parser->private_len );
    if ( error )
      goto Exit;

    /* now, propagate the subrs, charstrings, and glyphnames tables */
    /* to the Type1 data                                            */
    type1->num_glyphs = loader.num_glyphs;

    if ( !loader.subrs.init )
    {
      FT_ERROR(( "Z1_Open_Face: no subrs array in face!\n" ));
      error = T1_Err_Invalid_File_Format;
    }

    if ( !loader.charstrings.init )
    {
      FT_ERROR(( "Z1_Open_Face: no charstrings array in face!\n" ));
      error = T1_Err_Invalid_File_Format;
    }

    loader.subrs.init  = 0;
    type1->num_subrs   = loader.num_subrs;
    type1->subrs_block = loader.subrs.block;
    type1->subrs       = loader.subrs.elements;
    type1->subrs_len   = loader.subrs.lengths;

    loader.charstrings.init  = 0;
    type1->charstrings_block = loader.charstrings.block;
    type1->charstrings       = loader.charstrings.elements;
    type1->charstrings_len   = loader.charstrings.lengths;

    /* we copy the glyph names `block' and `elements' fields; */
    /* the `lengths' field must be released later             */
    type1->glyph_names_block    = loader.glyph_names.block;
    type1->glyph_names          = (FT_String**)loader.glyph_names.elements;
    loader.glyph_names.block    = 0;
    loader.glyph_names.elements = 0;

    /* we must now build type1.encoding when we have a custom */
    /* array..                                                */
    if ( type1->encoding_type == t1_encoding_array )
    {
      FT_Int    charcode, index, min_char, max_char;
      FT_Byte*  char_name;
      FT_Byte*  glyph_name;


      /* OK, we do the following: for each element in the encoding  */
      /* table, look up the index of the glyph having the same name */
      /* the index is then stored in type1.encoding.char_index, and */
      /* a the name to type1.encoding.char_name                     */

      min_char = +32000;
      max_char = -32000;

      charcode = 0;
      for ( ; charcode < loader.encoding_table.num_elems; charcode++ )
      {
        type1->encoding.char_index[charcode] = 0;
        type1->encoding.char_name [charcode] = ".notdef";

        char_name = loader.encoding_table.elements[charcode];
        if ( char_name )
          for ( index = 0; index < type1->num_glyphs; index++ )
          {
            glyph_name = (FT_Byte*)type1->glyph_names[index];
            if ( strcmp( (const char*)char_name,
                         (const char*)glyph_name ) == 0 )
            {
              type1->encoding.char_index[charcode] = index;
              type1->encoding.char_name [charcode] = (char*)glyph_name;

              if (charcode < min_char) min_char = charcode;
              if (charcode > max_char) max_char = charcode;
              break;
            }
          }
      }
      type1->encoding.code_first = min_char;
      type1->encoding.code_last  = max_char;
      type1->encoding.num_chars  = loader.num_chars;
   }

  Exit:
    t1_done_loader( &loader );
    return error;
  }


/* END */
