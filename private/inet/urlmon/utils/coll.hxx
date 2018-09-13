#ifndef __COLL_HXX__
#define __COLL_HXX__

#ifdef __cplusplus

#include <map_kv.h>
//#include "clist.hxx"
#include <tchar.h>

// AFX_CDECL is used for rare functions taking variable arguments
#ifndef AFX_CDECL
        #define AFX_CDECL __cdecl
#endif

#ifdef unix
#undef ASSERT
#endif /* unix */
#ifdef ALPHA
#undef ASSERT
#endif
#define ASSERT(x)
#define VERIFY(f) (f)



class CObject;
class CString;
class CArchive;                       // object persistence tool

#include "clist.hxx"

const CString& AFXAPI AfxGetEmptyString();
#define afxEmptyString AfxGetEmptyString()


/////////////////////////////////////////////////////////////////////////////
// class CObject is the root of all compliant objects

struct CRuntimeClass
{
// Attributes
        LPCSTR m_lpszClassName;
        int m_nObjectSize;
        UINT m_wSchema; // schema number of the loaded class
        CObject* (PASCAL* m_pfnCreateObject)(); // NULL => abstract class
#ifdef _AFXDLL
        CRuntimeClass* (PASCAL* m_pfnGetBaseClass)();
#else
        CRuntimeClass* m_pBaseClass;
#endif

// Operations
        CObject* CreateObject();
        BOOL IsDerivedFrom(const CRuntimeClass* pBaseClass) const;

// Implementation
//        void Store(CArchive& ar) const;
//        static CRuntimeClass* PASCAL Load(CArchive& ar, UINT* pwSchemaNum);

        // CRuntimeClass objects linked together in simple list
        CRuntimeClass* m_pNextClass;       // linked list of registered classes
};

class CObject
{
public:

// Object model (types, destruction, allocation)
        //virtual CRuntimeClass* GetRuntimeClass() const;
        virtual ~CObject();  // virtual destructors are necessary

        // Diagnostic allocations
        void* PASCAL operator new(size_t nSize);
        void* PASCAL operator new(size_t, void* p);
        void PASCAL operator delete(void* p);

#if defined(_DEBUG) && !defined(_AFX_NO_DEBUG_CRT)
        // for file name/line number tracking using DEBUG_NEW
        void* PASCAL operator new(size_t nSize, LPCSTR lpszFileName, int nLine);
#endif

        // Disable the copy constructor and assignment by default so you will get
        //   compiler errors instead of unexpected behaviour if you pass objects
        //   by value or assign objects.
protected:
        CObject();
private:
        CObject(const CObject& objectSrc);              // no implementation
        void operator=(const CObject& objectSrc);       // no implementation

// Attributes
public:
        BOOL IsSerializable() const;
        //BOOL IsKindOf(const CRuntimeClass* pClass) const;

// Overridables
//        virtual void Serialize(CArchive& ar);

        // Diagnostic Support
        virtual void AssertValid() const;
//        virtual void Dump(CDumpContext& dc) const;

// Implementation
public:
//        static const AFX_DATA CRuntimeClass classCObject;
#ifdef _AFXDLL
        //static CRuntimeClass* PASCAL _GetBaseClass();
#endif
};

// Helper macros
//#define RUNTIME_CLASS(class_name) ((CRuntimeClass*)(&class_name::class##class_name))
#define ASSERT_KINDOF(class_name, object) \
        ASSERT((object)->IsKindOf(RUNTIME_CLASS(class_name)))

/////////////////////////////////////////////////////////////////////////////

class CString;                        // growable string type
struct CStringData
{
        long nRefs;     // reference count
        int nDataLength;
        int nAllocLength;
        // TCHAR data[nAllocLength]

        TCHAR* data()
                { return (TCHAR*)(this+1); }
};

