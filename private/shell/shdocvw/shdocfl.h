//
//  shdocfl.h 
//  
//  helper functions that are useful outside of shdocfl.cpp
//  but are definitely tied to the implementation of the namespace
//
//  WARNING - this should not be abused  - ZekeL - 24-NOV-98
//      by exposing the internal structure of the pidls
//


STDAPI_(LPITEMIDLIST) IEILAppendFragment(LPITEMIDLIST pidl, LPCWSTR pszFragment);
STDAPI_(BOOL) IEILGetFragment(LPCITEMIDLIST pidl, LPWSTR pszFragment, DWORD cchFragment);
STDAPI_(UINT) IEILGetCP(LPCITEMIDLIST pidl);

