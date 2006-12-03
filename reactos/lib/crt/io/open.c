/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/io/open.c
 * PURPOSE:     Opens a file and translates handles to fileno
 * PROGRAMER:   Ariadne
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
/*
 * Some stuff taken from active perl: perl\win32.c (ioinfo stuff)
 *
 * (c) 1995 Microsoft Corporation. All rights reserved.
 *       Developed by hip communications inc., http://info.hip.com/info/
 * Portions (c) 1993 Intergraph Corporation. All rights reserved.
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 */
/*
 * Some functions taken from/based on wine\dlls\msvcrt\file.c:
 *  split_oflags
 *  _open_osfhandle
 *  many more...
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 * Copyright 2004 Eric Pouech
 * Copyright 2004 Juan Lang
 */

// rember to interlock the allocation of fileno when making this thread safe

// possibly store extra information at the handle

#include <precomp.h>

#if !defined(NDEBUG) && defined(DBG)
#include <stdarg.h>
#endif

#include <sys/stat.h>
#include <string.h>
#include <share.h>

#define NDEBUG
#include <internal/debug.h>



FDINFO first_bucket[FDINFO_ENTRIES_PER_BUCKET];
FDINFO* __pioinfo[FDINFO_BUCKETS] = {first_bucket};


/* This critical section protects the tables MSVCRT_fdesc and MSVCRT_fstreams,
 * and their related indexes, MSVCRT_fdstart, MSVCRT_fdend,
 * and MSVCRT_stream_idx, from race conditions.
 * It doesn't protect against race conditions manipulating the underlying files
 * or flags; doing so would probably be better accomplished with per-file
 * protection, rather than locking the whole table for every change.
 */
static CRITICAL_SECTION g_file_cs;
#define LOCK_FILES()    do { EnterCriticalSection(&g_file_cs); } while (0)
#define UNLOCK_FILES()  do { LeaveCriticalSection(&g_file_cs); } while (0)

/////////////////////////////////////////

static int g_fdstart = 3; /* first unallocated fd */
static int g_fdend = 3; /* highest allocated fd */

/*
 * INTERNAL
 */
 /*
static __inline FD_INFO* fdinfo(int fd)
{
   FD_INFO* bucket = __pioinfo[fd >> FDINFO_ENTRIES_PER_BUCKET_SHIFT];
   if (!bucket){
      bucket = alloc_init_bucket(fd);
   }
   return bucket + (fd & (FDINFO_ENTRIES_PER_BUCKET - 1));
}
*/


/*
 * INTERNAL
 */
__inline BOOL is_valid_fd(int fd)
{
   BOOL b = (fd >= 0 && fd < g_fdend && (fdinfo(fd)->fdflags & FOPEN));

   if (!b){
      if (fd >= 0 && fd < g_fdend)
      {
         DPRINT1("not valid fd %i, g_fdend %i, fdinfo %x, bucket %x, fdflags %x\n",
                 fd,g_fdend,fdinfo(fd),fdinfo_bucket(fd),fdinfo(fd)->fdflags);
      }
      else
      {
         DPRINT1("not valid fd %i, g_fdend %i\n",fd,g_fdend);
      }

   }

   return b;
}

/*
 * INTERNAL
 */
char split_oflags(int oflags)
{
    char         fdflags = 0;

    if (oflags & _O_APPEND)              fdflags |= FAPPEND;

    if (oflags & _O_BINARY)              ;
    else if (oflags & _O_TEXT)           fdflags |= FTEXT;
    else if (_fmode& _O_BINARY)  ;
    else                                        fdflags |= FTEXT; /* default to TEXT*/

    if (oflags & _O_NOINHERIT)           fdflags |= FNOINHERIT;

    if (oflags & ~(_O_BINARY|_O_TEXT|_O_APPEND|_O_TRUNC|
                   _O_EXCL|_O_CREAT|_O_RDWR|_O_WRONLY|
                   _O_TEMPORARY|_O_NOINHERIT))
        DPRINT1(":unsupported oflags 0x%04x\n",oflags);

    return fdflags;
}