class CString
{
public:
// Constructors
        CString();
        CString(const CString& stringSrc);
        CString(TCHAR ch, int nRepeat = 1);
        CString(LPCSTR lpsz);
        CString(LPCWSTR lpsz);
        CString(LPCTSTR lpch, int nLength);
        CString(const unsigned char* psz);

// Attributes & Operations
        // as an array of characters
        int GetLength() const;
        BOOL IsEmpty() const;
        void Empty();                       // free up the data

        TCHAR GetAt(int nIndex) const;      // 0 based
        TCHAR operator[](int nIndex) const; // same as GetAt
        void SetAt(int nIndex, TCHAR ch);
        operator LPCTSTR() const;           // as a C string

        // overloaded assignment
        const CString& operator=(const CString& stringSrc);
        const CString& operator=(TCHAR ch);
#ifdef _UNICODE
        const CString& operator=(char ch);
#endif
        const CString& operator=(LPCSTR lpsz);
        const CString& operator=(LPCWSTR lpsz);
        const CString& operator=(const unsigned char* psz);

        // string concatenation
        const CString& operator+=(const CString& string);
        const CString& operator+=(TCHAR ch);
#ifdef _UNICODE
        const CString& operator+=(char ch);
#endif
        const CString& operator+=(LPCTSTR lpsz);

        friend CString AFXAPI operator+(const CString& string1,
                        const CString& string2);
        friend CString AFXAPI operator+(const CString& string, TCHAR ch);
        friend CString AFXAPI operator+(TCHAR ch, const CString& string);
#ifdef _UNICODE
        friend CString AFXAPI operator+(const CString& string, char ch);
        friend CString AFXAPI operator+(char ch, const CString& string);
#endif
        friend CString AFXAPI operator+(const CString& string, LPCTSTR lpsz);
        friend CString AFXAPI operator+(LPCTSTR lpsz, const CString& string);

        // string comparison
        int Compare(LPCTSTR lpsz) const;         // straight character
        int CompareNoCase(LPCTSTR lpsz) const;   // ignore case
        int Collate(LPCTSTR lpsz) const;         // NLS aware

        // simple sub-string extraction
        CString Mid(int nFirst, int nCount) const;
        CString Mid(int nFirst) const;
        CString Left(int nCount) const;
        CString Right(int nCount) const;

        CString SpanIncluding(LPCTSTR lpszCharSet) const;
        CString SpanExcluding(LPCTSTR lpszCharSet) const;

        // upper/lower/reverse conversion
        void MakeUpper();
        void MakeLower();
        void MakeReverse();

        // trimming whitespace (either side)
        void TrimRight();
        void TrimLeft();

        // searching (return starting index, or -1 if not found)
        // look for a single character match
        int Find(TCHAR ch) const;               // like "C" strchr
        int ReverseFind(TCHAR ch) const;
        int FindOneOf(LPCTSTR lpszCharSet) const;

        // look for a specific sub-string
        int Find(LPCTSTR lpszSub) const;        // like "C" strstr

        // simple formatting
        void AFX_CDECL Format(LPCTSTR lpszFormat, ...);
        void AFX_CDECL Format(UINT nFormatID, ...);

        // formatting for localization (uses FormatMessage API)
        void AFX_CDECL FormatMessage(LPCTSTR lpszFormat, ...);
        void AFX_CDECL FormatMessage(UINT nFormatID, ...);

        // input and output
#ifdef _DEBUG
//        friend CDumpContext& AFXAPI operator<<(CDumpContext& dc, const CString& string);
#endif
//        friend CArchive& AFXAPI operator<<(CArchive& ar, const CString& string);
//        friend CArchive& AFXAPI operator>>(CArchive& ar, CString& string);

        // Windows support
        //BOOL LoadString(UINT nID);          // load from string resource
                                                                                // 255 chars max
#ifndef _UNICODE
        // ANSI <-> OEM support (convert string in place)
        void AnsiToOem();
        void OemToAnsi();
#endif

#ifndef _AFX_NO_BSTR_SUPPORT
        // OLE BSTR support (use for OLE automation)
        BSTR AllocSysString() const;
        BSTR SetSysString(BSTR* pbstr) const;
#endif

