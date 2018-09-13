#ifndef __EXTOBJ_H
#define __EXTOBJ_H
////
//
// ExpandoObject header file
//
//
//
#include "IPServer.H"

////
//
// IDispatchEx
//
////

////
//
// the GUID
//

// {A0AAC450-A77B-11cf-91D0-00AA00C14A7C}
DEFINE_GUID(IID_IDispatchEx, 0xa0aac450, 0xa77b, 0x11cf, 0x91, 0xd0, 0x0, 0xaa, 0x0, 0xc1, 0x4a, 0x7c);

////
//
// IDispatchEx flags:
//

enum
{
	fdexNil = 0x00,				// empty
	fdexDontCreate = 0x01,		// don't create slot if non-existant otherwise do
	fdexInitNull = 0x02,		// init a new slot to VT_NULL as opposed to VT_EMPTY
	fdexCaseSensitive = 0x04,	// match names as case sensitive
};

////
//
// This is the interface for extensible IDispatch objects.
//

class IDispatchEx : public IDispatch
{
public:
	// Get dispID for names, with options
	virtual HRESULT STDMETHODCALLTYPE GetIDsOfNamesEx(
		REFIID riid,
		LPOLESTR *prgpsz,
		UINT cpsz,
		LCID lcid,
		DISPID *prgid,
		DWORD grfdex
	) = 0;

	// Enumerate dispIDs and their associated "names".
	// Returns S_FALSE if the enumeration is done, NOERROR if it's not, an
	// error code if the call fails.
	virtual HRESULT STDMETHODCALLTYPE GetNextDispID(
		DISPID id,
		DISPID *pid,
		BSTR *pbstrName
	) = 0;
};

////
//
// Globals and definitions
//
////

#define NUM_EXPANDO_DISPIDS		250
#define	NUM_CORE_DISPIDS		250
#define NUM_RESERVED_EXTENDER_DISPIDS (NUM_CORE_DISPIDS + NUM_EXPANDO_DISPIDS)
#define EXTENDER_DISPID_BASE ((ULONG)(0x80010000))
#define IS_EXTENDER_DISPID(x) ( ( (ULONG)(x) & 0xFFFF0000 ) == EXTENDER_DISPID_BASE )

////
//
// Slot: the state of a value slot
//

inline WCHAR ToUpper(WCHAR ch)   
{
	if (ch>='a' && ch <= 'z')  
		return ch - 'a' + 'A';
	else
		return ch;

}

class CExpandoObjectSlot
{
public:
	////
	//
	// Constructor/Destructor
	//

	// because these monsters are malloc'ed, we need a manual constructor and destructor methods
	void Construct()
	{
		m_name = NULL;
		m_next = -1;
		VariantInit(&m_value);
		// set hash and dispId to dummy values
		m_hash = 0;
		m_dispId = DISPID_UNKNOWN;
	}

	void Destruct()
	{
		if (m_name)
			SysFreeString(m_name);
		VariantClear(&m_value);
	}

private:
	// the constructors and destructors are private because they should never be called ...
	// we could use in-place construction if we wanted to be clever ...
	CExpandoObjectSlot()
	{
	}

	~CExpandoObjectSlot()
	{
	}

public:
	////
	//
	// Init the slot
	//

	HRESULT Init(LPOLESTR name, LCID lcid, DISPID dispId, VARIANT* value)
	{
		// allocate the string
		m_name = SysAllocString(name);
		if (m_name == NULL)
			return E_OUTOFMEMORY;

		// compute the hash: uses the standard OLE string hashing function
		// note that this function is case insensitive
		m_hash = LHashValOfName(lcid, name);

		// set the dispId
		m_dispId = dispId;

		// Copy the variant value
		return VariantCopy(&m_value, value);
	}

	////
	//
	// Name information
	//

	// get the name
	BSTR Name()
	{ return m_name; }

