#ifndef _INC_CSCVIEW_OBJTREE_H
#define _INC_CSCVIEW_OBJTREE_H

#ifndef _WINDOWS_
#   include <windows.h>
#endif

#ifndef _INC_CSCVIEW_STRCLASS_H
#   include "strclass.h"
#endif

#ifndef _INC_CSCVIEW_CARRAY_H
#   include "carray.h"
#endif

#ifndef _INC_CSCVIEW_UNCPATH_H
#   include "uncpath.h"
#endif

#ifndef _INC_CSCVIEW_FILETIME_H
#   include "filetime.h"
#endif

#ifndef _INC_CSCVIEW_FILESIZE_H
#   include "filesize.h"
#endif

#ifndef _INC_CSCVIEW_CSCUTILS_H
#   include "cscutils.h"
#endif

#include <resource.h>

//
//
//  Csc Object Tree 
//  Class Inheritance hierarchy
//
//
//                      +---------+
//                  +---| CscFile |
//                  |   +---------+
//  +-----------+   |
//  | CscObject |---+                          +-----------+  
//  +-----------+   |                      +---| CscFolder |
//                  |   +--------------+   |   +-----------+
//                  +---| CscObjParent |---|
//                      +--------------+   |   +-----------+
//                                         +---| CscShare  |
//                                             +-----------+
//
//


//
// NOTE:  The efficiency of this implementation depends heavily on the reference
//        counted string class CString.  Because of the reference counting,
//        there should only be one instance of each unique name string in 
//        existance.
//
//
// Class used to retrieve various display strings for a branch
// in the CSC object tree.  A reference to one of these objects
// is passed up the tree where each object along the way adds
// it's information.  When the branch traversal returns, the
// information reflects all items in the branch.
//
class CscObjDispInfo
{
    public:
        CscObjDispInfo(void) : m_fMask(0), m_iIcon(-1) { }

        DWORD   m_fMask;               // ODI_XXXXX mask.
        CString m_strServer;           // Server name string.
        CString m_strShare;            // Share name string.
        CString m_strPath;             // Path name string.
        CString m_strFile;             // File name string.
        CString m_strShareDisplayName; // Share name with drv ltr. "ntspecs (E:)"
        CString m_strShareStatus;      // Share status text (i.e. "Connected").
        CString m_strFileTime;         // File modified date as a string (if appl).
        CString m_strFileSize;         // File size as a string (if applicable).
        CString m_strStaleReason;      // Reason for stale file.
        short   m_iIcon;               // Icon index in imagelist.
};

//
// Flag constants for the CscObjDispInfo.m_fMask member.
//
const DWORD ODI_SERVER        = 0x00000001;
const DWORD ODI_SHARE         = 0x00000002;
const DWORD ODI_PATH          = 0x00000004;
const DWORD ODI_FILE          = 0x00000008;
const DWORD ODI_SHAREDISPNAME = 0x00000010;
const DWORD ODI_SHARESTATUS   = 0x00000020;
const DWORD ODI_FILETIME      = 0x00000040;
const DWORD ODI_FILESIZE      = 0x00000080;
const DWORD ODI_STALEREASON   = 0x00000100;
const DWORD ODI_ICON          = 0x00000200;
const DWORD ODI_ALL           = 0x000001FF;


class CscObject;    // fwd decl.
class CscObjParent; // fwd decl.
class CscObjTree;   // fwd decl.
class CscShare;     // fwd decl.

//
// Iterator item exlusion flags.
//
enum IterExcl
{
    EXCLUDE_NONE        = 0x00000000,
    EXCLUDE_NONEXISTING = 0x00000001, // Exclude items not yet created in tree.
    EXCLUDE_SPARSEFILES = 0x00000002, // Exclude files marked as "sparse".
    EXCLUDE_MASK        = 0x00000003
};


//
// Iterator for enumerating children of a tree entry.
// This code assumes that the referenced array of CscObject pointers
// remains alive while the iterator is alive.  No reference counting
// (like COM) of the source array is done at this time.
//
class CscObjIterator
{
    public:
        explicit CscObjIterator(const CscObjParent *pParent, DWORD fExclude = EXCLUDE_NONE) throw();

        CscObjIterator(void) throw()
            : m_pParent(NULL),
              m_iChild(0),
              m_fExclude(EXCLUDE_NONE) { }

        virtual ~CscObjIterator(void) throw() { };

        bool ExcludeItem(CscObject *pObject) const throw();

        bool ExistingOnly(void) const throw()
            { return (0 != (EXCLUDE_NONEXISTING & m_fExclude)); }

        virtual bool Next(CscObject **ppObjOut);

        virtual void Reset(void) throw()
            { m_iChild = 0; }

    private:
        const CscObjParent *m_pParent;       // Address of parent object.
        int                 m_iChild;        // Index for iteration context.
        DWORD               m_fExclude;      // Exclusions.
};


//
// Pure virtual base object for all entries in the CSC object tree.
// Uses "IsA()" virtual function to avoid using RTTI.  
//
class CscObject
{
    public:
        //
        // The order of these items is important and is assumed 
        // in several areas (sorting in particular).
        //
        enum ObjType { Root, Share, Folder, File };

        explicit CscObject(CscObjParent *pParent = NULL, WORD wStatus = 0, WORD wPinState = 0) throw()
            : m_cRef(0),
              m_pParent(pParent),
              m_wCscStatus(wStatus),
              m_wCscPinState(wPinState),
              m_iIcon(-1) { }

        virtual ~CscObject(void) { };

