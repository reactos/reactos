/*
 * fcache.c - File cache ADT module.
 */

/*

   The file cache ADT may be disabled by #defining NOFCACHE.  If NOFCACHE is
#defined, file cache ADT calls are translated into their direct Win32 file
system API equivalents.

*/


/* Headers
 **********/

#include "project.h"
#pragma hdrstop


/* Constants
 ************/

/* last resort default minimum cache size */

#define DEFAULT_MIN_CACHE_SIZE      (32)


/* Types
 ********/

#ifndef NOFCACHE

/* cached file description structure */

typedef struct _icachedfile
{
   /* current position of file pointer in file */

   DWORD dwcbCurFilePosition;

   /* file handle of cached file */

   HANDLE hfile;

   /* file open mode */

   DWORD dwOpenMode;

   /* size of cache in bytes */

   DWORD dwcbCacheSize;

   /* pointer to base of cache */

   PBYTE pbyteCache;

   /* size of default cache in bytes */

   DWORD dwcbDefaultCacheSize;

   /* default cache */

   PBYTE pbyteDefaultCache;

   /* length of file (including data written to cache) */

   DWORD dwcbFileLen;

   /* offset of start of cache in file */

   DWORD dwcbFileOffsetOfCache;

   /* number of valid bytes in cache, starting at beginning of cache */

   DWORD dwcbValid;

   /* number of uncommitted bytes in cache, starting at beginning of cache */

   DWORD dwcbUncommitted;

   /* path of cached file */

   LPTSTR pszPath;
}
ICACHEDFILE;
DECLARE_STANDARD_TYPES(ICACHEDFILE);

#endif   /* NOFCACHE */


/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

PRIVATE_CODE FCRESULT SetUpCachedFile(PCCACHEDFILE, PHCACHEDFILE);

#ifndef NOFCACHE

PRIVATE_CODE void BreakDownCachedFile(PICACHEDFILE);
PRIVATE_CODE void ResetCacheToEmpty(PICACHEDFILE);
PRIVATE_CODE DWORD ReadFromCache(PICACHEDFILE, PVOID, DWORD);
PRIVATE_CODE DWORD GetValidReadData(PICACHEDFILE, PBYTE *);
PRIVATE_CODE BOOL FillCache(PICACHEDFILE, PDWORD);
PRIVATE_CODE DWORD WriteToCache(PICACHEDFILE, PCVOID, DWORD);
PRIVATE_CODE DWORD GetAvailableWriteSpace(PICACHEDFILE, PBYTE *);
PRIVATE_CODE BOOL CommitCache(PICACHEDFILE);

#ifdef VSTF

PRIVATE_CODE BOOL IsValidPCICACHEDFILE(PCICACHEDFILE);

#endif   /* VSTF */

#endif   /* NOFCACHE */

#ifdef VSTF

PRIVATE_CODE BOOL IsValidPCCACHEDFILE(PCCACHEDFILE);

#endif   /* VSTF */


