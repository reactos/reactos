/***************************************************************************/
/*                                                                         */
/*  fttypes.h                                                              */
/*                                                                         */
/*    FreeType simple types definitions (specification only).              */
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


#ifndef FTTYPES_H
#define FTTYPES_H


#include <freetype/ftsystem.h>
#include <freetype/ftimage.h>


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Bool                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A typedef of unsigned char, used for simple booleans.              */
  /*                                                                       */
  typedef unsigned char  FT_Bool;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_FWord                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A signed 16-bit integer used to store a distance in original font  */
  /*    units.                                                             */
  /*                                                                       */
  typedef signed short  FT_FWord;   /* distance in FUnits */


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_UFWord                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    An unsigned 16-bit integer used to store a distance in original    */
  /*    font units.                                                        */
  /*                                                                       */
  typedef unsigned short  FT_UFWord;  /* unsigned distance */


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Char                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A simple typedef for the _signed_ char type.                       */
  /*                                                                       */
  typedef signed char  FT_Char;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Byte                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A simple typedef for the _unsigned_ char type.                     */
  /*                                                                       */
  typedef unsigned char  FT_Byte;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_String                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A simple typedef for the char type, usually used for strings.      */
  /*                                                                       */
  typedef char  FT_String;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Short                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A typedef for signed short.                                        */
  /*                                                                       */
  typedef signed short  FT_Short;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_UShort                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A typedef for unsigned short.                                      */
  /*                                                                       */
  typedef unsigned short  FT_UShort;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Int                                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A typedef for the int type.                                        */
  /*                                                                       */
  typedef int  FT_Int;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_UInt                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A typedef for the unsigned int type.                               */
  /*                                                                       */
  typedef unsigned int  FT_UInt;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Long                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A typedef for signed long.                                         */
  /*                                                                       */
  typedef signed long  FT_Long;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_ULong                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A typedef for unsigned long.                                       */
  /*                                                                       */
  typedef unsigned long  FT_ULong;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_F2Dot14                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A signed 2.14 fixed float type used for unit vectors.              */
  /*                                                                       */
  typedef signed short  FT_F2Dot14;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_F26Dot6                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A signed 26.6 fixed float type used for vectorial pixel            */
  /*    coordinates.                                                       */
  /*                                                                       */
  typedef signed long  FT_F26Dot6;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Fixed                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This type is used to store 16.16 fixed float values, like scales   */
  /*    or matrix coefficients.                                            */
  /*                                                                       */
  typedef signed long  FT_Fixed;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Error                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The FreeType error code type.  A value of 0 is always interpreted  */
  /*    as a successful operation.                                         */
  /*                                                                       */
  typedef int  FT_Error;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Pointer                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A simple typedef for a typeless pointer.                           */
  /*                                                                       */
  typedef void*  FT_Pointer;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_UnitVector                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A simple structure used to store a 2D vector unit vector.  Uses    */
  /*    FT_F2Dot14 types.                                                  */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    x :: Horizontal coordinate.                                        */
  /*                                                                       */
  /*    y :: Vertical coordinate.                                          */
  /*                                                                       */
  typedef struct  FT_UnitVector_
  {
    FT_F2Dot14  x;
    FT_F2Dot14  y;

  } FT_UnitVector;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_Matrix                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A simple structure used to store a 2x2 matrix.  Coefficients are   */
  /*    in 16.16 fixed float format.  The computation performed is:        */
  /*                                                                       */
  /*       {                                                               */
  /*          x' = x*xx + y*xy                                             */
  /*          y' = x*yx + y*yy                                             */
  /*       }                                                               */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    xx :: Matrix coefficient.                                          */
  /*                                                                       */
  /*    xy :: Matrix coefficient.                                          */
  /*                                                                       */
  /*    yx :: Matrix coefficient.                                          */
  /*                                                                       */
  /*    yy :: Matrix coefficient.                                          */
  /*                                                                       */
  typedef struct  FT_Matrix_
  {
    FT_Fixed  xx, xy;
    FT_Fixed  yx, yy;

  } FT_Matrix;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_BBox                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to hold an outline's bounding box, i.e., the      */
  /*    coordinates of its extrema in the horizontal and vertical          */
  /*    directions.                                                        */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    xMin :: The horizontal minimum (left-most).                        */
  /*                                                                       */
  /*    yMin :: The vertical minimum (bottom-most).                        */
  /*                                                                       */
  /*    xMax :: The horizontal maximum (right-most).                       */
  /*                                                                       */
  /*    yMax :: The vertical maximum (top-most).                           */
  /*                                                                       */
  typedef struct  FT_BBox_
  {
    FT_Pos  xMin, yMin;
    FT_Pos  xMax, yMax;

  } FT_BBox;


  /*************************************************************************/
  /*                                                                       */
  /* <Macro>                                                               */
  /*    FT_MAKE_TAG                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This macro converts four letter tags which are used to label       */
  /*    TrueType tables into an unsigned long to be used within FreeType.  */
  /*                                                                       */
#define FT_MAKE_TAG( _x1, _x2, _x3, _x4 ) \
          ( ( (FT_ULong)_x1 << 24 ) |     \
            ( (FT_ULong)_x2 << 16 ) |     \
            ( (FT_ULong)_x3 <<  8 ) |     \
              (FT_ULong)_x4         )


  /*************************************************************************/
  /*************************************************************************/
  /*                                                                       */
  /*                    L I S T   M A N A G E M E N T                      */
  /*                                                                       */
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_ListNode                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*     Many elements and objects in FreeType are listed through a        */
  /*     FT_List record (see FT_ListRec).  As its name suggests, a         */
  /*     FT_ListNode is a handle to a single list element.                 */
  /*                                                                       */
  typedef struct FT_ListNodeRec_*  FT_ListNode;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_List                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a list record (see FT_ListRec).                        */
  /*                                                                       */
  typedef struct FT_ListRec_*  FT_List;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_ListNodeRec                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to hold a single list element.                    */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    prev :: The previous element in the list.  NULL if first.          */
  /*                                                                       */
  /*    next :: The next element in the list.  NULL if last.               */
  /*                                                                       */
  /*    data :: A typeless pointer to the listed object.                   */
  /*                                                                       */
  typedef struct  FT_ListNodeRec_
  {
    FT_ListNode  prev;
    FT_ListNode  next;
    void*        data;

  } FT_ListNodeRec;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_ListRec                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to hold a simple doubly-linked list.  These are   */
  /*    used in many parts of FreeType.                                    */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    head :: The head (first element) of doubly-linked list.            */
  /*                                                                       */
  /*    tail :: The tail (last element) of doubly-linked list.             */
  /*                                                                       */
  typedef struct  FT_ListRec_
  {
    FT_ListNode  head;
    FT_ListNode  tail;

  } FT_ListRec;


#define FT_IS_EMPTY( list )  ( (list).head == 0 )


#endif /* FTTYPES_H */


/* END */
