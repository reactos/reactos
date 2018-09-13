#ifndef __ARRAY_INCLUDED__
#define __ARRAY_INCLUDED__

#ifndef __SHARED_INCLUDED__
#include <shared.h>
#endif

#ifndef __BUFFER_INCLUDED__
#include <buffer_t.h>
#endif

template <class T> inline void swap(T& t1, T& t2) {
	T t = t1;
	t1 = t2;
	t2 = t;
}

#ifndef self
#define self (*this)
#endif

template <class T> class Array {
	T* rgt;
	unsigned itMac;
	unsigned itMax;
	enum { itMaxMax = (1<<29) };
public:
	Array() {
		rgt = 0;
		itMac = itMax = 0;
	}
	Array(unsigned itMac_) {
		rgt = (itMac_ > 0) ? new T[itMac_] : 0;
		itMac = itMax = rgt ? itMac_ : 0;
	}
	~Array() {
		if (rgt)
			delete [] rgt;
	}
	Array& operator=(const Array& a) {
		if (&a != this) {
			if (a.itMac > itMax) {
				if (rgt)
					delete [] rgt;
				itMax = a.itMac;
				if (!(rgt = new T[itMax])) {
					// REVIEW. No good way to indicate failure.
					itMac = itMax = 0;
					return *this;
				}
			}
			itMac = a.itMac;
			for (unsigned it = 0; it < itMac; it++)
				rgt[it] = a.rgt[it];
		}
		return *this;
	}
	BOOL isValidSubscript(unsigned it) const {
		return 0 <= it && it < itMac;
	}
	unsigned size() const {
		return itMac;
	}
	T* pEnd() const {
		return &rgt[itMac];
	}
	BOOL getAt(unsigned it, T** ppt) const {
		if (isValidSubscript(it)) {
			*ppt = &rgt[it];
			return TRUE;
		}
		else
			return FALSE;
	}
	BOOL putAt(unsigned it, const T& t) {
		if (isValidSubscript(it)) {
			rgt[it] = t;
			return TRUE;
		}
		else
			return FALSE;
	}
	T& operator[](unsigned it) const {
		precondition(isValidSubscript(it));
		return rgt[it];
	}
    BOOL append(const T& t) {
		if (setSize(size() + 1)) {
			self[size() - 1] = t;
			return TRUE;
		} else
			return FALSE;
	}
	void swap(Array& a) {
		::swap(rgt,   a.rgt);
		::swap(itMac, a.itMac);
		::swap(itMax, a.itMax);
	}
	void reset() {
		setSize(0);
	}
	void fill(const T& t) {
		for (unsigned it = 0; it < size(); it++)
			self[it] = t;
	}
	BOOL insertAt(unsigned itInsert, const T& t);
	void deleteAt(unsigned it);
	void deleteRunAt(unsigned it, int ct);
	BOOL setSize(unsigned itMacNew);
	BOOL findFirstEltSuchThat(BOOL (*pfn)(T*, void*), void* pArg, unsigned *pit) const;
	BOOL findFirstEltSuchThat_Rover(BOOL (*pfn)(T*, void*), void* pArg, unsigned *pit) const;
	unsigned binarySearch(BOOL (*pfnLE)(T*, void*), void* pArg) const;
	BOOL save(Buffer* pbuf) const;
	BOOL reload(PB* ppb);
private:
	Array(const Array&); // not implemented
};

template <class T> inline BOOL Array<T>::insertAt(unsigned it, const T& t) {
	precondition(isValidSubscript(it) || it == size());

	if (setSize(size() + 1)) {
		memmove(&rgt[it + 1], &rgt[it], (size() - (it + 1)) * sizeof(T));
		rgt[it] = t;
		return TRUE;
	}
	else
		return FALSE;
}

template <class T> inline void Array<T>::deleteAt(unsigned it) {
	precondition(isValidSubscript(it));

 	memmove(&rgt[it], &rgt[it + 1], (size() - (it + 1)) * sizeof(T));
	rgt[size() - 1] = T();
	setSize(size() - 1);
}

template <class T> inline void Array<T>::deleteRunAt(unsigned it, int ct) {
	unsigned itMacNew = it + ct;
	precondition(isValidSubscript(it) && isValidSubscript(itMacNew - 1));

 	memmove(&rgt[it], &rgt[itMacNew], (size() - itMacNew) * sizeof(T));
	for ( ; it < itMacNew; it++)
		rgt[it] = T();
	setSize(size() - ct);
}

// Grow the array to a new size.
template <class T> inline
BOOL Array<T>::setSize(unsigned itMacNew) {
	precondition(0 <= itMacNew && itMacNew <= itMaxMax);

	if (itMacNew > itMax) {
		// Ensure growth is by at least 50% of former size.
		unsigned itMaxNew = max(itMacNew, 3*itMax/2);
 		assert(itMaxNew <= itMaxMax);

		T* rgtNew = new T[itMaxNew];
		if (!rgtNew)
			return FALSE;
		if (rgt) {
			for (unsigned it = 0; it < itMac; it++)
				rgtNew[it] = rgt[it];
			delete [] rgt;
		}
		rgt = rgtNew;
		itMax = itMaxNew;
	}
	itMac = itMacNew;
	return TRUE;
}

template <class T> inline
BOOL Array<T>::save(Buffer* pbuf) const {
	return pbuf->Append((PB)&itMac, sizeof itMac) &&
		   (itMac == 0 || pbuf->Append((PB)rgt, itMac*sizeof(T)));
}

template <class T> inline
BOOL Array<T>::reload(PB* ppb) {
	unsigned itMacNew = *((unsigned UNALIGNED *&)*ppb)++;
	if (!setSize(itMacNew))
		return FALSE;
	memcpy(rgt, *ppb, itMac*sizeof(T));
	*ppb += itMac*sizeof(T);
	return TRUE;
}

template <class T> inline
BOOL Array<T>::findFirstEltSuchThat(BOOL (*pfn)(T*, void*), void* pArg, unsigned *pit) const
{
	for (unsigned it = 0; it < size(); ++it) {
		if ((*pfn)(&rgt[it], pArg)) {
			*pit = it;
			return TRUE;
		}
	}
	return FALSE;
}

template <class T> inline
BOOL Array<T>::findFirstEltSuchThat_Rover(BOOL (*pfn)(T*, void*), void* pArg, unsigned *pit) const
{
	precondition(pit);

	if (!(0 <= *pit && *pit < size()))
		*pit = 0;

	for (unsigned it = *pit; it < size(); ++it) {
		if ((*pfn)(&rgt[it], pArg)) {
			*pit = it;
			return TRUE;
		}
	}

	for (it = 0; it < *pit; ++it) {
		if ((*pfn)(&rgt[it], pArg)) {
			*pit = it;
			return TRUE;
		}
	}

	return FALSE;
}

template <class T> inline
unsigned Array<T>::binarySearch(BOOL (*pfnLE)(T*, void*), void* pArg) const
{
	unsigned itLo = 0;
	unsigned itHi = size(); 
	while (itLo < itHi) {
		// (low + high) / 2 might overflow
		unsigned itMid = itLo + (itHi - itLo) / 2;
		if ((*pfnLE)(&rgt[itMid], pArg))
			itHi = itMid;
		else
			itLo = itMid + 1;
	}
	postcondition(itLo == 0      || !(*pfnLE)(&rgt[itLo - 1], pArg));
	postcondition(itLo == size() ||  (*pfnLE)(&rgt[itLo], pArg));
	return itLo;
}

#endif // !__ARRAY_INCLUDED__
