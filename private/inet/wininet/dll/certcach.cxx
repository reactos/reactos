/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    certcach.cxx

Abstract:

    Contains class implementation for certificate cache object.
    This object will hold various Certificate entries.

    Contents:
        SECURITY_CACHE_LIST_ENTRY::SECURITY_CACHE_LIST_ENTRY
        SECURITY_CACHE_LIST_ENTRY::~SECURITY_CACHE_LIST_ENTRY
        SECURITY_CACHE_LIST_ENTRY::AddRef
        SECURITY_CACHE_LIST_ENTRY::Release
        SECURITY_CACHE_LIST_ENTRY::Clear
        SECURITY_CACHE_LIST::Find
        SECURITY_CACHE_LIST::Add
        SECURITY_CACHE_LIST::ClearList

    TODO:  Add Cert validation.  What if Cert is given but different?

Author:

    Arthur L Bierer (arthurbi) 20-Apr-1996

Revision History:

    20-Apr-1996 arthurbi
        Created

--*/
#include <wininetp.h>

//
// private manifests
//

#define MAX_CERT_CACHE_CERTS    16

//
// private types
//

//
// SECURITY_CACHE_LIST_ENTRY member functions
//


SECURITY_CACHE_LIST_ENTRY::SECURITY_CACHE_LIST_ENTRY(
    IN LPSTR lpszHostName
    )

/*++

Routine Description:

    SECURITY_CACHE_LIST_ENTRY constructor. Create object; don't add it to list

Arguments:

    lpszHostName    - name of host for which this cache entry created

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "SECURITY_CACHE_LIST_ENTRY::SECURITY_CACHE_LIST_ENTRY",
                 "%q",
                 lpszHostName
                 ));

#if INET_DEBUG
    _List.Flink = _List.Blink = NULL;
#endif
    _cRef = 1;
    _fInCache = FALSE;
    _ServerName = lpszHostName;
    ZeroMemory(&_CertInfo, sizeof(_CertInfo));
    _dwSecurityFlags = 0;
    _pCertContextArray = NULL;

#if INET_DEBUG
    m_Signature = 0x454c4353;   // 'SCLE'
#endif

    DEBUG_LEAVE(0);
}


SECURITY_CACHE_LIST_ENTRY::~SECURITY_CACHE_LIST_ENTRY()

/*++

Routine Description:

    SECURITY_CACHE_LIST_ENTRY destructor.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "~SECURITY_CACHE_LIST_ENTRY",
                 "{%#x [%q]}",
                 this,
                 _ServerName.StringAddress()
                 ));

    INET_ASSERT((_List.Flink == NULL) && (_List.Blink == NULL));

    Clear();

    DEBUG_LEAVE(0);
}


LONG
SECURITY_CACHE_LIST_ENTRY::AddRef(
    VOID
    )

/*++

Routine Description:

    Increment reference count of SECURITY_CACHE_LIST_ENTRY

Arguments:

    None.

Return Value:

    LONG    - reference count after increment

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 Int,
                 "SECURITY_CACHE_LIST_ENTRY::AddRef",
                 "{%#x [%q, %d]}",
                 this,
                 _ServerName.StringAddress(),
                 _cRef
                 ));

    InterlockedIncrement(&_cRef);

    DEBUG_LEAVE(_cRef);

    return _cRef;
}


LONG
SECURITY_CACHE_LIST_ENTRY::Release(
    VOID
    )

/*++

Routine Description:

    Decrement reference count and destroy object if (<=) zero

Arguments:

    None.

Return Value:

    LONG    - reference count after decrement

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 Int,
                 "SECURITY_CACHE_LIST_ENTRY::Release",
                 "{%q [%d]}",
                 _ServerName.StringAddress(),
                 _cRef
                 ));

    LONG cRet;

    if (0 >= (cRet = InterlockedDecrement(&_cRef))) {
        delete this;
    }

    DEBUG_LEAVE(cRet);

    return cRet;
}


VOID
SECURITY_CACHE_LIST_ENTRY::Clear()

/*++

Routine Description:

    Clear out SECURITY_CACHE_LIST_ENTRY

Arguments:

    Clear   -

Return Value:

    None.

--*/

{
    if (_CertInfo.pCertificate != NULL) {
        __try {
            CertFreeCertificateContext(_CertInfo.pCertificate);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
        }
        ENDEXCEPT
        _CertInfo.pCertificate = NULL;
    }

    ZeroMemory(&_CertInfo, sizeof(_CertInfo));
    _CertInfo.dwSize = sizeof(_CertInfo);

    _dwSecurityFlags = 0;
    _ServerName = NULL;
    _pCertContextArray = NULL;
    if( _pCertContextArray )
    {
        delete _pCertContextArray;
        _pCertContextArray = NULL;
    }
}

//
// SECURITY_CACHE_LIST member functions
//


