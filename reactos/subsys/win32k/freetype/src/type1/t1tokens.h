/***************************************************************************/
/*                                                                         */
/*  t1tokens.h                                                             */
/*                                                                         */
/*    Type 1 tokenizer (specification).                                    */
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


#ifndef T1TOKENS_H
#define T1TOKENS_H


#ifdef FT_FLAT_COMPILE

#include "t1objs.h"

#else

#include <freetype/src/type1/t1objs.h>

#endif


#ifdef __cplusplus
  extern "C" {
#endif


  /* enum value of first keyword */
#define key_first_  100

  /* enum value of first immediate name */
#define imm_first_  200


  typedef enum  T1_TokenType_
  {
    tok_error = 0,

    tok_eof,             /* end of file                  */

    /* simple token types */

    tok_keyword,         /* keyword                      */
    tok_number,          /* number (integer or real)     */
    tok_string,          /* postscript string            */
    tok_program,         /* postscript program           */
    tok_immediate,       /* any immediate name           */
    tok_array,           /* matrix, array, etc..         */
    tok_hexarray,        /* array of hexadecimal nibbles */
    tok_any,             /* anything else                */

    /* Postscript keywords -- placed in lexicographical order */

    key_RD_alternate = key_first_,      /* `-|' = alternate form of RD */
    key_ExpertEncoding,
    key_ND,
    key_NP,
    key_RD,
    key_StandardEncoding,
    key_array,
    key_begin,
    key_closefile,
    key_currentdict,
    key_currentfile,
    key_def,
    key_dict,
    key_dup,
    key_eexec,
    key_end,
    key_execonly,
    key_false,
    key_for,
    key_index,
    key_noaccess,
    key_put,
    key_readonly,
    key_true,
    key_userdict,
    key_NP_alternate,                   /* `|' = alternate form of NP  */
    key_ND_alternate,                   /* `|-' = alternate form of ND */

    key_max,   /* always keep this value there */

    /* Postscript immediate names -- other names will be ignored, except */
    /* in charstrings                                                    */

    imm_RD_alternate = imm_first_,      /* `-|' = alternate form of RD */
    imm_notdef,                         /* `/.notdef' immediate        */
    imm_BlendAxisTypes,
    imm_BlueFuzz,
    imm_BlueScale,
    imm_BlueShift,
    imm_BlueValues,
    imm_CharStrings,
    imm_Encoding,
    imm_FamilyBlues,
    imm_FamilyName,
    imm_FamilyOtherBlues,
    imm_FID,
    imm_FontBBox,
    imm_FontID,
    imm_FontInfo,
    imm_FontMatrix,
    imm_FontName,
    imm_FontType,
    imm_ForceBold,
    imm_FullName,
    imm_ItalicAngle,
    imm_LanguageGroup,
    imm_Metrics,
    imm_MinFeature,
    imm_ND,
    imm_NP,
    imm_Notice,
    imm_OtherBlues,
    imm_OtherSubrs,
    imm_PaintType,
    imm_Private,
    imm_RD,
    imm_RndStemUp,
    imm_StdHW,
    imm_StdVW,
    imm_StemSnapH,
    imm_StemSnapV,
    imm_StrokeWidth,
    imm_Subrs,
    imm_UnderlinePosition,
    imm_UnderlineThickness,
    imm_UniqueID,
    imm_Weight,

    imm_isFixedPitch,
    imm_lenIV,
    imm_password,
    imm_version,

    imm_NP_alternate,                   /* `|'  = alternate form of NP */
    imm_ND_alternate,                   /* `|-' = alternate form of ND */

    imm_max   /* always keep this value here */

  } T1_TokenType;


  /* these arrays are visible for debugging purposes */
  extern const char*  t1_keywords[];
  extern const char*  t1_immediates[];


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    T1_Token                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to describe a token in the current input stream.  */
  /*    Note that the Type1 driver doesn't try to interpret tokens until   */
  /*    it really needs to.                                                */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    kind  :: The token type.  Describes the token to the loader.       */
  /*                                                                       */
  /*    kind2 :: Detailed token type.                                      */
  /*                                                                       */
  /*    start :: The index of the first character of token in the input    */
  /*             stream.                                                   */
  /*                                                                       */
  /*    len   :: The length of the token in characters.                    */
  /*                                                                       */
  typedef struct  T1_Token_
  {
    T1_TokenType  kind;     /* simple type                    */
    T1_TokenType  kind2;    /* detailed type                  */
    FT_Int        start;    /* index of first token character */
    FT_Int        len;      /* length of token in chars       */

  } T1_Token;


  typedef struct  T1_TokenParser_
  {
    FT_Memory  memory;
    FT_Stream  stream;

    FT_Bool    in_pfb;      /* true if PFB file, PFA otherwise */
    FT_Bool    in_private;  /* true if in private dictionary   */

    FT_Byte*   base;        /* base address of current read buffer */
    FT_Long    cursor;      /* current position in read buffer     */
    FT_Long    limit;       /* limit of current read buffer        */
    FT_Long    max;         /* maximum size of read buffer         */

    FT_Error   error;       /* last error                          */
    T1_Token   token;       /* last token read                     */

  } T1_TokenParser;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    T1_Tokenizer                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to an object used to extract tokens from the input.  The  */
  /*    object is able to perform PFA/PFB recognition, eexec decryption of */
  /*    the private dictionary, as well as eexec decryption of the         */
  /*    charstrings.                                                       */
  /*                                                                       */
  typedef T1_TokenParser*  T1_Tokenizer;


  LOCAL_DEF
  FT_Error  New_Tokenizer( FT_Stream      stream,
                           T1_Tokenizer*  tokenizer );

  LOCAL_DEF
  FT_Error  Done_Tokenizer( T1_Tokenizer  tokenizer );

  LOCAL_DEF
  FT_Error  Open_PrivateDict( T1_Tokenizer  tokenizer );

  LOCAL_DEF
  FT_Error  Read_Token( T1_Tokenizer  tokenizer );


#if 0
  LOCAL_DEF
  FT_Error  Read_CharStrings( T1_Tokenizer  tokenizer,
                              FT_Int        num_chars,
                              FT_Byte*      buffer );
#endif /* 0 */

  LOCAL_DEF
  void  t1_decrypt( FT_Byte*   buffer,
                    FT_Int     length,
                    FT_UShort  seed );


#ifdef __cplusplus
  }
#endif

#endif /* T1TOKENS_H */


/* END */
