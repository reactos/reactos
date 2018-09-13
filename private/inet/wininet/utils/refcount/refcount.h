//=--------------------------------------------------------------------------=
//  (C) Copyright 1997-1998 Microsoft Corporation. All Rights Reserved.
//	TriEdit SDK team
//	Author: Yury Polyakovsky
//	contact: a-yurip@microsof.com
//=--------------------------------------------------------------------------=
class CRefCount
{
public:
	CRefCount() {m_dwRefCount = 1;}
	void SetInstalFlag(BOOL flag) {m_fInstall = flag;}
	void Change(char *szName, PHKEY phkRef);
	BOOL ValueExist(char *sz_RegSubkey, char *sz_RegValue);
	void ValueGet(char *sz_RegSubkey, char *sz_ValueName, LPBYTE *p_Value, DWORD *pdwValueSize);
	void ValueSet(char *sz_RegSubkey, char *sz_RegValue);
	void ValueClear(char *sz_RegSubkey, char *sz_RegValue);
	DWORD GetCount() { return m_dwRefCount;}
private:
	BOOL m_fInstall;
	DWORD m_dwRefCount;
};