VOID
SECURITY_CACHE_LIST::ClearList(
    VOID
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "SECURITY_CACHE_LIST::ClearList",
                 NULL
                 ));

    LockSerializedList(&_List);

    while (!IsSerializedListEmpty(&_List)) {

        SECURITY_CACHE_LIST_ENTRY * CacheEntry;

        //
        // remove the PROXY_SERVER_LIST_ENTRY at the head of the serialized
        // list
        //

        LPVOID entry = SlDequeueHead(&_List);

        //
        // entry should not be NULL - IsSerializedListEmpty() told us we
        // could expect something
        //

        INET_ASSERT(entry != NULL);

        //
        // get the address of the object (should be the same as entry) and
        // delete it
        //

        CacheEntry = CONTAINING_RECORD(entry, SECURITY_CACHE_LIST_ENTRY, _List);

        DEBUG_PRINT(OBJECTS,
                    INFO,
                    ("releasing %q (%d)\n",
                    CacheEntry->_ServerName.StringAddress(),
                    CacheEntry->_cRef
                    ));

        CacheEntry->Release();
    }

    UnlockSerializedList(&_List);

    DEBUG_LEAVE(0);
}


DWORD
SECURITY_CACHE_LIST::Add(
    IN SECURITY_CACHE_LIST_ENTRY * entry
    )

/*++

Routine Description:

    Adds a CertInfo Structure to the list front of the list.

Arguments:

    lpszHost    - Hostname to add.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "SECURITY_CACHE_LIST::Add",
                 "%#x [%q, %d]",
                 entry,
                 entry ? entry->_ServerName.StringAddress() : "",
                 entry ? entry->_cRef : 0
                 ));

    DWORD error = ERROR_SUCCESS;

    INET_ASSERT(entry != NULL);

    if (entry != NULL) {
        LockSerializedList(&_List);

        //
        // If we've grown too much, nuke the oldest one.
        //

        if (ElementsOnSerializedList(&_List) >= MAX_CERT_CACHE_CERTS) {

            SECURITY_CACHE_LIST_ENTRY *pOld;
            LPVOID old_entry = SlDequeueTail(&_List);

            INET_ASSERT(old_entry != NULL);

            pOld = CONTAINING_RECORD(old_entry, SECURITY_CACHE_LIST_ENTRY, _List);

            //
            // entry should not be NULL - IsSerializedListEmpty() told us we
            // could expect something
            //

            pOld->_fInCache = FALSE;

            //
            // Clean Our old object, and reinstatiate with a new name.
            //

            pOld->Release();
        }
        InsertAtHeadOfSerializedList(&_List, &entry->_List);
        entry->AddRef();
        entry->_fInCache = TRUE;
        UnlockSerializedList(&_List);
    }

    DEBUG_LEAVE(error);

    return error;
}


SECURITY_CACHE_LIST_ENTRY *
SECURITY_CACHE_LIST::Find(
    IN LPSTR lpszHost
    )

/*++

Routine Description:

    Searches the linked list for the Cert, and returns
    the found entry, or NULL if not found.

Arguments:

    lpszHost    - Hostname to search on.

Return Value:

    CERT_CACHE_LIST_ENTRY *
        Success - Pointer to found entry.

        Failure - NULL, not found.

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Pointer,
                 "SECURITY_CACHE_LIST::Find",
                 "%q",
                 lpszHost
                 ));

    SECURITY_CACHE_LIST_ENTRY * info = NULL;

    //
    // BUGBUG need to validate against Server Certifcate on every
    //  connection, this Find only validates by Hostname.
    //  What about DNS spoofing?  Won't we be hosed?
    //

    //
    // TODO if found, need to push to front of list.
    //

    LockSerializedList(&_List);
    for (PLIST_ENTRY entry = HeadOfSerializedList(&_List);
        entry != (PLIST_ENTRY)SlSelf(&_List);
        entry = entry->Flink)
    {
        info = CONTAINING_RECORD(entry, SECURITY_CACHE_LIST_ENTRY, _List);

        //
        // check to see if they match.
        //

        if (info->_ServerName.Stricmp(lpszHost) == 0) {
            info->AddRef();
            break; // match.
        }
        info = NULL;
    }
    UnlockSerializedList(&_List);

    DEBUG_LEAVE(info);

    return info;
}

VOID
SECURITY_CACHE_LIST::Remove(
    IN LPSTR lpszHost
    )

/*++

Routine Description:

    Searches the linked list for the Cert, and removes
    the entry.

Arguments:

    lpszHost    - Hostname to search on.

Return Value:

    None

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Pointer,
                 "SECURITY_CACHE_LIST::Remove",
                 "%q",
                 lpszHost
                 ));

    SECURITY_CACHE_LIST_ENTRY * info = NULL;


    LockSerializedList(&_List);
    for (PLIST_ENTRY entry = HeadOfSerializedList(&_List);
        entry != (PLIST_ENTRY)SlSelf(&_List);
        entry = entry->Flink)
    {
        info = CONTAINING_RECORD(entry, SECURITY_CACHE_LIST_ENTRY, _List);

        //
        // check to see if they match.
        //

        if (info->_ServerName.Stricmp(lpszHost) == 0) {
            RemoveFromSerializedList(&_List, entry);
            break; // match.
        }
        info = NULL;
    }
    UnlockSerializedList(&_List);

    DEBUG_LEAVE(0);

}
