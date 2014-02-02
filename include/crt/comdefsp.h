/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the mingw-w64 runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#if !defined(_INC_COMDEFSP)
#define _INC_COMDEFSP

#include <_mingw.h>

#if !defined(RC_INVOKED) && USE___UUIDOF != 0

#ifndef __cplusplus
#error Native compiler support only available in C++ compiler.
#endif

#ifndef _COM_SMARTPTR_TYPEDEF
#error The header file comdefsp.h requires comdef.h to be included first.
#endif

#if defined(__AsyncIAdviseSink_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(AsyncIAdviseSink,IID_AsyncIAdviseSink);
#endif
#if defined(__AsyncIAdviseSink2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(AsyncIAdviseSink2,IID_AsyncIAdviseSink2);
#endif
#if defined(__AsyncIMultiQI_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(AsyncIMultiQI,IID_AsyncIMultiQI);
#endif
#if defined(__AsyncIPipeByte_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(AsyncIPipeByte,IID_AsyncIPipeByte);
#endif
#if defined(__AsyncIPipeDouble_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(AsyncIPipeDouble,IID_AsyncIPipeDouble);
#endif
#if defined(__AsyncIPipeLong_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(AsyncIPipeLong,IID_AsyncIPipeLong);
#endif
#if defined(__AsyncIUnknown_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(AsyncIUnknown,IID_AsyncIUnknown);
#endif
#if defined(__FolderItem_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(FolderItem,IID_FolderItem);
#endif
#if defined(__FolderItemVerb_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(FolderItemVerb,IID_FolderItemVerb);
#endif
#if defined(__FolderItemVerbs_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(FolderItemVerbs,IID_FolderItemVerbs);
#endif
#if defined(__FolderItems_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(FolderItems,IID_FolderItems);
#endif
#if defined(__IAccessible_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IAccessible,IID_IAccessible);
#endif
#if defined(__IActiveScript_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IActiveScript,IID_IActiveScript);
#endif
#if defined(__IActiveScriptError_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IActiveScriptError,IID_IActiveScriptError);
#endif
#if defined(__IActiveScriptParse_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IActiveScriptParse,IID_IActiveScriptParse);
#endif
#if defined(__IActiveScriptParseProcedure_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IActiveScriptParseProcedure,IID_IActiveScriptParseProcedure);
#endif
#if defined(__IActiveScriptParseProcedureOld_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IActiveScriptParseProcedureOld,IID_IActiveScriptParseProcedureOld);
#endif
#if defined(__IActiveScriptSite_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IActiveScriptSite,IID_IActiveScriptSite);
#endif
#if defined(__IActiveScriptSiteInterruptPoll_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IActiveScriptSiteInterruptPoll,IID_IActiveScriptSiteInterruptPoll);
#endif
#if defined(__IActiveScriptSiteWindow_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IActiveScriptSiteWindow,IID_IActiveScriptSiteWindow);
#endif
#if defined(__IActiveScriptStats_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IActiveScriptStats,IID_IActiveScriptStats);
#endif
#if defined(__IAddrExclusionControl_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IAddrExclusionControl,IID_IAddrExclusionControl);
#endif
#if defined(__IAddrTrackingControl_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IAddrTrackingControl,IID_IAddrTrackingControl);
#endif
#if defined(__IAdviseSink_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IAdviseSink,IID_IAdviseSink);
#endif
#if defined(__IAdviseSink2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IAdviseSink2,IID_IAdviseSink2);
#endif
#if defined(__IAdviseSinkEx_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IAdviseSinkEx,IID_IAdviseSinkEx);
#endif
#if defined(__IAsyncManager_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IAsyncManager,IID_IAsyncManager);
#endif
#if defined(__IAsyncRpcChannelBuffer_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IAsyncRpcChannelBuffer,IID_IAsyncRpcChannelBuffer);
#endif
#if defined(__IAuthenticate_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IAuthenticate,IID_IAuthenticate);
#endif
#if defined(__IBindCtx_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IBindCtx,IID_IBindCtx);
#endif
#if defined(__IBindEventHandler_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IBindEventHandler,IID_IBindEventHandler);
#endif
#if defined(__IBindHost_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IBindHost,IID_IBindHost);
#endif
#if defined(__IBindProtocol_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IBindProtocol,IID_IBindProtocol);
#endif
#if defined(__IBindStatusCallback_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IBindStatusCallback,IID_IBindStatusCallback);
#endif
#if defined(__IBinding_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IBinding,IID_IBinding);
#endif
#if defined(__IBlockingLock_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IBlockingLock,IID_IBlockingLock);
#endif
#if defined(__ICSSFilter_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ICSSFilter,IID_ICSSFilter);
#endif
#if defined(__ICSSFilterSite_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ICSSFilterSite,IID_ICSSFilterSite);
#endif
#if defined(__ICallFactory_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ICallFactory,IID_ICallFactory);
#endif
#if defined(__ICancelMethodCalls_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ICancelMethodCalls,IID_ICancelMethodCalls);
#endif
#if defined(__ICatInformation_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ICatInformation,IID_ICatInformation);
#endif
#if defined(__ICatRegister_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ICatRegister,IID_ICatRegister);
#endif
#if defined(__ICatalogFileInfo_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ICatalogFileInfo,IID_ICatalogFileInfo);
#endif
#if defined(__IChannelHook_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IChannelHook,IID_IChannelHook);
#endif
#if defined(__IChannelMgr_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IChannelMgr,IID_IChannelMgr);
#endif
#if defined(__IClassActivator_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IClassActivator,IID_IClassActivator);
#endif
#if defined(__IClassFactory_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IClassFactory,IID_IClassFactory);
#endif
#if defined(__IClassFactory2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IClassFactory2,IID_IClassFactory2);
#endif
#if defined(__IClientSecurity_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IClientSecurity,IID_IClientSecurity);
#endif
#if defined(__ICodeInstall_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ICodeInstall,IID_ICodeInstall);
#endif
#if defined(__IConnectionPoint_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IConnectionPoint,IID_IConnectionPoint);
#endif
#if defined(__IConnectionPointContainer_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IConnectionPointContainer,IID_IConnectionPointContainer);
#endif
#if defined(__IContinue_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IContinue,IID_IContinue);
#endif
#if defined(__IContinueCallback_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IContinueCallback,IID_IContinueCallback);
#endif
#if defined(__ICreateErrorInfo_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ICreateErrorInfo,IID_ICreateErrorInfo);
#endif
#if defined(__ICreateTypeInfo_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ICreateTypeInfo,IID_ICreateTypeInfo);
#endif
#if defined(__ICreateTypeInfo2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ICreateTypeInfo2,IID_ICreateTypeInfo2);
#endif
#if defined(__ICreateTypeLib_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ICreateTypeLib,IID_ICreateTypeLib);
#endif
#if defined(__ICreateTypeLib2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ICreateTypeLib2,IID_ICreateTypeLib2);
#endif
#if defined(__ICustomDoc_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ICustomDoc,IID_ICustomDoc);
#endif
#if defined(__IDataAdviseHolder_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IDataAdviseHolder,IID_IDataAdviseHolder);
#endif
#if defined(__IDataFilter_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IDataFilter,IID_IDataFilter);
#endif
#if defined(__IDataObject_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IDataObject,IID_IDataObject);
#endif
#if defined(__IDeskBand_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IDeskBand,IID_IDeskBand);
#endif
#if defined(__IDirectWriterLock_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IDirectWriterLock,IID_IDirectWriterLock);
#endif
#if defined(__IDispError_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IDispError,IID_IDispError);
#endif
#if defined(__IDispatch_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IDispatch,IID_IDispatch);
#endif
#if defined(__IDispatchEx_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IDispatchEx,IID_IDispatchEx);
#endif
#if defined(__IDocHostShowUI_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IDocHostShowUI,IID_IDocHostShowUI);
#endif
#if defined(__IDocHostUIHandler_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IDocHostUIHandler,IID_IDocHostUIHandler);
#endif
#if defined(__IDockingWindow_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IDockingWindow,IID_IDockingWindow);
#endif
#if defined(__IDropSource_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IDropSource,IID_IDropSource);
#endif
#if defined(__IDropTarget_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IDropTarget,IID_IDropTarget);
#endif
#if defined(__IDummyHICONIncluder_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IDummyHICONIncluder,IID_IDummyHICONIncluder);
#endif
#if defined(__IEncodingFilterFactory_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IEncodingFilterFactory,IID_IEncodingFilterFactory);
#endif
#if defined(__IEnumCATEGORYINFO_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IEnumCATEGORYINFO,IID_IEnumCATEGORYINFO);
#endif
#if defined(__IEnumChannels_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IEnumChannels,IID_IEnumChannels);
#endif
#if defined(__IEnumCodePage_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IEnumCodePage,IID_IEnumCodePage);
#endif
#if defined(__IEnumConnectionPoints_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IEnumConnectionPoints,IID_IEnumConnectionPoints);
#endif
#if defined(__IEnumConnections_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IEnumConnections,IID_IEnumConnections);
#endif
#if defined(__IEnumFORMATETC_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IEnumFORMATETC,IID_IEnumFORMATETC);
#endif
#if defined(__IEnumGUID_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IEnumGUID,IID_IEnumGUID);
#endif
#if defined(__IEnumHLITEM_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IEnumHLITEM,IID_IEnumHLITEM);
#endif
#if defined(__IEnumIDList_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IEnumIDList,IID_IEnumIDList);
#endif
#if defined(__IEnumMoniker_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IEnumMoniker,IID_IEnumMoniker);
#endif
#if defined(__IEnumOLEVERB_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IEnumOLEVERB,IID_IEnumOLEVERB);
#endif
#if defined(__IEnumOleDocumentViews_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IEnumOleDocumentViews,IID_IEnumOleDocumentViews);
#endif
#if defined(__IEnumOleUndoUnits_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IEnumOleUndoUnits,IID_IEnumOleUndoUnits);
#endif
#if defined(__IEnumRfc1766_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IEnumRfc1766,IID_IEnumRfc1766);
#endif
#if defined(__IEnumSTATDATA_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IEnumSTATDATA,IID_IEnumSTATDATA);
#endif
#if defined(__IEnumSTATPROPSETSTG_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IEnumSTATPROPSETSTG,IID_IEnumSTATPROPSETSTG);
#endif
#if defined(__IEnumSTATPROPSTG_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IEnumSTATPROPSTG,IID_IEnumSTATPROPSTG);
#endif
#if defined(__IEnumSTATSTG_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IEnumSTATSTG,IID_IEnumSTATSTG);
#endif
#if defined(__IEnumSTATURL_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IEnumSTATURL,IID_IEnumSTATURL);
#endif
#if defined(__IEnumString_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IEnumString,IID_IEnumString);
#endif
#if defined(__IEnumUnknown_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IEnumUnknown,IID_IEnumUnknown);
#endif
#if defined(__IEnumVARIANT_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IEnumVARIANT,IID_IEnumVARIANT);
#endif
#if defined(__IErrorInfo_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IErrorInfo,IID_IErrorInfo);
#endif
#if defined(__IErrorLog_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IErrorLog,IID_IErrorLog);
#endif
#if defined(__IExtensionServices_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IExtensionServices,IID_IExtensionServices);
#endif
#if defined(__IExternalConnection_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IExternalConnection,IID_IExternalConnection);
#endif
#if defined(__IFillLockBytes_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IFillLockBytes,IID_IFillLockBytes);
#endif
#if defined(__IFilter_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IFilter,IID_IFilter);
#endif
#if defined(__IFolderViewOC_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IFolderViewOC,IID_IFolderViewOC);
#endif
#if defined(__IFont_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IFont,IID_IFont);
#endif
#if defined(__IFontDisp_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IFontDisp,IID_IFontDisp);
#endif
#if defined(__IFontEventsDisp_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IFontEventsDisp,IID_IFontEventsDisp);
#endif
#if defined(__IForegroundTransfer_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IForegroundTransfer,IID_IForegroundTransfer);
#endif
#if defined(__IGlobalInterfaceTable_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IGlobalInterfaceTable,IID_IGlobalInterfaceTable);
#endif
#if defined(__IHTMLAnchorElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLAnchorElement,IID_IHTMLAnchorElement);
#endif
#if defined(__IHTMLAreaElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLAreaElement,IID_IHTMLAreaElement);
#endif
#if defined(__IHTMLAreasCollection_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLAreasCollection,IID_IHTMLAreasCollection);
#endif
#if defined(__IHTMLBGsound_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLBGsound,IID_IHTMLBGsound);
#endif
#if defined(__IHTMLBRElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLBRElement,IID_IHTMLBRElement);
#endif
#if defined(__IHTMLBaseElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLBaseElement,IID_IHTMLBaseElement);
#endif
#if defined(__IHTMLBaseFontElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLBaseFontElement,IID_IHTMLBaseFontElement);
#endif
#if defined(__IHTMLBlockElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLBlockElement,IID_IHTMLBlockElement);
#endif
#if defined(__IHTMLBodyElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLBodyElement,IID_IHTMLBodyElement);
#endif
#if defined(__IHTMLButtonElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLButtonElement,IID_IHTMLButtonElement);
#endif
#if defined(__IHTMLCommentElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLCommentElement,IID_IHTMLCommentElement);
#endif
#if defined(__IHTMLControlElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLControlElement,IID_IHTMLControlElement);
#endif
#if defined(__IHTMLControlRange_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLControlRange,IID_IHTMLControlRange);
#endif
#if defined(__IHTMLDDElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLDDElement,IID_IHTMLDDElement);
#endif
#if defined(__IHTMLDListElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLDListElement,IID_IHTMLDListElement);
#endif
#if defined(__IHTMLDTElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLDTElement,IID_IHTMLDTElement);
#endif
#if defined(__IHTMLDatabinding_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLDatabinding,IID_IHTMLDatabinding);
#endif
#if defined(__IHTMLDialog_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLDialog,IID_IHTMLDialog);
#endif
#if defined(__IHTMLDivElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLDivElement,IID_IHTMLDivElement);
#endif
#if defined(__IHTMLDivPosition_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLDivPosition,IID_IHTMLDivPosition);
#endif
#if defined(__IHTMLDocument_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLDocument,IID_IHTMLDocument);
#endif
#if defined(__IHTMLDocument2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLDocument2,IID_IHTMLDocument2);
#endif
#if defined(__IHTMLElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLElement,IID_IHTMLElement);
#endif
#if defined(__IHTMLElementCollection_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLElementCollection,IID_IHTMLElementCollection);
#endif
#if defined(__IHTMLEmbedElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLEmbedElement,IID_IHTMLEmbedElement);
#endif
#if defined(__IHTMLEventObj_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLEventObj,IID_IHTMLEventObj);
#endif
#if defined(__IHTMLFieldSetElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLFieldSetElement,IID_IHTMLFieldSetElement);
#endif
#if defined(__IHTMLFiltersCollection_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLFiltersCollection,IID_IHTMLFiltersCollection);
#endif
#if defined(__IHTMLFontElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLFontElement,IID_IHTMLFontElement);
#endif
#if defined(__IHTMLFontNamesCollection_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLFontNamesCollection,IID_IHTMLFontNamesCollection);
#endif
#if defined(__IHTMLFontSizesCollection_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLFontSizesCollection,IID_IHTMLFontSizesCollection);
#endif
#if defined(__IHTMLFormElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLFormElement,IID_IHTMLFormElement);
#endif
#if defined(__IHTMLFrameBase_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLFrameBase,IID_IHTMLFrameBase);
#endif
#if defined(__IHTMLFrameElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLFrameElement,IID_IHTMLFrameElement);
#endif
#if defined(__IHTMLFrameSetElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLFrameSetElement,IID_IHTMLFrameSetElement);
#endif
#if defined(__IHTMLFramesCollection2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLFramesCollection2,IID_IHTMLFramesCollection2);
#endif
#if defined(__IHTMLHRElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLHRElement,IID_IHTMLHRElement);
#endif
#if defined(__IHTMLHeaderElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLHeaderElement,IID_IHTMLHeaderElement);
#endif
#if defined(__IHTMLIFrameElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLIFrameElement,IID_IHTMLIFrameElement);
#endif
#if defined(__IHTMLImageElementFactory_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLImageElementFactory,IID_IHTMLImageElementFactory);
#endif
#if defined(__IHTMLImgElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLImgElement,IID_IHTMLImgElement);
#endif
#if defined(__IHTMLInputButtonElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLInputButtonElement,IID_IHTMLInputButtonElement);
#endif
#if defined(__IHTMLInputFileElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLInputFileElement,IID_IHTMLInputFileElement);
#endif
#if defined(__IHTMLInputHiddenElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLInputHiddenElement,IID_IHTMLInputHiddenElement);
#endif
#if defined(__IHTMLInputImage_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLInputImage,IID_IHTMLInputImage);
#endif
#if defined(__IHTMLInputTextElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLInputTextElement,IID_IHTMLInputTextElement);
#endif
#if defined(__IHTMLIsIndexElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLIsIndexElement,IID_IHTMLIsIndexElement);
#endif
#if defined(__IHTMLLIElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLLIElement,IID_IHTMLLIElement);
#endif
#if defined(__IHTMLLabelElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLLabelElement,IID_IHTMLLabelElement);
#endif
#if defined(__IHTMLLegendElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLLegendElement,IID_IHTMLLegendElement);
#endif
#if defined(__IHTMLLinkElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLLinkElement,IID_IHTMLLinkElement);
#endif
#if defined(__IHTMLListElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLListElement,IID_IHTMLListElement);
#endif
#if defined(__IHTMLLocation_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLLocation,IID_IHTMLLocation);
#endif
#if defined(__IHTMLMapElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLMapElement,IID_IHTMLMapElement);
#endif
#if defined(__IHTMLMarqueeElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLMarqueeElement,IID_IHTMLMarqueeElement);
#endif
#if defined(__IHTMLMetaElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLMetaElement,IID_IHTMLMetaElement);
#endif
#if defined(__IHTMLMimeTypesCollection_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLMimeTypesCollection,IID_IHTMLMimeTypesCollection);
#endif
#if defined(__IHTMLNextIdElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLNextIdElement,IID_IHTMLNextIdElement);
#endif
#if defined(__IHTMLNoShowElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLNoShowElement,IID_IHTMLNoShowElement);
#endif
#if defined(__IHTMLOListElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLOListElement,IID_IHTMLOListElement);
#endif
#if defined(__IHTMLObjectElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLObjectElement,IID_IHTMLObjectElement);
#endif
#if defined(__IHTMLOpsProfile_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLOpsProfile,IID_IHTMLOpsProfile);
#endif
#if defined(__IHTMLOptionButtonElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLOptionButtonElement,IID_IHTMLOptionButtonElement);
#endif
#if defined(__IHTMLOptionElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLOptionElement,IID_IHTMLOptionElement);
#endif
#if defined(__IHTMLOptionElementFactory_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLOptionElementFactory,IID_IHTMLOptionElementFactory);
#endif
#if defined(__IHTMLOptionsHolder_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLOptionsHolder,IID_IHTMLOptionsHolder);
#endif
#if defined(__IHTMLParaElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLParaElement,IID_IHTMLParaElement);
#endif
#if defined(__IHTMLPhraseElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLPhraseElement,IID_IHTMLPhraseElement);
#endif
#if defined(__IHTMLPluginsCollection_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLPluginsCollection,IID_IHTMLPluginsCollection);
#endif
#if defined(__IHTMLRuleStyle_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLRuleStyle,IID_IHTMLRuleStyle);
#endif
#if defined(__IHTMLScreen_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLScreen,IID_IHTMLScreen);
#endif
#if defined(__IHTMLScriptElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLScriptElement,IID_IHTMLScriptElement);
#endif
#if defined(__IHTMLSelectElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLSelectElement,IID_IHTMLSelectElement);
#endif
#if defined(__IHTMLSelectionObject_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLSelectionObject,IID_IHTMLSelectionObject);
#endif
#if defined(__IHTMLSpanElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLSpanElement,IID_IHTMLSpanElement);
#endif
#if defined(__IHTMLSpanFlow_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLSpanFlow,IID_IHTMLSpanFlow);
#endif
#if defined(__IHTMLStyle_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLStyle,IID_IHTMLStyle);
#endif
#if defined(__IHTMLStyleElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLStyleElement,IID_IHTMLStyleElement);
#endif
#if defined(__IHTMLStyleFontFace_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLStyleFontFace,IID_IHTMLStyleFontFace);
#endif
#if defined(__IHTMLStyleSheet_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLStyleSheet,IID_IHTMLStyleSheet);
#endif
#if defined(__IHTMLStyleSheetRule_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLStyleSheetRule,IID_IHTMLStyleSheetRule);
#endif
#if defined(__IHTMLStyleSheetRulesCollection_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLStyleSheetRulesCollection,IID_IHTMLStyleSheetRulesCollection);
#endif
#if defined(__IHTMLStyleSheetsCollection_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLStyleSheetsCollection,IID_IHTMLStyleSheetsCollection);
#endif
#if defined(__IHTMLTable_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLTable,IID_IHTMLTable);
#endif
#if defined(__IHTMLTableCaption_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLTableCaption,IID_IHTMLTableCaption);
#endif
#if defined(__IHTMLTableCell_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLTableCell,IID_IHTMLTableCell);
#endif
#if defined(__IHTMLTableCol_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLTableCol,IID_IHTMLTableCol);
#endif
#if defined(__IHTMLTableRow_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLTableRow,IID_IHTMLTableRow);
#endif
#if defined(__IHTMLTableSection_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLTableSection,IID_IHTMLTableSection);
#endif
#if defined(__IHTMLTextAreaElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLTextAreaElement,IID_IHTMLTextAreaElement);
#endif
#if defined(__IHTMLTextContainer_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLTextContainer,IID_IHTMLTextContainer);
#endif
#if defined(__IHTMLTextElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLTextElement,IID_IHTMLTextElement);
#endif
#if defined(__IHTMLTitleElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLTitleElement,IID_IHTMLTitleElement);
#endif
#if defined(__IHTMLTxtRange_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLTxtRange,IID_IHTMLTxtRange);
#endif
#if defined(__IHTMLUListElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLUListElement,IID_IHTMLUListElement);
#endif
#if defined(__IHTMLUnknownElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLUnknownElement,IID_IHTMLUnknownElement);
#endif
#if defined(__IHTMLWindow2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHTMLWindow2,IID_IHTMLWindow2);
#endif
#if defined(__IHlink_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHlink,IID_IHlink);
#endif
#if defined(__IHlinkBrowseContext_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHlinkBrowseContext,IID_IHlinkBrowseContext);
#endif
#if defined(__IHlinkFrame_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHlinkFrame,IID_IHlinkFrame);
#endif
#if defined(__IHlinkSite_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHlinkSite,IID_IHlinkSite);
#endif
#if defined(__IHlinkTarget_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHlinkTarget,IID_IHlinkTarget);
#endif
#if defined(__IHttpNegotiate_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHttpNegotiate,IID_IHttpNegotiate);
#endif
#if defined(__IHttpNegotiate2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHttpNegotiate2,IID_IHttpNegotiate2);
#endif
#if defined(__IHttpSecurity_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IHttpSecurity,IID_IHttpSecurity);
#endif
#if defined(__IImageDecodeEventSink_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IImageDecodeEventSink,IID_IImageDecodeEventSink);
#endif
#if defined(__IImageDecodeFilter_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IImageDecodeFilter,IID_IImageDecodeFilter);
#endif
#if defined(__IInternalUnknown_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IInternalUnknown,IID_IInternalUnknown);
#endif
#if defined(__IInternet_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IInternet,IID_IInternet);
#endif
#if defined(__IInternetBindInfo_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IInternetBindInfo,IID_IInternetBindInfo);
#endif
#if defined(__IInternetHostSecurityManager_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IInternetHostSecurityManager,IID_IInternetHostSecurityManager);
#endif
#if defined(__IInternetPriority_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IInternetPriority,IID_IInternetPriority);
#endif
#if defined(__IInternetProtocol_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IInternetProtocol,IID_IInternetProtocol);
#endif
#if defined(__IInternetProtocolInfo_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IInternetProtocolInfo,IID_IInternetProtocolInfo);
#endif
#if defined(__IInternetProtocolRoot_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IInternetProtocolRoot,IID_IInternetProtocolRoot);
#endif
#if defined(__IInternetProtocolSink_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IInternetProtocolSink,IID_IInternetProtocolSink);
#endif
#if defined(__IInternetProtocolSinkStackable_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IInternetProtocolSinkStackable,IID_IInternetProtocolSinkStackable);
#endif
#if defined(__IInternetSecurityManager_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IInternetSecurityManager,IID_IInternetSecurityManager);
#endif
#if defined(__IInternetSecurityMgrSite_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IInternetSecurityMgrSite,IID_IInternetSecurityMgrSite);
#endif
#if defined(__IInternetSession_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IInternetSession,IID_IInternetSession);
#endif
#if defined(__IInternetThreadSwitch_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IInternetThreadSwitch,IID_IInternetThreadSwitch);
#endif
#if defined(__IInternetZoneManager_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IInternetZoneManager,IID_IInternetZoneManager);
#endif
#if defined(__ILayoutStorage_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ILayoutStorage,IID_ILayoutStorage);
#endif
#if defined(__ILockBytes_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ILockBytes,IID_ILockBytes);
#endif
#if defined(__IMLangCodePages_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IMLangCodePages,IID_IMLangCodePages);
#endif
#if defined(__IMLangConvertCharset_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IMLangConvertCharset,IID_IMLangConvertCharset);
#endif
#if defined(__IMLangFontLink_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IMLangFontLink,IID_IMLangFontLink);
#endif
#if defined(__IMLangLineBreakConsole_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IMLangLineBreakConsole,IID_IMLangLineBreakConsole);
#endif
#if defined(__IMLangString_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IMLangString,IID_IMLangString);
#endif
#if defined(__IMLangStringAStr_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IMLangStringAStr,IID_IMLangStringAStr);
#endif
#if defined(__IMLangStringBufA_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IMLangStringBufA,IID_IMLangStringBufA);
#endif
#if defined(__IMLangStringBufW_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IMLangStringBufW,IID_IMLangStringBufW);
#endif
#if defined(__IMLangStringWStr_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IMLangStringWStr,IID_IMLangStringWStr);
#endif
#if defined(__IMalloc_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IMalloc,IID_IMalloc);
#endif
#if defined(__IMallocSpy_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IMallocSpy,IID_IMallocSpy);
#endif
#if defined(__IMapMIMEToCLSID_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IMapMIMEToCLSID,IID_IMapMIMEToCLSID);
#endif
#if defined(__IMarshal_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IMarshal,IID_IMarshal);
#endif
#if defined(__IMarshal2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IMarshal2,IID_IMarshal2);
#endif
#if defined(__IMessageFilter_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IMessageFilter,IID_IMessageFilter);
#endif
#if defined(__IMimeInfo_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IMimeInfo,IID_IMimeInfo);
#endif
#if defined(__IMoniker_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IMoniker,IID_IMoniker);
#endif
#if defined(__IMonikerProp_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IMonikerProp,IID_IMonikerProp);
#endif
#if defined(__IMultiLanguage_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IMultiLanguage,IID_IMultiLanguage);
#endif
#if defined(__IMultiQI_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IMultiQI,IID_IMultiQI);
#endif
#if defined(__IObjectIdentity_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IObjectIdentity,IID_IObjectIdentity);
#endif
#if defined(__IObjectSafety_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IObjectSafety,IID_IObjectSafety);
#endif
#if defined(__IObjectWithSite_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IObjectWithSite,IID_IObjectWithSite);
#endif
#if defined(__IOleAdviseHolder_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleAdviseHolder,IID_IOleAdviseHolder);
#endif
#if defined(__IOleCache_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleCache,IID_IOleCache);
#endif
#if defined(__IOleCache2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleCache2,IID_IOleCache2);
#endif
#if defined(__IOleCacheControl_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleCacheControl,IID_IOleCacheControl);
#endif
#if defined(__IOleClientSite_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleClientSite,IID_IOleClientSite);
#endif
#if defined(__IOleCommandTarget_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleCommandTarget,IID_IOleCommandTarget);
#endif
#if defined(__IOleContainer_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleContainer,IID_IOleContainer);
#endif
#if defined(__IOleControl_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleControl,IID_IOleControl);
#endif
#if defined(__IOleControlSite_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleControlSite,IID_IOleControlSite);
#endif
#if defined(__IOleDocument_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleDocument,IID_IOleDocument);
#endif
#if defined(__IOleDocumentSite_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleDocumentSite,IID_IOleDocumentSite);
#endif
#if defined(__IOleDocumentView_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleDocumentView,IID_IOleDocumentView);
#endif
#if defined(__IOleInPlaceActiveObject_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleInPlaceActiveObject,IID_IOleInPlaceActiveObject);
#endif
#if defined(__IOleInPlaceFrame_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleInPlaceFrame,IID_IOleInPlaceFrame);
#endif
#if defined(__IOleInPlaceObject_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleInPlaceObject,IID_IOleInPlaceObject);
#endif
#if defined(__IOleInPlaceObjectWindowless_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleInPlaceObjectWindowless,IID_IOleInPlaceObjectWindowless);
#endif
#if defined(__IOleInPlaceSite_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleInPlaceSite,IID_IOleInPlaceSite);
#endif
#if defined(__IOleInPlaceSiteEx_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleInPlaceSiteEx,IID_IOleInPlaceSiteEx);
#endif
#if defined(__IOleInPlaceSiteWindowless_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleInPlaceSiteWindowless,IID_IOleInPlaceSiteWindowless);
#endif
#if defined(__IOleInPlaceUIWindow_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleInPlaceUIWindow,IID_IOleInPlaceUIWindow);
#endif
#if defined(__IOleItemContainer_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleItemContainer,IID_IOleItemContainer);
#endif
#if defined(__IOleLink_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleLink,IID_IOleLink);
#endif
#if defined(__IOleObject_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleObject,IID_IOleObject);
#endif
#if defined(__IOleParentUndoUnit_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleParentUndoUnit,IID_IOleParentUndoUnit);
#endif
#if defined(__IOleUndoManager_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleUndoManager,IID_IOleUndoManager);
#endif
#if defined(__IOleUndoUnit_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleUndoUnit,IID_IOleUndoUnit);
#endif
#if defined(__IOleWindow_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOleWindow,IID_IOleWindow);
#endif
#if defined(__IOmHistory_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOmHistory,IID_IOmHistory);
#endif
#if defined(__IOmNavigator_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOmNavigator,IID_IOmNavigator);
#endif
#if defined(__IOplockStorage_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IOplockStorage,IID_IOplockStorage);
#endif
#if defined(__IPSFactoryBuffer_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPSFactoryBuffer,IID_IPSFactoryBuffer);
#endif
#if defined(__IParseDisplayName_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IParseDisplayName,IID_IParseDisplayName);
#endif
#if defined(__IPerPropertyBrowsing_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPerPropertyBrowsing,IID_IPerPropertyBrowsing);
#endif
#if defined(__IPersist_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPersist,IID_IPersist);
#endif
#if defined(__IPersistFile_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPersistFile,IID_IPersistFile);
#endif
#if defined(__IPersistFolder_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPersistFolder,IID_IPersistFolder);
#endif
#if defined(__IPersistFolder2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPersistFolder2,IID_IPersistFolder2);
#endif
#if defined(__IPersistHistory_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPersistHistory,IID_IPersistHistory);
#endif
#if defined(__IPersistMemory_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPersistMemory,IID_IPersistMemory);
#endif
#if defined(__IPersistMoniker_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPersistMoniker,IID_IPersistMoniker);
#endif
#if defined(__IPersistPropertyBag_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPersistPropertyBag,IID_IPersistPropertyBag);
#endif
#if defined(__IPersistPropertyBag2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPersistPropertyBag2,IID_IPersistPropertyBag2);
#endif
#if defined(__IPersistStorage_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPersistStorage,IID_IPersistStorage);
#endif
#if defined(__IPersistStream_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPersistStream,IID_IPersistStream);
#endif
#if defined(__IPersistStreamInit_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPersistStreamInit,IID_IPersistStreamInit);
#endif
#if defined(__IPicture_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPicture,IID_IPicture);
#endif
#if defined(__IPictureDisp_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPictureDisp,IID_IPictureDisp);
#endif
#if defined(__IPipeByte_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPipeByte,IID_IPipeByte);
#endif
#if defined(__IPipeDouble_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPipeDouble,IID_IPipeDouble);
#endif
#if defined(__IPipeLong_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPipeLong,IID_IPipeLong);
#endif
#if defined(__IPointerInactive_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPointerInactive,IID_IPointerInactive);
#endif
#if defined(__IPrint_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPrint,IID_IPrint);
#endif
#if defined(__IProgressNotify_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IProgressNotify,IID_IProgressNotify);
#endif
#if defined(__IPropertyBag_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPropertyBag,IID_IPropertyBag);
#endif
#if defined(__IPropertyBag2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPropertyBag2,IID_IPropertyBag2);
#endif
#if defined(__IPropertyNotifySink_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPropertyNotifySink,IID_IPropertyNotifySink);
#endif
#if defined(__IPropertyPage_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPropertyPage,IID_IPropertyPage);
#endif
#if defined(__IPropertyPage2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPropertyPage2,IID_IPropertyPage2);
#endif
#if defined(__IPropertyPageSite_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPropertyPageSite,IID_IPropertyPageSite);
#endif
#if defined(__IPropertySetStorage_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPropertySetStorage,IID_IPropertySetStorage);
#endif
#if defined(__IPropertyStorage_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IPropertyStorage,IID_IPropertyStorage);
#endif
#if defined(__IProvideClassInfo_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IProvideClassInfo,IID_IProvideClassInfo);
#endif
#if defined(__IProvideClassInfo2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IProvideClassInfo2,IID_IProvideClassInfo2);
#endif
#if defined(__IProvideMultipleClassInfo_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IProvideMultipleClassInfo,IID_IProvideMultipleClassInfo);
#endif
#if defined(__IQuickActivate_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IQuickActivate,IID_IQuickActivate);
#endif
#if defined(__IROTData_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IROTData,IID_IROTData);
#endif
#if defined(__IRecordInfo_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IRecordInfo,IID_IRecordInfo);
#endif
#if defined(__IReleaseMarshalBuffers_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IReleaseMarshalBuffers,IID_IReleaseMarshalBuffers);
#endif
#if defined(__IRootStorage_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IRootStorage,IID_IRootStorage);
#endif
#if defined(__IRpcChannelBuffer_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IRpcChannelBuffer,IID_IRpcChannelBuffer);
#endif
#if defined(__IRpcChannelBuffer2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IRpcChannelBuffer2,IID_IRpcChannelBuffer2);
#endif
#if defined(__IRpcChannelBuffer3_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IRpcChannelBuffer3,IID_IRpcChannelBuffer3);
#endif
#if defined(__IRpcHelper_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IRpcHelper,IID_IRpcHelper);
#endif
#if defined(__IRpcOptions_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IRpcOptions,IID_IRpcOptions);
#endif
#if defined(__IRpcProxyBuffer_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IRpcProxyBuffer,IID_IRpcProxyBuffer);
#endif
#if defined(__IRpcStubBuffer_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IRpcStubBuffer,IID_IRpcStubBuffer);
#endif
#if defined(__IRpcSyntaxNegotiate_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IRpcSyntaxNegotiate,IID_IRpcSyntaxNegotiate);
#endif
#if defined(__IRunnableObject_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IRunnableObject,IID_IRunnableObject);
#endif
#if defined(__IRunningObjectTable_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IRunningObjectTable,IID_IRunningObjectTable);
#endif
#if defined(__ISequentialStream_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ISequentialStream,IID_ISequentialStream);
#endif
#if defined(__IServerSecurity_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IServerSecurity,IID_IServerSecurity);
#endif
#if defined(__IServiceProvider_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IServiceProvider,IID_IServiceProvider);
#endif
#if defined(__IShellBrowser_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IShellBrowser,IID_IShellBrowser);
#endif
#if defined(__IShellDispatch_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IShellDispatch,IID_IShellDispatch);
#endif
#if defined(__IShellExtInit_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IShellExtInit,IID_IShellExtInit);
#endif
#if defined(__IShellFolder_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IShellFolder,IID_IShellFolder);
#endif
#if defined(__IShellFolderViewDual_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IShellFolderViewDual,IID_IShellFolderViewDual);
#endif
#if defined(__IShellLinkA_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IShellLinkA,IID_IShellLinkA);
#endif
#if defined(__IShellLinkDual_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IShellLinkDual,IID_IShellLinkDual);
#endif
#if defined(__IShellLinkW_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IShellLinkW,IID_IShellLinkW);
#endif
#if defined(__IShellPropSheetExt_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IShellPropSheetExt,IID_IShellPropSheetExt);
#endif
#if defined(__IShellUIHelper_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IShellUIHelper,IID_IShellUIHelper);
#endif
#if defined(__IShellView_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IShellView,IID_IShellView);
#endif
#if defined(__IShellView2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IShellView2,IID_IShellView2);
#endif
#if defined(__IShellWindows_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IShellWindows,IID_IShellWindows);
#endif
#if defined(__ISimpleFrameSite_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ISimpleFrameSite,IID_ISimpleFrameSite);
#endif
#if defined(__ISoftDistExt_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ISoftDistExt,IID_ISoftDistExt);
#endif
#if defined(__ISpecifyPropertyPages_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ISpecifyPropertyPages,IID_ISpecifyPropertyPages);
#endif
#if defined(__IStdMarshalInfo_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IStdMarshalInfo,IID_IStdMarshalInfo);
#endif
#if defined(__IStorage_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IStorage,IID_IStorage);
#endif
#if defined(__IStream_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IStream,IID_IStream);
#endif
#if defined(__ISubscriptionMgr_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ISubscriptionMgr,IID_ISubscriptionMgr);
#endif
#if defined(__ISupportErrorInfo_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ISupportErrorInfo,IID_ISupportErrorInfo);
#endif
#if defined(__ISurrogate_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ISurrogate,IID_ISurrogate);
#endif
#if defined(__ISynchronize_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ISynchronize,IID_ISynchronize);
#endif
#if defined(__ISynchronizeContainer_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ISynchronizeContainer,IID_ISynchronizeContainer);
#endif
#if defined(__ISynchronizeEvent_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ISynchronizeEvent,IID_ISynchronizeEvent);
#endif
#if defined(__ISynchronizeHandle_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ISynchronizeHandle,IID_ISynchronizeHandle);
#endif
#if defined(__ISynchronizeMutex_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ISynchronizeMutex,IID_ISynchronizeMutex);
#endif
#if defined(__IThumbnailExtractor_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IThumbnailExtractor,IID_IThumbnailExtractor);
#endif
#if defined(__ITimeAndNoticeControl_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ITimeAndNoticeControl,IID_ITimeAndNoticeControl);
#endif
#if defined(__ITimer_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ITimer,IID_ITimer);
#endif
#if defined(__ITimerService_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ITimerService,IID_ITimerService);
#endif
#if defined(__ITimerSink_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ITimerSink,IID_ITimerSink);
#endif
#if defined(__ITypeChangeEvents_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ITypeChangeEvents,IID_ITypeChangeEvents);
#endif
#if defined(__ITypeComp_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ITypeComp,IID_ITypeComp);
#endif
#if defined(__ITypeFactory_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ITypeFactory,IID_ITypeFactory);
#endif
#if defined(__ITypeInfo_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ITypeInfo,IID_ITypeInfo);
#endif
#if defined(__ITypeInfo2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ITypeInfo2,IID_ITypeInfo2);
#endif
#if defined(__ITypeLib_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ITypeLib,IID_ITypeLib);
#endif
#if defined(__ITypeLib2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ITypeLib2,IID_ITypeLib2);
#endif
#if defined(__ITypeMarshal_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(ITypeMarshal,IID_ITypeMarshal);
#endif
#if defined(__IUnknown_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IUnknown,IID_IUnknown);
#endif
#if defined(__IUrlHistoryNotify_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IUrlHistoryNotify,IID_IUrlHistoryNotify);
#endif
#if defined(__IUrlHistoryStg_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IUrlHistoryStg,IID_IUrlHistoryStg);
#endif
#if defined(__IUrlHistoryStg2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IUrlHistoryStg2,IID_IUrlHistoryStg2);
#endif
#if defined(__IUrlMon_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IUrlMon,IID_IUrlMon);
#endif
#if defined(__IVariantChangeType_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IVariantChangeType,IID_IVariantChangeType);
#endif
#if defined(__IViewObject_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IViewObject,IID_IViewObject);
#endif
#if defined(__IViewObject2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IViewObject2,IID_IViewObject2);
#endif
#if defined(__IViewObjectEx_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IViewObjectEx,IID_IViewObjectEx);
#endif
#if defined(__IWaitMultiple_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IWaitMultiple,IID_IWaitMultiple);
#endif
#if defined(__IWebBrowser_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IWebBrowser,IID_IWebBrowser);
#endif
#if defined(__IWebBrowser2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IWebBrowser2,IID_IWebBrowser2);
#endif
#if defined(__IWebBrowserApp_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IWebBrowserApp,IID_IWebBrowserApp);
#endif
#if defined(__IWinInetHttpInfo_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IWinInetHttpInfo,IID_IWinInetHttpInfo);
#endif
#if defined(__IWinInetInfo_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IWinInetInfo,IID_IWinInetInfo);
#endif
#if defined(__IWindowForBindingUI_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IWindowForBindingUI,IID_IWindowForBindingUI);
#endif
#if defined(__IWrappedProtocol_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IWrappedProtocol,IID_IWrappedProtocol);
#endif
#if defined(__IXMLAttribute_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLAttribute,IID_IXMLAttribute);
#endif
#if defined(__IXMLDOMAttribute_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLDOMAttribute,IID_IXMLDOMAttribute);
#endif
#if defined(__IXMLDOMCDATASection_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLDOMCDATASection,IID_IXMLDOMCDATASection);
#endif
#if defined(__IXMLDOMCharacterData_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLDOMCharacterData,IID_IXMLDOMCharacterData);
#endif
#if defined(__IXMLDOMComment_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLDOMComment,IID_IXMLDOMComment);
#endif
#if defined(__IXMLDOMDocument_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLDOMDocument,IID_IXMLDOMDocument);
#endif
#if defined(__IXMLDOMDocumentFragment_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLDOMDocumentFragment,IID_IXMLDOMDocumentFragment);
#endif
#if defined(__IXMLDOMDocumentType_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLDOMDocumentType,IID_IXMLDOMDocumentType);
#endif
#if defined(__IXMLDOMElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLDOMElement,IID_IXMLDOMElement);
#endif
#if defined(__IXMLDOMEntity_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLDOMEntity,IID_IXMLDOMEntity);
#endif
#if defined(__IXMLDOMEntityReference_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLDOMEntityReference,IID_IXMLDOMEntityReference);
#endif
#if defined(__IXMLDOMImplementation_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLDOMImplementation,IID_IXMLDOMImplementation);
#endif
#if defined(__IXMLDOMNamedNodeMap_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLDOMNamedNodeMap,IID_IXMLDOMNamedNodeMap);
#endif
#if defined(__IXMLDOMNode_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLDOMNode,IID_IXMLDOMNode);
#endif
#if defined(__IXMLDOMNodeList_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLDOMNodeList,IID_IXMLDOMNodeList);
#endif
#if defined(__IXMLDOMNotation_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLDOMNotation,IID_IXMLDOMNotation);
#endif
#if defined(__IXMLDOMParseError_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLDOMParseError,IID_IXMLDOMParseError);
#endif
#if defined(__IXMLDOMProcessingInstruction_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLDOMProcessingInstruction,IID_IXMLDOMProcessingInstruction);
#endif
#if defined(__IXMLDOMText_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLDOMText,IID_IXMLDOMText);
#endif
#if defined(__IXMLDSOControl_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLDSOControl,IID_IXMLDSOControl);
#endif
#if defined(__IXMLDocument_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLDocument,IID_IXMLDocument);
#endif
#if defined(__IXMLDocument2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLDocument2,IID_IXMLDocument2);
#endif
#if defined(__IXMLElement_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLElement,IID_IXMLElement);
#endif
#if defined(__IXMLElement2_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLElement2,IID_IXMLElement2);
#endif
#if defined(__IXMLElementCollection_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLElementCollection,IID_IXMLElementCollection);
#endif
#if defined(__IXMLError_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLError,IID_IXMLError);
#endif
#if defined(__IXMLHttpRequest_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXMLHttpRequest,IID_IXMLHttpRequest);
#endif
#if defined(__IXTLRuntime_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(IXTLRuntime,IID_IXTLRuntime);
#endif
#if defined(__OLEDBSimpleProvider_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(OLEDBSimpleProvider,IID_OLEDBSimpleProvider);
#endif
#if defined(__OLEDBSimpleProviderListener_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(OLEDBSimpleProviderListener,IID_OLEDBSimpleProviderListener);
#endif
#if defined(__XMLDOMDocumentEvents_INTERFACE_DEFINED__)
_COM_SMARTPTR_TYPEDEF(XMLDOMDocumentEvents,IID_XMLDOMDocumentEvents);
#endif

#if defined(__DOMDocument_FWD_DEFINED__)
_COM_SMARTPTR_TYPEDEF(DOMDocument,IID_DOMDocument);
#endif
#if defined(__DOMFreeThreadedDocument_FWD_DEFINED__)
_COM_SMARTPTR_TYPEDEF(DOMFreeThreadedDocument,IID_DOMFreeThreadedDocument);
#endif
#if defined(__XMLDSOControl_FWD_DEFINED__)
_COM_SMARTPTR_TYPEDEF(XMLDSOControl,IID_XMLDSOControl);
#endif
#if defined(__XMLDocument_FWD_DEFINED__)
_COM_SMARTPTR_TYPEDEF(XMLDocument,IID_XMLDocument);
#endif
#if defined(__XMLHTTPRequest_FWD_DEFINED__)
_COM_SMARTPTR_TYPEDEF(XMLHTTPRequest,IID_XMLHTTPRequest);
#endif
#endif
#endif
