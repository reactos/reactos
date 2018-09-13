#include "stdafx.h"
#include "global.h"
#include "pbrush.h"
#include "pbrusdoc.h"
#include "bmobject.h"
#include "undo.h"
#include "props.h"
#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CUndoBmObj, CBitmapObj)

#include "memtrace.h"

CUndoBmObj NEAR theUndo;

static BOOL m_bFlushAtEnd;

/////////////////////////////////////////////////////////////////////////////
//
// A CBmObjSequence is a packed array of slob property changes or custom
// actions.  Each record contains a property or action id, a pointer to
// a slob, a property type, and a value (depending on the type).
//
// These sequences are used to store undo/redo information in theUndo.
// Each undo/redo-able thing is contained in one CBmObjSequence.
//


CBmObjSequence::CBmObjSequence() : CByteArray(), m_strDescription()
    {
    SetSize(0, 100); // increase growth rate
    m_nCursor = 0;
    }

CBmObjSequence::~CBmObjSequence()
    {
    Cleanup();
    }

// Pull an array of bytes out of the sequence.
//
void CBmObjSequence::Retrieve( BYTE* rgb, int cb )
    {
    for (int ib = 0; ib < cb; ib += 1)
        *rgb++ = GetAt(m_nCursor++);
    }

// Pull a string out the sequence.

void CBmObjSequence::RetrieveStr( CString& str )
    {
    int nStrLen;
    RetrieveInt(nStrLen);
    if (nStrLen == 0)
        {
        str.Empty();
        }
    else
        {
        BYTE* pb = (BYTE*)str.GetBufferSetLength(nStrLen);
        for (int nByte = 0; nByte < nStrLen; nByte += 1)
            *pb++ = GetAt(m_nCursor++);
        str.ReleaseBuffer(nStrLen);
        }
    }

// Traverse the sequence and remove any slobs that are contained within.
//
void CBmObjSequence::Cleanup()
    {
    m_nCursor = 0;

    while (m_nCursor < GetSize())
        {
        BYTE op;
        CBitmapObj* pSlob;
        int nPropID;

        RetrieveByte(op);

        RetrievePtr(pSlob);
        RetrieveInt(nPropID);

        switch (op)
            {
            default:
                TRACE1("Illegal undo opcode (%d)\n", op);
                ASSERT(FALSE);

            case CUndoBmObj::opAction:
                {
                int cbUndoRecord;
                RetrieveInt(cbUndoRecord);
                int ib = m_nCursor;
                pSlob->DeleteUndoAction(this, nPropID);
                m_nCursor = ib + cbUndoRecord;
                }
                break;

            case CUndoBmObj::opIntProp:
            case CUndoBmObj::opBoolProp:
                {
                int val;
                RetrieveInt(val);
                }
                break;

            case CUndoBmObj::opLongProp:
                {
                long val;
                RetrieveLong(val);
                }
                break;

            case CUndoBmObj::opDoubleProp:
                {
                double num;
                RetrieveNum(num);
                }
                break;

            case CUndoBmObj::opStrProp:
                {
                CString str;
                RetrieveStr(str);
                }
                break;

            case CUndoBmObj::opSlobProp:
                {
                CBitmapObj* pSlobVal;
                RetrievePtr(pSlobVal);
                }
                break;

            case CUndoBmObj::opRectProp:
                {
                CRect rcVal;
                RetrieveRect(rcVal);
                }
                break;

            case CUndoBmObj::opPointProp:
                {
                CPoint ptVal;
                RetrievePoint(ptVal);
                }
                break;
            }
        }
    }


