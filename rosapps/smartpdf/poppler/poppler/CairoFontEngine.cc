//========================================================================
//
// CairoFontEngine.cc
//
// Copyright 2003 Glyph & Cog, LLC
// Copyright 2004 Red Hat, Inc
//
//========================================================================

#include <config.h>

#include "config.h"
#include <string.h>
#include "CairoFontEngine.h"
#include "CharCodeToUnicode.h"
#include "GlobalParams.h"
#include <fofi/FoFiTrueType.h>
#include <fofi/FoFiType1C.h>
#include "goo/gfile.h"
#include "Error.h"

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

static void fileWrite(void *stream, char *data, int len) {
  fwrite(data, 1, len, (FILE *)stream);
}

//------------------------------------------------------------------------
// CairoFont
//------------------------------------------------------------------------

static void cairo_font_face_destroy (void *data)
{
  CairoFont *font = (CairoFont *) data;

  delete font;
}

CairoFont *CairoFont::create(GfxFont *gfxFont, XRef *xref, FT_Library lib, GBool useCIDs) {
  Ref embRef;
  Object refObj, strObj;
  GooString *tmpFileName, *fileName, *substName,*tmpFileName2;
  DisplayFontParam *dfp;
  FILE *tmpFile;
  int c, i, n, code, cmap;
  GfxFontType fontType;
  char **enc;
  char *name;
  FoFiTrueType *ff;
  FoFiType1C *ff1c;
  CharCodeToUnicode *ctu;
  Unicode uBuf[8];
  Ref ref;
  static cairo_user_data_key_t cairo_font_face_key;
  cairo_font_face_t *cairo_font_face;
  FT_Face face;

  Gushort *codeToGID;
  int codeToGIDLen;
  
  dfp = NULL;
  codeToGID = NULL;
  codeToGIDLen = 0;
  cairo_font_face = NULL;
  
  ref = *gfxFont->getID();
  fontType = gfxFont->getType();

  tmpFileName = NULL;

  if (gfxFont->getEmbeddedFontID(&embRef)) {
    if (!openTempFile(&tmpFileName, &tmpFile, "wb", NULL)) {
      error(-1, "Couldn't create temporary font file");
      goto err2;
    }
    
    refObj.initRef(embRef.num, embRef.gen);
    refObj.fetch(xref, &strObj);
    refObj.free();
    strObj.streamReset();
    while ((c = strObj.streamGetChar()) != EOF) {
      fputc(c, tmpFile);
    }
    strObj.streamClose();
    strObj.free();
    fclose(tmpFile);
    fileName = tmpFileName;
    
  } else if (!(fileName = gfxFont->getExtFontFile())) {
    // look for a display font mapping or a substitute font
    dfp = NULL;
    if (gfxFont->getName()) {
      dfp = globalParams->getDisplayFont(gfxFont);
    }
    if (!dfp) {
      error(-1, "Couldn't find a font for '%s'",
	    gfxFont->getName() ? gfxFont->getName()->getCString()
	    : "(unnamed)");
      goto err2;
    }
    switch (dfp->kind) {
    case displayFontT1:
      fileName = dfp->t1.fileName;
      fontType = gfxFont->isCIDFont() ? fontCIDType0 : fontType1;
      break;
    case displayFontTT:
      fileName = dfp->tt.fileName;
      fontType = gfxFont->isCIDFont() ? fontCIDType2 : fontTrueType;
      break;
    }
  }

  switch (fontType) {
  case fontType1:
  case fontType1C:
    if (FT_New_Face(lib, fileName->getCString(), 0, &face)) {
      error(-1, "could not create type1 face");
      goto err2;
    }
    
    enc = ((Gfx8BitFont *)gfxFont)->getEncoding();
    
    codeToGID = (Gushort *)gmallocn(256, sizeof(int));
    codeToGIDLen = 256;
    for (i = 0; i < 256; ++i) {
      codeToGID[i] = 0;
      if ((name = enc[i])) {
	codeToGID[i] = (Gushort)FT_Get_Name_Index(face, name);
      }
    }
    break;
    
  case fontCIDType2:
    codeToGID = NULL;
    n = 0;
    if (dfp) {
      // create a CID-to-GID mapping, via Unicode
      if ((ctu = ((GfxCIDFont *)gfxFont)->getToUnicode())) {
        if ((ff = FoFiTrueType::load(fileName->getCString()))) {
          // look for a Unicode cmap
          for (cmap = 0; cmap < ff->getNumCmaps(); ++cmap) {
            if ((ff->getCmapPlatform(cmap) == 3 &&
                 ff->getCmapEncoding(cmap) == 1) ||
                 ff->getCmapPlatform(cmap) == 0) {
              break;
            }
          }
          if (cmap < ff->getNumCmaps()) {
            // map CID -> Unicode -> GID
            n = ctu->getLength();
            codeToGID = (Gushort *)gmallocn(n, sizeof(Gushort));
            for (code = 0; code < n; ++code) {
              if (ctu->mapToUnicode(code, uBuf, 8) > 0) {
                  codeToGID[code] = ff->mapCodeToGID(cmap, uBuf[0]);
              } else {
                codeToGID[code] = 0;
              }
            }
          }
          delete ff;
        }
        ctu->decRefCnt();
      } else {
        error(-1, "Couldn't find a mapping to Unicode for font '%s'",
              gfxFont->getName() ? gfxFont->getName()->getCString()
                        : "(unnamed)");
	goto err2;
      }
    } else {
      if (((GfxCIDFont *)gfxFont)->getCIDToGID()) {
	n = ((GfxCIDFont *)gfxFont)->getCIDToGIDLen();
	codeToGID = (Gushort *)gmallocn(n, sizeof(Gushort));
	memcpy(codeToGID, ((GfxCIDFont *)gfxFont)->getCIDToGID(),
	       n * sizeof(Gushort));
      }
    }
    codeToGIDLen = n;
    /* Fall through */
  case fontTrueType:
    if (!(ff = FoFiTrueType::load(fileName->getCString()))) {
      error(-1, "failed to load truetype font\n");
      goto err2;
    }
    /* This might be set already for the CIDType2 case */
    if (fontType == fontTrueType) {
      codeToGID = ((Gfx8BitFont *)gfxFont)->getCodeToGIDMap(ff);
      codeToGIDLen = 256;
    }
    if (!openTempFile(&tmpFileName2, &tmpFile, "wb", NULL)) {
      delete ff;
      error(-1, "failed to open truetype tempfile\n");
      goto err2;
    }
    ff->writeTTF(&fileWrite, tmpFile);
    fclose(tmpFile);
    delete ff;

    if (FT_New_Face(lib, tmpFileName2->getCString(), 0, &face)) {
      error(-1, "could not create truetype face\n");
      goto err2;
    }
    unlink (tmpFileName2->getCString());
    delete tmpFileName2;
    break;
    
  case fontCIDType0:
  case fontCIDType0C:

    codeToGID = NULL;
    codeToGIDLen = 0;

    if (!useCIDs)
    {
      if ((ff1c = FoFiType1C::load(fileName->getCString()))) {
        codeToGID = ff1c->getCIDToGIDMap(&codeToGIDLen);
        delete ff1c;
      }
    }

    if (FT_New_Face(lib, fileName->getCString(), 0, &face)) {
      gfree(codeToGID);
      codeToGID = NULL;
      error(-1, "could not create cid face\n");
      goto err2;
    }
    break;
    
  default:
    printf ("font type not handled\n");
    goto err2;
    break;
  }

  // delete the (temporary) font file -- with Unix hard link
  // semantics, this will remove the last link; otherwise it will
  // return an error, leaving the file to be deleted later
  if (fileName == tmpFileName) {
    unlink (fileName->getCString());
    delete tmpFileName;
  }

  cairo_font_face = cairo_ft_font_face_create_for_ft_face (face,
							   FT_LOAD_NO_HINTING |
							   FT_LOAD_NO_BITMAP);
  if (cairo_font_face == NULL) {
    error(-1, "could not create cairo font\n");
    goto err2; /* this doesn't do anything, but it looks like we're
		* handling the error */
  } {
  CairoFont *ret = new CairoFont(ref, cairo_font_face, face, codeToGID, codeToGIDLen);
  cairo_font_face_set_user_data (cairo_font_face, 
				 &cairo_font_face_key,
				 ret,
				 cairo_font_face_destroy);

  return ret;
  }
 err2:
  /* hmm? */
  printf ("some font thing failed\n");
  return NULL;
}

