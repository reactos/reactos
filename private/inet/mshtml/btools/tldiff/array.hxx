#ifndef __ARRAY_HXX_INCLUDED__
#define __ARRAY_HXX_INCLUDED__

#pragma warning(disable:4710)       // inline fn not expanded (retail warning)

template <class T> class CAutoArray
{
public:
    CAutoArray();
    virtual ~CAutoArray();
    int Size() { return _iHighestItemNum; }
    bool EnsureSize(int minSize);

    bool GetAt( int index, T* pRet );
    bool Set( int index, T& val );
    bool Append( T& val );

    void Empty();

private:
    T *_pArray;
    int _iArraySize;
    int _iHighestItemNum;

    enum { ARRAY_GROWTH_GRANULARITY = 4 };
};

template <class T>
CAutoArray<T>::CAutoArray() :
 _pArray( NULL ),
 _iArraySize( 0 ),
 _iHighestItemNum( 0 )
{
    
}

template <class T>
CAutoArray<T>::~CAutoArray()
{
    if ( _pArray ) {
        delete [] _pArray;
    }
}

template <class T>
bool CAutoArray<T>::EnsureSize(int minSize)
{
    assert( minSize > 0 );
    int i;
    if ( minSize > _iArraySize ) {
        T* pNewArray = new T[minSize+ARRAY_GROWTH_GRANULARITY];
        if ( pNewArray ) {
            if ( _pArray ) {
                for (i=0 ; i < _iHighestItemNum ; ++i) {
                    pNewArray[i] = _pArray[i];
                }
                delete [] _pArray;
            }
            _pArray = pNewArray;
            _iArraySize = minSize+ARRAY_GROWTH_GRANULARITY;
            return true;    // new array alloc'ed/copied
        }
        return false;   // failed to create new array
    }
    return true;
}

template <class T>
bool CAutoArray<T>::GetAt( int index, T* pRet )
{
    // BUGBUG: Do an EnsureSize here?
    if ( index < _iArraySize ) {
        *pRet = _pArray[index];
        return true;
    }
    return false;
}

template <class T>
bool CAutoArray<T>::Set( int index, T& val )
{
    // BUGBUG: Do an EnsureSize here?
    if ( index < _iArraySize ) {
        _pArray[index] = val;
        if ( index+1 > _iHighestItemNum ) {
            _iHighestItemNum = index+1;
        }
        return true;
    }
    return false;
}


template <class T>
bool CAutoArray<T>::Append( T& val )
{
    unsigned int iIndex = Size();

    if (EnsureSize(iIndex + 1))
    {
        return Set(iIndex, val);
    }

    return false;
}


template <class T>
void CAutoArray<T>::Empty()
{
    if ( _pArray ) {
        delete [] _pArray;
    }

    _pArray = NULL;
    _iHighestItemNum = 0;
}


#endif // __ARRAY_HXX_INCLUDED__