        //
        // This reference counting code was a last-minute addition.
        // Originally I was not deleting items from the object tree but
        // merely marking them as "deleted" then exluding "deleted" items
        // from the various views and iterations (as if they didn't exist).
        // Later I decided to actually delete the objects but this
        // introduced the need for reference counting as pointers to
        // tree objects are held in several places:
        //    1. Viewer's listview
        //    2. Object tree
        //    3. Icon hound input queue.
        // Object full-path strings are formed by calling up the object
        // hierarchy, retrieving each generation's portion of the path
        // and appending it onto the full path.  This requires that 
        // while any given object is alive, all of it's ancestors must
        // also remain alive.  Calling AddRef/Release on the parent
        // accomplishes this goal.  This scheme proved successful evidenced
        // by Bounds Checker reporting no memory leaks.
        //
        void AddRef(void) const throw()
            { InterlockedIncrement(&m_cRef); }

        void Release(void) const throw()
            { if (0 == InterlockedDecrement(&m_cRef)) 
                  delete this; 
            }

        //
        // Call AddRef on all of the object's ancestors.
        // This must be a function separate from AddRef() so we avoid the deadly
        // circular reference count.  Object tree functions NEVER call this.
        // Only functions taking outside references to tree objects like the
        // viewer's listview, icon hound etc.
        //
        void LockAncestors(void) const throw();
        //
        // Call Release on all of the object's ancestors.
        // The circular ref count description with LockAncestors applies
        // here also.
        //
        void ReleaseAncestors(void) const throw();
        //
        // Retrieve address of parent in tree.
        //
        CscObjParent *GetParent(void) const throw()
            { return m_pParent; }
        //
        // Some objects may be parent-capable but just not have any 
        // children yet.  This attribute indicates if the object 
        // is capable of having children.  This is different from having kids.
        // Parent-capable derived classes will override this and return true.
        // i.e. CscObjParent.
        //
        virtual bool CanBeParent(void) const throw()
            { return false; }
        //
        // As with CanBeParent(), derived classes (i.e. CscObjParent)
        // will override this function and indicate if they ARE a parent.
        //
        virtual bool IsParent(void) const throw()
            { return false; }
        //
        // Is this object descended from pObject?
        //
        bool IsDescendantOf(const CscObject *pObject) const throw();
        //
        // How many children does this object have?
        //
        virtual int CountChildren(void) const throw()
            { return 0; }
        //
        // Count all descendants.
        //
        virtual int CountDescendants(void) const
            { return 0; }
        //
        // By default, an object returns a NULL iterator (an iterator that
        // doesn't iterate anything).  The first call to Next() will return
        // false.  Descendant classes (i.e. CscObjParent) can override this
        // and return an iterator with contents.
        //
        virtual CscObjIterator CreateChildIterator(DWORD /* unused */) const throw()
            { return CscObjIterator(); }
        //
        // Return address of the object's ancestor share object.
        //
        CscShare *GetShare(void) const throw();
        //
        // Determine if a file's share is "online".
        //
        bool IsShareOnLine(void) const throw();
        //
        // Return "ObjType" enumerated value to indicate type of 
        // derived object.  This is more efficient than using the C++ dynamic
        // RTTI mechanism.
        //
        virtual int IsA(void) const throw() = 0;

        bool IsRoot(void) const throw()
            { return CscObject::Root == IsA(); }

        bool IsFile(void) const throw()
            { return CscObject::File == IsA(); }

        bool IsFolder(void) const throw()
            { return CscObject::Folder == IsA(); }

        bool IsShare(void) const throw()
            { return CscObject::Share == IsA(); }
        //
        // Retrieve the various pieces of display info for an object.
        // "Server", "Share", "Path", "File".
        //
        virtual void GetDispInfo(CscObjDispInfo *pdi, DWORD fMask, bool bCalledByChild = false) const = 0;
        //
        // These functions expose the CSC_STATUS and CSC_HINT flags
        // The actual flag data is stored as protected values in CscObject.
        // Each derived class provides it's own interpretation of that
        // data.  By default, all of these members return false.
        //
        virtual bool DataLocallyModified(void) const throw()
            { return false; }
        virtual bool AttribLocallyModified(void) const throw()
            { return false; }
        virtual bool TimeLocallyModified(void) const throw()
            { return false; }
        virtual bool LocallyModified(void) const throw()
            { return false; }
        virtual bool LocallyCreated(void) const throw()
            { return false; }
        virtual bool LocallyDeleted(void) const throw()
            { return false; }
        virtual bool IsStale(void) const throw()
            { return false; }
        virtual bool IsSparse(void) const throw()
            { return false; }
        virtual bool IsOrphan(void) const throw()
            { return false; }
        virtual bool IsSuspect(void) const throw()
            { return false; }
        virtual bool ModifiedOffline(void) const throw()
            { return false; }
        virtual bool IsConnected(void) const throw()
            { return false; }
        virtual bool IsPinnedUser(void) const throw()
            { return false; }
        virtual bool IsPinnedSystem(void) const throw()
            { return false; }
        virtual bool IsPinned(void) const throw()
            { return false; }
        virtual bool IsPinnedInheritUser(void) const throw()
            { return false; }
        virtual bool IsPinnedInheritSystem(void) const throw()
            { return false; }
        virtual bool IsPinnedInherit(void) const throw()
            { return false; }
        virtual bool NeedToSync(void) const throw()
            { return false; }
        virtual bool IsEncrypted(void) const throw()
            { return false; }
        //
        // Have CSC pin or unpin the object.
        // If successful, also change the m_wCscPinState member
        // to reflect the new state.
        //
        virtual HRESULT Pin(bool bPin);
        //
        // Determine if an object can provide file time information.
        // Can save some time by calling this before GetFileTime or
        // GetFileTimeString.  Only files and folders have file time info.
        //
        virtual bool HasFileTimeInfo(void) const throw()
            { return false; }
        //
        // Retrieve object's file time as a FILETIME struct.
        //
        virtual FILETIME GetFileTime(void) const throw()
            { FILETIME ft = {0, 0}; return ft; }
        //
        // Retrieve object's file time formatted as a text string.
        // Uses shell's file time format.
        //
        virtual void GetFileTimeString(CString *pstrFT) const
            { DBGASSERT((NULL != pstrFT)); pstrFT->Empty(); }
        //
        // Determine if an object can provide file size information.
        // Can save some time by calling this before GetFileSize or
        // GetFileSizeString.  Only files have file size info.
        //
        virtual bool HasFileSizeInfo(void) const throw()
            { return false; }
        //
        // Retrieve the object's file size as a number.
        //
        virtual ULONGLONG GetFileSize(void) const throw()
            { return (ULONGLONG)0; }
        //
        // Retrieve the object's file size formatted as a text string.
        // Uses the shell's format (i.e. "10.5 MB").
        //
        virtual void GetFileSizeString(CString *pstrFS) const
            { DBGASSERT((NULL != pstrFS)); pstrFS->Empty(); }
        //
        // Determine if an object and all of it's children are populated
        // in the tree.  For all non-parent objects, this is always true.
        // For parent objects, this is only true when all of the parent's
        // children have been created.
        //
        virtual bool IsComplete(void) const throw()
            { return true; }
        //
        // Clear the "sparse" flag after a file has been successfully
        // sychronized.
        // This function is logically const but not physically const.
        // Thus the const_cast<> is required.
        //
        void ClearSparseness(void) const throw()
            { (const_cast<CscObject *>(this)->m_wCscStatus) &= ~FLAG_CSC_COPY_STATUS_SPARSE; }

