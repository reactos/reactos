//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  Class:      CMshtmlEdFactory
//
//  Contents:   Class Factory Definition for Mshtmled
//
//  History:    7-Jan-98   raminh  Created
//----------------------------------------------------------------------------


MtExtern(CMshtmlEdFactory)

class CMshtmlEdFactory : public IClassFactory
{
public:

    // IUnknown methods
    DECLARE_FORMS_IUNKNOWN_METHODS ;

	// IClassFactory methods
	STDMETHOD (CreateInstance)(IUnknown* pUnknownOuter,
	                                         const IID& iid,
	                                         void** ppv) ;
	STDMETHOD (LockServer)(BOOL bLock) ; 

	// Constructor
	CMshtmlEdFactory() : _ulRefs(1) {}

	// Destructor
	~CMshtmlEdFactory() {}

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMshtmlEdFactory));

private:
	long _ulRefs ;
} ;

