/***************************************************************************
 *             chm_lib.c - CHM archive manipulation routines               *
 *                           -------------------                           *
 *                                                                         *
 *  author:     Jed Wing <jedwin@ugcs.caltech.edu>                         *
 *  version:    0.3                                                        *
 *  notes:      These routines are meant for the manipulation of microsoft *
 *              .chm (compiled html help) files, but may likely be used    *
 *              for the manipulation of any ITSS archive, if ever ITSS     *
 *              archives are used for any other purpose.                   *
 *                                                                         *
 *              Note also that the section names are statically handled.   *
 *              To be entirely correct, the section names should be read   *
 *              from the section names meta-file, and then the various     *
 *              content sections and the "transforms" to apply to the data *
 *              they contain should be inferred from the section name and  *
 *              the meta-files referenced using that name; however, all of *
 *              the files I've been able to get my hands on appear to have *
 *              only two sections: Uncompressed and MSCompressed.          *
 *              Additionally, the ITSS.DLL file included with Windows does *
 *              not appear to handle any different transforms than the     *
 *              simple LZX-transform.  Furthermore, the list of transforms *
 *              to apply is broken, in that only half the required space   *
 *              is allocated for the list.  (It appears as though the      *
 *              space is allocated for ASCII strings, but the strings are  *
 *              written as unicode.  As a result, only the first half of   *
 *              the string appears.)  So this is probably not too big of   *
 *              a deal, at least until CHM v4 (MS .lit files), which also  *
 *              incorporate encryption, of some description.               *
 *                                                                         *
 ***************************************************************************/

/***************************************************************************
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
 *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 * Adapted for Wine by Mike McCormack                                      *
 *                                                                         *
 ***************************************************************************/

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wine/unicode.h"

#include "chm_lib.h"
#include "lzx.h"

#define CHM_ACQUIRE_LOCK(a) do {                        \
        EnterCriticalSection(&(a));                     \
    } while(0)
#define CHM_RELEASE_LOCK(a) do {                        \
        LeaveCriticalSection(&(a));                     \
    } while(0)

#define CHM_NULL_FD (INVALID_HANDLE_VALUE)
#define CHM_CLOSE_FILE(fd) CloseHandle((fd))

/*
 * defines related to tuning
 */
#ifndef CHM_MAX_BLOCKS_CACHED
#define CHM_MAX_BLOCKS_CACHED 5
#endif
#define CHM_PARAM_MAX_BLOCKS_CACHED 0

/*
 * architecture specific defines
 *
 * Note: as soon as C99 is more widespread, the below defines should
 * probably just use the C99 sized-int types.
 *
 * The following settings will probably work for many platforms.  The sizes
 * don't have to be exactly correct, but the types must accommodate at least as
 * many bits as they specify.
 */

/* i386, 32-bit, Windows */
typedef BYTE   UChar;
typedef SHORT  Int16;
typedef USHORT UInt16;
typedef LONG   Int32;
typedef DWORD      UInt32;
typedef LONGLONG   Int64;
typedef ULONGLONG  UInt64;

/* utilities for unmarshalling data */
static int _unmarshal_char_array(unsigned char **pData,
                                 unsigned int *pLenRemain,
                                 char *dest,
                                 int count)
{
    if (count <= 0  ||  (unsigned int)count > *pLenRemain)
        return 0;
    memcpy(dest, (*pData), count);
    *pData += count;
    *pLenRemain -= count;
    return 1;
}

static int _unmarshal_uchar_array(unsigned char **pData,
                                  unsigned int *pLenRemain,
                                  unsigned char *dest,
                                  int count)
{
        if (count <= 0  ||  (unsigned int)count > *pLenRemain)
        return 0;
    memcpy(dest, (*pData), count);
    *pData += count;
    *pLenRemain -= count;
    return 1;
}

static int _unmarshal_int32(unsigned char **pData,
                            unsigned int *pLenRemain,
                            Int32 *dest)
{
    if (4 > *pLenRemain)
        return 0;
    *dest = (*pData)[0] | (*pData)[1]<<8 | (*pData)[2]<<16 | (*pData)[3]<<24;
    *pData += 4;
    *pLenRemain -= 4;
    return 1;
}

static int _unmarshal_uint32(unsigned char **pData,
                             unsigned int *pLenRemain,
                             UInt32 *dest)
{
    if (4 > *pLenRemain)
        return 0;
    *dest = (*pData)[0] | (*pData)[1]<<8 | (*pData)[2]<<16 | (*pData)[3]<<24;
    *pData += 4;
    *pLenRemain -= 4;
    return 1;
}

static int _unmarshal_int64(unsigned char **pData,
                            unsigned int *pLenRemain,
                            Int64 *dest)
{
    Int64 temp;
    int i;
    if (8 > *pLenRemain)
        return 0;
    temp=0;
    for(i=8; i>0; i--)
    {
        temp <<= 8;
        temp |= (*pData)[i-1];
    }
    *dest = temp;
    *pData += 8;
    *pLenRemain -= 8;
    return 1;
}

static int _unmarshal_uint64(unsigned char **pData,
                             unsigned int *pLenRemain,
                             UInt64 *dest)
{
    UInt64 temp;
    int i;
    if (8 > *pLenRemain)
        return 0;
    temp=0;
    for(i=8; i>0; i--)
    {
        temp <<= 8;
        temp |= (*pData)[i-1];
    }
    *dest = temp;
    *pData += 8;
    *pLenRemain -= 8;
    return 1;
}

static int _unmarshal_uuid(unsigned char **pData,
                           unsigned int *pDataLen,
                           unsigned char *dest)
{
    return _unmarshal_uchar_array(pData, pDataLen, dest, 16);
}