        // Access to string implementation buffer as "C" character array
        LPTSTR GetBuffer(int nMinBufLength);
        void ReleaseBuffer(int nNewLength = -1);
        LPTSTR GetBufferSetLength(int nNewLength);
        void FreeExtra();

        // Use LockBuffer/UnlockBuffer to turn refcounting off
        LPTSTR LockBuffer();
        void UnlockBuffer();

// Implementation
public:
        ~CString();
        int GetAllocLength() const;

protected:
        LPTSTR m_pchData;   // pointer to ref counted string data

        // implementation helpers
        CStringData* GetData() const;
        void Init();
        //{ m_pchData = afxEmptyString.m_pchData; }

        void AllocCopy(CString& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;
        void AllocBuffer(int nLen);
        void AssignCopy(int nSrcLen, LPCTSTR lpszSrcData);
        void ConcatCopy(int nSrc1Len, LPCTSTR lpszSrc1Data, int nSrc2Len, LPCTSTR lpszSrc2Data);
        void ConcatInPlace(int nSrcLen, LPCTSTR lpszSrcData);
        void FormatV(LPCTSTR lpszFormat, va_list argList);
        void CopyBeforeWrite();
        void AllocBeforeWrite(int nLen);
        void Release();
        static void PASCAL Release(CStringData* pData);
        static int PASCAL SafeStrlen(LPCTSTR lpsz);
};
// Compare helpers
inline BOOL AFXAPI operator==(const CString& s1, const CString& s2);
inline BOOL AFXAPI operator==(const CString& s1, LPCTSTR s2);
inline BOOL AFXAPI operator==(LPCTSTR s1, const CString& s2);
inline BOOL AFXAPI operator!=(const CString& s1, const CString& s2);
inline BOOL AFXAPI operator!=(const CString& s1, LPCTSTR s2);
inline BOOL AFXAPI operator!=(LPCTSTR s1, const CString& s2);
inline BOOL AFXAPI operator<(const CString& s1, const CString& s2);
inline BOOL AFXAPI operator<(const CString& s1, LPCTSTR s2);
inline BOOL AFXAPI operator<(LPCTSTR s1, const CString& s2);
inline BOOL AFXAPI operator>(const CString& s1, const CString& s2);
inline BOOL AFXAPI operator>(const CString& s1, LPCTSTR s2);
inline BOOL AFXAPI operator>(LPCTSTR s1, const CString& s2);
inline BOOL AFXAPI operator<=(const CString& s1, const CString& s2);
inline BOOL AFXAPI operator<=(const CString& s1, LPCTSTR s2);
inline BOOL AFXAPI operator<=(LPCTSTR s1, const CString& s2);
inline BOOL AFXAPI operator>=(const CString& s1, const CString& s2);
inline BOOL AFXAPI operator>=(const CString& s1, LPCTSTR s2);
inline BOOL AFXAPI operator>=(LPCTSTR s1, const CString& s2);


class CMapStringToPtr //: public CObject
{

        //DECLARE_DYNAMIC(CMapStringToPtr)
protected:
        // Association
        struct CAssoc
        {
                CAssoc* pNext;
                UINT nHashValue;  // needed for efficient iteration
                CString key;
                void* value;
        };

public:

// Construction
        CMapStringToPtr(int nBlockSize = 10);

// Attributes
        // number of elements
        int GetCount() const;
        BOOL IsEmpty() const;

        // Lookup
        BOOL Lookup(LPCTSTR key, void*& rValue) const;
        BOOL LookupKey(LPCTSTR key, LPCTSTR& rKey) const;

// Operations
        // Lookup and add if not there
        void*& operator[](LPCTSTR key);

        // add a new (key, value) pair
        void SetAt(LPCTSTR key, void* newValue);

