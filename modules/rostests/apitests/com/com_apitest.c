/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         COM interface test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "com_apitest.h"

#include <winreg.h>
#include <mshtmhst.h>
#include <shlwapi.h>
#include <commoncontrols.h>
#include <activscp.h>
#include <ndk/rtlfuncs.h>

#define NDEBUG
#include <debug.h>

#define myskip(c, ...) ((c) ? 0 : (skip(__VA_ARGS__), 1))
#define mytrace(...) do {       \
    int debug = winetest_debug; \
    winetest_debug = 1;         \
    trace(__VA_ARGS__);         \
    winetest_debug = debug;     \
} while (0)

typedef struct _KNOWN_INTERFACE
{
    const IID *iid;
    PCSTR name;
    PCWSTR wname;
    BOOLEAN (*IsRegistered)(ULONG version);
} KNOWN_INTERFACE;
typedef const KNOWN_INTERFACE *PCKNOWN_INTERFACE;

#undef ID_NAME
#define ID_NAME(c) &c, #c, L ## #c
#define ID_NAME_EX(c, d) &c, #d, L ## #d

BOOLEAN RegisteredAlways(ULONG version)            { return TRUE; }
BOOLEAN RegisteredNever(ULONG version)             { return FALSE; }
BOOLEAN RegisteredOnWS03AndVista(ULONG version)    { return version >= NTDDI_WS03 && version < NTDDI_WIN7; }
BOOLEAN RegisteredOnVistaOrNewer(ULONG version)    { return version >= NTDDI_VISTA; }
BOOLEAN RegisteredOnVistaOnly(ULONG version)       { return version >= NTDDI_VISTA && version < NTDDI_WIN7; }
BOOLEAN RegisteredOnVistaAndWin7(ULONG version)    { return version >= NTDDI_VISTA && version < NTDDI_WIN8; }
BOOLEAN RegisteredOnVistaToWin8Dot1(ULONG version) { return version >= NTDDI_VISTA && version < NTDDI_WIN10; }
BOOLEAN RegisteredOnWin7OrNewer(ULONG version)     { return version >= NTDDI_WIN7; }
BOOLEAN RegisteredOnWin8OrNewer(ULONG version)     { return version >= NTDDI_WIN8; }
BOOLEAN RegisteredOnWS03OrOlder(ULONG version)     { return version <= NTDDI_WS03SP4; }