/* names of sections essential to decompression */
static const WCHAR _CHMU_RESET_TABLE[] = {
':',':','D','a','t','a','S','p','a','c','e','/',
        'S','t','o','r','a','g','e','/',
        'M','S','C','o','m','p','r','e','s','s','e','d','/',
        'T','r','a','n','s','f','o','r','m','/',
        '{','7','F','C','2','8','9','4','0','-','9','D','3','1',
          '-','1','1','D','0','-','9','B','2','7','-',
          '0','0','A','0','C','9','1','E','9','C','7','C','}','/',
        'I','n','s','t','a','n','c','e','D','a','t','a','/',
        'R','e','s','e','t','T','a','b','l','e',0
};
static const WCHAR _CHMU_LZXC_CONTROLDATA[] = {
':',':','D','a','t','a','S','p','a','c','e','/',
        'S','t','o','r','a','g','e','/',
        'M','S','C','o','m','p','r','e','s','s','e','d','/',
        'C','o','n','t','r','o','l','D','a','t','a',0
};
static const WCHAR _CHMU_CONTENT[] = {
':',':','D','a','t','a','S','p','a','c','e','/',
        'S','t','o','r','a','g','e','/',
        'M','S','C','o','m','p','r','e','s','s','e','d','/',
        'C','o','n','t','e','n','t',0
};

/*
 * structures local to this module
 */

/* structure of ITSF headers */
#define _CHM_ITSF_V2_LEN (0x58)
#define _CHM_ITSF_V3_LEN (0x60)
struct chmItsfHeader
{
    char        signature[4];           /*  0 (ITSF) */
    Int32       version;                /*  4 */
    Int32       header_len;             /*  8 */
    Int32       unknown_000c;           /*  c */
    UInt32      last_modified;          /* 10 */
    UInt32      lang_id;                /* 14 */
    UChar       dir_uuid[16];           /* 18 */
    UChar       stream_uuid[16];        /* 28 */
    UInt64      unknown_offset;         /* 38 */
    UInt64      unknown_len;            /* 40 */
    UInt64      dir_offset;             /* 48 */
    UInt64      dir_len;                /* 50 */
    UInt64      data_offset;            /* 58 (Not present before V3) */
}; /* __attribute__ ((aligned (1))); */

static int _unmarshal_itsf_header(unsigned char **pData,
                                  unsigned int *pDataLen,
                                  struct chmItsfHeader *dest)
{
    /* we only know how to deal with the 0x58 and 0x60 byte structures */
    if (*pDataLen != _CHM_ITSF_V2_LEN  &&  *pDataLen != _CHM_ITSF_V3_LEN)
        return 0;

    /* unmarshal common fields */
    _unmarshal_char_array(pData, pDataLen,  dest->signature, 4);
    _unmarshal_int32     (pData, pDataLen, &dest->version);
    _unmarshal_int32     (pData, pDataLen, &dest->header_len);
    _unmarshal_int32     (pData, pDataLen, &dest->unknown_000c);
    _unmarshal_uint32    (pData, pDataLen, &dest->last_modified);
    _unmarshal_uint32    (pData, pDataLen, &dest->lang_id);
    _unmarshal_uuid      (pData, pDataLen,  dest->dir_uuid);
    _unmarshal_uuid      (pData, pDataLen,  dest->stream_uuid);
    _unmarshal_uint64    (pData, pDataLen, &dest->unknown_offset);
    _unmarshal_uint64    (pData, pDataLen, &dest->unknown_len);
    _unmarshal_uint64    (pData, pDataLen, &dest->dir_offset);
    _unmarshal_uint64    (pData, pDataLen, &dest->dir_len);

    /* error check the data */
    /* XXX: should also check UUIDs, probably, though with a version 3 file,
     * current MS tools do not seem to use them.
     */
    if (memcmp(dest->signature, "ITSF", 4) != 0)
        return 0;
    if (dest->version == 2)
    {
        if (dest->header_len < _CHM_ITSF_V2_LEN)
            return 0;
    }
    else if (dest->version == 3)
    {
        if (dest->header_len < _CHM_ITSF_V3_LEN)
            return 0;
    }
    else
        return 0;

    /* now, if we have a V3 structure, unmarshal the rest.
     * otherwise, compute it
     */
    if (dest->version == 3)
    {
        if (*pDataLen != 0)
            _unmarshal_uint64(pData, pDataLen, &dest->data_offset);
        else
            return 0;
    }
    else
        dest->data_offset = dest->dir_offset + dest->dir_len;

    return 1;
}

/* structure of ITSP headers */
#define _CHM_ITSP_V1_LEN (0x54)
struct chmItspHeader
{
    char        signature[4];           /*  0 (ITSP) */
    Int32       version;                /*  4 */
    Int32       header_len;             /*  8 */
    Int32       unknown_000c;           /*  c */
    UInt32      block_len;              /* 10 */
    Int32       blockidx_intvl;         /* 14 */
    Int32       index_depth;            /* 18 */
    Int32       index_root;             /* 1c */
    Int32       index_head;             /* 20 */
    Int32       unknown_0024;           /* 24 */
    UInt32      num_blocks;             /* 28 */
    Int32       unknown_002c;           /* 2c */
    UInt32      lang_id;                /* 30 */
    UChar       system_uuid[16];        /* 34 */
    UChar       unknown_0044[16];       /* 44 */
}; /* __attribute__ ((aligned (1))); */

static int _unmarshal_itsp_header(unsigned char **pData,
                                  unsigned int *pDataLen,
                                  struct chmItspHeader *dest)
{
    /* we only know how to deal with a 0x54 byte structures */
    if (*pDataLen != _CHM_ITSP_V1_LEN)
        return 0;

    /* unmarshal fields */
    _unmarshal_char_array(pData, pDataLen,  dest->signature, 4);
    _unmarshal_int32     (pData, pDataLen, &dest->version);
    _unmarshal_int32     (pData, pDataLen, &dest->header_len);
    _unmarshal_int32     (pData, pDataLen, &dest->unknown_000c);
    _unmarshal_uint32    (pData, pDataLen, &dest->block_len);
    _unmarshal_int32     (pData, pDataLen, &dest->blockidx_intvl);
    _unmarshal_int32     (pData, pDataLen, &dest->index_depth);
    _unmarshal_int32     (pData, pDataLen, &dest->index_root);
    _unmarshal_int32     (pData, pDataLen, &dest->index_head);
    _unmarshal_int32     (pData, pDataLen, &dest->unknown_0024);
    _unmarshal_uint32    (pData, pDataLen, &dest->num_blocks);
    _unmarshal_int32     (pData, pDataLen, &dest->unknown_002c);
    _unmarshal_uint32    (pData, pDataLen, &dest->lang_id);
    _unmarshal_uuid      (pData, pDataLen,  dest->system_uuid);
    _unmarshal_uchar_array(pData, pDataLen, dest->unknown_0044, 16);