/*
 * INTERNAL
 */
char __is_text_file(FILE* p)
{
   if ( p == NULL || fdinfo_bucket((p)->_file) == NULL )
     return FALSE;
   return (!((p)->_flag&_IOSTRG) && (fdinfo((p)->_file)->fdflags & FTEXT));
}

/*
 * @implemented
 */
int _open(const char* _path, int _oflag,...)
{
#if !defined(NDEBUG) && defined(DBG)
   va_list arg;
   int pmode;
#endif
   HANDLE hFile;
   DWORD dwDesiredAccess = 0;
   DWORD dwShareMode = 0;
   DWORD dwCreationDistribution = 0;
   DWORD dwFlagsAndAttributes = 0;
   SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

#if !defined(NDEBUG) && defined(DBG)
   va_start(arg, _oflag);
   pmode = va_arg(arg, int);
#endif


   TRACE("_open('%s', %x, (%x))\n", _path, _oflag);


   if ((_oflag & S_IREAD ) == S_IREAD)
     dwShareMode = FILE_SHARE_READ;
   else if ((_oflag & S_IWRITE) == S_IWRITE) {
      dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
   }
   /*
    *
    * _O_BINARY   Opens file in binary (untranslated) mode. (See fopen for a description of binary mode.)
    * _O_TEXT   Opens file in text (translated) mode. (For more information, see Text and Binary Mode File I/O and fopen.)
    *
    * _O_APPEND   Moves file pointer to end of file before every write operation.
    */
#ifdef _OLD_BUILD_
   if ((_oflag & _O_RDWR) == _O_RDWR)
     dwDesiredAccess |= GENERIC_WRITE|GENERIC_READ;
   else if ((_oflag & O_RDONLY) == O_RDONLY)
     dwDesiredAccess |= GENERIC_READ;
   else if ((_oflag & _O_WRONLY) == _O_WRONLY)
     dwDesiredAccess |= GENERIC_WRITE ;
#else
   if ((_oflag & _O_WRONLY) == _O_WRONLY )
     dwDesiredAccess |= GENERIC_WRITE ;
   else if ((_oflag & _O_RDWR) == _O_RDWR )
     dwDesiredAccess |= GENERIC_WRITE|GENERIC_READ;
   else //if ((_oflag & O_RDONLY) == O_RDONLY)
     dwDesiredAccess |= GENERIC_READ;
#endif

   if (( _oflag & (_O_CREAT | _O_EXCL)) == (_O_CREAT | _O_EXCL))
     dwCreationDistribution |= CREATE_NEW;

   else if ((_oflag &  O_TRUNC ) == O_TRUNC) {
      if ((_oflag &  O_CREAT ) ==  O_CREAT)
    dwCreationDistribution |= CREATE_ALWAYS;
      else if ((_oflag & O_RDONLY ) != O_RDONLY)
    dwCreationDistribution |= TRUNCATE_EXISTING;
   }
   else if ((_oflag & _O_APPEND) == _O_APPEND)
     dwCreationDistribution |= OPEN_EXISTING;
   else if ((_oflag &  _O_CREAT) == _O_CREAT)
     dwCreationDistribution |= OPEN_ALWAYS;
   else
     dwCreationDistribution |= OPEN_EXISTING;

   if ((_oflag &  _O_RANDOM) == _O_RANDOM )
     dwFlagsAndAttributes |= FILE_FLAG_RANDOM_ACCESS;
   if ((_oflag &  _O_SEQUENTIAL) == _O_SEQUENTIAL)
     dwFlagsAndAttributes |= FILE_FLAG_SEQUENTIAL_SCAN;
   if ((_oflag &  _O_TEMPORARY) == _O_TEMPORARY) {
     dwFlagsAndAttributes |= FILE_FLAG_DELETE_ON_CLOSE;
     DPRINT("FILE_FLAG_DELETE_ON_CLOSE\n");
   }
   if ((_oflag &  _O_SHORT_LIVED) == _O_SHORT_LIVED) {
     dwFlagsAndAttributes |= FILE_FLAG_DELETE_ON_CLOSE;
     DPRINT("FILE_FLAG_DELETE_ON_CLOSE\n");
   }
   if (_oflag & _O_NOINHERIT)
     sa.bInheritHandle = FALSE;

   if (dwCreationDistribution == OPEN_EXISTING &&
       (dwDesiredAccess & (GENERIC_WRITE|GENERIC_READ)) == GENERIC_READ) {
      /* Allow always shared read for a file which is opened for read only */
      dwShareMode |= FILE_SHARE_READ;
   }

   hFile = CreateFileA(_path,
               dwDesiredAccess,
               dwShareMode,
               &sa,
               dwCreationDistribution,
               dwFlagsAndAttributes,
               NULL);
	if (hFile == (HANDLE)-1) {
		_dosmaperr(GetLastError());
      return( -1);
	}
   DPRINT("OK\n");
   if (!(_oflag & (_O_TEXT|_O_BINARY))) {
       _oflag |= _fmode;
   }
   return(alloc_fd(hFile, split_oflags(_oflag)));
}