// Start looking right after the begin op for ops we really need to keep.
// If none are found, the entire record is discarded below.  (For now, we
// only throw away records that are empty or consist only of selection
// change ops.)
//
BOOL CBmObjSequence::IsUseful(CBitmapObj*& pLastSlob, int& nLastPropID)
    {
    m_nCursor = 0;
    while (m_nCursor < GetSize() && GetAt(m_nCursor) == CUndoBmObj::opAction)
        {
        BYTE op;
        int nAction, cbActionRecord;
        CBitmapObj* pSlob;

        RetrieveByte(op);
        ASSERT(op == CUndoBmObj::opAction);
        RetrievePtr(pSlob);
        RetrieveInt(nAction);
        RetrieveInt(cbActionRecord);

        if (nAction != A_PreSel && nAction != A_PostSel)
            {
            // Back cursor up to the opcode...
            m_nCursor -= sizeof (int) * 2 + sizeof (CBitmapObj*) + 1;
            break;
            }

        m_nCursor += cbActionRecord;
        }

    if (m_nCursor == GetSize())
        return FALSE; // sequnce consists only of selection changes


    // Now check if we should throw this away because it's just
    // modifying the same string or rectangle property as the last
    // undoable operation...  This is an incredible hack to implement
    // a "poor man's" Multiple-Consecutive-Changes-to-a-Property-as-
    // One-Operation feature.

    BYTE op;
    RetrieveByte(op);

    if (op == CUndoBmObj::opStrProp || op == CUndoBmObj::opRectProp)
        {
        CBitmapObj* pSlob;
        int nPropID;

        RetrievePtr(pSlob);
        RetrieveInt(nPropID);

        nLastPropID = nPropID;
        pLastSlob = pSlob;
        }

    m_nCursor = 0;
    return TRUE;
    }


// Perform the property changes and actions listed in the sequence.
//
void CBmObjSequence::Apply()
    {
    m_nCursor = 0;
    while (m_nCursor < GetSize())
        {
        BYTE op;
        CBitmapObj* pSlob;
        int nPropID;

        RetrieveByte(op);
        RetrievePtr(pSlob);
        RetrieveInt(nPropID);

        switch (op)
            {
            default:
                TRACE1("Illegal undo opcode (%d)\n", op);
                ASSERT(FALSE);

            case CUndoBmObj::opAction:
                pSlob->UndoAction(this, nPropID);
                break;

            case CUndoBmObj::opIntProp:
            case CUndoBmObj::opBoolProp:
                {
                int val;
                RetrieveInt(val);
                pSlob->SetIntProp(nPropID, val);
                }
                break;
            }
        }
    }

/////////////////////////////////////////////////////////////////////////////



CUndoBmObj::CUndoBmObj() : m_seqs()
    {
    ASSERT(this == &theUndo); // only one of these is allowed!

    m_nRecording = 0;
    m_cbUndo = 0;
    m_nMaxLevels = 2;
    m_pLastSlob = NULL;
    m_nLastPropID = 0;
    m_nPauseLevel = 0;
    m_nRedoSeqs = 0;
    }


CUndoBmObj::~CUndoBmObj()
    {
    Flush();
    }


// Set the maximum number of sequences that can be held at once.
//
void CUndoBmObj::SetMaxLevels(int nLevels)
    {
    if (nLevels < 1)
        return;

    m_nMaxLevels = nLevels;
    Truncate();
    }


// Returns the maximum number of sequences that can be held at once.
//
int CUndoBmObj::GetMaxLevels() const
    {
    return m_nMaxLevels;
    }


// Call this to after a sequence is recorded to prevent the next
// sequence from being coalesced with it.
//
void CUndoBmObj::FlushLast()
    {
    m_pLastSlob = NULL;
    m_nLastPropID = 0;
    }


// Call this at the start of an undoable user action.  Calls may be nested
// as long as each call to BeginUndo is balanced with a call to EndUndo.
// Only the "outermost" calls actually have any affect on the undo buffer.
//
// The szCmd parameter should contain the text that you want to appear
// after "Undo" in the Edit menu.
//
// The bResetCursor parameter is only used internally to modify behaviour
// when recording redo sequences and you should NOT pass anything for this
// parameter.
//
void CUndoBmObj::BeginUndo(const TCHAR* szCmd, BOOL bResetCursor)
    {
#ifdef _DEBUG
    if (theApp.m_bLogUndo)
        TRACE2("BeginUndo: %s (%d)\n", szCmd, m_nRecording);
#endif

    // Handle nesting
    m_nRecording += 1;
    if (m_nRecording != 1)
        return;

    if (bResetCursor) // this is the default case
        {
        // Disable Redo for non-Undo/Redo commands...
        while (m_nRedoSeqs > 0)
            {
            delete m_seqs.GetHead();
            m_seqs.RemoveHead();
            m_nRedoSeqs -= 1;
            }
        }

    m_pCurSeq = new CBmObjSequence;
    m_pCurSeq->m_strDescription = szCmd;

    m_bFlushAtEnd = FALSE;
    }