    /* error check the data */
    if (memcmp(dest->signature, "ITSP", 4) != 0)
        return 0;
    if (dest->version != 1)
        return 0;
    if (dest->header_len != _CHM_ITSP_V1_LEN)
        return 0;

    return 1;
}

/* structure of PMGL headers */
static const char _chm_pmgl_marker[4] = "PMGL";
#define _CHM_PMGL_LEN (0x14)
struct chmPmglHeader
{
    char        signature[4];           /*  0 (PMGL) */
    UInt32      free_space;             /*  4 */
    UInt32      unknown_0008;           /*  8 */
    Int32       block_prev;             /*  c */
    Int32       block_next;             /* 10 */
}; /* __attribute__ ((aligned (1))); */

static int _unmarshal_pmgl_header(unsigned char **pData,
                                  unsigned int *pDataLen,
                                  struct chmPmglHeader *dest)
{
    /* we only know how to deal with a 0x14 byte structures */
    if (*pDataLen != _CHM_PMGL_LEN)
        return 0;

    /* unmarshal fields */
    _unmarshal_char_array(pData, pDataLen,  dest->signature, 4);
    _unmarshal_uint32    (pData, pDataLen, &dest->free_space);
    _unmarshal_uint32    (pData, pDataLen, &dest->unknown_0008);
    _unmarshal_int32     (pData, pDataLen, &dest->block_prev);
    _unmarshal_int32     (pData, pDataLen, &dest->block_next);

    /* check structure */
    if (memcmp(dest->signature, _chm_pmgl_marker, 4) != 0)
        return 0;

    return 1;
}

/* structure of PMGI headers */
static const char _chm_pmgi_marker[4] = "PMGI";
#define _CHM_PMGI_LEN (0x08)
struct chmPmgiHeader
{
    char        signature[4];           /*  0 (PMGI) */
    UInt32      free_space;             /*  4 */
}; /* __attribute__ ((aligned (1))); */

static int _unmarshal_pmgi_header(unsigned char **pData,
                                  unsigned int *pDataLen,
                                  struct chmPmgiHeader *dest)
{
    /* we only know how to deal with a 0x8 byte structures */
    if (*pDataLen != _CHM_PMGI_LEN)
        return 0;

    /* unmarshal fields */
    _unmarshal_char_array(pData, pDataLen,  dest->signature, 4);
    _unmarshal_uint32    (pData, pDataLen, &dest->free_space);

    /* check structure */
    if (memcmp(dest->signature, _chm_pmgi_marker, 4) != 0)
        return 0;

    return 1;
}

/* structure of LZXC reset table */
#define _CHM_LZXC_RESETTABLE_V1_LEN (0x28)
struct chmLzxcResetTable
{
    UInt32      version;
    UInt32      block_count;
    UInt32      unknown;
    UInt32      table_offset;
    UInt64      uncompressed_len;
    UInt64      compressed_len;
    UInt64      block_len;     
}; /* __attribute__ ((aligned (1))); */

static int _unmarshal_lzxc_reset_table(unsigned char **pData,
                                       unsigned int *pDataLen,
                                       struct chmLzxcResetTable *dest)
{
    /* we only know how to deal with a 0x28 byte structures */
    if (*pDataLen != _CHM_LZXC_RESETTABLE_V1_LEN)
        return 0;

    /* unmarshal fields */
    _unmarshal_uint32    (pData, pDataLen, &dest->version);
    _unmarshal_uint32    (pData, pDataLen, &dest->block_count);
    _unmarshal_uint32    (pData, pDataLen, &dest->unknown);
    _unmarshal_uint32    (pData, pDataLen, &dest->table_offset);
    _unmarshal_uint64    (pData, pDataLen, &dest->uncompressed_len);
    _unmarshal_uint64    (pData, pDataLen, &dest->compressed_len);
    _unmarshal_uint64    (pData, pDataLen, &dest->block_len);

    /* check structure */
    if (dest->version != 2)
        return 0;

    return 1;
}

/* structure of LZXC control data block */
#define _CHM_LZXC_MIN_LEN (0x18)
#define _CHM_LZXC_V2_LEN (0x1c)
struct chmLzxcControlData
{
    UInt32      size;                   /*  0        */
    char        signature[4];           /*  4 (LZXC) */
    UInt32      version;                /*  8        */
    UInt32      resetInterval;          /*  c        */
    UInt32      windowSize;             /* 10        */
    UInt32      windowsPerReset;        /* 14        */
    UInt32      unknown_18;             /* 18        */
};

static int _unmarshal_lzxc_control_data(unsigned char **pData,
                                        unsigned int *pDataLen,
                                        struct chmLzxcControlData *dest)
{
    /* we want at least 0x18 bytes */
    if (*pDataLen < _CHM_LZXC_MIN_LEN)
        return 0;

    /* unmarshal fields */
    _unmarshal_uint32    (pData, pDataLen, &dest->size);
    _unmarshal_char_array(pData, pDataLen,  dest->signature, 4);
    _unmarshal_uint32    (pData, pDataLen, &dest->version);
    _unmarshal_uint32    (pData, pDataLen, &dest->resetInterval);
    _unmarshal_uint32    (pData, pDataLen, &dest->windowSize);
    _unmarshal_uint32    (pData, pDataLen, &dest->windowsPerReset);

    if (*pDataLen >= _CHM_LZXC_V2_LEN)
        _unmarshal_uint32    (pData, pDataLen, &dest->unknown_18);
    else
        dest->unknown_18 = 0;

    if (dest->version == 2)
    {
        dest->resetInterval *= 0x8000;
        dest->windowSize *= 0x8000;
    }
    if (dest->windowSize == 0  ||  dest->resetInterval == 0)
        return 0;

    /* for now, only support resetInterval a multiple of windowSize/2 */
    if (dest->windowSize == 1)
        return 0;
    if ((dest->resetInterval % (dest->windowSize/2)) != 0)
        return 0;

    /* check structure */
    if (memcmp(dest->signature, "LZXC", 4) != 0)
        return 0;

    return 1;
}

/* the structure used for chm file handles */
struct chmFile
{
    HANDLE              fd;

    CRITICAL_SECTION    mutex;
    CRITICAL_SECTION    lzx_mutex;
    CRITICAL_SECTION    cache_mutex;