/*
** SetUpCachedFile()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE FCRESULT SetUpCachedFile(PCCACHEDFILE pccf, PHCACHEDFILE phcf)
{
   FCRESULT fcr;
   HANDLE hfNew;

   ASSERT(IS_VALID_STRUCT_PTR(pccf, CCACHEDFILE));
   ASSERT(IS_VALID_WRITE_PTR(phcf, HCACHEDFILE));

   /* Open the file with the requested open and sharing flags. */

   hfNew = CreateFile(pccf->pcszPath, pccf->dwOpenMode, pccf->dwSharingMode,
                      pccf->psa, pccf->dwCreateMode, pccf->dwAttrsAndFlags,
                      pccf->hTemplateFile);

   if (hfNew != INVALID_HANDLE_VALUE)
   {

#ifdef NOFCACHE

      *phcf = hfNew;

      fcr = FCR_SUCCESS;

#else
      PICACHEDFILE picf;

      fcr = FCR_OUT_OF_MEMORY;

      /* Try to allocate a new cached file structure. */

      if (AllocateMemory(sizeof(*picf), &picf))
      {
         DWORD dwcbDefaultCacheSize;

         /* Allocate the default cache for the cached file. */

         if (pccf->dwcbDefaultCacheSize > 0)
            dwcbDefaultCacheSize = pccf->dwcbDefaultCacheSize;
         else
         {
            dwcbDefaultCacheSize = DEFAULT_MIN_CACHE_SIZE;

            WARNING_OUT((TEXT("SetUpCachedFile(): Using minimum cache size of %lu instead of %lu."),
                         dwcbDefaultCacheSize,
                         pccf->dwcbDefaultCacheSize));
         }

         if (AllocateMemory(dwcbDefaultCacheSize, &(picf->pbyteDefaultCache)))
         {
            if (StringCopy(pccf->pcszPath, &(picf->pszPath)))
            {
               DWORD dwcbFileLenHigh;

               picf->dwcbFileLen = GetFileSize(hfNew, &dwcbFileLenHigh);

               if (picf->dwcbFileLen != INVALID_FILE_SIZE && ! dwcbFileLenHigh)
               {
                  /* Success!  Fill in cached file structure fields. */

                  picf->hfile = hfNew;
                  picf->dwcbCurFilePosition = 0;
                  picf->dwcbCacheSize = dwcbDefaultCacheSize;
                  picf->pbyteCache = picf->pbyteDefaultCache;
                  picf->dwcbDefaultCacheSize = dwcbDefaultCacheSize;
                  picf->dwOpenMode = pccf->dwOpenMode;

                  ResetCacheToEmpty(picf);

                  *phcf = (HCACHEDFILE)picf;
                  fcr = FCR_SUCCESS;

                  ASSERT(IS_VALID_HANDLE(*phcf, CACHEDFILE));

                  TRACE_OUT((TEXT("SetUpCachedFile(): Created %lu byte default cache for file %s."),
                             picf->dwcbCacheSize,
                             picf->pszPath));
               }
               else
               {
                  fcr = FCR_OPEN_FAILED;

SETUPCACHEDFILE_BAIL1:
                  FreeMemory(picf->pbyteDefaultCache);
SETUPCACHEDFILE_BAIL2:
                  FreeMemory(picf);
SETUPCACHEDFILE_BAIL3:
                  /*
                   * Failing to close the file properly is not a failure
                   * condition here.
                   */
                  CloseHandle(hfNew);
               }
            }
            else
               goto SETUPCACHEDFILE_BAIL1;
         }
         else
            goto SETUPCACHEDFILE_BAIL2;
      }
      else
         goto SETUPCACHEDFILE_BAIL3;

#endif   /* NOFCACHE */

   }
   else
   {
      switch (GetLastError())
      {
         /* Returned when file opened by local machine. */
         case ERROR_SHARING_VIOLATION:
            fcr = FCR_FILE_LOCKED;
            break;

         default:
            fcr = FCR_OPEN_FAILED;
            break;
      }
   }

   return(fcr);
}


#ifndef NOFCACHE

/*
** BreakDownCachedFile()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void BreakDownCachedFile(PICACHEDFILE picf)
{
   ASSERT(IS_VALID_STRUCT_PTR(picf, CICACHEDFILE));

   /* Are we using the default cache? */

   if (picf->pbyteCache != picf->pbyteDefaultCache)
      /* No.  Free the cache. */
      FreeMemory(picf->pbyteCache);

   /* Free the default cache. */

   FreeMemory(picf->pbyteDefaultCache);

   TRACE_OUT((TEXT("BreakDownCachedFile(): Destroyed cache for file %s."),
              picf->pszPath));

   FreeMemory(picf->pszPath);
   FreeMemory(picf);

   return;
}


/*
** ResetCacheToEmpty()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE void ResetCacheToEmpty(PICACHEDFILE picf)
{
   /*
    * Don't fully validate *picf here since we may be called by
    * SetUpCachedFile() before *picf has been set up.
    */

   ASSERT(IS_VALID_WRITE_PTR(picf, ICACHEDFILE));

   picf->dwcbFileOffsetOfCache = picf->dwcbCurFilePosition;
   picf->dwcbValid = 0;
   picf->dwcbUncommitted = 0;

   return;
}


