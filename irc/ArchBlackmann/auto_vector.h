// auto_vector.h
// This file is (C) 2002-2004 Royce Mitchell III
// and released under the LGPL & BSD licenses

#ifndef AUTO_VECTOR_H
#define AUTO_VECTOR_H

#include <sys/types.h>
#include "verify.h"
#include "auto_ptr.h"

template<class T>
class auto_vector
{
public:
	explicit auto_vector ( size_t capacity = 0 )
		: _arr(0), _capacity(0), _end(0)
	{
		if ( capacity != 0 )
			_arr = new auto_ptr<T>[capacity];
		_capacity = capacity;
	}

	~auto_vector()
	{
		delete []_arr;
	}

	size_t size() const
	{
		return _end;
	}

	const auto_ptr<T>& operator [] ( size_t i ) const
	{
		ASSERT ( i < _end );
		return _arr[i];
	}

	auto_ptr<T>& operator [] ( size_t i )
	{
		ASSERT ( i < _end );
		return _arr[i];
	}

	void assign ( size_t i, auto_ptr<T>& p )
	{
		ASSERT ( i < _end );
		_arr[i] = p;
	}

	void assign_direct ( size_t i, T * p )
	{
		ASSERT ( i < _end );
		reserve ( i + 1 );
		_arr[i].reset ( p );
	}

	void push_back ( auto_ptr<T>& p )
	{
		reserve ( _end + 1 );
		_arr[_end++] = p;
	}

	auto_ptr<T>& back()
	{
		ASSERT ( _end != 0 );
		return _arr[_end-1];
	}

	void push_back ( T * p )
	{
		reserve ( _end + 1 );
		auto_ptr<T> tmp(p);
		_arr[_end++] = tmp;
		//GCC is pedantic, this is an error.
		//_arr[_end++] = auto_ptr<T>(p);
	}

	auto_ptr<T> pop_back()
	{
		ASSERT ( _end != 0 );
		if ( !_end )
		{
			auto_ptr<T> tmp((T*)0);
			return tmp;
			//GCC, this is an error.
			//return auto_ptr<T>(NULL);
		}
		return _arr[--_end];
	}

	void resize ( size_t newSize )
	{
		ASSERT ( newSize >= 0 );
		reserve ( newSize ); // make sure we have at least this much room
		_end = newSize;
	}

	void reserve ( size_t reqCapacity )
	{
		if ( reqCapacity <= _capacity )
			return;
		size_t newCapacity = 2 * _capacity;
		if ( reqCapacity > newCapacity )
			newCapacity = reqCapacity;
		// allocate new array
		auto_ptr<T> * arrNew = new auto_ptr<T> [newCapacity];
		// transfer all entries
		for ( size_t i = 0; i < _capacity; ++i )
			arrNew[i] = _arr[i];
		_capacity = newCapacity;
		// free old memory
		delete[] _arr;
		// substitute new array for old array
		_arr = arrNew;
	}

	void remove ( size_t off )
	{
		size_t last = _end-1;
		if ( off == last )
			resize ( last );
		else if ( off < last )
		{
			auto_ptr<T> tmp ( pop_back().release() );
			_arr[off] = tmp;
		}
	}

	//typedef const_auto_iterator<T> const_iterator;
	//const_iterator begin () const { return _arr; }
	//const_iterator end () const { return _arr + _end; }

private:
	auto_ptr<T>  *_arr;
	size_t        _capacity;
	size_t        _end;
};

#endif//AUTO_VECTOR_H
