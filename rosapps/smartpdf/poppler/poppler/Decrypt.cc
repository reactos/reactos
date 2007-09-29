//========================================================================
//
// Decrypt.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <string.h>
#include "goo/gmem.h"
#include "Decrypt.h"

static void rc4InitKey(Guchar *key, int keyLen, Guchar *state);
static Guchar rc4DecryptByte(Guchar *state, Guchar *x, Guchar *y, Guchar c);
static void md5(Guchar *msg, int msgLen, Guchar *digest);

static Guchar passwordPad[32] = {
  0x28, 0xbf, 0x4e, 0x5e, 0x4e, 0x75, 0x8a, 0x41,
  0x64, 0x00, 0x4e, 0x56, 0xff, 0xfa, 0x01, 0x08, 
  0x2e, 0x2e, 0x00, 0xb6, 0xd0, 0x68, 0x3e, 0x80, 
  0x2f, 0x0c, 0xa9, 0xfe, 0x64, 0x53, 0x69, 0x7a
};

//------------------------------------------------------------------------
// Decrypt
//------------------------------------------------------------------------

Decrypt::Decrypt(Guchar *fileKey, int keyLength, int objNum, int objGen) {
  int i;

  // construct object key
  for (i = 0; i < keyLength; ++i) {
    objKey[i] = fileKey[i];
  }
  objKey[keyLength] = objNum & 0xff;
  objKey[keyLength + 1] = (objNum >> 8) & 0xff;
  objKey[keyLength + 2] = (objNum >> 16) & 0xff;
  objKey[keyLength + 3] = objGen & 0xff;
  objKey[keyLength + 4] = (objGen >> 8) & 0xff;
  md5(objKey, keyLength + 5, objKey);

  // set up for decryption
  x = y = 0;
  if ((objKeyLength = keyLength + 5) > 16) {
    objKeyLength = 16;
  }
  rc4InitKey(objKey, objKeyLength, state);
}

void Decrypt::reset() {
  x = y = 0;
  rc4InitKey(objKey, objKeyLength, state);
}

Guchar Decrypt::decryptByte(Guchar c) {
  return rc4DecryptByte(state, &x, &y, c);
}

GBool Decrypt::makeFileKey(int encVersion, int encRevision, int keyLength,
                           GooString *ownerKey, GooString *userKey,
                           int permissions, GooString *fileID,
                           GooString *ownerPassword, GooString *userPassword,
                           Guchar *fileKey, GBool encryptMetadata,
                           GBool *ownerPasswordOk) {
  Guchar test[32], test2[32];
  GooString *userPassword2;
  Guchar fState[256];
  Guchar tmpKey[16];
  Guchar fx, fy;
  int len, i, j;

  // try using the supplied owner password to generate the user password
  *ownerPasswordOk = gFalse;
  if (ownerPassword) {
    len = ownerPassword->getLength();
    if (len < 32) {
      memcpy(test, ownerPassword->getCString(), len);
      memcpy(test + len, passwordPad, 32 - len);
    } else {
      memcpy(test, ownerPassword->getCString(), 32);
    }
    md5(test, 32, test);
    if (encRevision == 3) {
      for (i = 0; i < 50; ++i) {
        md5(test, 16, test);
      }
    }
    if (encRevision == 2) {
      rc4InitKey(test, keyLength, fState);
      fx = fy = 0;
      for (i = 0; i < 32; ++i) {
        test2[i] = rc4DecryptByte(fState, &fx, &fy, ownerKey->getChar(i));
      }
    } else {
      memcpy(test2, ownerKey->getCString(), 32);
      for (i = 19; i >= 0; --i) {
        for (j = 0; j < keyLength; ++j) {
          tmpKey[j] = test[j] ^ i;
        }
        rc4InitKey(tmpKey, keyLength, fState);
        fx = fy = 0;
        for (j = 0; j < 32; ++j) {
          test2[j] = rc4DecryptByte(fState, &fx, &fy, test2[j]);
        }
      }
    }
    userPassword2 = new GooString((char *)test2, 32);
    if (makeFileKey2(encVersion, encRevision, keyLength, ownerKey, userKey,
             permissions, fileID, userPassword2, fileKey,
             encryptMetadata)) {
      *ownerPasswordOk = gTrue;
      delete userPassword2;
      return gTrue;
    }
    delete userPassword2;
  }

  // try using the supplied user password
  return makeFileKey2(encVersion, encRevision, keyLength, ownerKey, userKey,
                      permissions, fileID, userPassword, fileKey,
                      encryptMetadata);
}