        virtual void GetFullPath(CString *pstrPath) const
            { DBGASSERT((NULL != pstrPath)); pstrPath->Empty(); }

        virtual void GetPath(CString *pstrPath) const
            { DBGASSERT((NULL != pstrPath)); pstrPath->Empty(); }

        short GetIconImageIndex(void) const throw()
            { return m_iIcon; }

        void SetIconImageIndex(short iIcon) throw()
            { m_iIcon = iIcon; }

        virtual void UpdateCscInfo(void) { /* does nothing */ };

#if DBG
        void Dump(void);
#endif

    protected:
        WORD m_wCscStatus;    // FLAG_CSC_COPY[SHARE]_STATUS_XXXX bits.
        WORD m_wCscPinState;  // FLAG_CSC_HINT_PIN_XXXX bits.

    private:
        mutable LONG  m_cRef;           
        CscObjParent *m_pParent;       // Parent of this entry.
        short         m_iIcon;         // Icon imagelist index.

        //
        // Prevent copy.
        //
        CscObject(const CscObject& rhs);
        CscObject& operator = (const CscObject& rhs);
};


//
// Base class for all entries that can be a parent.
// Provides an array of ptrs to children.  Array ptr is NULL if entry has no children.
//
class CscObjParent : public CscObject
{
    public:
        explicit CscObjParent(CscObjParent *pParent, WORD wStatus, WORD wPinState) throw()
            : CscObject(pParent, wStatus, wPinState),
              m_prgChildren(NULL),
              m_bIsComplete(false) { }

        virtual ~CscObjParent(void);

        void AddChild(CscObject *pChild);

        HRESULT DestroyChild(CscObject *pChild, bool bDeleteFromCache = true);

        void DestroyChildren(void);

        CscObjIterator CreateChildIterator(DWORD fExclude = EXCLUDE_NONE) const throw();

        CscObject *GetChild(int i) const
            { return (NULL != m_prgChildren) ? m_prgChildren->GetAt(i) : NULL; }

        virtual bool IsParent(void) const throw()
            { return NULL != m_prgChildren && 0 < m_prgChildren->Count(); }

        virtual bool CanBeParent(void) const throw()
            { return true; }

        virtual bool IsComplete(void) const throw()
            { return m_bIsComplete; }

        bool IsDescendant(const CscObject *pObject) const throw();

        virtual int CountChildren(void) const throw()
            { return NULL != m_prgChildren ? m_prgChildren->Count() : 0; }

        virtual int CountDescendants(void) const;

        void AllChildrenCreated(void) throw()
            { m_bIsComplete = true; }

        virtual bool IsPinned(void) const throw() 
            { return false; }

    private:
        CArray<CscObject *> *m_prgChildren; // Ptr to array of children.
        bool                 m_bIsComplete; // All children created?

        //
        // Prevent copy.
        //
        CscObjParent(const CscObjParent& rhs);
        CscObjParent& operator = (const CscObjParent& rhs);
        //
        // Iterator needs access to m_prgChildren pointer.
        //
        friend class CscObjIterator;
        //
        // Tree needs to call AllChildrenCreated().
        //
        friend class CscObjTree;
};



//
// Concrete class representing a file in the CSC object tree.
//
// You may ask why the common information in CscFile and CscFolder isn't abstracted
// out into a common base class.  The problem is that a CscFolder is also a CscObjParent
// but CscFile is not.  With such an inheritance hierarchy, CscFolder ends up with 
// a multiple-inheritance graph that introduces all sorts of complications.  Yes, 
// it's a cleaner design on paper but the complexity cost isn't worth it.  I 
// chose to duplicate some of the functionality and data in both CscFile and
// CscFolder in order to eliminate the multiple-inheritance problem.
// [brianau - 10/15/97]
//
class CscFile : public CscObject
{
    public:
        CscFile(CscObjParent *pParent, 
                const CString& strName, 
                WORD wStatus, 
                WORD wPinState, 
                const FileTime& LastModified, 
                const FileSize& FileSize);

        virtual int IsA(void) const throw()
            { return CscObject::File; }

        void GetName(CString *pstrName) const
            { DBGASSERT((NULL != pstrName)); *pstrName = m_strName; }

