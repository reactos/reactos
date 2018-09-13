#pragma once
#include "crefobj.h"

template <class T>
class SimpleArray : public CRefCountedObj {
protected:
    unsigned    _cT;
    T *         _rgT;

public:
    SimpleArray ( unsigned cT ) {
        if ( cT ) {
            _rgT = new T[cT];
            if ( _rgT ) {
                _cT = cT;
                }
            }
        else {
            _cT = 0;
            _rgT = 0;
            }
        }

    ~SimpleArray() {
        if ( _rgT ) {
            delete [] _rgT;
            }
        _cT = 0;
        _rgT = 0;
        }

    unsigned Count() const {
        return _cT;
        }

    const T * Base() const {
        return _rgT;
        }

    T * Base() {
        return _rgT;
        }

    T & operator[] ( unsigned it ) {
        return _rgT[it];
        }

    const T & operator[] ( unsigned it ) const {
        return _rgT[it];
        }
    };

class SimpleString: public SimpleArray<char>
{
public:
    unsigned Length() const { return strlen( _rgT ); } // # non-nulls
    const char* operator=( const char *str ) {
        return Set( str, strlen( str ) );
    }
    void Clear() {
        assert( Count() > 0 );
        Set( "", 0 );
    }
    const char* Set( const char* str, unsigned len ) {
        Grow( len+1 );
        strncpy( _rgT, str, len );
        _rgT[ len ] = '\0';
        return Base();
    }
    SimpleString( const SimpleString& str ) 
        : SimpleArray<char>( str.Length()+1 ) {
        Set( str.Base(), str.Length()+1 );
    }
    SimpleString( unsigned len = 256 ) : SimpleArray<char>( max( len, 1 ) ) {
        Set( "", 0 );
    }
private:
    bool Grow( unsigned cb ) {
        if ( Count() < cb ) {
            delete [] _rgT;
            _rgT = new char[cb];
            _cT = cb;
        }
        return _rgT != 0;
    }

};
    