    UInt64              dir_offset;
    UInt64              dir_len;    
    UInt64              data_offset;
    Int32               index_root;
    Int32               index_head;
    UInt32              block_len;     

    UInt64              span;
    struct chmUnitInfo  rt_unit;
    struct chmUnitInfo  cn_unit;
    struct chmLzxcResetTable reset_table;

    /* LZX control data */
    int                 compression_enabled;
    UInt32              window_size;
    UInt32              reset_interval;
    UInt32              reset_blkcount;

    /* decompressor state */
    struct LZXstate    *lzx_state;
    int                 lzx_last_block;

    /* cache for decompressed blocks */
    UChar             **cache_blocks;
    Int64              *cache_block_indices;
    Int32               cache_num_blocks;
};

/*
 * utility functions local to this module
 */

/* utility function to handle differences between {pread,read}(64)? */
static Int64 _chm_fetch_bytes(struct chmFile *h,
                              UChar *buf,
                              UInt64 os,
                              Int64 len)
{
    Int64 readLen=0;
    if (h->fd  ==  CHM_NULL_FD)
        return readLen;

    CHM_ACQUIRE_LOCK(h->mutex);
    /* NOTE: this might be better done with CreateFileMapping, et cetera... */
    {
        LARGE_INTEGER old_pos, new_pos;
        DWORD actualLen=0;

        /* awkward Win32 Seek/Tell */
        new_pos.QuadPart = 0;
        SetFilePointerEx( h->fd, new_pos, &old_pos, FILE_CURRENT );
        new_pos.QuadPart = os;
        SetFilePointerEx( h->fd, new_pos, NULL, FILE_BEGIN );

        /* read the data */
        if (ReadFile(h->fd,
                     buf,
                     (DWORD)len,
                     &actualLen,
                     NULL))
            readLen = actualLen;
        else
            readLen = 0;

        /* restore original position */
        SetFilePointerEx( h->fd, old_pos, NULL, FILE_BEGIN );
    }
    CHM_RELEASE_LOCK(h->mutex);
    return readLen;
}

/*
 * set a parameter on the file handle.
 * valid parameter types:
 *          CHM_PARAM_MAX_BLOCKS_CACHED:
 *                 how many decompressed blocks should be cached?  A simple
 *                 caching scheme is used, wherein the index of the block is
 *                 used as a hash value, and hash collision results in the
 *                 invalidation of the previously cached block.
 */
static void chm_set_param(struct chmFile *h,
                          int paramType,
                          int paramVal)
{
    switch (paramType)
    {
        case CHM_PARAM_MAX_BLOCKS_CACHED:
            CHM_ACQUIRE_LOCK(h->cache_mutex);
            if (paramVal != h->cache_num_blocks)
            {
                UChar **newBlocks;
                Int64 *newIndices;
                int     i;

                /* allocate new cached blocks */
                newBlocks = HeapAlloc(GetProcessHeap(), 0, paramVal * sizeof (UChar *));
                newIndices = HeapAlloc(GetProcessHeap(), 0, paramVal * sizeof (UInt64));
                for (i=0; i<paramVal; i++)
                {
                    newBlocks[i] = NULL;
                    newIndices[i] = 0;
                }

                /* re-distribute old cached blocks */
                if (h->cache_blocks)
                {
                    for (i=0; i<h->cache_num_blocks; i++)
                    {
                        int newSlot = (int)(h->cache_block_indices[i] % paramVal);

                        if (h->cache_blocks[i])
                        {
                            /* in case of collision, destroy newcomer */
                            if (newBlocks[newSlot])
                            {
                                HeapFree(GetProcessHeap(), 0, h->cache_blocks[i]);
                                h->cache_blocks[i] = NULL;
                            }
                            else
                            {
                                newBlocks[newSlot] = h->cache_blocks[i];
                                newIndices[newSlot] =
                                            h->cache_block_indices[i];
                            }
                        }
                    }

                    HeapFree(GetProcessHeap(), 0, h->cache_blocks);
                    HeapFree(GetProcessHeap(), 0, h->cache_block_indices);
                }

                /* now, set new values */
                h->cache_blocks = newBlocks;
                h->cache_block_indices = newIndices;
                h->cache_num_blocks = paramVal;
            }
            CHM_RELEASE_LOCK(h->cache_mutex);
            break;

        default:
            break;
    }
}

/* open an ITS archive */
struct chmFile *chm_openW(const WCHAR *filename)
{
    unsigned char               sbuffer[256];
    unsigned int                sremain;
    unsigned char              *sbufpos;
    struct chmFile             *newHandle=NULL;
    struct chmItsfHeader        itsfHeader;
    struct chmItspHeader        itspHeader;
#if 0
    struct chmUnitInfo          uiSpan;
#endif
    struct chmUnitInfo          uiLzxc;
    struct chmLzxcControlData   ctlData;

    /* allocate handle */
    newHandle = HeapAlloc(GetProcessHeap(), 0, sizeof(struct chmFile));
    newHandle->fd = CHM_NULL_FD;
    newHandle->lzx_state = NULL;
    newHandle->cache_blocks = NULL;
    newHandle->cache_block_indices = NULL;
    newHandle->cache_num_blocks = 0;

    /* open file */
    if ((newHandle->fd=CreateFileW(filename,
                                   GENERIC_READ,
                                   FILE_SHARE_READ,
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL)) == CHM_NULL_FD)
    {
        HeapFree(GetProcessHeap(), 0, newHandle);
        return NULL;
    }