/*
** ReadFromCache()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE DWORD ReadFromCache(PICACHEDFILE picf, PVOID hpbyteBuffer, DWORD dwcb)
{
   DWORD dwcbRead;
   PBYTE pbyteStart;
   DWORD dwcbValid;

   ASSERT(IS_VALID_STRUCT_PTR(picf, CICACHEDFILE));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(hpbyteBuffer, BYTE, (UINT)dwcb));

   ASSERT(IS_FLAG_SET(picf->dwOpenMode, GENERIC_READ));
   ASSERT(dwcb > 0);

   /* Is there any valid data that can be read from the cache? */

   dwcbValid = GetValidReadData(picf, &pbyteStart);

   if (dwcbValid > 0)
   {
      /* Yes.  Copy it into the buffer. */

      dwcbRead = min(dwcbValid, dwcb);

      CopyMemory(hpbyteBuffer, pbyteStart, dwcbRead);

      picf->dwcbCurFilePosition += dwcbRead;
   }
   else
      dwcbRead = 0;

   return(dwcbRead);
}


/*
** GetValidReadData()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE DWORD GetValidReadData(PICACHEDFILE picf, PBYTE *ppbyteStart)
{
   DWORD dwcbValid;

   ASSERT(IS_VALID_STRUCT_PTR(picf, CICACHEDFILE));
   ASSERT(IS_VALID_WRITE_PTR(ppbyteStart, PBYTE *));

   ASSERT(IS_FLAG_SET(picf->dwOpenMode, GENERIC_READ));

   /* Is there any valid read data in the cache? */

   /* The current file position must be inside the valid data in the cache. */

   /* Watch out for overflow. */

   ASSERT(picf->dwcbFileOffsetOfCache <= DWORD_MAX - picf->dwcbValid);

   if (picf->dwcbCurFilePosition >= picf->dwcbFileOffsetOfCache &&
       picf->dwcbCurFilePosition < picf->dwcbFileOffsetOfCache + picf->dwcbValid)
   {
      DWORD dwcbStartBias;

      /* Yes. */

      dwcbStartBias = picf->dwcbCurFilePosition - picf->dwcbFileOffsetOfCache;

      *ppbyteStart = picf->pbyteCache + dwcbStartBias;

      /* The second clause above protects against underflow here. */

      dwcbValid = picf->dwcbValid - dwcbStartBias;
   }
   else
      /* No. */
      dwcbValid = 0;

   return(dwcbValid);
}


/*
** FillCache()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL FillCache(PICACHEDFILE picf, PDWORD pdwcbNewData)
{
   BOOL bResult = FALSE;

   ASSERT(IS_VALID_STRUCT_PTR(picf, CICACHEDFILE));
   ASSERT(IS_VALID_WRITE_PTR(pdwcbNewData, DWORD));

   ASSERT(IS_FLAG_SET(picf->dwOpenMode, GENERIC_READ));

   if (CommitCache(picf))
   {
      DWORD dwcbOffset;

      ResetCacheToEmpty(picf);

      /* Seek to start position. */

      dwcbOffset = SetFilePointer(picf->hfile, picf->dwcbCurFilePosition, NULL, FILE_BEGIN);

      if (dwcbOffset != INVALID_SEEK_POSITION)
      {
         DWORD dwcbRead;

         ASSERT(dwcbOffset == picf->dwcbCurFilePosition);

         /* Fill cache from file. */

         if (ReadFile(picf->hfile, picf->pbyteCache, picf->dwcbCacheSize, &dwcbRead, NULL))
         {
            picf->dwcbValid = dwcbRead;

            *pdwcbNewData = dwcbRead;
            bResult = TRUE;

            TRACE_OUT((TEXT("FillCache(): Read %lu bytes into cache starting at offset %lu in file %s."),
                       dwcbRead,
                       dwcbOffset,
                       picf->pszPath));
         }
      }
   }

   return(bResult);
}


