//========================================================================
//
// Parser.h
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef PARSER_H
#define PARSER_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "Lexer.h"

//------------------------------------------------------------------------
// Parser
//------------------------------------------------------------------------

class Parser {
public:

  Parser(XRef *xrefA, Lexer *lexerA);

  ~Parser();

  // Get the next object from the input stream.
  Object *getObj(Object *obj,
                 Guchar *fileKey = NULL, int keyLength = 0,
                 int objNum = 0, int objGen = 0);

  // Get stream.
  Stream *getStream() { return lexer->getStream(); }

  // Get current position in file.
  int getPos() { return lexer->getPos(); }

private:

  XRef *xref;           // the xref table for this PDF file
  Lexer *lexer;         // input stream
  Object buf1, buf2;    // next two tokens
  int inlineImg;        // set when inline image data is encountered

  Stream *makeStream(Object *dict);
  void shift(int objNum = -1);
};

#endif

