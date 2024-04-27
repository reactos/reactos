/*
 * File Decompression Interface
 *
 * Copyright 2000-2002 Stuart Caie
 * Copyright 2002 Patrik Stridvall
 * Copyright 2003 Greg Turner
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
 *
 * This is a largely redundant reimplementation of the stuff in cabextract.c.  It
 * would be theoretically preferable to have only one, shared implementation, however
 * there are semantic differences which may discourage efforts to unify the two.  It
 * should be possible, if awkward, to go back and reimplement cabextract.c using FDI.
 * But this approach would be quite a bit less performant.  Probably a better way
 * would be to create a "library" of routines in cabextract.c which do the actual
 * decompression, and have both fdi.c and cabextract share those routines.  The rest
 * of the code is not sufficiently similar to merit a shared implementation.
 *
 * The worst thing about this API is the bug.  "The bug" is this: when you extract a
 * cabinet, it /always/ informs you (via the hasnext field of PFDICABINETINFO), that
 * there is no subsequent cabinet, even if there is one.  wine faithfully reproduces
 * this behavior.
 *
 * TODO:
 *
 * Wine does not implement the AFAIK undocumented "enumerate" callback during
 * FDICopy.  It is implemented in Windows and therefore worth investigating...
 *
 * Lots of pointers flying around here... am I leaking RAM?
 *
 * WTF is FDITruncate?
 *
 * Probably, I need to weed out some dead code-paths.
 *
 * Test unit(s).
 *
 * The fdintNEXT_CABINET callbacks are probably not working quite as they should.
 * There are several FIXMEs in the source describing some of the deficiencies in
 * some detail.  Additionally, we do not do a very good job of returning the right
 * error codes to this callback.
 *
 * FDICopy and fdi_decomp are incomprehensibly large; separating these into smaller
 * functions would be nice.
 *
 *   -gmt
 */

#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "fdi.h"
#include "cabinet.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cabinet);

THOSE_ZIP_CONSTS;

struct fdi_file {
  struct fdi_file *next;               /* next file in sequence          */
  LPSTR filename;                     /* output name of file            */
  int    fh;                           /* open file handle or NULL       */
  cab_ULONG length;                    /* uncompressed length of file    */
  cab_ULONG offset;                    /* uncompressed offset in folder  */
  cab_UWORD index;                     /* magic index number of folder   */
  cab_UWORD time, date, attribs;       /* MS-DOS time/date/attributes    */
  BOOL oppressed;                      /* never to be processed          */
};

struct fdi_folder {
  struct fdi_folder *next;
  cab_off_t offset;                    /* offset to data blocks (32 bit) */
  cab_UWORD comp_type;                 /* compression format/window size */
  cab_ULONG comp_size;                 /* compressed size of folder      */
  cab_UBYTE num_splits;                /* number of split blocks + 1     */
  cab_UWORD num_blocks;                /* total number of blocks         */
};

/*
 * this structure fills the gaps between what is available in a PFDICABINETINFO
 * vs what is needed by FDICopy.  Memory allocated for these becomes the responsibility
 * of the caller to free.  Yes, I am aware that this is totally, utterly inelegant.
 * To make things even more unnecessarily confusing, we now attach these to the
 * fdi_decomp_state.
 */
typedef struct {
   char *prevname, *previnfo;
   char *nextname, *nextinfo;
   BOOL hasnext;  /* bug free indicator */
   int folder_resv, header_resv;
   cab_UBYTE block_resv;
} MORE_ISCAB_INFO, *PMORE_ISCAB_INFO;

typedef struct
{
  unsigned int magic;
  PFNALLOC     alloc;
  PFNFREE      free;
  PFNOPEN      open;
  PFNREAD      read;
  PFNWRITE     write;
  PFNCLOSE     close;
  PFNSEEK      seek;
  PERF         perf;
} FDI_Int;

#define FDI_INT_MAGIC 0xfdfdfd05

/*
 * ugh, well, this ended up being pretty damn silly...
 * now that I've conceded to build equivalent structures to struct cab.*,
 * I should have just used those, or, better yet, unified the two... sue me.
 * (Note to Microsoft: That's a joke.  Please /don't/ actually sue me! -gmt).
 * Nevertheless, I've come this far, it works, so I'm not gonna change it
 * for now.  This implementation has significant semantic differences anyhow.
 */

typedef struct fdi_cds_fwd {
  FDI_Int *fdi;                    /* the hfdi we are using                 */
  INT_PTR filehf, cabhf;           /* file handle we are using              */
  struct fdi_folder *current;      /* current folder we're extracting from  */
  cab_ULONG offset;                /* uncompressed offset within folder     */
  cab_UBYTE *outpos;               /* (high level) start of data to use up  */
  cab_UWORD outlen;                /* (high level) amount of data to use up */
  int (*decompress)(int, int, struct fdi_cds_fwd *); /* chosen compress fn  */
  cab_UBYTE inbuf[CAB_INPUTMAX+2]; /* +2 for lzx bitbuffer overflows!       */
  cab_UBYTE outbuf[CAB_BLOCKMAX];
  union {
    struct ZIPstate zip;
    struct QTMstate qtm;
    struct LZXstate lzx;
  } methods;
  /* some temp variables for use during decompression */
  cab_UBYTE q_length_base[27], q_length_extra[27], q_extra_bits[42];
  cab_ULONG q_position_base[42];
  cab_ULONG lzx_position_base[51];
  cab_UBYTE extra_bits[51];
  USHORT  setID;                   /* Cabinet set ID */
  USHORT  iCabinet;                /* Cabinet number in set (0 based) */
  struct fdi_cds_fwd *decomp_cab;
  MORE_ISCAB_INFO mii;
  struct fdi_folder *firstfol; 
  struct fdi_file   *firstfile;
  struct fdi_cds_fwd *next;
} fdi_decomp_state;

#define ZIPNEEDBITS(n) {while(k<(n)){cab_LONG c=*(ZIP(inpos)++);\
    b|=((cab_ULONG)c)<<k;k+=8;}}
#define ZIPDUMPBITS(n) {b>>=(n);k-=(n);}

/* endian-neutral reading of little-endian data */
#define EndGetI32(a)  ((((a)[3])<<24)|(((a)[2])<<16)|(((a)[1])<<8)|((a)[0]))
#define EndGetI16(a)  ((((a)[1])<<8)|((a)[0]))

#define CAB(x) (decomp_state->x)
#define ZIP(x) (decomp_state->methods.zip.x)
#define QTM(x) (decomp_state->methods.qtm.x)
#define LZX(x) (decomp_state->methods.lzx.x)
#define DECR_OK           (0)
#define DECR_DATAFORMAT   (1)
#define DECR_ILLEGALDATA  (2)
#define DECR_NOMEMORY     (3)
#define DECR_CHECKSUM     (4)
#define DECR_INPUT        (5)
#define DECR_OUTPUT       (6)
#define DECR_USERABORT    (7)

static void set_error( FDI_Int *fdi, int oper, int err )
{
    fdi->perf->erfOper = oper;
    fdi->perf->erfType = err;
    fdi->perf->fError = TRUE;
    if (err) SetLastError( err );
}

static FDI_Int *get_fdi_ptr( HFDI hfdi )
{
    FDI_Int *fdi= (FDI_Int *)hfdi;

    if (!fdi || fdi->magic != FDI_INT_MAGIC)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return NULL;
    }
    return fdi;
}

/****************************************************************
 * QTMupdatemodel (internal)
 */
static void QTMupdatemodel(struct QTMmodel *model, int sym) {
  struct QTMmodelsym temp;
  int i, j;

  for (i = 0; i < sym; i++) model->syms[i].cumfreq += 8;

  if (model->syms[0].cumfreq > 3800) {
    if (--model->shiftsleft) {
      for (i = model->entries - 1; i >= 0; i--) {
        /* -1, not -2; the 0 entry saves this */
        model->syms[i].cumfreq >>= 1;
        if (model->syms[i].cumfreq <= model->syms[i+1].cumfreq) {
          model->syms[i].cumfreq = model->syms[i+1].cumfreq + 1;
        }
      }
    }
    else {
      model->shiftsleft = 50;
      for (i = 0; i < model->entries ; i++) {
        /* no -1, want to include the 0 entry */
        /* this converts cumfreqs into frequencies, then shifts right */
        model->syms[i].cumfreq -= model->syms[i+1].cumfreq;
        model->syms[i].cumfreq++; /* avoid losing things entirely */
        model->syms[i].cumfreq >>= 1;
      }

      /* now sort by frequencies, decreasing order -- this must be an
       * inplace selection sort, or a sort with the same (in)stability
       * characteristics
       */
      for (i = 0; i < model->entries - 1; i++) {
        for (j = i + 1; j < model->entries; j++) {
          if (model->syms[i].cumfreq < model->syms[j].cumfreq) {
            temp = model->syms[i];
            model->syms[i] = model->syms[j];
            model->syms[j] = temp;
          }
        }
      }

      /* then convert frequencies back to cumfreq */
      for (i = model->entries - 1; i >= 0; i--) {
        model->syms[i].cumfreq += model->syms[i+1].cumfreq;
      }
      /* then update the other part of the table */
      for (i = 0; i < model->entries; i++) {
        model->tabloc[model->syms[i].sym] = i;
      }
    }
  }
}

/*************************************************************************
 * make_decode_table (internal)
 *
 * This function was coded by David Tritscher. It builds a fast huffman
 * decoding table out of just a canonical huffman code lengths table.
 *
 * PARAMS
 *   nsyms:  total number of symbols in this huffman tree.
 *   nbits:  any symbols with a code length of nbits or less can be decoded
 *           in one lookup of the table.
 *   length: A table to get code lengths from [0 to syms-1]
 *   table:  The table to fill up with decoded symbols and pointers.
 *
 * RETURNS
 *   OK:    0
 *   error: 1
 */
static int make_decode_table(cab_ULONG nsyms, cab_ULONG nbits,
                             const cab_UBYTE *length, cab_UWORD *table) {
  register cab_UWORD sym;
  register cab_ULONG leaf;
  register cab_UBYTE bit_num = 1;
  cab_ULONG fill;
  cab_ULONG pos         = 0; /* the current position in the decode table */
  cab_ULONG table_mask  = 1 << nbits;
  cab_ULONG bit_mask    = table_mask >> 1; /* don't do 0 length codes */
  cab_ULONG next_symbol = bit_mask; /* base of allocation for long codes */

  /* fill entries for codes short enough for a direct mapping */
  while (bit_num <= nbits) {
    for (sym = 0; sym < nsyms; sym++) {
      if (length[sym] == bit_num) {
        leaf = pos;

        if((pos += bit_mask) > table_mask) return 1; /* table overrun */

        /* fill all possible lookups of this symbol with the symbol itself */
        fill = bit_mask;
        while (fill-- > 0) table[leaf++] = sym;
      }
    }
    bit_mask >>= 1;
    bit_num++;
  }

  /* if there are any codes longer than nbits */
  if (pos != table_mask) {
    /* clear the remainder of the table */
    for (sym = pos; sym < table_mask; sym++) table[sym] = 0;

    /* give ourselves room for codes to grow by up to 16 more bits */
    pos <<= 16;
    table_mask <<= 16;
    bit_mask = 1 << 15;

    while (bit_num <= 16) {
      for (sym = 0; sym < nsyms; sym++) {
        if (length[sym] == bit_num) {
          leaf = pos >> 16;
          for (fill = 0; fill < bit_num - nbits; fill++) {
            /* if this path hasn't been taken yet, 'allocate' two entries */
            if (table[leaf] == 0) {
              table[(next_symbol << 1)] = 0;
              table[(next_symbol << 1) + 1] = 0;
              table[leaf] = next_symbol++;
            }
            /* follow the path and select either left or right for next bit */
            leaf = table[leaf] << 1;
            if ((pos >> (15-fill)) & 1) leaf++;
          }
          table[leaf] = sym;

          if ((pos += bit_mask) > table_mask) return 1; /* table overflow */
        }
      }
      bit_mask >>= 1;
      bit_num++;
    }
  }

  /* full table? */
  if (pos == table_mask) return 0;

  /* either erroneous table, or all elements are 0 - let's find out. */
  for (sym = 0; sym < nsyms; sym++) if (length[sym]) return 1;
  return 0;
}

/*************************************************************************
 * checksum (internal)
 */
static cab_ULONG checksum(const cab_UBYTE *data, cab_UWORD bytes, cab_ULONG csum) {
  int len;
  cab_ULONG ul = 0;

  for (len = bytes >> 2; len--; data += 4) {
    csum ^= ((data[0]) | (data[1]<<8) | (data[2]<<16) | (data[3]<<24));
  }

  switch (bytes & 3) {
  case 3: ul |= *data++ << 16;
  /* fall through */
  case 2: ul |= *data++ <<  8;
  /* fall through */
  case 1: ul |= *data;
  }
  csum ^= ul;

  return csum;
}

