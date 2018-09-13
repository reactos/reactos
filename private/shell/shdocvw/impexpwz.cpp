/*
 * Author: t-franks
 *
 * Last Modified On: Oct 16, 1998
 * Last Modified By: t-joshp
 *
 */

#include "priv.h"
#include "resource.h"
#include "impexp.h"
#include "mluisupp.h"  // for MLLoadString

//
// Indices into our imagelist
// (used for the open and closed folder icons on the tree view)
//
#define FOLDER_CLOSED 0
#define FOLDER_OPEN   1

#define ImportCookieFile ImportCookieFileW
#define ExportCookieFile ExportCookieFileW

BOOL ImportCookieFileW(IN LPCWSTR szFilename);
BOOL ExportCookieFileW(IN LPCWSTR szFilename, BOOL fAppend);

extern void SetListViewToString (HWND hLV, LPCTSTR pszString);

//
// used to display "file already exists" and "file not found" messages
//
int WarningMessageBox(HWND hwnd, UINT idTitle, UINT idMessage, LPCTSTR szFile, DWORD dwFlags);

//
// Strings that don't need localizing
//

#define NS3_COOKIE_REG_PATH         TEXT("Software\\Netscape\\Netscape Navigator\\Cookies")
#define NS3_COOKIE_REG_KEY          TEXT("Cookie File")

#ifndef UNIX
#define NS3_BOOKMARK_REG_PATH       TEXT("Software\\Netscape\\Netscape Navigator\\Bookmark List")
#else
#define NS3_BOOKMARK_REG_PATH       TEXT("SOFTWARE\\Microsoft\\Internet Explorer\\unix\\nsbookmarks")
#endif

#define NS3_BOOKMARK_REG_KEY        TEXT("File Location")

#define NS4_USERS_REG_PATH          TEXT("Software\\Netscape\\Netscape Navigator\\Users")
#define NS4_USERPATH_REG_KEY        TEXT("DirRoot")

#define NS_FALLBACK_ROOT_REG_KEY    TEXT("Software\\Netscape\\Netscape Navigator")
#define NS_FALLBACK_VERSION_REG_VAL TEXT("CurrentVersion")
#define NS_FALLBACK_MAIN_REG_VAL    TEXT("Main")
#define NS_FALLBACK_INST_REG_VAL    TEXT("Install Directory")

#ifndef UNIX
#define ALL_FILES_WILDCARD          TEXT("\\*.*")
#else
#define ALL_FILES_WILDCARD          TEXT("/*")
#endif

#define DOT_DIR                     TEXT(".")
#define DOT_DOT_DIR                 TEXT("..")

#ifdef UNIX
#define DIR_SEPARATOR_CHAR  TEXT('/')
#else
#define DIR_SEPARATOR_CHAR  TEXT('\\')
#endif

//*************************************************************
//
//      class ListIterator
//
//  Keeps hold on a position in a list.  Allows basic access
//to a list.  The list is set up to map a name to a value.

class NestedList;

class ListIterator
{
    friend NestedList;
    
    struct node
    {
        LPTSTR _sName;
        LPTSTR _sValue;
        DWORD _cNameSize, _cValueSize;
        node* _pnNext;
        node* _pnSublist;
    };

    //  A position is held by pointing to the
    //current node and the pointer that is directed
    //to that node.  The back pointer is kept so the 
    //list can be manipulated at the current element.
    //  when m_pnCurrent == NULL, the iterator is
    //at the end of the list.
    node** m_ppnPrev;
    node* m_pnCurrent;

    //  The invariant could be broken if two iterators
    //point to the same node, and one inserts or deletes
    //an element.  So only one iterator should exist in 
    //a branch of the list at a time.
    BOOL invariant()
    {
        return *m_ppnPrev == m_pnCurrent;
    }

public:
    ListIterator( node** ppnPrev)
    {
        m_ppnPrev = ppnPrev;
        m_pnCurrent = *m_ppnPrev;
    }

    BOOL Insert( LPCTSTR sName, DWORD cNameSize, LPCTSTR sValue, DWORD cValueSize);
    BOOL Remove();

    ListIterator GetSublist();
    void DeleteSublist();

    BOOL Next();
    BOOL AtEndOfList();

    LPCTSTR GetName();
    DWORD GetNameSize();
    LPCTSTR GetValue();
    DWORD GetValueSize();
};


//*************************************************************
//
//  class NestedList
//      Keeps a pointer to a node which heads a list,
//  and deletes that list on destruction.


class NestedList
{
    ListIterator::node* m_pnRoot;
    
public:
    NestedList();
    ~NestedList();

    operator ListIterator();
};


NestedList::NestedList()
: m_pnRoot(NULL)
{
}


NestedList::~NestedList()
{
    while( ((ListIterator)*this).Remove())
    {
    }
}


NestedList::operator ListIterator()
{
    return ListIterator( &m_pnRoot);
}

//*************************************************************
//*************************************************************
//
//  ListIterator functions
//


//  Inserts an element before the current one,
//leaves iterator pointing at new node.
BOOL ListIterator::Insert( 
    LPCTSTR sName, 
    DWORD cNameSize, 
    LPCTSTR sValue, 
    DWORD cValueSize)
{
    ASSERT( invariant());

    node* pNewNode = (node*)(new BYTE[ sizeof(node) 
                                       + (( cNameSize + cValueSize)
                                          * sizeof(TCHAR))]);

    if( pNewNode == NULL)
        return FALSE;

    //  the name and value will be appended to the node.
    pNewNode->_sName = (LPTSTR)((BYTE*)pNewNode + sizeof(node));
    pNewNode->_sValue = pNewNode->_sName + cNameSize;

    pNewNode->_cNameSize = cNameSize;
    pNewNode->_cValueSize = cValueSize;

    memcpy( pNewNode->_sName, sName, pNewNode->_cNameSize * sizeof(TCHAR));
    memcpy( pNewNode->_sValue, sValue, pNewNode->_cValueSize * sizeof(TCHAR));

    // insert new node in list
    pNewNode->_pnNext = m_pnCurrent;
    *m_ppnPrev = pNewNode;

    //  The iterator now points to the new element.
    m_pnCurrent = *m_ppnPrev;
    
    ASSERT( invariant());

    return TRUE;
}


//  Deletes the current node.
//  Returns FALSE if at end of list.
BOOL ListIterator::Remove()
{
    ASSERT( invariant());
    
    //  If this list is empty, or if the iterator 
    //points at the end of the list, there is nothing to
    //delete.
    if( m_pnCurrent == NULL)
        return FALSE;

    // remove sublist
    DeleteSublist();
    
    //  Remember where target node is
    //so it can be deleted once out of
    //the list.
    node* pOldNode = m_pnCurrent;

    // take the target node out of the list.
    //(iterator points to next node or end of list)
    *m_ppnPrev = m_pnCurrent->_pnNext;
    m_pnCurrent = *m_ppnPrev;

    //  Get rid of target node.
    delete [] (BYTE*)pOldNode;

    ASSERT( invariant());

    return TRUE;    
}


//  Returns the sublist of the current node.
ListIterator ListIterator::GetSublist()
{
    ASSERT( invariant());
    
    return ListIterator( &(m_pnCurrent->_pnSublist));
}


//  deletes the children of the current node.
void ListIterator::DeleteSublist()
{
    ASSERT( invariant());
    
    ListIterator sublist( &(m_pnCurrent->_pnSublist));
    
    while( sublist.Remove())
    {
    }

    ASSERT( invariant());
}


//  Advances to the next node.
//  Returns FALSE if already at end of list.
BOOL ListIterator::Next()
{
    ASSERT( invariant());

    if( m_pnCurrent == NULL)
        return FALSE;

    m_ppnPrev = &(m_pnCurrent->_pnNext);
    m_pnCurrent = *m_ppnPrev;

    ASSERT( invariant());

    return m_pnCurrent != NULL;
}


//  
BOOL ListIterator::AtEndOfList()
{
    return ( m_pnCurrent == NULL) ? TRUE : FALSE;
};


//
LPCTSTR ListIterator::GetName()
{
    ASSERT( invariant() && m_pnCurrent != NULL);

    return m_pnCurrent->_sName;
}


//
DWORD ListIterator::GetNameSize()
{
    ASSERT( invariant() && m_pnCurrent != NULL);

    return m_pnCurrent->_cNameSize;
}


//
LPCTSTR ListIterator::GetValue()
{
    ASSERT( invariant() && m_pnCurrent != NULL);

    return m_pnCurrent->_sValue;
}


//
DWORD ListIterator::GetValueSize()
{
    ASSERT( invariant() && m_pnCurrent != NULL);

    return m_pnCurrent->_cValueSize;
}


//*************************************************************
//*************************************************************
//
//  class ImpExpUserProcess
//
//      maintains the description of an import/export process
//  for an import/export wizard, and finally executes the
//  the import/export.

enum ExternalType { INVALID_EXTERNAL = 0, COOKIES, BOOKMARKS};
enum TransferType { INVALID_TRANSFER = 0, IMPORT, EXPORT};

class ImpExpUserProcess
{
public:
    ImpExpUserProcess();
    ~ImpExpUserProcess();
    
    //  the first step the wizard should do is identify the type of
    //import/export process to be done.
    void SelectExternalType( ExternalType selection)    { m_ExternalType = selection; }
    void SelectTransferType( TransferType selection)    { m_TransferType = selection; }
    ExternalType GetExternalType()                      { return m_ExternalType; }
    TransferType GetTransferType()                      { return m_TransferType; }

    BOOL PopulateComboBoxForExternalSelection( HWND hComboBox);
    BOOL GetExternalManualDefault( LPTSTR sExternal, DWORD* pcSize);

    //
    // used to fill the listbox with names of netscape profiles
    //
    void purgeExternalList();
    BOOL populateExternalList();
    BOOL populateExternalListForCookiesOrBookmarks();

    //
    // for netscape 3.x
    //
    BOOL populateExternalListForCookiesOrBookmarksWithNS3Entry();

    //
    // for netscape 4.x
    //
    BOOL populateExternalListForCookiesOrBookmarksWithNS4Entries();

    //
    // fallback case for "funny" versions of netscape
    //
    BOOL populateExternalListFromFolders(LPTSTR pszPath);
    BOOL populateExternalListWithNSEntriesFallBack();

