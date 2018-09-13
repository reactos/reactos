#include <cache.hxx>
#include <conmgr.hxx>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include <errno.h>


extern int errno;

/* Code swiped from cachecfg.cxx */
static BOOL _NormalisePath(LPCTSTR pszPath, LPCTSTR pszEnvVar,
                           LPTSTR pszResult, UINT cbResult)
{
     TCHAR szEnvVar[MAX_PATH];

     // don't count the NULL
     ExpandEnvironmentStrings(pszEnvVar, szEnvVar, sizeof(szEnvVar)-1);
     DWORD dwEnvVar = lstrlen(szEnvVar);

     if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, szEnvVar,
                       dwEnvVar, pszPath, dwEnvVar) == 2)
     {
        if (lstrlen(pszPath) + dwEnvVar < cbResult)
        {
           strncpy(pszResult, pszEnvVar, MAX_PATH);
           strncat(pszResult, pszPath + dwEnvVar, MAX_PATH);
           return TRUE;
         }
     }

     return FALSE;
}

void UnixNormalisePath(LPTSTR pszOrigPath, LPCTSTR pszEnvVar)
{
     TCHAR szScratch[MAX_PATH];
    
     if (_NormalisePath(pszOrigPath,pszEnvVar,szScratch,sizeof(szScratch)))
        strncpy(pszOrigPath,szScratch,MAX_PATH);
}

void UnixNormaliseIfCachePath(LPTSTR pszOrigPath, LPCTSTR pszEnvVar,
                              LPCTSTR pszKeyName)
{
     if (!strncmp(pszKeyName,CACHE_PATH_VALUE,lstrlen(CACHE_PATH_VALUE)))
        UnixNormalisePath(pszOrigPath,pszEnvVar);
}

int UnixPathExists(LPCTSTR pszPath)
{
     struct stat statbuf;

     if (stat(pszPath, &statbuf) < 0)
     {
        /* If path does not exist */
        if (errno == ENOENT)
           return 0;
        else
           return -1;
     }
     
     /* TODO */
     /* Make sure path points to a directory */

     return 1;
}

void UnixGetValidParentPath(LPTSTR szDevice)
{
     TCHAR szDeviceExists[MAX_PATH];
     PTSTR pszDeviceExists = NULL;
     PTSTR pszEnd          = NULL;
 
     if (!szDevice)
        return;

     lstrcpy(szDeviceExists, szDevice);
 
     pszDeviceExists = szDeviceExists;
     pszEnd          = szDeviceExists + lstrlen(szDeviceExists);
 
     for(;;)
     {
        int   fPathExists;
 
        if (pszEnd == pszDeviceExists)
           break;
 
        fPathExists = UnixPathExists(pszDeviceExists);
        if (fPathExists == -1)
        {
           /* Error */
           break;
        }
        else
        if (fPathExists == 0)
        {
           /* Path does not exist */
           while (*pszEnd != DIR_SEPARATOR_CHAR &&
                  pszEnd != pszDeviceExists)
                 pszEnd--;
 
           *pszEnd = '\0';

           continue;
        }
        else
        {
           /* Path exists */
           lstrcpy(szDevice, pszDeviceExists);
           break;
        }
    }
}

/* CopyDir */

static int DoCopy();

static int UnixCopyCacheFile(const char* file_src,
                         const char* file_dest,
                         mode_t fmode);

static int UnixCreateCacheFolder( const char* dir_dest, mode_t fmode);

#ifndef BUFSIZ
#define BUFSIZ 4096
#endif /* BUFSIZ */

#define CUR_DIR  "."
#define PREV_DIR ".."

static char* pathdir1 = NULL;
static char* pathdir2 = NULL;

