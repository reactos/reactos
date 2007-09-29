//========================================================================
//
// CharCodeToUnicode.cc
//
// Copyright 2001-2003 Glyph & Cog, LLC
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdio.h>
#include <string.h>
#include "goo/gmem.h"
#include "goo/gfile.h"
#include "goo/GooString.h"
#include "Error.h"
#include "GlobalParams.h"
#include "PSTokenizer.h"
#include "CharCodeToUnicode.h"

//------------------------------------------------------------------------

#define maxUnicodeString 8

struct CharCodeToUnicodeString {
  CharCode c;
  Unicode u[maxUnicodeString];
  int len;
};

//------------------------------------------------------------------------

static int getCharFromString(void *data) {
  char *p;
  int c;

  p = *(char **)data;
  if (*p) {
    c = *p++;
    *(char **)data = p;
  } else {
    c = EOF;
  }
  return c;
}

static int getCharFromFile(void *data) {
  return fgetc((FILE *)data);
}

//------------------------------------------------------------------------

CharCodeToUnicode *CharCodeToUnicode::parseCIDToUnicode(GooString *fileName,
							GooString *collection) {
  FILE *f;
  Unicode *mapA;
  CharCode size, mapLenA;
  char buf[64];
  Unicode u;
  CharCodeToUnicode *ctu;

  if (!(f = fopen(fileName->getCString(), "r"))) {
    error(-1, "Couldn't open cidToUnicode file '%s'",
	  fileName->getCString());
    return NULL;
  }

  size = 32768;
  mapA = (Unicode *)gmallocn(size, sizeof(Unicode));
  mapLenA = 0;

  while (getLine(buf, sizeof(buf), f)) {
    if (mapLenA == size) {
      size *= 2;
      mapA = (Unicode *)greallocn(mapA, size, sizeof(Unicode));
    }
    if (sscanf(buf, "%x", &u) == 1) {
      mapA[mapLenA] = u;
    } else {
      error(-1, "Bad line (%d) in cidToUnicode file '%s'",
	    (int)(mapLenA + 1), fileName->getCString());
      mapA[mapLenA] = 0;
    }
    ++mapLenA;
  }
  fclose(f);

  ctu = new CharCodeToUnicode(collection->copy(), mapA, mapLenA, gTrue,
			      NULL, 0, 0);
  gfree(mapA);
  return ctu;
}

CharCodeToUnicode *CharCodeToUnicode::parseUnicodeToUnicode(
						    GooString *fileName) {
  FILE *f;
  Unicode *mapA;
  CharCodeToUnicodeString *sMapA;
  CharCode size, oldSize, len, sMapSizeA, sMapLenA;
  char buf[256];
  char *tok;
  Unicode u0;
  Unicode uBuf[maxUnicodeString];
  CharCodeToUnicode *ctu;
  int line, n, i;

  if (!(f = fopen(fileName->getCString(), "r"))) {
    error(-1, "Couldn't open unicodeToUnicode file '%s'",
	  fileName->getCString());
    return NULL;
  }

  size = 4096;
  mapA = (Unicode *)gmallocn(size, sizeof(Unicode));
  memset(mapA, 0, size * sizeof(Unicode));
  len = 0;
  sMapA = NULL;
  sMapSizeA = sMapLenA = 0;

  line = 0;
  while (getLine(buf, sizeof(buf), f)) {
    ++line;
    if (!(tok = strtok(buf, " \t\r\n")) ||
	sscanf(tok, "%x", &u0) != 1) {
      error(-1, "Bad line (%d) in unicodeToUnicode file '%s'",
	    line, fileName->getCString());
      continue;
    }
    n = 0;
    while (n < maxUnicodeString) {
      if (!(tok = strtok(NULL, " \t\r\n"))) {
	break;
      }
      if (sscanf(tok, "%x", &uBuf[n]) != 1) {
	error(-1, "Bad line (%d) in unicodeToUnicode file '%s'",
	      line, fileName->getCString());
	break;
      }
      ++n;
    }
    if (n < 1) {
      error(-1, "Bad line (%d) in unicodeToUnicode file '%s'",
	    line, fileName->getCString());
      continue;
    }
    if (u0 >= size) {
      oldSize = size;
      while (u0 >= size) {
	size *= 2;
      }
      mapA = (Unicode *)greallocn(mapA, size, sizeof(Unicode));
      memset(mapA + oldSize, 0, (size - oldSize) * sizeof(Unicode));
    }
    if (n == 1) {
      mapA[u0] = uBuf[0];
    } else {
      mapA[u0] = 0;
      if (sMapLenA == sMapSizeA) {
	sMapSizeA += 16;
	sMapA = (CharCodeToUnicodeString *)
	          greallocn(sMapA, sMapSizeA, sizeof(CharCodeToUnicodeString));
      }
      sMapA[sMapLenA].c = u0;
      for (i = 0; i < n; ++i) {
	sMapA[sMapLenA].u[i] = uBuf[i];
      }
      sMapA[sMapLenA].len = n;
      ++sMapLenA;
    }
    if (u0 >= len) {
      len = u0 + 1;
    }
  }
  fclose(f);

  ctu = new CharCodeToUnicode(fileName->copy(), mapA, len, gTrue,
			      sMapA, sMapLenA, sMapSizeA);
  gfree(mapA);
  return ctu;
}