    //  If the transfer is for favorites, the wizard needs to specify
    //an internal folder to import to or export from.
    LPCTSTR GetInternalSelection()       { return m_pSelectedInternal; }

    BOOL PopulateTreeViewForInternalSelection( HWND TreeView);
    BOOL populateTreeViewWithInternalList( HWND hTreeView, ListIterator iterator, HTREEITEM hParent);
    BOOL ExpandTreeViewRoot ( HWND hTreeView ) ;

    BOOL SelectInternalSelection( HWND TreeView);

    void purgeInternalList();
    BOOL populateInternalList();
    BOOL populateInternalListForBookmarks();
    BOOL appendSubdirsToInternalList( LPTSTR sPath, ListIterator iterator);
    
    //  And then, the import/export can be completed.
    void PerformImpExpProcess(HWND hwnd);

    //
    // The filename that we're exporting to or 
    // importing from.
    //
    TCHAR m_szFileName[MAX_PATH];

private:
    ExternalType m_ExternalType;
    TransferType m_TransferType;

    //  m_ExternalList is a flat list of names associated with files
    //example: name =  "Netscape 4.0 profile - Dr. Falken"
    //         value =  "c:\netscapeprofiledir\DrFalken.chs"
    NestedList m_ExternalList;

    //  m_InternalList is a nested list favorites' pathnames,
    //associated with the complete path.
    NestedList m_InternalList;

    //  Maintain synchronization between m_ExternalType/m_TransferType 
    //and m_InternalList
    ExternalType m_InternalListExternalType;
    TransferType m_InternalListTransferType;

    // if ExternalType == BOOKMARKS,
    //m_pSelectedInternal is the path of a Favorites folder,
    //residing in m_InternalList somewhere, or NULL if a folder
    //hasn't been selected yet.
    LPTSTR m_pSelectedInternal;

};


ImpExpUserProcess::ImpExpUserProcess()
:   m_ExternalType(INVALID_EXTERNAL), m_TransferType(INVALID_TRANSFER),
    m_InternalListExternalType(INVALID_EXTERNAL), m_InternalListTransferType(INVALID_TRANSFER),
    m_pSelectedInternal(0)
{
}


ImpExpUserProcess::~ImpExpUserProcess()
{
}


//*************************************************************
//   PopulateComboBoxForExternal
//
//  Loads content for list box into memory and into List Box,
//associating value of each element with the list element.

//  returns FALSE if the list box is left empty
BOOL ImpExpUserProcess::PopulateComboBoxForExternalSelection( HWND hComboBox)
{
    ASSERT ( m_ExternalType != INVALID_EXTERNAL ) ;

    ComboBox_ResetContent(hComboBox);
   
    //  If ExternalList is invalid, the list box will be left empty.
    if( !populateExternalList() )
        return FALSE;

    ListIterator iterator = m_ExternalList;

    //  Detect and notify if the list is empty.
    if( iterator.AtEndOfList() )
        return FALSE;

    //  add entries from the new ExternalList to the ComboBox.
    do
    {
        int index = ComboBox_AddString( hComboBox, const_cast<LPTSTR>(iterator.GetName() ) );
        ComboBox_SetItemData( hComboBox, index, const_cast<LPTSTR>(iterator.GetValue() ) );
    } while( iterator.Next());

    // set the first one as selected
    ComboBox_SetCurSel( hComboBox, 0 );

    return TRUE;
}


//*************************************************************
//
//  GetExternalManualDefault
//
//  Allows user interface to offer some sort of default
//  filename/location.
//
BOOL ImpExpUserProcess::GetExternalManualDefault(LPTSTR sExternal, DWORD* pcSize)
{
    ASSERT(NULL != pcSize);

    //
    // We only fill it in if it's blank
    //
    if (m_szFileName[0])
    {
        return FALSE;
    }

    ListIterator iterator = m_ExternalList;

    TCHAR szFileName[MAX_PATH];
    INT cchFileName;
    if(m_ExternalType == BOOKMARKS)
        MLLoadString(IDS_NETSCAPE_BOOKMARK_FILE,szFileName,ARRAYSIZE(szFileName));
    else
        MLLoadString(IDS_NETSCAPE_COOKIE_FILE,szFileName,ARRAYSIZE(szFileName));
    cchFileName = lstrlen(szFileName) + 1;

    //  Grab the first item in the External List and use its value.
    if( ((ListIterator)m_ExternalList).AtEndOfList() == FALSE
        && ((ListIterator)m_ExternalList).GetValue() != NULL
        && *pcSize >= ((ListIterator)m_ExternalList).GetValueSize())
    {
        StrCpyN( sExternal,
                 ((ListIterator)m_ExternalList).GetValue(),
                 ((ListIterator)m_ExternalList).GetValueSize());
        *pcSize = ((ListIterator)m_ExternalList).GetValueSize();

        return TRUE;
    }
    //  If there is enough room, specify some file with the correct name
    //  in the "my documents" directory.
    else 
    {
        ASSERT(m_ExternalType == BOOKMARKS || m_ExternalType == COOKIES);
        
        TCHAR szMyDocsPath[MAX_PATH];

        SHGetSpecialFolderPath(NULL,szMyDocsPath,CSIDL_PERSONAL,TRUE);

        *pcSize = wnsprintf(sExternal,MAX_PATH,TEXT("%s%c%s"),szMyDocsPath,DIR_SEPARATOR_CHAR,szFileName);

        return *pcSize > 0;
    }
}


//*************************************************************
//
//
//  purgeExternalList
//
//  Used to clear external target/source list loaded into memory

void ImpExpUserProcess::purgeExternalList()
{
    // delete elements until they're all gone.
    ListIterator iterator = m_ExternalList;

    while( iterator.Remove())
    {
    }

}


//*************************************************************
//
//  populeExternalList
//
//  Used to load external target/source list into memory

BOOL ImpExpUserProcess::populateExternalList()
{
    ASSERT(m_ExternalType != INVALID_EXTERNAL)

    purgeExternalList();

    if(!populateExternalListForCookiesOrBookmarks())
    {
        //
        // If we didn't get any entries using the "standard"
        // techniques, then (and only then) we try the "fallback"
        //
        if (!populateExternalListWithNSEntriesFallBack())
        {
            purgeExternalList();
            return FALSE;
        }

    }

    return TRUE;
}


//*************************************************************
//
//  populateExternalListforCookiesOrBookmarks
//
//  Used to lod external target/source list into memory
//in the case that the content to be transfered is cookies
//or bookmarks.

//  returns TRUE if any elements have been added to the external list
BOOL ImpExpUserProcess::populateExternalListForCookiesOrBookmarks()
{
    ASSERT( m_ExternalType == COOKIES || m_ExternalType == BOOKMARKS);

    BOOL fHasAddedElements = FALSE;

    if( populateExternalListForCookiesOrBookmarksWithNS3Entry())
        fHasAddedElements = TRUE;

    if( populateExternalListForCookiesOrBookmarksWithNS4Entries())
        fHasAddedElements = TRUE;
 
    return fHasAddedElements;
}


//*************************************************************
//
//  populateExternalList..WithNS3Entry
//
//  subfunc of populateExternalListForCookiesOrBookmarks.

//  returns TRUE if any elements have been added to the external list
BOOL ImpExpUserProcess::populateExternalListForCookiesOrBookmarksWithNS3Entry()
{
    BOOL retVal = FALSE;

    //  Determine where to look for reg key
    LPTSTR sNS3RegPath;
    LPTSTR sNS3RegKey;

    if( m_ExternalType == BOOKMARKS)
    {
        sNS3RegPath = NS3_BOOKMARK_REG_PATH;
        sNS3RegKey = NS3_BOOKMARK_REG_KEY;
    }
    else
    {
        sNS3RegPath = NS3_COOKIE_REG_PATH;
        sNS3RegKey = NS3_COOKIE_REG_KEY;
    }

    //  Get the file location and add it to the list
    //  The registry location has the complete path + filename.
    TCHAR sFilePath[MAX_PATH];
    DWORD cbFilePathSize = sizeof(sFilePath);
    DWORD dwType;
    if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, sNS3RegPath, sNS3RegKey,
                                    &dwType, (BYTE*)sFilePath, &cbFilePathSize)
        && (dwType == REG_SZ || dwType == REG_EXPAND_SZ))
    {
        TCHAR szBuffer[MAX_PATH];

        MLLoadString(IDS_NS3_VERSION_CAPTION, szBuffer, MAX_PATH);
        
        retVal = ((ListIterator)m_ExternalList).Insert( 
                   szBuffer, lstrlen(szBuffer)+1,
                   sFilePath, cbFilePathSize / sizeof(TCHAR));
    }

    return retVal;
}


//*************************************************************
//
//  populateExternalList..WithNS4Entries
//
//  subfunc of populateExternalListForCookiesOrBookmarks.

