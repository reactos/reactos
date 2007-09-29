//========================================================================
//
// FoFiType1.cc
//
// Copyright 1999-2003 Glyph & Cog, LLC
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdlib.h>
#include <string.h>
#include "goo/gmem.h"
#include "FoFiEncodings.h"
#include "FoFiType1.h"

//------------------------------------------------------------------------
// FoFiType1
//------------------------------------------------------------------------

FoFiType1 *FoFiType1::make(char *fileA, int lenA) {
  return new FoFiType1(fileA, lenA, gFalse);
}

FoFiType1 *FoFiType1::load(char *fileName) {
  char *fileA;
  int lenA;

  if (!(fileA = FoFiBase::readFile(fileName, &lenA))) {
    return NULL;
  }
  return new FoFiType1(fileA, lenA, gTrue);
}

FoFiType1::FoFiType1(char *fileA, int lenA, GBool freeFileDataA):
  FoFiBase(fileA, lenA, freeFileDataA)
{
  name = NULL;
  encoding = NULL;
  parsed = gFalse;
}

FoFiType1::~FoFiType1() {
  int i;

  if (name) {
    gfree(name);
  }
  if (encoding && encoding != fofiType1StandardEncoding) {
    for (i = 0; i < 256; ++i) {
      gfree(encoding[i]);
    }
    gfree(encoding);
  }
}

char *FoFiType1::getName() {
  if (!parsed) {
    parse();
  }
  return name;
}

char **FoFiType1::getEncoding() {
  if (!parsed) {
    parse();
  }
  return encoding;
}

void FoFiType1::writeEncoded(char **newEncoding,
			     FoFiOutputFunc outputFunc, void *outputStream) {
  char buf[512];
  char *line;
  int i;

  // copy everything up to the encoding
  for (line = (char *)file;
       line && strncmp(line, "/Encoding", 9);
       line = getNextLine(line)) ;
  if (!line) {
    // no encoding - just copy the whole font file
    (*outputFunc)(outputStream, (char *)file, len);
    return;
  }
  (*outputFunc)(outputStream, (char *)file, line - (char *)file);

  // write the new encoding
  (*outputFunc)(outputStream, "/Encoding 256 array\n", 20);
  (*outputFunc)(outputStream,
		"0 1 255 {1 index exch /.notdef put} for\n", 40);
  for (i = 0; i < 256; ++i) {
    if (newEncoding[i]) {
      sprintf(buf, "dup %d /%s put\n", i, newEncoding[i]);
      (*outputFunc)(outputStream, buf, strlen(buf));
    }
  }
  (*outputFunc)(outputStream, "readonly def\n", 13);
  
  // copy everything after the encoding
  if (!strncmp(line, "/Encoding StandardEncoding def", 30)) {
    line = getNextLine(line);
  } else {
    for (line = getNextLine(line);
	 line && strncmp(line, "readonly def", 12);
	 line = getNextLine(line)) ;
    if (line) {
      line = getNextLine(line);
    }
  }
  if (line) {
    (*outputFunc)(outputStream, line, ((char *)file + len) - line);
  }
}

char *FoFiType1::getNextLine(char *line) {
  while (line < (char *)file + len && *line != '\x0a' && *line != '\x0d') {
    ++line;
  }
  if (line < (char *)file + len && *line == '\x0d') {
    ++line;
  }
  if (line < (char *)file + len && *line == '\x0a') {
    ++line;
  }
  if (line >= (char *)file + len) {
    return NULL;
  }
  return line;
}

void FoFiType1::parse() {
  char *line, *line1, *p, *p2;
  char buf[256];
  char c;
  int n, code, i, j;

  for (i = 1, line = (char *)file;
       i <= 100 && line && (!name || !encoding);
       ++i) {

    // get font name
    if (!name && !strncmp(line, "/FontName", 9)) {
      strncpy(buf, line, 255);
      buf[255] = '\0';
      if ((p = strchr(buf+9, '/')) &&
	  (p = strtok(p+1, " \t\n\r"))) {
	name = copyString(p);
      }
      line = getNextLine(line);

    // get encoding
    } else if (!encoding &&
	       !strncmp(line, "/Encoding StandardEncoding def", 30)) {
      encoding = fofiType1StandardEncoding;
    } else if (!encoding &&
	       !strncmp(line, "/Encoding 256 array", 19)) {
      encoding = (char **)gmallocn(256, sizeof(char *));
      for (j = 0; j < 256; ++j) {
	encoding[j] = NULL;
      }
      for (j = 0, line = getNextLine(line);
	   j < 300 && line && (line1 = getNextLine(line));
	   ++j, line = line1) {
	if ((n = line1 - line) > 255) {
	  n = 255;
	}
	strncpy(buf, line, n);
	buf[n] = '\0';
	for (p = buf; *p == ' ' || *p == '\t'; ++p) ;
	if (!strncmp(p, "dup", 3)) {
	  for (p += 3; *p == ' ' || *p == '\t'; ++p) ;
	  for (p2 = p; *p2 >= '0' && *p2 <= '9'; ++p2) ;
	  if (*p2) {
	    c = *p2;
	    *p2 = '\0';
	    if ((code = atoi(p)) < 256) {
	      *p2 = c;
	      for (p = p2; *p == ' ' || *p == '\t'; ++p) ;
	      if (*p == '/') {
		++p;
		for (p2 = p; *p2 && *p2 != ' ' && *p2 != '\t'; ++p2) ;
		*p2 = '\0';
		encoding[code] = copyString(p);
	      }
	    }
	  }
	} else {
	  if (strtok(buf, " \t") &&
	      (p = strtok(NULL, " \t\n\r")) && !strcmp(p, "def")) {
	    break;
	  }
	}
      }
      //~ check for getinterval/putinterval junk

    } else {
      line = getNextLine(line);
    }
  }

  parsed = gTrue;
}