// In most cases, this overloaded function will be called.  It takes a
// resource ID instead of a char*, allowing easier internationalization
//
void CUndoBmObj::BeginUndo(const UINT idCmd, BOOL bResetCursor)
    {
    CString strCmd;
    VERIFY(strCmd.LoadString(idCmd));

    BeginUndo(strCmd, bResetCursor);
    }


// Call this at the end of an undoable user action to cause the sequence
// since the BeginUndo to be stored in the undo buffer.
//
void CUndoBmObj::EndUndo()
    {
#ifdef _DEBUG
    if (theApp.m_bLogUndo)
        TRACE1("EndUndo: %d\n", m_nRecording - 1);
#endif

    ASSERT(m_nRecording > 0);

    // Handle nesting
    m_nRecording -= 1;
    if (m_nRecording != 0)
        return;

    if (!m_pCurSeq->IsUseful(m_pLastSlob, m_nLastPropID))
        {
        // Remove empty or otherwise useless undo records!
        delete m_pCurSeq;
        m_pCurSeq = NULL;
        return;
        }

    // We'll keep it, add it to the list...
    if (m_nRedoSeqs > 0)
        {
        // Add AFTER any redo sequences we have but before any undo's
        POSITION pos = m_seqs.FindIndex(m_nRedoSeqs - 1);
        ASSERT(pos != NULL);
        m_seqs.InsertAfter(pos, m_pCurSeq);
        }
    else
        {
        // Just add before any other undo sequences
        m_seqs.AddHead(m_pCurSeq);
        }
    m_pCurSeq = NULL;

    Truncate(); // Make sure the undo buffer doesn't get too big!

    if (m_bFlushAtEnd)
        Flush();
    }


// This functions ensures there aren't too many levels in the buffer.
//
void CUndoBmObj::Truncate()
    {
    POSITION pos = m_seqs.FindIndex(m_nRedoSeqs + m_nMaxLevels);
    while (pos != NULL)
        {
#ifdef _DEBUG
    if (theApp.m_bLogUndo)
        TRACE(TEXT("Undo record fell off the edge...\n"));
#endif
        POSITION posRemove = pos;
        delete m_seqs.GetNext(pos);
        m_seqs.RemoveAt(posRemove);
        }
    }


// Call this to perform an undo command.
//
void CUndoBmObj::DoUndo()
    {
    CWaitCursor waitCursor;

    if (m_nRedoSeqs == m_seqs.GetCount())
        return; // nothing to undo!

    m_bPerformingUndoRedo = TRUE;

    POSITION pos = m_seqs.FindIndex(m_nRedoSeqs);
    ASSERT(pos != NULL);
    CBmObjSequence* pSeq = (CBmObjSequence*)m_seqs.GetAt(pos);

    BeginUndo(pSeq->m_strDescription, FALSE); // Setup Redo

    // Remove this sequence after BeginUndo so the one inserted
    // there goes to the right place...
    m_seqs.RemoveAt(pos);

    pSeq->Apply();

    FlushLast();
    EndUndo();
    FlushLast();

    m_bPerformingUndoRedo = FALSE;

    delete pSeq;

    // Do not bump the redo count if the undo flushed the buffer!  (This
    // happens when a resource is pasted/dropped, then opened, then a
    // property in it changes, and the user undoes back to before the
    // paste.)
    if (m_seqs.GetCount() != 0)
        m_nRedoSeqs += 1;


    }


// Call this to perform a redo command.
//
void CUndoBmObj::DoRedo()
    {
    if (m_nRedoSeqs == 0)
        return; // nothing in redo buffer

    m_nRedoSeqs -= 1;
    DoUndo();

    // Do not drop the redo count if the undo flushed the buffer!  (This
    // happens when a resource is pasted/dropped, then opened, then a
    // property in it changes, and the user undoes back to before the
    // paste.)
    if (m_seqs.GetCount() != 0)
        m_nRedoSeqs -= 1;
    }