static KNOWN_INTERFACE KnownInterfaces[] =
{
    { ID_NAME(DIID__SearchAssistantEvents),        RegisteredOnWS03OrOlder },
    { ID_NAME(IID_IMarshal),                       RegisteredOnWS03OrOlder },
    { ID_NAME(IID_ICDBurnPriv),                    RegisteredOnWS03OrOlder },
    { ID_NAME(IID_ICompositeFolder),               RegisteredOnWS03OrOlder },
    { ID_NAME(IID_IDefViewSafety),                 RegisteredOnWS03OrOlder },
    { ID_NAME(IID_IDropSource),                    RegisteredOnWS03OrOlder },
    { ID_NAME(IID_ISearch),                        RegisteredOnWS03OrOlder },
    { ID_NAME(IID_ISearchAssistantOC),             RegisteredOnWS03OrOlder },
    { ID_NAME(IID_ISearchAssistantOC2),            RegisteredOnWS03OrOlder },
    { ID_NAME(IID_ISearchAssistantOC3),            RegisteredOnWS03OrOlder },
    { ID_NAME(IID_ISearchBar),                     RegisteredOnWS03OrOlder },
    { ID_NAME(IID_ISearches),                      RegisteredOnWS03OrOlder },
    { ID_NAME(IID_IEFrameAuto),                    RegisteredOnWS03OrOlder },
    { ID_NAME(IID_IWebBrowserPriv2),               RegisteredOnWS03OrOlder },
    /* These interfaces are different between PSDK and registry/shell32 */
    { ID_NAME_EX(IID_ITransferAdviseSinkPriv,
                 IID_ITransferAdviseSink),         RegisteredOnWS03OrOlder },
    { ID_NAME_EX(IID_IDriveFolderExtOld,
                 IID_IDriveFolderExt),             RegisteredOnWS03OrOlder },

    { ID_NAME(IID_IAggregateFilterCondition),      RegisteredOnWS03AndVista },
    { ID_NAME(IID_IBandNavigate),                  RegisteredOnWS03AndVista },

    { ID_NAME(IID_IAccIdentity),                   RegisteredAlways },
    { ID_NAME(IID_IAccPropServer),                 RegisteredAlways },
    { ID_NAME(IID_IAccPropServices),               RegisteredAlways },
    { ID_NAME(IID_IAccessible),                    RegisteredAlways },
    { ID_NAME(IID_IAccessibleHandler),             RegisteredAlways },
    { ID_NAME(IID_IAccessor),                      RegisteredAlways },
    { ID_NAME(IID_IActionProgress),                RegisteredAlways },
    { ID_NAME(IID_IActionProgressDialog),          RegisteredAlways },
    { ID_NAME(IID_IAutoCompleteDropDown),          RegisteredAlways },
    { ID_NAME(IID_IBandHost),                      RegisteredAlways },
    { ID_NAME(IID_IBandSite),                      RegisteredAlways },
    { ID_NAME(IID_IBindCtx),                       RegisteredAlways },
    { ID_NAME(IID_IBindEventHandler),              RegisteredAlways },
    { ID_NAME(IID_IBindHost),                      RegisteredAlways },
    { ID_NAME(IID_IBinding),                       RegisteredAlways },
    { ID_NAME(IID_IBindResource),                  RegisteredAlways },
    { ID_NAME(IID_IBindStatusCallback),            RegisteredAlways },
    { ID_NAME(IID_IBlockingLock),                  RegisteredAlways },
    { ID_NAME(IID_IBrowserService),                RegisteredAlways },
    { ID_NAME(IID_ICDBurn),                        RegisteredAlways },
    { ID_NAME(IID_ICDBurnExt),                     RegisteredAlways },
    { ID_NAME(IID_ICatInformation),                RegisteredAlways },
    { ID_NAME(IID_ICatRegister),                   RegisteredAlways },
    { ID_NAME(IID_IClassActivator),                RegisteredAlways },
    { ID_NAME(IID_IClassFactory),                  RegisteredAlways },
    { ID_NAME(IID_IClassFactory2),                 RegisteredAlways },
    { ID_NAME(IID_ICommDlgBrowser),                RegisteredAlways },
    { ID_NAME(IID_ICommDlgBrowser2),               RegisteredAlways },
    { ID_NAME(IID_ICommDlgBrowser3),               RegisteredAlways },
    { ID_NAME(IID_IComputerInfoChangeNotify),      RegisteredAlways },
    { ID_NAME(IID_IConnectionPoint),               RegisteredAlways },
    { ID_NAME(IID_IConnectionPointContainer),      RegisteredAlways },
    { ID_NAME(IID_IContextMenuSite),               RegisteredAlways },
    { ID_NAME(IID_IContinue),                      RegisteredAlways },
    { ID_NAME(IID_IContinueCallback),              RegisteredAlways },
    { ID_NAME(IID_ICustomizeInfoTip),              RegisteredAlways },
    { ID_NAME(IID_IDataObject),                    RegisteredAlways },
    { ID_NAME(IID_IDefViewFrame3),                 RegisteredAlways },
    { ID_NAME(IID_IDefViewFrameGroup),             RegisteredAlways },
    { ID_NAME(IID_IDeskBand),                      RegisteredAlways },
    { ID_NAME(IID_IDeskBandEx),                    RegisteredAlways },
    // { ID_NAME(IID_IDefViewID),                     RegisteredAlways }, == DefViewFrame3
    { ID_NAME(IID_IDiscMasterProgressEvents),      RegisteredAlways },
    { ID_NAME(IID_IDispatch),                      RegisteredAlways },
    { ID_NAME(IID_IDispatchEx),                    RegisteredAlways },
    { ID_NAME(IID_IDockingWindow),                 RegisteredAlways },
    { ID_NAME(IID_IDropTarget),                    RegisteredAlways },
    // { ID_NAME(IID_IEnumCATID),                     RegisteredAlways }, == EnumGUID
    // { ID_NAME(IID_IEnumCLSID),                     RegisteredAlways }, == EnumGUID
    { ID_NAME(IID_IEnumCATEGORYINFO),              RegisteredAlways },
    { ID_NAME(IID_IEnumConnectionPoints),          RegisteredAlways },
    { ID_NAME(IID_IEnumConnections),               RegisteredAlways },
    { ID_NAME(IID_IEnumExtraSearch),               RegisteredAlways },
    { ID_NAME(IID_IEnumGUID),                      RegisteredAlways },
    { ID_NAME(IID_IEnumIDList),                    RegisteredAlways },
    { ID_NAME(IID_IEnumMoniker),                   RegisteredAlways },
    { ID_NAME(IID_IEnumNetConnection),             RegisteredAlways },
    { ID_NAME(IID_IEnumShellItems),                RegisteredAlways },
    { ID_NAME(IID_IEnumSTATSTG),                   RegisteredAlways },
    { ID_NAME(IID_IEnumString),                    RegisteredAlways },
    { ID_NAME(IID_IEnumUnknown),                   RegisteredAlways },
    { ID_NAME(IID_IEnumVARIANT),                   RegisteredAlways },
    { ID_NAME(IID_IErrorLog),                      RegisteredAlways },
    { ID_NAME(IID_IExplorerBrowser),               RegisteredAlways },
    { ID_NAME(IID_IExtractImage),                  RegisteredAlways },
    { ID_NAME(IID_IExtractImage2),                 RegisteredAlways },
    { ID_NAME(IID_IFileDialog),                    RegisteredAlways },
    { ID_NAME(IID_IFileOpenDialog),                RegisteredAlways },
    { ID_NAME(IID_IFileSaveDialog),                RegisteredAlways },
    { ID_NAME(IID_IFileSearchBand),                RegisteredAlways },
    { ID_NAME(IID_IFilter),                        RegisteredAlways },
    { ID_NAME(IID_IFilterCondition),               RegisteredAlways },
    { ID_NAME(IID_IFolderBandPriv),                RegisteredAlways },
    { ID_NAME(IID_IFolderFilter),                  RegisteredAlways },
    { ID_NAME(IID_IFolderFilterSite),              RegisteredAlways },
    { ID_NAME(IID_IFolderView),                    RegisteredAlways },
    { ID_NAME(IID_IFolderView2),                   RegisteredAlways },
    { ID_NAME(IID_IFolderViewOC),                  RegisteredAlways },
    { ID_NAME(IID_IFolderViewSettings),            RegisteredAlways },
    { ID_NAME(IID_IHWEventHandler),                RegisteredAlways },
    { ID_NAME(IID_IHWEventHandler2),               RegisteredAlways },
    { ID_NAME(IID_IHlinkFrame),                    RegisteredAlways },
    { ID_NAME(IID_IInitializeWithBindCtx),         RegisteredAlways },
    { ID_NAME(IID_IInitializeWithFile),            RegisteredAlways },
    { ID_NAME(IID_IInputObject),                   RegisteredAlways },
    { ID_NAME(IID_IInputObjectSite),               RegisteredAlways },
    { ID_NAME(IID_IInternetSecurityManager),       RegisteredAlways },
    { ID_NAME(IID_IItemNameLimits),                RegisteredAlways },
    { ID_NAME(IID_IModalWindow),                   RegisteredAlways },
    { ID_NAME(IID_IMoniker),                       RegisteredAlways },
    { ID_NAME(IID_INamespaceWalk),                 RegisteredAlways },
    { ID_NAME(IID_INamespaceWalkCB),               RegisteredAlways },
    { ID_NAME(IID_INamespaceWalkCB2),              RegisteredAlways },
    { ID_NAME(IID_INetConnectionManager),          RegisteredAlways },
    { ID_NAME(IID_INewMenuClient),                 RegisteredAlways },
    { ID_NAME(IID_INewWindowManager),              RegisteredAlways },
    { ID_NAME(IID_IObjectSafety),                  RegisteredAlways },
    { ID_NAME(IID_IObjectWithBackReferences),      RegisteredAlways },
    { ID_NAME(IID_IObjectWithSite),                RegisteredAlways },
    { ID_NAME(IID_IOleClientSite),                 RegisteredAlways },
    { ID_NAME(IID_IOleCommandTarget),              RegisteredAlways },
    { ID_NAME(IID_IOleContainer),                  RegisteredAlways },
    { ID_NAME(IID_IOleControl),                    RegisteredAlways },
    { ID_NAME(IID_IOleControlSite),                RegisteredAlways },
    { ID_NAME(IID_IOleInPlaceActiveObject),        RegisteredAlways },
    { ID_NAME(IID_IOleInPlaceFrame),               RegisteredAlways },
    { ID_NAME(IID_IOleInPlaceObject),              RegisteredAlways },
    { ID_NAME(IID_IOleInPlaceSite),                RegisteredAlways },
    { ID_NAME(IID_IOleInPlaceSiteEx),              RegisteredAlways },
    { ID_NAME(IID_IOleInPlaceUIWindow),            RegisteredAlways },
    { ID_NAME(IID_IOleItemContainer),              RegisteredAlways },
    { ID_NAME(IID_IOleLink),                       RegisteredAlways },
    { ID_NAME(IID_IOleObject),                     RegisteredAlways },
    { ID_NAME(IID_IOleWindow),                     RegisteredAlways },
    { ID_NAME(IID_IParentAndItem),                 RegisteredAlways },
    { ID_NAME(IID_IParseDisplayName),              RegisteredAlways },
    { ID_NAME(IID_IPersist),                       RegisteredAlways },
    { ID_NAME(IID_IPersistFile),                   RegisteredAlways },
    { ID_NAME(IID_IPersistFolder),                 RegisteredAlways },
    { ID_NAME(IID_IPersistFolder2),                RegisteredAlways },
    { ID_NAME(IID_IPersistFolder3),                RegisteredAlways },
    { ID_NAME(IID_IPersistHistory),                RegisteredAlways },
    { ID_NAME(IID_IPersistIDList),                 RegisteredAlways },
    { ID_NAME(IID_IPersistMemory),                 RegisteredAlways },
    { ID_NAME(IID_IPersistPropertyBag),            RegisteredAlways },
    { ID_NAME(IID_IPersistPropertyBag2),           RegisteredAlways },
    { ID_NAME(IID_IPersistStorage),                RegisteredAlways },
    { ID_NAME(IID_IPersistStream),                 RegisteredAlways },
    { ID_NAME(IID_IPersistStreamInit),             RegisteredAlways },
    { ID_NAME(IID_IPreviewHandler),                RegisteredAlways },
    { ID_NAME(IID_IPreviewHandlerFrame),           RegisteredAlways },
    { ID_NAME(IID_IPreviewHandlerVisuals),         RegisteredAlways },
    { ID_NAME(IID_IPropertyBag),                   RegisteredAlways },
    { ID_NAME(IID_IPropertyBag2),                  RegisteredAlways },
    { ID_NAME(IID_IPropertySetStorage),            RegisteredAlways },
    { ID_NAME(IID_IPropertyStore),                 RegisteredAlways },
    { ID_NAME(IID_IProvideClassInfo),              RegisteredAlways },
    { ID_NAME(IID_IProvideClassInfo2),             RegisteredAlways },
    { ID_NAME(IID_IQueryCancelAutoPlay),           RegisteredAlways },
    { ID_NAME(IID_IQuickActivate),                 RegisteredAlways },
    { ID_NAME(IID_IRemoteComputer),                RegisteredAlways },
    { ID_NAME(IID_IResolveShellLink),              RegisteredAlways },
    { ID_NAME(IID_IROTData),                       RegisteredAlways },
    { ID_NAME(IID_IRunnableObject),                RegisteredAlways },
    { ID_NAME(IID_IRunningObjectTable),            RegisteredAlways },
    { ID_NAME(IID_IScriptErrorList),               RegisteredAlways },
    { ID_NAME(IID_ISecMgrCacheSeedTarget),         RegisteredAlways },
    { ID_NAME(IID_IServiceProvider),               RegisteredAlways },
    { ID_NAME(IID_IShellBrowser),                  RegisteredAlways },
    { ID_NAME(IID_IShellDispatch),                 RegisteredAlways },
    { ID_NAME(IID_IShellDispatch2),                RegisteredAlways },
    { ID_NAME(IID_IShellDispatch3),                RegisteredAlways },
    { ID_NAME(IID_IShellDispatch4),                RegisteredAlways },
    { ID_NAME(IID_IShellFavoritesNameSpace),       RegisteredAlways },
    { ID_NAME(IID_IShellFolder),                   RegisteredAlways },
    { ID_NAME(IID_IShellFolder2),                  RegisteredAlways },
    { ID_NAME(IID_IShellFolderViewDual),           RegisteredAlways },
    { ID_NAME(IID_IShellFolderViewDual2),          RegisteredAlways },
    { ID_NAME(IID_IShellIcon),                     RegisteredAlways },
    { ID_NAME(IID_IShellItem),                     RegisteredAlways },
    { ID_NAME(IID_IShellItem2),                    RegisteredAlways },
    { ID_NAME(IID_IShellItemArray),                RegisteredAlways },
    { ID_NAME(IID_IShellItemFilter),               RegisteredAlways },
    { ID_NAME(IID_IShellLinkA),                    RegisteredAlways },
    { ID_NAME(IID_IShellLinkDual),                 RegisteredAlways },
    { ID_NAME(IID_IShellLinkDual2),                RegisteredAlways },
    { ID_NAME(IID_IShellLinkW),                    RegisteredAlways },
    { ID_NAME(IID_IShellNameSpace),                RegisteredAlways },
    { ID_NAME(IID_IShellUIHelper),                 RegisteredAlways },
    { ID_NAME(IID_IShellView),                     RegisteredAlways },
    { ID_NAME(IID_IShellView2),                    RegisteredAlways },
    { ID_NAME(IID_IShellView3),                    RegisteredAlways },
    { ID_NAME(IID_IShellWindows),                  RegisteredAlways },
    { ID_NAME(IID_ISpecifyPropertyPages),          RegisteredAlways },
    { ID_NAME(IID_IStorage),                       RegisteredAlways },
    { ID_NAME(IID_IStream),                        RegisteredAlways },
    { ID_NAME(IID_ISurrogate),                     RegisteredAlways },
    { ID_NAME(IID_ISynchronize),                   RegisteredAlways },
    { ID_NAME(IID_ITargetEmbedding),               RegisteredAlways },
    { ID_NAME(IID_ITargetFrame),                   RegisteredAlways },
    { ID_NAME(IID_ITargetFrame2),                  RegisteredAlways },
    { ID_NAME(IID_ITargetFramePriv),               RegisteredAlways },
    { ID_NAME(IID_ITargetFramePriv2),              RegisteredAlways },
    { ID_NAME(IID_ITargetNotify),                  RegisteredAlways },
    { ID_NAME(IID_ITaskbarList),                   RegisteredAlways },
    { ID_NAME(IID_ITaskbarList2),                  RegisteredAlways },
    { ID_NAME(IID_IUnknown),                       RegisteredAlways },
    { ID_NAME(IID_IUrlHistoryNotify),              RegisteredAlways },
    { ID_NAME(IID_IUrlHistoryStg),                 RegisteredAlways },
    { ID_NAME(IID_IUrlHistoryStg2),                RegisteredAlways },
    { ID_NAME(IID_IViewObject),                    RegisteredAlways },
    { ID_NAME(IID_IViewObject2),                   RegisteredAlways },
    { ID_NAME(IID_IVisualProperties),              RegisteredAlways },
    { ID_NAME(IID_IWebBrowser),                    RegisteredAlways },
    { ID_NAME(IID_IWebBrowser2),                   RegisteredAlways },
    { ID_NAME(IID_IWebBrowserApp),                 RegisteredAlways },
    { ID_NAME(IID_IWebBrowserPriv),                RegisteredAlways },
    { ID_NAME(DIID_DShellFolderViewEvents),        RegisteredAlways },
    { ID_NAME(DIID_DShellNameSpaceEvents),         RegisteredAlways },
    { ID_NAME(DIID_DShellWindowsEvents),           RegisteredAlways },
    { ID_NAME(DIID_DWebBrowserEvents),             RegisteredAlways },
    { ID_NAME(DIID_DWebBrowserEvents2),            RegisteredAlways },
    { ID_NAME(DIID_XMLDOMDocumentEvents),          RegisteredAlways },
    { ID_NAME(IID_Folder),                         RegisteredAlways },
    { ID_NAME(IID_Folder2),                        RegisteredAlways },
    { ID_NAME(IID_Folder3),                        RegisteredAlways },
    { ID_NAME(IID_FolderItem),                     RegisteredAlways },
    { ID_NAME(IID_FolderItem2),                    RegisteredAlways },
    { ID_NAME(IID_FolderItems),                    RegisteredAlways },
    { ID_NAME(IID_FolderItems2),                   RegisteredAlways },
    { ID_NAME(IID_FolderItems3),                   RegisteredAlways },
    { ID_NAME(IID_FolderItemVerb),                 RegisteredAlways },
    { ID_NAME(IID_FolderItemVerbs),                RegisteredAlways },
    { ID_NAME(IID_IQueryContinue),                 RegisteredAlways },
    { ID_NAME(IID_IUserNotification),              RegisteredAlways },
    { ID_NAME(IID_IUserNotificationCallback),      RegisteredAlways },
    { ID_NAME(IID_IUserNotification2),             RegisteredAlways },
    { ID_NAME(IID_IUserEventTimer),                RegisteredAlways },
    { ID_NAME(IID_IUserEventTimerCallback),        RegisteredAlways },

    { ID_NAME(IID_IControlPanelEnumerator),        RegisteredOnVistaOnly    },
    { ID_NAME(IID_IShellFolder3),                  RegisteredOnVistaOnly    },
    { ID_NAME_EX(IID_IShellBrowserService4,
                 IID_IShellBrowserService),        RegisteredOnVistaOnly    },

    { ID_NAME(IID_IDriveFolderExt),                RegisteredOnVistaOrNewer },
    { ID_NAME(IID_INetConnectionConnectUi),        RegisteredOnVistaOrNewer },
    { ID_NAME(IID_INetConnectionCommonUi2),        RegisteredOnVistaOrNewer }, // This also covers IID_INetLanConnectionUiInfo.
    { ID_NAME(IID_ISLTracker),                     RegisteredOnVistaOrNewer },
    { ID_NAME(IID_IShellDispatch5),                RegisteredOnVistaOrNewer },
    { ID_NAME(IID_IShellFolderViewDual3),          RegisteredOnVistaOrNewer },
    { ID_NAME(IID_IShellLinkDataList),             RegisteredOnVistaOrNewer },
    { ID_NAME(IID_IShellUIHelper2),                RegisteredOnVistaOrNewer },
    { ID_NAME(IID_IBackReferencedObject),          RegisteredOnVistaOrNewer },
    { ID_NAME(IID_ICommonLayoutDefinition),        RegisteredOnVistaOrNewer },
    { ID_NAME(IID_IDelegateHostItemContainer),     RegisteredOnVistaOrNewer },
    { ID_NAME(IID_IExecuteCommand),                RegisteredOnVistaOrNewer },
    { ID_NAME(IID_IFolderType),                    RegisteredOnVistaOrNewer },
    { ID_NAME(IID_IFrameLayoutDefinition),         RegisteredOnVistaOrNewer },
    { ID_NAME(IID_IItemFilter),                    RegisteredOnVistaOrNewer },
    { ID_NAME(IID_IItemFilterOwner),               RegisteredOnVistaOrNewer },
    { ID_NAME(IID_INewItemAdvisor),                RegisteredOnVistaOrNewer },
    { ID_NAME(IID_IObjectWithAssociationElement),  RegisteredOnVistaOrNewer },
    { ID_NAME(IID_IObjectWithQuerySource),         RegisteredOnVistaOrNewer },
    { ID_NAME(IID_IObjectWithSelection),           RegisteredOnVistaOrNewer },
    { ID_NAME(IID_IPersistString2),                RegisteredOnVistaOrNewer },
    { ID_NAME(IID_IRootAndRelativeParsingFolder),  RegisteredOnVistaOrNewer },

    { ID_NAME(IID_IAssociationArray),              RegisteredOnVistaAndWin7 },
    { ID_NAME(IID_IObjectWithAssociationList),     RegisteredOnVistaAndWin7 },

    { ID_NAME(IID_IRegItemFolder),                 RegisteredOnVistaToWin8Dot1 },

    { ID_NAME(IID_IFileDialog2),                   RegisteredOnWin7OrNewer },
    { ID_NAME(IID_INameSpaceTreeControl),          RegisteredOnWin7OrNewer },
    { ID_NAME(IID_ITaskbarList3),                  RegisteredOnWin7OrNewer },
    { ID_NAME(IID_ITaskbarList4),                  RegisteredOnWin7OrNewer },
    { ID_NAME(IID_IShellBrowserService),           RegisteredOnWin7OrNewer },

    { ID_NAME(IID_ITransferAdviseSink),            RegisteredOnWin8OrNewer },

    { ID_NAME(IID_IACList),                        RegisteredNever  },
    { ID_NAME(IID_IACList2),                       RegisteredNever  },
    { ID_NAME(IID_IADesktopP2),                    RegisteredNever  },
    { ID_NAME(IID_IAccessControl),                 RegisteredNever  },
    { ID_NAME(IID_IACLCustomMRU),                  RegisteredNever  },
    { ID_NAME(IID_IActiveDesktop),                 RegisteredNever  },
    { ID_NAME(IID_IActiveDesktopP),                RegisteredNever  },
    { ID_NAME(IID_IAddressBarParser),              RegisteredNever  },
    { ID_NAME(IID_IAddressBand),                   RegisteredNever  },
    { ID_NAME(IID_IAddressEditBox),                RegisteredNever  },
    { ID_NAME(IID_IAsyncMoniker),                  RegisteredNever  },
    { ID_NAME(IID_IAugmentedShellFolder),          RegisteredNever  },
    { ID_NAME(IID_IAugmentedShellFolder2),         RegisteredNever  },
    { ID_NAME(IID_IAutoComplete),                  RegisteredNever  },
    { ID_NAME(IID_IAutoComplete2),                 RegisteredNever  },
    { ID_NAME(IID_IBandProxy),                     RegisteredNever  },
    { ID_NAME(IID_IBandSiteHelper),                RegisteredNever  },
    { ID_NAME(IID_IBanneredBar),                   RegisteredNever  },
    { ID_NAME(IID_IBindProtocol),                  RegisteredNever  },
    { ID_NAME(IID_IBrowserFrameOptions),           RegisteredNever  },
    { ID_NAME(IID_IBrowserService2),               RegisteredNever  },
    { ID_NAME(IID_IBrowserService3),               RegisteredNever  },
    { ID_NAME(IID_IBrowserService4),               RegisteredNever  },
    { ID_NAME(IID_ICallFactory),                   RegisteredNever  },
    { ID_NAME(IID_ICancelMethodCalls),             RegisteredNever  },
    { ID_NAME(IID_IClassFactory3),                 RegisteredNever  },
    { ID_NAME(IID_IClientSecurity),                RegisteredNever  },
    { ID_NAME(IID_IComThreadingInfo),              RegisteredNever  },
    { ID_NAME(IID_IContext),                       RegisteredNever  },
    { ID_NAME(IID_IContextMenu),                   RegisteredNever  },
    { ID_NAME(IID_IContextMenu2),                  RegisteredNever  },
    { ID_NAME(IID_IContextMenu3),                  RegisteredNever  },
    { ID_NAME(IID_IContextMenuCB),                 RegisteredNever  },
    { ID_NAME(IID_ICopyHookA),                     RegisteredNever  },
    { ID_NAME(IID_ICopyHookW),                     RegisteredNever  },
    { ID_NAME(IID_ICurrentWorkingDirectory),       RegisteredNever  },
    { ID_NAME(IID_IDVGetEnum),                     RegisteredNever  },
    { ID_NAME(IID_IDefViewFrame),                  RegisteredNever  },
    { ID_NAME(IID_IDefViewScript),                 RegisteredNever  },
    { ID_NAME(IID_IDelayedRelease),                RegisteredNever  },
    { ID_NAME(IID_IDeskBar),                       RegisteredNever  },
    { ID_NAME(IID_IDeskBarClient),                 RegisteredNever  },
    { ID_NAME(IID_IDeskMovr),                      RegisteredNever  },
    { ID_NAME(IID_IDockingWindowFrame),            RegisteredNever  },
    { ID_NAME(IID_IDockingWindowSite),             RegisteredNever  },
    { ID_NAME(IID_IDocViewSite),                   RegisteredNever  },
    { ID_NAME(IID_IDragSourceHelper),              RegisteredNever  },
    { ID_NAME(IID_IDropTargetHelper),              RegisteredNever  },
    { ID_NAME(IID_IEnumNetCfgComponent),           RegisteredNever  },
    { ID_NAME(IID_IExplorerToolbar),               RegisteredNever  },
    { ID_NAME(IID_IExtractIconA),                  RegisteredNever  },
    { ID_NAME(IID_IExtractIconW),                  RegisteredNever  },
    { ID_NAME(IID_IFileViewerA),                   RegisteredNever  },
    { ID_NAME(IID_IFileViewerSite),                RegisteredNever  },
    { ID_NAME(IID_IFileViewerW),                   RegisteredNever  },
    { ID_NAME(IID_IFolderViewHost),                RegisteredNever  },
    { ID_NAME(IID_IForegroundTransfer),            RegisteredNever  },
    { ID_NAME(IID_IGetNameSpaceExtensionPointer),  RegisteredNever  },
    { ID_NAME(IID_IGlobalFolderSettings),          RegisteredNever  },
    { ID_NAME(IID_IImageList),                     RegisteredNever  },
    { ID_NAME(IID_IImageList2),                    RegisteredNever  },
    { ID_NAME(IID_IInitializeObject),              RegisteredNever  },
    { ID_NAME(IID_IInternalUnknown),               RegisteredNever  },
    { ID_NAME(IID_IInternetZoneManager),           RegisteredNever  },
    { ID_NAME(IID_IMarshal2),                      RegisteredNever  },
    { ID_NAME(IID_IMenuBand),                      RegisteredNever  },
    { ID_NAME(IID_IMenuPopup),                     RegisteredNever  },
    { ID_NAME(IID_IMruDataList),                   RegisteredNever  },
    { ID_NAME(IID_IMruPidlList),                   RegisteredNever  },
    { ID_NAME(IID_IMultiMonitorDockingSite),       RegisteredNever  },
    { ID_NAME(IID_IMultiQI),                       RegisteredNever  },
    { ID_NAME(IID_INamespaceProxy),                RegisteredNever  },
    { ID_NAME(IID_INetCfg),                        RegisteredNever  },
    { ID_NAME(IID_INetCfgComponent),               RegisteredNever  },
    { ID_NAME(IID_INetCfgComponentBindings),       RegisteredNever  },
    { ID_NAME(IID_INetCfgComponentControl),        RegisteredNever  },
    { ID_NAME(IID_INetCfgComponentPropertyUi),     RegisteredNever  },
    { ID_NAME(IID_INetCfgLock),                    RegisteredNever  },
    { ID_NAME(IID_INetCfgPnpReconfigCallback),     RegisteredNever  },
    { ID_NAME(IID_INetConnectionPropertyUi),       RegisteredNever  },
    { ID_NAME(IID_INetConnectionPropertyUi2),      RegisteredNever  },
    { ID_NAME(IID_INewShortcutHookA),              RegisteredNever  },
    { ID_NAME(IID_INewShortcutHookW),              RegisteredNever  },
    { ID_NAME(IID_INSCTree),                       RegisteredNever  },
    { ID_NAME(IID_INSCTree2),                      RegisteredNever  },
    { ID_NAME(IID_IObjMgr),                        RegisteredNever  },
    { ID_NAME(IID_IOleInPlaceObjectWindowless),    RegisteredNever  },
    { ID_NAME(IID_IOleInPlaceSiteWindowless),      RegisteredNever  },
    { ID_NAME(IID_IPersistFreeThreadedObject),     RegisteredNever  },
    { ID_NAME(IID_IProgressDialog),                RegisteredNever  },
    { ID_NAME(IID_IPropSheetPage),                 RegisteredNever  },
    { ID_NAME(IID_IQueryAssociations),             RegisteredNever  },
    { ID_NAME(IID_IQueryInfo),                     RegisteredNever  },
    { ID_NAME(IID_IRegTreeOptions),                RegisteredNever  },
    { ID_NAME(IID_IRpcOptions),                    RegisteredNever  },
    { ID_NAME(IID_IServerSecurity),                RegisteredNever  },
    { ID_NAME(IID_IShellApp),                      RegisteredNever  },
    { ID_NAME(IID_IShellChangeNotify),             RegisteredNever  },
    { ID_NAME(IID_IShellCopyHookA),                RegisteredNever  },
    { ID_NAME(IID_IShellCopyHookW),                RegisteredNever  },
    { ID_NAME(IID_IShellDesktopTray),              RegisteredNever  },
    { ID_NAME(IID_IShellDetails),                  RegisteredNever  },
    { ID_NAME(IID_IShellDispatch6),                RegisteredNever  },
    { ID_NAME(IID_IShellExecuteHookA),             RegisteredNever  },
    { ID_NAME(IID_IShellExecuteHookW),             RegisteredNever  },
    { ID_NAME(IID_IShellExtInit),                  RegisteredNever  },
    { ID_NAME(IID_IShellFolderBand),               RegisteredNever  },
    { ID_NAME(IID_IShellFolderSearchable),         RegisteredNever  },
    { ID_NAME(IID_IShellFolderSearchableCallback), RegisteredNever  },
    { ID_NAME(IID_IShellFolderView),               RegisteredNever  },
    { ID_NAME(IID_IShellFolderViewCB),             RegisteredNever  },
    { ID_NAME(IID_IShellFolderViewType),           RegisteredNever  },
    { ID_NAME(IID_IShellIconOverlay),              RegisteredNever  },
    { ID_NAME(IID_IShellIconOverlayIdentifier),    RegisteredNever  },
    { ID_NAME(IID_IShellImageData),                RegisteredNever  },
    { ID_NAME(IID_IShellImageDataAbort),           RegisteredNever  },
    { ID_NAME(IID_IShellImageDataFactory),         RegisteredNever  },
    { ID_NAME(IID_IShellMenu),                     RegisteredNever  },
    { ID_NAME(IID_IShellMenu2),                    RegisteredNever  },
    { ID_NAME(IID_IShellMenuAcc),                  RegisteredNever  },
    { ID_NAME(IID_IShellMenuCallback),             RegisteredNever  },
    { ID_NAME(IID_IShellPropSheetExt),             RegisteredNever  },
    { ID_NAME(IID_IShellService),                  RegisteredNever  },
    { ID_NAME(IID_IShellTaskScheduler),            RegisteredNever  },
    { ID_NAME(IID_ISynchronizeContainer),          RegisteredNever  },
    { ID_NAME(IID_ISynchronizeEvent),              RegisteredNever  },
    { ID_NAME(IID_ISynchronizeHandle),             RegisteredNever  },
    { ID_NAME(IID_ITrackShellMenu),                RegisteredNever  },
    { ID_NAME(IID_ITransferDestination),           RegisteredNever  },
    { ID_NAME(IID_ITransferSource),                RegisteredNever  },
    { ID_NAME(IID_ITranslateShellChangeNotify),    RegisteredNever  },
    { ID_NAME(IID_ITrayPriv),                      RegisteredNever  },
    { ID_NAME(IID_ITrayPriv2),                     RegisteredNever  },
    { ID_NAME(IID_IURLSearchHook),                 RegisteredNever  },
    { ID_NAME(IID_IURLSearchHook2),                RegisteredNever  },
    { ID_NAME(IID_IViewObjectEx),                  RegisteredNever  },
    { ID_NAME(IID_IWinEventHandler),               RegisteredNever  },
    { ID_NAME(IID_DFConstraint),                   RegisteredNever  },
    { ID_NAME(IID_CDefView),                       RegisteredNever  },
    { ID_NAME(CLSID_ShellDesktop),                 RegisteredNever  },
    { ID_NAME(IID_IAliasRegistrationCallback),     RegisteredNever  },
    { ID_NAME(IID_IAssociationArrayInitialize),    RegisteredNever  },
    { ID_NAME(IID_IAssociationList),               RegisteredNever  },
    { ID_NAME(IID_IBasePropPage),                  RegisteredNever  },
    { ID_NAME(IID_IDrawPropertyControl),           RegisteredNever  },
    { ID_NAME(IID_IEnumAssociationElements),       RegisteredNever  },
    { ID_NAME(IID_IEnumerateAssociationElements),  RegisteredNever  },
    { ID_NAME(IID_IFolderNotify),                  RegisteredNever  },
    { ID_NAME(IID_IFolderProperties),              RegisteredNever  },
    { ID_NAME(IID_IFolderWithSearchRoot),          RegisteredNever  },
    { ID_NAME(IID_ILocalizableItemParent),         RegisteredNever  },
    { ID_NAME(IID_IPrinterFolder),                 RegisteredNever  },
    { ID_NAME(IID_IPropertyControl),               RegisteredNever  },
    { ID_NAME(IID_IPropertyControlBase),           RegisteredNever  },
    { ID_NAME(IID_IPropertyControlSite),           RegisteredNever  },
    { ID_NAME(IID_IRegItemCustomAttributes),       RegisteredNever  },
    { ID_NAME(IID_IRegItemCustomEnumerator),       RegisteredNever  },
    { ID_NAME(IID_IScope),                         RegisteredNever  },
    { ID_NAME(IID_IScopeItem),                     RegisteredNever  },
    { ID_NAME(IID_ITaskCondition),                 RegisteredNever  },
    { ID_NAME(IID_ITaskConditionCombiner),         RegisteredNever  },
    { ID_NAME(IID_ITaskConditionInit),             RegisteredNever  },
    { ID_NAME(IID_ITransferProvider),              RegisteredNever  },
    { ID_NAME(IID_IAssociationArrayOld),           RegisteredNever  },
    { ID_NAME(IID_IPinnedListOld),                 RegisteredNever  },
    { ID_NAME(IID_IPinnedList),                    RegisteredNever  },
    { ID_NAME(IID_IAttachmentExecute),             RegisteredNever  },
    { ID_NAME(IID_IComponentData),                 RegisteredNever  },
    { ID_NAME(IID_IConsole),                       RegisteredNever  },
    { ID_NAME(IID_IConsole2),                      RegisteredNever  },
    { ID_NAME(IID_IConsoleNameSpace),              RegisteredNever  },
    { ID_NAME(IID_IConsoleNameSpace2),             RegisteredNever  },
    { ID_NAME(IID_IPropertySheetCallback),         RegisteredNever  },
    { ID_NAME(IID_IPropertySheetProvider),         RegisteredNever  },
    { ID_NAME(IID_IExtendPropertySheet),           RegisteredNever  },
    { ID_NAME(IID_IExtendPropertySheet2),          RegisteredNever  },
    { ID_NAME(IID_IHeaderCtrl),                    RegisteredNever  },
    { ID_NAME(IID_IToolbar),                       RegisteredNever  },
    { ID_NAME(IID_IImageList_mmc),                 RegisteredNever  },
    { ID_NAME(IID_IConsoleVerb),                   RegisteredNever  },
    { ID_NAME(IID_ISnapInAbout),                   RegisteredNever  },
    { ID_NAME(IID_ICertificateManager),            RegisteredNever  },
    { ID_NAME(IID_IEnumNetCfgBindingInterface),    RegisteredNever  },
    { ID_NAME(IID_IEnumNetCfgBindingPath),         RegisteredNever  },
    { ID_NAME(IID_INetCfgBindingInterface),        RegisteredNever  },
    { ID_NAME(IID_INetCfgBindingPath),             RegisteredNever  },
};
static const INT KnownInterfaceCount = RTL_NUMBER_OF(KnownInterfaces);

