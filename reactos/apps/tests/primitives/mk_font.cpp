                    
// ------------------------------------------------------------------
// Windows 2000 Graphics API Black Book
// Chapter 4 - Utility functions
//
// Created by Damon Chandler <dmc27@ee.cornell.edu>
// Updates can be downloaded at: <www.coriolis.com>
//
// Please do not hesistate to e-mail me at dmc27@ee.cornell.edu 
// if you have any questions about this code.
// ------------------------------------------------------------------

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#include <windows.h>
#include <cassert>

#include "mk_font.h"
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

namespace font {

// creates a logical font
HFONT MakeFont(
    IN HDC hDestDC,           // handle to target DC
    IN LPCSTR typeface_name,  // font's typeface name
    IN int point_size,        // font's point size 
    IN const BYTE charset,    // font's character set
    IN const DWORD style      // font's styles
   )
{
   //
   // NOTE: On Windows 9x/Me, GetWorldTransform is not
   // supported.  For compatibility with these platforms you
   // should initialize the XFORM::eM22 data member to 1.0.
   //
   XFORM xf = {0, 0, 0, 1.0};
   GetWorldTransform(hDestDC, &xf);
   int pixels_per_inch = GetDeviceCaps(hDestDC, LOGPIXELSY);

   POINT PSize = {
      0, 
      -MulDiv(static_cast<int>(xf.eM22 * point_size + 0.5), 
              pixels_per_inch, 72)
      };

   HFONT hResult = NULL;      
   if (DPtoLP(hDestDC, &PSize, 1))
   {
      LOGFONT lf;
      memset(&lf, 0, sizeof(LOGFONT));

      lf.lfHeight = PSize.y;
      lf.lfCharSet = charset;
      lstrcpyn(reinterpret_cast<LPTSTR>(&lf.lfFaceName),
               typeface_name, LF_FACESIZE);

      lf.lfWeight = (style & FS_BOLD) ? FW_BOLD : FW_DONTCARE;
      lf.lfItalic = (style & FS_ITALIC) ? true : false;
      lf.lfUnderline = (style & FS_UNDERLINE) ? true : false;
      lf.lfStrikeOut = (style & FS_STRIKEOUT) ? true : false;

      // create the logical font
      hResult = CreateFontIndirect(&lf);
   }
   return hResult;
}
//-------------------------------------------------------------------------

} // namespace font
