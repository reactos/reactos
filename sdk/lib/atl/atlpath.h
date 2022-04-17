// PROJECT:        ReactOS ATL CPathT
// LICENSE:        Public Domain
// PURPOSE:        Provides compatibility to Microsoft ATL
// PROGRAMMERS:    Benedikt Freisen

#ifndef __ATLPATH_H__
#define __ATLPATH_H__

#pragma once

#include <atlcore.h>
#include <atlstr.h>
#include <shlwapi.h>

namespace ATL
{

template<typename StringType>
class CPathT
{
    // const
    inline BOOL PathFileExistsX(LPCSTR pszPath) const { return PathFileExistsA(pszPath); }
    inline BOOL PathFileExistsX(LPCWSTR pszPath) const { return PathFileExistsW(pszPath); }
    inline LPCSTR PathFindExtensionX(LPCSTR pszPath) const { return PathFindExtensionA(pszPath); }
    inline LPCWSTR PathFindExtensionX(LPCWSTR pszPath) const { return PathFindExtensionW(pszPath); }
    inline LPCSTR PathFindFileNameX(LPCSTR pszPath) const { return PathFindFileNameA(pszPath); }
    inline LPCWSTR PathFindFileNameX(LPCWSTR pszPath) const { return PathFindFileNameW(pszPath); }
    inline int PathGetDriveNumberX(LPCSTR pszPath) const { return PathGetDriveNumberA(pszPath); }
    inline int PathGetDriveNumberX(LPCWSTR pszPath) const { return PathGetDriveNumberW(pszPath); }
    inline BOOL PathIsDirectoryX(LPCSTR pszPath) const { return PathIsDirectoryA(pszPath); }
    inline BOOL PathIsDirectoryX(LPCWSTR pszPath) const { return PathIsDirectoryW(pszPath); }
    inline BOOL PathIsFileSpecX(LPCSTR pszPath) const { return PathIsFileSpecA(pszPath); }
    inline BOOL PathIsFileSpecX(LPCWSTR pszPath) const { return PathIsFileSpecW(pszPath); }
    inline BOOL PathIsPrefixX(LPCSTR pszPath, LPCSTR pszPrefix) { return PathIsPrefixA(pszPath, pszPrefix); }
    inline BOOL PathIsPrefixX(LPCWSTR pszPath, LPCWSTR pszPrefix) { return PathIsPrefixW(pszPath, pszPrefix); }
    inline BOOL PathIsRelativeX(LPCSTR pszPath) const { return PathIsRelativeA(pszPath); }
    inline BOOL PathIsRelativeX(LPCWSTR pszPath) const { return PathIsRelativeW(pszPath); }
    inline BOOL PathIsRootX(LPCSTR pszPath) const { return PathIsRootA(pszPath); }
    inline BOOL PathIsRootX(LPCWSTR pszPath) const { return PathIsRootW(pszPath); }
    inline BOOL PathIsSameRootX(LPCSTR pszPath, LPCSTR pszOther) const { return PathIsSameRootA(pszPath, pszOther); }
    inline BOOL PathIsSameRootX(LPCWSTR pszPath, LPCWSTR pszOther) const { return PathIsSameRootW(pszPath, pszOther); }
    inline BOOL PathIsUNCX(LPCSTR pszPath) { return PathIsUNCA(pszPath); }
    inline BOOL PathIsUNCX(LPCWSTR pszPath) { return PathIsUNCW(pszPath); }
    inline BOOL PathIsUNCServerX(LPCSTR pszPath) { return PathIsUNCServerA(pszPath); }
    inline BOOL PathIsUNCServerX(LPCWSTR pszPath) { return PathIsUNCServerW(pszPath); }
    inline BOOL PathIsUNCServerShareX(LPCSTR pszPath) { return PathIsUNCServerShareA(pszPath); }
    inline BOOL PathIsUNCServerShareX(LPCWSTR pszPath) { return PathIsUNCServerShareW(pszPath); }
    inline BOOL PathMatchSpecX(LPCSTR pszPath, LPCSTR pszSpec) const { return PathMatchSpecA(pszPath, pszSpec); }
    inline BOOL PathMatchSpecX(LPCWSTR pszPath, LPCWSTR pszSpec) const { return PathMatchSpecW(pszPath, pszSpec); }
    inline LPCSTR PathSkipRootX(LPCSTR pszPath) const { return PathSkipRootA(pszPath); }
    inline LPCWSTR PathSkipRootX(LPCWSTR pszPath) const { return PathSkipRootW(pszPath); }

