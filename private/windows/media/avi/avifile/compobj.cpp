
/*	-	-	-	-	-	-	-	-	*/

/*
**	Copyright (C) Microsoft Corporation 1992-1995. All rights reserved.
*/

/*	-	-	-	-	-	-	-	-	*/

#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <ole2.h>
#include <coguid.h>

#define	INTERNAL_(type)	type


const char	aszRegServerKey[] = "InprocServer";
const char	aszServerEntry[] = "DllGetClassObject";
const char	aszServerQuery[] = "DllCanUnloadNow";
const char	aszCLSID[] = "CLSID\\";
STDAPI_(void) MyFreeUnusedLibraries(void);

/*	-	-	-	-	-	-	-	-	*/

struct DllEntry {
public:
	CLSID	clsid;
	IClassFactory FAR*	pFactory;
	LPFNCANUNLOADNOW	lpfnCanUnloadNow;
	HINSTANCE	hInstance;
	DllEntry FAR*	pNextDll;
};

class FAR CTask {
public:
	static CTask FAR* LookupTask(HTASK FAR& hTask);
	static IClassFactory FAR* LookupClass(CTask FAR* pTask, REFCLSID clsid, HINSTANCE hInstance);
	static void FreeUnusedLibraries(CTask FAR* pTask);
	HRESULT AddTaskDll(REFCLSID rclsid, IClassFactory FAR* pFactory, LPFNCANUNLOADNOW lpfnCanUnloadNow, HINSTANCE hInstance);
	IMalloc FAR* QueryMalloc(void)
	{
		return m_pMalloc;
	};
	CTask(HTASK hTask, IMalloc FAR* pMalloc);
	AddRef(void);
	Release(void);
	~CTask(void);
private:
	ULONG	m_refs;
	HTASK	m_hTask;
	IMalloc FAR*	m_pMalloc;
	DllEntry FAR*	m_pDllEntry;
	CTask FAR*	m_pTaskNext;
};

/*	-	-	-	-	-	-	-	-	*/

#define	GlobalPtrHandle(pv)	((HGLOBAL)LOWORD(GlobalHandle(SELECTOROF(pv))))

class CStdMalloc : public IMalloc {
public:
	CStdMalloc(void)
	{
		m_refs = 0;
	}
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void FAR* FAR* ppvObj)
	{
		if (iid == IID_IUnknown || iid == IID_IMalloc) {
			*ppvObj = this;
			m_refs++;
			return NULL;
		} else
			return ResultFromScode(E_NOINTERFACE);
	}
	ULONG STDMETHODCALLTYPE AddRef(void)
	{
		return ++m_refs;
	}
	ULONG STDMETHODCALLTYPE Release(void)
	{
		return --m_refs;
	}
	void FAR* STDMETHODCALLTYPE Alloc(ULONG cb)
	{
		return (void FAR*)GlobalLock(GlobalAlloc(GMEM_SHARE | GMEM_FIXED, cb));
	}
	void FAR* STDMETHODCALLTYPE Realloc(void FAR* pv, ULONG cb)
	{
		HGLOBAL	h;

		h = GlobalPtrHandle(pv);
		GlobalUnlock(h);
		return (void FAR*)GlobalLock(GlobalReAlloc(h, cb, GMEM_FIXED));
	}
	void STDMETHODCALLTYPE Free(void FAR* pv)
	{
		GlobalFree(GlobalPtrHandle(pv));
	}
	ULONG STDMETHODCALLTYPE GetSize(void FAR* pv)
	{
		return GlobalSize(GlobalPtrHandle(pv));
	}
	int STDMETHODCALLTYPE DidAlloc(void FAR* pv)
	{
		return !IsBadWritePtr(pv, 0);
	}
	void STDMETHODCALLTYPE HeapMinimize(void)
	{
		GlobalCompact(-1);
	}
private:
	ULONG	m_refs;
};

/*	-	-	-	-	-	-	-	-	*/

CTask FAR*	pTaskList;
CStdMalloc NEAR	v_stdMalloc;