/*
 * INTERNAL
 */
static void init_bucket(FDINFO* entry)
{
   int i;

   for(i=0;
       i < FDINFO_ENTRIES_PER_BUCKET;
       i++, entry++)
   {
      entry->hFile = INVALID_HANDLE_VALUE;
      entry->fdflags = 0;
      entry->pipechar = LF;
      entry->lockinitflag = 0;
   }
}

/*
 * INTERNAL
 */
static BOOL alloc_init_bucket(int fd)
{
   fdinfo_bucket(fd) = malloc(FDINFO_ENTRIES_PER_BUCKET * sizeof(FDINFO));
   if (!fdinfo_bucket(fd)) return FALSE;

   init_bucket(fdinfo_bucket(fd));

   return TRUE;
}




/*
 * INTERNAL
 *  Allocate an fd slot from a Win32 HANDLE, starting from fd
 *  caller must hold the files lock
 */
static int alloc_fd_from(HANDLE hand, char flag, int fd)
{

   if (fd >= FDINFO_ENTRIES)
   {
      DPRINT1("files exhausted!\n");
      return -1;
   }

   if (!fdinfo_bucket(fd))
   {
      if (!alloc_init_bucket(fd)){
         //errno = ENOMEM
         return -1;
      }
   }

   fdinfo(fd)->hFile = hand;
   fdinfo(fd)->fdflags = FOPEN | (flag & (FNOINHERIT | FAPPEND | FTEXT));
   fdinfo(fd)->pipechar = LF;
   fdinfo(fd)->lockinitflag = 0;
   //fdinfo(fd)->lock

   /* locate next free slot */
   if (fd == g_fdstart && fd == g_fdend)
   {
      g_fdstart = g_fdend + 1;
   }
   else
   {
#if 0  /* alternate (untested) impl. maybe a tiny bit faster? -Gunnar */
      int i, bidx;

      for (bidx = fdinfo_bucket_idx(g_fdstart); bidx < FDINFO_BUCKETS && __pioinfo[bidx]; bidx++)
      {
         for (i = fdinfo_bucket_entry_idx(g_fdstart);
              g_fdstart < g_fdend && fdinfo(g_fdstart)->fdflags & FOPEN && i < FDINFO_BUCKET_ENTRIES;
              i++)
         {
            g_fdstart++;
         }
      }
#else

      while (g_fdstart < g_fdend &&
             fdinfo_bucket(g_fdstart) &&
             (fdinfo(g_fdstart)->fdflags & FOPEN))
      {
         g_fdstart++;
      }
#endif
   }

  /* update last fd in use */
   if (fd >= g_fdend)
      g_fdend = fd + 1;

   /* alloc more fdinfo buckets by demand.
    * FIXME: should we dealloc buckets when they become unused also? */
   if (!fdinfo_bucket(g_fdstart) && g_fdstart < FDINFO_ENTRIES)
   {
      alloc_init_bucket(g_fdstart);
   }

   DPRINT("fdstart is %d, fdend is %d\n", g_fdstart, g_fdend);

   switch (fd)
   {
      case 0: SetStdHandle(STD_INPUT_HANDLE,  hand); break;
      case 1: SetStdHandle(STD_OUTPUT_HANDLE, hand); break;
      case 2: SetStdHandle(STD_ERROR_HANDLE,  hand); break;
   }

   return fd;
}


