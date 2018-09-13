//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:	stgwrap.hxx
//
//  Contents:	IStorage and IStream wrappers for async docfile
//
//  Classes:	CAsyncStorage
//				CAsyncRootStorage
//				CAsyncStream	
//				CConnectionPoint
//
//  Functions:	
//
//  History:	27-Dec-95	SusiA	Created
//
//----------------------------------------------------------------------------

#ifndef __ASYNCEXPDF_HXX__
#define __ASYNCEXPDF_HXX__

#include "sinklist.hxx"
#include "filllkb.hxx"

//BUGBUG:  defined in dfmsp.hxx.  
typedef DWORD LPSTGSECURITY;

//+---------------------------------------------------------------------------
//
//  Class:	CAsyncStorage
//
//  Purpose:	Wrap storage objects for Async Docfiles	
//
//  Interface:	
//
//  History:	28-Dec-95	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------
	
class CAsyncStorage: 
		public IStorage,
		public IConnectionPointContainer
{
public:
	inline CAsyncStorage(IStorage *pstg, CFillLockBytes *pflb);
    inline ~CAsyncStorage(void);
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // IStorage
    STDMETHOD(CreateStream)(OLECHAR const *pwcsName,
                            DWORD grfMode,
                            DWORD reserved1,
                            DWORD reserved2,
                            IStream **ppstm);
    STDMETHOD(OpenStream)(OLECHAR const *pwcsName,
			  void *reserved1,
                          DWORD grfMode,
                          DWORD reserved2,
                          IStream **ppstm);
    STDMETHOD(CreateStorage)(OLECHAR const *pwcsName,
                             DWORD grfMode,
                             DWORD reserved1,
                             LPSTGSECURITY reserved2,
                             IStorage **ppstg);
    STDMETHOD(OpenStorage)(OLECHAR const *pwcsName,
                           IStorage *pstgPriority,
                           DWORD grfMode,
                           SNB snbExclude,
                           DWORD reserved,
                           IStorage **ppstg);
    STDMETHOD(CopyTo)(DWORD ciidExclude,
		      IID const *rgiidExclude,
		      SNB snbExclude,
		      IStorage *pstgDest);
    STDMETHOD(MoveElementTo)(OLECHAR const *lpszName,
    			     IStorage *pstgDest,
                             OLECHAR const *lpszNewName,
                             DWORD grfFlags);
    STDMETHOD(Commit)(DWORD grfCommitFlags);
    STDMETHOD(Revert)(void);
    STDMETHOD(EnumElements)(DWORD reserved1,
			    void *reserved2,
			    DWORD reserved3,
			    IEnumSTATSTG **ppenm);
    STDMETHOD(DestroyElement)(OLECHAR const *pwcsName);
    STDMETHOD(RenameElement)(OLECHAR const *pwcsOldName,
                             OLECHAR const *pwcsNewName);
    STDMETHOD(SetElementTimes)(const OLECHAR *lpszName,
    			       FILETIME const *pctime,
                               FILETIME const *patime,
                               FILETIME const *pmtime);
    STDMETHOD(SetClass)(REFCLSID clsid);
    STDMETHOD(SetStateBits)(DWORD grfStateBits, DWORD grfMask);
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);

	//From IConnectionPointContainer
    STDMETHOD(EnumConnectionPoints)(IEnumConnectionPoints **ppEnum);
    STDMETHOD(FindConnectionPoint)(REFIID iid, IConnectionPoint **ppCP);

	SCODE Notify(void);

protected:
	LONG _cReferences;
	IStorage *_pRealStg;
	CFillLockBytes *_pflb;
	CConnectionPoint _cpoint;
	
};

inline CAsyncStorage::CAsyncStorage(IStorage *pstg, CFillLockBytes *pflb)
{	
	_cReferences = 1;
	_pRealStg = pstg;
	_pflb = pflb;
}

inline CAsyncStorage::~CAsyncStorage(void)
{
	if (_pRealStg != NULL)
		_pRealStg->Release;
}



//+---------------------------------------------------------------------------
//
//  Class:	CAsyncRootStorage
//
//  Purpose:	Wrap Root Storage objects for Async Docfiles	
//
//  Interface:	
//
//  History:	28-Dec-95	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

class CAsyncRootStorage: 
		public IRootStorage,
		public CAsyncStorage
{
public:
    inline CAsyncRootStorage(IStorage *pstg, CFillLockBytes *pflb);
    
	// IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);
    // IRootStorage
    STDMETHOD(SwitchToFile)(OLECHAR *ptcsFile);
};

