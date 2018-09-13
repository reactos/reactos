//-----------------------------------------------------------------------------
//
// Handy-Dandy doubly linked list package.
//
// These lists cannot be copied or assigned (we can add this later if someone
// cares.)
//
// (Although these are implemented as rings the user sees normal linked lists
// in that Prev and Next return NULL when walking off of the ends.)
//
// Created by tedsmith
//

// To support a variety of uses these templates implement three interfaces:
//
//    TIntrusiveDblLinkedList:
//      User must derive his object from the linked list's magic link type:
//        CIntrusiveDblLinkedListNode.
//      User's object can only exist in 1 linked list at a time.
//      User is responsible for memory management of objects - objects must be
//        excised from linked list before object is released to free store.
//      Since user's pointer and list's pointer are the same, an iterator is not
//        required to walk the list.  Prev, Next, etc. are provided both by the
//        List and the List's Iterator (see iterator notes notes below,
//        however.)
//
//    TValueDblLinkedList
//      User's objects are copied in and out of linked cells
//      A copy of the user's object can exist in many linked lists at a time.
//      Linked list abstraction is responsible for memory management.
//      Requires that the user's object has a copy constructor, a destructor and
//        an operator== (for Find)
//      Since the user can't get a pointer to the object embedded in the list,
//        an iterator is required to walk the list.  Next, etc. are provided
//        only via an iterator.
//
//    TPointerDblLinkedList
//      A pointer to the user's object is stored in the linked list
//      User's object can exist in many linked lists at a time.
//      User is responsible for memory management of objects - objects must be
//        excised from linked list before object is released to free store.
//      Since the user can't get a pointer to the object embedded in the list,
//        an iterator is required to walk the list.  Next, etc. are provided
//        only via an iterator.
//
// The iterator model is:
//    Iterator's positions (i.e. Key()) are undefined just after construction or
//      being Reset.  (This is the "advance and then return" rule.)
//    Passing a NULL to Reset as the position sets the iterator to its initial
//      state (and is useful for getting to the first (with Next()) or last
//      (Prev()) object quickly.)
//    When the object that an iterator is positioned at is deleted exogenously
//      the state of that iterator is undefined and it will fail unless it is
//      Reset (or destroyed.)  For this reason it is best when deleting to
//      either use the base list and no iterators or to delete with exactly one
//      active iterator.
//    Although Intrusive iterator's operator(), Prev and Next return a pointer
//      to the new current item or NULL if the end of the list is reached;
//      this isn't done for Value or Pointer based list iterators.  The most
//      generic iterator idiom is therefore:
//
//           T...List<CFoo> FooList;
//           ...
//           T...ListIterator<CFoo> i(FooList);
//           while (i())
//           {
//               CFoo *pCurrentFoo = i.Key();
//               ... pCurrentFoo ...
//           }

#ifndef I_DBLLINK_HXX_
#define I_DBLLINK_HXX_
#pragma INCMSG("--- Beg 'dbllink.hxx'")

//-----------------------------------------------------------------------------
//
// Debugging declarations
//

#if DBG == 1
#define CALL_ISVALIDOBJECT(this) ((this)->IsValidObject())
#define DECLARE_ISVALIDOBJECT    virtual void IsValidObject() const;
#else
#define CALL_ISVALIDOBJECT(this)
#define DECLARE_ISVALIDOBJECT
#endif


//-----------------------------------------------------------------------------
//
// Internal helper class
//
// Implements one copy of common code for all ...DblLinkedList... templates
// Don't declare any of these
//
class CDblLinkedListBase
{
    friend class CDblLinkedListBaseIterator;
public:
    class Node
    {
        friend class CDblLinkedListBase;
        friend class CDblLinkedListBaseIterator;
    public:
        Node();
        ~Node() {}
    private:
        NO_COPY(Node);
        Node *_pPrev;
        Node *_pNext;
    };

    DECLARE_ISVALIDOBJECT

#ifdef WIN16
    // The watcom compiler spits out weird warnings if this is protected.
#else
    protected:
#endif
    CDblLinkedListBase() { _Anchor._pPrev = _Anchor._pNext = &_Anchor; }
    ~CDblLinkedListBase();

protected:

    void  Prepend(Node*);
    void  Append(Node*);
#if DBG == 1
    BOOL  Contains(const Node*) const;
#endif // DBG == 1
    Node *First()               const;
    Node *Last()                const;
    Node *Prev(const Node*)     const;
    Node *Next(const Node*)     const;
    BOOL  IsEmpty()             const { return IsAnchor(_Anchor._pNext); }
    void  Remove(Node*);
    void  InsertAfter(Node *pPosition, Node *pNewNode);
    void  InsertBefore(Node *pPosition, Node *pNewNode);

private:
    BOOL  IsAnchor(const Node *pn) const { return pn == &_Anchor; }
    Node  _Anchor;
    NO_COPY(CDblLinkedListBase);
};


typedef CDblLinkedListBase::Node CIntrusiveDblLinkedListNode;

inline CIntrusiveDblLinkedListNode::Node()
{
#if DBG == 1
    _pPrev = _pNext = NULL;
#endif
}