/***********************************************************************
 *		FDICreate (CABINET.20)
 *
 * Provided with several callbacks (all of them are mandatory),
 * returns a handle which can be used to perform operations
 * on cabinet files.
 *
 * PARAMS
 *   pfnalloc [I]  A pointer to a function which allocates ram.  Uses
 *                 the same interface as malloc.
 *   pfnfree  [I]  A pointer to a function which frees ram.  Uses the
 *                 same interface as free.
 *   pfnopen  [I]  A pointer to a function which opens a file.  Uses
 *                 the same interface as _open.
 *   pfnread  [I]  A pointer to a function which reads from a file into
 *                 a caller-provided buffer.  Uses the same interface
 *                 as _read
 *   pfnwrite [I]  A pointer to a function which writes to a file from
 *                 a caller-provided buffer.  Uses the same interface
 *                 as _write.
 *   pfnclose [I]  A pointer to a function which closes a file handle.
 *                 Uses the same interface as _close.
 *   pfnseek  [I]  A pointer to a function which seeks in a file.
 *                 Uses the same interface as _lseek.
 *   cpuType  [I]  The type of CPU; ignored in wine (recommended value:
 *                 cpuUNKNOWN, aka -1).
 *   perf     [IO] A pointer to an ERF structure.  When FDICreate
 *                 returns an error condition, error information may
 *                 be found here as well as from GetLastError.
 *
 * RETURNS
 *   On success, returns an FDI handle of type HFDI.
 *   On failure, the NULL file handle is returned. Error
 *   info can be retrieved from perf.
 *
 * INCLUDES
 *   fdi.h
 * 
 */
HFDI __cdecl FDICreate(
	PFNALLOC pfnalloc,
	PFNFREE  pfnfree,
	PFNOPEN  pfnopen,
	PFNREAD  pfnread,
	PFNWRITE pfnwrite,
	PFNCLOSE pfnclose,
	PFNSEEK  pfnseek,
	int      cpuType,
	PERF     perf)
{
  FDI_Int *fdi;

  TRACE("(pfnalloc == ^%p, pfnfree == ^%p, pfnopen == ^%p, pfnread == ^%p, pfnwrite == ^%p, "
        "pfnclose == ^%p, pfnseek == ^%p, cpuType == %d, perf == ^%p)\n",
        pfnalloc, pfnfree, pfnopen, pfnread, pfnwrite, pfnclose, pfnseek,
        cpuType, perf);

  if ((!pfnalloc) || (!pfnfree)) {
    perf->erfOper = FDIERROR_NONE;
    perf->erfType = ERROR_BAD_ARGUMENTS;
    perf->fError = TRUE;

    SetLastError(ERROR_BAD_ARGUMENTS);
    return NULL;
  }

  if (!((fdi = pfnalloc(sizeof(FDI_Int))))) {
    perf->erfOper = FDIERROR_ALLOC_FAIL;
    perf->erfType = 0;
    perf->fError = TRUE;
    return NULL;
  }

  fdi->magic = FDI_INT_MAGIC;
  fdi->alloc = pfnalloc;
  fdi->free  = pfnfree;
  fdi->open  = pfnopen;
  fdi->read  = pfnread;
  fdi->write = pfnwrite;
  fdi->close = pfnclose;
  fdi->seek  = pfnseek;
  /* no-brainer: we ignore the cpu type; this is only used
     for the 16-bit versions in Windows anyhow... */
  fdi->perf = perf;

  return (HFDI)fdi;
}

/*******************************************************************
 * FDI_getoffset (internal)
 *
 * returns the file pointer position of a file handle.
 */
static LONG FDI_getoffset(FDI_Int *fdi, INT_PTR hf)
{
  return fdi->seek(hf, 0, SEEK_CUR);
}

/**********************************************************************
 * FDI_read_string (internal)
 *
 * allocate and read an arbitrarily long string from the cabinet
 */
static char *FDI_read_string(FDI_Int *fdi, INT_PTR hf, long cabsize)
{
  size_t len=256,
         base = FDI_getoffset(fdi, hf),
         maxlen = cabsize - base;
  BOOL ok = FALSE;
  unsigned int i;
  cab_UBYTE *buf = NULL;

  TRACE("(fdi == %p, hf == %Id, cabsize == %ld)\n", fdi, hf, cabsize);

  do {
    if (len > maxlen) len = maxlen;
    if (!(buf = fdi->alloc(len))) break;
    if (!fdi->read(hf, buf, len)) break;

    /* search for a null terminator in what we've just read */
    for (i=0; i < len; i++) {
      if (!buf[i]) {ok=TRUE; break;}
    }

    if (!ok) {
      if (len == maxlen) {
        ERR("cabinet is truncated\n");
        break;
      }
      /* The buffer is too small for the string. Reset the file to the point
       * where we started, free the buffer and increase the size for the next try
       */
      fdi->seek(hf, base, SEEK_SET);
      fdi->free(buf);
      buf = NULL;
      len *= 2;
    }
  } while (!ok);

  if (!ok) {
    if (buf)
      fdi->free(buf);
    else
      ERR("out of memory!\n");
    return NULL;
  }

  /* otherwise, set the stream to just after the string and return */
  fdi->seek(hf, base + strlen((char *)buf) + 1, SEEK_SET);

  return (char *) buf;
}

/******************************************************************
 * FDI_read_entries (internal)
 *
 * process the cabinet header in the style of FDIIsCabinet, but
 * without the sanity checks (and bug)
 */
static BOOL FDI_read_entries(
        FDI_Int         *fdi,
        INT_PTR          hf,
        PFDICABINETINFO  pfdici,
        PMORE_ISCAB_INFO pmii)
{
  int num_folders, num_files, header_resv, folder_resv = 0;
  LONG cabsize;
  USHORT setid, cabidx, flags;
  cab_UBYTE buf[64], block_resv;
  char *prevname = NULL, *previnfo = NULL, *nextname = NULL, *nextinfo = NULL;

  TRACE("(fdi == ^%p, hf == %Id, pfdici == ^%p)\n", fdi, hf, pfdici);

  /* read in the CFHEADER */
  if (fdi->read(hf, buf, cfhead_SIZEOF) != cfhead_SIZEOF) {
    if (pmii) set_error( fdi, FDIERROR_NOT_A_CABINET, 0 );
    return FALSE;
  }

  /* check basic MSCF signature */
  if (EndGetI32(buf+cfhead_Signature) != 0x4643534d) {
    if (pmii) set_error( fdi, FDIERROR_NOT_A_CABINET, 0 );
    return FALSE;
  }

  /* get the cabinet size */
  cabsize = EndGetI32(buf+cfhead_CabinetSize);

  /* get the number of folders */
  num_folders = EndGetI16(buf+cfhead_NumFolders);

  /* get the number of files */
  num_files = EndGetI16(buf+cfhead_NumFiles);

  /* setid */
  setid = EndGetI16(buf+cfhead_SetID);

  /* cabinet (set) index */
  cabidx = EndGetI16(buf+cfhead_CabinetIndex);

  /* check the header revision */
  if ((buf[cfhead_MajorVersion] > 1) ||
      (buf[cfhead_MajorVersion] == 1 && buf[cfhead_MinorVersion] > 3))
  {
    WARN("cabinet format version > 1.3\n");
    if (pmii) set_error( fdi, FDIERROR_UNKNOWN_CABINET_VERSION, 0 /* ? */ );
    return FALSE;
  }

  /* pull the flags out */
  flags = EndGetI16(buf+cfhead_Flags);

  /* read the reserved-sizes part of header, if present */
  if (flags & cfheadRESERVE_PRESENT) {
    if (fdi->read(hf, buf, cfheadext_SIZEOF) != cfheadext_SIZEOF) {
      ERR("bunk reserve-sizes?\n");
      if (pmii) set_error( fdi, FDIERROR_CORRUPT_CABINET, 0 /* ? */ );
      return FALSE;
    }

    header_resv = EndGetI16(buf+cfheadext_HeaderReserved);
    if (pmii) pmii->header_resv = header_resv;
    folder_resv = buf[cfheadext_FolderReserved];
    if (pmii) pmii->folder_resv = folder_resv;
    block_resv  = buf[cfheadext_DataReserved];
    if (pmii) pmii->block_resv = block_resv;

    if (header_resv > 60000) {
      WARN("WARNING; header reserved space > 60000\n");
    }

    /* skip the reserved header */
    if ((header_resv) && (fdi->seek(hf, header_resv, SEEK_CUR) == -1)) {
      ERR("seek failure: header_resv\n");
      if (pmii) set_error( fdi, FDIERROR_CORRUPT_CABINET, 0 /* ? */ );
      return FALSE;
    }
  }

  if (flags & cfheadPREV_CABINET) {
    prevname = FDI_read_string(fdi, hf, cabsize);
    if (!prevname) {
      if (pmii) set_error( fdi, FDIERROR_CORRUPT_CABINET, 0 /* ? */ );
      return FALSE;
    } else
      if (pmii)
        pmii->prevname = prevname;
      else
        fdi->free(prevname);
    previnfo = FDI_read_string(fdi, hf, cabsize);
    if (previnfo) {
      if (pmii) 
        pmii->previnfo = previnfo;
      else
        fdi->free(previnfo);
    }
  }

  if (flags & cfheadNEXT_CABINET) {
    if (pmii)
      pmii->hasnext = TRUE;
    nextname = FDI_read_string(fdi, hf, cabsize);
    if (!nextname) {
      if ((flags & cfheadPREV_CABINET) && pmii) {
        if (pmii->prevname) fdi->free(prevname);
        if (pmii->previnfo) fdi->free(previnfo);
      }
      set_error( fdi, FDIERROR_CORRUPT_CABINET, 0 /* ? */ );
      return FALSE;
    } else
      if (pmii)
        pmii->nextname = nextname;
      else
        fdi->free(nextname);
    nextinfo = FDI_read_string(fdi, hf, cabsize);
    if (nextinfo) {
      if (pmii)
        pmii->nextinfo = nextinfo;
      else
        fdi->free(nextinfo);
    }
  }

  /* we could process the whole cabinet searching for problems;
     instead lets stop here.  Now let's fill out the paperwork */
  pfdici->cbCabinet = cabsize;
  pfdici->cFolders  = num_folders;
  pfdici->cFiles    = num_files;
  pfdici->setID     = setid;
  pfdici->iCabinet  = cabidx;
  pfdici->fReserve  = (flags & cfheadRESERVE_PRESENT) != 0;
  pfdici->hasprev   = (flags & cfheadPREV_CABINET) != 0;
  pfdici->hasnext   = (flags & cfheadNEXT_CABINET) != 0;
  return TRUE;
}

/***********************************************************************
 * FDIIsCabinet (CABINET.21)
 *
 * Informs the caller as to whether or not the provided file handle is
 * really a cabinet or not, filling out the provided PFDICABINETINFO
 * structure with information about the cabinet.  Brief explanations of
 * the elements of this structure are available as comments accompanying
 * its definition in wine's include/fdi.h.
 *
 * PARAMS
 *   hfdi   [I]  An HFDI from FDICreate
 *   hf     [I]  The file handle about which the caller inquires
 *   pfdici [IO] Pointer to a PFDICABINETINFO structure which will
 *               be filled out with information about the cabinet
 *               file indicated by hf if, indeed, it is determined
 *               to be a cabinet.
 *
 * RETURNS
 *   TRUE  if the file is a cabinet.  The info pointed to by pfdici will
 *         be provided.
 *   FALSE if the file is not a cabinet, or if an error was encountered
 *         while processing the cabinet.  The PERF structure provided to
 *         FDICreate can be queried for more error information.
 *
 * INCLUDES
 *   fdi.c
 */
BOOL __cdecl FDIIsCabinet(HFDI hfdi, INT_PTR hf, PFDICABINETINFO pfdici)
{
  BOOL rv;
  FDI_Int *fdi = get_fdi_ptr( hfdi );

  TRACE("(hfdi == ^%p, hf == ^%Id, pfdici == ^%p)\n", hfdi, hf, pfdici);

  if (!fdi) return FALSE;

  if (!pfdici) {
    SetLastError(ERROR_BAD_ARGUMENTS);
    return FALSE;
  }
  rv = FDI_read_entries(fdi, hf, pfdici, NULL);

  if (rv)
    pfdici->hasnext = FALSE; /* yuck. duplicate apparent cabinet.dll bug */

  return rv;
}

/******************************************************************
 * QTMfdi_initmodel (internal)
 *
 * Initialize a model which decodes symbols from [s] to [s]+[n]-1
 */
static void QTMfdi_initmodel(struct QTMmodel *m, struct QTMmodelsym *sym, int n, int s) {
  int i;
  m->shiftsleft = 4;
  m->entries    = n;
  m->syms       = sym;
  memset(m->tabloc, 0xFF, sizeof(m->tabloc)); /* clear out look-up table */
  for (i = 0; i < n; i++) {
    m->tabloc[i+s]     = i;   /* set up a look-up entry for symbol */
    m->syms[i].sym     = i+s; /* actual symbol */
    m->syms[i].cumfreq = n-i; /* current frequency of that symbol */
  }
  m->syms[n].cumfreq = 0;
}

/******************************************************************
 * QTMfdi_init (internal)
 */
