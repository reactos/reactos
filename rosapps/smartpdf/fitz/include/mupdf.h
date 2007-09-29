#ifdef _MUPDF_H_
#error "mupdf.h must only be included once"
#endif
#define _MUPDF_H_

#ifndef _FITZ_BASE_H_
#error "fitz-base.h must be included before mupdf.h"
#endif

#ifndef _FITZ_STREAM_H_
#error "fitz-stream.h must be included before mupdf.h"
#endif

#ifndef _FITZ_WORLD_H_
#error "fitz-world.h must be included before mupdf.h"
#endif

#ifdef  __cplusplus
extern "C" {
#endif

void pdf_logxref(char *fmt, ...);
void pdf_logrsrc(char *fmt, ...);
void pdf_logfont(char *fmt, ...);
void pdf_logimage(char *fmt, ...);
void pdf_logshade(char *fmt, ...);
void pdf_logpage(char *fmt, ...);

#include "mupdf/syntax.h"
#include "mupdf/xref.h"
#include "mupdf/rsrc.h"
#include "mupdf/content.h"
#include "mupdf/annot.h"
#include "mupdf/page.h"

#ifdef  __cplusplus
}
#endif

