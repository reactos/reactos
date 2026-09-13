/*
 * Copyright 2005 Kees Cook <kees@outflux.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */


/*
 * The Win32 CryptProtectData and CryptUnprotectData functions are meant
 * to provide a mechanism for encrypting data on a machine where other users
 * of the system can't be trusted.  It is used in many examples as a way
 * to store username and password information to the registry, but store
 * it not in the clear.
 *
 * The encryption is symmetric, but the method is unknown.  However, since
 * it is keyed to the machine and the user, it is unlikely that the values
 * would be portable.  Since programs must first call CryptProtectData to
 * get a cipher text, the underlying system doesn't have to exactly
 * match the real Windows version.  However, attempts have been made to
 * at least try to look like the Windows version, including guesses at the
 * purpose of various portions of the "opaque data blob" that is used.
 *
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

#define CRYPT32_PROTECTDATA_PROV      PROV_RSA_FULL
#define CRYPT32_PROTECTDATA_HASH_CALG CALG_SHA1
#define CRYPT32_PROTECTDATA_HASH_LEN  160
#define CRYPT32_PROTECTDATA_KEY_CALG  CALG_3DES
#define CRYPT32_PROTECTDATA_KEY_LEN   168
#define CRYPT32_PROTECTDATA_SALT_LEN  16

static const BYTE crypt32_protectdata_secret[] = {
    'I','\'','m',' ','h','u','n','t','i','n','g',' ',
    'w','a','b','b','i','t','s',0
};

/*
 * The data format returned by the real Windows CryptProtectData seems
 * to be something like this:

 DWORD  count0;         - how many "info0_*[16]" blocks follow (was always 1)
 BYTE   info0_0[16];    - unknown information - persistent across invocations,
 ...                      reboots, password changes, and users
 DWORD  count1;         - how many "info1_*[16]" blocks follow (was always 1)
 BYTE   info1_0[16];    - unknown information - unique to each user, but
 ...                      persistent across reboots and password changes
 DWORD  null0;          - NULL "end of records"?
 DWORD  str_len;        - byte length of WCHAR string including term
 BYTE   str[str_len];   - The "dataDescription" value as a NULL-terminated
                          little-endian WCHAR string
 ALG_ID cipher_alg;     - cipher algo - was CALG_3DES
 DWORD  cipher_key_len; - cipher key bit length - was 0xa8==168
 DWORD  data_len;       - length of data (was 16 in samples)
 BYTE   data[data_len]; - unknown data (fingerprint?)
 DWORD  null1;          - NULL ?
 ALG_ID hash_alg;       - hash algo - was CALG_SHA1
 DWORD  hash_len;       - bit length of hash - was 0xa0==160
 DWORD  salt_len;       - length of salt(?) data
 BYTE   salt[salt_len]; - salt(?) for symmetric encryption
 DWORD  cipher_len;     - length of cipher(?) data - was close to plain len
 BYTE   cipher[cipher_len]; - cipher text?
 DWORD  crc_len;        - length of fingerprint(?) data - was 20 byte==160b SHA1
 BYTE   crc[crc_len];   - fingerprint of record?

 * The data structures used in Wine are modelled after this guess.
 */

struct protect_data_t
{
    DWORD       count0;
    DATA_BLOB   info0;        /* using this to hold crypt_magic_str */
    DWORD       count1;
    DATA_BLOB   info1;
    DWORD       null0;
    WCHAR *     szDataDescr;  /* serialized differently than the DATA_BLOBs */
    ALG_ID      cipher_alg;
    DWORD       cipher_key_len;
    DATA_BLOB   data0;
    DWORD       null1;
    ALG_ID      hash_alg;
    DWORD       hash_len;
    DATA_BLOB   salt;
    DATA_BLOB   cipher;
    DATA_BLOB   fingerprint;
};

/* this is used to check if an incoming structure was built by Wine */
static const char crypt_magic_str[] = "Wine Crypt32 ok";

/* debugging tool to print strings of hex chars */
static const char *
hex_str(const unsigned char *p, int n)
{
    const char * ptr;
    char report[80];
    int r=-1;
    report[0]='\0';
    ptr = wine_dbg_sprintf("%s","");
    while (--n >= 0)
    {
        if (r++ % 20 == 19)
        {
            ptr = wine_dbg_sprintf("%s%s",ptr,report);
            report[0]='\0';
        }
        sprintf(report+strlen(report),"%s%02x", r ? "," : "", *p++);
    }
    return wine_dbg_sprintf("%s%s",ptr,report);
}

#define TRACE_DATA_BLOB(blob) do { \
    TRACE("%s cbData: %u\n", #blob ,(unsigned int)((blob)->cbData)); \
    TRACE("%s pbData @ %p:%s\n", #blob ,(blob)->pbData, \
          hex_str((blob)->pbData, (blob)->cbData)); \
} while (0)

