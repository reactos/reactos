/*
 * cabextract.c
 *
 * Copyright 2000-2002 Stuart Caie
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Principal author: Stuart Caie <kyzer@4u.net>
 *
 * Based on specification documents from Microsoft Corporation
 * Quantum decompression researched and implemented by Matthew Russoto
 * Huffman code adapted from unlzx by Dave Tritscher.
 * InfoZip team's INFLATE implementation adapted to MSZIP by Dirk Stoecker.
 * Major LZX fixes by Jae Jung.
 */
 
#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"

#include "cabinet.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cabinet);

THOSE_ZIP_CONSTS;

/* all the file IO is abstracted into these routines:
 * cabinet_(open|close|read|seek|skip|getoffset)
 * file_(open|close|write)
 */

/* try to open a cabinet file, returns success */
BOOL cabinet_open(struct cabinet *cab)
{
  char *name = (char *)cab->filename;
  HANDLE fh;

  TRACE("(cab == ^%p)\n", cab);

  if ((fh = CreateFileA( name, GENERIC_READ, FILE_SHARE_READ,
              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL )) == INVALID_HANDLE_VALUE) {
    ERR("Couldn't open %s\n", debugstr_a(name));
    return FALSE;
  }

  /* seek to end of file and get the length */
  if ((cab->filelen = SetFilePointer(fh, 0, NULL, FILE_END)) == INVALID_SET_FILE_POINTER) {
    if (GetLastError() != NO_ERROR) {
      ERR("Seek END failed: %s\n", debugstr_a(name));
      CloseHandle(fh);
      return FALSE;
    }
  }

  /* return to the start of the file */
  if (SetFilePointer(fh, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
    ERR("Seek BEGIN failed: %s\n", debugstr_a(name));
    CloseHandle(fh);
    return FALSE;
  }

  cab->fh = fh;
  return TRUE;
}

/*******************************************************************
 * cabinet_close (internal)
 *
 * close the file handle in a struct cabinet.
 */
void cabinet_close(struct cabinet *cab) {
  TRACE("(cab == ^%p)\n", cab);
  if (cab->fh) CloseHandle(cab->fh);
  cab->fh = 0;
}

/*******************************************************
 * ensure_filepath2 (internal)
 */
BOOL ensure_filepath2(char *path) {
  BOOL ret = TRUE;
  int len;
  char *new_path;

  new_path = HeapAlloc(GetProcessHeap(), 0, (strlen(path) + 1));
  strcpy(new_path, path);

  while((len = strlen(new_path)) && new_path[len - 1] == '\\')
    new_path[len - 1] = 0;

  TRACE("About to try to create directory %s\n", debugstr_a(new_path));
  while(!CreateDirectoryA(new_path, NULL)) {
    char *slash;
    DWORD last_error = GetLastError();

    if(last_error == ERROR_ALREADY_EXISTS)
      break;

    if(last_error != ERROR_PATH_NOT_FOUND) {
      ret = FALSE;
      break;
    }

    if(!(slash = strrchr(new_path, '\\'))) {
      ret = FALSE;
      break;
    }

    len = slash - new_path;
    new_path[len] = 0;
    if(! ensure_filepath2(new_path)) {
      ret = FALSE;
      break;
    }
    new_path[len] = '\\';
    TRACE("New path in next iteration: %s\n", debugstr_a(new_path));
  }

  HeapFree(GetProcessHeap(), 0, new_path);
  return ret;
}


/**********************************************************************
 * ensure_filepath (internal)
 *
 * ensure_filepath("a\b\c\d.txt") ensures a, a\b and a\b\c exist as dirs
 */
BOOL ensure_filepath(char *path) {
  char new_path[MAX_PATH];
  int len, i, lastslashpos = -1;

  TRACE("(path == %s)\n", debugstr_a(path));

  strcpy(new_path, path); 
  /* remove trailing slashes (shouldn't need to but wth...) */
  while ((len = strlen(new_path)) && new_path[len - 1] == '\\')
    new_path[len - 1] = 0;
  /* find the position of the last '\\' */
  for (i=0; i<MAX_PATH; i++) {
    if (new_path[i] == 0) break; 
    if (new_path[i] == '\\')
      lastslashpos = i;
  }
  if (lastslashpos > 0) {
    new_path[lastslashpos] = 0;
    /* may be trailing slashes but ensure_filepath2 will chop them */
    return ensure_filepath2(new_path);
  } else
    return TRUE; /* ? */
}

/*******************************************************************
 * file_open (internal)
 *
 * opens a file for output, returns success
 */
BOOL file_open(struct cab_file *fi, BOOL lower, LPCSTR dir)
{
  char c, *s, *d, *name;
  BOOL ok = FALSE;

  TRACE("(fi == ^%p, lower == %s, dir == %s)\n", fi, lower ? "TRUE" : "FALSE", debugstr_a(dir));

  if (!(name = malloc(strlen(fi->filename) + (dir ? strlen(dir) : 0) + 2))) {
    ERR("out of memory!\n");
    return FALSE;
  }
  
  /* start with blank name */
  *name = 0;

  /* add output directory if needed */
  if (dir) {
    strcpy(name, dir);
    strcat(name, "\\");
  }

  /* remove leading slashes */
  s = (char *) fi->filename;
  while (*s == '\\') s++;

  /* copy from fi->filename to new name.
   * lowercases characters if needed.
   */
  d = &name[strlen(name)];
  do {
    c = *s++;
    *d++ = (lower ? tolower((unsigned char) c) : c);
  } while (c);

  /* create directories if needed, attempt to write file */
  if (ensure_filepath(name)) {
    fi->fh = CreateFileA(name, GENERIC_WRITE, 0, NULL,
                         CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (fi->fh != INVALID_HANDLE_VALUE)
      ok = TRUE;
    else {
      ERR("CreateFileA returned INVALID_HANDLE_VALUE\n");
      fi->fh = 0;
    }
  } else 
    ERR("Couldn't ensure filepath for %s\n", debugstr_a(name));

  if (!ok) {
    ERR("Couldn't open file %s for writing\n", debugstr_a(name));
  }

  /* as full filename is no longer needed, free it */
  free(name);

  return ok;
}

/********************************************************
 * close_file (internal)
 *
 * closes a completed file
 */
void file_close(struct cab_file *fi)
{
  TRACE("(fi == ^%p)\n", fi);

  if (fi->fh) {
    CloseHandle(fi->fh);
  }
  fi->fh = 0;
}

/******************************************************************
 * file_write (internal)
 *
 * writes from buf to a file specified as a cab_file struct.
 * returns success/failure
 */
BOOL file_write(struct cab_file *fi, cab_UBYTE *buf, cab_off_t length)
{
  DWORD bytes_written;

  TRACE("(fi == ^%p, buf == ^%p, length == %u)\n", fi, buf, length);

  if ((!WriteFile( fi->fh, (LPCVOID) buf, length, &bytes_written, FALSE) ||
      (bytes_written != length))) {
    ERR("Error writing file: %s\n", debugstr_a(fi->filename));
    return FALSE;
  }
  return TRUE;
}


/*******************************************************************
 * cabinet_skip (internal)
 *
 * advance the file pointer associated with the cab structure
 * by distance bytes
 */
void cabinet_skip(struct cabinet *cab, cab_off_t distance)
{
  TRACE("(cab == ^%p, distance == %u)\n", cab, distance);
  if (SetFilePointer(cab->fh, distance, NULL, FILE_CURRENT) == INVALID_SET_FILE_POINTER) {
    if (distance != INVALID_SET_FILE_POINTER)
      ERR("%s\n", debugstr_a((char *) cab->filename));
  }
}

/*******************************************************************
 * cabinet_seek (internal)
 *
 * seek to the specified absolute offset in a cab
 */
void cabinet_seek(struct cabinet *cab, cab_off_t offset) {
  TRACE("(cab == ^%p, offset == %u)\n", cab, offset);
  if (SetFilePointer(cab->fh, offset, NULL, FILE_BEGIN) != offset)
    ERR("%s seek failure\n", debugstr_a((char *)cab->filename));
}

/*******************************************************************
 * cabinet_getoffset (internal)
 *
 * returns the file pointer position of a cab
 */
cab_off_t cabinet_getoffset(struct cabinet *cab) 
{
  return SetFilePointer(cab->fh, 0, NULL, FILE_CURRENT);
}

/*******************************************************************
 * cabinet_read (internal)
 *
 * read data from a cabinet, returns success
 */
BOOL cabinet_read(struct cabinet *cab, cab_UBYTE *buf, cab_off_t length)
{
  DWORD bytes_read;
  cab_off_t avail = cab->filelen - cabinet_getoffset(cab);

  TRACE("(cab == ^%p, buf == ^%p, length == %u)\n", cab, buf, length);

  if (length > avail) {
    WARN("%s: WARNING; cabinet is truncated\n", debugstr_a((char *)cab->filename));
    length = avail;
  }

  if (! ReadFile( cab->fh, (LPVOID) buf, length, &bytes_read, NULL )) {
    ERR("%s read error\n", debugstr_a((char *) cab->filename));
    return FALSE;
  } else if (bytes_read != length) {
    ERR("%s read size mismatch\n", debugstr_a((char *) cab->filename));
    return FALSE;
  }

  return TRUE;
}

/**********************************************************************
 * cabinet_read_string (internal)
 *
 * allocate and read an aribitrarily long string from the cabinet
 */
char *cabinet_read_string(struct cabinet *cab)
{
  cab_off_t len=256, base = cabinet_getoffset(cab), maxlen = cab->filelen - base;
  BOOL ok = FALSE;
  int i;
  cab_UBYTE *buf = NULL;

  TRACE("(cab == ^%p)\n", cab);

  do {
    if (len > maxlen) len = maxlen;
    if (!(buf = realloc(buf, (size_t) len))) break;
    if (!cabinet_read(cab, buf, (size_t) len)) break;

    /* search for a null terminator in what we've just read */
    for (i=0; i < len; i++) {
      if (!buf[i]) {ok=TRUE; break;}
    }

    if (!ok) {
      if (len == maxlen) {
        ERR("%s: WARNING; cabinet is truncated\n", debugstr_a((char *) cab->filename));
        break;
      }
      len += 256;
      cabinet_seek(cab, base);
    }
  } while (!ok);

  if (!ok) {
    if (buf)
      free(buf);
    else
      ERR("out of memory!\n");
    return NULL;
  }

  /* otherwise, set the stream to just after the string and return */
  cabinet_seek(cab, base + ((cab_off_t) strlen((char *) buf)) + 1);

  return (char *) buf;
}

/******************************************************************
 * cabinet_read_entries (internal)
 *
 * reads the header and all folder and file entries in this cabinet
 */
BOOL cabinet_read_entries(struct cabinet *cab)
{
  int num_folders, num_files, header_resv, folder_resv = 0, i;
  struct cab_folder *fol, *linkfol = NULL;
  struct cab_file *file, *linkfile = NULL;
  cab_off_t base_offset;
  cab_UBYTE buf[64];

  TRACE("(cab == ^%p)\n", cab);

  /* read in the CFHEADER */
  base_offset = cabinet_getoffset(cab);
  if (!cabinet_read(cab, buf, cfhead_SIZEOF)) {
    return FALSE;
  }
  
  /* check basic MSCF signature */
  if (EndGetI32(buf+cfhead_Signature) != 0x4643534d) {
    ERR("%s: not a Microsoft cabinet file\n", debugstr_a((char *) cab->filename));
    return FALSE;
  }

  /* get the number of folders */
  num_folders = EndGetI16(buf+cfhead_NumFolders);
  if (num_folders == 0) {
    ERR("%s: no folders in cabinet\n", debugstr_a((char *) cab->filename));
    return FALSE;
  }

  /* get the number of files */
  num_files = EndGetI16(buf+cfhead_NumFiles);
  if (num_files == 0) {
    ERR("%s: no files in cabinet\n", debugstr_a((char *) cab->filename));
    return FALSE;
  }

  /* just check the header revision */
  if ((buf[cfhead_MajorVersion] > 1) ||
      (buf[cfhead_MajorVersion] == 1 && buf[cfhead_MinorVersion] > 3))
  {
    WARN("%s: WARNING; cabinet format version > 1.3\n", debugstr_a((char *) cab->filename));
  }

  /* read the reserved-sizes part of header, if present */
  cab->flags = EndGetI16(buf+cfhead_Flags);
  if (cab->flags & cfheadRESERVE_PRESENT) {
    if (!cabinet_read(cab, buf, cfheadext_SIZEOF)) return FALSE;
    header_resv     = EndGetI16(buf+cfheadext_HeaderReserved);
    folder_resv     = buf[cfheadext_FolderReserved];
    cab->block_resv = buf[cfheadext_DataReserved];

    if (header_resv > 60000) {
      WARN("%s: WARNING; header reserved space > 60000\n", debugstr_a((char *) cab->filename));
    }

    /* skip the reserved header */
    if (header_resv) 
      if (SetFilePointer(cab->fh, (cab_off_t) header_resv, NULL, FILE_CURRENT) == INVALID_SET_FILE_POINTER)
        ERR("seek failure: %s\n", debugstr_a((char *) cab->filename));
  }

  if (cab->flags & cfheadPREV_CABINET) {
    cab->prevname = cabinet_read_string(cab);
    if (!cab->prevname) return FALSE;
    cab->previnfo = cabinet_read_string(cab);
  }

  if (cab->flags & cfheadNEXT_CABINET) {
    cab->nextname = cabinet_read_string(cab);
    if (!cab->nextname) return FALSE;
    cab->nextinfo = cabinet_read_string(cab);
  }

  /* read folders */
  for (i = 0; i < num_folders; i++) {
    if (!cabinet_read(cab, buf, cffold_SIZEOF)) return FALSE;
    if (folder_resv) cabinet_skip(cab, folder_resv);

    fol = (struct cab_folder *) calloc(1, sizeof(struct cab_folder));
    if (!fol) {
      ERR("out of memory!\n");
      return FALSE;
    }

    fol->cab[0]     = cab;
    fol->offset[0]  = base_offset + (cab_off_t) EndGetI32(buf+cffold_DataOffset);
    fol->num_blocks = EndGetI16(buf+cffold_NumBlocks);
    fol->comp_type  = EndGetI16(buf+cffold_CompType);

    if (!linkfol) 
      cab->folders = fol; 
    else 
      linkfol->next = fol;

    linkfol = fol;
  }

  /* read files */
  for (i = 0; i < num_files; i++) {
    if (!cabinet_read(cab, buf, cffile_SIZEOF))
      return FALSE;

    file = (struct cab_file *) calloc(1, sizeof(struct cab_file));
    if (!file) { 
      ERR("out of memory!\n"); 
      return FALSE;
    }
      
    file->length   = EndGetI32(buf+cffile_UncompressedSize);
    file->offset   = EndGetI32(buf+cffile_FolderOffset);
    file->index    = EndGetI16(buf+cffile_FolderIndex);
    file->time     = EndGetI16(buf+cffile_Time);
    file->date     = EndGetI16(buf+cffile_Date);
    file->attribs  = EndGetI16(buf+cffile_Attribs);
    file->filename = cabinet_read_string(cab);

    if (!file->filename) {
      free(file);
      return FALSE;
    }

    if (!linkfile) 
      cab->files = file;
    else 
      linkfile->next = file;

    linkfile = file;
  }
  return TRUE;
}

/***********************************************************
 * load_cab_offset (internal)
 *
 * validates and reads file entries from a cabinet at offset [offset] in
 * file [name]. Returns a cabinet structure if successful, or NULL
 * otherwise.
 */
struct cabinet *load_cab_offset(LPCSTR name, cab_off_t offset)
{
  struct cabinet *cab = (struct cabinet *) calloc(1, sizeof(struct cabinet));
  int ok;

  TRACE("(name == %s, offset == %u)\n", debugstr_a((char *) name), offset);

  if (!cab) return NULL;

  cab->filename = name;
  if ((ok = cabinet_open(cab))) {
    cabinet_seek(cab, offset);
    ok = cabinet_read_entries(cab);
    cabinet_close(cab);
  }

  if (ok) return cab;
  free(cab);
  return NULL;
}

/* MSZIP decruncher */

/* Dirk Stoecker wrote the ZIP decoder, based on the InfoZip deflate code */

/********************************************************
 * Ziphuft_free (internal)
 */
void Ziphuft_free(struct Ziphuft *t)
{
  register struct Ziphuft *p, *q;

  /* Go through linked list, freeing from the allocated (t[-1]) address. */
  p = t;
  while (p != (struct Ziphuft *)NULL)
  {
    q = (--p)->v.t;
    free(p);
    p = q;
  } 
}

/*********************************************************
 * Ziphuft_build (internal)
 */
cab_LONG Ziphuft_build(cab_ULONG *b, cab_ULONG n, cab_ULONG s, cab_UWORD *d, cab_UWORD *e,
struct Ziphuft **t, cab_LONG *m, cab_decomp_state *decomp_state)
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
    *t = (struct Ziphuft *)NULL;
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
  ZIP(u)[0] = (struct Ziphuft *)NULL;   /* just to keep compilers happy */
  q = (struct Ziphuft *)NULL;      /* ditto */
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
        z = (z = g - w) > (cab_ULONG)*m ? *m : z;        /* upper limit */
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
        if (!(q = (struct Ziphuft *) malloc((z + 1)*sizeof(struct Ziphuft))))
        {
          if(h)
            Ziphuft_free(ZIP(u)[0]);
          return 3;             /* not enough memory */
        }
        *t = q + 1;             /* link to list for Ziphuft_free() */
        *(t = &(q->v.t)) = (struct Ziphuft *)NULL;
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
 * Zipinflate_codes (internal)
 */
cab_LONG Zipinflate_codes(struct Ziphuft *tl, struct Ziphuft *td,
  cab_LONG bl, cab_LONG bd, cab_decomp_state *decomp_state)
{
  register cab_ULONG e;  /* table entry flag/number of extra bits */
  cab_ULONG n, d;        /* length and index for copy */
  cab_ULONG w;           /* current window position */
  struct Ziphuft *t;     /* pointer to table entry */
  cab_ULONG ml, md;      /* masks for bl and bd bits */
  register cab_ULONG b;  /* bit buffer */
  register cab_ULONG k;  /* number of bits in bit buffer */

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
    if((e = (t = tl + ((cab_ULONG)b & ml))->e) > 16)
      do
      {
        if (e == 99)
          return 1;
        ZIPDUMPBITS(t->b)
        e -= 16;
        ZIPNEEDBITS(e)
      } while ((e = (t = t->v.t + ((cab_ULONG)b & Zipmask[e]))->e) > 16);
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
      n = t->v.n + ((cab_ULONG)b & Zipmask[e]);
      ZIPDUMPBITS(e);

      /* decode distance of block to copy */
      ZIPNEEDBITS((cab_ULONG)bd)
      if ((e = (t = td + ((cab_ULONG)b & md))->e) > 16)
        do {
          if (e == 99)
            return 1;
          ZIPDUMPBITS(t->b)
          e -= 16;
          ZIPNEEDBITS(e)
        } while ((e = (t = t->v.t + ((cab_ULONG)b & Zipmask[e]))->e) > 16);
      ZIPDUMPBITS(t->b)
      ZIPNEEDBITS(e)
      d = w - t->v.n - ((cab_ULONG)b & Zipmask[e]);
      ZIPDUMPBITS(e)
      do
      {
        n -= (e = (e = ZIPWSIZE - ((d &= ZIPWSIZE-1) > w ? d : w)) > n ?n:e);
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
cab_LONG Zipinflate_stored(cab_decomp_state *decomp_state)
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
  n = ((cab_ULONG)b & 0xffff);
  ZIPDUMPBITS(16)
  ZIPNEEDBITS(16)
  if (n != (cab_ULONG)((~b) & 0xffff))
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
 * Zipinflate_fixed (internal)
 */
cab_LONG Zipinflate_fixed(cab_decomp_state *decomp_state)
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
  if((i = Ziphuft_build(l, 288, 257, (cab_UWORD *) Zipcplens,
  (cab_UWORD *) Zipcplext, &fixed_tl, &fixed_bl, decomp_state)))
    return i;

  /* distance table */
  for(i = 0; i < 30; i++)      /* make an incomplete code set */
    l[i] = 5;
  fixed_bd = 5;
  if((i = Ziphuft_build(l, 30, 0, (cab_UWORD *) Zipcpdist, (cab_UWORD *) Zipcpdext,
  &fixed_td, &fixed_bd, decomp_state)) > 1)
  {
    Ziphuft_free(fixed_tl);
    return i;
  }

  /* decompress until an end-of-block code */
  i = Zipinflate_codes(fixed_tl, fixed_td, fixed_bl, fixed_bd, decomp_state);

  Ziphuft_free(fixed_td);
  Ziphuft_free(fixed_tl);
  return i;
}

/**************************************************************
 * Zipinflate_dynamic (internal)
 */
cab_LONG Zipinflate_dynamic(cab_decomp_state *decomp_state)
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
  nl = 257 + ((cab_ULONG)b & 0x1f);      /* number of literal/length codes */
  ZIPDUMPBITS(5)
  ZIPNEEDBITS(5)
  nd = 1 + ((cab_ULONG)b & 0x1f);        /* number of distance codes */
  ZIPDUMPBITS(5)
  ZIPNEEDBITS(4)
  nb = 4 + ((cab_ULONG)b & 0xf);         /* number of bit length codes */
  ZIPDUMPBITS(4)
  if(nl > 288 || nd > 32)
    return 1;                   /* bad lengths */

  /* read in bit-length-code lengths */
  for(j = 0; j < nb; j++)
  {
    ZIPNEEDBITS(3)
    ll[Zipborder[j]] = (cab_ULONG)b & 7;
    ZIPDUMPBITS(3)
  }
  for(; j < 19; j++)
    ll[Zipborder[j]] = 0;

  /* build decoding table for trees--single level, 7 bit lookup */
  bl = 7;
  if((i = Ziphuft_build(ll, 19, 19, NULL, NULL, &tl, &bl, decomp_state)) != 0)
  {
    if(i == 1)
      Ziphuft_free(tl);
    return i;                   /* incomplete code set */
  }

  /* read in literal and distance code lengths */
  n = nl + nd;
  m = Zipmask[bl];
  i = l = 0;
  while((cab_ULONG)i < n)
  {
    ZIPNEEDBITS((cab_ULONG)bl)
    j = (td = tl + ((cab_ULONG)b & m))->b;
    ZIPDUMPBITS(j)
    j = td->v.n;
    if (j < 16)                 /* length of code in bits (0..15) */
      ll[i++] = l = j;          /* save last length in l */
    else if (j == 16)           /* repeat last length 3 to 6 times */
    {
      ZIPNEEDBITS(2)
      j = 3 + ((cab_ULONG)b & 3);
      ZIPDUMPBITS(2)
      if((cab_ULONG)i + j > n)
        return 1;
      while (j--)
        ll[i++] = l;
    }
    else if (j == 17)           /* 3 to 10 zero length codes */
    {
      ZIPNEEDBITS(3)
      j = 3 + ((cab_ULONG)b & 7);
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
      j = 11 + ((cab_ULONG)b & 0x7f);
      ZIPDUMPBITS(7)
      if ((cab_ULONG)i + j > n)
        return 1;
      while (j--)
        ll[i++] = 0;
      l = 0;
    }
  }

  /* free decoding table for trees */
  Ziphuft_free(tl);

  /* restore the global bit buffer */
  ZIP(bb) = b;
  ZIP(bk) = k;

  /* build the decoding tables for literal/length and distance codes */
  bl = ZIPLBITS;
  if((i = Ziphuft_build(ll, nl, 257, (cab_UWORD *) Zipcplens, (cab_UWORD *) Zipcplext,
                        &tl, &bl, decomp_state)) != 0)
  {
    if(i == 1)
      Ziphuft_free(tl);
    return i;                   /* incomplete code set */
  }
  bd = ZIPDBITS;
  Ziphuft_build(ll + nl, nd, 0, (cab_UWORD *) Zipcpdist, (cab_UWORD *) Zipcpdext,
                &td, &bd, decomp_state);

  /* decompress until an end-of-block code */
  if(Zipinflate_codes(tl, td, bl, bd, decomp_state))
    return 1;

  /* free the decoding tables, return */
  Ziphuft_free(tl);
  Ziphuft_free(td);
  return 0;
}

/*****************************************************
 * Zipinflate_block (internal)
 */
cab_LONG Zipinflate_block(cab_LONG *e, cab_decomp_state *decomp_state) /* e == last block flag */
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
  t = (cab_ULONG)b & 3;
  ZIPDUMPBITS(2)

  /* restore the global bit buffer */
  ZIP(bb) = b;
  ZIP(bk) = k;

  /* inflate that block type */
  if(t == 2)
    return Zipinflate_dynamic(decomp_state);
  if(t == 0)
    return Zipinflate_stored(decomp_state);
  if(t == 1)
    return Zipinflate_fixed(decomp_state);
  /* bad block type */
  return 2;
}

/****************************************************
 * ZIPdecompress (internal)
 */
int ZIPdecompress(int inlen, int outlen, cab_decomp_state *decomp_state)
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

  do
  {
    if(Zipinflate_block(&e, decomp_state))
      return DECR_ILLEGALDATA;
  } while(!e);

  /* return success */
  return DECR_OK;
}

