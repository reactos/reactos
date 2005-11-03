// auto_ptr.h
// This file is (C) 2002-2003 Royce Mitchell III
// and released under the LGPL & BSD licenses

#ifndef AUTO_PTR_H
#define AUTO_PTR_H

template<class T>
class auto_ptr
{
public:
	typedef T element_type;

	explicit auto_ptr(T *p = 0) : _p(p)
	{
	}

	auto_ptr(auto_ptr<T>& rhs) : _p(rhs.release())
	{
	}

	auto_ptr<T>& operator=(auto_ptr<T>& rhs)
	{
		if ( &rhs != this )
		{
			dispose();
			_p = rhs.release();
		}
		return *this;
	}

	auto_ptr<T>& set ( auto_ptr<T>& rhs )
	{
		if ( &rhs != this )
		{
			dispose();
			_p = rhs.release();
		}
		return *this;
	}

	~auto_ptr()
	{
		dispose();
	}

	void dispose()
	{
		if ( _p )
		{
			delete _p;
			_p = 0;
		}
	}

	T& operator[] ( int i )
	{
		return _p[i];
	}

	T& operator*() const
	{
		return *_p;
	}

	T* operator->() const
	{
		return _p;
	}

	T* get() const
	{
		return _p;
	}

	T* release()
	{
		T* p = _p;
		_p = 0;
		return p;
	}

private:
	T* _p;
};

#endif//AUTO_PTR_H