static
void serialize_dword(DWORD value,BYTE ** ptr)
{
    /*TRACE("called\n");*/

    memcpy(*ptr,&value,sizeof(DWORD));
    *ptr+=sizeof(DWORD);
}

static
void serialize_string(const BYTE *str, BYTE **ptr, DWORD len, DWORD width,
                      BOOL prepend_len)
{
    /*TRACE("called %ux%u\n",(unsigned int)len,(unsigned int)width);*/

    if (prepend_len)
    {
        serialize_dword(len,ptr);
    }
    memcpy(*ptr,str,len*width);
    *ptr+=len*width;
}

static
BOOL unserialize_dword(const BYTE *ptr, DWORD *index, DWORD size, DWORD *value)
{
    /*TRACE("called\n");*/

    if (!ptr || !index || !value) return FALSE;

    if (*index+sizeof(DWORD)>size)
    {
        return FALSE;
    }

    memcpy(value,&(ptr[*index]),sizeof(DWORD));
    *index+=sizeof(DWORD);

    return TRUE;
}

static
BOOL unserialize_string(const BYTE *ptr, DWORD *index, DWORD size,
                        DWORD len, DWORD width, BOOL inline_len,
                        BYTE ** data, DWORD * stored)
{
    /*TRACE("called\n");*/

    if (!ptr || !data) return FALSE;

    if (inline_len) {
        if (!unserialize_dword(ptr,index,size,&len))
            return FALSE;
    }

    if (*index+len*width>size)
    {
        return FALSE;
    }

    if (!(*data = CryptMemAlloc( len*width)))
    {
        return FALSE;
    }

    memcpy(*data,&(ptr[*index]),len*width);
    if (stored)
    {
        *stored = len;
    }
    *index+=len*width;

    return TRUE;
}

static
BOOL serialize(const struct protect_data_t *pInfo, DATA_BLOB *pSerial)
{
    BYTE * ptr;
    DWORD dwStrLen;
    DWORD dwStruct;

    TRACE("called\n");

    if (!pInfo || !pInfo->szDataDescr || !pSerial ||
        !pInfo->info0.pbData || !pInfo->info1.pbData ||
        !pInfo->data0.pbData || !pInfo->salt.pbData ||
        !pInfo->cipher.pbData || !pInfo->fingerprint.pbData)
    {
        return FALSE;
    }

    if (pInfo->info0.cbData!=16)
    {
        ERR("protect_data_t info0 not 16 bytes long\n");
    }

    if (pInfo->info1.cbData!=16)
    {
        ERR("protect_data_t info1 not 16 bytes long\n");
    }

    dwStrLen=lstrlenW(pInfo->szDataDescr);

    pSerial->cbData=0;
    pSerial->cbData+=sizeof(DWORD)*8; /* 8 raw DWORDs */
    pSerial->cbData+=sizeof(DWORD)*4; /* 4 BLOBs with size */
    pSerial->cbData+=pInfo->info0.cbData;
    pSerial->cbData+=pInfo->info1.cbData;
    pSerial->cbData+=(dwStrLen+1)*sizeof(WCHAR) + 4; /* str, null, size */
    pSerial->cbData+=pInfo->data0.cbData;
    pSerial->cbData+=pInfo->salt.cbData;
    pSerial->cbData+=pInfo->cipher.cbData;
    pSerial->cbData+=pInfo->fingerprint.cbData;

    /* save the actual structure size */
    dwStruct = pSerial->cbData;
    /* There may be a 256 byte minimum, but I can't prove it. */
    /*if (pSerial->cbData<256) pSerial->cbData=256;*/

    pSerial->pbData=LocalAlloc(LPTR,pSerial->cbData);
    if (!pSerial->pbData) return FALSE;

    ptr=pSerial->pbData;

    /* count0 */
    serialize_dword(pInfo->count0,&ptr);
    /*TRACE("used %u\n",ptr-pSerial->pbData);*/
    
    /* info0 */
    serialize_string(pInfo->info0.pbData,&ptr,
                     pInfo->info0.cbData,sizeof(BYTE),FALSE);
    /*TRACE("used %u\n",ptr-pSerial->pbData);*/

    /* count1 */
    serialize_dword(pInfo->count1,&ptr);
    /*TRACE("used %u\n",ptr-pSerial->pbData);*/

    /* info1 */
    serialize_string(pInfo->info1.pbData,&ptr,
                     pInfo->info1.cbData,sizeof(BYTE),FALSE);
    /*TRACE("used %u\n",ptr-pSerial->pbData);*/

    /* null0 */
    serialize_dword(pInfo->null0,&ptr);
    /*TRACE("used %u\n",ptr-pSerial->pbData);*/
    
    /* szDataDescr */
    serialize_string((BYTE*)pInfo->szDataDescr,&ptr,
                     (dwStrLen+1)*sizeof(WCHAR),sizeof(BYTE),TRUE);
    /*TRACE("used %u\n",ptr-pSerial->pbData);*/

    /* cipher_alg */
    serialize_dword(pInfo->cipher_alg,&ptr);
    /*TRACE("used %u\n",ptr-pSerial->pbData);*/
    /* cipher_key_len */
    serialize_dword(pInfo->cipher_key_len,&ptr);
    /*TRACE("used %u\n",ptr-pSerial->pbData);*/
    
    /* data0 */
    serialize_string(pInfo->data0.pbData,&ptr,
                     pInfo->data0.cbData,sizeof(BYTE),TRUE);
    /*TRACE("used %u\n",ptr-pSerial->pbData);*/

    /* null1 */
    serialize_dword(pInfo->null1,&ptr);
    /*TRACE("used %u\n",ptr-pSerial->pbData);*/
    
    /* hash_alg */
    serialize_dword(pInfo->hash_alg,&ptr);
    /*TRACE("used %u\n",ptr-pSerial->pbData);*/
    /* hash_len */
    serialize_dword(pInfo->hash_len,&ptr);
    /*TRACE("used %u\n",ptr-pSerial->pbData);*/
    
    /* salt */
    serialize_string(pInfo->salt.pbData,&ptr,
                     pInfo->salt.cbData,sizeof(BYTE),TRUE);
    /*TRACE("used %u\n",ptr-pSerial->pbData);*/

    /* cipher */
    serialize_string(pInfo->cipher.pbData,&ptr,
                     pInfo->cipher.cbData,sizeof(BYTE),TRUE);
    /*TRACE("used %u\n",ptr-pSerial->pbData);*/

    /* fingerprint */
    serialize_string(pInfo->fingerprint.pbData,&ptr,
                     pInfo->fingerprint.cbData,sizeof(BYTE),TRUE);
    /*TRACE("used %u\n",ptr-pSerial->pbData);*/

    if (ptr - pSerial->pbData != dwStruct)
    {
        ERR("struct size changed!? expected %lu\n", dwStruct);
        LocalFree(pSerial->pbData);
        pSerial->pbData=NULL;
        pSerial->cbData=0;
        return FALSE;
    }

    return TRUE;
}