/* Quantum decruncher */

/* This decruncher was researched and implemented by Matthew Russoto. */
/* It has since been tidied up by Stuart Caie */

/******************************************************************
 * QTMinitmodel (internal)
 *
 * Initialise a model which decodes symbols from [s] to [s]+[n]-1
 */
void QTMinitmodel(struct QTMmodel *m, struct QTMmodelsym *sym, int n, int s) {
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
 * QTMinit (internal)
 */
int QTMinit(int window, int level, cab_decomp_state *decomp_state) {
  int wndsize = 1 << window, msz = window * 2, i;
  cab_ULONG j;

  /* QTM supports window sizes of 2^10 (1Kb) through 2^21 (2Mb) */
  /* if a previously allocated window is big enough, keep it    */
  if (window < 10 || window > 21) return DECR_DATAFORMAT;
  if (QTM(actual_size) < wndsize) {
    if (QTM(window)) free(QTM(window));
    QTM(window) = NULL;
  }
  if (!QTM(window)) {
    if (!(QTM(window) = malloc(wndsize))) return DECR_NOMEMORY;
    QTM(actual_size) = wndsize;
  }
  QTM(window_size) = wndsize;
  QTM(window_posn) = 0;

  /* initialise static slot/extrabits tables */
  for (i = 0, j = 0; i < 27; i++) {
    CAB(q_length_extra)[i] = (i == 26) ? 0 : (i < 2 ? 0 : i - 2) >> 2;
    CAB(q_length_base)[i] = j; j += 1 << ((i == 26) ? 5 : CAB(q_length_extra)[i]);
  }
  for (i = 0, j = 0; i < 42; i++) {
    CAB(q_extra_bits)[i] = (i < 2 ? 0 : i-2) >> 1;
    CAB(q_position_base)[i] = j; j += 1 << CAB(q_extra_bits)[i];
  }

  /* initialise arithmetic coding models */

  QTMinitmodel(&QTM(model7), &QTM(m7sym)[0], 7, 0);

  QTMinitmodel(&QTM(model00), &QTM(m00sym)[0], 0x40, 0x00);
  QTMinitmodel(&QTM(model40), &QTM(m40sym)[0], 0x40, 0x40);
  QTMinitmodel(&QTM(model80), &QTM(m80sym)[0], 0x40, 0x80);
  QTMinitmodel(&QTM(modelC0), &QTM(mC0sym)[0], 0x40, 0xC0);

  /* model 4 depends on table size, ranges from 20 to 24  */
  QTMinitmodel(&QTM(model4), &QTM(m4sym)[0], (msz < 24) ? msz : 24, 0);
  /* model 5 depends on table size, ranges from 20 to 36  */
  QTMinitmodel(&QTM(model5), &QTM(m5sym)[0], (msz < 36) ? msz : 36, 0);
  /* model 6pos depends on table size, ranges from 20 to 42 */
  QTMinitmodel(&QTM(model6pos), &QTM(m6psym)[0], msz, 0);
  QTMinitmodel(&QTM(model6len), &QTM(m6lsym)[0], 27, 0);

  return DECR_OK;
}

/****************************************************************
 * QTMupdatemodel (internal)
 */
void QTMupdatemodel(struct QTMmodel *model, int sym) {
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

/*******************************************************************
 * QTMdecompress (internal)
 */
int QTMdecompress(int inlen, int outlen, cab_decomp_state *decomp_state)
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

/* LZX decruncher */

/* Microsoft's LZX document and their implementation of the
 * com.ms.util.cab Java package do not concur.
 *
 * In the LZX document, there is a table showing the correlation between
 * window size and the number of position slots. It states that the 1MB
 * window = 40 slots and the 2MB window = 42 slots. In the implementation,
 * 1MB = 42 slots, 2MB = 50 slots. The actual calculation is 'find the
 * first slot whose position base is equal to or more than the required
 * window size'. This would explain why other tables in the document refer
 * to 50 slots rather than 42.
 *
 * The constant NUM_PRIMARY_LENGTHS used in the decompression pseudocode
 * is not defined in the specification.
 *
 * The LZX document does not state the uncompressed block has an
 * uncompressed length field. Where does this length field come from, so
 * we can know how large the block is? The implementation has it as the 24
 * bits following after the 3 blocktype bits, before the alignment
 * padding.
 *
 * The LZX document states that aligned offset blocks have their aligned
 * offset huffman tree AFTER the main and length trees. The implementation
 * suggests that the aligned offset tree is BEFORE the main and length
 * trees.
 *
 * The LZX document decoding algorithm states that, in an aligned offset
 * block, if an extra_bits value is 1, 2 or 3, then that number of bits
 * should be read and the result added to the match offset. This is
 * correct for 1 and 2, but not 3, where just a huffman symbol (using the
 * aligned tree) should be read.
 *
 * Regarding the E8 preprocessing, the LZX document states 'No translation
 * may be performed on the last 6 bytes of the input block'. This is
 * correct.  However, the pseudocode provided checks for the *E8 leader*
 * up to the last 6 bytes. If the leader appears between -10 and -7 bytes
 * from the end, this would cause the next four bytes to be modified, at
 * least one of which would be in the last 6 bytes, which is not allowed
 * according to the spec.
 *
 * The specification states that the huffman trees must always contain at
 * least one element. However, many CAB files contain blocks where the
 * length tree is completely empty (because there are no matches), and
 * this is expected to succeed.
 */


/* LZX uses what it calls 'position slots' to represent match offsets.
 * What this means is that a small 'position slot' number and a small
 * offset from that slot are encoded instead of one large offset for
 * every match.
 * - lzx_position_base is an index to the position slot bases
 * - lzx_extra_bits states how many bits of offset-from-base data is needed.
 */

/************************************************************
 * LZXinit (internal)
 */
int LZXinit(int window, cab_decomp_state *decomp_state) {
  cab_ULONG wndsize = 1 << window;
  int i, j, posn_slots;

  /* LZX supports window sizes of 2^15 (32Kb) through 2^21 (2Mb) */
  /* if a previously allocated window is big enough, keep it     */
  if (window < 15 || window > 21) return DECR_DATAFORMAT;
  if (LZX(actual_size) < wndsize) {
    if (LZX(window)) free(LZX(window));
    LZX(window) = NULL;
  }
  if (!LZX(window)) {
    if (!(LZX(window) = malloc(wndsize))) return DECR_NOMEMORY;
    LZX(actual_size) = wndsize;
  }
  LZX(window_size) = wndsize;

  /* initialise static tables */
  for (i=0, j=0; i <= 50; i += 2) {
    CAB(extra_bits)[i] = CAB(extra_bits)[i+1] = j; /* 0,0,0,0,1,1,2,2,3,3... */
    if ((i != 0) && (j < 17)) j++; /* 0,0,1,2,3,4...15,16,17,17,17,17... */
  }
  for (i=0, j=0; i <= 50; i++) {
    CAB(lzx_position_base)[i] = j; /* 0,1,2,3,4,6,8,12,16,24,32,... */
    j += 1 << CAB(extra_bits)[i]; /* 1,1,1,1,2,2,4,4,8,8,16,16,32,32,... */
  }

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

  /* initialise tables to 0 (because deltas will be applied to them) */
  for (i = 0; i < LZX_MAINTREE_MAXSYMBOLS; i++) LZX(MAINTREE_len)[i] = 0;
  for (i = 0; i < LZX_LENGTH_MAXSYMBOLS; i++)   LZX(LENGTH_len)[i]   = 0;

  return DECR_OK;
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
int make_decode_table(cab_ULONG nsyms, cab_ULONG nbits, cab_UBYTE *length, cab_UWORD *table) {
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

/************************************************************
 * lzx_read_lens (internal)
 */
int lzx_read_lens(cab_UBYTE *lens, cab_ULONG first, cab_ULONG last, struct lzx_bits *lb,
                  cab_decomp_state *decomp_state) {
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
 * LZXdecompress (internal)
 */
int LZXdecompress(int inlen, int outlen, cab_decomp_state *decomp_state) {
  cab_UBYTE *inpos  = CAB(inbuf);
  cab_UBYTE *endinp = inpos + inlen;
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
        READ_LENGTHS(MAINTREE, 0, 256, lzx_read_lens);
        READ_LENGTHS(MAINTREE, 256, LZX(main_elements), lzx_read_lens);
        BUILD_TABLE(MAINTREE);
        if (LENTABLE(MAINTREE)[0xE8] != 0) LZX(intel_started) = 1;

        READ_LENGTHS(LENGTH, 0, LZX_NUM_SECONDARY_LENGTHS, lzx_read_lens);
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

/*********************************************************
 * find_cabs_in_file (internal)
 */
struct cabinet *find_cabs_in_file(LPCSTR name, cab_UBYTE search_buf[])
{
  struct cabinet *cab, *cab2, *firstcab = NULL, *linkcab = NULL;
  cab_UBYTE *pstart = &search_buf[0], *pend, *p;
  cab_off_t offset, caboff, cablen = 0, foffset = 0, filelen, length;
  int state = 0, found = 0, ok = 0;

  TRACE("(name == %s)\n", debugstr_a((char *) name));

  /* open the file and search for cabinet headers */
  if ((cab = (struct cabinet *) calloc(1, sizeof(struct cabinet)))) {
    cab->filename = name;
    if (cabinet_open(cab)) {
      filelen = cab->filelen;
      for (offset = 0; (offset < filelen); offset += length) {
	/* search length is either the full length of the search buffer,
	 * or the amount of data remaining to the end of the file,
	 * whichever is less.
	 */
	length = filelen - offset;
	if (length > CAB_SEARCH_SIZE) length = CAB_SEARCH_SIZE;

	/* fill the search buffer with data from disk */
	if (!cabinet_read(cab, search_buf, length)) break;

	/* read through the entire buffer. */
	p = pstart;
	pend = &search_buf[length];
	while (p < pend) {
	  switch (state) {
	  /* starting state */
	  case 0:
	    /* we spend most of our time in this while loop, looking for
	     * a leading 'M' of the 'MSCF' signature
	     */
	    while (*p++ != 0x4D && p < pend);
	    if (p < pend) state = 1; /* if we found tht 'M', advance state */
	    break;

	  /* verify that the next 3 bytes are 'S', 'C' and 'F' */
	  case 1: state = (*p++ == 0x53) ? 2 : 0; break;
	  case 2: state = (*p++ == 0x43) ? 3 : 0; break;
	  case 3: state = (*p++ == 0x46) ? 4 : 0; break;

	  /* we don't care about bytes 4-7 */
	  /* bytes 8-11 are the overall length of the cabinet */
	  case 8:  cablen  = *p++;       state++; break;
	  case 9:  cablen |= *p++ << 8;  state++; break;
	  case 10: cablen |= *p++ << 16; state++; break;
	  case 11: cablen |= *p++ << 24; state++; break;

	  /* we don't care about bytes 12-15 */
	  /* bytes 16-19 are the offset within the cabinet of the filedata */
	  case 16: foffset  = *p++;       state++; break;
	  case 17: foffset |= *p++ << 8;  state++; break;
	  case 18: foffset |= *p++ << 16; state++; break;
	  case 19: foffset |= *p++ << 24;
	    /* now we have received 20 bytes of potential cab header. */
	    /* work out the offset in the file of this potential cabinet */
	    caboff = offset + (p-pstart) - 20;

	    /* check that the files offset is less than the alleged length
	     * of the cabinet, and that the offset + the alleged length are
	     * 'roughly' within the end of overall file length
	     */
	    if ((foffset < cablen) &&
		((caboff + foffset) < (filelen + 32)) &&
		((caboff + cablen) < (filelen + 32)) )
	    {
	      /* found a potential result - try loading it */
	      found++;
	      cab2 =  load_cab_offset(name, caboff);
	      if (cab2) {
		/* success */
		ok++;

		/* cause the search to restart after this cab's data. */
		offset = caboff + cablen;
		if (offset < cab->filelen) cabinet_seek(cab, offset);
		length = 0;
		p = pend;

		/* link the cab into the list */
		if (linkcab == NULL) firstcab = cab2;
		else linkcab->next = cab2;
		linkcab = cab2;
	      }
	    }
	    state = 0;
	    break;
	  default:
	    p++, state++; break;
	  }
	}
      }
      cabinet_close(cab);
    }
    free(cab);
  }

  /* if there were cabinets that were found but are not ok, point this out */
  if (found > ok) {
    WARN("%s: found %d bad cabinets\n", debugstr_a(name), found-ok);
  }

  /* if no cabinets were found, let the user know */
  if (!firstcab) {
    WARN("%s: not a Microsoft cabinet file.\n", debugstr_a(name));
  }
  return firstcab;
}

/***********************************************************************
 * find_cabinet_file (internal)
 *
 * tries to find *cabname, from the directory path of origcab, correcting the
 * case of *cabname if necessary, If found, writes back to *cabname.
 */
void find_cabinet_file(char **cabname, LPCSTR origcab) {

  char *tail, *cab, *name, *nextpart, nametmp[MAX_PATH], *filepart;
  int found = 0;

  TRACE("(*cabname == ^%p, origcab == %s)\n", cabname ? *cabname : NULL, debugstr_a(origcab));

  /* ensure we have a cabinet name at all */
  if (!(name = *cabname)) {
    WARN("no cabinet name at all\n");
  }

  /* find if there's a directory path in the origcab */
  tail = origcab ? max(strrchr(origcab, '/'), strrchr(origcab, '\\')) : NULL;

  if ((cab = (char *) malloc(MAX_PATH))) {
    /* add the directory path from the original cabinet name */
    if (tail) {
      memcpy(cab, origcab, tail - origcab);
      cab[tail - origcab] = '\0';
    } else {
      /* default directory path of '.' */
      cab[0] = '.';
      cab[1] = '\0';
    }

    do {
      TRACE("trying cab == %s\n", debugstr_a(cab));

      /* we don't want null cabinet filenames */
      if (name[0] == '\0') {
        WARN("null cab name\n");
        break;
      }

      /* if there is a directory component in the cabinet name,
       * look for that alone first
       */
      nextpart = strchr(name, '\\');
      if (nextpart) *nextpart = '\0';

      found = SearchPathA(cab, name, NULL, MAX_PATH, nametmp, &filepart);

      /* if the component was not found, look for it in the current dir */
      if (!found) {
        found = SearchPathA(".", name, NULL, MAX_PATH, nametmp, &filepart);
      }
      
      if (found) 
        TRACE("found: %s\n", debugstr_a(nametmp));
      else
        TRACE("not found.\n");

      /* restore the real name and skip to the next directory component
       * or actual cabinet name
       */
      if (nextpart) *nextpart = '\\', name = &nextpart[1];

      /* while there is another directory component, and while we
       * successfully found the current component
       */
    } while (nextpart && found);

    /* if we found the cabinet, change the next cabinet's name.
     * otherwise, pretend nothing happened
     */
    if (found) {
      free((void *) *cabname);
      *cabname = cab;
      strncpy(cab, nametmp, found+1);
      TRACE("result: %s\n", debugstr_a(cab));
    } else {
      free((void *) cab);
      TRACE("result: nothing\n");
    }
  }
}

/************************************************************************
 * process_files (internal)
 *
 * this does the tricky job of running through every file in the cabinet,
 * including spanning cabinets, and working out which file is in which
 * folder in which cabinet. It also throws out the duplicate file entries
 * that appear in spanning cabinets. There is memory leakage here because
 * those entries are not freed. See the XAD CAB client (function CAB_GetInfo
 * in CAB.c) for an implementation of this that correctly frees the discarded
 * file entries.
 */
struct cab_file *process_files(struct cabinet *basecab) {
  struct cabinet *cab;
  struct cab_file *outfi = NULL, *linkfi = NULL, *nextfi, *fi, *cfi;
  struct cab_folder *fol, *firstfol, *lastfol = NULL, *predfol;
  int i, mergeok;

  FIXME("(basecab == ^%p): Memory leak.\n", basecab);

  for (cab = basecab; cab; cab = cab->nextcab) {
    /* firstfol = first folder in this cabinet */
    /* lastfol  = last folder in this cabinet */
    /* predfol  = last folder in previous cabinet (or NULL if first cabinet) */
    predfol = lastfol;
    firstfol = cab->folders;
    for (lastfol = firstfol; lastfol->next;) lastfol = lastfol->next;
    mergeok = 1;

    for (fi = cab->files; fi; fi = nextfi) {
      i = fi->index;
      nextfi = fi->next;

      if (i < cffileCONTINUED_FROM_PREV) {
        for (fol = firstfol; fol && i--; ) fol = fol->next;
        fi->folder = fol; /* NULL if an invalid folder index */
      }
      else {
        /* folder merging */
        if (i == cffileCONTINUED_TO_NEXT
        ||  i == cffileCONTINUED_PREV_AND_NEXT) {
          if (cab->nextcab && !lastfol->contfile) lastfol->contfile = fi;
        }

        if (i == cffileCONTINUED_FROM_PREV
        ||  i == cffileCONTINUED_PREV_AND_NEXT) {
          /* these files are to be continued in yet another
           * cabinet, don't merge them in just yet */
          if (i == cffileCONTINUED_PREV_AND_NEXT) mergeok = 0;

          /* only merge once per cabinet */
          if (predfol) {
            if ((cfi = predfol->contfile)
            && (cfi->offset == fi->offset)
            && (cfi->length == fi->length)
            && (strcmp(cfi->filename, fi->filename) == 0)
            && (predfol->comp_type == firstfol->comp_type)) {
              /* increase the number of splits */
              if ((i = ++(predfol->num_splits)) > CAB_SPLITMAX) {
                mergeok = 0;
                ERR("%s: internal error: CAB_SPLITMAX exceeded. please report this to wine-devel@winehq.org)\n",
		    debugstr_a(basecab->filename));
              }
              else {
                /* copy information across from the merged folder */
                predfol->offset[i] = firstfol->offset[0];
                predfol->cab[i]    = firstfol->cab[0];
                predfol->next      = firstfol->next;
                predfol->contfile  = firstfol->contfile;

                if (firstfol == lastfol) lastfol = predfol;
                firstfol = predfol;
                predfol = NULL; /* don't merge again within this cabinet */
              }
            }
            else {
              /* if the folders won't merge, don't add their files */
              mergeok = 0;
            }
          }

          if (mergeok) fi->folder = firstfol;
        }
      }

      if (fi->folder) {
        if (linkfi) linkfi->next = fi; else outfi = fi;
        linkfi = fi;
      }
    } /* for (fi= .. */
  } /* for (cab= ...*/

  return outfi;
}

/****************************************************************
 * convertUTF (internal)
 *
 * translate UTF -> ASCII
 *
 * UTF translates two-byte unicode characters into 1, 2 or 3 bytes.
 * %000000000xxxxxxx -> %0xxxxxxx
 * %00000xxxxxyyyyyy -> %110xxxxx %10yyyyyy
 * %xxxxyyyyyyzzzzzz -> %1110xxxx %10yyyyyy %10zzzzzz
 *
 * Therefore, the inverse is as follows:
 * First char:
 *  0x00 - 0x7F = one byte char
 *  0x80 - 0xBF = invalid
 *  0xC0 - 0xDF = 2 byte char (next char only 0x80-0xBF is valid)
 *  0xE0 - 0xEF = 3 byte char (next 2 chars only 0x80-0xBF is valid)
 *  0xF0 - 0xFF = invalid
 * 
 * FIXME: use a winapi to do this
 */
int convertUTF(cab_UBYTE *in) {
  cab_UBYTE c, *out = in, *end = in + strlen((char *) in) + 1;
  cab_ULONG x;

  do {
    /* read unicode character */
    if ((c = *in++) < 0x80) x = c;
    else {
      if (c < 0xC0) return 0;
      else if (c < 0xE0) {
        x = (c & 0x1F) << 6;
        if ((c = *in++) < 0x80 || c > 0xBF) return 0; else x |= (c & 0x3F);
      }
      else if (c < 0xF0) {
        x = (c & 0xF) << 12;
        if ((c = *in++) < 0x80 || c > 0xBF) return 0; else x |= (c & 0x3F)<<6;
        if ((c = *in++) < 0x80 || c > 0xBF) return 0; else x |= (c & 0x3F);
      }
      else return 0;
    }

    /* terrible unicode -> ASCII conversion */
    if (x > 127) x = '_';

    if (in > end) return 0; /* just in case */
  } while ((*out++ = (cab_UBYTE) x));
  return 1;
}

/****************************************************
 * NONEdecompress (internal)
 */
int NONEdecompress(int inlen, int outlen, cab_decomp_state *decomp_state)
{
  if (inlen != outlen) return DECR_ILLEGALDATA;
  memcpy(CAB(outbuf), CAB(inbuf), (size_t) inlen);
  return DECR_OK;
}

/**************************************************
 * checksum (internal)
 */
cab_ULONG checksum(cab_UBYTE *data, cab_UWORD bytes, cab_ULONG csum) {
  int len;
  cab_ULONG ul = 0;

  for (len = bytes >> 2; len--; data += 4) {
    csum ^= ((data[0]) | (data[1]<<8) | (data[2]<<16) | (data[3]<<24));
  }

  switch (bytes & 3) {
  case 3: ul |= *data++ << 16;
  case 2: ul |= *data++ <<  8;
  case 1: ul |= *data;
  }
  csum ^= ul;

  return csum;
}

/**********************************************************
 * decompress (internal)
 */
int decompress(struct cab_file *fi, int savemode, int fix, cab_decomp_state *decomp_state)
{
  cab_ULONG bytes = savemode ? fi->length : fi->offset - CAB(offset);
  struct cabinet *cab = CAB(current)->cab[CAB(split)];
  cab_UBYTE buf[cfdata_SIZEOF], *data;
  cab_UWORD inlen, len, outlen, cando;
  cab_ULONG cksum;
  cab_LONG err;

  TRACE("(fi == ^%p, savemode == %d, fix == %d)\n", fi, savemode, fix);

  while (bytes > 0) {
    /* cando = the max number of bytes we can do */
    cando = CAB(outlen);
    if (cando > bytes) cando = bytes;

    /* if cando != 0 */
    if (cando && savemode)
      file_write(fi, CAB(outpos), cando);

    CAB(outpos) += cando;
    CAB(outlen) -= cando;
    bytes -= cando; if (!bytes) break;

    /* we only get here if we emptied the output buffer */

    /* read data header + data */
    inlen = outlen = 0;
    while (outlen == 0) {
      /* read the block header, skip the reserved part */
      if (!cabinet_read(cab, buf, cfdata_SIZEOF)) return DECR_INPUT;
      cabinet_skip(cab, cab->block_resv);

      /* we shouldn't get blocks over CAB_INPUTMAX in size */
      data = CAB(inbuf) + inlen;
      len = EndGetI16(buf+cfdata_CompressedSize);
      inlen += len;
      if (inlen > CAB_INPUTMAX) return DECR_INPUT;
      if (!cabinet_read(cab, data, len)) return DECR_INPUT;

      /* clear two bytes after read-in data */
      data[len+1] = data[len+2] = 0;

      /* perform checksum test on the block (if one is stored) */
      cksum = EndGetI32(buf+cfdata_CheckSum);
      if (cksum && cksum != checksum(buf+4, 4, checksum(data, len, 0))) {
	/* checksum is wrong */
	if (fix && ((fi->folder->comp_type & cffoldCOMPTYPE_MASK)
		    == cffoldCOMPTYPE_MSZIP))
        {
	  WARN("%s: checksum failed\n", debugstr_a(fi->filename)); 
	}
	else {
	  return DECR_CHECKSUM;
	}
      }

      /* outlen=0 means this block was part of a split block */
      outlen = EndGetI16(buf+cfdata_UncompressedSize);
      if (outlen == 0) {
        cabinet_close(cab);
        cab = CAB(current)->cab[++CAB(split)];
        if (!cabinet_open(cab)) return DECR_INPUT;
        cabinet_seek(cab, CAB(current)->offset[CAB(split)]);
      }
    }

    /* decompress block */
    if ((err = CAB(decompress)(inlen, outlen, decomp_state))) {
      if (fix && ((fi->folder->comp_type & cffoldCOMPTYPE_MASK)
		  == cffoldCOMPTYPE_MSZIP))
      {
	ERR("%s: failed decrunching block\n", debugstr_a(fi->filename)); 
      }
      else {
	return err;
      }
    }
    CAB(outlen) = outlen;
    CAB(outpos) = CAB(outbuf);
  }

  return DECR_OK;
}

/****************************************************************
 * extract_file (internal)
 *
 * workhorse to extract a particular file from a cab
 */
void extract_file(struct cab_file *fi, int lower, int fix, LPCSTR dir, cab_decomp_state *decomp_state)
{
  struct cab_folder *fol = fi->folder, *oldfol = CAB(current);
  cab_LONG err = DECR_OK;

  TRACE("(fi == ^%p, lower == %d, fix == %d, dir == %s)\n", fi, lower, fix, debugstr_a(dir));

  /* is a change of folder needed? do we need to reset the current folder? */
  if (fol != oldfol || fi->offset < CAB(offset)) {
    cab_UWORD comptype = fol->comp_type;
    int ct1 = comptype & cffoldCOMPTYPE_MASK;
    int ct2 = oldfol ? (oldfol->comp_type & cffoldCOMPTYPE_MASK) : 0;

    /* if the archiver has changed, call the old archiver's free() function */
    if (ct1 != ct2) {
      switch (ct2) {
      case cffoldCOMPTYPE_LZX:
        if (LZX(window)) {
	  free(LZX(window));
	  LZX(window) = NULL;
	}
	break;
      case cffoldCOMPTYPE_QUANTUM:
	if (QTM(window)) {
	  free(QTM(window));
	  QTM(window) = NULL;
	}
	break;
      }
    }

    switch (ct1) {
    case cffoldCOMPTYPE_NONE:
      CAB(decompress) = NONEdecompress;
      break;

    case cffoldCOMPTYPE_MSZIP:
      CAB(decompress) = ZIPdecompress;
      break;

    case cffoldCOMPTYPE_QUANTUM:
      CAB(decompress) = QTMdecompress;
      err = QTMinit((comptype >> 8) & 0x1f, (comptype >> 4) & 0xF, decomp_state);
      break;

    case cffoldCOMPTYPE_LZX:
      CAB(decompress) = LZXdecompress;
      err = LZXinit((comptype >> 8) & 0x1f, decomp_state);
      break;

    default:
      err = DECR_DATAFORMAT;
    }
    if (err) goto exit_handler;

    /* initialisation OK, set current folder and reset offset */
    if (oldfol) cabinet_close(oldfol->cab[CAB(split)]);
    if (!cabinet_open(fol->cab[0])) goto exit_handler;
    cabinet_seek(fol->cab[0], fol->offset[0]);
    CAB(current) = fol;
    CAB(offset) = 0;
    CAB(outlen) = 0; /* discard existing block */
    CAB(split)  = 0;
  }

  if (fi->offset > CAB(offset)) {
    /* decode bytes and send them to /dev/null */
    if ((err = decompress(fi, 0, fix, decomp_state))) goto exit_handler;
    CAB(offset) = fi->offset;
  }
  
  if (!file_open(fi, lower, dir)) return;
  err = decompress(fi, 1, fix, decomp_state);
  if (err) CAB(current) = NULL; else CAB(offset) += fi->length;
  file_close(fi);

exit_handler:
  if (err) {
    const char *errmsg;
    char *cabname;
    switch (err) {
    case DECR_NOMEMORY:
      errmsg = "out of memory!\n"; break;
    case DECR_ILLEGALDATA:
      errmsg = "%s: illegal or corrupt data\n"; break;
    case DECR_DATAFORMAT:
      errmsg = "%s: unsupported data format\n"; break;
    case DECR_CHECKSUM:
      errmsg = "%s: checksum error\n"; break;
    case DECR_INPUT:
      errmsg = "%s: input error\n"; break;
    case DECR_OUTPUT:
      errmsg = "%s: output error\n"; break;
    default:
      errmsg = "%s: unknown error (BUG)\n";
    }

    if (CAB(current)) {
      cabname = (char *) (CAB(current)->cab[CAB(split)]->filename);
    }
    else {
      cabname = (char *) (fi->folder->cab[0]->filename);
    }

    ERR(errmsg, cabname);
  }
}

/*********************************************************
 * print_fileinfo (internal)
 */
void print_fileinfo(struct cab_file *fi) {
  int d = fi->date, t = fi->time;
  char *fname = NULL;

  if (fi->attribs & cffile_A_NAME_IS_UTF) {
    fname = malloc(strlen(fi->filename) + 1);
    if (fname) {
      strcpy(fname, fi->filename);
      convertUTF((cab_UBYTE *) fname);
    }
  }

  TRACE("%9u | %02d.%02d.%04d %02d:%02d:%02d | %s\n",
    fi->length, 
    d & 0x1f, (d>>5) & 0xf, (d>>9) + 1980,
    t >> 11, (t>>5) & 0x3f, (t << 1) & 0x3e,
    fname ? fname : fi->filename
  );

  if (fname) free(fname);
}

/****************************************************************************
 * process_cabinet (internal) 
 *
 * called to simply "extract" a cabinet file.  Will find every cabinet file
 * in that file, search for every chained cabinet attached to those cabinets,
 * and will either extract the cabinets, or ? (call a callback?)
 *
 * PARAMS
 *   cabname [I] name of the cabinet file to extract
 *   dir     [I] directory to extract to
 *   fix     [I] attempt to process broken cabinets
 *   lower   [I] ? (lower case something or other?)
 *   dest    [O] 
 *
 * RETURNS
 *   Success: TRUE
 *   Failure: FALSE
 */
BOOL process_cabinet(LPCSTR cabname, LPCSTR dir, BOOL fix, BOOL lower, EXTRACTdest *dest)
{
  struct cabinet *basecab, *cab, *cab1, *cab2;
  struct cab_file *filelist, *fi;
  struct ExtractFileList **destlistptr = &(dest->filelist);

  /* The first result of a search will be returned, and
   * the remaining results will be chained to it via the cab->next structure
   * member.
   */
  cab_UBYTE search_buf[CAB_SEARCH_SIZE];

  cab_decomp_state decomp_state_local;
  cab_decomp_state *decomp_state = &decomp_state_local;

  /* has the list-mode header been seen before? */
  int viewhdr = 0;

  ZeroMemory(decomp_state, sizeof(cab_decomp_state));

  TRACE("Extract %s\n", debugstr_a(cabname));

  /* load the file requested */
  basecab = find_cabs_in_file(cabname, search_buf);
  if (!basecab) return FALSE;

  /* iterate over all cabinets found in that file */
  for (cab = basecab; cab; cab=cab->next) {

    /* bi-directionally load any spanning cabinets -- backwards */
    for (cab1 = cab; cab1->flags & cfheadPREV_CABINET; cab1 = cab1->prevcab) {
      TRACE("%s: extends backwards to %s (%s)\n", debugstr_a(cabname),
            debugstr_a(cab1->prevname), debugstr_a(cab1->previnfo));
      find_cabinet_file(&(cab1->prevname), cabname);
      if (!(cab1->prevcab = load_cab_offset(cab1->prevname, 0))) {
        ERR("%s: can't read previous cabinet %s\n", debugstr_a(cabname), debugstr_a(cab1->prevname));
        break;
      }
      cab1->prevcab->nextcab = cab1;
    }

    /* bi-directionally load any spanning cabinets -- forwards */
    for (cab2 = cab; cab2->flags & cfheadNEXT_CABINET; cab2 = cab2->nextcab) {
      TRACE("%s: extends to %s (%s)\n", debugstr_a(cabname),
            debugstr_a(cab2->nextname), debugstr_a(cab2->nextinfo));
      find_cabinet_file(&(cab2->nextname), cabname);
      if (!(cab2->nextcab = load_cab_offset(cab2->nextname, 0))) {
        ERR("%s: can't read next cabinet %s\n", debugstr_a(cabname), debugstr_a(cab2->nextname));
        break;
      }
      cab2->nextcab->prevcab = cab2;
    }

    filelist = process_files(cab1);
    CAB(current) = NULL;

    if (!viewhdr) {
      TRACE("File size | Date       Time     | Name\n");
      TRACE("----------+---------------------+-------------\n");
      viewhdr = 1;
    }
    for (fi = filelist; fi; fi = fi->next) {
	print_fileinfo(fi);
        dest->filecount++;
    }
    TRACE("Beginning Extraction...\n");
    for (fi = filelist; fi; fi = fi->next) {
	TRACE("  extracting: %s\n", debugstr_a(fi->filename));
	extract_file(fi, lower, fix, dir, decomp_state);
        sprintf(dest->lastfile, "%s%s%s",
                strlen(dest->directory) ? dest->directory : "",
                strlen(dest->directory) ? "\\": "",
                fi->filename);
	*destlistptr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				sizeof(struct ExtractFileList));
	if(*destlistptr) {
	   (*destlistptr)->unknown = TRUE; /* FIXME: were do we get the value? */
	   (*destlistptr)->filename = HeapAlloc(GetProcessHeap(), 0, (
						strlen(fi->filename)+1));
	   if((*destlistptr)->filename) 
		lstrcpyA((*destlistptr)->filename, fi->filename);
	   destlistptr = &((*destlistptr)->next);
	}
    }
  }

  TRACE("Finished processing cabinet.\n");

  return TRUE;
}
