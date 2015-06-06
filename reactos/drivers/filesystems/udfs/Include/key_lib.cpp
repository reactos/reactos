////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////

static const char XPEHb[] = "zfvbgt^&*()aq,lpwdenjsxnygv!@yhuhb#$%chimuokbr";

UDF_FibonachiNum(
    int n,
    int* f
    )
{
    int a=0xff557788;
    int i;
    // do something
    n <<= 8;
    for(i=0; i<n; i = i++) {
        a = ((a+i)*2) ^ ((a+n) * (XPEHb[i % (sizeof(XPEHb)-1)]) & 0xfffffffe) + 1;
        if(i*2 >= n) {
            n >>= 4;
            (*f) = (*f) ^ (a+n);
            n >>= 1;
            a = n & a;
        }
    }
    n >>= 3;
    // if(n < 2)
    if(!(n & ~1))
        return 1;
    n--;
    if(!(n+1))
        return 1;
    a = UDF_FibonachiNum(n, f);
    return UDF_FibonachiNum(n-1, f) + a;
}

void
UDF_build_long_key(
    char* buffer,
    int blen,
    char* key_str,
    int klen
    )
{
    int i, k, j;
    int r[32];
    int* tmp = (int*)buffer;
    int f, fn;

    memcpy(buffer, key_str, klen);
    for(i=0; i<klen/4; i++) {
        r[i%32] = tmp[i];
    }
    f = 0xf4acb89e;
    for(k=0, fn=1, j=0; i<blen/4; i++) {
        if(!fn) {
            tmp[i] = tmp[k%(klen/4)];
            fn = UDF_FibonachiNum(k, &f);
            k++;
            continue;
        }
        if(i>=blen/4)
            break;
        r[j%(klen/4)] = (int32)( ((int64)r[j%(klen/4)] * 0x8088405 + 1) >> 3 );
        tmp[i] = r[j%(klen/4)] ^ f;
        j++;
        fn--;
    }
} // end UDF_build_long_key()


void
UDF_build_hash_by_key(
    char* longkey_buffer,
    int   longkey_len,
    char* key_hash,
    char* key_str
    )
{
    UDF_MD5_CTX context;
    char key1[16];
    int m;

    UDF_build_long_key(longkey_buffer, longkey_len, key_str, 16);
    UDF_MD5Init(&context);
    UDF_MD5Update(&context, (PUCHAR)longkey_buffer, longkey_len);
    UDF_MD5Pad (&context);
    UDF_MD5Final((PUCHAR)key_hash, &context);
    memcpy(key1, key_hash, 16);
    for(m = 0; m<113; m++) {
        UDF_build_long_key(longkey_buffer, longkey_len, key_hash, 16);
        UDF_MD5Init(&context);
        UDF_MD5Update(&context, (PUCHAR)longkey_buffer, longkey_len);
        UDF_MD5Pad (&context);
        UDF_MD5Final((PUCHAR)key_hash, &context);
    }
    for(m=0; m<16; m++) {
        key_hash[m] = key_hash[m] ^ key1[m];
    }
} // end UDF_build_hash_by_key()