static int QTMfdi_init(int window, int level, fdi_decomp_state *decomp_state) {
  unsigned int wndsize = 1 << window;
  int msz = window * 2, i;
  cab_ULONG j;

  /* QTM supports window sizes of 2^10 (1Kb) through 2^21 (2Mb) */
  /* if a previously allocated window is big enough, keep it    */
  if (window < 10 || window > 21) return DECR_DATAFORMAT;
  if (QTM(actual_size) < wndsize) {
    if (QTM(window)) CAB(fdi)->free(QTM(window));
    QTM(window) = NULL;
  }
  if (!QTM(window)) {
    if (!(QTM(window) = CAB(fdi)->alloc(wndsize))) return DECR_NOMEMORY;
    QTM(actual_size) = wndsize;
  }
  QTM(window_size) = wndsize;
  QTM(window_posn) = 0;

  /* initialize static slot/extrabits tables */
  for (i = 0, j = 0; i < 27; i++) {
    CAB(q_length_extra)[i] = (i == 26) ? 0 : (i < 2 ? 0 : i - 2) >> 2;
    CAB(q_length_base)[i] = j; j += 1 << ((i == 26) ? 5 : CAB(q_length_extra)[i]);
  }
  for (i = 0, j = 0; i < 42; i++) {
    CAB(q_extra_bits)[i] = (i < 2 ? 0 : i-2) >> 1;
    CAB(q_position_base)[i] = j; j += 1 << CAB(q_extra_bits)[i];
  }

  /* initialize arithmetic coding models */

  QTMfdi_initmodel(&QTM(model7), QTM(m7sym), 7, 0);

  QTMfdi_initmodel(&QTM(model00), QTM(m00sym), 0x40, 0x00);
  QTMfdi_initmodel(&QTM(model40), QTM(m40sym), 0x40, 0x40);
  QTMfdi_initmodel(&QTM(model80), QTM(m80sym), 0x40, 0x80);
  QTMfdi_initmodel(&QTM(modelC0), QTM(mC0sym), 0x40, 0xC0);

  /* model 4 depends on table size, ranges from 20 to 24  */
  QTMfdi_initmodel(&QTM(model4), QTM(m4sym), (msz < 24) ? msz : 24, 0);
  /* model 5 depends on table size, ranges from 20 to 36  */
  QTMfdi_initmodel(&QTM(model5), QTM(m5sym), (msz < 36) ? msz : 36, 0);
  /* model 6pos depends on table size, ranges from 20 to 42 */
  QTMfdi_initmodel(&QTM(model6pos), QTM(m6psym), msz, 0);
  QTMfdi_initmodel(&QTM(model6len), QTM(m6lsym), 27, 0);

  return DECR_OK;
}

/************************************************************
 * LZXfdi_init (internal)
 */
static int LZXfdi_init(int window, fdi_decomp_state *decomp_state) {
  static const cab_UBYTE bits[]  =
                        { 0,  0,  0,  0,  1,  1,  2,  2,  3,  3,  4,  4,  5,  5,  6,  6,
                          7,  7,  8,  8,  9,  9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14,
                         15, 15, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
                         17, 17, 17};
  static const cab_ULONG base[] =
                {      0,       1,       2,       3,       4,       6,       8,      12,
                      16,      24,      32,      48,      64,      96,     128,     192,
                     256,     384,     512,     768,    1024,    1536,    2048,    3072,
                    4096,    6144,    8192,   12288,   16384,   24576,   32768,   49152,
                   65536,   98304,  131072,  196608,  262144,  393216,  524288,  655360,
                  786432,  917504, 1048576, 1179648, 1310720, 1441792, 1572864, 1703936,
                 1835008, 1966080, 2097152};
  cab_ULONG wndsize = 1 << window;
  int posn_slots;

  /* LZX supports window sizes of 2^15 (32Kb) through 2^21 (2Mb) */
  /* if a previously allocated window is big enough, keep it     */
  if (window < 15 || window > 21) return DECR_DATAFORMAT;
  if (LZX(actual_size) < wndsize) {
    if (LZX(window)) CAB(fdi)->free(LZX(window));
    LZX(window) = NULL;
  }
  if (!LZX(window)) {
    if (!(LZX(window) = CAB(fdi)->alloc(wndsize))) return DECR_NOMEMORY;
    LZX(actual_size) = wndsize;
  }
  LZX(window_size) = wndsize;

  /* initialize static tables */
  memcpy(CAB(extra_bits), bits, sizeof(bits));
  memcpy(CAB(lzx_position_base), base, sizeof(base));

  /* calculate required position slots */
  if (window == 20) posn_slots = 42;
  else if (window == 21) posn_slots = 50;
  else posn_slots = window << 1;

  /*posn_slots=i=0; while (i < wndsize) i += 1 << CAB(extra_bits)[posn_slots++]; */

  LZX(R0)  =  LZX(R1)  = LZX(R2) = 1;
  LZX(main_elements)   = LZX_NUM_CHARS + (posn_slots << 3);
  LZX(header_read)     = 0;
  LZX(frames_read)     = 0;
  LZX(block_remaining) = 0;
  LZX(block_type)      = LZX_BLOCKTYPE_INVALID;
  LZX(intel_curpos)    = 0;
  LZX(intel_started)   = 0;
  LZX(window_posn)     = 0;

  /* initialize tables to 0 (because deltas will be applied to them) */
  memset(LZX(MAINTREE_len), 0, sizeof(LZX(MAINTREE_len)));
  memset(LZX(LENGTH_len), 0, sizeof(LZX(LENGTH_len)));

  return DECR_OK;
}

/****************************************************
 * NONEfdi_decomp(internal)
 */
static int NONEfdi_decomp(int inlen, int outlen, fdi_decomp_state *decomp_state)
{
  if (inlen != outlen) return DECR_ILLEGALDATA;
  if (outlen > CAB_BLOCKMAX) return DECR_DATAFORMAT;
  memcpy(CAB(outbuf), CAB(inbuf), (size_t) inlen);
  return DECR_OK;
}

/********************************************************
 * Ziphuft_free (internal)
 */
static void fdi_Ziphuft_free(FDI_Int *fdi, struct Ziphuft *t)
{
  register struct Ziphuft *p, *q;

  /* Go through linked list, freeing from the allocated (t[-1]) address. */
  p = t;
  while (p != NULL)
  {
    q = (--p)->v.t;
    fdi->free(p);
    p = q;
  } 
}

/*********************************************************
 * fdi_Ziphuft_build (internal)
 */
static cab_LONG fdi_Ziphuft_build(cab_ULONG *b, cab_ULONG n, cab_ULONG s, const cab_UWORD *d, const cab_UWORD *e,
struct Ziphuft **t, cab_LONG *m, fdi_decomp_state *decomp_state)
{
  cab_ULONG a;                   	/* counter for codes of length k */
  cab_ULONG el;                  	/* length of EOB code (value 256) */
  cab_ULONG f;                   	/* i repeats in table every f entries */
  cab_LONG g;                    	/* maximum code length */
  cab_LONG h;                    	/* table level */
  register cab_ULONG i;          	/* counter, current code */
  register cab_ULONG j;          	/* counter */
  register cab_LONG k;           	/* number of bits in current code */
  cab_LONG *l;                  	/* stack of bits per table */
  register cab_ULONG *p;         	/* pointer into ZIP(c)[],ZIP(b)[],ZIP(v)[] */
  register struct Ziphuft *q;           /* points to current table */
  struct Ziphuft r;                     /* table entry for structure assignment */
  register cab_LONG w;                  /* bits before this table == (l * h) */
  cab_ULONG *xp;                 	/* pointer into x */
  cab_LONG y;                           /* number of dummy codes added */
  cab_ULONG z;                   	/* number of entries in current table */

  l = ZIP(lx)+1;

  /* Generate counts for each bit length */
  el = n > 256 ? b[256] : ZIPBMAX; /* set length of EOB code, if any */

  for(i = 0; i < ZIPBMAX+1; ++i)
    ZIP(c)[i] = 0;
  p = b;  i = n;
  do
  {
    ZIP(c)[*p]++; p++;               /* assume all entries <= ZIPBMAX */
  } while (--i);
  if (ZIP(c)[0] == n)                /* null input--all zero length codes */
  {
    *t = NULL;
    *m = 0;
    return 0;
  }

  /* Find minimum and maximum length, bound *m by those */
  for (j = 1; j <= ZIPBMAX; j++)
    if (ZIP(c)[j])
      break;
  k = j;                        /* minimum code length */
  if ((cab_ULONG)*m < j)
    *m = j;
  for (i = ZIPBMAX; i; i--)
    if (ZIP(c)[i])
      break;
  g = i;                        /* maximum code length */
  if ((cab_ULONG)*m > i)
    *m = i;

  /* Adjust last length count to fill out codes, if needed */
  for (y = 1 << j; j < i; j++, y <<= 1)
    if ((y -= ZIP(c)[j]) < 0)
      return 2;                 /* bad input: more codes than bits */
  if ((y -= ZIP(c)[i]) < 0)
    return 2;
  ZIP(c)[i] += y;

  /* Generate starting offsets LONGo the value table for each length */
  ZIP(x)[1] = j = 0;
  p = ZIP(c) + 1;  xp = ZIP(x) + 2;
  while (--i)
  {                 /* note that i == g from above */
    *xp++ = (j += *p++);
  }

  /* Make a table of values in order of bit lengths */
  p = b;  i = 0;
  do{
    if ((j = *p++) != 0)
      ZIP(v)[ZIP(x)[j]++] = i;
  } while (++i < n);


  /* Generate the Huffman codes and for each, make the table entries */
  ZIP(x)[0] = i = 0;                 /* first Huffman code is zero */
  p = ZIP(v);                        /* grab values in bit order */
  h = -1;                       /* no tables yet--level -1 */
  w = l[-1] = 0;                /* no bits decoded yet */
  ZIP(u)[0] = NULL;             /* just to keep compilers happy */
  q = NULL;                     /* ditto */
  z = 0;                        /* ditto */

  /* go through the bit lengths (k already is bits in shortest code) */
  for (; k <= g; k++)
  {
    a = ZIP(c)[k];
    while (a--)
    {
      /* here i is the Huffman code of length k bits for value *p */
      /* make tables up to required level */
      while (k > w + l[h])
      {
        w += l[h++];            /* add bits already decoded */

        /* compute minimum size table less than or equal to *m bits */
        if ((z = g - w) > (cab_ULONG)*m)    /* upper limit */
          z = *m;
        if ((f = 1 << (j = k - w)) > a + 1)     /* try a k-w bit table */
        {                       /* too few codes for k-w bit table */
          f -= a + 1;           /* deduct codes from patterns left */
          xp = ZIP(c) + k;
          while (++j < z)       /* try smaller tables up to z bits */
          {
            if ((f <<= 1) <= *++xp)
              break;            /* enough codes to use up j bits */
            f -= *xp;           /* else deduct codes from patterns */
          }
        }
        if ((cab_ULONG)w + j > el && (cab_ULONG)w < el)
          j = el - w;           /* make EOB code end at table */
        z = 1 << j;             /* table entries for j-bit table */
        l[h] = j;               /* set table size in stack */

        /* allocate and link in new table */
        if (!(q = CAB(fdi)->alloc((z + 1)*sizeof(struct Ziphuft))))
        {
          if(h)
            fdi_Ziphuft_free(CAB(fdi), ZIP(u)[0]);
          return 3;             /* not enough memory */
        }
        *t = q + 1;             /* link to list for Ziphuft_free() */
        *(t = &(q->v.t)) = NULL;
        ZIP(u)[h] = ++q;             /* table starts after link */

        /* connect to last table, if there is one */
        if (h)
        {
          ZIP(x)[h] = i;              /* save pattern for backing up */
          r.b = (cab_UBYTE)l[h-1];    /* bits to dump before this table */
          r.e = (cab_UBYTE)(16 + j);  /* bits in this table */
          r.v.t = q;                  /* pointer to this table */
          j = (i & ((1 << w) - 1)) >> (w - l[h-1]);
          ZIP(u)[h-1][j] = r;        /* connect to last table */
        }
      }

      /* set up table entry in r */
      r.b = (cab_UBYTE)(k - w);
      if (p >= ZIP(v) + n)
        r.e = 99;               /* out of values--invalid code */
      else if (*p < s)
      {
        r.e = (cab_UBYTE)(*p < 256 ? 16 : 15);    /* 256 is end-of-block code */
        r.v.n = *p++;           /* simple code is just the value */
      }
      else
      {
        r.e = (cab_UBYTE)e[*p - s];   /* non-simple--look up in lists */
        r.v.n = d[*p++ - s];
      }

      /* fill code-like entries with r */
      f = 1 << (k - w);
      for (j = i >> w; j < z; j += f)
        q[j] = r;

      /* backwards increment the k-bit code i */
      for (j = 1 << (k - 1); i & j; j >>= 1)
        i ^= j;
      i ^= j;

      /* backup over finished tables */
      while ((i & ((1 << w) - 1)) != ZIP(x)[h])
        w -= l[--h];            /* don't need to update q */
    }
  }

  /* return actual size of base table */
  *m = l[0];

  /* Return true (1) if we were given an incomplete table */
  return y != 0 && g != 1;
}

/*********************************************************
 * fdi_Zipinflate_codes (internal)
 */
