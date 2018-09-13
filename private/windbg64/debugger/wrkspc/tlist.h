// Usage:
// TList<DWORD> listDwords;
//


//
//
//
template<class T>
class TListEntry
{
public:
    // Forward and nackward links
    TListEntry * Flink;
    TListEntry * Blink;

    T m_tData;

    TListEntry() { Flink = Blink = this; }
    TListEntry(T t) { m_tData = t; Flink = Blink = this; }

    virtual ~TListEntry();

    void Remove();
};


//
// All of the entries must have been allocated by a new.
//
template<class T>
class TList
{
public:
    TListEntry<T> m_ListHead;

public:
    virtual ~TList();

public:
    // NT list wrapper functions
    BOOL IsEmpty() const { return &m_ListHead == m_ListHead.Flink; }


    // Get the data item for the first/last item in the list
    T& GetHeadData();
    T& GetTailData();


    // Preferred methods for inserting and removing elements into the list
    void InsertHead(T t);
    void InsertTail(T t);
    void RemoveHead();
    void RemoveTail();


    // Raw bare bones methods for inserting and removing elements into the list
    void RawInsertHead(TListEntry<T> * pEntry);
    void RawInsertTail(TListEntry<T> * pEntry);
    TListEntry<T> * RawRemoveHead();
    TListEntry<T> * RawRemoveTail();


public:
    // Misc helper functions
    DWORD Size() const;
    TListEntry<T> * Find(const void * const pv,
        BOOL (* pfnComp)(const void * const pv, T t));
    void ClearList();

public:
    TListEntry<T> * FirstEntry() const { return m_ListHead.Flink; }
    TListEntry<T> * LastEntry() const { return m_ListHead.Blink; }
    const TListEntry<T> * Stop() const { return &m_ListHead; }
};


//
// TListEntry
//
template<class T>
TListEntry<T>::~TListEntry()
{
    Remove();
}

template<class T>
inline
void
TListEntry<T>::Remove()
{
    Blink->Flink = Flink;
    Flink->Blink = Blink;
    Flink = Blink = this;
}


//
// TList
//
template<class T>
TList<T>::~TList()
{
    ClearList();
}

template<class T>
inline
T&
TList<T>::GetHeadData()
{
    return m_ListHead.Flink->m_tData;
}

template<class T>
inline
T&
TList<T>::GetTailData()
{
    return m_ListHead.Blink->m_tData;
}

template<class T>
inline
void
TList<T>::InsertHead(T t)
{
    RawInsertHead( new TListEntry<T>(t) );
}

template<class T>
inline
void
TList<T>::InsertTail(T t)
{
    RawInsertTail( new TListEntry<T>(t) );
}

template<class T>
inline
void
TList<T>::RawInsertHead(TListEntry<T> * pEntry)
{
    Assert(pEntry);

    pEntry->Flink = m_ListHead.Flink;
    pEntry->Blink = &m_ListHead;

    pEntry->Flink->Blink = pEntry;

    m_ListHead.Flink = pEntry;
}

template<class T>
inline
void
TList<T>::RawInsertTail(TListEntry<T> * pEntry)
{
    Assert(pEntry);

    pEntry->Blink = m_ListHead.Blink;
    pEntry->Flink = &m_ListHead;

    pEntry->Blink->Flink = pEntry;

    m_ListHead.Blink = pEntry;
}

template<class T>
inline
void
TList<T>::RemoveHead()
{
    delete m_ListHead.Flink;
}

template<class T>
inline
void
TList<T>::RemoveTail()
{
    delete m_ListHead.Blink;
}

template<class T>
inline
TListEntry<T> *
TList<T>::RawRemoveHead()
{
    TListEntry<T> * pEntry = m_ListHead.Flink;
    pEntry->Remove();
    return pEntry;
}

template<class T>
inline
TListEntry<T> *
TList<T>::RawRemoveTail()
{
    TListEntry<T> * pEntry = m_ListHead.Blink;
    pEntry->Remove();
    return pEntry;
}

template<class T>
DWORD
TList<T>::Size() const
{
    DWORD dw = 0;

    TListEntry<T> * pEntry = FirstEntry();
    for (; pEntry != Stop(); pEntry = pEntry->Flink) {
        dw++;
    }
    return dw;
}

template<class T>
TListEntry<T> *
TList<T>::Find(
    const void * const pv,
    BOOL (* pfnComp)(const void * const pv, T t)
    )
{
    Assert(pv);
    Assert(pfnComp);

    TListEntry<T> * pEntry = FirstEntry();
    for (; pEntry != Stop(); pEntry = pEntry->Flink) {

        Assert(pEntry->m_tData);

        if (pfnComp(pv, pEntry->m_tData)) {
            return pEntry;
        }
    }
    return NULL;
}


template<class T>
void
TList<T>::ClearList()
{
    while (!IsEmpty()) {
        delete m_ListHead.Flink;
    }
}
