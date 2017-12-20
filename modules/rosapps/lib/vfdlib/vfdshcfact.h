/*
	vfdshcfact.h

	Virtual Floppy Drive for Windows
	Driver control library
	shell extension COM class-factory class header

	Copyright (c) 2003-2005 Ken Kato
*/

#ifndef _VFDSHCFACT_H_
#define _VFDSHCFACT_H_

//
//	CVfdFactory
//	class factory class to create the COM shell extension object
//
class CVfdFactory : public IClassFactory
{
protected:
	ULONG	m_cRefCnt;		//	Reference count to the object

public:
	//	Constructor
	CVfdFactory();

	//	Destructor
	~CVfdFactory();

	//	IUnknown inheritance
	STDMETHODIMP QueryInterface(REFIID, LPVOID *);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	//	IClassFactory inheritance
	STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID *);
	STDMETHODIMP LockServer(BOOL);
};

typedef CVfdFactory *LPCVFDFACTORY;

#endif	// _VFDSHCFACT_H_