static cab_LONG fdi_Zipinflate_codes(const struct Ziphuft *tl, const struct Ziphuft *td,
  cab_LONG bl, cab_LONG bd, fdi_decomp_state *decomp_state)
{
  register cab_ULONG e;     /* table entry flag/number of extra bits */
  cab_ULONG n, d;           /* length and index for copy */
  cab_ULONG w;              /* current window position */
  const struct Ziphuft *t;  /* pointer to table entry */
  cab_ULONG ml, md;         /* masks for bl and bd bits */
  register cab_ULONG b;     /* bit buffer */
  register cab_ULONG k;     /* number of bits in bit buffer */

  /* make local copies of globals */
  b = ZIP(bb);                       /* initialize bit buffer */
  k = ZIP(bk);
  w = ZIP(window_posn);                       /* initialize window position */

  /* inflate the coded data */
  ml = Zipmask[bl];           	/* precompute masks for speed */
  md = Zipmask[bd];

  for(;;)
  {
    ZIPNEEDBITS((cab_ULONG)bl)
    if((e = (t = tl + (b & ml))->e) > 16)
      do
      {
        if (e == 99)
          return 1;
        ZIPDUMPBITS(t->b)
        e -= 16;
        ZIPNEEDBITS(e)
      } while ((e = (t = t->v.t + (b & Zipmask[e]))->e) > 16);
    ZIPDUMPBITS(t->b)
    if (e == 16)                /* then it's a literal */
      CAB(outbuf)[w++] = (cab_UBYTE)t->v.n;
    else                        /* it's an EOB or a length */
    {
      /* exit if end of block */
      if(e == 15)
        break;

      /* get length of block to copy */
      ZIPNEEDBITS(e)
      n = t->v.n + (b & Zipmask[e]);
      ZIPDUMPBITS(e);

      /* decode distance of block to copy */
      ZIPNEEDBITS((cab_ULONG)bd)
      if ((e = (t = td + (b & md))->e) > 16)
        do {
          if (e == 99)
            return 1;
          ZIPDUMPBITS(t->b)
          e -= 16;
          ZIPNEEDBITS(e)
        } while ((e = (t = t->v.t + (b & Zipmask[e]))->e) > 16);
      ZIPDUMPBITS(t->b)
      ZIPNEEDBITS(e)
      d = w - t->v.n - (b & Zipmask[e]);
      ZIPDUMPBITS(e)
      do
      {
        d &= ZIPWSIZE - 1;
        e = ZIPWSIZE - max(d, w);
        e = min(e, n);
        n -= e;
        do
        {
          CAB(outbuf)[w++] = CAB(outbuf)[d++];
        } while (--e);
      } while (n);
    }
  }

  /* restore the globals from the locals */
  ZIP(window_posn) = w;              /* restore global window pointer */
  ZIP(bb) = b;                       /* restore global bit buffer */
  ZIP(bk) = k;

  /* done */
  return 0;
}

/***********************************************************
 * Zipinflate_stored (internal)
 */
static cab_LONG fdi_Zipinflate_stored(fdi_decomp_state *decomp_state)
/* "decompress" an inflated type 0 (stored) block. */
{
  cab_ULONG n;           /* number of bytes in block */
  cab_ULONG w;           /* current window position */
  register cab_ULONG b;  /* bit buffer */
  register cab_ULONG k;  /* number of bits in bit buffer */

  /* make local copies of globals */
  b = ZIP(bb);                       /* initialize bit buffer */
  k = ZIP(bk);
  w = ZIP(window_posn);              /* initialize window position */

  /* go to byte boundary */
  n = k & 7;
  ZIPDUMPBITS(n);

  /* get the length and its complement */
  ZIPNEEDBITS(16)
  n = (b & 0xffff);
  ZIPDUMPBITS(16)
  ZIPNEEDBITS(16)
  if (n != ((~b) & 0xffff))
    return 1;                   /* error in compressed data */
  ZIPDUMPBITS(16)

  /* read and output the compressed data */
  while(n--)
  {
    ZIPNEEDBITS(8)
    CAB(outbuf)[w++] = (cab_UBYTE)b;
    ZIPDUMPBITS(8)
  }

  /* restore the globals from the locals */
  ZIP(window_posn) = w;              /* restore global window pointer */
  ZIP(bb) = b;                       /* restore global bit buffer */
  ZIP(bk) = k;
  return 0;
}

/******************************************************
 * fdi_Zipinflate_fixed (internal)
 */
static cab_LONG fdi_Zipinflate_fixed(fdi_decomp_state *decomp_state)
{
  struct Ziphuft *fixed_tl;
  struct Ziphuft *fixed_td;
  cab_LONG fixed_bl, fixed_bd;
  cab_LONG i;                /* temporary variable */
  cab_ULONG *l;

  l = ZIP(ll);

  /* literal table */
  for(i = 0; i < 144; i++)
    l[i] = 8;
  for(; i < 256; i++)
    l[i] = 9;
  for(; i < 280; i++)
    l[i] = 7;
  for(; i < 288; i++)          /* make a complete, but wrong code set */
    l[i] = 8;
  fixed_bl = 7;
  if((i = fdi_Ziphuft_build(l, 288, 257, Zipcplens, Zipcplext, &fixed_tl, &fixed_bl, decomp_state)))
    return i;

  /* distance table */
  for(i = 0; i < 30; i++)      /* make an incomplete code set */
    l[i] = 5;
  fixed_bd = 5;
  if((i = fdi_Ziphuft_build(l, 30, 0, Zipcpdist, Zipcpdext, &fixed_td, &fixed_bd, decomp_state)) > 1)
  {
    fdi_Ziphuft_free(CAB(fdi), fixed_tl);
    return i;
  }

  /* decompress until an end-of-block code */
  i = fdi_Zipinflate_codes(fixed_tl, fixed_td, fixed_bl, fixed_bd, decomp_state);

  fdi_Ziphuft_free(CAB(fdi), fixed_td);
  fdi_Ziphuft_free(CAB(fdi), fixed_tl);
  return i;
}

/**************************************************************
 * fdi_Zipinflate_dynamic (internal)
 */
static cab_LONG fdi_Zipinflate_dynamic(fdi_decomp_state *decomp_state)
 /* decompress an inflated type 2 (dynamic Huffman codes) block. */
{
  cab_LONG i;          	/* temporary variables */
  cab_ULONG j;
  cab_ULONG *ll;
  cab_ULONG l;           	/* last length */
  cab_ULONG m;           	/* mask for bit lengths table */
  cab_ULONG n;           	/* number of lengths to get */
  struct Ziphuft *tl;           /* literal/length code table */
  struct Ziphuft *td;           /* distance code table */
  cab_LONG bl;                  /* lookup bits for tl */
  cab_LONG bd;                  /* lookup bits for td */
  cab_ULONG nb;          	/* number of bit length codes */
  cab_ULONG nl;          	/* number of literal/length codes */
  cab_ULONG nd;          	/* number of distance codes */
  register cab_ULONG b;         /* bit buffer */
  register cab_ULONG k;	        /* number of bits in bit buffer */

  /* make local bit buffer */
  b = ZIP(bb);
  k = ZIP(bk);
  ll = ZIP(ll);

  /* read in table lengths */
  ZIPNEEDBITS(5)
  nl = 257 + (b & 0x1f);      /* number of literal/length codes */
  ZIPDUMPBITS(5)
  ZIPNEEDBITS(5)
  nd = 1 + (b & 0x1f);        /* number of distance codes */
  ZIPDUMPBITS(5)
  ZIPNEEDBITS(4)
  nb = 4 + (b & 0xf);         /* number of bit length codes */
  ZIPDUMPBITS(4)
  if(nl > 288 || nd > 32)
    return 1;                   /* bad lengths */

  /* read in bit-length-code lengths */
  for(j = 0; j < nb; j++)
  {
    ZIPNEEDBITS(3)
    ll[Zipborder[j]] = b & 7;
    ZIPDUMPBITS(3)
  }
  for(; j < 19; j++)
    ll[Zipborder[j]] = 0;

  /* build decoding table for trees--single level, 7 bit lookup */
  bl = 7;
  if((i = fdi_Ziphuft_build(ll, 19, 19, NULL, NULL, &tl, &bl, decomp_state)) != 0)
  {
    if(i == 1)
      fdi_Ziphuft_free(CAB(fdi), tl);
    return i;                   /* incomplete code set */
  }

  /* read in literal and distance code lengths */
  n = nl + nd;
  m = Zipmask[bl];
  i = l = 0;
  while((cab_ULONG)i < n)
  {
    ZIPNEEDBITS((cab_ULONG)bl)
    j = (td = tl + (b & m))->b;
    ZIPDUMPBITS(j)
    j = td->v.n;
    if (j < 16)                 /* length of code in bits (0..15) */
      ll[i++] = l = j;          /* save last length in l */
    else if (j == 16)           /* repeat last length 3 to 6 times */
    {
      ZIPNEEDBITS(2)
      j = 3 + (b & 3);
      ZIPDUMPBITS(2)
      if((cab_ULONG)i + j > n)
        return 1;
      while (j--)
        ll[i++] = l;
    }
    else if (j == 17)           /* 3 to 10 zero length codes */
    {
      ZIPNEEDBITS(3)
      j = 3 + (b & 7);
      ZIPDUMPBITS(3)
      if ((cab_ULONG)i + j > n)
        return 1;
      while (j--)
        ll[i++] = 0;
      l = 0;
    }
    else                        /* j == 18: 11 to 138 zero length codes */
    {
      ZIPNEEDBITS(7)
      j = 11 + (b & 0x7f);
      ZIPDUMPBITS(7)
      if ((cab_ULONG)i + j > n)
        return 1;
      while (j--)
        ll[i++] = 0;
      l = 0;
    }
  }

  /* free decoding table for trees */
  fdi_Ziphuft_free(CAB(fdi), tl);

  /* restore the global bit buffer */
  ZIP(bb) = b;
  ZIP(bk) = k;

  /* build the decoding tables for literal/length and distance codes */
  bl = ZIPLBITS;
  if((i = fdi_Ziphuft_build(ll, nl, 257, Zipcplens, Zipcplext, &tl, &bl, decomp_state)) != 0)
  {
    if(i == 1)
      fdi_Ziphuft_free(CAB(fdi), tl);
    return i;                   /* incomplete code set */
  }
  bd = ZIPDBITS;
  fdi_Ziphuft_build(ll + nl, nd, 0, Zipcpdist, Zipcpdext, &td, &bd, decomp_state);

  /* decompress until an end-of-block code */
  if(fdi_Zipinflate_codes(tl, td, bl, bd, decomp_state))
    return 1;

  /* free the decoding tables, return */
  fdi_Ziphuft_free(CAB(fdi), tl);
  fdi_Ziphuft_free(CAB(fdi), td);
  return 0;
}

/*****************************************************
 * fdi_Zipinflate_block (internal)
 */
static cab_LONG fdi_Zipinflate_block(cab_LONG *e, fdi_decomp_state *decomp_state) /* e == last block flag */
{ /* decompress an inflated block */
  cab_ULONG t;           	/* block type */
  register cab_ULONG b;     /* bit buffer */
  register cab_ULONG k;     /* number of bits in bit buffer */

  /* make local bit buffer */
  b = ZIP(bb);
  k = ZIP(bk);

  /* read in last block bit */
  ZIPNEEDBITS(1)
  *e = (cab_LONG)b & 1;
  ZIPDUMPBITS(1)

  /* read in block type */
  ZIPNEEDBITS(2)
  t = b & 3;
  ZIPDUMPBITS(2)

  /* restore the global bit buffer */
  ZIP(bb) = b;
  ZIP(bk) = k;

  /* inflate that block type */
  if(t == 2)
    return fdi_Zipinflate_dynamic(decomp_state);
  if(t == 0)
    return fdi_Zipinflate_stored(decomp_state);
  if(t == 1)
    return fdi_Zipinflate_fixed(decomp_state);
  /* bad block type */
  return 2;
}

/****************************************************
 * ZIPfdi_decomp(internal)
 */
static int ZIPfdi_decomp(int inlen, int outlen, fdi_decomp_state *decomp_state)
{
  cab_LONG e;               /* last block flag */

  TRACE("(inlen == %d, outlen == %d)\n", inlen, outlen);

  ZIP(inpos) = CAB(inbuf);
  ZIP(bb) = ZIP(bk) = ZIP(window_posn) = 0;
  if(outlen > ZIPWSIZE)
    return DECR_DATAFORMAT;

  /* CK = Chris Kirmse, official Microsoft purloiner */
  if(ZIP(inpos)[0] != 0x43 || ZIP(inpos)[1] != 0x4B)
    return DECR_ILLEGALDATA;
  ZIP(inpos) += 2;

  do {
    if(fdi_Zipinflate_block(&e, decomp_state))
      return DECR_ILLEGALDATA;
  } while(!e);

  /* return success */
  return DECR_OK;
}

/*******************************************************************
 * QTMfdi_decomp(internal)
 */