    // non-const
    inline void PathAddBackslashX(LPSTR pszPath) { PathAddBackslashA(pszPath); }
    inline void PathAddBackslashX(LPWSTR pszPath) { PathAddBackslashW(pszPath); }
    inline BOOL PathAddExtensionX(LPSTR pszPath, LPCSTR pszExt) { return PathAddExtensionA(pszPath, pszExt); }
    inline BOOL PathAddExtensionX(LPWSTR pszPath, LPCWSTR pszExt) { return PathAddExtensionW(pszPath, pszExt); }
    inline BOOL PathAppendX(LPSTR pszPath, LPCSTR pszMore) { return PathAppendA(pszPath, pszMore); }
    inline BOOL PathAppendX(LPWSTR pszPath, LPCWSTR pszMore) { return PathAppendW(pszPath, pszMore); }
    inline void PathBuildRootX(LPSTR pszRoot, int iDrive) { PathBuildRootA(pszRoot, iDrive); }
    inline void PathBuildRootX(LPWSTR pszRoot, int iDrive) { PathBuildRootW(pszRoot, iDrive); }
    inline void PathCanonicalizeX(LPSTR pszDst, LPCSTR pszSrc) { PathCanonicalizeA(pszDst, pszSrc); }
    inline void PathCanonicalizeX(LPWSTR pszDst, LPCWSTR pszSrc) { PathCanonicalizeW(pszDst, pszSrc); }
    inline void PathCombineX(LPSTR pszPathOut, LPCSTR pszPathIn, LPCSTR pszMore) { PathCombineA(pszPathOut, pszPathIn, pszMore); }
    inline void PathCombineX(LPWSTR pszPathOut, LPCWSTR pszPathIn, LPCWSTR pszMore) { PathCombineW(pszPathOut, pszPathIn, pszMore); }
    inline CPathT<StringType> PathCommonPrefixX(LPCSTR pszFile1, LPCSTR pszFile2, LPSTR pszPath) { return PathCommonPrefixA(pszFile1, pszFile2, pszPath); }
    inline CPathT<StringType> PathCommonPrefixX(LPCWSTR pszFile1, LPCWSTR pszFile2, LPWSTR pszPath) { return PathCommonPrefixW(pszFile1, pszFile2, pszPath); }
    inline BOOL PathCompactPathX(HDC hDC, LPSTR pszPath, UINT dx) { return PathCompactPathA(hDC, pszPath, dx); }
    inline BOOL PathCompactPathX(HDC hDC, LPWSTR pszPath, UINT dx) { return PathCompactPathW(hDC, pszPath, dx); }
    inline BOOL PathCompactPathExX(LPSTR pszOut, LPCSTR pszSrc, UINT cchMax, DWORD dwFlags) { return PathCompactPathExA(pszOut, pszSrc, cchMax, dwFlags); }
    inline BOOL PathCompactPathExX(LPWSTR pszOut, LPCWSTR pszSrc, UINT cchMax, DWORD dwFlags) { return PathCompactPathExW(pszOut, pszSrc, cchMax, dwFlags); }
    inline BOOL PathMakePrettyX(LPSTR pszPath) { return PathMakePrettyA(pszPath); }
    inline BOOL PathMakePrettyX(LPWSTR pszPath) { return PathMakePrettyW(pszPath); }
    inline void PathQuoteSpacesX(LPSTR pszPath) { PathQuoteSpacesA(pszPath); }
    inline void PathQuoteSpacesX(LPWSTR pszPath) { PathQuoteSpacesW(pszPath); }
    inline BOOL PathRelativePathToX(LPSTR pszPath, LPCSTR pszFrom, DWORD dwAttrFrom, LPCSTR pszTo, DWORD dwAttrTo) { return PathRelativePathToA(pszPath, pszFrom, dwAttrFrom, pszTo, dwAttrTo); }
    inline BOOL PathRelativePathToX(LPWSTR pszPath, LPCWSTR pszFrom, DWORD dwAttrFrom, LPCWSTR pszTo, DWORD dwAttrTo) { return PathRelativePathToW(pszPath, pszFrom, dwAttrFrom, pszTo, dwAttrTo); }
    inline void PathRemoveArgsX(LPSTR pszPath) { PathRemoveArgsA(pszPath); }
    inline void PathRemoveArgsX(LPWSTR pszPath) { PathRemoveArgsW(pszPath); }
    inline void PathRemoveBackslashX(LPSTR pszPath) { PathRemoveBackslashA(pszPath); }
    inline void PathRemoveBackslashX(LPWSTR pszPath) { PathRemoveBackslashW(pszPath); }
    inline void PathRemoveBlanksX(LPSTR pszPath) { PathRemoveBlanksA(pszPath); }
    inline void PathRemoveBlanksX(LPWSTR pszPath) { PathRemoveBlanksW(pszPath); }
    inline void PathRemoveExtensionX(LPSTR pszPath) { PathRemoveExtensionA(pszPath); }
    inline void PathRemoveExtensionX(LPWSTR pszPath) { PathRemoveExtensionW(pszPath); }
    inline BOOL PathRemoveFileSpecX(LPSTR pszPath) { return PathRemoveFileSpecA(pszPath); }
    inline BOOL PathRemoveFileSpecX(LPWSTR pszPath) { return PathRemoveFileSpecW(pszPath); }
    inline BOOL PathRenameExtensionX(LPSTR pszPath, LPCSTR pszExt) { return PathRenameExtensionA(pszPath, pszExt); }
    inline BOOL PathRenameExtensionX(LPWSTR pszPath, LPCWSTR pszExt) { return PathRenameExtensionW(pszPath, pszExt); }
    inline void PathStripPathX(LPSTR pszPath) { PathStripPathA(pszPath); }
    inline void PathStripPathX(LPWSTR pszPath) { PathStripPathW(pszPath); }
    inline BOOL PathStripToRootX(LPSTR pszPath) { return PathStripToRootA(pszPath); }
    inline BOOL PathStripToRootX(LPWSTR pszPath) { return PathStripToRootW(pszPath); }
    inline void PathUnquoteSpacesX(LPSTR pszPath) { PathUnquoteSpacesA(pszPath); }
    inline void PathUnquoteSpacesX(LPWSTR pszPath) { PathUnquoteSpacesW(pszPath); }

public:
    typedef typename StringType::PCXSTR PCXSTR;
    typedef typename StringType::PXSTR PXSTR;
    typedef typename StringType::XCHAR XCHAR;

