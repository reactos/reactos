//*******************************************************************************
// COPYRIGHT NOTES
// ---------------
// You may use this source code, compile or redistribute it as part of your application 
// for free. You cannot redistribute it as a part of a software development 
// library without the agreement of the author. If the sources are 
// distributed along with the application, you should leave the original 
// copyright notes in the source code without any changes.
// This code can be used WITHOUT ANY WARRANTIES at your own risk.
// 
// For the latest updates to this code, check this site:
// http://www.masmex.com 
// after Sept 2000
// 
// Copyright(C) 2000 Philip Oldaker <email: philip@masmex.com>
//*******************************************************************************

#ifndef __SHELLDETAILS_H__
#define __SHELLDETAILS_H__

////////////////////////////////////////////////
// CShellDetails
////////////////////////////////////////////////
class CShellDetails
{
public:
	CShellDetails(const CShellDetails &rOther);
	const CShellDetails &operator=(const CShellDetails &rOther);
    CShellDetails();
    virtual ~CShellDetails();
// Attributes
	void SetShellDetails(IUnknown *pUnk);
	bool IsValidDetails();
// Operations
	HRESULT GetDetailsOf(LPCITEMIDLIST pidl,UINT iColumn,LPSHELLDETAILS pDetail);
protected:
	void FreeInterfaces();
private:
    IUnknown *m_pUnk;
};


#endif //__SHELLDETAILS_H__