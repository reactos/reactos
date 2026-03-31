/*
 * File Compression Interface
 *
 * Copyright 2002 Patrik Stridvall
 * Copyright 2005 Gerold Jens Wucherpfennig
 * Copyright 2011 Alexandre Julliard
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

There is still some work to be done:

- unknown behaviour if files>=2GB or cabinet >=4GB
- check if the maximum size for a cabinet is too small to store any data
- call pfnfcignc on exactly the same position as MS FCIAddFile in every case

*/

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <zlib.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winternl.h"
#include "fci.h"
#include "cabinet.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cabinet);

#ifdef WORDS_BIGENDIAN
#define fci_endian_ulong(x) RtlUlongByteSwap(x)
#define fci_endian_uword(x) RtlUshortByteSwap(x)
#else
#define fci_endian_ulong(x) (x)
#define fci_endian_uword(x) (x)
#endif


typedef struct {
  cab_UBYTE signature[4]; /* !CAB for unfinished cabinets else MSCF */
  cab_ULONG reserved1;
  cab_ULONG cbCabinet;    /*  size of the cabinet file in bytes*/
  cab_ULONG reserved2;
  cab_ULONG coffFiles;    /* offset to first CFFILE section */
  cab_ULONG reserved3;
  cab_UBYTE versionMinor; /* 3 */
  cab_UBYTE versionMajor; /* 1 */
  cab_UWORD cFolders;     /* number of CFFOLDER entries in the cabinet*/
  cab_UWORD cFiles;       /* number of CFFILE entries in the cabinet*/
  cab_UWORD flags;        /* 1=prev cab, 2=next cabinet, 4=reserved sections*/
  cab_UWORD setID;        /* identification number of all cabinets in a set*/
  cab_UWORD iCabinet;     /* number of the cabinet in a set */
  /* additional area if "flags" were set*/
} CFHEADER; /* minimum 36 bytes */

typedef struct {
  cab_ULONG coffCabStart; /* offset to the folder's first CFDATA section */
  cab_UWORD cCFData;      /* number of this folder's CFDATA sections */
  cab_UWORD typeCompress; /* compression type of data in CFDATA section*/
  /* additional area if reserve flag was set */
} CFFOLDER; /* minimum 8 bytes */

typedef struct {
  cab_ULONG cbFile;          /* size of the uncompressed file in bytes */
  cab_ULONG uoffFolderStart; /* offset of the uncompressed file in the folder */
  cab_UWORD iFolder;         /* number of folder in the cabinet 0=first  */
                             /* for special values see below this structure*/
  cab_UWORD date;            /* last modification date*/
  cab_UWORD time;            /* last modification time*/
  cab_UWORD attribs;         /* DOS fat attributes and UTF indicator */
  /* ... and a C string with the name of the file */
} CFFILE; /* 16 bytes + name of file */


typedef struct {
  cab_ULONG csum;          /* checksum of this entry*/
  cab_UWORD cbData;        /* number of compressed bytes  */
  cab_UWORD cbUncomp;      /* number of bytes when data is uncompressed */
  /* optional reserved area */
  /* compressed data */
} CFDATA;

struct temp_file
{
    INT_PTR   handle;
    char      name[CB_MAX_FILENAME];
};

struct folder
{
    struct list      entry;
    struct list      files_list;
    struct list      blocks_list;
    struct temp_file data;
    cab_ULONG        data_start;
    cab_UWORD        data_count;
    TCOMP            compression;
};

struct file
{
    struct list entry;
    cab_ULONG   size;    /* uncompressed size */
    cab_ULONG   offset;  /* offset in folder */
    cab_UWORD   folder;  /* index of folder */
    cab_UWORD   date;
    cab_UWORD   time;
    cab_UWORD   attribs;
    char        name[1];
};

struct data_block
{
    struct list entry;
    cab_UWORD   compressed;
    cab_UWORD   uncompressed;
};

typedef struct FCI_Int
{
  unsigned int       magic;
  PERF               perf;
  PFNFCIFILEPLACED   fileplaced;
  PFNFCIALLOC        alloc;
  PFNFCIFREE         free;
  PFNFCIOPEN         open;
  PFNFCIREAD         read;
  PFNFCIWRITE        write;
  PFNFCICLOSE        close;
  PFNFCISEEK         seek;
  PFNFCIDELETE       delete;
  PFNFCIGETTEMPFILE  gettemp;
  CCAB               ccab;
  PCCAB              pccab;
  BOOL               fPrevCab;
  BOOL               fNextCab;
  BOOL               fSplitFolder;
  cab_ULONG          statusFolderCopied;
  cab_ULONG          statusFolderTotal;
  BOOL               fGetNextCabInVain;
  void               *pv;
  char               szPrevCab[CB_MAX_CABINET_NAME]; /* previous cabinet name */
  char               szPrevDisk[CB_MAX_DISK_NAME];   /* disk name of previous cabinet */
  unsigned char      data_in[CAB_BLOCKMAX];          /* uncompressed data blocks */
  unsigned char      data_out[2 * CAB_BLOCKMAX];     /* compressed data blocks */
  cab_UWORD          cdata_in;
  ULONG              cCompressedBytesInFolder;
  cab_UWORD          cFolders;
  cab_UWORD          cFiles;
  cab_ULONG          cDataBlocks;
  cab_ULONG          cbFileRemainder; /* uncompressed, yet to be written data */
               /* of spanned file of a spanning folder of a spanning cabinet */
  struct temp_file   data;
  BOOL               fNewPrevious;
  cab_ULONG          estimatedCabinetSize;
  struct list        folders_list;
  struct list        files_list;
  struct list        blocks_list;
  cab_ULONG          folders_size;
  cab_ULONG          files_size;          /* size of files not yet assigned to a folder */
  cab_ULONG          placed_files_size;   /* size of files already placed into a folder */
  cab_ULONG          pending_data_size;   /* size of data not yet assigned to a folder */
  cab_ULONG          folders_data_size;   /* total size of data contained in the current folders */
  TCOMP              compression;
  cab_UWORD        (*compress)(struct FCI_Int *);
} FCI_Int;

#define FCI_INT_MAGIC 0xfcfcfc05

static void set_error( FCI_Int *fci, int oper, int err )
{
    fci->perf->erfOper = oper;
    fci->perf->erfType = err;
    fci->perf->fError = TRUE;
    if (err) SetLastError( err );
}

static FCI_Int *get_fci_ptr( HFCI hfci )
{
    FCI_Int *fci= (FCI_Int *)hfci;

    if (!fci || fci->magic != FCI_INT_MAGIC)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return NULL;
    }
    return fci;
}

/* compute the cabinet header size */
static cab_ULONG get_header_size( FCI_Int *fci )
{
    cab_ULONG ret = sizeof(CFHEADER) + fci->ccab.cbReserveCFHeader;

    if (fci->ccab.cbReserveCFHeader || fci->ccab.cbReserveCFFolder || fci->ccab.cbReserveCFData)
        ret += 4;

    if (fci->fPrevCab)
        ret += strlen( fci->szPrevCab ) + 1 + strlen( fci->szPrevDisk ) + 1;

    if (fci->fNextCab)
        ret += strlen( fci->pccab->szCab ) + 1 + strlen( fci->pccab->szDisk ) + 1;

    return ret;
}

static BOOL create_temp_file( FCI_Int *fci, struct temp_file *file )
{
    int err;

    if (!fci->gettemp( file->name, CB_MAX_FILENAME, fci->pv ))
    {
        set_error( fci, FCIERR_TEMP_FILE, ERROR_FUNCTION_FAILED );
        return FALSE;
    }
    if ((file->handle = fci->open( file->name, _O_RDWR | _O_CREAT | _O_EXCL | _O_BINARY,
                                   _S_IREAD | _S_IWRITE, &err, fci->pv )) == -1)
    {
        set_error( fci, FCIERR_TEMP_FILE, err );
        return FALSE;
    }
    return TRUE;
}

