/* this file is the master definition of all GUIDs for the component object
   model and is included in compobj.h.  Some GUIDs for moinkers and storage 
   appear here as well.  All of these GUIDs are OLE GUIDs only in the sense 
   that part of the GUID range owned by OLE was used to define them.  
   
   NOTE: The second byte of all of these GUIDs is 0.
*/
   

DEFINE_GUID(GUID_NULL, 0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

DEFINE_OLEGUID(IID_IUnknown,            0x00000000L, 0, 0);
DEFINE_OLEGUID(IID_IClassFactory,       0x00000001L, 0, 0);
DEFINE_OLEGUID(IID_IMalloc,             0x00000002L, 0, 0);
DEFINE_OLEGUID(IID_IMarshal,            0x00000003L, 0, 0);

/* RPC related interfaces */
DEFINE_OLEGUID(IID_IRpcChannel,         0x00000004L, 0, 0);
DEFINE_OLEGUID(IID_IRpcStub,            0x00000005L, 0, 0);
DEFINE_OLEGUID(IID_IStubManager,        0x00000006L, 0, 0);
DEFINE_OLEGUID(IID_IRpcProxy,           0x00000007L, 0, 0);
DEFINE_OLEGUID(IID_IProxyManager,       0x00000008L, 0, 0);
DEFINE_OLEGUID(IID_IPSFactory,          0x00000009L, 0, 0);

/* storage related interfaces */
DEFINE_OLEGUID(IID_ILockBytes,          0x0000000aL, 0, 0);
DEFINE_OLEGUID(IID_IStorage,            0x0000000bL, 0, 0);
DEFINE_OLEGUID(IID_IStream,             0x0000000cL, 0, 0);
DEFINE_OLEGUID(IID_IEnumSTATSTG,        0x0000000dL, 0, 0);

/* moniker related interfaces */
DEFINE_OLEGUID(IID_IBindCtx,            0x0000000eL, 0, 0);
DEFINE_OLEGUID(IID_IMoniker,            0x0000000fL, 0, 0);
DEFINE_OLEGUID(IID_IRunningObjectTable, 0x00000010L, 0, 0);
DEFINE_OLEGUID(IID_IInternalMoniker,    0x00000011L, 0, 0);

/* storage related interfaces */
DEFINE_OLEGUID(IID_IRootStorage,        0x00000012L, 0, 0);
DEFINE_OLEGUID(IID_IDfReserved1,        0x00000013L, 0, 0);
DEFINE_OLEGUID(IID_IDfReserved2,        0x00000014L, 0, 0);
DEFINE_OLEGUID(IID_IDfReserved3,        0x00000015L, 0, 0);

/* concurrency releated interfaces */
DEFINE_OLEGUID(IID_IMessageFilter,      0x00000016L, 0, 0);

/* CLSID of standard marshaler */
DEFINE_OLEGUID(CLSID_StdMarshal,        0x00000017L, 0, 0);

/* interface on server for getting info for std marshaler */
DEFINE_OLEGUID(IID_IStdMarshalInfo,     0x00000018L, 0, 0);

/* NOTE: LSB 0x19 through 0xff are reserved for future use */