static
BOOL unserialize(const DATA_BLOB *pSerial, struct protect_data_t *pInfo)
{
    BYTE * ptr;
    DWORD index;
    DWORD size;
    BOOL status=TRUE;

    TRACE("called\n");

    if (!pInfo || !pSerial || !pSerial->pbData)
        return FALSE;

    index=0;
    ptr=pSerial->pbData;
    size=pSerial->cbData;

    /* count0 */
    if (!unserialize_dword(ptr,&index,size,&pInfo->count0))
    {
        ERR("reading count0 failed!\n");
        return FALSE;
    }
    
    /* info0 */
    if (!unserialize_string(ptr,&index,size,16,sizeof(BYTE),FALSE,
                            &pInfo->info0.pbData, &pInfo->info0.cbData))
    {
        ERR("reading info0 failed!\n");
        return FALSE;
    }

    /* count1 */
    if (!unserialize_dword(ptr,&index,size,&pInfo->count1))
    {
        ERR("reading count1 failed!\n");
        return FALSE;
    }

    /* info1 */
    if (!unserialize_string(ptr,&index,size,16,sizeof(BYTE),FALSE,
                            &pInfo->info1.pbData, &pInfo->info1.cbData))
    {
        ERR("reading info1 failed!\n");
        return FALSE;
    }

    /* null0 */
    if (!unserialize_dword(ptr,&index,size,&pInfo->null0))
    {
        ERR("reading null0 failed!\n");
        return FALSE;
    }
    
    /* szDataDescr */
    if (!unserialize_string(ptr,&index,size,0,sizeof(BYTE),TRUE,
                            (BYTE**)&pInfo->szDataDescr, NULL))
    {
        ERR("reading szDataDescr failed!\n");
        return FALSE;
    }

    /* cipher_alg */
    if (!unserialize_dword(ptr,&index,size,(DWORD*)&pInfo->cipher_alg))
    {
        ERR("reading cipher_alg failed!\n");
        return FALSE;
    }
    
    /* cipher_key_len */
    if (!unserialize_dword(ptr,&index,size,&pInfo->cipher_key_len))
    {
        ERR("reading cipher_key_len failed!\n");
        return FALSE;
    }
    
    /* data0 */
    if (!unserialize_string(ptr,&index,size,0,sizeof(BYTE),TRUE,
                            &pInfo->data0.pbData, &pInfo->data0.cbData))
    {
        ERR("reading data0 failed!\n");
        return FALSE;
    }

    /* null1 */
    if (!unserialize_dword(ptr,&index,size,&pInfo->null1))
    {
        ERR("reading null1 failed!\n");
        return FALSE;
    }
    
    /* hash_alg */
    if (!unserialize_dword(ptr,&index,size,(DWORD*)&pInfo->hash_alg))
    {
        ERR("reading hash_alg failed!\n");
        return FALSE;
    }
    
    /* hash_len */
    if (!unserialize_dword(ptr,&index,size,&pInfo->hash_len))
    {
        ERR("reading hash_len failed!\n");
        return FALSE;
    }
    
    /* salt */
    if (!unserialize_string(ptr,&index,size,0,sizeof(BYTE),TRUE,
                            &pInfo->salt.pbData, &pInfo->salt.cbData))
    {
        ERR("reading salt failed!\n");
        return FALSE;
    }

    /* cipher */
    if (!unserialize_string(ptr,&index,size,0,sizeof(BYTE),TRUE,
                            &pInfo->cipher.pbData, &pInfo->cipher.cbData))
    {
        ERR("reading cipher failed!\n");
        return FALSE;
    }

    /* fingerprint */
    if (!unserialize_string(ptr,&index,size,0,sizeof(BYTE),TRUE,
                            &pInfo->fingerprint.pbData, &pInfo->fingerprint.cbData))
    {
        ERR("reading fingerprint failed!\n");
        return FALSE;
    }

    /* allow structure size to be too big (since some applications
     * will pad this up to 256 bytes, it seems) */
    if (index>size)
    {
        /* this is an impossible-to-reach test, but if the padding
         * issue is ever understood, this may become more useful */
        ERR("loaded corrupt structure! (used %lu expected %lu)\n", index, size);
        status=FALSE;
    }

    return status;
}