static BOOL close_temp_file( FCI_Int *fci, struct temp_file *file )
{
    int err;

    if (file->handle == -1) return TRUE;
    if (fci->close( file->handle, &err, fci->pv ) == -1)
    {
        set_error( fci, FCIERR_TEMP_FILE, err );
        return FALSE;
    }
    file->handle = -1;
    if (fci->delete( file->name, &err, fci->pv ) == -1)
    {
        set_error( fci, FCIERR_TEMP_FILE, err );
        return FALSE;
    }
    return TRUE;
}

static struct file *add_file( FCI_Int *fci, const char *filename )
{
    unsigned int size = FIELD_OFFSET( struct file, name[strlen(filename) + 1] );
    struct file *file = fci->alloc( size );

    if (!file)
    {
        set_error( fci, FCIERR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }
    file->size    = 0;
    file->offset  = fci->cDataBlocks * CAB_BLOCKMAX + fci->cdata_in;
    file->folder  = fci->cFolders;
    file->date    = 0;
    file->time    = 0;
    file->attribs = 0;
    strcpy( file->name, filename );
    list_add_tail( &fci->files_list, &file->entry );
    fci->files_size += sizeof(CFFILE) + strlen(filename) + 1;
    return file;
}

static struct file *copy_file( FCI_Int *fci, const struct file *orig )
{
    unsigned int size = FIELD_OFFSET( struct file, name[strlen(orig->name) + 1] );
    struct file *file = fci->alloc( size );

