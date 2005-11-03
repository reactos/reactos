/*
** Author: Samuel R. Blackburn
** Internet: wfc@pobox.com
**
** You can use it any way you like as long as you don't try to sell it.
**
** Any attempt to sell WFC in source code form must have the permission
** of the original author. You can produce commercial executables with
** WFC but you can't sell WFC.
**
** Copyright, 2000, Samuel R. Blackburn
**
** NOTE: I modified the info block below so it hopefully wouldn't conflict
** with the original file. Royce Mitchell III
*/

#ifndef QUEUET_CLASS_HEADER
#define QUEUET_CLASS_HEADER

#include "ReliMT.h"

#ifdef WIN32
#include <sys/types.h> // off_t
#define HEAPCREATE(size) m_Heap = ::HeapCreate ( HEAP_NO_SERIALIZE, size, 0 )
#define HEAPALLOC(size) ::HeapAlloc ( m_Heap, HEAP_NO_SERIALIZE, size )
#define HEAPREALLOC(p,size) ::HeapReAlloc( m_Heap, HEAP_NO_SERIALIZE, p, size )
#define HEAPFREE(p) ::HeapFree ( m_Heap, HEAP_NO_SERIALIZE, p )
#define HEAPDESTROY() ::HeapDestroy ( m_Heap ); m_Heap = 0;
#else
#define HEAPCREATE(size)
#define HEAPALLOC(size) malloc(size)
#define HEAPREALLOC(p,size) realloc(p,size);
#define HEAPFREE(p) free(p)
#define HEAPDESTROY()
#endif

template <class T>
class CQueueT : public Uncopyable
{
protected:

	// What we want to protect

	Mutex m_AddMutex;
	Mutex m_GetMutex;

	T* m_Items;

	off_t m_AddIndex;
	off_t m_GetIndex;
	size_t m_Size;

#ifdef WIN32
	HANDLE m_Heap;
#endif//WIN32
	inline void m_GrowBy ( size_t number_of_new_items );

public:

	inline  CQueueT ( size_t initial_size = 1024 );
	inline ~CQueueT();

	inline bool  Add( const T& new_item );
	inline void  Empty() { m_AddIndex = 0; m_GetIndex = 0; };
	inline bool  Get( T& item );
	inline size_t GetLength() const;
	inline size_t GetMaximumLength() const { return( m_Size ); };
	inline bool  AddArray ( const T* new_items, int item_count );
	inline int GetArray ( T* items, const int maxget, const T& tEnd );
	inline bool Contains ( const T& t );
};

template <class T>
inline CQueueT<T>::CQueueT ( size_t initial_size )
{
	m_AddIndex = 0;
	m_GetIndex = 0;
	m_Items    = NULL;

	if ( initial_size == 0 )
		initial_size = 1;

	/*
	** 1999-11-05
	** We create our own heap because all of the pointers used are allocated
	** and freed be us. We don't have to worry about a non-serialized thread
	** accessing something we allocated. Because of this, we can perform our
	** memory allocations in a heap dedicated to queueing. This means when we
	** have to allocate more memory, we don't have to wait for all other threads
	** to pause while we allocate from the shared heap (like the C Runtime heap)
	*/

	HEAPCREATE( ( ( ( 2 * initial_size * sizeof(T) ) < 65536 ) ? 65536 : (2 * initial_size * sizeof(T) ) ) );

	m_Items = (T*)HEAPALLOC ( initial_size * sizeof(T) );

	m_Size = ( m_Items == NULL ) ? 0 : initial_size;
}

template <class T>
inline CQueueT<T>::~CQueueT()
{
	m_AddIndex = 0;
	m_GetIndex = 0;
	m_Size     = 0;

	if ( m_Items != NULL )
	{
		HEAPFREE(m_Items);
		m_Items = NULL;
	}

	HEAPDESTROY();
}

template <class T>
inline bool CQueueT<T>::Add ( const T& item )
{
	// Block other threads from entering Add();
	Mutex::Lock addlock ( m_AddMutex );

	// Add the item

	m_Items[ m_AddIndex ] = item;

	// 1999-12-08
	// Many many thanks go to Lou Franco (lfranco@spheresoft.com)
	// for finding an bug here. It rare but recreatable situations,
	// m_AddIndex could be in an invalid state.

	// Make sure m_AddIndex is never invalid

	off_t new_add_index = ( ( m_AddIndex + 1 ) >= (off_t)m_Size ) ? 0 : m_AddIndex + 1;

	if ( new_add_index == m_GetIndex )
	{
		// The queue is full. We need to grow.
		// Stop anyone from getting from the queue
		Mutex::Lock getlock ( m_GetMutex );

		m_AddIndex = new_add_index;

		// One last double-check.

		if ( m_AddIndex == m_GetIndex )
		{
			m_GrowBy ( m_Size );
		}

	}
	else
	{
		m_AddIndex = new_add_index;
	}

	return true;
}