static int QTMfdi_decomp(int inlen, int outlen, fdi_decomp_state *decomp_state)
{
  cab_UBYTE *inpos  = CAB(inbuf);
  cab_UBYTE *window = QTM(window);
  cab_UBYTE *runsrc, *rundest;
  cab_ULONG window_posn = QTM(window_posn);
  cab_ULONG window_size = QTM(window_size);

  /* used by bitstream macros */
  register int bitsleft, bitrun, bitsneed;
  register cab_ULONG bitbuf;

  /* used by GET_SYMBOL */
  cab_ULONG range;
  cab_UWORD symf;
  int i;

  int extra, togo = outlen, match_length = 0, copy_length;
  cab_UBYTE selector, sym;
  cab_ULONG match_offset = 0;

  cab_UWORD H = 0xFFFF, L = 0, C;

  TRACE("(inlen == %d, outlen == %d)\n", inlen, outlen);

  /* read initial value of C */
  Q_INIT_BITSTREAM;
  Q_READ_BITS(C, 16);

  /* apply 2^x-1 mask */
  window_posn &= window_size - 1;
  /* runs can't straddle the window wraparound */
  if ((window_posn + togo) > window_size) {
    TRACE("straddled run\n");
    return DECR_DATAFORMAT;
  }

  while (togo > 0) {
    GET_SYMBOL(model7, selector);
    switch (selector) {
    case 0:
      GET_SYMBOL(model00, sym); window[window_posn++] = sym; togo--;
      break;
    case 1:
      GET_SYMBOL(model40, sym); window[window_posn++] = sym; togo--;
      break;
    case 2:
      GET_SYMBOL(model80, sym); window[window_posn++] = sym; togo--;
      break;
    case 3:
      GET_SYMBOL(modelC0, sym); window[window_posn++] = sym; togo--;
      break;

    case 4:
      /* selector 4 = fixed length of 3 */
      GET_SYMBOL(model4, sym);
      Q_READ_BITS(extra, CAB(q_extra_bits)[sym]);
      match_offset = CAB(q_position_base)[sym] + extra + 1;
      match_length = 3;
      break;

    case 5:
      /* selector 5 = fixed length of 4 */
      GET_SYMBOL(model5, sym);
      Q_READ_BITS(extra, CAB(q_extra_bits)[sym]);
      match_offset = CAB(q_position_base)[sym] + extra + 1;
      match_length = 4;
      break;

    case 6:
      /* selector 6 = variable length */
      GET_SYMBOL(model6len, sym);
      Q_READ_BITS(extra, CAB(q_length_extra)[sym]);
      match_length = CAB(q_length_base)[sym] + extra + 5;
      GET_SYMBOL(model6pos, sym);
      Q_READ_BITS(extra, CAB(q_extra_bits)[sym]);
      match_offset = CAB(q_position_base)[sym] + extra + 1;
      break;

    default:
      TRACE("Selector is bogus\n");
      return DECR_ILLEGALDATA;
    }

    /* if this is a match */
    if (selector >= 4) {
      rundest = window + window_posn;
      togo -= match_length;

      /* copy any wrapped around source data */
      if (window_posn >= match_offset) {
        /* no wrap */
        runsrc = rundest - match_offset;
      } else {
        runsrc = rundest + (window_size - match_offset);
        copy_length = match_offset - window_posn;
        if (copy_length < match_length) {
          match_length -= copy_length;
          window_posn += copy_length;
          while (copy_length-- > 0) *rundest++ = *runsrc++;
          runsrc = window;
        }
      }
      window_posn += match_length;

      /* copy match data - no worries about destination wraps */
      while (match_length-- > 0) *rundest++ = *runsrc++;
    }
  } /* while (togo > 0) */

  if (togo != 0) {
    TRACE("Frame overflow, this_run = %d\n", togo);
    return DECR_ILLEGALDATA;
  }

  memcpy(CAB(outbuf), window + ((!window_posn) ? window_size : window_posn) -
    outlen, outlen);

  QTM(window_posn) = window_posn;
  return DECR_OK;
}

/************************************************************
 * fdi_lzx_read_lens (internal)
 */
static int fdi_lzx_read_lens(cab_UBYTE *lens, cab_ULONG first, cab_ULONG last, struct lzx_bits *lb,
                  fdi_decomp_state *decomp_state) {
  cab_ULONG i,j, x,y;
  int z;

  register cab_ULONG bitbuf = lb->bb;
  register int bitsleft = lb->bl;
  cab_UBYTE *inpos = lb->ip;
  cab_UWORD *hufftbl;
  
  for (x = 0; x < 20; x++) {
    READ_BITS(y, 4);
    LENTABLE(PRETREE)[x] = y;
  }
  BUILD_TABLE(PRETREE);

  for (x = first; x < last; ) {
    READ_HUFFSYM(PRETREE, z);
    if (z == 17) {
      READ_BITS(y, 4); y += 4;
      while (y--) lens[x++] = 0;
    }
    else if (z == 18) {
      READ_BITS(y, 5); y += 20;
      while (y--) lens[x++] = 0;
    }
    else if (z == 19) {
      READ_BITS(y, 1); y += 4;
      READ_HUFFSYM(PRETREE, z);
      z = lens[x] - z; if (z < 0) z += 17;
      while (y--) lens[x++] = z;
    }
    else {
      z = lens[x] - z; if (z < 0) z += 17;
      lens[x++] = z;
    }
  }

  lb->bb = bitbuf;
  lb->bl = bitsleft;
  lb->ip = inpos;
  return 0;
}

/*******************************************************
 * LZXfdi_decomp(internal)
 */
static int LZXfdi_decomp(int inlen, int outlen, fdi_decomp_state *decomp_state) {
  cab_UBYTE *inpos  = CAB(inbuf);
  const cab_UBYTE *endinp = inpos + inlen;
  cab_UBYTE *window = LZX(window);
  cab_UBYTE *runsrc, *rundest;
  cab_UWORD *hufftbl; /* used in READ_HUFFSYM macro as chosen decoding table */

  cab_ULONG window_posn = LZX(window_posn);
  cab_ULONG window_size = LZX(window_size);
  cab_ULONG R0 = LZX(R0);
  cab_ULONG R1 = LZX(R1);
  cab_ULONG R2 = LZX(R2);

  register cab_ULONG bitbuf;
  register int bitsleft;
  cab_ULONG match_offset, i,j,k; /* ijk used in READ_HUFFSYM macro */
  struct lzx_bits lb; /* used in READ_LENGTHS macro */

  int togo = outlen, this_run, main_element, aligned_bits;
  int match_length, copy_length, length_footer, extra, verbatim_bits;

  TRACE("(inlen == %d, outlen == %d)\n", inlen, outlen);

  INIT_BITSTREAM;

  /* read header if necessary */
  if (!LZX(header_read)) {
    i = j = 0;
    READ_BITS(k, 1); if (k) { READ_BITS(i,16); READ_BITS(j,16); }
    LZX(intel_filesize) = (i << 16) | j; /* or 0 if not encoded */
    LZX(header_read) = 1;
  }

  /* main decoding loop */
  while (togo > 0) {
    /* last block finished, new block expected */
    if (LZX(block_remaining) == 0) {
      if (LZX(block_type) == LZX_BLOCKTYPE_UNCOMPRESSED) {
        if (LZX(block_length) & 1) inpos++; /* realign bitstream to word */
        INIT_BITSTREAM;
      }

      READ_BITS(LZX(block_type), 3);
      READ_BITS(i, 16);
      READ_BITS(j, 8);
      LZX(block_remaining) = LZX(block_length) = (i << 8) | j;

      switch (LZX(block_type)) {
      case LZX_BLOCKTYPE_ALIGNED:
        for (i = 0; i < 8; i++) { READ_BITS(j, 3); LENTABLE(ALIGNED)[i] = j; }
        BUILD_TABLE(ALIGNED);
        /* rest of aligned header is same as verbatim */

      case LZX_BLOCKTYPE_VERBATIM:
        READ_LENGTHS(MAINTREE, 0, 256, fdi_lzx_read_lens);
        READ_LENGTHS(MAINTREE, 256, LZX(main_elements), fdi_lzx_read_lens);
        BUILD_TABLE(MAINTREE);
        if (LENTABLE(MAINTREE)[0xE8] != 0) LZX(intel_started) = 1;

        READ_LENGTHS(LENGTH, 0, LZX_NUM_SECONDARY_LENGTHS, fdi_lzx_read_lens);
        BUILD_TABLE(LENGTH);
        break;

      case LZX_BLOCKTYPE_UNCOMPRESSED:
        LZX(intel_started) = 1; /* because we can't assume otherwise */
        ENSURE_BITS(16); /* get up to 16 pad bits into the buffer */
        if (bitsleft > 16) inpos -= 2; /* and align the bitstream! */
        R0 = inpos[0]|(inpos[1]<<8)|(inpos[2]<<16)|(inpos[3]<<24);inpos+=4;
        R1 = inpos[0]|(inpos[1]<<8)|(inpos[2]<<16)|(inpos[3]<<24);inpos+=4;
        R2 = inpos[0]|(inpos[1]<<8)|(inpos[2]<<16)|(inpos[3]<<24);inpos+=4;
        break;

      default:
        return DECR_ILLEGALDATA;
      }
    }

    /* buffer exhaustion check */
    if (inpos > endinp) {
      /* it's possible to have a file where the next run is less than
       * 16 bits in size. In this case, the READ_HUFFSYM() macro used
       * in building the tables will exhaust the buffer, so we should
       * allow for this, but not allow those accidentally read bits to
       * be used (so we check that there are at least 16 bits
       * remaining - in this boundary case they aren't really part of
       * the compressed data)
       */
      if (inpos > (endinp+2) || bitsleft < 16) return DECR_ILLEGALDATA;
    }

    while ((this_run = LZX(block_remaining)) > 0 && togo > 0) {
      if (this_run > togo) this_run = togo;
      togo -= this_run;
      LZX(block_remaining) -= this_run;

      /* apply 2^x-1 mask */
      window_posn &= window_size - 1;
      /* runs can't straddle the window wraparound */
      if ((window_posn + this_run) > window_size)
        return DECR_DATAFORMAT;

      switch (LZX(block_type)) {

      case LZX_BLOCKTYPE_VERBATIM:
        while (this_run > 0) {
          READ_HUFFSYM(MAINTREE, main_element);

          if (main_element < LZX_NUM_CHARS) {
            /* literal: 0 to LZX_NUM_CHARS-1 */
            window[window_posn++] = main_element;
            this_run--;
          }
          else {
            /* match: LZX_NUM_CHARS + ((slot<<3) | length_header (3 bits)) */
            main_element -= LZX_NUM_CHARS;
  
            match_length = main_element & LZX_NUM_PRIMARY_LENGTHS;
            if (match_length == LZX_NUM_PRIMARY_LENGTHS) {
              READ_HUFFSYM(LENGTH, length_footer);
              match_length += length_footer;
            }
            match_length += LZX_MIN_MATCH;
  
            match_offset = main_element >> 3;
  
            if (match_offset > 2) {
              /* not repeated offset */
              if (match_offset != 3) {
                extra = CAB(extra_bits)[match_offset];
                READ_BITS(verbatim_bits, extra);
                match_offset = CAB(lzx_position_base)[match_offset] 
                               - 2 + verbatim_bits;
              }
              else {
                match_offset = 1;
              }
  
              /* update repeated offset LRU queue */
              R2 = R1; R1 = R0; R0 = match_offset;
            }
            else if (match_offset == 0) {
              match_offset = R0;
            }
            else if (match_offset == 1) {
              match_offset = R1;
              R1 = R0; R0 = match_offset;
            }
            else /* match_offset == 2 */ {
              match_offset = R2;
              R2 = R0; R0 = match_offset;
            }

            rundest = window + window_posn;
            this_run -= match_length;

            /* copy any wrapped around source data */
            if (window_posn >= match_offset) {
              /* no wrap */
              runsrc = rundest - match_offset;
            } else {
              runsrc = rundest + (window_size - match_offset);
              copy_length = match_offset - window_posn;
              if (copy_length < match_length) {
                match_length -= copy_length;
                window_posn += copy_length;
                while (copy_length-- > 0) *rundest++ = *runsrc++;
                runsrc = window;
              }
            }
            window_posn += match_length;

            /* copy match data - no worries about destination wraps */
            while (match_length-- > 0) *rundest++ = *runsrc++;
          }
        }
        break;

      case LZX_BLOCKTYPE_ALIGNED:
        while (this_run > 0) {
          READ_HUFFSYM(MAINTREE, main_element);
  
          if (main_element < LZX_NUM_CHARS) {
            /* literal: 0 to LZX_NUM_CHARS-1 */
            window[window_posn++] = main_element;
            this_run--;
          }
          else {
            /* match: LZX_NUM_CHARS + ((slot<<3) | length_header (3 bits)) */
            main_element -= LZX_NUM_CHARS;
  
            match_length = main_element & LZX_NUM_PRIMARY_LENGTHS;
            if (match_length == LZX_NUM_PRIMARY_LENGTHS) {
              READ_HUFFSYM(LENGTH, length_footer);
              match_length += length_footer;
            }
            match_length += LZX_MIN_MATCH;
  
            match_offset = main_element >> 3;
  
            if (match_offset > 2) {
              /* not repeated offset */
              extra = CAB(extra_bits)[match_offset];
              match_offset = CAB(lzx_position_base)[match_offset] - 2;
              if (extra > 3) {
                /* verbatim and aligned bits */
                extra -= 3;
                READ_BITS(verbatim_bits, extra);
                match_offset += (verbatim_bits << 3);
                READ_HUFFSYM(ALIGNED, aligned_bits);
                match_offset += aligned_bits;
              }
              else if (extra == 3) {
                /* aligned bits only */
                READ_HUFFSYM(ALIGNED, aligned_bits);
                match_offset += aligned_bits;
              }
              else if (extra > 0) { /* extra==1, extra==2 */
                /* verbatim bits only */
                READ_BITS(verbatim_bits, extra);
                match_offset += verbatim_bits;
              }
              else /* extra == 0 */ {
                /* ??? */
                match_offset = 1;
              }
  
              /* update repeated offset LRU queue */
              R2 = R1; R1 = R0; R0 = match_offset;
            }
            else if (match_offset == 0) {
              match_offset = R0;
            }
            else if (match_offset == 1) {
              match_offset = R1;
              R1 = R0; R0 = match_offset;
            }
            else /* match_offset == 2 */ {
              match_offset = R2;
              R2 = R0; R0 = match_offset;
            }

            rundest = window + window_posn;
            this_run -= match_length;

            /* copy any wrapped around source data */
            if (window_posn >= match_offset) {
              /* no wrap */
              runsrc = rundest - match_offset;
            } else {
              runsrc = rundest + (window_size - match_offset);
              copy_length = match_offset - window_posn;
              if (copy_length < match_length) {
                match_length -= copy_length;
                window_posn += copy_length;
                while (copy_length-- > 0) *rundest++ = *runsrc++;
                runsrc = window;
              }
            }
            window_posn += match_length;

            /* copy match data - no worries about destination wraps */
            while (match_length-- > 0) *rundest++ = *runsrc++;
          }
        }
        break;

      case LZX_BLOCKTYPE_UNCOMPRESSED:
        if ((inpos + this_run) > endinp) return DECR_ILLEGALDATA;
        memcpy(window + window_posn, inpos, (size_t) this_run);
        inpos += this_run; window_posn += this_run;
        break;

      default:
        return DECR_ILLEGALDATA; /* might as well */
      }

    }
  }

  if (togo != 0) return DECR_ILLEGALDATA;
  memcpy(CAB(outbuf), window + ((!window_posn) ? window_size : window_posn) -
    outlen, (size_t) outlen);

  LZX(window_posn) = window_posn;
  LZX(R0) = R0;
  LZX(R1) = R1;
  LZX(R2) = R2;

  /* intel E8 decoding */
  if ((LZX(frames_read)++ < 32768) && LZX(intel_filesize) != 0) {
    if (outlen <= 6 || !LZX(intel_started)) {
      LZX(intel_curpos) += outlen;
    }
    else {
      cab_UBYTE *data    = CAB(outbuf);
      cab_UBYTE *dataend = data + outlen - 10;
      cab_LONG curpos    = LZX(intel_curpos);
      cab_LONG filesize  = LZX(intel_filesize);
      cab_LONG abs_off, rel_off;

      LZX(intel_curpos) = curpos + outlen;

      while (data < dataend) {
        if (*data++ != 0xE8) { curpos++; continue; }
        abs_off = data[0] | (data[1]<<8) | (data[2]<<16) | (data[3]<<24);
        if ((abs_off >= -curpos) && (abs_off < filesize)) {
          rel_off = (abs_off >= 0) ? abs_off - curpos : abs_off + filesize;
          data[0] = (cab_UBYTE) rel_off;
          data[1] = (cab_UBYTE) (rel_off >> 8);
          data[2] = (cab_UBYTE) (rel_off >> 16);
          data[3] = (cab_UBYTE) (rel_off >> 24);
        }
        data += 4;
        curpos += 5;
      }
    }
  }
  return DECR_OK;
}