/*	-	-	-	-	-	-	-	-	*/

CTask FAR* CTask::LookupTask(
	HTASK FAR&	hTask)
{
	CTask FAR*	pTaskCurrent;

	hTask = GetCurrentTask();
	for (pTaskCurrent = pTaskList; pTaskCurrent; pTaskCurrent = pTaskCurrent->m_pTaskNext)
		if (pTaskCurrent->m_hTask == hTask)
			return pTaskCurrent;
	return NULL;
}

/*	-	-	-	-	-	-	-	-	*/

IClassFactory FAR* CTask::LookupClass(
	CTask FAR*	pTask,
	REFCLSID	rclsid,
	HINSTANCE	hInstance)
{
	DllEntry FAR*	pDllEntry;

	for (pDllEntry = pTask->m_pDllEntry; pDllEntry; pDllEntry = pDllEntry->pNextDll) {
		if ((hInstance == pDllEntry->hInstance) &&
		    (rclsid == pDllEntry->clsid))
			return pDllEntry->pFactory;
	}
	return NULL;
}

/*	-	-	-	-	-	-	-	-	*/

void CTask::FreeUnusedLibraries(CTask FAR* pTask)
{
	DllEntry FAR*	pDllEntryPrev;
	DllEntry FAR*	pDllEntryCur;

	pDllEntryPrev = NULL;
	pDllEntryCur = pTask->m_pDllEntry;
	for (; pDllEntryCur;)
		if (pDllEntryCur->lpfnCanUnloadNow() == S_OK) {
			pDllEntryCur->pFactory->Release();
			FreeModule(pDllEntryCur->hInstance);
			if (pDllEntryPrev == NULL) {
				pTask->m_pDllEntry = pDllEntryCur->pNextDll;
				pTask->m_pMalloc->Free(pDllEntryCur);
				pDllEntryCur = pTask->m_pDllEntry;
			} else {
				pDllEntryPrev->pNextDll = pDllEntryCur->pNextDll;
				pTask->m_pMalloc->Free(pDllEntryCur);
				pDllEntryCur = pDllEntryPrev->pNextDll;
			}
			
		} else {
			pDllEntryPrev = pDllEntryCur;
			pDllEntryCur = pDllEntryCur->pNextDll;
		}
}

/*	-	-	-	-	-	-	-	-	*/

HRESULT CTask::AddTaskDll(
	REFCLSID	rclsid,
	IClassFactory FAR*	pFactory,
	LPFNCANUNLOADNOW	lpfnCanUnloadNow,
	HINSTANCE	hInstance)
{
	DllEntry FAR*	pDllEntry;

	pDllEntry = (DllEntry FAR*)(m_pMalloc->Alloc(sizeof(DllEntry)));
	if (!pDllEntry)
		return ResultFromScode(E_OUTOFMEMORY);
	pDllEntry->clsid = rclsid;
	pDllEntry->pFactory = pFactory;
	pDllEntry->lpfnCanUnloadNow = lpfnCanUnloadNow;
	pDllEntry->hInstance = hInstance;
	pDllEntry->pNextDll = m_pDllEntry;
	m_pDllEntry = pDllEntry;
	return NULL;
}

/*	-	-	-	-	-	-	-	-	*/

CTask::CTask(
	HTASK	hTask,
	IMalloc FAR*	pMalloc)
{
	m_refs = 1;
	m_hTask = hTask;
	m_pMalloc = pMalloc;
	m_pMalloc->AddRef();
	m_pDllEntry = NULL;
	m_pTaskNext = pTaskList;
	pTaskList = this;
}

CTask::AddRef(void)
{
	++m_refs;

	return 0;
}

CTask::Release(void)
{
	if (m_refs == 1)
		delete this;
	else
		--m_refs;

	return 0;
}

/*	-	-	-	-	-	-	-	-	*/