/* perform sanity checks */
static
BOOL valid_protect_data(const struct protect_data_t *pInfo)
{
    BOOL status=TRUE;

    TRACE("called\n");

    if (pInfo->count0 != 0x0001)
    {
        ERR("count0 != 0x0001 !\n");
        status=FALSE;
    }
    if (pInfo->count1 != 0x0001)
    {
        ERR("count0 != 0x0001 !\n");
        status=FALSE;
    }
    if (pInfo->null0 != 0x0000)
    {
        ERR("null0 != 0x0000 !\n");
        status=FALSE;
    }
    if (pInfo->null1 != 0x0000)
    {
        ERR("null1 != 0x0000 !\n");
        status=FALSE;
    }
    /* since we have no idea what info0 is used for, and it seems
     * rather constant, we can test for a Wine-specific magic string
     * there to be reasonably sure we're using data created by the Wine
     * implementation of CryptProtectData.
     */
    if (pInfo->info0.cbData!=strlen(crypt_magic_str)+1 ||
        strcmp( (LPCSTR)pInfo->info0.pbData,crypt_magic_str) != 0)
    {
        ERR("info0 magic value not matched !\n");
        status=FALSE;
    }

    if (!status)
    {
        ERR("unrecognized CryptProtectData block\n");
    }

    return status;
}

static
void free_protect_data(struct protect_data_t * pInfo)
{
    TRACE("called\n");

    if (!pInfo) return;

    CryptMemFree(pInfo->info0.pbData);
    CryptMemFree(pInfo->info1.pbData);
    CryptMemFree(pInfo->szDataDescr);
    CryptMemFree(pInfo->data0.pbData);
    CryptMemFree(pInfo->salt.pbData);
    CryptMemFree(pInfo->cipher.pbData);
    CryptMemFree(pInfo->fingerprint.pbData);
}

/* copies a string into a data blob */
static
BYTE *convert_str_to_blob(LPCSTR str, DATA_BLOB *blob)
{
    if (!str || !blob) return NULL;

    blob->cbData=strlen(str)+1;
    if (!(blob->pbData=CryptMemAlloc(blob->cbData)))
    {
        blob->cbData=0;
    }
    else {
        strcpy((LPSTR)blob->pbData, str);
    }

    return blob->pbData;
}

/*
 * Populates everything except "cipher" and "fingerprint".
 */