CharCodeToUnicode *CharCodeToUnicode::make8BitToUnicode(Unicode *toUnicode) {
  return new CharCodeToUnicode(NULL, toUnicode, 256, gTrue, NULL, 0, 0);
}

CharCodeToUnicode *CharCodeToUnicode::parseCMap(GooString *buf, int nBits) {
  CharCodeToUnicode *ctu;
  char *p;

  ctu = new CharCodeToUnicode(NULL);
  p = buf->getCString();
  ctu->parseCMap1(&getCharFromString, &p, nBits);
  return ctu;
}

void CharCodeToUnicode::mergeCMap(GooString *buf, int nBits) {
  char *p;

  p = buf->getCString();
  parseCMap1(&getCharFromString, &p, nBits);
}

void CharCodeToUnicode::parseCMap1(int (*getCharFunc)(void *), void *data,
				   int nBits) {
  PSTokenizer *pst;
  char tok1[256], tok2[256], tok3[256];
  int nDigits, n1, n2, n3;
  CharCode i;
  CharCode code1, code2;
  GooString *name;
  FILE *f;

  nDigits = nBits / 4;
  pst = new PSTokenizer(getCharFunc, data);
  pst->getToken(tok1, sizeof(tok1), &n1);
  while (pst->getToken(tok2, sizeof(tok2), &n2)) {
    if (!strcmp(tok2, "usecmap")) {
      if (tok1[0] == '/') {
	name = new GooString(tok1 + 1);
	if ((f = globalParams->findToUnicodeFile(name))) {
	  parseCMap1(&getCharFromFile, f, nBits);
	  fclose(f);
	} else {
	  error(-1, "Couldn't find ToUnicode CMap file for '%s'",
		name->getCString());
	}
	delete name;
      }
      pst->getToken(tok1, sizeof(tok1), &n1);
    } else if (!strcmp(tok2, "beginbfchar")) {
      while (pst->getToken(tok1, sizeof(tok1), &n1)) {
	if (!strcmp(tok1, "endbfchar")) {
	  break;
	}
	if (!pst->getToken(tok2, sizeof(tok2), &n2) ||
	    !strcmp(tok2, "endbfchar")) {
	  error(-1, "Illegal entry in bfchar block in ToUnicode CMap");
	  break;
	}
	if (!(n1 == 2 + nDigits && tok1[0] == '<' && tok1[n1 - 1] == '>' &&
	      tok2[0] == '<' && tok2[n2 - 1] == '>')) {
	  
	  // check there was no line jump inside the token and so the length is 
	  // longer than it should be
	  int countAux = 0;
	  for (int k = 0; k < n1; k++)
	    if (tok1[k] != '\n' && tok1[k] != '\r') countAux++;
	
	  if (!(countAux == 2 + nDigits && tok1[0] == '<' && tok1[n1 - 1] == '>' &&
	      tok2[0] == '<' && tok2[n2 - 1] == '>')) {
	    error(-1, "Illegal entry in bfchar block in ToUnicode CMap");
	    continue;
	  }
	}
	tok1[n1 - 1] = tok2[n2 - 1] = '\0';
	if (sscanf(tok1 + 1, "%x", &code1) != 1) {
	  error(-1, "Illegal entry in bfchar block in ToUnicode CMap");
	  continue;
	}
	addMapping(code1, tok2 + 1, n2 - 2, 0);
      }
      pst->getToken(tok1, sizeof(tok1), &n1);
    } else if (!strcmp(tok2, "beginbfrange")) {
      while (pst->getToken(tok1, sizeof(tok1), &n1)) {
	if (!strcmp(tok1, "endbfrange")) {
	  break;
	}
	if (!pst->getToken(tok2, sizeof(tok2), &n2) ||
	    !strcmp(tok2, "endbfrange") ||
	    !pst->getToken(tok3, sizeof(tok3), &n3) ||
	    !strcmp(tok3, "endbfrange")) {
	  error(-1, "Illegal entry in bfrange block in ToUnicode CMap");
	  break;
	}
	if (!(n1 == 2 + nDigits && tok1[0] == '<' && tok1[n1 - 1] == '>' &&
	      n2 == 2 + nDigits && tok2[0] == '<' && tok2[n2 - 1] == '>')) {
	  // check there was no line jump inside the token and so the length is 
	  // longer than it should be
	  int countAux = 0;
	  for (int k = 0; k < n1; k++)
	    if (tok1[k] != '\n' && tok1[k] != '\r') countAux++;
	  
	  int countAux2 = 0;
	  for (int k = 0; k < n1; k++)
	    if (tok2[k] != '\n' && tok2[k] != '\r') countAux++;
	  
	  if (!(countAux == 2 + nDigits && tok1[0] == '<' && tok1[n1 - 1] == '>' &&
	      countAux2 == 2 + nDigits && tok2[0] == '<' && tok2[n2 - 1] == '>')) {
	    error(-1, "Illegal entry in bfrange block in ToUnicode CMap");
	    continue;
	  }
	}
	tok1[n1 - 1] = tok2[n2 - 1] = '\0';
	if (sscanf(tok1 + 1, "%x", &code1) != 1 ||
	    sscanf(tok2 + 1, "%x", &code2) != 1) {
	  error(-1, "Illegal entry in bfrange block in ToUnicode CMap");
	  continue;
	}
	if (!strcmp(tok3, "[")) {
	  i = 0;
	  while (pst->getToken(tok1, sizeof(tok1), &n1) &&
		 code1 + i <= code2) {
	    if (!strcmp(tok1, "]")) {
	      break;
	    }
	    if (tok1[0] == '<' && tok1[n1 - 1] == '>') {
	      tok1[n1 - 1] = '\0';
	      addMapping(code1 + i, tok1 + 1, n1 - 2, 0);
	    } else {
	      error(-1, "Illegal entry in bfrange block in ToUnicode CMap");
	    }
	    ++i;
	  }
	} else if (tok3[0] == '<' && tok3[n3 - 1] == '>') {
	  tok3[n3 - 1] = '\0';
	  for (i = 0; code1 <= code2; ++code1, ++i) {
	    addMapping(code1, tok3 + 1, n3 - 2, i);
	  }

	} else {
	  error(-1, "Illegal entry in bfrange block in ToUnicode CMap");
	}
      }
      pst->getToken(tok1, sizeof(tok1), &n1);
    } else {
      strcpy(tok1, tok2);
    }
  }
  delete pst;
}