    StringType m_strPath;

    CPathT(PCXSTR pszPath)
    {
        m_strPath = StringType(pszPath);
    }

    CPathT(const CPathT<StringType>& path)
    {
        m_strPath = path.m_strPath;
    }

    CPathT() throw()
    {
        // do nothing, m_strPath initializes itself
    }

    void AddBackslash()
    {
        PXSTR str = m_strPath.GetBuffer(MAX_PATH);
        PathAddBackslashX(str);
        m_strPath.ReleaseBuffer();
    }

    BOOL AddExtension(PCXSTR pszExtension)
    {
        PXSTR str = m_strPath.GetBuffer(MAX_PATH);
        BOOL result = PathAddExtensionX(str, pszExtension);
        m_strPath.ReleaseBuffer();
        return result;
    }

    BOOL Append(PCXSTR pszMore)
    {
        PXSTR str = m_strPath.GetBuffer(MAX_PATH);
        BOOL result = PathAppendX(str, pszMore);
        m_strPath.ReleaseBuffer();
        return result;
    }

    void BuildRoot(int iDrive)
    {
        PXSTR str = m_strPath.GetBuffer(4);
        PathBuildRootX(str, iDrive);
        m_strPath.ReleaseBuffer();
    }

    void Canonicalize()
    {
        StringType strTemp;
        PXSTR str = strTemp.GetBuffer(MAX_PATH);
        PathCanonicalizeX(str, m_strPath);
        strTemp.ReleaseBuffer();
        m_strPath = strTemp;
    }

    void Combine(PCXSTR pszDir, PCXSTR pszFile)
    {
        PXSTR str = m_strPath.GetBuffer(MAX_PATH);
        PathCombineX(str, pszDir, pszFile);
        m_strPath.ReleaseBuffer();
    }

    CPathT<StringType> CommonPrefix(PCXSTR pszOther)
    {
        StringType result;
        result.SetString(m_strPath, PathCommonPrefixX(m_strPath, pszOther, NULL));
        return result;
    }

    BOOL CompactPath(HDC hDC, UINT nWidth)
    {
        PXSTR str = m_strPath.GetBuffer(MAX_PATH);
        BOOL result = PathCompactPathX(hDC, str, nWidth);
        m_strPath.ReleaseBuffer();
        return result;
    }

    BOOL CompactPathEx(UINT nMaxChars, DWORD dwFlags = 0)
    {
        StringType strTemp;
        PXSTR str = strTemp.GetBuffer(nMaxChars);
        BOOL result = PathCompactPathExX(str, m_strPath, nMaxChars, dwFlags);
        strTemp.ReleaseBuffer();
        m_strPath = strTemp;
        return result;
    }

    BOOL FileExists() const
    {
        return PathFileExistsX(m_strPath);
    }

    int FindExtension() const
    {
        PCXSTR extension = PathFindExtensionX(m_strPath);
        if (*extension == '\0')
            return -1;

        PCXSTR str = m_strPath;
        return (int)(extension - str);
    }

    int FindFileName() const
    {
        PCXSTR filename = PathFindFileNameX(m_strPath);
        if (*filename == '\0')
            return -1;

        PCXSTR str = m_strPath;
        return (int)(filename - str);
    }