/**********************************************************
 * fdi_decomp (internal)
 *
 * Decompress the requested number of bytes.  If savemode is zero,
 * do not save the output anywhere, just plow through blocks until we
 * reach the specified (uncompressed) distance from the starting point,
 * and remember the position of the cabfile pointer (and which cabfile)
 * after we are done; otherwise, save the data out to CAB(filehf),
 * decompressing the requested number of bytes and writing them out.  This
 * is also where we jump to additional cabinets in the case of split
 * cab's, and provide (some of) the NEXT_CABINET notification semantics.
 */
static int fdi_decomp(const struct fdi_file *fi, int savemode, fdi_decomp_state *decomp_state,
  char *pszCabPath, PFNFDINOTIFY pfnfdin, void *pvUser)
{
  cab_ULONG bytes = savemode ? fi->length : fi->offset - CAB(offset);
  cab_UBYTE buf[cfdata_SIZEOF], *data;
  cab_UWORD inlen, len, outlen, cando;
  cab_ULONG cksum;
  cab_LONG err;
  fdi_decomp_state *cab = (savemode && CAB(decomp_cab)) ? CAB(decomp_cab) : decomp_state;

  TRACE("(fi == ^%p, savemode == %d, bytes == %d)\n", fi, savemode, bytes);

  while (bytes > 0) {
    /* cando = the max number of bytes we can do */
    cando = CAB(outlen);
    if (cando > bytes) cando = bytes;

    /* if cando != 0 */
    if (cando && savemode)
      CAB(fdi)->write(CAB(filehf), CAB(outpos), cando);

    CAB(outpos) += cando;
    CAB(outlen) -= cando;
    bytes -= cando; if (!bytes) break;

    /* we only get here if we emptied the output buffer */

    /* read data header + data */
    inlen = outlen = 0;
    while (outlen == 0) {
      /* read the block header, skip the reserved part */
      if (CAB(fdi)->read(cab->cabhf, buf, cfdata_SIZEOF) != cfdata_SIZEOF)
        return DECR_INPUT;

      if (CAB(fdi)->seek(cab->cabhf, cab->mii.block_resv, SEEK_CUR) == -1)
        return DECR_INPUT;

      /* we shouldn't get blocks over CAB_INPUTMAX in size */
      data = CAB(inbuf) + inlen;
      len = EndGetI16(buf+cfdata_CompressedSize);
      inlen += len;
      if (inlen > CAB_INPUTMAX) return DECR_INPUT;
      if (CAB(fdi)->read(cab->cabhf, data, len) != len)
        return DECR_INPUT;

      /* clear two bytes after read-in data */
      data[len+1] = data[len+2] = 0;

      /* perform checksum test on the block (if one is stored) */
      cksum = EndGetI32(buf+cfdata_CheckSum);
      if (cksum && cksum != checksum(buf+4, 4, checksum(data, len, 0)))
        return DECR_CHECKSUM; /* checksum is wrong */

      outlen = EndGetI16(buf+cfdata_UncompressedSize);

      /* outlen=0 means this block was the last contiguous part
         of a split block, continued in the next cabinet */
      if (outlen == 0) {
        int pathlen, filenamelen;
        INT_PTR cabhf;
        char fullpath[MAX_PATH], userpath[256];
        FDINOTIFICATION fdin;
        FDICABINETINFO fdici;
        char emptystring = '\0';
        cab_UBYTE buf2[64];
        BOOL success = FALSE;
        struct fdi_folder *fol = NULL, *linkfol = NULL; 
        struct fdi_file   *file = NULL, *linkfile = NULL;

        tryanothercab:

        /* set up the next decomp_state... */
        if (!(cab->next)) {
          unsigned int i;

          if (!cab->mii.hasnext) return DECR_INPUT;

          if (!((cab->next = CAB(fdi)->alloc(sizeof(fdi_decomp_state)))))
            return DECR_NOMEMORY;

          ZeroMemory(cab->next, sizeof(fdi_decomp_state));

          /* copy pszCabPath to userpath */
          ZeroMemory(userpath, 256);
          pathlen = pszCabPath ? strlen(pszCabPath) : 0;
          if (pathlen) {
            if (pathlen < 256) /* else we are in a weird place... let's leave it blank and see if the user fixes it */
              strcpy(userpath, pszCabPath);
          }

          /* initial fdintNEXT_CABINET notification */
          ZeroMemory(&fdin, sizeof(FDINOTIFICATION));
          fdin.psz1 = cab->mii.nextname ? cab->mii.nextname : &emptystring;
          fdin.psz2 = cab->mii.nextinfo ? cab->mii.nextinfo : &emptystring;
          fdin.psz3 = userpath;
          fdin.fdie = FDIERROR_NONE;
          fdin.pv = pvUser;

          if (((*pfnfdin)(fdintNEXT_CABINET, &fdin))) return DECR_USERABORT;

          do {

            pathlen = strlen(userpath);
            filenamelen = cab->mii.nextname ? strlen(cab->mii.nextname) : 0;

            /* slight overestimation here to save CPU cycles in the developer's brain */
            if ((pathlen + filenamelen + 3) > MAX_PATH) {
              ERR("MAX_PATH exceeded.\n");
              return DECR_ILLEGALDATA;
            }

            /* paste the path and filename together */
            fullpath[0] = '\0';
            if (pathlen) {
              strcpy(fullpath, userpath);
              if (fullpath[pathlen - 1] != '\\')
                strcat(fullpath, "\\");
            }
            if (filenamelen)
              strcat(fullpath, cab->mii.nextname);
            else if (fullpath[0])
                fullpath[strlen(fullpath)-1] = 0; /* remove trailing backslash */

            TRACE("full cab path/file name: %s\n", debugstr_a(fullpath));

            /* try to get a handle to the cabfile */
            cabhf = CAB(fdi)->open(fullpath, _O_RDONLY|_O_BINARY, _S_IREAD | _S_IWRITE);
            if (cabhf == -1) {
              /* no file.  allow the user to try again */
              fdin.fdie = FDIERROR_CABINET_NOT_FOUND;
              if (((*pfnfdin)(fdintNEXT_CABINET, &fdin))) return DECR_USERABORT;
              continue;
            }

            if (cabhf == 0) {
              ERR("PFDI_OPEN returned zero for %s.\n", fullpath);
              fdin.fdie = FDIERROR_CABINET_NOT_FOUND;
              if (((*pfnfdin)(fdintNEXT_CABINET, &fdin))) return DECR_USERABORT;
              continue;
            }

            /* check if it's really a cabfile. Note that this doesn't implement the bug */
            if (!FDI_read_entries(CAB(fdi), cabhf, &fdici, &(cab->next->mii))) {
              WARN("FDIIsCabinet failed.\n");
              CAB(fdi)->close(cabhf);
              fdin.fdie = FDIERROR_NOT_A_CABINET;
              if (((*pfnfdin)(fdintNEXT_CABINET, &fdin))) return DECR_USERABORT;
              continue;
            }

            if ((fdici.setID != cab->setID) || (fdici.iCabinet != (cab->iCabinet + 1))) {
              WARN("Wrong Cabinet.\n");
              CAB(fdi)->close(cabhf);
              fdin.fdie = FDIERROR_WRONG_CABINET;
              if (((*pfnfdin)(fdintNEXT_CABINET, &fdin))) return DECR_USERABORT;
              continue;
            }
           
            break;

          } while (1);
          
          /* cabinet notification */
          ZeroMemory(&fdin, sizeof(FDINOTIFICATION));
          fdin.setID = fdici.setID;
          fdin.iCabinet = fdici.iCabinet;
          fdin.pv = pvUser;
          fdin.psz1 = (cab->next->mii.nextname) ? cab->next->mii.nextname : &emptystring;
          fdin.psz2 = (cab->next->mii.nextinfo) ? cab->next->mii.nextinfo : &emptystring;
          fdin.psz3 = pszCabPath;
        
          if (((*pfnfdin)(fdintCABINET_INFO, &fdin))) return DECR_USERABORT;
          
          cab->next->setID = fdici.setID;
          cab->next->iCabinet = fdici.iCabinet;
          cab->next->fdi = CAB(fdi);
          cab->next->filehf = CAB(filehf);
          cab->next->cabhf = cabhf;
          cab->next->decompress = CAB(decompress); /* crude, but unused anyhow */

          cab = cab->next; /* advance to the next cabinet */

          /* read folders */
          for (i = 0; i < fdici.cFolders; i++) {
            if (CAB(fdi)->read(cab->cabhf, buf2, cffold_SIZEOF) != cffold_SIZEOF)
              return DECR_INPUT;

            if (cab->mii.folder_resv > 0)
              CAB(fdi)->seek(cab->cabhf, cab->mii.folder_resv, SEEK_CUR);

            fol = CAB(fdi)->alloc(sizeof(struct fdi_folder));
            if (!fol) {
              ERR("out of memory!\n");
              return DECR_NOMEMORY;
            }
            ZeroMemory(fol, sizeof(struct fdi_folder));
            if (!(cab->firstfol)) cab->firstfol = fol;
        
            fol->offset = (cab_off_t) EndGetI32(buf2+cffold_DataOffset);
            fol->num_blocks = EndGetI16(buf2+cffold_NumBlocks);
            fol->comp_type  = EndGetI16(buf2+cffold_CompType);
        
            if (linkfol)
              linkfol->next = fol; 
            linkfol = fol;
          }
        
          /* read files */
          for (i = 0; i < fdici.cFiles; i++) {
            if (CAB(fdi)->read(cab->cabhf, buf2, cffile_SIZEOF) != cffile_SIZEOF)
              return DECR_INPUT;

            file = CAB(fdi)->alloc(sizeof(struct fdi_file));
            if (!file) {
              ERR("out of memory!\n"); 
              return DECR_NOMEMORY;
            }
            ZeroMemory(file, sizeof(struct fdi_file));
            if (!(cab->firstfile)) cab->firstfile = file;
              
            file->length   = EndGetI32(buf2+cffile_UncompressedSize);
            file->offset   = EndGetI32(buf2+cffile_FolderOffset);
            file->index    = EndGetI16(buf2+cffile_FolderIndex);
            file->time     = EndGetI16(buf2+cffile_Time);
            file->date     = EndGetI16(buf2+cffile_Date);
            file->attribs  = EndGetI16(buf2+cffile_Attribs);
            file->filename = FDI_read_string(CAB(fdi), cab->cabhf, fdici.cbCabinet);
        
            if (!file->filename) return DECR_INPUT;
        
            if (linkfile)
              linkfile->next = file;
            linkfile = file;
          }
        
        } else 
            cab = cab->next; /* advance to the next cabinet */

        /* iterate files -- if we encounter the continued file, process it --
           otherwise, jump to the label above and keep looking */

        for (file = cab->firstfile; (file); file = file->next) {
          if ((file->index & cffileCONTINUED_FROM_PREV) == cffileCONTINUED_FROM_PREV) {
            /* check to ensure a real match */
            if (lstrcmpiA(fi->filename, file->filename) == 0) {
              success = TRUE;
              if (CAB(fdi)->seek(cab->cabhf, cab->firstfol->offset, SEEK_SET) == -1)
                return DECR_INPUT;
              break;
            }
          }
        }
        if (!success) goto tryanothercab; /* FIXME: shouldn't this trigger
                                             "Wrong Cabinet" notification? */
      }
    }

    /* decompress block */
    if ((err = CAB(decompress)(inlen, outlen, decomp_state)))
      return err;
    CAB(outlen) = outlen;
    CAB(outpos) = CAB(outbuf);
  }
  
  CAB(decomp_cab) = cab;
  return DECR_OK;
}

