#ifndef _SORTLIST_HXX_
#define _SORTLIST_HXX_

template<class KEY, class TYPE, class ARG_TYPE>
class CSortList
{
protected:
    struct NEW_ARG
    {
        KEY        key;
        TYPE       type;
    };

public:
//private:

    TYPE RemoveTail()
    {
        CLock lck(_mxs);
        NotfAssert(( _cElements && (_cElements ==  _XList.GetCount()) ));

        _cElements--;
        return _XList.RemoveTail().type;
    }

    POSITION GetTailPosition()
    {
        CLock lck(_mxs);
        return _XList.GetTailPosition();
    }
    TYPE& GetPrev(POSITION& rPosition) // return *Position--
    {
        CLock lck(_mxs);
        return _XList.GetPrev(rPosition).type;
    }
    TYPE GetPrev(POSITION& rPosition) const // return *Position--
    {
        return _XList.GetPrev(rPosition).type;
    }
    TYPE& GetAt(POSITION position)
    {
        return _XList.GetAt(position).type;
    }
    TYPE GetAt(POSITION position) const
    {
        return _XList.GetAt(position).type;
    }

// new public methods
public:
    TYPE RemoveHead()
    {
        CLock lck(_mxs);
        NotfAssert(( _cElements && (_cElements ==  _XList.GetCount()) ));

        _cElements--;
        return _XList.RemoveHead().type;
    }

    void RemoveAt(POSITION position)
    {
        CLock lck(_mxs);
        NotfAssert(( _cElements && (_cElements ==  _XList.GetCount()) ));
        NotfAssert((position));

        _cElements--;
        _XList.RemoveAt(position);
    }
    void RemoveAll()
    {
        CLock lck(_mxs);
        //_cElements.Set(0);
        _XList.RemoveAll();
        while (_cElements)
        {
          _cElements--;  
        }
    }


    // iteration
    POSITION GetHeadPosition()
    {
        CLock lck(_mxs);
        return _XList.GetHeadPosition();
    }

    int GetCount()
    {
        CLock lck(_mxs);
        NotfAssert(( _cElements ==  _XList.GetCount() ));
        return _XList.GetCount();
    }

    BOOL IsEmpty()
    {
        CLock lck(_mxs);
        NotfAssert(( _cElements ==  _XList.GetCount() ));
        return _XList.IsEmpty();
    }
    /*
    TYPE GetFirst() const
    {
        POSITION pos = _XList.GetHeadPosition();
        return (pos) ? _XList.GetAt(pos).type : NULL;
    }
    */
    

    TYPE& GetNext(POSITION& rPosition) // return *Position++
    {
        CLock lck(_mxs);
        return _XList.GetNext(rPosition).type;
    }
    TYPE GetNext(POSITION& rPosition) const // return *Position++
    {
        return _XList.GetNext(rPosition).type;
    }


    STDMETHODIMP AddVal(KEY key, ARG_TYPE newValue)
    {
        NotfDebugOut((DEB_SCHEDLST, "%p _IN CSortList::AddVal\n", this));
        HRESULT hr = E_FAIL;
        CLock lck(_mxs);

        NEW_ARG newElement;
        newElement.key = key;
        newElement.type = newValue;

        //
        // start at the tail and loop forward
        //
        POSITION pos = _XList.GetTailPosition();

        if (pos)
        {
            NEW_ARG arg = _XList.GetAt(pos);

            while (pos)
            {
                // check if current position is smaller if so
                if (arg.key < key)
                {
                    // found the place
                    POSITION posInsert = _XList.InsertAfter(pos, newElement);
                    _cElements++;
                    pos = 0;
                    hr = NOERROR;
                }
                else
                {
                    arg = _XList.GetPrev(pos); // return *Position--
                }
            }
        }

        // did not find the place
        // add the element at the front
        //
        if ((hr != NOERROR) && !pos)
        {
            pos = _XList.AddHead(newElement);
            _cElements++;
            hr = NOERROR;
        }

        NotfDebugOut((DEB_SCHEDLST, "%p OUT CSortList::AddVal (hr:%lx)\n",this, hr));
        return hr;
    }

    STDMETHODIMP FindFirst (KEY& rkey, TYPE& rValue, POSITION& rPos)
    {
        NotfDebugOut((DEB_SCHEDLST, "%p _IN CSortList::FindFirst\n", this));
        HRESULT hr = E_FAIL;
        CLock lck(_mxs);

        if (_cElements)
        {
            HRESULT hr = E_FAIL;

            NEW_ARG newElement;
            newElement.key = rkey;
            newElement.type = rValue;

            // start at the head
            POSITION pos = _XList.GetHeadPosition();
            NotfAssert((pos));
            NEW_ARG arg = _XList.GetAt(pos);

            while (pos)
            {
                if (arg.key >= rkey)
                {
                    // found the place
                    rValue = arg.type;
                    hr = NOERROR;
                    pos = 0;
                }
                else
                {
                    arg = _XList.GetNext(pos); // return *Position++
                }
            }
        }

        NotfDebugOut((DEB_SCHEDLST, "%p OUT CSortList::FindFirst (hr:%lx)\n",this, hr));
        return hr;
    }
/*
    // Lookup and add if not there
    TYPE& operator[](KEY& rKey)
    {
        TYPE *pType;
        HRESULT hr;
        POSITION pos;
        hr = FindFirst(rKey, *pType, pos);
        return *pType;
    }
*/
public:
    CSortList() : _cElements(0), _CRefs(1)
    {
    }

    ~CSortList()
    {
        NotfAssert((_cElements == 0));
    }

private:
    CRefCount       _CRefs;         // the total refcount of this object
    CRefCount       _cElements;     // # of elements
    CMutexSem       _mxs;           // single access to the list
    POSITION        _pos;

    CList<NEW_ARG, NEW_ARG &>      _XList;
};  // CSortList

#endif // _SORTLIST_HXX_