CTask::~CTask(
	void)
{
	for (; m_pDllEntry;) {
		DllEntry FAR*	pDllEntry;

		m_pDllEntry->pFactory->Release();
		FreeModule(m_pDllEntry->hInstance);
		pDllEntry = m_pDllEntry->pNextDll;
		m_pMalloc->Free(m_pDllEntry);
		m_pDllEntry = pDllEntry;
	}
	m_pMalloc->Release();
	if (this == pTaskList)
		pTaskList = m_pTaskNext;
	else {
		CTask FAR*	pTask;

		for (pTask = pTaskList; pTask->m_pTaskNext != this; pTask = pTask->m_pTaskNext)
			;
		pTask->m_pTaskNext = m_pTaskNext;
	}
}

/*	-	-	-	-	-	-	-	-	*/

// converts GUID into (...) form without leading identifier; no errors
INTERNAL_(int) StringFromGUID2(REFGUID rguid, LPSTR lpsz)
{
    wsprintf(lpsz, "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
	    rguid.Data1, rguid.Data2, rguid.Data3,
	    rguid.Data4[0], rguid.Data4[1],
	    rguid.Data4[2], rguid.Data4[3],
	    rguid.Data4[4], rguid.Data4[5],
	    rguid.Data4[6], rguid.Data4[7]);

	return _fstrlen(lpsz) + 1;
}

/*	-	-	-	-	-	-	-	-	*/

#define GUIDSTR_MAX (1+ 3*sizeof(GUID) +sizeof(GUID)-1 +1 +1)
#define CLSIDSTR_MAX (sizeof(aszCLSID)-1+GUIDSTR_MAX)

// alternate to StringFromCLSID which puts string in caller-supplied buffer;
// returns the amount of data copied including the zero terminator; 0 if none.
STDAPI_(int) StringFromCLSID2(REFCLSID rclsid, LPSTR lpsz, int cbMax)
{
	if (cbMax < CLSIDSTR_MAX)
		return 0;

	return sizeof(aszCLSID)-1 +
		StringFromGUID2(rclsid, _fstrchr(_fstrcpy(lpsz, aszCLSID),'\0'));
}

/*	-	-	-	-	-	-	-	-	*/

static LONG RegQueryClassValue(REFCLSID rclsid, LPCSTR lpszSubKey, LPSTR lpszValue, int cbMax)
{
	char szKey[256];
	int cbClsid;
	LONG cbValue = cbMax;

	// translate rclsid into string
	cbClsid = StringFromCLSID2(rclsid, &szKey[0], sizeof(szKey));

	szKey[cbClsid-1] = '\\';
	_fstrcpy(&szKey[cbClsid], lpszSubKey);

	return RegQueryValue(HKEY_CLASSES_ROOT, szKey, lpszValue, &cbValue);
}

/*	-	-	-	-	-	-	-	-	*/

STDAPI  CoGetClassObject(
	REFCLSID	rclsid,
	DWORD	dwClsContext,
	LPVOID	pvReserved,
	REFIID	riid,
	void FAR* FAR*	ppv)
{
	char	aszServer[256];
	HTASK	htask;
	CTask FAR*	pTask;
	IClassFactory FAR*	pFactory;
	HINSTANCE	hInstance;
	HRESULT	hr;

	if (pvReserved != NULL)
		return ResultFromScode(E_INVALIDARG);
	if (!(dwClsContext & CLSCTX_INPROC_SERVER))
		return ResultFromScode(E_INVALIDARG);
	if (!(pTask = CTask::LookupTask(htask)))
		return ResultFromScode(E_UNEXPECTED);
	if (RegQueryClassValue(rclsid, aszRegServerKey, aszServer, sizeof(aszServer)) != 0)
		return ResultFromScode(E_UNEXPECTED);
	hInstance = LoadLibrary(aszServer);
	if (hInstance < HINSTANCE_ERROR)
		return ResultFromScode(E_UNEXPECTED);
	if (pFactory = CTask::LookupClass(pTask, rclsid, hInstance))
		hr = pFactory->QueryInterface(riid, ppv);
	else {
		LPFNCANUNLOADNOW	lpfnCanUnloadNow;
		LPFNGETCLASSOBJECT	lpfnGetClassObject;

		lpfnCanUnloadNow = (LPFNCANUNLOADNOW)GetProcAddress(hInstance, aszServerQuery);
		if ((lpfnGetClassObject = (LPFNGETCLASSOBJECT)GetProcAddress(hInstance, aszServerEntry)) != NULL) {
			IMalloc FAR*	pMalloc;

			pMalloc = pTask->QueryMalloc();
			hr = (*lpfnGetClassObject)(rclsid, IID_IClassFactory, (void FAR* FAR*)&pFactory);
			if (!hr) {
				hr = pTask->AddTaskDll(rclsid, pFactory, lpfnCanUnloadNow, hInstance);
				if (!hr)
					return pFactory->QueryInterface(riid, ppv);
				pFactory->Release();
			}
		} else
			hr = ResultFromScode(E_UNEXPECTED);
	}
	FreeLibrary(hInstance);
	return hr;
}

