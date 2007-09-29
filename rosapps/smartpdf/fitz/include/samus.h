#ifdef _SAMUS_H_
#error "samus.h must only be included once"
#endif
#define _SAMUS_H_

#ifndef _FITZ_BASE_H_
#error "fitz-base.h must be included before mupdf.h"
#endif

#ifndef _FITZ_STREAM_H_
#error "fitz-stream.h must be included before mupdf.h"
#endif

#ifndef _FITZ_WORLD_H_
#error "fitz-world.h must be included before mupdf.h"
#endif

#include "samus/misc.h"
#include "samus/zip.h"
#include "samus/xml.h"
#include "samus/pack.h"

#include "samus/names.h"
#include "samus/fixdoc.h"