/*
 * INTERNAL: Allocate an fd slot from a Win32 HANDLE
 */
int alloc_fd(HANDLE hand, char flag)
{
  int ret;

  LOCK_FILES();

//  TRACE(":handle (%p) allocating fd (%d)\n",hand,MSVCRT_fdstart);
  ret = alloc_fd_from(hand, flag, g_fdstart);

  UNLOCK_FILES();
  return ret;
}



/*
 * INTERNAL
 */
char __fileno_getmode(int fd)
{
    if (!is_valid_fd(fd)) {
        __set_errno(EBADF);
        return -1;
    }
    return fdinfo(fd)->fdflags;

}

/*
 * INTERNAL
 */
void free_fd(int fd)
{
   LOCK_FILES();


   fdinfo(fd)->hFile = INVALID_HANDLE_VALUE;
   fdinfo(fd)->fdflags = 0;

   if (fd < 3) /* don't use 0,1,2 for user files */
   {
      switch (fd)
      {
         case 0: SetStdHandle(STD_INPUT_HANDLE,  NULL); break;
         case 1: SetStdHandle(STD_OUTPUT_HANDLE, NULL); break;
         case 2: SetStdHandle(STD_ERROR_HANDLE,  NULL); break;
      }
   }
   else
   {
      if (fd == g_fdend - 1)
         g_fdend--;

      if (fd < g_fdstart)
         g_fdstart = fd;
   }


   UNLOCK_FILES();
}

/*
 * @implemented
 */
int _open_osfhandle(long osfhandle, int oflags)
{
   /*
   PREV:
   The _open_osfhandle() function in MSVCRT is expected to take the absence
   of either _O_TEXT or _O_BINARY to mean _O_BINARY. Currently it defaults to
   _O_TEXT.

   An example of this is MFC's CStdioFile::Open in binary mode - it passes flags
   of 0 when it wants to write a binary file - under WINE we do text mode conversions!

   The attached patch ensures that _O_BINARY is set if neither is set in the passed-in
flags.


   * file, so set the write flag. It also only sets _O_TEXT if it wants
   * text - it never sets _O_BINARY.
    */
   /* FIXME: handle more flags */
/*
  flags |= MSVCRT__IOREAD|MSVCRT__IOWRT;
  if ( !( flags & _O_TEXT ) ) flags |= _O_BINARY;

  fd = msvcrt_alloc_fd((HANDLE)hand,flags);
  TRACE(":handle (%ld) fd (%d) flags 0x%08x\n",hand,fd, flags);
*/
  /* MSVCRT__O_RDONLY (0) always matches, so set the read flag
   * MFC's CStdioFile clears O_RDONLY (0)! if it wants to write to the
   * file, so set the write flag. It also only sets MSVCRT__O_TEXT if it wants
   * text - it never sets MSVCRT__O_BINARY.
   */
  /* FIXME: handle more flags */
  /*
  LAG TEST SOM TESTER UT ALT DETTE flag tingern
  */
  if (!(oflags & (_O_BINARY | _O_TEXT)) && (_fmode & _O_BINARY))
      oflags |= _O_BINARY;
  else
      oflags |= _O_TEXT;

    return alloc_fd((HANDLE)osfhandle, split_oflags(oflags));
}

/*
 * @implemented
 */
long _get_osfhandle(int fd)
{
   TRACE("_get_osfhandle(%i)",fd);

    if (!is_valid_fd(fd)) {
        return( -1 );
    }
    return( (long)fdinfo(fd)->hFile );
}



