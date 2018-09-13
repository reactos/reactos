/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
/*
 */

#include "core.hxx"
#pragma hdrstop

#include "core/io/file.hxx"

DEFINE_CLASS_MEMBERS(File, _T("File"), Base);

/**
 */
#ifdef UNIX
const TCHAR File::separatorChar = '/'; // separator.charAt(0);
#else
const TCHAR File::separatorChar = '\\'; // separator.charAt(0);
#endif

/**
 */
const TCHAR File::pathSeparatorChar = ':'; // pathSeparator.charAt(0);

SRString File::separator;
SRString File::pathSeparator;

void 
File::classInit()
{

#ifdef UNIX
    if (!File::separator)
        File::separator = String::newString(_T("/")); // System.getProperty("file.separator");
    if (!File::pathSeparator)
        File::pathSeparator = String::newString(_T(":")); // System.getProperty("pat
#else
    if (File::separator == null)
        File::separator = String::newString(_T("\\")); // System.getProperty("file.separator");
    if (File::pathSeparator == null)
        File::pathSeparator = String::newString(_T(":")); // System.getProperty("path.separator");
#endif
}


/**
 */
File * 
File::newFile(String * path) 
{
    return newFile(path, null);
}    

/**
 */
File *
File::newFile(String * path, String * name) 
{
    File * f = new File();
    f->init(path, name);
    return f;
}

/**
 */
File *
File::newFile(File * dir, String * name) 
{
    return newFile(dir->getPath(), name);
}

/**
 */
void File::init(String * path, String * name)
{
    if (path != null) 
    {
        if (name != null)
        {
            if (path->endsWith(separator)) 
            {
                this->path = String::add(path, name, null);
            } 
            else 
            {
                this->path = String::add(path, separator, name, null);
            } 
        }
        else
        {
            this->path = path;
        }
    } 
}

/**
 */
String * File::getName() 
{
    int index = path->lastIndexOf(separatorChar);
#ifdef UNIX
    //
    // CODE REVIEW - Is this what we really want to do???
    // path is a RString, so would a (String*)&path be better?
    //
    return (index < 0) ? (String*)path : path->substring(index + 1);
#else
    return (index < 0) ? path : path->substring(index + 1);
#endif

}

/**
 */
String * File::getUserDirectory()
{
    const int size = MAX_PATH;
    char buf[size+1];

    DWORD dw = ::GetCurrentDirectoryA(size, buf);
    if (dw == 0)
    {
        Exception::throwLastError();         
    }
    buf[dw] = '\0';
    return String::newString(buf);
}

/**
 */
String * File::getAbsolutePath() 
{
    DWORD size = MAX_PATH + 1;
    char* buffer = new char[size];
    char* file = null;

    DWORD rc = GetFullPathNameA((char *)AsciiText(path),
        size, buffer, &file);
   
    String* result = null;
    if (rc > 0)
        result = String::newString(buffer);

    delete[] buffer;
    return result;
}

/**
 */
String * File::getCanonicalPath() //throws IOException 
{
    return isAbsolute() ? canonPath(path) :
        // canonPath(System.getProperty("user.dir") + separator + path);
        canonPath(String::add(getUserDirectory(), separator, path, null));
}

/**
 */
String * File::getParent() 
{
    int index = path->lastIndexOf(separatorChar);
    return (index <= 0) ? (String *)null : path->substring(0, index);
}

String * File::canonPath(String * p) //throws IOException;
{
    // save current directory, set it to p 
    // to get canonical form back, and reset it
    char save[1024];
    char buf[1024];

    //
    // BUGBUG - Should use Unicode on WinNT
    //

    buf[0] = 0;
    DWORD dw = ::GetCurrentDirectoryA(1024, save);
    if (dw == 0)
    {
        Exception::throwLastError();
    }
    save[dw] = '\0';
    dw = 0;
    if (::SetCurrentDirectoryA((char *)AsciiText(p)))
    {
        dw = ::GetCurrentDirectoryA(1024, buf);
    }
    ::SetCurrentDirectoryA(save);
    if (dw == 0)
    {
        Exception::throwLastError();
    }
    buf[dw] = '\0';
    return String::newString(buf);
}

/**
 */
bool File::exists() 
{    //
    // BUGBUG - Should use unicode on WinNT
    //

    return ::GetFileAttributesA((char *)AsciiText(path)) != 0xFFFFFFFF;
}

/**
 */
bool File::canWrite() 
{
    //
    // BUGBUG - Should use unicode on WinNT
    //

    DWORD dw = GetFileAttributesA((char *)AsciiText(path));
    return dw != 0xFFFFFFFF && !(dw & (FILE_ATTRIBUTE_READONLY | 
                        FILE_ATTRIBUTE_DIRECTORY | 
                        FILE_ATTRIBUTE_OFFLINE | 
                		FILE_ATTRIBUTE_SYSTEM));
}