        // removing existing (key, ?) pair
        BOOL RemoveKey(LPCTSTR key);
        void RemoveAll();

        // iterating all (key, value) pairs
        POSITION GetStartPosition() const;
        void GetNextAssoc(POSITION& rNextPosition, CString& rKey, void*& rValue) const;

        // advanced features for derived classes
        UINT GetHashTableSize() const;
        void InitHashTable(UINT hashSize, BOOL bAllocNow = TRUE);

// Overridables: special non-virtual (see map implementation for details)
        // Routine used to user-provided hash keys
        UINT HashKey(LPCTSTR key) const;

// Implementation
protected:
        CAssoc** m_pHashTable;
        UINT m_nHashTableSize;
        int m_nCount;
        CAssoc* m_pFreeList;
        struct CPlex* m_pBlocks;
        int m_nBlockSize;

        CAssoc* NewAssoc();
        void FreeAssoc(CAssoc*);
        CAssoc* GetAssocAt(LPCTSTR, UINT&) const;

public:
        ~CMapStringToPtr();
#ifdef _DEBUG
//        void Dump(CDumpContext&) const;
        void AssertValid() const;
#endif

protected:
        // local typedefs for CTypedPtrMap class template
        typedef CString BASE_KEY;
        typedef LPCTSTR BASE_ARG_KEY;
        typedef void* BASE_VALUE;
        typedef void* BASE_ARG_VALUE;
};

/////////////////////////////////////////////////////////////////////////////

class CMapStringToOb : public CObject
{

//        DECLARE_SERIAL(CMapStringToOb)
protected:
        // Association
        struct CAssoc
        {
                CAssoc* pNext;
                UINT nHashValue;  // needed for efficient iteration
                CString key;
                CObject* value;
        };

public:

// Construction
        CMapStringToOb(int nBlockSize = 10);

// Attributes
        // number of elements
        int GetCount() const;
        BOOL IsEmpty() const;

        // Lookup
        BOOL Lookup(LPCTSTR key, CObject*& rValue) const;
        BOOL LookupKey(LPCTSTR key, LPCTSTR& rKey) const;

// Operations
        // Lookup and add if not there
        CObject*& operator[](LPCTSTR key);

        // add a new (key, value) pair
        void SetAt(LPCTSTR key, CObject* newValue);

        // removing existing (key, ?) pair
        BOOL RemoveKey(LPCTSTR key);
        void RemoveAll();

        // iterating all (key, value) pairs
        POSITION GetStartPosition() const;
        void GetNextAssoc(POSITION& rNextPosition, CString& rKey, CObject*& rValue) const;

        // advanced features for derived classes
        UINT GetHashTableSize() const;
        void InitHashTable(UINT hashSize, BOOL bAllocNow = TRUE);

// Overridables: special non-virtual (see map implementation for details)
        // Routine used to user-provided hash keys
        UINT HashKey(LPCTSTR key) const;

// Implementation
protected:
        CAssoc** m_pHashTable;
        UINT m_nHashTableSize;
        int m_nCount;
        CAssoc* m_pFreeList;
        struct CPlex* m_pBlocks;
        int m_nBlockSize;

        CAssoc* NewAssoc();
        void FreeAssoc(CAssoc*);
        CAssoc* GetAssocAt(LPCTSTR, UINT&) const;

public:
        ~CMapStringToOb();

//        void Serialize(CArchive&);
#ifdef _DEBUG
//        void Dump(CDumpContext&) const;
        void AssertValid() const;
#endif

protected:
        // local typedefs for CTypedPtrMap class template
        typedef CString BASE_KEY;
        typedef LPCTSTR BASE_ARG_KEY;
        typedef CObject* BASE_VALUE;
        typedef CObject* BASE_ARG_VALUE;
};


/////////////////////////////////////////////////////////////////////////////

class CMapPtrToPtr //: public CObject
{