/*
 * INTERNAL
 */
int __fileno_dup2(int handle1, int handle2)
{
   HANDLE hProcess;
   BOOL result;

   if (handle1 >= FDINFO_ENTRIES || handle1 < 0 || handle2 >= FDINFO_ENTRIES || handle2 < 0) {
      __set_errno(EBADF);
      return -1;
   }
//   if (_pioinfo[handle1]->fd == -1) {
   if (fdinfo(handle1)->hFile == INVALID_HANDLE_VALUE) {
      __set_errno(EBADF);
      return -1;
   }
   if (handle1 == handle2)
      return handle1;
//   if (_pioinfo[handle2]->fd != -1) {
   if (fdinfo(handle2)->hFile != INVALID_HANDLE_VALUE) {
      _close(handle2);
   }
   hProcess = GetCurrentProcess();
   result = DuplicateHandle(hProcess,
                fdinfo(handle1)->hFile,
                hProcess,
                &fdinfo(handle2)->hFile,
                0,
                fdinfo(handle1)->fdflags & FNOINHERIT ? FALSE : TRUE,
                DUPLICATE_SAME_ACCESS);
   if (result) {
//      _pioinfo[handle2]->fd = handle2;
      fdinfo(handle2)->fdflags = fdinfo(handle1)->fdflags;
      switch (handle2) {
      case 0:
         SetStdHandle(STD_INPUT_HANDLE, fdinfo(handle2)->hFile);
         break;
      case 1:
         SetStdHandle(STD_OUTPUT_HANDLE, fdinfo(handle2)->hFile);
         break;
      case 2:
         SetStdHandle(STD_ERROR_HANDLE, fdinfo(handle2)->hFile);
         break;
      }

      return handle1;
   } else {
      __set_errno(EMFILE);  // Is this the correct error no.?
      return -1;
   }
}


void* malloc(size_t sizeObject);



/*
 * INTERNAL
 */
BOOL __fileno_init(void)
{
  STARTUPINFOA  si;
  int           i;

  init_bucket(first_bucket);

  GetStartupInfoA(&si);

   if (si.cbReserved2 != 0 && si.lpReserved2 != NULL)
   {
    char*       fdflags_ptr;
    HANDLE*     handle_ptr;

    g_fdend = *(unsigned*)si.lpReserved2;

    fdflags_ptr= (char*)(si.lpReserved2 + sizeof(unsigned));
    handle_ptr = (HANDLE*)(fdflags_ptr + g_fdend * sizeof(char));

   g_fdend = min(g_fdend, FDINFO_ENTRIES);
   for (i = 0; i < g_fdend; i++)
   {
      if (!fdinfo_bucket(i))
      {
         if (!alloc_init_bucket(i)){
            /* FIXME: free other buckets? */
            return FALSE;
         }
      }

      if ((*fdflags_ptr & FOPEN) && *handle_ptr != INVALID_HANDLE_VALUE)
      {
        fdinfo(i)->fdflags  = *fdflags_ptr;
        fdinfo(i)->hFile = *handle_ptr;
      }
/*
      else
      {
        fdinfo(i)->fdflags  = 0;
        fdinfo(i)->hFile = INVALID_HANDLE_VALUE;
      }
*/
      fdflags_ptr++; handle_ptr++;
    }
    for (g_fdstart = 3; g_fdstart < g_fdend; g_fdstart++)
        if (fdinfo(g_fdstart)->hFile == INVALID_HANDLE_VALUE) break;
   }

   InitializeCriticalSection(&g_file_cs);


   if (fdinfo(0)->hFile == INVALID_HANDLE_VALUE || !(fdinfo(0)->fdflags & FOPEN)) {
      fdinfo(0)->hFile = GetStdHandle(STD_INPUT_HANDLE);
      if (fdinfo(0)->hFile == NULL)
         fdinfo(0)->hFile = INVALID_HANDLE_VALUE;
      fdinfo(0)->fdflags = FOPEN|FTEXT;
   }
   if (fdinfo(1)->hFile == INVALID_HANDLE_VALUE || !(fdinfo(1)->fdflags & FOPEN)) {
      fdinfo(1)->hFile = GetStdHandle(STD_OUTPUT_HANDLE);
      if (fdinfo(1)->hFile == NULL)
         fdinfo(1)->hFile = INVALID_HANDLE_VALUE;
      fdinfo(1)->fdflags = FOPEN|FTEXT;
   }
   if (fdinfo(2)->hFile == INVALID_HANDLE_VALUE || !(fdinfo(2)->fdflags & FOPEN)) {
      fdinfo(2)->hFile = GetStdHandle(STD_ERROR_HANDLE);
      if (fdinfo(2)->hFile == NULL)
         fdinfo(2)->hFile = INVALID_HANDLE_VALUE;
      fdinfo(2)->fdflags = FOPEN|FTEXT;
   }




   for (i = 0; i < 3; i++)
   {
      /* FILE structs for stdin/out/err are static and never deleted */
//      MSVCRT_fstreams[i] = &MSVCRT__iob[i];
   }
//   MSVCRT_stream_idx = 3;

   return TRUE;
}