//  returns TRUE if any elements have been added to the external list
BOOL ImpExpUserProcess::populateExternalListForCookiesOrBookmarksWithNS4Entries()
{
    BOOL retVal = FALSE;

    //  Get an iterator to advance position as items are inserted.
    ListIterator iterator = (ListIterator)m_ExternalList;

    //  Get the filename to be attached and the associated string size.
    TCHAR sFilename[MAX_PATH];
    DWORD cFilenameLength;
    if(m_ExternalType == BOOKMARKS)
        MLLoadString(IDS_NETSCAPE_BOOKMARK_FILE,sFilename,ARRAYSIZE(sFilename));
    else
        MLLoadString(IDS_NETSCAPE_COOKIE_FILE,sFilename,ARRAYSIZE(sFilename));
    cFilenameLength = lstrlen(sFilename);

    //  Get the reg key of the root of the NS profiles for enumeration.
    HKEY hUserRootKey = NULL;

    if( RegOpenKeyEx( HKEY_LOCAL_MACHINE, NS4_USERS_REG_PATH, 
                      0, KEY_READ, &hUserRootKey) 
        != ERROR_SUCCESS)
    {
        hUserRootKey = NULL;
        goto donePopulateExternalListForCookiesOrBookmarksWithNS4Entries;
    }

    DWORD dwNumberOfProfiles;
    if( RegQueryInfoKey( hUserRootKey, NULL, NULL, NULL, &dwNumberOfProfiles,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL) != ERROR_SUCCESS
        || dwNumberOfProfiles == 0)
    {
        goto donePopulateExternalListForCookiesOrBookmarksWithNS4Entries;
    }

    //  Enumerate over the NS profiles, getting their names and
    //directory paths.  Associated the profile name with the path
    //of the desired files by appending the filename to the
    //user's root.
    TCHAR sProfileName[MAX_PATH];
    DWORD cProfileNameSize;  
    cProfileNameSize = MAX_PATH;
    DWORD iEnumIndex;  iEnumIndex = 0;
    while( RegEnumKeyEx( hUserRootKey, (iEnumIndex++), sProfileName, 
                         &cProfileNameSize, NULL, NULL, NULL, NULL) 
           == ERROR_SUCCESS)
    {
        //RegEnumKeyEx gives us the ProfileNameSize w/out the '\0'.
        cProfileNameSize = MAX_PATH;

        HKEY hProfileKey = NULL;

        if( RegOpenKeyEx( hUserRootKey, sProfileName, 0, KEY_READ, &hProfileKey) 
            != ERROR_SUCCESS)
        {
            hProfileKey = NULL;
            goto doneWithEntryInPopulateExternalListForCookiesOrBookmarksWithNS4Entries;
        }

        DWORD dwType;  //  should be REG_SZ when returned from QueryValue
        TCHAR sProfilePath[MAX_PATH];
        DWORD cProfilePathSize;  cProfilePathSize = sizeof(sProfilePath);
        if( (RegQueryValueEx( hProfileKey, NS4_USERPATH_REG_KEY, NULL, &dwType, 
                             (LPBYTE)sProfilePath, &cProfilePathSize) 
                != ERROR_SUCCESS)
            || dwType != REG_SZ)
        {
            goto doneWithEntryInPopulateExternalListForCookiesOrBookmarksWithNS4Entries;
        }
        cProfilePathSize /= sizeof(TCHAR);
        
        if( (MAX_PATH - cProfilePathSize) < cFilenameLength)
        {
            goto doneWithEntryInPopulateExternalListForCookiesOrBookmarksWithNS4Entries;
        }

        //  append "\\sFilename\0" to the path.
        sProfilePath[ cProfilePathSize - 1] = TCHAR(FILENAME_SEPARATOR);
        memcpy( &sProfilePath[cProfilePathSize], 
                sFilename, cFilenameLength * sizeof(TCHAR));
        cProfilePathSize += cFilenameLength;
        sProfilePath[cProfilePathSize++] = TCHAR('\0');

        // we can only import files if they exist!
        if( m_TransferType == IMPORT
            && GetFileAttributes(sProfilePath) == 0xFFFFFFFF)
                goto doneWithEntryInPopulateExternalListForCookiesOrBookmarksWithNS4Entries;

        //
        // construct the string for the combo box
        //
        TCHAR sRawProfileName[MAX_PATH];
        TCHAR sRealProfileName[MAX_PATH];
        UINT cRealProfileName;

        MLLoadString(IDS_NS4_FRIENDLY_PROFILE_NAME, sRawProfileName, MAX_PATH);

        cRealProfileName = 
            wnsprintf(sRealProfileName, MAX_PATH, 
                      sRawProfileName, sProfileName);

        //  Insert the profile into the list.  If it inserts, thats
        //enough to consider the whole functions call a success.
        if( iterator.Insert(sRealProfileName, cRealProfileName + 1,
                            sProfilePath, cProfilePathSize))
            retVal = TRUE;

    doneWithEntryInPopulateExternalListForCookiesOrBookmarksWithNS4Entries:
        if( hProfileKey != NULL)
            RegCloseKey(hProfileKey);
    }

donePopulateExternalListForCookiesOrBookmarksWithNS4Entries:
    if( hUserRootKey != NULL)
        RegCloseKey( hUserRootKey);

    return retVal;
}