// Generate a string appropriate for the undo menu command.
//
void CUndoBmObj::GetUndoString(CString& strUndo)
    {
    static CString NEAR strUndoTemplate;

    if (strUndoTemplate.IsEmpty())
        VERIFY(strUndoTemplate.LoadString(IDS_UNDO));

    CString strUndoCmd;

    if (CanUndo())
        {
        POSITION pos = m_seqs.FindIndex(m_nRedoSeqs);
        strUndoCmd = ((CBmObjSequence*)m_seqs.GetAt(pos))->m_strDescription;
        }

    int cchUndo = strUndoTemplate.GetLength() - 2; // less 2 for "%s"
    wsprintf(strUndo.GetBufferSetLength(cchUndo + strUndoCmd.GetLength()),
             strUndoTemplate, (const TCHAR*)strUndoCmd);
    }


// Generate a string appropriate for the redo menu command.
//
void CUndoBmObj::GetRedoString(CString& strRedo)
    {
    static CString NEAR strRedoTemplate;

    if (strRedoTemplate.IsEmpty())
        VERIFY(strRedoTemplate.LoadString(IDS_REDO));

    CString strRedoCmd;

    if (CanRedo())
        {
        POSITION pos = m_seqs.FindIndex(m_nRedoSeqs - 1);
        strRedoCmd = ((CBmObjSequence*)m_seqs.GetAt(pos))->m_strDescription;
        }

    int cchRedo = strRedoTemplate.GetLength() - 2; // less 2 for "%s"
    wsprintf(strRedo.GetBufferSetLength(cchRedo + strRedoCmd.GetLength()),
        strRedoTemplate, (const TCHAR*)strRedoCmd);
    }


// Call this to completely empty the undo buffer.
//
void CUndoBmObj::Flush()
    {
    PreTerminateList(&m_seqs);

    m_cbUndo = 0;
    m_nRedoSeqs = 0;

    m_bFlushAtEnd = TRUE;
    }


void CUndoBmObj::OnInform(CBitmapObj* pChangedSlob, UINT idChange)
    {
    if (idChange == SN_DESTROY)
        {
        // When a slob we have a reference to is deleted (for real), we
        // have no choice but to flush the whole buffer...  This normally
        // only happens when a resource editor window is closed...  (If
        // the slob's container is the undo buffer, then we are already
        // in the process of flushing, so don't recurse!)

        Flush();
        }

    CBitmapObj::OnInform(pChangedSlob, idChange);
    }


//
// The following functions are used by the CBitmapObj code to insert commands
// into the undo/redo sequence currently being recorded.  All of the On...
// functions are used to record changes to the various types of properties
// and are called by the CBitmapObj::Set...Prop functions exclusively.
//


// Insert an array of bytes.
//
UINT CUndoBmObj::Insert(const void* pv, int cb)
    {
    ASSERT(m_pCurSeq != NULL);

    BYTE* rgb = (BYTE*)pv;

    m_pCurSeq->InsertAt(0, 0, cb);

    for (int ib = 0; ib < cb; ib += 1)
        m_pCurSeq->SetAt(ib, *rgb++);

    return cb;
    }


// Insert a string.
//
UINT CUndoBmObj::InsertStr(const TCHAR* sz)
    {
    ASSERT(m_pCurSeq != NULL);

    BYTE* pb = (BYTE*)sz;
    int nStrLen = lstrlen(sz);

    InsertInt(nStrLen);
    if (nStrLen > 0)
        {
        m_pCurSeq->InsertAt(sizeof (int), 0, nStrLen);
        for (int nByte = 0; nByte < nStrLen; nByte += 1)
            m_pCurSeq->SetAt(sizeof (int) + nByte, *pb++);
        }
    return nStrLen + sizeof (int);
    }