/*
** WriteToCache()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE DWORD WriteToCache(PICACHEDFILE picf, PCVOID hpbyteBuffer, DWORD dwcb)
{
   DWORD dwcbAvailable;
   PBYTE pbyteStart;
   DWORD dwcbWritten;
   DWORD dwcbNewUncommitted;

   ASSERT(IS_VALID_STRUCT_PTR(picf, CICACHEDFILE));
   ASSERT(IS_VALID_READ_BUFFER_PTR(hpbyteBuffer, BYTE, (UINT)dwcb));

   ASSERT(IS_FLAG_SET(picf->dwOpenMode, GENERIC_WRITE));
   ASSERT(dwcb > 0);

   /* Is there any room left to write data into the cache? */

   dwcbAvailable = GetAvailableWriteSpace(picf, &pbyteStart);

   /* Yes.  Determine how much to copy into cache. */

   dwcbWritten = min(dwcbAvailable, dwcb);

   /* Can we write anything into the cache? */

   if (dwcbWritten > 0)
   {
      /* Yes.  Write it. */

      CopyMemory(pbyteStart, hpbyteBuffer, dwcbWritten);

      /* Watch out for overflow. */

      ASSERT(picf->dwcbCurFilePosition <= DWORD_MAX - dwcbWritten);

      picf->dwcbCurFilePosition += dwcbWritten;

      /* Watch out for underflow. */

      ASSERT(picf->dwcbCurFilePosition >= picf->dwcbFileOffsetOfCache);

      dwcbNewUncommitted = picf->dwcbCurFilePosition - picf->dwcbFileOffsetOfCache;

      if (picf->dwcbUncommitted < dwcbNewUncommitted)
         picf->dwcbUncommitted = dwcbNewUncommitted;

      if (picf->dwcbValid < dwcbNewUncommitted)
      {
         DWORD dwcbNewFileLen;

         picf->dwcbValid = dwcbNewUncommitted;

         /* Watch out for overflow. */

         ASSERT(picf->dwcbFileOffsetOfCache <= DWORD_MAX - dwcbNewUncommitted);

         dwcbNewFileLen = picf->dwcbFileOffsetOfCache + dwcbNewUncommitted;

         if (picf->dwcbFileLen < dwcbNewFileLen)
            picf->dwcbFileLen = dwcbNewFileLen;
      }
   }

   return(dwcbWritten);
}


/*
** GetAvailableWriteSpace()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE DWORD GetAvailableWriteSpace(PICACHEDFILE picf, PBYTE *ppbyteStart)
{
   DWORD dwcbAvailable;

   ASSERT(IS_VALID_STRUCT_PTR(picf, CICACHEDFILE));
   ASSERT(IS_VALID_WRITE_PTR(ppbyteStart, PBYTE *));

   ASSERT(IS_FLAG_SET(picf->dwOpenMode, GENERIC_WRITE));

   /* Is there room to write data in the cache? */

   /*
    * The current file position must be inside or just after the end of the
    * valid data in the cache, or at the front of the cache when there is no
    * valid data in the cache.
    */

   /* Watch out for overflow. */

   ASSERT(picf->dwcbFileOffsetOfCache <= DWORD_MAX - picf->dwcbValid);

   if (picf->dwcbCurFilePosition >= picf->dwcbFileOffsetOfCache &&
       picf->dwcbCurFilePosition <= picf->dwcbFileOffsetOfCache + picf->dwcbValid)
   {
      DWORD dwcbStartBias;

      /* Yes. */

      dwcbStartBias = picf->dwcbCurFilePosition - picf->dwcbFileOffsetOfCache;

      *ppbyteStart = picf->pbyteCache + dwcbStartBias;

      /* Watch out for underflow. */

      ASSERT(picf->dwcbCacheSize >= dwcbStartBias);

      dwcbAvailable = picf->dwcbCacheSize - dwcbStartBias;
   }
   else
      /* No. */
      dwcbAvailable = 0;

   return(dwcbAvailable);
}


/*
** CommitCache()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** Calling CommitCache() on a file opened without write access is a NOP.
*/
PRIVATE_CODE BOOL CommitCache(PICACHEDFILE picf)
{
   BOOL bResult;

   ASSERT(IS_VALID_STRUCT_PTR(picf, CICACHEDFILE));

   /* Any data to commit? */

   if (IS_FLAG_SET(picf->dwOpenMode, GENERIC_WRITE) &&
       picf->dwcbUncommitted > 0)
   {
      DWORD dwcbOffset;

      /* Yes.  Seek to start position of cache in file. */

      bResult = FALSE;

      dwcbOffset = SetFilePointer(picf->hfile, picf->dwcbFileOffsetOfCache, NULL, FILE_BEGIN);

      if (dwcbOffset != INVALID_SEEK_POSITION)
      {
         DWORD dwcbWritten;

         ASSERT(dwcbOffset == picf->dwcbFileOffsetOfCache);

         /* Write to file from cache. */

         if (WriteFile(picf->hfile, picf->pbyteCache, picf->dwcbUncommitted, &dwcbWritten, NULL) &&
             dwcbWritten == picf->dwcbUncommitted)
         {
            TRACE_OUT((TEXT("CommitCache(): Committed %lu uncommitted bytes starting at offset %lu in file %s."),
                       dwcbWritten,
                       dwcbOffset,
                       picf->pszPath));

            bResult = TRUE;
         }
      }
   }
   else
      bResult = TRUE;

   return(bResult);
}