GBool Decrypt::makeFileKey2(int encVersion, int encRevision, int keyLength,
                            GooString *ownerKey, GooString *userKey,
                            int permissions, GooString *fileID,
                            GooString *userPassword, Guchar *fileKey,
                            GBool encryptMetadata) {
  Guchar *buf;
  Guchar test[32];
  Guchar fState[256];
  Guchar tmpKey[16];
  Guchar fx, fy;
  int len, i, j;
  GBool ok;

  // generate file key
  buf = (Guchar *)gmalloc(72 + fileID->getLength());
  if (userPassword) {
    len = userPassword->getLength();
    if (len < 32) {
      memcpy(buf, userPassword->getCString(), len);
      memcpy(buf + len, passwordPad, 32 - len);
    } else {
      memcpy(buf, userPassword->getCString(), 32);
    }
  } else {
    memcpy(buf, passwordPad, 32);
  }
  memcpy(buf + 32, ownerKey->getCString(), 32);
  buf[64] = permissions & 0xff;
  buf[65] = (permissions >> 8) & 0xff;
  buf[66] = (permissions >> 16) & 0xff;
  buf[67] = (permissions >> 24) & 0xff;
  memcpy(buf + 68, fileID->getCString(), fileID->getLength());
  len = 68 + fileID->getLength();
  if (!encryptMetadata) {
    buf[len++] = 0xff;
    buf[len++] = 0xff;
    buf[len++] = 0xff;
    buf[len++] = 0xff;
  }
  md5(buf, len, fileKey);
  if (encRevision == 3) {
    for (i = 0; i < 50; ++i) {
      md5(fileKey, keyLength, fileKey);
    }
  }

  // test user password
  if (encRevision == 2) {
    rc4InitKey(fileKey, keyLength, fState);
    fx = fy = 0;
    for (i = 0; i < 32; ++i) {
      test[i] = rc4DecryptByte(fState, &fx, &fy, userKey->getChar(i));
    }
    ok = memcmp(test, passwordPad, 32) == 0;
  } else if (encRevision == 3) {
    memcpy(test, userKey->getCString(), 32);
    for (i = 19; i >= 0; --i) {
      for (j = 0; j < keyLength; ++j) {
        tmpKey[j] = fileKey[j] ^ i;
      }
      rc4InitKey(tmpKey, keyLength, fState);
      fx = fy = 0;
      for (j = 0; j < 32; ++j) {
        test[j] = rc4DecryptByte(fState, &fx, &fy, test[j]);
      }
    }
    memcpy(buf, passwordPad, 32);
    memcpy(buf + 32, fileID->getCString(), fileID->getLength());
    md5(buf, 32 + fileID->getLength(), buf);
    ok = memcmp(test, buf, 16) == 0;
  } else {
    ok = gFalse;
  }

  gfree(buf);
  return ok;
}

//------------------------------------------------------------------------
// RC4-compatible decryption
//------------------------------------------------------------------------

static void rc4InitKey(Guchar *key, int keyLen, Guchar *state) {
  Guchar index1, index2;
  Guchar t;
  int i;

  for (i = 0; i < 256; ++i)
    state[i] = i;
  index1 = index2 = 0;
  for (i = 0; i < 256; ++i) {
    index2 = (key[index1] + state[i] + index2) % 256;
    t = state[i];
    state[i] = state[index2];
    state[index2] = t;
    index1 = (index1 + 1) % keyLen;
  }
}

static Guchar rc4DecryptByte(Guchar *state, Guchar *x, Guchar *y, Guchar c) {
  Guchar x1, y1, tx, ty;

  x1 = *x = (*x + 1) % 256;
  y1 = *y = (state[*x] + *y) % 256;
  tx = state[x1];
  ty = state[y1];
  state[x1] = ty;
  state[y1] = tx;
  return c ^ state[(tx + ty) % 256];
}

//------------------------------------------------------------------------
// MD5 message digest
//------------------------------------------------------------------------

// this works around a bug in older Sun compilers
static inline Gulong rotateLeft(Gulong x, int r) {
  x &= 0xffffffff;
  return ((x << r) | (x >> (32 - r))) & 0xffffffff;
}

static inline Gulong md5Round1(Gulong a, Gulong b, Gulong c, Gulong d,
                               Gulong Xk,  Gulong s, Gulong Ti) {
  return b + rotateLeft((a + ((b & c) | (~b & d)) + Xk + Ti), s);
}

