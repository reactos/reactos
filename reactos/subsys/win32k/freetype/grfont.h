#ifndef GRFONT_H
#define GRFONT_H

#include "graph.h"

  extern const unsigned char  font_8x8[];

  extern void grGotobitmap( grBitmap*  bitmap );
  extern void grSetMargin( int right, int top );
  extern void grGotoxy ( int x, int y );

  extern void grWrite  ( const char*  string );
  extern void grWriteln( const char* string );
  extern void grLn();

#endif /* GRFONT_H */