void CharCodeToUnicode::addMapping(CharCode code, char *uStr, int n,
				   int offset) {
  CharCode oldLen, i;
  Unicode u;
  char uHex[5];
  int j;

  if (code >= mapLen) {
    oldLen = mapLen;
    mapLen = (code + 256) & ~255;
    map = (Unicode *)greallocn(map, mapLen, sizeof(Unicode));
    for (i = oldLen; i < mapLen; ++i) {
      map[i] = 0;
    }
  }
  if (n <= 4) {
    if (sscanf(uStr, "%x", &u) != 1) {
      error(-1, "Illegal entry in ToUnicode CMap");
      return;
    }
    map[code] = u + offset;
  } else {
    if (sMapLen >= sMapSize) {
      sMapSize = sMapSize + 16;
      sMap = (CharCodeToUnicodeString *)
	       greallocn(sMap, sMapSize, sizeof(CharCodeToUnicodeString));
    }
    map[code] = 0;
    sMap[sMapLen].c = code;
    sMap[sMapLen].len = n / 4;
    for (j = 0; j < sMap[sMapLen].len && j < maxUnicodeString; ++j) {
      strncpy(uHex, uStr + j*4, 4);
      uHex[4] = '\0';
      if (sscanf(uHex, "%x", &sMap[sMapLen].u[j]) != 1) {
	error(-1, "Illegal entry in ToUnicode CMap");
      }
    }
    sMap[sMapLen].u[sMap[sMapLen].len - 1] += offset;
    ++sMapLen;
  }
}

CharCodeToUnicode::CharCodeToUnicode(GooString *tagA) {
  CharCode i;

  tag = tagA;
  mapLen = 256;
  map = (Unicode *)gmallocn(mapLen, sizeof(Unicode));
  for (i = 0; i < mapLen; ++i) {
    map[i] = 0;
  }
  sMap = NULL;
  sMapLen = sMapSize = 0;
  refCnt = 1;
#if MULTITHREADED
  gInitMutex(&mutex);
#endif
}

