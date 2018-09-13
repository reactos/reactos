// key.h : header file
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

/////////////////////////////////////////////////////////////////////////////
// CKey 

class CKey
{
public:
	CKey() {m_hKey = NULL;}
	~CKey() {Close();}

// Attributes
public:
	HKEY m_hKey;
	BOOL SetStringValue(LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL);
	BOOL GetStringValue(CString& str, LPCTSTR lpszValueName = NULL);

// Operations
public:
	BOOL Create(HKEY hKey, LPCTSTR lpszKeyName);
	BOOL Open(HKEY hKey, LPCTSTR lpszKeyName);
	void Close();

// Overrides

// Implementation
protected:
};

/////////////////////////////////////////////////////////////////////////////
