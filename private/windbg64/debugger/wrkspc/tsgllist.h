// Usage:
// TList<DWORD> listDwords;
// 


//
//
//
template<class T>
class TSingle_List_Entry
{
public:
    T m_tData;

    TSingle_List_Entry * Next;

    TSingle_List_Entry() { Next = NULL; };
    TSingle_List_Entry(T t) { m_tData = t; Next = NULL; };
};


//
//
//
template<class T>
class TSingle_List
{
public:
    TSingle_List_Entry<T> m_ListHead;

public:
    TSingle_List();
    virtual ~TSingle_List();

    BOOL IsEmpty() const { return NULL == m_ListHead.Next; }

    // Preferred methods for inserting and removing elements into the list
    T PopEntry();
    TSingle_List_Entry<T> *PushEntry(T t);

    // Raw bare bones methods for inserting and removing elements into the list
    TSingle_List_Entry<T> * RawPopEntry();
    void RawPushEntry(TSingle_List_Entry<T> * pEntry);
};


//
// TSingle_List
//
template<class T>
inline
TSingle_List<T>::TSingle_List()
{
    m_ListHead.Next = NULL;
}

template<class T>
TSingle_List<T>::~TSingle_List()
{
    while (!IsEmpty()) {
        delete RawPopEntry();
    }
}

template<class T>
inline
T
TSingle_List<T>::PopEntry()
{
    Assert(!IsEmpty());

    TSingle_List_Entry<T> * pEntry = RawPopEntry();
    T tTmp = pEntry->m_tData;
    
    delete pEntry;
    return tTmp;
}

template<class T>
inline
TSingle_List_Entry<T> *
TSingle_List<T>::PushEntry(T t)
{
    TSingle_List_Entry<T> * pEntry = new TSingle_List_Entry<T>(t);
    RawPushEntry(pEntry);
    return pEntry;
}

template<class T>
inline
TSingle_List_Entry<T> *
TSingle_List<T>::RawPopEntry()
{
    Assert(!IsEmpty());

    TSingle_List_Entry<T> * pEntry = m_ListHead.Next;

    m_ListHead.Next = pEntry->Next;
    return pEntry;
}

template<class T>
inline
void
TSingle_List<T>::RawPushEntry(TSingle_List_Entry<T> * pEntry)
{
    Assert(pEntry);

    pEntry->Next = m_ListHead.Next;
    m_ListHead.Next = pEntry;
}



