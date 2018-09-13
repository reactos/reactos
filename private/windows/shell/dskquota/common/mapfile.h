#ifndef _INC_DSKQUOTA_MAPFILE_H
#define _INC_DSKQUOTA_MAPFILE_H

//
// Simple encapsulation of a mapped file for opening font files.
//
class MappedFile
{
    public:
        MappedFile(VOID);
        ~MappedFile(VOID);
        //
        // Open the mapped file.
        //
        HRESULT Open(LPCTSTR pszFile);
        //
        // Close the mapped file.
        //
        VOID Close(VOID);
        //
        // Get the base virtual address of the mapped file.
        //
        LPBYTE Base(VOID) const
            { return m_pbBase; }
        //
        // How many bytes in the mapped file?
        //
        LONGLONG Size(VOID) const;

    private:
        HANDLE   m_hFile;
        HANDLE   m_hFileMapping;
        LPBYTE   m_pbBase;
        LONGLONG m_llSize;

        //
        // Prevent copy.
        //
        MappedFile(const MappedFile& rhs);
        MappedFile& operator = (const MappedFile& rhs);
};

#endif // _INC_DSKQUOTA_MAPFILE_H