static
BOOL fill_protect_data(struct protect_data_t * pInfo, LPCWSTR szDataDescr,
                       HCRYPTPROV hProv)
{
    DWORD dwStrLen;

    TRACE("called\n");

    if (!pInfo) return FALSE;

    dwStrLen=lstrlenW(szDataDescr);

    memset(pInfo,0,sizeof(*pInfo));

    pInfo->count0=0x0001;

    convert_str_to_blob(crypt_magic_str, &pInfo->info0);

    pInfo->count1=0x0001;

    convert_str_to_blob(crypt_magic_str, &pInfo->info1);

    pInfo->null0=0x0000;

    if ((pInfo->szDataDescr=CryptMemAlloc((dwStrLen+1)*sizeof(WCHAR))))
    {
        memcpy(pInfo->szDataDescr,szDataDescr,(dwStrLen+1)*sizeof(WCHAR));
    }

    pInfo->cipher_alg=CRYPT32_PROTECTDATA_KEY_CALG;
    pInfo->cipher_key_len=CRYPT32_PROTECTDATA_KEY_LEN;

    convert_str_to_blob(crypt_magic_str, &pInfo->data0);

    pInfo->null1=0x0000;
    pInfo->hash_alg=CRYPT32_PROTECTDATA_HASH_CALG;
    pInfo->hash_len=CRYPT32_PROTECTDATA_HASH_LEN;

    /* allocate memory to hold a salt */
    if ((pInfo->salt.pbData=CryptMemAlloc(CRYPT32_PROTECTDATA_SALT_LEN)))
    {
        /* generate random salt */
        if (!CryptGenRandom(hProv, CRYPT32_PROTECTDATA_SALT_LEN, pInfo->salt.pbData))
        {
            ERR("CryptGenRandom\n");
            free_protect_data(pInfo);
            return FALSE;
        }
        pInfo->salt.cbData=CRYPT32_PROTECTDATA_SALT_LEN;
        /* debug: show our salt */
        TRACE_DATA_BLOB(&pInfo->salt);
    }
    pInfo->cipher.cbData=0;
    pInfo->cipher.pbData=NULL;

    pInfo->fingerprint.cbData=0;
    pInfo->fingerprint.pbData=NULL;

    /* check all the allocations at once */
    if (!pInfo->info0.pbData ||
        !pInfo->info1.pbData ||
        !pInfo->szDataDescr  ||
        !pInfo->data0.pbData ||
        !pInfo->salt.pbData
        )
    {
        ERR("could not allocate protect_data structures\n");
        free_protect_data(pInfo);
        return FALSE;
    }

    return TRUE;
}

static
BOOL convert_hash_to_blob(HCRYPTHASH hHash, DATA_BLOB * blob)
{
    DWORD dwSize;

    TRACE("called\n");

    if (!blob) return FALSE;

    dwSize=sizeof(DWORD);
    if (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&blob->cbData,
                           &dwSize, 0))
    {
        ERR("failed to get hash size\n");
        return FALSE;
    }

    if (!(blob->pbData=CryptMemAlloc(blob->cbData)))
    {
        ERR("failed to allocate blob memory\n");
        return FALSE;
    }

    dwSize=blob->cbData;
    if (!CryptGetHashParam(hHash, HP_HASHVAL, blob->pbData, &dwSize, 0))
    {
        ERR("failed to get hash value\n");
        CryptMemFree(blob->pbData);
        blob->pbData=NULL;
        blob->cbData=0;
        return FALSE;
    }

    return TRUE;
}

/* test that a given hash matches an exported-to-blob hash value */
static
BOOL hash_matches_blob(HCRYPTHASH hHash, const DATA_BLOB *two)
{
    BOOL rc = FALSE;
    DATA_BLOB one;

    if (!two || !two->pbData) return FALSE;

    if (!convert_hash_to_blob(hHash,&one)) {
        return FALSE;
    }

    if ( one.cbData == two->cbData &&
         memcmp( one.pbData, two->pbData, one.cbData ) == 0 )
    {
        rc = TRUE;
    }

    CryptMemFree(one.pbData);
    return rc;
}

