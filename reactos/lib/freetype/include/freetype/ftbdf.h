/***************************************************************************/
/*                                                                         */
/*  ftbdf.h                                                                */
/*                                                                         */
/*    FreeType API for accessing BDF-specific strings (specification).     */
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


#ifndef __FTBDF_H__
#define __FTBDF_H__

#include <ft2build.h>
#include FT_FREETYPE_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    bdf_fonts                                                          */
  /*                                                                       */
  /* <Title>                                                               */
  /*    BDF Fonts                                                          */
  /*                                                                       */
  /* <Abstract>                                                            */
  /*    BDF-specific APIs                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This section contains the declaration of BDF-specific functions.   */
  /*                                                                       */
  /*************************************************************************/


 /**********************************************************************
  *
  * @enum:
  *    FT_PropertyType
  *
  * @description:
  *    list of BDF property types
  *
  * @values:
  *    BDF_PROPERTY_TYPE_NONE ::
  *      value 0 is used to indicate a missing property
  *
  *    BDF_PROPERTY_TYPE_ATOM ::
  *      property is a string atom
  *
  *    BDF_PROPERTY_TYPE_INTEGER ::
  *      property is a 32-bit signed integer
  *
  *    BDF_PROPERTY_TYPE_CARDINAL ::
  *      property is a 32-bit unsigned integer
  */
  typedef enum
  {
    BDF_PROPERTY_TYPE_NONE     = 0,
    BDF_PROPERTY_TYPE_ATOM     = 1,
    BDF_PROPERTY_TYPE_INTEGER  = 2,
    BDF_PROPERTY_TYPE_CARDINAL = 3

  } BDF_PropertyType;


 /**********************************************************************
  *
  * @type:  BDF_Property
  *
  * @description:
  *    handle to a @BDF_PropertyRec structure used to model a given
  *    BDF/PCF property
  */
  typedef struct BDF_PropertyRec_*   BDF_Property;

 /**********************************************************************
  *
  * @struct:  BDF_PropertyRec
  *
  * @description:
  *    models a given BDF/PCF property
  *
  * @note:
  *    type       :: property type
  *    u.atom     :: atom string, when type is @BDF_PROPERTY_TYPE_ATOM
  *    u.integer  :: signed integer, when type is @BDF_PROPERTY_TYPE_INTEGER
  *    u.cardinal :: unsigned integer, when type is @BDF_PROPERTY_TYPE_CARDINAL
  */
  typedef struct BDF_PropertyRec_
  {
    BDF_PropertyType   type;
    union {
      const char*   atom;
      FT_Int32      integer;
      FT_UInt32     cardinal;

    } u;

  } BDF_PropertyRec;


 /**********************************************************************
  *
  * @function:
  *    FT_Get_BDF_Charset_ID
  *
  * @description:
  *    Retrieves a BDF font character set identity, according to
  *    the BDF specification.
  *
  * @input:
  *    face ::
  *       handle to input face
  *
  * @output:
  *    acharset_encoding ::
  *       Charset encoding, as a C string, owned by the face.
  *
  *    acharset_registry ::
  *       Charset registry, as a C string, owned by the face.
  *
  * @return:
  *   FreeType rror code.  0 means success.
  *
  * @note:
  *   This function only works with BDF faces, returning an error otherwise.
  */
  FT_EXPORT( FT_Error )
  FT_Get_BDF_Charset_ID( FT_Face       face,
                         const char*  *acharset_encoding,
                         const char*  *acharset_registry );

 /**********************************************************************
  *
  * @function:
  *    FT_Get_BDF_Property
  *
  * @description:
  *    Retrieves a BDF property from a BDF or PCF font file
  *
  * @input:
  *    face :: handle to input face
  *    name :: property name
  *
  * @output:
  *    aproperty :: the property
  *
  * @return:
  *   FreeType error code.  0 means success.
  *
  * @note:
  *   This function works with BDF _and_ PCF fonts. It returns an error
  *   otherwise. it also returns an error when the property is not in the
  *   font.
  *
  *   in case of error, "aproperty->type" is always set to
  *   @BDF_PROPERTY_TYPE_NONE
  */
  FT_EXPORT( FT_Error )
  FT_Get_BDF_Property( FT_Face           face,
                       const char*       prop_name,
                       BDF_PropertyRec  *aproperty );

 /* */

FT_END_HEADER

#endif /* __FTBDF_H__ */


/* END */
