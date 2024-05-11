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
    BOOLEAN noreg;
} KNOWN_INTERFACE;
typedef const KNOWN_INTERFACE *PCKNOWN_INTERFACE;

#undef ID_NAME
#define ID_NAME(c) &c, #c, L ## #c
static KNOWN_INTERFACE KnownInterfaces[] =
{
    { ID_NAME(IID_IACList),                     TRUE },
    { ID_NAME(IID_IACList2),                    TRUE },
    { ID_NAME(IID_IADesktopP2),                 TRUE },
    { ID_NAME(IID_IAccIdentity)                      },
    { ID_NAME(IID_IAccPropServer)                    },
    { ID_NAME(IID_IAccPropServices)                  },
    { ID_NAME(IID_IAccessible)                       },
    { ID_NAME(IID_IAccessibleHandler)                },
    { ID_NAME(IID_IAccessControl),              TRUE },
    { ID_NAME(IID_IAccessor)                         },
    { ID_NAME(IID_IACLCustomMRU),               TRUE },
    { ID_NAME(IID_IActiveDesktop),              TRUE },
    { ID_NAME(IID_IActiveDesktopP),             TRUE },
    { ID_NAME(IID_IActionProgress)                   },
    { ID_NAME(IID_IActionProgressDialog)             },
    { ID_NAME(IID_IAddressBarParser),           TRUE },
    { ID_NAME(IID_IAddressBand),                TRUE },
    { ID_NAME(IID_IAddressEditBox),             TRUE },
    { ID_NAME(IID_IAsyncMoniker),               TRUE },
    { ID_NAME(IID_IAugmentedShellFolder),       TRUE },
    { ID_NAME(IID_IAugmentedShellFolder2),      TRUE },
    { ID_NAME(IID_IAutoComplete),               TRUE },
    { ID_NAME(IID_IAutoComplete2),              TRUE },
    { ID_NAME(IID_IAutoCompleteDropDown)             },
    { ID_NAME(IID_IBandHost)                         },
    { ID_NAME(IID_IBandNavigate),                    },
    { ID_NAME(IID_IBandProxy),                  TRUE },
    { ID_NAME(IID_IBandSite)                         },
    { ID_NAME(IID_IBandSiteHelper),             TRUE },
    { ID_NAME(IID_IBanneredBar),                TRUE },
    { ID_NAME(IID_IBindCtx)                          },
    { ID_NAME(IID_IBindEventHandler)                 },
    { ID_NAME(IID_IBindHost)                         },
    { ID_NAME(IID_IBinding)                          },
    { ID_NAME(IID_IBindProtocol),               TRUE },
    { ID_NAME(IID_IBindResource)                     },
    { ID_NAME(IID_IBindStatusCallback)               },
    { ID_NAME(IID_IBlockingLock)                     },
    { ID_NAME(IID_IBrowserFrameOptions),        TRUE },
    { ID_NAME(IID_IBrowserService)                   },
    { ID_NAME(IID_IBrowserService2),            TRUE },
    { ID_NAME(IID_IBrowserService3),            TRUE },
    { ID_NAME(IID_IBrowserService4),            TRUE },
    { ID_NAME(IID_ICDBurn)                           },
    { ID_NAME(IID_ICDBurnExt)                        },
    { ID_NAME(IID_ICDBurnPriv)                       },
    { ID_NAME(IID_ICallFactory),                TRUE },
    { ID_NAME(IID_ICancelMethodCalls),          TRUE },
    { ID_NAME(IID_ICatInformation)                   },
    { ID_NAME(IID_ICatRegister)                      },
    { ID_NAME(IID_IClassActivator),                  },
    { ID_NAME(IID_IClassFactory)                     },
    { ID_NAME(IID_IClassFactory2)                    },
    { ID_NAME(IID_IClassFactory3),              TRUE },
    { ID_NAME(IID_IClientSecurity),             TRUE },
    { ID_NAME(IID_ICommDlgBrowser)                   },
    { ID_NAME(IID_ICommDlgBrowser2)                  },
    { ID_NAME(IID_ICommDlgBrowser3)                  },
    { ID_NAME(IID_ICompositeFolder)                  },
    { ID_NAME(IID_IComputerInfoChangeNotify),        },
    { ID_NAME(IID_IComThreadingInfo),           TRUE },
    { ID_NAME(IID_IConnectionPoint)                  },
    { ID_NAME(IID_IConnectionPointContainer)         },
    { ID_NAME(IID_IContext),                    TRUE },
    { ID_NAME(IID_IContextMenu),                TRUE },
    { ID_NAME(IID_IContextMenu2),               TRUE },
    { ID_NAME(IID_IContextMenu3),               TRUE },
    { ID_NAME(IID_IContextMenuCB),              TRUE },
    { ID_NAME(IID_IContextMenuSite)                  },
    { ID_NAME(IID_IContinue)                         },
    { ID_NAME(IID_IContinueCallback)                 },
    { ID_NAME(IID_ICopyHookA),                  TRUE },
    { ID_NAME(IID_ICopyHookW),                  TRUE },
    { ID_NAME(IID_ICurrentWorkingDirectory),    TRUE },
    { ID_NAME(IID_ICustomizeInfoTip)                 },
    { ID_NAME(IID_IDVGetEnum),                  TRUE },
    { ID_NAME(IID_IDataObject)                       },
    //{ ID_NAME(IID_IDefViewID)                        }, == DefViewFrame3
    { ID_NAME(IID_IDefViewFrame),               TRUE },
    { ID_NAME(IID_IDefViewFrame3)                    },
    { ID_NAME(IID_IDefViewFrameGroup)                },
    { ID_NAME(IID_IDefViewSafety),                   },
    { ID_NAME(IID_IDefViewScript),              TRUE },
    { ID_NAME(IID_IDelayedRelease),             TRUE },
    { ID_NAME(IID_IDeskBand)                         },
    { ID_NAME(IID_IDeskBandEx)                       },
    { ID_NAME(IID_IDeskBar),                    TRUE },
    { ID_NAME(IID_IDeskBarClient),              TRUE },
    { ID_NAME(IID_IDeskMovr),                   TRUE },
    { ID_NAME(IID_IDiscMasterProgressEvents)         },
    { ID_NAME(IID_IDispatch)                         },
    { ID_NAME(IID_IDispatchEx)                       },
    { ID_NAME(IID_IDockingWindow)                    },
    { ID_NAME(IID_IDockingWindowFrame),         TRUE },
    { ID_NAME(IID_IDockingWindowSite),          TRUE },
    { ID_NAME(IID_IDocViewSite),                TRUE },
    { ID_NAME(IID_IDragSourceHelper),           TRUE },
    { ID_NAME(IID_IDriveFolderExt),             TRUE },
    { ID_NAME(IID_IDropSource)                       },
    { ID_NAME(IID_IDropTarget)                       },
    { ID_NAME(IID_IDropTargetHelper),           TRUE },
    { ID_NAME(IID_IEFrameAuto)                       },
    //{ ID_NAME(IID_IEnumCATID)                        }, == EnumGUID
    //{ ID_NAME(IID_IEnumCLSID)                        }, == EnumGUID
    { ID_NAME(IID_IEnumCATEGORYINFO)                 },
    { ID_NAME(IID_IEnumConnectionPoints)             },
    { ID_NAME(IID_IEnumConnections)                  },
    { ID_NAME(IID_IEnumExtraSearch)                  },
    { ID_NAME(IID_IEnumGUID)                         },
    { ID_NAME(IID_IEnumIDList)                       },
    { ID_NAME(IID_IEnumMoniker)                      },
    //{ ID_NAME(IID_IEnumNetCfgBindingInterface)       },
    //{ ID_NAME(IID_IEnumNetCfgBindingPath)            },
    { ID_NAME(IID_IEnumNetCfgComponent),        TRUE },
    { ID_NAME(IID_IEnumNetConnection)                },
    { ID_NAME(IID_IEnumShellItems)                   },
    { ID_NAME(IID_IEnumSTATSTG)                       },
    { ID_NAME(IID_IEnumString)                       },
    { ID_NAME(IID_IEnumUnknown)                      },
    { ID_NAME(IID_IEnumVARIANT)                      },
    { ID_NAME(IID_IErrorLog)                         },
    { ID_NAME(IID_IExplorerBrowser)                  },
    { ID_NAME(IID_IExplorerToolbar),            TRUE },
    { ID_NAME(IID_IExtractIconA),               TRUE },
    { ID_NAME(IID_IExtractIconW),               TRUE },
    { ID_NAME(IID_IExtractImage)                     },
    { ID_NAME(IID_IExtractImage2)                    },
    { ID_NAME(IID_IFileDialog)                       },
    { ID_NAME(IID_IFileDialog2),                TRUE },
    { ID_NAME(IID_IFileOpenDialog)                   },
    { ID_NAME(IID_IFileSaveDialog)                   },
    { ID_NAME(IID_IFileSearchBand)                   },
    { ID_NAME(IID_IFileViewerA),                TRUE },
    { ID_NAME(IID_IFileViewerSite),             TRUE },
    { ID_NAME(IID_IFileViewerW),                TRUE },
    { ID_NAME(IID_IFilter)                           },
    { ID_NAME(IID_IFolderBandPriv)                   },
    { ID_NAME(IID_IFolderFilter)                     },
    { ID_NAME(IID_IFolderFilterSite)                 },
    { ID_NAME(IID_IFolderView)                       },
    { ID_NAME(IID_IFolderView2)                      },
    { ID_NAME(IID_IFolderViewHost),             TRUE },
    { ID_NAME(IID_IFolderViewOC)                     },
    { ID_NAME(IID_IFolderViewSettings)               },
    { ID_NAME(IID_IForegroundTransfer),         TRUE },
    { ID_NAME(IID_IGetNameSpaceExtensionPointer),TRUE},
    { ID_NAME(IID_IGlobalFolderSettings),       TRUE },
    { ID_NAME(IID_IHWEventHandler)                   },
    { ID_NAME(IID_IHWEventHandler2)                  },
    { ID_NAME(IID_IHlinkFrame)                       },
    { ID_NAME(IID_IImageList),                  TRUE },
    { ID_NAME(IID_IImageList2),                 TRUE },
    { ID_NAME(IID_IInitializeObject),           TRUE },
    { ID_NAME(IID_IInitializeWithBindCtx)            },
    { ID_NAME(IID_IInitializeWithFile)               },
    { ID_NAME(IID_IInputObject)                      },
    { ID_NAME(IID_IInputObjectSite)                  },
    { ID_NAME(IID_IInternalUnknown),            TRUE },
    { ID_NAME(IID_IInternetSecurityManager)          },
    { ID_NAME(IID_IInternetZoneManager),        TRUE },
    { ID_NAME(IID_IItemNameLimits)                   },
    { ID_NAME(IID_IMarshal)                          },
    { ID_NAME(IID_IMarshal2),                   TRUE },
    { ID_NAME(IID_IMenuBand),                   TRUE },
    { ID_NAME(IID_IMenuPopup),                  TRUE },
    { ID_NAME(IID_IModalWindow)                      },
    { ID_NAME(IID_IMoniker)                          },
    { ID_NAME(IID_IMruDataList),                TRUE },
    { ID_NAME(IID_IMruPidlList),                TRUE },
    { ID_NAME(IID_IMultiMonitorDockingSite),    TRUE },
    { ID_NAME(IID_IMultiQI),                    TRUE },
    { ID_NAME(IID_INameSpaceTreeControl),       TRUE },
    { ID_NAME(IID_INamespaceProxy),             TRUE },
    { ID_NAME(IID_INamespaceWalk)                    },
    { ID_NAME(IID_INamespaceWalkCB)                  },
    { ID_NAME(IID_INamespaceWalkCB2)                 },
    { ID_NAME(IID_INetCfg),                     TRUE },
    //{ ID_NAME(IID_INetCfgBindingInterface)           },
    //{ ID_NAME(IID_INetCfgBindingPath)                },
    { ID_NAME(IID_INetCfgComponent),            TRUE },
    { ID_NAME(IID_INetCfgComponentBindings),    TRUE },
    { ID_NAME(IID_INetCfgComponentControl),     TRUE },
    { ID_NAME(IID_INetCfgComponentPropertyUi),  TRUE },
    { ID_NAME(IID_INetCfgLock),                 TRUE },
    { ID_NAME(IID_INetCfgPnpReconfigCallback),  TRUE },
    { ID_NAME(IID_INetConnectionConnectUi),     TRUE },
    { ID_NAME(IID_INetConnectionPropertyUi),    TRUE },
    { ID_NAME(IID_INetConnectionPropertyUi2),   TRUE },
    { ID_NAME(IID_INetConnectionManager)             },
    { ID_NAME(IID_INetLanConnectionUiInfo),     TRUE },
    { ID_NAME(IID_INewMenuClient)                    },
    { ID_NAME(IID_INewShortcutHookA),           TRUE },
    { ID_NAME(IID_INewShortcutHookW),           TRUE },
    { ID_NAME(IID_INewWindowManager)                 },
    { ID_NAME(IID_INSCTree),                    TRUE },
    { ID_NAME(IID_INSCTree2),                   TRUE },
    { ID_NAME(IID_IObjMgr),                     TRUE },
    { ID_NAME(IID_IObjectSafety)                     },
    { ID_NAME(IID_IObjectWithBackReferences)         },
    { ID_NAME(IID_IObjectWithSite)                   },
    { ID_NAME(IID_IOleClientSite)                    },
    { ID_NAME(IID_IOleCommandTarget)                 },
    { ID_NAME(IID_IOleContainer)                     },
    { ID_NAME(IID_IOleControl)                       },
    { ID_NAME(IID_IOleControlSite)                   },
    { ID_NAME(IID_IOleInPlaceActiveObject)           },
    { ID_NAME(IID_IOleInPlaceFrame)                  },
    { ID_NAME(IID_IOleInPlaceObject)                 },
    { ID_NAME(IID_IOleInPlaceObjectWindowless), TRUE },
    { ID_NAME(IID_IOleInPlaceSite)                   },
    { ID_NAME(IID_IOleInPlaceSiteEx)                 },
    { ID_NAME(IID_IOleInPlaceSiteWindowless),   TRUE },
    { ID_NAME(IID_IOleInPlaceUIWindow)               },
    { ID_NAME(IID_IOleItemContainer),                },
    { ID_NAME(IID_IOleLink),                         },
    { ID_NAME(IID_IOleObject)                        },
    { ID_NAME(IID_IOleWindow)                        },
    { ID_NAME(IID_IParentAndItem)                    },
    { ID_NAME(IID_IParseDisplayName),                },
    { ID_NAME(IID_IPersist)                          },
    { ID_NAME(IID_IPersistFile)                      },
    { ID_NAME(IID_IPersistFolder)                    },
    { ID_NAME(IID_IPersistFolder2)                   },
    { ID_NAME(IID_IPersistFolder3)                   },
    { ID_NAME(IID_IPersistFreeThreadedObject),  TRUE },
    { ID_NAME(IID_IPersistHistory)                   },
    { ID_NAME(IID_IPersistIDList)                    },
    { ID_NAME(IID_IPersistMemory)                    },
    { ID_NAME(IID_IPersistPropertyBag)               },
    { ID_NAME(IID_IPersistPropertyBag2)              },
    { ID_NAME(IID_IPersistStorage)                   },
    { ID_NAME(IID_IPersistStream)                    },
    { ID_NAME(IID_IPersistStreamInit)                },
    { ID_NAME(IID_IPreviewHandler)                   },
    { ID_NAME(IID_IPreviewHandlerFrame)              },
    { ID_NAME(IID_IPreviewHandlerVisuals)            },
    { ID_NAME(IID_IProgressDialog),             TRUE },
    { ID_NAME(IID_IPropertyBag)                      },
    { ID_NAME(IID_IPropertyBag2)                     },
    { ID_NAME(IID_IPropertySetStorage)               },
    { ID_NAME(IID_IPropertyStore)                    },
    { ID_NAME(IID_IPropSheetPage),              TRUE },
    { ID_NAME(IID_IProvideClassInfo)                 },
    { ID_NAME(IID_IProvideClassInfo2)                },
    { ID_NAME(IID_IQueryAssociations),          TRUE },
    { ID_NAME(IID_IQueryCancelAutoPlay)              },
    { ID_NAME(IID_IQueryInfo),                  TRUE },
    { ID_NAME(IID_IQuickActivate)                    },
    { ID_NAME(IID_IRegTreeOptions),             TRUE },
    { ID_NAME(IID_IRemoteComputer)                   },
    { ID_NAME(IID_IResolveShellLink)                 },
    { ID_NAME(IID_IROTData),                         },
    { ID_NAME(IID_IRpcOptions),                 TRUE },
    { ID_NAME(IID_IRunnableObject)                   },
    { ID_NAME(IID_IRunningObjectTable),              },
    { ID_NAME(IID_ISLTracker),                  TRUE },
    { ID_NAME(IID_IScriptErrorList)                  },
    { ID_NAME(IID_ISearch)                           },
    { ID_NAME(IID_ISearchAssistantOC)                },
    { ID_NAME(IID_ISearchAssistantOC2)               },
    { ID_NAME(IID_ISearchAssistantOC3)               },
    { ID_NAME(IID_ISearchBar)                        },
    { ID_NAME(IID_ISearches)                         },
    { ID_NAME(IID_ISecMgrCacheSeedTarget)            },
    { ID_NAME(IID_IServerSecurity),             TRUE },
    { ID_NAME(IID_IServiceProvider)                  },
    { ID_NAME(IID_IShellApp),                   TRUE },
    { ID_NAME(IID_IShellBrowser)                     },
    { ID_NAME(IID_IShellBrowserService),        TRUE },
    { ID_NAME(IID_IShellChangeNotify),          TRUE },
    { ID_NAME(IID_IShellCopyHookA),             TRUE },
    { ID_NAME(IID_IShellCopyHookW),             TRUE },
    { ID_NAME(IID_IShellDesktopTray),           TRUE },
    { ID_NAME(IID_IShellDetails),               TRUE },
    { ID_NAME(IID_IShellDispatch)                    },
    { ID_NAME(IID_IShellDispatch2)                   },
    { ID_NAME(IID_IShellDispatch3)                   },
    { ID_NAME(IID_IShellDispatch4)                   },
    { ID_NAME(IID_IShellDispatch5),             TRUE },
    { ID_NAME(IID_IShellDispatch6),             TRUE },
    { ID_NAME(IID_IShellExecuteHookA),          TRUE },
    { ID_NAME(IID_IShellExecuteHookW),          TRUE },
    { ID_NAME(IID_IShellExtInit),               TRUE },
    { ID_NAME(IID_IShellFavoritesNameSpace)          },
    { ID_NAME(IID_IShellFolder)                      },
    { ID_NAME(IID_IShellFolder2)                     },
    { ID_NAME(IID_IShellFolderBand),            TRUE },
    { ID_NAME(IID_IShellFolderSearchable),      TRUE },
    { ID_NAME(IID_IShellFolderSearchableCallback), TRUE },
    { ID_NAME(IID_IShellFolderView),            TRUE },
    { ID_NAME(IID_IShellFolderViewCB),          TRUE },
    { ID_NAME(IID_IShellFolderViewDual)              },
    { ID_NAME(IID_IShellFolderViewDual2)             },
    { ID_NAME(IID_IShellFolderViewDual3),       TRUE },
    { ID_NAME(IID_IShellFolderViewType),        TRUE },
    { ID_NAME(IID_IShellIcon)                        },
    { ID_NAME(IID_IShellIconOverlay),           TRUE },
    { ID_NAME(IID_IShellIconOverlayIdentifier), TRUE },
    { ID_NAME(IID_IShellImageData),             TRUE },
    { ID_NAME(IID_IShellImageDataAbort),        TRUE },
    { ID_NAME(IID_IShellImageDataFactory),      TRUE },
    { ID_NAME(IID_IShellItem)                        },
    { ID_NAME(IID_IShellItem2)                       },
    { ID_NAME(IID_IShellItemArray)                   },
    { ID_NAME(IID_IShellItemFilter)                  },
    { ID_NAME(IID_IShellLinkA)                       },
    { ID_NAME(IID_IShellLinkDataList),          TRUE },
    { ID_NAME(IID_IShellLinkDual)                    },
    { ID_NAME(IID_IShellLinkDual2)                   },
    { ID_NAME(IID_IShellLinkW)                       },
    { ID_NAME(IID_IShellMenu),                  TRUE },
    { ID_NAME(IID_IShellMenu2),                 TRUE },
    { ID_NAME(IID_IShellMenuAcc),               TRUE },
    { ID_NAME(IID_IShellMenuCallback),          TRUE },
    { ID_NAME(IID_IShellNameSpace)                   },
    { ID_NAME(IID_IShellPropSheetExt),          TRUE },
    { ID_NAME(IID_IShellService),               TRUE },
    { ID_NAME(IID_IShellTaskScheduler),         TRUE },
    { ID_NAME(IID_IShellUIHelper)                    },
    { ID_NAME(IID_IShellUIHelper2),             TRUE },
    { ID_NAME(IID_IShellView)                        },
    { ID_NAME(IID_IShellView2)                       },
    { ID_NAME(IID_IShellView3)                       },
    { ID_NAME(IID_IShellWindows)                     },
    { ID_NAME(IID_ISpecifyPropertyPages)             },
    { ID_NAME(IID_IStorage)                          },
    { ID_NAME(IID_IStream)                           },
    { ID_NAME(IID_ISurrogate)                        },
    { ID_NAME(IID_ISynchronize)                      },
    { ID_NAME(IID_ISynchronizeContainer),       TRUE },
    { ID_NAME(IID_ISynchronizeEvent),           TRUE },
    { ID_NAME(IID_ISynchronizeHandle),          TRUE },
    { ID_NAME(IID_ITargetEmbedding)                  },
    { ID_NAME(IID_ITargetFrame)                      },
    { ID_NAME(IID_ITargetFrame2)                     },
    { ID_NAME(IID_ITargetFramePriv)                  },
    { ID_NAME(IID_ITargetFramePriv2)                 },
    { ID_NAME(IID_ITargetNotify)                     },
    { ID_NAME(IID_ITaskbarList)                      },
    { ID_NAME(IID_ITaskbarList2)                     },
    { ID_NAME(IID_ITaskbarList3),               TRUE },
    { ID_NAME(IID_ITaskbarList4),               TRUE },
    { ID_NAME(IID_ITrackShellMenu),             TRUE },
    /* This interface is completely different between PSDK and registry/shell32 */
    { ID_NAME(IID_ITransferAdviseSink),         TRUE },
#define IID_ITransferAdviseSink IID_ITransferAdviseSinkPriv
    { ID_NAME(IID_ITransferAdviseSink)               },
#undef IID_ITransferAdviseSink
    { ID_NAME(IID_ITransferDestination),        TRUE },
    { ID_NAME(IID_ITransferSource),             TRUE },
    { ID_NAME(IID_ITranslateShellChangeNotify), TRUE },
    { ID_NAME(IID_ITrayPriv),                   TRUE },
    { ID_NAME(IID_ITrayPriv2),                  TRUE },
    { ID_NAME(IID_IUnknown)                          },
    { ID_NAME(IID_IURLSearchHook),              TRUE },
    { ID_NAME(IID_IURLSearchHook2),             TRUE },
    { ID_NAME(IID_IUrlHistoryNotify)                 },
    { ID_NAME(IID_IUrlHistoryStg)                    },
    { ID_NAME(IID_IUrlHistoryStg2)                   },
    { ID_NAME(IID_IViewObject)                       },
    { ID_NAME(IID_IViewObject2)                      },
    { ID_NAME(IID_IViewObjectEx),               TRUE },
    { ID_NAME(IID_IVisualProperties)                 },
    { ID_NAME(IID_IWebBrowser)                       },
    { ID_NAME(IID_IWebBrowser2)                      },
    { ID_NAME(IID_IWebBrowserApp)                    },
    { ID_NAME(IID_IWebBrowserPriv)                   },
    { ID_NAME(IID_IWebBrowserPriv2)                  },
    { ID_NAME(IID_IWinEventHandler),            TRUE },

    { ID_NAME(IID_DFConstraint),                TRUE },
    { ID_NAME(DIID__SearchAssistantEvents)           },
    { ID_NAME(DIID_DShellFolderViewEvents)           },
    { ID_NAME(DIID_DShellNameSpaceEvents)            },
    { ID_NAME(DIID_DShellWindowsEvents)              },
    { ID_NAME(DIID_DWebBrowserEvents)                },
    { ID_NAME(DIID_DWebBrowserEvents2)               },
    { ID_NAME(DIID_XMLDOMDocumentEvents )            },

    { ID_NAME(IID_CDefView),                    TRUE },
    { ID_NAME(IID_Folder)                            },
    { ID_NAME(IID_Folder2)                           },
    { ID_NAME(IID_Folder3)                           },
    { ID_NAME(IID_FolderItem)                        },
    { ID_NAME(IID_FolderItem2)                       },
    { ID_NAME(IID_FolderItems)                       },
    { ID_NAME(IID_FolderItems2)                      },
    { ID_NAME(IID_FolderItems3)                      },
    { ID_NAME(IID_FolderItemVerb)                    },
    { ID_NAME(IID_FolderItemVerbs)                   },

    { ID_NAME(CLSID_ShellDesktop),              TRUE },

    { ID_NAME(IID_IQueryContinue)                    },
    { ID_NAME(IID_IUserNotification)                 },
    { ID_NAME(IID_IUserNotificationCallback)         }, // On Vista+
    { ID_NAME(IID_IUserNotification2)                }, // On Vista+

    { ID_NAME(IID_IAggregateFilterCondition)         },
    { ID_NAME(IID_IAliasRegistrationCallback),  TRUE },
    { ID_NAME(IID_IAssociationArrayInitialize), TRUE },
    { ID_NAME(IID_IAssociationList),            TRUE },
    { ID_NAME(IID_IBackReferencedObject),       TRUE },
    { ID_NAME(IID_IBasePropPage),               TRUE },
    { ID_NAME(IID_ICommonLayoutDefinition),     TRUE },
    { ID_NAME(IID_IControlPanelEnumerator),     TRUE },
    { ID_NAME(IID_IDelegateHostItemContainer),  TRUE },
    { ID_NAME(IID_IDrawPropertyControl),        TRUE },
    { ID_NAME(IID_IEnumAssociationElements),    TRUE },
    { ID_NAME(IID_IEnumerateAssociationElements),TRUE },
    { ID_NAME(IID_IExecuteCommand),             TRUE },
    { ID_NAME(IID_IFilterCondition)                  },
    { ID_NAME(IID_IFolderNotify),               TRUE },
    { ID_NAME(IID_IFolderProperties),           TRUE },
    { ID_NAME(IID_IFolderType),                 TRUE },
    { ID_NAME(IID_IFolderWithSearchRoot),       TRUE },
    { ID_NAME(IID_IFrameLayoutDefinition),      TRUE },
    { ID_NAME(IID_IItemFilter),                 TRUE },
    { ID_NAME(IID_IItemFilterOwner),            TRUE },
    { ID_NAME(IID_ILocalizableItemParent),      TRUE },
    { ID_NAME(IID_INewItemAdvisor),             TRUE },
    { ID_NAME(IID_IObjectWithAssociationElement),TRUE },
    { ID_NAME(IID_IObjectWithAssociationList),  TRUE },
    { ID_NAME(IID_IObjectWithQuerySource),      TRUE },
    { ID_NAME(IID_IObjectWithSelection),        TRUE },
    { ID_NAME(IID_IPersistString2),             TRUE },
    { ID_NAME(IID_IPrinterFolder),              TRUE },
    { ID_NAME(IID_IPropertyControl),            TRUE },
    { ID_NAME(IID_IPropertyControlBase),        TRUE },
    { ID_NAME(IID_IPropertyControlSite),        TRUE },
    { ID_NAME(IID_IRegItemCustomAttributes),    TRUE },
    { ID_NAME(IID_IRegItemCustomEnumerator),    TRUE },
    { ID_NAME(IID_IRegItemFolder),              TRUE },
    { ID_NAME(IID_IRootAndRelativeParsingFolder),TRUE },
    { ID_NAME(IID_IScope),                      TRUE },
    { ID_NAME(IID_IScopeItem),                  TRUE },
    { ID_NAME(IID_IShellBrowserServce),         TRUE },
    { ID_NAME(IID_IShellFolder3),               TRUE },
    { ID_NAME(IID_ITaskCondition),              TRUE },
    { ID_NAME(IID_ITaskConditionCombiner),      TRUE },
    { ID_NAME(IID_ITaskConditionInit),          TRUE },
    { ID_NAME(IID_ITransferProvider),           TRUE },
    { ID_NAME(IID_IUserEventTimer)                   },
    { ID_NAME(IID_IUserEventTimerCallback)           },
    { ID_NAME(IID_IAssociationArrayOld),        TRUE },
    { ID_NAME(IID_IAssociationArray),           TRUE },
#define IID_IDriveFolderExt IID_IDriveFolderExtOld
    { ID_NAME(IID_IDriveFolderExt)                   },
#undef IID_IDriveFolderExt
    { ID_NAME(IID_IPinnedListOld),              TRUE },
    { ID_NAME(IID_IPinnedList),                 TRUE },
    { ID_NAME(IID_IAttachmentExecute),          TRUE },

    // + MMC stuff
    { ID_NAME(IID_IComponentData),              TRUE },
    { ID_NAME(IID_IConsole),                    TRUE },
    { ID_NAME(IID_IConsole2),                   TRUE },
    { ID_NAME(IID_IConsoleNameSpace),           TRUE },
    { ID_NAME(IID_IConsoleNameSpace2),          TRUE },
    { ID_NAME(IID_IPropertySheetCallback),      TRUE },
    { ID_NAME(IID_IPropertySheetProvider),      TRUE },
    { ID_NAME(IID_IExtendPropertySheet),        TRUE },
    { ID_NAME(IID_IExtendPropertySheet2),       TRUE },
    { ID_NAME(IID_IHeaderCtrl),                 TRUE },
    { ID_NAME(IID_IToolbar),                    TRUE },
    { ID_NAME(IID_IImageList_mmc),              TRUE },
    { ID_NAME(IID_IConsoleVerb),                TRUE },
    { ID_NAME(IID_ISnapInAbout),                TRUE },
    // - MMC stuff

    { ID_NAME(IID_ICertificateManager),         TRUE },
};
static const INT KnownInterfaceCount = RTL_NUMBER_OF(KnownInterfaces);

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
    _In_ const IID *iid)
{
    INT i;

    for (i = 0; class->ifaces[i].iid; i++)
        if (IsEqualIID(class->ifaces[i].iid, iid))
            return TRUE;
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
    _In_ INT ExpectedInterfaceCount)
{
    HRESULT hr;
    PVOID pObj;
    PUNKNOWN pUnk;
    INT iClass, iIntf;
    PCCLASS_AND_INTERFACES class;

    for (iClass = 0; iClass < ExpectedInterfaceCount; iClass++)
    {
        class = &ExpectedInterfaces[iClass];
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

        /* Check that all expected interfaces are present and have the right offset */
        for (iIntf = 0; class->ifaces[iIntf].iid; iIntf++)
        {
            PCKNOWN_INTERFACE iface = FindInterface(class->ifaces[iIntf].iid);
            LONG offset = GetInterfaceOffset(pUnk, iface->iid);
            if (offset == INTF_NOT_EXPOSED)
                ok(0, "%s is missing %s (offset %ld)\n", class->name, iface->name, class->ifaces[iIntf].offset);
            else if (class->ifaces[iIntf].offset != FARAWY)
            {
#ifdef FAIL_WRONG_OFFSET
                ok(offset == class->ifaces[iIntf].offset, "%s, %s offset is %ld, expected %ld\n", class->name, iface->name, offset, class->ifaces[iIntf].offset);
#else
                if (offset != class->ifaces[iIntf].offset)
                    mytrace("%s, %s offset is %ld, expected %ld\n", class->name, iface->name, offset, class->ifaces[iIntf].offset);
#endif
            }
        }

        /* Check that none other than the expected interfaces are present */
        for (iIntf = 0; iIntf < KnownInterfaceCount; iIntf++)
        {
            PCKNOWN_INTERFACE iface = &KnownInterfaces[iIntf];
            LONG offset;
            if (IsInterfaceExpected(class, iface->iid))
                continue;
            offset = GetInterfaceOffset(pUnk, iface->iid);
#ifdef GENERATE_TABLE_ENTRIES
            ok(offset == INTF_NOT_EXPOSED, "%s: { %s0x%lx,   &%s },\n", class->name, offset < 0 ? "-" : "", offset < 0 ? -offset : offset, iface->name);
#else
            ok(offset == INTF_NOT_EXPOSED, "%s exposes %s (offset %ld), but shouldn't\n", class->name, iface->name, offset);
#endif
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
    _In_ INT ExpectedInterfaceCount)
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
    _In_ INT ExpectedInterfaceCount)
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
TestClasses(
    _In_ PCWSTR ModuleName,
    _In_ PCCLASS_AND_INTERFACES ExpectedInterfaces,
    _In_ INT ExpectedInterfaceCount)
{
    HRESULT hr;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ok(hr == S_OK, "CoInitializeEx failed. hr=0x%lx\n", hr);
    if (myskip(SUCCEEDED(hr), "Failed to initialize COM. Cannot perform tests\n"))
        return;

    TestModuleInterfaces(ExpectedInterfaces, ExpectedInterfaceCount);
    TestModuleRegistry(ModuleName, ExpectedInterfaces, ExpectedInterfaceCount);
    TestManualInstantiation(ModuleName, ExpectedInterfaces, ExpectedInterfaceCount);

    CoUninitialize();
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

    result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Classes\\Interface", 0, KEY_ENUMERATE_SUB_KEYS, &hKeyInterface);
    ok(result == NO_ERROR, "Failed to open interface key, error %lu\n", result);
    if (!myskip(result == NO_ERROR, "No interface key\n"))
    {
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
            if (iface->noreg)
            {
                ok(result == ERROR_FILE_NOT_FOUND, "RegOpenKeyEx returned %lu for %s\n", result, iface->name);
            }
            else
            {
                ok(result == NO_ERROR, "Failed to open key for %s, error %lu\n", iface->name, result);
                (void)myskip(result == NO_ERROR, "No key\n");
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
