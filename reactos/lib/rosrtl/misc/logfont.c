#include <windows.h>
#include <string.h>
#include <rosrtl/logfont.h>

void
RosRtlLogFontA2W ( LPLOGFONTW pW, const LPLOGFONTA pA )
{
#define COPYS(f,len) MultiByteToWideChar ( CP_THREAD_ACP, 0, pA->f, len, pW->f, len )
#define COPYN(f) pW->f = pA->f

  COPYN(lfHeight);
  COPYN(lfWidth);
  COPYN(lfEscapement);
  COPYN(lfOrientation);
  COPYN(lfWeight);
  COPYN(lfItalic);
  COPYN(lfUnderline);
  COPYN(lfStrikeOut);
  COPYN(lfCharSet);
  COPYN(lfOutPrecision);
  COPYN(lfClipPrecision);
  COPYN(lfQuality);
  COPYN(lfPitchAndFamily);
  COPYS(lfFaceName,LF_FACESIZE);

#undef COPYN
#undef COPYS
}

void
RosRtlLogFontW2A ( LPLOGFONTA pA, const LPLOGFONTW pW )
{
#define COPYS(f,len) WideCharToMultiByte ( CP_THREAD_ACP, 0, pW->f, len, pA->f, len, NULL, NULL )
#define COPYN(f) pA->f = pW->f

  COPYN(lfHeight);
  COPYN(lfWidth);
  COPYN(lfEscapement);
  COPYN(lfOrientation);
  COPYN(lfWeight);
  COPYN(lfItalic);
  COPYN(lfUnderline);
  COPYN(lfStrikeOut);
  COPYN(lfCharSet);
  COPYN(lfOutPrecision);
  COPYN(lfClipPrecision);
  COPYN(lfQuality);
  COPYN(lfPitchAndFamily);
  COPYS(lfFaceName,LF_FACESIZE);

#undef COPYN
#undef COPYS
}
