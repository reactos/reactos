/***************************************************************************/
/*                                                                         */
/*  t1load.c                                                               */
/*                                                                         */
/*    Type 1 font loader (body).                                           */
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


#include <freetype/config/ftconfig.h>
#include <freetype/internal/ftdebug.h>
#include <freetype/internal/t1types.h>


#ifdef FT_FLAT_COMPILE

#include "t1tokens.h"
#include "t1parse.h"

#else

#include <type1/t1tokens.h>
#include <type1/t1parse.h>

#endif


#include <stdio.h>

#include <string.h>     /* for strncpy(), strncmp(), strlen() */


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t1load


  typedef FT_Error  (*T1_Parse_Func)( T1_Parser*  parser );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Init_T1_Parser                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a given parser object to build a given T1_Face.        */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    parser    :: A handle to the newly built parser object.            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face      :: A handle to the target Type 1 face object.            */
  /*                                                                       */
  /*    tokenizer :: A handle to the target Type 1 token manager.          */
  /*                                                                       */
  LOCAL_FUNC
  void  Init_T1_Parser( T1_Parser*    parser,
                        T1_Face       face,
                        T1_Tokenizer  tokenizer )
  {
    parser->error     = 0;
    parser->face      = face;
    parser->tokenizer = tokenizer;
    parser->top       = parser->stack;
    parser->limit     = parser->stack + T1_MAX_STACK_DEPTH;

    parser->state_index    = 0;
    parser->state_stack[0] = dict_none;

    parser->encoding_type    = t1_encoding_none;
    parser->encoding_names   = 0;
    parser->encoding_offsets = 0;
    parser->encoding_lengths = 0;

    parser->dump_tokens            = 0;
    face->type1.private_dict.lenIV = 4;  /* XXX : is it sure? */
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Next_T1_Token                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Grabs the next significant token from a parser's input stream.     */
  /*    This function ignores a number of tokens, and translates           */
  /*    alternate forms into their common ones.                            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    parser :: A handle to the source parser.                           */
  /*                                                                       */
  /* <Output>                                                              */
  /*    token  :: The extracted token descriptor.                          */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeTyoe error code.  0 means success.                             */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  Next_T1_Token( T1_Parser*  parser,
                           T1_Token*   token )
  {
    FT_Error      error;
    T1_Tokenizer  tokzer = parser->tokenizer;


  L1:
    error = Read_Token( tokzer );
    if ( error )
      return error;

    /* we now must ignore a number of tokens like `dup', `executeonly', */
    /* `readonly', etc.                                                 */
    *token = tokzer->token;
    if ( token->kind == tok_keyword )
      switch( token->kind2 )
      {
      case key_dup:
      case key_execonly:
      case key_readonly:
      case key_noaccess:
      case key_userdict:
        /* do nothing - loop */
        goto L1;

        /* we also translate some other keywords from their alternative */
        /* to their `normal' form                                       */

      case key_NP_alternate:
        token->kind2 = key_NP;
        break;

      case key_RD_alternate:
        token->kind2 = key_RD;
        break;

      case key_ND_alternate:
        token->kind2 = key_ND;
        break;

      default:
        ;
      }

#if defined( FT_DEBUG_LEVEL_ERROR ) || defined( FT_DEBUG_LEVEL_TRACE )

    /* Dump the token when requested.  This feature is only available */
    /* in the `error' and `trace' debug levels.                       */
    if ( parser->dump_tokens )
    {
      FT_String  temp_string[128];
      FT_Int     len;


      len = token->len;
      if ( len > 127 )
        len = 127;
      strncpy( temp_string,
               (FT_String*)tokzer->base + token->start,
               len );
      temp_string[len] = '\0';
      FT_ERROR(( "%s\n", temp_string ));
    }

#endif /* FT_DEBUG_LEVEL_ERROR or FT_DEBUG_LEVEL_TRACE */

    return T1_Err_Ok;
  }


  static
  FT_Error  Expect_Keyword( T1_Parser*    parser,
                            T1_TokenType  keyword )
  {
    T1_Token  token;
    FT_Error  error;


    error = Next_T1_Token( parser, &token );
    if ( error )
      goto Exit;

    if ( token.kind  != tok_keyword ||
         token.kind2 != keyword     )
    {
      error = T1_Err_Syntax_Error;
      FT_ERROR(( "Expect_Keyword: keyword `%s' expected.\n",
                 t1_keywords[keyword - key_first_] ));
    }

  Exit:
    return error;
  }


  static
  FT_Error  Expect_Keyword2( T1_Parser*    parser,
                             T1_TokenType  keyword1,
                             T1_TokenType  keyword2 )
  {
    T1_Token  token;
    FT_Error  error;


    error = Next_T1_Token( parser, &token );
    if ( error )
      goto Exit;

    if ( token.kind  != tok_keyword  ||
         ( token.kind2 != keyword1 &&
           token.kind2 != keyword2 ) )
    {
      error = T1_Err_Syntax_Error;
      FT_ERROR(( "Expect_Keyword2: keyword `%s' or `%s' expected.\n",
                 t1_keywords[keyword1 - key_first_],
                 t1_keywords[keyword2 - key_first_] ));
    }

  Exit:
    return error;
  }


  static
  void  Parse_Encoding( T1_Parser*  parser )
  {
    T1_Token*     token  = parser->top+1;
    FT_Memory     memory = parser->face->root.memory;
    T1_Encoding*  encode = &parser->face->type1.encoding;
    FT_Error      error  = 0;


    if ( token->kind  == tok_keyword              &&
         ( token->kind2 == key_StandardEncoding ||
           token->kind2 == key_ExpertEncoding   ) )
    {
      encode->num_chars  = 256;
      encode->code_first = 32;
      encode->code_last  = 255;

      if ( ALLOC_ARRAY( encode->char_index, 256, FT_Short ) )
        goto Exit;

      encode->char_name = 0;  /* no need to store glyph names */

      /* Now copy the encoding */
      switch ( token->kind2 )
      {
      case key_ExpertEncoding:
        parser->encoding_type = t1_encoding_expert;
        break;

      default:
        parser->encoding_type = t1_encoding_standard;
        break;
      }
    }
    else
    {
      FT_ERROR(( "Parse_Encoding: invalid encoding type\n" ));
      error = T1_Err_Syntax_Error;
    }

  Exit:
    parser->error = error;
  }


  /*************************************************************************/
  /*                                                                       */
  /*                                                                       */
  /*           IMPLEMENTATION OF THE `DEF' KEYWORD DEPENDING ON            */
  /*                        CURRENT DICTIONARY STATE                       */
  /*                                                                       */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Do_Def_Font                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function performs a `def' if in the Font dictionary.  Its     */
  /*    purpose is to build the T1_Face attributes directly from the       */
  /*    stream.                                                            */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    parser :: A handle to the current parser.                          */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static
  FT_Error  Do_Def_Font( T1_Parser*  parser )
  {
    T1_Token*  top   = parser->top;
    T1_Face    face  = parser->face;
    T1_Font*   type1 = &face->type1;


    switch ( top[0].kind2 )
    {
    case imm_FontName:
      /* in some cases, the /FontName is an immediate like */
      /* /TimesNewRoman.  In this case, we simply copy the */
      /* token string (without the /).                     */
      if ( top[1].kind == tok_immediate )
      {
        FT_Memory  memory = parser->tokenizer->memory;
        FT_Error   error;
        FT_Int     len = top[1].len;


        if ( ALLOC( type1->font_name, len + 1 ) )
        {
          parser->error = error;
          return error;
        }

        MEM_Copy( type1->font_name,
                  parser->tokenizer->base + top[1].start,
                  len );
        type1->font_name[len] = '\0';
      }
      else
        type1->font_name = CopyString( parser );
      break;

    case imm_Encoding:
      Parse_Encoding( parser );
      break;

    case imm_PaintType:
      type1->paint_type = (FT_Byte)CopyInteger( parser );
      break;

    case imm_FontType:
      type1->font_type = (FT_Byte)CopyInteger( parser );
      break;

    case imm_FontMatrix:
      CopyMatrix( parser, &type1->font_matrix );
      break;

    case imm_FontBBox:
      CopyBBox( parser, &type1->font_bbox );
      break;

    case imm_UniqueID:
      type1->private_dict.unique_id = CopyInteger( parser );
      break;

    case imm_StrokeWidth:
      type1->stroke_width = CopyInteger( parser );
      break;

    case imm_FontID:
      type1->font_id = CopyInteger( parser );
      break;

    default:
      /* ignore all other things */
      parser->error = T1_Err_Ok;
    }

    return parser->error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Do_Def_FontInfo                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function performs a `def' if in the FontInfo dictionary. Its  */
  /*    purpose is to build the T1_FontInfo structure directly from the    */
  /*    stream.                                                            */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    parser :: A handle to the current parser.                          */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeTyoe error code.  0 means success.                             */
  /*                                                                       */
  static
  FT_Error  Do_Def_FontInfo( T1_Parser*  parser )
  {
    T1_Token*     top  = parser->top;
    T1_FontInfo*  info = &parser->face->type1.font_info;


    switch ( top[0].kind2 )
    {
    case imm_version:
      info->version = CopyString( parser );
      break;

    case imm_Notice:
      info->notice = CopyString( parser );
      break;

    case imm_FullName:
      info->full_name = CopyString( parser );
      break;

    case imm_FamilyName:
      info->family_name = CopyString( parser );
      break;

    case imm_Weight:
      info->weight = CopyString( parser );
      break;

    case imm_ItalicAngle:
      info->italic_angle = CopyInteger( parser );
      break;

    case imm_isFixedPitch:
      info->is_fixed_pitch = CopyBoolean( parser );
      break;

    case imm_UnderlinePosition:
      info->underline_position = (FT_Short)CopyInteger( parser );
      break;

    case imm_UnderlineThickness:
      info->underline_thickness = (FT_Short)CopyInteger( parser );
      break;

    default:
      /* ignore all other things */
      parser->error = T1_Err_Ok;
    }

    return parser->error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Do_Def_Private                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function performs a `def' if in the Private dictionary.  Its  */
  /*    purpose is to build the T1_Private structure directly from the     */
  /*    stream.                                                            */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    parser :: A handle to the current parser.                          */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeTyoe error code.  0 means success.                             */
  /*                                                                       */
  static
  FT_Error  Do_Def_Private( T1_Parser*  parser )
  {
    T1_Token*   top   = parser->top;
    T1_Private* priv  = &parser->face->type1.private_dict;


    switch ( top[0].kind2 )
    {
      /* Ignore the definitions of RD, NP, ND, and their alternate forms */
    case imm_RD:
    case imm_RD_alternate:
    case imm_ND:
    case imm_ND_alternate:
    case imm_NP:
    case imm_NP_alternate:
      parser->error = T1_Err_Ok;
      break;

    case imm_BlueValues:
      CopyArray( parser, &priv->num_blue_values,
                 priv->blue_values, 14 );
      break;

    case imm_OtherBlues:
      CopyArray( parser, &priv->num_other_blues,
                 priv->other_blues, 10 );
      break;

    case imm_FamilyBlues:
      CopyArray( parser, &priv->num_family_blues,
                 priv->family_blues, 14 );
      break;

    case imm_FamilyOtherBlues:
      CopyArray( parser, &priv->num_family_other_blues,
                 priv->family_other_blues, 10 );
      break;

    case imm_BlueScale:
      priv->blue_scale = CopyFloat( parser, 0x10000L );
      break;

    case imm_BlueShift:
      priv->blue_shift = CopyInteger( parser );
      break;

    case imm_BlueFuzz:
      priv->blue_fuzz = CopyInteger( parser );
      break;

    case imm_StdHW:
      CopyArray( parser, 0, (FT_Short*)&priv->standard_width, 1 );
      break;

    case imm_StdVW:
      CopyArray( parser, 0, (FT_Short*)&priv->standard_height, 1 );
      break;

    case imm_StemSnapH:
      CopyArray( parser, &priv->num_snap_widths,
                 priv->snap_widths, 12 );
      break;

    case imm_StemSnapV:
      CopyArray( parser, &priv->num_snap_heights,
                 priv->snap_heights, 12 );
      break;

    case imm_ForceBold:
      priv->force_bold = CopyBoolean( parser );
      break;

    case imm_LanguageGroup:
      priv->language_group = CopyInteger( parser );
      break;

    case imm_password:
      priv->password = CopyInteger( parser );
      break;

    case imm_UniqueID:
      priv->unique_id = CopyInteger( parser );
      break;

    case imm_lenIV:
      priv->lenIV = CopyInteger( parser );
      break;

    case imm_MinFeature:
      CopyArray( parser, 0, priv->min_feature, 2 );
      break;

    default:
      /* ignore all other things */
      parser->error = T1_Err_Ok;
    }

    return parser->error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Do_Def_Error                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function returns a simple syntax error when invoked.  It is   */
  /*    used for the `def' keyword if in the `encoding', `subrs',          */
  /*    `othersubrs', and `charstrings' dictionary states.                 */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    parser :: A handle to the current parser.                          */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static
  FT_Error  Do_Def_Error( T1_Parser*  parser )
  {
    FT_ERROR(( "Do_Def_Error:" ));
    FT_ERROR(( " `def' keyword encountered in bad dictionary/array\n" ));

    parser->error = T1_Err_Syntax_Error;

    return parser->error;
  }


  static
  FT_Error  Do_Def_Ignore( T1_Parser*  parser )
  {
    FT_UNUSED( parser );
    return T1_Err_Ok;
  }


  static
  T1_Parse_Func  def_funcs[dict_max] =
  {
    Do_Def_Error,
    Do_Def_Font,
    Do_Def_FontInfo,
    Do_Def_Ignore,
    Do_Def_Private,
    Do_Def_Ignore,
    Do_Def_Ignore,
    Do_Def_Ignore,
    Do_Def_Ignore,
    Do_Def_Ignore,
    Do_Def_Ignore,
  };


  /*************************************************************************/
  /*                                                                       */
  /*                                                                       */
  /*           IMPLEMENTATION OF THE `PUT' KEYWORD DEPENDING ON            */
  /*                        CURRENT DICTIONARY STATE                       */
  /*                                                                       */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Do_Put_Encoding                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function performs a `put' if in the Encoding array.  The      */
  /*    glyph name is copied into the T1 recorder, and the charcode and    */
  /*    glyph name pointer are written into the face object encoding.      */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    parser :: A handle to the current parser.                          */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static
  FT_Error  Do_Put_Encoding( T1_Parser*  parser )
  {
    FT_Error      error  = T1_Err_Ok;
    T1_Face       face   = parser->face;
    T1_Token*     top    = parser->top;
    T1_Encoding*  encode = &face->type1.encoding;
    FT_Int        index;


    /* record and check the character code */
    if ( top[0].kind != tok_number )
    {
      FT_TRACE4(( "Do_Put_Encoding: number expected\n" ));
      goto Syntax_Error;
    }
    index = (FT_Int)CopyInteger( parser );
    if ( parser->error )
      return parser->error;

    if ( index < 0 || index >= encode->num_chars )
    {
      FT_TRACE4(( "Do_Put_Encoding: invalid character code\n" ));
      goto Syntax_Error;
    }

    /* record the immediate name */
    if ( top[1].kind != tok_immediate )
    {
      FT_TRACE4(( "Do_Put_Encoding: immediate name expected\n" ));
      goto Syntax_Error;
    }

    /* if the glyph name is `.notdef', store a NULL char name; */
    /* otherwise, record the glyph name                        */
    if ( top[1].kind == imm_notdef )
    {
      parser->table.elements[index] = 0;
      parser->table.lengths [index] = 0;
    }
    else
    {
      FT_String  temp_name[128];
      T1_Token*  token = top + 1;
      FT_Int     len   = token->len - 1;


      /* copy immediate name */
      if ( len > 127 )
        len = 127;
      MEM_Copy( temp_name, parser->tokenizer->base + token->start + 1, len );
      temp_name[len] = '\0';

      error = T1_Add_Table( &parser->table, index,
                            (FT_Byte*)temp_name, len + 1 );

      /* adjust code_first and code_last */
      if ( index < encode->code_first ) encode->code_first = index;
      if ( index > encode->code_last  ) encode->code_last  = index;
    }
    return error;

  Syntax_Error:
    /* ignore the error, and simply clear the stack */
    FT_TRACE4(( "Do_Put_Encoding: invalid syntax encountered\n" ));
    parser->top = parser->stack;

    return T1_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /*                                                                       */
  /*           IMPLEMENTATION OF THE "RD" KEYWORD DEPENDING ON             */
  /*                        CURRENT DICTIONARY STATE                       */
  /*                                                                       */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Do_RD_Subrs                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function performs an `RD' if in the Subrs dictionary.  It     */
  /*    simply records the array of bytecodes/charstrings corresponding to */
  /*    the sub-routine.                                                   */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    parser :: A handle to the current parser.                          */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static
  FT_Error  Do_RD_Subrs( T1_Parser*  parser )
  {
    FT_Error      error  = T1_Err_Ok;
    T1_Face       face   = parser->face;
    T1_Token*     top    = parser->top;
    T1_Tokenizer  tokzer = parser->tokenizer;
    FT_Int        index, count;


    /* record and check the character code */
    if ( top[0].kind != tok_number ||
         top[1].kind != tok_number )
    {
      FT_ERROR(( "Do_RD_Subrs: number expected\n" ));
      goto Syntax_Error;
    }
    index = (FT_Int)CopyInteger( parser );
    error = parser->error;
    if ( error )
      goto Exit;

    count = (FT_Int)CopyInteger( parser );
    error = parser->error;
    if ( error )
      goto Exit;

    if ( index < 0 || index >= face->type1.num_subrs )
    {
      FT_ERROR(( "Do_RD_Subrs: invalid character code\n" ));
      goto Syntax_Error;
    }

    /* decrypt charstring and skip it */
    {
      FT_Byte*  base = tokzer->base + tokzer->cursor;


      tokzer->cursor += count;

      /* some fonts use a value of -1 for lenIV to indicate that */
      /* the charstrings are unencoded.                          */
      /*                                                         */
      /* Thanks to Tom Kacvinsky for pointing this out.          */
      /*                                                         */
      if ( face->type1.private_dict.lenIV >= 0 )
      {
        t1_decrypt( base, count, 4330 );

        base  += face->type1.private_dict.lenIV;
        count -= face->type1.private_dict.lenIV;
      }

      error = T1_Add_Table( &parser->table, index, base, count );
    }

    /* consume the closing NP or `put' */
    error = Expect_Keyword2( parser, key_NP, key_put );

  Exit:
    return error;

  Syntax_Error:
    return T1_Err_Syntax_Error;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Do_RD_CharStrings                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function performs an `RD' if in the CharStrings dictionary.   */
  /*    It simply records the array of bytecodes/charstrings corresponding */
  /*    to the glyph program string.                                       */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    parser :: A handle to the current parser.                          */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  static
  FT_Error  Do_RD_Charstrings( T1_Parser*  parser )
  {
    FT_Error      error = T1_Err_Ok;
    T1_Face       face  = parser->face;
    T1_Token*     top   = parser->top;
    T1_Tokenizer  tokzer = parser->tokenizer;
    FT_Int        index, count;


    /* check the character name argument */
    if ( top[0].kind != tok_immediate )
    {
      FT_ERROR(( "Do_RD_Charstrings: immediate character name expected\n" ));
      goto Syntax_Error;
    }

    /* check the count argument */
    if ( top[1].kind != tok_number )
    {
      FT_ERROR(( "Do_RD_Charstrings: number expected\n" ));
      goto Syntax_Error;
    }

    parser->args++;
    count = (FT_Int)CopyInteger( parser );
    error = parser->error;
    if ( error )
      goto Exit;

    /* record the glyph name and get the corresponding glyph index */
    if ( top[0].kind2 == imm_notdef )
      index = 0;
    else
    {
      FT_String  temp_name[128];
      T1_Token*  token = top;
      FT_Int     len   = token->len - 1;


      /* copy immediate name */
      if ( len > 127 )
        len = 127;
      MEM_Copy( temp_name, parser->tokenizer->base + token->start + 1, len );
      temp_name[len] = '\0';

      index = parser->cur_name++;
      error = T1_Add_Table( &parser->table, index * 2,
                            (FT_Byte*)temp_name, len + 1 );
      if ( error )
        goto Exit;
    }

    /* decrypt and record charstring, then skip them */
    {
      FT_Byte*  base = tokzer->base + tokzer->cursor;


      tokzer->cursor += count;  /* skip */

      if ( face->type1.private_dict.lenIV >= 0 )
      {
        t1_decrypt( base, count, 4330 );

        base  += face->type1.private_dict.lenIV;
        count -= face->type1.private_dict.lenIV;
      }

      error = T1_Add_Table( &parser->table, index * 2 + 1, base, count );
    }

    /* consume the closing `ND' */
    if ( !error )
      error = Expect_Keyword( parser, key_ND );

  Exit:
    return error;

  Syntax_Error:
    return T1_Err_Syntax_Error;
  }


  static
  FT_Error  Expect_Dict_Arguments( T1_Parser*    parser,
                                   FT_Int        num_args,
                                   T1_TokenType  immediate,
                                   T1_DictState  new_state,
                                   FT_Int*       count )
  {
    /* check that we have enough arguments in the stack, including */
    /* the `dict' keyword                                          */
    if ( parser->top - parser->stack < num_args )
    {
      FT_ERROR(( "Expect_Dict_Arguments: expecting at least %d arguments",
                 num_args ));
      goto Syntax_Error;
    }

    /* check that we have the correct immediate, if needed */
    if ( num_args == 2 )
    {
      if ( parser->top[-2].kind  != tok_immediate ||
           parser->top[-2].kind2 != immediate     )
      {
        FT_ERROR(( "Expect_Dict_Arguments: expecting `/%s' dictionary\n",
                   t1_immediates[immediate - imm_first_] ));
        goto Syntax_Error;
      }
    }

    parser->args = parser->top-1;

    /* check that the count argument is a number */
    if ( parser->args->kind != tok_number )
    {
      FT_ERROR(( "Expect_Dict_Arguments:" ));
      FT_ERROR(( " expecting numerical count argument for `dict'\n" ));
      goto Syntax_Error;
    }

    if ( count )
    {
      *count = CopyInteger( parser );
      if ( parser->error )
        return parser->error;
    }

    /* save the dictionary state */
    parser->state_stack[++parser->state_index] = new_state;

    /* consume the `begin' keyword and clear the stack */
    parser->top -= num_args;
    return Expect_Keyword( parser, key_begin );

  Syntax_Error:
    return T1_Err_Syntax_Error;
  }


  static
  FT_Error  Expect_Array_Arguments( T1_Parser*  parser )
  {
    T1_Token*     top   = parser->top;
    FT_Error      error = T1_Err_Ok;
    T1_DictState  new_state;
    FT_Int        count;
    T1_Face       face   = parser->face;
    FT_Memory     memory = face->root.memory;


    /* Check arguments format */
    if ( top - parser->stack < 2 )
    {
      FT_ERROR(( "Expect_Array_Arguments: two arguments expected\n" ));
      error = T1_Err_Stack_Underflow;
      goto Exit;
    }

    parser->top -= 2;
    top         -= 2;
    parser->args = top + 1;

    if ( top[0].kind != tok_immediate )
    {
      FT_ERROR(( "Expect_Array_Arguments:" ));
      FT_ERROR(( " first argument must be an immediate name\n" ));
      goto Syntax_Error;
    }

    if ( top[1].kind != tok_number )
    {
      FT_ERROR(( "Expect_Array_Arguments:" ));
      FT_ERROR(( " second argument must be a number\n" ));
      goto Syntax_Error;
    }

    count = (FT_Int)CopyInteger( parser );

    /* Is this an array we know about? */
    switch ( top[0].kind2 )
    {
    case imm_Encoding:
      {
        T1_Encoding*  encode = &face->type1.encoding;


        new_state = dict_encoding;

        encode->code_first = count;
        encode->code_last  = 0;
        encode->num_chars  = count;

        /* Allocate the table of character indices.  The table of   */
        /* character names is allocated through init_t1_recorder(). */
        if ( ALLOC_ARRAY( encode->char_index, count, FT_Short ) )
          return error;

        error = T1_New_Table( &parser->table, count, memory );
        if ( error )
          goto Exit;

        parser->encoding_type = t1_encoding_array;
      }
      break;

    case imm_Subrs:
      new_state             = dict_subrs;
      face->type1.num_subrs = count;

      error = T1_New_Table( &parser->table, count, memory );
      if ( error )
        goto Exit;
      break;

    case imm_CharStrings:
      new_state = dict_charstrings;
      break;

    default:
      new_state = dict_unknown_array;
    }

    parser->state_stack[++parser->state_index] = new_state;

  Exit:
    return error;

  Syntax_Error:
    return T1_Err_Syntax_Error;
  }


  static
  FT_Error  Finalize_Parsing( T1_Parser*  parser )
  {
    T1_Face             face    = parser->face;
    T1_Font*            type1   = &face->type1;
    FT_Memory           memory  = face->root.memory;
    T1_Table*           strings = &parser->table;
    PSNames_Interface*  psnames = (PSNames_Interface*)face->psnames;

    FT_Int              num_glyphs;
    FT_Int              n;
    FT_Error            error;


    num_glyphs = type1->num_glyphs = parser->cur_name;

    /* allocate glyph names and charstrings arrays */
    if ( ALLOC_ARRAY( type1->glyph_names,     num_glyphs, FT_String* ) ||
         ALLOC_ARRAY( type1->charstrings,     num_glyphs, FT_Byte* )   ||
         ALLOC_ARRAY( type1->charstrings_len, num_glyphs, FT_Int*  )   )
      return error;

    /* copy glyph names and charstrings offsets and lengths */
    type1->charstrings_block = strings->block;
    for ( n = 0; n < num_glyphs; n++ )
    {
      type1->glyph_names[n]     = (FT_String*)strings->elements[2 * n];
      type1->charstrings[n]     = strings->elements[2 * n + 1];
      type1->charstrings_len[n] = strings->lengths [2 * n + 1];
    }

    /* now free the old tables */
    FREE( strings->elements );
    FREE( strings->lengths );

    if ( !psnames )
    {
      FT_ERROR(( "Finalize_Parsing: `PSNames' module missing!\n" ));
      return T1_Err_Unimplemented_Feature;
    }

    /* compute encoding if required */
    if ( parser->encoding_type == t1_encoding_none )
    {
      FT_ERROR(( "Finalize_Parsing: no encoding specified in font file\n" ));
      return T1_Err_Syntax_Error;
    }

    {
      FT_Int        n;
      T1_Encoding*  encode = &type1->encoding;


      encode->code_first = encode->num_chars - 1;
      encode->code_last  = 0;

      for ( n = 0; n < encode->num_chars; n++ )
      {
        FT_String** names;
        FT_Int      index;
        FT_Int      m;


        switch ( parser->encoding_type )
        {
        case t1_encoding_standard:
          index = psnames->adobe_std_encoding[n];
          names = 0;
          break;

        case t1_encoding_expert:
          index = psnames->adobe_expert_encoding[n];
          names = 0;
          break;

        default:
          index = n;
          names = (FT_String**)parser->encoding_offsets;
        }

        encode->char_index[n] = 0;

        if ( index )
        {
          FT_String*  name;


          if ( names )
            name = names[index];
          else
            name = (FT_String*)psnames->adobe_std_strings(index);

          if ( name )
          {
            FT_Int  len = strlen( name );


            /* lookup glyph index from name */
            for ( m = 0; m < num_glyphs; m++ )
            {
              if ( strncmp( type1->glyph_names[m], name, len ) == 0 )
              {
                encode->char_index[n] = m;
                break;
              }
            }

            if ( n < encode->code_first ) encode->code_first = n;
            if ( n > encode->code_last  ) encode->code_last  = n;
          }
        }
      }

      parser->encoding_type = t1_encoding_none;

      FREE( parser->encoding_names );
      FREE( parser->encoding_lengths );
      FREE( parser->encoding_offsets );
    }

    return T1_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Parse_T1_FontProgram                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Parses a given Type 1 font file and builds its face object.        */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    parser :: A handle to the target parser object.                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The parser contains a handle to the target face object.            */
  /*                                                                       */
  LOCAL_FUNC
  FT_Error  Parse_T1_FontProgram( T1_Parser*  parser )
  {
    FT_Error  error;
    T1_Font*  type1 = &parser->face->type1;


    for (;;)
    {
      T1_Token      token;
      T1_Token*     top;
      T1_DictState  dict_state;
      FT_Int        dict_index;


      error      = Next_T1_Token( parser, &token );
      top        = parser->top;
      dict_index = parser->state_index;
      dict_state = parser->state_stack[dict_index];

      switch ( token.kind )
      {
        /* a keyword has been detected */
      case tok_keyword:
        switch ( token.kind2 )
        {
        case key_dict:
          switch ( dict_state )
          {
          case dict_none:
            /* All right, we are beginning the font dictionary.  */
            /* Check that we only have one number argument, then */
            /* consume the `begin' and change to `dict_font'     */
            /* state.                                            */
            error = Expect_Dict_Arguments( parser, 1, tok_error,
                                           dict_font, 0 );
            if ( error )
              goto Exit;

            /* clear stack from all the previous content.  This */
            /* could be some stupid Postscript code.            */
            parser->top = parser->stack;
            break;

          case dict_font:
            /* This must be the /FontInfo dictionary, so check */
            /* that we have at least two arguments, that they  */
            /* are `/FontInfo' and a number, then change the   */
            /* dictionary state.                               */
            error = Expect_Dict_Arguments( parser, 2, imm_FontInfo,
                                           dict_fontinfo, 0 );
            if ( error )
              goto Exit;
            break;

          case dict_none2:
            error = Expect_Dict_Arguments( parser, 2, imm_Private,
                                           dict_private, 0 );
            if ( error )
              goto Exit;
            break;

          case dict_private:
            {
              T1_Face  face = parser->face;
              FT_Int   count;


              error = Expect_Dict_Arguments( parser, 2, imm_CharStrings,
                                             dict_charstrings, &count );
              if ( error )
                goto Exit;

              type1->num_glyphs = count;
              error = T1_New_Table( &parser->table, count * 2,
                                    face->root.memory );
              if ( error )
                goto Exit;

              /* record `.notdef' as the first glyph in the font */
              error = T1_Add_Table( &parser->table, 0,
                                    (FT_Byte*)".notdef", 8 );
              parser->cur_name = 1;
              /* XXX: DO SOMETHING HERE */
            }
            break;

          default:
            /* All other uses are invalid */
            FT_ERROR(( "Parse_T1_FontProgram:" ));
            FT_ERROR(( " invalid use of `dict' keyword\n" ));
            goto Syntax_Error;
          }
          break;

        case key_array:
          /* Are we in an array yet?  If so, raise an error */
          switch ( dict_state )
          {
          case dict_encoding:
          case dict_subrs:
          case dict_othersubrs:
          case dict_charstrings:
          case dict_unknown_array:
            FT_ERROR(( "Parse_T1_FontProgram: nested array definitions\n" ));
            goto Syntax_Error;

          default:
            ;
          }
          error = Expect_Array_Arguments( parser );
          if ( error )
            goto Exit;
          break;

        case key_ND:
        case key_NP:
        case key_def:
          /* Are we in an array? If so, finalize it. */
          switch ( dict_state )
          {
          case dict_encoding:    /* finish encoding array */
            /* copy table names to the face object */
            T1_Done_Table( &parser->table );

            parser->encoding_names   = parser->table.block;
            parser->encoding_lengths = parser->table.lengths;
            parser->encoding_offsets = parser->table.elements;

            parser->state_index--;
            break;

          case dict_subrs:
            /* copy recorder sub-routines */
            T1_Done_Table( &parser->table );

            parser->subrs      = parser->table.block;
            type1->subrs       = parser->table.elements;
            type1->subrs_len   = parser->table.lengths;
            type1->subrs_block = parser->table.block;

            parser->state_index--;
            break;

          case dict_charstrings:
          case dict_othersubrs:
          case dict_unknown_array:
            FT_ERROR(( "Parse_T1_FontProgram: unsupported array\n" ));
            goto Syntax_Error;
            break;

          default:   /* normal `def' processing */
            /* Check that we have sufficient operands in the stack */
            if ( top >= parser->stack + 2 )
            {
              /* Now check that the first operand is an immediate. */
              /* If so, call the appropriate `def' routine based   */
              /* on the current parser state.                      */
              if ( top[-2].kind == tok_immediate )
              {
                parser->top -= 2;
                parser->args = parser->top + 1;
                error = def_funcs[dict_state](parser);
              }
              else
              {
                /* This is an error, but some fonts contain  */
                /* stupid Postscript code.  We simply ignore */
                /* an invalid `def' by clearing the stack.   */
#if 0
                FT_ERROR(( "Parse_T1_FontProgram: immediate expected\n" ));
                goto Syntax_Error;
#else
                parser->top = parser->stack;
#endif
              }
            }
            else
            {
              FT_ERROR(( "Parse_T1_FontProgram: not enough arguments\n" ));
              goto Stack_Underflow;
            }
          }
          break;

        case key_index:
          if ( top <= parser->stack )
          {
            FT_ERROR(( "Parse_T1_FontProgram: not enough arguments\n" ));
            goto Stack_Underflow;
          }

          /* simply ignore? */
          parser->top --;
          break;

        case key_put:
          /* Check that we have sufficient operands in stack */
          if ( top < parser->stack + 2 )
          {
            FT_ERROR(( "Parse_T1_FontProgram: not enough arguments\n" ));
            goto Stack_Underflow;
          }

          parser->top -= 2;
          parser->args = parser->top;

          switch ( dict_state )
          {
          case dict_encoding:
            error = Do_Put_Encoding( parser );
            if ( error )
              goto Exit;
            break;

          case dict_unknown_array:   /* ignore the `put' */
            break;

          default:
#if 0
            FT_ERROR(( "Parse_T1_FontProgram: invalid context\n" ));
            goto Syntax_Error;
#else
            /* invalid context; simply ignore the `put' and */
            /* clear the stack (stupid Postscript code)     */
            FT_TRACE4(( "Parse_T1_FontProgram: invalid context ignored.\n" ));
            parser->top = parser->stack;
#endif
          }
          break;

        case key_RD:
          /* Check that we have sufficient operands in stack */
          if ( top < parser->stack + 2 )
          {
            FT_ERROR(( "Parse_T1_FontProgram: not enough arguments\n" ));
            goto Stack_Underflow;
          }

          parser->top -= 2;
          parser->args = parser->top;
          switch ( dict_state )
          {
          case dict_subrs:
            error = Do_RD_Subrs( parser );
            if ( error )
              goto Exit;
            break;

          case dict_charstrings:
            error = Do_RD_Charstrings( parser );
            if ( error )
              goto Exit;
            break;

          default:
            FT_ERROR(( "Parse_T1_FontProgram: invalid context\n" ));
            goto Syntax_Error;
          }
          break;

        case key_end:
          /* Were we in a dictionary or in an array? */
          if ( dict_index <= 0 )
          {
            FT_ERROR(( "Parse_T1_FontProgram: no dictionary defined\n" ));
            goto Syntax_Error;
          }

          switch ( dict_state )
          {
            /* jump to the private dictionary if we are closing the */
            /* `/Font' dictionary                                   */
          case dict_font:
            goto Open_Private;

            /* exit the parser when closing the CharStrings dictionary */
          case dict_charstrings:
            return Finalize_Parsing( parser );

          default:
            /* Pop the current dictionary state and return to previous */
            /* one.  Consume the `def'.                                */

            /* Because some buggy fonts (BitStream) have incorrect     */
            /* syntax, we never escape from the private dictionary     */
            if ( dict_state != dict_private )
              parser->state_index--;

            /* many fonts use `NP' instead of `def' or `put', so */
            /* we simply ignore the next token                   */
#if 0
            error = Expect_Keyword2( parser, key_def, key_put );
            if ( error )
              goto Exit;
#else
            (void)Expect_Keyword2( parser, key_def, key_put );
#endif
          }
          break;

        case key_for:
          /* check that we have four arguments and simply */
          /* ignore them                                  */
          if ( top - parser->stack < 4 )
          {
            FT_ERROR(( "Parse_T1_FontProgram: not enough arguments\n" ));
            goto Stack_Underflow;
          }

          parser->top -= 4;
          break;

        case key_currentdict:
        Open_Private:
          parser->state_index    = 0;
          parser->state_stack[0] = dict_none2;
          error = Open_PrivateDict( parser->tokenizer );
          if ( error )
            goto Exit;
          break;

        case key_true:
        case key_false:
        case key_StandardEncoding:
        case key_ExpertEncoding:
          goto Push_Element;

        default:
          FT_ERROR(( "Parse_T1_FontProgram:" ));
          FT_ERROR(( " invalid keyword in context\n" ));
          error = T1_Err_Syntax_Error;
        }
        break;

        /* check for the presence of `/BlendAxisTypes' -- we cannot deal */
        /* with multiple master fonts, so we must return a correct error */
        /* code to allow another driver to load them                     */
      case tok_immediate:
        if ( token.kind2 == imm_BlendAxisTypes )
        {
          error = FT_Err_Unknown_File_Format;
          goto Exit;
        }
        /* fallthrough */

        /* A number was detected */
      case tok_string:
      case tok_program:
      case tok_array:
      case tok_hexarray:
      case tok_any:
      case tok_number:                        /* push number on stack */

      Push_Element:
        if ( top >= parser->limit )
        {
          error = T1_Err_Stack_Overflow;
          goto Exit;
        }
        else
          *parser->top++ = token;
        break;

        /* anything else is an error per se the spec, but we     */
        /* frequently encounter stupid postscript code in fonts, */
        /* so just ignore them                                   */
      default:
        error = T1_Err_Ok;  /* ignore token */
      }

      if ( error )
        return error;
    }

  Exit:
    return error;

  Syntax_Error:
    return T1_Err_Syntax_Error;

  Stack_Underflow:
    return T1_Err_Stack_Underflow;
  }


/* END */
