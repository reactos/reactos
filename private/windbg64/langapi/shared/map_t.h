#pragma once
#ifndef __MAP_INCLUDED__
#define __MAP_INCLUDED__

#ifndef __ARRAY_INCLUDED__
#include "array_t.h"
#endif
#ifndef __TWO_INCLUDED__
#include "two.h"
#endif
#ifndef __ISET_INCLUDED__
#include "iset_t.h"
#endif

#include "enum_t.h"

template <class H,int>
class HashClass {
public:
	inline HASH __fastcall operator()(H);
	};

template <class H,int>
class LHashClass {
public:
	inline LHASH __fastcall operator()(H);
	};

//
// standard version of the HashClass merely casts the object to a HASH.
//	by convention, this one is always HashClass<H,hcCast>
//
#define hcCast 0
// SIG is an unsigned long (like NI!) and needs a different hash function
#define hcSig 1
// KEY is an unsigned long (like NI!) and needs a different hash function
#define hcKey 2

template <class H,int i> inline HASH __fastcall
HashClass<H,i>::operator()(H h) {
	assert(i==hcCast);
	return HASH(h);
	}

template <class H,int i> inline LHASH __fastcall
LHashClass<H,i>::operator()(H h) {
	assert(i==hcCast);
	return LHASH(h);
	}

typedef HashClass<unsigned long, hcCast>	HcNi;

// fwd decl template enum class used as friend
template <class D, class R, class H> class EnumMap;

const unsigned iNil = (unsigned)-1;

template <class D, class R, class H>
class Map {	// map from Domain type to Range type
public:
	Map(unsigned cdrInitial =1) :
		rgd(cdrInitial > 0 ? cdrInitial : 1),
		rgr(cdrInitial > 0 ? cdrInitial : 1)
	{
		cdr = 0;
		traceOnly(cFinds = 0;)
		traceOnly(cProbes = 0;)

		postcondition(fullInvariants());
	}
	~Map() {
		traceOnly(if (cProbes>50) trace((trMap, "~Map() cFinds=%d cProbes=%d cdr=%u rgd.size()=%d\n",
										 cFinds, cProbes, cdr, rgd.size()));)
	}
	void reset();
	BOOL map(D d, R* pr) const;
	BOOL map(D d, R** ppr) const;
	BOOL contains(D d) const;
	BOOL add(D d, R r);
	BOOL remove(D d);
	BOOL save(Buffer* pbuf);
	BOOL reload(PB* ppb);
	void swap(Map& m);
	CB	 cbSave() const;
	unsigned count() const;
private:
	Array<D> rgd;
	Array<R> rgr;
	ISet isetPresent;
	ISet isetDeleted;
	unsigned cdr;
	traceOnly(unsigned cProbes;)
	traceOnly(unsigned cFinds;)

	BOOL find(D d, unsigned *pi) const;
	BOOL grow();
	void shrink();
	BOOL fullInvariants() const;
	BOOL partialInvariants() const;
	unsigned cdrLoadMax() const {
		// we do not permit the hash table load factor to exceed 67%
		return rgd.size() * 2/3 + 1;
	}
	BOOL setHashSize(unsigned size) {
		assert(size >= rgd.size());
		return rgd.setSize(size) && rgr.setSize(size);
	}
	Map(const Map&);
	friend class EnumMap<D,R,H>;
};

template <class D, class R, class H> inline
void Map<D,R,H>::reset() {
	cdr = 0;
	isetPresent.reset();
	isetDeleted.reset();
	rgd.setSize(1);
	rgr.setSize(1);
}

template <class D, class R, class H> inline
BOOL Map<D,R,H>::map(D d, R* pr) const {
	precondition(pr);

	R * prT;
	if (map(d, &prT)) {
		*pr = *prT;
		return TRUE;
	}
	return FALSE;
}

template <class D, class R, class H> inline
BOOL Map<D,R,H>::map(D d, R** ppr) const {
	precondition(partialInvariants());
	precondition(ppr);

	unsigned i;
	if (find(d, &i)) {
		*ppr = &rgr[i];
		return TRUE;
	}
	else
		return FALSE;
}

