/***************************************************************************/
/*                                                                         */
/*  t2parse.c                                                              */
/*                                                                         */
/*    OpenType parser (body).                                              */
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


#ifdef FT_FLAT_COMPILE

#include "t2parse.h"

#else

#include <freetype/src/cff/t2parse.h>

#endif


#include <freetype/internal/t2errors.h>


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t2parse


#define T2_Err_Stack_Underflow   FT_Err_Invalid_Argument
#define T2_Err_Syntax_Error      FT_Err_Invalid_Argument


  enum
  {
    t2_kind_none = 0,
    t2_kind_num,
    t2_kind_fixed,
    t2_kind_string,
    t2_kind_bool,
    t2_kind_delta,
    t2_kind_callback,

    t2_kind_max  /* do not remove */
  };


  /* now generate handlers for the most simple fields */
  typedef FT_Error  (*T2_Field_Reader)( T2_Parser*  parser );

  typedef struct  T2_Field_Handler_
  {
    int              kind;
    int              code;
    FT_UInt          offset;
    FT_Byte          size;
    T2_Field_Reader  reader;
    FT_UInt          array_max;
    FT_UInt          count_offset;

  } T2_Field_Handler;


  LOCAL_FUNC
  void  T2_Parser_Init( T2_Parser*  parser,
                        FT_UInt     code,
                        void*       object )
  {
    MEM_Set( parser, 0, sizeof ( *parser ) );

    parser->top         = parser->stack;
    parser->object_code = code;
    parser->object      = object;
  }


  /* reads an integer */
  static
  FT_Long  parse_t2_integer( FT_Byte*  start,
                             FT_Byte*  limit )
  {
    FT_Byte*  p   = start;
    FT_Int    v   = *p++;
    FT_Long   val = 0;


    if ( v == 28 )
    {
      if ( p + 2 > limit )
        goto Bad;

      val = (FT_Short)( ( (FT_Int)p[0] << 8 ) | p[1] );
      p  += 2;
    }
    else if ( v == 29 )
    {
      if ( p + 4 > limit )
        goto Bad;

      val = ( (FT_Long)p[0] << 24 ) |
            ( (FT_Long)p[1] << 16 ) |
            ( (FT_Long)p[2] <<  8 ) |
                       p[3];
      p += 4;
    }
    else if ( v < 247 )
    {
      val = v - 139;
    }
    else if ( v < 251 )
    {
      if ( p + 1 > limit )
        goto Bad;

      val = ( v - 247 ) * 256 + p[0] + 108;
      p++;
    }
    else
    {
      if ( p + 1 > limit )
        goto Bad;

      val = -( v - 251 ) * 256 - p[0] - 108;
      p++;
    }

  Exit:
    return val;

  Bad:
    val = 0;
    goto Exit;
  }


  /* read a real */
  static
  FT_Fixed  parse_t2_real( FT_Byte*  start,
                           FT_Byte*  limit,
                           FT_Int    power_ten )
  {
    FT_Byte*  p    = start;
    FT_Long   num, divider, result, exp;
    FT_Int    sign = 0, exp_sign = 0;
    FT_Byte   nib;
    FT_Byte   phase;


    result  = 0;
    num     = 0;
    divider = 1;

    /* first of all, read the integer part */
    phase = 4;
    p--;

    for (;;)
    {
      /* read one nibble at a time */
      if ( phase && ++p >= limit )
        goto Bad;

      nib   = ( p[0] >> phase ) & 0xF;
      phase = 4 - phase;

      if ( nib == 0xE )
        sign = 1;
      else if ( nib > 9 )
        break;
      else
        result = result * 10 + nib;
    }

    /* read decimal part, if any */
    if ( nib == 0xa )
      for (;;)
      {
        /* read one nibble at a time */
        if ( !phase && ++p >= limit )
          goto Bad;

        phase = 4 - phase;
        nib   = ( p[0] >> phase ) & 0xF;

        if ( nib >= 10 )
          break;

        if (divider < 10000000L)
        {
          num      = num * 10 + nib;
          divider *= 10;
        }
      }

    /* read exponent, if any */
    if ( nib == 12 )
    {
      exp_sign = 1;
      nib      = 11;
    }

    if ( nib == 11 )
    {
      exp = 0;

      for (;;)
      {
        /* read one nibble at a time */
        if ( !phase && ++p >= limit )
          goto Bad;

        phase = 4 - phase;
        nib   = ( p[0] >> phase ) & 0xF;

        if ( nib >= 10 )
          break;

        exp = exp * 10 + nib;
      }

      if ( exp_sign )
        exp = -exp;

      power_ten += exp;
    }

    /* raise to power of ten if needed */
    while ( power_ten > 0 )
    {
      result = result * 10;
      num    = num * 10;

      power_ten--;
    }

    while ( power_ten < 0 )
    {
      result  = result / 10;
      divider = divider * 10;

      power_ten++;
    }

    if ( num )
      result += FT_DivFix( num, divider );

    if ( sign )
      result = -result;

  Exit:
    return result;

  Bad:
    result = 0;
    goto Exit;
  }


  /* read a number, either integer or real */
  static
  FT_Long  t2_parse_num( FT_Byte**  d )
  {
    return ( **d == 30 ? ( parse_t2_real( d[0], d[1], 0 ) >> 16 )
                       : parse_t2_integer( d[0], d[1] ) );
  }


  /* reads a floating point number, either integer or real */
  static
  FT_Fixed  t2_parse_fixed( FT_Byte**  d )
  {
    return ( **d == 30 ? parse_t2_real( d[0], d[1], 0 )
                       : parse_t2_integer( d[0], d[1] ) << 16 );
  }


  static
  FT_Error  parse_font_matrix( T2_Parser*  parser )
  {
    CFF_Font_Dict*  dict   = (CFF_Font_Dict*)parser->object;
    FT_Matrix*      matrix = &dict->font_matrix;
    FT_Byte**       data   = parser->stack;
    FT_Error        error;


    error = T2_Err_Stack_Underflow;

    if ( parser->top >= parser->stack + 4 )
    {
      matrix->xx = t2_parse_fixed( data++ );
      matrix->yx = t2_parse_fixed( data++ );
      matrix->xy = t2_parse_fixed( data++ );
      matrix->yy = t2_parse_fixed( data   );
      error = T2_Err_Ok;
    }

    return error;
  }


  static
  FT_Error  parse_font_bbox( T2_Parser*  parser )
  {
    CFF_Font_Dict*  dict = (CFF_Font_Dict*)parser->object;
    FT_BBox*        bbox = &dict->font_bbox;
    FT_Byte**       data = parser->stack;
    FT_Error        error;


    error = T2_Err_Stack_Underflow;

    if ( parser->top >= parser->stack + 4 )
    {
      bbox->xMin = t2_parse_num( data++ );
      bbox->yMin = t2_parse_num( data++ );
      bbox->xMax = t2_parse_num( data++ );
      bbox->yMax = t2_parse_num( data   );
      error = T2_Err_Ok;
    }

    return error;
  }


  static
  FT_Error  parse_private_dict( T2_Parser*  parser )
  {
    CFF_Font_Dict*  dict = (CFF_Font_Dict*)parser->object;
    FT_Byte**       data = parser->stack;
    FT_Error        error;


    error = T2_Err_Stack_Underflow;

    if ( parser->top >= parser->stack + 2 )
    {
      dict->private_size   = t2_parse_num( data++ );
      dict->private_offset = t2_parse_num( data   );
      error = T2_Err_Ok;
    }

    return error;
  }


  static
  FT_Error  parse_cid_ros( T2_Parser*  parser )
  {
    CFF_Font_Dict*  dict   = (CFF_Font_Dict*)parser->object;
    FT_Byte**       data   = parser->stack;
    FT_Error        error;


    error = T2_Err_Stack_Underflow;

    if ( parser->top >= parser->stack + 3 )
    {
      dict->cid_registry   = (FT_UInt)t2_parse_num( data++ );
      dict->cid_ordering   = (FT_UInt)t2_parse_num( data++ );
      dict->cid_supplement = (FT_ULong)t2_parse_num( data );
      error = T2_Err_Ok;
    }

    return error;
  }