    /* initialize mutexes, if needed */
    InitializeCriticalSection(&newHandle->mutex);
    newHandle->mutex.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": chmFile.mutex");
    InitializeCriticalSection(&newHandle->lzx_mutex);
    newHandle->lzx_mutex.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": chmFile.lzx_mutex");
    InitializeCriticalSection(&newHandle->cache_mutex);
    newHandle->cache_mutex.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": chmFile.cache_mutex");

    /* read and verify header */
    sremain = _CHM_ITSF_V3_LEN;
    sbufpos = sbuffer;
    if (_chm_fetch_bytes(newHandle, sbuffer, 0, sremain) != sremain    ||
        !_unmarshal_itsf_header(&sbufpos, &sremain, &itsfHeader))
    {
        chm_close(newHandle);
        return NULL;
    }

    /* stash important values from header */
    newHandle->dir_offset  = itsfHeader.dir_offset;
    newHandle->dir_len     = itsfHeader.dir_len;
    newHandle->data_offset = itsfHeader.data_offset;

    /* now, read and verify the directory header chunk */
    sremain = _CHM_ITSP_V1_LEN;
    sbufpos = sbuffer;
    if (_chm_fetch_bytes(newHandle, sbuffer,
                         itsfHeader.dir_offset, sremain) != sremain    ||
        !_unmarshal_itsp_header(&sbufpos, &sremain, &itspHeader))
    {
        chm_close(newHandle);
        return NULL;
    }

    /* grab essential information from ITSP header */
    newHandle->dir_offset += itspHeader.header_len;
    newHandle->dir_len    -= itspHeader.header_len;
    newHandle->index_root  = itspHeader.index_root;
    newHandle->index_head  = itspHeader.index_head;
    newHandle->block_len   = itspHeader.block_len;

    /* if the index root is -1, this means we don't have any PMGI blocks.
     * as a result, we must use the sole PMGL block as the index root
     */
    if (newHandle->index_root == -1)
        newHandle->index_root = newHandle->index_head;

    /* initialize cache */
    chm_set_param(newHandle, CHM_PARAM_MAX_BLOCKS_CACHED,
                  CHM_MAX_BLOCKS_CACHED);

    /* By default, compression is enabled. */
    newHandle->compression_enabled = 1;

    /* prefetch most commonly needed unit infos */
    if (CHM_RESOLVE_SUCCESS != chm_resolve_object(newHandle,
                                                  _CHMU_RESET_TABLE,
                                                  &newHandle->rt_unit)    ||
        newHandle->rt_unit.space == CHM_COMPRESSED                        ||
        CHM_RESOLVE_SUCCESS != chm_resolve_object(newHandle,
                                                  _CHMU_CONTENT,
                                                  &newHandle->cn_unit)    ||
        newHandle->cn_unit.space == CHM_COMPRESSED                        ||
        CHM_RESOLVE_SUCCESS != chm_resolve_object(newHandle,
                                                  _CHMU_LZXC_CONTROLDATA,
                                                  &uiLzxc)                ||
        uiLzxc.space == CHM_COMPRESSED)
    {
        newHandle->compression_enabled = 0;
    }

    /* read reset table info */
    if (newHandle->compression_enabled)
    {
        sremain = _CHM_LZXC_RESETTABLE_V1_LEN;
        sbufpos = sbuffer;
        if (chm_retrieve_object(newHandle, &newHandle->rt_unit, sbuffer,
                                0, sremain) != sremain                        ||
            !_unmarshal_lzxc_reset_table(&sbufpos, &sremain,
                                         &newHandle->reset_table))
        {
            newHandle->compression_enabled = 0;
        }
    }

    /* read control data */
    if (newHandle->compression_enabled)
    {
        sremain = (unsigned long)uiLzxc.length;
        sbufpos = sbuffer;
        if (chm_retrieve_object(newHandle, &uiLzxc, sbuffer,
                                0, sremain) != sremain                       ||
            !_unmarshal_lzxc_control_data(&sbufpos, &sremain,
                                          &ctlData))
        {
            newHandle->compression_enabled = 0;
        }

        newHandle->window_size = ctlData.windowSize;
        newHandle->reset_interval = ctlData.resetInterval;

/* Jed, Mon Jun 28: Experimentally, it appears that the reset block count */
/*       must be multiplied by this formerly unknown ctrl data field in   */
/*       order to decompress some files.                                  */
#if 0
        newHandle->reset_blkcount = newHandle->reset_interval /
                    (newHandle->window_size / 2);
#else
        newHandle->reset_blkcount = newHandle->reset_interval    /
                                    (newHandle->window_size / 2) *
                                    ctlData.windowsPerReset;
#endif
    }

    return newHandle;
}

/* close an ITS archive */
void chm_close(struct chmFile *h)
{
    if (h != NULL)
    {
        if (h->fd != CHM_NULL_FD)
            CHM_CLOSE_FILE(h->fd);
        h->fd = CHM_NULL_FD;

        h->mutex.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&h->mutex);
        h->lzx_mutex.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&h->lzx_mutex);
        h->cache_mutex.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&h->cache_mutex);

        if (h->lzx_state)
            LZXteardown(h->lzx_state);
        h->lzx_state = NULL;

        if (h->cache_blocks)
        {
            int i;
            for (i=0; i<h->cache_num_blocks; i++)
            {
                HeapFree(GetProcessHeap(), 0, h->cache_blocks[i]);
            }
            HeapFree(GetProcessHeap(), 0, h->cache_blocks);
            h->cache_blocks = NULL;
        }

        HeapFree(GetProcessHeap(), 0, h->cache_block_indices);
        h->cache_block_indices = NULL;

        HeapFree(GetProcessHeap(), 0, h);
    }
}

/*
 * helper methods for chm_resolve_object
 */

/* skip a compressed dword */
static void _chm_skip_cword(UChar **pEntry)
{
    while (*(*pEntry)++ >= 0x80)
        ;
}

/* skip the data from a PMGL entry */
static void _chm_skip_PMGL_entry_data(UChar **pEntry)
{
    _chm_skip_cword(pEntry);
    _chm_skip_cword(pEntry);
    _chm_skip_cword(pEntry);
}

/* parse a compressed dword */
static UInt64 _chm_parse_cword(UChar **pEntry)
{
    UInt64 accum = 0;
    UChar temp;
    while ((temp=*(*pEntry)++) >= 0x80)
    {
        accum <<= 7;
        accum += temp & 0x7f;
    }

    return (accum << 7) + temp;
}

/* parse a utf-8 string into an ASCII char buffer */
static int _chm_parse_UTF8(UChar **pEntry, UInt64 count, WCHAR *path)
{
    /* MJM - Modified to return real Unicode strings */ 
    while (count != 0)
    {
        *path++ = (*(*pEntry)++);
        --count;
    }

    *path = '\0';
    return 1;
}

/* parse a PMGL entry into a chmUnitInfo struct; return 1 on success. */
static int _chm_parse_PMGL_entry(UChar **pEntry, struct chmUnitInfo *ui)
{
    UInt64 strLen;

    /* parse str len */
    strLen = _chm_parse_cword(pEntry);
    if (strLen > CHM_MAX_PATHLEN)
        return 0;

    /* parse path */
    if (! _chm_parse_UTF8(pEntry, strLen, ui->path))
        return 0;

    /* parse info */
    ui->space  = (int)_chm_parse_cword(pEntry);
    ui->start  = _chm_parse_cword(pEntry);
    ui->length = _chm_parse_cword(pEntry);
    return 1;
}