        virtual bool HasFileTimeInfo(void) const throw()
            { return true; }

        virtual FILETIME GetFileTime(void) const throw()
            { return (FILETIME)m_LastModified; }

        virtual void GetFileTimeString(CString *pstrFT) const
            { DBGASSERT((NULL != pstrFT)); m_LastModified.GetString(pstrFT); }

        virtual bool HasFileSizeInfo(void) const throw()
            { return true; }

        virtual ULONGLONG GetFileSize(void) const throw()
            { return (ULONGLONG)m_FileSize; }

        virtual void GetFileSizeString(CString *pstrFS) const
            { DBGASSERT((NULL != pstrFS)); m_FileSize.GetString(pstrFS); }


        virtual bool DataLocallyModified(void) const throw()
            { return boolify(m_wCscStatus & FLAG_CSC_COPY_STATUS_DATA_LOCALLY_MODIFIED); }
        virtual bool AttribLocallyModified(void) const throw()
            { return boolify(m_wCscStatus & FLAG_CSC_COPY_STATUS_ATTRIB_LOCALLY_MODIFIED); }
        virtual bool TimeLocallyModified(void) const throw()
            { return boolify(m_wCscStatus & FLAG_CSC_COPY_STATUS_TIME_LOCALLY_MODIFIED); }
        virtual bool LocallyModified(void) const throw()
            { return boolify(m_wCscStatus & (FLAG_CSC_COPY_STATUS_DATA_LOCALLY_MODIFIED |
                                             FLAG_CSC_COPY_STATUS_ATTRIB_LOCALLY_MODIFIED |
                                             FLAG_CSC_COPY_STATUS_TIME_LOCALLY_MODIFIED)); }
        virtual bool LocallyCreated(void) const throw()
            { return boolify(m_wCscStatus & FLAG_CSC_COPY_STATUS_LOCALLY_CREATED); }
        virtual bool LocallyDeleted(void) const throw()
            { return boolify(m_wCscStatus & FLAG_CSC_COPY_STATUS_LOCALLY_DELETED); }
        virtual bool IsStale(void) const throw()
            { return boolify(m_wCscStatus & FLAG_CSC_COPY_STATUS_STALE);  }
        virtual bool IsSparse(void) const throw()
            { return boolify(m_wCscStatus & FLAG_CSC_COPY_STATUS_SPARSE); }
        virtual bool IsOrphan(void) const throw()
            { return boolify(m_wCscStatus & FLAG_CSC_COPY_STATUS_ORPHAN); }
        virtual bool IsSuspect(void) const throw()
            { return boolify(m_wCscStatus & FLAG_CSC_COPY_STATUS_SUSPECT); }
        virtual bool IsPinnedUser(void) const throw()
            { return boolify(m_wCscPinState & FLAG_CSC_HINT_PIN_USER); }
        virtual bool IsPinnedSystem(void) const throw()
            { return boolify(m_wCscPinState & FLAG_CSC_HINT_PIN_SYSTEM); }
        virtual bool IsPinned(void) const throw()
            { return boolify(m_wCscPinState & (FLAG_CSC_HINT_PIN_USER | FLAG_CSC_HINT_PIN_SYSTEM)); }
        virtual bool IsPinnedInheritUser(void) const throw()
            { return boolify(m_wCscPinState & FLAG_CSC_HINT_PIN_INHERIT_USER); }
        virtual bool IsPinnedInheritSystem(void) const throw()
            { return boolify(m_wCscPinState & FLAG_CSC_HINT_PIN_INHERIT_SYSTEM); }
        virtual bool IsPinnedInherit(void) const throw()
            { return boolify(m_wCscPinState & (FLAG_CSC_HINT_PIN_INHERIT_USER | FLAG_CSC_HINT_PIN_INHERIT_SYSTEM)); }
        virtual bool NeedToSync(void) const throw()
            { return boolify(FLAG_CSC_COPY_STATUS_NEEDTOSYNC_FILE & m_wCscStatus); }

        virtual HRESULT Pin(bool bPin);

        virtual void GetDispInfo(CscObjDispInfo *pdi, DWORD fMask, bool bCalledByChild = false) const;

        virtual void GetFullPath(CString *pstrPath) const;

        virtual void GetPath(CString *pstrPath) const;

        int CompareName(const CString& strName) const
            { return m_strName.CompareNoCase((LPCTSTR)strName); }

        int CompareName(const CscFile& rhs) const
            { return CompareName(rhs.m_strName); }

        int CompareTime(const FileTime& LastModified) const throw()
            { return m_LastModified.Compare(LastModified); }

        int CompareTime(const CscFile& rhs) const throw()
            { return CompareTime(rhs.m_LastModified); }

        int CompareSize(const FileSize& rhs) const throw()
            { return m_FileSize.Compare(rhs); }

        int CompareSize(const CscFile& rhs) const throw()
            { return CompareSize(rhs.m_FileSize); }

        void GetStaleDescription(CString *pstrDesc) const;

        int GetStaleReasonCode(void) const
            { return CscFileStatusToStaleReasonId(m_wCscStatus); }

        virtual void UpdateCscInfo(void);

    private:
        CString    m_strName;      // File name.
        FileTime   m_LastModified; // When was file last modified.
        FileSize   m_FileSize;     // Size of file (from CSC database).

        //
        // Prevent copy.
        //
        CscFile(const CscFile& rhs);
        CscFile& operator = (const CscFile& rhs);
};


//
// Concrete class representing a folder in the CSC object tree.
//
class CscFolder : public CscObjParent
{
    public:
        CscFolder(CscObjParent *pParent, 
                  const CString& strName,                 
                  WORD wStatus, 
                  WORD wPinState, 
                  const FileTime& LastModified, 
                  const FileSize& FileSize);