static void free_decompression_temps(FDI_Int *fdi, const struct fdi_folder *fol,
  fdi_decomp_state *decomp_state)
{
  switch (fol->comp_type & cffoldCOMPTYPE_MASK) {
  case cffoldCOMPTYPE_LZX:
    if (LZX(window)) {
      fdi->free(LZX(window));
      LZX(window) = NULL;
    }
    break;
  case cffoldCOMPTYPE_QUANTUM:
    if (QTM(window)) {
      fdi->free(QTM(window));
      QTM(window) = NULL;
    }
    break;
  }
}

static void free_decompression_mem(FDI_Int *fdi, fdi_decomp_state *decomp_state)
{
  struct fdi_folder *fol;
  while (decomp_state) {
    fdi_decomp_state *prev_fds;

    fdi->close(CAB(cabhf));

    /* free the storage remembered by mii */
    if (CAB(mii).nextname) fdi->free(CAB(mii).nextname);
    if (CAB(mii).nextinfo) fdi->free(CAB(mii).nextinfo);
    if (CAB(mii).prevname) fdi->free(CAB(mii).prevname);
    if (CAB(mii).previnfo) fdi->free(CAB(mii).previnfo);

    while (CAB(firstfol)) {
      fol = CAB(firstfol);
      CAB(firstfol) = CAB(firstfol)->next;
      fdi->free(fol);
    }
    while (CAB(firstfile)) {
      struct fdi_file *file = CAB(firstfile);
      if (file->filename) fdi->free(file->filename);
      CAB(firstfile) = CAB(firstfile)->next;
      fdi->free(file);
    }
    prev_fds = decomp_state;
    decomp_state = CAB(next);
    fdi->free(prev_fds);
  }
}

/***********************************************************************
 *		FDICopy (CABINET.22)
 *
 * Iterates through the files in the Cabinet file indicated by name and
 * file-location.  May chain forward to additional cabinets (typically
 * only one) if files which begin in this Cabinet are continued in another
 * cabinet.  For each file which is partially contained in this cabinet,
 * and partially contained in a prior cabinet, provides fdintPARTIAL_FILE
 * notification to the pfnfdin callback.  For each file which begins in
 * this cabinet, fdintCOPY_FILE notification is provided to the pfnfdin
 * callback, and the file is optionally decompressed and saved to disk.
 * Notification is not provided for files which are not at least partially
 * contained in the specified cabinet file.
 *
 * See below for a thorough explanation of the various notification
 * callbacks.
 *
 * PARAMS
 *   hfdi       [I] An HFDI from FDICreate
 *   pszCabinet [I] C-style string containing the filename of the cabinet
 *   pszCabPath [I] C-style string containing the file path of the cabinet
 *   flags      [I] "Decoder parameters".  Ignored.  Suggested value: 0.
 *   pfnfdin    [I] Pointer to a notification function.  See CALLBACKS below.
 *   pfnfdid    [I] Pointer to a decryption function.  Ignored.  Suggested
 *                  value: NULL.
 *   pvUser     [I] arbitrary void * value which is passed to callbacks.
 *
 * RETURNS
 *   TRUE if successful.
 *   FALSE if unsuccessful (error information is provided in the ERF structure
 *     associated with the provided decompression handle by FDICreate).
 *
 * CALLBACKS
 *
 *   Two pointers to callback functions are provided as parameters to FDICopy:
 *   pfnfdin(of type PFNFDINOTIFY), and pfnfdid (of type PFNFDIDECRYPT).  These
 *   types are as follows:
 *
 *     typedef INT_PTR (__cdecl *PFNFDINOTIFY)  ( FDINOTIFICATIONTYPE fdint,
 *                                               PFDINOTIFICATION  pfdin );
 *
 *     typedef int     (__cdecl *PFNFDIDECRYPT) ( PFDIDECRYPT pfdid );
 *
 *   You can create functions of this type using the FNFDINOTIFY() and
 *   FNFDIDECRYPT() macros, respectively.  For example:
 *
 *     FNFDINOTIFY(mycallback) {
 *       / * use variables fdint and pfdin to process notification * /
 *     }
 *
 *   The second callback, which could be used for decrypting encrypted data,
 *   is not used at all.
 *
 *   Each notification informs the user of some event which has occurred during
 *   decompression of the cabinet file; each notification is also an opportunity
 *   for the callee to abort decompression.  The information provided to the
 *   callback and the meaning of the callback's return value vary drastically
 *   across the various types of notification.  The type of notification is the
 *   fdint parameter; all other information is provided to the callback in
 *   notification-specific parts of the FDINOTIFICATION structure pointed to by
 *   pfdin.  The only part of that structure which is assigned for every callback
 *   is the pv element, which contains the arbitrary value which was passed to
 *   FDICopy in the pvUser argument (psz1 is also used each time, but its meaning
 *   is highly dependent on fdint).
 *   
 *   If you encounter unknown notifications, you should return zero if you want
 *   decompression to continue (or -1 to abort).  All strings used in the
 *   callbacks are regular C-style strings.  Detailed descriptions of each
 *   notification type follow:
 *
 *   fdintCABINET_INFO:
 * 
 *     This is the first notification provided after calling FDICopy, and provides
 *     the user with various information about the cabinet.  Note that this is
 *     called for each cabinet FDICopy opens, not just the first one.  In the
 *     structure pointed to by pfdin, psz1 contains a pointer to the name of the
 *     next cabinet file in the set after the one just loaded (if any), psz2
 *     contains a pointer to the name or "info" of the next disk, psz3
 *     contains a pointer to the file-path of the current cabinet, setID
 *     contains an arbitrary constant associated with this set of cabinet files,
 *     and iCabinet contains the numerical index of the current cabinet within
 *     that set.  Return zero, or -1 to abort.
 *
 *   fdintPARTIAL_FILE:
 *
 *     This notification is provided when FDICopy encounters a part of a file
 *     contained in this cabinet which is missing its beginning.  Files can be
 *     split across cabinets, so this is not necessarily an abnormality; it just
 *     means that the file in question begins in another cabinet.  No file
 *     corresponding to this notification is extracted from the cabinet.  In the
 *     structure pointed to by pfdin, psz1 contains a pointer to the name of the
 *     partial file, psz2 contains a pointer to the file name of the cabinet in
 *     which this file begins, and psz3 contains a pointer to the disk name or
 *     "info" of the cabinet where the file begins. Return zero, or -1 to abort.
 *
 *   fdintCOPY_FILE:
 *
 *     This notification is provided when FDICopy encounters a file which starts
 *     in the cabinet file, provided to FDICopy in pszCabinet.  (FDICopy will not
 *     look for files in cabinets after the first one).  One notification will be
 *     sent for each such file, before the file is decompressed.  By returning
 *     zero, the callback can instruct FDICopy to skip the file.  In the structure
 *     pointed to by pfdin, psz1 contains a pointer to the file's name, cb contains
 *     the size of the file (uncompressed), attribs contains the file attributes,
 *     and date and time contain the date and time of the file.  attributes, date,
 *     and time are of the 16-bit ms-dos variety.  Return -1 to abort decompression
 *     for the entire cabinet, 0 to skip just this file but continue scanning the
 *     cabinet for more files, or an FDIClose()-compatible file-handle.
 *
 *   fdintCLOSE_FILE_INFO:
 *
 *     This notification is important, don't forget to implement it.  This
 *     notification indicates that a file has been successfully uncompressed and
 *     written to disk.  Upon receipt of this notification, the callee is expected
 *     to close the file handle, to set the attributes and date/time of the
 *     closed file, and possibly to execute the file.  In the structure pointed to
 *     by pfdin, psz1 contains a pointer to the name of the file, hf will be the
 *     open file handle (close it), cb contains 1 or zero, indicating respectively
 *     that the callee should or should not execute the file, and date, time
 *     and attributes will be set as in fdintCOPY_FILE.  Bizarrely, the Cabinet SDK
 *     specifies that _A_EXEC will be xor'ed out of attributes!  wine does not do
 *     do so.  Return TRUE, or FALSE to abort decompression.
 *
 *   fdintNEXT_CABINET:
 *
 *     This notification is called when FDICopy must load in another cabinet.  This
 *     can occur when a file's data is "split" across multiple cabinets.  The
 *     callee has the opportunity to request that FDICopy look in a different file
 *     path for the specified cabinet file, by writing that data into a provided
 *     buffer (see below for more information).  This notification will be received
 *     more than once per-cabinet in the instance that FDICopy failed to find a
 *     valid cabinet at the location specified by the first per-cabinet
 *     fdintNEXT_CABINET notification.  In such instances, the fdie element of the
 *     structure pointed to by pfdin indicates the error which prevented FDICopy
 *     from proceeding successfully.  Return zero to indicate success, or -1 to
 *     indicate failure and abort FDICopy.
 *
 *     Upon receipt of this notification, the structure pointed to by pfdin will
 *     contain the following values: psz1 pointing to the name of the cabinet
 *     which FDICopy is attempting to open, psz2 pointing to the name ("info") of
 *     the next disk, psz3 pointing to the presumed file-location of the cabinet,
 *     and fdie containing either FDIERROR_NONE, or one of the following: 
 *
 *       FDIERROR_CABINET_NOT_FOUND, FDIERROR_NOT_A_CABINET,
 *       FDIERROR_UNKNOWN_CABINET_VERSION, FDIERROR_CORRUPT_CABINET,
 *       FDIERROR_BAD_COMPR_TYPE, FDIERROR_RESERVE_MISMATCH, and 
 *       FDIERROR_WRONG_CABINET.
 *
 *     The callee may choose to change the path where FDICopy will look for the
 *     cabinet after this notification.  To do so, the caller may write the new
 *     pathname to the buffer pointed to by psz3, which is 256 characters in
 *     length, including the terminating null character, before returning zero.
 *
 *   fdintENUMERATE:
 *
 *     Undocumented and unimplemented in wine, this seems to be sent each time
 *     a cabinet is opened, along with the fdintCABINET_INFO notification.  It
 *     probably has an interface similar to that of fdintCABINET_INFO; maybe this
 *     provides information about the current cabinet instead of the next one....
 *     this is just a guess, it has not been looked at closely.
 *
 * INCLUDES
 *   fdi.c
 */