/* find an exact entry in PMGL; return NULL if we fail */
static UChar *_chm_find_in_PMGL(UChar *page_buf,
                         UInt32 block_len,
                         const WCHAR *objPath)
{
    /* XXX: modify this to do a binary search using the nice index structure
     *      that is provided for us.
     */
    struct chmPmglHeader header;
    UInt32 hremain;
    UChar *end;
    UChar *cur;
    UChar *temp;
    UInt64 strLen;
    WCHAR buffer[CHM_MAX_PATHLEN+1];

    /* figure out where to start and end */
    cur = page_buf;
    hremain = _CHM_PMGL_LEN;
    if (! _unmarshal_pmgl_header(&cur, &hremain, &header))
        return NULL;
    end = page_buf + block_len - (header.free_space);

    /* now, scan progressively */
    while (cur < end)
    {
        /* grab the name */
        temp = cur;
        strLen = _chm_parse_cword(&cur);
        if (! _chm_parse_UTF8(&cur, strLen, buffer))
            return NULL;

        /* check if it is the right name */
        if (! strcmpiW(buffer, objPath))
            return temp;

        _chm_skip_PMGL_entry_data(&cur);
    }

    return NULL;
}

/* find which block should be searched next for the entry; -1 if no block */
static Int32 _chm_find_in_PMGI(UChar *page_buf,
                        UInt32 block_len,
                        const WCHAR *objPath)
{
    /* XXX: modify this to do a binary search using the nice index structure
     *      that is provided for us
     */
    struct chmPmgiHeader header;
    UInt32 hremain;
    int page=-1;
    UChar *end;
    UChar *cur;
    UInt64 strLen;
    WCHAR buffer[CHM_MAX_PATHLEN+1];

    /* figure out where to start and end */
    cur = page_buf;
    hremain = _CHM_PMGI_LEN;
    if (! _unmarshal_pmgi_header(&cur, &hremain, &header))
        return -1;
    end = page_buf + block_len - (header.free_space);

    /* now, scan progressively */
    while (cur < end)
    {
        /* grab the name */
        strLen = _chm_parse_cword(&cur);
        if (! _chm_parse_UTF8(&cur, strLen, buffer))
            return -1;

        /* check if it is the right name */
        if (strcmpiW(buffer, objPath) > 0)
            return page;

        /* load next value for path */
        page = (int)_chm_parse_cword(&cur);
    }

    return page;
}

/* resolve a particular object from the archive */
int chm_resolve_object(struct chmFile *h,
                       const WCHAR *objPath,
                       struct chmUnitInfo *ui)
{
    /*
     * XXX: implement caching scheme for dir pages
     */

    Int32 curPage;

    /* buffer to hold whatever page we're looking at */
    UChar *page_buf = HeapAlloc(GetProcessHeap(), 0, h->block_len);

    /* starting page */
    curPage = h->index_root;

    /* until we have either returned or given up */
    while (curPage != -1)
    {

        /* try to fetch the index page */
        if (_chm_fetch_bytes(h, page_buf,
                             h->dir_offset + (UInt64)curPage*h->block_len,
                             h->block_len) != h->block_len)
	{
	    HeapFree(GetProcessHeap(), 0, page_buf);
            return CHM_RESOLVE_FAILURE;
	}

        /* now, if it is a leaf node: */
        if (memcmp(page_buf, _chm_pmgl_marker, 4) == 0)
        {
            /* scan block */
            UChar *pEntry = _chm_find_in_PMGL(page_buf,
                                              h->block_len,
                                              objPath);
            if (pEntry == NULL)
            {
	        HeapFree(GetProcessHeap(), 0, page_buf);
                return CHM_RESOLVE_FAILURE;
            }

            /* parse entry and return */
            _chm_parse_PMGL_entry(&pEntry, ui);
	    HeapFree(GetProcessHeap(), 0, page_buf);
            return CHM_RESOLVE_SUCCESS;
        }

        /* else, if it is a branch node: */
        else if (memcmp(page_buf, _chm_pmgi_marker, 4) == 0)
            curPage = _chm_find_in_PMGI(page_buf, h->block_len, objPath);

        /* else, we are confused.  give up. */
        else
        {
	    HeapFree(GetProcessHeap(), 0, page_buf);
            return CHM_RESOLVE_FAILURE;
        }
    }

    /* didn't find anything.  fail. */
    HeapFree(GetProcessHeap(), 0, page_buf);
    return CHM_RESOLVE_FAILURE;
}

/*
 * utility methods for dealing with compressed data
 */

/* get the bounds of a compressed block.  return 0 on failure */
static int _chm_get_cmpblock_bounds(struct chmFile *h,
                             UInt64 block,
                             UInt64 *start,
                             Int64 *len)
{
    UChar buffer[8], *dummy;
    UInt32 remain;

    /* for all but the last block, use the reset table */
    if (block < h->reset_table.block_count-1)
    {
        /* unpack the start address */
        dummy = buffer;
        remain = 8;
        if (_chm_fetch_bytes(h, buffer,
                             h->data_offset
                                + h->rt_unit.start
                                + h->reset_table.table_offset
                                + block*8,
                             remain) != remain                            ||
            !_unmarshal_uint64(&dummy, &remain, start))
            return 0;

        /* unpack the end address */
        dummy = buffer;
        remain = 8;
        if (_chm_fetch_bytes(h, buffer,
                             h->data_offset
                                + h->rt_unit.start
                                + h->reset_table.table_offset
                                + block*8 + 8,
                         remain) != remain                                ||
            !_unmarshal_int64(&dummy, &remain, len))
            return 0;
    }

    /* for the last block, use the span in addition to the reset table */
    else
    {
        /* unpack the start address */
        dummy = buffer;
        remain = 8;
        if (_chm_fetch_bytes(h, buffer,
                             h->data_offset
                                + h->rt_unit.start
                                + h->reset_table.table_offset
                                + block*8,
                             remain) != remain                            ||
            !_unmarshal_uint64(&dummy, &remain, start))
            return 0;

        *len = h->reset_table.compressed_len;
    }

    /* compute the length and absolute start address */
    *len -= *start;
    *start += h->data_offset + h->cn_unit.start;

    return 1;
}

