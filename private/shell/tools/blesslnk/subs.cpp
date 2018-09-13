#include <windows.h>
#include <wininet.h>


void main()
{
    // Assume existing file C:\temp\foo.bar to be 
    // committed to cache associated with 
    // http://www.foo.bar/
    LPSTR szFile = "c:\\temp\\foo.bar";
    LPSTR szUrl  = "http://www.foo.bar/";
    CHAR szCacheFile[MAX_PATH];

    // Expiry and Last-Modified times of 0.
    LONGLONG llZero = 0;
    FILETIME ftExpire  = *((FILETIME*) & llZero);
    FILETIME ftLastMod = *((FILETIME*) & llZero);

    // Create cache file.
    CreateUrlCacheEntry(szUrl, 0, NULL, szCacheFile, 0);
    // Copy existing file to cache file.
    CopyFile(szFile, szCacheFile, FALSE);    

    CommitUrlCacheEntry(szUrl, 
                        szCacheFile, 
                        ftExpire,
                        ftLastMod, 
                        0, 
                        NULL, 
                        0, 
                        NULL, 
                        0);
}