void CUndoBmObj::OnSetIntProp(CBitmapObj* pChangedSlob, UINT nPropID, UINT nOldVal)
        {
    ASSERT(m_nRecording != 0);

    CIntUndoRecord undoRecord;
    undoRecord.m_op = opIntProp;
    undoRecord.m_pBitmapObj = pChangedSlob;
    undoRecord.m_nPropID = nPropID;
    undoRecord.m_nOldVal = nOldVal;
    Insert(&undoRecord, sizeof (undoRecord));
    pChangedSlob->AddDependant(this);
    }

#ifdef _DEBUG

/////////////////////////////////////////////////////////////////////////////
//
// Undo related debugging aids
//

void CBmObjSequence::Dump()
    {
    m_nCursor = 0;
    while (m_nCursor < GetSize())
        {
        BYTE op;
        CBitmapObj* pSlob;
        int nPropID;

        RetrieveByte(op);
        RetrievePtr(pSlob);
        RetrieveInt(nPropID);

        switch (op)
            {
        default:
            TRACE1("Illegal undo opcode (%d)\n", op);
            ASSERT(FALSE);

        case CUndoBmObj::opAction:
                {
                int cbUndoRecord;
                RetrieveInt(cbUndoRecord);
                m_nCursor += cbUndoRecord;

                TRACE3("opAction: pSlob = 0x%08lx, nActionID = %d, "
                    TEXT("nBytes = %d\n"), pSlob, nPropID, cbUndoRecord);
                }
            break;

        case CUndoBmObj::opIntProp:
        case CUndoBmObj::opBoolProp:
                {
                int val;
                RetrieveInt(val);
                TRACE3("opInt: pSlob = 0x%08lx, nPropID = %d, val = %d\n",
                    pSlob, nPropID, val);
                }
            break;

        case CUndoBmObj::opLongProp:
                {
                long val;
                RetrieveLong(val);
                TRACE3("opInt: pSlob = 0x%08lx, nPropID = %d, val = %ld\n",
                    pSlob, nPropID, val);
                }
            break;

        case CUndoBmObj::opDoubleProp:
                {
                double num;
                RetrieveNum(num);
                TRACE3("opInt: pSlob = 0x%08lx, nPropID = %d, val = %f\n",
                    pSlob, nPropID, num);
                }
            break;

        case CUndoBmObj::opStrProp:
                {
                CString str;
                RetrieveStr(str);
                if (str.GetLength() > 80)
                    {
                    str = str.Left(80);
                    str += TEXT("...");
                    }
                TRACE3("opStr: pSlob = 0x%08lx, nPropID = %d, val = %s\n",
                    pSlob, nPropID, (const TCHAR*)str);
                }
            break;

        case CUndoBmObj::opSlobProp:
                {
                CBitmapObj* pSlobVal;
                RetrievePtr(pSlobVal);
                TRACE3("opInt: pSlob = 0x%08lx, nPropID = %d, "
                    TEXT("val = 0x%08lx\n"), pSlob, nPropID, pSlobVal);
                }
            break;

        case CUndoBmObj::opRectProp:
                {
                CRect rcVal;
                RetrieveRect(rcVal);
                TRACE3("opRect: pSlob = 0x%08lx, nPropID = %d, "
                    TEXT("val = %d,%d,%d,%d\n"), pSlob, nPropID, rcVal);
                }
            break;

        case CUndoBmObj::opPointProp:
                {
                CPoint ptVal;
                RetrievePoint(ptVal);
                TRACE3("opPoint: pSlob = 0x%08lx, nPropID = %d, "
                    TEXT("val = %d,%d,%d,%d\n"), pSlob, nPropID, ptVal);
                }
            break;
            }
        }
    }


void CUndoBmObj::Dump()
    {
    int nRecord = 0;
    POSITION pos = m_seqs.GetHeadPosition();
    while (pos != NULL)
        {
        CBmObjSequence* pSeq = (CBmObjSequence*)m_seqs.GetNext(pos);
        TRACE2("Record (%d) %s:\n", nRecord,
            nRecord < m_nRedoSeqs ? TEXT("redo") : TEXT("undo"));
        pSeq->Dump();
        nRecord += 1;
        }
    }


extern "C" void DumpUndo()
    {
    theUndo.Dump();
    }

#endif