#ifdef VSTF

/*
** IsValidPCICACHEDFILE()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCICACHEDFILE(PCICACHEDFILE pcicf)
{
   return(IS_VALID_READ_PTR(pcicf, CICACHEDFILE) &&
          IS_VALID_HANDLE(pcicf->hfile, FILE) &&
          FLAGS_ARE_VALID(pcicf->dwOpenMode, ALL_FILE_ACCESS_FLAGS) &&
          EVAL(pcicf->dwcbCacheSize > 0) &&
          IS_VALID_WRITE_BUFFER_PTR(pcicf->pbyteCache, BYTE, (UINT)(pcicf->dwcbCacheSize)) &&
          IS_VALID_WRITE_BUFFER_PTR(pcicf->pbyteDefaultCache, BYTE, (UINT)(pcicf->dwcbDefaultCacheSize)) &&
          EVAL(pcicf->dwcbCacheSize > pcicf->dwcbDefaultCacheSize ||
               pcicf->pbyteCache == pcicf->pbyteDefaultCache) &&
          IS_VALID_STRING_PTR(pcicf->pszPath, STR) &&
          EVAL(IS_FLAG_SET(pcicf->dwOpenMode, GENERIC_WRITE) ||
               ! pcicf->dwcbUncommitted) &&
          (EVAL(pcicf->dwcbValid <= pcicf->dwcbCacheSize) &&
           EVAL(pcicf->dwcbUncommitted <= pcicf->dwcbCacheSize) &&
           EVAL(pcicf->dwcbUncommitted <= pcicf->dwcbValid) &&
           (EVAL(! pcicf->dwcbValid ||
                 pcicf->dwcbFileLen >= pcicf->dwcbFileOffsetOfCache + pcicf->dwcbValid) &&
            EVAL(! pcicf->dwcbUncommitted ||
                 pcicf->dwcbFileLen >= pcicf->dwcbFileOffsetOfCache + pcicf->dwcbUncommitted))));
}

#endif   /* VSTF */

#endif   /* NOFCACHE */


#ifdef VSTF

/*
** IsValidPCCACHEDFILE()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PRIVATE_CODE BOOL IsValidPCCACHEDFILE(PCCACHEDFILE pccf)
{
   return(IS_VALID_READ_PTR(pccf, CCACHEDFILE) &&
          IS_VALID_STRING_PTR(pccf->pcszPath, CSTR) &&
          EVAL(pccf->dwcbDefaultCacheSize > 0) &&
          FLAGS_ARE_VALID(pccf->dwOpenMode, ALL_FILE_ACCESS_FLAGS) &&
          FLAGS_ARE_VALID(pccf->dwSharingMode, ALL_FILE_SHARING_FLAGS) &&
          (! pccf->psa ||
           IS_VALID_STRUCT_PTR(pccf->psa, CSECURITY_ATTRIBUTES)) &&
          IsValidFileCreationMode(pccf->dwCreateMode) &&
          FLAGS_ARE_VALID(pccf->dwAttrsAndFlags, ALL_FILE_ATTRIBUTES_AND_FLAGS) &&
          IS_VALID_HANDLE(pccf->hTemplateFile, TEMPLATEFILE));
}

#endif   /* VSTF */


/****************************** Public Functions *****************************/


/*
** CreateCachedFile()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE FCRESULT CreateCachedFile(PCCACHEDFILE pccf, PHCACHEDFILE phcf)
{
   ASSERT(IS_VALID_STRUCT_PTR(pccf, CCACHEDFILE));
   ASSERT(IS_VALID_WRITE_PTR(phcf, HCACHEDFILE));

   return(SetUpCachedFile(pccf, phcf));
}


/*
** SetCachedFileCacheSize()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  Commits the cache, and discards cached data.
*/
PUBLIC_CODE FCRESULT SetCachedFileCacheSize(HCACHEDFILE hcf, DWORD dwcbNewCacheSize)
{
   FCRESULT fcr;

   /* dwcbNewCacheSize may be any value here. */

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));