int CopyDir(const char* dirname1, const char* dirname2)
{
    int Error = 0;
    struct stat statdir1, statdir2;

    if (!dirname1 || !dirname2)
    {
       goto Cleanup;
    }

    /* We are assuming that dirname1 and dirname2 are absolute paths */
    if (stat(dirname1, &statdir1) < 0)
    {
       Error = errno;
       goto Cleanup;
    }
    else
    if (!S_ISDIR(statdir1.st_mode))
    {
       Error = -1; /* source is not directory */
       goto Cleanup;
    }

    if (stat(dirname2, &statdir2) < 0)
    {
       if (errno != ENOENT)
       {
          Error = errno;
          goto Cleanup;
       }
       /* It is fine if the destination dir does not exist
        * provided all directories above the leaf dir exist
        */
    }
    else
    if (!S_ISDIR(statdir2.st_mode))
    {
       Error = -1; /* destination is not directory */
       goto Cleanup; 
    }

    pathdir1 = (char*)malloc((MAX_PATH+1)*sizeof(char));
    pathdir2 = (char*)malloc((MAX_PATH+1)*sizeof(char));

    lstrcpy(pathdir1, dirname1);
    lstrcpy(pathdir2, dirname2);

    Error = DoCopy();

Cleanup:

    if (pathdir1)
       free(pathdir1);

    if (pathdir2)
       free(pathdir2);

    pathdir1 = pathdir2 = NULL;

    return Error;
}

int DoCopy()
{
    struct stat statbuf;
    struct dirent *dirp;
    DIR           *dp;
    int           Error;
    char          *ptr1, *ptr2;

    if (stat(pathdir1, &statbuf) < 0)
    {
       Error = errno;
       goto Cleanup;
    }

    /* Check if this is a regular file */
    if ((statbuf.st_mode & S_IFMT) == S_IFREG)
    {
       Error = UnixCopyCacheFile(pathdir1, pathdir2, statbuf.st_mode);
       goto Cleanup;
    }

    /* Now, we are dealing with a directory */
    if ((Error = UnixCreateCacheFolder(pathdir2, statbuf.st_mode)))
       goto Cleanup;

    ptr1 = pathdir1 + lstrlen(pathdir1);
    *ptr1++ = '/';
    *ptr1   = 0;

    ptr2 = pathdir2 + lstrlen(pathdir2);
    *ptr2++ = '/';
    *ptr2 = 0;

    if ((dp = opendir(pathdir1)) == NULL)
    {
       Error = errno;
       goto Cleanup;
    }

    while ((dirp = readdir(dp)) != NULL)
    {
          if (!lstrcmp(dirp->d_name, CUR_DIR) ||
              !lstrcmp(dirp->d_name, PREV_DIR))
             continue;

          lstrcpy(ptr1, dirp->d_name);
          lstrcpy(ptr2, dirp->d_name);

          if ((Error = DoCopy()))
             break;
    }
    ptr1[-1] = 0;
    ptr2[-1] = 0;

    /* If this fails, ignore this error */
    closedir(dp);

Cleanup:

    return Error;
}

static int UnixCreateCacheFolder( const char* path_dest, mode_t mode_src)
{
    int Error = 0;
    struct stat statbuf2;

    if (stat(path_dest, &statbuf2) < 0)
    {
       if (errno == ENOENT)
       {
          if (mkdir(path_dest, mode_src) < 0)
          {
             Error = errno;
             goto Cleanup;
          }
       }
       else
       {
          Error = errno;
          goto Cleanup;
       }
    }
    else
    if (!S_ISDIR(statbuf2.st_mode))
       Error = -1; /* we are expecting a directory */

Cleanup:
    return Error;
}

int UnixCopyCacheFile(const char* file_src, const char* file_dest, mode_t fmode)
{
    int Error = 0;
    int fd1, fd2;
    char buf[BUFSIZ];
    int  nread, nwrite;

    if ((fd1 = open(file_src, O_RDONLY)) < 0)
    {
       Error = errno;
       goto Cleanup;
    }

    if ((fd2 = open(file_dest, O_CREAT|O_TRUNC|O_WRONLY, fmode)) < 0)
    {
       Error = errno;
       goto Cleanup;
    }

    while((nread = read(fd1, buf, BUFSIZ)) > 0)
    {
         if ((nwrite = write(fd2, buf, nread)) != nread)
         {
            Error = errno;
            goto Cleanup;
         }
    }

    Error = 0;

Cleanup:

    if (fd1 > 0)
       close(fd1);

    if (fd2 > 0)
       close(fd2);

    return Error;
}
