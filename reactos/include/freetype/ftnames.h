/***************************************************************************/
/*                                                                         */
/*  ftnames.h                                                              */
/*                                                                         */
/*    Simple interface to access SFNT name tables (which are used          */
/*    to hold font names, copyright info, notices, etc.).                  */
/*                                                                         */
/*    This is _not_ used to retrieve glyph names!                          */
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


#ifndef FTNAMES_H
#define FTNAMES_H


#include <freetype/freetype.h>


  typedef struct  FT_SfntName_
  {
    FT_UShort  platform_id;
    FT_UShort  encoding_id;
    FT_UShort  language_id;
    FT_UShort  name_id;
  
    FT_Byte*   string;
    FT_UInt    string_len;  /* in bytes */
  
  } FT_SfntName;


  FT_EXPORT_DEF( FT_UInt )  FT_Get_Sfnt_Name_Count( FT_Face  face );
  
  FT_EXPORT_DEF( FT_Error )  FT_Get_Sfnt_Name( FT_Face       face,
                                               FT_UInt       index,
                                               FT_SfntName*  aname );
                                               

#endif /* FTNAMES_H */


/* END */