#ifdef NOFCACHE

   fcr = FCR_SUCCESS;

#else

   /* Use default cache size instead of 0. */

   if (! dwcbNewCacheSize)
   {
      ASSERT(((PICACHEDFILE)hcf)->dwcbDefaultCacheSize > 0);

      dwcbNewCacheSize = ((PICACHEDFILE)hcf)->dwcbDefaultCacheSize;
   }

   /* Is the cache size changing? */

   if (dwcbNewCacheSize == ((PICACHEDFILE)hcf)->dwcbCacheSize)
      /* No.  Whine about it. */
      WARNING_OUT((TEXT("SetCachedFileCacheSize(): Cache size is already %lu bytes."),
                   dwcbNewCacheSize));

   /* Commit the cache so we can change its size. */

   if (CommitCache((PICACHEDFILE)hcf))
   {
      PBYTE pbyteNewCache;

      /* Throw away cached data. */

      ResetCacheToEmpty((PICACHEDFILE)hcf);

      /* Do we need to allocate a new cache? */

      if (dwcbNewCacheSize <= ((PICACHEDFILE)hcf)->dwcbDefaultCacheSize)
      {
         /* No. */

         pbyteNewCache = ((PICACHEDFILE)hcf)->pbyteDefaultCache;

         fcr = FCR_SUCCESS;

         TRACE_OUT((TEXT("SetCachedFileCacheSize(): Using %lu bytes of %lu bytes allocated to default cache."),
                    dwcbNewCacheSize,
                    ((PICACHEDFILE)hcf)->dwcbDefaultCacheSize));
      }
      else
      {
         /* Yes. */

         if (AllocateMemory(dwcbNewCacheSize, &pbyteNewCache))
         {
            fcr = FCR_SUCCESS;

            TRACE_OUT((TEXT("SetCachedFileCacheSize(): Allocated %lu bytes for new cache."),
                       dwcbNewCacheSize));
         }
         else
            fcr = FCR_OUT_OF_MEMORY;
      }

      if (fcr == FCR_SUCCESS)
      {
         /* Do we need to free the old cache? */

         if (((PICACHEDFILE)hcf)->pbyteCache != ((PICACHEDFILE)hcf)->pbyteDefaultCache)
         {
            /* Yes. */

            ASSERT(((PICACHEDFILE)hcf)->dwcbCacheSize > ((PICACHEDFILE)hcf)->dwcbDefaultCacheSize);

            FreeMemory(((PICACHEDFILE)hcf)->pbyteCache);
         }

         /* Use new cache. */

         ((PICACHEDFILE)hcf)->pbyteCache = pbyteNewCache;
         ((PICACHEDFILE)hcf)->dwcbCacheSize = dwcbNewCacheSize;
      }
   }
   else
      fcr = FCR_WRITE_FAILED;

#endif

   return(fcr);
}


/*
** SeekInCachedFile()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE DWORD SeekInCachedFile(HCACHEDFILE hcf, DWORD dwcbSeek, DWORD uOrigin)
{
   DWORD dwcbResult;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(uOrigin == FILE_BEGIN || uOrigin == FILE_CURRENT || uOrigin == FILE_END);

#ifdef NOFCACHE

   dwcbResult = SetFilePointer(hcf, dwcbSeek, NULL, uOrigin);

#else

   {
      BOOL bValidTarget = TRUE;
      DWORD dwcbWorkingOffset = 0;

      /* Determine seek base. */

      switch (uOrigin)
      {
         case SEEK_CUR:
            dwcbWorkingOffset = ((PICACHEDFILE)hcf)->dwcbCurFilePosition;
            break;

         case SEEK_SET:
            break;

         case SEEK_END:
            dwcbWorkingOffset = ((PICACHEDFILE)hcf)->dwcbFileLen;
            break;

         default:
            bValidTarget = FALSE;
            break;
      }

      if (bValidTarget)
      {
         /* Add bias. */

         /* Watch out for overflow. */

         ASSERT(dwcbWorkingOffset <= DWORD_MAX - dwcbSeek);

         dwcbWorkingOffset += dwcbSeek;

         ((PICACHEDFILE)hcf)->dwcbCurFilePosition = dwcbWorkingOffset;
         dwcbResult = dwcbWorkingOffset;
      }
      else
         dwcbResult = INVALID_SEEK_POSITION;
   }