#define ValidClassForVersion(pClass, version) \
    ((pClass)->MinClassNTDDIVersion <= (version) && (pClass)->MaxClassNTDDIVersion >= (version))
#define ValidInterfaceForVersion(interface, version) \
    ((interface).MinInterfaceNTDDIVersion <= (version) && (interface).MaxInterfaceNTDDIVersion >= (version))

static
PCKNOWN_INTERFACE
FindInterface(
    _In_ const IID *iid)
{
    INT i;

    for (i = 0; i < KnownInterfaceCount; i++)
        if (IsEqualIID(KnownInterfaces[i].iid, iid))
            return &KnownInterfaces[i];
    ASSERT(i != KnownInterfaceCount);
    return NULL;
}

static
BOOLEAN
IsInterfaceExpected(
    _In_ PCCLASS_AND_INTERFACES class,
    _In_ const IID *iid,
    _In_ ULONG NTDDIVersion)
{
    INT i;

    for (i = 0; class->ifaces[i].iid; i++)
    {
        if (ValidInterfaceForVersion(class->ifaces[i], NTDDIVersion) &&
            IsEqualIID(class->ifaces[i].iid, iid))
        {
            return TRUE;
        }
    }
    return FALSE;
}

#define INTF_NOT_EXPOSED LONG_MAX
static
LONG
GetInterfaceOffset(
    _In_ PUNKNOWN pUnk,
    _In_ const IID *iid)
{
    HRESULT hr;
    PVOID pObj;
    PUNKNOWN pUnk2;
    LONG offset;

    hr = IUnknown_QueryInterface(pUnk, iid, &pObj);
    ok(hr == S_OK || hr == E_NOINTERFACE, "IUnknown::QueryInterface returned 0x%lx\n", hr);
    if (FAILED(hr))
        return INTF_NOT_EXPOSED;

    pUnk2 = pObj;
    offset = (LONG_PTR)pObj - (LONG_PTR)pUnk;
    IUnknown_Release(pUnk2);
    return offset;
}