	// compare two names
	BOOL CompareName(LPOLESTR name, ULONG hash, BOOL caseSensitive)
	{

		// hash should be the same, length should be the same, and strings should compare
		// BUGBUG robwell 8May96 These functions are probably verboten.
		if (hash != m_hash)
			return FALSE;

		if (!name)
			return !m_name;

		WCHAR *c1 = name;
		WCHAR *c2 = m_name;

		// Travel down both strings until we reach a mismatched character
		// or the end of one (or both) of the strings

		if (caseSensitive)
			while (*c1 && *c2 && *c1++==*c2++);
		else
			while (*c1 && *c2 && ToUpper(*c1++)==ToUpper(*c2++));

		// The strings match if we reached the end of both without a mismatch
		return !*c1 && !*c2;
 	}

	////
	//
	// DispId information
	//

	// get the dispatch id
	DISPID DispId()
	{ return m_dispId; }

	////
	//
	// Get and set the property values
	//

	HRESULT Get(VARIANT* result)
	{ return VariantCopy(result, &m_value); }

	HRESULT Set(VARIANT* value)
	{ return VariantCopy(&m_value, value); }

	////
	//
	// List management
	//

	CExpandoObjectSlot* Next(CExpandoObjectSlot* base)
	{ return m_next == -1? NULL: &base[m_next]; }

	CExpandoObjectSlot* Insert(CExpandoObjectSlot* base, LONG& prev)
	{
		m_next = prev;
		prev = (LONG)(this - base);
		return this;
	}

private:
	// the DispId
	DISPID		m_dispId;
	// the name
	LPOLESTR	m_name;
	// the name hash
	ULONG		m_hash;
	// the property value
	VARIANT		m_value;
	// the hash bucket link (index based)
	LONG		m_next;
};

// NB: CExpandoObject implements a crippled version of aggegation.
// It delegates all IUnknown calls to its controlling IUnknown, and has no
// private IUnknown interface.
// If you want the CExpandoObject to go away, simply call delete on it.
class CExpandoObject: public IDispatchEx
{
public:

	////
	//
	// Constructor/Destructor
	//

	CExpandoObject(IUnknown *punkOuter, IDispatch *pdisp, ULONG dispIdBase = EXTENDER_DISPID_BASE + NUM_CORE_DISPIDS) 
	{
		// remember our controlling outer
		m_punkOuter = punkOuter;

		// remember the IDispatch to try first for IDispatch functionality
		m_pdisp = pdisp;
		
		// clear the name hash table
		ClearHashTable();
		// set the total slots and the table of slots to 0 and empty respectively)
		m_totalSlots = 0;
		m_slotTableSize = 0;
		m_slots = NULL;
		m_dispIdBase = dispIdBase;
	}

	STDMETHODIMP_(ULONG) AddRef()
	{
		return m_punkOuter->AddRef();
	}

	STDMETHODIMP_(ULONG)Release()
	{
		return m_punkOuter->Release();
	}

    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObjOut)
	{
		return m_punkOuter->QueryInterface(riid, ppvObjOut);
	}

    virtual ~CExpandoObject(void)
	{
		FreeAllSlots();
	}


    // Copy all of the properties from obj 
   	CloneProperties(CExpandoObject& obj);

	////
	//
	//
	// Utility functions
	//

	// free all slots
	void FreeAllSlots();

	// IDispatch methods
	virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);
	virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(
		UINT itinfo,
		LCID lcid,
		ITypeInfo **pptinfo
	);
	virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(
		REFIID riid,
		LPOLESTR *prgpsz,
		UINT cpsz,
		LCID lcid,
		DISPID *prgdispID
	);
	virtual HRESULT STDMETHODCALLTYPE Invoke(
		DISPID dispID,
		REFIID riid,
		LCID lcid,
		WORD wFlags,
		DISPPARAMS *pdispparams,
		VARIANT *pvarRes,
		EXCEPINFO *pexcepinfo,
		UINT *puArgErr
	);

	// IDispatchEx methods

	// Get dispID for names, with options
	virtual HRESULT STDMETHODCALLTYPE GetIDsOfNamesEx(
		REFIID riid,
		LPOLESTR *prgpsz,
		UINT cpsz,
		LCID lcid,
		DISPID *prgid,
		DWORD grfdex
	);

	// Enumerate dispIDs and their associated "names".
	// Returns S_FALSE if the enumeration is done, NOERROR if it's not, an
	// error code if the call fails.
	virtual HRESULT STDMETHODCALLTYPE GetNextDispID(
		DISPID id,
		DISPID *pid,
		BSTR *pbstrName
	);