    if (!file)
    {
        set_error( fci, FCIERR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }
    memcpy( file, orig, size );
    return file;
}

static void free_file( FCI_Int *fci, struct file *file )
{
    list_remove( &file->entry );
    fci->free( file );
}

/* create a new data block for the data in fci->data_in */
static BOOL add_data_block( FCI_Int *fci, PFNFCISTATUS status_callback )
{
    int err;
    struct data_block *block;

    if (!fci->cdata_in) return TRUE;

    if (fci->data.handle == -1 && !create_temp_file( fci, &fci->data )) return FALSE;

    if (!(block = fci->alloc( sizeof(*block) )))
    {
        set_error( fci, FCIERR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }
    block->uncompressed = fci->cdata_in;
    block->compressed   = fci->compress( fci );

    if (fci->write( fci->data.handle, fci->data_out,
                    block->compressed, &err, fci->pv ) != block->compressed)
    {
        set_error( fci, FCIERR_TEMP_FILE, err );
        fci->free( block );
        return FALSE;
    }

    fci->cdata_in = 0;
    fci->pending_data_size += sizeof(CFDATA) + fci->ccab.cbReserveCFData + block->compressed;
    fci->cCompressedBytesInFolder += block->compressed;
    fci->cDataBlocks++;
    list_add_tail( &fci->blocks_list, &block->entry );

    if (status_callback( statusFile, block->compressed, block->uncompressed, fci->pv ) == -1)
    {
        set_error( fci, FCIERR_USER_ABORT, 0 );
        return FALSE;
    }
    return TRUE;
}

/* add compressed blocks for all the data that can be read from the file */
static BOOL add_file_data( FCI_Int *fci, char *sourcefile, char *filename, BOOL execute,
                           PFNFCIGETOPENINFO get_open_info, PFNFCISTATUS status_callback )
{
    int err, len;
    INT_PTR handle;
    struct file *file;

    if (!(file = add_file( fci, filename ))) return FALSE;

    handle = get_open_info( sourcefile, &file->date, &file->time, &file->attribs, &err, fci->pv );
    if (handle == -1)
    {
        free_file( fci, file );
        set_error( fci, FCIERR_OPEN_SRC, err );
        return FALSE;
    }
    if (execute) file->attribs |= _A_EXEC;

    for (;;)
    {
        len = fci->read( handle, fci->data_in + fci->cdata_in,
                         CAB_BLOCKMAX - fci->cdata_in, &err, fci->pv );
        if (!len) break;

        if (len == -1)
        {
            set_error( fci, FCIERR_READ_SRC, err );
            return FALSE;
        }
        file->size += len;
        fci->cdata_in += len;
        if (fci->cdata_in == CAB_BLOCKMAX && !add_data_block( fci, status_callback )) return FALSE;
    }
    fci->close( handle, &err, fci->pv );
    return TRUE;
}

static void free_data_block( FCI_Int *fci, struct data_block *block )
{
    list_remove( &block->entry );
    fci->free( block );
}

static struct folder *add_folder( FCI_Int *fci )
{
    struct folder *folder = fci->alloc( sizeof(*folder) );

    if (!folder)
    {
        set_error( fci, FCIERR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }
    folder->data.handle = -1;
    folder->data_start  = fci->folders_data_size;
    folder->data_count  = 0;
    folder->compression = fci->compression;
    list_init( &folder->files_list );
    list_init( &folder->blocks_list );
    list_add_tail( &fci->folders_list, &folder->entry );
    fci->folders_size += sizeof(CFFOLDER) + fci->ccab.cbReserveCFFolder;
    fci->cFolders++;
    return folder;
}

static void free_folder( FCI_Int *fci, struct folder *folder )
{
    struct file *file, *file_next;
    struct data_block *block, *block_next;

    LIST_FOR_EACH_ENTRY_SAFE( file, file_next, &folder->files_list, struct file, entry )
        free_file( fci, file );
    LIST_FOR_EACH_ENTRY_SAFE( block, block_next, &folder->blocks_list, struct data_block, entry )
        free_data_block( fci, block );
    close_temp_file( fci, &folder->data );
    list_remove( &folder->entry );
    fci->free( folder );
}

/* reset state for the next cabinet file once the current one has been flushed */
static void reset_cabinet( FCI_Int *fci )
{
    struct folder *folder, *folder_next;

    LIST_FOR_EACH_ENTRY_SAFE( folder, folder_next, &fci->folders_list, struct folder, entry )
        free_folder( fci, folder );

    fci->cFolders          = 0;
    fci->cFiles            = 0;
    fci->folders_size      = 0;
    fci->placed_files_size = 0;
    fci->folders_data_size = 0;
}

static cab_ULONG fci_get_checksum( const void *pv, UINT cb, cab_ULONG seed )
{
  cab_ULONG     csum;
  cab_ULONG     ul;
  int           cUlong;
  const BYTE    *pb;

  csum = seed;
  cUlong = cb / 4;
  pb = pv;

  while (cUlong-- > 0) {
    ul = *pb++;
    ul |= (((cab_ULONG)(*pb++)) <<  8);
    ul |= (((cab_ULONG)(*pb++)) << 16);
    ul |= (((cab_ULONG)(*pb++)) << 24);
    csum ^= ul;
  }

  ul = 0;
  switch (cb % 4) {
    case 3:
      ul |= (((ULONG)(*pb++)) << 16);
      /* fall through */
    case 2:
      ul |= (((ULONG)(*pb++)) <<  8);
      /* fall through */
    case 1:
      ul |= *pb;
      /* fall through */
    default:
      break;
  }
  csum ^= ul;

  return csum;
}

/* copy all remaining data block to a new temp file */
static BOOL copy_data_blocks( FCI_Int *fci, INT_PTR handle, cab_ULONG start_pos,
                              struct temp_file *temp, PFNFCISTATUS status_callback )
{
    struct data_block *block;
    int err;

    if (fci->seek( handle, start_pos, SEEK_SET, &err, fci->pv ) != start_pos)
    {
        set_error( fci, FCIERR_TEMP_FILE, err );
        return FALSE;
    }
    if (!create_temp_file( fci, temp )) return FALSE;

    LIST_FOR_EACH_ENTRY( block, &fci->blocks_list, struct data_block, entry )
    {
        if (fci->read( handle, fci->data_out, block->compressed,
                       &err, fci->pv ) != block->compressed)
        {
            close_temp_file( fci, temp );
            set_error( fci, FCIERR_TEMP_FILE, err );
            return FALSE;
        }
        if (fci->write( temp->handle, fci->data_out, block->compressed,
                        &err, fci->pv ) != block->compressed)
        {
            close_temp_file( fci, temp );
            set_error( fci, FCIERR_TEMP_FILE, err );
            return FALSE;
        }
        fci->pending_data_size += sizeof(CFDATA) + fci->ccab.cbReserveCFData + block->compressed;
        fci->statusFolderCopied += block->compressed;

        if (status_callback( statusFolder, fci->statusFolderCopied,
                             fci->statusFolderTotal, fci->pv) == -1)
        {
            close_temp_file( fci, temp );
            set_error( fci, FCIERR_USER_ABORT, 0 );
            return FALSE;
        }
    }
    return TRUE;
}

/* write all folders to disk and remove them from the list */
static BOOL write_folders( FCI_Int *fci, INT_PTR handle, cab_ULONG header_size, PFNFCISTATUS status_callback )
{
    struct folder *folder;
    int err;
    CFFOLDER *cffolder = (CFFOLDER *)fci->data_out;
    cab_ULONG folder_size = sizeof(CFFOLDER) + fci->ccab.cbReserveCFFolder;

    memset( cffolder, 0, folder_size );

    /* write the folders */
    LIST_FOR_EACH_ENTRY( folder, &fci->folders_list, struct folder, entry )
    {
        cffolder->coffCabStart = fci_endian_ulong( folder->data_start + header_size );
        cffolder->cCFData      = fci_endian_uword( folder->data_count );
        cffolder->typeCompress = fci_endian_uword( folder->compression );
        if (fci->write( handle, cffolder, folder_size, &err, fci->pv ) != folder_size)
        {
            set_error( fci, FCIERR_CAB_FILE, err );
            return FALSE;
        }
    }
    return TRUE;
}

/* write all the files to the cabinet file */
static BOOL write_files( FCI_Int *fci, INT_PTR handle, PFNFCISTATUS status_callback )
{
    cab_ULONG file_size;
    struct folder *folder;
    struct file *file;
    int err;
    CFFILE *cffile = (CFFILE *)fci->data_out;

    LIST_FOR_EACH_ENTRY( folder, &fci->folders_list, struct folder, entry )
    {
        LIST_FOR_EACH_ENTRY( file, &folder->files_list, struct file, entry )
        {
            cffile->cbFile          = fci_endian_ulong( file->size );
            cffile->uoffFolderStart = fci_endian_ulong( file->offset );
            cffile->iFolder         = fci_endian_uword( file->folder );
            cffile->date            = fci_endian_uword( file->date );
            cffile->time            = fci_endian_uword( file->time );
            cffile->attribs         = fci_endian_uword( file->attribs );
            lstrcpynA( (char *)(cffile + 1), file->name, CB_MAX_FILENAME );
            file_size = sizeof(CFFILE) + strlen( (char *)(cffile + 1) ) + 1;
            if (fci->write( handle, cffile, file_size, &err, fci->pv ) != file_size)
            {
                set_error( fci, FCIERR_CAB_FILE, err );
                return FALSE;
            }
            if (!fci->fSplitFolder)
            {
                fci->statusFolderCopied = 0;
                /* TODO TEST THIS further */
                fci->statusFolderTotal = fci->folders_data_size + fci->placed_files_size;
            }
            fci->statusFolderCopied += file_size;
            /* report status about copied size of folder */
            if (status_callback( statusFolder, fci->statusFolderCopied,
                                 fci->statusFolderTotal, fci->pv ) == -1)
            {
                set_error( fci, FCIERR_USER_ABORT, 0 );
                return FALSE;
            }
        }
    }
    return TRUE;
}

/* write all data blocks to the cabinet file */
static BOOL write_data_blocks( FCI_Int *fci, INT_PTR handle, PFNFCISTATUS status_callback )
{
    struct folder *folder;
    struct data_block *block;
    int err, len;
    CFDATA *cfdata;
    void *data;
    cab_UWORD header_size;

    header_size = sizeof(CFDATA) + fci->ccab.cbReserveCFData;
    cfdata = (CFDATA *)fci->data_out;
    memset( cfdata, 0, header_size );
    data = (char *)cfdata + header_size;

    LIST_FOR_EACH_ENTRY( folder, &fci->folders_list, struct folder, entry )
    {
        if (fci->seek( folder->data.handle, 0, SEEK_SET, &err, fci->pv ) != 0)
        {
            set_error( fci, FCIERR_CAB_FILE, err );
            return FALSE;
        }
        LIST_FOR_EACH_ENTRY( block, &folder->blocks_list, struct data_block, entry )
        {
            len = fci->read( folder->data.handle, data, block->compressed, &err, fci->pv );
            if (len != block->compressed) return FALSE;

            cfdata->cbData = fci_endian_uword( block->compressed );
            cfdata->cbUncomp = fci_endian_uword( block->uncompressed );
            cfdata->csum = fci_endian_ulong( fci_get_checksum( &cfdata->cbData,
                                                               header_size - FIELD_OFFSET(CFDATA, cbData),
                                                               fci_get_checksum( data, len, 0 )));

            fci->statusFolderCopied += len;
            len += header_size;
            if (fci->write( handle, fci->data_out, len, &err, fci->pv ) != len)
            {
                set_error( fci, FCIERR_CAB_FILE, err );
                return FALSE;
            }
            if (status_callback( statusFolder, fci->statusFolderCopied, fci->statusFolderTotal, fci->pv) == -1)
            {
                set_error( fci, FCIERR_USER_ABORT, 0 );
                return FALSE;
            }
        }
    }
    return TRUE;
}

/* write the cabinet file to disk */
static BOOL write_cabinet( FCI_Int *fci, PFNFCISTATUS status_callback )
{
    char filename[CB_MAX_CAB_PATH + CB_MAX_CABINET_NAME];
    int err;
    char *ptr;
    INT_PTR handle;
    CFHEADER *cfheader = (CFHEADER *)fci->data_out;
    cab_UWORD flags = 0;
    cab_ULONG header_size = get_header_size( fci );
    cab_ULONG total_size = header_size + fci->folders_size +
                           fci->placed_files_size + fci->folders_data_size;

    assert( header_size <= sizeof(fci->data_out) );
    memset( cfheader, 0, header_size );

    if (fci->fPrevCab) flags |= cfheadPREV_CABINET;
    if (fci->fNextCab) flags |= cfheadNEXT_CABINET;
    if (fci->ccab.cbReserveCFHeader || fci->ccab.cbReserveCFFolder || fci->ccab.cbReserveCFData)
      flags |= cfheadRESERVE_PRESENT;

    memcpy( cfheader->signature, "!CAB", 4 );
    cfheader->cbCabinet    = fci_endian_ulong( total_size );
    cfheader->coffFiles    = fci_endian_ulong( header_size + fci->folders_size );
    cfheader->versionMinor = 3;
    cfheader->versionMajor = 1;
    cfheader->cFolders     = fci_endian_uword( fci->cFolders );
    cfheader->cFiles       = fci_endian_uword( fci->cFiles );
    cfheader->flags        = fci_endian_uword( flags );
    cfheader->setID        = fci_endian_uword( fci->ccab.setID );
    cfheader->iCabinet     = fci_endian_uword( fci->ccab.iCab );
    ptr = (char *)(cfheader + 1);

    if (flags & cfheadRESERVE_PRESENT)
    {
        struct
        {
            cab_UWORD cbCFHeader;
            cab_UBYTE cbCFFolder;
            cab_UBYTE cbCFData;
        } *reserve = (void *)ptr;

        reserve->cbCFHeader = fci_endian_uword( fci->ccab.cbReserveCFHeader );
        reserve->cbCFFolder = fci->ccab.cbReserveCFFolder;
        reserve->cbCFData   = fci->ccab.cbReserveCFData;
        ptr = (char *)(reserve + 1);
    }
    ptr += fci->ccab.cbReserveCFHeader;

    if (flags & cfheadPREV_CABINET)
    {
        strcpy( ptr, fci->szPrevCab );
        ptr += strlen( ptr ) + 1;
        strcpy( ptr, fci->szPrevDisk );
        ptr += strlen( ptr ) + 1;
    }

    if (flags & cfheadNEXT_CABINET)
    {
        strcpy( ptr, fci->pccab->szCab );
        ptr += strlen( ptr ) + 1;
        strcpy( ptr, fci->pccab->szDisk );
        ptr += strlen( ptr ) + 1;
    }

    assert( ptr - (char *)cfheader == header_size );

    strcpy( filename, fci->ccab.szCabPath );
    strcat( filename, fci->ccab.szCab );

    if ((handle = fci->open( filename, _O_RDWR | _O_CREAT | _O_TRUNC | _O_BINARY,
                             _S_IREAD | _S_IWRITE, &err, fci->pv )) == -1)
    {
        set_error( fci, FCIERR_CAB_FILE, err );
        return FALSE;
    }

    if (fci->write( handle, cfheader, header_size, &err, fci->pv ) != header_size)
    {
        set_error( fci, FCIERR_CAB_FILE, err );
        goto failed;
    }

    /* add size of header size of all CFFOLDERs and size of all CFFILEs */
    header_size += fci->placed_files_size + fci->folders_size;
    if (!write_folders( fci, handle, header_size, status_callback )) goto failed;
    if (!write_files( fci, handle, status_callback )) goto failed;
    if (!write_data_blocks( fci, handle, status_callback )) goto failed;

    /* update the signature */
    if (fci->seek( handle, 0, SEEK_SET, &err, fci->pv ) != 0 )
    {
        set_error( fci, FCIERR_CAB_FILE, err );
        goto failed;
    }
    memcpy( cfheader->signature, "MSCF", 4 );
    if (fci->write( handle, cfheader->signature, 4, &err, fci->pv ) != 4)
    {
        set_error( fci, FCIERR_CAB_FILE, err );
        goto failed;
    }
    fci->close( handle, &err, fci->pv );

    reset_cabinet( fci );
    status_callback( statusCabinet, fci->estimatedCabinetSize, total_size, fci->pv );
    return TRUE;

failed:
    fci->close( handle, &err, fci->pv );
    fci->delete( filename, &err, fci->pv );
    return FALSE;
}

/* add all pending data blocks folder */
static BOOL add_data_to_folder( FCI_Int *fci, struct folder *folder, cab_ULONG *payload,
                                PFNFCISTATUS status_callback )
{
    struct data_block *block, *new, *next;
    BOOL split_block = FALSE;
    cab_ULONG current_size, start_pos = 0;

    *payload = 0;
    current_size = get_header_size( fci ) + fci->folders_size +
                   fci->files_size + fci->placed_files_size + fci->folders_data_size;

    /* move the temp file into the folder structure */
    folder->data = fci->data;
    fci->data.handle = -1;
    fci->pending_data_size = 0;

    LIST_FOR_EACH_ENTRY_SAFE( block, next, &fci->blocks_list, struct data_block, entry )
    {
        /* No more CFDATA fits into the cabinet under construction */
        /* So don't try to store more data into it */
        if (fci->fNextCab && (fci->ccab.cb <= sizeof(CFDATA) + fci->ccab.cbReserveCFData +
                              current_size + sizeof(CFFOLDER) + fci->ccab.cbReserveCFFolder))
            break;

        if (!(new = fci->alloc( sizeof(*new) )))
        {
            set_error( fci, FCIERR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY );
            return FALSE;
        }
        /* Is cabinet with new CFDATA too large? Then data block has to be split */
        if( fci->fNextCab &&
            (fci->ccab.cb < sizeof(CFDATA) + fci->ccab.cbReserveCFData +
             block->compressed + current_size + sizeof(CFFOLDER) + fci->ccab.cbReserveCFFolder))
        {
            /* Modify the size of the compressed data to store only a part of the */
            /* data block into the current cabinet. This is done to prevent */
            /* that the maximum cabinet size will be exceeded. The remainder */
            /* will be stored into the next following cabinet. */

            new->compressed = fci->ccab.cb - (sizeof(CFDATA) + fci->ccab.cbReserveCFData + current_size +
                                              sizeof(CFFOLDER) + fci->ccab.cbReserveCFFolder );
            new->uncompressed = 0; /* on split blocks of data this is zero */
            block->compressed -= new->compressed;
            split_block = TRUE;
        }
        else
        {
            new->compressed   = block->compressed;
            new->uncompressed = block->uncompressed;
        }

        start_pos += new->compressed;
        current_size += sizeof(CFDATA) + fci->ccab.cbReserveCFData + new->compressed;
        fci->folders_data_size += sizeof(CFDATA) + fci->ccab.cbReserveCFData + new->compressed;
        fci->statusFolderCopied += new->compressed;
        (*payload) += new->uncompressed;

        list_add_tail( &folder->blocks_list, &new->entry );
        folder->data_count++;

        /* report status with pfnfcis about copied size of folder */
        if (status_callback( statusFolder, fci->statusFolderCopied,
                             fci->statusFolderTotal, fci->pv ) == -1)
        {
            set_error( fci, FCIERR_USER_ABORT, 0 );
            return FALSE;
        }
        if (split_block) break;
        free_data_block( fci, block );
        fci->cDataBlocks--;
    }

    if (list_empty( &fci->blocks_list )) return TRUE;
    return copy_data_blocks( fci, folder->data.handle, start_pos, &fci->data, status_callback );
}

/* add all pending files to folder */
static BOOL add_files_to_folder( FCI_Int *fci, struct folder *folder, cab_ULONG payload )
{
    cab_ULONG sizeOfFiles = 0, sizeOfFilesPrev;
    cab_ULONG cbFileRemainder = 0;
    struct file *file, *next;

    LIST_FOR_EACH_ENTRY_SAFE( file, next, &fci->files_list, struct file, entry )
    {
        cab_ULONG size = sizeof(CFFILE) + strlen(file->name) + 1;

        /* fnfilfnfildest: placed file on cabinet */
        fci->fileplaced( &fci->ccab, file->name, file->size,
                         (file->folder == cffileCONTINUED_FROM_PREV), fci->pv );

        sizeOfFilesPrev = sizeOfFiles;
        /* set complete size of all processed files */
        if (file->folder == cffileCONTINUED_FROM_PREV && fci->cbFileRemainder != 0)
        {
            sizeOfFiles += fci->cbFileRemainder;
            fci->cbFileRemainder = 0;
        }
        else sizeOfFiles += file->size;

        /* check if spanned file fits into this cabinet folder */
        if (sizeOfFiles > payload)
        {
            if (file->folder == cffileCONTINUED_FROM_PREV)
                file->folder = cffileCONTINUED_PREV_AND_NEXT;
            else
                file->folder = cffileCONTINUED_TO_NEXT;
        }

        list_remove( &file->entry );
        list_add_tail( &folder->files_list, &file->entry );
        fci->placed_files_size += size;
        fci->cFiles++;

        /* This is only true for files which will be written into the */
        /* next cabinet of the spanning folder */
        if (sizeOfFiles > payload)
        {
            /* add a copy back onto the list */
            if (!(file = copy_file( fci, file ))) return FALSE;
            list_add_before( &next->entry, &file->entry );

            /* Files which data will be partially written into the current cabinet */
            if (file->folder == cffileCONTINUED_PREV_AND_NEXT || file->folder == cffileCONTINUED_TO_NEXT)
            {
                if (sizeOfFilesPrev <= payload)
                {
                    /* The size of the uncompressed, data of a spanning file in a */
                    /* spanning data */
                    cbFileRemainder = sizeOfFiles - payload;
                }
                file->folder = cffileCONTINUED_FROM_PREV;
            }
            else file->folder = 0;
        }
        else
        {
            fci->files_size -= size;
        }
    }
    fci->cbFileRemainder = cbFileRemainder;
    return TRUE;
}

static cab_UWORD compress_NONE( FCI_Int *fci )
{
    memcpy( fci->data_out, fci->data_in, fci->cdata_in );
    return fci->cdata_in;
}

static void *zalloc( void *opaque, unsigned int items, unsigned int size )
{
    FCI_Int *fci = opaque;
    return fci->alloc( items * size );
}

static void zfree( void *opaque, void *ptr )
{
    FCI_Int *fci = opaque;
    fci->free( ptr );
}

static cab_UWORD compress_MSZIP( FCI_Int *fci )
{
    z_stream stream;

    stream.zalloc = zalloc;
    stream.zfree  = zfree;
    stream.opaque = fci;
    if (deflateInit2( &stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY ) != Z_OK)
    {
        set_error( fci, FCIERR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }
    stream.next_in   = fci->data_in;
    stream.avail_in  = fci->cdata_in;
    stream.next_out  = fci->data_out + 2;
    stream.avail_out = sizeof(fci->data_out) - 2;
    /* insert the signature */
    fci->data_out[0] = 'C';
    fci->data_out[1] = 'K';
    deflate( &stream, Z_FINISH );
    deflateEnd( &stream );
    return stream.total_out + 2;
}


/***********************************************************************
 *		FCICreate (CABINET.10)
 *
 * FCICreate is provided with several callbacks and
 * returns a handle which can be used to create cabinet files.
 *
 * PARAMS
 *   perf       [IO]  A pointer to an ERF structure.  When FCICreate
 *                    returns an error condition, error information may
 *                    be found here as well as from GetLastError.
 *   pfnfiledest [I]  A pointer to a function which is called when a file
 *                    is placed. Only useful for subsequent cabinet files.
 *   pfnalloc    [I]  A pointer to a function which allocates ram.  Uses
 *                    the same interface as malloc.
 *   pfnfree     [I]  A pointer to a function which frees ram.  Uses the
 *                    same interface as free.
 *   pfnopen     [I]  A pointer to a function which opens a file.  Uses
 *                    the same interface as _open.
 *   pfnread     [I]  A pointer to a function which reads from a file into
 *                    a caller-provided buffer.  Uses the same interface
 *                    as _read.
 *   pfnwrite    [I]  A pointer to a function which writes to a file from
 *                    a caller-provided buffer.  Uses the same interface
 *                    as _write.
 *   pfnclose    [I]  A pointer to a function which closes a file handle.
 *                    Uses the same interface as _close.
 *   pfnseek     [I]  A pointer to a function which seeks in a file.
 *                    Uses the same interface as _lseek.
 *   pfndelete   [I]  A pointer to a function which deletes a file.
 *   pfnfcigtf   [I]  A pointer to a function which gets the name of a
 *                    temporary file.
 *   pccab       [I]  A pointer to an initialized CCAB structure.
 *   pv          [I]  A pointer to an application-defined notification
 *                    function which will be passed to other FCI functions
 *                    as a parameter.
 *
 * RETURNS
 *   On success, returns an FCI handle of type HFCI.
 *   On failure, the NULL file handle is returned. Error
 *   info can be retrieved from perf.
 *
 * INCLUDES
 *   fci.h
 *
 */
HFCI __cdecl FCICreate(
	PERF perf,
	PFNFCIFILEPLACED   pfnfiledest,
	PFNFCIALLOC        pfnalloc,
	PFNFCIFREE         pfnfree,
	PFNFCIOPEN         pfnopen,
	PFNFCIREAD         pfnread,
	PFNFCIWRITE        pfnwrite,
	PFNFCICLOSE        pfnclose,
	PFNFCISEEK         pfnseek,
	PFNFCIDELETE       pfndelete,
	PFNFCIGETTEMPFILE  pfnfcigtf,
	PCCAB              pccab,
	void *pv)
{
  FCI_Int *p_fci_internal;

  if (!perf) {
    SetLastError(ERROR_BAD_ARGUMENTS);
    return NULL;
  }
  if ((!pfnalloc) || (!pfnfree) || (!pfnopen) || (!pfnread) ||
      (!pfnwrite) || (!pfnclose) || (!pfnseek) || (!pfndelete) ||
      (!pfnfcigtf) || (!pccab)) {
    perf->erfOper = FCIERR_NONE;
    perf->erfType = ERROR_BAD_ARGUMENTS;
    perf->fError = TRUE;

    SetLastError(ERROR_BAD_ARGUMENTS);
    return NULL;
  }

  if (!((p_fci_internal = pfnalloc(sizeof(FCI_Int))))) {
    perf->erfOper = FCIERR_ALLOC_FAIL;
    perf->erfType = ERROR_NOT_ENOUGH_MEMORY;
    perf->fError = TRUE;

    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return NULL;
  }

  memset(p_fci_internal, 0, sizeof(FCI_Int));
  p_fci_internal->magic = FCI_INT_MAGIC;
  p_fci_internal->perf = perf;
  p_fci_internal->fileplaced = pfnfiledest;
  p_fci_internal->alloc = pfnalloc;
  p_fci_internal->free = pfnfree;
  p_fci_internal->open = pfnopen;
  p_fci_internal->read = pfnread;
  p_fci_internal->write = pfnwrite;
  p_fci_internal->close = pfnclose;
  p_fci_internal->seek = pfnseek;
  p_fci_internal->delete = pfndelete;
  p_fci_internal->gettemp = pfnfcigtf;
  p_fci_internal->ccab = *pccab;
  p_fci_internal->pccab = pccab;
  p_fci_internal->pv = pv;
  p_fci_internal->data.handle = -1;
  p_fci_internal->compress = compress_NONE;

  list_init( &p_fci_internal->folders_list );
  list_init( &p_fci_internal->files_list );
  list_init( &p_fci_internal->blocks_list );

  memcpy(p_fci_internal->szPrevCab, pccab->szCab, CB_MAX_CABINET_NAME);
  memcpy(p_fci_internal->szPrevDisk, pccab->szDisk, CB_MAX_DISK_NAME);

  return (HFCI)p_fci_internal;
}




static BOOL fci_flush_folder( FCI_Int *p_fci_internal,
	BOOL                  fGetNextCab,
	PFNFCIGETNEXTCABINET  pfnfcignc,
	PFNFCISTATUS          pfnfcis)
{
  cab_ULONG payload;
  cab_ULONG read_result;
  struct folder *folder;

  if ((!pfnfcignc) || (!pfnfcis)) {
    set_error( p_fci_internal, FCIERR_NONE, ERROR_BAD_ARGUMENTS );
    return FALSE;
  }

  if( p_fci_internal->fGetNextCabInVain &&
      p_fci_internal->fNextCab ){
    /* internal error */
    set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
    return FALSE;
  }

  /* If there was no FCIAddFile or FCIFlushFolder has already been called */
  /* this function will return TRUE */
  if( p_fci_internal->files_size == 0 ) {
    if ( p_fci_internal->pending_data_size != 0 ) {
      /* error handling */
      set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
      return FALSE;
    }
    return TRUE;
  }

  /* FCIFlushFolder has already been called... */
  if (p_fci_internal->fSplitFolder && p_fci_internal->placed_files_size!=0) {
    return TRUE;
  }

  /* This can be set already, because it makes only a difference */
  /* when the current function exits with return FALSE */
  p_fci_internal->fSplitFolder=FALSE;

  /* START of COPY */
  if (!add_data_block( p_fci_internal, pfnfcis )) return FALSE;

  /* reset to get the number of data blocks of this folder which are */
  /* actually in this cabinet ( at least partially ) */
  p_fci_internal->cDataBlocks=0;

  p_fci_internal->statusFolderTotal = get_header_size( p_fci_internal ) +
      sizeof(CFFOLDER) + p_fci_internal->ccab.cbReserveCFFolder +
      p_fci_internal->placed_files_size+
      p_fci_internal->folders_data_size + p_fci_internal->files_size+
      p_fci_internal->pending_data_size + p_fci_internal->folders_size;
  p_fci_internal->statusFolderCopied = 0;

  /* report status with pfnfcis about copied size of folder */
  if( (*pfnfcis)(statusFolder, p_fci_internal->statusFolderCopied,
      p_fci_internal->statusFolderTotal, /* TODO total folder size */
      p_fci_internal->pv) == -1) {
    set_error( p_fci_internal, FCIERR_USER_ABORT, 0 );
    return FALSE;
  }

  /* USE the variable read_result */
  read_result = get_header_size( p_fci_internal ) + p_fci_internal->folders_data_size +
      p_fci_internal->placed_files_size + p_fci_internal->folders_size;

  if(p_fci_internal->files_size!=0) {
    read_result+= sizeof(CFFOLDER)+p_fci_internal->ccab.cbReserveCFFolder;
  }

  /* Check if multiple cabinets have to be created. */

  /* Might be too much data for the maximum allowed cabinet size.*/
  /* When any further data will be added later, it might not */
  /* be possible to flush the cabinet, because there might */
  /* not be enough space to store the name of the following */
  /* cabinet and name of the corresponding disk. */
  /* So take care of this and get the name of the next cabinet */
  if( p_fci_internal->fGetNextCabInVain==FALSE &&
      p_fci_internal->fNextCab==FALSE &&
      (
        (
          p_fci_internal->ccab.cb < read_result +
          p_fci_internal->pending_data_size +
          p_fci_internal->files_size +
          CB_MAX_CABINET_NAME +   /* next cabinet name */
          CB_MAX_DISK_NAME        /* next disk name */
        ) || fGetNextCab
      )
  ) {
    /* increment cabinet index */
    ++(p_fci_internal->pccab->iCab);
    /* get name of next cabinet */
    p_fci_internal->estimatedCabinetSize=p_fci_internal->statusFolderTotal;
    if (!(*pfnfcignc)(p_fci_internal->pccab,
        p_fci_internal->estimatedCabinetSize, /* estimated size of cab */
        p_fci_internal->pv)) {
      set_error( p_fci_internal, FCIERR_NONE, ERROR_FUNCTION_FAILED );
      return FALSE;
    }

    /* Skip a few lines of code. This is caught by the next if. */
    p_fci_internal->fGetNextCabInVain=TRUE;
  }

  /* too much data for cabinet */
  if( (p_fci_internal->fGetNextCabInVain ||
        p_fci_internal->fNextCab ) &&
      (
        (
          p_fci_internal->ccab.cb < read_result +
          p_fci_internal->pending_data_size +
          p_fci_internal->files_size +
          strlen(p_fci_internal->pccab->szCab)+1 +   /* next cabinet name */
          strlen(p_fci_internal->pccab->szDisk)+1    /* next disk name */
        ) || fGetNextCab
      )
  ) {
    p_fci_internal->fGetNextCabInVain=FALSE;
    p_fci_internal->fNextCab=TRUE;

    /* return FALSE if there is not enough space left*/
    /* this should never happen */
    if (p_fci_internal->ccab.cb <=
        p_fci_internal->files_size +
        read_result +
        strlen(p_fci_internal->pccab->szCab)+1 + /* next cabinet name */
        strlen(p_fci_internal->pccab->szDisk)+1  /* next disk name */
    ) {

      return FALSE;
    }

    /* the folder will be split across cabinets */
    p_fci_internal->fSplitFolder=TRUE;

  } else {
    /* this should never happen */
    if (p_fci_internal->fNextCab) {
      /* internal error */
      set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
      return FALSE;
    }
  }

  if (!(folder = add_folder( p_fci_internal ))) return FALSE;
  if (!add_data_to_folder( p_fci_internal, folder, &payload, pfnfcis )) return FALSE;
  if (!add_files_to_folder( p_fci_internal, folder, payload )) return FALSE;

  /* reset CFFolder specific information */
  p_fci_internal->cDataBlocks=0;
  p_fci_internal->cCompressedBytesInFolder=0;

  return TRUE;
}




static BOOL fci_flush_cabinet( FCI_Int *p_fci_internal,
	BOOL                  fGetNextCab,
	PFNFCIGETNEXTCABINET  pfnfcignc,
	PFNFCISTATUS          pfnfcis)
{
  cab_ULONG read_result=0;
  BOOL returntrue=FALSE;

  /* TODO test if fci_flush_cabinet really aborts if there was no FCIAddFile */

  /* when FCIFlushCabinet was or FCIAddFile hasn't been called */
  if( p_fci_internal->files_size==0 && fGetNextCab ) {
    returntrue=TRUE;
  }

  if (!fci_flush_folder(p_fci_internal,fGetNextCab,pfnfcignc,pfnfcis)){
    /* TODO set error */
    return FALSE;
  }

  if(returntrue) return TRUE;

  if ( (p_fci_internal->fSplitFolder && p_fci_internal->fNextCab==FALSE)||
       (p_fci_internal->folders_size==0 &&
         (p_fci_internal->files_size!=0 ||
          p_fci_internal->placed_files_size!=0 )
     ) )
  {
      /* error */
      set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
      return FALSE;
  }

  /* create the cabinet */
  if (!write_cabinet( p_fci_internal, pfnfcis )) return FALSE;

  p_fci_internal->fPrevCab=TRUE;
  /* The sections szPrevCab and szPrevDisk are not being updated, because */
  /* MS CABINET.DLL always puts the first cabinet name and disk into them */

  if (p_fci_internal->fNextCab) {
    p_fci_internal->fNextCab=FALSE;

    if (p_fci_internal->files_size==0 && p_fci_internal->pending_data_size!=0) {
      /* THIS CAN NEVER HAPPEN */
      /* set error code */
      set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
      return FALSE;
    }

    if( p_fci_internal->fNewPrevious ) {
      memcpy(p_fci_internal->szPrevCab, p_fci_internal->ccab.szCab,
        CB_MAX_CABINET_NAME);
      memcpy(p_fci_internal->szPrevDisk, p_fci_internal->ccab.szDisk,
        CB_MAX_DISK_NAME);
      p_fci_internal->fNewPrevious=FALSE;
    }
    p_fci_internal->ccab = *p_fci_internal->pccab;

    /* REUSE the variable read_result */
    read_result=get_header_size( p_fci_internal );
    if(p_fci_internal->files_size!=0) {
        read_result+=p_fci_internal->ccab.cbReserveCFFolder;
    }
    read_result+= p_fci_internal->pending_data_size +
      p_fci_internal->files_size + p_fci_internal->folders_data_size +
      p_fci_internal->placed_files_size + p_fci_internal->folders_size +
      sizeof(CFFOLDER); /* set size of new CFFolder entry */

    /* too much data for the maximum size of a cabinet */
    if( p_fci_internal->fGetNextCabInVain==FALSE &&
        p_fci_internal->ccab.cb < read_result ) {
      return fci_flush_cabinet( p_fci_internal, FALSE, pfnfcignc, pfnfcis);
    }

    /* Might be too much data for the maximum size of a cabinet.*/
    /* When any further data will be added later, it might not */
    /* be possible to flush the cabinet, because there might */
    /* not be enough space to store the name of the following */
    /* cabinet and name of the corresponding disk. */
    /* So take care of this and get the name of the next cabinet */
    if (p_fci_internal->fGetNextCabInVain==FALSE && (
      p_fci_internal->ccab.cb < read_result +
      CB_MAX_CABINET_NAME + CB_MAX_DISK_NAME
    )) {
      /* increment cabinet index */
      ++(p_fci_internal->pccab->iCab);
      /* get name of next cabinet */
      p_fci_internal->estimatedCabinetSize=p_fci_internal->statusFolderTotal;
      if (!(*pfnfcignc)(p_fci_internal->pccab,
          p_fci_internal->estimatedCabinetSize, /* estimated size of cab */
          p_fci_internal->pv)) {
        /* error handling */
        set_error( p_fci_internal, FCIERR_NONE, ERROR_FUNCTION_FAILED );
        return FALSE;
      }
      /* Skip a few lines of code. This is caught by the next if. */
      p_fci_internal->fGetNextCabInVain=TRUE;
    }

    /* too much data for cabinet */
    if (p_fci_internal->fGetNextCabInVain && (
        p_fci_internal->ccab.cb < read_result +
        strlen(p_fci_internal->ccab.szCab)+1+
        strlen(p_fci_internal->ccab.szDisk)+1
    )) {
      p_fci_internal->fGetNextCabInVain=FALSE;
      p_fci_internal->fNextCab=TRUE;
      return fci_flush_cabinet( p_fci_internal, FALSE, pfnfcignc, pfnfcis);
    }

    /* if the FolderThreshold has been reached flush the folder automatically */
    if (p_fci_internal->cCompressedBytesInFolder >= p_fci_internal->ccab.cbFolderThresh)
        return fci_flush_folder(p_fci_internal, FALSE, pfnfcignc, pfnfcis);

    if( p_fci_internal->files_size>0 ) {
      if( !fci_flush_folder(p_fci_internal, FALSE, pfnfcignc, pfnfcis) ) return FALSE;
      p_fci_internal->fNewPrevious=TRUE;
    }
  } else {
    p_fci_internal->fNewPrevious=FALSE;
    if( p_fci_internal->files_size>0 || p_fci_internal->pending_data_size) {
      /* THIS MAY NEVER HAPPEN */
      /* set error structures */
      set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
      return FALSE;
    }
  }

  return TRUE;
} /* end of fci_flush_cabinet */





/***********************************************************************
 *		FCIAddFile (CABINET.11)
 *
 * FCIAddFile adds a file to the to be created cabinet file
 *
 * PARAMS
 *   hfci          [I]  An HFCI from FCICreate
 *   pszSourceFile [I]  A pointer to a C string which contains the name and
 *                      location of the file which will be added to the cabinet
 *   pszFileName   [I]  A pointer to a C string which contains the name under
 *                      which the file will be stored in the cabinet
 *   fExecute      [I]  A boolean value which indicates if the file should be
 *                      executed after extraction of self extracting
 *                      executables
 *   pfnfcignc     [I]  A pointer to a function which gets information about
 *                      the next cabinet
 *   pfnfcis      [IO]  A pointer to a function which will report status
 *                      information about the compression process
 *   pfnfcioi      [I]  A pointer to a function which reports file attributes
 *                      and time and date information
 *   typeCompress  [I]  Compression type
 *
 * RETURNS
 *   On success, returns TRUE
 *   On failure, returns FALSE
 *
 * INCLUDES
 *   fci.h
 *
 */
BOOL __cdecl FCIAddFile(
	HFCI                  hfci,
	char                 *pszSourceFile,
	char                 *pszFileName,
	BOOL                  fExecute,
	PFNFCIGETNEXTCABINET  pfnfcignc,
	PFNFCISTATUS          pfnfcis,
	PFNFCIGETOPENINFO     pfnfcigoi,
	TCOMP                 typeCompress)
{
  cab_ULONG read_result;
  FCI_Int *p_fci_internal = get_fci_ptr( hfci );

  if (!p_fci_internal) return FALSE;

  if ((!pszSourceFile) || (!pszFileName) || (!pfnfcignc) || (!pfnfcis) ||
      (!pfnfcigoi) || strlen(pszFileName)>=CB_MAX_FILENAME) {
    set_error( p_fci_internal, FCIERR_NONE, ERROR_BAD_ARGUMENTS );
    return FALSE;
  }

  if (typeCompress != p_fci_internal->compression)
  {
      if (!FCIFlushFolder( hfci, pfnfcignc, pfnfcis )) return FALSE;
      switch (typeCompress)
      {
      case tcompTYPE_MSZIP:
          p_fci_internal->compression = tcompTYPE_MSZIP;
          p_fci_internal->compress    = compress_MSZIP;
          break;
      default:
          FIXME( "compression %x not supported, defaulting to none\n", typeCompress );
          /* fall through */
      case tcompTYPE_NONE:
          p_fci_internal->compression = tcompTYPE_NONE;
          p_fci_internal->compress    = compress_NONE;
          break;
      }
  }

  /* TODO check if pszSourceFile??? */

  if(p_fci_internal->fGetNextCabInVain && p_fci_internal->fNextCab) {
    /* internal error */
    set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
    return FALSE;
  }

  if(p_fci_internal->fNextCab) {
    /* internal error */
    set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
    return FALSE;
  }

  /* REUSE the variable read_result */
  read_result=get_header_size( p_fci_internal ) + p_fci_internal->ccab.cbReserveCFFolder;

  read_result+= sizeof(CFFILE) + strlen(pszFileName)+1 +
    p_fci_internal->files_size + p_fci_internal->folders_data_size +
    p_fci_internal->placed_files_size + p_fci_internal->folders_size +
    sizeof(CFFOLDER); /* size of new CFFolder entry */

  /* Might be too much data for the maximum size of a cabinet.*/
  /* When any further data will be added later, it might not */
  /* be possible to flush the cabinet, because there might */
  /* not be enough space to store the name of the following */
  /* cabinet and name of the corresponding disk. */
  /* So take care of this and get the name of the next cabinet */
  if( p_fci_internal->fGetNextCabInVain==FALSE &&
      p_fci_internal->fNextCab==FALSE &&
      ( p_fci_internal->ccab.cb < read_result +
        CB_MAX_CABINET_NAME + CB_MAX_DISK_NAME
      )
  ) {
    /* increment cabinet index */
    ++(p_fci_internal->pccab->iCab);
    /* get name of next cabinet */
    p_fci_internal->estimatedCabinetSize=p_fci_internal->statusFolderTotal;
    if (!(*pfnfcignc)(p_fci_internal->pccab,
        p_fci_internal->estimatedCabinetSize, /* estimated size of cab */
        p_fci_internal->pv)) {
      /* error handling */
      set_error( p_fci_internal, FCIERR_NONE, ERROR_FUNCTION_FAILED );
      return FALSE;
    }
    /* Skip a few lines of code. This is caught by the next if. */
    p_fci_internal->fGetNextCabInVain=TRUE;
  }

  if( p_fci_internal->fGetNextCabInVain &&
      p_fci_internal->fNextCab
  ) {
    /* THIS CAN NEVER HAPPEN */
    /* set error code */
    set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
    return FALSE;
  }

  /* too much data for cabinet */
  if( p_fci_internal->fGetNextCabInVain &&
     (
      p_fci_internal->ccab.cb < read_result +
      strlen(p_fci_internal->pccab->szCab)+1+
      strlen(p_fci_internal->pccab->szDisk)+1
  )) {
    p_fci_internal->fGetNextCabInVain=FALSE;
    p_fci_internal->fNextCab=TRUE;
    if(!fci_flush_cabinet( p_fci_internal, FALSE, pfnfcignc, pfnfcis)) return FALSE;
  }

  if( p_fci_internal->fNextCab ) {
    /* THIS MAY NEVER HAPPEN */
    /* set error code */
    set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
    return FALSE;
  }

  if (!add_file_data( p_fci_internal, pszSourceFile, pszFileName, fExecute, pfnfcigoi, pfnfcis ))
      return FALSE;

  /* REUSE the variable read_result */
  read_result = get_header_size( p_fci_internal ) + p_fci_internal->ccab.cbReserveCFFolder;
  read_result+= p_fci_internal->pending_data_size +
    p_fci_internal->files_size + p_fci_internal->folders_data_size +
    p_fci_internal->placed_files_size + p_fci_internal->folders_size +
    sizeof(CFFOLDER); /* set size of new CFFolder entry */

  /* too much data for the maximum size of a cabinet */
  /* (ignoring the unflushed data block) */
  if( p_fci_internal->fGetNextCabInVain==FALSE &&
      p_fci_internal->fNextCab==FALSE && /* this is always the case */
      p_fci_internal->ccab.cb < read_result ) {
    return fci_flush_cabinet( p_fci_internal, FALSE, pfnfcignc, pfnfcis);
  }

  /* Might be too much data for the maximum size of a cabinet.*/
  /* When any further data will be added later, it might not */
  /* be possible to flush the cabinet, because there might */
  /* not be enough space to store the name of the following */
  /* cabinet and name of the corresponding disk. */
  /* So take care of this and get the name of the next cabinet */
  /* (ignoring the unflushed data block) */
  if( p_fci_internal->fGetNextCabInVain==FALSE &&
      p_fci_internal->fNextCab==FALSE &&
      ( p_fci_internal->ccab.cb < read_result +
        CB_MAX_CABINET_NAME + CB_MAX_DISK_NAME
      )
  ) {
    /* increment cabinet index */
    ++(p_fci_internal->pccab->iCab);
    /* get name of next cabinet */
    p_fci_internal->estimatedCabinetSize=p_fci_internal->statusFolderTotal;
    if (!(*pfnfcignc)(p_fci_internal->pccab,
        p_fci_internal->estimatedCabinetSize,/* estimated size of cab */
        p_fci_internal->pv)) {
      /* error handling */
      set_error( p_fci_internal, FCIERR_NONE, ERROR_FUNCTION_FAILED );
      return FALSE;
    }
    /* Skip a few lines of code. This is caught by the next if. */
    p_fci_internal->fGetNextCabInVain=TRUE;
  }

  if( p_fci_internal->fGetNextCabInVain &&
      p_fci_internal->fNextCab
  ) {
    /* THIS CAN NEVER HAPPEN */
    set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
    return FALSE;
  }

  /* too much data for cabinet */
  if( (p_fci_internal->fGetNextCabInVain ||
      p_fci_internal->fNextCab) && (
      p_fci_internal->ccab.cb < read_result +
      strlen(p_fci_internal->pccab->szCab)+1+
      strlen(p_fci_internal->pccab->szDisk)+1
  )) {

    p_fci_internal->fGetNextCabInVain=FALSE;
    p_fci_internal->fNextCab=TRUE;
    return fci_flush_cabinet( p_fci_internal, FALSE, pfnfcignc, pfnfcis);
  }

  if( p_fci_internal->fNextCab ) {
    /* THIS MAY NEVER HAPPEN */
    /* set error code */
    set_error( p_fci_internal, FCIERR_NONE, ERROR_GEN_FAILURE );
    return FALSE;
  }

  /* if the FolderThreshold has been reached flush the folder automatically */
  if (p_fci_internal->cCompressedBytesInFolder >= p_fci_internal->ccab.cbFolderThresh)
      return FCIFlushFolder(hfci, pfnfcignc, pfnfcis);

  return TRUE;
} /* end of FCIAddFile */





/***********************************************************************
 *		FCIFlushFolder (CABINET.12)
 *
 * FCIFlushFolder completes the CFFolder structure under construction.
 *
 * All further data which is added by FCIAddFile will be associated to
 * the next CFFolder structure.
 *
 * FCIFlushFolder will be called by FCIAddFile automatically if the
 * threshold (stored in the member cbFolderThresh of the CCAB structure
 * pccab passed to FCICreate) is exceeded.
 *
 * FCIFlushFolder will be called by FCIFlushFolder automatically before
 * any data will be written into the cabinet file.
 *
 * PARAMS
 *   hfci          [I]  An HFCI from FCICreate
 *   pfnfcignc     [I]  A pointer to a function which gets information about
 *                      the next cabinet
 *   pfnfcis      [IO]  A pointer to a function which will report status
 *                      information about the compression process
 *
 * RETURNS
 *   On success, returns TRUE
 *   On failure, returns FALSE
 *
 * INCLUDES
 *   fci.h
 *
 */
BOOL __cdecl FCIFlushFolder(
	HFCI                  hfci,
	PFNFCIGETNEXTCABINET  pfnfcignc,
	PFNFCISTATUS          pfnfcis)
{
    FCI_Int *p_fci_internal = get_fci_ptr( hfci );

    if (!p_fci_internal) return FALSE;
    return fci_flush_folder(p_fci_internal,FALSE,pfnfcignc,pfnfcis);
}



/***********************************************************************
 *		FCIFlushCabinet (CABINET.13)
 *
 * FCIFlushCabinet stores the data which has been added by FCIAddFile
 * into the cabinet file. If the maximum cabinet size (stored in the
 * member cb of the CCAB structure pccab passed to FCICreate) has been
 * exceeded FCIFlushCabinet will be called automatic by FCIAddFile.
 * The remaining data still has to be flushed manually by calling
 * FCIFlushCabinet.
 *
 * After FCIFlushCabinet has been called (manually) FCIAddFile must
 * NOT be called again. Then hfci has to be released by FCIDestroy.
 *
 * PARAMS
 *   hfci          [I]  An HFCI from FCICreate
 *   fGetNextCab   [I]  Whether you want to add additional files to a
 *                      cabinet set (TRUE) or whether you want to
 *                      finalize it (FALSE)
 *   pfnfcignc     [I]  A pointer to a function which gets information about
 *                      the next cabinet
 *   pfnfcis      [IO]  A pointer to a function which will report status
 *                      information about the compression process
 *
 * RETURNS
 *   On success, returns TRUE
 *   On failure, returns FALSE
 *
 * INCLUDES
 *   fci.h
 *
 */
BOOL __cdecl FCIFlushCabinet(
	HFCI                  hfci,
	BOOL                  fGetNextCab,
	PFNFCIGETNEXTCABINET  pfnfcignc,
	PFNFCISTATUS          pfnfcis)
{
  FCI_Int *p_fci_internal = get_fci_ptr( hfci );

  if (!p_fci_internal) return FALSE;

  if(!fci_flush_cabinet(p_fci_internal,fGetNextCab,pfnfcignc,pfnfcis)) return FALSE;

  while( p_fci_internal->files_size>0 ||
         p_fci_internal->placed_files_size>0 ) {
    if(!fci_flush_cabinet(p_fci_internal,fGetNextCab,pfnfcignc,pfnfcis)) return FALSE;
  }

  return TRUE;
}


/***********************************************************************
 *		FCIDestroy (CABINET.14)
 *
 * Frees a handle created by FCICreate.
 * Only reason for failure would be an invalid handle.
 *
 * PARAMS
 *   hfci [I] The HFCI to free
 *
 * RETURNS
 *   TRUE for success
 *   FALSE for failure
 */
BOOL __cdecl FCIDestroy(HFCI hfci)
{
    struct folder *folder, *folder_next;
    struct file *file, *file_next;
    struct data_block *block, *block_next;
    FCI_Int *p_fci_internal = get_fci_ptr( hfci );

    if (!p_fci_internal) return FALSE;

    /* before hfci can be removed all temporary files must be closed */
    /* and deleted */
    p_fci_internal->magic = 0;

    LIST_FOR_EACH_ENTRY_SAFE( folder, folder_next, &p_fci_internal->folders_list, struct folder, entry )
    {
        free_folder( p_fci_internal, folder );
    }
    LIST_FOR_EACH_ENTRY_SAFE( file, file_next, &p_fci_internal->files_list, struct file, entry )
    {
        free_file( p_fci_internal, file );
    }
    LIST_FOR_EACH_ENTRY_SAFE( block, block_next, &p_fci_internal->blocks_list, struct data_block, entry )
    {
        free_data_block( p_fci_internal, block );
    }

    close_temp_file( p_fci_internal, &p_fci_internal->data );

    /* hfci can now be removed */
    p_fci_internal->free(hfci);
    return TRUE;
}
