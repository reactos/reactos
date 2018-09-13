///////////////////////////////////////////////////////////////////////////////
/*  File: objtree.cpp

    Description: Implements a tree structure mirroring the file hierarchy
        of the CSC database entries.  Each entry in the tree represents 
        either a network share, directory (folder) or file.  All entries
        are derived from the common base class CscObject.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#pragma hdrstop

#include <process.h>
#include "objtree.h"
#include "thdsync.h"
#include "cscutils.h"

//
// The object tree was designed with several objectives:
//
// 1. Provide a mirror of CSC database contents.
// 2. Representation should be space efficient.
// 3. Population and retrieval should be time efficient.
// 4. Population should not block a client UI.
// 5. UI should control retrieval operations.
//
// The design satisfies these objectives as follows:
//
// 1. Tree structure creates share-folder-file hierarchy.
// 2. Use of reference-counted CString class is space efficient.
// 3. Caching of display info used to help retrieval time.
// 4. Population of each share is placed on a separate thread.
// 5. Iterators are provided for client UI use during retrieval.
//
//
// An important part of this design was to support the client UI's ability
// to select a new share for display at any time with little or no delay
// in updating the view.  To meet this goal, I decided to place the
// responsibility of loading a particular share on a unique instance
// of a "Worker" object.  Each share to be loaded is assigned to a new 
// worker.  Each worker runs on it's own thread but only one worker is
// running at any one time.  A single controlling "Load Manager" object 
// is created as a member of the tree to coordinate the activities of
// the worker objects.  This coordination is performed with a Win32 
// event object for each worker and a single Win32 mutex object.  The
// mutex ensures that only one object (worker or manager) is operating
// on the tree at any one time.  The worker event object allows the
// manager to control which worker is allowed to work loading the tree.  
//
// The worker's loading code is designed to wait at a known location until 
// the worker's event is "set" and the worker obtains the mutex.  If both 
// conditions are true, then the worker is allowed to load one more 
// file/folder.  When that particular file/folder is loaded, the worker 
// notifies the manager that the job is complete then waits again for 
// approval to load the next file/folder.  This work-and-wait behavior ensures
// that worker threads are either waiting in a known location or are
// performing work on the tree.  At no time is a worker thread suspended
// while loading a file/folder.  This ensures that the loading of a single
// file/folder is considered an atomic operation with respect to the
// tree's public interface.
//
// The following table illustrates this manager-worker coordination.
// The vertical line to the left of each section indicates which object
// is allowed to work on the tree at the given time period.
//
//
//  LoadMgr                 WorkerA                     WorkerB
//  ----------------------- --------------------------- -----------------------
//    Wait for mutex.
//  * Acquire mutex.
//  | Create WorkerA          Wait on mutex and event.
//  | Set WorkerA event
//  - Release mutex.        * Acquire mutex/event sig.
//                          | Add file to obj tree.
//                          | Release mutex.
//                          | Wait on mutex and event.
//                          | Acquire mutex.
//                          | Add file to obj tree.
//                          | Release mutex.
//                          - Wait on mutex and event.
//    Wait for mutex.       
//  * Acquire mutex.
//  | Create WorkerB                                      Wait on mutex and event
//  | Reset WorkerA event
//  | Set WorkerB event                                 
//  - Release mutex.                                    * Acquire mutex/event sig.
//                                                      | Add file to obj tree
//                                                      | Release mutex.
//                                                      - Wait on mutex and event
//    Wait for mutex.
//  * Acquire mutex.
//  | Reset WorkerB event
//  | Set WorkerA event
//  - Release mutex.        * Acquire mutex/event sig.
//                          | Add file to obj tree
//                          | Release mutex.
//                          | Wait on mutex and event.
//
//
//            <<<<  CONTINUED UNTIL TREE IS LOADED or DESTROYED >>>>
//


//-----------------------------------------------------------------------------
// class CscObject
//-----------------------------------------------------------------------------
//
// Returns NULL if "this" is a tree root.
// Returns "this" if "this" is a share.
// Otherwise returns pointer to parent share object.
//
CscShare *
CscObject::GetShare(
    void
    ) const throw()
{
    CscObject *po = const_cast<CscObject *>(this);
    if (!po->IsShare())
    {
        while(NULL != po && !po->IsShare())
            po = po->GetParent();
    }
    return static_cast<CscShare *>(po);
}


bool 
CscObject::IsShareOnLine(
    void
    ) const throw()
{ 
    return GetShare()->IsConnected(); 
}


//
// Is "this" a descendant of "pObject" in the object tree?
//
bool 
CscObject::IsDescendantOf(
    const CscObject *pObject
    ) const
{
    DBGASSERT((NULL != pObject));
    CscObject *pTemp = GetParent();

    while(pTemp && pTemp != pObject)
        pTemp = pTemp->GetParent();

    return NULL != pTemp;
}

//
// Pin or unpin the object.
//
HRESULT
CscObject::Pin(
    bool bPin
    )
{
    HRESULT hr = S_FALSE;
    //
    // Only pin/unpin if the request is to change the pinned state.
    //
    if (bPin != IsPinned())
    {
        //
        // "PPF" means pointer to pinning function.
        //
        typedef BOOL (WINAPI * PPF)(LPCTSTR, DWORD, LPDWORD, LPDWORD, LPDWORD);

        const PPF rgPinFunc[] = { CSCUnpinFile, CSCPinFile };

        DWORD dwStatus    = 0;
        DWORD dwPinCount  = 0;
        DWORD dwHintFlags = FLAG_CSC_HINT_PIN_USER;
        CString strPath;

        if (IsFolder())
            dwHintFlags |= FLAG_CSC_HINT_PIN_INHERIT_USER;

        GetFullPath(&strPath);
        if (rgPinFunc[bPin](strPath,
                            dwHintFlags,
                            &dwStatus,
                            &dwPinCount,
                            &dwHintFlags))
        {
            GetShare()->AdjustPinnedFileCount(bPin ? 1 : -1);
            //
            // Note:  m_wCscPinState is modified in the derived
            //        class version of Pin().
            //
            hr = NOERROR;
        }
        else 
        {
            DWORD dwError = GetLastError();
            //
            // HACK:  The CSC APIs don't always correctly set the LastError
            //        when they fail.  Sometimes, LastError is 0 even when
            //        the APIs return FALSE.  This forces the return value to
            //        an error code so that callers don't think the operation
            //        succeeded when it really failed.
            //        BUGBUG:  This should be fixed once Shishir has the
            //                 CSC code working properly.
            //
            if (ERROR_SUCCESS == dwError)
            {
                DBGERROR((TEXT("CSCPinFile failed but returned ERROR_SUCCESS!")));
                dwError = ERROR_GEN_FAILURE;
            }
            hr = HRESULT_FROM_WIN32(dwError);
        }
    }
    return hr;
}


void 
CscObject::LockAncestors(
    void
    ) const throw()
{
    if (NULL != m_pParent)
    {
        reinterpret_cast<CscObject *>(m_pParent)->LockAncestors(); 
        reinterpret_cast<CscObject *>(m_pParent)->AddRef(); 
    }
}

void 
CscObject::ReleaseAncestors(
    void
    ) const throw()
{
    if (NULL != m_pParent)
    {
        reinterpret_cast<CscObject *>(m_pParent)->ReleaseAncestors();
        reinterpret_cast<CscObject *>(m_pParent)->Release();
    }
}                             

#if DBG
void
CscObject::Dump(
    void
    )
{
    CscObjDispInfo name;
    GetDispInfo(&name, ODI_ALL);
    DBGPRINT((TEXT("Dumping CscObject @ 0x%08X"), (DWORD)this));
    DBGPRINT((TEXT("\t\\\\%s\\%s%s%s%s"), (LPCTSTR)name.m_strServer,
                                          (LPCTSTR)name.m_strShare,
                                          (LPCTSTR)name.m_strPath,
                                          name.m_strFile.IsEmpty() ? TEXT("") : TEXT("\\"),
                                          (LPCTSTR)name.m_strFile));
    CString strScratch;
    if (IsShare())
    {
        DBGPRINT((TEXT("\tDisplayName......: = %s"), (LPCTSTR)name.m_strShareDisplayName));
        static_cast<CscShare *>(this)->GetLongDisplayName(&strScratch);
        DBGPRINT((TEXT("\tLong DisplayName.: = %s"), (LPCTSTR)strScratch));
        DBGPRINT((TEXT("\tStatus...........: = %s"), (LPCTSTR)name.m_strShareStatus));
    }

    DBGPRINT((TEXT("\tpObject..........: = 0x%08X"), (DWORD)this));
    DBGPRINT((TEXT("\tpParent..........: = 0x%08X"), (DWORD)GetParent()));
    DBGPRINT((TEXT("\tIsParent.........: = %s"), IsParent() ? TEXT("YES"):TEXT("NO")));
    DBGPRINT((TEXT("\tCanBeParent......: = %s"), CanBeParent() ? TEXT("YES"):TEXT("NO")));
    DBGPRINT((TEXT("\tChild count......: = %d"), CountChildren()));
    GetFileTimeString(&strScratch);
    DBGPRINT((TEXT("\tDate.............: = %s"), (LPCTSTR)strScratch));
    GetFileSizeString(&strScratch);
    DBGPRINT((TEXT("\tSize.............: = %s"), (LPCTSTR)strScratch));
}
#endif // DBG




//-----------------------------------------------------------------------------
// class CscFile
//-----------------------------------------------------------------------------
//
// CscFile ctor.
//
CscFile::CscFile(
    CscObjParent *pParent, 
    const CString& strName, 
    WORD wStatus,
    WORD wPinState,
    const FileTime& LastModified, 
    const FileSize& FileSize
    ) : CscObject(pParent, wStatus, wPinState),
        m_strName(strName),
        m_LastModified(LastModified),
        m_FileSize(FileSize)
{

}


//
// Retrieve the display information for a file object.
//
void CscFile::GetDispInfo(
    CscObjDispInfo *pdi,
    DWORD fMask,
    bool bCalledByChild
    ) const
{
    DBGASSERT((NULL != pdi));
    //
    // Get display information from parent folder and share.
    //
    GetParent()->GetDispInfo(pdi, fMask, true);

    //
    // Add file's display information.
    //
    if (ODI_FILE & fMask)
        pdi->m_strFile = m_strName;

    if (ODI_FILETIME & fMask)
        GetFileTimeString(&pdi->m_strFileTime);

    if (ODI_FILESIZE & fMask)
        GetFileSizeString(&pdi->m_strFileSize);

    if (ODI_STALEREASON & fMask)
        GetStaleDescription(&pdi->m_strStaleReason);

    //
    // Set the mask bits in the display info struct indicating
    // what values are now present.
    //
    pdi->m_fMask |= ((ODI_FILE | ODI_FILETIME | ODI_FILESIZE | ODI_STALEREASON) & fMask);

}


//
// Create a full path to the file.
//
void 
CscFile::GetFullPath(
    CString *pstrPath
    ) const
{
    DBGASSERT((NULL != pstrPath));

    CscObjDispInfo di;
    GetDispInfo(&di, ODI_SERVER | ODI_SHARE | ODI_PATH | ODI_FILE);

    pstrPath->Format(TEXT("\\\\%1\\%2%3\\%4"),
                     (LPCTSTR)di.m_strServer,
                     (LPCTSTR)di.m_strShare,
                     (LPCTSTR)di.m_strPath,
                     (LPCTSTR)di.m_strFile);
}


void 
CscFile::GetPath(
    CString *pstrPath
    ) const
{
    DBGASSERT((NULL != pstrPath));

    CscObjDispInfo di;
    GetDispInfo(&di, ODI_PATH | ODI_FILE);

    pstrPath->Format(TEXT("%1\\%2"),
                     (LPCTSTR)di.m_strPath,
                     (LPCTSTR)di.m_strFile);
}

void 
CscFile::GetStaleDescription(
    CString *pstrDesc
    ) const
{
    DBGASSERT((NULL != pstrDesc)); 
    if (NeedToSync())
    {
        pstrDesc->Format(Viewer::g_hInstance, GetStaleReasonCode());
    }
    else
    {
        pstrDesc->Empty();
    }
}

//
// Update the CSC-specific information for the object.
// Typically called after a sychronize operation.
//
void
CscFile::UpdateCscInfo(
    void
    )
{
    DBGTRACE((DM_OBJTREE, DL_MID, TEXT("CscFile::UpdateCscInfo")));

    WIN32_FIND_DATA fd;
    DWORD dwStatus = 0;
    DWORD dwHintFlags = 0;
    DWORD dwPinCount = 0;
    FILETIME ft;

    bool bWasPinned    = IsPinned();
    bool bNeededToSync = NeedToSync();

    CString strPath;
    GetFullPath(&strPath);

    CscFindHandle hFind(CSCFindFirstFile(strPath, 
                                         &fd,
                                         &dwStatus,
                                         &dwPinCount,
                                         &dwHintFlags,
                                         &ft));
    
    if (!hFind.IsValidHandle())
    {
        DBGERROR((TEXT("Error 0x%08X for CSCFindFirstFile(\"%s\")"), 
                 GetLastError(), strPath.Cstr()));
    }
    else
    {
        m_wCscStatus   = LOWORD(dwStatus);
        m_wCscPinState = LOWORD(dwHintFlags);
        m_LastModified = FileTime(fd.ftLastWriteTime);
        m_FileSize     = FileSize(MAKEULONGLONG(fd.nFileSizeHigh, fd.nFileSizeLow));
        CscShare *pShare = GetShare();
        if (bWasPinned != IsPinned())
            pShare->AdjustPinnedFileCount(IsPinned() ? +1 : -1);
        if (bNeededToSync != NeedToSync())
            pShare->AdjustStaleFileCount(NeedToSync() ? +1 : -1);
    }
}


HRESULT
CscFile::Pin(
    bool bPin
    )
{
    //
    // Call the base class implementation to do the actual
    // pinning.  If successful, set/clear the "pinned" flag.
    //
    HRESULT hr = CscObject::Pin(bPin);
    if (SUCCEEDED(hr))
    {
        if (bPin)
            m_wCscPinState |= FLAG_CSC_HINT_PIN_USER;
        else
            m_wCscPinState &= ~FLAG_CSC_HINT_PIN_USER;
    }
    return hr;
}


//-----------------------------------------------------------------------------
// class CscObjParent
//-----------------------------------------------------------------------------

CscObjParent::~CscObjParent(
    void
    ) 
{ 
    DestroyChildren(); 
}



//
// Destroy all of a parent's child objects.
//
void
CscObjParent::DestroyChildren(
    void
    )
{
    CscObjIterator iter = CreateChildIterator(EXCLUDE_NONEXISTING);
    CscObject *pChild = NULL;

    while(iter.Next(&pChild))
    {
        if (pChild->IsParent())
            static_cast<CscObjParent *>(pChild)->DestroyChildren();
        pChild->Release();
    }

    delete[] m_prgChildren;
    m_prgChildren = NULL;
}


//
// Add an object to a parent's list of child objects.
//
void
CscObjParent::AddChild(
    CscObject *pChild
    )
{
    DBGASSERT((NULL != pChild));

    if (NULL == m_prgChildren)
    {
        //
        // Defer creating a child array until one is really needed.
        //
        m_prgChildren = new CArray<CscObject *>;
    }
    m_prgChildren->Append(pChild);
    CscShare *pShare = GetShare();
    if (NULL == pShare)
    {
        //
        // If GetShare returns NULL, we know this object is a tree root.
        // AddChild was called from LoadCscShares.
        //
        reinterpret_cast<CscObjTree *>(this)->IncrLoadedObjectCount();
    }
    else
    {
        //
        // Otherwise, increment the loaded file count for the parent share
        // object.  This will also increment the tree's loaded
        // object count.
        //
        pShare->IncrLoadedFileCount();
    }
}


//
// Delete one child object.  This is the only way to destroy
// a single object in the tree.
//
HRESULT
CscObjParent::DestroyChild(
    CscObject *pChild,
    bool bDeleteFromCache    // default == true
    )
{
    HRESULT hr = S_FALSE;
    if (NULL != m_prgChildren)
    {
        int n = m_prgChildren->Count();
        for (int i = 0; i < n; i++)
        {
            if (pChild == m_prgChildren->GetAt(i))
            {
                hr = S_OK;
                if (bDeleteFromCache)
                {
                    CString strPath;
                    pChild->GetFullPath(&strPath);
                    DWORD dwErr = CscDelete(strPath);
                    if (ERROR_SUCCESS == dwErr)
                    {
                        //
                        // Adjust the object's root share's 
                        // file counts.
                        //
                        CscShare *pShare = GetShare();
                        pShare->AdjustFileCount(-1);
                        if (pChild->IsPinned())
                            pShare->AdjustPinnedFileCount(-1);
                        if (pChild->NeedToSync())
                            pShare->AdjustStaleFileCount(-1);
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(dwErr);
                    }
                }
                if (SUCCEEDED(hr))
                {
                    DBGASSERT(!pChild->IsParent());
                    m_prgChildren->Delete(i);
                    pChild->Release();
                }
                //
                // Found child in parent's list.
                // Deletion either failed or succeeded.
                // Under normal circumstances, this should be the
                // exit point from the function.
                //
                return hr;
            }
        }
    }
    //
    // Parent has no children or couldn't find this
    // child in parent's list.  This isn't fatal.  Just
    // means that caller asked to delete a child that didn't
    // exist.  This can happen as a result of the viewer showing
    // a hierarchy as a flat list.  The user is able to select a folder
    // and files in that folder as part of the listview selection.  During
    // the deletion process, we automatically delete the decendants of a
    // folder.  If a decendant is directly selected for deletion via user
    // selection and also indirectly selected by user selection of an 
    // ancestor folder, DestroyChild() will be called twice.  This code path
    // will be executed the second call.
    //
    DBGPRINT((DM_OBJTREE, DL_LOW, TEXT("Tried to destroy non-existent child 0x%08X in parent 0x%08X"),
             this, pChild));
    return hr;
}


//
// Create and return an interator object for enumerating a 
// parent's child objects.
//
CscObjIterator
CscObjParent::CreateChildIterator(
    DWORD fExclude
    ) const throw()
{
    return CscObjIterator(this, fExclude);
}


//
// Is "pObject" a descendant of "this" in the object tree?
//
bool 
CscObjParent::IsDescendant(
    const CscObject *pObject
    ) const throw()
{
    DBGASSERT((NULL != pObject));
    return pObject->IsDescendantOf(static_cast<const CscObject *>(this));
}


//
// Count up all of the descendants of this object.
//
int
CscObjParent::CountDescendants(
    void
    ) const
{
    int n = 0;
    int cKids = n = CountChildren();
    for (int i = 0; i < cKids; i++)
        n += GetChild(i)->CountDescendants();

    return n;
}


//-----------------------------------------------------------------------------
// class CscFolder
//-----------------------------------------------------------------------------
//
// CscFolder ctor.
//
CscFolder::CscFolder(
    CscObjParent *pParent, 
    const CString& strName, 
    WORD wStatus,
    WORD wPinState,
    const FileTime& LastModified, 
    const FileSize& FileSize
    ) : CscObjParent(pParent, wStatus, wPinState),
        m_strName(strName),
        m_LastModified(LastModified),
        m_FileSize(FileSize)
{

}


//
// Retreive the display information for a folder object.
//
void CscFolder::GetDispInfo(
    CscObjDispInfo *pdi,
    DWORD fMask,
    bool bCalledByChild
    ) const
{
    DBGASSERT((NULL != pdi));

    //
    // First get the display information from the folder's parent(s).
    //
    GetParent()->GetDispInfo(pdi, fMask, true);

    //
    // Append this folder's path information.
    //
    if (ODI_PATH & fMask)
    {
        pdi->m_strPath += CString(TEXT("\\"));
        pdi->m_strPath += m_strName;
    }
    
    if (!bCalledByChild)
    {
        //
        // Caller wants info for a folder object (not a file object).
        //
        if (ODI_FILETIME & fMask)
            GetFileTimeString(&pdi->m_strFileTime);

        if (ODI_FILESIZE & fMask)
            GetFileSizeString(&pdi->m_strFileSize);

        if (ODI_STALEREASON & fMask)
            GetStaleDescription(&pdi->m_strStaleReason);

        pdi->m_fMask |= ((ODI_FILETIME | ODI_FILESIZE | ODI_STALEREASON) & fMask);
    }

    //
    // Set the mask in the display info struct to indicate
    // which parts now have valid data.
    //
    pdi->m_fMask |= (ODI_PATH & fMask);
}


//
// Create a full path to the folder
//
void 
CscFolder::GetFullPath(
    CString *pstrPath
    ) const
{
    DBGASSERT((NULL != pstrPath));

    CscObjDispInfo di;
    GetDispInfo(&di, ODI_SERVER | ODI_SHARE | ODI_PATH);

    pstrPath->Format(TEXT("\\\\%1\\%2%3"),
                     (LPCTSTR)di.m_strServer,
                     (LPCTSTR)di.m_strShare,
                     (LPCTSTR)di.m_strPath);
}


void 
CscFolder::GetPath(
    CString *pstrPath
    ) const
{
    DBGASSERT((NULL != pstrPath));

    CscObjDispInfo di;
    GetDispInfo(&di, ODI_PATH);
    *pstrPath = di.m_strPath;
}

void 
CscFolder::GetStaleDescription(
    CString *pstrDesc
    ) const
{ 
    DBGASSERT((NULL != pstrDesc)); 
    if (NeedToSync())
    {
        pstrDesc->Format(Viewer::g_hInstance, GetStaleReasonCode());
    }
    else
    {
        pstrDesc->Empty();
    }
}

void
CscFolder::UpdateCscInfo(
    void
    )
{
    DBGTRACE((DM_OBJTREE, DL_MID, TEXT("CscFolder::UpdateCscInfo")));
    WIN32_FIND_DATA fd;
    DWORD dwStatus = 0;
    DWORD dwHintFlags = 0;
    DWORD dwPinCount = 0;
    FILETIME ft;

    bool bWasPinned    = IsPinned();
    bool bNeededToSync = NeedToSync();

    CString strPath;
    GetFullPath(&strPath);

    CscFindHandle hFind(CSCFindFirstFile(strPath, 
                                         &fd,
                                         &dwStatus,
                                         &dwPinCount,
                                         &dwHintFlags,
                                         &ft));
    
    if (!hFind.IsValidHandle())
    {
        DBGERROR((TEXT("Error 0x%08X for CSCFindFirstFile(\"%s\")"), 
                 GetLastError(), strPath.Cstr()));
    }
    else
    {
        m_wCscStatus   = LOWORD(dwStatus);
        m_wCscPinState = LOWORD(dwHintFlags);
        m_LastModified = FileTime(fd.ftLastWriteTime);
        m_FileSize     = FileSize(MAKEULONGLONG(fd.nFileSizeHigh, fd.nFileSizeLow));
        CscShare *pShare = GetShare();
        if (bWasPinned != IsPinned())
            pShare->AdjustPinnedFileCount(IsPinned() ? +1 : -1);
        if (bNeededToSync != NeedToSync())
            pShare->AdjustStaleFileCount(NeedToSync() ? +1 : -1);
    }
}



HRESULT
CscFolder::Pin(
    bool bPin
    )
{
    //
    // Call the base class implementation to do the actual
    // pinning.  If successful, set/clear the "pinned" flag.
    //
    HRESULT hr = CscObject::Pin(bPin);
    if (SUCCEEDED(hr))
    {
        if (bPin)
            m_wCscPinState |= (FLAG_CSC_HINT_PIN_USER | FLAG_CSC_HINT_PIN_INHERIT_USER);
        else
            m_wCscPinState &= ~(FLAG_CSC_HINT_PIN_USER | FLAG_CSC_HINT_PIN_INHERIT_USER);
    }
    return hr;
}



//-----------------------------------------------------------------------------
// CscObjIterator
//-----------------------------------------------------------------------------
CscObjIterator::CscObjIterator(
    const CscObjParent *pParent, 
    DWORD fExclude
    ) throw()
      : m_pParent(pParent),
        m_iChild(0),
        m_fExclude(fExclude)
{ 
    DBGASSERT((NULL != pParent)); 
}



//
// Should object be exluded from enumeration?
//
bool 
CscObjIterator::ExcludeItem(
    CscObject *pObject
    ) const throw()
{
    bool bExclude = false;
    if (EXCLUDE_SPARSEFILES & m_fExclude)
    {
        //
        // Folders are always sparse so they are always included.
        //
        bExclude = pObject->IsFile() && pObject->IsSparse();
    }
    return bExclude;
}



//
// Retrieve the next CscObject from an object iterator.
// Returns true when an object is returned or if a subsequent call
// might return more items.
// Returns false when no more items are (or will be) available.
// Returned object pointers are passed through the pObjOut reference.
//
bool
CscObjIterator::Next(
    CscObject **ppObject
    )
{
    DBGASSERT((NULL != ppObject));

    bool bResult = false;

    *ppObject = NULL;

    if (NULL != m_pParent && m_pParent->CanBeParent())
    {
        //
        // The iterator references a parent object and that 
        // parent object in fact can have children.
        // Remember:  m_pParent is NULL for a "nul iterator".
        //
        if (NULL != m_pParent->m_prgChildren)
        {
            while(m_iChild < m_pParent->m_prgChildren->Count())
            {   
                *ppObject = (*(m_pParent->m_prgChildren))[m_iChild++];
                if (!ExcludeItem(*ppObject))
                {
                    bResult = true;
                    break;
                }
            }
            DBGPRINT((DM_OBJTREE, DL_LOW, TEXT("CscObjIterator::Next, Exhausted at child %d of %d"),
                     m_iChild, 
                     m_pParent->m_prgChildren->Count()));
        }
        if (!bResult && !ExistingOnly() && !m_pParent->IsComplete())
        {
            //
            // No more children to enumerate at this time but the parent object
            // is still incomplete.  Client should call again to see
            // if another child has been added.  This feature is provided
            // to support tree population and iteration on separate threads.
            //
            bResult = true;
            DBGPRINT((DM_OBJTREE, DL_LOW, TEXT("CscObjIterator::Next, Exhaustion is temporary")));
        }
    }
    return bResult;
}


//-----------------------------------------------------------------------------
// class CscShare
//-----------------------------------------------------------------------------

CscShare::CscShare(
    CscObjTree *pTreeRoot,
    const CString& strServer, 
    const CString& strShare,
    TCHAR chDrive,
    WORD wStatus,
    int cFiles, 
    int cPinnedFiles, 
    int cStale
    ) : CscObjParent(pTreeRoot, wStatus, 0),
        m_strServer(strServer),
        m_strShare(strShare),
        m_cFiles(cFiles),
        m_cPinnedFiles(cPinnedFiles),
        m_cStale(cStale),
        m_cFilesLoaded(0)
{
    DBGPRINT((DM_OBJTREE, DL_LOW, 
              TEXT("Created Share object \"\\\\%s\\%s\""), 
                   (LPCTSTR)strServer, (LPCTSTR)strShare));
    DBGPRINT((DM_OBJTREE, DL_LOW, 
              TEXT("\tchDrive......: %c"), chDrive > 31 ? chDrive : TEXT('?')));
    DBGPRINT((DM_OBJTREE, DL_LOW, 
              TEXT("\twStatus......: 0x%04X"), wStatus));
    DBGPRINT((DM_OBJTREE, DL_LOW, 
              TEXT("\tcFiles.......: %d"), cFiles));
    DBGPRINT((DM_OBJTREE, DL_LOW, 
              TEXT("\tcPinnedFiles.: %d"), cPinnedFiles));
    DBGPRINT((DM_OBJTREE, DL_LOW, 
              TEXT("\tcStale.......: %d"), cStale));


    if (TEXT('\0') != chDrive)
    {
        //
        // Drive character provided.  Format both long and short format
        // display strings.
        // 
        // Short = "ntspecs (E:)"
        // Long  = "ntspecs on 'worf' (E:)"
        //
        TCHAR szDrive[] = TEXT("?");
        szDrive[0] = chDrive;
        m_strShortDisplayName.Format(Viewer::g_hInstance, 
                                     IDS_FMT_SHARENAME_SHORT,
                                     (LPCTSTR)m_strShare,
                                     szDrive);

        m_strLongDisplayName.Format(Viewer::g_hInstance, 
                                    IDS_FMT_SHARENAME_LONG,
                                    (LPCTSTR)m_strShare,
                                    (LPCTSTR)m_strServer,
                                    szDrive);
    }
    else
    {
        //
        // Drive character not provided.  Format both long and short format
        // display strings.
        // 
        // Short = "ntspecs"
        // Long  = "ntspecs on 'worf'"
        //
        m_strShortDisplayName = m_strShare;
        m_strLongDisplayName.Format(Viewer::g_hInstance,
                                    IDS_FMT_SHARENAME_LONG_NODRIVE,
                                    (LPCTSTR)m_strShare,
                                    (LPCTSTR)m_strServer);
    }
    //
    // Share status is either "Connected" or "Disconnected".
    //
    m_strStatus.Format(Viewer::g_hInstance,
                       IsConnected() ? IDS_SHARE_STATUS_ONLINE : IDS_SHARE_STATUS_OFFLINE);
}



//
// Get display information for a share object.
// Note that since a share is the highest object in the object tree hierarchy
// (except for the root node), this function doesn't ask a parent for 
// it's display info.
//
void 
CscShare::GetDispInfo(
    CscObjDispInfo *pdi,
    DWORD fMask,
    bool bCalledByChild   // ignored.
    ) const
{
    DBGASSERT((NULL != pdi));

    if (ODI_ICON & fMask)
        pdi->m_iIcon = GetIconImageIndex();

    if (ODI_SERVER & fMask)
        pdi->m_strServer = m_strServer;

    if (ODI_SHARE & fMask)
        pdi->m_strShare = m_strShare;

    if (ODI_SHAREDISPNAME & fMask)
        pdi->m_strShareDisplayName = m_strShortDisplayName;

    if (ODI_SHARESTATUS & fMask)
        pdi->m_strShareStatus = m_strStatus;

    pdi->m_fMask = ((ODI_ICON | ODI_SERVER | ODI_SHARE | ODI_SHAREDISPNAME | ODI_SHARESTATUS) & fMask);

    //
    // Remove any residual info from elements that may or may not be
    // filled in upon return up the recursion chain.
    //
    pdi->m_strPath.Empty();
    pdi->m_strFile.Empty();
    pdi->m_strFileTime.Empty();
    pdi->m_strFileSize.Empty();
    pdi->m_strStaleReason.Empty();
}

//
// Retrieve the share's name as "\\Share\Server". 
//
void 
CscShare::GetName(
    CString *pstrName
    ) const
{
    DBGASSERT((NULL != pstrName));

    pstrName->Format(TEXT("\\\\%1\\%2"), (LPCTSTR)m_strServer, (LPCTSTR)m_strShare);
}


//
// Create a full path to the folder
//
void 
CscShare::GetFullPath(
    CString *pstrPath
    ) const
{
    DBGASSERT((NULL != pstrPath));
    //
    // For shares, GetName satisfies the requirement for a full path.
    //
    GetName(pstrPath);
}


void
CscShare::GetPath(
    CString *pstrPath
    ) const
{
    DBGASSERT((NULL != pstrPath));
    //
    // For shares, GetName satisfies the requirement for a full path.
    //
    GetName(pstrPath);
}    


void
CscShare::IncrLoadedFileCount(
    void
    ) throw()
{ 
    m_cFilesLoaded ++; 
    reinterpret_cast<CscObjTree *>(GetParent())->IncrLoadedObjectCount(); 
}


void
CscShare::UpdateCscInfo(
    void
    )
{
    DBGTRACE((DM_OBJTREE, DL_MID, TEXT("CscShare::UpdateCscInfo")));
    CscShareInformation si;
    CString strPath;

    GetFullPath(&strPath);
    CscGetShareInformation(strPath, &si);

    m_wCscStatus   = LOWORD(si.Status());
    m_cFiles       = si.TotalCount();
    m_cPinnedFiles = si.PinnedCount();
    m_cStale       = si.StaleCount();

    m_strStatus.Format(Viewer::g_hInstance,
                       IsConnected() ? IDS_SHARE_STATUS_ONLINE : IDS_SHARE_STATUS_OFFLINE);

}



//-----------------------------------------------------------------------------
// class CscObjNameCache
//-----------------------------------------------------------------------------
//
// Cache ctor.
//
CscObjNameCache::CscObjNameCache(
    int cMaxEntries,
    int cHashBuckets
    ) : m_cEntries(0),
        m_cMaxEntries(cMaxEntries),
        m_cHashBuckets(cHashBuckets),
        m_rgHashBuckets(new Entry[cHashBuckets])
{
    DBGTRACE((DM_OBJTREE, DL_MID, TEXT("CscObjNameCache::CscObjNameCache")));
    DBGPRINT((DM_OBJTREE, DL_MID, 
              TEXT("Creating object name cache.  Max entries = %d, Hash buckets = %d"),
              m_cMaxEntries, m_cHashBuckets));

    //
    // Initialize cache MRU list by having the ZNode point to itself.
    //
    m_ZNodeMru.m_pPIM = m_ZNodeMru.m_pNIM = &m_ZNodeMru;
    //
    // Initialize the hash buckets by having each array entry point to itself.
    // Remember, each bucket array entry is a head node with no data.
    //
    for (int i = 0; i < cHashBuckets; i++)
    {
        m_rgHashBuckets[i].m_pPIB = m_rgHashBuckets[i].m_pNIB = &m_rgHashBuckets[i];
    }
}


//
// Cache dtor.
//
CscObjNameCache::~CscObjNameCache(
    void
    ) throw()
{
    DBGTRACE((DM_OBJTREE, DL_MID, TEXT("CscObjNameCache::~CscObjNameCache")));

    Clear();
    delete[] m_rgHashBuckets;
}


//
// Clear the cache so that it's empty, ready to accept new entries.
//
void
CscObjNameCache::Clear(
    void
    ) throw()
{
    //
    // Delete each node in the hash buckets.
    //
    AutoLockCs lock(m_cs);
    if (NULL != m_rgHashBuckets)
    {
        for (int i = 0; i < m_cHashBuckets; i++)
        {
            Entry& bucket = m_rgHashBuckets[i];
            while (&m_rgHashBuckets[i] != bucket.m_pNIB)
            {
                Entry *pDeleteThis = bucket.m_pNIB;
                bucket.m_pNIB = pDeleteThis->m_pNIB;
                delete pDeleteThis;
            }
            bucket.m_pPIB = bucket.m_pNIB = &m_rgHashBuckets[i];
        }
    }
    //
    // Reset the MRU list and entry count.
    //
    m_ZNodeMru.m_pPIM = m_ZNodeMru.m_pNIM = &m_ZNodeMru;
    m_cEntries = 0;
}



void 
CscObjNameCache::AppendInBucket(
    Entry& AppendThis,
    Entry& Target
    ) throw()
{
    AppendThis.m_pPIB = &Target;
    AppendThis.m_pNIB = Target.m_pNIB;
    AppendThis.m_pNIB->m_pPIB = Target.m_pNIB = &AppendThis;
}


void 
CscObjNameCache::DetachFromBucket(
    Entry& DetachThis
    ) throw()
{
    DetachThis.m_pPIB->m_pNIB = DetachThis.m_pNIB;
    DetachThis.m_pNIB->m_pPIB = DetachThis.m_pPIB;

    DetachThis.m_pPIB = DetachThis.m_pNIB = NULL;
}


void 
CscObjNameCache::AppendInMru(
    Entry& AppendThis,
    Entry& Target
    ) throw()
{
    AppendThis.m_pPIM = &Target;
    AppendThis.m_pNIM = Target.m_pNIM;
    AppendThis.m_pNIM->m_pPIM = Target.m_pNIM = &AppendThis;
}


void 
CscObjNameCache::DetachFromMru(
    Entry& DetachThis
    ) throw()
{
    DetachThis.m_pPIM->m_pNIM = DetachThis.m_pNIM;
    DetachThis.m_pNIM->m_pPIM = DetachThis.m_pPIM;

    DetachThis.m_pPIM = DetachThis.m_pNIM = NULL;
}


int
CscObjNameCache::Hash(
    const CscObject *pObject
    ) throw()
{
    return (DWORD)pObject % m_cHashBuckets;
}


CscObjNameCache::Entry *
CscObjNameCache::Find(
    const CscObject *pObject,
    int *piHashBucket
    ) throw()
{
    DBGASSERT((NULL != piHashBucket));

    *piHashBucket = Hash(pObject);
    Entry *pEntry = m_rgHashBuckets[*piHashBucket].m_pNIB;
    while(&m_rgHashBuckets[*piHashBucket] != pEntry && pEntry->m_pObject != pObject)
    {
        pEntry = pEntry->m_pNIB;
    }
    return pEntry == &m_rgHashBuckets[*piHashBucket] ? NULL : pEntry;
}


void
CscObjNameCache::PromoteInMru(
    Entry& entry
    ) throw()
{
    DetachFromMru(entry);
    AppendInMru(entry, m_ZNodeMru);
}


void
CscObjNameCache::DetachAndDeleteEntry(
    Entry& entry
    ) throw()
{
    DetachFromMru(entry);
    DetachFromBucket(entry);
    delete &entry;
}


//
// Create a new cache entry and add it to the cache.
// Items are added to the head of the cache list.  If the list is full,
// the last item is removed from the list and deleted.
//
void 
CscObjNameCache::Add(
    const CscObject *pObject, 
    const CscObjDispInfo& di
    )
{
    int iHashBucket;
    Entry *pEntry = Find(pObject, &iHashBucket);
    if (NULL != pEntry)
    {
        //
        // Set display info for existing entry.
        //
        *pEntry->m_pDispInfo = di;
    }
    else
    {
        //
        // Create a new cache entry.
        //
        Entry *pEntry = new Entry(pObject, di);
        AutoLockCs lock(m_cs);

        if (m_cMaxEntries <= m_cEntries++)
        {
            //
            // Cache is full.  Remove oldest entry.
            //
            m_cEntries--;
            DetachAndDeleteEntry(*m_ZNodeMru.m_pPIM);
        }
        //
        // Add new entry to cache.
        //
        AppendInBucket(*pEntry, m_rgHashBuckets[iHashBucket]);
        AppendInMru(*pEntry, m_ZNodeMru);
    }
}



//
// Retrieve display information from the cache.  pObject is the "key" 
// value used to locate the desired entry.  If a match is found, the
// display information is returned through "diOut" and the entry is
// moved to the head of the list.  This ensures that the most requested
// items have the best chance of remaining in the cache.
// If a match is found, the function returns true.  Otherwise, false.
//
bool 
CscObjNameCache::Retrieve(
    const CscObject *pObject, 
    CscObjDispInfo *pdi
    ) throw()
{
    DBGASSERT((NULL != pdi));

    int iHashBucket;
    Entry *pEntry = Find(pObject, &iHashBucket);
    if (NULL != pEntry)
    {
        *pdi = *pEntry->m_pDispInfo;
        PromoteInMru(*pEntry);
    }
    return NULL != pEntry;
}


//
// For development.
//
#if DBG
void CscObjNameCache::Dump(
    void
    )
{
    DBGPRINT((TEXT("Dumping CscObjNameCache @ 0x%08X"), (DWORD)this));
    int i = 0;

    for (Entry *pEntry = m_ZNodeMru.m_pNIM; &m_ZNodeMru != pEntry; pEntry = pEntry->m_pNIM)
    {
        DBGPRINT((TEXT("Entry[%3d]: Object @ 0x%08X"), i++, pEntry->m_pObject));
        if (NULL != pEntry->m_pDispInfo)
        {
            DBGPRINT((TEXT("\tName = \\\\%s\\%s%s%c%s"),
                      (LPCTSTR)pEntry->m_pDispInfo->m_strServer,
                      (LPCTSTR)pEntry->m_pDispInfo->m_strShare,
                      (LPCTSTR)pEntry->m_pDispInfo->m_strPath,
                      !pEntry->m_pDispInfo->m_strPath.IsEmpty() ? TEXT('\\') : TEXT('\0'),
                      (LPCTSTR)pEntry->m_pDispInfo->m_strFile));
            DBGPRINT((TEXT("\tDate = %s"), (LPCTSTR)(pEntry->m_pDispInfo->m_strFileTime)));
            DBGPRINT((TEXT("\tSize = %s"), (LPCTSTR)(pEntry->m_pDispInfo->m_strFileSize)));
        }
    }
}
#endif // DBG


//-----------------------------------------------------------------------------
// class CscObjTree
//-----------------------------------------------------------------------------
//
// CscObjTree ctor.
//
//
// Use of 'this' in base member initializer list is intentional.
// Disable the compiler warning.
// 
#pragma warning (disable : 4355)

CscObjTree::CscObjTree(
    int cMaxNameCacheEntries,
    int cNameCacheHashBuckets
    ) : CscObjParent(NULL, 0, 0),
        m_NameCache(cMaxNameCacheEntries, cNameCacheHashBuckets),
        m_LoadMgr(*this),
        m_cObjectsLoaded(0)
{
    DBGTRACE((DM_OBJTREE, DL_HIGH, TEXT("CscObjTree::CscObjTree")));
    DBGPRINT((DM_OBJTREE, DL_HIGH, TEXT("\tMaxNameCacheEntries = %d, NameCacheHashBuckets = %d"),
                                   cMaxNameCacheEntries, cNameCacheHashBuckets));

#if DBG
    m_cNameCacheRequests = m_cNameCacheHits = 0;
#endif
}

#pragma warning (default : 4355)

//
// CscObjTree dtor.
//
CscObjTree::~CscObjTree(
    void
    )
{
    DBGTRACE((DM_OBJTREE, DL_HIGH, TEXT("CscObjTree::~CscObjTree")));
    DBGPRINT((TEXT("\tName cache requests = %d, hits = %d, hit rate = %d pct."),
              m_cNameCacheRequests, 
              m_cNameCacheHits, 
              0 != m_cNameCacheRequests ? (m_cNameCacheHits * 100)/ m_cNameCacheRequests : 0));

    Clear();  // Destroy tree contents.
}



//
// Initialize the object tree by loading only share objects from the 
// CSC database.
//
void
CscObjTree::LoadCscShares(
    void
    )
{
    DBGTRACE((DM_OBJTREE, DL_HIGH, TEXT("CscObjTree::LoadCscShares")));

    CArray<CscShareInformation> rgsi;
    CscGetShareInformation(&rgsi);
    int cShares = rgsi.Count();
    for (int i = 0; i < cShares; i++)
    {
        CscShareInformation *psi = &(rgsi[i]);
        if (0 < psi->TotalCount())
        {
            CscObject *pShare = new CscShare(this,
                                             psi->ServerName(),
                                             psi->ShareName(),
                                             GetShareDriveLetter(psi->Name()),
                                             LOWORD(psi->Status()),
                                             psi->TotalCount(),
                                             psi->PinnedCount(),
                                             psi->StaleCount());
            try
            {
                AddChild(pShare);
                pShare->AddRef();
            }
            catch(...)
            {
                DBGERROR((TEXT("C++ exception caught in CscObjTree::LoadCscShares")));
                delete pShare;
                throw;
            }
        }
    }
}


//
// Count all the objects in the tree.
//
int 
CscObjTree::Count(
    void
    ) const
{
    int cObjects = 0;
    m_LoadMgr.PauseLoad();

    try
    {
        CscObjTreeIterator iter(*this);
        CscObject *pObject = NULL;

        cObjects = CountChildren(); // The root's children (shares).
        while(iter.Next(&pObject))
        {
            if (NULL != pObject)
                cObjects += pObject->CountChildren();
        }
    }
    catch(...)
    {
        DBGERROR((TEXT("C++ exception caught in CscObjTree::Count")));
        m_LoadMgr.ResumeLoad();
        throw;
    }
    m_LoadMgr.ResumeLoad();
    return cObjects;
}


//
// See if a share exists in the tree.
//
bool
CscObjTree::ShareExists(
    const UNCPath& path
    ) const
{
    return NULL != FindShare(path); 
}


//
// Locate a share in the tree and return it's object pointer.
//
CscShare *
CscObjTree::FindShare(
    const UNCPath& path
    ) const
{
    //
    // Create a minimal Share object to act as a comparison key.
    //
    CscShare key(NULL, path.m_strServer, path.m_strShare, TEXT('\0'), false, 0, 0, 0);

    CscShare *pShare = NULL;

    m_LoadMgr.PauseLoad();
    try
    {
        CscObjIterator iter = CreateChildIterator(EXCLUDE_NONEXISTING);
        CscObject *pChild = NULL;

        while(iter.Next(&pChild))
        {
            if (NULL != pChild)
            {
                DBGASSERT((pChild->IsShare()));  // All children of the "root" must be shares.

                pShare = reinterpret_cast<CscShare *>(pChild);
                if (*pShare == key)
                    break;

                pShare = NULL;
            }
        }
    }
    catch(...)
    {
        DBGERROR((TEXT("C++ exception caught in CscObjTree::FindShare")));
        m_LoadMgr.ResumeLoad();
        throw;
    }
    m_LoadMgr.ResumeLoad();

    return pShare;
}

//
// Retrieve a list of share names in the CSC database.
// This is a static function that may be called at any time.
//
void 
CscObjTree::GetShareList(
    CArray<CscShare *> *prgpShares,
    bool bSort
    ) const
{
    DBGASSERT((NULL != prgpShares));

    //
    // Using the flag EXCLUDE_NONEXISTING might seem strange since
    // we want a list of ALL share objects, not just the ones "currently
    // created".  However, since the share objects are created in the
    // tree's ctor, they are guaranteed to be there.  Also, there
    // is no guarantee that the tree has been fully populated.  If we 
    // don't specify this flag, the iterator's Next() function  will keep 
    // returning TRUE until the entire tree is populated and we'll 
    // just sit spinning in this loop until population is complete.
    //
    CscObjIterator iter = CreateChildIterator(EXCLUDE_NONEXISTING);
    CscObject *pObject = NULL;
    while(iter.Next(&pObject))
    {
        if (NULL != pObject)
        {
            DBGASSERT((pObject->IsShare()));
            prgpShares->Append(reinterpret_cast<CscShare *>(pObject));
        }
    }

    if (bSort)
    {
        //
        // Do a simple bubble-sort of the items based on display name.
        // The list should be short so perf isn't a real concern here.
        // I chose to take the most simple and direct approach.
        // Also, since the name strings are reference-counted,  
        // GetLongDisplayName isn't really copying strings; just
        // a pointer and bumping a reference count.  The Swap is
        // just swapping two pointers in the list.  The most expensive
        // thing is the comparison.
        //
        CString s1;
        CString s2;
        int n = prgpShares->Count();
        int s;
        do
        {
            s = 0;
            for (int i = 0; i < (n - 1); i++)
            {
                (*prgpShares)[i]->GetLongDisplayName(&s1);
                (*prgpShares)[i+1]->GetLongDisplayName(&s2);
                if (s2 < s1)
                {
                    SWAP((*prgpShares)[i], (*prgpShares)[i+1]);
                    s++;
                }
            }
        }
        while(0 < s);
    }
}


//
// Calculate the sum of total files in the CSC cache.
// Note that this value is valid only if the share objects have been
// created.  Since share objects are created in the tree ctor,
// this isn't a problem.
//
int CscObjTree::GetObjectCount(
    void
    ) const
{
    CArray<CscShare *> rgpShares;
    GetShareList(&rgpShares, false);

    int cShares  = rgpShares.Count();
    int cObjects = cShares;

    for (int i = 0; i < cShares; i++)
    {
        cObjects += rgpShares[i]->GetFileCount();
    }
    return cObjects;
}



//
// Retrieve display information for an object in the tree.
//
void
CscObjTree::GetObjDispInfo(
    const CscObject *pObject,
    CscObjDispInfo *pdi,
    DWORD fMask
    ) const
{
    DBGASSERT((NULL != pObject));
    DBGASSERT((NULL != pdi));

    //
    // First see if the information is cached.
    // If so, we're done.
    //
#if DBG
    m_cNameCacheRequests++;
#endif
    if (!m_NameCache.Retrieve(pObject, pdi) || (fMask != (pdi->m_fMask & fMask)))
    {
        //
        // Not cached.  Build name info from the tree entries
        // and cache it.  GetDispInfo() recurses up through the tree until
        // it hits the share object.  On exit from each function, each 
        // name component is added to the "diOut" structure.  
        // When this call to GetDispInfo() returns, "diOut" contains
        // all of the name information for the object.  Post-order
        // traversal is required so that the path string is properly
        // formatted.
        //
        m_LoadMgr.PauseLoad();
        try
        {
            pObject->GetDispInfo(pdi, fMask);
            m_NameCache.Add(pObject, *pdi);
        }
        catch(...)
        {
            DBGERROR((TEXT("C++ exception caught in CscObjTree::GetObjDispInfo")));
            m_LoadMgr.ResumeLoad();
            throw;
        }
        m_LoadMgr.ResumeLoad();
    }
#if DBG
    else
        m_cNameCacheHits++;
#endif
        
}


//
// Helper function for appending stuff onto path strings.
//
void
CscObjTree::LoadMgr::Worker::AppendFileToPath(
    LPCTSTR pszPath,
    LPCTSTR pszFile,
    CString *pstrFullPath
    )
{
    DBGASSERT((NULL != pstrFullPath));

    *pstrFullPath = pszPath;
    if (TEXT('\0') != (*pstrFullPath)[pstrFullPath->Length() - 1]) // BUGBUG:  DBCS compatible?
    {
        *pstrFullPath += CString(TEXT("\\"));
    }
    *pstrFullPath += CString(pszFile);
}



#if DBG
//
// For development.
//
void
CscObjTree::Dump(
    void
    )
{
    CscObjTreeIterator iter(*this);
    CscObject *pObject = NULL;
    int iObject = 0;
    while(iter.Next(&pObject))
    {
        if (NULL != pObject)
        {
            DBGPRINT((TEXT("Dumping object [%d]"), iObject++));
            pObject->Dump();
        }
    }

    m_NameCache.Dump();
}
#endif // DBG


void
CscObjTree::Refresh(
    void
    )
{
    DBGTRACE((DM_OBJTREE, DL_HIGH, TEXT("CscObjTree::Refresh")));
    Clear();
    LoadCscShares();
}


void 
CscObjTree::Clear(
    void
    )
{ 
    DBGTRACE((DM_OBJTREE, DL_HIGH, TEXT("CscObjTree::Clear")));
    CancelLoad(); 
    DestroyChildren(); 
    m_NameCache.Clear();
    m_cObjectsLoaded = 0;
}


void 
CscObjTree::CancelLoad(
    void
    )
{ 
    DBGTRACE((DM_OBJTREE, DL_HIGH, TEXT("CscObjTree::CancelLoad")));

    m_LoadMgr.ResumeLoad(); // Resume in case the client has paused the loading.
    m_LoadMgr.CancelLoad();
}


//
// Load one or all shares from the CSC database.
// If strShare is empty (""), all shares are loaded.
//
HRESULT 
CscObjTree::LoadShare(
    const CString& strShare
    )
{ 
    DBGTRACE((DM_OBJTREE, DL_HIGH, TEXT("CscObjTree::LoadShare")));
    DBGPRINT((DM_OBJTREE, DL_HIGH, TEXT("\tloading share \"%s\""), (LPCTSTR)strShare));

    HRESULT hr = E_FAIL;
    //
    // If strShare is empty, we load all shares in which case the
    // load manager organizes the workers as a team.
    // Remember:  We have to set the "teamwork" flag here since
    // LoadAllShares() calls LoadShare() for each share.
    // 
    if (strShare.IsEmpty())
    {
        m_LoadMgr.UseTeamWork(true);
        hr = m_LoadMgr.LoadAllShares();
    }
    else
    {
        m_LoadMgr.UseTeamWork(false);
        hr = m_LoadMgr.LoadShare(strShare); 
    }
    return hr;
}


//
// Load a list of shares.
//
HRESULT
CscObjTree::LoadShares(
    const CArray<CString>& rgstrShares
    )
{
    DBGTRACE((DM_OBJTREE, DL_HIGH, TEXT("CscObjTree::LoadShares")));
    HRESULT hr = NOERROR;
    m_LoadMgr.UseTeamWork(true);
    for (int i = 0; i < rgstrShares.Count(); i++)
    {
        if (FAILED(hr = m_LoadMgr.LoadShare(rgstrShares[i])))
            break;
    }
    return hr;
}



CscObjTree::LoadMgr::~LoadMgr(
    void
    )
{
    DBGTRACE((DM_LOADMGR, DL_HIGH, TEXT("CscObjTree::LoadMgr::~LoadMgr")));
    CancelLoad();
}


void
CscObjTree::LoadMgr::CancelLoad(
    void
    )
{
    DBGTRACE((DM_LOADMGR, DL_HIGH, TEXT("CscObjTree::LoadMgr::CancelLoad")));

    //
    // Cancel each worker then destroy the workers.
    //
    int cWorkers = m_rgpWorkers.Count();
    for (int i = 0; i < cWorkers; i++)
    {
        CancelWorker(*(m_rgpWorkers[i]));
    }

    DestroyWorkers();
}


void
CscObjTree::LoadMgr::CancelWorker(
    Worker& worker
    )
{
    DBGTRACE((DM_LOADMGR, DL_MID, TEXT("CscObjTree::LoadMgr::CancelWorker")));
    DBGPRINT((DM_LOADMGR, DL_MID, TEXT("\tworker 0x%08X"), (DWORD)&worker));
    //
    // Set the worker's "cancelled" flag then let the worker go
    // to completion.  The worker will notice the "cancelled" flag
    // and terminate it's thread normally.
    //
    worker.CancelLoad();
    ActivateWorker(worker);
    //
    // Wait for the worker's thread to terminate.
    //
    WaitForWorkerToFinish(worker);
}


void
CscObjTree::LoadMgr::DestroyWorkers(
    void
    )
{
    DBGTRACE((DM_LOADMGR, DL_MID, TEXT("CscObjTree::LoadMgr::DestroyWorkers")));

    while(0 < m_rgpWorkers.Count())
    {
        delete m_rgpWorkers[0];
        m_rgpWorkers[0] = NULL;
        m_rgpWorkers.Delete(0);
    }
    m_pCurrentWorker = NULL;
}


void 
CscObjTree::LoadMgr::UseTeamWork(
    bool bTeamWork
    )
{
    DBGTRACE((DM_LOADMGR, DL_LOW, TEXT("CscObjTree::LoadMgr::UseTeamWork")));

    m_bUseTeamWork = bTeamWork;
    int cWorkers = m_rgpWorkers.Count();
    for (int i = 0; i < cWorkers; i++)
    {
        m_rgpWorkers[i]->JoinTheTeam(bTeamWork);
    }
}


//
// Returns:  true  = Next team member found and activated.
//           false = All team members have completed their work.
//
bool
CscObjTree::LoadMgr::ActivateNextTeamMember(
    void
    )
{
    DBGTRACE((DM_LOADMGR, DL_MID, TEXT("CscObjTree::LoadMgr::ActivateNextTeamMember")));

    bool bActivated = false;
    int cWorkers = m_rgpWorkers.Count();
    //
    // Search the worker pool for the current worker.
    //
    for (int i = 0; i < cWorkers; i++)
    {
        if (m_pCurrentWorker == m_rgpWorkers[i])
        {
            //
            // Now look for the next worker in line that is not 
            // finished with its work.
            //
            for (int j = (i + 1) % cWorkers; j != i; j = (j + 1) % cWorkers)
            {
                if (Worker::eDone != m_rgpWorkers[j]->GetState())
                {
                    //
                    // Assign the job to the next non-finished worker.
                    //
                    DBGPRINT((DM_LOADMGR, DL_MID, TEXT("\tWorker 0x%08X selected for activation"), (DWORD)m_rgpWorkers[j]));
                    ActivateWorker(*m_rgpWorkers[j]);
                    bActivated = true;
                    break;
                }
            }
            break;
        }
    }
    return bActivated;
}



HRESULT
CscObjTree::LoadMgr::LoadAllShares(
    void
    )
{
    DBGTRACE((DM_LOADMGR, DL_HIGH, TEXT("CscObjTree::LoadMgr::LoadAllShares")));

    HRESULT hr = NOERROR;
    CArray<CscShare *> rgpShares;
    m_tree.GetShareList(&rgpShares, false);
    //
    // Load shares in reverse order.
    // 
    for (int i = rgpShares.Count() - 1; i >= 0; i--)
    {
        CString strShare;
        rgpShares[i]->GetName(&strShare);
        if (FAILED(hr = LoadShare(strShare)))
            break;
    }
    m_tree.AllChildrenCreated();
    return hr;
}


//
// First looks for an existing worker assigned to this share.  If found
// and the worker isn't currently working, activate the share's worker.
// If an existing worker for this share is not found, create a new
// worker, assign it this share and put it to work.
//
HRESULT
CscObjTree::LoadMgr::LoadShare(
    const CString& strShare
    )
{
    DBGTRACE((DM_LOADMGR, DL_HIGH, TEXT("CscObjTree::LoadMgr::LoadShare")));
    DBGPRINT((DM_LOADMGR, DL_HIGH, TEXT("\tloading share \"%s\""), (LPCTSTR)strShare));

    HRESULT hr = E_FAIL;

    Worker *pWorker = FindWorker(strShare);
    if (NULL != pWorker)
    {
        //
        // A worker has already been created to handle this share.
        // We need to tell it to "get to work".
        //
        if (pWorker == m_pCurrentWorker)
        {
            //
            // Share is already being loaded by the "current"
            // worker.  No need to continue.
            //
            DBGPRINT((DM_LOADMGR, DL_MID, TEXT("Current worker 0x%08X is loading share"), (DWORD)pWorker));
            return S_OK;
        }

        switch(pWorker->GetState())
        {
            case Worker::eRunning:
            case Worker::eStopped:
            case Worker::eSuspended:
                hr = S_OK;
                break;

            case Worker::eDone:
                DBGPRINT((DM_LOADMGR, DL_MID, TEXT("Worker 0x%08X is done with share")));
                if (m_bUseTeamWork)
                {
                    //
                    // The worker for this share is already done loading.
                    // If it is part of a team, activate the next team
                    // member to continue loading "all shares".
                    //
                    DBGPRINT((DM_LOADMGR, DL_MID, TEXT("Activating next team member")));
                    pWorker = NULL;
                    ActivateNextTeamMember();
                }
                hr = S_FALSE;
                break;

            default:
                DBGERROR((TEXT("Invalid worker state (%d)"), pWorker->GetState()));
                hr = E_FAIL;
                break;
        }
    }
    else
    {
        //
        // No current worker created to load this share.
        // Create one, start it and add it to the worker list.
        // This is the ONLY place workers are created.
        //
        DBGPRINT((DM_LOADMGR, DL_MID, TEXT("No worker for this share.  Creating new worker")));
        pWorker = new Worker(*this, m_tree, strShare, m_bUseTeamWork);
        m_rgpWorkers.Append(pWorker);

        hr = S_OK;
    }

    if (NULL != pWorker)
    {
        ActivateWorker(*pWorker);
    }

    return hr;
}


//
// Locate an existing worker in the manager's list of workers.
// Workers are identified by the share they're assigned to load.
// Returns NULL if no match is found.
//
CscObjTree::LoadMgr::Worker *
CscObjTree::LoadMgr::FindWorker(
    const CString& strShareKey
    )
{
    DBGTRACE((DM_LOADMGR, DL_LOW, TEXT("CscObjTree::LoadMgr::FindWorker")));
    DBGPRINT((DM_LOADMGR, DL_LOW, TEXT("\tworker for share \"%s\""), (LPCTSTR)strShareKey));

    CString strShareWorker;
    int cWorkers = m_rgpWorkers.Count();

    for (int i = 0; i < cWorkers; i++)
    {
        m_rgpWorkers[i]->GetShare(&strShareWorker);
        if (strShareWorker == strShareKey)
        {
            DBGPRINT((DM_LOADMGR, DL_LOW, TEXT("\tfound worker 0x%08X"), (DWORD)m_rgpWorkers[i]));
            return m_rgpWorkers[i];
        }
    }
    DBGPRINT((DM_LOADMGR, DL_LOW, TEXT("\tno worker found for share.")));
    return NULL;
}



CscObjTree::LoadMgr::Worker::Worker(
    LoadMgr& manager,
    CscObjTree& tree, 
    const CString& strShare,
    bool bTeam
    ) : m_manager(manager),
        m_tree(tree),
        m_hThread(NULL),
        m_idThread(0),
        m_WorkAuth(true, false),
        m_strShare(strShare),
        m_eState(eStopped),
        m_bLoadCancelled(false),
        m_bIsTeamMember(bTeam)
{ 
    DBGTRACE((DM_LOADWORKER, DL_HIGH, TEXT("CscObjTree::LoadMgr::Worker::Worker")));
    m_hThread = (HANDLE)_beginthreadex(NULL,
                               0,          // Default stack size
                               ThreadProc,
                               (LPVOID)this,
                               CREATE_SUSPENDED,
                               (UINT *)(&m_idThread));

}


CscObjTree::LoadMgr::Worker::~Worker(
    void
    )
{
    DBGTRACE((DM_LOADWORKER, DL_HIGH, TEXT("CscObjTree::LoadMgr::Worker::~Worker")));
    if (NULL != m_hThread)
    {
        //
        // This code counts on the m_bCancel member of CscObjTree
        // being set to true before this destructor is invoked.  
        // Otherwise, the worker thread will run (and we'll block) until 
        // it's share is completely loaded.
        //
        DBGPRINT((DM_LOADWORKER, DL_HIGH, TEXT("CscObjTree::LoadMgr::Worker::~Worker. Waiting for thread %d exit..."), m_idThread));
        WaitForSingleObject(m_hThread, INFINITE);
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }
}


//
// We place the loading operation for each worker object on it's own thread.
// This decouples the loading from the client UI.  The UI independently employs
// a tree iterator for each share so that the UI is in control of when visual
// information is updated (pull model).
//
// If pThis->m_strShare is empty (""), the worker will load all shares.
//
UINT
CscObjTree::LoadMgr::Worker::ThreadProc(
    LPVOID pvParam
    )
{
    HRESULT hr = NOERROR;

    DBGTRACE((DM_LOADWORKER, DL_HIGH, TEXT("CscObjTree::LoadMgr::Worker::ThreadProc")));
    Worker *pThis = reinterpret_cast<Worker *>(pvParam);
    if (NULL != pThis)
    {
        //
        // This worker loads its share.
        //
        pThis->m_eState = eRunning;
        hr = pThis->LoadShare(pThis->m_strShare);
        DBGPRINT((DM_LOADWORKER, DL_MID, TEXT("Worker 0x%08X done with result 0x%08X.  %s team member"), 
                 (DWORD)pThis, hr, pThis->m_bIsTeamMember ? TEXT("Is") : TEXT("Is not")));
        pThis->m_eState = eDone;
        if (pThis->m_bIsTeamMember)
        {
            //
            // The worker is part of a team loading all shares.
            // Tell the manager to activate the next worker on the team.
            //
            DBGPRINT((DM_LOADWORKER, DL_MID, TEXT("Activating next team member")));
            pThis->m_manager.ActivateNextTeamMember();
        }
    }
    return 0;
}



//
// Load a single share from the CSC database.
// The share is identified by a UNC path string ("\\server\share").
//
HRESULT
CscObjTree::LoadMgr::Worker::LoadShare(
    const CString& strShare
    )
{
    DBGTRACE((DM_LOADWORKER, DL_HIGH, TEXT("CscObjTree::LoadMgr::Worker::LoadShare")));
    DBGPRINT((DM_LOADWORKER, DL_HIGH, TEXT("\tloading share \"%s\""), (LPCTSTR)strShare));
    HRESULT hr = E_FAIL;

    //
    // Wait for approval from the load manager to start the job.
    //
    WaitForJobApproval();
    if (m_bLoadCancelled)
    {
        DBGPRINT((DM_LOADWORKER, DL_MID, TEXT("Job cancelled")));
        JobComplete();
        return S_FALSE;  // Load cancelled while waiting for approval.
    }

    //
    // Find the share object.  The tree is populated with it's 
    // share objects upon CscObjTree creation.
    //
    CscShare *pShare = m_tree.FindShare(UNCPath((LPCTSTR)strShare));
    if (NULL != pShare)
    {
        //
        // Now add the share's contents to the tree.
        // This is a recursive operation that will result in all children
        // of the share being added.
        //
        hr = LoadSubTree(pShare, strShare);
    }

    return hr;
}

//
// Load a subtree from the CSC database rooted at a given tree object.
// This function is called recursively to load an entire directory
// tree from the CSC database.  All LoadXXXXX() functions eventually get here.
//
HRESULT
CscObjTree::LoadMgr::Worker::LoadSubTree(
    CscObjParent *pParent,
    const CString& strPath
    )
{
    DBGTRACE((DM_LOADWORKER, DL_LOW, TEXT("CscObjTree::LoadMgr::Worker::LoadSubTree")));
    DBGPRINT((DM_LOADWORKER, DL_LOW, TEXT("\tpath = \"%s\""), (LPCTSTR)strPath));

    DBGASSERT((NULL != pParent));
    DBGASSERT(!strPath.IsEmpty());

    HRESULT hr        = S_FALSE;         // Assume no files in subtree.
    DWORD dwLastError = ERROR_SUCCESS;
    WIN32_FIND_DATA fd;
    DWORD dwStatus = 0;
    DWORD dwHintFlags = 0;
    DWORD dwPinCount = 0;
    FILETIME ft;

    //
    // Find first file in folder.
    //
    CscFindHandle hFind(CSCFindFirstFile((LPCTSTR)strPath, 
                                         &fd,
                                         &dwStatus,
                                         &dwPinCount,
                                         &dwHintFlags,
                                         &ft));
    if (!hFind.IsValidHandle())
    {
        //
        // Find first failed.  Either found no files or an error
        // occured.  If no files found, we just return S_FALSE.
        //
        DBGASSERT((S_FALSE == hr));
        dwLastError = GetLastError();
        if (ERROR_FILE_NOT_FOUND != dwLastError &&
            ERROR_SUCCESS != dwLastError)
        {
            DBGPRINT((DM_LOADWORKER, DL_LOW, TEXT("CSCFindFirstFile for \"%s\" failed with error 0x%08X"), (LPCTSTR)strPath, dwLastError));
            hr = HRESULT_FROM_WIN32(dwLastError);
        }
    }
    else
    {
        //
        // Load all files, folders and contents of folders.
        //
        do
        {
            //
            // Wait for approval from the load manager to start the job.
            // This ensures that only one entity (manager or one worker) are
            // operating on the tree at any one time.  It also ensures that a 
            // worker can't be suspended within a half-completed job.
            // Workers always wait here for the next approval.
            //
            JobComplete();
            WaitForJobApproval();
            if (m_bLoadCancelled)
            {
                DBGPRINT((DM_LOADWORKER, DL_MID, TEXT("Job cancelled")));
                dwLastError = ERROR_NO_MORE_FILES;
                break;
            }

            //
            // Don't include "system pinned" files or folders.
            //
            if (0 == (FLAG_CSC_HINT_PIN_SYSTEM & dwHintFlags))
            {
                if (IsDirectory(fd))
                {
                    if (!IsDotOrDotDot(fd))
                    {
                        //
                        // It's a directory and not the '.' or '..' aliases.
                        // Add a new folder to this subtree.
                        //
                        DBGPRINT((DM_OBJTREE, DL_LOW, TEXT("Adding folder \"%s\""), fd.cFileName));
                        CscObject *pFolder = new CscFolder(pParent,
                                                           CString(fd.cFileName),
                                                           LOWORD(dwStatus),
                                                           LOWORD(dwHintFlags),
                                                           FileTime(fd.ftLastWriteTime),
                                                           FileSize(MAKEULONGLONG(fd.nFileSizeHigh, fd.nFileSizeLow)));

                        try
                        {
                            pParent->AddChild(pFolder);
                            pFolder->AddRef();
                        }
                        catch(...)
                        {
                            DBGERROR((TEXT("C++ exception caught in Worker::LoadSubTree")));
                            //
                            // Folder wasn't added to tree.  Delete it.
                            // 
                            delete pFolder;
                            JobComplete();
                            throw;
                        }

                        //
                        // Recursively add subtrees.
                        //
                        CString strFolder;
                        AppendFileToPath(strPath, fd.cFileName, &strFolder);

                        hr = LoadSubTree(static_cast<CscObjParent *>(pFolder), strFolder);
                    }
                }
                else
                {
                    DBGPRINT((DM_OBJTREE, DL_LOW, TEXT("Adding file \"%s\""), fd.cFileName));

                    CscObject *pFile = new CscFile(pParent, 
                                                   CString(fd.cFileName), 
                                                   LOWORD(dwStatus),
                                                   LOWORD(dwHintFlags),
                                                   FileTime(fd.ftLastWriteTime),
                                                   FileSize(MAKEULONGLONG(fd.nFileSizeHigh, fd.nFileSizeLow)));
                    try
                    {
                        //
                        // Add a new file to this subtree.
                        //
                        pParent->AddChild(pFile);
                        pFile->AddRef();
                    }
                    catch(...)
                    {
                        DBGERROR((TEXT("C++ exception caught in Worker::LoadSubTree")));
                        delete pFile;
                        throw;
                    }
                }
            }

            dwStatus    = 0;
            dwHintFlags = 0;
            dwPinCount  = 0;
            //
            // Get the next file in this folder.
            //
            if (!CSCFindNextFile(hFind, 
                                 &fd,
                                 &dwStatus,
                                 &dwPinCount,
                                 &dwHintFlags,
                                 &ft))
            {
                dwLastError = GetLastError();
                DBGPRINT((DM_LOADWORKER, DL_LOW, TEXT("CSCFindNextFile failed for \"%s\" with last error = %d"), strPath.Cstr(), dwLastError));
            }

        } while(!m_bLoadCancelled && ERROR_SUCCESS == dwLastError);

#if DBG
        if (ERROR_NO_MORE_FILES != dwLastError && ERROR_SUCCESS != dwLastError)
            DBGERROR((TEXT("CSCFindNextFile for \"%s\" failed with error 0x%08X"), (LPCTSTR)strPath, dwLastError));
#endif

        hr = (ERROR_NO_MORE_FILES == dwLastError ? S_OK : HRESULT_FROM_WIN32(dwLastError));
    }

    //
    // Mark this parent object as having all of it's children 
    // created.  This will cause the object to return true 
    // in response to IsComplete().
    //
    DBGPRINT((DM_LOADWORKER, DL_LOW, TEXT("All children created for parent \"%s\", @0x%08X,  hr = 0x%08X"), strPath.Cstr(), (DWORD)pParent, hr));
    pParent->AllChildrenCreated();
    JobComplete();
    return hr;
}



//
// Workers call this when they are ready to begin loading a CSC file.
// This causes all workers to wait in a known loading state before the manager
// grants load approval to a worker.  If the worker already owns the manager's
// mutex and the worker's event is signaled, approval is granted immediately.
// Otherwise, the worker must wait until the mutex is available and it's 
// event is signaled.
//
void
CscObjTree::LoadMgr::WaitForJobApproval(
    Worker& worker
    )
{
    DBGPRINT((DM_LOADMGR, DL_LOW, TEXT("CscObjTree::LoadMgr::WaitForJobApproval.")));
    HANDLE rgHandles[2];
    rgHandles[0] = m_mutex.Handle();
    rgHandles[1] = worker.GetWorkAuthorizationHandle();

    //
    // Wait for both the manager's mutex and the worker's work authorization 
    // event.
    //
    WaitForMultipleObjects(ARRAYSIZE(rgHandles), 
                           rgHandles, 
                           TRUE,                
                           INFINITE);

};


//
// Workers call this when they are complete with their current "job".
//
void
CscObjTree::LoadMgr::JobComplete(
    Worker& worker
    )
{
    DBGPRINT((DM_LOADMGR, DL_LOW, TEXT("CscObjTree::LoadMgr::JobComplete.")));

    //
    // Release the manager's mutex.
    //
    m_mutex.Release();
}


//
// The load manager calls this to wait until all workers are paused 
// waiting for job approval.  This ensures that all workers are in 
// a known state and that no worker is currently loading a CSC file.
//
void 
CscObjTree::LoadMgr::WaitForWorkerAttention(
    void
    )
{
    DBGPRINT((DM_LOADMGR, DL_LOW, TEXT("CscObjTree::LoadMgr::WaitForWorkerAttention.")));
    //
    // When the load manager's mutex is available, no worker is
    // currently loading a file into the tree.  The manager is now
    // free to assign a job to another worker or cancel the loading
    // process.
    //
    m_mutex.Wait();
}


//
// Called by the load manager when a CSC share is to be loaded into the
// tree.  The function ensures all workers are stopped in a known state
// then activates the specified worker to continue or begin loading
// it's assigned CSC share.
//
void
CscObjTree::LoadMgr::ActivateWorker(
    Worker& worker
    )
{
    DBGTRACE((DM_LOADMGR, DL_MID, TEXT("CscObjTree::LoadMgr::ActivateWorker.")));
    DBGPRINT((DM_LOADMGR, DL_MID, TEXT("\tworker 0x%08X"), (DWORD)&worker));

    //
    // Wait until all workers are in a known state.  This is the
    // state where no worker owns the load manager's mutex object.
    //
    WaitForWorkerAttention();
    //
    // Reset the current worker's work authorization event so that
    // the worker will finish it's current job and then wait for
    // future approval.
    //
    if (NULL != m_pCurrentWorker)
    {
        DBGPRINT((DM_LOADMGR, DL_MID, TEXT("\tworker 0x%08X was current"), (DWORD)m_pCurrentWorker));
        m_pCurrentWorker->RevokeWorkAuthorization();
    }

    m_pCurrentWorker = &worker;
    //  
    // Set this worker's work authorization event.  This sets the specific 
    // worker's event so it will begin working once the load manager's
    // mutex is available (when we release it below).
    //
    m_pCurrentWorker->GrantWorkAuthorization();
    //
    // If this the first time the worker is receiving a job, the worker's
    // thread is suspended (created suspended).  Need to resume the thread.
    //
    m_pCurrentWorker->WakeUp();
    //
    // Release the load manager's mutex.  This causes the worker to begin
    // loading since both it's event and the manager's mutex are 
    // signaled.  
    //
    m_mutex.Release();
}


//-----------------------------------------------------------------------------
// class CscObjTreeIterator
//
// An iterator for enumerating all objects in a CscObjTree tree.
// This may be one subtree or the entire subtree.
//
//-----------------------------------------------------------------------------
CscObjTreeIterator::CscObjTreeIterator(
    const CscObject& tree,
    DWORD fExclude
    ) : m_tree(tree),
        m_fExclude(fExclude)
{
    DBGTRACE((DM_OBJTREE, DL_HIGH, TEXT("CscObjTreeIterator::CscObjTreeIterator")));
    //
    // Initialize the iterator by pushing the object's child iterator
    // onto the iterator stack.
    //
    Reset();
}


CscObjTreeIterator::~CscObjTreeIterator(
    void
    )
{
    DBGTRACE((DM_OBJTREE, DL_HIGH, TEXT("CscObjTreeIterator::CscObjTreeIterator")));
}


void
CscObjTreeIterator::Reset(
    void
    )
{
    m_rgChildIterators.Clear();
    m_rgChildIterators.Insert(m_tree.CreateChildIterator(m_fExclude));
}


//
// Retrieve the address of the next CscObject in the enumeration.
// Returns false when the enumeration is complete.
// Returns true if there are more (or may be more) objects to enumerate.
// This implementation maintains a stack of CscObjIterator objects - 
// one for each folder visited.  This stack holds the search context for
// each folder between successive calls to Next().  Note that these
// child iterators we're pushing and popping are very small (one
// pointer and one integer).  Copying the objects is very cheap.
//
bool
CscObjTreeIterator::Next(
    CscObject **ppObject
    )
{
    DBGASSERT((NULL != ppObject));

    bool bResult = false;
    *ppObject = NULL;

    while(!bResult && 0 < m_rgChildIterators.Count())
    {
        bResult = m_rgChildIterators[0].Next(ppObject);
        if (!bResult)
        {
            m_rgChildIterators.Delete(0);
        }
    }

    if (bResult && NULL != *ppObject && (*ppObject)->CanBeParent())
    {
        //
        // Found an object that may be a parent.  Create a child iterator for
        // the folder and push it onto the iterator stack.  It will be used
        // the next time Next() is called causing us to drop down one level
        // in the tree.
        //
        m_rgChildIterators.Insert((*ppObject)->CreateChildIterator(m_fExclude));
    }
    return bResult;
}


