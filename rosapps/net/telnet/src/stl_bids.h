// This is the STL wrapper for classlib/arrays.h from Borland's web site
// It has been modified to be compatible with vc++ (Paul Branann 5/7/98)

#ifndef STL_ARRAY_AS_VECTOR
#define STL_ARRAY_AS_VECTOR

#ifdef _MSC_VER
#pragma warning(disable: 4786)
#endif

// #include <vector.h>
// #include <algo.h>
#include <vector>
#include <algorithm>
using namespace std;

template <class T>
class TArrayAsVector : public vector<T> {
private:
	const unsigned int growable;
	const size_type lowerbound;
public:
	TArrayAsVector(size_type upper,
		size_type lower = 0,
		int delta = 0) :
	vector<T>( ),
		growable(delta),
		lowerbound(lower)
	{ reserve(upper-lower + 1);}
	
	~TArrayAsVector( )
	{ // This call is unnecessary?  (Paul Brannan 5/7/98)
		// vector<T>::~vector( );
	}
	
	int Add(const T& item)
	{ if(!growable && size( ) == capacity( ))
	return 0;
	else
		insert(end( ), item);
	return 1; }
	
	int AddAt(const T& item, size_type index)
	{ if(!growable &&
	((size( ) == capacity( )) ||
	(ZeroBase(index > capacity( )) )))
	return 0;
	if(ZeroBase(index) > capacity( )) // out of bounds
	{ insert(end( ), 
	ZeroBase(index) - size( ), T( ));
	insert(end( ), item); }
	else
	{ insert(begin( ) + ZeroBase(index), item); }
	return 1;
	}
	
	size_type ArraySize( )
	{ return capacity( ); }
	
	size_type BoundBase(size_type location) const
	{ if(location == UINT_MAX)
	return INT_MAX;
	else
		return location + lowerbound; }
	void Detach(size_type index)
	{ erase(begin( ) + ZeroBase(index)); }
	
	void Detach(const T& item)
	{ Destroy(Find(item)); }
	
	void Destroy(size_type index)
	{ erase(begin( ) + ZeroBase(index)); }
	
	void Destroy(const T& item)
	{ Destroy(Find(item)); }
	
	size_type Find(const T& item) const
	{ const_iterator location = find(begin( ), 
	end( ), item);
	if(location != end( ))
		return BoundBase(size_type(location - 
		begin( )));
	else
		return INT_MAX; }
	
	size_type GetItemsInContainer( )
	{ return size( ); }
	
	void Grow(size_type index)
	{ if( index < lowerbound )
	Reallocate(ArraySize( ) + (index - 
	lowerbound));
	else if( index >= BoundBase(size( )))
		Reallocate(ZeroBase(index) ); }
	
	int HasMember(const T& item)
	{ if(Find(item) != INT_MAX)
	return 1;
	else
		return 0; }
	
	int IsEmpty( )
	{ return empty( ); }
	
	int IsFull( )
	{ if(growable)
	return 0;
	if(size( ) == capacity( ))
		return 1;
	else
		return 0; }
	
	size_type LowerBound( )
	{ return lowerbound; }
	
	T& operator[] (size_type index)
	{ return vector<T>::
	operator[](ZeroBase(index)); }
	
	const T& operator[] (size_type index) const
	{ return vector<T>::
	operator[](ZeroBase(index)); }
	
	void Flush( )
	{
		// changed this to get it to work with VC++
		// (Paul Brannan 5/7/98)
#ifdef _MSC_VER
		_Destroy(begin(), end());
		_Last = _First;
#else
		destroy(begin( ), end( ));
//#ifdef __CYGWIN__
//		_M_finish = _M_start;
//#else
//		finish = start;
//#endif
#endif
	}
	
	void Reallocate(size_type sz, 
		size_type offset = 0)
	{ if(offset)
	insert(begin( ), offset, T( ));
	reserve(sz);
	erase(end( ) - offset, end( )); }
	
	void RemoveEntry(size_type index)
	{ Detach(index); }
	
	void SetData(size_type index, const T& item)
	{ (*this)[index] = item; }
	
	size_type UpperBound( )
	{ return BoundBase(capacity( )) - 1; }
	
	size_type ZeroBase(size_type index) const
	{ return index - lowerbound; }
	
	// The assignment operator is not inherited (Paul Brannan 5/25/98)
	TArrayAsVector& operator=(const TArrayAsVector& v) {
		vector<T>::operator=(v);
		// should growable and lowerbound be copied as well?
		return *this;
	}

};

#endif