/* INTERNAL: Create an inheritance data block (for spawned process)
 * The inheritance block is made of:
 *      00      int     nb of file descriptor (NBFD)
 *      04      char    file flags (wxflag): repeated for each fd
 *      4+NBFD  HANDLE  file handle: repeated for each fd
 */
unsigned create_io_inherit_block(STARTUPINFOA* si)
{
  int         fd;
  char*       fdflags_ptr;
  HANDLE*     handle_ptr;

  TRACE("create_io_inherit_block(%x)",si);

  si->cbReserved2 = sizeof(unsigned) + (sizeof(char) + sizeof(HANDLE)) * g_fdend;
  si->lpReserved2 = calloc(si->cbReserved2, 1);
  if (!si->lpReserved2)
  {
    si->cbReserved2 = 0;
    return( FALSE );
  }
  fdflags_ptr = (char*)si->lpReserved2 + sizeof(unsigned);
  handle_ptr = (HANDLE*)(fdflags_ptr + g_fdend * sizeof(char));

  *(unsigned*)si->lpReserved2 = g_fdend;
  for (fd = 0; fd < g_fdend; fd++)
  {
    /* to be inherited, we need it to be open, and that DONTINHERIT isn't set */
    if ((fdinfo(fd)->fdflags & (FOPEN | FNOINHERIT)) == FOPEN)
    {
      *fdflags_ptr = fdinfo(fd)->fdflags;
      *handle_ptr = fdinfo(fd)->hFile;
    }
    else
    {
      *fdflags_ptr = 0;
      *handle_ptr = INVALID_HANDLE_VALUE;
    }
    fdflags_ptr++; handle_ptr++;
  }
  return( TRUE );
}




/*
 * @implemented
 */
int _setmode(int fd, int newmode)
{
   int prevmode;

   TRACE("_setmode(%d, %d)", fd, newmode);

   if (!is_valid_fd(fd))
   {
      DPRINT1("_setmode: inval fd (%d)\n",fd);
      //errno = EBADF;
      return(-1);
   }

   if (newmode & ~(_O_TEXT|_O_BINARY))
   {
      DPRINT1("_setmode: fd (%d) mode (0x%08x) unknown\n",fd,newmode);
      /* FIXME: Should we fail with EINVAL here? */
   }

   prevmode = fdinfo(fd)->fdflags & FTEXT ? _O_TEXT : _O_BINARY;

   if ((newmode & _O_TEXT) == _O_TEXT)
   {
      fdinfo(fd)->fdflags |= FTEXT;
   }
   else
   {
      /* FIXME: If both _O_TEXT and _O_BINARY are set, we get here.
       * Should we fail with EINVAL instead? -Gunnar
       */
      fdinfo(fd)->fdflags &= ~FTEXT;
   }

   return(prevmode);
}