/* decompress the block.  must have lzx_mutex. */
static Int64 _chm_decompress_block(struct chmFile *h,
                                   UInt64 block,
                                   UChar **ubuffer)
{
    UChar *cbuffer = HeapAlloc( GetProcessHeap(), 0,
                              ((unsigned int)h->reset_table.block_len + 6144));
    UInt64 cmpStart;                                    /* compressed start  */
    Int64 cmpLen;                                       /* compressed len    */
    int indexSlot;                                      /* cache index slot  */
    UChar *lbuffer;                                     /* local buffer ptr  */
    UInt32 blockAlign = (UInt32)(block % h->reset_blkcount); /* reset interval align */
    UInt32 i;                                           /* local loop index  */

    /* let the caching system pull its weight! */
    if (block - blockAlign <= h->lzx_last_block  &&
        block              >= h->lzx_last_block)
        blockAlign = (block - h->lzx_last_block);

    /* check if we need previous blocks */
    if (blockAlign != 0)
    {
        /* fetch all required previous blocks since last reset */
        for (i = blockAlign; i > 0; i--)
        {
            UInt32 curBlockIdx = block - i;

            /* check if we most recently decompressed the previous block */
            if (h->lzx_last_block != curBlockIdx)
            {
                if ((curBlockIdx % h->reset_blkcount) == 0)
                {
#ifdef CHM_DEBUG
                    fprintf(stderr, "***RESET (1)***\n");
#endif
                    LZXreset(h->lzx_state);
                }

                indexSlot = (int)((curBlockIdx) % h->cache_num_blocks);
                h->cache_block_indices[indexSlot] = curBlockIdx;
                if (! h->cache_blocks[indexSlot])
                    h->cache_blocks[indexSlot] =
                      HeapAlloc(GetProcessHeap(), 0,
                                (unsigned int)(h->reset_table.block_len));
                lbuffer = h->cache_blocks[indexSlot];

                /* decompress the previous block */
#ifdef CHM_DEBUG
                fprintf(stderr, "Decompressing block #%4d (EXTRA)\n", curBlockIdx);
#endif
                if (!_chm_get_cmpblock_bounds(h, curBlockIdx, &cmpStart, &cmpLen) ||
                    _chm_fetch_bytes(h, cbuffer, cmpStart, cmpLen) != cmpLen      ||
                    LZXdecompress(h->lzx_state, cbuffer, lbuffer, (int)cmpLen,
                                  (int)h->reset_table.block_len) != DECR_OK)
                {
#ifdef CHM_DEBUG
                    fprintf(stderr, "   (DECOMPRESS FAILED!)\n");
#endif
                    HeapFree(GetProcessHeap(), 0, cbuffer);
                    return 0;
                }

                h->lzx_last_block = (int)curBlockIdx;
            }
        }
    }
    else
    {
        if ((block % h->reset_blkcount) == 0)
        {
#ifdef CHM_DEBUG
            fprintf(stderr, "***RESET (2)***\n");
#endif
            LZXreset(h->lzx_state);
        }
    }

    /* allocate slot in cache */
    indexSlot = (int)(block % h->cache_num_blocks);
    h->cache_block_indices[indexSlot] = block;
    if (! h->cache_blocks[indexSlot])
        h->cache_blocks[indexSlot] =
          HeapAlloc(GetProcessHeap(), 0, ((unsigned int)h->reset_table.block_len));
    lbuffer = h->cache_blocks[indexSlot];
    *ubuffer = lbuffer;

    /* decompress the block we actually want */
#ifdef CHM_DEBUG
    fprintf(stderr, "Decompressing block #%4d (REAL )\n", block);
#endif
    if (! _chm_get_cmpblock_bounds(h, block, &cmpStart, &cmpLen)          ||
        _chm_fetch_bytes(h, cbuffer, cmpStart, cmpLen) != cmpLen          ||
        LZXdecompress(h->lzx_state, cbuffer, lbuffer, (int)cmpLen,
                      (int)h->reset_table.block_len) != DECR_OK)
    {
#ifdef CHM_DEBUG
        fprintf(stderr, "   (DECOMPRESS FAILED!)\n");
#endif
        HeapFree(GetProcessHeap(), 0, cbuffer);
        return 0;
    }
    h->lzx_last_block = (int)block;

    /* XXX: modify LZX routines to return the length of the data they
     * decompressed and return that instead, for an extra sanity check.
     */
    HeapFree(GetProcessHeap(), 0, cbuffer);
    return h->reset_table.block_len;
}

/* grab a region from a compressed block */
static Int64 _chm_decompress_region(struct chmFile *h,
                                    UChar *buf,
                                    UInt64 start,
                                    Int64 len)
{
    UInt64 nBlock, nOffset;
    UInt64 nLen;
    UInt64 gotLen;
    UChar *ubuffer = NULL;

        if (len <= 0)
                return 0;

    /* figure out what we need to read */
    nBlock = start / h->reset_table.block_len;
    nOffset = start % h->reset_table.block_len;
    nLen = len;
    if (nLen > (h->reset_table.block_len - nOffset))
        nLen = h->reset_table.block_len - nOffset;

    /* if block is cached, return data from it. */
    CHM_ACQUIRE_LOCK(h->lzx_mutex);
    CHM_ACQUIRE_LOCK(h->cache_mutex);
    if (h->cache_block_indices[nBlock % h->cache_num_blocks] == nBlock    &&
        h->cache_blocks[nBlock % h->cache_num_blocks] != NULL)
    {
        memcpy(buf,
               h->cache_blocks[nBlock % h->cache_num_blocks] + nOffset,
               (unsigned int)nLen);
        CHM_RELEASE_LOCK(h->cache_mutex);
        CHM_RELEASE_LOCK(h->lzx_mutex);
        return nLen;
    }
    CHM_RELEASE_LOCK(h->cache_mutex);

    /* data request not satisfied, so... start up the decompressor machine */
    if (! h->lzx_state)
    {
        int window_size = ffs(h->window_size) - 1;
        h->lzx_last_block = -1;
        h->lzx_state = LZXinit(window_size);
    }

    /* decompress some data */
    gotLen = _chm_decompress_block(h, nBlock, &ubuffer);
    if (gotLen < nLen)
        nLen = gotLen;
    memcpy(buf, ubuffer+nOffset, (unsigned int)nLen);
    CHM_RELEASE_LOCK(h->lzx_mutex);
    return nLen;
}