inline CDblLinkedListBase::~CDblLinkedListBase()
{
    CALL_ISVALIDOBJECT(this);
    Assert("List should be empty now" &&
           (IsEmpty()) );
}


inline CIntrusiveDblLinkedListNode *
CDblLinkedListBase::Prev(const Node *pNode) const
{
    Assert("Node must exist in Linked list" &&
           (pNode && Contains(pNode)) );

    Node  *ret = IsAnchor(pNode->_pPrev) ? NULL : pNode->_pPrev;
    return ret;
}


inline CIntrusiveDblLinkedListNode *
CDblLinkedListBase::Next(const Node *pNode) const
{
    Assert("Node must exist in Linked list" &&
           (pNode && Contains(pNode)) );

    Node  *ret = IsAnchor(pNode->_pNext) ? NULL : pNode->_pNext;
    return ret;
}


inline CIntrusiveDblLinkedListNode *
CDblLinkedListBase::First() const
{
    return Next(&_Anchor);
}


inline CIntrusiveDblLinkedListNode *
CDblLinkedListBase::Last() const
{
    return Prev(&_Anchor);
}


inline void
CDblLinkedListBase::Prepend(Node *pNode)
{
    InsertAfter(&_Anchor, pNode);
}


inline void
CDblLinkedListBase::Append(Node *pNode)
{
    InsertBefore(&_Anchor, pNode);
}


//-----------------------------------------------------------------------------
//
// Internal helper class
//
// Implements one copy of common code for all ...DblLinkedList... iterators
// Don't declare any of these
//

class CDblLinkedListBaseIterator
{
public:
    typedef CIntrusiveDblLinkedListNode Node;
    DECLARE_ISVALIDOBJECT

protected:
    CDblLinkedListBaseIterator() : _pList(NULL), _pNode(NULL) {}
    CDblLinkedListBaseIterator(CDblLinkedListBase &rList);
    CDblLinkedListBaseIterator(CDblLinkedListBase &rList, Node *pNode);
    ~CDblLinkedListBaseIterator() {}

    void  Reset();
    void  Reset(Node *pNode);
    void  Reset(CDblLinkedListBase &rList);
    void  Reset(CDblLinkedListBase &rList, Node *pNode);

    Node *Prev();
    Node *Next();
    Node *operator()();
    Node *Key() const { return _pNode; }

    Node *Remove();
    void  InsertAfter(Node *);
    void  InsertBefore(Node *);

private:
    CDblLinkedListBase *_pList;
    Node               *_pNode;
};


inline void
CDblLinkedListBaseIterator::Reset(CDblLinkedListBase &rList, Node *pNode)
{
    _pList = &rList;
    _pNode = pNode;
}


inline void
CDblLinkedListBaseIterator::Reset(CDblLinkedListBase &rList)
{
    _pList = &rList;
    _pNode = &rList._Anchor;
}


inline void
CDblLinkedListBaseIterator::Reset(Node *pNode)
{
    _pNode = pNode;
}


inline void
CDblLinkedListBaseIterator::Reset()
{
    _pNode = &_pList->_Anchor;
}


inline
CDblLinkedListBaseIterator::CDblLinkedListBaseIterator(
    CDblLinkedListBase &rList, Node *pNode )
{
    Reset(rList, pNode);
}


inline
CDblLinkedListBaseIterator::CDblLinkedListBaseIterator(
    CDblLinkedListBase &rList )
{
    Reset(rList);
}


inline CIntrusiveDblLinkedListNode *
CDblLinkedListBaseIterator::operator()()
{
    CALL_ISVALIDOBJECT(this);

    _pNode = _pList->Next(_pNode);
    return _pNode;
}


inline CIntrusiveDblLinkedListNode *
CDblLinkedListBaseIterator::Prev()
{
    CALL_ISVALIDOBJECT(this);

    _pNode = _pList->Prev(_pNode);
    return _pNode;
}


inline CIntrusiveDblLinkedListNode *
CDblLinkedListBaseIterator::Next()
{
    return (*this)(); // The compiler only inlines to a certain depth
}                     //  I like "while (i())" better that "while (i.Next())"


inline CIntrusiveDblLinkedListNode *
CDblLinkedListBaseIterator::Remove()
{
    CALL_ISVALIDOBJECT(this);

    Node *ret = _pNode;
    _pNode = _pNode->_pPrev;
    _pList->Remove(ret);
    return ret;
}


inline void
CDblLinkedListBaseIterator::InsertAfter(Node *pNewNode)
{
    CALL_ISVALIDOBJECT(this);
    _pList->InsertAfter(_pNode, pNewNode);
}


inline void
CDblLinkedListBaseIterator::InsertBefore(Node *pNewNode)
{
    CALL_ISVALIDOBJECT(this);
    _pList->InsertBefore(_pNode, pNewNode);
}


//-----------------------------------------------------------------------------
//
// TIntrusiveDblLinkedList
//
// See comments at the top of this file
//

template <class T>
class TIntrusiveDblLinkedList : public CDblLinkedListBase
{
    typedef CDblLinkedListBase super;
public:
    TIntrusiveDblLinkedList() {}
    ~TIntrusiveDblLinkedList() {}