template <class D, class R, class H> inline
BOOL Map<D,R,H>::contains(D d) const {
	unsigned iDummy;
	return find(d, &iDummy);
}

template <class D, class R, class H> inline
BOOL Map<D,R,H>::add(D d, R r) {
	precondition(partialInvariants());

	unsigned i;
	if (find(d, &i)) {
		// some mapping d->r2 already exists, replace with d->r
		assert(isetPresent.contains(i) && !isetDeleted.contains(i) && rgd[i] == d);
		rgr[i] = r;
	}
	else {
		// establish a new mapping d->r in the first unused entry
		assert(!isetPresent.contains(i));
		isetDeleted.remove(i);
		isetPresent.add(i);
		rgd[i] = d;
		rgr[i] = r;
		grow();
	}

	debug(R rCheck);
	postcondition(map(d, &rCheck) && r == rCheck);
	postcondition(fullInvariants());
	return TRUE;
}

template <class D, class R, class H> inline
void Map<D,R,H>::shrink() {
	--cdr;
}

template <class D, class R, class H> inline
BOOL Map<D,R,H>::remove(D d) {
	precondition(partialInvariants());

	unsigned i;
	if (find(d, &i)) {
		assert(isetPresent.contains(i) && !isetDeleted.contains(i));
		isetPresent.remove(i);
		isetDeleted.add(i);
		shrink();
	}

	postcondition(fullInvariants());
	return TRUE;
}

template <class D, class R, class H> inline
BOOL Map<D,R,H>::find(D d, unsigned *pi) const {  
	precondition(partialInvariants());
	precondition(pi);

	traceOnly(++((Map<D,R,H>*)this)->cFinds;)

	H hasher;
	unsigned n		= rgd.size();
	unsigned h		= hasher(d) % n;
	unsigned i		= h;
	unsigned iEmpty	= iNil;

	do {
		traceOnly(++((Map<D,R,H>*)this)->cProbes;)

		assert(!(isetPresent.contains(i) && isetDeleted.contains(i)));
		if (isetPresent.contains(i)) {
			if (rgd[i] == d) {
				*pi = i;
				return TRUE;
			}
		} else {
			if (iEmpty == iNil)
				iEmpty = i;
			if (!isetDeleted.contains(i))
				break;
		}

		i = (i+1 < n) ? i+1 : 0;
	} while (i != h);

	// not found
	*pi = iEmpty;
	postcondition(*pi != iNil);
	postcondition(!isetPresent.contains(*pi));
	return FALSE;
}

// append a serialization of this map to the buffer
// format:
//	cdr
//	rgd.size()
//	isetPresent
//	isetDeleted
//	group of (D,R) pairs which were present, a total of cdr of 'em
//
template <class D, class R, class H>
BOOL Map<D,R,H>::save(Buffer* pbuf) {
	precondition(fullInvariants());

	unsigned size = rgd.size();
	if (!(pbuf->Append((PB)&cdr, sizeof(cdr)) &&
		  pbuf->Append((PB)&size, sizeof(size)) &&
		  isetPresent.save(pbuf) &&
		  isetDeleted.save(pbuf)))
		return FALSE;

	for (unsigned i = 0; i < rgd.size(); i++)
		if (isetPresent.contains(i))
			if (!(pbuf->Append((PB)&rgd[i], sizeof(rgd[i])) &&
				  pbuf->Append((PB)&rgr[i], sizeof(rgr[i]))))
				return FALSE;


	return TRUE;
}
			   
// reload a serialization of this empty NMT from the buffer; leave
// *ppb pointing just past the NMT representation
template <class D, class R, class H>
BOOL Map<D,R,H>::reload(PB* ppb) {
	precondition(cdr == 0);

	cdr = *((unsigned UNALIGNED *&)*ppb)++;
	unsigned size = *((unsigned UNALIGNED *&)*ppb)++;
	if (!setHashSize(size))
		return FALSE;

	if (!(isetPresent.reload(ppb) && isetDeleted.reload(ppb)))
		return FALSE;

	for (unsigned i = 0; i < rgd.size(); i++) {
		if (isetPresent.contains(i)) {
			rgd[i] = *((D UNALIGNED *&)*ppb)++;
			rgr[i] = *((R UNALIGNED *&)*ppb)++;
		}
	}

	postcondition(fullInvariants());
	return TRUE;
}

