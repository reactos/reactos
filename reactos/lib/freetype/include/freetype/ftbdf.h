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

 /* */

FT_END_HEADER

#endif /* __FTBDF_H__ */


/* END */
