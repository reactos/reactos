//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       map_dwp.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-29-1996   JohannP (Johann Posch)   taken from OLE
//
//----------------------------------------------------------------------------


class FAR CMapDwordPtr
{
public:
   // Construction
   CMapDwordPtr(UINT nBlockSize=10)
        : m_mkv(sizeof(void FAR*), sizeof(DWORD), nBlockSize) { }

   // Attributes
   // number of elements
   int     GetCount() const
                        { return m_mkv.GetCount(); }
   BOOL    IsEmpty() const
                        { return GetCount() == 0; }

   // Lookup
   BOOL    Lookup(DWORD key, void FAR* FAR& value) const
                        { return m_mkv.Lookup((LPVOID) &key, sizeof(DWORD), (LPVOID)&value); }

   BOOL    LookupHKey(HMAPKEY hKey, void FAR* FAR& value) const
                        { return m_mkv.LookupHKey(hKey, (LPVOID)&value); }

   BOOL    LookupAdd(DWORD key, void FAR* FAR& value) const
                        { return m_mkv.LookupAdd((LPVOID)&key, sizeof(DWORD), (LPVOID)&value); }

   // Add/Delete
   // add a new (key, value) pair
   BOOL    SetAt(DWORD key, void FAR* value)
                        { return m_mkv.SetAt((LPVOID) &key, sizeof(DWORD), (LPVOID)&value); }
   BOOL    SetAtHKey(HMAPKEY hKey, void FAR* value)
                        { return m_mkv.SetAtHKey(hKey, (LPVOID)&value); }

   // removing existing (key, ?) pair
   BOOL    RemoveKey(DWORD key)
                        { return m_mkv.RemoveKey((LPVOID) &key, sizeof(DWORD)); }

   BOOL    RemoveHKey(HMAPKEY hKey)
                        { return m_mkv.RemoveHKey(hKey); }

   void    RemoveAll()
                        { m_mkv.RemoveAll(); }


   // iterating all (key, value) pairs
   POSITION GetStartPosition() const
                        { return m_mkv.GetStartPosition(); }

   void    GetNextAssoc(POSITION FAR& rNextPosition, DWORD FAR& rKey, void FAR* FAR& rValue) const
                        { m_mkv.GetNextAssoc(&rNextPosition, (LPVOID)&rKey, NULL, (LPVOID)&rValue); }

   HMAPKEY GetHKey(DWORD key) const
                        { return m_mkv.GetHKey((LPVOID)&key, sizeof(DWORD)); }

#if DBG==1
   void    AssertValid() const
                        { m_mkv.AssertValid(); }
#endif

private:
   CMapKeyToValue m_mkv;
};
