#ifndef _INC_DSKQUOTA_OWNERLST_H
#define _INC_DSKQUOTA_OWNERLST_H

#ifndef _INC_DSKQUOTA_STRCLASS_H
#   include "strclass.h"
#endif

#ifndef _INC_DSKQUOTA_CARRAY_H
#   include "carray.h"
#endif

#ifndef _INC_DSKQUOTA_H
#   include <dskquota.h>
#endif

//-----------------------------------------------------------------------------
// The following classes are used in managing the list of files owned by
// one or more users selected for deletion from within the details view.
//
//  COwnerList - A list of "owner" objects.  Each representing one of the
//      account selected for deletion from the details view.
//
//  COwnerListEntry - A single entry in the COwnerList container.  Each
//      entry contains a pointer to the IDiskQuotaUser inteface of the
//      file owner and an array of CStrings containing the names of the
//      files owned by that user.  A blank filename is considered "deleted".
//
//  COwnerListItemHandle - This is a simple class used to hide the encoding
//      of the owner index and file index in a listview LPARAM value.  The
//      owner index is the index of the LV item's owner in the COwnerList
//      container.  The file index is the index of the item's filename in the
//      owner's COwnerListEntry in the COwnerList container. Perfectly clear,
//      right?  Each listview item's LPARAM contains enough information
//      (iOwner and iFile) to locate the owner and file information in the
//      COwnerList container.  This encoding was done for efficiency reasons.
//      The encoding is currently 10 bits for iOwner (max of 1024) and 22
//      bits for iFile (max of 4 meg).  These values can be adjusted if
//      the balance isn't quite right.
//
//
class COwnerListEntry
{
    public:
        explicit COwnerListEntry(IDiskQuotaUser *pOwner);
        ~COwnerListEntry(void)
            { if (m_pOwner) m_pOwner->Release(); }

        IDiskQuotaUser* GetOwner(void) const
            { m_pOwner->AddRef(); return m_pOwner; }

        void GetOwnerName(CString *pstrOwner) const
            { *pstrOwner = m_strOwnerName; }

        int AddFile(LPCTSTR pszFile);

        void MarkFileDeleted(int iFile)
            { m_rgFiles[iFile].Empty(); }

        bool IsFileDeleted(int iFile) const
            { return !!m_rgFiles[iFile].IsEmpty(); }

        void GetFileName(int iFile, CPath *pstrFile) const
            { m_rgFiles[iFile].GetFileSpec(pstrFile); }

        void GetFolderName(int iFile, CPath *pstrFolder) const
            { m_rgFiles[iFile].GetPath(pstrFolder); }

        void GetFileFullPath(int iFile, CPath *pstrFullPath) const
            { *pstrFullPath = m_rgFiles[iFile]; }

        int FileCount(void);

#if DBG
        void Dump(void) const;
#endif

    private:
        IDiskQuotaUser *m_pOwner;       // Ptr to owner object.
        CString         m_strOwnerName; // Owner's name for display.
        CArray<CPath>   m_rgFiles;      // Filenames for display.

        //
        // Prevent copy.  Array makes it too expensive.
        //
        COwnerListEntry(const COwnerListEntry& rhs);
        COwnerListEntry& operator = (const COwnerListEntry& rhs);
};


class COwnerList
{
    public:
        COwnerList(void) { }
        ~COwnerList(void);

        int AddOwner(IDiskQuotaUser *pOwner);

        IDiskQuotaUser *GetOwner(int iOwner) const;

        void GetOwnerName(int iOwner, CString *pstrOwner) const
            { m_rgpOwners[iOwner]->GetOwnerName(pstrOwner); }

        int AddFile(int iOwner, LPCTSTR pszFile)
            { return m_rgpOwners[iOwner]->AddFile(pszFile); }

        void MarkFileDeleted(int iOwner, int iFile)
            { m_rgpOwners[iOwner]->MarkFileDeleted(iFile); }

        bool IsFileDeleted(int iOwner, int iFile) const
            { return m_rgpOwners[iOwner]->IsFileDeleted(iFile); }

        void GetFileName(int iOwner, int iFile, CPath *pstrFile) const
            { m_rgpOwners[iOwner]->GetFileName(iFile, pstrFile); }

        void GetFolderName(int iOwner, int iFile, CPath *pstrFolder) const
            { m_rgpOwners[iOwner]->GetFolderName(iFile, pstrFolder); }

        void GetFileFullPath(int iOwner, int iFile, CPath *pstrFullPath) const
            { m_rgpOwners[iOwner]->GetFileFullPath(iFile, pstrFullPath); }

        void Clear(void);

        int FileCount(int iOwner = -1) const;

        int OwnerCount(void) const
            { return m_rgpOwners.Count(); }

#if DBG
        void Dump(void) const;
#endif

    private:
        CArray<COwnerListEntry *> m_rgpOwners;

        //
        // Prevent copy.
        //
        COwnerList(const COwnerList& rhs);
        COwnerList& operator = (const COwnerList& rhs);
};



class COwnerListItemHandle
{
    public:
        explicit COwnerListItemHandle(int iOwner = -1, int iFile = -1)
            : m_handle((iOwner & MASK) | ((iFile << SHIFT) & ~MASK)) { }

        COwnerListItemHandle(LPARAM lParam)
            : m_handle(lParam) { }

        operator LPARAM() const
            { return m_handle; }

        int OwnerIndex(void) const
            { return int(m_handle & MASK); }

        int FileIndex(void) const
            { return int((m_handle >> SHIFT) & (~MASK >> SHIFT)); }

    private:
        LPARAM m_handle;

        enum { MASK = 0x3FF, SHIFT = 10 };
};



#endif // _INC_DSKQUOTA_OWNERLST_H
