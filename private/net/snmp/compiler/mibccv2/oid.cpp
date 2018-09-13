#include <iostream.h>

#include <afx.h>
#include <afxtempl.h>
#include <objbase.h>
#include <afxwin.h>
#include <afxole.h>
#include <afxmt.h>
#include <wchar.h>
#include <process.h>
#include <objbase.h>
#include <initguid.h>

#include "debug.hpp"
#include "Oid.hpp"

// DESCRIPTION;
//     constructor, implicitly construct m_nOIDs and m_szOIDs
Oid::Oid()
{
}

// DESCRIPTION:
//     Adds a new Oid component to the END of the internal arrays!
// PARAMETERS:
//     (in) integer component of the Oid
//     (out) symbolic name of the component
// RETURN VALUE:
//      0 on success, -1 on failure
int Oid::AddComponent(int nOidComp, const char * szOidComp)
{
	char *szOidCopy = NULL;

	_VERIFY(m_nOidComp.Add((WORD)nOidComp)!=-1, -1);
	if (szOidComp != NULL)
	{
		szOidCopy = new char [strlen(szOidComp)+1];
		_VERIFY(szOidCopy != NULL, -1);
		strcpy(szOidCopy, szOidComp);
	}
	m_szOidComp.Add((CObject *)szOidCopy);
	return 0;
}

// DESCRIPTION:
//      Reverses the components of the OID from both
//		m_nOidComp and m_szOidComp
// RETURN VALUE:
//      0 on success, -1 on failure
int Oid::ReverseComponents()
{
	INT_PTR fwd, rev;

	for (fwd = 0, rev=m_nOidComp.GetSize()-1;
		 fwd < rev;
		 fwd ++, rev--)
	{
		int nOidComp;
		const char *szOidComp;

		nOidComp = m_nOidComp.GetAt(fwd);
		m_nOidComp.SetAt(fwd, m_nOidComp.GetAt(rev));
		m_nOidComp.SetAt(rev, (WORD)nOidComp);

		szOidComp = (const char *)m_szOidComp.GetAt(fwd);
		m_szOidComp.SetAt(fwd, m_szOidComp.GetAt(rev));
		m_szOidComp.SetAt(rev, (CObject *)szOidComp);
	}
	return 0;
}

// DESCRIPTION:
//      Output operator, displays the whole Oid
ostream& operator<< (ostream& outStream, const Oid& oid)
{
	INT_PTR sz = oid.m_nOidComp.GetSize();

	_ASSERT(sz == oid.m_szOidComp.GetSize(), "Size mismatch in Oid arrays", NULL);

	for (INT_PTR i=0; i<sz; i++)
	{
		unsigned int nId;
		const char *szId;

		// skip over the first component zero(0)
		if (i == 0)
			continue;

		nId = oid.m_nOidComp.GetAt(i);
		szId = (const char *)oid.m_szOidComp.GetAt(i);
		if (szId != NULL)
		{
			outStream << szId << "(";
			outStream << nId << ")";
		}
		else
			outStream << nId;
		if (i != sz-1)
			outStream << ".";
	}
	return outStream;
}

// DESCRIPTION:
//     destructor
Oid::~Oid()
{
	/*
	m_nOidComp.RemoveAll();
	for (int i=m_szOidComp.GetSize()-1; i>=0; i--)
	{
		char *szName = (char *)m_szOidComp.GetAt(i);
		if (szName != NULL)
		{
			// allocated with new in the AddComponent() member function
			delete szName;
		}
	}
	m_szOidComp.RemoveAll();
	*/
}
