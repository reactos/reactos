/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
/*
 */

#ifndef _CORE_IO_FILE
#define _CORE_IO_FILE


DEFINE_CLASS(File);

class FilenameFilter;

/**
 */
class File: public Base
{
    public: static void classInit();

    DECLARE_CLASS_MEMBERS(File, Base);

    /**
     */
    private: RString path;

    /**
     */
    public: static SRString separator;

    /**
     */
    public: static const TCHAR separatorChar;
    
    /**
     */
    public: static SRString pathSeparator;

    /**
     */
    public: static const TCHAR pathSeparatorChar;

    protected: File() : path(REF_NOINIT) {}
    
    /**
     */
    public: static File * newFile(String * path);

    /**
     */
    public: static File * newFile(String * path, String * name);

    /**
     */
    public: static File * newFile(File * dir, String * name);

    /**
     * Helper to concatenate the directory with file name
     */
    private: void init(String * path, String * name);

    /**
     */
    public: virtual String * getName();

    /**
     */
    public: virtual String * getPath() 
            {
                return path;
            }


    /**
     * Helper return current user directory.
     */
    public: String * getUserDirectory();

    /**
     */
    public: virtual String * getAbsolutePath();

    /**
     */
     public: virtual String * getCanonicalPath(); //throws IOException 

    /**
     */
    public: virtual String * getParent();

    private: virtual String * canonPath(String * p); //throws IOException;

    /**
     */
    public: virtual bool exists();

    /**
     */
    public: virtual bool canWrite();

    /**
     */
    public: virtual bool canRead();

    /**
     */
    public: virtual bool isFile();

    /**
     */
    public: virtual bool isDirectory();

    /**
     */
    public: virtual bool isAbsolute();

    /**
     */
    public: virtual int64 lastModified();

    /**
     */
    public: virtual int64 length();

    /**
     */
    public: virtual bool mkdir();

    /**
     */
    public: virtual bool renameTo(File * dest);

    /**
     */
    public: virtual bool mkdirs();

    /**
     */
    public: virtual bool remove();

    /**
     */
    public: virtual int hashCode() 
            {
                return path->hashCode() ^ 1234321;
            }

    /**
     */
    public: virtual bool equals(Object * obj);

    /**
     */
    public: virtual String * toString() 
            {
                return getPath();
            }

    virtual void finalize()
    {
        path = null;
        super::finalize();
    }

  // #ifdef UNIX
  //   BOOL operator == (T * p) const { return _p ==  p; }
  // #endif
};


#endif _CORE_IO_FILE
