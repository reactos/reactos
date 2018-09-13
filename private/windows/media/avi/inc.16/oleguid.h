/* this file is the master definition of all public GUIDs specific to OLE 
   and is included in ole2.h.
   
   NOTE: The second least significant byte of all of these GUIDs is 1.
*/
   

DEFINE_OLEGUID(IID_IEnumUnknown,            0x00000100, 0, 0);
DEFINE_OLEGUID(IID_IEnumString,             0x00000101, 0, 0);
DEFINE_OLEGUID(IID_IEnumMoniker,            0x00000102, 0, 0);
DEFINE_OLEGUID(IID_IEnumFORMATETC,          0x00000103, 0, 0);
DEFINE_OLEGUID(IID_IEnumOLEVERB,            0x00000104, 0, 0);
DEFINE_OLEGUID(IID_IEnumSTATDATA,           0x00000105, 0, 0);

DEFINE_OLEGUID(IID_IEnumGeneric,            0x00000106, 0, 0);
DEFINE_OLEGUID(IID_IEnumHolder,             0x00000107, 0, 0);
DEFINE_OLEGUID(IID_IEnumCallback,           0x00000108, 0, 0);

DEFINE_OLEGUID(IID_IPersistStream,          0x00000109, 0, 0);
DEFINE_OLEGUID(IID_IPersistStorage,         0x0000010a, 0, 0);
DEFINE_OLEGUID(IID_IPersistFile,            0x0000010b, 0, 0);
DEFINE_OLEGUID(IID_IPersist,                0x0000010c, 0, 0);

DEFINE_OLEGUID(IID_IViewObject,             0x0000010d, 0, 0);
DEFINE_OLEGUID(IID_IDataObject,             0x0000010e, 0, 0);
DEFINE_OLEGUID(IID_IAdviseSink,             0x0000010f, 0, 0);
DEFINE_OLEGUID(IID_IDataAdviseHolder,       0x00000110, 0, 0);
DEFINE_OLEGUID(IID_IOleAdviseHolder,        0x00000111, 0, 0);

DEFINE_OLEGUID(IID_IOleObject,              0x00000112, 0, 0);
DEFINE_OLEGUID(IID_IOleInPlaceObject,       0x00000113, 0, 0);
DEFINE_OLEGUID(IID_IOleWindow,              0x00000114, 0, 0);
DEFINE_OLEGUID(IID_IOleInPlaceUIWindow,     0x00000115, 0, 0);
DEFINE_OLEGUID(IID_IOleInPlaceFrame,        0x00000116, 0, 0);
DEFINE_OLEGUID(IID_IOleInPlaceActiveObject, 0x00000117, 0, 0);

DEFINE_OLEGUID(IID_IOleClientSite,          0x00000118, 0, 0);
DEFINE_OLEGUID(IID_IOleInPlaceSite,         0x00000119, 0, 0);

DEFINE_OLEGUID(IID_IParseDisplayName,       0x0000011a, 0, 0);
DEFINE_OLEGUID(IID_IOleContainer,           0x0000011b, 0, 0);
DEFINE_OLEGUID(IID_IOleItemContainer,       0x0000011c, 0, 0);

DEFINE_OLEGUID(IID_IOleLink,                0x0000011d, 0, 0);
DEFINE_OLEGUID(IID_IOleCache,               0x0000011e, 0, 0);
DEFINE_OLEGUID(IID_IOleManager,             0x0000011f, 0, 0);
DEFINE_OLEGUID(IID_IOlePresObj,             0x00000120, 0, 0);

DEFINE_OLEGUID(IID_IDropSource,             0x00000121, 0, 0);
DEFINE_OLEGUID(IID_IDropTarget,             0x00000122, 0, 0);

DEFINE_OLEGUID(IID_IDebug,                  0x00000123, 0, 0);
DEFINE_OLEGUID(IID_IDebugStream,            0x00000124, 0, 0);

DEFINE_OLEGUID(IID_IAdviseSink2,            0x00000125, 0, 0);


/* NOTE: LSB values 0x26 through 0xff are reserved */


/* GUIDs defined in OLE's private range */
DEFINE_OLEGUID(CLSID_StaticMetafile,        0x00000315, 0, 0);
DEFINE_OLEGUID(CLSID_StaticDib,             0x00000316, 0, 0);


