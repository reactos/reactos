////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////

#include "platform.h"
#include "md5.h"

#define UDF_LONG_KEY_SIZE  1024

extern void
UDF_build_long_key(
    char* buffer,
    int blen,
    char* key_str,
    int klen
    );

extern void
UDF_build_hash_by_key(
    char* longkey_buffer,
    int   longkey_len,
    char* key_hash,
    char* key_str
    );