#endif   /* NOFCACHE */

   return(dwcbResult);
}


/*
** SetEndOfCachedFile()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  Commits cache.
*/
PUBLIC_CODE BOOL SetEndOfCachedFile(HCACHEDFILE hcf)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));

   bResult = CommitCache((PICACHEDFILE)hcf);

   if (bResult)
   {
      bResult = (SetFilePointer(((PICACHEDFILE)hcf)->hfile,
                                ((PICACHEDFILE)hcf)->dwcbCurFilePosition, NULL,
                                FILE_BEGIN) ==
                 ((PICACHEDFILE)hcf)->dwcbCurFilePosition);

      if (bResult)
      {
         bResult = SetEndOfFile(((PICACHEDFILE)hcf)->hfile);

         if (bResult)
         {
            ResetCacheToEmpty((PICACHEDFILE)hcf);

            ((PICACHEDFILE)hcf)->dwcbFileLen = ((PICACHEDFILE)hcf)->dwcbCurFilePosition;

#ifdef DEBUG

            {
               DWORD dwcbFileSizeHigh;
               DWORD dwcbFileSizeLow;

               dwcbFileSizeLow = GetFileSize(((PICACHEDFILE)hcf)->hfile, &dwcbFileSizeHigh);

               ASSERT(! dwcbFileSizeHigh);
               ASSERT(((PICACHEDFILE)hcf)->dwcbFileLen == dwcbFileSizeLow);
               ASSERT(((PICACHEDFILE)hcf)->dwcbCurFilePosition == dwcbFileSizeLow);
            }

#endif

         }
      }
   }

   return(bResult);
}


/*
** GetCachedFilePointerPosition()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE DWORD GetCachedFilePointerPosition(HCACHEDFILE hcf)
{
   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));

   return(((PICACHEDFILE)hcf)->dwcbCurFilePosition);
}


/*
** GetCachedFileSize()
**
** 
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE DWORD GetCachedFileSize(HCACHEDFILE hcf)
{
   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));

   return(((PICACHEDFILE)hcf)->dwcbFileLen);
}


/*
** ReadFromCachedFile()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL ReadFromCachedFile(HCACHEDFILE hcf, PVOID hpbyteBuffer, DWORD dwcb,
                               PDWORD pdwcbRead)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_WRITE_BUFFER_PTR(hpbyteBuffer, BYTE, (UINT)dwcb));
   ASSERT(! pdwcbRead || IS_VALID_WRITE_PTR(pdwcbRead, DWORD));

   *pdwcbRead = 0;

#ifdef NOFCACHE

   bResult = ReadFile(hcf, hpbyteBuffer, dwcb, pdwcbRead, NULL);

#else

   /*
    * Make sure that the cached file has been set up for read access before
    * allowing a read.
    */

   if (IS_FLAG_SET(((PICACHEDFILE)hcf)->dwOpenMode, GENERIC_READ))
   {
      DWORD dwcbToRead = dwcb;

      /* Read requested data. */

      bResult = TRUE;

      while (dwcbToRead > 0)
      {
         DWORD dwcbRead;

         dwcbRead = ReadFromCache((PICACHEDFILE)hcf, hpbyteBuffer, dwcbToRead);

         /* Watch out for underflow. */

         ASSERT(dwcbRead <= dwcbToRead);

         dwcbToRead -= dwcbRead;

         if (dwcbToRead > 0)
         {
            DWORD dwcbNewData;

            if (FillCache((PICACHEDFILE)hcf, &dwcbNewData))
            {
               hpbyteBuffer = (PBYTE)hpbyteBuffer + dwcbRead;

               if (! dwcbNewData)
                  break;
            }
            else
            {
               bResult = FALSE;
               break;
            }
         }
      }

      /* Watch out for underflow. */

      ASSERT(dwcb >= dwcbToRead);

      if (bResult && pdwcbRead)
         *pdwcbRead = dwcb - dwcbToRead;
   }
   else
      bResult = FALSE;

#endif   /* NOFCACHE */

   ASSERT(! pdwcbRead ||
          ((bResult && *pdwcbRead <= dwcb) ||
           (! bResult && ! *pdwcbRead)));

   return(bResult);
}


