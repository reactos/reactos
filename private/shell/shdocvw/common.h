#ifndef _COMMON_H
#define _COMMON_H

#if defined(__cplusplus) && !defined(CINTERFACE)
#define QueryInterface(punk, iid, pobj)	(punk)->QueryInterface(iid, pobj)
#define AddRef(punk)			(punk)->AddRef()
#define Release(punk)			(punk)->Release()
#else  /* CINTERFACE */
#define QueryInterface(punk, iid, pobj) (punk)->lpVtbl->QueryInterface(punk, iid, pobj)
#define AddRef(punk)                    (punk)->lpVtbl->AddRef(punk)
#define Release(punk)                   (punk)->lpVtbl->Release(punk)
#endif /* CINTERFACE */

#ifdef _DEBUG

#define ReleaseLast(punk)   Assert(Release(punk) == 0)

#else

#define ReleaseLast(punk)   Release(punk)

#endif  // _DEBUG

#endif  // _COMMON_H