private:
	////
	//
	// Implementation constants
	//

	enum
	{
		kSlotHashTableSize = 10,
		kInitialSlotTableSize = 4,
		kMaxTotalSlots = NUM_EXPANDO_DISPIDS
	};

	////
	//
	// Utility functions
	//

	//
	CExpandoObjectSlot* GetHashTableHead(UINT hashIndex)
	{
		LONG index;

		return (index = m_hashTable[hashIndex]) == -1? NULL: &m_slots[index];
	}

	// get the ID of from a slot name
	HRESULT GetIDOfName(LPOLESTR name, LCID lcid, BOOL caseSensitive, DISPID* id);
	// add a slot to the object
	HRESULT AddSlot(LPOLESTR name, LCID lcid, BOOL caseSensitive, VARIANT* initialValue, DISPID* id);
	// allocate a slot from the slot table
	CExpandoObjectSlot* AllocSlot();
	// clear the hash table
	void ClearHashTable()
	{
		UINT i;

		for (i=0; i<kSlotHashTableSize; ++i)
			m_hashTable[i] = -1;
	}

	////
	//
	// Slot operations
	//
	// DISPIDS start at kInitialDispId so we need to offset them by that amount
	// in this code.
	//

	HRESULT GetSlot(DISPID id, VARIANT* result)
	{
		if ((ULONG) id < m_dispIdBase || (ULONG) id >= (m_totalSlots+m_dispIdBase))
			return DISP_E_MEMBERNOTFOUND;

		return m_slots[id-m_dispIdBase].Get(result);
	}

	HRESULT SetSlot(DISPID id, VARIANT* result)
	{
		if ((ULONG) id < m_dispIdBase || (ULONG) id >= (m_totalSlots+m_dispIdBase))
			return DISP_E_MEMBERNOTFOUND;

		return m_slots[id-m_dispIdBase].Set(result);
	}

	////
	//
	// Iteration operations
	//

	UINT	NumDispIds()
	{ return m_totalSlots; }

	DISPID	First()
	{ return m_dispIdBase; }

	DISPID	Last()
	{ return m_totalSlots + m_dispIdBase - 1; }

	BOOL	ValidDispId(DISPID id)
	{ return id >= First() && id <= Last(); }

	HRESULT	Next(DISPID key, CExpandoObjectSlot*& slot)
	{
		// zero restarts the enumerator
		if (key == 0)
		{
			// if there are no slots we are done
			if (NumDispIds() == 0)
				return S_FALSE;

			// return the first slot
			slot = &m_slots[0];
			return NOERROR;
		}
		else
		if (key == Last())
		{
			// the key was the last slot so we are done
			return S_FALSE;
		}
		else
		if (ValidDispId(key))
		{
			// return the next slot
			slot = &m_slots[key-m_dispIdBase+1];
			return NOERROR;
		}
		else
			// the key must be invalid
			return E_INVALIDARG;
	}

	////
	//
	// The local state of the object
	//

	// the objects reference count
	ULONG	m_ref;

	// the base of objectIds
	ULONG	m_dispIdBase;

	// the hash table of slots - for fast GetIDSofNames lookup
	LONG	m_hashTable[kSlotHashTableSize];

	// the number of slots (and the next dispId to allocate)
	UINT	m_totalSlots;

	// the size of the allocated array of slots
	UINT	m_slotTableSize;

	// a pointer to the allocated array of slots
	CExpandoObjectSlot* m_slots;

	// controlling unknown
	IUnknown *m_punkOuter;

	// controlling IDispatch
	IDispatch *m_pdisp;
};

#endif // __EXTOBJ_H