CairoFont::CairoFont(Ref ref, cairo_font_face_t *cairo_font_face, FT_Face face,
    Gushort *codeToGID, int codeToGIDLen) : ref(ref), cairo_font_face(cairo_font_face),
					    face(face), codeToGID(codeToGID),
					    codeToGIDLen(codeToGIDLen) { }

CairoFont::~CairoFont() {
  FT_Done_Face (face);
  gfree(codeToGID);
}

GBool
CairoFont::matches(Ref &other) {
  return (other.num == ref.num && other.gen == ref.gen);
}

cairo_font_face_t *
CairoFont::getFontFace(void) {
  return cairo_font_face;
}

unsigned long
CairoFont::getGlyph(CharCode code,
		    Unicode *u, int uLen) {
  FT_UInt gid;

  if (codeToGID && code < codeToGIDLen) {
    gid = (FT_UInt)codeToGID[code];
  } else {
    gid = (FT_UInt)code;
  }
  return gid;
}

//------------------------------------------------------------------------
// CairoFontEngine
//------------------------------------------------------------------------

CairoFontEngine::CairoFontEngine(FT_Library libA) {
  int i;

  lib = libA;
  for (i = 0; i < cairoFontCacheSize; ++i) {
    fontCache[i] = NULL;
  }
  
  FT_Int major, minor, patch;
  // as of FT 2.1.8, CID fonts are indexed by CID instead of GID
  FT_Library_Version(lib, &major, &minor, &patch);
  useCIDs = major > 2 ||
            (major == 2 && (minor > 1 || (minor == 1 && patch > 7)));
}

CairoFontEngine::~CairoFontEngine() {
  int i;
  
  for (i = 0; i < cairoFontCacheSize; ++i) {
    if (fontCache[i])
      delete fontCache[i];
  }
}

CairoFont *
CairoFontEngine::getFont(GfxFont *gfxFont, XRef *xref) {
  int i, j;
  Ref ref;
  CairoFont *font;
  GfxFontType fontType;
  
  fontType = gfxFont->getType();
  if (fontType == fontType3) {
    /* Need to figure this out later */
    //    return NULL;
  }

  ref = *gfxFont->getID();

  for (i = 0; i < cairoFontCacheSize; ++i) {
    font = fontCache[i];
    if (font && font->matches(ref)) {
      for (j = i; j > 0; --j) {
	fontCache[j] = fontCache[j-1];
      }
      fontCache[0] = font;
      return font;
    }
  }
  
  font = CairoFont::create (gfxFont, xref, lib, useCIDs);
  //XXX: if font is null should we still insert it into the cache?
  if (fontCache[cairoFontCacheSize - 1]) {
    delete fontCache[cairoFontCacheSize - 1];
  }
  for (j = cairoFontCacheSize - 1; j > 0; --j) {
    fontCache[j] = fontCache[j-1];
  }
  fontCache[0] = font;
  return font;
}