/*	-	-	-	-	-	-	-	-	*/

STDAPI CoCreateInstance(
	REFCLSID	rclsid,
	IUnknown FAR* pUnkOuter,
	DWORD	dwClsContext,
	REFIID	riid,
	LPVOID FAR*	ppv)
{
	HRESULT	hr;
	IClassFactory FAR*	pFactory;

	hr = CoGetClassObject(rclsid, dwClsContext, NULL, IID_IClassFactory, (void FAR* FAR*)&pFactory);
	if (!hr) {
		hr = pFactory->CreateInstance(pUnkOuter, riid, ppv);
		pFactory->Release();
	}
	return hr;
}

/*	-	-	-	-	-	-	-	-	*/

STDAPI GetStandardTaskMalloc(
	IMalloc FAR* FAR* ppMalloc)
{
	v_stdMalloc.AddRef();
	*ppMalloc = &v_stdMalloc;
	return NULL;
}

/*	-	-	-	-	-	-	-	-	*/

STDAPI CoGetMalloc(
	DWORD	dwMemContext,
	IMalloc FAR* FAR* ppMalloc)
{
	HTASK	htask;
	CTask FAR*	pTask;
	IMalloc FAR*	pMalloc;

	if (dwMemContext != MEMCTX_TASK)
		return ResultFromScode(E_UNEXPECTED);
	if (!(pTask = CTask::LookupTask(htask)))
		return ResultFromScode(E_UNEXPECTED);
	pMalloc = pTask->QueryMalloc();
	pMalloc->AddRef();
	*ppMalloc = pMalloc;
	return NULL;
}

/*	-	-	-	-	-	-	-	-	*/

STDAPI CoInitialize(
	IMalloc FAR*	pMalloc)
{
	HTASK	htask;
	CTask FAR*	pTask;

	if (!pMalloc)
		pMalloc = (IMalloc FAR *) &v_stdMalloc;
	if (pTask = CTask::LookupTask(htask)) {
		pTask->AddRef();
		return ResultFromScode(S_FALSE);
	}
	pTask = new FAR CTask(htask, pMalloc);
	return pTask ? NULL : ResultFromScode(E_OUTOFMEMORY);
}

/*	-	-	-	-	-	-	-	-	*/

STDAPI_(void) CoUninitialize(
	void)
{
	HTASK	htask;
	CTask FAR*	pTask;

	if (pTask = CTask::LookupTask(htask))
		pTask->Release();
}

/*	-	-	-	-	-	-	-	-	*/

STDAPI_(void) MyFreeUnusedLibraries(
	void)
{
	HTASK	htask;
	CTask FAR*	pTask;

	if (pTask = CTask::LookupTask(htask))
		CTask::FreeUnusedLibraries(pTask);
}

/*	-	-	-	-	-	-	-	-	*/

STDAPI_(BOOL)  IsEqualGUID(REFGUID guid1, REFGUID guid2)
{
	return guid1 == guid2;
}

