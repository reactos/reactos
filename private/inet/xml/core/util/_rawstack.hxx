/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _RAWSTACK_HXX
#define _RAWSTACK_HXX

//===========================================================================
// This is a raw stack based on a contiguous block of memory that is divided
// up into chunks.
//
// This is a Non-GC class because it is used in the tokenizer.
//

class RawStack
{
public:
    RawStack(long entrySize, long growth);
    ~RawStack();

protected:
    inline char* _push() { if (_ncSize == _ncUsed) return __push(); return &_pStack[_lEntrySize * _ncUsed++]; }
    inline char* _pop() { if (_ncUsed > 0) _ncUsed--; return _peek(); }
    inline char* _peek() { if (_ncUsed == 0) return NULL; return &_pStack[_lEntrySize * (_ncUsed - 1)]; }
    inline char* _item(long index) { return &_pStack[_lEntrySize * index]; }

    long    _lEntrySize;    
    char*   _pStack;
    long    _ncUsed;
    long    _ncSize;
    long    _lGrowth;

private:
    char* __push();
};

//===========================================================================
// This class implements a raw stack of C primitive types (or structs).

template <class T> class _rawstack : public RawStack
{
public:        
        _rawstack<T>(long growth) : RawStack(sizeof(T),growth)
        { 
        }

        T* push()
        {
            return (T*)_push();
        }

        T* pop()
        {
            return (T*)_pop();
        }

        T* peek()
        {
            return (T*)_peek();
        }

        long size()
        {
            return _ncSize;
        }

        long used()
        {
            return _ncUsed;
        }

        T* operator[](long index)
        {
            return (T*)_item(index);
        }
};    

#endif _RAWSTACK_HXX