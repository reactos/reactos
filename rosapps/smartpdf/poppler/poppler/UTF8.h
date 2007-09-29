//========================================================================
//
// UTF8.h
//
// Copyright 2001-2003 Glyph & Cog, LLC
//
//========================================================================

static int mapUTF8(Unicode u, char *buf, int bufSize) {
  if        (u <= 0x0000007f) {
    if (bufSize < 1) {
      return 0;
    }
    buf[0] = (char)u;
    return 1;
  } else if (u <= 0x000007ff) {
    if (bufSize < 2) {
      return 0;
    }
    buf[0] = (char)(0xc0 + (u >> 6));
    buf[1] = (char)(0x80 + (u & 0x3f));
    return 2;
  } else if (u <= 0x0000ffff) {
    if (bufSize < 3) {
      return 0;
    }
    buf[0] = (char)(0xe0 + (u >> 12));
    buf[1] = (char)(0x80 + ((u >> 6) & 0x3f));
    buf[2] = (char)(0x80 + (u & 0x3f));
    return 3;
  } else if (u <= 0x0010ffff) {
    if (bufSize < 4) {
      return 0;
    }
    buf[0] = (char)(0xf0 + (u >> 18));
    buf[1] = (char)(0x80 + ((u >> 12) & 0x3f));
    buf[2] = (char)(0x80 + ((u >> 6) & 0x3f));
    buf[3] = (char)(0x80 + (u & 0x3f));
    return 4;
  } else {
    return 0;
  }
}

static int mapUCS2(Unicode u, char *buf, int bufSize) {
  if (u <= 0xffff) {
    if (bufSize < 2) {
      return 0;
    }
    buf[0] = (char)((u >> 8) & 0xff);
    buf[1] = (char)(u & 0xff);
    return 2;
  } else {
    return 0;
  }
}