inline CAsyncRootStorage::CAsyncRootStorage(IStorage *pstg, CFillLockBytes *pflb)
  :CAsyncStorage(pstg, pflb)
{	
}


//+---------------------------------------------------------------------------
//
//  Class:		CAsyncStream
//
//  Purpose:	Wrap Stream objects for Async Docfiles
//
//  Interface:	
//
//  History:	28-Dec-95	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

class CAsyncStream:
		public IStream,
		public IConnectionPointContainer
{
public:
    inline CAsyncStream(IStream *pstm, CFillLockBytes *pflb);
    inline ~CAsyncStream(void);

    // From IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);


    // From IStream
    STDMETHOD(Read)(VOID HUGEP *pv,
                   ULONG cb,
                   ULONG *pcbRead);
    STDMETHOD(Write)(VOID const HUGEP *pv,
                    ULONG cb,
                    ULONG *pcbWritten);
    STDMETHOD(Seek)(LARGE_INTEGER dlibMove,
                   DWORD dwOrigin,
                   ULARGE_INTEGER *plibNewPosition);
    STDMETHOD(SetSize)(ULARGE_INTEGER cb);
    STDMETHOD(CopyTo)(IStream *pstm,
                     ULARGE_INTEGER cb,
                     ULARGE_INTEGER *pcbRead,
                     ULARGE_INTEGER *pcbWritten);
    STDMETHOD(Commit)(DWORD grfCommitFlags);
    STDMETHOD(Revert)(void);
    STDMETHOD(LockRegion)(ULARGE_INTEGER libOffset,
                          ULARGE_INTEGER cb,
                          DWORD dwLockType);
    STDMETHOD(UnlockRegion)(ULARGE_INTEGER libOffset,
                            ULARGE_INTEGER cb,
                            DWORD dwLockType);
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHOD(Clone)(IStream **ppstm);
 
	//From IConnectionPointContainer
    STDMETHOD(EnumConnectionPoints)(IEnumConnectionPoints **ppEnum);
    STDMETHOD(FindConnectionPoint)(REFIID iid, IConnectionPoint **ppCP);

	SCODE Notify(void);

private:
	LONG _cReferences;
	IStream *_pRealStm;
	CFillLockBytes *_pflb;
	CConnectionPoint _cpoint;

};

inline CAsyncStream::CAsyncStream(IStream *pstm, CFillLockBytes *pflb)
{	
	_cReferences = 1;
	_pRealStm = pstm;
	_pflb = pflb;

}

inline CAsyncStream::~CAsyncStream(void)
{
	if (_pRealStm != NULL)
		_pRealStm->Release;
}
//+---------------------------------------------------------------------------
//
//  Class:		CAsyncEnumSTATSTG
//
//  Purpose:	Wrap EnumSTATSTG objects for Async Docfiles
//
//  Interface:	
//
//  History:	28-Dec-95	SusiA	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

class CAsyncEnum
    : public IEnumSTATSTG,
	  public IConnectionPointContainer
{
public:
    inline CAsyncEnum(IEnumSTATSTG *penum, CFillLockBytes *pflb);
    inline ~CAsyncEnum(void);

    // From IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IEnumSTATSTG
    STDMETHOD(Next)(ULONG celt, STATSTG FAR *rgelt, ULONG *pceltFetched);
    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset)(void);
    STDMETHOD(Clone)(IEnumSTATSTG **ppenm);

	//From IConnectionPointContainer
    STDMETHOD(EnumConnectionPoints)(IEnumConnectionPoints **ppEnum);
    STDMETHOD(FindConnectionPoint)(REFIID iid, IConnectionPoint **ppCP);

	SCODE Notify(void);
private:

	LONG _cReferences;
	IEnumSTATSTG *_pRealEnum;
	CFillLockBytes *_pflb;
	CConnectionPoint _cpoint;
};

inline CAsyncEnum::CAsyncEnum(IEnumSTATSTG *penum, CFillLockBytes *pflb)
{	
	_cReferences = 1;
	_pRealEnum = penum;
	_pflb = pflb;

}

inline CAsyncEnum::~CAsyncEnum(void)
{
	if (_pRealEnum != NULL)
		_pRealEnum->Release;
}   
#endif // #ifndef __ASYNCEXPDF_HXX__
