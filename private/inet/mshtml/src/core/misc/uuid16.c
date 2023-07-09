#define INITGUID
#include <w4warn.h>
#include <windows.h>
#include <basetyps.h>
#include <olectlid.h>

DEFINE_GUID(IID_IRequireClasses, 0x6d5140d0, 0x7436, 0x11ce, 0x80, 0x34, 0x00, 0xaa, 0x00, 0x60, 0x09, 0xfa);
DEFINE_GUID(IID_IActiveScriptSiteInterruptPoll, 0x539698a0, 0xcdca, 0x11cf, 0xa5, 0xeb, 0x00, 0xaa, 0x00, 0x47, 0xa0, 0x63);

// These are bogus IIDs.  We still need to sync up with the standard ID values
DEFINE_OLEGUID(IID_IAdviseSinkEx, 0x00000419, 0, 0);
DEFINE_OLEGUID(IID_IOleInPlaceSiteWindowless, 0x00000408, 0, 0);
DEFINE_OLEGUID(IID_IOleInPlaceSiteEx, 0x00000417, 0, 0);
DEFINE_OLEGUID(IID_IProvideClassInfo2, 0x00000410, 0, 0);
DEFINE_OLEGUID(IID_IOleParentUndoUnit, 0x00000422, 0, 0);
DEFINE_OLEGUID(IID_IOleUndoManager, 0x0000040c, 0, 0);
DEFINE_OLEGUID(IID_IEnumOleUndoUnits, 0x00000423, 0, 0);
DEFINE_OLEGUID(IID_IOleUndoUnit, 0x00000424, 0, 0);
DEFINE_OLEGUID(IID_IViewObjectEx, 0x0000041d, 0, 0);
DEFINE_OLEGUID(IID_IQuickActivate, 0x0000040b, 0, 0);
DEFINE_OLEGUID(IID_ICategorizeProperties, 0x0000042a, 0, 0);
DEFINE_OLEGUID(IID_IOleInPlaceObjectWindowless, 0x00000416, 0, 0);
DEFINE_OLEGUID(IID_ILocalRegistry, 0x0000041e, 0, 0);
DEFINE_OLEGUID(IID_ISelectionContainer, 0x00000428, 0, 0);
DEFINE_OLEGUID(IID_ICodeNavigate, 0x00000429, 0, 0);
DEFINE_OLEGUID(IID_IObjectWithSite, 0x0000042c, 0, 0);
DEFINE_OLEGUID(IID_IPointerInactive, 0x0000042d, 0, 0);
DEFINE_OLEGUID(IID_ILicensedClassManager, 0x00000412, 0, 0);
DEFINE_OLEGUID(IID_IVideoWindow, 0x00000430, 0, 0);
DEFINE_OLEGUID(CLSID_FilterGraph, 0x00000431, 0, 0);
DEFINE_OLEGUID(IID_IGraphBuilder, 0x00000432, 0, 0);
DEFINE_OLEGUID(IID_IBasicAudio, 0x00000433, 0, 0);
DEFINE_OLEGUID(IID_IBasicVideo, 0x00000434, 0, 0);
DEFINE_OLEGUID(IID_IMediaControl, 0x00000436, 0, 0);
DEFINE_OLEGUID(IID_IMediaPosition, 0x00000437, 0, 0);
DEFINE_OLEGUID(IID_IMediaEventEx, 0x00000435, 0, 0);
DEFINE_OLEGUID(IID_IMediaEvent, 0x00000438, 0, 0);
DEFINE_OLEGUID(IID_ITypeInfo2,      0x00020412L, 0, 0);


DEFINE_OLEGUID(IID_IBoundObject,      0x00000439L, 0, 0);
DEFINE_OLEGUID(IID_IBoundObjectSite,      0x00000440L, 0, 0);
DEFINE_OLEGUID(IID_ICursor,      0x00000441L, 0, 0);