/* create an encryption key from a given salt and optional entropy */
static
BOOL load_encryption_key(HCRYPTPROV hProv, DWORD key_len, const DATA_BLOB *salt,
                         const DATA_BLOB *pOptionalEntropy, HCRYPTKEY *phKey)
{
    BOOL rc = TRUE;
    HCRYPTHASH hSaltHash;
    char * szUsername = NULL;
    DWORD dwUsernameLen;
    DWORD dwError;

    /* create hash for salt */
    if (!salt || !phKey ||
        !CryptCreateHash(hProv,CRYPT32_PROTECTDATA_HASH_CALG,0,0,&hSaltHash))
    {
        ERR("CryptCreateHash\n");
        return FALSE;
    }

    /* This should be the "logon credentials" instead of username */
    dwError=GetLastError();
    dwUsernameLen = 0;
    if (!GetUserNameA(NULL, &dwUsernameLen) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER && dwUsernameLen &&
        (szUsername = CryptMemAlloc(dwUsernameLen)))
    {
        szUsername[0]='\0';
        GetUserNameA( szUsername, &dwUsernameLen );
    }
    SetLastError(dwError);

    /* salt the hash with:
     * - the user id
     * - an "internal secret"
     * - randomness (from the salt)
     * - user-supplied entropy
     */
    if ((szUsername && !CryptHashData(hSaltHash,(LPBYTE)szUsername,dwUsernameLen,0)) ||
        !CryptHashData(hSaltHash,crypt32_protectdata_secret,
                                 sizeof(crypt32_protectdata_secret)-1,0) ||
        !CryptHashData(hSaltHash,salt->pbData,salt->cbData,0) ||
        (pOptionalEntropy && !CryptHashData(hSaltHash,
                                            pOptionalEntropy->pbData,
                                            pOptionalEntropy->cbData,0)))
    {
        ERR("CryptHashData\n");
        rc = FALSE;
    }

    /* produce a symmetric key */
    if (rc && !CryptDeriveKey(hProv,CRYPT32_PROTECTDATA_KEY_CALG,
                              hSaltHash,key_len << 16 | CRYPT_EXPORTABLE,phKey))
    {
        ERR("CryptDeriveKey\n");
        rc = FALSE;
    }

    /* clean up */
    CryptDestroyHash(hSaltHash);
    CryptMemFree(szUsername);

    return rc;
}

/* debugging tool to print the structures of a ProtectData call */
static void
report(const DATA_BLOB* pDataIn, const DATA_BLOB* pOptionalEntropy,
       CRYPTPROTECT_PROMPTSTRUCT* pPromptStruct, DWORD dwFlags)
{
    TRACE("pPromptStruct: %p\n", pPromptStruct);
    if (pPromptStruct)
    {
        TRACE("  cbSize: 0x%lx\n", pPromptStruct->cbSize);
        TRACE("  dwPromptFlags: 0x%lx\n", pPromptStruct->dwPromptFlags);
        TRACE("  hwndApp: %p\n", pPromptStruct->hwndApp);
        TRACE("  szPrompt: %p %s\n",
              pPromptStruct->szPrompt,
              pPromptStruct->szPrompt ? debugstr_w(pPromptStruct->szPrompt)
              : "");
    }
    TRACE("dwFlags: 0x%04lx\n", dwFlags);
    TRACE_DATA_BLOB(pDataIn);
    if (pOptionalEntropy)
    {
        TRACE_DATA_BLOB(pOptionalEntropy);
        TRACE("  %s\n",debugstr_an((LPCSTR)pOptionalEntropy->pbData,pOptionalEntropy->cbData));
    }

}


/***************************************************************************
 * CryptProtectData     [CRYPT32.@]
 *
 * Generate Cipher data from given Plain and Entropy data.
 *
 * PARAMS
 *  pDataIn          [I] Plain data to be enciphered
 *  szDataDescr      [I] Optional Unicode string describing the Plain data
 *  pOptionalEntropy [I] Optional entropy data to adjust cipher, can be NULL
 *  pvReserved       [I] Reserved, must be NULL
 *  pPromptStruct    [I] Structure describing if/how to prompt during ciphering
 *  dwFlags          [I] Flags describing options to the ciphering
 *  pDataOut         [O] Resulting Cipher data, for calls to CryptUnprotectData
 *
 * RETURNS
 *  TRUE  If a Cipher was generated.
 *  FALSE If something failed and no Cipher is available.
 *
 * FIXME
 *  The true Windows encryption and keying mechanisms are unknown.
 *
 *  dwFlags and pPromptStruct are currently ignored.
 *
 * NOTES
 *  Memory allocated in pDataOut must be freed with LocalFree.
 *
 */
