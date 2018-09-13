/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _ARRAY_HXX
#define _ARRAY_HXX

class DLLEXPORT NOVTABLE __array : public Base
{
    friend class System;
    friend class String;

    DECLARE_CLASS_MEMBERS(__array, Base);

public:        int length() const
            {
                return _length;
            }

protected:    virtual int getDataSize() = 0;

protected:    virtual void assign(void * d, void * s) = 0;

protected:    void * fetch(int at, int m);

protected:    static void indexError();

protected:    void copy(int offset, int count, const __array * src, int src_offset);

protected:    int    _length;

protected:    byte _data[1];

};

template <class T> class _array : public __array
{
friend class System;
friend class String;
friend class StringBuffer;

    public: void * __cdecl operator new(size_t cb, int length)
            {
                void * p = MemAllocObject(cb + sizeof(T) * length);
                ((_array *)p)->_length = length;
                return p;
            }

    public: void __cdecl operator delete(void * pv)
            {
                MemFree(pv);
            }

    protected:	virtual void assign(void * d, void * s)
			{
				*(T *)d = *(T *)s;
			}

    protected:    virtual void finalize()
            {
                T * p = (T *)_data;
                for (int i = _length; i--; p++)
                {
                    p->~T();
                }
                super::finalize();
            }

    private:    void error();
    
    public:     T & operator[] (int index) { return item(index); }
    public:     T & item(int index)
            {
                if (index < 0 || index >= _length)
                {
                    indexError();
                }
                return ((T *)_data)[index];
            }
    public:     const T & item(int index) const
            {
                if (index < 0 || index >= _length)
                {
                    indexError();
                }
                return ((T *)_data)[index];
            }

    protected:    virtual int getDataSize()
            {
                return sizeof(T);
            }

    public:       const T * getData() const
            {
                return (const T *)_data;
            }

    public:  void simpleCopy(int offset, int count, const T *src)
            {
                // Copy elements from src to this array.
                Assert(offset + count <= length()); 
                memcpy((void *) (getData() + offset), src, count * sizeof(T) );
            }

    public:  void simpleCopy(const _array<T> *src)
            {
                // Copy elements from src to this array.
                Assert(src->length() <= length()); 
                memcpy((void *) getData(), (void *) src->getData(), src->length() * sizeof(T) );
            }

    public: void copy(int offset, int count, const _array<T> * src, int src_offset)
            {
                // Copy elements from src to this array using assign
                Assert(offset + count <= length()); 
                __array::copy(offset, count, src, src_offset);
            }

    public: _array<T> * resize(int size)
            {
                _array<T> * a = new (size) _array<T>;
                a->copy(0, length(), this, 0);
                return a;
            }
};

typedef _array<byte> abyte;
typedef _array<char> achar;
typedef _array<unsigned short> aushort;
typedef _array<TCHAR> ATCHAR;
typedef _array<WCHAR> AWCHAR;
typedef _array<short> ashort;
typedef _array<int> aint;
typedef _array<unsigned int> auint;

typedef _array<int> aint;
typedef _reference<abyte> rabyte;
typedef _reference<achar> rachar;
typedef _reference<aushort> raushort;
typedef _reference<ATCHAR> RATCHAR;
typedef _reference<AWCHAR> RAWCHAR;
typedef _reference<ashort> rashort;
typedef _reference<aint> raint;
typedef _reference<auint> rauint;

#endif _ARRAY_HXX
