/*  bdftypes.h

  FreeType font driver for bdf fonts

  Copyright (C) 2001, 2002 by
  Francesco Zappa Nardelli

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef __BDFTYPES_H__
#define __BDFTYPES_H__

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BDF_H


FT_BEGIN_HEADER


  typedef struct  BDF_Public_FaceRec_
  {
    FT_FaceRec  root;

    char*       charset_encoding;
    char*       charset_registry;

  } BDF_Public_FaceRec, *BDF_Public_Face;


  typedef FT_Error  (*BDF_GetPropertyFunc)( FT_Face           face,
                                            const char*       prop_name,
                                            BDF_PropertyRec  *aproperty );

FT_END_HEADER


#endif  /* __BDFTYPES_H__ */


/* END */
