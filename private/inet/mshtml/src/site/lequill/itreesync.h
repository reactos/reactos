
#ifndef _itreesync_H
#define _itreesync_H

#pragma once

extern "C"{

#ifndef __ITreeSyncBehavior_FWD_DEFINED__
#define __ITreeSyncBehavior_FWD_DEFINED__
typedef interface ITreeSyncBehavior ITreeSyncBehavior ; 
#endif

#ifndef __ITreeSyncServices_FWD_DEFINED__
#define __ITreeSyncServices_FWD_DEFINED__
typedef interface ITreeSyncServices ITreeSyncServices ; 
#endif

#ifndef __ITreeSyncLogSink_FWD_DEFINED__
#define __ITreeSyncLogSink_FWD_DEFINED__
typedef interface ITreeSyncLogSink ITreeSyncLogSink ; 
#endif

#ifndef __ITreeSyncLogSource_FWD_DEFINED__
#define __ITreeSyncLogSource_FWD_DEFINED__
typedef interface ITreeSyncLogSource ITreeSyncLogSource ; 
#endif

#ifndef __ITreeSyncRemapHack_FWD_DEFINED__
#define __ITreeSyncRemapHack_FWD_DEFINED__
typedef interface ITreeSyncRemapHack ITreeSyncRemapHack ; 
#endif

#include "objidl.h"


typedef ITreeSyncBehavior __RPC_FAR *LPTREESYNCBEHAVIOR ; 


EXTERN_C const IID IID_ITreeSyncBehavior;

  interface  ITreeSyncBehavior : public IUnknown
    {
    public:
        virtual HRESULT  STDMETHODCALLTYPE GetHtmlElement(
                IHTMLElement __RPC_FAR *__RPC_FAR *pphe ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetDataHtmlElement(
                IHTMLElement __RPC_FAR *__RPC_FAR *pphe ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetMirrorHtmlElements(
                IHTMLElement __RPC_FAR *__RPC_FAR *rgphe , DWORD __RPC_FAR *pdwCount ) = 0;
    };

#ifdef DEBUG

#define CheckITreeSyncBehaviorMembers(klass)\
	struct _##klass##_ITreeSyncBehavior_GetHtmlElement\
		{\
		_##klass##_ITreeSyncBehavior_GetHtmlElement(HRESULT  (##klass##::*pfn)(IHTMLElement __RPC_FAR *__RPC_FAR *)){}\
		};\
	struct _##klass##_ITreeSyncBehavior_GetDataHtmlElement\
		{\
		_##klass##_ITreeSyncBehavior_GetDataHtmlElement(HRESULT  (##klass##::*pfn)(IHTMLElement __RPC_FAR *__RPC_FAR *)){}\
		};\
	struct _##klass##_ITreeSyncBehavior_GetMirrorHtmlElements\
		{\
		_##klass##_ITreeSyncBehavior_GetMirrorHtmlElements(HRESULT  (##klass##::*pfn)(IHTMLElement __RPC_FAR *__RPC_FAR *, DWORD __RPC_FAR *)){}\
		};\
	void klass::VerifyITreeSyncBehavior(){\
	_##klass##_ITreeSyncBehavior_GetHtmlElement pfn1(GetHtmlElement);\
	_##klass##_ITreeSyncBehavior_GetDataHtmlElement pfn2(GetDataHtmlElement);\
	_##klass##_ITreeSyncBehavior_GetMirrorHtmlElements pfn3(GetMirrorHtmlElements);\
	}\

#else

#define CheckITreeSyncBehaviorMembers(klass)

#endif /* DEBUG */

#define idITreeSyncBehavior	0x8860B602

#ifdef DeclareSmartPointer
#ifndef ITreeSyncBehaviorSPMethodsDefined
extern "C++" { TEMPLATECLASSB class ITreeSyncBehaviorSPMethods : public TEMPLATEBASEB(IUnknownSPMethods) { }; }
#define ITreeSyncBehaviorSPMethodsDefined
#endif
DeclareSmartPointer(ITreeSyncBehavior)
#define SPITreeSyncBehavior auto SP_ITreeSyncBehavior
#endif

#define DeclareITreeSyncBehaviorMethods()\
	(FNOBJECT) GetMirrorHtmlElements,\
	(FNOBJECT) GetDataHtmlElement,\
	(FNOBJECT) GetHtmlElement,\

#define DeclareITreeSyncBehaviorVtbl()\
	,_Method1\
	,_Method2\
	,_Method3\


typedef ITreeSyncServices __RPC_FAR *LPTREESYNCSERVICES ; 


EXTERN_C const IID IID_ITreeSyncServices;

  interface  ITreeSyncServices : public IUnknown
    {
    public:
        virtual HRESULT  STDMETHODCALLTYPE GetBindBehavior(
                IHTMLElement __RPC_FAR *pElemTreeSyncRoot , ITreeSyncBehavior __RPC_FAR *__RPC_FAR *pTreeSyncRoot ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE MoveSyncInfoToPointer(
                IMarkupPointer __RPC_FAR *pPointer , IHTMLElement __RPC_FAR *__RPC_FAR *pElemTreeSyncRoot , long __RPC_FAR *pcpRelative ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE MovePointerToSyncInfo(
                IMarkupPointer __RPC_FAR *pPointer , IHTMLElement __RPC_FAR *pElemTreeSyncRoot , long cpRelative ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE ApplyForward(
                IHTMLElement __RPC_FAR *pElemTreeSyncRoot , BYTE __RPC_FAR *rgbData , DWORD cbData , long cpRootBaseAdjust ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE ApplyReverse(
                IHTMLElement __RPC_FAR *pElemTreeSyncRoot , BYTE __RPC_FAR *rgbData , DWORD cbData , long cpRootBaseAdjust ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE InsertElementViewOnly(
                IHTMLElement __RPC_FAR *pIElem , IMarkupPointer __RPC_FAR *pIStart , IMarkupPointer __RPC_FAR *pIEnd ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE RemoveElementViewOnly(
                IHTMLElement __RPC_FAR *pIElem ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE MoveSyncInfoToElement(
                IHTMLElement __RPC_FAR *pElement , IHTMLElement __RPC_FAR *__RPC_FAR *pElemTreeSyncRoot , long __RPC_FAR *pcpRelative ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetElementFromSyncInfo(
                IHTMLElement __RPC_FAR *__RPC_FAR *pElement , IHTMLElement __RPC_FAR *pElemTreeSyncRoot , long cpRelative ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE RebuildCpMap(
                IHTMLElement __RPC_FAR *pIElementSyncRoot , ITreeSyncRemapHack __RPC_FAR *pSyncUpdateCallback ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE GetSyncBaseIndexI(
                IHTMLElement __RPC_FAR *pElementSyncRoot , long __RPC_FAR *pcpRelative ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE ApplyForward1(
                IHTMLElement __RPC_FAR *pElemTreeSyncRoot , DWORD opcode , BYTE __RPC_FAR *rgbStruct , long cpRootBaseAdjust ) = 0;
    };

#ifdef DEBUG

#define CheckITreeSyncServicesMembers(klass)\
	struct _##klass##_ITreeSyncServices_GetBindBehavior\
		{\
		_##klass##_ITreeSyncServices_GetBindBehavior(HRESULT  (##klass##::*pfn)(IHTMLElement __RPC_FAR *, ITreeSyncBehavior __RPC_FAR *__RPC_FAR *)){}\
		};\
	struct _##klass##_ITreeSyncServices_MoveSyncInfoToPointer\
		{\
		_##klass##_ITreeSyncServices_MoveSyncInfoToPointer(HRESULT  (##klass##::*pfn)(IMarkupPointer __RPC_FAR *, IHTMLElement __RPC_FAR *__RPC_FAR *, long __RPC_FAR *)){}\
		};\
	struct _##klass##_ITreeSyncServices_MovePointerToSyncInfo\
		{\
		_##klass##_ITreeSyncServices_MovePointerToSyncInfo(HRESULT  (##klass##::*pfn)(IMarkupPointer __RPC_FAR *, IHTMLElement __RPC_FAR *, long )){}\
		};\
	struct _##klass##_ITreeSyncServices_ApplyForward\
		{\
		_##klass##_ITreeSyncServices_ApplyForward(HRESULT  (##klass##::*pfn)(IHTMLElement __RPC_FAR *, BYTE __RPC_FAR *, DWORD , long )){}\
		};\
	struct _##klass##_ITreeSyncServices_ApplyReverse\
		{\
		_##klass##_ITreeSyncServices_ApplyReverse(HRESULT  (##klass##::*pfn)(IHTMLElement __RPC_FAR *, BYTE __RPC_FAR *, DWORD , long )){}\
		};\
	struct _##klass##_ITreeSyncServices_InsertElementViewOnly\
		{\
		_##klass##_ITreeSyncServices_InsertElementViewOnly(HRESULT  (##klass##::*pfn)(IHTMLElement __RPC_FAR *, IMarkupPointer __RPC_FAR *, IMarkupPointer __RPC_FAR *)){}\
		};\
	struct _##klass##_ITreeSyncServices_RemoveElementViewOnly\
		{\
		_##klass##_ITreeSyncServices_RemoveElementViewOnly(HRESULT  (##klass##::*pfn)(IHTMLElement __RPC_FAR *)){}\
		};\
	struct _##klass##_ITreeSyncServices_MoveSyncInfoToElement\
		{\
		_##klass##_ITreeSyncServices_MoveSyncInfoToElement(HRESULT  (##klass##::*pfn)(IHTMLElement __RPC_FAR *, IHTMLElement __RPC_FAR *__RPC_FAR *, long __RPC_FAR *)){}\
		};\
	struct _##klass##_ITreeSyncServices_GetElementFromSyncInfo\
		{\
		_##klass##_ITreeSyncServices_GetElementFromSyncInfo(HRESULT  (##klass##::*pfn)(IHTMLElement __RPC_FAR *__RPC_FAR *, IHTMLElement __RPC_FAR *, long )){}\
		};\
	struct _##klass##_ITreeSyncServices_RebuildCpMap\
		{\
		_##klass##_ITreeSyncServices_RebuildCpMap(HRESULT  (##klass##::*pfn)(IHTMLElement __RPC_FAR *, ITreeSyncRemapHack __RPC_FAR *)){}\
		};\
	struct _##klass##_ITreeSyncServices_GetSyncBaseIndexI\
		{\
		_##klass##_ITreeSyncServices_GetSyncBaseIndexI(HRESULT  (##klass##::*pfn)(IHTMLElement __RPC_FAR *, long __RPC_FAR *)){}\
		};\
	struct _##klass##_ITreeSyncServices_ApplyForward1\
		{\
		_##klass##_ITreeSyncServices_ApplyForward1(HRESULT  (##klass##::*pfn)(IHTMLElement __RPC_FAR *, DWORD , BYTE __RPC_FAR *, long )){}\
		};\
	void klass::VerifyITreeSyncServices(){\
	_##klass##_ITreeSyncServices_GetBindBehavior pfn1(GetBindBehavior);\
	_##klass##_ITreeSyncServices_MoveSyncInfoToPointer pfn2(MoveSyncInfoToPointer);\
	_##klass##_ITreeSyncServices_MovePointerToSyncInfo pfn3(MovePointerToSyncInfo);\
	_##klass##_ITreeSyncServices_ApplyForward pfn4(ApplyForward);\
	_##klass##_ITreeSyncServices_ApplyReverse pfn5(ApplyReverse);\
	_##klass##_ITreeSyncServices_InsertElementViewOnly pfn6(InsertElementViewOnly);\
	_##klass##_ITreeSyncServices_RemoveElementViewOnly pfn7(RemoveElementViewOnly);\
	_##klass##_ITreeSyncServices_MoveSyncInfoToElement pfn8(MoveSyncInfoToElement);\
	_##klass##_ITreeSyncServices_GetElementFromSyncInfo pfn9(GetElementFromSyncInfo);\
	_##klass##_ITreeSyncServices_RebuildCpMap pfn10(RebuildCpMap);\
	_##klass##_ITreeSyncServices_GetSyncBaseIndexI pfn11(GetSyncBaseIndexI);\
	_##klass##_ITreeSyncServices_ApplyForward1 pfn12(ApplyForward1);\
	}\

#else

#define CheckITreeSyncServicesMembers(klass)

#endif /* DEBUG */

#define idITreeSyncServices	0x8860B601

#ifdef DeclareSmartPointer
#ifndef ITreeSyncServicesSPMethodsDefined
extern "C++" { TEMPLATECLASSB class ITreeSyncServicesSPMethods : public TEMPLATEBASEB(IUnknownSPMethods) { }; }
#define ITreeSyncServicesSPMethodsDefined
#endif
DeclareSmartPointer(ITreeSyncServices)
#define SPITreeSyncServices auto SP_ITreeSyncServices
#endif

#define DeclareITreeSyncServicesMethods()\
	(FNOBJECT) ApplyForward1,\
	(FNOBJECT) GetSyncBaseIndexI,\
	(FNOBJECT) RebuildCpMap,\
	(FNOBJECT) GetElementFromSyncInfo,\
	(FNOBJECT) MoveSyncInfoToElement,\
	(FNOBJECT) RemoveElementViewOnly,\
	(FNOBJECT) InsertElementViewOnly,\
	(FNOBJECT) ApplyReverse,\
	(FNOBJECT) ApplyForward,\
	(FNOBJECT) MovePointerToSyncInfo,\
	(FNOBJECT) MoveSyncInfoToPointer,\
	(FNOBJECT) GetBindBehavior,\

#define DeclareITreeSyncServicesVtbl()\
	,_Method1\
	,_Method2\
	,_Method3\
	,_Method4\
	,_Method5\
	,_Method6\
	,_Method7\
	,_Method8\
	,_Method9\
	,_Method10\
	,_Method11\
	,_Method12\



EXTERN_C const IID IID_ITreeSyncLogSink;

  interface  ITreeSyncLogSink : public IUnknown
    {
    public:
        virtual HRESULT  STDMETHODCALLTYPE ReceiveStreamData(
                IHTMLElement __RPC_FAR *pElemTreeSyncRoot , BYTE __RPC_FAR *rgbData , DWORD cbData , long cpRootBaseAdjust ) = 0;
    };

#ifdef DEBUG

#define CheckITreeSyncLogSinkMembers(klass)\
	struct _##klass##_ITreeSyncLogSink_ReceiveStreamData\
		{\
		_##klass##_ITreeSyncLogSink_ReceiveStreamData(HRESULT  (##klass##::*pfn)(IHTMLElement __RPC_FAR *, BYTE __RPC_FAR *, DWORD , long )){}\
		};\
	void klass::VerifyITreeSyncLogSink(){\
	_##klass##_ITreeSyncLogSink_ReceiveStreamData pfn1(ReceiveStreamData);\
	}\

#else

#define CheckITreeSyncLogSinkMembers(klass)

#endif /* DEBUG */

#define idITreeSyncLogSink	0x48a18696

#ifdef DeclareSmartPointer
#ifndef ITreeSyncLogSinkSPMethodsDefined
extern "C++" { TEMPLATECLASSB class ITreeSyncLogSinkSPMethods : public TEMPLATEBASEB(IUnknownSPMethods) { }; }
#define ITreeSyncLogSinkSPMethodsDefined
#endif
DeclareSmartPointer(ITreeSyncLogSink)
#define SPITreeSyncLogSink auto SP_ITreeSyncLogSink
#endif

#define DeclareITreeSyncLogSinkMethods()\
	(FNOBJECT) ReceiveStreamData,\

#define DeclareITreeSyncLogSinkVtbl()\
	,_Method1\



EXTERN_C const IID IID_ITreeSyncLogSource;

  interface  ITreeSyncLogSource : public IUnknown
    {
    public:
        virtual HRESULT  STDMETHODCALLTYPE RegisterLogSink(
                ITreeSyncLogSink __RPC_FAR *pLogSink ) = 0;
        virtual HRESULT  STDMETHODCALLTYPE UnregisterLogSink(
                ITreeSyncLogSink __RPC_FAR *pLogSink ) = 0;
    };

#ifdef DEBUG

#define CheckITreeSyncLogSourceMembers(klass)\
	struct _##klass##_ITreeSyncLogSource_RegisterLogSink\
		{\
		_##klass##_ITreeSyncLogSource_RegisterLogSink(HRESULT  (##klass##::*pfn)(ITreeSyncLogSink __RPC_FAR *)){}\
		};\
	struct _##klass##_ITreeSyncLogSource_UnregisterLogSink\
		{\
		_##klass##_ITreeSyncLogSource_UnregisterLogSink(HRESULT  (##klass##::*pfn)(ITreeSyncLogSink __RPC_FAR *)){}\
		};\
	void klass::VerifyITreeSyncLogSource(){\
	_##klass##_ITreeSyncLogSource_RegisterLogSink pfn1(RegisterLogSink);\
	_##klass##_ITreeSyncLogSource_UnregisterLogSink pfn2(UnregisterLogSink);\
	}\

#else

#define CheckITreeSyncLogSourceMembers(klass)

#endif /* DEBUG */

#define idITreeSyncLogSource	0x704bc5e4

#ifdef DeclareSmartPointer
#ifndef ITreeSyncLogSourceSPMethodsDefined
extern "C++" { TEMPLATECLASSB class ITreeSyncLogSourceSPMethods : public TEMPLATEBASEB(IUnknownSPMethods) { }; }
#define ITreeSyncLogSourceSPMethodsDefined
#endif
DeclareSmartPointer(ITreeSyncLogSource)
#define SPITreeSyncLogSource auto SP_ITreeSyncLogSource
#endif

#define DeclareITreeSyncLogSourceMethods()\
	(FNOBJECT) UnregisterLogSink,\
	(FNOBJECT) RegisterLogSink,\

#define DeclareITreeSyncLogSourceVtbl()\
	,_Method1\
	,_Method2\



EXTERN_C const IID IID_ITreeSyncRemapHack;

  interface  ITreeSyncRemapHack : public IUnknown
    {
    public:
        virtual HRESULT  STDMETHODCALLTYPE AdjustViewCpOneRoot(
                IHTMLDocument2 __RPC_FAR *pIDoc , IHTMLElement __RPC_FAR *pIElementSyncRoot , long cpView , long cpViewInsert ) = 0;
    };

#ifdef DEBUG

#define CheckITreeSyncRemapHackMembers(klass)\
	struct _##klass##_ITreeSyncRemapHack_AdjustViewCpOneRoot\
		{\
		_##klass##_ITreeSyncRemapHack_AdjustViewCpOneRoot(HRESULT  (##klass##::*pfn)(IHTMLDocument2 __RPC_FAR *, IHTMLElement __RPC_FAR *, long , long )){}\
		};\
	void klass::VerifyITreeSyncRemapHack(){\
	_##klass##_ITreeSyncRemapHack_AdjustViewCpOneRoot pfn1(AdjustViewCpOneRoot);\
	}\

#else

#define CheckITreeSyncRemapHackMembers(klass)

#endif /* DEBUG */

#define idITreeSyncRemapHack	0x12208630

#ifdef DeclareSmartPointer
#ifndef ITreeSyncRemapHackSPMethodsDefined
extern "C++" { TEMPLATECLASSB class ITreeSyncRemapHackSPMethods : public TEMPLATEBASEB(IUnknownSPMethods) { }; }
#define ITreeSyncRemapHackSPMethodsDefined
#endif
DeclareSmartPointer(ITreeSyncRemapHack)
#define SPITreeSyncRemapHack auto SP_ITreeSyncRemapHack
#endif

#define DeclareITreeSyncRemapHackMethods()\
	(FNOBJECT) AdjustViewCpOneRoot,\

#define DeclareITreeSyncRemapHackVtbl()\
	,_Method1\


}



#endif

