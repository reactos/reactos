#include <windows.h>
#include <msvcrt/sys/types.h>
#include <msvcrt/sys/stat.h>
#include <msvcrt/fcntl.h>
#include <msvcrt/io.h>
#include <msvcrt/errno.h>


int _stat(const char* path, struct stat* buffer)
{
    HANDLE fh;
    WIN32_FIND_DATA wfd;

    fh = FindFirstFile(path, &wfd);
    if (fh == INVALID_HANDLE_VALUE) {
        __set_errno(ENOFILE);
        return -1;
    }
    if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        int fd = _open(path, _O_RDONLY);
        int ret;
        ret = fstat(fd, buffer);
        _close(fd);
        return ret;
    }
    buffer->st_ctime = FileTimeToUnixTime(&wfd.ftCreationTime,NULL);
    buffer->st_atime = FileTimeToUnixTime(&wfd.ftLastAccessTime,NULL);
    buffer->st_mtime = FileTimeToUnixTime(&wfd.ftLastWriteTime,NULL);

    if (buffer->st_atime ==0)
        buffer->st_atime = buffer->st_mtime;
    if (buffer->st_ctime ==0)
        buffer->st_ctime = buffer->st_mtime;

    buffer->st_mode = S_IREAD;
    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        buffer->st_mode |= S_IFDIR;
    else
        buffer->st_mode |= S_IFREG;
    if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
        buffer->st_mode |= S_IWRITE | S_IEXEC;

    buffer->st_size = wfd.nFileSizeLow; 
    buffer->st_nlink = 1;
    if (FindNextFile(fh, &wfd)) {
        __set_errno(ENOFILE);
        FindClose(fh);
        return -1;
    }
    return 0;
}