static
VOID
TestModuleInterfaces(
    _In_ PCCLASS_AND_INTERFACES ExpectedInterfaces,
    _In_ INT ExpectedInterfaceCount,
    _In_ ULONG NTDDIVersion)
{
    HRESULT hr;
    PVOID pObj;
    PUNKNOWN pUnk;
    INT iClass, iIntf;
    PCCLASS_AND_INTERFACES class;

    for (iClass = 0; iClass < ExpectedInterfaceCount; iClass++)
    {
        class = &ExpectedInterfaces[iClass];
        if (!ValidClassForVersion(class, NTDDIVersion))
            continue;
        hr = CoCreateInstance(class->clsid,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              &IID_IUnknown,
                              &pObj);
        ok(hr == S_OK, "CoCreateInstance failed. hr=0x%lx\n", hr);
        if (FAILED(hr))
        {
            skip("Failed to instantiate %s.\n", class->name);
            continue;
        }

        pUnk = pObj;

        /* Check that all expected interfaces are present */
        for (iIntf = 0; class->ifaces[iIntf].iid; iIntf++)
        {
            if (!ValidInterfaceForVersion(class->ifaces[iIntf], NTDDIVersion))
                continue;
            PCKNOWN_INTERFACE iface = FindInterface(class->ifaces[iIntf].iid);
            LONG offset = GetInterfaceOffset(pUnk, iface->iid);
            if (offset == INTF_NOT_EXPOSED)
                ok(0, "%s is missing %s\n", class->name, iface->name);
#ifdef LOG_COM_INTERFACE_OFFSETS
            else
                mytrace("%s0x%lx, %s, %s\n", offset < 0 ? "-" : "", offset < 0 ? -offset : offset, class->name, iface->name);
#endif
        }

        /* Check that none other than the expected interfaces are present */
        for (iIntf = 0; iIntf < KnownInterfaceCount; iIntf++)
        {
            PCKNOWN_INTERFACE iface = &KnownInterfaces[iIntf];
            LONG offset;
            if (IsInterfaceExpected(class, iface->iid, NTDDIVersion))
                continue;
            offset = GetInterfaceOffset(pUnk, iface->iid);
            ok(offset == INTF_NOT_EXPOSED, "%s exposes %s (offset %s0x%lx), but shouldn't\n", class->name, iface->name, offset < 0 ? "-" : "", offset < 0 ? -offset : offset);
        }

        // TODO: do some aggregation

        IUnknown_Release(pUnk);
    }
}

