#pragma once

typedef struct tagGUIDStruct
{
    BYTE dummy; /* offset 01 is unknown */
    GUID guid;  /* offset 02 */
} GUIDStruct;

#define PT_GUID 0x1F

typedef struct tagPIDLDATA
{
    BYTE type;			/*00*/
    union
    {
        struct tagGUIDStruct guid;
        struct tagVALUEStruct value;
    } u;
} PIDLDATA, *LPPIDLDATA;

typedef struct tagENUMLIST
{
    struct tagENUMLIST *pNext;
    LPITEMIDLIST pidl;
} ENUMLIST, *LPENUMLIST;

class CEnumIDList final :
    public IEnumIDList
{
    public:
        CEnumIDList();
        BOOL AddToEnumList(LPITEMIDLIST pidl);

        // IUnknown
        virtual HRESULT WINAPI QueryInterface(REFIID riid, LPVOID *ppvOut);
        virtual ULONG WINAPI AddRef();
        virtual ULONG WINAPI Release();

        // IEnumIDList
        virtual HRESULT STDMETHODCALLTYPE Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
        virtual HRESULT STDMETHODCALLTYPE Skip(ULONG celt);
        virtual HRESULT STDMETHODCALLTYPE Reset();
        virtual HRESULT STDMETHODCALLTYPE Clone(IEnumIDList **ppenum);

    private:
        ~CEnumIDList();

        LONG        m_ref;
        LPENUMLIST  m_pFirst;
        LPENUMLIST  m_pLast;
        LPENUMLIST  m_pCurrent;
};