        virtual int IsA(void) const throw()
            { return CscObject::Folder; }

        void GetName(CString *pstrName) const
            { DBGASSERT((NULL != pstrName)); *pstrName = m_strName; }

        virtual bool HasFileInfo(void) const throw()
            { return true; }

        virtual FILETIME GetFileTime(void) const throw()
            { return (FILETIME)m_LastModified; }

        virtual void GetFileTimeString(CString *pstrFT) const
            { DBGASSERT((NULL != pstrFT)); m_LastModified.GetString(pstrFT); }

        virtual bool HasFileSizeInfo(void) const throw()
            { return true; }

        virtual ULONGLONG GetFileSize(void) const throw()
            { return (ULONGLONG)m_FileSize; }

        virtual void GetFileSizeString(CString *pstrFS) const
            { DBGASSERT((NULL != pstrFS)); m_FileSize.GetString(pstrFS); }

        virtual bool DataLocallyModified(void) const throw()
            { return boolify(m_wCscStatus & FLAG_CSC_COPY_STATUS_DATA_LOCALLY_MODIFIED); }
        virtual bool AttribLocallyModified(void) const throw()
            { return boolify(m_wCscStatus & FLAG_CSC_COPY_STATUS_ATTRIB_LOCALLY_MODIFIED); }
        virtual bool TimeLocallyModified(void) const throw()
            { return boolify(m_wCscStatus & FLAG_CSC_COPY_STATUS_TIME_LOCALLY_MODIFIED); }
        virtual bool LocallyModified(void) const throw()
            { return boolify(m_wCscStatus & (FLAG_CSC_COPY_STATUS_DATA_LOCALLY_MODIFIED |
                                             FLAG_CSC_COPY_STATUS_ATTRIB_LOCALLY_MODIFIED |
                                             FLAG_CSC_COPY_STATUS_TIME_LOCALLY_MODIFIED)); }
        virtual bool LocallyCreated(void) const throw()
            { return boolify(m_wCscStatus & FLAG_CSC_COPY_STATUS_LOCALLY_CREATED); }
        virtual bool LocallyDeleted(void) const throw()
            { return boolify(m_wCscStatus & FLAG_CSC_COPY_STATUS_LOCALLY_DELETED); }
        virtual bool IsStale(void) const throw()
            { return boolify(m_wCscStatus & FLAG_CSC_COPY_STATUS_STALE);  }
        virtual bool IsSparse(void) const throw()
            { return boolify(m_wCscStatus & FLAG_CSC_COPY_STATUS_SPARSE); }
        virtual bool IsOrphan(void) const throw()
            { return boolify(m_wCscStatus & FLAG_CSC_COPY_STATUS_ORPHAN); }
        virtual bool IsSuspect(void) const throw()
            { return boolify(m_wCscStatus & FLAG_CSC_COPY_STATUS_SUSPECT); }
        virtual bool IsPinnedUser(void) const throw()
            { return boolify(m_wCscPinState & FLAG_CSC_HINT_PIN_USER); }
        virtual bool IsPinnedSystem(void) const throw()
            { return boolify(m_wCscPinState & FLAG_CSC_HINT_PIN_SYSTEM); }
        virtual bool IsPinned(void) const throw()
            { return boolify(m_wCscPinState & (FLAG_CSC_HINT_PIN_USER)); }
        virtual bool IsPinnedInheritUser(void) const throw()
            { return boolify(m_wCscPinState & FLAG_CSC_HINT_PIN_INHERIT_USER); }
        virtual bool IsPinnedInheritSystem(void) const throw()
            { return boolify(m_wCscPinState & FLAG_CSC_HINT_PIN_INHERIT_SYSTEM); }
        virtual bool IsPinnedInherit(void) const throw()
            { return boolify(m_wCscPinState & (FLAG_CSC_HINT_PIN_INHERIT_USER | FLAG_CSC_HINT_PIN_INHERIT_SYSTEM)); }
        virtual bool NeedToSync(void) const throw()
            { return boolify(FLAG_CSC_COPY_STATUS_NEEDTOSYNC_FOLDER & m_wCscStatus); }

        virtual HRESULT Pin(bool bPin);

        virtual void GetDispInfo(CscObjDispInfo *pdi, DWORD fMask, bool bCalledByChild = false) const;

        virtual void GetFullPath(CString *pstrPath) const;

        virtual void GetPath(CString *pstrPath) const;

        int CompareName(const CString& strName) const
            { return m_strName.CompareNoCase((LPCTSTR)strName); }

        int CompareName(const CscFolder& rhs) const
            { return CompareName(rhs.m_strName); }

        int CompareTime(const FileTime& LastModified) const throw()
            { return m_LastModified.Compare(LastModified); }

        int CompareTime(const CscFolder& rhs) const throw()
            { return CompareTime(rhs.m_LastModified); }

        void GetStaleDescription(CString *pstrDesc) const;

        int GetStaleReasonCode(void) const
            { return CscFileStatusToStaleReasonId(m_wCscStatus); }

        virtual void UpdateCscInfo(void);

    private:
        CString    m_strName;      // Folder name.
        FileTime   m_LastModified; // When was folder last modified.
        FileSize   m_FileSize;     // Size of file (from CSC database).

        //
        // Prevent copy.
        //
        CscFolder(const CscFolder& rhs);
        CscFolder& operator = (const CscFolder& rhs);
};
      

//
// Concrete class representing a share in the CSC object tree.
//
class CscShare : public CscObjParent
{
    public:
        CscShare(CscObjTree *pTreeRoot,
                 const CString& strServer, 
                 const CString& strShare,
                 TCHAR chDrive,
                 WORD wStatus,
                 int cFiles, 
                 int cPinnedFiles, 
                 int cStale);

