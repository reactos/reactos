#ifndef _OBJFWD_H
#define _OBJFWD_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#include <basetyps.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef interface IMoniker *LPMONIKER;
typedef interface IStream *LPSTREAM;
typedef interface IMarshal *LPMARSHAL;
typedef interface IMalloc *LPMALLOC;
typedef interface IMallocSpy *LPMALLOCSPY;
typedef interface IMessageFilter *LPMESSAGEFILTER;
typedef interface IPersist *LPPERSIST;
typedef interface IPersistStream *LPPERSISTSTREAM;
typedef interface IRunningObjectTable *LPRUNNINGOBJECTTABLE;
typedef interface IBindCtx *LPBINDCTX,*LPBC;
typedef interface IAdviseSink *LPADVISESINK;
typedef interface IAdviseSink2 *LPADVISESINK2;
typedef interface IDataObject *LPDATAOBJECT;
typedef interface IDataAdviseHolder *LPDATAADVISEHOLDER;
typedef interface IEnumMoniker *LPENUMMONIKER;
typedef interface IEnumFORMATETC *LPENUMFORMATETC;
typedef interface IEnumSTATDATA *LPENUMSTATDATA;
typedef interface IEnumSTATSTG *LPENUMSTATSTG;
typedef interface IEnumSTATPROPSTG LPENUMSTATPROPSTG;
typedef interface IEnumString *LPENUMSTRING;
typedef interface IEnumUnknown *LPENUMUNKNOWN;
typedef interface IStorage *LPSTORAGE;
typedef interface IPersistStorage *LPPERSISTSTORAGE;
typedef interface ILockBytes *LPLOCKBYTES;
typedef interface IStdMarshalInfo *LPSTDMARSHALINFO;
typedef interface IExternalConnection *LPEXTERNALCONNECTION;
typedef interface IRunnableObject *LPRUNNABLEOBJECT;
typedef interface IROTData *LPROTDATA;
typedef interface IPersistFile *LPPERSISTFILE;
typedef interface IRootStorage *LPROOTSTORAGE;
typedef interface IRpcChannelBuffer *LPRPCCHANNELBUFFER;
typedef interface IRpcProxyBuffer *LPRPCPROXYBUFFER;
typedef interface IRpcStubBuffer *LPRPCSTUBBUFFER;
typedef interface IPropertyStorage *LPPROPERTYSTORAGE;
typedef interface IEnumSTATPROPSETSTG *LPENUMSTATPROPSETSTG;
typedef interface IPropertySetStorage *LPPROPERTYSETSTORAGE;
typedef interface IClientSecurity *LPCLIENTSECURITY;
typedef interface IServerSecurity *LPSERVERSECURITY;
typedef interface IClassActivator *LPCLASSACTIVATOR;
typedef interface IFillLockBytes *LPFILLLOCKBYTES;
typedef interface IProgressNotify *LPPROGRESSNOTIFY;
typedef interface ILayoutStorage *LPLAYOUTSTORAGE;
#ifdef __cplusplus
}
#endif
#endif