/*
** WriteToCachedFile()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
**
** N.b., callers don't currently check that *pdwcbWritten == dwcb when
** WriteToCachedFile() returns TRUE.
*/
PUBLIC_CODE BOOL WriteToCachedFile(HCACHEDFILE hcf, PCVOID hpbyteBuffer, DWORD dwcb,
                              PDWORD pdwcbWritten)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));
   ASSERT(IS_VALID_READ_BUFFER_PTR(hpbyteBuffer, BYTE, (UINT)dwcb));

   ASSERT(dwcb > 0);

#ifdef NOFCACHE

   bResult = WriteFile(hcf, hpbyteBuffer, dwcb, pdwcbWritten, NULL);

#else

   /*
    * Make sure that the cached file has been set up for write access before
    * allowing a write.
    */

   if (IS_FLAG_SET(((PICACHEDFILE)hcf)->dwOpenMode, GENERIC_WRITE))
   {
      DWORD dwcbToWrite = dwcb;

      /* Write requested data. */

      bResult = TRUE;

      while (dwcbToWrite > 0)
      {
         DWORD dwcbWritten;

         dwcbWritten = WriteToCache((PICACHEDFILE)hcf, hpbyteBuffer, dwcbToWrite);

         /* Watch out for underflow. */

         ASSERT(dwcbWritten <= dwcbToWrite);

         dwcbToWrite -= dwcbWritten;

         if (dwcbToWrite > 0)
         {
            if (CommitCache((PICACHEDFILE)hcf))
            {
               ResetCacheToEmpty((PICACHEDFILE)hcf);

               hpbyteBuffer = (PCBYTE)hpbyteBuffer + dwcbWritten;
            }
            else
            {
               bResult = FALSE;

               break;
            }
         }
      }

      ASSERT(dwcb >= dwcbToWrite);

      if (pdwcbWritten)
      {
         if (bResult)
         {
            ASSERT(! dwcbToWrite);

            *pdwcbWritten = dwcb;
         }
         else
            *pdwcbWritten = 0;
      }
   }
   else
      bResult = FALSE;

#endif   /* NOFCACHE */

   ASSERT(! pdwcbWritten ||
          ((bResult && *pdwcbWritten == dwcb) ||
           (! bResult && ! *pdwcbWritten)));

   return(bResult);
}


/*
** CommitCachedFile()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL CommitCachedFile(HCACHEDFILE hcf)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));

#ifdef NOFCACHE

   bResult = TRUE;

#else

   /*
    * Make sure that the cached file has been set up for write access before
    * allowing a commit.
    */

   if (IS_FLAG_SET(((PICACHEDFILE)hcf)->dwOpenMode, GENERIC_WRITE))
      bResult = CommitCache((PICACHEDFILE)hcf);
   else
      bResult = FALSE;

#endif   /* NOFCACHE */

   return(bResult);
}


/*
** GetFileHandle()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE HANDLE GetFileHandle(HCACHEDFILE hcf)
{
   HANDLE hfResult;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));

#ifdef NOFCACHE

   hfResult = hcf;

#else

   hfResult = ((PCICACHEDFILE)hcf)->hfile;

#endif   /* NOFCACHE */

   return(hfResult);
}


/*
** CloseCachedFile()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL CloseCachedFile(HCACHEDFILE hcf)
{
   BOOL bResult;

   ASSERT(IS_VALID_HANDLE(hcf, CACHEDFILE));

#ifdef NOFCACHE

   bResult = CloseHandle(hcf);

#else

   {
      BOOL bCommit;
      BOOL bClose;

      bCommit = CommitCache((PICACHEDFILE)hcf);

      bClose = CloseHandle(((PCICACHEDFILE)hcf)->hfile);

      BreakDownCachedFile((PICACHEDFILE)hcf);

      bResult = bCommit && bClose;
   }

#endif   /* NOFCACHE */

   return(bResult);
}


#if defined(DEBUG) || defined(VSTF)

/*
** IsValidHCACHEDFILE()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
PUBLIC_CODE BOOL IsValidHCACHEDFILE(HCACHEDFILE hcf)
{
   BOOL bResult;

#ifdef NOFCACHE

   bResult = TRUE;

#else

   bResult = IS_VALID_STRUCT_PTR((PCICACHEDFILE)hcf, CICACHEDFILE);

#endif   /* NOFCACHE */

   return(bResult);
}

#endif   /* DEBUG || VSTF */