        virtual int IsA(void) const throw()
            { return CscObject::Share; }

        virtual bool ModifiedOffline(void) const throw()
            { return boolify(m_wCscStatus & FLAG_CSC_SHARE_STATUS_MODIFIED_OFFLINE); }

        virtual bool IsConnected(void) const throw()
            { return !boolify(m_wCscStatus & FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP); }

        virtual void GetDispInfo(CscObjDispInfo *pdi, DWORD fMask, bool bCalledByChild = false) const;

        virtual void GetFullPath(CString *pstrPath) const;

        virtual void GetPath(CString *pstrPath) const;

        void GetName(CString *pstrName) const;

        void GetServerName(CString *pstrName) const
            { DBGASSERT((NULL != pstrName)); *pstrName = m_strServer; }

        void GetShareName(CString *pstrName) const
            { DBGASSERT((NULL != pstrName)); *pstrName = m_strShare; }

        void GetLongDisplayName(CString *pstrName) const
            { DBGASSERT((NULL != pstrName)); *pstrName = m_strLongDisplayName; }

        void GetShortDisplayName(CString *pstrName) const
            { DBGASSERT((NULL != pstrName)); *pstrName = m_strShortDisplayName; }

        void GetStatusText(CString *pstrText) const
            { DBGASSERT((NULL != pstrText)); *pstrText = m_strStatus; }

        int CompareServerName(const CString& strServer) const
            { return m_strServer.CompareNoCase((LPCTSTR)strServer); }

        int CompareShareName(const CString& strShare) const
            { return m_strShare.CompareNoCase((LPCTSTR)strShare); }

        int GetFileCount(void) const throw()
            { return m_cFiles; }

        int GetPinnedFileCount(void) const throw()
            { return m_cPinnedFiles; }

        int GetStaleFileCount(void) const throw()
            { return m_cStale; }

        void AdjustPinnedFileCount(int c) throw()
            { m_cPinnedFiles += c; DBGASSERT((0 <= m_cPinnedFiles)); }

        void AdjustStaleFileCount(int c) throw()
            { m_cStale += c; DBGASSERT((0 <= m_cStale)); }

        void AdjustFileCount(int c) throw()
            { m_cFiles += c; DBGASSERT((0 <= m_cFiles)); }

        int GetLoadedFileCount(void) const throw()
            { return m_cFilesLoaded; }

        void IncrLoadedFileCount(void) throw();

        virtual void UpdateCscInfo(void);

    private:
        CString m_strServer;          // "worf"
        CString m_strShare;           // "ntspecs"
        CString m_strLongDisplayName; // "ntspecs on 'worf' (E:)"
        CString m_strShortDisplayName;// "ntspecs (E:)"
        CString m_strStatus;          // "Connected", "Disconnected"
        int     m_cFiles;             // Total files cached on share.
        int     m_cPinnedFiles;       // Count of cached pinned files.
        int     m_cStale;             // Count of cached "dirty" files. 
        int     m_cFilesLoaded;       // Total files loaded into tree.

        //
        // Prevent copy.
        //
        CscShare(const CscShare& rhs);
        CscShare& operator = (const CscShare& rhs);

        friend bool operator == (const CscShare& a, const CscShare& b);
        friend bool operator != (const CscShare& a, const CscShare& b);
};


inline bool operator == (const CscShare& a, const CscShare& b)
{ 
    return 0 == a.CompareServerName(b.m_strServer) &&
           0 == a.CompareShareName(b.m_strShare); 
}


inline bool operator != (const CscShare& a, const CscShare& b)
{ 
    return !(a == b);
}



//
// A cache of object name information.  This is maintained to help 
// with repaints in the CSC cache viewer's listview.  The listview
// is designed so that display information is provided on demand.
// Therefore, repeated requests for display information (i.e. repaints) 
// can quickly retrieve display strings from this cache rather than having 
// to traverse the object tree and rebuild each display string.  Since 
// the name strings in both the cache and object tree use the CString class, 
// they leverage the reference counting design of CString to eliminate 
// redundant storage of like strings.
//
// The cache uses a hash table with chaining to resolve table index collisions.
// There is also a list maintained for identifying the most-recently-used
// entries in the cache.  Each entry lives in both a hash table bucket and
// the MRU list.  Note that for performance reasons, the cache does not
// enforce a no-duplicates policy.  It is the responsibility of the cache 
// client to call Retrieve (to see if an entry exists) before adding
// a new entry to the cache.
//
class CscObjNameCache
{
    public:
        CscObjNameCache(int cMaxEntries, int cMaxHashBuckets);
        ~CscObjNameCache(void) throw();

        void Add(const CscObject *pObject, const CscObjDispInfo& di);
        bool Retrieve(const CscObject *pObject, CscObjDispInfo *pdi) throw();
        void Clear(void) throw();

#if DBG
        void Dump(void);
#endif

    private:
        //
        // A single entry in the cache list.
        // The link inherited from class Link is used for linking into
        // the hash bucket.
        //
        class Entry
        {
            public:
                Entry(const CscObject *pObject, const CscObjDispInfo& di)
                    : m_pNIB(NULL),
                      m_pPIB(NULL),
                      m_pNIM(NULL),
                      m_pPIM(NULL),
                      m_pObject(pObject),
                      m_pDispInfo(new CscObjDispInfo(di)) { }

                Entry(void) throw ()
                    : m_pNIB(NULL),
                      m_pPIB(NULL),
                      m_pNIM(NULL),
                      m_pPIM(NULL),
                      m_pObject(NULL),
                      m_pDispInfo(NULL) { }

                ~Entry(void) throw() { delete m_pDispInfo; }

                Entry *m_pNIB; // Link to next in bucket.
                Entry *m_pPIB; // Link to prev in bucket.
                Entry *m_pNIM; // Link to next in mru list.
                Entry *m_pPIM; // Link to prev in mru list.