#define T2_FIELD_NUM( code, name ) \
          T2_FIELD( code, name, t2_kind_num )
#define T2_FIELD_FIXED( code, name ) \
          T2_FIELD( code, name, t2_kind_fixed )
#define T2_FIELD_STRING( code, name ) \
          T2_FIELD( code, name, t2_kind_string )
#define T2_FIELD_BOOL( code, name ) \
          T2_FIELD( code, name, t2_kind_bool )
#define T2_FIELD_DELTA( code, name,max ) \
          T2_FIELD( code, name, t2_kind_delta )

#define T2_REF( s, f )  ( ((s*)0)->f )

#define T2_FIELD_CALLBACK( code, name ) \
          {                             \
            t2_kind_callback,           \
            code | T2CODE,              \
            0, 0,                       \
            parse_ ## name,             \
            0, 0                        \
          },

#undef  T2_FIELD
#define T2_FIELD( code, name, kind )                 \
          {                                          \
            kind,                                    \
            code | T2CODE,                           \
            (FT_UInt)(char*)&T2_REF( T2TYPE, name ), \
            sizeof( T2_REF( T2TYPE, name ) ),        \
            0, 0, 0                                  \
          },

#undef  T2_FIELD_DELTA
#define T2_FIELD_DELTA( code, name, max )                    \
        {                                                    \
          t2_kind_delta,                                     \
          code | T2CODE,                                     \
          (FT_UInt)(char*)&T2_REF( T2TYPE, name ),           \
          sizeof( T2_REF( T2TYPE, name )[0] ),               \
          0,                                                 \
          max,                                               \
          (FT_UInt)(char*)&T2_REF( T2TYPE, num_ ## name )    \
        },

#define T2CODE_TOPDICT  0x1000
#define T2CODE_PRIVATE  0x2000

  static const T2_Field_Handler  t2_field_handlers[] =
  {

#ifdef FT_FLAT_COMPILE

#include "t2tokens.h"

#else

#include <freetype/src/cff/t2tokens.h>

#endif

    { 0, 0, 0, 0, 0, 0, 0 }
  };


  LOCAL_FUNC
  FT_Error  T2_Parser_Run( T2_Parser*  parser,
                           FT_Byte*    start,
                           FT_Byte*    limit )
  {
    FT_Byte*  p     = start;
    FT_Error  error = T2_Err_Ok;


    parser->top    = parser->stack;
    parser->start  = start;
    parser->limit  = limit;
    parser->cursor = start;

    while ( p < limit )
    {
      FT_Byte  v = *p;


      if ( v >= 27 && v != 31 )
      {
        /* it's a number; we will push its position on the stack */
        if ( parser->top - parser->stack >= T2_MAX_STACK_DEPTH )
          goto Stack_Overflow;

        *parser->top ++ = p;

        /* now, skip it */
        if ( v == 30 )
        {
          /* skip real number */
          for (;;)
          {
            if ( p >= limit )
              goto Syntax_Error;
            v = p[0] >> 4;
            if ( v == 15 )
              break;
            v = p[0] & 0xF;
            if ( v == 15 )
              break;
            p++;
          }
          p++;
        }
        else if ( v == 28 )
          p += 2;
        else if ( v == 29 )
          p += 4;
        else if ( v > 246 )
          p += 1;
      }
      else
      {
        /* This is not a number, hence it's an operator.  Compute its code */
        /* and look for it in our current list.                            */

        FT_UInt                  code;
        FT_UInt                  num_args = (FT_UInt)
                                              ( parser->top - parser->stack );
        const T2_Field_Handler*  field;


        /* first of all, a trivial check */
        if ( num_args < 1 )
          goto Stack_Underflow;

        *parser->top = p;
        code = v;
        if ( v == 12 )
        {
          /* two byte operator */
          p++;
          code = 0x100 | p[0];
        }
        code = code | parser->object_code;

        for ( field = t2_field_handlers; field->kind; field++ )
        {
          if ( field->code == (FT_Int)code )
          {
            /* we found our field's handler; read it */
            FT_Long   val;
            FT_Byte*  q = (FT_Byte*)parser->object + field->offset;


            switch ( field->kind )
            {
            case t2_kind_bool:
            case t2_kind_string:
            case t2_kind_num:
              val = t2_parse_num( parser->stack );
              goto Store_Number;

            case t2_kind_fixed:
              val = t2_parse_fixed( parser->stack );

            Store_Number:
              switch ( field->size )
              {
              case 1:
                *(FT_Byte*)q = (FT_Byte)val;
                break;

              case 2:
                *(FT_Short*)q = (FT_Short)val;
                break;

              case 4:
                *(FT_Int32*)q = (FT_Int)val;
                break;

              default:  /* for 64-bit systems where long is 8 bytes */
                *(FT_Long*)q = val;
              }
              break;

            case t2_kind_delta:
              {
                FT_Byte*   qcount = (FT_Byte*)parser->object +
                                      field->count_offset;

                FT_Long    val;
                FT_Byte**  data = parser->stack;


                if ( num_args > field->array_max )
                  num_args = field->array_max;

                /* store count */
                *qcount = (FT_Byte)num_args;

                val = 0;
                while ( num_args > 0 )
                {
                  val += t2_parse_num( data++ );
                  switch ( field->size )
                  {
                  case 1:
                    *(FT_Byte*)q = (FT_Byte)val;
                    break;

                  case 2:
                    *(FT_Short*)q = (FT_Short)val;
                    break;

                  case 4:
                    *(FT_Int32*)q = (FT_Int)val;
                    break;

                  default:  /* for 64-bit systems */
                    *(FT_Long*)q = val;
                  }

                  q += field->size;
                  num_args--;
                }
              }
              break;

            default:  /* callback */
              error = field->reader( parser );
              if ( error )
                goto Exit;
            }
            goto Found;
          }
        }

        /* this is an unknown operator, or it is unsupported; */
        /* we will ignore it for now.                         */

      Found:
        /* clear stack */
        parser->top = parser->stack;
      }
      p++;
    }

  Exit:
    return error;

  Stack_Overflow:
    error = T2_Err_Invalid_Argument;
    goto Exit;

  Stack_Underflow:
    error = T2_Err_Invalid_Argument;
    goto Exit;

  Syntax_Error:
    error = T2_Err_Invalid_Argument;
    goto Exit;
  }


/* END */