        //DECLARE_DYNAMIC(CMapPtrToPtr)
#ifndef unix
protected:
#else
    // If this is not made public we get:
    // "map_pp.cxx", line 114: Warning (Anachronism): CMapPtrToPtr::CAssoc is not accessible from file level.
public:
#endif /* unix */
        // Association
        struct CAssoc
        {
                CAssoc* pNext;

                void* key;
                void* value;
        };

public:

// Construction
        CMapPtrToPtr(int nBlockSize = 10);

// Attributes
        // number of elements
        int GetCount() const;
        BOOL IsEmpty() const;

        // Lookup
        BOOL Lookup(void* key, void*& rValue) const;

// Operations
        // Lookup and add if not there
        void*& operator[](void* key);

        // add a new (key, value) pair
        void SetAt(void* key, void* newValue);

        // removing existing (key, ?) pair
        BOOL RemoveKey(void* key);
        void RemoveAll();

        // iterating all (key, value) pairs
        POSITION GetStartPosition() const;
        void GetNextAssoc(POSITION& rNextPosition, void*& rKey, void*& rValue) const;

        // advanced features for derived classes
        UINT GetHashTableSize() const;
        void InitHashTable(UINT hashSize, BOOL bAllocNow = TRUE);

// Overridables: special non-virtual (see map implementation for details)
        // Routine used to user-provided hash keys
        UINT HashKey(void* key) const;

// Implementation
protected:
        CAssoc** m_pHashTable;
        UINT m_nHashTableSize;
        int m_nCount;
        CAssoc* m_pFreeList;
        struct CPlex* m_pBlocks;
        int m_nBlockSize;

        CAssoc* NewAssoc();
        void FreeAssoc(CAssoc*);
        CAssoc* GetAssocAt(void*, UINT&) const;

public:
        ~CMapPtrToPtr();
#ifdef _DEBUG
//        void Dump(CDumpContext&) const;
        void AssertValid() const;
#endif

        void* GetValueAt(void* key) const;


protected:
        // local typedefs for CTypedPtrMap class template
        typedef void* BASE_KEY;
        typedef void* BASE_ARG_KEY;
        typedef void* BASE_VALUE;
        typedef void* BASE_ARG_VALUE;
};


class CArchive
{
public:
// Flag values
        enum Mode { store = 0, load = 1, bNoFlushOnDelete = 2, bNoByteSwap = 4 };

        //CArchive(CFile* pFile, UINT nMode, int nBufSize = 4096, void* lpBuf = NULL);
        ~CArchive();
// Attributes
#ifndef unix
        BOOL IsLoading() const;
        BOOL IsStoring() const;
        BOOL IsByteSwapping() const;
        BOOL IsBufferEmpty() const;
#else
// In UNIX all the member functions which are defined here somehow
// referenced by notifictn/notifhlp.cxx. WIthout these we get undefined symbol
// references on UNIX. So I added these. -Pravin. 
        BOOL IsLoading() const { return TRUE; }
        BOOL IsStoring() const { return TRUE; }
        BOOL IsByteSwapping() const { return TRUE;}
        BOOL IsBufferEmpty() const { return TRUE; }
#endif /* !unix */
        //CFile* GetFile() const;
        UINT GetObjectSchema(); // only valid when reading a CObject*
        void SetObjectSchema(UINT nSchema);

        // pointer to document being serialized -- must set to serialize
        //  COleClientItems in a document!
        //CDocument* m_pDocument;

// Operations
#ifndef unix
        UINT Read(void* lpBuf, UINT nMax);
        void Write(const void* lpBuf, UINT nMax);
#else
        UINT Read(void* lpBuf, UINT nMax) { return 0; }
        void Write(const void* lpBuf, UINT nMax) { return; }
#endif /* !unix */
        void Flush();
        void Close();
        void Abort();   // close and shutdown without exceptions