/* retrieve (part of) an object */
LONGINT64 chm_retrieve_object(struct chmFile *h,
                               struct chmUnitInfo *ui,
                               unsigned char *buf,
                               LONGUINT64 addr,
                               LONGINT64 len)
{
    /* must be valid file handle */
    if (h == NULL)
        return 0;

    /* starting address must be in correct range */
    if (addr >= ui->length)
        return 0;

    /* clip length */
    if (addr + len > ui->length)
        len = ui->length - addr;

    /* if the file is uncompressed, it's simple */
    if (ui->space == CHM_UNCOMPRESSED)
    {
        /* read data */
        return _chm_fetch_bytes(h,
                                buf,
                                h->data_offset + ui->start + addr,
                                len);
    }

    /* else if the file is compressed, it's a little trickier */
    else /* ui->space == CHM_COMPRESSED */
    {
        Int64 swath=0, total=0;

        /* if compression is not enabled for this file... */
        if (! h->compression_enabled)
            return total;

        do {

            /* swill another mouthful */
            swath = _chm_decompress_region(h, buf, ui->start + addr, len);

            /* if we didn't get any... */
            if (swath == 0)
                return total;

            /* update stats */
            total += swath;
            len -= swath;
            addr += swath;
            buf += swath;

        } while (len != 0);

        return total;
    }
}

int chm_enumerate_dir(struct chmFile *h,
                      const WCHAR *prefix,
                      int what,
                      CHM_ENUMERATOR e,
                      void *context)
{
    /*
     * XXX: do this efficiently (i.e. using the tree index)
     */

    Int32 curPage;

    /* buffer to hold whatever page we're looking at */
    UChar *page_buf = HeapAlloc(GetProcessHeap(), 0, h->block_len);
    struct chmPmglHeader header;
    UChar *end;
    UChar *cur;
    unsigned int lenRemain;

    /* set to 1 once we've started */
    int it_has_begun=0;

    /* the current ui */
    struct chmUnitInfo ui;
    int flag;
    UInt64 ui_path_len;

    /* the length of the prefix */
    WCHAR prefixRectified[CHM_MAX_PATHLEN+1];
    int prefixLen;
    WCHAR lastPath[CHM_MAX_PATHLEN];
    int lastPathLen;

    /* starting page */
    curPage = h->index_head;

    /* initialize pathname state */
    lstrcpynW(prefixRectified, prefix, CHM_MAX_PATHLEN);
    prefixLen = strlenW(prefixRectified);
    if (prefixLen != 0)
    {
        if (prefixRectified[prefixLen-1] != '/')
        {
            prefixRectified[prefixLen] = '/';
            prefixRectified[prefixLen+1] = '\0';
            ++prefixLen;
        }
    }
    lastPath[0] = '\0';
    lastPathLen = -1;

    /* until we have either returned or given up */
    while (curPage != -1)
    {

        /* try to fetch the index page */
        if (_chm_fetch_bytes(h,
                             page_buf,
                             h->dir_offset + (UInt64)curPage*h->block_len,
                             h->block_len) != h->block_len)
        {
            HeapFree(GetProcessHeap(), 0, page_buf);
            return 0;
        }

        /* figure out start and end for this page */
        cur = page_buf;
        lenRemain = _CHM_PMGL_LEN;
        if (! _unmarshal_pmgl_header(&cur, &lenRemain, &header))
        {
            HeapFree(GetProcessHeap(), 0, page_buf);
            return 0;
        }
        end = page_buf + h->block_len - (header.free_space);

        /* loop over this page */
        while (cur < end)
        {
            if (! _chm_parse_PMGL_entry(&cur, &ui))
            {
                HeapFree(GetProcessHeap(), 0, page_buf);
                return 0;
            }

            /* check if we should start */
            if (! it_has_begun)
            {
                if (ui.length == 0  &&  strncmpiW(ui.path, prefixRectified, prefixLen) == 0)
                    it_has_begun = 1;
                else
                    continue;

                if (ui.path[prefixLen] == '\0')
                    continue;
            }

            /* check if we should stop */
            else
            {
                if (strncmpiW(ui.path, prefixRectified, prefixLen) != 0)
                {
                    HeapFree(GetProcessHeap(), 0, page_buf);
                    return 1;
                }
            }

            /* check if we should include this path */
            if (lastPathLen != -1)
            {
                if (strncmpiW(ui.path, lastPath, lastPathLen) == 0)
                    continue;
            }
            strcpyW(lastPath, ui.path);
            lastPathLen = strlenW(lastPath);

            /* get the length of the path */
            ui_path_len = strlenW(ui.path)-1;

            /* check for DIRS */
            if (ui.path[ui_path_len] == '/'  &&  !(what & CHM_ENUMERATE_DIRS))
                continue;

            /* check for FILES */
            if (ui.path[ui_path_len] != '/'  &&  !(what & CHM_ENUMERATE_FILES))
                continue;

            /* check for NORMAL vs. META */
            if (ui.path[0] == '/')
            {

                /* check for NORMAL vs. SPECIAL */
                if (ui.path[1] == '#'  ||  ui.path[1] == '$')
                    flag = CHM_ENUMERATE_SPECIAL;
                else
                    flag = CHM_ENUMERATE_NORMAL;
            }
            else
                flag = CHM_ENUMERATE_META;
            if (! (what & flag))
                continue;

            /* call the enumerator */
            {
                int status = (*e)(h, &ui, context);
                switch (status)
                {
                    case CHM_ENUMERATOR_FAILURE:
                        HeapFree(GetProcessHeap(), 0, page_buf);
                        return 0;
                    case CHM_ENUMERATOR_CONTINUE:
                        break;
                    case CHM_ENUMERATOR_SUCCESS:
                        HeapFree(GetProcessHeap(), 0, page_buf);
                        return 1;
                    default:
                        break;
                }
            }
        }

        /* advance to next page */
        curPage = header.block_next;
    }

    HeapFree(GetProcessHeap(), 0, page_buf);
    return 1;
}