static
VOID
TestModuleRegistry(
    _In_ PCWSTR ModuleName,
    _In_ PCCLASS_AND_INTERFACES ExpectedInterfaces,
    _In_ INT ExpectedInterfaceCount,
    _In_ ULONG NTDDIVersion)
{
    INT iClass;
    PCCLASS_AND_INTERFACES class;
    HKEY hKeyClasses;
    LONG result;

    result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Classes\\CLSID", 0, KEY_ENUMERATE_SUB_KEYS, &hKeyClasses);
    ok(result == NO_ERROR, "Failed to open classes key, error %lu\n", result);
    if (!myskip(result == NO_ERROR, "No classes key\n"))
    {
        for (iClass = 0; iClass < ExpectedInterfaceCount; iClass++)
        {
            HKEY hKey;
            HKEY hKeyServer;
            NTSTATUS status;
            UNICODE_STRING clsid;
            DWORD type;
            WCHAR data[100];
            DWORD dataSize;
            PCWSTR expectedThreadingModel;

            class = &ExpectedInterfaces[iClass];
            if (!ValidClassForVersion(class, NTDDIVersion))
                continue;
            status = RtlStringFromGUID(class->clsid, &clsid);
            ok(status == STATUS_SUCCESS, "Failed to convert guid to string for %s, status %lx\n", class->name, status);
            if (myskip(NT_SUCCESS(status), "No guid string\n"))
                continue;

            result = RegOpenKeyExW(hKeyClasses, clsid.Buffer, 0, KEY_ENUMERATE_SUB_KEYS, &hKey);
            ok(result == NO_ERROR, "Failed to open key for %s, error %lu\n", class->name, result);
            RtlFreeUnicodeString(&clsid);
            if (myskip(result == NO_ERROR, "No key\n"))
                continue;

            result = RegOpenKeyExW(hKey, L"InProcServer32", 0, KEY_QUERY_VALUE, &hKeyServer);
            ok(result == NO_ERROR, "Failed to open key for %s, error %lu\n", class->name, result);
            RegCloseKey(hKey);
            if (myskip(result == NO_ERROR, "No key\n"))
                continue;

            dataSize = sizeof(data);
            result = RegQueryValueExW(hKeyServer, NULL, NULL, &type, (PBYTE)data, &dataSize);
            ok(result == NO_ERROR, "Failed to query value for %s, error %lu\n", class->name, result);
            if (!myskip(result == NO_ERROR, "No module name\n"))
            {
                ok(type == REG_SZ || type == REG_EXPAND_SZ, "type %lu for %s\n", type, class->name);
                ok(dataSize % sizeof(WCHAR) == 0, "size %lu for %s\n", dataSize, class->name);
                ok(dataSize <= sizeof(data), "size %lu for %s\n", dataSize, class->name);
                ok(data[dataSize / sizeof(WCHAR) - 1] == UNICODE_NULL, "Not null terminated for %s\n", class->name);
                // TODO: Use SearchPath (or assume everything's in system32) and do a proper full path compare
                PathStripPathW(data);
                PathRemoveExtensionW(data);
                ok(!_wcsicmp(data, ModuleName), "Server is %ls, expected %ls for %s\n", data, ModuleName, class->name);
            }

            dataSize = sizeof(data);
            result = RegQueryValueExW(hKeyServer, L"ThreadingModel", NULL, &type, (PBYTE)data, &dataSize);
            ok(result == NO_ERROR, "Failed to query value for %s, error %lu\n", class->name, result);
            if (!myskip(result == NO_ERROR, "No ThreadingModel\n"))
            {
                ok(type == REG_SZ || type == REG_EXPAND_SZ, "type %lu for %s\n", type, class->name);
                ok(dataSize % sizeof(WCHAR) == 0, "size %lu for %s\n", dataSize, class->name);
                ok(dataSize <= sizeof(data), "size %lu for %s\n", dataSize, class->name);
                ok(data[dataSize / sizeof(WCHAR) - 1] == UNICODE_NULL, "Not null terminated for %s\n", class->name);
                expectedThreadingModel = class->ThreadingModel;
                if (!expectedThreadingModel)
                    expectedThreadingModel = L"Apartment";
                ok(!_wcsicmp(data, expectedThreadingModel), "Server is %ls, expected %ls for %s\n", data, expectedThreadingModel, class->name);
            }

            RegCloseKey(hKeyServer);
        }
        RegCloseKey(hKeyClasses);
    }
}