CharCodeToUnicode::CharCodeToUnicode(GooString *tagA, Unicode *mapA,
				     CharCode mapLenA, GBool copyMap,
				     CharCodeToUnicodeString *sMapA,
				     int sMapLenA, int sMapSizeA) {
  tag = tagA;
  mapLen = mapLenA;
  if (copyMap) {
    map = (Unicode *)gmallocn(mapLen, sizeof(Unicode));
    memcpy(map, mapA, mapLen * sizeof(Unicode));
  } else {
    map = mapA;
  }
  sMap = sMapA;
  sMapLen = sMapLenA;
  sMapSize = sMapSizeA;
  refCnt = 1;
#if MULTITHREADED
  gInitMutex(&mutex);
#endif
}

CharCodeToUnicode::~CharCodeToUnicode() {
  if (tag) {
    delete tag;
  }
  gfree(map);
  if (sMap) {
    gfree(sMap);
  }
#if MULTITHREADED
  gDestroyMutex(&mutex);
#endif
}

void CharCodeToUnicode::incRefCnt() {
#if MULTITHREADED
  gLockMutex(&mutex);
#endif
  ++refCnt;
#if MULTITHREADED
  gUnlockMutex(&mutex);
#endif
}

void CharCodeToUnicode::decRefCnt() {
  GBool done;

#if MULTITHREADED
  gLockMutex(&mutex);
#endif
  done = --refCnt == 0;
#if MULTITHREADED
  gUnlockMutex(&mutex);
#endif
  if (done) {
    delete this;
  }
}

GBool CharCodeToUnicode::match(GooString *tagA) {
  return tag && !tag->cmp(tagA);
}

void CharCodeToUnicode::setMapping(CharCode c, Unicode *u, int len) {
  int i, j;

  if (len == 1) {
    map[c] = u[0];
  } else {
    for (i = 0; i < sMapLen; ++i) {
      if (sMap[i].c == c) {
	break;
      }
    }
    if (i == sMapLen) {
      if (sMapLen == sMapSize) {
	sMapSize += 8;
	sMap = (CharCodeToUnicodeString *)
	         greallocn(sMap, sMapSize, sizeof(CharCodeToUnicodeString));
      }
      ++sMapLen;
    }
    map[c] = 0;
    sMap[i].c = c;
    sMap[i].len = len;
    for (j = 0; j < len && j < maxUnicodeString; ++j) {
      sMap[i].u[j] = u[j];
    }
  }
}

int CharCodeToUnicode::mapToUnicode(CharCode c, Unicode *u, int size) {
  int i, j;

  if (c >= mapLen) {
    return 0;
  }
  if (map[c]) {
    u[0] = map[c];
    return 1;
  }
  for (i = 0; i < sMapLen; ++i) {
    if (sMap[i].c == c) {
      for (j = 0; j < sMap[i].len && j < size; ++j) {
	u[j] = sMap[i].u[j];
      }
      return j;
    }
  }
  return 0;
}

//------------------------------------------------------------------------

CharCodeToUnicodeCache::CharCodeToUnicodeCache(int sizeA) {
  int i;

  size = sizeA;
  cache = (CharCodeToUnicode **)gmallocn(size, sizeof(CharCodeToUnicode *));
  for (i = 0; i < size; ++i) {
    cache[i] = NULL;
  }
}

CharCodeToUnicodeCache::~CharCodeToUnicodeCache() {
  int i;

  for (i = 0; i < size; ++i) {
    if (cache[i]) {
      cache[i]->decRefCnt();
    }
  }
  gfree(cache);
}

CharCodeToUnicode *CharCodeToUnicodeCache::getCharCodeToUnicode(GooString *tag) {
  CharCodeToUnicode *ctu;
  int i, j;

  if (cache[0] && cache[0]->match(tag)) {
    cache[0]->incRefCnt();
    return cache[0];
  }
  for (i = 1; i < size; ++i) {
    if (cache[i] && cache[i]->match(tag)) {
      ctu = cache[i];
      for (j = i; j >= 1; --j) {
	cache[j] = cache[j - 1];
      }
      cache[0] = ctu;
      ctu->incRefCnt();
      return ctu;
    }
  }
  return NULL;
}

void CharCodeToUnicodeCache::add(CharCodeToUnicode *ctu) {
  int i;

  if (cache[size - 1]) {
    cache[size - 1]->decRefCnt();
  }
  for (i = size - 1; i >= 1; --i) {
    cache[i] = cache[i - 1];
  }
  cache[0] = ctu;
  ctu->incRefCnt();
}