    void  Prepend(T *pt)          {            super::Prepend(pt); }
    void  Append(T *pt)           {            super::Append(pt); }
    T    *FindRef(const T*) const;
    T    *FindRef(BOOL (*pfnbEqual)(const T*, const T*), const T*) const;
    T    *First()           const { return (T*)super::First(); }
    T    *Last()            const { return (T*)super::Last(); }
    T    *Prev(const T *pt) const { return (T*)super::Prev(pt); }
    T    *Next(const T *pt) const { return (T*)super::Next(pt); }
    BOOL  IsEmpty()         const { return     super::IsEmpty(); }
    void  Remove(T *pt)           {            super::Remove(pt); }
private:
    NO_COPY(TIntrusiveDblLinkedList);
};

#ifndef WIN16
template <class T>
T *
TIntrusiveDblLinkedList<T>::FindRef(const T *pt) const
{
    for (T *pCur = First(); pCur && !(*pCur == *pt); pCur = Next(pCur));
    return pCur;
}
#endif // !WIN16

template <class T>
T *
TIntrusiveDblLinkedList<T>::FindRef(BOOL (*pfnbEqual)(const T*, const T*),
                                    const T *pt ) const
{
    for (T *pCur = First();
         pCur && !(*pfnbEqual)(pCur, pt);
         pCur = Next(pCur) );
    return pCur;
}


//-----------------------------------------------------------------------------
//
// TIntrusiveDblLinkedListOfStruct              IEUNIX
//
// This is an exact copy of TIntrusiveDblLinkedList except for the removal
// of FindRef(const T*).  The Unix compiler can not handle == of structs
// and one must use the other FindRef method.
//

template <class T>
class TIntrusiveDblLinkedListOfStruct : public CDblLinkedListBase
{
    typedef CDblLinkedListBase super;
public:
    TIntrusiveDblLinkedListOfStruct() {}
    ~TIntrusiveDblLinkedListOfStruct() {}

    void  Prepend(T *pt)          {            super::Prepend(pt); }
    void  Append(T *pt)           {            super::Append(pt); }
    T    *FindRef(BOOL (*pfnbEqual)(const T*, const T*), const T*) const;
    T    *First()           const { return (T*)super::First(); }
    T    *Last()            const { return (T*)super::Last(); }
    T    *Prev(const T *pt) const { return (T*)super::Prev(pt); }
    T    *Next(const T *pt) const { return (T*)super::Next(pt); }
    BOOL  IsEmpty()         const { return     super::IsEmpty(); }
    void  Remove(T *pt)           {            super::Remove(pt); }
private:
    NO_COPY(TIntrusiveDblLinkedListOfStruct);
};


template <class T>
T *
TIntrusiveDblLinkedListOfStruct<T>::FindRef(BOOL (*pfnbEqual)(const T*, const T*),
                                            const T *pt ) const
{
    for (T *pCur = First();
         pCur && !(*pfnbEqual)(pCur, pt);
         pCur = Next(pCur) );
    return pCur;
}


//-----------------------------------------------------------------------------
//
// TIntrusiveDblLinkedListIterator
//
// See comments at the top of this file
//

template <class T>
class TIntrusiveDblLinkedListIterator : public CDblLinkedListBaseIterator
{
    typedef CDblLinkedListBaseIterator super;
public:
    TIntrusiveDblLinkedListIterator() {}
    TIntrusiveDblLinkedListIterator(TIntrusiveDblLinkedList<T> &rList) :
        super(rList) {}
    TIntrusiveDblLinkedListIterator(TIntrusiveDblLinkedList<T> &rList,
                                    T *pNode ) : super(rList, pNode) {}
    ~TIntrusiveDblLinkedListIterator() {}

    void  Reset()                {            super::Reset(); }
    void  Reset(T *pNode)        {            super::Reset(pNode); }
    void  Reset(TIntrusiveDblLinkedList<T> &rList);
    void  Reset(TIntrusiveDblLinkedList<T> &rList, T *pNode);

    T    *Prev()                 { return (T*)super::Prev(); }
    T    *Next()                 { return (T*)super::Next(); }
    T    *operator()()           { return (T*)super::operator()(); }
    T    *Key() const            { return (T*)super::Key(); }

    T    *Remove()               { return (T*)super::Remove(); }
    void  InsertAfter(T *pNode)  {            super::InsertAfter(pNode); }
    void  InsertBefore(T *pNode) {            super::InsertBefore(pNode); }
};


template <class T>
inline void
TIntrusiveDblLinkedListIterator<T>::Reset(TIntrusiveDblLinkedList<T> &rList)
{
    super::Reset(rList);
}


template <class T>
inline void
TIntrusiveDblLinkedListIterator<T>::Reset(TIntrusiveDblLinkedList<T> &rList,
                                          T *pNode )
{
    super::Reset(rList, pNode);
}



#pragma INCMSG("--- End 'dbllink.hxx'")
#else
#pragma INCMSG("*** Dup 'dbllink.hxx'")
#endif