                const CscObject *m_pObject;   // CSC object key (obj ptr).
                CscObjDispInfo  *m_pDispInfo; // Cached display information.

                //
                // Prevent copy.
                //
                Entry(const Entry& rhs);
                Entry& operator = (const Entry& rhs);
        };

        Entry m_ZNodeMru;       // Dummy "head" node for mru list.
        Entry *m_rgHashBuckets; // Each bucket has a dummy head node.
        int m_cEntries;         // Entries in cache.
        int m_cMaxEntries;      // Max entries in the cache.
        int m_cHashBuckets;     // Max buckets in hash table array.
        CCriticalSection m_cs;  // For thread sync control.

        void AppendInBucket(Entry& AppendThis, Entry& Target) throw();
        void DetachFromBucket(Entry& DetachThis) throw();
        void AppendInMru(Entry& AppendThis, Entry& Target) throw();
        void DetachFromMru(Entry& DetachThis) throw();
        void PromoteInMru(Entry& entry) throw();
        int Hash(const CscObject *m_pObject) throw();
        Entry *Find(const CscObject *pObject, int *piHashBucket) throw();
        void DetachAndDeleteEntry(Entry& entry) throw();

        //
        // Prevent copy.
        //
        CscObjNameCache(const CscObjNameCache& rhs);
        CscObjNameCache& operator = (const CscObjNameCache& rhs);
};

class CscObjTreeIterator; // fwd decl.

//
// The tree is really just a "parent" object with some additional stuff.
//
class CscObjTree : public CscObjParent
{
    public:
        CscObjTree(int cMaxNameCacheEntries, int cNameCacheHashBuckets);
        ~CscObjTree(void);

        //
        // Load a single share into the object tree.  
        //
        HRESULT LoadShare(const CString& strShare);
        //
        // Load multilple shares into the object tree.
        //
        HRESULT LoadShares(const CArray<CString>& rgstrShares);
        //
        // Load all CSC shares into the object tree.
        //
        HRESULT LoadAllShares(void);
        //
        // Empty and reload the share objects.
        //
        void Refresh(void);

        void PauseLoading(void)
            { m_LoadMgr.PauseLoad(); }
            
        void ResumeLoading(void)
            { m_LoadMgr.ResumeLoad(); }

        int GetObjectCount(void) const;

        int GetLoadedObjectCount(void) const
            { return m_cObjectsLoaded; }

        //
        // Find a specific share in the tree.
        //
        CscShare *FindShare(const UNCPath& path) const;
        bool ShareExists(const UNCPath& path) const;
        //
        // Retrieve a list of shares from the CSC database.
        // Returns list even if tree is empty.
        //
        void GetShareList(CArray<CscShare *> *prgpShares, bool bSort = true) const;
        //
        // Get the locally-mapped drive letter for a network share.
        // TEXT('\0') means "share not mapped".
        //
        TCHAR GetShareDriveLetter(const CString& strShare)
            { return g_TheCnxNameCache.GetShareDriveLetter(strShare); }
        //
        // Retrieve name information for a given object in the tree.
        //
        void GetObjDispInfo(const CscObject *pObject, CscObjDispInfo *pdi, DWORD fMask) const;

        virtual int IsA(void) const throw()
            { return CscObject::Root; }

        virtual void GetDispInfo(CscObjDispInfo *pdi, DWORD fMask, bool bCalledByChild = false) const { /* empty */ }

        //
        // BUGBUG:  This function counts objects.  Is it obsolete now that we're
        //          keeping m_cObjects?
        //
        int Count(void) const;

#if DBG
        void Dump(void);
#endif

    private:
        int     m_cObjectsLoaded; // Count of objects loaded.
        CString m_strPath;        // Path used for FindFirst/FindNext.
        CString m_strFileSpec;    // Filespec used for FindFirst/FindNext.

        mutable CscObjNameCache  m_NameCache;    // Cache of object name info.

#if DBG
        mutable int m_cNameCacheHits;
        mutable int m_cNameCacheRequests;
#endif
        //
        // Cancel all loading and empty the object tree.
        //
        void Clear(void);
        //
        // Cancel the loading operation.
        //
        void CancelLoad(void);
        //
        // Initialize the object tree with shares from the CSC database.
        // Share objects only, no children.
        //
        void LoadCscShares(void);

        void IncrLoadedObjectCount(void) throw()
            { m_cObjectsLoaded++; }
        //
        // Prevent copy.
        //
        CscObjTree(const CscObjTree& rhs);
        CscObjTree& operator = (const CscObjTree& rhs);

        //
        // Internal private class to manage loading of shares into the 
        // object tree.  The LoadMgr schedules a "Worker" object to 
        // handle the loading of each share.  Only one share is actually
        // loading at any given point in time.  If one share is currently
        // loading when another share is requested, the loading share
        // is suspended while the new share is loaded.  
        //
        class LoadMgr
        {
            public:
                explicit LoadMgr(CscObjTree& tree)
                    : m_tree(tree),
                      m_pCurrentWorker(NULL) { }

                ~LoadMgr(void);            

                HRESULT LoadShare(const CString& strShare);
                HRESULT LoadAllShares(void);

                void CancelLoad(void);

                void PauseLoad(void)
                    { m_mutex.Wait(); }

                void ResumeLoad(void)
                    { m_mutex.Release(); }

                void UseTeamWork(bool bTeamWork);

                //
                // Worker calls this when it's a member of a team and it is through
                // loading its share.  Like passing a baton in a relay race.
                //
                bool ActivateNextTeamMember(void);

