// Reli.h
// lots of code here is (c) Bartosz Milewski, 1996, www.relisoft.com
// The rest is (C) 2002-2004 Royce Mitchell III
// and released under the LGPL & BSD licenses

#ifndef __RELI_H
#define __RELI_H

////////////////////////////////////////////////////////////////////////////////
// Assert

#undef Assert
#ifdef NDEBUG
	#define Assert(exp)	((void)0)
#else
	void _wassert (char* szExpr, char* szFile, int line);
	#define Assert(exp) (void)( (exp) || (_wassert(#exp, __FILE__, __LINE__), 0) )
#endif	/* NDEBUG */

////////////////////////////////////////////////////////////////////////////////
// Swap

template <class T>
void Swap(T a,T b)
{
	T t = a;
	a = b;
	b = t;
}

////////////////////////////////////////////////////////////////////////////////
// Uncopyable - base class disabling copy ctors
class Uncopyable
{
public:
	Uncopyable(){} // need a default ctor
private:
	Uncopyable ( const Uncopyable& );
	const Uncopyable& operator = ( const Uncopyable& );
};

////////////////////////////////////////////////////////////////////////////////
// SPtr - Smart Pointer's must be passed by reference or const reference

template <class T>
class SPtr : public Uncopyable
{
public:
	virtual ~SPtr () { Destroy(); }
	T * operator->() { return _p; }
	T const * operator->() const { return _p; }
	operator T&() { Assert(_p); return *_p; }
	operator const T&() const { Assert(_p); return *_p; }
	void Acquire ( SPtr<T>& t ) { Destroy(); Swap(_p,t._p); }
	void Destroy() { if ( _p ) { delete _p; _p = 0; } }
protected:
	SPtr (): _p (0) {}
	explicit SPtr (T* p): _p (p) {}
	T * _p;
private:
	operator T* () { return _p; }
};

#define DECLARE_SPTR(cls,init,init2) \
class S##cls : public SPtr<cls> \
{ \
public: \
	S##cls ( cls* p ) : SPtr<cls>(p) {} \
	explicit S##cls init : SPtr<cls> (new cls init2) {} \
};
/* Example Usage of DECLARE_SPTR:
class MyClass
{
public: // can be protected
	MyClass ( int i )
	{
		...
	}
	...
}; DECLARE_SPTR(MyClass,(int i),(i))
SMyClass ptr(i);
*/
#define DECLARE_SPTRV(cls) typedef SPtr<cls> S##cls;
/* Example Usage of DECLARE_SPTRV:
class MyAbstractClass
{
public: // can be protected
	MyAbstractClass ( int i )
	{
		...
	}
	void MyPureVirtFunc() = 0;
	...
}; DECLARE_SPTRV(MyAbstractClass)
SMyAbstractClass ptr ( new MySubClass(i) );
*/

#define DECLARE_PTR(cls,init,init2) \
	class Ptr : public SPtr<cls> \
	{ \
		Ptr(cls* p) : SPtr<cls> ( p ) \
		{ \
		} \
		Ptr init : SPtr<cls> ( new cls init2 ) {} \
	};
/* Example Usage of DECLARE_PTR:
class MyClass
{
	DECLARE_PTR(MyClass,(int i),(i))
public: // can be protected
	MyClass ( int i )
	{
		...
	}
	void MyPureVirtFunc() = 0;
	...
};
MyClass::Ptr ptr ( i );
*/

#define DECLARE_PTRV(cls) \
	class Ptr : public SPtr<cls> \
	{ \
		Ptr(cls* p) : SPtr<cls> ( p ) \
		{ \
		} \
	};
/* Example Usage of DECLARE_PTRV:
class MyAbstractClass
{
	DECLARE_PTRV(MyAbstractClass)
public: // can be protected
	MyAbstractClass ( int i )
	{
		...
	}
	void MyPureVirtFunc() = 0;
	...
};
MyAbstractClass::Ptr ptr ( new MySubClass(i) );
*/

#endif//__RELI_H