BOOL WINAPI CryptProtectData(DATA_BLOB* pDataIn,
                             LPCWSTR szDataDescr,
                             DATA_BLOB* pOptionalEntropy,
                             PVOID pvReserved,
                             CRYPTPROTECT_PROMPTSTRUCT* pPromptStruct,
                             DWORD dwFlags,
                             DATA_BLOB* pDataOut)
{
    BOOL rc = FALSE;
    HCRYPTPROV hProv;
    struct protect_data_t protect_data;
    HCRYPTHASH hHash;
    HCRYPTKEY hKey;
    DWORD dwLength;

    TRACE("called\n");

    SetLastError(ERROR_SUCCESS);

    if (!pDataIn || !pDataOut)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto finished;
    }

    /* debug: show our arguments */
    report(pDataIn,pOptionalEntropy,pPromptStruct,dwFlags);
    TRACE("\tszDataDescr: %p %s\n", szDataDescr,
          szDataDescr ? debugstr_w(szDataDescr) : "");

    /* Windows appears to create an empty szDataDescr instead of maintaining
     * a NULL */
    if (!szDataDescr)
        szDataDescr = L"";

    /* get crypt context */
    if (!CryptAcquireContextW(&hProv,NULL,MS_ENHANCED_PROV_W,CRYPT32_PROTECTDATA_PROV,CRYPT_VERIFYCONTEXT))
    {
        ERR("CryptAcquireContextW failed\n");
        goto finished;
    }

    /* populate our structure */
    if (!fill_protect_data(&protect_data,szDataDescr,hProv))
    {
        ERR("fill_protect_data\n");
        goto free_context;
    }

    /* load key */
    if (!load_encryption_key(hProv,protect_data.cipher_key_len,&protect_data.salt,pOptionalEntropy,&hKey))
    {
        goto free_protect_data;
    }

    /* create a hash for the encryption validation */
    if (!CryptCreateHash(hProv,CRYPT32_PROTECTDATA_HASH_CALG,0,0,&hHash))
    {
        ERR("CryptCreateHash\n");
        goto free_key;
    }

    /* calculate storage required */
    dwLength=pDataIn->cbData;
    if (CryptEncrypt(hKey, 0, TRUE, 0, pDataIn->pbData, &dwLength, 0) ||
        GetLastError()!=ERROR_MORE_DATA)
    {
        ERR("CryptEncrypt\n");
        goto free_hash;
    }
    TRACE("required encrypted storage: %lu\n", dwLength);

    /* copy plain text into cipher area for CryptEncrypt call */
    protect_data.cipher.cbData=dwLength;
    if (!(protect_data.cipher.pbData=CryptMemAlloc(
                                                protect_data.cipher.cbData)))
    {
        ERR("CryptMemAlloc\n");
        goto free_hash;
    }
    memcpy(protect_data.cipher.pbData,pDataIn->pbData,pDataIn->cbData);

    /* encrypt! */
    dwLength=pDataIn->cbData;
    if (!CryptEncrypt(hKey, hHash, TRUE, 0, protect_data.cipher.pbData,
                      &dwLength, protect_data.cipher.cbData))
    {
        ERR("CryptEncrypt %lu\n", GetLastError());
        goto free_hash;
    }
    protect_data.cipher.cbData=dwLength;

    /* debug: show the cipher */
    TRACE_DATA_BLOB(&protect_data.cipher);

    /* attach our fingerprint */
    if (!convert_hash_to_blob(hHash, &protect_data.fingerprint))
    {
        ERR("convert_hash_to_blob\n");
        goto free_hash;
    }

    /* serialize into an opaque blob */
    if (!serialize(&protect_data, pDataOut))
    {
        ERR("serialize\n");
        goto free_hash;
    }

    /* success! */
    rc=TRUE;

free_hash:
    CryptDestroyHash(hHash);
free_key:
    CryptDestroyKey(hKey);
free_protect_data:
    free_protect_data(&protect_data);
free_context:
    CryptReleaseContext(hProv,0);
finished:
    /* If some error occurred, and no error code was set, force one. */
    if (!rc && GetLastError()==ERROR_SUCCESS)
    {
        SetLastError(ERROR_INVALID_DATA);
    }

    if (rc)
    {
        SetLastError(ERROR_SUCCESS);

        TRACE_DATA_BLOB(pDataOut);
    }

    TRACE("returning %s\n", rc ? "ok" : "FAIL");

    return rc;
}


/***************************************************************************
 * CryptUnprotectData   [CRYPT32.@]
 *
 * Generate Plain data and Description from given Cipher and Entropy data.
 *
 * PARAMS
 *  pDataIn          [I] Cipher data to be decoded
 *  ppszDataDescr    [O] Optional Unicode string describing the Plain data
 *  pOptionalEntropy [I] Optional entropy data to adjust cipher, can be NULL
 *  pvReserved       [I] Reserved, must be NULL
 *  pPromptStruct    [I] Structure describing if/how to prompt during decoding
 *  dwFlags          [I] Flags describing options to the decoding
 *  pDataOut         [O] Resulting Plain data, from calls to CryptProtectData
 *
 * RETURNS
 *  TRUE  If a Plain was generated.
 *  FALSE If something failed and no Plain is available.
 *
 * FIXME
 *  The true Windows encryption and keying mechanisms are unknown.
 *
 *  dwFlags and pPromptStruct are currently ignored.
 *
 * NOTES
 *  Memory allocated in pDataOut and non-NULL ppszDataDescr must be freed
 *  with LocalFree.
 *
 */