static
VOID
TestManualInstantiation(
    _In_ PCWSTR ModuleName,
    _In_ PCCLASS_AND_INTERFACES ExpectedInterfaces,
    _In_ INT ExpectedInterfaceCount,
    _In_ ULONG NTDDIVersion)
{
    INT iClass;
    PCCLASS_AND_INTERFACES class;
    HRESULT (__stdcall *DllGetClassObject)(REFCLSID, REFIID, PVOID *);

    DllGetClassObject = (PVOID)GetProcAddress(GetModuleHandleW(ModuleName), "DllGetClassObject");
    ok(DllGetClassObject != NULL, "DllGetClassObject not found in %ls, error %lu\n", ModuleName, GetLastError());
    if (myskip(DllGetClassObject != NULL, "No DllGetClassObject\n"))
        return;

    for (iClass = 0; iClass < ExpectedInterfaceCount; iClass++)
    {
        PVOID pv;
        HRESULT hr;
        class = &ExpectedInterfaces[iClass];
        if (!ValidClassForVersion(class, NTDDIVersion))
            continue;
        hr = DllGetClassObject(class->clsid, &IID_IClassFactory, &pv);
        ok(hr == S_OK, "DllGetClassObject failed for %s, hr = 0x%lx\n", class->name, hr);
        if (!myskip(SUCCEEDED(hr), "No class factory\n"))
        {
            IClassFactory *pCF = pv;
            hr = IClassFactory_CreateInstance(pCF, NULL, &IID_IUnknown, &pv);
            ok(hr == S_OK, "IClassFactory::CreateInstance failed for %s, hr = 0x%lx\n", class->name, hr);
            if (!myskip(SUCCEEDED(hr), "No instance\n"))
            {
                IUnknown *pUnk = pv;
                IUnknown_Release(pUnk);
            }
            IClassFactory_Release(pCF);
        }
    }
}

