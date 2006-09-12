#ifndef BYTESEX_H
#define BYTESEX_H

namespace {
static inline WORD dtohs(WORD in)
{
    PBYTE in_ptr = (PBYTE)&in;
    return in_ptr[0] | (in_ptr[1] << 8);
}

static inline WORD htods(WORD in)
{
    WORD out;
    PBYTE out_ptr = (PBYTE)&out;
    out_ptr[0] = in; out_ptr[1] = in >> 8;
    return out;
}

static inline DWORD dtohl(DWORD in)
{
    PBYTE in_ptr = (PBYTE)&in;
    return in_ptr[0] | (in_ptr[1] << 8) | (in_ptr[2] << 16) | (in_ptr[3] << 24);
}

static inline DWORD htodl(DWORD in)
{
    DWORD out;
    PBYTE out_ptr = (PBYTE)&out;
    out_ptr[0] = in      ; out_ptr[1] = in >> 8;
    out_ptr[2] = in >> 16; out_ptr[3] = in >> 24;
    return out;
}

template <class T> T toHost( T value );
template <class T> T toDest( T value );
template <> WORD toHost<WORD>( WORD value ) { return dtohs(value); }
template <> DWORD toHost<DWORD>( DWORD value ) { return dtohl(value); }
template <> int toHost<int>( int value ) { return dtohl(value); }
template <> WORD toDest<WORD>( WORD value ) { return htods(value); }
template <> DWORD toDest<DWORD>( DWORD value ) { return htodl(value); }
template <> int toDest<int>( int value ) { return htodl(value); }
}

namespace ReactosBytesex {
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
    LENumber &operator &= (T other) { value = toDest<T>(toHost<T>(value) & other); return *this; }
    LENumber &operator |= (T other) { value = toDest<T>(toHost<T>(value) | other); return *this; }
private:
    T value;
};

typedef LENumber<WORD> LEWord;
typedef LENumber<DWORD> LEDWord;
}

#endif/*BYTESEX_H*/