BOOL WINAPI CryptUnprotectData(DATA_BLOB* pDataIn,
                               LPWSTR * ppszDataDescr,
                               DATA_BLOB* pOptionalEntropy,
                               PVOID pvReserved,
                               CRYPTPROTECT_PROMPTSTRUCT* pPromptStruct,
                               DWORD dwFlags,
                               DATA_BLOB* pDataOut)
{
    BOOL rc = FALSE;

    HCRYPTPROV hProv;
    struct protect_data_t protect_data;
    HCRYPTHASH hHash;
    HCRYPTKEY hKey;
    DWORD dwLength;

    const char * announce_bad_opaque_data = "CryptUnprotectData received a DATA_BLOB that seems to have NOT been generated by Wine.  Please enable tracing ('export WINEDEBUG=crypt') to see details.";

    TRACE("called\n");

    SetLastError(ERROR_SUCCESS);

    if (!pDataIn || !pDataOut)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto finished;
    }
    if (!pDataIn->cbData)
    {
        SetLastError(ERROR_INVALID_DATA);
        goto finished;
    }

    /* debug: show our arguments */
    report(pDataIn,pOptionalEntropy,pPromptStruct,dwFlags);
    TRACE("\tppszDataDescr: %p\n", ppszDataDescr);

    /* take apart the opaque blob */
    if (!unserialize(pDataIn, &protect_data))
    {
        SetLastError(ERROR_INVALID_DATA);
        FIXME("%s\n",announce_bad_opaque_data);
        goto finished;
    }

    /* perform basic validation on the resulting structure */
    if (!valid_protect_data(&protect_data))
    {
        SetLastError(ERROR_INVALID_DATA);
        FIXME("%s\n",announce_bad_opaque_data);
        goto free_protect_data;
    }

    /* get a crypt context */
    if (!CryptAcquireContextW(&hProv,NULL,MS_ENHANCED_PROV_W,CRYPT32_PROTECTDATA_PROV,CRYPT_VERIFYCONTEXT))
    {
        ERR("CryptAcquireContextW failed\n");
        goto free_protect_data;
    }

    /* load key */
    if (!load_encryption_key(hProv,protect_data.cipher_key_len,&protect_data.salt,pOptionalEntropy,&hKey))
    {
        goto free_context;
    }

    /* create a hash for the decryption validation */
    if (!CryptCreateHash(hProv,CRYPT32_PROTECTDATA_HASH_CALG,0,0,&hHash))
    {
        ERR("CryptCreateHash\n");
        goto free_key;
    }

    /* prepare for plaintext */
    pDataOut->cbData=protect_data.cipher.cbData;
    if (!(pDataOut->pbData=LocalAlloc( LPTR, pDataOut->cbData)))
    {
        ERR("CryptMemAlloc\n");
        goto free_hash;
    }
    memcpy(pDataOut->pbData,protect_data.cipher.pbData,protect_data.cipher.cbData);

    /* decrypt! */
    if (!CryptDecrypt(hKey, hHash, TRUE, 0, pDataOut->pbData,
                      &pDataOut->cbData) ||
        /* check the hash fingerprint */
        pDataOut->cbData > protect_data.cipher.cbData ||
        !hash_matches_blob(hHash, &protect_data.fingerprint))
    {
        SetLastError(ERROR_INVALID_DATA);

        LocalFree( pDataOut->pbData );
        pDataOut->pbData = NULL;
        pDataOut->cbData = 0;

        goto free_hash;
    }

    /* Copy out the description */
    dwLength = (lstrlenW(protect_data.szDataDescr)+1) * sizeof(WCHAR);
    if (ppszDataDescr)
    {
        if (!(*ppszDataDescr = LocalAlloc(LPTR,dwLength)))
        {
            ERR("LocalAlloc (ppszDataDescr)\n");
            goto free_hash;
        }
        else {
            memcpy(*ppszDataDescr,protect_data.szDataDescr,dwLength);
        }
    }

    /* success! */
    rc = TRUE;

free_hash:
    CryptDestroyHash(hHash);
free_key:
    CryptDestroyKey(hKey);
free_context:
    CryptReleaseContext(hProv,0);
free_protect_data:
    free_protect_data(&protect_data);
finished:
    /* If some error occurred, and no error code was set, force one. */
    if (!rc && GetLastError()==ERROR_SUCCESS)
    {
        SetLastError(ERROR_INVALID_DATA);
    }

    if (rc) {
        SetLastError(ERROR_SUCCESS);

        if (ppszDataDescr)
        {
            TRACE("szDataDescr: %s\n",debugstr_w(*ppszDataDescr));
        }
        TRACE_DATA_BLOB(pDataOut);
    }

    TRACE("returning %s\n", rc ? "ok" : "FAIL");

    return rc;
}