VOID
TestClassesEx(
    _In_ PCWSTR ModuleName,
    _In_ PCCLASS_AND_INTERFACES ExpectedInterfaces,
    _In_ INT ExpectedInterfaceCount,
    _In_ ULONG MinimumNTDDIVersion,
    _In_ ULONG MaximumNTDDIVersion,
    _In_ BOOLEAN IsWinRT)
{
    HRESULT hr;
    ULONG NTDDIVersion;

    NTDDIVersion = GetNTDDIVersion();

    if (NTDDIVersion < MinimumNTDDIVersion || NTDDIVersion > MaximumNTDDIVersion)
    {
        skip("Skipping all tests for module %S, NTDDI version (0x%08lx) is outside of the supported range (0x%08lx-0x%08lx).\n",
             ModuleName, NTDDIVersion, MinimumNTDDIVersion, MaximumNTDDIVersion);
        return;
    }

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ok(hr == S_OK, "CoInitializeEx failed. hr=0x%lx\n", hr);
    if (myskip(SUCCEEDED(hr), "Failed to initialize COM. Cannot perform tests\n"))
        return;

    TestModuleInterfaces(ExpectedInterfaces, ExpectedInterfaceCount, NTDDIVersion);
    TestModuleRegistry(ModuleName, ExpectedInterfaces, ExpectedInterfaceCount, NTDDIVersion);
    if (IsWinRT)
        skip("%S is a WinRT module, skipping manual instantiation tests.\n", ModuleName);
    else
        TestManualInstantiation(ModuleName, ExpectedInterfaces, ExpectedInterfaceCount, NTDDIVersion);

    CoUninitialize();
}