static inline Gulong md5Round2(Gulong a, Gulong b, Gulong c, Gulong d,
                               Gulong Xk,  Gulong s, Gulong Ti) {
  return b + rotateLeft((a + ((b & d) | (c & ~d)) + Xk + Ti), s);
}

static inline Gulong md5Round3(Gulong a, Gulong b, Gulong c, Gulong d,
                               Gulong Xk,  Gulong s, Gulong Ti) {
  return b + rotateLeft((a + (b ^ c ^ d) + Xk + Ti), s);
}

static inline Gulong md5Round4(Gulong a, Gulong b, Gulong c, Gulong d,
                               Gulong Xk,  Gulong s, Gulong Ti) {
  return b + rotateLeft((a + (c ^ (b | ~d)) + Xk + Ti), s);
}

static void md5(Guchar *msg, int msgLen, Guchar *digest) {
  Gulong x[16];
  Gulong a, b, c, d, aa, bb, cc, dd;
  int n64;
  int i, j, k;

  // compute number of 64-byte blocks
  // (length + pad byte (0x80) + 8 bytes for length)
  n64 = (msgLen + 1 + 8 + 63) / 64;

  // initialize a, b, c, d
  a = 0x67452301;
  b = 0xefcdab89;
  c = 0x98badcfe;
  d = 0x10325476;

  // loop through blocks
  k = 0;
  for (i = 0; i < n64; ++i) {

    // grab a 64-byte block
    for (j = 0; j < 16 && k < msgLen - 3; ++j, k += 4)
      x[j] = (((((msg[k+3] << 8) + msg[k+2]) << 8) + msg[k+1]) << 8) + msg[k];
    if (i == n64 - 1) {
      if (k == msgLen - 3)
        x[j] = 0x80000000 + (((msg[k+2] << 8) + msg[k+1]) << 8) + msg[k];
      else if (k == msgLen - 2)
        x[j] = 0x800000 + (msg[k+1] << 8) + msg[k];
      else if (k == msgLen - 1)
        x[j] = 0x8000 + msg[k];
      else
        x[j] = 0x80;
      ++j;
      while (j < 16)
        x[j++] = 0;
      x[14] = msgLen << 3;
    }

    // save a, b, c, d
    aa = a;
    bb = b;
    cc = c;
    dd = d;

    // round 1
    a = md5Round1(a, b, c, d, x[0],   7, 0xd76aa478);
    d = md5Round1(d, a, b, c, x[1],  12, 0xe8c7b756);
    c = md5Round1(c, d, a, b, x[2],  17, 0x242070db);
    b = md5Round1(b, c, d, a, x[3],  22, 0xc1bdceee);
    a = md5Round1(a, b, c, d, x[4],   7, 0xf57c0faf);
    d = md5Round1(d, a, b, c, x[5],  12, 0x4787c62a);
    c = md5Round1(c, d, a, b, x[6],  17, 0xa8304613);
    b = md5Round1(b, c, d, a, x[7],  22, 0xfd469501);
    a = md5Round1(a, b, c, d, x[8],   7, 0x698098d8);
    d = md5Round1(d, a, b, c, x[9],  12, 0x8b44f7af);
    c = md5Round1(c, d, a, b, x[10], 17, 0xffff5bb1);
    b = md5Round1(b, c, d, a, x[11], 22, 0x895cd7be);
    a = md5Round1(a, b, c, d, x[12],  7, 0x6b901122);
    d = md5Round1(d, a, b, c, x[13], 12, 0xfd987193);
    c = md5Round1(c, d, a, b, x[14], 17, 0xa679438e);
    b = md5Round1(b, c, d, a, x[15], 22, 0x49b40821);

    // round 2
    a = md5Round2(a, b, c, d, x[1],   5, 0xf61e2562);
    d = md5Round2(d, a, b, c, x[6],   9, 0xc040b340);
    c = md5Round2(c, d, a, b, x[11], 14, 0x265e5a51);
    b = md5Round2(b, c, d, a, x[0],  20, 0xe9b6c7aa);
    a = md5Round2(a, b, c, d, x[5],   5, 0xd62f105d);
    d = md5Round2(d, a, b, c, x[10],  9, 0x02441453);
    c = md5Round2(c, d, a, b, x[15], 14, 0xd8a1e681);
    b = md5Round2(b, c, d, a, x[4],  20, 0xe7d3fbc8);
    a = md5Round2(a, b, c, d, x[9],   5, 0x21e1cde6);
    d = md5Round2(d, a, b, c, x[14],  9, 0xc33707d6);
    c = md5Round2(c, d, a, b, x[3],  14, 0xf4d50d87);
    b = md5Round2(b, c, d, a, x[8],  20, 0x455a14ed);
    a = md5Round2(a, b, c, d, x[13],  5, 0xa9e3e905);
    d = md5Round2(d, a, b, c, x[2],   9, 0xfcefa3f8);
    c = md5Round2(c, d, a, b, x[7],  14, 0x676f02d9);
    b = md5Round2(b, c, d, a, x[12], 20, 0x8d2a4c8a);

    // round 3
    a = md5Round3(a, b, c, d, x[5],   4, 0xfffa3942);
    d = md5Round3(d, a, b, c, x[8],  11, 0x8771f681);
    c = md5Round3(c, d, a, b, x[11], 16, 0x6d9d6122);
    b = md5Round3(b, c, d, a, x[14], 23, 0xfde5380c);
    a = md5Round3(a, b, c, d, x[1],   4, 0xa4beea44);
    d = md5Round3(d, a, b, c, x[4],  11, 0x4bdecfa9);
    c = md5Round3(c, d, a, b, x[7],  16, 0xf6bb4b60);
    b = md5Round3(b, c, d, a, x[10], 23, 0xbebfbc70);
    a = md5Round3(a, b, c, d, x[13],  4, 0x289b7ec6);
    d = md5Round3(d, a, b, c, x[0],  11, 0xeaa127fa);
    c = md5Round3(c, d, a, b, x[3],  16, 0xd4ef3085);
    b = md5Round3(b, c, d, a, x[6],  23, 0x04881d05);
    a = md5Round3(a, b, c, d, x[9],   4, 0xd9d4d039);
    d = md5Round3(d, a, b, c, x[12], 11, 0xe6db99e5);
    c = md5Round3(c, d, a, b, x[15], 16, 0x1fa27cf8);
    b = md5Round3(b, c, d, a, x[2],  23, 0xc4ac5665);

    // round 4
    a = md5Round4(a, b, c, d, x[0],   6, 0xf4292244);
    d = md5Round4(d, a, b, c, x[7],  10, 0x432aff97);
    c = md5Round4(c, d, a, b, x[14], 15, 0xab9423a7);
    b = md5Round4(b, c, d, a, x[5],  21, 0xfc93a039);
    a = md5Round4(a, b, c, d, x[12],  6, 0x655b59c3);
    d = md5Round4(d, a, b, c, x[3],  10, 0x8f0ccc92);
    c = md5Round4(c, d, a, b, x[10], 15, 0xffeff47d);
    b = md5Round4(b, c, d, a, x[1],  21, 0x85845dd1);
    a = md5Round4(a, b, c, d, x[8],   6, 0x6fa87e4f);
    d = md5Round4(d, a, b, c, x[15], 10, 0xfe2ce6e0);
    c = md5Round4(c, d, a, b, x[6],  15, 0xa3014314);
    b = md5Round4(b, c, d, a, x[13], 21, 0x4e0811a1);
    a = md5Round4(a, b, c, d, x[4],   6, 0xf7537e82);
    d = md5Round4(d, a, b, c, x[11], 10, 0xbd3af235);
    c = md5Round4(c, d, a, b, x[2],  15, 0x2ad7d2bb);
    b = md5Round4(b, c, d, a, x[9],  21, 0xeb86d391);

    // increment a, b, c, d
    a += aa;
    b += bb;
    c += cc;
    d += dd;
  }

  // break digest into bytes
  digest[0] = (Guchar)(a & 0xff);
  digest[1] = (Guchar)((a >>= 8) & 0xff);
  digest[2] = (Guchar)((a >>= 8) & 0xff);
  digest[3] = (Guchar)((a >>= 8) & 0xff);
  digest[4] = (Guchar)(b & 0xff);
  digest[5] = (Guchar)((b >>= 8) & 0xff);
  digest[6] = (Guchar)((b >>= 8) & 0xff);
  digest[7] = (Guchar)((b >>= 8) & 0xff);
  digest[8] = (Guchar)(c & 0xff);
  digest[9] = (Guchar)((c >>= 8) & 0xff);
  digest[10] = (Guchar)((c >>= 8) & 0xff);
  digest[11] = (Guchar)((c >>= 8) & 0xff);
  digest[12] = (Guchar)(d & 0xff);
  digest[13] = (Guchar)((d >>= 8) & 0xff);
  digest[14] = (Guchar)((d >>= 8) & 0xff);
  digest[15] = (Guchar)((d >>= 8) & 0xff);
}
