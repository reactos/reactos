#ifndef BYTESEX_H
#define BYTESEX_H

enum EndianOrder { BE = -1, LE = 0 };
enum WordSizeMask { W = 1, D = 3, Q = 7 };
extern enum EndianOrder order;

#ifdef __GNUC__
typedef long long LL_T;
typedef unsigned long long ULL_T;
#else
typedef __int64 LL_T;
typedef unsigned __int64 ULL_T;
#endif

namespace ReactosBytesex {
#define C(x,e,m) (x ^ (e & m))

static inline USHORT dtohs(USHORT in)
{
    PUCHAR in_ptr = (PUCHAR)&in;
    return in_ptr[C(0,order,W)] | (in_ptr[C(1,order,W)] << 8);
}

static inline USHORT htods(USHORT in)
{
    USHORT out;
    PUCHAR out_ptr = (PUCHAR)&out;
    out_ptr[C(0,order,W)] = in; out_ptr[C(1,order,W)] = in >> 8;
    return out;
}

static inline ULONG dtohl(ULONG in)
{
    PUCHAR in_ptr = (PUCHAR)&in;
    return 
	in_ptr[C(0,order,D)] | 
	(in_ptr[C(1,order,D)] << 8) | 
	(in_ptr[C(2,order,D)] << 16) | 
	(in_ptr[C(3,order,D)] << 24);
}

static inline ULONG htodl(ULONG in)
{
    ULONG out;
    PUCHAR out_ptr = (PUCHAR)&out;
    out_ptr[C(0,order,D)] = in      ; out_ptr[C(1,order,D)] = in >> 8;
    out_ptr[C(2,order,D)] = in >> 16; out_ptr[C(3,order,D)] = in >> 24;
    return out;
}

static inline LL_T dtohq(LL_T in)
{
    int i;
    LL_T out = 0;
    for( i = 0; i < 8; i++ )
    {
	out |= (in & 0xff) << ((C(i,order,Q)) * 8);
	in >>= 8;
    }
    return out;
}

static inline LL_T htodq(LL_T in)
{
    int i;
    LL_T out;
    PUCHAR out_ptr = (PUCHAR)&out;
    for( i = 0; i < 8; i++ )
	out_ptr[C(i,order,Q)] = in >> (i * 8);
}

namespace {
template <class T> T toHost( T value );
template <class T> T toDest( T value );
template <> SHORT toHost<SHORT>( SHORT value ) { return dtohs(value); }
template <> USHORT toHost<USHORT>( USHORT value ) { return dtohs(value); }
template <> LONG toHost<LONG>( LONG value ) { return dtohl(value); }
template <> ULONG toHost<ULONG>( ULONG value ) { return dtohl(value); }
template <> LL_T toHost<LL_T>( LL_T value ) { return dtohq(value); }
template <> ULL_T toHost<ULL_T>( ULL_T value ) { return dtohq(value); }
template <> SHORT toDest<SHORT>( SHORT value ) { return htods(value); }
template <> USHORT toDest<USHORT>( USHORT value ) { return htods(value); }
template <> LONG toDest<LONG>( LONG value ) { return htodl(value); }
template <> ULONG toDest<ULONG>( ULONG value ) { return htodl(value); }
template <> LL_T toDest<LL_T>( LL_T value ) { return htodq(value); }
template <> ULL_T toDest<ULL_T>( ULL_T value ) { return htodq(value); }
}

#ifdef _MSC_VER
#pragma pack ( push, hive_header, 1 )
#endif

template <class T>
struct LENumber {
public:
    operator T () const { return toHost<T>(value); }
    LENumber &operator = (T other) { value = toDest<T>(other); return *this; }
    T operator ++ () { (*this) += 1; return *this; }
    LENumber &operator ++ (int) { (*this) += 1; return *this; }
    T operator -- () { (*this) -= 1; return *this; }
    LENumber &operator -- (int) { (*this) -= 1; return *this; }
    LENumber &operator += (T other) { value = toDest<T>(toHost<T>(value) + other); return *this; }
    LENumber &operator -= (T other) { return (*this) += -other; }
    LENumber &operator *= (T other) { int x = (*this); x *= other; (*this) = x; }
    LENumber &operator &= (T other) { value = toDest<T>(toHost<T>(value) & other); return *this; }
    LENumber &operator |= (T other) { value = toDest<T>(toHost<T>(value) | other); return *this; }
private:
    T value;
} __attribute__((packed));

#ifdef _MSC_VER
#pragma pack (pop, hive_header)
#endif

typedef LENumber<SHORT> E_WCHAR;
typedef LENumber<SHORT> E_SHORT;
typedef LENumber<USHORT> E_USHORT;
typedef LENumber<LONG> E_LONG;
typedef LENumber<ULONG> E_ULONG;
typedef LENumber<ULL_T> E_FILETIME;
}

#endif/*BYTESEX_H*/