VOID
TestClasses(
    _In_ PCWSTR ModuleName,
    _In_ PCCLASS_AND_INTERFACES ExpectedInterfaces,
    _In_ INT ExpectedInterfaceCount)
{
    TestClassesEx(ModuleName, ExpectedInterfaces, ExpectedInterfaceCount, NTDDI_MIN, NTDDI_MAX, FALSE);
}

static
VOID
TestInterfaceRegistry(
    _In_ PCKNOWN_INTERFACE Interfaces,
    _In_ INT InterfaceCount)
{
    INT i;
    HKEY hKeyInterface;
    LONG result;
    ULONG CurrentNTDDI;

    result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Classes\\Interface", 0, KEY_ENUMERATE_SUB_KEYS, &hKeyInterface);
    ok(result == NO_ERROR, "Failed to open interface key, error %lu\n", result);
    if (!myskip(result == NO_ERROR, "No interface key\n"))
    {
        CurrentNTDDI = GetNTDDIVersion();

        for (i = 0; i < InterfaceCount; i++)
        {
            HKEY hKey;
            NTSTATUS status;
            UNICODE_STRING iid;
            DWORD type;
            WCHAR data[100];
            DWORD dataSize;
            PCKNOWN_INTERFACE iface;
            PCWSTR expectedName;

            iface = &Interfaces[i];
            status = RtlStringFromGUID(iface->iid, &iid);
            ok(status == STATUS_SUCCESS, "Failed to convert guid to string for %s, status %lx\n", iface->name, status);
            if (myskip(NT_SUCCESS(status), "No guid string\n"))
                continue;

            result = RegOpenKeyExW(hKeyInterface, iid.Buffer, 0, KEY_QUERY_VALUE, &hKey);
            if (iface->IsRegistered(CurrentNTDDI))
            {
                ok(result == NO_ERROR, "%s should be registered. (Error %lu)\n", iface->name, result);
                // (void)myskip(result == NO_ERROR, "No key\n");
            }
            else
            {
                ok(result == ERROR_FILE_NOT_FOUND, "%s should not be registered. (Error %lu)\n", iface->name, result);
            }
            RtlFreeUnicodeString(&iid);
            if (result != NO_ERROR)
                continue;

            dataSize = sizeof(data);
            result = RegQueryValueExW(hKey, NULL, NULL, &type, (PBYTE)data, &dataSize);
            ok(result == NO_ERROR, "Failed to query value for %s, error %lu\n", iface->name, result);
            if (!myskip(result == NO_ERROR, "No module name\n"))
            {
                ok(type == REG_SZ, "type %lu for %s\n", type, iface->name);
                ok(dataSize % sizeof(WCHAR) == 0, "size %lu for %s\n", dataSize, iface->name);
                ok(dataSize <= sizeof(data), "size %lu for %s\n", dataSize, iface->name);
                ok(data[dataSize / sizeof(WCHAR) - 1] == UNICODE_NULL, "Not null terminated for %s\n", iface->name);
                expectedName = wcschr(iface->wname, L'_');
                if (expectedName)
                    expectedName++;
                else
                    expectedName = iface->wname;
                ok(!_wcsicmp(data, expectedName), "Name is %ls, expected %ls\n", data, expectedName);
            }

            RegCloseKey(hKey);
        }
        RegCloseKey(hKeyInterface);
    }
}

START_TEST(interfaces)
{
    TestInterfaceRegistry(KnownInterfaces, KnownInterfaceCount);
}