/**
 */
bool File::canRead() 
{
    //
    // BUGBUG - Should use unicode on WinNT
    //

    DWORD dw = GetFileAttributesA((char *)AsciiText(path));
    return dw != 0xFFFFFFFF && !(dw & (FILE_ATTRIBUTE_DIRECTORY | 
                        FILE_ATTRIBUTE_OFFLINE | 
                        FILE_ATTRIBUTE_SYSTEM));
}

/**
 */
bool File::isFile() 
{
    //
    // BUGBUG - Should use unicode on WinNT
    //

    DWORD dw = GetFileAttributesA((char *)AsciiText(path));
    return dw != 0xFFFFFFFF && !(dw & (FILE_ATTRIBUTE_DIRECTORY));
}

/**
 */
bool File::isDirectory() 
{
    //
    // BUGBUG - Should use unicode on WinNT
    //

    DWORD dw = GetFileAttributesA((char *)AsciiText(path));
    return dw != 0xFFFFFFFF && (dw & (FILE_ATTRIBUTE_DIRECTORY));
}

/**
 */
bool File::isAbsolute()
{
    TRY
    {
        String * p = canonPath(path);
        return p->substring(0, 2)->equalsIgnoreCase(path->substring(0, 2));
    }
    CATCH
    {
        return false;
    }
    ENDTRY

}

/**
 */
int64 File::lastModified() 
{
    //
    // BUGBUG - Should use unicode on WinNT
    //

    HANDLE h = ::CreateFileA(
        (char *)AsciiText(path),
        GENERIC_READ,
        FILE_SHARE_READ|FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        isDirectory() ? FILE_FLAG_BACKUP_SEMANTICS : FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (h == INVALID_HANDLE_VALUE)
    {
        Exception::throwLastError();
    }
    BY_HANDLE_FILE_INFORMATION s;
    bool fail = !::GetFileInformationByHandle(h, &s);
    ::CloseHandle(h);
    if (fail)
    {
        Exception::throwLastError();
    }
    return *(int64 *)&s.ftLastWriteTime;
}

/**
 */
int64 File::length() 
{
    //
    // BUGBUG - Should use unicode on WinNT
    //

    HANDLE h = ::CreateFileA(
        (char *)AsciiText(path),
        GENERIC_READ,
        FILE_SHARE_READ|FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        isDirectory() ? FILE_FLAG_BACKUP_SEMANTICS : FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (h == INVALID_HANDLE_VALUE)
    {
        Exception::throwLastError();
    }
    LARGE_INTEGER li;
    li.LowPart = ::GetFileSize(h, (DWORD *)&li.HighPart);
    DWORD error = 0;
    if (li.LowPart == 0xFFFFFFFF)
    {
        error = ::GetLastError();
    }
    ::CloseHandle(h);
    if (error)
    {
        Exception::throwLastError();
    }
#ifdef UNIX
    // BUGBUG
    // see /vobs/build/userx/public/sdk/inc/ntdef.h 
    // union includes QuadPart which is ULONGULONG
    // BIG Endian / LITTLE Endian may effect this
    return QUAD_PART(li); 
#else
    return li.QuadPart;
#endif

}

/**
 */
bool File::mkdir() 
{
    //
    // BUGBUG - Should use unicode on WinNT
    //

    return ::CreateDirectoryA((char *)AsciiText(path), NULL) != 0;
}

/**
 */
bool File::renameTo(File * dest) 
{
    //
    // BUGBUG - Should use unicode on WinNT
    //


    return ::MoveFileA((char *)AsciiText(path), (char *)AsciiText(dest)) != 0;
}

/**
 */
bool File::mkdirs() 
{
    if(exists()) 
    {
        return false;
    }
    if (mkdir()) 
    {
         return true;
    }

    String * parent = getParent();
    return (parent != null) && (File::newFile(parent)->mkdirs() && mkdir());
}


/**
 */
bool File::remove() 
{
    //
    // BUGBUG - Should use unicode on WinNT
    //

    if (isDirectory())
    {
        return ::RemoveDirectoryA((char *)AsciiText(path)) != 0;
    }
    else
    {
        return ::DeleteFileA((char *)AsciiText(path)) != 0;
    }
}

/**
 */
bool File::equals(Object * obj) 
{
    if ((obj != null) && (File::_getClass()->isInstance(obj))) 
    {
        return path->equalsIgnoreCase(((File *)obj)->path);
    }
    return false;
}