template <class D, class R, class H>
BOOL Map<D,R,H>::fullInvariants() const {
	ISet isetInt;
	if (!partialInvariants())
		return FALSE;
	else if (cdr != isetPresent.cardinality())
		return FALSE;
	else if (!intersect(isetPresent, isetDeleted, isetInt))
		return FALSE;
	else if (isetInt.cardinality() != 0)
		return FALSE;
	else
		return TRUE;
}

template <class D, class R, class H>
BOOL Map<D,R,H>::partialInvariants() const {
	if (rgd.size() == 0)
		return FALSE;
	else if (rgd.size() != rgr.size())
		return FALSE;
	else if (cdr > rgd.size())
		return FALSE;
	else if (cdr > 0 && cdr >= cdrLoadMax())
		return FALSE;
	else
		return TRUE;
}

// Swap contents with "map", a la Smalltalk-80 become.
template <class D, class R, class H>
void Map<D,R,H>::swap(Map<D,R,H>& map) {
	isetPresent.swap(map.isetPresent);
	isetDeleted.swap(map.isetDeleted);
	rgd.swap(map.rgd);
	rgr.swap(map.rgr);
	::swap(cdr, map.cdr);
	traceOnly(::swap(cProbes, map.cProbes));
	traceOnly(::swap(cFinds,  map.cFinds));
}

// Return the size that would be written, right now, via save()
template <class D, class R, class H> inline
CB Map<D,R,H>::cbSave() const {
	assert(partialInvariants());
	return
		sizeof(cdr) +
		sizeof(unsigned) +
		isetPresent.cbSave() +
		isetDeleted.cbSave() +
		cdr * (sizeof(D) + sizeof(R))
		;
}

// Return the count of elements
template <class D, class R, class H> inline
unsigned Map<D,R,H>::count() const {
	assert(partialInvariants());
	return cdr;
}


// EnumMap must continue to enumerate correctly in the presence
// of Map<foo>::remove() being called in the midst of the enumeration.
template <class D, class R, class H>
class EnumMap : public Enum {
public:
	EnumMap(const Map<D,R,H>& map) {
		pmap = &map;
		reset();
	}
	void release() {
		delete this;
	}
	void reset() {
		i = (unsigned)-1;
	}
	BOOL next() {
		while (++i < pmap->rgd.size())
			if (pmap->isetPresent.contains(i))
				return TRUE;
		return FALSE;
	}
	void get(OUT D* pd, OUT R* pr) {
		precondition(pd && pr);
		precondition(0 <= i && i < pmap->rgd.size());
		precondition(pmap->isetPresent.contains(i));

		*pd = pmap->rgd[i];
		*pr = pmap->rgr[i];
	}
	void get(OUT D* pd, OUT R** ppr) {
		precondition(pd && ppr);
		precondition(0 <= i && i < pmap->rgd.size());
		precondition(pmap->isetPresent.contains(i));

		*pd = pmap->rgd[i];
		*ppr = &pmap->rgr[i];
	}
private:
	const Map<D,R,H>* pmap;
	unsigned i;
};

template <class D, class R, class H> inline
BOOL Map<D,R,H>::grow() {
	if (++cdr >= cdrLoadMax()) {
		// Table is becoming too full.  Rehash.  Create a second map twice
		// as large as the first, propagate current map contents to new map,
		// then "become" (Smalltalk-80 style) the new map.
		//
		// The storage behind the original map is reclaimed on exit from this block.
		Map<D,R,H> map;
		if (!map.setHashSize(2*cdrLoadMax()))
			return FALSE;

		EnumMap<D,R,H> e(self);
		while (e.next()) {
			D d; R r;
			e.get(&d, &r);
			if (!map.add(d, r))
				return FALSE;
		}
		self.swap(map);
	}
	return TRUE;
}

#endif // !__MAP_INCLUDED__
