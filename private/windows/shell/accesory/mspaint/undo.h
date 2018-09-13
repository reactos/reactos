#ifndef __UNDO_H__
#define __UNDO_H__


// A CBmObjSequence holds the codes for one undo or redo operation.
class CBmObjSequence : public CByteArray
    {
    public:

     CBmObjSequence();
    ~CBmObjSequence();

    void Retrieve(BYTE* rgb, int cb);
    void RetrieveStr(CString& str);

    inline void RetrieveByte(BYTE& b)     { Retrieve(&b, 1); }
    inline void RetrieveInt(int& n)       { Retrieve((BYTE*)&n  , sizeof (int)); }
    inline void RetrieveLong(long& n)     { Retrieve((BYTE*)&n  , sizeof (long)); }
    inline void RetrieveNum(double& num)  { Retrieve((BYTE*)&num, sizeof (double)); }
    inline void RetrievePtr(CBitmapObj*& ptr)  { Retrieve((BYTE*)&ptr, sizeof (CBitmapObj*)); }
    inline void RetrieveRect(CRect& rc)   { Retrieve((BYTE*)&rc , sizeof (rc)); }
    inline void RetrievePoint(CPoint& pt) { Retrieve((BYTE*)&pt , sizeof (pt)); }

    void Cleanup();
    BOOL IsUseful(CBitmapObj*&, int&);
    void Apply();

    #ifdef _DEBUG
    void Dump();
    #endif

    int m_nCursor;
    CString m_strDescription;
    };


class CUndoBmObj : public CBitmapObj
    {
    DECLARE_DYNAMIC(CUndoBmObj)

    public:
     CUndoBmObj();
    ~CUndoBmObj();

    void BeginUndo(const TCHAR* szCmd, BOOL bResetCursor = TRUE);
    void BeginUndo(const UINT idCmd, BOOL bResetCursor = TRUE);
    void EndUndo();

    inline BOOL CanUndo() const
            { return m_nRedoSeqs < m_seqs.GetCount(); }

    inline BOOL CanRedo() const
            { return m_nRedoSeqs > 0; }

    inline BOOL InUndoRedo() const
            { return m_bPerformingUndoRedo; }

    void GetUndoString(CString& strUndo);
    void GetRedoString(CString& strRedo);

    void DoUndo();
    void DoRedo();

    void SetMaxLevels(int nLevels);
    int  GetMaxLevels() const;

    void OnSetIntProp( CBitmapObj* pChangedSlob, UINT nPropID, UINT nOldVal );

    #ifdef _DEBUG
    void Dump();
    #endif

    inline BOOL IsRecording() { return m_nRecording != 0 && m_nPauseLevel == 0; }

    inline void Pause() { m_nPauseLevel += 1; }

    inline void Resume() { ASSERT(m_nPauseLevel > 0); m_nPauseLevel -= 1; }

    enum
        {
        // Note correspondence with PRD
        opStart,
        opEnd,
        opAction,
        opIntProp,
        opLongProp,
        opBoolProp,
        opDoubleProp,
        opStrProp,
        opSlobProp,
        opRectProp,
        opPointProp
        };

    UINT Insert(const void* rgb, int cb);
    UINT InsertStr(const TCHAR* sz);

    inline UINT InsertByte(BYTE b) { return Insert(&b, 1); }
    inline UINT InsertInt(int n) { return Insert((BYTE*)&n, sizeof (int)); }
    inline UINT InsertLong(long n) { return Insert((BYTE*)&n, sizeof (long)); }
    inline UINT InsertNum(double num) { return Insert((BYTE*)&num, sizeof (double)); }
    inline UINT InsertPtr(const void* ptr)
                {
                if (ptr != NULL)
                    {
                    ASSERT(((CObject*)ptr)->IsKindOf(RUNTIME_CLASS(CBitmapObj)));
                    ((CBitmapObj*)ptr)->AddDependant(this);
                    }
                return Insert((BYTE*)&ptr, sizeof (CBitmapObj*));
                }
    inline UINT InsertRect(const CRect& rc) { return Insert((BYTE*)&rc, sizeof (CRect)); }
    inline UINT InsertPoint(const CPoint& pt) { return Insert((BYTE*)&pt, sizeof (CPoint)); }

    void Flush();

    void OnInform(CBitmapObj* pChangedSlob, UINT idChange);

    void FlushLast();

    private:

    void Truncate();

    int m_nRecording; // BeginUndo() nesting count
    int m_nPauseLevel; // Pause() nesting count

    int m_cbUndo;

    // These ?Last* variables are used to coalesce consecutive changes
    // to the same property...
    CBitmapObj* m_pLastSlob;
    int m_nLastPropID;

    // Properties...
    int m_nMaxLevels;

    CObList m_seqs; // pointers to CBmObjSequences
    int m_nRedoSeqs;
    CBmObjSequence* m_pCurSeq;

    BOOL m_bPerformingUndoRedo;

    friend class CBmObjSequence;
    };


#pragma pack(1)

class CUndoRecord
    {
    public:

    BYTE m_op;
    CBitmapObj* m_pBitmapObj;
    UINT m_nPropID;
    };


class CIntUndoRecord : public CUndoRecord
    {
    public:

    int m_nOldVal;
    };


class CLongUndoRecord : public CUndoRecord
    {
    public:

    long m_nOldVal;
    };


class CDoubleUndoRecord : public CUndoRecord
    {
    public:

    double m_numOldVal;
    };


class CRectUndoRecord : public CUndoRecord
    {
    public:

    CRect m_rectOldVal;
    };


class CPointUndoRecord : public CUndoRecord
    {
    public:

    CPoint m_ptOldVal;
    };


class CBitmapObjUndoRecord : public CUndoRecord
    {
    public:

    const CBitmapObj* m_pOldVal;
    };

#pragma pack()

extern CUndoBmObj NEAR theUndo;

#endif // __UNDO_H__
