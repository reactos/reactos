//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       filelist.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCVIEW_FILELIST_H
#define _INC_CSCVIEW_FILELIST_H
//////////////////////////////////////////////////////////////////////////////
/*  File: filelist.h

    Description: Simplifies the transmission of a list of share and 
        associated file names between components of the CSC UI.  See
        description further below for details.

        Classes:

            CscFilenameList
            CscFilenameList::HSHARE
            CscFilenameList::ShareIter
            CscFilenameList::FileIter

        Note:  This module was written to be used by any part of the CSCUI,
               not just the viewer.  Therefore, I don't assume that the
               new operator will throw an exception on allocation failures.
               I don't like all the added code to detect allocation failures
               but it's not reasonable to expect code in the other components
               to become exception-aware with respect to "new" failures.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    11/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _WINDOWS_
#   include <windows.h>
#endif

//
// This set of classes is designed to transfer a list of network share
// names and network file names between components of the CSC UI
// (i.e. wizard, viewer, onestop etc).  Data is transferred through
// a formatted byte buffer that is essentially an opaque blob
// to both the sender and receiver.  The class CscFilenameList
// was created to prevent the sender and receiver from having
// to understand the buffer format.  The buffer is formatted as follows:
//
// +----------------+---------------------------------+
// | <namelist hdr> |  <share descriptors>            |
// +----------------+---------------------------------+
// |                                                  |
// |                                                  |
// |         <share and file names>                   |
// |                                                  |
// |                                                  |
// +--------------------------------------------------+
//
//
// Where: <namelist hdr> is a single block of type CSC_NAMELIST_HDR.
//              This block describes the size of the buffer and the
//              share count.
//
//        <share descriptors> is an array of type CSC_NAMELIST_SHARE_DESC,
//              one for each share in the buffer.  Each share descriptor
//              describes the offset to the share name, the offset
//              to the first file name and the number of file names
//              in the buffer.  All offsets are byte offsets from the
//              start of the buffer.
//
//        <share and file names> is an array of TCHARs containing the
//              names of the shares and files stored in the buffer.
//              Each name string is nul-terminated.
//
//
// The following outline describes how I see someone using this feature:
//
//  1. Instantiate a CscFilenameList object (no ctor arguments).
//  2. Call AddShare() whenever a share is to be added.
//  3. Call AddFile() whenever a file is to be added.  Note that
//     saving the HSHARE returned by AddShare() can make file name addition
//     more efficient as it eliminates the need for an internal lookup
//     of the associated share name each time a file name is added.
//
//      i.e.:  CscFilenameList fnl;
//             HSHARE hShare = fnl.AddShare("\\\\server\\share");
//             fnl.AddFile(hShare, "foo.txt");
//             fnl.AddFile(hShare, "bar.txt");
//
//             is more efficient than...
//
//             CscFilenameList fnl;
//             fnl.AddFile("\\\\server\\share", "foo.txt");
//             fnl.AddFile("\\\\server\\share", "bar.txt");
//
//             ...which is slightly more efficient than...
//
//             CscFilenameList fnl;
//             fnl.AddFile("\\\\server\\share\\foo.txt");
//             fnl.AddFile("\\\\server\\share\\bar.txt");
//
//             ...although all 3 methods are supported.
//
//  4. Once all shares and files are added, call CreateListBuffer() to retrieve
//     the information formatted in a byte buffer.  The buffer is allocated
//     on the heap.
//  5. Pass the buffer address to the desired program component.
//  6. The receiving component instantiates a CscFilenameList object passing
//     the address of the buffer to the ctor.  This initializes the namelist
//     so that it merely references the information in the buffer rather than
//     duplicating the name information in memory.  Note that the buffer must
//     remain in memory while this namelist object is in use.
//  7. The receiving component creates a Share Iterator by calling
//     CreateShareIterator().  The returned iterator enumerates each of the
//     shares contained in the namelist object.  
//  8. The receiving component enumerates the shares (receiving an HSHARE) for 
//     each.  To get a share's name string, call GetShareName() passing the
//     HSHARE for the desired share.
//  9. For each share, the recieving component creates a File Iterator by
//     calling CreateFileIterator() passing the HSHARE for the desired share.
//     The returned iterator enumerates each of the file names associated 
//     with the share.
// 10. Once the operation is complete, FreeListBuffer is called to delete
//     the byte buffer created by CreateListBuffer().
//
//
// The following example illustrates this process:
//
//
//  void Foo(void)
//  {
//      CscFilenameList         fnl;     // Namelist object.
//      CscFilenameList::HSHARE hShare;  // Share handle.
//
//      // Add a share and some files
//
//      fnl.AddShare(TEXT("\\\\worf\\ntspecs"), &hShare);
//      fnl.AddFile(hShare, TEXT("foo.txt"));
//      fnl.AddFile(hShare, TEXT("bar.txt"));
//
//      // Add another share and some files.
//
//      fnl.AddShare(TEXT("\\\\msoffice\\products"), &hShare);
//      fnl.AddFile(hShare, TEXT("word.doc"));
//      fnl.AddFile(hShare, TEXT("excel.doc"));
//      fnl.AddFile(hShare, TEXT("powerpoint.doc"));
//      fnl.AddFile(hShare, TEXT("access.doc"));
//
//      // Add another share and more files using the less-efficient
//      // method.  It's valid, just less efficient.
//
//      fnl.AddFile(TEXT("\\\\performance\\poor"), TEXT("sucks.doc"));
//      fnl.AddFile(TEXT("\\\\performance\\poor"), TEXT("blows.doc"));
//      fnl.AddFile(TEXT("\\\\performance\\poor"), TEXT("bites.doc"));
//
//      // Create the byte buffer from the namelist and pass it to
//      // the receiving component.
//
//      LPBYTE pbBuffer = fnl.CreateListBuffer();
//      Bar(pbBuffer);
//
//      // Delete the byte buffer when we're done.
//
//      FreeListBuffer(pbBuffer);
//  }
//
//
//  void Bar(LPBYTE pbBuffer)
//  {
//      // Create a new namelist object from the byte buffer.
//
//      CscFileNameList fnl(pbBuffer);
//
//      // Create a share iterator.
//      
//      CscFilenameList::ShareIter si = fnl.CreateShareIterator();
//      CscFilenameList::HSHARE hShare;
//
//      // Iterate over the shares in the namelist collection.
//
//      while(si.Next(&hShare))
//      {
//          _tprintf(TEXT("Share..: \"%s\"\n"), fnl.GetShareName(hShare));
//
//          // Create a file iterator for the share.
//
//          CscFilenameList::FileIter fi = fl.CreateFileIterator(hShare);
//          LPCTSTR pszFile;
//
//          // Iterate over the filenames associated with the share.
//
//          while(pszFile = fi.Next())
//          {
//              _tprintf(TEXT("\tFile..: \"%s\"\n"), pszFile);
//          }
//      }
//  }
//
//  [brianau - 11/28/97]
//

//
// Namelist byte buffer header block (offset 0).
//
typedef struct 
{
    DWORD cbSize;
    DWORD cShares;

} CSC_NAMELIST_HDR, *PCSC_NAMELIST_HDR;

//
// Namelist byte buffer share descriptor block.  Array (one per share)
// starting at offset 12 (immediately following the header block).
//
typedef struct
{
    DWORD cbOfsShareName;
    DWORD cbOfsFileNames;
    DWORD cFiles;

} CSC_NAMELIST_SHARE_DESC, *PCSC_NAMELIST_SHARE_DESC;



class CscFilenameList
{
    private:
        class Share;

    public:
        class ShareIter;
        class FileIter;
        class HSHARE;

        // --------------------------------------------------------------------
        // CscFilenameList::HSHARE
        //
        // Share "handle" to communicate the identity of an internal "share"
        // object with the client without exposing the share object directly.
        // The client just knows an HSHARE.  Clients get one as the return
        // value from AddShare().
        //
        class HSHARE
        {
            public:
                HSHARE(void);
                HSHARE(const HSHARE& rhs);
                HSHARE& operator = (const HSHARE& rhs);
                ~HSHARE(void) { }

                bool operator ! ()
                    { return NULL == m_pShare; }

            private:
                //
                // Private so only we can create meaninful HSHARE objects.
                //
                HSHARE(Share *pShare)
                    : m_pShare(pShare) { }

                Share *m_pShare;  // ptr to the actual share object.

                friend class CscFilenameList;
                friend class ShareIter;
        };

        // --------------------------------------------------------------------
        // CscFilenameList::FileIter
        //
        // Iterator for enumerating each file name associated with a 
        // particular share. Clients create one using CreateFileIterator().
        //
        class FileIter
        {
            public:
                FileIter(void);
                ~FileIter(void) { }
                FileIter(const FileIter& rhs);
                FileIter& operator = (const FileIter& rhs);

                LPCTSTR Next(void);
                void Reset(void);

            private:
                FileIter(const Share *pShare);

                const Share *m_pShare; // ptr to associated share object.
                int          m_iFile;  // current file iteration index.

                friend class CscFilenameList;
        };

        // --------------------------------------------------------------------
        // CscFilenameList::ShareIter
        //
        // Iterator for enumerating each share in the namelist collection.
        // Clients create one using CreateShareIterator().
        //
        class ShareIter
        {
            public:
                ShareIter(void);
                ShareIter(const CscFilenameList& fnl);
                ~ShareIter(void) { }
                ShareIter(const ShareIter& rhs);
                ShareIter& operator = (const ShareIter& rhs);

                bool Next(HSHARE *phShare);
                void Reset(void);

            private:
                const CscFilenameList *m_pfnl; // ptr to filename collection obj.
                int   m_iShare;                // current share iteration index.
        };

        // --------------------------------------------------------------------
        // Namelist object public interface.
        //
        // Create an empty namelist collection ready to accept share and
        // file names.
        //
        CscFilenameList(void);
        //
        // Create a namelist collection and initialize it with the contents
        // of a byte buffer created by CreateListBuffer().  
        // If bCopy is false, the subsequent namelist object merely references 
        // the data in the byte buffer rather than duplicating the namestrings 
        // in memory.  If bCopy is true, name strings are created as if the
        // names had been added using AddShare() and AddFile().  Note that
        // additional share and file name strings may be added at any time.
        // However, they are added to internal structures and not to the 
        // byte buffer.  Call CreateListBuffer() to add them to a new byte
        // buffer.  
        //
        CscFilenameList(PCSC_NAMELIST_HDR pbNames, bool bCopy);
        ~CscFilenameList(void);
        //
        // Add a share name to the collection.  Does not create a 
        // duplicate share entry if one already exists.  Returns a handle
        // the a share object.
        //
        bool AddShare(LPCTSTR pszShare, HSHARE *phShare, bool bCopy = true);
        //
        // Add a file for a share.  More efficient to use the first
        // version taking a share handle rather than a share name.
        //
        bool AddFile(HSHARE& hShare, LPCTSTR pszFile, bool bDirectory = false, bool bCopy = true);
        bool AddFile(LPCTSTR pszShare, LPCTSTR pszFile, bool bDirectory = false, bool bCopy = true);
        bool AddFile(LPCTSTR pszFullPath, bool bDirectory = false, bool bCopy = true);
        bool RemoveFile(HSHARE& hShare, LPCTSTR pszFile);
        bool RemoveFile(LPCTSTR pszShare, LPCTSTR pszFile);
        bool RemoveFile(LPCTSTR pszFullPath);
        //
        // Retrieve miscellaneous information about the collection.
        //
        int GetShareCount(void) const;
        int GetFileCount(void) const;
        LPCTSTR GetShareName(HSHARE& hShare) const;
        int GetShareFileCount(HSHARE& hShare) const;
        bool GetShareHandle(LPCTSTR pszShare, HSHARE *phShare) const;
        //
        // Determine if a given share or file exists in the collection.
        // For the FileExists() functions, if bExact is true (the default)
        // only exact character-for-character matches return true.  If
        // bExact is false, the filename "\\server\share\dirA\dirB\foo.txt"
        // will match if any of the following four entries exist in the
        // namelist:
        //
        //     "\\server\share\dirA\dirB\foo.txt"  (exact match)
        //     "\\server\share\*"                  (wildcard match)
        //     "\\server\share\dirA\*"             (wildcard match)
        //     "\\server\share\dirA\dirB\*"        (wildcard match)
        //
        bool ShareExists(LPCTSTR pszShare) const;
        bool FileExists(HSHARE& hShare, LPCTSTR pszFile, bool bExact = true) const;
        bool FileExists(LPCTSTR pszShare, LPCTSTR pszFile, bool bExact = true) const;
        bool FileExists(LPCTSTR pszFullPath, bool bExact = true) const;

        //
        // Create iterators for enumerating collection contents.
        //
        ShareIter CreateShareIterator(void) const;
        FileIter CreateFileIterator(HSHARE& hShare) const;
        //
        // Create/free a byte buffer containing the contents of the collection.
        //
        PCSC_NAMELIST_HDR CreateListBuffer(void) const;
        static void FreeListBuffer(PCSC_NAMELIST_HDR pbNames);
        //
        // Check after initializing object from byte buffer.
        //
        bool IsValid(void) const
            { return m_bValid; }

#ifdef FILELIST_TEST
        void Dump(void) const;
        void DumpListBuffer(PCSC_NAMELIST_HDR pbBuffer) const;
#endif // FILELIST_TEST


    private:
        // --------------------------------------------------------------------
        // CscFilenameList::NamePtr
        //
        // Simple wrapper around a string pointer to add a notion of "ownership".
        // This lets us store a string address as either a pointer to dynamic
        // heap memory (that later must be freed) or the address of a string
        // in a character buffer owned by someone else (owner frees it if 
        // necessary).
        //
        class NamePtr
        {
            public:
                NamePtr(void)
                    : m_pszName(NULL), 
                      m_bOwns(false) { }

                NamePtr(LPCTSTR pszName, bool bCopy);
                ~NamePtr(void);

                NamePtr(NamePtr& rhs);
                NamePtr& operator = (NamePtr& rhs);

                bool IsValid(void) const
                    { return NULL != m_pszName; }

                operator LPCTSTR () const
                    { return m_pszName; }

            private:
                LPCTSTR m_pszName; // address of string.
                bool    m_bOwns;   // do we need to free it on destruction?

                friend class CscFilenameList;
                friend class Share;
        };


        // --------------------------------------------------------------------
        // CscFilenameList::Share
        //
        // Represents a share in the namelist collection.  It's really just
        // a convenient container for a share name and a list of file names
        // associated with the share.
        //
        class Share
        {
            public:
                Share(LPCTSTR pszShare, bool bCopy = true);
                ~Share(void);

                bool AddFile(LPCTSTR pszFile, bool bDirectory = false, bool bCopy = true);
                bool RemoveFile(LPCTSTR pszFile);

                int FileCount(void) const
                    { return m_cFiles; }

                int ByteCount(void) const
                    { return (m_cchShareName + m_cchFileNames) * sizeof(TCHAR); }

                int Write(LPBYTE pbBufferStart, 
                          CSC_NAMELIST_SHARE_DESC *pDesc, 
                          LPTSTR pszBuffer, 
                          int cchBuffer) const;

#ifdef FILELIST_TEST
                void Dump(void) const;
#endif // FILELIST_TEST

            private:
                int    m_cFiles;          // Cnt of files in share.
                int    m_cAllocated;      // Cnt of share ptrs allocated.
                int    m_cchShareName;    // Bytes req'd to hold share name.
                int    m_cchFileNames;    // Bytes req'd to hold file names.
                NamePtr m_pszShareName;    // Address of share name string.
                NamePtr *m_rgpszFileNames; // Array of ptrs to file name strings
                static int m_cGrow;       // File name array growth increment.

                int WriteFileNames(LPTSTR pszBuffer, int cchBuffer) const;
                int WriteName(LPTSTR pszBuffer, int cchBuffer) const;
                bool GrowFileNamePtrList(void);

                friend class CscFilenameList;
                friend class FileIter;
        };

        // --------------------------------------------------------------------
        // Namelist object private members.
        //
        int     m_cShares;    // How many shares in collection.
        int     m_cAllocated; // Allocated size of m_rgpShares[].
        Share **m_rgpShares;  // Dynamic array of ptrs to Share objects.
        bool    m_bValid;     // Ctor completion check.
        static int m_cGrow;   // How much to grow array when necessary.

        //
        // Prevent copy.
        //
        CscFilenameList(const CscFilenameList& rhs);
        CscFilenameList& operator = (const CscFilenameList& rhs);

        bool GrowSharePtrList(void);
        bool LoadFromBuffer(PCSC_NAMELIST_HDR pbBuffer, bool bCopy);

        void ParseFullPath(LPTSTR pszFullPath, 
                           LPTSTR *ppszShare, 
                           LPTSTR *ppszFile) const;

        static bool Compare(LPCTSTR pszTemplate, LPCTSTR pszFile, bool *pbExact);

        friend class ShareIter;
        friend class Share;
};

#endif // _INC_CSCVIEW_FILELIST_H