template <class T>
inline bool CQueueT<T>::Get( T& item )
{
	// Prevent other threads from entering Get()
	Mutex::Lock getlock ( m_GetMutex );

	if ( m_GetIndex == m_AddIndex )
	{
		// Let's check to see if our queue has grown too big
		// If it has, then shrink it

		if ( m_Size > 1024 )
		{
			// Yup, we're too big for our britches
			Mutex::TryLock addlock ( m_AddMutex );
			if ( addlock )
			{
				// Now, no one can add to the queue

				if ( m_GetIndex == m_AddIndex ) // is queue empty?
				{
					// See if we can just shrink it...
					T* return_value = (T*)HEAPREALLOC(m_Items,1024 * sizeof(T));

					if ( return_value != NULL )
					{
						m_Items = (T*) return_value;
					}
					else
					{
						// Looks like we'll have to do it the hard way
						HEAPFREE ( m_Items );
						m_Items = (T*) HEAPALLOC ( 1024 * sizeof(T) );
					}

					m_Size     = ( m_Items == NULL ) ? 0 : 1024;
					m_AddIndex = 0;
					m_GetIndex = 0;
				}
				else
				{
					// m_GetIndex != m_AddIndex, this means that someone added
					// to the queue between the time we checked m_Size for being
					// too big and the time we entered the add critical section.
					// If this happened then we are too busy to shrink
				}
			}
		}
		return false;
	}

	item = m_Items[ m_GetIndex ];

	// Make sure m_GetIndex is never invalid

	m_GetIndex = ( ( m_GetIndex + 1 ) >= (off_t)m_Size ) ? 0 : m_GetIndex + 1;

	return true;
}

template <class T>
inline int CQueueT<T>::GetArray ( T* items, const int maxget, const T& tEnd )
{
	// TODO - oooh baby does this need to be optimized
	// Prevent other threads from entering Get()
	Mutex::Lock getlock ( m_GetMutex ); //::EnterCriticalSection( &m_GetCriticalSection );

	int iResult = 0;
	for ( int i = 0; i < maxget; i++ )
	{
		if ( !Get(items[i]) )
			break;
		iResult++;
		if ( items[i] == tEnd )
			break;
	}
	// Let other threads call Get() now
	//::LeaveCriticalSection( &m_GetCriticalSection );
	return iResult;
}

template <class T>
inline size_t CQueueT<T>::GetLength() const
{
	// This is a very expensive process!
	// No one can call Add() or Get() while we're computing this

	size_t number_of_items_in_the_queue = 0;

	Mutex::Lock addlock ( m_AddMutex );
	Mutex::Lock getlock ( m_GetMutex );

	number_of_items_in_the_queue = ( m_AddIndex >= m_GetIndex ) ?
	                               ( m_AddIndex  - m_GetIndex ) :
	                               ( ( m_AddIndex + m_Size ) - m_GetIndex );

	return number_of_items_in_the_queue;
}

template <class T>
inline void CQueueT<T>::m_GrowBy ( size_t number_of_new_items )
{
	// Prevent other threads from calling Get().
	// We don't need to enter the AddCriticalSection because
	// m_GrowBy() is only called from Add();

	T* new_array       = NULL;
	T* pointer_to_free = NULL;

	size_t new_size = m_Size + number_of_new_items;

	{ // Prevent other threads from getting
		Mutex::Lock getlock ( m_GetMutex );

		// 2000-05-16
		// Thanks go to Royce Mitchell III (royce3@aim-controls.com) for finding
		// a HUGE bug here. I was using HeapReAlloc as a short cut but my logic
		// was flawed. In certain circumstances, queue items were being dropped.

		new_array = (T*)HEAPALLOC ( new_size * sizeof(T) );

		// Now copy all of the old items from the old queue to the new one.

		// Get the entries from the get-index to the end of the array
		memcpy ( new_array, &m_Items[m_GetIndex], ( m_Size - m_GetIndex ) * sizeof(T) );

		// Get the entries from the beginning of the array to the add-index
		memcpy ( &new_array[m_Size-m_GetIndex], m_Items, m_AddIndex * sizeof(T) );

		m_AddIndex      = (off_t)m_Size;
		m_GetIndex      = 0;
		m_Size          = new_size;
		pointer_to_free = m_Items;
		m_Items         = new_array;
	} // Mutex::Lock
	HEAPFREE ( pointer_to_free );
}

template <class T>
inline bool CQueueT<T>::Contains ( const T& t )
{
	Mutex::Lock addlock ( m_AddMutex );
	Mutex::Lock getlock ( m_GetMutex );

	for ( int i = m_GetIndex; i != m_AddIndex; i++ )
	{
		if ( i == m_Size )
			i = 0;
		if ( m_Items[i] == t )
			return true;
	}
	return m_Items[m_AddIndex] == t;
}

typedef CQueueT<char> CCharQueue;

#endif // QUEUE_CLASS_HEADER