BOOL ImpExpUserProcess::populateExternalListFromFolders(LPTSTR pszPath)
{

    BOOL retval = FALSE;
    TCHAR szFileName[MAX_PATH];
    TCHAR szPathWithWildcards[MAX_PATH];

    ListIterator iterator = (ListIterator)m_ExternalList;

    HANDLE hFind = NULL;
    WIN32_FIND_DATA wfd;

    //
    // what are we looking for?
    //
    if(m_ExternalType == BOOKMARKS)
        MLLoadString(IDS_NETSCAPE_BOOKMARK_FILE,szFileName,ARRAYSIZE(szFileName));
    else
        MLLoadString(IDS_NETSCAPE_COOKIE_FILE,szFileName,ARRAYSIZE(szFileName));

    //
    // prepare the path variable
    //
    StrCpyN(szPathWithWildcards,pszPath,MAX_PATH);
    StrCatBuff(szPathWithWildcards,ALL_FILES_WILDCARD,MAX_PATH);

    //
    // start the find file thing
    //
    hFind = FindFirstFile(szPathWithWildcards,&wfd);

    if (hFind == INVALID_HANDLE_VALUE)
        goto Cleanup;

    do
    {

        //
        // the actual bookmark or cookie file
        //
        TCHAR szFullPath[MAX_PATH];
        int cchFullPath;

        //
        // a "friendly" name for the corresponding profile
        //
        TCHAR szProfileFormat[MAX_PATH];
        TCHAR szProfileName[MAX_PATH];
        int cchProfileName;

        //
        // skip over "." and ".."
        //
        if(!StrCmp(wfd.cFileName, DOT_DIR) ||
           !StrCmp(wfd.cFileName, DOT_DOT_DIR))
            continue;

        //
        // skip over any non-directories
        //
        if (!(wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
            continue;

        //
        // generate the path
        //
#ifndef UNIX
        cchFullPath = wnsprintf(szFullPath,MAX_PATH,TEXT("%s\\%s\\%s"),pszPath,wfd.cFileName,szFileName);
#else
        cchFullPath = wnsprintf(szFullPath,MAX_PATH,TEXT("%s/%s/%s"),pszPath,wfd.cFileName,szFileName);
#endif

        //
        // see if the file actually exists
        //
        if (GetFileAttributes(szFullPath) == 0xFFFFFFFF)
            continue;

        //
        // generate the profile name
        //
        MLLoadString(IDS_FB_FRIENDLY_PROFILE_NAME, szProfileFormat, MAX_PATH);
        cchProfileName = wnsprintf(szProfileName, MAX_PATH, szProfileFormat, wfd.cFileName);

        //
        // add the entry to the list
        //
        iterator.Insert(
            szProfileName,cchProfileName+1,
            szFullPath,cchFullPath+1);

        retval = TRUE;

    } while(FindNextFile(hFind,&wfd));

Cleanup:

    if (hFind)
        FindClose(hFind);

    return retval;

}

BOOL ImpExpUserProcess::populateExternalListWithNSEntriesFallBack()
{

    BOOL retVal = FALSE;

    HKEY hRoot = NULL;
    HKEY hCurrentVersion = NULL;
    HKEY hCurrentVersionMain = NULL;

    TCHAR szUsersDir[64]; // will contain "..\\Users"

    DWORD dwType;
    TCHAR szVersion[64];
    TCHAR szPath[MAX_PATH];
    DWORD cbSize;

    LONG result;

    //
    // Open the root of netscape's HKLM registry hierarchy
    //
    result = RegOpenKeyEx(
         HKEY_LOCAL_MACHINE, 
         NS_FALLBACK_ROOT_REG_KEY,
         0, 
         KEY_READ, 
         &hRoot);
    
    if (result != ERROR_SUCCESS)
        goto Cleanup;

    //
    // Retrieve the "CurrentVersion" value
    //
    cbSize = sizeof(szVersion);
    result = RegQueryValueEx(
        hRoot, 
        NS_FALLBACK_VERSION_REG_VAL, 
        NULL, 
        &dwType, 
        (LPBYTE)szVersion, 
        &cbSize);

    if (result != ERROR_SUCCESS || dwType != REG_SZ)
        goto Cleanup;

    //
    // Open the sub-hierarchy corresponding to the current version
    //
    result = RegOpenKeyEx(
         hRoot, 
         szVersion, 
         0, 
         KEY_READ, 
         &hCurrentVersion);

    if (result != ERROR_SUCCESS)
        goto Cleanup;

    //
    // Open the "main" sub-hierarchy
    //
    result = RegOpenKeyEx(
         hCurrentVersion, 
         NS_FALLBACK_MAIN_REG_VAL, 
         0, 
         KEY_READ, 
         &hCurrentVersionMain);

    if (result != ERROR_SUCCESS)
        goto Cleanup;

    //
    // Retrieve the "Install Directory" value
    //
    cbSize = sizeof(szPath);
    result = RegQueryValueEx(
        hCurrentVersionMain, 
        NS_FALLBACK_INST_REG_VAL, 
        NULL, 
        &dwType, 
        (LPBYTE)szPath, 
        &cbSize);

    if (result != ERROR_SUCCESS || dwType != REG_SZ)
        goto Cleanup;

    //
    // Take a wild guess at where the "Users" dir might be
    //
    MLLoadString(IDS_NETSCAPE_USERS_DIR,szUsersDir,ARRAYSIZE(szUsersDir));
    StrCatBuff(szPath,szUsersDir,ARRAYSIZE(szPath));

    //
    // Fill in the list
    //
    if (populateExternalListFromFolders(szPath))
        retVal = TRUE;

Cleanup:

    if (hRoot)
        RegCloseKey(hRoot);

    if (hCurrentVersion)
        RegCloseKey(hCurrentVersion);

    if (hCurrentVersionMain)
        RegCloseKey(hCurrentVersionMain);

    return retVal;

}


//*************************************************************
//
//  PopulateTreeViewForInternalSelection
//
//  Load a nested list of the favorites folders into memory
//and then into a Tree View.

//  returns FALSE if TreeView is left empty.
BOOL ImpExpUserProcess::PopulateTreeViewForInternalSelection( HWND hTreeView)
{
    ASSERT( m_TransferType != INVALID_TRANSFER);

    TreeView_DeleteAllItems( hTreeView);

    if( !populateInternalList())
        return FALSE;

    return populateTreeViewWithInternalList
            ( hTreeView, (ListIterator)m_InternalList, TVI_ROOT);
}


//*************************************************************
//
//  populateTreeViewWithInternalList
//
//  Loads list entries at 'iterator' below tree view item 'hParent'
//  into 'hTreeView'.  Associates value of each list entry with 
//  the Param of the Tree View node.
//
BOOL ImpExpUserProcess::populateTreeViewWithInternalList
(
    HWND hTreeView,
    ListIterator iterator,
    HTREEITEM hParent
)
{
    BOOL retVal = FALSE;
    
    if( iterator.AtEndOfList())
        goto donePopulateTreeViewWithInternalList;

    TVINSERTSTRUCT newTV;
    HTREEITEM hNew;
    
    //  declare parent and intent to put at end of list.
    newTV.hParent = hParent;
    newTV.hInsertAfter = TVI_LAST;

    // build info struct
    newTV.itemex.mask = TVIF_TEXT
                        | TVIF_PARAM
                        | TVIF_CHILDREN
                        | TVIF_IMAGE
                        | TVIF_SELECTEDIMAGE;

    // give name
    newTV.itemex.cchTextMax = lstrlen( iterator.GetName()) + 1;
    newTV.itemex.pszText = const_cast<LPTSTR>(iterator.GetName());
    
    // associate the necessary data
    newTV.itemex.lParam = (LPARAM)iterator.GetValue();

    // tell tree view if there are any children.
    newTV.itemex.cChildren = 
        iterator.GetSublist().AtEndOfList() == TRUE ? FALSE : TRUE;

    //  use correct icons
    newTV.itemex.iSelectedImage = FOLDER_OPEN ;
    newTV.itemex.iImage = FOLDER_CLOSED ;

    hNew = TreeView_InsertItem( hTreeView, &newTV );

    if( hNew == NULL)
        goto donePopulateTreeViewWithInternalList;

    //  an element has been added, so we should return TRUE.
    retVal = TRUE;

    //  add children
    populateTreeViewWithInternalList( hTreeView, iterator.GetSublist(), hNew );

    //  add siblings
    if( iterator.Next())
        populateTreeViewWithInternalList( hTreeView, iterator, hParent );

donePopulateTreeViewWithInternalList:
    return retVal;

}

BOOL ImpExpUserProcess::ExpandTreeViewRoot ( HWND hTreeView ) 
{

    HTREEITEM hRoot ;

    hRoot = TreeView_GetRoot ( hTreeView ) ;

    if ( hRoot != NULL )
        TreeView_Expand ( hTreeView, hRoot, TVE_EXPAND ) ;
    else
        return FALSE ;

    return TRUE ;

}

//*************************************************************
//
//  SelectInternalSelection
//
//  Gets the data associated with the current selection of
//'hTreeView'.

BOOL ImpExpUserProcess::SelectInternalSelection( HWND hTreeView)
{
    HTREEITEM hSelection = TreeView_GetSelection( hTreeView);
    
    if( hSelection == NULL)
        return FALSE;

    //TVITEM is built up to query the lParam
    //(the lParam has been associated with a pointer to the path value)
    TVITEM TV;
    TV.mask = TVIF_PARAM;
    TV.hItem = hSelection;
    
    if( !TreeView_GetItem( hTreeView, &TV))
        return FALSE;

    m_pSelectedInternal = (LPTSTR)TV.lParam;

    ASSERT( m_pSelectedInternal != NULL);
    
    return TRUE;
}


//*************************************************************
//
//  purgeInternalList
//
//  Wipes out whatever has been loaded in the internal
//target/source list.

void ImpExpUserProcess::purgeInternalList()
{
    // clear the list.
    ListIterator iterator = (ListIterator)m_InternalList;

    while( iterator.Remove())
    {
    }

    m_pSelectedInternal = NULL;
    m_InternalListExternalType = INVALID_EXTERNAL;
    m_InternalListTransferType = INVALID_TRANSFER;
}


//*************************************************************
//
//  populateInternalList
//
//  Builds the internal list for potential internal target/sources.
//  This currently only makes sense for bookmarks, where a favorites
//directory has to be picked.

//  returns TRUE if any elements have been added to the internal list
BOOL ImpExpUserProcess::populateInternalList()
{
    ASSERT( m_ExternalType != INVALID_EXTERNAL);

    if( m_InternalListExternalType == m_ExternalType
        && m_InternalListTransferType == m_TransferType)
        return TRUE;

    purgeInternalList();

    // (could switch on different m_ExternalTypes here)
    if( !populateInternalListForBookmarks())
    {
        purgeInternalList();
        return FALSE;
    }

    m_InternalListExternalType = m_ExternalType;
    m_InternalListTransferType = m_TransferType;
    return TRUE;
}


//*************************************************************
//
//  populateInternalListForBookmarks

//  returns TRUE if any elements have been added to the internal list
BOOL ImpExpUserProcess::populateInternalListForBookmarks()
{
    ASSERT( m_ExternalType == BOOKMARKS);

    TCHAR szFavoritesPath[MAX_PATH];

    if( SHGetSpecialFolderPath( NULL, szFavoritesPath, CSIDL_FAVORITES, FALSE)
        && appendSubdirsToInternalList( szFavoritesPath, m_InternalList))
    {
        return TRUE;
    }
    else return FALSE;
}


//*************************************************************
//
//  appendSubdirsToInternalList
//
//  Takes 'sPath' as a specification for a file search.  All
//directories that match that are added to the internal list 
//at 'iterator'.
//  Recursively adds subdirectories found.
//
//typical usage:
//     szPath is "c:\Root\Favorites",
//       finds "c:\Root\Favorites",
//   recursively calls itself with
//     szPath = "c:\Root\Favorites\*.*"
//       finding and recursing into all subdirs

//  returns TRUE if any directories have been added to the internal list
//  Edits the contents of the buffer past the last '\\'.
BOOL ImpExpUserProcess::appendSubdirsToInternalList
(
    LPTSTR sPath, 
    ListIterator iterator
)
{
    BOOL fHaveAddedDirectories = FALSE;

    DWORD cPathLength = lstrlen(sPath);

    HANDLE hEnum;
    WIN32_FIND_DATA currentFile;

    hEnum = FindFirstFile( sPath, &currentFile);

    //example:
    //given: "c:\root\*.*"  (will find all dirs in root)
    //want: "c:\root\"
    //given: "c:\favorites" (will find favorites in root)
    //want: "c:\"
    //  left search to '\\' to find the path of the files to be found.
    while( cPathLength > 0
           && sPath[ --cPathLength] != TCHAR(FILENAME_SEPARATOR))
    {
    }
    cPathLength++;

    if( hEnum == INVALID_HANDLE_VALUE)
        return FALSE;

    do
    {
        DWORD cFileNameLength;
        
        // we only handle directories
        if( !(currentFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            continue;

        // we don't want '.' and '..' to show up.
        if( !StrCmp( currentFile.cFileName, DOT_DIR)
            || !StrCmp( currentFile.cFileName, DOT_DOT_DIR))
            continue;

        cFileNameLength = lstrlen( currentFile.cFileName);
        memcpy( sPath + cPathLength, currentFile.cFileName, cFileNameLength * sizeof(TCHAR));
        sPath[ cPathLength + cFileNameLength] = TCHAR('\0');

        if( iterator.Insert( currentFile.cFileName, cFileNameLength + 1,
                             sPath, cPathLength + cFileNameLength + 1))
        {
            memcpy( sPath + cPathLength + cFileNameLength,
                    ALL_FILES_WILDCARD, sizeof(ALL_FILES_WILDCARD));
            appendSubdirsToInternalList( sPath, iterator.GetSublist());
            // we know now that a directory has been added
            fHaveAddedDirectories = TRUE;
        }
    } while( FindNextFile( hEnum, &currentFile));

    ASSERT( GetLastError() == ERROR_NO_MORE_FILES);
    FindClose( hEnum);

    return fHaveAddedDirectories;
}


//*************************************************************
//
//  PerformImpExpProcess
//
//  Once everything is set up right, this should do the trick.

void ImpExpUserProcess::PerformImpExpProcess(HWND hwnd)
{
    ASSERT( GetExternalType() != INVALID_EXTERNAL);
    ASSERT( GetTransferType() != INVALID_TRANSFER);
    ASSERT( (GetExternalType() == BOOKMARKS) ? (GetInternalSelection() != NULL) : TRUE);

    HCURSOR hOldCursor;

    //
    // This could take a while, so show an hourglass cursor
    //
    hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    switch( GetExternalType())
    {
    case COOKIES:

        switch( GetTransferType())
        {
        case IMPORT:
            if (ImportCookieFile(m_szFileName))
            {
                MLShellMessageBox(
                    hwnd, 
                    MAKEINTRESOURCE(IDS_IMPORTSUCCESS_COOK), 
                    MAKEINTRESOURCE(IDS_CONFIRM_IMPTTL_COOK),
                    MB_OK);
            }
            else
            {
                MLShellMessageBox(
                    hwnd, 
                    MAKEINTRESOURCE(IDS_IMPORTFAILURE_COOK), 
                    MAKEINTRESOURCE(IDS_CONFIRM_IMPTTL_COOK),
                    MB_OK);
            }
            break;

        case EXPORT:
            if (SUCCEEDED(SHPathPrepareForWriteWrap(hwnd, NULL, m_szFileName, FO_COPY, (SHPPFW_DEFAULT | SHPPFW_IGNOREFILENAME))))
            {
                //  FALSE specifies that we will overwrite cookies
                if (ExportCookieFile(m_szFileName, FALSE ))
                {
                    MLShellMessageBox(
                        hwnd, 
                        MAKEINTRESOURCE(IDS_EXPORTSUCCESS_COOK), 
                        MAKEINTRESOURCE(IDS_CONFIRM_EXPTTL_COOK),
                        MB_OK);
                }
                else
                {
                    MLShellMessageBox(
                        hwnd, 
                        MAKEINTRESOURCE(IDS_EXPORTFAILURE_COOK), 
                        MAKEINTRESOURCE(IDS_CONFIRM_EXPTTL_COOK),
                        MB_OK);
                }
            }
            break;

        default:
            ASSERT(0);
            
        }
        break;
        
    case BOOKMARKS:

        DoImportOrExport(
            GetTransferType()==IMPORT,
            m_pSelectedInternal,
            m_szFileName,
            FALSE);

        break;

    default:
        ASSERT(0);

    }

    //
    // Put the old cursor back when finished
    //
    SetCursor(hOldCursor);

}


//*************************************************************
//*************************************************************
//
//   ImpExpUserDlg
//
//  Handles the user interface side of things, building
//  up an ImpExpUserProcess then executing it.
//  The dialog procedures below will all have a return value
//  which can be set to something besides FALSE if used, or left
//  as FALSE if not used.  Since only one section of code should
//  attempt to give the return value a value before returning,
//  class RetVal is set up to throw an assertion if two pieces
//  of code intended to pass back a return value at the same
//  time.

class ReturnValue
{

private:
    BOOL_PTR m_value;

public:
    ReturnValue()
    { 
        m_value = FALSE;
    }
    
    BOOL_PTR operator =(BOOL_PTR newVal)
    {
        ASSERT( m_value == FALSE);
        m_value = newVal;
        return m_value;
    }
    
    operator BOOL_PTR ()
    {
        return m_value;
    }
};

class ImpExpUserDlg
{

private:

    static HIMAGELIST m_himl ;
    static BOOL InitImageList ( HWND hwndTree ) ;   
    static BOOL DestroyImageList ( HWND hwndTree ) ;    

    static HFONT m_hfont ;
    static BOOL InitFont ( HWND hwndStatic ) ;
    static BOOL DestroyFont ( HWND hwndStatic ) ;

    //  A sheet knows its resource ID and what process
    //it contributes to.
    struct SheetData
    {
        int _idPage;
        ImpExpUserProcess* _pImpExp;

        SheetData( int idPage, ImpExpUserProcess* pImpExp )
        : _idPage( idPage ), _pImpExp( pImpExp )
        {
        }
    };
    //
    //  InitializePropertySheetPage() will associate a dialog 
    //  with an allocated copy of SheetData, which will be
    //  found at PSN_SETACTIVE with and stored with SetWindowLong.
    //  The allocated SheetData will be cleaned up by callback
    //  procedure PropertySheetPageProc().
    //
    //  Callback functions sure are a drag for maintaining identity.
    //  GetWindowLong and SetWindowLong will be used to keep tabs
    //  on who is who, setting 'ghost' member variables.
    //
    // 'ghost' SheetData*         This;
    // 'ghost' ImpExpUserProcess* m_pImpExp;
    // 'ghost' DWORD              m_idPage;
    //
    //  CommonDialogProc retrieves the 'ghost' values and does other 
    //  shared behavior.
    //
    static DWORD CommonDialogProc
    ( 
        IN HWND hwndDlg, IN UINT msg, IN WPARAM wParam, IN LPARAM lParam,
        OUT ImpExpUserProcess** ppImpExp, OUT DWORD* pPageId,
        IN OUT ReturnValue& retVal
    );

    static void InitializePropertySheetPage( PROPSHEETPAGE* psp, DWORD idDialogTemplate, DWORD idTitle, DWORD idSubTitle,DLGPROC dlgProc, ImpExpUserProcess* lParam);
    static UINT CALLBACK PropertySheetPageProc( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

    //  some dialog procedures
    static BOOL_PTR CALLBACK Wizard97DlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    static BOOL_PTR CALLBACK TransferTypeDlg(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    static BOOL_PTR CALLBACK InternalDlg(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
    static BOOL_PTR CALLBACK ExternalDlg(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

    static void HandleTransferTypeChange ( HWND hwndDlg, ImpExpUserProcess* m_pImpExp, UINT iSelect ) ;

public:
    static BOOL RunNewDialogProcess( HWND hParent ) ;

};

HIMAGELIST ImpExpUserDlg::m_himl = NULL ;

BOOL ImpExpUserDlg::InitImageList ( HWND hwndTree )
{

    //
    // Code to retrieve icons for open and closed folders
    // was based on code in private/samples/sampview/utility.cpp.
    //

    TCHAR       szFolder[MAX_PATH];
    SHFILEINFO  sfi;
    HIMAGELIST  himlOld ;
    DWORD       dwRet ;

    // create the image list
    m_himl = ImageList_Create ( GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLORDDB, 2, 2 ) ;

    if ( m_himl == NULL )
        return FALSE ;

    ImageList_SetBkColor( m_himl, GetSysColor(COLOR_WINDOW) ) ;

    // add the closed folder icon
    GetWindowsDirectory(szFolder, MAX_PATH);
    SHGetFileInfo( szFolder,
                   0,
                   &sfi,
                   sizeof(sfi),
                   SHGFI_ICON | SHGFI_SMALLICON);
    dwRet = ImageList_AddIcon(m_himl, sfi.hIcon);
    ASSERT ( dwRet == FOLDER_CLOSED ) ;

    // add the open folder icon
    SHGetFileInfo( szFolder,
                   0,
                   &sfi,
                   sizeof(sfi),
                   SHGFI_ICON | SHGFI_SMALLICON | SHGFI_OPENICON);
    dwRet = ImageList_AddIcon(m_himl, sfi.hIcon);
    ASSERT ( dwRet == FOLDER_OPEN ) ;

    himlOld = TreeView_SetImageList( hwndTree, m_himl, TVSIL_NORMAL );

    if ( himlOld != NULL )
    {
        BOOL fOk ;
        fOk = ImageList_Destroy ( himlOld ) ;
        ASSERT ( fOk ) ;
    }

    return TRUE ;

}

BOOL ImpExpUserDlg::DestroyImageList ( HWND hwndTree ) 
{
    HIMAGELIST himlOld ;

    himlOld = TreeView_SetImageList( hwndTree, NULL, TVSIL_NORMAL );

    if ( himlOld != NULL )
    {
        BOOL fOk ;
        fOk = ImageList_Destroy ( himlOld ) ;
        ASSERT ( fOk ) ;
    }

    return TRUE ;
}


HFONT ImpExpUserDlg::m_hfont = NULL ;

BOOL ImpExpUserDlg::InitFont ( HWND hwndStatic ) 
{

    HDC hdc = GetDC ( hwndStatic ) ;

    if ( hdc == NULL )
        return FALSE ;

    LOGFONT lf;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH;
    lf.lfItalic = 0;
    lf.lfWeight = FW_BOLD;
    lf.lfStrikeOut = 0;
    lf.lfUnderline = 0;
    lf.lfWidth = 0;
    lf.lfHeight = -MulDiv(13, GetDeviceCaps(hdc, LOGPIXELSY), 72);

    LOGFONT lfTmp;
    HFONT   hFontOrig = (HFONT)SendMessage(hwndStatic, WM_GETFONT, (WPARAM)0, (LPARAM)0);
    if (hFontOrig && GetObject(hFontOrig, sizeof(lfTmp), &lfTmp))
    {
        lf.lfCharSet = lfTmp.lfCharSet;
        StrCpyN(lf.lfFaceName, lfTmp.lfFaceName, LF_FACESIZE);
    }
    else
    {
        lf.lfCharSet = GetTextCharset(hdc);
        StrCpyN(lf.lfFaceName, TEXT("MS Shell Dlg"), LF_FACESIZE);
    }

    m_hfont = CreateFontIndirect(&lf);

    if ( m_hfont == NULL )
        return FALSE ;

    SendMessage ( hwndStatic, WM_SETFONT, (WPARAM)m_hfont, MAKELPARAM(FALSE, 0) ) ;

    ReleaseDC ( hwndStatic,hdc ) ;

    return TRUE ;

}

BOOL ImpExpUserDlg::DestroyFont ( HWND hwndDlg )
{

    if ( m_hfont )
        DeleteObject ( m_hfont ) ;

    return TRUE ;
}

void ImpExpUserDlg::InitializePropertySheetPage
(
    PROPSHEETPAGE* psp, 
    DWORD idDialogTemplate,
    DWORD idTitle,
    DWORD idSubTitle,
    DLGPROC dlgProc,
    ImpExpUserProcess* lParam
)
{
    psp->dwSize = sizeof(PROPSHEETPAGE);
    psp->dwFlags = PSP_USECALLBACK | PSP_USETITLE;
    psp->hInstance = MLGetHinst();
    psp->pszTemplate = MAKEINTRESOURCE(idDialogTemplate);
    psp->pfnDlgProc = dlgProc;
    psp->lParam = (LPARAM)(new SheetData(idDialogTemplate,lParam));
    psp->pfnCallback = PropertySheetPageProc;
    psp->pszHeaderTitle = MAKEINTRESOURCE(idTitle);
    psp->pszHeaderSubTitle = MAKEINTRESOURCE(idSubTitle);
    psp->pszTitle = MAKEINTRESOURCE(IDS_IMPEXP_CAPTION);

    if ( idDialogTemplate == IDD_IMPEXPWELCOME ||
         idDialogTemplate == IDD_IMPEXPCOMPLETE )
    {
        psp->dwFlags |= PSP_HIDEHEADER; 
    }
    else
    {
        psp->dwFlags |= (PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE);
    }

}


UINT CALLBACK ImpExpUserDlg::PropertySheetPageProc
(
    HWND hwnd, 
    UINT uMsg, 
    LPPROPSHEETPAGE ppsp
)
{

    switch(uMsg)
    {

    case PSPCB_CREATE:
        break;

    case PSPCB_RELEASE:
        delete (SheetData*)ppsp->lParam;
        ppsp->lParam = NULL;
        break;

    default:
        break;

    }

    return TRUE ;
}


//*************************************************************
//
//  RunNewDialogProcess
//
//  Runs the Import Export Wizard.

BOOL_PTR CALLBACK TEMP(
  HWND hwndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
    return (uMsg==WM_INITDIALOG);
}

BOOL ImpExpUserDlg::RunNewDialogProcess(HWND hParent)
{


    const int numPages = 9;
    ImpExpUserProcess* pImpExp = new ImpExpUserProcess();

    if( pImpExp == NULL)
        return FALSE;
        
    PROPSHEETPAGE psp[numPages];
    PROPSHEETHEADER psh;

    InitializePropertySheetPage( &psp[0], IDD_IMPEXPWELCOME,        0,                              0,                                  Wizard97DlgProc, pImpExp );
    InitializePropertySheetPage( &psp[1], IDD_IMPEXPTRANSFERTYPE,   IDS_IMPEXPTRANSFERTYPE_TITLE,   IDS_IMPEXPTRANSFERTYPE_SUBTITLE,    TransferTypeDlg, pImpExp );
    InitializePropertySheetPage( &psp[2], IDD_IMPEXPIMPFAVSRC,      IDS_IMPEXPIMPFAVSRC_TITLE,      IDS_IMPEXPIMPFAVSRC_SUBTITLE,       ExternalDlg, pImpExp );
    InitializePropertySheetPage( &psp[3], IDD_IMPEXPIMPFAVDES,      IDS_IMPEXPIMPFAVDES_TITLE,      IDS_IMPEXPIMPFAVDES_SUBTITLE,       InternalDlg, pImpExp );
    InitializePropertySheetPage( &psp[4], IDD_IMPEXPEXPFAVSRC,      IDS_IMPEXPEXPFAVSRC_TITLE,      IDS_IMPEXPEXPFAVSRC_SUBTITLE,       InternalDlg, pImpExp );
    InitializePropertySheetPage( &psp[5], IDD_IMPEXPEXPFAVDES,      IDS_IMPEXPEXPFAVDES_TITLE,      IDS_IMPEXPEXPFAVDES_SUBTITLE,       ExternalDlg, pImpExp );
    InitializePropertySheetPage( &psp[6], IDD_IMPEXPIMPCKSRC,       IDS_IMPEXPIMPCKSRC_TITLE,       IDS_IMPEXPIMPCKSRC_SUBTITLE,        ExternalDlg, pImpExp );
    InitializePropertySheetPage( &psp[7], IDD_IMPEXPEXPCKDES,       IDS_IMPEXPEXPCKDES_TITLE,       IDS_IMPEXPEXPCKDES_SUBTITLE,        ExternalDlg, pImpExp );
    InitializePropertySheetPage( &psp[8], IDD_IMPEXPCOMPLETE,       0,                              0,                                  Wizard97DlgProc, pImpExp );

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_WIZARD97 | PSH_PROPSHEETPAGE | PSH_HEADER | PSH_WATERMARK ; 
    psh.hwndParent = hParent;
    psh.hInstance = MLGetHinst();
    psh.pszCaption = MAKEINTRESOURCE(IDS_IMPEXP_CAPTION);
    psh.nPages = numPages;
    psh.nStartPage = 0;
    psh.ppsp = psp;
    psh.pszbmWatermark = MAKEINTRESOURCE(IDB_IMPEXPWATERMARK);
    psh.pszbmHeader = MAKEINTRESOURCE(IDB_IMPEXPHEADER);

    int iResult = (int)PropertySheet(&psh) ;
    delete pImpExp;
    return iResult;

}


//*************************************************************
//
//  CommonDialogProc
//
//  Prepares 'ghost' member variables of the user dialog process,
//  handles ordering details of wizard pages and initializes common
//  dialog elements.
//
//  retVal passes through CommonDialogProc so that it can be set
//  if necessary.  Clients of CommonDialogProc should not need
//  to specify a new return value if CommonDialogProc has specified
//  a non-FALSE return value.
//
//  If CommonDialogProc returns FALSE dialog procedure should
//  considered 'msg' handled and return retVal immediately.
//
//  If this dialog has yet to receive WM_INITDIALOG, the 'ghost'
//  values will be zero (and invalid).
//

DWORD ImpExpUserDlg::CommonDialogProc
( 
    IN HWND hwndDlg, 
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam,
    OUT ImpExpUserProcess** ppImpExp,
    OUT DWORD* pPageId,
    ReturnValue& retVal
)
{

    SheetData* sheetData;
    ImpExpUserProcess* m_pImpExp = NULL;
    DWORD m_idPage = 0;

    //
    // Do init-dialog stuff
    //
    if ( WM_INITDIALOG == msg )
    {
        sheetData = (SheetData*)(((PROPSHEETPAGE*)lParam)->lParam);
        SetWindowLongPtr( hwndDlg, DWLP_USER, (LONG_PTR)sheetData);
    }

    //
    // Initialize the sheetData field
    //
    sheetData = (SheetData*)GetWindowLongPtr( hwndDlg, DWLP_USER ) ;
    if ( sheetData != NULL )
    {
        m_pImpExp = *ppImpExp = sheetData->_pImpExp;
        m_idPage = *pPageId = sheetData->_idPage;
    }

    //
    // Next, we check to make sure we're on the correct page.  If not, simply
    // return -1 and the wizard will automatically advance to the next page.
    //
    if( WM_NOTIFY == msg && PSN_SETACTIVE == ((LPNMHDR)lParam)->code )
    {

        BOOL fPageValidation = TRUE ;

        switch( m_idPage )
        {

        case IDD_IMPEXPWELCOME:
        case IDD_IMPEXPTRANSFERTYPE:
        case IDD_IMPEXPCOMPLETE:                    
            break;

        case IDD_IMPEXPIMPFAVSRC:
        case IDD_IMPEXPIMPFAVDES:
            if(m_pImpExp->GetTransferType() != IMPORT || m_pImpExp->GetExternalType() != BOOKMARKS)
                fPageValidation = FALSE;
            break;
    
        case IDD_IMPEXPEXPFAVSRC:
        case IDD_IMPEXPEXPFAVDES:
            if(m_pImpExp->GetTransferType() != EXPORT || m_pImpExp->GetExternalType() != BOOKMARKS)
                fPageValidation = FALSE;
            break;

        case IDD_IMPEXPIMPCKSRC:
            if(m_pImpExp->GetTransferType() != IMPORT || m_pImpExp->GetExternalType() != COOKIES)
                fPageValidation = FALSE;
            break;

        case IDD_IMPEXPEXPCKDES:
            if(m_pImpExp->GetTransferType() != EXPORT || m_pImpExp->GetExternalType() != COOKIES)
                fPageValidation = FALSE;
            break;
        }

        SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, fPageValidation ? 0 : -1 ) ;
        retVal = TRUE ;
        
        if ( ! fPageValidation )
            return FALSE ;

    }

    //
    // Initialize fonts and image lists (if needed)
    //
    if ( WM_NOTIFY == msg )
    {

        HWND hwndTitle = GetDlgItem ( hwndDlg, IDC_IMPEXPTITLETEXT )  ;
        HWND hwndTree = GetDlgItem ( hwndDlg, IDC_IMPEXPFAVTREE )  ;

        switch ( ((LPNMHDR)lParam)->code )
        {

        case PSN_SETACTIVE:
        
            if ( hwndTitle )
                InitFont ( hwndTitle ) ;

            if ( hwndTree )
                InitImageList( hwndTree ) ;
    
            break ;

        case PSN_KILLACTIVE:
        case PSN_QUERYCANCEL:

            if ( hwndTitle )
                DestroyFont ( hwndTitle ) ;

            if ( hwndTree )
                DestroyImageList( hwndTree ) ;

            break;

        }
    
    }

    if( WM_NOTIFY == msg && PSN_SETACTIVE == ((LPNMHDR)lParam)->code )
    {

        HWND hwndParent = GetParent( hwndDlg);

        switch( m_idPage )
        {
        case IDD_IMPEXPWELCOME:
            PropSheet_SetWizButtons( hwndParent, PSWIZB_NEXT );
            break;
            
        case IDD_IMPEXPCOMPLETE:
            {

                UINT idText ;
                const TCHAR *szInsert = m_pImpExp->m_szFileName ;
                TCHAR szRawString[1024] ;
                TCHAR szRealString[1024] ;

                //
                // First, we need to figure out which string should 
                // be used to describe what the wizard is going to 
                // do (for example "Import the cookies from...")
                //
                if ( m_pImpExp->GetTransferType() == IMPORT )
                {
                    if ( m_pImpExp->GetExternalType() == COOKIES )
                        idText = IDS_IMPEXP_COMPLETE_IMPCK ;
                    else
                        idText = IDS_IMPEXP_COMPLETE_IMPFV ;
                }
                else
                {
                    if ( m_pImpExp->GetExternalType() == COOKIES )
                        idText = IDS_IMPEXP_COMPLETE_EXPCK ;
                    else
                        idText = IDS_IMPEXP_COMPLETE_EXPFV ;
                }

                LoadString(MLGetHinst(), idText, szRawString, 1024);

                wnsprintf(szRealString, 1024, szRawString, szInsert);

                //
                // Set the text in the listview, and do all the other magic to make
                // the tooltips work, etc.
                //
                SetListViewToString(GetDlgItem(hwndDlg,IDC_IMPEXPCOMPLETECONFIRM), szRealString);

                //
                // The SetListViewToString function helpfully sets the background color to
                // gray instead of the default (white).  But we actually want it white, so 
                // let's reset it here.
                // 
                ListView_SetBkColor(GetDlgItem(hwndDlg,IDC_IMPEXPCOMPLETECONFIRM), GetSysColor(COLOR_WINDOW));
                ListView_SetTextBkColor(GetDlgItem(hwndDlg,IDC_IMPEXPCOMPLETECONFIRM), GetSysColor(COLOR_WINDOW));
                
                PropSheet_SetWizButtons(hwndParent, PSWIZB_BACK|PSWIZB_FINISH);

            }
            break;
            
        default:
            PropSheet_SetWizButtons( hwndParent, PSWIZB_NEXT | PSWIZB_BACK );
            break;
        }

    }

    return TRUE ;
}


//*************************************************************
//
//  Wizard97DlgProc
//
//  Dialog proc for welcome and complete pages.
//  

BOOL_PTR CALLBACK ImpExpUserDlg::Wizard97DlgProc
(
    HWND hwndDlg, 
    UINT msg, 
    WPARAM wParam, 
    LPARAM lParam
)
{
    ReturnValue retVal;

    ImpExpUserProcess* m_pImpExp = NULL;
    DWORD m_idPage = 0;

    if( !CommonDialogProc( hwndDlg, msg, wParam, lParam, 
                        &m_pImpExp, &m_idPage, retVal))
    {
        return retVal;
    }

    if( m_idPage == IDD_IMPEXPCOMPLETE 
        && msg == WM_NOTIFY
        && PSN_WIZFINISH == ((LPNMHDR)lParam)->code)

    m_pImpExp->PerformImpExpProcess(hwndDlg);

    return retVal;;
}


//*************************************************************
//
//  TransferTypeDlg
//
//  Dialog proc for dialog where user picks transfer type
//  (import vs. export), (cookies vs. bookmarks)

BOOL_PTR CALLBACK ImpExpUserDlg::TransferTypeDlg
(
    HWND hwndDlg, 
    UINT msg, 
    WPARAM wParam, 
    LPARAM lParam
)
{
    ReturnValue retVal;

    ImpExpUserProcess* m_pImpExp = NULL;
    DWORD m_idPage = 0;

    if( !CommonDialogProc( hwndDlg, msg, wParam, lParam, 
                        &m_pImpExp, &m_idPage, retVal))
    {
        return retVal;
    }

    HWND hwndDlgItem;
   
    switch( msg)
    {
    case WM_INITDIALOG:
        {
            hwndDlgItem = GetDlgItem( hwndDlg, IDC_IMPEXPACTIONLISTBOX);

            LRESULT index;
            TCHAR szBuffer[MAX_PATH];
            const DWORD cbSize = MAX_PATH;

            if( MLLoadString( IDS_IMPFAVORITES, szBuffer, cbSize))
            {
                index = ListBox_AddString( hwndDlgItem, szBuffer);
                ListBox_SetItemData( hwndDlgItem, index, IDS_IMPFAVORITES);
            }

            if( MLLoadString( IDS_EXPFAVORITES, szBuffer, cbSize))
            {
                index = ListBox_AddString( hwndDlgItem, szBuffer);
                ListBox_SetItemData( hwndDlgItem, index, IDS_EXPFAVORITES);
            }
            
            if( MLLoadString( IDS_IMPCOOKIES, szBuffer, cbSize))
            {
                index = ListBox_AddString( hwndDlgItem, szBuffer);
                ListBox_SetItemData( hwndDlgItem, index, IDS_IMPCOOKIES);
            }
            
            if( MLLoadString( IDS_EXPCOOKIES, szBuffer, cbSize))
            {
                index = ListBox_AddString( hwndDlgItem, szBuffer);
                ListBox_SetItemData( hwndDlgItem, index, IDS_EXPCOOKIES);
            }

            // Select the first list item, by default
            ListBox_SetCurSel(hwndDlgItem, 0);
            HandleTransferTypeChange(hwndDlg, m_pImpExp, IDS_IMPFAVORITES);

        }  // end of WM_INITDIALOG
        break;
        
    case WM_COMMAND:
        //  when the user selects an option, choose it and
        //and update the description box.
        hwndDlgItem = GetDlgItem(hwndDlg, IDC_IMPEXPACTIONLISTBOX);

        if(hwndDlgItem == (HWND)lParam
           && HIWORD(wParam) == LBN_SELCHANGE)
        {

            //  find out which string resource was selected.
            LRESULT index = ListBox_GetCurSel(hwndDlgItem);
            LRESULT selection = ListBox_GetItemData(hwndDlgItem, index);

            HandleTransferTypeChange ( hwndDlg, m_pImpExp, (UINT)selection ) ;
            retVal = TRUE;

        }
        break;
        
    case WM_NOTIFY:

        //
        //  Prevent advancement until user has made valid choices
        //
        if( ((LPNMHDR)lParam)->code == PSN_WIZNEXT
            &&  (m_pImpExp->GetExternalType() == INVALID_EXTERNAL
                 || m_pImpExp->GetTransferType() == INVALID_TRANSFER))
        {
            SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, -1);
            retVal = TRUE;
        }

        //
        // otherwise, set the filename to nul (so we get the default)
        // and allow default navigation behavior
        //
        m_pImpExp->m_szFileName[0] = TEXT('\0');

        break;
    }

    return retVal;
}

void ImpExpUserDlg::HandleTransferTypeChange ( HWND hwndDlg, ImpExpUserProcess* m_pImpExp, UINT iSelect )
{

    TCHAR szBuffer[MAX_PATH];
    const DWORD cbSize = MAX_PATH;

    //
    //  Note:  The description of each option has a resource id
    //  which is one higher than the resource id of the option name.
    //
    switch( iSelect )
    {
    case IDS_IMPFAVORITES:
        if( MLLoadString( IDS_IMPFAVORITES + 1, szBuffer, cbSize ) )
            SetWindowText( GetDlgItem( hwndDlg, IDC_IMPEXPACTIONDESCSTATIC ),
                           szBuffer );
        m_pImpExp->SelectExternalType( BOOKMARKS );
        m_pImpExp->SelectTransferType( IMPORT );
        break;
        
    case IDS_EXPFAVORITES:
        if( MLLoadString( IDS_EXPFAVORITES + 1, szBuffer, cbSize ) )
            SetWindowText( GetDlgItem( hwndDlg, IDC_IMPEXPACTIONDESCSTATIC ),
                           szBuffer );
        m_pImpExp->SelectExternalType( BOOKMARKS );
        m_pImpExp->SelectTransferType( EXPORT );
        break;
        
    case IDS_IMPCOOKIES:
        if( MLLoadString( IDS_IMPCOOKIES + 1, szBuffer, cbSize))
            SetWindowText( GetDlgItem( hwndDlg, IDC_IMPEXPACTIONDESCSTATIC),
                           szBuffer);
        m_pImpExp->SelectExternalType( COOKIES);
        m_pImpExp->SelectTransferType( IMPORT);
        break;

    case IDS_EXPCOOKIES:
        if( MLLoadString( IDS_EXPCOOKIES + 1, szBuffer, cbSize))
            SetWindowText( GetDlgItem( hwndDlg, IDC_IMPEXPACTIONDESCSTATIC),
                           szBuffer);
        m_pImpExp->SelectExternalType( COOKIES);
        m_pImpExp->SelectTransferType( EXPORT);
        break;
    }

}


//*************************************************************
//
//  InternalDlg
//
//  Allows user to pick internal target/source from tree view.

BOOL_PTR CALLBACK ImpExpUserDlg::InternalDlg
(
    HWND hwndDlg, 
    UINT msg, 
    WPARAM wParam, 
    LPARAM lParam
)
{
    ReturnValue retVal;

    ImpExpUserProcess* m_pImpExp = NULL;
    DWORD m_idPage = 0;

    if( !CommonDialogProc( hwndDlg, msg, wParam, lParam, 
                        &m_pImpExp, &m_idPage, retVal))
    {
        return retVal;
    }


    HWND hwndDlgItem;

    switch( msg)
    {
    case WM_INITDIALOG:

        //
        // Populate the tree control
        //
        hwndDlgItem = GetDlgItem(hwndDlg, IDC_IMPEXPFAVTREE);
        if ( hwndDlgItem )
        {
            m_pImpExp->PopulateTreeViewForInternalSelection(hwndDlgItem);
            m_pImpExp->ExpandTreeViewRoot ( hwndDlgItem ) ;
        }
        else
            ASSERT(0);

        return TRUE;


    case WM_NOTIFY:
        switch( ((LPNMHDR)lParam)->code)
        {

        case PSN_WIZNEXT:

            //  Only allow user to go to next if there is a valid selection.
            if( !m_pImpExp->SelectInternalSelection(GetDlgItem(hwndDlg,IDC_IMPEXPFAVTREE)) )
            {
                SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, -1);
                retVal = TRUE;
            }

        }
    }

    return retVal;
}

BOOL IsValidFileOrURL(LPTSTR szFileOrURL)
{
    if (szFileOrURL == NULL)
        return FALSE;

    //
    // any URL is ok
    //
    if (PathIsURL(szFileOrURL))
        return TRUE;

    //
    // just a directory is no good, we need a filename too
    //
    if (PathIsDirectory(szFileOrURL))
        return FALSE;

    //
    // just a filename is no good, we need a directory too
    //
    if (PathIsFileSpec(szFileOrURL))
        return FALSE;

    //
    // relative paths are no good
    //
    if (PathIsRelative(szFileOrURL))
        return FALSE;

    //
    // now make sure it parses correctly
    //
    if (PathFindFileName(szFileOrURL) == szFileOrURL)
        return FALSE;

    return TRUE;

}

//*************************************************************
//
//  ExternalDlg
//
//  Allows user to pick external target/source from list box
//or manual browse.

BOOL_PTR CALLBACK ImpExpUserDlg::ExternalDlg
(
    HWND hwndDlg, 
    UINT msg, 
    WPARAM wParam, 
    LPARAM lParam
)
{
    ReturnValue retVal;

    ImpExpUserProcess* m_pImpExp = NULL;
    DWORD m_idPage = 0;

    if( !CommonDialogProc( hwndDlg, msg, wParam, lParam, 
                           &m_pImpExp, &m_idPage, retVal))
    {
        return retVal;
    }

    HWND hwndDlgItem;
    
    switch(msg)
    {

    case WM_COMMAND:

        hwndDlgItem = (HWND) lParam;
        if( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_IMPEXPBROWSE)
        {
            OPENFILENAME ofn;
            TCHAR szFile[MAX_PATH];
            TCHAR szTitle[MAX_PATH];
            TCHAR szFilter[MAX_PATH];
            TCHAR szInitialPath[MAX_PATH];
            int i;

            ZeroMemory(&ofn, sizeof(OPENFILENAME));
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = hwndDlg;
            ofn.hInstance = MLGetHinst();
            ofn.lpstrFilter = szFilter;
            ofn.nFilterIndex = 1;
            ofn.lpstrCustomFilter = NULL;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFileTitle = NULL;
            ofn.lpstrInitialDir = szInitialPath;
            ofn.lpstrTitle = szTitle;
            ofn.lpstrDefExt = (m_pImpExp->GetExternalType()==COOKIES) ? TEXT("txt") : TEXT("htm");

            GetDlgItemText(hwndDlg, IDC_IMPEXPMANUAL, szInitialPath, ARRAYSIZE(szFile));
            szFile[0] = 0;

            if (PathIsDirectory(szInitialPath))
            {
                ofn.lpstrInitialDir = szInitialPath;
                szFile[0] = TEXT('\0');    
            }
            else
            {
                TCHAR *pchFilePart;

                pchFilePart = PathFindFileName(szInitialPath);

                if (pchFilePart == szInitialPath || pchFilePart == NULL)
                {

                    if (PathIsFileSpec(szInitialPath))
                        StrCpyN(szFile,szInitialPath,MAX_PATH);
                    else
                        szFile[0] = TEXT('\0');

                    ofn.lpstrInitialDir = szInitialPath;
                    SHGetSpecialFolderPath(NULL,szInitialPath,CSIDL_DESKTOP,FALSE);

                }
                else
                {
                    pchFilePart[-1] = TEXT('\0');
                    ofn.lpstrInitialDir = szInitialPath;
                    StrCpyN(szFile,pchFilePart,MAX_PATH);
                }

            }
            
            //
            // Work out the title and the filter strings
            //
            if (m_pImpExp->GetExternalType() == BOOKMARKS)
            {
                MLLoadShellLangString(IDS_IMPEXP_CHOSEBOOKMARKFILE,szTitle,MAX_PATH);
                MLLoadShellLangString(IDS_IMPEXP_BOOKMARKFILTER,szFilter,MAX_PATH);
            }
            else
            {
                MLLoadShellLangString(IDS_IMPEXP_CHOSECOOKIEFILE,szTitle,MAX_PATH);
                MLLoadShellLangString(IDS_IMPEXP_COOKIEFILTER,szFilter,MAX_PATH);
            }

            //
            // Search and replace '@' with nul in the filter string
            //
            for (i=0; szFilter[i]; i++)
                if (szFilter[i]==TEXT('@'))
                    szFilter[i]=TEXT('\0');

            //
            // Set the flags for openfilename
            //
            ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY ;
            if (m_pImpExp->GetTransferType() == IMPORT)
                ofn.Flags |= (OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST);

            //
            // Show the dialog
            //
            if(GetSaveFileName(&ofn))
                if(SetWindowText(GetDlgItem(hwndDlg, IDC_IMPEXPMANUAL), ofn.lpstrFile))
                {
                    Button_SetCheck(GetDlgItem( hwndDlg, IDC_IMPEXPRADIOFILE), BST_CHECKED);
                    Button_SetCheck(GetDlgItem( hwndDlg, IDC_IMPEXPRADIOAPP), BST_UNCHECKED);
                }

            retVal = TRUE;
        }
        break;

    case WM_NOTIFY:
        switch( ((LPNMHDR)lParam)->code )
        {

        case PSN_SETACTIVE:
            {

                TCHAR sBuffer[MAX_PATH];
                DWORD cbSize = ARRAYSIZE(sBuffer);

                hwndDlgItem = GetDlgItem( hwndDlg, IDC_IMPEXPEXTERNALCOMBO );
                
                //
                // Load the "application list" into the combo box.
                // If the list is empty, then disable the combo box, 
                // disable the associated radio button, and select the
                // "to/from file" option (the second radio button).
                //
                if( hwndDlgItem != NULL
                    && m_pImpExp->PopulateComboBoxForExternalSelection( hwndDlgItem ) )
                {
                    EnableWindow ( GetDlgItem(hwndDlg, IDC_IMPEXPRADIOAPP), TRUE ) ;
                    EnableWindow ( hwndDlgItem, TRUE ) ;
                    Button_SetCheck( GetDlgItem( hwndDlg, IDC_IMPEXPRADIOAPP), BST_CHECKED);
                    Button_SetCheck( GetDlgItem( hwndDlg, IDC_IMPEXPRADIOFILE), BST_UNCHECKED);
                }
                else if ( hwndDlgItem != NULL)
                {
                    EnableWindow ( GetDlgItem(hwndDlg, IDC_IMPEXPRADIOAPP), FALSE ) ;
                    EnableWindow( hwndDlgItem, FALSE ) ;
                    Button_SetCheck( GetDlgItem( hwndDlg, IDC_IMPEXPRADIOFILE), BST_CHECKED);
                    Button_SetCheck( GetDlgItem( hwndDlg, IDC_IMPEXPRADIOAPP), BST_UNCHECKED);
                }
                
                //  Put a default value in the browse option.
                if(m_pImpExp->GetExternalManualDefault(sBuffer, &cbSize))
                    SetDlgItemText(hwndDlg, IDC_IMPEXPMANUAL, sBuffer);

                SHAutoComplete(GetDlgItem(hwndDlg, IDC_IMPEXPMANUAL), SHACF_FILESYSTEM);
            }
            break;

        case PSN_WIZNEXT:
            
            //    If the application radio button is checked,
            //  select the selection from the application combo box.  If
            //  the manual button is checked, select the selection
            //  using the manual edit box.

            retVal = TRUE;
            
            if (Button_GetCheck(GetDlgItem(hwndDlg,IDC_IMPEXPRADIOAPP)) == BST_CHECKED)
            {
                
                HWND hwndComboBox = GetDlgItem(hwndDlg,IDC_IMPEXPEXTERNALCOMBO);
                
                if (hwndComboBox != NULL)
                {
                    // Find out the index of the selected item
                    INT nIndex = ComboBox_GetCurSel(hwndDlg);
                    
                    if (nIndex != CB_ERR)
                    {
                        // Retrieve a pointer to the filename
                        LPTSTR pszFileName = (LPTSTR)ComboBox_GetItemData(hwndComboBox, nIndex);
                        
                        if (pszFileName != NULL)
                            StrCpyN(m_pImpExp->m_szFileName,pszFileName,MAX_PATH);
                        
                    }
                    
                }
            }
            else if (Button_GetCheck(GetDlgItem(hwndDlg,IDC_IMPEXPRADIOFILE)) == BST_CHECKED)
            {
                
                // just get the text from the edit box
                GetDlgItemText(hwndDlg,IDC_IMPEXPMANUAL,m_pImpExp->m_szFileName,MAX_PATH);

                //
                // Don't allow "next" if the edit control contains a bogus filename
                //
                if (!IsValidFileOrURL(m_pImpExp->m_szFileName))
                {
                    
                    TCHAR szFmt[128];
                    TCHAR szMsg[INTERNET_MAX_URL_LENGTH+128];
                    MLLoadShellLangString(IDS_INVALIDURLFILE, szFmt, ARRAYSIZE(szFmt));
                    wnsprintf(szMsg, INTERNET_MAX_URL_LENGTH+40, szFmt, m_pImpExp->m_szFileName);
                    MLShellMessageBox(
                        hwndDlg, 
                        szMsg, 
                        (IMPORT == m_pImpExp->GetTransferType()) ? 
                           MAKEINTRESOURCE(IDS_CONFIRM_IMPTTL_FAV) : 
                           MAKEINTRESOURCE(IDS_CONFIRM_EXPTTL_FAV), 
                        MB_OK);
                    
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
                    return retVal;
                }

                //
                // If the user doesn't type an extension, then we add ".htm"
                // or ".txt" as appropriate.  Otherwise, we don't touch it.
                //
                if (*PathFindExtension(m_pImpExp->m_szFileName) == TEXT('\0'))
                {
                    PathRenameExtension(
                        m_pImpExp->m_szFileName,
                        (m_pImpExp->GetExternalType()==COOKIES) ? TEXT(".txt") : TEXT(".htm"));
                }

            }
            else
            {
                ASSERT(0);
                m_pImpExp->m_szFileName[0] = TEXT('\0');
            }

            //
            // Finally, show an overwrite or file-not-found message
            // (but supress it if importing or exporting to a web address)
            //

            if (m_pImpExp->GetExternalType() == COOKIES ||
                !PathIsURL(m_pImpExp->m_szFileName))
            {
                if ( EXPORT == m_pImpExp->GetTransferType() && 
                    GetFileAttributes(m_pImpExp->m_szFileName) != 0xFFFFFFFF )
                {
                    int answer ;
                    UINT idTitle ;
                    
                    if ( m_pImpExp->GetExternalType() == COOKIES )
                        idTitle = IDS_EXPCOOKIES ;
                    else if ( m_pImpExp->GetExternalType() == BOOKMARKS )
                        idTitle = IDS_EXPFAVORITES ;
                    else
                        ASSERT(0);
                    
                    answer = WarningMessageBox(
                        hwndDlg,
                        idTitle,
                        IDS_IMPEXP_FILEEXISTS,
                        m_pImpExp->m_szFileName,
                        MB_YESNO | MB_ICONEXCLAMATION);
                    
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (IDYES==answer)?0:-1);

                }
                else
                {
                    if (IMPORT == m_pImpExp->GetTransferType())
                    {
                        // Give the user a chance to insert the floppy if it's not already in.
                        if (SUCCEEDED(SHPathPrepareForWriteWrap(hwndDlg, NULL, m_pImpExp->m_szFileName, FO_COPY, (SHPPFW_DEFAULT | SHPPFW_IGNOREFILENAME))) &&
                            (GetFileAttributes(m_pImpExp->m_szFileName) == 0xFFFFFFFF))
                        {
                            UINT idTitle ;
                    
                            if ( m_pImpExp->GetExternalType() == COOKIES )
                                idTitle = IDS_IMPCOOKIES ;
                            else if ( m_pImpExp->GetExternalType() == BOOKMARKS )
                                idTitle = IDS_IMPFAVORITES ;
                            else
                                ASSERT(0);
                    
                            WarningMessageBox(
                                hwndDlg,
                                idTitle,
                                IDS_IMPEXP_FILENOTFOUND,
                                m_pImpExp->m_szFileName,
                                MB_OK | MB_ICONEXCLAMATION);
                    
                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
                    
                        }
                    }
                }

            }
            
            break; // PSN_WIZNEXT

        } // WM_NOTIFY

        break;
    
    } // switch(msg)

    return retVal;
}

BOOL WINAPI RunImportExportFavoritesWizard(HWND hDlg)
{

    ImpExpUserDlg::RunNewDialogProcess(hDlg);
    return TRUE;

}

int WarningMessageBox(HWND hwnd, UINT idTitle, UINT idMessage, LPCTSTR szFile, DWORD dwFlags)
{

    TCHAR szBuffer[1024];
    TCHAR szFormat[1024];

    //
    // load the string (must contain "%s")
    //
    MLLoadShellLangString(idMessage, szFormat, 1024);
   
    //
    // insert the filename
    //
    wnsprintf(szBuffer,1024,szFormat,szFile);

    //
    // display the messagebox
    //
    return MLShellMessageBox(
        hwnd,
        szBuffer,
        MAKEINTRESOURCE(idTitle),
        dwFlags);
}