    int GetDriveNumber() const
    {
        return PathGetDriveNumberX(m_strPath);
    }

    StringType GetExtension() const
    {
        return StringType(PathFindExtensionX(m_strPath));
    }

    BOOL IsDirectory() const
    {
        return PathIsDirectoryX(m_strPath);
    }

    BOOL IsFileSpec() const
    {
        return PathIsFileSpecX(m_strPath);
    }

    BOOL IsPrefix(PCXSTR pszPrefix) const
    {
        return PathIsPrefixX(m_strPath);
    }

    BOOL IsRelative() const
    {
        return PathIsRelativeX(m_strPath);
    }

    BOOL IsRoot() const
    {
        return PathIsRootX(m_strPath);
    }

    BOOL IsSameRoot(PCXSTR pszOther) const
    {
        return PathIsSameRootX(m_strPath, pszOther);
    }

    BOOL IsUNC() const
    {
        return PathIsUNCX(m_strPath);
    }

    BOOL IsUNCServer() const
    {
        return PathIsUNCServerX(m_strPath);
    }

    BOOL IsUNCServerShare() const
    {
        return PathIsUNCServerShareX(m_strPath);
    }

    BOOL MakePretty()
    {
        PXSTR str = m_strPath.GetBuffer(MAX_PATH);
        BOOL result = PathMakePrettyX(str);
        m_strPath.ReleaseBuffer();
        return result;
    }

    BOOL MatchSpec(PCXSTR pszSpec) const
    {
        return PathMatchSpecX(m_strPath, pszSpec);
    }

    void QuoteSpaces()
    {
        PXSTR str = m_strPath.GetBuffer(MAX_PATH);
        PathQuoteSpacesX(str);
        m_strPath.ReleaseBuffer();
    }

    BOOL RelativePathTo(PCXSTR pszFrom, DWORD dwAttrFrom, PCXSTR pszTo, DWORD dwAttrTo)
    {
        PXSTR str = m_strPath.GetBuffer(MAX_PATH);
        BOOL result = PathRelativePathToX(str, pszFrom, dwAttrFrom, pszTo, dwAttrTo);
        m_strPath.ReleaseBuffer();
        return result;
    }

    void RemoveArgs()
    {
        PXSTR str = m_strPath.GetBuffer(MAX_PATH);
        PathRemoveArgsX(str);
        m_strPath.ReleaseBuffer();
    }

    void RemoveBackslash()
    {
        PXSTR str = m_strPath.GetBuffer(MAX_PATH);
        PathRemoveBackslashX(str);
        m_strPath.ReleaseBuffer();
    }

    void RemoveBlanks()
    {
        PXSTR str = m_strPath.GetBuffer(MAX_PATH);
        PathRemoveBlanksX(str);
        m_strPath.ReleaseBuffer();
    }

    void RemoveExtension()
    {
        PXSTR str = m_strPath.GetBuffer(MAX_PATH);
        PathRemoveExtensionX(str);
        m_strPath.ReleaseBuffer();
    }

    BOOL RemoveFileSpec()
    {
        PXSTR str = m_strPath.GetBuffer(MAX_PATH);
        BOOL result = PathRemoveFileSpecX(str);
        m_strPath.ReleaseBuffer();
        return result;
    }

    BOOL RenameExtension(PCXSTR pszExtension)
    {
        PXSTR str = m_strPath.GetBuffer(MAX_PATH);
        BOOL result = PathRenameExtensionX(str, pszExtension);
        m_strPath.ReleaseBuffer();
        return result;
    }

    int SkipRoot() const
    {
        PCXSTR str = m_strPath;
        return (int)(PathSkipRootX(m_strPath) - str);
    }

    void StripPath()
    {
        PXSTR str = m_strPath.GetBuffer(MAX_PATH);
        PathStripPathX(str);
        m_strPath.ReleaseBuffer();
    }

    BOOL StripToRoot()
    {
        PXSTR str = m_strPath.GetBuffer(MAX_PATH);
        BOOL result = PathStripToRootX(str);
        m_strPath.ReleaseBuffer();
        return result;
    }

    void UnquoteSpaces()
    {
        PXSTR str = m_strPath.GetBuffer(MAX_PATH);
        PathUnquoteSpacesX(str);
        m_strPath.ReleaseBuffer();
    }

    operator const StringType&() const throw()
    {
        return m_strPath;
    }

    operator PCXSTR() const throw()
    {
        return m_strPath;
    }

    operator StringType&() throw()
    {
        return m_strPath;
    }

    CPathT<StringType>& operator+=(PCXSTR pszMore)
    {
        Append(pszMore);
        return *this;
    }

};

typedef CPathT<CString> CPath;
typedef CPathT<CStringA> CPathA;
typedef CPathT<CStringW> CPathW;

}  // namespace ATL

#endif
