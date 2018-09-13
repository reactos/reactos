//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       filelist.cpp
//
//--------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
/*  File: filelist.cpp

    Description: Simplifies the transmission of a list of share and 
        associated file names between components of the CSC UI.  See
        description in filelist.h for details.

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
#include "pch.h"
#pragma hdrstop

#include "filelist.h"

#ifdef FILELIST_TEST
#include <stdio.h>
#include <tchar.h>
#endif


static LPTSTR DupStr(LPCTSTR psz)
{
    LPTSTR pszNew = new TCHAR[lstrlen(psz) + 1];
    if (NULL != pszNew)
    {
        lstrcpy(pszNew, psz);
    }
    return pszNew;
}


int CscFilenameList::m_cGrow        =  4;
int CscFilenameList::Share::m_cGrow = 10;


CscFilenameList::CscFilenameList(
    void
    ) : m_cShares(0),
        m_cAllocated(0),
        m_rgpShares(NULL),
        m_bValid(true)
{

}



CscFilenameList::CscFilenameList(
    PCSC_NAMELIST_HDR pbNames,
    bool bCopy
    ) : m_cShares(0),
        m_cAllocated(0),
        m_rgpShares(NULL),
        m_bValid(true)
{
    m_bValid = LoadFromBuffer(pbNames, bCopy);
}



CscFilenameList::~CscFilenameList(
    void
    )
{
    for (int i = 0; i < m_cShares; i++)
    {
        delete m_rgpShares[i];
    }
    delete[] m_rgpShares;
}



bool
CscFilenameList::LoadFromBuffer(
    PCSC_NAMELIST_HDR pHdr,
    bool bCopy
    )
{
    LPBYTE pbBuffer = reinterpret_cast<LPBYTE>(pHdr);
    CSC_NAMELIST_SHARE_DESC *pShareDesc = (CSC_NAMELIST_SHARE_DESC *)(pbBuffer + sizeof(CSC_NAMELIST_HDR));
    for (UINT i = 0; i < pHdr->cShares; i++)
    {
        LPCTSTR pszShareName = (LPCTSTR)(pbBuffer + pShareDesc->cbOfsShareName);
        LPCTSTR pszFileName  = (LPCTSTR)(pbBuffer + pShareDesc->cbOfsFileNames);
        HSHARE hShare;
        
        if (!AddShare(pszShareName, &hShare, bCopy))
        {
            return false; // memory allocation failure.
        }
        else
        {
            for (UINT j = 0; j < pShareDesc->cFiles; j++)
            {
                //
                // Note that we always pass "false" for the bDirectory
                // argument to AddFile().  Passing true causes the
                // string "\*" to be appended to the filename stored
                // in the collection.  Since we're getting names from
                // an existing namelist buffer, directories will already
                // have the "\*" appended.  We don't want to append a 
                // second instance.
                //
                if (!AddFile(hShare, pszFileName, false, bCopy))
                {
                    return false; // memory allocation failure.
                }
                pszFileName += (lstrlen(pszFileName) + 1);
            }
        }
        pShareDesc++;
    }
    return true;
}



bool
CscFilenameList::AddShare(
    LPCTSTR pszShare,
    HSHARE *phShare,
    bool bCopy
    )
{
    bool bResult = true;
    if (m_cShares == m_cAllocated)
        bResult = GrowSharePtrList();

    if (bResult)
    {
        Share *pShare = new Share(pszShare, bCopy);
        if (NULL != pShare)
        {
            m_rgpShares[m_cShares++] = pShare;
            *phShare = HSHARE(pShare);
            bResult = true;
        }
    }
    return bResult;
}



bool
CscFilenameList::AddFile(
    HSHARE& hShare, 
    LPCTSTR pszFile,
    bool bDirectory,
    bool bCopy
    )
{
    return hShare.m_pShare->AddFile(pszFile, bDirectory, bCopy);
}


//
// Given a full UNC path name, separate the share and file names with
// a nul character and return the address of the share and file name
// parts.  Note that the buffer pointed to by pszFullPath is modified.
//
void
CscFilenameList::ParseFullPath(
    LPTSTR pszFullPath,
    LPTSTR *ppszShare,
    LPTSTR *ppszFile
    ) const
{
    *ppszShare = NULL;
    *ppszFile = pszFullPath;

    LPTSTR psz = pszFullPath;
    if (*psz && *psz == TEXT('\\'))
    {
        psz++;
        if (*psz && *psz == TEXT('\\'))
        {
            *ppszShare = pszFullPath; // Assume a share name.
            *ppszFile  = NULL;

            psz++;
            while(*psz && *psz != TEXT('\\'))
            {
                psz = CharNext(psz);
            }
            if (*psz)
                psz = CharNext(psz);
            while(*psz && *psz != TEXT('\\'))
            {
                psz = CharNext(psz);
            }
            if (*psz)
            {
                *ppszFile = CharNext(psz);
                *psz = TEXT('\0');
            }
        }
    }
}


bool
CscFilenameList::AddFile(
    LPCTSTR pszFullPath,
    bool bDirectory, 
    bool bCopy
    )
{
    bool bResult = false;
    LPTSTR pszShare      = NULL;
    LPTSTR pszFile       = NULL;
    LPTSTR pszParsedPath = DupStr(pszFullPath);

    if (NULL != pszParsedPath)
    {
        TCHAR szBackslash[] = TEXT("\\");
        ParseFullPath(pszParsedPath, &pszShare, &pszFile);
        if (NULL == pszFile || TEXT('\0') == *pszFile)
        {
            //
            // Path was just a share name with no file or subdirectory.
            //
            pszFile = szBackslash;
        }

        if (NULL != pszShare)
        {
            bResult = AddFile(pszShare, pszFile, bDirectory, bCopy);
        }
        delete[] pszParsedPath;
    }
    return bResult;
}


bool
CscFilenameList::AddFile(
    LPCTSTR pszShare, 
    LPCTSTR pszFile,
    bool bDirectory, 
    bool bCopy
    )
{
    HSHARE hShare;
    if (!GetShareHandle(pszShare, &hShare))
    {
        if (!AddShare(pszShare, &hShare, bCopy))
        {
            return false; // memory allocation failure.
        }
    }
    return AddFile(hShare, pszFile, bDirectory, bCopy);
}


bool
CscFilenameList::RemoveFile(
    HSHARE& hShare, 
    LPCTSTR pszFile
    )
{
    return hShare.m_pShare->RemoveFile(pszFile);
}


bool
CscFilenameList::RemoveFile(
    LPCTSTR pszFullPath
    )
{
    bool bResult = false;
    LPTSTR pszShare      = NULL;
    LPTSTR pszFile       = NULL;
    LPTSTR pszParsedPath = DupStr(pszFullPath);

    if (NULL != pszParsedPath)
    {
        TCHAR szBackslash[] = TEXT("\\");
        ParseFullPath(pszParsedPath, &pszShare, &pszFile);
        if (NULL == pszFile || TEXT('\0') == *pszFile)
        {
            //
            // Path was just a share name with no file or subdirectory.
            //
            pszFile = szBackslash;
        }

        if (NULL != pszShare)
        {
            bResult = RemoveFile(pszShare, pszFile);
        }
        delete[] pszParsedPath;
    }
    return bResult;
}


bool
CscFilenameList::RemoveFile(
    LPCTSTR pszShare, 
    LPCTSTR pszFile
    )
{
    HSHARE hShare;
    if (!GetShareHandle(pszShare, &hShare))
    {
        return false; // doesn't exist
    }
    return RemoveFile(hShare, pszFile);
}


LPCTSTR 
CscFilenameList::GetShareName(
    HSHARE& hShare
    ) const
{
    return static_cast<LPCTSTR>(hShare.m_pShare->m_pszShareName);
}


int 
CscFilenameList::GetShareCount(
    void
    ) const
{
    return m_cShares;
}



int 
CscFilenameList::GetFileCount(
    void
    ) const
{
    int cFiles = 0;
    ShareIter si = CreateShareIterator();
    HSHARE hShare;
    while(si.Next(&hShare))
    {
        cFiles += GetShareFileCount(hShare);
    }
    return cFiles;
}



int 
CscFilenameList::GetShareFileCount(
    HSHARE& hShare
    ) const
{
    return hShare.m_pShare->m_cFiles;
}



bool 
CscFilenameList::ShareExists(
    LPCTSTR pszShare
    ) const
{
    HSHARE hShare;
    return GetShareHandle(pszShare, &hShare);
}


//
// Replacement for lstrcmpi that adds a little twist for the
// filename list.
// If the *pbExact argument [in] is false AND if the s1 
// string argument is appended with "\*", it is assumed to be a 
// directory name and all descendents of that directory produce a 
// match.  *pbExact is modified to indicate if the match was
// an exact match or a wildcard match as just described.
// If *pbExact is false on entry, the function works just like
// lstrcmpi except that the return value is true/false instead
// of <0, 0, >0.
//
bool
CscFilenameList::Compare(
    LPCTSTR s1,
    LPCTSTR s2,
    bool *pbExact
    )
{
    LPCTSTR s1First = s1;
    bool bMatch = false;

    TraceAssert((NULL != pbExact));

    while(*s1 || *s2)
    {
        //
        // Do a case-insensitive comparison.
        //
        if (PtrToUlong(CharUpper((LPTSTR)(*s1))) == PtrToUlong(CharUpper((LPTSTR)(*s2))))
        {
            s1 = CharNext(s1);
            s2 = CharNext(s2);
        }
        else
        {
            if (!(*pbExact))
            {
                //
                // An exact match is not required.  Now check for 
                // wildcard match.
                //
                if (TEXT('\0') == *s2 &&
                    TEXT('\\') == *s1 &&
                    TEXT('*')  == *(s1+1))
                {
                    //
                    // At end of key string provided by user.
                    // We have a match if the string being tested
                    // contains "\*" at current test location.
                    //
                    // i.e. "foo\bar" matches "foo\bar\*"
                    // 
                    bMatch = true;
                    *pbExact = false;
                }
                else if (TEXT('*') == *s1)
                {
                    //
                    // We hit a '*' in the stored string.
                    // Since we've matched up to this point, the
                    // user's string is an automatic match.
                    // 
                    bMatch = TEXT('\0') == *(s1+1);
                    *pbExact = false;
                }
            }
            goto return_result;
        }
    }
    if (TEXT('\0') == *s1 && TEXT('\0') == *s2)
    {
        //
        // Exact match.
        //
        *pbExact = bMatch = true;
        goto return_result;
    }

return_result:
    return bMatch;
}


bool 
CscFilenameList::FileExists(
    HSHARE& hShare, 
    LPCTSTR pszFile,
    bool bExact     // Default == true
    ) const
{
    FileIter fi = CreateFileIterator(hShare);
    //
    // Skip past any leading backslashes for matching purposes.
    // File names (paths) stored in the filename list don't have
    // leading backslashes.  It's implied as the root directory.
    //
    while(*pszFile && TEXT('\\') == *pszFile)
        pszFile = CharNext(pszFile);

    LPCTSTR psz;
    while(NULL != (psz = fi.Next()))
    {
        bool bExactResult = bExact; 
        if (Compare(psz, pszFile, &bExactResult)) // Modifies bExactResult.
        {
            return !bExact || (bExact && bExactResult);
        }
    }
    return false;
}


bool 
CscFilenameList::FileExists(
    LPCTSTR pszShare, 
    LPCTSTR pszFile,
    bool bExact      // Default == true
    ) const
{
    HSHARE hShare;
    return (GetShareHandle(pszShare, &hShare) &&
            FileExists(hShare, pszFile, bExact));
}



bool
CscFilenameList::FileExists(
    LPCTSTR pszFullPath,
    bool bExact          // Default = true
    ) const
{
    bool bResult         = false;
    LPTSTR pszShare      = NULL;
    LPTSTR pszFile       = NULL;
    LPTSTR pszParsedPath = DupStr(pszFullPath);
    if (NULL != pszParsedPath)
    {
        ParseFullPath(pszParsedPath, &pszShare, &pszFile);
        if (NULL != pszShare && NULL != pszFile)
        {
            bResult = FileExists(pszShare, pszFile, bExact);
        }
        delete[] pszParsedPath;
    }
    return bResult;
}



CscFilenameList::FileIter 
CscFilenameList::CreateFileIterator(
    HSHARE& hShare
    ) const
{
    return FileIter(hShare.m_pShare);
}



CscFilenameList::ShareIter
CscFilenameList::CreateShareIterator(
    void
    ) const
{
    return ShareIter(*this);
}



bool
CscFilenameList::GetShareHandle(
    LPCTSTR pszShare,
    HSHARE *phShare
    ) const
{
    Share *pShare = NULL;
    for (int i = 0; i < m_cShares; i++)
    {
        pShare = m_rgpShares[i];
        if (pShare->m_pszShareName.IsValid() && 0 == lstrcmpi(pszShare, pShare->m_pszShareName))
        {
            *phShare = HSHARE(pShare);
            return true;
        }
    }
    return false;
}



bool
CscFilenameList::GrowSharePtrList(
    void
    )
{
    Share **rgpShares = new Share *[m_cAllocated + m_cGrow];
    if (NULL != rgpShares)
    {
        if (NULL != m_rgpShares) 
            CopyMemory(rgpShares, m_rgpShares, m_cAllocated * sizeof(Share *));
        delete[] m_rgpShares;
        m_rgpShares = rgpShares;
        m_cAllocated += m_cGrow;
    }
    return (NULL != rgpShares);
}


//
// Create a memory buffer to hold the contents of the name list.
// The buffer is formatted as described in the header of filelist.h.
// 
PCSC_NAMELIST_HDR
CscFilenameList::CreateListBuffer(
    void
    ) const
{
    int i;

    //
    // Calculate the required buffer size and allocate buffer.
    //
    int cbOfsNames = sizeof(CSC_NAMELIST_HDR) +
                     sizeof(CSC_NAMELIST_SHARE_DESC) * m_cShares;
    int cbBuffer = cbOfsNames;

    for (i = 0; i < m_cShares; i++)
    {
        cbBuffer += m_rgpShares[i]->ByteCount();
    }

    LPBYTE pbBuffer = new BYTE[cbBuffer];
    PCSC_NAMELIST_HDR pHdr = reinterpret_cast<PCSC_NAMELIST_HDR>(pbBuffer);
    if (NULL != pbBuffer)
    {
        LPTSTR pszNames = reinterpret_cast<LPTSTR>(pbBuffer + cbOfsNames);
        CSC_NAMELIST_SHARE_DESC *pShareDescs = reinterpret_cast<CSC_NAMELIST_SHARE_DESC *>(pbBuffer + sizeof(CSC_NAMELIST_HDR));
        int cchNames = (cbBuffer - cbOfsNames) / sizeof(TCHAR);
        //
        // Write the share and file name strings.
        //
        for (i = 0; i < m_cShares; i++)
        {
            CSC_NAMELIST_SHARE_DESC *pDesc = pShareDescs + i;
            Share *pShare = m_rgpShares[i];

            int cch = pShare->Write(pbBuffer, 
                                    pDesc,
                                    pszNames,
                                    cchNames);
            cchNames -= cch;
            pszNames += cch;
        }
        //
        // Fill in the buffer header.
        // BUGBUG:  A checksum might be good to verify buffer integrity later.
        //
        pHdr->cbSize  = cbBuffer;
        pHdr->cShares = m_cShares;
    }

    return pHdr;
}


        
void
CscFilenameList::FreeListBuffer(
    PCSC_NAMELIST_HDR pbNames
    )
{
    delete[] pbNames;
}

#ifdef FILELIST_TEST
void
CscFilenameList::Dump(
    void
    ) const
{
    _tprintf(TEXT("Dump share name list at 0x%08X\n"), (DWORD)this);
    _tprintf(TEXT("\tm_cShares......: %d\n"), m_cShares);
    _tprintf(TEXT("\tm_cAllocated...: %d\n"), m_cAllocated);

   
    for (int i = 0; i < m_cShares; i++)
    {
        m_rgpShares[i]->Dump();
    }
}

void
CscFilenameList::DumpListBuffer(
    PCSC_NAMELIST_HDR pHdr
    ) const
{
    LPBYTE pbBuffer = (LPBYTE)pHdr;
    _tprintf(TEXT("Dump buffer at 0x%08X\n"), (DWORD)pbBuffer);
    _tprintf(TEXT("hdr.cbSize......: %d\n"), pHdr->cbSize); 
    _tprintf(TEXT("hdr.flags.......: %d\n"), pHdr->flags); 
    _tprintf(TEXT("hdr.cShares.....: %d\n"), pHdr->cShares);
    CSC_NAMELIST_SHARE_DESC *pDesc = (CSC_NAMELIST_SHARE_DESC *)(pbBuffer + sizeof(CSC_NAMELIST_HDR));
    for (UINT i = 0; i < pHdr->cShares; i++)
    {
        _tprintf(TEXT("\tShare [%d] header\n"), i);
        _tprintf(TEXT("\t\tcbOfsShareName..:%d\n"), pDesc->cbOfsShareName);
        _tprintf(TEXT("\t\tcbOfsFileNames..:%d\n"), pDesc->cbOfsFileNames);
        _tprintf(TEXT("\t\tcFiles..........:%d\n"), pDesc->cFiles);
        LPTSTR pszName = (LPTSTR)(pbBuffer + pDesc->cbOfsShareName);
        _tprintf(TEXT("\t\tShare name......: \"%s\"\n"), pszName);
        pszName += lstrlen(pszName) + 1;
        for (UINT j = 0; j < pDesc->cFiles; j++)
        {
            _tprintf(TEXT("\t\tFile[%3d] name...: \"%s\"\n"), j, pszName);
            pszName += lstrlen(pszName) + 1;
        }
        pDesc++;
    }
}

#endif // FILELIST_TEST

CscFilenameList::Share::Share(
    LPCTSTR pszShare,
    bool bCopy
    ) : m_pszShareName(pszShare, bCopy),
        m_cFiles(0),
        m_cAllocated(0),
        m_cchFileNames(0),
        m_cchShareName(lstrlen(pszShare) + 1),
        m_rgpszFileNames(NULL)
{


}



CscFilenameList::Share::~Share(
    void
    )
{
    delete[] m_rgpszFileNames;
}



bool
CscFilenameList::Share::AddFile(
    LPCTSTR pszFile,
    bool bDirectory,
    bool bCopy
    )
{
    bool bResult = true;

    TraceAssert((NULL != pszFile && TEXT('\0') != *pszFile));
    if (NULL == pszFile || TEXT('\0') == *pszFile)
        return false;

    if (m_cFiles == m_cAllocated)
        bResult = GrowFileNamePtrList();

    if (bResult)
    {
        LPTSTR pszFileCopy = NULL;
        if (bDirectory)
        {
            int cchFile = lstrlen(pszFile);
            pszFileCopy = new TCHAR[cchFile + 3]; // length + "\*" + nul
            if (NULL == pszFileCopy)
                return false; // memory alloc failure.
            //
            // Append "\*" to a directory entry.
            // This will allow us to do lookups on files that
            // are descendants of a directory.  See
            // CscFilenameList::FileExists() for details.
            //
            lstrcpy(pszFileCopy, pszFile);
            if (TEXT('\\') == *(pszFileCopy + cchFile - 1))
            {
                //
                // Guard against pszFile already having a trailing backslash.
                //
                cchFile--;
            }
            lstrcpy(pszFileCopy + cchFile, TEXT("\\*"));
            pszFile = pszFileCopy;
        }

        //
        // Skip past any leading backslash.
        //
        while(*pszFile && TEXT('\\') == *pszFile)
            pszFile = CharNext(pszFile);

        NamePtr np(pszFile, bCopy);
        if (bResult = np.IsValid())
        {
            m_rgpszFileNames[m_cFiles++] = np;
            m_cchFileNames += (lstrlen(pszFile) + 1);
        }
        delete[] pszFileCopy;
    }
    return bResult;
}



bool
CscFilenameList::Share::RemoveFile(
    LPCTSTR pszFile
    )
{
    TraceAssert((NULL != pszFile && TEXT('\0') != *pszFile));
    if (NULL == pszFile || TEXT('\0') == *pszFile)
        return false;

    //
    // Skip past any leading backslashes for matching purposes.
    // File names (paths) stored in the filename list don't have
    // leading backslashes.  It's implied as the root directory.
    //
    while(*pszFile && TEXT('\\') == *pszFile)
        pszFile = CharNext(pszFile);

    for(int i = 0; i < m_cFiles; i++)
    {
        bool bExactResult = true; 
        if (Compare(m_rgpszFileNames[i], pszFile, &bExactResult)
            && bExactResult)
        {
            // Found an exact match. Move the last file into the
            // current array location and decrement the count.
            m_rgpszFileNames[i] = m_rgpszFileNames[--m_cFiles];
            return true;
        }
    }
    return false;
}



bool
CscFilenameList::Share::GrowFileNamePtrList(
    void
    )
{
    CscFilenameList::NamePtr *rgpsz = new CscFilenameList::NamePtr[m_cAllocated + m_cGrow];
    if (NULL != rgpsz)
    {
        m_cAllocated += m_cGrow;
        if (NULL != m_rgpszFileNames) 
        {
            for (int i = 0; i < m_cFiles; i++)
            {
                rgpsz[i] = m_rgpszFileNames[i];
            }
        }
        delete[] m_rgpszFileNames;
        m_rgpszFileNames = rgpsz;
    }
    return (NULL != rgpsz);
}


//
// Write the share name and file names to a text buffer.
//
int
CscFilenameList::Share::Write(
    LPBYTE pbBufferStart,             // Address of buffer start.
    CSC_NAMELIST_SHARE_DESC *pShare,  // Address of share descriptor.
    LPTSTR pszBuffer,                 // Address of name buffer
    int cchBuffer                     // Chars left in name buffer.
    ) const
{
    pShare->cbOfsShareName = (DWORD)((LPBYTE)pszBuffer - pbBufferStart);
    int cch = WriteName(pszBuffer, cchBuffer);
    cchBuffer -= cch;
    pszBuffer += cch;
    pShare->cbOfsFileNames = (DWORD)((LPBYTE)pszBuffer - pbBufferStart);
    cch += WriteFileNames(pszBuffer, cchBuffer);
    pShare->cFiles = m_cFiles;
    return cch;
}



//
// Write the share name to a text buffer.
//
int
CscFilenameList::Share::WriteName(
    LPTSTR pszBuffer,
    int cchBuffer
    ) const
{
    if (m_pszShareName.IsValid() && m_cchShareName <= cchBuffer)
    {
        lstrcpy(pszBuffer, m_pszShareName);
        return m_cchShareName;
    }
    return 0;
}



//
// Write the file names to a text buffer.
//
int
CscFilenameList::Share::WriteFileNames(
    LPTSTR pszBuffer, 
    int cchBuffer
    ) const
{
    int cchWritten = 0;
    if (m_cchFileNames <= cchBuffer)
    {   
        for (int i = 0; i < m_cFiles; i++)
        {
            
            lstrcpy(pszBuffer, m_rgpszFileNames[i]);
            int cch = (lstrlen(m_rgpszFileNames[i]) + 1);
            pszBuffer += cch;
            cchWritten += cch;
        }
    }
    return cchWritten;
}

#ifdef FILELIST_TEST

void
CscFilenameList::Share::Dump(
    void
    ) const
{
    _tprintf(TEXT("Share \"%s\"\n"), (LPCTSTR)m_pszShareName ? (LPCTSTR)m_pszShareName : TEXT("<null>"));
    _tprintf(TEXT("\tm_cFiles........: %d\n"), m_cFiles);
    _tprintf(TEXT("\tm_cAllocated....: %d\n"), m_cAllocated);
    _tprintf(TEXT("\tm_cchShareName..: %d\n"), m_cchShareName);
    _tprintf(TEXT("\tm_cchFileNames..: %d\n"), m_cchFileNames);
    for (int i = 0; i < m_cFiles; i++)
    {
        _tprintf(TEXT("\tFile[%3d].......: \"%s\"\n"), i, (LPCTSTR)m_rgpszFileNames[i] ? (LPCTSTR)m_rgpszFileNames[i] : TEXT("<null>"));
    }
}

#endif // FILELIST_TEST


CscFilenameList::FileIter::FileIter(
    const CscFilenameList::Share *pShare
    ) : m_pShare(pShare),
        m_iFile(0)
{

}



CscFilenameList::FileIter::FileIter(
    void
    ) : m_pShare(NULL),
        m_iFile(0)
{

}



CscFilenameList::FileIter::FileIter(
    const CscFilenameList::FileIter& rhs
    )
{
    *this = rhs;
}



CscFilenameList::FileIter& 
CscFilenameList::FileIter::operator = (
    const CscFilenameList::FileIter& rhs
    )
{
    if (this != &rhs)
    {
        m_pShare = rhs.m_pShare;
        m_iFile  = rhs.m_iFile;
    }
    return *this;
}



LPCTSTR 
CscFilenameList::FileIter::Next(
    void
    )
{
    if (0 < m_pShare->m_cFiles && m_iFile < m_pShare->m_cFiles)
        return m_pShare->m_rgpszFileNames[m_iFile++];
    return NULL;
}



void
CscFilenameList::FileIter::Reset(
    void
    )
{
    m_iFile = 0;
}



CscFilenameList::ShareIter::ShareIter(
    const CscFilenameList& fnl
    ) : m_pfnl(&fnl),
        m_iShare(0)
{

}



CscFilenameList::ShareIter::ShareIter(
    void
    ) : m_pfnl(NULL),
        m_iShare(0)
{

}



CscFilenameList::ShareIter::ShareIter(
    const CscFilenameList::ShareIter& rhs
    )
{
    *this = rhs;
}



CscFilenameList::ShareIter& 
CscFilenameList::ShareIter::operator = (
    const CscFilenameList::ShareIter& rhs
    )
{
    if (this != &rhs)
    {
        m_pfnl   = rhs.m_pfnl;
        m_iShare = rhs.m_iShare;
    }
    return *this;
}



bool
CscFilenameList::ShareIter::Next(
    HSHARE *phShare
    )
{
    if (0 < m_pfnl->m_cShares && m_iShare < m_pfnl->m_cShares)
    {
        *phShare = HSHARE(m_pfnl->m_rgpShares[m_iShare++]);
        return true;
    }
    return false;
}



void
CscFilenameList::ShareIter::Reset(
    void
    )
{
    m_iShare = 0;
}



CscFilenameList::NamePtr::NamePtr(
    LPCTSTR pszName,
    bool bCopy
    ) : m_pszName(pszName),
        m_bOwns(bCopy)
{
    if (bCopy)
    {
        m_pszName = DupStr(pszName);
        m_bOwns = true;
    }
}



CscFilenameList::NamePtr::~NamePtr(
    void
    )
{
    if (m_bOwns)
    {
        delete[] const_cast<LPTSTR>(m_pszName);
    }
}



CscFilenameList::NamePtr::NamePtr(
    CscFilenameList::NamePtr& rhs
    )
{
    *this = rhs;
}



CscFilenameList::NamePtr& 
CscFilenameList::NamePtr::operator = (
    CscFilenameList::NamePtr& rhs
    )
{
    if (this != &rhs)
    {
        if (m_bOwns)
            delete[] (LPTSTR)m_pszName;

        m_pszName = rhs.m_pszName;
        m_bOwns   = false;
        if (rhs.m_bOwns)
        {
            //
            // Assume ownership of the buffer.
            //
            rhs.m_bOwns = false;
            m_bOwns     = true;
        }
    }
    return *this;
}



CscFilenameList::HSHARE::HSHARE(
    void
    ) : m_pShare(NULL)
{

}



CscFilenameList::HSHARE::HSHARE(
    const HSHARE& rhs
    )
{
    *this = rhs;
}



CscFilenameList::HSHARE& 
CscFilenameList::HSHARE::operator = (
    const CscFilenameList::HSHARE& rhs
    )
{
    if (this != &rhs)
    {
        m_pShare = rhs.m_pShare;
    }
    return *this;
}
