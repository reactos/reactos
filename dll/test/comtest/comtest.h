#include <windows.h>
#include <shlobj.h>
#include <shlobj_undoc.h>
#include <shlguid.h>
#include <shlguid_undoc.h>
#include <shlwapi.h>
#include <tchar.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include "iComTest.h"
//#include "iComTest.h"
//#include "iComTest_i.c"
//#include "iComTest_p.c"

#define IDR_COMTEST                   101


// {C4CC78DB-8266-4ae7-8685-8EE05376CB09}
DEFINE_GUID(CLSID_ComTest, 0xc4cc78db, 0x8266, 0x4ae7, 0x86, 0x85, 0x8e, 0xe0, 0x53, 0x76, 0xcb, 0x9);
//static const GUID CLSID_ComTest = {0xc4cc78db,0x8266,0x4ae7,{0x86,0x85,0x8e,0xe0,0x53,0x76,0xcb,0x9}};


//const IID IID_IComTest = {0x10CDF249,0xA336,0x406F,{0xB4,0x72,0x20,0xF0,0x86,0x60,0xD6,0x09}};


class ComTestBase :
	public CComObjectRootEx<CComMultiThreadModelNoCS>,
	public IComTest	
{
	public:
	ComTestBase();
	~ComTestBase();

	// *** IComTest methods ***
	virtual HRESULT STDMETHODCALLTYPE test(void);


	BEGIN_COM_MAP(ComTestBase)
		COM_INTERFACE_ENTRY_IID(IID_IComTest, IComTest)
	END_COM_MAP()
};

class ComTest :
	public CComCoClass<ComTest, &CLSID_ComTest>,
	public ComTestBase
{
	public:
	DECLARE_REGISTRY_RESOURCEID(IDR_COMTEST)
	DECLARE_NOT_AGGREGATABLE(ComTest)

	DECLARE_PROTECT_FINAL_CONSTRUCT()
};