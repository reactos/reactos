//========================================================================
//
// DCTStream.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#include "DCTStream.h"

static void str_init_source(j_decompress_ptr cinfo)
{
}

static boolean str_fill_input_buffer(j_decompress_ptr cinfo)
{
  int c;
  struct str_src_mgr * src = (struct str_src_mgr *)cinfo->src;
  if (src->index == 0) {
    c = 0xFF;
    src->index++;
  }
  else if (src->index == 1) {
    c = 0xD8;
    src->index++;
  }
  else c = src->str->getChar();
  if (c != EOF)
  {
    src->buffer = c;
    src->pub.next_input_byte = &src->buffer;
    src->pub.bytes_in_buffer = 1;
    return TRUE;
  }
  else return FALSE;
}

static void str_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
  struct str_src_mgr * src = (struct str_src_mgr *)cinfo->src;
  if (num_bytes > 0) {
    while (num_bytes > (long) src->pub.bytes_in_buffer) {
      num_bytes -= (long) src->pub.bytes_in_buffer;
      str_fill_input_buffer(cinfo);
    }
    src->pub.next_input_byte += (size_t) num_bytes;
    src->pub.bytes_in_buffer -= (size_t) num_bytes;
  }
}

static void str_term_source(j_decompress_ptr cinfo)
{
}

DCTStream::DCTStream(Stream *strA):
  FilterStream(strA) {
  init();
}

DCTStream::~DCTStream() {
  jpeg_destroy_decompress(&cinfo);
  delete str;
}

void DCTStream::init()
{
  jpeg_create_decompress(&cinfo);
  src.pub.init_source = str_init_source;
  src.pub.fill_input_buffer = str_fill_input_buffer;
  src.pub.skip_input_data = str_skip_input_data;
  src.pub.resync_to_restart = jpeg_resync_to_restart;
  src.pub.term_source = str_term_source;
  src.pub.bytes_in_buffer = 0;
  src.pub.next_input_byte = NULL;
  src.str = str;
  src.index = 0;
  cinfo.src = (jpeg_source_mgr *)&src;
  cinfo.err = jpeg_std_error(&jerr);
  x = 0;
  row_buffer = NULL;
}

void DCTStream::reset() {
  int row_stride;

  str->reset();

  if (row_buffer)
  {
    jpeg_destroy_decompress(&cinfo);
    init();
  }

  // JPEG data has to start with 0xFF 0xD8
  // but some pdf like the one on 
  // https://bugs.freedesktop.org/show_bug.cgi?id=3299
  // does have some garbage before that this seeks for
  // the start marker...
  bool startFound = false;
  int c = 0, c2 = 0;
  while (!startFound)
  {
    if (!c)
    {
      c = str->getChar();
      if (c == -1)
      {
        error(-1, "Could not find start of jpeg data");
        exit(1);
      }
      if (c != 0xFF) c = 0;
    }
    else
    {
      c2 = str->getChar();
      if (c2 != 0xD8)
      {
        c = 0;
        c2 = 0;
      }
      else startFound = true;
    }
  }

  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  row_stride = cinfo.output_width * cinfo.output_components;
  row_buffer = cinfo.mem->alloc_sarray((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
}

int DCTStream::getChar() {
  int c;

  if (x == 0) {
    if (cinfo.output_scanline < cinfo.output_height)
    {
      if (!jpeg_read_scanlines(&cinfo, row_buffer, 1)) return EOF;
    }
    else return EOF;
  }
  c = row_buffer[0][x];
  x++;
  if (x == cinfo.output_width * cinfo.output_components)
    x = 0;
  return c;
}

int DCTStream::lookChar() {
  int c;
  c = row_buffer[0][x];
  return c;
}

GooString *DCTStream::getPSFilter(int psLevel, char *indent) {
  GooString *s;

  if (psLevel < 2) {
    return NULL;
  }
  if (!(s = str->getPSFilter(psLevel, indent))) {
    return NULL;
  }
  s->append(indent)->append("<< >> /DCTDecode filter\n");
  return s;
}

GBool DCTStream::isBinary(GBool last) {
  return str->isBinary(gTrue);
}