                //
                // Internal public class designed to handle the loading of a 
                // single share into the object tree.  Each worker runs on 
                // its own thread.  Only one worker is working at any given
                // time.  This is enforced by the LoadMgr.
                //
                class Worker
                {
                    public:
                        enum State { eStopped, eSuspended, eRunning, eDone };

                        Worker(LoadMgr& manager, CscObjTree& tree, const CString& strShare, bool bTeam = false);
                        ~Worker(void);

                        void CancelLoad(void) throw()
                            { m_bLoadCancelled = true; }

                        State GetState(void) const throw()
                            { return m_eState; }

                        void GetShare(CString *pstrShare) const
                            { DBGASSERT((NULL != pstrShare)); *pstrShare = m_strShare; }

                        HANDLE GetWorkAuthorizationHandle(void) throw()
                            { return m_WorkAuth.Handle(); }

                        void RevokeWorkAuthorization(void) throw()
                            { m_WorkAuth.Reset(); }

                        void GrantWorkAuthorization(void) throw()
                            { m_WorkAuth.Set(); }

                        void WaitUntilFinished(void)
                            { if (NULL != m_hThread) WaitForSingleObject(m_hThread, INFINITE); }

                        void WakeUp(void) throw()
                            { ResumeThread(m_hThread); }

                        void JoinTheTeam(bool bTeamMember) throw()
                            { m_bIsTeamMember = bTeamMember; }

                    private:
                        CscObjTree& m_tree;           // The tree I'm loading into.
                        CString     m_strShare;       // Name of share I'm loading.
                        LoadMgr&    m_manager;        // The manager I'm working for.
                        HANDLE      m_hThread;        // My thread handle.
                        DWORD       m_idThread;       // My thread ID.
                        CEvent      m_WorkAuth;       // My "I can/can't work now" flag.
                        State       m_eState;         // My current state.
                        bool        m_bLoadCancelled; // Load has been cancelled?
                        bool        m_bIsTeamMember;  // Is member of team loading all shares?

                        static UINT WINAPI ThreadProc(LPVOID pvParam);

                        HRESULT LoadShare(const CString& strShare);

                        HRESULT LoadSubTree(CscObjParent *pParent, const CString& strPath);

                        void WaitForJobApproval(void)
                            { m_eState = eSuspended; m_manager.WaitForJobApproval(*this); m_eState = eRunning; }

                        void JobComplete(void)
                            { m_manager.JobComplete(*this); }

                        static void AppendFileToPath(LPCTSTR pszPath, LPCTSTR pszFile, CString *pstrFullPath);
                        static bool IsDirectory(const WIN32_FIND_DATA& fd) throw();
                        static bool IsDotOrDotDot(const WIN32_FIND_DATA& fd) throw();

                        //
                        // Prevent copy.
                        //
                        Worker(const Worker& rhs);
                        Worker& operator = (const Worker& rhs);
                };

                //
                // Worker calls this to get manager approval for a "job".
                //
                void WaitForJobApproval(Worker& worker);
                //
                // Worker calls this when the "job" is complete.
                //
                void JobComplete(Worker& worker);

            private:
                CscObjTree&      m_tree;           // The tree we're working on.
                CArray<Worker *> m_rgpWorkers;     // My crew of workers.
                Worker          *m_pCurrentWorker; // The current one working.
                CMutex           m_mutex;          // Enforce only 1 worker working.
                bool             m_bUseTeamWork;   // Workers team up to load all shares.

                Worker *FindWorker(const CString& strShare);
                void DestroyWorkers(void);

                //
                // Manager calls this to assign the next "job" to a worker.
                //
                void ActivateWorker(Worker& worker);
                //
                // Manager calls this when the attention of all workers is reqd.
                //
                void WaitForWorkerAttention(void);
                //
                // Wait until a worker is finished.
                //
                void WaitForWorkerToFinish(Worker& worker)
                    { worker.WaitUntilFinished(); }
                //
                // Cancel a specific worker.
                //
                void CancelWorker(Worker& worker);

                //
                // Prevent copy.
                //
                LoadMgr(const LoadMgr& rhs);
                LoadMgr& operator = (const LoadMgr& rhs);

        };
        
        mutable LoadMgr m_LoadMgr;

        friend class CscObjParent;
        friend class CscShare;
};



inline bool
CscObjTree::LoadMgr::Worker::IsDirectory(
    const WIN32_FIND_DATA& fd
    ) throw()
{
    return 0 != (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}



inline bool
CscObjTree::LoadMgr::Worker::IsDotOrDotDot(
    const WIN32_FIND_DATA& fd
    ) throw()
{
    if (TEXT('.') == fd.cFileName[0])
    {
        return (TEXT('\0') == fd.cFileName[1]) ||
               (TEXT('.') == fd.cFileName[1] && TEXT('\0') == fd.cFileName[2]);
    }
    return false;
}


//
// Simple iterator for enumerating all objects in a given CscObject tree or
// subtree.  The class maintains enumeration context by keeping a stack of
// child object iterators for each folder visited.  See Next() for details.
//
class CscObjTreeIterator : public CscObjIterator
{
    public:
        explicit CscObjTreeIterator(const CscObject& tree, DWORD fExclude = EXCLUDE_NONE);
        virtual ~CscObjTreeIterator(void);

        virtual bool Next(CscObject **ppObjOut);

        virtual void Reset(void);

    private:
        //
        // Array of child iterators maintains a stack of iteration 
        // context while traversing the tree.
        //
        CArray<CscObjIterator> m_rgChildIterators;
        DWORD m_fExclude;
        const CscObject& m_tree;
        //
        // Prevent copy.  Too complex.
        //
        CscObjTreeIterator(const CscObjTreeIterator& rhs);
        CscObjTreeIterator& operator = (const CscObjTreeIterator& rhs);
};




#endif // _INC_CSCVIEW_OBJTREE_H

