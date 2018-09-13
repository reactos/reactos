
//
// an aribrary list of sized items
//

typedef struct LIST *PLIST;

typedef struct LIST {
    PVOID pData;
    UINT  nBytes;
    PLIST next;
    PLIST prev;
} LIST;

class CList {
    private:
        PLIST m_pListHead;
        PLIST m_pListCurr;
        PLIST m_pListTail;
    public:
        CList();
        ~CList();
        BOOL   IsEmpty() { return (NULL == m_pListHead); }
        void   RemoveAll();
        void   RemoveHead(PVOID pData);
        void   RemoveHead(PVOID pData, PUINT pnBytes);
        BOOL   Add(PVOID pData, UINT nBytes);
        PVOID  PeekHead() { return (IsEmpty() ? NULL : m_pListHead->pData); }
};

