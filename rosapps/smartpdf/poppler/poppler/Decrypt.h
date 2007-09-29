//========================================================================
//
// Decrypt.h
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef DECRYPT_H
#define DECRYPT_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "goo/gtypes.h"
#include "goo/GooString.h"

//------------------------------------------------------------------------
// Decrypt
//------------------------------------------------------------------------

class Decrypt {
public:

  // Initialize the decryptor object.
  Decrypt(Guchar *fileKey, int keyLength, int objNum, int objGen);

  // Reset decryption.
  void reset();

  // Decrypt one byte.
  Guchar decryptByte(Guchar c);

  // Generate a file key.  The <fileKey> buffer must have space for at
  // least 16 bytes.  Checks <ownerPassword> and then <userPassword>
  // and returns true if either is correct.  Sets <ownerPasswordOk> if
  // the owner password was correct.  Either or both of the passwords
  // may be NULL, which is treated as an empty string.
  static GBool makeFileKey(int encVersion, int encRevision, int keyLength,
			   GooString *ownerKey, GooString *userKey,
			   int permissions, GooString *fileID,
			   GooString *ownerPassword, GooString *userPassword,
			   Guchar *fileKey, GBool encryptMetadata,
			   GBool *ownerPasswordOk);

private:

  static GBool makeFileKey2(int encVersion, int encRevision, int keyLength,
			    GooString *ownerKey, GooString *userKey,
			    int permissions, GooString *fileID,
			    GooString *userPassword, Guchar *fileKey,
			    GBool encryptMetadata);

  int objKeyLength;
  Guchar objKey[21];
  Guchar state[256];
  Guchar x, y;
};

#endif
