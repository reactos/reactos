3 stdcall HlinkCreateFromMoniker(ptr wstr wstr ptr long ptr ptr ptr)
4 stdcall HlinkCreateFromString(wstr wstr wstr ptr long ptr ptr ptr)
5 stdcall HlinkCreateFromData(ptr ptr long ptr ptr ptr)
6 stdcall HlinkCreateBrowseContext(ptr ptr ptr)
7 stdcall HlinkClone(ptr ptr ptr long ptr)
8 stdcall HlinkNavigateToStringReference(wstr wstr ptr long ptr long ptr ptr ptr)
9 stdcall HlinkOnNavigate(ptr ptr long ptr wstr wstr ptr)
10 stdcall HlinkNavigate(ptr ptr long ptr ptr ptr)
11 stdcall HlinkUpdateStackItem(ptr ptr long ptr wstr wstr)
12 stub HlinkOnRenameDocument
14 stdcall HlinkResolveMonikerForData(ptr long ptr long ptr ptr ptr)
15 stub HlinkResolveStringForData
16 stub OleSaveToStreamEx
18 stdcall HlinkParseDisplayName(ptr wstr long ptr ptr)
20 stdcall HlinkQueryCreateFromData(ptr)
21 stub HlinkSetSpecialReference
22 stdcall HlinkGetSpecialReference(long ptr)
23 stub HlinkCreateShortcut
24 stub HlinkResolveShortcut
25 stdcall HlinkIsShortcut(wstr)
26 stub HlinkResolveShortcutToString
27 stub HlinkCreateShortcutFromString
28 stub HlinkGetValueFromParams
29 stub HlinkCreateShortcutFromMoniker
30 stub HlinkResolveShortcutToMoniker
31 stdcall HlinkTranslateURL(wstr long ptr)
32 stdcall HlinkCreateExtensionServices(wstr long wstr wstr ptr ptr ptr)
33 stub HlinkPreprocessMoniker

@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
