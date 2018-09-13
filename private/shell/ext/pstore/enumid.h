#ifndef ENUMID_H
#define ENUMID_H

#define PIDL_TYPE_PROVIDER  0
#define PIDL_TYPE_TYPE      1
#define PIDL_TYPE_SUBTYPE   2
#define PIDL_TYPE_ITEM      3

typedef struct {
    DWORD dwType;
    PST_KEY KeyType;
    GUID  guid;         // guid associated with Pidl
} PIDL_CONTENT, *PPIDL_CONTENT, *LPPIDL_CONTENT;
// WCHAR Array follows.

class CEnumIDList : public IEnumIDList
{
protected:
    LONG m_ObjRefCount;

public:
    CEnumIDList(LPITEMIDLIST, BOOL);
    ~CEnumIDList();

    //
    // IUnknown methods
    //

    STDMETHOD (QueryInterface)(REFIID, LPVOID*);
    STDMETHOD_ (DWORD, AddRef)();
    STDMETHOD_ (DWORD, Release)();

    //
    // IEnumIDList
    //

    STDMETHOD (Next) (ULONG, LPITEMIDLIST*, ULONG *);
    STDMETHOD (Skip) (ULONG);
    STDMETHOD (Reset) (void);
    STDMETHOD (Clone) (LPENUMIDLIST*);

private:
    LPMALLOC m_pMalloc;

    STDMETHOD (CreateIDList) (DWORD, PST_KEY, GUID *, LPCWSTR, LPITEMIDLIST *);

    ULONG m_ulCurrent;
    DWORD m_dwType;
    PST_KEY m_KeyType;
    GUID m_guidType;
    GUID m_guidSubtype;

    BOOL m_bEnumItems;

    IEnumPStoreProviders    *m_pIEnumProviders;
    IPStore                 *m_pIPStoreProvider;
    IEnumPStoreTypes        *m_pIEnumTypes;
    IEnumPStoreTypes        *m_pIEnumTypesGlobal;
    IEnumPStoreTypes        *m_pIEnumSubtypes;
    IEnumPStoreItems        *m_pIEnumItems;
};

#endif   // ENUMID_H
