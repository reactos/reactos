#ifndef __IMGFILE_H__
#define __IMGFILE_H__

// This is a little helper class for writing things to temporary files
// and giving them the correct name after the save suceeds...
//
class CFileSaver
    {
    public:

     CFileSaver(const TCHAR* szFileName); // setup and create temp names
    ~CFileSaver();                       // make sure temp files are gone

    BOOL CanSave() const;               // checks for R/O
    const TCHAR* GetSafeName() const     // return name of file to create
                                    { return m_strTempName; }
    const TCHAR* GetRealName() const     // return name of final file
                                    { return m_strName; }
    BOOL Finish();                      // rename new file as original

    private:

    CString m_strName;
    CString m_strBackupName;
    CString m_strTempName;

    static const TCHAR BASED_CODE c_szAps [];
    };

struct ICONFILEHEADER
    {
    WORD icoReserved;
    WORD icoResourceType;
    WORD icoResourceCount;
    };


struct ICONDIRENTRY
    {
    BYTE nWidth;
    BYTE nHeight;
    BYTE nColorCount;
    BYTE bReserved;
    WORD wReserved1;
    WORD wReserved2;
    DWORD icoDIBSize;
    DWORD icoDIBOffset;
    };

struct CURSORFILEHEADER
    {
    WORD curReserved;
    WORD curResourceType;
    WORD curResourceCount;
    };


struct CURSORDIRENTRY
    {
    BYTE nWidth;
    BYTE nHeight;
    WORD wReserved;
    WORD curXHotspot;
    WORD curYHotspot;
    DWORD curDIBSize;
    DWORD curDIBOffset;
    };



extern int MkPath(TCHAR *szPath);
extern void MkFullPath(CString& strFullPath, const CString& strRelPath,
    BOOL bPathOnly = FALSE);
extern BOOL OpenSubFile(CFile& file, const CFileSaver& saver, UINT nOpenFlags,
    CFileException* pError = NULL);



/////////////////////////////////////////////////////////////////////////

#endif // __IMGFILE_H__
