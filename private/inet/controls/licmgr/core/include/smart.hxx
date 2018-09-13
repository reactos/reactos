//+----------------------------------------------------------------------------
//  File:   smart.hxx
//
//  Synopsis:
//
//-----------------------------------------------------------------------------


#ifndef	_SMART_HXX
#define	_SMART_HXX


///////////////////////////////////////////////////////////////////////////////
//	TSRef:	Smart COM reference template
//
template <class TYPE> class TSRef			// Hungarian: srp
{
public:
	TSRef()             { _punk = NULL; }
	TSRef(TYPE* punk)   { _punk = punk; }
	~TSRef()            { ::SRelease(_punk); }

	operator TYPE*()    { Assert(_punk); return _punk; }

	TYPE*	operator->()            { Assert(_punk); return _punk; }
	TYPE**	operator&()             { Assert(!_punk); return &_punk; }
	TYPE*	operator=(TYPE* punk)   { Assert(!_punk); _punk = punk; return _punk; }

	TYPE*	Disown()    { _punk = NULL; }
	TYPE**	InOut()     { return &_punk; }
	void	SClear()    { ::SClear(_punk); }

protected:
	TYPE*	_punk;

private:
	TSRef(const TSRef<TYPE>& srp);
	TSRef<TYPE>& operator=(const TSRef<TYPE>& srp);
};


///////////////////////////////////////////////////////////////////////////////
//	TCRef:	Smart COM object pointer template
//
template <class TYPE> class TCRef			// Hungarian: crp
{
public:
	TCRef()             { _punk = NULL; }
	TCRef(TYPE* punk)   { _punk = punk; }
	~TCRef()            { ::SRelease((IUnknown *)_punk); }

	operator TYPE*()    { Assert(_punk); return _punk; }

	TYPE*	operator->()            { Assert(_punk); return _punk; }
	TYPE**	operator&()             { Assert(!_punk); return &_punk; }
	TYPE*	operator=(TYPE* punk)   { Assert(!_punk); _punk = punk; return _punk; }

	TYPE*	Disown()    { _punk = NULL; }
	TYPE**	InOut()     { return &_punk; }
	void	SClear()    { ::SClear((IUnknown *)_punk); }

protected:
	TYPE*	_punk;

private:
	TCRef(const TCRef<TYPE>& srp);
	TCRef<TYPE>& operator=(const TCRef<TYPE>& srp);
};


///////////////////////////////////////////////////////////////////////////////
//	TSPtr:	Smart pointer template
//
template <class TYPE> class TSPtr			// Hungarian: sp
{
public:
	TSPtr()             { _pv = NULL; }
	TSPtr(TYPE* pv)     { _pv = pv; }
	~TSPtr()            { if (_pv) delete pv; }

	operator TYPE*()    { Assert(_pv); return _pv; }

	TYPE*	operator->()        { Assert(_pv); return _pv; }
	TYPE&	operator*()         { Assert(_pv); return *_pv; }
	TYPE**	operator&()         { Assert(!_pv); return &_pv; }
	TYPE*	operator=(TYPE* pv) { Assert(!_pv); _pv = pv; return _pv; }

	TYPE*	Disown()    { _pv = NULL; }
	TYPE**	InOut()     { return &_pv; }
	void	SClear()    { delete _pv; _pv = NULL; }

protected:
	TYPE*	_pv;

private:
	TSPtr(const TSPtr<TYPE>& srp);
	TSPtr<TYPE>& operator=(const TSPtr<TYPE>& srp);
};


///////////////////////////////////////////////////////////////////////////////
//	TSArray:	Smart array template
//
template <class TYPE> class TSArray			// Hungarian: ssz or sa
{
public:
	TSArray()           { _pv = NULL; }
	TSArray(TYPE* pv)   { _pv = pv; }
	~TSArray()          { delete [] _pv; }

	operator TYPE*()    { Assert(_pv); return _pv; }

	TYPE*	operator->()        { Assert(_pv); return _pv; }
	TYPE&	operator*()         { Assert(_pv); return *_pv; }
	TYPE**	operator&()         { Assert(!_pv); return &_pv; }
	TYPE*	operator=(TYPE* pv) { Assert(!_pv); _pv = pv; return _pv; }
	TYPE&	operator[](int iItem)       { Assert(_pv); return *(_pv + iItem); }
	TYPE*	operator+(const int cItem)  { Assert(_pv); return _pv + cItem; }
	TYPE*	operator-(const int cItem)  { Assert(_pv); return _pv - cItem; }
	int		operator-(const TSArray<TYPE>& sa)  { Assert(_pv); Assert(sz._pv); return _pv - sa._pv; }
	int		operator-(const TYPE* pv)   { Assert(_pv); Assert(pv); return _pv - pv; }

	TYPE*	Disown()    { _pv = NULL; }
	TYPE**	InOut()     { return &_pv; }
	void	SClear()    { delete [] _pv; _pv = NULL; }

protected:
	TYPE*	_pv;

private:
	TSArray(const TSArray<TYPE>& sa);
	TSArray<TYPE>& operator=(const TSArray<TYPE>& sa);
};


#endif // _SMART_HXX
