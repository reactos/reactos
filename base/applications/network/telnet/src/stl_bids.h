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
#include <limits.h>
using namespace std;

template <class T>
class TArrayAsVector : public vector<T> {
private:
	const unsigned int growable;
	typedef size_t size_type;
	typedef typename vector<T>::const_iterator const_iterator;
	const size_type lowerbound;
public:
	TArrayAsVector(size_type upper,
		size_type lower = 0,
		int delta = 0) :
	vector<T>( ),
		growable(delta),
		lowerbound(lower)
	{ vector<T>::reserve(upper-lower + 1);}

	~TArrayAsVector( )
	{ // This call is unnecessary?  (Paul Brannan 5/7/98)
		// vector<T>::~vector( );
	}

	int Add(const T& item)
	{ if(!growable && vector<T>::size( ) == vector<T>::capacity( ))
	return 0;
	else
		insert(vector<T>::end( ), item);
	return 1; }

	int AddAt(const T& item, size_type index)
	{ if(!growable &&
	((vector<T>::size( ) == vector<T>::capacity( )) ||
	(ZeroBase(index > vector<T>::capacity( )) )))
	return 0;
	if(ZeroBase(index) > vector<T>::capacity( )) // out of bounds
	{ insert(vector<T>::end( ),
	ZeroBase(index) - vector<T>::size( ), T( ));
	insert(vector<T>::end( ), item); }
	else
	{ insert(vector<T>::begin( ) + ZeroBase(index), item); }
	return 1;
	}

	size_type ArraySize( )
	{ return vector<T>::capacity( ); }

	size_type BoundBase(size_type location) const
	{ if(location == UINT_MAX)
	return INT_MAX;
	else
		return location + lowerbound; }
	void Detach(size_type index)
	{ erase(vector<T>::begin( ) + ZeroBase(index)); }

	void Detach(const T& item)
	{ Destroy(Find(item)); }

	void Destroy(size_type index)
	{ erase(vector<T>::begin( ) + ZeroBase(index)); }

	void Destroy(const T& item)
	{ Destroy(Find(item)); }

	size_type Find(const T& item) const
	{ const_iterator location = find(vector<T>::begin( ),
	vector<T>::end( ), item);
	if(location != vector<T>::end( ))
		return BoundBase(size_type(location -
		vector<T>::begin( )));
	else
		return INT_MAX; }

	size_type GetItemsInContainer( )
	{ return vector<T>::size( ); }

	void Grow(size_type index)
	{ if( index < lowerbound )
	Reallocate(ArraySize( ) + (index -
	lowerbound));
	else if( index >= BoundBase(vector<T>::size( )))
		Reallocate(ZeroBase(index) ); }

	int HasMember(const T& item)
	{ if(Find(item) != INT_MAX)
	return 1;
	else
		return 0; }

	int IsEmpty( )
	{ return vector<T>::empty( ); }

	int IsFull( )
	{ if(growable)
	return 0;
	if(vector<T>::size( ) == vector<T>::capacity( ))
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
		vector<T>::clear();
	}

	void Reallocate(size_type sz,
		size_type offset = 0)
	{ if(offset)
	insert(vector<T>::begin( ), offset, T( ));
	vector<T>::reserve(sz);
	erase(vector<T>::end( ) - offset, vector<T>::end( )); }

	void RemoveEntry(size_type index)
	{ Detach(index); }

	void SetData(size_type index, const T& item)
	{ (*this)[index] = item; }

	size_type UpperBound( )
	{ return BoundBase(vector<T>::capacity( )) - 1; }

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