BOOL __cdecl FDICopy(
        HFDI           hfdi,
        char          *pszCabinet,
        char          *pszCabPath,
        int            flags,
        PFNFDINOTIFY   pfnfdin,
        PFNFDIDECRYPT  pfnfdid,
        void          *pvUser)
{ 
  FDICABINETINFO    fdici;
  FDINOTIFICATION   fdin;
  INT_PTR           cabhf, filehf = 0;
  unsigned int      i;
  char              fullpath[MAX_PATH];
  size_t            pathlen, filenamelen;
  char              emptystring = '\0';
  cab_UBYTE         buf[64];
  struct fdi_folder *fol = NULL, *linkfol = NULL; 
  struct fdi_file   *file = NULL, *linkfile = NULL;
  fdi_decomp_state *decomp_state;
  FDI_Int *fdi = get_fdi_ptr( hfdi );

  TRACE("(hfdi == ^%p, pszCabinet == %s, pszCabPath == %s, flags == %x, "
        "pfnfdin == ^%p, pfnfdid == ^%p, pvUser == ^%p)\n",
        hfdi, debugstr_a(pszCabinet), debugstr_a(pszCabPath), flags, pfnfdin, pfnfdid, pvUser);

  if (!fdi) return FALSE;

  if (!(decomp_state = fdi->alloc(sizeof(fdi_decomp_state))))
  {
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
  }
  ZeroMemory(decomp_state, sizeof(fdi_decomp_state));

  pathlen = pszCabPath ? strlen(pszCabPath) : 0;
  filenamelen = pszCabinet ? strlen(pszCabinet) : 0;

  /* slight overestimation here to save CPU cycles in the developer's brain */
  if ((pathlen + filenamelen + 3) > MAX_PATH) {
    ERR("MAX_PATH exceeded.\n");
    fdi->free(decomp_state);
    set_error( fdi, FDIERROR_CABINET_NOT_FOUND, ERROR_FILE_NOT_FOUND );
    return FALSE;
  }

  /* paste the path and filename together */
  fullpath[0] = '\0';
  if (pathlen)
    strcpy(fullpath, pszCabPath);
  if (filenamelen)
    strcat(fullpath, pszCabinet);

  TRACE("full cab path/file name: %s\n", debugstr_a(fullpath));

  /* get a handle to the cabfile */
  cabhf = fdi->open(fullpath, _O_RDONLY|_O_BINARY, _S_IREAD | _S_IWRITE);
  if (cabhf == -1) {
    fdi->free(decomp_state);
    set_error( fdi, FDIERROR_CABINET_NOT_FOUND, 0 );
    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
  }

  /* check if it's really a cabfile. Note that this doesn't implement the bug */
  if (!FDI_read_entries(fdi, cabhf, &fdici, &(CAB(mii)))) {
    WARN("FDI_read_entries failed: %u\n", fdi->perf->erfOper);
    fdi->free(decomp_state);
    fdi->close(cabhf);
    return FALSE;
  }

  /* cabinet notification */
  ZeroMemory(&fdin, sizeof(FDINOTIFICATION));
  fdin.setID = fdici.setID;
  fdin.iCabinet = fdici.iCabinet;
  fdin.pv = pvUser;
  fdin.psz1 = (CAB(mii).nextname) ? CAB(mii).nextname : &emptystring;
  fdin.psz2 = (CAB(mii).nextinfo) ? CAB(mii).nextinfo : &emptystring;
  fdin.psz3 = pszCabPath;

  if (pfnfdin(fdintCABINET_INFO, &fdin) == -1) {
    set_error( fdi, FDIERROR_USER_ABORT, 0 );
    goto bail_and_fail;
  }

  CAB(setID) = fdici.setID;
  CAB(iCabinet) = fdici.iCabinet;
  CAB(cabhf) = cabhf;

  /* read folders */
  for (i = 0; i < fdici.cFolders; i++) {
    if (fdi->read(cabhf, buf, cffold_SIZEOF) != cffold_SIZEOF) {
      set_error( fdi, FDIERROR_CORRUPT_CABINET, 0 );
      goto bail_and_fail;
    }

    if (CAB(mii).folder_resv > 0)
      fdi->seek(cabhf, CAB(mii).folder_resv, SEEK_CUR);

    fol = fdi->alloc(sizeof(struct fdi_folder));
    if (!fol) {
      ERR("out of memory!\n");
      set_error( fdi, FDIERROR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY );
      goto bail_and_fail;
    }
    ZeroMemory(fol, sizeof(struct fdi_folder));
    if (!CAB(firstfol)) CAB(firstfol) = fol;

    fol->offset = (cab_off_t) EndGetI32(buf+cffold_DataOffset);
    fol->num_blocks = EndGetI16(buf+cffold_NumBlocks);
    fol->comp_type  = EndGetI16(buf+cffold_CompType);

    if (linkfol)
      linkfol->next = fol; 
    linkfol = fol;
  }

  /* read files */
  for (i = 0; i < fdici.cFiles; i++) {
    if (fdi->read(cabhf, buf, cffile_SIZEOF) != cffile_SIZEOF) {
      set_error( fdi, FDIERROR_CORRUPT_CABINET, 0 );
      goto bail_and_fail;
    }

    file = fdi->alloc(sizeof(struct fdi_file));
    if (!file) { 
      ERR("out of memory!\n"); 
      set_error( fdi, FDIERROR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY );
      goto bail_and_fail;
    }
    ZeroMemory(file, sizeof(struct fdi_file));
    if (!CAB(firstfile)) CAB(firstfile) = file;
      
    file->length   = EndGetI32(buf+cffile_UncompressedSize);
    file->offset   = EndGetI32(buf+cffile_FolderOffset);
    file->index    = EndGetI16(buf+cffile_FolderIndex);
    file->time     = EndGetI16(buf+cffile_Time);
    file->date     = EndGetI16(buf+cffile_Date);
    file->attribs  = EndGetI16(buf+cffile_Attribs);
    file->filename = FDI_read_string(fdi, cabhf, fdici.cbCabinet);

    if (!file->filename) {
      set_error( fdi, FDIERROR_CORRUPT_CABINET, 0 );
      goto bail_and_fail;
    }

    if (linkfile)
      linkfile->next = file;
    linkfile = file;
  }

  for (file = CAB(firstfile); (file); file = file->next) {

    /*
     * FIXME: This implementation keeps multiple cabinet files open at once
     * when encountering a split cabinet.  It is a quirk of this implementation
     * that sometimes we decrypt the same block of data more than once, to find
     * the right starting point for a file, moving the file-pointer backwards.
     * If we kept a cache of certain file-pointer information, we could eliminate
     * that behavior... in fact I am not sure that the caching we already have
     * is not sufficient.
     * 
     * The current implementation seems to work fine in straightforward situations
     * where all the cabinet files needed for decryption are simultaneously
     * available.  But presumably, the API is supposed to support cabinets which
     * are split across multiple CDROMS; we may need to change our implementation
     * to strictly serialize its file usage so that it opens only one cabinet
     * at a time.  Some experimentation with Windows is needed to figure out the
     * precise semantics required.  The relevant code is here and in fdi_decomp().
     */

    /* partial-file notification */
    if ((file->index & cffileCONTINUED_FROM_PREV) == cffileCONTINUED_FROM_PREV) {
      /*
       * FIXME: Need to create a Cabinet with a single file spanning multiple files
       * and perform some tests to figure out the right behavior.  The SDK says
       * FDICopy will notify the user of the filename and "disk name" (info) of
       * the cabinet where the spanning file /started/.
       *
       * That would certainly be convenient for the API-user, who could abort,
       * everything (or parallelize, if that's allowed (it is in wine)), and call
       * FDICopy again with the provided filename, so as to avoid partial file
       * notification and successfully unpack.  This task could be quite unpleasant
       * from wine's perspective: the information specifying the "start cabinet" for
       * a file is associated nowhere with the file header and is not to be found in
       * the cabinet header.  We have only the index of the cabinet wherein the folder
       * begins, which contains the file.  To find that cabinet, we must consider the
       * index of the current cabinet, and chain backwards, cabinet-by-cabinet (for
       * each cabinet refers to its "next" and "previous" cabinet only, like a linked
       * list).
       *
       * Bear in mind that, in the spirit of CABINET.DLL, we must assume that any
       * cabinet other than the active one might be at another filepath than the
       * current one, or on another CDROM. This could get rather dicey, especially
       * if we imagine parallelized access to the FDICopy API.
       *
       * The current implementation punts -- it just returns the previous cabinet and
       * its info from the header of this cabinet.  This provides the right answer in
       * 95% of the cases; it's worth checking if Microsoft cuts the same corner before
       * we "fix" it.
       */
      ZeroMemory(&fdin, sizeof(FDINOTIFICATION));
      fdin.pv = pvUser;
      fdin.psz1 = (char *)file->filename;
      fdin.psz2 = (CAB(mii).prevname) ? CAB(mii).prevname : &emptystring;
      fdin.psz3 = (CAB(mii).previnfo) ? CAB(mii).previnfo : &emptystring;

      if (pfnfdin(fdintPARTIAL_FILE, &fdin) == -1) {
        set_error( fdi, FDIERROR_USER_ABORT, 0 );
        goto bail_and_fail;
      }
      /* I don't think we are supposed to decompress partial files.  This prevents it. */
      file->oppressed = TRUE;
    }
    if (file->oppressed) {
      filehf = 0;
    } else {
      ZeroMemory(&fdin, sizeof(FDINOTIFICATION));
      fdin.pv = pvUser;
      fdin.psz1 = (char *)file->filename;
      fdin.cb = file->length;
      fdin.date = file->date;
      fdin.time = file->time;
      fdin.attribs = file->attribs;
      fdin.iFolder = file->index;
      if ((filehf = ((*pfnfdin)(fdintCOPY_FILE, &fdin))) == -1) {
        set_error( fdi, FDIERROR_USER_ABORT, 0 );
        filehf = 0;
        goto bail_and_fail;
      }
    }

    /* find the folder for this file if necc. */
    if (filehf) {
      fol = CAB(firstfol);
      if ((file->index & cffileCONTINUED_TO_NEXT) == cffileCONTINUED_TO_NEXT) {
        /* pick the last folder */
        while (fol->next) fol = fol->next;
      } else {
        unsigned int i2;

        for (i2 = 0; (i2 < file->index); i2++)
          if (fol->next) /* bug resistance, should always be true */
            fol = fol->next;
      }
    }

    if (filehf) {
      cab_UWORD comptype = fol->comp_type;
      int ct1 = comptype & cffoldCOMPTYPE_MASK;
      int ct2 = CAB(current) ? (CAB(current)->comp_type & cffoldCOMPTYPE_MASK) : 0;
      int err = 0;

      TRACE("Extracting file %s as requested by callee.\n", debugstr_a(file->filename));

      /* set up decomp_state */
      CAB(fdi) = fdi;
      CAB(filehf) = filehf;

      /* Was there a change of folder?  Compression type?  Did we somehow go backwards? */
      if ((ct1 != ct2) || (CAB(current) != fol) || (file->offset < CAB(offset))) {

        TRACE("Resetting folder for file %s.\n", debugstr_a(file->filename));

        /* free stuff for the old decompressor */
        switch (ct2) {
        case cffoldCOMPTYPE_LZX:
          if (LZX(window)) {
            fdi->free(LZX(window));
            LZX(window) = NULL;
          }
          break;
        case cffoldCOMPTYPE_QUANTUM:
          if (QTM(window)) {
            fdi->free(QTM(window));
            QTM(window) = NULL;
          }
          break;
        }

        CAB(decomp_cab) = NULL;
        CAB(fdi)->seek(CAB(cabhf), fol->offset, SEEK_SET);
        CAB(offset) = 0;
        CAB(outlen) = 0;

        /* initialize the new decompressor */
        switch (ct1) {
        case cffoldCOMPTYPE_NONE:
          CAB(decompress) = NONEfdi_decomp;
          break;
        case cffoldCOMPTYPE_MSZIP:
          CAB(decompress) = ZIPfdi_decomp;
          break;
        case cffoldCOMPTYPE_QUANTUM:
          CAB(decompress) = QTMfdi_decomp;
          err = QTMfdi_init((comptype >> 8) & 0x1f, (comptype >> 4) & 0xF, decomp_state);
          break;
        case cffoldCOMPTYPE_LZX:
          CAB(decompress) = LZXfdi_decomp;
          err = LZXfdi_init((comptype >> 8) & 0x1f, decomp_state);
          break;
        default:
          err = DECR_DATAFORMAT;
        }
      }

      CAB(current) = fol;

      switch (err) {
        case DECR_OK:
          break;
        case DECR_NOMEMORY:
          set_error( fdi, FDIERROR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY );
          goto bail_and_fail;
        default:
          set_error( fdi, FDIERROR_CORRUPT_CABINET, 0 );
          goto bail_and_fail;
      }

      if (file->offset > CAB(offset)) {
        /* decode bytes and send them to /dev/null */
        switch (fdi_decomp(file, 0, decomp_state, pszCabPath, pfnfdin, pvUser)) {
          case DECR_OK:
            break;
          case DECR_USERABORT:
            set_error( fdi, FDIERROR_USER_ABORT, 0 );
            goto bail_and_fail;
          case DECR_NOMEMORY:
            set_error( fdi, FDIERROR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY );
            goto bail_and_fail;
          default:
            set_error( fdi, FDIERROR_CORRUPT_CABINET, 0 );
            goto bail_and_fail;
        }
        CAB(offset) = file->offset;
      }

      /* now do the actual decompression */
      err = fdi_decomp(file, 1, decomp_state, pszCabPath, pfnfdin, pvUser);
      if (err) CAB(current) = NULL; else CAB(offset) += file->length;

      /* fdintCLOSE_FILE_INFO notification */
      ZeroMemory(&fdin, sizeof(FDINOTIFICATION));
      fdin.pv = pvUser;
      fdin.psz1 = (char *)file->filename;
      fdin.hf = filehf;
      fdin.cb = (file->attribs & cffile_A_EXEC) != 0; /* FIXME: is that right? */
      fdin.date = file->date;
      fdin.time = file->time;
      fdin.attribs = file->attribs; /* FIXME: filter _A_EXEC? */
      fdin.iFolder = file->index;
      ((*pfnfdin)(fdintCLOSE_FILE_INFO, &fdin));
      filehf = 0;

      switch (err) {
        case DECR_OK:
          break;
        case DECR_USERABORT:
          set_error( fdi, FDIERROR_USER_ABORT, 0 );
          goto bail_and_fail;
        case DECR_NOMEMORY:
          set_error( fdi, FDIERROR_ALLOC_FAIL, ERROR_NOT_ENOUGH_MEMORY );
          goto bail_and_fail;
        default:
          set_error( fdi, FDIERROR_CORRUPT_CABINET, 0 );
          goto bail_and_fail;
      }
    }
  }

  if (fol) free_decompression_temps(fdi, fol, decomp_state);
  free_decompression_mem(fdi, decomp_state);
 
  return TRUE;

  bail_and_fail: /* here we free ram before error returns */

  if (fol) free_decompression_temps(fdi, fol, decomp_state);

  if (filehf) fdi->close(filehf);

  free_decompression_mem(fdi, decomp_state);

  return FALSE;
}

/***********************************************************************
 *		FDIDestroy (CABINET.23)
 *
 * Frees a handle created by FDICreate.  Do /not/ call this in the middle
 * of FDICopy.  Only reason for failure would be an invalid handle.
 * 
 * PARAMS
 *   hfdi [I] The HFDI to free
 *
 * RETURNS
 *   TRUE for success
 *   FALSE for failure
 */
BOOL __cdecl FDIDestroy(HFDI hfdi)
{
    FDI_Int *fdi = get_fdi_ptr( hfdi );

    TRACE("(hfdi == ^%p)\n", hfdi);
    if (!fdi) return FALSE;
    fdi->magic = 0; /* paranoia */
    fdi->free(fdi);
    return TRUE;
}

/***********************************************************************
 *		FDITruncateCabinet (CABINET.24)
 *
 * Removes all folders of a cabinet file after and including the
 * specified folder number.
 * 
 * PARAMS
 *   hfdi            [I] Handle to the FDI context.
 *   pszCabinetName  [I] Filename of the cabinet.
 *   iFolderToDelete [I] Index of the first folder to delete.
 * 
 * RETURNS
 *   Success: TRUE.
 *   Failure: FALSE.
 * 
 * NOTES
 *   The PFNWRITE function supplied to FDICreate must truncate the
 *   file at the current position if the number of bytes to write is 0.
 */
BOOL __cdecl FDITruncateCabinet(
	HFDI    hfdi,
	char   *pszCabinetName,
	USHORT  iFolderToDelete)
{
  FDI_Int *fdi = get_fdi_ptr( hfdi );

  FIXME("(hfdi == ^%p, pszCabinetName == %s, iFolderToDelete == %hu): stub\n",
    hfdi, debugstr_a(pszCabinetName), iFolderToDelete);

  if (!fdi) return FALSE;

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
