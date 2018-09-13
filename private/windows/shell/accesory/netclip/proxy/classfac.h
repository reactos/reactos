#ifndef _SHELLEXFACTORY_INCLUDE
#define _SHELLEXFACTORY_INCLUDE

class CShellExFactory : public IClassFactory {
public:
	HRESULT _stdcall QueryInterface(REFIID riid, void** ppObject);
	ULONG	_stdcall AddRef();
	ULONG	_stdcall Release();
	HRESULT _stdcall CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppObject);
	HRESULT	_stdcall LockServer(BOOL fLock);

	CShellExFactory::CShellExFactory();
private:
	ULONG m_dwRefCount;
};

#endif