        // reading and writing strings
        void WriteString(LPCTSTR lpsz);
        LPTSTR ReadString(LPTSTR lpsz, UINT nMax);
        BOOL ReadString(CString& rString);

public:
        // Object I/O is pointer based to avoid added construction overhead.
        // Use the Serialize member function directly for embedded objects.
        friend CArchive& AFXAPI operator<<(CArchive& ar, const CObject* pOb);

        friend CArchive& AFXAPI operator>>(CArchive& ar, CObject*& pOb);
        friend CArchive& AFXAPI operator>>(CArchive& ar, const CObject*& pOb);

        // insertion operations
        CArchive& operator<<(BYTE by);
        CArchive& operator<<(WORD w);
        CArchive& operator<<(LONG l);
        CArchive& operator<<(DWORD dw);
        CArchive& operator<<(float f);
        CArchive& operator<<(double d);

        CArchive& operator<<(int i);
        CArchive& operator<<(short w);
        CArchive& operator<<(char ch);
        CArchive& operator<<(unsigned u);

        // extraction operations
        CArchive& operator>>(BYTE& by);
        CArchive& operator>>(WORD& w);
        CArchive& operator>>(DWORD& dw);
        CArchive& operator>>(LONG& l);
        CArchive& operator>>(float& f);
        CArchive& operator>>(double& d);

        CArchive& operator>>(int& i);
        CArchive& operator>>(short& w);
        CArchive& operator>>(char& ch);
        CArchive& operator>>(unsigned& u);

        // object read/write
        CObject* ReadObject(const CRuntimeClass* pClass);
        void WriteObject(const CObject* pOb);
        // advanced object mapping (used for forced references)
        void MapObject(const CObject* pOb);

        // advanced versioning support
        void WriteClass(const CRuntimeClass* pClassRef);
        CRuntimeClass* ReadClass(const CRuntimeClass* pClassRefRequested = NULL,
                UINT* pSchema = NULL, DWORD* pObTag = NULL);
        void SerializeClass(const CRuntimeClass* pClassRef);

        // advanced operations (used when storing/loading many objects)
        void SetStoreParams(UINT nHashSize = 2053, UINT nBlockSize = 128);
        void SetLoadParams(UINT nGrowBy = 1024);

// Implementation
public:
        BOOL m_bForceFlat;  // for COleClientItem implementation (default TRUE)
        BOOL m_bDirectBuffer;   // TRUE if m_pFile supports direct buffering
        void FillBuffer(UINT nBytesNeeded);
        void CheckCount();  // throw exception if m_nMapCount is too large

        // special functions for reading and writing (16-bit compatible) counts
#ifndef unix
        DWORD ReadCount();
        void WriteCount(DWORD dwCount);
#else
        DWORD ReadCount() { return 0; }
        void WriteCount(DWORD dwCount) { return; }
#endif /* !unix */

        // public for advanced use
        UINT m_nObjectSchema;
        CString m_strFileName;

protected:
        // archive objects cannot be copied or assigned
        CArchive(const CArchive& arSrc);
        void operator=(const CArchive& arSrc);

        BOOL m_nMode;
        BOOL m_bUserBuf;
        int m_nBufSize;
        //CFile* m_pFile;
        BYTE* m_lpBufCur;
        BYTE* m_lpBufMax;
        BYTE* m_lpBufStart;

        // array/map for CObject* and CRuntimeClass* load/store
        UINT m_nMapCount;
        union
        {
                //CPtrArray* m_pLoadArray;
                CMapPtrToPtr* m_pStoreMap;
        };
        // map to keep track of mismatched schemas
        CMapPtrToPtr* m_pSchemaMap;

        // advanced parameters (controls performance with large archives)
        UINT m_nGrowSize;
        UINT m_nHashSize;
};


/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

#define  _AFX_ENABLE_INLINES

#ifdef _AFX_ENABLE_INLINES
#define _AFXCOLL_INLINE inline
#define _AFX_INLINE inline
#include "collinl.hxx"
#endif

#include "afxtempl.h"

#endif // __cplusplus


#endif // __COLL_HXX__

 
